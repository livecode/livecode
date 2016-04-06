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
#ifndef	FLIST_H
#define	FLIST_H

#include "dllst.h"

class MCFontnode : public MCDLlist
{
	MCNewAutoNameRef reqname;
	uint2 reqsize;
	uint2 reqstyle;
	Boolean reqprinter;
	MCFontStruct *font;
public:
	MCFontnode(MCNameRef name, uint2 &size, uint2 style, Boolean printer);
	~MCFontnode();
	MCFontStruct *getfont(MCNameRef fname, uint2 size, uint2 style, Boolean printer);
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
	Boolean isprinterfont()
	{
		return reqprinter;
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
		MCDLlist::splitat((MCDLlist *)node);
	}
	MCFontnode *remove(MCFontnode *&list)
	{
		return (MCFontnode *)MCDLlist::remove((MCDLlist *&)list);
	}
};

class MCFontlist
{
	MCFontnode *fonts;
public:
	MCFontlist();
	~MCFontlist();
	MCFontStruct *getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer);
	void freeprinterfonts();
	bool getfontnames(MCStringRef p_type, MCListRef& r_names);
	bool getfontsizes(MCStringRef p_fname, MCListRef& r_sizes);
	bool getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles);
	bool getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font);
};
#endif
