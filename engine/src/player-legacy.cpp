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


// SN-2014-07-03: [[ PlatformPlayer ]]
// We do not want to compile this in case the PlatformPlayer
// is the one targeted
#ifndef FEATURE_PLATFORM_PLAYER

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "util.h"
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "stacklst.h"
#include "card.h"
#include "field.h"
#include "aclip.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "mode.h"
#include "context.h"
#include "osspec.h"
#include "redraw.h"

#include "player-legacy.h"

#include "graphics_util.h"
#include "exec-interface.h"

#if defined(_LINUX_DESKTOP)
#include "lnxmplayer.h"
#endif

//// X11

#undef X11
#ifdef TARGET_PLATFORM_LINUX
#define X11
#endif

//////////

extern bool MCFiltersBase64Encode(MCDataRef p_src, MCStringRef& r_dst);
extern void copy_custom_list_as_string_and_release(MCExecContext& ctxt, MCExecCustomTypeInfo *p_type, void *p_elements, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string);

//-----------------------------------------------------------------------------
// Control Implementation
//

#define XANIM_WAIT 10.0
#define XANIM_COMMAND 1024

MCPlayer::MCPlayer()
{	
	flags |= F_TRAVERSAL_ON;
	nextplayer = nil;
	rect.width = rect.height = 128;
	filename = MCValueRetain(kMCEmptyString);
	istmpfile = False;
	scale = 1.0;
	rate = 1.0;
	lasttime = 0;
	starttime = endtime = MAXUINT4;
	disposable = istmpfile = False;
	userCallbackStr = MCValueRetain(kMCEmptyString);
	formattedwidth = formattedheight = 0;
	loudness = 100;
	dontuseqt = False;
	usingqt = False;
	
#ifdef FEATURE_MPLAYER
	command = NULL;
	m_player = NULL ;
	atom = GDK_NONE;
#endif
    
}

MCPlayer::MCPlayer(const MCPlayer &sref) : MCControl(sref)
{
	nextplayer = nil;
	filename = MCValueRetain(sref.filename);
	istmpfile = False;
	scale = 1.0;
	rate = sref.rate;
	lasttime = sref.lasttime;
	starttime = sref.starttime;
	endtime = sref.endtime;
	disposable = istmpfile = False;
	userCallbackStr = MCValueRetain(sref.userCallbackStr);
	formattedwidth = formattedheight = 0;
	loudness = sref.loudness;
	dontuseqt = False;
	usingqt = False;
	
#ifdef FEATURE_MPLAYER
	command = NULL;
	m_player = NULL ;
	atom = GDK_NONE;
#endif
    
}

MCPlayer::~MCPlayer()
{
    removefromplayers();
    
#ifdef FEATURE_MPLAYER
	if ( m_player != NULL )
		delete m_player ;
#endif
    
	MCValueRelease(filename);
	MCValueRelease(userCallbackStr);
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
		prepare(kMCEmptyString);
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

Boolean MCPlayer::kdown(MCStringRef p_string, KeySym key)
{
	if (!(state & CS_NO_MESSAGES))
		if (MCObject::kdown(p_string, key))
			return True;
    
	return False;
}

Boolean MCPlayer::kup(MCStringRef p_string, KeySym key)
{
	return False;
}

Boolean MCPlayer::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || showinvisible())
        || (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return False;
    
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
                if (message_with_valueref_args(MCM_mouse_down, MCSTR("1")) == ES_NORMAL)
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
            if (message_with_valueref_args(MCM_mouse_down, MCSTR("2")) == ES_NORMAL)
                return True;
            break;
		case Button3:
            message_with_valueref_args(MCM_mouse_down, MCSTR("3"));
            break;
	}
	return True;
}

