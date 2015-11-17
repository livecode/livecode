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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"

//#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "script.h"

#include <unistd.h>
#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, MCStringRef argv[], MCStringRef envp[]);
void X_main_loop_iteration();
int X_close();

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while(!MCquit)
	{
		NSAutoreleasePool *t_pool;
		t_pool = [[NSAutoreleasePool alloc] init];
		X_main_loop_iteration();
		[t_pool release];
	}
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
	extern bool MCS_mac_elevation_bootstrap_main(int argc, char* argv[]);
	if (argc == 2 && strcmp(argv[1], "-elevated-slave") == 0)
		if (!MCS_mac_elevation_bootstrap_main(argc, argv))
			return -1;
	
	// MW-2011-08-18: [[ Bug ]] Make sure we initialize Cocoa on startup.
	NSApplicationLoad();
	
	if (!MCInitialize() || !MCSInitialize() || !MCScriptInitialize())
		exit(-1);
	
	// On OSX, argv and envp are encoded as UTF8
	MCStringRef *t_new_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_new_argv);
	
	for (int i = 0; i < argc; i++)
	{
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)argv[i], strlen(argv[i]), kMCStringEncodingUTF8, false, t_new_argv[i]);
	}
	
	MCStringRef *t_new_envp;
	/* UNCHECKED */ MCMemoryNewArray(1, t_new_envp);
	
	int i = 0;
	uindex_t t_envp_count = 0;
	
	while (envp[i] != NULL)
	{
		t_envp_count++;
		uindex_t t_count = i;
		/* UNCHECKED */ MCMemoryResizeArray(i + 1, t_new_envp, t_count);
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)envp[i], strlen(envp[i]), kMCStringEncodingUTF8, false, t_new_envp[i]);
		i++;
	}
	
	/* UNCHECKED */ MCMemoryResizeArray(i + 1, t_new_envp, t_envp_count);
	t_new_envp[i] = nil;
		
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	bool t_init;
	t_init = X_init(argc, t_new_argv, t_new_envp);
	
	for (i = 0; i < argc; i++)
		MCValueRelease(t_new_argv[i]);
	for (i = 0; i < t_envp_count; i++)
		MCValueRelease(t_new_envp[i]);
	
	MCMemoryDeleteArray(t_new_argv);
	MCMemoryDeleteArray(t_new_envp);
	
	if (!t_init)
	{
		[t_pool release];
		
		MCAutoStringRef t_caption;
		MCAutoStringRef t_text;
		MCModeGetStartupErrorMessage(&t_caption, &t_text);
        MCAutoStringRefAsUTF8String t_utf8_caption, t_utf8_text;
        /* UNCHECKED */ t_utf8_caption . Lock(*t_caption);
        /* UNCHECKED */ t_utf8_text . Lock(*t_text);
		fprintf(stderr, "%s - %s\n", *t_utf8_caption, *t_utf8_text);
		exit(-1);
	}

	X_main_loop();
	
	t_pool = [[NSAutoreleasePool alloc] init];
	int t_exit_code = X_close();
	[t_pool release];
	
    MCScriptFinalize();
	MCFinalize();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
