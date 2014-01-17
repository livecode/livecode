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

#include "platform.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "debug.h"
#include "scrolbar.h"
#include "button.h"
#include "resolution.h"
#include "redraw.h"
#include "notify.h"

#include "desktop-dc.h"

#include "platform.h"

#include <Carbon/Carbon.h>

////////////////////////////////////////////////////////////////////////////////

Boolean tripleclick = False;

MCDisplay *MCScreenDC::s_monitor_displays = nil;
uint4 MCScreenDC::s_monitor_count = 0;

////////////////////////////////////////////////////////////////////////////////

MCScreenDC::MCScreenDC(void)
{
	MCNotifyInitialize();
}

MCScreenDC::~MCScreenDC(void)
{
}

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	switch(p_feature)
	{
		case PLATFORM_FEATURE_WINDOW_TRANSPARENCY:
		case PLATFORM_FEATURE_OS_COLOR_DIALOGS:
		case PLATFORM_FEATURE_OS_FILE_DIALOGS:
		case PLATFORM_FEATURE_OS_PRINT_DIALOGS:
			return true;
			break;
			
		case PLATFORM_FEATURE_TRANSIENT_SELECTION:
			return false;
			break;
			
		default:
			assert(false);
			break;
	}
	
	return false;
}

Boolean MCScreenDC::open()
{
	black_pixel.red = black_pixel.green = black_pixel.blue = 0; //black pixel
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF; //white pixel
	black_pixel.pixel = 0;
	white_pixel.pixel = 0xFFFFFF;
	
	MCzerocolor = MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8888;
	alloccolor(gray_pixel);
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xffff;
	alloccolor(background_pixel);

	MCPlatformGetSystemProperty(kMCPlatformSystemPropertyHiliteColor, kMCPlatformPropertyTypeColor, &MChilitecolor);
	alloccolor(MChilitecolor);
	
	MCPlatformGetSystemProperty(kMCPlatformSystemPropertyAccentColor, kMCPlatformPropertyTypeColor, &MCaccentcolor);
	alloccolor(MCaccentcolor);
	
	MCPlatformGetSystemProperty(kMCPlatformSystemPropertyDoubleClickInterval, kMCPlatformPropertyTypeDouble, &MCdoubletime);
	
	MCtemplatescrollbar->alloccolors();
	MCtemplatebutton->allocicons();
	
	MCcursors[PI_NONE] = nil;
	
	MCPlatformGetSystemProperty(kMCPlatformSystemPropertyCaretBlinkInterval, kMCPlatformPropertyTypeDouble, &MCblinkrate);
			
	MCDisplay const *t_displays;
	getdisplays(t_displays, false);
	MCwbr = t_displays[0] . workarea;
	
	// COCOA-TODO: menus / textinput / dragdrop / iconmenu / colormapping
	
	return True;
}

Boolean MCScreenDC::close(Boolean force)
{
	// COCOA-TODO: Is this still needed?
	uint2 i;
	if (ncolors != 0)
	{
		int2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
			if (colornames[i] != NULL)
				delete colornames[i];
		}
		delete colors;
		delete colornames;
		delete allocs;
	}
	
	return True;
}

const char *MCScreenDC::getdisplayname()
{
	return "local Mac";
}

uint2 MCScreenDC::getmaxpoints(void)
{
	return 32767;
}

uint2 MCScreenDC::getvclass(void)
{
	return DirectColor;
}

uint2 MCScreenDC::getdepth(void)
{
	return 32;
}

void MCScreenDC::getvendorstring(MCExecPoint &ep)
{
	ep.setstaticcstring("Mac OS");
}

uint2 MCScreenDC::getpad()
{
	return 32;
}

