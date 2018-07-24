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
#include "mcio.h"
#include "sysdefs.h"
#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "player.h"
#include "exec-interface.h"
#include "filepath.h"

#include "osspec.h"

#include "player.h"

////////////////////////////////////////////////////////////////////////////////

static void MCMultimediaQTVRConstraintsParse(MCExecContext& ctxt, MCStringRef p_input, MCMultimediaTrackList& r_output)
{
}

static void MCMultimediaQTVRConstraintsFormat(MCExecContext& ctxt, const MCMultimediaQTVRConstraints& p_input, MCStringRef& r_output)
{
	if (MCStringFormat(r_output, "%f,%f\n%f,%f\n%f,%f", p_input . minpan, p_input . maxpan,
						p_input . mintilt, p_input . maxtilt, p_input . minzoom, p_input . maxzoom))
		return;

	ctxt . Throw();
}

static void MCMultimediaQTVRConstraintsFree(MCExecContext& ctxt, MCMultimediaTrackList& p_input)
{
}

static MCExecCustomTypeInfo _kMCMultimediaQTVRConstraintsTypeInfo =
{
	"Multimedia.QTVRConstraints",
	sizeof(MCMultimediaQTVRConstraints),
	(void *)MCMultimediaQTVRConstraintsParse,
	(void *)MCMultimediaQTVRConstraintsFormat,
	(void *)MCMultimediaQTVRConstraintsFree
};

//////////

static void MCMultimediaTrackFormat(MCExecContext& ctxt, const MCMultimediaTrack& p_input, MCStringRef& r_output)
{
	if (MCStringFormat(r_output, "%d,%@,%d,%d", p_input . id, p_input . name,
									  p_input . offset, p_input . duration))
		return;

	ctxt . Throw();
}

static void MCMultimediaTrackParse(MCExecContext& ctxt, MCStringRef p_input, MCMultimediaTrack& r_output)
{
}

static void MCMultimediaTrackFree(MCExecContext& ctxt, MCMultimediaTrack& p_input)
{
	MCValueRelease(p_input . name);
}

static MCExecCustomTypeInfo _kMCMultimediaTrackTypeInfo =
{
	"Multimedia.Track",
	sizeof(MCMultimediaTrack),
	(void *)MCMultimediaTrackParse,
	(void *)MCMultimediaTrackFormat,
	(void *)MCMultimediaTrackFree
};

//////////

static void MCMultimediaQTVRNodeFormat(MCExecContext& ctxt, const MCMultimediaQTVRNode& p_input, MCStringRef& r_output)
{
	if (MCStringFormat(r_output, "%d,%s", p_input . id, p_input . type == kMCQTVRNodePanoramaType ? "panorama" : "object"))
		return;	

	ctxt . Throw();
}

static void MCMultimediaQTVRNodeParse(MCExecContext& ctxt, MCStringRef p_input, MCMultimediaQTVRNode& r_output)
{
}

static void MCMultimediaQTVRNodeFree(MCExecContext& ctxt, MCMultimediaQTVRNode& p_input)
{
}

static MCExecCustomTypeInfo _kMCMultimediaQTVRNodeTypeInfo =
{
	"Multimedia.QTVRNode",
	sizeof(MCMultimediaQTVRNode),
	(void *)MCMultimediaQTVRNodeParse,
	(void *)MCMultimediaQTVRNodeFormat,
	(void *)MCMultimediaQTVRNodeFree
};

//////////

static void MCMultimediaQTVRHotSpotFormat(MCExecContext& ctxt, const MCMultimediaQTVRHotSpot& p_input, MCStringRef& r_output)
{
	switch (p_input . type)
	{
	case kMCQTVRHotSpotLinkType:
		if (MCStringFormat(r_output, "%d,%s", p_input . id, "link"))
			return;
		break;
	case kMCQTVRHotSpotURLType:
		if (MCStringFormat(r_output, "%d,%s", p_input . id, "url"))
			return;
		break;
	case kMCQTVRHotSpotUndefinedType:
		if (MCStringFormat(r_output, "%d,%s", p_input . id, "undefined"))
			return;
		break;
	}

	ctxt . Throw();
}

static void MCMultimediaQTVRHotSpotParse(MCExecContext& ctxt, MCStringRef p_input, MCMultimediaQTVRHotSpot& r_output)
{
}

