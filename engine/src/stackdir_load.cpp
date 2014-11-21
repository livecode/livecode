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

/* Load a single property by parsing using scanner, and store it in
 * x_propset. External files should be loaded relative to
 * p_external_dir.  */
static bool MCStackdirIOLoadProperty (MCStackdirIORef op, MCStackdirIOScannerRef scanner, MCStringRef p_external_dir, MCArrayRef x_propset);

/* Load a property set from p_propset_path and store the properties
 * found in x_propset.  If p_required is false, then IO errors while
 * opening the file will be ignored if the file doesn't exist.. */
static bool MCStackdirIOLoadPropFile (MCStackdirIORef op, MCStringRef p_propset_path, MCStringRef p_external_dir, MCArrayRef x_propset, bool p_required);

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

MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorArrayOrdering,
						kMCStackdirStatusSyntaxError,
						"Array entry descriptor before array's descriptor")
MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorArrayType,
						kMCStackdirStatusSyntaxError,
						"Array entry descriptor for non-array property")
MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorPropertyUnquotedStringLiteral,
						kMCStackdirStatusSyntaxError,
						"Invalid unquoted string literal (expected 'true', "
						"'false' or 'array')")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyExternalType,
							 kMCStackdirStatusSyntaxError,
							 "Invalid external storage type (expected unquoted "
							 "string)")
MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorPropertyExternalTypeCode,
						kMCStackdirStatusSyntaxError,
						"Invalid external storage type (expected 'array', "
						"'data' or 'string')")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyExternalRead,
							 kMCStackdirStatusIOError,
							 "Failed to read external file")
MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorPropertyExternalInvalid,
						kMCStackdirStatusSyntaxError,
						"External storage not permitted here.")
MC_STACKDIR_ERROR_FUNC (MCStackdirIOLoadErrorPropertyExternalPathRequired,
						kMCStackdirStatusSyntaxError,
						"Filename cannot be inferred for externally stored "
						"property")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyStorageSpec,
							 kMCStackdirStatusSyntaxError,
							 "Invalid storage specifier (expected string)")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyStorageSeparator,
							 kMCStackdirStatusSyntaxError,
							 "Invalid storage specifier terminator (expected "
							 "space or ':')")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyFlagMissing,
							 kMCStackdirStatusSyntaxError,
							 "Flag indicator without flag name")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyFlagSep,
							 kMCStackdirStatusSyntaxError,
							 "Invalid flag separator (expected space)")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyFlagType,
							 kMCStackdirStatusSyntaxError,
							 "Invalid flag name (expected string")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyLiteral,
							 kMCStackdirStatusSyntaxError,
							 "Invalid property value (expected literal or "
							 "external)")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorPropertyNewline,
							 kMCStackdirStatusSyntaxError,
							 "Property descriptor lacks terminating newline")

MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorCustomNotDir,
							 kMCStackdirStatusBadStructure,
							 "Non-directory custom property set")

MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorSharedHeader,
							 kMCStackdirStatusSyntaxError,
							 "Expected shared property header")
MC_STACKDIR_ERROR_FUNC_FULL (MCStackdirIOLoadErrorSharedNewline,
							 kMCStackdirStatusSyntaxError,
							 "Shared header not followed by new line")

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
 * Property loading
 * ================================================================ */


static inline bool
MCStackdirIOLoadProperty_Array (MCStackdirIORef op,
								MCStackdirIOScannerRef scanner,
								MCStringRef p_external_dir,
								MCStringRef p_storage_spec,
								MCArrayRef x_propset)
{
	MCNewAutoNameRef t_storage_spec_key;
	if (!MCNameCreate (p_storage_spec, &t_storage_spec_key))
		return MCStackdirIOErrorOutOfMemory (op);

	/* We need to ensure that a literal info array *already* exists in
	 * the state array for the destination array literal. */
	MCValueRef t_literal_info;
	if (!MCArrayFetchValue (x_propset,
							true,
							*t_storage_spec_key,
							t_literal_info))
		return MCStackdirIOLoadErrorArrayOrdering (op);

	/* Check that the literal info is actually an array.  If it isn't,
	 * then it indicates that the property was previously defined as a
	 * non-array basic type, which would be a syntax error. */
	if (!MCValueIsArray (t_literal_info))
		return MCStackdirIOLoadErrorArrayType (op);

	/* Get the actual literal from the literal info array.  Once
	 * again, this shouldn't fail. */
	bool t_success;
	MCValueRef t_array_props;
	t_success = MCArrayFetchValue ((MCArrayRef) t_literal_info,
								   true,
								   kMCStackdirLiteralKey,
								   t_array_props);
	MCAssert (t_success);

	/* It's possible that a properties file could define a property as
	 * a string, and the in a later line attempt to add an array entry
	 * to it.  That's a syntax error. */
	if (!MCValueIsArray (t_array_props))
		return MCStackdirIOLoadErrorArrayType (op);

	return MCStackdirIOLoadProperty (op,
									 scanner,
									 p_external_dir,
									 (MCArrayRef) t_array_props);
}

