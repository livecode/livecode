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

#include "text-controlblock.h"

#include "text-paragraph.h"


MCTextControlBlock::MCTextControlBlock(MCTextParagraph* p_paragraph, const MCRange& p_codeunits) :
  MCTextBlock(p_paragraph, p_codeunits)
{
    ;
}

void MCTextControlBlock::performLayout()
{
    // No layout required
}

void MCTextControlBlock::clearLayout()
{
    // No layout required
}

void MCTextControlBlock::draw(MCDC* dc)
{
    // Non-drawing block
}

MCTextBlockType MCTextControlBlock::getBlockType() const
{
    return kMCTextBlockTypeControl;
}

coord_t MCTextControlBlock::getCursorPositionBefore(uindex_t p_grapheme_offset)
{
    // Zero-width block
    return 0.0f;
}

coord_t MCTextControlBlock::getCursorPositionAfter(uindex_t p_grapheme_offset)
{
    // Zero-width block
    return 0.0f;
}
