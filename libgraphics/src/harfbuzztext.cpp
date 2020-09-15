/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "graphics.h"
#include "graphics-internal.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include <SkTypeface.h>
#include <SkPoint.h>
#include <SkTypes.h>
#include <SkFontMgr.h>
#include <SkUtils.h>
#include <SkTypeface_FreeType.h>

#include "foundation-unicode.h"
#include <unicode/uscript.h>

#define HB_SCALE_FACTOR 64

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCHarfbuzzShapeCallback)(void *context, const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count, const MCGFont &p_font);

extern bool MCCStringFromUnicodeSubstring(const unichar_t * p_text, uindex_t p_length, char*& r_text);
extern void MCCStringFree(char *p_string);

////////////////////////////////////////////////////////////////////////////////

static MCGFont MCGFontFromSkTypeface(SkTypeface *p_typeface, uindex_t p_size)
{
	MCGFont t_font;
	MCMemoryClear(t_font);
	t_font.fid = p_typeface;
	t_font.size = p_size;

	sk_sp<SkTypeface> t_typeface = sk_ref_sp<SkTypeface>(p_typeface);

	SkPaint t_paint;
	t_paint.setTypeface(t_typeface);
	t_paint.setTextSize(p_size);

	SkPaint::FontMetrics t_metrics;

	t_paint.getFontMetrics(&t_metrics);

	// SkPaint::FontMetrics gives the ascent value as a negative offset from the baseline, where we expect the (positive) distance.
	t_font.m_ascent = -t_metrics.fAscent;
	t_font.m_descent = t_metrics.fDescent;
	t_font.m_leading = t_metrics.fLeading;

	return t_font;
}

////////////////////////////////////////////////////////////////////////////////

static void skia_get_replacement_glyph(const MCGFont &p_font , uint16_t &glyph, hb_glyph_position_t &r_position)
{
	sk_sp<SkTypeface> t_typeface = sk_ref_sp<SkTypeface>((SkTypeface*)p_font.fid);

	SkPaint paint;
	paint . setTextEncoding(SkPaint::kUTF16_TextEncoding);
	paint . setTextSize(p_font.size);
	paint . setTypeface(t_typeface);

	codepoint_t t_replacement = 0xFFFD;

	SkScalar skWidth;
	SkRect skBounds;
	paint . textToGlyphs(&t_replacement, 2, &glyph);
	paint . getTextWidths(&t_replacement, 2, &skWidth, &skBounds);

	r_position.x_offset = skBounds.fLeft;
	r_position.y_offset = 0;
	r_position.x_advance = skWidth;
	r_position.y_advance = 0;
}

struct MCGlyphRun
{
    uindex_t count;
    SkPoint *positions;
    uint16_t *glyphs;
};

bool MCGlyphRunMake(const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_count, MCGPoint &x_location, MCGlyphRun& r_run)
{
	bool t_success;
	t_success = true;

	MCAutoArray<uint16_t> t_glyphs;
	MCAutoArray<SkPoint> t_positions;

	if (!t_glyphs.New(p_count) || !t_positions.New(p_count))
		return false;

	for (uindex_t i = 0; i < p_count; i++)
	{
		MCGFloat x_offset, y_offset;

		x_offset = x_location.x + (MCGFloat)p_positions[i].x_offset / (MCGFloat)HB_SCALE_FACTOR;
		y_offset = x_location.y + (MCGFloat)p_positions[i].y_offset / (MCGFloat)HB_SCALE_FACTOR;

		x_location.x += (MCGFloat)p_positions[i].x_advance / (MCGFloat)HB_SCALE_FACTOR;
		x_location.y += (MCGFloat)p_positions[i].y_advance / (MCGFloat)HB_SCALE_FACTOR;

		t_glyphs[i] = p_infos[i].codepoint;
		t_positions[i] = SkPoint::Make(x_offset, y_offset);
	}
	
	t_glyphs.Take(r_run.glyphs, r_run.count);
	t_positions.Take(r_run.positions, r_run.count);

	return true;
}

