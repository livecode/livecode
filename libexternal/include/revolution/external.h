#ifndef __REVOLUTION_EXTERNAL__
#define __REVOLUTION_EXTERNAL__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

///////////////////////////////////////////////////////////////////////////////
//
// External API declarations
//

#define EXTERNAL_SUCCESS 0
#define EXTERNAL_FAILURE 1

#ifndef Bool
#define Bool int
#define True 1
#define False 0
#endif

typedef void (*ExternalHandler)(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_error);

typedef struct _ExternalDeclaration
{
	const char *name;
	const char *type;
	unsigned long __not_used_1;
	ExternalHandler handler;
	void *__not_used_2;
} ExternalDeclaration;

typedef struct _ExternalString
{
	const char *buffer;
	int length;
} ExternalString;

typedef struct _ExternalArray
{
  int nelements;
  ExternalString *strings;
  char **keys;
} ExternalArray;

typedef struct _MCRectangle32
{
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} MCRectangle32;

// Function:
//   SendCardMessage
// Parameters:
//   p_message: const char *
//   r_success: int *
// Semantics:
//   Send the message <p_message> to this card of this stack. The message may
//   contain parameters, but these should be pre-evaluated.
//
//   If the call succeeds *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void SendCardMessage(const char *p_message, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SendCardMessage, but with UTF8-encoded parameters
extern void SendCardMessageUTF8(const char *p_message, int *r_success);

// Function:
//   EvalExpr
// Parameters:
//   p_expression: const char *
//   r_success: int *
// Returns:
//   char *
// Semantics:
//   Evaluate the Revolution expression <p_expression> in the context of the
//   handler that invoked the current external command or function.
//
//   The result of the evaluation will be returned as a C-string that the
//   caller then takes ownership of. i.e. You need to 'free' the result of
//   EvalExpr when you are finished with it.
//
//   If the call succeeds *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern char *EvalExpr(const char *p_expression, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to EvalExpr, but with UTF8-encoded parameters
extern char *EvalExprUTF8(const char *p_expression, int *r_success);

// Function:
//   GetGlobal
// Parameters:
//   p_name: const char *
//   r_success: int *
// Returns:
//   char *
// Semantics:
//   Return the contents of the global variable called <p_name>.
//
//   The contents is returned as a C-string - meaning you cannot use it to
//   fetch binary data. (The data will be truncated at the first NUL
//   character).
//
//   The caller takes ownership of the returned C-string. i.e. You need to
//   'free' the result of GetGlobal when you are finished with it.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern char *GetGlobal(const char *p_name, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to GetGlobal, but with UTF8-encoded parameters
extern char *GetGlobalUTF8(const char *p_name, int *r_success);

