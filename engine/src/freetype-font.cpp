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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-legacy.h"
#include "customfont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsnames.h>

#include "freetype-font.h"

bool MCFreeTypeGetFontInfoFromData(FT_Library p_library, MCDataRef p_data, MCStringRef &r_name, MCStringRef &r_family, MCCustomFontStyle &r_style)
{
	bool t_success = true;

	FT_Face t_font_face;
	t_font_face = nil;
	if (t_success)
		t_success = FT_New_Memory_Face(p_library, (FT_Byte *)MCDataGetBytePtr(p_data), MCDataGetLength(p_data), 0, &t_font_face) == 0;

	MCAutoStringRef t_family;
	if (t_success)
		t_success = MCStringCreateWithCString(t_font_face->family_name, &t_family);
	
	uint32_t t_style = 0;
	if (t_success)
	{
		if (MCCStringEqualCaseless(t_font_face->style_name, "bold"))
			t_style = kMCCustomFontStyleBold;
		else if (MCCStringEqualCaseless(t_font_face->style_name, "italic"))
			t_style = kMCCustomFontStyleItalic;
		else if (MCCStringEqualCaseless(t_font_face->style_name, "bold italic"))
			t_style = kMCCustomFontStyleBoldItalic;
		else
			t_style = kMCCustomFontStyleRegular;
	}
	
	MCAutoStringRef t_name;
	if (t_success)
	{
		FT_SfntName t_sft_name;
		for (uint32_t i = 0; i < FT_Get_Sfnt_Name_Count(t_font_face); i++)
		{
			// Attempt to fetch the name of the font.  The name is name id 4 as defined in https://developer.apple.com/fonts/TTRefMan/RM06/Chap6name.html
			// It appears that the platform to use here is 1 (Macintosh according to the spec).
			FT_Get_Sfnt_Name(t_font_face, i, &t_sft_name);
			if (t_sft_name.name_id == 4 && t_sft_name.platform_id == 1 && t_sft_name.encoding_id == 0 && t_sft_name.language_id == 0 && t_sft_name.string_len != 0)
			{
				t_success = MCStringCreateWithNativeChars((char_t *)t_sft_name.string, t_sft_name.string_len, &t_name);
				break;
			}
		}
	}
	
	if (t_success)
		t_success = t_name.IsSet();
	
	if (t_success)
	{
		r_name = t_name.Take();
		r_family = t_family.Take();
		r_style = t_style;
	}
	
	if (t_font_face != nil)
		FT_Done_Face(t_font_face);
	
	return t_success;
}
