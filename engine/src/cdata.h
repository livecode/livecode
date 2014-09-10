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

#ifndef	FDATA_H
#define	FDATA_H

#include "dllst.h"

class MCParagraph;
class MCField;

#define COMPACT_PARAGRAPHS 0x80000000

class MCCdata : public MCDLlist
{
	uint4 id;
	void *data;
public:
	MCCdata();
	MCCdata(uint4 newid);
	MCCdata(const MCCdata &fref);
	~MCCdata();
	IO_stat load(IO_handle stream, MCObject *parent, uint32_t version);
	IO_stat save(IO_handle stream, Object_type type, uint4 p_part);
	uint4 getid();
	void setid(uint4 newid);
	MCParagraph *getparagraphs();
	void setparagraphs(MCParagraph *&newpar);
	Boolean getset();
	void setset(Boolean newdata);
	void setdata(int4 newdata)
	{
		data = (void *)newdata;
	}
	int4 getdata()
	{
		return (int4)(intptr_t)data;
	}
	MCCdata *next()
	{
		return (MCCdata *)MCDLlist::next();
	}
	MCCdata *prev()
	{
		return (MCCdata *)MCDLlist::prev();
	}
	void totop(MCCdata *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCCdata *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCCdata *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCCdata *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCCdata *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}
	MCCdata *remove(MCCdata *&list)
	{
		return (MCCdata *)MCDLlist::remove((MCDLlist *&)list);
	}
};

#endif
