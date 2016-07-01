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
#include "dcdefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "crypt.h"
#include "stack.h"
#include "field.h"

#include "util.h"

#include "globals.h"

#include "mctheme.h"

#include "context.h"

#include "unicode.h"
#include "paragraph.h"

MCParagraph::MCParagraph(void)
{
	m_state = 0;
	m_opened = 0;
	
	m_focused_index = 0;
	m_start_index = m_end_index = m_original_index = MAXUINT2;

	m_style_table = 0;
	m_object_table = 0;

	m_text_size = 0;
	m_text = NULL;

	m_styles_size = 0;
	m_styles = NULL;

	m_lines_size = 0;
	m_lines = NULL;

	m_parent = NULL;
}

MCParagraph::MCParagraph(const MCParagraph& p_original)
{
	m_parent = p_original . m_parent;

	m_state = 0;
	m_opened = 0;

	m_lines_size = 0;
	m_lines = NULL;
	
	m_style_table = p_original . m_style_table;
	m_object_table = p_original . m_object_table;

	p_original . CloneText(m_text, m_text_size);
	p_original . CloneStyles(m_styles, m_styles_size);
}

MCParagraph::MCParagraph(const MCParagraph& p_original, uint4 p_first, uint4 p_last)
{
	m_parent = p_original . m_parent;

	m_state = 0;
	m_opened = 0;

	m_lines_size = 0;
	m_lines = NULL;

	m_style_table = p_original . m_style_table;
	m_object_table = p_original . m_object_table;

	p_original . CopyText(p_first, p_last, m_text, m_text_size);
	p_original . CopyStyles(p_first, p_last, m_styles, m_styles_size);
}

MCParagraph::~MCParagraph(void)
{
	while(m_opened)
		close();

	DestroyText();
	DestroyStyles();
}

///////////////////////////////////////////////////////////////////////////////

static IO_stat IO_read_color(uint4& r_color, bool p_has_name, IO_handle stream)
{
	IO_stat stat;
	stat = IO_NORMAL;

	uint2 t_red;
	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&t_red, stream);

	uint2 t_green;
	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&t_green, stream);

	uint2 t_blue;
	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&t_blue, stream);

	if (stat == IO_NORMAL && p_has_name)
	{
		char *t_name;
		stat = IO_read_string(t_name, stream);
		if (stat == IO_NORMAL)
			delete t_name;
	}

	return stat;
}

IO_stat MCParagraph::load_block(IO_handle stream, const char *version, uint4& r_block_offset, uint4& r_block_length, uint4& r_style)
{
	IO_stat stat;
	stat = IO_NORMAL;

	uint4 t_flags;
	t_flags = 0;
	if (stat == IO_NORMAL)
		stat = IO_read_uint4(&t_flags, stream);

	uint2 t_font_index;
	t_font_index = 0;
	if (stat == IO_NORMAL && (t_flags & F_FONT) != 0)
	{
		if (strncmp(version, "1.3", 3) > 0)
			stat = IO_read_uint2(&t_font_index, stream);
		else
		{
			char *t_fname;
			t_fname = NULL;
			if (stat == IO_NORMAL)
				stat = IO_read_string(t_fname, stream);

			uint2 t_fsize;
			if (stat == IO_NORMAL)
				stat = IO_read_uint2(&t_fsize, stream);

			uint2 t_fstyle;
			if (stat == IO_NORMAL)
				stat = IO_read_uint2(&t_fstyle, stream);

			if (stat == IO_NORMAL)
				t_font_index = m_parent -> getstack() -> loadfont(t_fname, t_fsize, t_fstyle);

			delete t_fname;
		}
	}

	uint4 t_foreground_color;
	t_foreground_color = 0;
	if (stat == IO_NORMAL && (t_flags & F_HAS_COLOR) != 0)
		stat = IO_read_color(t_foreground_color, (t_flags & F_HAS_COLOR_NAME) != 0, stream);

	uint4 t_background_color;
	t_background_color = 0;
	if (stat == IO_NORMAL && (t_flags & F_HAS_BACK_COLOR) != 0)
		stat = IO_read_color(t_background_color, strncmp(version, "2.0", 3) < 0 && (t_flags & F_HAS_BACK_COLOR_NAME) != 0, stream);
	
	int2 t_text_shift;
	t_text_shift = 0;
	if (stat == IO_NORMAL && (t_flags & F_HAS_SHIFT) != 0)
		stat = IO_read_int2(&t_text_shift, stream);

	char *t_link;
	t_link = NULL;
	if (stat == IO_NORMAL && (t_flags & F_HAS_LINK) != 0)
		stat = IO_read_string(t_link, stream);

	char *t_image;
	t_image = NULL;
	if (stat == IO_NORMAL && (t_flags & F_HAS_IMAGE) != 0)
		stat = IO_read_string(t_image, stream);

	uint2 t_block_offset;
	t_block_offset = 0;
	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&t_block_offset, stream);

	uint2 t_block_length;
	t_block_length = 0;
	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&t_block_length, stream);

	if (stat == IO_NORMAL && t_block_length > 0)
	{
		MCParagraphCharStyle t_style_info;
		memset(&t_style_info, 0, sizeof(MCParagraphCharStyle));

		t_style_info . has_foreground = (t_flags & F_HAS_COLOR) != 0;
		t_style_info . foreground = t_foreground_color;

		t_style_info . has_background = (t_flags & F_HAS_BACK_COLOR) != 0;
		t_style_info . background = t_background_color;

		if ((t_flags & F_FONT) != 0)
		{
			const char *t_font_name;
			uint2 t_font_size;
			uint2 t_font_style;
			m_parent -> getstack() -> lookupfontcache(t_font_index, t_font_name, t_font_size, t_font_style);

			t_style_info . has_font = true;
			t_style_info . font = AcquireFont(t_font_name);
			t_style_info . has_size = true;
			t_style_info . size = t_font_size;
			t_style_info . has_attrs = true;
			t_style_info . attrs = t_font_style;
		}
		else
		{
			t_style_info . has_font = false;
			t_style_info . has_size = false;
			t_style_info . has_attrs = false;
		}

		if ((t_flags & F_HAS_LINK) != 0)
		{
			t_style_info . has_link = true;
			t_style_info . link = AcquireLink(t_link);
		}
		else
			t_style_info . has_link = false;

		if ((t_flags & F_HAS_IMAGE) != 0)
		{
			t_style_info . has_image = true;
			t_style_info . image = AcquireImage(t_image);
		}
		else
			t_style_info . has_image = false;

		t_style_info . is_unicode = (t_flags & F_HAS_UNICODE) != 0;

		uint2 t_style;
		t_style = AcquireCharStyle(t_style_info);

		r_block_offset = t_block_offset;
		r_block_length = t_block_length;
		r_style = t_style;
	}

	delete t_link;
	delete t_image;

	return stat;
}

