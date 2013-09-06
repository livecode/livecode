/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif

#include "LiveCode.h"

////////////////////////////////////////////////////////////////////////////////

#undef __WINDOWS__
#undef __LINUX__
#undef __MAC__
#undef __IOS__
#undef __ANDROID__

#if defined(_MSC_VER) && !defined(WINCE)
#define __WINDOWS__ 1
#elif defined(__GNUC__) && defined(__APPLE__) && defined(TARGET_OS_MAC) && !defined(TARGET_OS_IPHONE)
#define __MAC__ 1
#elif defined(__GNUC__) && !defined(__APPLE__) && !defined(ANDROID)
#define __LINUX__ 1
#elif defined(__GNUC__) && defined(__APPLE__) && defined(TARGET_OS_MAC) && defined(TARGET_OS_IPHONE)
#define __IOS__ 1
#elif defined(ANDROID)
#define __ANDROID__ 1
#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __WINDOWS__
#include <windows.h>
typedef unsigned int uint32_t;
#else
#include <pthread.h>
#include <stdint.h>
#endif

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
	kMCExternalContextVarDefaultCard = 13,
	kMCExternalContextVarWholeMatches = 14,
} MCExternalContextVar;

typedef enum MCExternalVariableQuery
{
	kMCExternalVariableQueryIsTemporary = 1,
	kMCExternalVariableQueryIsTransient = 2,
	kMCExternalVariableQueryFormat = 3,
	kMCExternalVariableQueryRetention = 4,
	kMCExternalVariableQueryIsAnArray = 5,
	kMCExternalVariableQueryIsASequence = 6,
	kMCExternalVariableQueryIsEmpty = 7,
} MCExternalVariableQuery;

typedef enum MCExternalInterfaceQuery
{
	kMCExternalInterfaceQueryView = 1,
	kMCExternalInterfaceQueryViewScale = 2,
	kMCExternalInterfaceQueryViewController = 3,
	kMCExternalInterfaceQueryActivity = 4,
	kMCExternalInterfaceQueryContainer = 5,
	kMCExternalInterfaceQueryScriptJavaEnv = 6,
	kMCExternalInterfaceQuerySystemJavaEnv = 7,
	kMCExternalInterfaceQueryEngine = 8,
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
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] New context methods for script execution.
	MCError (*context_evaluate)(const char *p_expression, unsigned int options, MCVariableRef *binds, unsigned int bind_count, MCVariableRef result);
	MCError (*context_execute)(const char *p_expression, unsigned int options, MCVariableRef *binds, unsigned int bind_count);
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

static MCError MCVariableClear(MCVariableRef var)
{
	return s_interface -> variable_clear(var);
}

static MCError MCVariableIsTransient(MCVariableRef var, bool *r_transient)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsTransient, r_transient);
}

static MCError MCVariableIsEmpty(MCVariableRef var, bool *r_empty)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsEmpty, r_empty);
}

static MCError MCVariableIsAnArray(MCVariableRef var, bool *r_array)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsAnArray, r_array);
}

static MCError MCVariableIsASequence(MCVariableRef var, bool *r_sequence)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsASequence, r_sequence);
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

// MW-2013-06-14: [[ ExternalsApiV5 ]] New methods to convert to/from an internal
//   variable.
static LCError LCValueFetch(MCVariableRef var, unsigned int options, void *r_value);
static LCError LCValueStore(MCVariableRef var, unsigned int options, void *r_value);

#ifdef __OBJC__

// MW-2013-06-14: [[ ExternalsApiV5 ]] New methods to convert to/from objc-arrays
//   and dictionaries.
static LCError LCValueArrayToObjcArray(MCVariableRef src, NSArray*& r_dst);
static LCError LCValueArrayFromObjcArray(MCVariableRef src, NSArray* r_dst);
static LCError LCValueArrayToObjcDictionary(MCVariableRef src, NSDictionary*& r_dst);
static LCError LCValueArrayFromObjcDictionary(MCVariableRef src, NSDictionary* r_dst);

// Convert a LiveCode value into an element of an Objc Array/Dictionary. This
// converts all non-array values to strings, arrays which are sequences to
// NSArray, and arrays which are maps to NSDictionary.
static LCError LCValueArrayValueToObjcValue(MCVariableRef var, id& r_dst)
{
	MCError t_error;
	t_error = kMCErrorNone;
	
	if (t_error == kMCErrorNone)
	{
		bool t_is_empty;
		t_error = MCVariableIsEmpty(var, &t_is_empty);
		if (t_error == kMCErrorNone && t_is_empty)
		{
			r_dst = @"";
			return kLCErrorNone;
		}
	}
	
	if (t_error == kMCErrorNone)
	{
		bool t_is_array;
		t_error = MCVariableIsAnArray(var, &t_is_array);
		if (t_error == kMCErrorNone && t_is_array)
			return LCValueFetch(var, kLCValueOptionAsObjcString, &r_dst);
	}
	
	if (t_error == kMCErrorNone)
	{
		bool t_is_sequence;
		t_error = MCVariableIsASequence(var, &t_is_sequence);
		if (t_error == kMCErrorNone && t_is_sequence)
			return LCValueArrayToObjcArray(var, (NSArray*&)r_dst);
	}
	
	if (t_error == kMCErrorNone)
		return LCValueArrayToObjcDictionary(var, (NSDictionary*&)r_dst);
	
	return (LCError)t_error;
}

static LCError LCValueArrayValueFromObjcValue(MCVariableRef var, id src)
{
	if ([src isKindOfClass: [NSNull class]])
		return (LCError)MCVariableClear(var);
	
	if ((CFBooleanRef)src == kCFBooleanTrue || (CFBooleanRef)src == kCFBooleanFalse)
	{
		bool t_bool;
		t_bool = (CFBooleanRef)src == kCFBooleanTrue;
		return LCValueStore(var, kLCValueOptionAsBoolean, &t_bool);
	}
		
	if ([src isKindOfClass: [NSNumber class]])
		return LCValueStore(var, kLCValueOptionAsObjcNumber, &src);
		
	if ([src isKindOfClass: [NSString class]])
		return LCValueStore(var, kLCValueOptionAsObjcString, &src);
		
	if ([src isKindOfClass: [NSArray class]])
		return LCValueArrayFromObjcArray(var, (NSArray *)src);
	
	if ([src isKindOfClass: [NSDictionary class]])
		return LCValueArrayFromObjcDictionary(var, (NSDictionary *)src);
	
	NSString *t_as_string;
	t_as_string = [src description];
	return LCValueStore(var, kLCValueOptionAsObjcString, &t_as_string);
}

