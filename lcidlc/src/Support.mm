/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#include "LiveCode.h"

////////////////////////////////////////////////////////////////////////////////

#ifndef nil
#define nil NULL
#endif

#define BEGIN_EXTERN_C extern "C" {
#define END_EXTERN_C }

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCVariable *MCVariableRef;
typedef struct __MCVariableIterator *MCVariableIteratorRef;
typedef struct __MCObject *MCObjectRef;

typedef enum MCError
{
	kMCErrorNone = 0,
	kMCErrorOutOfMemory = 1,
	kMCErrorNotImplemented = 2,
	kMCErrorNoVariable = 3,
	kMCErrorNoValue = 4,
	kMCErrorNoIterator = 5,
	kMCErrorNoBuffer = 6,
	kMCErrorNotAnExternalTemporary = 7,
	kMCErrorInvalidValueType = 8,
	kMCErrorNotABoolean = 9,
	kMCErrorNotANumber = 10,
	kMCErrorNotAnInteger = 11,
	kMCErrorNotAString = 12,
	kMCErrorNotACString = 13,
	kMCErrorNotAnArray = 14,
	kMCErrorDstNotAString = 15,
	kMCErrorNumericOverflow = 16,
	kMCErrorInvalidCovertOctalsOption = 17,
	kMCErrorInvalidCaseSensitiveOption = 18,
	kMCErrorInvalidVariableQuery = 19,
	kMCErrorInvalidContextQuery = 20,
	kMCErrorVariableNoLongerExists = 21,
	kMCErrorVariableInvalidEdit = 22,
	kMCErrorNoObject = 23,
	kMCErrorNoObjectId = 24,
	kMCErrorNoObjectMessage = 25,
	kMCErrorNoObjectArguments = 26,
	kMCErrorMalformedObjectChunk = 27,
	kMCErrorCouldNotResolveObject = 28,
	kMCErrorObjectDoesNotExist = 29,
	kMCErrorNoDefaultStack = 30,
	kMCErrorAborted = 31,
	kMCErrorFailed = 32,
	kMCErrorExited = 33,
	kMCErrorNoObjectProperty = 34,
	kMCErrorNoObjectPropertyValue = 35,
	kMCErrorInvalidInterfaceQuery = 36,
	kMCErrorNotSupported = 37,
} MCError;

typedef uint32_t MCValueOptions;
enum
{
	kMCOptionAsVariable = 0,
	kMCOptionAsBoolean = 1,
	kMCOptionAsInteger = 2,
	kMCOptionAsCardinal = 3,
	kMCOptionAsReal = 4,
	kMCOptionAsString = 5,
	kMCOptionAsCString = 6,
	kMCOptionNumberFormatDefault = 0 << 26,
	kMCOptionNumberFormatDecimal = 1 << 26,
	kMCOptionNumberFormatScientific = 2 << 26,
	kMCOptionNumberFormatGeneral = 3 << 26,
	kMCOptionConvertOctalsDefault = 0 << 28,
	kMCOptionConvertOctalsTrue = 1 << 28,
	kMCOptionConvertOctalsFalse = 2 << 28,
	kMCOptionCaseSensitiveDefault = 0 << 30,
	kMCOptionCaseSensitiveTrue = 1 << 30,
	kMCOptionCaseSensitiveFalse = 2 << 30
};

typedef enum MCDispatchType
{
	kMCDispatchTypeCommand = 0,
	kMCDispatchTypeFunction = 1
} MCDispatchType;

typedef enum MCDispatchStatus
{
	kMCDispatchStatusHandled = 0,
	kMCDispatchStatusNotHandled = 1,
	kMCDispatchStatusPassed = 2,
	kMCDispatchStatusError = 3,
	kMCDispatchStatusExit = 4,
	kMCDispatchStatusAbort = 5,
} MCDispatchStatus;

enum
{
	kMCRunOnMainThreadSend = 0 << 0,
	kMCRunOnMainThreadPost = 1 << 0,
	kMCRunOnMainThreadOptional = 0 << 1,
	kMCRunOnMainThreadRequired = 1 << 1,
	kMCRunOnMainThreadSafe = 0 << 2,
	kMCRunOnMainThreadUnsafe = 1 << 2,
	kMCRunOnMainThreadImmediate = 0 << 3,
	kMCRunOnMainThreadDeferred = 1 << 3,
	kMCRunOnMainThreadJumpToUI = 1 << 4,
	kMCRunOnMainThreadJumpToEngine = 2 << 4,
};

typedef struct MCExternalHandler
{
	uint32_t type;
	const char *name;
	bool (*function)(MCVariableRef *, uint32_t, MCVariableRef);
} MCExternalHandler;

typedef enum MCExternalContextVar
{
	kMCExternalContextVarMe = 1,
	kMCExternalContextVarTarget = 2,
	kMCExternalContextVarResult = 3,
	kMCExternalContextVarIt = 4,
	kMCExternalContextVarCaseSensitive = 5,
	kMCExternalContextVarConvertOctals = 6,
	kMCExternalContextVarNumberFormat = 7,
	kMCExternalContextVarItemDelimiter = 8,
	kMCExternalContextVarLineDelimiter = 9,
	kMCExternalContextVarColumnDelimiter = 10,
	kMCExternalContextVarRowDelimiter = 11,
	kMCExternalContextVarDefaultStack = 12,
	kMCExternalContextVarDefaultCard = 13
} MCExternalContextVar;

typedef enum MCExternalVariableQuery
{
	kMCExternalVariableQueryIsTemporary = 1,
	kMCExternalVariableQueryIsTransient = 2,
	kMCExternalVariableQueryFormat = 3,
	kMCExternalVariableQueryRetention = 4,
	kMCExternalVariableQueryIsAnArray = 5,
	kMCExternalVariableQueryIsAString = 6,
	kMCExternalVariableQueryIsANumber = 7,
	kMCExternalVariableQueryIsAnInteger = 8,
	kMCExternalVariableQueryIsABoolean = 9,
} MCExternalVariableQuery;

typedef enum MCExternalInterfaceQuery
{
	kMCExternalInterfaceQueryView = 1,
	kMCExternalInterfaceQueryViewScale = 2,
	kMCExternalInterfaceQueryViewController = 3,
	kMCExternalInterfaceQueryActivity = 4,
	kMCExternalInterfaceQueryContainer = 5,
} MCExternalInterfaceQuery;

enum
{
	kMCExternalObjectUpdateOptionRect = 1 << 0,
};

typedef struct MCExternalInterface
{
	uint32_t version;
	MCError (*engine_run_on_main_thread)(void *callback, void *callback_state, uint32_t options);
	MCError (*context_query)(MCExternalContextVar op, void *result);
	MCError (*variable_create)(MCVariableRef* var);
	MCError (*variable_retain)(MCVariableRef var);
	MCError (*variable_release)(MCVariableRef var);
	MCError (*variable_query)(MCVariableRef var, MCExternalVariableQuery query, void *result);
	MCError (*variable_clear)(MCVariableRef var);
	MCError (*variable_exchange)(MCVariableRef var_a, MCVariableRef var_b);
	MCError (*variable_store)(MCVariableRef var, MCValueOptions options, void *value);
	MCError (*variable_fetch)(MCVariableRef var, MCValueOptions options, void *value);
	MCError (*variable_append)(MCVariableRef var, MCValueOptions options, void *value);
	MCError (*variable_prepend)(MCVariableRef var, MCValueOptions options, void *value);
	MCError (*variable_edit)(MCVariableRef var, MCValueOptions options, uint32_t reserve_length, void **r_buffer, uint32_t *r_current_length);
	MCError (*variable_count_keys)(MCVariableRef var, uint32_t* r_count);
	MCError (*variable_iterate_keys)(MCVariableRef var, MCVariableIteratorRef *iterator, MCValueOptions options, void *key, MCVariableRef *r_value);
	MCError (*variable_remove_key)(MCVariableRef var, MCValueOptions options, void *key);
	MCError (*variable_lookup_key)(MCVariableRef var, MCValueOptions options, void *key, bool p_ensure, MCVariableRef *r_var);
	MCError (*object_resolve)(const char *chunk, MCObjectRef* r_object);
	MCError (*object_exists)(MCObjectRef object, bool *r_exists);
	MCError (*object_retain)(MCObjectRef object);
	MCError (*object_release)(MCObjectRef object);
	MCError (*object_dispatch)(MCObjectRef object, MCDispatchType type, const char *message, MCVariableRef *argv, uint32_t argc, MCDispatchStatus *r_status);
	MCError (*wait_run)(void *unused, unsigned int options);
	MCError (*wait_break)(void *unused, unsigned int options);
	MCError (*object_get)(MCObjectRef object, unsigned int options, const char *name, const char *key, MCVariableRef value);
	MCError (*object_set)(MCObjectRef object, unsigned int options, const char *name, const char *key, MCVariableRef value);
	MCError (*interface_query)(MCExternalInterfaceQuery query, void *result);
	MCError (*object_update)(MCObjectRef object, unsigned int options, void *region);
} MCExternalInterface;

