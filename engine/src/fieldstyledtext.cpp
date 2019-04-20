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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "field.h"
#include "paragraf.h"
#include "cdata.h"
#include "mcerror.h"

#include "exec.h"
#include "util.h"
#include "MCBlock.h"

#include "globals.h"
#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

// Context for styled text export.
struct export_styled_text_t
{
    //MCExecContext ctxt;
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
static void export_styled_text_paragraph_style(MCArrayRef p_style_array, const MCFieldParagraphStyle& p_style, bool p_effective)
{
	if (p_style . has_text_align || p_effective)
	{
        MCAutoValueRef t_value;
		MCF_unparsetextatts(P_TEXT_ALIGN, p_style . text_align << F_ALIGNMENT_SHIFT, nil, 0, 0, 0, &t_value);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textAlign"), *t_value);
	}
	if (p_style . has_list_style)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("listStyle"), MCSTR(MCliststylestrings[p_style . list_style]));
    
		// MW-2012-02-22: [[ Bug ]] The listDepth property is stored internally as depth - 1, so adjust.
        MCAutoNumberRef t_depth;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . list_depth + 1, &t_depth);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("listDepth"), *t_depth);
		if (p_style . has_list_indent)
		{
            MCAutoNumberRef t_indent;
            /* UNCHECKED */ MCNumberCreateWithInteger(p_style . list_indent, &t_indent);
            /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("listIndent"), *t_indent);
		}
	}
	if (!p_style . has_list_indent && (p_style . has_first_indent || p_effective))
	{
        MCAutoNumberRef t_first_indent;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . first_indent, &t_first_indent);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("firstIndent"), *t_first_indent);
	}
	if (p_style . has_left_indent || p_effective)
	{
        MCAutoNumberRef t_left_indent;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . left_indent, &t_left_indent);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("leftIndent"), *t_left_indent);
	}
	if (p_style . has_list_index)
	{
        MCAutoNumberRef t_index;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . list_index, &t_index);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("listIndex"), *t_index);
	}
	if (p_style . has_right_indent || p_effective)
	{
        MCAutoNumberRef t_right_indent;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . right_indent, &t_right_indent);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("rightIndent"), *t_right_indent);
	}
	if (p_style . has_space_above || p_effective)
	{
        MCAutoNumberRef t_space_above;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . space_above, &t_space_above);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("spaceAbove"), *t_space_above);
	}
	if (p_style . has_space_below || p_effective)
	{
        MCAutoNumberRef t_space_below;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . space_below, &t_space_below);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("spaceBelow"), *t_space_below);
	}
	if (p_style . has_tabs || p_effective)
	{
        MCAutoStringRef t_string;
		MCField::formattabstops(P_TAB_STOPS, p_style . tabs, p_style . tab_count, &t_string);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("tabStops"), *t_string);
	}
	if (p_style . has_tab_alignments || p_effective)
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCField::formattabalignments(p_style . tab_alignments, p_style . tab_alignment_count, &t_string);
		/* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("tabAlign"), *t_string);
	}
	if (p_style . has_background_color)
	{
        MCAutoStringRef t_string;
        MCColor t_color;
        MCColorSetPixel(t_color, p_style . background_color);
        /* UNCHECKED */ MCU_format_color(t_color, &t_string);
		/* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("backgroundColor"), *t_string);
	}
	if (p_style . has_border_width || p_effective)
	{
        MCAutoNumberRef t_border_width;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . border_width, &t_border_width);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("borderWidth"), *t_border_width);
    }
	if (p_style . has_hgrid || p_effective)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("hGrid"), p_style . hgrid == True ? kMCTrue : kMCFalse);
	}
	if (p_style . has_vgrid || p_effective)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("vGrid"), p_style . vgrid == True ? kMCTrue : kMCFalse);
	}
	if (p_style . has_border_color || p_effective)
	{
        MCAutoStringRef t_string;
        MCColor t_color;
        MCColorSetPixel(t_color, p_style . border_color);
        /* UNCHECKED */ MCU_format_color(t_color, &t_string);
		/* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("borderColor"), *t_string);
	}
	if (p_style . has_dont_wrap || p_effective)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("dontWrap"), p_style . dont_wrap == True ? kMCTrue : kMCFalse);
	}
	if (p_style . has_padding || p_effective)
	{
        MCAutoNumberRef t_padding;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . padding, &t_padding);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("padding"), *t_padding);
    }
    
    if (p_style . hidden  || p_effective)
    {
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("hidden"), p_style . hidden == True ? kMCTrue : kMCFalse);
    }
}


// Copy the styles from the struct into the array.
static void export_styled_text_character_style(MCArrayRef p_style_array, const MCFieldCharacterStyle& p_style, bool p_effective)
{
	if (p_style . has_text_color || p_effective)
	{
        MCAutoStringRef t_string;
        MCColor t_color;
        MCColorSetPixel(t_color, p_style . text_color);
        /* UNCHECKED */ MCU_format_color(t_color, &t_string);
		/* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textColor"), *t_string);
	}
	if (p_style . has_background_color)
	{
        MCAutoStringRef t_string;
        MCColor t_color;
        MCColorSetPixel(t_color, p_style . background_color);
        /* UNCHECKED */ MCU_format_color(t_color, &t_string);
		/* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("backgroundColor"), *t_string);
	}
	if (p_style . has_link_text)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("linkText"), p_style . link_text);
	}
	if (p_style . has_image_source)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("imageSource"), p_style . image_source);
	}
	if (p_style . has_text_font || p_effective)
	{
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textFont"), p_style . text_font);
	}
	if (p_style . has_text_style || p_effective)
	{
        MCAutoValueRef t_value;
		MCF_unparsetextatts(P_TEXT_STYLE, 0, nil, 0, 0, p_style . text_style, &t_value);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textStyle"), *t_value);
	}
	if (p_style . has_text_size || p_effective)
	{
        MCAutoNumberRef t_text_size;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . text_size, &t_text_size);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textSize"), *t_text_size);
	}
	if (p_style . has_text_shift)
	{
        MCAutoNumberRef t_text_shift;
        /* UNCHECKED */ MCNumberCreateWithInteger(p_style . text_shift, &t_text_shift);
        /* UNCHECKED */ MCArrayStoreValue(p_style_array, true, MCNAME("textShift"), *t_text_shift);
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
			export_styled_text_paragraph_style(*t_paragraph_style_array, p_event_data . paragraph_style, ctxt . effective);
            /* UNCHECKED */ MCArrayStoreValue(ctxt . paragraph_array, true, MCNAME("style"), *t_paragraph_style_array);
		}

		// MW-2012-11-13: [[ ParaMetadata ]] If the paragraph has metadata, then apply it.
		if (p_event_data . has_paragraph_style && p_event_data . paragraph_style . has_metadata)
		{
            /* UNCHECKED */ MCArrayStoreValue(ctxt . paragraph_array, true, MCNAME("metadata"), p_event_data . paragraph_style . metadata);
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
            /* UNCHECKED */ MCArrayStoreValueAtIndex(ctxt . runs_array, ctxt . run_index, ctxt . last_run);
			MCValueRelease(ctxt . last_run);
			ctxt . last_run = nil;
		}

        /* UNCHECKED */ MCArrayStoreValue(ctxt . paragraph_array, true, MCNAME("runs"), ctxt . runs_array);
		MCValueRelease(ctxt . runs_array);
		ctxt . runs_array = nil;

        /* UNCHECKED */ MCArrayStoreValueAtIndex(ctxt . paragraphs_array, ctxt . paragraph_index, ctxt . paragraph_array);
		MCValueRelease(ctxt . paragraph_array);
		ctxt . paragraph_array = nil;
	}
	else if (p_event_type == kMCFieldExportEventNativeRun || p_event_type == kMCFieldExportEventUnicodeRun)
	{
		if (ctxt . last_run != nil)
		{
            /* UNCHECKED */ MCArrayStoreValueAtIndex(ctxt . runs_array, ctxt . run_index, ctxt . last_run);
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
        // FG-2014-10-21: [[ Bugfix 13742 ]] Only export as "text" in 7.0+
		MCAutoStringRef t_string;
		/* UNCHECKED */ MCStringCopySubstring(p_event_data.m_text, p_event_data.m_range, &t_string);
        /* UNCHECKED */ MCArrayStoreValue(ctxt . last_run, true, MCNAME("text"), *t_string);

		// Set the block's 'metadata' entry in the block's run array.
		if (p_event_data . character_style . has_metadata)
		{
            MCArrayStoreValue(ctxt . last_run, true, MCNAME("metadata"), p_event_data . character_style . metadata);
		}

		// Now if the block has any attributes then build the array.
		if (p_event_data . has_character_style || ctxt . effective)
		{
			MCAutoArrayRef t_run_style_array;
			/* UNCHECKED */ MCArrayCreateMutable(&t_run_style_array);
			export_styled_text_character_style(*t_run_style_array, p_event_data . character_style, ctxt . effective);
            
            MCArrayStoreValue(ctxt . last_run, true, MCNAME("style"), *t_run_style_array);
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
        
        MCValueRef t_value;
        MCNameRef t_key;
        t_key = MCNAME("text");
        MCAutoStringRef t_new_string;
        MCArrayFetchValue(ctxt . last_run, true, t_key, t_value);
        
        MCStringFormat(&t_new_string, "%@\x0b", (MCStringRef)t_value);
        
        /* UNCHECKED */ MCArrayStoreValue(ctxt . last_run, true, t_key, *t_new_string);
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
bool MCField::exportasstyledtext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective, MCArrayRef &r_array)
{
    return exportasstyledtext(resolveparagraphs(p_part_id), p_start_index, p_finish_index, p_formatted, p_effective, r_array);
}

bool MCField::exportasstyledtext(MCParagraph* p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_formatted, bool p_effective, MCArrayRef &r_array)
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
		doexport(t_flags, p_paragraphs, p_start_index, p_finish_index, export_styled_text, &t_context);

		if (MCArrayCopyAndRelease(t_context . paragraphs_array, r_array))
			return true;
        
        MCValueRelease(t_context.paragraphs_array);
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
MCParagraph *MCField::styledtexttoparagraphs(MCArrayRef p_array)
{	
	if (!MCArrayIsSequence(p_array))
		return nil;

	MCParagraph *t_paragraphs;
	t_paragraphs = nil;
	parsestyledtextarray(p_array, true, t_paragraphs);

	return t_paragraphs;
}

MCParagraph *MCField::parsestyledtextappendparagraph(MCArrayRef p_style, MCStringRef p_metadata, bool p_split, MCParagraph*& x_paragraphs)
{
	MCParagraph *t_new_paragraph;
	t_new_paragraph = new (nothrow) MCParagraph;
	t_new_paragraph -> setparent(this);
	t_new_paragraph -> inittext();
	
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

static bool convert_array_value_to_number_if_non_empty(MCExecContext& ctxt, MCArrayRef p_array, MCNameRef p_key, MCNumberRef &r_value)
{
	MCValueRef t_value;
	if (!MCArrayFetchValue(p_array, false, p_key, t_value))
		return false;
	if (MCValueIsEmpty(t_value))
		return false;
	if (!ctxt . ConvertToNumber(t_value, r_value))
		return false;
	return true;
} 

static bool copy_element_as_color_if_non_empty(MCExecContext& ctxt, MCArrayRef array, MCNameRef key, MCColor& r_value)
{
	MCValueRef t_value;
	if (!MCArrayFetchValue(array, false, key, t_value))
		return false;
	if (MCValueIsEmpty(t_value))
		return false;
	MCAutoStringRef t_string;
	if (!ctxt . ConvertToString(t_value, &t_string))
		return false;
	return MCscreen -> parsecolor(*t_string, r_value, nil);
}

void MCField::parsestyledtextappendblock(MCParagraph *p_paragraph, MCArrayRef p_style, MCStringRef p_string, MCStringRef p_metadata)
{
	// Do nothing if there is no text to add
	if (MCStringIsEmpty(p_string))
        return;

    MCExecContext ctxt(nil, nil, nil);

	// Create a block for the text we wish to append
	MCBlock *t_block = p_paragraph->AppendText(p_string);
		
	// Now set the block styles.
	
	// Set metadata
	if (p_metadata != nil)
        t_block -> SetMetadata(ctxt, p_metadata);

	if (p_style == nil)
		return;

	MCValueRef t_valueref;
    t_valueref = nil;

	// Set foreground
	{
		MCColor t_color;
		if (copy_element_as_color_if_non_empty(ctxt, p_style, MCNAME("textColor"), t_color))
			t_block -> setcolor(&t_color);
	}
	
	// Set background
	{
		MCColor t_color;
		if (copy_element_as_color_if_non_empty(ctxt, p_style, MCNAME("backgroundColor"), t_color))
			t_block -> setbackcolor(&t_color);
	}
	// Set textshift
	{
		MCAutoNumberRef t_number;
		if (convert_array_value_to_number_if_non_empty(ctxt, p_style, MCNAME("textShift"), &t_number))
            t_block -> setshift(MCNumberFetchAsInteger(*t_number));
	}

    MCAutoStringRef t_font;
	// If the block is unicode then we always have to set textFont. In either
	// case if there is a textFont key, strip any ,unicode tag first.
	if (MCArrayFetchValue(p_style, false, MCNAME("textFont"), t_valueref) && !MCValueIsEmpty(t_valueref))
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ ctxt . ConvertToString(t_valueref, &t_string);
		uindex_t t_comma;
		if (MCStringFirstIndexOfChar(*t_string, ',', 0, kMCCompareExact, t_comma))
			MCStringCopySubstring(*t_string, MCRangeMake(0, t_comma), &t_font);
        else
            t_font = *t_string;
	}

	// Now if we have unicode, or a textFont style set it.
	if (*t_font != nil)
        t_block -> SetTextFont(ctxt, *t_font);
	
	// Set textsize
	{
		MCAutoNumberRef t_number;
        uinteger_t t_size;
		if (convert_array_value_to_number_if_non_empty(ctxt, p_style, MCNAME("textSize"), &t_number))
        {
            t_size = MCNumberFetchAsUnsignedInteger(*t_number);
            t_block -> SetTextSize(ctxt, &t_size);
        }
	}
	
	// Set textstyle
	if (MCArrayFetchValue(p_style, false, MCNAME("textStyle"), t_valueref) && !MCValueIsEmpty(t_valueref))
	{
		MCAutoStringRef t_string;
		/* UNCHECKED */ ctxt . ConvertToString(t_valueref, &t_string);
		uint4 flags;
		MCAutoStringRef fname;
		uint2 height;
		uint2 size;
        uint2 style;

		MCF_parsetextatts(P_TEXT_STYLE, *t_string, flags, &fname, height, size, style);

        MCInterfaceTextStyle t_style;
        t_style . style = style;
        t_block -> SetTextStyle(ctxt, t_style);
	}

	// Set linktext
	if (MCArrayFetchValue(p_style, false, MCNAME("linkText"), t_valueref))
	{	
        MCAutoStringRef t_string;
        if (ctxt . ConvertToString(t_valueref, &t_string))
            t_block -> SetLinktext(ctxt, *t_string);
	}

	// Set imagesource
	if (MCArrayFetchValue(p_style, false, MCNAME("imageSource"), t_valueref))
	{
        MCAutoStringRef t_string;
        if (ctxt . ConvertToString(t_valueref, &t_string))
            t_block -> SetImageSource(ctxt, *t_string);
	}

}

void MCField::parsestyledtextblockarray(MCArrayRef p_block_value, MCParagraph*& x_paragraphs)
{
    MCExecContext ctxt(nil, nil, nil);
	// If the value is a sequence, recurse for each element.
	if (MCArrayIsSequence(p_block_value))
	{
		for(uint32_t j = 1; j <= MCArrayGetCount(p_block_value); j++)
		{
			MCValueRef t_block_entry;
			if (!MCArrayFetchValueAtIndex(p_block_value, j, t_block_entry))
				continue;

			MCAutoArrayRef t_array;
			if (ctxt . ConvertToArray(t_block_entry, &t_array))
				parsestyledtextblockarray(*t_array, x_paragraphs);		
		}
		return;
	}
	
	MCValueRef t_valueref;
    t_valueref = nil;
	MCAutoArrayRef t_style_entry;   

	// Set foreground
    if (MCArrayFetchValue(p_block_value, false, MCNAME("style"), t_valueref) && !MCValueIsEmpty(t_valueref))
	{
		MCArrayRef t_array;
		if (ctxt . ConvertToArray(t_valueref, t_array))
            /* UNCHECKED */ MCArrayCopyAndRelease(t_array, &t_style_entry);
	}
	// Get the metadata (if any)
	MCAutoStringRef t_metadata;
	if (MCArrayFetchValue(p_block_value, false, MCNAME("metadata"), t_valueref) && !MCValueIsEmpty(t_valueref))
	{
		MCAutoStringRef t_string;
		if (ctxt . ConvertToString(t_valueref, &t_string))
            t_metadata = *t_string;
	}	
	// If there are no paragraphs yet, create one.
	MCParagraph *t_paragraph;
	t_paragraph = x_paragraphs -> prev();
	
	// Now loop through each newline delimited chunk in the text.
	uint32_t t_text_length;
    uindex_t t_start_index;

	if (!MCArrayFetchValue(p_block_value, false, MCNAME("text"), t_valueref) || MCValueIsEmpty(t_valueref))
		/* UNCKECKED */ MCArrayFetchValue(p_block_value, false, MCNAME("unicodeText"), t_valueref);
    
	if (MCValueIsEmpty(t_valueref) || MCValueGetTypeCode(t_valueref) == kMCValueTypeCodeArray)
		return;
	
	MCAutoStringRef t_temp;
	/* UNCHECKED */ ctxt . ConvertToString(t_valueref, &t_temp);
    
    t_start_index = 0;
	t_text_length = MCStringGetLength(*t_temp);
	while(t_start_index < t_text_length)
	{
		bool t_add_paragraph;
		uindex_t t_text_initial_start_index;
		uindex_t t_text_end_index;
		t_text_initial_start_index = t_start_index;
        
		if (MCStringFirstIndexOfChar(*t_temp, '\n', t_start_index, kMCStringOptionCompareExact, t_text_end_index))
		{
			t_start_index = t_text_end_index + 1;
			t_add_paragraph = true;
		}
		else
		{
			t_start_index += t_text_length;
			t_text_end_index = t_start_index;
			t_add_paragraph = false;
		}

		// We now add the range initial...final as a block.
        MCAutoStringRef t_substring;
        MCStringCopySubstring(*t_temp, MCRangeMakeMinMax(t_text_initial_start_index, t_text_end_index), &t_substring);
		parsestyledtextappendblock(t_paragraph, *t_style_entry, *t_substring, *t_metadata);

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
        MCAutoStringRef t_metadata;
		MCValueRef t_metadata_val = nil;
		if (!MCArrayFetchValue((MCArrayRef)t_entry, false, MCN_metadata, t_metadata_val))
			t_metadata_val = nil;
		if (t_metadata_val != nil)
        {
            MCExecContext ctxt(nil, nil, nil);
            /* UNCHECKED */ ctxt . ConvertToString(t_metadata_val, &t_metadata);
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
				if (!MCArrayIsEmpty(t_runs_array) && !MCArrayIsSequence(t_runs_array))
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
