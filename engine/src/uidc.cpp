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

#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "param.h"
#include "globals.h"
#include "execpt.h"
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

#include "graphicscontext.h"

#include "resolution.h"

class MCNullPrinter: public MCPrinter
{
protected:
	void DoInitialize(void) {}
	void DoFinalize(void) {}

	bool DoReset(const char *p_name) {return false;}
	bool DoResetSettings(const MCString& p_settings) {return false;}

	const char *DoFetchName(void) {return "";}
	void DoFetchSettings(void*& r_buffer, uint4& r_length) {r_buffer = NULL; r_length = 0;}

	void DoResync(void) {}

	MCPrinterDialogResult DoPrinterSetup(bool p_window_modal, Window p_owner) {return PRINTER_DIALOG_RESULT_ERROR;}
	MCPrinterDialogResult DoPageSetup(bool p_window_modal, Window p_owner) {return PRINTER_DIALOG_RESULT_ERROR;}

	MCPrinterResult DoBeginPrint(const char *p_document, MCPrinterDevice*& r_device) {return PRINTER_RESULT_ERROR;}
	MCPrinterResult DoEndPrint(MCPrinterDevice* p_device) {return PRINTER_RESULT_ERROR;}
};

typedef struct
{
	uint1 red;
	uint1 green;
	uint1 blue;
}
SCCLUT;

static SCCLUT sccolors[] =
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

static uint4 stdcmap[256] =
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

MCMovingList::~MCMovingList()
{
	delete pts;
}

MCUIDC::MCUIDC()
{
	messageid = 0;
	nmessages = maxmessages = 0;
	messages = NULL;
	moving = NULL;
	ncolors = 0;
	colors = NULL;
	allocs = NULL;
	colornames = NULL;
	lockmods = False;

	m_sound_internal = NULL ;
}

MCUIDC::~MCUIDC()
{
	while (nmessages != 0)
		cancelmessageindex(0, True);
	delete messages;
}


bool MCUIDC::setbeepsound ( const char * p_internal) 
{
	if ( MCString(p_internal) == "internal" )
	{
		m_sound_internal = "internal";
		return true ;
	}
	
	if ( MCString(p_internal) == "system")
	{
		m_sound_internal = "system" ;
		return true ;
	}
	return false ;
}

const char * MCUIDC::getbeepsound(void)
{
	if ( m_sound_internal == NULL )
		m_sound_internal = "system" ;
	return m_sound_internal;
}


bool MCUIDC::hasfeature(MCPlatformFeature p_feature)
{
	return false;
}

void MCUIDC::setstatus(const char *status)
{ }
Boolean MCUIDC::open()
{
	return True;
}
Boolean MCUIDC::close(Boolean force)
{
	return True;
}
const char *MCUIDC::getdisplayname()
{
	return "";
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

uint2 MCUIDC::getwidth()
{
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	return ceil(device_getwidth() / t_scale) ;
}

uint2 MCUIDC::getheight()
{
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	return ceil(device_getheight() / t_scale);
}

//////////

uint16_t MCUIDC::device_getwidth()
{
	return 1;
}

uint16_t MCUIDC::device_getheight()
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

uint4 MCUIDC::getdisplays(const MCDisplay*& p_rectangles, bool p_effective)
{
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	if (!device_getdisplays(p_effective, t_displays, t_count))
		return 0;
	
	for (uint32_t i = 0; i < t_count; i++)
	{
		t_displays[i].viewport = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_displays[i].device_viewport));
		t_displays[i].workarea = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_displays[i].device_workarea));
	}
	
	p_rectangles = t_displays;
	
	return t_count;
}

//////////

bool MCUIDC::device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

const MCDisplay *MCUIDC::getnearestdisplay(const MCRectangle& p_rectangle)
{
	MCRectangle t_device_rect;
	t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(p_rectangle));
	
	return device_getnearestdisplay(t_device_rect);
}

//////////