IO_stat MCParagraph::load(IO_handle stream, const char *version)
{
	IO_stat stat;
	stat = IO_NORMAL;

	if (stat == IO_NORMAL)
		stat = IO_read_uint2(&m_text_size, stream);

	if (m_text_size == 1)
	{
		if (stat == IO_NORMAL)
		{
			char t_temp;
			uint4 t_temp_count;
			t_temp_count = 1;
			stat = IO_read(&t_temp, sizeof(uint1), t_temp_count, stream);
		}

		if (stat == IO_NORMAL)
			m_text_size = 0;
	}
	else if (m_text_size > 1)
	{
		if (stat == IO_NORMAL)
		{
			m_text = malloc(m_text_size);
			if (m_text == NULL)
				stat = IO_ERROR;
		}

		if (stat == IO_NORMAL)
		{
			uint4 t_temp_text_size;
			t_temp_text_size = m_text_size;
			stat = IO_read(m_text, sizeof(uint1), t_temp_text_size, stream);
		}

		if (stat == IO_NORMAL)
		{
			if (MCencryptstring != NULL)
				MCX_passde((char *)m_text, MCencryptstring, m_text_size);

			m_text_size -= 1;
		}
	}

	while(stat == IO_NORMAL)
	{
		uint1 t_type;
		stat = IO_read_uint1(&t_type, stream);
		if (stat != IO_NORMAL)
			break;

		uint4 t_block_offset, t_block_length, t_block_style;
		stat = load_block(stream, version, t_block_offset, t_block_length, t_block_style);
		if (stat != IO_NORMAL)
			break;

		// Make sure the block doesn't overrun the text buffer (shouldn't ever be
		// necessary...)
		t_block_length = MCU_min(m_text_size - t_block_offset, t_block_length);

		// Byte-swap the text to native byte-order if its unicode, otherwise translate
		// Mac<->ISO if necessary.
		const MCParagraphCharStyle *t_style_info;
		t_style_info = FetchCharStyle(t_block_style);
		if (t_style_info -> is_unicode)
		{
			uint4 t_length;
			t_length = t_block_length / 2;

			uint2 *t_ptr;
			t_ptr = (uint2 *)((uint1 *)m_text + t_block_offset);

			for(; t_length > 0; t_length -= 1)
				swap_uint2(t_ptr++);
		}
		else if (MCtranslatechars)
		{
#ifdef _MACOSX
			IO_iso_to_mac((char *)m_text + t_block_offset, t_block_length);
#else
			IO_mac_to_iso((char *)m_text + t_block_offset, t_block_length);
#endif
		}

		// Replace the style in the given range with the style
		ReplaceStyles(t_block_offset, t_block_length, t_block_style);
	}

	if (stat == IO_NORMAL)
	{
		// If no styles were applied, we need to translate the text
		if (m_styles_size == 0 && MCtranslatechars && m_text_size > 0)
#ifdef _MACOSX
			IO_iso_to_mac((char *)m_text, m_text_size);
#else
			IO_mac_to_iso((char *)m_text, m_text_size);
#endif

		Compact();

		MCS_seek_cur(stream, -1);
	}

	return stat;
}

IO_stat MCParagraph::save(IO_handle stream, uint4 p_part)
{
	return IO_ERROR;
}

void MCParagraph::open(void)
{
}

void MCParagraph::close(void)
{
}

uint2 MCParagraph::getopened(void)
{
	return m_opened;
}

MCField *MCParagraph::getparent(void)
{
	return m_parent;
}

void MCParagraph::setparent(MCField *p_parent)
{
	m_parent = p_parent;
}

void MCParagraph::flow(MCFontStruct *p_parent_font)
{
}

void MCParagraph::noflow(MCFontStruct *p_parent_font)
{
}

void MCParagraph::join(void)
{
	assert(next() != this);

	MCParagraph *t_next;
	t_next = next();

	MCParagraphCursor t_cursor(this, m_text_size);

	// We enclose this in a block to make sure the cursor deconstructs before
	// we delete the paragraph
	{
		MCParagraphCursor t_other_cursor(t_next, 0, t_next -> m_text_size);
		t_cursor . Replace(t_other_cursor);
	}

	delete t_next;
}

void MCParagraph::split(void)
{
}

void MCParagraph::deletestring(uint2 si, uint2 ei)
{
	assert(si <= ei);
	assert(ei <= m_text_size);

	if (si == ei)
		return;

	MCParagraphCursor t_cursor(this, si, ei);
	t_cursor . Delete();

	if (m_focused_index >= ei)
		m_focused_index -= ei - si;
	else if (m_focused_index > si)
		m_focused_index = si;

	m_start_index = m_end_index = m_original_index = m_focused_index;
}

void MCParagraph::deleteselection(void)
{
	if (m_start_index != m_end_index)
	{
		m_focused_index = m_start_index;
		deletestring(m_start_index, m_end_index);
	}

	m_start_index = m_end_index = m_original_index = 0xffff;
	m_state &= ~kStateHilited;
}

