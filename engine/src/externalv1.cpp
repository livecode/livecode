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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
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

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

typedef MCVariableValue *MCExternalVariableRef;
typedef MCObjectHandle *MCExternalObjectRef;

typedef void *MCExternalVariableIteratorRef;
typedef uint32_t MCExternalValueOptions;
typedef uint32_t MCExternalRunOnMainThreadOptions;
typedef void (*MCExternalThreadOptionalCallback)(void *state);
typedef void (*MCExternalThreadRequiredCallback)(void *state, int flags);

// MW-2013-06-14: [[ ExternalsApiV5 ]] Update the interface version.
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
	kMCExternalValueOptionAsString = 5,
	kMCExternalValueOptionAsCString = 6,

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
	
	kMCExternalContextQueryItemDelimiter,
	kMCExternalContextQueryLineDelimiter,
	kMCExternalContextQueryColumnDelimiter,
	kMCExternalContextQueryRowDelimiter,
	
	kMCExternalContextQueryDefaultStack,
	kMCExternalContextQueryDefaultCard,
	
	// MW-2013-06-14: [[ ExternalsApiV5 ]] Accessor to fetch 'the wholeMatches'.
	kMCExternalContextQueryWholeMatches,
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

	MCExternalError (*variable_edit)(MCExternalVariableRef var, MCExternalValueOptions options, uint32_t reserve_length, void **r_buffer, uint32_t *r_length);

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
	t_describe = (MCExternalDescribeProc)MCS_resolvemodulesymbol(m_module, "MCExternalDescribe");
	
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
	t_initialize = (MCExternalInitializeProc)MCS_resolvemodulesymbol(m_module, "MCExternalInitialize");
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
	t_finalize = (MCExternalFinalizeProc)MCS_resolvemodulesymbol(m_module, "MCExternalFinalize");
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
		MCVariableValue **t_parameter_vars;
		t_parameter_vars = NULL;
		if (t_parameter_count != 0)
		{
			t_parameter_vars = new MCVariableValue *[t_parameter_count];
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
				t_parameter_vars[i] = &t_var -> getvalue();
			}
			else
			{
				// We need a temporary so do an exchange. Note that unlike other handlers,
				// external handlers can't 'pass' so we don't need the value in the parameter
				// anymore after this 'Handle' call.
				t_parameter_vars[i] = new MCVariableValue;
				if (t_parameter_vars[i] != nil)
				{
					t_parameter_vars[i] -> set_temporary();
					t_parameter_vars[i] -> exchange(p_parameters -> getvalue());
				}
				else
					t_stat = ES_ERROR;
			}
		}

		// We have our list of parameters (hopefully), so now call - passing a temporary
		// result.
		if (t_stat == ES_NORMAL)
		{
			MCVariableValue t_result;
			t_result . set_temporary();
			t_result . assign_empty();

			// Invoke the external handler. If 'false' is returned, treat the result as a
			// string value containing an error hint.
			if ((t_handler -> handler)(t_parameter_vars, t_parameter_count, &t_result))
				MCresult -> getvalue() . exchange(t_result);
			else
			{
				MCeerror -> add(EE_EXTERNAL_EXCEPTION, 0, 0, t_result . is_string() ? t_result . get_string() : "");
				t_stat = ES_ERROR;
			}
		}

		// Finally, loop through and free the parameters as necessary.
		for(uint32_t i = 0; i < t_parameter_count; i++)
			if (t_parameter_vars[i] -> get_temporary())
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

static bool options_get_convert_octals(MCExternalValueOptions p_options)
{
	switch(p_options & kMCExternalValueOptionConvertOctalsMask)
	{
		case kMCExternalValueOptionDefaultConvertOctals:
			return MCEPptr -> getconvertoctals() == True;
		case kMCExternalValueOptionConvertOctals:
			return true;
		case kMCExternalValueOptionDoNotConvertOctals:
			return false;
		default:
			break;
	}
	return false;
}

