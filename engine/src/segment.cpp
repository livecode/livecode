/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "paragraf.h"
#include "MCBlock.h"
#include "line.h"

#include "uidc.h"
#include "context.h"

#include "segment.h"


MCSegment::MCSegment(MCLine *p_parent)
    : m_FirstVisualBlock(),
      m_LastVisualBlock(),
      m_HAlign(),
      m_VAlign()
{
    m_Parent = p_parent;
    m_FirstBlock = m_LastBlock = NULL;
    
    m_LeftEdge = m_RightEdge = m_TopEdge = m_BottomEdge = 0;
    m_ContentWidth = 0;
    // SN-2014-08-14: [[ Bug 13106 ]] m_Padding initialisation according to the VGrid property of the field
    if (p_parent -> getparent() -> getvgrid())
        m_Padding = p_parent -> getparent() -> gethpadding();
    else
        m_Padding = 0.0;
}


MCSegment::MCSegment(const MCSegment *sg)
    : m_FirstVisualBlock(),
      m_LastVisualBlock()
{
    m_Parent = sg->m_Parent;
    m_FirstBlock = m_LastBlock = NULL;
    
    m_LeftEdge = m_RightEdge = m_TopEdge = m_BottomEdge = 0;
    m_ContentWidth = 0;
    
    m_HAlign = sg->m_HAlign;
    m_VAlign = sg->m_VAlign;
    // SN-2014-08-14: [[ Bug 13106 ]] m_Padding member added to Segment
    m_Padding = sg->m_Padding;
}


MCSegment::~MCSegment()
{
    
}

void MCSegment::AddBlockRange(MCBlock *first, MCBlock *last)
{
    m_FirstBlock = first;
    m_LastBlock = last;
    
    MCBlock *bptr = m_FirstBlock;
    do
    {
        bptr -> SetSegment(this);
        bptr = bptr->next();
    }
    while (bptr->prev() != m_LastBlock);
}

coord_t MCSegment::GetContentLength()
{
    if (m_ContentWidth != 0)
        return m_ContentWidth;
    
    MCBlock *bptr = m_FirstBlock;
    do
    {
        m_ContentWidth += bptr->getwidth();
        bptr = bptr->next();
    }
    while (bptr->prev() != m_LastBlock);
    
    return m_ContentWidth;
}

int16_t MCSegment::GetContentHeight() const
{
    // TODO: implement
    return 0;
}

