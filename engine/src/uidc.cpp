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

#include "sysdefs.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "param.h"
#include "globals.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "field.h"
#include "paragraf.h"
#include "mctheme.h"
#include "printer.h"
#include "osspec.h"
#include "redraw.h"
#include "notify.h"
#include "dispatch.h"
#include "notify.h"
#include "mode.h"
#include "eventqueue.h"

#include "graphicscontext.h"

#include "resolution.h"

#include "exec.h"

class MCNullPrinter: public MCPrinter
{
protected:
	void DoInitialize(void) {}
	void DoFinalize(void) {}

	bool DoReset(MCStringRef p_name) {return false;}
	bool DoResetSettings(MCDataRef p_settings) {return false;}

	const char *DoFetchName(void) {return "";}
	void DoFetchSettings(void*& r_buffer, uint4& r_length) {r_buffer = NULL; r_length = 0;}

	void DoResync(void) {}

	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner) {return PRINTER_DIALOG_RESULT_ERROR;}
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner) {return PRINTER_DIALOG_RESULT_ERROR;}

	MCPrinterResult DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device) {return PRINTER_RESULT_ERROR;}
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device) {return PRINTER_RESULT_ERROR;}
};

typedef struct
{
	uint1 red;
	uint1 green;
	uint1 blue;
}
SCCLUT;

static const SCCLUT sccolors[] =
    {
        {255,255,255}, {255,255,204}, {255,255,153}, {255,255,102},
        {255,255,51}, {255,255,0}, {255,204,255}, {255,204,204},
        {255,204,153}, {255,204,102}, {255,204,51}, {255,204,0},
        {255,153,255}, {255,153,204}, {255,153,153}, {255,153,102},
        {255,153,51}, {255,153,0}, {255,102,255}, {255,102,204},
        {255,102,153}, {255,102,102}, {255,102,51}, {255,102,0},
        {255,51,255}, {255,51,204}, {255,51,153}, {255,51,102}, {255,51,51},
        {255,51,0}, {255,0,255}, {255,0,204}, {255,0,153}, {255,0,102},
        {255,0,51}, {255,0,0}, {204,255,255}, {204,255,204}, {204,255,153},
        {204,255,102}, {204,255,51}, {204,255,0}, {204,204,255},
        {204,204,204}, {204,204,153}, {204,204,102}, {204,204,51},
        {204,204,0}, {204,153,255}, {204,153,204}, {204,153,153},
        {204,153,102}, {204,153,51}, {204,153,0}, {204,102,255},
        {204,102,204}, {204,102,153}, {204,102,102}, {204,102,51},
        {204,102,0}, {204,51,255}, {204,51,204}, {204,51,153}, {204,51,102},
        {204,51,51}, {204,51,0}, {204,0,255}, {204,0,204}, {204,0,153},
        {204,0,102}, {204,0,51}, {204,0,0}, {153,255,255}, {153,255,204},
        {153,255,153}, {153,255,102}, {153,255,51}, {153,255,0},
        {153,204,255}, {153,204,204}, {153,204,153}, {153,204,102},
        {153,204,51}, {153,204,0}, {153,153,255}, {153,153,204},
        {153,153,153}, {153,153,102}, {153,153,51}, {153,153,0},
        {153,102,255}, {153,102,204}, {153,102,153}, {153,102,102},
        {153,102,51}, {153,102,0}, {153,51,255}, {153,51,204}, {153,51,153},
        {153,51,102}, {153,51,51}, {153,51,0}, {153,0,255}, {153,0,204},
        {153,0,153}, {153,0,102}, {153,0,51}, {153,0,0}, {102,255,255},
        {102,255,204}, {102,255,153}, {102,255,102}, {102,255,51},
        {102,255,0}, {102,204,255}, {102,204,204}, {102,204,153},
        {102,204,102}, {102,204,51}, {102,204,0}, {102,153,255},
        {102,153,204}, {102,153,153}, {102,153,102}, {102,153,51},
        {102,153,0}, {102,102,255}, {102,102,204}, {102,102,153},
        {102,102,102}, {102,102,51}, {102,102,0}, {102,51,255},
        {102,51,204}, {102,51,153}, {102,51,102}, {102,51,51}, {102,51,0},
        {102,0,255}, {102,0,204}, {102,0,153}, {102,0,102}, {102,0,51},
        {102,0,0}, {51,255,255}, {51,255,204}, {51,255,153}, {51,255,102},
        {51,255,51}, {51,255,0}, {51,204,255}, {51,204,204}, {51,204,153},
        {51,204,102}, {51,204,51}, {51,204,0}, {51,153,255}, {51,153,204},
        {51,153,153}, {51,153,102}, {51,153,51}, {51,153,0}, {51,102,255},
        {51,102,204}, {51,102,153}, {51,102,102}, {51,102,51}, {51,102,0},
        {51,51,255}, {51,51,204}, {51,51,153}, {51,51,102}, {51,51,51},
        {51,51,0}, {51,0,255}, {51,0,204}, {51,0,153}, {51,0,102},
        {51,0,51}, {51,0,0}, {0,255,255}, {0,255,204}, {0,255,153},
        {0,255,102}, {0,255,51}, {0,255,0}, {0,204,255}, {0,204,204},
        {0,204,153}, {0,204,102}, {0,204,51}, {0,204,0}, {0,153,255},
        {0,153,204}, {0,153,153}, {0,153,102}, {0,153,51}, {0,153,0},
        {0,102,255}, {0,102,204}, {0,102,153}, {0,102,102}, {0,102,51},
        {0,102,0}, {0,51,255}, {0,51,204}, {0,51,153}, {0,51,102},
        {0,51,51}, {0,51,0}, {0,0,255}, {0,0,204}, {0,0,153}, {0,0,102},
        {0,0,51}, {238,0,0}, {221,0,0}, {187,0,0}, {170,0,0}, {136,0,0},
        {119,0,0}, {85,0,0}, {68,0,0}, {34,0,0}, {17,0,0}, {0,238,0},
        {0,221,0}, {0,187,0}, {0,170,0}, {0,136,0}, {0,119,0}, {0,85,0},
        {0,68,0}, {0,34,0}, {0,17,0}, {0,0,238}, {0,0,221}, {0,0,187},
        {0,0,170}, {0,0,136}, {0,0,119}, {0,0,85}, {0,0,68}, {0,0,34},
        {0,0,17}, {238,238,238}, {221,221,221}, {187,187,187},
        {170,170,170}, {136,136,136}, {119,119,119}, {85,85,85}, {68,68,68},
        {34,34,34}, {17,17,17}, {0,0,0}
    };

