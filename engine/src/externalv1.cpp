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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

//#include "execpt.h"
#include "param.h"
#include "scriptpt.h"
#include "chunk.h"
#include "handler.h"
#include "license.h"
#include "util.h"
#include "mcerror.h"
#include "osspec.h"
#include "globals.h"
#include "object.h"
#include "control.h"
#include "notify.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "debug.h"

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

typedef class MCExternalVariable *MCExternalVariableRef;
typedef MCObjectHandle *MCExternalObjectRef;

typedef void *MCExternalVariableIteratorRef;
typedef uint32_t MCExternalValueOptions;
typedef uint32_t MCExternalRunOnMainThreadOptions;
typedef void (*MCExternalThreadOptionalCallback)(void *state);
typedef void (*MCExternalThreadRequiredCallback)(void *state, int flags);

// MW-2013-06-14: [[ ExternalsApiV5 ]] Update the interface version.
// MW-2014-06-26: [[ ExternalsApiV6 ]] Update the interface version for unicode changes.
#define kMCExternalInterfaceVersion 5

enum
{
	// Post the callback and wait until the callback is invoked
	kMCExternalRunOnMainThreadSend = 0 << 0,
	// Post the callback and return immediately
	kMCExternalRunOnMainThreadPost = 1 << 0,
	// The callback does not have to be executed
	kMCExternalRunOnMainThreadOptional = 0 << 1,
	// The callback has to be executed (changes signature)
	kMCExternalRunOnMainThreadRequired = 1 << 1,
	// The callback should be invoked in a script-safe environment
	kMCExternalRunOnMainThreadSafe = 0 << 2,
	// The callback should can be invoked in a non-script-safe environment
	kMCExternalRunOnMainThreadUnsafe = 1 << 2,
	// The callback should be invoked as soon as possible
	kMCExternalRunOnMainThreadImmediate = 0 << 3,
	// The callback should be invoked synchronized to the event queue
	kMCExternalRunOnMainThreadDeferred = 1 << 3,
    
    // The mask for the JumpTo options.
    kMCExternalRunOnMainThreadJumpTo = 3 << 4,

	// Call the callback on the UI thread (V4+).
	kMCExternalRunOnMainThreadJumpToUI = 1 << 4,
	// Call the callback on the Engine thread (V4+)
	kMCExternalRunOnMainThreadJumpToEngine = 2 << 4,
};

enum
{
	kMCExternalWaitOptionBlocking = 0,
	kMCExternalWaitOptionDispatching = 1 << 0
};

enum
{
	kMCExternalValueOptionAsVariable = 0,
	kMCExternalValueOptionAsBoolean = 1,
	kMCExternalValueOptionAsInteger = 2,
	kMCExternalValueOptionAsCardinal = 3,
	kMCExternalValueOptionAsReal = 4,
    
    // This fetches the value as a native string with length
	kMCExternalValueOptionAsString = 5,
    // This fetchs the value as a native string with NUL terminator
	kMCExternalValueOptionAsCString = 6,
    
    // V6-ADDITIONS-START
    kMCExternalValueOptionAsUTF8String = 7,
    kMCExternalValueOptionAsUTF8CString = 8,
    kMCExternalValueOptionAsUTF16String = 9,
    kMCExternalValueOptionAsUTF16CString = 10,
    
#ifdef __HAS_CORE_FOUNDATION__
    kMCExternalValueOptionAsNSNumber = 17,
    kMCExternalValueOptionAsCFNumber = 18,
    kMCExternalValueOptionAsNSString = 19,
    kMCExternalValueOptionAsCFString = 20,
    kMCExternalValueOptionAsNSData = 21,
    kMCExternalValueOptionAsCFData = 22,
    kMCExternalValueOptionAsNSArray = 23,
    kMCExternalValueOptionAsCFArray = 24,
    kMCExternalValueOptionAsNSDictionary = 25,
    kMCExternalValueOptionAsCFDictionary = 26,
#endif
    // V6-ADDITIONS-END
    
	kMCExternalValueOptionCaseSensitiveMask = 3 << 30,
	kMCExternalValueOptionDefaultCaseSensitive = 0 << 30,
	kMCExternalValueOptionCaseSensitive = 1 << 30,
	kMCExternalValueOptionNotCaseSensitive = 2 << 30,

	kMCExternalValueOptionConvertOctalsMask = 3 << 28,
	kMCExternalValueOptionDefaultConvertOctals = 0 << 28,
	kMCExternalValueOptionConvertOctals = 1 << 28,
	kMCExternalValueOptionDoNotConvertOctals = 2 << 28,

	kMCExternalValueOptionNumberFormatMask = 3 << 26,
	kMCExternalValueOptionDefaultNumberFormat = 0 << 26,
	kMCExternalValueOptionDecimalNumberFormat = 1 << 26,
	kMCExternalValueOptionScientificNumberFormat = 2 << 26,
	kMCExternalValueOptionCompactNumberFormat = 3 << 26
};

enum MCExternalError
{
	kMCExternalErrorNone = 0,

	kMCExternalErrorOutOfMemory = 1,
	kMCExternalErrorNotImplemented = 2,

	kMCExternalErrorNoVariable = 3,
	kMCExternalErrorNoValue = 4,
	kMCExternalErrorNoIterator = 5,
	kMCExternalErrorNoBuffer = 6,
	kMCExternalErrorNotAnExternalTemporary = 7,
	kMCExternalErrorInvalidValueType = 8,
	kMCExternalErrorNotABoolean = 9,
	kMCExternalErrorNotANumber = 10,
	kMCExternalErrorNotAnInteger = 11,
	kMCExternalErrorNotAString = 12,
	kMCExternalErrorNotACString = 13,
	kMCExternalErrorNotAnArray = 14,
	kMCExternalErrorDstNotAString = 15,
	kMCExternalErrorNumericOverflow = 16,
	kMCExternalErrorInvalidConvertOctalsOption = 17,
	kMCExternalErrorInvalidCaseSensitiveOption = 18,
	kMCExternalErrorInvalidVariableQuery = 19,
	kMCExternalErrorInvalidContextQuery = 20,
	kMCExternalErrorVariableDoesNotExist = 21,
	kMCExternalErrorInvalidEdit = 22,

	kMCExternalErrorNoObject = 23,
	kMCExternalErrorNoObjectId = 24,
	kMCExternalErrorNoObjectMessage = 25,
	kMCExternalErrorNoObjectArguments = 26,
	kMCExternalErrorMalformedObjectChunk = 27,
	kMCExternalErrorCouldNotResolveObject = 18,
	kMCExternalErrorObjectDoesNotExist = 29,
	
	kMCExternalErrorNoDefaultStack = 30,
	
	kMCExternalErrorAborted = 31,
	kMCExternalErrorFailed = 32,
	kMCExternalErrorExited = 33,
	
	kMCExternalErrorNoObjectProperty = 34,
	kMCExternalErrorNoObjectPropertyValue = 35,
	
	kMCExternalErrorInvalidInterfaceQuery = 36,
    
    // SN-2014-07-01" [[ ExternalsApiV6 ]] Errors which might get triggered when converting from an MC* type to a CF* type
    // Following the definitions in Support.mm
#ifdef __HAS_CORE_FOUNDATION__
    kMCExternalErrorNotASequence = 40,
    kMCExternalErrorCannotEncodeMap = 41,
#endif
};

enum MCExternalContextQueryTag
{
	kMCExternalContextQueryMe = 1,
	kMCExternalContextQueryTarget,
	kMCExternalContextQueryResult,
	kMCExternalContextQueryIt,

	kMCExternalContextQueryCaseSensitive,
	kMCExternalContextQueryConvertOctals,
	kMCExternalContextQueryNumberFormat,
	
    // V6-TODO: Make sure these return the same as they did in previous versions - i.e. native char
	kMCExternalContextQueryItemDelimiter,
	kMCExternalContextQueryLineDelimiter,
	kMCExternalContextQueryColumnDelimiter,
	kMCExternalContextQueryRowDelimiter,
	
	kMCExternalContextQueryDefaultStack,
	kMCExternalContextQueryDefaultCard,
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to fetch 'the wholeMatches'.
	kMCExternalContextQueryWholeMatches,
    
    // SN-2014-07-01: [[ ExternalsApiV6 ]] These return a UTF16CString
	kMCExternalContextQueryUnicodeItemDelimiter,
	kMCExternalContextQueryUnicodeLineDelimiter,
	kMCExternalContextQueryUnicodeColumnDelimiter,
	kMCExternalContextQueryUnicodeRowDelimiter,
};

enum MCExternalVariableQueryTag
{
	kMCExternalVariableQueryIsTemporary = 1,
	kMCExternalVariableQueryIsTransient,
	kMCExternalVariableQueryFormat,
	kMCExternalVariableQueryRetention,
	kMCExternalVariableQueryIsAnArray,
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to determine if a variable
	//   is a 1-based, dense, numerically keyed array (aka a sequence).
	kMCExternalVariableQueryIsASequence, // V5
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to determine if a variable
	//   is empty.
	kMCExternalVariableQueryIsEmpty, // V5
};

enum MCExternalInterfaceQueryTag
{
	kMCExternalInterfaceQueryView = 1,
	kMCExternalInterfaceQueryViewScale = 2,
	kMCExternalInterfaceQueryViewController = 3,
	kMCExternalInterfaceQueryActivity = 4, // V4
	kMCExternalInterfaceQueryContainer = 5, // V4
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to get the JavaEnv for the
	//   script thread.
	kMCExternalInterfaceQueryScriptJavaEnv = 6, // V5
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to get the JavaEnv for the
	//   system thread.
	kMCExternalInterfaceQuerySystemJavaEnv = 7, // V5
	
	// MW-2013-07-25: [[ ExternalsApiV5 ]] Accessor to get the EngineApi object
	kMCExternalInterfaceQueryEngine = 8, // V5
};

enum
{
	kMCExternalObjectUpdateOptionRect = 1 << 0,
};

enum MCExternalDispatchType
{
	kMCExternalDispatchCommand,
	kMCExternalDispatchFunction
};

enum MCExternalDispatchStatus
{
	kMCExternalDispatchStatusHandled,
	kMCExternalDispatchStatusNotHandled,
	kMCExternalDispatchStatusPassed,
	kMCExternalDispatchStatusError,
	kMCExternalDispatchStatusExit,
	kMCExternalDispatchStatusAbort,
};

enum MCExternalHandlerType
{
	kMCExternalHandlerTypeNone,
	kMCExternalHandlerTypeCommand,
	kMCExternalHandlerTypeFunction
};

struct MCExternalHandler
{
	MCExternalHandlerType type;
	const char *name;
	bool (*handler)(MCExternalVariableRef *argv, uint32_t argc, MCExternalVariableRef result);
};

struct MCExternalInfo
{
	uint32_t version;
	uint32_t flags;
	const char *name;
	MCExternalHandler *handlers;
};

struct MCExternalInterface
{
	uint32_t version;

	//////////
	
	MCExternalError (*engine_run_on_main_thread)(void *callback, void *callback_state, MCExternalRunOnMainThreadOptions options);

	//////////

	MCExternalError (*context_query)(MCExternalContextQueryTag op, void *result);

	//////////

	MCExternalError (*variable_create)(MCExternalVariableRef* var);
	MCExternalError (*variable_retain)(MCExternalVariableRef var);
	MCExternalError (*variable_release)(MCExternalVariableRef var);

	MCExternalError (*variable_query)(MCExternalVariableRef var, MCExternalVariableQueryTag tag, void *result);
	MCExternalError (*variable_clear)(MCExternalVariableRef var);
	MCExternalError (*variable_exchange)(MCExternalVariableRef var_a, MCExternalVariableRef var_b);
	
