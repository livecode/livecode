/* libs/graphics/ports/SkFontHost_FreeType.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "SkColorPriv.h"
#include "SkScalerContext.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkDescriptor.h"
#include "SkFDot6.h"
#include "SkFontHost.h"
#include "SkMask.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkThread.h"
#include "SkTemplates.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_SIZES_H
#include FT_TRUETYPE_TABLES_H
#include FT_BITMAP_H
// In the past, FT_GlyphSlot_Own_Bitmap was defined in this header file.
#include FT_SYNTHESIS_H

#if defined(SK_SUPPORT_LCDTEXT)
#include FT_LCD_FILTER_H
#endif

#ifdef   FT_ADVANCES_H
#include FT_ADVANCES_H
#endif

#if 0
// Also include the files by name for build tools which require this.
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <freetype/ftsizes.h>
#include <freetype/tttables.h>
#include <freetype/ftadvanc.h>
#include <freetype/ftlcdfil.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftsynth.h>
#endif

//#define ENABLE_GLYPH_SPEW     // for tracing calls
//#define DUMP_STRIKE_CREATION

#ifdef ERROR
#undef ERROR
#endif

#ifdef SK_DEBUG
    #define SkASSERT_CONTINUE(pred)                                                         \
        do {                                                                                \
            if (!(pred))                                                                    \
                SkDebugf("file %s:%d: assert failed '" #pred "'\n", __FILE__, __LINE__);    \
        } while (false)
#else
    #define SkASSERT_CONTINUE(pred)
#endif

//////////////////////////////////////////////////////////////////////////

struct SkFaceRec;

static SkMutex      gFTMutex;
static int          gFTCount;
static FT_Library   gFTLibrary;
static SkFaceRec*   gFaceRecHead;
static bool         gLCDSupportValid;  // true iff |gLCDSupport| has been set.
static bool         gLCDSupport;  // true iff LCD is supported by the runtime.

/////////////////////////////////////////////////////////////////////////

// See http://freetype.sourceforge.net/freetype2/docs/reference/ft2-bitmap_handling.html#FT_Bitmap_Embolden
// This value was chosen by eyeballing the result in Firefox and trying to match it.
static const FT_Pos kBitmapEmboldenStrength = 1 << 6;

static bool
InitFreetype() {
    FT_Error err = FT_Init_FreeType(&gFTLibrary);
    if (err)
        return false;

#if defined(SK_SUPPORT_LCDTEXT)
    // Setup LCD filtering. This reduces colour fringes for LCD rendered
    // glyphs.
    err = FT_Library_SetLcdFilter(gFTLibrary, FT_LCD_FILTER_DEFAULT);
    gLCDSupport = err == 0;
#endif
    gLCDSupportValid = true;

    return true;
}

class SkScalerContext_FreeType : public SkScalerContext {
public:
    SkScalerContext_FreeType(const SkDescriptor* desc);
    virtual ~SkScalerContext_FreeType();

    bool success() const {
        return fFaceRec != NULL &&
               fFTSize != NULL &&
               fFace != NULL;
    }

protected:
    virtual unsigned generateGlyphCount() const;
    virtual uint16_t generateCharToGlyph(SkUnichar uni);
    virtual void generateAdvance(SkGlyph* glyph);
    virtual void generateMetrics(SkGlyph* glyph);
    virtual void generateImage(const SkGlyph& glyph);
    virtual void generatePath(const SkGlyph& glyph, SkPath* path);
    virtual void generateFontMetrics(SkPaint::FontMetrics* mx,
                                     SkPaint::FontMetrics* my);
    virtual SkUnichar generateGlyphToChar(uint16_t glyph);

private:
    SkFaceRec*  fFaceRec;
    FT_Face     fFace;              // reference to shared face in gFaceRecHead
    FT_Size     fFTSize;            // our own copy
    SkFixed     fScaleX, fScaleY;
    FT_Matrix   fMatrix22;
    uint32_t    fLoadGlyphFlags;

    FT_Error setupSize();
    void emboldenOutline(FT_Outline* outline);
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#include "SkStream.h"

struct SkFaceRec {
    SkFaceRec*      fNext;
    FT_Face         fFace;
    FT_StreamRec    fFTStream;
    SkStream*       fSkStream;
    uint32_t        fRefCnt;
    uint32_t        fFontID;

    // assumes ownership of the stream, will call unref() when its done
    SkFaceRec(SkStream* strm, uint32_t fontID);
    ~SkFaceRec() {
        fSkStream->unref();
    }
};

extern "C" {
    static unsigned long sk_stream_read(FT_Stream       stream,
                                        unsigned long   offset,
                                        unsigned char*  buffer,
                                        unsigned long   count ) {
        SkStream* str = (SkStream*)stream->descriptor.pointer;

        if (count) {
            if (!str->rewind()) {
                return 0;
            } else {
                unsigned long ret;
                if (offset) {
                    ret = str->read(NULL, offset);
                    if (ret != offset) {
                        return 0;
                    }
                }
                ret = str->read(buffer, count);
                if (ret != count) {
                    return 0;
                }
                count = ret;
            }
        }
        return count;
    }

    static void sk_stream_close( FT_Stream stream) {}
}

SkFaceRec::SkFaceRec(SkStream* strm, uint32_t fontID)
        : fSkStream(strm), fFontID(fontID) {
//    SkDEBUGF(("SkFaceRec: opening %s (%p)\n", key.c_str(), strm));

    sk_bzero(&fFTStream, sizeof(fFTStream));
    fFTStream.size = fSkStream->getLength();
    fFTStream.descriptor.pointer = fSkStream;
    fFTStream.read  = sk_stream_read;
    fFTStream.close = sk_stream_close;
}

// Will return 0 on failure
static SkFaceRec* ref_ft_face(uint32_t fontID) {
    SkFaceRec* rec = gFaceRecHead;
    while (rec) {
        if (rec->fFontID == fontID) {
            SkASSERT(rec->fFace);
            rec->fRefCnt += 1;
            return rec;
        }
        rec = rec->fNext;
    }

    SkStream* strm = SkFontHost::OpenStream(fontID);
    if (NULL == strm) {
        SkDEBUGF(("SkFontHost::OpenStream failed opening %x\n", fontID));
        return 0;
    }

    // this passes ownership of strm to the rec
    rec = SkNEW_ARGS(SkFaceRec, (strm, fontID));

    FT_Open_Args    args;
    memset(&args, 0, sizeof(args));
    const void* memoryBase = strm->getMemoryBase();

    if (NULL != memoryBase) {
//printf("mmap(%s)\n", keyString.c_str());
        args.flags = FT_OPEN_MEMORY;
        args.memory_base = (const FT_Byte*)memoryBase;
        args.memory_size = strm->getLength();
    } else {
//printf("fopen(%s)\n", keyString.c_str());
        args.flags = FT_OPEN_STREAM;
        args.stream = &rec->fFTStream;
    }

    int face_index;
    int length = SkFontHost::GetFileName(fontID, NULL, 0, &face_index);
    FT_Error err = FT_Open_Face(gFTLibrary, &args, length ? face_index : 0,
                                &rec->fFace);

    if (err) {    // bad filename, try the default font
        fprintf(stderr, "ERROR: unable to open font '%x'\n", fontID);
        SkDELETE(rec);
        return 0;
    } else {
        SkASSERT(rec->fFace);
        //fprintf(stderr, "Opened font '%s'\n", filename.c_str());
        rec->fNext = gFaceRecHead;
        gFaceRecHead = rec;
        rec->fRefCnt = 1;
        return rec;
    }
}

static void unref_ft_face(FT_Face face) {
    SkFaceRec*  rec = gFaceRecHead;
    SkFaceRec*  prev = NULL;
    while (rec) {
        SkFaceRec* next = rec->fNext;
        if (rec->fFace == face) {
            if (--rec->fRefCnt == 0) {
                if (prev) {
                    prev->fNext = next;
                } else {
                    gFaceRecHead = next;
                }
                FT_Done_Face(face);
                SkDELETE(rec);
            }
            return;
        }
        prev = rec;
        rec = next;
    }
    SkASSERT("shouldn't get here, face not in list");
}

///////////////////////////////////////////////////////////////////////////

void SkFontHost::FilterRec(SkScalerContext::Rec* rec) {
    if (!gLCDSupportValid) {
      InitFreetype();
      FT_Done_FreeType(gFTLibrary);
    }

    if (!gLCDSupport && rec->isLCD()) {
      // If the runtime Freetype library doesn't support LCD mode, we disable
      // it here.
      rec->fMaskFormat = SkMask::kA8_Format;
    }

    SkPaint::Hinting h = rec->getHinting();
    if (SkPaint::kFull_Hinting == h && !rec->isLCD()) {
        // collapse full->normal hinting if we're not doing LCD
        h = SkPaint::kNormal_Hinting;
    } else if ((rec->fFlags & SkScalerContext::kSubpixelPositioning_Flag) &&
               SkPaint::kNo_Hinting != h) {
        // to do subpixel, we must have at most slight hinting
        h = SkPaint::kSlight_Hinting;
    }
    rec->setHinting(h);
}

SkScalerContext_FreeType::SkScalerContext_FreeType(const SkDescriptor* desc)
        : SkScalerContext(desc) {
    SkAutoMutexAcquire  ac(gFTMutex);

    if (gFTCount == 0) {
        if (!InitFreetype()) {
            sk_throw();
        }
    }
    ++gFTCount;

    // load the font file
    fFTSize = NULL;
    fFace = NULL;
    fFaceRec = ref_ft_face(fRec.fFontID);
    if (NULL == fFaceRec) {
        return;
    }
    fFace = fFaceRec->fFace;

    // compute our factors from the record

    SkMatrix    m;

    fRec.getSingleMatrix(&m);

#ifdef DUMP_STRIKE_CREATION
    SkString     keyString;
    SkFontHost::GetDescriptorKeyString(desc, &keyString);
    printf("========== strike [%g %g %g] [%g %g %g %g] hints %d format %d %s\n", SkScalarToFloat(fRec.fTextSize),
           SkScalarToFloat(fRec.fPreScaleX), SkScalarToFloat(fRec.fPreSkewX),
           SkScalarToFloat(fRec.fPost2x2[0][0]), SkScalarToFloat(fRec.fPost2x2[0][1]),
           SkScalarToFloat(fRec.fPost2x2[1][0]), SkScalarToFloat(fRec.fPost2x2[1][1]),
           fRec.getHinting(), fRec.fMaskFormat, keyString.c_str());
#endif

    //  now compute our scale factors
    SkScalar    sx = m.getScaleX();
    SkScalar    sy = m.getScaleY();

    if (m.getSkewX() || m.getSkewY() || sx < 0 || sy < 0) {
        // sort of give up on hinting
        sx = SkMaxScalar(SkScalarAbs(sx), SkScalarAbs(m.getSkewX()));
        sy = SkMaxScalar(SkScalarAbs(m.getSkewY()), SkScalarAbs(sy));
        sx = sy = SkScalarAve(sx, sy);

        SkScalar inv = SkScalarInvert(sx);

        // flip the skew elements to go from our Y-down system to FreeType's
        fMatrix22.xx = SkScalarToFixed(SkScalarMul(m.getScaleX(), inv));
        fMatrix22.xy = -SkScalarToFixed(SkScalarMul(m.getSkewX(), inv));
        fMatrix22.yx = -SkScalarToFixed(SkScalarMul(m.getSkewY(), inv));
        fMatrix22.yy = SkScalarToFixed(SkScalarMul(m.getScaleY(), inv));
    } else {
        fMatrix22.xx = fMatrix22.yy = SK_Fixed1;
        fMatrix22.xy = fMatrix22.yx = 0;
    }

    fScaleX = SkScalarToFixed(sx);
    fScaleY = SkScalarToFixed(sy);

    // compute the flags we send to Load_Glyph
    {
        FT_Int32 loadFlags = FT_LOAD_DEFAULT;

        if (SkMask::kBW_Format == fRec.fMaskFormat) {
            // See http://code.google.com/p/chromium/issues/detail?id=43252#c24
            loadFlags = FT_LOAD_TARGET_MONO;
            if (fRec.getHinting() == SkPaint::kNo_Hinting)
                loadFlags = FT_LOAD_NO_HINTING;
        } else {
            switch (fRec.getHinting()) {
            case SkPaint::kNo_Hinting:
                loadFlags = FT_LOAD_NO_HINTING;
                break;
            case SkPaint::kSlight_Hinting:
                loadFlags = FT_LOAD_TARGET_LIGHT;  // This implies FORCE_AUTOHINT
                break;
            case SkPaint::kNormal_Hinting:
                if (fRec.fFlags & SkScalerContext::kAutohinting_Flag)
                    loadFlags = FT_LOAD_FORCE_AUTOHINT;
                else
                    loadFlags = FT_LOAD_NO_AUTOHINT;
                break;
            case SkPaint::kFull_Hinting:
                if (fRec.fFlags & SkScalerContext::kAutohinting_Flag) {
                    loadFlags = FT_LOAD_FORCE_AUTOHINT;
                    break;
                }
                loadFlags = FT_LOAD_TARGET_NORMAL;
                if (SkMask::kHorizontalLCD_Format == fRec.fMaskFormat)
                    loadFlags = FT_LOAD_TARGET_LCD;
                else if (SkMask::kVerticalLCD_Format == fRec.fMaskFormat)
                    loadFlags = FT_LOAD_TARGET_LCD_V;
                break;
            default:
                SkDebugf("---------- UNKNOWN hinting %d\n", fRec.getHinting());
                break;
            }
        }

        if ((fRec.fFlags & SkScalerContext::kEmbeddedBitmapText_Flag) == 0)
            loadFlags |= FT_LOAD_NO_BITMAP;

        fLoadGlyphFlags = loadFlags;
    }

    // now create the FT_Size

    {
        FT_Error    err;

        err = FT_New_Size(fFace, &fFTSize);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_New_Size(%x): FT_Set_Char_Size(0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fFontID, fScaleX, fScaleY, err));
            fFace = NULL;
            return;
        }

        err = FT_Activate_Size(fFTSize);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_Activate_Size(%x, 0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fFontID, fScaleX, fScaleY, err));
            fFTSize = NULL;
        }

        err = FT_Set_Char_Size( fFace,
                                SkFixedToFDot6(fScaleX), SkFixedToFDot6(fScaleY),
                                72, 72);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_Set_Char_Size(%x, 0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fFontID, fScaleX, fScaleY, err));
            fFace = NULL;
            return;
        }

        FT_Set_Transform( fFace, &fMatrix22, NULL);
    }
}

SkScalerContext_FreeType::~SkScalerContext_FreeType() {
    if (fFTSize != NULL) {
        FT_Done_Size(fFTSize);
    }

    SkAutoMutexAcquire  ac(gFTMutex);

    if (fFace != NULL) {
        unref_ft_face(fFace);
    }
    if (--gFTCount == 0) {
//        SkDEBUGF(("FT_Done_FreeType\n"));
        FT_Done_FreeType(gFTLibrary);
        SkDEBUGCODE(gFTLibrary = NULL;)
    }
}

/*  We call this before each use of the fFace, since we may be sharing
    this face with other context (at different sizes).
*/
FT_Error SkScalerContext_FreeType::setupSize() {
    /*  In the off-chance that a font has been removed, we want to error out
        right away, so call resolve just to be sure.

        TODO: perhaps we can skip this, by walking the global font cache and
        killing all of the contexts when we know that a given fontID is going
        away...
     */
    if (!SkFontHost::ValidFontID(fRec.fFontID)) {
        return (FT_Error)-1;
    }

    FT_Error    err = FT_Activate_Size(fFTSize);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::FT_Activate_Size(%x, 0x%x, 0x%x) returned 0x%x\n",
                    fFaceRec->fFontID, fScaleX, fScaleY, err));
        fFTSize = NULL;
    } else {
        // seems we need to reset this every time (not sure why, but without it
        // I get random italics from some other fFTSize)
        FT_Set_Transform( fFace, &fMatrix22, NULL);
    }
    return err;
}

