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

/* This file contains code relating to saving Expanded LiveCode
 * Stackfiles.
 *
 * Each save operation is carried out in transaction-style: the new
 * stack is constructed in a temporary directory, and on success, it's
 * moved into place to replace the original directory.
 */

/* ================================================================
 * File-local declarations
 * ================================================================ */

typedef struct _MCStackdirIOObjectSave MCStackdirIOObjectSave;
typedef MCStackdirIOObjectSave *MCStackdirIOObjectSaveRef;

struct _MCStackdirIOObjectSave
{
	MCStackdirIORef m_op;

	/* Object's UUID */
	MCNameRef m_uuid;

	/* Object's state array */
	MCArrayRef m_state;

	/* Path to object directory */
	MCStringRef m_path;
};

/* ----------------------------------------------------------------
 * [Private] Utility functions
 * ---------------------------------------------------------------- */

/* Recursively delete a directory */
static bool MCStackdirIORemoveFolderRecursive (MCStackdirIORef op, MCStringRef p_path);

/* ----------------------------------------------------------------
 * [Private] Save operations
 * ---------------------------------------------------------------- */

/* Save operations are wrapped in a transaction to try to ensure that
 * if an error occurs, the existing data on disk isn't altered. */
static bool MCStackdirIOSaveTransactionStart (MCStackdirIORef op);
static bool MCStackdirIOSaveTransactionCancel (MCStackdirIORef op);
static bool MCStackdirIOSaveTransactionEnd (MCStackdirIORef op);

/* Save the "_version" file. */
static bool MCStackdirIOSaveVersion (MCStackdirIORef op);

/* Save one stack object */
static bool MCStackdirIOSaveObject (MCStackdirIORef op, MCNameRef p_uuid, MCArrayRef p_state);

/* Create object directory */
static bool MCStackdirIOSaveObjectDirectory (MCStackdirIOObjectSaveRef info);

/* Create object's "_kind" file */
static bool MCStackdirIOSaveObjectKind (MCStackdirIOObjectSaveRef info);

/* Create object's "_parent" file */
static bool MCStackdirIOSaveObjectParent (MCStackdirIOObjectSaveRef info);

/* Save object's internal properties */
static bool MCStackdirIOSaveObjectInternal (MCStackdirIOObjectSaveRef info);

/* Save object's custom property sets (in ".propset" directories, and
 * create "_propsets" if necessary. */
static bool MCStackdirIOSaveObjectPropsets (MCStackdirIOObjectSaveRef info);

/* Save object's shared properties (in "_shared" file and ".shared"
 * directories) */
static bool MCStackdirIOSaveObjectShared (MCStackdirIOObjectSaveRef info);

/* Save a property set.
 *
 * Generates "_contents" and/or "_overflow" files in p_propset_path,
 * along with any necessary external storage files. */
static bool MCStackdirIOSavePropSet (MCArrayRef p_propset, MCStringRef p_propset_path);

/* ================================================================
 * Utility functions
 * ================================================================ */

static bool
MCStackdirIORemoveFolderRecursive_Callback (void *p_context,
											const MCSystemFolderEntry *p_entry)
{
	MCStackdirIORef t_op = (MCStackdirIORef) p_context;

	if (MCStringIsEqualTo (p_entry->name, MCSTR (".."),
						   kMCStringOptionCompareExact) ||
		MCStringIsEqualTo (p_entry->name, MCSTR ("."),
						   kMCStringOptionCompareExact))
		return true;

	if (p_entry->is_folder)
	{
		/* Recurse */
		if (!MCStackdirIORemoveFolderRecursive (t_op, p_entry->name))
			return false;
	}
	else
	{
		if (!MCS_unlink (p_entry->name))
			return MCStackdirIOErrorIO (t_op, p_entry->name,
										MCSTR ("Failed to delete file"));
	}
	return true;
}

