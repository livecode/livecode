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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "object.h"
#include "MCBlock.h"
#include "button.h"
#include "graphic.h"
#include "group.h"
#include "mcio.h"
#include "field.h"
#include "styledtext.h"
#include "font.h"
#include "mcstring.h"

#include "stackfileformat.h"

////////////////////////////////////////////////////////////////////////////////

struct MCLogicalFontTableEntry
{
	MCNameRef textfont;
	uint2 textstyle;
	uint2 textsize;
};

inline bool operator== (const MCLogicalFontTableEntry& a, const MCLogicalFontTableEntry& b)
{
    return a.textfont == b.textfont
            && a.textstyle == b.textstyle
            && a.textsize == b.textsize;
}

static uint32_t s_logical_font_table_size = 0;
static uint32_t s_logical_font_table_capacity = 0;
static MCLogicalFontTableEntry *s_logical_font_table = nil;

////////////////////////////////////////////////////////////////////////////////

static void MCLogicalFontTableGetEntry(uint32_t p_index, MCNameRef& r_textfont, uint2& r_textstyle, uint2& r_textsize, bool& r_unicode)
{
	r_textfont = s_logical_font_table[p_index] . textfont;
	r_textstyle = s_logical_font_table[p_index] . textstyle;
	r_textsize = s_logical_font_table[p_index] . textsize & 0x7fff;
	r_unicode = (s_logical_font_table[p_index] . textsize & 0x8000) != 0;
}

static void MCLogicalFontTableSetEntry(uint32_t p_index, MCNameRef p_textfont, uint2 p_textstyle, uint2 p_textsize, bool p_unicode)
{
    s_logical_font_table[p_index] . textfont = p_textfont;
	s_logical_font_table[p_index] . textstyle = p_textstyle;
	s_logical_font_table[p_index] . textsize = p_textsize & 0x7fff;
	if (p_unicode)
		s_logical_font_table[p_index] . textsize |= 0x8000;
}

static uint32_t MCLogicalFontTableLookupEntry(MCNameRef p_textfont, uint2 p_textstyle, uint2 p_textsize, bool p_unicode, bool p_add)
{
	MCLogicalFontTableEntry t_entry;
	t_entry . textfont = p_textfont;
	t_entry . textstyle = p_textstyle;
	t_entry . textsize = p_textsize & 0x7fff;
	if (p_unicode)
		t_entry . textsize |= 0x8000;
	
	for(uint32_t i = 0; i < s_logical_font_table_size; i++)
		if (t_entry == s_logical_font_table[i])
			return i;

	if (p_add)
	{
		if (s_logical_font_table_size == s_logical_font_table_capacity)
			/* UNCHECKED */ MCMemoryResizeArray(s_logical_font_table_capacity > 0 ? s_logical_font_table_capacity * 2 : 32, s_logical_font_table, s_logical_font_table_capacity);
		
		s_logical_font_table[s_logical_font_table_size++] = t_entry;
        MCValueRetain(t_entry.textfont);
	}

	return s_logical_font_table_size - 1;
}

////////////////////////////////////////////////////////////////////////////////

class MCLogicalFontTableBuildVisitor: public MCObjectVisitor
{
public:
	MCLogicalFontTableBuildVisitor(void)
	{
		m_current_textfont = nil;
		m_current_textsize = 0;
		m_current_textstyle = 0;
	}

	~MCLogicalFontTableBuildVisitor(void)
	{
	}

	bool OnObject(MCObject *p_object)
	{
		// MW-2012-02-19: [[ SplitTextAttrs ]] If the object needs to save a
		//   font record, then register an entry in the font table.
		if (p_object -> needtosavefontrecord())
		{
			MCNameRef t_textfont;
			uint2 t_textsize;
			uint2 t_textstyle;
			p_object -> getfontattsnew(t_textfont, t_textsize, t_textstyle);
			MCLogicalFontTableLookupEntry(t_textfont, t_textstyle, t_textsize, p_object -> hasunicode(), true);
		}

		return true;
	}