	MCExternalError (*variable_store)(MCExternalVariableRef var, MCExternalValueOptions options, void *value);
	MCExternalError (*variable_fetch)(MCExternalVariableRef var, MCExternalValueOptions options, void *value);
	MCExternalError (*variable_append)(MCExternalVariableRef var, MCExternalValueOptions options, void *value);
	MCExternalError (*variable_prepend)(MCExternalVariableRef var, MCExternalValueOptions options, void *value);

    // V6-TODO: This method was never exposed / used so is now unimplemented.
	MCExternalError (*variable_edit)(MCExternalVariableRef var, MCExternalValueOptions options, uint32_t reserve_length, void **r_buffer, uint32_t *r_length);

    // V6-TODO: These methods are not valid for V6, the array interface needs rethinking at a later
    //   date. The only use for them at the moment is in the lcidl support layer for converting between
    //   obj-c and LiveCode arrays / dictionaries. If we move this ability into the variable_fetch/store
    //   methods we don't have to give external access to arrays right now.
	MCExternalError (*variable_count_keys)(MCExternalVariableRef var, uint32_t* r_count);
	MCExternalError (*variable_iterate_keys)(MCExternalVariableRef var, MCExternalVariableIteratorRef *iterator, MCExternalValueOptions options, void *key, MCExternalVariableRef *r_value);
	MCExternalError (*variable_remove_key)(MCExternalVariableRef var, MCExternalValueOptions options, void *key);
	MCExternalError (*variable_lookup_key)(MCExternalVariableRef var, MCExternalValueOptions options, void *key, bool ensure, MCExternalVariableRef *r_var);

	/////////

	MCExternalError (*object_resolve)(const char *chunk, MCExternalObjectRef* r_object);
	MCExternalError (*object_exists)(MCExternalObjectRef object, bool *r_exists);
	MCExternalError (*object_retain)(MCExternalObjectRef object);
	MCExternalError (*object_release)(MCExternalObjectRef object);
	MCExternalError (*object_dispatch)(MCExternalObjectRef object, MCExternalDispatchType type, const char *message, MCExternalVariableRef *argv, uint32_t argc, MCExternalDispatchStatus *r_status);
	
	/////////
	
	MCExternalError (*wait_run)(void *unused, unsigned int options); // V2, changed in V3
	MCExternalError (*wait_break)(void *unused, unsigned int options); // V2, changed in V3
	
	/////////
	
	MCExternalError (*object_get)(MCExternalObjectRef object, unsigned int options, const char *name, const char *key, MCExternalVariableRef value); // V3
	MCExternalError (*object_set)(MCExternalObjectRef object, unsigned int options, const char *name, const char *key, MCExternalVariableRef value); // V3
	
	/////////
	
	MCExternalError (*interface_query)(MCExternalInterfaceQueryTag op, void *result); // V3
	
	/////////
	
	MCExternalError (*object_update)(MCExternalObjectRef object, unsigned int options, void *region); // V3
	
	/////////
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Method to evaluate the given expression
	//   in the current context.
	// MW-2013-06-21: [[ ExternalsApiV5 ]] Added binds parameters for future extension.
	MCExternalError (*context_evaluate)(const char *p_expression, unsigned int options, MCExternalVariableRef *binds, unsigned int bind_count, MCExternalVariableRef result); // V5
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Method to execute the given expression
	//   in the current context.
	// MW-2013-06-21: [[ ExternalsApiV5 ]] Added binds parameters for future extension.
	MCExternalError (*context_execute)(const char *p_expression, unsigned int options, MCExternalVariableRef *binds, unsigned int bind_count); // V5
};

typedef MCExternalInfo *(*MCExternalDescribeProc)(void);
typedef bool (*MCExternalInitializeProc)(const MCExternalInterface *intf);
typedef void (*MCExternalFinalizeProc)(void);

extern MCExternalInterface g_external_interface;

extern MCExecPoint *MCEPptr;

////////////////////////////////////////////////////////////////////////////////

// MW-2014-01-22: [[ CompatV1 ]] Shim classes that emulate the old MCVariableValue
//   semantics using the new MCValueRef imp.
class MCExternalVariable
{
public:
	MCExternalVariable(void);
	virtual ~MCExternalVariable(void);
	
	uint32_t GetReferenceCount(void);
	void Retain(void);
	void Release(void);
	
	Value_format GetFormat(void);
	
	MCExternalError Set(MCExternalVariable *other);
	MCExternalError SetBoolean(bool value);
	MCExternalError SetInteger(int32_t value);
	MCExternalError SetCardinal(uint32_t value);
	MCExternalError SetReal(real64_t value);
    // SN-2014-07-01 [[ ExternalsApiV6 ]] Default string in use is now a StringRef
	MCExternalError SetString(MCStringRef string);
	MCExternalError SetCString(const char *cstring);
	
	MCExternalError Append(MCExternalValueOptions options, MCExternalVariable *other);
	MCExternalError AppendBoolean(MCExternalValueOptions options, bool value);
	MCExternalError AppendInteger(MCExternalValueOptions options, int32_t value);
	MCExternalError AppendCardinal(MCExternalValueOptions options, uint32_t value);
	MCExternalError AppendReal(MCExternalValueOptions options, real64_t value);
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Default string in use is now a StringRef
	MCExternalError AppendString(MCExternalValueOptions options, MCStringRef string);
	MCExternalError AppendCString(MCExternalValueOptions options, const char *cstring);
	
	MCExternalError GetBoolean(MCExternalValueOptions options, bool& r_value);
	MCExternalError GetInteger(MCExternalValueOptions options, int32_t& r_value);
	MCExternalError GetCardinal(MCExternalValueOptions options, uint32_t& r_value);
	MCExternalError GetReal(MCExternalValueOptions options, real64_t& r_value);
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Default string in use is now a StringRef
    //  New function added to get the CData - allowing nil bytes in the string
	MCExternalError GetString(MCExternalValueOptions options, MCStringRef& r_string);
    MCExternalError GetCData(MCExternalValueOptions options, void* r_data);
	MCExternalError GetCString(MCExternalValueOptions options, const char*& r_cstring);
    
	virtual bool IsTemporary(void) = 0;
	virtual bool IsTransient(void) = 0;
	
	virtual MCValueRef GetValueRef(void) = 0;
	virtual void SetValueRef(MCValueRef value) = 0;
	
private:
	uint32_t m_references;
	char* m_string_conversion;
};

class MCTransientExternalVariable: public MCExternalVariable
{
public:
	MCTransientExternalVariable(MCValueRef value);
	~MCTransientExternalVariable(void);
	
	virtual bool IsTemporary(void);
	virtual bool IsTransient(void);
	virtual MCValueRef GetValueRef(void);
	virtual void SetValueRef(MCValueRef value);
	
private:
	MCValueRef m_value;
};

class MCTemporaryExternalVariable: public MCTransientExternalVariable
{
public:
	MCTemporaryExternalVariable(MCValueRef value);
	
	virtual bool IsTemporary(void);
};

class MCReferenceExternalVariable: public MCExternalVariable
{
public:
	MCReferenceExternalVariable(MCVariable *value);
	~MCReferenceExternalVariable(void);
	
	virtual bool IsTemporary(void);
	virtual bool IsTransient(void);
	virtual MCValueRef GetValueRef(void);
	virtual void SetValueRef(MCValueRef value);
	
private:
	MCVariable *m_variable;
};

// MW-2014-01-22: [[ CompatV1 ]] This global holds the current handlers it-shim.
static MCReferenceExternalVariable *s_external_v1_current_it = nil;
// MW-2014-01-22: [[ CompatV1 ]] This global holds the result-shim.
static MCReferenceExternalVariable *s_external_v1_result = nil;

////////////////////////////////////////////////////////////////////////////////

class MCExternalV1: public MCExternal
{
public:
	MCExternalV1(void);
	virtual ~MCExternalV1(void);

	virtual const char *GetName(void) const;
	virtual Handler_type GetHandlerType(uint32_t index) const;
	virtual bool ListHandlers(MCExternalListHandlersCallback callback, void *state);
	virtual Exec_stat Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters);

private:
	virtual bool Prepare(void);
	virtual bool Initialize(void);
	virtual void Finalize(void);

	MCExternalInfo *m_info;
};

////////////////////////////////////////////////////////////////////////////////

static bool number_to_string(double p_number, MCExternalValueOptions p_options, MCStringRef &r_buffer)
{
    bool t_success;
    t_success = false;
	switch(p_options & kMCExternalValueOptionNumberFormatMask)
	{
		case kMCExternalValueOptionDefaultNumberFormat:
            t_success = MCU_r8tos(p_number, MCECptr -> GetNumberFormatWidth(), MCECptr -> GetNumberFormatTrailing(), MCECptr -> GetNumberFormatForce(), r_buffer);
			break;
		case kMCExternalValueOptionDecimalNumberFormat:
			t_success = MCStringFormat(r_buffer, "%f", p_number);
			break;
		case kMCExternalValueOptionScientificNumberFormat:
			t_success = MCStringFormat(r_buffer, "%e", p_number);
			break;
		case kMCExternalValueOptionCompactNumberFormat:
			t_success = MCStringFormat(r_buffer, "%0.16g", p_number);
			break;
	}
    
    return t_success;
}

static bool options_get_convert_octals(MCExternalValueOptions p_options)
{
	switch(p_options & kMCExternalValueOptionConvertOctalsMask)
	{
		case kMCExternalValueOptionDefaultConvertOctals:
			return MCECptr -> GetConvertOctals();
		case kMCExternalValueOptionConvertOctals:
			return true;
		case kMCExternalValueOptionDoNotConvertOctals:
			return false;
		default:
			break;
	}
	return false;
}

static MCExternalError string_to_boolean(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	if (MCStringIsEqualTo(p_string, kMCTrueString, kMCStringOptionCompareCaseless))
		*(bool *)r_value = true;
	else if (MCStringIsEqualTo(p_string, kMCFalseString, kMCStringOptionCompareCaseless))
		*(bool *)r_value = false;
	else
		return kMCExternalErrorNotABoolean;
	
	return kMCExternalErrorNone;
}

static MCExternalError string_to_integer(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
	uint32_t l;
    MCAutoPointer<char> t_chars;
    
	/* UNCHECKED */ MCStringConvertToNative(p_string, (char_t*&)&t_chars, l);
    s = (const char*)*t_chars;
    
	// Skip any whitespace before the number.
	MCU_skip_spaces(s, l);
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Check to see if we have a sign.
	bool t_negative;
	t_negative = false;
	if (*s == '-' || *s == '+')
	{
		t_negative = (*s == '-');
		s++;
		l--;
	}
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Check to see what base we are using.
	uint32_t t_base;
	t_base = 10;
	if (*s == '0')
	{
		if (l >= 2 && (s[1] == 'X' || s[1] == 'x'))
		{
			t_base = 16;
			s += 2;
			l -= 2;
		}
		else if (options_get_convert_octals(p_options))
		{
			t_base = 8;
			s += 1;
			l -= 1;
		}
	}
	
	uint32_t t_value;
	t_value = 0;
	
	bool t_is_integer;
	t_is_integer = true;
	
	if (t_base == 10)
	{
		uint32_t sl;
		sl = l;
		while(l != 0 && isdigit(*s))
		{
			uint32_t t_new_value;
			t_new_value = t_value * 10 + (*s - '0');
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
			
			s += 1;
			l -= 1;
		}
		
		if (l != 0 && *s == '.')
		{
			if (sl <= 1)
				return kMCExternalErrorNotANumber;
			
			do
			{
				s += 1;
				l -= 1;
				if (l != 0 && *s != '0')
					t_is_integer = false;
			}
			while(l != 0 && isdigit(*s));
		}
	}
	else if (t_base == 16)
	{
		while(l != 0 && isxdigit(*s))
		{
			uint32_t t_new_value;
			if (isdigit(*s))
				t_new_value = t_value * 16 + (*s - '0');
			else
				t_new_value = t_value * 16 + (((*s) & ~32) - 'A');
			
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
			
			s += 1;
			l -= 1;
		}
	}
	else
	{
		while(l != 0 && isdigit(*s) && *s < '8')
		{
			uint32_t t_new_value;
			t_new_value = t_value * 8 + (*s - '0');
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
		}
	}
	
	MCU_skip_spaces(s, l);
	if (l != 0)
		return kMCExternalErrorNotANumber;
	
	if (!t_is_integer)
		return kMCExternalErrorNotAnInteger;
	
	if ((p_options & 0xf) == kMCExternalValueOptionAsInteger)
	{
		int32_t t_as_value;
		if (t_negative)
		{
			if (t_value > 0x80000000U)
				return kMCExternalErrorNumericOverflow;
			t_as_value = -(int32_t)t_value;
		}
		else
		{
			if (t_value > MAXINT4)
				return kMCExternalErrorNumericOverflow;
			t_as_value = t_value;
		}
		*(int32_t *)r_value = t_as_value;
	}
	else
	{
		if (t_negative)
			return kMCExternalErrorNumericOverflow;
		*(uint32_t *)r_value = t_value;
	}
	
	return kMCExternalErrorNone;
}

