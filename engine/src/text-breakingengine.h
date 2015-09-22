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

#ifndef TEXT_BREAKINGENGINE_H
#define TEXT_BREAKINGENGINE_H


#include "foundation.h"


// Forward declarations
class MCTextBlock;


// Interface used for text breaking engines
class MCTextBreakingEngine
{
public:
    
    // Destructor.
    virtual ~MCTextBreakingEngine();
    
    // Given a list of text blocks and an available width, accepts as many
    // blocks as possible as will fit into the width. If only part of a block
    // will fit, it may be split so that the initial part will fit. A pointer to
    // the last block that would fit is returned along with a boolean to
    // indicate whether fitting should continue.
    virtual bool fitBlocks(MCTextBlock* p_blocks, coord_t p_available_width, bool p_start_of_line, MCTextBlock*& r_last_fit) = 0;
};


#endif  // ifndef TEXT_BREAKINGENGINE_H
