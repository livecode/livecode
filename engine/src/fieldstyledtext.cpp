/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "field.h"
#include "paragraf.h"
#include "cdata.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "MCBlock.h"

#include "globals.h"
#include "unicode.h"

////////////////////////////////////////////////////////////////////////////////

// Context for styled text export.
struct export_styled_text_t
{
	// A temporary ep for array manipulation.
	MCExecPoint ep;
	// Whether a request was made for effective styles.
	bool effective;
	// Whether a request was made for formatted form.
	bool formatted;
	// The index of the last processed paragraph.
	uint32_t paragraph_index;
	// The index of the last processed run.
	uint32_t run_index;
	// The array of paragraphs.
	MCVariableValue *paragraphs_array;
	// The array of runs in the paragraph currently being processed.
	MCVariableValue *runs_array;
	// The last run in the current paragraph (or nil if there hasn't been any).
	bool last_run_unicode;
	MCVariableValue *last_run;
	
};

// Copy the styles from the struct into the array.
static void export_styled_text_paragraph_style(MCExecPoint& ep, MCVariableValue *p_style_array, const MCFieldParagraphStyle& p_style, bool p_effective)
{
	if (p_style . has_text_align || p_effective)
	{
		MCF_unparsetextatts(P_TEXT_ALIGN, ep, p_style . text_align << F_ALIGNMENT_SHIFT, nil, 0, 0, 0);
		p_style_array -> store_element(ep, "textAlign");
	}
	if (p_style . has_list_style)
	{
		ep . setstaticcstring(MCliststylestrings[p_style . list_style]);
		p_style_array -> store_element(ep, "listStyle");
		// MW-2012-02-22: [[ Bug ]] The listDepth property is stored internally as depth - 1, so adjust.
		ep . setint(p_style . list_depth + 1);
		p_style_array -> store_element(ep, "listDepth");
		if (p_style . has_list_indent)
		{
			ep . setint(p_style . list_indent);
			p_style_array -> store_element(ep, "listIndent");
		}
	}
	if (!p_style . has_list_indent && (p_style . has_first_indent || p_effective))
	{
		ep . setint(p_style . first_indent);
		p_style_array -> store_element(ep, "firstIndent");
	}
	if (p_style . has_left_indent || p_effective)
	{
		ep . setint(p_style . left_indent);
		p_style_array -> store_element(ep, "leftIndent");
	}
	if (p_style . has_list_index)
	{
		ep . setint(p_style . list_index);
		p_style_array -> store_element(ep, "listIndex");
	}
	if (p_style . has_right_indent || p_effective)
	{
		ep . setint(p_style . right_indent);
		p_style_array -> store_element(ep, "rightIndent");
	}
	if (p_style . has_space_above || p_effective)
	{
		ep . setint(p_style . space_above);
		p_style_array -> store_element(ep, "spaceAbove");
	}
	if (p_style . has_space_below || p_effective)
	{
		ep . setint(p_style . space_below);
		p_style_array -> store_element(ep, "spaceBelow");
	}
	if (p_style . has_tabs || p_effective)
	{
		MCField::formattabstops(P_TAB_STOPS, ep, p_style . tabs, p_style . tab_count);
		p_style_array -> store_element(ep, "tabStops");
	}
	if (p_style . has_background_color)
	{
		ep . setpixel(p_style . background_color);
		p_style_array -> store_element(ep, "backgroundColor");
	}
	if (p_style . has_border_width || p_effective)
	{
		ep . setint(p_style . border_width);
		p_style_array -> store_element(ep, "borderWidth");
	}
	if (p_style . has_hgrid || p_effective)
	{
		ep . setboolean(p_style . hgrid);
		p_style_array -> store_element(ep, "hGrid");
	}
	if (p_style . has_vgrid || p_effective)
	{
		ep . setboolean(p_style . vgrid);
		p_style_array -> store_element(ep, "vGrid");
	}
	if (p_style . has_border_color || p_effective)
	{
		ep . setpixel(p_style . border_color);
		p_style_array -> store_element(ep, "borderColor");
	}
	if (p_style . has_dont_wrap || p_effective)
	{
		ep . setboolean(p_style . dont_wrap);
		p_style_array -> store_element(ep, "dontWrap");
	}
	if (p_style . has_padding || p_effective)
	{
		ep . setint(p_style . padding);
		p_style_array -> store_element(ep, "padding");
	}
}

