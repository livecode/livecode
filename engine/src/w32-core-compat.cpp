/* Copyright (C) 2016 LiveCode Ltd.

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

#include "globals.h"
#include "uidc.h"

////////////////////////////////////////////////////////////////////////////////
// MCPlatform* compatability functions
//   Implementation of MCPlatform* functions required for MCPlatformPlayer implementation.

void MCPlatformBreakWait(void)
{
	MCscreen->pingwait();
}

bool MCPlatformWaitForEvent(double duration, bool blocking)
{
	bool t_dispatch = !blocking;
	return MCscreen->wait(duration, t_dispatch, false);
}

////////////////////////////////////////////////////////////////////////////////
