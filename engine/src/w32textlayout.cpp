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

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    This file contains the implementation of MCTextLayout for Windows. This
//    is intended to be an emulation of GDI TextOut calls for arbitrary Unicode
//    strings that produces runs of glyphs in specific fonts, rather than
//    graphics output.
//
//    The implementation is quite complex as it has to take into account font
//    fallback as well as linking. The general process the implementation
//    follows is thus:
//        1) Use ScriptItemize to generate individual runs of text using
//           consistent 'script engine' and direction.
//        2) Pass each item through ScriptStringAnalyse into a metafile and
//           parse the metafile to work out what fonts to use for ranges of
//           characters (there's no other way to get at this info as fallback
//           is hard-coded into the ScriptString APIs!).
//        3) If a run found in this manner uses a font which is 'linked' to
//           others, break it into clusters using ScriptBreak and attempt to
//           shape with each linked font in order. This results in one or
//           more additional runs.
//        4) For each run with specific font, use ScriptShape to generate glyph
//           indices and then pass these through to ScriptPlace.
//        5) Build a 'span' for each shaped and placed run, ensuring the glyph
//           order is visual rather than logical.
//        6) Use ScriptLayout to re-order all the runs.
//
//    These steps correspond loosely to the functions:
//      1) (first half of) MCTextLayout
//      2) MCTextLayoutStyleItem
//      3) MCTextLayoutLinkItem
//      4) MCTextLayoutShapeItem 
//      5) MCTextLayoutPlaceItem
//      6) (last half of) MCTextLayout
//
//    Here, the use of a metafile at step (2) allows direct use of the hard-coded
//    font fallback mechanism used by Uniscribe.
//
//    The font-linking process mentioned in (3) uses the keys that are set in
//    the registry to effectively build 'super fonts' incorporating glyphs from
//    many fonts into one.
//
////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"
#include <usp10.h>

#include "globdefs.h"
#include "textlayout.h"

////////////////////////////////////////////////////////////////////////////////

// The linked font structures are built in MCTextLayoutInitialize and taken
// from the appropriate registry key.

struct MCTextLayoutLinkedFontEntry
{
	char *face;
	uint32_t u, v;
};

struct MCTextLayoutLinkedFont
{
	MCTextLayoutLinkedFont *next;
	char *name;
	MCTextLayoutLinkedFontEntry *entries;
	uint32_t entry_count;
};

// Allocated font structures are cached in between calls to Initialize/
// Finalize to speed up processing and also to allow sequential spans of
// equal font to be elided.

struct MCTextLayoutFont
{
	MCTextLayoutFont *next;
	MCTextLayoutLinkedFont *linking;
	LOGFONTA info;
	HFONT handle;
	SCRIPT_CACHE cache;
	uint16_t default_glyph;
};

// The text layout runs are generated after glyph placement and represent
// a single contiguous sequence of glyphs/chars in a single font.

struct MCTextLayoutRun
{
	unichar_t *chars;
	uint16_t *clusters;
	uint32_t char_count;

	uint16_t *glyphs;
	int *advances;
	GOFFSET *goffsets;
	uint32_t glyph_count;

	uint8_t embedding_level;

	MCTextLayoutFont *font;
};

// The layout state contains various info that is used across all the layout
// methods.