typedef struct MCExternalInfo
{
	uint32_t version;
	uint32_t flags;
	const char *name;
	MCExternalHandler *handlers;
} MCExternalInfo;

typedef struct MCExternalLibraryExport
{
	const char *name;
	void *address;
} MCExternalLibraryExport;

typedef struct MCExternalLibraryInfo
{
	const char **name;
	struct MCExternalLibraryExport *exports;
} MCExternalLibraryInfo;

static MCExternalInterface *s_interface = 0;

////////////////////////////////////////////////////////////////////////////////

static MCError MCVariableCreate(MCVariableRef *r_var)
{
	return s_interface -> variable_create(r_var);
}

static MCError MCVariableRetain(MCVariableRef var)
{
	return s_interface -> variable_retain(var);
}

static MCError MCVariableRelease(MCVariableRef var)
{
	return s_interface -> variable_release(var);
}

static MCError MCVariableIsTransient(MCVariableRef var, bool *r_transient)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsTransient, r_transient);
}

static MCError MCVariableIsAnArray(MCVariableRef var, bool *r_array)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsAnArray, r_array);
}

static MCError MCVariableStore(MCVariableRef var, MCValueOptions options, void *value)
{
	return s_interface -> variable_store(var, options, value);
}

static MCError MCVariableFetch(MCVariableRef var, MCValueOptions options, void *value)
{
	return s_interface -> variable_fetch(var, options, value);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

BEGIN_EXTERN_C

static unsigned int LCValueOptionsGetCaseSensitive(unsigned int p_options)
{
	if ((p_options & kLCValueOptionMaskCaseSensitive) == 0)
		return kMCOptionCaseSensitiveFalse;
	
	return ((p_options & kLCValueOptionMaskCaseSensitive) + (1 << 30)) & kLCValueOptionMaskCaseSensitive;
}

static unsigned int LCValueOptionsGetConvertOctals(unsigned int p_options)
{
	if ((p_options & kLCValueOptionMaskConvertOctals) == 0)
		return kLCValueOptionConvertOctalsFalse;
	
	return ((p_options & kLCValueOptionMaskConvertOctals) + (1 << 30)) & kLCValueOptionMaskConvertOctals;
}

static unsigned int LCValueOptionsGetNumberFormat(unsigned int p_options)
{
	if ((p_options & kLCValueOptionMaskNumberFormat) == 0)
		return kLCValueOptionConvertOctalsFalse;
	
	return ((p_options & kLCValueOptionMaskNumberFormat) + (1 << 26)) & kLCValueOptionMaskNumberFormat;
}

static MCError LCValueFetch(MCVariableRef p_var, unsigned int p_options, void *r_value)
{
	MCError t_error;
	t_error = kMCErrorNone;

	unsigned int t_options_to_use;
	void *t_value_to_use;
	union
	{
		double t_number_value;
		LCBytes t_cdata_value;
		char *t_cstring_value;
	};
	if (t_error == kMCErrorNone)
	{
		switch(p_options & kLCValueOptionMaskAs)
		{
			case kLCValueOptionAsBoolean:
				t_options_to_use = kMCOptionAsBoolean;
				t_value_to_use = r_value;
				break;
			case kLCValueOptionAsInteger:
				t_options_to_use = kMCOptionAsInteger;
				t_options_to_use |= LCValueOptionsGetConvertOctals(p_options);
				t_value_to_use = r_value;
				break;
			case kLCValueOptionAsReal:
				t_options_to_use = kMCOptionAsReal;
				t_options_to_use |= LCValueOptionsGetConvertOctals(p_options);
				t_value_to_use = r_value;
				break;
				
			case kLCValueOptionAsCString:
				t_options_to_use = kMCOptionAsCString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cstring_value;
				break;
			case kLCValueOptionAsCData:
				t_options_to_use = kMCOptionAsString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cdata_value;
				break;
				
#ifdef __OBJC__
			case kLCValueOptionAsObjcNumber:
				t_options_to_use = kMCOptionAsReal;
				t_options_to_use |= LCValueOptionsGetConvertOctals(p_options);
				t_value_to_use = &t_number_value;
				break;
			case kLCValueOptionAsObjcString:
				t_options_to_use = kMCOptionAsCString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cstring_value;
				break;
			case kLCValueOptionAsObjcData:
				t_options_to_use = kMCOptionAsString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cdata_value;
				break;
#endif
				
			default:
				t_error = (MCError)kLCErrorBadValueOptions;
		}
		if (t_error == kMCErrorNone)
			t_error = s_interface -> variable_fetch(p_var, t_options_to_use, t_value_to_use);
	}
	
	if (t_error == kMCErrorNone && t_value_to_use == r_value)
		return kMCErrorNone;
	
	if (t_error == kMCErrorNone)
	{
		switch(p_options & kLCValueOptionMaskAs)
		{
			case kLCValueOptionAsCString:
				t_cstring_value = strdup(t_cstring_value);
				if (t_cstring_value != nil)
					*(char **)r_value = t_cstring_value;
				else
					t_error = kMCErrorOutOfMemory;
				break;
			case kLCValueOptionAsCData:
			{
				void *t_buffer;
				t_buffer = malloc(t_cdata_value . length);
				if (t_buffer != nil)
				{
					memcpy(t_buffer, t_cdata_value . buffer, t_cdata_value . length);
					((LCBytes *)r_value) -> buffer = t_buffer;
					((LCBytes *)r_value) -> length = t_cdata_value . length;
				}
				else
					t_error = kMCErrorOutOfMemory;
			}
			break;
				
#ifdef __OBJC__
			case kLCValueOptionAsObjcNumber:
				*(NSNumber **)r_value = [NSNumber numberWithDouble: t_number_value];
				break;
			case kLCValueOptionAsObjcString:
				*(NSString **)r_value = [NSString stringWithCString: t_cstring_value encoding: NSMacOSRomanStringEncoding];
				break;
			case kLCValueOptionAsObjcData:
				*(NSData **)r_value = [NSData dataWithBytes: t_cdata_value . buffer length: t_cdata_value . length];
				break;
#endif
		}
	}

	return (MCError)t_error;
}	

static MCError LCValueStore(MCVariableRef p_var, unsigned int p_options, void *p_value)
{
	MCError t_error;
	t_error = kMCErrorNone;

	unsigned int t_options_to_use;
	void *t_value_to_use;
	union
	{
		double t_number_value;
		const char *t_cstring_value;
		LCBytes t_cdata_value;
	};
	
	switch(p_options & kLCValueOptionMaskAs)
	{
		case kLCValueOptionAsBoolean:
		case kLCValueOptionAsInteger:
		case kLCValueOptionAsReal:
		case kLCValueOptionAsCString:
		case kLCValueOptionAsCData:
			t_options_to_use = p_options & kLCValueOptionMaskAs;
			t_value_to_use = p_value;
			break;
		case kLCValueOptionAsLCArray:
			t_options_to_use = kMCOptionAsVariable;
			t_value_to_use = *(void **)p_value;
			break;
#ifdef __OBJC__
		case kLCValueOptionAsObjcNumber:
			t_options_to_use = kMCOptionAsReal;
			t_number_value = [*(NSNumber **)p_value doubleValue];
			t_value_to_use = &t_number_value;
			break;
		case kLCValueOptionAsObjcString:
			t_options_to_use = kMCOptionAsCString;
			t_cstring_value = [*(NSString **)p_value cStringUsingEncoding: NSMacOSRomanStringEncoding];
			if (t_cstring_value != nil)
				t_value_to_use = &t_cstring_value;
			else
				t_error = (MCError)kLCErrorCannotEncodeCString;
			break;
		case kLCValueOptionAsObjcData:
			t_options_to_use = kMCOptionAsString;
			t_cdata_value . buffer = (char *)[*(NSData **)p_value bytes];
			t_cdata_value . length = [*(NSData **)p_value length];
			t_value_to_use = &t_cdata_value;
			break;
#endif
		default:
			t_error = (MCError)kLCErrorBadValueOptions;
			break;
	}
	
	if (t_error == kMCErrorNone)
		t_error = s_interface -> variable_store(p_var, t_options_to_use, t_value_to_use);
	
	return (MCError)t_error;
}

#ifdef __NOT_YET_FINISHED__

LCError LCArrayCreate(unsigned int p_options, LCArrayRef* r_array)
{
	return (LCError)s_interface -> variable_create((MCVariableRef *)r_array);
}

LCError LCArrayRetain(LCArrayRef p_array)
{
	return (LCError)s_interface -> variable_retain((MCVariableRef)p_array);
}

LCError LCArrayRelease(LCArrayRef p_array)
{
	return (LCError)s_interface -> variable_release((MCVariableRef)p_array);
}

//////////

static MCError LCArrayFetchAsArray(MCVariableRef p_key, bool *r_exists, void *r_value)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	bool t_is_array;
	if (t_error == kMCErrorNone)
	{
		s_interface -> variable_query(p_key, kMCExternalVariableQueryIsAnArray, &t_is_array);
		if (!t_is_array)
			t_error = kMCErrorNotAnArray;
	}
	
	MCVariableRef t_var_copy;
	t_var_copy = nil;
	if (t_error == kMCErrorNone)
		t_error = s_interface -> variable_create(&t_var_copy);
	
	if (t_error == kMCErrorNone)
		t_error = s_interface -> variable_store(t_var_copy, kMCOptionAsVariable, p_key);
	
	if (t_error == kMCErrorNone)
	{
		*(LCArrayRef *)r_value = (LCArrayRef)t_var_copy;
		*r_exists = true;
	}
	
	return t_error;
}