static void MCMultimediaQTVRHotSpotFree(MCExecContext& ctxt, MCMultimediaQTVRHotSpot& p_input)
{
}

static MCExecCustomTypeInfo _kMCMultimediaQTVRHotSpotTypeInfo =
{
	"Multimedia.QTVRHotSpot",
	sizeof(MCMultimediaQTVRHotSpot),
	(void *)MCMultimediaQTVRHotSpotParse,
	(void *)MCMultimediaQTVRHotSpotFormat,
	(void *)MCMultimediaQTVRHotSpotFree
};

//////////

static MCExecSetTypeElementInfo _kMCInterfaceMediaTypesElementInfo[] =
{	
	{ "video", PLAYER_MEDIA_TYPE_VIDEO_BIT },
	{ "audio", PLAYER_MEDIA_TYPE_AUDIO_BIT },
	{ "text", PLAYER_MEDIA_TYPE_TEXT_BIT },
	{ "qtvr", PLAYER_MEDIA_TYPE_QTVR_BIT },
	{ "sprite", PLAYER_MEDIA_TYPE_SPRITE_BIT },
	{ "flash", PLAYER_MEDIA_TYPE_FLASH_BIT },
};

static MCExecSetTypeInfo _kMCInterfaceMediaTypesTypeInfo =
{
	"Interface.MediaTypes",
	sizeof(_kMCInterfaceMediaTypesElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCInterfaceMediaTypesElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfacePlayerStatusElementInfo[] =
{
    { "", kMCInterfacePlayerStatusNone, true },
	{ "loading", kMCInterfacePlayerStatusLoading, true },
	{ "playing", kMCInterfacePlayerStatusPlaying, true },
	{ "paused", kMCInterfacePlayerStatusPaused, true },
};

static MCExecEnumTypeInfo _kMCInterfacePlayerStatusTypeInfo =
{
	"Interface.PlayerStatus",
	sizeof(_kMCInterfacePlayerStatusElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfacePlayerStatusElementInfo
};

//////////

MCExecEnumTypeInfo *kMCInterfacePlayerStatusTypeInfo = &_kMCInterfacePlayerStatusTypeInfo;
MCExecSetTypeInfo *kMCInterfaceMediaTypesTypeInfo = &_kMCInterfaceMediaTypesTypeInfo;
MCExecCustomTypeInfo *kMCMultimediaTrackTypeInfo = &_kMCMultimediaTrackTypeInfo;
MCExecCustomTypeInfo *kMCMultimediaQTVRConstraintsTypeInfo = &_kMCMultimediaQTVRConstraintsTypeInfo;
MCExecCustomTypeInfo *kMCMultimediaQTVRNodeTypeInfo = &_kMCMultimediaQTVRNodeTypeInfo;
MCExecCustomTypeInfo *kMCMultimediaQTVRHotSpotTypeInfo = &_kMCMultimediaQTVRHotSpotTypeInfo;

void copy_custom_list_as_string_and_release(MCExecContext& ctxt, MCExecCustomTypeInfo *p_type, void *p_elements, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(p_delimiter, &t_list))
		goto throw_error;

	for(uindex_t i = 0; i < p_count; i++)
	{
		MCAutoStringRef t_element_as_string;
		((void(*)(MCExecContext&, void *, MCStringRef&))p_type -> format)(ctxt, (byte_t *)p_elements + p_type -> size * i, &t_element_as_string);
		if (ctxt . HasError())
			return;

		if (!MCListAppend(*t_list, *t_element_as_string))
			goto throw_error;

		((void(*)(MCExecContext&, void *))p_type -> free)(ctxt,(byte_t*)p_elements + p_type -> size * i);

		if (ctxt . HasError())
			return;
	}

	if (!MCListCopyAsString(*t_list, r_string))
		goto throw_error;
	
	return;

throw_error:
	ctxt . Throw();
	return;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlayer::Redraw(void)
{
	if (!opened || !(flags & F_VISIBLE))
		return;

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

////////////////////////////////////////////////////////////////////////////////

static bool MCPathIsURL(MCStringRef p_path)
{
    return MCStringBeginsWithCString(p_path, (char_t*)"http://", kMCStringOptionCompareCaseless) ||
            MCStringBeginsWithCString(p_path, (char_t*)"https://", kMCStringOptionCompareCaseless) ||
            MCStringBeginsWithCString(p_path, (char_t*)"ftp://", kMCStringOptionCompareCaseless)||
            // PM-2015-06-30: [[ Bug 14418 ]] Allow URLs of the form file://
            MCStringBeginsWithCString(p_path, (char_t*)"file://", kMCStringOptionCompareCaseless);
}

// PM-2014-12-19: [[ Bug 14245 ]] Make possible to set the filename using a relative path to the stack folder
// PM-2015-01-26: [[ Bug 14435 ]] Make possible to set the filename using a relative path to the default folder
bool MCPlayer::resolveplayerfilename(MCStringRef p_filename, MCStringRef &r_filename)
{
    if (MCPathIsAbsolute(p_filename) || MCPathIsURL(p_filename))
    {
        r_filename = MCValueRetain(p_filename);
        return true;
    }

	{
		MCAutoStringRef t_filename;
		bool t_relative_to_stack = getstack()->resolve_relative_path(p_filename, &t_filename);
		if (t_relative_to_stack && MCS_exists(*t_filename, True))
			return MCStringCopy(*t_filename, r_filename);
	}

	{
		MCAutoStringRef t_filename;
		bool t_relative_to_default_folder = getstack()->resolve_relative_path_to_default_folder(p_filename, &t_filename);
		if (t_relative_to_default_folder && MCS_exists(*t_filename, True))
			return MCStringCopy(*t_filename, r_filename);
	}

    return false;
}

void MCPlayer::GetFileName(MCExecContext& ctxt, MCStringRef& r_name)
{
	if (filename == nil)
		return;
	
	r_name = MCValueRetain(filename);
}

void MCPlayer::SetFileName(MCExecContext& ctxt, MCStringRef p_name)
{
    // Edge case: Suppose filenameA is a valid relative path to defaultFolderA,
    //  but invalid relative path to defaultFolderB
    //    1. Set defaultFolder to defaultFolderB. Set the filename to filenameA.
    //       Video will become empty, since the relative path is invalid.
    //    2. Change the defaultFolder to defaultFolderA. Set the filename again
    //       to filenameA. Now the relative path is valid
    MCAutoStringRef t_string;
    bool t_resolved_path;
    t_resolved_path = false;

    t_resolved_path = resolveplayerfilename(p_name, &t_string);

    // handle the edge case mentioned below: If t_resolved_path then the movie path has to be updated
    // PM-2015-04-14: [[ Bug 15196 ]] Allow setting the player filename to empty more than once
	if (filename == nil || MCStringIsEmpty(p_name) ||
        !MCStringIsEqualTo(p_name, filename, kMCCompareExact) || t_resolved_path)
	{
		MCValueRelease(filename);
		filename = NULL;
		playstop();
		starttime = MAXUINT4; //clears the selection
		endtime = MAXUINT4;

        // PM-2015-01-26: [[ Bug 14435 ]] Resolve the filename in MCPlayer::prepare(),
        //  to avoid prepending the defaultFolder or the stack folder to the filename property
		if (p_name != nil)
            filename = MCValueRetain(p_name);
		prepare(kMCEmptyString);

#ifdef FEATURE_PLATFORM_PLAYER
		attachplayer();
#endif

		Redraw();
    }
#ifdef FEATURE_PLATFORM_PLAYER
    // PM-2014-12-22: [[ Bug 14232 ]] Update the result in case a an invalid/corrupted filename is set more than once in a row
    else if (MCStringIsEqualTo(p_name, filename, kMCStringOptionCompareCaseless) && (hasinvalidfilename() || !t_resolved_path))
        ctxt . SetTheResultToCString("could not create movie reference");
#endif

}

void MCPlayer::GetDontRefresh(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DONT_REFRESH);
}

void MCPlayer::SetDontRefresh(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_DONT_REFRESH))
		Redraw();
}

void MCPlayer::GetCurrentTime(MCExecContext& ctxt, double& r_time)
{
	r_time = getmoviecurtime();
}

void MCPlayer::SetCurrentTime(MCExecContext& ctxt, double p_time)
{
	setcurtime(p_time, false);
	if (isbuffering())
		Redraw();
    // AL-2014-11-17: [[ Bug 13954 ]] Redraw the player controller when current time is set
#ifdef FEATURE_PLATFORM_PLAYER
    else
        redrawcontroller();
#endif
}

void MCPlayer::GetDuration(MCExecContext& ctxt, double& r_duration)
{
	r_duration = getduration();
}

// PM-2014-11-03: [[ Bug 13920 ]] Make sure we support loadedTime property
void MCPlayer::GetLoadedTime(MCExecContext& ctxt, double& r_loaded_time)
{
#ifdef FEATURE_PLATFORM_PLAYER
	r_loaded_time = getmovieloadedtime();
#else
    r_loaded_time = 0;
#endif
}

void MCPlayer::GetLooping(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LOOPING);
}