void MCGlyphRunDestroy(MCGlyphRun& p_run)
{
    MCMemoryDeleteArray(p_run.glyphs);
    MCMemoryDeleteArray(p_run.positions);
}

////////////////////////////////////////////////////////////////////////////////

// AL-2014-10-08: [[ Bug 13542 ]] Cache harfbuzz face structures
static MCGCacheTableRef s_hb_face_cache = nil;

#define kMCHarfbuzzFaceCacheTableSize 32
#define kMCHarfbuzzFaceCacheByteSize kMCHarfbuzzFaceCacheTableSize * 256
#define kMCHarfbuzzFaceCacheMaxOccupancy kMCHarfbuzzFaceCacheTableSize * 0.5

void MCGPlatformInitialize(void)
{
    s_hb_face_cache = nil;
    /* UNCHECKED */ MCGCacheTableCreate(kMCHarfbuzzFaceCacheTableSize, kMCHarfbuzzFaceCacheMaxOccupancy, kMCHarfbuzzFaceCacheByteSize, s_hb_face_cache);
}

void MCGPlatformFinalize(void)
{
    MCGCacheTableDestroy(s_hb_face_cache);
    s_hb_face_cache = nil;
}

////////////////////////////////////////////////////////////////////////////////

static hb_script_t HBScriptFromText(const unichar_t *p_text, uindex_t p_count)
{
    UScriptCode t_script;
    t_script = USCRIPT_COMMON;
    
    while (p_count && MCUnicodeIsWhitespace(*p_text))
    {
        p_text++;
        p_count--;
    }
    
    if (p_count)
    {
        UErrorCode t_error = U_ZERO_ERROR;
        t_script = uscript_getScript((uint32_t)(*p_text), &t_error);
        
        //MCLog("script: %s", uscript_getShortName(t_script));
    }
    
    return hb_icu_script_to_script(t_script);
}

struct MCHarfbuzzSkiaFace
{
	hb_font_t *hb_font;
	SkTypeface *sk_typeface;
	FT_Face ft_face;
	FT_Size ft_size;
	FT_Size ft_old_size;
	bool locked;
};

MCHarfbuzzSkiaFace *MCHarfbuzzGetFaceForSkiaTypeface(SkTypeface *p_typeface)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = s_hb_face_cache != nil;
	
	void *t_key;
	t_key = nil;
	
	uint32_t t_id;
	t_id = p_typeface -> uniqueID();
	
	// AL-2014-10-08: [[ Bug 13542 ]] Retrieve harfbuzz face from cache if possible
	if (t_success)
	{
		t_success = MCMemoryNew(sizeof(t_id), t_key);
		MCMemoryCopy(t_key, &t_id, sizeof(t_id));
	}
	
	if (t_success)
	{
		MCHarfbuzzSkiaFace *t_hb_sk_face;
		MCHarfbuzzSkiaFace **t_hb_sk_face_ptr;
		// IM-2015-10-02: [[ Bug 14786 ]] Make sure we store & dereference the *pointer* to MCHarfbuzzSkiaFace
		//    instead of its contents.
		t_hb_sk_face_ptr = (MCHarfbuzzSkiaFace **)MCGCacheTableGet(s_hb_face_cache, t_key, sizeof(t_id));
		if (t_hb_sk_face_ptr != nil)
		{
			t_hb_sk_face = *t_hb_sk_face_ptr;
			// AL-2014-10-27: [[ Bug 13802 ]] Make sure to set the size when we retrieve the cached face
			MCMemoryDelete(t_key);
			return t_hb_sk_face;
		}

		FT_Face t_ft_face;
		t_ft_face = SkTypeface_GetFTFace(p_typeface);

		FT_Size t_ft_size;
		/* UNCHECKED */ FT_Err_Ok == FT_New_Size(t_ft_face, &t_ft_size);

		hb_font_t *t_hb_ft_font;
		t_hb_ft_font = hb_ft_font_create(t_ft_face, nil);

		t_hb_sk_face = new (nothrow) MCHarfbuzzSkiaFace;
		t_hb_sk_face->hb_font = t_hb_ft_font;
		t_hb_sk_face->ft_face = t_ft_face;
		t_hb_sk_face->ft_size = t_ft_size;
		t_hb_sk_face->sk_typeface = p_typeface;
		t_hb_sk_face->locked = false;

		p_typeface -> ref();
		
		// IM-2015-10-02: [[ Bug 14786 ]] Make sure we store & dereference the *pointer* to MCHarfbuzzSkiaFace
		//    instead of its contents.
		MCGCacheTableSet(s_hb_face_cache, t_key, sizeof(t_id), &t_hb_sk_face, sizeof(MCHarfbuzzSkiaFace*));
		return t_hb_sk_face;
	}
	
	if (!t_success)
		MCMemoryDelete(t_key);
	
	return nil;
}

