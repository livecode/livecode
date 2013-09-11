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
#include "player.h"
#include "aclip.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "mode.h"
#include "context.h"
#include "osspec.h"
#include "redraw.h"

#ifdef _WINDOWS_DESKTOP
#include "w32prefix.h"
#include "w32dc.h"
#include "w32context.h"

#include "digitalv.h"
#include "QTML.h"
#include <Movies.h>
#include <MediaHandlers.h>
#include <QuickTimeVR.h>
#include <QuickTimeVRFormat.h>
#include <Endian.h>
#include <QuickTimeComponents.h>
#include <ImageCodec.h>

#define PIXEL_FORMAT_32 k32BGRAPixelFormat

#elif defined(_MAC_DESKTOP)
#include "osxprefix.h"

#ifdef __LITTLE_ENDIAN__
#define PIXEL_FORMAT_32 k32BGRAPixelFormat
#else
#define PIXEL_FORMAT_32 k32ARGBPixelFormat
#endif

#elif defined(_LINUX_DESKTOP)
#include "lnxmplayer.h"
#endif

#undef X11
#ifdef TARGET_PLATFORM_LINUX
#define X11 
#endif

#ifdef FEATURE_QUICKTIME
#ifndef _MACOSX
#include "FixMath.h"
#include "Gestalt.h"
#include "TextUtils.h"
#endif

#ifdef _WINDOWS

struct MCPlayerOffscreenBuffer
{
	HDC hdc;
	HBITMAP hbitmap;
	MCImageBitmap image_bitmap;
};

PixMapHandle GetPortPixMap(CGrafPtr port)
{
	return port->portPixMap;
}

#ifndef HiWord
#define HiWord HIWORD
#endif

#ifndef LoWord
#define LoWord LOWORD
#endif

static OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec);
#endif

#define QTMFORMATS 6
static const OSType qtmediatypes[] =
{
	VisualMediaCharacteristic,
	AudioMediaCharacteristic,
	TextMediaType,
	kQTVRQTVRType,
	SpriteMediaType,
	FlashMediaType
};

static const char *qtmediastrings[] =
{
	"video",
	"audio",
	"text",
	"qtvr",
	"sprite",
	"flash"
};

static Boolean QTMovieHasType(Movie tmovie, OSType movtype);
static pascal void userMovieCallbacks(QTCallBack mcb, long index);
static pascal void MovieEndCallback(QTCallBack mycb, long player);
static pascal OSErr qt_movie_drawing_completion_callback(Movie p_movie, long p_reference);
static pascal Boolean controllerMsgFilter(MovieController mc, short action, void *params, long player);
static pascal OSErr enterNodeCallback(QTVRInstance theInstance, UInt32 nodeid, SInt32 player);
static pascal void clickHotSpotCallback(QTVRInstance qtvr, QTVRInterceptPtr qtvrMsg, SInt32 player, Boolean *cancel);

static inline MCRectangle RectToMCRectangle(Rect r)
{
	MCRectangle mcr;
	mcr . x = r . left;
	mcr . y = r . top;
	mcr . width = r . right - r . left;
	mcr . height = r . bottom - r . top;
	return mcr;
}

#endif

#ifdef _MACOSX
struct MCPlayerOffscreenBuffer
{
	CGrafPtr gworld;
	MCImageBitmap image_bitmap;
};

static Boolean IsQTVRInstalled(void);
#endif

#ifdef _WINDOWS
extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
#endif

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

#ifdef FEATURE_MPLAYER
	command = NULL;
	m_player = NULL ;
#endif

#ifdef FEATURE_QUICKTIME
	theMovie = NULL;
	theMC = NULL;
	stopMovieCB = NULL;
	interestingTimeCB = NULL; //initialized at bufferDraw()
	qtvrstate = QTVR_NOT_INITTED;
	qtvrinstance = NULL;
#endif

#ifdef _MACOSX
	m_offscreen = nil;
#elif defined _WINDOWS
	deviceID = 0;
	hwndMovie = NULL;
	m_offscreen = nil;
	bufferGW = NULL;//gworld for buffering draw - for QT only
	m_has_port_association = false;
#endif
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

#ifdef FEATURE_MPLAYER
	command = NULL;
	m_player = NULL ;
#endif

#ifdef FEATURE_QUICKTIME
	theMovie = NULL;
	theMC = NULL;
	stopMovieCB = NULL;
	interestingTimeCB = NULL; //initialized at bufferDraw()
	bufferGW = NULL; //for QT movie in WIN & MAC
	qtvrstate = QTVR_NOT_INITTED;
	qtvrinstance = NULL;
#endif

#ifdef _MACOSX
	m_offscreen = nil;
#elif defined _WINDOWS
	deviceID = 0;
	hwndMovie = NULL;
	m_offscreen = nil;
	m_has_port_association = false;
#endif
}

MCPlayer::~MCPlayer()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the player is actually closed before deletion, otherwise dangling references may still exist.
	while (opened)
		close();
	
	playstop();

#ifdef FEATURE_QUICKTIME
	if (this == MCtemplateplayer && qtstate == QT_INITTED && MCplayers == NULL)
	{
		stoprecording();
		if (qtvrstate == QTVR_INITTED)
		{
#ifdef _WINDOWS
			TerminateQTVR();
#endif
			qtvrstate = QTVR_NOT_INITTED;
		}

#ifdef _WINDOWS
		ExitMovies(); //or ExitToShell() according to QT Developer Q&A
#endif

		if (qteffects != NULL)
		{
			uint2 i;
			for (i=0;i < neffects; i++)
				delete qteffects[i].token;
			delete qteffects;
			qteffects = NULL;
			neffects = 0;
		}

		delete s_ephemeral_player;
		s_ephemeral_player = NULL;
		qtstate = QT_NOT_INITTED;
	}
#endif

#ifdef FEATURE_MPLAYER
	if ( m_player != NULL )
		delete m_player ;
#endif

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
	if (flags & F_ALWAYS_BUFFER && !isbuffering())
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

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED && state & CS_PREPARED)
	{
		checktimes();
		qt_key(true, key);
	}
#endif

	return False;
}

Boolean MCPlayer::kup(const char *string, KeySym key)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
	{
		qt_key(false, key);
	}
#endif

	return False;
}

Boolean MCPlayer::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
	        || flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
		return False;
		
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_move(x, y);
#endif

	return MCControl::mfocus(x, y);
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
#ifdef FEATURE_QUICKTIME
			if (qtstate == QT_INITTED)
				qt_click(true, 1);
#endif
			if (message_with_args(MCM_mouse_down, "1") == ES_NORMAL)
				return True;
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
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_click(true, 2);
#endif
		if (message_with_args(MCM_mouse_down, "2") == ES_NORMAL)
			return True;
		break;
	case Button3:
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_click(true, 3);
#endif
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
#ifdef FEATURE_QUICKTIME
			//if PLAYER's flag is show badge and controller is NOT visible
			if ((flags & F_SHOW_BADGE) && !(flags & F_SHOW_CONTROLLER))
				if (MCGetVisible((MovieController)theMC))
				{ //if the CONTROLLER indicates it is visible
					rect.height += 16;         //set the player rect to include the controller
					flags |= F_SHOW_CONTROLLER; //set player flag to indicate so
				}
#endif

#ifdef FEATURE_QUICKTIME
			if (qtstate == QT_INITTED)
				qt_click(false, 1);
#endif

			if (MCU_point_in_rect(rect, mx, my))
				message_with_args(MCM_mouse_up, "1");
			else
				message_with_args(MCM_mouse_release, "1");

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
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_click(false, which == Button2 ? 2 : 3);
#endif
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
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_setrect(nrect);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_setrect(nrect);
#endif
#elif defined(X11)
	x11_setrect(nrect);
#endif
}

void MCPlayer::timer(MCNameRef mptr, MCParameter *params)
{
#ifdef FEATURE_QUICKTIME
	if (this == s_ephemeral_player && qtstate == QT_INITTED && MCplayers != NULL && MCNameIsEqualTo(mptr, MCM_internal2, kMCCompareCaseless))
	{
		long t_next_time;
		
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (!MCRedrawIsScreenLocked())
		{
			bool t_is_playing;
			t_is_playing = false;

			for(MCPlayer *t_player = MCplayers; t_player != NULL; t_player = t_player -> getnextplayer())
			{
				MovieController t_mc;
				t_mc = (MovieController)t_player -> getMovieController();
				if (t_mc == NULL)
					continue;

				if (!t_is_playing)
					t_is_playing = !t_player -> getstate(CS_PAUSED);

				MCIdle(t_mc);
			}

			if (t_is_playing)
				t_next_time = MCqtidlerate;
			else
			{
				if ((qtversion >> 24) >= 6)
				{
					if (QTGetTimeUntilNextTask(&t_next_time, 1000) != noErr)
						t_next_time = MCqtidlerate;
				}
				else
					t_next_time = MCqtidlerate;

				if (t_next_time > MCidleRate)
					t_next_time = MCidleRate;
				else if (t_next_time < (long)MCqtidlerate)
					t_next_time = MCqtidlerate;
			}
		}
		else
			t_next_time = MCqtidlerate;

		MCscreen -> addtimer(this, MCM_internal2, (uint4)t_next_time);
		return;
	}
#endif

	if (MCrecording && this == MCtemplateplayer && MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
	{
		MCscreen->addtimer(this, MCM_internal, PLAY_RATE);
		return;
	}

	if (MCNameIsEqualTo(mptr, MCM_play_stopped, kMCCompareCaseless))
	{
		state |= CS_PAUSED;
		if (isbuffering()) //so the last frame gets to be drawn
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		if (disposable)
		{
			playstop();
			return; //obj is already deleted, do not pass msg up.
		}
	}
#ifdef FEATURE_QUICKTIME
	else if (MCNameIsEqualTo(mptr, MCM_play_paused, kMCCompareCaseless))
		{
			state |= CS_PAUSED;
			if (isbuffering()) //so the last frame gets to be drawn
			{
				// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
				layer_redrawall();
			}
		}
	else if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless) && qtstate == QT_INITTED && state & CS_PREPARED)
			{
				checktimes();
				return;
			}
#endif
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
#ifndef FEATURE_QUICKTIME
		ep.setint((int)NULL);
#else
		ep.setint((int4)theMC);
#endif
		break;
	case P_PLAY_LOUDNESS:
		ep.setint(getloudness());
		break;
	case P_TRACK_COUNT:
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED && state & CS_PREPARED)
			i = (uint2)GetMovieTrackCount((Movie)theMovie);
#endif
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
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED && state & CS_PREPARED)
		{
			uint2 i;
			bool first = true;
			for (i = 0 ; i < QTMFORMATS ; i++)
				if (QTMovieHasType((Movie)theMovie, qtmediatypes[i]))
				{
					ep.concatcstring(qtmediastrings[i], EC_COMMA, first);
					first = false;
				}
		}
#endif
		break;
	case P_CURRENT_NODE:
#ifdef FEATURE_QUICKTIME
		if (qtvrinstance != NULL)
			i = (uint2)QTVRGetCurrentNodeID((QTVRInstance)qtvrinstance);
#endif
		ep.setint(i);
		break;
	case P_PAN:
		{
			real8 pan = 0.0;
#ifdef FEATURE_QUICKTIME
			if (qtvrinstance != NULL)
				pan = QTVRGetPanAngle((QTVRInstance)qtvrinstance);
#endif
			ep.setr8(pan, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
		break;
	case P_TILT:
		{
			real8 tilt = 0.0;
#ifdef FEATURE_QUICKTIME
			if (qtvrinstance != NULL)
				tilt = QTVRGetTiltAngle((QTVRInstance)qtvrinstance);
#endif
			ep.setr8(tilt, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
		break;
	case P_ZOOM:
		{
			real8 zoom = 0.0;
#ifdef FEATURE_QUICKTIME
			if (qtvrinstance != NULL)
				zoom = QTVRGetFieldOfView((QTVRInstance)qtvrinstance);
#endif
			ep.setr8(zoom, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		}
		break;
	case P_CONSTRAINTS:
		ep.clear();
#ifdef FEATURE_QUICKTIME
		if (qtvrinstance != NULL)
		{
			char buffer[R4L * 2];
			real4 minrange, maxrange;
			uint2 i;
			minrange = maxrange = 0.0;
			for (i = 0 ; i <= 2 ; i++)
			{
				QTVRGetConstraints((QTVRInstance)qtvrinstance, i, &minrange, &maxrange);
				sprintf(buffer, "%f,%f", minrange, maxrange);
				ep.concatcstring(buffer, EC_RETURN, i == 0);
			}
		}
#endif
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
#ifdef FEATURE_QUICKTIME
		deleteUserCallbacks(); //delete all callbacks for this player
#endif
		delete userCallbackStr;
		if (data.getlength() == 0)
			userCallbackStr = NULL;
		else
		{
			userCallbackStr = data.clone();
#ifdef FEATURE_QUICKTIME
			installUserCallbacks(); //install all callbacks for this player
#endif
		}
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
#ifndef _MOBILE
			if (endtime == MAXUINT4) //if endtime is not set, set it to the length of movie
				endtime = getduration();
			else
				if (starttime > endtime)
					endtime = starttime;
#endif
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
#ifndef _MOBILE
			if (starttime == MAXUINT4)
				starttime = 0;
			else
				if (starttime > endtime)
					starttime = endtime;
#endif
		}
		setselection();
		break;
	case P_TRAVERSAL_ON:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED && getstate(CS_PREPARED))
			MCDoAction((MovieController)theMC, mcActionSetKeysEnabled, (void*)((flags & F_TRAVERSAL_ON) != 0));
#endif
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
#ifdef FEATURE_QUICKTIME
		{
			uint4 l = data.getlength();
			const char *sptr = data.getstring();
			playstop();
			if ((theMC = (void *)MCU_strtol(sptr, l, ' ', dirty)) == 0 || !dirty)
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			theMovie = MCGetMovie((MovieController)theMC);
		}
#endif
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
#ifdef FEATURE_QUICKTIME
		{
			uint2 nodeid;
			if (!MCU_stoui2(data,nodeid))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (qtvrinstance != NULL)
			{
				if (QTVRGoToNodeID((QTVRInstance)qtvrinstance,nodeid) != noErr)
				{
					MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
					return ES_ERROR;
				}
				if (isbuffering())
					dirty = True;
			}
		}
#endif
		break;
	case P_PAN:
#ifdef FEATURE_QUICKTIME
		{
			real8 pan;
			if (!MCU_stor8(data, pan))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (qtvrinstance != NULL)
				QTVRSetPanAngle((QTVRInstance)qtvrinstance, (float)pan);
			if (isbuffering())
				dirty = True;
		}
#endif
		break;
	case P_TILT:
#ifdef FEATURE_QUICKTIME
		{
			real8 tilt;
			if (!MCU_stor8(data, tilt))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (qtvrinstance != NULL)
				QTVRSetTiltAngle((QTVRInstance)qtvrinstance, (float)tilt);
			if (isbuffering())
				dirty = True;
		}
#endif
		break;
	case P_ZOOM:
#ifdef FEATURE_QUICKTIME
		{
			real8 zoom;
			if (!MCU_stor8(data, zoom))
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
				return ES_ERROR;
			}
			if (qtvrinstance != NULL)
				QTVRSetFieldOfView((QTVRInstance)qtvrinstance, (float)zoom);
			if (isbuffering())
				dirty = True;
		}
#endif
		break;
	case P_VISIBLE:
	case P_INVISIBLE:
		{
			uint4 oldflags = flags;
			Exec_stat stat = MCControl::setprop(parid, p, ep, effective);
			if (flags != oldflags && !(flags & F_VISIBLE))
				playstop();
#ifdef FEATURE_QUICKTIME
			if (theMC != NULL)
				MCSetVisible((MovieController)theMC, getflag(F_VISIBLE) && getflag(F_SHOW_CONTROLLER));
#endif
			
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
#ifdef FEATURE_QUICKTIME
	if (qtstate != QT_INITTED)
		return;

	bool t_should_buffer;

	// MW-2011-09-13: [[ Layers ]] If the layer is dynamic then the player must be buffered.
	t_should_buffer = getstate(CS_SELECTED) || getflag(F_ALWAYS_BUFFER) || getstack() -> getstate(CS_EFFECT) || (p_dc != nil && p_dc -> gettype() != CONTEXT_TYPE_SCREEN) || !MCModeMakeLocalWindows() || layer_issprite();

	if (t_should_buffer && !isbuffering())
		bufferDraw(false);
	else if (!t_should_buffer && isbuffering())
		unbufferDraw();
#endif
}

// MW-2007-08-14: [[ Bug 1949 ]] On Windows ensure we load and unload QT if not
//   currently in use.
void MCPlayer::getversion(MCExecPoint &ep)
{
#if defined(X11)
	ep.setstaticcstring("2.0");
#elif defined(_WINDOWS)
	// MW-2010-09-13: [[ Bug 8956 ]] Make sure QT is kept in memory after checking the version,
	//   if the user hasn't turned it off (dontUseQT == false).
	bool t_transient_qt;
	t_transient_qt = false;
	if (qtstate != QT_INITTED)
	{
		if (!MCdontuseQT)
			initqt();
		else
		{
			if (InitializeQTML(0L) == noErr)
				t_transient_qt = true;
			else
				t_transient_qt = false;
		}
	}

	long attrs;
	if ((t_transient_qt || qtstate == QT_INITTED) && Gestalt(gestaltQuickTimeVersion, &attrs) == noErr)
		ep.setstringf("%ld.%ld.%ld", attrs >> 24, (attrs >> 20) & 0xF, (attrs >> 16) & 0xF);
	else
		ep.setstaticcstring("0.0");

	if (t_transient_qt)
		TerminateQTML();
#elif defined(_MACOSX)
	initqt();
	if (qtstate == QT_INITTED)
	{//for QT
		long attrs;
		if (Gestalt(gestaltQuickTimeVersion, &attrs) == noErr)
			ep.setstringf("%ld.%ld.%ld", attrs >> 24, (attrs >> 20) & 0xF, (attrs >> 16) & 0xF);
		else
			ep.setstaticcstring("0.0");  //indicates that no QT installed
	}
	else //for AVI movie
		ep.setstaticcstring("0.0");  //indicates that no QT installed
#else
	ep.clear();
#endif
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
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		return qt_getduration();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		return avi_getduration();
#endif
#elif defined(X11)
	return x11_getduration();
#else
	return 0;
#endif
}

uint4 MCPlayer::gettimescale() //get moive time scale
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		return qt_gettimescale();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		return avi_gettimescale();
#endif
#elif defined(X11) //X11 stuff
	return x11_gettimescale();
#else
	return 0;
#endif
}

uint4 MCPlayer::getmoviecurtime()
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		return qt_getmoviecurtime();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		return avi_getmoviecurtime();
#endif
#elif defined(X11)
	return x11_getmoviecurtime();
#else
	return 0;
#endif
}

