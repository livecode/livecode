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
#include "block.h"

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
	MCArrayRef paragraphs_array;
	// The current paragraph being built.
	MCArrayRef paragraph_array;
	// The array of runs in the paragraph currently being processed.
	MCArrayRef runs_array;
	// The last run in the current paragraph (or nil if there hasn't been any).
	bool last_run_unicode;
	MCArrayRef last_run;
	
};

// Copy the styles from the struct into the array.
static void export_styled_text_paragraph_style(MCExecPoint& ep, MCArrayRef p_style_array, const MCFieldParagraphStyle& p_style, bool p_effective)
{
	if (p_style . has_text_align || p_effective)
	{
		MCF_unparsetextatts(P_TEXT_ALIGN, ep, p_style . text_align << F_ALIGNMENT_SHIFT, nil, 0, 0, 0);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textAlign");
	}
	if (p_style . has_list_style)
	{
		ep . setstaticcstring(MCliststylestrings[p_style . list_style]);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "listStyle");
		// MW-2012-02-22: [[ Bug ]] The listDepth property is stored internally as depth - 1, so adjust.
		ep . setint(p_style . list_depth + 1);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "listDepth");
		if (p_style . has_list_indent)
		{
			ep . setint(p_style . list_indent);
			/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "listIndent");
		}
	}
	if (!p_style . has_list_indent && (p_style . has_first_indent || p_effective))
	{
		ep . setint(p_style . first_indent);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "firstIndent");
	}
	if (p_style . has_left_indent || p_effective)
	{
		ep . setint(p_style . left_indent);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "leftIndent");
	}
	if (p_style . has_list_index)
	{
		ep . setint(p_style . list_index);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "listIndex");
	}
	if (p_style . has_right_indent || p_effective)
	{
		ep . setint(p_style . right_indent);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "rightIndent");
	}
	if (p_style . has_space_above || p_effective)
	{
		ep . setint(p_style . space_above);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "spaceAbove");
	}
	if (p_style . has_space_below || p_effective)
	{
		ep . setint(p_style . space_below);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "spaceBelow");
	}
	if (p_style . has_tabs || p_effective)
	{
		MCField::formattabstops(P_TAB_STOPS, ep, p_style . tabs, p_style . tab_count);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "tabStops");
	}
	if (p_style . has_background_color)
	{
		ep . setpixel(p_style . background_color);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "backgroundColor");
	}
	if (p_style . has_border_width || p_effective)
	{
		ep . setint(p_style . border_width);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "borderWidth");
	}
	if (p_style . has_hgrid || p_effective)
	{
		ep . setboolean(p_style . hgrid);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "hGrid");
	}
	if (p_style . has_vgrid || p_effective)
	{
		ep . setboolean(p_style . vgrid);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "vGrid");
	}
	if (p_style . has_border_color || p_effective)
	{
		ep . setpixel(p_style . border_color);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "borderColor");
	}
	if (p_style . has_dont_wrap || p_effective)
	{
		ep . setboolean(p_style . dont_wrap);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "dontWrap");
	}
	if (p_style . has_padding || p_effective)
	{
		ep . setint(p_style . padding);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "padding");
	}
}