void SkScalerContext_FreeType::emboldenOutline(FT_Outline* outline) {
    FT_Pos strength;
    strength = FT_MulFix(fFace->units_per_EM, fFace->size->metrics.y_scale)
               / 24;
    FT_Outline_Embolden(outline, strength);
}

unsigned SkScalerContext_FreeType::generateGlyphCount() const {
    return fFace->num_glyphs;
}

uint16_t SkScalerContext_FreeType::generateCharToGlyph(SkUnichar uni) {
    return SkToU16(FT_Get_Char_Index( fFace, uni ));
}

SkUnichar SkScalerContext_FreeType::generateGlyphToChar(uint16_t glyph) {
    // iterate through each cmap entry, looking for matching glyph indices
    FT_UInt glyphIndex;
    SkUnichar charCode = FT_Get_First_Char( fFace, &glyphIndex );

    while (glyphIndex != 0) {
        if (glyphIndex == glyph) {
            return charCode;
        }
        charCode = FT_Get_Next_Char( fFace, charCode, &glyphIndex );
    }

    return 0;
}

static FT_Pixel_Mode compute_pixel_mode(SkMask::Format format) {
    switch (format) {
        case SkMask::kHorizontalLCD_Format:
        case SkMask::kVerticalLCD_Format:
            SkASSERT(!"An LCD format should never be passed here");
            return FT_PIXEL_MODE_GRAY;
        case SkMask::kBW_Format:
            return FT_PIXEL_MODE_MONO;
        case SkMask::kA8_Format:
        default:
            return FT_PIXEL_MODE_GRAY;
    }
}

