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

#ifndef __MC_MOBILE_ANDROID_TYPEFACE__
#define __MC_MOBILE_ANDROID_TYPEFACE__

struct __MCAndroidTypeface;
typedef __MCAndroidTypeface *MCAndroidTypefaceRef;

struct MCAndroidFont
{
	uint32_t size;
	MCAndroidTypefaceRef typeface;
};

bool MCAndroidTypefaceCreateWithData(void *p_data, uint32_t p_length, MCAndroidTypefaceRef &r_typeface);
bool MCAndroidTypefaceCreateWithName(const char *p_name, bool p_bold, bool p_italic, MCAndroidTypefaceRef &r_typeface);
void MCAndroidTypefaceRelease(MCAndroidTypefaceRef p_typeface);
bool MCAndroidTypefaceGetMetrics(MCAndroidTypefaceRef p_typeface, uint32_t p_size, float &r_ascent, float &r_descent, float& r_leading, float& r_xheight);
bool MCAndroidTypefaceMeasureText(MCAndroidTypefaceRef p_typeface, uint32_t p_size, const char *p_text, uint32_t p_text_length, bool p_utf16, float &r_length);

#endif
