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

	/* Shared properties */
	MCListRef m_shared;
	MCStringRef m_shared_path;
};

typedef struct _MCStackdirIOPropsetSave MCStackdirIOPropsetSave;
typedef MCStackdirIOPropsetSave *MCStackdirIOPropsetSaveRef;

struct _MCStackdirIOPropsetSave
{
	MCStackdirIORef m_op;

	/* Propset directory */
	MCStringRef m_path;

	/* _contents property descriptors */
	MCListRef m_contents;
	MCStringRef m_contents_path;

	/* _overflow property descriptors */
	MCListRef m_overflow;
	MCStringRef m_overflow_path;
};

typedef struct _MCStackdirIOArraySave MCStackdirIOArraySave;
typedef MCStackdirIOArraySave *MCStackdirIOArraySaveRef;

struct _MCStackdirIOArraySave
{
	MCStackdirIORef m_op;

	/* Storage spec prefix */
	MCStringRef m_storage_spec;

	/* Target list */
	MCListRef m_list;
};

typedef struct _MCStackdirIOSharedSave MCStackdirIOSharedSave;
typedef MCStackdirIOSharedSave *MCStackdirIOSharedSaveRef;

struct _MCStackdirIOSharedSave
{
	MCStackdirIORef m_op;

	/* Object directory */
	MCStringRef m_path;

	/* Target list */
	MCListRef m_list;

	/* .shared directory */
	MCStringRef m_external_dir;
};

typedef struct _MCStackdirIORemoveFolderRecursiveContext MCStackdirIORemoveFolderRecursiveContext;
typedef MCStackdirIORemoveFolderRecursiveContext *MCStackdirIORemoveFolderRecursiveContextRef;

struct _MCStackdirIORemoveFolderRecursiveContext
{
	MCStackdirIORef m_op;
	MCStringRef m_parent_dir;
};

typedef struct _MCStackdirIOArrayGetPropertyInfoContext MCStackdirIOArrayGetPropertyInfoContext;
typedef MCStackdirIOArrayGetPropertyInfoContext *MCStackdirIOArrayGetPropertyInfoContextRef;

struct _MCStackdirIOArrayGetPropertyInfoContext
{
	MCStackdirIORef m_op;
	MCArrayRef m_flags;
};

/* ----------------------------------------------------------------
 * [Private] Utility functions
 * ---------------------------------------------------------------- */

/* Recursively delete a directory */
static bool MCStackdirIORemoveFolderRecursive (MCStackdirIORef op, MCStringRef p_path);

/* Split up a property record */
static bool MCStackdirIOArrayGetPropertyInfo (MCStackdirIORef op, MCValueRef p_info, MCNameRef & r_type, MCArrayRef & r_flags, MCValueRef & r_literal);

/* Format property flags */
static bool MCStackdirIOSaveFormatFlags (MCStackdirIORef op, MCArrayRef p_flags, MCStringRef & r_formatted);

/* Test whether a property record contains a specific flag */
static bool MCStackdirIOPropertyInfoHasFlag (MCStackdirIORef op, MCValueRef p_info, MCNameRef p_flag);

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

/* Extract the value corresponding to p_key from the object's state
 * array, and save it to a file with name p_key. */
static bool MCStackdirIOSaveObjectKeyDirect (MCStackdirIOObjectSaveRef info, MCStringRef p_key, bool p_required=true);

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

/* Recursively format property descriptors for array contents.
 *
 * The p_storage_spec is prepended to the name of each entry in the
 * array.  It does not need to include a trailing ":".  The
 * p_storage_spec is permitted to be nil or empty.  N.b. the
 * p_storage_spec must be in *on-disk* format.
 *
 * The p_path is used to generate error messages if a problem occurs.
 */
static bool MCStackdirIOSaveArrayDescriptorsRecursive (MCStackdirIORef op, MCArrayRef p_array, MCStringRef p_storage_spec, MCListRef r_descriptor_list);

/* Format an immediate property descriptor.
 *
 * The p_type_spec is permitted to be nil or empty.
 */
static bool MCStackdirIOSaveImmediateDescriptor (MCStackdirIORef op, MCStringRef p_storage_spec, MCStringRef p_type_spec, MCStringRef p_flags, MCStringRef p_literal, MCStringRef & r_descriptor);

/* Format an external property descriptor.
 *
 * The p_type_spec and the p_file_name are permitted to be nil or empty.
 */
static bool MCStackdirIOSaveExternalDescriptor (MCStackdirIORef op, MCStringRef p_storage_spec, MCStringRef p_type_spec, MCStringRef p_flags, MCStringRef p_file_type, MCStringRef & r_descriptor);

/* Create a property descriptor and add it to a property list.
 *
 * The p_external_path is the path in which any excessively large
 * properties should be stored as external files.  If it is nil, no
 * properties will be saved into external files.
 *
 * The x_overflow_list is used for excessively large properties that
 * can't be saved into external files.  If the x_overflow_list is nil,
 * these troublesome values will be stored in the x_list.
 */
static bool MCStackdirIOSaveProperty (MCStackdirIORef op, MCNameRef p_name, MCValueRef p_value, MCStringRef p_external_dir, MCListRef x_list, MCListRef x_overflow_list);

/* Save a property set.
 *
 * Generates "_contents" and/or "_overflow" files in p_propset_path,
 * along with any necessary external storage files. */
static bool MCStackdirIOSavePropSet (MCStackdirIORef op, MCArrayRef p_propset, MCStringRef p_propset_path);

