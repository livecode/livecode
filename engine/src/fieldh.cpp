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

#include "util.h"
#include "MCBlock.h"
#include "line.h"
#include "globals.h"
#include "text.h"
#include "osspec.h"

//#include "textbuffer.h"
#include "variable.h"
#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

// Context structure for computing paragraph numbering.
struct paragraph_numbering_t
{
	// True if we have a liststyle at some depth.
	bool seen_list_style;
	// The current list styles at each depth (if any).
	uint8_t list_styles[16];
	// The number applying to each depth at the moment.
	uint32_t list_indices[16];
};

// This method updates the numbering context for a paragraph with the given style
// and depth, returning its number or 0 if it has none.
static uint32_t compute_paragraph_number(paragraph_numbering_t& ctxt, uint32_t p_list_style, uint32_t p_list_depth, uint32_t p_list_index)
{
	// If the paragraph doesn't have a listStyle, then it terminates any current
	// lists.
	if (p_list_style == kMCParagraphListStyleNone)
	{
		if (!ctxt . seen_list_style)
			return true;

		memset(ctxt . list_styles, 0, sizeof(ctxt . list_styles));
		ctxt . seen_list_style = false;

		return 0;
	}

	// Terminate any lists whose depth is greater than the current paragraph.
	if (ctxt . seen_list_style)
	{
		memset(ctxt . list_styles + p_list_depth + 1, 0, sizeof(ctxt . list_styles) - p_list_depth + 1);
		ctxt . seen_list_style = false;
	}

	// If the list is of 'skip' style or non numbered, then set the number to 0 (no label).
	if (p_list_style < kMCParagraphListStyleNumeric || p_list_style == kMCParagraphListStyleSkip)
		return 0;

	// If the list style setting is different, we reset the numbering.
	if (ctxt . list_styles[p_list_depth] != p_list_style)
		ctxt . list_indices[p_list_depth] = 0;

	// Finally, the number of this paragraph is 1 + the previous one at this
	// depth if the index is 0, otherwise it is the index.
	ctxt . list_styles[p_list_depth] = p_list_style;
	if (p_list_index == 0)
		ctxt . list_indices[p_list_depth] += 1;
	else
		ctxt . list_indices[p_list_depth] = p_list_index;
	ctxt . seen_list_style = true;

	return ctxt . list_indices[p_list_depth];
}

// This method checks to see if two flattened character styles are the same.
static bool flattened_character_style_is_equal(MCFieldCharacterStyle& left, MCFieldCharacterStyle& right)
{
	if (left . text_color != right . text_color)
		return false;
	if (left . background_color != right . background_color)
		return false;
	if (left . link_text != right . link_text)
		return false;
	if (left . image_source != right . image_source)
		return false;
	if (left . metadata != right . metadata)
		return false;
	if (left . text_font != right . text_font)
		return false;
	if (left . text_style != right . text_style)
		return false;
	if (left . text_size != right . text_size)
		return false;
	if (left . text_shift != right . text_shift)
		return false;
	return true;
}

// This method is a wrapper around the main doexport function which resolves the appropriate field
// paragraphs first.
bool MCField::doexport(MCFieldExportFlags p_flags, uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCFieldExportCallback p_callback, void *p_context)
{
	return doexport(p_flags, resolveparagraphs(p_part_id), p_start_index, p_finish_index, p_callback, p_context);
}

