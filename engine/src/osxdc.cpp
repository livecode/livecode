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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "globals.h"
#include "execpt.h"
#include "notify.h"

#include "osxdc.h"
#include "osxcontext.h"
#include "osxprinter.h"

///////////////////////////////////////////////////////////////////////////////

extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release = true);

///////////////////////////////////////////////////////////////////////////////

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
	menuBarHidden = False; //menu bar is showing initially�
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
	
	MCNotifyInitialize();
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

extern int4 OSX_DrawUnicodeText(int2 x, int2 y, const void *p_text, uint4 p_text_byte_length, MCFontStruct *f, bool p_fill_background, bool p_measure_only = false);

int4 MCScreenDC::textwidth(MCFontStruct *f, MCStringRef s, uint2 len, bool p_unicode_override)
{
	if (len == 0)
		return 0;
	if (f->unicode || p_unicode_override)
	{
		if (MCmajorosversion >= 0x1050)
		{
			return OSX_DrawUnicodeText(0, 0, MCStringGetCString(s), len, f, false, true);
		}
		else
		{
			int4 fwidth;
			short oldfid = GetPortTextFont(GetQDGlobalsThePort());
			short oldsize = GetPortTextSize(GetQDGlobalsThePort());
			short oldstyle = GetPortTextFace(GetQDGlobalsThePort());
			TextFont((short)(intptr_t)f->fid);
			TextSize(f->size);
			TextFace(f->style);


			SInt16  baseline;
			CFStringRef cfstring;

			char *tempbuffer = NULL;


			if (len)
			{
				uint2 *testchar = (uint2 *)s;
				if (testchar[(len - 2 )>> 1] == 12398)
				{
					tempbuffer = new char[len+2];
					memcpy(tempbuffer,s,len);
					uint2 *tchar = (uint2 *)&tempbuffer[len];
					*tchar = 0;
				}
			}

            MCAutoStringRef t_tempbuffer;
            /* UNCHECKED */ MCStringCreateWithCString(tempbuffer, &t_tempbuffer);
			cfstring = CFStringCreateWithCharactersNoCopy(NULL, (UniChar *)(tempbuffer != NULL? MCStringGetCString(*t_tempbuffer): MCStringGetCString(s)), (tempbuffer != NULL? len + 2:len) >> 1,
					   kCFAllocatorNull);
			Point dimensions = {0, 0};
			GetThemeTextDimensions(cfstring, kThemeCurrentPortFont, kThemeStateActive, false, &dimensions, &baseline);
			fwidth = dimensions.h;
			CFRelease(cfstring);
			if (tempbuffer)
				delete tempbuffer;

			TextFont(oldfid);
			TextSize(oldsize);
			TextFace(oldstyle);
			return fwidth;
		}
	}
	else
	{
		// MW-2012-09-21: [[ Bug 3884 ]] If the font is wide, measure using OS routine.
		if (f -> wide)
		{
			int4 fwidth;
			short oldfid = GetPortTextFont(GetQDGlobalsThePort());
			short oldsize = GetPortTextSize(GetQDGlobalsThePort());
			short oldstyle = GetPortTextFace(GetQDGlobalsThePort());
			TextFont((short)(intptr_t)f->fid);
			TextSize(f->size);
			TextFace(f->style);

			fwidth = TextWidth(s, 0, len);

			TextFont(oldfid);
			TextSize(oldsize);
			TextFace(oldstyle);
			return fwidth;
		}
		else
		{
            int4 iwidth = 0;
            uindex_t t_start;
            t_start = 0;
            while (len--)
            {
                iwidth += f->widths[(uint1)MCStringGetNativeCharAtIndex(s, t_start)];
                
            }
            return iwidth;
        }
    }
}

MCContext *MCScreenDC::createcontext(Drawable p_drawable, MCBitmap *p_alpha)
{
	MCQuickDrawContext *t_context;
	t_context = MCQuickDrawContext::create_with_port(p_drawable -> type == DC_WINDOW ? GetWindowPort((WindowPtr)p_drawable -> handle . window) : (CGrafPtr)p_drawable -> handle . pixmap, false, true);
	t_context -> setexternalalpha(p_alpha);
	return t_context;
}

MCContext *MCScreenDC::createcontext(Drawable p_drawable, bool p_alpha, bool p_transient)
{
	return MCQuickDrawContext::create_with_port(p_drawable -> type == DC_WINDOW ? GetWindowPort((WindowPtr)p_drawable -> handle . window) : (CGrafPtr)p_drawable -> handle . pixmap, p_transient, p_alpha);
}

MCContext *MCScreenDC::creatememorycontext(uint2 p_width, uint2 p_height, bool p_alpha, bool p_transient)
{
	return MCQuickDrawContext::create_with_parameters(p_width, p_height, p_transient, p_alpha);
}

void MCScreenDC::freecontext(MCContext *p_context)
{
	delete p_context;
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

MCStack *MCScreenDC::getstackatpoint(int32_t x, int32_t y)
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