// Copy the styles from the struct into the array.
static void export_styled_text_character_style(MCExecPoint& ep, MCVariableValue *p_style_array, const MCFieldCharacterStyle& p_style, bool p_effective)
{
	if (p_style . has_text_color || p_effective)
	{
		ep . setpixel(p_style . text_color);
		p_style_array -> store_element(ep, "textColor");
	}
	if (p_style . has_background_color)
	{
		ep . setpixel(p_style . background_color);
		p_style_array -> store_element(ep, "backgroundColor");
	}
	if (p_style . has_link_text)
	{
		ep . setnameref_unsafe(p_style . link_text);
		p_style_array -> store_element(ep, "linkText");
	}
	if (p_style . has_image_source)
	{
		ep . setnameref_unsafe(p_style . image_source);
		p_style_array -> store_element(ep, "imageSource");
	}
	if (p_style . has_text_font || p_effective)
	{
		ep . setnameref_unsafe(p_style . text_font);
		p_style_array -> store_element(ep, "textFont");
	}
	if (p_style . has_text_style || p_effective)
	{
		MCF_unparsetextatts(P_TEXT_STYLE, ep, 0, nil, 0, 0, p_style . text_style);
		p_style_array -> store_element(ep, "textStyle");
	}
	if (p_style . has_text_size || p_effective)
	{
		ep . setint(p_style . text_size);
		p_style_array -> store_element(ep, "textSize");
	}
	if (p_style . has_text_shift)
	{
		ep . setint(p_style . text_shift);
		p_style_array -> store_element(ep, "textShift");
	}
}

