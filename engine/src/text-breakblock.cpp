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

#include "text-breakblock.h"

#include "text-paragraph.h"


MCTextBreakBlock::MCTextBreakBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunits, MCTextBlockBreakClass p_class) :
  MCTextBlock(p_paragraph, p_codeunits),
  m_break_class(p_class)
{
    ;
}

void MCTextBreakBlock::performLayout()
{
    // No layout required
}

void MCTextBreakBlock::clearLayout()
{
    // No layout required
}

void MCTextBreakBlock::draw(MCDC* dc)
{
    // Non-drawing block
}

MCTextBlockType MCTextBreakBlock::getBlockType() const
{
    return kMCTextBlockTypeBreak;
}

coord_t MCTextBreakBlock::getCursorPositionBefore(uindex_t p_grapheme_offset)
{
    // Zero-width block
    return 0.0f;
}

coord_t MCTextBreakBlock::getCursorPositionAfter(uindex_t p_grapheme_offset)
{
    // Zero-width block
    return 0.0f;
}
