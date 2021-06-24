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

#ifndef __MC_CUSTOMFONT_H__
#define __MC_CUSTOMFONT_H__

typedef uint32_t MCCustomFontStyle;
enum
{
	kMCCustomFontStyleRegular = 1 << 0,
	kMCCustomFontStyleBold = 1 << 1,
	kMCCustomFontStyleItalic = 1 << 2,
	kMCCustomFontStyleBoldItalic = 1 << 3
};

typedef bool (*MCCustomFontLoadTypefaceCallback(void *p_context, void *&r_typeface));

struct MCCustomFont {
	MCStringRef path;
	MCStringRef name;
	MCStringRef family;
	MCCustomFontStyle style;

	MCCustomFont *next;
};

bool MCCustomFontListInitialize();
void MCCustomFontListFinalize();

bool MCCustomFontCreate(MCStringRef p_path, MCStringRef p_name, MCStringRef p_family, MCCustomFontStyle p_style);
void MCCustomFontDelete(MCCustomFont* p_custom_font);

void MCCustomFontListAddFont(MCCustomFont *p_font);
bool MCCustomFontListLookupFontByName(MCStringRef p_name, MCCustomFont* &r_font);
bool MCCustomFontListLookupFontByFamilyAndStyle(MCStringRef p_family, bool p_bold, bool p_italic, MCCustomFont* &r_font);
bool MCCustomFontListLookupFont(MCStringRef p_name, bool p_bold, bool p_italic, MCCustomFont* &r_font);

bool MCCustomFontListGetNames(MCStringRef& r_names);
MCCustomFontStyle MCCustomFontListGetStylesForName(MCStringRef p_name);

#endif // __MC_CUSTOMFONT_H__
