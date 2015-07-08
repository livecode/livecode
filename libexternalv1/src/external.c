#include "external.h"

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCExternalStartupProc)(void);
typedef void (*MCExternalShutdownProc)(void);

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

typedef struct MCExternalInterface
{
	uint32_t version;

	//////////
	
	MCError (*engine_run_on_main_thread)(void *callback, void *callback_state, uint32_t options);

	//////////

	MCError (*context_query)(MCExternalContextVar op, void *result);

	//////////

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

	/////////

	MCError (*object_resolve)(const char *chunk, MCObjectRef* r_object);
	MCError (*object_exists)(MCObjectRef object, bool *r_exists);
	MCError (*object_retain)(MCObjectRef object);
	MCError (*object_release)(MCObjectRef object);
	MCError (*object_dispatch)(MCObjectRef object, MCDispatchType type, const char *message, MCVariableRef *argv, uint32_t argc, MCDispatchStatus *r_status);
	
	/////////
	
	// MM-2014-03-18: [[ iOS 7.1 Support ]] Added new WaitRun and WaitBreak functions required by reviphone external for iOS 7.1 support.
	MCError (*wait_run)(void *unused, unsigned int options);
	MCError (*wait_break)(void *unused, unsigned int options);
	
	/////////
	
} MCExternalInterface;

typedef struct MCExternalInfo
{
	uint32_t version;
	uint32_t flags;
	const char *name;
	MCExternalHandler *handlers;
} MCExternalInfo;

#if defined(_WIN32)
#define MC_EXTERNAL_EXPORT __declspec(dllexport)
#else
#define MC_EXTERNAL_EXPORT
#endif

////////////////////////////////////////////////////////////////////////////////

static MCExternalInterface *s_interface = 0;

//////////

MCError MCVariableCreate(MCVariableRef *r_var)
{
	return s_interface -> variable_create(r_var);
}

MCError MCVariableRetain(MCVariableRef var)
{
	return s_interface -> variable_retain(var);
}

MCError MCVariableRelease(MCVariableRef var)
{
	return s_interface -> variable_release(var);
}

MCError MCVariableIsTransient(MCVariableRef var, bool *r_transient)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsTransient, r_transient);
}

MCError MCVariableIsTemporary(MCVariableRef var, bool *r_temporary)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsTemporary, r_temporary);
}

MCError MCVariableIsAnArray(MCVariableRef var, bool *r_array)
{
	return s_interface -> variable_query(var, kMCExternalVariableQueryIsAnArray, r_array);
}

MCError MCVariableClear(MCVariableRef var)
{
	return s_interface -> variable_clear(var);
}

MCError MCVariableExchange(MCVariableRef var_a, MCVariableRef var_b)
{
	return s_interface -> variable_exchange(var_a, var_b);
}

MCError MCVariableStore(MCVariableRef var, MCValueOptions options, void *value)
{
	return s_interface -> variable_store(var, options, value);
}

MCError MCVariableFetch(MCVariableRef var, MCValueOptions options, void *value)
{
	return s_interface -> variable_fetch(var, options, value);
}

MCError MCVariableAppend(MCVariableRef var, MCValueOptions options, void *value)
{
	return s_interface -> variable_append(var, options, value);
}

MCError MCVariableCountKeys(MCVariableRef var, uint32_t *r_count)
{
	return s_interface -> variable_count_keys(var, r_count);
}

MCError MCVariableIterateKeys(MCVariableRef var, MCVariableIteratorRef *iterator, MCValueOptions options, void *key, MCVariableRef* r_value)
{
	return s_interface -> variable_iterate_keys(var, iterator, options, key, r_value);
}

MCError MCVariableRemoveKey(MCVariableRef var, MCValueOptions key_type, void *key)
{
	return s_interface -> variable_remove_key(var, key_type, key);
}

MCError MCVariableLookupKey(MCVariableRef var, MCValueOptions key_type, void *key, bool ensure, MCVariableRef *r_value)
{
	return s_interface -> variable_lookup_key(var, key_type, key, ensure, r_value);
}

//////////

MCError MCObjectResolve(const char *id, MCObjectRef *r_object)
{
	return s_interface -> object_resolve(id, r_object);
}

MCError MCObjectDispatch(MCObjectRef object, MCDispatchType type, const char *message, MCVariableRef *argv, uint32_t argc, MCDispatchStatus *r_status)
{
	return s_interface -> object_dispatch(object, type, message, argv, argc, r_status);
}

MCError MCObjectRetain(MCObjectRef object)
{
	return s_interface -> object_retain(object);
}

MCError MCObjectRelease(MCObjectRef object)
{
	return s_interface -> object_release(object);
}

MCError MCObjectExists(MCObjectRef object, bool *r_exists)
{
	return s_interface -> object_exists(object, r_exists);
}

//////////