static MCExternalError string_to_real(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
    MCAutoPointer<char> t_chars;
	uint32_t l;
    uint32_t t_length;
    
	/* UNCHECKED */ MCStringConvertToNative(p_string, (char_t*&)&t_chars, l);
    t_length = l;
    s = (const char*)*t_chars;
	
	// Skip space before the number.
	MCU_skip_spaces(s, l);
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// See if the number is negative.
	bool t_negative;
	t_negative = false;
	if (*s == '-' || *s == '+')
	{
		t_negative = (*s == '-');
		s++;
		l--;
	}
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Now see if it has to be interpreted as an integer (0x or 0 prefix).
	if (*s == '0' &&
		(l >= 2 && (s[1] == 'X' || s[1] == 'x')) ||
		options_get_convert_octals(p_options))
	{
		MCExternalError t_error;
        MCAutoStringRef t_substring;
        /* UNCHECKED */ MCStringCopySubstring(p_string, MCRangeMake(t_length - l, l), &t_substring);
		uint32_t t_value;
		t_error = string_to_integer(*t_substring, (p_options & ~0xf) | kMCExternalValueOptionAsCardinal, &t_value);
		if (t_error != kMCExternalErrorNone)
			return t_error;
		
		*(double *)r_value = !t_negative ? (double)t_value : -(double)t_value;
		return kMCExternalErrorNone;
	}
	
	// Otherwise we convert as a double - note that we need a NUL terminated
	// string here so temporarily copy into a buffer...
	char t_tmp_s[R8L];
	uint32_t t_tmp_l;
	t_tmp_l = MCU_min(R8L - 1U, l);
	memcpy(t_tmp_s, s, t_tmp_l);
	t_tmp_s[t_tmp_l] = '\0';
	
	double t_value;
	char *t_tmp_end;
	t_value = strtod(t_tmp_s, &t_tmp_end);
	
	s += t_tmp_end - t_tmp_s;
	l -= t_tmp_end - t_tmp_s;
	MCU_skip_spaces(s, l);
	
	if (l != 0)
		return kMCExternalErrorNotANumber;
	
	*(double *)r_value = !t_negative ? t_value : -t_value;
	
	return kMCExternalErrorNone;
}

static MCExternalError number_to_integer(double p_number, MCExternalValueOptions p_options, void *r_value)
{
	bool t_negative;
	if (p_number >= 0.0)
		t_negative = false;
	else
	{
		t_negative = true;
		p_number = -p_number;
	}
	
	double t_integer, t_fraction;
	t_fraction = modf(p_number, &t_integer);
	
	uint32_t t_value;
	if (t_fraction < MC_EPSILON)
		t_value = (uint32_t)t_integer;
	else if ((1.0 - t_fraction) < MC_EPSILON)
		t_value = (uint32_t)t_integer + 1;
	else
		return kMCExternalErrorNotAnInteger;
	
	if ((p_options & 0xf) == kMCExternalValueOptionAsInteger)
	{
		int32_t t_as_value;
		if (t_negative)
		{
			if (t_value > 0x80000000U)
				return kMCExternalErrorNumericOverflow;
			t_as_value = -(int32_t)t_value;
		}
		else
		{
			if (t_value > MAXINT4)
				return kMCExternalErrorNumericOverflow;
			t_as_value = t_value;
		}
		*(int32_t *)r_value = t_as_value;
	}
	else
	{
		if (t_negative)
			return kMCExternalErrorNumericOverflow;
		*(uint32_t *)r_value = t_value;
	}
	
	return kMCExternalErrorNone;
}

static MCExternalError number_to_real(double p_number, MCExternalValueOptions p_options, void *r_value)
{
	*(double *)r_value = p_number;
	return kMCExternalErrorNone;
}

#ifdef __HAS_CORE_FOUNDATION__

#import <Foundation/Foundation.h>

// MW-2013-06-14: [[ ExternalsApiV5 ]] New methods to convert to/from objc-arrays
//   and dictionaries.
static MCExternalError MCExternalValueArrayToObjcArray(MCExternalVariableRef src, NSArray*& r_dst);
static MCExternalError MCExternalValueArrayFromObjcArray(MCExternalVariableRef src, NSArray* r_dst);
static MCExternalError MCExternalValueArrayToObjcDictionary(MCExternalVariableRef src, NSDictionary*& r_dst);
static MCExternalError MCExternalValueArrayFromObjcDictionary(MCExternalVariableRef src, NSDictionary* r_dst);

#endif

////////////////////////////////////////////////////////////////////////////////

MCExternalVariable::MCExternalVariable(void)
{
	m_references = 1;
	m_string_conversion = nil;
}

MCExternalVariable::~MCExternalVariable(void)
{
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
}

uint32_t MCExternalVariable::GetReferenceCount(void)
{
	return m_references;
}

void MCExternalVariable::Retain(void)
{
	m_references += 1;
}

void MCExternalVariable::Release(void)
{
	m_references -= 1;
	if (m_references > 0)
		return;
	
	delete this;
}

Value_format MCExternalVariable::GetFormat(void)
{
	switch(MCValueGetTypeCode(GetValueRef()))
	{
		case kMCValueTypeCodeNull:
			return VF_STRING;
		case kMCValueTypeCodeBoolean:
			return VF_STRING;
		case kMCValueTypeCodeNumber:
			return VF_NUMBER;
		case kMCValueTypeCodeName:
			return VF_STRING;
		case kMCValueTypeCodeString:
			return VF_STRING;
		case kMCValueTypeCodeData:
			return VF_STRING;
		case kMCValueTypeCodeArray:
			return VF_ARRAY;
		default:
			assert(false);
	}
	
	return VF_UNDEFINED;
}