struct MCTextLayoutState
{
	HDC dc;
	MCTextLayoutFont *primary_font;
	MCTextLayoutRun *runs;
	uint32_t run_count;
	uint32_t run_limit;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> inline void MCSwap(T& a, T& b)
{
	T t;
	t = a;
	a = b;
	b = t;
}

////////////////////////////////////////////////////////////////////////////////

static MCTextLayoutLinkedFont *s_linked_fonts = nil;
static MCTextLayoutFont *s_fonts = nil;

static void MCTextLayoutFontDestroy(MCTextLayoutFont *self)
{
	if (self == nil)
		return;
	
	DeleteObject(self -> handle);
	ScriptFreeCache(&self -> cache);
	MCMemoryDelete(self);
}

static MCTextLayoutFont *MCTextLayoutFontFind(const LOGFONTA& p_info)
{
	for(MCTextLayoutFont *t_font = s_fonts; t_font != nil; t_font = t_font -> next)
		if (MCMemoryEqual(&p_info, &t_font -> info, sizeof(LOGFONTA)))
			return t_font;

	return nil;
}

static bool MCTextLayoutFontFromHFONT(void *p_font, MCTextLayoutFont*& r_font)
{
	bool t_success;
	t_success = true;

	// First fetch the HFONT's LOGFONT structure
	LOGFONTA t_logfont;
	if (t_success)
		if (!GetObjectA(p_font, sizeof(LOGFONTA), &t_logfont))
			t_success = false;

	if (t_success)
		t_logfont . lfHeight = -256;

	// Now use this to search for an existing layout font
	MCTextLayoutFont *self;
	self = nil;
	if (t_success)
	{
		self = MCTextLayoutFontFind(t_logfont);
		if (self != nil)
		{
			r_font = self;
			return true;
		}
	}

	// Otherwise we must go ahead and create a new font
	if (t_success)
		t_success = MCMemoryNew(self);

	if (t_success)
	{
		self -> handle = CreateFontIndirectA(&t_logfont);
		if (self -> handle == nil)
			t_success = false;
	}

	if (t_success)
	{
		MCListPushFront(s_fonts, self);
		self -> info = t_logfont;

		// Now see if the font is a linked font
		for(MCTextLayoutLinkedFont *t_links = s_linked_fonts; t_links != nil; t_links = t_links -> next)
			if (MCCStringEqualCaseless(t_links -> name, self -> info . lfFaceName))
			{
				self -> linking = t_links;
				break;
			}

		r_font = self;
	}
	else
		MCTextLayoutFontDestroy(self);
	
	return t_success;
}

static bool MCTextLayoutFontWithLink(MCTextLayoutFont *p_base, MCTextLayoutLinkedFontEntry *p_link, MCTextLayoutFont*& r_font)
{
	LOGFONTA t_logfont;
	t_logfont = p_base -> info;
	MCMemoryCopy(&t_logfont . lfFaceName, p_link -> face, MCMax(MCCStringLength(p_link -> face) + 1, sizeof(t_logfont . lfFaceName)));

	MCTextLayoutFont *t_font;
	t_font = MCTextLayoutFontFind(t_logfont);
	if (t_font != nil)
	{
		r_font = t_font;
		return true;
	}
	
	HFONT t_hfont;
	t_hfont = CreateFontIndirectA(&t_logfont);
	if (t_hfont == nil)
		return false;

	bool t_success;
	t_success = MCTextLayoutFontFromHFONT(t_hfont, t_font);
	
	DeleteObject(t_hfont);

	r_font = t_font;

	return t_success;
}

static void MCTextLayoutLinkedFontDestroy(MCTextLayoutLinkedFont *self)
{
	for(uint32_t i = 0; i < self -> entry_count; i++)
		MCCStringFree(self -> entries[i] . face);
	MCMemoryDeleteArray(self -> entries);
	MCCStringFree(self ->  name);
	MCMemoryDelete(self);
}

//////////

typedef bool (*MCRegistryListValuesCallback)(void *context, const char *name, DWORD p_type, void *value, uint32_t value_length);

static bool MCRegistryListValues(HKEY p_root, const char *p_key, MCRegistryListValuesCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	// Attempt to open the given key.
	HKEY t_handle;
	t_handle = nil;
	if (t_success)
		if (RegOpenKeyExA(p_root, p_key, 0, KEY_QUERY_VALUE, &t_handle) != ERROR_SUCCESS)
			t_success = false;

	// Next determine the maximum length of the value names.
	DWORD t_max_name_length;
	if (t_success)
		if (RegQueryInfoKeyA(t_handle, nil, nil, nil, nil, nil, nil, nil, &t_max_name_length, nil, nil, nil) != ERROR_SUCCESS)
			t_success = false;

	// Allocate a buffer big enough for the name
	char *t_name_buffer;
	t_name_buffer = nil;
	if (t_success)
		t_success = MCMemoryNewArray(t_max_name_length + 1, t_name_buffer);

	if (t_success)
	{
		DWORD t_index;
		t_index = 0;
		while(t_success)
		{
			DWORD t_name_length, t_value_length;
			t_name_length = t_max_name_length + 1;
			t_value_length = 0;

			LSTATUS t_result;
			if (t_success)
			{
				t_result = RegEnumValueA(t_handle, t_index, t_name_buffer, &t_name_length, nil, nil, nil, &t_value_length);
				if (t_result == ERROR_NO_MORE_ITEMS)
					break;
				if (t_result != ERROR_SUCCESS)
					t_success = false;
			}

			void *t_value_buffer;
			t_value_buffer = nil;
			if (t_success)
				t_success = MCMemoryAllocate(t_value_length, t_value_buffer);

			DWORD t_type;
			if (t_success)
			{
				t_name_length = t_max_name_length + 1;
				if (RegEnumValueA(t_handle, t_index, t_name_buffer, &t_name_length, nil, &t_type, (LPBYTE)t_value_buffer, &t_value_length) != ERROR_SUCCESS)
					t_success = false;
			}

			if (t_success && p_callback != nil)
				p_callback(p_context, t_name_buffer, t_type, t_value_buffer, t_value_length);

			MCMemoryDeallocate(t_value_buffer);

			t_index++;
		}
	}

	MCMemoryDeleteArray(t_name_buffer);

	if (t_handle != nil)
		RegCloseKey(t_handle);

	return t_success;
}

static bool MCTextLayoutFontLinkCallback(void *p_context, const char *p_key, DWORD p_type, void *p_value, uint32_t p_value_length)
{
	bool t_success;
	t_success = true;

	// Ignore the empty key
	if (MCCStringEqual(p_key, ""))
		return true;

	// Ignore a non REG_MULTI_SZ type
	if (p_type != REG_MULTI_SZ)
		return true;

	// Create a new linked font structure
	MCTextLayoutLinkedFont *t_font;
	t_font = nil;
	if (t_success)
		t_success = MCMemoryNew(t_font);

	if (t_success)
		t_success = MCCStringClone(p_key, t_font -> name);

	// Attempt to parse the font link string - it should be of type
	// REG_MULTI_SZ and be a list of entries of the form:
	//   file, face, [ scale, scale ]
	if (t_success)
	{
		char *t_ptr;
		uint32_t t_len;
		t_ptr = (char *)p_value;
		t_len = p_value_length;
		while(t_success && t_len > 0)
		{
			char *t_end;
			for(t_end = t_ptr; *t_end != '\0' && t_len > 0; t_end += 1, t_len -= 1)
				;

			if (t_end - t_ptr > 0)
			{
				char **t_items;
				uint32_t t_item_count;
				t_items = nil;
				t_item_count = 0;
				if (t_success)
					t_success = MCCStringSplit(t_ptr, ',', t_items, t_item_count);

				if (t_item_count >= 2)
				{
					if (t_success)
						t_success = MCMemoryResizeArray(t_font -> entry_count + 1, t_font -> entries, t_font -> entry_count);
				
					if (t_success)
						t_success = MCCStringClone(t_items[1], t_font -> entries[t_font -> entry_count - 1] . face);
				}

				for(uint32_t i = 0; i < t_item_count; i++)
					MCCStringFree(t_items[i]);
				MCMemoryDeleteArray(t_items);
			}

			t_ptr = t_end + 1;
			t_len -= 1;
		}
	}

	if (t_success)
		MCListPushFront(s_linked_fonts, t_font);
	else
		MCTextLayoutLinkedFontDestroy(t_font);

	return t_success;
}

bool MCTextLayoutInitialize(void)
{
	MCRegistryListValues(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink", MCTextLayoutFontLinkCallback, nil);
	return true;
}

void MCTextLayoutFinalize(void)
{
	while(s_linked_fonts != nil)
		MCTextLayoutLinkedFontDestroy(MCListPopFront(s_linked_fonts));

	while(s_fonts != nil)
		MCTextLayoutFontDestroy(MCListPopFront(s_fonts));
}

////////////////////////////////////////////////////////////////////////////////

static bool MCTextLayoutPlaceItem(MCTextLayoutState& self, SCRIPT_ANALYSIS p_analysis, const unichar_t *p_chars, uint32_t p_char_count, const uint16_t *p_clusters, const uint16_t *p_glyphs, const SCRIPT_VISATTR *p_attrs, uint32_t p_glyph_count, MCTextLayoutFont *p_font)
{
	bool t_success;
	t_success = true;

	MCTextLayoutRun *t_run;
	if (self . run_count != 0)
		t_run = &self . runs[self . run_count - 1];
	else
		t_run = nil;

	// We need to append a new run if the font or embedding level has changed.
	if (t_run == nil ||
		t_run -> font != p_font ||
		t_run -> embedding_level != p_analysis . s . uBidiLevel)
	{
		if (self . run_count + 1 > self . run_limit)
			t_success = MCMemoryResizeArray(self . run_limit + 8, self . runs, self . run_limit);

		if (t_success)
		{
			t_run = &self . runs[self . run_count];
			t_run -> font = p_font;
			t_run -> embedding_level = p_analysis . s . uBidiLevel;
		
			self . run_count += 1;
		}
	}

	// Allocate the cluster, glyph, goffset and advance arrays that we need.
	if (t_success)
		t_success =
			MCMemoryReallocate(t_run -> chars, (t_run -> char_count + p_char_count) * sizeof(unichar_t), t_run -> chars) &&
			MCMemoryReallocate(t_run -> clusters, (t_run -> char_count + p_char_count) * sizeof(uint16_t), t_run -> clusters) &&
			MCMemoryReallocate(t_run -> glyphs, (t_run -> glyph_count + p_glyph_count) * sizeof(uint16_t), t_run -> glyphs) &&
			MCMemoryReallocate(t_run -> goffsets, (t_run -> glyph_count + p_glyph_count) * sizeof(GOFFSET), t_run -> goffsets) &&
			MCMemoryReallocate(t_run -> advances, (t_run -> glyph_count + p_glyph_count) * sizeof(int), t_run -> advances);

	// Run the script placement method
	if (t_success)
	{
		SelectObject(self . dc, p_font -> handle);
		if (ScriptPlace(
				self . dc,
				&p_font -> cache,
				p_glyphs,
				p_glyph_count,
				p_attrs,
				&p_analysis,
				t_run -> advances + t_run -> glyph_count,
				t_run -> goffsets + t_run -> glyph_count,
				nil) != S_OK)
			t_success = false;
	}

	// Fill in the span structure
	if (t_success)
	{
		MCMemoryCopy(t_run -> chars + t_run -> char_count, p_chars, sizeof(unichar_t) * p_char_count);

		if ((p_analysis . s . uBidiLevel & 1) == 0)
		{
			MCMemoryCopy(t_run -> glyphs + t_run -> glyph_count, p_glyphs, sizeof(uint16_t) * p_glyph_count);
			MCMemoryCopy(t_run -> clusters + t_run -> char_count, p_clusters, sizeof(uint16_t) * p_char_count);
		}
		else
		{
			// In this case things are going Right to Left, so remap into visual order.

			for(uint32_t i = 0; i < p_char_count; i++)
				t_run -> clusters[i + t_run -> char_count] = p_glyph_count - p_clusters[i] - 1;

			for(uint32_t i = 0; i < p_glyph_count; i++)
				t_run -> glyphs[i + t_run -> glyph_count] = p_glyphs[p_glyph_count - i - 1];

			for(uint32_t i = 0; i < p_glyph_count / 2; i++)
			{
				MCSwap(t_run -> advances[i + t_run -> glyph_count], t_run -> advances[t_run -> glyph_count + (p_glyph_count - i - 1)]);
				MCSwap(t_run -> goffsets[i + t_run -> glyph_count], t_run -> goffsets[t_run -> glyph_count + (p_glyph_count - i - 1)]);
			}
		}

		// As we might have elided a span, we need to shift all the cluster offsets up by the current
		// glyph count.
		for(uint32_t i = 0; i < p_char_count; i++)
			t_run -> clusters[i + t_run -> char_count] += t_run -> glyph_count;

		t_run -> char_count += p_char_count;
		t_run -> glyph_count += p_glyph_count;
	}

	return t_success;
}

static bool MCTextLayoutShapeItem(MCTextLayoutState& self, SCRIPT_ANALYSIS p_analysis, const unichar_t *p_chars, uint32_t p_char_count, MCTextLayoutFont *p_font, bool* p_try)
{
	bool t_success;
	t_success = true;

	// We do everything in logical order, doing any swizzling of glyphs at the
	// end.
	p_analysis . fLogicalOrder = true;

	// We start off by using the given font, however if this causes an issue
	// with shaping we fall back to the primary font.
	MCTextLayoutFont *t_layout_font;
	t_layout_font = p_font;

	// If the initial font is nil, then we use the primary font and undefined
	// script.
	if (t_layout_font == nil)
	{
		p_analysis . eScript = SCRIPT_UNDEFINED;
		t_layout_font = self . primary_font;
	}

	// Allocate a big enough cluster array for the item
	WORD *t_clusters;
	uint32_t t_cluster_limit;
	t_clusters = nil;
	t_cluster_limit = 0;
	if (t_success)
		t_success = MCMemoryResizeArray(p_char_count, t_clusters, t_cluster_limit);

	// Loop until we succeed in generating some glyphs - note that this loop
	// is a little hairy. It loops until we don't get E_OUTOFMEMORY, but
	// additionally, may loop with USP_E_SCRIPT_NOT_IN_FONT, in which case
	// t_script_font will have changed, as might p_analysis.
	WORD *t_glyphs;
	SCRIPT_VISATTR *t_attrs;
	uint32_t t_glyph_limit, t_attr_limit, t_glyph_count;
	t_glyphs = nil;
	t_attrs = nil;
	t_glyph_limit = 0;
	t_attr_limit = 0;
	t_glyph_count = 0;
	while(t_success)
	{
		// Make sure we have enough room in the glyph buffer
		uint32_t t_new_glyph_limit;
		t_new_glyph_limit = (t_glyph_limit == 0 ? p_char_count + p_char_count / 2 + 16 : t_glyph_limit + p_char_count / 2);
		t_success = 
			MCMemoryResizeArray(t_new_glyph_limit, t_glyphs, t_glyph_limit) &&
			MCMemoryResizeArray(t_new_glyph_limit, t_attrs, t_attr_limit);

		// Now try to shape
		HRESULT t_result;
		if (t_success)
		{
			SelectObject(self . dc, t_layout_font -> handle);
			t_result = ScriptShape(self . dc, &t_layout_font -> cache, p_chars, p_char_count, t_glyph_limit, &p_analysis, t_glyphs, t_clusters, t_attrs, (int *)&t_glyph_count);
			if (t_result != S_OK && t_result != E_OUTOFMEMORY && t_result != USP_E_SCRIPT_NOT_IN_FONT)
				t_success = false;
		}

		// If the result was 'script not in font' then reprocess with the undefined
		// script with the primary font.
		if (t_success && t_result == USP_E_SCRIPT_NOT_IN_FONT)
		{
			// If we are just testing for shaping success, break on no font.
			if (p_try != nil)
				break;

			t_layout_font = self . primary_font;
			p_analysis . eScript = SCRIPT_UNDEFINED;
			continue;
		}

		// If the result is S_OK, we can break
		if (t_success && t_result == S_OK)
			break;
	}

	// Assuming nothing failed, we now have a sequence of glyphs to place (only
	// if we aren't just testing for shaping success).
	if (t_success && p_try == nil)
		t_success = MCTextLayoutPlaceItem(self, p_analysis, p_chars, p_char_count, t_clusters, t_glyphs, t_attrs, t_glyph_count, t_layout_font);
	
	// If we are testing for shaping success then scan the glyph buffer for
	// undefined glyphs.
	if (t_success && p_try != nil)
	{
		uint32_t t_undefined;
		t_undefined = 0;
		for(uint32_t i = 0; i < t_glyph_count; i++)
			if (t_glyphs[i] == t_layout_font -> default_glyph)
				t_undefined += 1;

		// We use the following heuristic - if the first glyph is undefined
		// then shaping failed, otherwise shaping only fails if *all* the
		// glyphs are undefined.
		*p_try = t_glyph_count != 0 && t_undefined != t_glyph_count && t_glyphs[0] != t_layout_font -> default_glyph;
	}

	// Free up the arrays we allocated - placing causes the contents to be
	// copied/processed into separate run buffers.
	MCMemoryDeleteArray(t_clusters);
	MCMemoryDeleteArray(t_glyphs);
	MCMemoryDeleteArray(t_attrs);

	return t_success;
}

static bool MCTextLayoutLinkItem(MCTextLayoutState& self, SCRIPT_ANALYSIS p_analysis, const unichar_t *p_chars, uint32_t p_char_count, MCTextLayoutFont *p_font)
{
	bool t_success;
	t_success = true;

	// If the font is not linked, we can move directly to shaping
	if (p_font -> linking == nil)
		return MCTextLayoutShapeItem(self, p_analysis, p_chars, p_char_count, p_font, nil);

	// Otherwise we need to split up the run further into font ranges resulting
	// from font linking. Unfortunately, this is more complex than just looking
	// up each UTF-16 codepoint in the fonts support since we need to handle
	// combining cases and surrogates.

	// Make an array to hold cached text layout fonts, we get these as needed.
	MCTextLayoutFont **t_fonts;
	t_fonts = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_font -> linking -> entry_count + 1, t_fonts);

	// The first entry in the array is always the font we start with.
	if (t_success)
		t_fonts[0] = p_font;

	// We shape cluster by cluster, so determine the cluster boundaries using
	// ScriptBreak.
	SCRIPT_LOGATTR *t_breaks;
	t_breaks = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_char_count, t_breaks);