void SkScalerContext_FreeType::generateAdvance(SkGlyph* glyph) {
#ifdef FT_ADVANCES_H
   /* unhinted and light hinted text have linearly scaled advances
    * which are very cheap to compute with some font formats...
    */
    {
        SkAutoMutexAcquire  ac(gFTMutex);

        if (this->setupSize()) {
            glyph->zeroMetrics();
            return;
        }

        FT_Error    error;
        FT_Fixed    advance;

        error = FT_Get_Advance( fFace, glyph->getGlyphID(fBaseGlyphCount),
                                fLoadGlyphFlags | FT_ADVANCE_FLAG_FAST_ONLY,
                                &advance );
        if (0 == error) {
            glyph->fRsbDelta = 0;
            glyph->fLsbDelta = 0;
            glyph->fAdvanceX = advance;  // advance *2/3; //DEBUG
            glyph->fAdvanceY = 0;
            return;
        }
    }
#endif /* FT_ADVANCES_H */
    /* otherwise, we need to load/hint the glyph, which is slower */
    this->generateMetrics(glyph);
    return;
}

void SkScalerContext_FreeType::generateMetrics(SkGlyph* glyph) {
    SkAutoMutexAcquire  ac(gFTMutex);

    glyph->fRsbDelta = 0;
    glyph->fLsbDelta = 0;

    FT_Error    err;

    if (this->setupSize()) {
        goto ERROR;
    }

    err = FT_Load_Glyph( fFace, glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags );
    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generateMetrics(%x): FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    fFaceRec->fFontID, glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags, err));
    ERROR:
        glyph->zeroMetrics();
        return;
    }

    switch ( fFace->glyph->format ) {
      case FT_GLYPH_FORMAT_OUTLINE:
        FT_BBox bbox;

        if (fRec.fFlags & kEmbolden_Flag) {
            emboldenOutline(&fFace->glyph->outline);
        }
        FT_Outline_Get_CBox(&fFace->glyph->outline, &bbox);

        if (fRec.fFlags & SkScalerContext::kSubpixelPositioning_Flag) {
            int dx = glyph->getSubXFixed() >> 10;
            int dy = glyph->getSubYFixed() >> 10;
            // negate dy since freetype-y-goes-up and skia-y-goes-down
            bbox.xMin += dx;
            bbox.yMin -= dy;
            bbox.xMax += dx;
            bbox.yMax -= dy;
        }

        bbox.xMin &= ~63;
        bbox.yMin &= ~63;
        bbox.xMax  = (bbox.xMax + 63) & ~63;
        bbox.yMax  = (bbox.yMax + 63) & ~63;

        glyph->fWidth   = SkToU16((bbox.xMax - bbox.xMin) >> 6);
        glyph->fHeight  = SkToU16((bbox.yMax - bbox.yMin) >> 6);
        glyph->fTop     = -SkToS16(bbox.yMax >> 6);
        glyph->fLeft    = SkToS16(bbox.xMin >> 6);
        break;

      case FT_GLYPH_FORMAT_BITMAP:
        if (fRec.fFlags & kEmbolden_Flag) {
            FT_GlyphSlot_Own_Bitmap(fFace->glyph);
            FT_Bitmap_Embolden(gFTLibrary, &fFace->glyph->bitmap, kBitmapEmboldenStrength, 0);
        }
        glyph->fWidth   = SkToU16(fFace->glyph->bitmap.width);
        glyph->fHeight  = SkToU16(fFace->glyph->bitmap.rows);
        glyph->fTop     = -SkToS16(fFace->glyph->bitmap_top);
        glyph->fLeft    = SkToS16(fFace->glyph->bitmap_left);
        break;

      default:
        SkASSERT(!"unknown glyph format");
        goto ERROR;
    }

    if ((fRec.fFlags & SkScalerContext::kSubpixelPositioning_Flag) == 0) {
        glyph->fAdvanceX = SkFDot6ToFixed(fFace->glyph->advance.x);
        glyph->fAdvanceY = -SkFDot6ToFixed(fFace->glyph->advance.y);
        if (fRec.fFlags & kDevKernText_Flag) {
            glyph->fRsbDelta = SkToS8(fFace->glyph->rsb_delta);
            glyph->fLsbDelta = SkToS8(fFace->glyph->lsb_delta);
        }
    } else {
        glyph->fAdvanceX = SkFixedMul(fMatrix22.xx, fFace->glyph->linearHoriAdvance);
        glyph->fAdvanceY = -SkFixedMul(fMatrix22.yx, fFace->glyph->linearHoriAdvance);
    }

