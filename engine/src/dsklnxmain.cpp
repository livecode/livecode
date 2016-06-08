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

bool X_init(int argc, MCStringRef argv[], MCStringRef envp[]);
void X_main_loop_iteration();
int X_close();

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while(!MCquit)
		X_main_loop_iteration();
}

extern "C" bool MCModulesInitialize();
extern "C" void MCModulesFinalize();

int platform_main(int argc, char *argv[], char *envp[])
{
	// On Linux, the argv and envp could be in pretty much any format. The
	// safest thing to do is let the C library's iconv convert to a known
	// format. To do this, the system locale needs to be retrieved.
	setlocale(LC_ALL, "");
	MCsysencoding = strclone(nl_langinfo(CODESET));
	
	if (!MCInitialize() || !MCSInitialize() ||
	    !MCModulesInitialize() || !MCScriptInitialize())
		exit(-1);
    
	// Convert the argv array to StringRefs
	MCStringRef* t_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_argv);
	for (int i = 0; i < argc; i++)
	{
        /* UNCHECKED */ MCStringCreateWithSysString(argv[i], t_argv[i]);
	}
	
	// Convert the envp array to StringRefs
	int envc = 0;
	MCStringRef* t_envp = nil;
	while (envp[envc] != NULL)
	{
		uindex_t t_count = envc + 1;
		/* UNCHECKED */ MCMemoryResizeArray(t_count + 1, t_envp, t_count);
        /* UNCHECKED */ MCStringCreateWithSysString(envp[envc], t_envp[envc]);
		envc++;
	}
	
	// Terminate the envp array
	t_envp[envc] = nil;
	
	extern int MCSystemElevatedMain(int, char* argv[]);
	if (argc == 3&& strcmp(argv[1], "-elevated-slave") == 0)
		return MCSystemElevatedMain(argc, argv);
	
	if (!X_init(argc, t_argv, t_envp))
    {
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
		exit(-1);
	}
	
	// Clean up the created argv/envp StringRefs
	for (int i = 0; i < argc; i++)
		MCValueRelease(t_argv[i]);
	for (int i = 0; i < envc; i++)
		MCValueRelease(t_envp[i]);
	MCMemoryDeleteArray(t_argv);
	MCMemoryDeleteArray(t_envp);
	
	X_main_loop();
	
	int t_exit_code = X_close();

	MCScriptFinalize();
	MCModulesFinalize();
	MCFinalize();

	exit(t_exit_code);
}
