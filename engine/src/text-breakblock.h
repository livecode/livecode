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

#ifndef TEXT_BREAKBLOCK_H
#define TEXT_BREAKBLOCK_H


#include "text-block.h"


// Forward declarations
class MCTextParagraph;



// Breaking classes
enum MCTextBlockBreakClass
{
    kMCTextBlockBreakClassSegment,      // Ends a segment
    kMCTextBlockBreakClassLine,         // Ends a line
    kMCTextBlockBreakClassParagraph     // Ends a paragraph
};



class MCTextBreakBlock :
  public MCTextBlock
{
public:
    
    // Constructor
    MCTextBreakBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range, MCTextBlockBreakClass p_class);
    
    // Inherited from MCTextCell
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    
    // Inherited from MCTextBlock
    virtual MCTextBlockType getBlockType() const;
    virtual coord_t getCursorPositionBefore(uindex_t p_grapheme_offset);
    virtual coord_t getCursorPositionAfter(uindex_t p_grapheme_offset);
    
    // Break block accessors
    MCTextBlockBreakClass getBreakClass() const     { return m_break_class; }
    
private:
    
    MCTextBlockBreakClass m_break_class;
};


#endif  // ifndef TEXT_BREAKBLOCK_H
