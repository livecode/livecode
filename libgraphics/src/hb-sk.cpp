#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkFontHost.h>
#include <SkTypeface.h>
#include <SkUtils.h>

#include "hb-private.hh"

#include "hb-sk.h"

#include "hb-font-private.hh"

#ifndef HB_DEBUG_FT
#define HB_DEBUG_FT (HB_DEBUG+0)
#endif


static hb_position_t SkiaScalarToHarfbuzzPosition(SkScalar value)
{
    return value * 64;
}

static void SkiaSetPaint(SkPaint *paint, hb_skia_face *p_face, SkPaint::TextEncoding p_encoding)
{
    paint -> setTextEncoding(p_encoding);
    paint -> setTextSize(p_face -> size);
    paint -> setTypeface(p_face -> typeface);
}

static void SkiaGetGlyphWidthAndExtents(SkPaint* paint, hb_skia_face *p_face, hb_codepoint_t codepoint, hb_position_t* width, hb_glyph_extents_t* extents)
{
    SkiaSetPaint(paint, p_face, SkPaint::kGlyphID_TextEncoding);

    SkScalar skWidth;
    SkRect skBounds;
    uint16_t glyph = codepoint;
    
    paint->getTextWidths(&glyph, sizeof(glyph), &skWidth, &skBounds);
    if (width)
        *width = SkiaScalarToHarfbuzzPosition(skWidth);
    if (extents) {
        extents->x_bearing = SkiaScalarToHarfbuzzPosition(skBounds.fLeft);
        extents->y_bearing = SkiaScalarToHarfbuzzPosition(skBounds.fTop);
        extents->width = SkiaScalarToHarfbuzzPosition(skBounds.width());
        extents->height = SkiaScalarToHarfbuzzPosition(skBounds.height());
    }
}

static hb_bool_t
hb_sk_get_glyph (hb_font_t *font HB_UNUSED,
		 void *font_data,
		 hb_codepoint_t unicode,
		 hb_codepoint_t variation_selector,
		 hb_codepoint_t *glyph,
		 void *user_data HB_UNUSED)

{
    hb_skia_face *typeface = (hb_skia_face *)font_data;
    SkPaint paint;
    SkiaSetPaint(&paint, typeface, SkPaint::kUTF16_TextEncoding);
    
    uint16_t text[4];
    size_t length = SkUTF16_FromUnichar(unicode, text);
    
    uint16_t glyph16;
    paint.textToGlyphs(text, length, &glyph16);
    
    *glyph = glyph16;
    return true;
}

static hb_position_t
hb_sk_get_glyph_h_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
    hb_skia_face *typeface = (hb_skia_face *)font_data;
    SkPaint paint;
    
    hb_position_t advance = 0;
    SkiaGetGlyphWidthAndExtents(&paint, typeface, glyph, &advance, 0);
    return advance;
}

static hb_position_t
hb_sk_get_glyph_v_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
    return 0;
}

static hb_bool_t
hb_sk_get_glyph_h_origin (hb_font_t *font HB_UNUSED,
			  void *font_data HB_UNUSED,
			  hb_codepoint_t glyph HB_UNUSED,
			  hb_position_t *x HB_UNUSED,
			  hb_position_t *y HB_UNUSED,
			  void *user_data HB_UNUSED)
{
  /* We always work in the horizontal coordinates. */
  return true;
}

static hb_bool_t
hb_sk_get_glyph_v_origin (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_codepoint_t glyph,
			  hb_position_t *x,
			  hb_position_t *y,
			  void *user_data HB_UNUSED)
{
    return false;
}

static hb_position_t
hb_sk_get_glyph_h_kerning (hb_font_t *font,
			   void *font_data,
			   hb_codepoint_t left_glyph,
			   hb_codepoint_t right_glyph,
			   void *user_data HB_UNUSED)
{
    return 0;
}

static hb_position_t
hb_sk_get_glyph_v_kerning (hb_font_t *font HB_UNUSED,
			   void *font_data HB_UNUSED,
			   hb_codepoint_t top_glyph HB_UNUSED,
			   hb_codepoint_t bottom_glyph HB_UNUSED,
			   void *user_data HB_UNUSED)
{
    return 0;
}

