/* libs/graphics/sgl/SkPaint.cpp
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

#include "SkPaint.h"
#include "SkColorFilter.h"
#include "SkDrawLooper.h"
#include "SkFontHost.h"
#include "SkMaskFilter.h"
#include "SkPathEffect.h"
#include "SkRasterizer.h"
#include "SkShader.h"
#include "SkScalerContext.h"
#include "SkStroke.h"
#include "SkTypeface.h"
#include "SkXfermode.h"
#include "SkAutoKern.h"

#define SK_DefaultTextSize      SkIntToScalar(12)

#define SK_DefaultFlags         0   //(kNativeHintsText_Flag)

SkPaint::SkPaint() {
    // since we may have padding, we zero everything so that our memcmp() call
    // in operator== will work correctly.
    // with this, we can skip 0 and null individual initializations
    sk_bzero(this, sizeof(*this));

#if 0   // not needed with the bzero call above
    fTypeface   = NULL;
    fTextSkewX  = 0;
    fPathEffect  = NULL;
    fShader      = NULL;
    fXfermode    = NULL;
    fMaskFilter  = NULL;
    fColorFilter = NULL;
    fRasterizer  = NULL;
    fLooper      = NULL;
    fWidth      = 0;
#endif

    fTextSize   = SK_DefaultTextSize;
    fTextScaleX = SK_Scalar1;
    fColor      = SK_ColorBLACK;
    fMiterLimit = SK_DefaultMiterLimit;
    fFlags      = SK_DefaultFlags;
    fCapType    = kDefault_Cap;
    fJoinType   = kDefault_Join;
    fTextAlign  = kLeft_Align;
    fStyle      = kFill_Style;
    fTextEncoding = kUTF8_TextEncoding;
    fHinting    = kNormal_Hinting;
}

SkPaint::SkPaint(const SkPaint& src)
{
    memcpy(this, &src, sizeof(src));

    fTypeface->safeRef();
    fPathEffect->safeRef();
    fShader->safeRef();
    fXfermode->safeRef();
    fMaskFilter->safeRef();
    fColorFilter->safeRef();
    fRasterizer->safeRef();
    fLooper->safeRef();
}

SkPaint::~SkPaint()
{
    fTypeface->safeUnref();
    fPathEffect->safeUnref();
    fShader->safeUnref();
    fXfermode->safeUnref();
    fMaskFilter->safeUnref();
    fColorFilter->safeUnref();
    fRasterizer->safeUnref();
    fLooper->safeUnref();
}

SkPaint& SkPaint::operator=(const SkPaint& src)
{
    SkASSERT(&src);

    src.fTypeface->safeRef();
    src.fPathEffect->safeRef();
    src.fShader->safeRef();
    src.fXfermode->safeRef();
    src.fMaskFilter->safeRef();
    src.fColorFilter->safeRef();
    src.fRasterizer->safeRef();
    src.fLooper->safeRef();

    fTypeface->safeUnref();
    fPathEffect->safeUnref();
    fShader->safeUnref();
    fXfermode->safeUnref();
    fMaskFilter->safeUnref();
    fColorFilter->safeUnref();
    fRasterizer->safeUnref();
    fLooper->safeUnref();

    memcpy(this, &src, sizeof(src));

    return *this;
}

int operator==(const SkPaint& a, const SkPaint& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0;
}

void SkPaint::reset()
{
    SkPaint init;

    *this = init;
}

void SkPaint::setFlags(uint32_t flags)
{
    fFlags = flags;
}

void SkPaint::setAntiAlias(bool doAA)
{
    this->setFlags(SkSetClearMask(fFlags, doAA, kAntiAlias_Flag));
}

void SkPaint::setDither(bool doDither)
{
    this->setFlags(SkSetClearMask(fFlags, doDither, kDither_Flag));
}

void SkPaint::setSubpixelText(bool doSubpixel)
{
    this->setFlags(SkSetClearMask(fFlags, doSubpixel, kSubpixelText_Flag));
}

void SkPaint::setLCDRenderText(bool doLCDRender)
{
    this->setFlags(SkSetClearMask(fFlags, doLCDRender, kLCDRenderText_Flag));
}

void SkPaint::setEmbeddedBitmapText(bool doEmbeddedBitmapText)
{
    this->setFlags(SkSetClearMask(fFlags, doEmbeddedBitmapText, kEmbeddedBitmapText_Flag));
}

void SkPaint::setAutohinted(bool useAutohinter)
{
    this->setFlags(SkSetClearMask(fFlags, useAutohinter, kAutoHinting_Flag));
}

void SkPaint::setLinearText(bool doLinearText)
{
    this->setFlags(SkSetClearMask(fFlags, doLinearText, kLinearText_Flag));
}

void SkPaint::setUnderlineText(bool doUnderline)
{
    this->setFlags(SkSetClearMask(fFlags, doUnderline, kUnderlineText_Flag));
}

void SkPaint::setStrikeThruText(bool doStrikeThru)
{
    this->setFlags(SkSetClearMask(fFlags, doStrikeThru, kStrikeThruText_Flag));
}

void SkPaint::setFakeBoldText(bool doFakeBold)
{
    this->setFlags(SkSetClearMask(fFlags, doFakeBold, kFakeBoldText_Flag));
}

void SkPaint::setDevKernText(bool doDevKern)
{
    this->setFlags(SkSetClearMask(fFlags, doDevKern, kDevKernText_Flag));
}

void SkPaint::setFilterBitmap(bool doFilter)
{
    this->setFlags(SkSetClearMask(fFlags, doFilter, kFilterBitmap_Flag));
}

void SkPaint::setStyle(Style style)
{
    if ((unsigned)style < kStyleCount)
        fStyle = style;
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setStyle(%d) out of range\n", style);
#endif
}

void SkPaint::setColor(SkColor color)
{
    fColor = color;
}

void SkPaint::setAlpha(U8CPU a)
{
    fColor = SkColorSetARGB(a, SkColorGetR(fColor), SkColorGetG(fColor), SkColorGetB(fColor));
}

void SkPaint::setARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    fColor = SkColorSetARGB(a, r, g, b);
}

void SkPaint::setStrokeWidth(SkScalar width)
{
    if (width >= 0)
        fWidth = width;
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setStrokeWidth() called with negative value\n");
#endif
}

void SkPaint::setStrokeMiter(SkScalar limit)
{
    if (limit >= 0)
        fMiterLimit = limit;
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setStrokeMiter() called with negative value\n");
#endif
}

void SkPaint::setStrokeCap(Cap ct)
{
    if ((unsigned)ct < kCapCount)
        fCapType = SkToU8(ct);
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setStrokeCap(%d) out of range\n", ct);
#endif
}

void SkPaint::setStrokeJoin(Join jt)
{
    if ((unsigned)jt < kJoinCount)
        fJoinType = SkToU8(jt);
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setStrokeJoin(%d) out of range\n", jt);
#endif
}

//////////////////////////////////////////////////////////////////

void SkPaint::setTextAlign(Align align)
{
    if ((unsigned)align < kAlignCount)
        fTextAlign = SkToU8(align);
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setTextAlign(%d) out of range\n", align);
#endif
}

void SkPaint::setTextSize(SkScalar ts)
{
    if (ts > 0)
        fTextSize = ts;
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setTextSize() called with non-positive value\n");
#endif
}

void SkPaint::setTextScaleX(SkScalar scaleX)
{
    fTextScaleX = scaleX;
}

void SkPaint::setTextSkewX(SkScalar skewX)
{
    fTextSkewX = skewX;
}

void SkPaint::setTextEncoding(TextEncoding encoding)
{
    if ((unsigned)encoding <= kGlyphID_TextEncoding)
        fTextEncoding = encoding;
#ifdef SK_DEBUG
    else
        SkDebugf("SkPaint::setTextEncoding(%d) out of range\n", encoding);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

SkTypeface* SkPaint::setTypeface(SkTypeface* font)
{
    SkRefCnt_SafeAssign(fTypeface, font);
    return font;
}

SkRasterizer* SkPaint::setRasterizer(SkRasterizer* r)
{
    SkRefCnt_SafeAssign(fRasterizer, r);
    return r;
}

SkDrawLooper* SkPaint::setLooper(SkDrawLooper* looper)
{
    SkRefCnt_SafeAssign(fLooper, looper);
    return looper;
}

///////////////////////////////////////////////////////////////////////////////

#include "SkGlyphCache.h"
#include "SkUtils.h"

int SkPaint::textToGlyphs(const void* textData, size_t byteLength,
                          uint16_t glyphs[]) const {
    if (byteLength == 0) {
        return 0;
    }
    
    SkASSERT(textData != NULL);

    if (NULL == glyphs) {
        switch (this->getTextEncoding()) {
        case kUTF8_TextEncoding:
            return SkUTF8_CountUnichars((const char*)textData, byteLength);
        case kUTF16_TextEncoding:
            return SkUTF16_CountUnichars((const uint16_t*)textData,
                                         byteLength >> 1);
        case kGlyphID_TextEncoding:
            return byteLength >> 1;
        default:
            SkASSERT(!"unknown text encoding");
        }
        return 0;
    }
    
    // if we get here, we have a valid glyphs[] array, so time to fill it in
    
    // handle this encoding before the setup for the glyphcache
    if (this->getTextEncoding() == kGlyphID_TextEncoding) {
        // we want to ignore the low bit of byteLength
        memcpy(glyphs, textData, byteLength >> 1 << 1);
        return byteLength >> 1;
    }
    
    SkAutoGlyphCache autoCache(*this, NULL);
    SkGlyphCache*    cache = autoCache.getCache();

    const char* text = (const char*)textData;
    const char* stop = text + byteLength;
    uint16_t*   gptr = glyphs;

    switch (this->getTextEncoding()) {
        case SkPaint::kUTF8_TextEncoding:
            while (text < stop) {
                *gptr++ = cache->unicharToGlyph(SkUTF8_NextUnichar(&text));
            }
            break;
        case SkPaint::kUTF16_TextEncoding: {
            const uint16_t* text16 = (const uint16_t*)text;
            const uint16_t* stop16 = (const uint16_t*)stop;
            while (text16 < stop16) {
                *gptr++ = cache->unicharToGlyph(SkUTF16_NextUnichar(&text16));
            }
            break;
        }
        default:
            SkASSERT(!"unknown text encoding");
    }
    return gptr - glyphs;
}

bool SkPaint::containsText(const void* textData, size_t byteLength) const {
    if (0 == byteLength) {
        return true;
    }
    
    SkASSERT(textData != NULL);
    
    // handle this encoding before the setup for the glyphcache
    if (this->getTextEncoding() == kGlyphID_TextEncoding) {
        const uint16_t* glyphID = static_cast<const uint16_t*>(textData);
        size_t count = byteLength >> 1;
        for (size_t i = 0; i < count; i++) {
            if (0 == glyphID[i]) {
                return false;
            }
        }
        return true;
    }
    
    SkAutoGlyphCache autoCache(*this, NULL);
    SkGlyphCache*    cache = autoCache.getCache();

    switch (this->getTextEncoding()) {
        case SkPaint::kUTF8_TextEncoding: {
            const char* text = static_cast<const char*>(textData);
            const char* stop = text + byteLength;
            while (text < stop) {
                if (0 == cache->unicharToGlyph(SkUTF8_NextUnichar(&text))) {
                    return false;
                }
            }
            break;
        }
        case SkPaint::kUTF16_TextEncoding: {
            const uint16_t* text = static_cast<const uint16_t*>(textData);
            const uint16_t* stop = text + (byteLength >> 1);
            while (text < stop) {
                if (0 == cache->unicharToGlyph(SkUTF16_NextUnichar(&text))) {
                    return false;
                }
            }
            break;
        }
        default:
            SkASSERT(!"unknown text encoding");
            return false;
    }
    return true;
}

void SkPaint::glyphsToUnichars(const uint16_t glyphs[], int count,
                               SkUnichar textData[]) const {
    if (count <= 0) {
        return;
    }

    SkASSERT(glyphs != NULL);
    SkASSERT(textData != NULL);

    SkAutoGlyphCache autoCache(*this, NULL);
    SkGlyphCache*    cache = autoCache.getCache();

    for (int index = 0; index < count; index++) {
        textData[index] = cache->glyphToUnichar(glyphs[index]);
    }
}

///////////////////////////////////////////////////////////////////////////////

static const SkGlyph& sk_getMetrics_utf8_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF8_NextUnichar(text));
}

static const SkGlyph& sk_getMetrics_utf8_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF8_PrevUnichar(text));
}

static const SkGlyph& sk_getMetrics_utf16_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF16_NextUnichar((const uint16_t**)text));
}

static const SkGlyph& sk_getMetrics_utf16_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF16_PrevUnichar((const uint16_t**)text));
}

static const SkGlyph& sk_getMetrics_glyph_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    unsigned glyphID = *ptr;
    ptr += 1;
    *text = (const char*)ptr;
    return cache->getGlyphIDMetrics(glyphID);
}

static const SkGlyph& sk_getMetrics_glyph_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    ptr -= 1;
    unsigned glyphID = *ptr;
    *text = (const char*)ptr;
    return cache->getGlyphIDMetrics(glyphID);
}

///

static const SkGlyph& sk_getAdvance_utf8_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharAdvance(SkUTF8_NextUnichar(text));
}

static const SkGlyph& sk_getAdvance_utf8_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharAdvance(SkUTF8_PrevUnichar(text));
}

static const SkGlyph& sk_getAdvance_utf16_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharAdvance(SkUTF16_NextUnichar((const uint16_t**)text));
}

static const SkGlyph& sk_getAdvance_utf16_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharAdvance(SkUTF16_PrevUnichar((const uint16_t**)text));
}

static const SkGlyph& sk_getAdvance_glyph_next(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    unsigned glyphID = *ptr;
    ptr += 1;
    *text = (const char*)ptr;
    return cache->getGlyphIDAdvance(glyphID);
}

static const SkGlyph& sk_getAdvance_glyph_prev(SkGlyphCache* cache, const char** text)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    ptr -= 1;
    unsigned glyphID = *ptr;
    *text = (const char*)ptr;
    return cache->getGlyphIDAdvance(glyphID);
}

SkMeasureCacheProc SkPaint::getMeasureCacheProc(TextBufferDirection tbd,
                                                bool needFullMetrics) const
{
    static const SkMeasureCacheProc gMeasureCacheProcs[] = {
        sk_getMetrics_utf8_next,
        sk_getMetrics_utf16_next,
        sk_getMetrics_glyph_next,
        
        sk_getMetrics_utf8_prev,
        sk_getMetrics_utf16_prev,
        sk_getMetrics_glyph_prev,
        
        sk_getAdvance_utf8_next,
        sk_getAdvance_utf16_next,
        sk_getAdvance_glyph_next,
        
        sk_getAdvance_utf8_prev,
        sk_getAdvance_utf16_prev,
        sk_getAdvance_glyph_prev
    };
    
    unsigned index = this->getTextEncoding();

    if (kBackward_TextBufferDirection == tbd)
        index += 3;
    if (!needFullMetrics && !this->isDevKernText())
        index += 6;

    SkASSERT(index < SK_ARRAY_COUNT(gMeasureCacheProcs));
    return gMeasureCacheProcs[index];
}

///////////////////////////////////////////////////////////////////////////////

static const SkGlyph& sk_getMetrics_utf8_00(SkGlyphCache* cache,
                                            const char** text, SkFixed, SkFixed)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF8_NextUnichar(text));
}

static const SkGlyph& sk_getMetrics_utf8_xy(SkGlyphCache* cache,
                                            const char** text, SkFixed x, SkFixed y)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF8_NextUnichar(text), x, y);
}

static const SkGlyph& sk_getMetrics_utf16_00(SkGlyphCache* cache, const char** text,
                                             SkFixed, SkFixed)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF16_NextUnichar((const uint16_t**)text));
}

static const SkGlyph& sk_getMetrics_utf16_xy(SkGlyphCache* cache,
                                             const char** text, SkFixed x, SkFixed y)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    return cache->getUnicharMetrics(SkUTF16_NextUnichar((const uint16_t**)text),
                                    x, y);
}

static const SkGlyph& sk_getMetrics_glyph_00(SkGlyphCache* cache, const char** text,
                                             SkFixed, SkFixed)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    unsigned glyphID = *ptr;
    ptr += 1;
    *text = (const char*)ptr;
    return cache->getGlyphIDMetrics(glyphID);
}

static const SkGlyph& sk_getMetrics_glyph_xy(SkGlyphCache* cache,
                                             const char** text, SkFixed x, SkFixed y)
{
    SkASSERT(cache != NULL);
    SkASSERT(text != NULL);
    
    const uint16_t* ptr = *(const uint16_t**)text;
    unsigned glyphID = *ptr;
    ptr += 1;
    *text = (const char*)ptr;
    return cache->getGlyphIDMetrics(glyphID, x, y);
}

SkDrawCacheProc SkPaint::getDrawCacheProc() const
{
    static const SkDrawCacheProc gDrawCacheProcs[] = {
        sk_getMetrics_utf8_00,
        sk_getMetrics_utf16_00,
        sk_getMetrics_glyph_00,
        
        sk_getMetrics_utf8_xy,
        sk_getMetrics_utf16_xy,
        sk_getMetrics_glyph_xy
    };
    
    unsigned index = this->getTextEncoding();
    if (fFlags & kSubpixelText_Flag)
        index += 3;
    
    SkASSERT(index < SK_ARRAY_COUNT(gDrawCacheProcs));
    return gDrawCacheProcs[index];
}

///////////////////////////////////////////////////////////////////////////////

class SkAutoRestorePaintTextSizeAndFrame {
public:
    SkAutoRestorePaintTextSizeAndFrame(const SkPaint* paint) : fPaint((SkPaint*)paint)
    {
        fTextSize = paint->getTextSize();
        fStyle = paint->getStyle();
        fPaint->setStyle(SkPaint::kFill_Style);
    }
    ~SkAutoRestorePaintTextSizeAndFrame()
    {
        fPaint->setStyle(fStyle);
        fPaint->setTextSize(fTextSize);
    }
    
private:
    SkPaint*        fPaint;
    SkScalar        fTextSize;
    SkPaint::Style  fStyle;
};

static void set_bounds(const SkGlyph& g, SkRect* bounds)
{
    bounds->set(SkIntToScalar(g.fLeft),
                SkIntToScalar(g.fTop),
                SkIntToScalar(g.fLeft + g.fWidth),
                SkIntToScalar(g.fTop + g.fHeight));
}

// 64bits wide, with a 16bit bias. Useful when accumulating lots of 16.16 so
// we don't overflow along the way
typedef int64_t Sk48Dot16;

#ifdef SK_SCALAR_IS_FLOAT
    static inline float Sk48Dot16ToScalar(Sk48Dot16 x) {
        return (float) (x * 1.5258789e-5);   // x * (1 / 65536.0f)
    }
#else
    static inline SkFixed Sk48Dot16ToScalar(Sk48Dot16 x) {
        // just return the low 32bits
        return static_cast<SkFixed>(x);
    }
#endif

static void join_bounds(const SkGlyph& g, SkRect* bounds, Sk48Dot16 dx)
{
    SkScalar sx = Sk48Dot16ToScalar(dx);
    bounds->join(SkIntToScalar(g.fLeft) + sx,
                 SkIntToScalar(g.fTop),
                 SkIntToScalar(g.fLeft + g.fWidth) + sx,
                 SkIntToScalar(g.fTop + g.fHeight));
}

SkScalar SkPaint::measure_text(SkGlyphCache* cache,
                               const char* text, size_t byteLength,
                               int* count, SkRect* bounds) const
{
    SkASSERT(count);
    if (byteLength == 0)
    {
        *count = 0;
        if (bounds)
            bounds->setEmpty();
        return 0;
    }

    SkMeasureCacheProc glyphCacheProc;
    glyphCacheProc = this->getMeasureCacheProc(kForward_TextBufferDirection,
                                               NULL != bounds);

    int         n = 1;
    const char* stop = (const char*)text + byteLength;
    const SkGlyph* g = &glyphCacheProc(cache, &text);
    // our accumulated fixed-point advances might overflow 16.16, so we use
    // a 48.16 (64bit) accumulator, and then convert that to scalar at the
    // very end.
    Sk48Dot16 x = g->fAdvanceX;

    SkAutoKern  autokern;

    if (NULL == bounds)
    {
        if (this->isDevKernText())
        {
            int rsb;
            for (; text < stop; n++) {
                rsb = g->fRsbDelta;
                g = &glyphCacheProc(cache, &text);
                x += SkAutoKern_AdjustF(rsb, g->fLsbDelta) + g->fAdvanceX;
            }
        }
        else
        {
            for (; text < stop; n++) {
                x += glyphCacheProc(cache, &text).fAdvanceX;
            }
        }
    }
    else
    {
        set_bounds(*g, bounds);
        if (this->isDevKernText())
        {
            int rsb;
            for (; text < stop; n++) {
                rsb = g->fRsbDelta;
                g = &glyphCacheProc(cache, &text);
                x += SkAutoKern_AdjustF(rsb, g->fLsbDelta);
                join_bounds(*g, bounds, x);
                x += g->fAdvanceX;
            }
        }
        else
        {
            for (; text < stop; n++) {
                g = &glyphCacheProc(cache, &text);
                join_bounds(*g, bounds, x);
                x += g->fAdvanceX;
            }
        }
    }
    SkASSERT(text == stop);

    *count = n;
    return Sk48Dot16ToScalar(x);
}

SkScalar SkPaint::measureText(const void* textData, size_t length,
                              SkRect* bounds, SkScalar zoom) const
{
    const char* text = (const char*)textData;
    SkASSERT(text != NULL || length == 0);

    SkScalar                            scale = 0;
    SkAutoRestorePaintTextSizeAndFrame  restore(this);

    if (this->isLinearText())
    {
        scale = fTextSize / kCanonicalTextSizeForPaths;
        // this gets restored by restore
        ((SkPaint*)this)->setTextSize(SkIntToScalar(kCanonicalTextSizeForPaths));
    }
    
    SkMatrix    zoomMatrix, *zoomPtr = NULL;
    if (zoom)
    {
        zoomMatrix.setScale(zoom, zoom);
        zoomPtr = &zoomMatrix;
    }

    SkAutoGlyphCache    autoCache(*this, zoomPtr);
    SkGlyphCache*       cache = autoCache.getCache();

    SkScalar width = 0;
    
    if (length > 0)
    {
        int tempCount;

        width = this->measure_text(cache, text, length, &tempCount, bounds);
        if (scale)
        {
            width = SkScalarMul(width, scale);
            if (bounds)
            {
                bounds->fLeft = SkScalarMul(bounds->fLeft, scale);
                bounds->fTop = SkScalarMul(bounds->fTop, scale);
                bounds->fRight = SkScalarMul(bounds->fRight, scale);
                bounds->fBottom = SkScalarMul(bounds->fBottom, scale);
            }
        }
    }
    return width;
}

typedef bool (*SkTextBufferPred)(const char* text, const char* stop);

static bool forward_textBufferPred(const char* text, const char* stop)
{
    return text < stop;
}

static bool backward_textBufferPred(const char* text, const char* stop)
{
    return text > stop;
}

static SkTextBufferPred chooseTextBufferPred(SkPaint::TextBufferDirection tbd,
                            const char** text, size_t length, const char** stop)
{
    if (SkPaint::kForward_TextBufferDirection == tbd)
    {
        *stop = *text + length;
        return forward_textBufferPred;
    }
    else
    {
        // text should point to the end of the buffer, and stop to the beginning
        *stop = *text;
        *text += length;
        return backward_textBufferPred;
    }
}

size_t SkPaint::breakText(const void* textD, size_t length, SkScalar maxWidth,
                          SkScalar* measuredWidth,
                          TextBufferDirection tbd) const
{
    if (0 == length || 0 >= maxWidth)
    {
        if (measuredWidth)
            *measuredWidth = 0;
        return 0;
    }

    SkASSERT(textD != NULL);
    const char* text = (const char*)textD;

    SkScalar                            scale = 0;
    SkAutoRestorePaintTextSizeAndFrame  restore(this);

    if (this->isLinearText())
    {
        scale = fTextSize / kCanonicalTextSizeForPaths;
        maxWidth = SkScalarMulDiv(maxWidth, kCanonicalTextSizeForPaths, fTextSize);
        // this gets restored by restore
        ((SkPaint*)this)->setTextSize(SkIntToScalar(kCanonicalTextSizeForPaths));
    }
    
    SkAutoGlyphCache    autoCache(*this, NULL);
    SkGlyphCache*       cache = autoCache.getCache();

    SkMeasureCacheProc glyphCacheProc = this->getMeasureCacheProc(tbd, false);
    const char*      stop;
    SkTextBufferPred pred = chooseTextBufferPred(tbd, &text, length, &stop);
    // use 64bits for our accumulator, to avoid overflowing 16.16
    Sk48Dot16        max = SkScalarToFixed(maxWidth);
    Sk48Dot16        width = 0;

    SkAutoKern  autokern;

    if (this->isDevKernText())
    {
        int rsb = 0;
        while (pred(text, stop))
        {
            const char* curr = text;
            const SkGlyph& g = glyphCacheProc(cache, &text);
            SkFixed x = SkAutoKern_AdjustF(rsb, g.fLsbDelta) + g.fAdvanceX;
            if ((width += x) > max)
            {
                width -= x;
                text = curr;
                break;
            }
            rsb = g.fRsbDelta;
        }
    }
    else
    {
        while (pred(text, stop))
        {
            const char* curr = text;
            SkFixed x = glyphCacheProc(cache, &text).fAdvanceX;
            if ((width += x) > max)
            {
                width -= x;
                text = curr;
                break;
            }
        }
    }
    
    if (measuredWidth)
    {
        
        SkScalar scalarWidth = Sk48Dot16ToScalar(width);
        if (scale)
            scalarWidth = SkScalarMul(scalarWidth, scale);
        *measuredWidth = scalarWidth;
    }
    
    // return the number of bytes measured
    return (kForward_TextBufferDirection == tbd) ?
                text - stop + length : stop - text + length;
}

///////////////////////////////////////////////////////////////////////////////

static bool FontMetricsCacheProc(const SkGlyphCache* cache, void* context)
{
    *(SkPaint::FontMetrics*)context = cache->getFontMetricsY();
    return false;   // don't detach the cache
}

static void FontMetricsDescProc(const SkDescriptor* desc, void* context)
{
    SkGlyphCache::VisitCache(desc, FontMetricsCacheProc, context);
}

SkScalar SkPaint::getFontMetrics(FontMetrics* metrics, SkScalar zoom) const
{
    SkScalar                            scale = 0;
    SkAutoRestorePaintTextSizeAndFrame  restore(this);

    if (this->isLinearText())
    {
        scale = fTextSize / kCanonicalTextSizeForPaths;
        // this gets restored by restore
        ((SkPaint*)this)->setTextSize(SkIntToScalar(kCanonicalTextSizeForPaths));
    }
    
    SkMatrix    zoomMatrix, *zoomPtr = NULL;
    if (zoom)
    {
        zoomMatrix.setScale(zoom, zoom);
        zoomPtr = &zoomMatrix;
    }

#if 0
    SkAutoGlyphCache    autoCache(*this, zoomPtr);
    SkGlyphCache*       cache = autoCache.getCache();
    const FontMetrics&  my = cache->getFontMetricsY();
#endif
    FontMetrics storage;
    if (NULL == metrics)
        metrics = &storage;
    
    this->descriptorProc(zoomPtr, FontMetricsDescProc, metrics);

    if (scale)
    {
        metrics->fTop = SkScalarMul(metrics->fTop, scale);
        metrics->fAscent = SkScalarMul(metrics->fAscent, scale);
        metrics->fDescent = SkScalarMul(metrics->fDescent, scale);
        metrics->fBottom = SkScalarMul(metrics->fBottom, scale);
        metrics->fLeading = SkScalarMul(metrics->fLeading, scale);
    }
    return metrics->fDescent - metrics->fAscent + metrics->fLeading;
}

////////////////////////////////////////////////////////////////////////////////////////////

static void set_bounds(const SkGlyph& g, SkRect* bounds, SkScalar scale)
{
    bounds->set(g.fLeft * scale,
                g.fTop * scale,
                (g.fLeft + g.fWidth) * scale,
                (g.fTop + g.fHeight) * scale);
}

int SkPaint::getTextWidths(const void* textData, size_t byteLength, SkScalar widths[],
                           SkRect bounds[]) const
{
    if (0 == byteLength)
        return 0;

    SkASSERT(NULL != textData);

    if (NULL == widths && NULL == bounds)
        return this->countText(textData, byteLength);

    SkAutoRestorePaintTextSizeAndFrame  restore(this);
    SkScalar                            scale = 0;

    if (this->isLinearText())
    {
        scale = fTextSize / kCanonicalTextSizeForPaths;
        // this gets restored by restore
        ((SkPaint*)this)->setTextSize(SkIntToScalar(kCanonicalTextSizeForPaths));
    }

    SkAutoGlyphCache    autoCache(*this, NULL);
    SkGlyphCache*       cache = autoCache.getCache();
    SkMeasureCacheProc  glyphCacheProc;
    glyphCacheProc = this->getMeasureCacheProc(kForward_TextBufferDirection,
                                               NULL != bounds);

    const char* text = (const char*)textData;
    const char* stop = text + byteLength;
    int         count = 0;

    if (this->isDevKernText())
    {
        // we adjust the widths returned here through auto-kerning
        SkAutoKern  autokern;
        SkFixed     prevWidth = 0;

        if (scale) {
            while (text < stop) {
                const SkGlyph& g = glyphCacheProc(cache, &text);
                if (widths) {
                    SkFixed  adjust = autokern.adjust(g);

                    if (count > 0) {
                        SkScalar w = SkFixedToScalar(prevWidth + adjust);
                        *widths++ = SkScalarMul(w, scale);
                    }
                    prevWidth = g.fAdvanceX;
                }
                if (bounds) {
                    set_bounds(g, bounds++, scale);
                }
                ++count;
            }
            if (count > 0 && widths) {
                *widths = SkScalarMul(SkFixedToScalar(prevWidth), scale);
            }
        } else {
            while (text < stop) {
                const SkGlyph& g = glyphCacheProc(cache, &text);
                if (widths) {
                    SkFixed  adjust = autokern.adjust(g);

                    if (count > 0) {
                        *widths++ = SkFixedToScalar(prevWidth + adjust);
                    }
                    prevWidth = g.fAdvanceX;
                }
                if (bounds) {
                    set_bounds(g, bounds++);
                }
                ++count;
            }
            if (count > 0 && widths) {
                *widths = SkFixedToScalar(prevWidth);
            }
        }
    } else {    // no devkern
        if (scale) {
            while (text < stop) {
                const SkGlyph& g = glyphCacheProc(cache, &text);
                if (widths) {
                    *widths++ = SkScalarMul(SkFixedToScalar(g.fAdvanceX),
                                            scale);
                }
                if (bounds) {
                    set_bounds(g, bounds++, scale);
                }
                ++count;
            }
        } else {
            while (text < stop) {
                const SkGlyph& g = glyphCacheProc(cache, &text);
                if (widths) {
                    *widths++ = SkFixedToScalar(g.fAdvanceX);
                }
                if (bounds) {
                    set_bounds(g, bounds++);
                }
                ++count;
            }
        }
    }

    SkASSERT(text == stop);
    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////

#include "SkDraw.h"

void SkPaint::getTextPath(const void* textData, size_t length, SkScalar x, SkScalar y, SkPath* path) const
{
    const char* text = (const char*)textData;
    SkASSERT(length == 0 || text != NULL);
    if (text == NULL || length == 0 || path == NULL)
        return;

    SkTextToPathIter    iter(text, length, *this, false, true);
    SkMatrix            matrix;
    SkScalar            prevXPos = 0;

    matrix.setScale(iter.getPathScale(), iter.getPathScale());
    matrix.postTranslate(x, y);
    path->reset();

    SkScalar        xpos;
    const SkPath*   iterPath;
    while ((iterPath = iter.next(&xpos)) != NULL)
    {
        matrix.postTranslate(xpos - prevXPos, 0);
        path->addPath(*iterPath, matrix);
        prevXPos = xpos;
    }
}

static void add_flattenable(SkDescriptor* desc, uint32_t tag,
                            SkFlattenableWriteBuffer* buffer) {
    buffer->flatten(desc->addEntry(tag, buffer->size(), NULL));
}

/*
 *  interpolates to find the right value for key, in the function represented by the 'length' number of pairs: (keys[i], values[i])
    inspired by a desire to change the multiplier for thickness in fakebold
    therefore, i assumed number of pairs (length) will be small, so a linear search is sufficient
    repeated keys are allowed for discontinuous functions (so long as keys is monotonically increasing), and if 
        key is the value of a repeated scalar in keys, the first one will be used 
    - this may change if a binary search is used
    - also, this ensures that there is no divide by zero (an assert also checks for that)
*/
static SkScalar interpolate(SkScalar key, const SkScalar keys[], const SkScalar values[], int length)
{

    SkASSERT(length > 0);
    SkASSERT(keys != NULL);    
    SkASSERT(values != NULL);
#ifdef SK_DEBUG
    for (int i = 1; i < length; i++)
        SkASSERT(keys[i] >= keys[i-1]);
#endif
    int right = 0;
    while (right < length && key > keys[right])
        right++;
    //could use sentinal values to eliminate conditionals
    //i assume i am not in control of input values, so i want to make it simple
    if (length == right)
        return values[length-1];
    if (0 == right)
        return values[0];
    //otherwise, we interpolate between right-1 and right
    SkScalar rVal = values[right];
    SkScalar lVal = values[right-1];
    SkScalar rightKey = keys[right];
    SkScalar leftKey = keys[right-1];
    SkASSERT(rightKey != leftKey);
    //fractional amount which we will multiply by the difference in the left value and right value
    SkScalar fract = SkScalarDiv(key-leftKey,rightKey-leftKey);
    return lVal + SkScalarMul(fract, rVal-lVal);
}