// Copy the styles from the struct into the array.
static void export_styled_text_character_style(MCExecPoint& ep, MCArrayRef p_style_array, const MCFieldCharacterStyle& p_style, bool p_effective)
{
	if (p_style . has_text_color || p_effective)
	{
		ep . setpixel(p_style . text_color);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textColor");
	}
	if (p_style . has_background_color)
	{
		ep . setpixel(p_style . background_color);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "backgroundColor");
	}
	if (p_style . has_link_text)
	{
		ep . setvalueref(p_style . link_text);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "linkText");
	}
	if (p_style . has_image_source)
	{
		ep . setvalueref(p_style . image_source);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "imageSource");
	}
	if (p_style . has_text_font || p_effective)
	{
		ep . setvalueref(p_style . text_font);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textFont");
	}
	if (p_style . has_text_style || p_effective)
	{
		MCF_unparsetextatts(P_TEXT_STYLE, ep, 0, nil, 0, 0, p_style . text_style);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textStyle");
	}
	if (p_style . has_text_size || p_effective)
	{
		ep . setint(p_style . text_size);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textSize");
	}
	if (p_style . has_text_shift)
	{
		ep . setint(p_style . text_shift);
		/* UNCHECKED */ ep . storearrayelement_cstring(p_style_array, "textShift");
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
		/* UNCHECKED */ MCArrayCreateMutable(ctxt . paragraph_array);

		// If the paragraph has styling, then apply it.
		if (p_event_data . has_paragraph_style || ctxt . effective)
		{
			MCAutoArrayRef t_paragraph_style_array;
			/* UNCHECKED */ MCArrayCreateMutable(&t_paragraph_style_array);
			export_styled_text_paragraph_style(ctxt . ep, *t_paragraph_style_array, p_event_data . paragraph_style, ctxt . effective);
			/* UNCHECKED */ ctxt . ep . setvalueref(*t_paragraph_style_array);
			ctxt . ep . storearrayelement_cstring(ctxt . paragraph_array, "style");
		}

		// MW-2012-11-13: [[ ParaMetadata ]] If the paragraph has metadata, then apply it.
		if (p_event_data . has_paragraph_style && p_event_data . paragraph_style . has_metadata)
		{
			/* UNCHECKED */ ctxt . ep . setvalueref(p_event_data . paragraph_style . metadata);
			ctxt . ep . storearrayelement_cstring(ctxt . paragraph_array, "metadata");
		}
		
		// MW-2012-03-05: [[ HiddenText ]] If the paragraph is hidden, mark it as such in the
		//   array.
		if (p_event_data . paragraph_style . hidden)
		{
			ctxt . ep . setboolean(true);
			ctxt . ep . storearrayelement_cstring(ctxt . paragraph_array, "hidden");
		}

		// Now create the 'runs' entry in the paragraph array.
		/* UNCHECKED */ MCArrayCreateMutable(ctxt . runs_array);

		ctxt . run_index = 0;
		ctxt . last_run = nil;
	}
	else if (p_event_type == kMCFieldExportEventEndParagraph)
	{
		if (ctxt . last_run != nil)
		{
			/* UNCHECKED */ ctxt . ep . setvalueref(ctxt . last_run);
			/* UNCHECKED */ ctxt . ep . storearrayindex(ctxt . runs_array, ctxt . run_index);
			MCValueRelease(ctxt . last_run);
			ctxt . last_run = nil;
		}

		/* UNCHECKED */ ctxt . ep . setvalueref(ctxt . runs_array);
		/* UNCHECKED */ ctxt . ep . storearrayelement_cstring(ctxt . paragraph_array, "runs");
		MCValueRelease(ctxt . runs_array);
		ctxt . runs_array = nil;

		/* UNCHECKED */ ctxt . ep . setvalueref(ctxt . paragraph_array);
		/* UNCHECKED */ ctxt . ep . storearrayindex(ctxt . paragraphs_array, ctxt . paragraph_index);
		MCValueRelease(ctxt . paragraph_array);
		ctxt . paragraph_array = nil;
	}
	else if (p_event_type == kMCFieldExportEventNativeRun || p_event_type == kMCFieldExportEventUnicodeRun)
	{
		if (ctxt . last_run != nil)
		{	
			/* UNCHECKED */ ctxt . ep . setvalueref(ctxt . last_run);
			/* UNCHECKED */ ctxt . ep . storearrayindex(ctxt . runs_array, ctxt . run_index);
			MCValueRelease(ctxt . last_run);
			ctxt . last_run = nil;
		}

		// Update the run index.
		ctxt . run_index += 1;

		// Create the block's 'run' entry in the paragraph's runs array.
		/* UNCHECKED */ MCArrayCreateMutable(ctxt . last_run);
		ctxt . last_run_unicode = p_event_type == kMCFieldExportEventUnicodeRun;

		// Set the block's 'text' or 'unicodeText' entry in the block's run array.
		// Depending on whether its unicode or not.
		ctxt . ep . setstaticmcstring(MCString((char *)p_event_data . bytes, p_event_data . byte_count));
		/* UNCHECKED */ ctxt . ep . storearrayelement_cstring(ctxt . last_run, p_event_type == kMCFieldExportEventUnicodeRun ? "unicodeText" : "text");

		// Set the block's 'metadata' entry in the block's run array.
		if (p_event_data . character_style . has_metadata)
		{
			ctxt . ep . setvalueref(p_event_data . character_style . metadata);
			/* UNCHECKED */ ctxt . ep . storearrayelement_cstring(ctxt . last_run, "metadata");
		}

		// Now if the block has any attributes then build the array.
		if (p_event_data . has_character_style || ctxt . effective)
		{
			MCAutoArrayRef t_run_style_array;
			/* UNCHECKED */ MCArrayCreateMutable(&t_run_style_array);
			export_styled_text_character_style(ctxt . ep, *t_run_style_array, p_event_data . character_style, ctxt . effective);
			/* UNCHECKED */ ctxt . ep . setvalueref(*t_run_style_array);
			/* UNCHECKED */ ctxt . ep . storearrayelement_cstring(ctxt . last_run, "style");
		}
	}
	else if (p_event_type == kMCFieldExportEventLineBreak)
	{
		if (ctxt . last_run == nil)
		{
			// Update the run index.
			ctxt . run_index += 1;
			
			// Create a run entry.
			/* UNCHECKED */ MCArrayCreateMutable(ctxt . last_run);
			ctxt . last_run_unicode = false;
		}

		// Now append the line break char - either as UTF-16 or native, depending
		// on what mode we are in.
		if (ctxt . last_run_unicode)
		{
			uint16_t t_vtab;
			t_vtab = 11;
			ctxt . ep . setstaticbytes(&t_vtab, 2);
			/* UNCHECKED */ ctxt . ep . appendarrayelement_cstring(ctxt . last_run, "unicodeText");
		}
		else
		{
			ctxt . ep . setstaticcstring("\x0b");
			/* UNCHECKED */ ctxt . ep . appendarrayelement_cstring(ctxt . last_run, "text");
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
void MCField::exportasstyledtext(uint32_t p_part_id, MCExecPoint& ep, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective)
{
	MCAutoArrayRef t_array;

	if (exportasstyledtext(p_part_id, p_start_index, p_finish_index, p_formatted, p_effective, &t_array))
		/* UNCHECKED */ ep . setvalueref(*t_array);
	else
		ep . clear();
}

bool MCField::exportasstyledtext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective, MCArrayRef &r_array)
{
	export_styled_text_t t_context;
	t_context . effective = p_effective;
	t_context . formatted = p_formatted;
	t_context . paragraph_index = 0;
	t_context . run_index = 0;
	t_context . runs_array = nil;
	if (MCArrayCreateMutable(t_context . paragraphs_array))
	{
		// Compute the flags we need - in particular flattening styles if effective
		// is true.
		uint32_t t_flags;
		t_flags = kMCFieldExportRuns | kMCFieldExportParagraphs | kMCFieldExportCharacterStyles | kMCFieldExportParagraphStyles;
		if (p_effective)
			t_flags |= kMCFieldExportFlattenStyles;
		if (p_formatted)
			t_flags |= kMCFieldExportLines;
		doexport(t_flags, p_part_id, p_start_index, p_finish_index, export_styled_text, &t_context);

		if (MCArrayCopyAndRelease(t_context . paragraphs_array, r_array))
			return true;
	}

	return false;

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
	// Get the array itself, and if it is not a sequence, do nothing.
	MCAutoArrayRef t_array;
	/* UNCHECKED */ ep . copyasarrayref(&t_array);
	if (ep . isempty())
		return nil;
	else
		return styledtexttoparagraphs(*t_array);
}

MCParagraph *MCField::styledtexttoparagraphs(MCArrayRef p_array)
{	
	if (!MCArrayIsSequence(p_array))
		return nil;

	MCParagraph *t_paragraphs;
	t_paragraphs = nil;
	parsestyledtextarray(p_array, true, t_paragraphs);

	return t_paragraphs;
}

MCParagraph *MCField::parsestyledtextappendparagraph(MCArrayRef p_style, MCNameRef p_metadata, bool p_split, MCParagraph*& x_paragraphs)
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

void MCField::parsestyledtextappendblock(MCParagraph *p_paragraph, MCArrayRef p_style, const char *p_initial, const char *p_final, MCStringRef p_metadata, bool p_is_unicode)
{
	// Make sure we don't try and append any more than 64K worth of bytes.
	uint32_t t_text_length;
	t_text_length = MCMin(p_final - p_initial, 65534 - p_paragraph -> textsize);
	
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
	
	// Set metadata
	if (p_metadata != nil)
		t_block -> setatts(P_METADATA, (void *)p_metadata);

	// Make sure the unicode flag is set.
	if (p_is_unicode)
		t_block -> flags |= F_HAS_UNICODE;
	
	// Make sure the tab flag is updated.
	if (t_block -> textstrchr(p_paragraph -> text + t_block -> index, t_block -> size, '\t'))
		t_block -> flags |= F_HAS_TAB;
	
	if (p_style == nil)
		return;

	// Set foreground
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "textColor");
	if (!ep . isempty())
	{
		MCColor t_color;
		if (MCscreen -> parsecolor(ep . getsvalue(), &t_color, nil))
			t_block -> setcolor(&t_color);
	}
	
	// Set background
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "backgroundColor");
	if (!ep . isempty())
	{
		MCColor t_color;
		if (MCscreen -> parsecolor(ep . getsvalue(), &t_color, nil))
			t_block -> setbackcolor(&t_color);
	}
	
	// Set textshift
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "textShift");
	if (!ep . isempty())
		t_block -> setshift(ep . getint4());
	
	// If the block is unicode then we always have to set textFont. In either
	// case if there is a textFont key, strip any ,unicode tag first.
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "textFont");
	if (!ep . isempty())
	{
		const char *t_font_tag;
		t_font_tag = strchr(ep . getcstring(), ',');
		if (t_font_tag != nil)
			ep . substring(0, ep . getcstring() - t_font_tag);
	}

	// Now if we have unicode, or a textFont style set it.
	if (!ep . isempty())
		t_block -> setatts(P_TEXT_FONT, (void *)ep . getcstring());
	
	// Set textsize
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "textSize");
	if (!ep . isempty())
		t_block -> setatts(P_TEXT_SIZE, (void *)ep . getint4());
	
	// Set textstyle
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "textStyle");
	if (!ep . isempty())
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
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "linkText");
	if (!ep . isempty())
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ ep . copyasstringref(&t_string);
		t_block -> setatts(P_LINK_TEXT, (void *)*t_string);
	}

	// Set imagesource
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_style, "imageSource");
	if (!ep . isempty())
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ ep . copyasstringref(&t_string);
		t_block -> setatts(P_IMAGE_SOURCE, (void *)*t_string);
	}
}