MCError MCContextMe(MCObjectRef *r_object)
{
	return s_interface -> context_query(kMCExternalContextVarMe, r_object);
}

MCError MCContextTarget(MCObjectRef *r_target)
{
	return s_interface -> context_query(kMCExternalContextVarTarget, r_target);
}

MCError MCContextResult(MCVariableRef *r_result)
{
	return s_interface -> context_query(kMCExternalContextVarResult, r_result);
}

MCError MCContextIt(MCVariableRef *r_it)
{
	return s_interface -> context_query(kMCExternalContextVarIt, r_it);
}

MCError MCContextCaseSensitive(bool *r_cs)
{
	return s_interface -> context_query(kMCExternalContextVarCaseSensitive, r_cs);
}

MCError MCContextConvertOctals(bool *r_co)
{
	return s_interface -> context_query(kMCExternalContextVarConvertOctals, r_co);
}

MCError MCContextItemDelimiter(uint32_t *r_del)
{
	return s_interface -> context_query(kMCExternalContextVarItemDelimiter, r_del);
}

MCError MCContextLineDelimiter(uint32_t *r_del)
{
	return s_interface -> context_query(kMCExternalContextVarLineDelimiter, r_del);
}

MCError MCContextColumnDelimiter(uint32_t *r_del)
{
	return s_interface -> context_query(kMCExternalContextVarColumnDelimiter, r_del);
}

MCError MCContextRowDelimiter(uint32_t *r_del)
{
	return s_interface -> context_query(kMCExternalContextVarRowDelimiter, r_del);
}

MCError MCContextDefaultStack(MCObjectRef *r_object)
{
	return s_interface -> context_query(kMCExternalContextVarDefaultStack, r_object);
}

MCError MCContextDefaultCard(MCObjectRef *r_object)
{
	return s_interface -> context_query(kMCExternalContextVarDefaultCard, r_object);
}

//////////

MCError MCRunOnMainThread(MCThreadCallback callback, void *state, MCRunOnMainThreadOptions options)
{
	return s_interface -> engine_run_on_main_thread(callback, state, options);
}

//////////
// MM-2014-03-18: [[ iOS 7.1 Support ]] Added new WaitRun and WaitBreak functions required by reviphone external for iOS 7.1 support.

MCError MCWaitRun(void)
{
	return s_interface -> wait_run(nil, 0);
}

MCError MCWaitBreak(void)
{
	return s_interface -> wait_break(nil, 0);
}

////////////////////////////////////////////////////////////////////////////////

const char *MCErrorToString(MCError p_error)
{
	static const char *s_engine_strings[] =
	{
		"",
		"out of memory",
		"not implemented",
		"no variable",
		"no value",
		"no iterator",
		"no buffer",
		"not an external temporary",
		"invalid value type",
		"not a boolean",
		"not a number",
		"not an integer",
		"not a string",
		"not a cstring",
		"not an array",
		"numeric overflow",
		"invalid convert octals option",
		"invalid case sensitive option",
		"invalid variable query",
		"invalid context query",
		"variable no longer exists",
		"invalid variable edit",
		"no object",
		"no object id",
		"no object message",
		"no object arguments",
		"malformed object chunk",
		"could not resolve object",
		"object does not exist",
		"no default stack"
	};

	if (p_error < sizeof(s_engine_strings)/sizeof(*s_engine_strings))
		return s_engine_strings[p_error];

	return "variable already bound";
}

////////////////////////////////////////////////////////////////////////////////

extern const char *kMCExternalName;
extern MCExternalStartupProc kMCExternalStartup;
extern MCExternalShutdownProc kMCExternalShutdown;
extern MCExternalHandler kMCExternalHandlers[];

#if defined(_LINUX) || defined(_MACOSX)
MCExternalInfo MC_EXTERNAL_EXPORT *MCExternalDescribe(void)  __attribute__((visibility("default")));
bool MC_EXTERNAL_EXPORT MCExternalInitialize(MCExternalInterface *p_interface)  __attribute__((visibility("default")));
void MC_EXTERNAL_EXPORT MCExternalFinalize(void)  __attribute__((visibility("default")));
#endif

MCExternalInfo MC_EXTERNAL_EXPORT *MCExternalDescribe(void)
{
	static MCExternalInfo s_info;
	s_info . version = 1;
	s_info . flags = 0;
	s_info . name = kMCExternalName;
	s_info . handlers = kMCExternalHandlers;
	return &s_info;
}

bool MC_EXTERNAL_EXPORT MCExternalInitialize(MCExternalInterface *p_interface)
{
	s_interface = p_interface;
	if (kMCExternalStartup != 0 && !kMCExternalStartup())
		return false;

	return true;
}

void MC_EXTERNAL_EXPORT MCExternalFinalize(void)
{
	if (kMCExternalShutdown != 0)
		kMCExternalShutdown();
}

////////////////////////////////////////////////////////////////////////////////
