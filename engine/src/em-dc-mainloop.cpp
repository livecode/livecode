/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 Runtime Revolution Ltd.

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
#include "em-util.h"

#include "globdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "variable.h"
#include "osspec.h"
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

/* ================================================================ */

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

	MCswapbytes = MCEmscriptenIsLittleEndian();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;

	/* ---------- Initialise all the things */
	MCS_init();
	MCU_initialize_names();

	if (!MCFontInitialize())
	{
		return false;
	}

	if (!MCLogicalFontTableInitialize())
	{
		return false;
	}

	/* ---------- More globals */

	/* executable file name */
	if (!MCsystem->PathFromNative(argv[0], MCcmd))
	{
		return false;
	}

	/* Locales */
	if (!MCLocaleCreateWithName(MCSTR("en_US"), kMCBasicLocale))
	{
		return false;
	}

	kMCSystemLocale = MCS_getsystemlocale();

	if (nil == kMCSystemLocale)
	{
		return false;
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

	/* ---------- Continue booting... */
	return X_open(argc, argv, envp);
}

bool
X_main_loop_iteration()
{
	MCEmscriptenNotImplemented();
}