const MCDisplay *MCUIDC::device_getnearestdisplay(const MCRectangle& p_rectangle)
{
	MCDisplay const *t_displays;
	uint4 t_display_count;
	uint4 t_home;
	uint4 t_max_area, t_max_distance;
	uint4 t_max_area_index, t_max_distance_index;

	t_display_count = MCscreen -> getdisplays(t_displays, false);

	t_max_area = 0;
	t_max_distance = MAXUINT4;
	for(uint4 t_display = 0; t_display < t_display_count; ++t_display)
	{
		MCRectangle t_workarea;
		t_workarea = t_displays[t_display] . device_workarea;
		
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

Boolean MCUIDC::getwindowgeometry(Window p_window, MCRectangle &r_rect)
{
	MCRectangle t_rect;
	if (!device_getwindowgeometry(p_window, t_rect))
		return False;
	
	r_rect = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_rect));
	return True;
}

//////////

bool MCUIDC::device_getwindowgeometry(Window p_window, MCRectangle &r_rect)
{
	r_rect = MCU_make_rect(0, 0, 32, 32);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::boundrect(MCRectangle &x_rect, Boolean p_title, Window_mode p_mode)
{
	MCRectangle t_rect;
	t_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(x_rect));
	
	MCscreen->device_boundrect(t_rect, p_title, p_mode);
	
	x_rect = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_rect));
}

//////////

void MCUIDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode m)
{ }

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::querymouse(int2 &x, int2 &y)
{
	int16_t t_x, t_y;
	device_querymouse(t_x, t_y);
	
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	x = t_x / t_scale;
	y = t_y / t_scale;
}

//////////

void MCUIDC::device_querymouse(int2 &x, int2 &y)
{ }

////////////////////////////////////////////////////////////////////////////////

void MCUIDC::setmouse(int2 x, int2 y)
{
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	device_setmouse(x * t_scale, y * t_scale);
}

//////////

void MCUIDC::device_setmouse(int2 x, int2 y)
{ }

////////////////////////////////////////////////////////////////////////////////

MCStack *MCUIDC::getstackatpoint(int32_t x, int32_t y)
{
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();

	return device_getstackatpoint(x * t_scale, y * t_scale);
}

//////////

MCStack *MCUIDC::device_getstackatpoint(int32_t x, int32_t y)
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
void MCUIDC::setname(Window window, const char *newname)
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

int4 MCUIDC::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return 0;
}

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

uint4 MCUIDC::dtouint4(Drawable d)
{
	return 1;
}


Boolean MCUIDC::uint4towindow(uint4, Window &w)
{
	w = (Window)1;
	return True;
}

void MCUIDC::getbeep(uint4 property, MCExecPoint &ep)
{
	ep.clear();
}

void MCUIDC::setbeep(uint4 property, int4 beep)
{ }
void MCUIDC::getvendorstring(MCExecPoint &ep)
{
	ep.clear();
}

uint2 MCUIDC::getpad()
{
	return 32;
}

Window MCUIDC::getroot()
{
	return (Window)1;
}

MCImageBitmap *MCUIDC::snapshot(MCRectangle &r, MCGFloat p_scale_factor, uint4 window,
                           const char *displayname)
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
	if (n < 256)
		c.pixel = stdcmap[n];
	else
		c.pixel = 0;
	querycolor(c);
}

void MCUIDC::alloccolor(MCColor &color)
{
	color.pixel = MCGPixelPackNative(
							   color.red >> 8,
							   color.green >> 8,
							   color.blue >> 8,
							   255);
}