	if (t_success)
		if (ScriptBreak(p_chars, p_char_count, &p_analysis, t_breaks) != S_OK)
			t_success = false;

	// Now loop cluster by cluster, determining the font of each one. Note that
	// we can't use the results of shaping individual clusters to build up the
	// glyphs of the item since adjacent clusters may interact. For example, in
	// Sylfaen 'f' and 'i' and elide to form a proper 'fi' ligature.
	if (t_success)
	{
		MCTextLayoutFont *t_span_font;
		uint32_t t_span_start, t_span_finish;
		t_span_start = t_span_finish = 0;
		t_span_font = nil;
		while(t_success && t_span_finish < p_char_count)
		{
			// Compute bounds of next cluster
			uint32_t t_cluster_start, t_cluster_finish;
			t_cluster_start = t_span_finish;
			t_cluster_finish = t_cluster_start + 1;
			while(t_cluster_finish < p_char_count && !t_breaks[t_cluster_finish] . fCharStop)
				t_cluster_finish++;

			// Now try to shape the cluster in each font successively.
			MCTextLayoutFont *t_cluster_font;
			t_cluster_font = nil;
			for(uint32_t i = 0; i < p_font -> linking -> entry_count + 1 && t_success; i++)
			{
				// Get the font, if it doesn't already exist
				if (t_fonts[i] == nil)
					t_success = MCTextLayoutFontWithLink(p_font, &p_font -> linking -> entries[i - 1], t_fonts[i]);

				// Now try to shape the cluster with it
				bool t_shaped;
				if (t_success)
					t_success = MCTextLayoutShapeItem(self, p_analysis, p_chars + t_cluster_start, t_cluster_finish - t_cluster_start, t_fonts[i], &t_shaped);

				// If the shaping was successful we are done
				if (t_success && t_shaped)
				{
					t_cluster_font = t_fonts[i];
					break;
				}
			}

			// If the cluster's font is not the same as the span's font then we
			// have a run to shape.
			if (t_success && t_cluster_font != t_span_font)
			{
				if (t_span_start != t_span_finish)
					t_success = MCTextLayoutShapeItem(self, p_analysis, p_chars + t_span_start, t_span_finish - t_span_start, t_span_font, nil);

				// The next span starts where the previous one finishes and has
				// the font of the cluster we just shaped.
				if (t_success)
				{
					t_span_start = t_span_finish;
					t_span_font = t_cluster_font;
				}
			}
			
			// End of span is end of cluster
			t_span_finish = t_cluster_finish;
		}

		// Shape the trailing span
		if (t_success)
			t_success = MCTextLayoutShapeItem(self, p_analysis, p_chars + t_span_start, t_span_finish - t_span_start, t_span_font, nil);
	}