bool MCHarfbuzzLockFont(MCHarfbuzzSkiaFace *p_face, uint16_t p_size, hb_font_t *&r_font)
{
	if (p_face == nil)
		return false;

	if (p_face->locked)
		return false;

	bool t_success;
	t_success = true;

	p_face->ft_old_size = p_face->ft_face->size;
	if (t_success)
		t_success = FT_Err_Ok == FT_Activate_Size(p_face->ft_size);
	if (t_success)
		t_success = FT_Err_Ok == FT_Set_Char_Size(p_face->ft_face, p_size * HB_SCALE_FACTOR, 0, 0, 0);

	if (t_success)
	{
		FT_Reference_Face(p_face->ft_face);

		hb_font_set_scale(p_face->hb_font, HB_SCALE_FACTOR, HB_SCALE_FACTOR);
		hb_font_set_ppem (p_face->hb_font, p_face->ft_size->metrics.x_ppem, p_face->ft_size->metrics.y_ppem);
		p_face->locked = true;

		r_font = p_face->hb_font;
	}

	return t_success;
}

void MCHarfbuzzUnlockFont(MCHarfbuzzSkiaFace *p_face)
{
	if (p_face == nil)
		return;
	if (!p_face->locked)
		return;

	/* UNCHECKED */ FT_Err_Ok == FT_Activate_Size(p_face->ft_old_size);
	FT_Done_Face(p_face->ft_face);
	p_face->locked = false;
}

// Check if the font supports all glyphs in a given cluster from shape info
static bool cluster_is_supported(hb_glyph_info_t* p_info, uindex_t p_index, uindex_t p_count, uindex_t& r_cluster_end)
{
    bool t_supported = true;
    uindex_t t_cluster = p_info[p_index] . cluster;
    while (p_index < p_count &&
           p_info[p_index] . cluster == t_cluster)
    {
        if (p_info[p_index] . codepoint == 0)
        {
            // If any of these glyphs' codepoints are 0 then the
            // whole cluster is unsupported
            t_supported = false;
        }
        p_index++;
    }
    
    r_cluster_end = p_index;
    return t_supported;
}

////////////////////////////////////////////////////////////////////////////////

inline void cluster_to_char_bounds(uindex_t p_start, uindex_t p_end, const hb_glyph_info_t *p_infos, uindex_t p_glyph_count, uindex_t p_char_count, bool p_rtl, uindex_t &r_char_start, uindex_t &r_char_end)
{
	if (p_rtl)
	{
		r_char_start = p_infos[p_end - 1].cluster;
		r_char_end = p_char_count;
		if (p_start > 0)
			r_char_end = p_infos[p_start - 1].cluster;
	}
	else
	{
		r_char_start = p_infos[p_start].cluster;
		r_char_end = p_char_count;
		if (p_end < p_glyph_count)
			r_char_end = p_infos[p_end].cluster;
	}
}