static MCError LCArrayResolvePath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, MCVariableRef& r_var)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	MCVariableRef t_key;
	t_key = (MCVariableRef)p_array;
	for(unsigned int i = 0; i <= p_path_length; i++)
	{
		t_error = s_interface -> variable_lookup_key((MCVariableRef)t_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, i < p_path_length ? (void *)&p_path[i] : (void *)&p_key, false, &t_key);
		if (t_error != kMCErrorNone || t_key == nil)
			break;
	}
	
	if (t_error == kMCErrorNone)
		r_var = t_key;
	
	return t_error;
}

//////////

LCError LCArrayCountKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, unsigned int *r_count)
{		
	MCError t_error;
	t_error = kMCErrorNone;
	
	if (p_options != 0)
		t_error = (MCError)kLCErrorBadArrayOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kMCErrorNone)
	{
		if (t_var == nil)
			*r_count = 0;
		else
			t_error = s_interface -> variable_count_keys(t_var, r_count);
	}
	
	return (LCError)t_error;
}

LCError LCArrayListAllKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, char **p_key_buffer, unsigned int p_key_buffer_size)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	if (p_options != 0)
		t_error = (MCError)kLCErrorBadArrayOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	unsigned int t_key_count;
	t_key_count = 0;
	if (t_error == kMCErrorNone)
		t_error = s_interface -> variable_count_keys((MCVariableRef)p_array, &t_key_count);
	
	if (t_error == kMCErrorNone && t_key_count > p_key_buffer_size)
		t_error = (MCError)kLCErrorArrayBufferTooSmall;
	
	char **t_keys;
	t_keys = nil;
	if (t_error == kMCErrorNone)
	{
		t_keys = (char **)calloc(sizeof(char *), t_key_count);
		if (t_keys == nil)
			t_error = (MCError)kLCErrorOutOfMemory;
	}
	
	MCVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(unsigned int i = 0; i < t_key_count && t_error == kMCErrorNone; i++)
	{
		MCVariableRef t_key_var;
		t_error = s_interface -> variable_iterate_keys(t_var, &t_iterator, kMCOptionAsCString, &t_keys[i], &t_key_var);
		if (t_error == kMCErrorNone)
		{
			t_keys[i] = strdup(t_keys[i]);
			if (t_keys[i] == NULL)
				t_error = kMCErrorNone;
		}
	}
	
	if (t_error == kMCErrorNone)
	{
		memcpy(p_key_buffer, t_keys, sizeof(char *) * t_key_count);
		memset(p_key_buffer + t_key_count, 0, sizeof(char *) * (p_key_buffer_size - t_key_count));
	}
	else
	{
		if (t_keys != nil)
			for(unsigned int i = 0; i < t_key_count && t_keys[i] != nil; i++)
				free(t_keys[i]);
	}
	
	free(t_keys);
	
	return (LCError)t_error;
}

LCError LCArrayRemoveAllKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	if (p_options != 0)
		t_error = (MCError)kLCErrorBadArrayOptions;
	
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kMCErrorNone)
		s_interface -> variable_clear(t_var);
	
	return (LCError)t_error;
}

LCError LCArrayHasKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, bool *r_exists)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	if ((p_options & ~kLCValueOptionMaskCaseSensitive) != 0)
		t_error = (MCError)kLCErrorBadArrayOptions;

	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
		t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length, p_key, t_var);
	
	if (t_error == kMCErrorNone)
		*r_exists = t_var != nil;
	
	return (LCError)t_error;
}

LCError LCArrayHasKeyWithPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, bool *r_exists)
{
	if (p_path_length == 0)
		return kLCErrorBadArrayPath;
	
	return LCArrayHasKeyOnPath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], r_exists);
}

LCError LCArrayLookupKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, bool *r_exists, void *r_value)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
		t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length, p_key, t_var);
	
	if (t_error == kMCErrorNone && t_var == nil)
	{
		switch(p_options & kLCValueOptionMaskAs)
		{
		case kLCValueOptionAsBoolean:
			*(bool *)r_value = false;
			break;
		case kLCValueOptionAsInteger:
			*(int *)r_value = 0;
			break;
		case kLCValueOptionAsReal:
			*(double *)r_value = 0.0;
			break;
		case kLCValueOptionAsCData:
			((LCBytes *)r_value) -> length = 0;
			((LCBytes *)r_value) -> buffer = nil;
			break;
		case kLCValueOptionAsCString:
		case kLCValueOptionAsLCArray:
		case kLCValueOptionAsObjcNumber:
		case kLCValueOptionAsObjcString:
		case kLCValueOptionAsObjcData:
			*(void **)r_value = nil;
			break;
		default:
			t_error = (MCError)kLCErrorBadArrayOptions;
			break;
		}
		
		if (t_error == kMCErrorNone)
			*r_exists = false;
		
		return (LCError)t_error;
	}
	
	if (t_error == kMCErrorNone && (p_options & kLCValueOptionMaskAs) == kLCValueOptionAsLCArray)
		return (LCError)LCArrayFetchAsArray(t_var, r_exists, r_value);
	
	if (t_error == kMCErrorNone)
		t_error = LCValueFetch(t_var, p_options, r_value);
	
	if (t_error == kMCErrorNone)
		*r_exists = true;
	
	return (LCError)t_error;
}

LCError LCArrayLookupKeyWithPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, bool *r_exists, void *r_value)
{
	if (p_path_length == 0)
		return kLCErrorBadArrayPath;
	
	return LCArrayLookupKeyOnPath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], r_exists, r_value);
}

LCError LCArrayFetchKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, void *r_value)
{
	bool t_exists;
	return LCArrayLookupKeyOnPath(p_array, p_options, p_path, p_path_length, p_key, &t_exists, r_value);
}

LCError LCArrayFetchKeyWithPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, void *r_value)
{
	bool t_exists;
	return LCArrayLookupKeyWithPath(p_array, p_options, p_path, p_path_length, &t_exists, r_value);
}

