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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "player.h"
#include "exec.h"

#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCPlayer::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_FILE_NAME, OptionalString, MCPlayer, FileName)
	DEFINE_RW_OBJ_PROPERTY(P_DONT_REFRESH, Bool, MCPlayer, DontRefresh)
    // PM-2014-09-15: [[ Bug 13437 ]] Make sure the time-related vars are large enough to hold larger time values
	DEFINE_RW_OBJ_PROPERTY(P_CURRENT_TIME, Double, MCPlayer, CurrentTime)
    // PM-2014-11-03: [[ Bug 13920 ]] Make sure the loadedTime property is defined
    DEFINE_RO_OBJ_PROPERTY(P_MOVIE_LOADED_TIME, Double, MCPlayer, LoadedTime)
	DEFINE_RO_OBJ_PROPERTY(P_DURATION, Double, MCPlayer, Duration)
	DEFINE_RW_OBJ_PROPERTY(P_LOOPING, Bool, MCPlayer, Looping)
	DEFINE_RW_OBJ_PROPERTY(P_PAUSED, Bool, MCPlayer, Paused)
	DEFINE_RW_OBJ_PROPERTY(P_ALWAYS_BUFFER, Bool, MCPlayer, AlwaysBuffer)
	DEFINE_RW_OBJ_PROPERTY(P_PLAY_RATE, Double, MCPlayer, PlayRate)
	DEFINE_RW_OBJ_PROPERTY(P_START_TIME, OptionalDouble, MCPlayer, StartTime)
	DEFINE_RW_OBJ_PROPERTY(P_END_TIME, OptionalDouble, MCPlayer, EndTime)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_BADGE, Bool, MCPlayer, ShowBadge)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_CONTROLLER, Bool, MCPlayer, ShowController)
	DEFINE_RW_OBJ_PROPERTY(P_PLAY_SELECTION, Bool, MCPlayer, PlaySelection)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_SELECTION, Bool, MCPlayer, ShowSelection)
	DEFINE_RW_OBJ_PROPERTY(P_CALLBACKS, String, MCPlayer, Callbacks)
    DEFINE_RW_OBJ_PROPERTY(P_MOVIE_CONTROLLER_ID, Int32, MCPlayer, MovieControllerId)
	DEFINE_RO_OBJ_SET_PROPERTY(P_MEDIA_TYPES, InterfaceMediaTypes, MCPlayer, MediaTypes)
	DEFINE_RW_OBJ_PROPERTY(P_CURRENT_NODE, UInt16, MCPlayer, CurrentNode)
	DEFINE_RW_OBJ_PROPERTY(P_PAN, Double, MCPlayer, Pan)
	DEFINE_RW_OBJ_PROPERTY(P_TILT, Double, MCPlayer, Tilt)
	DEFINE_RW_OBJ_PROPERTY(P_ZOOM, Double, MCPlayer, Zoom)
	DEFINE_RO_OBJ_PROPERTY(P_TRACKS, String, MCPlayer, Tracks)
    DEFINE_RO_OBJ_PROPERTY(P_TRACK_COUNT, UInt32, MCPlayer, TrackCount)
	DEFINE_RO_OBJ_PROPERTY(P_NODES, String, MCPlayer, Nodes)
	DEFINE_RO_OBJ_PROPERTY(P_HOT_SPOTS, String, MCPlayer, HotSpots)
	DEFINE_RO_OBJ_CUSTOM_PROPERTY(P_CONSTRAINTS, MultimediaQTVRConstraints, MCPlayer, Constraints)
    // PM-2015-06-01: [[ Bug 15439 ]] EnabledTracks should be RW
    DEFINE_RW_OBJ_LIST_PROPERTY(P_ENABLED_TRACKS, LinesOfLooseUInt, MCPlayer, EnabledTracks)
    DEFINE_RW_OBJ_PROPERTY(P_PLAY_LOUDNESS, UInt16, MCPlayer, PlayLoudness)
    DEFINE_RO_OBJ_PROPERTY(P_TIME_SCALE, Double, MCPlayer, TimeScale)
    // SN-2014-08-06: [[ Bug 13115 ]] Missing formatted (width|height) in the property table
    DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_HEIGHT, Int32, MCPlayer, FormattedHeight)
    DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_WIDTH, Int32, MCPlayer, FormattedWidth)
    DEFINE_RW_OBJ_PROPERTY(P_DONT_USE_QT, Bool, MCPlayer, DontUseQT)
    DEFINE_RW_OBJ_PROPERTY(P_LEFT_BALANCE, Double, MCPlayer, LeftBalance)
    DEFINE_RW_OBJ_PROPERTY(P_RIGHT_BALANCE, Double, MCPlayer, RightBalance)
    DEFINE_RW_OBJ_PROPERTY(P_AUDIO_PAN, Double, MCPlayer, AudioPan)
