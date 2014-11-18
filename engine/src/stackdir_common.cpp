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
#include "uuid.h"

#include "stackdir.h"

#include "stackdir_private.h"

static bool MCStackdirIOErrorReport (MCStackdirIORef op, MCStackdirStatus p_status, MCStringRef p_filename, MCValueRef p_file_line, MCValueRef p_file_col, MCStringRef p_message, MCArrayRef & r_report);
static bool MCStackdirIOErrorLocation (MCStackdirIORef op, MCArrayRef & r_location);

/* ================================================================
 * Errors
 * ================================================================ */

MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOCommonErrorWriteUTF8File,
							kMCStackdirStatusIOError,
							"Failed to write UTF-8 text file")
MC_STACKDIR_ERROR_FUNC_FULL(MCStackdirIOCommonErrorReadUTF8File,
							kMCStackdirStatusIOError,
							"Failed to read UTF-8 text file")

/* ----------------------------------------------------------------
 * Miscellaneous utility functions
 * ---------------------------------------------------------------- */

static int
MCStackdirArrayApplySorted_DefaultCompare (const MCNameRef *a, const MCNameRef *b)
{
	return (int) MCStringCompareTo (MCNameGetString (*a), MCNameGetString (*b),
									kMCStringOptionCompareCaseless);
}

bool
MCStackdirArrayApplySorted (MCArrayRef array,
							MCArrayApplyCallback p_callback,
							void *p_callback_context,
							MCStackdirArrayApplySortedCompareFunc p_compare)
{
	/* Trivial case */
	if (MCArrayIsEmpty (array)) return true;

	MCAutoArray<MCNameRef> t_keys;
	/* UNCHECKED */ t_keys.New(0);

	/* Construct list of all keys */
	uintptr_t t_iterator = 0;
	MCNameRef t_key;
	MCValueRef t_value;
	while (MCArrayIterate (array, t_iterator, t_key, t_value))
	{
		t_keys.Push (t_key);
	}

	/* Sort the keys using the comparison function. */
	if (p_compare == NULL)
		p_compare = MCStackdirArrayApplySorted_DefaultCompare;
	qsort (t_keys.Ptr (), t_keys.Size(), sizeof (MCNameRef),
		   (int (*)(const void *, const void *)) p_compare);

	/* Call the callback for each key/value */
	bool t_result = true;
	for (uindex_t i = 0; i < t_keys.Size(); ++i)
	{
		t_key = t_keys[i];

		/* We use case-sensitive fetch here because the keys should
		 * *exactly* match (we got them from the array!) */
		/* UNCHECKED */ MCArrayFetchValue (array, true, t_key, t_value);

		t_result = p_callback (p_callback_context, array, t_key, t_value);
		if (!t_result) break;
	}

	return t_result;
}

/* Helper function for writing a UTF-8 text file */
bool
MCStackdirIOSaveUTF8 (MCStackdirIORef op,
					  MCStringRef p_path,
					  MCStringRef p_contents)
{
	MCAutoDataRef t_data;

	if (!MCStringEncode (p_contents, kMCStringEncodingUTF8,
						 false, &t_data))
		return MCStackdirIOErrorOutOfMemory (op);
	if (!MCS_savebinaryfile (p_path, *t_data))
		return MCStackdirIOCommonErrorWriteUTF8File (op, p_path);
	return true;
}

/* Helper function for loading a UTF-8 text file */
bool
MCStackdirIOLoadUTF8 (MCStackdirIORef op,
					  MCStringRef p_path,
					  MCStringRef & p_contents,
					  bool p_allow_missing)
{
	/* Read file */
	MCAutoDataRef t_data;
	if (!MCS_loadbinaryfile (p_path, &t_data))
	{
		if (p_allow_missing
			&& !MCS_exists (p_path, true)
			&& !MCS_exists (p_path, false))
		{
			p_contents = MCValueRetain (kMCEmptyString);
			return false;
		}
		else
		{
			return MCStackdirIOCommonErrorReadUTF8File (op, p_path);
		}
	}

	/* Decode data */
	MCAutoStringRef t_string;
	if (!MCStringDecode (*t_data, kMCStringEncodingUTF8, false, &t_string))
		return MCStackdirIOErrorOutOfMemory (op);

	p_contents = MCValueRetain (*t_string);
	return true;
}

