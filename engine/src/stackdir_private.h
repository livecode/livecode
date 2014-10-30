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

#ifndef _MC_STACKDIR_PRIVATE_H_
#define _MC_STACKDIR_PRIVATE_H_

/* Private shared API definitions for Expanded LiveCode Stackfile
 * support. */

#define MCStackdirIOAssertType(op, type) do {		  \
	MCAssert ((op) != nil);						  \
	MCAssert ((op)->m_type == (type));				  \
	} while (false)
#define MCStackdirIOAssertSave(op) MCStackdirIOAssertType(op, kMCStackdirIOTypeSave)
#define MCStackdirIOAssertLoad(op) MCStackdirIOAssertType(op, kMCStackdirIOTypeLoad)

/* ----------------------------------------------------------------
 * [Private] Data structures and constants
 * ---------------------------------------------------------------- */

enum MCStackdirIOType
{
	kMCStackdirIOTypeSave = 1,
	kMCStackdirIOTypeLoad = 2,
};

/* If you modify this structure, don't forget to update
 * MCStackdirIONew() and MCStackdirIODestroy(). */
struct _MCStackdirIO
{
	/* Which type of stackdir is this? */
	MCStackdirIOType m_type;

	/* Current status of the IO operation */
	MCStackdirStatus m_status;
	/* Array with all accumulated errors & warnings */
	MCArrayRef m_error_info;

	/* Path of the root of the expanded stack directory tree */
	MCStringRef m_path;

	/* ---- Save only members */
	/* Root of the root of the save transaction directory tree */
	MCStringRef m_save_build_dir;
	/* Temporary directory for backup during transaction end */
	MCStringRef m_save_backup_dir;
	/* Location of original data during transaction end */
	MCStringRef m_save_backup_path;
	/* Stack state array */
	MCArrayRef m_save_state;

	/* ---- Load only members */
	/* Stack state */
	MCArrayRef m_load_state;
	/* Alternative stack state in case of VCS conflict */
	MCArrayRef m_load_state_theirs;
	/* Source location information from load operation */
	MCArrayRef m_source_info;
};

/* Create a new IO operation state structure.
 *
 * Allocates the structure and initialises the common members.
 */
bool MCStackdirIONew (MCStackdirIORef &op);

/* Filenames */
/* FIXME these should probably be MCStringRefs */
#define kMCStackdirVersionFile  MCSTR("_version")
#define kMCStackdirKindFile     MCSTR("_kind")
#define kMCStackdirParentFile   MCSTR("_parent")
#define kMCStackdirContentsFile MCSTR("_contents")
#define kMCStackdirOverflowFile MCSTR("_overflow")
#define kMCStackdirSaveBackupDir MCSTR("_save_backup")

/* Contents of version file */
/* FIXME Version info should be separated out.  Also, this should
 * probably be an MCStringRef. */
#define kMCStackdirMagicString MCSTR("Expanded LiveCode Stackfile 1.0.0\n")

/* ----------------------------------------------------------------
 * [Private] Error recording
 * ---------------------------------------------------------------- */

inline bool MCStackdirIOHasError (MCStackdirIORef op)
{
	return (op->m_status != kMCStackdirStatusSuccess);
}

/* These functions are used to help populate
 * MCStackdirIORef::m_error_info.  Please note:
 *
 * 1. p_filename arguments must be standard paths.
 *
 * 2. p_message arguments should begin with a capital letter and
 *    should not contain a trailing full stop or newline.
 *
 * 3. These functions always return false.
 */

bool MCStackdirIOErrorIO (MCStackdirIORef op, MCStringRef p_filename, MCStringRef p_message);
bool MCStackdirIOErrorOutOfMemory (MCStackdirIORef op);
bool MCStackdirIOErrorBadPath (MCStackdirIORef op, MCStringRef p_message);
bool MCStackdirIOErrorBadState (MCStackdirIORef op, MCStringRef p_filename, MCStringRef p_message);

/* ----------------------------------------------------------------
 * [Private] Indirect implementations of public functions
 * ---------------------------------------------------------------- */

/* MCStackdirIOCommit() */
void MCStackdirIOCommitSave (MCStackdirIORef op);
void MCStackdirIOCommitLoad (MCStackdirIORef op);

/* ----------------------------------------------------------------
 * Utility functions
 * ---------------------------------------------------------------- */

/* Apply a callback function to each element of an array, in an order
 * defined by sorting the keys using p_func. */
typedef int (*MCStackdirArrayApplySortedCompareFunc)(const MCNameRef *, const MCNameRef *);
bool MCStackdirArrayApplySorted (MCArrayRef array, MCArrayApplyCallback p_callback, void *p_callback_context, MCStackdirArrayApplySortedCompareFunc p_compare = NULL);

/* ----------------------------------------------------------------
 * [Private] Filename generation
 * ---------------------------------------------------------------- */

/* Generate a valid stackdir filename from an arbitrary string or
 * name.
 *
 * If no valid stackdir filename could be generated for the string,
 * returns false.  Otherwise, returns true.
 */
bool MCStackdirCreateFilename (MCStringRef p_str, MCStringRef & r_filename);
bool MCStackdirCreateFilename (MCNameRef p_str, MCStringRef & r_filename);

/* ----------------------------------------------------------------
 * [Private] Tests on expanded stackfile bundles
 * ---------------------------------------------------------------- */

/* Test whether p_path is an expanded stackfile directory tree.
 *
 * p_path must be a standard (i.e. not native) path. */
bool MCStackdirPathIsStackdir (MCStringRef p_path);

/* ----------------------------------------------------------------
 * [Private] Data formatting
 * ---------------------------------------------------------------- */

/* Format a string as a stackdir filename with an optional suffix.
 *
 * The p_string will be normalised and case-folded before conversion.
 *
 * Returns false if formatting failed (e.g. because the resulting
 * filename was invalid or because of memory exhaustion).
 */
bool MCStackdirFormatFilename (MCStringRef p_string, MCStringRef p_suffix, MCStringRef & r_filename);

/* Format a base type value as a stackdir-encoded string.
 *
 * Returns false if formatting failed (e.g. due to memory exhaustion). */
bool MCStackdirFormatLiteral (MCValueRef p_value, MCStringRef & r_literal);

#  endif /* ! _MC_STACKDIR_PRIVATE_H_ */
