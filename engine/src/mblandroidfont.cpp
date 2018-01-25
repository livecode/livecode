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
#include "mblandroidtypeface.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsnames.h>

////////////////////////////////////////////////////////////////////////////////

typedef uint32_t MCAndroidFontStyle;
enum
{
	kMCAndroidFontStyleRegular = 1 << 0,
	kMCAndroidFontStyleBold = 1 << 1,
	kMCAndroidFontStyleItalic = 1 << 2,
    kMCAndroidFontStyleBoldItalic = 1 << 3    
};

////////////////////////////////////////////////////////////////////////////////

// For the moment the Droid * font details are hard coded in (used to populate the fontNames and fontStyles properties).
// The built in font support is limited by the Skia library, so we don't support any custom fonts the user may have installed.
// None the less, it might be a better idea to get the built in font names dynamically (if possible).

struct MCAndroidDroidFontMap
{
    const char *name;
    MCAndroidFontStyle styles;
};

static const MCAndroidDroidFontMap s_droid_fonts[] = {
    {"Droid Sans", kMCAndroidFontStyleRegular | kMCAndroidFontStyleBold},
    {"Droid Sans Mono", kMCAndroidFontStyleRegular},
    {"Droid Serif", kMCAndroidFontStyleRegular | kMCAndroidFontStyleBold | kMCAndroidFontStyleItalic | kMCAndroidFontStyleBoldItalic},
    {nil, 0}
};

////////////////////////////////////////////////////////////////////////////////

static bool MCAndroidCustomFontsList(MCStringRef& r_font_list);
static MCAndroidFontStyle MCAndroidCustomFontsGetStyle(MCStringRef p_name);

bool MCSystemListFontFamilies(MCListRef& r_names)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;
    for (uint32_t i = 0; s_droid_fonts[i].name != nil; i++)
		if (!MCListAppendCString(*t_list, s_droid_fonts[i].name))
			return false;
    
    MCAutoStringRef t_custom_font_names;
    if (!MCAndroidCustomFontsList(&t_custom_font_names) ||
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
        t_styles = MCAndroidCustomFontsGetStyle(p_family);
    
    
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

    if (t_styles & kMCAndroidFontStyleRegular)
        if (!MCListAppendCString(*t_list, "plain"))
            return false;
    if (t_styles & kMCAndroidFontStyleBold)
        if (!MCListAppendCString(*t_list, "bold"))
            return false;
    if (t_styles & kMCAndroidFontStyleItalic)
        if (!MCListAppendCString(*t_list, "italic"))
            return false;
    if (t_styles & kMCAndroidFontStyleBoldItalic)
        if (!MCListAppendCString(*t_list, "bold-italic"))
            return false;
    
	return MCListCopy(*t_list, r_styles);
}

////////////////////////////////////////////////////////////////////////////////

struct MCAndroidCustomFont {
    MCStringRef path;
    MCStringRef name;
    MCStringRef family;
    MCAndroidFontStyle style;
    MCAndroidCustomFont *next;
};

static MCAndroidCustomFont *s_custom_fonts = nil;
static MCStringRef s_font_folder = nil;

static MCAndroidCustomFont* look_up_custom_font_by_name(MCStringRef p_name);
static MCAndroidCustomFont* look_up_custom_font_by_family_and_style(MCStringRef p_family, bool p_bold, bool p_italic);
static MCAndroidCustomFont* look_up_custom_font(MCStringRef p_name, bool p_bold, bool p_italic);
static bool create_custom_font_from_path(MCStringRef p_path, FT_Library p_library, MCAndroidCustomFont *&r_custom_font);
static void delete_custom_font(MCAndroidCustomFont *p_font);

void MCAndroidCustomFontsInitialize()
{
    s_custom_fonts = nil;
    s_font_folder = MCSTR("fonts/");
}

