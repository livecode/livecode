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

#include "text.h"
#include "rtf.h"

RTFState::RTFState(void)
{
	m_entries = NULL;
}

RTFState::~RTFState(void)
{
	while(m_entries != NULL)
		Restore();
}

RTFStatus RTFState::Save(void)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	Entry *t_new_entry;
	t_new_entry = NULL;
	if (t_status == kRTFStatusSuccess)
	{
		t_new_entry = new (nothrow) Entry;
		if (t_new_entry == NULL)
			t_status = kRTFStatusOverflow;
	}
	
	if (t_status == kRTFStatusSuccess)
	{
		t_new_entry -> previous = m_entries;
		if (m_entries != NULL)
		{
			t_new_entry -> destination = m_entries -> destination;
			t_new_entry -> text_encoding = m_entries -> text_encoding;
			t_new_entry -> font_name = m_entries -> font_name;
			t_new_entry -> font_style = m_entries -> font_style;
			t_new_entry -> font_size = m_entries -> font_size;
			t_new_entry -> foreground_color = m_entries -> foreground_color;
			t_new_entry -> background_color = m_entries -> background_color;
			t_new_entry -> unicode_skip = m_entries -> unicode_skip;
			t_new_entry -> list_style = m_entries -> list_style;
			t_new_entry -> list_level = m_entries -> list_level;
			t_new_entry -> list_index = m_entries -> list_index;

			// MW-2012-03-14: [[ RtfParaStyles ]] Copy across all the previous paragraph styling.
			t_new_entry -> text_align = m_entries -> text_align;
			t_new_entry -> border_width = m_entries -> border_width;
			t_new_entry -> padding = m_entries -> padding;
			t_new_entry -> first_indent = m_entries -> first_indent;
			t_new_entry -> left_indent = m_entries -> left_indent;
			t_new_entry -> right_indent = m_entries -> right_indent;
			t_new_entry -> space_above = m_entries -> space_above;
			t_new_entry -> space_below = m_entries -> space_below;
			t_new_entry -> paragraph_background_color = m_entries -> paragraph_background_color;
			t_new_entry -> border_color = m_entries -> border_color;

            /* UNCHECKED */ MCStringCopy(m_entries -> metadata, t_new_entry -> metadata);
            /* UNCHECKED */ MCStringCopy(m_entries -> paragraph_metadata, t_new_entry -> paragraph_metadata);
            t_new_entry->hyperlink = MCValueRetain(m_entries -> hyperlink);
		}
		else
		{
			t_new_entry -> destination = kRTFDestinationNormal;
			t_new_entry -> text_encoding = kMCTextEncodingUndefined;
			t_new_entry -> font_name = NULL;
			t_new_entry -> font_style = 0;
			t_new_entry -> font_size = 0;
			t_new_entry -> foreground_color = 0xffffffff;
			t_new_entry -> background_color = 0xffffffff;
			t_new_entry -> unicode_skip = 1;
			t_new_entry -> list_style = kMCTextListStyleNone;
			t_new_entry -> list_level = 0;
			t_new_entry -> list_index = 0;

			// MW-2012-03-14: [[ RtfParaStyles ]] Set all the paragraph styling to defaults.
			t_new_entry -> text_align = kMCTextTextAlignLeft;
			t_new_entry -> border_width = 0;
			t_new_entry -> padding = 0;
			t_new_entry -> first_indent = 0;
			t_new_entry -> left_indent = 0;
			t_new_entry -> right_indent = 0;
			t_new_entry -> space_above = 0;
			t_new_entry -> space_below = 0;
			t_new_entry -> paragraph_background_color = 0xffffffff;
			t_new_entry -> border_color = 0xffffffff;

            t_new_entry -> metadata = MCValueRetain(kMCEmptyString);
            t_new_entry -> paragraph_metadata = MCValueRetain(kMCEmptyString);
            t_new_entry -> hyperlink = MCValueRetain(kMCEmptyName);
		}
		
		m_entries = t_new_entry;
	}
	
	return t_status;
}

RTFStatus RTFState::Restore(void)
{
	RTFStatus t_status;
	t_status = kRTFStatusSuccess;
	
	if (t_status == kRTFStatusSuccess)
	{
		if (m_entries == NULL)
			t_status = kRTFStatusUnderflow;
	}
	
	if (t_status == kRTFStatusSuccess)
	{
		Entry *t_entry;
		t_entry = m_entries;
		m_entries = t_entry -> previous;
		
        MCValueRelease(t_entry -> metadata);
        MCValueRelease(t_entry -> paragraph_metadata);
		MCValueRelease(t_entry -> hyperlink);

		delete t_entry;
	}
	
	return t_status;
}