LCError LCArrayStoreKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, void *p_value)
{
	MCError t_error;
	t_error = kMCErrorNone;

	unsigned int t_path_index;
	MCVariableRef t_previous_key, t_key;
	t_key = (MCVariableRef)p_array;
	for(t_path_index = 0; t_path_index <= p_path_length; t_path_index++)
	{
		t_previous_key = t_key;
		t_error = s_interface -> variable_lookup_key((MCVariableRef)t_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, t_path_index < p_path_length ? (void *)&p_path[t_path_index] : (void *)&p_key, false, &t_key);
		if (t_error != kMCErrorNone || t_key == nil)
			break;
	}
	
	if (t_error == kMCErrorNone && t_key == nil)
		t_error = s_interface -> variable_create(&t_key);
	
	if (t_error == kMCErrorNone)
	{
		t_error = LCValueStore(t_key, p_options, p_value);
	}
	
	if (t_error == kMCErrorNone && t_path_index == p_path_length + 1)
		return kLCErrorNone;
	
	MCVariableRef t_parent_key;
	t_parent_key = nil;
	if (t_error == kMCErrorNone && t_path_index < p_path_length)
		t_error = s_interface -> variable_create(&t_parent_key);
	
	for(unsigned int i = p_path_length + 1; i > t_path_index && t_error == kMCErrorNone; i--)
	{
		MCVariableRef t_key_value;
		if (t_error == kMCErrorNone)
			t_error = s_interface -> variable_lookup_key(i - 1 == t_path_index ? t_previous_key : t_parent_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, i <= p_path_length ? (void *)&p_path[i - 1] : (void *)&p_key, true, &t_key_value);
		if (t_error == kMCErrorNone)
			t_error = s_interface -> variable_exchange(t_key_value, t_key);
		if (t_error == kMCErrorNone)
		{
			MCVariableRef t;
			t = t_parent_key;
			t_parent_key = t_key;
			t_key = t;
		}
	}
	
	if (t_parent_key != nil)
		s_interface -> variable_release(t_parent_key);
	
	if (t_key != nil)
		s_interface -> variable_release(t_key);
	
	return (LCError)t_error;
}

LCError LCArrayStoreKeyWithPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, void *p_value)
{
	if (p_path_length == 0)
		return kLCErrorBadArrayPath;
	
	return LCArrayStoreKeyOnPath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], p_value);
}

LCError LCArrayRemoveKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	if ((p_options & ~kLCValueOptionMaskCaseSensitive) != 0)
		t_error = (MCError)kLCErrorBadArrayOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kMCErrorNone && t_var != nil)
		t_error = s_interface -> variable_remove_key(t_var, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, (void *)&p_key);
		
	return (LCError)t_error;
}

LCError LCArrayRemoveKeyWithPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length)
{
	if (p_path_length == 0)
		return kLCErrorBadArrayPath;
	
	return LCArrayRemoveKeyOnPath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1]);
}

//////////

LCError LCArrayCountKeys(LCArrayRef array, unsigned int options, unsigned int *r_count)
{
	return LCArrayCountKeysOnPath(array, options, nil, 0, r_count);
}

LCError LCArrayListAllKeys(LCArrayRef array, unsigned int options, char **key_buffer, unsigned int key_buffer_size)
{
	return LCArrayListAllKeysOnPath(array, options, nil, 0, key_buffer, key_buffer_size);
}

LCError LCArrayRemoveAllKeys(LCArrayRef array, unsigned int options)
{
	return LCArrayRemoveAllKeysOnPath(array, options, nil, 0);
}

LCError LCArrayHasKey(LCArrayRef array, unsigned int options, const char *key, bool *r_exists)
{
	return LCArrayHasKeyWithPath(array, options, &key, 1, r_exists);
}

LCError LCArrayLookupKey(LCArrayRef array, unsigned int options, const char *key, bool *r_exists, void *r_value)
{
	return LCArrayLookupKeyWithPath(array, options, &key, 1, r_exists, r_value);
}

LCError LCArrayFetchKey(LCArrayRef array, unsigned int options, const char *key, void *r_value)
{
	return LCArrayFetchKeyWithPath(array, options, &key, 1, r_value);
}

LCError LCArrayStoreKey(LCArrayRef array, unsigned int options, const char *key, void *value)
{
	return LCArrayStoreKeyWithPath(array, options, &key, 1, value);
}

LCError LCArrayRemoveKey(LCArrayRef array, unsigned int options, const char *key)
{
	return LCArrayRemoveKeyWithPath(array, options, &key, 1);
}

#endif

END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////

BEGIN_EXTERN_C

typedef struct __LCObject *LCObjectRef;
typedef struct __LCWait *LCWaitRef;

LCError LCContextMe(LCObjectRef *r_object)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarMe, r_object);
}

LCError LCContextTarget(LCObjectRef *r_target)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarTarget, r_target);
}

LCError LCContextDefaultStack(LCObjectRef *r_object)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarDefaultStack, r_object);
}

LCError LCContextDefaultCard(LCObjectRef *r_object)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarDefaultCard, r_object);
}

//////////

LCError LCObjectResolve(const char *chunk, LCObjectRef *r_object)
{
	return (LCError)s_interface -> object_resolve(chunk, (MCObjectRef *)r_object);
}

LCError LCObjectRetain(LCObjectRef object)
{
	return (LCError)s_interface -> object_retain((MCObjectRef)object);
}

LCError LCObjectRelease(LCObjectRef object)
{
	return (LCError)s_interface -> object_release((MCObjectRef)object);
}

LCError LCObjectExists(LCObjectRef object, bool *r_exists)
{
	return (LCError)s_interface -> object_exists((MCObjectRef)object, r_exists);
}

static void LCArgumentsDestroy(MCVariableRef *p_argv, uint32_t p_argc)
{
	for(uint32_t i = 0; i < p_argc; i++)
		if (p_argv[i] != nil)
			MCVariableRelease(p_argv[i]);
	free(p_argv);
}
	
static MCError LCArgumentsCreateV(const char *p_signature, va_list p_args, MCVariableRef*& r_argv, uint32_t& r_argc)
{
	uint32_t t_argc;
	t_argc = strlen(p_signature);
	
	MCVariableRef *t_argv;
	t_argv = (MCVariableRef *)calloc(t_argc, sizeof(MCVariableRef));
	if (t_argv == nil)
		return kMCErrorOutOfMemory;
	
	MCError t_error;
	t_error = kMCErrorNone;
	for(uint32_t i = 0; i < t_argc; i++)
	{
		t_error = MCVariableCreate(&t_argv[i]);
		if (t_error != kMCErrorNone)
			break;
		
		switch(p_signature[i])
		{
			case 'b': // boolean
			{
				bool t_boolean;
				t_boolean = va_arg(p_args, int) != 0;
				t_error = MCVariableStore(t_argv[i], kMCOptionAsBoolean, &t_boolean);
			}
			break;
				
			case 'i': // integer
			{
				int t_integer;
				t_integer = va_arg(p_args, int);
				t_error = MCVariableStore(t_argv[i], kMCOptionAsInteger, &t_integer);
			}
			break;
				
			case 'r': // real
			{
				double t_real;
				t_real = va_arg(p_args, double);
				t_error = MCVariableStore(t_argv[i], kMCOptionAsReal, &t_real);
			}
			break;
				
			case 'z': // cstring
			{
				const char *t_cstring;
				t_cstring = va_arg(p_args, const char *);
				t_error = MCVariableStore(t_argv[i], kMCOptionAsCString, &t_cstring);
			}
			break;
				
			case 'y': // bytes
			{
				const LCBytes *t_bytes;
				t_bytes = va_arg(p_args, const LCBytes *);
				t_error = MCVariableStore(t_argv[i], kMCOptionAsString, &t_bytes);
			}
			break;
				
#ifdef __OBJC__
			case 'N': // NSNumber*
			{
				NSNumber* t_number;
				t_number = va_arg(p_args, NSNumber *);
				double t_real;
				t_real = [t_number doubleValue];
				t_error = MCVariableStore(t_argv[i], kMCOptionAsReal, &t_real);
			}
			break;
				
			case 'S': // NSString*
			{
				NSString *t_string;
				t_string = va_arg(p_args, NSString *);
				const char *t_cstring;
				t_cstring = [t_string cStringUsingEncoding: NSMacOSRomanStringEncoding];
				if (t_cstring == nil)
					t_error = (MCError)kLCErrorCannotEncodeCString;
				t_error = MCVariableStore(t_argv[i], kMCOptionAsCString, &t_cstring);
			}
			break;
				
			case 'D': // NSData*
			{
				NSData *t_data;
				t_data = va_arg(p_args, NSData *);
				LCBytes t_string;
				t_string . buffer = (char *)[t_data bytes];
				t_string . length = [t_data length];
				t_error = MCVariableStore(t_argv[i], kMCOptionAsString, &t_string);
			}
			break;
#endif
		}
	}
	
	if (t_error == kMCErrorNone)
	{
		r_argv = t_argv;
		r_argc = t_argc;
	}
	else
		LCArgumentsDestroy(t_argv, t_argc);
	
	return t_error;
}		
	
struct LCObjectPostV_event
{
	MCObjectRef object;
	char *message;
	MCVariableRef *argv;
	uint32_t argc;
};

struct LCObjectPostV_context
{
	MCObjectRef object;
	const char *message;
	const char *signature;
	va_list args;
	LCError result;
};