/* ----------------------------------------------------------------
 * [Private] Adding error reports
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOErrorReport (MCStackdirIORef op,
						 MCStackdirStatus p_status,
						 MCStringRef p_filename,
						 MCValueRef p_file_line,
						 MCValueRef p_file_col,
						 MCStringRef p_message,
						 MCArrayRef & r_report)
{
	MCAutoNumberRef t_status;
	if (!MCNumberCreateWithInteger (p_status, &t_status))
	{
		return MCStackdirIOErrorOutOfMemory (op);
	}

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

	if (!MCArrayCreate (false, t_keys, t_values, 5, r_report))
		return MCStackdirIOErrorOutOfMemory (op);

	return true;
}

void
MCStackdirIOErrorLocationPush (MCStackdirIORef op,
							   MCStringRef p_path,
							   MCValueRef p_line,
							   MCValueRef p_column)
{
	MCAutoStringRef t_filename, t_resolved;
	if (!MCStringIsEmpty (p_path))
	{
		/* Attempt to resolve the filename.  If we can't, it's not a
		 * problem; just use the filename how it is. */
		if (MCS_resolvepath (p_path, &t_resolved))
			&t_filename = MCValueRetain (*t_resolved);
		else
			&t_filename = MCValueRetain (p_path);
	}
	else if (0 < MCArrayGetCount (op->m_error_location_stack))
	{
		/* Pull the filename from the current top of the error
		 * location stack. */
		MCAutoArrayRef t_prev_report;
		if (!MCStackdirIOErrorLocation (op, &t_prev_report))
			return;

		MCValueRef t_prev_path;
		if (!MCArrayFetchValue (*t_prev_report,
								true,
								MCNAME ("filename"),
								t_prev_path))
		{
			MCStackdirIOErrorOutOfMemory (op);
			return;
		}
		MCAssert (MCValueGetTypeCode (t_prev_path) == kMCValueTypeCodeString);

		&t_filename = MCValueRetain ((MCStringRef) t_prev_path);
	}

	/* The error location is stored in the same way as an error or
	 * warning report.  This is to simplify the process of adding
	 * reports in MCStackdirIOError(). */

	/* N.b. dense arrays are 1-indexed */
	uindex_t t_idx = MCArrayGetCount (op->m_error_location_stack) + 1;

	MCAutoArrayRef t_report;
	if (!MCStackdirIOErrorReport (op,
								  kMCStackdirStatusSuccess,
								  *t_filename,
								  p_line,
								  p_column,
								  kMCEmptyString,
								  &t_report))
		return;
	if (!MCArrayStoreValueAtIndex (op->m_error_location_stack, t_idx,
								   *t_report))
		MCStackdirIOErrorOutOfMemory (op);
}

void
MCStackdirIOErrorLocationPush (MCStackdirIORef op,
							   MCStringRef p_path,
							   index_t p_line,
							   index_t p_column)
{
	MCAutoNumberRef t_line, t_column;
	MCValueRef t_line_value, t_column_value;
	if (p_line != -1)
	{
		if (!MCNumberCreateWithInteger (p_line, &t_line))
		{
			MCStackdirIOErrorOutOfMemory (op);
			return;
		}
		t_line_value = (MCValueRef) *t_line;
	}
	else
		t_line_value = (MCValueRef) kMCEmptyString;

	if (p_column != -1)
	{
		if (!MCNumberCreateWithInteger (p_column, &t_column))
		{
			MCStackdirIOErrorOutOfMemory (op);
			return;
		}
		t_column_value = (MCValueRef) *t_column;
	}
	else
		t_column_value = (MCValueRef) kMCEmptyString;

	MCStackdirIOErrorLocationPush (op, p_path,
								   t_line_value,
								   t_column_value);
}

void
MCStackdirIOErrorLocationPop (MCStackdirIORef op)
{
	MCAssert (op != nil);
	MCAssert (op->m_error_location_stack != nil);

	if (MCArrayIsEmpty (op->m_error_location_stack))
		return;

	uindex_t t_idx = MCArrayGetCount (op->m_error_location_stack);
	if (!MCArrayRemoveValueAtIndex (op->m_error_location_stack, t_idx))
		MCStackdirIOErrorOutOfMemory (op);
}


