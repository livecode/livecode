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
extern void *iphone_font_create(const char *name, uint32_t size, bool bold, bool italic);
extern void iphone_font_destroy(void *font);
extern void iphone_font_get_metrics(void *font, float& a, float& d);
#elif defined(TARGET_SUBPLATFORM_ANDROID)
extern void *android_font_create(const char *name, uint32_t size, bool bold, bool italic);
extern void android_font_destroy(void *font);
extern void android_font_get_metrics(void *font, float& a, float& d);
#endif

MCFontnode::MCFontnode(const MCString& p_name, uint2& p_size, uint2 p_style)
{
	reqname = p_name.clone();
	reqsize = p_size;
	reqstyle = p_style;

#if defined(TARGET_SUBPLATFORM_IPHONE)
	font = new MCFontStruct;
	font -> charset = 0;
	font -> size = p_size;
	
	char *t_comma;
	t_comma = strchr(reqname, ',');

	uint1 t_charset;
	t_charset = LCH_ENGLISH;
	if (t_comma != nil)
		t_charset = MCU_languagetocharset(t_comma + 1);
	
	if (t_charset > LCH_ROMAN)
	{
		*t_comma = '\0';
		font -> unicode = True;
		font -> fid = (MCSysFontHandle)iphone_font_create(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
		*t_comma = ',';
	}
	else
	{
		font -> unicode = False;
		font -> fid = (MCSysFontHandle)iphone_font_create(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	
	if (font -> unicode)
		font -> charset = LCH_UNICODE;
	
	font -> ascent = p_size - 1;
	font -> descent = p_size * 2 / 14 + 1;
	
	float ascent, descent;
	iphone_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > p_size)
		font -> ascent++;
	
#elif defined(TARGET_SUBPLATFORM_ANDROID)
	font = new MCFontStruct;
	font -> charset = 0;
	font -> size = p_size;
	
	char *t_comma;
	t_comma = strchr(reqname, ',');

	uint1 t_charset;
	t_charset = LCH_ENGLISH;
	if (t_comma != nil)
		t_charset = MCU_languagetocharset(t_comma + 1);
	
	if (t_charset > LCH_ROMAN)
	{
		*t_comma = '\0';
		font -> unicode = True;
		font -> fid = (MCSysFontHandle)android_font_create(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
		*t_comma = ',';
	}
	else
	{
		font -> unicode = False;
		font -> fid = (MCSysFontHandle)android_font_create(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	}
	
	font -> ascent = p_size - 1;
	font -> descent = p_size * 2 / 14 + 1;
	
	float ascent, descent;
	android_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > p_size)
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
	delete reqname;
	delete font;
}

MCFontStruct *MCFontnode::getfont(const MCString& p_name, uint2 p_size, uint2 p_style)
{
	if (p_name != reqname)
		return NULL;
	if (p_size == 0)
		return font;
	if (p_style != reqstyle || p_size != reqsize)
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

MCFontStruct *MCFontlist::getfont(const MCString &fname, uint2 &size, uint2 style, Boolean printer)
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

extern void MCSystemListFontFamilies(MCExecPoint& ep);
void MCFontlist::getfontnames(MCExecPoint &ep, char *type)
{
	MCSystemListFontFamilies(ep);
}

void MCFontlist::getfontsizes(const char *fname, MCExecPoint &ep)
{
	// MW-2013-03-14: [[ Bug 10744 ]] All fonts are scalable on mobile - so return 0.
	ep . setuint(0);
}

extern void MCSystemListFontsForFamily(MCExecPoint& ep, const char *fname);
void MCFontlist::getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep)
{
	MCSystemListFontsForFamily(ep, fname);
}

// MW-2012-09-19: [[ Bug 10248 ]] Previously unimplemented, this method is required
//   for PDF printing (and normal printing!)
bool MCFontlist::getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
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
