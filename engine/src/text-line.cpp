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

#include "text-line.h"

#include <foundation-math.h>

#include "text-block.h"
#include "text-breakblock.h"
#include "text-breakingengine.h"
#include "text-paragraph.h"
#include "text-segment.h"
#include "context.h"


MCTextLine::MCTextLine(MCTextParagraph* p_paragraph) :
  MCTextCell(*p_paragraph),
  m_parent(p_paragraph),
  m_segments(NULL),
  m_ascent(0.0f),
  m_descent(0.0f),
  m_leading(0.0f)
{
    ;
}

MCTextCellType MCTextLine::getType() const
{
    return kMCTextCellTypeLine;
}

MCTextCell* MCTextLine::getParent() const
{
    return m_parent;
}

MCTextCell* MCTextLine::getChildren() const
{
    return m_segments;
}

void MCTextLine::performLayout()
{
    // Clear the font metric and size information
    m_ascent = m_descent = m_leading = 0.0f;
    
    // Iterate through the list of segments and lay them out
    MCTextSegment* t_segment;
    uindex_t t_segment_count;
    coord_t t_previous_segment_end, t_xpos;
    t_segment = m_segments;
    t_segment_count = 0;
    t_previous_segment_end = t_xpos = 0.0f;
    do
    {
        // Tell the segment to lay itself out
        t_segment->setPosition(t_xpos, 0.0f);
        t_segment->performLayout();
        
        // Update the segment end position
        t_previous_segment_end = t_xpos + t_segment->getWidth();
        
        // Calculate the location of the next segment, based on the width of the
        // previous segments (so multiple-of-x tab positions work)
        coord_t t_x = t_xpos;
        t_xpos = offsetForSegment(t_segment_count, t_previous_segment_end);
        
        // Update the measurements for this line
        m_ascent = MCMax(m_ascent, t_segment->getTextAscent());
        m_descent = MCMax(m_descent, t_segment->getTextDescent());
        m_leading = MCMax(m_leading, t_segment->getTextLeading());
        
        // Move on to the next segment
        t_segment_count++;
        t_segment = t_segment->next();
    }
    while (t_segment != m_segments);
    
    // Width of the line is determined by the last segment in the line
    coord_t t_width;
    t_width = m_segments->prev()->getX() + m_segments->prev()->getWidth();
    
    // Update the maximum sizes of the segments - these are needed for alignment
    // and clipping purposes
    t_segment = m_segments;
    do
    {
        if (t_segment->next() != m_segments)
        {
            // Bound is the start of the next segment
            setMaxSize(t_segment->next()->getX() - t_segment->getX(), INFINITY);
        }
        else
        {
            // Bound is the rest of the line
            setMaxSize(t_width - t_segment->getX(), INFINITY);
        }
    
        t_segment = t_segment->next();
    }
    while (t_segment != m_segments);
    
    // Update the size of this line
    recordSize(t_width, m_ascent + m_descent + m_leading);
    finishLayout();
}

void MCTextLine::clearLayout()
{
    // Do nothing - the parent paragraph will clear via clearAllSegments
}

void MCTextLine::draw(MCDC* dc)
{
    // Draw each of the segments in turn
    MCTextSegment* t_segment;
    t_segment = m_segments;
    do
    {
        dc->save();
        dc->setorigin(t_segment->getX(), t_segment->getY());
        t_segment->draw(dc);
        dc->restore();
        
        t_segment = t_segment->next();
    }
    while (t_segment != m_segments);
}

void MCTextLine::repositionChildren()
{
    // Reposition each of the segments
    MCTextSegment* t_segment;
    coord_t t_segment_start, t_segment_end;
    uindex_t t_segment_count;
    t_segment = m_segments;
    t_segment_start = t_segment_end = 0.0f;
    t_segment_count = 0;
    do
    {
        // Calculate the starting position of this segment
        t_segment_start = offsetForSegment(t_segment_count, t_segment_end);
        
        // Calculate the ending position of the segment
        t_segment_end = t_segment_start + t_segment->getWidth();
        
        // If the text direction is reversed, the end points will be too
        if (getParagraph()->isReversedTextOrder())
        {
            t_segment->setPosition(getWidth() - t_segment_end, 0.0f);
        }
        else
        {
            t_segment->setPosition(t_segment_start, 0.0f);
        }
        
        // The blocks within the segment may need to react to alignment changes
        t_segment->repositionChildren();
        
        t_segment = t_segment->next();
    }
    while (t_segment != m_segments);
}