void MCPlayer::setcurtime(uint4 newtime)
{
	lasttime = newtime;
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_setcurtime(newtime);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_setcurtime(newtime);
#endif
#elif defined(X11)
	x11_setcurtime(newtime);
#endif
}

void MCPlayer::setselection()
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_setselection();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_setselection();
#endif
#elif defined(X11)
	x11_setselection();
#endif
}

void MCPlayer::setlooping(Boolean loop)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED) // loop or unloop QT movie
		qt_setlooping(loop);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_setlooping(loop);
#endif
#elif defined(X11)
	x11_setlooping(loop);
#endif
}

void MCPlayer::setplayrate()
{
#ifdef FEATURE_QUICKTIME //MAC or WIN
	if (qtstate == QT_INITTED)
		qt_setplayrate();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_setplayrate();
#endif
#elif defined(X11)
	x11_setplayrate();
#endif

	if (rate != 0)
		state = state & ~CS_PAUSED;
	else
		state = state | CS_PAUSED;
}

void MCPlayer::showbadge(Boolean show)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)// set QT movie's play rate, for QT movie only
		qt_showbadge(show);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_showbadge(show);
#endif
#elif defined(X11)
	x11_showbadge(show);
#endif
}

void MCPlayer::editmovie(Boolean edit)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)//on & off the ability to set selection
		qt_editmovie(edit);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_editmovie(edit);
#endif
#elif defined(X11)
	x11_editmovie(edit);
#endif
}

void MCPlayer::playselection(Boolean play)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_playselection(play);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_playselection(play);
#endif
#elif defined(X11)
	x11_playselection(play);
#endif
}

Boolean MCPlayer::ispaused()
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		return qt_ispaused();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		return avi_ispaused();
#endif
#elif defined(X11)
	return x11_ispaused();
#else
	return True;
#endif
}

void MCPlayer::showcontroller(Boolean show)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_showcontroller(show);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_showcontroller(show);
#endif
#elif defined(X11)
	x11_showcontroller(show);
#endif
}

Boolean MCPlayer::prepare(const char *options)
{
	Boolean ok = False;

	if (state & CS_PREPARED || filename == NULL)
		return True;
	
	if (!opened)
		return False;

#ifdef X11
	ok = x11_prepare();
#elif defined FEATURE_QUICKTIME
	initqt();
	if (qtstate == QT_INITTED)
		ok = qt_prepare();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		ok = avi_prepare();
#endif
#endif

	if (ok)
	{
		state |= CS_PREPARED | CS_PAUSED;

#ifdef FEATURE_QUICKTIME
		// MW-2007-07-06: [[ Bug 3848 ]] We shouldn't set up this timer if we
		//   aren't using QT (s_ephemeral_player == NULL).
		if (MCplayers == NULL && s_ephemeral_player != NULL)
			MCscreen -> addtimer(s_ephemeral_player, MCM_internal2, 0);
#endif
		
#ifdef X11
		// If we get here and MClastvideowindow == DNULL it means that MClastvideowindow
		// was set to DNULL and the video window destroyed in the SIGCHLD handler -- this
		// means that the child process has terminated, which is most likly caused by 
		// mplayer not being available.
		if (MClastvideowindow == DNULL)
		{
		}
		else
#endif
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

#ifdef TARGET_PLATFORM_WINDOWS
	if (qtstate != QT_INITTED)
		setstate(avi_ispaused(), CS_PAUSED);
#endif
#ifdef FEATURE_QUICKTIME
	if (on == getstate(CS_PAUSED))
		return True;
#endif

	Boolean ok;
	ok = False;

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		ok = qt_playpause(on);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		ok = avi_playpause(on);
#endif
#elif defined(X11)
	ok = x11_playpause(on);
#endif
	
	if (ok)
		setstate(on, CS_PAUSED);

	return ok;
}

void MCPlayer::playstepforward()
{
	if (!getstate(CS_PREPARED))
		return;

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_playstepforward();
#ifdef TARGET_PLATFORM_WINDOWS		
	else
		avi_playstepforward();
#endif
#elif defined(X11)
	x11_playstepforward();
#endif
}

void MCPlayer::playstepback()
{
	if (!getstate(CS_PREPARED))
		return;

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		qt_playstepback();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_playstepback();
#endif
#elif defined(X11)
	x11_playstepback();
#endif
}

Boolean MCPlayer::playstop()
{
	formattedwidth = formattedheight = 0;
	if (!getstate(CS_PREPARED))
		return False;

	Boolean needmessage = True;
	
	state &= ~(CS_PREPARED | CS_PAUSED);
	lasttime = 0;

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		needmessage = qt_playstop();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		needmessage = avi_playstop();
#endif
#elif defined(X11)
	needmessage = x11_playstop();
#endif

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

#ifdef FEATURE_QUICKTIME
	if (qtstate == QT_INITTED)
		return qt_getpreferredrect();
#ifdef TARGET_PLATFORM_WINDOWS
	else
		return avi_getpreferredrect();
#endif
#elif defined(X11)
	return x11_getpreferredrect();
#else
	MCRectangle t_bounds;
	MCU_set_rect(t_bounds, 0, 0, 0, 0);
	return t_bounds;
#endif
}

uint2 MCPlayer::getloudness()
{
	if (getstate(CS_PREPARED))
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			loudness = qt_getloudness();
#ifdef TARGET_PLATFORM_WINDOWS
		else
			loudness = avi_getloudness();
#endif
#elif defined(X11)
		loudness = x11_getloudness();
#else
		loudness = loudness;
#endif
	return loudness;
}

void MCPlayer::setloudness()
{
	if (state & CS_PREPARED)
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_setloudness(loudness);
#ifdef TARGET_PLATFORM_WINDOWS
		else
			avi_setloudness(loudness);
#endif
#elif defined(X11)
		x11_setloudness(loudness);
#else
	loudness = loudness;
#endif
}

void MCPlayer::gettracks(MCExecPoint &ep)
{
	ep . clear();

	if (getstate(CS_PREPARED))
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_gettracks(ep);
#ifdef TARGET_PLATFORM_WINDOWS
		else
			avi_gettracks(ep);
#endif
#elif defined(X11)
		x11_gettracks(ep);
#else
	0 == 0;
#endif
}

void MCPlayer::getenabledtracks(MCExecPoint &ep)
{
	ep.clear();

	if (getstate(CS_PREPARED))
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			qt_getenabledtracks(ep);
#ifdef TARGET_PLATFORM_WINDOWS
		else
			avi_getenabledtracks(ep);
#endif
#elif defined(X11)
		x11_getenabledtracks(ep);
#else
		0 == 0;
#endif
}

Boolean MCPlayer::setenabledtracks(const MCString &s)
{
	if (getstate(CS_PREPARED))
#ifdef FEATURE_QUICKTIME
		if (qtstate == QT_INITTED)
			return qt_setenabledtracks(s);
#ifdef TARGET_PLATFORM_WINDOWS
		else
			return avi_setenabledtracks(s);
#endif
#elif defined(X11)
		return x11_setenabledtracks(s);
#else
		0 == 0;
#endif

	return True;
}

void MCPlayer::getnodes(MCExecPoint &ep)
{
	ep.clear();
#ifdef FEATURE_QUICKTIME
	if (qtvrinstance != NULL)
	{
		QTAtomContainer			qtatomcontainer;
		QTAtom					qtatom;
		uint2				    numnodes = 0;
		if (QTVRGetVRWorld((QTVRInstance)qtvrinstance, &qtatomcontainer) != noErr)
			return;
		qtatom = QTFindChildByIndex(qtatomcontainer, kParentAtomIsContainer, kQTVRNodeParentAtomType, 1, NULL);
		if (qtatom == 0)
		{
			QTDisposeAtomContainer(qtatomcontainer);
			return;
		}
		numnodes = QTCountChildrenOfType(qtatomcontainer, qtatom, kQTVRNodeIDAtomType);
		QTAtomID qtatomid;
		uint2 index;
		for (index = 1; index <= numnodes; index++)
		{
			QTFindChildByIndex(qtatomcontainer, qtatom, kQTVRNodeIDAtomType, index, &qtatomid);
			ep.concatuint(qtatomid, EC_RETURN, index == 1); // id
			ep.concatcstring(QTVRGetNodeType((QTVRInstance)qtvrinstance, (uint2)qtatomid) == kQTVRPanoramaType ? "panorama" : "object", EC_COMMA, false);
		}
		QTDisposeAtomContainer(qtatomcontainer);
	}
#endif
}

void MCPlayer::gethotspots(MCExecPoint &ep)
{
	ep.clear();
#ifdef FEATURE_QUICKTIME
	if (qtvrinstance != NULL)
	{
		QTAtomContainer qtatomcontainer;
		QTAtom qtatom;
		uint2 numhotspots = 0;
		if (QTVRGetNodeInfo((QTVRInstance)qtvrinstance, QTVRGetCurrentNodeID((QTVRInstance)qtvrinstance),
		                    &qtatomcontainer) != noErr)
			return;
		qtatom = QTFindChildByID(qtatomcontainer, kParentAtomIsContainer,
		                         kQTVRHotSpotParentAtomType, 1, NULL);
		if (qtatom == 0)
		{
			QTDisposeAtomContainer(qtatomcontainer);
			return;
		}
		numhotspots = QTCountChildrenOfType(qtatomcontainer, qtatom,
		                                    kQTVRHotSpotAtomType);
		QTAtomID qtatomid;
		OSType hotspottype;
		uint2 index;
		for (index = 1; index <= numhotspots; index++)
		{
			QTFindChildByIndex(qtatomcontainer, qtatom, kQTVRHotSpotAtomType, index, &qtatomid);
			ep.concatuint(qtatomid, EC_RETURN, index == 1);//id

			const char *t_type;
			QTVRGetHotSpotType((QTVRInstance)qtvrinstance,(uint2)qtatomid,&hotspottype);
			switch (hotspottype)
			{
			case kQTVRHotSpotLinkType: t_type = "link"; break;
			case kQTVRHotSpotURLType: t_type = "url"; break;
			default:
				t_type = "undefined";
			}
			ep.concatcstring(t_type ,EC_COMMA, false);//type
		}
		QTDisposeAtomContainer(qtatomcontainer);
	}
#endif
}

#ifdef _WINDOWS
void MCPlayer::changewindow(MCSysWindowHandle p_old_window)
{
	HWND t_new_window;
	t_new_window = (HWND)getstack()->getqtwindow();
	SetParent((HWND)hwndMovie, t_new_window);
}

HWND create_player_child_window(MCRectangle &p_rect, HWND p_parent, LPCSTR p_window_class);

LRESULT CALLBACK MCQTPlayerWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam)
{
	MSG t_winMsg;
	EventRecord t_qtEvent;
	t_winMsg.hwnd = hwnd;
	t_winMsg.message = msg;
	t_winMsg.wParam = wParam;
	t_winMsg.lParam = lParam;

	NativeEventToMacEvent(&t_winMsg, &t_qtEvent);


	for (MCPlayer *t_player = MCplayers; t_player != NULL; t_player = t_player->getnextplayer())
	{
		if ((HWND)t_player->getplayerwindow() == hwnd)
		{
			MCIsPlayerEvent((MovieController)t_player->getMovieController(), &t_qtEvent);
			break;
		}
	}

	return DefWindowProcA(hwnd, msg, wParam, lParam);
}
#endif

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

#ifdef FEATURE_QUICKTIME
	if (!(state & CS_CLOSING))
		prepare(MCnullstring);

	if (qtstate == QT_INITTED)
		qt_draw(dc, dirty);
#ifdef TARGET_PLATFORM_WINDOWS
	else
		avi_draw(dc, dirty);
#endif
#else
	setforeground(dc, DI_BACK, False);
	dc->setbackground(MCscreen->getwhite());
	dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
	dc->fillrect(rect);
	dc->setbackground(MCzerocolor);
	dc->setfillstyle(FillSolid, nil, 0, 0);
#endif

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

//  Redraw Management
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  QT Event Processing
//

#ifdef FEATURE_QUICKTIME
void MCPlayer::qt_move(int2 x, int2 y)
{
}

void MCPlayer::qt_click(bool p_state, uint4 p_button)
{
#ifdef TARGET_PLATFORM_WINDOWS
	if (theMC != NULL)
	{
		MSG t_msg;
		EventRecord t_event;

		if (p_button == 2)
			return;

		if (!p_state)
			return;

		int32_t t_dx, t_dy;
		t_dx = t_dy = 0;
		if (!isbuffering())
		{
			MCRectangle t_rect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
			t_dx = t_rect.x;
			t_dy = t_rect.y;
		}
		t_msg . hwnd = (HWND)hwndMovie;
		t_msg . message = p_button == 1 ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
		t_msg . wParam = 0;
		t_msg . lParam = (mx - t_dx) | (my - t_dy) << 16;
		t_msg . time = MCeventtime;
		t_msg . pt . x = mx - t_dx;
		t_msg . pt . y = my - t_dy;
		WinEventToMacEvent(&t_msg, &t_event);

		t_event . where . h = mx - t_dx;
		t_event . where . v = my - t_dy;
		if (!getflag(F_TRAVERSAL_ON))
			t_event . modifiers &= activeFlagBit | btnStateBit | cmdKeyBit;

		MCClick((MovieController)theMC, GetHWNDPort(hwndMovie), t_event . where, t_event . when, t_event . modifiers);
	}
#else
	if (theMC != NULL)
	{
		long t_when;
		long t_modifiers;
		Point t_where;
		
		t_when = MCeventtime * 60 / 1000;
		if (isbuffering())	
			t_where . h = mx - rect . x, t_where . v = my - rect . y;
		else
			t_where . h = mx, t_where . v = my;
		t_modifiers = 0;
		
		if (p_button == 1)
			t_modifiers |= (1 << btnStateBit);
		else
			t_modifiers |= (1 << btnStateBit) | (1 << cmdKeyBit);
		
		if ((MCmodifierstate & MS_SHIFT) != 0)
			t_modifiers |= (1 << shiftKeyBit);
		if ((MCmodifierstate & MS_CONTROL) != 0)
			t_modifiers |= (1 << controlKeyBit);
		if ((MCmodifierstate & MS_MOD1) != 0)
			t_modifiers |= (1 << optionKeyBit);
		
		MCRectangle t_rect;
		t_rect = getstack() -> getrect();
		
		EventRecord t_event;
		t_event . what = p_state ? mouseDown : mouseUp;
		t_event . message = 0;
		t_event . when = t_when;
		t_event . where . h = t_rect . x + t_where . h;
		t_event . where . v = t_rect . y + t_where . v - getstack() -> getscroll();
		t_event . message = 0;
		t_event . modifiers = t_modifiers;
		
		MCIsPlayerEvent((MovieController)theMC, &t_event);
	}
#endif
}

void MCPlayer::qt_key(bool p_state, uint4 p_key)
{
	if (theMC != NULL)
	{
		if (!getflag(F_TRAVERSAL_ON))
			return;
	
		if (!p_state)
			return;

		SInt8 t_key;
		long t_modifiers;

		t_modifiers = 0;
		if ((MCmodifierstate & MS_SHIFT) != 0)
			t_modifiers |= 1 << shiftKeyBit;
		if ((MCmodifierstate & MS_CONTROL) != 0)
			t_modifiers |= 1 << controlKeyBit;
		if ((MCmodifierstate & MS_MOD1) != 0)
			t_modifiers |= 1 << optionKeyBit;

		switch(p_key)
		{
		case XK_Home:
			t_key = kHomeCharCode;
		break;
		case XK_KP_Enter:
			t_key = kEnterCharCode;
		break;
		case XK_End:
			t_key = kEndCharCode;
		break;
		case XK_Help:
			t_key = kHelpCharCode;
		break;
		case XK_BackSpace:
			t_key = kBackspaceCharCode;
		break;
		case XK_Tab:
			t_key = kTabCharCode;
		break;
		case XK_Linefeed:
			t_key = kLineFeedCharCode;
		break;
		case XK_Prior:
			t_key = kPageUpCharCode;
		break;
		case XK_Next:
			t_key = kPageDownCharCode;
		break;
		case XK_Return:
			t_key = kReturnCharCode;
		break;
		case XK_Escape:
			t_key = kEscapeCharCode;
		break;
		case XK_Clear:
			t_key = kClearCharCode;
		break;
		case XK_Left:
			t_key = kLeftArrowCharCode;
		break;
		case XK_Right:
			t_key = kRightArrowCharCode;
		break;
		case XK_Up:
			t_key = kUpArrowCharCode;
		break;
		case XK_Down:
			t_key = kDownArrowCharCode;
		break;
		case XK_Delete:
			t_key = kDeleteCharCode;
		break;
		case XK_KP_Space:
			t_key = ' ';
		break;
		default:
			if (p_key > 127)
				return;
			t_key = p_key;
		break;
		}

		MCKey((MovieController)theMC, t_key, t_modifiers);
	}
}
#endif
//
//  QT Event Processing
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// QuickTime Player Implementation
//
#ifdef FEATURE_QUICKTIME
// Determines whether QuickTime is initialised.
QTstate MCPlayer::qtstate = QT_NOT_INITTED;
long MCPlayer::qtversion = 0;
MCPlayer *MCPlayer::s_ephemeral_player = NULL;