// Convert a LiveCode array into an NSArray. The returned NSArray is alloc'd.
static LCError LCValueArrayToObjcArray(MCVariableRef src, NSArray*& r_dst)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	if (t_error == kLCErrorNone)
	{
		bool t_is_sequence;
		t_error = (LCError)MCVariableIsASequence(src, &t_is_sequence);
		if (t_error == kLCErrorNone && !t_is_sequence)
			t_error = kLCErrorNotASequence;
	}
	
	uint32_t t_count;
	t_count = 0;
	if (t_error = kLCErrorNone)
		t_count = s_interface -> variable_count_keys(src, &t_count);
	
	id *t_objects;
	t_objects = nil;
	if (t_error == kLCErrorNone)
	{
		t_objects = (id *)calloc(sizeof(id), t_count);
		if (t_objects == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	MCVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(uint32_t i = 0; i < t_count && t_error == kLCErrorNone; i++)
	{
		// Fetch the key and value.
		const char *t_key;
		MCVariableRef t_value;
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_iterate_keys(src, &t_iterator, kMCOptionAsCString, &t_key, &t_value);
		
		// Now convert the value - remembering that LC sequences are 1 based, and
		// Objc arrays are 0 based. Note that we don't have to validate the key as
		// its guaranteed to be of the correct form as we checked the array was a
		// sequence.
		if (t_error == kLCErrorNone)
			t_error = LCValueArrayValueToObjcValue(t_value, t_objects[strtoul(t_key, nil, 10) - 1]);
	}
	
	// If we succeeded, then try to build an NSArray.
	NSArray *t_array;
	if (t_error == kLCErrorNone)
	{
		t_array = [[NSArray alloc] initWithObjects: t_objects count: t_count];
		if (t_array == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
		r_dst = t_array;
	
	// We free the objects array since its copied by NSArray.
	for(uint32_t i = 0; i < t_count; i++)
		[t_objects[i] release];
	free(t_objects);
	
	return t_error;
}

static LCError LCValueArrayFromObjcArray(MCVariableRef var, NSArray *src)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	for(unsigned int t_index = 0; t_index < [src count] && t_error == kLCErrorNone; t_index++)
	{
		char t_key[12];
		if (t_error == kLCErrorNone)
			sprintf(t_key, "%ud", t_index + 1);
		
		MCVariableRef t_value;
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_lookup_key(var, kMCOptionAsCString, t_key, true, &t_value);
		
		if (t_error == kLCErrorNone)
			t_error = LCValueArrayValueFromObjcValue(t_value, [src objectAtIndex: t_index]);
	}
	
	return t_error;
}

static LCError LCValueArrayToObjcDictionary(MCVariableRef src, NSDictionary*& r_dst)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	uint32_t t_count;
	t_count = 0;
	if (t_error = kLCErrorNone)
		t_count = s_interface -> variable_count_keys(src, &t_count);
	
	id *t_keys, *t_values;
	t_keys = t_values = nil;
	if (t_error == kLCErrorNone)
	{
		t_keys = (id *)calloc(sizeof(id), t_count);
		t_values = (id *)calloc(sizeof(id), t_count);
		if (t_keys == nil || t_values == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	MCVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(uint32_t i = 0; i < t_count && t_error == kLCErrorNone; i++)
	{
		// Fetch the key and value.
		const char *t_key;
		MCVariableRef t_value;
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_iterate_keys(src, &t_iterator, kMCOptionAsCString, &t_key, &t_value);
		
		// Convert the key.
		if (t_error == kLCErrorNone)
		{
			t_keys[i] = [[NSString alloc] initWithCString: t_key encoding: NSMacOSRomanStringEncoding];
			if (t_keys[i] == nil)
				t_error = kLCErrorOutOfMemory;
		}
		
		// Now convert the value.
		if (t_error == kLCErrorNone)
			t_error = LCValueArrayValueToObjcValue(t_value, t_values[i]);
	}
	
	// If we succeeded then build the dictionary.
	NSDictionary *t_dictionary;
	if (t_error == kLCErrorNone)
	{
		t_dictionary = [[NSDictionary alloc] initWithObjects: t_values forKeys: t_keys count: t_count];
		if (t_dictionary == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
		r_dst = t_dictionary;
	
	for(uint32_t i = 0; i < t_count; i++)
	{
		[t_keys[i] release];
		[t_values[i] release];
	}
	free(t_keys);
	free(t_values);
	
	return t_error;
}

static LCError LCValueArrayFromObjcDictionary(MCVariableRef var, NSDictionary *p_src)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
#ifndef __OBJC2__
	NSEnumerator *t_enumerator;
	t_enumerator = [p_src objectEnumerator];
	for(;;)
	{
		id t_key;
		t_key = [t_enumerator nextObject];
		if (t_key == nil)
			break;
#else
	for(id t_key in p_src)
	{
#endif
		if (t_error == kLCErrorNone && ![t_key isKindOfClass: [NSString class]])
			t_error = kLCErrorCannotEncodeMap;
		
		const char *t_key_cstring;
		if (t_error == kLCErrorNone)
		{
			t_key_cstring = [(NSString *)t_key cStringUsingEncoding: NSMacOSRomanStringEncoding];
			if (t_key_cstring == nil)
				t_error = kLCErrorCannotEncodeCString;
		}
		
		MCVariableRef t_value;
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_lookup_key(var, kMCOptionAsCString, (void *)t_key_cstring, true, &t_value);
		
		if (t_error == kLCErrorNone)
			t_error = LCValueArrayValueFromObjcValue(t_value, [p_src objectForKey: t_key]);
	}
	[t_pool release];
	
	return t_error;
}

#endif

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

static LCError LCValueFetch(MCVariableRef p_var, unsigned int p_options, void *r_value)
{
	LCError t_error;
	t_error = kLCErrorNone;

	unsigned int t_options_to_use;
	void *t_value_to_use;
	union
	{
		double t_number_value;
		LCBytes t_cdata_value;
		char *t_cstring_value;
		MCVariableRef t_array_value;
	};
	if (t_error == kLCErrorNone)
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
			case kLCValueOptionAsCData:
				t_options_to_use = kMCOptionAsString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cdata_value;
				break;
			case kLCValueOptionAsCString:
				t_options_to_use = kMCOptionAsCString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cstring_value;
				break;
			case kLCValueOptionAsCChar:
				t_options_to_use = kMCOptionAsString;
				t_options_to_use |= LCValueOptionsGetNumberFormat(p_options);
				t_value_to_use = &t_cdata_value;
				break;
				
			case kLCValueOptionAsLCArray:
				t_options_to_use = kMCOptionAsVariable;
				t_value_to_use = &t_array_value;
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
			case kLCValueOptionAsObjcArray:
			case kLCValueOptionAsObjcDictionary:
				t_options_to_use = kMCOptionAsVariable;
				t_value_to_use = &t_array_value;
				break;
#endif
				
			default:
				t_error = kLCErrorBadValueOptions;
		}
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_fetch(p_var, t_options_to_use, t_value_to_use);
	}
	
	// If the output value pointer was the one passed in, we are done.
	if (t_error == kLCErrorNone && t_value_to_use == r_value)
		return kLCErrorNone;
	
	// Otherwise we need to process the output value.
	if (t_error == kLCErrorNone)
	{
		switch(p_options & kLCValueOptionMaskAs)
		{
			case kLCValueOptionAsCString:
				t_cstring_value = strdup(t_cstring_value);
				if (t_cstring_value != nil)
					*(char **)r_value = t_cstring_value;
				else
					t_error = kLCErrorOutOfMemory;
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
					t_error = kLCErrorOutOfMemory;
			}
			break;
			case kLCValueOptionAsCChar:
			{
				if (t_cdata_value . length == 1)
					*(char *)r_value = *(char *)t_cdata_value . buffer;
				else
					t_error = kLCErrorNotAChar;
			}
			break;
			
			case kLCValueOptionAsLCArray:
			{
				// Check the value is actually an array.
				bool t_is_array;
				t_is_array = false;
				if (t_error == kLCErrorNone)
					t_error = (LCError)MCVariableIsAnArray(t_array_value, &t_is_array);
				if (t_error == kLCErrorNone && !t_is_array)
					t_error = kLCErrorNotAnArray;
				
				// Now clone the array.
				MCVariableRef t_var_copy;
				t_var_copy = nil;
				if (t_error == kLCErrorNone)
					t_error = (LCError)MCVariableCreate(&t_var_copy);
				if (t_error == kLCErrorNone)
					t_error = (LCError)MCVariableStore(t_var_copy, kMCOptionAsVariable, t_array_value);
				if (t_error == kLCErrorNone)
					*(LCArrayRef *)r_value = (LCArrayRef)t_array_value;
				else
				{
					// Make sure we free the array if there's a problem.
					if (t_var_copy != nil)
						MCVariableRelease(t_var_copy);
				}
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
			case kLCValueOptionAsObjcArray:
				t_error = LCValueArrayToObjcArray(t_array_value, *(NSArray **)r_value);
				if (t_error == kLCErrorNone)
					[*(NSArray **)r_value autorelease];
				break;
			case kLCValueOptionAsObjcDictionary:
				t_error = LCValueArrayToObjcDictionary(t_array_value, *(NSDictionary **)r_value);
				if (t_error == kLCErrorNone)
					[*(NSDictionary **)r_value autorelease];
				break;
#endif
		}
	}

	return t_error;
}	

static LCError LCValueStore(MCVariableRef p_var, unsigned int p_options, void *p_value)
{
	LCError t_error;
	t_error = kLCErrorNone;

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
		case kLCValueOptionAsCChar:
			t_options_to_use = kMCOptionAsString;
			t_value_to_use = &t_cdata_value;
			t_cdata_value . buffer = p_value;
			t_cdata_value . length = 1;
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
				t_error = kLCErrorCannotEncodeCString;
			break;
		case kLCValueOptionAsObjcData:
			t_options_to_use = kMCOptionAsString;
			t_cdata_value . buffer = (char *)[*(NSData **)p_value bytes];
			t_cdata_value . length = [*(NSData **)p_value length];
			t_value_to_use = &t_cdata_value;
			break;
		case kLCValueOptionAsObjcArray:
		{
			// For efficiency, we use 'exchange' - this prevents copying a temporary array.
			MCVariableRef t_tmp_array;
			t_tmp_array = nil;
			if (t_error == kLCErrorNone)
				t_error = (LCError)MCVariableCreate(&t_tmp_array);
			if (t_error == kLCErrorNone)
				t_error = LCValueArrayFromObjcArray(t_tmp_array, *(NSArray **)p_value);
			if (t_error == kLCErrorNone)
				t_error = (LCError)s_interface -> variable_exchange(p_var, t_tmp_array);
			if (t_tmp_array != nil)
				MCVariableRelease(t_tmp_array);
			return t_error;
		}
		//  break;
		case kLCValueOptionAsObjcDictionary:
		{
			// For efficiency, we use 'exchange' - this prevents copying a temporary array.
			MCVariableRef t_tmp_array;
			t_tmp_array = nil;
			if (t_error == kLCErrorNone)
				t_error = (LCError)MCVariableCreate(&t_tmp_array);
			if (t_error == kLCErrorNone)
				t_error = LCValueArrayFromObjcDictionary(t_tmp_array, *(NSDictionary **)p_value);
			if (t_error == kLCErrorNone)
				t_error = (LCError)s_interface -> variable_exchange(p_var, t_tmp_array);
			if (t_tmp_array != nil)
				MCVariableRelease(t_tmp_array);
			return t_error;
		}
		//  break;
#endif
		default:
			t_error = kLCErrorBadValueOptions;
			break;
	}

	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> variable_store(p_var, t_options_to_use, t_value_to_use);
	
	return t_error;
}

// MW-2013-06-14: [[ ExternalsApiV5 ]] New internal method to convert between
//   values.
static LCError LCValueConvert(unsigned int p_in_options, void *p_in_value, unsigned int p_out_options, void *r_out_value)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	MCVariableRef t_temp_var;
	t_temp_var = nil;
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableCreate(&t_temp_var);
	
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableStore(t_temp_var, p_in_options, p_in_value);
		
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableFetch(t_temp_var, p_out_options, r_out_value);
		
	if (t_temp_var != nil)
		MCVariableRelease(t_temp_var);
		
	return t_error;
}

//////////

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

static LCError LCArrayResolvePath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, MCVariableRef& r_var)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	MCVariableRef t_key;
	t_key = (MCVariableRef)p_array;
	for(unsigned int i = 0; i <= p_path_length; i++)
	{
		t_error = (LCError)s_interface -> variable_lookup_key((MCVariableRef)t_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, i < p_path_length ? (void *)&p_path[i] : (void *)&p_key, false, &t_key);
		if (t_error != kLCErrorNone || t_key == nil)
			break;
	}
	
	if (t_error == kLCErrorNone)
		r_var = t_key;
	
	return t_error;
}