/* ================================================================
 * Errors
 * ================================================================ */

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorRecursiveDeleteFile,
							kMCStackdirStatusIOError,
							"Failed to delete file")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorRecursiveDeleteDirectory,
							kMCStackdirStatusIOError,
							"Failed to delete directory")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorResolveTransactionPath,
					   kMCStackdirStatusIOError,
					   "Failed to resolve transaction path");
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorUniqueTransactionPath,
					   kMCStackdirStatusIOError,
					   "Failed to generate unique transaction path");
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorCreateTransactionDir,
							kMCStackdirStatusIOError,
							"Failed to create unique transaction directory");

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorRestoreOverwrite,
							kMCStackdirStatusIOError,
							"Failed to move original stackdir back to target location");
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorDeleteBackupDirectory,
							kMCStackdirStatusIOError,
							"Failed to remove overwrite backup directory");

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorCannotOverwrite,
					   kMCStackdirStatusBadPath,
					   "Target exists and is not a stackdir");
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorRenameOverwrite,
					   kMCStackdirStatusIOError,
					   "Failed to move original stackdir to backup directory")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorRenameComplete,
					   kMCStackdirStatusIOError,
					   "Failed to move new stackdir into place")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorBadTypeInfo,
					   kMCStackdirStatusBadState,
					   "Object state contains invalid value info")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatArrayPropertyName,
					   kMCStackdirStatusBadState,
					   "Failed to format name of array member")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatArrayPropertyType,
					   kMCStackdirStatusBadState,
					   "Failed to format type of array member")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatArrayPropertyValue,
					   kMCStackdirStatusBadState,
					   "Failed to format value of array member")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatPropertyName,
					   kMCStackdirStatusBadState,
					   "Failed to format property name")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatPropertyType,
					   kMCStackdirStatusBadState,
					   "Failed to format property type")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorFormatPropertyValue,
					   kMCStackdirStatusBadState,
					   "Failed to format property value")

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorCreateExternalDir,
							kMCStackdirStatusIOError,
							"Failed to create directory for externally-stored values")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorWriteExternalFile,
							kMCStackdirStatusIOError,
							"Failed to write externally-stored value file")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorInvalidObjectUUID,
					   kMCStackdirStatusBadState,
					   "Object has invalid UUID");
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorCreateObjectLSBDir,
							kMCStackdirStatusIOError,
							"Failed to create object LSB directory")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorCreateObjectDir,
							kMCStackdirStatusIOError,
							"Failed to create object directory")

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorObjectRequiredKey,
							kMCStackdirStatusBadState,
							"Object state lacks required key");

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorObjectMissingInternal,
					   kMCStackdirStatusBadState,
					   "Object state lacks internal property array");
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorObjectInvalidInternal,
					   kMCStackdirStatusBadState,
					   "Invalid internal property array");

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorInvalidPropsetName,
					   kMCStackdirStatusBadState,
					   "Custom propset has unsupported name");
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOSaveErrorCreatePropsetDir,
							kMCStackdirStatusIOError,
							"Failed to create custom propset directory")

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorObjectInvalidCustom,
					   kMCStackdirStatusBadState,
					   "Invalid custom propsets array");

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorInvalidSharedCardUUID,
					   kMCStackdirStatusBadState,
					   "Invalid card UUID for shared properties")
MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorInvalidSharedCardArray,
					   kMCStackdirStatusBadState,
					   "Invalid per-card array of shared properties");

MC_STACKDIR_ERROR_FUNC(MCStackdirIOSaveErrorObjectInvalidShared,
					   kMCStackdirStatusBadState,
					   "Invalid shared properties array");

/* ================================================================
 * Utility functions
 * ================================================================ */

static bool
MCStackdirIORemoveFolderRecursive_Callback (void *p_context,
											const MCSystemFolderEntry *p_entry)
{
	MCStackdirIORemoveFolderRecursiveContextRef t_context =
		(MCStackdirIORemoveFolderRecursiveContextRef) p_context;
	MCStackdirIORef t_op = t_context->m_op;

	if (MCStringIsEqualTo (p_entry->name, MCSTR (".."),
						   kMCStringOptionCompareExact) ||
		MCStringIsEqualTo (p_entry->name, MCSTR ("."),
						   kMCStringOptionCompareExact))
		return true;

	MCAutoStringRef t_path;
	if (!MCStringFormat (&t_path, "%@/%@",
						 t_context->m_parent_dir, p_entry->name))
		return MCStackdirIOErrorOutOfMemory (t_op);

	if (p_entry->is_folder)
	{
		/* Recurse */
		return MCStackdirIORemoveFolderRecursive (t_op, *t_path);
	}
	else
	{
		if (!MCS_unlink (*t_path))
			return MCStackdirIOSaveErrorRecursiveDeleteFile (t_op, *t_path);
	}
	return true;
}

static bool
MCStackdirIORemoveFolderRecursive (MCStackdirIORef op, MCStringRef p_path)
{
	MCAutoStringRef t_save_cwd;
	bool t_success = true;

	/* Loop over directory entries */
	if (t_success)
	{
		MCStackdirIORemoveFolderRecursiveContext info;
		info.m_op = op;
		info.m_parent_dir = p_path;

		MCAutoStringRef t_native_path;
		/* UNCHECKED */ MCS_pathtonative (p_path, &t_native_path);
		t_success = MCsystem->ListFolderEntries (*t_native_path,
								MCStackdirIORemoveFolderRecursive_Callback,
								&info);
	}

	/* Attempt to remove the directory itself. */
	if (t_success && !MCS_rmdir (p_path))
		return MCStackdirIOSaveErrorRecursiveDeleteDirectory (op, p_path);

	return t_success;
}

static bool
MCStackdirIOArrayGetPropertyInfo_Callback (void *context,
										   MCArrayRef array,
										   MCNameRef p_key,
										   MCValueRef p_value)
{
	MCStackdirIOArrayGetPropertyInfoContextRef info =
		(MCStackdirIOArrayGetPropertyInfoContextRef) context;

	/* Keys beginning with "_" aren't flag names */
	if (MCStringBeginsWithCString (MCNameGetString (p_key),
								   (const char_t *) "_",
								   kMCStringOptionCompareExact))
		return true;

	/* Other keys *are* flag names */
	index_t t_index = MCArrayGetCount (info->m_flags) + 1;
	if (!MCArrayStoreValueAtIndex (info->m_flags, t_index, p_key))
		return MCStackdirIOErrorOutOfMemory (info->m_op);
	return true;
}

static bool
MCStackdirIOArrayGetPropertyInfo (MCStackdirIORef op,
								  MCValueRef p_info,
								  MCNameRef & r_type,
								  MCArrayRef & r_flags,
								  MCValueRef & r_literal)
{
	/*
	 * If the property information (p_info) is an array, it must have
	 * three keys:
	 *
	 * - p_info["_type"] should be an MCNameRef with the type of the
	 *   property's value.  This might be kMCEmptyString if the
	 *   property is of a basic type.
	 *
	 * - p_info["_literal"] could be any basic type, and has to be
	 *   formatted correctly.
	 *
	 * - All keys in the array not starting with "_" are interpreted
	 *   as flag names.
	 *
	 * All other types have the type inferred to be an empty string.
	 */

	if (!MCValueIsArray (p_info))
	{
		r_literal = MCValueRetain (p_info);
		r_flags = MCValueRetain (kMCEmptyArray);
		r_type = MCValueRetain (kMCEmptyName);
		return true;
	}

	MCArrayRef t_array = (MCArrayRef) p_info;
	MCValueRef t_type, t_literal;
	if (!(MCArrayFetchValue (t_array, false, kMCStackdirTypeKey, t_type) &&
		  MCArrayFetchValue (t_array, false, kMCStackdirLiteralKey, t_literal) &&
		  MCValueGetTypeCode (t_type) == kMCValueTypeCodeName))
		return MCStackdirIOSaveErrorBadTypeInfo (op);

	MCAutoArrayRef t_flags;
	if (!MCArrayCreateMutable (&t_flags))
		return MCStackdirIOErrorOutOfMemory (op);

	MCStackdirIOArrayGetPropertyInfoContext t_flags_context;
	t_flags_context.m_op = op;
	t_flags_context.m_flags = *t_flags;
	if (!MCArrayApply (t_array,
					   MCStackdirIOArrayGetPropertyInfo_Callback,
					   &t_flags_context))

		return false;

	r_type = (MCNameRef) MCValueRetain (t_type);
	r_flags = MCValueRetain (*t_flags);
	r_literal = MCValueRetain (t_literal);
	return true;
}