#ifdef ENABLE_GLYPH_SPEW
    SkDEBUGF(("FT_Set_Char_Size(this:%p sx:%x sy:%x ", this, fScaleX, fScaleY));
    SkDEBUGF(("Metrics(glyph:%d flags:0x%x) w:%d\n", glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags, glyph->fWidth));
#endif
}

#if defined(SK_SUPPORT_LCDTEXT)
namespace skia_freetype_support {
// extern functions from SkFontHost_FreeType_Subpixel
extern void CopyFreetypeBitmapToLCDMask(const SkGlyph& dest, const FT_Bitmap& source);
extern void CopyFreetypeBitmapToVerticalLCDMask(const SkGlyph& dest, const FT_Bitmap& source);
}

using namespace skia_freetype_support;
#endif

void SkScalerContext_FreeType::generateImage(const SkGlyph& glyph) {
    SkAutoMutexAcquire  ac(gFTMutex);

    FT_Error    err;

    if (this->setupSize()) {
        goto ERROR;
    }

    err = FT_Load_Glyph( fFace, glyph.getGlyphID(fBaseGlyphCount), fLoadGlyphFlags);
    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generateImage: FT_Load_Glyph(glyph:%d width:%d height:%d rb:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), glyph.fWidth, glyph.fHeight, glyph.rowBytes(), fLoadGlyphFlags, err));
    ERROR:
        memset(glyph.fImage, 0, glyph.rowBytes() * glyph.fHeight);
        return;
    }

    const bool lcdRenderMode = fRec.fMaskFormat == SkMask::kHorizontalLCD_Format ||
                               fRec.fMaskFormat == SkMask::kVerticalLCD_Format;

    switch ( fFace->glyph->format ) {
        case FT_GLYPH_FORMAT_OUTLINE: {
            FT_Outline* outline = &fFace->glyph->outline;
            FT_BBox     bbox;
            FT_Bitmap   target;

            if (fRec.fFlags & kEmbolden_Flag) {
                emboldenOutline(outline);
            }

            int dx = 0, dy = 0;
            if (fRec.fFlags & SkScalerContext::kSubpixelPositioning_Flag) {
                dx = glyph.getSubXFixed() >> 10;
                dy = glyph.getSubYFixed() >> 10;
                // negate dy since freetype-y-goes-up and skia-y-goes-down
                dy = -dy;
            }
            FT_Outline_Get_CBox(outline, &bbox);
            /*
                what we really want to do for subpixel is
                    offset(dx, dy)
                    compute_bounds
                    offset(bbox & !63)
                but that is two calls to offset, so we do the following, which
                achieves the same thing with only one offset call.
            */
            FT_Outline_Translate(outline, dx - ((bbox.xMin + dx) & ~63),
                                          dy - ((bbox.yMin + dy) & ~63));

#if defined(SK_SUPPORT_LCDTEXT)
            if (lcdRenderMode) {
                // FT_Outline_Get_Bitmap cannot render LCD glyphs. In this case
                // we have to call FT_Render_Glyph and memcpy the image out.
                const bool isVertical = fRec.fMaskFormat == SkMask::kVerticalLCD_Format;
                FT_Render_Mode mode = isVertical ? FT_RENDER_MODE_LCD_V : FT_RENDER_MODE_LCD;
                FT_Render_Glyph(fFace->glyph, mode);

                if (isVertical)
                    CopyFreetypeBitmapToVerticalLCDMask(glyph, fFace->glyph->bitmap);
                else
                    CopyFreetypeBitmapToLCDMask(glyph, fFace->glyph->bitmap);

                break;
            }
#endif

            target.width = glyph.fWidth;
            target.rows = glyph.fHeight;
            target.pitch = glyph.rowBytes();
            target.buffer = reinterpret_cast<uint8_t*>(glyph.fImage);
            target.pixel_mode = compute_pixel_mode(
                                            (SkMask::Format)fRec.fMaskFormat);
            target.num_grays = 256;

            memset(glyph.fImage, 0, glyph.rowBytes() * glyph.fHeight);
            FT_Outline_Get_Bitmap(gFTLibrary, outline, &target);
        } break;

        case FT_GLYPH_FORMAT_BITMAP: {
            if (fRec.fFlags & kEmbolden_Flag) {
                FT_GlyphSlot_Own_Bitmap(fFace->glyph);
                FT_Bitmap_Embolden(gFTLibrary, &fFace->glyph->bitmap, kBitmapEmboldenStrength, 0);
            }
            SkASSERT_CONTINUE(glyph.fWidth == fFace->glyph->bitmap.width);
            SkASSERT_CONTINUE(glyph.fHeight == fFace->glyph->bitmap.rows);
            SkASSERT_CONTINUE(glyph.fTop == -fFace->glyph->bitmap_top);
            SkASSERT_CONTINUE(glyph.fLeft == fFace->glyph->bitmap_left);

            const uint8_t*  src = (const uint8_t*)fFace->glyph->bitmap.buffer;
            uint8_t*        dst = (uint8_t*)glyph.fImage;

            if (fFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY ||
                (fFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO &&
                 glyph.fMaskFormat == SkMask::kBW_Format)) {
                unsigned    srcRowBytes = fFace->glyph->bitmap.pitch;
                unsigned    dstRowBytes = glyph.rowBytes();
                unsigned    minRowBytes = SkMin32(srcRowBytes, dstRowBytes);
                unsigned    extraRowBytes = dstRowBytes - minRowBytes;

                for (int y = fFace->glyph->bitmap.rows - 1; y >= 0; --y) {
                    memcpy(dst, src, minRowBytes);
                    memset(dst + minRowBytes, 0, extraRowBytes);
                    src += srcRowBytes;
                    dst += dstRowBytes;
                }
            } else if (fFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO &&
                       (glyph.fMaskFormat == SkMask::kA8_Format ||
                        glyph.fMaskFormat == SkMask::kHorizontalLCD_Format ||
                        glyph.fMaskFormat == SkMask::kVerticalLCD_Format)) {
                for (int y = 0; y < fFace->glyph->bitmap.rows; ++y) {
                    uint8_t byte = 0;
                    int bits = 0;
                    const uint8_t* src_row = src;
                    uint8_t* dst_row = dst;

                    for (int x = 0; x < fFace->glyph->bitmap.width; ++x) {
                        if (!bits) {
                            byte = *src_row++;
                            bits = 8;
                        }

                        *dst_row++ = byte & 0x80 ? 0xff : 0;
                        bits--;
                        byte <<= 1;
                    }

                    src += fFace->glyph->bitmap.pitch;
                    dst += glyph.rowBytes();
                }
            } else {
              SkASSERT(!"unknown glyph bitmap transform needed");
            }

            if (lcdRenderMode)
                glyph.expandA8ToLCD();

        } break;

    default:
        SkASSERT(!"unknown glyph format");
        goto ERROR;
    }
}