void MCUIDC::querycolor(MCColor &color)
{
	uint8_t t_r, t_g, t_b, t_a;
	MCGPixelUnpackNative(color.pixel, t_r, t_g, t_b, t_a);
	color.red = t_r;
	color.green = t_g;
	color.blue = t_b;
	color.red |= color.red << 8;
	color.green |= color.green << 8;
	color.blue |= color.blue << 8;
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

void MCUIDC::delaymessage(MCObject *optr, MCNameRef mptr, char *p1, char *p2)
{
	if (nmessages == maxmessages)
	{
		maxmessages++;
		MCU_realloc((char **)&messages, nmessages, maxmessages, sizeof(MCMessageList));
	}
	messages[nmessages].object = optr;
	/* UNCHECKED */ MCNameClone(mptr, messages[nmessages].message);
	messages[nmessages].time = MCS_time();
	messages[nmessages].id = ++messageid;
	MCParameter *params = NULL;
	if (p1 != NULL)
	{
		params = new MCParameter;
		params->setbuffer(p1, strlen(p1));
		if (p2 != NULL)
		{
			params->setnext(new MCParameter);
			params->getnext()->setbuffer(p2, strlen(p2));
		}
	}
	messages[nmessages++].params = params;
}

void MCUIDC::addmessage(MCObject *optr, MCNameRef mptr, real8 time, MCParameter *params)
{
	if (nmessages == maxmessages)
	{
		maxmessages++;
		MCU_realloc((char **)&messages, nmessages,
		            maxmessages, sizeof(MCMessageList));
	}
	messages[nmessages].object = optr;
	/* UNCHECKED */ MCNameClone(mptr, messages[nmessages].message);
	messages[nmessages].time = time;
	messages[nmessages].id = ++messageid;
	char buffer[U4L];
	sprintf(buffer, "%u", messages[nmessages].id);
	MCresult->copysvalue(buffer);
	messages[nmessages++].params = params;
}

Boolean MCUIDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
	real8 curtime = MCS_time();
	if (duration < 0.0)
		duration = 0.0;
	real8 exittime = curtime + duration;
	Boolean done = False;
	Boolean donepending = False;
	do
	{
		real8 eventtime = exittime;
		donepending = handlepending(curtime, eventtime, dispatch);
		siguser();
		if (MCquit)
			return True;
		if (curtime < eventtime)
		{
			done = MCS_poll(donepending ? 0 : eventtime - curtime, 0);
			curtime = MCS_time();
		}
	}
	while (curtime < exittime  && !(anyevent && (done || donepending)));
	return False;
}

void MCUIDC::pingwait(void)
{
#ifdef _DESKTOP
	// MW-2013-06-14: [[ DesktopPingWait ]] Use the notify mechanism to wake up
	//   any running wait.
	MCNotifyPing(false);
#endif
}

void MCUIDC::flushevents(uint2 e)
{ }
;

Boolean MCUIDC::istripleclick()
{
	return False;
}

void MCUIDC::getkeysdown(MCExecPoint &ep)
{
	ep.clear();
}

char *MCUIDC::charsettofontname(uint1 chharset, const char *oldfontname)
{
#ifdef TARGET_PLATFORM_LINUX
	const char *t_charset;
	t_charset = strchr(oldfontname, ',');
	if (t_charset != NULL)
		return strndup(oldfontname, t_charset - oldfontname);
#endif
	return strclone(oldfontname);
}

uint1 MCUIDC::fontnametocharset(const char *oldfontname)
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

void MCUIDC::updatemenubar(Boolean force)
{
	if (MCdefaultmenubar == NULL)
		MCdefaultmenubar = MCmenubar;

	MCGroup *newMenuGroup;
	if (MCmenubar != NULL)
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
};

void MCUIDC::addtimer(MCObject *optr, MCNameRef mptr, uint4 delay)
{
	uint2 i;
	for (i = 0 ; i < nmessages ; i++)
		if (messages[i].object == optr && MCNameIsEqualTo(messages[i].message, mptr, kMCCompareCaseless))
		{
			messages[i].time = MCS_time() + delay / 1000.0;
			return;
		}
	if (nmessages == maxmessages)
	{
		maxmessages++;
		MCU_realloc((char **)&messages, nmessages,
		            maxmessages, sizeof(MCMessageList));
	}
	messages[nmessages].object = optr;
	/* UNCHECKED */ MCNameClone(mptr, messages[nmessages].message);
	messages[nmessages].time = MCS_time() + delay / 1000.0;
	messages[nmessages].id = 0;
	messages[nmessages++].params = NULL;
}

