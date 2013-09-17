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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"

#include <unistd.h>
#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, char *argv[], char *envp[]);
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

int main(int argc, char *argv[], char *envp[])
{
	extern bool MCS_mac_elevation_bootstrap_main(int argc, char* argv[]);
	if (argc == 2 && strcmp(argv[1], "-elevated-slave") == 0)
		if (!MCS_mac_elevation_bootstrap_main(argc, argv))
			return -1;
	
	// MW-2011-08-18: [[ Bug ]] Make sure we initialize Cocoa on startup.
	NSApplicationLoad();
	
	if (!MCInitialize())
		exit(-1);
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	if (!X_init(argc, argv, envp))
	{
		[t_pool release];
		
		MCAutoStringRef t_caption;
		MCAutoStringRef t_text;
		MCModeGetStartupErrorMessage(&t_caption, &t_text);
		fprintf(stderr, "%s - %s\n", MCStringGetCString(*t_caption), MCStringGetCString(*t_text));
		exit(-1);
	}

	X_main_loop();
	
	t_pool = [[NSAutoreleasePool alloc] init];
	int t_exit_code = X_close();
	[t_pool release];
	
	MCFinalize();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