void MCPlayer::initqt()
{
	if (MCdontuseQT)
		return;

	if (qtstate == QT_INITTED)
		return;

	s_ephemeral_player = new MCPlayer;

#ifdef _WINDOWS
	if (InitializeQTML(0L) != noErr || EnterMovies() != noErr)
		qtstate = QT_FAILED;
	else
	{
		Gestalt(gestaltQuickTimeVersion, &qtversion);
		qtstate = QT_INITTED;
	}
#elif defined _MACOSX
	long response;
	if (Gestalt(gestaltQuickTimeVersion, &response) != noErr || EnterMovies() != noErr)
		qtstate = QT_FAILED;
	else
	{
		qtstate = QT_INITTED;
		qtversion = response;
	}
#endif
}

void MCPlayer::initqtvr()
{
	if (MCdontuseQT)
		return;

	if (qtvrstate == QTVR_INITTED)
		return;

#ifdef _WINDOWS
	if (InitializeQTVR() != noErr)
		qtvrstate = QTVR_FAILED;
	else
		qtvrstate = QTVR_INITTED;
#elif defined _MACOSX
	if (!IsQTVRInstalled())
		qtvrstate = QTVR_FAILED;
	else
		qtvrstate = QTVR_INITTED;
#endif
}

void MCPlayer::checktimes()
{
	if (state & CS_NO_MESSAGES)
		return;

	TimeRecord start, duration;
	if (MCDoAction((MovieController)theMC, mcActionGetSelectionBegin, &start))
	{
		Boolean changed = False;
		if (start.value.lo != starttime)
		{
			starttime = start.value.lo;
			changed = True;
		}
		MCDoAction((MovieController)theMC, mcActionGetSelectionDuration, &duration);
		int4 tm = start.value.lo + duration.value.lo;
		if (tm != (int4)endtime)
		{
			endtime = tm;
			changed = True;
		}
		if (changed) //send a selection changed msg to engine
			message(MCM_selection_changed);
	}
	/* check the player's last message time, the last time the player sends a *
	 * current_time_changed message                                           */
	//get movie's current time
	TimeValue t = GetMovieTime((Movie)theMovie, nil);
	if (t != lasttime)
	{
		lasttime = t;
		message_with_args(MCM_current_time_changed, lasttime);
	}
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
			trect.height = 16;
			rect = trect;
		}
		else
		{
			x = trect.x + (trect.width >> 1);
			y = trect.y + (trect.height >> 1);
			trect.width = (uint2)(formattedwidth * scale);
			trect.height = (uint2)(formattedheight * scale);
			if (flags & F_SHOW_CONTROLLER)
				trect.height += 16;
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

void MCPlayer::setMCposition(const MCRectangle &newrect)
{  //re-position the Movie Controller
	Rect playingRect;
	playingRect.left = newrect.x;
	playingRect.top = newrect.y;
	playingRect.right = newrect.x + newrect.width;
	playingRect.bottom = newrect.y + newrect.height;
	if ((flags & F_SHOW_CONTROLLER && newrect.height > 16) && formattedheight != 0) //if controller is showing
		MCPositionController((MovieController)theMC, &playingRect, NULL, (long)mcScaleMovieToFit);
	else
		MCPositionController((MovieController)theMC, (const Rect *)&playingRect, NULL,
		                     (long)(mcScaleMovieToFit | mcTopLeftMovie));
}

void MCPlayer::bufferDraw(bool p_resize)
{ //direct movie controller to draw to OFFSCREEN buffer
if (theMovie == NULL)
		return;
	MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
	
	// MW-2012-09-04: [[ Bug 10361 ]] Make sure we don't try and resize the rect
	//   to nothing otherwise things go awry.
	if (trect.width == 0)
		trect.width = 1;
	if (trect.height == 0)
		trect.height = 1;
	
#ifdef _WINDOWS
	ShowWindow((HWND)hwndMovie, SW_HIDE);
	if (p_resize)
	{
		HBITMAP t_old_bitmap;
		HDC t_old_dc;
		GWorldPtr t_old_gworld;

		t_old_gworld = (GWorldPtr)bufferGW;
		t_old_dc = m_offscreen->hdc;
		t_old_bitmap = m_offscreen->hbitmap;

		if (t_old_gworld != NULL)
		{
			DisposeGWorld(t_old_gworld);
			DeleteObject(t_old_bitmap);
			DeleteDC(t_old_dc);
		}
	}
	else
		/* UNCHECKED */ MCMemoryNew(m_offscreen);

	HBITMAP t_new_bitmap = NULL;
	HDC t_new_dc = NULL;
	GWorldPtr t_new_gworld = NULL;
	void *t_bits;
	bool t_error = false;

	t_error = (t_new_dc = CreateCompatibleDC(NULL)) == NULL;
	if (!t_error)
		t_error = !create_temporary_dib(t_new_dc, trect . width, trect . height, t_new_bitmap, t_bits);
	if (!t_error)
	{
		SelectObject(t_new_dc, t_new_bitmap);
		t_error = (NewGWorldFromHBITMAP(&t_new_gworld, (CTabHandle)NULL, NULL, useTempMem, t_new_bitmap, t_new_dc) != noErr);
	}

	if (t_error)
	{
		if (t_new_dc != NULL)
			DeleteDC(t_new_dc);
		if (t_new_bitmap != NULL)
			DeleteObject(t_new_bitmap);
		MCMemoryDelete(m_offscreen);
		m_offscreen = nil;
		if (p_resize)
			bufferDraw(false);
		return;
	}

	m_offscreen->hbitmap = t_new_bitmap;
	m_offscreen->hdc = t_new_dc;
	m_offscreen->image_bitmap.width = trect.width;
	m_offscreen->image_bitmap.height = trect.height;
	m_offscreen->image_bitmap.data = (uint32_t*)t_bits;
	m_offscreen->image_bitmap.stride = trect.width * sizeof(uint32_t);
	m_offscreen->image_bitmap.has_transparency = m_offscreen->image_bitmap.has_alpha = false;

	bufferGW = t_new_gworld;
	SetGWorld((CGrafPtr)bufferGW, (GDHandle)NULL); // Port or graphics world to make current
	MCSetControllerPort((MovieController)theMC, (CGrafPtr)bufferGW);

	MCRectangle r = trect;
	r.x = r.y = 0;
	setMCposition(r); //reset the movie & controller position to be drawn to the offscreen buffer
	if (flags & F_SHOW_BADGE) //if the showbadge is on, turn it off, turn it back on in unbufferDraw()
		showbadge(False);
		
	MCSetClip((MovieController)theMC, NULL, NULL);
	MCDoAction((MovieController)theMC, mcActionDraw, bufferGW);
#elif defined _MACOSX
	//if player and stack are buffering, player draws into the stack's buffer
	//if player is buffering, the stack is not. Don't know what will happen yet, we have to test
	//this part of code to see if it works correctly
	
	GWorldPtr t_old_buffer;
	if (p_resize)
		t_old_buffer = m_offscreen->gworld;
	else
		t_old_buffer = NULL;
	
	if (m_offscreen != NULL)
		MCMemoryDelete(m_offscreen);
	
	Rect macr;
	macr.left = macr.top = 0;
	macr.bottom = trect.height;
	macr.right = trect.width;
	/* UNCHECKED */ MCMemoryNew(m_offscreen);
	NewGWorld((CGrafPtr *)&m_offscreen->gworld, 32, &macr, NULL, NULL, useTempMem | kNativeEndianPixMap);
	
	if (m_offscreen->gworld == NULL) //not successful, bail out
	{
		MCMemoryDelete(m_offscreen);
		m_offscreen = nil;
		unbufferDraw();
		if (p_resize)
			bufferDraw(false);
		return;
	}
	
	m_offscreen->image_bitmap.width = trect.width;
	m_offscreen->image_bitmap.height = trect.height;
	m_offscreen->image_bitmap.has_transparency = m_offscreen->image_bitmap.has_alpha = false;
	
	MCSetControllerPort((MovieController)theMC, m_offscreen->gworld);
	MCRectangle r = trect;
	r.x = r.y = 0;

	setMCposition(r); //reset the movie & controller position in offscreen buffer
	if (flags & F_SHOW_BADGE) //if the showbadge is on, turn it off
		showbadge(False);
		
	MCSetClip((MovieController)theMC, NULL, NULL);
	MCDoAction((MovieController)theMC, mcActionDraw, NULL);

	if (t_old_buffer != NULL)
		DisposeGWorld(t_old_buffer);
#endif

  SetMovieDrawingCompleteProc((Movie)theMovie, movieDrawingCallWhenChanged, qt_movie_drawing_completion_callback, (long)this);
}

void MCPlayer::unbufferDraw()
{
	SetMovieDrawingCompleteProc((Movie)theMovie, 0, NULL, 0);

	MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
#ifdef _WINDOWS
	MoveWindow((HWND)hwndMovie, trect.x, trect.y, trect.width, trect.height, False);
	ShowWindow((HWND)hwndMovie, SW_SHOW);
	trect.x = trect.y = 0;
	HWND movieWin = (HWND)hwndMovie;

	CGrafPtr winport = (CGrafPtr)GetNativeWindowPort((void *)movieWin);
	SetGWorld(winport, nil);
	MCSetControllerPort((MovieController)theMC, winport);

	DisposeGWorld((GWorldPtr)bufferGW);
	bufferGW = NULL;
	DeleteObject(m_offscreen->hbitmap);
	DeleteDC(m_offscreen->hdc);
	MCMemoryDelete(m_offscreen);
	m_offscreen = nil;

	setMCposition(trect); //reset the movie & controller postion
	if (flags & F_SHOW_BADGE) //if the showbadge is supposed to be on turn it back on
		showbadge(True);
	if (flags & F_SHOW_CONTROLLER)
		MCDoAction((MovieController)theMC, mcActionActivate, (void *)NULL);

#elif defined _MACOSX
	//reset back to draw to window
	MCSetControllerPort((MovieController)theMC, GetWindowPort((WindowPtr)getstack()->getqtwindow()));
	
	// Ok-2007-06-15: Fix for bug 5151. If the width or height of a player is set to zero, m_offscreen
	// is deleted and set to NULL in bufferDraw, unbufferDraw is then called, previously leading to a crash.
	if (m_offscreen != NULL)
	{
		DisposeGWorld(m_offscreen->gworld);
		MCMemoryDelete(m_offscreen);
		m_offscreen = nil;
	}
	// MW-2011-11-30: [[ Bug 9887 ]] Make sure the player rect is adjusted to account for
	//   menubar.
	trect . y -= getstack() -> getscroll();
	setMCposition(trect);
	if (flags & F_SHOW_BADGE) //if the showbadge is supposed to be on, restore it
		showbadge(True);
	if (flags & F_SHOW_CONTROLLER)
		MCDoAction((MovieController)theMC, mcActionActivate, (void *)NULL);
#endif
}

static MCRegionRef PicToRegion(PicHandle thePicture)
{
	Rect					myRect;
	GWorldPtr				myGWorld = NULL;
	PixMapHandle			myPixMap = NULL;
	CGrafPtr				mySavedPort = NULL;
	GDHandle				mySavedDevice = NULL;
	RgnHandle				myRegion = NULL;
	OSErr					myErr = noErr;
	if (thePicture == NULL)
		return NULL;

	// get the current graphics port and device
	GetGWorld(&mySavedPort, &mySavedDevice);

	// get the bounding box of the picture
	myRect = (**thePicture).picFrame;
	myRect.bottom = EndianS16_BtoN(myRect.bottom);
	myRect.right = EndianS16_BtoN(myRect.right);

	// create a new GWorld and draw the picture into it
	myErr = QTNewGWorld(&myGWorld, k1MonochromePixelFormat, &myRect,
	                    NULL, NULL, kICMTempThenAppMemory);
	if (myGWorld == NULL)
		goto bail;

	SetGWorld(myGWorld, NULL);

	myPixMap = GetPortPixMap(myGWorld);
	if (myPixMap == NULL)
		goto bail;

	LockPixels(myPixMap);
	HLock((Handle)myPixMap);

	EraseRect(&myRect);
	DrawPicture(thePicture, &myRect);

	// create a new region and convert the pixmap into a region
	myRegion = NewRgn();
	myErr = MemError();
	if (myErr != noErr)
		goto bail;
	myErr = BitMapToRegion(myRegion, (BitMap *)*myPixMap);
bail:
	if (myErr != noErr)
	{
		if (myRegion != NULL)
		{
			DisposeRgn(myRegion);
			myRegion = NULL;
		}
	}
	if (myGWorld != NULL)
		DisposeGWorld(myGWorld);
	// restore the original graphics port and device
	SetGWorld(mySavedPort, mySavedDevice);
	return (MCRegionRef)myRegion;
}

MCRegionRef MCPlayer::makewindowregion()
{
#ifdef QT5 //MediaGetPublicInfo is a QT5-only function
	Track myTrack = NULL;
	MediaHandler myHandler = NULL;
	PicHandle myPicture = NULL;
	RgnHandle myRegion = NULL;
	MatrixRecord myMatrix;
	OSErr myErr = noErr;
	if (qtstate == QT_INITTED && state & CS_PREPARED)
	{
		myTrack = GetMovieIndTrackType(theMovie, 1, FOUR_CHAR_CODE('skin'),
		                               movieTrackCharacteristic);
		if (myTrack != NULL)
		{
			myHandler = GetMediaHandler(GetTrackMedia(myTrack));
			if (myHandler != NULL)
			{
				// get the current movie matrix
				GetMovieMatrix(theMovie, &myMatrix);
				myPicture = (PicHandle)NewHandle(0);
				if (myPicture == NULL)
					return NULL;
				// get the content region picture
				myErr = MediaGetPublicInfo(myHandler, FOUR_CHAR_CODE('skcr'),
				                           myPicture, NULL);
				if (myErr == noErr)
				{
					// convert it to a region
					MCScreenDC *pms = (MCScreenDC *)MCscreen;
					myRegion = PicToRegion(myPicture);
					if (myRegion)
					{
						KillPicture(myPicture);
#ifdef _MACOSX

						return myRegion;
#elif defined _WINDOWS

						HRGN winrgn = (HRGN)MacRegionToNativeRegion(myRegion);
						RECT	myRect;
						GetRgnBox(winrgn, &myRect);
						OffsetRgn(winrgn, -myRect.left + GetSystemMetrics(SM_CXFRAME),
						          -myRect.top + GetSystemMetrics(SM_CYCAPTION) +
						          GetSystemMetrics(SM_CXFRAME));
						return winrgn;
#endif

					}
				}
			}
		}
	}
	if (myPicture != NULL)
		KillPicture(myPicture);
#endif

	return NULL;
}

#endif

//--

#ifdef FEATURE_QUICKTIME
Boolean MCPlayer::qt_prepare(void)
{
	CGrafPtr oldPort;
	GDHandle oldDevice;
	GetGWorld(&oldPort, &oldDevice);   // Save previous graphics world

	// MW-2010-06-02: [[ Bug 8773 ]] Make sure we pass 'https' urls through to QT's
	//   URL data handler.
	theMovie = NULL;
	if (strnequal(filename, "https:", 6) || strnequal(filename, "http:", 5) || strnequal(filename, "ftp:", 4) || strnequal(filename, "file:", 5) || strnequal(filename, "rtsp:", 5))
	{
		Size mySize = (Size)strlen(filename) + 1;
		if (mySize)
		{
			Handle myHandle = NewHandleClear(mySize);
			if (myHandle != NULL)
			{
				BlockMove(filename, *myHandle, mySize);
				NewMovieFromDataRef((Movie *)&theMovie, newMovieActive, NULL, myHandle, URLDataHandlerSubType);
				DisposeHandle(myHandle);
			}
		}
	}
	else
	{
#if defined(TARGET_PLATFORM_MACOS_X)
		// OK-2009-01-09: [[Bug 1161]] - File resolving code standardized between image and player
		char *t_filename;
		t_filename = getstack() -> resolve_filename(filename);

		char *t_resolved_filename;
		t_resolved_filename = MCS_resolvepath(t_filename);
		
		CFStringRef t_cf_filename;
		t_cf_filename = NULL;
		if (t_resolved_filename != NULL)
			t_cf_filename = CFStringCreateWithCString(kCFAllocatorDefault, t_resolved_filename, CFStringGetSystemEncoding());
		
		OSErr t_error;
		Handle t_data_ref;
		OSType t_data_ref_type;
		t_error = noErr;
		t_data_ref = NULL;
		if (t_cf_filename != NULL)
			t_error = QTNewDataReferenceFromFullPathCFString(t_cf_filename, kQTPOSIXPathStyle, 0, &t_data_ref, &t_data_ref_type);
		
		short t_res_id;
		t_res_id = 0;
		if (t_error == noErr)
			t_error = NewMovieFromDataRef((Movie *)&theMovie, newMovieActive, &t_res_id, t_data_ref, t_data_ref_type);
			
		if (t_data_ref != NULL)
			DisposeHandle(t_data_ref);
			
		if (t_cf_filename != NULL)
			CFRelease(t_cf_filename);
			
		if (t_resolved_filename != NULL)
			delete t_resolved_filename;

		if (t_filename != NULL)
			delete t_filename;

#elif defined(_WINDOWS_DESKTOP)
		// OK-2009-01-09: [[Bug 1161]] - File resolving code standardized between image and player
		char *t_windows_filename;
		t_windows_filename = getstack() -> resolve_filename(filename);

		FSSpec fspec;
		MCS_path2FSSpec(t_windows_filename, &fspec);
		short refNum;
		if (OpenMovieFile(&fspec, &refNum, fsRdPerm) != noErr)
		{
			freetmp();
			MCresult->sets("could not open movie file");
			return False;
		}
		short mResID = 0;   //want first movie
		Str255 mName;
		Boolean wasChanged;
		NewMovieFromFile((Movie *)&theMovie, refNum, &mResID, mName, newMovieActive, &wasChanged);
		CloseMovieFile(refNum);

		if (t_windows_filename != NULL)
			delete t_windows_filename;
#else
#error qt_prepare not implemented for this platform
#endif
	}
	
	if (theMovie == NULL)
	{
#ifdef _MACOSX
		SetGWorld(oldPort, oldDevice); // Restore previous graphics world
#endif
		freetmp();
		MCresult->sets("could not create movie reference");
		return False;
	}
	
	Rect movieRect, playingRect;
	GetMovieBox((Movie)theMovie, &movieRect);
	MCRectangle trect = resize(RectToMCRectangle(movieRect));

#ifdef _WINDOWS
	hwndMovie = (MCSysWindowHandle)create_player_child_window(trect, (HWND)getstack()->getrealwindow(), MC_QTVIDEO_WIN_CLASS_NAME);
	trect.x = trect.y = 0;
	EnableWindow((HWND)hwndMovie, False);
	ShowWindow((HWND)hwndMovie, SW_SHOW);
	HWND movieWin = (HWND)hwndMovie;
	
	if (!m_has_port_association)
	{
		CreatePortAssociation(movieWin, NULL, kQTMLNoIdleEvents);
		m_has_port_association = true;
	}

	SetMovieGWorld((Movie)theMovie, (CGrafPtr)GetNativeWindowPort((void *)movieWin), NULL);
#elif defined _MACOSX
	SetMovieGWorld((Movie)theMovie, GetWindowPort((WindowPtr)getstack()->getqtwindow()), GetMainDevice());
#endif
	
	// MW-2011-11-30: [[ Bug 9887 ]] Make sure the player rect is adjusted to account for
	//   menubar.
	trect.y -= getstack() -> getscroll();
	
	//set the movie playing rect
	playingRect.left = trect.x;
	playingRect.top = trect.y;
	playingRect.right = trect.x + trect.width;
	playingRect.bottom = trect.y + trect.height;

	long mcflag = 0;

	if (!(flags & F_SHOW_CONTROLLER))
		mcflag |= mcNotVisible; //set invisible flag on for Movie Controller
	if (flags & F_SHOW_BADGE) //show movie controller bages, if specified
		mcflag |= mcWithBadge;

	// Attach a movie controller
	SetMovieBox((Movie)theMovie, &playingRect);
	theMC = NewMovieController((Movie)theMovie, &playingRect, mcflag);

	setMCposition((const MCRectangle&)trect);
	SetMovieTimeValue((Movie)theMovie, lasttime);
	MCDoAction((MovieController)theMC, mcActionSetDragEnabled, (void*)False);
	MCDoAction((MovieController)theMC, mcActionSetKeysEnabled,  (void*)((flags & F_TRAVERSAL_ON) != 0));
	MCDoAction((MovieController)theMC, mcActionSetUseBadge, (void*)((flags & F_SHOW_BADGE) != 0));
	MCDoAction((MovieController)theMC, mcActionSetLooping, (void*)((flags & F_LOOPING) != 0));
	MCEnableEditing((MovieController)theMC, (flags & F_SHOW_SELECTION) != 0);
	setselection(); //set or unset selection
	MCDoAction((MovieController)theMC, mcActionSetPlaySelection,
	           (void*)((flags & F_PLAY_SELECTION) != 0));
	MCMovieChanged((MovieController)theMC, (Movie)theMovie);
	MCSetActionFilterWithRefCon((MovieController)theMC, controllerMsgFilter, (long)this);
	// check for missing controller visual
	state |= CS_PREPARED;
	Rect tr;
	MCGetControllerBoundsRect((MovieController)theMC, &tr);
	if (flags & F_SHOW_CONTROLLER && tr.bottom < trect.y + trect.height
	        && movieRect.bottom)
	{
		flags &= ~F_SHOW_CONTROLLER;
		showcontroller(False);
	}
	isinteractive =  (QTMovieHasType((Movie)theMovie, FlashMediaType)
	                  || QTMovieHasType((Movie)theMovie, SpriteMediaType)
	                  || QTMovieHasType((Movie)theMovie, kQTVRQTVRType));
	if (QTMovieHasType((Movie)theMovie, kQTVRQTVRType))
	{
		initqtvr();
		if (qtvrstate == QTVR_INITTED)
		{
			Track trak = QTVRGetQTVRTrack((Movie)theMovie, 1);
			if (trak != NULL)
			{
				QTVRGetQTVRInstance((QTVRInstance *)&qtvrinstance,trak, (MovieController)theMC);
				QTVRGoToNodeID((QTVRInstance)qtvrinstance, kQTVRDefaultNode);
				QTVRSetEnteringNodeProc((QTVRInstance)qtvrinstance,enterNodeCallback, (SInt32)this, 0);
				QTVRInstallInterceptProc((QTVRInstance)qtvrinstance, kQTVRTriggerHotSpotSelector, clickHotSpotCallback, (SInt32)this, 0);
				}
			}
		}
	stopMovieCB = NewCallBack(GetMovieTimeBase((Movie)theMovie), callBackAtExtremes);
	deleteUserCallbacks();  //delete user define callbacks.
	installUserCallbacks(); //installs user defined callbacks.
	CallMeWhen((QTCallBack)stopMovieCB, MovieEndCallback, (long)this, triggerAtStop , 0,
			   GetMovieTimeScale((Movie)theMovie));
#ifdef _MACOSX
	SetGWorld(oldPort, oldDevice);     // Restore previous graphics world
#else
#endif

	// MW-2006-07-26: [[ Bug 3645 ]] - We buffer here in the case of a buffered player, this prevents
	//   the player drawing itself somewhere else...
  if (getflag(F_ALWAYS_BUFFER) || !MCModeMakeLocalWindows())
		bufferDraw(m_offscreen != NULL);

	MCSetVisible((MovieController)theMC, getflag(F_VISIBLE) && getflag(F_SHOW_CONTROLLER));
	
	// MW-2011-10-24: [[ Bug 9800 ]] Make sure we force a redraw.
	layer_redrawall();

	setloudness();

	MCresult->clear(False);
	return True;
}

Boolean MCPlayer::qt_playpause(Boolean on)
{
	if (!on)
	{
		short denominator = (short)(MAXUINT1 / rate); //compute play rate
		short numerator = (short)(rate * denominator);
		if ((OSErr)MCDoAction((MovieController)theMC, mcActionPrerollAndPlay, (void*)FixRatio(numerator, denominator)) != noErr)
			return False;
	}
	else
	{
		StopMovie((Movie)theMovie);
		MCMovieChanged((MovieController)theMC, (Movie)theMovie);
	}

	return True;
}

void MCPlayer::qt_playstepforward(void)
{
	MCDoAction((MovieController)theMC, mcActionStep, (long *)0);
}

void MCPlayer::qt_playstepback(void)
{
	MCDoAction((MovieController)theMC, mcActionStep, (long *)-1);
}

Boolean MCPlayer::qt_playstop(void)
{
	Boolean needmessage;

	if (isbuffering())
		unbufferDraw();

	deleteUserCallbacks(); //delete user defined callbacks for this player
	DisposeCallBack((QTCallBack)stopMovieCB);
			
	if (!ispaused())
		MCDoAction((MovieController)theMC, mcActionPlay, (void*)0);  //stop playing

	needmessage = getduration() > getmoviecurtime();
	if (theMC != NULL)
	{
		DisposeMovieController((MovieController)theMC);
		theMC = NULL;
	}

	if (theMovie != NULL)
	{
		DisposeMovie((Movie)theMovie);
		theMovie = NULL;
	}

	if (qtvrinstance != NULL)
	{
		QTVRSetEnteringNodeProc((QTVRInstance)qtvrinstance,NULL, 0, 0);
		QTVRInstallInterceptProc((QTVRInstance)qtvrinstance, kQTVRTriggerHotSpotSelector,
				                      NULL, 0, 0);
		qtvrinstance = NULL;
	}

#ifdef _WINDOWS
	DestroyWindow((HWND)hwndMovie);
	hwndMovie = NULL;
	if (m_has_port_association)
	{
		DestroyPortAssociation((CGrafPtr)GetHWNDPort((HWND)hwndMovie));
		m_has_port_association = false;
	}
#endif

	return needmessage;
}

void MCPlayer::qt_setrect(const MCRectangle& nrect)
{
	bool t_resized;
	t_resized = nrect . width != rect . width || nrect . height != rect . height;
	rect = nrect;

	if (isbuffering())
	{
		if (t_resized)
			bufferDraw(true);
	}
	else
	{
		MCRectangle trect = MCU_reduce_rect(rect, getflag(F_SHOW_BORDER) ? borderwidth : 0);
#ifdef _WINDOWS
		MoveWindow((HWND)hwndMovie, trect.x, trect.y, trect.width, trect.height, False);
		trect.x = trect.y = 0;
#endif
		// MW-2011-11-30: [[ Bug 9887 ]] Make sure the player rect is adjusted to account for
		//   menubar.
		trect . y -= getstack() -> getscroll();
		setMCposition(trect);
	}
}

uint4 MCPlayer::qt_getduration(void)
{
	TimeValue result = GetMovieDuration((Movie)theMovie);
	if (result < 0)
	{
		MCS_seterrno(-2010);
		return 0;
	}
	else
		return result;
}

uint4 MCPlayer::qt_gettimescale(void)
{
	TimeValue result = GetMovieTimeScale((Movie)theMovie);
	if (result == -2010)
	{//invalid movie
		MCS_seterrno(result); //60 = sixtieths of a second, 1000 = milliseconds
		return 0;
	}
	else
		return result;
}

uint4 MCPlayer::qt_getmoviecurtime(void)
{
  if (getstate(CS_PREPARED))
		return GetMovieTime((Movie)theMovie, nil);

	return lasttime;
}

void MCPlayer::qt_setcurtime(uint4 newtime)
{
	if (getstate(CS_PREPARED))
	{
		SetMovieTimeValue((Movie)theMovie, newtime);
		MCMovieChanged((MovieController)theMC, (Movie)theMovie);
		if (isbuffering())
#ifdef _WINDOWS
			MCDoAction((MovieController)theMC, mcActionDraw, bufferGW);
#else
			MCDoAction((MovieController)theMC, mcActionDraw, m_offscreen->gworld);
#endif

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

void MCPlayer::qt_setselection(void)
{
	uint4 st, et;
	if (starttime == MAXUINT4 || endtime == MAXUINT4)
		st = et = 0;
	else
	{
		st = starttime;
		et = endtime;
	}

	uint4 oldstate = state;
	state |= CS_NO_MESSAGES;
	
	TimeRecord tr;
	tr.value.hi = 0;
	tr.value.lo = st;
	tr.base = 0;
	tr.scale = GetMovieTimeScale((Movie)theMovie);
	MCDoAction((MovieController)theMC, mcActionSetSelectionBegin, &tr);
	tr.value.lo = et - st;
	MCDoAction((MovieController)theMC, mcActionSetSelectionDuration, &tr);
	
	state = oldstate;
}

void MCPlayer::qt_setlooping(Boolean loop)
{
	MCDoAction((MovieController)theMC, mcActionSetLooping, (void *)loop);
	if (getstate(CS_PAUSED))
		StopMovie((Movie)theMovie);
}

void MCPlayer::qt_setplayrate(void)
{
	short denominator = (short)(MAXUINT1 / rate);
	short numerator = (short)(rate * denominator);

	SetMovieRate((Movie)theMovie, rate == 0 ? 0 :FixRatio(numerator, denominator));
	MCMovieChanged((MovieController)theMC, (Movie)theMovie);
	
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCPlayer::qt_showbadge(Boolean show)
{
	MCDoAction((MovieController)theMC, mcActionSetUseBadge, (void *)show);
}

void MCPlayer::qt_editmovie(Boolean edit)
{
	MCEnableEditing((MovieController)theMC, edit);
}

void MCPlayer::qt_playselection(Boolean play)
{
	MCDoAction((MovieController)theMC, mcActionSetPlaySelection, (void *)play);
}

Boolean MCPlayer::qt_ispaused(void)
{
	if (getstate(CS_PREPARED))
	{
		long t_flags;
		MCGetControllerInfo((MovieController)theMC, &t_flags);
		return (!(t_flags & mcInfoIsPlaying));
	}

	return True;
}

void MCPlayer::qt_showcontroller(Boolean show)
{
	MCRectangle drect = rect;
	MCRectangle oldrect = rect;
	if (show)
	{
		if (state & CS_PREPARED && formattedheight == 0) // audio clip
			drect.height = 16;
		else
			drect.height += 16;
		if (state & CS_PREPARED)
			MCSetVisible((MovieController)theMC, getflag(F_VISIBLE)); //show controller
		// MW-2011-08-18: [[ Layers ]] Set the rect.
		layer_setrect(drect, true);
	}
	else
	{
		if (state & CS_PREPARED && formattedheight == 0) // audio clip
			drect.height = 16;
		else
			drect.height -= 16;
		if (state & CS_PREPARED)
			MCSetVisible((MovieController)theMC, getflag(F_VISIBLE));//hide controller
		// MW-2011-08-18: [[ Layers ]] Set the rect.
		layer_setrect(drect, true);
	}
}

MCRectangle MCPlayer::qt_getpreferredrect(void)
{
	Rect naturalbounds;
	GetMovieNaturalBoundsRect((Movie)theMovie, &naturalbounds);

	MCRectangle trect;
	trect . x = trect . y = 0;
	trect . height = naturalbounds.bottom - naturalbounds.top;
	trect . width = naturalbounds.right - naturalbounds.left;

	return trect;
}

uint2 MCPlayer::qt_getloudness(void)
{
	uint2 vol;
	MCDoAction((MovieController)theMC, mcActionGetVolume, &vol);
	return (uint2)((vol*100)/255);
}

void MCPlayer::qt_setloudness(uint2 loudn)
{
	uint2 vol;
	vol = (uint2)((loudn * 255) / 100);
	MCDoAction((MovieController)theMC, mcActionSetVolume, (void *)vol);
}

void MCPlayer::qt_gettracks(MCExecPoint& ep)
{
	uint2 trackcount = (uint2)GetMovieTrackCount((Movie)theMovie);
	char buffer[256];
	uint2 i;

	for (i = 1 ; i <= trackcount ; i++)
	{
		Track trak = GetMovieIndTrack((Movie)theMovie,i);
		if (trak == NULL)
			break;
		ep.concatuint(GetTrackID(trak), EC_RETURN, i == 1);//id

		Media media = GetTrackMedia(trak);
		MediaHandler mediahandler = GetMediaHandler(media);
		MediaGetName(mediahandler, (unsigned char *)buffer, 0, NULL);
		p2cstr((unsigned char*)buffer);

		ep.concatcstring(buffer, EC_COMMA, false);//type
		ep.concatuint((uint4)GetTrackOffset(trak), EC_COMMA, false);//start
		ep.concatuint((uint4)GetTrackDuration(trak), EC_COMMA, false);//end
	}
}

void MCPlayer::qt_getenabledtracks(MCExecPoint& ep)
{
	uint2 trackcount = (uint2)GetMovieTrackCount((Movie)theMovie);
	uint2 i;
	bool first = true;

	for (i = 1 ; i <= trackcount ; i++)
	{
		Track trak = GetMovieIndTrack((Movie)theMovie,i);
		if (trak == NULL)
			break;
		if (GetTrackEnabled(trak))
		{
			ep.concatuint(GetTrackID(trak), EC_RETURN, first);
			first = false;
		}
	}
}

Boolean MCPlayer::qt_setenabledtracks(const MCString& s)
{
	uint2 trackcount = (uint2)GetMovieTrackCount((Movie)theMovie);
	uint2 i;
	for (i = 1 ; i <= trackcount ; i++)
	{//disable all tracks
		Track trak = GetMovieIndTrack((Movie)theMovie,i);
		SetTrackEnabled(trak, False);
	}
	char *data = s.clone();
	char *sptr = data;
	uint2 offset = 0;
	while (*sptr)
	{
		char *tptr;
		if ((tptr = strchr(sptr, '\n')) != NULL)
			*tptr++ = '\0';
		else
			tptr = &sptr[strlen(sptr)];
		if (strlen(sptr) != 0)
		{
			Track trak = GetMovieTrack((Movie)theMovie,strtol(sptr, NULL, 10));
			if (trak == NULL)
			{
				delete data;
				return False;
			}
			SetTrackEnabled(trak, True);
		}
		sptr = tptr;
		if (++offset >= trackcount)
			break;
	}
	delete data;
	MCMovieChanged((MovieController)theMC, (Movie)theMovie);
	Rect movieRect;
	GetMovieBox((Movie)theMovie, &movieRect);
	MCRectangle trect = resize(RectToMCRectangle(movieRect));
	if (flags & F_SHOW_BORDER)
		trect = MCU_reduce_rect(trect, -borderwidth);
	setrect(trect);

	return True;
}

#if defined(TARGET_PLATFORM_MACOS_X)
inline void MCRect2MacRect(const MCRectangle& p_rect, Rect& r_rect)
{
	r_rect . left = p_rect . x;
	r_rect . top = p_rect . y;
	r_rect . right = p_rect . x + p_rect . width;
	r_rect . bottom = p_rect . y + p_rect . height;
}
#endif

void MCPlayer::qt_draw(MCDC *dc, const MCRectangle& dirty)
{
	if (theMC == NULL)
		return;

	MCSetVisible((MovieController)theMC, getflag(F_VISIBLE) && getflag(F_SHOW_CONTROLLER));

#if defined(TARGET_PLATFORM_WINDOWS)
	MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);
	
	// MW-2011-09-23: Sync the buffering state.
	syncbuffering(dc);

	if (isbuffering())
	{
		MCImageDescriptor t_image;
		MCMemoryClear(&t_image, sizeof(t_image));
		t_image.filter = kMCGImageFilterNearest;
		t_image.bitmap = &m_offscreen->image_bitmap;

		dc -> drawimage(t_image, 0, 0, trect.width, trect.height, trect.x, trect.y);
	}
	else
	{
		MCDraw((MovieController)theMC, GetHWNDPort(hwndMovie));
	}
#elif defined(TARGET_PLATFORM_MACOS_X)
	MCRectangle trect = MCU_reduce_rect(rect, flags & F_SHOW_BORDER ? borderwidth : 0);

	// MW-2011-09-23: Sync the buffering state.
	syncbuffering(dc);

	if (isbuffering())
	{
		PixMapHandle t_pixmap;
		t_pixmap = GetGWorldPixMap(m_offscreen->gworld);
		LockPixels(t_pixmap);
		
		m_offscreen->image_bitmap.data = (uint32_t*)GetPixBaseAddr(t_pixmap);
		m_offscreen->image_bitmap.stride = GetPixRowBytes(t_pixmap);
		
		MCImageDescriptor t_image;
		MCMemoryClear(&t_image, sizeof(t_image));
		t_image.filter = kMCGImageFilterNearest;
		t_image.bitmap = &m_offscreen->image_bitmap;
		
		dc -> drawimage(t_image, 0, 0, trect.width, trect.height, trect.x, trect.y);

		UnlockPixels(t_pixmap);
	}
	else
	{
		// MW-2011-11-23: [[ Bug ]] Players are now redrawn after compositing the rest
		//   of the content if they aren't buffered, thus making rendering here redundent.
	}
#else
#error Implement qt_draw
#endif
}

#ifdef _WINDOWS
OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec)
{ //For QT movie only
	char *filename = MCS_resolvepath(fname);
	if (filename[1] != ':' && filename[0] != '/')
	{//not c:/mc/xxx, not /mc/xxx
		char *tpath = MCS_getcurdir();
		if (strlen(fname) + strlen(tpath) < PATH_MAX)
		{
			// MW-2005-01-25: If the current directory is the root of a volume then it *does*
			//   have a path separator so we don't need to add one
			if (tpath[strlen(tpath) - 1] != '/')
				strcat(tpath, "/");
			strcat(tpath, filename);
			delete filename;
			MCU_path2native(tpath);
			filename = tpath;
		}
	}
	OSErr err = NativePathNameToFSSpec(filename, fspec, 0);
	delete filename;
	return err;
}
#endif

//
// QuickTime Player Implementation
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// AVI Player Implementation
//

#ifdef _WINDOWS

HWND create_player_child_window(MCRectangle &p_rect, HWND p_parent, LPCSTR p_window_class)
{
	return CreateWindowExA(0, p_window_class, NULL, WS_CHILD,
	                           p_rect.x, p_rect.y, p_rect.width, p_rect.height,
	                           p_parent, NULL, MChInst, NULL);
}

Boolean MCPlayer::avi_prepare(void)
{
	MCI_DGV_OPEN_PARMSA mciOpen;
	memset(&mciOpen, 0, sizeof(MCI_DGV_OPEN_PARMSA));
	mciOpen.lpstrDeviceType = "AVIVideo";

	// OK-2009-01-09: [[Bug 1161]] - File resolving code standardized between image and player
	char *t_resolved_filename;
	t_resolved_filename = getstack() -> resolve_filename(filename);

	mciOpen.lpstrElementName = t_resolved_filename;
	mciOpen.dwStyle = WS_CHILD;
	mciOpen.hWndParent = (HWND)getstack()->getrealwindow();
	//if lpstrDeviceType is NULL, then MCI_OPEN_TYPE should not be
	//specified. MCI will automatically figure out the device to use
	//by looking at the file extension.  can't get MCI_OPEN_SHAREABLE flag
	//to work. This limits to ONE movie can only play in ONE player
	if (mciSendCommandA(0, MCI_OPEN,
	                   MCI_OPEN_ELEMENT | MCI_DGV_OPEN_PARENT | MCI_DGV_OPEN_WS,
	                   (DWORD)(LPSTR)&mciOpen) != 0)
	{
		delete t_resolved_filename;
		MCresult->sets("could not open video player");
		return False;
	}

	deviceID = mciOpen.wDeviceID;
	delete t_resolved_filename;

	// Get movie dimensions (cropped from the frame buffer) that is
	//stretched to fit the destination rectangle on the display
	MCI_DGV_RECT_PARMS mciRect;
	mciSendCommandA(deviceID, MCI_WHERE, MCI_DGV_WHERE_SOURCE,
	               (DWORD)(LPSTR)&mciRect);
	formattedheight = (uint2)mciRect.rc.bottom;
	formattedwidth = (uint2)mciRect.rc.right;

	// Note that the Right and Bottom members of RECT structures in MCI
	// are unusual; rc.right is set to the rectangle's width, and rc.bottom
	// is set to the rectangle's height.
	MCRectangle trect = rect;
	if (!(flags & F_LOCK_LOCATION))
	{
		int2 x = trect.x + (trect.width >> 1);
		int2 y = trect.y + (trect.height >> 1);
		trect.width = (uint2)(mciRect.rc.right * scale);
		trect.height = (uint2)(mciRect.rc.bottom * scale);
		trect.x = x - (trect.width >> 1);
		trect.y = y - (trect.height >> 1);
		if (flags & F_SHOW_BORDER)
			rect = MCU_reduce_rect(trect, -borderwidth);
		else
			rect = trect;
	}
	else
		if (flags & F_SHOW_BORDER)
			trect = MCU_reduce_rect(trect, borderwidth);

	// Create the playing window. Make it bigger for the border
	// Center the movie in the playing window
	hwndMovie = (MCSysWindowHandle)create_player_child_window(trect, (HWND)getstack()->getrealwindow(), MC_VIDEO_WIN_CLASS_NAME);
	if (hwndMovie == NULL)
	{
		MCresult->sets("could not create movie window");
		return False;
	}
	EnableWindow((HWND)hwndMovie, False); //disable child window, so the msgs go to parent window
	//playing window created O.K. make it the playback window
	MCI_DGV_WINDOW_PARMSA mciWindow;
	memset(&mciWindow, 0, sizeof(MCI_DGV_WINDOW_PARMSA));
	mciWindow.hWnd = (HWND)hwndMovie;
	mciSendCommandA(deviceID, MCI_WINDOW, MCI_DGV_WINDOW_HWND,
	               (DWORD)(LPSTR)&mciWindow); //associate window handle with the device

	MCI_DGV_PUT_PARMS mciPut;
	memset(&mciPut, 0, sizeof(MCI_DGV_PUT_PARMS));
	mciPut.rc.left = 0; //relative to hwndMovie's coord
	mciPut.rc.top = 0;  //relative to hwndMovie's coord
	mciPut.rc.right = trect.width;
	mciPut.rc.bottom = trect.height;
	mciSendCommandA(deviceID, MCI_PUT, MCI_DGV_RECT | MCI_DGV_PUT_DESTINATION,
	               (DWORD)(LPSTR)&mciPut);

	if ((flags & F_PLAY_SELECTION) && endtime != 0)
		setAVIplayselection(True);      //play selection only
	else
		setAVIplayselection(False); //play the entire movie

	mciPlayFlag = MCI_NOTIFY;
	if (flags & F_LOOPING)
		mciPlayFlag |= MCI_DGV_PLAY_REPEAT;

	AVIseek(lasttime); //set AVI movie current postion
	ShowWindow((HWND)hwndMovie, SW_SHOW); //show the playback window
	MCStack *sptr = NULL;
	if (MCscreen->getdepth() == 8)
	{ // bug in NT, need to force bg palette
		sptr = MCfocusedstackptr;
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		SetFocus(pms->getinvisiblewindow());
	}
	MCscreen->expose();

	//set movie's unit format to be in milliseconds
	MCI_SET_PARMS set;
	memset(&set, 0, sizeof(MCI_SET_PARMS));
	set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
	mciSendCommandA(deviceID, MCI_SET,
	               MCI_SET_TIME_FORMAT | MCI_FORMAT_MILLISECONDS,
	               (DWORD)(LPMCI_SET_PARMS)&set
	              );
	setloudness();

	if (sptr != NULL)
		SetFocus((HWND)sptr->getstack()->getrealwindow());
	MCresult->clear(False);
	return True;
}

Boolean MCPlayer::avi_playpause(Boolean on)
{
	if (!on)
	{
		MCI_STATUS_PARMS status; //no need to deal with the lasttime issue.
		memset(&status, 0, sizeof(MCI_STATUS_PARMS));
		status.dwItem = MCI_STATUS_MODE;
		mciSendCommandA(deviceID, MCI_STATUS, MCI_STATUS_ITEM,
						  (DWORD)(LPMCI_STATUS_PARMS)&status);

		if (status.dwReturn == MCI_MODE_PAUSE)
		{
			MCI_GENERIC_PARMS gp;
			memset(&gp, 0, sizeof(MCI_GENERIC_PARMS));
			if (mciSendCommandA(deviceID, MCI_RESUME, MCI_NOTIFY, (DWORD)&gp) != 0)
				return False;
		}
		else
		{//play from the begining
			MCIERROR err;
			MCI_DGV_PLAY_PARMS mciPlay;
			mciPlay.dwCallback = (DWORD)(getstack()->getrealwindow());
			mciPlay.dwFrom = mciPlayFrom;
			mciPlay.dwTo = mciPlayTo;
			if (flags & F_PLAY_SELECTION)
			{ //play selection only
				setAVIplayselection(True);
				mciPlayFlag = mciPlayFlag ;
				err = mciSendCommandA(deviceID, MCI_PLAY, mciPlayFlag,
										(DWORD)(LPVOID)&mciPlay);
			}
			else
			{ //play entire movie
				setAVIplayselection(False);
				err = mciSendCommandA(deviceID, MCI_PLAY, mciPlayFlag,
										(DWORD)(LPVOID)&mciPlay);
			}
			if (err != 0)
			{
				char xx[130];
				mciGetErrorStringA(err, xx, 130);
				MCI_GENERIC_PARMS mciGen;
				memset(&mciGen, 0, sizeof(MCI_GENERIC_PARMS));
				mciSendCommandA(deviceID, MCI_CLOSE, 0, (DWORD)(LPVOID)&mciGen);
				DestroyWindow((HWND)hwndMovie);
				hwndMovie = NULL;
				return False;
			}
		}   //play AVI movie from begining
	}
	else
	{
		if (!ispaused())
		{
			MCI_DGV_PAUSE_PARMS pause;
			memset(&pause, 0, sizeof(MCI_DGV_PAUSE_PARMS));
			if (mciSendCommandA(deviceID, MCI_PAUSE, 0, (DWORD)(LPVOID)&pause) != 0)
				return False;
		}
	}

	return True;
}

void MCPlayer::avi_playstepforward(void)
{
	MCI_DGV_STEP_PARMS step;
	memset(&step, 0, sizeof(MCI_DGV_STEP_PARMS));
	step.dwFrames = 1;
	mciSendCommandA(deviceID, MCI_STEP, MCI_DGV_STEP_FRAMES, (DWORD)(LPSTR)&step);
}

void MCPlayer::avi_playstepback(void)
{
	MCI_DGV_STEP_PARMS step;
	memset(&step, 0, sizeof(MCI_DGV_STEP_PARMS));
	step.dwFrames = 1;
	mciSendCommandA(deviceID, MCI_STEP, MCI_DGV_STEP_FRAMES | MCI_DGV_STEP_REVERSE, (DWORD)(LPSTR)&step);
}

Boolean MCPlayer::avi_playstop(void)
{
	if (hwndMovie != NULL && mode_avi_closewindowonplaystop())
	{ //destroy the playback window
		DestroyWindow((HWND)hwndMovie);
		hwndMovie = NULL;
	}

	if (deviceID != 0)
	{ //send msg to mci to close the device
		MCI_GENERIC_PARMS mciClose;
		memset(&mciClose, 0, sizeof(MCI_GENERIC_PARMS));
		if (mciSendCommandA(deviceID, MCI_CLOSE, 0, (DWORD)(LPSTR)&mciClose) != 0)
			return False;
	}

	return True;
}

void MCPlayer::avi_setrect(const MCRectangle& nrect)
{
	rect = nrect;

	if (!hwndMovie)
		return;

	MoveWindow((HWND)hwndMovie,rect.x,rect.y,rect.width,rect.height,True);
	MCI_DGV_PUT_PARMS mciPut;
	memset(&mciPut, 0, sizeof(MCI_DGV_PUT_PARMS));
	mciPut.rc.left = 0; //relative to hwndMovie's coord
	mciPut.rc.top = 0;  //relative to hwndMovie's coord
	mciPut.rc.right = rect.width;
	mciPut.rc.bottom = rect.height;
	mciSendCommandA(deviceID, MCI_PUT, MCI_DGV_RECT | MCI_DGV_PUT_DESTINATION, (DWORD)(LPSTR)&mciPut);
	
	MCI_DGV_UPDATE_PARMS gp;
	memset(&gp, 0, sizeof(MCI_DGV_UPDATE_PARMS));
	gp.dwCallback = (DWORD)hwndMovie;
	gp.hDC = GetDC((HWND)hwndMovie);
	gp.rc.left = 0;
	gp.rc.right = 0;
	gp.rc.right = rect.width;
	gp.rc.bottom = rect.height;
	mciSendCommandA(deviceID, MCI_UPDATE, MCI_DGV_UPDATE_HDC | MCI_DGV_UPDATE_PAINT , (DWORD)&gp);
	ReleaseDC((HWND)hwndMovie, gp.hDC);
}

uint4 MCPlayer::avi_getduration(void)
{
	if (getstate(CS_PREPARED))
	{
		MCI_STATUS_PARMS status;
		memset(&status, 0, sizeof(MCI_STATUS_PARMS));
		status.dwItem = MCI_STATUS_LENGTH;
		mciSendCommandA(deviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPSTR) &status);
		return status.dwReturn;
	}

	return 0;
}

uint4 MCPlayer::avi_gettimescale(void)
{
	return 1000;
}

uint4 MCPlayer::avi_getmoviecurtime(void)
{
	if (getstate(CS_PREPARED))
	{
		MCI_STATUS_PARMS status; //no need to deal with the lasttime issue.
		memset(&status, 0, sizeof(MCI_STATUS_PARMS));
		status.dwItem = MCI_STATUS_POSITION;
		mciSendCommandA(deviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPMCI_STATUS_PARMS)&status);
		return status.dwReturn;
	}

	return lasttime;
}

