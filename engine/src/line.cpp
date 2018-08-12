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

#include "field.h"
#include "paragraf.h"
#include "MCBlock.h"
#include "segment.h"
#include "line.h"
#include "context.h"
#include "uidc.h"

#include "globals.h"
#include "foundation-bidi.h"

MCLine::MCLine(MCParagraph *paragraph)
{
	parent = paragraph;
	firstblock = lastblock = NULL;
    firstsegment = lastsegment = NULL;
    width = 0.0f;
    m_ascent = m_descent = m_leading = 0.0f;
    dirtywidth = 0.0f;
    m_offset = 0;
}

MCLine::~MCLine()
{
}

// SN-2015-01-21: [[ Bug 14229 ]] We want to get the length of the line we replace
void MCLine::takewidth(MCLine *lptr)
{
    if (width != lptr -> width)
        dirtywidth = MCU_max(width, lptr->width);
}

/*void MCLine::takebreaks(MCLine *lptr)
{
	if (firstblock != lptr->firstblock)
	{
		dirtywidth = MCU_max(width, lptr->width);
		firstblock = lptr->firstblock;
	}
	if (lastblock != lptr->lastblock)
	{
		dirtywidth = MCU_max(width, lptr->width);
		lastblock = lptr->lastblock;
	}
	if (width != lptr->width)
	{
		dirtywidth = MCU_max(width, lptr->width);
		width = lptr->width;
	}
	ascent = lptr->ascent;
	descent = lptr->descent;
    m_ascent = lptr->m_ascent;
    m_descent = lptr->m_descent;
    m_leading = lptr->m_leading;
	lptr->ascent = lptr->descent = 0;
    lptr->m_ascent = lptr->m_descent = lptr->m_leading = 0;
	lptr->firstblock = lptr->lastblock = NULL;
	lptr->width = 0;
}*/

/*MCBlock *MCLine::fitblocks(MCBlock* p_first, MCBlock* p_sentinal, uint2 p_max_width)
{
	MCBlock *t_block;
	t_block = p_first;
	
	coord_t t_frontier_width;
	t_frontier_width = 0;

	MCBlock *t_break_block;
	t_break_block = NULL;

	findex_t t_break_index;
	t_break_index = 0;

	do
	{
		t_block -> reset();

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
	while(t_block != p_sentinal);
	
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
			  parent -> TextIsWordBreak(parent -> GetCodepointAtIndex(t_break_index)))
            t_break_index++;
        {
            // SN-2015-01-21: [[ Bug 14421 ]] If the break block is a unicode block,
            //  then the next index is 2 'char' (a unichar) further, not one. This can lead to a
            //  size of 1 for a unicode block, and a hang in FontBreaking.
            if (t_break_block -> getflag(F_HAS_UNICODE))
                t_break_index += sizeof(unichar_t);
            else
                t_break_index++;
        }
		
		if (t_break_index < (t_break_block -> GetOffset() + t_break_block -> GetLength()))
			break;

        // Get the next non empty block.
        MCBlock *t_next_block;
        t_next_block = t_break_block -> next();
        while(t_next_block -> GetLength() == 0 && t_next_block != p_sentinal)
            t_next_block = t_next_block -> next();

		// If we are at the end of the list of blocks there is nothing more to do.
		if (t_next_block == p_sentinal)
			break;
		
		// If the first char of the next block is not a space, then there is nothing more
		// to do.
        // AL-2014-03-21: [[ Bug 11958 ]] Break index is paragraph index, not block index.
		if (!parent -> TextIsWordBreak(parent -> GetCodepointAtIndex(t_break_index)))
			break;
		
		// The next block starts with a space, so advance the break block.
		t_break_block = t_next_block;
		t_break_index = t_break_block -> GetOffset();
	}
	
	// MW-2012-02-21: [[ LineBreak ]] Check to see if there is a vtab char before the
	//   break index.
	bool t_is_explicit_line_break;
	t_is_explicit_line_break = false;
	t_block = p_first;
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
	if (t_break_index < t_break_block -> GetOffset() + t_break_block -> GetLength() ||
		(t_is_explicit_line_break && t_break_index == t_break_block -> GetOffset() + t_break_block -> GetLength()))
		t_break_block -> split(t_break_index);
		
	firstblock = p_first;
	lastblock = t_break_block;
	
	ResolveDisplayOrder();
	
	dirtywidth = width;
	
	return lastblock -> next();
}*/

