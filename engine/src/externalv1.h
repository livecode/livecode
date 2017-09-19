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

#ifndef __MC_EXTERNALV1_H__
#define __MC_EXTERNALV1_H__

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


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
#include "mccontrol.h"
#include "notify.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "debug.h"

#include "external.h"

////////////////////////////////////////////////////////////////////////////////

typedef class MCExternalVariable *MCExternalVariableRef;
typedef MCObjectProxy<MCObject>* MCExternalObjectRef;

typedef void *MCExternalVariableIteratorRef;
typedef uint32_t MCExternalValueOptions;
typedef uint32_t MCExternalRunOnMainThreadOptions;
typedef void (*MCExternalThreadOptionalCallback)(void *state);
typedef void (*MCExternalThreadRequiredCallback)(void *state, int flags);

// MW-2013-06-14: [[ ExternalsApiV5 ]] Update the interface version.
// MW-2014-06-26: [[ ExternalsApiV6 ]] Update the interface version for unicode changes.
// MW-2016-02-17: [[ ExternalsApiV7 ]] Update the interface version for license check.
#define kMCExternalInterfaceVersion 7

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
    
    // SN-2015-01-19: [[ Bug 14057 ]] Update the enum to match
    //  the ones declared in lcidlc/include/LiveCode.h
    //  and lcidlc/src/Support.mm
#ifdef __HAS_CORE_FOUNDATION__
    kMCExternalValueOptionAsNSNumber = 17,
    kMCExternalValueOptionAsNSString = 18,
    kMCExternalValueOptionAsNSData = 19,
    kMCExternalValueOptionAsNSArray = 20,
    kMCExternalValueOptionAsNSDictionary = 21,
#endif
    // V6-ADDITIONS-END
    
    // SN-2015-01-19: [[ Bug 14057 ]] Added forgotten C-char value type
    kMCExternalValueOptionAsCChar = 22,
    
    // SN-2015-02-13:[[ Bug 14057 ]] Added CF-types (non-releasing)
    kMCExternalValueOptionAsCFNumber = 23,
    kMCExternalValueOptionAsCFString = 24,
    kMCExternalValueOptionAsCFData = 25,
    kMCExternalValueOptionAsCFArray = 26,
    kMCExternalValueOptionAsCFDictionary = 27,
    
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
    kMCExternalErrorNotASequence = 40,
    kMCExternalErrorCannotEncodeMap = 41,
	
	kMCExternalErrorUnlicensed = 42,
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
	
	// If fetching this accessor works, and it returns true then
	// the license check API is present.
	kMCExternalContextQueryHasLicenseCheck,
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

enum MCExternalLicenseType
{
	kMCExternalLicenseTypeNone = 0,
	kMCExternalLicenseTypeCommunity = 1000,
    kMCExternalLicenseTypeCommunityPlus = 1500,
    kMCExternalLicenseTypeIndy = 2000,
	kMCExternalLicenseTypeBusiness = 3000,
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
    
    MCExternalError (*context_query_legacy)(MCExternalContextQueryTag op, void *result);
    
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
    
    // SN-2015-01-26: [[ Bug 14057 ]] Update context query, to allow the user to set the return type
    MCExternalError (*context_query)(MCExternalContextQueryTag op, MCExternalValueOptions p_options, void *r_result); // V6
	
	// MW-2016-02-17: [[ LicenseCheck ]] Method to check the licensing of the engine
	MCExternalError (*license_check_edition)(unsigned int options, unsigned int min_edition); // V7
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
    
    // SN-2015-01-14: [[ Bug 14057 ]] Function added to uniformise the External conversion
    //  to string, and make it accessible outside of a MCExternalVariable
    static MCExternalError ConvertToString(MCValueRef t_value, MCExternalValueOptions options, MCStringRef& r_string);
    
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
    MCReferenceExternalVariable(MCContainer& value);
    ~MCReferenceExternalVariable(void);
    
    virtual bool IsTemporary(void);
    virtual bool IsTransient(void);
    virtual MCValueRef GetValueRef(void);
    virtual void SetValueRef(MCValueRef value);
    
private:
    MCContainer& m_container;
};

// MW-2014-01-22: [[ CompatV1 ]] This global holds the current handlers it-shim.
static MCReferenceExternalVariable *s_external_v1_current_it = nil;
// MW-2014-01-22: [[ CompatV1 ]] This global holds the result-shim.
static MCReferenceExternalVariable *s_external_v1_result = nil;
static MCContainer *s_external_v1_result_container = nil;

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
	
	void SetWasLicensed(bool p_value);
	
private:
    virtual bool Prepare(void);
    virtual bool Initialize(void);
    virtual void Finalize(void);
	
	MCExternalInfo *m_info;
	
	// This is true if startup succeeded (returned true) and license_fail was
	// not called.
	bool m_licensed : 1;
	
	// This is set to true if the last external handler call on this external
	// did not call license_fail.
	bool m_was_licensed : 1;
};

#endif