//////////

LCError LCArrayCountKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, unsigned int *r_count)
{		
	LCError t_error;
	t_error = kLCErrorNone;
	
	if (p_options != 0)
		t_error = kLCErrorBadValueOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kLCErrorNone)
	{
		if (t_var == nil)
			*r_count = 0;
		else
			t_error = (LCError)s_interface -> variable_count_keys(t_var, r_count);
	}
	
	return (LCError)t_error;
}

LCError LCArrayListKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, char **p_key_buffer, unsigned int p_key_buffer_size)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	if (p_options != 0)
		t_error = kLCErrorBadValueOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	unsigned int t_key_count;
	t_key_count = 0;
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> variable_count_keys((MCVariableRef)p_array, &t_key_count);
	
	if (t_error == kLCErrorNone && t_key_count > p_key_buffer_size)
		t_error = kLCErrorArrayBufferTooSmall;
	
	char **t_keys;
	t_keys = nil;
	if (t_error == kLCErrorNone)
	{
		t_keys = (char **)calloc(sizeof(char *), t_key_count);
		if (t_keys == nil)
			t_error = (LCError)kLCErrorOutOfMemory;
	}
	
	MCVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(unsigned int i = 0; i < t_key_count && t_error == kLCErrorNone; i++)
	{
		MCVariableRef t_key_var;
		t_error = (LCError)s_interface -> variable_iterate_keys(t_var, &t_iterator, kMCOptionAsCString, &t_keys[i], &t_key_var);
		if (t_error == kLCErrorNone)
		{
			t_keys[i] = strdup(t_keys[i]);
			if (t_keys[i] == NULL)
				t_error = kLCErrorOutOfMemory;
		}
	}
	
	if (t_error == kLCErrorNone)
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
	
	return t_error;
}

LCError LCArrayRemoveKeysOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	if (p_options != 0)
		t_error = kLCErrorBadValueOptions;
	
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kLCErrorNone)
		s_interface -> variable_clear(t_var);
	
	return (LCError)t_error;
}