void MCPlayer::SetLooping(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_LOOPING))
	{
		setlooping((flags & F_LOOPING) != 0); //set/unset movie looping
		Redraw();
	}
}

void MCPlayer::GetPaused(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = ispaused() == True;
}

void MCPlayer::SetPaused(MCExecContext& ctxt, bool setting)
{
	playpause(setting); //pause or unpause the player
}

void MCPlayer::GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_ALWAYS_BUFFER);
}

void MCPlayer::SetAlwaysBuffer(MCExecContext& ctxt, bool setting)
{
	// The actual buffering state is determined upon redrawing - therefore
	// we trigger a redraw to ensure we don't unbuffer when it is 
	// needed.

	if (changeflag(setting, F_ALWAYS_BUFFER) || opened)
		Redraw();
}

void MCPlayer::GetPlayRate(MCExecContext& ctxt, double& r_rate)
{
	r_rate = rate;
}

void MCPlayer::SetPlayRate(MCExecContext& ctxt, double p_rate)
{
	rate = p_rate;
	setplayrate();
}

void MCPlayer::GetStartTime(MCExecContext& ctxt, double*& r_time)
{
	if (starttime == MAXUINT4)
		r_time = nil;
	else
		*r_time = starttime; //for QT, this is the selection start time
}

void MCPlayer::SetStartTime(MCExecContext& ctxt, double* p_time)
{
	//for QT, this is the selection start time
	if (p_time == nil)
		starttime = endtime = MAXUINT4;
	else
	{
		starttime = *p_time;
#ifndef _MOBILE
		if (endtime == MAXUINT4) //if endtime is not set, set it to the length of movie
			endtime = getduration();
		else
			if (starttime > endtime)
				endtime = starttime;
#endif
	}
	setselection(false);
}

