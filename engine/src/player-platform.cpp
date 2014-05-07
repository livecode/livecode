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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "core.h"

#include "execpt.h"
#include "util.h"
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "stacklst.h"
#include "card.h"
#include "field.h"
#include "player-platform.h"
#include "aclip.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "mode.h"
#include "context.h"
#include "osspec.h"
#include "redraw.h"

#include "graphics_util.h"

//// PLATFORM PLAYER

#include "platform.h"

static MCPlatformPlayerMediaType ppmediatypes[] =
{
	kMCPlatformPlayerMediaTypeVideo,
	kMCPlatformPlayerMediaTypeAudio,
	kMCPlatformPlayerMediaTypeText,
	kMCPlatformPlayerMediaTypeQTVR,
	kMCPlatformPlayerMediaTypeSprite,
	kMCPlatformPlayerMediaTypeFlash,
};

static const char *ppmediastrings[] =
{
	"video",
	"audio",
	"text",
	"qtvr",
	"sprite",
	"flash"
};

#define CONTROLLER_HEIGHT 16

enum
{
    kMCPlayerControllerPartUnknown,
    
    kMCPlayerControllerPartVolume,
    kMCPlayerControllerPartPlay,
    kMCPlayerControllerPartScrubBack,
    kMCPlayerControllerPartScrubForward,
    kMCPlayerControllerPartThumb,
    kMCPlayerControllerPartWell,
    kMCPlayerControllerPartSelectionStart,
    kMCPlayerControllerPartSelectionFinish,
};

//-----------------------------------------------------------------------------
// Control Implementation
//

#define XANIM_WAIT 10.0
#define XANIM_COMMAND 1024

MCPlayer::MCPlayer()
{
	flags |= F_TRAVERSAL_ON;
	nextplayer = NULL;
	rect.width = rect.height = 128;
	filename = NULL;
	istmpfile = False;
	scale = 1.0;
	rate = 1.0;
	lasttime = 0;
	starttime = endtime = MAXUINT4;
	disposable = istmpfile = False;
	userCallbackStr = NULL;
	formattedwidth = formattedheight = 0;
	loudness = 100;
    
	m_platform_player = nil;
}

MCPlayer::MCPlayer(const MCPlayer &sref) : MCControl(sref)
{
	nextplayer = NULL;
	filename = strclone(sref.filename);
	istmpfile = False;
	scale = 1.0;
	rate = sref.rate;
	lasttime = sref.lasttime;
	starttime = sref.starttime;
	endtime = sref.endtime;
	disposable = istmpfile = False;
	userCallbackStr = strclone(sref.userCallbackStr);
	formattedwidth = formattedheight = 0;
	loudness = sref.loudness;
	
	m_platform_player = nil;
}

MCPlayer::~MCPlayer()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the player is actually closed before deletion, otherwise dangling references may still exist.
	while (opened)
		close();
	
	playstop();
    
	if (m_platform_player != nil)
		MCPlatformPlayerRelease(m_platform_player);

	delete filename;
	delete userCallbackStr;
}

Chunk_term MCPlayer::gettype() const
{
	return CT_PLAYER;
}

const char *MCPlayer::gettypestring()
{
	return MCplayerstring;
}

MCRectangle MCPlayer::getactiverect(void)
{
	return MCU_reduce_rect(getrect(), getflag(F_SHOW_BORDER) ? borderwidth : 0);
}

void MCPlayer::open()
{
	MCControl::open();
	prepare(MCnullstring);
}

void MCPlayer::close()
{
	MCControl::close();
	if (opened == 0)
	{
		state |= CS_CLOSING;
		playstop();
		state &= ~CS_CLOSING;
	}
}

Boolean MCPlayer::kdown(const char *string, KeySym key)
{
	if (!(state & CS_NO_MESSAGES))
		if (MCObject::kdown(string, key))
			return True;
    
	return False;
}

Boolean MCPlayer::kup(const char *string, KeySym key)
{
    return False;
}

Boolean MCPlayer::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
        || flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
		return False;
    
    Boolean t_success;
    t_success = MCControl::mfocus(x, y);
    if (t_success)
        handle_mfocus(x,y);
    return t_success;
}

void MCPlayer::munfocus()
{
	getstack()->resetcursor(True);
	MCControl::munfocus();
}

Boolean MCPlayer::mdown(uint2 which)
{
    if (state & CS_MFOCUSED || flags & F_DISABLED)
		return False;
    if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	state |= CS_MFOCUSED;
	if (flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
		getstack()->kfocusset(this);
	switch (which)
	{
        case Button1:
            switch (getstack()->gettool(this))
		{
            case T_BROWSE:
                message_with_args(MCM_mouse_down, "1");
                handle_mdown(which);
                break;
            case T_POINTER:
            case T_PLAYER:  //when the movie object is in editing mode
                start(True); //starting draggin or resizing
                playpause(True);  //pause the movie
                break;
            case T_HELP:
                break;
            default:
                return False;
		}
            break;
		case Button2:
            if (message_with_args(MCM_mouse_down, "2") == ES_NORMAL)
                return True;
            break;
		case Button3:
            message_with_args(MCM_mouse_down, "3");
            break;
	}
	return True;
}

Boolean MCPlayer::mup(uint2 which) //mouse up
{
	if (!(state & CS_MFOCUSED))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which);
	state &= ~CS_MFOCUSED;
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	switch (which)
	{
        case Button1:
            switch (getstack()->gettool(this))
		{
            case T_BROWSE:
                if (MCU_point_in_rect(rect, mx, my))
                    message_with_args(MCM_mouse_up, "1");
                else
                    message_with_args(MCM_mouse_release, "1");
                handle_mup(which);
                break;
            case T_PLAYER:
            case T_POINTER:
                end();       //stop dragging or moving the movie object, will change controller size
                break;
            case T_HELP:
                help();
                break;
            default:
                return False;
		}
            break;
        case Button2:
        case Button3:
            if (MCU_point_in_rect(rect, mx, my))
                message_with_args(MCM_mouse_up, which);
            else
                message_with_args(MCM_mouse_release, which);
            break;
	}
	return True;
}