// MW-2014-01-08: [[ Bug 11627 ]] Used to detect if a paragraph needs to be emiited
//   on change of group (otherwise paragraph state is lost if there has been no
//   style changes on text within the group).
bool RTFState::HasParagraphChanged(void) const
{
	if (m_entries == nil)
		return false;
	
	if (m_entries -> previous == nil)
		return
			m_entries -> list_style != kMCTextListStyleNone || m_entries -> list_level != 0 || m_entries -> list_index != 0 ||
			m_entries -> text_align != kMCTextTextAlignLeft || m_entries -> border_width != 0 || m_entries -> padding != 0 ||
			m_entries -> first_indent != 0 || m_entries -> left_indent != 0 || m_entries -> right_indent != 0 ||
			m_entries -> space_above != 0 || m_entries -> space_below != 0 ||
			m_entries -> paragraph_background_color != 0xffffffff ||
			m_entries -> border_color !=0xffffffff ||
			m_entries -> paragraph_metadata != kMCEmptyString;
	
	return m_entries -> list_style != m_entries -> previous -> list_style ||
			m_entries -> list_level != m_entries -> previous -> list_level ||
			m_entries -> list_index != m_entries -> previous -> list_index ||
			m_entries -> text_align != m_entries -> previous -> text_align ||
			m_entries -> border_width != m_entries -> previous -> border_width ||
			m_entries -> padding != m_entries -> previous -> padding ||
			m_entries -> first_indent != m_entries -> previous -> first_indent ||
			m_entries -> left_indent != m_entries -> previous -> left_indent ||
			m_entries -> right_indent != m_entries -> previous -> right_indent ||
			m_entries -> space_above != m_entries -> previous -> space_above ||
			m_entries -> space_below != m_entries -> previous -> space_below ||
			m_entries -> paragraph_background_color != m_entries -> previous -> paragraph_background_color ||
			m_entries -> border_color != m_entries -> previous -> border_color ||
			m_entries -> paragraph_metadata != m_entries -> previous -> paragraph_metadata;
}

//

RTFFontTable::RTFFontTable(void)
{
	m_entries = NULL;
	m_entry_count = 0;
}

RTFFontTable::~RTFFontTable(void)
{
	for(uint4 i = 0; i < m_entry_count; ++i)
	{
		if (m_entries[i] . name != NULL)
			free(m_entries[i] . name);
	}
	
	delete m_entries;
}

RTFStatus RTFFontTable::Define(uint4 p_index, char *p_name, uint4 p_charset)
{
	if (!Extend())
		return kRTFStatusNoMemory;
		
	m_entries[m_entry_count] . index = p_index;
	m_entries[m_entry_count] . name = p_name;
	m_entries[m_entry_count] . charset = p_charset;
	
	m_entry_count += 1;

	return kRTFStatusSuccess;
}

bool RTFFontTable::Extend(void)
{
	Entry *t_new_entries;
	t_new_entries = (Entry *)realloc(m_entries, sizeof(Entry) * (m_entry_count + 1));
	if (t_new_entries == NULL)
		return false;
	
	m_entries = t_new_entries;

	return true;
}

bool RTFFontTable::Find(uint4 p_index, uint4& r_location) const
{
	for(uint4 i = 0; i < m_entry_count; ++i)
		if (m_entries[i] . index == p_index)
		{
			r_location = i;
			return true;
		}
		
	r_location = m_entry_count;

	return false;
}

////////////////////////////////////////////////////////////////////////

RTFListTable::RTFListTable(void)
{
	m_lists = NULL;
	m_list_count = 0;
}

RTFListTable::~RTFListTable(void)
{
	if (m_lists != NULL)
		free(m_lists);
}

RTFStatus RTFListTable::Define(void)
{
	if (!Extend())
		return kRTFStatusNoMemory;
		
	m_lists[m_list_count] . list_id = -1;
	for(uint32_t i = 0; i < 10; i++)
		m_lists[m_list_count] . list_styles[i]= kMCTextListStyleNone;
	m_list_count += 1;
	
	return kRTFStatusSuccess;
}

void RTFListTable::SetId(int4 p_id)
{
	if (m_list_count == 0)
		return;
	
	m_lists[m_list_count - 1] . list_id = p_id;
}

