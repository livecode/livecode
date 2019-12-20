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

extern bool MCCStringFromUnicodeSubstring(const unichar_t * p_text, uindex_t p_length, char*& r_text);
extern void MCCStringFree(char *p_string);

////////////////////////////////////////////////////////////////////////////////

static void skia_get_replacement_glyph(uint16_t p_size, sk_sp<SkTypeface> p_typeface, uint16_t& glyph, MCGFloat& advance)
{
    SkPaint paint;
    paint . setTextEncoding(SkPaint::kUTF16_TextEncoding);
    paint . setTextSize(p_size);
    paint . setTypeface(p_typeface);
    
    codepoint_t t_replacement = 0xFFFD;
    
    SkScalar skWidth;
    SkRect skBounds;
    paint . textToGlyphs(&t_replacement, 2, &glyph);
    paint . getTextWidths(&t_replacement, 2, &skWidth, &skBounds);
    
    advance = (MCGFloat)skWidth;
}

struct MCGlyphRun
{
    uindex_t count;
    SkPoint *positions;
    uint16_t *glyphs;
    sk_sp<SkTypeface> typeface;
};

void MCGlyphRunMake(hb_glyph_info_t *p_infos, hb_glyph_position_t *p_positions, MCGPoint* x_location, uindex_t p_start, uindex_t p_end, SkTypeface* p_typeface, uint16_t p_size, MCGlyphRun& r_run)
{
    // Skia APIs expect typefaces to be passed as shared pointers
    sk_sp<SkTypeface> t_typeface(p_typeface);
    t_typeface->ref();
    
    uindex_t t_count = p_end - p_start;
    MCMemoryNewArray(t_count, r_run.glyphs);
    MCMemoryNewArray(t_count, r_run.positions);
    
	// IM-2016-03-31: [[ Bug 17281 ]] positions may have negative values, so we need to use signed offsets.
	MCGFloat x_offset, y_offset;
	
    uindex_t run_index = 0;
    MCGFloat advance_y = 0;
    MCGFloat advance_x = 0;
    
    for (uindex_t i = p_start; i < p_end; i++)
    {
        x_offset = x_location -> x + (MCGFloat)p_positions[i] . x_offset / (MCGFloat)HB_SCALE_FACTOR + advance_x;
        y_offset = x_location -> y + (MCGFloat)p_positions[i] . y_offset / (MCGFloat)HB_SCALE_FACTOR + advance_y;
        r_run . positions[run_index] = SkPoint::Make(x_offset, y_offset);
        
        uint16_t t_glyph = p_infos[i] . codepoint;
        if (t_glyph)
        {
            r_run . glyphs[run_index] = t_glyph;

            advance_x += (MCGFloat)p_positions[i] . x_advance / (MCGFloat)HB_SCALE_FACTOR;
            advance_y += (MCGFloat)p_positions[i] . y_advance / (MCGFloat)HB_SCALE_FACTOR;
        }
        else
        {
            MCGFloat advance;
            skia_get_replacement_glyph(p_size, t_typeface, r_run . glyphs[run_index], advance);
            advance_x += advance;
        }
            
        run_index++;
    }
    
    x_location -> x += advance_x;
    x_location -> y += advance_y;
    
    r_run.typeface = t_typeface;
    
    r_run . count = t_count;
}

