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

////////////////////////////////////////////////////////////////////////////////

#include "stacktile.h"

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "systhreads.h"
#include "region.h"

////////////////////////////////////////////////////////////////////////////////


// A tile thread sits and waits on a condition variable. When the variable is signalled,
// it wakes up and starts processing the request. When the tile thread has done its work
// it notifies the main thread's condition variable.

struct MCStackTile
{
    MCStackTile             *next;
    MCPlatformStackTile     *tile;
};

static MCStackTile *s_inactive_tiles = NULL;
static MCStackTile *s_waiting_tiles = NULL;
static uindex_t s_active_tile_count = 0;

void MCStackTileRender(void *p_ctxt)
{
    //NSAutoreleasePool *t_pool;
    //t_pool = [[NSAutoreleasePool alloc] init];
    
    MCStackTile *t_tile;
	t_tile = (MCStackTile *)p_ctxt;
    t_tile -> tile -> Render();
    
    MCThreadMainThreadMutexLock();
    t_tile -> next = s_waiting_tiles;
    s_waiting_tiles = t_tile;
    MCThreadMainThreadConditionSignal();
    MCThreadMainThreadMutexUnlock();
}

void MCStackTilePush(MCPlatformStackTile *p_tile)
{
    // Make sure we have at least one inactive tile.
    if (s_inactive_tiles == nil)
    {
        MCStackTile *t_new_tile;
        t_new_tile = new MCStackTile;
        t_new_tile -> next = s_inactive_tiles;
        t_new_tile -> tile = NULL;
        s_inactive_tiles = t_new_tile;
    }
    
    // Pull the first inactive tile off the list (we know we have at least one!).
    MCStackTile *t_tile;
    t_tile = s_inactive_tiles;
    s_inactive_tiles = t_tile -> next;
    s_active_tile_count++;
    
    t_tile -> tile = p_tile;
    t_tile -> tile -> Lock();
    MCThreadPoolPushTask(MCStackTileRender, (void *) t_tile);
}

void MCStackTileCollectAll(void)
{
    MCThreadMainThreadMutexLock();
    while(s_active_tile_count > 0)
    {
        if (s_waiting_tiles == nil)
            MCThreadMainThreadConditionWait();
        while (s_waiting_tiles != nil)
        {
            MCStackTile *t_tile;
            t_tile = s_waiting_tiles;
            s_waiting_tiles = s_waiting_tiles -> next;
            s_active_tile_count -= 1;
            MCThreadMainThreadMutexUnlock();
            
            t_tile -> tile -> Unlock();
            delete t_tile -> tile;
            t_tile -> tile = NULL;
            
            // Move the tile to the inactive list.
            t_tile -> next = s_inactive_tiles;
            s_inactive_tiles = t_tile;
            
            MCThreadMainThreadMutexLock();
        }
    }
    MCThreadMainThreadMutexUnlock();
}

////////////////////////////////////////////////////////////////////////////////