void MCPlayer::avi_setcurtime(uint4 newtime)
{
	if (getstate(CS_PREPARED))
		AVIseek(newtime);
}

void MCPlayer::avi_setselection(void)
{
	setAVIplayselection(getflag(F_PLAY_SELECTION));
}

void MCPlayer::avi_setlooping(Boolean loop)
{
	if (loop)
		mciPlayFlag |= MCI_DGV_PLAY_REPEAT;
	else
		mciPlayFlag &= ~ MCI_DGV_PLAY_REPEAT;
}

void MCPlayer::avi_setplayrate(void)
{
	MCI_DGV_SET_PARMS set;
	memset(&set, 0, sizeof(MCI_DGV_SET_PARMS));
	set.dwSpeed = (DWORD)rate * 1000; //or should it be 1000 / rate?
	mciSendCommandA(deviceID, MCI_SET, MCI_DGV_SET_SPEED, (DWORD)(LPMCI_SET_PARMS)&set);
}

void MCPlayer::avi_showbadge(Boolean show)
{
}

void MCPlayer::avi_editmovie(Boolean edit)
{
}

void MCPlayer::avi_playselection(Boolean play)
{
	setAVIplayselection(play);
}

Boolean MCPlayer::avi_ispaused(void)
{
	if (!getstate(CS_PREPARED))
		return True;

	MCI_STATUS_PARMS status; //no need to deal with the lasttime issue.
	memset(&status, 0, sizeof(MCI_STATUS_PARMS));
	status.dwItem = MCI_STATUS_MODE;
	mciSendCommandA(deviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)(LPMCI_STATUS_PARMS)&status);
	
	return status.dwReturn == MCI_MODE_PAUSE || status.dwReturn == MCI_MODE_STOP;
}