MCParagraph *MCParagraph::copyselection(void)
{
	assert(m_start_index != 0xffff);
	assert(m_end_index != 0xffff);
	return new MCParagraph(*this, m_start_index, m_end_index);
}

Boolean MCParagraph::finsert(const MCString& p_string, bool p_dont_break)
{
	return False;
}

int2 MCParagraph::fdelete(Field_translations p_type, MCParagraph*& r_undo)
{
	MCParagraphCursor t_cursor(this, m_focused_index);
	switch(p_type)
	{
	case FT_DELBCHAR:
		if (!t_cursor . MoveStart(kMCParagraphCursorMoveCharacterBegin, -1))
			return -1;
	break;

	case FT_DELBWORD:
		if (!t_cursor . MoveStart(kMCParagraphCursorMoveWordBegin, -1))
			return -1;
	break;

	case FT_DELFCHAR:
		if (!t_cursor . MoveFinish(kMCParagraphCursorMoveCharacterEnd, 1))
			return 1;
	break;

	case FT_DELFWORD:
		if (!t_cursor . MoveFinish(kMCParagraphCursorMoveWordEnd, 1))
			return 1;
	break;

	default:
		break;
	}

	r_undo = t_cursor . Clone();
	t_cursor . Delete();

	m_focused_index = t_cursor . GetOffset();

	return 0;
}

uint1 MCParagraph::fmovefocus(Field_translations p_type)
{
	MCParagraphCursor t_cursor(this, m_focused_index);
	switch(p_type)
	{
	case FT_LEFTCHAR:
		if (!t_cursor . Move(kMCParagraphCursorMoveCharacterBegin, -1))
			return FT_LEFTCHAR;
	break;

	case FT_LEFTWORD:
		if (!t_cursor . Move(kMCParagraphCursorMoveWordBegin, -1))
			return FT_LEFTCHAR;
	break;

	case FT_RIGHTCHAR:
		if (!t_cursor . Move(kMCParagraphCursorMoveCharacterEnd, 1))
			return FT_RIGHTCHAR;
	break;

	case FT_RIGHTWORD:
		if (!t_cursor . Move(kMCParagraphCursorMoveWordEnd, 1))
			return FT_RIGHTCHAR;
	break;

	case FT_BOS:
		if (!t_cursor . Move(kMCParagraphCursorMoveSentenceBegin, 1))
			return FT_BOS;
	break;

	case FT_EOS:
		if (!t_cursor . Move(kMCParagraphCursorMoveSentenceEnd, 1))
			return FT_EOS;
	break;

	case FT_BOP:
		if (!t_cursor . Move(kMCParagraphCursorMoveParagraphBegin, 1))
			return FT_BOP;
	break;

	case FT_EOP:
		if (!t_cursor . Move(kMCParagraphCursorMoveParagraphEnd, 1))
			return FT_EOP;
	break;

	default:
	break;
	}

	m_focused_index = t_cursor . GetOffset();

	return FT_UNDEFINED;
}

void MCParagraph::clean(void)
{
}

uint2 MCParagraph::gettextsize(void)
{
	return m_text_size;
}

uint2 MCParagraph::gettextsizecr(void)
{
	return m_text_size + 1;
}

uint2 MCParagraph::gettextlength(void)
{
	if (m_styles_size == 0)
		return m_text_size;

	uint2 t_length;
	t_length = 0;
	for(uint4 i = 0; i < m_styles_size; ++i)
	{
		const MCParagraphCharStyle *t_style;
		t_style = FetchCharStyle(GetStyleEntryIndex(m_styles[i]));

		uint4 t_style_length;
		t_style_length = GetStyleEntryOffset(m_styles[i + 1]) - GetStyleEntryOffset(m_styles[i]);
		if (t_style -> is_unicode)
			t_length += t_style_length / 2;
		else
			t_length += t_style_length;
	}

	return t_length;
}

const char *MCParagraph::gettext(void)
{
	if (m_text_size == 0)
		return "";

	return (const char *)m_text;
}

char *MCParagraph::getformattedtext(uint2& r_length)
{
	uint4 t_length;
	char *t_result;
	if (m_lines_size == 0)
	{
		t_result = new char[m_text_size];
		t_length = m_text_size;

		memcpy(t_result, m_text, m_text_size);
	}
	else
	{
		t_result = new char[m_text_size + m_lines_size - 1];
		t_length = 0;
		for(uint4 i = 0; i < (uint4)m_lines_size - 1; ++i)
		{
			uint4 t_line_length;
			t_line_length = m_lines[i + 1] - m_lines[i];
			memcpy(t_result + t_length, (char *)m_text + m_lines[i], t_line_length);
			t_length += t_line_length;
			if (i + 1 != m_lines_size)
			{
				t_result[0] = '\n';
				t_length += 1;
			}
		}
	}

	r_length = t_length;
	return t_result;
}

void MCParagraph::gethtmltext(MCExecPoint& ep)
{
	ep . clear();
}

void MCParagraph::settext(char *p_text, uint2 p_length)
{
	DestroyText();
	DestroyLines();
	DestroyStyles();

	if (p_length != 0)
	{
		m_text = p_text;
		m_text_size = p_length;
	}
}

void MCParagraph::resettext(char *p_text, uint2 p_length)
{
	m_text = p_text;
	m_text_size = p_length;
	if (m_styles != NULL)
		m_styles[m_styles_size - 1] = MakeStyleEntry(p_length, 0);
}

///////////////////////////////////////////////////////////////////////////////

