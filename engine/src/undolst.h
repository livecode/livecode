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

//
// List and stack of undos
//
#ifndef	UNDOLIST_H
#define	UNDOLIST_H

#include "dllst.h"

enum Undo_type {
    UT_MOVE,
    UT_SIZE,
    UT_DELETE,
    UT_REPLACE,
    UT_DELETE_TEXT,
    UT_REPLACE_TEXT,
    UT_TYPE_TEXT,
	UT_MOVE_TEXT,
    UT_PAINT
};

struct DTstruct
{
	uint4 index;
	uint4 old_index;
	Boolean newline;
	uint4 newchars;
	MCParagraph *data;
};

struct Ustruct
{
	Undo_type type;
	union {
		MCRectangle rect;
		MCPoint deltas;
		uint2 layer;
		DTstruct text;
	} ud;
};

class MCUndonode : public MCDLlist
{
	MCObject *object;
	Ustruct *savedata;
public:
	MCUndonode(MCObject *objptr, Ustruct *data);
	~MCUndonode();
	void undo();
	MCObject *getobject()
	{
		return object;
	}
	Ustruct *getdata()
	{
		return savedata;
	}
	MCUndonode *next()
	{
		return (MCUndonode *)MCDLlist::next();
	}
	MCUndonode *prev()
	{
		return (MCUndonode *)MCDLlist::prev();
	}
	void totop(MCUndonode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCUndonode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCUndonode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCUndonode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCUndonode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCUndonode *remove
	(MCUndonode *&list)
	{
		return (MCUndonode *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCUndolist
{
	MCUndonode *nodes;
public:
	MCUndolist();
	~MCUndolist();
	void savestate(MCObject *objptr, Ustruct *data);
	void freestate();
	void freeobject(MCObject *objptr);
	MCObject *getobject();
	Ustruct *getstate();
	Boolean undo();
};
#endif