LCError LCArrayHasKeyOnPath(LCArrayRef p_array, unsigned int p_options, const char **p_path, unsigned int p_path_length, const char *p_key, bool *r_exists)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	if ((p_options & ~kLCValueOptionMaskCaseSensitive) != 0)
		t_error = kLCErrorBadValueOptions;

	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
		t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length, p_key, t_var);
	
	if (t_error == kLCErrorNone)
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
	LCError t_error;
	t_error = kLCErrorNone;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
		t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length, p_key, t_var);
	
	if (t_error == kLCErrorNone && t_var == nil)
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
		case kLCValueOptionAsCChar:
			*(char *)r_value = '\0';
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
			t_error = kLCErrorBadValueOptions;
			break;
		}
		
		if (t_error == kLCErrorNone)
			*r_exists = false;
		
		return t_error;
	}

	if (t_error == kLCErrorNone)
		t_error = LCValueFetch(t_var, p_options, r_value);
	
	if (t_error == kLCErrorNone)
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
	LCError t_error;
	t_error = kLCErrorNone;

	unsigned int t_path_index;
	MCVariableRef t_previous_key, t_key;
	t_key = (MCVariableRef)p_array;
	for(t_path_index = 0; t_path_index <= p_path_length; t_path_index++)
	{
		t_previous_key = t_key;
		t_error = (LCError)s_interface -> variable_lookup_key((MCVariableRef)t_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, t_path_index < p_path_length ? (void *)&p_path[t_path_index] : (void *)&p_key, false, &t_key);
		if (t_error != kLCErrorNone || t_key == nil)
			break;
	}
	
	if (t_error == kLCErrorNone && t_key == nil)
		t_error = (LCError)s_interface -> variable_create(&t_key);
	
	if (t_error == kLCErrorNone)
		t_error = LCValueStore(t_key, p_options, p_value);
	
	if (t_error == kLCErrorNone && t_path_index == p_path_length + 1)
		return kLCErrorNone;
	
	MCVariableRef t_parent_key;
	t_parent_key = nil;
	if (t_error == kLCErrorNone && t_path_index < p_path_length)
		t_error = (LCError)s_interface -> variable_create(&t_parent_key);
	
	for(unsigned int i = p_path_length + 1; i > t_path_index && t_error == kLCErrorNone; i--)
	{
		MCVariableRef t_key_value;
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_lookup_key(i - 1 == t_path_index ? t_previous_key : t_parent_key, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, i <= p_path_length ? (void *)&p_path[i - 1] : (void *)&p_key, true, &t_key_value);
		if (t_error == kLCErrorNone)
			t_error = (LCError)s_interface -> variable_exchange(t_key_value, t_key);
		if (t_error == kLCErrorNone)
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
	LCError t_error;
	t_error = kLCErrorNone;
	
	if ((p_options & ~kLCValueOptionMaskCaseSensitive) != 0)
		t_error = kLCErrorBadValueOptions;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
	{
		if (p_path_length != 0)
			t_error = LCArrayResolvePath(p_array, p_options, p_path, p_path_length - 1, p_path[p_path_length - 1], t_var);
		else
			t_var = (MCVariableRef)p_array;
	}
	
	if (t_error == kLCErrorNone && t_var != nil)
		t_error = (LCError)s_interface -> variable_remove_key(t_var, LCValueOptionsGetCaseSensitive(p_options) | kMCOptionAsCString, (void *)&p_key);
		
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

LCError LCArrayListKeys(LCArrayRef array, unsigned int options, char **key_buffer, unsigned int key_buffer_size)
{
	return LCArrayListKeysOnPath(array, options, nil, 0, key_buffer, key_buffer_size);
}

LCError LCArrayRemoveKeys(LCArrayRef array, unsigned int options)
{
	return LCArrayRemoveKeysOnPath(array, options, nil, 0);
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

END_EXTERN_C

////////////////////////////////////////////////////////////////////////////////

BEGIN_EXTERN_C

typedef struct __LCObject *LCObjectRef;
typedef struct __LCWait *LCWaitRef;

//////////

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

LCError LCContextCaseSensitive(bool *r_case_sensitive)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarCaseSensitive, r_case_sensitive);
}

LCError LCContextConvertOctals(bool *r_convert_octals)
{
	return (LCError)s_interface -> context_query(kMCExternalContextVarConvertOctals, r_convert_octals);
}

LCError LCContextWholeMatches(bool *r_whole_matches)
{
	if (s_interface -> version < 5)
		return kLCErrorNotImplemented;
	return (LCError)s_interface -> context_query(kMCExternalContextVarWholeMatches, r_whole_matches);
}

//////////

static LCError LCContextQueryDelimiter(MCExternalContextVar p_var, unsigned int p_options, void *r_value)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	// Fetch the delimiter char from the context - this is returned as a uint32_t, but is just
	// a native char.
	uint32_t t_delimiter_char;
	t_delimiter_char = '\0';
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> context_query(p_var, &t_delimiter_char);
	
	// Convert the native char to the requested output.
	if (t_error == kLCErrorNone)
	{
		char t_native_delimiter_char;
		t_native_delimiter_char = (char)t_delimiter_char;
		t_error = LCValueConvert(kLCValueOptionAsCChar, &t_delimiter_char, p_options, r_value);
	}
	
	return t_error;
}

LCError LCContextItemDelimiter(unsigned int p_options, void *r_value)
{
	return LCContextQueryDelimiter(kMCExternalContextVarItemDelimiter, p_options, r_value);
}

LCError LCContextLineDelimiter(unsigned int p_options, void *r_value)
{
	return LCContextQueryDelimiter(kMCExternalContextVarLineDelimiter, p_options, r_value);
}

LCError LCContextRowDelimiter(unsigned int p_options, void *r_value)
{
	return LCContextQueryDelimiter(kMCExternalContextVarRowDelimiter, p_options, r_value);
}

LCError LCContextColumnDelimiter(unsigned int p_options, void *r_value)
{
	return LCContextQueryDelimiter(kMCExternalContextVarColumnDelimiter, p_options, r_value);
}

//////////

static LCError LCContextQueryVariable(MCExternalContextVar p_var, unsigned int p_options, void *r_value)
{
	LCError t_error;
	t_error = kLCErrorNone;

	// Request (direct) access to the variable.
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> context_query(kMCExternalContextVarIt, &t_var);
	
	// Fetch the value in the format requested.
	if (t_error == kLCErrorNone)
		t_error = LCValueFetch(t_var, p_options, r_value);
		
	// Release the variable.
	if (t_var != nil)
		MCVariableRelease(t_var);
		
	return t_error;
}

LCError LCContextIt(unsigned int p_options, void *r_value)
{
	return LCContextQueryVariable(kMCExternalContextVarIt, p_options, r_value);
}

LCError LCContextResult(unsigned int p_options, void *r_value)
{
	return LCContextQueryVariable(kMCExternalContextVarResult, p_options, r_value);
}

//////////

LCError LCContextEvaluate(const char *p_expression, unsigned int p_options, void *r_value)
{	
	if (s_interface -> version < 5)
		return kLCErrorNotImplemented;
		
	//////////

	LCError t_error;
	t_error = kLCErrorNone;

	MCVariableRef t_value_var;
	t_value_var = nil;
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableCreate(&t_value_var);
		
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> context_evaluate(p_expression, 0, nil, 0, t_value_var);
		
	if (t_error == kLCErrorNone)
		t_error = LCValueFetch(t_value_var, p_options, r_value);
		
	if (t_value_var != nil)
		MCVariableRelease(t_value_var);
		
	return t_error;
}