void MCField::parsestyledtextblockarray(MCArrayRef p_block_value, MCParagraph*& x_paragraphs)
{	
	// If the value is a sequence, recurse for each element.
	if (MCArrayIsSequence(p_block_value))
	{
		for(uint32_t j = 1; j <= MCArrayGetCount(p_block_value); j++)
		{
			MCValueRef t_block_entry;
			if (!MCArrayFetchValueAtIndex(p_block_value, j, t_block_entry))
				continue;

			if (!MCValueIsArray(t_block_entry))
				continue;

			parsestyledtextblockarray((MCArrayRef)t_block_entry, x_paragraphs);
		}

		return;
	}
	
	// We'll need an ep for processing.
	MCExecPoint ep(nil, nil, nil);
	
	// Fetch the style array.
	MCAutoArrayRef t_style_entry;
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_block_value, "style");
	if (!ep . isempty())
		/* UNCHECKED */ ep . copyasarrayref(&t_style_entry);

	// Get the metadata (if any)
	MCAutoStringRef t_metadata;
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_block_value, "metadata");
	if (!ep . isempty())
		/* UNCHECKED */ ep . copyasstringref(&t_metadata);
		
	// If there are no paragraphs yet, create one.
	MCParagraph *t_paragraph;
	t_paragraph = x_paragraphs -> prev();
	
	// Now loop through each newline delimited chunk in the text.
	const char *t_text_ptr;
	uint32_t t_text_length;

	// Fetch either the text entry, or the unicodeText entry and set the is unicode bool
	// appropriately.
	bool t_is_unicode;
	t_is_unicode = false;
	/* UNCHECKED */ ep . fetcharrayelement_cstring(p_block_value, "text");
	if (ep . isempty())
	{
		/* UNCHECKED */ ep . fetcharrayelement_cstring(p_block_value, "unicodeText");
		t_is_unicode = true;
	}
	if (ep . isempty() || ep . isarray())
		return;
	
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
		parsestyledtextappendblock(t_paragraph, *t_style_entry, t_text_initial_ptr, t_text_final_ptr, *t_metadata, t_is_unicode);
		
		// And, if we need a new paragraph, add it.
		if (t_add_paragraph)
			t_paragraph = parsestyledtextappendparagraph(nil, nil, true, x_paragraphs);
	}
}

