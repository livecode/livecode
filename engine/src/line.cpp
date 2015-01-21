/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "block.h"
#include "line.h"
#include "context.h"
#include "uidc.h"

#include "globals.h"

MCLine::MCLine(MCParagraph *paragraph)
{
	parent = paragraph;
	firstblock = lastblock = NULL;
	width = ascent = descent = 0;
	dirtywidth = 0;
}

MCLine::~MCLine()
{
}

void MCLine::takebreaks(MCLine *lptr)
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
	lptr->ascent = lptr->descent = 0;
	lptr->firstblock = lptr->lastblock = NULL;
	lptr->width = 0;
}

MCBlock *MCLine::fitblocks(MCBlock* p_first, MCBlock* p_sentinal, coord_t p_max_width)
{
	MCBlock *t_block;
	t_block = p_first;
	
	coord_t t_frontier_width;
	t_frontier_width = 0;

	MCBlock *t_break_block;
	t_break_block = NULL;

	uint2 t_break_index;
	t_break_index = 0;

	do
	{
		t_block -> reset();

		// MW-2008-07-08: [[ Bug 6475 ]] Breaking works incorrectly on lines with tabs and multiple blocks.
		//   This is due to the tab computations within the block being incorrect as t_frontier_width was
		//   not being passed (width == 0 was being passed instead).
		uint2 t_new_break_index;
		bool t_break_fits, t_block_fits;
		t_block_fits = t_block -> fit(t_frontier_width, p_max_width < t_frontier_width ? 0 : p_max_width - t_frontier_width, t_new_break_index, t_break_fits);

		bool t_continue;
		if (t_block_fits)
		{
			if (t_new_break_index > t_block -> getindex() || (t_block -> getsize() == 0 && t_break_fits))
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
			if (t_new_break_index > t_block -> getindex())
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
		t_break_index = t_break_block -> getindex() + t_break_block -> getsize();
	}
	
	// MW-2014-01-06: [[ Bug 11628 ]] We have a break block and index, so now extend the
	//   break index through any space chars.
	const char *t_text;
	t_text = parent -> gettext();
	for(;;)
	{
		// Consume all spaces after the break index.
		while(t_break_index < (t_break_block -> getindex() + t_break_block -> getsize()) &&
			  t_break_block -> textisspace(&t_text[t_break_index]))
        {
            // SN-2015-01-21: [[ Bug 14421 ]] If the break block is a unicode block,
            //  then the next index is 2 'char' further, not one. This can lead to a
            //  size of 1 for a unicode block, and a hang in FontBreaking.
            if (t_break_block -> getflag(F_HAS_UNICODE))
                t_break_index += 2;
            else
                t_break_index++;
        }
		
		if (t_break_index < (t_break_block -> getindex() + t_break_block -> getsize()))
			break;

        // Get the next non empty block.
        MCBlock *t_next_block;
        t_next_block = t_break_block -> next();
        while(t_next_block -> getsize() == 0 && t_next_block != p_sentinal)
            t_next_block = t_next_block -> next();

		// If we are at the end of the list of blocks there is nothing more to do.
		if (t_next_block == p_sentinal)
			break;
		
		// If the first char of the next block is not a space, then there is nothing more
		// to do.
		if (!t_next_block -> textisspace(&t_text[t_next_block -> getindex()]))
			break;
		
		// The next block starts with a space, so advance the break block.
		t_break_block = t_next_block;
		t_break_index = t_break_block -> getindex();
	}
	
	// MW-2012-02-21: [[ LineBreak ]] Check to see if there is a vtab char before the
	//   break index.
	bool t_is_explicit_line_break;
	t_is_explicit_line_break = false;
	t_block = p_first;
	do
	{
		uint2 t_line_break_index;
		if (t_block -> getfirstlinebreak(t_line_break_index))
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
	if (t_break_index < t_break_block -> getindex() + t_break_block -> getsize() ||
		t_is_explicit_line_break && t_break_index == t_break_block -> getindex() + t_break_block -> getsize())
		t_break_block -> split(t_break_index);
		
	firstblock = p_first;
	lastblock = t_break_block;
	
	t_block = firstblock;
	do
	{
		setscents(t_block);
		width += t_block -> getwidth(NULL, width);
		t_block = t_block -> next();
	}
	while(t_block != lastblock -> next());
	
	dirtywidth = width;
	
	return lastblock -> next();
}

void MCLine::appendall(MCBlock *bptr)
{
	firstblock = bptr;
	lastblock = (MCBlock *)bptr->prev();
	coord_t oldwidth = width;
	width = 0;
	bptr = lastblock;
	ascent = descent = 0;
	do
	{
		bptr = (MCBlock *)bptr->next();
		setscents(bptr);
		width += bptr->getwidth(NULL, width);
	}
	while (bptr != lastblock);
	dirtywidth = MCU_max(width, oldwidth);
}

void MCLine::draw(MCDC *dc, int2 p_x, int2 y, uint2 si, uint2 ei, const char *tptr, uint2 pstyle)
{
	coord_t x = p_x;
    coord_t cx = 0;
	MCBlock *bptr = (MCBlock *)firstblock->prev();
	
	uint32_t t_flags;
	t_flags = 0;
	
	bool t_is_flagged;
	t_is_flagged = false;
	coord_t t_flagged_sx, t_flagged_ex;
	t_flagged_sx = 0;
	t_flagged_ex = 0;
	do
	{
		bptr = (MCBlock *)bptr->next();
		
		// MW-2012-02-27: [[ Bug 2939 ]] We need to compute whether to render the left/right
		//   edge of the box style. This is determined by whether the previous block has such
		//   a style or not.
		
		// Fetch the style of this block.
		uint2 t_this_style;
		if (!bptr -> gettextstyle(t_this_style))
			t_this_style = pstyle;
		
		// Start off with 0 flags.
		t_flags = 0;
		
		// If this block has a box style then we have something to do.
		if ((t_this_style & (FA_BOX | FA_3D_BOX)) != 0)
		{
			// If we are not the first block, check the text style of the previous one.
			// Otherwise we must render the left edge.
			if (bptr != firstblock)
			{
				uint2 t_prev_style;
				if (!bptr -> prev() -> gettextstyle(t_prev_style))
					t_prev_style = pstyle;
				if ((t_this_style & FA_BOX) != 0 && (t_prev_style & FA_BOX) == 0)
					t_flags |= DBF_DRAW_LEFT_EDGE;
				else if ((t_this_style & FA_3D_BOX) != 0 && (t_prev_style & FA_3D_BOX) == 0)
					t_flags |= DBF_DRAW_LEFT_EDGE;
			}
			else
				t_flags |= DBF_DRAW_LEFT_EDGE;
			
			// If we are not the last block, check the text style of the next one.
			// Otherwise we must render the right edge.
			if (bptr != lastblock)
			{
				uint2 t_next_style;
				if (!bptr -> next() -> gettextstyle(t_next_style))
					t_next_style = pstyle;
				if ((t_this_style & FA_BOX) != 0 && (t_next_style & FA_BOX) == 0)
					t_flags |= DBF_DRAW_RIGHT_EDGE;
				else if ((t_this_style & FA_3D_BOX) != 0 && (t_next_style & FA_3D_BOX) == 0)
					t_flags |= DBF_DRAW_RIGHT_EDGE;
			}
			else
				t_flags |= DBF_DRAW_RIGHT_EDGE;
		}
		
		// Pass the computed flags to the block to draw.
		bptr->draw(dc, x, cx, y, si, ei, tptr, pstyle, t_flags);
		
		coord_t twidth;
		twidth = bptr->getwidth(dc, cx);
		
		if (bptr -> getflagged())
		{
			if (!t_is_flagged)
			{
				t_is_flagged = true;
				t_flagged_sx = x;
			}
			t_flagged_ex = x + twidth;
		}
		
		if (t_is_flagged && (!bptr -> getflagged() || bptr == lastblock))
		{
			static uint1 s_dashes[2] = {3, 2};
			MCColor t_color;
			t_color . red = 255 << 8;
			t_color . green = 0 << 8;
			t_color . blue = 0 << 8;
			MCscreen -> alloccolor(t_color);
			dc -> setforeground(t_color);
			dc -> setquality(QUALITY_SMOOTH);
			dc -> setlineatts(2, LineOnOffDash, CapButt, JoinRound);
			dc -> setdashes(0, s_dashes, 2);
			dc -> drawline(t_flagged_sx, y + 1, t_flagged_ex, y + 1);
			dc -> setlineatts(0, LineSolid, CapButt, JoinBevel);
			dc -> setquality(QUALITY_DEFAULT);
			t_is_flagged = false;
			parent->getparent()->setforeground(dc, DI_FORE, False, True);
		}
		
		x += twidth;
		cx += twidth;
	}
	while (bptr != lastblock);
}

void MCLine::setscents(MCBlock *bptr)
{
	uint2 aheight = bptr->getascent();
	if (aheight > ascent)
		ascent = aheight;
	uint2 dheight = bptr->getdescent();
	if (dheight > descent)
		descent = dheight;
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

void MCLine::getindex(uint2 &i, uint2 &l)
{
	firstblock->getindex(i, l);
	uint2 j;
	lastblock->getindex(j, l);
	l = j + l - i;
}

coord_t MCLine::getcursorx(uint2 fi)
{
	coord_t x = 0;
	MCBlock *bptr = (MCBlock *)firstblock;
	uint2 i, l;
	bptr->getindex(i, l);
	while (fi > i + l && bptr != lastblock)
	{
		x += bptr->getwidth(NULL, x);
		bptr = (MCBlock *)bptr->next();
		bptr->getindex(i, l);
	}
	return x + bptr->getcursorx(x, fi);
}

uint2 MCLine::getcursorindex(coord_t cx, Boolean chunk)
{
	coord_t x = 0;
	MCBlock *bptr = firstblock;
	coord_t bwidth = bptr->getwidth(NULL, x);
	while (cx > bwidth && bptr != lastblock)
	{
		cx -= bwidth;
		x += bwidth;
		bptr = (MCBlock *)bptr->next();
		bwidth = bptr->getwidth(NULL, x);
	}
	return bptr->getcursorindex(x, cx, chunk, bptr == lastblock);
}

uint2 MCLine::getwidth()
{
    // AL-2014-10-21: [[ Bug 13403 ]] Returned line width as integer needs to be rounded up
	return ceil(width);
}

uint2 MCLine::getheight()
{
	return ascent + descent;
}

uint2 MCLine::getascent()
{
	return ascent;
}

uint2 MCLine::getdescent()
{
	return descent;
}

// MW-2012-02-10: [[ FixedTable ]] In fixed-width table mode the paragraph needs to
//   explicitly set the width of the table.
void MCLine::setwidth(uint2 p_new_width)
{
	width = p_new_width;
}