//used for interpolating in fakeBold
static const SkScalar pointSizes[] = { SkIntToScalar(9), SkIntToScalar(36) };
static const SkScalar multipliers[] = { SK_Scalar1/24, SK_Scalar1/32 };

static SkMask::Format computeMaskFormat(const SkPaint& paint)
{
    uint32_t flags = paint.getFlags();

    // Antialiasing being disabled trumps all other settings.
    if (!(flags & SkPaint::kAntiAlias_Flag))
        return SkMask::kBW_Format;

#if defined(SK_SUPPORT_LCDTEXT)
    if (flags & SkPaint::kLCDRenderText_Flag)
        return SkFontHost::GetSubpixelOrientation() == SkFontHost::kHorizontal_LCDOrientation ?
                   SkMask::kHorizontalLCD_Format : SkMask::kVerticalLCD_Format;
#endif

    return SkMask::kA8_Format;
}

// if linear-text is on, then we force hinting to be off (since that's sort of
// the point of linear-text.
static SkPaint::Hinting computeHinting(const SkPaint& paint) {
    SkPaint::Hinting h = paint.getHinting();
    if (paint.isLinearText()) {
        h = SkPaint::kNo_Hinting;
    }
    return h;
}

void SkScalerContext::MakeRec(const SkPaint& paint,
                              const SkMatrix* deviceMatrix, Rec* rec)
{
    SkASSERT(deviceMatrix == NULL ||
             (deviceMatrix->getType() & SkMatrix::kPerspective_Mask) == 0);

    rec->fFontID = SkTypeface::UniqueID(paint.getTypeface());
    rec->fTextSize = paint.getTextSize();
    rec->fPreScaleX = paint.getTextScaleX();
    rec->fPreSkewX  = paint.getTextSkewX();

    if (deviceMatrix)
    {
        rec->fPost2x2[0][0] = deviceMatrix->getScaleX();
        rec->fPost2x2[0][1] = deviceMatrix->getSkewX();
        rec->fPost2x2[1][0] = deviceMatrix->getSkewY();
        rec->fPost2x2[1][1] = deviceMatrix->getScaleY();
    }
    else
    {
        rec->fPost2x2[0][0] = rec->fPost2x2[1][1] = SK_Scalar1;
        rec->fPost2x2[0][1] = rec->fPost2x2[1][0] = 0;
    }
    
    SkPaint::Style  style = paint.getStyle();
    SkScalar        strokeWidth = paint.getStrokeWidth();
    
    unsigned flags = SkFontHost::ComputeGammaFlag(paint);

    if (paint.isFakeBoldText())
    {
#ifdef SK_USE_FREETYPE_EMBOLDEN
        flags |= SkScalerContext::kEmbolden_Flag;
#else
        SkScalar fakeBoldScale = interpolate(paint.getTextSize(), pointSizes, multipliers, 2);
        SkScalar extra = SkScalarMul(paint.getTextSize(), fakeBoldScale);
        
        if (style == SkPaint::kFill_Style)
        {
            style = SkPaint::kStrokeAndFill_Style;
            strokeWidth = extra;    // ignore paint's strokeWidth if it was "fill"
        }
        else
            strokeWidth += extra;
#endif
    }

    if (paint.isDevKernText())
        flags |= SkScalerContext::kDevKernText_Flag;
    
    if (style != SkPaint::kFill_Style && strokeWidth > 0)
    {
        rec->fFrameWidth = strokeWidth;
        rec->fMiterLimit = paint.getStrokeMiter();
        rec->fStrokeJoin = SkToU8(paint.getStrokeJoin());

        if (style == SkPaint::kStrokeAndFill_Style)
            flags |= SkScalerContext::kFrameAndFill_Flag;
    }
    else
    {
        rec->fFrameWidth = 0;
        rec->fMiterLimit = 0;
        rec->fStrokeJoin = 0;
    }

    rec->fMaskFormat = SkToU8(computeMaskFormat(paint));
    rec->fFlags = SkToU8(flags);
    rec->setHinting(computeHinting(paint));
    if (paint.isEmbeddedBitmapText())
        rec->fFlags |= SkScalerContext::kEmbeddedBitmapText_Flag;
    if (paint.isSubpixelText())
        rec->fFlags |= SkScalerContext::kSubpixelPositioning_Flag;
    if (paint.isAutohinted())
        rec->fFlags |= SkScalerContext::kAutohinting_Flag;

    /*  Allow the fonthost to modify our rec before we use it as a key into the
        cache. This way if we're asking for something that they will ignore,
        they can modify our rec up front, so we don't create duplicate cache
        entries.
     */
    SkFontHost::FilterRec(rec);
}

