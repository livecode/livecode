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

int4 MCScreenDC::textwidth(MCFontStruct *f, const char *s, uint2 len, bool p_unicode_override)
{
	if (len == 0)
		return 0;

	// MW-2012-09-21: [[ Bug 3884 ]] If the font is wide, measure using OS routine.
	if (f->printer || f->unicode || f->charset || f->wide || p_unicode_override)
	{
		HDC hdc;
		if (f->printer)
			hdc = static_cast<MCWindowsPrinter *>(MCsystemprinter) -> GetDC();
		else
			hdc = f_src_dc;
		HFONT oldfont = (HFONT)SelectObject(hdc, (HFONT)f->fid);
		SIZE tsize;

		if (f->unicode || p_unicode_override)
		{
			if ((long)s & 1)
			{ // odd byte boundary, must be realigned
				char *b = new char[len];
				memcpy(b, s, len);
				GetTextExtentPoint32W(hdc, (LPCWSTR)b,
				                      (int)len >> 1, &tsize);
				delete b;
			}
			else
				GetTextExtentPoint32W(hdc, (LPCWSTR)s,
				                      (int)len >> 1, &tsize);
		}
		else
			GetTextExtentPoint32A(hdc, (LPCSTR)s, (int)len, &tsize);

		SelectObject(hdc, oldfont);
		return tsize.cx;
	}
	else
	{
		int4 iwidth = 0;
		while (len--)
			iwidth += f->widths[(uint1)*s++];
		return iwidth;
	}
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

MCStack *MCScreenDC::getstackatpoint(int32_t x, int32_t y)
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