static const uint4 stdcmap[256] =
    {
        0x000000, 0x800000, 0x008000, 0x808000, 0x000080,
        0x800080, 0x008080, 0xC0C0C0, 0xC0DCC0, 0xA6CAF0,
        0x000000, 0x000033, 0x000066, 0x000099, 0x0000CC, 0x0000FF,
        0x003300, 0x003333, 0x003366, 0x003399, 0x0033CC, 0x0033FF,
        0x006600, 0x006633, 0x006666, 0x006699, 0x0066CC, 0x0066FF,
        0x009900, 0x009933, 0x009966, 0x009999, 0x0099CC, 0x0099FF,
        0x00CC00, 0x00CC33, 0x00CC66, 0x00CC99, 0x00CCCC, 0x00CCFF,
        0x00FF00, 0x00FF33, 0x00FF66, 0x00FF99, 0x00FFCC, 0x00FFFF,
        0x330000, 0x330033, 0x330066, 0x330099, 0x3300CC, 0x3300FF,
        0x333300, 0x333333, 0x333366, 0x333399, 0x3333CC, 0x3333FF,
        0x336600, 0x336633, 0x336666, 0x336699, 0x3366CC, 0x3366FF,
        0x339900, 0x339933, 0x339966, 0x339999, 0x3399CC, 0x3399FF,
        0x33CC00, 0x33CC33, 0x33CC66, 0x33CC99, 0x33CCCC, 0x33CCFF,
        0x33FF00, 0x33FF33, 0x33FF66, 0x33FF99, 0x33FFCC, 0x33FFFF,
        0x660000, 0x660033, 0x660066, 0x660099, 0x6600CC, 0x6600FF,
        0x663300, 0x663333, 0x663366, 0x663399, 0x6633CC, 0x6633FF,
        0x666600, 0x666633, 0x666666, 0x666699, 0x6666CC, 0x6666FF,
        0x669900, 0x669933, 0x669966, 0x669999, 0x6699CC, 0x6699FF,
        0x66CC00, 0x66CC33, 0x66CC66, 0x66CC99, 0x66CCCC, 0x66CCFF,
        0x66FF00, 0x66FF33, 0x66FF66, 0x66FF99, 0x66FFCC, 0x66FFFF,
        0x990000, 0x990033, 0x990066, 0x990099, 0x9900CC, 0x9900FF,
        0x993300, 0x993333, 0x993366, 0x993399, 0x9933CC, 0x9933FF,
        0x996600, 0x996633, 0x996666, 0x996699, 0x9966CC, 0x9966FF,
        0x999900, 0x999933, 0x999966, 0x999999, 0x9999CC, 0x9999FF,
        0x99CC00, 0x99CC33, 0x99CC66, 0x99CC99, 0x99CCCC, 0x99CCFF,
        0x99FF00, 0x99FF33, 0x99FF66, 0x99FF99, 0x99FFCC, 0x99FFFF,
        0xCC0000, 0xCC0033, 0xCC0066, 0xCC0099, 0xCC00CC, 0xCC00FF,
        0xCC3300, 0xCC3333, 0xCC3366, 0xCC3399, 0xCC33CC, 0xCC33FF,
        0xCC6600, 0xCC6633, 0xCC6666, 0xCC6699, 0xCC66CC, 0xCC66FF,
        0xCC9900, 0xCC9933, 0xCC9966, 0xCC9999, 0xCC99CC, 0xCC99FF,
        0xCCCC00, 0xCCCC33, 0xCCCC66, 0xCCCC99, 0xCCCCCC, 0xCCCCFF,
        0xCCFF00, 0xCCFF33, 0xCCFF66, 0xCCFF99, 0xCCFFCC, 0xCCFFFF,
        0xFF0000, 0xFF0033, 0xFF0066, 0xFF0099, 0xFF00CC, 0xFF00FF,
        0xFF3300, 0xFF3333, 0xFF3366, 0xFF3399, 0xFF33CC, 0xFF33FF,
        0xFF6600, 0xFF6633, 0xFF6666, 0xFF6699, 0xFF66CC, 0xFF66FF,
        0xFF9900, 0xFF9933, 0xFF9966, 0xFF9999, 0xFF99CC, 0xFF99FF,
        0xFFCC00, 0xFFCC33, 0xFFCC66, 0xFFCC99, 0xFFCCCC, 0xFFCCFF,
        0xFFFF00, 0xFFFF33, 0xFFFF66, 0xFFFF99, 0xFFFFCC, 0xFFFFFF,
        0x101010, 0x1C1C1C, 0x282828, 0x343434, 0x404040,
        0x4C4C4C, 0x585858, 0x646464, 0x707070, 0x7C7C7C,
        0x888888, 0x949494, 0xA0A0A0, 0xACACAC, 0xB8B8B8,
        0xC4C4C4, 0xD0D0D0, 0xDCDCDC, 0xE8E8E8, 0xF4F4F4,
        0xFFF6F0, 0xA0A0A4, 0x808080, 0xFF0000, 0x00FF00,
        0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
    };

KeySym MCKeySymToLower(KeySym p_key)
{
	if ((p_key & XK_Class_mask) == XK_Class_codepoint)
		return MCS_tolower(p_key & XK_Codepoint_mask) | XK_Class_codepoint;
	else if (p_key < 0x80)
		return MCS_tolower(p_key);
	else
		return p_key;
}

MCMovingList::~MCMovingList()
{
	delete pts;
}

////////////////////////////////////////////////////////////////////////////////

MCPendingMessagesList::~MCPendingMessagesList()
{
    // Delete all messages remaining on the queue
    for (size_t i = GetCount(); i > 0; i--)
        DeleteMessage(i - 1, true);
    
    MCMemoryDelete(m_array);
}

void MCPendingMessage::DeleteParameters()
{
    while (m_params != NULL)
    {
        MCParameter *t_param = m_params;
        m_params = m_params->getnext();
        delete t_param;
    }
}

bool MCPendingMessagesList::InsertMessageAtIndex(size_t p_index, const MCPendingMessage& p_msg)
{
    MCAssert(p_index <= m_count);
    
    // Extend the array if necessary
    if (m_count + 1 > m_capacity)
    {
        if (!MCMemoryReallocate(m_array, (m_count + 1) * sizeof(MCPendingMessage), m_array))
            return false;
        
        // Ensure that the memory has been initialised
        new (&m_array[m_count]) MCPendingMessage;
        
        m_capacity = m_count + 1;
    }
    
    // Move all the messages in the range [p_index, m_count) up one
    for (size_t i = m_count; i > p_index; i--)
    {
        m_array[i] = m_array[i - 1];
    }
    
    // Insert the message into the newly-created space
    m_array[p_index] = p_msg;
    m_count += 1;
    return true;
}

void MCPendingMessagesList::DeleteMessage(size_t p_index, bool p_delete_params)
{
    MCAssert(p_index < m_count);
    
    if (p_delete_params)
        m_array[p_index].DeleteParameters();
    
    // Shift the remaining messages to cover the hole
    for (size_t i = p_index; i < m_count - 1; i++)
    {
        m_array[i] = m_array[i + 1];
    }
    
    // Clear the vacated entry at the end
    m_array[m_count - 1] = MCPendingMessage();
    
    m_count -= 1;
}

void MCPendingMessagesList::ShiftMessageTo(size_t p_to, size_t p_from, real64_t p_newtime)
{
    MCAssert(p_to < m_count);
    MCAssert(p_from < m_count);
    
    // Capture the message that needs moving
    MCPendingMessage t_msg = m_array[p_from];
    
    // Move all messages in the range [from + 1, to) down one
    for (size_t i = p_from; i < p_to; i++)
    {
        m_array[i] = m_array[i + 1];
    }
    
    // Move the message into place
    m_array[p_to] = t_msg;
    m_array[p_to].m_time = p_newtime;
}

////////////////////////////////////////////////////////////////////////////////

