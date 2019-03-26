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

//
// MCFontlist class functions
//

#include "osxprefix.h"
#include "osxprefix-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"


#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "osspec.h"
#include "osxflst.h"

extern void *coretext_font_create_with_name_size_and_style(MCStringRef p_name, uint32_t p_size, bool p_bold, bool p_italic);
extern bool coretext_font_destroy(void *p_font);
extern bool coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent, float& r_leading, float& r_xheight);
extern bool coretext_get_font_names(MCListRef &r_names);
extern bool core_text_get_font_styles(MCStringRef p_name, uint32_t p_size, MCListRef &r_styles);
extern void coretext_get_font_name(void *p_font, MCNameRef& r_name);
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

MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style)
{
    reqname = MCValueRetain(fname);
	reqsize = size;
	reqstyle = style;
    
    uinteger_t t_comma_index;
    MCAutoStringRef t_reqname;
	font = new (nothrow) MCFontStruct; //create MCFont structure
	font -> size = size;
	
    if (MCStringFirstIndexOfChar(MCNameGetString(fname), ',', 0, kMCStringOptionCompareExact, t_comma_index))
        MCStringCopySubstring(MCNameGetString(fname), MCRangeMake(0, t_comma_index), &t_reqname);
    else
        MCStringCopy(MCNameGetString(fname), &t_reqname);
	
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
    font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(*t_reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	
    // if font does not exist then find MAC equivalent of X font name
    if (font -> fid == NULL)
	{
		for (uint2 i = 0 ; i < MAX_XFONT2MACFONT ; i++)
			if (MCStringIsEqualToCString(MCNameGetString(fname), XMfonts[i] . Xfontname, kMCStringOptionCompareExact))
			{
                // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
                font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(MCNameGetString(fname), reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
				break;
			}
	}
    
    calculatemetrics();
}

MCFontnode::MCFontnode(MCSysFontHandle p_handle, MCNameRef p_name)
{
    if (p_name == nil)
        coretext_get_font_name(p_handle, reqname);
    else
        reqname = MCValueRetain(p_name);
    
    reqsize = coretext_get_font_size(p_handle);
    reqstyle = FA_DEFAULT_STYLE | FA_SYSTEM_FONT;
    
    font = new (nothrow) MCFontStruct;
    font->size = reqsize;
    
    font->fid = p_handle;
    
    calculatemetrics();
}

void MCFontnode::calculatemetrics()
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
    coretext_font_get_metrics(font -> fid, font -> m_ascent, font -> m_descent, font -> m_leading, font -> m_xheight);
}

MCFontnode::~MCFontnode()
{
    MCValueRelease(reqname);
    
    /* The actual font handle for system fonts is cached by the theme related
     * code on Mac so don't release it here. */
    if ((reqstyle & FA_SYSTEM_FONT) == 0)
    {
        // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
        coretext_font_destroy(font -> fid);
    }
    
    delete font;
}

MCFontStruct *MCFontnode::getfont(MCNameRef fname, uint2 size, uint2 style)
{
	if (reqstyle & FA_SYSTEM_FONT)
        return NULL;
    if (!MCNameIsEqualToCaseless(fname, reqname))
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
		MCFontnode *fptr = fonts->remove(fonts);
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
	tmp = new (nothrow) MCFontnode(fname, size, style);
	tmp->appendto(fonts);
	return tmp->getfont(fname, size, style);
}

MCFontStruct *MCFontlist::getfontbyhandle(MCSysFontHandle p_fid, MCNameRef p_name)
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
    tmp = new (nothrow) MCFontnode(p_fid, p_name);
    tmp->appendto(fonts);
    return tmp->getfontstruct();
}

bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    return coretext_get_font_names(r_names);
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
    // MM-2014-06-02: [[ CoreText ]] Assume all core text fonts are scaleable.
	r_sizes = MCValueRetain(kMCEmptyList);
	return true;
}

bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    return core_text_get_font_styles(p_fname, fsize, r_styles);
}

bool MCFontlist::getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
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