// The callback for exporting as styled text.
static bool export_styled_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	export_styled_text_t& ctxt = *(export_styled_text_t *)p_context;

	if (p_event_type == kMCFieldExportEventBeginParagraph)
	{
		// Update the paragraph index.
		ctxt . paragraph_index += 1;
		
		// Lookup (and create) the next paragraph array entry in the top-level
		// array.
		MCVariableValue *t_paragraph_array;
		ctxt . paragraphs_array -> lookup_index(ctxt . ep, ctxt . paragraph_index, t_paragraph_array);

		// If the paragraph has styling, then apply it.
		if (p_event_data . has_paragraph_style || ctxt . effective)
		{
			MCVariableValue *t_paragraph_style_array;
			t_paragraph_array -> lookup_element(ctxt . ep, "style", t_paragraph_style_array);
			export_styled_text_paragraph_style(ctxt . ep, t_paragraph_style_array, p_event_data . paragraph_style, ctxt . effective);
		}

		// MW-2012-11-13: [[ ParaMetadata ]] If the paragraph has metadata, then apply it.
		if (p_event_data . has_paragraph_style && p_event_data . paragraph_style . has_metadata)
		{
			ctxt . ep . setnameref_unsafe(p_event_data . paragraph_style . metadata);
			t_paragraph_array -> store_element(ctxt . ep, "metadata");
		}
		
		// MW-2012-03-05: [[ HiddenText ]] If the paragraph is hidden, mark it as such in the
		//   array.
		if (p_event_data . paragraph_style . hidden)
		{
			ctxt . ep . setboolean(true);
			t_paragraph_array -> store_element(ctxt . ep, "hidden");
		}

		// Now create the 'runs' entry in the paragraph array.
		t_paragraph_array -> lookup_element(ctxt . ep, "runs", ctxt . runs_array);
		ctxt . run_index = 0;
		ctxt . last_run = nil;
	}
	else if (p_event_type == kMCFieldExportEventNativeRun || p_event_type == kMCFieldExportEventUnicodeRun)
	{
		// Update the run index.
		ctxt . run_index += 1;

		// Create the block's 'run' entry in the paragraph's runs array.
		MCVariableValue *t_run_array;
		ctxt . runs_array -> lookup_index(ctxt . ep, ctxt . run_index, t_run_array);

		// Set the block's 'text' or 'unicodeText' entry in the block's run array.
		// Depending on whether its unicode or not.
		ctxt . ep . setstaticmcstring(MCString((char *)p_event_data . bytes, p_event_data . byte_count));
		t_run_array -> store_element(ctxt . ep, p_event_type == kMCFieldExportEventUnicodeRun ? "unicodeText" : "text");

		// Set the block's 'metadata' entry in the block's run array.
		if (p_event_data . character_style . has_metadata)
		{
			ctxt . ep . setnameref_unsafe(p_event_data . character_style . metadata);
			t_run_array -> store_element(ctxt . ep, "metadata");
		}

		// Now if the block has any attributes then build the array.
		if (p_event_data . has_character_style || ctxt . effective)
		{
			MCVariableValue *t_run_style_array;
			t_run_array -> lookup_element(ctxt . ep, "style", t_run_style_array);
			export_styled_text_character_style(ctxt . ep, t_run_style_array, p_event_data . character_style, ctxt . effective);
		}
		
		ctxt . last_run_unicode = p_event_type == kMCFieldExportEventUnicodeRun;
		ctxt . last_run = t_run_array;
	}
	else if (p_event_type == kMCFieldExportEventLineBreak)
	{
		MCVariableValue *t_run_array;
		bool t_is_unicode;
		if (ctxt . last_run == nil)
		{
			// Update the run index.
			ctxt . run_index += 1;
			
			// Create a run entry.
			ctxt . runs_array -> lookup_index(ctxt . ep, ctxt . run_index, t_run_array);
			
			// No need for unicode-ness as this will be an independent run.
			t_is_unicode = false;
		}
		else
		{
			// Take the run and unicode setting from the context.
			t_run_array = ctxt . last_run;
			t_is_unicode = ctxt . last_run_unicode;
		}

		// Now append the line break char - either as UTF-16 or native, depending
		// on what mode we are in.
		if (t_is_unicode)
		{
			uint16_t t_vtab;
			t_vtab = 11;
			ctxt . ep . setstaticbytes(&t_vtab, 2);
			t_run_array -> append_element(ctxt . ep, "unicodeText");
		}
		else
		{
			ctxt . ep . setstaticcstring("\x0b");
			t_run_array -> append_element(ctxt . ep, "text");
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

// This method builds a 'styledText' array. The top-level of the array is a
// sequence of paragraph arrays.
//
// Each paragraph array has two keys (style, runs). Where 'style' is an array
// of style properties and 'runs' is a sequence of block arrays. If the para-
// graph has no non-default styles set, then the style key is not present. If the
// paragraph has no blocks (i.e. is empty) then the runs key is empty.
//
// Each block array has two keys (style, text). Where 'style' is an array
// of style properties and 'text' is the sequence of chars making up that
// block. If the block has no non-default styles then the style key is not
// present.
//
void MCField::exportasstyledtext(MCExecPoint& ep, MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective)
{
	export_styled_text_t t_context;
	t_context . effective = p_effective;
	t_context . formatted = p_formatted;
	t_context . paragraph_index = 0;
	t_context . run_index = 0;
	t_context . runs_array = nil;
	t_context . paragraphs_array = new MCVariableValue;

	// Compute the flags we need - in particular flattening styles if effective
	// is true.
	uint32_t t_flags;
	t_flags = kMCFieldExportRuns | kMCFieldExportParagraphs | kMCFieldExportCharacterStyles | kMCFieldExportParagraphStyles;
	if (p_effective)
		t_flags |= kMCFieldExportFlattenStyles;
	if (p_formatted)
		t_flags |= kMCFieldExportLines;
	doexport(t_flags, p_paragraphs, p_start_index, p_finish_index, export_styled_text, &t_context);

	ep . setarray(t_context . paragraphs_array, True);
}

void MCField::exportasstyledtext(uint32_t p_part_id, MCExecPoint& ep, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective)
{
	exportasstyledtext(ep, resolveparagraphs(p_part_id), p_start_index, p_finish_index, p_formatted, p_effective);
}

////////////////////////////////////////////////////////////////////////////////

// This method converts a general styledText array to paragraphs objects. The
// 'parsing' of the array is quite permissive, implementing the following
// algorithm:
//	parseStyledTextArray pStyledText
//		repeat for each element tEntry of pStyledText
//			if tEntry is a sequence then
//				parseStyledTextArray tEntry
//			else if tEntry has key "runs" then
//				begin paragraph with style tEntry["style"]
//				repeat for each element tRun in tEntry["runs"
//					append tRun["text"] with style tRun["style"]
//				end paragraph
//			else if tEntry is an array then
//				append tEntry["text"] with style tEntry["style"]
//
MCParagraph *MCField::styledtexttoparagraphs(MCExecPoint& ep)
{
	// If the ep isn't an array, do nothing.
	MCVariableValue *t_styled_text;
	t_styled_text = ep . getarray();
	if (t_styled_text == nil)
		return nil;
	
	// Get the array itself, and if it is not a sequence, do nothing.
	MCVariableArray *t_array;
	t_array = t_styled_text -> get_array();
	if (!t_array -> issequence())
		return nil;

	MCParagraph *t_paragraphs;
	t_paragraphs = nil;
	parsestyledtextarray(t_array, true, t_paragraphs);

	return t_paragraphs;
}

MCParagraph *MCField::parsestyledtextappendparagraph(MCVariableValue *p_style, const char *p_metadata, bool p_split, MCParagraph*& x_paragraphs)
{
	MCParagraph *t_new_paragraph;
	t_new_paragraph = new MCParagraph;
	t_new_paragraph -> state |= PS_LINES_NOT_SYNCHED;
	t_new_paragraph -> setparent(this);
	t_new_paragraph -> blocks = new MCBlock;
	t_new_paragraph -> blocks -> setparent(t_new_paragraph);
	t_new_paragraph -> blocks -> index = 0;
	t_new_paragraph -> blocks -> size = 0;
	
	if (p_style != nil || p_metadata != nil)
	{
		if (p_style != nil)
			t_new_paragraph -> fetchattrs(p_style);
		if (p_metadata != nil)
			t_new_paragraph -> setmetadata(p_metadata);
	}
	else if (p_split)
	{
		t_new_paragraph -> copyattrs(*(x_paragraphs -> prev()));

		// Splitting paragraphs shouldn't copy the list index.
		t_new_paragraph -> setlistindex(0);
	}
	
	if (x_paragraphs != nil)
		x_paragraphs -> prev() -> append(t_new_paragraph);
	else
		x_paragraphs = t_new_paragraph;
	
	return t_new_paragraph;
}

void MCField::parsestyledtextappendblock(MCParagraph *p_paragraph, MCVariableValue *p_style, const char *p_initial, const char *p_final, const char *p_metadata, bool p_is_unicode)
{
	// Make sure we don't try and append any more than 64K worth of bytes.
	uint32_t t_text_length;
	
	// MW-2013-05-03: [[ x64 ]] Make sure both sides are unsigned to ensure
	//   correct template can be selected (through auto promotion).
	t_text_length = MCMin((unsigned)(p_final - p_initial), (unsigned)(65534 - p_paragraph -> textsize));

	// If we are unicode and the text length is odd, chop off the last char.
	if (p_is_unicode && (t_text_length & 1) != 0)
		t_text_length -= 1;
	
	// If there is not text to fill, do nothing.
	if (t_text_length == 0)
		return;
	
	// If the buffer is not big enough, then extend it.
	if (p_paragraph -> textsize + t_text_length > p_paragraph -> buffersize)
	{
		p_paragraph -> buffersize = MCU_min(65534U, (p_paragraph -> textsize + t_text_length + 64) & ~63);
		p_paragraph -> text = (char *)realloc(p_paragraph -> text, p_paragraph -> buffersize);
	}
	
	// Get the block we will work on, creating one if it has already been
	// populated.
	MCBlock *t_block;
	t_block = p_paragraph -> blocks -> prev();
	if (t_block -> size != 0)
	{
		MCBlock *t_new_block;
		t_new_block = new MCBlock;
		t_new_block -> parent = p_paragraph;
		t_block -> append(t_new_block);
		t_block = t_new_block;
	}
	
	// The start of the block is the end of the text.
	t_block -> index = p_paragraph -> textsize;
	
	// The size of the block is the text length.
	t_block -> size = t_text_length;
	
	// Copy across the bytes of the text.
	memcpy(p_paragraph -> text + t_block -> index, p_initial, t_block -> size);
	p_paragraph -> textsize += t_block -> size;
	
	// Now set the block styles.
	MCExecPoint ep(nil, nil, nil);
	
	// Set foreground
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "textColor", false))
	{
		MCColor t_color;
		char *t_cname;
		t_cname = nil;
		if (MCscreen -> parsecolor(ep . getsvalue(), &t_color, &t_cname))
		{
			t_block -> setcolor(&t_color);
			delete t_cname;
		}
	}
	
	// Set background
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "backgroundColor", false))
	{
		MCColor t_color;
		char *t_cname;
		t_cname = nil;
		if (MCscreen -> parsecolor(ep . getsvalue(), &t_color, &t_cname))
		{
			t_block -> setbackcolor(&t_color);
			delete t_cname;
		}
	}
	
	// Set textshift
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "textShift", false))
		t_block -> setshift(ep . getint4());
	
	// If the block is unicode then we always have to set textFont. In either
	// case if there is a textFont key, strip any ,unicode tag first.
	bool t_has_text_font;
	t_has_text_font = false;
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "textFont", false))
	{
		const char *t_font_tag;
		t_font_tag = strchr(ep . getcstring(), ',');
		if (t_font_tag != nil)
			ep . substring(0, ep . getcstring() - t_font_tag);

		// If the block is empty after stripping the tag, then we don't have
		// a textFont.
		t_has_text_font = !ep.isempty();
	}
	else
		ep . clear();

	// Now if we have unicode, or a textFont style set it.
	if (t_has_text_font)
		t_block -> setatts(P_TEXT_FONT, (void *)ep . getcstring());
	
	// Make sure the unicode flag is set.
	if (p_is_unicode)
		t_block -> flags |= F_HAS_UNICODE;
	
	// Make sure the tab flag is updated.
	if (t_block -> textstrchr(p_paragraph -> text + t_block -> index, t_block -> size, '\t'))
		t_block -> flags |= F_HAS_TAB;
	
	// Set textsize
    // AL-2013-11-26: [[ Bug 11458 ]] Ensure text size array element is converted to int4
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "textSize", false))
    {
        int4 t_size;
        if (MCU_stoi4(ep . getsvalue(), t_size))
            t_block -> setatts(P_TEXT_SIZE, (void *)t_size);
    }
	
	// Set textstyle
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "textStyle", false))
	{
		uint4 flags;
		char *fname;
		uint2 height;
		uint2 size;
		uint2 style;
		MCF_parsetextatts(P_TEXT_STYLE, ep . getsvalue(), flags, fname, height, size, style);
		t_block -> setatts(P_TEXT_STYLE, (void *)style);
	}

	// Set linktext
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "linkText", false))
		t_block -> setatts(P_LINK_TEXT, (void *)ep . getcstring());

	// Set imagesource
	if (p_style != nil && p_style -> fetch_element_if_exists(ep, "imageSource", false))
		t_block -> setatts(P_IMAGE_SOURCE, (void *)ep . getcstring());

	// Set metadata
	if (p_metadata != nil)
		t_block -> setatts(P_METADATA, (void *)p_metadata);
}