MCUIDC::MCUIDC()
{
#if defined(FEATURE_NOTIFY)
	MCNotifyInitialize();
#endif
    
	messageid = 0;
	moving = NULL;
	lockmoves = False;
	locktime = 0.0;
	ncolors = 0;
	colors = NULL;
	allocs = NULL;
	colornames = nil;
	lockmods = False;
    
	redbits = greenbits = bluebits = 8;
	redshift = 16;
	greenshift = 8;
	blueshift = 0;
	
	black_pixel.red = black_pixel.green = black_pixel.blue = 0;
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF;
	
	MCselectioncolor = MCpencolor = black_pixel;
	
	MConecolor = MCbrushcolor = white_pixel;
	
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;
	
	MChilitecolor.red = MChilitecolor.green = 0x0000;
	MChilitecolor.blue = 0x8080;
	
	MCaccentcolor = MChilitecolor;
	
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xC0C0;

	m_sound_internal = NULL ;

	// IM-2014-03-06: [[ revBrowserCEF ]] List of callback functions to call during wait()
	m_runloop_actions = nil;
    
    m_modal_loops = NULL;
}

MCUIDC::~MCUIDC()
{
#if defined(FEATURE_NOTIFY)
	MCNotifyFinalize();
#endif
}


bool MCUIDC::setbeepsound(MCStringRef p_beep_sound) 
{
	if (MCStringIsEqualToCString(p_beep_sound, "internal", kMCCompareCaseless))
	{
		m_sound_internal = "internal";
		return true ;
	}
	
	if (MCStringIsEqualToCString(p_beep_sound, "system", kMCCompareCaseless))
	{
		m_sound_internal = "system" ;
		return true ;
	}
	return false ;
}

bool MCUIDC::getbeepsound(MCStringRef& r_beep_sound)
{
	if ( m_sound_internal == NULL )
		m_sound_internal = "system" ;
	return MCStringCreateWithCString(m_sound_internal, r_beep_sound);
}


bool MCUIDC::hasfeature(MCPlatformFeature p_feature)
{
	return false;
}

void MCUIDC::setstatus(MCStringRef status)
{ }
Boolean MCUIDC::open()
{
	return True;
}
Boolean MCUIDC::close(Boolean force)
{
	return True;
}
MCNameRef MCUIDC::getdisplayname()
{
	return kMCEmptyName;
}
void MCUIDC::resetcursors()
{ }
void MCUIDC::setcursor(Window w, MCCursorRef c)
{ }
void MCUIDC::grabpointer(Window w)
{ }
void MCUIDC::ungrabpointer()
{ }
uint2 MCUIDC::getwidthmm()
{
	return 1;
}
uint2 MCUIDC::getheightmm()
{
	return 1;
}
uint2 MCUIDC::getmaxpoints()
{
	return 1;
}
uint2 MCUIDC::getvclass()
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-07-09: [[ Bug 12602 ]] Standard DC does not scale
MCPoint MCUIDC::logicaltoscreenpoint(const MCPoint &p_point)
{
	return p_point;
}

// IM-2014-07-09: [[ Bug 12602 ]] Standard DC does not scale
MCPoint MCUIDC::screentologicalpoint(const MCPoint &p_point)
{
	return p_point;
}

// IM-2014-07-09: [[ Bug 12602 ]] Standard DC does not scale
MCRectangle MCUIDC::logicaltoscreenrect(const MCRectangle &p_rect)
{
	return p_rect;
}

// IM-2014-07-09: [[ Bug 12602 ]] Standard DC does not scale
MCRectangle MCUIDC::screentologicalrect(const MCRectangle &p_rect)
{
	return p_rect;
}

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::setmouseloc(MCStack *p_target, MCPoint p_loc)
{
	MCPoint t_mouseloc;
	t_mouseloc = p_loc;

	if (p_target != nil)
		t_mouseloc = p_target->windowtostackloc(t_mouseloc);

	MCmousestackptr = p_target;
	MCmousex = t_mouseloc.x;
	MCmousey = t_mouseloc.y;
}

void MCUIDC::getmouseloc(MCStack *&r_target, MCPoint &r_loc)
{
    r_loc = MCPointMake(MCmousex, MCmousey);
    
    if (MCmousestackptr)
    {
        r_target = MCmousestackptr;
        r_loc = MCmousestackptr->stacktowindowloc(r_loc);
    }
    else
        r_target = nil;
}

void MCUIDC::setclickloc(MCStack *p_target, MCPoint p_loc)
{
	MCPoint t_clickloc;
	t_clickloc = p_loc;

	if (p_target != nil)
		t_clickloc = p_target->windowtostackloc(t_clickloc);

	MCclickstackptr = p_target;
	MCclicklocx = t_clickloc.x;
	MCclicklocy = t_clickloc.y;
}

