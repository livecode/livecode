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


#include "text-simplebreakingengine.h"

#include "foundation-locale.h"
#include "foundation-unicode.h"
#include "text-block.h"
#include "text-run.h"


MCTextSimpleBreakingEngine::~MCTextSimpleBreakingEngine()
{
    ;
}

bool MCTextSimpleBreakingEngine::fitBlocks(MCTextBlock* p_blocks, coord_t p_available_width, MCTextBlock*& r_last_fit)
{
    // Create a word break iterator to find potential breaking boundaries
    MCBreakIteratorRef t_iter;
    MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeWord, t_iter);
    MCLocaleBreakIteratorSetText(t_iter, p_blocks->fetchStringRef());
    
    // Scan the list of blocks
    coord_t t_remaining_width;
    MCTextBlock* t_block;
    bool t_break;
    t_remaining_width = p_available_width;
    t_block = p_blocks;
    t_break = false;
    do
    {
        // Measure the whole block first to avoid the iterator if not required
        coord_t t_block_width;
        t_block_width = t_block->measure();
        if (t_block_width <= t_remaining_width)
        {
            // Accept all of this block
            t_remaining_width -= t_block_width;
            t_block = t_block->next();
            continue;
        }
        
        // At this point, we know fitting can't continue beyond this block
        t_break = true;
        
        // If this isn't a text run, we can't break it
        if (t_block->getBlockType() != kMCTextBlockTypeRun)
        {
            break;
        }
        
        // Point the break iterator at the start of this block
        uindex_t t_offset;
        t_offset = p_blocks->getCodeunitRange().offset;
        if (p_blocks->getCodeunitRange().offset != 0)
            MCLocaleBreakIteratorAfter(t_iter, t_offset-1);
        
        // See how many words we can fit
        uindex_t t_break_pos, t_last_break_pos;
        t_break_pos = t_last_break_pos = t_offset;
        while ((t_break_pos = MCLocaleBreakIteratorAdvance(t_iter)) != kMCLocaleBreakIteratorDone)
        {
            // Can we fit this sequence of words?
            coord_t t_subwidth;
            t_subwidth = t_block->getSubWidth(0, t_block->codeunitToGrapheme(t_break_pos - t_offset));
            if (t_subwidth > t_remaining_width)
            {
                t_break_pos = t_last_break_pos;
                break;
            }
            
            t_last_break_pos = t_break_pos;
        }
        
        // The whole block didn't fit so this shouldn't happen...
        MCAssert(t_break_pos != kMCLocaleBreakIteratorDone);
        
        // Did any of the words fit?
        if (t_break_pos != t_offset)
        {
            // Block will have to be split. Create a new one.
            MCTextRun* t_new_run;
            MCRange t_new_range;
            t_new_range = MCRangeMake(t_break_pos, t_offset + t_block->getCodeunitRange().length - t_break_pos);
            t_new_run = new MCTextRun(*static_cast<MCTextRun*>(t_block), t_new_range);
            t_block->adjustCodeunitRange(0, -(t_break_pos - t_offset));
            t_block->append(t_new_run);
            
            // Increment the pointer to point to the block that doesn't fit
            t_block = t_block->next();
            
            // Done
            break;
        }
        
        // If we accepted at least one block, there is nothing more to do
        if (t_block != p_blocks)
            break;
        
        // We are still on the first block and the first word does not fit...
        // TODO: handle this!
        MCAssert(false);
    }
    while (t_block != p_blocks);
    
    MCLocaleBreakIteratorRelease(t_iter);
    
    // t_block will be equal to p_blocks if all blocks were accepted
    r_last_fit = t_block->prev();
    return !t_break;
}