///////////////////////////////////////////////////////////////////////////////

#define ft2sk(x)    SkFixedToScalar((x) << 10)

#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 2
    #define CONST_PARAM const
#else   // older freetype doesn't use const here
    #define CONST_PARAM
#endif

static int move_proc(CONST_PARAM FT_Vector* pt, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->close();  // to close the previous contour (if any)
    path->moveTo(ft2sk(pt->x), -ft2sk(pt->y));
    return 0;
}

static int line_proc(CONST_PARAM FT_Vector* pt, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->lineTo(ft2sk(pt->x), -ft2sk(pt->y));
    return 0;
}

static int quad_proc(CONST_PARAM FT_Vector* pt0, CONST_PARAM FT_Vector* pt1,
                     void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->quadTo(ft2sk(pt0->x), -ft2sk(pt0->y), ft2sk(pt1->x), -ft2sk(pt1->y));
    return 0;
}

static int cubic_proc(CONST_PARAM FT_Vector* pt0, CONST_PARAM FT_Vector* pt1,
                      CONST_PARAM FT_Vector* pt2, void* ctx) {
    SkPath* path = (SkPath*)ctx;
    path->cubicTo(ft2sk(pt0->x), -ft2sk(pt0->y), ft2sk(pt1->x),
                  -ft2sk(pt1->y), ft2sk(pt2->x), -ft2sk(pt2->y));
    return 0;
}

