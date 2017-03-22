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
#include "stacksecurity.h"
#include "system.h"

#include "libscript/script.h"

#include "platform.h"
#include "platform-legacy.h"

////////////////////////////////////////////////////////////////////////////////

static uint2 nvars;

static void create_var(MCStringRef p_var)
{
	MCAutoStringRef t_vname;
	/* UNCHECKED */ MCStringFormat(&t_vname, "$%d", nvars++);
	
	MCVariable *tvar;
	MCNewAutoNameRef t_name;
	/* UNCHECKED */ MCNameCreate(*t_vname, &t_name);
	/* UNCHECKED */ MCVariable::ensureglobal(*t_name, tvar);
	tvar->setvalueref(p_var);

	MCU_realloc((char **)&MCstacknames, MCnstacks, MCnstacks + 1, sizeof(MCStringRef));
	MCstacknames[MCnstacks++] = MCValueRetain(p_var);
}

static void create_var(uint4 p_v)
{
	MCVariable *tvar;
	/* UNCHECKED */ MCVariable::ensureglobal(MCNAME("$#"), tvar);
	tvar->setnvalue(p_v);
}

static Boolean byte_swapped()
{
	uint2 test = 1;
	return *((uint1 *)&test);
}

////////////////////////////////////////////////////////////////////////////////

bool X_open(int argc, MCStringRef argv[], MCStringRef envp[]);
extern void X_clear_globals(void);
extern void MCU_initialize_names();

