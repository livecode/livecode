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

#include "osxprefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

//#include "execpt.h"
#include "mcerror.h"
#include "ans.h"
#include "stack.h"
#include "stacklst.h"
#include "dispatch.h"
#include "globals.h"
#include "util.h"
#include "scriptenvironment.h"

#include "osxdc.h"

#include <mach-o/dyld.h>

typedef enum {
	kJSFlagNone = 0,
	kJSFlagDebug = 1 << 0,
	kJSFlagConvertAssociativeArray = 1 << 1 /* associative arrays will be converted to dictionaries */
} JSFlags;

typedef struct OpaqueJSTypeRef* JSTypeRef;
typedef JSTypeRef JSObjectRef;
typedef JSTypeRef JSRunRef;
typedef UInt32 JSTypeID;

typedef void (*JSObjectDisposeProcPtr)(void* data);
typedef CFArrayRef (*JSObjectCopyPropertyNamesProcPtr)(void* data);
typedef JSObjectRef (*JSObjectCopyPropertyProcPtr)(void* data, CFStringRef propertyName);
typedef void (*JSObjectSetPropertyProcPtr)(void* data, CFStringRef propertyName, JSObjectRef jsValue);
typedef JSObjectRef (*JSObjectCallFunctionProcPtr)(void* data, JSObjectRef thisObj, CFArrayRef args);
typedef CFTypeRef (*JSObjectCopyCFValueProcPtr)(void* data);
typedef UInt8 (*JSObjectEqualProcPtr)(void* data1, void* data2);

struct JSObjectCallBacks {
	JSObjectDisposeProcPtr dispose;
	JSObjectEqualProcPtr equal;
	JSObjectCopyCFValueProcPtr copyCFValue;
	JSObjectCopyPropertyProcPtr copyProperty;
	JSObjectSetPropertyProcPtr setProperty;
	JSObjectCallFunctionProcPtr callFunction;
	JSObjectCopyPropertyNamesProcPtr copyPropertyNames;
};
typedef struct JSObjectCallBacks JSObjectCallBacks, *JSObjectCallBacksPtr;

static void JSRelease(JSTypeRef p_ref);

static void JSLockInterpreter(void);
static void JSUnlockInterpreter(void);

static JSObjectRef JSObjectCallFunction(JSObjectRef p_ref, JSObjectRef p_this, CFArrayRef p_args);
static CFMutableArrayRef JSCreateJSArrayFromCFArray(CFArrayRef p_array);
static JSRunRef JSRunCreate(CFStringRef p_source, JSFlags p_flags);
static JSObjectRef JSRunCopyGlobalObject(JSRunRef p_ref);
static JSObjectRef JSRunEvaluate(JSRunRef p_ref);
static bool JSRunCheckSyntax(JSRunRef p_ref);
static JSObjectRef JSObjectCreate(void *p_data, JSObjectCallBacksPtr p_callbacks);
static void JSObjectSetProperty(JSObjectRef p_ref, CFStringRef p_property, JSObjectRef p_value);
static JSObjectRef JSObjectCreateWithCFType(CFTypeRef p_ref);
static CFTypeRef JSObjectCopyCFValue(JSObjectRef p_ref);
static JSObjectRef JSObjectCopyProperty(JSObjectRef p_ref, CFStringRef p_property);

///////////////////////////////////////////////////////////////////////////////

class MCMacOSXOldJavaScriptEnvironment: public MCScriptEnvironment
{
public:
	MCMacOSXOldJavaScriptEnvironment(void);
	~MCMacOSXOldJavaScriptEnvironment(void);
	
	void Retain(void);
	void Release(void);
	
	bool Define(const char *p_function, MCScriptEnvironmentCallback p_callback);
	
	void Run(MCStringRef p_script, MCStringRef& r_out);
	
	char *Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count);
	
private:
	struct Function
	{
		char *name;
		MCScriptEnvironmentCallback callback;
	};
	
	unsigned int m_references;
	
	JSRunRef m_runtime;
	
	Function *m_functions;
	uint4 m_function_count;
};

///////////////////////////////////////////////////////////////////////////////

