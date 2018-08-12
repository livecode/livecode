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

#ifndef __LIVECODE__
#define __LIVECODE__

#if defined(__cplusplus)
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN64) || defined(_WIN32)
#define __WINDOWS__
#elif defined(__APPLE__)
#  include <TargetConditionals.h>
#  if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#    define __IOS__
#  elif TARGET_OS_MAC
#    define __MAC__
#  else
#    error unsupported platform
#  endif
#elif defined(__linux__)
#define __LINUX__
#elif defined(__ANDROID__)
// #define __ANDROID__
#else
#error unsupported platform
#endif
	
////////////////////////////////////////////////////////////////////////////////

typedef enum LCError
{
	// No errors occurred, the operation succeeded.
	kLCErrorNone = 0,
	
	// Memory ran out while performing the operation.
	kLCErrorOutOfMemory = 1,
	
	// The requested operation, or operation with requested options is not
	// supported.
	kLCErrorNotImplemented = 2,
	
	// A request was made for a boolean value, but the source was not convertible
	// to one.
	kLCErrorNotABoolean = 9,
	
	// A request was made for a numeric value, but the source was not convertible
	// to one.
	kLCErrorNotANumber = 10,
	
	// A request was made for a integer value, but the source was not convertible
	// to one.
	kLCErrorNotAnInteger = 11,
	
	// A request was made for a binary string value, but the source was not
	// convertible to one.
	kLCErrorNotABinaryString = 12,
	
	// A request was made for a string value, but the source was not convertible
	// to one.
	kLCErrorNotAString = 13,
	
	// A request was made for an array value, but the source was not convertible
	// to one.
	kLCErrorNotAnArray = 14,
	
	// A request was made for a number, but the source was too big to fit in the
	// requested type.
	kLCErrorNumberTooBig = 16,

	// No object handle was passed to an object manipulation function
	kLCErrorNoObject = 23,

	// No long id string was passed to object resolve
	kLCErrorNoObjectId = 24,

	// No message string was passed to dispatch
	kLCErrorNoObjectMessage = 25,

	// No arguments were passed to dispatch when a non-zero arg count was passed
	kLCErrorNoObjectArguments = 26,

	// The chunk string passed to object resolve was malformed
	kLCErrorMalformedObjectChunk = 27,

	// The object chunk could not be resolved
	kLCErrorCouldNotResolveObject = 28,

	// The object has been deleted since the handle was acquired
	kLCErrorObjectDoesNotExist = 29,
	
	// There is no 'defaultStack' in the requested context
	kLCErrorNoDefaultStack = 30,
	
	// An operation was aborted
	kLCErrorAborted = 31,
	
	// An operation caused an error to be thrown
	kLCErrorFailed = 32,
	
	// An operation caused 'exit to top' to be invkoed
	kLCErrorExited = 33,
	
	// The property argument to object set/get was nil
	kLCErrorNoObjectProperty = 34,
	
	// The value argument to object set/get was nil
	kLCErrorNoObjectPropertyValue = 35,
    
    // The query option passed to InterfaceQuery was unknown
    kLCErrorInvalidInterfaceQuery = 36,
    
    // Returned if LicenseCheck fails.
    kLCErrorUnlicensed = 42,
	
	//////////
	
	// A string could not be encoded as a (native) cstring
	kLCErrorCannotEncodeCString = 16384,
	
	// No wait object was passed to a wait manipulation function
	kLCErrorNoWait = 16385,
	
	// The wait is already running and cannot be run or destroyed
	kLCErrorWaitRunning = 16386,

	// An illegal set of options was passed to an array function
	kLCErrorBadValueOptions = 16387,
	
	// The path passed to an array function was too short (of zero-length)
	kLCErrorBadArrayPath = 16388,
	
	// The keys buffer passed to an 'AllKeys' function is too small
	kLCErrorArrayBufferTooSmall = 16389,
	
	// The object passed to LCImageAttach was not an image object.
	kLCErrorNotAnImageObject = 16390,
	
	// No image object was passed to a image manipulation function.
	kLCErrorNoImage = 16391,
	
	// The mask of an image object was requested, but none was attached.
	kLCErrorMaskNotAttached = 16392,
	
	// The value was requested as a (native) char, but the source was not convertible.
	kLCErrorNotAChar = 16393,
	
	// An array cannot be encoded as a sequence (i.e. NSArray).
	kLCErrorNotASequence = 16394,
	
	// A dictionary could not be encoded as an array.
	kLCErrorCannotEncodeMap = 16395
} LCError;

typedef struct LCBytes
{
	void *buffer;
	unsigned int length;
} LCBytes;
	
////////////////////////////////////////////////////////////////////////////////

// THREADS
//
// The LiveCode engine runs on a pair of threads and care has to be taken to
// ensure code is executed on the correct one.
//
// On startup, in addition to the main thread started by the OS - the 'system'
// thread; there is an auxillary thread - the 'script' or 'engine' thread.
//
// The engine thread is the one on which script executes and, more importantly,
// where the stack of current waits accumulate.
//
// The system thread is where most system code and calls must be made (most OSes
// only support access to frameworks such as UI and Video playback from the main
// application thread).
//
// Therefore, it is necessary in many cases for external handlers to switch
// between these two threads to perform specific functions. The main primitive
// for doing this is LCRunOnSystemThread (and LCRunBlockOnSystemThread - iOS
// only). This call ensures a given callback is executed immediately on the
// system thread.
//
// As the pair of threads run co-operatively, there is no synchronization issue
// to worry about - when LCRunOnSystemThread is called, the current thread will
// pause while the code is executed on the system thread and control will then
// return to the current thread.
//
// In cases where an external handler does not need to use anything other than
// 'native' context LC API calls then the external handler can be marked as
// 'tail' in the IDL. This causes the entire handler to be executed on the
// system thread - which can simplify coding in many cases. Note that it is illegal
// to call anything other than native context LC APIs - if you need to call
// such APIs from a handler it cannot be marked 'tail' and you must switch
// explicitly to the system thread to call system APIs (using LCRunOnSystemThread).

