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

#include <libscript/script.h>

#include "globdefs.h"
#include "parsedef.h"
#include "scriptpt.h"
#include "globals.h"
#include "variable.h"
#include "stack.h"
#include "dispatch.h"

#include <unistd.h>

/* ================================================================
 * Emscripten engine start-up
 * ================================================================ */

static void
MCEmscriptenBootError(const char *p_message)
{
	MCLog("Startup error: %s", p_message);
	exit(99);
}

MC_DLLEXPORT_DEF int
platform_main(int argc, char *argv[], char *envp[])
{
	/* ---------- Core initialisation */
	/* Record the address of the bottom of the stack */
	{
		char magic_stack_base = 0;
		MCstackbottom = &magic_stack_base;
	}

	if (!MCInitialize())
	{
		MCEmscriptenBootError("Core initialisation");
	}
    if (!MCSInitialize())
    {
		MCEmscriptenBootError("Core System initialisation");
    }
	if (!MCScriptInitialize())
	{
		MCEmscriptenBootError("LCB VM initialisation");
	}

	/* ---------- Process command-line arguments.
	 * Emscripten usually passes fairly meaningless values here. */

	MCStringRef *t_argv;
	if (!MCMemoryNewArray(argc, t_argv))
	{
		MCEmscriptenBootError("arguments");
	}
	for (int i = 0; i < argc; ++i)
	{
		MCLog("args: %i: %s", i, argv[i]);

		/* FIXME should probably be UTF-8 */
		if (!MCStringCreateWithSysString(argv[i], t_argv[i]))
		{
			MCEmscriptenBootError("argument import");
		}
	}

	/* ---------- Process the environment */
	/* Count env variables */
	int t_envc;
	for (t_envc = 0; envp != nil && envp[t_envc] != nil; ++t_envc);

	/* Import. Note that the envp array is null-terminated */
	MCStringRef *t_envp;
	if (!MCMemoryNewArray(t_envc + 1, t_envp))
	{
		MCEmscriptenBootError("environment");
	}

	for (int i = 0; i < t_envc; ++i)
	{
		MCLog("env: %s", envp[i]);

		/* FIXME should probably be UTF-8 */
		if (!MCStringCreateWithSysString(envp[i], t_envp[i]))
		{
			MCEmscriptenBootError("environment import");
		}
	}

	t_envp[t_envc] = nil; /* null-terminated */

	/* ---------- Engine boot */
    struct X_init_options t_options;
    t_options.argc = argc;
    t_options.argv = t_argv;
    t_options.envp = t_envp;
    t_options.app_code_path = nullptr;
    
	MCresult = nil;
	if (!X_init(t_options))
	{
		MCStringRef t_string = nil;
		if (MCresult != nil)
		{
			MCExecContext ctxt(nil, nil, nil);
			MCAutoValueRef t_result;

			MCresult->eval(ctxt, &t_result);
			ctxt.ConvertToString(*t_result, t_string);
		}

		MCAutoStringRefAsSysString t_message;
		if (nil != t_string &&
		    t_message.Lock(t_string))
		{
			MCEmscriptenBootError(*t_message);
		}
		else
		{
			MCEmscriptenBootError("unknown boot failure");
		}
	}

	/* ---------- Clean up arguments & environment */
	for (int i = 0; i < argc; ++i)
	{
		MCValueRelease(t_argv[i]);
	}
	MCMemoryDeleteArray(t_argv);

	for (int i = 0; i < t_envc; ++i)
	{
		MCValueRelease(t_envp[i]);
	}
	MCMemoryDeleteArray(t_envp);

	/* ---------- Prepare to enter main loop */
	if (!MCquit)
	{
		MCdispatcher->gethome()->open();
	}

	/* ---------- Main loop */
	while (X_main_loop_iteration());

	/* ---------- Shutdown */
	int t_exit_code = X_close();

	MCFinalize();

	exit(t_exit_code);
}
