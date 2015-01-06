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

#include "foundation.h"

#include <SkStream.h>
#include <SkTypeface.h>
#include <SkPaint.h>
#include "mblandroidtypeface.h"

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidTypefaceCreateWithData(void *p_data, uint32_t p_length, MCAndroidTypefaceRef &r_typeface)
{
	bool t_success = true;
	
    SkMemoryStream *t_stream;
    t_stream = nil;
    if (t_success)
    {
        t_stream = new SkMemoryStream(p_data, p_length, false);
        t_success = t_stream != nil;
    }
    
    SkTypeface *t_type_face;
    t_type_face = nil;
    if (t_success)
    {
        t_type_face = SkTypeface::CreateFromStream(t_stream);
        t_success = t_type_face != nil;
    }
	
	if (t_success)
		r_typeface = (MCAndroidTypefaceRef)t_type_face;
	else
	{
		if (t_stream != nil)
			delete t_stream;
	}
	
	return t_success;
}

bool MCAndroidTypefaceCreateWithName(const char *p_name, bool p_bold, bool p_italic, MCAndroidTypefaceRef &r_typeface)
{
	SkTypeface::Style t_style = SkTypeface::kNormal;
	if (p_bold)
	{
		if (p_italic)
			t_style = SkTypeface::kBoldItalic;
		else
			t_style = SkTypeface::kBold;
	}
	else if (p_italic)
		t_style = SkTypeface::kItalic;
	
	SkTypeface *t_typeface = nil;
	t_typeface = SkTypeface::CreateFromName(p_name, t_style);
	
	if (t_typeface == nil)
		return false;
	
	r_typeface = (MCAndroidTypefaceRef)t_typeface;
	return true;
}

void MCAndroidTypefaceRelease(MCAndroidTypefaceRef p_typeface)
{
	if (p_typeface != nil)
		((SkTypeface*)p_typeface)->unref();
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidTypefaceGetMetrics(MCAndroidTypefaceRef p_typeface, uint32_t p_size, float &r_ascent, float &r_descent, float &r_leading, float &r_xheight)
{
	bool t_success = true;
	
	SkPaint t_paint;
	t_paint.setTypeface((SkTypeface*)p_typeface);
	t_paint.setTextSize(p_size);
    
	SkPaint::FontMetrics t_metrics;
    
	t_paint.getFontMetrics(&t_metrics);
    
	r_ascent = t_metrics.fAscent;
	r_descent = t_metrics.fDescent;
    r_leading = t_metrics.fLeading;
    r_xheight = t_metrics.fXHeight;
	
	return true;
}

bool MCAndroidTypefaceMeasureText(MCAndroidTypefaceRef p_typeface, uint32_t p_size, const char *p_text, uint32_t p_text_length, bool p_utf16, float &r_length)
{
	SkPaint t_paint;
	t_paint.setTypeface((SkTypeface*)p_typeface);
	t_paint.setTextSize(p_size);
    
	t_paint.setTextEncoding(p_utf16 ? SkPaint::kUTF16_TextEncoding : SkPaint::kUTF8_TextEncoding);
	r_length = t_paint.measureText(p_text, p_text_length);
	
	return true;
}