void MCUIDC::cancelmessageindex(uint2 i, Boolean dodelete)
{
	if (dodelete)
	{
		while (messages[i].params != NULL)
		{
			MCParameter *tmp = messages[i].params;
			messages[i].params = messages[i].params->getnext();
			delete tmp;
		}
		MCNameDelete(messages[i] . message);
	}
	nmessages--;
	while (i++ < nmessages)
		messages[i - 1] = messages[i];
}

void MCUIDC::cancelmessageid(uint4 id)
{
	uint2 i;
	for (i = 0 ; i < nmessages ; i++)
		if (messages[i].id == id)
		{
			cancelmessageindex(i, True);
			return;
		}
}

void MCUIDC::cancelmessageobject(MCObject *optr, MCNameRef mptr)
{
	uint2 i;
	for (i = 0 ; i < nmessages ; i++)
		if (messages[i].object == optr
		        && (mptr == NULL || MCNameIsEqualTo(messages[i].message, mptr, kMCCompareCaseless)))
			cancelmessageindex(i--, True);
}

void MCUIDC::listmessages(MCExecPoint &ep)
{
	uint2 i;
	MCExecPoint ep1(ep);
	bool first;
	first = true;
	ep.clear();
	for (i = 0 ; i < nmessages ; i++)
	{
		if (messages[i].id != 0)
		{
			ep.concatuint(messages[i].id, EC_RETURN, first);
			ep.concatreal(messages[i].time, EC_COMMA, false);
			ep.concatnameref(messages[i].message, EC_COMMA, false);
			messages[i].object->getprop(0, P_LONG_ID, ep1, false);
			ep.concatmcstring(ep1.getsvalue(), EC_COMMA, false);
			first = false;
		}
	}
}

Boolean MCUIDC::handlepending(real8 &curtime, real8 &eventtime,
                              Boolean dispatch)
{
	Boolean doneone = False;
	uint2 mdone = 0;
	while (nmessages > mdone)
	{
		int2 minindex = -1;
		uint2 i;
		for (i = 0 ; i < nmessages ; i++)
			if ((dispatch || messages[i].id == 0)
			        && (minindex == -1 || messages[i].time < messages[minindex].time))
				minindex = i;
		if (minindex == -1)
			break;
		if (curtime < messages[minindex].time)
		{
			if (eventtime > messages[minindex].time
			        && (dispatch || messages[minindex].id == 0))
				eventtime = messages[minindex].time;
			break;
		}
		if (dispatch || messages[minindex].id == 0)
		{
			mdone++;
			if (!dispatch && MCNameIsEqualTo(messages[minindex].message, MCM_idle, kMCCompareCaseless))
				messages[minindex].time = curtime + ((real8)MCidleRate / 1000.0);
			else
			{
				doneone = True;
				MCParameter *p = messages[minindex].params;
				MCNameRef m = messages[minindex].message;
				MCObject *o = messages[minindex].object;
				cancelmessageindex(minindex, False);
				MCSaveprops sp;
				MCU_saveprops(sp);
				MCU_resetprops(False);
				o->timer(m, p);
				MCU_restoreprops(sp);
				while (p != NULL)
				{
					MCParameter *tmp = p;
					p = p->getnext();
					delete tmp;
				}
				MCNameDelete(m);
				curtime = MCS_time();
			}
		}
	}
	if (moving != NULL)
		handlemoves(curtime, eventtime);
	real8 stime = IO_cleansockets(curtime);
	if (stime < eventtime)
		eventtime = stime;
	// MW-2012-08-27: [[ Bug ]] Make sure the 'eventtime' is correct - it should be the
	//   time of the next message to process in the queue.
	if (doneone)
	{
		for(uint32_t i = 0; i < nmessages; i++)
			if (eventtime > messages[i] . time)
				eventtime = messages[i] . time;
	}
	return doneone;
}

