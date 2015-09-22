/* Copyright (C) 2015 LiveCode Ltd.
 
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

#ifndef TEXT_SEGMENT_H
#define TEXT_SEGMENT_H


#include "text-cell.h"


// Forward declarations
class MCTextParagraph;
class MCTextLine;
class MCTextBlock;

class MCTextSegment :
  public MCTextCell
{
public:
    
    MCDLlistAdaptorFunctions(MCTextSegment)
    
    // Constructor
    MCTextSegment(MCTextParagraph* p_paragraph);
    ~MCTextSegment();
    
    // Inherited from MCTextCell
    virtual MCTextCellType getType() const;
    virtual MCTextCell* getParent() const;
    virtual MCTextCell* getChildren() const;
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    virtual void repositionChildren();
    
    // Segment accessors
    MCTextParagraph* getParagraph() const       { return m_paragraph; }
    MCTextLine* getLine() const                 { return m_line; }
    MCTextBlock* getBlocks() const              { return m_blocks; }
    coord_t getTextAscent() const               { return m_ascent; }
    coord_t getTextDescent() const              { return m_descent; }
    coord_t getTextLeading() const              { return m_leading; }
    void setLine(MCTextLine* p_line)            { m_line = p_line; }
    
    // Adds the given list of blocks to the segment, up to the next break block
    // that performs a segment break. A pointer to the remaining blocks is
    // returned (this will be NULL if all blocks were added).
    MCTextBlock* addBlocks(MCTextBlock* p_blocks);
    
    // Removes all blocks from the segment and returns them as a list
    MCTextBlock* clearAllBlocks();

private:
    
    // The line and paragraph to which this segment belongs
    MCTextLine* m_line;
    MCTextParagraph* m_paragraph;
    
    // The blocks within this segment
    MCTextBlock* m_blocks;
    
    // Array mapping from visual ordering indices to blocks
    MCAutoArray<MCTextBlock*> m_visual_order;
    
    // Font metrics for the largest font within this segment
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    
    
    // Calculates the visual ordering for blocks within this segment
    void calculateVisualOrder();
};


#endif // ifndef TEXT_SEGMENT_H
