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

//
// MCFontlist class functions
//

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "osspec.h"
#include "osxflst.h"

extern void *coretext_font_create_with_name_size_and_style(const char *p_name, uint32_t p_size, bool p_bold, bool p_italic);
extern void coretext_font_destroy(void *p_font);
extern void coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent);
extern void coretext_get_font_names(MCExecPoint &ep);
extern void core_text_get_font_styles(const char *p_name, uint32_t p_size, MCExecPoint &ep);
extern void coretext_get_font_name(void *p_font, char*& r_name);
extern uint32_t coretext_get_font_size(void *p_font);

#define MAX_XFONT2MACFONT    11

typedef struct _x2MacFont
{
	const char *Xfontname;
	const char *Macfontname;
}
X2MacFontTable;

static X2MacFontTable XMfonts[MAX_XFONT2MACFONT] = { // X to Mac font table
            { "Charter", "Courier" },
            { "Clean", "Courier" },
            { "fixed", "Chicago" },
            { "Helvetica", "Geneva" },
            { "Lucida", "Courier" },
            { "LucidaBright", "Courier" },
            { "LucidaTypewriter", "Courier" },
            { "MS Sans Serif", "Geneva" },
            { "New Century Schoolbook", "Times" },
            { "System", "Charcoal" },
            { "terminal", "Courier" }
        };

MCFontnode::MCFontnode(const MCString& fname, uint2& size, uint2 style)
{
	reqname = fname . clone();
	reqsize = size;
	reqstyle = style;
	
	font = new MCFontStruct;
	font -> size = size;
	
	char *t_comma;
	t_comma = strchr(reqname, ',');
	if (t_comma != nil)
		*t_comma = '\0';
	
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
    font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	
    // if font does not exist then find MAC equivalent of X font name
    if (font -> fid == NULL)
	{
		for (uint2 i = 0 ; i < MAX_XFONT2MACFONT ; i++)
			if (fname == XMfonts[i] . Xfontname)
			{
                // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
                font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(XMfonts[i] . Macfontname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
				break;
			}
	}
    
    calculatemetrics();
}

MCFontnode::MCFontnode(MCSysFontHandle p_handle)
{
    coretext_get_font_name(p_handle, reqname);
    reqsize = coretext_get_font_size(p_handle);
    reqstyle = FA_DEFAULT_STYLE;
    
    font = new MCFontStruct;
    font->size = reqsize;
    
    font->fid = p_handle;
    
    calculatemetrics();
}

void MCFontnode::calculatemetrics()
{
	font -> ascent = reqsize - 1;
	font -> descent = reqsize * 2 / 14 + 1;
	
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	float ascent, descent;
	coretext_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > reqsize)
		font -> ascent++;
}

MCFontnode::~MCFontnode()
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	coretext_font_destroy(font -> fid);
	delete reqname;
	delete font;
}

MCFontStruct *MCFontnode::getfont(const MCString &fname, uint2 size, uint2 style)
{
	if (fname != reqname)
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

MCFontStruct *MCFontlist::getfont(const MCString &fname, uint2 &size,
                                  uint2 style, Boolean printer)
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

MCFontStruct *MCFontlist::getfontbyhandle(MCSysFontHandle p_fid)
{
    MCFontnode *tmp = fonts;
    if (tmp != NULL)
        do
        {
            MCFontStruct *font = tmp->getfontstruct();
            if (font->fid == p_fid)
                return font;
            tmp = tmp->next();
        }
        while (tmp != fonts);
    
    // Font has not yet been added to the list
    tmp = new MCFontnode(p_fid);
    tmp->appendto(fonts);
    return tmp->getfontstruct();
}

void MCFontlist::getfontnames(MCExecPoint &ep, char *type)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    coretext_get_font_names(ep);
}

void MCFontlist::getfontsizes(const char *fname, MCExecPoint &ep)
{
    // MM-2014-06-02: [[ CoreText ]] Assume all core text fonts are scaleable.
	ep . clear();
	ep . concatuint(0, EC_RETURN, true);
}

void MCFontlist::getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    core_text_get_font_styles(fname, fsize, ep);
}

bool MCFontlist::getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	MCFontnode *t_font = fonts;
	do
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
    while (t_font != fonts);
	return false;
}