LCError LCContextExecute(const char *p_commands, unsigned int p_options)
{
	if (s_interface -> version < 5)
		return kLCErrorNotImplemented;
		
	return (LCError)s_interface -> context_execute(p_commands, 0, nil, 0);
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
				
			case 'c': // char
			{
				char t_char;
				LCBytes t_bytes;
				t_bytes . buffer = &t_char;
				t_bytes . length = 1;
				t_char = (char)va_arg(p_args, int);
				t_error = MCVariableStore(t_argv[i], kMCOptionAsString, &t_bytes);
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
	LCError t_error;
	t_error = kLCErrorNone;
	
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableCreate(&t_var);
	
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> object_get((MCObjectRef)p_object, p_options, p_name, p_key, t_var);
	
	if (t_error == kLCErrorNone)
		t_error = LCValueFetch(t_var, p_options, r_value);
	
	if (t_var != nil)
		MCVariableRelease(t_var);
	
	return t_error;
}

LCError LCObjectSet(LCObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, void *p_value)
{
	LCError t_error;
	t_error = kLCErrorNone;
		
	MCVariableRef t_var;
	t_var = nil;
	if (t_error == kLCErrorNone)
		t_error = (LCError)MCVariableCreate(&t_var);
	
	if (t_error == kLCErrorNone)
		t_error = LCValueStore(t_var, p_options, p_value);
	
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> object_set((MCObjectRef)p_object, p_options, p_name, p_key, t_var);
	
	if (t_var != nil)
		MCVariableRelease(t_var);
	
	return t_error;
}

/////////

struct __LCWait
{
	unsigned int references;
	unsigned int options;
	bool running;
	bool broken;
#ifdef __WINDOWS__
	HANDLE lock;
#else
	pthread_mutex_t lock;
#endif
};

static void LCWaitLock(LCWaitRef p_wait)
{
#ifdef __WINDOWS__
	WaitForSingleObject(p_wait -> lock, INFINITE);
#else
	pthread_mutex_lock(&p_wait -> lock);
#endif
}

static void LCWaitUnlock(LCWaitRef p_wait)
{
#ifdef __WINDOWS__
	ReleaseMutex(p_wait -> lock);
#else
	pthread_mutex_unlock(&p_wait -> lock);
#endif
}
	
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
#ifdef __WINDOWS__
	t_wait -> lock = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&t_wait -> lock, nil);
#endif
	
	*r_wait = t_wait;
	
	return (LCError)kMCErrorNone;
}

static void LCWaitDestroy(LCWaitRef p_wait)
{
#ifdef __WINDOWS__
	CloseHandle(p_wait -> lock);
#else
	pthread_mutex_destroy(&p_wait -> lock);
#endif
	free(p_wait);
}

LCError LCWaitRetain(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	LCWaitLock(p_wait);
	p_wait -> references += 1;
	LCWaitUnlock(p_wait);
	
	return kLCErrorNone;
}

LCError LCWaitRelease(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	LCWaitLock(p_wait);
	p_wait -> references -= 1;
	LCWaitUnlock(p_wait);
	
	if (p_wait -> references == 0)
		LCWaitDestroy(p_wait);
	
	return kLCErrorNone;
}

LCError LCWaitIsRunning(LCWaitRef p_wait, bool *r_running)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	LCWaitLock(p_wait);
	*r_running = p_wait -> running;
	LCWaitUnlock(p_wait);
	
	return kLCErrorNone;
}	

LCError LCWaitRun(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
	
	if (p_wait -> running)
		return kLCErrorWaitRunning;
	
	LCWaitLock(p_wait);
	
	p_wait -> running = true;

	MCError t_error;
	t_error = kMCErrorNone;
	for(;;)
	{
		if (p_wait -> broken)
			break;
		
		LCWaitUnlock(p_wait);
		
		t_error = s_interface -> wait_run(nil, p_wait -> options & kLCWaitOptionDispatching);
		
		LCWaitLock(p_wait);
		
		if (t_error != kMCErrorNone)
			break;
	}	

	p_wait -> running = false;
	
	LCWaitUnlock(p_wait);
	
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
	
	LCWaitLock(p_wait);
	if (p_wait -> running && !p_wait -> broken)
	{
		p_wait -> broken = true;
		s_interface -> wait_break(nil, 0);
	}
	LCWaitUnlock(p_wait);
	
	return kLCErrorNone;
}

LCError LCWaitReset(LCWaitRef p_wait)
{
	if (p_wait == nil)
		return kLCErrorNoWait;
		
	if (p_wait -> running)
		return kLCErrorWaitRunning;
	
	LCWaitLock(p_wait);
	p_wait -> broken = false;
	LCWaitUnlock(p_wait);
	
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
	
	// MERG-2013-06-14: [[ Bug ]] Make sure 'name' is initialized to nil.
	char *t_name;
	t_name = nil;
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

#if defined(__OBJC__) && defined(__BLOCKS__)
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

static bool error__bad_api_call(LCError err)
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

static bool error__report_not_supported(MCVariableRef result)
{
	const char *msg = "not supported";
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

static bool fetch__report_error(LCError err, const char *arg)
{
	if (err == kLCErrorOutOfMemory)
		return error__out_of_memory();
	if (err == kLCErrorNotABoolean)
		return error__format("parameter '%s' is not a boolean", arg);
	if (err == kLCErrorNotAString)
		return error__format("parameter '%s' is not a string", arg);
	if (err == kLCErrorNotABinaryString)
		return error__format("parameter '%s' is not a binary string", arg);
	if (err == kLCErrorNotAnInteger)
		return error__format("parameter '%s' is not an integer", arg);
	if (err == kLCErrorNotANumber)
		return error__format("parameter '%s' is not a number", arg);
	if (err == kLCErrorNumberTooBig)
		return error__format("parameter '%s' is too big", arg);
	if (err == kLCErrorNotASequence)
		return error__format("parameter '%s' is not a sequence", arg);
	if (err == kLCErrorNotAnArray)
		return error__format("parameter '%s' is not an array", arg);
	return error__bad_api_call(err);
}

static bool fetch__bool(const char *arg, MCVariableRef var, bool& r_value)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsBoolean, &r_value);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__cstring(const char *arg, MCVariableRef var, char*& r_value)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsCString, &r_value);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__cdata(const char *arg, MCVariableRef var, LCBytes& r_value)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsCData, &r_value);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__int(const char *arg, MCVariableRef var, int& r_value)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsInteger, &r_value);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__double(const char *arg, MCVariableRef var, double& r_value)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsReal, &r_value);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__lc_array(const char *arg, MCVariableRef var, LCArrayRef& r_array)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsLCArray, &r_array);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

#ifdef __OBJC__
static bool fetch__objc_string(const char *arg, MCVariableRef var, NSString*& r_string)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsObjcString, &r_string);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__objc_number(const char *arg, MCVariableRef var, NSNumber*& r_number)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsObjcNumber, &r_number);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__objc_data(const char *arg, MCVariableRef var, NSData*& r_data)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsObjcData, &r_data);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__objc_array(const char *arg, MCVariableRef var, NSArray*& r_array)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsObjcArray, &r_array);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}

static bool fetch__objc_dictionary(const char *arg, MCVariableRef var, NSDictionary*& r_dictionary)
{
	LCError err;
	err = LCValueFetch(var, kLCValueOptionAsObjcDictionary, &r_dictionary);
	if (err == kLCErrorNone)
		return true;
	return fetch__report_error(err, arg);
}
#endif

////////////////////////////////////////////////////////////////////////////////

static bool store__report_error(LCError err)
{
	if (err == kLCErrorOutOfMemory)
		return error__out_of_memory();
	if (err == kLCErrorCannotEncodeCString)
		return error__bad_string_conversion();
	return error__bad_api_call(err);
}