// CONTEXTS
//
// The LiveCode API specifies three different 'contexts' in which calls can be
// made. These are (from most restrictive to least):
//   Native context (any thread)
//   Dispatch context (engine thread only)
//   Handler context (engine thread only)
// The contexts are cumulative in the sense that handler context also counts as
// being dispatch context.
//
// Calls marked as being safe in 'Native' context can be made at any time and
// from any thread - it doesn't matter where the code is currently executing nor
// how it was invoked.
//
// Calls marked as being safe in 'Dispatch' context can only be made when the
// code is running in an environment provided by the engine. Such calls typically
// require access to engine structures, or their invocation may result in
// script being executed. For example, any code executing as the result of a
// handler entry point, or the callback invoked via LCPostOnMainThread is in
// dispatch context.
//
// Calls marked as being safe in 'Handler' context can only be made when the
// code is running as a result of a handler entry point invocation. Such calls
// typically only make sense in such a context as they require a LiveCode
// handler/object context to make sense.
//
// In general, the 'context safety' of API calls only comes into play when you
// are using multiple threads, or using LCPostOnMainThread.
//
// For reference, here is a list of all calls ordered by context safety:
//
// Native Context
//   LCObjectPost
//   LCWaitBreak
//   LCRunOnMainThread
//   LCPostOnMainThread
//   LCInterfaceQueryView (main thread only)
// Dispatch Context
//   LCObjectResolve
//   LCObjectExists
//   LCObjectRetain
//   LCObjectRelease
//   LCObjectSend
//   LCObjectGet
//   LCObjectSet
//   LCImageAttach
//   LCImageDetach
//   LCImageDescribe
//   LCImageDescribeMask
//   LCImageUpdate
//   LCImageUpdateRect
// Handler Context
//   LCExceptionRaise
//   LCContextMe
//   LCContextTarget
//   LCContextDefaultStack
//   LCContextDefaultCard
//   LCWaitCreate
//   LCWaitDestroy
//   LCWaitRun
//   LCWaitReset
//   LCInterfacePresentModalViewController
//   LCInterfaceDismissModalViewController

// VALUES
//
// A number of LiveCode API calls allow storing and fetching values such as
// integers, reals, strings and arrays. All of these calls operate in a uniform
// way with regards passing and returning native types in the appropriate way.
//
// Such calls will take an 'options' parameter and a (generic) 'value'
// parameter. The 'value' parameter is always of type 'void *' and must be
// passed a pointer to a (native) variable of the correct type as specified by
// 'options'. This corresponds as follows:
//     AsBoolean - pointer to bool variable
//     AsInteger - pointer to int variable
//     AsReal - pointer to double variable
//     AsCString - pointer to (const) char * variable
//     AsCData - pointer to (const) LCBytes variable
//     AsObjcNumber - pointer to NSNumber * variable
//     AsObjcString - pointer to NSString * variable
//     AsObjcData - pointer to NSData * variable
//
// When storing, or passing a value into an API call, the call will copy the
// specified value (in this case CString and CData values are considered
// 'const'). For example:
//     const char *t_my_string;
//     t_my_string = "Hello World!";
//     LCObjectSet(t_object, kLCValueOptionAsCString, "text", nil, &t_my_string);
//
// When fetching, or getting a value out an API call, the ownership depends on
// the type. For atomic types (bool, int, double) there is no ownership issue.
// For 'c' types (c-string and c-data), the ownership passes to the caller which
// is then responsible for calling 'free()' on the buffers. For obj-c types, the
// returned values are autoreleased and so the caller does not have to worry
// about explicitly releasing them. For example:
//     char *t_obj_string;
//     LCObjectGet(t_object, kLCValueOptionAsCString, "text", nil, &t_obj_string);
//     ... use t_obj_string ...
//     free(t_obj_string);
//
	
////////////////////////////////////////////////////////////////////////////////

// Function:
//   LCExceptionRaise
// Parameters:
//   (in) format - const char *
// Errors:
//   <none>
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Set the error to throw on return from the current external handler.
//
//   The thrown string is constructed using the printf-style formatting string
//   'format' and any subsequent arguments as appropriate.
//
void LCExceptionRaise(const char *format, ...);

////////////////////////////////////////////////////////////////////////////////
	
enum
{
	kLCValueOptionMaskAs = 0xff,
	kLCValueOptionMaskCaseSensitive = 3 << 30,
	kLCValueOptionMaskConvertOctals = 3 << 28,
	kLCValueOptionMaskNumberFormat = 3 << 26,
	
	// The 'value' parameter is a pointer to a bool variable.
	kLCValueOptionAsBoolean = 1,
	// The 'value' parameter is a pointer to an int variable.
	kLCValueOptionAsInteger = 2,
	// The 'value' parameter is a pointer to a double variable.
	kLCValueOptionAsReal = 4,
	// The 'value' parameter is a pointer to an LCBytes variable.
	kLCValueOptionAsCData = 5,
	// The 'value' parameter is a pointer to a char * variable (native encoding)
	kLCValueOptionAsCString = 6,
    
    
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Unicode strings
    // The 'value' parameter is a point to an LCBytes storing a UTF-8-encoded string
    kLCValueOptionAsUTF8CData = 7,
    // The 'value' parameter is a point to a char * variable (UTF-8 encoding)
    kLCValueOptionAsUTF8CString = 8,
    // The 'value' parameter is a point to an LCBytes storing a UTF-16-encoded string
    kLCValueOptionAsUTF16CData = 9,
    // The 'value' parameter is a point to a uint16_t * variable (UTF-16 encoding)
    kLCValueOptionAsUTF16CString = 10,

