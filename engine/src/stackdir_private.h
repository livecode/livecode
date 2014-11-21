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
	/* Error location stack */
	/* FIXME replace with proper list */
	MCArrayRef m_error_location_stack;

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
	/* Source location information from load operation */
	MCArrayRef m_source_info;
};

/* Create a new IO operation state structure.
 *
 * Allocates the structure and initialises the common members.
 */
bool MCStackdirIONew (MCStackdirIORef &op);

/* Filenames */
#define kMCStackdirVersionFile  MCSTR("_version")
#define kMCStackdirKindFile     MCSTR("_kind")
#define kMCStackdirParentFile   MCSTR("_parent")
#define kMCStackdirContentsFile MCSTR("_contents")
#define kMCStackdirOverflowFile MCSTR("_overflow")
#define kMCStackdirEmptyFile    MCSTR("_empty")
#define kMCStackdirSharedFile    MCSTR("_shared")

#define kMCStackdirInternalKey  MCNAME("_internal")
#define kMCStackdirTypeKey      MCNAME("_type")
#define kMCStackdirLiteralKey   MCNAME("_literal")
#define kMCStackdirCustomKey    MCNAME("_custom")
#define kMCStackdirSharedKey    MCNAME("_shared")

#define kMCStackdirStringSuffix  MCSTR(".txt")
#define kMCStackdirDataSuffix    MCSTR(".bin")
#define kMCStackdirArraySuffix   MCSTR(".map")
#define kMCStackdirPropsetSuffix MCSTR(".propset")
#define kMCStackdirSharedSuffix  MCSTR(".shared")

#define kMCStackdirStringType   MCSTR("string")
#define kMCStackdirDataType     MCSTR("data")
#define kMCStackdirArrayType    MCSTR("array")

#define kMCStackdirTrueLiteral  MCSTR("true")
#define kMCStackdirFalseLiteral MCSTR("false")
#define kMCStackdirArrayLiteral MCSTR("array")

#define kMCStackdirExternalFlag MCNAME("external")

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
 * 1. p_path arguments must be standard paths.
 *
 * 2. p_message arguments should begin with a capital letter and
 *    should not contain a trailing full stop or newline.
 *
 * 3. These functions always return false.
 */

bool MCStackdirIOError (MCStackdirIORef op, MCStackdirStatus p_status, MCStringRef p_message);
bool MCStackdirIOErrorFull (MCStackdirIORef op, MCStackdirStatus p_status, MCStringRef p_path, index_t p_line, index_t p_column, MCStringRef p_message);

bool MCStackdirIOErrorOutOfMemory (MCStackdirIORef op);

void MCStackdirIOErrorLocationPop (MCStackdirIORef op);
void MCStackdirIOErrorLocationPush (MCStackdirIORef op, MCStringRef p_path, MCValueRef p_line = kMCEmptyString, MCValueRef p_column = kMCEmptyString);
void MCStackdirIOErrorLocationPush (MCStackdirIORef op, MCStringRef p_path, index_t p_line, index_t p_column = -1);

#define MC_STACKDIR_ERROR_FUNC(f,s,m)			\
	static bool f(MCStackdirIORef op) { return MCStackdirIOError (op, (s), MCSTR((m))); }

#define MC_STACKDIR_ERROR_FUNC_FULL(f,s,m)						\
	static bool f(MCStackdirIORef op, MCStringRef path,					\
				  index_t line = -1, index_t col = -1) {				\
	return MCStackdirIOErrorFull (op, s, path, line, col, MCSTR(m)); }

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

/* Encode and write a UTF-8 text file */
bool MCStackdirIOSaveUTF8 (MCStackdirIORef op, MCStringRef p_path, MCStringRef p_contents);

/* Read and decode a UTF-8 text file */
bool MCStackdirIOLoadUTF8 (MCStackdirIORef op, MCStringRef p_path, MCStringRef & p_contents, bool p_allow_missing);

/* ----------------------------------------------------------------
 * [Private] Tests on expanded stackfile bundles
 * ---------------------------------------------------------------- */

/* Test whether p_path is an expanded stackfile directory tree.
 *
 * p_path must be a standard (i.e. not native) path. */
bool MCStackdirPathIsStackdir (MCStringRef p_path);

/* ----------------------------------------------------------------
 * [Private] UUID handling
 * ---------------------------------------------------------------- */

/* Test whether p_uuid is a valid UUID */
bool MCStackdirStringIsUUID (MCStringRef p_uuid);

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

/* ----------------------------------------------------------------
 * [Private] Data scanning and parsing
 * ---------------------------------------------------------------- */

/* Parse a stackdir filename to a string with an optional suffix.
 *
 * If the p_suffix is non-empty, it will be required to be present and
 * removed before parsing.
 *
 * Returns false if parsing failed (e.g. because the suffix was not
 * present, the filname was syntactically invalid, or because of
 * memory exhaustion).
 */
bool MCStackdirParseFilename (MCStringRef p_filename, MCStringRef p_suffix, MCStringRef & r_string);

enum MCStackdirIOTokenType
{
	kMCStackdirIOTokenTypeNone = 0,
	kMCStackdirIOTokenTypeNewline,
	kMCStackdirIOTokenTypeSpace,
	kMCStackdirIOTokenTypeNumber,
	kMCStackdirIOTokenTypeString,
	kMCStackdirIOTokenTypeUnquotedString,
	kMCStackdirIOTokenTypeData,
	kMCStackdirIOTokenTypeStorageSeparator,
	kMCStackdirIOTokenTypeExternalIndicator,
	kMCStackdirIOTokenTypeSharedHeader,
	kMCStackdirIOTokenTypeFlag,
	kMCStackdirIOTokenTypeEOF,
	kMCStackdirIOTokenTypeError,
};

typedef struct _MCStackdirIOToken MCStackdirIOToken;

struct _MCStackdirIOToken
{
	_MCStackdirIOToken()
		: m_line(0), m_column (0),
		  m_type (kMCStackdirIOTokenTypeNone),
		  m_value (nil)
	{}

	~_MCStackdirIOToken()
	{
		MCValueRelease (m_value);
	}

	uindex_t m_line;
	uindex_t m_column;
	MCStackdirIOTokenType m_type;
	MCValueRef m_value;
};

typedef struct _MCStackdirIOScanner MCStackdirIOScanner;
typedef MCStackdirIOScanner *MCStackdirIOScannerRef;

/* Copy a token */
void MCStackdirIOTokenCopy (MCStackdirIOToken & p_src, MCStackdirIOToken & r_dest);

/* Create a new scanner for p_string */
bool MCStackdirIOScannerNew (MCStringRef p_string, MCStackdirIOScannerRef & scanner);

/* Peek the next token from the scanner. The r_token will be valid
 * until the next call to MCStackdirIOScannerPeek() or
 * MCStackdirIOScannerConsume().  Returns true if a token was
 * found. */
bool MCStackdirIOScannerPeek (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token, MCStackdirIOTokenType p_accept_type=kMCStackdirIOTokenTypeNone);

/* Consume the next token from the scanner.  The r_token will be valid
 * until the next call to MCStackdirIOScannerPeek() or
 * MCStackdirIOScannerConsume().  Returns true if a token was
 * found. */
bool MCStackdirIOScannerConsume (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token, MCStackdirIOTokenType p_accept_type=kMCStackdirIOTokenTypeNone);

/* Clean up a scanner when no longer needed */
void MCStackdirIOScannerDestroy (MCStackdirIOScannerRef & scanner);

#  endif /* ! _MC_STACKDIR_PRIVATE_H_ */