static bool store__int(MCVariableRef var, int value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsInteger, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__double(MCVariableRef var, double value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsReal, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__bool(MCVariableRef var, bool value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsBoolean, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__cstring(MCVariableRef var, const char* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsCString, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__cdata(MCVariableRef var, LCBytes value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsCData, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__lc_array(MCVariableRef var, LCArrayRef value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsLCArray, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

#ifdef __OBJC__
static bool store__objc_string(MCVariableRef var, NSString* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsObjcString, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__objc_number(MCVariableRef var, NSNumber* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsObjcNumber, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__objc_data(MCVariableRef var, NSData* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsObjcData, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__objc_array(MCVariableRef var, NSArray* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsObjcArray, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}

static bool store__objc_dictionary(MCVariableRef var, NSDictionary* value)
{
	LCError err;
	err = LCValueStore(var, kLCValueOptionAsObjcDictionary, &value);
	if (err == kLCErrorNone)
		return true;
	return store__report_error(err);
}
#endif

////////////////////////////////////////////////////////////////////////////////

static bool verify__out_parameter(const char *arg, MCVariableRef var)
{
	LCError err;
	bool transient;
	err = (LCError)MCVariableIsTransient(var, &transient);
	if (err == kLCErrorNone)
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
static JNIEnv *s_android_env = nil;
static JNIEnv *s_engine_env = nil;
static jclass s_java_class = nil;

//////////

enum JavaNativeType
{
	kJavaNativeTypeBoolean,
	kJavaNativeTypeInteger,
	kJavaNativeTypeDouble,
	kJavaNativeTypeString,
	kJavaNativeTypeData,
};

struct JavaNativeMapping
{
	JavaNativeType type;
	const char *class_name; 
	const char *method_name; 
	const char *method_sig; 
	jclass java_class; 
	jmethodID java_method; 
};

static JavaNativeMapping s_native_mappings[] =
{
	{ kJavaNativeTypeBoolean, "java/lang/Boolean", "booleanValue", "()Z", nil, nil },
	{ kJavaNativeTypeInteger, "java/lang/Integer", "intValue", "()I", nil, nil },
	{ kJavaNativeTypeDouble, "java/lang/Double", "doubleValue", "()D", nil, nil },
	{ kJavaNativeTypeString, "java/lang/String", nil, nil, nil, nil },
	{ kJavaNativeTypeData, "[B", nil, nil, nil, nil },
	{ kJavaNativeTypeData, "java/nio/ByteBuffer", "array", "()[B", nil, nil },
};

static bool java__initialize(JNIEnv *env)
{
	for(uint32_t i = 0; i < sizeof(s_native_mappings) / sizeof(s_native_mappings[0]); i++)
	{
		jclass t_class;
		t_class = env -> FindClass(s_native_mappings[i] . class_name);
		if (t_class == nil)
			return false;
		
		s_native_mappings[i] . java_class = (jclass)env -> NewGlobalRef(t_class);
		if (s_native_mappings[i] . java_class == nil)
			return false;
		
		if (s_native_mappings[i] . method_name != nil)
		{
			s_native_mappings[i] . java_method = env -> GetMethodID(t_class, s_native_mappings[i] . method_name, s_native_mappings[i] . method_sig);
			if (s_native_mappings[i] . java_method == nil)
				return false;
		}
		else
			s_native_mappings[i] . java_method = nil;
	}
	
	return true;
}
	
static void java__finalize(JNIEnv *env)
{
	for(uint32_t i = 0; i < sizeof(s_native_mappings) / sizeof(s_native_mappings[0]); i++)
		env -> DeleteGlobalRef(s_native_mappings[i] . java_class);
}
	
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

static jobject java__get_engine(void)
{
	jobject t_activity;
	s_interface -> interface_query(kMCExternalInterfaceQueryEngine, &t_activity);
	return t_activity;
}

//////////
    

static LCError java_from__cstring(JNIEnv *env, const char* p_value, jobject& r_value)
{
    LCError err;
    err = kLCErrorNone;
    
    size_t t_char_count;
    jchar *t_jchar_value;
    t_jchar_value = nil;
    if (err == kLCErrorNone)
    {
        t_char_count = strlen(p_value);
        t_jchar_value = (jchar *)malloc(sizeof(jchar) * t_char_count);
        if (t_jchar_value == nil)
            err = kLCErrorOutOfMemory;
    }
    
    if (err == kLCErrorNone)
    {
        for(uint32_t i = 0; i < t_char_count; i++)
            t_jchar_value[i] = p_value[i];
    }
    
    jobject t_java_value;
    if (err == kLCErrorNone)
    {
        t_java_value = (jobject)env -> NewString(t_jchar_value, t_char_count);
        if (t_java_value == nil || env -> ExceptionOccurred() != nil)
        {
            env -> ExceptionClear();
            err = kLCErrorOutOfMemory;
        }
    }
    
    free(t_jchar_value);
    
    if (err == kLCErrorNone)
        r_value = t_java_value;
    
    return err;
}

static bool default__java_string(JNIEnv *env, const char *p_value, jobject& r_java_value)
{
    LCError err;
    err = kLCErrorNone;
    
    err = java_from__cstring(env, p_value, r_java_value);
    
    if (err == kLCErrorNone)
        return true;
    else
        return error__out_of_memory();
}
    
static bool fetch__java_string(JNIEnv *env, const char *arg, MCVariableRef var, jobject& r_value)
{
	LCError err;
	err = kLCErrorNone;
	
	char *t_cstring_value;
	t_cstring_value = nil;
	if (err == kLCErrorNone)
		err = LCValueFetch(var, kLCValueOptionAsCString, &t_cstring_value);
    
    jobject t_java_value;
    t_java_value = nil;
    if (err == kLCErrorNone)
        err = java_from__cstring(env, t_cstring_value, t_java_value);

    free(t_cstring_value);
	
	if (err == kLCErrorNone)
	{
		r_value = t_java_value;
		return true;
	}
	
	return fetch__report_error(err, arg);
}
    
static bool fetch__java_data(JNIEnv *env, const char *arg, MCVariableRef var, jobject& r_value)
{
	LCError err;
	err = kLCErrorNone;
	
	LCBytes t_cdata_value;
	t_cdata_value . buffer = nil;
	t_cdata_value . length = 0;
	if (err == kLCErrorNone)
		err = LCValueFetch(var, kLCValueOptionAsCString, &t_cdata_value);
	
	jobject t_java_value;
	t_java_value = nil;
	if (err == kLCErrorNone)
	{
		t_java_value = (jobject)env -> NewByteArray(t_cdata_value . length);
		if (t_java_value == nil || env -> ExceptionOccurred() != nil)
		{
			env -> ExceptionClear();
			err = kLCErrorOutOfMemory;
		}
	}
	
	if (err == kLCErrorNone)
	{
		env -> SetByteArrayRegion((jbyteArray)t_java_value, 0, t_cdata_value . length, (const jbyte *)t_cdata_value . buffer);
		if (env -> ExceptionOccurred() != nil)
		{
			env -> ExceptionClear();
			err = kLCErrorOutOfMemory;
		}
	}
	
	free(t_cdata_value . buffer);
	
	if (err == kLCErrorNone)
	{
		r_value = t_java_value;
		return true;
	}
	
	return fetch__report_error(err, arg);
}

static LCError java_to__cstring(JNIEnv *env, jobject value, char*& r_cstring)
{
	LCError err;
	err = kLCErrorNone;
	
	const jchar *t_unichars;
	uint32_t t_unichar_count;
	t_unichars = 0;
	t_unichar_count = 0;
	if (err == kLCErrorNone)
	{
		t_unichars = env -> GetStringChars((jstring)value, nil);
		if (t_unichars == nil || env -> ExceptionOccurred() != nil)
		{
			env -> ExceptionClear();
			err = kLCErrorOutOfMemory;
		}
	}
	
	char *t_native_value;
	t_native_value = nil;
	if (err == kLCErrorNone)
	{
		t_unichar_count = env -> GetStringLength((jstring)value);
		t_native_value = (char *)malloc(t_unichar_count + 1);
		if (t_native_value == nil)
			err = kLCErrorOutOfMemory;
	}
	
	if (err == kLCErrorNone)
	{
		for(uint32_t i = 0; i < t_unichar_count; i++)
			t_native_value[i] = t_unichars[i] < 256 ? t_unichars[i] : '?';
		t_native_value[t_unichar_count] = 0;
	}

	if (err == kLCErrorNone)
		r_cstring = t_native_value;
	else
		free(t_native_value);

	if (t_unichars != nil)
		env -> ReleaseStringChars((jstring)value, t_unichars);

	return err;
}

static bool store__java_string(JNIEnv *env, MCVariableRef var, jobject value)
{
	LCError err;
	err = kLCErrorNone;
	
	char *t_native_value;
	if (err == kLCErrorNone)
		err = java_to__cstring(env, value, t_native_value);
	
	if (err == kLCErrorNone)
		err = LCValueStore(var, kLCValueOptionAsCString, &t_native_value);
		
	free(t_native_value);
		
	if (err == kLCErrorNone)
		return true;
		
	return store__report_error(err);
}

static bool store__java_data(JNIEnv *env, MCVariableRef var, jobject value)
{
	LCError err;
	err = kLCErrorNone;
	
	LCBytes t_native_value;
	t_native_value . buffer = nil;
	t_native_value . length = 0;
	if (err == kLCErrorNone)
	{
		t_native_value . buffer = env -> GetByteArrayElements((jbyteArray)value, nil);
		if (t_native_value . buffer == nil || env -> ExceptionOccurred() != nil)
		{
			env -> ExceptionClear();
			err = kLCErrorOutOfMemory;
		}
	}
	
	if (err == kLCErrorNone)
	{
		t_native_value . length = env -> GetArrayLength((jbyteArray)value);
		err = LCValueStore(var, kLCValueOptionAsCData, &t_native_value);
	}
	
	if (t_native_value . buffer != nil)
		env -> ReleaseByteArrayElements((jbyteArray)value, (jbyte *)t_native_value . buffer, 0);
		
	if (err == kLCErrorNone)
		return true;
		
	return store__report_error(err);
}

static void free__java_string(JNIEnv *env, jobject value)
{
	env -> DeleteLocalRef(value);
}

static void free__java_data(JNIEnv *env, jobject value)
{
	env -> DeleteLocalRef(value);
}

//////////

static void java_lcapi__throw(JNIEnv *env, LCError p_error)
{
	// TODO
}
	
static jlong java_lcapi_ObjectResolve(JNIEnv *env, jobject chunk)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	char *t_chunk_cstring;
	t_chunk_cstring = nil;
	if (t_error == kLCErrorNone)
		t_error = java_to__cstring(env, chunk, t_chunk_cstring);
	
	LCObjectRef t_object;
	t_object = nil;
	if (t_error == kLCErrorNone)
		t_error = LCObjectResolve(t_chunk_cstring, &t_object);
	
	free(t_chunk_cstring);

	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return 0;
	}
	
	return (jlong)t_object;
}

static void java_lcapi_ObjectRetain(JNIEnv *env, jlong object)
{
	LCError t_error;
	t_error = LCObjectRetain((LCObjectRef)object);
	if (t_error != kLCErrorNone)
		java_lcapi__throw(env, t_error);
}

static void java_lcapi_ObjectRelease(JNIEnv *env, jlong object)
{
	LCError t_error;
	t_error = LCObjectRelease((LCObjectRef)object);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}

static jboolean java_lcapi_ObjectExists(JNIEnv *env, jlong object)
{
	LCError t_error;
	bool t_exists;
	t_error = LCObjectExists((LCObjectRef)object, &t_exists);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return false;
	}
	return t_exists;
}
	
static LCError java_lcapi_LCCreateArguments(JNIEnv *env, jobjectArray arguments, MCVariableRef*& r_argv, uint32_t& r_argc)
{	
    LCError t_error;
    t_error = kLCErrorNone;
    
    uint32_t t_argc;
    t_argc = env->GetArrayLength(arguments);
    
    MCVariableRef *t_argv;
    
    if (t_error == kLCErrorNone)
    {
        t_argv = nil;
        t_argv = (MCVariableRef *)calloc(t_argc, sizeof(MCVariableRef));
        if (t_argv == nil)
            return kLCErrorOutOfMemory;
    }

    for (uint32_t i = 0; i < t_argc; i++)
    {
		if (t_error == kLCErrorNone)
			t_error = (LCError)MCVariableCreate(&t_argv[i]);
		
        jobject t_param;
        t_param = nil;
		if (t_error == kLCErrorNone)
        {
            t_param = env->GetObjectArrayElement(arguments, i);
            if (t_param == nil)
                t_error = kLCErrorFailed;
        }
                
        if (t_error == kLCErrorNone)
        {
			for(uint32_t j = 0; j < sizeof(s_native_mappings) / sizeof(s_native_mappings[0]); j++)
			{
				if (!env -> IsInstanceOf(t_param, s_native_mappings[j] . java_class) == JNI_TRUE)
					continue;
				
				JavaNativeMapping *t_mapping;
				t_mapping = &s_native_mappings[j];
				switch(t_mapping -> type)
				{
					case kJavaNativeTypeBoolean:
					{
						bool t_boolean;
						t_boolean = (bool)env -> CallBooleanMethod(t_param, t_mapping -> java_method);
						t_error = LCValueStore(t_argv[i], kLCValueOptionAsBoolean, &t_boolean);
					}
					break;
					case kJavaNativeTypeInteger:
					{
						int t_integer;
						t_integer = (bool)env -> CallIntMethod(t_param, t_mapping -> java_method);
						t_error = LCValueStore(t_argv[i], kLCValueOptionAsInteger, &t_integer);
					}
					break;
					case kJavaNativeTypeDouble:
					{
						double t_double;
						t_double = (bool)env -> CallDoubleMethod(t_param, t_mapping -> java_method);
						t_error = LCValueStore(t_argv[i], kLCValueOptionAsReal, &t_double);
					}
					break;
					case kJavaNativeTypeString:
					{
						jobject t_java_string;
						if (t_mapping -> java_method != nil)
							t_java_string = (jobject)env -> CallObjectMethod(t_param, t_mapping -> java_method);
						else
							t_java_string = t_param;
						
						char *t_cstring;
						t_cstring = nil;
						if (t_error == kLCErrorNone)
							t_error = java_to__cstring(env, t_java_string, t_cstring);
						if (t_error == kLCErrorNone)
							t_error = LCValueStore(t_argv[i], kLCValueOptionAsCString, &t_cstring);
						free(t_cstring);
					}
					break;
					case kJavaNativeTypeData:
					{
						jobject t_java_data;
						if (t_mapping -> java_method != nil)
							t_java_data = (jobject)env -> CallObjectMethod(t_param, t_mapping -> java_method);
						else
							t_java_data = t_param;
						
						LCBytes t_cdata;
						if (t_error == kLCErrorNone)
						{
							t_cdata . buffer = env -> GetByteArrayElements((jbyteArray)t_java_data, nil);
							t_cdata . length = env -> GetArrayLength((jbyteArray)t_java_data);
							t_error = LCValueStore(t_argv[i], kLCValueOptionAsCData, &t_cdata);
							env -> ReleaseByteArrayElements((jbyteArray)t_java_data, (jbyte *)t_cdata . buffer, 0);
						}
					}
					break;
				}
			}
		}
	}
    
    if (t_error == kLCErrorNone)
    {
        r_argv = t_argv;
        r_argc = t_argc;
    }
    else
        LCArgumentsDestroy(t_argv, t_argc);

    return t_error;
}

static void java_lcapi_ObjectSend(JNIEnv *env, jlong object, jobject message, jobjectArray arguments)
{
	LCError t_error;
	t_error = kLCErrorNone;
	
	char *t_message_cstring;
	t_message_cstring = nil;
	if (t_error == kLCErrorNone)
		t_error = java_to__cstring(env, message, t_message_cstring);

	MCVariableRef *t_argv;
	uint32_t t_argc;
	t_argv = nil;
	t_argc = 0;
	if (t_error == kLCErrorNone)
		t_error = java_lcapi_LCCreateArguments(env, arguments, t_argv, t_argc);
	
	if (t_error == kLCErrorNone)
	{
		MCDispatchStatus t_status;
		t_error = (LCError)s_interface -> object_dispatch((MCObjectRef)object, kMCDispatchTypeCommand, t_message_cstring, t_argv, t_argc, &t_status);
		if (t_error == kLCErrorNone)
		{
			switch(t_status)
			{
				case kMCDispatchStatusError:
					t_error = kLCErrorFailed;
					break;
				case kMCDispatchStatusExit:
					t_error = kLCErrorExited;
					break;
				case kMCDispatchStatusAbort:
					t_error = kLCErrorAborted;
					break;
				default:
					break;
			}
		}
	}
	
	LCArgumentsDestroy(t_argv, t_argc);

	free(t_message_cstring);
	
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}

struct java_lcapi_LCObjectPost_context
{
    MCObjectRef object;
    char *message;
    MCVariableRef *argv;
	uint32_t argc;
    LCError result;
};

static void java_lcapi_ObjectPost_perform(void *p_context)
{
    java_lcapi_LCObjectPost_context *context;
	context = (java_lcapi_LCObjectPost_context *)p_context;
    
    LCError t_error;
    t_error = kLCErrorNone;
	
	struct LCObjectPostV_event *t_event;
	t_event = nil;
	if (t_error == kLCErrorNone)
	{
		t_event = (LCObjectPostV_event *)calloc(1, sizeof(LCObjectPostV_event));
		if (t_event == nil)
			t_error = kLCErrorOutOfMemory;
	}
	
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> object_retain(context -> object);
	
	if (t_error == kLCErrorNone)
	{
		t_event -> object = context -> object;
		t_event -> message = context -> message;
		t_event -> argv = context -> argv;
		t_event -> argc = context -> argc;
		t_error = (LCError)s_interface -> engine_run_on_main_thread((void *)LCObjectPostV_dispatch, t_event, kMCRunOnMainThreadPost | kMCRunOnMainThreadRequired | kMCRunOnMainThreadSafe | kMCRunOnMainThreadDeferred);
	}
    
	if (t_error != kLCErrorNone)
	{
		if (t_event -> object != nil)
			s_interface -> object_release(t_event -> object);
		free(t_event -> message);
		LCArgumentsDestroy(context -> argv, context -> argc);
		free(t_event);
	}

    context -> result = (LCError)t_error;
}

static void java_lcapi_ObjectPost(JNIEnv *env, jlong object, jobject message, jobjectArray arguments)
{
	LCError t_error;
    t_error = kLCErrorNone;
    
	// The cstring will be given to the perform method.
	char *t_message_cstring;
	t_message_cstring = nil;
	if (t_error == kLCErrorNone)
		t_error = java_to__cstring(env, message, t_message_cstring);

	// Create the arguments array here to make sure we don't get JNI thread problems.
	// (this could be called from any thread, and we want to avoid having to make 
	//  globalref's to the arguments array). The arguments array will be freed
	// by the perform method (to stop threading problems engine-side).
	MCVariableRef *t_argv;
	uint32_t t_argc;
	t_argv = nil;
	t_argc = 0;
	if (t_error == kLCErrorNone)
        t_error = java_lcapi_LCCreateArguments(env, arguments, t_argv, t_argc);
	
    struct java_lcapi_LCObjectPost_context t_context;
    t_context . object = (MCObjectRef)object;
	t_context . message = t_message_cstring;
	t_context . argv = t_argv;
	t_context . argc = t_argc;
	if (t_error == kLCErrorNone)
		t_error = (LCError)s_interface -> engine_run_on_main_thread((void *)java_lcapi_ObjectPost_perform, &t_context, kMCRunOnMainThreadSend | kMCRunOnMainThreadOptional | kMCRunOnMainThreadUnsafe | kMCRunOnMainThreadImmediate);
    
    if (t_error == kLCErrorNone)
        t_error = t_context . result;
	
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}	
}

static jlong java_lcapi_ContextMe(JNIEnv *env)
{
	LCError t_error;
	LCObjectRef t_me;
	t_error = LCContextMe(&t_me);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return false;
	}
	return (jlong)t_me;
}

static jlong java_lcapi_ContextTarget(JNIEnv *env)
{
	LCError t_error;
	LCObjectRef t_target;
	t_error = LCContextTarget(&t_target);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return false;
	}
	return (jlong)t_target;
}

static jobject java_lcapi_InterfaceQueryActivity(JNIEnv *env)
{
	return java__get_activity();
}
	
static jobject java_lcapi_InterfaceQueryContainer(JNIEnv *env)
{
	return java__get_container();
}

static jobject java_lcapi_InterfaceQueryEngine(JNIEnv *env)
{
	return java__get_engine();
}

static void java_lcapi_RunOnSystemThread_callback(void *context)
{
	jclass t_class;
	t_class = s_android_env -> GetObjectClass((jobject)context);
	
	jmethodID t_method;
	t_method = s_android_env -> GetMethodID(t_class, "run", "()V");
	
	if (t_method != 0)
		s_android_env -> CallVoidMethod((jobject)context, t_method);
}		
	
static void java_lcapi_RunOnSystemThread(JNIEnv *env, jobject runnable)
{
	jobject t_global_runnable;
	t_global_runnable = s_engine_env -> NewGlobalRef(runnable);
	LCRunOnSystemThread(java_lcapi_RunOnSystemThread_callback, t_global_runnable);
	s_engine_env -> DeleteGlobalRef(t_global_runnable);
}		

static jlong java_lcapi_WaitCreate(JNIEnv *env, jint options)
{
	LCError t_error;
	LCWaitRef t_wait;
	t_error = LCWaitCreate(options, &t_wait);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return false;
	}
	return (jlong)t_wait;
}

static void java_lcapi_WaitRelease(JNIEnv *env, jlong wait)
{
	LCError t_error;
	t_error = LCWaitRelease((LCWaitRef)wait);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}

static jboolean java_lcapi_WaitIsRunning(JNIEnv *env, jlong wait)
{
	LCError t_error;
	bool t_running;
	t_error = LCWaitIsRunning((LCWaitRef)wait, &t_running);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return false;
	}
	return t_running;
}
	
static void java_lcapi_WaitRun(JNIEnv *env, jlong wait)
{
	LCError t_error;
	t_error = LCWaitRun((LCWaitRef)wait);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}

static void java_lcapi_WaitReset(JNIEnv *env, jlong wait)
{
	LCError t_error;
	t_error = LCWaitReset((LCWaitRef)wait);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}

static void java_lcapi_WaitBreak(JNIEnv *env, jlong wait)
{
	LCError t_error;
	t_error = LCWaitBreak((LCWaitRef)wait);
	if (t_error != kLCErrorNone)
	{
		java_lcapi__throw(env, t_error);
		return;
	}
}
	
#endif

////////////////////////////////////////////////////////////////////////////////