#define MIN_SIZE_FOR_EFFECT_BUFFER  1024

void SkPaint::descriptorProc(const SkMatrix* deviceMatrix,
                             void (*proc)(const SkDescriptor*, void*),
                             void* context) const
{
    SkScalerContext::Rec    rec;

    SkScalerContext::MakeRec(*this, deviceMatrix, &rec);

    size_t          descSize = sizeof(rec);
    int             entryCount = 1;
    SkPathEffect*   pe = this->getPathEffect();
    SkMaskFilter*   mf = this->getMaskFilter();
    SkRasterizer*   ra = this->getRasterizer();

    SkFlattenableWriteBuffer    peBuffer(MIN_SIZE_FOR_EFFECT_BUFFER);
    SkFlattenableWriteBuffer    mfBuffer(MIN_SIZE_FOR_EFFECT_BUFFER);
    SkFlattenableWriteBuffer    raBuffer(MIN_SIZE_FOR_EFFECT_BUFFER);

    if (pe) {
        peBuffer.writeFlattenable(pe);
        descSize += peBuffer.size();
        entryCount += 1;
        rec.fMaskFormat = SkMask::kA8_Format;   // force antialiasing when we do the scan conversion
        // seems like we could support kLCD as well at this point...
    }
    if (mf) {
        mfBuffer.writeFlattenable(mf);
        descSize += mfBuffer.size();
        entryCount += 1;
        rec.fMaskFormat = SkMask::kA8_Format;   // force antialiasing with maskfilters
    }
    if (ra) {
        raBuffer.writeFlattenable(ra);
        descSize += raBuffer.size();
        entryCount += 1;
        rec.fMaskFormat = SkMask::kA8_Format;   // force antialiasing when we do the scan conversion
    }
    descSize += SkDescriptor::ComputeOverhead(entryCount);

    SkAutoDescriptor    ad(descSize);
    SkDescriptor*       desc = ad.getDesc();

    desc->init();
    desc->addEntry(kRec_SkDescriptorTag, sizeof(rec), &rec);

    if (pe) {
        add_flattenable(desc, kPathEffect_SkDescriptorTag, &peBuffer);
    }
    if (mf) {
        add_flattenable(desc, kMaskFilter_SkDescriptorTag, &mfBuffer);
    }
    if (ra) {
        add_flattenable(desc, kRasterizer_SkDescriptorTag, &raBuffer);
    }

    SkASSERT(descSize == desc->getLength());
    desc->computeChecksum();

    proc(desc, context);
}