void MCUIDC::getclickloc(MCStack *&r_target, MCPoint &r_loc)
{
    r_loc = MCPointMake(MCclicklocx, MCclicklocy);
    
    if (MCclickstackptr)
    {
        r_target = MCclickstackptr;
        r_loc = MCclickstackptr->stacktowindowloc(r_loc);
    }
    else
        r_target = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUIDC::fullscreenwindows(void)
{
	return false;
}

MCRectangle MCUIDC::fullscreenrect(const MCDisplay *p_display)
{
	return p_display->viewport;
}

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
uint2 MCUIDC::getwidth()
{
	return platform_getwidth();
}

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
uint2 MCUIDC::getheight()
{
	return platform_getheight();
}

//////////

uint16_t MCUIDC::platform_getwidth()
{
	return 1;
}

uint16_t MCUIDC::platform_getheight()
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

MCDisplay *MCUIDC::s_displays = NULL;
uint4 MCUIDC::s_display_count = 0;
bool MCUIDC::s_display_info_effective = false;

// IM-2014-01-24: [[ HiDPI ]] Refactor to implement caching of display info in MCUIDC instead of subclasses
// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
uint4 MCUIDC::getdisplays(const MCDisplay *&r_displays, bool p_effective)
{
	if (p_effective != s_display_info_effective || !platform_displayinfocacheable())
		cleardisplayinfocache();
	
	if (s_displays == nil)
	{
		/* UNCHECKED */ platform_getdisplays(p_effective, s_displays, s_display_count);
		s_display_info_effective = p_effective;
	}
	
	r_displays = s_displays;
	return s_display_count;
}

void MCUIDC::updatedisplayinfo(bool &r_changed)
{
	MCDisplay *t_displays;
	t_displays = nil;

	uint32_t t_display_count;
	t_display_count = 0;

	/* UNCHECKED */ platform_getdisplays(s_display_info_effective, t_displays, t_display_count);

	r_changed = t_display_count != s_display_count ||
		(MCMemoryCompare(t_displays, s_displays, sizeof(MCDisplay) * s_display_count) != 0);

	MCMemoryDeleteArray(s_displays);
	s_displays = t_displays;
	s_display_count = t_display_count;
}

void MCUIDC::cleardisplayinfocache(void)
{
	MCMemoryDeleteArray(s_displays);
	s_displays = nil;
	s_display_count = 0;
}

bool MCUIDC::platform_displayinfocacheable(void)
{
	return false;
}

//////////

bool MCUIDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
const MCDisplay *MCUIDC::getnearestdisplay(const MCRectangle& p_rectangle)
{
	MCDisplay const *t_displays;
	uint4 t_display_count;
	uint4 t_home;
	uint4 t_max_area, t_max_distance;
	uint4 t_max_area_index, t_max_distance_index;

	t_display_count = MCscreen -> getdisplays(t_displays, false);

	t_max_area = 0;
	t_max_distance = MAXUINT4;
    t_max_distance_index = 0;
	for(uint4 t_display = 0; t_display < t_display_count; ++t_display)
	{
		MCRectangle t_workarea;
		t_workarea = t_displays[t_display] . workarea;
		
		MCRectangle t_intersection;
		uint4 t_area, t_distance;
		t_intersection = MCU_intersect_rect(p_rectangle, t_workarea);
		t_area = t_intersection . width * t_intersection . height;

		uint4 t_dx, t_dy;
		t_dx = (t_workarea . x + t_workarea . width / 2) - (p_rectangle . x + p_rectangle . width / 2);
		t_dy = (t_workarea . y + t_workarea . height / 2) - (p_rectangle . y + p_rectangle . height / 2);
		t_distance = t_dx * t_dx + t_dy * t_dy;

		if (t_area > t_max_area)
		{
			t_max_area = t_area;
			t_max_area_index = t_display;
		}

		if (t_distance < t_max_distance)
		{
			t_max_distance = t_distance;
			t_max_distance_index = t_display;
		}
	}

	if (t_max_area == 0)
		t_home = t_max_distance_index;
	else
		t_home = t_max_area_index;

	return &t_displays[t_home];
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Return the maximum pixel scale of all displays in use
bool MCUIDC::getmaxdisplayscale(MCGFloat &r_scale)
{
	const MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	t_count = MCscreen->getdisplays(t_displays, false);
	
	MCGFloat t_scale;
	if (t_count == 0)
		t_scale = 1.0;
	else
		t_scale = t_displays[0].pixel_scale;
	
	for (uint32_t i = 1; i < t_count; i++)
		if (t_displays[i].pixel_scale > t_scale)
			t_scale = t_displays[i].pixel_scale;
	
	r_scale = t_scale;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
Boolean MCUIDC::getwindowgeometry(Window p_window, MCRectangle &r_rect)
{
	if (!platform_getwindowgeometry(p_window, r_rect))
		return False;
	
	return True;
}

//////////

bool MCUIDC::platform_getwindowgeometry(Window p_window, MCRectangle &r_rect)
{
	r_rect = MCU_make_rect(0, 0, 32, 32);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
void MCUIDC::boundrect(MCRectangle &x_rect, Boolean p_title, Window_mode p_mode, Boolean p_resizable)
{
	platform_boundrect(x_rect, p_title, p_mode, p_resizable);
}

//////////

void MCUIDC::platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable)
{ }

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
void MCUIDC::querymouse(int2 &x, int2 &y)
{
	platform_querymouse(x, y);
}

//////////

void MCUIDC::platform_querymouse(int2 &x, int2 &y)
{
    x = y = 0;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
void MCUIDC::setmouse(int2 x, int2 y)
{
	platform_setmouse(x, y);
}

//////////

void MCUIDC::platform_setmouse(int2 x, int2 y)
{ }

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
MCStack *MCUIDC::getstackatpoint(int32_t x, int32_t y)
{
	return platform_getstackatpoint(x, y);
}

//////////

MCStack *MCUIDC::platform_getstackatpoint(int32_t x, int32_t y)
{
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::openwindow(Window w, Boolean override)
{ }
void MCUIDC::closewindow(Window window)
{ }
void MCUIDC::destroywindow(Window &window)
{ }
void MCUIDC::raisewindow(Window window)
{ }
void MCUIDC::iconifywindow(Window window)
{ }
void MCUIDC::uniconifywindow(Window window)
{ }
void MCUIDC::setname(Window window, MCStringRef newname)
{ }
void MCUIDC::setcmap(MCStack *sptr)
{ }
void MCUIDC::sync(Window w)
{ }

void MCUIDC::flush(Window w)
{ }

void MCUIDC::beep()
{ }
void MCUIDC::setinputfocus(Window window)
{ }

uint2 MCUIDC::getrealdepth(void)
{
	//fprintf(stderr,"UIDC::getrealdepth() called\n");
	return 0;
}
uint2 MCUIDC::getdepth(void)
{
	return 0;
}

void MCUIDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{ }
void MCUIDC::copyarea(Drawable source, Drawable dest, int2 depth,
                      int2 sx, int2 sy, uint2 sw, uint2 sh,
                      int2 dx, int2 dy, uint4 rop)
{ }

MCColorTransformRef MCUIDC::createcolortransform(const MCColorSpaceInfo& info)
{
	return nil;
}

void MCUIDC::destroycolortransform(MCColorTransformRef transform)
{
}

bool MCUIDC::transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image)
{
	return false;
}

MCCursorRef MCUIDC::createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot)
{
	return nil;
}

void MCUIDC::freecursor(MCCursorRef c)
{ }

uintptr_t MCUIDC::dtouint(Drawable d)
{
	return 1;
}


Boolean MCUIDC::uinttowindow(uintptr_t, Window &w)
{
	w = (Window)1;
	return True;
}

void *MCUIDC::GetNativeWindowHandle(Window p_window)
{
	return nil;
}


void MCUIDC::getbeep(uint4 property, int4& r_value)
{
	r_value = 0;
}

void MCUIDC::setbeep(uint4 property, int4 beep)
{ }

MCNameRef MCUIDC::getvendorname(void)
{
	return kMCEmptyName;
}

uint2 MCUIDC::getpad()
{
	return 32;
}

Window MCUIDC::getroot()
{
	return (Window)1;
}

MCImageBitmap *MCUIDC::snapshot(MCRectangle &r, uint4 window,
                           MCStringRef displayname, MCPoint *size)
{
	return NULL;
}

void MCUIDC::enablebackdrop(bool p_hard)
{
}

void MCUIDC::disablebackdrop(bool p_hard)
{
}

void MCUIDC::configurebackdrop(const MCColor&, MCPatternRef p_pattern, MCImage *)
{
}

void MCUIDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
}

void MCUIDC::hidemenu()
{ }
void MCUIDC::showmenu()
{ }
void MCUIDC::hidetaskbar()
{ }
void MCUIDC::showtaskbar()
{ }

void MCUIDC::getpaletteentry(uint4 n, MCColor &c)
{
	uint32_t t_pixel;
	if (n < 256)
		t_pixel = stdcmap[n];
	else
		t_pixel = 0;
	MCColorSetPixel(c, t_pixel);
}

MCColor *MCUIDC::getaccentcolors()
{
	return NULL;
}

void MCUIDC::expose()
{ }
Boolean MCUIDC::abortkey()
{
	return False;
}

void MCUIDC::waitconfigure(Window w)
{ }
void MCUIDC::waitreparent(Window w)
{ }
void MCUIDC::waitfocus()
{ }
uint2 MCUIDC::querymods()
{
	return 0;
}
Boolean MCUIDC::getmouse(uint2 button, Boolean& r_abort)
{
	r_abort = False;
	return False;
}
Boolean MCUIDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	r_abort = False;
	return False;
}

Boolean MCUIDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
	MCwaitdepth++;
    MCDeletedObjectsEnterWait(dispatch);
    
	real8 curtime = MCS_time();
	if (duration < 0.0)
		duration = 0.0;
	real8 exittime = curtime + duration;
    Boolean abort = False;
	Boolean done = False;
	Boolean donepending = False;
	do
	{
		// IM-2014-03-06: [[ revBrowserCEF ]] call additional runloop callbacks
		DoRunloopActions();

		real8 eventtime = exittime;
        donepending = handlepending(curtime, eventtime, dispatch) ||
            (dispatch && MCEventQueueDispatch());
        
		siguser();
        
		MCModeQueueEvents();

#if defined(FEATURE_NOTIFY)
		if ((MCNotifyDispatch(dispatch == True) || donepending) && anyevent)
			break;
#endif
		if (abort)
			break;
		if (MCquit)
            break;
        
		if (curtime < eventtime)
		{
			done = MCS_poll(donepending ? 0 : eventtime - curtime, 0);
			curtime = MCS_time();
		}
	}
	while (curtime < exittime  && !(anyevent && (done || donepending)));
    
    if (MCquit)
        abort = True;
    
    MCDeletedObjectsLeaveWait(dispatch);
    MCwaitdepth--;
    
	return abort;
}

