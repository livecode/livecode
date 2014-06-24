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
#include "block.h"
#include "line.h"
#include "globals.h"
#include "unicode.h"
#include "text.h"
#include "osspec.h"
#include "textbuffer.h"

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
	// If the null range is requested, there is nothing to do.
	if (p_start_index == p_finish_index)
		return true;

	// Fetch the paragraphs
	MCParagraph *t_paragraphs;
	t_paragraphs = p_paragraphs;

	// The first and last boundaries of the export.
	MCParagraph *t_first_paragraph;
	int32_t t_first_offset, t_last_offset;
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
		t_inherited_paragraph_style . vgrid = getflag(F_VGRID) == True;
		t_inherited_paragraph_style . hgrid = getflag(F_HGRID) == True;
		t_inherited_paragraph_style . dont_wrap = getflag(F_DONT_WRAP) == True;
		t_inherited_paragraph_style . first_indent = indent;
		t_inherited_paragraph_style . tab_count = ntabs;
		t_inherited_paragraph_style . tabs = tabs;
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
		t_inherited_character_style . link_text = kMCEmptyName;
		t_inherited_character_style . image_source = kMCEmptyName;
		t_inherited_character_style . metadata = kMCEmptyName;
		getfontattsnew(t_inherited_character_style . text_font, t_inherited_character_style . text_size, t_inherited_character_style . text_style);
	}

	// Initialize the event data.
	MCFieldExportEventData t_data;
	t_data . bytes = nil;
	t_data . byte_count = 0;
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

	// Now loop through the paragraphs, starting at the one that has been
	// identified as the first.
	MCParagraph *t_paragraph;
	t_paragraph = t_first_paragraph;
	for(;;)
	{
		t_data . is_last_paragraph = t_last_offset <= t_paragraph -> gettextsize() || t_paragraph -> next() == t_paragraphs;

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
						if (t_last -> getindex() + t_last -> getsize() > t_first_offset)
							break;
							
						// MW-2013-09-02: [[ Bug 11144 ]] Make sure we advance to the next line,
						//   otherwise we just get an infinite loop.
						t_line = t_line -> next();
					}
			}
		}

		// If the paragraph has text, then we must process its blocks.
		bool t_last_block;
		if ((p_flags & kMCFieldExportRuns) && t_paragraph -> gettextsize() != 0)
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
			
			// Fetch the text of the paragraph.
			const char *t_text;
			t_text = t_paragraph -> gettext();
			
			// Find the block containing the first index.
			MCBlock *t_block;
			t_block = t_blocks;
			while(t_first_offset > t_block -> getindex() + t_block -> getsize())
				t_block = t_block -> next();

			// Now loop through all the blocks, until we reach one containing
			// last offset (or the end of the list for the paragraph).
			bool t_last_block_on_line;
			t_last_block_on_line = false;
			for(;;)
			{
				// Skip any null blocks (as long as we are not on the last one).
				while(t_block -> next() != t_blocks && t_block -> getsize() == 0)
					t_block = t_block -> next();

				// Work out how far the 'virtual' block (a sequence of blocks with the
				// same attrs - depending on export type).
				int32_t t_start, t_count;
				t_start = MCMax(t_first_offset, t_block -> getindex());
				t_count = t_block -> getindex() + t_block -> getsize() - t_start;
	
				// If we want run styles, then update the array.
				if (t_block -> getsize() != 0 && (p_flags & kMCFieldExportCharacterStyles) != 0)
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
					while(t_next_block != t_blocks && t_next_block -> getsize() == 0)
						t_next_block = t_next_block -> next();

					// Check to see if we are the last block: either the block
					// containing last_offset, or the block at the end of the list.
					if (t_last_offset <= t_block -> getindex() + t_block -> getsize())
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
					if (t_block -> getsize() != 0)
					{
						// If this block has different encoding to the next we must
						// break as they are treated as different events.
						if (t_block -> hasunicode() != t_next_block -> hasunicode())
							break;

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
						t_count += t_next_block -> getsize();
					}
					
					// Advance to the next (non-empty) block.
					t_block = t_next_block;
				}
			
				// Make sure we don't emit more than last offset allows.
				t_count = MCMin(t_last_offset - t_start, t_count);
			
				// Emit the block.
				t_data . bytes = t_text + t_start;
				t_data . byte_count = t_count;
				
				// If we are the last block on the line, trim any VTAB
				bool t_explicit_line_break;
				t_explicit_line_break = false;
				if (t_last_block_on_line && t_count > 0)
				{
					// MW-2012-03-16: [[ Bug ]] Only trim the end if we have an explicit line break.
					if (t_block -> hasunicode())
					{
						if (t_block -> textcomparechar(t_text + t_start + t_count - 2, 11) == True)
							t_explicit_line_break = true, t_data . byte_count -= 2;
					}
					else
					{
						if (t_text[t_start + t_count - 1] == 11)
							t_explicit_line_break = true, t_data . byte_count -= 1;
					}
				}
				
				if (!p_callback(p_context, t_block -> hasunicode() ? kMCFieldExportEventUnicodeRun : kMCFieldExportEventNativeRun, t_data))
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
		t_last_offset -= t_paragraph -> gettextsizecr();

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
	
	if (p_event_type == kMCFieldExportEventUnicodeRun)
		t_count += p_event_data . byte_count / 2;
	else if (p_event_type == kMCFieldExportEventNativeRun)
		t_count += p_event_data . byte_count;
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		t_count += 1;
		
	return true;
}