bool X_init(int argc, MCStringRef argv[], MCStringRef envp[])
{
    void *t_bottom;
    MCstackbottom = (char *)&t_bottom;
	
#ifdef _WINDOWS_DESKTOP
	// MW-2011-07-26: Make sure errno pointer is initialized - this won't be
	//   if the engine is running through the plugin.
	extern int *g_mainthread_errno;
	if (g_mainthread_errno == nil)
		g_mainthread_errno = _errno();
#endif

	////
	
	X_clear_globals();
	
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
	MCperror = new (nothrow) MCError();
	MCeerror = new (nothrow) MCError();
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

    // Create the basic locale and the system locale
    if (!MCLocaleCreateWithName(MCSTR("en_US"), kMCBasicLocale))
        return false;
    kMCSystemLocale = MCS_getsystemlocale();
    if (kMCSystemLocale == nil)
        return false;
		
	MCSCommandLineSetName (argv[0]);

	if (MCModeIsExecutableFirstArgument())
		create_var(argv[0]);

	/* This list will be used to set the argument list returned by
	 * MCSCommandLineGetArguments(). */
	MCAutoProperListRef t_arguments;
	/* UNCHECKED */ MCProperListCreateMutable (&t_arguments);

    MCAutoStringRefAsUTF8String t_mccmd_utf8;
    /* UNCHECKED */ t_mccmd_utf8 . Lock(MCcmd);
    
	for (int i = 1; i < argc; i++)
	{
		if (MCStringIsEqualToCString(argv[i], "-d", kMCCompareExact)
		   || MCStringIsEqualToCString(argv[i], "-display", kMCCompareExact))
		{
			if (++i >= argc || MCStringGetCharAtIndex(argv[i], 0) == '-')
			{
				fprintf(stderr, "%s: bad display name\n", *t_mccmd_utf8);
				return False;
			}
            /* UNCHECKED */ MCStringConvertToCString(argv[i], MCdisplayname);
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "-f", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-files", kMCCompareExact))

		{
			MCnofiles = True;
			MCsecuremode = MC_SECUREMODE_ALL;
			continue;
		}

		/* TODO Remove -g,-geometry flag because it's not used any more */
		if (MCStringIsEqualToCString(argv[i], "-g", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-geometry", kMCCompareExact))
		{
			if (++i >= argc)
			{
				fprintf(stderr, "%s: bad geometry\n", *t_mccmd_utf8);
				return False;
			}
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "-m", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-mmap", kMCCompareExact))
		{
			MCmmap = False;
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "-n", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-nopixmaps", kMCCompareExact))
		{
			MCnopixmaps = True;
			continue;
		}
			
		if (MCStringIsEqualToCString(argv[i], "-x", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-xftoff", kMCCompareExact))
		{
			MCuseXft = False;
			continue;
		}
		
#ifdef _MAC_DESKTOP
		if (MCStringIsEqualToCString(argv[i], "-psn", kMCCompareExact))
		{
			// psn passed, skip it
			continue;
		}
#endif

		if (MCStringIsEqualToCString(argv[i], "-s", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-sharedoff", kMCCompareExact))
		{
			MCshmoff = True;
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "+s", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "+sharedon", kMCCompareExact))
		{
			MCshmon = True;
			continue;
		}

		if (MCStringIsEqualToCString(argv[i], "-u", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-ui", kMCCompareExact))		{
			MCnoui = True;
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "-v", kMCCompareExact)
			|| MCStringIsEqualToCString(argv[i], "-visualid", kMCCompareExact))
		{
			uint4 visualid = 0;
			if (++i >= argc || MCStringGetCharAtIndex(argv[i], 0) == '-' || !MCU_stoui4(argv[i], visualid))
			{
				fprintf(stderr, "%s: bad visual id\n", *t_mccmd_utf8);
				return False;
			}
			MCvisualid = visualid;
			continue;
		}
		
		if (MCStringIsEqualToCString(argv[i], "-h", kMCCompareExact)
			&& MCglobals == NULL)
		{
            MCAutoPointer<char> t_MCN_version;
            /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(MCN_version_string), &t_MCN_version);
			fprintf(stderr, "LiveCode %s Copyright 2003-2015 LiveCode Ltd\n\
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
			        [stackname(s) | argument(s)]\n",  *t_MCN_version, *t_mccmd_utf8);

			return False;
		}
		
		create_var(argv[i]);
		/* UNCHECKED */ MCProperListPushElementOntoBack (*t_arguments, argv[i]);
	}
	create_var(nvars);
	/* UNCHECKED */ MCSCommandLineSetArguments (*t_arguments);

	if (!X_open(argc, argv, envp))
		return false;

	if (MCstacknames != NULL && MCModeShouldLoadStacksOnStartup())
	{
		for(int i = 0 ; i < MCnstacks ; i++)
		{
			MCStack *sptr;
			if (MCdispatcher->loadfile(MCstacknames[i], sptr) == IO_NORMAL)
				sptr->open();
            MCValueRelease(MCstacknames[i]);
		}
        MCnstacks = 0;
        delete[] MCstacknames; /* Allocated with new[] */
		MCstacknames = NULL;
		MCeerror->clear();
	}
	
	return true;
}

void X_main_loop_iteration()
{
    void *t_bottom;
    MCstackbottom = (char *)&t_bottom;

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
	if (MCtracedobject)
	{
		MCtracedobject->message(MCM_trace_done);
		MCtracedobject = nil;
	}
    if (!MCtodestroy -> isempty())
    {
        MCtooltip -> cleartip();
        MCtodestroy -> destroy();
    }
	MCU_cleaninserted();
	MCscreen->siguser();
	MCdefaultstackptr = MCstaticdefaultstackptr;
	MCS_alarm(0.0);
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
    extern bool MCS_mac_elevation_bootstrap_main(int argc, char* argv[]);
    if (argc == 2 && strcmp(argv[1], "-elevated-slave") == 0)
        if (!MCS_mac_elevation_bootstrap_main(argc, argv))
            return -1;
    
    if (!MCInitialize() ||
            !MCSInitialize() ||
            !MCScriptInitialize())
        exit(-1);
    
    if (MCPlatformInitialize())
    {
        return MCplatform -> Run(argc, argv, envp);
    }
    
    MCScriptFinalize();
    MCFinalize();
    
    return -1;
}


////////////////////////////////////////////////////////////////////////////////