Boolean MCPlayer::doubledown(uint2 which)
{
	return MCControl::doubledown(which);
}

Boolean MCPlayer::doubleup(uint2 which)
{
	return MCControl::doubleup(which);
}

void MCPlayer::setrect(const MCRectangle &nrect)
{
	rect = nrect;
	
	if (m_platform_player != nil)
	{
		MCRectangle trect = MCU_reduce_rect(rect, getflag(F_SHOW_BORDER) ? borderwidth : 0);
        
        if (getflag(F_SHOW_CONTROLLER))
            trect . height -= CONTROLLER_HEIGHT;
        
        // MW-2014-04-09: [[ Bug 11922 ]] Make sure we use the view not device transform
        //   (backscale factor handled in platform layer).
		trect = MCRectangleGetTransformedBounds(trect, getstack()->getviewtransform());
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRect, kMCPlatformPropertyTypeRectangle, &trect);
	}
}

void MCPlayer::timer(MCNameRef mptr, MCParameter *params)
{	
    if (MCNameIsEqualTo(mptr, MCM_play_started, kMCCompareCaseless))
    {
        state &= ~CS_PAUSED;
    }
    else
        if (MCNameIsEqualTo(mptr, MCM_play_stopped, kMCCompareCaseless))
        {
            state |= CS_PAUSED;

            if (disposable)
            {
                playstop();
                return; //obj is already deleted, do not pass msg up.
            }
        }
        else if (MCNameIsEqualTo(mptr, MCM_play_paused, kMCCompareCaseless))
		{
			state |= CS_PAUSED;

		}
        else if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
        {
            if (mup(Button1))
            {
                handle_mstilldown(Button1);
                //MCscreen->addtimer(this, MCM_internal, MCsyncrate);
            }

        }
        
	MCControl::timer(mptr, params);
}