void MCPlayer::avi_showcontroller(Boolean show)
{
}

MCRectangle MCPlayer::avi_getpreferredrect(void)
{
	MCRectangle trect;
	trect.x = trect.y = 0;
	trect.height = formattedheight;
	trect.width = formattedwidth;
	return trect;
}

uint2 MCPlayer::avi_getloudness(void)
{
	return loudness;
}

void MCPlayer::avi_setloudness(uint2 loudn)
{
	MCI_DGV_SETAUDIO_PARMSA sap;
	memset(&sap, 0, sizeof(MCI_DGV_SETAUDIO_PARMSA));
	sap.dwItem = MCI_DGV_SETAUDIO_VOLUME;
	sap.dwValue = loudn * 10; // scaled between 0 - 1000
	mciSendCommandA(deviceID, MCI_SETAUDIO,
			           MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM,
			           (DWORD)(LPMCI_GENERIC_PARMS)&sap);
}

void MCPlayer::avi_gettracks(MCExecPoint& ep)
{
}

void MCPlayer::avi_getenabledtracks(MCExecPoint& ep)
{
}

Boolean MCPlayer::avi_setenabledtracks(const MCString& s)
{
	return True;
}

void MCPlayer::avi_draw(MCDC *dc, const MCRectangle& dirty)
{
	MCI_DGV_UPDATE_PARMS gp;
	memset(&gp, 0, sizeof(MCI_DGV_UPDATE_PARMS));
	gp.dwCallback = (DWORD)hwndMovie;
	gp.hDC = GetDC((HWND)hwndMovie);
	mciSendCommandA(deviceID, MCI_UPDATE, MCI_DGV_UPDATE_HDC, (DWORD)&gp);
	ReleaseDC((HWND)hwndMovie, gp.hDC);
}