static inline bool
MCStackdirIOLoadProperty_Unquoted (MCStackdirIORef op,
								   MCStringRef p_literal,
								   MCValueRef & r_value)
{
	if (MCStringIsEqualTo (p_literal,
						   kMCStackdirTrueLiteral,
						   kMCStringOptionCompareExact))
	{
		/* TRUE */
		r_value = MCValueRetain (kMCTrue);
	}
	else if (MCStringIsEqualTo (p_literal,
								kMCStackdirFalseLiteral,
								kMCStringOptionCompareExact))
	{
		/* FALSE */
		r_value = MCValueRetain (kMCFalse);
	}
	else if (MCStringIsEqualTo (p_literal,
								kMCStackdirArrayLiteral,
								kMCStringOptionCompareExact))
	{
		/* ARRAY */
		MCArrayRef t_array_value;
		if (!MCArrayCreateMutable (t_array_value))
			return MCStackdirIOErrorOutOfMemory (op);
		r_value = t_array_value;
	}
	else
	{
		return MCStackdirIOLoadErrorPropertyUnquotedStringLiteral (op);
	}
	return true;
}

static inline bool
MCStackdirIOLoadProperty_ParseExternal (MCStackdirIORef op,
										MCStackdirIOScannerRef scanner,
										MCStringRef & r_type)
{
	/* The external indicator character "&" should already have been
	 * consumed. */

	/* The first item in the external descriptor must be the external
	 * storage type: string, data or array (after some whitespace). */
	MCStackdirIOToken t_type_token;
	MCAutoStringRef t_external_type_string;
	if (!(MCStackdirIOScannerConsume (scanner, t_type_token,
									  kMCStackdirIOTokenTypeSpace) &&
		  MCStackdirIOScannerConsume (scanner, t_type_token,
									  kMCStackdirIOTokenTypeUnquotedString)))
		return MCStackdirIOLoadErrorPropertyExternalType (op,
					kMCEmptyString, t_type_token.m_line, t_type_token.m_column);

	r_type = MCValueRetain ((MCStringRef) t_type_token.m_value);
	return true;
}

static bool
MCStackdirIOLoadProperty_LoadExternal (MCStackdirIORef op,
									   MCStringRef p_path,
									   MCValueTypeCode p_type_code,
									   MCValueRef & r_value)
{
	/* Read the file */
	MCAutoDataRef t_content;
	if (!MCS_loadbinaryfile (p_path, &t_content))
		return MCStackdirIOLoadErrorPropertyExternalRead (op, p_path);

	/* Convert the data */
	switch (p_type_code)
	{
	case kMCValueTypeCodeData:
		/* Data is trivially returned without modification */
		r_value = MCValueRetain (*t_content);
		break;

	case kMCValueTypeCodeString:
		{
			MCAutoStringRef t_string;
			/* Strings must be decoded as UTF-8 */
			if (!MCStringDecode (*t_content,
								 kMCStringEncodingUTF8,
								 false,
								 &t_string))
				return MCStackdirIOErrorOutOfMemory (op);
			r_value = MCValueRetain (*t_string);
		}
		break;

	case kMCValueTypeCodeArray:
		{
			/* Arrays need to be parsed */
			MCAutoArrayRef t_array;
			if (!MCArrayCreateMutable (&t_array))
				return MCStackdirIOErrorOutOfMemory (op);

			if (!MCStackdirIOLoadPropFile (op,
										   p_path,
										   nil, /* p_external_dir */
										   *t_array,
										   true))
				return false;
			r_value = MCValueRetain (*t_array);
		}
		break;
	default:
		MCUnreachable ();
	}

	return true;
}