	bool OnField(MCField *p_field)
	{
		// Record the current font atts for the field, in case blocks need to
		// inherit them.
		p_field -> getfontattsnew(m_current_textfont, m_current_textsize, m_current_textstyle);

		// MW-2012-02-19: [[ Bug ]] Make sure we call the object method so that
		//   the field's font details (if any) can be recorded.
		return OnObject(p_field);
	}

	bool OnStyledText(MCStyledText *p_styled_text)
	{
		// Record the current font atts for the field, in case blocks need to
		// inherit them.
		p_styled_text -> getfontattsnew(m_current_textfont, m_current_textsize, m_current_textstyle);

		return true;
	}

	bool OnBlock(MCBlock *p_block)
	{
		// MW-2012-02-17: [[ SplitTextAttrs ]] Fetch each of the font attrs from the block in
		//   turn. Falling back on the parent attrs we have if necessary.
		MCNameRef t_textfont;
		uint2 t_textsize;
		uint2 t_textstyle;
		if (!p_block -> gettextfont(t_textfont))
			t_textfont = m_current_textfont;
		if (!p_block -> gettextsize(t_textsize))
			t_textsize = m_current_textsize;
		if (!p_block -> gettextstyle(t_textstyle))
			t_textstyle = m_current_textstyle;

		MCLogicalFontTableLookupEntry(t_textfont, t_textstyle, t_textsize, true, true);
		
		return true;
	}

private:
	MCNameRef m_current_textfont;
	uint2 m_current_textsize;
	uint2 m_current_textstyle;
};

////////////////////////////////////////////////////////////////////////////////

bool MCLogicalFontTableInitialize(void)
{
	s_logical_font_table_size = 0;
	s_logical_font_table_capacity = 0;
	s_logical_font_table = nil;
	return true;
}

void MCLogicalFontTableFinalize(void)
{
	MCLogicalFontTableFinish();
}

void MCLogicalFontTableBuild(MCObject *p_object, uint32_t p_part)
{
	// Clear any existing font table.
	MCLogicalFontTableFinish();

	// Visit the object in depth last order - this ensures fields are found
	// before their blocks.
	MCLogicalFontTableBuildVisitor t_visitor;
	p_object -> visit(VISIT_STYLE_DEPTH_LAST, p_part, &t_visitor);
}

void MCLogicalFontTableFinish(void)
{
	for(uint32_t i = 0; i < s_logical_font_table_size; i++)
	{
		MCNameRef t_textfont;
		uint2 t_textstyle;
		uint2 t_textsize;
		bool t_unicode;
		MCLogicalFontTableGetEntry(i, t_textfont, t_textstyle, t_textsize, t_unicode);
		MCValueRelease(t_textfont);
	}
	MCMemoryDeleteArray(s_logical_font_table);

	s_logical_font_table_size = 0;
	s_logical_font_table_capacity = 0;
	s_logical_font_table = nil;
}

