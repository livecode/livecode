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
#include "sha1.h"

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

	/* File that the array's being saved to */
	MCStringRef m_path;

	/* Target list */
	MCListRef m_list;
};

/* ----------------------------------------------------------------
 * [Private] Utility functions
 * ---------------------------------------------------------------- */

/* Recursively delete a directory */
static bool MCStackdirIORemoveFolderRecursive (MCStackdirIORef op, MCStringRef p_path);

/* Encode and write a UTF-8 text file */
static bool MCStackdirIOSaveUTF8 (MCStackdirIORef op, MCStringRef p_path, MCStringRef p_contents);

/* Generate a SHA-1 hash as a string */
static bool MCStackdirIODataSha1 (MCDataRef p_data, MCStringRef & p_hash);

/* Split up a property record */
static bool MCStackdirIOArrayGetPropertyInfo (MCValueRef p_info, MCNameRef & r_type, MCValueRef & r_literal);

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
static bool MCStackdirIOSaveObjectKeyDirect (MCStackdirIOObjectSaveRef info, MCStringRef p_key);

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
static bool MCStackdirIOSaveArrayDescriptorsRecursive (MCStackdirIORef op, MCArrayRef p_array, MCStringRef p_storage_spec, MCStringRef p_path, MCListRef r_descriptor_list);

/* Format an immediate property descriptor.
 *
 * The p_type_spec is permitted to be nil or empty.
 */
static bool MCStackdirIOSaveImmediateDescriptor (MCStackdirIORef op, MCStringRef p_storage_spec, MCStringRef p_type_spec, MCStringRef p_literal, MCStringRef & r_descriptor);

/* Format an external property descriptor.
 *
 * The p_type_spec and the p_file_name are permitted to be nil or empty.
 */
static bool MCStackdirIOSaveExternalDescriptor (MCStackdirIORef op, MCStringRef p_storage_spec, MCStringRef p_type_spec, MCStringRef p_file_type, MCDataRef p_file_contents, MCStringRef p_file_name, MCStringRef & r_descriptor);

/* Save a property set.
 *
 * Generates "_contents" and/or "_overflow" files in p_propset_path,
 * along with any necessary external storage files. */
static bool MCStackdirIOSavePropSet (MCStackdirIORef op, MCArrayRef p_propset, MCStringRef p_propset_path);

/* ================================================================
 * Utility functions
 * ================================================================ */

/* Helper function for writing a UTF-8 text file */
static bool
MCStackdirIOSaveUTF8 (MCStackdirIORef op, MCStringRef p_path,
					  MCStringRef p_contents)
{
	MCAutoDataRef t_data;
	/* UNCHECKED */ MCStringEncode (p_contents, kMCStringEncodingUTF8,
									false, &t_data);
	if (!MCS_savebinaryfile (p_path, *t_data))
		return MCStackdirIOErrorIO (op, p_path,
									MCSTR ("Failed to set contents of UTF-8 text file"));

	return true;
}

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

static bool
MCStackdirIODataSha1 (MCDataRef p_data, MCStringRef & p_hash)
{
	sha1_state_t t_sha1;
	sha1_init (&t_sha1);

	/* Add the data to the sha1 stream */
	sha1_append (&t_sha1, MCDataGetBytePtr (p_data), MCDataGetLength (p_data));

	/* Extract the resulting digest from the stream */
	uint8_t t_digest[20];
	sha1_finish (&t_sha1, t_digest);

	/* Format the digest as a string */
	char t_digest_hex[41]; /* N.b. includes trailing nul */
	for (int i = 0; i < 20; ++i)
		sprintf(t_digest_hex + i*2, "%02x", t_digest[i]);

	return MCStringCreateWithCString (t_digest_hex, p_hash);
}