static bool
MCStackdirIOSaveFormatFlags (MCStackdirIORef op,
							 MCArrayRef p_flags,
							 MCStringRef & r_formatted)
{
	index_t t_num_flags = MCArrayGetCount (p_flags);

	if (0 == t_num_flags)
	{
		r_formatted = MCValueRetain (kMCEmptyString);
		return true;
	}

	if (!MCStringCreateMutable (0, r_formatted))
		return MCStackdirIOErrorOutOfMemory (op);

	for (index_t t_index = 1; t_index <= t_num_flags; ++t_index)
	{
		bool t_success;
		MCValueRef t_flag;
		t_success = MCArrayFetchValueAtIndex (p_flags, t_index, t_flag);
		/* Ill-formed array is stackdir programming error */
		MCAssert (t_success);

		MCAutoStringRef t_formatted_flag;
		if (!(MCStackdirFormatLiteral ((MCNameRef) t_flag,
									   &t_formatted_flag) &&
			  MCStringAppendFormat (r_formatted, "!%@",
									*t_formatted_flag)))
			return MCStackdirIOErrorOutOfMemory (op);

		/* Add a space if there are still more flags */
		if (t_index < t_num_flags)
		{
			if (!MCStringAppendChar (r_formatted, ' '))
				return MCStackdirIOErrorOutOfMemory (op);
		}
	}

	/* Remove the trailing space character */
	return true;
}

static bool
MCStackdirIOPropertyInfoHasFlag (MCStackdirIORef op,
								 MCValueRef p_info,
								 MCNameRef p_flag)
{
	if (!MCValueIsArray (p_info)) return false;

	MCArrayRef t_array = (MCArrayRef) p_info;

	MCValueRef t_ignored;
	return MCArrayFetchValue (t_array, true, p_flag, t_ignored);
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
		return MCStackdirIOSaveErrorResolveTransactionPath (op);

	/* If the target path ends in a "/", remove it */
	if (MCStringEndsWithCString (*t_resolved_path, (const char_t *) "/",
								 kMCStringOptionCompareExact))
	{
		uindex_t len = MCStringGetLength (*t_resolved_path) - 1;
		if (!MCStringCopySubstring (*t_resolved_path, MCRangeMake (0, len),
									&t_base_path))
			return MCStackdirIOErrorOutOfMemory (op);
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
		uint32_t t_suffix_numeric = 0;
		MCAutoStringRef t_native_path;

		/* Create a random number */
		if (!MCU_random_bytes (sizeof (t_suffix_numeric), &t_suffix_data))
			return MCStackdirIOSaveErrorUniqueTransactionPath (op);
		for (size_t i = 0; i < sizeof (t_suffix_numeric); ++i)
		{
			t_suffix_numeric += MCDataGetByteAtIndex (*t_suffix_data, i) << i;
		}

		/* Generate directory path */
		if (!MCStringFormat (&t_temp_path, "%@.temp%x",
							 *t_base_path, t_suffix_numeric))
			return MCStackdirIOErrorOutOfMemory (op);

		/* Attempt to create directory */
		if (MCS_mkdir (*t_temp_path))
			break;

		/* If creating the folder failed despite the fact it doesn't
		 * exist, there must be some other IO error. */
		if (!(MCS_exists (*t_temp_path, true) ||
			  MCS_exists (*t_temp_path, false)))
			return MCStackdirIOSaveErrorCreateTransactionDir (op, *t_temp_path);
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
			MCStackdirIOSaveErrorRestoreOverwrite (op, op->m_save_backup_path);
			t_success = false;
		}
	}

	/* If we created the backup directory, attempt to delete it.  Note
	 * that this will fail if it has contents, and *that's okay*,
	 * because it'll have contents if we copied the original data into
	 * it and failed to put it back. */
	if (op->m_save_backup_dir != nil &&
		!MCS_rmdir (op->m_save_backup_dir))
		MCStackdirIOSaveErrorDeleteBackupDirectory (op, op->m_save_build_dir);
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
		return MCStackdirIOSaveErrorCannotOverwrite (op);
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

		if (!MCStringFormat (&t_backup_path, "%@/%@",
							 *t_backup_dir, kMCStackdirSaveBackupDir))
		{
			MCStackdirIOSaveTransactionCancel (op);
			return MCStackdirIOErrorOutOfMemory (op);
		}

		if (!MCS_rename (op->m_path, *t_backup_path))
		{
			MCStackdirIOSaveTransactionCancel (op);
			return MCStackdirIOSaveErrorRenameOverwrite (op);
		}
		op->m_save_backup_path = MCValueRetain (*t_backup_path);
	}

	/* Move the newly-written bundle into place */
	if (!MCS_rename (op->m_save_build_dir, op->m_path))
	{
		/* Oops! That should have worked!  During cancellation, we'll
		 * try to put the original data back. */
		MCStackdirIOSaveTransactionCancel (op);
		return MCStackdirIOSaveErrorRenameComplete (op);
		return false;
	}

	/* Now delete the temporary directory we created for the original
	 * data.  Note that even if this fails, it's only a warning; the
	 * save has actually been successful and the user's data is
	 * safe. */
	if (t_overwriting)
	{
		MCStackdirStatus t_op_status = op->m_status;
		/* INTENTIONALLY UNCHECKED */ MCStackdirIORemoveFolderRecursive (op, *t_backup_dir);

		op->m_status = t_op_status; /* Reset the status */
	}

	MCStackdirIOSaveTransactionCleanup (op);
	return true;
}

/* ================================================================
 * Property descriptors
 * ================================================================ */