	MCMemoryDeleteArray(t_breaks);
	MCMemoryDeleteArray(t_fonts);

	return t_success;
}

//////////

struct MCTextLayoutStyleItemContext
{
	MCTextLayoutState *state;

	const unichar_t *chars;
	uint32_t char_count;
	SCRIPT_ANALYSIS analysis;

	MCTextLayoutFont *primary_font;
	MCTextLayoutFont *current_font;

	// The last item processed - held in pending until we know it isn't a 
	// 'phantom' run.
	int32_t pending_x;
	uint32_t pending_char_count;
	unichar_t *pending_chars;
	MCTextLayoutFont *pending_font;
};

static int CALLBACK MCTextLayoutStyleItemCallback(HDC p_dc, HANDLETABLE *p_handles, const ENHMETARECORD *p_record, int p_object_count, LPARAM p_context)
{
	// This method assumes that the order of the TextOut records is in logical
	// order - i.e. no reversing for right-to-left text. This should be a valid
	// assumption to make since each SCRIPT_ITEM is rendered individually and
	// each such item is either all LTR or all RTL.
	
	// Additionally, it assumes that all printable chars (i.e. non-control) in
	// the analysed string will be part of some ExtTextOut record, even if the
	// suggested font (with linking) doesn't contain the characters.

	// Finally, it assumes that errant records which should not be rendered cause
	// a reference point advance of 0.

	MCTextLayoutStyleItemContext *context;
	context = (MCTextLayoutStyleItemContext *)p_context;

	bool t_success;
	t_success = true;

	bool t_flush;
	t_flush = false;

	int32_t t_new_pending_x;
	uint32_t t_new_pending_char_count;
	unichar_t *t_new_pending_chars;
	MCTextLayoutFont *t_new_pending_font;
	t_new_pending_x = 0;
	t_new_pending_char_count = 0;
	t_new_pending_chars = nil;
	t_new_pending_font = nil;

	switch(p_record -> iType)
	{
	case EMR_EXTCREATEFONTINDIRECTW:
		{
			EMREXTCREATEFONTINDIRECTW *t_record;
			t_record = (EMREXTCREATEFONTINDIRECTW *)p_record;

			// Attempt to create the font with the given info.
			p_handles -> objectHandle[t_record -> ihFont] = CreateFontIndirectW(&t_record -> elfw . elfLogFont);
			if (p_handles -> objectHandle[t_record -> ihFont] == nil)
				t_success = false;
		}
		break;
	case EMR_SELECTOBJECT:
		{
			EMRSELECTOBJECT *t_record;
			t_record = (EMRSELECTOBJECT *)p_record;

			// If the object being selected is a font, update our current font.
			if (GetObjectType(p_handles -> objectHandle[t_record -> ihObject]) == OBJ_FONT)
				t_success = MCTextLayoutFontFromHFONT((HFONT)p_handles -> objectHandle[t_record -> ihObject], context -> current_font);
		}
		break;
	case EMR_DELETEOBJECT:
		{
			EMRDELETEOBJECT *t_record;
			t_record = (EMRDELETEOBJECT *)p_record;

			// If the object being deleted is our current font, reset selection
			// to nil.
			if (p_handles -> objectHandle[t_record -> ihObject] == context -> current_font)
				context -> current_font = context -> primary_font;

			DeleteObject(p_handles -> objectHandle[t_record -> ihObject]);
			p_handles -> objectHandle[t_record -> ihObject] = nil;
		}
		break;
	case EMR_EXTTEXTOUTW:
		{
			EMREXTTEXTOUTW *t_record;
			t_record = (EMREXTTEXTOUTW *)p_record;

			unichar_t *t_run_chars;
			uint32_t t_run_char_count;
			t_run_chars = (unichar_t *)((char *)p_record + t_record -> emrtext . offString);
			t_run_char_count = t_record -> emrtext . nChars;

			if (t_run_char_count > 0)
			{
				if (MCMemoryNewArray(t_run_char_count, t_new_pending_chars))
				{
					t_flush = true;
					t_new_pending_x = t_record -> emrtext . ptlReference . x;
					t_new_pending_font = context -> current_font;
					t_new_pending_char_count = t_run_char_count;
					MCMemoryCopy(t_new_pending_chars, t_run_chars, t_new_pending_char_count * sizeof(uint16_t));
				}
				else
					t_success = false;
			}
		}
		break;
	case EMR_EOF:
		t_new_pending_x = MAXINT4;
		t_flush = true;
		break;
	}

	if (t_success && t_flush)
	{
		if (context -> pending_chars != nil && t_new_pending_x != context -> pending_x)
			t_success = MCTextLayoutLinkItem(*(context -> state), context -> analysis, context -> pending_chars, context -> pending_char_count, context -> pending_font);
		
		MCMemoryDeleteArray(context -> pending_chars);

		context -> pending_chars = t_new_pending_chars;
		context -> pending_char_count = t_new_pending_char_count;
		context -> pending_font = t_new_pending_font;
		context -> pending_x = t_new_pending_x;
	}

	if (!t_success)
		MCMemoryDeleteArray(context -> pending_chars);

	return t_success;
}

