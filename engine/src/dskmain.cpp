/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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
#include "mcio.h"

#include "mcerror.h"
#include "globals.h"
#include "variable.h"
#include "util.h"
#include "mode.h"
#include "securemode.h"
#include "dispatch.h"
#include "stacklst.h"
#include "stack.h"
#include "debug.h"
#include "card.h"
#include "tooltip.h"
#include "osspec.h"
#include "redraw.h"
#include "font.h"

////////////////////////////////////////////////////////////////////////////////

static uint2 nvars;

static void create_var(char *v)
{
	char vname[U2L + 1];
	sprintf(vname, "$%d", nvars);
	nvars++;
	
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal_cstring(vname, tvar);
	tvar->copysvalue(v);

	MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(MCStringRef));
	/* UNCHECKED */ MCStringCreateWithCString(v, MCstacknames[MCnstacks++]);
}

static void create_var(uint4 p_v)
{
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal_cstring("$#", tvar);
	tvar->setnvalue(p_v);
}

static Boolean byte_swapped()
{
	uint2 test = 1;
	return *((uint1 *)&test);
}

////////////////////////////////////////////////////////////////////////////////

bool X_open(int argc, char *argv[], char *envp[]);
extern void MCU_initialize_names();

static char apppath[PATH_MAX];

bool X_init(int argc, char *argv[], char *envp[])
{
	int i;
	MCstackbottom = (char *)&i;

#ifdef _WINDOWS_DESKTOP
	// MW-2011-07-26: Make sure errno pointer is initialized - this won't be
	//   if the engine is running through the plugin.
	extern int *g_mainthread_errno;
	if (g_mainthread_errno == nil)
		g_mainthread_errno = _errno();
#endif

	////

#ifndef _WINDOWS_DESKTOP
	MCS_init();
#endif
	
	////
	
	MCU_initialize_names();

	////
	
	// MW-2012-02-23: [[ FontRefs ]] Initialize the font module.
	MCFontInitialize();
	// MW-2012-02-23: [[ FontRefs ]] Initialize the logical font table module.
	MCLogicalFontTableInitialize();
	
	////

	MCswapbytes = byte_swapped();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;
	MCnullmcstring = MCnullmcstring;

#ifdef _WINDOWS_DESKTOP
	// Make sure certain things are already created before MCS_init. This is
	// required right now because the Windows MCS_init uses MCS_query_registry
	// which needs these things.
	MCperror = new MCError();
	MCeerror = new MCError();
	/* UNCHECKED */ MCVariable::create(MCresult);
#endif

#ifdef _WINDOWS_DESKTOP
	MCS_init();
#endif

#ifdef _WINDOWS_DESKTOP
	delete MCperror;
	delete MCeerror;
	delete MCresult;
#endif
	
	/* UNCHECKED */ MCStringCreateWithCString(argv[0], MCcmd);

#if defined(_MAC_DESKTOP)
	/* UNCHECKED */ MCStringCreateWithBytes((byte_t*)argv[0], strlen(argv[0]), kMCStringEncodingUTF8, MCcmd);
#endif
		
#if defined(_LINUX_DESKTOP) || defined(_MAC_DESKTOP)   //get fullpath
	{
      MCStringRef t_resolved_cmd;
      MCS_resolvepath(MCcmd, t_resolved_cmd);
      MCValueAssign(MCcmd, t_resolved_cmd);
	}
#endif

	if (MCModeIsExecutableFirstArgument())
		create_var(argv[0]);

	for (int i = 1; i < argc; i++)
	{
		if (strnequal(argv[i], "-d", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-display", 8) && strlen(argv[i]) == 8)
		{
			if (++i >= argc || *argv[i] == '-')
			{
				fprintf(stderr, "%s: bad display name\n", MCStringGetCString(MCcmd));
				return False;
			}
			MCdisplayname = argv[i];
			continue;
		}
		if (strnequal(argv[i], "-f", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-files", 6) && strlen(argv[i]) == 6)
		{
			MCnofiles = True;
			MCsecuremode = MC_SECUREMODE_ALL;
			continue;
		}
		if (strnequal(argv[i], "-g", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-geometry", 9) && strlen(argv[i]) == 9)
		{
			char *geometry = NULL;
			if (++i >= argc)
			{
				fprintf(stderr, "%s: bad geometry\n", MCStringGetCString(MCcmd));
				return False;
			}
			geometry = argv[i];
			continue;
		}
		if (strnequal(argv[i], "-m", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-mmap", 5) && strlen(argv[i]) == 5)
		{
			MCmmap = False;
			continue;
		}
		if (strnequal(argv[i], "-n", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-nopixmaps", 10) && strlen(argv[i]) == 10)
		{
			MCnopixmaps = True;
			continue;
		}
		if (strnequal(argv[i], "-x", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-xftoff", 7) && strlen(argv[i]) == 7)
		{
			MCuseXft = False;
			continue;
		}
		
#ifdef _MAC_DESKTOP
		if (strnequal(argv[i], "-psn", 4))
		{
			// psn passed, skip it
			continue;
		}
#endif

		if (strnequal(argv[i], "-s", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-sharedoff", 10) && strlen(argv[i]) == 10)
		{
			MCshmoff = True;
			continue;
		}
		if (strnequal(argv[i], "+s", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "+sharedon", 9) && strlen(argv[i]) == 9)
		{
			MCshmon = True;
			continue;
		}

		if (strnequal(argv[i], "-u", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-ui", 3) && strlen(argv[i]) == 3)
		{
			MCnoui = True;
			continue;
		}
		if (strnequal(argv[i], "-v", 2) && strlen(argv[i]) == 2
		        || strnequal(argv[i], "-visualed", 9) && strlen(argv[i]) == 9)
		{
			uint4 visualid = 0;
			if (++i >= argc || *argv[i] == '-' || !MCU_stoui4(argv[i], visualid))
			{
				fprintf(stderr, "%s: bad visual id\n", MCStringGetCString(MCcmd));
				return False;
			}
			MCvisualid = visualid;
			continue;
		}
		if (strnequal(argv[i], "-h", 2) && strlen(argv[i]) == 2
		        && MCglobals == NULL)
		{
			fprintf(stderr, "Revolution %s Copyright 2003-2008 Runtime Revolution Ltd\n\
			        Usage: %s [-d[isplay] displayname] \n\
			        [-f[iles] (disable access to files and processes)\n\
			        [-g[eometry] ={+-}<xoffset>{+-}<yoffset>]\n\
			        [-i[nput] fd] read commands from fd (0 is stdin) or named pipe\n\
			        [-m[map]] (don't memory map files)\n\
			        [-n[opixmaps]] (draw direct to screen)\n\
			        [-p[ointerfocus]] (use active focus)\n\
			        [-s[haredoff]] (don't use shared memory server extension)\n\
			        [+s[haredon]] (use shared memory server extension)\n\
			        [-u[i]] (don't create graphical user interface)\n\
			        [-v[isualid] n] (use visual id n as listed from xdpyinfo)\n\
			        [-w[indowid] n] (watch window id n for commands)\n\
			        [stackname(s) | argument(s)]\n", MCNameGetCString(MCN_version_string), MCStringGetCString(MCcmd));
			return False;
		}
		if (argv[i] != NULL && strlen(argv[i]) > 0)
			create_var(argv[i]);
	}
	create_var(nvars);

	// If we have 'no ui' make sure we also have noxft
	if (MCnoui)
		MCuseXft = False;

	if (!X_open(argc, argv, envp))
		return false;

	if (MCstacknames != NULL && MCModeShouldLoadStacksOnStartup())
	{
		for(int i = 0 ; i < MCnstacks ; i++)
		{
			MCStack *sptr;
			if (MCdispatcher->loadfile(MCStringGetCString(MCstacknames[i]), sptr) == IO_NORMAL)
				sptr->open();
		}
		delete MCstacknames;
		MCstacknames = NULL;
		MCeerror->clear();
	}
	
	return true;
}

void X_main_loop_iteration()
{
	int i;
	MCstackbottom = (char *)&i;

	////

	if (MCiconicstacks == 0 && !MCscreen->hasmessages() && MCstacks->isempty() && MCnsockets == 0)
	{
		// MW-2005-11-01: We want to keep the result here so we call with send=True
		//   (the result is used by the development environment bootstrap code)
		MCdefaultstackptr->getcard()->message(MCM_shut_down, (MCParameter *)NULL, True, True);
		MCquit = True;
		return;
	}
	MCscreen->wait(MCmaxwait, True, True);
	MCU_resetprops(True);
	// MW-2011-08-19: [[ Redraw ]] Make sure we flush any updates.
	MCRedrawUpdateScreen();
	MCabortscript = False;
	if (MCtracedobject != NULL)
	{
		MCtracedobject->message(MCM_trace_done);
		MCtracedobject = NULL;
	}
	if (!MCtodestroy->isempty() || MCtodelete != NULL)
	{
		MCtooltip->cleartip();
		while (MCtodelete != NULL)
		{
			MCObject *optr = MCtodelete->remove(MCtodelete);
			delete optr;
		}
		MCtodestroy->destroy();
	}
	MCU_cleaninserted();
	MCscreen->siguser();
	MCdefaultstackptr = MCstaticdefaultstackptr;
	MCS_alarm(0.0);
}

////////////////////////////////////////////////////////////////////////////////
