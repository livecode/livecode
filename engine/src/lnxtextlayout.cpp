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

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "filedefs.h"

#include "util.h"
#include "globals.h"

#include "textlayout.h"
#include "lnxflst.h"

bool MCTextLayoutInitialize(void)
{
	return true;
}

void MCTextLayoutFinalize(void)
{
}

bool MCTextLayout(const unichar_t *p_chars, uint32_t p_char_count, MCFontStruct *p_font, MCTextLayoutCallback p_callback, void *p_context)
{
	return MCFontlistGetCurrent() -> ctxt_layouttext(p_chars, p_char_count, p_font, p_callback, p_context);
}
