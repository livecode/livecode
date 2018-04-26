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
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"


#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "variable.h"
#include "libscript/script.h"

#include <locale.h>
#include <langinfo.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" void initialise_required_weak_link_glib();

void X_main_loop(void)
{
	while(!MCquit)
		X_main_loop_iteration();
}

int platform_main(int argc, char *argv[], char *envp[])
{
	// On Linux, the argv and envp could be in pretty much any format. The
	// safest thing to do is let the C library's iconv convert to a known
	// format. To do this, the system locale needs to be retrieved.
	setlocale(LC_ALL, "");
	MCsysencoding = strclone(nl_langinfo(CODESET));
		
	if (!MCInitialize())
	{
		fprintf(stderr, "Fatal: initialization failed\n");
		exit(-1);
	}
	
	if (!MCSInitialize())
	{
		fprintf(stderr, "Fatal: platform initialization failed\n");
		exit(-1);
	}
	
	if (!MCScriptInitialize())
	{
		fprintf(stderr, "Fatal: script initialization failed\n");
		exit(-1);
	}
	
	// Linux needs the platform layer to be initialised early so that it can
	// use it to load the weakly-linked dynamic libraries that the engine
	// depends on.
	MCS_preinit();
		
	// Core initialisation complete; 
	// This depends on libFoundation and MCsystem being initialised first
	initialise_required_weak_link_glib();
	
	// Convert the argv array to StringRefs
    MCAutoStringRefArray t_argv;
    /* UNCHECKED */ t_argv.New(argc);
	for (int i = 0; i < argc; i++)
	{
        /* UNCHECKED */ MCStringCreateWithSysString(argv[i], t_argv[i]);
	}
	
	// Convert the envp array to StringRefs
    int envc = 0;
    while (envp[envc] != nullptr)
        ++envc;
    MCAutoStringRefArray t_envp;
    /* UNCHECKED */ t_envp.New(envc + 1);
    for (int i = 0; envp[i] != nullptr; ++i)
    {
        /* UNCHECKED */ MCStringCreateWithSysString(envp[i], t_envp[i]);
    }

	// Terminate the envp array
	t_envp[envc] = nil;
	
	extern int MCSystemElevatedMain(int, char* argv[]);
	if (argc == 3&& strcmp(argv[1], "-elevated-slave") == 0)
		return MCSystemElevatedMain(argc, argv);
	
    struct X_init_options t_options;
    t_options.argc = argc;
    t_options.argv = *t_argv;
    t_options.envp = *t_envp;
    t_options.app_code_path = nullptr;
	if (!X_init(t_options))
    {
		// Try to print an informative error message or, failing that, just
		// report that an error occurred.
		if (MCresult != nil)
        {
            MCExecContext ctxt(nil, nil, nil);
            MCAutoValueRef t_result;
            MCAutoStringRef t_string;
            MCresult -> eval(ctxt, &t_result);
            ctxt . ConvertToString(*t_result, &t_string);
            MCAutoStringRefAsSysString t_autostring;
            /* UNCHECKED */ t_autostring . Lock(*t_string);
            fprintf(stderr, "Startup error - %s\n", *t_autostring);
		}
		else
		{
			fprintf(stderr, "Fatal: unknown startup error\n");
		}
		
		exit(-1);
	}
	
	X_main_loop();
	
	int t_exit_code = X_close();

	MCScriptFinalize();
	MCFinalize();

	exit(t_exit_code);
}