static bool
MCStackdirIOLoadProperty_External (MCStackdirIORef op,
								   MCStackdirIOScannerRef scanner,
								   MCStringRef p_external_dir,
								   MCStringRef p_storage_spec,
								   MCValueRef & r_value)
{
	/* If p_external_dir is nil, then external properties are not
	 * permitted. */
	if (p_external_dir == nil)
		return MCStackdirIOLoadErrorPropertyExternalInvalid (op);

	/* Dealing with external files is more complicated!
	 * First parse the information from the property descriptor */
	MCAutoStringRef t_external_type_string;
	if (!MCStackdirIOLoadProperty_ParseExternal (op,
												 scanner,
												 &t_external_type_string))
		return false;

	/* Decode the storage type */
	MCValueTypeCode t_external_type_code;

	struct ExternalTypeInfo
	{
		MCStringRef m_str;
		MCValueTypeCode m_code;
	};
	struct ExternalTypeInfo t_type_map[] = {
		{ kMCStackdirStringType, kMCValueTypeCodeString },
		{ kMCStackdirDataType, kMCValueTypeCodeData },
		{ kMCStackdirArrayType, kMCValueTypeCodeArray },
		{ NULL, kMCValueTypeCodeNull },
	};

	t_external_type_code = kMCValueTypeCodeNull;
	for (int i = 0; t_type_map[i].m_str != NULL; ++i)
	{
		if (MCStringIsEqualTo (t_type_map[i].m_str,
							   *t_external_type_string,
							   kMCStringOptionCompareExact))
		{
			t_external_type_code = t_type_map[i].m_code;
			break;
		}
	}

	if (t_external_type_code == kMCValueTypeCodeNull)
		return MCStackdirIOLoadErrorPropertyExternalTypeCode (op);

	/* Generate the filename.  In some cases it may not be possible to
	 * do so, in which case one should have been provided. */
	/* FIXME it may be possible to factor this out from both here and
	 * the corresponding save code. */
	MCAutoStringRef t_file_name;
	MCStringRef t_suffix;
	switch (t_external_type_code)
	{
	case kMCValueTypeCodeString:
		t_suffix = kMCStackdirStringSuffix;
		break;
	case kMCValueTypeCodeArray:
		t_suffix = kMCStackdirArraySuffix;
		break;
	case kMCValueTypeCodeData:
		t_suffix = kMCStackdirDataSuffix;
		break;
	default:
		MCUnreachable ();
	}
	if (!MCStackdirFormatFilename (p_storage_spec,
								   t_suffix,
								   &t_file_name))
		return MCStackdirIOLoadErrorPropertyExternalPathRequired (op);

	/* Construct the full path to the external file */
	MCAutoStringRef t_path;
	if (!MCStringFormat (&t_path, "%@/%@", p_external_dir, *t_file_name))
		return MCStackdirIOErrorOutOfMemory (op);

	/* Load the data */
	return MCStackdirIOLoadProperty_LoadExternal (op,
												  *t_path,
												  t_external_type_code,
												  r_value);
}