static void LCObjectPostV_dispatch(void *p_context, int p_flags)
{
	LCObjectPostV_event *event;
	event = (LCObjectPostV_event *)p_context;
	if ((p_flags & 1) == 0)
	{
		MCDispatchStatus t_status;
		s_interface -> object_dispatch(event -> object, kMCDispatchTypeCommand, event -> message, event -> argv, event -> argc, &t_status);
	}
	
	s_interface -> object_release(event -> object);
	free(event -> message);
	LCArgumentsDestroy(event -> argv, event -> argc);
	free(event);
}

static void LCObjectPostV_perform(void *p_context)
{
	LCObjectPostV_context *context;
	context = (LCObjectPostV_context *)p_context;
	
	MCError t_error;
	t_error = kMCErrorNone;
	
	MCVariableRef *t_argv;
	uint32_t t_argc;
	t_argv = nil;
	t_argc = 0;
	if (t_error == kMCErrorNone)
		t_error = LCArgumentsCreateV(context -> signature, context -> args, t_argv, t_argc);
	
	struct LCObjectPostV_event *t_event;
	t_event = nil;
	if (t_error == kMCErrorNone)
	{
		t_event = (LCObjectPostV_event *)calloc(1, sizeof(LCObjectPostV_event));
		if (t_event == nil)
			t_error = kMCErrorOutOfMemory;
	}
	
	if (t_error == kMCErrorNone)
	{
		t_error = s_interface -> object_retain(context -> object);
		if (t_error == kMCErrorNone)
			t_event -> object = context -> object;
	}
	
	if (t_error == kMCErrorNone)
	{
		t_event -> message = strdup(context -> message);
		if (t_event -> message == nil)
			t_error = kMCErrorOutOfMemory;
	}
	
	if (t_error == kMCErrorNone)
	{
		t_event -> argc = t_argc;
		t_event -> argv = t_argv;
		t_error = s_interface -> engine_run_on_main_thread((void *)LCObjectPostV_dispatch, t_event, kMCRunOnMainThreadPost | kMCRunOnMainThreadRequired | kMCRunOnMainThreadSafe | kMCRunOnMainThreadDeferred);
	}
	
	if (t_error != kMCErrorNone)
	{
		if (t_event -> object != nil)
			s_interface -> object_release(t_event -> object);
		free(t_event -> message);
		LCArgumentsDestroy(t_event -> argv, t_event -> argc);
		free(t_event);
	}
	
	context -> result = (LCError)t_error;
}

LCError LCObjectPostV(LCObjectRef p_object, const char *p_message, const char *p_signature, va_list p_args)
{
	LCObjectPostV_context t_context;
	t_context . object = (MCObjectRef)p_object;
	t_context . message = p_message;
	t_context . signature = p_signature;
	t_context . args = p_args;
	s_interface -> engine_run_on_main_thread((void *)LCObjectPostV_perform, &t_context, kMCRunOnMainThreadSend | kMCRunOnMainThreadOptional | kMCRunOnMainThreadUnsafe | kMCRunOnMainThreadImmediate);
	return t_context . result;
}

LCError LCObjectPost(LCObjectRef p_object, const char *p_message, const char *p_signature, ...)
{
	LCError t_error;
	
	va_list t_args;
	va_start(t_args, p_signature);
	t_error = LCObjectPostV(p_object, p_message, p_signature, t_args);
	va_end(t_args);
	
	return t_error;
}

LCError LCObjectSendV(LCObjectRef p_object, const char *p_message, const char *p_signature, va_list p_args)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	MCVariableRef *t_argv;
	uint32_t t_argc;
	t_argv = nil;
	t_argc = 0;
	if (t_error == kMCErrorNone)
		t_error = LCArgumentsCreateV(p_signature, p_args, t_argv, t_argc);
	
	if (t_error == kMCErrorNone)
	{
		MCDispatchStatus t_status;
		t_error = s_interface -> object_dispatch((MCObjectRef)p_object, kMCDispatchTypeCommand, p_message, t_argv, t_argc, &t_status);
		if (t_error == kMCErrorNone)
		{
			switch(t_status)
			{
				case kMCDispatchStatusError:
					t_error = kMCErrorFailed;
					break;
				case kMCDispatchStatusExit:
					t_error = kMCErrorExited;
					break;
				case kMCDispatchStatusAbort:
					t_error = kMCErrorAborted;
					break;
				default:
					break;
			}
		}
	}
	
	LCArgumentsDestroy(t_argv, t_argc);
	
	return (LCError)t_error;
	
}

LCError LCObjectSend(LCObjectRef p_object, const char *p_message, const char *p_signature, ...)
{
	LCError t_error;
	
	va_list t_args;
	va_start(t_args, p_signature);
	t_error = LCObjectSendV(p_object, p_message, p_signature, t_args);
	va_end(t_args);
	
	return t_error;
}

LCError LCObjectGet(LCObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, void *r_value)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
		t_error = MCVariableCreate(&t_var);
	
	if (t_error == kMCErrorNone)
		t_error = s_interface -> object_get((MCObjectRef)p_object, p_options, p_name, p_key, t_var);
	
	if (t_error == kMCErrorNone)
		t_error = LCValueFetch(t_var, p_options, r_value);
	
	if (t_var != nil)
		MCVariableRelease(t_var);
	
	return (LCError)t_error;
}

LCError LCObjectSet(LCObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, void *p_value)
{
	MCError t_error;
	t_error = kMCErrorNone;
		
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kMCErrorNone)
		t_error = MCVariableCreate(&t_var);
	
	if (t_error == kMCErrorNone)
		t_error = LCValueStore(t_var, p_options, p_value);
	
	if (t_error == kMCErrorNone)
		t_error = s_interface -> object_set((MCObjectRef)p_object, p_options, p_name, p_key, t_var);
	
	if (t_var != nil)
		MCVariableRelease(t_var);
	
	return (LCError)t_error;
}

/////////

struct __LCWait
{
	unsigned int references;
	unsigned int options;
	bool running;
	bool broken;
	pthread_mutex_t lock;
};

LCError LCWaitCreate(unsigned int p_options, LCWaitRef* r_wait)
{		
	LCWaitRef t_wait;
	t_wait = (LCWaitRef)malloc(sizeof(struct __LCWait));
	if (t_wait == nil)
		return (LCError)kMCErrorOutOfMemory;
	
	t_wait -> references = 1;
	t_wait -> options = p_options;
	t_wait -> running = false;
	t_wait -> broken = false;
	pthread_mutex_init(&t_wait -> lock, nil);
	
	*r_wait = t_wait;
	
	return (LCError)kMCErrorNone;
}

static void LCWaitDestroy(LCWaitRef p_wait)
{
	pthread_mutex_destroy(&p_wait -> lock);
	free(p_wait);
}

LCError LCWaitRetain(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	pthread_mutex_lock(&p_wait -> lock);
	p_wait -> references += 1;
	pthread_mutex_unlock(&p_wait -> lock);
	
	return kLCErrorNone;
}

LCError LCWaitRelease(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;

	pthread_mutex_lock(&p_wait -> lock);
	p_wait -> references -= 1;
	pthread_mutex_unlock(&p_wait -> lock);
	
	if (p_wait -> references == 0)
		LCWaitDestroy(p_wait);
	
	return kLCErrorNone;
}

LCError LCWaitIsRunning(LCWaitRef p_wait, bool *r_running)
{
	if (p_wait == nil)
		return kLCErrorNoWait;

	pthread_mutex_lock(&p_wait -> lock);
	*r_running = p_wait -> running;
	pthread_mutex_unlock(&p_wait -> lock);
	
	return kLCErrorNone;
}	

LCError LCWaitRun(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	if (p_wait -> running)
		return kLCErrorWaitRunning;
	
	pthread_mutex_lock(&p_wait -> lock);
	
	p_wait -> running = true;

	MCError t_error;
	t_error = kMCErrorNone;
	for(;;)
	{
		if (p_wait -> broken)
			break;
		
		pthread_mutex_unlock(&p_wait -> lock);
		
		t_error = s_interface -> wait_run(nil, p_wait -> options & kLCWaitOptionDispatching);
		
		pthread_mutex_lock(&p_wait -> lock);
		
		if (t_error != kMCErrorNone)
			break;
	}	

	p_wait -> running = false;
	
	pthread_mutex_unlock(&p_wait -> lock);
	
	return (LCError)t_error;
}

static void LCWaitBreak_perform(void *context)
{
	s_interface -> wait_break(nil, 0);
}
	