static bool
MCStackdirIORemoveFolderRecursive (MCStackdirIORef op, MCStringRef p_path)
{
	MCAutoStringRef t_save_cwd;
	bool t_success = true;

	/* Change to the target directory. */
	MCS_getcurdir (&t_save_cwd);
	if (!MCS_setcurdir (p_path))
		return MCStackdirIOErrorIO (op, p_path,
									MCSTR ("Failed to change directory"));

	/* Recurse */
	t_success = MCsystem->ListFolderEntries (MCStackdirIORemoveFolderRecursive_Callback, op);

	/* Restore working directory and remove target directory */
	if (!MCS_setcurdir (*t_save_cwd))
		return MCStackdirIOErrorIO (op, *t_save_cwd,
									MCSTR ("Failed to change directory"));
	if (t_success && !MCS_rmdir (p_path))
		return MCStackdirIOErrorIO (op, p_path,
									MCSTR ("Failed to delete directory"));

	return t_success;
}

/* ================================================================
 * Save transactions
 * ================================================================ */

/* Set up a temporary working directory for the transaction.  The
 * new directory is created in the same directory as the target
 * expanded stackfile bundle, with a randomly-generated suffix.
 *
 * We don't use the system temporary directory because we want to
 * ensure that the new directory is created on the same filesystem as
 * the existing one.
 */
static bool
MCStackdirIOSaveTransactionCreateDir (MCStackdirIORef op, MCStringRef &r_temp_path)
{
	MCAutoStringRef t_resolved_path, t_base_path, t_temp_path;

	if (!MCS_resolvepath (op->m_path, &t_resolved_path))
		return MCStackdirIOErrorIO (op, op->m_path,
									MCSTR ("Failed to resolve path"));

	/* If the target path ends in a "/", remove it */
	if (MCStringEndsWithCString (*t_resolved_path, (const char_t *) "/",
								 kMCStringOptionCompareExact))
	{
		uindex_t len = MCStringGetLength (*t_resolved_path) - 1;
		/* UNCHECKED */ MCStringCopySubstring (*t_resolved_path, MCRangeMake (0, len),
											   &t_base_path);
	}
	else
	{
		t_base_path = MCValueRetain (*t_resolved_path);
	}

	/* Repeatedly generate a random suffix and check that it's not
	 * already a file or suffix that exists.
	 */
	while (true)
	{
		MCAutoDataRef t_suffix_data;
		uint32_t t_suffix_numeric;
		MCAutoStringRef t_native_path;

		/* Create a random number */
		/* UNCHECKED */ MCU_random_bytes (sizeof (t_suffix_numeric),
										  &t_suffix_data);
		for (size_t i = 0; i < sizeof (t_suffix_numeric); ++i)
		{
			t_suffix_numeric += MCDataGetByteAtIndex (*t_suffix_data, i) << i;
		}

		/* Generate directory path */
		/* UNCHECKED */ MCStringFormat (&t_temp_path, "%@.temp%x",
										*t_base_path, t_suffix_numeric);

		/* Attempt to create directory */
		if (MCS_mkdir (*t_temp_path))
			break;

		/* If creating the folder failed despite the fact it doesn't
		 * exist, there must be some other IO error. */
		if (!(MCS_exists (*t_temp_path, true) ||
			  MCS_exists (*t_temp_path, false)))
			return MCStackdirIOErrorIO (op, *t_temp_path,
										MCSTR ("Failed to create temporary directory"));
	}

	r_temp_path = MCValueRetain (*t_temp_path);
	return true;
}

/* Clear variables used by transaction */
static void
MCStackdirIOSaveTransactionCleanup (MCStackdirIORef op)
{
	MCValueRelease (op->m_save_backup_path);
	op->m_save_backup_path = nil;
	MCValueRelease (op->m_save_backup_dir);
	op->m_save_backup_dir = nil;
	MCValueRelease (op->m_save_build_dir);
	op->m_save_build_dir = nil;
}

static bool
MCStackdirIOSaveTransactionStart (MCStackdirIORef op)
{
	/* Check that a path has been provided */
	MCAssert (op->m_path);

	return MCStackdirIOSaveTransactionCreateDir (op, op->m_save_build_dir);
  }