static bool MCTextLayoutStyleItem(MCTextLayoutState& self, SCRIPT_ANALYSIS p_analysis, const unichar_t *p_chars, uint32_t p_char_count, MCTextLayoutFont *p_primary_font)
{
	bool t_success;
	t_success = true;

	// Create a metafile dc into which we render the text
	HDC t_metafile_dc;
	t_metafile_dc = nil;
	if (t_success)
	{
		t_metafile_dc = CreateEnhMetaFile(nil, nil, nil, nil);
		if (t_metafile_dc == nil)
			t_success = false;
	}

	// Choose the primary font
	if (t_success)
		SelectObject(t_metafile_dc, p_primary_font -> handle);

	// Now use ScriptStringAnalyse to output the text into the metafile.
	SCRIPT_STRING_ANALYSIS t_ssa;
	t_ssa = nil;
	if (t_success)
	{
		SCRIPT_STATE t_script_state;
		SCRIPT_CONTROL t_script_control;
		MCMemoryClear(&t_script_state, sizeof(SCRIPT_STATE));
		MCMemoryClear(&t_script_control, sizeof(SCRIPT_CONTROL));
		if (ScriptStringAnalyse(t_metafile_dc, p_chars, p_char_count, 0, -1, SSA_METAFILE | SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &t_script_control, &t_script_state, nil, nil, nil, &t_ssa) != S_OK)
			t_success = false;
	}

	// Render the analysed text into the metafile.
	if (t_success)
		if (ScriptStringOut(t_ssa, 0, 0, 0, nil, 0, 0, FALSE) != S_OK)
			t_success = false;

	// Fetch the metafile (we are done with the DC now)
	HENHMETAFILE t_metafile;
	t_metafile = nil;
	if (t_metafile_dc != nil)
	{
		t_metafile = CloseEnhMetaFile(t_metafile_dc);
		if (t_metafile == nil)
			t_success = false;
	}

	// Now process the metafile to get the subranges of text styled to the
	// appropriate fallback font.
	if (t_success)
	{
		MCTextLayoutStyleItemContext t_context;
		t_context . state = &self;
		t_context . chars = p_chars;
		t_context . char_count = p_char_count;
		t_context . analysis = p_analysis;
		t_context . primary_font = p_primary_font;
		t_context . current_font = p_primary_font;
		t_context . pending_x = 0;
		t_context . pending_font = nil;
		t_context . pending_chars = nil;
		t_context . pending_char_count = 0;
		if (!EnumEnhMetaFile(nil, t_metafile, MCTextLayoutStyleItemCallback, &t_context, nil))
			t_success = false;
	}

	// Free the metafile
	if (t_metafile != nil)
		DeleteEnhMetaFile(t_metafile);

	return t_success;
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	// The state structure we use to record the list of runs and the dc we use
	// for processing.
	MCTextLayoutState self;
	MCMemoryClear(&self, sizeof(MCTextLayoutState));

	if (t_success)
	{
		self . dc = CreateCompatibleDC(nil);
		if (self . dc == nil)
			t_success = false;
	}

	// Fetch a layout font for the provided HFONT.
	if (t_success)
		t_success = MCTextLayoutFontFromHFONT(p_font -> fid, self . primary_font);

	// First thing we need to do is itemize the input string. The ScriptItemize
	// function splits up the chars into runs, each run being potentially
	// processed differently by Uniscribe.
	// Unfortunately, there is no way to predict for an arbitrary string how
	// many items might be generated, nor is the ScriptItemize function
	// incremental, thus we must loop with an every increasing buffer until
	// we have enough room.
	SCRIPT_ITEM *t_items;
	uint32_t t_item_limit, t_item_count;
	SCRIPT_STATE t_script_state;
	SCRIPT_CONTROL t_script_control;
	t_items = nil;
	t_item_limit = 0;
	t_item_count = 0;
	MCMemoryClear(&t_script_state, sizeof(SCRIPT_STATE));
	MCMemoryClear(&t_script_control, sizeof(SCRIPT_CONTROL));
	while(t_success)
	{
		// Increase the item array by 32 each time
		if (t_success)
			t_success = MCMemoryResizeArray(t_item_limit + 32, t_items, t_item_limit);
		
		// Attempt to itemize
		HRESULT t_result;
		if (t_success)
		{
			t_result = ScriptItemize(p_chars, p_char_count, t_item_limit, &t_script_control, &t_script_state, t_items, (int *)&t_item_count);
			if (t_result != S_OK && t_result != E_OUTOFMEMORY)
				t_success = false;
		}

		if (t_success && t_result == S_OK)
			break;
	}

	// Next we loop through the items one by one, processing them as we go, this
	// process is slightly recursive - LayoutItem may recurse to fill in any
	// 'holes' caused by glyphs not in the primary font.
	for(uint32_t i = 0; i < t_item_count && t_success; i++)
		t_success = MCTextLayoutStyleItem(
						self,
						t_items[i] . a,
						p_chars + t_items[i] . iCharPos,
						t_items[i + 1] . iCharPos - t_items[i] . iCharPos,
						self . primary_font);

	// At this point we should have an array of runs to render. First though we
	// need to compute the visual to logical mapping.
	uint8_t *t_levels;
	int *t_map;
	t_levels = nil;
	t_map = nil;
	if (t_success)
		t_success =
			MCMemoryNewArray(self . run_count, t_levels) &&
			MCMemoryNewArray(self . run_count, t_map);

	// Work out the run mapping, but only if we have runs to map!
	if (t_success && self . run_count > 0)
	{
		for(uint32_t i = 0; i < self . run_count; i++)
			t_levels[i] = self . runs[i] . embedding_level;

		if (ScriptLayout(self . run_count, t_levels, t_map, nil) != S_OK)
			t_success = false;
	}

	// Now we have the mapping we loop through the runs in the correct order
	// dispatching them to the callback.
	if (t_success && p_callback != nil)
	{
		double t_x;
		t_x = 0.0;
		for(uint32_t i = 0; i < self . run_count && t_success; i++)
		{
			MCTextLayoutRun *t_run;
			t_run = &self . runs[t_map[i]];

			// Allocate a temporary array for the glyph structures
			MCTextLayoutGlyph *t_glyphs;
			t_glyphs = nil;
			if (t_success)
				t_success = MCMemoryNewArray(t_run -> glyph_count, t_glyphs);

			if (t_success)
			{
				// Compute the position for each glyph, keeping a running
				// total of the advance width.
				for(uint32_t i = 0; i < t_run -> glyph_count; i++)
				{
					// MW-2013-11-07: [[ Bug 10508 ]] We lay out in 256px units, so scale.
					t_glyphs[i] . index = t_run -> glyphs[i];
					t_glyphs[i] . x = (t_x + t_run -> goffsets[i] . du) * p_font -> size / 256.0;
					t_glyphs[i] . y = (t_run -> goffsets[i] . dv) * p_font -> size / 256.0;
					t_x += t_run -> advances[i];
				}

				// Dispatch the span to the callback.
				MCTextLayoutSpan t_span;
				t_span . chars = t_run -> chars;
				t_span . clusters = t_run -> clusters;
				t_span . char_count = t_run -> char_count;
				t_span . glyphs = t_glyphs;
				t_span . glyph_count = t_run -> glyph_count;
				t_span . font = t_run -> font -> handle;
				t_success = p_callback(p_context, &t_span);
			}

			// Free the temporary array.
			MCMemoryDeleteArray(t_glyphs);
		}
	}

	// Free all the arrays and other resources that have been allocated.
	MCMemoryDeleteArray(t_map);
	MCMemoryDeleteArray(t_levels);

	for(uint32_t i = 0; i < self . run_count; i++)
	{
		MCMemoryDeallocate(self . runs[i] . chars);
		MCMemoryDeallocate(self . runs[i] . clusters);
		MCMemoryDeallocate(self . runs[i] . glyphs);
		MCMemoryDeallocate(self . runs[i] . advances);
		MCMemoryDeallocate(self . runs[i] . goffsets);
	}

	MCMemoryDeleteArray(self . runs);
	
	MCMemoryDeleteArray(t_items);

	if (self . dc != nil)
		DeleteDC(self . dc);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
