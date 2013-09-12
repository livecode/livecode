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
	// On Linux, the argv and envp could be in pretty much any format. It
	// is probably safest to assume they match the encoding specified by
	// environment variables. (Technically, it could be absolutely anything
	// as it all depends on the programme that called exec()).
	const char *lang = getenv("LANG");
	const char *encoding = strrchr(lang, '.');
	if (encoding == NULL)
	{
		// Couldn't find an encoding in the $LANG variable, try $LC_ALL
		lang = getenv("LC_ALL");
		encoding = strrchr(encoding, '.');
		if (encoding == NULL)
		{
			// Still couldn't find an encoding. Assume ISO-8859-1 (this may
			// not be correct but it matches the historical LiveCode behaviour)
			encoding = "ISO-8859-1";
		}
	}
	
	// The C library string functions should obey the $LANG encoding for now
	setlocale(LC_CTYPE, "");
	
	// Which encoding was discovered?
	bool t_is_utf8;
	if (strcasecmp(encoding, "UTF-8") == 0 || strcasecmp(encoding, "UTF8") == 0)
		t_is_utf8 = true;
	else if (strcasecmp(encoding, "ISO-8859-1") == 0)
		t_is_utf8 = false;
	else
	{
		/* TODO */
		// Support more encodings (if required) via iconv
		fprintf(stderr, "%s: unknown or unsupported character encoding ($LANG=%s) ($LC_ALL=%)",
				argv[0], getenv("LANG"), getenv("LC_ALL"));
	}
	
	// Convert the argv array to StringRefs
	MCStringRef* t_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_argv);
	for (int i = 0; i < argc; i++)
	{
		/* UNCHECKED */ MCStringCreateWithBytes(argv[i], strlen(argv[i]), 
												t_is_utf8 ? kMCStringEncodingUTF8 : kMCStringEncodingISO8859_1,
												false, t_argv[i]);
	}
	
	// Convert the envp array to StringRefs
	int envc = 0;
	MCStringRef* t_envp;
	while (envp[envc] != NULL)
	{
		uindex_t t_count = envc + 1;
		/* UNCHECKED */ MCMemoryResizeArray(t_count + 1, t_envp, t_count);
		/* UNCHECKED */ MCStringCreateWithBytes(envp[envc], strlen(envp[envc]),
												t_is_utf8 ? kMCStringEncodingUTF8 : kMCStringEncodingISO8859_1,
												false, t_envp[envc]);
		envc++;
	}
	
	// Terminate the envp array
	t_envp[envc] = nil;
	
	extern int MCSystemElevatedMain(int, char*[], char*[]);
	if (argc == 3&& strcmp(argv[1], "-elevated-slave") == 0)
		return MCSystemElevatedMain(argc, argv, envp);
	
	if (!MCInitialize())
		exit(-1);

	if (!X_init(argc, t_argv, t_envp))
	{
		MCExecPoint ep;
		if (MCresult != nil)
		{
			MCresult -> eval(ep);
			fprintf(stderr, "Startup error - %s\n", ep . getcstring());
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

	MCFinalize();

	exit(t_exit_code);
}

////////////////////////////////////////////////////////////////////////////////
