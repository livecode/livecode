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

#include <SkTypeface_FreeType.h>

#include "graphics.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

bool MCTextLayoutInitialize(void)
{
	return true;
}

void MCTextLayoutFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

bool make_textlayout_glyphs(const MCGGlyphInfo *p_glyph_infos, uindex_t p_glyph_count, MCGPoint &x_location, MCTextLayoutGlyph *&r_glyphs)
{
	bool t_success;
	t_success = true;
	
	MCTextLayoutGlyph *t_glyphs;
	t_glyphs = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_glyph_count, t_glyphs);
	
	if (t_success)
	{
		for (uindex_t i = 0; i < p_glyph_count; i++)
		{
			t_glyphs[i].index = p_glyph_infos[i].codepoint;
			t_glyphs[i].x = x_location.x + p_glyph_infos[i].x_offset;
			t_glyphs[i].y = x_location.y + p_glyph_infos[i].y_offset;
			
			x_location.x += p_glyph_infos[i].x_advance;
			x_location.y += p_glyph_infos[i].y_advance;
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

bool make_textlayout_clusters(const MCGGlyphInfo *p_glyph_infos, uindex_t p_glyph_count, uindex_t p_char_count, uint16_t *&r_clusters)
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
		t_current_cluster = p_glyph_infos[0].cluster;
		
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
				if (t_next_glyph < p_glyph_count)
					t_next_cluster = p_glyph_infos[t_next_glyph].cluster;
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

//////////

struct _layout_context_t
{
	MCTextLayoutCallback callback;
	void *context;
	MCGPoint location;
	bool success;
};

static bool _layout_callback(void *p_context, const MCGFont &p_font, const MCGGlyphInfo *p_glyphs, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count)
{
	_layout_context_t *self;
	self = (_layout_context_t*)p_context;

	bool t_success;
	t_success = true;
	bool t_continue;
	t_continue = true;

	MCTextLayoutGlyph *t_glyphs;
	t_glyphs = nil;
	if (t_success)
		t_success = make_textlayout_glyphs(p_glyphs, p_glyph_count, self->location, t_glyphs);
	
	uint16_t *t_clusters;
	t_clusters = nil;
	if (t_success)
		t_success = make_textlayout_clusters(p_glyphs, p_glyph_count, p_char_count, t_clusters);
	
	if (t_success)
	{
		// Android-specific - get freetype font face
		FT_Face t_face;
		t_face = SkTypeface_GetFTFace((SkTypeface*)p_font.fid);

		MCTextLayoutSpan t_span;
		t_span.chars = p_chars;
		t_span.char_count = p_char_count;
		t_span.clusters = t_clusters;
		t_span.glyphs = t_glyphs;
		t_span.glyph_count = p_glyph_count;
		t_span.font = t_face;
		
		t_continue = self->callback(self->context, &t_span);

		FT_Done_Face(t_face);
	}
	
	if (t_glyphs != nil)
		MCMemoryDeleteArray(t_glyphs);
	if (t_clusters != nil)
		MCMemoryDeleteArray(t_clusters);
	
	self->success = t_success;

	return t_success && t_continue;
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{
	_layout_context_t t_context;
	t_context.callback = p_callback;
	t_context.context = p_context;
	t_context.location = MCGPointMake(0.0, 0.0);
	t_context.success = true;

	bool t_success;
	t_success = MCGFontLayoutText(MCFontStructToMCGFont(p_font), p_chars, p_char_count, false, _layout_callback, &t_context);

	return t_success && t_context.success;
}

////////////////////////////////////////////////////////////////////////////////