void MCPlayer::GetEndTime(MCExecContext& ctxt, double*& r_time)
{
	if (endtime == MAXUINT4)
		r_time = nil;
	else
		*r_time = endtime; //for QT, this is the selection's end time
}

void MCPlayer::SetEndTime(MCExecContext& ctxt, double* p_time)
{
	//for QT, this is the selection end time
	if (p_time == nil)
		starttime = endtime = MAXUINT4;
    
	else
	{
        endtime = *p_time >= getduration() ? getduration() : *p_time;
#ifndef _MOBILE
		if (starttime == MAXUINT4)
			starttime = 0;
		else
			if (starttime > endtime)
				starttime = endtime;
#endif
	}
	setselection(false);
}

void MCPlayer::GetShowBadge(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_BADGE);
}

void MCPlayer::SetShowBadge(MCExecContext& ctxt, bool setting)
{
	//if in the buffering mode we do not want to show/hide the badge
	if (!(flags & F_ALWAYS_BUFFER))
	{ //if always buffer flag is not set
		if (changeflag(setting, F_SHOW_BADGE))
		{
			if (!isbuffering()) //we are not actually buffering, let's show/hide the badge
				showbadge((flags & F_SHOW_BADGE) != 0); //show/hide movie's badge
			Redraw();
		}
	}
}

void MCPlayer::GetShowController(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_CONTROLLER);
}

void MCPlayer::SetShowController(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_CONTROLLER))
		showcontroller((flags & F_VISIBLE) != 0 && (flags & F_SHOW_CONTROLLER) != 0);
}

void MCPlayer::GetPlaySelection(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_PLAY_SELECTION);
}

