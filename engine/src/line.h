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

//
// MCLines of text within a paragraph
//
#ifndef	LINE_H
#define	LINE_H

#include "dllst.h"

class MCParagraph;
class MCBlock;

class MCLine : public MCDLlist
{
	MCParagraph *parent;
	MCBlock *firstblock;
	MCBlock *lastblock;
	coord_t width;
	uint2 ascent;
	uint2 descent;
	coord_t dirtywidth;
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
public:
	MCLine(MCParagraph *paragraph);
	~MCLine();
	void takebreaks(MCLine *lptr);
	MCBlock *fitblocks(MCBlock *p_first, MCBlock *p_sentinal, coord_t maxwidth);
	void appendall(MCBlock *bptr);
	void draw(MCDC *dc, int2 x, int2 y, uint2 si, uint2 ei, const char *tptr, uint2 pstyle);
	void setscents(MCBlock *bptr);
	uint2 getdirtywidth();
	void makedirty();
	void clean();
	void getindex(uint2 &i, uint2 &l);
	uint2 getcursorindex(coord_t x, Boolean chunk);
	coord_t getcursorx(uint2 i);
	uint2 getwidth();
	uint2 getheight();
	uint2 getascent();
	uint2 getdescent();
    coord_t GetAscent() const;
    coord_t GetDescent() const;
    coord_t GetLeading() const;
    coord_t GetHeight() const;
	void clearzeros(MCBlock*& p_list);
	// MW-2012-02-10: [[ FixedTable ]] Set the width of the line explicitly.
	void setwidth(uint2 new_width);
	
	void getblocks(MCBlock*& r_first, MCBlock*& r_last)
	{
		r_first = firstblock;
		r_last = lastblock;
	}
	
	MCLine *next()
	{
		return (MCLine *)MCDLlist::next();
	}
	MCLine *prev()
	{
		return (MCLine *)MCDLlist::prev();
	}
	void totop(MCLine *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCLine *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCLine *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCLine *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCLine *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCLine *remove(MCLine *&list)
	{
		return (MCLine *)MCDLlist::remove((MCDLlist *&)list);
	}
};

#endif
