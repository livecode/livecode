/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#ifndef __STACK_TILE__
#define __STACK_TILE__

#include "graphics.h"

bool MCStackTileInitialize();
void MCStackTileFinalize();

class MCStack;
class MCStackSurface;

class MCPlatformStackTile
{
public:
    MCPlatformStackTile(MCStack *stack, MCStackSurface *surface, const MCGIntegerRectangle &region);
    ~MCPlatformStackTile();
    
    bool Lock(void);
	void Unlock(void);
    void Render(void);
    
private:
    MCStack             *m_stack;
    MCStackSurface      *m_surface;
    MCGIntegerRectangle m_region;
    MCGContextRef       m_context;
    MCGRaster           m_raster;
};

void MCStackTilePush(MCPlatformStackTile *tile);
void MCStackTileCollectAll(void);

#endif