static void DetachDescProc(const SkDescriptor* desc, void* context)
{
    *((SkGlyphCache**)context) = SkGlyphCache::DetachCache(desc);
}

SkGlyphCache* SkPaint::detachCache(const SkMatrix* deviceMatrix) const
{
    SkGlyphCache* cache;
    this->descriptorProc(deviceMatrix, DetachDescProc, &cache);
    return cache;
}

///////////////////////////////////////////////////////////////////////////////

#include "SkStream.h"

static uintptr_t asint(const void* p) {
    return reinterpret_cast<uintptr_t>(p);
}

union Scalar32 {
    SkScalar    fScalar;
    uint32_t    f32;
};

static uint32_t* write_scalar(uint32_t* ptr, SkScalar value) {
    SkASSERT(sizeof(SkScalar) == sizeof(uint32_t));
    Scalar32 tmp;
    tmp.fScalar = value;
    *ptr = tmp.f32;
    return ptr + 1;
}

static SkScalar read_scalar(const uint32_t*& ptr) {
    SkASSERT(sizeof(SkScalar) == sizeof(uint32_t));
    Scalar32 tmp;
    tmp.f32 = *ptr++;
    return tmp.fScalar;
}

static uint32_t pack_4(unsigned a, unsigned b, unsigned c, unsigned d) {
    SkASSERT(a == (uint8_t)a);
    SkASSERT(b == (uint8_t)b);
    SkASSERT(c == (uint8_t)c);
    SkASSERT(d == (uint8_t)d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

enum FlatFlags {
    kHasTypeface_FlatFlag   = 0x01,
    kHasEffects_FlatFlag    = 0x02
};

// The size of a flat paint's POD fields
static const uint32_t kPODPaintSize =   5 * sizeof(SkScalar) +
                                        1 * sizeof(SkColor) +
                                        1 * sizeof(uint16_t) +
                                        6 * sizeof(uint8_t);

/*  To save space/time, we analyze the paint, and write a truncated version of
    it if there are not tricky elements like shaders, etc.
 */
void SkPaint::flatten(SkFlattenableWriteBuffer& buffer) const {
    uint8_t flatFlags = 0;
    if (this->getTypeface()) {
        flatFlags |= kHasTypeface_FlatFlag;
    }
    if (asint(this->getPathEffect()) |
        asint(this->getShader()) |
        asint(this->getXfermode()) |
        asint(this->getMaskFilter()) |
        asint(this->getColorFilter()) |
        asint(this->getRasterizer()) |
        asint(this->getLooper())) {
        flatFlags |= kHasEffects_FlatFlag;
    }
    
    SkASSERT(SkAlign4(kPODPaintSize) == kPODPaintSize);
    uint32_t* ptr = buffer.reserve(kPODPaintSize);

    ptr = write_scalar(ptr, this->getTextSize());
    ptr = write_scalar(ptr, this->getTextScaleX());
    ptr = write_scalar(ptr, this->getTextSkewX());
    ptr = write_scalar(ptr, this->getStrokeWidth());
    ptr = write_scalar(ptr, this->getStrokeMiter());
    *ptr++ = this->getColor();
    *ptr++ = (this->getFlags() << 16) | (this->getTextAlign() << 8) | flatFlags;
    *ptr++ = pack_4(this->getStrokeCap(), this->getStrokeJoin(),
                    this->getStyle(), this->getTextEncoding());

    // now we're done with ptr and the (pre)reserved space. If we need to write
    // additional fields, use the buffer directly
    if (flatFlags & kHasTypeface_FlatFlag) {
        buffer.writeTypeface(this->getTypeface());
    }
    if (flatFlags & kHasEffects_FlatFlag) {
        buffer.writeFlattenable(this->getPathEffect());
        buffer.writeFlattenable(this->getShader());
        buffer.writeFlattenable(this->getXfermode());
        buffer.writeFlattenable(this->getMaskFilter());
        buffer.writeFlattenable(this->getColorFilter());
        buffer.writeFlattenable(this->getRasterizer());
        buffer.writeFlattenable(this->getLooper());
    }
}

void SkPaint::unflatten(SkFlattenableReadBuffer& buffer) {
    SkASSERT(SkAlign4(kPODPaintSize) == kPODPaintSize);
    const void* podData = buffer.skip(kPODPaintSize);
    const uint32_t* pod = reinterpret_cast<const uint32_t*>(podData);
    
    // the order we read must match the order we wrote in flatten()
    this->setTextSize(read_scalar(pod));
    this->setTextScaleX(read_scalar(pod));
    this->setTextSkewX(read_scalar(pod));
    this->setStrokeWidth(read_scalar(pod));
    this->setStrokeMiter(read_scalar(pod));    
    this->setColor(*pod++);

    uint32_t tmp = *pod++;
    this->setFlags(tmp >> 16);
    this->setTextAlign(static_cast<Align>((tmp >> 8) & 0xFF));
    uint8_t flatFlags = tmp & 0xFF;

    tmp = *pod++;
    this->setStrokeCap(static_cast<Cap>((tmp >> 24) & 0xFF));
    this->setStrokeJoin(static_cast<Join>((tmp >> 16) & 0xFF));
    this->setStyle(static_cast<Style>((tmp >> 8) & 0xFF));
    this->setTextEncoding(static_cast<TextEncoding>((tmp >> 0) & 0xFF));

    if (flatFlags & kHasTypeface_FlatFlag) {
        this->setTypeface(buffer.readTypeface());
    } else {
        this->setTypeface(NULL);
    }

    if (flatFlags & kHasEffects_FlatFlag) {
        this->setPathEffect((SkPathEffect*) buffer.readFlattenable())->safeUnref();
        this->setShader((SkShader*) buffer.readFlattenable())->safeUnref();
        this->setXfermode((SkXfermode*) buffer.readFlattenable())->safeUnref();
        this->setMaskFilter((SkMaskFilter*) buffer.readFlattenable())->safeUnref();
        this->setColorFilter((SkColorFilter*) buffer.readFlattenable())->safeUnref();
        this->setRasterizer((SkRasterizer*) buffer.readFlattenable())->safeUnref();
        this->setLooper((SkDrawLooper*) buffer.readFlattenable())->safeUnref();
    } else {
        this->setPathEffect(NULL);
        this->setShader(NULL);
        this->setXfermode(NULL);
        this->setMaskFilter(NULL);
        this->setColorFilter(NULL);
        this->setRasterizer(NULL);
        this->setLooper(NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////

SkShader* SkPaint::setShader(SkShader* shader)
{
    SkRefCnt_SafeAssign(fShader, shader);
    return shader;
}

SkColorFilter* SkPaint::setColorFilter(SkColorFilter* filter)
{
    SkRefCnt_SafeAssign(fColorFilter, filter);
    return filter;
}

SkXfermode* SkPaint::setXfermode(SkXfermode* mode)
{
    SkRefCnt_SafeAssign(fXfermode, mode);
    return mode;
}

SkXfermode* SkPaint::setXfermodeMode(SkXfermode::Mode mode) {
    SkSafeUnref(fXfermode);
    fXfermode = SkXfermode::Create(mode);
    return fXfermode;
}

SkPathEffect* SkPaint::setPathEffect(SkPathEffect* effect)
{
    SkRefCnt_SafeAssign(fPathEffect, effect);
    return effect;
}

SkMaskFilter* SkPaint::setMaskFilter(SkMaskFilter* filter)
{
    SkRefCnt_SafeAssign(fMaskFilter, filter);
    return filter;
}

////////////////////////////////////////////////////////////////////////////////////////

bool SkPaint::getFillPath(const SkPath& src, SkPath* dst) const
{
    SkPath          effectPath, strokePath;
    const SkPath*   path = &src;

    SkScalar width = this->getStrokeWidth();
    
    switch (this->getStyle()) {
    case SkPaint::kFill_Style:
        width = -1; // mark it as no-stroke
        break;
    case SkPaint::kStrokeAndFill_Style:
        if (width == 0)
            width = -1; // mark it as no-stroke
        break;
    case SkPaint::kStroke_Style:
        break;
    default:
        SkASSERT(!"unknown paint style");
    }

    if (this->getPathEffect())
    {
        // lie to the pathEffect if our style is strokeandfill, so that it treats us as just fill
        if (this->getStyle() == SkPaint::kStrokeAndFill_Style)
            width = -1; // mark it as no-stroke

        if (this->getPathEffect()->filterPath(&effectPath, src, &width))
            path = &effectPath;
        
        // restore the width if we earlier had to lie, and if we're still set to no-stroke
        // note: if we're now stroke (width >= 0), then the pathEffect asked for that change
        // and we want to respect that (i.e. don't overwrite their setting for width)
        if (this->getStyle() == SkPaint::kStrokeAndFill_Style && width < 0)
        {
            width = this->getStrokeWidth();
            if (width == 0)
                width = -1;
        }
    }
    
    if (width > 0 && !path->isEmpty())
    {
        SkStroke stroker(*this, width);
        stroker.strokePath(*path, &strokePath);
        path = &strokePath;
    }

    if (path == &src)
        *dst = src;
    else
    {
        SkASSERT(path == &effectPath || path == &strokePath);
        dst->swap(*(SkPath*)path);
    }

    return width != 0;  // return true if we're filled, or false if we're hairline (width == 0)
}

const SkRect& SkPaint::computeStrokeFastBounds(const SkRect& src,
                                               SkRect* storage) const {
    SkASSERT(storage);
    SkASSERT(this->getStyle() != SkPaint::kFill_Style);

    // since we're stroked, outset the rect by the radius (and join type)
    SkScalar radius = SkScalarHalf(this->getStrokeWidth());
    if (0 == radius) {  // hairline
        radius = SK_Scalar1;
    } else if (this->getStrokeJoin() == SkPaint::kMiter_Join) {
        SkScalar scale = this->getStrokeMiter();
        if (scale > SK_Scalar1) {
            radius = SkScalarMul(radius, scale);
        }
    }
    storage->set(src.fLeft - radius, src.fTop - radius,
                 src.fRight + radius, src.fBottom + radius);
    return *storage;
}

////////////////////////////////////////////////////////////////////////////////////////

static bool has_thick_frame(const SkPaint& paint)
{
    return paint.getStrokeWidth() > 0 && paint.getStyle() != SkPaint::kFill_Style;
}

SkTextToPathIter::SkTextToPathIter( const char text[], size_t length,
                                    const SkPaint& paint,
                                    bool applyStrokeAndPathEffects,
                                    bool forceLinearTextOn)
                                    : fPaint(paint) /* make a copy of the paint */
{
    fGlyphCacheProc = paint.getMeasureCacheProc(SkPaint::kForward_TextBufferDirection,
                                                true);

    if (forceLinearTextOn)
        fPaint.setLinearText(true);
    fPaint.setMaskFilter(NULL);   // don't want this affecting our path-cache lookup

    if (fPaint.getPathEffect() == NULL && !has_thick_frame(fPaint))
        applyStrokeAndPathEffects = false;

    // can't use our canonical size if we need to apply patheffects/strokes
    if (fPaint.isLinearText() && !applyStrokeAndPathEffects)
    {
        fPaint.setTextSize(SkIntToScalar(SkPaint::kCanonicalTextSizeForPaths));
        fScale = paint.getTextSize() / SkPaint::kCanonicalTextSizeForPaths;
    }
    else
        fScale = SK_Scalar1;
    
    if (!applyStrokeAndPathEffects)
    {
        fPaint.setStyle(SkPaint::kFill_Style);
        fPaint.setPathEffect(NULL);
    }

    fCache = fPaint.detachCache(NULL);

    SkPaint::Style  style = SkPaint::kFill_Style;
    SkPathEffect*   pe = NULL;

    if (!applyStrokeAndPathEffects)
    {
        style = paint.getStyle();   // restore
        pe = paint.getPathEffect();     // restore
    }
    fPaint.setStyle(style);
    fPaint.setPathEffect(pe);
    fPaint.setMaskFilter(paint.getMaskFilter());    // restore

    // now compute fXOffset if needed

    SkScalar xOffset = 0;
    if (paint.getTextAlign() != SkPaint::kLeft_Align)   // need to measure first
    {
        int      count;
        SkScalar width = SkScalarMul(fPaint.measure_text(fCache, text, length, &count, NULL), fScale);
        if (paint.getTextAlign() == SkPaint::kCenter_Align)
            width = SkScalarHalf(width);
        xOffset = -width;
    }
    fXPos = xOffset;
    fPrevAdvance = 0;

    fText = text;
    fStop = text + length;
}

SkTextToPathIter::~SkTextToPathIter()
{
    SkGlyphCache::AttachCache(fCache);
}

const SkPath* SkTextToPathIter::next(SkScalar* xpos)
{
    while (fText < fStop)
    {
        const SkGlyph& glyph = fGlyphCacheProc(fCache, &fText);

        fXPos += SkScalarMul(SkFixedToScalar(fPrevAdvance + fAutoKern.adjust(glyph)), fScale);
        fPrevAdvance = glyph.fAdvanceX;   // + fPaint.getTextTracking();

        if (glyph.fWidth)
        {
            if (xpos)
                *xpos = fXPos;
            return fCache->findPath(glyph);
        }
    }
    return NULL;
}
