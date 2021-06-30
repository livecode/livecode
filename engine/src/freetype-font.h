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

#ifndef __MC_FREETYPE_FONT_H__
#define __MC_FREETYPE_FONT_H__

bool MCFreeTypeGetFontInfoFromData(FT_Library p_library, MCDataRef p_data, MCStringRef &r_name, MCStringRef &r_family, MCCustomFontStyle &r_style);

#endif //__MC_FREETYPE_FONT_H__