void MCParagraph::Delete(uint4 p_start, uint4 p_finish)
{
	assert(p_start <= p_finish);
	assert(p_finish <= m_text_size);

	if (p_start == p_finish)
		return;

	uint4 t_text_capacity;
	t_text_capacity = FetchTextCapacity();
	memmove((char *)m_text + p_start, (char *)m_text + p_finish, m_text_size - p_finish);
	StoreTextCapacity(t_text_capacity);

	if (m_styles_size == NULL)
		return;

	uint4 t_lower, t_upper;
	SearchStyleEntries(p_start, p_finish, t_lower, t_upper);

	// At this point <p_lower> is the entry of the run immediately before or at <p_start>.
	// Since we are interested in the style *before* <p_start>, we adjust:
	if (p_start > 0 && GetStyleEntryOffset(m_styles[t_lower]) == p_start)
		t_lower -= 1;

	// At this point <t_upper> is the entry of the run immediately after ot at <p_finish>.
	// Since we are interested in the style *after* <p_finish>, we adjust:
	if (GetStyleEntryOffset(m_styles[t_upper]) != p_finish)
		t_upper -= 1;

	// Thus we have:
	//   t_lower is the entry with the style immediately preceeding p_start
	//   t_upper is the entry with the style immediately following p_finish
	// i.e.
	//   offset(t_lower) < p_start < offset(t_upper) <= p_finish

	// We now have two cases:
	//   1) the style at t_lower == the style at t_upper, in which case we remove
	//      all entries i s.t. t_lower < i <= t_upper
	//   2) the style at t_lower != the style at t_upper, in which case we delete
	//      all entries i s.t. t_lower < i < t_upper
	// After the deletion we offset all entries above t_lower by -(p_finish - p_start)

	if (GetStyleEntryIndex(m_styles[t_lower]) == GetStyleEntryIndex(m_styles[t_upper]))
		DeleteStyleEntries(t_lower + 1, t_upper - t_lower);
	else
		DeleteStyleEntries(t_lower + 1, t_upper - t_lower - 1);

	for(uint4 i = t_lower + 1; i < m_styles_size; ++i)
		m_styles[i] = ShiftStyleEntry(m_styles[i], -(int4)(p_finish - p_start));
}