	// The 'value' parameter is a pointer to an LCArrayRef variable.
	kLCValueOptionAsLCArray = 16,
	// The 'value' parameter is a pointer to an NSNumber* variable.
	kLCValueOptionAsObjcNumber = 17,
	// The 'value' parameter is a pointer to an NSString* variable.
	kLCValueOptionAsObjcString = 18,
	// The 'value' parameter is a pointer to an NSData* variable.
	kLCValueOptionAsObjcData = 19,
	// The 'value' parameter is a pointer to an NSArray* variable.
	kLCValueOptionAsObjcArray = 20,
	// The 'value' parameter is a pointer to an NSDictionary *variable.
	kLCValueOptionAsObjcDictionary = 21,
	
	// The 'value' parameter is a pointer to a char variable (native encoding)
	kLCValueOptionAsCChar = 22,
    
    // SN-2015-02-13: [[ ExternalsApiV6 ]] Added CF-type arguments, which
    //  are NOT autoreleased when used as input
    // The 'value' parameter is a pointer to an CFNumberRef variable.
    kLCValueOptionAsCFNumber = 23,
    // The 'value' parameter is a pointer to an CFStringRef variable.
    kLCValueOptionAsCFString = 24,
    // The 'value' parameter is a pointer to an CFDataRef variable.
    kLCValueOptionAsCFData = 25,
    // The 'value' parameter is a pointer to an CFArrayRef variable.
    kLCValueOptionAsCFArray = 26,
    // The 'value' parameter is a pointer to an CFDictionaryRef variable.
    kLCValueOptionAsCFDictionary = 27,
	
	// Treat array keys as case-insensitive.
	kLCValueOptionCaseSensitiveFalse = 0 << 30,
	// Treat array keys as case-sensitive.
	kLCValueOptionCaseSensitiveTrue = 1 << 30,
	// Treat array keys case-sensitivity using the current caseSensitive value
	// in effect in the calling (LiveCode) handler.
	// Must only be used in handler context.
	kLCValueOptionCaseSensitiveFromContext = 3 << 30,
	
	// When converting a string to a number, ignore leading 0's.
	kLCValueOptionConvertOctalsFalse = 0 << 28,
	// When converting a string to a number, treat a leading 0 as indicating
	// the number is in base 8.
	kLCValueOptionConvertOctalsTrue = 1 << 28,
	// When converting a string to a number, treat a leading 0 as per the
	// current convertOctals value in effect in the calling (LiveCode) handler.
	// Must only be used in handler context.
	kLCValueOptionConvertOctalsFromContext = 3 << 28,
	
	// When converting a number to a string, use decimal (not scientific)
	// format.
	kLCValueOptionNumberFormatDecimal = 0 << 26,
	// When converting a number to a string, use standard scientific format
	//      x.xxxxxxxxxxe+-xxxxx
	kLCValueOptionNumberFormatScientific = 1 << 26,
	// When converting a number to a string, use decimal or scientific format
	// depending on whichever is more compact.
	kLCValueOptionNumberFormatCompact = 2 << 26,
	// When converting a number to a string, use the current numberFormat
	// settings in effect in the calling (LiveCode) handler.
	// Must only be used in handler context.
	kLCValueOptionNumberFormatFromContext = 3 << 26,	
};
	
enum
{
    kLCLicenseEditionNone = 0,
    kLCLicenseEditionCommunity = 1000,
    kLCLicenseEditionCommunityPlus = 1500,
    kLCLicenseEditionIndy = 2000,
    kLCLicenseEditionBusiness = 3000,
};
    
////////////////////////////////////////////////////////////////////////////////
	
typedef struct __LCArray *LCArrayRef;
	
LCError LCArrayCreate(unsigned int options, LCArrayRef* r_array);
LCError LCArrayRetain(LCArrayRef array);
LCError LCArrayRelease(LCArrayRef array);
	
LCError LCArrayCountKeys(LCArrayRef array, unsigned int options, unsigned int *r_count);
LCError LCArrayCountKeysOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, unsigned int *r_count);

LCError LCArrayListKeys(LCArrayRef array, unsigned int options, char **key_buffer, unsigned int key_buffer_size);
LCError LCArrayListKeysOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, char **key_buffer, unsigned int key_buffer_size);
	
LCError LCArrayRemoveKeys(LCArrayRef array, unsigned int options);
LCError LCArrayRemoveKeysOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length);
	
LCError LCArrayHasKey(LCArrayRef array, unsigned int options, const char *key, bool *r_exists);
LCError LCArrayHasKeyOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, const char *key, bool *r_exists);
LCError LCArrayHasKeyWithPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, bool *r_exists);

LCError LCArrayFetchKey(LCArrayRef array, unsigned int options, const char *key, void *r_value);
LCError LCArrayFetchKeyOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, const char *key, void *r_value);
LCError LCArrayFetchKeyWithPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, void *r_value);

LCError LCArrayLookupKey(LCArrayRef array, unsigned int options, const char *key, bool *r_exists, void *r_value);
LCError LCArrayLookupKeyOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, const char *key, bool *r_exists, void *r_value);
LCError LCArrayLookupKeyWithPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, bool *r_exists, void *r_value);

LCError LCArrayStoreKey(LCArrayRef array, unsigned int options, const char *key, void *value);
LCError LCArrayStoreKeyOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, const char *key, void *value);
LCError LCArrayStoreKeyWithPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, void *value);

LCError LCArrayRemoveKey(LCArrayRef array, unsigned int options, const char *key);
LCError LCArrayRemoveKeyOnPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length, const char *key);
LCError LCArrayRemoveKeyWithPath(LCArrayRef array, unsigned int options, const char **path, unsigned int path_length);
	
////////////////////////////////////////////////////////////////////////////////

#ifdef __ANDROID__

// Function:
//   LCAttachCurrentThread
// Parameters:
//   none
// Errors:
//   Failed - System was unable to attach the current thread to the Java VM
//
LCError LCAttachCurrentThread(void);

// Function:
//   LCDetachCurrentThread
// Parameters:
//   none
// Errors:
//   Failed - System was unable to detach the current thread from the Java VM
//
LCError LCDetachCurrentThread(void);

#endif

