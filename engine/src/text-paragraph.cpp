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

#include "text-paragraph.h"

#include <foundation-math.h>

#include "foundation-locale.h"
#include "foundation-unicode.h"
#include "text-line.h"
#include "text-segment.h"
#include "text-block.h"
#include "text-breakblock.h"
#include "text-controlblock.h"
#include "text-simplebreakingengine.h"
#include "text-run.h"
#include "context.h"


// Some codepoints that are interesting to the paragraph
enum
{
    // Segment breaking characters
    kMCCodepointHT      = 0x0009,       // Horizontal Tab
    
    // Line and/or paragraph breaking characters
    kMCCodepointLF      = 0x000A,       // Line Feed
    kMCCodepointVT      = 0x000B,       // Vertical Tab
    kMCCodepointFF      = 0x000C,       // Form Feed
    kMCCodepointCR      = 0x000D,       // Carriage return
    kMCCodepointNEL     = 0x0085,       // Next Line
    kMCCodepointLS      = 0x2028,       // Line Separator
    kMCCodepointPS      = 0x2029,       // Paragraph Separator
    
    // Bi-Directional control characters
    kMCCodepointLRM     = 0x200E,       // Left-to-Right Mark
    kMCCodepointRLM     = 0x200F,       // Right-to-Left Mark
    kMCCodepointALM     = 0x061C,       // Arabic Letter Mark
    kMCCodepointLRE     = 0x202A,       // Left-to-Right Embedding
    kMCCodepointLRO     = 0x202D,       // Left-to-Right Override
    kMCCodepointRLE     = 0x202B,       // Right-to-Left Embedding
    kMCCodepointRLO     = 0x202E,       // Right-to-Left Override
    kMCCodepointPDF     = 0x202C,       // Pop Directional Formatting
    kMCCodepointLRI     = 0x2066,       // Left-to-Right Isolate
    kMCCodepointRLI     = 0x2067,       // Right-to-Left Isolate
    kMCCodepointFSI     = 0x2068,       // First Strong Isolate
    kMCCodepointPDI     = 0x2069,       // Pop Directional Isolate
};


MCTextParagraph::MCTextParagraph(MCTextCell* p_parent) :
  MCTextCellContainable(p_parent),
  m_lines(NULL),
  m_blocks(NULL),
  m_text(NULL),
  m_grapheme_to_unit(),
  m_unit_to_grapheme(),
  m_breaking_engine(NULL)
{
    ;
}

MCTextParagraph::~MCTextParagraph()
{
    deleteContents();
}

MCTextCellType MCTextParagraph::getType() const
{
    return kMCTextCellTypeParagraph;
}

MCTextCell* MCTextParagraph::getChildren() const
{
    // Ensure the entire hierarchy is returned regardless of layout state
    if (m_lines != NULL)
        return m_lines;
    else
        return m_blocks;
}

void MCTextParagraph::performLayout()
{
    if (!needsLayout())
        return;
    
    // Clear any existing layout
    clearLayout();
    
    // Split the paragraph into lines
    layoutLines(createSegments());
    
    // Calculate the size of this paragraph
    MCTextLine* t_line;
    coord_t t_total_height;
    coord_t t_max_width;
    t_total_height = t_max_width = 0.0f;
    t_line = m_lines;
    do
    {
        t_total_height += t_line->getHeight();
        t_max_width = MCMax(t_max_width, t_line->getWidth());
        t_line = t_line->next();
    }
    while (t_line != m_lines);
    
    if (isinf(getMaxWidth()))
        recordSize(t_max_width, t_total_height);
    else
        recordSize(getMaxWidth(), t_total_height);
    
    // Ensure all children are positioned correctly
    finishLayout();
    repositionChildren();
}

void MCTextParagraph::clearLayout()
{
    // Remove each of the lines and take their blocks back
    while (m_lines != NULL)
    {
        MCTextLine* t_line;
        MCTextBlock* t_blocks;
        t_line = m_lines->remove(m_lines);
        t_blocks = t_line->clearAllSegments();
        t_blocks->appendto(m_blocks);
        delete t_line;
    }
}

void MCTextParagraph::draw(MCDC* dc)
{
    // Ensure layout has been completed
    performLayout();
    
    // Draw each of the lines in turn
    MCTextLine* t_line;
    t_line = m_lines;
    do
    {
        dc->save();
        dc->setorigin(t_line->getX(), t_line->getY());
        t_line->draw(dc);
        dc->restore();
        
        t_line = t_line->next();
    }
    while (t_line != m_lines);
}

