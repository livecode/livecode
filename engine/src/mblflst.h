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

#ifndef __MC_MOBILE_FONTLIST__
#define __MC_MOBILE_FONTLIST__

#ifndef __MC_DLLST__
#include "dllst.h"
#endif

class MCFontnode : public MCDLlist
{
	MCNewAutoNameRef reqname;
	uint2 reqsize;
	uint2 reqstyle;
	MCFontStruct *font;
public:
	MCFontnode(MCNameRef fname, uint2 &size, uint2 style);
#ifdef TARGET_SUBPLATFORM_IPHONE
    MCFontnode(MCSysFontHandle, MCNameRef name);
#endif
	~MCFontnode();

	MCFontStruct *getfont(MCNameRef fname, uint2 size, uint2 style);

	MCFontStruct *getfontstruct()
	{
		return font;
	}

	MCNameRef getname()
	{
		return *reqname;
	}
	uint2 getsize()
	{
		return reqsize;
	}
	uint2 getstyle()
	{
		return reqstyle;
	}
	MCFontnode *next()
	{
		return (MCFontnode *)MCDLlist::next();
	}
	MCFontnode *prev()
	{
		return (MCFontnode *)MCDLlist::prev();
	}
	void totop(MCFontnode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCFontnode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCFontnode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCFontnode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCFontnode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCFontnode *remove
	(MCFontnode *&list)
	{
		return (MCFontnode *)MCDLlist::remove
        ((MCDLlist *&)list);
	}
};

class MCFontlist
{
	MCFontnode *fonts;

public:
	MCFontlist();
	~MCFontlist();

	MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer);

	bool getfontnames(MCStringRef p_type, MCListRef& r_names);
	bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes);
	bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles);
	bool getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font);
    
#ifdef TARGET_SUBPLATFORM_IPHONE
    MCFontStruct *getfontbyhandle(MCSysFontHandle, MCNameRef name);
#endif
};


#endif