Exec_stat MCPlayer::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	uint2 i = 0;
	switch (which)
	{
#ifdef /* MCPlayer::getprop */ LEGACY_EXEC
        case P_FILE_NAME:
            if (filename == NULL)
                ep.clear();
            else
                ep.setsvalue(filename);
            break;
        case P_DONT_REFRESH:
            ep.setboolean(getflag(F_DONT_REFRESH));
            break;
        case P_CURRENT_TIME:
            ep.setint(getmoviecurtime());
            break;
        case P_DURATION:
            ep.setint(getduration());
            break;
        case P_LOOPING:
            ep.setboolean(getflag(F_LOOPING));
            break;
        case P_PAUSED:
            ep.setboolean(ispaused());
            break;
        case P_ALWAYS_BUFFER:
            ep.setboolean(getflag(F_ALWAYS_BUFFER));
            break;
        case P_PLAY_RATE:
            ep.setr8(rate, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
            return ES_NORMAL;
        case P_START_TIME:
            if (starttime == MAXUINT4)
                ep.clear();
            else
                ep.setnvalue(starttime);//for QT, this is the selection start time
            break;
        case P_END_TIME:
            if (endtime == MAXUINT4)
                ep.clear();
            else
                ep.setnvalue(endtime); //for QT, this is the selection's end time
            break;
        case P_SHOW_BADGE:
            ep.setboolean(getflag(F_SHOW_BADGE));
            break;
        case P_SHOW_CONTROLLER:
            ep.setboolean(getflag(F_SHOW_CONTROLLER));
            break;
        case P_PLAY_SELECTION:
            ep.setboolean(getflag(F_PLAY_SELECTION));
            break;
        case P_SHOW_SELECTION:
            ep.setboolean(getflag(F_SHOW_SELECTION));
            break;
        case P_CALLBACKS:
            ep.setsvalue(userCallbackStr);
            break;
        case P_TIME_SCALE:
            ep.setint(gettimescale());
            break;
        case P_FORMATTED_HEIGHT:
            ep.setint(getpreferredrect().height);
            break;
        case P_FORMATTED_WIDTH:
            ep.setint(getpreferredrect().width);
            break;
        case P_MOVIE_CONTROLLER_ID:
            ep.setint((int)NULL);
            break;
        case P_PLAY_LOUDNESS:
            ep.setint(getloudness());
            break;
        case P_TRACK_COUNT:
            if (m_platform_player != nil)
            {
                uindex_t t_count;
                MCPlatformCountPlayerTracks(m_platform_player, t_count);
                i = t_count;
            }
            ep.setint(i);
            break;
        case P_TRACKS:
            gettracks(ep);
            break;
        case P_ENABLED_TRACKS:
            getenabledtracks(ep);
            break;
        case P_MEDIA_TYPES:
            ep.clear();
            if (m_platform_player != nil)
            {
                MCPlatformPlayerMediaTypes t_types;
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMediaTypes, kMCPlatformPropertyTypePlayerMediaTypes, &t_types);
                bool first = true;
                for (i = 0 ; i < sizeof(ppmediatypes) / sizeof(ppmediatypes[0]) ; i++)
                    if ((t_types & (1 << ppmediatypes[i])) != 0)
                    {
                        ep.concatcstring(ppmediastrings[i], EC_COMMA, first);
                        first = false;
                    }
            }
            break;
        case P_CURRENT_NODE:
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &i);
            ep.setint(i);
            break;
        case P_PAN:
		{
			real8 pan = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);

			ep.setr8(pan, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_TILT:
		{
			real8 tilt = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);

			ep.setr8(tilt, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_ZOOM:
		{
			real8 zoom = 0.0;
			if (m_platform_player != nil)
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);

			ep.setr8(zoom, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
            break;
        case P_CONSTRAINTS:
			ep.clear();
			if (m_platform_player != nil)
			{
				MCPlatformPlayerQTVRConstraints t_constraints;
				MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRConstraints, kMCPlatformPropertyTypePlayerQTVRConstraints, &t_constraints);
				ep.appendstringf("%lf,%lf\n", t_constraints . x_min, t_constraints . x_max);
				ep.appendstringf("%lf,%lf\n", t_constraints . y_min, t_constraints . y_max);
				ep.appendstringf("%lf,%lf", t_constraints . z_min, t_constraints . z_max);
			}
            break;
        case P_NODES:
            getnodes(ep);
            break;
        case P_HOT_SPOTS:
            gethotspots(ep);
            break;
#endif /* MCPlayer::getprop */
        default:
            return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCPlayer::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = False;
	Boolean wholecard = False;
	uint4 ctime;
	MCString data = ep.getsvalue();
    
	switch (p)
	{
#ifdef /* MCPlayer::setprop */ LEGACY_EXEC
        case P_FILE_NAME:
            if (filename == NULL || data != filename)
            {
                delete filename;
                filename = NULL;
                playstop();
                starttime = MAXUINT4; //clears the selection
                endtime = MAXUINT4;
                if (data != MCnullmcstring)
                    filename = data.clone();
                prepare(MCnullstring);
                dirty = wholecard = True;
            }
            break;
        case P_DONT_REFRESH:
            if (!MCU_matchflags(data, flags, F_DONT_REFRESH, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            break;
        case P_ALWAYS_BUFFER:
            if (!MCU_matchflags(data, flags, F_ALWAYS_BUFFER, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            
            // The actual buffering state is determined upon redrawing - therefore
            // we trigger a redraw to ensure we don't unbuffer when it is
            // needed.
            
            if (opened)
                dirty = True;
            break;
        case P_CALLBACKS:
            delete userCallbackStr;
            if (data.getlength() == 0)
                userCallbackStr = NULL;
            else
            {
                userCallbackStr = data.clone();
            }
			SynchronizeUserCallbacks();
            break;
        case P_CURRENT_TIME:
            if (!MCU_stoui4(data, ctime))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            setcurtime(ctime);
            if (isbuffering())
                dirty = True;
            break;
        case P_LOOPING:
            if (!MCU_matchflags(data, flags, F_LOOPING, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                setlooping((flags & F_LOOPING) != 0); //set/unset movie looping
            break;
        case P_PAUSED:
            playpause(data == MCtruemcstring); //pause or unpause the player
            break;
        case P_PLAY_RATE:
            if (!MCU_stor8(data, rate))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            setplayrate();
            break;
        case P_START_TIME: //for QT, this is the selection start time
            if (data.getlength() == 0)
                starttime = endtime = MAXUINT4;
            else
            {
                if (!MCU_stoui4(data, starttime))
                {
                    MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                    return ES_ERROR;
                }
            }
            setselection();
            break;
        case P_END_TIME: //for QT, this is the selection end time
            if (data.getlength() == 0)
                starttime = endtime = MAXUINT4;
            else
            {
                if (!MCU_stoui4(data, endtime))
                {
                    MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                    return ES_ERROR;
                }
            }
            setselection();
            break;
        case P_TRAVERSAL_ON:
            if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
                return ES_ERROR;
            break;
        case P_SHOW_BADGE: //if in the buffering mode we do not want to show/hide the badge
            if (!(flags & F_ALWAYS_BUFFER))
            { //if always buffer flag is not set
                if (!MCU_matchflags(data, flags, F_SHOW_BADGE, dirty))
                {
                    MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                    return ES_ERROR;
                }
                if (dirty && !isbuffering()) //we are not actually buffering, let's show/hide the badge
                    showbadge((flags & F_SHOW_BADGE) != 0); //show/hide movie's badge
            }
            break;
        case P_SHOW_CONTROLLER:
            if (!MCU_matchflags(data, flags, F_SHOW_CONTROLLER, dirty))
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
            {
                showcontroller((flags & F_VISIBLE) != 0
                               && (flags & F_SHOW_CONTROLLER) != 0);
                dirty = False;
            }
            break;
        case P_PLAY_SELECTION: //make QT movie plays only the selected part
            if (!MCU_matchflags(data, flags, F_PLAY_SELECTION, dirty))
            {
                MCeerror->add
                (EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                playselection((flags & F_PLAY_SELECTION) != 0);
            break;
        case P_SHOW_SELECTION: //means make QT movie editable
            if (!MCU_matchflags(data, flags, F_SHOW_SELECTION, dirty))
            {
                MCeerror->add
                (EE_OBJECT_NAB, 0, 0, data);
                return ES_ERROR;
            }
            if (dirty)
                editmovie((flags & F_SHOW_SELECTION) != 0);
            break;
        case P_SHOW_BORDER:
        case P_BORDER_WIDTH:
            if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
                return ES_ERROR;
            setrect(rect);
            dirty = True;
            break;
        case P_MOVIE_CONTROLLER_ID:
            break;
        case P_PLAY_LOUDNESS:
            if (!MCU_stoui2(data, loudness))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            loudness = MCU_max(0, loudness);
            loudness = MCU_min(loudness, 100);
            setloudness();
            break;
        case P_ENABLED_TRACKS:
            if (!setenabledtracks(data))
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
                return ES_ERROR;
            }
            dirty = wholecard = True;
            break;
        case P_CURRENT_NODE:
		{
			uint2 nodeid;
			if (!MCU_stoui2(data,nodeid))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRNode, kMCPlatformPropertyTypeUInt16, &nodeid);
		}
            break;
        case P_PAN:
		{
			real8 pan;
			if (!MCU_stor8(data, pan))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRPan, kMCPlatformPropertyTypeDouble, &pan);

			if (isbuffering())
				dirty = True;
		}
            break;
        case P_TILT:
		{
			real8 tilt;
			if (!MCU_stor8(data, tilt))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRTilt, kMCPlatformPropertyTypeDouble, &tilt);

			if (isbuffering())
				dirty = True;
		}
            break;
        case P_ZOOM:
		{
			real8 zoom;
			if (!MCU_stor8(data, zoom))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (m_platform_player != nil)
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyQTVRZoom, kMCPlatformPropertyTypeDouble, &zoom);

			if (isbuffering())
				dirty = True;
		}
            break;
        case P_VISIBLE:
        case P_INVISIBLE:
		{
			uint4 oldflags = flags;
			Exec_stat stat = MCControl::setprop(parid, p, ep, effective);
			if (flags != oldflags && !(flags & F_VISIBLE))
				playstop();
			if (m_platform_player != nil)
			{
				bool t_visible;
				t_visible = getflag(F_VISIBLE);
				MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVisible, kMCPlatformPropertyTypeBool, &t_visible);
			}
	
			return stat;
		}
            break;
#endif /* MCPlayer::setprop */
        default:
            return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty && opened && flags & F_VISIBLE)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
}

// MW-2011-09-23: Make sure we sync the buffer state at this point, rather than
//   during drawing.
void MCPlayer::select(void)
{
	MCControl::select();
	syncbuffering(nil);
}

// MW-2011-09-23: Make sure we sync the buffer state at this point, rather than
//   during drawing.
void MCPlayer::deselect(void)
{
	MCControl::deselect();
	syncbuffering(nil);
}

MCControl *MCPlayer::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCPlayer *newplayer = new MCPlayer(*this);
	if (attach)
		newplayer->attach(p, invisible);
	return newplayer;
}

IO_stat MCPlayer::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCPlayer::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	return defaultextendedload(p_stream, p_version, p_remaining);
}

IO_stat MCPlayer::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;
	if (!disposable)
	{
		if ((stat = IO_write_uint1(OT_PLAYER, stream)) != IO_NORMAL)
			return stat;
		if ((stat = MCControl::save(stream, p_part, p_force_ext)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_string(filename, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(starttime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(endtime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int4((int4)(rate / 10.0 * MAXINT4),
		                          stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_string(userCallbackStr, stream)) != IO_NORMAL)
			return stat;
	}
	return savepropsets(stream);
}

IO_stat MCPlayer::load(IO_handle stream, const char *version)
{
	IO_stat stat;
    
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_string(filename, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint4(&starttime, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint4(&endtime, stream)) != IO_NORMAL)
		return stat;
	int4 trate;
	if ((stat = IO_read_int4(&trate, stream)) != IO_NORMAL)
		return stat;
	rate = (real8)trate * 10.0 / MAXINT4;
	if ((stat = IO_read_string(userCallbackStr, stream)) != IO_NORMAL)
		return stat;
	return loadpropsets(stream);
}

// MW-2011-09-23: Ensures the buffering state is consistent with current flags
//   and state.
void MCPlayer::syncbuffering(MCContext *p_dc)
{
	bool t_should_buffer;
	
	// MW-2011-09-13: [[ Layers ]] If the layer is dynamic then the player must be buffered.
	t_should_buffer = getstate(CS_SELECTED) || getflag(F_ALWAYS_BUFFER) || getstack() -> getstate(CS_EFFECT) || (p_dc != nil && p_dc -> gettype() != CONTEXT_TYPE_SCREEN) || !MCModeMakeLocalWindows() || layer_issprite();
    
    // MW-2014-04-24: [[ Bug 12249 ]] If we are not in browse mode for this object, then it should be buffered.
    t_should_buffer = t_should_buffer || getstack() -> gettool(this) != T_BROWSE;
	
	if (m_platform_player != nil)
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_should_buffer);
}

// MW-2007-08-14: [[ Bug 1949 ]] On Windows ensure we load and unload QT if not
//   currently in use.
void MCPlayer::getversion(MCExecPoint &ep)
{
    extern void MCQTGetVersion(MCExecPoint& ep);
    MCQTGetVersion(ep);
}

void MCPlayer::freetmp()
{
	if (istmpfile)
	{
		MCS_unlink(filename);
		delete filename;
		filename = NULL;
	}
}

uint4 MCPlayer::getduration() //get movie duration/length
{
	uint4 duration;
	if (m_platform_player != nil)
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &duration);
	else
		duration = 0;
	return duration;
}

uint4 MCPlayer::gettimescale() //get moive time scale
{
	uint4 timescale;
	if (m_platform_player != nil)
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyTimescale, kMCPlatformPropertyTypeUInt32, &timescale);
	else
		timescale = 0;
	return timescale;
}

uint4 MCPlayer::getmoviecurtime()
{
	uint4 curtime;
	if (m_platform_player != nil)
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &curtime);
	else
		curtime = 0;
	return curtime;
}