// This method is the main work-horse of export. It iterates over the given paragraphs between start
// and finish, invoking the callback for each and every requested event.
bool MCField::doexport(MCFieldExportFlags p_flags, MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, MCFieldExportCallback p_callback, void *p_context)
{
	// If the null range is requested, only export if there are attributes.
    bool t_empty_range;
    t_empty_range = p_start_index == p_finish_index;

	// Fetch the paragraphs
	MCParagraph *t_paragraphs;
	t_paragraphs = p_paragraphs;

	// The first and last boundaries of the export.
	MCParagraph *t_first_paragraph;
	findex_t t_first_offset, t_last_offset;
	t_first_offset = p_start_index;
	t_last_offset = p_finish_index;

	// Fetch the paragraph and offset for the first index.
	if (t_first_offset != 0)
		t_first_paragraph = indextoparagraph(t_paragraphs, t_first_offset, t_last_offset);
	else
		t_first_paragraph = t_paragraphs;
	
	// Make sure the indices are rounded to the nearest char.
	verifyindex(t_first_paragraph, t_first_offset, false);

	// If we are collecting paragraph styles and flattening then fetch the inherited
	// styles.
	MCFieldParagraphStyle t_inherited_paragraph_style;
	if ((p_flags & kMCFieldExportParagraphStyles) != 0 &&
		(p_flags & kMCFieldExportFlattenStyles) != 0)
	{
		memset(&t_inherited_paragraph_style, 0, sizeof(MCFieldParagraphStyle));
		t_inherited_paragraph_style . text_align = (getflags() & F_ALIGNMENT) >> F_ALIGNMENT_SHIFT;
		t_inherited_paragraph_style . vgrid = getflag(F_VGRID);
		t_inherited_paragraph_style . hgrid = getflag(F_HGRID);
		t_inherited_paragraph_style . dont_wrap = getflag(F_DONT_WRAP);
		t_inherited_paragraph_style . first_indent = indent;
		t_inherited_paragraph_style . tab_count = ntabs;
		t_inherited_paragraph_style . tabs = tabs;
		t_inherited_paragraph_style . tab_alignment_count = nalignments;
		t_inherited_paragraph_style . tab_alignments = alignments;
		t_inherited_paragraph_style . border_color = getcoloraspixel(DI_BORDER);
	}

	// If we are collecting character styles and flattening then fetch the inherited
	// styles.
	MCFieldCharacterStyle t_inherited_character_style;
	if ((p_flags & kMCFieldExportCharacterStyles) != 0 &&
		(p_flags & kMCFieldExportFlattenStyles) != 0)
	{
		memset(&t_inherited_character_style, 0, sizeof(MCFieldCharacterStyle));
		t_inherited_character_style . text_color = getcoloraspixel(DI_FORE);
		t_inherited_character_style . link_text = kMCEmptyString;
		t_inherited_character_style . image_source = kMCEmptyString;
		t_inherited_character_style . metadata = kMCEmptyString;
		getfontattsnew(t_inherited_character_style . text_font, t_inherited_character_style . text_size, t_inherited_character_style . text_style);
	}

	// Initialize the event data.
	MCFieldExportEventData t_data;
	t_data . m_text = nil;
	t_data . m_range = MCRangeMake(0, 0);
	t_data . paragraph_number = 0;
	t_data . is_first_paragraph = true;
	t_data . is_last_paragraph = false;

	// Initialize the paragraph numbering data if needed - including computing the
	// number for the paragraph preceeding the first one (but only if offset == 0,
	// since we don't include paragraph styles with paragraphs starting away from
	// their beginning).
	paragraph_numbering_t t_numbering;
	if ((p_flags & kMCFieldExportNumbering) != 0)
	{
		memset(&t_numbering, 0, sizeof(paragraph_numbering_t));

		if (t_first_offset == 0 && t_first_paragraph != t_paragraphs)
			for(MCParagraph *t_paragraph = t_paragraphs; t_paragraph != t_first_paragraph -> prev(); t_paragraph = t_paragraph -> next())
				compute_paragraph_number(t_numbering, t_paragraph -> getliststyle(), t_paragraph -> getlistdepth(), t_paragraph -> getlistindex());
	}

    // Abort if this is the empty range and there are no paragraph attributes
    if (t_empty_range && !(t_first_paragraph->hasattrs()))
        return true;
    
	// Now loop through the paragraphs, starting at the one that has been
	// identified as the first.
	MCParagraph *t_paragraph;
	t_paragraph = t_first_paragraph;
	for(;;)
	{
		t_data . is_last_paragraph = t_last_offset <= t_paragraph -> gettextlength() || t_paragraph -> next() == t_paragraphs;

		// Compute the numbering (if required).
		if ((p_flags & kMCFieldExportNumbering) != 0 && t_first_offset == 0)
			t_data . paragraph_number = compute_paragraph_number(t_numbering, t_paragraph -> getliststyle(), t_paragraph -> getlistdepth(), t_paragraph -> getlistindex());

		// If we want to notify about paragraphs, do so.
		if ((p_flags & kMCFieldExportParagraphs) != 0)
		{
			if ((p_flags & kMCFieldExportParagraphStyles) != 0)
			{
				t_data . has_paragraph_style = false;

				// If we are flattening styles, then copy the inherited ones.
				if ((p_flags & kMCFieldExportFlattenStyles) != 0)
					t_data . paragraph_style = t_inherited_paragraph_style;
				else
					memset(&t_data . paragraph_style, 0, sizeof(MCFieldParagraphStyle));

				// MW-2013-02-21: [[ Bug 10685 ]] Originally the paragraph properties were only
				//   applied if the first offset is 0. This seems unreasonable so have adjusted.
				if ((p_flags & kMCFieldExportParagraphStyles) != 0 && t_paragraph -> hasattrs())
				{
					t_data . has_paragraph_style = true;
					t_paragraph -> exportattrs(t_data . paragraph_style);
				}
			}
			
			// Reset the character styles to defaults so we know the inherited
			// char attrs at the begin paragraph event.
			if ((p_flags & kMCFieldExportCharacterStyles) != 0)
			{
				t_data . has_character_style = false;
				
				if ((p_flags & kMCFieldExportFlattenStyles) != 0)
					t_data . character_style = t_inherited_character_style;
				else
					memset(&t_data . character_style, 0, sizeof(MCFieldCharacterStyle));
			}

			// Generate a paragraph begin event.
			if (!p_callback(p_context, kMCFieldExportEventBeginParagraph, t_data))
				return false;
		}
		
		// Setup line processing (if possible).
		MCLine *t_line;
		t_line = nil;
		if ((p_flags & kMCFieldExportLines) != 0)
		{
			// Fetch the paragraph's lines.
			MCLine *t_lines;
			t_lines = t_paragraph -> getlines();
			if (t_lines != nil)
			{
				// The line we start off with is the first one.
				t_line = t_lines;
				
				// Unless first offset is not 0, in which case we have to loop through
				// to find the line containing it.
				if (t_first_offset != 0)
					for(;;)
					{
						MCBlock *t_first, *t_last;
						t_line -> getblocks(t_first, t_last);

						// If the last index in the block is after first offset, then this
						// must be the line.
						if (t_last -> GetOffset() + t_last -> GetLength() > t_first_offset)
							break;
							
						// MW-2013-09-02: [[ Bug 11144 ]] Make sure we advance to the next line,
						//   otherwise we just get an infinite loop.
						t_line = t_line -> next();
					}
			}
		}

		// If the paragraph has text, then we must process its blocks.
		bool t_last_block;
		if ((p_flags & kMCFieldExportRuns) && !t_paragraph -> IsEmpty())
		{
			// Fetch the paragraph's blocks - if there are none, make sure we initialize
			// the paragraph using inittext().
			MCBlock *t_blocks;
			t_blocks = t_paragraph -> getblocks();
			if (t_blocks == nil)
			{
				t_paragraph -> inittext();
				t_blocks = t_paragraph -> getblocks();
			}
			
			// Find the block containing the first index.
			MCBlock *t_block;
			t_block = t_blocks;
			while(t_first_offset > t_block -> GetOffset() + t_block -> GetLength())
				t_block = t_block -> next();

			// Now loop through all the blocks, until we reach one containing
			// last offset (or the end of the list for the paragraph).
			bool t_last_block_on_line;
			t_last_block_on_line = false;
			for(;;)
			{
				// Skip any null blocks (as long as we are not on the last one).
				while(t_block -> next() != t_blocks && t_block -> GetLength() == 0)
					t_block = t_block -> next();

				// Work out how far the 'virtual' block (a sequence of blocks with the
				// same attrs - depending on export type).
				int32_t t_start, t_count;
				t_start = MCMax(t_first_offset, t_block -> GetOffset());
				t_count = t_block -> GetOffset() + t_block -> GetLength() - t_start;
	
				// If we want run styles, then update the array.
				if (t_block -> GetLength() != 0 && (p_flags & kMCFieldExportCharacterStyles) != 0)
				{
					t_data . has_character_style = false;

					// If this block is not the first in the paragraph, then reset the
					// attrs.
					if (t_start != 0)
					{
						// If we are flattening styles, then copy the inherited ones.
						if ((p_flags & kMCFieldExportFlattenStyles) != 0)
							t_data . character_style = t_inherited_character_style;
						else
							memset(&t_data . character_style, 0, sizeof(MCFieldCharacterStyle));
					}
					
					// If the block has attrs to apply, do so.
					if (t_block -> hasatts())
					{
						t_data . has_character_style = true;
						t_block -> exportattrs(t_data . character_style);
					}
				}
			
				// Notionally merge together blocks with the same attrs depending
				// on the export type.
				for(;;)
				{
					// Compute the next (non empty) block.
					MCBlock *t_next_block;
					t_next_block = t_block -> next();
					while(t_next_block != t_blocks && t_next_block -> GetLength() == 0)
						t_next_block = t_next_block -> next();

					// Check to see if we are the last block: either the block
					// containing last_offset, or the block at the end of the list.
					if (t_last_offset <= t_block -> GetOffset() + t_block -> GetLength())
						t_last_block = true;
					else if (t_next_block == t_blocks)
						t_last_block = true;
					else
						t_last_block = false;

					// If we have a line, and this block is the last on the line, we
					// are done.
					if (t_line != nil)
					{
						MCBlock *t_first, *t_last;
						t_line -> getblocks(t_first, t_last);

						// If this block is the last on the line, then set the flag
						// and move to the next line.
						if (t_block == t_last)
						{
							t_line = t_line -> next();
							t_last_block_on_line = true;
							break;
						}
					}
						
					// If we are the last block, there isn't a 'next block' to
					// compare with so we are done.
					if (t_last_block)
						break;

					// If the block is not of zero size, then compare its style.
					if (t_block -> GetLength() != 0)
					{
						// If we are processing with styles, then we need to compare all
						// the attrs; otherwise we just need check the unicode flag.
						if ((p_flags & kMCFieldExportCharacterStyles) != 0)
						{
							// If the styles are being flattened, flatten the next block
							// and compare those.
							if ((p_flags & kMCFieldExportFlattenStyles) != 0)
							{
								MCFieldCharacterStyle t_next_block_style;
								t_next_block_style = t_inherited_character_style;
								t_next_block -> exportattrs(t_next_block_style);

								if (!flattened_character_style_is_equal(t_data . character_style, t_next_block_style))
									break;
							}
							else if (!t_block -> sameatts(t_next_block, true))
								break;
						}
						
						// The length of the run goes up to the end of the next block.
						t_count += t_next_block -> GetLength();
					}
					
					// Advance to the next (non-empty) block.
					t_block = t_next_block;
				}
			
				// Make sure we don't emit more than last offset allows.
				t_count = MCMin(t_last_offset - t_start, t_count);
			
				// Emit the block.
				t_data . m_text = t_paragraph->GetInternalStringRef();
				t_data . m_range = MCRangeMake(t_start, t_count);
				
				// If we are the last block on the line, trim any VTAB
				bool t_explicit_line_break;
				t_explicit_line_break = false;
				if (t_last_block_on_line && t_count > 0)
				{
					// MW-2012-03-16: [[ Bug ]] Only trim the end if we have an explicit line break.
					// TODO: TextIsExplicitLineBreak
					if (t_paragraph -> GetCodepointAtIndex(t_start + t_count - 1) == '\v')
						t_explicit_line_break = true, t_data . m_range . length -= 1;
				}
				
                MCFieldExportEventType t_export_type;
                if (MCStringIsNative(t_data . m_text))
                    t_export_type = kMCFieldExportEventNativeRun;
                else
                    t_export_type = kMCFieldExportEventUnicodeRun;
                
				if (!p_callback(p_context, t_export_type, t_data))
					return false;

				// If we just processed the last block on the line then emit a line break.
				// Note that we only emit a line break after the last block on a line if
				// it was explicit.
				if (t_last_block_on_line && (!t_last_block || t_explicit_line_break))
				{
					if (!p_callback(p_context, kMCFieldExportEventLineBreak, t_data))
						return false;
					
					// We are no longer the last block on the line.
					t_last_block_on_line = false;
				}
				
				// If we just processed the last block, we are done.
				if (t_last_block)
					break;
				
				// Advance to the next block.
				t_block = t_block -> next();
			}
		}
		else
			t_last_block = false;
		
		// Generate a paragraph end event.
		if ((p_flags & kMCFieldExportParagraphs) != 0)
			if (!p_callback(p_context, kMCFieldExportEventEndParagraph, t_data))
				return false;

		if (t_data . is_last_paragraph)
			break;
		
		// We are no longer the first paragraph.
		t_data . is_first_paragraph = false;

		// Update the first / last indices we want.
		t_first_offset = 0;
        t_last_offset -= t_paragraph -> gettextlengthcr();

		// Advance to the next paragraph.
		t_paragraph = t_paragraph -> next();
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// This method calculates the size of the text content of the field when encoded
// as native.
static bool estimate_char_count(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	uint32_t& t_count = *(uint32_t *)p_context;
	
	// TODO: the kMCFieldExportEvent{Unicode,Native}Run constants should be merged
	if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		t_count += p_event_data . m_range . length;
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		t_count += 1;
		
	return true;
}

// This method extracts the text content of the field.
static bool export_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	MCStringRef t_buffer = (MCStringRef)p_context;
	
	if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		/* UNCHECKED */ MCStringAppendSubstring(t_buffer, p_event_data.m_text, p_event_data.m_range);
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		/* UNCHECKED */ MCStringAppendChar(t_buffer, '\n');

	return true;
}

// This method appends a list label to the buffer.
static void append_list_label(MCStringRef p_buffer, uint32_t p_list_depth, uint32_t p_list_style, uint32_t p_paragraph_number)
{
	static const char s_tabs[16] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

	/* UNCHECKED */ MCStringAppendFormat(p_buffer, "%.*s", p_list_depth, s_tabs);

	switch(p_list_style)
	{
	case kMCParagraphListStyleDisc:
	case kMCParagraphListStyleCircle:
	case kMCParagraphListStyleSquare:
		{
			uint16_t t_bullet;
			t_bullet = MCParagraph::getliststylebullet(p_list_style, true);
			/* UNCHECKED */ MCStringAppendChar(p_buffer, t_bullet);
		}
		break;

	case kMCParagraphListStyleNumeric:
	case kMCParagraphListStyleUpperRoman:
	case kMCParagraphListStyleLowerRoman:
	case kMCParagraphListStyleUpperCase:
	case kMCParagraphListStyleLowerCase:
	{
		char t_label_buffer[PG_MAX_INDEX_SIZE];
		const char *t_label_string;
		uint32_t t_label_length;
		MCParagraph::formatliststyleindex(p_list_style, p_paragraph_number, t_label_buffer, t_label_string, t_label_length);
		/* UNCHECKED */ MCStringAppendFormat(p_buffer, "%.*s", t_label_length, t_label_buffer);
	}
	break;

	default:
		break;
	}

	/* UNCHECKED */ MCStringAppendChar(p_buffer, '\t');
}

// This method extracts the text content of the field including lists.
static bool export_plain_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	MCStringRef t_buffer = (MCStringRef)p_context;

	if (p_event_type == kMCFieldExportEventBeginParagraph && p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
		append_list_label(t_buffer, p_event_data . paragraph_style . list_depth, p_event_data . paragraph_style . list_style, p_event_data . paragraph_number);
	else if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		/* UNCHECKED */ MCStringAppendSubstring(t_buffer, p_event_data.m_text, p_event_data.m_range);
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		/* UNCHECKED */ MCStringAppendChar(t_buffer, '\n');

	return true;
}

static bool export_formatted_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	MCStringRef t_buffer = (MCStringRef)p_context;

	if (p_event_type == kMCFieldExportEventBeginParagraph && p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
		append_list_label(t_buffer, p_event_data . paragraph_style . list_depth, p_event_data . paragraph_style . list_style, p_event_data . paragraph_number);
	else if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		/* UNCHECKED */ MCStringAppendSubstring(t_buffer, p_event_data.m_text, p_event_data.m_range);
	else if (p_event_type == kMCFieldExportEventLineBreak)
	{
		static const char s_tabs[17] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		/* UNCHECKED */ MCStringAppendChar(t_buffer, '\n');
		if (p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
			/* UNCHECKED */ MCStringAppendFormat(t_buffer, "%.*s", p_event_data.paragraph_style.list_depth+1, s_tabs);
	}
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		/* UNCHECKED */ MCStringAppendChar(t_buffer, '\n');

	return true;
}


// MW-2012-02-20: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode.

/* UNSAFE */ bool MCField::exportastext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, estimate_char_count, &t_char_count);

	MCStringRef t_buffer;
	/* UNCHECKED */ MCStringCreateMutable(0, t_buffer);

	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, export_text, t_buffer);
	
	/* UNCHECKED */ MCStringCopyAndRelease(t_buffer, r_string);
	return true;
}