void MCGlyphRunDestroy(MCGlyphRun& p_run)
{
    MCMemoryDeleteArray(p_run.glyphs);
    MCMemoryDeleteArray(p_run.positions);
    p_run.typeface.reset();
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

static uindex_t shape_text_and_add_to_glyph_array(const unichar_t* p_text, uindex_t p_char_count, bool p_rtl, const MCGFont &p_font, bool p_use_fallback, MCGPoint* x_location, MCAutoArray<MCGlyphRun>& x_runs)
{
    if (p_font . fid == nil)
        return 0;
    
    SkTypeface *t_typeface = nil;
    if (p_use_fallback)
    {
        // If we're to use a fallback font, use Skia's font manager to find a
        // font that has glyphs for the first char in the string.
        const unichar_t* t_text = p_text;
        SkUnichar t_uni_char = SkUTF16_NextUnichar(&t_text);
        sk_sp<SkFontMgr> t_fnt_mgr(SkFontMgr::RefDefault());
        t_typeface = t_fnt_mgr -> matchFamilyStyleCharacter(nil, ((SkTypeface *)p_font . fid) -> fontStyle(), nil, 0, t_uni_char);
        
        // If there is no fallback font for the char, use the replacement glyph
        // in the original font.
        if (t_typeface == nil)
            t_typeface = (SkTypeface *)p_font . fid;
    }
    else
        t_typeface = (SkTypeface *)p_font . fid;
    
    if (t_typeface == nil)
        return 0;
    
    MCHarfbuzzSkiaFace *t_hb_sk_face;
    t_hb_sk_face = MCHarfbuzzGetFaceForSkiaTypeface(t_typeface);
    
    if (t_hb_sk_face == nil)
        return 0;
    
    hb_font_t *t_hb_font;
    t_hb_font = nil;
    /* UNCHECKED */ MCHarfbuzzLockFont(t_hb_sk_face, p_font.size, t_hb_font);

    // Set up the HarfBuzz buffer
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buffer, p_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    
    hb_buffer_set_script(buffer, HBScriptFromText(p_text, p_char_count));
    
    hb_buffer_add_utf16(buffer, p_text, p_char_count, 0, p_char_count);
    
    hb_shape(t_hb_font, buffer, NULL, 0);
    
    uindex_t glyph_count = hb_buffer_get_length(buffer);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, 0);
    
    uindex_t t_cur_glyph = 0, t_start = 0, t_run_count = 0;
    uindex_t t_cluster_end = 0;
    
    // deal with runs of supported and unsupported glyphs
    while (t_cur_glyph < glyph_count)
    {
        MCGlyphRun t_run;
        t_start = t_cur_glyph;
        
        // Run of glyphs in successfully shaped clusters
        while (t_cur_glyph < glyph_count &&
               cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end))
        {
            t_cur_glyph = t_cluster_end;
        }

        if (t_start != t_cur_glyph)
        {
            MCGlyphRunMake(glyph_info, glyph_pos, x_location, t_start, t_cur_glyph, t_typeface, p_font . size, t_run);
            x_runs . Push(t_run);
            t_start = t_cur_glyph;
            t_run_count++;
        }
        
        // If the first cluster is unsupported and we're already using the fallback
        // font for that cluster, assume there is no support and use a replacement.
        if (t_cur_glyph == 0 &&
            !cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end)
            && p_use_fallback)
        {
            // Enforce a replacement glyph for the whole cluster by passing a 0
            // codepoint and only adding a single glyph to the run
            glyph_info[t_cur_glyph].codepoint = 0;
            MCGlyphRunMake(glyph_info, glyph_pos, x_location, t_start, t_start + 1, (SkTypeface *)p_font . fid, p_font . size, t_run);
            x_runs . Push(t_run);
            t_cur_glyph = t_cluster_end;
            t_start = t_cur_glyph;
            t_run_count++;
        }
        
        // Deal with run of unsupported clusters for this font.
        while (t_cur_glyph < glyph_count &&
               !cluster_is_supported(glyph_info, t_cur_glyph, glyph_count, t_cluster_end))
        {
            t_cur_glyph = t_cluster_end;
        }
        
        // For the run of unsupported clusters, shape using a fallback font.
        if (t_start != t_cur_glyph)
        {
            // Reshape from the beginning of the unsupported cluster run
            // to the beginning of the cluster containing t_cur_glyph.
            // At this point, t_cur_glyph might be equal to glyph_count,
            // in which case compute the reshape char count using the old char
            // count.
            uindex_t t_end_index = p_char_count;
            if (t_cur_glyph != glyph_count)
                t_end_index = glyph_info[t_cur_glyph] . cluster;

            t_run_count += shape_text_and_add_to_glyph_array(
                p_text + glyph_info[t_start] . cluster,
                t_end_index - glyph_info[t_start] . cluster,
                p_rtl,
                p_font,
                true,
                x_location,
                x_runs
            );
        }
    }
    
    hb_buffer_destroy(buffer);

    MCHarfbuzzUnlockFont(t_hb_sk_face);

    return t_run_count;
}

