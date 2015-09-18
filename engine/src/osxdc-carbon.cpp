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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
#include "execpt.h"
#include "player.h"
#include "group.h"
#include "globals.h"
#include "mode.h"
#include "eventqueue.h"
#include "osspec.h"
#include "redraw.h"

#include "osxdc.h"
#include "osxprinter.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release = true);

////////////////////////////////////////////////////////////////////////////////

WindowPtr MCScreenDC::getinvisiblewin(void)
{
	if (invisibleWin == nil)
	{
		Rect invisibleWinRect;
		SetRect(&invisibleWinRect, 0, 0, 20, 20);
		invisibleWin = NewCWindow(nil, &invisibleWinRect, "\p", False,
								  kUtilityWindowClass, (WindowRef)(-1L), False, 0);
	}
	return invisibleWin;
}

////////////////////////////////////////////////////////////////////////////////

MCDisplay *MCScreenDC::s_monitor_displays = NULL;
uint4 MCScreenDC::s_monitor_count = 0;

bool MCScreenDC::device_getdisplays(bool p_effective, MCDisplay *& p_displays, uint32_t &r_count)
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
	
	p_displays = t_displays;
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

////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-01 [[ DynamicFonts ]]
bool MCScreenDC::loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle)
{	
	FSRef t_ref;
    FSSpec t_fsspec;
	OSErr t_os_error;
	
	t_os_error = MCS_pathtoref(p_path, &t_ref); // resolves and converts to UTF8.
	if (t_os_error != noErr)
		return false;
    
    t_os_error = MCS_fsref_to_fsspec(&t_ref, &t_fsspec);
    if (t_os_error != noErr)
		return false;
    
    ATSFontContext t_context = kATSFontContextLocal;
    if (p_globally)
        t_context = kATSFontContextGlobal;
    
    ATSFontContainerRef t_container = NULL;
    
    // Note: ATSFontActivateFromFileReference should be used from 10.5 onward.
    //       ATSFontActivateFromFileSpecification deprecated in 10.5.
    t_os_error = ATSFontActivateFromFileSpecification(&t_fsspec, t_context, kATSFontFormatUnspecified, NULL, kATSOptionFlagsDefault, &t_container);
    if (t_os_error != noErr)
        return false;
    
    r_loaded_font_handle = (void *)t_container;
    
    return true;
}

bool MCScreenDC::unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle)
{
    OSStatus t_status;
    t_status = ATSFontDeactivate((ATSFontContainerRef)r_loaded_font_handle, NULL, kATSOptionFlagsDefault);
    
    return (t_status == noErr);
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::listprinters(MCExecPoint& ep)
{
	ep . clear();
	
	CFArrayRef t_printers;
	if (PMServerCreatePrinterList(kPMServerLocal, &t_printers) != noErr)
		return;
	
	for(CFIndex i = 0; i < CFArrayGetCount(t_printers); ++i)
	{
		char *t_name;
		t_name = osx_cfstring_to_cstring(PMPrinterGetName((PMPrinter)CFArrayGetValueAtIndex(t_printers, i)), false);
		ep . concatcstring(t_name, EC_RETURN, i == 0);
		free(t_name);
	}
	
	CFRelease(t_printers);
}

MCPrinter *MCScreenDC::createprinter(void)
{
	return new MCMacOSXPrinter;
}

////////////////////////////////////////////////////////////////////////////////

static MCColor accentcolors[8];

MCColor* MCScreenDC::getaccentcolors()
{
	
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

// MW-2007-8-29: [[ Bug 4691 ]] Make sure the working screenrect is adjusted appropriately when we show/hide the menubar
void MCScreenDC::hidemenu()
{
	HideMenuBar();
	menubarhidden = true ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}

void MCScreenDC::showmenu()
{
	ShowMenuBar();
	menubarhidden = false ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