void MCTextParagraph::repositionChildren()
{
    // Do nothing if a full layout is needed anyway
    if (needsLayout())
        return;
    
    // Loop through the lines and update their positions
    MCTextLine* t_line;
    t_line = m_lines;
    do
    {
        coord_t t_x, t_y;
        t_x = getLineX(t_line);
        t_y = getLineY(t_line);
        t_line->setPosition(t_x, t_y);
        t_line->repositionChildren();
        t_line = t_line->next();
    }
    while (t_line != m_lines);
}

MCTextBreakingEngine* MCTextParagraph::getBreakingEngine()
{
    if (m_breaking_engine == NULL)
        m_breaking_engine = new MCTextSimpleBreakingEngine;
    
    return m_breaking_engine;
}

void MCTextParagraph::setContentsPlain(MCStringRef p_string)
{
    // Clear any existing contents
    deleteContents();
    
    // Set the text then create a single block to cover all of it
    m_text = MCValueRetain(p_string);
    m_blocks = new MCTextRun(this, MCRangeMake(0, MCStringGetLength(p_string)));
    
    // Perform a block refresh to ensure that any breaks are dealt with
    refreshBlocks();
    
    // A re-layout will be needed
    setNeedsLayout();
}

uindex_t MCTextParagraph::graphemeToCodeunit(uindex_t p_grapheme) const
{
    return m_grapheme_to_unit[p_grapheme];
}

uindex_t MCTextParagraph::codeunitToGrapheme(uindex_t p_codeunit) const
{
    return m_unit_to_grapheme[p_codeunit];
}

MCTextDirection MCTextParagraph::getConcreteTextDirection()
{
    if (getTextDirection() == kMCTextDirectionAuto)
        return MCTextDirection(MCBidiFirstStrongIsolate(m_text, 0));
    else
        return getTextDirection();
}

MCTextSegment* MCTextParagraph::createSegments()
{
    // Early return if there are no blocks to operate on (because the paragraph
    // is empty or because it is already laid out)
    if (m_blocks == NULL)
        return NULL;
    
    // Make sure that the blocks list is properly set up
    refreshBlocks();
    
    // Create the initial segment
    MCTextSegment* t_first_segment;
    MCTextSegment* t_last_segment;
    t_first_segment = t_last_segment = new MCTextSegment(this);
    
    // Scan the block list for segment-breaking blocks
    MCTextBlock* t_remaining_blocks;
    t_remaining_blocks = m_blocks;
    while ((t_remaining_blocks = t_last_segment->addBlocks(t_remaining_blocks)) != NULL)
    {
        // Segment break found - create a new segment
        MCTextSegment* t_new_segment;
        t_new_segment = new MCTextSegment(this);
        t_last_segment->append(t_new_segment);
        t_last_segment = t_new_segment;
    }
    
    // All the blocks have been added to segments
    m_blocks = NULL;
    return t_first_segment;
}

void MCTextParagraph::layoutLines(MCTextSegment* p_segments)
{
    // Create the first line
    MCTextLine* t_line;
    m_lines = t_line = new MCTextLine(this);
    
    // Fit the segments into the line
    MCTextSegment* t_segments;
    uindex_t t_line_count;
    t_segments = p_segments;
    t_line_count = 0;
    while (t_segments != NULL)
    {
        // Add as many segments as possible to the line
        t_segments = t_line->addSegmentsWrapped(t_segments, getContentWidthForLine(t_line_count));
        
        // If there are still segments to add or the line ends with an explicit
        // line break, create a new line
        if (t_segments != NULL || t_line->endsWithExplicitLineBreak())
        {
            MCTextLine* t_new_line;
            t_new_line = new MCTextLine(this);
            t_line->append(t_new_line);
            t_line = t_line->next();
            t_line_count++;
        }
    }
    
    // Go back through the list of lines and instruct them to finish layout
    t_line = m_lines;
    do
    {
        t_line->performLayout();
        t_line = t_line->next();
    }
    while (t_line != m_lines);
}