// This method extracts the text content of the field.
static bool export_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	text_buffer_t& t_buffer = *(text_buffer_t *)p_context;
	
	if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		t_buffer . appendtext(p_event_data . bytes, p_event_data . byte_count, p_event_type == kMCFieldExportEventUnicodeRun);
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		t_buffer . appendtext("\n", 1, false);

	return true;
}

// This method appends a list label to the buffer.
static void append_list_label(text_buffer_t& p_buffer, uint32_t p_list_depth, uint32_t p_list_style, uint32_t p_paragraph_number)
{
	static const char s_tabs[16] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

	p_buffer . appendtext(s_tabs, p_list_depth, false);

	switch(p_list_style)
	{
	case kMCParagraphListStyleDisc:
	case kMCParagraphListStyleCircle:
	case kMCParagraphListStyleSquare:
		if (p_buffer . isunicode())
		{
			uint16_t t_bullet;
			t_bullet = MCParagraph::getliststylebullet(p_list_style, true);
			p_buffer . appendtext(&t_bullet, 2, true);
		}
		else
		{
			char t_bullet;
			t_bullet = MCParagraph::getliststylebullet(p_list_style, false);
			p_buffer . appendtext(&t_bullet, 1, false);
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
		p_buffer . appendtext(t_label_string, t_label_length, false);
	}
	break;

	default:
		break;
	}

	p_buffer . appendtext(s_tabs, 1, false);
}

// This method extracts the text content of the field including lists.
static bool export_plain_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	text_buffer_t& t_buffer = *(text_buffer_t *)p_context;

	if (p_event_type == kMCFieldExportEventBeginParagraph && p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
		append_list_label(t_buffer, p_event_data . paragraph_style . list_depth, p_event_data . paragraph_style . list_style, p_event_data . paragraph_number);
	else if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		t_buffer . appendtext(p_event_data . bytes, p_event_data . byte_count, p_event_type == kMCFieldExportEventUnicodeRun);
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		t_buffer . appendtext("\n", 1, false);

	return true;
}

