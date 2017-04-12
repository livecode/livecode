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
#include "objdefs.h"
#include "parsedef.h"

#include "param.h"
#include "mcerror.h"

#include "util.h"
#include "object.h"

#include "w32dc.h"

int4 MCScreenDC::getsoundvolume(void)
{
	return 0;
}

void MCScreenDC::setsoundvolume(int4 p_volume)
{
}

void MCScreenDC::startplayingsound(IO_handle p_stream, MCObject *p_callback, bool p_next, int p_volume)
{
}

void MCScreenDC::stopplayingsound(void)
{
}
