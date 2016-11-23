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
	MCStack *t_startup_stack = nil;

	/* Load the startup stack */
	if (IO_NORMAL != MCdispatcher->loadfile(p_params.output, t_startup_stack))
	{
		MCDeployThrow(kMCDeployErrorBadRead);
		goto error_cleanup;
	}

	/* Prepare the startup stack for use during engine boot  */
	if (!MCStackSecurityEmscriptenPrepareStartupStack(t_startup_stack))
	{
		MCDeployThrow(kMCDeployErrorEmscriptenBadStack);
		goto error_cleanup;
	}

	/* Save the stack back to disk */
	if (IO_NORMAL != MCdispatcher->savestack(t_startup_stack, p_params.output))
	{
		MCDeployThrow(kMCDeployErrorBadWrite);
		goto error_cleanup;
	}

	/* Clean up */
	delete t_startup_stack;

	return ES_NORMAL;

 error_cleanup:
	if (nil != t_startup_stack)
	{
		delete t_startup_stack;
	}

	return ES_ERROR;
}