static bool
MCStackdirIOLoadProperty (MCStackdirIORef op,
						  MCStackdirIOScannerRef scanner,
						  MCStringRef p_external_dir,
						  MCArrayRef x_propset)
{
	/* Storage spec */
	MCStackdirIOToken t_storage_token;
	MCStackdirIOScannerConsume (scanner, t_storage_token);
	switch (t_storage_token.m_type)
	{
	case kMCStackdirIOTokenTypeUnquotedString:
	case kMCStackdirIOTokenTypeString:
		break;

	default:
		return MCStackdirIOLoadErrorPropertyStorageSpec (op,
					kMCEmptyString, t_storage_token.m_line,
					t_storage_token.m_column);
	}

	MCAutoStringRef t_storage_spec;
	&t_storage_spec = MCValueRetain ((MCStringRef) t_storage_token.m_value);

	/* Storage separator (whitespace or ":") */
	/* If the next token is ":", then recurse (because this is a
	 * property descriptor for an entry in an array).  Otherwise, the
	 * next token must be space. */
	MCStackdirIOToken t_storage_sep_token;
	MCStackdirIOScannerConsume (scanner, t_storage_sep_token);
	switch (t_storage_sep_token.m_type)
	{
	case kMCStackdirIOTokenTypeSpace:
		break;
	case kMCStackdirIOTokenTypeStorageSeparator:
		bool t_success;
		MCStackdirIOErrorLocationPush (op, kMCEmptyString,
									   t_storage_sep_token.m_line,
									   t_storage_sep_token.m_column);
		t_success = MCStackdirIOLoadProperty_Array (op,
													scanner,
													p_external_dir,
													*t_storage_spec,
													x_propset);
		MCStackdirIOErrorLocationPop (op);
		return t_success;
	default:
		return MCStackdirIOLoadErrorPropertyStorageSeparator (op,
					kMCEmptyString, t_storage_sep_token.m_line,
					t_storage_sep_token.m_column);
	}

	/* The next token might be a type.  Alternatively, it might be a
	 * flag, or a literal value.  If it's a string or unquoted string,
	 * check if it's followed by a space -- that indicates that it
	 * must be a type. */
	MCStackdirIOToken t_type_token, t_value_token;
	MCNewAutoNameRef t_type;
	bool t_have_type, t_have_flags, t_have_value;
	MCStackdirIOScannerPeek (scanner, t_type_token);
	switch (t_type_token.m_type)
	{
	case kMCStackdirIOTokenTypeString:
	case kMCStackdirIOTokenTypeUnquotedString:
		{
			MCStackdirIOToken t_type_sep_token;
			MCStackdirIOScannerConsume (scanner, t_type_token);
			/* Check if the following token is a space */
			if (MCStackdirIOScannerPeek (scanner, t_type_sep_token,
										 kMCStackdirIOTokenTypeSpace))
			{
				MCStackdirIOScannerConsume (scanner, t_type_sep_token);
				if (!MCNameCreate ((MCStringRef) t_type_token.m_value,
								   &t_type))
					return MCStackdirIOErrorOutOfMemory (op);

				t_have_type = true;
				t_have_flags = true;
				t_have_value = false;
			}
			else
			{
				MCStackdirIOTokenCopy (t_type_token, t_value_token);

				t_have_type = false;
				t_have_flags = false;
				t_have_value = true;
			}
		}
		break;
	default:
		t_have_type = false;
		t_have_flags = true;
		t_have_value = false;
		break;
	}

	if (!t_have_type)
		&t_type = MCValueRetain (kMCEmptyName);

	/* Check for flags */
	MCStackdirIOToken t_flag_token;
	MCAutoArrayRef t_flags;
	if (t_have_flags &&
		MCStackdirIOScannerPeek (scanner, t_flag_token,
								 kMCStackdirIOTokenTypeFlag))
	{
		if (!MCArrayCreateMutable (&t_flags))
			return MCStackdirIOErrorOutOfMemory (op);
		t_have_flags = true;
	}
	else
	{
		&t_flags = MCValueRetain (kMCEmptyArray);
		t_have_flags = false;
	}

	/* Consume flags in a loop */
	index_t t_num_flags = 0;
	while (t_have_flags &&
		   MCStackdirIOScannerPeek (scanner, t_flag_token,
									kMCStackdirIOTokenTypeFlag))
	{
		MCStackdirIOScannerConsume (scanner, t_flag_token);

		/* The next tokens must be a string or unquoted string */
		if (!MCStackdirIOScannerConsume (scanner, t_flag_token))
			return MCStackdirIOLoadErrorPropertyFlagMissing (op,
						kMCEmptyString, t_flag_token.m_line,
						t_flag_token.m_column);

		/* The flag name must be followed by a space character */
		MCStackdirIOToken t_flag_sep_token;
		if (!MCStackdirIOScannerConsume (scanner, t_flag_sep_token,
										  kMCStackdirIOTokenTypeSpace))
			return MCStackdirIOLoadErrorPropertyFlagSep (op,
						kMCEmptyString, t_flag_token.m_line,
						t_flag_token.m_column);

		switch (t_flag_token.m_type)
		{
		case kMCStackdirIOTokenTypeString:
		case kMCStackdirIOTokenTypeUnquotedString:
			if (!MCArrayStoreValueAtIndex (*t_flags,
										   ++t_num_flags,
										   t_flag_token.m_value))
				return MCStackdirIOErrorOutOfMemory (op);
			break;
		default:
			return MCStackdirIOLoadErrorPropertyFlagType (op,
						kMCEmptyString, t_flag_token.m_line,
						t_flag_token.m_column);
		}
	}

	/* Obtain the value specifier.  We may have read it earlier while
	 * looking for the type. */
	if (!t_have_value)
	{
		MCStackdirIOScannerConsume (scanner, t_value_token); /* value */
		t_have_value = true;
	}

	/* Interpret the value */
	MCAutoValueRef t_value;
	MCStackdirIOErrorLocationPush (op, kMCEmptyString,
							t_value_token.m_line, t_value_token.m_column);
	switch (t_value_token.m_type)
	{
	case kMCStackdirIOTokenTypeUnquotedString:
		/* There are a few valid unquoted string values that can be
		 * stored in a property descriptor. */
		if (!MCStackdirIOLoadProperty_Unquoted (op,
										(MCStringRef) t_value_token.m_value,
										&t_value))
			return false;
		break;

	case kMCStackdirIOTokenTypeString:
	case kMCStackdirIOTokenTypeData:
	case kMCStackdirIOTokenTypeNumber:
		/* These are always self-evaluating! */
		&t_value = MCValueRetain (t_value_token.m_value);
		break;

	case kMCStackdirIOTokenTypeExternalIndicator:
		if (!MCStackdirIOLoadProperty_External (op,
												scanner,
												p_external_dir,
												*t_storage_spec,
												&t_value))
			return false;
		break;

	default:
		return MCStackdirIOLoadErrorPropertyLiteral (op,
											kMCEmptyString,
											t_value_token.m_line,
											t_value_token.m_column);
	}
	MCStackdirIOErrorLocationPop (op);

	/* Terminating newline */
	MCStackdirIOToken t_newline_token;
	if (!MCStackdirIOScannerConsume (scanner, t_newline_token,
									 kMCStackdirIOTokenTypeNewline))
		return MCStackdirIOLoadErrorPropertyNewline (op,
					kMCEmptyString, t_newline_token.m_line,
					t_newline_token.m_column);

	/* Finally, store the data in the state array */
	MCNewAutoNameRef t_key;
	if (!MCNameCreate (*t_storage_spec, &t_key))
		return MCStackdirIOErrorOutOfMemory (op);

	if (MCNameIsEmpty (*t_type) &&
		MCArrayIsEmpty (*t_flags) &&
		!MCValueIsArray (*t_value))
	{
		/* If there's no derived type involved, and the value *isn't*
		 * an array, then just store the value directly in the state
		 * array. */
		if (!MCArrayStoreValue (x_propset,
								true,
								*t_key,
								*t_value))
			return MCStackdirIOErrorOutOfMemory (op);
	}
	else
	{
		/* Create a literal info structure and store it in the propset. */
		MCAutoArrayRef t_literal_info;
		if (!(MCArrayCreateMutable (&t_literal_info) &&
			  MCArrayStoreValue (*t_literal_info,
								 true,
								 kMCStackdirTypeKey,
								 *t_type) &&
			  MCArrayStoreValue (*t_literal_info,
								 true,
								 kMCStackdirLiteralKey,
								 *t_value)))
			return MCStackdirIOErrorOutOfMemory (op);

		index_t t_flag_index;
		index_t t_num_flags = MCArrayGetCount (*t_flags);
		for (t_flag_index = 1; t_flag_index <= t_num_flags; ++t_flag_index)
		{
			MCValueRef t_flag_string;
			bool t_success;
			t_success = MCArrayFetchValueAtIndex (*t_flags,
												  t_flag_index,
												  t_flag_string);
			MCAssert (t_success); /* We just built this, it has to work! */

			MCNewAutoNameRef t_flag_key;
			if (!(MCNameCreate ((MCStringRef) t_flag_string, &t_flag_key) &&
				  MCArrayStoreValue (*t_literal_info,
									 true,
									 *t_flag_key,
									 kMCTrue)))
				return MCStackdirIOErrorOutOfMemory (op);
		}

		if (!MCArrayStoreValue (x_propset,
								true,
								*t_key,
								*t_literal_info))
		return MCStackdirIOErrorOutOfMemory (op);
	}
return true;
}