MCLine *MCSegment::Fit(coord_t p_max_width)
{
    MCBlock *t_block;
    t_block = m_FirstBlock;
    
    coord_t t_frontier_width;
    t_frontier_width = 0;
    
    MCBlock *t_break_block;
    t_break_block = NULL;
    
    findex_t t_break_index;
    t_break_index = 0;
    
    do
    {
        t_block->reset();
        
        // MW-2008-07-08: [[ Bug 6475 ]] Breaking works incorrectly on lines with tabs and multiple blocks.
		//   This is due to the tab computations within the block being incorrect as t_frontier_width was
		//   not being passed (width == 0 was being passed instead).
		findex_t t_new_break_index;
		bool t_break_fits, t_block_fits;
		t_block_fits = t_block -> fit(t_frontier_width, p_max_width < t_frontier_width ? 0 : p_max_width - t_frontier_width, t_new_break_index, t_break_fits);
        
        bool t_continue;
		if (t_block_fits)
		{
			if (t_new_break_index > t_block -> GetOffset() || (t_block -> GetLength() == 0 && t_break_fits))
			{
				// The whole block fits, so record the break position
				t_break_block = t_block;
				t_break_index = t_new_break_index;
			}
			else
			{
				// There is no break, so we need to look ahead to find a block
				// with a break
			}
            
			t_continue = true;
		}
		else
		{
			if (t_new_break_index > t_block -> GetOffset())
			{
				// We have a break position but the whole block doesn't fit
				
				if (t_break_fits)
				{
					// The break fits, but the block doesn't so finish
					t_break_block = t_block;
					t_break_index = t_new_break_index;
					t_continue = false;
				}
				else if (t_break_block != NULL)
				{
					// The break doesn't fit and we've seen a break before
					t_continue = false;
				}
				else
				{
					// The break doesn't fit and we've not seen a break before
					t_break_block = t_block;
					t_break_index = t_new_break_index;
					t_continue = false;
				}
			}
			else
			{
				// We have no break position and the block doesn't fit
				
				if (t_break_block != NULL)
				{
					// The block doesn't fit and we've seen a break before
					t_continue = false;
				}
				else
				{
					// We have no previous break and the block doesn't fit
					// continue until we get to a break or the last block
					t_continue = true;
				}
			}
		}
        
		if (!t_continue)
			break;
        
		t_frontier_width += t_block -> getwidth(NULL, t_frontier_width);
		
		t_block = t_block -> next();
    }
    while (t_block->prev() != m_LastBlock);
    
    if (t_break_block == NULL)
	{
		t_break_block = t_block -> prev();
		t_break_index = t_break_block -> GetOffset() + t_break_block -> GetLength();
	}
    
    // MW-2014-01-06: [[ Bug 11628 ]] We have a break block and index, so now extend the
	//   break index through any space chars.
	for(;;)
	{
		// Consume all spaces after the break index.
        // AL-2014-03-21: [[ Bug 11958 ]] Break index is paragraph index, not block index.
		while(t_break_index < (t_break_block -> GetOffset() + t_break_block -> GetLength()) &&
			  MCParagraph::TextIsWordBreak(m_Parent->getparent() -> GetCodepointAtIndex(t_break_index)))
			t_break_index++;
		
		if (t_break_index < (t_break_block -> GetOffset() + t_break_block -> GetLength()))
			break;
        
        // Get the next non empty block.
        MCBlock *t_next_block;
        t_next_block = t_break_block -> next();
        while(t_next_block -> GetLength() == 0 && t_next_block->prev() != m_LastBlock)
            t_next_block = t_next_block -> next();
        
		// If we are at the end of the list of blocks there is nothing more to do.
		if (t_next_block->prev() == m_LastBlock)
			break;
		
		// If the first char of the next block is not a space, then there is nothing more
		// to do.
        // AL-2014-03-21: [[ Bug 11958 ]] Break index is paragraph index, not block index.
		if (!MCParagraph::TextIsWordBreak(m_Parent->getparent() -> GetCodepointAtIndex(t_break_index)))
			break;
		
		// The next block starts with a space, so advance the break block.
		t_break_block = t_next_block;
		t_break_index = t_break_block -> GetOffset();
	}
    
    // MW-2012-02-21: [[ LineBreak ]] Check to see if there is a vtab char before the
	//   break index.
	bool t_is_explicit_line_break;
	t_is_explicit_line_break = false;
	t_block = m_FirstBlock;
	do
	{
		findex_t t_line_break_index;
		if (t_block -> GetFirstLineBreak(t_line_break_index))
		{
			// If the explicit line break is the same as the break, then make sure we
			// mark it as explicit so that line breaks at ends of lines work.
			if (t_line_break_index <= t_break_index)
			{
				t_is_explicit_line_break = true;
				t_break_index = t_line_break_index;
				t_break_block = t_block;
			}
			break;
		}
		t_block = t_block -> next();
	}
	while(t_block != t_break_block -> next());
    
    // If the break index is before the end of the block *or* if we are explicit and it
	// is at the end of the block, split the block. [ The latter rule means there is an
	// empty block to have as a line ].
    bool t_need_break_segment = t_break_block != m_LastBlock || t_break_index != m_LastBlock->GetOffset() + m_LastBlock->GetLength();
    bool t_need_break_block = t_break_index < t_break_block -> GetOffset() + t_break_block -> GetLength() ||
    (t_is_explicit_line_break && t_break_index == t_break_block -> GetOffset() + t_break_block -> GetLength());
    
	if (t_need_break_block)
    {
		t_break_block -> split(t_break_index);
        
        // Temporarily update the last block pointer, if required
        if (t_break_block == m_LastBlock)
            m_LastBlock = m_LastBlock->next();
    }
    
    // If this is not the first segment of the line and there were no suitable
    // breaking locations before we ran out of space, split the line before
    // this segment (assuming the segment was created using a tab)
    bool t_need_break_line = false;
    if (!t_need_break_segment && !t_need_break_block
        && t_frontier_width > p_max_width && this != m_Parent->GetFirstSegment()
        && prev()->GetLastBlock()->GetCodepointAtIndex(prev()->GetLastBlock()->GetLength() - 1) == '\t')
    {
        t_need_break_line = true;
    }
    
    // Was breaking required?
    // FG-2014-10-21: [[ Bugfix 13727 ]] Breaking a block implies breaking a segment
    MCLine *t_newline = NULL;
    if (t_need_break_line || t_need_break_segment || t_need_break_block)
    {
        // A block was broken and therefore the segment possibly needs to be split
        MCSegment *t_split_segment;
        if (t_need_break_line)
        {
            t_split_segment = this;
            t_break_block = m_FirstBlock->prev();
        }
        else if (t_break_block != m_LastBlock)
        {
            // Split this segment
            t_split_segment = new (nothrow) MCSegment(this);
            append(t_split_segment);
            t_split_segment->AddBlockRange(t_break_block->next(), m_LastBlock);
            
            // Temporarily update the last segment pointer, if required
            if (m_Parent->lastsegment == this)
                m_Parent->lastsegment = t_split_segment;
        }
        else
        {
            // The break was at the end of a segment
            t_split_segment = next();
        }
        
        MCLine* t_parent_line;
        MCSegment* t_prev_segment;
        t_parent_line = m_Parent;
        t_prev_segment = t_split_segment->prev();
        
        // Update the last block pointer if we're not taking all of the segment
        if (t_split_segment != this)
            m_LastBlock = t_break_block;
        
        // Create a new line containing the segments and blocks that don't fit
        t_newline = new (nothrow) MCLine(*m_Parent);
        t_newline->appendsegments(t_split_segment, t_parent_line->lastsegment);
        
        // Update the parent line's block and segment pointers
        t_parent_line->lastblock = t_break_block;
        t_parent_line->lastsegment = t_prev_segment;
    }
    
    // Return the new line, if it exists
    return t_newline;
}

