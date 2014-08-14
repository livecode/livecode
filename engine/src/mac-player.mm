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

#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"
#include "graphics_util.h"
#include "mac-player.h"

////////////////////////////////////////////////////////////////////////////////

MCPlatformPlayer::MCPlatformPlayer(void)
{
	m_references = 1;
	m_window = nil;
}

MCPlatformPlayer::~MCPlatformPlayer(void)
{
	// PM-2014-08-11: [[ Bug 13109 ]] Moved code to MCPlatformPlayer::Release
}

void MCPlatformPlayer::Retain(void)
{
	m_references += 1;
}

void MCPlatformPlayer::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
    {
        // PM-2014-08-11: [[ Bug 13109 ]] Fixes issue of abort due to a pure virtual call
        if (m_window != nil)
            Detach();
		delete this;
    }
}

void MCPlatformPlayer::Attach(MCPlatformWindowRef p_window)
{
	if (m_window == p_window)
		return;
    
	if (m_window != nil)
		Detach();
    
	m_window = p_window;
	MCPlatformRetainWindow(m_window);
    
	m_window -> AttachObject(this, DoWindowStateChanged);
}

void MCPlatformPlayer::Detach(void)
{
	if (m_window == nil)
		return;
    
	m_window -> DetachObject(this);
    
	MCPlatformReleaseWindow(m_window);
	m_window = nil;
}

void MCPlatformPlayer::DoWindowStateChanged(void *p_ctxt, bool p_realized)
{
	MCPlatformPlayer *t_player;
	t_player = (MCPlatformPlayer *)p_ctxt;
    
	if (p_realized)
		t_player -> Realize();
	else
		t_player -> Unrealize();
}

///////////////////////////////////////////////////////////////////////////////

class MCAVFoundationPlayer;
class MCQTKitPlayer;

extern MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void);
extern MCQTKitPlayer *MCQTKitPlayerCreate(void);
extern uint4 MCmajorosversion;
extern bool MCQTInit(void);

void MCPlatformCreatePlayer(MCPlatformPlayerRef& r_player)
{
    // MW-2014-07-16: [[ QTSupport ]] If we manage to init QT (i.e. dontUseQT is false and
    //   QT is available) then use QTKit, else use AVFoundation if 10.8 and above.
    if (!MCQTInit() && MCmajorosversion >= 0x1080)
        r_player = (MCPlatformPlayerRef)MCAVFoundationPlayerCreate();
    else
        r_player = (MCPlatformPlayerRef)MCQTKitPlayerCreate();
}

void MCPlatformPlayerRetain(MCPlatformPlayerRef player)
{
	player -> Retain();
}

void MCPlatformPlayerRelease(MCPlatformPlayerRef player)
{
	player -> Release();
}

void MCPlatformAttachPlayer(MCPlatformPlayerRef player, MCPlatformWindowRef window)
{
	player -> Attach(window);
}

void MCPlatformDetachPlayer(MCPlatformPlayerRef player)
{
	player -> Detach();
}

bool MCPlatformPlayerIsPlaying(MCPlatformPlayerRef player)
{
	return player -> IsPlaying();
}

void MCPlatformStepPlayer(MCPlatformPlayerRef player, int amount)
{
	player -> Step(amount);
}

void MCPlatformStartPlayer(MCPlatformPlayerRef player, double rate)
{
	player -> Start(rate);
}

/*
void MCPlatformFastPlayer(MCPlatformPlayerRef player, Boolean forward)
{
	player -> Fast(forward);
}

void MCPlatformFastForwardPlayer(MCPlatformPlayerRef player)
{
	player -> FastForward();
}

void MCPlatformFastBackPlayer(MCPlatformPlayerRef player)
{
	player -> FastBack();
}
*/

void MCPlatformStopPlayer(MCPlatformPlayerRef player)
{
	player -> Stop();
}

void MCPlatformLockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap*& r_bitmap)
{
	player -> LockBitmap(r_bitmap);
}

void MCPlatformUnlockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap *bitmap)
{
	player -> UnlockBitmap(bitmap);
}

void MCPlatformSetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
	player -> SetProperty(property, type, value);
}

void MCPlatformGetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value)
{
	player -> GetProperty(property, type, value);
}

void MCPlatformCountPlayerTracks(MCPlatformPlayerRef player, uindex_t& r_track_count)
{
	player -> CountTracks(r_track_count);
}

void MCPlatformGetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
	player -> GetTrackProperty(index, property, type, value);
}

void MCPlatformSetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value)
{
	player -> SetTrackProperty(index, property, type, value);
}

bool MCPlatformFindPlayerTrackWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
	return player -> FindTrackWithId(id, r_index);
}

void MCPlatformCountPlayerNodes(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

void MCPlatformGetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformSetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformFindPlayerNodeWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

void MCPlatformCountPlayerHotSpots(MCPlatformPlayerRef player, uindex_t& r_node_count)
{
}

void MCPlatformGetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformSetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value)
{
}

void MCPlatformFindPlayerHotSpotWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index)
{
}

////////////////////////////////////////////////////////////////////////////////