void MCField::parsestyledtextblockarray(MCVariableValue *p_block_value, MCParagraph*& x_paragraphs)
{
	// If the value isn't an array, do nothing.
	if (!p_block_value -> is_array())
		return;
	
	// If the value is a sequence, recurse for each element.
	if (p_block_value -> get_array() -> issequence())
	{
		for(uint32_t j = 1; j <= p_block_value -> get_array() -> getnfilled(); j++)
		{
			MCHashentry *t_block_entry;
			t_block_entry = p_block_value -> get_array() -> lookupindex(j, False);
			if (t_block_entry == nil)
				continue;
		
			parsestyledtextblockarray(&t_block_entry -> value, x_paragraphs);
		}

		return;
	}
	
	// Fetch either the text entry, or the unicodeText entry and set the is unicode bool
	// appropriately.
	bool t_is_unicode;
	MCHashentry *t_text_entry;
	t_text_entry = p_block_value -> get_array() -> lookuphash("text", False, False);
	if (t_text_entry == nil)
	{
		t_text_entry = p_block_value -> get_array() -> lookuphash("unicodeText", False, False);
		t_is_unicode = true;
	}
	else
		t_is_unicode = false;
	
	// If the text entry is empty or is an array, or isn't found, do nothing.
	if (t_text_entry == nil || t_text_entry -> value . is_array() || t_text_entry -> value . is_empty())
		return;
	
	// We'll need an ep for processing.
	MCExecPoint ep(nil, nil, nil);
	
	// Fetch the style array.
	MCHashentry *t_style_entry;
	t_style_entry = p_block_value -> get_array() -> lookuphash("style", False, False);

	// Get the metadata (if any)
	char *t_metadata;
	t_metadata = nil;
	if (p_block_value -> fetch_element_if_exists(ep, "metadata", false))
		t_metadata = ep . getsvalue() . clone();

	// If there are no paragraphs yet, create one.
	MCParagraph *t_paragraph;
	t_paragraph = x_paragraphs -> prev();
	
	// Now loop through each newline delimited chunk in the text.
	const char *t_text_ptr;
	uint32_t t_text_length;
	t_text_entry -> value . fetch(ep, false);
	t_text_ptr = ep . getsvalue() . getstring();
	t_text_length = ep . getsvalue() . getlength();
	while(t_text_length != 0)
	{
		bool t_add_paragraph;
		const char *t_text_initial_ptr;
		const char *t_text_final_ptr;
		t_text_initial_ptr = t_text_ptr;
		if (MCU_strchr(t_text_ptr, t_text_length, '\n', t_is_unicode))
		{
			t_text_final_ptr = t_text_ptr;
			// MW-2012-05-17: [[ Bug ]] Make sure we reduce the remaining text length since
			//   we are advancing ptr - otherwise we get random chars at the end of paragraph
			//   sometimes.
			t_text_ptr += (t_is_unicode ? 2 : 1);
			t_text_length -= (t_is_unicode ? 2 : 1);
			t_add_paragraph = true;
		}
		else
		{
			t_text_ptr += t_text_length;
			t_text_length = 0;
			t_text_final_ptr = t_text_ptr;
			t_add_paragraph = false;
		}

		// We now add the range initial...final as a block.
		parsestyledtextappendblock(t_paragraph, t_style_entry != nil ? &t_style_entry -> value : nil, t_text_initial_ptr, t_text_final_ptr, t_metadata, t_is_unicode);
		
		// And, if we need a new paragraph, add it.
		if (t_add_paragraph)
			t_paragraph = parsestyledtextappendparagraph(nil, nil, true, x_paragraphs);
	}

	// We had to copy the metadata temporarily (as its evaluated
	// into an ep and we needed a cstring), so delete it.
	delete t_metadata;
}