// Function:
//   SetGlobal
// Parameters:
//   p_name: const char *
//   p_value: const char *
//   r_success: int *
// Semantics:
//   Set the value of a global variable called <p_name> to <p_value>.
//
//   The value must be a (NUL-terminated) C-string - meaning it is unsuitable
//   to use this call to set binary data.
//
//   If the variable does not exist it will be created.
//
//   Revolution copies the contents of <p_value> so ownership of the memory
//   pointed to by it does not change.
//
//   If the call succeeds *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//   
extern void SetGlobal(const char *p_name, const char *p_value, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SetGlobal, but with UTF8-encoded parameters
extern void SetGlobalUTF8(const char *p_name, const char *p_value, int *r_success);
    
// Function:
//   GetVariable
// Parameters:
//   p_name: const char *
//   r_success: int *
// Returns:
//   char *
// Semantics:
//   Get the value of a variable with name <p_name> that is accessible within
//   the scope of the handler that invoked the external command or function.
//
//   The contents is returned as a C-string - meaning you cannot use it to
//   fetch binary data. (The data will be truncated at the first NUL
//   character).
//
//   The caller takes ownership of the returned C-string. i.e. You need to
//   'free' the result of GetGlobal when you are finished with it.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern char *GetVariable(const char *p_name, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to GetVariable, but with UTF8-encoded parameters
extern char *GetVariableUTF8(const char *p_name, int *r_success);

// Function:
//   SetVariable
// Parameters:
//   p_name: const char *
//   p_value: const char *
//   r_success: int *
// Semantics:
//   Set the value of a variable with name <p_name> that is accessible within
//   the scope of the handler that invoked the external command or function to
//   <p_value>.
//
//   The value must be a (NUL-terminated) C-string - meaning it is unsuitable
//   to use this call to set binary data.
//
//   Revolution copies the contents of <p_value> so ownership of the memory
//   pointed to by it does not change.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void SetVariable(const char *p_name, const char *p_value, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SetVariable, but with UTF8-encoded parameters
extern void SetVariableUTF8(const char *p_name, const char *p_value, int *r_success);

// Function:
//   GetVariableEx
// Parameters:
//   p_name: const char *
//   p_key: const char *
//   r_value: ExternalString *
//   r_success: int *
// Semantics:
//   Get the value of a variable with name <p_name> that is accessible within
//   the scope of the handler that invoked the external command or function.
//
//   If <p_key> is the empty string ("") then the value fetched will be that
//   of the variable as a non-array (i.e. if the variable is an array the
//   resulting value will be empty).
//
//   If <p_key> is not the empty string then the value fetched will be the
//   value of the element of the variable with key <p_key>, or empty if
//   there is no such key or the variable is not an array.
//
//   The value is returned in <r_value> and Revolution retains ownership of
//   the memory. The pointer r_value -> buffer is only guaranteed to be valid
//   until control returns to Revolution or the variable, or variable's element
//   is next modified.
//
//   This call is safe to use with binary and text data as an 'ExternalString'
//   need not be NUL-terminated - it carries the length of the data alongside
//   the pointer to the data.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void GetVariableEx(const char *p_name, const char *p_key, ExternalString *r_value, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to GetVariableEx, but with UTF8-encoded parameters
// p_is_text allows to choose how r_value is encoded.
//      with p_is_text set to true, the value is returned as utf-8 encoded text
//      with p_is_text set to false, the value is returned as binary data (the same as native encoded text)
extern void GetVariableExUTF8(const char *p_name, const char *p_key, const ExternalString *r_value, Bool p_is_text, int *r_success);

// Function:
//   SetVariableEx
// Parameters:
//   p_name: const char *
//   p_key: const char *
//   p_value: ExternalString *
//   r_success: int *
// Semantics:
//   Set the value of a variable with name <p_name> that is accessible within
//   the scope of the handler that invoked the external command or function to
//   <p_value>.
//
//   If <p_key> is the empty string (""), then the variable will be set as
//   a non-array. In particular, if it was previously an array its elements
//   will be deleted.
//
//   If <p_key> is not the empty string (""), then the element with key <p_key>
//   will be set to <p_value>. In particular, if the variable was not previously
//   an array it's value will be cleared and it will become one. Similarly, if
//   the element with the given key does not exist, it will be created.
//
//   Revolution copies the data referenced in <p_value> and so ownership of the
//   memory does not change.
//
//   This call is safe to use with binary and text data as an 'ExternalString'
//   need not be NUL-terminated - it carries the length of the data alongside
//   the pointer to the data.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void SetVariableEx(const char *p_name, const char *p_key, const ExternalString *p_value, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SetVariableEx, but with UTF8-encoded parameters
// p_is_text allows to choose how p_value is encoded.
//      with p_is_text set to true, the value is returned as utf-8 encoded text
//      with p_is_text set to false, the value is returned as binary data (the same as native encoded text)
extern void SetVariableExUTF8(const char *p_name, const char *p_key, const ExternalString *p_value, Bool p_is_text, int *r_success);

// Function:
//   GetArray
// Parameters:
//   p_name: const char *
//   r_element_count: int *
//   r_values: ExternalString *
//   r_keys: char **
//   r_success: int *
// Semantics:
//   Fetch contents of an array variable with name <p_name> that is accessible
//   within the scope of the handler that invoked the external command or
//   function.
//
//   On entry if *r_element_count is 0, *r_element_count will be updated with
//   the number of elements of the array and the call will return.
//
//   If *r_element_count is not 0, then r_keys must point to an array of char *
//   pointers of length *r_element_count. Additionally, r_values may point to an
//   array of ExternalString's of length *r_element_count.
//
//   On exit, if *r_element_count is not 0, then:
//     - *r_element_count will be updated with the number of entries fetched
//       (min(number of elements of array, *r_element_count).
//     - each entry in the r_keys array will point to a C-string of the key of
//       each element fetched
//     - each entry in the r_values array (if non-NULL) will point to the value
//       of each element fetched
//
//   The ownership of all pointers returned (both in r_keys and r_values) remains
//   with Revolution. Therefore these pointers are only guaranteed to be valid
//   until control returns to Revolution or the variable is next modified.
//
//   This call is safe to use with binary and text data as an 'ExternalString'
//   need not be NUL-terminated - it carries the length of the data alongside
//   the pointer to the data.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void GetArray(const char *p_name, int *r_element_count, ExternalString *r_values, char **r_keys, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to GetArray, but with UTF8-encoded parameters
// p_is_text allows to choose how r_values are encoded.
//      with p_is_text set to true, the values are returned as utf-8 encoded text
//      with p_is_text set to false, the values are returned as binary data (the same as native encoded text)
extern void GetArrayUTF8(const char *p_name, int *r_element_count, ExternalString *r_values, char **r_keys, Bool p_is_text, int *r_success);

// Function:
//   SetArray
// Parameters:
//   p_name: const char *
//   p_element_count: int
//   p_values: ExternalString *
//   p_keys: char **
//   r_success: int *
// Semantics:
//   Set the contents of an array variable with name <p_name> that is accessible
//   within the scope of the handler that invoked the external command or
//   function to elements with keys <p_keys> and values <p_values>.
//
//   On entry, <p_element_count> must contain the number of elements to set and
//   <p_values> must point to an array of ExternalString's of length
//   <p_element_count>. Additionally, <p_keys> may point to an array of char *'s
//   of length <p_element_count>.
//
//   This call clear the contents of the named variable and then set it up as
//   an array with the specified values.
//
//   If <p_keys> is NULL, the variable will be made into an integer-keyed array,
//   entry <n> mapping to p_values[n].
//   If <p_keys> is non-NULL, the variable will be made into a string-keyed array,
//   entry p_key[n] mapping to p_values[n].
//
//   Revolution copies all data referenced and so the ownership of any memory
//   does not change.
//   
//   This call is safe to use with binary and text data as an 'ExternalString'
//   need not be NUL-terminated - it carries the length of the data alongside
//   the pointer to the data.
//
//   If the variable exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void SetArray(const char *p_name, int p_element_count, ExternalString *p_values, char **p_keys, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SetArray, but with UTF8-encoded parameters
// p_is_text allows to choose how p_values are encoded.
//      with p_is_text set to true, the values are returned as utf-8 encoded text
//      with p_is_text set to false, the values are returned as binary data (the same as native encoded text)
extern void SetArrayUTF8(const char *p_name, int p_element_count, ExternalString *p_values, char **p_keys, Bool p_is_text, int *r_success);

// Function:
//   GetFieldBy*
// Parameters:
//   p_group: const char *
//   p_name: const char *
//   p_index: int
//   p_id: unsigned long
//   r_success: int *
// Result:
//   char *
// Semantics:
//   These functions fetch the contents of the specified field. The field
//   searched for depends on the form of the function:
//
//   For Name:
//     field <p_name> of this card of this stack
//   For Num:
//     field <p_index> of this card of this stack
//   For Id:
//     field id <p_id> of this card of this stack
//
//   If p_group is "true" then it has the same effect as searching for
//   'card field'.
//   If p_group is "false" then it has the same effect as searching for
//   'background field"'.
//   If p_group is NULL then no modifier is used.
//
//   The contents is returned as a C-string - meaning you cannot use it to
//   fetch binary data. (The data will be truncated at the first NUL
//   character).
//
//   The caller takes ownership of the returned C-string. i.e. You need to
//   'free' the result of GetGlobal when you are finished with it.
//
//   If the field exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern char *GetFieldByName(const char *p_group, const char *p_name, int *r_success);
extern char *GetFieldByNum(const char *p_group, int p_index, int *r_success);
extern char *GetFieldById(const char *p_group, unsigned long p_id, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to GetFieldByName/Num, but with UTF8-encoded char* parameters
extern char *GetFieldByNameUTF8(const char *p_group, const char *p_name, int *r_success);
extern char *GetFieldByNumUTF8(const char *p_group, int p_index, int *r_success);
extern char *GetFieldByIdUTF8(const char *p_group, unsigned long p_id, int *r_success);

// Function:
//   SetFieldBy*
// Parameters:
//   p_group: const char *
//   p_name: const char *
//   p_index: int
//   p_id: unsigned long
//   p_value: const char *
//   r_success: int *
// Semantics:
//   These functions set the contents of the specified field. The field
//   searched for depends on the form of the function:
//
//   For Name:
//     field <p_name> of this card of this stack
//   For Num:
//     field <p_index> of this card of this stack
//   For Id:
//     field id <p_id> of this card of this stack
//
//   If p_group is "true" then it has the same effect as searching for
//   'card field'.
//   If p_group is "false" then it has the same effect as searching for
//   'background field"'.
//   If p_group is NULL then no modifier is used.
//
//   The value must be a (NUL-terminated) C-string - meaning it is unsuitable
//   to use this call to set binary data.
//
//   Revolution copies the contents of <p_value> so ownership of the memory
//   pointed to by it does not change.
//
//   If the field exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void SetFieldByName(const char *p_group, const char *p_name, const char *p_value, int *r_success);
extern void SetFieldByNum(const char *p_group, int p_index, const char *p_value, int *r_success);
extern void SetFieldById(const char *p_group, unsigned long p_id, const char *p_value, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to SetFieldByName/Num, but with UTF8-encoded char* parameters
extern void SetFieldByNameUTF8(const char *p_group, const char *p_name, const char *p_value, int *r_success);
extern void SetFieldByNumUTF8(const char *p_group, int p_index, const char *p_value, int *r_success);
extern void SetFieldByIdUTF8(const char *p_group, unsigned long p_id, const char *p_value, int *r_success);

// Function:
//   ShowImageBy*
// Parameters:
//   p_group: const char *
//   p_name: const char *
//   p_index: int
//   p_id: unsigned long
//   r_success: int *
// Semantics:
//   These functions cause the specified image to be redrawn. The image
//   searched for depends on the form of the function:
//
//   For Name:
//     field <p_name> of this card of this stack
//   For Num:
//     field <p_index> of this card of this stack
//   For Id:
//     field id <p_id> of this card of this stack
//
//   If p_group is "true" then it has the same effect as searching for
//   'card field'.
//   If p_group is "false" then it has the same effect as searching for
//   'background field"'.
//   If p_group is NULL then no modifier is used.
//
//   If the field exists *r_success will be EXTERNAL_SUCCESS, otherwise it
//   will be EXTERNAL_FAILURE.
//
extern void ShowImageByName(const char *p_group, const char *p_name, int *r_success);
extern void ShowImageByNum(const char *p_group, int p_index, int *r_success);
extern void ShowImageById(const char *p_group, unsigned long p_id, int *r_success);
extern void ShowImageByLongId(const char *p_long_id, int *r_success);
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Equivalent to ShowImageByName/Num, but with UTF8-encoded char* parameters
extern void ShowImageByNameUTF8(const char *p_group, const char *p_name, int *r_success);
extern void ShowImageByNumUTF8(const char *p_group, int p_index, int *r_success);
extern void ShowImageByIdUTF8(const char *p_group, unsigned long p_id, int *r_success);
extern void ShowImageByLongIdUTF8(const char *p_long_id, int *r_success);
    

// IM-2014-03-06: [[ revBrowserCEF ]] Definition of runloop action callback & action ref
typedef void (*MCRunloopActionCallback)(void *context);
typedef void *MCRunloopActionRef;

// IM-2014-03-06: [[ revBrowserCEF ]] Register the callback to run during the engine's run loop
extern void AddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef *r_action, int *r_success);
// IM-2014-03-06: [[ revBrowserCEF ]] Remove the runloop action from the run loop
extern void RemoveRunloopAction(MCRunloopActionRef p_action, int *r_success);

// IM-2014-03-06: [[ revBrowserCEF ]] Run the engine runloop
extern void RunloopWait(int *r_success);

// IM-2014-07-09: [[ Bug 12225 ]] Convert stack coords to logical window coords
extern void StackToWindowRect(uintptr_t p_win_id, MCRectangle32 *x_rect, int *r_success);
// IM-2014-07-09: [[ Bug 12225 ]] Convert logical window coords to stack coords
extern void WindowToStackRect(uintptr_t p_win_id, MCRectangle32 *x_rect, int *r_success);

// AL-2015-02-10: [[ SB Inclusions ]] Add wrappers for ExternalV0 module loading callbacks
// SN-2015-02-24: [[ Broken Win Compilation ]] LoadModule is a Win32 API function...
extern void LoadModuleByName(const char *p_module, void **r_handle, int *r_success);
extern void UnloadModule(void *p_handle, int *r_success);
extern void ResolveSymbolInModule(void *p_handle, const char *p_symbol, void **r_resolved, int *r_success);

extern char *CopyNativePathOfModule(void *p_handle, int* r_success);
    
//
extern Bool SecurityCanAccessFile(const char *p_file);
extern Bool SecurityCanAccessHost(const char *p_host);	
extern Bool SecurityCanAccessLibrary(const char *p_library);
    
// SN-2014-07-04: [[ UnicodeExternalsV0 ]] Same as SecurityCanAccess*, but with UTF8-encoded parameters
extern Bool SecurityCanAccessFileUTF8(const char *p_file);
extern Bool SecurityCanAccessHostUTF8(const char *p_host);
extern Bool SecurityCanAccessLibraryUTF8(const char *p_library);
    

// SN-2015-03-12: [[ Bug 14413 ]] Added UTF-8 <-> native string conversion
// The string returned are owned by the engine, and must not be free'd
extern const char *ConvertCStringFromNativeToUTF8(const char *p_native, int *r_success);
extern const char *ConvertCStringToNativeFromUTF8(const char *p_utf8, int *r_success);
	
// IM-2014-09-23: [[ RevBrowserCEF ]] Retrieve the Xserver connection info
extern void GetXDisplayHandle(void **r_display, int *r_success);
extern void GetXScreenHandle(void **r_screen, int *r_success);

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// External declaration macros
//

#ifdef __cplusplus

#define EXTERNAL_REFERENCE_LIBRARY \
  extern "C" void getXtable(void); \
	void __dummy_function(void) \
	{ \
	  getXtable(); \
	}
	
#else

#define EXTERNAL_REFERENCE_LIBRARY \
  void getXtable(void); \
	void __dummy_function(void) \
	{ \
	  getXtable(); \
	}
	
#endif

#ifdef __cplusplus

#define EXTERNAL_BEGIN_DECLARATIONS(m_name) \
	extern "C" { \
		EXTERNAL_REFERENCE_LIBRARY \
		const char *g_external_name = m_name; \
		ExternalDeclaration g_external_table[] = \
		{ \

#define EXTERNAL_END_DECLARATIONS \
			{ "", "", 0, 0, 0 } \
		}; \
  }

#else

#define EXTERNAL_BEGIN_DECLARATIONS(m_name) \
	EXTERNAL_REFERENCE_LIBRARY \
	const char *g_external_name = m_name; \
	ExternalDeclaration g_external_table[] = \
  { \

#define EXTERNAL_END_DECLARATIONS \
		{ "", "", 0, 0, 0 } \
	};

#endif

#if defined(__cplusplus) && defined(__EXCEPTIONS)

#include <cstring>
#include <exception>
#include <cstdlib>

template<ExternalHandler u_handler> void ExternalWrapper(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	*r_result = NULL;

	try
	{
		u_handler(p_arguments, p_argument_count, r_result, r_pass, r_err);
	}
	catch(std::exception& t_exception)
	{
		if (*r_result != NULL)
			free(*r_result);

		*r_result = strdup(t_exception . what());
		*r_err = True;
	}
	catch(...)
	{
		if (*r_result != NULL)
			free(*r_result);

		*r_result = strdup("unknown C++ exception");
		*r_err = True;
	}
}

#define EXTERNAL_DECLARE_COMMAND(m_name, m_function) \
		{ m_name, "C", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION(m_name, m_function) \
		{ m_name, "F", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_COMMAND_UTF8(m_name, m_function) \
        { m_name, "c", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION_UTF8(m_name, m_function) \
        { m_name, "f", 0, ExternalWrapper<m_function>, NULL },


#ifdef __OBJC__

#include <Foundation/Foundation.h>

inline void ExternalWrapperObjCCall(ExternalHandler p_handler, char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	NS_DURING
	p_handler(p_arguments, p_argument_count, r_result, r_pass, r_err);
	NS_HANDLER
	*r_result = strdup([[localException reason] cStringUsingEncoding:NSMacOSRomanStringEncoding]);
	*r_err = True;
	NS_ENDHANDLER
}

template<ExternalHandler u_handler> void ExternalWrapperObjC(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	*r_result = NULL;

	try
	{
		NSAutoreleasePool *t_pool;
		t_pool = [[NSAutoreleasePool alloc] init];
		if (t_pool != NULL)
		{
			ExternalWrapperObjCCall(u_handler, p_arguments, p_argument_count, r_result, r_pass, r_err);
			[t_pool release];
		}
	}
	catch(std::exception& t_exception)
	{
		if (*r_result != NULL)
			free(*r_result);

		*r_result = strdup(t_exception . what());
		*r_err = True;
	}
	catch(...)
	{
		if (*r_result != NULL)
			free(*r_result);

		*r_result = strdup("unknown C++ exception");
		*r_err = True;
	}
}

#define EXTERNAL_DECLARE_COMMAND_OBJC(m_name, m_function) \
		{ m_name, "C", 0, ExternalWrapperObjC<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION_OBJC(m_name, m_function) \
		{ m_name, "F", 0, ExternalWrapperObjC<m_function>, NULL },

#define EXTERNAL_DECLARE_COMMAND_OBJC_UTF8(m_name, m_function) \
        { m_name, "c", 0, ExternalWrapperObjC<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8(m_name, m_function) \
        { m_name, "f", 0, ExternalWrapperObjC<m_function>, NULL },

#else


#define EXTERNAL_DECLARE_COMMAND_OBJC(m_name, m_function) \
		{ m_name, "C", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION_OBJC(m_name, m_function) \
		{ m_name, "F", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_COMMAND_OBJC_UTF8(m_name, m_function) \
        { m_name, "c", 0, ExternalWrapper<m_function>, NULL },

#define EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8(m_name, m_function) \
        { m_name, "f", 0, ExternalWrapper<m_function>, NULL },

#endif

#else

#define EXTERNAL_DECLARE_COMMAND(m_name, m_function) \
		{ m_name, "C", 0, m_function, 0 },

#define EXTERNAL_DECLARE_FUNCTION(m_name, m_function) \
		{ m_name, "F", 0, m_function, 0 },

#define EXTERNAL_DECLARE_COMMAND_UTF8(m_name, m_function) \
        { m_name, "c", 0, m_function, 0 },

#define EXTERNAL_DECLARE_FUNCTION_UTF8(m_name, m_function) \
        { m_name, "f", 0, m_function, 0 },

#endif

#endif
