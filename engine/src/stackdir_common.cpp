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
 * [Public] Run IO operation
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