void MCUIDC::pingwait(void)
{
#if defined(_DESKTOP) && defined(FEATURE_NOTIFY)
	// MW-2013-06-14: [[ DesktopPingWait ]] Use the notify mechanism to wake up
	//   any running wait.
	MCNotifyPing(false);
#endif
}

bool MCUIDC::FindRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action)
{
	for (MCRunloopAction *t_action = m_runloop_actions; t_action != nil; t_action = t_action->next)
	{
		if (t_action->callback == p_callback && t_action->context == p_context)
		{
			r_action = t_action;
			return true;
		}
	}
	
	return false;
}

// IM-2014-03-06: [[ revBrowserCEF ]] Add callback & context to runloop action list
bool MCUIDC::AddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action)
{
	MCRunloopAction *t_action;
	t_action = nil;

	if (FindRunloopAction(p_callback, p_context, t_action))
	{
		r_action = t_action;
		r_action->references++;
		
		return true;
	}
	
	if (!MCMemoryNew(t_action))
		return false;

	t_action->callback = p_callback;
	t_action->context = p_context;

	t_action->references = 1;
	t_action->next = m_runloop_actions;
	
	m_runloop_actions = t_action;

	r_action = t_action;

	return true;
}

// IM-2014-03-06: [[ revBrowserCEF ]] Remove action from runloop action list
void MCUIDC::RemoveRunloopAction(MCRunloopActionRef p_action)
{
	if (p_action == nil)
		return;

	if (p_action->references > 1)
	{
		p_action->references--;
		return;
	}
	
	MCRunloopAction *t_remove_action;
	t_remove_action = nil;

	if (p_action == m_runloop_actions)
	{
		t_remove_action = m_runloop_actions;
		m_runloop_actions = p_action->next;
	}
	else
	{
		MCRunloopAction *t_action;
		t_action = m_runloop_actions;

		while (t_action != nil && t_remove_action == nil)
		{
			if (t_action->next == p_action)
			{
				t_remove_action = p_action;
				t_action->next = p_action->next;
			}

			t_action = t_action->next;
		}
	}

	if (t_remove_action != nil)
		MCMemoryDelete(t_remove_action);
}

// IM-2014-03-06: [[ revBrowserCEF ]] Call runloop action callbacks
void MCUIDC::DoRunloopActions(void)
{
	MCRunloopAction *t_action;
	t_action = m_runloop_actions;

	while (t_action != nil)
	{
		// IM-2014-05-06: [[ Bug 12364 ]] Guard against runloop action deletion within callback
		MCRunloopAction *t_next;
		t_next = t_action->next;

		t_action->callback(t_action->context);
		t_action = t_next;
	}
}

bool MCUIDC::HasRunloopActions()
{
	return m_runloop_actions != nil;
}

void MCUIDC::flushevents(uint2 e)
{ }
;

Boolean MCUIDC::istripleclick()
{
	return False;
}

bool MCUIDC::getkeysdown(MCListRef& r_list)
{
	r_list = MCValueRetain(kMCEmptyList);
	return true;
}

uint1 MCUIDC::fontnametocharset(MCStringRef p_fontname)
{
	return 0;
}

void MCUIDC::openIME()
{}
void MCUIDC::activateIME(Boolean activate)
{}
void MCUIDC::clearIME(Window w)
{}
void MCUIDC::closeIME()
{}
void MCUIDC::configureIME(int32_t x, int32_t y)
{}

void MCUIDC::updatemenubar(Boolean force)
{
	if (!MCdefaultmenubar)
		MCdefaultmenubar = MCmenubar;

	MCGroup *newMenuGroup;
	if (MCmenubar)
		newMenuGroup = MCmenubar;
	else
		newMenuGroup = MCdefaultmenubar;
	if (newMenuGroup != NULL)
	{
		MCButton *bptr;
		uint2 i = 0;
		uint2 which = 0;
		while ((bptr = (MCButton *)newMenuGroup->findnum(CT_MENU, i)) != NULL)
		{
			bptr->findmenu();
			which++;
			i = which;
		}
	}
}

// MW-2014-04-16: [[ Bug 11690 ]] Pending message list is now sorted by time, all
//   pending message generation functions use 'doaddmessage()' to insert the
//   message in the right place.
void MCUIDC::doaddmessage(MCObject *optr, MCNameRef mptr, real8 time, uint4 id, MCParameter *params)
{
    // Find where in the list to insert the pending message.
    size_t t_index;
    for(t_index = 0; t_index < m_messages.GetCount(); t_index++)
        if (m_messages[t_index].m_time > time)
            break;

    m_messages.InsertMessageAtIndex(t_index, MCPendingMessage(optr, mptr, time, params, id));
}

// MW-2014-04-16: [[ Bug 11690 ]] Shift a message to a new time in the future.
size_t MCUIDC::doshiftmessage(size_t index, real8 newtime)
{
    // Find the first message after the new time.
    size_t t_index;
    for(t_index = index; t_index < m_messages.GetCount() - 1; t_index++)
        if (m_messages[t_index + 1].m_time > newtime)
            break;
    
    // If the message is already in the correct place, do nothing
    if (t_index == index)
        return index;
    
    m_messages.ShiftMessageTo(t_index, index, newtime);
    
    return t_index;
}

void MCUIDC::delaymessage(MCObject *optr, MCNameRef mptr, MCStringRef p1, MCStringRef p2)
{
	MCParameter *params = NULL;
	if (p1 != NULL)
	{
		params = new (nothrow) MCParameter;
		params->setvalueref_argument(p1);
		if (p2 != NULL)
		{
			params->setnext(new MCParameter);
			params->getnext()->setvalueref_argument(p2);
		}
	}
    
    doaddmessage(optr, mptr, MCS_time(), ++messageid, params);
}

void MCUIDC::addmessage(MCObject *optr, MCNameRef mptr, real8 time, MCParameter *params)
{
    uint4 t_id;
    t_id = ++messageid;
    doaddmessage(optr, mptr, time, t_id, params);
    
    // MW-2014-05-28: [[ Bug 12463 ]] Previously the result would have been set here which is
    //   incorrect as engine pending messages should not set the result.
}

void MCUIDC::addtimer(MCObject *optr, MCNameRef mptr, uint4 delay)
{
    // Remove existing message from the queue.
    cancelmessageobject(optr, mptr);
    
    doaddmessage(optr, mptr, MCS_time() + delay / 1000.0, 0);
}

void MCUIDC::addsubtimer(MCObject *optr, MCValueRef suboptr, MCNameRef mptr, uint4 delay)
{
    cancelmessageobject(optr, mptr, suboptr);
    
    MCParameter *t_param;
    t_param = new (nothrow) MCParameter;
    t_param -> setvalueref_argument(suboptr);
    doaddmessage(optr, mptr, MCS_time() + delay / 1000.0, 0, t_param);
}

void MCUIDC::cancelsubtimer(MCObject *optr, MCNameRef mptr, MCValueRef suboptr)
{
    cancelmessageobject(optr, mptr, suboptr);
}

void MCUIDC::cancelmessageindex(size_t i, bool dodelete)
{
	m_messages.DeleteMessage(i, dodelete);
}

void MCUIDC::cancelmessageid(uint4 id)
{
	for(size_t i = 0 ; i < m_messages.GetCount(); i++)
    {
		if (m_messages[i].m_id == id)
		{
			cancelmessageindex(i, True);
			return;
		}
    }
}