void MCTextParagraph::refreshBlocks()
{
    // Do nothing if there are no blocks (or we are laid out)
    if (m_blocks == NULL)
        return;
    
    // Clear the existing mapping arrays
    m_grapheme_to_unit.Delete();
    m_grapheme_to_unit.Push(0);
    m_unit_to_grapheme.Resize(MCStringGetLength(m_text) + 1);
    
    // Create a break iterator for generating the grapheme <-> codeunit mappings
    MCBreakIteratorRef t_break_iter;
    uindex_t t_boundary, t_last_boundary, t_grapheme_count;
    t_last_boundary = 0;
    t_grapheme_count = 0;
    /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeCharacter, t_break_iter);
    /* UNCHECKED */ MCLocaleBreakIteratorSetText(t_break_iter, m_text);
    while ((t_boundary = MCLocaleBreakIteratorAdvance(t_break_iter)) != kMCLocaleBreakIteratorDone)
    {
        // Set the grapheme index for the covered code units
        for (uindex_t i = t_last_boundary; i < t_boundary; i++)
            m_unit_to_grapheme[i] = t_grapheme_count;
        m_grapheme_to_unit.Push(t_boundary);
        t_grapheme_count++;
        t_last_boundary = t_boundary;
    }

    m_unit_to_grapheme[MCStringGetLength(m_text)] = t_grapheme_count;
    m_grapheme_to_unit.Push(MCStringGetLength(m_text));
    
    MCLocaleBreakIteratorRelease(t_break_iter);
    
    // Merge all blocks together that we can
    MCTextBlock* t_block;
    t_block = m_blocks;
    do
    {
        // Merge this block with as many trailing blocks as possible
        while (t_block->mergeWithNext())
            ;
        
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
    
    // Scan through the existing blocks. We use codeunits for the text scan
    // because all the control characters of interest are encoded in a single
    // unit and we can therefore avoid the codepoint -> codeunit mapping costs.
    t_block = m_blocks;
    do
    {
        MCRange t_block_range;
        t_block_range = t_block->getCodeunitRange();
        
        // Only scan basic runs - control blocks don't need re-scanning
        if (t_block->getBlockType() == kMCTextBlockTypeRun)
        {
            for (uindex_t i = t_block_range.offset; i < t_block_range.offset+t_block_range.length; i++)
            {
                codepoint_t t_codepoint;
                t_codepoint = MCStringGetCharAtIndex(m_text, i);
                
                bool t_break, t_control;
                MCTextBlockBreakClass t_break_class;
                t_break = false;
                t_control = false;
                
                switch (t_codepoint)
                {
                    case kMCCodepointHT:
                        // Segment break
                        t_break = true;
                        t_break_class = kMCTextBlockBreakClassSegment;
                        break;
                        
                    case kMCCodepointVT:
                    case kMCCodepointLS:
                        // Line break
                        t_break = true;
                        t_break_class = kMCTextBlockBreakClassLine;
                        break;
                        
                    case kMCCodepointCR:
                    case kMCCodepointLF:
                    case kMCCodepointPS:
                        // Paragraph break
                        t_break = true;
                        t_break_class = kMCTextBlockBreakClassParagraph;
                        break;
                        
                    case kMCCodepointFF:
                    case kMCCodepointNEL:
                        // What to do with these?
                        break;
                        
                    default:
                        // Just an ordinary character
                        break;
                }
                
                // Continue looping if the character isn't special
                if (!t_break && !t_control)
                    continue;
                
                // The range that we will be removing
                MCRange t_range;
                t_range = MCRangeMake(i, 1);
                
                // The new block that will be created
                MCTextBlock* t_new_block;
                
                // Handle explicit break characters
                if (t_break)
                {
                    // If we have a CR, check for a following LF
                    if (t_codepoint == kMCCodepointCR
                        && (i + 1) < t_block_range.offset + t_block_range.length
                        && MCStringGetCharAtIndex(m_text, i + 1) == kMCCodepointLF)
                    {
                        t_range.length++;
                    }
                    
                    // Create a new block to represent the break
                    t_new_block = new MCTextBreakBlock(this, t_range, t_break_class);
                }
                
                // Handle control characters
                if (t_control)
                {
                    // Create a new block covering this control character
                    t_new_block = new MCTextControlBlock(this, t_range);
                }
                
                // Where in the run did the special character appear?
                if (t_range.offset == t_block_range.offset)
                {
                    // Beginning of the block
                    // Check for the case of consuming the whole block
                    if (t_range.length == t_block_range.length)
                    {
                        // Whole block
                        // Replace the existing run with the special block
                        t_block->append(t_new_block);
                        t_block->remove(m_blocks);
                        delete t_block;
                    }
                    else
                    {
                        // Front of the block
                        // Remove the range from the front of the existing block
                        t_block->adjustCodeunitRange(t_range.length, -t_range.length);
                        if (m_blocks == t_block)
                            t_new_block->insertto(m_blocks);
                        else
                            t_block->append(t_new_block);
                    }
                }
                else if (t_range.offset+t_range.length == t_block_range.offset+t_block_range.length)
                {
                    // End of the block
                    // Shrink the existing block and append the new one
                    t_block->adjustCodeunitRange(0, -t_range.length);
                    t_block->append(t_new_block);
                }
                else
                {
                    // In the middle of the block
                    // Split the block at the start of the special block
                    t_block->splitAfter(t_range.offset - t_block_range.offset);
                    t_block->next()->adjustCodeunitRange(t_range.length, -t_range.length);
                    t_block->append(t_new_block);
                }
                
                // No need to re-scan the newly added block
                t_block = t_new_block;
                break;
            }
        }
        
        t_block = t_block->next();
    }
    while (t_block != m_blocks);
    
    // Run the Unicode bi-directional algorithm to support mixed-direction text
    runBidiAlgorithm();
}

