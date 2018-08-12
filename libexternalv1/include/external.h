#ifndef __MC_EXTERNAL__
#define __MC_EXTERNAL__

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)

typedef signed int int32_t;
typedef unsigned int uint32_t;

#ifndef __cplusplus
typedef unsigned char bool;
enum
{
	false = 0,
	true = 1
};
#endif

#elif defined(__APPLE__)

typedef signed int int32_t;
typedef unsigned int uint32_t;

#ifndef __cplusplus

#ifdef __ppc__
typedef unsigned int bool;
#else
typedef unsigned char bool;
#endif
	
enum
{
	false = 0,
	true = 1
};

#endif
	
#else
	
	typedef signed int int32_t;
	typedef unsigned int uint32_t;
	
#ifndef __cplusplus

	typedef unsigned char bool;
	
	enum
	{
		false = 0,
		true = 1
	};
	
#endif
	
#endif

#ifndef nil
#define nil (0)
#endif

////////////////////////////////////////////////////////////////////////////////

// MCVariableRef is an opaque type given direct access to the contents of a
// Revolution variable.
typedef struct __MCVariable *MCVariableRef;

// MCVariableIteratorRef is an opaque type used by the variable iteration
// interface.
typedef struct __MCVariableIterator *MCVariableIteratorRef;

// MCObjectRef is an opaque type given access to an object handle.
typedef struct __MCObject *MCObjectRef;

// MCError assigns a unique value to each of the possible errors that can be
// generated while using the externals interface.
typedef enum MCError
{
	// No errors occurred, the operation succeeded.
	kMCErrorNone = 0,

	// Memory ran out while performing the operation.
	kMCErrorOutOfMemory = 1,

	// The requested operation, or operation with requested options is not
	// supported.
	kMCErrorNotImplemented = 2,

	// A nil reference was passed as the 'var' parameter to the operation.
	kMCErrorNoVariable = 3,
	
	// A nil pointer was passed as the 'value' parameter to the operation.
	kMCErrorNoValue = 4,

	// A nil pointer was passed as the 'iterator' parameter to the operation.
	kMCErrorNoIterator = 5,

	// Nil pointers were passed to the buffer parameters.
	kMCErrorNoBuffer = 6,

	// The given var cannot be destroyed as it was not created with Create.
	kMCErrorNotAnExternalTemporary = 7,

	// An unrecognised value type was requested.
	kMCErrorInvalidValueType = 8,

	// A request was made for a boolean value, but the coercion failed.
	kMCErrorNotABoolean = 9,

	// A request was made for a numeric value, but the coercion failed.
	kMCErrorNotANumber = 10,

	// A request was made for an integer value, but the coercion failed.
	kMCErrorNotAnInteger = 11,

	// A request was made for a string value, but the coercion failed.
	kMCErrorNotAString = 12,

	// A request was made for a cstring value, but the coercion failed.
	kMCErrorNotACString = 13,

	// An array operations was attempted on a non-array value.
	kMCErrorNotAnArray = 14,

	// The numeric value would not fit in desired type.
	kMCErrorNumericOverflow = 15,

	// An invalid setting of the 'convert octals' option was passed.
	kMCErrorInvalidCovertOctalsOption = 16,

	// An invalid setting of the 'case sensitive' option was passed.
	kMCErrorInvalidCaseSensitiveOption = 17,

	// An invalid setting was specified in the variable query function
	kMCErrorInvalidVariableQuery = 18,

	// An invalid setting was specified in the context query function
	kMCErrorInvalidContextQuery = 19,

	// An attempt was used to access/use a variable that no longer exists
	kMCErrorVariableNoLongerExists = 20,

	// An invalid edit operation was performed on a variable.
	kMCErrorVariableInvalidEdit = 21,

	// No object handle was passed to an object manipulation function
	kMCErrorNoObject = 22,

	// No long id string was passed to object resolve
	kMCErrorNoObjectId = 23,

	// No message string was passed to dispatch
	kMCErrorNoObjectMessage = 24,

	// No arguments were passed to dispatch when a non-zero arg count was passed
	kMCErrorNoObjectArguments = 25,

	// The chunk string passed to object resolve was malformed
	kMCErrorMalformedObjectChunk = 26,

	// The object chunk could not be resolved
	kMCErrorCouldNotResolveObject = 27,

	// The object has been deleted since the handle was acquired
	kMCErrorObjectDoesNotExist = 28,
	
	// There is no 'defaultStack' in the requested context
	kMCErrorNoDefaultStack = 29,

	////////

	kMCErrorVariableAlreadyBound = 32768
} MCError;

// MCValueOptions gives control over the interpretation of a variable's
// contents when performing various manipulation operations.
typedef uint32_t MCValueOptions;
enum
{
	// The 'value' is an MCVariableRef.
	kMCOptionAsVariable = 0,

	// The 'value' is a pointer to 'bool'
	kMCOptionAsBoolean = 1,