void MCPlayer::setcurtime(uint4 newtime)
{
	lasttime = newtime;
	if (m_platform_player != nil)
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &newtime);
}

void MCPlayer::setselection()
{
	if (m_platform_player != nil)
	{
		uint4 st, et;
		if (starttime == MAXUINT4 || endtime == MAXUINT4)
			st = et = 0;
		else
		{
			st = starttime;
			et = endtime;
		}
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypeUInt32, &starttime);
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &endtime);
	}
}

void MCPlayer::setlooping(Boolean loop)
{
	if (m_platform_player != nil)
	{
		bool t_loop;
		t_loop = loop;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoop, kMCPlatformPropertyTypeBool, &t_loop);
	}
}

void MCPlayer::setplayrate()
{
	if (m_platform_player != nil)
	{
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyPlayRate, kMCPlatformPropertyTypeDouble, &rate);
		if (rate != 0.0f)
			MCPlatformStartPlayer(m_platform_player);
	}
    
	if (rate != 0)
		state = state & ~CS_PAUSED;
	else
		state = state | CS_PAUSED;
}

void MCPlayer::showbadge(Boolean show)
{
#if 0
	if (m_platform_player != nil)
	{
		bool t_show;
		t_show = show;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowBadge, kMCPlatformPropertyTypeBool, &t_show);
	}
