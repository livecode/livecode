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

#ifndef TEXT_SIMPLEBREAKINGENGINE_H
#define TEXT_SIMPLEBREAKINGENGINE_H


#include "text-breakingengine.h"


// A simple breaking engine that will break between words (where possible)
class MCTextSimpleBreakingEngine :
  public MCTextBreakingEngine
{
public:
    
    // Destructor.
    virtual ~MCTextSimpleBreakingEngine();
    
    // Inherited from MCTextBreakingEngine
    virtual bool fitBlocks(MCTextBlock* p_blocks, coord_t p_available_width, bool p_start_of_line, MCTextBlock*& r_last_fit);
};


#endif  // ifndef TEXT_SIMPLEBREAKINGENGINE_H