void MCField::parsestyledtextarray(MCArrayRef p_styled_text, bool p_paragraph_break, MCParagraph*& x_paragraphs)
{	
	for(uint32_t i = 1; i <= MCArrayGetCount(p_styled_text); i++)
	{
		// Lookup the index - this shouldn't ever fail as its a sequence
		// but just continue gracefully if it does.
		MCValueRef t_entry;
		if (!MCArrayFetchValueAtIndex(p_styled_text, i, t_entry))
			continue;
		
		// If the entry isn't an array, just continue.
		if (!MCValueIsArray(t_entry))
			continue;
			
		// If the array is a sequence, then recurse.
		if (MCArrayIsSequence((MCArrayRef)t_entry))
		{
			parsestyledtextarray((MCArrayRef)t_entry, p_paragraph_break, x_paragraphs);
			continue;
		}
		
		// Fetch the style array.
		MCValueRef t_style_entry;
		if (!MCArrayFetchValue((MCArrayRef)t_entry, false, MCN_style, t_style_entry))
			t_style_entry = nil;

		// MW-2012-11-13: [[ ParaMetadata ]] Fetch the metadata (if any)
		MCNewAutoNameRef t_metadata;
		MCValueRef t_metadata_val = nil;
		if (!MCArrayFetchValue((MCArrayRef)t_entry, false, MCN_metadata, t_metadata_val))
			t_metadata_val = nil;
		if (t_metadata_val != nil)
		{
			MCExecPoint ep;
			ep.setvalueref(t_metadata_val);
			/* UNCHECKED */ ep.copyasnameref(&t_metadata);
		}

		// If the array looks like a paragraph array, then treat it as such.
		MCValueRef t_runs_entry;
		if (!MCArrayFetchValue((MCArrayRef)t_entry, false, MCN_runs, t_runs_entry))
			t_runs_entry = nil;

		if (t_runs_entry != nil)
		{
			// To continue, the value must either be a sequence-style array or
			// empty.
			MCArrayRef t_runs_array;
			if (MCValueIsArray(t_runs_entry))
			{
				t_runs_array = (MCArrayRef)t_runs_entry;
				if (!MCArrayIsSequence(t_runs_array))
					continue;
			}
			else if (!MCValueIsEmpty(t_runs_entry))
				continue;
			else
				t_runs_array = nil;
			
			// Begin paragraph with style
			if (t_style_entry != nil && MCValueIsArray(t_style_entry))
				parsestyledtextappendparagraph((MCArrayRef)t_style_entry, *t_metadata, false, x_paragraphs);
			else
				parsestyledtextappendparagraph(nil, *t_metadata, false, x_paragraphs);
					
			// Finally, we are a sequence so loop through all the elements.
			if (t_runs_array != nil)
				for(uint32_t j = 1; j <= MCArrayGetCount(t_runs_array); j++)
				{
					MCValueRef t_block_entry;
					if (!MCArrayFetchValueAtIndex(t_runs_array, j, t_block_entry))
						continue;
					
					if (!MCValueIsArray(t_block_entry))
						continue;

					parsestyledtextblockarray((MCArrayRef)t_block_entry, x_paragraphs);
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
		parsestyledtextblockarray((MCArrayRef)t_entry, x_paragraphs);
	}
}

////////////////////////////////////////////////////////////////////////////////
