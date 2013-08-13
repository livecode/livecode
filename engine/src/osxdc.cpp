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

#include "graphicscontext.h"

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

extern int4 OSX_DrawUnicodeText(int2 x, int2 y, const void *p_text, uint4 p_text_byte_length, MCFontStruct *f, bool p_fill_background, bool p_measure_only = false);

int4 MCScreenDC::textwidth(MCFontStruct *f, const char *s, uint2 len, bool p_unicode_override)
{
	if (len == 0)
		return 0;
	if (f->unicode || p_unicode_override)
	{
		if (MCmajorosversion >= 0x1050)
		{
			return OSX_DrawUnicodeText(0, 0, s, len, f, false, true);
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


			cfstring = CFStringCreateWithCharactersNoCopy(NULL, (UniChar *)(tempbuffer != NULL? tempbuffer: s), (tempbuffer != NULL? len + 2:len) >> 1,
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
			while (len--)
				iwidth += f->widths[(uint1)*s++];
			return iwidth;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

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
	
	MCRectangle t_bounds;
	if (t_success)
		t_success = drawtexttocgcontext(0, 0, t_text, t_length, p_font, nil, t_bounds);
	
	if (t_success)
	{
		MCGRectangle t_gbounds;
		t_gbounds = MCGRectangleMake(t_bounds . x, t_bounds . y, t_bounds . width, t_bounds . height);
		t_gbounds = MCGRectangleApplyAffineTransform(t_gbounds, p_transform);
		
		t_bounds . x = floor(t_gbounds . origin .x);
		t_bounds . y = floor(t_gbounds . origin . y);
		t_bounds . width = ceil(t_gbounds . origin . x + t_gbounds . size . width) - t_bounds . x;
		t_bounds . height = ceil(t_gbounds . origin . y + t_gbounds . size . height) - t_bounds . y;
		
		t_bounds = MCU_intersect_rect(t_bounds, p_clip);
		
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
		CGContextTranslateCTM(t_cgcontext, -t_bounds . x, -t_bounds . y);
		CGContextConcatCTM(t_cgcontext, CGAffineTransformMake(p_transform . a, p_transform . b, p_transform . c, p_transform . d, p_transform . tx, p_transform . ty));
		
		CGContextSetRGBFillColor(t_cgcontext, 1.0, 1.0, 1.0, 0.0);
		CGContextFillRect(t_cgcontext, CGRectMake(t_bounds . x, t_bounds . y, t_bounds . width, t_bounds . height));
		CGContextSetRGBFillColor(t_cgcontext, 0.0, 0.0, 0.0, 1.0);
	}
	
	if (t_success)
		t_success = drawtexttocgcontext(0, 0, t_text, t_length, p_font, t_cgcontext, t_bounds);
	
	MCGMaskRef t_mask;
	if (t_success)
	{
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
