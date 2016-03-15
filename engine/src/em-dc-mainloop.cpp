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

#include "em-dc-mainloop.h"
#include "em-dc.h"
#include "em-standalone.h"
#include "em-util.h"

#include "globals.h"
#include "variable.h"
#include "osspec.h"
#include "tooltip.h"
#include "stacklst.h"
#include "redraw.h"
#include "system.h"
#include "font.h"
#include "util.h"

#include <arpa/inet.h>

/* ================================================================ */

/* Functions that incomprehensibly fail to appear in any header
 * files. */

bool X_open(int argc, MCStringRef argv[], MCStringRef envp[]);
void X_clear_globals(void);
void MCU_initialize_names(void);

/* ================================================================
 * Platform-specific initialisation
 * ================================================================ */

/* FIXME these functions are pretty much the same for every X_init()
 * implementation. */

/* Returns 0 on big-endian platforms, 1 on little-endian */
static Boolean
/*byte_swapped*/MCEmscriptenIsLittleEndian(void)
{
	return (1 != htons(1)) ? 1 : 0;
}

/* Create a $<N> variable for a numbered command-line argument
 * string */
static bool
/*create_var*/MCEmscriptenCreateArgVar(MCStringRef p_arg, uint32_t p_number)
{
	MCAutoStringRef t_name_str;
	MCVariable *t_variable;
	MCNewAutoNameRef t_name;

	if (!MCStringFormat(&t_name_str, "$%d", p_number))
	{
		return false;
	}

	if (!MCNameCreate(*t_name_str, &t_name))
	{
		return false;
	}

	if (!MCVariable::ensureglobal(*t_name, t_variable))
	{
		return false;
	}

	t_variable->setvalueref(p_arg);

	MCU_realloc((char **)&MCstacknames,
	            MCnstacks,
	            MCnstacks + 1,
	            sizeof(MCStringRef));

	MCstacknames[MCnstacks++] = MCValueRetain(p_arg);

	return true;
}

/* Create the $# variable (number of command line arguments) */
static bool
/*create_var*/MCEmscriptenCreateArgCountVar(uint32_t p_number)
{
	MCVariable *t_variable;
	if (!MCVariable::ensureglobal(MCNAME("$#"), t_variable))
	{
		return false;
	}

	t_variable->setnvalue(p_number);

	return true;
}

/* ================================================================ */

bool
X_init(int argc,
       MCStringRef argv[],
       int envc,
       MCStringRef envp[])
{
	/* ---------- Set up global variables */
	X_clear_globals();

	/* No blocking during initialization */
	++MCwaitdepth;

	MCswapbytes = MCEmscriptenIsLittleEndian();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;

	/* On Emscripten, we let the engine sleep indefinitely in the
	 * absence on incoming events. */
	MCmaxwait = INFINITY;

	/* ---------- Initialise all the things */
	MCS_init();
	MCU_initialize_names();

	if (!MCFontInitialize())
	{
		goto error_cleanup;
	}

	if (!MCLogicalFontTableInitialize())
	{
		goto error_cleanup;
	}

	/* ---------- More globals */

	/* executable file name */
	if (!MCsystem->PathFromNative(argv[0], MCcmd))
	{
		goto error_cleanup;
	}

	/* Locales */
	if (!MCLocaleCreateWithName(MCSTR("en_US"), kMCBasicLocale))
	{
		goto error_cleanup;
	}

	kMCSystemLocale = MCS_getsystemlocale();

	if (nil == kMCSystemLocale)
	{
		goto error_cleanup;
	}

	/* ---------- argv[] global variables */
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] != nil)
		{
			MCEmscriptenCreateArgVar(argv[i], i);
		}
	}
	MCEmscriptenCreateArgCountVar(argc);

	/* ---------- Initialise the filesystem */

	/* We have to unpack the standalone data here rather than in
	 * MCDispatch::startup() because we have to have font data
	 * available in the VFS before calling X_open(). */
	if (!MCEmscriptenStandaloneUnpack())
	{
		goto error_cleanup;
	}

	/* ---------- Continue booting... */
	if (!X_open(argc, argv, envp))
	{
		goto error_cleanup;
	}

	--MCwaitdepth;
	return true;

 error_cleanup:
	--MCwaitdepth;
	return false;
}

/* ================================================================
 * Emscripten platform main loop
 * ================================================================ */

bool
X_main_loop_iteration()
{
	/* Check if the engine is in a runnable state */
	if (!MCscreen->hasmessages() &&
	    MCstacks->isempty() &&
	    0 == MCnsockets)
	{
		MCquit = true;
		return false;
	}

	/* Process pending events */
	MCscreen->wait(MCmaxwait, true, true);

	/* Since the main loop returned to the top level, make sure to
	 * reset any locks and perform any pending redraws.  This can only
	 * be handled here, not in MCScreenDC::wait(). */
	MCU_resetprops(true);
	MCRedrawUpdateScreen();

	MCabortscript = false;

	MCU_cleaninserted();

	MCscreen->siguser();

	MCdefaultstackptr = MCstaticdefaultstackptr;

	MCS_alarm(0);

	return !MCquit;
}
