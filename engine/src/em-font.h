/* Copyright (C) 2020 LiveCode Ltd.

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

#ifndef __MC_EM_FONT_H__
#define __MC_EM_FONT_H__

bool MCEmscriptenCustomFontsInitialize();
void MCEmscriptenCustomFontsFinalize();

bool MCEmscriptenCustomFontsLoad();

bool MCEmscriptenListFontFamilies(MCListRef& r_names);
bool MCEmscriptenListFontsForFamily(MCStringRef p_family, uint32_t p_fsize, MCListRef& r_styles);

void* MCEmscriptenFontCreate(MCStringRef p_name, uint32_t p_size, bool p_bold, bool p_italic);
void MCEmscriptenFontDestroy(void *p_font);
void MCEmscriptenFontGetMetrics(void *p_font, float &r_ascent, float &r_descent, float &r_leading, float &r_xheight);
float MCEmscriptenFontMeasureText(void *p_font, const char *p_text, uint32_t p_text_length, bool p_is_unicode);

#endif // __MC_EM_FONT_H__