static bool
MCStackdirIOArrayGetPropertyInfo (MCValueRef p_info,
								  MCNameRef & r_type,
								  MCValueRef & r_literal)
{
	/*
	 * If the property information (p_info) is an array, it must have
	 * two sections:
	 *
	 * - p_value["_type"] should be an MCNameRef with the type of the
	 *   property's value.  This might be kMCEmptyString if the
	 *   property is of a basic type.
	 *
	 * - p_value["_literal"] could be any basic type, and has to be
	 *   formatted correctly.
	 *
	 * All other types have the type inferred to be an empty string.
	 */

	if (!MCValueIsArray (p_info))
	{
		r_literal = MCValueRetain (p_info);
		r_type = MCValueRetain (kMCEmptyName);
		return true;
	}

	MCArrayRef t_array = (MCArrayRef) p_info;
	MCValueRef t_type, t_literal;
	if (!(MCArrayFetchValue (t_array, false, kMCStackdirTypeKey, t_type) &&
		  MCArrayFetchValue (t_array, false, kMCStackdirLiteralKey, t_literal) &&
		  MCValueGetTypeCode (t_type) == kMCValueTypeCodeName))
		return false;

	r_type = (MCNameRef) MCValueRetain (t_type);
	r_literal = MCValueRetain (t_literal);
	return true;
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
		uint32_t t_suffix_numeric = 0;
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

	MCStringRef t_invalid_property_message =
		MCSTR ("Invalid property information array");

	/* Extract property type and literal from the p_value */
	MCNameRef t_prop_name = p_key;
	MCNewAutoNameRef t_prop_type;
	MCAutoValueRef t_prop_literal;

	if (!MCStackdirIOArrayGetPropertyInfo (p_value, &t_prop_type,
										   &t_prop_literal))
		return MCStackdirIOErrorBadState (info->m_op,
										  info->m_path,
										  t_invalid_property_message);

	/* Format parts of property descriptor */
	MCAutoStringRef t_name, t_type_spec, t_literal;

	if (!(MCStackdirFormatLiteral (t_prop_name, &t_name) &&
		  MCStackdirFormatLiteral (*t_prop_literal, &t_literal)))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  t_invalid_property_message);

	/* Need to cope with the fact that the type spec is permitted to
	 * be omitted if the value is a basic type */
	if (MCNameIsEmpty (*t_prop_type))
		/* UNCHECKED */ MCStringCopy (kMCEmptyString, &t_type_spec);
	else if (!MCStackdirFormatLiteral (*t_prop_type, &t_type_spec))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  t_invalid_property_message);

	/* Create the storage spec */
	MCAutoStringRef t_storage_spec;
	/* UNCHECKED */ MCStringMutableCopy (info->m_storage_spec, &t_storage_spec);
	/* UNCHECKED */ MCStringAppend (*t_storage_spec, *t_name);

	/* Format the property descriptor */
	MCAutoStringRef t_property_descriptor;
	/* UNCHECKED */ MCStackdirIOSaveImmediateDescriptor (info->m_op,
														 *t_storage_spec,
														 *t_type_spec,
														 *t_literal,
														 &t_property_descriptor);

	/* Add the property descriptor to the target list */
	/* UNCHECKED */ MCListAppend (info->m_list, *t_property_descriptor);

	/* If the literal was an array, recurse */
	if (MCValueGetTypeCode (*t_prop_literal) == kMCValueTypeCodeArray)
		return MCStackdirIOSaveArrayDescriptorsRecursive (info->m_op,
														  (MCArrayRef) *t_prop_literal,
														  *t_storage_spec,
														  info->m_path,
														  info->m_list);

	return true;
}

static bool
MCStackdirIOSaveArrayDescriptorsRecursive (MCStackdirIORef op,
										   MCArrayRef p_array,
										   MCStringRef p_storage_spec,
										   MCStringRef p_path,
										   MCListRef r_descriptor_list)
{
	MCAssert (op != nil);
	MCAssert (p_array != nil);
	MCAssert (r_descriptor_list != nil);

	if (p_path == nil) p_path = kMCEmptyString;
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
	info.m_path = p_path;
	info.m_list = r_descriptor_list;

	return MCStackdirArrayApplySorted (p_array,
									   MCStackdirIOSaveArrayDescriptorsRecursive_Callback,
									   &info);
}


