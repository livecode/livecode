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

#include "text-block.h"

#include "text-paragraph.h"
#include "text-segment.h"


MCTextBlock::MCTextBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunit_range) :
  MCTextCell(*p_paragraph),
  m_grapheme_range(MCRangeMake(0, 0)),
  m_codeunit_range(p_codeunit_range),
  m_segment(NULL),
  m_paragraph(p_paragraph)
{
    ;
}

MCTextBlock::MCTextBlock(const MCTextBlock& p_copy, const MCRange& p_codeunit_range) :
  MCTextCell(p_copy),
  m_grapheme_range(MCRangeMake(0, 0)),
  m_codeunit_range(p_codeunit_range),
  m_segment(p_copy.m_segment),
  m_paragraph(p_copy.m_paragraph)
{
    ;
}

MCTextBlock::~MCTextBlock()
{
    ;
}


MCTextCellType MCTextBlock::getType() const
{
    return kMCTextCellTypeBlock;
}

MCTextCell* MCTextBlock::getParent() const
{
    if (m_segment != NULL)
        return m_segment;
    else
        return m_paragraph;
}

MCTextCell* MCTextBlock::getChildren() const
{
    return NULL;
}

coord_t MCTextBlock::getSubWidth(uindex_t p_first_grapheme, uindex_t p_last_grapheme)
{
    return fabsf(getCursorPositionAfter(p_last_grapheme) - getCursorPositionBefore(p_first_grapheme));
}

coord_t MCTextBlock::measure()
{
    return getSubWidth(0, m_grapheme_range.length-1);
}

coord_t MCTextBlock::getTextAscent() const
{
    return 0.0f;
}

coord_t MCTextBlock::getTextDescent() const
{
    return 0.0f;
}

coord_t MCTextBlock::getTextLeading() const
{
    return 0.0f;
}

bool MCTextBlock::mergeWithNext()
{
    return false;
}

MCTextBlock* MCTextBlock::splitAfter(uindex_t p_codeunit_offset)
{
    return NULL;
}

void MCTextBlock::adjustCodeunitRange(index_t p_offset_delta, index_t p_length_delta)
{
    m_codeunit_range.offset += p_offset_delta;
    m_codeunit_range.length += p_length_delta;
    
    m_grapheme_range.offset = m_paragraph->codeunitToGrapheme(m_codeunit_range.offset);
    m_grapheme_range.length = m_paragraph->codeunitToGrapheme(m_codeunit_range.offset + m_codeunit_range.length) - m_grapheme_range.offset;
}

MCStringRef MCTextBlock::fetchStringRef()
{
    return m_paragraph->fetchText();
}

void MCTextBlock::setSegment(MCTextSegment* p_segment)
{
    m_segment = p_segment;
}

void MCTextBlock::setBidiLevel(uint8_t p_level)
{
    if (p_level != m_bidi_level)
        setNeedsLayout();
    m_bidi_level = p_level;
}

uindex_t MCTextBlock::graphemeToCodeunit(uindex_t p_grapheme) const
{
    return m_paragraph->graphemeToCodeunit(p_grapheme + m_grapheme_range.offset) - m_codeunit_range.offset;
}

uindex_t MCTextBlock::codeunitToGrapheme(uindex_t p_codeunit) const
{
    return m_paragraph->codeunitToGrapheme(p_codeunit + m_codeunit_range.offset) - m_grapheme_range.offset;
}
