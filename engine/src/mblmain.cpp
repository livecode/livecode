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
#include "card.h"
#include "tooltip.h"
#include "osspec.h"
#include "redraw.h"
#include "system.h"
#include "font.h"
#include "debug.h"
#include "eventqueue.h"

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

MCTheme *MCThemeCreateNative(void)
{
	return nil;
}

static bool
X_initialize_mccmd(const X_init_options& p_options)
{
    return MCsystem->PathFromNative(p_options.argv[0],
                                    MCcmd);
}

/* Compute the app code path - on mobile this is the folder containing
 * the executable. */
static bool
X_initialize_mcappcodepath(const X_init_options& p_options)
{
    MCSAutoLibraryRef t_self;
    MCAutoStringRef t_lib_path;
    return MCSLibraryCreateWithAddress(reinterpret_cast<void *>(X_initialize_mcappcodepath),
                                       &t_self) &&
            MCSLibraryCopyPath(*t_self,
                               &t_lib_path) &&
            MCU_path_split(*t_lib_path,
                           &MCappcodepath,
                           nullptr);
}

bool X_init(const X_init_options& p_options)
{
    int argc = p_options.argc;
    MCStringRef *argv = p_options.argv;
    MCStringRef *envp = p_options.envp;

	X_clear_globals();
	
	////	
	
	MCswapbytes = byte_swapped();
	MCtruemcstring = MCtruestring;
	MCfalsemcstring = MCfalsestring;
	MCnullmcstring = MCnullmcstring;
	
	MCS_init();
	
    X_initialize_mccmd(p_options);
    
    X_initialize_mcappcodepath(p_options);
    
	////

	X_initialize_names();
	
	// MW-2012-02-23: [[ FontRefs ]] Initialize the font module.
	MCFontInitialize();
	// MW-2012-02-23: [[ FontRefs ]] Initialize the logical font table module.
	MCLogicalFontTableInitialize();

	// Initialize the event queue
	MCEventQueueInitialize();

    // MM-2014-02-10: [[ LipOpenSSL 1.0.1e ]] Attempt load revsecurity library on Java side.
#if defined(TARGET_SUBPLATFORM_ANDROID)
	extern bool revandroid_loadExternalLibrary(MCStringRef p_external, MCStringRef& r_filename);
    MCAutoStringRef t_filename;
    revandroid_loadExternalLibrary(MCSTR("revsecurity"), &t_filename);
#endif
    
	////
	
    // Create the basic locale and the system locale
    if (!MCLocaleCreateWithName(MCSTR("en_US"), kMCBasicLocale))
        return false;
    kMCSystemLocale = MCS_getsystemlocale();
    if (kMCSystemLocale == nil)
        return false;
    
	// Create the $<n> variables.
	for(uint32_t i = 2; i < argc; ++i)
		if (argv[i] != nil)
			create_var(argv[i]);
	create_var(nvars);

	////
	
	return X_open(argc, argv, envp);
}

bool X_main_loop_iteration(void)
{
	if (!MCscreen->hasmessages() && MCstacks->isempty() && MCnsockets == 0)
	{
		MCquit = True;
		return false;
	}
	MCscreen->wait(MCmaxwait, True, True);
	MCU_resetprops(True);
	// MW-2011-08-26: [[ Redraw ]] Make sure we flush any updates.
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
	
	if (MCquit)
		return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