static bool
MCStackdirIOSaveImmediateDescriptor (MCStackdirIORef op,
									 MCStringRef p_storage_spec,
									 MCStringRef p_type_spec,
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

	if (t_success && t_has_type)
		t_success = MCListAppend (*t_list, p_type_spec);
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
									MCStringRef p_file_type,
									MCDataRef p_file_contents,
									MCStringRef p_file_name,
									MCStringRef & r_descriptor)
{
	MCAutoListRef t_list;
	MCAutoStringRef t_hash;
	bool t_success;
	t_success = (MCListCreateMutable (' ', &t_list) &&
				 MCListAppend (*t_list, p_storage_spec) &&
				 MCStackdirIODataSha1 (p_file_contents, &t_hash));

	/* Lexical analysis:
	 *
	 *    STORAGE_SPEC WS [TYPE_SPEC WS] "&" WS FILE_TYPE WS FILE_HASH [WS FILE_NAME]
	 */
	bool t_has_type = (p_type_spec != nil &&
					   !MCStringIsEmpty (p_type_spec));
	bool t_has_file_name = (p_file_name != nil &&
							!MCStringIsEmpty (p_file_name));

	if (t_success && t_has_type)
		t_success = MCListAppend (*t_list, p_type_spec);
	if (t_success)
		t_success = (MCListAppendCString (*t_list, "&") &&
					 MCListAppend (*t_list, p_file_type) &&
					 MCListAppend (*t_list, *t_hash));
	if (t_success && t_has_file_name)
		t_success = MCListAppend (*t_list, p_file_name);

	if (t_success)
		t_success = MCListCopyAsString (*t_list, r_descriptor);

	if (!t_success)
		MCStackdirIOErrorOutOfMemory (op);
	return t_success;
}

/* ================================================================
 * Property sets
 * ================================================================ */

/* Choose how to store the property descriptor. There are several
 * rules.  Properties that cannot be stored in the _contents file
 * are evaluated for suitability for external storage.  Properties
 * that cannot be stored externally end up in the _overflow
 * file. */
enum MCStackdirIOPropSetTarget
{
	kMCStackdirIOPropSetTargetContents,
	kMCStackdirIOPropSetTargetExternal,
	kMCStackdirIOPropSetTargetOverflow,

	kMCStackdirIOPropSetTargetMaxDescriptorLen = 78,
	kMCStackdirIOPropSetTargetMaxValueLen = 40,
};