static bool export_formatted_text(void *p_context, MCFieldExportEventType p_event_type, const MCFieldExportEventData& p_event_data)
{
	text_buffer_t& t_buffer = *(text_buffer_t *)p_context;

	if (p_event_type == kMCFieldExportEventBeginParagraph && p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
		append_list_label(t_buffer, p_event_data . paragraph_style . list_depth, p_event_data . paragraph_style . list_style, p_event_data . paragraph_number);
	else if (p_event_type == kMCFieldExportEventUnicodeRun || p_event_type == kMCFieldExportEventNativeRun)
		t_buffer . appendtext(p_event_data . bytes, p_event_data . byte_count, p_event_type == kMCFieldExportEventUnicodeRun);
	else if (p_event_type == kMCFieldExportEventLineBreak)
	{
		static const char s_tabs[17] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		t_buffer . appendtext("\n", 1, false);
		if (p_event_data . paragraph_style . list_style != kMCParagraphListStyleNone)
			t_buffer . appendtext(s_tabs, p_event_data . paragraph_style . list_depth + 1, false);
	}
	else if (p_event_type == kMCFieldExportEventEndParagraph && !p_event_data . is_last_paragraph)
		t_buffer . appendtext("\n", 1, false);

	return true;
}


// MW-2012-02-20: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode.
void MCField::exportastext(uint32_t p_part_id, MCExecPoint& ep, int32_t p_start_index, int32_t p_finish_index, bool p_as_unicode)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, estimate_char_count, &t_char_count);
	
	if (p_as_unicode)
		t_char_count *= 2;

	text_buffer_t t_buffer;
	t_buffer . setasunicode(p_as_unicode);

	if (t_buffer . ensure(t_char_count))
		doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, export_text, &t_buffer);
	
	t_buffer . givetoep(ep);
}

// MW-2012-02-20: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode including any list indices.
void MCField::exportasplaintext(MCExecPoint& ep, MCParagraph *p_paragraphs, int32_t p_start_index, int32_t p_finish_index, bool p_as_unicode)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_paragraphs, p_start_index, p_finish_index, estimate_char_count, &t_char_count);

	if (p_as_unicode)
		t_char_count *= 2;

	text_buffer_t t_buffer;
	t_buffer . setasunicode(p_as_unicode);

	if (t_buffer . ensure(t_char_count))
		doexport(kMCFieldExportParagraphs | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportNumbering, p_paragraphs, p_start_index, p_finish_index, export_plain_text, &t_buffer);

	t_buffer . givetoep(ep);
}

void MCField::exportasplaintext(uint32_t p_part_id, MCExecPoint& ep, int32_t p_start_index, int32_t p_finish_index, bool p_as_unicode)
{
	exportasplaintext(ep, resolveparagraphs(p_part_id), p_start_index, p_finish_index, p_as_unicode);
}

