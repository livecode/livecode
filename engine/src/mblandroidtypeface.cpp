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

#include "foundation.h"

#include <SkStream.h>
#include <SkTypeface.h>
#include <SkPaint.h>
#include "mblandroidtypeface.h"

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidTypefaceCreateWithData(void *p_data, uint32_t p_length, MCAndroidTypefaceRef &r_typeface)
{
	bool t_success = true;
	
    SkMemoryStream *t_stream = nil;
    if (t_success)
    {
        t_stream = new (nothrow) SkMemoryStream(p_data, p_length, false);
        t_success = t_stream != nil;
    }
    
    sk_sp<SkTypeface> t_type_face;
    if (t_success)
    {
        t_type_face = SkTypeface::MakeFromStream(t_stream);
        t_success = t_type_face != nil;
    }
    
    // SkTypeface::MakeFromStream takes ownership of the stream
    if (!t_success && t_stream != nil)
        delete t_stream;
	
	if (t_success)
		r_typeface = (MCAndroidTypefaceRef)(t_type_face.release());
	
	return t_success;
}

bool MCAndroidTypefaceCreateWithName(const char *p_name, bool p_bold, bool p_italic, MCAndroidTypefaceRef &r_typeface)
{
    SkFontStyle::Weight t_weight;
    SkFontStyle::Width t_width;
    SkFontStyle::Slant t_slant;
    
	if (p_bold)
        t_weight = SkFontStyle::kBold_Weight;
    else
        t_weight = SkFontStyle::kNormal_Weight;
    
    t_width = SkFontStyle::kNormal_Width;
    
    if (p_italic)
        t_slant = SkFontStyle::kItalic_Slant;
    else
        t_slant = SkFontStyle::kUpright_Slant;
	
    SkFontStyle t_style(t_weight, t_width, t_slant);
    
	sk_sp<SkTypeface> t_typeface = SkTypeface::MakeFromName(p_name, t_style);
	
	if (t_typeface == nil)
		return false;
	
	r_typeface = (MCAndroidTypefaceRef)(t_typeface.release());
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
    // Skia APIs expect typefaces to be passed as shared pointers
    sk_sp<SkTypeface> t_typeface = sk_ref_sp((SkTypeface *)p_typeface);
    
	SkPaint t_paint;
	t_paint.setTypeface(t_typeface);
	t_paint.setTextSize(p_size);
    
	SkPaint::FontMetrics t_metrics;
    
	t_paint.getFontMetrics(&t_metrics);
    
	// SkPaint::FontMetrics gives the ascent value as a negative offset from the baseline, where we expect the (positive) distance.
	r_ascent = - t_metrics.fAscent;
	r_descent = t_metrics.fDescent;
    r_leading = t_metrics.fLeading;
    r_xheight = t_metrics.fXHeight;
	
	return true;
}

bool MCAndroidTypefaceMeasureText(MCAndroidTypefaceRef p_typeface, uint32_t p_size, const char *p_text, uint32_t p_text_length, bool p_utf16, float &r_length)
{
    // Skia APIs expect typefaces to be passed as shared pointers
    sk_sp<SkTypeface> t_typeface = sk_ref_sp((SkTypeface *)p_typeface);
    
    SkPaint t_paint;
	t_paint.setTypeface(t_typeface);
	t_paint.setTextSize(p_size);
    
	t_paint.setTextEncoding(p_utf16 ? SkPaint::kUTF16_TextEncoding : SkPaint::kUTF8_TextEncoding);
	r_length = t_paint.measureText(p_text, p_text_length);
	
	return true;
}
