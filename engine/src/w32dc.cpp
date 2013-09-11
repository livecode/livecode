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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "osspec.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "stacklst.h"
#include "execpt.h"
#include "globals.h"
#include "core.h"
#include "notify.h"

#include "w32dc.h"
#include "w32printer.h"
#include "w32context.h"

#include "mctheme.h"

#include "graphicscontext.h"
#include "graphics_util.h"

MCScreenDC::MCScreenDC()
{
	f_src_dc = NULL;
	f_dst_dc = NULL;

#ifdef FEATURE_TASKBAR_ICON
	f_has_icon = False;
	f_icon_menu = NULL;
#endif

	pendingevents = NULL;
	beeppitch = 440L;                    //440 Hz
	beepduration = 100;                 // 1/100 second
	grabbingclipboard = False;
	exposures = False;
	opened = 0;
	dnddata = NULL;
	taskbarhidden = False;

	backdrop_active = false;
	backdrop_hard = false;
	backdrop_window = NULL;
	memset(&backdrop_colour, 0, sizeof(MCColor));
	backdrop_pattern = NULL;
	backdrop_badge = NULL;

	m_printer_dc = NULL;
	m_printer_dc_locked = false;
	m_printer_dc_changed = false;

	m_clipboard = NULL;

	MCNotifyInitialize();
}

MCScreenDC::~MCScreenDC()
{
	MCNotifyFinalize();

	showtaskbar();
	while (opened)
		close(True);
	if (ncolors != 0)
	{
		uint2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
			if (colornames[i] != NULL)
				delete colornames[i];
		}
		delete colors;
		delete colornames;
		delete allocs;
	}
	while (pendingevents != NULL)
	{
		MCEventnode *tptr =(MCEventnode *)pendingevents->remove(pendingevents);
		delete tptr;
	}
}

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	switch(p_feature)
	{
	case PLATFORM_FEATURE_WINDOW_TRANSPARENCY:
		return MCmajorosversion >= 0x0500;
	break;

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
// TD-2013-07-01 [[ DynamicFonts ]]
bool MCScreenDC::loadfont(const char *p_path, bool p_globally, void*& r_loaded_font_handle)
{
	bool t_success = true;
    DWORD t_private = NULL;
    
    if (!p_globally)
        t_private = FR_PRIVATE;
    
	if (t_success)
		t_success = (MCS_exists(p_path, True) == True);
	
	if (t_success)
		t_success = (AddFontResourceExA(p_path, t_private, 0) != 0);
    
	if (t_success && p_globally)
		PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    
	return t_success;
}


bool MCScreenDC::unloadfont(const char *p_path, bool p_globally, void *r_loaded_font_handle)
{
    bool t_success = true;
    DWORD t_private = NULL;
    
    if (p_globally)
        t_private = FR_PRIVATE;
    
    if (t_success)
		t_success = (RemoveFontResourceExA(p_path, t_private, 0) != 0);
    
	if (t_success && p_globally)
		PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
    
	return t_success;
}

// MM-2013-08-30: [[ RefactorGraphics ]] Move text measuring to libgraphics.
int4 MCScreenDC::textwidth(MCFontStruct *p_font, const char *p_text, uint2 p_length, bool p_unicode_override)
{
	if (p_length == 0 || p_text == NULL)
		return 0;
	
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font);
	
	MCExecPoint ep;
	ep . setsvalue(MCString(p_text, p_length));
	if (!p_font -> unicode && !p_unicode_override)
		ep . nativetoutf16();
	
	return MCGContextMeasurePlatformText(NULL, (unichar_t *) ep . getsvalue() . getstring(), ep . getsvalue() . getlength(), t_font);
}	

///////////////////////////////////////////////////////////////////////////////

extern int UTF8ToUnicode(const char *p_source_str, int p_source, uint2 *p_dest_str, int p_dest);

LPWSTR MCScreenDC::convertutf8towide(const char *p_utf8_string)
{
	int t_new_length;
	t_new_length = UTF8ToUnicode(p_utf8_string, strlen(p_utf8_string), NULL, 0);

	LPWSTR t_result;
	t_result = new WCHAR[t_new_length + 2];

	t_new_length = UTF8ToUnicode(p_utf8_string, strlen(p_utf8_string), (uint2*)t_result, t_new_length);
	t_result[t_new_length / 2] = 0;

	return t_result;
}

LPCSTR MCScreenDC::convertutf8toansi(const char *p_utf8_string)
{
	LPWSTR t_wide;
	t_wide = convertutf8towide(p_utf8_string);

	int t_length;
	t_length = WideCharToMultiByte(CP_ACP, 0, t_wide, -1, NULL, 0, NULL, NULL);

	LPSTR t_result;
	t_result = new CHAR[t_length];
	WideCharToMultiByte(CP_ACP, 0, t_wide, -1, t_result, t_length, NULL, NULL);

	delete t_wide;

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

MCPrinter *MCScreenDC::createprinter(void)
{
	return new MCWindowsPrinter;
}

void MCScreenDC::listprinters(MCExecPoint& ep)
{
	ep . clear();

	DWORD t_flags;
	DWORD t_level;
	t_flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
	t_level = 4;

	DWORD t_printer_count;
	DWORD t_bytes_needed;
	t_printer_count = 0;
	t_bytes_needed = 0;
	if (EnumPrintersA(t_flags, NULL, t_level, NULL, 0, &t_bytes_needed, &t_printer_count) == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return;

	char *t_printers;
	t_printers = new char[t_bytes_needed];
	if (EnumPrintersA(t_flags, NULL, t_level, (LPBYTE)t_printers, t_bytes_needed, &t_bytes_needed, &t_printer_count) != 0)
	{
		for(uint4 i = 0; i < t_printer_count; ++i)
		{
			const char *t_printer_name;
			t_printer_name = ((PRINTER_INFO_4A *)t_printers)[i] . pPrinterName;
			ep . concatcstring(t_printer_name, EC_RETURN, i == 0);
		}
	}

	delete t_printers;
}

///////////////////////////////////////////////////////////////////////////////

MCStack *MCScreenDC::device_getstackatpoint(int32_t x, int32_t y)
{
	POINT t_location;
	t_location . x = x;
	t_location . y = y;
	
	HWND t_window;
	t_window = WindowFromPoint(t_location);

	if (t_window == nil)
		return nil;
		
	_Drawable d;
	d . type = DC_WINDOW;
	d . handle . window = (MCSysWindowHandle)t_window;
		
	return MCdispatcher -> findstackd(&d);
}

///////////////////////////////////////////////////////////////////////////////

// IM-2013-08-08: [[ ResIndependence ]] Windows implementation currently returns 1.0
MCGFloat MCResGetDeviceScale(void)
{
	return 1.0;
}

///////////////////////////////////////////////////////////////////////////////
