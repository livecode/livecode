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

#ifndef __MC_STYLED_TEXT__
#define __MC_STYLED_TEXT__

#ifndef __MC_OBJECT__
#include "object.h"
#endif

class MCStyledText: public MCObject
{
public:
	MCStyledText(void);
	~MCStyledText(void);

	void setparagraphs(MCParagraph *p_paragraphs);
	MCParagraph *getparagraphs(void);

	MCParagraph *grabparagraphs(MCField *p_field);

	bool visit_self(MCObjectVisitor *p_visitor);
	bool visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	IO_stat save(IO_handle p_stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat load(IO_handle p_stream, uint32_t version);

private:
	MCParagraph *m_paragraphs;
};

#endif
