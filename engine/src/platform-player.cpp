/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include "globdefs.h"

#include "platform.h"
#include "platform-internal.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCPlatformPlayer::MCPlatformPlayer(void)
{
	m_references = 1;
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
		delete this;
}

///////////////////////////////////////////////////////////////////////////////

void MCPlatformPlayerRetain(MCPlatformPlayerRef player)
{
	player -> Retain();
}

void MCPlatformPlayerRelease(MCPlatformPlayerRef player)
{
	player -> Release();
}

void *MCPlatformPlayerGetNativeView(MCPlatformPlayerRef player)
{
	void *t_view;
	if (!player -> GetNativeView(t_view))
		return nil;
	
	return t_view;
}

bool MCPlatformPlayerSetNativeParentView(MCPlatformPlayerRef p_player, void *p_parent_view)
{
	return p_player -> SetNativeParentView(p_parent_view);
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

bool MCPlatformLockPlayerBitmap(MCPlatformPlayerRef player, const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap)
{
	return player -> LockBitmap(p_size, r_bitmap);
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

#ifdef TARGET_PLATFORM_MACOS_X

class MCAVFoundationPlayer;

extern MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void);
extern uint4 MCmajorosversion;

void MCPlatformCreatePlayer(MCPlatformPlayerRef& r_player)
{
	r_player = (MCPlatformPlayerRef)MCAVFoundationPlayerCreate();
}

#endif

#ifdef TARGET_PLATFORM_WINDOWS
class MCWin32DSPlayer;
extern MCWin32DSPlayer *MCWin32DSPlayerCreate(void);
void MCPlatformCreatePlayer(MCPlatformPlayerRef &r_player)
{
	r_player = (MCPlatformPlayerRef)MCWin32DSPlayerCreate();
}
#endif

////////////////////////////////////////////////////////////////////////////////
