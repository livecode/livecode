/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "parsedef.h"
#include "filedefs.h"

#include "dispatch.h"
#include "stacksecurity.h"
#include "globals.h"
#include "deploy.h"

Exec_stat
MCDeployToEmscripten(const MCDeployParameters & p_params)
{
	bool t_success = true;

	/* Open the output file */
	MCDeployFileRef t_output = nil;
	if (t_success && !MCDeployFileOpen(p_params . output, kMCOpenFileModeCreate, t_output))
		t_success = MCDeployThrow(kMCDeployErrorNoOutput);

	uint32_t t_project_size = 0;
	/* Write the stack capsule data */
	if (t_success)
		t_success = MCDeployWriteProject(p_params, false, t_output, 0, t_project_size);
	
	MCDeployFileClose(t_output);
	
	return t_success ? ES_NORMAL : ES_ERROR;
}