////////////////////////////////////////////////////////////////////////////////

typedef struct __LCObject *LCObjectRef;

// Parameters:
//   LCObjectResolve
// Parmaeters:
//   (in) chunk - const char *
//   (out) r_object - LCObjectRef
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NoObjectId - the 'chunk' parameter was nil
//   MalformedObjectChunk - a parse error occurred processing 'chunk'
//   CouldNotResolveObject - the object referred to by 'chunk' could not be found
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Parses the 'chunk' parameter as an object reference and attempts to locate
//   it. If no error occurs, a weak object handle is returned in 'r_object'.
//
//   The caller is responsible for calling 'LCObjectRelease' on the handle when
//   it is no longer needed.
//
//   A weak object handle is an indirect reference to a LiveCode object. It's
//   lifespan is independent of that of the target object and is controlled by
//   a reference count.
//
LCError LCObjectResolve(const char *chunk, LCObjectRef *r_object);

// Function:
//   LCObjectExists
// Parameters:
//   (in) object - LCObjectRef
//   (out) r_exists - bool
// Errors:
//   NoObject - the 'object' parameter was nil
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Checks to see if the object referred to by the handle 'object' still
//   exists. The 'r_exists' parameter is set to 'true' if it is still there, or
//   'false' otherwise.
//
//   
LCError LCObjectExists(LCObjectRef object, bool *r_exists);

// Function:
//   LCObjectRetain
// Parameters:
//   (in) object - LCObjectRef
// Errors:
//   NoObject - the 'object' parameter was nil
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Increase the reference count of the handle passed in 'object'.
//
LCError LCObjectRetain(LCObjectRef object);

// Function:
//   LCObjectRelease
// Parameters:
//   (in) object - LCObjectRef
// Errors:
//   NoObject - the 'object' parameter was nil
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Decrease the reference count of the handle passed in 'object'. If the
//   reference count reaches zero, the handle is destroyed and is no longer
//   valid.
//
LCError LCObjectRelease(LCObjectRef object);
	
// Function:
//   LCObjectSend
// Parameters:
//   (in) object - LCObjectRef
//   (in) message - const char *
//   (in) signature - const char *
//   (in) ... - variadic argument list
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NoObject - the 'object' parameter was nil
//   NoObjectMessage - the 'message' parameter was nil
//   ObjectDoesNotExist - the object handles target no longer exists
//   Exited - the message caused 'exit to top' to be called
//   Failed - the message caused an error to be thrown
// Context Safety:
//   Must be called in dispatch context.
// Semantics:
//   Sends 'message' to 'object' with the given parameters. The caller blocks
//   until the message has been handled.
// 
//   The parameter list is constructed based on the 'signature' c-string. Each
//   character in the signature determines the type of the subsequent arguments
//   and is used to convert them to a form suitable for LiveCode script.
//
//   The characters that are currently understood are:
//     'b' - the parameter is of 'bool' type, converts to 'true' or 'false'
//     'i' - the parameter is of 'int' type, converts to a number
//     'r' - the parameter is of 'double' type, converts to a number
//     'z' - the parameter is of 'c-string' type, converts to a (text) string
//     'u' - the parameter is of 'utf8 c-string' type, converts to a (text) string
//     'w' - the parameter is of 'utf16 c-string' type, converts to a (text) string
//	   'y' - the parameter is of 'c-data' type, converts to a (binary) string
//	   'v' - the parameter is of 'utf8 c-data' type, converts to a (text) string
//	   't' - the parameter is of 'utf16 c-data' type, converts to a (text) string
//     'c' - the parameter is of 'char' type, converts to a (text) string
//     'N' - the parameter is of 'NSNumber*' type, converts to a number
//     'S' - the parameter is of 'NSString*' type, converts to a (text) string
//     'D' - the parameter is of 'NSData*' type, converts to a (binary) string
//     'A' - the parameter is of 'NSArray*' type, converts to a sequentially indexed array
//     'M' - the parameter is of 'NSDictionary*' type, converts to a key/value map array
//
//   The parameters appear in the resulting LiveCode message in the same order
//   that they appear in the signature.
//
//   The 'z' & 'u' types should be passed a 'const char *' (zero-terminated) string.
//   The 'w' type should be passed a 'const uint16_t *' (zero-terminated) string.
//   The 'y','v' & 't' types should be passed a 'const LCBytes *' type.
//
LCError LCObjectSend(LCObjectRef object, const char *message, const char *signature, ...);
	
// Function:
//   LCObjectPost
// Parameters:
//   (in) object - LCObjectRef
//   (in) message - const char *
//   (in) signature - const char *
//   (in) ... - variadic argument list
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NoObject - the 'object' parameter was nil
//   NoObjectMessage - the 'message' parameter was nil
//   ObjectDoesNotExist - the object handles target no longer exists
// Context Safety:
//   May be called in universal context.
// Semantics:
//   Appends an event to the internal event queue. When the event is dispatched
//   'message' is sent to the target object with the given parameters.
//
//   Note: Posting from an auxillary thread will block until the main thread
//   reaches a suitable point to process the request and schedule the event.
//
LCError LCObjectPost(LCObjectRef object, const char *message, const char *signature, ...);

// Function:
//   LCObjectGet
// Parameters:
//   (in) object - LCObjectRef
//   (in) options - unsigned int
//   (in) property - const char *
//   (in) key - const char *
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NoObject - the 'object' parameter was nil
//   NoObjectProperty - the 'property' parameter was nil
//   NoObjectPropertyValue - the 'value' parameter was nil
//   ObjectDoesNotExist - the object handles target no longer exists
//   Exited - the message caused 'exit to top' to be called
//   Failed - the message caused an error to be thrown
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called in dispatch context.
// Semantics:
//   Fetches the value of the given (array) property from the specified object
//   and returns the value in the way specified by 'options'. The usage of
//   options and values is as described in the section on 'values'.
//
//   This method works in an identical way to property fetching in script.
//
//   i.e.
//     LCObjectGet(object, ..., property, NULL, value)
//   is the same as:
//     put the <property> of <object> into <value>
//
//   i.e.
//     LCObjectGet(object, ..., property, key, value)
//   is the same as:
//     put the <property>[<key>] of <object> into <value>
//
//   Note that just like the script analogs, if <property> is not an engine
//   property custom properties are used (with getprop messages dispatched as
//   needed), and thus the current custom property set of the object comes into
//   effect when required.
//
LCError LCObjectGet(LCObjectRef object, unsigned int options, const char *property, const char *key, void *value);