void RTFListTable::SetStyle(uint32_t p_level, MCTextListStyle p_style)
{
	if (m_list_count == 0)
		return;
	
	p_level = MCU_min(MCU_max(p_level, 1U), 9U);

	if (m_lists[m_list_count - 1] . list_styles[p_level - 1] == kMCTextListStyleNone)
		m_lists[m_list_count - 1] . list_styles[p_level - 1] = p_style;
}

bool RTFListTable::Get(int4 p_id, uint32_t p_level, MCTextListStyle& r_style)
{
	p_level = MCU_min(MCU_max(p_level, 1U), 9U);

	for(uint32_t i = 0; i < m_list_count; i++)
		if (m_lists[i] . list_id == p_id)
		{
			r_style = m_lists[i] . list_styles[p_level - 1];
			return true;
		}
	
	return false;
}

bool RTFListTable::Extend(void)
{
	Entry *t_new_lists;
	t_new_lists = (Entry *)realloc(m_lists, sizeof(Entry) * (m_list_count + 1));
	if (t_new_lists == NULL)
		return false;
	
	m_lists = t_new_lists;
	
	return true;
}

////////////////////////////////////////////////////////////////////////

RTFListOverrideTable::RTFListOverrideTable(void)
{
	m_overrides = NULL;
	m_override_count = 0;
}

RTFListOverrideTable::~RTFListOverrideTable(void)
{
	if (m_overrides != NULL)
		free(m_overrides);
}

RTFStatus RTFListOverrideTable::Define(void)
{
	if (!Extend())
		return kRTFStatusNoMemory;
	
	m_overrides[m_override_count] . list_id = -1;
	m_overrides[m_override_count] . override_id = -1;
	m_override_count += 1;
	
	return kRTFStatusSuccess;
}

void RTFListOverrideTable::SetListId(int4 p_id)
{
	if (m_override_count == 0)
		return;
	
	m_overrides[m_override_count - 1] . list_id = p_id;
}

void RTFListOverrideTable::SetOverrideId(int4 p_id)
{
	if (m_override_count == 0)
		return;
	
	m_overrides[m_override_count - 1] . override_id = p_id;
}

bool RTFListOverrideTable::Get(int4 p_id, int4& r_list_id)
{
	for(uint32_t i = 0; i < m_override_count; i++)
		if (m_overrides[i] . override_id == p_id)
		{
			r_list_id = m_overrides[i] . list_id;
			return true;
		}
	
	return false;
}

bool RTFListOverrideTable::Extend(void)
{
	Entry *t_new_overrides;
	t_new_overrides = (Entry *)realloc(m_overrides, sizeof(Entry) * (m_override_count + 1));
	if (t_new_overrides == NULL)
		return false;
	
	m_overrides = t_new_overrides;
	
	return true;
}

////////////////////////////////////////////////////////////////////////

RTFColorTable::RTFColorTable(void)
{
	m_colors = NULL;
	m_color_count = 0;
}

RTFColorTable::~RTFColorTable(void)
{
	if (m_colors != NULL)
		free(m_colors);
}

RTFStatus RTFColorTable::Define(uint4 p_color)
{
	if (!Extend())
		return kRTFStatusNoMemory;
	
	m_colors[m_color_count] = p_color;
	m_color_count += 1;
	
	return kRTFStatusSuccess;
}

bool RTFColorTable::Extend(void)
{
	uint4 *t_new_colors;
	t_new_colors = (uint4 *)realloc(m_colors, sizeof(uint4) * (m_color_count + 1));
	if (t_new_colors == NULL)
		return false;
	
	m_colors = t_new_colors;
	
	return true;
}

bool RTFText::Flush(void)
{
	void *t_output;
	uint4 t_output_limit;
	m_output . Borrow(t_output, t_output_limit);

	uint4 t_used;
	if (!MCTextEncodeToUnicode(m_input_encoding, m_input . GetBase(), m_input . GetLength(), t_output, t_output_limit, t_used))
	{
		if (!m_output . Ensure(t_used))
			return false;

		m_output . Borrow(t_output, t_output_limit);
		MCTextEncodeToUnicode(m_input_encoding, m_input . GetBase(), m_input . GetLength(), t_output, t_output_limit, t_used);
	}

	m_output . Advance(t_used);

	m_input . Clear();

	return true;
}

////////////////////////////////////////////////////////////////////////