void MCSegment::Draw(MCDC *dc, coord_t p_line_origin_x, int16_t p_line_origin_y, findex_t si, findex_t ei, MCStringRef p_text, uint16_t p_style)
{
    MCBlock *bptr = m_FirstBlock;
    
    uint32_t t_flags;
    t_flags = 0;
    
    bool t_is_flagged;
    t_is_flagged = false;
    int32_t t_flagged_sx, t_flagged_ex;
    t_flagged_sx = 0;
    t_flagged_ex = 0;

    // Calculate the coordinates for drawing the contents of the segment
    coord_t x, y;
    if (m_HAlign == kMCSegmentTextHAlignLeft)
    {
        // Left-hand edge of the cell
        x = p_line_origin_x + m_LeftEdge;
    }
    else if (m_HAlign == kMCSegmentTextHAlignCenter)
    {
        // Centre of the cell minus half the length of the content
        // SN-2014-08-14: [[ Bug 13106 ]] Update to GetInnerWidth
        x = p_line_origin_x + m_LeftEdge + (GetInnerWidth() / 2) - (GetContentLength() / 2);
    }
    else if (m_HAlign == kMCSegmentTextHAlignRight)
    {
        // Right-hand edge of the cell
        // SN-2014-08-14: [[ Bug 13106 ]] Update to GetInnerWidth
        x = p_line_origin_x + m_LeftEdge + GetInnerWidth() - GetContentLength();
    }
    else    // m_HAlign == kMCSegmentTextHAlignJustify
    {
        MCUnreachableReturn();
    }
    
    // TODO: vertical alignment
    y = p_line_origin_y;
    
    do
    {
        // MW-2012-02-27: [[ Bug 2939 ]] We need to compute whether to render the left/right
		//   edge of the box style. This is determined by whether the previous block has such
		//   a style or not.
		
		// Fetch the style of this block.
		uint2 t_this_style;
		if (!bptr -> gettextstyle(t_this_style))
			t_this_style = p_style;
		
		// Start off with 0 flags.
		t_flags = 0;
		
		// If this block has a box style then we have something to do.
		if ((t_this_style & (FA_BOX | FA_3D_BOX)) != 0)
		{
			// If we are not the first block, check the text style of the previous one.
			// Otherwise we must render the left edge.
			if (bptr != m_FirstBlock)
			{
				uint2 t_prev_style;
				if (!bptr -> prev() -> gettextstyle(t_prev_style))
					t_prev_style = p_style;
				if ((t_this_style & FA_BOX) != 0 && (t_prev_style & FA_BOX) == 0)
					t_flags |= DBF_DRAW_LEFT_EDGE;
				else if ((t_this_style & FA_3D_BOX) != 0 && (t_prev_style & FA_3D_BOX) == 0)
					t_flags |= DBF_DRAW_LEFT_EDGE;
			}
			else
				t_flags |= DBF_DRAW_LEFT_EDGE;
			
			// If we are not the last block, check the text style of the next one.
			// Otherwise we must render the right edge.
			if (bptr != m_LastBlock)
			{
				uint2 t_next_style;
				if (!bptr -> next() -> gettextstyle(t_next_style))
					t_next_style = p_style;
				if ((t_this_style & FA_BOX) != 0 && (t_next_style & FA_BOX) == 0)
					t_flags |= DBF_DRAW_RIGHT_EDGE;
				else if ((t_this_style & FA_3D_BOX) != 0 && (t_next_style & FA_3D_BOX) == 0)
					t_flags |= DBF_DRAW_RIGHT_EDGE;
			}
			else
				t_flags |= DBF_DRAW_RIGHT_EDGE;
		}
        
		// Pass the computed flags to the block to draw.
        // SN-2014-08-13: [[ Bug 13016 ]] Added a parameter for the left of the cell
		bptr->draw(dc, x + bptr->getorigin(), p_line_origin_x + m_LeftEdge, p_line_origin_x + m_RightEdge, y, si, ei, p_text, p_style, t_flags);

		coord_t twidth;
		twidth = bptr->getwidth(dc);
		
		if (bptr -> getflagged())
		{
			// AL-2014-09-01: [[ Bug 13311 ]] Take block origin into account when drawing flagged range
			if (!t_is_flagged)
			{
				t_is_flagged = true;
				t_flagged_sx = floorf(x + bptr -> getorigin());
			}
			t_flagged_ex = ceilf(x + bptr -> getorigin() + twidth);
		}
		
		if (t_is_flagged && (!bptr -> getflagged() || bptr == m_LastBlock))
		{
			static uint1 s_dashes[2] = {3, 2};
			MCColor t_color;
			t_color . red = 255 << 8;
			t_color . green = 0 << 8;
			t_color . blue = 0 << 8;
			dc -> setforeground(t_color);
			dc -> setquality(QUALITY_SMOOTH);
			dc -> setlineatts(2, LineOnOffDash, CapButt, JoinRound);
			dc -> setdashes(0, s_dashes, 2);
			dc -> drawline(t_flagged_sx, y + 1, t_flagged_ex, y + 1);
			dc -> setlineatts(0, LineSolid, CapButt, JoinBevel);
			dc -> setquality(QUALITY_DEFAULT);
			t_is_flagged = false;
			m_Parent->getparent()->getparent()->setforeground(dc, DI_FORE, False, True);
		}
        
        bptr = bptr->next();
    }
    while (bptr->prev() != m_LastBlock);
}