void MCAndroidCustomFontsFinalize()
{
    for (MCAndroidCustomFont *t_font = s_custom_fonts; t_font != nil; )
    {
        MCAndroidCustomFont *t_next_font;
        t_next_font = t_font->next;
        delete_custom_font(t_font);
        t_font = t_next_font;
    }
    s_custom_fonts = nil;    
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
    
    MCAndroidCustomFont *t_last_font;
    t_last_font = s_custom_fonts;
    
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
                MCAndroidCustomFont *t_font;
                t_font = nil;
                if (create_custom_font_from_path(*t_file, t_ft_library, t_font))
                {
                    if (t_last_font == nil)
                        s_custom_fonts = t_font;
                    else
                        t_last_font->next = t_font;
                    t_last_font = t_font;
                }                
            }
        t_offset++;
	}
    
    if (t_ft_library != nil)
        FT_Done_FreeType(t_ft_library);
}

static bool MCAndroidCustomFontsList(MCStringRef& r_font_list)
{
    bool t_success;
    t_success = true;
    
    MCAutoListRef t_list;
    if (!MCListCreateMutable('\n', &t_list))
		return false;
    
    for (MCAndroidCustomFont *t_font = s_custom_fonts; t_success && t_font != nil; t_font = t_font->next)
        t_success = MCListAppend(*t_list, t_font->name);
    
    if (t_success)
        return MCListCopyAsString(*t_list, r_font_list);
    
    return false;
}

static MCAndroidFontStyle MCAndroidCustomFontsGetStyle(MCStringRef p_name)
{
    MCAndroidFontStyle t_styles;
    t_styles = 0;
    
    for (MCAndroidCustomFont *t_font = s_custom_fonts; t_font != nil; t_font = t_font->next)
    {
        if (MCStringIsEqualTo(p_name, t_font->family, kMCCompareCaseless))
            t_styles |= t_font->style;
    }
    
    if (t_styles == 0)
    {
        MCAndroidCustomFont *t_font;
        t_font = look_up_custom_font_by_name(p_name);
        if (t_font != nil)
            t_styles |= t_font->style;
    }
    
    return t_styles;    
}

////////////////////////////////////////////////////////////////////////////////

static MCAndroidCustomFont* look_up_custom_font(MCStringRef p_name, bool p_bold, bool p_italic)
{
    MCAutoStringRef t_styled_name;
    if (!MCStringMutableCopy(p_name, &t_styled_name))
        return nil;    
    if (p_bold && !MCStringAppend(*t_styled_name, MCSTR(" Bold")))
        return nil;
    if (p_italic && !MCStringAppend(*t_styled_name, MCSTR(" Italic")))
        return nil;    
    
    // Fist of all, attempt to look the font up by taking into account its name and style.
    // e.g. textFont:Arial textStyle:Bold - look for a font named Arial Bold.
    // This will fail for textFonts which include style information e.g. textFont:Arial textStyle:Bold will search for Arial Bold Bold
    MCAndroidCustomFont *t_font;
    t_font = look_up_custom_font_by_name(*t_styled_name);
  
    if (t_font != nil)
        return t_font;
    
    // If no font found, look up based purley on the name.  This will solve cases where style
    // information is included in the name e.g. Arial Bold.
    if (p_bold || p_italic)
    {
        t_font = look_up_custom_font_by_name(p_name);
        if (t_font != nil)
            return t_font;
    }
    
    // If we've still not found a matching font, look up based on the family and style.
    // This function will attempt to provide a closest match e.g. Arial Bold is requested but only Arial is installed.
    t_font = look_up_custom_font_by_family_and_style(p_name, p_bold, p_italic);
   return t_font;
}

static MCAndroidCustomFont* look_up_custom_font_by_name(MCStringRef p_name)
{
    for (MCAndroidCustomFont *t_font = s_custom_fonts; t_font != nil; t_font = t_font->next)
    {
        if (MCStringIsEqualTo(p_name, t_font->name, kMCCompareCaseless))
            return t_font;
    }
    return nil;
}

