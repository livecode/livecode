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
#include "osxprinter.h"

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

#if 0
// MM-2013-08-16: [[ RefactorGraphics ]] Refactored to use drawtexttocgcontext to measure the bounds of the text.
int4 MCScreenDC::textwidth(MCFontStruct *p_font, const char *p_text, uint2 p_length, bool p_unicode_override)

	if (p_length == 0)
		return 0;
	
	bool t_success;
	t_success = true;
	
	MCExecPoint ep;
	uint2 t_length;
	const void *t_text;
	t_text = nil;
	if (t_success)
	{
		if (p_font -> unicode || p_unicode_override)
		{
			t_text = p_text;
			t_length = p_length;
		}
		else
		{
			ep . setsvalue(MCString(p_text, p_length));
			ep . nativetoutf16();
			t_text =  ep . getsvalue() . getstring();
			t_length =  ep . getsvalue() . getlength();
		}
		t_success = t_text != nil;
	}
	
	MCRectangle t_bounds;
	if (t_success)
		t_success = drawtexttocgcontext(0, 0, t_text, t_length, p_font, nil, t_bounds);
	
	if (t_success)
		return t_bounds . width;
	else
		return 0;
}
#endif


// MM-2013-08-16: [[ RefactorGraphics ]] Render text into mask taking into account clip and transform.
bool MCScreenDC::textmask(MCFontStruct *p_font, const char *p_text, uint2 p_length, bool p_unicode_override, MCRectangle p_clip, MCGAffineTransform p_transform, MCGMaskRef& r_mask)
{
	bool t_success;
	t_success = true;
	
	MCExecPoint ep;
	uint2 t_length;
	const void *t_text;
	t_text = nil;
	if (t_success)
	{
		if (p_font -> unicode || p_unicode_override)
		{
			t_text = p_text;
			t_length = p_length;
		}
		else
		{
			ep . setsvalue(MCString(p_text, p_length));
			ep . nativetoutf16();
			t_text =  ep . getsvalue() . getstring();
			t_length =  ep . getsvalue() . getlength();
		}
		t_success = t_text != nil;
	}
	
	MCRectangle t_text_bounds;
	if (t_success)
		t_success = drawtexttocgcontext(0, 0, t_text, t_length, p_font, nil, t_text_bounds);
	
	MCRectangle t_transformed_bounds;
	MCRectangle t_bounds;
	if (t_success)
	{
		MCGRectangle t_gbounds;
		t_gbounds = MCGRectangleMake(t_text_bounds . x, t_text_bounds . y, t_text_bounds . width, t_text_bounds . height);
		t_gbounds = MCGRectangleApplyAffineTransform(t_gbounds, p_transform);
		
		t_transformed_bounds . x = floor(t_gbounds . origin . x);
		t_transformed_bounds . y = floor(t_gbounds . origin . y);
		t_transformed_bounds . width = ceil(t_gbounds . origin . x + t_gbounds . size . width) - t_transformed_bounds . x;
		t_transformed_bounds . height = ceil(t_gbounds . origin . y + t_gbounds . size . height) - t_transformed_bounds . y;
		
		t_bounds = MCU_intersect_rect(t_transformed_bounds, p_clip);
		
		if (t_bounds . width == 0 || t_bounds . height == 0)
		{
			r_mask = nil;
			return true;
		}
	}
	
	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryNew(t_bounds . width * t_bounds . height, t_data);
	
	CGContextRef t_cgcontext;
	t_cgcontext = nil;
	if (t_success)
	{
		t_cgcontext = CGBitmapContextCreate(t_data, t_bounds . width, t_bounds . height, 8, t_bounds . width, nil, kCGImageAlphaOnly);
		t_success = t_cgcontext != nil;
	}
		
	if (t_success)
	{
		CGContextSetRGBFillColor(t_cgcontext, 1.0, 1.0, 1.0, 0.0);
		CGContextFillRect(t_cgcontext, CGRectMake(0, 0, t_bounds . width, t_bounds . height));
		CGContextSetRGBFillColor(t_cgcontext, 0.0, 0.0, 0.0, 1.0);
		
		CGContextTranslateCTM(t_cgcontext, -(t_bounds . x - t_transformed_bounds . x), -(t_bounds . y - t_transformed_bounds . y));
		CGContextTranslateCTM(t_cgcontext, 0, t_transformed_bounds . height + t_transformed_bounds . y);
		CGContextConcatCTM(t_cgcontext, CGAffineTransformMake(p_transform . a, p_transform . b, p_transform . c, p_transform . d, p_transform . tx, p_transform . ty));
		
		t_success = drawtexttocgcontext(0, 0, t_text, t_length, p_font, t_cgcontext, t_bounds);
	}
	
	MCGMaskRef t_mask;
	t_mask = nil;
	if (t_success)
	{
		CGContextFlush(t_cgcontext);
		
		MCGDeviceMaskInfo t_mask_info;
		t_mask_info . format = kMCGMaskFormat_A8;
		t_mask_info . x = t_bounds . x;
		t_mask_info . y = t_bounds . y;
		t_mask_info . width = t_bounds . width;
		t_mask_info . height = t_bounds . height;
		t_mask_info . data = t_data;
		t_success = MCGMaskCreateWithInfoAndRelease(t_mask_info, t_mask);
	}
	
	if (t_success)
		r_mask = t_mask;
	else
		MCMemoryDelete(t_data);
	
	CGContextRelease(t_cgcontext);	
	return t_success;	
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
