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

#include "execpt.h"
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

MCFontStruct *MCFontlist::getfont(const MCString &fname, uint2 &size, uint2 style, Boolean printer)
{
	return NULL;
}

void MCFontlist::getfontnames(MCExecPoint &ep, char *type)
{
	ep . clear();
}

void MCFontlist::getfontsizes(const char *fname, MCExecPoint &ep)
{
	ep . clear();
}

void MCFontlist::getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep)
{
	ep . clear();
}

bool MCFontlist::getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	return false;
}