	// The 'value' is a pointer to 'int32_t'
	kMCOptionAsInteger = 2,

	// The 'value' is a pointer to 'uint32_t'
	kMCOptionAsCardinal = 3,

	// The 'value' is a pointer to 'double'
	kMCOptionAsReal = 4,

	// The 'value' is a pointer to 'MCString'
	kMCOptionAsString = 5,

	// The 'value' is a pointer to 'const char *'
	kMCOptionAsCString = 6,

	/////////
    
    // SN-2014-10-29: [[ Bug 13827 ]] Update libexternalv1 to match the Unicode-strings update
    // V6-ADDITIONS-START
    kMCOptionAsUTF8String = 7,
    kMCOptionAsUTF8CString = 8,
    kMCOptionAsUTF16String = 9,
    kMCOptionAsUTF16CString = 10,
    
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

	// Use the numberFormat settings in the calling context.
	kMCOptionNumberFormatDefault = 0 << 26,

	// Format the number as a decimal (%f stdio format).
	kMCOptionNumberFormatDecimal = 1 << 26,

	// Format the number using exponential notation (%e stdio format).
	kMCOptionNumberFormatScientific = 2 << 26,

	// Format the number using the shortest format (%g stdio format).
	kMCOptionNumberFormatGeneral = 3 << 26,

	/////////

	// Use the convertOctals setting from the calling context.
	kMCOptionConvertOctalsDefault = 0 << 28,

	// Convert as if convertOctals were true.
	kMCOptionConvertOctalsTrue = 1 << 28,

	// Convert as if convertOctals were false.
	kMCOptionConvertOctalsFalse = 2 << 28,

	//////////

	// Use the caseSensitive setting from the calling context.
	kMCOptionCaseSensitiveDefault = 0 << 30,

	// Do the operation as if caseSensitive were true.
	kMCOptionCaseSensitiveTrue = 1 << 30,

	// Do the operation as if caseSesntivie were false.
	kMCOptionCaseSensitiveFalse = 2 << 30
};

// MCDispatchType is used to determine how to dispatch a message
typedef enum MCDispatchType
{
	// Dispatch the message as a command
	kMCDispatchTypeCommand = 0,

	// Dispatch the message as a function
	kMCDispatchTypeFunction = 1
} MCDispatchType;

// MCDispatchStatus is used to return the status of a message dispatch
typedef enum MCDispatchStatus
{
	// The message was successfully sent and handled.
	kMCDispatchStatusHandled = 0,

	// The message was sent, but no handler existed in the message path.
	kMCDispatchStatusNotHandled = 1,

	// The message was sent, handled by at least one handler then passed.
	kMCDispatchStatusPassed = 2,

	// The message was sent and handled, but an error was raised during execution.
	kMCDispatchStatusError = 3,

	// The message was sent and handled, but 'exit to top' was performed during execution.
	kMCDispatchStatusExit = 4
} MCDispatchStatus;

// MCInvokeStatus is used to return the status of a script execution
typedef enum MCInvokeStatus
{
	// The script was successfully executed.
	kMCInvokeStatusSuccess = 0,
	
	// A parse error occurred while compiling the script
	kMCInvokeStatusParseError = 1,
	
	// An execution error occurred while running the script
	kMCInvokeStatusExecutionError = 2
} MCInvokeStatus;
	
// MCThreadCallback is the signature of the function to be invoked when a run on main thread
// callback is requested.
typedef void (*MCThreadCallback)(void *state);

// MCRunOnMainThreadOptions is used to pass options to the RunOnMainThread call.
typedef uint32_t MCRunOnMainThreadOptions;
enum
{
	// The callback will be invoked as soon as possible, and the calling thread
	// will block until it has been invoked.
	kMCRunOnMainThreadNow = 0 << 0,

	// The callback will be added to a queue to be called at the next available
	// opportunity. The calling thread will not block in this case.
	kMCRunOnMainThreadLater = 1 << 0
};

// MCString is used to store a string, unlike C strings, MCStrings can store
// NUL bytes as the length is explicitly stated.
typedef struct MCString
{
	char *buffer;
	uint32_t length;
} MCString;

////////////////////////////////////////////////////////////////////////////////
//
//  CONTAINER MANIPULATION FUNCTIONS
//

// Function:
//   MCVariableCreate
// Parameters:
//   (out) r_var - MCVariableRef *
// Errors:
//   OutOfMemory - memory ran out while trying to create the variable
// Semantics:
//   Create a new temporary variable, placing its handle in *r_var. The new
//   variable's lifetime is managed by using Retain/Release.
//
//   A variable created in this fashion can be used in exactly the same way as
//   any other variable and reports true for both IsTemporary and IsExternal.
//
//   The variable is created containing the 'undefined' value.
//
MCError MCVariableCreate(MCVariableRef *r_var);