// Function:
//   LCObjectSet
// Parameters:
//   (in) object - LCObjectRef
//   (in) options - unsigned int
//   (in) property - const char *
//   (in) key - const char *
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NoObject - the 'object' parameter was nil
//   NoObjectProperty - the 'property' parameter was nil
//   NoObjectPropertyValue - the 'value' parameter was nil
//   ObjectDoesNotExist - the object handles target no longer exists
//   Exited - the message caused 'exit to top' to be called
//   Failed - the message caused an error to be thrown
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called in dispatch context.
// Semantics:
//   Sets the value of the given (array) property to the given value in the way
//   specified by 'options'. The usage of options and values is as described in
//   the section on 'values'.
//
//   This method works in an identical way to property setting in script.
//
//   i.e.
//     LCObjectSet(object, ..., property, NULL, value)
//   is the same as:
//     set the <property> of <object> to <value>
//
//   i.e.
//     LCObjectSet(object, ..., property, key, value)
//   is the same as:
//     set the <property>[<key>] of <object> to <value>
//
//   Note that just like the script analogs, if <property> is not an engine
//   property custom properties are used (with setprop messages dispatched as
//   needed), and thus the current custom property set of the object comes into
//   effect when required.
//
LCError LCObjectSet(LCObjectRef object, unsigned int options, const char *property, const char *key, void *value);

////////////////////////////////////////////////////////////////////////////////

// Function:
//   LCContextMe
// Parameters:
//   (out) r_me - LCObjectRef
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a weak object handle to object's whose script invoked the external
//   handler.
//
//   The caller is responsible for calling 'LCObjectRelease' on the handle when
//   it is no longer needed.
//
LCError LCContextMe(LCObjectRef *r_me);

// Function:
//   LCContextTarget
// Parameters:
//   (out) r_target - LCObjectRef
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a weak object handle to 'the target'.
//
//   The caller is responsible for calling 'LCObjectRelease' on the handle when
//   it is no longer needed.
//
LCError LCContextTarget(LCObjectRef *r_target);

// Function:
//   LCContextDefaultStack
// Parameters:
//   (out) r_default_stack - LCObjectRef
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a weak object handle to 'the defaultStack'.
//
//   The caller is responsible for calling 'LCObjectRelease' on the handle when
//   it is no longer needed.
//
LCError	LCContextDefaultStack(LCObjectRef *r_default_stack);

// Function:
//   LCContextDefaultCard
// Parameters:
//   (out) r_default_card - LCObjectRef
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a weak object handle to 'this card of the defaultStack'.
//
//   The caller is responsible for calling 'LCObjectRelease' on the handle when
//   it is no longer needed.
//
LCError LCContextDefaultCard(LCObjectRef *r_default_card);	

// Function:
//   LCContextCaseSensitive
// Parameters:
//   (out) r_case_sensitive - bool
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'caseSensitive' property.
//
LCError LCContextCaseSensitive(bool* r_case_sensitive);

// Function:
//   LCContextConvertOctals
// Parameters:
//   (out) r_convert_octals - bool
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'convertOctals' property.
//
LCError LCContextConvertOctals(bool *r_convert_octals);

// Function:
//   LCContextWholeMatches
// Parameters:
//   (out) r_whole_matches - bool
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'wholeMatches' property.
//
LCError LCContextWholeMatches(bool* r_whole_matches);

// Function:
//   LCContextItemDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'itemDelimiter' property.
//
LCError LCContextItemDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextLineDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'lineDelimiter' property.
//
LCError LCContextLineDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextColumnDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'columnDelimiter' property.
//
LCError LCContextColumnDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextRowDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Returns a the current value of the local 'rowDelimiter' property.
//
LCError LCContextRowDelimiter(unsigned int options, void *r_value);

    
// Function:
//   LCContextUnicodeItemDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
//   This function will only work if the external is called by an engine post-7.0
//      and return NotImplemented for the earlier versions.
//   The returned value must be free'd by the caller of this function.
// Semantics:
//   Returns a the current value of the local 'itemDelimiter' property.
//
LCError LCContextUnicodeItemDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextUnicodeLineDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
//   This function will only work if the external is called by an engine post-7.0
//      and return NotImplemented for the earlier versions.
//   The returned value must be free'd by the caller of this function.
// Semantics:
//   Returns a the current value of the local 'lineDelimiter' property.
//
LCError LCContextUnicodeLineDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextUnicodeColumnDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
//   This function will only work if the external is called by an engine post-7.0
//      and return NotImplemented for the earlier versions.
//   The returned value must be free'd by the caller of this function.
// Semantics:
//   Returns a the current value of the local 'columnDelimiter' property.
//
LCError LCContextColumnDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextUnicodeRowDelimiter
// Parameters:
//   (in) options - unsigned int
//   (out) value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
//   This function will only work if the external is called by an engine post-7.0
//      and return NotImplemented for the earlier versions.
//   The returned value must be free'd by the caller of this function.
// Semantics:
//   Returns a the current value of the local 'rowDelimiter' property.
//
LCError LCContextRowDelimiter(unsigned int options, void *r_value);

// Function:
//   LCContextResult
// Parameters:
//   (in) options - unsigned int
//   (out) r_value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Fetches the value of 'the result' in the way specified by 'options'.
//   The usage of options and values is as described in the section on 'values'.
//
LCError LCContextResult(unsigned int options, void *r_value);

