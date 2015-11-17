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

#include <foundation-math.h>

#include "text-run.h"

#include "text-paragraph.h"
#include "text-line.h"
#include "text-segment.h"
#include "font.h"
#include "context.h"


MCTextRun::MCTextRun(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range) :
  MCTextBlock(p_paragraph, p_codeunit_range),
  m_font(NULL)
{
    // TESTING
    MCFontCreate(MCNAME("Times New Roman"), 0, 48, m_font);
}

MCTextRun::MCTextRun(const MCTextRun& p_copy, const MCRange& p_codeunit_range) :
  MCTextBlock(p_copy, p_codeunit_range),
  m_font(NULL)
{
    // TESTING
    MCFontCreate(MCNAME("Times New Roman"), 0, 48, m_font);
}

MCTextRun::~MCTextRun()
{
    ;
}

void MCTextRun::performLayout()
{
    // The height of the block is the sum of the font height parameters and the
    // width is simply the width of all of the text
    adjustCodeunitRange(0, 0);
    recordSize(measure(), getTextAscent()+getTextDescent()+getTextLeading());
    setMaxSize(getWidth(), getHeight());
    finishLayout();
}

void MCTextRun::clearLayout()
{
    // Nothing needs to be done
}

void MCTextRun::draw(MCDC* dc)
{
    // Calculate the base line for drawing the text
    MCTextLine* t_line;
    coord_t t_y;
    t_line = getSegment()->getLine();
    t_y = t_line->getHeight() - t_line->getTextDescent();
    
    dc->drawtext_substring(0, t_y, fetchStringRef(), m_codeunit_range, m_font, False);
}

MCTextBlockType MCTextRun::getBlockType() const
{
    return kMCTextBlockTypeRun;
}

coord_t MCTextRun::getCursorPositionBefore(uindex_t p_grapheme_offset)
{
    MCRange t_range;
    t_range = MCRangeMake(m_codeunit_range.offset, graphemeToCodeunit(p_grapheme_offset));
    return MCFontMeasureTextSubstringFloat(m_font, fetchStringRef(), t_range, getTransform());
}

coord_t MCTextRun::getCursorPositionAfter(uindex_t p_grapheme_offset)
{
    MCRange t_range;
    t_range = MCRangeMake(m_codeunit_range.offset, graphemeToCodeunit(p_grapheme_offset+1));
    return MCFontMeasureTextSubstringFloat(m_font, fetchStringRef(), t_range, getTransform());
}

bool MCTextRun::mergeWithNext()
{
    // Merge if the block is identical in terms of attributes
    // TODO: proper attributes
    if (next() == getParagraph()->getBlocks()
        || next()->getBlockType() != kMCTextBlockTypeRun)
        return false;
    
    m_codeunit_range.length += next()->getCodeunitRange().length;
    m_grapheme_range.length += next()->getGraphemeRange().length;
    MCTextBlock* t_dummy = this;
    delete next()->remove(t_dummy);
    return true;
}

MCTextRun* MCTextRun::splitAfter(uindex_t p_codeunit_offset)
{
    MCRange t_new_range;
    MCTextRun* t_new_run;
    t_new_range = MCRangeMake(m_codeunit_range.offset+p_codeunit_offset, m_codeunit_range.length-p_codeunit_offset);
    t_new_run = new MCTextRun(m_paragraph, t_new_range);
    adjustCodeunitRange(0, -(m_codeunit_range.length - p_codeunit_offset));
    t_new_run->adjustCodeunitRange(0, 0);
    append(t_new_run);
    return t_new_run;
}

coord_t MCTextRun::getTextAscent() const
{
    // Shouldn't happen...
    if (m_font == NULL)
        return NAN;
    
    return MCFontGetAscent(m_font);
}

coord_t MCTextRun::getTextDescent() const
{
    // Shouldn't happen...
    if (m_font == NULL)
        return NAN;
    
    return MCFontGetDescent(m_font);
}

coord_t MCTextRun::getTextLeading() const
{
    // Shouldn't happen...
    if (m_font == NULL)
        return NAN;
    
    return MCFontGetLeading(m_font);
}