void MCLine::appendall(MCBlock *bptr, bool p_flow)
{
	firstblock = bptr;
	lastblock = (MCBlock *)bptr->prev();
	coord_t oldwidth = width;
	width = 0;
	bptr = lastblock;
    SegmentLine();
    
    // Only resolve the display order if this line is not flowed (if it is
    // flowed, line breaking will be required and this effort will be wasted)
    if (!p_flow)
    {
        ResolveDisplayOrder();
    }
	
    dirtywidth = MCU_max(width, oldwidth);
    m_ascent = m_descent = m_leading = 0;
}

void MCLine::appendsegments(MCSegment *first, MCSegment *last)
{
    firstblock = first->GetFirstBlock();
    lastblock = last->GetLastBlock();
    
    firstsegment = first;
    lastsegment = last;
    
    m_ascent = m_descent = m_leading = 0;
    
    coord_t oldwidth = width;
    width = 0;
    dirtywidth = MCU_max(width, oldwidth);
    
    // Take ownership of the segments
    MCSegment *sgptr = firstsegment;
    do
    {
        sgptr->SetParent(this);
        sgptr = sgptr->next();
    }
    while (sgptr->prev() != lastsegment);
}

void MCLine::draw(MCDC *dc, int2 x, int2 y, findex_t si, findex_t ei, MCStringRef p_string, uint2 pstyle)
{
	MCSegment *sgptr = firstsegment;
    do
    {
        sgptr->Draw(dc, x, y, si, ei, p_string, pstyle);
        sgptr = sgptr->next();
    }
    while (sgptr->prev() != lastsegment);
}

void MCLine::setscents(MCBlock *bptr)
{
    coord_t t_aheight, t_dheight, t_leading;
    t_aheight = bptr->GetAscent();
    t_dheight = bptr->GetDescent();
    t_leading = bptr->GetLeading();
    if (t_aheight > m_ascent)
        m_ascent = t_aheight;
    if (t_dheight > m_descent)
        m_descent = t_dheight;
    if (t_leading > m_leading)
        m_leading = t_leading;
}

uint2 MCLine::getdirtywidth()
{
	return dirtywidth;
}

void MCLine::clean()
{
	dirtywidth = 0;
}

void MCLine::makedirty()
{
	dirtywidth = MCU_max(width, 1.0f);
}

void MCLine::GetRange(findex_t &i, findex_t &l)
{
	firstblock->GetRange(i, l);
	findex_t j;
	lastblock->GetRange(j, l);
	l = j + l - i;
}

coord_t MCLine::GetCursorXHelper(findex_t fi, bool prefer_forward)
{
    MCBlock *bptr = firstblock;
    MCSegment *sgptr = firstsegment;
	findex_t i, l;
	bptr->GetRange(i, l);
    
    while (bptr != lastblock
           // SN-2014-08-11: [[ Bug 13124 ]] If we are the last visual block of the segment,
           //  we don't split the cursor
           && (((bptr == sgptr -> GetLastVisualBlock() || prefer_forward) && fi >= i + l)
           || (!prefer_forward && fi > i + l)))
    {
        bptr = bptr -> next();
        bptr -> GetRange(i, l);
        
        // Advance to the next segment, if necessary
        if (sgptr->next()->GetFirstBlock() == bptr)
            sgptr = sgptr->next();
    }
   
    // The position of the segment containing the block needs to be included
    return bptr->GetCursorX(fi) + sgptr->GetLeftEdge();
}

coord_t MCLine::GetCursorXPrimary(findex_t fi, bool moving_forward)
{
	return GetCursorXHelper(fi, !moving_forward);
}

coord_t MCLine::GetCursorXSecondary(findex_t fi, bool moving_forward)
{
    return GetCursorXHelper(fi, moving_forward);
}