void MCUIDC::cancelmessageobject(MCObject *optr, MCNameRef mptr, MCValueRef subobject)
{
    // MW-2014-05-14: [[ Bug 12294 ]] Cancel list in reverse order to minimize movement.
	for (size_t i = m_messages.GetCount(); i > 0; i--)
    {
        const MCPendingMessage& t_msg = m_messages[i - 1];
        
	// If this message refers to a dead object, take this opportunity to
	// prune it from the pending queue
	if (!t_msg.m_object.IsValid())
	{
	    cancelmessageindex(i - 1, true);
	    continue;
	}
        
        if (t_msg.m_object.Get() == optr
		        && (mptr == NULL || MCNameIsEqualToCaseless(*t_msg.m_message, mptr))
                && (subobject == NULL || (t_msg.m_params != nil &&
                                          t_msg.m_params -> getvalueref_argument() == subobject)))
			cancelmessageindex(i - 1, true);
    }
}

bool MCUIDC::listmessages(MCExecContext& ctxt, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	for (size_t i = 0; i < m_messages.GetCount(); i++)
	{
		const MCPendingMessage& t_msg = m_messages[i];
        
        if (t_msg.m_id != 0)
		{
			MCAutoListRef t_msg_info;
			MCAutoValueRef t_id_string;
			MCAutoStringRef t_time_string;

            if (!t_msg.m_object.IsValid())
                continue;

			if (!MCListCreateMutable(',', &t_msg_info))
				return false;

			if (!MCListAppendUnsignedInteger(*t_msg_info, t_msg.m_id))
				return false;

			if (!ctxt.FormatReal(t_msg.m_time, &t_time_string)
				|| !MCListAppend(*t_msg_info, *t_time_string))
				return false;

			if (!MCListAppend(*t_msg_info, *t_msg.m_message))
				return false;

			if (!t_msg.m_object->names(P_LONG_ID, &t_id_string) ||
				!MCListAppend(*t_msg_info, *t_id_string))
				return false;

			if (!MCListAppend(*t_list, *t_msg_info))
				return false;
		}
	}

	return MCListCopy(*t_list, r_list);
}

// MW-2014-05-28: [[ Bug 12463 ]] This is called by 'send in time' to queue a user defined message.
//   It puts a limit on the number of script sent messages of 64k which should be enough for any
//   reasonable app. Note that the engine's internal / sent messages are still allowed beyond this
//   limit as they definitely do not have a double-propagation problem that could cause engine lock-up.
bool MCUIDC::addusermessage(MCObject* optr, MCNameRef name, real8 time, MCParameter *params)
{
    // Arbitrary limit on the number of pending messages
    if (m_messages.GetCount() >= UINT16_MAX)
        return false;
    
    addmessage(optr, name, time, params);
    
    // MW-2014-05-28: [[ Bug 12463 ]] Set the result to the pending message id.
	char buffer[U4L];
	sprintf(buffer, "%u", messageid);
	MCresult->copysvalue(buffer);
    
    return true;
}

bool MCUIDC::hasmessagestodispatch(void)
{
    if (m_messages.GetCount() == 0)
    {
        return false;
    }
    
    return m_messages[0].m_time <= MCS_time();
}

// MW-2014-04-16: [[ Bug 11690 ]] Rework pending message handling to take advantage
//   of messages[] now being a sorted list.
Boolean MCUIDC::handlepending(real8& curtime, real8& eventtime, Boolean dispatch)
{
    Boolean t_handled;
    t_handled = False;
    for(uindex_t i = 0; i < m_messages.GetCount(); i++)
    {
        MCPendingMessage t_msg = m_messages[i];
        
        // If the next message is later than curtime, we've not processed a message.
        if (t_msg.m_time > curtime)
            break;
        
        if (!dispatch && t_msg.m_id == 0 && MCNameIsEqualToCaseless(*t_msg.m_message, MCM_idle))
        {
            doshiftmessage(i, curtime + MCidleRate / 1000.0);
            continue;
        }
        
        if (dispatch || t_msg.m_id == 0)
        {
            // Remove this message from the queue
            cancelmessageindex(i, false);
            
            // If the object is still live, dispatch the message to it
            if (t_msg.m_object.IsValid())
            {
                MCSaveprops sp;
                MCU_saveprops(sp);
                MCU_resetprops(False);
                t_msg.m_object->timer(*t_msg.m_message, t_msg.m_params);
                MCU_restoreprops(sp);
                t_msg.DeleteParameters();
            }
            
            // A message has been removed from the queue, so don't increment the
            // counter on this iteration.
            i -= 1;
            
            curtime = MCS_time();
            
            t_handled = True;
            break;
        }
    }
    
    if (moving != NULL)
        handlemoves(curtime, eventtime);
    
	real8 stime = IO_cleansockets(curtime);
    if (stime < eventtime)
        eventtime = stime;
    
    // SN-2014-12-12: [[ Bug 13360 ]] We don't want to change the eventtime if the message is not forced to be dispatched nor internal
    if (m_messages.GetCount() > 0
            && (dispatch || m_messages[0].m_id == 0)
            && m_messages[0].m_time < eventtime)
        eventtime = m_messages[0].m_time;
    
    return t_handled;
}


Boolean MCUIDC::getlockmoves() const
{
	return lockmoves;
}

void MCUIDC::setlockmoves(Boolean b)
{
	if (lockmoves == b)
		return;

	lockmoves = b;

	if (lockmoves) {
		// then save the time the lock started
		locktime = MCS_time(); 

	} else {
		// adjust the start time of each movement.
		real8 offset = MCS_time() - locktime;
		if (moving != NULL)	{
			MCMovingList *mptr = moving;
			do {
				mptr->starttime += offset;
				mptr = mptr->next();
			} while (mptr != moving);
		}
	}
}

void MCUIDC::addmove(MCObject *optr, MCPoint *pts, uint2 npts,
                     real8 &duration, Boolean waiting)
{
	stopmove(optr, False);
	MCMovingList *mptr = new (nothrow) MCMovingList;
	mptr->appendto(moving);
	mptr->object = optr;
	mptr->pts = pts;
	mptr->lastpt = npts - 1;
	mptr->curpt = 0;
	mptr->dx = pts[1].x - pts[0].x;
	mptr->dy = pts[1].y - pts[0].y;
	mptr->donex = pts[1].x - (optr->getrect().width >> 1);
	mptr->doney = pts[1].y - (optr->getrect().height >> 1);
	mptr->waiting = waiting;

	real8 distance = 0.0;
	uint2 i;
	for (i = 0 ; i < mptr->lastpt ; i++)
	{
		real8 dx = pts[i + 1].x - pts[i].x;
		real8 dy = pts[i + 1].y - pts[i].y;
		distance += sqrt(dx * dx + dy * dy);
	}
	if (duration == 0.0)
	{
		// MW-2009-10-31: [[ Bug 8176 ]] Make sure we use a minimum of 1 in the divide!
		mptr->speed = MCmovespeed == 0 ? 1 : MCmovespeed;
		duration = distance / (MCmovespeed == 0 ? 1 : MCmovespeed);
	}
	else
		mptr->speed = distance / duration;
	mptr->duration = sqrt((double)(mptr->dx * mptr->dx
	                               + mptr->dy * mptr->dy)) / mptr->speed;
	MCRectangle rect = optr->getrect();
	MCRectangle newrect = rect;
	newrect.x = pts[0].x - (rect.width >> 1);
	newrect.y = pts[0].y - (rect.height >> 1);
	if (rect.x != newrect.x || rect.y != newrect.y)
	{
		// MW-2011-08-18: [[ Layers ]] Notify of position change.
		if (optr -> gettype() >= CT_GROUP)
			static_cast<MCControl *>(optr)->layer_setrect(newrect, false);
		else
			optr -> setrect(newrect);
	}

	if (lockmoves) {
		mptr->starttime = locktime;
	} else {
		mptr->starttime = MCS_time();
	}
}