void MCPlayer::setAVIplayselection(Boolean selectionOnly)
{
	uint4 t_currtime = avi_getmoviecurtime();
	uint4 t_duration = avi_getduration();
	uint4 t_starttime = 0;
	uint4 t_endtime = t_duration;
	if (selectionOnly)
	{
		if (starttime != MAXUINT4)
		{
			t_starttime = starttime;
		}
		if (endtime != MAXUINT4)
		{
			t_endtime = endtime;
		}
	}
	if (t_currtime > t_starttime && t_currtime < t_endtime)
		t_starttime = t_currtime;
	mciPlayFrom = t_starttime;
	mciPlayTo = endtime;
	if (t_starttime != t_currtime)
	{
		mciPlayFlag |= MCI_FROM;
	}
	if (t_endtime != t_duration)
	{
		mciPlayFlag |= MCI_TO;
	}
	AVIseek(mciPlayFrom);
}

Boolean MCPlayer::AVIseek(uint4 to)
{
	MCI_SEEK_PARMS seek; //seek to the designated position
	memset(&seek, 0, sizeof(MCI_SEEK_PARMS));
	seek.dwTo = to;
	return (mciSendCommandA(deviceID, MCI_SEEK, MCI_TO, (DWORD)(LPMCI_SEEK_PARMS)&seek) == 0);
}

#endif

//
// AVI Player Implementation
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// QuickTime Specific Utility Functions
//

static Boolean IsQTVRInstalled(void)
{
	static Boolean t_installed = False;
	OSErr myErr;
	long myAttrs;
	myErr = Gestalt(gestaltQTVRMgrAttr, &myAttrs);
	if (myErr == noErr)
		if (myAttrs & (1 << gestaltQTVRMgrPresent))
			t_installed = True;
	return t_installed;

}

static Boolean IsQTVRMovie(Movie theMovie)
{
	Boolean IsQTVR = False;
	OSType evaltype,targettype =  kQTVRUnknownType;
	UserData myUserData;
	if (theMovie == NULL)
		return False;
	myUserData = GetMovieUserData(theMovie);
	if (myUserData != NULL)
	{
		GetUserDataItem(myUserData, &targettype, sizeof(targettype),
		                kUserDataMovieControllerType, 0);
		evaltype = EndianU32_BtoN(targettype);
		if (evaltype == kQTVRQTVRType || evaltype == kQTVROldPanoType
		        || evaltype == kQTVROldObjectType)
			IsQTVR = true;
	}
	return(IsQTVR);
}

static Boolean QTMovieHasType(Movie tmovie, OSType movtype)
{
	switch (movtype)
	{
	case VisualMediaCharacteristic:
	case AudioMediaCharacteristic:
		return (GetMovieIndTrackType(tmovie, 1, movtype,
		                             movieTrackCharacteristic) != NULL);
	case kQTVRQTVRType:
		return IsQTVRMovie(tmovie);
	default:
		return (GetMovieIndTrackType(tmovie, 1, movtype,
		                             movieTrackMediaType) != NULL);
	}
}

//
// QuickTime Specific Utility Functions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// QuickTime Specific Callback Implementations
//

// This is the callback table entry structure
typedef struct _UserCallback UserCallbackStruct;
struct _UserCallback
{
	QTCallBack cb;        //pointer to call back
	MCPlayer *playerObj;  //which player object
	MCNameRef msg;            //user msg, when a call back is invoked
	char *param;
	uint4 calledAtTime;   //movie time, when the call back is invoked
	long timeScale;
	Boolean wascalled;
};

// The callback table is stored globally in a single array.
static UserCallbackStruct *callbacktable = NULL;
static uint2 ncallbacks = 0; 

// This callback is triggered when a user-defined call point is reached.
//   mcb - the relevant QTCallback object
//   index - the entry in our 'callbacktable'
//
static pascal void userMovieCallbacks(QTCallBack mcb, long index)
{
	if (mcb == NULL)
		return;

	callbacktable[index].wascalled = True;
	MCPlayer *tplayer = (MCPlayer *)callbacktable[index].playerObj;

// MW-2006-08-14: This seems to call undue problems with callbacks being invoked.
//   Indeed, callbacks should *always* be invoked regardless of whether their time
//   has passed.

//	int4 tdiff = callbacktable[index].calledAtTime - tplayer->getmoviecurtime();
//	uint4 ztime = tplayer->gettimescale();
//	if (MCU_abs(tdiff) < (ztime / 15) )
		MCscreen->delaymessage(callbacktable[index].playerObj, callbacktable[index].msg, strclone(callbacktable[index].param), NULL);
}

// This callback is triggered when the end of the movie is reached.
//   mycb - the relevant QTCallback object
//   player - the target player
static pascal void MovieEndCallback(QTCallBack mycb, long player)
{
	MCPlayer *tplayer = (MCPlayer *)player;
	tplayer->reloadcallbacks(True, 0);
}

// This callback is triggered whenever the movie's frame is redrawn.
//   p_movie - the target movie
//   p_reference - a pointer to the target player
static pascal OSErr qt_movie_drawing_completion_callback(Movie p_movie, long p_reference)
{
	MCPlayer *t_player = (MCPlayer *)p_reference;
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	t_player -> layer_redrawall();
	return noErr;
}

// This callback is triggered whenever the MovieController performs
// a user-initiated action.
static pascal Boolean controllerMsgFilter(MovieController mc, short action, void *params, long player)
{
	MCPlayer *tplayer = (MCPlayer *)player;

#ifdef _MACOSX
	Boolean wake = False;
	TimeRecord *t = (TimeRecord *)params;
	CGrafPtr oldPort;
	GDHandle oldDevice;
	switch (action)
	{
	case mcActionSetSelectionBegin:
		if (t->value.lo != tplayer->getstarttime())
		{
			tplayer->setstarttime(t->value.lo);
			GetGWorld(&oldPort, &oldDevice);
			tplayer->message(MCM_selection_changed);
			wake = True;
		}
		break;

	case mcActionSetSelectionDuration:
		if (t->value.lo != tplayer->getendtime() - tplayer->getstarttime())
		{

			if ((int4)t->value.lo < 0)
			{
				tplayer->setendtime(tplayer->getstarttime());
				tplayer->setstarttime(tplayer->getstarttime() + t->value.lo);
			}

			else
				tplayer->setendtime(tplayer->getstarttime() + t->value.lo);

			GetGWorld(&oldPort, &oldDevice);
			tplayer->message(MCM_selection_changed);
			wake = True;
		}
		break;

	case mcActionGoToTime:
		if (t->value.lo != (uint4)tplayer->getlasttime())
		{
			tplayer->setlasttime(t->value.lo);
			GetGWorld(&oldPort, &oldDevice);
			tplayer->message_with_args(MCM_current_time_changed, t->value.lo);
			wake = True;
		}
		break;
#else

	switch (action)
	{
	case mcActionSetSelectionBegin:
	case mcActionSetSelectionDuration:
	case mcActionGoToTime:
		// MW-2005-02-20: Prevent messages being sent if the callback wasn't
		//   user initiated.
		if (!tplayer -> getstate(CS_NO_MESSAGES))
			MCscreen->addmessage(tplayer, MCM_internal, MCS_time(), NULL);
		break;
#endif

	case mcActionPlay:
		if ((long)params != 0)
		{//play forward > 0, play backward < 0, play stop == 0
			if (tplayer -> getduration() <= tplayer -> getmoviecurtime())
				tplayer -> reloadcallbacks(False, 0);
			else
				tplayer->reloadcallbacks(False, params < 0 ? -tplayer -> getmoviecurtime() : tplayer -> getmoviecurtime());
			MCscreen->addmessage(tplayer, MCM_play_started, MCS_time(), NULL);
			tplayer -> setstate(False, CS_PAUSED);
		}
		else
		{
			// there has to be a distinction between play-stopped and play-paused,
			// if a movie is paused and is disposable, sending play-stop msg will
			// kill the the movie
			tplayer -> setstate(True, CS_PAUSED);
			if (tplayer->getduration() > tplayer->getmoviecurtime())
				MCscreen->addmessage(tplayer, MCM_play_paused, MCS_time(), NULL);
			else  //stopped
				MCscreen->addmessage(tplayer, MCM_play_stopped, MCS_time(), NULL);
		}
		break;
	case mcActionShowMessageString:
		if (params != NULL)
		{
			char *m = strclone(p2cstr((unsigned char *)params));
			c2pstr((char *)params);
			MCParameter *p = new MCParameter;
			p->setbuffer(m, strlen(m));
			MCscreen->addmessage(tplayer, MCM_qtdebugstr, MCS_time(), p);
		}
		break;
	case mcActionMovieClick:
	case mcActionSuspend:
	case mcActionResume:
	case mcActionDeactivate:
		return tplayer->isInteractive() ? False : True;
	}
#ifdef _MACOSX
	if (wake)
	{
		SetGWorld(oldPort, oldDevice);
		ProcessSerialNumber mypsn;
		GetCurrentProcess(&mypsn);
		WakeUpProcess(&mypsn);
	}
#endif
	return False;
}

// This is the QTVR node-entering callback
static pascal OSErr enterNodeCallback(QTVRInstance theInstance, UInt32 nodeid, SInt32 player)
{
	OSErr err = noErr;
	char *m = new char[U2L];
	sprintf(m, "%u", (unsigned int)nodeid);
	MCParameter *p = new MCParameter;
	p->setbuffer(m, strlen(m));
	MCscreen->addmessage((MCPlayer*)player, MCM_node_changed, MCS_time(), p);
	return err;
}

// This is the QTVR hotspot-click callback
static pascal void clickHotSpotCallback(QTVRInstance qtvr, QTVRInterceptPtr qtvrMsg, SInt32 player, Boolean *cancel)
{
	*cancel = False;
	switch (qtvrMsg->selector)
	{
	case kQTVRTriggerHotSpotSelector:
		{
			char *m = new char[U2L];
			
			// MW-2005-04-26: [[Tiger]] Seems to complain about the conversion to uint2... 
			sprintf(m, "%d", (uint4)(qtvrMsg->parameter[0]));
			MCParameter *p = new MCParameter;
			p->setbuffer(m, strlen(m));
			MCscreen->addmessage((MCPlayer*)player, MCM_hot_spot_clicked, MCS_time(), p);
		}
		break;
	}
}

void MCPlayer::reloadcallbacks(Boolean reloadstopmovie, long p_from_time)
{
	uint2 i = 0;
	if (!reloadstopmovie)
	while (i < ncallbacks)
	{
		if (this == callbacktable[i].playerObj)
		{
			if (p_from_time == 0 || (p_from_time < 0 && callbacktable[i] . calledAtTime < p_from_time) || (p_from_time > 0 && callbacktable[i] . calledAtTime > p_from_time))
			{
				CallMeWhen(callbacktable[i].cb, userMovieCallbacks, (long)i,
								   triggerTimeEither, callbacktable[i].calledAtTime,
								   callbacktable[i].timeScale); //pass "i" as index to table
			}

			callbacktable[i].wascalled = False;
		}
		i++;
	}
	if (reloadstopmovie)
		CallMeWhen((QTCallBack)stopMovieCB, MovieEndCallback, (long)this, triggerAtStop , 0,
				   GetMovieTimeScale((Movie)theMovie));
}

void MCPlayer::deleteUserCallbacks(void)
{//delete all the callbacks for THIS PLAYER
	uint2 i = 0;
	while (i < ncallbacks)
	{
		if (this == callbacktable[i].playerObj)
		{
			DisposeCallBack(callbacktable[i].cb);
			MCNameDelete(callbacktable[i].msg); //delete msg string
			delete callbacktable[i].param; //delete param string
			callbacktable[i].playerObj = NULL;
			callbacktable[i].wascalled = False;
		}
		else
			i++;
	}
}

Boolean MCPlayer::installUserCallbacks(void)
{
	// parse the user callback string and install callback funcs
	// if movie is prepared,
	if (userCallbackStr == NULL)
		return True;
	char *cblist = strclone(userCallbackStr);
	char *str;
	str = cblist;
	while (*str)
	{
		char *ptr, *data1, *data2;
		if ((data1 = strchr(str, ',')) == NULL)
		{//search ',' as separator
			delete cblist;
			return False; //wrong formatf
		}
		*data1 = '\0';
		data1 ++;
		if ((data2 = strchr(data1, '\n')) != NULL)// more than one callback
			*data2++ = '\0';
		else
			data2 = data1 + strlen(data1);

		if (state & CS_PREPARED)
		{
			uint2 i;
			for (i = 0; i < ncallbacks; i++) //find a hole to insert a new callback
				if (callbacktable[i].playerObj == NULL)
					break;

			if (i == ncallbacks)
			{ //no hole for a new callback, create a new entry
				MCU_realloc((char **)&callbacktable, ncallbacks,
				            ncallbacks + 1, sizeof(UserCallbackStruct));
				ncallbacks++;
			}
			callbacktable[i].calledAtTime = strtol(str, NULL, 10);

			while (isspace(*data1))//strip off preceding and trailing blanks
				data1++;
			ptr = data1;
			callbacktable[i].param = NULL;
			while (*ptr)
			{
				if (isspace(*ptr))
				{
					*ptr++ = '\0';
					callbacktable[i].param = strclone(ptr);
					break;
				}
				ptr++;
			}
			if (callbacktable[i].param == NULL)
				callbacktable[i].param = strclone(str);
			/* UNCHECKED */ MCNameCreateWithCString(data1, callbacktable[i].msg);
			callbacktable[i].playerObj = this;
			callbacktable[i].wascalled = False;
			if ((callbacktable[i].cb = NewCallBack(GetMovieTimeBase((Movie)theMovie),
			                                       callBackAtTime)) == NULL)
			{
				delete cblist;
				return False;
			}
			callbacktable[i].timeScale = GetMovieTimeScale((Movie)theMovie);
		}
		str = data2;
	} //End bulding the table and verifing the callback string
	delete cblist;
	return True;
}
#endif

//
// QuickTime Specific Implementation
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// QuickTime Visual Effects Implementation
//

#ifdef FEATURE_QUICKTIME
QTEffect *MCPlayer::qteffects = NULL;
uint2 MCPlayer::neffects = 0;
#endif

Boolean MCPlayer::stdeffectdlg(MCExecPoint &ep, const char *p_title, Boolean sheet)
{
#ifdef FEATURE_QUICKTIME
	ep.clear();
	if (qtstate != QT_INITTED)
		initqt();
	if (qtstate != QT_INITTED)
		return False;
	QTAtomContainer effectlist = NULL;
	queryeffects((void **)&effectlist);
	if (effectlist == NULL)
	{
		MCresult->sets("can't get effect list");
		return False;
	}
	OSErr result;
	QTAtomContainer effectdesc = NULL;
	QTParameterDialog createdDialogID;
	if (QTNewAtomContainer(&effectdesc) != noErr)
		return False;
	result = QTCreateStandardParameterDialog(effectlist, effectdesc,
	         0, &createdDialogID);
	while (result == noErr)
	{
		EventRecord theEvent;
		WaitNextEvent(everyEvent, &theEvent, 0, nil);
		result = QTIsStandardParameterDialogEvent(&theEvent, createdDialogID);
		switch (result)
		{
		case featureUnsupported:

			{
				result = noErr;
				switch (theEvent.what)
				{
				case updateEvt:
					BeginUpdate((WindowPtr)theEvent.message);
					EndUpdate((WindowPtr)theEvent.message);
					break;
				}
				break;
			}
		case codecParameterDialogConfirm:
		case userCanceledErr:
			QTDismissStandardParameterDialog(createdDialogID);
			createdDialogID =nil;
			break;
		}
	}
	if (result == userCanceledErr)
	{
		MCresult->sets(MCcancelstring);
		QTDisposeAtomContainer(effectlist);
		return False;
	}
	HLock((Handle)effectdesc);
	uint4 datasize = GetHandleSize(effectdesc) + sizeof(long) * 2;
	char *dataptr = ep.getbuffer(datasize);
	long *aLong = (long *)dataptr;
	HLock((Handle)effectdesc);
	aLong[0] = EndianU32_NtoB(datasize);
	aLong[1] = EndianU32_NtoB('qtfx');
	memcpy((char *)(dataptr + (sizeof(long) * 2)),
	       *effectdesc ,GetHandleSize(effectdesc));
	HUnlock((Handle)effectdesc);
	ep.setlength(datasize);
	MCU_base64encode(ep);
	QTDisposeAtomContainer(effectdesc);
	QTDisposeAtomContainer(effectlist);
	return True;
#endif

	return True;
}


#ifdef FEATURE_QUICKTIME
static int compare_qteffect(const void *a, const void *b)
{
	const QTEffect *qa;
	const QTEffect *qb;
	qa = (QTEffect *)a;
	qb = (QTEffect *)b;
	return strcmp(qa -> token, qb -> token);
}
#endif

void MCPlayer::geteffectlist(MCExecPoint &ep)
{
	ep.clear();

#ifdef FEATURE_QUICKTIME
	if (qtstate != QT_INITTED)
		initqt();
	if (qtstate != QT_INITTED)
		return;

	queryeffects(NULL);

	// MW-2008-01-08: [[ Bug 5700 ]] Make sure the effect list is sorted alphabetically
	qsort(qteffects, neffects, sizeof(QTEffect), compare_qteffect);

	uint2 i;
	for (i = 0; i < neffects; i++)
		ep.concatcstring(qteffects[i].token, EC_RETURN, i == 0);
#endif
}

#ifdef FEATURE_QUICKTIME
void MCPlayer::queryeffects(void **effectatomptr)
{
	if (qteffects != NULL && effectatomptr == NULL)
		return;
	QTAtomContainer effectatom = NULL;
	uint2 numeffects;
	// get a list of the available effects
	if  (QTNewAtomContainer(&effectatom) != noErr
	        || QTGetEffectsList(&effectatom, 2, -1, 0L) != noErr)
	{
		if (effectatom != NULL)
			QTDisposeAtomContainer(effectatom);
		return;
	}
	if (effectatomptr != NULL)
		*effectatomptr = effectatom;
	if (qteffects != NULL)
		return;
	
#if defined(_MACOSX) && defined(__LITTLE_ENDIAN__)
	// MW-2007-12-17: [[ Bug 3851 ]] For some reason the dissolve effect doesn't appear in the
	//   effects dialog on Mac Intel. So we just add it oursleves!

	OSType t_type;
	t_type = kCrossFadeTransitionType;
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectTypeAtom, 0, 0, sizeof(OSType), &t_type, NULL);
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectNameAtom, 0, 0, 10, (void *)"Cross Fade", NULL);
	t_type = kAppleManufacturer;
	QTInsertChild(effectatom, kParentAtomIsContainer, kEffectManufacturerAtom, 0, 0, sizeof(OSType), &t_type, NULL);