// MW-2012-02-21: [[ FieldExport ]] This method exports the content of the
//   field as either native or unicode, including any list indices and implicit
//   line breaks.
void MCField::exportasformattedtext(uint32_t p_part_id, MCExecPoint& ep, int32_t p_start_index, int32_t p_finish_index, bool p_as_unicode)
{
	uint32_t t_char_count;
	t_char_count = 0;
	doexport(kMCFieldExportParagraphs | kMCFieldExportRuns, p_part_id, p_start_index, p_finish_index, estimate_char_count, &t_char_count);

	if (p_as_unicode)
		t_char_count *= 2;

	text_buffer_t t_buffer;
	t_buffer . setasunicode(p_as_unicode);

	if (t_buffer . ensure(t_char_count))
		doexport(kMCFieldExportParagraphs | kMCFieldExportLines | kMCFieldExportRuns | kMCFieldExportParagraphStyles | kMCFieldExportNumbering, p_part_id, p_start_index, p_finish_index, export_formatted_text, &t_buffer);

	t_buffer . givetoep(ep);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-03-05: [[ FieldImport ]] Creates a new paragraph at the end of the list
//   and gives it the given styling.
bool MCField::importparagraph(MCParagraph*& x_paragraphs, const MCFieldParagraphStyle *p_style)
{
	MCParagraph *t_new_paragraph;
	t_new_paragraph = new MCParagraph;
	t_new_paragraph -> state |= PS_LINES_NOT_SYNCHED;
	t_new_paragraph -> setparent(this);
	t_new_paragraph -> blocks = new MCBlock;
	t_new_paragraph -> blocks -> setparent(t_new_paragraph);
	t_new_paragraph -> blocks -> index = 0;
	t_new_paragraph -> blocks -> size = 0;
	if (p_style != nil)
		t_new_paragraph -> importattrs(*p_style);
	
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
	// Make sure we don't try and append any more than 64K worth of bytes.
	uint32_t t_text_length;
	t_text_length = MCMin(p_byte_count, 65534U - p_paragraph -> textsize);
	
	// If we are unicode and the text length is odd, chop off the last char.
	if (p_is_unicode && (p_byte_count & 1) != 0)
		t_text_length -= 1;
	
	// If there is not text to fill, do nothing.
	if (t_text_length == 0)
		return true;

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
	
	// MW-2012-03-10: [[ Bug ]] Make sure we set the block's unicode flag (if necessary)
	if (p_is_unicode)
		t_block -> sethasunicode(p_is_unicode);
	
	// Copy across the bytes of the text.
	memcpy(p_paragraph -> text + t_block -> index, p_bytes, t_block -> size);
	p_paragraph -> textsize += t_block -> size;
	
	// Import the block attributes.
	t_block -> importattrs(p_style);

	// Make sure the tab flag is updated.
	if (t_block -> textstrchr(p_paragraph -> text + t_block -> index, t_block -> size, '\t'))
		t_block -> flags |= F_HAS_TAB;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCField::sethtml(uint4 parid, const MCString &data)
{
	if (state & CS_NO_FILE)
	{
		MCresult->sets("can't set HTMLtext while images are loading");
		return ES_ERROR;
	}
	state |= CS_NO_FILE; // prevent interactions while downloading images
	// MW-2012-03-08: [[ FieldImport ]] Use the new htmlText importer.
	MCParagraph *htmlpgptr = importhtmltext(data);
	setparagraphs(htmlpgptr, parid);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

Exec_stat MCField::setrtf(uint4 parid, const MCString &data)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *htmlpgptr = rtftoparagraphs(data);
	setparagraphs(htmlpgptr, parid);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

Exec_stat MCField::setstyledtext(uint4 parid, MCExecPoint &ep)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *stpgptr = styledtexttoparagraphs(ep);
	if (stpgptr == nil)
		setpartialtext(parid, MCnullmcstring, false);
	else
		setparagraphs(stpgptr, parid);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

Exec_stat MCField::setpartialtext(uint4 parid, const MCString &data, bool p_unicode)
{
	state |= CS_NO_FILE; // prevent interactions while downloading images
	MCParagraph *htmlpgptr = texttoparagraphs(data, p_unicode);
	setparagraphs(htmlpgptr, parid);
	state &= ~CS_NO_FILE;
	return ES_NORMAL;
}

MCParagraph *MCField::texttoparagraphs(const MCString& p_text, Boolean p_unicode)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = new MCParagraph;
	t_paragraphs -> state |= PS_LINES_NOT_SYNCHED;
	t_paragraphs -> setparent(this);
	t_paragraphs -> blocks = new MCBlock;
	t_paragraphs -> blocks -> setparent(t_paragraphs);
	t_paragraphs -> blocks -> index = 0;
	t_paragraphs -> blocks -> size = 0;

	const char *t_native_text;
	t_native_text = p_text . getstring();

	uint2 *t_unicode_text;
	t_unicode_text = (uint2 *)p_text . getstring();

	uint4 t_text_length;
	t_text_length = p_text . getlength();
	if (p_unicode)
		t_text_length /= 2;
	
	MCTextBlock t_block;
	memset(&t_block, 0, sizeof(MCTextBlock));
	t_block . string_native = (p_unicode == False);
	t_block . foreground_color = 0xffffffff;
	t_block . background_color = 0xffffffff;	
	while(t_text_length > 0)
	{
		uint4 t_next;
		for(t_next = 0; t_next < t_text_length; ++t_next)
		{
			if (p_unicode)
			{
				if (t_unicode_text[t_next] == (uint2)'\n')
					break;
			}
			else if (t_native_text[t_next] == '\n')
				break;
		}

		if (t_next > 0)
		{
			if (p_unicode)
				t_block . string_buffer = t_unicode_text;
			else
				t_block . string_buffer = (uint2 *)t_native_text;
			t_block . string_length = t_next;
			converttoparagraphs(t_paragraphs, NULL, &t_block);
		}

		while(t_next < t_text_length)
		{
			if (p_unicode)
			{
				if (t_unicode_text[t_next] != (uint2)'\n')
					break;
			}
			else if (t_native_text[t_next] != '\n')
				break;

			converttoparagraphs(t_paragraphs, NULL, NULL);

			t_next += 1;
		}

		t_text_length -= t_next;
		if (p_unicode)
			t_unicode_text += t_next;
		else
			t_native_text += t_next;
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
		t_paragraph -> text = (char *)realloc(t_paragraph -> text, t_paragraph -> textsize);
		t_paragraph -> buffersize = t_paragraph -> textsize;
		t_paragraph -> defrag();

		MCParagraph *t_new_paragraph;
		t_new_paragraph = new MCParagraph;
		t_new_paragraph -> state |= PS_LINES_NOT_SYNCHED;
		t_new_paragraph -> setparent(t_paragraph -> getparent());
		t_new_paragraph -> blocks = new MCBlock;
		t_new_paragraph -> blocks -> setparent(t_new_paragraph);
		t_new_paragraph -> blocks -> index = 0;
		t_new_paragraph -> blocks -> size = 0;

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
			if (p_paragraph -> metadata != nil && p_paragraph -> metadata != kMCEmptyName)
				t_style . metadata = p_paragraph -> metadata, t_style . has_metadata = true, t_count += 1;

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
	else if (t_paragraph -> textsize < 65535 && p_block -> string_length > 0)
	{
		// Append the block to the current paragraph

		// If there isn't enough room for the string as unicode, then extend the
		// capacity of the text buffer appropriately.
		uint4 t_string_byte_length;
		if (p_block -> string_native)
			t_string_byte_length = p_block -> string_length;
		else
			t_string_byte_length = p_block -> string_length * 2;

		if (t_paragraph -> textsize + t_string_byte_length > t_paragraph -> buffersize)
		{
			t_paragraph -> buffersize = MCU_min(65534U, (t_paragraph -> textsize + t_string_byte_length + 64) & ~63);
			t_paragraph -> text = (char *)realloc(t_paragraph -> text, t_paragraph -> buffersize);
		}

		if (t_paragraph -> text != NULL)
		{
			MCBlock *t_block;
			t_block = t_paragraph -> blocks -> prev();

			// If the previous block is not empty then create a new one.
			if (t_block -> size != 0)
			{
				MCBlock *t_new_block;
				t_new_block = new MCBlock;
				t_new_block -> parent = t_paragraph;
				t_block -> append(t_new_block);
				t_block = t_new_block;
			}

			// MW-2008-06-12: [[ Bug 6397 ]] Pasting styled text munges the color.
			if (p_block -> foreground_color != 0xffffffff)
			{
				MCColor t_color;
				t_color . pixel = 0;
				t_color . red = ((p_block -> foreground_color & 0xff) << 8) | (p_block -> foreground_color & 0xff);
				t_color . green = (p_block -> foreground_color & 0xff00) | ((p_block -> foreground_color & 0xff00) >> 8);
				t_color . blue = ((p_block -> foreground_color & 0xff0000) >> 8) | ((p_block -> foreground_color & 0xff0000) >> 16);
				t_color . flags = 0xff;
				t_color . pad = 0;
				t_block -> setcolor(&t_color);
			}

			if (p_block -> background_color != 0xffffffff)
			{
				MCColor t_color;
				t_color . pixel = 0;
				t_color . red = ((p_block -> background_color & 0xff) << 8) | (p_block -> background_color & 0xff);
				t_color . green = (p_block -> background_color & 0xff00) | ((p_block -> background_color & 0xff00) >> 8);
				t_color . blue = ((p_block -> background_color & 0xff0000) >> 8) | ((p_block -> background_color & 0xff0000) >> 16);
				t_color . flags = 0xff;
				t_color . pad = 0;
				t_block -> setbackcolor(&t_color);
			}

			if (p_block -> text_shift != 0)
				t_block -> setshift(p_block -> text_shift);

			if (p_block -> text_link != nil)
				t_block -> setatts(P_LINK_TEXT, (void *)MCNameGetCString(p_block -> text_link));

			if (p_block -> text_metadata != nil)
				t_block -> setatts(P_METADATA, (void *)MCNameGetCString(p_block -> text_metadata));

			const uint2 *t_input_text;
			t_input_text = p_block -> string_buffer;

			uint4 t_input_length;
			t_input_length = p_block -> string_length;

			const char *t_font_name;
			t_font_name = p_block -> font_name == NULL ? "" : p_block -> font_name;

#ifdef _MACOSX
			// MW-2011-03-13: [[ Bug ]] Try different variants of font searching to ensure we don't
			//   get strange choices. (e.g. Helvetica -> Helvetica Light Oblique).
			char t_derived_font_name[256];
			if (macmatchfontname(t_font_name, t_derived_font_name))
				t_font_name = t_derived_font_name;
#endif

			while(t_input_length > 0 && t_paragraph -> textsize < 65534)
			{
				uint4 t_made, t_used;
				
				// MW-2008-08-21: [[ Bug 6969 ]] Make sure we bound the size of the input passed to MCTextRunnify to be
				//   the maximum we could accept in the output buffer. Otherwise we get a crash...
				if (p_block -> string_native)
				{
					memcpy(t_paragraph -> text + t_paragraph -> textsize, t_input_text, t_input_length);
					t_used = t_input_length;
					t_made = t_input_length;
				}
				else
					MCTextRunnify(t_input_text, MCU_min(t_input_length, (uint4)t_paragraph -> buffersize - t_paragraph -> textsize), (uint1 *)t_paragraph -> text + t_paragraph -> textsize, t_used, t_made);
				
				bool t_is_unicode;
				t_is_unicode = t_made == 0;

				MCBlock *t_block;
				t_block = t_paragraph -> blocks -> prev();
				if (t_block -> size != 0 && t_block -> hasunicode() != t_is_unicode)
				{
					MCBlock *t_new_block;
					t_new_block = new MCBlock(*t_block);
					t_new_block -> size = 0;
					t_block -> append(t_new_block);
					t_block = t_new_block;
				}

				t_block -> index = t_paragraph -> textsize;

				// MW-2012-02-13: [[ Block Unicode ]] Create the appropriate type of block based
				//   on whether the text is unicode or not.
				if (t_made == 0)
				{
					// Unicode run
					if (t_paragraph -> textsize + t_used * 2 > 65534)
						t_used = MCU_min(t_used, (uint4)(65535 - t_paragraph -> textsize) / 2);
						
					memcpy(t_paragraph -> text + t_paragraph -> textsize, t_input_text, t_used * 2);
					t_paragraph -> textsize += t_used * 2;

					t_block -> size += t_used * 2;
					t_block -> flags |= F_HAS_UNICODE;
				}
				else
				{
					// Native run
					if (t_paragraph -> textsize + t_made > 65534)
						t_made = MCU_min(t_made, 65535U - t_paragraph -> textsize);
						
					t_paragraph -> textsize += t_made;

					t_block -> size += t_made;
					t_block -> flags &= ~F_HAS_UNICODE;
				}

				t_block -> setatts(P_TEXT_FONT, (void *)t_font_name);
				if (t_block -> textstrchr(t_paragraph -> text + t_block -> index, t_block -> size, '\t'))
					t_block -> flags |= F_HAS_TAB;

				if (p_block -> font_size != 0)
					t_block -> setatts(P_TEXT_SIZE, (void *)p_block -> font_size);

				if (p_block -> font_style != 0)
					t_block -> setatts(P_TEXT_STYLE, (void *)p_block -> font_style);

				t_input_text += t_used;
				t_input_length -= t_used;
			}
		}
	}

	return true;
}

extern bool RTFRead(const char *p_rtf, uint4 p_length, MCTextConvertCallback p_writer, void *p_writer_context);

MCParagraph *MCField::rtftoparagraphs(const MCString& p_data)
{
	MCParagraph *t_paragraphs;
	t_paragraphs = new MCParagraph;
	t_paragraphs -> state |= PS_LINES_NOT_SYNCHED;
	t_paragraphs -> setparent(this);
	t_paragraphs -> blocks = new MCBlock;
	t_paragraphs -> blocks -> setparent(t_paragraphs);
	t_paragraphs -> blocks -> index = 0;
	t_paragraphs -> blocks -> size = 0;

	RTFRead(p_data . getstring(), p_data . getlength(), converttoparagraphs, t_paragraphs);
	
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
