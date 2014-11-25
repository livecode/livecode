/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#ifndef TEXT_PANE_H
#define TEXT_PANE_H


#include "text-cell.h"


// Forward declarations
class MCTextParagraph;


class MCTextPane :
  public MCTextCell
{
public:
    
    // Constructor and destructor.
    MCTextPane();
    ~MCTextPane();
    
    // Inherited from MCTextCell
    virtual MCTextCellType getType() const;
    virtual MCTextCell* getParent() const;
    virtual MCTextCell* getChildren() const;
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    
    // Sets the contents of this pane to the plain-text string given
    void setContentsPlain(MCStringRef p_string);
    
protected:
    
    // Inherited from MCTextCell
    virtual MCGAffineTransform getTransform() const;
    
private:
    
    // The list of paragraphs in this pane
    MCTextParagraph* m_paragraphs;
    
    // Offsets within the content area to draw the paragraphs. These are
    // calculated based on the size of the contained paragraphs and what
    // alignment has been requested.
    coord_t m_aligned_origin_x;
    coord_t m_aligned_offset_y;
    
    // The stack to which this pane belongs (needed for stack scaling)
    MCStack* m_stack;
    
    
    // Deletes all of the paragraphs (and therefore contents) of the pane
    void deleteContents();
};


#endif  // ifndef TEXT_PANE_H