// Function:
//   MCVariableRetain
// Parameters:
//   (in) var - MCVariableRef
// Errors:
//   NoVariable - the 'var' parameter is nil
// Semantics:
//   Increases the retain count of the given temporary variable. For non-
//   temporary variables, it has no effect.
//
MCError MCVariableRetain(MCVariableRef var);

// Function:
//   MCVariableRelease
// Parameters:
//   (in) var - MCVariableRef
// Error:
//   NoVariable - the 'var' parameter is nil
// Semantics:
//   Decreases the retain count of the given temporary variable. When the
//   retain count reaches zero, the variable is destroyed.
//
MCError MCVariableRelease(MCVariableRef var);

// Function:
//   MCVariableIsTransient
// Parameters:
//   (in) var - MCVariableRef
//   (out) r_temporary - bool *
// Errors:
//   NoVariable - the 'var' parameter is nil
// Semantics:
//   Returns true if 'var' does not represent a script-accessible variable. In
//   particular, variables created with 'Create' are transient as are parameters
//   computed from non-variable expressions.
//
MCError MCVariableIsTransient(MCVariableRef var, bool *r_transient);

// Function:
//   MCVariableIsTemporary
// Parameters:
//   (in) var - MCVariableRef
//   (out) r_external - bool *
// Errors:
//   NoVariable - the 'var' parameter is nil
// Semantics:
//   Returns 'true' if 'var' was created using Create.
//
MCError MCVariableIsTemporary(MCVariableRef var, bool *r_temporary);

// Function:
//   MCVariableIsAnArray
// Parameters:
//   (in) var - MCVariableRef
//   (out) r_array - bool *
// Errors:
//   NoVaraible - the 'var' parameter is nil
// Semantics:
//   Returns 'true' if 'var' is an array. (Note that the empty value
//   is taken to be the zero-element array).
//
MCError MCVariableIsAnArray(MCVariableRef var, bool *r_success);

// Function:
//   MCVariableClear
// Paramaeters:
//   (in) var - MCVariableRef
// Errors:
//   NoVariable - the 'var' parameter is nil
// Semantics:
//   Reset the variable's value to 'undefined', releasing any resources it
//   currently uses.
//
MCError MCVariableClear(MCVariableRef var);

// Function:
//   MCVariableExchange
// Parameters:
//   (in) var_a - MCVariableRef
//   (in) var_b - MCVariableRef
// Errors:
//   NoVariable - the 'var_a' or 'var_b' parameters are nil
// Semantics:
//   Swap the values held in the two variables. After this call, 'var_b' will
//   hold what 'var_a' held before the call, and 'var_a' will hold what 'var_a'
//   held before the call.
//
//   Note: Only the contents of the variables is affects - the temporary and/or
//     transient status of them remain unchanged.
//
MCError MCVariableExchange(MCVariableRef var_a, MCVariableRef var_b);

////////////////////////////////////////////////////////////////////////////////
//
//  CONTENTS MANIPULATION FUNCTIONS
//

// The following functions all allow access and manipulation of a variable's
// contents using a number of native types, and with full consideration to the
// numberFormat, convertOctals and caseSensitive Revolution context properties.
//
// Each of the functions take a pair of parameters: a variable handle (var) and
// a pointer to a native type variable, the particular type being determined by
// the options parameter.
//
// The native type options currently known are Boolean, Integer, Cardinal, Real,
// String and CString. These correspond to the native types 'bool', 'int32_t',
// 'uint32_t', 'double', 'MCString' and 'char *'. In all of these cases, the
// 'value' parameter must be a pointer to a native variable holding the
// appropriate type - even for CString's:
//
//   const char *t_my_cstring = "Hello World!";
//   // The following call is the correct usage...
//   MCVariableStore(var, kMCValueOptionAsCString, (void *)&t_my_cstring);
//   // The following call is incorrect and will likely crash-and-burn...
//   MCVariableStore(var, kMCValueOptionAsCString, (void *)t_my_cstring);
//
// Additionally, these functions can all take a value of type MCVariableRef and
// in this case, no indirection is necessary:
//
//   MCVariableRef t_my_temp_var;
//   MCVariableCreate(&t_my_temp_var);
//   // The following call is the correct usage...
//   MCVariableFetch(var, kMCValueOptionAsVariable, t_my_temp);
//   // The following call is incorrect and will do bad things...
//   MCVariableStore(var, kMCValueOptionAsVariable, (void *)&t_my_temp);
//
// [ The apparant discrepancy here vanishes when one considers these functions as
// requiring an 'l-value' ]
//
// When converting a number to a string, the 'numberFormat' comes into play. This
// option determines how the numeric string is formatted:
//   - kMCValueOptionDefaultNumberFormat uses 'the numberFormat' setting in the
//     current context.
//   - kMCValueOptionDecimalNumberFormat converts a number to a decimal string
//     without rounding: dddddddd.dddddddddddd
//   - kMCValueOptionScientificNumberFormat converts a number to a decimal string
//     with normalized exponent: d.ddddddddddde+/-ddddd
//   - kMCValueOptionCompactNumberFormat converts the number using either Decimal
//     or Scientific form, whichever is shorter.