static bool MCHarfbuzzShape(const unichar_t* p_text, uindex_t p_char_count, bool p_rtl, const MCGFont &p_font, bool p_use_fallback, MCHarfbuzzShapeCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	sk_sp<SkTypeface> t_typeface;

	MCGFont t_fallback_font;

	const MCGFont *t_font;
	t_font = nil;

	if (p_use_fallback)
	{
		// If we're to use a fallback font, use Skia's font manager to find a
		// font that has glyphs for the first char in the string.
		const unichar_t* t_text = p_text;
		SkUnichar t_uni_char = SkUTF16_NextUnichar(&t_text);
		sk_sp<SkFontMgr> t_fnt_mgr(SkFontMgr::RefDefault());
		t_typeface = sk_ref_sp<SkTypeface>(t_fnt_mgr -> matchFamilyStyleCharacter(nil, ((SkTypeface *)p_font . fid) -> fontStyle(), nil, 0, t_uni_char));
	}

	if (t_typeface != nil)
	{
		t_fallback_font = MCGFontFromSkTypeface(t_typeface.get(), p_font.size);
		t_font = &t_fallback_font;
	}
	else
	{
		// If there is no fallback font for the char, use the replacement glyph
		// in the original font.
		t_typeface = sk_ref_sp<SkTypeface>((SkTypeface *)p_font . fid);
		t_font = &p_font;
	}
	
	t_success = t_typeface != nil;
	
	MCHarfbuzzSkiaFace *t_hb_sk_face;
	t_hb_sk_face = nil;
	if (t_success)
	{
		t_hb_sk_face = MCHarfbuzzGetFaceForSkiaTypeface(t_typeface.get());
		t_success = t_hb_sk_face != nil;
	}
	
	hb_font_t *t_hb_font;
	t_hb_font = nil;
	if (t_success)
		t_success = MCHarfbuzzLockFont(t_hb_sk_face, p_font.size, t_hb_font);

	// Set up the HarfBuzz buffer
	hb_buffer_t *buffer;
	buffer = nil;
	uindex_t glyph_count;
	glyph_count = 0;
	hb_glyph_info_t *glyph_info;
	glyph_info = nil;
	hb_glyph_position_t *glyph_pos;
	glyph_pos = nil;

	if (t_success)
	{
		buffer = hb_buffer_create();

		hb_buffer_set_unicode_funcs(buffer, hb_icu_get_unicode_funcs());
		hb_buffer_set_direction(buffer, p_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
		hb_buffer_set_script(buffer, HBScriptFromText(p_text, p_char_count));

		hb_buffer_add_utf16(buffer, p_text, p_char_count, 0, p_char_count);
		hb_shape(t_hb_font, buffer, NULL, 0);

		glyph_count = hb_buffer_get_length(buffer);
		glyph_info = hb_buffer_get_glyph_infos(buffer, 0);
		glyph_pos = hb_buffer_get_glyph_positions(buffer, 0);
	}
	
	if (t_hb_sk_face != nil && t_hb_font != nil)
		MCHarfbuzzUnlockFont(t_hb_sk_face);

	uindex_t t_cur_glyph = 0;
	
	bool t_continue;
	t_continue = true;

	// deal with runs of supported and unsupported glyphs
	while (t_continue && t_success && t_cur_glyph < glyph_count)
	{
		uindex_t t_start;
		t_start = t_cur_glyph;

		uindex_t t_cluster_end;
		t_cluster_end = 0;
		
		if (cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end))
		{
			// Run of glyphs in successfully shaped clusters
			t_cur_glyph = t_cluster_end;
			while (t_cur_glyph < glyph_count && cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end))
				t_cur_glyph = t_cluster_end;

			uindex_t t_char_start, t_char_end;
			cluster_to_char_bounds(t_start, t_cur_glyph, glyph_info, glyph_count, p_char_count, p_rtl, t_char_start, t_char_end);

			t_continue = p_callback(p_context,
				glyph_info + t_start,
				glyph_pos + t_start,
				t_cur_glyph - t_start,
				p_text + t_char_start,
				t_char_end - t_char_start,
				*t_font);
		}
		else if (t_cur_glyph == 0 && p_use_fallback)
		{
			// If the first cluster is unsupported and we're already using the fallback
			// font for that cluster, assume there is no support and use a replacement.
			t_cur_glyph = t_cluster_end;

			uindex_t t_char_start, t_char_end;
			cluster_to_char_bounds(t_start, t_cur_glyph, glyph_info, glyph_count, p_char_count, p_rtl, t_char_start, t_char_end);

			// Use a replacement glyph for the whole cluster
			hb_glyph_info_t t_replacement_info;
			t_replacement_info = glyph_info[t_start];

			hb_glyph_position_t t_replacement_position;
			t_replacement_position = glyph_pos[t_start];

			uint16_t t_replacement_glyph;
			skia_get_replacement_glyph(p_font, t_replacement_glyph, t_replacement_position);

			t_replacement_info.codepoint = t_replacement_glyph;

			t_continue = p_callback(p_context,
				&t_replacement_info,
				&t_replacement_position,
				1,
				p_text + t_char_start,
				t_char_end - t_char_start,
				p_font);
		}
		else
		{
			// Deal with run of unsupported clusters for this font.
			t_cur_glyph = t_cluster_end;
			while (t_cur_glyph < glyph_count && !cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end))
				t_cur_glyph = t_cluster_end;

			uindex_t t_char_start, t_char_end;
			cluster_to_char_bounds(t_start, t_cur_glyph, glyph_info, glyph_count, p_char_count, p_rtl, t_char_start, t_char_end);

			// For the run of unsupported clusters, shape using a fallback font.
			t_success = MCHarfbuzzShape(
				p_text + t_char_start,
				t_char_end - t_char_start,
				p_rtl,
				p_font,
				true,
				p_callback,
				p_context
			);
		}
	}
	
	if (buffer != nil)
		hb_buffer_destroy(buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct _draw_text_context_t
{
	SkCanvas *canvas;
	SkPaint paint;
	MCGPoint location;
};

static bool _draw_text_callback(void *context, const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count, const MCGFont &p_font)
{
	_draw_text_context_t *self;
	self = (_draw_text_context_t*)context;

	sk_sp<SkTypeface> t_typeface = sk_ref_sp<SkTypeface>((SkTypeface*)p_font.fid);

	MCGlyphRun t_run;
	MCMemoryClear(t_run);
	if (MCGlyphRunMake(p_infos, p_positions, p_glyph_count, self->location, t_run))
	{
		self->paint.setTypeface(t_typeface);
		self->canvas->drawPosText(t_run.glyphs, t_run.count * 2, t_run.positions, self->paint);
		MCGlyphRunDestroy(t_run);
	}

	return true;
}

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	if (!MCGContextIsValid(self))
		return;	
	
	SkPaint t_paint;
	if (!MCGContextSetupFill(self, t_paint))
	{
		self->is_valid = false;
		return;
	}
	
	// Force skia to render text with antialiasing enabled.
	t_paint . setAntiAlias(true);

	t_paint . setTextSize(p_font . size);
	t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
	
	_draw_text_context_t t_context;
	t_context.canvas = self->layer->canvas;
	t_context.paint = t_paint;
	t_context.location = p_location;

	/* UNCHECKED */ MCHarfbuzzShape(p_text, p_length / 2, p_rtl, p_font, false, _draw_text_callback, &t_context);

	self -> is_valid = true;
}