/* Cancel the transaction by recursively deleting all the files
 * that have been created and the temporary working directory.
 *
 * In addition, if cancellation occurs during the transaction commit
 * stage, try to roll back any changes made to the original data.
 */
static bool
MCStackdirIOSaveTransactionCancel (MCStackdirIORef op)
{
	bool t_success = true;

	/* If we got part-way through ending the transaction, check
	 * whether the original data was moved, and if so, move it
	 * back! */
	if (op->m_save_backup_path != nil)
	{
		if (!MCS_rename (op->m_save_backup_path, op->m_path))
		{
			/* We can't even put it back!  In this case, we just keep
			 * everything rather than cancelling the transaction as
			 * normal in the hope that the user can straighten things
			 * up. */
			MCLog ("MCStackdirIOCommit(): CRITICAL: Save failed; "
				   "original data moved to '%@'.",
				   op->m_save_backup_path);
			MCStackdirIOErrorIO (op, op->m_path,
								 MCSTR ("Failed to restore original stackdir"));
			t_success = false;
		}
	}

	/* If we created the backup directory, attempt to delete it.  Note
	 * that this will fail if it has contents, and *that's okay*,
	 * because it'll have contents if we copied the original data into
	 * it and failed to put it back. */
	if (op->m_save_backup_dir != nil &&
		!MCS_rmdir (op->m_save_build_dir))
		MCStackdirIOErrorIO (op, op->m_save_build_dir,
							 MCSTR ("Failed to remove temporary backup directory"));
		t_success = false;

	/* If the build directory was successfully created, delete it. */
	if (op->m_save_build_dir != nil &&
		!MCStackdirIORemoveFolderRecursive (op, op->m_save_build_dir))
		t_success = false;

	MCStackdirIOSaveTransactionCleanup (op);

	return t_success;
}

/* Complete the transaction, moving the temporary directory into the
 * target location.  If the target path exists, and it *isn't* an
 * expanded stackfile, then the transaction fails (and is
 * automatically cancelled).  If the target path exists and it *is* an
 * expanded stackfile, then we move the current data into the
 * temporary directory, move the temporary directory to the target
 * path, and then delete the current data.
 */
static bool
MCStackdirIOSaveTransactionEnd (MCStackdirIORef op)
{
	bool t_overwriting = false;
	MCAutoStringRef t_backup_dir;
	MCAutoStringRef t_backup_path;

	/* If the target path exists and it *isn't* an expanded stackfile,
	 * then fail and cancel the transaction. */

	if (MCStackdirPathIsStackdir (op->m_path))
	{
		t_overwriting = true;
	}
	else if (MCS_exists (op->m_path, true) ||
			 MCS_exists (op->m_path, false))
	{
		MCStackdirIOSaveTransactionCancel (op);
		return MCStackdirIOErrorBadPath (op, MCSTR ("Target exists and is not a stackdir"));
	}

	/* If overwriting, create an additional temporary directory, and
	 * move the existing bundle there. */

	if (t_overwriting)
	{
		if (!MCStackdirIOSaveTransactionCreateDir (op, &t_backup_dir))
		{
			MCStackdirIOSaveTransactionCancel (op);
			return false;
		}
		op->m_save_backup_dir = MCValueRetain (*t_backup_dir);

		/* UNCHECKED */ MCStringFormat (&t_backup_path, "%@/%@",
										*t_backup_dir, kMCStackdirSaveBackupDir);

		if (!MCS_rename (op->m_path, *t_backup_path))
		{
			MCStackdirIOSaveTransactionCancel (op);
			return MCStackdirIOErrorIO (op, op->m_path,
										MCSTR ("Failed to rename original stackdir"));
		}
		op->m_save_backup_path = MCValueRetain (*t_backup_path);
	}

	/* Move the newly-written bundle into place */
	if (!MCS_rename (op->m_save_build_dir, op->m_path))
	{
		/* Oops! That should have worked!  During cancellation, we'll
		 * try to put the original data back. */
		MCStackdirIOSaveTransactionCancel (op);
		return MCStackdirIOErrorIO (op, op->m_path,
									MCSTR ("Failed to move new stackdir into place"));
		return false;
	}

	/* Now delete the temporary directory we created for the original
	 * data.  Note that even if this fails, it's only a warning; the
	 * save has actually been successful and the user's data is
	 * safe. */
	if (t_overwriting)
	{
		MCStackdirStatus t_op_status = op->m_status;
		/* UNCHECKED */ MCStackdirIORemoveFolderRecursive (op, *t_backup_dir);

		op->m_status = t_op_status; /* Reset the status */
	}

	MCStackdirIOSaveTransactionCleanup (op);
	return true;
}