void shape(const unichar_t* p_text, uindex_t p_char_count, MCGPoint p_location, bool p_rtl, const MCGFont &p_font, MCGlyphRun*& r_runs, uindex_t& r_run_count)
{
    MCAutoArray<MCGlyphRun> t_runs;
    uindex_t t_run_count = shape_text_and_add_to_glyph_array(p_text, p_char_count, p_rtl, p_font, false, &p_location, t_runs);

    if (t_run_count == 0)
    {
        r_run_count = 0;
        r_runs = nil;
    }
    else
    {
        r_run_count = t_run_count;
        t_runs . Take(r_runs, r_run_count);
    }
}

////////////////////////////////////////////////////////////////////////////////

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
    
    MCGlyphRun *t_glyph_runs;
    uindex_t t_count;

    shape(p_text, p_length/2, p_location, p_rtl, p_font, t_glyph_runs, t_count);
    
    for (uindex_t i = 0; i < t_count; i++)
    {
        t_paint . setTypeface(t_glyph_runs[i] . typeface);
        self -> layer -> canvas -> drawPosText(&(t_glyph_runs[i] . glyphs[0]), t_glyph_runs[i] . count * 2, &(t_glyph_runs[i] . positions[0]), t_paint);
        
        MCGlyphRunDestroy(t_glyph_runs[i]);
    }
    
    MCMemoryDeleteArray(t_glyph_runs);
    
	self -> is_valid = true;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
    
	MCGFloat t_width;
	t_width = 0.0;
    
    SkPaint t_paint;
    t_paint . setTextSize(p_font . size);
    t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    
    MCGlyphRun *t_glyph_runs;
    uindex_t t_count;
    
    MCGPoint t_dummy = MCGPointMake(0,0);
    shape(p_text, p_length/2, t_dummy, false, p_font, t_glyph_runs, t_count);
    
    for (uindex_t i = 0; i < t_count; i++)
    {
        t_paint . setTypeface(t_glyph_runs[i] . typeface);
        t_width +=  (MCGFloat) t_paint . measureText(&(t_glyph_runs[i] . glyphs[0]), t_glyph_runs[i] . count * 2);
        
        MCGlyphRunDestroy(t_glyph_runs[i]);
    }
    
    MCMemoryDeleteArray(t_glyph_runs);

	//self -> is_valid = t_success;
	return t_width;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	SkRect t_bounds;
	t_bounds = SkRect::MakeEmpty();
	
	SkPaint t_paint;
	t_paint . setTextSize(p_font . size);
	t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
	
	MCGlyphRun *t_glyph_runs;
	uindex_t t_count;
	
	MCGPoint t_dummy = MCGPointMake(0,0);
	shape(p_text, p_length/2, t_dummy, false, p_font, t_glyph_runs, t_count);
	
	for (uindex_t i = 0; i < t_count; i++)
	{
		SkRect t_glyph_bounds;
		t_paint . setTypeface(t_glyph_runs[i] . typeface);
		t_paint . measureText(&(t_glyph_runs[i] . glyphs[0]), t_glyph_runs[i] . count * 2, &t_glyph_bounds);
		
		if (!t_bounds.isEmpty())
			t_glyph_bounds.offsetTo(t_bounds.right(), t_glyph_bounds.y());
		t_bounds.join(t_glyph_bounds);
		
		MCGlyphRunDestroy(t_glyph_runs[i]);
	}
	
	MCMemoryDeleteArray(t_glyph_runs);
	
	r_bounds = MCGRectangleFromSkRect(t_bounds);
	return true;
}

