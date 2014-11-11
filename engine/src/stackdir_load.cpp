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

typedef struct _MCStackdirIOObjectLoad MCStackdirIOObjectLoad;
typedef MCStackdirIOObjectLoad *MCStackdirIOObjectLoadRef;

struct _MCStackdirIOObjectLoad
{
	MCStackdirIORef m_op;

	/* Path of object directory */
	MCStringRef m_path;

	/* State and source info for "ours" */
	MCArrayRef m_state;
	MCArrayRef m_source_info;

	/* State and source info for "theirs" */
	MCArrayRef m_state_theirs;
	MCArrayRef m_source_info_theirs;
};

/* This structure is only used to pass information about the LSB
 * directory through to the object directory handler. */
typedef struct _MCStackdirIOObjectDirInfo MCStackdirIOObjectDirInfo;
typedef MCStackdirIOObjectDirInfo *MCStackdirIOObjectDirInfoRef;

struct _MCStackdirIOObjectDirInfo
{
	MCStackdirIORef m_op;

	/* Object's LSB directory name */
	MCStringRef m_lsb_name;

	/* Path to LSB directory */
	MCStringRef m_lsb_path;
};

/* ----------------------------------------------------------------
 * [Private] Utility functions
 * ---------------------------------------------------------------- */

/* Test whether the load operation has been performed by calling
 * MCStackdirIOCommit() */
static inline bool MCStackdirIOLoadIsOperationComplete (MCStackdirIORef op)
{
	return (op->m_load_state != nil);
}

/* Test whether p_object_dir is a validly-named object directory and
 * whether p_lsb_dir is a valid LSB directory for p_object_dir. */
static bool MCStackdirIOLoadIsValidObjectDir (MCStringRef p_object_dir, MCStringRef p_lsb_dir);

/* ----------------------------------------------------------------
 * [Private] Load operations
 * ---------------------------------------------------------------- */

static bool MCStackdirIOLoadObject (MCStackdirIORef op, MCStringRef p_uuid, MCStringRef p_obj_path);

/* Read a datum from the file named p_key, and save it into the state
 * vector with the same p_key. */
static bool MCStackdirIOLoadObjectKeyDirect (MCStackdirIOObjectLoadRef info, MCStringRef p_key, bool p_required=true);

/* Load an object's "_parent" file */
static bool MCStackdirIOLoadObjectParent (MCStackdirIOObjectLoadRef info);

/* Load an object's "_kind" file */
static bool MCStackdirIOLoadObjectKind (MCStackdirIOObjectLoadRef info);

/* Load object's internal properties */
static bool MCStackdirIOLoadObjectInternal (MCStackdirIOObjectLoadRef info);

/* Load object's custom properties (in ".propset" directories, and
 * reading "_propsets" if present. */
static bool MCStackdirIOLoadObjectCustom (MCStackdirIOObjectLoadRef info);

/* Load object's shared properties (in "_shared" file and ".shared"
 * directories) */
static bool MCStackdirIOLoadObjectShared (MCStackdirIOObjectLoadRef info);

/* ================================================================
 * Errors
 * ================================================================ */

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOLoadErrorNotStackdir,
							kMCStackdirStatusBadPath,
							"Target is not a stackdir")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOLoadErrorInvalidObjectDir,
							kMCStackdirStatusBadStructure,
							"Object directory layout is incorrect")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOLoadErrorDirectLiteral,
							kMCStackdirStatusSyntaxError,
							"Expected literal value")

/* ================================================================
 * Utility functions
 * ================================================================ */

static bool
MCStackdirIOLoadIsValidObjectDir (MCStringRef p_object_dir,
								  MCStringRef p_lsb_dir)
{
	/* The object directory must be named according to the UUID */
	if (!MCStackdirStringIsUUID (p_object_dir))
		return false;

	/* The LSB directory has to be exactly two hex characters. */
	if (2 != MCStringGetLength (p_lsb_dir))
		return false;

	/* The LSB directory's name should be the last two characters of
	   the UUID. */
	if (!MCStringEndsWith (p_object_dir, p_lsb_dir,
						   kMCStringOptionCompareExact))
		return false;

	return true;
}

/* ================================================================
 * Object loading
 * ================================================================ */

