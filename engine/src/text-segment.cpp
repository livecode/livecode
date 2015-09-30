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

#include "text-segment.h"

#include "text-block.h"
#include "text-breakblock.h"
#include "text-line.h"
#include "text-paragraph.h"
#include "context.h"


MCTextSegment::MCTextSegment(MCTextParagraph* p_paragraph) :
  MCTextCell(*p_paragraph),
  m_line(NULL),
  m_paragraph(p_paragraph),
  m_blocks(NULL),
  m_visual_order(),
  m_ascent(0.0f),
  m_descent(0.0f),
  m_leading(0.0f)
{
    ;
}

MCTextSegment::~MCTextSegment()
{
    // The segment must be empty when it is destroyed - if not, something has
    // gone badly wrong with the paragraph code and blocks were lost...
    MCAssert(m_blocks == NULL);
}

MCTextCellType MCTextSegment::getType() const
{
    return kMCTextCellTypeSegment;
}

MCTextCell* MCTextSegment::getParent() const
{
    if (m_line != NULL)
        return m_line;
    else
        return m_paragraph;
}

MCTextCell* MCTextSegment::getChildren() const
{
    return m_blocks;
}

void MCTextSegment::performLayout()
{
    // Reset the font metric information
    m_ascent = m_descent = m_leading = 0.0f;
    
    // Calculate the dimensions of the segment from the dimensions of the blocks
    MCTextBlock* t_block;
    coord_t t_width;
    t_width = 0;
    t_block = m_blocks;
    do
    {
        // Tell the block to finish its own layout
        t_block->performLayout();
        
        // The width of the segment is sum of the width of the blocks
        t_width += t_block->getWidth();
        
        // The font metrics are the greatest values found in any block
        m_ascent = MCMax(m_ascent, t_block->getTextAscent());
        m_descent = MCMax(m_descent, t_block->getTextDescent());
        m_leading = MCMax(m_leading, t_block->getTextLeading());
        
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
    
    recordSize(t_width, m_ascent + m_descent + m_leading);
    finishLayout();
}

void MCTextSegment::clearLayout()
{
    // Do nothing - the parent line will clear us via clearAllBlocks
}

void MCTextSegment::draw(MCDC* dc)
{
    // Draw each of the blocks in turn
    MCTextBlock* t_block;
    t_block = m_blocks;
    do
    {
        dc->save();
        dc->setorigin(t_block->getX(), t_block->getY());
        t_block->draw(dc);
        dc->restore();
        
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
}

void MCTextSegment::repositionChildren()
{
    // The visual ordering of the blocks needs to be calculated before their
    // positions can be calculated
    calculateVisualOrder();
    
    // Calculate the offset that we are going to use for the position, based on
    // the alignment that the contents of the segment should take.
    coord_t t_x = 0.0f;
    /*switch (getHorizontalAlignment())
    {
        case kMCTextCellAlignCenter:
            t_x += (getMaxWidth() - getWidth()) / 2;
            break;
            
        case kMCTextCellAlignStart:
        case kMCTextCellAlignJustify:
            break;
            
        case kMCTextCellAlignEnd:
            t_x += getMaxWidth() - getWidth();
            break;
            
        default:
            MCUnreachable();
    }*/
    
    // Position the blocks in visual order
    for (uindex_t i = 0; i < m_visual_order.Size(); i++)
    {
        if (getParagraph()->isReversedTextOrder())
        {
            t_x += m_visual_order[i]->getWidth();
            m_visual_order[i]->setPosition(getMaxWidth() - t_x, 0.0f);
        }
        else
        {
            m_visual_order[i]->setPosition(t_x, 0.0f);
            t_x += m_visual_order[i]->getWidth();
        }
    }
}

MCTextBlock* MCTextSegment::addBlocks(MCTextBlock* p_blocks)
{
    // Scan through the list of blocks until the end or a segment-breaking (or
    // stronger) block is encountered.
    MCTextBlock* t_block;
    bool t_broken;
    t_block = p_blocks;
    t_broken = false;
    do
    {
        // Is this a segment-breaking block? If so, terminate the segment after
        // appending it (segment breaks add post-padding so come at the end of
        // segments rather than the beginning).
        if (t_block != p_blocks && t_block->getBlockType() == kMCTextBlockTypeBreak)
        {
            MCTextBreakBlock* t_break_block;
            t_break_block = static_cast<MCTextBreakBlock*>(t_block);
            if (t_break_block->getBreakClass() >= kMCTextBlockBreakClassSegment)
                t_broken = true;
        }
        
        // Append this block to the block list
        // The remove operation advances the t_block pointer implicitly
        MCTextBlock* t_segment_block;
        t_segment_block = t_block->remove(t_block);
        t_segment_block->appendto(m_blocks);
        t_segment_block->setSegment(this);
    }
    while (t_block != NULL && !t_broken);
    
    // This will be NULL if all of the blocks were consumed
    return t_block;
}

MCTextBlock* MCTextSegment::clearAllBlocks()
{
    // The blocks no longer belong to this segment
    MCTextBlock* t_block;
    t_block = m_blocks;
    do
    {
        t_block->setSegment(NULL);
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
    
    // Just return the list of blocks directly
    MCTextBlock* t_blocks = m_blocks;
    m_blocks = NULL;
    return t_blocks;
}

void MCTextSegment::calculateVisualOrder()
{
    // Compute the number of blocks within this segment and find the minimum and
    // maximum block direction levels.
    MCTextBlock* t_block;
    uindex_t t_block_count;
    uint8_t t_min_level, t_max_level;
    t_block = m_blocks;
    t_block_count = 0;
    t_min_level = 255;
    t_max_level = 0;
    do
    {
        t_block_count++;
        
        t_min_level = MCMin(t_min_level, t_block->getBidiLevel());
        t_max_level = MCMax(t_max_level, t_block->getBidiLevel());
        
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
    
    // Allocate the visual ordering array
    m_visual_order.Delete();
    m_visual_order.New(t_block_count);
    
    // Start with the visual order being identical to the logical order
    t_block = m_blocks;
    for (uindex_t i = 0; i < t_block_count; i++, t_block = t_block->next())
        m_visual_order[i] = t_block;
    
    // Start the scan on an odd level (we don't need to reverse the first level
    // if it is even because it is already in the correct order)
    t_min_level += (t_min_level & 1) ? 0 : 1;
    
    // Scan in reverse level order, reversing runs of higher-ordered levels
    for (uindex_t i = t_max_level; i >= t_min_level; i--)
    {
        // Scan through the list of blocks for levels > this level
        uindex_t j = 0;
        while (j < t_block_count)
        {
            // Level is lower than the current level. Next block.
            if (m_visual_order[j]->getBidiLevel() < i)
            {
                j++;
                continue;
            }
            
            // Find the end of this run of blocks
            uindex_t t_run_length;
            t_run_length = 0;
            while (j + t_run_length < t_block_count)
            {
                if (m_visual_order[j + t_run_length]->getBidiLevel() < i)
                    break;
                t_run_length++;
            }
            
            // Reverse the visual order of this array section
            uindex_t t_start, t_end;
            t_start = j;
            t_end = t_start + t_run_length - 1;
            while (t_end > t_start)
            {
                MCTextBlock* t_temporary = m_visual_order[t_start];
                m_visual_order[t_start] = m_visual_order[t_end];
                m_visual_order[t_end ] = t_temporary;
                
                t_start++;
                t_end--;
            }
            
            // This run has been reversed and we can skip the remainder of it
            j += t_run_length;
        }
    }
}