MCColor *MCScreenDC::getaccentcolors()
{
	static MCColor accentcolors[8];
	SInt32 response;
	if (Gestalt(gestaltAppearanceVersion, &response) == noErr)
	{ // 1.0.1 or later
		// theme drawing only supported in version 1.1, OS 8.5 and later
		RegisterAppearanceClient(); // no harm in calling this multiple times
		if (response < 0x0110)
			MClook = LF_MAC;
		else
		{
			ThemeScrollBarThumbStyle tstyle;
			GetThemeScrollBarThumbStyle(&tstyle);
			MCproportionalthumbs = tstyle == kThemeScrollBarThumbProportional;
		}
		//get the 8 accent colors, for drawing scroll bar
		CTabHandle themeCT; //handle to the Theme color table
		if (GetThemeAccentColors(&themeCT) == noErr)
		{
			uint2 i = MCU_min(8, (*themeCT)->ctSize + 1);
			while (i--)
			{
				accentcolors[i].red = (*themeCT)->ctTable[i].rgb.red;
				accentcolors[i].green = (*themeCT)->ctTable[i].rgb.green;
				accentcolors[i].blue = (*themeCT)->ctTable[i].rgb.blue;
			}
			return accentcolors;
		}
		return NULL;
	}
	
	MClook = LF_MAC;
	return NULL;
}

// IM-2013-08-01: [[ ResIndependence ]] refactored methods that return device coordinates
uint16_t MCScreenDC::device_getwidth(void)
{
	GDHandle mainScreen = GetMainDevice();
	HLock((Handle)mainScreen);
	uint2 swidth = ((GDPtr)*mainScreen)->gdRect.right
	- ((GDPtr)*mainScreen)->gdRect.left;
	HUnlock((Handle)mainScreen);
	return swidth;
}

uint16_t MCScreenDC::device_getheight(void)
{
	GDHandle mainScreen = GetMainDevice();
	HLock((Handle)mainScreen);
	uint2 sheight = ((GDPtr)*mainScreen)->gdRect.bottom
	- ((GDPtr)*mainScreen)->gdRect.top;
	HUnlock((Handle)mainScreen);
	return sheight;
}

bool MCScreenDC::device_getdisplays(bool p_effective, MCDisplay *& r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_display_count;
	t_display_count = 0;
	
	if (s_monitor_count != 0)
	{
		t_displays = s_monitor_displays;
		t_display_count = s_monitor_count;
	}
	else
	{
		for(GDHandle t_device = GetDeviceList(); t_device != NULL; t_device = GetNextDevice(t_device))
			if (TestDeviceAttribute(t_device, screenDevice) && TestDeviceAttribute(t_device, screenActive))
				t_display_count += 1;
		
		t_success = t_display_count != 0;
		
		if (t_success)
			t_success = MCMemoryNewArray(t_display_count, t_displays);
		
		if (t_success)
		{
			uint4 t_current_index = 1;
			for(GDHandle t_device = GetDeviceList(); t_device != NULL; t_device = GetNextDevice(t_device))
				if (TestDeviceAttribute(t_device, screenDevice) && TestDeviceAttribute(t_device, screenActive))
				{
					uint4 t_index;
					
					HLock((Handle)t_device);
					
					if (TestDeviceAttribute(t_device, mainScreen))
						t_index = 0;
					else
						t_index = t_current_index++;
					
					t_displays[t_index] . index = t_index;
					
					t_displays[t_index] . device_viewport . x = (*t_device) -> gdRect . left;
					t_displays[t_index] . device_viewport . y = (*t_device) -> gdRect . top;
					t_displays[t_index] . device_viewport . width = (*t_device) -> gdRect . right - (*t_device) -> gdRect . left;
					t_displays[t_index] . device_viewport . height = (*t_device) -> gdRect . bottom - (*t_device) -> gdRect . top;
					
					Rect t_workarea;
					GetAvailableWindowPositioningBounds(t_device, &t_workarea);
					t_displays[t_index] . device_workarea . x = t_workarea . left;
					t_displays[t_index] . device_workarea . y = t_workarea . top;
					t_displays[t_index] . device_workarea . width = t_workarea . right - t_workarea . left;
					t_displays[t_index] . device_workarea . height = t_workarea . bottom - t_workarea . top;
					
					HUnlock((Handle)t_device);
				}
		}
		
		if (t_success)
		{
			s_monitor_count = t_display_count;
			s_monitor_displays = t_displays;
		}
		else
			MCMemoryDeleteArray(t_displays);
	}
	
	if (!t_success)
	{
		static MCDisplay t_display;
		Rect t_workarea;
		
		MCU_set_rect(t_display . device_viewport, 0, 0, device_getwidth(), device_getheight());
		GetAvailableWindowPositioningBounds(GetMainDevice(), &t_workarea);
		t_display . index = 0;
		t_display . device_workarea . x = t_workarea . left;
		t_display . device_workarea . y = t_workarea . top;
		t_display . device_workarea . width = t_workarea . right - t_workarea . left;
		t_display . device_workarea . height = t_workarea . bottom - t_workarea . top;
		
		t_displays = &t_display;
		t_display_count = 1;
	}
	
	r_displays = t_displays;
	r_count = t_display_count;
	
	return true;
}

void MCScreenDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode mode)
{
	MCRectangle srect;
	
	if (mode >= WM_MODAL)
	{
		const MCDisplay *t_display;
		t_display = getnearestdisplay(rect);
		srect = t_display -> device_workarea;
	}
	else
		srect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(MCwbr));
	
	uint2 sr, sw, sb, sh;
	Rect screenRect;
	
	SetRect(&screenRect, srect . x, srect . y, srect . x + srect . width, srect . y + srect . height);
	
	if (title && mode <= WM_SHEET && mode != WM_DRAWER)
	{
		if (mode == WM_PALETTE)
			screenRect.top += 13;
		else
		{
			long osversion;
			Gestalt(gestaltSystemVersion, &osversion);
			if (osversion >= 0x00000800)
				screenRect.top += 22;
			else
				screenRect.top += 19;
		}
		sr = sb = 10;
		sw = 20;
		sh = 12;
	}
	else
		sr = sw = sb = sh = 0;
	
	if (rect.x < screenRect.left)
		rect.x = screenRect.left;
	if (rect.x + rect.width > screenRect.right - sr)
	{
		if (rect.width > screenRect.right - screenRect.left - sw)
			rect.width = screenRect.right - screenRect.left - sw;
		rect.x = screenRect.right - rect.width - sr;
	}
	
	if (rect.y < screenRect.top)
		rect.y = screenRect.top;
	if (rect.y + rect.height > screenRect.bottom - sb)
	{
		if (rect.height > screenRect.bottom - screenRect.top - sh)
			rect.height = screenRect.bottom - screenRect.top - sh;
		rect.y = screenRect.bottom - rect.height - sb;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::resetcursors()
{
}

void MCScreenDC::setcursor(Window w, MCCursorRef c)
{
}

MCCursorRef MCScreenDC::createcursor(MCImageBitmap *image, int2 xhot, int2 yhot)
{
	return nil;
}

void MCScreenDC::freecursor(MCCursorRef c)
{
}

////////////////////////////////////////////////////////////////////////////////

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{
	if (w == nil)
		return false;
	MCPlatformGetWindowContentRect(w, drect);
	return true;
}

void MCScreenDC::openwindow(Window w, Boolean override)
{
	MCPlatformShowWindow(w);
}

void MCScreenDC::closewindow(Window window)
{
	MCPlatformHideWindow(window);
}

void MCScreenDC::destroywindow(Window &window)
{
	MCPlatformReleaseWindow(window);
	window = nil;
}

void MCScreenDC::raisewindow(Window window)
{
	MCPlatformRaiseWindow(window);
}

void MCScreenDC::iconifywindow(Window window)
{
	MCPlatformIconifyWindow(window);
}

void MCScreenDC::uniconifywindow(Window window)
{
	MCPlatformUniconifyWindow(window);
}

void MCScreenDC::setname(Window window, const char *newname)
{
	MCPlatformSetWindowProperty(window, kMCPlatformWindowPropertyTitle, kMCPlatformPropertyTypeUTF8CString, &newname);
}

void MCScreenDC::setinputfocus(Window window)
{
	MCPlatformFocusWindow(window);
}

uint4 MCScreenDC::dtouint4(Drawable d)
{
	return 0;
}

Boolean MCScreenDC::uint4towindow(uint4, Window &w)
{
	return False;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::enablebackdrop(bool p_hard)
{
}

void MCScreenDC::disablebackdrop(bool p_hard)
{
}

void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge)
{
}

void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::grabpointer(Window w)
{
	MCPlatformGrabPointer(w);
}

void MCScreenDC::ungrabpointer()
{
	MCPlatformUngrabPointer();
}

void MCScreenDC::device_querymouse(int2 &x, int2 &y)
{
	MCPoint t_location;
	MCPlatformGetMousePosition(t_location);
	x = t_location . x;
	y = t_location . y;
}

void MCScreenDC::device_setmouse(int2 x, int2 y)
{
	MCPoint t_location;
	t_location . x = x;
	t_location . y = y;
	MCPlatformSetMousePosition(t_location);
}

MCStack *MCScreenDC::device_getstackatpoint(int32_t x, int32_t y)
{
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

MCColorTransformRef MCScreenDC::createcolortransform(const MCColorSpaceInfo& info)
{
	return nil;
}

void MCScreenDC::destroycolortransform(MCColorTransformRef transform)
{
}

bool MCScreenDC::transformimagecolors(MCColorTransformRef transform, MCImageBitmap *image)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::beep()
{
	SndSetSysBeepState(sysBeepEnable | sysBeepSynchronous);
	SysBeep(beepduration / 16);
}

void MCScreenDC::getbeep(uint4 which, MCExecPoint &ep)
{
	long v;
	switch (which)
	{
		case P_BEEP_LOUDNESS:
			GetSysBeepVolume(&v);
			ep.setint(v);
			break;
		case P_BEEP_PITCH:
			ep.setint(beeppitch);
			break;
		case P_BEEP_DURATION:
			ep.setint(beepduration);
			break;
	}
}

void MCScreenDC::setbeep(uint4 which, int4 beep)
{
	switch (which)
	{
		case P_BEEP_LOUDNESS:
			SetSysBeepVolume(beep);
			break;
		case P_BEEP_PITCH:
			if (beep == -1)
				beeppitch = 1440;
			else
				beeppitch = beep;
			break;
		case P_BEEP_DURATION:
			if (beep == -1)
				beepduration = 500;
			else
				beepduration = beep;
			break;
	}
}	

////////////////////////////////////////////////////////////////////////////////

Boolean MCScreenDC::abortkey()
{
	return MCPlatformGetAbortKeyPressed();
}

uint2 MCScreenDC::querymods()
{
	if (lockmods)
		return MCmodifierstate;

	uint2 state;
	state = 0;
}

void MCScreenDC::getkeysdown(MCExecPoint &ep)
{
	// COCOA-TODO: We should be able to use CGEventSourceKeyState here - although
	//   it might be somewhat inefficient.
	ep.clear();
}

Boolean MCScreenDC::istripleclick()
{
	return tripleclick;
}

Boolean MCScreenDC::getmouse(uint2 button, Boolean& r_abort)
{
	// COCOA-TODO: See what the impact of no delay is...
	r_abort = False;
	return MCPlatformGetMouseButtonState(button);
}

Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	r_abort = False;
	return MCPlatformGetMouseClick(button);
	
#if PRE_PLATFORM
	// Collect all pending events in the event queue.
	r_abort = wait(0.0, False, False);
	if (r_abort)
		return False;
	
	// Now get the eventqueue to filter for a mouse click.
	return MCEventQueueGetMouseClick(button);
#endif
}

Boolean MCScreenDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
	real8 curtime = MCS_time();
	
	if (duration < 0.0)
		duration = 0.0;
	
	real8 exittime = curtime + duration;
	
	Boolean abort = False;
	Boolean reset = False;
	Boolean done = False;
	
	MCwaitdepth++;
	
	do
	{
		// Dispatch any notify events.
		if (MCNotifyDispatch(dispatch == True) && anyevent)
			break;
		
		// Handle pending events
		real8 eventtime = exittime;
		if (handlepending(curtime, eventtime, dispatch))
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}
		
		// If dispatching then dispatch the first event.
		/*if (dispatch && MCEventQueueDispatch())
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}*/
		
		// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
		//   any engine event handling methods need us to.
		MCRedrawUpdateScreen();
		
		// Get the time now
		curtime = MCS_time();
		
		// And work out how long to sleep for.
		real8 t_sleep;
		t_sleep = 0.0;
		if (curtime >= exittime)
			done = True;
		else if (!done && eventtime > curtime)
			t_sleep = MCMin(eventtime - curtime, exittime - curtime);
		
		// Wait for t_sleep seconds and collect at most one event. If an event
		// is collected and anyevent is True, then we are done.
		if (MCPlatformWaitForEvent(t_sleep, dispatch == False) && anyevent)
			done = True;
		
		// If 'quit' has been set then we must have got a finalization request
		if (MCquit)
		{
			abort = True;
			break;
		}
	}
	while(!done);
	
	MCwaitdepth--;
	
	// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
	//   any engine event handling methods need us to.
	MCRedrawUpdateScreen();
	
	return abort;
}