#endif
}

void MCPlayer::editmovie(Boolean edit)
{
#if 0
	if (m_platform_player != nil)
	{
		bool t_edit;
		t_edit = edit;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowSelection, kMCPlatformPropertyTypeBool, &t_edit);
	}
#endif
}

void MCPlayer::playselection(Boolean play)
{
	if (m_platform_player != nil)
	{
		bool t_play;
		t_play = play;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOnlyPlaySelection, kMCPlatformPropertyTypeBool, &t_play);
	}
}

Boolean MCPlayer::ispaused()
{
	if (m_platform_player != nil)
		return !MCPlatformPlayerIsPlaying(m_platform_player);
}

void MCPlayer::showcontroller(Boolean show)
{
#if 0
	if (m_platform_player != nil)
	{
		bool t_show;
		t_show = show;
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyShowController, kMCPlatformPropertyTypeBool, &t_show);
	}
#endif
    
    // The showController property has changed, this means we must do two things - resize
    // the movie rect and then redraw ourselves to make sure we can see the controller.
    
    setrect(rect);
    layer_redrawall();
}

Boolean MCPlayer::prepare(const char *options)
{
	Boolean ok = False;
    
	if (state & CS_PREPARED || filename == NULL)
		return True;
	
	if (!opened)
		return False;
    
    extern bool MCQTInit(void);
    if (!MCQTInit())
        return False;
    
	if (m_platform_player == nil)
		MCPlatformCreatePlayer(m_platform_player);
    
	if (strnequal(filename, "https:", 6) || strnequal(filename, "http:", 5) || strnequal(filename, "ftp:", 4) || strnequal(filename, "file:", 5) || strnequal(filename, "rtsp:", 5))
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyURL, kMCPlatformPropertyTypeNativeCString, &filename);
	else
		MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFilename, kMCPlatformPropertyTypeNativeCString, &filename);
	
	MCRectangle t_movie_rect;
	MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
	
	MCRectangle trect = resize(t_movie_rect);
    
    // Adjust so that the controller isn't included in the movie rect.
    if (getflag(F_SHOW_CONTROLLER))
        trect . height -= CONTROLLER_HEIGHT;
	
	// IM-2011-11-12: [[ Bug 11320 ]] Transform player rect to device coords
    // MW-2014-04-09: [[ Bug 11922 ]] Make sure we use the view not device transform
    //   (backscale factor handled in platform layer).
	trect = MCRectangleGetTransformedBounds(trect, getstack()->getviewtransform());
	
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyRect, kMCPlatformPropertyTypeRectangle, &trect);
	
	bool t_looping, t_play_selection;
	
	t_looping = getflag(F_LOOPING);
	t_play_selection = getflag(F_PLAY_SELECTION);
	
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &lasttime);
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyLoop, kMCPlatformPropertyTypeBool, &t_looping);

	setselection();
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOnlyPlaySelection, kMCPlatformPropertyTypeBool, &t_play_selection);
	SynchronizeUserCallbacks();
	
	bool t_offscreen;
	t_offscreen = getflag(F_ALWAYS_BUFFER);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_offscreen);
	
	bool t_visible;
	t_visible = getflag(F_VISIBLE);
	MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVisible, kMCPlatformPropertyTypeBool, &t_visible);
	
	MCPlatformAttachPlayer(m_platform_player, getstack() -> getwindow());
	
	layer_redrawall();
	
	setloudness();
	
	MCresult -> clear(False);
	
	ok = True;
	
	if (ok)
	{
		state |= CS_PREPARED | CS_PAUSED;
		{
			nextplayer = MCplayers;
			MCplayers = this;
		}
	}
    
	return ok;
}

Boolean MCPlayer::playstart(const char *options)
{
	if (!prepare(options))
		return False;
	playpause(False);
	return True;
}

Boolean MCPlayer::playpause(Boolean on)
{
	if (!(state & CS_PREPARED))
		return False;
	
	Boolean ok;
	ok = False;
	
	if (m_platform_player != nil)
	{
		if (!on)
			MCPlatformStartPlayer(m_platform_player);
		else
			MCPlatformStopPlayer(m_platform_player);
		ok = True;
	}
	
	if (ok)
		setstate(on, CS_PAUSED);
    
	return ok;
}

void MCPlayer::playstepforward()
{
	if (!getstate(CS_PREPARED))
		return;
    
	if (m_platform_player != nil)
		MCPlatformStepPlayer(m_platform_player, 1);
}


void MCPlayer::playfastforward()
{
	if (!getstate(CS_PREPARED))
		return;
    
	if (m_platform_player != nil)
		MCPlatformFastForwardPlayer(m_platform_player);
}

void MCPlayer::playfastback()
{
	if (!getstate(CS_PREPARED))
		return;
    
	if (m_platform_player != nil)
		MCPlatformFastBackPlayer(m_platform_player);
}

void MCPlayer::playstepback()
{
	if (!getstate(CS_PREPARED))
		return;
	
	if (m_platform_player != nil)
		MCPlatformStepPlayer(m_platform_player, -1);
}

Boolean MCPlayer::playstop()
{
	formattedwidth = formattedheight = 0;
	if (!getstate(CS_PREPARED))
		return False;
    
	Boolean needmessage = True;
	
	state &= ~(CS_PREPARED | CS_PAUSED);
	lasttime = 0;
	
	if (m_platform_player != nil)
	{
		MCPlatformStopPlayer(m_platform_player);
		
		needmessage = getduration() > getmoviecurtime();
		
		MCPlatformDetachPlayer(m_platform_player);
	}
    
	freetmp();
    
	if (MCplayers != NULL)
	{
		if (MCplayers == this)
			MCplayers = nextplayer;
		else
		{
			MCPlayer *tptr = MCplayers;
			while (tptr->nextplayer != NULL && tptr->nextplayer != this)
				tptr = tptr->nextplayer;
			if (tptr->nextplayer == this)
                tptr->nextplayer = nextplayer;
		}
	}
	nextplayer = NULL;
    
	if (disposable)
	{
		if (needmessage)
			getcard()->message_with_args(MCM_play_stopped, getname());
		delete this;
	}
	else
		if (needmessage)
			message_with_args(MCM_play_stopped, getname());
    
	return True;
}


