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

#ifndef TEXT_RUN_H
#define TEXT_RUN_H


#include "text-block.h"


// Forward declarations
class MCTextParagraph;


class MCTextRun :
  public MCTextBlock
{
public:
    
    // Constructor. Creates a new run covering the given portion of a paragraph.
    MCTextRun(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range);
    
    // Constructor. Creates a new block that has the same attributes as the
    // supplied block and covers the supplied range.
    MCTextRun(const MCTextRun& p_copy, const MCRange& p_codeunit_range);
    
    // Destructor.
    ~MCTextRun();
    
    // Inherited from MCTextCell
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    
    // Interited from MCTextBlock
    virtual MCTextBlockType getBlockType() const;
    virtual coord_t getCursorPositionBefore(uindex_t p_grapheme_offset);
    virtual coord_t getCursorPositionAfter(uindex_t p_grapheme_offset);
    virtual bool mergeWithNext();
    virtual MCTextRun* splitAfter(uindex_t p_codeunit_offset);
    virtual coord_t getTextAscent() const;
    virtual coord_t getTextDescent() const;
    virtual coord_t getTextLeading() const;
    
private:
    
    // Cache of the font to use when rendering this run
    MCFontRef m_font;
    
    
    // Ensures that a font has been allocated for this run
    void ensureFont();
};


#endif  // ifndef TEXT_RUN_H
