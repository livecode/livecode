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
// MCLines of text within a paragraph
//
#ifndef	LINE_H
#define	LINE_H

#include "dllst.h"
#include "field.h"

class MCParagraph;
class MCBlock;
class MCSegment;

class MCLine : public MCDLlist
{
	MCParagraph *parent;
	MCBlock *firstblock;
	MCBlock *lastblock;
    MCSegment *firstsegment;
    MCSegment *lastsegment;
	coord_t width;
	coord_t dirtywidth;
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;

    // Offset at which to start drawing segments (for whole-line alignment)
    int16_t m_offset;
    
    // Dirty hack
    friend class MCSegment;

public:
	MCLine(MCParagraph *paragraph);
	~MCLine();
    // SN-2015-01-21: [[ Bug 14229 ]] Update the dirtywidth with the width of lptr.
    void takewidth(MCLine *lptr);
	void takebreaks(MCLine *lptr);
	MCBlock *fitblocks(MCBlock *p_first, MCBlock *p_sentinal, uint2 maxwidth);
	void appendall(MCBlock *bptr, bool p_flow);
    void appendsegments(MCSegment *first, MCSegment *last);
	void draw(MCDC *dc, int2 x, int2 y, findex_t si, findex_t ei, MCStringRef p_text, uint2 pstyle);
	void setscents(MCBlock *bptr);
	uint2 getdirtywidth();
	void makedirty();
	void clean();
	void getindex(uint2 &i, uint2 &l);
	uint2 getwidth();
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
	
    MCParagraph *getparent() const
    {
        return parent;
    }
    
    //////////
	
	void GetRange(findex_t &r_index, findex_t &r_length);
	findex_t GetOffset();
	findex_t GetLength();
	findex_t GetCursorIndex(coord_t x, Boolean chunk, bool forward);
	coord_t GetCursorXPrimary(findex_t i, bool forward);
    coord_t GetCursorXSecondary(findex_t i, bool forward);
    coord_t GetCursorXHelper(findex_t i, bool moving_forward);
	
    ////////// Layout
    
    // Fits as many blocks/segments into this line as possible using the given
    // width and returns a line containing the parts that would not fit (or NULL
    // if everything could be added).
    MCLine *Fit(int16_t linewidth);
    
    // Sets the correct drawing properties, etc for non-flowed lines
    void NoFlowLayout();
    
    // Utility methods for calculating tab positions and alignments
    int16_t CalculateTabPosition(uindex_t p_which_tab, int16_t p_from_position);
    intenum_t CalculateTabAlignment(uindex_t p_which_tab);
    
    int16_t GetLineOffset() const
    {
        return m_offset;
    }
    
    MCSegment *GetFirstSegment() const
    {
        return firstsegment;
    }
    
    MCSegment *GetLastSegment() const
    {
        return lastsegment;
    }
    
	//////////
	
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
    
private:
    
    ////////// BIDIRECTIONAL SUPPORT
    
    void ResolveDisplayOrder();
    
    ////////// TAB ALIGNMENT SUPPORT
    
    void SegmentLine();
    MCLine *DoLayout(bool p_flow, int16_t p_line_width);
};

#endif
