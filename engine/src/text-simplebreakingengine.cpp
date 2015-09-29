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

#include "text-simplebreakingengine.h"

#include "foundation-locale.h"
#include "foundation-unicode.h"
#include "text-block.h"
#include "text-breakblock.h"
#include "text-run.h"


// Non-breaking space characters
enum
{
    kMCCodepointNBSP        = 0x00A0,       // Non-breaking space
    kMCCodepointFigureSpace = 0x2007,       // Digit-sized space
    kMCCodepointNNBSP       = 0x202F        // Narrow non-breaking space
};


MCTextSimpleBreakingEngine::~MCTextSimpleBreakingEngine()
{
    ;
}

// Yes, this algorithm is about as "simple" as a text breaking algorithm can get
// without producing some very strange-looking results... "complex" would be a
// proper hyphenation engine.
bool MCTextSimpleBreakingEngine::fitBlocks(MCTextBlock* p_blocks, coord_t p_available_width, bool p_start_of_line, MCTextBlock*& r_last_fit)
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
        // Is this a line-breaking block?
        if (t_block->getBlockType() == kMCTextBlockTypeBreak)
        {
            MCTextBreakBlock* t_break_block;
            t_break_block = static_cast<MCTextBreakBlock*>(t_block);
            if (t_break_block->getBreakClass() >= kMCTextBlockBreakClassLine)
            {
                // Accept this block and no more
                t_block = t_block->next();
                t_break = true;
                break;
            }
        }
        
        // Measure the whole block first to avoid the iterator if not required
        coord_t t_block_width;
        t_block_width = t_block->measure();
        if (t_block_width <= t_remaining_width)
        {
            // Accept all of this block
            p_start_of_line = false;
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
        MCRange t_block_range;
        uindex_t t_offset;
        t_block_range = t_block->getCodeunitRange();
        t_offset = t_block_range.offset;
        if (t_block_range.offset != 0)
            MCLocaleBreakIteratorAfter(t_iter, t_offset-1);
        
        // See how many words we can fit
        uindex_t t_break_pos, t_last_break_pos;
        t_break_pos = t_last_break_pos = t_offset;
        while ((t_break_pos = MCLocaleBreakIteratorAdvance(t_iter)) != kMCLocaleBreakIteratorDone)
        {
            // Don't go past the end of the block
            if (t_break_pos > t_block_range.offset + t_block_range.length)
                break;
            
            // Advance past any non-breaking spaces and punctuation characters.
            // All of the interesting codepoints are represented using a single
            // codeunit so use them for efficiency.
            codepoint_t t_codepoint;
            t_codepoint = MCStringGetCharAtIndex(p_blocks->fetchStringRef(), t_break_pos);
            
            while (t_break_pos < t_block_range.offset + t_block_range.length
                   && (t_codepoint == kMCCodepointNBSP || t_codepoint == kMCCodepointNNBSP
                       || t_codepoint == kMCCodepointFigureSpace
                       || MCUnicodeIsPunctuation(t_codepoint)))
            {
                t_codepoint = MCStringGetCharAtIndex(p_blocks->fetchStringRef(), ++t_break_pos);
            }
            
            // Can we fit this sequence of words?
            // We always accept at least one word to avoid the degenerate case
            // of having a word that is longer than a whole line causing an
            // infinite loop.
            coord_t t_subwidth;
            t_subwidth = t_block->getSubWidth(0, t_block->codeunitToGrapheme(t_break_pos - t_offset));
            if (t_subwidth > t_remaining_width && !p_start_of_line)
            {
                t_break_pos = t_last_break_pos;
                
                // We have found the limit for what will fit. Accept any
                // trailing whitespace as it can be compressed to zero length
                // at the end of a line.
                while (t_break_pos < t_block_range.offset + t_block_range.length
                       && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(p_blocks->fetchStringRef(), t_break_pos)))
                {
                    t_break_pos++;
                }
                
                break;
            }
            
            p_start_of_line = false;
            t_last_break_pos = t_break_pos;
        }
        
        // The whole block didn't fit so this shouldn't happen...
        MCAssert(t_break_pos != kMCLocaleBreakIteratorDone);
        
        // Did any of the words fit?
        if (t_break_pos != t_offset)
        {
            // Check whether we need to split in the middle of a block (this
            // might not be the case if we ignored trailing whitespace in the
            // block).
            if (t_break_pos < t_block_range.offset + t_block_range.length)
            {
                // Block will have to be split. Create a new one.
                t_block->splitAfter(t_break_pos - t_offset);
            }
            
            // Increment the pointer to point to the block that doesn't fit
            t_block = t_block->next();
            
            // Done
            break;
        }
        
        // If we accepted at least one block, there is nothing more to do
        if (t_block != p_blocks)
            break;
        
        // We should have accepted at least one block in all circumstances
        MCUnreachable();
    }
    while (t_block != p_blocks);
    
    MCLocaleBreakIteratorRelease(t_iter);
    
    // t_block will be equal to p_blocks if all blocks were accepted
    r_last_fit = t_block->prev();
    return !t_break;
}
