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

#include "customfont.h"

static MCCustomFont *s_custom_font_list = nil;

bool MCCustomFontListInitialize()
{
	s_custom_font_list = nil;

	return true;
}

void MCCustomFontListFinalize()
{
	for (MCCustomFont *t_font = s_custom_font_list; t_font != nil; )
	{
		MCCustomFont *t_next_font;
		t_next_font = t_font->next;
		MCCustomFontDelete(t_font);
		t_font = t_next_font;
	}
	s_custom_font_list = nil;
}

bool MCCustomFontCreate(MCStringRef p_path, MCStringRef p_name, MCStringRef p_family, MCCustomFontStyle p_style, MCCustomFont* &r_font)
{
	if (!MCMemoryNew(r_font))
		return false;

	r_font->path = MCValueRetain(p_path);
	r_font->name = MCValueRetain(p_name);
	r_font->family = MCValueRetain(p_family);
	r_font->style = p_style;

	r_font->next = nil;

	return true;
}

void MCCustomFontDelete(MCCustomFont *p_font)
{
	if (p_font != nil)
	{
		MCValueRelease(p_font->path);
		MCValueRelease(p_font->name);
		MCValueRelease(p_font->family);
		MCMemoryDelete(p_font);
	}
}

void MCCustomFontListAddFont(MCCustomFont *p_font)
{
	if (s_custom_font_list == nil)
		s_custom_font_list = p_font;
	else
	{
		MCCustomFont *t_font = s_custom_font_list;
		while (t_font->next != nil)
			t_font = t_font->next;
		t_font->next = p_font;
	}
	p_font->next = nil;
}

bool MCCustomFontListGetNames(MCStringRef& r_names)
{
	bool t_success;
	t_success = true;
	
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	
	for (MCCustomFont *t_font = s_custom_font_list; t_success && t_font != nil; t_font = t_font->next)
		t_success = MCListAppend(*t_list, t_font->name);
	
	if (t_success)
		return MCListCopyAsString(*t_list, r_names);
	
	return false;
}

bool MCCustomFontListLookupFontByName(MCStringRef p_name, MCCustomFont* &r_font)
{
	for (MCCustomFont *t_font = s_custom_font_list; t_font != nil; t_font = t_font->next)
	{
		if (MCStringIsEqualTo(p_name, t_font->name, kMCStringOptionCompareCaseless))
		{
			r_font = t_font;
			return true;
		}
	}

	return false;
}

bool MCCustomFontListLookupFontByFamilyAndStyle(MCStringRef p_family, bool p_bold, bool p_italic, MCCustomFont* &r_font)
{
	MCCustomFont *t_closest_font;
	t_closest_font = nil;
	for (MCCustomFont *t_font = s_custom_font_list; t_font != nil; t_font = t_font->next)
	{
		if (MCStringIsEqualTo(p_family, t_font->family, kMCStringOptionCompareCaseless))
		{
			if ((p_bold && p_italic && t_font->style == kMCCustomFontStyleBoldItalic) || 
				(p_bold && t_font->style == kMCCustomFontStyleBold) || 
				(p_italic && t_font->style == kMCCustomFontStyleItalic))
			{
				r_font = t_font;
				return true;
			}
			else if (t_closest_font == nil)
				t_closest_font = t_font;
		}
	}

	if (t_closest_font != nil)
	{
		r_font = t_closest_font;
		return true;
	}

	return false;
}

bool MCCustomFontListLookupFont(MCStringRef p_name, bool p_bold, bool p_italic, MCCustomFont* &r_font)
{
	MCAutoStringRef t_styled_name;
	if (!MCStringMutableCopy(p_name, &t_styled_name))
		return false;
	if (p_bold && !MCStringAppend(*t_styled_name, MCSTR(" Bold")))
		return false;
	if (p_italic && !MCStringAppend(*t_styled_name, MCSTR(" Italic")))
		return false;
	
	// First of all, attempt to look the font up by taking into account its name and style.
	// e.g. textFont:Arial textStyle:Bold - look for a font named Arial Bold.
	// This will fail for textFonts which include style information e.g. textFont:Arial textStyle:Bold will search for Arial Bold Bold
	if (MCCustomFontListLookupFontByName(*t_styled_name, r_font))
		return true;
	
	// If no font found, look up based purely on the name.  This will solve cases where style
	// information is included in the name e.g. Arial Bold.
	if (p_bold || p_italic)
	{
		if (MCCustomFontListLookupFontByName(p_name, r_font))
			return true;
	}
	
	// If we've still not found a matching font, look up based on the family and style.
	// This function will attempt to provide a closest match e.g. Arial Bold is requested but only Arial is installed.
	if (MCCustomFontListLookupFontByFamilyAndStyle(p_name, p_bold, p_italic, r_font))
		return true;

	return false;
}

MCCustomFontStyle MCCustomFontListGetStylesForName(MCStringRef p_name)
{
	MCCustomFontStyle t_styles;
	t_styles = 0;
	
	for (MCCustomFont *t_font = s_custom_font_list; t_font != nil; t_font = t_font->next)
	{
		if (MCStringIsEqualTo(p_name, t_font->family, kMCStringOptionCompareCaseless))
			t_styles |= t_font->style;
	}
	
	if (t_styles == 0)
	{
		MCCustomFont *t_font = nil;
		if (MCCustomFontListLookupFontByName(p_name, t_font))
			t_styles |= t_font->style;
	}
	
	return t_styles;
}
