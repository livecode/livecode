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

#include <Cocoa/Cocoa.h>

#include "globdefs.h"
#include "uidc.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include <mach-o/dyld.h>

///////////////////////////////////////////////////////////////////////////////

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSContext* JSGlobalContextRef;
typedef struct OpaqueJSString* JSStringRef;
typedef struct OpaqueJSClass* JSClassRef;
typedef struct OpaqueJSPropertyNameArray* JSPropertyNameArrayRef;
typedef struct OpaqueJSPropertyNameAccumulator* JSPropertyNameAccumulatorRef;
typedef const struct OpaqueJSValue* JSValueRef;
typedef struct OpaqueJSValue* JSObjectRef;

typedef enum {
    kJSTypeUndefined,
    kJSTypeNull,
    kJSTypeBoolean,
    kJSTypeNumber,
    kJSTypeString,
    kJSTypeObject
} JSType;

enum { 
    kJSPropertyAttributeNone         = 0,
    kJSPropertyAttributeReadOnly     = 1 << 1,
    kJSPropertyAttributeDontEnum     = 1 << 2,
    kJSPropertyAttributeDontDelete   = 1 << 3
};
typedef unsigned JSPropertyAttributes;

enum { 
    kJSClassAttributeNone = 0,
    kJSClassAttributeNoAutomaticPrototype = 1 << 1
};
typedef unsigned JSClassAttributes;