MCScriptEnvironment *MCMacOSXCreateOldJavaScriptEnvironment(void)
{
	MCScriptEnvironment *t_environment;
	t_environment = new MCMacOSXOldJavaScriptEnvironment();
	return t_environment;
}

///////////////////////////////////////////////////////////////////////////////

extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release);

static JSObjectRef InvokeCallback(void *p_data, JSObjectRef p_this, CFArrayRef p_args)
{
	MCScriptEnvironmentCallback t_callback;
	t_callback = (MCScriptEnvironmentCallback)p_data;
	
	bool t_success;
	t_success = true;
	
	uint4 t_argument_count;
	t_argument_count = CFArrayGetCount(p_args);
	
	char **t_arguments;
	t_arguments = NULL;
	if (t_success)
	{
		t_arguments = new char *[t_argument_count];
		if (t_arguments == NULL)
			t_success = false;
	}
	
	if (t_success)
		for(uint4 i = 0; i < t_argument_count; ++i)
		{
			CFStringRef t_cf_argument;
			t_cf_argument = (CFStringRef)JSObjectCopyCFValue((JSObjectRef)CFArrayGetValueAtIndex(p_args, i));
			if (t_cf_argument != NULL && CFGetTypeID(t_cf_argument) == CFStringGetTypeID())
				t_arguments[i] = osx_cfstring_to_cstring(t_cf_argument, true);
			else
				t_arguments[i] = NULL;
			
			if (t_arguments[i] == NULL)
				t_success = false;
		}
	
	JSObjectRef t_result;
	t_result = NULL;
	if (t_success)
	{
		char *t_str_result;
		t_str_result = t_callback(t_arguments, t_argument_count);
		if (t_str_result != NULL)
		{
			CFStringRef t_cf_string;
			t_cf_string = CFStringCreateWithCString(kCFAllocatorDefault, t_str_result, kCFStringEncodingMacRoman);
			if (t_cf_string != NULL)
			{
				t_result = JSObjectCreateWithCFType(t_cf_string);
				CFRelease(t_cf_string);
			}
			
			delete t_str_result;
		}
	}
	
	if (t_arguments != NULL)
	{
		for(uint4 i = 0; i < t_argument_count; ++i)
			delete t_arguments[i];
		delete t_arguments;
	}
	
	return t_result;
}

MCMacOSXOldJavaScriptEnvironment::MCMacOSXOldJavaScriptEnvironment(void)
{
	m_references = 1;
	m_runtime = NULL;
	m_functions = NULL;
	m_function_count = 0;
}

MCMacOSXOldJavaScriptEnvironment::~MCMacOSXOldJavaScriptEnvironment(void)
{
	for(uint4 i = 0; i < m_function_count; ++i)
		delete m_functions[i] . name;
	
	delete m_functions;
	
	if (m_runtime != NULL)
		JSRelease(m_runtime);
}

void MCMacOSXOldJavaScriptEnvironment::Retain(void)
{
	m_references += 1;
}

void MCMacOSXOldJavaScriptEnvironment::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

bool MCMacOSXOldJavaScriptEnvironment::Define(const char *p_name, MCScriptEnvironmentCallback p_callback)
{
	Function *t_new_functions;
	t_new_functions = (Function *)realloc(m_functions, sizeof(Function) * (m_function_count + 1));
	if (t_new_functions == NULL)
		return false;
	
	t_new_functions[m_function_count] . name = strdup(p_name);
	t_new_functions[m_function_count] . callback = p_callback;
	
	m_functions = t_new_functions;
	m_function_count += 1;
	
	return true;
}