void MCField::parsestyledtextarray(MCVariableArray *p_styled_text, bool p_paragraph_break, MCParagraph*& x_paragraphs)
{	
	for(uint32_t i = 1; i <= p_styled_text -> getnfilled(); i++)
	{
		// Lookup the index - this shouldn't ever fail as its a sequence
		// but just continue gracefully if it does.
		MCHashentry *t_entry;
		t_entry = p_styled_text -> lookupindex(i, False);
		if (t_entry == nil)
			continue;
		
		// Fetch the value of the entry.
		MCVariableValue *t_value;
		t_value = &t_entry -> value;
		
		// If the value is not an array, then continue gracefully.
		if (!t_value -> is_array())
			continue;
			
		// If the array is a sequence, then recurse.
		if (t_value -> get_array() -> issequence())
		{
			parsestyledtextarray(t_value -> get_array(), p_paragraph_break, x_paragraphs);
			continue;
		}
		
		// Fetch the style array.
		MCHashentry *t_style_entry;
		t_style_entry = t_value -> get_array() -> lookuphash("style", False, False);
		
		// MW-2012-11-13: [[ ParaMetadata ]] Fetch the metadata (if any)
		MCExecPoint ep;
		MCAutoPointer<char> t_metadata;
		if (t_value -> fetch_element_if_exists(ep, "metadata", false))
			t_metadata = ep . getsvalue() . clone();

		// If the array looks like a paragraph array, then treat it as such.
		MCHashentry *t_runs_entry;
		t_runs_entry = t_value -> get_array() -> lookuphash("runs", False, False);
		if (t_runs_entry != nil)
		{
			// Fetch the runs array.
			MCVariableValue *t_runs_value;
			t_runs_value = &t_runs_entry -> value;
			
			// To continue, the value must either be a sequence-style array or
			// empty.
			MCVariableArray *t_runs_array;
			t_runs_array = nil;
			if (t_runs_value -> is_array())
			{
				t_runs_array = t_runs_value -> get_array();
				if (!t_runs_array -> issequence())
					continue;
			}
			else if (!t_runs_value -> is_empty())
				continue;
			
			// Begin paragraph with style
			parsestyledtextappendparagraph(t_style_entry != nil ? &t_style_entry -> value : nil, *t_metadata, false, x_paragraphs);
					
			// Finally, we are a sequence so loop through all the elements.
			if (t_runs_array != nil)
				for(uint32_t j = 1; j <= t_runs_array -> getnfilled(); j++)
				{
					MCHashentry *t_block_entry;
					t_block_entry = t_runs_array -> lookupindex(j, False);
					if (t_entry == nil)
						continue;
					
					parsestyledtextblockarray(&t_block_entry -> value, x_paragraphs);
				}
			
			// End paragraph
			p_paragraph_break = true;
			
			continue;
		}
		
		// If we need a new paragraph, add one.
		if (p_paragraph_break)
		{
			p_paragraph_break = false;
			parsestyledtextappendparagraph(nil, nil, false, x_paragraphs);
		}
			
		// Finally, attempt to parse a block array.
		parsestyledtextblockarray(t_value, x_paragraphs);
	}
}

////////////////////////////////////////////////////////////////////////////////