void MCPlayer::SetPlaySelection(MCExecContext& ctxt, bool setting)
{
	//make QT movie plays only the selected part
	if (changeflag(setting, F_PLAY_SELECTION))
		playselection((flags & F_PLAY_SELECTION) != 0);
}

void MCPlayer::GetShowSelection(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_SELECTION);
}

void MCPlayer::SetShowSelection(MCExecContext& ctxt, bool setting)
{
	//means make QT movie editable
	if (changeflag(setting, F_SHOW_SELECTION))
		editmovie((flags & F_SHOW_SELECTION) != 0);
}

void MCPlayer::GetCallbacks(MCExecContext& ctxt, MCStringRef& r_callbacks)
{
	r_callbacks = MCValueRetain(userCallbackStr);
}

void MCPlayer::SetCallbacks(MCExecContext& ctxt, MCStringRef p_callbacks)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_CALLBACKS property refactored in the MCPlayer implementations
    setcallbacks(p_callbacks);
}

void MCPlayer::GetTimeScale(MCExecContext& ctxt, double& r_scale)
{
	r_scale = gettimescale();
}

void MCPlayer::GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height)
{
	r_height = getpreferredrect() . height;
}

void MCPlayer::GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width)
{
	r_width = getpreferredrect() . width;
}

void MCPlayer::GetMovieControllerId(MCExecContext& ctxt, integer_t& r_id)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_MOVIE_CONTROLLER_ID property refactor in the MCPlayer implementations
    r_id = getmoviecontrollerid();
}

void MCPlayer::SetMovieControllerId(MCExecContext& ctxt, integer_t p_id)
{
	setmoviecontrollerid(p_id);
}

void MCPlayer::GetPlayLoudness(MCExecContext& ctxt, uinteger_t& r_loudness)
{
	r_loudness = getloudness();
}

void MCPlayer::SetPlayLoudness(MCExecContext& ctxt, uinteger_t p_loudness)
{
	loudness = MCU_min(p_loudness, (uint4)100);
	setloudness();
}

void MCPlayer::GetLeftBalance(MCExecContext &ctxt, double &r_left_balance)
{
	r_left_balance = getleftbalance();
}

void MCPlayer::SetLeftBalance(MCExecContext &ctxt, double p_left_balance)
{
	setleftbalance(MCMin(p_left_balance, 100.0));
}

void MCPlayer::GetRightBalance(MCExecContext &ctxt, double &r_right_balance)
{
	r_right_balance = getrightbalance();
}

void MCPlayer::SetRightBalance(MCExecContext &ctxt, double p_right_balance)
{
	setrightbalance(MCMin(p_right_balance, 100.0));
}

void MCPlayer::GetAudioPan(MCExecContext &ctxt, double &r_pan)
{
	r_pan = getaudiopan();
}

void MCPlayer::SetAudioPan(MCExecContext &ctxt, double p_pan)
{
	setaudiopan(MCClamp(p_pan, -100.0, 100.0));
}

void MCPlayer::GetTrackCount(MCExecContext& ctxt, uinteger_t& r_count)
{
	r_count = gettrackcount();
}

void MCPlayer::GetMediaTypes(MCExecContext& ctxt, intset_t& r_types)
{
	MCPlayerMediaTypeSet types;
	types = getmediatypes();
	r_types = (intenum_t)types;
}

void MCPlayer::GetCurrentNode(MCExecContext& ctxt, uinteger_t& r_node)
{
	r_node = getcurrentnode();
}

void MCPlayer::SetCurrentNode(MCExecContext& ctxt, uinteger_t p_node)
{
	if (changecurrentnode(p_node))
		Redraw();
}

void MCPlayer::GetPan(MCExecContext& ctxt, double& r_pan)
{
	r_pan = getpan();
}

void MCPlayer::SetPan(MCExecContext& ctxt, double p_pan)
{
	if (changepan(p_pan))
		Redraw();
}

void MCPlayer::GetTilt(MCExecContext& ctxt, double& r_tilt)
{
	r_tilt = gettilt();
}

void MCPlayer::SetTilt(MCExecContext& ctxt, double p_tilt)
{
	if (changetilt(p_tilt))
		Redraw();
}

void MCPlayer::GetZoom(MCExecContext& ctxt, double& r_zoom)
{
	r_zoom = getzoom();
}