static bool
MCStackdirIOLoadObject (MCStackdirIORef op,
						MCStringRef p_uuid,
						MCStringRef p_obj_path)
{
	/* Create state and source info arrays */
	MCAutoArrayRef t_object_state, t_object_state_theirs,
		t_object_source_info, t_object_source_info_theirs;
	if (!(MCArrayCreateMutable (&t_object_state) &&
		  MCArrayCreateMutable (&t_object_state_theirs) &&
		  MCArrayCreateMutable (&t_object_source_info) &&
		  MCArrayCreateMutable (&t_object_source_info_theirs)))
		return MCStackdirIOErrorOutOfMemory (op);

	/* Create object load information structure */
	MCStackdirIOObjectLoad t_load_info;
	t_load_info.m_op = op;
	t_load_info.m_path = p_obj_path;
	t_load_info.m_state = *t_object_state;
	t_load_info.m_source_info = *t_object_source_info;
	t_load_info.m_state_theirs = *t_object_state_theirs;
	t_load_info.m_source_info_theirs = *t_object_source_info_theirs;

	if (!(MCStackdirIOLoadObjectKind (&t_load_info) &&
		  MCStackdirIOLoadObjectParent (&t_load_info) &&
		  MCStackdirIOLoadObjectInternal (&t_load_info) &&
		  MCStackdirIOLoadObjectCustom (&t_load_info) &&
		  MCStackdirIOLoadObjectShared (&t_load_info)))
		return false;

	/* Store load results */
	MCNewAutoNameRef t_uuid_name;
	if (!(MCNameCreate (p_uuid, &t_uuid_name) &&
		  MCArrayStoreValue (op->m_load_state, true, *t_uuid_name,
							 (MCValueRef) *t_object_state) &&
		  MCArrayStoreValue (op->m_load_state_theirs, true, *t_uuid_name,
							 (MCValueRef) *t_object_state_theirs) &&
		  MCArrayStoreValue (op->m_source_info, true, *t_uuid_name,
							 (MCValueRef) *t_object_source_info) &&
		  MCArrayStoreValue (op->m_source_info_theirs, true, *t_uuid_name,
							 (MCValueRef) *t_object_source_info_theirs)))
		return MCStackdirIOErrorOutOfMemory (op);

	return true;
}

static bool
MCStackdirIOLoadObjectKeyDirect (MCStackdirIOObjectLoadRef info,
								 MCStringRef p_key,
								 bool p_required)
{
	/* Construct path for file */
	MCAutoStringRef t_path;
	if (!MCStringFormat (&t_path, "%@/%@", info->m_path, p_key))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Load file into memory.  If p_required is false, allow the file
	 * to be unreadable. */
	MCAutoStringRef t_content;
	if (!MCStackdirIOLoadUTF8 (info->m_op,
							   *t_path,
							   &t_content,
							   !p_required))
		return (!MCStackdirIOHasError (info->m_op));

	/* Parse a token from the contents of the file.  If the file
	 * exists, we require it to contain a valid value. */
	MCStackdirIOScannerRef t_scanner;
	MCStackdirIOTokenRef t_token;
	bool t_success = true;
	MCAutoValueRef t_value;
	if (!MCStackdirIOScannerNew (*t_content, t_scanner))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	if (t_success)
	{
		if (!MCStackdirIOScannerConsume (t_scanner, t_token) ||
			t_token->m_value == nil)
		{
			t_success = false;
			MCStackdirIOLoadErrorDirectLiteral (info->m_op,
												*t_path,
												t_token->m_line,
												t_token->m_column);
		}
	}

	if (t_success)
		&t_value = MCValueRetain (t_token->m_value);

	MCStackdirIOScannerDestroy (t_scanner);
	if (!t_success) return false;

	/* Insert the value into the state array */
	MCNewAutoNameRef t_key;
	if (!MCNameCreate (p_key, &t_key))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	if (!MCArrayStoreValue (info->m_state,
							false,
							*t_key,
							*t_value))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	return true;
}

static bool
MCStackdirIOLoadObjectParent (MCStackdirIOObjectLoadRef info)
{
	return MCStackdirIOLoadObjectKeyDirect (info,
											kMCStackdirParentFile,
											false);
}

static bool
MCStackdirIOLoadObjectKind (MCStackdirIOObjectLoadRef info)
{
	return MCStackdirIOLoadObjectKeyDirect (info,
											kMCStackdirKindFile,
											true);
}

