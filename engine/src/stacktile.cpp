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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "stacktile.h"
#include "systhreads.h"
#include "region.h"
#include "graphics_util.h"
#include "stack.h"

////////////////////////////////////////////////////////////////////////////////

MCPlatformStackTile::MCPlatformStackTile(MCStack *p_stack, MCStackSurface *p_surface, const MCGIntegerRectangle &p_region)
{
    m_stack = p_stack;
    m_surface = p_surface;
    m_region = p_region;
    m_context = NULL;
}

MCPlatformStackTile::~MCPlatformStackTile()
{
}

bool MCPlatformStackTile::Lock()
{
    return m_surface -> LockGraphics(m_region, m_context, m_raster);
}

void MCPlatformStackTile::Unlock()
{
    m_surface -> UnlockGraphics(m_region, m_context, m_raster);
}

void MCPlatformStackTile::Render()
{
#ifndef _MAC_DESKTOP
    // IM-2014-01-24: [[ HiDPI ]] Use view backing scale to transform surface -> logical coords
    MCGFloat t_backing_scale;
    t_backing_scale = m_stack -> view_getbackingscale();
    
    // p_region is in surface coordinates, translate to user-space coords & ensure any fractional pixels are accounted for
    MCRectangle t_rect;
    t_rect = MCRectangleGetScaledBounds(MCRectangleFromMCGIntegerRectangle(m_region), 1 / t_backing_scale);
    
    // scale user -> surface space
    MCGContextScaleCTM(m_context, t_backing_scale, t_backing_scale);
    
    m_stack -> view_render(m_context, t_rect);
#else
    m_stack -> view_render(m_context, MCRectangleFromMCGIntegerRectangle(m_region));
#endif
}

////////////////////////////////////////////////////////////////////////////////

struct MCStackTile
{
    MCStackTile             *next;
    MCPlatformStackTile     *tile;
};

static MCStackTile *s_inactive_tiles = NULL;
static MCStackTile *s_waiting_tiles = NULL;
static uindex_t s_active_tile_count = 0;
static MCThreadMutexRef s_main_thread_mutex = NULL;
static MCThreadConditionRef s_main_thread_condition = NULL;

////////////////////////////////////////////////////////////////////////////////

bool MCStackTileInitialize()
{
    s_inactive_tiles = NULL;
    s_waiting_tiles = NULL;
    s_active_tile_count = 0;
    s_main_thread_mutex = NULL;
    s_main_thread_condition = NULL;
    
    bool t_success;
    t_success = true;
    
    if (t_success)
        t_success = MCThreadMutexCreate(s_main_thread_mutex);
    
    if (t_success)
        t_success = MCThreadConditionCreate(s_main_thread_condition);

    return t_success;
}

void MCStackTileFinalize()
{
    MCThreadMutexRelease(s_main_thread_mutex);
    MCThreadConditionRelease(s_main_thread_condition);
    
    s_inactive_tiles = NULL;
    s_waiting_tiles = NULL;
    s_active_tile_count = 0;
    s_main_thread_mutex = NULL;
    s_main_thread_condition = NULL;
}

void MCStackTileRender(void *p_ctxt)
{
    MCStackTile *t_tile;
	t_tile = (MCStackTile *)p_ctxt;
    t_tile -> tile -> Render();
    
    MCThreadMutexLock(s_main_thread_mutex);
    t_tile -> next = s_waiting_tiles;
    s_waiting_tiles = t_tile;
    MCThreadConditionSignal(s_main_thread_condition);
    MCThreadMutexUnlock(s_main_thread_mutex);
}

void MCStackTilePush(MCPlatformStackTile *p_tile)
{
    if (!p_tile -> Lock())
    {
        delete p_tile;
        return;
    }
    
    // Make sure we have at least one inactive tile.
    if (s_inactive_tiles == nil)
    {
        MCStackTile *t_new_tile;
        /* UNCHECKED */ MCMemoryNew(t_new_tile);
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
    MCThreadPoolPushTask(MCStackTileRender, (void *) t_tile);
}

void MCStackTileCollectAll(void)
{
    MCThreadMutexLock(s_main_thread_mutex);
    while(s_active_tile_count > 0)
    {
        if (s_waiting_tiles == nil)
            MCThreadConditionWait(s_main_thread_condition, s_main_thread_mutex);
        while (s_waiting_tiles != nil)
        {
            MCStackTile *t_tile;
            t_tile = s_waiting_tiles;
            s_waiting_tiles = s_waiting_tiles -> next;
            s_active_tile_count -= 1;
            MCThreadMutexUnlock(s_main_thread_mutex);
            
            t_tile -> tile -> Unlock();
            delete t_tile -> tile;
            t_tile -> tile = NULL;
            
            // Move the tile to the inactive list.
            t_tile -> next = s_inactive_tiles;
            s_inactive_tiles = t_tile;
            
            MCThreadMutexLock(s_main_thread_mutex);
        }
    }
    MCThreadMutexUnlock(s_main_thread_mutex);
}

////////////////////////////////////////////////////////////////////////////////