// Function:
//   LCContextResult
// Parameters:
//   (in) options - unsigned int
//   (out) r_value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Fetches the value of 'it' in the way specified by 'options'. The usage of
//   options and values is as described in the section on 'values'.
//
LCError LCContextIt(unsigned int options, void *r_value);

// Function:
//   LCContextEvaluate
// Parameters:
//   (in) expression - const char *
//   (in) options - unsigned int
//   (out) r_value - depends on options
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   Exited - the script evaluation caused 'exit to top' to be invoked
//   Failed - the script evaluation caused an error to be thrown
//   NotABoolean - the value was requested as a boolean, and it is not a boolean
//   NotANumber - the value was requested as a number, and it is not a number
//   NotAnInteger - the value was requested as an integer, and it is not an
//     integer
//   NotABinaryString - the value was requested as binary data, and it is not
//     binary data
//   NotAString - the value was requested as a string, and it is not a string
//   NotAnArray - the value was requested as an array, and it is not an array
//   NotAChar - the value was requested as a char, and it is not a char
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Evaluates the given expression in the context of the calling handler, and
//   returns the value in the way specified by 'options'. The usage of options
//   and values is as described in the section on 'values'.
//
LCError LCContextEvaluate(const char *expression, unsigned int options, void *r_value);

// Function:
//   LCContextExecute
// Parameters:
//   (in) statements - const char *
//   (in) options - unsigned int
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
//   Exited - the script evaluation caused 'exit to top' to be invoked
//   Failed - the script evaluation caused an error to be thrown
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Executes the given sequence of statements in the context of the calling
//   handler.
//
LCError LCContextExecute(const char *statements, unsigned int options);

////////////////////////////////////////////////////////////////////////////////
	
typedef struct __LCWait *LCWaitRef;

enum
{
	// The wait should block all messages from being dispatched, background
	// engine processes still function however (cf. wait without messages).
	kLCWaitOptionBlocking = 0,
	// The wait should allow all messages to be dispatched, including those
	// result in LiveCode script being executed (cf. wait with messages).
	kLCWaitOptionDispatching = 1 << 0,
};

// Function:
//   LCWaitCreate
// Parameters:
//   (in) options - unsigned int
// Errors:
//   OutOfMemory - memory ran out while attempting to perform the operation
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Creates and returns a handle to a 'Wait' object, allowing pausing of script
//   execution in both blocking or dispatching modes.
//
//   The caller is responsible for callback 'LCWaitDestroy' on the handle when
//   it is no longer needed.
//
LCError LCWaitCreate(unsigned int options, LCWaitRef *r_wait);
	
// Function:
//   LCWaitRetain
// Parameters:
//   (in) wait - LCWaitRef
// Errors:
//   NoWait - the 'wait' parameter was nil
// Context Safety:
//   May be called in any context from any thread.
// Semantics:
//   Increases the reference count for the given wait object.
//
LCError LCWaitRetain(LCWaitRef wait);
	
// Function:
//   LCWaitRelease
// Parameters:
//   (in) wait - LCWaitRef
// Errors:
//   NoWait - the 'wait' parameter was nil
// Context Safety:
//   May be called in any context from any thread.
// Semantics:
//   Reduces the reference count for the given wait object, destroying it if the
//   reference count reaches zero.
//
LCError LCWaitRelease(LCWaitRef wait);

// Function:
//   LCWaitIsRunning
// Parameters:
//   (in) wait - LCWaitRef
//   (out) running - bool
// Errors:
//   NoWait - the 'wait' parameter was nil
// Context Safety:
//   May be called in any context from any thread.
// Semantics:
//   Returns 'true' if the wait object is currently running, false otherwise.
//
//   Note that although this call is thread-safe, the return value may be
//   immediately invalid if the wait object finishes just after its current
//   state is fetched.
//
LCError LCWaitIsRunning(LCWaitRef wait, bool *r_running);
	
// Function:
//   LCWaitRun
// Parameters:
//   (in) wait - LCWaitRef
// Errors:
//   NoWait - the 'wait' parameter was nil
//   WaitRunning - the 'wait' object is already running
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Blocks the caller's execution and runs the main application event loop
//   until either 'LCWaitBreak' is called on the wait object, or the wait is
//   aborted (due to application exit).
//
//   If 'LCWaitBreak' has already been called on the wait object LCWaitRun
//   will return without running the event loop.
//
//   To re-use the wait object after it has been run, use LCWaitReset.
//
LCError LCWaitRun(LCWaitRef wait);

// Function:
//   LCWaitBreak
// Parameters:
//   (in) wait - LCWaitRef
// Errors:
//   NoWait - the 'wait' parameter was nil
// Context Safety:
//   May be called in any context from any thread.
// Semantics:
//   Marks the given wait object as being 'broken' and interrupts the event loop
//   if LCWaitRun was invoked on the wait.
//
LCError LCWaitBreak(LCWaitRef wait);

// Function:
//   LCWaitReset
// Parameters:
//   (in) wait - LCWaitRef
// Errors:
//   NoWait - the 'wait' parameter was nil
//   WaitRunning - the 'wait' object is already running
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Resets the wait object so that it can be run again.
//
LCError LCWaitReset(LCWaitRef wait);

////////////////////////////////////////////////////////////////////////////////
    
// SN-2015-01-28: [[ Bug 13781 ]] Image functions disabled as they were using
//  imagePixmapId and maskPixmapId, which are no longer working reliably.