static bool
MCStackdirIOSaveArrayDescriptorsRecursive_Callback (void *context,
										MCArrayRef p_array,
										MCNameRef p_key,
										MCValueRef p_value)
{
	MCAssert (context != nil);
	MCAssert (p_array != nil);
	MCAssert (p_key != nil);
	MCAssert (p_value != nil);

	MCStackdirIOArraySaveRef info = (MCStackdirIOArraySaveRef) context;

	/* Extract property type and literal from the p_value */
	MCNameRef t_prop_name = p_key;
	MCNewAutoNameRef t_prop_type;
	MCAutoArrayRef t_prop_flags;
	MCAutoValueRef t_prop_literal;

	if (!MCStackdirIOArrayGetPropertyInfo (info->m_op,
										   p_value,
										   &t_prop_type,
										   &t_prop_flags,
										   &t_prop_literal))
		return false;

	/* Format parts of property descriptor */
	MCAutoStringRef t_name, t_type_spec, t_flags, t_literal;

	if (!MCStackdirFormatLiteral (t_prop_name, &t_name))
		return MCStackdirIOSaveErrorFormatArrayPropertyName (info->m_op);
	if (!MCStackdirIOSaveFormatFlags (info->m_op, *t_prop_flags, &t_flags))
		return false;
	if (!MCStackdirFormatLiteral (*t_prop_literal, &t_literal))
		return MCStackdirIOSaveErrorFormatArrayPropertyValue (info->m_op);

	/* Need to cope with the fact that the type spec is permitted to
	 * be omitted if the value is a basic type */
	if (MCNameIsEmpty (*t_prop_type))
	{
		if (!MCStringCopy (kMCEmptyString, &t_type_spec))
			return MCStackdirIOErrorOutOfMemory (info->m_op);
	}
	else if (!MCStackdirFormatLiteral (*t_prop_type, &t_type_spec))
		return MCStackdirIOSaveErrorFormatArrayPropertyType (info->m_op);

	/* Create the storage spec */
	MCAutoStringRef t_storage_spec;
	if (!(MCStringMutableCopy (info->m_storage_spec, &t_storage_spec) &&
		  MCStringAppend (*t_storage_spec, *t_name)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Format the property descriptor and add it to the target list*/
	MCAutoStringRef t_property_descriptor;
	if (!MCStackdirIOSaveImmediateDescriptor (info->m_op,
											  *t_storage_spec,
											  *t_type_spec,
											  *t_flags,
											  *t_literal,
											  &t_property_descriptor))
		return false;

	/* Add the property descriptor to the target list */
	if (!MCListAppend (info->m_list, *t_property_descriptor))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* If the literal was an array, recurse */
	if (MCValueGetTypeCode (*t_prop_literal) == kMCValueTypeCodeArray)
		return MCStackdirIOSaveArrayDescriptorsRecursive (info->m_op,
														  (MCArrayRef) *t_prop_literal,
														  *t_storage_spec,
														  info->m_list);

	return true;
}

static bool
MCStackdirIOSaveArrayDescriptorsRecursive (MCStackdirIORef op,
										   MCArrayRef p_array,
										   MCStringRef p_storage_spec,
										   MCListRef r_descriptor_list)
{
	MCAssert (op != nil);
	MCAssert (p_array != nil);
	MCAssert (r_descriptor_list != nil);

	if (p_storage_spec == nil) p_storage_spec = kMCEmptyString;

	bool t_success;
	t_success = true;
	
	/* If the storage spec is non-empty, then it's necessary to add a
	 * separator to the end of it. */
	MCAutoStringRef t_storage_spec;

	if (t_success)
		t_success = MCStringMutableCopy (p_storage_spec, &t_storage_spec);
	if (t_success &&
		!MCStringIsEmpty (*t_storage_spec))
		t_success = MCStringAppendCodepoint (*t_storage_spec, ':');

	if (!t_success)
		return MCStackdirIOErrorOutOfMemory (op);

	MCStackdirIOArraySave info;
	info.m_op = op;
	info.m_storage_spec = *t_storage_spec;
	info.m_list = r_descriptor_list;

	return MCStackdirArrayApplySorted (p_array,
									   MCStackdirIOSaveArrayDescriptorsRecursive_Callback,
									   &info);
}


static bool
MCStackdirIOSaveImmediateDescriptor (MCStackdirIORef op,
									 MCStringRef p_storage_spec,
									 MCStringRef p_type_spec,
									 MCStringRef p_flags,
									 MCStringRef p_literal,
									 MCStringRef & r_descriptor)
{
	MCAutoListRef t_list;
	bool t_success = true;

	if (t_success)
		t_success = (MCListCreateMutable (' ', &t_list) &&
					 MCListAppend (*t_list, p_storage_spec));

	/* Lexical analysis:
	 *
	 *    STORAGE_SPEC WS [TYPE_SPEC WS] LITERAL
	 */
	bool t_has_type = (p_type_spec != nil &&
					   !MCStringIsEmpty (p_type_spec));
	bool t_has_flags = !MCStringIsEmpty (p_flags);

	if (t_success && t_has_type)
		t_success = MCListAppend (*t_list, p_type_spec);
	if (t_success && t_has_flags)
		t_success = MCListAppend (*t_list, p_flags);
	if (t_success)
		t_success = MCListAppend (*t_list, p_literal);

	if (t_success)
		t_success = MCListCopyAsString (*t_list, r_descriptor);

	if (!t_success)
		MCStackdirIOErrorOutOfMemory (op);
	return t_success;
}

static bool
MCStackdirIOSaveExternalDescriptor (MCStackdirIORef op,
									MCStringRef p_storage_spec,
									MCStringRef p_type_spec,
									MCStringRef p_flags,
									MCStringRef p_file_type,
									MCStringRef & r_descriptor)
{
	MCAutoListRef t_list;
	MCAutoStringRef t_hash;
	bool t_success;
	t_success = (MCListCreateMutable (' ', &t_list) &&
				 MCListAppend (*t_list, p_storage_spec));

	/* Lexical analysis:
	 *
	 *    STORAGE_SPEC WS [TYPE_SPEC WS] "&" WS FILE_TYPE
	 */
	bool t_has_type = (p_type_spec != nil &&
					   !MCStringIsEmpty (p_type_spec));
	bool t_has_flags = !MCStringIsEmpty (p_flags);

	if (t_success && t_has_type)
		t_success = MCListAppend (*t_list, p_type_spec);
	if (t_success && t_has_flags)
		t_success = MCListAppend (*t_list, p_flags);
	if (t_success)
		t_success = (MCListAppendCString (*t_list, "&") &&
					 MCListAppend (*t_list, p_file_type));

	if (t_success)
		t_success = MCListCopyAsString (*t_list, r_descriptor);

	if (!t_success)
		MCStackdirIOErrorOutOfMemory (op);
	return t_success;
}

/* Choose how to store the property descriptor. There are several
 * rules.  Properties that cannot be stored in the _contents file
 * are evaluated for suitability for external storage.  Properties
 * that cannot be stored externally end up in the _overflow
 * file. */
enum MCStackdirIOPropertyTarget
{
	kMCStackdirIOPropertyTargetContents,
	kMCStackdirIOPropertyTargetExternal,
	kMCStackdirIOPropertyTargetOverflow,

	kMCStackdirIOPropertyTargetMaxDescriptorLen = 78,
	kMCStackdirIOPropertyTargetMaxValueLen = 40,
};

static bool
MCStackdirIOSaveProperty (MCStackdirIORef op,
						  MCNameRef p_name,
						  MCValueRef p_value,
						  MCStringRef p_external_dir,
						  MCListRef x_list,
						  MCListRef x_overflow_list)
{
	/* Check where the property descriptor should end up and stick it
	 * into the correct place.
	 */

	MCNameRef  t_prop_name = p_name;
	MCNewAutoNameRef  t_prop_type;
	MCAutoArrayRef t_prop_flags;
	MCAutoValueRef t_prop_literal;

	MCStringRef t_invalid_property_message =
		MCSTR ("Invalid property information array");

	/* First, extract the type and literal information from the
	 * p_value. */
	if (!MCStackdirIOArrayGetPropertyInfo (op, p_value,
										   &t_prop_type,
										   &t_prop_flags,
										   &t_prop_literal))
		return false;

	/* Next format each of the parts of the property descriptor */
	MCValueTypeCode t_prop_literal_typecode = MCValueGetTypeCode (*t_prop_literal);
	MCAutoStringRef t_storage_spec, t_type_spec, t_flags, t_literal;

	if (!MCStackdirFormatLiteral (t_prop_name, &t_storage_spec))
		return MCStackdirIOSaveErrorFormatPropertyName (op);
	if (!MCStackdirIOSaveFormatFlags (op, *t_prop_flags, &t_flags))
		return false;
	if (!MCStackdirFormatLiteral (*t_prop_literal, &t_literal))
		return MCStackdirIOSaveErrorFormatPropertyValue (op);

	/* Need to cope with the fact that the type spec is permitted to
	 * be omitted if the value is a basic type */
	if (MCNameIsEmpty (*t_prop_type))
	{
		if (!MCStringCopy (kMCEmptyString, &t_type_spec))
			return MCStackdirIOErrorOutOfMemory (op);
	}
	else if (!MCStackdirFormatLiteral (*t_prop_type, &t_type_spec))
		return MCStackdirIOSaveErrorFormatPropertyType (op);

	/* Decide where the property descriptor (and contents!) should end
	 * up */
	int t_target = kMCStackdirIOPropertyTargetContents;
	MCStringRef t_external_file_type;
	MCAutoDataRef t_external_file_contents;
	MCAutoStringRef t_external_filename;

	if (t_target == kMCStackdirIOPropertyTargetContents)
	{
		/* We can store in the _contents file if the encoded value
		 * isn't too big. */
		switch (t_prop_literal_typecode)
		{
		case kMCValueTypeCodeArray:
			/* Always attempt to save non-empty arrays externally */
			if (MCArrayGetCount ((MCArrayRef) *t_prop_literal) > 0)
				t_target = kMCStackdirIOPropertyTargetExternal;
			break;

		case kMCValueTypeCodeString:
		case kMCValueTypeCodeData:
			{
				/* Strings and data must have a sensible encoded length */
				uindex_t t_literal_len = MCStringGetLength (*t_literal);
				uindex_t t_descriptor_len = (t_literal_len +
											 MCStringGetLength (*t_storage_spec) +
											 MCStringGetLength (*t_flags) +
											 MCStringGetLength (*t_type_spec));

				if (t_literal_len > kMCStackdirIOPropertyTargetMaxValueLen &&
					t_descriptor_len > kMCStackdirIOPropertyTargetMaxDescriptorLen)
					t_target = kMCStackdirIOPropertyTargetExternal;

				/* If the "external" flag is present, always save
				 * externally. */
				if (MCStackdirIOPropertyInfoHasFlag (op, p_value,
													 kMCStackdirExternalFlag))
					t_target = kMCStackdirIOPropertyTargetExternal;
			}
			break;

		case kMCValueTypeCodeBoolean:
		case kMCValueTypeCodeNumber:
			/* Can always be stored in _contents */
			break;

		default:
			/* We shouldn't ever get here; all types other than the
			 * above should be weeded out by an earlier failure of
			 * MCStackdirFormatLiteral(). */
			MCUnreachable ();
		}
	}

	/* Only attempt external storage if an external path was
	 * provided. */
	if (t_target == kMCStackdirIOPropertyTargetExternal &&
		p_external_dir == nil)
		t_target = kMCStackdirIOPropertyTargetOverflow;

	if (t_target == kMCStackdirIOPropertyTargetExternal)
	{
		/* Check whether the name can be successfully encoded as a
		 * filename. If it can't, store it in _overflow. */
		MCStringRef t_suffix;
		switch (t_prop_literal_typecode)
		{
		case kMCValueTypeCodeData:
			t_suffix = kMCStackdirDataSuffix;
			break;
		case kMCValueTypeCodeString:
			t_suffix = kMCStackdirStringSuffix;
			break;
		case kMCValueTypeCodeArray:
			t_suffix = kMCStackdirArraySuffix;
			break;
		default:
			MCUnreachable ();
		}

		if (!MCStackdirFormatFilename (MCNameGetString (t_prop_name),
									   t_suffix, &t_external_filename))
			t_target = kMCStackdirIOPropertyTargetOverflow;
	}

	if (t_target == kMCStackdirIOPropertyTargetExternal)
	{
		/* Now that we're certain that external storage is possible,
		 * do some more expensive preparations and write the external
		 * file. */

		MCAutoStringRef t_external_path;
		if (!MCStringFormat (&t_external_path, "%@/%@", p_external_dir,
							 *t_external_filename))
			return MCStackdirIOErrorOutOfMemory (op);

		switch (t_prop_literal_typecode)
		{
		case kMCValueTypeCodeData:
			t_external_file_type = kMCStackdirDataType;
			/* UNCHECKED */ MCDataCopy ((MCDataRef) *t_prop_literal,
										&t_external_file_contents);
			break;
		case kMCValueTypeCodeString:
			t_external_file_type = kMCStackdirStringType;
			/* UNCHECKED */ MCStringEncode ((MCStringRef) *t_prop_literal,
											kMCStringEncodingUTF8,
											false, &t_external_file_contents);
			break;
		case kMCValueTypeCodeArray:
			t_external_file_type = kMCStackdirArrayType;
			{
				MCAutoListRef t_array_list;
				MCAutoStringRef t_array_string;
				/* UNCHECKED */ MCListCreateMutable ('\n', &t_array_list);
				if (!(MCStackdirIOSaveArrayDescriptorsRecursive (op,
											(MCArrayRef) *t_prop_literal,
											kMCEmptyString,
											*t_array_list) &&
					  MCListAppend (*t_array_list, kMCEmptyString)))
					return false;
				/* UNCHECKED */ MCListCopyAsString (*t_array_list,
													&t_array_string);
				/* UNCHECKED */ MCStringEncode (*t_array_string,
												kMCStringEncodingUTF8,
												false, &t_external_file_contents);
				break;
			}
			break;
		default:
			MCUnreachable ();
		}

		/* Attempt to create the external directory.  It's okay if it
		 * already exists. */
		if (!MCS_mkdir (p_external_dir) &&
			!MCS_exists (p_external_dir, false))
			return MCStackdirIOSaveErrorCreateExternalDir (op, p_external_dir);

		/* Save the external file */
		if (!MCS_savebinaryfile (*t_external_path, *t_external_file_contents))
			return MCStackdirIOSaveErrorWriteExternalFile (op, *t_external_path);
	}

	/* Only attempt overflow storage if an overflow list was provided */
	if (t_target == kMCStackdirIOPropertyTargetOverflow
		&& x_overflow_list == nil)
		t_target = kMCStackdirIOPropertyTargetContents;

	/* In theory, we've now decided how the property should be stored.
	 * Format a property descriptor of the correct type (immediate or
	 * external). */
	MCAutoStringRef t_property_descriptor;
	switch (t_target)
	{
	case kMCStackdirIOPropertyTargetExternal:
		/* UNCHECKED */ MCStackdirIOSaveExternalDescriptor (op,
							*t_storage_spec,
							*t_type_spec,
							*t_flags,
							t_external_file_type,
							&t_property_descriptor);
		break;
	case kMCStackdirIOPropertyTargetOverflow:
	case kMCStackdirIOPropertyTargetContents:
		/* UNCHECKED */ MCStackdirIOSaveImmediateDescriptor (op,
							*t_storage_spec,
							*t_type_spec,
							*t_flags,
							*t_literal,
							&t_property_descriptor);
		break;
	default:
		MCUnreachable ();
	}

	/* Add the property descriptor to the correct target file */
	if (t_target == kMCStackdirIOPropertyTargetOverflow)
		/* UNCHECKED */ MCListAppend (x_overflow_list, *t_property_descriptor);
	else
		/* UNCHECKED */ MCListAppend (x_list, *t_property_descriptor);

	/* If the value is an array and has internal storage, then add the
	 * property descriptors for its contents to the appropriate file
	 * too. */
	if (t_prop_literal_typecode == kMCValueTypeCodeArray &&
		t_target != kMCStackdirIOPropertyTargetExternal)
	{
		MCListRef t_list;
		switch (t_target)
		{
		case kMCStackdirIOPropertyTargetOverflow:
			t_list = x_overflow_list;
			break;
		case kMCStackdirIOPropertyTargetContents:
			t_list = x_list;
			break;
		default:
			MCUnreachable ();
		}

		if (!MCStackdirIOSaveArrayDescriptorsRecursive (op,
														(MCArrayRef) *t_prop_literal,
														*t_storage_spec,
														t_list))
			return false;
	}
	return true;
}

/* ================================================================
 * Property sets
 * ================================================================ */


static bool
MCStackdirIOSavePropSet_PropertyCallback (void *context,
										  MCArrayRef p_propset,
										  MCNameRef p_key,
										  MCValueRef p_value)
{
	MCStackdirIOPropsetSaveRef info = (MCStackdirIOPropsetSaveRef) context;

	return MCStackdirIOSaveProperty (info->m_op,
									 p_key,
									 p_value,
									 info->m_path,
									 info->m_contents,
									 info->m_overflow);
}

static bool
MCStackdirIOSavePropSet (MCStackdirIORef op, MCArrayRef p_propset,
						 MCStringRef p_propset_path)
{
	/* Fast path */
	if (MCArrayIsEmpty (p_propset)) return true;

	/* Create and initialise propset information structure */
	MCStackdirIOPropsetSave info;
	MCAutoListRef t_contents_list, t_overflow_list;
	MCAutoStringRef t_contents_path, t_overflow_path;
	if (!(MCStringFormat (&t_contents_path, "%@/%@",
						  p_propset_path, kMCStackdirContentsFile) &&
		  MCStringFormat (&t_overflow_path, "%@/%@",
						  p_propset_path, kMCStackdirOverflowFile) &&
		  MCListCreateMutable (kMCLineEndString, &t_contents_list) &&
		  MCListCreateMutable (kMCLineEndString, &t_overflow_list)))
		return MCStackdirIOErrorOutOfMemory (op);

	info.m_op = op;
	info.m_path = p_propset_path;
	info.m_contents_path = *t_contents_path;
	info.m_overflow_path = *t_overflow_path;
	info.m_contents = *t_contents_list;
	info.m_overflow = *t_overflow_list;

	/* Generate property descriptors */
	if (!MCStackdirArrayApplySorted (p_propset,
									 MCStackdirIOSavePropSet_PropertyCallback,
									 &info))
		return false;

	/* Write propset files. We append the empty string to each list in
	 * order to ensure that they contain a trailing newline
	 * character. */
	if (!MCListIsEmpty (*t_contents_list))
	{
		MCAutoStringRef t_contents;
		if (!(MCListAppend (*t_contents_list, kMCEmptyString) &&
			  MCListCopyAsString (*t_contents_list, &t_contents)))
			return MCStackdirIOErrorOutOfMemory (op);
		if (!MCStackdirIOSaveUTF8 (op, *t_contents_path, *t_contents))
			return false;
	}

	if (!MCListIsEmpty (*t_overflow_list))
	{
		MCAutoStringRef t_overflow;
		if (!(MCListAppend (*t_overflow_list, kMCEmptyString) &&
			  MCListCopyAsString (*t_overflow_list, &t_overflow)))
			return MCStackdirIOErrorOutOfMemory (op);
		if (!MCStackdirIOSaveUTF8 (op, *t_overflow_path, *t_overflow))
			return false;
	}

	return true;
}

/* ================================================================
 * Object saving
 * ================================================================ */

static bool
MCStackdirIOSaveObjectDirectory (MCStackdirIOObjectSaveRef info)
{
	/* Sanity-check UUID and make sure it's properly normalised. */
	MCAutoStringRef t_uuid;
	if (!(MCStackdirStringIsUUID (MCNameGetString (info->m_uuid)) &&
		  MCStackdirFormatFilename (MCNameGetString (info->m_uuid),
									kMCEmptyString, &t_uuid)))
		return MCStackdirIOSaveErrorInvalidObjectUUID (info->m_op);

	uindex_t t_uuid_len = MCStringGetLength (*t_uuid);

	/* First step is to create the LSB directory.  Just grab the last
	 * two characters of the UUID. */
	MCAutoStringRef t_uuid_lsb, t_lsb_dir, t_object_dir;
	if (!(MCStringCopySubstring (*t_uuid,
								 MCRangeMake (t_uuid_len - 2, 2),
								 &t_uuid_lsb) &&
		  MCStringFormat (&t_lsb_dir, "%@/%@",
						  info->m_op->m_save_build_dir, *t_uuid_lsb) &&
		  MCStringFormat (&t_object_dir, "%@/%@", *t_lsb_dir, *t_uuid)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* If creating the LSB directory fails but it
	 * already exists, that's okay */
	if (!MCS_mkdir (*t_lsb_dir) &&
		!MCS_exists (*t_lsb_dir, false))
		return MCStackdirIOSaveErrorCreateObjectLSBDir (info->m_op, *t_lsb_dir);

	/* Now create object directory within */
	if (!MCS_mkdir (*t_object_dir))
		/* Here we *require* the object directory not to exist yet. */
		return MCStackdirIOSaveErrorCreateObjectDir (info->m_op, *t_object_dir);

	info->m_path = MCValueRetain (*t_object_dir);
	return true;
}

static bool
MCStackdirIOSaveObjectKeyDirect (MCStackdirIOObjectSaveRef info, MCStringRef p_key, bool p_required)
{
	MCNewAutoNameRef t_key;
	MCValueRef t_value;
	if (!MCNameCreate (p_key, &t_key))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Construct path for file */
	MCAutoStringRef t_path;
	if (!MCStringFormat (&t_path, "%@/%@", info->m_path, p_key))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Each object's state array has to have the specified key */
	if (!MCArrayFetchValue (info->m_state, false, *t_key, t_value))
	{
		if (p_required)
			return MCStackdirIOSaveErrorObjectRequiredKey (info->m_op, *t_path);
		else
			return true;
	}

	MCAutoStringRef t_content_literal, t_content;
	if (!(MCStackdirFormatLiteral (t_value, &t_content_literal) &&
		  MCStringFormat (&t_content, "%@\n", *t_content_literal)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	return MCStackdirIOSaveUTF8 (info->m_op, *t_path, *t_content);
}

static bool
MCStackdirIOSaveObjectKind (MCStackdirIOObjectSaveRef info)
{
	return MCStackdirIOSaveObjectKeyDirect (info, kMCStackdirKindFile);
}

static bool
MCStackdirIOSaveObjectParent (MCStackdirIOObjectSaveRef info)
{
	return MCStackdirIOSaveObjectKeyDirect (info, kMCStackdirParentFile, false);
}

static bool
MCStackdirIOSaveObjectInternal (MCStackdirIOObjectSaveRef info)
{
	MCValueRef t_internal_props;

	/* The state array has to have an _internal array */
	if (!MCArrayFetchValue (info->m_state, false, kMCStackdirInternalKey,
							t_internal_props))
		return MCStackdirIOSaveErrorObjectMissingInternal (info->m_op);

	if (!MCValueIsArray (t_internal_props))
		return MCStackdirIOSaveErrorObjectInvalidInternal (info->m_op);

	/* Reuse the propset saving logic, saving into the object
	 * directory. */
	return MCStackdirIOSavePropSet (info->m_op,
									(MCArrayRef) t_internal_props,
									info->m_path);
}

static bool
MCStackdirIOSaveObjectPropsets_PropsetCallback (void *context, MCArrayRef p_array,
												MCNameRef p_key, MCValueRef p_value)
{
	MCStackdirIOObjectSaveRef info = (MCStackdirIOObjectSaveRef) context;

	/* The value should always be an array */
	MCAssert (MCValueIsArray (p_value));

	/* If there aren't actually any properties, don't write anything. */
	if (MCArrayIsEmpty ((MCArrayRef) p_value)) return true;

	/* The default custom propset is identified by the empty name, and
	 * gets given the special custom firectory name _empty. */
	MCAutoStringRef t_propset_filename, t_propset_path;
	if (MCNameIsEmpty (p_key))
	{
		if (!MCStringFormat (&t_propset_filename, "%@%@",
							 kMCStackdirEmptyFile,
							 kMCStackdirPropsetSuffix))
			return MCStackdirIOErrorOutOfMemory (info->m_op);
	}
	else
	{
		/* FIXME We don't yet support custom property sets with funny
		 * names. */
		if (!MCStackdirFormatFilename (MCNameGetString(p_key),
									   kMCStackdirPropsetSuffix,
									   &t_propset_filename))
			return MCStackdirIOSaveErrorInvalidPropsetName (info->m_op);
	}

	/* Create the propset directory */
	if (!MCStringFormat (&t_propset_path, "%@/%@", info->m_path,
						 *t_propset_filename))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	if (!MCS_mkdir (*t_propset_path))
		return MCStackdirIOSaveErrorCreatePropsetDir (info->m_op, *t_propset_path);

	bool t_success = true;
	if (t_success)
	{
		MCStackdirIOErrorLocationPush (info->m_op, *t_propset_path);
		t_success = MCStackdirIOSavePropSet (info->m_op,
											 (MCArrayRef) p_value,
											 *t_propset_path);
		MCStackdirIOErrorLocationPop (info->m_op);
	}

	return t_success;
}

static bool
MCStackdirIOSaveObjectPropsets (MCStackdirIOObjectSaveRef info)
{
	MCValueRef t_custom_propsets;

	/* The state array may have _custom array (even if it's empty) */
	if (!MCArrayFetchValue (info->m_state, false, kMCStackdirCustomKey,
							t_custom_propsets))
		return true;

	if (!MCValueIsArray (t_custom_propsets))
		return MCStackdirIOSaveErrorObjectInvalidCustom (info->m_op);

	/* Loop over all the property sets */
	return MCArrayApply ((MCArrayRef) t_custom_propsets,
						 MCStackdirIOSaveObjectPropsets_PropsetCallback,
						 info);
}

static bool
MCStackdirIOSaveObjectSharedHeader (MCStackdirIORef op, MCStringRef p_uuid,
									MCStringRef & r_header)
{
	if (!MCStringFormat (r_header, "[%@]", p_uuid))
		return MCStackdirIOErrorOutOfMemory (op);
	return true;
}

static bool
MCStackdirIOSaveObjectShared_PropCallback (void *context, MCArrayRef p_array,
										   MCNameRef p_key, MCValueRef p_value)
{
	MCStackdirIOSharedSaveRef info = (MCStackdirIOSharedSaveRef) context;

	return MCStackdirIOSaveProperty (info->m_op,
									 p_key,
									 p_value,
									 info->m_external_dir,
									 info->m_list,
									 nil);
}

static bool
MCStackdirIOSaveObjectShared_CardCallback (void *context, MCArrayRef p_array,
										   MCNameRef p_key, MCValueRef p_value)
{
	MCStackdirIOObjectSaveRef info = (MCStackdirIOObjectSaveRef) context;

	if (!MCValueIsArray (p_value))
		return MCStackdirIOSaveErrorInvalidSharedCardArray (info->m_op);

	MCArrayRef t_properties = (MCArrayRef) p_value;

	/* Fast path */
	if (MCArrayIsEmpty (t_properties)) return true;

	/* Sanity-check UUID and make sure it's properly normalised. */
	MCAutoStringRef t_uuid;
	if (!(MCStackdirStringIsUUID (MCNameGetString (p_key)) &&
		  MCStackdirFormatFilename (MCNameGetString (p_key),
									kMCEmptyString, &t_uuid)))
		return MCStackdirIOSaveErrorInvalidSharedCardUUID (info->m_op);

	/* Add a header to the _shared file */
	MCAutoStringRef t_header;
	if (!MCStackdirIOSaveObjectSharedHeader (info->m_op, *t_uuid, &t_header))
		return false;
	if (!MCListAppend (info->m_shared, *t_header))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Figure out the directory in which any external files should be
	 * stored. N.b. this'll get created only if needed. */
	MCAutoStringRef t_external_dir;
	if (!MCStringFormat (&t_external_dir, "%@/%@%@", info->m_path,
						 *t_uuid, kMCStackdirSharedSuffix))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Add the properties to the _shared file */
	MCStackdirIOSharedSave shared_info;
	shared_info.m_op = info->m_op;
	shared_info.m_list = info->m_shared;
	shared_info.m_path = info->m_shared_path;
	shared_info.m_external_dir = *t_external_dir;

	bool t_success = true;
	if (t_success)
	{
		MCStackdirIOErrorLocationPush (info->m_op, *t_external_dir);
		t_success = MCStackdirArrayApplySorted (t_properties,
												MCStackdirIOSaveObjectShared_PropCallback,
												&shared_info);
		MCStackdirIOErrorLocationPop (info->m_op);
	}
	return t_success;
}

static bool
MCStackdirIOSaveObjectShared (MCStackdirIOObjectSaveRef info)
{
	MCValueRef t_shared_properties;

	/* The state array may have a _shared array (even if it's empty) */
	if (!MCArrayFetchValue (info->m_state, false, kMCStackdirSharedKey,
							t_shared_properties))
		return true;

	if (!MCValueIsArray (t_shared_properties))
		return MCStackdirIOSaveErrorObjectInvalidShared (info->m_op);

	/* Create and initialize shared property information. */
	MCAutoListRef t_shared_list;
	MCAutoStringRef t_shared_path;
	if (!(MCListCreateMutable (kMCLineEndString, &t_shared_list) &&
		  MCStringFormat (&t_shared_path, "%@/%@", info->m_path,
						  kMCStackdirSharedFile)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	info->m_shared = *t_shared_list;
	info->m_shared_path = *t_shared_path;

	bool t_success = true;

	if (t_success)
	{
		MCStackdirIOErrorLocationPush (info->m_op, *t_shared_path);
		t_success = MCStackdirArrayApplySorted ((MCArrayRef) t_shared_properties,
												MCStackdirIOSaveObjectShared_CardCallback,
												info);
		MCStackdirIOErrorLocationPop (info->m_op);
	}
	if (!t_success) return false;

	/* Save the _shared file. Append an empty string to the list to
	 * ensure that the output file contains a trailing newline
	 * character. */
	if (!MCListIsEmpty (*t_shared_list))
	{
		MCAutoStringRef t_contents;
		if (!(MCListAppend (*t_shared_list, kMCEmptyString) &&
			  MCListCopyAsString (*t_shared_list, &t_contents)))
			return MCStackdirIOErrorOutOfMemory (info->m_op);
		if (!MCStackdirIOSaveUTF8 (info->m_op, *t_shared_path, *t_contents))
			return false;
	}

	return true;
}

static bool
MCStackdirIOSaveObject (MCStackdirIORef op, MCNameRef p_uuid, MCArrayRef p_state)
{
	bool t_success = true;
	MCStackdirIOObjectSave t_info;

	/* Create and initialise object information structure */
	t_info.m_op = op;
	t_info.m_uuid = p_uuid;
	t_info.m_state = p_state;
	t_info.m_path = nil;

	/* Create object directory */
	if (t_success)
		t_success = MCStackdirIOSaveObjectDirectory (&t_info);

	/* Generate object contents */
	if (t_success)
	{
		MCStackdirIOErrorLocationPush (op, t_info.m_path);
		t_success = (MCStackdirIOSaveObjectKind (&t_info) &&
					 MCStackdirIOSaveObjectParent (&t_info) &&
					 MCStackdirIOSaveObjectInternal (&t_info) &&
					 MCStackdirIOSaveObjectPropsets (&t_info) &&
					 MCStackdirIOSaveObjectShared (&t_info));
		MCStackdirIOErrorLocationPop (op);
	}

	/* Clean up */
	MCValueRelease (t_info.m_path);

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

	return MCStackdirIOSaveUTF8 (op, *t_version_path, kMCStackdirMagicString);
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
	MCAssert (op->m_path);

	MCStackdirIOErrorLocationPush (op, op->m_path);

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

	MCStackdirIOErrorLocationPop (op);
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

	MCAutoArrayRef t_error_info;
	MCStackdirStatus t_status;
	t_status = MCStackdirIOGetStatus (t_op, &(&t_error_info));

	if (t_status == kMCStackdirStatusSuccess)
	{
		ctxt.SetTheResultToBool(true);
	}
	else
	{
		ctxt.SetTheResultToValue (*t_error_info);
	}

	MCStackdirIODestroy (t_op);
}
