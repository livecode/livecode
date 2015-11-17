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
// List of currently loaded fonts
//
#ifndef	FONTLIST_H
#define	FONTLIST_H

#include "dllst.h"

#include "textlayout.h"

#include <pango/pangoft2.h>

class MCFontlist
{
public:
	virtual ~MCFontlist(void) {};
	virtual void destroy(void) = 0;

	virtual MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer) = 0;
	virtual bool getfontnames(MCStringRef p_type, MCListRef& r_names) = 0;
	virtual bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes) = 0;
	virtual bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles) = 0;
	virtual bool getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font) = 0;
	virtual void getfontreqs(MCFontStruct *f, MCNameRef& r_name, uint2& r_size, uint2& r_style) = 0;

	virtual int4 ctxt_textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override) = 0;
	virtual bool ctxt_layouttext(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context) = 0;
};

MCFontlist *MCFontlistCreateOld(void);
MCFontlist *MCFontlistCreateNew(void);

MCFontlist *MCFontlistGetCurrent(void);

struct MCNewFontStruct: public MCFontStruct
{
	// The requested details of the font
	MCNameRef family;
	uint16_t style;
	
	// The pango description
	PangoFontDescription *description;
	
	// The link to the next one.
	MCNewFontStruct *next;
};

#endif