// Function:
//   MCVariableStore
// Parameters:
//   (in) var - MCVariableRef
//   (in) options - MCValueOptions
//   (inout) value - (see above)
// Errors:
//   OutOfMemory - memory ran out while trying to complete the operation
//   NoVariable - the 'var' parameter was nil
//   NoValue - the 'value' parameter was nil
//   InvalidValueType - an unknown value type was passed in 'options'
// Semantics:
//   Store the given (native) type in the destination variable.
//
//   This call will automatically map the native types to the types that the
//   engine understands:
//     - boolean values get converted to the strings 'true' or 'false'
//     - integer and cardinal values get promoted to double
//
MCError MCVariableStore(MCVariableRef var, MCValueOptions options, void *value);

// Function:
//   MCVariableFetch
// Parameters:
//   (in) var - MCVariableRef
//   (in) options - MCValueOptions
//   (inout) value - (see above)
// Errors:
//   OutOfMemory - memory ran out while trying to complete the operation
//   NoVariable - the 'var' parameter was nil
//   NoValue - the 'value' parameter was nil
//   InvalidValueType - an unknown value type was passed in 'options'
//   NotABoolean - a boolean value was requested, but the variable's contents
//     cannot be converted to a bool.
//   NotANumber - a numeric value was requeted, but the variable's contents
//     cannot be converted to a number.
//   NotAnInteger - the numeric value of the variable has a fractional part.
//   NotAString - a string value was requested, but the variable's contents
//     cannot be converted to a string (i.e. it is undefined or an array).
//   NotACString - the string value of the variable contains NUL bytes and
//    so cannot be accessed as a C string.
//   NumberOverflow - a numeric value was requested, but the result will not
//     fit in the requested native type.
//   InvalidConvertOctalsOption - an unrecognized value for convertOctals option
//     was supplied.
//   InvalidCaseSensitiveOption - an unrecognized value for the caseSensitive
//     option was supplied.
// Semantics:
//   Fetch the contents of 'var' as the given (native) type.
//
//   When requesting a 'Boolean' type, the contents of the var must be either
//   the 'true' or 'false' strings. The comparison needed to determine this is
//   *not* affected by the caseSensitive option.
//
//   When requesting a numeric type, the contents of the var must be either a
//   number or a numeric string. By default a numeric string is one of:
//     - an integer
//     - a hexademical integer (has prefix '0x')
//     - a decimal with optional exponent
//   If the 'convertOctals' option is set, then an octal integer will also be
//   recognized (has prefix '0'). If an integral type is requested and the
//   (converted) number has a fractional part, an error is returned.
//
//   When requesting a cstring or string, the contents of the var must not
//   be an array nor undefined. Additionally, in the case of a CString, the
//   string value of the variable must not contain any NUL bytes. If the
//   contents of the variable is numeric, a conversion to a string will be
//   performed as per the 'numberFormat' option.
//
//   In the case of string and cstring, ownership of the underlying buffers
//   remains with the engine and their contents must not be modified. The
//   buffers will remain valid until the current external handler returns, or
//   the var is next accessed.
//
MCError MCVariableFetch(MCVariableRef var, MCValueOptions options, void *value);

MCError MCVariableAppend(MCVariableRef var, MCValueOptions options, void *value);

//////////

MCError MCVariableCountKeys(MCVariableRef var, uint32_t *r_count);
MCError MCVariableRemoveKey(MCVariableRef var, MCValueOptions key_type, void *key);
MCError MCVariableLookupKey(MCVariableRef var, MCValueOptions key_type, void *key, bool ensure, MCVariableRef* r_value);

MCError MCVariableIterateKeys(MCVariableRef var, MCVariableIteratorRef *iterator, MCValueOptions options, void *key, MCVariableRef* r_value);

////////////////////////////////////////////////////////////////////////////////

MCError MCContextMe(MCObjectRef *r_me);
MCError MCContextTarget(MCObjectRef *r_target);
MCError MCContextResult(MCVariableRef *r_result);
MCError MCContextIt(MCVariableRef *r_it);

MCError MCContextCaseSensitive(bool *r_is_case_sensitive);
MCError MCContextConvertOctals(bool *r_should_convert_octals);

MCError MCContextItemDelimiter(uint32_t *r_delimiter);
MCError MCContextLineDelimiter(uint32_t *r_delimiter);
MCError MCContextRowDelimiter(uint32_t *r_delimiter);
MCError MCContextColumnDelimiter(uint32_t *r_delimiter);

MCError MCContextDefaultStack(MCObjectRef *r_default_stack);
MCError MCContextDefaultCard(MCObjectRef *r_default_card);	
	