#endif
		
	// the returned effects list contains (at least) two atoms for each available effect component,
	// a name atom and a type atom; happily, this list is already sorted alphabetically by effect name
	numeffects = QTCountChildrenOfType(effectatom, kParentAtomIsContainer,
	                                   kEffectNameAtom);
	neffects = 0;
	qteffects = new QTEffect[numeffects];
	uint2 i;
	for (i = 1; i <= numeffects; i++)
	{
		QTAtom				nameatom = 0L;
		QTAtom				typeatom = 0L;
		nameatom = QTFindChildByIndex(effectatom, kParentAtomIsContainer,
		                              kEffectNameAtom, i, NULL);
		typeatom = QTFindChildByIndex(effectatom, kParentAtomIsContainer,
		                              kEffectTypeAtom, i, NULL);

		if (nameatom != 0L && typeatom != 0L)
		{
			long datasize;
			char *sptr;
			QTCopyAtomDataToPtr(effectatom, typeatom, false, sizeof(OSType),
			                    &qteffects[neffects].type, NULL);
			QTLockContainer(effectatom);
			QTGetAtomDataPtr(effectatom, nameatom, &datasize, (Ptr *)&sptr);
			qteffects[neffects].token = new char[datasize+1];
			memcpy(qteffects[neffects].token,sptr,datasize);
			qteffects[neffects].token[datasize] = '\0';
			QTUnlockContainer(effectatom);
			neffects++;
		}
	}

	if (effectatomptr == NULL)
		QTDisposeAtomContainer(effectatom);
}

//
// QuickTime Visual Effects Implementation
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sound Recording Implementation
//

// Related class variables
void *MCPlayer::sgSoundComp = NULL;
long MCPlayer::sgSndDriver = 0;
const char *MCPlayer::recordtempfile = NULL;
char *MCPlayer::recordexportfile = NULL;

// Utility functions
static SampleDescriptionHandle scanSoundTracks(Movie tmovie)
{
	short trackCount, index;
	SampleDescriptionHandle aDesc = NULL;
	trackCount = (short)GetMovieTrackCount(tmovie);
	for (index = 1 ; index <= trackCount ; index++)
	{
		OSType aTrackType;
		Track aTrack = NULL;
		Media aMedia = NULL;
		aTrack = GetMovieIndTrack(tmovie, index);
		aMedia = GetTrackMedia(aTrack);
		GetMediaHandlerDescription(aMedia, &aTrackType, 0, 0);
		if (aTrackType == SoundMediaType)
		{
			aDesc = (SampleDescriptionHandle)NewHandle(sizeof(SoundDescription));
			GetMediaSampleDescription(aMedia, 1, aDesc);
			if (GetMoviesError() != noErr)
			{
				DisposeHandle((Handle)aDesc);
				aDesc = NULL;
				continue;
			}
		}
	}
	return aDesc;
}

static bool path_to_dataref(const char *p_path, DataReferenceRecord& r_rec)
{
	bool t_success = true;
	CFStringRef t_cf_path = NULL;
	t_cf_path = CFStringCreateWithCString(NULL, p_path, kCFStringEncodingWindowsLatin1);
	t_success = (t_cf_path != NULL);
	if (t_success)
	{
		OSErr t_error;
		t_error = QTNewDataReferenceFromFullPathCFString(t_cf_path, kQTNativeDefaultPathStyle, 0, &r_rec . dataRef, &r_rec . dataRefType);
		t_success = noErr == t_error;
	}
	CFRelease(t_cf_path);
	return t_success;
}

static void exportToSoundFile(const char *sourcefile, const char *destfile)
{
	bool t_success = true;
	SoundDescriptionHandle myDesc = NULL;
	ComponentResult result = 0;
	Movie tmovie = nil;

	char *t_src_resolved = NULL;
	char *t_dst_resolved = NULL;
	t_src_resolved = MCS_resolvepath(sourcefile);
	t_dst_resolved = MCS_resolvepath(destfile);
	t_success = (t_src_resolved != NULL && t_dst_resolved != NULL);

	DataReferenceRecord t_src_rec, t_dst_rec;
	t_src_rec.dataRef = NULL;
	t_dst_rec.dataRef = NULL;
	
	if (t_success)
	{
		t_success = path_to_dataref(t_src_resolved, t_src_rec) &&
			path_to_dataref(t_dst_resolved, t_dst_rec);
	}

	free(t_src_resolved);
	free(t_dst_resolved);

	Boolean isActive = true;
	QTVisualContextRef aVisualContext = NULL;
	QTNewMoviePropertyElement aMovieProperties[] = {
		{kQTPropertyClass_DataLocation, kQTDataLocationPropertyID_DataReference, sizeof(t_src_rec), &t_src_rec, 0},
		{kQTPropertyClass_NewMovieProperty, kQTNewMoviePropertyID_Active, sizeof(isActive), &isActive, 0},
		{kQTPropertyClass_Context, kQTContextPropertyID_VisualContext, sizeof(aVisualContext), &aVisualContext, 0},
		};

	if (t_success)
		t_success = noErr == NewMovieFromProperties(3, aMovieProperties, 0, NULL, &tmovie);

	if (t_success)
		myDesc = (SoundDescriptionHandle)scanSoundTracks(tmovie);

	if (myDesc)
	{
		//open movie export component
		MovieExportComponent exporter;
		Component c;
		ComponentDescription cd;
		cd.componentType = MovieExportType;
		switch (MCrecordformat)
		{
		case EX_WAVE:
			cd.componentSubType = kQTFileTypeWave;
			break;
		case EX_ULAW:
			cd.componentSubType = kQTFileTypeMuLaw;
			break;
		case EX_AIFF:
			cd.componentSubType = kQTFileTypeAIFF;
			break;
		default:
			cd.componentSubType = kQTFileTypeMovie;
			break;
		}
		cd.componentManufacturer = SoundMediaType;
		cd.componentFlags = canMovieExportFiles;
		cd.componentFlagsMask = canMovieExportFiles;
		c = FindNextComponent(nil, &cd);
		(**myDesc).numChannels = MCrecordchannels;
		(**myDesc).sampleSize = MCrecordsamplesize;
		(**myDesc).sampleRate = (uint32_t)(MCrecordrate * 1000 * 65536);
		exporter = nil;
		exporter = OpenComponent(c);
		result = MovieExportSetSampleDescription(exporter, (SampleDescriptionHandle)myDesc,
		         SoundMediaType);
		errno = ConvertMovieToDataRef(tmovie, 0, t_dst_rec . dataRef, t_dst_rec . dataRefType, cd.componentSubType,
		                           0, 0, exporter);
		// try showUserSettingsDialog | movieToFileOnlyExport | movieFileSpecValid
		DisposeHandle((Handle) myDesc);
		if (exporter)
			CloseComponent(exporter);
		if (errno != noErr)
		{
			char buffer[26 + U4L];
			sprintf(buffer, "error %d exporting recording", errno);
			MCresult->copysvalue(buffer);
		}
	}

	if (t_src_rec.dataRef != NULL)
		DisposeHandle(t_src_rec.dataRef);
	if (t_dst_rec.dataRef != NULL)
		DisposeHandle(t_dst_rec.dataRef);

	DisposeMovie(tmovie);
}

void MCPlayer::handlerecord()
{
	if (MCrecording)
		SGIdle((SeqGrabComponent)sgSoundComp);
}

#endif

void MCPlayer::stoprecording()
{
#ifdef FEATURE_QUICKTIME
	if (MCrecording)
	{
		MCresult->clear();
		MCrecording = False;
		SGStop((SeqGrabComponent)sgSoundComp);
		if (sgSoundComp != NULL)
		{
			CloseComponent((SeqGrabComponent)sgSoundComp);
			sgSoundComp = NULL;
		}
#ifdef _WINDOWS
		if (MCrecordformat == EX_MOVIE)
			CopyFileA(recordtempfile,recordexportfile,False);
		else
#endif
		{
			MCS_unlink(recordexportfile);
			exportToSoundFile(recordtempfile, recordexportfile);
			MCS_unlink(recordtempfile);
		}
		recordexportfile = NULL;
		delete recordexportfile;
	}
#endif
}

void MCPlayer::recordsound(char *fname)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate != QT_INITTED)
		initqt();
	if (qtstate != QT_INITTED)
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	stoprecording();//just in case
	FSSpec fspec;
	recordtempfile = MCS_tmpnam();
	recordexportfile = fname;
	MCS_path2FSSpec(recordtempfile, &fspec);
	OSType compressionType, inputSource;
	memcpy(&compressionType, MCrecordcompression, 4);
	compressionType = EndianU32_NtoB(compressionType);
	if (strequal(MCrecordinput, "dflt"))
		inputSource = 0; // fake "default" entry
	else
	{
		memcpy(&inputSource, MCrecordinput, 4);
		inputSource = EndianU32_NtoB(inputSource);
	}
	// bug in component can't sample at anything except 44.1KHz

	UnsignedFixed sampleRate = 44100 << 16;
#ifdef _WINDOWS

	if (MCrecordformat == EX_MOVIE)
	{
		short denominator = (short)(MAXINT2 / MCrecordrate);
		short numerator = (short)(MCrecordrate * denominator);
		sampleRate = FixRatio(numerator, denominator) * 1000;
	}
#endif

	short sampleSize = MCrecordsamplesize;
	short numChannels = MCrecordchannels;

	int t_flags;
	if ((qtversion >> 24) >= 7)
	{
		// MW-2008-03-15: [[ Bug 6076 ]] Make sure we create the file before we start recording to it
		//   otherwise no recording happens.
		FILE *t_file;
		t_file = fopen(recordtempfile, "w");
		if (t_file != NULL)
			fclose(t_file);

		t_flags = seqGrabDontPreAllocateFileSize | seqGrabAppendToFile;
	}
	else
		t_flags = 0;

	sgSoundComp = OpenDefaultComponent(SeqGrabComponentType, 0);
	errno = SGInitialize((SeqGrabComponent)sgSoundComp);
	if (errno == noErr)
	{
		SGChannel sgSoundChan;
		if ((errno = SGNewChannel((SeqGrabComponent)sgSoundComp, SoundMediaType, &sgSoundChan)) == noErr
		        && (errno = SGSetChannelUsage(sgSoundChan, seqGrabRecord)) == noErr
		        && (errno = SGSetSoundInputRate(sgSoundChan, sampleRate)) == noErr
		        && (errno = SGSetSoundInputParameters(sgSoundChan, sampleSize,
		                                              numChannels, compressionType)) == noErr
		        && (errno = SGSetDataOutput((SeqGrabComponent)sgSoundComp, &fspec, seqGrabToDisk | t_flags)) == noErr
		        && (!inputSource
		            || (errno = SPBSetDeviceInfo(SGGetSoundInputDriver(sgSoundChan),
		                                         siOSTypeInputSource, &inputSource)) == noErr)
		        && (errno = SGSoundInputDriverChanged(sgSoundChan)) == noErr)
		{
			sgSndDriver = SGGetSoundInputDriver(sgSoundChan);
			//turn on sound input metering
			uint2 meterState = 1;
			if ((errno = SPBSetDeviceInfo(sgSndDriver, siLevelMeterOnOff,
			                              (char *)&meterState)) == noErr)

				errno = SGStartRecord((SeqGrabComponent)sgSoundComp);
		}
	}

	if (errno == noErr)
	{
		MCrecording = True;
		MCscreen->addtimer(MCtemplateplayer, MCM_internal, PLAY_RATE);
		MCresult->clear(False);
	}
	else
	{
		char buffer[21 + U4L];
		sprintf(buffer, "error %d starting recording", errno);
		MCresult->copysvalue(buffer);
		if (sgSoundComp != NULL)
		{
			CloseComponent((SeqGrabComponent)sgSoundComp);
			sgSoundComp = NULL;
		}
	}
#else
	MCresult->sets("not supported");
#endif
}

void MCPlayer::getrecordloudness(MCExecPoint &ep)
{
#ifdef FEATURE_QUICKTIME
	uint2 rloudness = 0;
	if (MCrecording)
	{
		uint2 meterState[2];
		SPBGetDeviceInfo(sgSndDriver, siLevelMeterOnOff, (char *)&meterState);
		rloudness = (uint2)((meterState[1] * 100) / 255);
	}
	ep.setint(rloudness);
#else

	MCresult->sets("not supported");
#endif
}

void MCPlayer::getrecordcompressionlist(MCExecPoint &ep)
{
	ep.clear();
#ifdef FEATURE_QUICKTIME
	if (qtstate != QT_INITTED)
		initqt();
	if (qtstate != QT_INITTED)
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	Component component = 0;
	ComponentDescription desc, info;
	Handle name = NewHandle(0);
	desc.componentType = kSoundCompressor;
	desc.componentSubType = 0;
	desc.componentManufacturer = 0;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	ep.concatcstring("No compression,raw ", EC_RETURN, true);
	while ((component = FindNextComponent(component, &desc)) != NULL)
	{
		GetComponentInfo(component, &info, name, 0, 0);
		if (GetHandleSize(name))
		{
			HLock(name);
			ep.concatcstring(p2cstr((unsigned char *)*name), EC_RETURN, false);
			char ssubtype[] = "????";
			long compType;
			compType = EndianU32_BtoN(info.componentSubType);
			memcpy(ssubtype, (char *)&compType, sizeof(OSType));
			ep.concatcstring(ssubtype, EC_COMMA, false);
			HUnlock(name);
		}
	}
	DisposeHandle(name);
#endif
}