void MCSegment::ResolveDisplayOrder()
{
    // Count the number of blocks in the line
    uindex_t t_block_count;
    t_block_count = 1;
    MCBlock *bptr;
    bptr = m_FirstBlock;
    while (bptr != m_LastBlock)
    {
        t_block_count++;
        bptr = bptr->next();
    }
    
    // Create an array to store the blocks in visual order
    MCAutoArray<MCBlock *> t_visual_order;
    /* UNCHECKED */ t_visual_order.New(t_block_count);
    
    // Initialise the visual order to be the logical order. Also take this
    // opportunity to find the highest and lowest direction level in the line.
    uint8_t t_max_level, t_min_level;
    t_max_level = 0;
    t_min_level = 255;
    bptr = m_FirstBlock;
    for (uindex_t i = 0; i < t_block_count; i++)
    {
        uint8_t t_level;
        t_level = bptr->GetDirectionLevel();
        if (t_level > t_max_level)
            t_max_level = t_level;
        
        if (t_level < t_min_level)
            t_min_level = t_level;
        
        t_visual_order[i] = bptr;
        bptr = bptr->next();
    }
    
    // Unicode Bidi Algorithm rule L2:
    //  "From the highest level found in the text to the lowest odd level on each line,
    //   including intermediate levels not actually present in the text, reverse any
    //   contiguous sequence of characters that are at that level or higher."
    //
    // We don't actually reverse the text here, just the ordering of the blocks.
    // To avoid creating a new stringref, a simple flag is used to indicate reversal.
    
    // Adjust the min level to be the minimum odd level
    t_min_level += (t_min_level & 1) ? 0 : 1;
    
    for (uindex_t i = t_max_level; i >= t_min_level; i--)
    {
        // Scan the block list for a block at this level or higher
        uindex_t j = 0;
        while (j < t_block_count)
        {
            // Not at the desired level; move to the next block
            if (t_visual_order[j]->GetDirectionLevel() < i)
            {
                j++;
                continue;
            }
            
            // Scan for the last contigious block at or above this level
            uindex_t t_length;
            t_length = 0;
            while (j + t_length < t_block_count && t_visual_order[j + t_length]->GetDirectionLevel() >= i)
                t_length++;
            
            // Reverse the affected section of the visual order array
            uindex_t t_even, t_pivot, t_stride;
            t_even = 1 - (t_length & 1);
            t_stride = t_length >> 1;
            t_pivot = j + t_stride;
            while (t_stride > 0)
            {
                uindex_t t_low, t_high;
                t_low = t_pivot - t_stride;
                t_high = t_pivot + t_stride - t_even;
                
                // TODO: toggle the "reversed" flag on the block or is the encoding enough?
                
                MCBlock *temp = t_visual_order[t_low];
                t_visual_order[t_low] = t_visual_order[t_high];
                t_visual_order[t_high] = temp;
                
                t_stride--;
            }
            
            // This run of blocks has been reversed
            j += t_length;
        }
    }
    
    // The blocks are now in visual order. Calculate their positions (and also
    // the width of this line). A second pass will be needed to resolve the
    // offsets to be used when calculating tabstops.
    coord_t t_width = 0;
    for (uindex_t i = 0; i < t_block_count; i++)
    {
        bptr = t_visual_order[i];
        m_Parent->setscents(bptr);
        bptr -> setorigin(t_width);
        bptr -> SetVisualIndex(i);
        t_width += bptr -> getwidth(NULL);
    }
    
    m_FirstVisualBlock = t_visual_order[0];
    m_LastVisualBlock = t_visual_order[t_block_count - 1];
}