void MCPlayer::setfilename(const char *vcname,
                           char *fname, Boolean istmp)
{
	setname_cstring(vcname);
	filename = fname;
	istmpfile = istmp;
	disposable = True;
}

void MCPlayer::setvolume(uint2 tloudness)
{
}

MCRectangle MCPlayer::getpreferredrect()
{
	if (!getstate(CS_PREPARED))
	{
		MCRectangle t_bounds;
		MCU_set_rect(t_bounds, 0, 0, formattedwidth, formattedheight);
		return t_bounds;
	}
    
    MCRectangle t_bounds;
	MCU_set_rect(t_bounds, 0, 0, 0, 0);
	if (m_platform_player != nil)
    {
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_bounds);
        // PM-2014-04-28: [[Bug 12299]] Make sure the correct MCRectangle is returned
        return t_bounds;
    }
}

uint2 MCPlayer::getloudness()
{
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVolume, kMCPlatformPropertyTypeUInt16, &loudness);
	return loudness;
}

void MCPlayer::setloudness()
{
	if (state & CS_PREPARED)
		if (m_platform_player != nil)
			MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyVolume, kMCPlatformPropertyTypeUInt16, &loudness);
}

void MCPlayer::gettracks(MCExecPoint &ep)
{
	ep . clear();
    
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				uint32_t t_id;
				MCAutoPointer<char> t_name;
				uint32_t t_offset, t_duration;
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyMediaTypeName, kMCPlatformPropertyTypeNativeCString, &(&t_name));
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyOffset, kMCPlatformPropertyTypeUInt32, &t_offset);
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_offset);
				ep . concatuint(t_id, EC_RETURN, i == 1);
				ep . concatcstring(*t_name, EC_COMMA, false);
				ep . concatuint(t_offset, EC_COMMA, false);
				ep . concatuint(t_duration, EC_COMMA, false);
			}
		}
}

void MCPlayer::getenabledtracks(MCExecPoint &ep)
{
	ep.clear();
    
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				uint32_t t_id;
				uint32_t t_enabled;
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyId, kMCPlatformPropertyTypeUInt32, &t_id);
				MCPlatformGetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
				if (t_enabled)
					ep . concatuint(t_id, EC_RETURN, i == 1);
			}
		}
}

Boolean MCPlayer::setenabledtracks(const MCString &s)
{
	if (getstate(CS_PREPARED))
		if (m_platform_player != nil)
		{
			uindex_t t_track_count;
			MCPlatformCountPlayerTracks(m_platform_player, t_track_count);
			for(uindex_t i = 0; i < t_track_count; i++)
			{
				bool t_enabled;
				t_enabled = false;
				MCPlatformSetPlayerTrackProperty(m_platform_player, i, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
			}
			char *data = s.clone();
			char *sptr = data;
			while (*sptr)
			{
				char *tptr;
				if ((tptr = strchr(sptr, '\n')) != NULL)
					*tptr++ = '\0';
				else
					tptr = &sptr[strlen(sptr)];
				if (strlen(sptr) != 0)
				{
					uindex_t t_index;
					if (!MCPlatformFindPlayerTrackWithId(m_platform_player, strtol(sptr, NULL, 10), t_index))
					{
						delete data;
						return False;
					}
					
					bool t_enabled;
					t_enabled = true;
					MCPlatformSetPlayerTrackProperty(m_platform_player, t_index, kMCPlatformPlayerTrackPropertyEnabled, kMCPlatformPropertyTypeBool, &t_enabled);
				}
				sptr = tptr;
			}
			delete data;
			MCRectangle t_movie_rect;
			MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMovieRect, kMCPlatformPropertyTypeRectangle, &t_movie_rect);
			MCRectangle trect = resize(t_movie_rect);
			if (flags & F_SHOW_BORDER)
				trect = MCU_reduce_rect(trect, -borderwidth);
			setrect(trect);
		}
    
	return True;
}

void MCPlayer::getnodes(MCExecPoint &ep)
{
	ep.clear();
	// COCOA-TODO: MCPlayer::getnodes();
}

void MCPlayer::gethotspots(MCExecPoint &ep)
{
	ep.clear();
	// COCOA-TODO: MCPlayer::gethotspots();
}


MCRectangle MCPlayer::resize(MCRectangle movieRect)
{
	int2 x, y;
	MCRectangle trect = rect;
	
	// MW-2011-10-24: [[ Bug 9800 ]] Store the current rect for layer notification.
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	// MW-2011-10-01: [[ Bug 9762 ]] These got inverted sometime.
	formattedheight = movieRect.height;
	formattedwidth = movieRect.width;
	
	if (!(flags & F_LOCK_LOCATION))
	{
		if (formattedheight == 0)
		{ // audio clip
			trect.height = CONTROLLER_HEIGHT;
			rect = trect;
		}
		else
		{
			x = trect.x + (trect.width >> 1);
			y = trect.y + (trect.height >> 1);
			trect.width = (uint2)(formattedwidth * scale);
			trect.height = (uint2)(formattedheight * scale);
			if (flags & F_SHOW_CONTROLLER)
				trect.height += CONTROLLER_HEIGHT;
			trect.x = x - (trect.width >> 1);
			trect.y = y - (trect.height >> 1);
			if (flags & F_SHOW_BORDER)
				rect = MCU_reduce_rect(trect, -borderwidth);
			else
				rect = trect;
		}
	}
	else
		if (flags & F_SHOW_BORDER)
			trect = MCU_reduce_rect(trect, borderwidth);
	
	// MW-2011-10-24: [[ Bug 9800 ]] If the rect has changed, notify the layer.
	if (!MCU_equal_rect(rect, t_old_rect))
		layer_rectchanged(t_old_rect, true);
	
	return trect;
}