/*
typedef enum LCImageRasterFormat
{
	// The raster uses the gray colorspace with 1-bit per pixel (i.e. it is a
	// 1-bit mask).
	kLCImageRasterFormat_G_1 = 1,
	// The raster uses the gray colorspace with 8-bits per pixel (i.e. it is an
	// alpha channel).
	kLCImageRasterFormat_G_8,
	
	// The raster consists of 8-bits per components, with or without alpha
	// channel in various byte-orders.
	kLCImageRasterFormat_RGBx_8888,
	kLCImageRasterFormat_RGBA_8888,
	kLCImageRasterFormat_xRGB_8888,
	kLCImageRasterFormat_ARGB_8888,
	kLCImageRasterFormat_BGRx_8888,
	kLCImageRasterFormat_BGRA_8888,
	kLCImageRasterFormat_xBGR_8888,
	kLCImageRasterFormat_ABGR_8888,
} LCImageRasterFormat;
	
typedef struct LCImageRaster
{
	// The format of the raster.
	LCImageRasterFormat format;
	// The width of the raster in pixels.
	int width;
	// The height of the raster in pixels.
	int height;
	// The number of bytes that takes you from one scanline in the raster to
	// the next one.
	int stride;
	// The bits that actually makes up the raster.
	void *data;
} LCImageRaster;

typedef struct __LCImage *LCImageRef;
	
enum
{
	// The image should have no mask.
	kLCImageOptionSolid = 0 << 0,
	// The image should have a one-bit mask.
	kLCImageOptionMasked = 1 << 0,
};

// Function:
//   LCImageAttach
// Parameters:
//   (in) object - LCObjectRef
//   (in) options - unsigned int
//   (out) r_image - LCImageRef
// Errors:
//   OutOfMemory - there was not enough memory to complete the operation.
//   ObjectDoesNotExist - the given object no longer exists.
//   NotAnImageObject - the given object is not an image object.
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Creates and attaches rasters to the given image object allowing direct
//   access to the bits of the main (color) raster, and it's mask (if any).
//
//   The attachment locks the size of the image leaving complete control of
//   the contents to the external. When detached, the image returns to its
//   previous state (including contents).
//
LCError LCImageAttach(LCObjectRef object, unsigned int options, LCImageRef *r_image);

// Function:
//   LCImageDetach
// Parameters:
//   (in) image - LCImageRef
// Errors:
//   NoImage - the 'image' parameter was nil.
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Detaches from an image, previously attached to using LCImageAttach.
//
LCError LCImageDetach(LCImageRef image);

// Function:
//   LCImageDescribe
// Paramaeters:
//   (in) image - LCImageRef
//   (out) r_raster - LCImageRaster
// Errors:
//   NoImage - the 'image' parameter was nil.
// Context Safety:
//   Can be called from any context.
// Semantics:
//   Returns the details of the image's main raster, allowing access to the
//   RGB values that make up the image.
//
//   The raster is always in the native format provided by the engine on the
//   the current platform.
//
//   On iOS this is 8-bit components, organised in memory as RGBx where the
//   last byte is ignored.
//
LCError LCImageDescribe(LCImageRef image, LCImageRaster *r_raster);
	
// Function:
//   LCImageDescribeMask
// Paramaeters:
//   (in) image - LCImageRef
//   (out) r_raster - LCImageRaster
// Errors:
//   NoImage - the 'image' parameter was nil.
// Context Safety:
//   Can be called from any context.
// Semantics:
//   Returns the details of the image's mask raster, allowing access to the
//   1-bit mask that controls transparency of the image.
//
//   The raster is always in the 1-bit gray format.
//	
LCError LCImageDescribeMask(LCImageRef image, LCImageRaster *r_raster);
	
// Function:
//   LCImageUpdate
// Paramaeters:
//   (in) image - LCImageRef
// Errors:
//   NoImage - the 'image' parameter was nil.
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Mark the image object to which 'image' is attached as needing a redraw.
//	
LCError LCImageUpdate(LCImageRef image);
	
// Function:
//   LCImageUpdateRect
// Paramaeters:
//   (in) image - LCImageRef
// Errors:
//   NoImage - the 'image' parameter was nil.
// Context Safety:
//   Must be called from dispatch context.
// Semantics:
//   Mark the image object to which 'image' is attached as needing a redraw but
//   only within the specified rectangle. The co-ordinates are relative to the
//   top-left of the image.
//	
LCError LCImageUpdateRect(LCImageRef image, int top, int left, int right, int bottom);

*/
	
////////////////////////////////////////////////////////////////////////////////

typedef void (*LCRunOnSystemThreadCallback)(void *state);

// Function:
//   LCRunOnSystemThread
// Parameters:
//   (in) callback - LCRunOnSystemThreadCallback
//   (in) state - void *
// Errors:
//   OutOfMemory - there was not enough memory to service the request.
// Context Safety:
//   May be called in any context from the system or engine thread.
// Semantics:
//   Executes the given callback on the system thread as soon as possible.
//
//   If called from the system thread, the callback will be invoked directly.
//
//   If called from the engine thread, the engine will temporarily jump to the
//   system thread to invoke the callback.
//
//   The context in which the callback is invoked should be considered native.
//   In particular, it is illegal to use LCObjectGet, LCObjectSet, LCObjectSend
//   and LCWaitRun - LCObjectPost is fine, however.
//
//   Note that when run in a version of the engine prior to 5.5.2-dp-1, this
//   call executes the callback directly - this means a single version of the
//   external will suffice to run in 5.5/5.5.1 and 5.5.2.
//
LCError LCRunOnSystemThread(LCRunOnSystemThreadCallback callback, void *state);

// Function:
//   LCRunBlockOnSystemThread
// Parameters:
//   (in) callback - void (^)(void)
// Errors:
//   OutOfMemory - there was not enough memory to service the request.
// Context Safety:
//   May be called in any context from the system or engine thread.
// Semantics:
//   Identical to LCRunOnSystemThread except that an Objective-C block is invoked
//   rather than a function.
//
#if defined(__OBJC__) && defined(__BLOCKS__)
LCError LCRunBlockOnSystemThread(void (^callback)(void));
#endif
	
////////////////////////////////////////////////////////////////////////////////
	
typedef void (*LCRunOnMainThreadCallback)(void *state);

enum
{
	// Blocks the caller until the callback has been executed.
	kLCRunOnMainThreadOptionWait = 0,
	// Posts the callback request and returns immediately.
	kLCRunOnMainThreadOptionDontWait = 1 << 0,
};