static MCExternalError string_to_boolean(const MCString& p_string, MCExternalValueOptions p_options, void *r_value)
{
	if (p_string == MCtruemcstring)
		*(bool *)r_value = true;
	else if (p_string == MCfalsemcstring)
		*(bool *)r_value = false;
	else
		return kMCExternalErrorNotABoolean;

	return kMCExternalErrorNone;
}

static MCExternalError string_to_integer(const MCString& p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
	uint32_t l;
	s = p_string . getstring();
	l = p_string . getlength();

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

static MCExternalError string_to_real(const MCString& p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
	uint32_t l;
	s = p_string . getstring();
	l = p_string . getlength();

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
		uint32_t t_value;
		t_error = string_to_integer(MCString(s, l), (p_options & ~0xf) | kMCExternalValueOptionAsCardinal, &t_value);
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

static void number_to_string(double p_number, MCExternalValueOptions p_options, char p_buffer[R8L])
{
	switch(p_options & kMCExternalValueOptionNumberFormatMask)
	{
	case kMCExternalValueOptionDefaultNumberFormat:
		{
			char *v;
			v = p_buffer;
			uint4 l;
			l = R8L;
			MCU_r8tos(v, l, p_number, MCEPptr -> getnffw(), MCEPptr -> getnftrailing(), MCEPptr -> getnfforce());
		}
		break;
	case kMCExternalValueOptionDecimalNumberFormat:
		sprintf(p_buffer, "%f", p_number);
		break;
	case kMCExternalValueOptionScientificNumberFormat:
		sprintf(p_buffer, "%e", p_number);
		break;
	case kMCExternalValueOptionCompactNumberFormat:
		sprintf(p_buffer, "%0.16g", p_number);
		break;
	}
}

static MCExternalError fetch_as_string(MCExternalVariableRef p_var, MCExternalValueOptions p_options, void *r_value)
{
	if (p_var -> is_number())
	{
		double t_number;
		t_number = p_var -> get_real();

		char t_buffer[R8L];
		number_to_string(t_number, p_options, t_buffer);

		if (!p_var -> assign_custom_both(t_buffer, t_number))
			return kMCExternalErrorOutOfMemory;

		if ((p_options & 0xf) == kMCExternalValueOptionAsString)
			*(MCString *)r_value = p_var -> get_custom_string();
		else
			*(const char **)r_value = p_var -> get_custom_string() . getstring();
	}
	else if (p_var -> is_string())
	{
		if ((p_options & 0xf) == kMCExternalValueOptionAsString)
			*(MCString *)r_value = p_var -> get_string();
		else
		{
			if (memchr(p_var -> get_string() . getstring(), '\0', p_var -> get_string() . getlength()) != NULL)
				return kMCExternalErrorNotACString;

			if (!p_var -> ensure_cstring())
				return kMCExternalErrorOutOfMemory;

			*(const char **)r_value = p_var -> get_string() . getstring();
		}
	}
	else
		return kMCExternalErrorNotAString;

	return kMCExternalErrorNone;
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
			t_handle = MCEPptr -> getobj() -> gethandle();
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
		*(MCExternalVariableRef *)result = &MCresult -> getvalue();
		break;
	case kMCExternalContextQueryIt:
		{
			MCVariable *t_var;
			t_var = MCEPptr -> gethandler() -> getit();
			*(MCExternalVariableRef *)result = (t_var != NULL) ? &t_var -> getvalue() : NULL;
		}
		break;
	case kMCExternalContextQueryCaseSensitive:
		*(bool *)result = MCEPptr -> getcasesensitive() == True;
		break;
	case kMCExternalContextQueryConvertOctals:
		*(bool *)result = MCEPptr -> getconvertoctals() == True;
		break;
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of 'the wholeMatches' query.
	case kMCExternalContextQueryWholeMatches:
		*(bool *)result = MCEPptr -> getwholematches() == True;
		break;
	case kMCExternalContextQueryItemDelimiter:
		*(uint32_t *)result = MCEPptr -> getitemdel();
		break;
	case kMCExternalContextQueryLineDelimiter:
		*(uint32_t *)result = MCEPptr -> getlinedel();
		break;
	case kMCExternalContextQueryColumnDelimiter:
		*(uint32_t *)result = MCEPptr -> getcolumndel();
		break;
	case kMCExternalContextQueryRowDelimiter:
		*(uint32_t *)result = MCEPptr -> getrowdel();
		break;
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
	MCEPptr -> setsvalue(p_expression);
	
	Exec_stat t_stat;
	t_stat = MCEPptr -> gethandler() -> eval(*MCEPptr);
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	
	if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	p_result -> store(*MCEPptr);
	
	return kMCExternalErrorNone;
}

// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of context_execute method.
MCExternalError MCExternalContextExecute(const char *p_commands, unsigned int p_options, MCExternalVariableRef *p_binds, unsigned int p_bind_count)
{
	MCEPptr -> setsvalue(p_commands);
	
	Exec_stat t_stat;
	t_stat = MCEPptr -> gethandler() -> doscript(*MCEPptr, 0, 0);
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	
	if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

struct MCTemporaryVariable: public MCVariableValue
{
	uint32_t references;
};

static MCExternalError MCExternalVariableCreate(MCExternalVariableRef* r_var)
{
	*r_var = new MCTemporaryVariable;
	if (r_var == nil)
		return kMCExternalErrorOutOfMemory;

	(*r_var) -> set_external();
	static_cast<MCTemporaryVariable *>(*r_var) -> references = 1;

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRetain(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (!var -> get_external())
		return kMCExternalErrorNone;

	// Otherwise increment the reference count.
	MCTemporaryVariable *t_temp_var;
	t_temp_var = static_cast<MCTemporaryVariable *>(var);
	t_temp_var -> references += 1;

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRelease(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	// If we aren't a temporary var, then we do nothing.
	if (!var -> get_external())
		return kMCExternalErrorNone;

	// Otherwise decrement the reference count.
	MCTemporaryVariable *t_temp_var;
	t_temp_var = static_cast<MCTemporaryVariable *>(var);
	t_temp_var -> references -= 1;
	if (t_temp_var -> references == 0)
		delete t_temp_var;
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableQuery(MCExternalVariableRef var, MCExternalVariableQueryTag p_query, void *r_result)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;
	
	switch(p_query)
	{
	case kMCExternalVariableQueryIsTemporary:
		*(bool *)r_result = var -> get_external();
		break;
	case kMCExternalVariableQueryIsTransient:
		*(bool *)r_result = var -> get_temporary();
		break;
	case kMCExternalVariableQueryFormat:
		*(Value_format *)r_result = var -> get_format();
		break;
	case kMCExternalVariableQueryRetention:
		if (!var -> get_external())
			*(uint32_t *)r_result = 0;
		else
			*(uint32_t *)r_result = static_cast<MCTemporaryVariable *>(var) -> references;
		break;
	case kMCExternalVariableQueryIsAnArray:
		*(bool *)r_result = var -> is_empty() || var -> is_array();
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsEmpty variable query.
	case kMCExternalVariableQueryIsEmpty:
		*(bool *)r_result = var -> is_empty();
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsASequence variable query.
	case kMCExternalVariableQueryIsASequence:
		*(bool *)r_result = var -> is_array() && var -> get_array() -> issequence();
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

	var -> clear();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableExchange(MCExternalVariableRef var_a, MCExternalVariableRef var_b)
{
	if (var_a == nil || var_b == nil)
		return kMCExternalErrorNoVariable;

	var_a -> exchange(*var_b);

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableStore(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;

	switch(p_options & 0xf)
	{
	case kMCExternalValueOptionAsVariable:
		if (!var -> assign(*(MCVariableValue *)p_value))
			return kMCExternalErrorOutOfMemory;
		break;
	case kMCExternalValueOptionAsBoolean:
		var -> assign_constant_string(*(bool *)p_value ? MCtruemcstring : MCfalsemcstring);
		break;
	case kMCExternalValueOptionAsInteger:
		var -> assign_real(*(int32_t *)p_value);
		break;
	case kMCExternalValueOptionAsCardinal:
		var -> assign_real(*(uint32_t *)p_value);
		break;
	case kMCExternalValueOptionAsReal:
		var -> assign_real(*(real64_t *)p_value);
		break;
	case kMCExternalValueOptionAsString:
		if (!var -> assign_string(*(MCString *)p_value))
			return kMCExternalErrorOutOfMemory;
		break;
	case kMCExternalValueOptionAsCString:
		if (!var -> assign_string(*(const char **)p_value))
			return kMCExternalErrorOutOfMemory;
		break;
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

	switch(p_options & 0xf)
	{
	case kMCExternalValueOptionAsVariable:
		if (!static_cast<MCVariableValue *>(p_value) -> assign(*var))
			return kMCExternalErrorOutOfMemory;
		break;

	case kMCExternalValueOptionAsBoolean:
		if (var -> is_string())
			return string_to_boolean(var -> get_string(), p_options, p_value);
		return kMCExternalErrorNotABoolean;

	case kMCExternalValueOptionAsInteger:
	case kMCExternalValueOptionAsCardinal:
		if (var -> is_number())
			return number_to_integer(var -> get_real(), p_options, p_value);
		else if (var -> is_string())
			return string_to_integer(var -> get_string(), p_options, p_value);
		return kMCExternalErrorNotANumber;

	case kMCExternalValueOptionAsReal:
		if (var -> is_number())
			return number_to_real(var -> get_real(), p_options, p_value);
		else if (var -> is_string())
			return string_to_real(var -> get_string(), p_options, p_value);
		return kMCExternalErrorNotANumber;

	case kMCExternalValueOptionAsString:
	case kMCExternalValueOptionAsCString:
		return fetch_as_string(var, p_options, p_value);
		break;

	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError coerce_to_string(MCExternalVariableRef var, MCExternalValueOptions p_options)
{
	if (var -> get_format() == VF_ARRAY)
		return kMCExternalErrorDstNotAString;

	if (var -> get_format() != VF_STRING)
	{
		if (var -> is_number())
		{
			char t_buffer[R8L];
			number_to_string(var -> get_real(), p_options, t_buffer);
			if (!var -> assign_string(t_buffer))
				return kMCExternalErrorOutOfMemory;
		}
		else
			var -> assign_constant_string(MCnullmcstring);
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableAppend(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;

	MCExternalError t_error;
	t_error = coerce_to_string(var, p_options);
	if (t_error != kMCExternalErrorNone)
		return t_error;

	switch(p_options & 0xf)
	{
	case kMCExternalValueOptionAsVariable:
		{
			MCExternalVariableRef t_value_var;
			t_value_var = (MCExternalVariableRef)p_value;

			if (t_value_var -> is_number())
			{
				double t_number;
				t_number = t_value_var -> get_real();

				char t_buffer[R8L];
				number_to_string(t_number, p_options, t_buffer);

				if (!var -> append_string(t_buffer))
					return kMCExternalErrorOutOfMemory;
			}
			else if (t_value_var -> is_string())
				if (!var -> append_string(t_value_var -> get_string()))
					return kMCExternalErrorOutOfMemory;
		}
		break;
	case kMCExternalValueOptionAsBoolean:
		if (!var -> append_string(*(bool *)p_value ? MCtruemcstring : MCfalsemcstring))
			return kMCExternalErrorOutOfMemory;
		break;
	case kMCExternalValueOptionAsInteger:
		{
			char t_buffer[I4L];
			sprintf(t_buffer, "%d", *(int32_t *)p_value);
			if (!var -> append_string(t_buffer))
				return kMCExternalErrorOutOfMemory;
		}
		break;
	case kMCExternalValueOptionAsCardinal:
		{
			char t_buffer[U4L];
			sprintf(t_buffer, "%u", *(uint32_t *)p_value);
			if (!var -> append_string(t_buffer))
				return kMCExternalErrorOutOfMemory;
		}
		break;
	case kMCExternalValueOptionAsString:
		if (!var -> append_string(*(MCString *)p_value))
			return kMCExternalErrorOutOfMemory;
		break;
	case kMCExternalValueOptionAsCString:
		if (!var -> append_string(*(const char **)p_value))
			return kMCExternalErrorOutOfMemory;
		break;
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

static MCExternalError MCExternalVariableEdit(MCExternalVariableRef var, MCExternalValueOptions p_options, uint32_t p_required_length, void **r_buffer, uint32_t *r_length)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	// Its a commit operation

	if (r_buffer == nil && r_length == nil)
	{
		if (!var -> is_string())
			return kMCExternalErrorDstNotAString;

		if (!var -> commit(p_required_length))
			return kMCExternalErrorInvalidEdit;

		return kMCExternalErrorNone;
	}

	// Its a reserve operation

	if (r_buffer == nil || r_length == nil)
		return kMCExternalErrorNoBuffer;

	MCExternalError t_error;
	t_error = coerce_to_string(var, p_options);
	if (t_error != kMCExternalErrorNone)
		return t_error;

	if (!var -> reserve(p_required_length, *r_buffer, *r_length))
		return kMCExternalErrorOutOfMemory;

	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalVariableCountKeys(MCExternalVariableRef var, uint32_t* r_count)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;
	
	if (var -> is_array())
		*r_count = var -> get_array() -> getnfilled();
	else if (var -> is_empty())
		*r_count = 0;
	else
		return kMCExternalErrorNotAnArray;
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableIterateKeys(MCExternalVariableRef var, MCExternalVariableIteratorRef *p_iterator, MCExternalValueOptions p_options, void *p_key, MCExternalVariableRef *r_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_iterator == nil)
		return kMCExternalErrorNoIterator;

	// If the var is not an array, we set the iterator to nil to indicate
	// there are no elements.
	if (!var -> is_array())
	{
		*p_iterator = nil;
		return var -> is_empty() ? kMCExternalErrorNone : kMCExternalErrorNotAnArray;
	}

	// If both key and value are nil, then the iteration is being cleaned up.
	if (p_key == nil && r_value == nil)
	{
		// We don't have anything to clean up at the moment...
		return kMCExternalErrorNone;
	}

	MCHashentry *t_entry;
	t_entry = var -> get_array() -> getnextkey(static_cast<MCHashentry *>(*p_iterator));

	// Update the iterator pointer. Note that we do this here to allow iteration
	// to continue after a value conversion error.
	*p_iterator = t_entry;

	// If we have an entry, then extract the key in the form that was requested
	// and return it's value.
	if (t_entry != nil)
	{
		*r_value = &t_entry -> value;
		switch(p_options & 0xf)
		{
		case kMCExternalValueOptionAsVariable:
			if (!((MCExternalVariableRef)p_key) -> assign_string(t_entry -> string))
				return kMCExternalErrorOutOfMemory;
			break;
		case kMCExternalValueOptionAsBoolean:
			return string_to_boolean(t_entry -> string, p_options, p_key);
		case kMCExternalValueOptionAsInteger:
		case kMCExternalValueOptionAsCardinal:
			return string_to_integer(t_entry -> string, p_options, p_key);
		case kMCExternalValueOptionAsReal:
			return string_to_real(t_entry -> string, p_options, p_key);
		case kMCExternalValueOptionAsString:
			*(MCString *)p_key = t_entry -> string;
			return kMCExternalErrorNone;
		case kMCExternalValueOptionAsCString:
			*(char **)p_key = t_entry -> string;
			return kMCExternalErrorNone;
		default:
			return kMCExternalErrorInvalidValueType;
		}
	}

	return kMCExternalErrorNone;
}

static MCExternalError fetch_hash_entry(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key, bool p_ensure, MCHashentry*& r_entry)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;
	
	if (p_key == nil)
		return kMCExternalErrorNoValue;
	
	Boolean t_case_sensitive;
	switch(p_options & kMCExternalValueOptionCaseSensitiveMask)
	{
		case kMCExternalValueOptionDefaultCaseSensitive:
			t_case_sensitive = MCEPptr -> getcasesensitive();
			break;
		case kMCExternalValueOptionCaseSensitive:
			t_case_sensitive = true;
			break;
		case kMCExternalValueOptionNotCaseSensitive:
			t_case_sensitive = false;
			break;
		default:
			return kMCExternalErrorInvalidCaseSensitiveOption;
	}			
			
	if (var -> is_array())
		;
	else if (var -> is_empty())
	{
		if (!p_ensure)
		{
			r_entry = nil;
			return kMCExternalErrorNone;
		}
		else
			var -> assign_new_array(TABLE_SIZE);
	}
	else
		return kMCExternalErrorNotAnArray;
	
	MCHashentry *t_entry;
	t_entry = nil;
	switch(p_options & 0xf)
	{
		case kMCExternalValueOptionAsVariable:
		{
			MCExternalVariableRef t_value_var;
			t_value_var = (MCExternalVariableRef)p_key;
			
			if (t_value_var -> is_number())
			{
				double t_number;
				t_number = t_value_var -> get_real();
				
				char t_buffer[R8L];
				number_to_string(t_number, p_options, t_buffer);
				
				t_entry = var -> get_array() -> lookuphash(t_buffer, t_case_sensitive, p_ensure);
			}
			else
				t_entry = var -> get_array() -> lookuphash(var -> is_string() ? var -> get_string() : MCnullmcstring, t_case_sensitive, p_ensure);
		}
		break;
		case kMCExternalValueOptionAsBoolean:
			t_entry = var -> get_array() -> lookuphash(*(bool *)p_key ? MCtruemcstring : MCfalsemcstring, t_case_sensitive, p_ensure);
		break;
		case kMCExternalValueOptionAsInteger:
		{
			char t_key_str[I4L];
			sprintf(t_key_str, "%d", *(int32_t *)p_key);
			t_entry = var -> get_array() -> lookuphash(t_key_str, t_case_sensitive, p_ensure);
		}
		break;
		case kMCExternalValueOptionAsCardinal:
		{
			char t_key_str[I4L];
			sprintf(t_key_str, "%u", *(uint32_t *)p_key);
			t_entry = var -> get_array() -> lookuphash(t_key_str, t_case_sensitive, p_ensure);
		}
		break;
		case kMCExternalValueOptionAsReal:
		{
			char t_key_str[R8L];
			number_to_string(*(double *)p_key, p_options, t_key_str);
			t_entry = var -> get_array() -> lookuphash(t_key_str, t_case_sensitive, p_ensure);
		}
		break;
		case kMCExternalValueOptionAsString:
			t_entry = var -> get_array() -> lookuphash(*(MCString *)p_key, t_case_sensitive, p_ensure);
		break;
		case kMCExternalValueOptionAsCString:
			t_entry = var -> get_array() -> lookuphash(*(const char **)p_key, t_case_sensitive, p_ensure);
		break;
		default:
			return kMCExternalErrorInvalidValueType;
	}
	
	if (p_ensure && t_entry == nil)
		return kMCExternalErrorOutOfMemory;
	
	r_entry = t_entry;

	return kMCExternalErrorNone;
				
}

static MCExternalError MCExternalVariableRemoveKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key)
{	
	MCExternalError t_error;

	MCHashentry *t_entry;
	t_error = fetch_hash_entry(var, p_options, p_key, false, t_entry);
	if (t_error == kMCExternalErrorNone && t_entry != nil)
		var -> remove_hash(t_entry);
	
	return t_error;
}

static MCExternalError MCExternalVariableLookupKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key, bool p_ensure, MCExternalVariableRef *r_var)
{
	MCExternalError t_error;
	
	MCHashentry *t_entry;
	t_error = fetch_hash_entry(var, p_options, p_key, p_ensure, t_entry);
	if (t_error == kMCExternalErrorNone)
		*r_var = t_entry != nil ? &t_entry -> value : nil;
		
	return t_error;
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

	// Create a script point with the value are setting the property to
	// as source text.
	MCScriptPoint sp(p_long_id);

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
	MCExecPoint ep2(*MCEPptr);
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
		if (t_param == nil ||
			!t_param -> getvalue() . assign(*p_argv[i]))
		{
			t_error = kMCExternalErrorOutOfMemory;
			break;
		}

		if (t_last_param == nil)
			t_params = t_param;
		else
			t_last_param -> setnext(t_param);

		t_last_param = t_param;
	}

	MCNameRef t_message_as_name;
	t_message_as_name = nil;
	if (t_error == kMCExternalErrorNone)
		if (!MCNameCreateWithCString(p_message, t_message_as_name))
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

static Properties parse_property_name(const char *p_name)
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
	
	Properties t_prop;
	t_prop = parse_property_name(p_name);
	
	MCObject *t_object;
	t_object = p_object -> Get();
	
	MCExecPoint t_ep(t_object, nil, nil);
	p_value -> fetch(t_ep, false);
	
	Exec_stat t_stat;
	if (t_prop == P_CUSTOM)
	{
		MCAutoNameRef t_propset_name, t_propset_key;
		if (p_key == nil)
		{
			/* UNCHECKED */ t_propset_name . Clone(t_object -> getdefaultpropsetname());
			/* UNCHECKED */ t_propset_key . CreateWithCString(p_name);
		}
		else
		{
			/* UNCHECKED */ t_propset_name . CreateWithCString(p_name);
			/* UNCHECKED */ t_propset_key . CreateWithCString(p_key);
		}
		t_stat = t_object -> setcustomprop(t_ep, t_propset_name, t_propset_key);
	}
	else if (t_prop >= P_FIRST_ARRAY_PROP)
	{
		MCAutoNameRef t_key_name;
		/* UNCHECKED */ t_key_name . CreateWithCString(p_key);

		// MW-2011-11-23: [[ Array Chunk Props ]] Array props can now have 'effective', but that
		//   only applies if its a chunk prop so just pass 'False'.
		t_stat = t_object -> setarrayprop(0, t_prop, t_ep, t_key_name, False);
	}
	else
		t_stat = t_object -> setprop(0, t_prop, t_ep, False);
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	return kMCExternalErrorNone;
}

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
	
	Properties t_prop;
	t_prop = parse_property_name(p_name);
	
	MCObject *t_object;
	t_object = p_object -> Get();
	
	MCExecPoint t_ep(t_object, nil, nil);
	
	Exec_stat t_stat;
	if (t_prop == P_CUSTOM)
	{
		MCAutoNameRef t_propset_name, t_propset_key;
		if (p_key == nil)
		{
			/* UNCHECKED */ t_propset_name . Clone(t_object -> getdefaultpropsetname());
			/* UNCHECKED */ t_propset_key . CreateWithCString(p_name);
		}
		else
		{
			/* UNCHECKED */ t_propset_name . CreateWithCString(p_name);
			/* UNCHECKED */ t_propset_key . CreateWithCString(p_key);
		}
		t_stat = t_object -> getcustomprop(t_ep, t_propset_name, t_propset_key);
	}
	else if (t_prop >= P_FIRST_ARRAY_PROP)
	{
		MCAutoNameRef t_key_name;
		/* UNCHECKED */ t_key_name . CreateWithCString(p_key);

		// MW-2011-11-23: [[ Array Chunk Props ]] Array props can now have 'effective', but that
		//   only applies if its a chunk prop so just pass 'False'.
		t_stat = t_object -> getarrayprop(0, t_prop, t_ep, t_key_name, False);
	}
	else
		t_stat = t_object -> getprop(0, t_prop, t_ep, False);
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	p_value -> store(t_ep);
	
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
