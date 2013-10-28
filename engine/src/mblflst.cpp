/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "font.h"
#include "util.h"
#include "globals.h"
#include "unicode.h"

#include "mblflst.h"

#if defined(TARGET_SUBPLATFORM_IPHONE)
extern void *iphone_font_create(MCStringRef name, uint32_t size, bool bold, bool italic);
extern void iphone_font_destroy(void *font);
extern void iphone_font_get_metrics(void *font, float& a, float& d);
#elif defined(TARGET_SUBPLATFORM_ANDROID)
extern void *android_font_create(const char *name, uint32_t size, bool bold, bool italic);
extern void android_font_destroy(void *font);
extern void android_font_get_metrics(void *font, float& a, float& d);
#endif

MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style)
{
	reqname = fname;
	reqsize = size;
	reqstyle = style;
    
#if defined(TARGET_SUBPLATFORM_IPHONE)
	font = new MCFontStruct;
	font -> charset = 0;
	font -> size = size;
	
	uindex_t t_comma;
	MCAutoStringRef reqname_str;
	reqname_str = MCNameGetString(*reqname);
	Boolean t_success;
	t_success = MCStringFirstIndexOfChar(*reqname_str, ',', 0, kMCCompareExact, t_comma);

	uint1 t_charset;
	t_charset = LCH_ENGLISH;
	if (t_success)
    {
        MCAutoStringRef t_after_comma;
        /* UNCHECKED */ MCStringCopySubstring(*reqname_str, MCRangeMake(t_comma + 1, MCStringGetLength(*reqname_str) - (t_comma + 1)), &t_after_comma);
		t_charset = MCU_languagetocharset(*t_after_comma);
    }
	
	if (t_charset > LCH_ROMAN)
	{
		 MCAutoStringRef t_before_comma;
        /* UNCHECKED */ MCStringCopySubstring(*reqname_str, MCRangeMake(0, t_comma - 1), &t_before_comma);
		font -> unicode = True;

		font -> fid = (MCSysFontHandle)iphone_font_create(*t_before_comma, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	else
	{
		font -> unicode = False;

		font -> fid = (MCSysFontHandle)iphone_font_create(*reqname_str, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	
	if (font -> unicode)
		font -> charset = LCH_UNICODE;
	
	font -> ascent = size - 1;
	font -> descent = size * 2 / 14 + 1;
	
	float ascent, descent;
	iphone_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > size)
		font -> ascent++;
    
#elif defined(TARGET_SUBPLATFORM_ANDROID)
	font = new MCFontStruct;
	font -> charset = 0;
	font -> size = size;
	
	uindex_t t_comma;
	MCAutoStringRef reqname_str;
	reqname_str = MCNameGetString(*reqname);
	Boolean t_success;
	t_success = MCStringFirstIndexOfChar(*reqname_str, ',', 0, kMCCompareExact, t_comma);

	uint1 t_charset;
	t_charset = LCH_ENGLISH;
	if (t_success)
    {
        MCAutoStringRef t_after_comma;
        /* UNCHECKED */ MCStringCopySubstring(*reqname_str, MCRangeMake(t_comma + 1, MCStringGetLength(*reqname_str) - (t_comma + 1)), &t_after_comma);
		t_charset = MCU_languagetocharset(*t_after_comma);
    }
	
	if (t_charset > LCH_ROMAN)
	{
		MCAutoStringRef t_before_comma;
        /* UNCHECKED */ MCStringCopySubstring(*reqname_str, MCRangeMake(0, t_comma - 1), &t_before_comma);
		font -> unicode = True;
		font -> fid = (MCSysFontHandle)android_font_create(MCStringGetCString(*t_before_comma), reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	
	else
	{
		font -> unicode = False;
		font -> fid = (MCSysFontHandle)android_font_create(MCStringGetCString(*reqname_str), reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	
	font -> ascent = size - 1;
	font -> descent = size * 2 / 14 + 1;
	
	float ascent, descent;
	android_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > size)
		font -> ascent++;
	
#endif
}

MCFontnode::~MCFontnode(void)
{
#if defined(TARGET_SUBPLATFORM_IPHONE)
	iphone_font_destroy(font -> fid);
#elif defined(TARGET_SUBPLATFORM_ANDROID)
	android_font_destroy(font->fid);
#endif
	delete font;
}

MCFontStruct *MCFontnode::getfont(MCNameRef fname, uint2 size, uint2 style)
{
	if (!MCNameIsEqualTo(fname, *reqname))
		return NULL;
	if (size == 0)
		return font;
	if (style != reqstyle || size != reqsize)
		return NULL;
	return font;
}

MCFontlist::MCFontlist()
{
	fonts = NULL;
}

MCFontlist::~MCFontlist()
{
	while (fonts != NULL)
	{
		MCFontnode *fptr = fonts->remove
		                   (fonts);
		delete fptr;
	}
}

MCFontStruct *MCFontlist::getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
	MCFontnode *tmp = fonts;
	if (tmp != NULL)
		do
		{
			MCFontStruct *font = tmp->getfont(fname, size, style);
			if (font != NULL)
				return font;
			tmp = tmp->next();
		}
		while (tmp != fonts);
	tmp = new MCFontnode(fname, size, style);
	tmp->appendto(fonts);
	return tmp->getfont(fname, size, style);
}

extern bool MCSystemListFontFamilies(MCListRef& r_names);
bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
	return MCSystemListFontFamilies(r_names);
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
	// MW-2013-03-14: [[ Bug 10744 ]] All fonts are scalable on mobile - so return 0.
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list) || !MCListAppendInteger(*t_list, 0))
		return false;

	return MCListCopy(*t_list, r_sizes);
}

extern bool MCSystemListFontsForFamily(MCStringRef p_family, MCListRef& r_styles);
bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
	return MCSystemListFontsForFamily(p_fname, r_styles);
}

bool MCFontlist::getfontstructinfo(MCNameRef&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	MCFontnode *t_font = fonts;
	while (t_font != NULL)
	{
		if (t_font->getfontstruct() == p_font)
		{
			r_name = t_font->getname();
			r_size = t_font->getsize();
			r_style = t_font->getstyle();
			return true;
		}
		t_font = t_font->next();
	}
	return false;
}