static bool
MCStackdirIOLoadPropFile (MCStackdirIORef op,
						  MCStringRef p_path,
						  MCStringRef p_external_dir,
						  MCArrayRef x_propset,
						  bool p_required)
{
	/* Load the file into memory.  If p_required is false, allow the
	 * file to be missing. */
	MCAutoStringRef t_content;
	if (!MCStackdirIOLoadUTF8 (op,
							   p_path,
							   &t_content,
							   !p_required))
		return (!MCStackdirIOHasError (op));

	/* Set up a scanner */
	MCStackdirIOScannerRef t_scanner;
	if (!MCStackdirIOScannerNew (*t_content, t_scanner))
		return MCStackdirIOErrorOutOfMemory (op);

	MCStackdirIOErrorLocationPush (op, p_path);

	/* Repeatedly parse property descriptors until EOF */
	bool t_success = true;
	MCStackdirIOToken t_token;
	while (t_success)
	{
		if (MCStackdirIOScannerPeek (t_scanner, t_token,
									 kMCStackdirIOTokenTypeEOF))
			break;

		t_success = MCStackdirIOLoadProperty (op,
											  t_scanner,
											  p_external_dir,
											  x_propset);
	}

	MCStackdirIOErrorLocationPop (op);
	MCStackdirIOScannerDestroy (t_scanner);

	return t_success;
}

