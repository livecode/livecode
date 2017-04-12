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
#include "parsedef.h"
#include "objdefs.h"
#include "mcio.h"

#include "stack.h"
#include "MCBlock.h"
#include "line.h"
#include "field.h"
#include "paragraf.h"

#include "util.h"
#include "styledtext.h"

MCStyledText::MCStyledText(void)
{
	m_paragraphs = NULL;
}

MCStyledText::~MCStyledText(void)
{
	while(m_paragraphs != NULL)
		delete m_paragraphs -> remove(m_paragraphs);
}

void MCStyledText::setparagraphs(MCParagraph *p_paragraphs)
{
	m_paragraphs = p_paragraphs;
}

MCParagraph *MCStyledText::getparagraphs(void)
{
	return m_paragraphs;
}

MCParagraph *MCStyledText::grabparagraphs(MCField *p_field)
{
	if (m_paragraphs == NULL)
		return NULL;

	MCParagraph *t_current;
	t_current = m_paragraphs;
	do
	{
		t_current -> setparent(p_field);
		t_current = t_current -> next();
	}
	while(t_current != m_paragraphs);

	MCParagraph *t_result;
	t_result = m_paragraphs;
	m_paragraphs = NULL;
	return t_result;
}

bool MCStyledText::visit_self(MCObjectVisitor *p_visitor)
{
	return p_visitor->OnStyledText(this);
}

bool MCStyledText::visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (t_continue && m_paragraphs != NULL)
	{
		MCParagraph *pgptr = m_paragraphs;
		MCParagraph *tpgptr = pgptr;
		do
		{
			t_continue = tpgptr -> visit(p_options, p_part, p_visitor);
			tpgptr = tpgptr->next();
		}
		while(t_continue && tpgptr != pgptr);
	}

	return t_continue;
}

// MW-2011-01-13: As styledtext is an internal (not published in anyway) format
//   we can change it to include the paragraph style for now.
IO_stat MCStyledText::save(IO_handle p_stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_STYLED_TEXT, p_stream)) != IO_NORMAL)
		return stat;

	MCParagraph *tptr = m_paragraphs;
	if (tptr != NULL)
		do
		{
			if ((stat = tptr->save(p_stream, p_part, p_version)) != IO_NORMAL)
				return stat;

			tptr = (MCParagraph *)tptr->next();
		}
		while (tptr != m_paragraphs);

	return IO_NORMAL;
}

IO_stat MCStyledText::load(IO_handle p_stream, uint32_t p_version)
{
	IO_stat stat;
	MCParagraph *paragraphs = NULL;
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, p_stream)) != IO_NORMAL)
			return checkloadstat(stat);
		switch (type)
		{
		// MW-2012-03-04: [[ StackFile5500 ]] Handle both the paragraph and extended
		//   paragraph record.
		case OT_PARAGRAPH:
		case OT_PARAGRAPH_EXT:
			{
				MCParagraph *newpar = new (nothrow) MCParagraph;
                if (parent)
                    newpar->setparent(parent.GetAs<MCField>());
				
				// MW-2012-03-04: [[ StackFile5500 ]] If the record is extended then
				//   pass in 'true' for 'is_ext'.
				if ((stat = newpar->load(p_stream, p_version, type == OT_PARAGRAPH_EXT)) != IO_NORMAL)
				{
					delete newpar;
					return checkloadstat(stat);
				}

				newpar->appendto(paragraphs);
			}
			break;
		default:
			m_paragraphs = paragraphs;
			MCS_seek_cur(p_stream, -1);
			return IO_NORMAL;
		}
	}
	return IO_NORMAL;
}
