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

#include "prefix.h"

#include "textlayout.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include <SkTypeface.h>
#include <SkFontMgr.h>
#include <SkUtils.h>
#include <SkTypeface_FreeType.h>

#include "graphics.h"
#include "mblandroidtypeface.h"

////////////////////////////////////////////////////////////////////////////////

#define HB_SCALE_FACTOR (64)

////////////////////////////////////////////////////////////////////////////////

bool MCTextLayoutInitialize(void)
{
	return true;
}

void MCTextLayoutFinalize(void)
{
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

bool make_textlayout_glyphs(hb_glyph_info_t *p_glyph_infos, hb_glyph_position_t *p_glyph_positions, uindex_t p_start, uindex_t p_length, MCGPoint &x_location, MCTextLayoutGlyph *&r_glyphs)
{
	bool t_success;
	t_success = true;
	
	MCTextLayoutGlyph *t_glyphs;
	t_glyphs = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_length, t_glyphs);
	
	if (t_success)
	{
		for (uindex_t i = 0; i < p_length; i++)
		{
			t_glyphs[i].index = p_glyph_infos[p_start + i].codepoint;
			t_glyphs[i].x = x_location.x + ((float)p_glyph_positions[p_start + i].x_offset / HB_SCALE_FACTOR);
			t_glyphs[i].y = x_location.y + ((float)p_glyph_positions[p_start + i].y_offset / HB_SCALE_FACTOR);
			
			x_location.x += ((float)p_glyph_positions[p_start + i].x_advance / HB_SCALE_FACTOR);
			x_location.y += ((float)p_glyph_positions[p_start + i].y_advance / HB_SCALE_FACTOR);
		}
		
		r_glyphs = t_glyphs;
	}
	else
	{
		if (t_glyphs != nil)
			MCMemoryDeleteArray(t_glyphs);
	}
	
	return t_success;
}

bool make_textlayout_clusters(hb_glyph_info_t *p_glyph_infos, uindex_t p_start, uindex_t p_length, uindex_t p_char_count, uint16_t *&r_clusters)
{
	bool t_success;
	t_success = true;
	
	uint16_t *t_clusters;
	t_clusters = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_char_count, t_clusters);
	
	if (t_success)
	{
		uindex_t t_current_glyph;
		t_current_glyph = 0;
		
		uindex_t t_current_cluster;
		t_current_cluster = p_glyph_infos[p_start].cluster;
		
		uindex_t i;
		i = 0;
		
		while (t_success && i < p_char_count)
		{
			uindex_t t_next_cluster;
			t_next_cluster = t_current_cluster;
			
			uindex_t t_next_glyph;
			t_next_glyph = t_current_glyph;
			
			while (t_next_cluster == t_current_cluster)
			{
				t_next_glyph++;
				if (t_next_glyph < p_length)
					t_next_cluster = p_glyph_infos[p_start + t_next_glyph].cluster;
				else
					t_next_cluster = UINDEX_MAX;
			}
			
			while (i < p_char_count && t_current_cluster < t_next_cluster)
			{
				t_clusters[i] = t_current_glyph;
				i++;
				t_current_cluster++;
			}
			
			t_current_glyph = t_next_glyph;
		}
	}
	
	if (t_success)
		r_clusters = t_clusters;
	else
	{
		if (t_clusters != nil)
			MCMemoryDeleteArray(t_clusters);
	}
	
	return t_success;
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