//////////

static bool _measure_text_callback(void *context, const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count, const MCGFont &p_font)
{
	MCGFloat *t_width;
	t_width = (MCGFloat*)context;

	for (uindex_t i = 0; i < p_glyph_count; i++)
	{
		MCGFloat x_advance;
		x_advance = (MCGFloat)p_positions[i].x_advance / (MCGFloat)HB_SCALE_FACTOR;

		*t_width += x_advance;
	}

	return true;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	MCGFloat t_width;
	t_width = 0.0;
	
	/* UNCHECKED */ MCHarfbuzzShape(p_text, p_length / 2, false, p_font, false, _measure_text_callback, &t_width);

	return t_width;
}

////////////////////////////////////////////////////////////////////////////////

struct _measure_image_bounds_context_t
{
	SkPaint paint;
	MCGPoint location;
	SkRect bounds;
};

static bool _measure_image_bounds_callback(void *context, const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count, const MCGFont &p_font)
{
	_measure_image_bounds_context_t *self;
	self = (_measure_image_bounds_context_t*)context;

	sk_sp<SkTypeface> t_typeface = sk_ref_sp<SkTypeface>((SkTypeface*)p_font.fid);

	MCAutoArray<SkRect> t_glyph_bounds;

	MCGlyphRun t_run;
	MCMemoryClear(t_run);
	if (MCGlyphRunMake(p_infos, p_positions, p_glyph_count, self->location, t_run))
	{
		/* UNCHECKED */ t_glyph_bounds.New(t_run.count);
		self->paint.setTypeface(t_typeface);
		self->paint.getTextWidths(t_run.glyphs, p_glyph_count * 2, nil, t_glyph_bounds.Ptr());

		for (uindex_t i = 0; i < t_run.count; i++)
		{
			t_glyph_bounds[i].offset(t_run.positions[i]);
			self->bounds.join(t_glyph_bounds[i]);
		}
		MCGlyphRunDestroy(t_run);
	}

	return true;
}

bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	SkPaint t_paint;
	t_paint . setTextSize(p_font . size);
	t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
	
	_measure_image_bounds_context_t t_context;
	t_context.paint = t_paint;
	t_context.location = MCGPointMake(0,0);
	t_context.bounds = SkRect::MakeEmpty();

	/* UNCHECKED */ MCHarfbuzzShape(p_text, p_length / 2, false, p_font, false, _measure_image_bounds_callback, &t_context);

	r_bounds = MCGRectangleFromSkRect(t_context.bounds);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct _layout_text_context_t
{
	MCGFontLayoutTextCallback callback;
	void *context;
	const unichar_t *chars;
	bool success;
};

static bool _layout_text_callback(void *context, const hb_glyph_info_t *p_infos, const hb_glyph_position_t *p_positions, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count, const MCGFont &p_font)
{
	_layout_text_context_t *self;
	self = (_layout_text_context_t*)context;

	MCAutoArray<MCGGlyphInfo> t_info;
	if (!t_info.New(p_glyph_count))
	{
		self->success = false;
		return false;
	}

	// Glyph cluster points to a char position in the original string
	// Calculate the offset of this substring so we can maintain the 
	// relationship from glyph to char index
	uindex_t t_char_offset;
	t_char_offset = p_chars - self->chars;

	for (uindex_t i = 0; i < p_glyph_count; i++)
	{
		t_info[i].codepoint = p_infos[i].codepoint;
		t_info[i].cluster = p_infos[i].cluster - t_char_offset;
		t_info[i].x_offset = (MCGFloat)p_positions[i].x_offset / (MCGFloat)HB_SCALE_FACTOR;
		t_info[i].y_offset = (MCGFloat)p_positions[i].y_offset / (MCGFloat)HB_SCALE_FACTOR;
		t_info[i].x_advance = (MCGFloat)p_positions[i].x_advance / (MCGFloat)HB_SCALE_FACTOR;
		t_info[i].y_advance = (MCGFloat)p_positions[i].y_advance / (MCGFloat)HB_SCALE_FACTOR;
	}

	return self->callback(self->context, p_font, t_info.Ptr(), t_info.Size(), p_chars, p_char_count);
}

bool MCGFontLayoutText(const MCGFont &p_font, const unichar_t *p_text, uindex_t p_char_count, bool p_rtl, MCGFontLayoutTextCallback p_callback, void *p_context)
{
	_layout_text_context_t t_context;
	t_context.callback = p_callback;
	t_context.context = p_context;
	t_context.success = true;

	bool t_success;
	t_success = MCHarfbuzzShape(p_text, p_char_count, p_rtl, p_font, false, _layout_text_callback, &t_context);
		return t_success && t_context.success;
}

////////////////////////////////////////////////////////////////////////////////