void MCMacOSXOldJavaScriptEnvironment::Run(MCStringRef p_script, MCStringRef& r_out)
{
	bool t_success;
	t_success = true;
	
	CFStringRef t_cf_script;
	t_cf_script = NULL;
    char *temp;
	if (t_success && MCStringConvertToCString(p_script, temp))
	{
		t_cf_script = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, temp, kCFStringEncodingMacRoman, kCFAllocatorNull);
		if (t_cf_script == NULL)
			t_success = false;
	}
    delete temp;
	
	JSRunRef t_runtime;
	t_runtime = NULL;
	if (t_success)
	{
		JSLockInterpreter();
		t_runtime = JSRunCreate(t_cf_script, (JSFlags)0);
		JSUnlockInterpreter();
		
		if (t_runtime == NULL)
			t_success = false;
	}
	
	if (t_success)
		t_success = JSRunCheckSyntax(t_runtime);
	
	JSObjectRef t_global;
	t_global = NULL;
	if (t_success)
	{
		t_global = JSRunCopyGlobalObject(t_runtime);
		if (t_global == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		JSObjectCallBacks t_callbacks = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		t_callbacks . callFunction = InvokeCallback;
		
		JSLockInterpreter();
		
		for(uint4 i = 0; i < m_function_count && t_success; ++i)
		{
			CFStringRef t_function_name;
			t_function_name = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, m_functions[i] . name, kCFStringEncodingMacRoman, kCFAllocatorNull);
			if (t_function_name == NULL)
				t_success = false;
			
			JSObjectRef t_function;
			t_function = NULL;
			if (t_success)
			{
				t_function = JSObjectCreate((void *)m_functions[i] . callback, &t_callbacks);
				if (t_function == NULL)
					t_success = false;
			}
			
			if (t_success)
				JSObjectSetProperty(t_global, t_function_name, t_function);
			
			if (t_function != NULL)
				JSRelease(t_function);
			
			if (t_function_name != NULL)
				CFRelease(t_function_name);
		}
		
		JSUnlockInterpreter();
	}
	
	if (t_global != NULL)
		JSRelease(t_global);
	
	if (t_cf_script != NULL)
		CFRelease(t_cf_script);
	
	if (t_success)
	{
		JSObjectRef t_eval;
		t_eval = JSRunEvaluate(t_runtime);
		if (t_eval != NULL)
			JSRelease(t_eval);
	}
	
	if (t_success)
	{
		m_runtime = t_runtime;
		r_out = MCValueRetain(kMCEmptyString);
	}
	else
	{
		JSRelease(m_runtime);
		r_out = nil;
	}
	
	return;
}

char *MCMacOSXOldJavaScriptEnvironment::Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count)
{
	if (m_runtime == NULL)
		return NULL;
	
	bool t_success;
	t_success = true;
	
	CFStringRef t_cf_method;
	t_cf_method = NULL;
	if (t_success)
	{
		t_cf_method = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, p_method, kCFStringEncodingMacRoman, kCFAllocatorNull);
		if (t_cf_method == NULL)
			t_success = false;
	}
	
	JSObjectRef t_js_global;
	t_js_global = NULL;
	if (t_success)
	{
		t_js_global = JSRunCopyGlobalObject(m_runtime);
		if (t_js_global == NULL)
			t_success = false;
	}
	
	JSObjectRef t_js_method;
	t_js_method = NULL;
	if (t_success)
	{
		t_js_method = JSObjectCopyProperty(t_js_global, t_cf_method);
		if (t_js_method == NULL)
			t_success = false;
	}
	
	CFMutableArrayRef t_cf_arguments;
	t_cf_arguments = NULL;
	if (t_success)
	{
		t_cf_arguments = CFArrayCreateMutable(kCFAllocatorDefault, p_argument_count, &kCFTypeArrayCallBacks);
		if (t_cf_arguments == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		for(uint4 i = 0; i < p_argument_count && t_success; ++i)
		{
			CFStringRef t_cf_argument;
			t_cf_argument = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, p_arguments[i], kCFStringEncodingMacRoman, kCFAllocatorNull);
			if (t_cf_argument == NULL)
				t_success = false;
			
			if (t_success)
				CFArrayAppendValue(t_cf_arguments, t_cf_argument);
			
			if (t_cf_argument != NULL)
				CFRelease(t_cf_argument);
		}
	}
	
	JSObjectRef t_js_result;
	t_js_result = NULL;
	if (t_success)
	{
		JSLockInterpreter();
		
		CFMutableArrayRef t_js_arguments;
		t_js_arguments = JSCreateJSArrayFromCFArray(t_cf_arguments);
		
		if (t_js_arguments != NULL)
		{
			t_js_result = JSObjectCallFunction(t_js_method, t_js_global, t_js_arguments);
			CFRelease(t_js_arguments);
		}
		
		JSUnlockInterpreter();
		
		if (t_js_result == NULL)
			t_success = false;
	}
	
	
	char *t_result;
	t_result = NULL;
	if (t_success)
	{
		CFTypeRef t_cf_result;
		t_cf_result = NULL;
		if (t_js_result != NULL)
			t_cf_result = JSObjectCopyCFValue(t_js_result);
		
		if (t_cf_result != NULL)
		{
			if (CFGetTypeID(t_cf_result) == CFStringGetTypeID())
			{
				CFIndex t_max_length;
				t_max_length = CFStringGetMaximumSizeForEncoding(CFStringGetLength((CFStringRef)t_cf_result), kCFStringEncodingMacRoman);
				
				t_result = (char *)malloc(t_max_length + 1);
				if (t_result != NULL)
					CFStringGetCString((CFStringRef)t_cf_result, t_result, t_max_length + 1, kCFStringEncodingMacRoman);
			}
		}
		
		if (t_result == NULL)
			t_success = false;
	}
	
	if (t_js_result != NULL)
		JSRelease(t_js_result);
	
	if (t_cf_arguments != NULL)
		CFRelease(t_cf_arguments);
	
	if (t_js_method != NULL)
		JSRelease(t_js_method);
	
	if (t_js_global != NULL)
		JSRelease(t_js_global);
	
	if (t_cf_method != NULL)
		CFRelease(t_cf_method);
	
	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

