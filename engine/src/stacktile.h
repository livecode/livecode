/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

class MCStackTile
{
public:
    virtual bool Lock(void) = 0;
	virtual void Unlock(void) = 0;
    virtual void Render(void) = 0;
};

void MCStackTilePush(MCStackTile *tile);
void MCStackTileCollectAll(void);

#endif