coord_t MCSegment::GetLeftEdge()
{
    // The offset depends on the alignment of the segment
    // SN-2014-08-14: [[ Bug 13106 ]] GetLeftEdge updated to adapt to the VGrid mode
    switch (m_HAlign)
    {
        case kMCSegmentTextHAlignLeft:
        case kMCSegmentTextHAlignJustify:
            return m_LeftEdge + m_Padding;
        case kMCSegmentTextHAlignCenter:
            if (m_Parent -> getparent() -> getvgrid())
                return MCMax(m_LeftEdge + (GetWidth() - GetContentLength()) / 2,
                             m_LeftEdge + m_Padding);
            else
                return m_LeftEdge + (GetWidth() - GetContentLength()) / 2;
        case kMCSegmentTextHAlignRight:
            if (m_Parent -> getparent() -> getvgrid())
                return MCMax(m_RightEdge - m_Padding - GetContentLength(),
                             m_LeftEdge + m_Padding);
            else
                return m_RightEdge - GetContentLength();
        default:
            MCAssert(false);
            return 0;
    }
}

coord_t MCSegment::GetRightEdge()
{
    // The right limit depends on the TAB alignment
    // SN-2014-08-14: [[ Bug 13106 ]] GetRightEdge updated to adapt to the VGrid mode
    switch(m_HAlign)
    {
        case kMCSegmentTextHAlignLeft:   
            if (m_Parent -> getparent() -> getvgrid())
                return MCMin(m_LeftEdge + m_Padding + GetContentLength(),
                             m_RightEdge - m_Padding);
            else
                return ((coord_t)m_LeftEdge + m_Padding) + GetContentLength();
        case kMCSegmentTextHAlignCenter:
            if (m_Parent -> getparent() -> getvgrid())
                return MCMin(m_RightEdge - (GetWidth() - GetContentLength()) / 2,
                             m_RightEdge - m_Padding);
            else
                return m_RightEdge - (GetWidth() - GetContentLength()) / 2;
        case kMCSegmentTextHAlignRight:
            return m_RightEdge - m_Padding;
        default:
            MCAssert(false);
            return 0;
    }
}