struct _JavaScriptCallBacks {
	void (*JSRelease_proc)(JSTypeRef);
	void (*JSLockInterpreter_proc)(void);
	void (*JSUnlockInterpreter_proc)(void);
	JSObjectRef (*JSObjectCallFunction_proc)(JSObjectRef, JSObjectRef, CFArrayRef);
	CFMutableArrayRef (*JSCreateJSArrayFromCFArray_proc)(CFArrayRef);
	JSRunRef (*JSRunCreate_proc)(CFStringRef, JSFlags);
	JSObjectRef (*JSRunCopyGlobalObject_proc)(JSRunRef);
	JSObjectRef (*JSRunEvaluate_proc)(JSRunRef);
	bool (*JSRunCheckSyntax_proc)(JSRunRef);
	JSObjectRef (*JSObjectCreate_proc)(void*, JSObjectCallBacksPtr);
	void (*JSObjectSetProperty_proc)(JSObjectRef, CFStringRef, JSObjectRef);
	JSObjectRef (*JSObjectCreateWithCFType_proc)(CFTypeRef);
	CFTypeRef (*JSObjectCopyCFValue_proc)(JSObjectRef);
	JSObjectRef (*JSObjectCopyProperty_proc)(JSObjectRef, CFStringRef);
};

static const char kJavaScriptLibraryPath[] = "/System/Library/PrivateFrameworks/JavaScriptGlue.framework/Versions/A/JavaScriptGlue";

static const void* JavaScriptLibrary = NULL;
static struct _JavaScriptCallBacks* JavaScriptCallBacks = NULL;

static const void* returns_ref(void) { return NULL; }
static bool returns_bool(void) { return 0; }
static void returns(void) { return; }


