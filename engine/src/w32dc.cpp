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

bool MCScreenDC::textmask(MCFontStruct *p_font, const char *p_text, uint2 p_length, bool p_unicode_override, MCRectangle p_clip, MCGAffineTransform p_transform, MCGMaskRef& r_mask)
{
	bool t_success;
	t_success = true;

	HDC t_gdicontext;
	t_gdicontext = NULL;
	if (t_success)
	{
		t_gdicontext = CreateCompatibleDC(NULL);
		t_success = t_gdicontext != NULL;
	}
	
	if (t_success)
		t_success = SetGraphicsMode(t_gdicontext, GM_ADVANCED) != 0;

	if (t_success)
		t_success = SelectObject(t_gdicontext, p_font -> fid) != NULL;

	SIZE t_size;
	if (t_success)
	{
		if (p_unicode_override || p_font -> unicode)
			t_success = GetTextExtentPoint32W(t_gdicontext, (LPCWSTR)p_text, p_length >> 1, &t_size);
		else
			t_success = GetTextExtentPoint32A(t_gdicontext, p_text, p_length, &t_size);
	}
	
	TEXTMETRICA t_metrics;
	if (t_success)
		t_success = GetTextMetricsA(t_gdicontext, &t_metrics);

	MCRectangle t_transformed_bounds;
	MCRectangle t_bounds;
	if (t_success)
	{
		MCGRectangle t_gbounds;
		t_gbounds . origin . x = 0;
		t_gbounds . origin . y = -t_metrics . tmAscent;
		t_gbounds . size . width = t_size . cx;
		t_gbounds . size . height = t_metrics . tmAscent + t_metrics . tmDescent;
		t_gbounds = MCGRectangleApplyAffineTransform(t_gbounds, p_transform);
		
		t_transformed_bounds . x = floor(t_gbounds . origin . x);
		t_transformed_bounds . y = floor(t_gbounds . origin . y);
		t_transformed_bounds . width = ceil(t_gbounds . origin . x + t_gbounds . size . width) - t_transformed_bounds . x;
		t_transformed_bounds . height = ceil(t_gbounds . origin . y + t_gbounds . size . height) - t_transformed_bounds . y;
		
		t_bounds = MCU_intersect_rect(t_transformed_bounds, p_clip);
		
		if (t_bounds . width == 0 || t_bounds . height == 0)
		{
			DeleteDC(t_gdicontext);
			r_mask = nil;
			return true;
		}
	}

	void *t_data;
	t_data = NULL;
	HBITMAP t_gdibitmap;
	t_gdibitmap = NULL;
	if (t_success)
	{
		BITMAPINFO t_bitmapinfo;
		MCMemoryClear(&t_bitmapinfo, sizeof(BITMAPINFO));
		t_bitmapinfo . bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
		t_bitmapinfo . bmiHeader . biCompression = BI_RGB;
		t_bitmapinfo . bmiHeader . biWidth = t_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -t_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_gdibitmap = CreateDIBSection(t_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_data, NULL, 0);
		t_success = t_gdibitmap != NULL && t_data != NULL;
	}

	if (t_success)
		t_success = SelectObject(t_gdicontext, t_gdibitmap) != NULL;

	HBRUSH t_brush;
	t_brush = NULL;
	/*if (t_success)
	{
		t_brush = CreateSolidBrush(0x00ffffff);
		t_success = t_brush != NULL;
	}
	if (t_success)
		t_success = SelectObject(t_gdicontext, t_brush) != NULL;
	if (t_success)
		t_success = Rectangle(t_gdicontext, 0, 0, t_bounds . width, t_bounds . height);
	DeleteObject(t_brush);*/
	if (t_success)
		memset(t_data, 0x00, t_bounds . width * t_bounds . height * 4);

	if (t_success)
	{
		XFORM t_transform;
		t_transform . eM11 = p_transform . a;
		t_transform . eM12 = p_transform . b;
		t_transform . eM21 = p_transform . c;
		t_transform . eM22 = p_transform . d;
		t_transform . eDx = p_transform . tx - (t_bounds . x - t_transformed_bounds . x);
		t_transform . eDy = p_transform . ty - (t_bounds . y - t_transformed_bounds . y);
		t_success = SetWorldTransform(t_gdicontext, &t_transform);
	}

	if (t_success)
	{
		SetTextColor(t_gdicontext, 0x00FFFFFF);
		SetBkColor(t_gdicontext, 0x00000000);
		SetBkMode(t_gdicontext, OPAQUE);

		if (p_unicode_override || p_font -> unicode)
			t_success = TextOutW(t_gdicontext, 0, 0, (LPCWSTR)p_text, p_length >> 1);
		else
			t_success = TextOutA(t_gdicontext, 0, 0, p_text, p_length);
	}

	if (t_success)
		t_success = GdiFlush();

	MCGMaskRef t_mask;
	t_mask = NULL;
	if (t_success)
	{
		MCGDeviceMaskInfo t_mask_info;
		t_mask_info . format = kMCGMaskFormat_LCD32;
		t_mask_info . x = t_bounds . x;
		t_mask_info . y = t_bounds . y;
		t_mask_info . width = t_bounds . width;
		t_mask_info . height = t_bounds . height;
		t_mask_info . data = memdup(t_data, t_bounds . width * t_bounds . height * 4);
		t_success = MCGMaskCreateWithInfoAndRelease(t_mask_info, t_mask);
	}

	if (t_success)
		r_mask = t_mask;

	if (t_gdibitmap != NULL)
		DeleteObject(t_gdibitmap);

	DeleteDC(t_gdicontext);
	return t_success;
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
