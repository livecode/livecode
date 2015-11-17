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

#include "prefix.h"

#include "text-cell.h"

#include <foundation-math.h>

#include <float.h>
#include "graphics.h"


coord_t MCTextCell::getWidth() const
{
    return m_width;
}

coord_t MCTextCell::getHeight() const
{
    return m_height;
}

void MCTextCell::setPosition(coord_t p_x, coord_t p_y)
{
    m_x = p_x;
    m_y = p_y;
}

void MCTextCell::setMaxSize(coord_t p_width, coord_t p_height)
{
    // Do nothing if the size hasn't changed
    if (m_max_width == p_width && m_max_height == p_height)
        return;
    
    m_max_width = p_width;
    m_max_height = p_height;
    
    setNeedsLayout();
}

MCTextCellAlignment MCTextCell::getHorizontalAlignment() const
{
    return m_horizontal_alignment;
}

MCTextCellAlignment MCTextCell::getVerticalAlignment() const
{
    return m_vertical_alignment;
}

MCTextCellLayoutDirection MCTextCell::getLayoutDirection() const
{
    return m_layout_direction;
}

MCTextDirection MCTextCell::getTextDirection() const
{
    return m_text_direction;
}

void MCTextCell::setAlignment(MCTextCellAlignment p_horizontal, MCTextCellAlignment p_vertical)
{
    if (p_horizontal == m_horizontal_alignment && p_vertical == m_vertical_alignment)
        return;
    
    m_horizontal_alignment = p_horizontal;
    m_vertical_alignment = p_vertical;
    repositionChildren();
}

void MCTextCell::setLayoutDirection(MCTextCellLayoutDirection p_direction)
{
    if (p_direction == m_layout_direction)
        return;
    
    m_layout_direction = p_direction;
    repositionChildren();
}

void MCTextCell::setTextDirection(MCTextDirection p_direction)
{
    if (p_direction == m_text_direction)
        return;
    
    m_text_direction = p_direction;
    setNeedsLayout();
}

void MCTextCell::setNeedsLayout()
{
    m_flags |= kMCTextCellNeedsLayout;
    
    if (getParent() != NULL)
        getParent()->setNeedsLayout();
}


bool MCTextCell::isReversedLineOrder() const
{
    switch (getLayoutDirection())
    {
        case kMCTextCellLayoutLeftThenDown:
        case kMCTextCellLayoutRightThenDown:
        case kMCTextCellLayoutDownThenLeft:
        case kMCTextCellLayoutUpThenLeft:
            return false;
            
        case kMCTextCellLayoutLeftThenUp:
        case kMCTextCellLayoutRightThenUp:
        case kMCTextCellLayoutDownThenRight:
        case kMCTextCellLayoutUpThenRight:
            return true;
            
        default:
            MCUnreachable();
    }
}

bool MCTextCell::isReversedTextOrder() const
{
    switch (getLayoutDirection())
    {
        case kMCTextCellLayoutLeftThenDown:
        case kMCTextCellLayoutLeftThenUp:
        case kMCTextCellLayoutDownThenLeft:
        case kMCTextCellLayoutDownThenRight:
            return false;
            
        case kMCTextCellLayoutRightThenDown:
        case kMCTextCellLayoutRightThenUp:
        case kMCTextCellLayoutUpThenRight:
        case kMCTextCellLayoutUpThenLeft:
            return true;
            
        default:
            MCUnreachable();
    }
}

bool MCTextCell::isVerticalLayout() const
{
    switch (getLayoutDirection())
    {
        case kMCTextCellLayoutLeftThenDown:
        case kMCTextCellLayoutLeftThenUp:
        case kMCTextCellLayoutRightThenDown:
        case kMCTextCellLayoutRightThenUp:
            return false;
            
        case kMCTextCellLayoutDownThenLeft:
        case kMCTextCellLayoutDownThenRight:
        case kMCTextCellLayoutUpThenLeft:
        case kMCTextCellLayoutUpThenRight:
            return true;
            
        default:
            MCUnreachable();
    }
}

MCTextCell::MCTextCell() :
  m_x(0.0),
  m_y(0.0),
  m_width(0.0),
  m_height(0.0),
  m_max_width(INFINITY),
  m_max_height(INFINITY),
  m_local_attributes(NULL),
  m_effective_attributes(NULL),
  m_horizontal_alignment(kMCTextCellAlignCenter),
  m_vertical_alignment(kMCTextCellAlignCenter),
  m_layout_direction(kMCTextCellLayoutLeftThenDown),
  m_text_direction(kMCTextDirectionAuto),
  m_flags(kMCTextCellNeedsLayout)
{
    ;
}

MCTextCell::MCTextCell(const MCTextCell& p_copy) :
  m_x(0.0),
  m_y(0.0),
  m_width(0.0),
  m_height(0.0),
  m_max_width(p_copy.m_max_width),
  m_max_height(p_copy.m_max_height),
  m_local_attributes(NULL),
  m_effective_attributes(NULL),
  m_horizontal_alignment(p_copy.m_horizontal_alignment),
  m_vertical_alignment(p_copy.m_vertical_alignment),
  m_layout_direction(p_copy.m_layout_direction),
  m_text_direction(p_copy.m_text_direction),
  m_flags(p_copy.m_flags | kMCTextCellNeedsLayout)
{
    ;
}

MCTextCell::~MCTextCell()
{
    delete m_local_attributes;
    delete m_effective_attributes;
}

MCTextCell* MCTextCell::pointToChild(coord_t p_x, coord_t p_y) const
{
    // Scan the list of children to see if the point is within any of them
    MCTextCell* t_cell;
    t_cell = getChildren();
    if (t_cell == NULL)
        return NULL;
    do
    {
        if (t_cell->ownsPoint(p_x, p_y))
            return t_cell;
        t_cell = t_cell->next();
    }
    while (t_cell != getChildren());
    
    // No child claims the point
    return NULL;
}

bool MCTextCell::ownsPoint(coord_t p_x, coord_t p_y) const
{
    // Answer yes if the point is within the bounds of the cell
    return getX() <= p_x && p_x < getX()+getWidth()
        && getY() <= p_y && p_y < getY()+getHeight();
}

void MCTextCell::recordSize(coord_t p_width, coord_t p_height)
{
    m_width = p_width;
    m_height = p_height;
}

bool MCTextCell::needsLayout() const
{
    return m_flags & kMCTextCellNeedsLayout;
}

void MCTextCell::finishLayout()
{
    m_flags &= ~kMCTextCellNeedsLayout;
}

MCGAffineTransform MCTextCell::getTransform() const
{
    return getParent()->getTransform();
}

void MCTextCell::repositionChildren()
{
    // Do nothing
}

////////////////////////////////////////////////////////////////////////////////

MCTextCellContainable::MCTextCellContainable(MCTextCell* p_parent) :
  MCTextCell(),
  m_parent(p_parent)
{
    ;
}

MCTextCell* MCTextCellContainable::getParent() const
{
    return m_parent;
}