bool MCUIDC::listmoves(MCExecContext& ctxt, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	if (moving != NULL)
	{
		MCMovingList *mptr = moving;
		do
		{
            if (mptr->object)
            {
                MCAutoValueRef t_string;
                if (!mptr->object->names(P_LONG_ID, &t_string))
                    return false;
                if (!MCListAppend(*t_list, *t_string))
                    return false;
            }
			mptr = mptr->next();
		}
		while (mptr != moving);
	}
	return MCListCopy(*t_list, r_list);
}

void MCUIDC::stopmove(MCObject *optr, Boolean finish)
{
	if (moving != NULL)
	{
		MCMovingList *mptr = moving;
		do
		{
			if (mptr->object.IsBoundTo(optr))
			{
				mptr->remove(moving);
				if (finish)
				{
					MCRectangle rect = mptr->object->getrect();
					int2 donex = mptr->pts[mptr->lastpt].x - (rect.width >> 1);
					int2 doney = mptr->pts[mptr->lastpt].y - (rect.height >> 1);
					if (rect.x != donex || rect.y != doney)
					{
						MCRectangle newrect = rect;
						newrect.x = donex;
						newrect.y = doney;

						// MW-2011-08-18: [[ Layers ]] Notify of position change.
						if (mptr->object -> gettype() >= CT_GROUP)
							mptr->object.GetAs<MCControl>()->layer_setrect(newrect, false);
						else
							mptr->object->setrect(newrect);
					}
				}
				delete mptr;
				break;
			}
			mptr = mptr->next();
		}
		while (mptr != moving);
	}
}

void MCUIDC::handlemoves(real8 &curtime, real8 &eventtime)
{
	if (lockmoves) 
		return;
	eventtime = curtime + (real8)MCsyncrate / 1000.0;
	MCMovingList *mptr = moving;
	Boolean moved = False;
	Boolean done = False;
	do
	{
        if (!mptr->object)
        {
            moving = mptr->prev();
            mptr->remove(moving);
            delete mptr;
            if (moving == NULL)
                mptr = NULL;
            else
                mptr = moving->next();
            continue;
        }
        
        MCRectangle rect = mptr->object->getrect();
        MCRectangle newrect = rect;
        real8 dt = 0.0;
        if (curtime >= mptr->starttime + mptr->duration
            || (rect.x == mptr->donex && rect.y == mptr->doney))
        {
            newrect.x = mptr->donex;
            newrect.y = mptr->doney;
            dt = curtime - (mptr->starttime + mptr->duration);
            done = True;
        }
        else
        {
            newrect.x = mptr->pts[mptr->curpt].x - (rect.width >> 1)
                        + (int2)(mptr->dx * (curtime - mptr->starttime) / mptr->duration);
            newrect.y = mptr->pts[mptr->curpt].y - (rect.height >> 1)
                        + (int2)(mptr->dy * (curtime - mptr->starttime) / mptr->duration);
        }
        if (newrect.x != rect.x || newrect.y != rect.y)
        {
            moved = True;
        
            // MW-2011-08-18: [[ Layers ]] Notify of position change.
            if (mptr->object -> gettype() >= CT_GROUP)
                mptr->object.GetAs<MCControl>()->layer_setrect(newrect, false);
            else
                mptr->object->setrect(newrect);
        }

		if (done)
		{
			if (mptr->curpt < mptr->lastpt - 1)
			{
				do
				{
					mptr->curpt++;
					mptr->dx = mptr->pts[mptr->curpt + 1].x - mptr->pts[mptr->curpt].x;
					mptr->dy = mptr->pts[mptr->curpt + 1].y - mptr->pts[mptr->curpt].y;
					mptr->duration = sqrt((double)(mptr->dx * mptr->dx + mptr->dy
					                               * mptr->dy)) / mptr->speed;
					dt -= mptr->duration;
				}
				while (dt > 0.0 && mptr->curpt < mptr->lastpt - 1);
				mptr->duration = -dt;
				mptr->starttime = curtime;
				mptr->donex = mptr->pts[mptr->curpt + 1].x - (rect.width >> 1);
				mptr->doney = mptr->pts[mptr->curpt + 1].y - (rect.height >> 1);
			}
			else
			{
				moving = mptr->prev();
				mptr->remove(moving);
				if (!mptr->waiting)
                {
					if (MClockmessages)
						delaymessage(mptr->object, MCM_move_stopped);
					else
						mptr->object->message(MCM_move_stopped);
                }
				delete mptr;
				if (moving == NULL)
					mptr = NULL;
				else
					mptr = moving->next();
			}
			done = False;
		}
		else
			mptr = mptr->next();
	}
	while (mptr != NULL && mptr != moving);
		
	// MW-2012-12-09: [[ Bug 9905 ]] Make sure we update the screen if something
	//   moved (previously it only did so if there were still things to move also!).
	if (moved)
	{
		// MW-2011-09-08: [[ Redraw ]] Make sure the screen is updated.
		MCRedrawUpdateScreen();
	}
}

void MCUIDC::siguser()
{
	while (MCsiguser1)
	{
		MCsiguser1--;
		MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_signal, MCSTR("1"));
	}
	while (MCsiguser2)
	{
		MCsiguser2--;
		MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_signal, MCSTR("2"));
	}
}

#include "rgb.cpp"

Boolean MCUIDC::lookupcolor(MCStringRef s, MCColor *color)
{
	MCAutoStringRefAsCString t_cstring;
	if (!t_cstring.Lock(s))
		return false;

    uint4 slength = strlen(*t_cstring);
    /* UNCHECKED */ MCAutoPointer<char[]> startptr = new (nothrow) char[slength + 1];

    MCU_lower(*startptr, *t_cstring);

    (*startptr)[slength] = '\0';

    char* sptr = *startptr;

	if (*sptr == '#')
	{
		uint2 r, g, b;
		sptr++;
		slength--;
		if (slength != 3 && slength != 6 && slength != 9 && slength != 12)
			return False;
		slength /= 3;
		g = b = 0;
		do
		{
			r = g;
			g = b;
			b = 0;
			int4 i;
			for (i = slength ; --i >= 0 ; )
			{
				char c = *sptr++;
				b <<= 4;
				if (c >= '0' && c <= '9')
					b |= c - '0';
				else
					if (c >= 'a' && c <= 'f')
						b |= c - ('a' - 10);
					else
						return False;
			}
		}
		while (*sptr != '\0');
		int4 goodbits = slength << 2;
		int4 shiftbits = 16 - goodbits;
		color->red = color->blue = color->green = 0;
		while (shiftbits > -goodbits)
		{
			if (shiftbits < 0)
			{
				color->red |= r >> -shiftbits;
				color->green |= g >> -shiftbits;
				color->blue |= b >> -shiftbits;
			}
			else
			{
				color->red |= r << shiftbits;
				color->green |= g << shiftbits;
				color->blue |= b << shiftbits;
			}
			shiftbits -= goodbits;
		}
		return True;
	}
	char *tptr = sptr;
	while (*tptr)
		if (isspace((uint1)*tptr))
            memmove(tptr, tptr + 1, strlen(tptr));
		else
			tptr++;
	uint2 high = ELEMENTS(color_table);
	uint2 low = 0;
	int2 cond;
	while (low < high)
	{
		uint2 mid = low + ((high - low) >> 1);
		uint4 length = MCU_min(slength, strlen(color_table[mid].token)) + 1;
		if ((cond = MCU_strncasecmp(sptr, color_table[mid].token, length)) < 0)
			high = mid;
		else
			if (cond > 0)
				low = mid + 1;
			else
			{
				color->red = (color_table[mid].red << 8) + color_table[mid].red;
				color->green = (color_table[mid].green << 8) + color_table[mid].green;
				color->blue = (color_table[mid].blue << 8) + color_table[mid].blue;
				return True;
			}
	}
	return False;
}

