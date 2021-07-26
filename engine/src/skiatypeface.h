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

#ifndef __MC_SKIA_TYPEFACE__
#define __MC_SKIA_TYPEFACE__

struct __MCSkiaTypeface;
typedef __MCSkiaTypeface *MCSkiaTypefaceRef;

struct MCSkiaFont
{
	uint32_t size;
	MCSkiaTypefaceRef typeface;
};

bool MCSkiaTypefaceCreateWithData(MCDataRef p_data, MCSkiaTypefaceRef &r_typeface);
bool MCSkiaTypefaceCreateWithName(MCStringRef p_name, bool p_bold, bool p_italic, MCSkiaTypefaceRef &r_typeface);
void MCSkiaTypefaceRelease(MCSkiaTypefaceRef p_typeface);
bool MCSkiaTypefaceGetMetrics(MCSkiaTypefaceRef p_typeface, uint32_t p_size, float &r_ascent, float &r_descent, float& r_leading, float& r_xheight);
bool MCSkiaTypefaceMeasureText(MCSkiaTypefaceRef p_typeface, uint32_t p_size, const char *p_text, uint32_t p_text_length, bool p_utf16, float &r_length);

#endif
