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

#include "osspec.h"
#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "globals.h"
//#include "execpt.h"
#include "notify.h"

#include "osxdc.h"
#include "osxprinter.h"
#include "securemode.h"
#include "mcerror.h"

#include "graphicscontext.h"
#include "graphics_util.h"

///////////////////////////////////////////////////////////////////////////////

extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release = true);

///////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Removed MCResGetSystemScale() function as part of HiDPI update
/* CODE REMOVED */

MCScreenDC::MCScreenDC()
{
	cursorhidden = false ;
	menubarhidden = false ;
	ncolors = 0;
	colors = NULL;
	colornames = NULL;
	lockcolormap = False;
	ownselection = False;
	messageid = 0;
	nmessages = maxmessages = 0;
	messages = NULL;
	beeppitch = 1440L;                    //1440 Hz
	beepduration = 500;                 // 1/2 second
	pendingevents = NULL;
	grabbed = False;
	mdown = False;
	
	backdrop_hard = false;
	backdrop_active = False;
	backdrop_window = NULL;
	backdrop_group = NULL;
	backdrop_background_group = NULL;
	backdrop_document_group = NULL;
	backdrop_palette_group = NULL;
	backdrop_pattern = NULL;
	backdrop_badge = NULL;
	backdrop_colour . red = 0;
	backdrop_colour . green = 0;
	backdrop_colour . blue = 0;
	backdrop_colour . pixel = 0;
	
	opened = 0;
	linesize = 0;
	menuBarHidden = False; //menu bar is showing initially
	bgw = NULL;
	dnddata = NULL;
	bgmode = False;
	
	m_drag_send_data_upp = NULL;
	m_scrap_promise_keeper_upp = NULL;
	m_current_scrap = NULL;
	m_current_scrap_data = NULL;
	m_drag_click = false;

	m_in_resize = false;

	m_dst_profile = nil;
}

MCScreenDC::~MCScreenDC()
{
	MCNotifyFinalize();

	if (opened)
		close(True);
	while (pendingevents != NULL)
	{
		MCEventnode *tptr =(MCEventnode *)pendingevents->remove(pendingevents);
		delete tptr;
	}
	if (bgw != NULL)
		DisposeGWorld(bgw);
}

uint2 MCScreenDC::getdepth(void)
{
	return devdepth;
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


///////////////////////////////////////////////////////////////////////////////

// TD-2013-07-01 [[ DynamicFonts ]]
bool MCScreenDC::loadfont(MCStringRef p_path, bool p_globally, void*& r_loaded_font_handle)
{	
	FSRef t_ref;
    FSSpec t_fsspec;
	OSErr t_os_error;
	
	t_os_error = MCS_mac_pathtoref(p_path, t_ref); // resolves and converts to UTF8.
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


bool MCScreenDC::unloadfont(MCStringRef p_path, bool p_globally, void *r_loaded_font_handle)
{
    OSStatus t_status;
    t_status = ATSFontDeactivate((ATSFontContainerRef)r_loaded_font_handle, NULL, kATSOptionFlagsDefault);
    
    return (t_status == noErr);
}

///////////////////////////////////////////////////////////////////////////////

bool MCScreenDC::listprinters(MCStringRef& r_printers)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	MCAutoCustomPointer<CFArrayRef, CFRelease> t_printers;
	if (PMServerCreatePrinterList(kPMServerLocal, &(&t_printers)) == noErr)
	{
		for(CFIndex i = 0; i < CFArrayGetCount(*t_printers); ++i)
		{
			MCAutoStringRef t_printer_name;
			if (!MCStringCreateWithCFString(PMPrinterGetName((PMPrinter)CFArrayGetValueAtIndex(*t_printers, i)), &t_printer_name))
				return false;
			if (!MCListAppend(*t_list, *t_printer_name))
				return false;
		}
	}

	return MCListCopyAsString(*t_list, r_printers);
}

MCPrinter *MCScreenDC::createprinter(void)
{
	return new MCMacOSXPrinter;
}

///////////////////////////////////////////////////////////////////////////////

MCStack *MCScreenDC::platform_getstackatpoint(int32_t x, int32_t y)
{
	Point t_location;
	t_location . h = x;
	t_location . v = y;
	
	WindowRef t_window;
	if (FindWindow(t_location, &t_window) != inContent)
		t_window = nil;
		
	if (t_window == nil)
		return nil;
		
	_Drawable d;
	d . type = DC_WINDOW;
	d . handle . window = (MCSysWindowHandle)t_window;
		
	return MCdispatcher -> findstackd(&d);
}

///////////////////////////////////////////////////////////////////////////////

CFStringRef MCScreenDC::convertutf8tocf(const char *p_utf8_string)
{
	return CFStringCreateWithCString(kCFAllocatorDefault, p_utf8_string, kCFStringEncodingUTF8);
}
