/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "variable.h"
#include "dispatch.h"
#include "osspec.h"

#include "stacksecurity.h"

/* ================================================================
 * Emscripten standalone startup logic
 * ================================================================ */

#define kMCEmscriptenBootStackFilename "/boot/standalone/__boot.livecode"
#define kMCEmscriptenStartupScriptFilename "/boot/__startup.livecodescript"

IO_stat
MCStackSecurityEmscriptenStartup(void)
{
	/* Load & run the startup script in a temporary stack */
	MCStack t_temporary_stack;

	MCAutoStringRef t_startup_script;
	if (!MCS_loadtextfile(MCSTR(kMCEmscriptenStartupScriptFilename),
	                      &t_startup_script))
	{
		MCresult->sets("failed to read startup script");
		return IO_ERROR;
	}

	if (ES_NORMAL != t_temporary_stack.domess(*t_startup_script))
	{
		MCresult->sets("failed to execute startup script");
		return IO_ERROR;
	}

	/* Load the initial stack */
	MCStack *t_stack;
	if (IO_NORMAL != MCdispatcher->loadfile(MCSTR(kMCEmscriptenBootStackFilename),
	                                        t_stack))
	{
		MCresult->sets("failed to read initial stackfile");
		return IO_ERROR;
	}

	MCdefaultstackptr = MCstaticdefaultstackptr = t_stack;

	return IO_NORMAL;
}
