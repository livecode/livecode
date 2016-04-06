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

#ifndef TEXT_CELL_H
#define TEXT_CELL_H

#include "prefix.h"

#include "foundation.h"
#include "dllst.h"
#include "sysdefs.h"

#include "text-api.h"


// Forward declarations
class MCTextAttributes;
struct MCGAffineTransform;


class MCTextCell :
  public MCDLlist
{
public:
    
    MCDLlistAdaptorFunctions(MCTextCell);
    
    // Returns the type of this cell.
    virtual MCTextCellType getType() const = 0;
    
    // Returns the parent cell. Will be NULL if there is no parent.
    virtual MCTextCell* getParent() const = 0;
    
    // Returns the list of child cells. Will be NULL if there are no children.
    virtual MCTextCell* getChildren() const = 0;
    
    // Performs a layout of the cell and updates the content bound measurements
    virtual void performLayout() = 0;
    
    // Clears the layout of the cell (i.e flags it for re-layout)
    virtual void clearLayout() = 0;
    
    // Draws the cell (the DC must be translated such that the origin of the
    // parent cell is at 0,0 or bad things will happen).
    virtual void draw(MCDC* dc) = 0;
    
    // Cell accessors. Non-const accessors may cause a re-layout of the cell.
    coord_t getX() const                    { return m_x; }
    coord_t getY() const                    { return m_y; }
    coord_t getWidth() const;
    coord_t getHeight() const;
    coord_t getMaxWidth() const             { return m_max_width; }
    coord_t getMaxHeight() const            { return m_max_height; }
    void setPosition(coord_t p_x, coord_t p_y);
    void setMaxSize(coord_t p_width, coord_t p_height);
    MCTextCellAlignment getHorizontalAlignment() const;
    MCTextCellAlignment getVerticalAlignment() const;
    MCTextCellLayoutDirection getLayoutDirection() const;
    MCTextDirection getTextDirection() const;
    void setAlignment(MCTextCellAlignment p_horizontal, MCTextCellAlignment p_vertical);
    void setLayoutDirection(MCTextCellLayoutDirection p_direction);
    void setTextDirection(MCTextDirection p_direction);

    // Marks the cell as requiring re-layout
    void setNeedsLayout();
    
    // Re-positions child cells on e.g. alignment change. This is for changes
    // that can be made without the cost of a full re-layout. By default, it
    // does nothing.
    virtual void repositionChildren();
    
    // Returns true if the order of lines should be reversed (from bottom to top
    // for horizontal text or from right to left for vertical text)
    bool isReversedLineOrder() const;
    
    // Returns true if the order of text runs within a line should be reversed
    // (starting from the right for horizontal text or from the bottom for
    // vertical text)
    bool isReversedTextOrder() const;
    
    // Returns true if the text should be laid out vertically rather than
    // horizontally.
    bool isVerticalLayout() const;
    
protected:
    
    // Constructor.
    MCTextCell();
    
    // Constructor. Duplicates an existing MCTextCell
    MCTextCell(const MCTextCell&);
    
    // Destructor.
    ~MCTextCell();
    
    // Turns a point in parent coordinates into the corresponding child or
    // descendant. Returns NULL if the point isn't within any.
    MCTextCell* pointToChild(coord_t p_x, coord_t p_y) const;
    MCTextCell* pointToDescendant(coord_t p_x, coord_t p_y) const;
    
    // Indicates whether this cell considers itself to own the given point
    virtual bool ownsPoint(coord_t p_x, coord_t p_y) const;
    
    // Called by subclasses when they have finished laying out
    void recordSize(coord_t p_width, coord_t p_height);
    
    // Returns true if the cell needs to be laid out
    bool needsLayout() const;
    
    // Called when a cell has finished its layout operations
    void finishLayout();
    
    // Returns the affine transformation to be used for drawing operations
    virtual MCGAffineTransform getTransform() const;
    
private:
    
    // Position of this cell relative to the parent cell
    coord_t m_x, m_y;
    
    // Measured dimensions of this cell
    coord_t m_width, m_height;
    
    // Maximum dimensions allowed for this cell
    coord_t m_max_width, m_max_height;
    
    // The attributes that have been set on this cell (un-set ones will be
    // inherited and are cached in the effective attribute structure)
    MCTextAttributes* m_local_attributes;
    MCTextAttributes* m_effective_attributes;
    
    // Horizontal and vertical alignments for the cell
    MCTextCellAlignment m_horizontal_alignment;
    MCTextCellAlignment m_vertical_alignment;
    
    // The layout direction for this cell
    MCTextCellLayoutDirection m_layout_direction;
    
    // Text directionality for this cell
    MCTextDirection m_text_direction;
    
    // Flags
    MCTextCellFlags m_flags;
};


// Sub-class for text cells that can be freely added to other text cells without
// any restrictions on the type of cell that may contain them
class MCTextCellContainable :
  public MCTextCell
{
public:
    
    // Inherited from MCTextCell
    virtual MCTextCell* getParent() const;
    
    // Sets the parent cell of this cell
    void setParent(MCTextCell* p_parent);
    
protected:
    
    // Constructor
    MCTextCellContainable(MCTextCell* p_parent = NULL);
    
private:
    
    // The parent cell
    MCTextCell* m_parent;
};


#endif // ifndef TEXT_CELL_H
