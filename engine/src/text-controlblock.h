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

#ifndef TEXT_CONTROLBLOCK_H
#define TEXT_CONTROLBLOCK_H


#include "text-block.h"


// Forward declarations
class MCTextParagraph;


class MCTextControlBlock :
public MCTextBlock
{
public:
    
    // Constructor
    MCTextControlBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range);
    
    // Inherited from MCTextCell
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    
    // Inherited from MCTextBlock
    virtual MCTextBlockType getBlockType() const;
    virtual coord_t getCursorPositionBefore(uindex_t p_grapheme_offset);
    virtual coord_t getCursorPositionAfter(uindex_t p_grapheme_offset);
    
    
};


#endif  // ifndef TEXT_CONTROLBLOCK_H
