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

#ifndef TEXT_PARAGRAPH_H
#define TEXT_PARAGRAPH_H


#include "foundation-auto.h"
#include "text-cell.h"


// Forward declarations
class MCTextLine;
class MCTextSegment;
class MCTextBlock;
class MCTextBreakingEngine;


class MCTextParagraph :
  public MCTextCellContainable
{
public:
    
    MCDLlistAdaptorFunctions(MCTextParagraph)
    
    // Constructor and destructor.
    MCTextParagraph(MCTextCell* p_parent = NULL);
    ~MCTextParagraph();
    
    // Inherited from MCTextCell
    virtual MCTextCellType getType() const;
    virtual MCTextCell* getChildren() const;
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC *dc);
    virtual void repositionChildren();
    
    // Paragraph accessors
    MCTextLine* getLines() const                    { return m_lines; }
    MCTextBlock* getBlocks() const                  { return m_blocks; }
    MCStringRef fetchText() const                   { return m_text; }
    MCTextBreakingEngine* getBreakingEngine();
    
    // Sets the contents of the paragraph to the given un-attributed string
    void setContentsPlain(MCStringRef p_string);
    
    // Maps between codeunit and grapheme offsets for this paragraph
    uindex_t graphemeToCodeunit(uindex_t) const;
    uindex_t codeunitToGrapheme(uindex_t) const;
    
    // Calculates the concrete text direction for this paragraph (i.e either
    // LTR or RTL but not "auto").
    MCTextDirection getConcreteTextDirection();
    
private:
    
    // The lines of this paragraph
    MCTextLine* m_lines;
    
    // All the blocks belonging to the paragraph are stored here when the
    // paragraph's layout is invalid. At other times, they are stored via the
    // lines -> segments -> blocks hierarchy.
    MCTextBlock* m_blocks;
    
    // The text of this paragraph. Instead of having one string per primitive
    // block, the paragraph holds all text and the blocks just refer to
    // substrings within it. Using a finer granularity adds a great deal of
    // overhead while using something coarser makes text editing too expensive.
    MCStringRef m_text;
    
    // These arrays cache the mapping from graphemes <-> codeunits
    MCAutoArray<uindex_t> m_grapheme_to_unit;
    MCAutoArray<uindex_t> m_unit_to_grapheme;
    
    // The text breaking engine configured for this paragraph
    MCTextBreakingEngine* m_breaking_engine;
    
    
    // Turns the list of blocks of this paragraph into a list of segments, i.e
    // scans the block list for segment-breaking blocks and breaks appropriately
    // - this removes the blocks from the m_blocks holding area.
    MCTextSegment* createSegments();
    
    // Turns the given list of segments into a list of lines for this paragraph
    void layoutLines(MCTextSegment* p_segments);
    
    // Scans the m_blocks array, consolidating blocks with identical attributes
    // and adding new blocks for breaking and control characters.
    void refreshBlocks();
    
    // Runs the Unicode bi-directional algorithm on the contents of the
    // paragraph and sets the direction level of all blocks appropriately.
    void runBidiAlgorithm();
    
    // Removes all of the contents of this paragraph
    void deleteContents();
    
    // Returns the dimensions available for the content of this paragraph
    coord_t getContentWidthForLine(uindex_t p_which_line) const;
    coord_t getContentHeight() const;
    
    // Calculates the x/y offset at which to start drawing the given line
    coord_t getLineX(MCTextLine*) const;
    coord_t getLineY(MCTextLine*) const;
    
    // Returns the indent for the first line of the paragraph
    coord_t getFirstLineIndent() const;
};


#endif  // ifndef TEXT_PARAGRAPH_H