#ifdef FEATURE_PLATFORM_PLAYER
    DEFINE_RO_OBJ_ENUM_PROPERTY(P_STATUS, InterfacePlayerStatus, MCPlayer, Status)
    DEFINE_RW_OBJ_PROPERTY(P_MIRRORED, Bool, MCPlayer, Mirrored)
#endif
};

MCObjectPropertyTable MCPlayer::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

bool MCPlayer::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnPlayer(this);
}

void MCPlayer::removereferences()
{
    // OK-2009-04-30: [[Bug 7517]] - Ensure the player is actually closed before deletion, otherwise dangling references may still exist.
    while (opened)
        close();
    
    playstop();
    
    MCObject::removereferences();
}

void MCPlayer::removefromplayers()
{
    if (MCplayers)
    {
        if (MCplayers == this)
            MCplayers = nextplayer;
        else
        {
            MCPlayer *tptr = MCplayers;
            while (tptr->nextplayer.IsValid() && tptr->nextplayer != this)
                tptr = tptr->nextplayer;
            
            if (tptr->nextplayer.IsValid() && tptr->nextplayer == this)
                tptr->nextplayer = nextplayer;
        }
     }
     nextplayer = nullptr;
}

void MCPlayer::SyncPlayers(MCStack* p_stack, MCContext *p_context)
{
    for(MCPlayerHandle t_player = MCplayers;
        t_player.IsValid();
        t_player = t_player -> getnextplayer())
    {
        if (p_stack == nullptr ||
            t_player -> getstack() == p_stack)
            t_player -> syncbuffering(p_context);
    }
}

#ifdef FEATURE_PLATFORM_PLAYER
void MCPlayer::DetachPlayers(MCStack* p_stack)
{
    for(MCPlayerHandle t_player = MCplayers;
        t_player.IsValid();
        t_player = t_player -> getnextplayer())
    {
        if (t_player -> getstack() == p_stack)
            t_player -> detachplayer();
    }
}

void MCPlayer::AttachPlayers(MCStack* p_stack)
{
    for(MCPlayerHandle t_player = MCplayers;
        t_player.IsValid();
        t_player = t_player -> getnextplayer())
    {
        if (t_player -> getstack() == p_stack)
            t_player -> attachplayer();
    }
}
#endif

void MCPlayer::StopPlayers(MCStack* p_stack)
{
    MCPlayerHandle t_player = MCplayers;
    while(t_player.IsValid())
    {
        if (t_player -> getstack() == p_stack)
        {
#ifdef FEATURE_PLATFORM_PLAYER
            t_player->playstop();
#else
            if (t_player->playstop())
            {
                // player was removed from list, start search over
                t_player = MCplayers;
                continue;
            }
                
#endif
        }
        t_player = t_player->getnextplayer();
    }
}

void MCPlayer::ClosePlayers(MCStack* p_stack)
{
    MCPlayerHandle t_player = MCplayers;
    while(t_player.IsValid())
    {
        MCPlayer *oldptr = t_player;
        t_player = t_player->getnextplayer();
        if (oldptr->getstack() == p_stack)
            oldptr->close();
    }
}

MCPlayer* MCPlayer::FindPlayerByName(MCNameRef p_name)
{
    MCPlayerHandle t_player = MCplayers;
    while (t_player.IsValid())
    {
        if (t_player -> hasname(p_name))
        {
            return t_player;
        }
        t_player = t_player->getnextplayer();
    }
    
    return nil;
}

MCPlayer* MCPlayer::FindPlayerById(uint32_t p_id)
{
    MCPlayerHandle t_player = MCplayers;
    while (t_player.IsValid())
    {
        if (t_player -> getaltid() == p_id)
        {
            return t_player;
        }
        t_player = t_player->getnextplayer();
    }
    return nil;
}

void MCPlayer::SetPlayersVolume(uinteger_t p_volume)
{
    MCPlayerHandle t_player = MCplayers;
    while (t_player.IsValid())
    {
        t_player -> setvolume(p_volume);
        t_player = t_player->getnextplayer();
    }
}
