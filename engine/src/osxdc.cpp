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

#include "core.h"
#include "osspec.h"
#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "globals.h"
#include "execpt.h"
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

// IM-2013-08-01: [[ ResIndependence ]] OSX implementation currently returns 1.0
MCGFloat MCResGetDeviceScale(void)
{
	return 1.0;
}

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
	menuBarHidden = False; //menu bar is showing initiallyÅ
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


///////////////////////////////////////////////////////////////////////////////

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

// MM-2013-08-16: [[ RefactorGraphics ]] Refactored from previous ATSUI drawing method.
//	If context is NULL, the bounds of the text is calculated. Otherwise, the text is rendered into the context at the given location.
static bool drawtexttocgcontext(int2 x, int2 y, const void *p_text, uint4 p_length, MCFontStruct *p_font, CGContextRef p_cg_context, MCRectangle& r_bounds)
{	
	OSStatus t_err;
	t_err = noErr;	
	
	ATSUFontID t_font_id;
	Fixed t_font_size;
	t_font_size = p_font -> size << 16;	
	
	ATSUAttributeTag t_tags[] =
	{
		kATSUFontTag,
		kATSUSizeTag,
	};
	ByteCount t_sizes[] =
	{
		sizeof(ATSUFontID),
		sizeof(Fixed),
	};
	ATSUAttributeValuePtr t_attrs[] =
	{
		&t_font_id,
		&t_font_size,
	};	
	
	ATSLineLayoutOptions t_layout_options;
	ATSUAttributeTag t_layout_tags[] =
	{
		kATSULineLayoutOptionsTag,
		kATSUCGContextTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(ATSLineLayoutOptions),
		sizeof(CGContextRef),
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_layout_options,
		&p_cg_context,
	};	
	
	if (t_err == noErr)
		t_err = ATSUFONDtoFontID((short)(intptr_t)p_font -> fid, p_font -> style, &t_font_id);
	
	ATSUStyle t_style;
	t_style = nil;
	if (t_err == noErr)
		t_err = ATSUCreateStyle(&t_style);
	if (t_err == noErr)
		t_err = ATSUSetAttributes(t_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
	
	ATSUTextLayout t_layout;
	t_layout = nil;
	if (t_err == noErr)
	{
		UniCharCount t_run;
		t_run = p_length / 2;
		t_err = ATSUCreateTextLayoutWithTextPtr((const UniChar *)p_text, 0, p_length / 2, p_length / 2, 1, &t_run, &t_style, &t_layout);
	}
	if (t_err == noErr)
		t_err = ATSUSetTransientFontMatching(t_layout, true);
	if (t_err == noErr)
	{
		t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
		t_err = ATSUSetLayoutControls(t_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	}	
	
	MCRectangle t_bounds;
	if (p_cg_context == nil)
	{
		ATSUTextMeasurement t_before, t_after, t_ascent, t_descent;
		if (t_err == noErr)
			t_err = ATSUGetUnjustifiedBounds(t_layout, 0, p_length / 2, &t_before, &t_after, &t_ascent, &t_descent);
		
		if (t_err == noErr)
		{
			t_ascent = (t_ascent + 0xffff) >> 16;
			t_descent = (t_descent + 0xffff) >> 16;
			t_after = (t_after + 0xffff) >> 16;
			
			t_bounds . x = x;
			t_bounds . y = y - p_font -> ascent;
			t_bounds . width = t_after;
			t_bounds . height = t_descent + t_ascent;
			
			r_bounds = t_bounds;
		}
	}
	
	if (t_err == noErr)
		if (p_cg_context != nil)
			t_err = ATSUDrawText(t_layout, 0, p_length / 2, x << 16, y << 16);
	
	if (t_layout != nil)
		ATSUDisposeTextLayout(t_layout);
	if (t_style != nil)
		ATSUDisposeStyle(t_style);
	
	return t_err == noErr;	
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

///////////////////////////////////////////////////////////////////////////////

MCStack *MCScreenDC::device_getstackatpoint(int32_t x, int32_t y)
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
