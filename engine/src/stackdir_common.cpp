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

#include "system.h"

#include "stackdir.h"

#include "stackdir_private.h"

/* ----------------------------------------------------------------
 * [Private] Adding error reports
 * ---------------------------------------------------------------- */

/* Format an error into an immutable array, then append it to the
 * operation's current list of error and warning reports, setting
 * the operation's status. */
static void
MCStackdirIOError (MCStackdirIORef op, MCStackdirStatus p_status,
				   MCStringRef p_filename, MCValueRef p_file_line,
				   MCValueRef p_file_col, MCStringRef p_message)
{
	MCAutoNumberRef t_status;
	if (!MCNumberCreateWithInteger (p_status, &t_status))
	{
		MCStackdirIOErrorOutOfMemory (op);
		return;
	}

	/* Providing the full path of the offending file is handy for
	 * debugging. If we can't resolve the path, fall back to just
	 * using it directly. */
	MCAutoStringRef t_resolved_filename;
	if (!MCS_resolvepath (p_filename, &t_resolved_filename))
		t_resolved_filename = MCValueRetain (p_filename);


	/* Create an (immutable) report array and append it onto the end
	 * of the error list */

	MCNameRef t_keys[] = {
		/* FIXME Turn these keys into named constants. */
		MCNAME ("status"),
		MCNAME ("filename"),
		MCNAME ("file_line"),
		MCNAME ("file_column"),
		MCNAME ("message"),
	};

	MCValueRef t_values[5];
	t_values[0] = *t_status;
	t_values[1] = p_filename;
	t_values[2] = p_file_line;
	t_values[3] = p_file_col;
	t_values[4] = p_message;


	/* N.b. dense arrays are 1-indexed */
	uindex_t t_idx = MCArrayGetCount (op->m_error_info) + 1;

	MCAutoArrayRef t_report;
	if (!(MCArrayCreate (false, t_keys, t_values, 5, &t_report) &&
		  MCArrayStoreValueAtIndex (op->m_error_info, t_idx,
									*t_report)))
	{
		MCStackdirIOErrorOutOfMemory (op);
		return;
	}

	/* Set the status of the operation, if it doesn't already have an
	 * error status */
	if (!MCStackdirIOHasError (op))
		op->m_status = p_status;
}

bool
MCStackdirIOErrorIO (MCStackdirIORef op, MCStringRef p_filename,
					 MCStringRef p_message)
{
	MCStackdirIOError (op, kMCStackdirStatusIOError,
					   p_filename,     /* filename */
					   kMCEmptyString, /* line */
					   kMCEmptyString, /* column */
					   p_message);        /* message */
	return false;
}

bool
MCStackdirIOErrorOutOfMemory (MCStackdirIORef op)
{
	/* Can't do anything! */
	op->m_status = kMCStackdirStatusOutOfMemory;
	return false;
}

bool
MCStackdirIOErrorBadPath (MCStackdirIORef op, MCStringRef p_message)
{
	MCStackdirIOError (op, kMCStackdirStatusBadPath,
					   op->m_path,     /* filename */
					   kMCEmptyString, /* line */
					   kMCEmptyString, /* column */
					   p_message);     /* message */
	return false;
}

bool
MCStackdirIOErrorBadState (MCStackdirIORef op, MCStringRef p_filename,
						   MCStringRef p_message)
{
	MCStackdirIOError (op, kMCStackdirStatusBadPath,
					   p_filename,     /* filename */
					   kMCEmptyString, /* line */
					   kMCEmptyString, /* column */
					   p_message);     /* message */
	return false;
}

/* ----------------------------------------------------------------
 * [Private] Low-level IO operation utilities
 * ---------------------------------------------------------------- */

bool
MCStackdirIONew (MCStackdirIORef & op)
{
	if (!MCMemoryNew (op)) return false;
	if (!MCArrayCreateMutable (op->m_error_info)) return false;

	op->m_status = kMCStackdirStatusSuccess;

	return true;
}

/* ----------------------------------------------------------------
 * [Private] Tests on expanded stackfile bundles
 * ---------------------------------------------------------------- */

bool
MCStackdirPathIsStackdir (MCStringRef p_path)
{
	/* FIXME record proper error information */

	/* Stackdir must be a folder */
	if (!MCS_exists (p_path, false))
		return false;

	/* Stackdir must contain a version file. */
	MCAutoStringRef t_version_path;
	/* UNCHECKED */ MCStringFormat (&t_version_path, "%@/%@", p_path,
									kMCStackdirVersionFile);

	/* The version file must have the correct contents */
	MCAutoDataRef t_version_data;
	MCAutoStringRef t_version;
	if (!(MCS_loadbinaryfile (*t_version_path, &t_version_data) &&
		  MCStringDecode (*t_version_data, kMCStringEncodingUTF8,
						  false, &t_version) &&
		  MCStringBeginsWith (*t_version, kMCStackdirMagicString,
							  kMCStringOptionCompareExact)))
		return false;

	return true;
}

/* ----------------------------------------------------------------
 * [Public] IO operation destruction
 * ---------------------------------------------------------------- */

void
MCStackdirIODestroy (MCStackdirIORef & op)
{
	if (op == nil) return;

	MCValueRelease (op->m_error_info);

	MCValueRelease (op->m_path);
	MCValueRelease (op->m_save_build_dir);
	MCValueRelease (op->m_save_backup_dir);
	MCValueRelease (op->m_save_backup_path);
	MCValueRelease (op->m_save_state);

	MCValueRelease (op->m_load_state);
	MCValueRelease (op->m_load_state_theirs);

	MCValueRelease (op->m_source_info);

	MCMemoryDelete (op);

	op = nil;
}

/* ----------------------------------------------------------------
 * [Public] Set and restore target path
 * ---------------------------------------------------------------- */

bool
MCStackdirIOSetPath (MCStackdirIORef op, MCStringRef p_path)
{
	MCAssert (op != nil);
	return MCStringCopy (p_path, op->m_path);
}

void
MCStackdirIOGetPath (MCStackdirIORef op, MCStringRef & r_path)
{
	r_path = MCValueRetain (op->m_path);
}

/* ----------------------------------------------------------------
 * [Public] Run IO operation and get results
 * ---------------------------------------------------------------- */

void
MCStackdirIOCommit (MCStackdirIORef op)
{
	MCAssert (op != nil);

	switch (op->m_type)
	{
	case kMCStackdirIOTypeSave:
		MCStackdirIOCommitSave (op);
		break;
	case kMCStackdirIOTypeLoad:
		MCStackdirIOCommitLoad (op);
		break;
	default:
		MCUnreachable();
		break;
	}
}

MCStackdirStatus
MCStackdirIOGetStatus (MCStackdirIORef op, MCArrayRef *r_error_info)
{
	if (r_error_info != nil)
		*r_error_info = MCValueRetain (op->m_error_info);

	return op->m_status;
}