// MW-2012-02-20: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode including any list indices.

bool MCField::exportasplaintext(MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_paragraphs, p_start_index, p_finish_index, estimate_char_count, &t_char_count);

	MCStringRef t_buffer;
	/* UNCHECKED */ MCStringCreateMutable(0, t_buffer);

	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportNumbering, p_paragraphs, p_start_index, p_finish_index, export_plain_text, t_buffer);

	/* UNCHECKED */ MCStringCopyAndRelease(t_buffer, r_string);
	return true;
}

bool MCField::exportasplaintext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string)
{
	return exportasplaintext(resolveparagraphs(p_part_id), p_start_index, p_finish_index, r_string);
}

// MW-2012-02-21: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode, including any list indices and implicit
//   line breaks.
bool MCField::exportasformattedtext(uint32_t p_part_id, int32_t p_start_index, int32_t p_finish_index, MCStringRef& r_string)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, estimate_char_count, &t_char_count);

	MCStringRef t_buffer;
	/* UNCHECKED */ MCStringCreateMutable(0, t_buffer);

	doexport(kMCFieldExportParagraphs | kMCFieldExportLines | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportNumbering, p_part_id, p_start_index, p_finish_index, export_formatted_text, t_buffer);

	/* UNCHECKED */ MCStringCopyAndRelease(t_buffer, r_string);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-03-05: [[ FieldImport ]] Creates a new paragraph at the end of the list