IO_stat MCLogicalFontTableLoad(IO_handle p_stream, uint32_t p_version)
{
	// Delete any existing font table.
	MCLogicalFontTableFinish();

	// Keep track of any IO errors.
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	// Read the number of entries.
	uint2 t_count;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint2(&t_count, p_stream);

	// Allocate an array big enough to hold all of them.
	if (t_stat == IO_NORMAL)
		if (!MCMemoryNewArray(t_count, s_logical_font_table))
			t_stat = IO_ERROR;

	if (t_stat == IO_NORMAL)
	{
		// The size and capacity of the table are equal to the count.
		s_logical_font_table_size = s_logical_font_table_capacity = t_count;

		// Iterate t_count times to load each entry in.
		for(uint32_t i = 0; i < t_count && t_stat == IO_NORMAL; i++)
		{
			MCAutoStringRef t_textfont_string;
			uint2 t_textsize, t_textstyle;

			// Read in the textSize, textStyle and (original) textFont props.
			t_stat = IO_read_uint2(&t_textsize, p_stream);
			if (t_stat == IO_NORMAL)
				t_stat = IO_read_uint2(&t_textstyle, p_stream);
			
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
			if (t_stat == IO_NORMAL)
				t_stat = IO_read_stringref_new(&t_textfont_string, p_stream, p_version >= kMCStackFileFormatVersion_7_0);

			// Now convert the textFont string into a name, splitting off the
			// lang tag (if any).
			MCAutoStringRef t_textfont;
			bool t_is_unicode;
			if (t_stat == IO_NORMAL)
			{
				uindex_t t_offset;
				if (MCStringFirstIndexOfChar(*t_textfont_string, ',', 0, kMCStringOptionCompareExact, t_offset))
				{
					if (!MCStringCopySubstring(*t_textfont_string, MCRangeMake(0, t_offset), &t_textfont))
						t_stat = IO_ERROR;
					t_is_unicode = true;
				}
				else
				{
					t_textfont = *t_textfont_string;
					t_is_unicode = false;
				}

			}
				
			MCNameRef t_textfont_name;
			if (t_stat == IO_NORMAL)
				if (!MCNameCreate(*t_textfont, t_textfont_name))
					t_stat = IO_ERROR;

			// Set the appropriate table entry.
			if (t_stat == IO_NORMAL)
				MCLogicalFontTableSetEntry(i, t_textfont_name, t_textstyle, t_textsize, t_is_unicode);
		}
	}

	return t_stat;
}

IO_stat MCLogicalFontTableSave(IO_handle p_stream, uint32_t p_version)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint2(s_logical_font_table_size, p_stream);
	
	if (t_stat == IO_NORMAL)
		for(uint32_t i = 0; i < s_logical_font_table_size && t_stat == IO_NORMAL; i++)
		{
			MCNameRef t_textfont;
			uint2 t_textstyle;
			uint2 t_textsize;
			bool t_is_unicode;
			MCLogicalFontTableGetEntry(i, t_textfont, t_textstyle, t_textsize, t_is_unicode);

			if (t_textfont == nil)
				t_textfont = kMCEmptyName;

			t_stat = IO_write_uint2(t_textsize, p_stream);
			if (t_stat == IO_NORMAL)
				t_stat = IO_write_uint2(t_textstyle, p_stream);

			// MW-2012-05-03: [[ Values* ]] Use a stringref and format to build
			//   the actual font name.
			MCAutoStringRef t_unicode_textfont;
			if (t_stat == IO_NORMAL)
			{
				if (t_is_unicode)
				{
                    if (!MCStringFormat(&t_unicode_textfont, "%@,unicode", MCNameGetString(t_textfont)))
						t_stat = IO_ERROR;
				}
				else
					t_unicode_textfont = MCNameGetString(t_textfont);
			}
			
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
            if (t_stat == IO_NORMAL)
                t_stat = IO_write_stringref_new(*t_unicode_textfont, p_stream, p_version >= kMCStackFileFormatVersion_7_0);
		}

	return t_stat;
}

uint16_t MCLogicalFontTableMap(MCNameRef p_font, uint16_t p_style, uint16_t p_size, bool p_is_unicode)
{
	return MCLogicalFontTableLookupEntry(p_font, p_style, p_size, p_is_unicode, false);
}

void MCLogicalFontTableLookup(uint16_t p_index, MCNameRef& r_font, uint16_t& r_style, uint16_t& r_size, bool& r_is_unicode)
{
	// MW-2012-02-17: [[ LogFonts ]] It appears that it is possible for objects
	//   to be saved with invalid font indices from previous engines, rather than
	//   abort we take the previous engines behavior - substitute the default
	//   font!
	if (p_index < s_logical_font_table_size)
		MCLogicalFontTableGetEntry(p_index, r_font, r_style, r_size, r_is_unicode);
	else
	{
		r_font = MCN_default_text_font;
		r_style = FA_DEFAULT_STYLE;
		r_size = DEFAULT_TEXT_SIZE;
		r_is_unicode = false;
	}

}

////////////////////////////////////////////////////////////////////////////////

