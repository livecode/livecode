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

#include <foundation-math.h>

#include "text-pane.h"

#include "text-paragraph.h"
#include "graphics.h"
#include "stack.h"
#include "context.h"


MCTextPane::MCTextPane() :
  MCTextCell(),
  m_paragraphs(NULL),
  m_stack(NULL),
  m_text_wrap(true)
{
    // TESTING
    m_stack = MCdefaultstackptr;
}

MCTextPane::~MCTextPane()
{
    deleteContents();
}

MCTextCellType MCTextPane::getType() const
{
    return kMCTextCellTypePane;
}

MCTextCell* MCTextPane::getParent() const
{
    return NULL;
}

MCTextCell* MCTextPane::getChildren() const
{
    return m_paragraphs;
}

void MCTextPane::performLayout()
{
    if (!needsLayout())
        return;
    
    // Tell each of the paragraphs to do their layout. This is necessary so that
    // the total size of the paragraphs can be calculated.
    MCTextParagraph* t_paragraph;
    coord_t t_total_height;
    coord_t t_max_width;
    t_paragraph = m_paragraphs;
    t_total_height = t_max_width = 0.0f;
    do
    {
        if (getTextWrap())
            t_paragraph->setMaxSize(getMaxWidth(), INFINITY);
        else
            t_paragraph->setMaxSize(INFINITY, INFINITY);
        
        t_paragraph->performLayout();
        t_total_height += t_paragraph->getHeight();
        t_max_width = MCMax(t_max_width, t_paragraph->getWidth());
        t_paragraph = t_paragraph->next();
    }
    while (t_paragraph != m_paragraphs);
    
    if (getTextWrap())
        recordSize(getMaxWidth(), t_total_height);
    else
        recordSize(t_max_width, t_total_height);
    
    // Position the paragraphs correctly
    finishLayout();
    repositionChildren();
}

void MCTextPane::clearLayout()
{
    // Clear the layout of every paragraph
    MCTextParagraph* t_paragraph;
    t_paragraph = m_paragraphs;
    do
    {
        t_paragraph->clearLayout();
        t_paragraph = t_paragraph->next();
    }
    while (t_paragraph != m_paragraphs);
}

void MCTextPane::draw(MCDC* dc)
{
    // Ensure that layout has been completed
    performLayout();
    
    // Draw each of the paragraphs in turn
    MCTextParagraph* t_paragraph;
    t_paragraph = m_paragraphs;
    do
    {
        dc->save();
        dc->setorigin(t_paragraph->getX(), t_paragraph->getY());
        t_paragraph->draw(dc);
        dc->restore();
        
        t_paragraph = t_paragraph->next();
    }
    while (t_paragraph != m_paragraphs);
}

void MCTextPane::repositionChildren()
{
    // Based on vertical alignment, calculate the offset for positioning
    coord_t t_offset;
    t_offset = 0.0f;
    if (getHeight() < getMaxHeight())
    {
        switch (getVerticalAlignment())
        {
            case kMCTextCellAlignCenter:
                t_offset = (getMaxHeight() - getHeight())/2;
                break;
                
            case kMCTextCellAlignStart:
            case kMCTextCellAlignJustify:
                break;
                
            case kMCTextCellAlignEnd:
                t_offset = getMaxHeight() - getHeight();
                break;
                
            default:
                MCUnreachable();
        }
    }
    
    // Position each of the paragraphs in turn
    coord_t t_y = 0;
    MCTextParagraph* t_paragraph;
    t_paragraph = m_paragraphs;
    do
    {
        // Position the paragraph at the top or bottom of the pane, as required
        if (isReversedLineOrder())
        {
            // Position from the bottom, remembering to account for the height
            // of this paragraph as well
            t_y += t_paragraph->getHeight();
            t_paragraph->setPosition(0, getHeight() - t_y + t_offset);
        }
        else
        {
            // Position from the top of the pane
            t_paragraph->setPosition(0, t_y + t_offset);
            t_y += t_paragraph->getHeight();
        }
        
        t_paragraph->repositionChildren();
        t_paragraph = t_paragraph->next();
    }
    while (t_paragraph != m_paragraphs);
}

void MCTextPane::setContentsPlain(MCStringRef p_string)
{
    // Clear the existing contents
    deleteContents();
    
    // Create a new paragraph to hold the contents of the pane
    m_paragraphs = new MCTextParagraph(this);
    m_paragraphs->setContentsPlain(p_string);
    
    // TODO: examine the list of blocks in the paragraph for paragraph breaks
    
    // Pane needs to be laid-out
    setNeedsLayout();
}

MCGAffineTransform MCTextPane::getTransform() const
{
    return m_stack->getdevicetransform();
}

void MCTextPane::deleteContents()
{
    if (m_paragraphs == NULL)
        return;
    
    // The layout needs to be reset
    clearLayout();
    
    // Delete each of the paragraphs
    while (m_paragraphs != NULL)
        delete m_paragraphs->remove(m_paragraphs);
}