/* ================================================================
 * Object saving
 * ================================================================ */

static bool
MCStackdirIOSaveObjectDirectory (MCStackdirIOObjectSaveRef info)
{
	MCStringRef t_uuid = MCNameGetString (info->m_uuid);
	uindex_t t_uuid_len = MCStringGetLength (t_uuid);

	/* Sanity-check UUID. Must be 36-character string. We assume that
	 * it has already been processed into canonical format. */
	/* FIXME Maybe these should be "bad state" errors. */
	MCAssert (36 == t_uuid_len);

	/* First step is to create the LSB directory. */
	MCAutoStringRef t_uuid_lsb, t_lsb_dir;
	/* UNCHECKED */ MCStringCopySubstring (t_uuid,
										   MCRangeMake (t_uuid_len - 3, 2),
										   &t_uuid_lsb);
	/* UNCHECKED */ MCStringFormat (&t_lsb_dir, "%@/%@",
									info->m_op->m_save_build_dir, *t_uuid_lsb);

	if (!MCS_mkdir (*t_lsb_dir))
	{
		/* Test whether creating the directory failed because it
		 * already existed (that's okay) */
		if (!MCS_exists (*t_lsb_dir, false))
			return MCStackdirIOErrorIO (info->m_op, *t_lsb_dir,
										MCSTR ("Failed to create object directory"));
	}

	/* Now create object directory within */
	MCAutoStringRef t_object_dir;
	/* UNCHECKED */ MCStringFormat (&t_object_dir, "%@/%@", *t_lsb_dir, t_uuid);

	if (!MCS_mkdir (*t_object_dir))
		/* Here we *require* the object directory not to exist yet. */
		return MCStackdirIOErrorIO (info->m_op, *t_lsb_dir,
									MCSTR ("Failed to create object directory"));

	info->m_path = MCValueRetain (*t_object_dir);
	return true;
}