void MCPlayer::markerchanged(uint32_t p_time)
{
    // Search for the first marker with the given time, and dispatch the message.
    for(uindex_t i = 0; i < m_callback_count; i++)
        if (p_time == m_callbacks[i] . time)
            message_with_args(m_callbacks[i] . message, m_callbacks[i] . parameter);
}

void MCPlayer::selectionchanged(void)
{
    MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyStartTime, kMCPlatformPropertyTypeUInt32, &starttime);
    MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyFinishTime, kMCPlatformPropertyTypeUInt32, &endtime);
    timer(MCM_selection_changed, nil);
}

void MCPlayer::currenttimechanged(void)
{
    redrawcontroller();

    timer(MCM_current_time_changed, nil);
}

void MCPlayer::SynchronizeUserCallbacks(void)
{
    if (userCallbackStr == nil)
        return;
    
    if (m_platform_player == nil)
        return;
    
    // Free the existing callback table.
    for(uindex_t i = 0; i < m_callback_count; i++)
    {
        MCNameDelete(m_callbacks[i] . message);
        MCNameDelete(m_callbacks[i] . parameter);
    }
    MCMemoryDeleteArray(m_callbacks);
    m_callbacks = nil;
    m_callback_count = 0;
    
    // Now reparse the callback string and build the table.
    char *cblist = strclone(userCallbackStr);
	char *str;
	str = cblist;
	while (*str)
	{
		char *ptr, *data1, *data2;
		if ((data1 = strchr(str, ',')) == NULL)
		{
            //search ',' as separator
			delete cblist;
			return;
		}
		*data1 = '\0';
		data1 ++;
		if ((data2 = strchr(data1, '\n')) != NULL)// more than one callback
			*data2++ = '\0';
		else
			data2 = data1 + strlen(data1);
        
        /* UNCHECKED */ MCMemoryResizeArray(m_callback_count + 1, m_callbacks, m_callback_count);
        m_callbacks[m_callback_count - 1] . time = strtol(str, NULL, 10);
        
        while (isspace(*data1))//strip off preceding and trailing blanks
            data1++;
        ptr = data1;
        while (*ptr)
        {
            if (isspace(*ptr))
            {
                *ptr++ = '\0';
                /* UNCHECKED */ MCNameCreateWithCString(ptr, m_callbacks[m_callback_count - 1] . parameter);
                break;
            }
            ptr++;
        }
        
        /* UNCHECKED */ MCNameCreateWithCString(data1, m_callbacks[m_callback_count - 1] . message);
        
        // If no parameter is specified, use the time.
        if (m_callbacks[m_callback_count - 1] . parameter == nil)
        /* UNCHECKED */ MCNameCreateWithCString(str, m_callbacks[m_callback_count - 1] . parameter);
		
        str = data2;
	}
	delete cblist;
    
    // Now set the markers in the player so that we get notified.
    array_t<uint32_t> t_markers;
    /* UNCHECKED */ MCMemoryNewArray(m_callback_count, t_markers . ptr);
    for(uindex_t i = 0; i < m_callback_count; i++)
        t_markers . ptr[i] = m_callbacks[i] . time;
    t_markers . count = m_callback_count;
    MCPlatformSetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyMarkers, kMCPlatformPropertyTypeUInt32Array, &t_markers);
    MCMemoryDeleteArray(t_markers . ptr);
    
	return True;
}

Boolean MCPlayer::isbuffering(void)
{
	if (m_platform_player == nil)
		return false;
	
	bool t_buffering;
	MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_buffering);
	
	return t_buffering;
}


//-----------------------------------------------------------------------------
//  Redraw Management

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCPlayer::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;
    
	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}
        
		// MW-2009-06-11: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(false);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, rect))
				return;
			dirty = dc -> getclip();
		}
	}
    
	if (MClook == LF_MOTIF && state & CS_KFOCUSED && !(extraflags & EF_NO_FOCUS_BORDER))
		drawfocus(dc, p_dirty);
	
	if (m_platform_player != nil)
	{
		syncbuffering(dc);
		
		bool t_offscreen;
		MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyOffscreen, kMCPlatformPropertyTypeBool, &t_offscreen);
		
		if (t_offscreen)
		{
			MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
			
			MCImageDescriptor t_image;
			MCMemoryClear(&t_image, sizeof(t_image));
			t_image.filter = kMCGImageFilterNone;
			MCPlatformLockPlayerBitmap(m_platform_player, t_image . bitmap);
			if (t_image . bitmap != nil)
				dc -> drawimage(t_image, 0, 0, trect.width, trect.height, trect.x, trect.y);
			MCPlatformUnlockPlayerBitmap(m_platform_player, t_image . bitmap);
		}
	}
    
    // Draw our controller
    if (getflag(F_SHOW_CONTROLLER))
        drawcontroller(dc);
    
	if (getflag(F_SHOW_BORDER))
		if (getflag(F_3D))
			draw3d(dc, rect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	
	if (!p_isolated)
	{
		if (getstate(CS_SELECTED))
			drawselected(dc);
	}
    
	if (!p_isolated)
		dc -> end();
}

void MCPlayer::drawcontroller(MCDC *dc)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolume));
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartPlay));
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartWell));

    
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartThumb));
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubBack));
    drawcontrollerbutton(dc, getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubForward));

    
    
}

int MCPlayer::hittestcontroller(int x, int y)
{
    MCRectangle t_rect;
    t_rect = getcontrollerrect();
    
    if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartPlay), x, y))
        return kMCPlayerControllerPartPlay;
    
    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartVolume), x, y))
        return kMCPlayerControllerPartVolume;

    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartWell), x, y))
        return kMCPlayerControllerPartWell;

    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubBack), x, y))
        return kMCPlayerControllerPartScrubBack;

    else if (MCU_point_in_rect(getcontrollerpartrect(t_rect, kMCPlayerControllerPartScrubForward), x, y))
        return kMCPlayerControllerPartScrubForward;

    else
        return kMCPlayerControllerPartUnknown;
}