void MCParagraph::Replace(uint4 p_start, uint4 p_finish, const MCParagraph *p_other, uint4 p_other_start, uint4 p_other_finish)
{
	assert(p_start <= p_finish);
	assert(p_finish <= m_text_size);

	assert(p_other_start <= p_other_finish);
	assert(p_other_finish <= p_other -> m_text_size);

	ReplaceText(p_start, p_finish, p_other -> m_text, p_other_start, p_other_finish);
	DeleteStyles(p_start, p_finish);

	uint4 t_at;
	SearchStyleEntries(p_start, t_at);

	uint4 *t_other_styles;
	t_other_styles = p_other -> m_styles;

	uint4 t_other_lower, t_other_upper;
	DoSearchStyleEntries(p_other -> m_styles,

}

void MCParagraph::Compact(void)
{
	if (m_text != NULL)
	{
		if (m_text_size == 0)
		{
			free(m_text);
			m_text = NULL;
		}
		else
			m_text = realloc(m_text, m_text_size);

		m_state &= ~kStateTextAvailable;
	}

	if (m_styles != NULL)
	{
		if (m_styles_size == 0)
		{
			free(m_styles);
			m_styles = NULL;
		}
		else if (m_styles_size == 2 && GetStyleEntryIndex(m_styles[0]) == 0)
		{
			free(m_styles);
			m_styles = NULL;
			m_styles_size = 0;
		}
		else
			m_styles = (uint4 *)realloc(m_styles, sizeof(uint4) * m_styles_size);

		m_state &= ~kStateStylesAvailable;
	}
}

///////////////////////////////////////////////////////////////////////////////

void MCParagraph::DestroyText(void)
{
	if (m_text == NULL)
		return;

	free(m_text);
	m_text_size = 0;
}


void MCParagraph::DestroyStyles(void)
{
	if (m_styles == NULL)
		return;

	for(uint4 i = 0; i < m_styles_size; ++i)
		ReleaseCharStyle(GetStyleEntryIndex(m_styles[i]));

	free(m_styles);
	m_styles_size = 0;
}

void MCParagraph::DestroyLines(void)
{
	if (m_lines == NULL)
		return;

	free(m_lines);
	m_lines_size = 0;
}

///////////////////////////////////////////////////////////////////////////////

static inline bool CloneBuffer(const void *p_buffer, uint2 p_buffer_size, void** r_new_buffer)
{
	if (p_buffer_size == 0)
	{
		*r_new_buffer = NULL;
		return true;
	}

	*r_new_buffer = malloc(p_buffer_size);
	if (*r_new_buffer == NULL)
		return false;

	memcpy(*r_new_buffer, p_buffer, p_buffer_size);

	return true;
}

void MCParagraph::CloneText(void*& r_text, uint2& r_text_size) const
{
	if (CloneBuffer(m_text, m_text_size, &r_text))
		r_text_size = m_text_size;
	else
		r_text_size = 0;
}

void MCParagraph::CloneStyles(uint4*& r_styles, uint2& r_styles_size) const
{
	if (CloneBuffer(m_styles, m_styles_size * sizeof(uint4), (void **)&r_styles))
		r_styles_size = m_styles_size;
	else
		r_styles_size = 0;
}

//

void MCParagraph::CopyText(uint4 p_start, uint4 p_finish, void*& r_text, uint2& r_text_size) const
{
	assert(p_finish <= m_text_size);
	assert(p_start <= p_finish);

	if (p_finish - p_start == 0)
	{
		r_text = NULL;
		r_text_size = 0;
		return;
	}

	if (CloneBuffer((char *)m_text + p_start, p_finish - p_start, &r_text))
		r_text_size = p_finish - p_start;
	else
		r_text_size = 0;
}

void MCParagraph::CopyStyles(uint4 p_start, uint4 p_finish, uint4*& r_styles, uint2& r_styles_size) const
{
	assert(p_finish <= m_text_size);
	assert(p_start <= p_finish);

	if (p_finish == p_start)
	{
		r_styles = NULL;
		r_styles_size = 0;
		return;
	}

	uint4 t_lower, t_higher;
	SearchStyleEntries(p_start, p_finish, t_lower, t_higher);
	
	if (t_lower == t_higher && GetStyleEntryIndex(m_styles[t_lower]) == 0)
	{
		r_styles = NULL;
		r_styles_size = 0;
		return;
	}

	if (CloneBuffer(m_styles + t_lower, t_higher - t_lower + 1, (void **)&r_styles))
	{
		r_styles_size = t_higher - t_lower + 1;
		for(uint4 i = 0; i < r_styles_size; ++i)
			m_styles[i] = MakeStyleEntry(GetStyleEntryOffset(m_styles[i]) - p_start, GetStyleEntryIndex(m_styles[i]));
	}
	else
		r_styles_size = 0;
}

//

void MCParagraph::ReplaceStyles(uint4 p_start, uint4 p_finish, uint4 p_style)
{
	assert(p_finish <= m_text_size);
	assert(p_start <= p_finish);

	// If the style run is of zero length, there is nothing to do.
	if (p_finish - p_start == 0)
		return;

	// If there are no styles currently set then to make other code easier
	// we special case.
	if (m_styles_size == 0)
	{
		m_styles = (uint4 *)malloc(sizeof(uint4) * 8);
		if (m_styles != NULL)
		{
			if (p_start > 0)
				m_styles[m_styles_size++] = MakeStyleEntry(0, 0);

			m_styles[m_styles_size++] = MakeStyleEntry(p_start, p_style);

			if (p_finish < m_text_size)
				m_styles[m_styles_size++] = MakeStyleEntry(p_finish, p_style);

			m_styles[m_styles_size++] = MakeStyleEntry(m_text_size, 0xffff);

			StoreStylesCapacity(8);
		}
		else
			ReleaseCharStyle(p_style);

		return;
	}

	// Locate the indices of the upper and lower indices. This call will compute
	//   t_lower s.t. m_styles[t_lower] . offset <= p_start < m_styles[t_lower + 1] . offset
	//   t_higher s.t. m_styles[t_higher - 1] . offset < p_finish <= m_styles[t_higher] . offset
	//
	uint4 t_lower, t_higher;
	SearchStyleEntries(p_start, p_finish, t_lower, t_higher);

	// At this point we have:
	//   +-----+-----+--...--+-----+-----+
	//   |     |     |       |     |     |
	//   |     |     |       |     |     |
	//   +-----+-----+--...--+-----+-----+
	//         A ^              ^  B
	// Where:
	//   - first ^ is p_start
	//   - second ^ is p_finish
	//   - A is offset(t_lower)
	//   - B is offset(t_higher)

	// Adjust start downwards if the lower entry has the same style
	bool t_start_split;
	if (GetStyleEntryIndex(m_styles[t_lower]) == p_style)
	{
		p_start = GetStyleEntryOffset(m_styles[t_lower]);
		t_start_split = false;
	}
	else
		t_start_split = true;

	// Adjust finish upwards if the upper entry has the same style
	bool t_finish_split;
	if (GetStyleEntryIndex(m_styles[t_higher - 1]) == p_style)
	{
		p_finish = GetStyleEntryOffset(m_styles[t_higher]);
		t_finish_split = false;
	}
	else
		t_finish_split = true;

	// At this point we can be sure that:
	//   m_styles[t_lower - 1] . style != p_style
	//   m_styles[t_higher] . style != p_style
	//   m_styles[t_lower] . offset <= p_start < p_finish <= m_styles[t_higher] . offset

	// Get the style used for the higher end of the split if relevant. Note that
	// we need to do some jiggery-pokery here in the case of splitting twice in
	// the same cell - if split in three, the start style will be duplicated.
	uint4 t_finish_style;
	if (t_finish_split)
	{
		t_finish_style = GetStyleEntryIndex(m_styles[t_higher - 1]);
		if (t_lower + 1 == t_higher)
		{
			if (t_start_split)
				RetainCharStyle(t_finish_style);
		}
	}
		
	// Compute the number of entries that need to be deleted/inserted
	int4 t_delta;
	t_delta = (int4)t_start_split + (int4)t_finish_split - (t_higher - t_lower - 1);

	if (t_delta < 0)
		DeleteStyleEntries(t_lower + 1, -t_delta);
	else
	{
		if (!InsertStyleEntries(t_lower + 1, t_delta))
		{
			ReleaseCharStyle(p_style);
			return;
		}
	}

	if (t_start_split)
		m_styles[t_lower + 1] = MakeStyleEntry(p_start, p_style);
	else
	{
		if (t_lower + 1 != t_higher || !t_finish_split)
			ReleaseCharStyle(GetStyleEntryIndex(m_styles[t_lower]));
		m_styles[t_lower] = MakeStyleEntry(p_start, p_style);
	}

	if (t_finish_split)
		m_styles[t_higher + t_delta - 1] = MakeStyleEntry(p_finish, t_finish_style);
}

//

void MCParagraph::DoSearchStyleEntries(const uint4 *p_styles, uint4 p_start, uint4 p_finish, uint4& r_lower, uint4& r_upper)
{
	uint4 i;
	for(i = 0; GetStyleEntryOffset(p_styles[i + 1]) < p_start; i++)
		;

	r_lower = i;

	for(i += 1; GetStyleEntryIndex(p_styles[i]) <= p_finish; i++)
		;

	r_upper = i;
}

void MCParagraph::SearchStyleEntries(uint4 p_start, uint4 p_finish, uint4& r_lower, uint4& r_upper) const
{
	assert(m_styles_size > 0);
	assert(p_finish <= m_text_size);
	assert(p_start <= p_finish);
	DoSearchStyleEntries(m_styles, p_start, p_finish, r_lower, r_upper);
}

void MCParagraph::DeleteStyleEntries(uint4 p_from, uint4 p_count)
{
	assert(p_from <= m_styles_size);
	assert(p_from + p_count <= m_styles_size);

	if (p_count == 0)
		return;

	uint4 t_capacity;
	t_capacity = FetchStylesCapacity();

	for(uint4 i = p_from; i < p_from + p_count; i++)
		ReleaseCharStyle(GetStyleEntryIndex(m_styles[i]));

	memmove(m_styles + p_from, m_styles + p_from + p_count, m_styles_size - p_from - p_count);
	m_styles_size -= p_count;

	StoreStylesCapacity(t_capacity);
}

bool MCParagraph::InsertStyleEntries(uint4 p_next, uint4 p_count)
{
	assert(p_next <= m_styles_size);
	assert(m_styles_size + p_count < 65536);

	if (p_count == 0)
		return true;

	uint4 t_capacity;
	t_capacity = FetchStylesCapacity();
	if (m_styles_size + p_count > t_capacity)
	{
		t_capacity = (t_capacity + p_count + 7) & ~8;

		uint4 *t_new_styles;
		t_new_styles = (uint4 *)realloc(m_styles, t_capacity * sizeof(uint4));
		if (t_new_styles == NULL)
			return false;

		m_styles = t_new_styles;
	}

	memmove(m_styles + p_next + p_count, m_styles + p_next, m_styles_size - p_next);
	m_styles_size += p_count;

	StoreStylesCapacity(t_capacity);

	return true;
}

uint4 MCParagraph::FetchStylesCapacity(void)
{
	if ((m_state & kStateStylesAvailable) != 0)
		return m_styles_size;

	return *((uint2 *)(m_styles + m_styles_size));
}

void MCParagraph::StoreStylesCapacity(uint4 p_capacity)
{
	assert(p_capacity < 65536);

	if (p_capacity == m_styles_size)
	{
		m_state &= ~kStateStylesAvailable;
		return;
	}

	m_state |= kStateStylesAvailable;
	*((uint2 *)(m_styles + m_styles_size)) = (uint2)p_capacity;
}

///////////////////////////////////////////////////////////////////////////////

static MCParagraphCharStyle **m_styles_table = NULL;
uint4 m_styles_table_size = 0;

uint4 MCParagraph::AcquireCharStyle(const MCParagraphCharStyle& p_style)
{
	int4 t_free;
	t_free = -1;
	for(uint4 i = 0; i < m_styles_table_size; ++i)
	{
		if (m_styles_table[i] != NULL)
		{
			if (memcmp(m_styles_table[i], &p_style, sizeof(MCParagraphCharStyle)) == 0)
				return i;
		}
		else if (t_free != -1)
			t_free = i;
	}

	if (t_free == -1)
	{
		t_free = m_styles_table_size;
		m_styles_table = (MCParagraphCharStyle **)realloc(m_styles_table, (m_styles_table_size + 256) * sizeof(MCParagraphCharStyle *));
		m_styles_table_size += 256;
	}

	m_styles_table[t_free] = new MCParagraphCharStyle;
	*m_styles_table[t_free] = p_style;

	return t_free;
}

const MCParagraphCharStyle *MCParagraph::FetchCharStyle(uint4 p_index)
{
	assert(p_index < m_styles_table_size);

	if (p_index == 0 && m_styles_table_size == 0)
	{
		MCParagraphCharStyle t_empty;
		memset(&t_empty, 0, sizeof(MCParagraphCharStyle));
		return m_styles_table[AcquireCharStyle(t_empty)];
	}
	
	return m_styles_table[p_index];
}

void MCParagraph::RetainCharStyle(uint4 p_index)
{
}

void MCParagraph::ReleaseCharStyle(uint4 p_index)
{
}

//

static const char **m_font_table = NULL;
uint4 m_font_table_size = 0;

uint4 MCParagraph::AcquireFont(const char *p_name)
{
	int4 t_free;
	t_free = -1;
	for(uint4 i = 0; i < m_font_table_size; ++i)
	{
		if (m_font_table[i] != NULL)
		{
			if (strcmp(m_font_table[i], p_name) == 0)
				return i;
		}
		else if (t_free != -1)
			t_free = i;
	}

	if (t_free == -1)
	{
		t_free = m_styles_table_size;
		m_font_table = (const char **)realloc(m_font_table, (m_font_table_size + 256) * sizeof(const char *));
		m_font_table_size += 256;
	}

	m_font_table[t_free] = strdup(p_name);

	return t_free;
}

void MCParagraph::ReleaseFont(uint4 p_index)
{
}

//

uint4 MCParagraph::AcquireLink(const char *p_text)
{
	return 0;
}

void MCParagraph::ReleaseLink(uint4 p_index)
{
}

//

uint4 MCParagraph::AcquireImage(const char *p_text)
{
	return 0;
}

void MCParagraph::ReleaseImage(uint4 p_index)
{
}

///////////////////////////////////////////////////////////////////////////////

MCParagraphPosition::MCParagraphPosition(MCParagraph *p_target)
{
	m_target = p_target;
	m_unicode = false;
	m_entry = 0;
	m_offset = 0;
}

MCParagraphPosition::MCParagraphPosition(MCParagraph *p_target, uint4 p_offset)
{
	m_target = p_target;

	m_offset = p_offset;

	if (m_target -> m_styles_size == 0)
	{
		m_entry = 0;
		m_unicode = false;
	}
	else
	{
		uint4 t_entry;
		m_target -> SearchStyleEntries(p_offset, t_entry);
		m_entry = t_entry;
		m_unicode = m_target -> FetchCharStyle(MCParagraph::GetStyleEntryIndex(m_target -> m_styles[m_entry])) -> is_unicode;
	}

	assert(m_unicode && (p_offset - MCParagraph::GetStyleEntryOffset(m_target -> m_styles[m_entry])) % 2 == 0);
}

MCParagraphPosition::MCParagraphPosition(const MCParagraphPosition& p_other)
{
	m_target = p_other . m_target;
	m_unicode = p_other . m_unicode;
	m_entry = p_other . m_entry;
	m_offset = p_other . m_offset;
}

MCParagraphPosition& MCParagraphPosition::operator = (const MCParagraphPosition& p_other)
{
	m_target = p_other . m_target;
	m_unicode = p_other . m_unicode;
	m_entry = p_other . m_entry;
	m_offset = p_other . m_offset;
	return *this;
}

bool MCParagraphPosition::Advance(void)
{
	if (m_offset == m_target -> m_text_size)
		return false;

	uint4 t_upper_offset;
	if (m_target -> m_styles == NULL)
		t_upper_offset = m_target -> m_text_size;
	else
		t_upper_offset = MCParagraph::GetStyleEntryOffset(m_target -> m_styles[m_entry + 1]);

	if (!m_unicode)
		m_offset += 1;
	else
	{
		m_offset += 2;
		if (m_offset > t_upper_offset)
			m_offset = t_upper_offset;
	}

	if (m_offset == t_upper_offset)
	{
		m_entry += 1;
		m_unicode = m_target -> FetchCharStyle(MCParagraph::GetStyleEntryIndex(m_target -> m_styles[m_entry])) -> is_unicode;
	}

	return true;
}

bool MCParagraphPosition::Retreat(void)
{
	if (m_offset == 0)
		return false;

	uint4 t_current_offset;
	if (m_target -> m_styles == NULL)
		t_current_offset = 0;
	else
		t_current_offset = MCParagraph::GetStyleEntryOffset(m_target -> m_styles[m_entry]);

	if (m_offset == t_current_offset)
	{
		m_entry -= 1;
		m_unicode = m_target -> FetchCharStyle(MCParagraph::GetStyleEntryIndex(m_target -> m_styles[m_entry])) -> is_unicode;
		if (m_unicode)
		{
			uint4 t_entry_length;
			t_entry_length = t_current_offset - MCParagraph::GetStyleEntryOffset(m_target -> m_styles[m_entry]);
			if (t_entry_length % 2 != 0)
				m_offset -= 1;
			else
				m_offset -= 2;
		}
		else
			m_offset -= 1;
	}
	else if (m_unicode)
		m_offset -= 2;
	else
		m_offset -= 1;

	return true;
}

uint4 MCParagraphPosition::GetNativeCodepoint(void) const
{
	void *t_ptr;
	t_ptr = (uint1 *)m_target -> m_text + m_offset;

	if (m_unicode)
		return *(uint2 *)t_ptr;

	return *(uint1 *)t_ptr;
}

uint4 MCParagraphPosition::GetUTF16Codepoint(void) const
{
	uint4 t_native;
	t_native = GetNativeCodepoint();
	if (m_unicode)
		return t_native;
	
	return MCUnicodeMapFromNative(t_native);
}

uint4 MCParagraphPosition::GetCodepoint(void) const
{
	if (!m_unicode)
		return MCUnicodeMapFromNative(GetNativeCodepoint());

	uint4 t_codepoint;
	t_codepoint = GetUTF16Codepoint();
	if (MCUnicodeCodepointIsHighSurrogate(t_codepoint))
	{
		uint4 t_upper_offset;
		t_upper_offset = MCParagraph::GetStyleEntryOffset(m_target -> m_styles[m_entry + 1]);
		if (t_upper_offset - m_offset >= 2)
		{
			uint4 t_other_codepoint;
			t_other_codepoint = *((uint2 *)((uint1 *)m_target -> m_text + m_offset + 2));
			if (MCUnicodeCodepointIsLowSurrogate(t_other_codepoint))
				return ((t_codepoint - 0xD800) << 10) | (t_other_codepoint - 0xDC00);
		}
	}

	return t_codepoint;
}

///////////////////////////////////////////////////////////////////////////////

MCParagraphCursor::MCParagraphCursor(MCParagraph *p_target)
	: m_start(p_target),
	  m_finish(p_target)
{
	m_target = p_target;
}

MCParagraphCursor::MCParagraphCursor(MCParagraph *p_target, uint4 p_index)
	: m_start(p_target, p_index),
	  m_finish(p_target, p_index)
{
	m_target = p_target;
}

MCParagraphCursor::MCParagraphCursor(MCParagraph *p_target, uint4 p_start, uint4 p_finish)
	: m_start(p_target, p_start),
	  m_finish(p_target, p_finish)
{
	m_target = p_target;
}

void MCParagraphCursor::Collapse(int4 p_delta)
{
	if (p_delta < 0)
		m_start = m_finish;
	else if (p_delta > 0)
		m_finish = m_start;
}

bool MCParagraphCursor::Move(MCParagraphCursorMove p_movement, int4 p_delta)
{
	if (p_delta == 0)
		return true;

	bool t_moved;
	if (p_delta < 0)
		t_moved = MoveStart(p_movement, p_delta);
	else if (p_delta > 0)
		t_moved = MoveFinish(p_movement, p_delta);

	Collapse(p_delta);

	return t_moved;
}

bool MCParagraphCursor::MoveStart(MCParagraphCursorMove p_movement, int4 p_delta)
{
	if (p_delta == 0)
		return true;

	return MoveIndex(m_start, p_movement, p_delta); 
}

bool MCParagraphCursor::MoveFinish(MCParagraphCursorMove p_movement, int4 p_delta)
{
	if (p_delta == 0)
		return true;

	return MoveIndex(m_finish, p_movement, p_delta); 
}

bool MCParagraphCursor::MoveIndex(MCParagraphPosition& x_position, MCParagraphCursorMove p_movement, int4 p_delta)
{
	bool t_moved;
	if (p_delta < 0)
	{
		MCParagraphPosition t_right_position(x_position);

		t_moved = t_right_position . Retreat();
		while(p_delta != 0 && t_moved)
		{
			MCParagraphPosition t_left_position(t_right_position);

			uint4 t_left_codepoint;
			if (!t_left_position . Retreat())
			{
				t_left_codepoint = 0xffffffff;
				t_moved = false;
			}
			else
				t_left_codepoint = t_left_position . GetCodepoint();

			if (BoundaryMatch(p_movement, t_left_codepoint, t_right_position . GetCodepoint()))
				p_delta += 1;
		}

		if (t_moved || p_delta == 0)
			x_position = t_right_position;
	}
	else
	{
		MCParagraphPosition t_left_position(x_position);
		MCParagraphPosition t_right_position(t_left_position);

		t_moved = true;
		while(p_delta != 0 && t_moved)
		{
			uint4 t_right_codepoint;
			if (!t_right_position . Advance())
			{
				t_right_codepoint = 0xffffffff;
				t_moved = false;
			}
			else
				t_right_codepoint = t_right_position . GetCodepoint();

			if (BoundaryMatch(p_movement, t_left_position . GetCodepoint(), t_right_codepoint))
				p_delta -= 1;
		}

		if (t_moved || p_delta == 0)
			x_position = t_right_position;
	}

	return t_moved;
}

MCParagraph *MCParagraphCursor::Clone(void)
{
	return new MCParagraph(*m_target, m_start . GetOffset(), m_finish . GetOffset());
}

void MCParagraphCursor::Delete(void)
{
	m_target -> Delete(m_start . GetOffset(), m_finish . GetOffset());
}

void MCParagraphCursor::Replace(const MCParagraphCursor& p_other)
{
	m_target -> Replace(m_start . GetOffset(), m_finish . GetOffset(), p_other . m_target, p_other . m_start . GetOffset(), p_other . m_finish . GetOffset());
}

///////////////////////////////////////////////////////////////////////////////

#if 0
			if ((t_block_flags & F_FONT) != 0)
			{
				const char *t_font_name;
				uint2 t_font_size;
				uint2 t_font_style;
				m_parent -> getstack() -> lookupfontcache(t_font_index, t_font_name, t_font_size, t_font_style);

				t_style . font = AcquireFont(t_font_name):

				t_style . size = t_font_size;

				if ((t_font_style & FA_ITALIC) != 0 || (t_font_style & FA_OBLIQUE) != 0)
					t_style . italic = kMCParagraphCharStyleItalicOn;
				else
					t_style . italic = kMCParagraphCharStyleItalicOff;

				if ((t_font_style & FA_WEIGHT) > 0x05)
					t_style . bold = kMCParagraphCharStyleBoldOn;
				else
					t_style . bole = kMCParagraphCharStyleBoldOff;

				if ((t_font_style & FA_EXPAND) > 0x50)
					t_style . expand = kMCParagraphCharStyleExtend;
				else if ((t_font_style & FA_EXPAND) < 0x50)
					t_style . expand = kMCParagraphCharStyleCondense;
				else
					t_style . expand = kMCParagraphCharStyleNone;

				if ((t_font_style & FA_LINK) != 0)
					t_style . link = kMCParagraphCharStyleLinkNormal;
				else
					t_style . link = kMCParagraphCharStyleLinkNone;

				if ((t_font_style & FA_UNDERLINE) != 0)
					t_style . underline = kMCParagraphCharStyleSingle;
				else
					t_style . underline = kMCParagraphCharStyleNone;

				if ((t_font_style & FA_STRIKE) != 0)
					t_style . strikeout = kMCParagraphCharStyleSingle;
				else
					t_style . strikeout = kMCParagraphCharStyleNone;

				if ((t_font_style & FA_3D_BOX) != 0)
					t_style . box = kMCParagraphCharStyleBoxRaised;
				else if ((t_font_style & FA_BOX) != 0)
					t_style . box = kMCParagraphCharStyleBoxFlat;
				else
					t_style . box = kMCParagraphCharStyleNone;
			}
#endif


#if 0
	if (GetStyleEntryOffset(t_lower_entry) == p_start)
	{
		// No need to split lower end
		ReleaseCharStyle(GetStyleEntryIndex(t_lower));
		m_styles[t_lower] = MakeStyleEntry(GetStyleEntryOffset(t_lower), p_style);

		if (GetStyleEntryOffset(t_higher_entry) == p_finish)
		{
			// No need to split higher end - delete all entries between t_lower
			// and t_higher.
			DeleteStyleEntries(t_lower + 1, t_higher);
		}
		else
		{
			// Split the higher end
			if (t_lower + 1 < t_higher)
			{
				// There is at least one entry between t_lower and t_higher, so
				// delete all others
				DeleteStyleEntries(t_lower + 2, t_higher - 1)
			}
			else
			{
				// There is no entry between t_lower and t_higher, so insert one
				InsertStyleEntries(t_higher, 1);
			}
		}
	}
	else
	{
	}

	if (m_styles[t_lower] . offset != p_start)
	{

	}

	// We now must:
	//   1) Insert an entry (p_start, p_style) after m_styles[t_lower]
	//   2) Insert an entry (p_finish, m_styles[t_higher]) after m_styles[t_higher]
	//   3) Delete all entries t_lo

	uint4 t_lower_entry;
	t_lower_entry = m_styles[t_lower];
	if (GetStyleEntryOffset(t_lower_entry) < p_offset && GetStyleEntryIndex(t_lower_entry) != p_style)
	{
		t_lower_entry = MakeStyleEntry(GetStyleEntryIndex(t_lower_entry)

	uint4 t_higher_entry
#endif

#if 0
	if (p_offset < GetStyleEntryOffset(m_styles[0]))
	{
		if (p_offset + p_length < GetStyleEntryOffset(m_styles[0]))
		{
			if (InsertStyleEntries(0, 2))
			m_styles[0] = MakeStyleEntry(p_offset
		}

	if (p_offset < GetStyleEntryOffset(m_styles[0]))
	{
		if (p_style == GetStyleEntryIndex(m_styles[0]))
			m_styles[0] = MakeStyleEntry(p_offset, 
		return;
	}
#endif