void SkScalerContext_FreeType::generatePath(const SkGlyph& glyph,
                                            SkPath* path) {
    SkAutoMutexAcquire  ac(gFTMutex);

    SkASSERT(&glyph && path);

    if (this->setupSize()) {
        path->reset();
        return;
    }

    uint32_t flags = fLoadGlyphFlags;
    flags |= FT_LOAD_NO_BITMAP; // ignore embedded bitmaps so we're sure to get the outline
    flags &= ~FT_LOAD_RENDER;   // don't scan convert (we just want the outline)

    FT_Error err = FT_Load_Glyph( fFace, glyph.getGlyphID(fBaseGlyphCount), flags);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generatePath: FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), flags, err));
        path->reset();
        return;
    }

    if (fRec.fFlags & kEmbolden_Flag) {
        emboldenOutline(&fFace->glyph->outline);
    }

    FT_Outline_Funcs    funcs;

    funcs.move_to   = move_proc;
    funcs.line_to   = line_proc;
    funcs.conic_to  = quad_proc;
    funcs.cubic_to  = cubic_proc;
    funcs.shift     = 0;
    funcs.delta     = 0;

    err = FT_Outline_Decompose(&fFace->glyph->outline, &funcs, path);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generatePath: FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), flags, err));
        path->reset();
        return;
    }

    path->close();
}