static bool
MCStackdirIOErrorLocation (MCStackdirIORef op,
						   MCArrayRef & r_location)
{

	/* N.b. dense arrays are 1-indexed */
	MCValueRef t_location_info;
	uindex_t t_location_idx = MCArrayGetCount (op->m_error_location_stack);

	if (MCArrayFetchValueAtIndex (op->m_error_location_stack,
								  t_location_idx, t_location_info))
	{
		if (!MCArrayMutableCopy ((MCArrayRef) t_location_info, r_location))
			return MCStackdirIOErrorOutOfMemory (op);
		return true;
	}

	return MCStackdirIOErrorReport (op,
									kMCStackdirStatusSuccess,
									kMCEmptyString,
									kMCEmptyString,
									kMCEmptyString,
									kMCEmptyString,
									r_location);
}

/* Format an error into an immutable array, then append it to the
 * operation's current list of error and warning reports, setting
 * the operation's status. */
bool
MCStackdirIOError (MCStackdirIORef op, MCStackdirStatus p_status,
				   MCStringRef p_message)
{
	MCAutoNumberRef t_status;
	if (!MCNumberCreateWithInteger (p_status, &t_status))
		return MCStackdirIOErrorOutOfMemory (op);

	bool t_success = true;

	/* N.b. dense arrays are 1-indexed */
	MCAutoArrayRef t_report;
	uindex_t t_idx = MCArrayGetCount (op->m_error_info) + 1;

	if (t_success)
		t_success = MCStackdirIOErrorLocation (op, &t_report);

	if (t_success)
	{
		/* Update and store the report */
		if (!(MCArrayStoreValue (*t_report, false,
								 MCNAME("status"), *t_status) &&
			  MCArrayStoreValue (*t_report, false,
								 MCNAME("message"), p_message) &&
			  MCArrayStoreValueAtIndex (op->m_error_info, t_idx,
										*t_report)))
			return MCStackdirIOErrorOutOfMemory (op);
	}

	/* Set the status of the operation, if it doesn't already have an
	 * error status */
	if (!MCStackdirIOHasError (op))
		op->m_status = p_status;
	return false;
}

bool
MCStackdirIOErrorFull (MCStackdirIORef op,
					   MCStackdirStatus p_status,
					   MCStringRef p_path,
					   index_t p_line,
					   index_t p_column,
					   MCStringRef p_message)
{
	MCStackdirIOErrorLocationPush (op, p_path, p_line, p_column);
	MCStackdirIOError (op, p_status, p_message);
	MCStackdirIOErrorLocationPop (op);
	return false;
}

bool
MCStackdirIOErrorOutOfMemory (MCStackdirIORef op)
{
	/* Can't do anything! */
	op->m_status = kMCStackdirStatusOutOfMemory;
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
	if (!MCArrayCreateMutable (op->m_error_location_stack)) return false;

	op->m_status = kMCStackdirStatusSuccess;

	return true;
}

/* ----------------------------------------------------------------
 * [Private] Tests on expanded stackfile bundles
 * ---------------------------------------------------------------- */

bool
MCStackdirPathIsStackdir (MCStringRef p_path)
{
	/* Stackdir must be a folder */
	if (!MCS_exists (p_path, false))
		return false;

	/* Stackdir must contain a version file. */
	MCAutoStringRef t_version_path;
	if (!MCStringFormat (&t_version_path, "%@/%@", p_path,
						 kMCStackdirVersionFile))
		return false;

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
 * [Private] UUID handling
 * ---------------------------------------------------------------- */

bool
MCStackdirStringIsUUID (MCStringRef p_uuid)
{
	MCUuid t_uuid;
	return MCUuidFromCString (MCStringGetCString (p_uuid), t_uuid);
}

/* ----------------------------------------------------------------
 * [Public] IO operation destruction
 * ---------------------------------------------------------------- */

void
MCStackdirIODestroy (MCStackdirIORef & op)
{
	if (op == nil) return;

	MCValueRelease (op->m_error_info);
	MCValueRelease (op->m_error_location_stack);

	MCValueRelease (op->m_path);
	MCValueRelease (op->m_save_build_dir);
	MCValueRelease (op->m_save_backup_dir);
	MCValueRelease (op->m_save_backup_path);
	MCValueRelease (op->m_save_state);

	MCValueRelease (op->m_load_state);
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
