/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "platform-internal.h"

extern MCPlatformCursorRef MCMacPlatformCreateCursor(void);
extern void MCMacPlatformHideCursorUntilMouseMoves(void);

////////////////////////////////////////////////////////////////////////////////

MCPlatformCursor::MCPlatformCursor(void)
: m_references(1)
{
}

MCPlatformCursor::~MCPlatformCursor(void)
{
}

void MCPlatformCursor::Retain(void)
{
    m_references += 1;
}

void MCPlatformCursor::Release(void)
{
    m_references -= 1;
    if (m_references == 0)
    {
        delete this;
    }
}

void MCPlatformCursor::CreateStandard(MCPlatformStandardCursor p_standard_cursor)
{
    is_standard = true;
    standard = p_standard_cursor;
}

void MCPlatformCreateStandardCursor(MCPlatformStandardCursor p_standard_cursor, MCPlatformCursorRef& r_cursor)
{
    MCPlatformCursorRef t_cursor;
    t_cursor = MCMacPlatformCreateCursor();
    
    t_cursor -> CreateStandard(p_standard_cursor);
    
    r_cursor = t_cursor;
}

void MCPlatformCreateCustomCursor(MCImageBitmap *p_image, MCPoint p_hotspot, MCPlatformCursorRef& r_cursor)
{
    MCPlatformCursorRef t_cursor;
    t_cursor = MCMacPlatformCreateCursor();
    
    t_cursor -> CreateCustom(p_image, p_hotspot);
    
    r_cursor = t_cursor;
}

void MCPlatformRetainCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Retain();
}

void MCPlatformReleaseCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Release();
}

void MCPlatformSetCursor(MCPlatformCursorRef p_cursor)
{
    p_cursor -> Set();
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHideCursorUntilMouseMoves(void)
{
    MCMacPlatformHideCursorUntilMouseMoves();
}
