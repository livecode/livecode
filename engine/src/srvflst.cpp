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
#include "parsedef.h"
#include "objdefs.h"

//#include "execpt.h"
#include "font.h"
#include "util.h"
#include "globals.h"

#include "srvflst.h"

MCFontnode::MCFontnode(const MCString& p_name, uint2& p_size, uint2 p_style)
{
}

MCFontnode::~MCFontnode(void)
{
}

MCFontStruct *MCFontnode::getfont(const MCString& p_name, uint2 p_size, uint2 p_style)
{
	return NULL;
}

MCFontlist::MCFontlist()
{
}

MCFontlist::~MCFontlist()
{
}

MCFontStruct *MCFontlist::getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
	return NULL;
}

bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
	r_names = MCValueRetain(kMCEmptyList);
	return true;
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
	r_sizes = MCValueRetain(kMCEmptyList);
	return true;
}

bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
	r_styles = MCValueRetain(kMCEmptyList);
	return true;
}

bool MCFontlist::getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	return false;
}