static MCAndroidCustomFont* look_up_custom_font_by_family_and_style(MCStringRef p_family, bool p_bold, bool p_italic)
{
    MCAndroidCustomFont *t_closest_font;
    t_closest_font = nil;
    for (MCAndroidCustomFont *t_font = s_custom_fonts; t_font != nil; t_font = t_font->next)
    {
        if (MCStringIsEqualTo(p_family, t_font->family, kMCCompareCaseless))
        {
            if ((p_bold && p_italic && t_font->style == kMCAndroidFontStyleBoldItalic) || 
                (p_bold && t_font->style == kMCAndroidFontStyleBold) || 
                (p_italic && t_font->style == kMCAndroidFontStyleItalic))
                return t_font;
            else if (t_closest_font == nil)
                t_closest_font = t_font;
        }
    }
    return t_closest_font;
}

static void delete_custom_font(MCAndroidCustomFont *p_font)
{
    if (p_font != nil)
    {
        /*UNCHECKED */ MCValueRelease(p_font->path);
        /*UNCHECKED */ MCValueRelease(p_font->name);
        /*UNCHECKED */ MCValueRelease(p_font->family);
        /*UNCHECKED */ MCMemoryDelete(p_font);
    }
}

static bool load_custom_font_file_into_buffer_from_path(MCStringRef p_path, char *&r_buffer, uint32_t &r_size)
{     
    bool t_success;
    t_success = true;
    
    MCAutoStringRef t_font_path;
    
    if (t_success)
        t_success = MCStringFormat(&t_font_path, "%@/%@%@", MCcmd, s_font_folder, p_path);
    
    if (t_success)
        t_success = MCS_exists(*t_font_path, true);
    
    IO_handle t_font_file_handle;
    t_font_file_handle = nil;
    if (t_success)
	{
        t_font_file_handle = MCS_open(*t_font_path, kMCOpenFileModeRead, false, false, 0);
		t_success = t_font_file_handle != nil;
	}
    
    uint32_t t_file_size;
    t_file_size = 0;
    char *t_buffer;
    t_buffer = nil;
	if (t_success)
	{
		t_file_size = MCS_fsize(t_font_file_handle);
		t_success = MCMemoryAllocate(t_file_size + 1, t_buffer);
	}
    
	if (t_success)
        if (MCS_readfixed(t_buffer, t_file_size, t_font_file_handle) != IO_NORMAL)
            t_success = false;
    
    if (t_success)
    {
        r_buffer = t_buffer;
        r_size = t_file_size;
    }
    else
        /*UNCHECKED */ MCMemoryDelete(t_buffer);
    
    return t_success;
}

static bool create_custom_font_from_path(MCStringRef p_path, FT_Library p_library, MCAndroidCustomFont *&r_custom_font)
{
    bool t_success;
    t_success = true;
    
    char *t_buffer;
    t_buffer = nil;
    uint32_t t_file_size;
    t_file_size = 0;
    if (t_success)
        t_success = load_custom_font_file_into_buffer_from_path(p_path, t_buffer, t_file_size);
    
    FT_Face t_font_face;
    t_font_face = nil;
    if (t_success)
        t_success = FT_New_Memory_Face(p_library, (FT_Byte *)t_buffer, t_file_size, 0, &t_font_face) == 0;
    
    MCAndroidCustomFont *t_font;
    t_font = nil;
    if (t_success)
        t_success = MCMemoryNew(t_font);
    
    if (t_success)
        MCValueAssign(t_font->path, p_path);
    
    if (t_success)
        t_success = MCStringCreateWithCString(t_font_face->family_name, t_font->family);
    
    if (t_success)
    {
        if (MCCStringEqualCaseless(t_font_face->style_name, "bold"))
            t_font->style = kMCAndroidFontStyleBold;
        else if (MCCStringEqualCaseless(t_font_face->style_name, "italic"))
            t_font->style = kMCAndroidFontStyleItalic;
        else if (MCCStringEqualCaseless(t_font_face->style_name, "bold italic"))
            t_font->style = kMCAndroidFontStyleBoldItalic;
        else
            t_font->style = kMCAndroidFontStyleRegular;
    }
    
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
                t_success = MCStringCreateWithNativeChars((char_t *)t_sft_name.string, t_sft_name.string_len, t_font->name);
                break;
            }
        }
    }
    
    if (t_success)
        t_success = t_font->name != nil;
    
    if (t_success)
        r_custom_font = t_font;
    else
        delete_custom_font(t_font);    
    
    if (t_font_face != nil)
        FT_Done_Face(t_font_face);
    
    /*UNCHECKED */ MCMemoryDelete(t_buffer);
    
    return t_success;
}