// MW-2005-05-15: For consistency, added title field
void MCPlayer::stdrecorddlg(MCExecPoint& ep, const char *p_title, Boolean sheet)
{
#ifdef FEATURE_QUICKTIME
	if (qtstate != QT_INITTED)
		initqt();
	if (qtstate != QT_INITTED)
	{
		MCresult->sets("could not initialize quicktime");
		return;
	}
	ComponentInstance ci = OpenDefaultComponent(StandardCompressionType,
	                       StandardCompressionSubTypeSound);
	if (ci == NULL)
	{
		MCresult->sets("can't open dialog");
		return;
	}
	short denominator = (short)(MAXINT2 / MCrecordrate);
	short numerator = (short)(MCrecordrate * denominator);
	UnsignedFixed sampleRate = FixRatio(numerator, denominator) * 1000;
	short sampleSize = MCrecordsamplesize;
	short numChannels = MCrecordchannels;
	OSType compressionType;
	memcpy(&compressionType, MCrecordcompression, 4);
	compressionType = EndianU32_NtoB(compressionType);
	SCSetInfo(ci, scSoundSampleRateType, &sampleRate);
	SCSetInfo(ci, scSoundSampleSizeType, &sampleSize);
	SCSetInfo(ci, scSoundChannelCountType, &numChannels);
	SCSetInfo(ci, scSoundCompressionType, &compressionType);
	errno = SCRequestImageSettings(ci);
	if (errno == noErr)
	{
		SCGetInfo(ci, scSoundSampleRateType, &sampleRate);
		SCGetInfo(ci, scSoundSampleSizeType, &sampleSize);
		SCGetInfo(ci, scSoundChannelCountType, &numChannels);
		SCGetInfo(ci, scSoundCompressionType, &compressionType);
		MCrecordrate = (HiWord(sampleRate) + LoWord(sampleRate)
		                / (real8)MAXINT2) / 1000.0;
		compressionType = EndianU32_BtoN(compressionType);
		memcpy(MCrecordcompression, &compressionType, 4);
		MCrecordsamplesize = sampleSize;
		MCrecordchannels = numChannels;
	}
	else
		if (errno == userCanceledErr)
		{
			MCresult->sets(MCcancelstring);
			return;
		}
		else
		{
			char buffer[22 + U4L];
			sprintf(buffer, "error %d opening dialog", errno);
			MCresult->copysvalue(buffer);
			return;
		}
	CloseComponent(ci);
#endif
}

//
// Sound Recording Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// X11 (using mplayer) Player Implementation
//
// The MPlayer object (mplayer.cpp) fully encapulates and manages the mplayer process and provides
// a nice, easy to use interface for controlling it.


#ifdef X11
Boolean MCPlayer::x11_prepare(void)
{
	if ( m_player == NULL )
		m_player = new MPlayer();
	
	// OK-2009-01-09: [[Bug 1161]] - File resolving code standardized between image and player.
	// MCPlayer::init appears to duplicate the filename buffer, so freeing it after the call should be ok.
	char *t_filename;
	t_filename = getstack() -> resolve_filename(filename);

	Boolean t_success;
	t_success = (m_player -> init(t_filename, getstack(), rect));

	delete t_filename;
	return t_success;
}

Boolean MCPlayer::x11_playpause(Boolean on)
{
	if ( m_player != NULL)
		m_player -> play(!on) ;
	return True;
}

void MCPlayer::x11_playstepforward(void)
{
	if ( m_player != NULL)
		m_player -> seek() ;
}

void MCPlayer::x11_playstepback(void)
{
	if ( m_player != NULL)
		m_player -> seek(-5) ;
}

Boolean MCPlayer::x11_playstop(void)
{
	if ( m_player != NULL)
		m_player -> pause();
	return True;
}

void MCPlayer::x11_setrect(const MCRectangle& nrect)
{
	rect = nrect;
	if ( m_player != NULL ) 
		m_player -> resize(nrect);
}
	
uint4 MCPlayer::x11_getduration(void)
{
	if ( m_player != NULL)
		return ( m_player -> getduration() ) ;
	else 
		return 0;
}

uint4 MCPlayer::x11_gettimescale(void)
{
	if ( m_player != NULL)
		return ( m_player -> gettimescale() ) ;
	else 
		return 0;
}

uint4 MCPlayer::x11_getmoviecurtime(void)
{
	if ( m_player != NULL)
		return ( m_player -> getcurrenttime() ) ;
	else 
		return 0;
}

void MCPlayer::x11_setlooping(Boolean loop)
{
	if ( m_player != NULL)
		m_player -> setlooping ( loop ) ;
}

void MCPlayer::x11_setplayrate(void)
{
	if ( m_player != NULL)
		m_player -> setspeed( rate ) ;
}

Boolean MCPlayer::x11_ispaused(void)
{
	if ( m_player != NULL)
		return ( m_player -> ispaused() ) ; 
	else 
		return false ;
}

uint2 MCPlayer::x11_getloudness(void)
{
	if ( m_player != NULL)
		return ( m_player -> getloudness () ) ;
	else 
		return 100; // Return 100% as the default
}
	
void MCPlayer::x11_setloudness(uint2 loudn)
{
	if ( m_player != NULL)
		m_player -> setloudness ( loudn );
}

pid_t MCPlayer::getpid(void)
{
	if ( m_player != NULL )
		return m_player -> getpid();
	return 0;
}

void MCPlayer::shutdown(void)
{
	if ( m_player != NULL) m_player -> shutdown(); 
}

#endif
//
// X11 (using mplayer) Player Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// QTEffect implementation

#ifdef FEATURE_QUICKTIME

void MCQTEffectEnd(void);

static CGrafPtr s_qt_target_port = nil;

static MCGImageRef s_qt_start_image = nil;
static CGrafPtr s_qt_start_port = NULL;

static MCGImageRef s_qt_end_image = nil;
static CGrafPtr s_qt_end_port = NULL;

static QTAtomContainer s_qt_effect_desc = NULL;

static ImageDescriptionHandle s_qt_sample_desc = NULL;
static ImageDescriptionHandle s_qt_start_desc = NULL;
static ImageDescriptionHandle s_qt_end_desc = NULL;
static TimeBase s_qt_timebase = NULL;
static ImageSequence s_qt_effect_seq = NULL;

static Boolean s_qt_reverse = False;

void QTEffectAddParameters(QTAtomContainer effectdescription,OSType theEffectType, Visual_effects dir,Boolean &reverse)
{
	OSType paramtype;
	reverse = False;
	long param = 0;
	switch (theEffectType)
	{
	case 'push'://push [right] [left] [top] [bottom]
		{
			switch (dir)
			{
			case VE_BOTTOM:
				param = 1;
				break;
			case VE_LEFT:
				param = 2;
				break;
			case VE_UP:
				param = 3;
				break;
			case VE_RIGHT:
				param = 4;
			default:
				break;
			}
			paramtype = 'from';
		}
		break;
	case 'smpt'://wipe [right] [left] [top] [bottom]
		{
			switch (dir)
			{
			case VE_LEFT:
				reverse = True;
			case VE_RIGHT:
				param = 1;
				break;
			case VE_UP:
				reverse = True;
			case VE_DOWN:
				param = 2;
			default:
				break;
			}
		}
	case 'smp2'://iris [open] [close]
		{
			switch (dir)
			{
			case VE_CLOSE:
				reverse = True;
			case VE_OPEN:
				param = 101;
			default:
				break;
			}
		}
	case 'smp3':
	case 'smp4':
		paramtype = 'wpID';
		break;
	default:
		paramtype = 0;
	}
	
	if (paramtype != 0 && param != 0)
	{
		param = EndianU32_NtoB(param);
		QTInsertChild(effectdescription, kParentAtomIsContainer, paramtype, 1, 0, sizeof(param), &param, NULL);
	}
	if (reverse)
	{
		QTAtom source1 = QTFindChildByIndex(effectdescription, kParentAtomIsContainer, kEffectSourceName, 1, NULL );
		QTAtom source2 = QTFindChildByIndex(effectdescription, kParentAtomIsContainer, kEffectSourceName, 2, NULL );
		if (source2)
			QTSwapAtoms(effectdescription,source1,source2);
	}
}

bool MCQTEffectBegin(Visual_effects p_type, const char *p_name, Visual_effects p_direction, MCGImageRef p_start, MCGImageRef p_end, const MCRectangle& p_area)
{
	if (MCdontuseQTeffects || !MCtemplateplayer -> isQTinitted())
		return false;

	OSType qteffect;
	qteffect = 0;
	
	switch (p_type)
	{
		case VE_DISSOLVE:
			qteffect = 'dslv';
		break;
		
		case VE_IRIS:
			qteffect = 'smp2';
			return false;
		break;
		
		case VE_PUSH:
			qteffect = 'push';
			return false;
		break;
		
		case VE_WIPE:
			qteffect = 'smpt';
			return false;
		break;
		
		case VE_UNDEFINED:
		{
			uint2 i;
			QTEffect *teffects = MCtemplateplayer->geteffects();
			uint2 tsize = MCtemplateplayer->getneffects();
			
			MCString effectname(p_name);
			for (i = 0 ; i < tsize; i++)
			{
				if (effectname == teffects[i].token)
				{
					qteffect = teffects[i].type;
					break;
				}
			}
			if (!qteffect && effectname.getlength() == 4)
			{
				memcpy(&qteffect, p_name, sizeof(OSType));
				qteffect = EndianU32_NtoB(qteffect);
			}
			else
			{
				MCExecPoint ep;
				ep.setsvalue(effectname);
				MCU_base64decode(ep);
				if (ep.getsvalue().getlength() > 8)
				{
					const char *dataptr = ep.getsvalue().getstring();
					long *aLong = (long *)dataptr;
					long datasize = EndianU32_BtoN(aLong[0]) - (sizeof(long)*2);
					OSType ostype = EndianU32_BtoN(aLong[1]);
					if (ostype == 'qtfx')
					{
						s_qt_effect_desc = NewHandle(datasize);
						HLock(s_qt_effect_desc);
						memcpy(*s_qt_effect_desc, (char *)(dataptr + (sizeof(long) * 2)), datasize);
						HUnlock(s_qt_effect_desc);
						QTAtom whatAtom = QTFindChildByID(s_qt_effect_desc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, NULL);
						if (whatAtom)
						{
							QTCopyAtomDataToPtr(s_qt_effect_desc, whatAtom, true, sizeof(qteffect), &qteffect, NULL);
							qteffect = EndianU32_BtoN(qteffect);
						}
					}
				}
			}
		}
		break;
	default:
		break;
	}
	
	Rect t_src_rect, t_dst_rect;
	MacSetRect(&t_src_rect, 0, 0, p_area . width, p_area . height);
	MacSetRect(&t_dst_rect, 0, 0, p_area . width, p_area . height);

	if (qteffect != 0)
	{
		MCGRaster t_start_raster, t_end_raster;
		/* UNCHECKED */ MCGImageGetRaster(p_start, t_start_raster);
		QTNewGWorldFromPtr(&s_qt_start_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0, t_start_raster.pixels, t_start_raster.stride);

		/* UNCHECKED */ MCGImageGetRaster(p_end, t_end_raster);
		QTNewGWorldFromPtr(&s_qt_end_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0, t_end_raster.pixels, t_end_raster.stride);
		
		QTNewGWorld(&s_qt_target_port, PIXEL_FORMAT_32, &t_src_rect, nil, nil, 0);
	}

	if (s_qt_target_port != nil && s_qt_start_port != NULL && s_qt_end_port != NULL)
	{
		OSType effecttype;
		if (s_qt_effect_desc == NULL)
		{
			QTNewAtomContainer(&s_qt_effect_desc);
			effecttype = EndianU32_NtoB(qteffect);
			QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kParameterWhatName, kParameterWhatID, 0, sizeof(effecttype), &effecttype, NULL);
		}
		
		effecttype = EndianU32_NtoB('srcA'); //source 1
		QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kEffectSourceName, 1, 0, sizeof(effecttype), &effecttype, NULL);
		
		effecttype = EndianU32_NtoB('srcB'); //source 2
		QTInsertChild(s_qt_effect_desc, kParentAtomIsContainer, kEffectSourceName, 2, 0, sizeof(effecttype), &effecttype, NULL);
	}
	
	if (s_qt_effect_desc != NULL)
		MakeImageDescriptionForEffect(qteffect, &s_qt_sample_desc);
		
	if (s_qt_sample_desc != NULL)
	{				
		(**s_qt_sample_desc).vendor = kAppleManufacturer;
		(**s_qt_sample_desc).temporalQuality = codecNormalQuality;
		(**s_qt_sample_desc).spatialQuality = codecNormalQuality;
		(**s_qt_sample_desc).width = p_area . width;
		(**s_qt_sample_desc).height = p_area . height;
		QTEffectAddParameters(s_qt_effect_desc, qteffect, p_direction,	s_qt_reverse);
		
		MatrixRecord t_matrix;
		RectMatrix(&t_matrix, &t_src_rect, &t_dst_rect);
		
		HLock((Handle)s_qt_effect_desc);
		DecompressSequenceBeginS(&s_qt_effect_seq, s_qt_sample_desc,
														 *s_qt_effect_desc, GetHandleSize(s_qt_effect_desc),
														 s_qt_target_port, nil,
														 nil, &t_matrix, ditherCopy, nil,
														 0, codecNormalQuality, nil);
		HUnlock((Handle)s_qt_effect_desc);
	}
	
	if (s_qt_effect_seq != 0)
	{
		ImageSequenceDataSource t_src_sequence;
		
		t_src_sequence = 0;
		PixMapHandle t_src_pixmap = GetGWorldPixMap(s_qt_start_port);
		MakeImageDescriptionForPixMap(t_src_pixmap, &s_qt_start_desc);
		CDSequenceNewDataSource(s_qt_effect_seq, &t_src_sequence, 'srcA', 1, (Handle)s_qt_start_desc, nil, 0);
		CDSequenceSetSourceData(t_src_sequence, GetPixBaseAddr(t_src_pixmap), (**s_qt_start_desc) . dataSize);
	}
	
	if (s_qt_start_desc != NULL)
	{
		ImageSequenceDataSource t_src_sequence;
		
		t_src_sequence = 0;
		PixMapHandle t_end_pixmap = GetGWorldPixMap(s_qt_end_port);
		MakeImageDescriptionForPixMap(t_end_pixmap, &s_qt_end_desc);
		CDSequenceNewDataSource(s_qt_effect_seq, &t_src_sequence, 'srcB', 1, (Handle)s_qt_end_desc, nil, 0);
		CDSequenceSetSourceData(t_src_sequence, GetPixBaseAddr(t_end_pixmap), (**s_qt_end_desc) . dataSize);
	}
	
	if (s_qt_end_desc != NULL)
	{
		s_qt_timebase = NewTimeBase();
		SetTimeBaseRate(s_qt_timebase, 0);
		CDSequenceSetTimeBase(s_qt_effect_seq, s_qt_timebase);
	}
	
	if (s_qt_timebase == NULL)
	{
		MCQTEffectEnd();
		return false;
	}
	
	return true;
}

bool MCQTEffectStep(const MCRectangle &drect, MCStackSurface *p_target, uint4 p_delta, uint4 p_duration)
{
	ICMFrameTimeRecord t_frame_time;
	memset((char *)&t_frame_time, 0, sizeof(ICMFrameTimeRecord));
	SetTimeBaseValue(s_qt_timebase, p_delta, p_duration);
	
	if (s_qt_reverse)
		p_delta = p_duration - p_delta;
	else if (p_delta == 0)
		p_delta = 1;
	
	t_frame_time . recordSize = sizeof(ICMFrameTimeRecord);
	t_frame_time . flags = icmFrameTimeHasVirtualStartTimeAndDuration;
	t_frame_time . frameNumber = 1;
	t_frame_time . value . lo = p_delta;
	t_frame_time . scale = t_frame_time . duration = t_frame_time . virtualDuration = p_duration;
	HLock((Handle)s_qt_effect_desc);
	DecompressSequenceFrameWhen(s_qt_effect_seq, *(Handle)s_qt_effect_desc, GetHandleSize((Handle)s_qt_effect_desc), 0, 0, nil, &t_frame_time);
	HUnlock((Handle)s_qt_effect_desc);
	
	PixMapHandle t_pixmap = GetGWorldPixMap(s_qt_target_port);
	LockPixels(t_pixmap);
	void *t_bits = GetPixBaseAddr(t_pixmap);
	uint32_t t_stride = QTGetPixMapHandleRowBytes(t_pixmap);

	MCGRaster t_raster;
	t_raster.width = drect.width;
	t_raster.height = drect.height;
	t_raster.pixels = t_bits;
	t_raster.stride = t_stride;
	t_raster.format = kMCGRasterFormat_xRGB;
	
	MCGImageRef t_image = nil;
	/* UNCHECKED */ MCGImageCreateWithRasterNoCopy(t_raster, t_image);
	
	MCGRectangle t_src_rect, t_dst_rect;
	t_src_rect = MCGRectangleMake(0, 0, drect.width, drect.height);
	t_dst_rect = MCGRectangleTranslate(t_src_rect, drect.x, drect.y);
	
	p_target->Composite(t_dst_rect, t_image, t_src_rect, 1.0, kMCGBlendModeCopy);
	MCGImageRelease(t_image);
	
	UnlockPixels(t_pixmap);
	
	return true;
}

void MCQTEffectEnd(void)
{
	if (s_qt_effect_seq != 0)
		CDSequenceEnd(s_qt_effect_seq), s_qt_effect_seq = NULL;

	if (s_qt_timebase != NULL)
		DisposeTimeBase(s_qt_timebase), s_qt_timebase = NULL;

	if (s_qt_end_desc != NULL)
		DisposeHandle((Handle)s_qt_end_desc), s_qt_end_desc = NULL;
	
	if (s_qt_start_desc != NULL)
		DisposeHandle((Handle)s_qt_start_desc), s_qt_start_desc = NULL;
		
	if (s_qt_sample_desc != NULL)
		DisposeHandle((Handle)s_qt_sample_desc), s_qt_sample_desc = NULL;
	
	if (s_qt_target_port != NULL)
		DisposeGWorld(s_qt_target_port), s_qt_target_port = NULL;
	
	if (s_qt_end_port != NULL)
		DisposeGWorld(s_qt_end_port), s_qt_end_port = NULL;
	
	if (s_qt_start_port != NULL)
		DisposeGWorld(s_qt_start_port), s_qt_start_port = NULL;
	
	if (s_qt_effect_desc != NULL)
	{
		QTDisposeAtomContainer(s_qt_effect_desc);
		s_qt_effect_desc = NULL;
	}
}

#endif

// 
//-----------------------------------------------------------------------------