static bool
MCStackdirIOLoadObjectInternal (MCStackdirIOObjectLoadRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOLoadObjectCustom (MCStackdirIOObjectLoadRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOLoadObjectShared (MCStackdirIOObjectLoadRef info)
{
	/* FIXME implementation */
	return true;
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

static bool
MCStackdirIOCommitLoad_ObjectDirCallback (void *context,
										  const MCSystemFolderEntry *p_entry)
{
	MCStackdirIOObjectDirInfoRef t_dir_info = (MCStackdirIOObjectDirInfoRef) context;

	if (MCStringBeginsWithCString (p_entry->name, (const char_t *) ".",
								   kMCStringOptionCompareExact))
		return true;

	MCAutoStringRef t_object_path;
	if (!MCStringFormat (&t_object_path, "%@/%@",
						 t_dir_info->m_lsb_path, p_entry->name))
		return MCStackdirIOErrorOutOfMemory (t_dir_info->m_op);

	/* Verify that the directory layout is valid. Check the LSB
	 * directory and object directory at the same time. */
	if (!p_entry->is_folder ||
		!MCStackdirIOLoadIsValidObjectDir (p_entry->name,
										   t_dir_info->m_lsb_name))
		return MCStackdirIOLoadErrorInvalidObjectDir (t_dir_info->m_op,
													  *t_object_path);

	/* Load object */
	bool t_success = true;

	MCStackdirIOErrorLocationPush (t_dir_info->m_op, *t_object_path);

	if (t_success)
		t_success = MCStackdirIOLoadObject (t_dir_info->m_op,
											p_entry->name,
											*t_object_path);

	MCStackdirIOErrorLocationPop (t_dir_info->m_op);

	return t_success;
}

static bool
MCStackdirIOCommitLoad_LsbDirCallback (void *context,
									   const MCSystemFolderEntry *p_entry)
{
	MCStackdirIORef t_op = (MCStackdirIORef) context;

	if (MCStringBeginsWithCString (p_entry->name, (const char_t *) ".",
								   kMCStringOptionCompareExact))
		return true;

	/* Only interested in directories, so skip other files */
	if (!p_entry->is_folder) return true;

	/* We don't actually verify whether this is a valid LSB directory
	 * name at this stage; we allow the object-specific callback
	 * function to check it. */
	MCAutoStringRef t_save_cwd;
	bool t_success = true;

	MCAutoStringRef t_lsb_path, t_lsb_native_path;
	if (!(MCStringFormat (&t_lsb_path, "%@/%@",
						  t_op->m_path, p_entry->name) &&
		  MCS_pathtonative (*t_lsb_path, &t_lsb_native_path)))
		return MCStackdirIOErrorOutOfMemory (t_op);

	/* Iterate over the contents of the LSB directory (which should be
	 * *only* object directories) */
	MCStackdirIOObjectDirInfo t_dir_info;
	t_dir_info.m_op = t_op;
	t_dir_info.m_lsb_name = p_entry->name;
	t_dir_info.m_lsb_path = *t_lsb_path;

	if (t_success)
		t_success = MCsystem->ListFolderEntries (*t_lsb_native_path,
												 MCStackdirIOCommitLoad_ObjectDirCallback,
												 &t_dir_info);

	return t_success;
}

void
MCStackdirIOCommitLoad (MCStackdirIORef op)
{
	MCStackdirIOAssertLoad (op);

	if (!(MCArrayCreateMutable (op->m_load_state) &&
		  MCArrayCreateMutable (op->m_source_info) &&
		  MCArrayCreateMutable (op->m_load_state_theirs) &&
		  MCArrayCreateMutable (op->m_source_info_theirs)))
	{
		MCStackdirIOErrorOutOfMemory (op);
		return;
	}

	/* Sanity check that the target path has been set */
	MCAssert (op->m_path);

	/* Check that the target path is, in fact, a stackdir */
	if (MCStackdirPathIsStackdir (op->m_path))
	{
		MCAutoStringRef t_save_cwd;
		bool t_success = true;

		MCStackdirIOErrorLocationPush (op, op->m_path);

		/* Iterate over stackdir contents (which should mostly be LSB
		 * directories) */
		if (t_success)
			t_success = MCsystem->ListFolderEntries (op->m_path,
										MCStackdirIOCommitLoad_LsbDirCallback,
										op);

		MCStackdirIOErrorLocationPop (op);
	}
	else
	{
		MCStackdirIOLoadErrorNotStackdir (op, op->m_path);
	}
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