static bool create_font_face_from_custom_font_name_and_style(MCStringRef p_name, bool p_bold, bool p_italic, MCAndroidTypefaceRef &r_typeface)
{    
    bool t_success;
    t_success = true;
    
    MCAndroidCustomFont *t_font;
    t_font = nil;
    if (t_success)
    {
        t_font = look_up_custom_font(p_name, p_bold, p_italic);
        t_success = t_font != nil;
    }
    
    char *t_buffer;
    t_buffer = nil;
    uint32_t t_file_size;
    t_file_size = 0;
    if (t_success)
        t_success = load_custom_font_file_into_buffer_from_path(t_font->path, t_buffer, t_file_size);
    
	MCAndroidTypefaceRef t_typeface;
    t_typeface = nil;
    if (t_success)
		t_success = MCAndroidTypefaceCreateWithData(t_buffer, t_file_size, t_typeface);
    
    if (t_success)
        r_typeface = t_typeface;
    else
        MCMemoryDelete(t_buffer);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void* MCAndroidCustomFontCreateTypeface(MCStringRef p_name, bool p_bold, bool p_italic)
{
    jobject t_typeface = nil;
    
    MCAndroidCustomFont *t_font = look_up_custom_font(p_name, p_bold, p_italic);
    if (t_font != nil)
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
	MCAndroidFont *t_font = nil;
    
	if (MCMemoryNew(t_font))
	{
        t_font -> typeface = nil;
        // MM-2012-03-06: Check to see if we have a custom font of the given style and name available
        if (!create_font_face_from_custom_font_name_and_style(name, bold, italic, t_font->typeface))
        {
            MCAutoStringRefAsCString t_name;
            /* UNCHECKED */ t_name . Lock(name);
            
            // AL-2014-09-10: [[ Bug 13335 ]] If the font does not exist, fall back to the default family
            //  but retain the styling.
			if (!MCAndroidTypefaceCreateWithName(*t_name, bold, italic, t_font->typeface))
                MCAndroidTypefaceCreateWithName(nil, bold, italic, t_font->typeface);
        }
        
		MCAssert(t_font->typeface != NULL);
		t_font->size = size;
	}
    
	return t_font;
}

void android_font_destroy(void *font)
{
	MCAndroidFont *t_font = (MCAndroidFont*)font;
	if (t_font != nil && t_font->typeface != nil)
		MCAndroidTypefaceRelease(t_font->typeface);
	MCMemoryDelete(font);
}

void android_font_get_metrics(void *font, float& a, float& d, float& leading, float& xheight)
{
	MCAndroidFont *t_font = (MCAndroidFont*)font;
    
	/* UNCHECKED */ MCAndroidTypefaceGetMetrics(t_font->typeface, t_font->size, a, d, leading, xheight);
}

float android_font_measure_text(void *p_font, const char *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	MCAndroidFont *t_font = (MCAndroidFont*)p_font;
    
	float t_length;
	if (p_is_unicode)
	{
		/* UNCHECKED */ MCAndroidTypefaceMeasureText(t_font->typeface, t_font->size, p_text, p_text_length, true, t_length);
	}
	else
	{
        MCAutoStringRef t_string;
        MCStringCreateWithCString(p_text, &t_string);
        MCAutoStringRefAsUTF8String t_utf8_string;
        /* UNCHECKED */ t_utf8_string . Lock(*t_string);
        
        /* UNCHECKED */ MCAndroidTypefaceMeasureText(t_font->typeface, t_font->size, *t_utf8_string, t_utf8_string.Size(), false, t_length);
	}
	
	return t_length;
}

////////////////////////////////////////////////////////////////////////////////