// Function:
//   LCRunOnMainThread
// Parameters:
//   (in) options - unsigned int
//   (in) callback - LCRunOnMainThreadCallback
//   (in) state - void *
// Errors:
//   OutOfMemory - there was not enough memory to service the request.
// Context Safety:
//   May be called in any context.
// Semantics:
//   Executes the given callback on the main (engine) thread as soon as is
//   possible.
//
//   If the call is made from the main thread and 'Wait' is passed as an option,
//   the callback is invoked before LCRunOnMainThread returns; otherwise the
//   callback is scheduled to run at the next invocation of the run loop (cf.
//   send ... to me in 0 millisecs).
//
//   If the call is made from an auxillary thread then the callback is
//   scheduled on the main thread's run loop to be executed at the next possible
//   opportunity. If 'Wait' is specified in this case, the caller will block
//   until the main thread has executed the callback; otherwise the caller will
//   block only until the request has been posted.
//
//   When invoked, the context of the callback is native - only certain API
//   calls can be made.
//
LCError LCRunOnMainThread(unsigned int options, LCRunOnMainThreadCallback callback, void *state);

#if defined(__OBJC__) && defined(__BLOCKS__)
LCError LCRunBlockOnMainThread(unsigned int options, void (^callback)(void));
#endif

////////////////////////////////////////////////////////////////////////////////
	
typedef void (*LCPostOnMainThreadCallback)(void *state, unsigned int flags);
	
enum
{
	// The callback request was cancelled as the event was never processed.
	kLCPostOnMainThreadFlagCancelled = 1 << 0,
};

// Function:
//   LCPostOnMainThread
// Parameters:
//   (in) options - unsigned int
//   (in) callback - LCPostOnMainThreadCallback
//   (in) context - void *
// Errors:
//   OutOfMemory - there was not enough memory to service the request.
// Context Safety:
//   May be called in any context.
// Semantics:
//   Posts an event to the event queue that when dispatched causes the given
//   callback to be invoked. If the event is never reached (due to application
//   exit), the callback is still invoked, but with the 'cancelled' flag this
//   allows the callback to clean-up any dynamic state associated with it.
//
//   The call will block until the request has been successfully posted to the
//   event queue and then return.
//
//   When invoked, the callback is executed in dispatch context - all but
//   handler context requring API calls can be made.
//
LCError LCPostOnMainThread(unsigned int options, LCPostOnMainThreadCallback callback, void *state);
	
#if defined(__OBJC__) && defined(__BLOCKS__)
LCError LCPostBlockOnMainThread(unsigned int options, void (^callback)(void));
#endif
	
////////////////////////////////////////////////////////////////////////////////

// Function:
//   LCLicenseCheckEdition
// Parameters:
//   (in) min_edition
//
// Errors:
//   Unlicensed - the license check failed.
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Checks that the engine is licensed with at min_edition edition level.
//
//   If the check fails, then an 'unlicensed' error will be returned when control
//   returns to the engine.
//
//   If a license check fails during the external's initialize handler then the
//   whole external is marked as unlicensed and all further calls to it will cause
//   an unlicensed error to be raised.
//
//   If a license check fails during an external's handler execution then just
//   that call will fail.
//
LCError LCLicenseCheckEdition(unsigned int min_edition);
    
////////////////////////////////////////////////////////////////////////////////
    
#if defined(__OBJC__) && TARGET_OS_IPHONE

#import <UIKit/UIKit.h>
    
// Function:
//   LCInterfaceQueryView
// Parameters:
//   (out) r_view - UIView *
// Errors:
//   (none)
// Context Safety:
//    May be called in any context on the main thread.
// Semantics:
//    Returns the UIView for the currently visible stack that makes up the main
//    view of the application.
//
LCError LCInterfaceQueryView(UIView **r_view);
    
// Function:
//   LCInterfaceQueryViewController
// Parameters:
//   (out) r_controller - UIViewController *
// Errors:
//   (none)
// Context Safety:
//    May be called in any context on the main thread.
// Semantics:
//    Returns the UIViewController for the currently visible stack that makes up the main
//    view of the application.
//

LCError LCInterfaceQueryViewController(UIViewController** r_controller);

// Function:
//   LCInterfaceQueryViewScale
// Parameters:
//   (out) r_scale - double
// Errors:
//   (none)
// Context Safety:
//   May be called on any context on the main thread.
// Semantics:
//   Returns the multiplier currently applied to LiveCode co-ordinates when
//   mapping to and from UIKit co-ordinates. In particular, if running on a
//   device with a Retina display, and 'UseDeviceResolution' is true, the
//   scale with be 2.0, otherwise it will be 1.0.
//
LCError LCInterfaceQueryViewScale(double* r_scale);
	
// Function:
//   LCInterfacePresentModalViewController
// Parameters:
//   (in) controller - UIViewController *
//   (in) animated - bool
// Errors:
//   (none)
// Context Safety:
//   Must be called from handler context.
// Semantics:
//   Presents the given view controller modally, with or without animation as
//   specified by the 'animated' parameter.
//
//   This call corresponds to doing:
//     [<engine view controller> presentModalViewController: controller animated: animated]
//
LCError LCInterfacePresentModalViewController(UIViewController *controller, bool animated);

// Function:
//   LCInterfaceDismissModalViewController
// Parameters:
//   (in) controller - UIViewController *
//   (in) animated - bool
// Errors:
//   (none)
// Context Safety:
//    Must be called from handler context.
// Semantics:
//   Dismisses the given view controller that was previously presented using
//   LCInterfacePresentModalViewController. The dismissal is performed with or
//   without animation, depending on the value of the 'animated' parameter.
//
//   This call corresponds to doing:
//     [<engine view controller> dismissModalViewController: controller animated: animated]
//
LCError LCInterfaceDismissModalViewController(UIViewController *controller, bool animated);

#endif

////////////////////////////////////////////////////////////////////////////////
	
#if defined(__cplusplus)
}
#endif

#endif
