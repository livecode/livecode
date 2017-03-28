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

#include "mac-platform.h"

#include <mach-o/dyld.h>

///////////////////////////////////////////////////////////////////////////////

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

// SN-2014-12-22: [[ Bug 14278 ]] Parameter added to choose a UTF-8 string.
char *ConvertCFStringToCString(CFStringRef p_string, bool p_release = true, bool p_utf8_string = false)
{
    bool t_success;
    t_success = true;
    
    if (p_string == NULL)
        t_success = false;
    
    char *t_cstring;
    t_cstring = NULL;
    if (t_success)
    {
        CFIndex t_string_length;
        t_string_length = CFStringGetLength(p_string);
        
        // SN-2014-12-22: [[ Bug 14278 ]] Parameter added to choose a UTF-8 string.
        CFStringEncoding t_encoding;
        if (p_utf8_string)
            t_encoding = kCFStringEncodingUTF8;
        else
            t_encoding = kCFStringEncodingMacRoman;
        
        CFIndex t_buffer_size;
        t_buffer_size = CFStringGetMaximumSizeForEncoding(t_string_length, t_encoding) + 1;
        t_cstring = (char *)malloc(t_buffer_size);
        
        if (t_cstring != NULL)
        {
            // MW-2012-03-15: [[ Bug 9935 ]] Use CFStringGetBytes() so that '?' is substituted for any non-
            //   mappable chars.
            CFIndex t_used;
            CFStringGetBytes(p_string, CFRangeMake(0, CFStringGetLength(p_string)), t_encoding, '?', False, (UInt8*)t_cstring, t_buffer_size, &t_used);
            t_cstring[t_used] = '\0';
        }
        else
            t_success = false;
        
        if (t_success)
            t_cstring = (char *)realloc(t_cstring, strlen(t_cstring) + 1);
    }
    
    if (!t_success)
    {
        if (t_cstring != NULL)
            free(t_cstring);
        t_cstring = NULL;
    }
    
    if (p_string != NULL && p_release)
        CFRelease(p_string);
    
    return t_cstring;
}

static bool ConvertJSStringToCString(JSStringRef p_js_string, char*& r_cstring)
{
	CFStringRef t_cf_string;
	t_cf_string = JSStringCopyCFString(kCFAllocatorDefault, p_js_string);
	if (t_cf_string == NULL)
		return false;
	
	char *t_cstring;
	t_cstring = ConvertCFStringToCString(t_cf_string, true);
	
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

MCMacPlatformScriptEnvironment::~MCMacPlatformScriptEnvironment(void)
{
	for(uint4 i = 0; i < m_function_count; ++i)
		delete m_functions[i] . name;
	
	delete m_functions;
	
	if (m_runtime != NULL)
		JSGlobalContextRelease(m_runtime);
}

bool MCMacPlatformScriptEnvironment::Define(const char *p_name, MCPlatformScriptEnvironmentCallback p_callback)
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

void MCMacPlatformScriptEnvironment::Run(MCStringRef p_script, MCStringRef &r_result)
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

char *MCMacPlatformScriptEnvironment::Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count)
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

MCPlatformScriptEnvironmentRef MCMacPlatformCore::CreateScriptEnvironment()
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

    MCPlatform::Ref<MCPlatformScriptEnvironment> t_ref = MCPlatform::makeRef<MCMacPlatformScriptEnvironment>();
    t_ref -> SetPlatform(this);
    t_ref -> SetCallback(m_callback);
    
    return t_ref.unsafeTake();
}

///////////////////////////////////////////////////////////////////////////////