LCError LCWaitBreak(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	pthread_mutex_lock(&p_wait -> lock);
	if (p_wait -> running && !p_wait -> broken)
	{
		p_wait -> broken = true;
		s_interface -> wait_break(nil, 0);
	}
	pthread_mutex_unlock(&p_wait -> lock);
	
	return kLCErrorNone;
}

LCError LCWaitReset(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
		
	if (p_wait -> running)
		return kLCErrorWaitRunning;
	
	pthread_mutex_lock(&p_wait -> lock);
	p_wait -> broken = false;
	pthread_mutex_unlock(&p_wait -> lock);
	
	return (LCError)kMCErrorNone;
}
	
/////////

struct Mobile_Bitmap
{
	int width;
	int height;
	int stride;
	bool is_mono;
	void *data;
};

static LCError alloc_bitmap(int p_width, int p_height, bool p_is_mask, void*& r_bits, int& r_stride, int& r_handle)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	Mobile_Bitmap *t_bitmap;
	t_bitmap = nil;
	if (t_error == kLCErrorNone)
	{
		t_bitmap = (Mobile_Bitmap *)calloc(1, sizeof(Mobile_Bitmap));
		if (t_bitmap == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
	{
		t_bitmap -> width = p_width;
		t_bitmap -> height = p_height;
		t_bitmap -> stride = p_is_mask ? ((p_width + 31) & ~31) / 8 : p_width * 4;
		t_bitmap -> is_mono = p_is_mask;
	}
	
	if (t_error == kLCErrorNone)
	{
		t_bitmap -> data = calloc(1, t_bitmap -> stride * t_bitmap -> height);
		if (t_bitmap -> data == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
	{
		r_bits = t_bitmap -> data;
		r_stride = t_bitmap -> stride;
		r_handle = (int)t_bitmap;
	}
	else
	{
		if (t_bitmap != nil)
			free(t_bitmap -> data);
		free(t_bitmap);
	}
	
	return t_error;
}

static void free_bitmap(int handle)
{
	if (handle == 0)
		return;

	free(((Mobile_Bitmap *)handle) -> data);
	free((Mobile_Bitmap *)handle);
}

struct __LCImage
{
	LCObjectRef object;
	unsigned int options;
	
	int width;
	int height;
	
	void *bitmap_data;
	int bitmap_stride;
	int bitmap_handle;
	
	void *mask_bitmap_data;
	int mask_bitmap_stride;
	int mask_bitmap_handle;
};

LCError LCImageAttach(LCObjectRef p_object, unsigned int p_options, LCImageRef *r_image)
{		
	if (p_object == nil)
		return kLCErrorNoObject;
	
	LCError t_error;
	t_error = kLCErrorNone;
	
	char *t_name;
	if (t_error == kLCErrorNone)
		t_error = LCObjectGet(p_object, kLCValueOptionAsCString, "name", nil, &t_name);
	
	if (t_error == kLCErrorNone && strncmp(t_name, "image ", 6) != 0)
		t_error = kLCErrorNotAnImageObject;
	
	int t_width, t_height;
	if (t_error == kLCErrorNone)
		t_error = LCObjectGet(p_object, kLCValueOptionAsInteger, "width", nil, &t_width);
	if (t_error == kLCErrorNone)
		t_error = LCObjectGet(p_object, kLCValueOptionAsInteger, "height", nil, &t_height);
	
	struct __LCImage *t_image;
	t_image = nil;
	if (t_error == kLCErrorNone)
	{
		t_image = (struct __LCImage *)calloc(1, sizeof(struct __LCImage));
		if (t_image == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
		t_error = alloc_bitmap(t_width, t_height, false, t_image -> bitmap_data, t_image -> bitmap_stride, t_image -> bitmap_handle);
	
	if (t_error == kLCErrorNone && (p_options & kLCImageOptionMasked) != 0)
		t_error = alloc_bitmap(t_width, t_height, true, t_image -> mask_bitmap_data, t_image -> mask_bitmap_stride, t_image -> mask_bitmap_handle);

	bool t_set_image_pixmap;
	t_set_image_pixmap = false;
	if (t_error == kLCErrorNone)
	{
		t_error = LCObjectSet(p_object, kLCValueOptionAsInteger, "imagePixmapId", nil, &t_image -> bitmap_handle);
		if (t_error == kLCErrorNone)
			t_set_image_pixmap = true;
	}
	
	bool t_set_mask_pixmap;
	t_set_mask_pixmap = false;
	if (t_error == kLCErrorNone && (p_options & kLCImageOptionMasked) != 0)
	{
		t_error = LCObjectSet(p_object, kLCValueOptionAsInteger, "maskPixmapId", nil, &t_image -> mask_bitmap_handle);
		if (t_error == kLCErrorNone)
			t_set_mask_pixmap = true;
	}
	
	if (t_error == kLCErrorNone)
	{
		t_image -> width = t_width;
		t_image -> height = t_height;
		t_image -> options = p_options;
		t_image -> object = p_object;
		LCObjectRetain(t_image -> object);
		*r_image = t_image;
	}
	else
	{
		const char *t_nil;
		t_nil = "";
		if (t_set_mask_pixmap)
			LCObjectSet(p_object, kLCValueOptionAsCString, "maskPixmapId", nil, &t_nil);
		
		if (t_set_image_pixmap)
			LCObjectSet(p_object, kLCValueOptionAsCString, "imagePixmapId", nil, &t_nil);
		
		if (t_image != nil)
		{
			free_bitmap(t_image -> mask_bitmap_handle);
			free_bitmap(t_image -> bitmap_handle);
			free(t_image);
		}
	}
	
	free(t_name);
	
	return t_error;
}

LCError LCImageDetach(LCImageRef p_image)
{
	if (p_image == nil)
		return kLCErrorNoImage;
	
	const char *t_nil;
	t_nil = "";
	if (p_image -> mask_bitmap_handle != 0)
	{
		LCObjectSet(p_image -> object, kLCValueOptionAsCString, "maskPixmapId", nil, &t_nil);
		free_bitmap(p_image -> mask_bitmap_handle);
	}
		
	LCObjectSet(p_image -> object, kLCValueOptionAsCString, "imagePixmapId", nil, &t_nil);
	free_bitmap(p_image -> bitmap_handle);
	
	LCObjectRelease(p_image -> object);
	
	free(p_image);
		
	return kLCErrorNone;
}

LCError LCImageDescribe(LCImageRef p_image, LCImageRaster *r_raster)
{
	if (p_image == nil)
		return kLCErrorNoImage;
	
	r_raster -> format = kLCImageRasterFormat_RGBx_8888;
	r_raster -> width = p_image -> width;
	r_raster -> height = p_image -> height;
	r_raster -> stride = p_image -> bitmap_stride;
	r_raster -> data = p_image -> bitmap_data;
	
	return kLCErrorNone;
}

LCError LCImageDescribeMask(LCImageRef p_image, LCImageRaster *r_raster)
{
	if (p_image == nil)
		return kLCErrorNoImage;
	
	if (p_image -> mask_bitmap_handle == 0)
		return kLCErrorMaskNotAttached;
	
	r_raster -> format = kLCImageRasterFormat_G_1;
	r_raster -> width = p_image -> width;
	r_raster -> height = p_image -> height;
	r_raster -> stride = p_image -> mask_bitmap_stride;
	r_raster -> data = p_image -> mask_bitmap_data;
	
	return kLCErrorNone;
}

LCError LCImageUpdate(LCImageRef p_image)
{
	if (p_image == nil)
		return kLCErrorNoImage;
	
	return (LCError)s_interface -> object_update((MCObjectRef)p_image -> object, 0, nil);
}

LCError LCImageUpdateRect(LCImageRef p_image, int p_left, int p_top, int p_right, int p_bottom)
{
	if (p_image == nil)
		return kLCErrorNoImage;
	
	int t_rect[4];
	t_rect[0] = p_left;
	t_rect[1] = p_top;
	t_rect[2] = p_right;
	t_rect[3] = p_bottom;
	
	return (LCError)s_interface -> object_update((MCObjectRef)p_image -> object, kMCExternalObjectUpdateOptionRect, t_rect);
}

/////////

LCError LCRunOnSystemThread(LCRunOnSystemThreadCallback p_callback, void *p_context)
{
	if (s_interface -> version >= 4)
		return (LCError)s_interface -> engine_run_on_main_thread((void *)p_callback, p_context, kMCRunOnMainThreadJumpToUI);
	
	p_callback(p_context);
	
	return kLCErrorNone;
}

LCError LCRunOnMainThread(unsigned int p_options, LCRunOnMainThreadCallback p_callback, void *p_context)
{
	return (LCError)s_interface -> engine_run_on_main_thread((void *)p_callback, p_context, p_options | kMCRunOnMainThreadOptional | kMCRunOnMainThreadUnsafe | kMCRunOnMainThreadImmediate);
}

LCError LCPostOnMainThread(unsigned int p_options, LCPostOnMainThreadCallback p_callback, void *p_context)
{
	return (LCError)s_interface -> engine_run_on_main_thread((void *)p_callback, p_context, kMCRunOnMainThreadPost | kMCRunOnMainThreadRequired | kMCRunOnMainThreadSafe | kMCRunOnMainThreadDeferred);
}

#ifdef __OBJC__
static void LCRunBlockOnSystemThreadCallback(void *p_context)
{
	void (^t_callback)(void) = (void (^)(void))p_context;
	t_callback();
}

LCError LCRunBlockOnSystemThread(void (^p_callback)(void))
{
	if (s_interface -> version >= 4)
		return (LCError)s_interface -> engine_run_on_main_thread((void *)LCRunBlockOnSystemThreadCallback, p_callback, kMCRunOnMainThreadJumpToUI);
		
	p_callback();
	
	return kLCErrorNone;
}
#endif
	
/////////

#if TARGET_OS_IPHONE
	
#import <UIKit/UIKit.h>

LCError LCInterfaceQueryView(UIView** r_view)
{
	return (LCError)s_interface -> interface_query(kMCExternalInterfaceQueryView, r_view);
}

LCError LCInterfaceQueryViewScale(double* r_scale)
{
	return (LCError)s_interface -> interface_query(kMCExternalInterfaceQueryViewScale, r_scale);
}

LCError LCInterfaceQueryViewController(UIViewController** r_controller)
{
	return (LCError)s_interface -> interface_query(kMCExternalInterfaceQueryViewController, r_controller);
}
	
LCError LCInterfacePresentModalViewController(UIViewController *p_controller, bool p_animated)
{
	UIViewController *t_controller;
	LCInterfaceQueryViewController(&t_controller);
	if (s_interface -> version < 4)
		[t_controller presentModalViewController: p_controller animated: p_animated ? YES : NO];
	else
	{
		LCRunBlockOnSystemThread(^(void) {
			[t_controller presentModalViewController: p_controller animated: p_animated ? YES : NO];
		});
	}
	return kLCErrorNone;
}

static void perform_DismissBreak(CFRunLoopTimerRef p_timer, void *p_info)
{
	s_interface -> wait_break(nil, 0);
}

LCError LCInterfaceDismissModalViewController(UIViewController *p_controller, bool p_animated)
{
	UIViewController *t_controller;
	LCInterfaceQueryViewController(&t_controller);

	if (s_interface -> version < 4)
		[p_controller dismissModalViewControllerAnimated: p_animated ? YES : NO];
	else
	{
		LCRunBlockOnSystemThread(^(void) {
			[p_controller dismissModalViewControllerAnimated: p_animated ? YES : NO];
		});
	}
	
	CFRunLoopTimerContext t_timer_context;
	t_timer_context . version = 0;
	t_timer_context . info = p_controller;
	t_timer_context . retain = NULL;
	t_timer_context . release = NULL;
	t_timer_context . copyDescription = NULL;

	CFRunLoopTimerRef t_timer;
	t_timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + 0.01, 0.01, 0, 0, perform_DismissBreak, &t_timer_context);
	CFRunLoopAddTimer(CFRunLoopGetMain(), t_timer, kCFRunLoopCommonModes);
	for(;;)
	{
		s_interface -> wait_run(nil, 0);
		if ([p_controller parentViewController] == nil)
			break;
	}
	
	CFRunLoopRemoveTimer(CFRunLoopGetMain(), t_timer, kCFRunLoopCommonModes);
	CFRunLoopTimerInvalidate(t_timer);
	CFRelease(t_timer);

	return kLCErrorNone;
}
	
#endif

END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////

static char *s_error = nil;

extern "C"
{

void LCExceptionRaise(const char *p_format, ...)
{
	va_list args;
	int length;
	va_start(args, p_format);
	length = vsnprintf(nil, 0, p_format, args);
	va_end(args);
	
	free(s_error);
	s_error = (char *)malloc(length);
	if (s_error == nil)
		return;
	
	va_start(args, p_format);
	vsprintf(s_error, p_format, args);
	va_end(args);
	
	return;
}

}

static void error__clear(void)
{
	free(s_error);
	s_error = nil;
}

static bool error__catch(void)
{
	return s_error == nil;
}

static bool error__raise(const char *message)
{
	free(s_error);
	s_error = strdup(message);
	return false;
}

static bool error__format(const char *format, ...)
{
	va_list args;
	int length;
	va_start(args, format);
	length = vsnprintf(nil, 0, format, args);
	va_end(args);
	
	free(s_error);
	s_error = (char *)malloc(length);
	if (s_error == nil)
		return false;
	
	va_start(args, format);
	vsprintf(s_error, format, args);
	va_end(args);
	
	return false;
}

static bool error__unknown(void)
{
	error__raise("uncaught c++ exception");
	return false;
}

static bool error__out_of_memory(void)
{
	error__format("out of memory");
	return false;
}

static bool error__bad_enum_element(const char *arg, const char *element)
{
	error__format("unknown enum element passed to parameter '%s'", arg);
	return false;
}

static bool error__bad_out_parameter(const char *arg)
{
	error__format("parameter '%s' is not a variable", arg);
	return false;
}

static bool error__bad_string_conversion(void)
{
	error__format("string conversion failed");
	return false;
}

static bool error__bad_api_call(MCError err)
{
	error__format("internal api error (%d)", err);
	return false;
}

static bool error__report(MCVariableRef result)
{
	const char *error;
	if (s_error != nil)
		error = s_error;
	else
		error = "unknown error";
	MCVariableStore(result, kMCOptionAsCString, &error);
	free(s_error);
	s_error = nil;
	return false;
}

static bool error__report_bad_parameter_count(MCVariableRef result)
{
	const char *msg = "invalid number of parameters";
	MCVariableStore(result, kMCOptionAsCString, &msg);
	return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool default__cstring(const char *arg, char*& r_value)
{
	char *t_arg_copy;
	t_arg_copy = strdup(arg);
	if (r_value != nil)
	{
		r_value = t_arg_copy;
		return true;
	}
	return error__out_of_memory();
}

////////////////////////////////////////////////////////////////////////////////

static bool fetch__bool(const char *arg, MCVariableRef var, bool& r_value)
{
	MCError err;
	err = MCVariableFetch(var, kMCOptionAsBoolean, &r_value);
	if (err == kMCErrorNone)
		return true;
	if (err == kMCErrorNotABoolean)
		return error__format("parameter '%s' is not a boolean", arg);
	return error__bad_api_call(err);
}

static bool fetch__cstring(const char *arg, MCVariableRef var, char*& r_value)
{
	MCError err;
	err = MCVariableFetch(var, kMCOptionAsCString, &r_value);
	if (err == kMCErrorNone)
	{
		r_value = strdup(r_value);
		if (r_value != nil)
			return true;
		return error__out_of_memory();
	}
	if (err == kMCErrorNotACString)
		return error__format("parameter '%s' is not a string", arg);
	return error__bad_api_call(err);
}

static bool fetch__cdata(const char *arg, MCVariableRef var, LCBytes& r_value)
{
	MCError err;
	err = MCVariableFetch(var, kMCOptionAsString, &r_value);
	if (err == kMCErrorNone)
	{
		if (r_value . length == 0)
			return true;
		
		void *buffer;
		buffer = malloc(r_value . length);
		if (buffer != nil)
		{
			memcpy(buffer, r_value . buffer, r_value . length);
			r_value . buffer = buffer;
			return true;
		}
		return error__out_of_memory();
	}
	if (err == kMCErrorNotAString)
		return error__format("parameter '%s' is not a binary string", arg);
	return error__bad_api_call(err);
}

static bool fetch__int(const char *arg, MCVariableRef var, int& r_value)
{
	MCError err;
	err = MCVariableFetch(var, kMCOptionAsInteger, &r_value);
	if (err == kMCErrorNone)
		return true;
	if (err == kMCErrorNotAnInteger || err == kMCErrorNotANumber)
		return error__format("parameter '%s' is not an integer", arg);
	if (err == kMCErrorNumericOverflow)
		return error__format("parameter '%s' is too big", arg);
	return error__bad_api_call(err);
}

static bool fetch__double(const char *arg, MCVariableRef var, double& r_value)
{
	MCError err;
	err = MCVariableFetch(var, kMCOptionAsReal, &r_value);
	if (err == kMCErrorNone)
		return true;
	if (err == kMCErrorNotANumber)
		return error__format("parameter '%s' is not a number", arg);
	if (err == kMCErrorNumericOverflow)
		return error__format("parameter '%s' is too big", arg);
	return error__bad_api_call(err);
}

#ifdef __OBJC__
static bool fetch__objc_string(const char *arg, MCVariableRef var, NSString*& r_string)
{
	MCError err;
	char *cstring;
	err = MCVariableFetch(var, kMCOptionAsCString, &cstring);
	if (err == kMCErrorNone)
	{
		r_string = [NSString stringWithCString: cstring encoding: NSMacOSRomanStringEncoding];
		return true;
	}
	if (err == kMCErrorNotACString)
		return error__format("parameter '%s' is not a string", arg);
	return error__bad_api_call(err);
}

static bool fetch__objc_number(const char *arg, MCVariableRef var, NSNumber*& r_number)
{
	double real;
	if (!fetch__double(arg, var, real))
		return false;
	r_number = [NSNumber numberWithDouble: real];
	return true;
}

static bool fetch__objc_data(const char *arg, MCVariableRef var, NSData*& r_data)
{
	MCError err;
	LCBytes data;
	err = MCVariableFetch(var, kMCOptionAsString, &data);
	if (err == kMCErrorNone)
	{
		r_data = [NSData dataWithBytes: data . buffer length: data . length];
		return true;
	}
	if (err == kMCErrorNotAString)
		return error__format("parameter '%s' is not a binary string", arg);
	return error__bad_api_call(err);
}
#endif

#ifdef __NOT_YET_FINISHED__

static bool fetch__lc_array(const char *arg, MCVariableRef var, LCArrayRef& r_array)
{
	MCError err;
	bool t_is_array;
	err = MCVariableIsAnArray(var, &t_is_array);
	if (err == kMCErrorNone && !t_is_array)
		return error__format("parameter '%s' is not an array", arg);
	
	MCVariableRef array;
	array = nil;
	if (err == kMCErrorNone)
		err = MCVariableCreate(&array);
	if (err == kMCErrorNone)
		err = MCVariableStore(array, kMCOptionAsVariable, var);
	if (err == kMCErrorNone)
	{
		r_array = (LCArrayRef)array;
		return true;
	}
	if (array != nil)
		MCVariableRelease(array);
	return error__bad_api_call(err);
}

#endif

////////////////////////////////////////////////////////////////////////////////

static bool store__int(MCVariableRef var, int value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsInteger, &value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__double(MCVariableRef var, double value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsReal, &value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__bool(MCVariableRef var, bool value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsBoolean, &value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__cstring(MCVariableRef var, const char* value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsCString, &value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__cdata(MCVariableRef var, LCBytes value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsString, (void *)&value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

#ifdef __OBJC__
static bool store__objc_string(MCVariableRef var, NSString* value)
{
	const char *cstring;
	cstring = [value cStringUsingEncoding: NSMacOSRomanStringEncoding];
	if (cstring == nil)
		return error__bad_string_conversion();
	MCError err;
	err = MCVariableStore(var, kMCOptionAsCString, &cstring);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__objc_number(MCVariableRef var, NSNumber* value)
{
	double real;
	real = [value doubleValue];
	MCError err;
	err = MCVariableStore(var, kMCOptionAsReal, &real);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

static bool store__objc_data(MCVariableRef var, NSData* value)
{
	LCBytes data;
	data . buffer = (char *)[value bytes];
	data . length = [value length];
	MCError err;
	err = MCVariableStore(var, kMCOptionAsString, &data);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}
#endif

#ifdef __NOT_YET_FINISHED__

static bool store__lc_array(MCVariableRef var, LCArrayRef value)
{
	MCError err;
	err = MCVariableStore(var, kMCOptionAsVariable, (void *)value);
	if (err == kMCErrorNone)
		return true;
	return error__bad_api_call(err);
}

#endif

////////////////////////////////////////////////////////////////////////////////

static bool verify__out_parameter(const char *arg, MCVariableRef var)
{
	MCError err;
	bool transient;
	err = MCVariableIsTransient(var, &transient);
	if (err == kMCErrorNone)
	{
		if (!transient)
			return true;
		else
			return error__bad_out_parameter(arg);
	}
	return error__bad_api_call(err);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __ANDROID__

#include <jni.h>
#include <android/log.h>

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) __attribute__((visibility("default")));

static JavaVM *s_java_vm = nil;
static JNIEnv *s_java_env = nil;
static jclass s_java_class = nil;

//////////

static jobject java__get_activity(void)
{
	jobject t_activity;
	s_interface -> interface_query(kMCExternalInterfaceQueryActivity, &t_activity);
	return t_activity;
}

static jobject java__get_container(void)
{
	jobject t_activity;
	s_interface -> interface_query(kMCExternalInterfaceQueryContainer, &t_activity);
	return t_activity;
}

//////////

static bool java_from__bool(bool p_value)
{
	return p_value;
}

static jobject java_from__cstring(const char *p_value)
{
	size_t t_char_count;
	t_char_count = strlen(p_value);
	
	jchar *t_chars;
	t_chars = (jchar *)malloc(sizeof(jchar) * t_char_count);
	for(uint32_t i = 0; i < t_char_count; i++)
		t_chars[i] = p_value[i];
	
	jobject t_java_value;
	t_java_value = (jobject)s_java_env -> NewString(t_chars, t_char_count);
	
	return t_java_value;
}

static jobject java_from__cdata(LCBytes p_value)
{
	jobject t_java_value;
	t_java_value = (jobject)s_java_env -> NewByteArray(p_value . length);
	s_java_env -> SetByteArrayRegion((jbyteArray)t_java_value, 0, p_value . length, (const jbyte *)p_value . buffer);
	return t_java_value;
}

static int java_from__int(int p_value)
{
	return p_value;
}

static double java_from__double(double p_value)
{
	return p_value;
}

//////////

static bool java_to__bool(bool p_value)
{
	return p_value;
}

static char *java_to__cstring(jobject p_value)
{
	const jchar *t_unichars;
	uint32_t t_unichar_count;
	t_unichars = s_java_env -> GetStringChars((jstring)p_value, nil);
	t_unichar_count = s_java_env -> GetStringLength((jstring)p_value);
	
	char *t_native_value;
	t_native_value = (char *)malloc(t_unichar_count + 1);
	for(uint32_t i = 0; i < t_unichar_count; i++)
		t_native_value[i] = t_unichars[i] < 256 ? t_unichars[i] : '?';
	t_native_value[t_unichar_count] = 0;
	
	s_java_env -> ReleaseStringChars((jstring)p_value, t_unichars);
	
	return t_native_value;
}

static LCBytes java_to__cdata(jobject p_value)
{
	LCBytes t_native_value;
	t_native_value . length = s_java_env -> GetArrayLength((jbyteArray)p_value);
	t_native_value . buffer = malloc(t_native_value . length);
	jbyte *t_bytes;
	t_bytes = s_java_env -> GetByteArrayElements((jbyteArray)p_value, nil);
	memcpy(t_native_value . buffer, t_bytes, t_native_value . length);
	s_java_env -> ReleaseByteArrayElements((jbyteArray)p_value, t_bytes, 0);
	return t_native_value;
}

static int java_to__int(int p_value)
{
	return p_value;
}

static double java_to__double(double p_value)
{
	return p_value;
}

//////////

static void java_free__bool(bool p_value)
{
}

static void java_free__cstring(jobject p_value)
{
	s_java_env -> DeleteLocalRef(p_value);
}

static void java_free__cdata(jobject p_value)
{
	s_java_env -> DeleteLocalRef(p_value);
}

static void java_free__int(int p_value)
{
}

static void java_free__double(double p_value)
{
}

//////////

static void native_free__bool(bool p_value)
{
}

static void native_free__cstring(char *p_value)
{
	free(p_value);
}

static void native_free__cdata(LCBytes p_value)
{
	free(p_value . buffer);
}

static void native_free__int(int p_value)
{
}

static void native_free__double(double p_value)
{
}

#endif

////////////////////////////////////////////////////////////////////////////////