static hb_bool_t
hb_sk_get_glyph_extents (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t glyph,
			 hb_glyph_extents_t *extents,
			 void *user_data HB_UNUSED)
{
    hb_skia_face *typeface = (hb_skia_face *)font_data;
    SkPaint paint;
    
    hb_position_t advance = 0;
    SkiaGetGlyphWidthAndExtents(&paint, typeface, glyph, 0, extents);
}

static hb_bool_t
hb_sk_get_glyph_contour_point (hb_font_t *font HB_UNUSED,
			       void *font_data,
			       hb_codepoint_t glyph,
			       unsigned int point_index,
			       hb_position_t *x,
			       hb_position_t *y,
			       void *user_data HB_UNUSED)
{
    return false;
}

static hb_bool_t
hb_sk_get_glyph_name (hb_font_t *font HB_UNUSED,
		      void *font_data,
		      hb_codepoint_t glyph,
		      char *name, unsigned int size,
		      void *user_data HB_UNUSED)
{
    return false;
}

static hb_bool_t
hb_sk_get_glyph_from_name (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   const char *name, int len, /* -1 means nul-terminated */
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
    return false;
}


static hb_font_funcs_t *
_hb_sk_get_font_funcs (void)
{
    static hb_font_funcs_t* hb_sk_font_funcs = 0;

    if (!hb_sk_font_funcs)
    {
        hb_sk_font_funcs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_func(hb_sk_font_funcs, hb_sk_get_glyph, 0, 0);
        hb_font_funcs_set_glyph_h_advance_func(hb_sk_font_funcs, hb_sk_get_glyph_h_advance, 0, 0);
        hb_font_funcs_set_glyph_v_advance_func(hb_sk_font_funcs, hb_sk_get_glyph_v_advance, 0, 0);
        hb_font_funcs_set_glyph_h_origin_func(hb_sk_font_funcs, hb_sk_get_glyph_h_origin, 0, 0);
        hb_font_funcs_set_glyph_v_origin_func(hb_sk_font_funcs, hb_sk_get_glyph_v_origin, 0, 0);
        hb_font_funcs_set_glyph_h_kerning_func(hb_sk_font_funcs, hb_sk_get_glyph_h_kerning, 0, 0);
        hb_font_funcs_set_glyph_v_kerning_func(hb_sk_font_funcs, hb_sk_get_glyph_v_kerning, 0, 0);
        hb_font_funcs_set_glyph_extents_func(hb_sk_font_funcs, hb_sk_get_glyph_extents, 0, 0);
        hb_font_funcs_set_glyph_contour_point_func(hb_sk_font_funcs, hb_sk_get_glyph_contour_point, 0, 0);
    }
    return hb_sk_font_funcs;
}


static hb_blob_t *
reference_table  (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
    hb_skia_face *hbs_face = (hb_skia_face *)user_data;
    
    const size_t tableSize = hbs_face -> typeface -> getTableSize(tag);
    if (!tableSize)
        return 0;

    char* buffer = reinterpret_cast<char*>(malloc(tableSize));
    if (!buffer)
        return 0;
    
    size_t actualSize = hbs_face -> typeface -> getTableData(tag, 0, tableSize, buffer);
    if (tableSize != actualSize)
        return 0;
    
    return hb_blob_create((const char*)buffer, tableSize, HB_MEMORY_MODE_WRITABLE, buffer, free);
}

hb_face_t *
hb_sk_face_create (hb_skia_face *p_face, hb_destroy_func_t destroy)
{
    hb_face_t* face = hb_face_create_for_tables(reference_table, p_face, 0);
    return face;
}

static void
hb_sk_face_finalize (hb_skia_face *p_typeface)
{

}

static void _do_nothing()
{
}

hb_font_t *
hb_sk_font_create (hb_skia_face *typeface, hb_destroy_func_t destroy)
{
    hb_font_t *font;
    hb_face_t *face;

    face = hb_sk_face_create (typeface, destroy);
    font = hb_font_create (face);
    hb_face_destroy (face);
    hb_font_set_funcs (font, _hb_sk_get_font_funcs (),
		     typeface, (hb_destroy_func_t) _do_nothing);
    
    hb_font_set_scale (font, typeface -> size, typeface -> size);
    int scale = typeface -> size * (1 << 16) / 1000;
    hb_font_set_ppem (font, scale, scale);

  return font;
}