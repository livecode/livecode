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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCCStringFromUnicodeSubstring(const unichar_t * p_text, uindex_t p_length, char*& r_text);
extern void MCCStringFree(char *p_string);

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	// TODO: RTL
    
    if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;        
    
    MCAutoStringRef t_unicode_string;
    MCAutoStringRefAsUTF8String t_utf8_string;
    
    t_success = MCStringCreateWithChars(p_text, p_length / 2, &t_unicode_string);
    
    if (t_success)
        t_success = t_utf8_string . Lock(*t_unicode_string);
	
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(true);
		t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
		t_paint . setTextSize(p_font . size);
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		self -> layer -> canvas -> drawText(*t_utf8_string, t_utf8_string . Size(), MCGCoordToSkCoord(p_location . x), MCGCoordToSkCoord(p_location . y), t_paint);
	}
	
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	bool t_success;
	t_success = true;
    
    MCAutoStringRef t_unicode_string;
    MCAutoStringRefAsUTF8String t_utf8_string;
    
    t_success = MCStringCreateWithChars(p_text, p_length / 2, &t_unicode_string);
    
    if (t_success)
        t_success = t_utf8_string . Lock(*t_unicode_string);
    
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setTextSize(p_font . size);
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		t_width =  (MCGFloat) t_paint . measureText(*t_utf8_string, t_utf8_string . Size());
	}
	
	//self -> is_valid = t_success;
	return t_width;
}

////////////////////////////////////////////////////////////////////////////////