////////////////////////////////////////////////////////////////////////////////

MCError MCRunOnMainThread(MCThreadCallback callback, void *state, MCRunOnMainThreadOptions options);

////////////////////////////////////////////////////////////////////////////////
	
// MM-2014-03-18: [[ iOS 7.1 Support ]] Added new WaitRun and WaitBreak functions required by reviphone external for iOS 7.1 support.
MCError MCWaitRun(void);
MCError MCWaitBreak(void);
	
////////////////////////////////////////////////////////////////////////////////

MCError MCObjectResolve(const char *chunk, MCObjectRef *r_object);
MCError MCObjectExists(MCObjectRef object, bool *r_exists);
MCError MCObjectRetain(MCObjectRef object);
MCError MCObjectRelease(MCObjectRef object);
MCError MCObjectDispatch(MCObjectRef object, MCDispatchType type, const char *message, MCVariableRef *argv, uint32_t argc, MCDispatchStatus *r_status);

//MCError MCObjectExecute(MCObjectRef object, const char *script, MCVariableRef result, MCInvokeStatus *r_status);
//MCError MCObjectEvaluate(MCObjectRef object, const char *expr, MCVariableRef result, MCInvokeStatus *r_status);	
	
////////////////////////////////////////////////////////////////////////////////

const char *MCErrorToString(MCError t_error);

////////////////////////////////////////////////////////////////////////////////

#if !defined(__cplusplus)

#define MC_EXTERNAL_NAME(m_name) \
	const char *kMCExternalName = m_name;

#define MC_EXTERNAL_STARTUP(m_startup) \
	bool (*kMCExternalStartup)(void) = m_startup;

#define MC_EXTERNAL_SHUTDOWN(m_shutdown) \
	void (*kMCExternalShutdown)(void) = m_shutdown;

#define MC_EXTERNAL_HANDLERS_BEGIN \
	struct { uint32_t t; const char *n; bool (*f)(MCVariableRef *, uint32_t, MCVariableRef); } kMCExternalHandlers[] = { \

#define MC_EXTERNAL_COMMAND(m_name, m_function) \
	{ 1, m_name, m_function },

#define MC_EXTERNAL_FUNCTION(m_name, m_function) \
	{ 2, m_name, m_function },

#define MC_EXTERNAL_HANDLERS_END \
	{ 0, nil, nil } };

#elif !defined(__EXCEPTIONS)

#define MC_EXTERNAL_NAME(m_name) \
	extern "C" { const char *kMCExternalName = m_name; }

#define MC_EXTERNAL_STARTUP(m_startup) \
	extern "C" { bool (*kMCExternalStartup)(void) = m_startup; }

#define MC_EXTERNAL_SHUTDOWN(m_shutdown) \
	extern "C" { void (*kMCExternalShutdown)(void) = m_shutdown; }

#define MC_EXTERNAL_HANDLERS_BEGIN \
	extern "C" { struct { uint32_t t; const char *n; bool (*f)(MCVariableRef *, uint32_t, MCVariableRef); } kMCExternalHandlers[] = { \

#define MC_EXTERNAL_COMMAND(m_name, m_function) \
	{ 1, m_name, m_function },

#define MC_EXTERNAL_FUNCTION(m_name, m_function) \
	{ 2, m_name, m_function },

#define MC_EXTERNAL_HANDLERS_END \
	{ 0, nil, nil } }; }

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus) && defined(__EXCEPTIONS)

#include <exception>

class MCException: public std::exception
{
public:
	MCException(const char *p_why)
	{
		m_why = p_why;
	}
	
	virtual const char *what(void) const throw()
	{
		return m_why;
	}
	
private:
	const char *m_why;
};

template<class T> class MCVariableIterator;

class MCVariable
{
public:
	MCVariable(void);
	MCVariable(const MCVariable& other);
	~MCVariable(void);

	bool operator ! (void) const;

	MCVariable& operator = (const MCVariable& other);

	operator MCVariableRef (void) const;

	//////////

	void Make(void);

	//////////

	bool IsTemporary(void) const;
	bool IsTransient(void) const;

	//////////

	bool IsAnArray(void) const;

	//////////

	void Clear(void);

	void Exchange(const MCVariable& other);
	void Fetch(const MCVariable& other);
	void Store(const MCVariable& other);
	void Append(const MCVariable& other);

	/////////

	const char *FetchAsCString(void);
	MCString FetchAsString(void);
	int32_t FetchAsInteger(void);
	double FetchAsReal(void);
	bool FetchAsBoolean(void);

	bool TryFetch(const char*& r_cstring);
	bool TryFetch(MCString& r_string);
	bool TryFetch(const void*& r_buffer, uint32_t& r_length);
	bool TryFetch(int32_t& r_integer);
	bool TryFetch(double& r_real);
	bool TryFetch(bool& r_boolean);