#define GET_DYNAMIC_SYMBOL(sym, rettype, arglist, alt) \
if (!JavaScriptLibrary) { \
JavaScriptLibrary = ((void*)NSAddImage(kJavaScriptLibraryPath, NSADDIMAGE_OPTION_WITH_SEARCHING | NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME)); \
if (!JavaScriptCallBacks) { \
JavaScriptCallBacks = (struct _JavaScriptCallBacks*)calloc(1, sizeof(JavaScriptCallBacks[0])); \
}	\
} \
if (!JavaScriptCallBacks->sym##_proc) { \
JavaScriptCallBacks->sym##_proc = (rettype(*)arglist)NSAddressOfSymbol(NSLookupSymbolInImage((const mach_header *)JavaScriptLibrary, "_"#sym, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND)); \
if (!JavaScriptCallBacks->sym##_proc) JavaScriptCallBacks->sym##_proc = (rettype(*)arglist)alt; \
} \


void
JSRelease(JSTypeRef ref) {
	
    GET_DYNAMIC_SYMBOL(JSRelease, void, (JSTypeRef), returns);
    
    return JavaScriptCallBacks->JSRelease_proc(ref);
}

void
JSLockInterpreter(void) {
	
    GET_DYNAMIC_SYMBOL(JSLockInterpreter, void, (void), returns);
    
    JavaScriptCallBacks->JSLockInterpreter_proc();
}

void
JSUnlockInterpreter(void) {
	
    GET_DYNAMIC_SYMBOL(JSUnlockInterpreter, void, (void), returns);
    
    JavaScriptCallBacks->JSUnlockInterpreter_proc();
}

JSObjectRef
JSObjectCallFunction(JSObjectRef ref, JSObjectRef thisObj, CFArrayRef args) {
	
    GET_DYNAMIC_SYMBOL(JSObjectCallFunction, JSObjectRef, (JSObjectRef, JSObjectRef, CFArrayRef), returns_ref);
    
    return JavaScriptCallBacks->JSObjectCallFunction_proc(ref, thisObj, args);
}

CFMutableArrayRef 
JSCreateJSArrayFromCFArray(CFArrayRef array) {
    GET_DYNAMIC_SYMBOL(JSCreateJSArrayFromCFArray, CFMutableArrayRef, (CFArrayRef), returns_ref);
    
    return JavaScriptCallBacks->JSCreateJSArrayFromCFArray_proc(array);
}



JSRunRef
JSRunCreate(CFStringRef jsSource, JSFlags inFlags) {
	
    GET_DYNAMIC_SYMBOL(JSRunCreate, JSRunRef, (CFStringRef, JSFlags), returns_ref);
    
    return JavaScriptCallBacks->JSRunCreate_proc(jsSource, inFlags);
}


JSObjectRef
JSRunCopyGlobalObject(JSRunRef ref) {
	
    GET_DYNAMIC_SYMBOL(JSRunCopyGlobalObject, JSObjectRef, (JSRunRef), returns_ref);
    
    return JavaScriptCallBacks->JSRunCopyGlobalObject_proc(ref);
}


JSObjectRef
JSRunEvaluate(JSRunRef ref) {
	
    GET_DYNAMIC_SYMBOL(JSRunEvaluate, JSObjectRef, (JSRunRef), returns_ref);
    
    return JavaScriptCallBacks->JSRunEvaluate_proc(ref);
}


bool
JSRunCheckSyntax(JSRunRef ref) {
	
    GET_DYNAMIC_SYMBOL(JSRunCheckSyntax, bool, (JSRunRef), returns_bool);
    
    return JavaScriptCallBacks->JSRunCheckSyntax_proc(ref);
}


JSObjectRef
JSObjectCreate(void* data, JSObjectCallBacksPtr callBacks) {
	
    GET_DYNAMIC_SYMBOL(JSObjectCreate, JSObjectRef, (void*, JSObjectCallBacksPtr), returns_ref);
    
    return JavaScriptCallBacks->JSObjectCreate_proc(data, callBacks);
}


void
JSObjectSetProperty(JSObjectRef ref, CFStringRef propertyName, JSObjectRef value) {
	
    GET_DYNAMIC_SYMBOL(JSObjectSetProperty, void, (JSObjectRef, CFStringRef, JSObjectRef), returns);
    
    return JavaScriptCallBacks->JSObjectSetProperty_proc(ref, propertyName, value);
}


JSObjectRef
JSObjectCreateWithCFType(CFTypeRef inRef) {
	
    GET_DYNAMIC_SYMBOL(JSObjectCreateWithCFType, JSObjectRef, (CFTypeRef), returns_ref);
    
    return JavaScriptCallBacks->JSObjectCreateWithCFType_proc(inRef);
}


CFTypeRef
JSObjectCopyCFValue(JSObjectRef ref) {
	
    GET_DYNAMIC_SYMBOL(JSObjectCopyCFValue, CFTypeRef, (JSObjectRef), returns_ref);
    
    return JavaScriptCallBacks->JSObjectCopyCFValue_proc(ref);
}


JSObjectRef
JSObjectCopyProperty(JSObjectRef ref, CFStringRef propertyName) {
	
    GET_DYNAMIC_SYMBOL(JSObjectCopyProperty, JSObjectRef, (JSObjectRef, CFStringRef), returns_ref);
	
    return JavaScriptCallBacks->JSObjectCopyProperty_proc(ref, propertyName);
}
