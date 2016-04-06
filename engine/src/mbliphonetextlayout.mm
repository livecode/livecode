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

#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

struct MCTextLayoutFont
{
	MCTextLayoutFont *next;
	NSString *name;
	CGFloat size;
	CTFontRef handle;
};

static MCTextLayoutFont *s_fonts = nil;

bool MCTextLayoutInitialize(void)
{
	return true;
}

void MCTextLayoutFinalize(void)
{
	while(s_fonts != nil)
	{
		MCTextLayoutFont *t_font;
		t_font = s_fonts;
		s_fonts = s_fonts -> next;
		
		[t_font -> name release];
		CFRelease(t_font -> handle);
		delete t_font;
	}
}

static CTFontRef ctfont_from_fontstruct(MCFontStruct *p_font_struct)
{
    // MM-2014-06-04: [[ CoreText ]] Fonts are now already coretext font refs. No extra work needed here anymore.
    return (CTFontRef)CFRetain((CTFontRef) p_font_struct -> fid);
}

static CTFontRef ctfont_from_ctfont(CTFontRef p_ct_font)
{
	NSString *t_ct_font_name;
	CGFloat t_ct_font_size;
	t_ct_font_name = (NSString *)CTFontCopyPostScriptName(p_ct_font);
	t_ct_font_size = CTFontGetSize(p_ct_font);
	
	CTFontRef t_handle;
	t_handle = nil;
	for(MCTextLayoutFont *t_font = s_fonts; t_font != nil && t_handle == nil; t_font = t_font -> next)
	{
		NSString *t_font_name;
		float t_font_size;
		t_font_name = (NSString *)CTFontCopyPostScriptName(t_font -> handle);
		t_font_size = CTFontGetSize(t_font -> handle);
		if ([t_font_name isEqualToString: t_ct_font_name] && t_font_size == t_ct_font_size)
			t_handle = t_font -> handle;
		[t_font_name release];
	}
	
	if (t_handle == nil)
	{
		MCTextLayoutFont *t_font;
		t_font = new MCTextLayoutFont;
		t_font -> next = s_fonts;
		t_font -> name = [t_ct_font_name retain];
		t_font -> size = t_ct_font_size;
		t_font -> handle = p_ct_font;
		CFRetain(p_ct_font);
		s_fonts = t_font;
		
		t_handle = p_ct_font;
	}
	
	[t_ct_font_name release];
	
	return t_handle;
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font_struct, MCTextLayoutCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;
	
	// Make the font
	
	CTFontRef t_font;
	t_font = nil;
	if (t_success)
	{
		t_font = ctfont_from_fontstruct(p_font_struct);
		if (t_font == nil)
			t_success = false;
	}
	
	// Build the typesetter
	CTTypesetterRef t_typesetter;
	t_typesetter = nil;
	if (t_success)
	{
		NSString *t_string;
		t_string = [[NSString alloc] initWithCharacters: (unichar_t *)p_chars length: p_char_count];
		
		NSDictionary *t_attributes;
		t_attributes = [[NSDictionary alloc] initWithObjectsAndKeys: (id)t_font, kCTFontAttributeName, nil];
		
		NSAttributedString *t_text;
		t_text = [[NSAttributedString alloc] initWithString: t_string attributes: t_attributes];
		[t_string release];
		[t_attributes release];
	
		t_typesetter = CTTypesetterCreateWithAttributedString((CFAttributedStringRef)t_text);
		[t_text release];
		if (t_typesetter == nil)
			t_success = false;
	}
	
	CTLineRef t_line;
	t_line = nil;
	if (t_success)
	{
		t_line = CTTypesetterCreateLine(t_typesetter, CFRangeMake(0, 0));
		if (t_line == nil)
			t_success = false;
	}
	
	if (t_success)
	{
		NSArray *t_runs;
		t_runs = (NSArray *)CTLineGetGlyphRuns(t_line);
		for(id t_run_id in t_runs)
		{
			CTRunRef t_run;
			t_run = (CTRunRef)t_run_id;
			
			uint32_t t_glyph_count;
			t_glyph_count = CTRunGetGlyphCount(t_run);
			
			CFRange t_char_range;
			t_char_range = CTRunGetStringRange(t_run);
			
			CGGlyph *t_glyphs;
			CGPoint *t_positions;
			CFIndex *t_indices;
			t_glyphs = new CGGlyph[t_glyph_count];
			t_positions = new CGPoint[t_glyph_count];
			t_indices = new CFIndex[t_glyph_count];
			CTRunGetGlyphs(t_run, CFRangeMake(0, 0), t_glyphs);
			CTRunGetPositions(t_run, CFRangeMake(0, 0), t_positions);
			CTRunGetStringIndices(t_run, CFRangeMake(0, 0), t_indices);
			
			MCTextLayoutGlyph *t_layout_glyphs;
			uint16_t *t_clusters;
			t_layout_glyphs = new MCTextLayoutGlyph[t_glyph_count];
			t_clusters = new uint16_t[t_char_range . length];
			for(uint32_t i = 0; i < t_glyph_count; i++)
			{
				t_layout_glyphs[i] . index = t_glyphs[i];
				t_layout_glyphs[i] . x = t_positions[i] . x;
				t_layout_glyphs[i] . y = t_positions[i] . y;
			}
			
			// Compute the clusters.
			for(uint32_t i = 0; i < t_char_range . length; i++)
				t_clusters[i] = 65535;
			for(uint32_t i = 0; i < t_glyph_count; i++)
				t_clusters[t_indices[i] - t_char_range . location] = MCMin((uint32_t)t_clusters[t_indices[i]], i);
			
			for(uint32_t i = 1; i < t_char_range . length; i++)
			{
				// If a cluster has 0xffff as its value it means it was never set and must be
				// part of a surrogate, so we set it to the previous char.
				if (t_clusters[i] == 0xffff)
					t_clusters[i] = t_clusters[i - 1];
			}
			
			MCTextLayoutSpan t_span;
			t_span . chars = p_chars + t_char_range . location;
			t_span . clusters = t_clusters;
			t_span . char_count = t_char_range . length;
			t_span . glyphs = t_layout_glyphs;
			t_span . glyph_count = t_glyph_count;
			t_span . font = (void *)ctfont_from_ctfont((CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(t_run), kCTFontAttributeName));
			p_callback(p_context, &t_span);

			delete[] t_indices;
			delete[] t_positions;
			delete[] t_glyphs;
			delete[] t_layout_glyphs;
			delete[] t_clusters;
		}
	}
	
	if (t_line != nil)
		CFRelease(t_line);
	if (t_typesetter != nil)
		CFRelease(t_typesetter);
	if (t_font != nil)
		CFRelease(t_font);
	
	return true;
}


