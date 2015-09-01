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

#include "osxprefix.h"

#include "textlayout.h"

extern ATSUFontID coretext_font_to_atsufontid(void *p_font);

bool MCTextLayoutInitialize(void)
{
	return true;
}

void MCTextLayoutFinalize(void)
{
}

static bool MCTextLayoutDraw(ATSUTextLayout p_layout, const unichar_t *p_chars, uint32_t p_char_count, int32_t p_x, MCTextLayoutCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;
	
	ATSLayoutRecord *t_records;
	ItemCount t_record_count;
	t_records = nil;
	t_record_count= 0;
	if (t_success)
		if (ATSUDirectGetLayoutDataArrayPtrFromTextLayout(p_layout, 0, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void **)&t_records, &t_record_count) != noErr)
			t_success = false;
	
	Fixed *t_baselines;
	ItemCount t_baseline_count;
	t_baselines = nil;
	t_baseline_count = 0;
	if (t_success)
		if (ATSUDirectGetLayoutDataArrayPtrFromTextLayout(p_layout, 0, kATSUDirectDataBaselineDeltaFixedArray, (void **)&t_baselines, &t_baseline_count) != noErr)
			t_success = false;
	
	MCTextLayoutGlyph *t_glyphs;
	t_glyphs = nil;
	if (t_success && t_record_count > 1)
		t_success = MCMemoryNewArray(t_record_count - 1, t_glyphs);
	
	uint16_t *t_clusters;
	t_clusters = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_char_count, t_clusters);
	
	if (t_success)
	{
		for(uint32_t i = 0; i < t_record_count - 1; i++)
		{
			t_glyphs[i] . index = t_records[i] . glyphID;
			t_glyphs[i] . x = p_x + t_records[i] . realPos / 65536.0;
			t_glyphs[i] . y = 0.0;
		}
		
		// Loop through the glyphs, breaking them into runs of similar font
		uint32_t t_frontier;
		t_frontier = 0;
		while(t_success && t_frontier < t_record_count - 1)
		{
			// Find a run of similary font'ed glyphs
			uint32_t t_glyph_index;
			t_glyph_index = t_frontier;
			
			uint32_t t_min_offset, t_max_offset;
			t_min_offset = MAXUINT4;
			t_max_offset = 0;
			
			ATSUFontID t_font_id;
			t_font_id = 0;
			while(t_glyph_index < t_record_count - 1)
			{
				ATSUStyle t_style;
				ATSUGetRunStyle(p_layout, t_records[t_glyph_index] . originalOffset / 2, &t_style, nil, nil);
				
				ATSUFontID t_new_font_id;
				ATSUGetAttribute(t_style, kATSUFontTag, sizeof(ATSUFontID), &t_new_font_id, nil);
				
				if (t_font_id == 0)
					t_font_id = t_new_font_id;
				else if (t_font_id != t_new_font_id)
					break;
				
				t_min_offset = MCMin(t_min_offset, t_records[t_glyph_index] . originalOffset / 2);
				t_max_offset = MCMax(t_max_offset, t_records[t_glyph_index] . originalOffset / 2 + (t_records[t_glyph_index] . flags & kATSGlyphInfoByteSizeMask) / 2);
				
				t_glyph_index += 1;
			}
			
			uint32_t t_char_count;
			t_char_count = t_max_offset - t_min_offset;
			
			// Compute the clusters. Each record has an 'originalOffset' field that contains
			// the byte offset of the character that spawned the glyph - we simply reverse this map.
			for(uint32_t i = 0; i < t_char_count; i++)
				t_clusters[i] = 65535;
			for(uint32_t i = t_frontier; i < t_glyph_index; i++)
			{
				uint32_t t_char_offset;
				t_char_offset = (t_records[i] . originalOffset / 2) - t_min_offset;
				
				t_clusters[t_char_offset] = MCMin((uint32_t)t_clusters[t_char_offset], i - t_frontier);
			}
			for(uint32_t i = 1; i < t_char_count; i++)
			{
				// If a cluster has 0xffff as its value it means it was never set and must be
				// part of a surrogate, so we set it to the previous char.
				if (t_clusters[i] == 0xffff)
					t_clusters[i] = t_clusters[i - 1];
				else if (t_records[t_frontier + t_clusters[i]] . glyphID == 0xffff)
					t_clusters[i] = t_clusters[i - 1];
			}
			
			MCTextLayoutSpan t_span;
			t_span . chars = p_chars + t_min_offset;
			t_span . clusters = t_clusters;
			t_span . char_count = t_char_count;
			t_span . glyphs = t_glyphs + t_frontier;
			t_span . glyph_count = t_glyph_index - t_frontier;
			t_span . font = (void *)t_font_id;
			t_success = p_callback(p_context, &t_span);
			
			t_frontier = t_glyph_index;
		}
	}
	
	MCMemoryDeleteArray(t_clusters);
	MCMemoryDeleteArray(t_glyphs);
	
	if (t_baselines != nil)
		ATSUDirectReleaseLayoutDataArrayPtr(nil, kATSUDirectDataBaselineDeltaFixedArray, (void **)&t_baselines);
	
	if (t_records != nil)
		ATSUDirectReleaseLayoutDataArrayPtr(nil, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void **)&t_records);
	
	return true;
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{	
	OSStatus t_err;
	ATSUTextLayout t_layout;
	ATSUStyle t_style;

	ATSUFontID t_font_id;
	Fixed t_font_size;
	Boolean t_font_is_bold;
	Boolean t_font_is_italic;
	Boolean t_font_is_underline;
	Boolean t_font_is_condensed;
	Boolean t_font_is_extended;
	ATSLineLayoutOptions t_layout_options;
	
	ATSUAttributeTag t_tags[] =
	{
		kATSUFontTag,
		kATSUSizeTag
	};
	ByteCount t_sizes[] =
	{
		sizeof(ATSUFontID),
		sizeof(Fixed)
	};
	ATSUAttributeValuePtr t_attrs[] =
	{
		&t_font_id,
		&t_font_size
	};
	
	// MW-2013-11-15: [[ Bug 11444 ]] It seems setting these makes things *less* like QuickDraw!
	/*ATSUAttributeTag t_layout_tags[] =
	{
		kATSULineLayoutOptionsTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(ATSLineLayoutOptions)
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_layout_options
	};*/
	
	UniCharCount t_run = p_char_count;
	Rect t_bounds;
	   
	FMFontStyle t_intrinsic_style;
    
    // MM-2014-06-04: [[ CoreText ]] Fonts are now coretext font refs. Make sure we convert to ATSUFontIDs.
    t_font_id = coretext_font_to_atsufontid(p_font -> fid);
	
	t_font_size = p_font -> size << 16;
	t_err = ATSUCreateStyle(&t_style);
	t_err = ATSUSetAttributes(t_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
	t_err = ATSUCreateTextLayout(&t_layout);
	t_err = ATSUSetTransientFontMatching(t_layout, false);
	
	/*
	t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
	
	t_err = ATSUSetLayoutControls(t_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	*/
	
	ATSUStyle *t_new_styles;
	uint32_t t_new_style_count;
	t_new_styles = nil;
	t_new_style_count = 0;
	
	t_err = ATSUSetTextPointerLocation(t_layout, (const UniChar *)p_chars, 0, p_char_count, p_char_count);
	t_err = ATSUSetRunStyle(t_layout, t_style, 0, p_char_count);
	
	uint32_t t_frontier;
	t_frontier = 0;
	while(t_frontier < p_char_count)
	{
		UniCharArrayOffset t_new_run_start;
		UniCharCount t_new_run_length;
		t_err = ATSUMatchFontsToText(t_layout, t_frontier, p_char_count - t_frontier, &t_font_id, &t_new_run_start, &t_new_run_length);
		if (t_err == noErr)
			break;
		
		MCMemoryResizeArray(t_new_style_count + 1, t_new_styles, t_new_style_count);
		
		ATSUStyle t_new_style;
		t_err = ATSUCreateStyle(&t_new_style);
		t_err = ATSUSetAttributes(t_new_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
		t_err = ATSUSetRunStyle(t_layout, t_new_style, t_new_run_start, t_new_run_length);
		
		t_new_styles[t_new_style_count - 1] = t_new_style;
		
		t_frontier = t_new_run_start + t_new_run_length;
	}
	
	MCTextLayoutDraw(t_layout, p_chars, p_char_count, 0, p_callback, p_context);
	
	t_err = ATSUDisposeTextLayout(t_layout);
	
	for(uint32_t i = 0; i < t_new_style_count; i++)
		ATSUDisposeStyle(t_new_styles[i]);
	MCMemoryDeleteArray(t_new_styles);
	
	t_err = ATSUDisposeStyle(t_style);
	
	return true;
}