MCTextSegment* MCTextLine::addSegmentsWrapped(MCTextSegment* p_segments, coord_t p_max_width)
{
    // Our maximum bounds are implied by the max_width parameter
    setMaxSize(p_max_width, INFINITY);
    
    // Go through the list of segments, appending to the line until either the
    // length is exceeded or a line-breaking block is encountered
    MCTextSegment* t_segment;
    coord_t t_remaining_width;
    bool t_continue;
    t_segment = p_segments;
    t_remaining_width = p_max_width;
    do
    {
        bool t_first_segment;
        t_first_segment = t_segment == p_segments;
        t_continue = fitSegment(t_segment, t_first_segment, t_remaining_width);
        
        // The remove operation implicitly advances the t_segment pointer
        MCTextSegment* t_new_segment;
        t_new_segment = t_segment->remove(t_segment);
        t_new_segment->setLine(this);
        t_new_segment->appendto(m_segments);
        t_remaining_width -= t_new_segment->getWidth();
    }
    while (t_segment != NULL && t_continue);
    
    // This will be NULL if all segments were added
    return t_segment;
}

MCTextSegment* MCTextLine::addSegmentsNonwrapped(MCTextSegment* p_segments)
{
    return addSegmentsWrapped(p_segments, INFINITY);
}

MCTextBlock* MCTextLine::clearAllSegments()
{
    // Extract all blocks from the segments and delete the segments
    MCTextBlock* t_blocks;
    t_blocks = NULL;
    while (m_segments != NULL)
    {
        MCTextSegment* t_segment;
        MCTextBlock* t_segment_blocks;
        t_segment = m_segments->remove(m_segments);
        t_segment_blocks = t_segment->clearAllBlocks();
        t_segment_blocks->appendto(t_blocks);
        delete t_segment;
    }
    
    return t_blocks;
}

bool MCTextLine::endsWithExplicitLineBreak() const
{
    bool t_explicit_break;
    t_explicit_break = false;
    
    // Is the last block (of the last segment) of the line a break block?
    MCTextBlock* t_block;
    t_block = m_segments->prev()->getBlocks()->prev();
    if (t_block->getBlockType() == kMCTextBlockTypeBreak)
    {
        MCTextBreakBlock* t_break_block;
        t_break_block = static_cast<MCTextBreakBlock*>(t_block);
        t_explicit_break = t_break_block->getBreakClass() >= kMCTextBlockBreakClassLine;
    }
    
    return t_explicit_break;
}

bool MCTextLine::fitSegment(MCTextSegment* p_segment, bool p_first_segment, coord_t p_available_width)
{
    // If the available space is infinite, just scan forward for the first line
    // breaking block and skip the expense of running a breaking algorithm.
    if (isinf(p_available_width))
    {
        MCTextBlock* t_block;
        bool t_broken;
        t_block = p_segment->getBlocks();
        t_broken = false;
        do
        {
            if (t_block->getBlockType() == kMCTextBlockTypeBreak)
            {
                MCTextBreakBlock* t_break_block;
                t_break_block = static_cast<MCTextBreakBlock*>(t_block);
                if (t_break_block->getBreakClass() >= kMCTextBlockBreakClassLine)
                {
                    t_broken = true;
                    break;
                }
            }
            
            t_block = t_block->next();
        }
        while (t_block != p_segment->getBlocks());
        
        // The MCTextBlock -> MCTextSegment code places all breaks at the end of
        // segments - this is fine for line breaks too, so nothing needs to be
        // split off. Just say that fitting should not continue.
        return !t_broken;
    }
    
    // Get the text breaking engine for this paragraph
    MCTextBreakingEngine* t_engine;
    t_engine = m_parent->getBreakingEngine();
    
    // Fit as many blocks as possible into the line
    MCTextBlock *t_first_block, *t_last_block;
    bool t_continue;
    t_first_block = p_segment->getBlocks();
    t_continue = t_engine->fitBlocks(t_first_block, p_available_width, p_first_segment, t_last_block);
    
    // If not all blocks fit, a split will be required
    if (t_last_block != t_first_block->prev())
    {
        // Remove the excess blocks from the segment
        MCTextBlock* t_split_blocks;
        t_split_blocks = NULL;
        do
        {
            MCTextBlock* t_split_block;
            t_split_block = t_last_block->next()->remove(t_last_block);
            t_split_block->appendto(t_split_blocks);
        }
        while (t_last_block != t_first_block->prev());
        
        // Then add them to the a segment
        MCTextSegment* t_new_segment;
        t_new_segment = new MCTextSegment(m_parent);
        t_new_segment->addBlocks(t_split_blocks);
        
        p_segment->append(t_new_segment);
    }
    
    return t_continue;
}

coord_t MCTextLine::offsetForSegment(uindex_t p_index, coord_t p_prev_segment_end)
{
    // TODO: implement
    if (p_index > 0 || p_prev_segment_end > 0)
        return p_prev_segment_end + 20;
    else
        return 0;
}