findex_t MCLine::GetCursorIndex(coord_t cx, Boolean chunk, bool moving_forward)
{    
    // BIDIRECTIONAL SUPPORT -
    //  Blocks cannot be assumed to be in visual order
    
    // TODO: when cx > line width, return the last block in visual order
    
    MCBlock *bptr = firstblock;
    MCSegment *sgptr = firstsegment;
    bool done = false;
    do
    {
        // Temporaries for visual ordering
        MCBlock *t_firstvisual, *t_lastvisual;
        t_firstvisual = t_lastvisual = bptr;
        
        bptr = sgptr->GetFirstBlock()->prev();
        do
        {
            bptr = bptr->next();

            // SN-2014-08-14: [[ Bug 13106 ]] We want the outside left edge
            coord_t origin = bptr->getorigin() + sgptr->GetLeft();

            // Different cases, according to the alignment:
            //  right-aligned: the origin belongs to the previous block
            //  others:        the origin belongs to the current block
            if ((sgptr -> GetHorizontalAlignment() == kMCSegmentTextHAlignRight
                        && cx > origin && cx <= (origin + bptr->getwidth()))
                    || (cx >= origin && cx <= (origin + bptr->getwidth())))
            {
                done = true;
                break;
            }
            
            if (bptr->GetVisualIndex() < t_firstvisual->GetVisualIndex())
                t_firstvisual = bptr;
            if (bptr->GetVisualIndex() > t_lastvisual->GetVisualIndex())
                t_lastvisual = bptr;
        }
        while (bptr != sgptr->GetLastBlock());
        
        // It is possible to be within the segment but not within a block due to
        // alignment within the segment. Pick the appropriate block.
        if (!done && cx > sgptr->GetLeft() && cx <= sgptr->GetRight())
        {
            // SN-2014-08-11: [[ Bug 13124 ]] We want to get the right block,
            // which might the one right before this segment in case we are centred/right-aligned
            
            // AL-2014-07-29: [[ Bug 12896 ]] Wrong coordinate returned for clicks in segment align space
            if (cx <= sgptr -> GetLeftEdge())
            {
                // Before the first block in visual ordering
                if (sgptr == firstsegment)
                    bptr = sgptr -> GetFirstVisualBlock();
                else
                {
                    // We need to find the appropriate block in which the cursor must be rendered
                    coord_t t_limit;
                    t_limit = (sgptr -> prev() -> GetRightEdge() + sgptr -> GetLeftEdge()) / 2;
                    
                    if (cx < t_limit)
                    {
                        bptr = sgptr -> prev() -> GetLastVisualBlock();
                        sgptr = sgptr -> prev();
                    }
                    else
                        bptr = sgptr -> GetFirstVisualBlock();
                }
                done = true;
            }
            else
            {
                // Otherwise, must be after the last block in visual ordering
                if (sgptr == lastsegment)
                    bptr = sgptr -> GetLastVisualBlock();
                else
                {
                    // We need to find the appropriate block in which the cursor must be rendered
                    coord_t t_limit;
                    t_limit = (sgptr -> GetRightEdge() + sgptr -> next() -> GetLeftEdge()) / 2;
                    
                    if (cx < t_limit)
                        bptr = sgptr -> GetLastVisualBlock();
                    else
                    {
                        bptr = sgptr -> next() -> GetFirstVisualBlock();
                        sgptr = sgptr -> next();
                    }
                }
                done = true;
            }
        }
        
        if (done)
            break;
        
        sgptr = sgptr -> next();
    }
    while (sgptr->prev() != lastsegment);
    
    return bptr->GetCursorIndex(cx - sgptr->GetLeftEdge(), chunk, bptr == lastblock, moving_forward);
}

uint2 MCLine::getwidth()
{
    // AL-2014-10-21: [[ Bug 13403 ]] Returned line width as integer needs to be rounded up
	return ceil(width);
}

coord_t MCLine::GetAscent() const
{
    return m_ascent;
}

coord_t MCLine::GetDescent() const
{
    return m_descent;
}

coord_t MCLine::GetLeading() const
{
    return m_leading;
}

coord_t MCLine::GetHeight() const
{
    return GetAscent() + GetDescent() + GetLeading();
}