	void Fetch(const char*& r_cstring);
	void Fetch(MCString& r_string);
	void Fetch(const void*& r_buffer, uint32_t& r_length);
	void Fetch(int32_t& r_integer);
	void Fetch(double& r_real);
	void Fetch(bool& r_boolean);

	/////////

	void Store(const char *cstring);
	void Store(const MCString& string);
	void Store(const void *buffer, uint32_t length);
	void Store(int32_t integer);
	void Store(double number);
	void Store(bool boolean);

	/////////

	void Append(const char *cstring);
	void Append(const MCString& string);
	void Append(const void *buffer, uint32_t length);
	void Append(int32_t integer);
	void Append(double number);
	void Append(bool boolean);

	/////////

	uint32_t CountKeys(void);

	void RemoveKey(const char *cstring);
	void RemoveKey(const MCString& string);

	MCVariable LookupKey(const char *cstring);
	MCVariable LookupKey(const MCString& string);

	MCVariable EnsureKey(const char *cstring);
	MCVariable EnsureKey(const MCString& string);

	static void Throw(MCError t_error);

private:
	MCVariable(MCVariableRef p_ref);
	void Take(MCVariableRef p_ref);

	MCVariableRef m_ref;

	template<class T> friend class MCVariableIterator;
};

template<>
class MCVariableIterator<const char *>
{
public:
	MCVariableIterator(const MCVariable& var);
	~MCVariableIterator(void);

	bool Next(void);

	const char *Key(void);

	MCVariable& Value(void);

private:
	MCVariableRef m_var_ref;
	MCVariableIteratorRef m_iterator_ref;
	const char *m_key;
	MCVariable m_value;
};

////////////////////////////////////////////////////////////////////////////////

inline MCVariable::MCVariable(void)
	: m_ref(0)
{
}

inline MCVariable::MCVariable(MCVariableRef p_ref)
	: m_ref(p_ref)
{
}

inline MCVariable::MCVariable(const MCVariable& other)
	: m_ref(0)
{
	if (other . m_ref != 0)
	{
		MCError t_error;
		t_error = MCVariableRetain(other . m_ref);
		if (t_error != kMCErrorNone)
			Throw(t_error);

		m_ref = other . m_ref;
	}
}

inline MCVariable::~MCVariable(void)
{
	if (m_ref != 0)
		MCVariableRelease(m_ref);
}

inline bool MCVariable::operator ! (void) const
{
	return m_ref == 0;
}

inline MCVariable& MCVariable::operator = (const MCVariable& p_other)
{
	if (m_ref != 0)
	{
		MCError t_error;
		t_error = MCVariableRelease(p_other . m_ref);
		if (t_error != kMCErrorNone)
			Throw(t_error);
		m_ref = 0;
	}

	if (p_other . m_ref != 0)
	{
		MCError t_error;
		t_error = MCVariableRetain(p_other . m_ref);
		if (t_error != kMCErrorNone)
			Throw(t_error);
		m_ref = p_other . m_ref;
	}

	return *this;
}

/////////

inline MCVariable::operator MCVariableRef (void) const
{
	return m_ref;
}

inline void MCVariable::Take(MCVariableRef p_ref)
{
	if (m_ref != 0)
		MCVariableRelease(m_ref);
	m_ref = p_ref;
}

/////////

inline void MCVariable::Make(void)
{
	if (m_ref != 0)
		Throw(kMCErrorVariableAlreadyBound);

	MCVariableRef t_new_var;
	MCError t_error;
	t_error = MCVariableCreate(&t_new_var);
	if (t_error != kMCErrorNone)
		Throw(t_error);

	m_ref = t_new_var;
}