//   and gives it the given styling.
bool MCField::importparagraph(MCParagraph*& x_paragraphs, const MCFieldParagraphStyle *p_style)
{
	MCParagraph *t_new_paragraph;
	t_new_paragraph = new (nothrow) MCParagraph;
    
    // SN-2014-04-25 [[ Bug 12177 ]] Importing HTML was creating parent-less paragraphs,
    // thus sometimes causing crashing when the parent was accessed - mainly when getfontattrs() was needed
    t_new_paragraph -> setparent(this);
    
    // AL-2014-05-28: [[ Bug 12515 ]] If inittext() is not called, the new paragraph
    //  can have nil blocks when it contains an image.
    t_new_paragraph -> inittext();
    
	if (p_style != nil)
		t_new_paragraph->importattrs(*p_style);

	if (x_paragraphs != nil)
		x_paragraphs -> prev() -> append(t_new_paragraph);
	else
		x_paragraphs = t_new_paragraph;
	
	return true;
}

// MW-2012-03-05: [[ FieldImport ]] Appends the given text to the end of the paragraph,
//   applying the specified style to it.
bool MCField::importblock(MCParagraph *p_paragraph, const MCFieldCharacterStyle& p_style, const void *p_bytes, uint32_t p_byte_count, bool p_is_unicode)
{
	// Do nothing if there is nothing to import
	if (p_byte_count == 0)
		return true;
	
	// Give the text to the paragraph to create a new block
	MCAutoStringRef t_text;
	/* UNCHECKED */ MCStringCreateWithBytes((const char_t*)p_bytes, p_byte_count, p_is_unicode?kMCStringEncodingUTF16:kMCStringEncodingNative, false, &t_text);
	MCBlock *t_block = p_paragraph->AppendText(*t_text);
	
	// Import the block attributes.
	t_block -> importattrs(p_style);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCField::sethtml(uint4 parid, MCValueRef data)
{
	if (state & CS_NO_FILE)
	{
		MCresult->sets("can't set HTMLtext while images are loading");
		return ES_ERROR;
	}
	state |= CS_NO_FILE; // prevent interactions while downloading images
	// MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
	MCParagraph *htmlpgptr = importhtmltext(data);
	setparagraphs(htmlpgptr, parid, false);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

Exec_stat MCField::setrtf(uint4 parid, MCStringRef data)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *htmlpgptr = rtftoparagraphs(data);
	setparagraphs(htmlpgptr, parid);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

void MCField::setstyledtext(uint32_t part_id, MCArrayRef p_text)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *stpgptr = styledtexttoparagraphs(p_text);
	if (stpgptr == nil)
		setpartialtext(part_id, kMCEmptyString);
	else
		setparagraphs(stpgptr, part_id);
	state &= ~CS_NO_FILE;
}

Exec_stat MCField::setpartialtext(uint4 parid, MCStringRef p_text)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *htmlpgptr = texttoparagraphs(p_text);
    // SN-2014-06-23: [[ Bug 12303 ]] Parameter added to preserve the 0-length styles
	setparagraphs(htmlpgptr, parid, true);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

MCParagraph *MCField::texttoparagraphs(MCStringRef p_text)
{
    // Create a new list of paragraphs
    MCParagraph *t_paragraphs;
	t_paragraphs = new (nothrow) MCParagraph;
	t_paragraphs -> setparent(this);
	t_paragraphs -> inittext();

	const unichar_t* t_unicode_text;
    t_unicode_text = MCStringGetCharPtr(p_text);

    uindex_t t_text_length;
    t_text_length = MCStringGetLength(p_text);
	
	MCTextBlock t_block;
	memset(&t_block, 0, sizeof(MCTextBlock));
    t_block . string_native = false;
	t_block . foreground_color = 0xffffffff;
	t_block . background_color = 0xffffffff;
    
	while(t_text_length > 0)
	{
		uindex_t t_next;
		for(t_next = 0; t_next < t_text_length; ++t_next)
		{
            if (t_unicode_text[t_next] == '\n')
                break;
		}

		if (t_next > 0)
		{
            t_block . string_buffer = (const uint2*)t_unicode_text;
			t_block . string_length = t_next;
            
			converttoparagraphs(t_paragraphs, NULL, &t_block);
		}

		while(t_next < t_text_length)
		{
            if (t_unicode_text[t_next] != (uint2)'\n')
                break;

			converttoparagraphs(t_paragraphs, NULL, NULL);

			t_next += 1;
		}

		t_text_length -= t_next;
        t_unicode_text += t_next;
	}

	converttoparagraphs(t_paragraphs, NULL, NULL);
	delete t_paragraphs -> prev();

	return t_paragraphs;
}

bool MCField::converttoparagraphs(void *p_context, const MCTextParagraph *p_paragraph, const MCTextBlock *p_block)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = (MCParagraph *)p_context;

	MCParagraph *t_paragraph;
	t_paragraph = t_paragraphs -> prev();

	if (p_block == NULL)
	{
		// Start a new paragraph
		t_paragraph -> defrag();

		MCParagraph *t_new_paragraph;
		t_new_paragraph = new (nothrow) MCParagraph;
		t_new_paragraph -> setparent(t_paragraph -> getparent());
		t_new_paragraph -> inittext();

		// MW-2012-03-14: [[ FieldImport ]] Apply the paragraph style to the new paragraph
		//   if present.
		if (p_paragraph != nil)
		{
			uint32_t t_count;
			t_count = 0;

			MCFieldParagraphStyle t_style;
			memset(&t_style, 0, sizeof(MCFieldParagraphStyle));
			if (p_paragraph -> text_align != kMCTextTextAlignLeft)
				t_style . text_align = p_paragraph -> text_align, t_style . has_text_align = true, t_count += 1;
			if (p_paragraph -> border_width != 0)
				t_style . border_width = p_paragraph -> border_width, t_style . has_border_width = true, t_count += 1;
			if (p_paragraph -> padding != 0)
				t_style . padding = p_paragraph -> padding, t_style . has_padding = true, t_count += 1;
			if (p_paragraph -> first_indent != 0)
				t_style . first_indent = p_paragraph -> first_indent, t_style . has_first_indent = true, t_count += 1;
			if (p_paragraph -> left_indent != 0)
				t_style . left_indent = p_paragraph -> left_indent, t_style . has_left_indent = true, t_count += 1;
			if (p_paragraph -> right_indent != 0)
				t_style . right_indent = p_paragraph -> right_indent, t_style . has_right_indent = true, t_count += 1;
			if (p_paragraph -> space_above != 0)
				t_style . space_above = p_paragraph -> space_above, t_style . has_space_above = true, t_count += 1;
			if (p_paragraph -> space_below != 0)
				t_style . space_below = p_paragraph -> space_below, t_style . has_space_below = true, t_count += 1;
			if (p_paragraph -> background_color != 0xffffffff)
				t_style . background_color = p_paragraph -> background_color, t_style . has_background_color = true, t_count += 1;
			if (p_paragraph -> border_color != 0xffffffff)
				t_style . border_color = p_paragraph -> border_color, t_style . has_border_color = true, t_count += 1;
            if (p_paragraph -> metadata != nil && !MCStringIsEmpty(p_paragraph -> metadata))
				t_style . metadata = p_paragraph -> metadata, t_style . has_metadata = true, t_count += 1;
            else
                t_style . metadata = kMCEmptyString;

			// MW-2012-03-14: [[ RtfParaStyles ]] Apply the listStyle attrs, if applicable.
			if (p_paragraph -> list_style != kMCTextListStyleNone)
			{
				t_style . list_style = p_paragraph -> list_style;
				t_style . list_depth = p_paragraph -> list_depth - 1;
				t_style . has_list_style = true;
				t_count += 1;
			}
			
			if (t_count > 0)
				t_new_paragraph -> importattrs(t_style);
		}

		t_paragraph -> append(t_new_paragraph);
	}
	else if (t_paragraph -> gettextlength() < PARAGRAPH_MAX_LEN && p_block -> string_length > 0)
	{
		// Append the block to the current paragraph
		MCAutoStringRef t_text;
        
        if (p_block -> string_native)
            /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)p_block->string_buffer, p_block->string_length, &t_text);
        else
            /* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)p_block->string_buffer, p_block->string_length, &t_text);
        
		MCBlock *t_block = t_paragraph->AppendText(*t_text);

		// MW-2008-06-12: [[ Bug 6397 ]] Pasting styled text munges the color.
		if (p_block -> foreground_color != 0xffffffff)
		{
			MCColor t_color;
			t_color . red = ((p_block -> foreground_color & 0xff) << 8) | (p_block -> foreground_color & 0xff);
			t_color . green = (p_block -> foreground_color & 0xff00) | ((p_block -> foreground_color & 0xff00) >> 8);
			t_color . blue = ((p_block -> foreground_color & 0xff0000) >> 8) | ((p_block -> foreground_color & 0xff0000) >> 16);
			t_block -> setcolor(&t_color);
		}

		if (p_block -> background_color != 0xffffffff)
		{
			MCColor t_color;
			t_color . red = ((p_block -> background_color & 0xff) << 8) | (p_block -> background_color & 0xff);
			t_color . green = (p_block -> background_color & 0xff00) | ((p_block -> background_color & 0xff00) >> 8);
			t_color . blue = ((p_block -> background_color & 0xff0000) >> 8) | ((p_block -> background_color & 0xff0000) >> 16);
			t_block -> setbackcolor(&t_color);
		}

        MCExecContext ctxt(nil, nil, nil);

		if (p_block -> text_shift != 0)
			t_block -> setshift(p_block -> text_shift);

        if (p_block -> text_link != nil)
            t_block -> SetLinktext(ctxt, MCNameGetString(p_block -> text_link));

        if (p_block -> text_metadata != nil)
            t_block -> SetMetadata(ctxt, p_block -> text_metadata);

		const char *t_font_name;
		t_font_name = p_block -> font_name == NULL ? "" : p_block -> font_name;
        
#if defined _MAC_DESKTOP
        
		// MW-2011-03-13: [[ Bug ]] Try different variants of font searching to ensure we don't
		//   get strange choices. (e.g. Helvetica -> Helvetica Light Oblique).
		char t_derived_font_name[256];
		if (*t_font_name != '\0' &&
            macmatchfontname(t_font_name, t_derived_font_name))
        {
			t_font_name = t_derived_font_name;
        }
        
#endif
        
        MCAutoStringRef t_font_name_ref;
        MCStringCreateWithCString(t_font_name, &t_font_name_ref);
        t_block -> SetTextFont(ctxt, *t_font_name_ref);
		
		if (p_block -> font_size != 0)
        {
            uinteger_t t_size;
            t_size = p_block -> font_size;
            t_block -> SetTextSize(ctxt, &t_size);
        }
		
		if (p_block -> font_style != 0)
        {
            MCInterfaceTextStyle t_style;
            t_style . style = p_block -> font_style;
            t_block -> SetTextStyle(ctxt, t_style);
        }
	}

	return true;
}

extern bool RTFRead(const char *p_rtf, uint4 p_length, MCTextConvertCallback p_writer, void *p_writer_context);


MCParagraph *MCField::rtftoparagraphs(MCStringRef p_data)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = new (nothrow) MCParagraph;
	t_paragraphs -> setparent(this);
	t_paragraphs -> inittext();

    MCAutoPointer<char> t_data;
    /* UNCHECKED */ MCStringConvertToCString(p_data, &t_data);
	RTFRead(*t_data, MCStringGetLength(p_data), converttoparagraphs, t_paragraphs);
	
	// MW-2012-03-13: [[ RtfParaStyles ]] Delete the first paragraph which is only
	//   needed as an initial starting point.
	// MW-2012-03-29: [[ Bug 10134 ]] But only if it isn't the only paragraph!
	if (t_paragraphs -> next() != t_paragraphs)
	{
		MCParagraph *t_head;
		t_head = t_paragraphs -> remove(t_paragraphs);
		delete t_head;
	}

	return t_paragraphs;
}

////////////////////////////////////////////////////////////////////////////////
