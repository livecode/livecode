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

#ifndef SEGMENT_H
#define SEGMENT_H


#include "dllst.h"
#include "field.h"


// Forward declarations
class MCLine;
class MCBlock;


enum
{
    kMCSegmentTextHAlignLeft,
    kMCSegmentTextHAlignCenter,
    kMCSegmentTextHAlignRight,
    kMCSegmentTextHAlignJustify
};

enum
{
    kMCSegmentTextVAlignTop,
    kMCSegmentTextVAlignCenter,
    kMCSegmentTextVAlignRight,
};


class MCSegment : public MCDLlist
{
private:
    
    // Line to which this segment belongs
    MCLine *m_Parent;
    
    // Blocks belonging to this segment
    MCBlock *m_FirstBlock;
    MCBlock *m_LastBlock;
    
    // First and last blocks in the visual order
    MCBlock *m_FirstVisualBlock;
    MCBlock *m_LastVisualBlock;
    
    // Boundaries within the line of this segment
    int16_t m_LeftEdge;     // Also serves as x coordinate
    int16_t m_RightEdge;
    int16_t m_TopEdge;      // Also serves as y coordinate
    int16_t m_BottomEdge;
    
    // Width of the contents of this segment
    coord_t m_ContentWidth;
    
    // SN-2014-08-14: [[ Bug 13106 ]] m_Padding member added
    uint16_t m_Padding;
    
    // Horizontal and vertical alignment of the text within this segment
    uint8_t m_HAlign;
    uint8_t m_VAlign;
    
public:
    
    MCSegment(MCLine *p_parent);
    MCSegment(const MCSegment*);
    ~MCSegment();
    
    MCLine* GetParent() const
    {
        return m_Parent;
    }
    MCBlock* GetFirstBlock() const
    {
        return m_FirstBlock;
    }
    MCBlock* GetLastBlock() const
    {
        return m_LastBlock;
    }
    
    MCBlock* GetFirstVisualBlock() const
    {
        return m_FirstVisualBlock;
    }
    MCBlock* GetLastVisualBlock() const
    {
        return m_LastVisualBlock;
    }
    
    void SetParent(MCLine *parent)
    {
        m_Parent = parent;
    }
    
    // Adds a range of blocks to the segment
    void AddBlockRange(MCBlock *first, MCBlock *last);
    
    // Segment boundaries
    void SetBoundaries(int16_t p_left, int16_t p_right, int16_t p_top, int16_t p_bottom)
    {
        m_LeftEdge = p_left;
        m_RightEdge = p_right;
        m_TopEdge = p_top;
        m_BottomEdge = p_bottom;
    }
    
    // SN-2014-08-14: [[ Bug 13106 ]] m_Padding member added to the Segment
    coord_t GetPadding() const
    {
        return m_Padding;
    }
    
    int16_t GetLeft() const
    {
        return m_LeftEdge;
    }
    int16_t GetRight() const
    {
        return m_RightEdge;
    }
    int16_t GetTop() const
    {
        return m_TopEdge;
    }
    int16_t GetBottom() const
    {
        return m_BottomEdge;
    }
    int16_t GetWidth() const
    {
        return m_RightEdge - m_LeftEdge;
    }
    // SN-2014-08-14: [[ Bug 13106 ]] Returns the width inside the cell, padding included
    int16_t GetInnerWidth() const
    {
        return m_RightEdge - m_LeftEdge - 2 * m_Padding;
    }
    int16_t GetHeight() const
    {
        return m_BottomEdge - m_TopEdge;
    }

    // Returns the length of the contents (if rendered as a single line)
    coord_t GetContentLength();
    
    // Returns the height of the contents (if rendered as a single line)
    int16_t GetContentHeight() const;
    
    // Does block fitting within the segment; using the given width, as many
    // blocks as possible will be retained by the segment and those that do not
    // fit will be returned in a new line. If all fit, NULL is returned.
    MCLine *Fit(coord_t p_available_width);
    
    // Horizontal and vertical alignment
    void SetHorizontalAlignment(uint8_t p_halign)
    {
        m_HAlign = p_halign;
    }
    void SetVerticalAlignment(uint8_t p_valign)
    {
        m_VAlign = p_valign;
    }
    uint8_t GetHorizontalAlignment() const
    {
        return m_HAlign;
    }
    uint8_t GetVerticalAlignment() const
    {
        return m_VAlign;
    }
    
    // Draws the contents of the segment
    void Draw(MCDC *dc, coord_t line_origin_x, int16_t line_origin_y, findex_t si, findex_t ei, MCStringRef p_text, uint16_t p_style);
    
    // Arranges the blocks of this segment into visual order for display
    void ResolveDisplayOrder();
    
    // Returns the offset used for cursor positioning within the segment
    coord_t GetLeftEdge();
    // SN-2014-08-11: [[ Bug 13124 ]] Returns the most on the right position within the segment
    coord_t GetRightEdge();
    
    // Linked list management
    MCSegment *next()
    {
        return (MCSegment *)MCDLlist::next();
    }
    MCSegment *prev()
    {
        return (MCSegment *)MCDLlist::prev();
    }
    void totop(MCSegment *&list)
    {
        MCDLlist::totop((MCDLlist *&)list);
    }
    void insertto(MCSegment *&list)
    {
        MCDLlist::insertto((MCDLlist *&)list);
    }
    void appendto(MCSegment *&list)
    {
        MCDLlist::appendto((MCDLlist *&)list);
    }
    void append(MCSegment *node)
    {
        MCDLlist::append((MCDLlist *)node);
    }
    void splitat(MCSegment *node)
    {
        MCDLlist::splitat((MCDLlist*)node);
    }
    MCSegment *remove(MCSegment *&list)
    {
        return (MCSegment *)MCDLlist::remove((MCDLlist *&)list);
    }
};


#endif // ifndef SEGMENT_H

