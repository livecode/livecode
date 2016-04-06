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

#ifndef TEXT_LINE_H
#define TEXT_LINE_H


#include "foundation.h"
#include "text-cell.h"


// Forward declarations
class MCTextParagraph;
class MCTextSegment;
class MCTextBlock;


class MCTextLine :
  public MCTextCell
{
public:
    
    MCDLlistAdaptorFunctions(MCTextLine);
    
    // Constructor.
    MCTextLine(MCTextParagraph* p_paragraph);
    
    // Inherited from MCTextCell
    virtual MCTextCellType getType() const;
    virtual MCTextCell* getParent() const;
    virtual MCTextCell* getChildren() const;
    virtual void performLayout();
    virtual void clearLayout();
    virtual void draw(MCDC* dc);
    virtual void repositionChildren();
    
    // Line accessors
    MCTextParagraph* getParagraph() const           { return m_parent; }
    MCTextSegment* getSegments() const              { return m_segments; }
    coord_t getTextAscent() const                   { return m_ascent; }
    coord_t getTextDescent() const                  { return m_descent; }
    coord_t getTextLeading() const                  { return m_leading; }
    
    // Adds as many segments as possible to the line, depending on the space
    // available for layout. The first form will split text runs to keep
    // within the specified width (i.e it performs wrapping) while the second
    // form will only split at explicit line breaks. The first segment that
    // was not added to the line is returned (or NULL if none remain)
    MCTextSegment* addSegmentsWrapped(MCTextSegment* p_segments, coord_t p_max_width);
    MCTextSegment* addSegmentsNonwrapped(MCTextSegment* p_segments);
    
    // Removes all segments from the line and returns them as a list of blocks
    MCTextBlock* clearAllSegments();
    
    // Returns true if the line ends with an explicit line break (this is used
    // by the paragraph to show an empty line afterwards).
    bool endsWithExplicitLineBreak() const;
    
private:
    
    // The group to which this line belongs
    MCTextParagraph* m_parent;
    
    // The segments which comprise this line
    MCTextSegment* m_segments;
    
    // Maximal text metrics for this line
    coord_t m_ascent;
    coord_t m_descent;
    coord_t m_leading;
    
    
    // Determines if the given segment can be added given the width available.
    // If not, the segment is split so that it will fit. False will be returned
    // if there was a failure to fit the entire segment or if an explicit line
    // break was encountered.
    bool fitSegment(MCTextSegment* p_segment, bool p_first_segment, coord_t p_available_width);
    
    // Calculates the x offset for where the given segment should be placed
    coord_t offsetForSegment(uindex_t t_index, coord_t p_prev_segment_end);
};


#endif  // ifndef TEXT_LINE_H