void MCTextParagraph::runBidiAlgorithm()
{
    // Run the Unicode BiDi algorithm to get the direction level for each
    // codeunit within this paragraph.
    MCAutoArray<uint8_t> t_levels;
    /* UNCHECKED */ MCBidiResolveTextDirection(m_text, getConcreteTextDirection(), t_levels.PtrRef(), t_levels.SizeRef());
    
    // Scan through the list of blocks and ensure that each block has a
    // consistent direction level (if not, it will need to be split)
    MCTextBlock* t_block;
    uindex_t t_index;
    t_block = m_blocks;
    t_index = 0;
    do
    {
        // Set the direction level for the block to be the level for the first
        // codeunit within the block. Any level changes will be corrected later.
        t_block->setBidiLevel(t_levels[t_index]);
        
        // Scan the block for any changes in direction level
        MCRange t_range;
        t_range = t_block->getCodeunitRange();
        for (t_index = t_range.offset; t_index < t_range.offset+t_range.length; t_index++)
        {
            if (t_levels[t_index] != t_block->getBidiLevel())
            {
                // Level has changed within the block. Split it.
                t_block->splitAfter(t_index - t_range.offset - 1);
                break;
            }
        }
        
        // Move on to the next block
        t_block = t_block->next();
    }
    while (m_blocks != t_block);
}

void MCTextParagraph::deleteContents()
{
    // Clear the layout to remove all the lines, etc
    clearLayout();
    
    // Delete all of the blocks
    while (m_blocks != NULL)
        delete m_blocks->remove(m_blocks);
    
    // Remove the grapheme <-> codeunit mapping arrays
    m_grapheme_to_unit.Delete();
    m_unit_to_grapheme.Delete();
    
    // The string is not needed anymore
    MCValueRelease(m_text);
    
    // And neither is the breaking engine
    delete m_breaking_engine;
}

coord_t MCTextParagraph::getContentWidthForLine(uindex_t p_which_line) const
{
    // TODO: margins, padding, etc
    if (p_which_line == 0)
        return getMaxWidth() - getFirstLineIndent();
    else
        return getMaxWidth();
}

coord_t MCTextParagraph::getContentHeight() const
{
    // TODO: margins, padding, etc
    return getMaxHeight();
}

coord_t MCTextParagraph::getLineX(MCTextLine* p_line) const
{
    coord_t t_x = 0.0f;
    coord_t t_width = p_line->getWidth();
    coord_t t_available_width = getWidth();
    
    // If the line is wider than the available space (this should only happen
    // when wrapping is disabled...) then ignore the specified alignment and
    // act as if it was kMCTextCellAlignStart
    if (t_width > t_available_width)
    {
        // Still need to apply the indent for the first line
        if (p_line == m_lines)
            t_x += getFirstLineIndent();
        return t_x;
    }
    
    switch (getHorizontalAlignment())
    {
        case kMCTextCellAlignCenter:
            t_x += (t_available_width - t_width) / 2;
            break;
            
        case kMCTextCellAlignStart:
        case kMCTextCellAlignJustify:
            if (p_line == m_lines)
                t_x += getFirstLineIndent();
            break;
            
        case kMCTextCellAlignEnd:
            t_x = t_available_width - t_width;
            if (p_line == m_lines)
                t_x -= getFirstLineIndent();
            break;
            
        default:
            MCUnreachable();
    }
    
    // If the text direction is reversed, reverse the line coordinate
    if (isReversedTextOrder())
        t_x = t_available_width - p_line->getWidth() - t_x;
    
    return t_x;
}

coord_t MCTextParagraph::getLineY(MCTextLine* p_line) const
{
    // Vertical alignment within paragraphs doesn't happen so the y position
    // only depends on whether the line order is reversed or not.

    // Calculate the height of the lines before this one
    coord_t t_y = 0.0f;
    MCTextLine* t_line = m_lines;
    while (t_line != p_line)
    {
        t_y += t_line->getHeight();
        t_line = t_line->next();
    }

    if (isReversedLineOrder())
    {
        // If the line order is reversed, the calculated y position is from the
        // bottom of the paragraph rather than the top. Additionally, it points
        // to the bottom of the line rather than the top.
        t_y = getHeight() - t_y;
        t_y -= t_line->getHeight();
    }
    
    return t_y;
}

coord_t MCTextParagraph::getFirstLineIndent() const
{
    // TODO: implement
    return 0.0f;
}