void SkScalerContext_FreeType::generateFontMetrics(SkPaint::FontMetrics* mx,
                                                   SkPaint::FontMetrics* my) {
    if (NULL == mx && NULL == my) {
        return;
    }

    SkAutoMutexAcquire  ac(gFTMutex);

    if (this->setupSize()) {
        ERROR:
        if (mx) {
            sk_bzero(mx, sizeof(SkPaint::FontMetrics));
        }
        if (my) {
            sk_bzero(my, sizeof(SkPaint::FontMetrics));
        }
        return;
    }

    FT_Face face = fFace;
    int upem = face->units_per_EM;
    if (upem <= 0) {
        goto ERROR;
    }

    SkPoint pts[6];
    SkFixed ys[6];
    SkFixed scaleY = fScaleY;
    SkFixed mxy = fMatrix22.xy;
    SkFixed myy = fMatrix22.yy;
    SkScalar xmin = SkIntToScalar(face->bbox.xMin) / upem;
    SkScalar xmax = SkIntToScalar(face->bbox.xMax) / upem;

    int leading = face->height - (face->ascender + -face->descender);
    if (leading < 0) {
        leading = 0;
    }

    // Try to get the OS/2 table from the font. This contains the specific
    // average font width metrics which Windows uses.
    TT_OS2* os2 = (TT_OS2*) FT_Get_Sfnt_Table(face, ft_sfnt_os2);

    ys[0] = -face->bbox.yMax;
    ys[1] = -face->ascender;
    ys[2] = -face->descender;
    ys[3] = -face->bbox.yMin;
    ys[4] = leading;
    ys[5] = os2 ? os2->xAvgCharWidth : 0;

    SkScalar x_height;
    if (os2 && os2->sxHeight) {
        x_height = SkFixedToScalar(SkMulDiv(fScaleX, os2->sxHeight, upem));
    } else {
        const FT_UInt x_glyph = FT_Get_Char_Index(fFace, 'x');
        if (x_glyph) {
            FT_BBox bbox;
            FT_Load_Glyph(fFace, x_glyph, fLoadGlyphFlags);
            if (fRec.fFlags & kEmbolden_Flag) {
                emboldenOutline(&fFace->glyph->outline);
            }
            FT_Outline_Get_CBox(&fFace->glyph->outline, &bbox);
            x_height = SkIntToScalar(bbox.yMax) / 64;
        } else {
            x_height = 0;
        }
    }

    // convert upem-y values into scalar points
    for (int i = 0; i < 6; i++) {
        SkFixed y = SkMulDiv(scaleY, ys[i], upem);
        SkFixed x = SkFixedMul(mxy, y);
        y = SkFixedMul(myy, y);
        pts[i].set(SkFixedToScalar(x), SkFixedToScalar(y));
    }

    if (mx) {
        mx->fTop = pts[0].fX;
        mx->fAscent = pts[1].fX;
        mx->fDescent = pts[2].fX;
        mx->fBottom = pts[3].fX;
        mx->fLeading = pts[4].fX;
        mx->fAvgCharWidth = pts[5].fX;
        mx->fXMin = xmin;
        mx->fXMax = xmax;
        mx->fXHeight = x_height;
    }
    if (my) {
        my->fTop = pts[0].fY;
        my->fAscent = pts[1].fY;
        my->fDescent = pts[2].fY;
        my->fBottom = pts[3].fY;
        my->fLeading = pts[4].fY;
        my->fAvgCharWidth = pts[5].fY;
        my->fXMin = xmin;
        my->fXMax = xmax;
        my->fXHeight = x_height;
    }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

SkScalerContext* SkFontHost::CreateScalerContext(const SkDescriptor* desc) {
    SkScalerContext_FreeType* c = SkNEW_ARGS(SkScalerContext_FreeType, (desc));
    if (!c->success()) {
        SkDELETE(c);
        c = NULL;
    }
    return c;
}

///////////////////////////////////////////////////////////////////////////////

/*  Export this so that other parts of our FonttHost port can make use of our
    ability to extract the name+style from a stream, using FreeType's api.
*/
SkTypeface::Style find_name_and_style(SkStream* stream, SkString* name) {
    FT_Library  library;
    if (FT_Init_FreeType(&library)) {
        name->set(NULL);
        return SkTypeface::kNormal;
    }

    FT_Open_Args    args;
    memset(&args, 0, sizeof(args));

    const void* memoryBase = stream->getMemoryBase();
    FT_StreamRec    streamRec;

    if (NULL != memoryBase) {
        args.flags = FT_OPEN_MEMORY;
        args.memory_base = (const FT_Byte*)memoryBase;
        args.memory_size = stream->getLength();
    } else {
        memset(&streamRec, 0, sizeof(streamRec));
        streamRec.size = stream->read(NULL, 0);
        streamRec.descriptor.pointer = stream;
        streamRec.read  = sk_stream_read;
        streamRec.close = sk_stream_close;

        args.flags = FT_OPEN_STREAM;
        args.stream = &streamRec;
    }

    FT_Face face;
    if (FT_Open_Face(library, &args, 0, &face)) {
        FT_Done_FreeType(library);
        name->set(NULL);
        return SkTypeface::kNormal;
    }

    name->set(face->family_name);
    int style = SkTypeface::kNormal;

    if (face->style_flags & FT_STYLE_FLAG_BOLD) {
        style |= SkTypeface::kBold;
    }
    if (face->style_flags & FT_STYLE_FLAG_ITALIC) {
        style |= SkTypeface::kItalic;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return (SkTypeface::Style)style;
}