static bool
MCStackdirIOSaveObjectKind (MCStackdirIOObjectSaveRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOSaveObjectParent (MCStackdirIOObjectSaveRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOSaveObjectInternal (MCStackdirIOObjectSaveRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOSaveObjectPropsets (MCStackdirIOObjectSaveRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOSaveObjectShared (MCStackdirIOObjectSaveRef info)
{
	/* FIXME implementation */
	return true;
}

static bool
MCStackdirIOSaveObject (MCStackdirIORef op, MCNameRef p_uuid, MCArrayRef p_state)
{
	bool t_success = true;
	MCStackdirIOObjectSaveRef info;

	/* Create and initialise object information structure */
	t_success = MCMemoryNew (info);
	if (!t_success)
		return MCStackdirIOErrorOutOfMemory (op);

	info->m_op = op;
	info->m_uuid = MCValueRetain (p_uuid);
	info->m_state = MCValueRetain (p_state);
	info->m_path = nil;

	/* Create object directory */
	t_success = t_success && MCStackdirIOSaveObjectDirectory (info);

	/* Generate object contents */
	t_success = (t_success &&
				 MCStackdirIOSaveObjectKind (info) &&
				 MCStackdirIOSaveObjectParent (info) &&
				 MCStackdirIOSaveObjectInternal (info) &&
				 MCStackdirIOSaveObjectPropsets (info) &&
				 MCStackdirIOSaveObjectShared (info));

	/* Clean up */
	MCValueRelease (info->m_uuid);
	MCValueRelease (info->m_state);
	MCValueRelease (info->m_path);
	MCMemoryDelete (info);

	return t_success;
}

/* ================================================================
 * Version information
 * ================================================================ */

static bool
MCStackdirIOSaveVersion (MCStackdirIORef op)
{
	MCAutoStringRef t_version_path;
	/* UNCHECKED */ MCStringFormat (&t_version_path, "%@/%@",
									op->m_save_build_dir,
									kMCStackdirVersionFile);

	MCAutoDataRef t_version_data;
	/* UNCHECKED */ MCStringEncode (kMCStackdirMagicString,
									kMCStringEncodingUTF8,
									false, &t_version_data);

	if (!MCS_savebinaryfile (*t_version_path, *t_version_data))
		/* FIXME record proper error information */
		return false;

	return true;
}

/* ================================================================
 * High-level entry points
 * ================================================================ */

/* ----------------------------------------------------------------
 * [Private] High-level operations
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOCommitSave_ObjectCallback (void *context, MCArrayRef p_array,
									   MCNameRef p_key, MCValueRef p_value)
{
	MCStackdirIORef t_op = (MCStackdirIORef) context;

	/* The object state should always be an array and the key should
	 * always be a non-empty string. */
	MCAssert (!MCNameIsEmpty (p_key));
	MCAssert (MCValueIsArray (p_value));
	MCArrayRef t_state = (MCArrayRef) p_value;

	return MCStackdirIOSaveObject (t_op, p_key, t_state);
}

void
MCStackdirIOCommitSave (MCStackdirIORef op)
{
	bool t_success = true;

	MCStackdirIOAssertSave(op);

	/* Sanity check the state that we're supposed to be saving. */
	MCAssert (op->m_save_state);

	if (!MCStackdirIOSaveTransactionStart (op)) return;

	/* Save version info and all objects. */
	t_success = t_success && MCStackdirIOSaveVersion (op);

	t_success = t_success && MCArrayApply (op->m_save_state,
										   MCStackdirIOCommitSave_ObjectCallback,
										   op);

	if (t_success)
		MCStackdirIOSaveTransactionEnd (op);
	else
		MCStackdirIOSaveTransactionCancel (op);
}

/* ----------------------------------------------------------------
 * [Public] High-level operations
 * ---------------------------------------------------------------- */

void
MCStackdirIOSetState (MCStackdirIORef op, MCArrayRef p_state)
{
	MCStackdirIOAssertSave (op);
	MCValueRelease (op->m_save_state);
	op->m_save_state = MCValueRetain (p_state);
}

bool
MCStackdirIONewSave (MCStackdirIORef & op)
{
	if (!MCStackdirIONew (op)) return false;
	op->m_type = kMCStackdirIOTypeSave;
	return true;
}

/* ----------------------------------------------------------------
 * [Public] Internal debugging commands
 * ---------------------------------------------------------------- */

/* _internal stackdir save <state> to <path> */

void
MCStackdirExecInternalSave (MCExecContext & ctxt, MCStringRef p_path,
							MCArrayRef p_state_array)
{
	/* Run save transaction */
	MCStackdirIORef t_op;
	/* UNCHECKED */ MCStackdirIONewSave (t_op);
	MCStackdirIOSetPath (t_op, p_path);
	MCStackdirIOSetState (t_op, p_state_array);

	MCStackdirIOCommit (t_op);

	MCArrayRef t_error_info = nil;
	MCStackdirStatus t_status;
	t_status = MCStackdirIOGetStatus (t_op, &t_error_info);

	if (t_status == kMCStackdirStatusSuccess)
	{
		ctxt.SetTheResultToBool(true);
	}
	else
	{
		ctxt.SetTheResultToValue (t_error_info);
		MCValueRelease (t_error_info);
	}

	MCStackdirIODestroy (t_op);
}