inline bool MCVariable::IsTemporary(void) const
{
	MCError t_error;
	bool t_result;
	t_error = MCVariableIsTemporary(m_ref, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

inline bool MCVariable::IsTransient(void) const
{
	MCError t_error;
	bool t_result;
	t_error = MCVariableIsTransient(m_ref, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

/////////

inline bool MCVariable::IsAnArray(void) const
{
	MCError t_error;
	bool t_result;
	t_error = MCVariableIsAnArray(m_ref, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

/////////

inline void MCVariable::Clear(void)
{
	MCError t_error;
	t_error = MCVariableClear(m_ref);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Exchange(const MCVariable& other)
{
	MCError t_error;
	t_error = MCVariableExchange(m_ref, other . m_ref);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(const MCVariable& other)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsVariable, other . m_ref);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Fetch(const MCVariable& other)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsVariable, other . m_ref);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(const MCVariable& other)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsVariable, other . m_ref);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

/////////

inline void MCVariable::Store(const char *p_value)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsCString, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(const MCString& p_value)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsString, (void *)&p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(const void *p_buffer, uint32_t p_length)
{
	MCString t_value;
	t_value . buffer = (char *)p_buffer;
	t_value . length = p_length;

	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsString, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(int32_t p_value)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsInteger, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(double p_value)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsReal, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Store(bool p_value)
{
	MCError t_error;
	t_error = MCVariableStore(m_ref, kMCOptionAsBoolean, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

/////////

inline const char *MCVariable::FetchAsCString(void)
{
	MCError t_error;
	const char *t_result;
	t_error = MCVariableFetch(m_ref, kMCOptionAsCString, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

inline MCString MCVariable::FetchAsString(void)
{
	MCError t_error;
	MCString t_result;
	t_error = MCVariableFetch(m_ref, kMCOptionAsString, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

inline int32_t MCVariable::FetchAsInteger(void)
{
	MCError t_error;
	int32_t t_result;
	t_error = MCVariableFetch(m_ref, kMCOptionAsInteger, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

inline double MCVariable::FetchAsReal(void)
{
	MCError t_error;
	int32_t t_result;
	t_error = MCVariableFetch(m_ref, kMCOptionAsReal, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

inline bool MCVariable::FetchAsBoolean(void)
{
	MCError t_error;
	bool t_result;
	t_error = MCVariableFetch(m_ref, kMCOptionAsBoolean, &t_result);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_result;
}

/////////

inline bool MCVariable::TryFetch(const char*& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsCString, &r_value);
	if (t_error == kMCErrorNone)
		return true;
	if (t_error == kMCErrorNotAString || t_error == kMCErrorNotACString)
		return false;
	Throw(t_error);
	return false;
}

inline bool MCVariable::TryFetch(MCString& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsString, &r_value);
	if (t_error == kMCErrorNone)
		return true;
	if (t_error == kMCErrorNotAString)
		return false;
	Throw(t_error);
	return false;
}

inline bool MCVariable::TryFetch(const void *&r_buffer, uint32_t& r_length)
{
	MCString t_value;
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsString, &t_value);
	if (t_error == kMCErrorNone)
	{
		r_buffer = (const void *)t_value . buffer;
		r_length = t_value . length;
		return true;
	}
	if (t_error == kMCErrorNotAString)
		return false;
	Throw(t_error);
	return false;
}

inline bool MCVariable::TryFetch(int32_t& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsInteger, &r_value);
	if (t_error == kMCErrorNone)
		return true;
	if (t_error == kMCErrorNotANumber || t_error == kMCErrorNotAnInteger)
		return false;
	Throw(t_error);
	return false;
}

inline bool MCVariable::TryFetch(double& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsReal, &r_value);
	if (t_error == kMCErrorNone)
		return true;
	if (t_error == kMCErrorNotANumber)
		return false;
	Throw(t_error);
	return false;
}

inline bool MCVariable::TryFetch(bool& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsBoolean, &r_value);
	if (t_error == kMCErrorNone)
		return true;
	if (t_error == kMCErrorNotABoolean)
		return false;
	Throw(t_error);
	return false;
}

/////////

inline void MCVariable::Fetch(const char*& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsCString, &r_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Fetch(MCString& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsCString, &r_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Fetch(const void *& r_buffer, uint32_t& r_length)
{
	MCString t_value;
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsString, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);

	r_buffer = t_value . buffer;
	r_length = t_value . length;
}

inline void MCVariable::Fetch(int32_t& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsInteger, &r_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Fetch(double& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsReal, &r_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Fetch(bool& r_value)
{
	MCError t_error;
	t_error = MCVariableFetch(m_ref, kMCOptionAsBoolean, &r_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

/////////

inline void MCVariable::Append(const char *p_value)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsCString, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(const MCString& p_value)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsString, (void *)&p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(const void *p_buffer, uint32_t p_length)
{
	MCString t_value;
	t_value . buffer = (char *)p_buffer;
	t_value . length = p_length;

	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsString, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(int32_t p_value)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsInteger, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(double p_value)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsReal, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::Append(bool p_value)
{
	MCError t_error;
	t_error = MCVariableAppend(m_ref, kMCOptionAsBoolean, &p_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

/////////

inline uint32_t MCVariable::CountKeys(void)
{
	uint32_t t_count;
	MCError t_error;
	t_error = MCVariableCountKeys(m_ref, &t_count);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return t_count;
}

inline void MCVariable::RemoveKey(const char *p_key)
{
	MCError t_error;
	t_error = MCVariableRemoveKey(m_ref, kMCOptionAsCString, &p_key);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline void MCVariable::RemoveKey(const MCString& p_key)
{
	MCError t_error;
	t_error = MCVariableRemoveKey(m_ref, kMCOptionAsString, (void *)&p_key);
	if (t_error != kMCErrorNone)
		Throw(t_error);
}

inline MCVariable MCVariable::LookupKey(const char *p_key)
{
	MCVariableRef t_value;
	MCError t_error;
	t_error = MCVariableLookupKey(m_ref, kMCOptionAsCString, &p_key, false, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	if (t_value == 0)
		return MCVariable();
	return MCVariable(t_value);
}

inline MCVariable MCVariable::LookupKey(const MCString& p_key)
{
	MCVariableRef t_value;
	MCError t_error;
	t_error = MCVariableLookupKey(m_ref, kMCOptionAsString, (void *)&p_key, false, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	if (t_value == 0)
		return MCVariable();
	return MCVariable(t_value);
}

inline MCVariable MCVariable::EnsureKey(const char *p_key)
{
	MCVariableRef t_value;
	MCError t_error;
	t_error = MCVariableLookupKey(m_ref, kMCOptionAsCString, &p_key, true, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return MCVariable(t_value);
}

inline MCVariable MCVariable::EnsureKey(const MCString& p_key)
{
	MCVariableRef t_value;
	MCError t_error;
	t_error = MCVariableLookupKey(m_ref, kMCOptionAsString, (void *)&p_key, true, &t_value);
	if (t_error != kMCErrorNone)
		Throw(t_error);
	return MCVariable(t_value);
}

/////////

inline void MCVariable::Throw(MCError p_error)
{
	throw MCException(MCErrorToString(p_error));
}

////////////////////////////////////////////////////////////////////////////////

inline MCVariableIterator<const char *>::MCVariableIterator(const MCVariable& p_var)
{
	m_var_ref = p_var;
	m_iterator_ref = 0;
	m_key = 0;
}

inline MCVariableIterator<const char *>::~MCVariableIterator(void)
{
	if (m_iterator_ref != 0)
		MCVariableIterateKeys(m_var_ref, &m_iterator_ref, 0, 0, 0);
}

inline bool MCVariableIterator<const char *>::Next(void)
{
	MCError t_error;
	MCVariableRef t_value;
	t_error = MCVariableIterateKeys(m_var_ref, &m_iterator_ref, kMCOptionAsCString, &m_key, &t_value);
	if (t_error != kMCErrorNone)
		MCVariable::Throw(t_error);
	if (m_iterator_ref != 0)
	{
		m_value . Take(t_value);
		return true;
	}
	return false;
}

inline const char *MCVariableIterator<const char *>::Key(void)
{
	return m_key;
}

inline MCVariable& MCVariableIterator<const char *>::Value(void)
{
	return m_value;
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCExternalStartupProc)(void);
typedef void (*MCExternalShutdownProc)(void);
typedef bool (*MCExternalHandlerProc)(MCVariableRef *argv, uint32_t argc, MCVariableRef result);

template<MCExternalStartupProc u_startup> bool MCExternalStartupWrapper(void)
{
	try
	{
		if (!u_startup())
			return false;
	}
	catch(...)
	{
		return false;
	}

	return true;
}

template<MCExternalShutdownProc u_shutdown> void MCExternalShutdownWrapper(void)
{
	try
	{
		u_shutdown();
	}
	catch(...)
	{
	}
}

template<MCExternalHandlerProc u_handler> bool MCExternalHandlerWrapper(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	try
	{
		if (!((bool(*)(MCVariableRef*,int32_t,MCVariableRef))u_handler)(argv, argc, result))
			return false;
	}
	catch(std::exception& t_exception)
	{
		const char *t_reason;
		t_reason = t_exception . what();
		MCVariableStore(result, kMCOptionAsCString, &t_reason);
		return false;
	}
	catch(...)
	{
		const char *t_reason;
		t_reason = "unknown C++ exception";
		MCVariableStore(result, kMCOptionAsCString, &t_reason);
		return false;
	}

	return true;
}

#define MC_EXTERNAL_NAME(m_name) \
	extern "C" { const char *kMCExternalName = m_name; }

#define MC_EXTERNAL_STARTUP(m_startup) \
	extern "C" { bool (*kMCExternalStartup)(void) = MCExternalStartupWrapper<m_startup>; }

#define MC_EXTERNAL_SHUTDOWN(m_shutdown) \
	extern "C" { void (*kMCExternalShutdown)(void) = MCExternalShutdownWrapper<m_shutdown>; }

#define MC_EXTERNAL_HANDLERS_BEGIN \
	extern "C" { struct { uint32_t t; const char *n; bool (*f)(MCVariableRef *, uint32_t, MCVariableRef); } kMCExternalHandlers[] = { \

#define MC_EXTERNAL_COMMAND(m_name, m_function) \
	{ 1, m_name, MCExternalHandlerWrapper<m_function> },

#define MC_EXTERNAL_FUNCTION(m_name, m_function) \
	{ 2, m_name, MCExternalHandlerWrapper<m_function> },

#define MC_EXTERNAL_COMMAND_C(m_name, m_function) \
	{ 1, m_name, m_function },

#define MC_EXTERNAL_FUNCTION_C(m_name, m_function) \
	{ 2, m_name, m_function },

#define MC_EXTERNAL_HANDLERS_END \
	{ 0, nil, nil } }; }

#endif

////////////////////////////////////////////////////////////////////////////////

#endif