Boolean MCPlayer::mup(uint2 which, bool p_release) //mouse up
{
	if (!(state & CS_MFOCUSED))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
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
                if (!p_release && MCU_point_in_rect(rect, mx, my))
                    message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
                else
                    message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
                
                break;
            case T_PLAYER:
            case T_POINTER:
                end(true, p_release);       //stop dragging or moving the movie object, will change controller size
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
            if (!p_release && MCU_point_in_rect(rect, mx, my))
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

void MCPlayer::applyrect(const MCRectangle &nrect)
{
#if defined(X11)
	x11_setrect(nrect);
#endif
}

void MCPlayer::timer(MCNameRef mptr, MCParameter *params)
{
    if (MCNameIsEqualToCaseless(mptr, MCM_play_stopped))
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
    else if (MCNameIsEqualToCaseless(mptr, MCM_play_paused))
    {
        state |= CS_PAUSED;
        if (isbuffering()) //so the last frame gets to be drawn
        {
            // MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
            layer_redrawall();
        }
    }
    
	MCControl::timer(mptr, params);
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
	MCPlayer *newplayer = new (nothrow) MCPlayer(*this);
	if (attach)
		newplayer->attach(p, invisible);
	return newplayer;
}

IO_stat MCPlayer::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCPlayer::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_remaining)
{
	return defaultextendedload(p_stream, p_version, p_remaining);
}

IO_stat MCPlayer::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	if (!disposable)
	{
		if ((stat = IO_write_uint1(OT_PLAYER, stream)) != IO_NORMAL)
			return stat;
		if ((stat = MCControl::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
			return stat;
		
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_stringref_new(filename, stream, p_version >= 7000)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(starttime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint4(endtime, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int4((int4)(rate / 10.0 * MAXINT4),
		                          stream)) != IO_NORMAL)
			return stat;
		
		// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
        if ((stat = IO_write_stringref_new(userCallbackStr, stream, p_version >= 7000)) != IO_NORMAL)
			return stat;
	}
	return savepropsets(stream, p_version);
}

IO_stat MCPlayer::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;
    
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_read_stringref_new(filename, stream, version >= 7000)) != IO_NORMAL)
		return stat;
	uint32_t t_starttime, t_endtime;
	if ((stat = IO_read_uint4(&t_starttime, stream)) != IO_NORMAL)
		return stat;
	starttime = t_starttime;
	if ((stat = IO_read_uint4(&t_endtime, stream)) != IO_NORMAL)
		return stat;
	endtime = t_endtime;
	int4 trate;
	if ((stat = IO_read_int4(&trate, stream)) != IO_NORMAL)
		return stat;
	rate = (real8)trate * 10.0 / MAXINT4;
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_read_stringref_new(userCallbackStr, stream, version >= 7000)) != IO_NORMAL)
		return stat;
	return loadpropsets(stream, version);
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
}

// MW-2007-08-14: [[ Bug 1949 ]] On Windows ensure we load and unload QT if not
//   currently in use.
bool MCPlayer::getversion(MCStringRef& r_string)
{
#if defined(X11)
	return MCStringCreateWithNativeChars((const char_t*)"2.0", 3, r_string);
#else
	r_string = MCValueRetain(kMCEmptyString);
    return true;
#endif
}

void MCPlayer::freetmp()
{
	if (istmpfile)
	{
		MCS_unlink(filename);
		MCValueAssign(filename, kMCEmptyString);
	}
}

MCPlayerDuration MCPlayer::getduration() //get movie duration/length
{
#if defined(X11)
	return x11_getduration();
#else
	return 0;
#endif
}

MCPlayerDuration MCPlayer::gettimescale() //get moive time scale
{
#if defined(X11) //X11 stuff
	return x11_gettimescale();
#else
	return 0;
#endif
}

MCPlayerDuration MCPlayer::getmoviecurtime()
{
#if defined(X11)
	return x11_getmoviecurtime();
#else
	return 0;
#endif
}

void MCPlayer::setcurtime(MCPlayerDuration newtime, bool notify)
{
	lasttime = newtime;
#if defined(X11)
	x11_setcurtime(newtime);
#endif
}

void MCPlayer::setselection(bool notify)
{
#if defined(X11)
	x11_setselection();
#endif
}

void MCPlayer::setlooping(Boolean loop)
{
#if defined(X11)
	x11_setlooping(loop);
#endif
}

void MCPlayer::setplayrate()
{
#if defined(X11)
	x11_setplayrate();
#endif
    
	if (rate != 0)
		state = state & ~CS_PAUSED;
	else
		state = state | CS_PAUSED;
}

void MCPlayer::showbadge(Boolean show)
{
#if defined(X11)
	x11_showbadge(show);
#endif
}

