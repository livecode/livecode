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

#ifndef TEXT_BLOCK_H
#define TEXT_BLOCK_H


#include "text-cell.h"


// Forward declarations
class MCTextSegment;
class MCTextParagraph;


// The different types of blocks
enum MCTextBlockType
{
    kMCTextBlockTypeRun,            // Run of text with uniform attributes
    kMCTextBlockTypeImage,          // A block containing an image
    kMCTextBlockTypeBreak,          // A segment, line or paragraph break character
    kMCTextBlockTypeControl         // Certain non-printing control characters (e.g BiDi controls)
};


class MCTextBlock :
  public MCTextCell
{
public:
    
    MCDLlistAdaptorFunctions(MCTextBlock)

    // Destructor.
    virtual ~MCTextBlock();
    
    // Inherited from MCTextCell
    virtual MCTextCellType getType() const;
    virtual MCTextCell* getParent() const;
    virtual MCTextCell* getChildren() const;
    virtual void performLayout() = 0;
    virtual void clearLayout() = 0;
    virtual void draw(MCDC* dc) = 0;
    
    // Returns the type of this block
    virtual MCTextBlockType getBlockType() const = 0;
    
    // Finds the x offset where a cursor would be placed if placed before/after
    // the specified grapheme (relative to the first grapheme of the segment).
    virtual coord_t getCursorPositionBefore(uindex_t p_grapheme_offset) = 0;
    virtual coord_t getCursorPositionAfter(uindex_t p_grapheme_offset) = 0;
    
    // Measures the width of a subrange of graphemes within the segment. This
    // call is equivalent to:
    //  getCursorPositionAfter(p_last) - getCursorPositionBefore(p_first)
    coord_t getSubWidth(uindex_t p_first_grapheme, uindex_t p_last_grapheme);
    
    // Measures the entire block and returns its width
    coord_t measure();
    
    // Returns the ascent above the baseline, descent below the baseline and
    // inter-line leading, respectively, for this block
    virtual coord_t getTextAscent() const;
    virtual coord_t getTextDescent() const;
    virtual coord_t getTextLeading() const;
    
    // Attempts to merge this segment with the following segment, returning true
    // if the merge was successful. Merges only happen between segments of the
    // same type and with identical attributes.
    virtual bool mergeWithNext();
    
    // Splits the block after the specified codeunit and returns a pointer to
    // the newly-created block (it can also be accessed via the next method).
    // Only text runs are splittable - other blocks will return NULL.
    virtual MCTextBlock* splitAfter(uindex_t p_codeunit_offset);
    
    // Adjusts the codeunit range of the block. The grapheme range is updated
    // automatically to account for the new range.
    void adjustCodeunitRange(index_t p_offset_delta, index_t p_length_delta);
    
    // Fetches the StringRef associated with this block. It must not be modified!
    MCStringRef fetchStringRef();
    
    // Maps between codeunit <-> grapheme
    uindex_t graphemeToCodeunit(uindex_t) const;
    uindex_t codeunitToGrapheme(uindex_t) const;
    
    // Sets the segment that this block belongs to
    void setSegment(MCTextSegment* p_parent);
    
    // Segment accessors
    const MCRange& getGraphemeRange() const     { return m_grapheme_range; }
    const MCRange& getCodeunitRange() const     { return m_codeunit_range; }
    uint8_t getBidiLevel() const                { return m_bidi_level; }
    MCTextSegment* getSegment() const           { return m_segment; }
    MCTextParagraph* getParagraph() const       { return m_paragraph; }
    void setBidiLevel(uint8_t p_bidi_level);
    
protected:
    
    // Constructor.
    MCTextBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range);
    
    // Constructor. Duplicates an existing MCTextBlock
    MCTextBlock(const MCTextBlock& p_copy, const MCRange& p_codeunit_range);
    
    
    // The range of text covered by this segment, measured both in graphemes
    // and code units (the former for things like cursor navigation and the
    // latter for efficiency during drawing operations etc)
    MCRange m_grapheme_range;
    MCRange m_codeunit_range;
    
    // The segment and/or paragraph that this block belongs to
    MCTextSegment* m_segment;
    MCTextParagraph* m_paragraph;
    
    // The direction level of this block, according to the Unicode BiDi
    // algorithm - even levels are LTR and odd levels RTL.
    uint8_t m_bidi_level;
};


#endif  // ifndef TEXT_BLOCK_H