bool shape(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCGPoint &x_location, bool p_use_fallback, MCTextLayoutCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;
	
	SkTypeface *t_sk_typeface;
	t_sk_typeface = nil;
	
	if (p_use_fallback)
	{
        // If we're to use a fallback font, use Skia's font manager to find a
        // font that has glyphs for the first char in the string.
        const unichar_t* t_text = p_chars;
        SkUnichar t_uni_char = SkUTF16_NextUnichar(&t_text);
        sk_sp<SkFontMgr> t_fnt_mgr(SkFontMgr::RefDefault());
        t_sk_typeface = t_fnt_mgr -> matchFamilyStyleCharacter(nil, ((SkTypeface *)p_font -> fid) -> fontStyle(), nil, 0, t_uni_char);
	}
	
	// If there is no fallback font for the char, use the replacement glyph
	// in the original font.
	if (t_sk_typeface == nil)
	{
		MCAndroidFont *t_android_font;
		t_android_font = (MCAndroidFont*)p_font->fid;
		
		t_sk_typeface = (SkTypeface*)t_android_font->typeface;
		t_sk_typeface->ref();
	}
	
	// Make the font
	
	FT_Face t_ft_face;
	t_ft_face = nil;
	if (t_success)
	{
		t_ft_face = SkTypeface_GetFTFace(t_sk_typeface);
		t_success = (t_ft_face != nil);
	}
	
	// Make sure the face has the required size. Use a new FT_Size object so
	// that the previous size settings can be restored after shaping.
	FT_Size t_old_ft_size;
	t_old_ft_size = nil;
	
	FT_Size t_ft_size;
	t_ft_size = nil;
	if (t_success)
	{
		t_old_ft_size = t_ft_face->size;
		
		t_success = FT_Err_Ok == FT_New_Size(t_ft_face, &t_ft_size);
	}
	
	if (t_success)
		t_success = FT_Err_Ok == FT_Activate_Size(t_ft_size);
	
	if (t_success)
		t_success = FT_Err_Ok == FT_Set_Char_Size(t_ft_face, p_font->size * 64, 0, 0, 0);
	
	hb_font_t *t_hb_font;
	t_hb_font = nil;
	if (t_success)
	{
		t_hb_font = hb_ft_font_create(t_ft_face, nil);
		t_success = (t_hb_font != nil);
	}
	
	// Scale font for sub-pixel precision
	if (t_success)
		hb_font_set_scale(t_hb_font, HB_SCALE_FACTOR, HB_SCALE_FACTOR);
	
	hb_buffer_t *t_hb_buffer;
	t_hb_buffer = nil;
	if (t_success)
	{
		t_hb_buffer = hb_buffer_create();
		t_success = hb_buffer_allocation_successful(t_hb_buffer);
	}
	
	if (t_success)
	{
		hb_buffer_set_unicode_funcs(t_hb_buffer, hb_icu_get_unicode_funcs());
		hb_buffer_set_direction(t_hb_buffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(t_hb_buffer, HBScriptFromText(p_chars, p_char_count));
		
		hb_buffer_add_utf16(t_hb_buffer, p_chars, p_char_count, 0, p_char_count);
		
		hb_shape(t_hb_font, t_hb_buffer, NULL, 0);
	}
	
	uindex_t t_glyph_count;
	t_glyph_count = 0;
	hb_glyph_info_t *t_glyph_infos;
	t_glyph_infos = nil;
	hb_glyph_position_t *t_glyph_positions;
	t_glyph_positions = nil;
	if (t_success)
	{
		t_glyph_count = hb_buffer_get_length(t_hb_buffer);
		t_glyph_infos = hb_buffer_get_glyph_infos(t_hb_buffer, nil);
		t_glyph_positions = hb_buffer_get_glyph_positions(t_hb_buffer, nil);
	}
	
	if (t_hb_font != nil)
		hb_font_destroy(t_hb_font);
	
	if (t_old_ft_size != nil)
		FT_Activate_Size(t_old_ft_size);
	if (t_ft_size != nil)
		FT_Done_Size(t_ft_size);

    uindex_t t_cur_glyph;
    t_cur_glyph = 0;
    
    // deal with runs of supported and unsupported glyphs
    while (t_cur_glyph < t_glyph_count)
	{
		uindex_t t_start, t_cluster_end;
		t_start = t_cur_glyph;
		t_cluster_end = 0;
		
		// Run of glyphs in successfully shaped clusters
		while (t_cur_glyph < t_glyph_count && cluster_is_supported(t_glyph_infos, t_cur_glyph, t_glyph_count, t_cluster_end))
			t_cur_glyph = t_cluster_end;
		
		if (t_start != t_cur_glyph)
		{
			uindex_t t_char_start, t_char_end;
			t_char_start = t_glyph_infos[t_start].cluster;
			
			t_char_end = p_char_count;
			if (t_cur_glyph != t_glyph_count)
				t_char_end = t_glyph_infos[t_cur_glyph].cluster;

			MCTextLayoutGlyph *t_glyphs;
			t_glyphs = nil;
			if (t_success)
				t_success = make_textlayout_glyphs(t_glyph_infos, t_glyph_positions, t_start, t_cur_glyph - t_start, x_location, t_glyphs);
			
			uint16_t *t_clusters;
			t_clusters = nil;
			if (t_success)
				t_success = make_textlayout_clusters(t_glyph_infos, t_start, t_cur_glyph - t_start, t_char_end - t_char_start, t_clusters);
			
			if (t_success)
			{
				MCTextLayoutSpan t_span;
				t_span.chars = p_chars + t_char_start;
				t_span.char_count = t_char_end - t_char_start;
				t_span.clusters = t_clusters;
				t_span.glyphs = t_glyphs;
				t_span.glyph_count = t_cur_glyph - t_start;
				t_span.font = t_ft_face;
				
				t_success = p_callback(p_context, &t_span);
			}
			
			if (t_glyphs != nil)
				MCMemoryDeleteArray(t_glyphs);
			if (t_clusters != nil)
				MCMemoryDeleteArray(t_clusters);
			
			t_start = t_cur_glyph;
		}
		
		// If the first cluster is unsupported and we're already using the fallback
		// font for that cluster, assume there is no support and use a replacement.
		if (t_cur_glyph == 0 && p_use_fallback)
		{
			t_cur_glyph = t_cluster_end;
			
            // Enforce a replacement glyph for the whole cluster by passing a 0
            // codepoint and only adding a single glyph to the run
            MCTextLayoutGlyph t_glyph;
            t_glyph.index = 0;
            t_glyph.x = x_location.x + ((float)t_glyph_positions[0].x_offset / HB_SCALE_FACTOR);
            t_glyph.y = x_location.y + ((float)t_glyph_positions[0].y_offset / HB_SCALE_FACTOR);
            x_location.x += ((float)t_glyph_positions[0].x_advance / HB_SCALE_FACTOR);
            x_location.y += ((float)t_glyph_positions[0].y_advance / HB_SCALE_FACTOR);
            
            uint16_t t_cluster;
            t_cluster = t_glyph_infos[0].cluster;
	
			uindex_t t_char_start, t_char_end;
			t_char_start = t_glyph_infos[t_start].cluster;
			
			t_char_end = p_char_count;
			if (t_cur_glyph != t_glyph_count)
				t_char_end = t_glyph_infos[t_cur_glyph].cluster;

			MCTextLayoutSpan t_span;
			t_span.chars = p_chars + t_char_start;
			t_span.char_count = t_char_end - t_char_start;
			t_span.clusters = &t_cluster;
			t_span.glyphs = &t_glyph;
			t_span.glyph_count = 1;
			t_span.font = t_ft_face;
			
			t_success = p_callback(p_context, &t_span);
			
			t_start = t_cur_glyph;
		}
		
		// Deal with run of unsupported clusters for this font.
		while (t_cur_glyph < t_glyph_count && !cluster_is_supported(t_glyph_infos, t_cur_glyph, t_glyph_count, t_cluster_end))
			t_cur_glyph = t_cluster_end;
		
        // For the run of unsupported clusters, shape using a fallback font.
		if (t_start != t_cur_glyph)
		{
            // Reshape from the beginning of the unsupported cluster run
            // to the beginning of the cluster containing t_cur_glyph.
            // At this point, t_cur_glyph might be equal to glyph_count,
            // in which case compute the reshape char count using the old char
            // count.
			uindex_t t_char_start, t_char_end;
			t_char_start = t_glyph_infos[t_start].cluster;
			
			t_char_end = p_char_count;
			if (t_cur_glyph != t_glyph_count)
				t_char_end = t_glyph_infos[t_cur_glyph].cluster;

			t_success = shape(p_chars + t_char_start, t_char_end - t_char_start, p_font, x_location, true, p_callback, p_context);
		}
	}
	
	if (t_hb_buffer != nil)
		hb_buffer_destroy(t_hb_buffer);
	
	if (t_ft_face != nil)
		FT_Done_Face(t_ft_face);
	if (t_sk_typeface != nil)
		t_sk_typeface->unref();
	
	return t_success;
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{
	MCGPoint t_loc;
	t_loc = MCGPointMake(0.0, 0.0);
	return shape(p_chars, p_char_count, p_font, t_loc, false, p_callback, p_context);
}

////////////////////////////////////////////////////////////////////////////////