// MW-2012-02-10: [[ FixedTable ]] In fixed-width table mode the paragraph needs to
//   explicitly set the width of the table.
void MCLine::setwidth(uint2 p_new_width)
{
	width = p_new_width;
}

void MCLine::ResolveDisplayOrder()
{
    // Resolve the display order of each of the segments in turn
    MCSegment *sgptr = firstsegment;
    do
    {
        sgptr->ResolveDisplayOrder();
        sgptr = sgptr->next();
    }
    while (sgptr->prev() != lastsegment);
}

void MCLine::SegmentLine()
{
    // Scan through each block on this line, looking for tab characters
    MCBlock *bptr = firstblock;
    MCBlock *segment_start = firstblock;
    uindex_t t_segment_length = 0;
    do
    {
        // Does this block contain a tab?
        uindex_t t_offset;
        // AL-2014-08-21: [[ Bug 13247 ]] Don't repeatedly search to the end of the paragraph to find tab chars.
        //  The search should only be in the range of the block.
        if (MCStringFirstIndexOfCharInRange(parent->GetInternalStringRef(), '\t', MCRangeMake(bptr -> GetOffset(), bptr -> GetLength()), kMCStringOptionCompareExact, t_offset))
        {
            // Split the block at the tab unless it would create an empty block
            // FG-2014-10-31: [[ Bugfix 13887 ]]
            // There was previously a comment here saying that creating the
            // empty block was a good thing. I have no idea why I wrote that...
            // Removed that behaviour as it breaks some things.
            // MW-2017-08-07: [[ Bug 17098 ]]
            // If the block has a trailing tab *and is the last block* we need
            // to make sure there is an empty block, otherwise we don't get a
            // segment for a trailing tab which means caret alignment / placement
            // doesn't work. This strategy seems to fix 17098 and not reintroduce
            // bug 13887.
            if ((t_offset + 1) < bptr->GetOffset() + bptr->GetLength() ||
                (bptr == lastblock && bptr->HasTrailingTab()))
            {
                bptr->split(t_offset + 1);
                if (bptr == lastblock)
                    lastblock = bptr->next();
            }
            
            // Create a segment covering the text up to this tab character
            MCSegment *new_segment = new (nothrow) MCSegment(this);
            new_segment->AddBlockRange(segment_start, bptr);
            if (firstsegment == NULL)
            {
                firstsegment = lastsegment = new_segment;
                if (parent->segments == NULL)
                    parent->segments = new_segment;
            }
            else
            {
                lastsegment->append(new_segment);
                lastsegment = new_segment;
            }

            segment_start = bptr->next();
            t_segment_length = 0;
        }
        else
        {
            t_segment_length++;
        }
        
        // Move on to the next block
        bptr = bptr->next();
    }
    while (bptr != firstblock);
    
    // Create a segment covering the remaining text
    if (t_segment_length > 0)
    {
        MCSegment *new_segment = new (nothrow) MCSegment(this);
        new_segment->AddBlockRange(segment_start, lastblock);
        if (firstsegment == NULL)
        {
            firstsegment = lastsegment = new_segment;
            if (parent->segments == NULL)
                parent->segments = new_segment;
        }
        else
        {
            lastsegment->append(new_segment);
            lastsegment = new_segment;
        }
    }
}