void MCPlayer::SetZoom(MCExecContext& ctxt, double p_zoom)
{
	if (changezoom(p_zoom))
		Redraw();
}

void MCPlayer::GetTracks(MCExecContext& ctxt, MCStringRef& r_tracks)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_TRACKS getter refactored to the MCPlayer implementation
    gettracks(r_tracks);
}

void MCPlayer::GetConstraints(MCExecContext& ctxt, MCMultimediaQTVRConstraints& r_constraints)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_CONSTRAITNS getter refactored to the MCPlayer implementations
    getconstraints(r_constraints);
}

void MCPlayer::GetNodes(MCExecContext& ctxt, MCStringRef& r_nodes)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_NODES getter refactored to the MCPlayer implementations
    getnodes(r_nodes);
}
	
void MCPlayer::GetHotSpots(MCExecContext& ctxt, MCStringRef& r_spots)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_HOTSPOTS getter refactored to the MCPlayer implementations
    gethotspots(r_spots);
}

void MCPlayer::SetShowBorder(MCExecContext& ctxt, bool setting)
{
    MCControl::SetShowBorder(ctxt, setting);
    setrect(rect);
    Redraw();
}

void MCPlayer::SetBorderWidth(MCExecContext& ctxt, uinteger_t width)
{
    MCObject::SetBorderWidth(ctxt, width);
    setrect(rect);
    Redraw();
}

void MCPlayer::SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting)
{
#ifndef FEATURE_PLATFORM_PLAYER
    uint4 oldflags = flags;
#endif
	MCControl::SetVisible(ctxt, part, setting);
    
    // PM-2015-07-01: [[ Bug 15191 ]] Keep the LC 6.7 behaviour in non-platform player, to make the video layer to hide 
#ifndef FEATURE_PLATFORM_PLAYER
    if (flags != oldflags && !(flags & F_VISIBLE))
        playstop();
#endif
    
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_VISIBLE getter refactored to the MCPlayer implementations
    updatevisibility();
}

void MCPlayer::SetTraversalOn(MCExecContext& ctxt, bool setting)
{
    MCObject::SetTraversalOn(ctxt, setting);
    
    if (!ctxt . HasError())
    {
        // SN-2014-07-03: [[ PlatformPlayer ]]
        // P_TRAVERSAL_ON needs an update on a QuickTime player 
        updatetraversal();
    }
}

void MCPlayer::GetEnabledTracks(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tracks)
{
    // SN-2014-07-03: [[ PlatformPlayer ]]
    // P_ENABLED_TRACKS getter refactored to the MCPlayer implementations
    getenabledtracks(r_count, r_tracks);
}

void MCPlayer::SetEnabledTracks(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_tracks)
{
    // PM-2015-06-01: [[ PlatformPlayer ]]
    // P_ENABLED_TRACKS setter refactored to the MCPlayer implementations
    setenabledtracks(p_count, p_tracks);
}

#ifdef FEATURE_PLATFORM_PLAYER
void MCPlayer::GetStatus(MCExecContext& ctxt, intenum_t& r_status)
{
    if(getmovieloadedtime() != 0 && getmovieloadedtime() < getduration())
        r_status = kMCInterfacePlayerStatusLoading;
    else if (!ispaused())
        r_status = kMCInterfacePlayerStatusPlaying;
    else if (ispaused())
        r_status = kMCInterfacePlayerStatusPaused;
    else
        r_status = kMCInterfacePlayerStatusNone;
    
}

void MCPlayer::GetMirrored(MCExecContext &ctxt, bool &r_mirrored)
{
    r_mirrored = getflag(F_MIRRORED);
}

void MCPlayer::SetMirrored(MCExecContext &ctxt, bool p_mirrored)
{
    bool t_dirty;
    t_dirty = changeflag(p_mirrored, F_MIRRORED);
    
    if (t_dirty)
        setmirrored((flags & F_MIRRORED) != 0); //set/unset mirrored player
}

#endif

void MCPlayer::SetDontUseQT(MCExecContext &ctxt, bool p_dont_use_qt)
{
    setdontuseqt(p_dont_use_qt);
}

void MCPlayer::GetDontUseQT(MCExecContext &ctxt, bool &r_dont_use_qt)
{
    getdontuseqt(r_dont_use_qt);
}