static bool
MCStackdirIOLoadPropset (MCStackdirIORef op,
						 MCStringRef p_propset_dir,
						 MCStringRef p_external_dir,
						 MCArrayRef x_propset)
{
	MCAutoStringRef t_contents_path, t_overflow_path;
	if (!(MCStringFormat (&t_contents_path, "%@/%@",
						  p_propset_dir, kMCStackdirContentsFile) &&
		  MCStringFormat (&t_overflow_path, "%@/%@",
						  p_propset_dir, kMCStackdirOverflowFile)))
		return MCStackdirIOErrorOutOfMemory (op);

	return (MCStackdirIOLoadPropFile (op,
									  *t_contents_path,
									  p_external_dir,
									  x_propset,
									  false) &&
			MCStackdirIOLoadPropFile (op,
									  *t_overflow_path,
									  p_external_dir,
									  x_propset,
									  false));
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
	MCAutoArrayRef t_object_state, t_object_source_info;
	if (!(MCArrayCreateMutable (&t_object_state) &&
		  MCArrayCreateMutable (&t_object_source_info)))
		return MCStackdirIOErrorOutOfMemory (op);

	/* Create object load information structure */
	MCStackdirIOObjectLoad t_load_info;
	t_load_info.m_op = op;
	t_load_info.m_path = p_obj_path;
	t_load_info.m_state = *t_object_state;
	t_load_info.m_source_info = *t_object_source_info;

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
		  MCArrayStoreValue (op->m_source_info, true, *t_uuid_name,
							 (MCValueRef) *t_object_source_info)))
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
	 * to be missing. */
	MCAutoStringRef t_content;
	if (!MCStackdirIOLoadUTF8 (info->m_op,
							   *t_path,
							   &t_content,
							   !p_required))
		return (!MCStackdirIOHasError (info->m_op));

	/* Parse a token from the contents of the file.  If the file
	 * exists, we require it to contain a valid value. */
	MCStackdirIOScannerRef t_scanner;
	MCStackdirIOToken t_token;
	bool t_success = true;
	MCAutoValueRef t_value;
	if (!MCStackdirIOScannerNew (*t_content, t_scanner))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	if (t_success)
	{
		if (!MCStackdirIOScannerConsume (t_scanner, t_token) ||
			t_token.m_value == nil)
		{
			t_success = false;
			MCStackdirIOLoadErrorDirectLiteral (info->m_op,
												*t_path,
												t_token.m_line,
												t_token.m_column);
		}
	}

	if (t_success)
		&t_value = MCValueRetain (t_token.m_value);

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
	MCAutoArrayRef t_internal;
	if (!(MCArrayCreateMutable (&t_internal) &&
		  MCArrayStoreValue (info->m_state,
							 true,
							 kMCStackdirInternalKey,
							 *t_internal)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	return MCStackdirIOLoadPropset (info->m_op,
									info->m_path, /* p_propset_dir */
									info->m_path, /* p_external_dir */
									*t_internal);
}

static bool
MCStackdirIOLoadObjectCustom_Callback (void *context,
									   const MCSystemFolderEntry *p_entry)
{
	MCStackdirIOObjectLoadRef info = (MCStackdirIOObjectLoadRef) context;

	/* Check if the directory looks like it might be a property set.
	 * There's a special case for the default property set, which has
	 * an empty name. */
	MCAutoStringRef t_propset_name;
	if (MCStringIsEqualTo (p_entry->name,
						   MCSTR ("_empty.propset"),
						   kMCStringOptionCompareExact))
		&t_propset_name = MCValueRetain (kMCEmptyString);
	else if (!MCStackdirParseFilename (p_entry->name,
									   kMCStackdirPropsetSuffix,
									   &t_propset_name))
		return true;

	/* Build the full path to the directory */
	MCAutoStringRef t_path;
	if (!MCStringFormat (&t_path, "%@/%@", info->m_path, p_entry->name))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* If it *looks* like it might be a propset directory, it *must*
	 * be a directory */
	if (!p_entry->is_folder)
		return MCStackdirIOLoadErrorCustomNotDir (info->m_op,
												  *t_path);

	/* Get the array of custom property sets */
	MCValueRef t_custom;
	bool t_success;
	t_success = MCArrayFetchValue (info->m_state,
								   true,
								   kMCStackdirCustomKey,
								   t_custom);
	MCAssert (t_success); /* This *must* be present */
	MCAssert (MCValueIsArray (t_custom));

	/* Create a new array for this propset */
	MCAutoArrayRef t_propset;
	MCNewAutoNameRef t_propset_key;
	if (!(MCArrayCreateMutable (&t_propset) &&
		  MCNameCreate (*t_propset_name, &t_propset_key) &&
		  MCArrayStoreValue ((MCArrayRef) t_custom,
							 true,
							 *t_propset_key,
							 *t_propset)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	return MCStackdirIOLoadPropset (info->m_op,
									*t_path, /* p_propset_dir */
									*t_path, /* p_external_dir */
									*t_propset);
}

static bool
MCStackdirIOLoadObjectCustom (MCStackdirIOObjectLoadRef info)
{
	MCAutoArrayRef t_custom;
	MCAutoStringRef t_native_path;
	if (!(MCArrayCreateMutable (&t_custom) &&
		  MCArrayStoreValue (info->m_state,
							 true,
							 kMCStackdirCustomKey,
							 *t_custom) &&
		  MCS_pathtonative (info->m_path, &t_native_path)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* FIXME add support for reading the _propsets file -- see also
	 * MCStackdirIOSaveObjectPropsets(). */

	/* Iterate over the contents of the object directory, looking for
	 * property sets. */
	return MCsystem->ListFolderEntries (*t_native_path,
										MCStackdirIOLoadObjectCustom_Callback,
										info);
}

static bool
MCStackdirIOLoadObjectShared (MCStackdirIOObjectLoadRef info)
{
	MCAutoArrayRef t_shared;
	MCAutoStringRef t_shared_file;
	if (!(MCArrayCreateMutable (&t_shared) &&
		  MCArrayStoreValue (info->m_state,
							 true,
							 kMCStackdirSharedKey,
							 *t_shared) &&
		  MCStringFormat (&t_shared_file, "%@/%@",
						  info->m_path, kMCStackdirSharedFile)))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	/* Load the file into memory. It's permitted to be missing */
	MCAutoStringRef t_shared_content;
	if (!MCStackdirIOLoadUTF8 (info->m_op,
							   *t_shared_file,
							   &t_shared_content,
							   false)) /* required */
		return (!MCStackdirIOHasError (info->m_op));

	/* Set up a scanner */
	MCStackdirIOScannerRef t_scanner;
	if (!MCStackdirIOScannerNew (*t_shared_content, t_scanner))
		return MCStackdirIOErrorOutOfMemory (info->m_op);

	MCStackdirIOErrorLocationPush (info->m_op, *t_shared_file);

	/* Repeatedly parse shared property sections until EOF */
	bool t_success = true;
	MCStackdirIOToken t_token;
	while (t_success)
	{
		if (MCStackdirIOScannerPeek (t_scanner, t_token,
									 kMCStackdirIOTokenTypeEOF))
			break;

		/* Each section must start with a header, which contains the
		 * UUID  of the card. */
		if (!MCStackdirIOScannerConsume (t_scanner, t_token,
										 kMCStackdirIOTokenTypeSharedHeader))
		{
			MCStackdirIOLoadErrorSharedHeader (info->m_op, kMCEmptyString,
											   t_token.m_line, t_token.m_column);
			t_success = false;
			break;
		}

		/* Set up the array for the shared properties, and the path to
		 * the directory to be checked for external files. */
		MCStringRef t_uuid;
		MCNewAutoNameRef t_uuid_key;
		MCAutoArrayRef t_state;
		MCAutoStringRef t_external_dir;
		t_uuid = (MCStringRef) t_token.m_value;
		if (!(MCNameCreate ((MCStringRef) t_token.m_value, &t_uuid_key) &&
			  MCArrayCreateMutable (&t_state) &&
			  MCArrayStoreValue (*t_shared,
								 true,
								 *t_uuid_key,
								 *t_state) &&
			  MCStringFormat (&t_external_dir, "%@/%@%@",
							  info->m_path, t_uuid, kMCStackdirSharedSuffix)))
		{
			MCStackdirIOErrorOutOfMemory (info->m_op);
			t_success = false;
			break;
		}

		/* The next token must be a linefeed */
		if (!MCStackdirIOScannerConsume (t_scanner, t_token,
										 kMCStackdirIOTokenTypeNewline))
		{
			MCStackdirIOLoadErrorSharedNewline (info->m_op, kMCEmptyString,
												t_token.m_line, t_token.m_column);
			t_success = false;
			break;
		}

		/* Now parse properties until the next header or EOF */
		while (t_success)
		{
			if (MCStackdirIOScannerPeek (t_scanner, t_token,
										 kMCStackdirIOTokenTypeEOF) ||
				MCStackdirIOScannerPeek (t_scanner, t_token,
										 kMCStackdirIOTokenTypeSharedHeader))
				break;

			t_success = MCStackdirIOLoadProperty (info->m_op,
												  t_scanner,
												  *t_external_dir,
												  *t_state);
		}
	}

	MCStackdirIOErrorLocationPop (info->m_op);
	MCStackdirIOScannerDestroy (t_scanner);

	return t_success;
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
MCStackdirIOGetState (MCStackdirIORef op,
					  MCArrayRef *r_state,
					  MCArrayRef *r_source_info)
{
	MCStackdirIOAssertLoad (op);

	if (!MCStackdirIOLoadIsOperationComplete (op)) return false;
	if (MCStackdirIOHasError (op)) return false;

	MCArrayRef t_state;
	MCArrayRef t_source_info;
	t_state = op->m_load_state;
	t_source_info = op->m_source_info;

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
		  MCArrayCreateMutable (op->m_source_info)))
	{
		MCStackdirIOErrorOutOfMemory (op);
		return;
	}

	/* Sanity check that the target path has been set */
	MCAssert (op->m_path);

	/* Convert to native path */
	MCAutoStringRef t_native_path;
	if (!MCS_pathtonative (op->m_path, &t_native_path))
	{
		MCStackdirIOErrorOutOfMemory (op);
		return;
	}

	/* Check that the target path is, in fact, a stackdir */
	if (MCStackdirPathIsStackdir (op->m_path))
	{
		MCAutoStringRef t_save_cwd;
		bool t_success = true;

		MCStackdirIOErrorLocationPush (op, op->m_path);

		/* Iterate over stackdir contents (which should mostly be LSB
		 * directories) */
		if (t_success)
			t_success = MCsystem->ListFolderEntries (*t_native_path,
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