void MCUIDC::addmove(MCObject *optr, MCPoint *pts, uint2 npts,
                     real8 &duration, Boolean waiting)
{
	stopmove(optr, False);
	MCMovingList *mptr = new MCMovingList;
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

	mptr->starttime = MCS_time();
}

void MCUIDC::listmoves(MCExecPoint &ep)
{
	ep.clear();
	if (moving != NULL)
	{
		MCExecPoint ep2(ep);
		MCMovingList *mptr = moving;
		do
		{
			mptr->object->getprop(0, P_LONG_ID, ep2, False);
			ep.concatmcstring(ep2.getsvalue(), EC_RETURN, mptr == moving);
			mptr = mptr->next();
		}
		while (mptr != moving);
	}
}

void MCUIDC::stopmove(MCObject *optr, Boolean finish)
{
	if (moving != NULL)
	{
		MCMovingList *mptr = moving;
		do
		{
			if (mptr->object == optr)
			{
				mptr->remove
				(moving);
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
							static_cast<MCControl *>(mptr->object)->layer_setrect(newrect, false);
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
	static real8 lasttime;
	eventtime = curtime + (real8)MCsyncrate / 1000.0;
	MCMovingList *mptr = moving;
	Boolean moved = False;
	Boolean done = False;
	do
	{
		if (MClockmoves)
		{
			if (lasttime != 0.0)
				mptr->starttime += curtime - lasttime;
			mptr = mptr->next();
		}
		else
		{
			MCRectangle rect = mptr->object->getrect();
			MCRectangle newrect = rect;
			real8 dt = 0.0;
			if (curtime >= mptr->starttime + mptr->duration
			        || rect.x == mptr->donex && rect.y == mptr->doney)
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
					static_cast<MCControl *>(mptr->object)->layer_setrect(newrect, false);
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
						if (MClockmessages)
							delaymessage(mptr->object, MCM_move_stopped);
						else
							mptr->object->message(MCM_move_stopped);
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
	}
	while (mptr != NULL && mptr != moving);
	if (MClockmoves)
		lasttime = curtime;
	else
		lasttime = 0.0;
		
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
		MCdefaultstackptr->getcurcard()->message_with_args(MCM_signal, "1");
	}
	while (MCsiguser2)
	{
		MCsiguser2--;
		MCdefaultstackptr->getcurcard()->message_with_args(MCM_signal, "2");
	}
}

#include "rgb.cpp"

Boolean MCUIDC::lookupcolor(const MCString &s, MCColor *color)
{
	uint4 slength = s.getlength();
	char *startptr = new char[slength + 1];
	char *sptr = startptr;
	MCU_lower(sptr, s);
	sptr[slength] = '\0';
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
					{
						delete startptr;
						return False;
					}
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
		color->flags = DoRed | DoGreen | DoBlue;
		delete startptr;
		return True;
	}
	char *tptr = sptr;
	while (*tptr)
		if (isspace((uint1)*tptr))
			strcpy(tptr, tptr + 1);
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
				color->flags = DoRed | DoGreen | DoBlue;
				delete startptr;
				return True;
			}
	}
	delete startptr;
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
	
	MCImageBitmap *image = snapshot(t_rect, 1.0, 0, nil);

	// If fetching the mouse pixel fails, then just return black.
	if (image == NULL)
	{
		*cptr = MCscreen -> getblack();
		return;
	}

	newcolor.pixel = MCImageBitmapGetPixel(image, 0, 0);
	MCImageFreeBitmap(image);
	querycolor(newcolor);
	if (cptr != NULL)
		*cptr = newcolor;
	else
	{
		alloccolor(newcolor);
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

Boolean MCUIDC::parsecolor(const MCString &s, MCColor *color, char **cname)
{
	if (cname)
	{
		delete *cname;
		*cname = NULL;
	}
	
	int2 i1, i2, i3;
	Boolean done;
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	
	// check for numeric first argument
	i1 = MCU_strtol(sptr, l, ',', done);
	if (!done)
	{
		// not numeric, check against the colornames
		if (lookupcolor(s, color))
		{
			if (cname)
				*cname = s.clone();
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
	color->red = (uint2)(i1 << 8) + i1;
	color->green = (uint2)(i2 << 8) + i2;
	color->blue = (uint2)(i3 << 8) + i3;
	
	return True;
}



Boolean MCUIDC::parsecolors(const MCString &s, MCColor *colors,
                            char *cnames[], uint2 ncolors)
{
	uint2 offset = 0;
	char *data = s.clone();
	char *sptr = data;

	while (*sptr && offset < ncolors)
	{
		char *tptr;
		if ((tptr = strchr(sptr, '\n')) != NULL)
			*tptr++ = '\0';
		else
			tptr = &sptr[strlen(sptr)];
		if (strlen(sptr) != 0)
		{
			if (!parsecolor(sptr, &colors[offset], &cnames[offset]))
			{
				while (offset--)
					if (cnames[offset] != NULL)
						delete cnames[offset];
				delete data;
				return False;
			}
			colors[offset].flags = DoRed | DoGreen | DoBlue;
		}
		else
		{
			colors[offset].flags = 0;
			cnames[offset] = NULL;
		}
		sptr = tptr;
		offset++;
	}
	while (offset < ncolors)
	{
		colors[offset].flags = 0;
		cnames[offset] = NULL;
		offset++;
	}
	delete data;
	return True;
}

Boolean MCUIDC::getcolors(MCExecPoint &ep)
{
		ep.setstaticcstring("fixed");
		return True;
	}

Boolean MCUIDC::setcolors(const MCString &values)
{
		return False;
		}

void MCUIDC::getcolornames(MCExecPoint &ep)
{
	ep.getbuffer(NAME_LENGTH);
	ep.clear();
	uint2 end = ELEMENTS(color_table);
	uint2 i;
	for (i = 0 ; i < end ; i++)
		ep.concatcstring(color_table[i].token, EC_RETURN, i == 0);
}

void MCUIDC::seticon(uint4 p_icon)
{
}

void MCUIDC::seticonmenu(const char *p_menu)
{
}

void MCUIDC::configurestatusicon(uint32_t icon_id, const char *menu, const char *tooltip)
{
}

MCPrinter *MCUIDC::createprinter(void)
{
	return new MCNullPrinter;
}

void MCUIDC::listprinters(MCExecPoint& ep)
{
	ep . clear();
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

bool MCUIDC::ownsselection(void)
{
	return false;
}

MCPasteboard *MCUIDC::getselection(void)
{
	return NULL;
}

bool MCUIDC::setselection(MCPasteboard *p_pasteboard)
{
	return false;
}

void MCUIDC::flushclipboard(void)
{
}

bool MCUIDC::ownsclipboard(void)
{
	return false;
}

MCPasteboard *MCUIDC::getclipboard(void)
{
	return NULL;
}

bool MCUIDC::setclipboard(MCPasteboard *p_pasteboard)
{
	return false;
}


// TD-2013-07-01: [[ DynamicFonts ]]
bool MCUIDC::loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle)
{
	return false;
}

bool MCUIDC::unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle)
{
	return false;
}

//

MCDragAction MCUIDC::dodragdrop(MCPasteboard* p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint *p_image_offset)
{
	return DRAG_ACTION_NONE;
}

//

MCScriptEnvironment *MCUIDC::createscriptenvironment(const char *p_language)
{
	return NULL;
}

void MCUIDC::enactraisewindows(void)
{

}
//

int32_t MCUIDC::popupanswerdialog(const char **p_buttons, uint32_t p_button_count, uint32_t p_type, const char *p_title, const char *p_message)
{
	return 0;
}

char *MCUIDC::popupaskdialog(uint32_t p_type, const char *p_title, const char *p_message, const char *p_initial, bool p_hint)
{
	return nil;
}