static bool
MCStackdirIOSavePropSet_PropertyCallback (void *context,
										  MCArrayRef p_propset,
										  MCNameRef p_key,
										  MCValueRef p_value)
{
	MCStackdirIOPropsetSaveRef info = (MCStackdirIOPropsetSaveRef) context;

	/* Check where the property descriptor should end up and stick it
	 * into the correct place.
	 */

	MCNameRef  t_prop_name = p_key;
	MCNewAutoNameRef  t_prop_type;
	MCAutoValueRef t_prop_literal;

	MCStringRef t_invalid_property_message =
		MCSTR ("Invalid property information array");

	/* First, extract the type and literal information from the
	 * p_value. */
	if (!MCStackdirIOArrayGetPropertyInfo (p_value, &t_prop_type,
										   &t_prop_literal))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  t_invalid_property_message);

	/* Next format each of the parts of the property descriptor */
	MCValueTypeCode t_prop_literal_typecode = MCValueGetTypeCode (*t_prop_literal);
	MCAutoStringRef t_storage_spec, t_type_spec, t_literal;

	if (!(MCStackdirFormatLiteral (t_prop_name, &t_storage_spec) &&
		  MCStackdirFormatLiteral (*t_prop_literal, &t_literal)))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  t_invalid_property_message);

	/* Need to cope with the fact that the type spec is permitted to
	 * be omitted if the value is a basic type */
	if (MCNameIsEmpty (*t_prop_type))
		/* UNCHECKED */ MCStringCopy (kMCEmptyString, &t_type_spec);
	else if (!MCStackdirFormatLiteral (*t_prop_type, &t_type_spec))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  t_invalid_property_message);

	/* Decide where the property descriptor (and contents!) should end
	 * up */
	int t_target = kMCStackdirIOPropSetTargetContents;
	MCStringRef t_external_file_type;
	MCAutoDataRef t_external_file_contents;
	MCAutoStringRef t_external_filename;

	if (t_target == kMCStackdirIOPropSetTargetContents)
	{
		/* We can store in the _contents file if the encoded value
		 * isn't too big. */
		switch (t_prop_literal_typecode)
		{
		case kMCValueTypeCodeArray:
			/* Always attempt to save non-empty arrays externally */
			if (MCArrayGetCount ((MCArrayRef) *t_prop_literal) > 0)
				t_target = kMCStackdirIOPropSetTargetExternal;
			break;

		case kMCValueTypeCodeString:
			/* Special case for "script" properties */
			if (MCNameIsEqualTo (t_prop_name, MCNAME("script")))
			{
				t_target = kMCStackdirIOPropSetTargetExternal;
				break;
			}
			/* FALL THROUGH TO NEXT CASE */

		case kMCValueTypeCodeData:
			{
				/* Strings and data must have a sensible encoded length */
				uindex_t t_literal_len = MCStringGetLength (*t_literal);
				uindex_t t_descriptor_len = (t_literal_len +
											 MCStringGetLength (*t_storage_spec) +
											 MCStringGetLength (*t_type_spec));

				if (t_literal_len > kMCStackdirIOPropSetTargetMaxValueLen &&
					t_descriptor_len > kMCStackdirIOPropSetTargetMaxDescriptorLen)
					t_target = kMCStackdirIOPropSetTargetExternal;
			}
			break;

		case kMCValueTypeCodeBoolean:
		case kMCValueTypeCodeNumber:
			/* Can always be stored in _contents */
			break;

		default:
			return MCStackdirIOErrorBadState (info->m_op, info->m_path,
											  t_invalid_property_message);
		}
	}

	if (t_target == kMCStackdirIOPropSetTargetExternal)
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
			t_target = kMCStackdirIOPropSetTargetOverflow;
	}

	if (t_target == kMCStackdirIOPropSetTargetExternal)
	{
		/* Now that we're certain that external storage is possible,
		 * do some more expensive preparations and write the external
		 * file. */

		MCAutoStringRef t_external_path;
		if (!MCStringFormat (&t_external_path, "%@/%@", info->m_path,
							 *t_external_filename))
			return MCStackdirIOErrorOutOfMemory (info->m_op);

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
				if (!(MCStackdirIOSaveArrayDescriptorsRecursive (info->m_op,
											(MCArrayRef) *t_prop_literal,
											kMCEmptyString,
											*t_external_path,
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

		if (!MCS_savebinaryfile (*t_external_path, *t_external_file_contents))
			return MCStackdirIOErrorIO (info->m_op, *t_external_path,
										MCSTR ("Failed to set contents of external file"));
	}

	/* In theory, we've now decided how the property should be stored.
	 * Format a property descriptor of the correct type (immediate or
	 * external). */
	MCAutoStringRef t_property_descriptor;
	switch (t_target)
	{
	case kMCStackdirIOPropSetTargetExternal:
		/* UNCHECKED */ MCStackdirIOSaveExternalDescriptor (info->m_op,
							*t_storage_spec,
							*t_type_spec,
							t_external_file_type,
							*t_external_file_contents,
							kMCEmptyString, /* Always use generated filename */
							&t_property_descriptor);
		break;
	case kMCStackdirIOPropSetTargetOverflow:
	case kMCStackdirIOPropSetTargetContents:
		/* UNCHECKED */ MCStackdirIOSaveImmediateDescriptor (info->m_op,
							*t_storage_spec,
							*t_type_spec,
							*t_literal,
							&t_property_descriptor);
		break;
	default:
		MCUnreachable ();
	}

	/* Add the property descriptor to the correct target file */
	if (t_target == kMCStackdirIOPropSetTargetOverflow)
		/* UNCHECKED */ MCListAppend (info->m_overflow, *t_property_descriptor);
	else
		/* UNCHECKED */ MCListAppend (info->m_contents, *t_property_descriptor);

	/* If the value is an array and has internal storage, then add the
	 * property descriptors for its contents to the appropriate file
	 * too. */
	if (t_prop_literal_typecode == kMCValueTypeCodeArray &&
		t_target != kMCStackdirIOPropSetTargetExternal)
	{
		MCStringRef t_path;
		MCListRef t_list;
		switch (t_target)
		{
		case kMCStackdirIOPropSetTargetOverflow:
			t_path = info->m_overflow_path;
			t_list = info->m_overflow;
			break;
		case kMCStackdirIOPropSetTargetContents:
			t_path = info->m_contents_path;
			t_list = info->m_contents;
			break;
		default:
			MCUnreachable ();
		}

		if (!MCStackdirIOSaveArrayDescriptorsRecursive (info->m_op,
														(MCArrayRef) *t_prop_literal,
														*t_storage_spec,
														t_path,
														t_list))
			return false;
	}

	return true;
}

static bool
MCStackdirIOSavePropSet (MCStackdirIORef op, MCArrayRef p_propset,
						 MCStringRef p_propset_path)
{
	/* Fast path */
	if (MCArrayIsEmpty (p_propset)) return true;

	bool t_success = true;

	MCAutoStringRef t_contents_path, t_overflow_path;
	if (t_success)
		t_success = (MCStringFormat (&t_contents_path, "%@/%@",
									 p_propset_path, kMCStackdirContentsFile) &&
					 MCStringFormat (&t_overflow_path, "%@/%@",
									 p_propset_path, kMCStackdirOverflowFile));

	/* Create and initialise propset information structure */
	MCStackdirIOPropsetSave info;
	MCAutoListRef t_contents_list, t_overflow_list;
	if (t_success)
		t_success = (MCListCreateMutable (kMCLineEndString, &t_contents_list) &&
					 MCListCreateMutable (kMCLineEndString, &t_overflow_list));

	if (t_success)
	{
		info.m_op = op;
		info.m_path = p_propset_path;
		info.m_contents_path = *t_contents_path;
		info.m_overflow_path = *t_overflow_path;
		info.m_contents = *t_contents_list;
		info.m_overflow = *t_overflow_list;
	}

	/* Generate property descriptors */
	if (t_success)
		t_success = MCStackdirArrayApplySorted (p_propset,
						MCStackdirIOSavePropSet_PropertyCallback,
						&info);

	/* Write propset files. We append the empty string to each list in
	 * order to ensure that they contain a trailing newline
	 * character. */
	if (t_success && !MCListIsEmpty (*t_contents_list))
	{
		MCAutoStringRef t_contents;
		t_success = (MCListAppend (*t_contents_list, kMCEmptyString) &&
					 MCListCopyAsString (*t_contents_list, &t_contents) &&
					 MCStackdirIOSaveUTF8 (op, *t_contents_path,
										   *t_contents));
	}

	if (t_success && !MCListIsEmpty (*t_overflow_list))
	{
		MCAutoStringRef t_overflow;
		t_success = (MCListAppend (*t_overflow_list, kMCEmptyString) &&
					 MCListCopyAsString (*t_overflow_list, &t_overflow) &&
					 MCStackdirIOSaveUTF8 (op, *t_overflow_path,
										   *t_overflow));
	}

	return t_success;
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
MCStackdirIOSaveObjectKeyDirect (MCStackdirIOObjectSaveRef info, MCStringRef p_key)
{
	MCNewAutoNameRef t_key;
	MCValueRef t_value;
	/* UNCHECKED */ MCNameCreate (p_key, &t_key);

	/* Each object's state array has to have the specified key */
	if (!MCArrayFetchValue (info->m_state, false, *t_key, t_value))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  MCSTR ("Missing required key in object."));

	MCAutoStringRef t_content_literal, t_content;
	/* UNCHECKED */ MCStackdirFormatLiteral (t_value, &t_content_literal);
	/* UNCHECKED */ MCStringFormat (&t_content, "%@\n", *t_content_literal);

	/* Construct path for file */
	MCAutoStringRef t_path;
	/* UNCHECKED */ MCStringFormat (&t_path, "%@/%@", info->m_path,
									p_key);

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
	return MCStackdirIOSaveObjectKeyDirect (info, kMCStackdirParentFile);
}

static bool
MCStackdirIOSaveObjectInternal (MCStackdirIOObjectSaveRef info)
{
	MCValueRef t_internal_props;

	/* The state array has to have an _internal array */
	if (!MCArrayFetchValue (info->m_state, false, kMCStackdirInternalKey,
							t_internal_props))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  MCSTR ("No internal properties in object."));

	if (!MCValueIsArray (t_internal_props))
		return MCStackdirIOErrorBadState (info->m_op, info->m_path,
										  MCSTR ("Internal properties not an array"));

	/* Reuse the propset saving logic, saving into the object
	 * directory. */
	return MCStackdirIOSavePropSet (info->m_op, (MCArrayRef) t_internal_props,
									info->m_path);
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
	if (t_success)
		t_success = MCStackdirIOSaveObjectDirectory (info);

	/* Generate object contents */
	if (t_success)
		t_success = (MCStackdirIOSaveObjectKind (info) &&
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
