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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "notify.h"
#include "statemnt.h"
#include "funcs.h"
#include "eventqueue.h"

#include "mode.h"
#include "util.h"
#include "osspec.h"

#include "system.h"

#include "mblandroidutil.h"
#include "skiatypeface.h"
#include "customfont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsnames.h>

#include "freetype-font.h"

////////////////////////////////////////////////////////////////////////////////

// For the moment the Droid * font details are hard coded in (used to populate the fontNames and fontStyles properties).
// The built in font support is limited by the Skia library, so we don't support any custom fonts the user may have installed.
// None the less, it might be a better idea to get the built in font names dynamically (if possible).

struct MCAndroidDroidFontMap
{
	const char *name;
	MCCustomFontStyle styles;
};

static const MCAndroidDroidFontMap s_droid_fonts[] = {
	{"Droid Sans", kMCCustomFontStyleRegular | kMCCustomFontStyleBold},
	{"Droid Sans Mono", kMCCustomFontStyleRegular},
	{"Droid Serif", kMCCustomFontStyleRegular | kMCCustomFontStyleBold | kMCCustomFontStyleItalic | kMCCustomFontStyleBoldItalic},
	{nil, 0}
};

////////////////////////////////////////////////////////////////////////////////

bool MCSystemListFontFamilies(MCListRef& r_names)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
	for (uint32_t i = 0; s_droid_fonts[i].name != nil; i++)
		if (!MCListAppendCString(*t_list, s_droid_fonts[i].name))
			return false;

	MCAutoStringRef t_custom_font_names;
	if (!MCCustomFontListGetNames(&t_custom_font_names) ||
		!MCListAppend(*t_list, *t_custom_font_names))
		return false;

	return MCListCopy(*t_list, r_names);
}

bool MCSystemListFontsForFamily(MCStringRef p_family, uint32_t p_fsize, MCListRef& r_styles)
{
	uint32_t t_styles;
	t_styles = 0;

	for (uint32_t i = 0; s_droid_fonts[i].name != nil; i++)
	{
		if (MCStringIsEqualToCString(p_family, s_droid_fonts[i].name, kMCCompareCaseless))
		{
			t_styles = s_droid_fonts[i].styles;
			break;
		}
	}

	if (t_styles == 0)
		t_styles = MCCustomFontListGetStylesForName(p_family);


	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	if (t_styles & kMCCustomFontStyleRegular)
		if (!MCListAppendCString(*t_list, "plain"))
			return false;
	if (t_styles & kMCCustomFontStyleBold)
		if (!MCListAppendCString(*t_list, "bold"))
			return false;
	if (t_styles & kMCCustomFontStyleItalic)
		if (!MCListAppendCString(*t_list, "italic"))
			return false;
	if (t_styles & kMCCustomFontStyleBoldItalic)
		if (!MCListAppendCString(*t_list, "bold-italic"))
			return false;

	return MCListCopy(*t_list, r_styles);
}

////////////////////////////////////////////////////////////////////////////////

static MCStringRef s_font_folder = nil;

static bool create_custom_font_from_path(MCStringRef p_path, FT_Library p_library, MCCustomFont *&r_custom_font);

void MCAndroidCustomFontsInitialize()
{
	MCCustomFontListInitialize();
	s_font_folder = MCSTR("fonts/");
}

void MCAndroidCustomFontsFinalize()
{
	MCCustomFontListFinalize();
}

// Parse the fonts folder (as set up by the standalone builder), locating any font files.
// For each font file found, attempt to parse using freetype and if successful, add to the custom font list.
// If particularly intensive, the parsing step could be moved to the IDE, which would just generate the required meta-data.
void MCAndroidCustomFontsLoad()
{
	bool t_success;
	t_success = true;
	
	FT_Library t_ft_library;
	t_ft_library = nil;
	if (t_success)
		t_success = FT_Init_FreeType(&t_ft_library) == 0;
	
	MCAutoStringRef t_file_list;
	if (t_success)
	{
		MCAndroidEngineCall("getAssetFolderEntryList", "xx", &(&t_file_list), s_font_folder);
		t_success = *t_file_list != nil;
	}
	
	uindex_t t_offset = 0;
	uindex_t t_old_offset;
	uindex_t t_length = MCStringGetLength(*t_file_list);
	
	while (t_success && t_offset < t_length)
	{
		t_old_offset = t_offset;
		MCAutoStringRef t_file;
		MCAutoStringRef t_is_folder_string;
		bool t_is_folder;
		
		if (!MCStringFirstIndexOfChar(*t_file_list, '\n', t_old_offset, kMCCompareExact, t_offset))
			break;
			
		t_success = MCStringCopySubstring(*t_file_list, MCRangeMakeMinMax(t_old_offset, t_offset), &t_file);
		
		if (t_success)
		{
			if (!MCStringFirstIndexOfChar(*t_file_list, ',', ++t_offset, kMCCompareExact, t_offset))
				break;
			
			t_old_offset = t_offset++;
			
			if (!MCStringFirstIndexOfChar(*t_file_list, '\n', ++t_old_offset, kMCCompareExact, t_offset))
				t_offset = t_length;
			
		   t_success = MCStringCopySubstring(*t_file_list, MCRangeMakeMinMax(t_old_offset, t_offset), &t_is_folder_string);
		}
	
		if (t_success)
			t_success = MCU_stob(*t_is_folder_string, t_is_folder);
		
		if (t_success)
			if (!t_is_folder)
			{
				MCCustomFont *t_font;
				t_font = nil;
				if (create_custom_font_from_path(*t_file, t_ft_library, t_font))
					MCCustomFontListAddFont(t_font);
			}
		t_offset++;
	}
	
	if (t_ft_library != nil)
		FT_Done_FreeType(t_ft_library);
}