void MCUIDC::dropper(Window w, int2 mx, int2 my, MCColor *cptr)
{
	MCColor newcolor;
	// MW-2012-03-30: [[ Bug ]] On Mac, use the snapshot method to get the mouseColor
	//   otherwise things fail on Lion.
	MCRectangle t_rect;
	MCU_set_rect(t_rect, mx, my, 1, 1);
	
	// IM-2013-07-30: [[ Bug 11018 ]] if the target is a window, then convert local coords to global
	if (w != nil)
	{
		MCRectangle t_stack_rect;
		MCStack *t_stack;
		t_stack = MCdispatcher->findstackd(w);
		if (t_stack != nil)
		{
			t_stack_rect = t_stack->getrect();
			t_rect.x += t_stack_rect.x;
			t_rect.y += t_stack_rect.y;
		}
	}
	
	MCImageBitmap *image = snapshot(t_rect, 0, nil, nil);

	// If fetching the mouse pixel fails, then just return black.
	if (image == NULL)
	{
		*cptr = MCscreen -> getblack();
		return;
	}

	MCColorSetPixel(newcolor, MCImageBitmapGetPixel(image, 0, 0));
	MCImageFreeBitmap(image);
	if (cptr != NULL)
		*cptr = newcolor;
	else
	{
		if (MCmodifierstate & MS_CONTROL)
		{
			if (MCbrushpattern != nil)
				MCpatternlist->freepat(MCbrushpattern);

			MCbrushcolor = newcolor;
		}
		else
		{
			if (MCpenpattern != nil)
				MCpatternlist->freepat(MCpenpattern);
			MCpencolor = newcolor;
		}
	}
}

/* WRAPPER */
bool MCUIDC::parsecolor(MCStringRef p_string, MCColor& r_color)
{
	return True == parsecolor(p_string, r_color, nil);
}

Boolean MCUIDC::parsecolor(MCStringRef s, MCColor& color, MCStringRef *cname)
{
	if (cname != nil)
	{
		MCValueRelease(*cname);
		*cname = nil;
	}
	
	int2 i1, i2, i3;
    Boolean done;
	MCAutoStringRefAsCString t_cstring;
	if (!t_cstring.Lock(s))
		return false;
	const char *sptr = *t_cstring;
    uint4 l = strlen(sptr);
	
	// check for numeric first argument
	i1 = MCU_strtol(sptr, l, ',', done);
	if (!done)
	{
        // not numeric, check against the colornames	
		if (lookupcolor(s, &color))
        {
			if (cname)
				*cname = MCValueRetain(s);
			return True;
		}
		return False;
	}
	// check for a numeric second argument (Green value)
	i2 = MCU_strtol(sptr, l, ',', done);
	if (!done)
	{
		// MDW-2013-06-12: [[ Bug 10950 ]] non-numeric second argument present
		if (l != 0)
			return False;
		// only a single integer as the color specification,
		// restrict it to 0-255 and get values from sccolors array
		i1 = MCU_max(1, MCU_min(i1, 256)) - 1;
		i3 = sccolors[i1].blue;
		i2 = sccolors[i1].green;
		i1 = sccolors[i1].red;
	}
	else
	{
		// check for a numeric third argument (Blue value)
		i3 = MCU_strtol(sptr, l, ',', done);
		// MDW-2013-06-12: [[ Bug 10950 ]] third argument not present or not numeric
		// or fourth argument present
		if (!done || (l != 0))
			return False;
	}
	color.red = (uint2)(i1 << 8) + i1;
	color.green = (uint2)(i2 << 8) + i2;
	color.blue = (uint2)(i3 << 8) + i3;
	
	return True;
}


bool MCUIDC::getcolornames(MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	uint2 end = ELEMENTS(color_table);
	uint2 i;
	for (i = 0 ; i < end ; i++)
	{
		if (!MCListAppendCString(*t_list, color_table[i].token))
			return false;
	}

	return MCListCopyAsString(*t_list, r_string);
}

void MCUIDC::seticon(uint4 p_icon)
{
}

void MCUIDC::seticonmenu(MCStringRef p_menu)
{
}

void MCUIDC::configurestatusicon(uint32_t icon_id, MCStringRef menu, MCStringRef tooltip)
{
}

MCPrinter *MCUIDC::createprinter(void)
{
	return new MCNullPrinter;
}

bool MCUIDC::listprinters(MCStringRef& r_printers)
{
	r_printers = (MCStringRef)MCValueRetain(kMCEmptyString);
	return true;
}

//

int4 MCUIDC::getsoundvolume(void)
{
	return 0;
}

void MCUIDC::setsoundvolume(int4 p_volume)
{
}

void MCUIDC::startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume)
{
}

void MCUIDC::stopplayingsound(void)
{
}

//

// TD-2013-07-01: [[ DynamicFonts ]]
bool MCUIDC::loadfont(MCStringRef p_path, bool p_globally, void*& r_loaded_font_handle)
{
	return false;
}

bool MCUIDC::unloadfont(MCStringRef p_path, bool p_globally, void *r_loaded_font_handle)
{
	return false;
}

//

MCDragAction MCUIDC::dodragdrop(Window w, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset)
{
	return DRAG_ACTION_NONE;
}

//

MCScriptEnvironment *MCUIDC::createscriptenvironment(MCStringRef p_language)
{
	return NULL;
}

void MCUIDC::enactraisewindows(void)
{

}
//

int32_t MCUIDC::popupanswerdialog(MCStringRef *p_buttons, uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message, bool p_blocking)
{
	return 0;
}

bool MCUIDC::popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result)
{
	return false;
}

//

void MCUIDC::controlgainedfocus(MCStack *s, uint32_t id)
{
}

void MCUIDC::controllostfocus(MCStack *s, uint32_t id)
{
}

//

void MCUIDC::hidecursoruntilmousemoves(void)
{
    // Default action is to do nothing - Mac overrides and performs the
    // appropriate function.
}

////////////////////////////////////////////////////////////////////////////////

bool MCUIDC::platform_get_display_handle(void *&r_display)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::breakModalLoops()
{
    modal_loop* loop = m_modal_loops;
    while (loop != NULL)
    {
        loop->break_function(loop->context);
        loop->broken = true;
        loop = loop->chain;
    }
}

void MCUIDC::modalLoopStart(modal_loop& info)
{
    info.chain = m_modal_loops;
    info.broken = false;
    m_modal_loops = &info;
}

void MCUIDC::modalLoopEnd()
{
    m_modal_loops = m_modal_loops->chain;
}

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::getsystemappearance(MCSystemAppearance &r_appearance)
{
	r_appearance = kMCSystemAppearanceLight;
}

////////////////////////////////////////////////////////////////////////////////
