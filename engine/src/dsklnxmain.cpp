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

#include "core.h"
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

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, char *argv[], char *envp[]);
void X_main_loop_iteration();
int X_close();

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while(!MCquit)
		X_main_loop_iteration();
}

int main(int argc, char *argv[], char *envp[])
{
	extern int MCSystemElevatedMain(int, char*[], char*[]);
	if (argc == 3&& strcmp(argv[1], "-elevated-slave") == 0)
		return MCSystemElevatedMain(argc, argv, envp);
	
	if (!X_init(argc, argv, envp))
	{
		if (MCresult != NULL && MCresult -> getvalue() . is_string())
		{
			char *t_message;
			t_message = MCresult -> getvalue() . get_string() . clone();
			fprintf(stderr, "Startup error - %s\n", t_message);
		}
		exit(-1);
	}

	X_main_loop();

	int t_exit_code = X_close();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