int16_t MCLine::CalculateTabPosition(uindex_t p_which_tab, int16_t p_from_position)
{
    // No tabs means the beginning of the line
    if (p_which_tab == 0)
        return 0;
    
    p_which_tab--;
    
    // Get the tab information from the parent paragraph
    uint16_t *t_tabs;
    uint16_t t_numtabs;
    Boolean t_fixed_tabs;
    parent->gettabs(t_tabs, t_numtabs, t_fixed_tabs);
    
    // What is the line offset for the beginning of this segment?
    uint16_t t_segment_pos = 0;
    if (t_fixed_tabs)
    {
        if (p_which_tab < t_numtabs)
        {
            // Just get the position for the list of tabstops
            t_segment_pos = t_tabs[p_which_tab];
        }
        else
        {
            // We have gone past the end of the list of fixed tabstops. In
            // this case, we calculate the position based on implicit stops;
            // these implicit stops are the same width as the gap between
            // the last two fixed stops.
            uint16_t t_diff;
            if (t_numtabs == 1)
                t_diff = t_tabs[0];     // Previous tabstop is implicitly 0
            else
                t_diff = t_tabs[t_numtabs - 1] - t_tabs[t_numtabs - 2];
            
            // There have been (t_segments - t_numtabs) tabs beyond the last
            // fixed tabstop
            t_segment_pos = t_tabs[t_numtabs - 1] + ((p_which_tab+1) - t_numtabs) * t_diff;
        }
        
        // If the position of the current tab co-incides with the last x position then
		// adjust upward by one to ensure we don't overwrite anything (legacy behavior).
		if (t_segment_pos == p_from_position)
			t_segment_pos += 1;
    }
    else
    {
        // Find the first fixed-position tabstop that is beyond the end of
        // the previous segment
        uindex_t i = 0;
        while (i < t_numtabs && t_segment_pos <= p_from_position)
            t_segment_pos = t_tabs[i++];
        
        // If no valid fixed-position tabstops were found, use implicit
        if (p_from_position >= t_segment_pos && t_numtabs != 0)
        {
            // We have gone past the end of the list of fixed tabstops. In
            // this case, we calculate the position based on implicit stops;
            // these implicit stops are the same width as the gap between
            // the last two fixed stops.
            uint16_t t_diff;
            if (t_numtabs == 1)
                t_diff = t_tabs[0];     // Previous tabstop is implicitly 0
            else
                t_diff = t_tabs[t_numtabs - 1] - t_tabs[t_numtabs - 2];
            
            // MW-2012-09-19: [[ Bug 10239 ]] The tab difference can now be zero, in
            //   the non-vGrid case, if this is the case then just take lasttab to be
            //   x.
            if (t_diff != 0)
                t_segment_pos = t_tabs[t_numtabs - 1] + t_diff * ((p_from_position - t_tabs[t_numtabs - 1]) / t_diff + 1);
            else
                t_segment_pos = p_from_position;
        }
        
        // If the position of the current tab co-incides with the last x position then
		// adjust upward by one to ensure we don't overwrite anything (legacy behavior).
		if (t_segment_pos == p_from_position)
			t_segment_pos += 1;
    }
    
    return t_segment_pos;
}

intenum_t MCLine::CalculateTabAlignment(uindex_t p_which_tab)
{
    intenum_t *t_alignments;
    uint16_t t_count;
    parent -> gettabaligns(t_alignments, t_count);
    
    if (t_count == 0)
        return parent->getbasetextdirection() == kMCTextDirectionRTL ? kMCSegmentTextHAlignRight : kMCSegmentTextHAlignLeft;
    
    if (p_which_tab < t_count)
        return t_alignments[p_which_tab];
    else
        return t_alignments[t_count - 1];
}

void MCLine::NoFlowLayout()
{
    MCLine *test = DoLayout(false, 0);
    MCAssert(test == NULL);
}

MCLine *MCLine::Fit(int16_t p_linewidth)
{
    return DoLayout(true, p_linewidth);
}

