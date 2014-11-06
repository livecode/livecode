/*                                                                   -*- c++ -*-
Copyright (C) 2003-2014 Runtime Revolution Ltd.

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
#include "sysdefs.h"
#include "exec.h"

#include "util.h"
#include "system.h"

#include "stackdir.h"

#include "stackdir_private.h"

/* This file contains code relating to loading Expanded LiveCode
 * Stackfiles.
 */

/* ================================================================
 * File-local declarations
 * ================================================================ */

/* ----------------------------------------------------------------
 * [Private] Utility functions
 * ---------------------------------------------------------------- */

static inline bool MCStackdirIOLoadIsOperationComplete (MCStackdirIORef op)
{
	return (op->m_load_state != nil);
}

/* ================================================================
 * High-level entry points
 * ================================================================ */

/* ----------------------------------------------------------------
 * [Public] High-level operations
 * ---------------------------------------------------------------- */

bool
MCStackdirIONewLoad (MCStackdirIORef & op)
{
	if (!MCStackdirIONew (op)) return false;
	op->m_type = kMCStackdirIOTypeLoad;
	return true;
}

bool
MCStackdirIOHasConflict (MCStackdirIORef op)
{
	MCStackdirIOAssertLoad (op);

	/* FIXME This is a stub and needs a proper implementation. */
	return false;
}

void
MCStackdirIOSetConflictPermitted (MCStackdirIORef op,
								  bool enabled)
{
	MCStackdirIOAssertLoad (op);
	op->m_load_allow_conflicts = enabled;
}

bool
MCStackdirIOGetConflictPermitted (MCStackdirIORef op)
{
	MCStackdirIOAssertLoad (op);
	return op->m_load_allow_conflicts;
}

bool
MCStackdirIOGetState (MCStackdirIORef op,
					  MCArrayRef *r_state,
					  MCArrayRef *r_source_info,
					  MCStackdirResult mode)
{
	MCStackdirIOAssertLoad (op);

	if (!MCStackdirIOLoadIsOperationComplete (op)) return false;
	if (MCStackdirIOHasError (op)) return false;

	MCArrayRef t_state;
	MCArrayRef t_source_info;
	switch (mode)
	{
	case kMCStackdirResultOurs:
		t_state = op->m_load_state;
		t_source_info = op->m_source_info;
		break;

	case kMCStackdirResultTheirs:
		/* If conflicts are disabled, do nothing */
		if (!MCStackdirIOGetConflictPermitted (op)) return false;

		/* If there was no conflict, then return the "ours" data. */
		if (!MCStackdirIOHasConflict (op))
			return MCStackdirIOGetState (op, r_state, r_source_info);

		t_state = op->m_load_state_theirs;
		t_source_info = op->m_source_info_theirs;
		break;

	default:
		MCUnreachable ();
	}

	if (r_state != nil)
	{
		MCAssert (t_state != nil);
		*r_state = MCValueRetain (t_state);
	}
	if (r_source_info != nil)
	{
		MCAssert (t_source_info);
		*r_source_info = MCValueRetain (t_source_info);
	}

	return true;

}

/* ----------------------------------------------------------------
 * [Private] High-level operations
 * ---------------------------------------------------------------- */

void
MCStackdirIOCommitLoad (MCStackdirIORef op)
{
	MCStackdirIOAssertLoad (op);

	/* FIXME implementation */
	MCArrayCreateMutable (op->m_load_state);
	MCArrayCreateMutable (op->m_source_info);
}

/* ----------------------------------------------------------------
 * [Public] Internal debugging commands
 * ---------------------------------------------------------------- */

/* _internal stackdir load <path> into <variable> */
void
MCStackdirExecInternalLoad (MCExecContext & ctxt,
							MCStringRef p_path,
							MCVariableChunkPtr p_var)
{
	/* Run load transaction */
	MCStackdirIORef t_op;
	/* UNCHECKED */ MCStackdirIONewLoad (t_op);
	MCStackdirIOSetPath (t_op, p_path);

	MCStackdirIOCommit (t_op);

	MCAutoArrayRef t_error_info;
	MCStackdirStatus t_status;
	t_status = MCStackdirIOGetStatus (t_op, &(&t_error_info));

	if (t_status == kMCStackdirStatusSuccess)
	{
		MCAutoArrayRef t_state;
		/* UNCHECKED */ MCStackdirIOGetState (t_op, &(&t_state), nil);

		MCEngineExecPutIntoVariable (ctxt,
									 (MCValueRef) *t_state,
									 PT_INTO,
									 p_var);
		ctxt.SetTheResultToBool (true);
	}
	else
	{
		ctxt.SetTheResultToValue (*t_error_info);
	}

	MCStackdirIODestroy (t_op);
}