MCExternalError MCExternalVariable::Set(MCExternalVariable *p_other)
{
	SetValueRef(p_other -> GetValueRef());
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetBoolean(bool p_value)
{
	SetValueRef(p_value ? kMCTrue : kMCFalse);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetInteger(int32_t p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithInteger(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetCardinal(uint32_t p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithUnsignedInteger(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetReal(double p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithReal(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

// SN-2014-07-01 [[ ExternalsApiV6 ]] Update to use a stringRef
MCExternalError MCExternalVariable::SetString(MCStringRef p_value)
{
	SetValueRef(p_value);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetCString(const char *p_value)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithCString(p_value, &t_string))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_string);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::Append(MCExternalValueOptions p_options, MCExternalVariable *p_value)
{
	MCExternalError t_error;
	MCAutoStringRef t_string;
	t_error = p_value -> GetString(p_options, &t_string);
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendBoolean(MCExternalValueOptions p_options, bool p_value)
{
	return AppendString(p_options, p_value ? kMCTrueString : kMCFalseString);
}

MCExternalError MCExternalVariable::AppendInteger(MCExternalValueOptions p_options, int32_t p_value)
{
    MCAutoStringRef t_string;
    
    if (!MCStringFormat(&t_string, "%d", p_value))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendCardinal(MCExternalValueOptions p_options, uint32_t p_value)
{
    MCAutoStringRef t_string;
	if (!MCStringFormat(&t_string, "%u", p_value))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendReal(MCExternalValueOptions p_options, real64_t p_value)
{
    MCAutoStringRef t_string;
	if (!number_to_string(p_value, p_options, &t_string))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendString(MCExternalValueOptions p_options, MCStringRef p_value)
{
	MCExternalError t_error;
	MCAutoStringRef t_current_value;
	t_error = GetString(p_options, &t_current_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	
	MCAutoStringRef t_new_value;
	if (!MCStringFormat(&t_new_value, "%@%@", *t_current_value, p_value))
		return kMCExternalErrorOutOfMemory;
	
	SetValueRef(*t_new_value);	
	return t_error;
}

MCExternalError MCExternalVariable::AppendCString(MCExternalValueOptions p_options, const char *p_value)
{
    MCAutoStringRef t_string;
    if (!MCStringCreateWithCString(p_value, &t_string))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::GetBoolean(MCExternalValueOptions p_options, bool& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
		r_value = t_value == kMCTrue;
	else
	{
		MCExternalError t_error;
		MCAutoStringRef t_value;
		t_error = GetString(p_options, &t_value);
		if (t_error != kMCExternalErrorNone)
			return t_error;
		
		return string_to_boolean(*t_value, p_options, &r_value);
	}
	
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::GetInteger(MCExternalValueOptions p_options, int32_t& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber)
		return number_to_integer(MCNumberFetchAsReal((MCNumberRef)t_value), p_options, &r_value);
	
	MCExternalError t_error;
	MCAutoStringRef t_string_value;
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	
	return string_to_integer(*t_string_value, p_options, &r_value);
}

MCExternalError MCExternalVariable::GetCardinal(MCExternalValueOptions p_options, uint32_t& r_value)
{
	return GetInteger(p_options, (int32_t&)r_value);
}

MCExternalError MCExternalVariable::GetReal(MCExternalValueOptions p_options, real64_t& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber)
		return number_to_real(p_options, MCNumberFetchAsReal((MCNumberRef)t_value), &r_value);
	
	MCExternalError t_error;
	MCAutoStringRef t_string_value;
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	
	return string_to_real(*t_string_value, p_options, &r_value);
}

MCExternalError MCExternalVariable::GetString(MCExternalValueOptions p_options, MCStringRef& r_value)
{
    MCAutoStringRef t_string_value;
	
	// Get the valueref.
	MCValueRef t_value;
	t_value = GetValueRef();
	
    // Avoid null pointer dereferences in externals
    if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNull)
        t_value = kMCEmptyString;

	switch(MCValueGetTypeCode(t_value))
	{
		case kMCValueTypeCodeBoolean:
			t_string_value = (t_value == kMCTrue ? kMCTrueString : kMCFalseString);
			break;
		case kMCValueTypeCodeNumber:
		{
			// Use the externalv1 method to convert.
			double t_number;
			t_number = MCNumberFetchAsReal((MCNumberRef)t_value);
			
			if (!number_to_string(t_number, p_options, &t_string_value))
                return kMCExternalErrorOutOfMemory;
		}
			break;
		case kMCValueTypeCodeName:
        {
            t_string_value = MCNameGetString((MCNameRef)t_value);
		}
			break;
		case kMCValueTypeCodeString:
		{
			t_string_value = (MCStringRef)t_value;
		}
			break;
		case kMCValueTypeCodeData:
		{
            MCDataRef t_dataref;
            t_dataref = (MCDataRef)t_value;
            if (!MCStringCreateWithBytes(MCDataGetBytePtr(t_dataref), MCDataGetLength(t_dataref), kMCStringEncodingNative, false, &t_string_value))
                return kMCExternalErrorOutOfMemory;
		}
			break;
		case kMCValueTypeCodeArray:
			// An array is never a string (from the point of view of the externals API).
			return kMCExternalErrorNotAString;
			break;
		default:
			assert(false);
	}
	
	r_value = MCValueRetain(*t_string_value);
	
	return kMCExternalErrorNone;
}

// SN-2014-07-16: [[ ExternalsApiV6 ]] Function to get the CData type - allowing nil bytes in the string
MCExternalError MCExternalVariable::GetCData(MCExternalValueOptions p_options, void *r_value)
{
	MCAutoStringRef t_string_value;
	MCExternalError t_error;
    MCString t_string;
    uindex_t t_length;
    
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
    
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
	
    if (!MCStringNormalizeAndConvertToNative(*t_string_value, (char_t*&)m_string_conversion, t_length))
        return kMCExternalErrorOutOfMemory;
	
	t_string . set(m_string_conversion, t_length);
    *(MCString*)r_value = t_string;
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::GetCString(MCExternalValueOptions p_options, const char*& r_value)
{
	MCAutoStringRef t_string_value;
	MCExternalError t_error;
    uindex_t t_length;
    
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
    
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
	
    if (!MCStringNormalizeAndConvertToNative(*t_string_value, (char_t*&)m_string_conversion, t_length))
        return kMCExternalErrorOutOfMemory;
	
	if (memchr(m_string_conversion, '\0', t_length) != nil)
    {
        MCMemoryDeleteArray(m_string_conversion);
        m_string_conversion = nil;
		return kMCExternalErrorNotACString;
    }
	
	r_value = m_string_conversion;
	return kMCExternalErrorNone;
}

//////////

MCTransientExternalVariable::MCTransientExternalVariable(MCValueRef p_value)
{
	m_value = MCValueRetain(p_value);
}

MCTransientExternalVariable::~MCTransientExternalVariable(void)
{
	MCValueRelease(m_value);
}

bool MCTransientExternalVariable::IsTemporary(void)
{
	return false;
}

bool MCTransientExternalVariable::IsTransient(void)
{
	return true;
}

MCValueRef MCTransientExternalVariable::GetValueRef(void)
{
	return m_value;
}

void MCTransientExternalVariable::SetValueRef(MCValueRef p_value)
{
	MCValueRetain(p_value);
	MCValueRelease(m_value);
	m_value = p_value;
}

//////////

MCTemporaryExternalVariable::MCTemporaryExternalVariable(MCValueRef p_value)
	: MCTransientExternalVariable(p_value)
{
}

bool MCTemporaryExternalVariable::IsTemporary(void)
{
	return true;
}

//////////

MCReferenceExternalVariable::MCReferenceExternalVariable(MCVariable *p_variable)
{
	m_variable = p_variable;
}

MCReferenceExternalVariable::~MCReferenceExternalVariable(void)
{
}

bool MCReferenceExternalVariable::IsTemporary(void)
{
	return false;
}

bool MCReferenceExternalVariable::IsTransient(void)
{
	return false;
}

MCValueRef MCReferenceExternalVariable::GetValueRef(void)
{
	return m_variable -> getvalueref();
}

void MCReferenceExternalVariable::SetValueRef(MCValueRef p_value)
{
	m_variable -> setvalueref(p_value);
}

////////////////////////////////////////////////////////////////////////////////

MCExternalV1::MCExternalV1(void)
{
	m_info = nil;
}

MCExternalV1::~MCExternalV1(void)
{
}

bool MCExternalV1::Prepare(void)
{
	// Fetch the description callback - this symbol has to exist since a V1
	// external is only created if it does!
	MCExternalDescribeProc t_describe;
	t_describe = (MCExternalDescribeProc)MCS_resolvemodulesymbol(m_module, MCSTR("MCExternalDescribe"));
	
	// Query the info record - if this returns nil something odd is going on!
	m_info = t_describe();
	if (m_info == nil)
		return false;

	return true;
}

bool MCExternalV1::Initialize(void)
{
	// Fetch the initialize entry point.
	MCExternalInitializeProc t_initialize;
	t_initialize = (MCExternalInitializeProc)MCS_resolvemodulesymbol(m_module, MCSTR("MCExternalInitialize"));
	if (t_initialize == nil)
		return true;

	// See if initialization succeeds.
	if (!t_initialize(&g_external_interface))
		return false;

	return true;
}

void MCExternalV1::Finalize(void)
{
	// Fetch the finalize entry point.
	MCExternalFinalizeProc t_finalize;
	t_finalize = (MCExternalFinalizeProc)MCS_resolvemodulesymbol(m_module, MCSTR("MCExternalFinalize"));
	if (t_finalize == nil)
		return;

	t_finalize();
}

const char *MCExternalV1::GetName(void) const
{
	return m_info -> name;
}

Handler_type MCExternalV1::GetHandlerType(uint32_t p_index) const
{
	if (m_info -> handlers[p_index] . type == kMCExternalHandlerTypeCommand)
		return HT_MESSAGE;
	return HT_FUNCTION;
		}

bool MCExternalV1::ListHandlers(MCExternalListHandlersCallback p_callback, void *p_state)
{
	for(uint32_t i = 0; m_info -> handlers[i] . type != kMCExternalHandlerTypeNone; i++)
		if (!p_callback(p_state, m_info -> handlers[i] . type == kMCExternalHandlerTypeCommand ? HT_MESSAGE : HT_FUNCTION, m_info -> handlers[i] . name, i))
			return false;

	return true;
}

Exec_stat MCExternalV1::Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters)
{
	MCExternalHandlerType t_type;
	if (p_type == HT_FUNCTION)
		t_type = kMCExternalHandlerTypeFunction;
		else
		t_type = kMCExternalHandlerTypeCommand;

	MCExternalHandler *t_handler;
	t_handler = &m_info -> handlers[p_index];
	if (t_handler -> type != t_type)
		return ES_NOT_HANDLED;

		Exec_stat t_stat;
		t_stat = ES_NORMAL;

		// Count the number of parameters.
		uint32_t t_parameter_count;
		t_parameter_count = 0;
		for(MCParameter *t_param = p_parameters; t_param != nil; t_param = t_param -> getnext())
			t_parameter_count += 1;
			
		// Allocate an array of values.
		MCExternalVariableRef *t_parameter_vars;
		t_parameter_vars = NULL;
		if (t_parameter_count != 0)
		{
			t_parameter_vars = new MCExternalVariableRef[t_parameter_count];
			if (t_parameter_vars == nil)
				return ES_ERROR;
		}
		
		// Now iterate through the parameters, fetching the parameter values as we go.
		for(uint32_t i = 0; p_parameters != nil && t_stat == ES_NORMAL; i++, p_parameters = p_parameters -> getnext())
		{
			MCVariable* t_var;
			t_var = p_parameters -> eval_argument_var();
			if (t_var != nil)
			{
				// We are a variable parameter so use the value directly (i.e. pass by
				// reference).
				
				// MW-2014-01-22: [[ CompatV1 ]] Create a reference to the MCVariable.
				t_parameter_vars[i] = new MCReferenceExternalVariable(t_var);
			}
			else
			{
                // AL-2014-08-28: [[ ArrayElementRefParams ]] Evaluate container if necessary
                MCAutoValueRef t_value;
                MCContainer *t_container;
                t_container = p_parameters -> eval_argument_container();
                
                if (t_container != nil)
                {
                    MCNameRef *t_path;
                    uindex_t t_length;
                    t_container -> getpath(t_path, t_length);
                    
                    MCExecContext ctxt(p_context, nil, nil);
                    
                    if (t_length == 0)
                        t_parameter_vars[i] = new MCReferenceExternalVariable(t_container -> getvar());
                    else
                        t_container -> eval(ctxt, &t_value);
                }
                else
                    t_value = p_parameters -> getvalueref_argument();
                
				// MW-2014-01-22: [[ CompatV1 ]] Create a temporary value var.
                if (*t_value != nil)
                    t_parameter_vars[i] = new MCTransientExternalVariable(*t_value);
			}
		}

		// We have our list of parameters (hopefully), so now call - passing a temporary
		// result.
		if (t_stat == ES_NORMAL)
		{
			// MW-2014-01-22: [[ CompatV1 ]] Initialize a temporary var to empty.
			MCTransientExternalVariable t_result(kMCEmptyString);
			
			// MW-2014-01-22: [[ CompatV1 ]] Make a reference var to hold it (should it be needed).
			//   We then store the previous global value of the it extvar, and set this one.
			//   As external calls are recursive, this should be fine :)
			MCReferenceExternalVariable t_it(MCECptr -> GetHandler() -> getit() -> evalvar(*MCECptr));
			MCReferenceExternalVariable *t_old_it;
			t_old_it = s_external_v1_current_it;
			s_external_v1_current_it = &t_it;
			
			// Invoke the external handler. If 'false' is returned, treat the result as a
			// string value containing an error hint.
			if ((t_handler -> handler)(t_parameter_vars, t_parameter_count, &t_result))
				MCresult -> setvalueref(t_result . GetValueRef());
			else
			{
				MCeerror -> add(EE_EXTERNAL_EXCEPTION, 0, 0, t_result . GetValueRef());
				t_stat = ES_ERROR;
			}
			
			// Restore the old it.
			s_external_v1_current_it = t_old_it;
		}

		// Finally, loop through and free the parameters as necessary.
		for(uint32_t i = 0; i < t_parameter_count; i++)
			delete t_parameter_vars[i];

		delete t_parameter_vars;

		return t_stat;
	}

////////////////////////////////////////////////////////////////////////////////

MCExternal *MCExternalCreateV1(void)
{
	return new MCExternalV1;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_SUBPLATFORM_IPHONE)
extern bool iphone_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
	if (!iphone_run_on_main_thread(p_callback, p_callback_state, p_options))
		return kMCExternalErrorNotImplemented;
	return kMCExternalErrorNone;
}
#elif defined(TARGET_SUBPLATFORM_ANDROID)
extern bool android_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
	if (!android_run_on_main_thread(p_callback, p_callback_state, p_options))
		return kMCExternalErrorNotImplemented;
	return kMCExternalErrorNone;
}
#else
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
#if defined(_DESKTOP)
    // MW-2014-10-30: [[ Bug 13875 ]] If either 'JumpTo' flag is specified, we just execute the callback direct.
    if ((p_options & kMCExternalRunOnMainThreadJumpTo) != 0)
    {
        // The 'JumpTo' option cannot have any other flags set.
        if ((p_options & ~kMCExternalRunOnMainThreadJumpTo) != 0)
            return kMCExternalErrorNotImplemented;
        
        ((MCExternalThreadOptionalCallback)p_callback)(p_callback_state);
        return kMCExternalErrorNone;
    }
    
	// MW-2013-06-25: [[ DesktopPingWait ]] Pass the correct parameters through
	//   to MCNotifyPush so that LCObjectPost works.
    // MW-2014-10-23: [[ Bug 13721 ]] Correctly compute the notify flags to pass - in particular
    //   compute the 'required' flag and pass that as that determines the signature of the
    //   callback.
    bool t_block, t_safe, t_required;
    t_block = (p_options & kMCExternalRunOnMainThreadPost) == kMCExternalRunOnMainThreadSend;
    t_safe = (p_options & kMCExternalRunOnMainThreadUnsafe) == kMCExternalRunOnMainThreadSafe;
    t_required = (p_options & kMCExternalRunOnMainThreadRequired) == kMCExternalRunOnMainThreadRequired;
    
    // MW-2014-10-30: [[ Bug 13875 ]] Make sure we return an appropriate error for invalid combinations of flags.
    if (t_block && t_safe)
        return kMCExternalErrorNotImplemented;
    
	if (!MCNotifyPush((MCExternalThreadOptionalCallback)p_callback, p_callback_state, t_block, t_safe, t_required))
		return kMCExternalErrorOutOfMemory;

	return kMCExternalErrorNone;
#else
	return kMCExternalErrorNotImplemented;
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalContextQuery(MCExternalContextQueryTag op, void *result)
{
	switch(op)
	{
	case kMCExternalContextQueryMe:
		{
			MCObjectHandle *t_handle;
			t_handle = MCECptr -> GetObject() -> gethandle();
			if (t_handle == nil)
				return kMCExternalErrorOutOfMemory;
			*(MCObjectHandle **)result = t_handle;
		}
		break;
	case kMCExternalContextQueryTarget:
		{
			MCObjectHandle *t_handle;
			t_handle = MCtargetptr -> gethandle();
			if (t_handle == nil)
				return kMCExternalErrorOutOfMemory;
			*(MCObjectHandle **)result = t_handle;
		}
		break;
	case kMCExternalContextQueryResult:
		// MW-2014-01-22: [[ CompatV1 ]] If the result shim hasn't been made, make it.
		if (s_external_v1_result == nil)
			s_external_v1_result = new MCReferenceExternalVariable(MCresult);
		*(MCExternalVariableRef *)result = s_external_v1_result;
		break;
	case kMCExternalContextQueryIt:
		{
			// MW-2014-01-22: [[ CompatV1 ]] Use the current it shim (initialized before
			//   a new handler is invoked).
			*(MCExternalVariableRef *)result = s_external_v1_current_it;
		}
		break;
	case kMCExternalContextQueryCaseSensitive:
		*(bool *)result = MCECptr -> GetCaseSensitive();
		break;
	case kMCExternalContextQueryConvertOctals:
		*(bool *)result = MCECptr -> GetConvertOctals();
		break;
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of 'the wholeMatches' query.
	case kMCExternalContextQueryWholeMatches:
		*(bool *)result = MCECptr -> GetWholeMatches();
		break;
	case kMCExternalContextQueryItemDelimiter:
        *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetItemDelimiter(), 0);
		break;
	case kMCExternalContextQueryLineDelimiter:
        *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetLineDelimiter(), 0);
		break;
	case kMCExternalContextQueryColumnDelimiter:
        *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetColumnDelimiter(), 0);
		break;
	case kMCExternalContextQueryRowDelimiter:
		*(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetRowDelimiter(), 0);
		break;
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Unicode delimiters, UTF-16 encoded
    case kMCExternalContextQueryUnicodeColumnDelimiter:
    {
        uindex_t t_dummy;
        if (!MCStringConvertToUnicode(MCECptr -> GetColumnDelimiter(), *(unichar_t**)result, t_dummy))
            return kMCExternalErrorOutOfMemory;
        break;
    }
    case kMCExternalContextQueryUnicodeItemDelimiter:
    {
        uindex_t t_dummy;
        if (!MCStringConvertToUnicode(MCECptr -> GetItemDelimiter(), *(unichar_t**)result, t_dummy))
            return kMCExternalErrorOutOfMemory;
        break;
    }
    case kMCExternalContextQueryUnicodeLineDelimiter:
    {
        uindex_t t_dummy;
        if (!MCStringConvertToUnicode(MCECptr -> GetLineDelimiter(), *(unichar_t**)result, t_dummy))
            return kMCExternalErrorOutOfMemory;
        break;
    }
    case kMCExternalContextQueryUnicodeRowDelimiter:
    {
        uindex_t t_dummy;
        if (!MCStringConvertToUnicode(MCECptr -> GetRowDelimiter(), *(unichar_t**)result, t_dummy))
            return kMCExternalErrorOutOfMemory;
        break;
    }
	case kMCExternalContextQueryDefaultStack:
    {
        if (MCdefaultstackptr == nil)
            return kMCExternalErrorNoDefaultStack;
        
        MCObjectHandle *t_handle;
        t_handle = MCdefaultstackptr -> gethandle();
        if (t_handle == nil)
            return kMCExternalErrorOutOfMemory;
        *(MCObjectHandle **)result = t_handle;
    }
        break;
	case kMCExternalContextQueryDefaultCard:
    {
        if (MCdefaultstackptr == nil)
            return kMCExternalErrorNoDefaultStack;
        
        MCObjectHandle *t_handle;
        t_handle = MCdefaultstackptr -> getcurcard() -> gethandle();
        if (t_handle == nil)
            return kMCExternalErrorOutOfMemory;
        *(MCObjectHandle **)result = t_handle;
    }
		break;
	default:
		return kMCExternalErrorInvalidContextQuery;
	}
	return kMCExternalErrorNone;
}

// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of context_evaluate method.
MCExternalError MCExternalContextEvaluate(const char *p_expression, unsigned int p_options, MCExternalVariableRef *p_binds, unsigned int p_bind_count, MCExternalVariableRef p_result)
{
    MCAutoStringRef t_expr;
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_expression now evaluated as UTF-8
	if (!MCStringCreateWithBytes((byte_t*)p_expression, strlen(p_expression), kMCStringEncodingUTF8, false, &t_expr))
		return kMCExternalErrorOutOfMemory;
	
	MCAutoValueRef t_value;
	MCECptr -> GetHandler() -> eval(*MCECptr, *t_expr, &t_value);
	if (MCECptr -> HasError())
	{	
		if (MCECptr -> GetExecStat() == ES_ERROR)
			return kMCExternalErrorFailed;
	
		if (MCECptr -> GetExecStat() == ES_EXIT_ALL)
			return kMCExternalErrorExited;
	}
	
	p_result -> SetValueRef(*t_value);
	
	return kMCExternalErrorNone;
}

// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of context_execute method.
MCExternalError MCExternalContextExecute(const char *p_commands, unsigned int p_options, MCExternalVariableRef *p_binds, unsigned int p_bind_count)
{
	MCAutoStringRef t_expr;
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_commands now evaluated as UTF-8
	if (!MCStringCreateWithBytes((byte_t*)p_commands, strlen(p_commands), kMCStringEncodingUTF8, false, &t_expr))
		return kMCExternalErrorOutOfMemory;
	
	Exec_stat t_stat;
	MCECptr -> GetHandler() -> doscript(*MCECptr, *t_expr, 0, 0);
	if (MCECptr -> HasError())
	{	
		if (MCECptr -> GetExecStat() == ES_ERROR)
			return kMCExternalErrorFailed;
		
		if (MCECptr -> GetExecStat() == ES_EXIT_ALL)
			return kMCExternalErrorExited;
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalVariableCreate(MCExternalVariableRef* r_var)
{
	*r_var = new MCTemporaryExternalVariable(kMCEmptyString);
	if (r_var == nil)
		return kMCExternalErrorOutOfMemory;

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRetain(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> Retain();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRelease(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> Release();
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableQuery(MCExternalVariableRef var, MCExternalVariableQueryTag p_query, void *r_result)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;
	
	switch(p_query)
	{
	case kMCExternalVariableQueryIsTemporary:
		*(bool *)r_result = var -> IsTemporary();
		break;
	case kMCExternalVariableQueryIsTransient:
		*(bool *)r_result = var -> IsTransient();
		break;
	case kMCExternalVariableQueryFormat:
		*(Value_format *)r_result = var -> GetFormat();
		break;
	case kMCExternalVariableQueryRetention:
		if (!var -> IsTemporary())
			*(uint32_t *)r_result = 0;
		else
			*(uint32_t *)r_result = var -> GetReferenceCount();
		break;
	case kMCExternalVariableQueryIsAnArray:
		*(bool *)r_result = MCValueIsArray(var -> GetValueRef()) || MCValueIsEmpty(var -> GetValueRef());
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsEmpty variable query.
	case kMCExternalVariableQueryIsEmpty:
		*(bool *)r_result = MCValueIsEmpty(var -> GetValueRef());
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsASequence variable query.
	case kMCExternalVariableQueryIsASequence:
			*(bool *)r_result = MCValueIsEmpty(var -> GetValueRef()) || (MCValueIsArray(var -> GetValueRef()) && MCArrayIsSequence((MCArrayRef)var -> GetValueRef()));
		break;

	default:
		return kMCExternalErrorInvalidVariableQuery;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableClear(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> SetValueRef(kMCEmptyString);

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableExchange(MCExternalVariableRef var_a, MCExternalVariableRef var_b)
{
	if (var_a == nil || var_b == nil)
		return kMCExternalErrorNoVariable;

	MCValueRef t_a_value;
	t_a_value = MCValueRetain(var_a -> GetValueRef());
	var_a -> SetValueRef(var_a -> GetValueRef());
	var_b -> SetValueRef(t_a_value);
	MCValueRelease(t_a_value);

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableStore(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;

	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return var -> Set((MCExternalVariableRef)p_value);
	case kMCExternalValueOptionAsBoolean:
		return var -> SetBoolean(*(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> SetInteger(*(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> SetCardinal(*(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
		return var -> SetReal(*(real64_t *)p_value);
	case kMCExternalValueOptionAsString:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((const byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingNative, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsCString:
        return var -> SetCString(*(const char **)p_value);
        
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Storing the new types
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;

        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        
        char* t_string = *(char**)p_value;
        
        if (!MCStringCreateWithBytes(*(byte_t**)p_value, strlen(*(char**)p_value), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), 2 * t_string->getlength(), kMCStringEncodingUTF16, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        uint16_t *t_chars;
        uindex_t t_char_count;
        
        t_chars = *(uint16_t**)p_value;

        for (t_char_count = 0 ; *t_chars != 0; ++t_char_count)
            ++t_chars;
        
        if (!MCStringCreateWithChars(*(const unichar_t**)p_value, t_char_count, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsCFNumber:
    case kMCExternalValueOptionAsNSNumber:
    {
        CFNumberRef t_number;
        t_number = *(CFNumberRef*)p_value;
        
        if (CFNumberIsFloatType(t_number))
        {
            real64_t t_double;
            if (!CFNumberGetValue(t_number, kCFNumberFloat64Type, &t_double))
                return kMCExternalErrorNotANumber;
            else
                return var -> SetReal(t_double);
        }
        else
        {
            int32_t t_integer;
            if (!CFNumberGetValue(t_number, kCFNumberIntType, &t_integer))
                return kMCExternalErrorNotANumber;
            else
                return var -> SetInteger(t_integer);
        }
    }
    case kMCExternalValueOptionAsCFString:
    case kMCExternalValueOptionAsNSString:
    {
        MCAutoStringRef t_string;
        
        if (!MCStringCreateWithCFString(*(CFStringRef*)p_value, &t_string))
            return kMCExternalErrorNotAString;
        
        return var -> SetString(*t_string);
    }
    case kMCExternalValueOptionAsCFData:
    case kMCExternalValueOptionAsNSData:
    {
        MCAutoStringRef t_string;
        CFDataRef t_data;
        t_data = *(CFDataRef*)p_value;
        
        if (!MCStringCreateWithBytes(CFDataGetBytePtr(t_data), CFDataGetLength(t_data), kMCStringEncodingNative, false, &t_string))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_string);
    }
    case kMCExternalValueOptionAsCFArray:
    case kMCExternalValueOptionAsNSArray:
    {
        // For efficiency, we use 'exchange' - this prevents copying a temporary array.
        MCExternalVariableRef t_tmp_array;
        MCExternalError t_error;
        
        t_error = kMCExternalErrorNone;
        t_tmp_array = nil;
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalVariableCreate(&t_tmp_array);
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalValueArrayFromObjcArray(t_tmp_array, *(NSArray **)p_value);
        if (t_error == kMCExternalErrorNone)
            t_error = g_external_interface . variable_exchange(var, t_tmp_array);
        if (t_tmp_array != nil)
            MCExternalVariableRelease(t_tmp_array);
        
        return t_error;
    }
    case kMCExternalValueOptionAsCFDictionary:
    case kMCExternalValueOptionAsNSDictionary:
    {
        // For efficiency, we use 'exchange' - this prevents copying a temporary array.
        MCExternalVariableRef t_tmp_array;
        MCExternalError t_error;
        
        t_error = kMCExternalErrorNone;
        t_tmp_array = nil;
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalVariableCreate(&t_tmp_array);
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalValueArrayFromObjcDictionary(t_tmp_array, *(NSDictionary **)p_value);
        if (t_error == kMCExternalErrorNone)
            t_error = g_external_interface . variable_exchange(var, t_tmp_array);
        if (t_tmp_array != nil)
            MCExternalVariableRelease(t_tmp_array);
        return t_error;
    }
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableFetch(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;
    
    MCExternalError t_error;
    
	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return ((MCExternalVariableRef)p_value) -> Set(var);
	case kMCExternalValueOptionAsBoolean:
		return var -> GetBoolean(p_options, *(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> GetInteger(p_options, *(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> GetCardinal(p_options, *(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
        return var -> GetReal(p_options, *(real64_t *)p_value);
    case kMCExternalValueOptionAsString:
        return var -> GetCData(p_options, p_value);
    case kMCExternalValueOptionAsCString:
        return var -> GetCString(p_options, *(const char **)p_value);
        
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Fetching the new types
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        uindex_t t_length;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUTF8(*t_stringref, t_chars, t_length))
            return kMCExternalErrorOutOfMemory;
        
        ((MCString*)p_value) -> set(t_chars, t_length);
        break;
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUTF8String(*t_stringref, t_chars))
            return kMCExternalErrorOutOfMemory;
        
        (*(char**)p_value) = t_chars;
        break;
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUnicode(*t_stringref, t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        ((MCString*)p_value) -> set((char*)t_chars, t_char_count);
        break;
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUnicode(*t_stringref, t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        (*(unichar_t**)p_value) = t_chars;
        break;
    }
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsCFNumber:
    case kMCExternalValueOptionAsNSNumber:
    {
        CFNumberRef t_number;
        real64_t t_real;
        
        t_error = var -> GetReal(p_options, t_real);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        *(CFNumberRef*)p_value = CFNumberCreate(NULL, kCFNumberFloat64Type, &t_real);
        
        // NS types must be autoreleasing
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSNumber)
            [*(NSNumber**)p_value autorelease];
        
        break;
    }
    case kMCExternalValueOptionAsCFString:
    case kMCExternalValueOptionAsNSString:
    {
        MCAutoStringRef t_stringref;
        
        t_error = var -> GetString(p_options, &t_stringref);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToCFStringRef(*t_stringref, *(CFStringRef*)p_value))
            return kMCExternalErrorOutOfMemory;
        
        // NS types must be autoreleasing
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSString)
            [*(NSString**)p_value autorelease];
        break;
    }
    case kMCExternalValueOptionAsCFData:
    case kMCExternalValueOptionAsNSData:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToNative(*t_stringref, (char_t*&)t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        *(CFDataRef*)p_value = CFDataCreateWithBytesNoCopy(NULL, (UInt8*)t_chars, t_char_count, NULL);
        
        // NS types must be autoreleasing
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSData)
            [*(NSData**)p_value autorelease];
        break;
    }
    case kMCExternalValueOptionAsCFArray:
    case kMCExternalValueOptionAsNSArray:
    {
        MCExternalError t_error;
        NSArray* t_value = *(NSArray**)p_value;
        
        t_error = MCExternalValueArrayToObjcArray(var, t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        // NS types must be autoreleasing
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSArray)
            [*(NSArray**)p_value autorelease];
        return t_error;
    }
    case kMCExternalValueOptionAsCFDictionary:
    case kMCExternalValueOptionAsNSDictionary:
    {
        MCExternalError t_error;
        NSDictionary* t_value = *(NSDictionary**)p_value;
        
        t_error = MCExternalValueArrayToObjcDictionary(var, t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        // NS types must be autoreleasing
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSDictionary)
            [*(NSDictionary**)p_value autorelease];
        
        return t_error;
    }
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableAppend(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;

	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return var -> Append(p_options, (MCExternalVariableRef)p_value);
	case kMCExternalValueOptionAsBoolean:
		return var -> AppendBoolean(p_options, *(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> AppendInteger(p_options, *(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> AppendCardinal(p_options, *(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
		return var -> AppendReal(p_options, *(real64_t *)p_value);
	case kMCExternalValueOptionAsString:
        {
            MCAutoStringRef t_stringref;
            MCString* t_string;
            t_string = (MCString*)p_value;
            if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingNative, false, &t_stringref))
                return kMCExternalErrorOutOfMemory;
            
            return var -> AppendString(p_options, *t_stringref);
        }
	case kMCExternalValueOptionAsCString:
        return var -> AppendCString(p_options, *(const char **)p_value);

    // SN-2014-07-01: [[ ExternalsApiV6 ]] Appending new types (same conversion as in MCExternalVariableStore)
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        
        if (!MCStringCreateWithBytes(*(byte_t**)p_value, strlen(*(char**)p_value), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), 2 * t_string->getlength(), kMCStringEncodingUTF16, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_chars = *(unichar_t**)p_value;
        for (t_char_count = 0 ; *t_chars; ++t_char_count)
            ++t_chars;
        
        if (!MCStringCreateWithChars(*(const unichar_t**)p_value, t_char_count, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsCFNumber:
    case kMCExternalValueOptionAsNSNumber:
    {
        CFNumberRef t_number;
        t_number = *(CFNumberRef*)p_value;
        
        if (CFNumberIsFloatType(t_number))
        {
            real64_t t_double;
            if (!CFNumberGetValue(t_number, kCFNumberFloat64Type, &t_double))
                return kMCExternalErrorNotANumber;
            else
                return var -> AppendReal(p_options, t_double);
        }
        else
        {
            int32_t t_integer;
            if (!CFNumberGetValue(t_number, kCFNumberIntType, &t_integer))
                return kMCExternalErrorNotANumber;
            else
                return var -> AppendInteger(p_options, t_integer);
        }
        break;
    }
    case kMCExternalValueOptionAsCFString:
    case kMCExternalValueOptionAsNSString:
    {
        MCAutoStringRef t_string;
        
        if (!MCStringCreateWithCFString(*(CFStringRef*)p_value, &t_string))
            return kMCExternalErrorNotAString;
        
        return var -> AppendString(p_options, *t_string);
    }
    case kMCExternalValueOptionAsCFData:
    case kMCExternalValueOptionAsNSData:
    {
        MCAutoStringRef t_string;
        CFDataRef t_data;
        t_data = *(CFDataRef*)p_value;
        
        if (!MCStringCreateWithBytes(CFDataGetBytePtr(t_data), CFDataGetLength(t_data), kMCStringEncodingNative, false, &t_string))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_string);
    }
    // SN-2014-07-01: [[ ExternalsApiV6 ]] CFArray and CFDictionary can't be appended.
    case kMCExternalValueOptionAsCFArray:
    case kMCExternalValueOptionAsNSArray:
    case kMCExternalValueOptionAsCFDictionary:
    case kMCExternalValueOptionAsNSDictionary:
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariablePrepend(MCExternalVariableRef var, MCExternalValueOptions p_options, void *value)
{
	return kMCExternalErrorNotImplemented;
}
    
////////////////////////////////////////////////////////////////////////////////


// This method was never exposed.
static MCExternalError MCExternalVariableEdit(MCExternalVariableRef var, MCExternalValueOptions p_options, uint32_t p_required_length, void **r_buffer, uint32_t *r_length)
{
	return kMCExternalErrorNotImplemented;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalVariableCountKeys(MCExternalVariableRef var, uint32_t* r_count)
{
	return kMCExternalErrorNotImplemented;
}

static MCExternalError MCExternalVariableIterateKeys(MCExternalVariableRef var, MCExternalVariableIteratorRef *p_iterator, MCExternalValueOptions p_options, void *p_key, MCExternalVariableRef *r_value)
{
	return kMCExternalErrorNotImplemented;
}

static MCExternalError MCExternalVariableRemoveKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key)
{	
	return kMCExternalErrorNotImplemented;
}

static MCExternalError MCExternalVariableLookupKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key, bool p_ensure, MCExternalVariableRef *r_var)
{
	return kMCExternalErrorNotImplemented;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalObjectResolve(const char *p_long_id, MCExternalObjectRef *r_handle)
{
	// If we haven't been given a long id, its an error.
	if (p_long_id == nil)
		return kMCExternalErrorNoObjectId;

	// If we haven't been given a result parameter, do nothing.
	if (r_handle == nil)
		return kMCExternalErrorNone;

	MCExternalError t_error;
	t_error = kMCExternalErrorNone;

	// MW-2014-01-22: [[ CompatV1 ]] Convert the long id to a stringref.
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_long_id now UTF8-encoded
	MCAutoStringRef t_long_id_ref;
	if (!MCStringCreateWithBytes((byte_t*)p_long_id, strlen(p_long_id), kMCStringEncodingUTF8, false, &t_long_id_ref))
		return kMCExternalErrorOutOfMemory;
	
	// Create a script point with the value are setting the property to
	// as source text.
	MCScriptPoint sp(*t_long_id_ref);

	// Create a new chunk object to parse the reference into
	MCChunk *t_chunk;
	t_chunk = new MCChunk(False);
	if (t_chunk == nil)
		t_error = kMCExternalErrorOutOfMemory;

	// Attempt to parse a chunk. We also check that there is no 'junk' at
	// the end of the string - if there is, its an error. Note the errorlock
	// here - it stops parse errors being pushed onto MCperror.
	Symbol_type t_next_type;
	MCerrorlock++;
	if (t_error == kMCExternalErrorNone)
		if (t_chunk -> parse(sp, False) != PS_NORMAL || sp.next(t_next_type) != PS_EOF)
			t_error = kMCExternalErrorMalformedObjectChunk;

	// Now attempt to evaluate the object reference - this will only succeed
	// if the object exists.
	MCExecContext ep2(*MCECptr);
	MCObject *t_object;
	uint32_t t_part_id;
	if (t_error == kMCExternalErrorNone)
		if (t_chunk -> getobj(ep2, t_object, t_part_id, False) != ES_NORMAL)
			t_error = kMCExternalErrorCouldNotResolveObject;

	MCerrorlock--;

	// If we found the object, attempt to create a handle.
	if (t_error == kMCExternalErrorNone)
	{
		MCObjectHandle *t_handle;
		t_handle = t_object -> gethandle();
		if (t_handle != NULL)
			*(MCExternalObjectRef *)r_handle = t_handle;
		else
			t_error = kMCExternalErrorOutOfMemory;
	}

	// Cleanup
	delete t_chunk;

	return t_error;
}

static MCExternalError MCExternalObjectExists(MCExternalObjectRef p_handle, bool *r_exists)
{
	if (p_handle == nil)
		return kMCExternalErrorNoObject;

	*r_exists = p_handle -> Exists();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectRetain(MCExternalObjectRef p_handle)
{
	if (p_handle == nil)
		return kMCExternalErrorNoObject;

	p_handle -> Retain();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectRelease(MCExternalObjectRef p_handle)
{
	if (p_handle == nil)
		return kMCExternalErrorNoObject;

	p_handle -> Release();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectDispatch(MCExternalObjectRef p_object, MCExternalDispatchType p_type, const char *p_message, MCExternalVariableRef *p_argv, uint32_t p_argc, MCExternalDispatchStatus *r_status)
{
	if (p_object == nil)
		return kMCExternalErrorNoObject;

	if (p_message == nil)
		return kMCExternalErrorNoObjectMessage;

	if (p_argv == nil && p_argc > 0)
		return kMCExternalErrorNoObjectArguments;

	if (!p_object -> Exists())
		return kMCExternalErrorObjectDoesNotExist;

	MCExternalError t_error;
	t_error = kMCExternalErrorNone;

	MCParameter *t_params, *t_last_param;
	t_params = t_last_param = nil;
	for(uint32_t i = 0; i < p_argc; i++)
	{
		MCParameter *t_param;
		t_param = new MCParameter;
		t_param -> setvalueref_argument(p_argv[i] -> GetValueRef());

		if (t_last_param == nil)
			t_params = t_param;
		else
			t_last_param -> setnext(t_param);

		t_last_param = t_param;
	}

	MCNameRef t_message_as_name;
    MCAutoStringRef t_message_as_string;
	t_message_as_name = nil;
	if (t_error == kMCExternalErrorNone)
        // SN-2014-07-01: [[ ExternalsApiV6 ]] p_message is now UTF8-encoded
		if (!MCStringCreateWithBytes((byte_t*)p_message, strlen(p_message), kMCStringEncodingUTF8, false, &t_message_as_string)
                || !MCNameCreate(*t_message_as_string, t_message_as_name))
			t_error = kMCExternalErrorOutOfMemory;

	if (t_error == kMCExternalErrorNone)
	{
		Exec_stat t_stat;
		t_stat = p_object -> Get() -> dispatch(p_type == kMCExternalDispatchCommand ? HT_MESSAGE : HT_FUNCTION, t_message_as_name, t_params);
		if (r_status != nil)
			switch(t_stat)
			{
			case ES_NORMAL:
				*r_status = MCexitall == False ? kMCExternalDispatchStatusHandled : kMCExternalDispatchStatusExit;
				break;
			case ES_NOT_HANDLED:
				*r_status = kMCExternalDispatchStatusNotHandled;
				break;
			case ES_PASS:
				*r_status = kMCExternalDispatchStatusPassed;
				break;
			case ES_ERROR:
				*r_status = kMCExternalDispatchStatusError;
				break;
			}
	}

	MCNameDelete(t_message_as_name);

	while(t_params != nil)
	{
		MCParameter *t_param;
		t_param = t_params;
		t_params = t_params -> getnext();
		delete t_param;
	}

	return kMCExternalErrorNone;
}

static Properties parse_property_name(MCStringRef p_name)
{
	MCScriptPoint t_sp(p_name);
	Symbol_type t_type;
	const LT *t_literal;
	if (t_sp . next(t_type) &&
		t_sp . lookup(SP_FACTOR, t_literal) == PS_NORMAL &&
		t_literal -> type == TT_PROPERTY &&
		t_sp . next(t_type) == PS_EOF)
		return (Properties)t_literal -> which;
	
	return P_CUSTOM;
}

// SN-2014-07-01: [[ ExternalsApiV6 ]] p_name and p_key can now be UTF8-encoded
static MCExternalError MCExternalObjectSet(MCExternalObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, MCExternalVariableRef p_value)
{
	if (p_object == nil)
		return kMCExternalErrorNoObject;
	
	if (p_name == nil)
		return kMCExternalErrorNoObjectProperty;

	if (p_value == nil)
		return kMCExternalErrorNoObjectPropertyValue;
	
	if (!p_object -> Exists())
		return kMCExternalErrorObjectDoesNotExist;
    
    MCAutoStringRef t_name;
    MCAutoStringRef t_key;
    if (!MCStringCreateWithBytes((byte_t*)p_name, strlen(p_name), kMCStringEncodingUTF8, false, &t_name))
        return kMCExternalErrorOutOfMemory;
    if (p_key != nil && MCStringCreateWithBytes((byte_t*)p_key, strlen(p_key), kMCStringEncodingUTF8, false, &t_key))
        return kMCExternalErrorOutOfMemory;
	
	Properties t_prop;
	t_prop = parse_property_name(*t_name);
	
	MCObject *t_object;
	t_object = p_object -> Get();
	
	MCExecContext t_ctxt;
	
	MCExecValue t_value;
	t_value . type = kMCExecValueTypeValueRef;
	t_value . valueref_value = p_value -> GetValueRef();
	
	Exec_stat t_stat;
    t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCNewAutoNameRef t_propset_name, t_propset_key;
		if (*t_key == nil)
		{
			t_propset_name = t_object -> getdefaultpropsetname();
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_key);
		}
		else
		{
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_name);
			/* UNCHECKED */ MCNameCreate(*t_key, &t_propset_key);
		}
		if (!t_object -> setcustomprop(t_ctxt, *t_propset_name, *t_propset_key, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	else
	{
		MCNewAutoNameRef t_index;
		if (*t_key != nil)
		{
			if (!MCNameCreate(*t_key, &t_index))
				return kMCExternalErrorOutOfMemory;
		}
		
		if (!t_object -> setprop(t_ctxt, 0, t_prop, *t_index, False, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	return kMCExternalErrorNone;
}

// SN-2014-07-01: [[ ExternalsApiV6 ]] p_name and p_key can now be UTF8-encoded
static MCExternalError MCExternalObjectGet(MCExternalObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, MCExternalVariableRef p_value)
{
	if (p_object == nil)
		return kMCExternalErrorNoObject;
	
	if (p_name == nil)
		return kMCExternalErrorNoObjectProperty;
	
	if (p_value == nil)
		return kMCExternalErrorNoObjectPropertyValue;
	
	if (!p_object -> Exists())
		return kMCExternalErrorObjectDoesNotExist;
    
    MCAutoStringRef t_name;
    MCAutoStringRef t_key;
    if (!MCStringCreateWithBytes((byte_t*)p_name, strlen(p_name), kMCStringEncodingUTF8, false, &t_name))
        return kMCExternalErrorOutOfMemory;
    if (p_key != nil && MCStringCreateWithBytes((byte_t*)p_key, strlen(p_key), kMCStringEncodingUTF8, false, &t_key))
        return kMCExternalErrorOutOfMemory;
	
	Properties t_prop;
	t_prop = parse_property_name(*t_name);
	
	MCObject *t_object;
	t_object = p_object -> Get();
	
	MCExecContext t_ctxt;
	MCExecValue t_value;
	
	Exec_stat t_stat;
    t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCNewAutoNameRef t_propset_name, t_propset_key;
		if (*t_key == nil)
		{
            t_propset_name = t_object -> getdefaultpropsetname();
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_key);
		}
		else
		{
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_name);
			/* UNCHECKED */ MCNameCreate(*t_key, &t_propset_key);
		}
		if (!t_object -> getcustomprop(t_ctxt, *t_propset_name, *t_propset_key, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	else
	{
		MCNewAutoNameRef t_index;
		if (*t_key != nil)
		{
			if (!MCNameCreate(*t_key, &t_index))
				return kMCExternalErrorOutOfMemory;
		}
		
		if (!t_object -> getprop(t_ctxt, 0, t_prop, *t_index, False, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	MCAutoValueRef t_value_ref;
    // SN-2014-11-14: [[ Bug 14026 ]] We need to get the address of the pointer, not the pointer itself
	MCExecTypeConvertAndReleaseAlways(t_ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &(&t_value_ref));
	if (t_ctxt . HasError())
		return kMCExternalErrorOutOfMemory;
	
	p_value -> SetValueRef(*t_value_ref);
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectUpdate(MCExternalObjectRef p_object, unsigned int p_options, void *p_region)
{
	if (p_object == nil)
		return kMCExternalErrorNoObject;
	
	if (!p_object -> Exists())
		return kMCExternalErrorObjectDoesNotExist;
	
	MCObject *t_object;
	t_object = p_object -> Get();

	// MW-2011-08-19: [[ Layers ]] Nothing to do if object not a control.
	if (t_object -> gettype() < CT_GROUP)
		return kMCExternalErrorNone;

	if ((p_options & kMCExternalObjectUpdateOptionRect) == 0)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		static_cast<MCControl *>(t_object) -> layer_redrawall();
	}
	else
	{
		MCRectangle t_obj_rect;
		t_obj_rect = t_object -> getrect();
		
		MCRectangle t_dirty_rect;
		t_dirty_rect . x = t_obj_rect . x + ((int *)p_region)[0];
		t_dirty_rect . y = t_obj_rect . y + ((int *)p_region)[1];
		t_dirty_rect . width = ((int *)p_region)[2] - ((int *)p_region)[0];
		t_dirty_rect . height = ((int *)p_region)[3] - ((int *)p_region)[1];

		// MW-2011-08-18: [[ Layers ]] Invalidate part of the object.
		static_cast<MCControl *>(t_object) -> layer_redrawrect(t_dirty_rect);
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////


// SN-2014-07-01: [[ ExternalsApiV6 ]] Conversion methods refactored here
// since the to-fetch/to-store enum now includes the CF* types.
// Taken from LCIDLC file 'Support.mm'

#ifdef __HAS_CORE_FOUNDATION__

// Convert a LiveCode value into an element of an Objc Array/Dictionary. This
// converts all non-array values to strings, arrays which are sequences to
// NSArray, and arrays which are maps to NSDictionary.
static MCExternalError MCExternalValueArrayValueToObjcValue(MCExternalVariableRef var, id& r_dst)
{
	MCExternalError t_error;
	t_error = kMCExternalErrorNone;
	
	if (t_error == kMCExternalErrorNone)
	{
        bool t_is_empty;
        
        t_error = MCExternalVariableQuery(var, kMCExternalVariableQueryIsEmpty, &t_is_empty);
        
        if (t_error == kMCExternalErrorNone && t_is_empty)
		{
			r_dst = @"";
			return kMCExternalErrorNone;
		}
	}
	
	if (t_error == kMCExternalErrorNone)
	{
        bool t_is_array;
        
        t_error = MCExternalVariableQuery(var, kMCExternalVariableQueryIsAnArray, &t_is_array);
        
        if (t_error == kMCExternalErrorNone && !t_is_array)
			return MCExternalVariableFetch(var, kMCExternalValueOptionAsNSString, r_dst);
	}
	
	if (t_error == kMCExternalErrorNone)
	{
        bool t_is_sequence;
        
        t_error = MCExternalVariableQuery(var, kMCExternalVariableQueryIsASequence, &t_is_sequence);
        
        if (t_error == kMCExternalErrorNone && t_is_sequence)
			return MCExternalValueArrayToObjcArray(var, (NSArray*&)r_dst);
	}
	
	if (t_error == kMCExternalErrorNone)
		return MCExternalValueArrayToObjcDictionary(var, (NSDictionary*&)r_dst);
	
	return (MCExternalError)t_error;
}

static MCExternalError MCExternalValueArrayValueFromObjcValue(MCExternalVariableRef var, id src)
{
	if ([src isKindOfClass: [NSNull class]])
		return (MCExternalError)MCExternalVariableClear(var);
	
	if ((CFBooleanRef)src == kCFBooleanTrue || (CFBooleanRef)src == kCFBooleanFalse)
	{
		bool t_bool;
		t_bool = (CFBooleanRef)src == kCFBooleanTrue;
		return MCExternalVariableStore(var, kMCExternalValueOptionAsBoolean, &t_bool);
	}
    
	if ([src isKindOfClass: [NSNumber class]])
		return MCExternalVariableStore(var, kMCExternalValueOptionAsNSNumber, &src);
    
	if ([src isKindOfClass: [NSString class]])
		return MCExternalVariableStore(var, kMCExternalValueOptionAsNSString, &src);
    
	if ([src isKindOfClass: [NSData class]])
		return MCExternalVariableStore(var, kMCExternalValueOptionAsNSData, &src);
    
	if ([src isKindOfClass: [NSArray class]])
		return MCExternalValueArrayFromObjcArray(var, (NSArray *)src);
	
	if ([src isKindOfClass: [NSDictionary class]])
		return MCExternalValueArrayFromObjcDictionary(var, (NSDictionary *)src);
	
	NSString *t_as_string;
	t_as_string = [src description];
	return MCExternalVariableStore(var, kMCExternalValueOptionAsNSString, &t_as_string);
}

// Convert a LiveCode array into an NSArray. The returned NSArray is alloc'd.
static MCExternalError MCExternalValueArrayToObjcArray(MCExternalVariableRef src, NSArray*& r_dst)
{
	MCExternalError t_error;
	t_error = kMCExternalErrorNone;
	
	if (t_error == kMCExternalErrorNone)
	{
		bool t_is_sequence;
		t_error = MCExternalVariableQuery(src, kMCExternalVariableQueryIsASequence, &t_is_sequence);
		if (t_error == kMCExternalErrorNone && !t_is_sequence)
			t_error = kMCExternalErrorNotASequence;
	}
	
	uint32_t t_count;
	t_count = 0;
	if (t_error == kMCExternalErrorNone)
		t_error = g_external_interface . variable_count_keys(src, &t_count);
	
	id *t_objects;
	t_objects = nil;
	if (t_error == kMCExternalErrorNone)
	{
		t_objects = (id *)calloc(sizeof(id), t_count);
		if (t_objects == nil)
			t_error = kMCExternalErrorOutOfMemory;
	}
	
	MCExternalVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(uint32_t i = 0; i < t_count && t_error == kMCExternalErrorNone; i++)
	{
		// Fetch the key and value.
		const char *t_key;
		MCExternalVariableRef t_value;
		if (t_error == kMCExternalErrorNone)
			t_error = g_external_interface . variable_iterate_keys(src, &t_iterator, kMCExternalValueOptionAsCString, &t_key, &t_value);
		
		// Now convert the value - remembering that LC sequences are 1 based, and
		// Objc arrays are 0 based. Note that we don't have to validate the key as
		// its guaranteed to be of the correct form as we checked the array was a
		// sequence.
		if (t_error == kMCExternalErrorNone)
			t_error = MCExternalValueArrayValueToObjcValue(t_value, t_objects[strtoul(t_key, nil, 10) - 1]);
	}
	
	// If we succeeded, then try to build an NSArray.
	NSArray *t_array;
	if (t_error == kMCExternalErrorNone)
	{
		t_array = [[NSArray alloc] initWithObjects: t_objects count: t_count];
		if (t_array == nil)
			t_error = kMCExternalErrorOutOfMemory;
	}
	
	if (t_error == kMCExternalErrorNone)
		r_dst = t_array;
	
	// We free the objects array since its copied by NSArray.
	for(uint32_t i = 0; i < t_count; i++)
		[t_objects[i] release];
	free(t_objects);
	
	return t_error;
}

static MCExternalError MCExternalValueArrayFromObjcArray(MCExternalVariableRef var, NSArray *src)
{
	MCExternalError t_error;
	t_error = kMCExternalErrorNone;
	
	for(unsigned int t_index = 0; t_index < [src count] && t_error == kMCExternalErrorNone; t_index++)
	{
		char t_key[12];
		if (t_error == kMCExternalErrorNone)
			sprintf(t_key, "%ud", t_index + 1);
		
		MCExternalVariableRef t_value;
		if (t_error == kMCExternalErrorNone)
			t_error = (MCExternalError)g_external_interface . variable_lookup_key(var, kMCExternalValueOptionAsCString, t_key, true, &t_value);
		
		if (t_error == kMCExternalErrorNone)
			t_error = MCExternalValueArrayValueFromObjcValue(t_value, [src objectAtIndex: t_index]);
	}
	
	return t_error;
}

static MCExternalError MCExternalValueArrayToObjcDictionary(MCExternalVariableRef src, NSDictionary*& r_dst)
{
	MCExternalError t_error;
	t_error = kMCExternalErrorNone;
	
	uint32_t t_count;
	t_count = 0;
	if (t_error == kMCExternalErrorNone)
		t_count = g_external_interface . variable_count_keys(src, &t_count);
	
	id *t_keys, *t_values;
	t_keys = t_values = nil;
	if (t_error == kMCExternalErrorNone)
	{
		t_keys = (id *)calloc(sizeof(id), t_count);
		t_values = (id *)calloc(sizeof(id), t_count);
		if (t_keys == nil || t_values == nil)
			t_error = kMCExternalErrorOutOfMemory;
	}
	
	MCExternalVariableIteratorRef t_iterator;
	t_iterator = nil;
	for(uint32_t i = 0; i < t_count && t_error == kMCExternalErrorNone; i++)
	{
		// Fetch the key and value.
		MCAutoStringRef t_key;
		MCExternalVariableRef t_value;
		if (t_error == kMCExternalErrorNone)
			t_error = g_external_interface . variable_iterate_keys(src, &t_iterator, kMCExternalValueOptionAsString, &t_key, &t_value);
		
		// Convert the key.
		if (t_error == kMCExternalErrorNone)
		{
			if (!MCStringConvertToCFStringRef(*t_key, (CFStringRef&)t_keys[i]))
				t_error = kMCExternalErrorOutOfMemory;
		}
		
		// Now convert the value.
		if (t_error == kMCExternalErrorNone)
			t_error = MCExternalValueArrayValueToObjcValue(t_value, t_values[i]);
	}
	
	// If we succeeded then build the dictionary.
	NSDictionary *t_dictionary;
	if (t_error == kMCExternalErrorNone)
	{
		t_dictionary = [[NSDictionary alloc] initWithObjects: t_values forKeys: t_keys count: t_count];
		if (t_dictionary == nil)
			t_error = kMCExternalErrorOutOfMemory;
	}
	
	if (t_error == kMCExternalErrorNone)
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

static MCExternalError MCExternalValueArrayFromObjcDictionary(MCExternalVariableRef var, NSDictionary *p_src)
{
	MCExternalError t_error;
	t_error = kMCExternalErrorNone;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
#ifndef __OBJC2__
	NSEnumerator *t_enumerator;
	t_enumerator = [p_src keyEnumerator];
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
        if (t_error == kMCExternalErrorNone && ![t_key isKindOfClass: [NSString class]])
            t_error = kMCExternalErrorCannotEncodeMap;
        
        MCAutoStringRef t_key_stringref;
        if (t_error == kMCExternalErrorNone)
        {
            if (!MCStringCreateWithCFString((CFStringRef)t_key, &t_key_stringref))
                t_error = kMCExternalErrorOutOfMemory;
        }
        
        MCExternalVariableRef t_value;
        if (t_error == kMCExternalErrorNone)
            t_error = g_external_interface . variable_lookup_key(var, kMCExternalValueOptionAsString, (void *)*t_key_stringref, true, &t_value);
        
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalValueArrayValueFromObjcValue(t_value, [p_src objectForKey: t_key]);
    }
    [t_pool release];
    
    return t_error;
}
    
#endif // defined(__HAS_CORE_FOUNDATION__)

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalWaitRun(void *unused, unsigned int p_options)
{
	MCscreen -> wait(60.0, (p_options & kMCExternalWaitOptionDispatching) != 0 ? True : False, True);
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalWaitBreak(void *unused, unsigned int p_options)
{
	// MW-2013-06-25: [[ DesktopPingWait ]] Do a 'pingwait()' on all platforms so
	//   that wait's and such work on all platforms.
	MCscreen -> pingwait();
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

extern MCGFloat MCResGetPixelScale(void);
extern MCGFloat MCResGetUIScale(void);

extern void *MCIPhoneGetView(void);
extern void *MCIPhoneGetViewController(void);
extern void *MCAndroidGetActivity(void);
extern void *MCAndroidGetContainer(void);

// MW-2013-06-13: [[ ExternalsApiV5 ]] Methods to get the JavaEnv's.
extern void *MCAndroidGetScriptJavaEnv(void);
extern void *MCAndroidGetSystemJavaEnv(void);

// MW-2013-07-25: [[ ExternalsApiV5 ]] Method to get the Engine object.
extern void *MCAndroidGetEngine(void);

static MCExternalError MCExternalInterfaceQuery(MCExternalInterfaceQueryTag op, void *r_value)
{
    
	switch(op)
	{
		case kMCExternalInterfaceQueryViewScale:
			// IM-2014-03-14: [[ HiDPI ]] Return the inverse of the logical -> ui coords scale
			*(double *)r_value = 1.0 / MCResGetUIScale();
			break;
			
#if defined(TARGET_SUBPLATFORM_IPHONE)
		case kMCExternalInterfaceQueryView:
			*(void **)r_value = MCIPhoneGetView();
			break;
		case kMCExternalInterfaceQueryViewController:
			*(void **)r_value = MCIPhoneGetViewController();
			break;
#endif
			
#if defined(TARGET_SUBPLATFORM_ANDROID)
		case kMCExternalInterfaceQueryActivity:
			*(void **)r_value = MCAndroidGetActivity();
			break;
		case kMCExternalInterfaceQueryContainer:
			*(void **)r_value = MCAndroidGetContainer();
			break;
			
		// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of the script JavaEnv accessor.
		case kMCExternalInterfaceQueryScriptJavaEnv:
			*(void **)r_value = MCAndroidGetScriptJavaEnv();
			break;
			
		// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of the systen JavaEnv accessor.
		case kMCExternalInterfaceQuerySystemJavaEnv:
			*(void **)r_value = MCAndroidGetSystemJavaEnv();
			break;

		// MW-2013-07-25: [[ ExternalsApiV5 ]] Implementation of the Engine accessor.
		case kMCExternalInterfaceQueryEngine:
			*(void **)r_value = MCAndroidGetEngine();
			break;
#endif

		default:
			return kMCExternalErrorInvalidInterfaceQuery;
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

MCExternalInterface g_external_interface =
{
	kMCExternalInterfaceVersion,

	MCExternalEngineRunOnMainThread,

	MCExternalContextQuery,

	MCExternalVariableCreate,
	MCExternalVariableRetain,
	MCExternalVariableRelease,
	MCExternalVariableQuery,
	MCExternalVariableClear,
	MCExternalVariableExchange,
	MCExternalVariableStore,
	MCExternalVariableFetch,
	MCExternalVariableAppend,
	MCExternalVariablePrepend,
	MCExternalVariableEdit,
	MCExternalVariableCountKeys,
	MCExternalVariableIterateKeys,
	MCExternalVariableRemoveKey,
	MCExternalVariableLookupKey,

	MCExternalObjectResolve,
	MCExternalObjectExists,
	MCExternalObjectRetain,
	MCExternalObjectRelease,
	MCExternalObjectDispatch,
	
	MCExternalWaitRun,
	MCExternalWaitBreak,
	
	MCExternalObjectGet,
	MCExternalObjectSet,
	
	MCExternalInterfaceQuery,
	
	MCExternalObjectUpdate,
	
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Declare the evaluate and execute methods
	//   for the outside world.
	MCExternalContextEvaluate,
	MCExternalContextExecute,
};

////////////////////////////////////////////////////////////////////////////////

