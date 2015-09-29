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

#include <cstring>

#include <stdlib.h>

#include <revolution/external.h>
#include <revolution/support.h>

#include "revfont.h"

void revFontLoad(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_error)
{
	const char *t_result;
	t_result = "";
	Bool t_error = False;
	if (p_argument_count == 1)
	{
		if (!SecurityCanAccessFile(p_arguments[0]))
		{
			t_result = "file access not permitted";
			t_error = True;
		}
		else
		{
			// OK-2008-01-22 : Bug 5771. Use functions in libexternal to resolve and convert the path in the same way as the engine.
			char *t_native_path;
			t_native_path = os_path_to_native(p_arguments[0]);

			char *t_resolved_path;
			t_resolved_path = os_path_resolve(t_native_path);
			free(t_native_path);

			FontLoadStatus t_status;
			t_status = FontLoad(t_resolved_path);
			if (t_status == kFontLoadStatusNotFound)
				t_result = "couldn't find font";
			else if (t_status == kFontLoadStatusBadFont)
				t_result = "couldn't load font";
				
			free(t_resolved_path);
		}
	}
	else
		t_result = "invalid arguments";
	
	*r_error = t_error;
	*r_pass = False;
	*r_result = strdup(t_result);
}

void revFontUnload(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_error)
{
	const char *t_result;
	t_result = "";

	// OK-2008-01-22 : Bug 5771. Use functions in libexternal to resolve and convert the path in the same way as the engine.
	char *t_native_path;
	t_native_path = os_path_to_native(p_arguments[0]);

	char *t_resolved_path;
	t_resolved_path = os_path_resolve(t_native_path);
	free(t_native_path);

	if (p_argument_count == 1)
	{
		if (!FontUnload(t_resolved_path))
			t_result = "couldn't unload font";
	}
	else
		t_result = "invalid arguments";
		
	free(t_resolved_path);

	*r_error = False;
	*r_pass = False;
	*r_result = strdup(t_result);
}

EXTERNAL_BEGIN_DECLARATIONS("revFont")
	EXTERNAL_DECLARE_COMMAND("revFontLoad", revFontLoad)
	EXTERNAL_DECLARE_COMMAND("revFontUnload", revFontUnload)

	EXTERNAL_DECLARE_COMMAND("XLOAD_FONT", revFontLoad)
	EXTERNAL_DECLARE_COMMAND("XUNLOAD_FONT", revFontUnload)
EXTERNAL_END_DECLARATIONS