void MCPlayer::drawcontrollerbutton(MCDC *dc, const MCRectangle& p_rect)
{
    dc -> setforeground(dc -> getwhite());
    dc -> fillrect(p_rect, true);
    
    dc -> setforeground(dc -> getblack());
    dc -> setlineatts(1, LineSolid, CapButt, JoinMiter);
    
    dc -> drawrect(p_rect, true);
}

MCRectangle MCPlayer::getcontrollerrect(void)
{
    MCRectangle t_rect;
    t_rect = rect;
    
    if (getflag(F_SHOW_BORDER))
        t_rect = MCU_reduce_rect(t_rect, borderwidth);
    
    t_rect . y = t_rect . y + t_rect . height - CONTROLLER_HEIGHT;
    t_rect . height = CONTROLLER_HEIGHT;
    
    return t_rect;
}

MCRectangle MCPlayer::getcontrollerpartrect(const MCRectangle& p_rect, int p_part)
{
    switch(p_part)
    {
        case kMCPlayerControllerPartVolume:
            return MCRectangleMake(p_rect . x, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
        case kMCPlayerControllerPartPlay:
            return MCRectangleMake(p_rect . x + CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartThumb:
        {
            if (m_platform_player == nil)
                return MCRectangleMake(0, 0, 0, 0);
            
            uint32_t t_current_time, t_duration;
            MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &t_current_time);
            MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
            
            MCRectangle t_well_rect;
            t_well_rect = getcontrollerpartrect(p_rect, kMCPlayerControllerPartWell);
            
            int t_active_well_width;
            t_active_well_width = t_well_rect . width - CONTROLLER_HEIGHT;
            
            int t_thumb_left;
            t_thumb_left = t_active_well_width * t_current_time / t_duration;
            
            return MCRectangleMake(t_well_rect . x + t_thumb_left, t_well_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
        }
        break;
        
        case kMCPlayerControllerPartWell:
            return MCRectangleMake(p_rect . x + 2 * CONTROLLER_HEIGHT, p_rect . y, p_rect . width - 4 * CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
          
        case kMCPlayerControllerPartScrubBack:
            return MCRectangleMake(p_rect . x + p_rect . width - 2 * CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
            
        case kMCPlayerControllerPartScrubForward:
            return MCRectangleMake(p_rect . x + p_rect . width - CONTROLLER_HEIGHT, p_rect . y, CONTROLLER_HEIGHT, CONTROLLER_HEIGHT);
        default:
            break;
    }
    
    return MCRectangleMake(0, 0, 0, 0);
}

void MCPlayer::redrawcontroller(void)
{
    if (!getflag(F_SHOW_CONTROLLER))
        return;
    
    layer_redrawrect(getcontrollerrect());
}

void MCPlayer::handle_mdown(int p_which)
{
    switch(hittestcontroller(mx, my))
    {
        case kMCPlayerControllerPartPlay:
            if (getstate(CS_PREPARED))
            {
                playpause(!ispaused());
            }
            else
                playstart(nil);
            break;
        case kMCPlayerControllerPartVolume:
            break;
        case kMCPlayerControllerPartWell:
        {
            MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
            
            uint32_t t_new_time, t_duration;
            MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
            
            t_new_time = (mx - t_part_well_rect . x) * t_duration / t_part_well_rect . width;
            setcurtime(t_new_time);
        }
            break;
        case kMCPlayerControllerPartScrubBack:
        
            MCscreen->addtimer(this, MCM_internal, MCblinkrate);
            
            if(ispaused())
                playstepback();
            else
            {
                playstepback();
                playpause(!ispaused());
            }
            
            break;
        case kMCPlayerControllerPartScrubForward:
            
            MCscreen->addtimer(this, MCM_internal, MCblinkrate);
            
            if(ispaused())
                playstepforward();
            else
            {
                playstepforward();
                playpause(!ispaused());
            }
            
            break;
            
        default:
            break;
    }
}

void MCPlayer::handle_mfocus(int x, int y)
{
    if (state & CS_MFOCUSED)
    {
        switch(hittestcontroller(mx, my))
        {
            case kMCPlayerControllerPartWell:
            {
                MCRectangle t_part_well_rect = getcontrollerpartrect(getcontrollerrect(), kMCPlayerControllerPartWell);
                
                uint32_t t_new_time, t_duration;
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
                
                t_new_time = (mx - t_part_well_rect . x) * t_duration / t_part_well_rect . width;
                setcurtime(t_new_time);
            }
                break;
                
            default:
                break;
        }
    }
}

void MCPlayer::handle_mstilldown(int p_which)
{
    switch(hittestcontroller(mx, my))
    {
        case kMCPlayerControllerPartScrubForward:
        {
            uint32_t t_current_time, t_duration;
            Boolean t_was_paused;
            t_was_paused = ispaused();
            while (mdown(p_which))
            {
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &t_current_time);
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
                
                if (t_current_time > t_duration)
                    t_current_time = t_duration;
                
                playfastforward();
            }
            playpause(t_was_paused);
        }
            break;
            
        case kMCPlayerControllerPartScrubBack:
        {
            uint32_t t_current_time, t_duration;
            Boolean t_was_paused;
            t_was_paused = ispaused();
            while (mdown(p_which))
            {
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &t_current_time);
                MCPlatformGetPlayerProperty(m_platform_player, kMCPlatformPlayerPropertyDuration, kMCPlatformPropertyTypeUInt32, &t_duration);
                
                if (t_current_time < 0.0)
                    t_current_time = 0.0;
                
                playfastback();
            }
            
            playpause(t_was_paused);
        }
            break;
            
        default:
            break;
    }
    
    
}

void MCPlayer::handle_mup(int p_which)
{
}