void MCScreenDC::flushevents(uint2 e)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::clearIME(Window w)
{
}

void MCScreenDC::openIME()
{
}

void MCScreenDC::activateIME(Boolean activate)
{
}

void MCScreenDC::closeIME()
{
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::listprinters(MCExecPoint& ep)
{
}

// COCOA-TODO: Printer creation.
MCPrinter *MCScreenDC::createprinter(void)
{
	return MCUIDC::createprinter();
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::flushclipboard(void)
{
}

bool MCScreenDC::ownsclipboard(void)
{
	return false;
}

bool MCScreenDC::setclipboard(MCPasteboard *p_pasteboard)
{
	return false;
}

MCPasteboard *MCScreenDC::getclipboard(void)
{
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-01: [[ DynamicFonts ]]
bool MCScreenDC::loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle)
{
	return false;
}

bool MCScreenDC::unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptEnvironment *MCScreenDC::createscriptenvironment(const char *p_language)
{
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

MCDragAction MCScreenDC::dodragdrop(MCPasteboard *p_pasteboard, MCDragActionSet p_allowed_actions, MCImage *p_image, const MCPoint* p_image_offset)
{
	return DRAG_ACTION_NONE;
}

////////////////////////////////////////////////////////////////////////////////

MCImageBitmap *MCScreenDC::snapshot(MCRectangle &r, MCGFloat p_scale_factor, uint4 window, const char *displayname)
{
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

double MCMacGetAnimationStartTime(void)
{
	return 0.0;
}

double MCMacGetAnimationCurrentTime(void)
{
	return 0.0;
}

WindowPtr MCMacGetInvisibleWindow(void)
{
	return nil;
}

void MCMacBreakWait(void)
{
	MCPlatformBreakWait();
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-01: [[ ResIndependence ]] OSX implementation currently returns 1.0
MCGFloat MCResGetDeviceScale(void)
{
	// COCOA-TODO: Integrate with platform
	return 1.0;
}

////////////////////////////////////////////////////////////////////////////////

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}

////////////////////////////////////////////////////////////////////////////////