void MCPlayer::editmovie(Boolean edit)
{
#if defined(X11)
	x11_editmovie(edit);
#endif
}

void MCPlayer::playselection(Boolean play)
{
#if defined(X11)
	x11_playselection(play);
#endif
}

Boolean MCPlayer::ispaused()
{
#if defined(X11)
	return x11_ispaused();
#else
	return True;
#endif
}

void MCPlayer::showcontroller(Boolean show)
{
#if defined(X11)
	x11_showcontroller(show);
#endif
}

Boolean MCPlayer::prepare(MCStringRef options)
{
	Boolean ok = False;
    
	if (state & CS_PREPARED || MCStringIsEmpty(filename))
		return True;
	
	if (!opened)
		return False;

#ifdef X11
	ok = x11_prepare();
#endif
    
	if (ok)
	{
		state |= CS_PREPARED | CS_PAUSED;
        
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

Boolean MCPlayer::playstart(MCStringRef options)
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
    
#if defined(X11)
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
    
#if defined(X11)
	x11_playstepforward();
#endif
}

void MCPlayer::playstepback()
{
	if (!getstate(CS_PREPARED))
		return;
	
#if defined(X11)
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
		
#if defined(X11)
	needmessage = x11_playstop();
#endif
    
	freetmp();
    
	if (MCplayers)
	{
		if (MCplayers == this)
			MCplayers = nextplayer;
		else
		{
			MCPlayer *tptr = MCplayers;
			while (tptr->nextplayer && tptr->nextplayer != this)
				tptr = tptr->nextplayer;
			if (tptr->nextplayer == this)
                tptr->nextplayer = nextplayer;
		}
	}
	nextplayer = nil;
    
	if (disposable)
	{
		if (needmessage)
			getcard()->message_with_valueref_args(MCM_play_stopped, getname());
		delete this;
	}
	else
		if (needmessage)
			message_with_valueref_args(MCM_play_stopped, getname());
    
	return True;
}


void MCPlayer::setfilename(MCStringRef vcname,
                           MCStringRef fname, Boolean istmp)
{
	// AL-2014-05-27: [[ Bug 12517 ]] Incoming strings can be nil
    MCNewAutoNameRef t_vcname;
    if (vcname != nil)
        MCNameCreate(vcname, &t_vcname);
    else
        t_vcname = kMCEmptyName;
    
	setname(*t_vcname);
	filename = MCValueRetain(fname != nil ? fname : kMCEmptyString);
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
    
#if defined(X11)
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
#if defined(X11)
    loudness = x11_getloudness();
#else
    loudness = loudness;
#endif
	return loudness;
}

void MCPlayer::setloudness()
{
	if (state & CS_PREPARED)
#if defined(X11)
    x11_setloudness(loudness);
#else
	loudness = loudness;
#endif
}

void MCPlayer::setenabledtracks(uindex_t p_count, uint32_t *p_tracks_id)
{
	if (getstate(CS_PREPARED))
#if defined(X11)
    x11_setenabledtracks(p_count, p_tracks_id);
#else
    0 == 0;
#endif
}

integer_t MCPlayer::getmediatypes()
{
    MCPlayerMediaTypeSet types = 0;
    return (integer_t)types;
}

uinteger_t MCPlayer::getcurrentnode()
{
    uint2 i = 0;
    return i;
}

bool MCPlayer::changecurrentnode(uinteger_t nodeid)
{
    return false;
}

real8 MCPlayer::getpan()
{
    real8 pan = 0.0;
    return pan;
}

bool MCPlayer::changepan(real8 pan)
{
    return isbuffering() == True;
}

real8 MCPlayer::gettilt()
{
    real8 tilt = 0.0;
    return tilt;
}

bool MCPlayer::changetilt(real8 tilt)
{
    return isbuffering() == True;
}

real8 MCPlayer::getzoom()
{
    real8 zoom = 0.0;
    return zoom;
}

bool MCPlayer::changezoom(real8 zoom)
{
    return isbuffering() == True;
}


void MCPlayer::getenabledtracks(uindex_t &r_count, uint32_t *&r_tracks_id)
{
    uindex_t t_count;
    uint32_t *t_tracks_id;
    
    t_count = 0;
    t_tracks_id = nil;
    
    if (getstate(CS_PREPARED))
#if defined(X11)
        x11_getenabledtracks(t_count, t_tracks_id);
#else
    // SN-2015-06-19: [[ CID 100295 ]] Use brackets instead of true assertion
    {}
#endif
    
    r_count = t_count;
    r_tracks_id = t_tracks_id;
}

void MCPlayer::gettracks(MCStringRef &r_tracks)
{
    if (getstate(CS_PREPARED))
#if defined(X11)
        x11_gettracks(r_tracks);
#else
        r_tracks = MCValueRetain(kMCEmptyString);
#endif
    else
        r_tracks = MCValueRetain(kMCEmptyString);
}

void MCPlayer::getconstraints(MCMultimediaQTVRConstraints &r_constraints)
{
}

void MCPlayer::getnodes(MCStringRef &r_nodes)
{
    MCStringRef t_nodes;
    t_nodes = MCValueRetain(kMCEmptyString);
    r_nodes = t_nodes;
}

void MCPlayer::gethotspots(MCStringRef &r_hotspots)
{
    MCStringRef t_spots;
    t_spots = MCValueRetain(kMCEmptyString);
    
    r_hotspots = t_spots;
}

void MCPlayer::updatevisibility()
{
}

void MCPlayer::updatetraversal()
{
}

void MCPlayer::setmoviecontrollerid(integer_t p_id)
{
}

integer_t MCPlayer::getmoviecontrollerid()
{
    return (integer_t)NULL;
}

uinteger_t MCPlayer::gettrackcount()
{
    uint2 i = 0;
    return i;
}

void MCPlayer::setcallbacks(MCStringRef p_callbacks)
{
    MCValueAssign(userCallbackStr, p_callbacks);
}

// End of property setters/getters
////////////////////////////////////////////////////////////////////////

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
    
	setforeground(dc, DI_BACK, False);
	dc->setbackground(MCscreen->getwhite());
	dc->setfillstyle(FillOpaqueStippled, nil, 0, 0);
	dc->fillrect(rect);
	dc->setbackground(MCzerocolor);
	dc->setfillstyle(FillSolid, nil, 0, 0);
    
	if (getflag(F_SHOW_BORDER))
	{
		if (getflag(F_3D))
			draw3d(dc, rect, ETCH_SUNKEN, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	}
	
	if (!p_isolated)
	{
		if (getstate(CS_SELECTED))
			drawselected(dc);
	}
    
	if (!p_isolated)
		dc -> end();
}

    
////////////////////////////////////////////////////////////////////
// QUICKTIME ACCESSORS

void MCPlayer::getqtvrconstraints(uint1 index, real4& minrange, real4& maxrange)
{
}

uint2 MCPlayer::getnodecount()
{
    uint2 numnodes = 0;
    return numnodes;
}

bool MCPlayer::getnode(uindex_t index, uint2 &id, MCMultimediaQTVRNodeType &type)
{
    return false;
}

uint2 MCPlayer::gethotspotcount()
{
    uint2 numspots = 0;
    return numspots;
}

bool MCPlayer::gethotspot(uindex_t index, uint2 &id, MCMultimediaQTVRHotSpotType &type)
{
    return false;
}

/////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// X11 (using mplayer) Player Implementation
//
// The MPlayer object (mplayer.cpp) fully encapulates and manages the mplayer process and provides
// a nice, easy to use interface for controlling it.

#ifdef X11
Boolean MCPlayer::x11_prepare(void)
{
    if ( m_player == NULL )
        m_player = new (nothrow) MPlayer();
    
    // OK-2009-01-09: [[Bug 1161]] - File resolving code standardized between image and player.
    // MCPlayer::init appears to duplicate the filename buffer, so freeing it after the call should be ok.
    MCAutoStringRef t_filename;
    getstack() -> resolve_filename(filename, &t_filename);
    
    Boolean t_success;
    MCAutoStringRefAsCString t_filename_cstring;
    /* UNCHECKED */ t_filename_cstring . Lock(*t_filename);
    t_success = (m_player -> init(*t_filename_cstring, getstack(), rect));
    
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
    
#endif // ifndef FEATURE_PLATFORM_PLAYER