////////////////////////////////////////////////////////////////////////////////

static bool load_custom_font_file_into_buffer_from_path(MCStringRef p_path, MCDataRef &r_data)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_font_path;
	
	if (t_success)
		t_success = MCStringFormat(&t_font_path, "%@/%@%@", MCcmd, s_font_folder, p_path);
	
	return MCS_loadbinaryfile(*t_font_path, r_data);
}

static bool create_custom_font_from_path(MCStringRef p_path, FT_Library p_library, MCCustomFont *&r_custom_font)
{
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_font_path;
	
	MCAutoDataRef t_buffer;
	if (t_success)
		t_success = load_custom_font_file_into_buffer_from_path(p_path, &t_buffer);
	
	MCCustomFont *t_font;
	t_font = nil;
	if (t_success)
		t_success = MCMemoryNew(t_font);
	
	if (t_success)
		t_success = MCFreeTypeGetFontInfoFromData(p_library, *t_buffer, t_font->name, t_font->family, t_font->style);

	if (t_success)
	{
		MCValueAssign(t_font->path, p_path);
		r_custom_font = t_font;
	}
	else
		MCCustomFontDelete(t_font);
	
	return t_success;
}

static bool create_font_face_from_custom_font_name_and_style(MCStringRef p_name, bool p_bold, bool p_italic, MCSkiaTypefaceRef &r_typeface)
{
	bool t_success;
	t_success = true;
	
	MCCustomFont *t_font;
	t_font = nil;
	if (t_success)
		t_success = MCCustomFontListLookupFont(p_name, p_bold, p_italic, t_font);
	
	MCAutoDataRef t_buffer;
	if (t_success)
		t_success = load_custom_font_file_into_buffer_from_path(t_font->path, &t_buffer);
	
	MCSkiaTypefaceRef t_typeface;
	t_typeface = nil;
	if (t_success)
		t_success = MCSkiaTypefaceCreateWithData(*t_buffer, t_typeface);
	
	if (t_success)
		r_typeface = t_typeface;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void* MCAndroidCustomFontCreateTypeface(MCStringRef p_name, bool p_bold, bool p_italic)
{
	jobject t_typeface = nil;
	
	MCCustomFont *t_font;
	if (MCCustomFontListLookupFont(p_name, p_bold, p_italic, t_font))
	{
		MCAutoStringRef t_font_path;
		if (!MCStringFormat(&t_font_path, "%@%@", s_font_folder, t_font->path))
			return nullptr;
		MCAndroidEngineCall("createTypefaceFromAsset", "ox", &t_typeface, *t_font_path);
	}

	return t_typeface;
}
////////////////////////////////////////////////////////////////////////////////

void *android_font_create(MCStringRef name, uint32_t size, bool bold, bool italic)
{
	MCSkiaFont *t_font = nil;
	
	if (MCMemoryNew(t_font))
	{
		t_font -> typeface = nil;
		// MM-2012-03-06: Check to see if we have a custom font of the given style and name available
		if (!create_font_face_from_custom_font_name_and_style(name, bold, italic, t_font->typeface))
		{
			// AL-2014-09-10: [[ Bug 13335 ]] If the font does not exist, fall back to the default family
			//  but retain the styling.
			if (!MCSkiaTypefaceCreateWithName(name, bold, italic, t_font->typeface))
				MCSkiaTypefaceCreateWithName(nil, bold, italic, t_font->typeface);
		}
		
		MCAssert(t_font->typeface != NULL);
		t_font->size = size;
	}
	
	return t_font;
}

void android_font_destroy(void *font)
{
	MCSkiaFont *t_font = (MCSkiaFont*)font;
	if (t_font != nil && t_font->typeface != nil)
		MCSkiaTypefaceRelease(t_font->typeface);
	MCMemoryDelete(font);
}

void android_font_get_metrics(void *font, float& a, float& d, float& leading, float& xheight)
{
	MCSkiaFont *t_font = (MCSkiaFont*)font;
	
	/* UNCHECKED */ MCSkiaTypefaceGetMetrics(t_font->typeface, t_font->size, a, d, leading, xheight);
}

float android_font_measure_text(void *p_font, const char *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	MCSkiaFont *t_font = (MCSkiaFont*)p_font;
	
	float t_length;
	if (p_is_unicode)
	{
		/* UNCHECKED */ MCSkiaTypefaceMeasureText(t_font->typeface, t_font->size, p_text, p_text_length, true, t_length);
	}
	else
	{
		MCAutoStringRef t_string;
		MCStringCreateWithCString(p_text, &t_string);
		MCAutoStringRefAsUTF8String t_utf8_string;
		/* UNCHECKED */ t_utf8_string . Lock(*t_string);
		
		/* UNCHECKED */ MCSkiaTypefaceMeasureText(t_font->typeface, t_font->size, *t_utf8_string, t_utf8_string.Size(), false, t_length);
	}
	
	return t_length;
}

////////////////////////////////////////////////////////////////////////////////