MCLine *MCLine::DoLayout(bool p_flow, int16_t p_linewidth)
{
    // A count of fitted segments is required so that the correct tab can be
    // chosen when fitting segments.
    uindex_t t_segments = 0;
    
    // Non-fixed tabstops are calculated as offsets from the last tab position
    // while fixed tabstops are always at the same position, regardless of the
    // length of text between the stops.
    coord_t t_last_segment_end = 0;
    
    // We need to know whether the tabstops are fixed or flexible
    Boolean t_fixed_tabs;
    uint16_t *t_tab_positions;
    uint16_t t_num_tabs;
    parent->gettabs(t_tab_positions, t_num_tabs, t_fixed_tabs);
    
    // Try to fit each segment into the line
    MCSegment *sgptr = firstsegment;
    MCLine *t_remaining = NULL;
    do
    {
        // Get the starting position for this segment
        int16_t t_segment_pos;
        t_segment_pos = CalculateTabPosition(t_segments, ceilf(t_last_segment_end));
        
        // Set the alignment of the segment.
        // This is done early to ensure all segments have an alignment set.
        sgptr->SetHorizontalAlignment(CalculateTabAlignment(t_segments));
        sgptr->SetVerticalAlignment(kMCSegmentTextVAlignTop);
        
        // We now know where the segment will be placed and therefore how much
        // room remains in the line for the segment. Tell it to do block fitting
        // - if not all blocks can fit, a segment containing the remainder will
        // be returned.
        if (p_flow)
            t_remaining = sgptr->Fit(p_linewidth - t_segment_pos);
        else
            t_remaining = NULL;
        
        // Fitting has been completed; get the width of the segment so that
        // the segment boundaries can be calculated
        // AL-2014-10-28: [[ Bug 13829 ]] Calculate line width as floating point
        coord_t t_segment_width;
        t_segment_width = sgptr->GetContentLength();

        // Position at which the next segment will be placed
        coord_t t_next_segment_pos;
        t_next_segment_pos = CalculateTabPosition(t_segments + 1, t_segment_pos + ceilf(t_segment_width));
        
        // The last segment of the line should be no larger than its contents
        // (because it doesn't contain the whitespace of another tab) unless it
        // is to be right aligned in LTR text or left-aligned in RTL text or it
        // terminates with a tab character.
        if (!t_fixed_tabs
            && (sgptr == lastsegment && !sgptr->GetLastBlock()->HasTrailingTab())
            && ((parent->getbasetextdirection() != kMCTextDirectionRTL && sgptr->GetHorizontalAlignment() != kMCSegmentTextHAlignRight)
            ||  (parent->getbasetextdirection() == kMCTextDirectionRTL && sgptr->GetHorizontalAlignment() != kMCSegmentTextHAlignLeft)))
        {
            t_next_segment_pos = t_segment_pos + t_segment_width;
        }
        
        // Update the width of the line
        width += t_next_segment_pos - t_segment_pos;
        
        // Tell the segment its boundaries. If the line is right-to-left, some
        // post-processing will be required to fix up the boundaries.
        int16_t t_left, t_right, t_top, t_bottom;
        t_left = t_segment_pos;
        t_right = ceilf(t_next_segment_pos);
        t_top = 0;
        t_bottom = 0;
        sgptr->SetBoundaries(t_left, t_right, t_top, t_bottom);
        
        // End the segment fitting if we have run out of space
        if (t_remaining != NULL)
            break;
        
        // Next segment
        t_segments++;
        sgptr = sgptr->next();
        t_last_segment_end = t_segment_pos + t_segment_width;
    }
    while (sgptr->prev() != lastsegment);
    
    // Fix the segment boundaries for right-to-left lines
    if (parent->getbasetextdirection() == kMCTextDirectionRTL)
    {
        sgptr = firstsegment;
        do
        {
            int16_t t_left, t_right, t_top, t_bottom;
            t_left = width - sgptr->GetRight();
            t_right = width - sgptr->GetLeft();
            t_top = 0;
            t_bottom = 0;
            sgptr->SetBoundaries(t_left, t_right, t_top, t_bottom);
            
            sgptr = sgptr->next();
        }
        while (sgptr->prev() != lastsegment);
    }
    
    // Calculate the drawing offset for this line, based on alignment (this is
    // only appropriate for fields with a fixed width, however)
    if (p_flow && !parent->getvgrid())
    {
        switch (parent->gettextalign())
        {
            case kMCParagraphTextAlignLeft:
            case kMCParagraphTextAlignJustify:
                m_offset = 0;
                break;
                
            case kMCParagraphTextAlignCenter:
                m_offset = (p_linewidth - width) / 2;
                break;
                
            case kMCParagraphTextAlignRight:
                m_offset = p_linewidth - width;
                break;
        }
    }
    else
    {
        // Alignment for non-wrapping lines is handled by the paragraph
        m_offset = 0;
    }
    
    // Update the line's drawing properties
    dirtywidth = width;
    ResolveDisplayOrder();
    
    return t_remaining;
}