typedef void
(*JSObjectInitializeCallback) (JSContextRef ctx, JSObjectRef object);
typedef void            
(*JSObjectFinalizeCallback) (JSObjectRef object);
typedef bool
(*JSObjectHasPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName);
typedef JSValueRef
(*JSObjectGetPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef bool
(*JSObjectSetPropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception);
typedef bool
(*JSObjectDeletePropertyCallback) (JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef void
(*JSObjectGetPropertyNamesCallback) (JSContextRef ctx, JSObjectRef object, JSPropertyNameAccumulatorRef propertyNames);
typedef JSValueRef 
(*JSObjectCallAsFunctionCallback) (JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
typedef JSObjectRef 
(*JSObjectCallAsConstructorCallback) (JSContextRef ctx, JSObjectRef constructor, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
typedef bool 
(*JSObjectHasInstanceCallback)  (JSContextRef ctx, JSObjectRef constructor, JSValueRef possibleInstance, JSValueRef* exception);
typedef JSValueRef
(*JSObjectConvertToTypeCallback) (JSContextRef ctx, JSObjectRef object, JSType type, JSValueRef* exception);
typedef struct {
    const char* const name;
    JSObjectCallAsFunctionCallback callAsFunction;
    JSPropertyAttributes attributes;
} JSStaticFunction;

typedef struct {
    const char* const name;
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSPropertyAttributes attributes;
} JSStaticValue;

typedef struct {
    int                                 version; // current (and only) version is 0
    JSClassAttributes                   attributes;
	
    const char*                         className;
    JSClassRef                          parentClass;
	
    const JSStaticValue*                staticValues;
    const JSStaticFunction*             staticFunctions;
    
    JSObjectInitializeCallback          initialize;
    JSObjectFinalizeCallback            finalize;
    JSObjectHasPropertyCallback         hasProperty;
    JSObjectGetPropertyCallback         getProperty;
    JSObjectSetPropertyCallback         setProperty;
    JSObjectDeletePropertyCallback      deleteProperty;
    JSObjectGetPropertyNamesCallback    getPropertyNames;
    JSObjectCallAsFunctionCallback      callAsFunction;
    JSObjectCallAsConstructorCallback   callAsConstructor;
    JSObjectHasInstanceCallback         hasInstance;
    JSObjectConvertToTypeCallback       convertToType;
} JSClassDefinition;

typedef JSGlobalContextRef (*JSGlobalContextCreatePtr)(JSClassRef globalObjectClass);
typedef void (*JSGlobalContextReleasePtr)(JSGlobalContextRef ctx);
typedef JSObjectRef (*JSContextGetGlobalObjectPtr)(JSContextRef ctx);
typedef JSValueRef (*JSEvaluateScriptPtr)(JSContextRef ctx, JSStringRef script, JSObjectRef thisObject, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception);
typedef bool (*JSCheckScriptSyntaxPtr)(JSContextRef ctx, JSStringRef script, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception);
typedef JSStringRef (*JSStringCreateWithCFStringPtr)(CFStringRef string);
typedef CFStringRef (*JSStringCopyCFStringPtr)(CFAllocatorRef alloc, JSStringRef string);
typedef JSValueRef (*JSValueMakeStringPtr)(JSContextRef ctx, JSStringRef string);
typedef JSStringRef (*JSValueToStringCopyPtr)(JSContextRef ctx, JSValueRef value, JSValueRef* exception);
typedef JSObjectRef (*JSValueToObjectPtr)(JSContextRef ctx, JSValueRef value, JSValueRef* exception);
typedef void (*JSValueProtectPtr)(JSContextRef ctx, JSValueRef value);
typedef void (*JSValueUnprotectPtr)(JSContextRef ctx, JSValueRef value);
typedef void (*JSStringReleasePtr)(JSStringRef string);
typedef JSClassRef (*JSClassCreatePtr)(const JSClassDefinition* definition);
typedef void (*JSClassReleasePtr)(JSClassRef jsClass);
typedef JSObjectRef (*JSObjectMakePtr)(JSContextRef ctx, JSClassRef jsClass, void* data);
typedef void* (*JSObjectGetPrivatePtr)(JSObjectRef object);
typedef JSValueRef (*JSObjectGetPropertyPtr)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
typedef void (*JSObjectSetPropertyPtr)(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exception);
typedef JSValueRef (*JSObjectCallAsFunctionPtr)(JSContextRef ctx, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

static const char kJavaScriptCoreLibraryPath[] = "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore";

static const void *JavaScriptCoreLibrary = NULL;

static JSGlobalContextCreatePtr JSGlobalContextCreate;
static JSGlobalContextReleasePtr JSGlobalContextRelease;
static JSContextGetGlobalObjectPtr JSContextGetGlobalObject;
static JSEvaluateScriptPtr JSEvaluateScript;
static JSCheckScriptSyntaxPtr JSCheckScriptSyntax;
static JSStringCreateWithCFStringPtr JSStringCreateWithCFString;
static JSStringCopyCFStringPtr JSStringCopyCFString;
static JSValueMakeStringPtr JSValueMakeString;
static JSValueToStringCopyPtr JSValueToStringCopy;
static JSValueToObjectPtr JSValueToObject;
static JSValueProtectPtr JSValueProtect;
static JSValueUnprotectPtr JSValueUnprotect;
static JSStringReleasePtr JSStringRelease;
static JSClassCreatePtr JSClassCreate;
static JSClassReleasePtr JSClassRelease;
static JSObjectMakePtr JSObjectMake;
static JSObjectGetPrivatePtr JSObjectGetPrivate;
static JSObjectGetPropertyPtr JSObjectGetProperty;
static JSObjectSetPropertyPtr JSObjectSetProperty;
static JSObjectCallAsFunctionPtr JSObjectCallAsFunction;

///////////////////////////////////////////////////////////////////////////////

class MCPlatformScriptEnvironment
{
public:
	MCPlatformScriptEnvironment(void);
	~MCPlatformScriptEnvironment(void);
	
	void Retain(void);
	void Release(void);
	
	bool Define(const char *p_function, MCPlatformScriptEnvironmentCallback p_callback);
	
	void Run(MCStringRef p_script, MCStringRef &r_result);
	
	char *Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count);
	
private:
	struct Function
	{
		char *name;
		MCPlatformScriptEnvironmentCallback callback;
	};
	
	unsigned int m_references;
	
	JSGlobalContextRef m_runtime;
	
	Function *m_functions;
	uint4 m_function_count;
};

///////////////////////////////////////////////////////////////////////////////

#define GET_JSC_SYMBOL(sym) \
sym = (sym##Ptr)NSAddressOfSymbol(NSLookupSymbolInImage((const mach_header *)JavaScriptCoreLibrary, "_"#sym, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND));

///////////////////////////////////////////////////////////////////////////////

// SN-2014-12-22: [[ Bug 14278 ]] Parameter added to choose a UTF-8 string.
extern char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release, bool p_utf8 = false);

static bool ConvertMCStringToJSString(MCStringRef p_string, JSStringRef &r_js_string)
{
    MCAutoStringRefAsCFString t_cf_string;
    
    if (!t_cf_string . Lock(p_string))
        return false;
    
	JSStringRef t_js_string;
	t_js_string = JSStringCreateWithCFString(*t_cf_string);
	
	if (t_js_string == NULL)
		return false;
	
	r_js_string = t_js_string;
	return true;    
}

static bool ConvertCStringToJSString(const char *p_cstring, JSStringRef& r_js_string)
{
	CFStringRef t_cf_string;
	t_cf_string = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, p_cstring, kCFStringEncodingMacRoman, kCFAllocatorNull);
	if (t_cf_string == NULL)
		return false;
	
	JSStringRef t_js_string;
	t_js_string = JSStringCreateWithCFString(t_cf_string);
	CFRelease(t_cf_string);
	
	if (t_js_string == NULL)
		return false;
	
	r_js_string = t_js_string;
	return true;
}

static bool ConvertJSStringToCString(JSStringRef p_js_string, char*& r_cstring)
{
	CFStringRef t_cf_string;
	t_cf_string = JSStringCopyCFString(kCFAllocatorDefault, p_js_string);
	if (t_cf_string == NULL)
		return false;
	
	char *t_cstring;
	t_cstring = osx_cfstring_to_cstring(t_cf_string, true);
	
	if (t_cstring == NULL)
		return false;
	
	r_cstring = t_cstring;
	return true;
}

static bool ConvertCStringToJSValue(JSContextRef p_context, const char *p_cstring, JSValueRef& r_value)
{
	JSStringRef t_js_string;
	if (!ConvertCStringToJSString(p_cstring, t_js_string))
		return false;
	
	JSValueRef t_js_value;
	t_js_value = JSValueMakeString(p_context, t_js_string);
	
	JSStringRelease(t_js_string);
	
	if (t_js_value == NULL)
		return false;
	
	JSValueProtect(p_context, t_js_value);
	r_value = t_js_value;
	
	return true;
}

static bool ConvertJSValueToCString(JSContextRef p_context, JSValueRef p_value, char*& r_cstring)
{
	JSStringRef t_js_string;
	t_js_string = JSValueToStringCopy(p_context, p_value, NULL);
	if (t_js_string == NULL)
		return false;
	
	bool t_success;
	t_success = ConvertJSStringToCString(t_js_string, r_cstring);
	
	JSStringRelease(t_js_string);
	
	return t_success;
}

static JSValueRef InvokeHostFunction(JSContextRef p_context, JSObjectRef p_function, JSObjectRef p_this, size_t p_argument_count, const JSValueRef p_arguments[], JSValueRef *p_exception)
{
	MCPlatformScriptEnvironmentCallback t_callback;
	t_callback = (MCPlatformScriptEnvironmentCallback)JSObjectGetPrivate(p_function);
	
	bool t_success;
	t_success = true;
	
	char **t_arguments;
	t_arguments = NULL;
	if (t_success)
	{
		t_arguments = new char *[p_argument_count];
		if (t_arguments != NULL)
			memset(t_arguments, 0, sizeof(char *) * p_argument_count);
		else
			t_success = false;
	}
	
	if (t_success)
		for(uindex_t i = 0; i < p_argument_count; i++)
		{
			JSStringRef t_string_arg;
			t_string_arg = JSValueToStringCopy(p_context, p_arguments[i], NULL);
			if (t_string_arg != NULL)
			{
				ConvertJSStringToCString(t_string_arg, t_arguments[i]);
				JSStringRelease(t_string_arg);
			}
			
			if (t_arguments[i] == NULL)
				t_success = false;
		}
	
	JSValueRef t_result;
	t_result = NULL;
	if (t_success)
	{
		char *t_str_result;
		t_str_result = t_callback(t_arguments, p_argument_count);
		if (t_str_result != NULL)
		{
			JSStringRef t_js_string;
			if (ConvertCStringToJSString(t_str_result, t_js_string))
			{
				t_result = JSValueMakeString(p_context, t_js_string);
				JSStringRelease(t_js_string);
			}
		}
		delete t_str_result;
	}
	
	if (t_arguments != NULL)
	{
		for(uint4 i = 0; i < p_argument_count; ++i)
			delete t_arguments[i];
		delete t_arguments;
	}
	
	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

MCPlatformScriptEnvironment::MCPlatformScriptEnvironment(void)
{
	m_references = 1;
	m_runtime = NULL;
	m_functions = NULL;
	m_function_count = 0;
}

MCPlatformScriptEnvironment::~MCPlatformScriptEnvironment(void)
{
	for(uint4 i = 0; i < m_function_count; ++i)
		delete m_functions[i] . name;
	
	delete m_functions;
	
	if (m_runtime != NULL)
		JSGlobalContextRelease(m_runtime);
}

void MCPlatformScriptEnvironment::Retain(void)
{
	m_references += 1;
}

void MCPlatformScriptEnvironment::Release(void)
{
	m_references -= 1;
	if (m_references == 0)
		delete this;
}

bool MCPlatformScriptEnvironment::Define(const char *p_name, MCPlatformScriptEnvironmentCallback p_callback)
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

void MCPlatformScriptEnvironment::Run(MCStringRef p_script, MCStringRef &r_result)
{
	bool t_success;
	t_success = true;
	
	JSStringRef t_js_script;
	t_js_script = NULL;
	if (t_success)
		t_success = ConvertMCStringToJSString(p_script, t_js_script);
	
	JSGlobalContextRef t_runtime;
	t_runtime = NULL;
	if (t_success)
	{
		t_runtime = JSGlobalContextCreate(NULL);
		
		if (t_runtime == NULL)
			t_success = false;
	}
	
	if (t_success)
		t_success = JSCheckScriptSyntax(t_runtime, t_js_script, NULL, 0, NULL);
	
	JSObjectRef t_global;
	t_global = NULL;
	if (t_success)
	{
		t_global = JSContextGetGlobalObject(t_runtime);
		if (t_global == NULL)
			t_success = false;
	}
	
	JSClassRef t_function_class;
	t_function_class = NULL;
	if (t_success)
	{
		static JSClassDefinition s_function_class =
		{
			0, 0, "__livecode_script_environment_function__", NULL, NULL, NULL,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, InvokeHostFunction, NULL, NULL, NULL
		};
		
		t_function_class = JSClassCreate(&s_function_class);
		if (t_function_class == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		for(uint4 i = 0; i < m_function_count && t_success; ++i)
		{
			JSStringRef t_js_function_name;
			t_success = ConvertCStringToJSString(m_functions[i] . name, t_js_function_name);
			
			JSObjectRef t_function;
			t_function = NULL;
			if (t_success)
			{
				t_function = JSObjectMake(t_runtime, t_function_class, (void *)m_functions[i] . callback);
				if (t_function == NULL)
					t_success = false;
			}
			
			if (t_success)
				JSObjectSetProperty(t_runtime, t_global, t_js_function_name, t_function, 0, NULL);
			
			if (t_js_function_name != NULL)
				JSStringRelease(t_js_function_name);
		}
	}
	
	if (t_function_class != NULL)
		JSClassRelease(t_function_class);
	
	if (t_success)
	{
		JSValueRef t_eval;
		t_eval = JSEvaluateScript(t_runtime, t_js_script, NULL, NULL, 0, NULL);
	}
	
	if (t_js_script != NULL)
		JSStringRelease(t_js_script);
	
	if (t_success)
	{
		m_runtime = t_runtime;
		r_result = MCValueRetain(kMCEmptyString);
	}
	else
	{
		JSGlobalContextRelease(t_runtime);
		r_result = NULL;
	}
}

char *MCPlatformScriptEnvironment::Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count)
{
	if (m_runtime == NULL)
		return NULL;
	
	bool t_success;
	t_success = true;
	
	JSStringRef t_js_method_name;
	t_js_method_name = NULL;
	if (t_success)
		t_success = ConvertCStringToJSString(p_method, t_js_method_name);
	
	
	JSObjectRef t_js_global;
	t_js_global = NULL;
	if (t_success)
	{
		t_js_global = JSContextGetGlobalObject(m_runtime);
		if (t_js_global == NULL)
			t_success = false;
	}
	
	JSObjectRef t_js_method;
	t_js_method = NULL;
	if (t_success)
	{
		JSValueRef t_js_method_val;
		t_js_method_val = JSObjectGetProperty(m_runtime, t_js_global, t_js_method_name, NULL);
		if (t_js_method_val != NULL)
			t_js_method = JSValueToObject(m_runtime, t_js_method_val, NULL);
		if (t_js_method == NULL)
			t_success = false;
	}
	
	JSValueRef *t_arguments;
	t_arguments = NULL;
	if (t_success)
		t_success = MCMemoryNewArray(p_argument_count, t_arguments);
	
	if (t_success)
		for(uint4 i = 0; i < p_argument_count && t_success; ++i)
			t_success = ConvertCStringToJSValue(m_runtime, p_arguments[i], t_arguments[i]);
	
	JSValueRef t_js_result;
	t_js_result = NULL;
	if (t_success)
	{
		t_js_result = JSObjectCallAsFunction(m_runtime, t_js_method, NULL, p_argument_count, p_argument_count != 0 ? t_arguments : NULL, NULL);
		if (t_js_result == NULL)
			t_success = false;
	}
	
	char *t_result;
	t_result = NULL;
	if (t_success)
		t_success = ConvertJSValueToCString(m_runtime, t_js_result, t_result);
	
	if (t_arguments != nil)
	{
		for(uindex_t i = 0; i < p_argument_count; i++)
			JSValueUnprotect(m_runtime, t_arguments[i]);
		MCMemoryDeleteArray(t_arguments);
	}
	
	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

// SN-2014-07-23: [[ Bug 12907 ]]
//  Update as well MCSreenDC::createscriptenvironment (and callees)
void MCPlatformScriptEnvironmentCreate(MCStringRef language, MCPlatformScriptEnvironmentRef& r_env)
{
	if (JavaScriptCoreLibrary == NULL)
	{
		JavaScriptCoreLibrary = (void *)NSAddImage(kJavaScriptCoreLibraryPath, NSADDIMAGE_OPTION_WITH_SEARCHING | NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME);
		GET_JSC_SYMBOL(JSGlobalContextCreate);
		GET_JSC_SYMBOL(JSGlobalContextRelease);
		GET_JSC_SYMBOL(JSContextGetGlobalObject);
		GET_JSC_SYMBOL(JSEvaluateScript);
		GET_JSC_SYMBOL(JSCheckScriptSyntax);
		GET_JSC_SYMBOL(JSStringCreateWithCFString);
		GET_JSC_SYMBOL(JSStringCopyCFString);
		GET_JSC_SYMBOL(JSValueMakeString);
		GET_JSC_SYMBOL(JSValueToStringCopy);
		GET_JSC_SYMBOL(JSValueToObject);
		GET_JSC_SYMBOL(JSValueProtect);
		GET_JSC_SYMBOL(JSValueUnprotect);
		GET_JSC_SYMBOL(JSStringRelease);
		GET_JSC_SYMBOL(JSClassCreate);
		GET_JSC_SYMBOL(JSClassRelease);
		GET_JSC_SYMBOL(JSObjectMake);
		GET_JSC_SYMBOL(JSObjectGetPrivate);
		GET_JSC_SYMBOL(JSObjectGetProperty);
		GET_JSC_SYMBOL(JSObjectSetProperty);
		GET_JSC_SYMBOL(JSObjectCallAsFunction);
	}
	
	r_env = new MCPlatformScriptEnvironment();
}

void MCPlatformScriptEnvironmentRetain(MCPlatformScriptEnvironmentRef env)
{
	env -> Retain();
}

void MCPlatformScriptEnvironmentRelease(MCPlatformScriptEnvironmentRef env)
{
	env -> Release();
}

bool MCPlatformScriptEnvironmentDefine(MCPlatformScriptEnvironmentRef env, const char *function, MCPlatformScriptEnvironmentCallback callback)
{
	return env -> Define(function, callback);
}

void MCPlatformScriptEnvironmentRun(MCPlatformScriptEnvironmentRef env, MCStringRef script, MCStringRef& r_result)
{
    env -> Run(script, r_result);
}

void MCPlatformScriptEnvironmentCall(MCPlatformScriptEnvironmentRef env, const char *method, const char **arguments, uindex_t argument_count, char*& r_result)
{
	r_result = env -> Call(method, arguments, argument_count);
}

////////////////////////////////////////////////////////////////////////////////
