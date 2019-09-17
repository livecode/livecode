/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include <core.h>
#include <jni.h>

#include "libbrowser_internal.h"

////////////////////////////////////////////////////////////////////////////////

#define LIBBROWSER_DUMMY_URL "http://libbrowser_dummy_url/"

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

MCBrowserFactoryMap kMCBrowserFactoryMap[] =
{
	{ "WebView", nil, MCAndroidWebViewBrowserFactoryCreate },
	{ nil, nil, nil },
};

MCBrowserFactoryMap* s_factory_list = kMCBrowserFactoryMap;

////////////////////////////////////////////////////////////////////////////////

#include <sys/time.h>

double MCBrowserTimeInSeconds()
{
	struct timezone tz;
	struct timeval tv;

	gettimeofday(&tv, &tz);
	return tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

bool MCBrowserJavaStringToUtf8String(JNIEnv* env, jstring p_java_string, char *&r_utf8_string)
{
    bool t_success = true;
    
    const char *t_utf8_string = nil;
    
    if (p_java_string == nil)
    {
		r_utf8_string = nil;
		return true;
	}
	
	t_success = nil != (t_utf8_string = env -> GetStringUTFChars(p_java_string, NULL));
    
    if (t_success)
    {
		t_success = MCCStringCloneSubstring(t_utf8_string, env -> GetStringUTFLength(p_java_string), r_utf8_string);
        env -> ReleaseStringUTFChars(p_java_string, t_utf8_string);
    }
    
    return t_success;
}

class MCBrowserJavaConverter
{
public:
	MCBrowserJavaConverter(JNIEnv *p_env) : m_env(p_env)
	{
		m_boolean_class = nil;
		m_boolean_value_method = nil;

		m_integer_class = nil;
		m_integer_value_method = nil;

		m_double_class = nil;
		m_double_value_method = nil;

		m_string_class = nil;
		
		m_jsonarray_class = nil;
		m_jsonarray_length_method = nil;
		m_jsonarray_get_method = nil;
		
		m_jsonobject_class = nil;
		m_jsonobject_names_method = nil;
		m_jsonobject_get_method = nil;
	}
	
	~MCBrowserJavaConverter()
	{
	}
	
	bool IsBoolean(jobject p_obj)
	{
		if (!InitBoolean())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_boolean_class);
	}
	
	bool IsInteger(jobject p_obj)
	{
		if (!InitInteger())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_integer_class);
	}
	
	bool IsDouble(jobject p_obj)
	{
		if (!InitDouble())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_double_class);
	}
	
	bool IsString(jobject p_obj)
	{
		if (!InitString())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_string_class);
	}
	
	bool IsJSONArray(jobject p_obj)
	{
		if (!InitJSONArray())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_jsonarray_class);
	}
	
	bool IsJSONObject(jobject p_obj)
	{
		if (!InitJSONObject())
			return false;
		
		return m_env->IsInstanceOf(p_obj, m_jsonobject_class);
	}
	
	bool GetBooleanValue(jobject p_obj, bool &r_value)
	{
		if (!InitBoolean())
			return false;
		
		r_value = m_env->CallBooleanMethod(p_obj, m_boolean_value_method);
		return true;
	}
	
	bool GetIntegerValue(jobject p_obj, int &r_value)
	{
		if (!InitInteger())
			return false;
		
		r_value = m_env->CallIntMethod(p_obj, m_integer_value_method);
		return true;
	}
	
	bool GetDoubleValue(jobject p_obj, double &r_value)
	{
		if (!InitDouble())
			return false;
		
		r_value = m_env->CallDoubleMethod(p_obj, m_double_value_method);
		return true;
	}
	
	bool GetStringValue(jstring p_string, char *&r_string)
	{
		return MCBrowserJavaStringToUtf8String(m_env, p_string, r_string);
	}
	
	bool GetBrowserValue(jobject p_obj, MCBrowserValue &r_value)
	{
		bool t_success;
		t_success = true;
		
		if (IsBoolean(p_obj))
		{
			bool t_val;
			t_success = GetBooleanValue(p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetBoolean(r_value, t_val);
		}
		else if (IsInteger(p_obj))
		{
			int t_val;
			t_success = GetIntegerValue(p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetInteger(r_value, t_val);
		}
		else if (IsDouble(p_obj))
		{
			double t_val;
			t_success = GetDoubleValue(p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetDouble(r_value, t_val);
		}
		else if (IsString(p_obj))
		{
			char *t_val;
			t_val = nil;
			t_success = GetStringValue((jstring)p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetUTF8String(r_value, t_val);
			
			if (t_val != nil)
				MCCStringFree(t_val);
		}
		else if (IsJSONArray(p_obj))
		{
			MCBrowserListRef t_val;
			t_val = nil;
			t_success = GetJSONArrayValue(p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetList(r_value, t_val);
			
			if (t_val != nil)
				MCBrowserListRelease(t_val);
		}
		else if (IsJSONObject(p_obj))
		{
			MCBrowserDictionaryRef t_val;
			t_val = nil;
			t_success = GetJSONObjectValue(p_obj, t_val);
			
			if (t_success)
				t_success = MCBrowserValueSetDictionary(r_value, t_val);
			
			if (t_val != nil)
				MCBrowserDictionaryRelease(t_val);
		}
		else
		{
			MCLog("Convert: unhandled object class: %p", p_obj);
			MCBrowserValueClear(r_value);
		}
		
		return t_success;
	}
	
	bool GetJSONArrayLength(jobject p_array, uint32_t &r_length)
	{
		if (!InitJSONArray())
			return false;
		
		r_length = m_env->CallIntMethod(p_array, m_jsonarray_length_method);
		return true;
	}
	
	bool GetJSONArrayElement(jobject p_array, uint32_t p_index, jobject &r_element)
	{
		if (!InitJSONArray())
			return false;
		
		r_element = m_env->CallObjectMethod(p_array, m_jsonarray_get_method, (int)p_index);
		return true;
	}
	
	bool GetJSONArrayValue(jobject p_array, MCBrowserListRef &r_list)
	{
		if (!InitJSONArray())
			return  false;
		
		bool t_success;
		t_success = true;
		
		uint32_t t_size;
		if (t_success)
			t_success = GetJSONArrayLength(p_array, t_size);

		MCBrowserListRef t_list;
		t_list = nil;
		
		if (t_success)
			t_success = MCBrowserListCreate(t_list, t_size);
		
		for (uint32_t i = 0; t_success && i < t_size; i++)
		{
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			jobject t_obj;
			t_obj = nil;
			
			t_success = GetJSONArrayElement(p_array, i, t_obj);
			
			if (t_success)
				t_success = GetBrowserValue(t_obj, t_value);
			
			if (t_success)
				t_success = MCBrowserListSetValue(t_list, i, t_value);
			
			if (t_obj != nil)
				m_env->DeleteLocalRef(t_obj);
			
			MCBrowserValueClear(t_value);
		}
		
		if (t_success)
			r_list = t_list;
		else
			MCBrowserListRelease(t_list);
			
		return t_success;
	}
	
	bool GetJSONObjectNames(jobject p_object, jobject &r_names_array)
	{
		if (!InitJSONObject())
			return false;
		
		r_names_array = m_env->CallObjectMethod(p_object, m_jsonobject_names_method);
		return true;
	}
	
	bool GetJSONObjectElement(jobject p_object, jstring p_key, jobject &r_element)
	{
		if (!InitJSONObject())
			return false;
		
		r_element = m_env->CallObjectMethod(p_object, m_jsonobject_get_method, p_key);
		return true;
	}
	
	bool GetJSONObjectValue(jobject p_object, MCBrowserDictionaryRef &r_dict)
	{
		if (!InitJSONObject())
			return true;
		
		bool t_success;
		t_success = true;
		
		jobject t_names;
		t_names = nil;
		if (t_success)
			t_success = GetJSONObjectNames(p_object, t_names);
		
		uint32_t t_size;
		if (t_success)
			t_success = GetJSONArrayLength(t_names, t_size);

		MCBrowserDictionaryRef t_dict;
		t_dict = nil;
		
		if (t_success)
			t_success = MCBrowserDictionaryCreate(t_dict, t_size);
		
		for (uint32_t i = 0; t_success && i < t_size; i++)
		{
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			jstring t_key;
			t_key = nil;
			
			if (t_success)
				t_success = GetJSONArrayElement(t_names, i, (jobject&)t_key);
			
			char *t_key_string;
			t_key_string = nil;
			
			if (t_success)
				t_success = GetStringValue(t_key, t_key_string);
			
			jobject t_obj;
			t_obj = nil;
			if (t_success)
				t_success = GetJSONObjectElement(p_object, t_key, t_obj);
			
			if (t_success)
				t_success = GetBrowserValue(t_obj, t_value);
			
			if (t_success)
				t_success = MCBrowserDictionarySetValue(t_dict, t_key_string, t_value);
			
			if (t_key_string != nil)
				MCCStringFree(t_key_string);
			
			if (t_key != nil)
				m_env->DeleteLocalRef(t_key);
			
			if (t_obj != nil)
				m_env->DeleteLocalRef(t_obj);
			
			MCBrowserValueClear(t_value);
		}
		
		if (t_names != nil)
			m_env->DeleteLocalRef(t_names);
		
		if (t_success)
			r_dict = t_dict;
		else
			MCBrowserDictionaryRelease(t_dict);
			
		return t_success;
	}
	
private:
	bool InitClass(const char *p_class, jclass &r_class)
	{
		if (m_env == nil)
			return false;

		jclass t_class;
		t_class = nil;
		
		t_class = m_env->FindClass(p_class);
		if (t_class == nil)
			return false;
		
		r_class = t_class;
		return true;
	}
	
	bool InitMethod(jclass p_class, const char *p_method, const char *p_signature, jmethodID &r_method)
	{
		if (m_env == nil || p_class == nil)
			return false;

		jmethodID t_method;
		t_method = nil;
		
		t_method = m_env->GetMethodID(p_class, p_method, p_signature);
		if (t_method == nil)
			return false;
		
		r_method = t_method;
		return true;
	}
	
	bool InitBoolean()
	{
		if (m_boolean_class == nil && !InitClass("java/lang/Boolean", m_boolean_class))
			return false;
		
		if (m_boolean_value_method == nil && !InitMethod(m_boolean_class, "booleanValue", "()Z", m_boolean_value_method))
			return false;
		
		return true;
	}
	
	bool InitInteger()
	{
		if (m_integer_class == nil && !InitClass("java/lang/Integer", m_integer_class))
			return false;
		
		if (m_integer_value_method == nil && !InitMethod(m_integer_class, "intValue", "()I", m_integer_value_method))
			return false;
		
		return true;
	}
	
	bool InitDouble()
	{
		if (m_double_class == nil && !InitClass("java/lang/Double", m_double_class))
			return false;
		
		if (m_double_value_method == nil && !InitMethod(m_double_class, "doubleValue", "()D", m_double_value_method))
			return false;
		
		return true;
	}
	
	bool InitString()
	{
		if (m_string_class == nil && !InitClass("java/lang/String", m_string_class))
			return false;
		
		return true;
	}
	
	bool InitJSONArray()
	{
		if (m_jsonarray_class == nil && !InitClass("org/json/JSONArray", m_jsonarray_class))
			return false;
		
		if (m_jsonarray_length_method == nil && !InitMethod(m_jsonarray_class, "length", "()I", m_jsonarray_length_method))
			return false;
		
		if (m_jsonarray_get_method == nil && !InitMethod(m_jsonarray_class, "get", "(I)Ljava/lang/Object;", m_jsonarray_get_method))
			return false;
		
		return true;
	}
	
	bool InitJSONObject()
	{
		if (m_jsonobject_class == nil && !InitClass("org/json/JSONObject", m_jsonobject_class))
			return false;
		
		if (m_jsonobject_names_method == nil && !InitMethod(m_jsonobject_class, "names", "()Lorg/json/JSONArray;", m_jsonobject_names_method))
			return false;
		
		if (m_jsonobject_get_method == nil && !InitMethod(m_jsonobject_class, "get", "(Ljava/lang/String;)Ljava/lang/Object;", m_jsonobject_get_method))
			return false;
		
		return true;
	}
	
	jclass m_boolean_class;
	jmethodID m_boolean_value_method;

	jclass m_integer_class;
	jmethodID m_integer_value_method;

	jclass m_double_class;
	jmethodID m_double_value_method;

	jclass m_string_class;

	jclass m_jsonarray_class;
	jmethodID m_jsonarray_length_method;
	jmethodID m_jsonarray_get_method;
	
	jclass m_jsonobject_class;
	jmethodID m_jsonobject_names_method;
	jmethodID m_jsonobject_get_method;
	
	JNIEnv *m_env;
};

bool MCBrowserJavaJSONArrayToMCBrowserList(JNIEnv *env, jobject p_array, MCBrowserListRef &r_list)
{
	MCBrowserJavaConverter t_converter(env);
	
	return t_converter.GetJSONArrayValue(p_array, r_list);
}

////////////////////////////////////////////////////////////////////////////////

extern void MCAndroidObjectRemoteCall(jobject p_object, const char *p_method, const char *p_signature, void *p_return_value, ...);
extern void MCAndroidStaticRemoteCall(const char *p_class_name, const char *p_method, const char *p_signature, void *p_return_value, ...);
extern JNIEnv *MCJavaGetThreadEnv();

class MCAndroidWebViewBrowser : public MCBrowserBase
{
private:
	jobject m_view;
	
	char *m_js_tag;
	char *m_js_result;

public:
	MCAndroidWebViewBrowser() : m_view(nil), m_js_tag(nil), m_js_result(nil)
	{
		MCBrowserBase::BrowserListAdd(this);
	}
	
	~MCAndroidWebViewBrowser()
	{
		MCBrowserBase::BrowserListRemove(this);
		
		if (m_view != nil)
		{
			JNIEnv *env;
			env = MCJavaGetThreadEnv();
			
			env->DeleteGlobalRef(m_view);
		}
	}
	
	bool Init()
	{
		jobject t_view;
		t_view = nil;
		
		MCAndroidStaticRemoteCall("com/runrev/android/libraries/LibBrowser", "createBrowserView", "o", &t_view);
		
		if (t_view == nil)
			return false;
		
		m_view = t_view;
		
		return true;
	}
	
	virtual void *GetNativeLayer()
	{
		return m_view;
	}
	
	virtual bool GetRect(MCBrowserRect &r_rect)
	{
		// TODO - implement (not needed when used with native layers)
		return false;
	}
	
	virtual bool SetRect(const MCBrowserRect &p_rect)
	{
		// TODO - implement (not needed when used with native layers)
		return false;
	}
	
	virtual bool GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
	{
		switch (p_property)
		{
			case kMCBrowserVerticalScrollbarEnabled:
				return GetVerticalScrollbarEnabled(r_value);
				
			case kMCBrowserHorizontalScrollbarEnabled:
				return GetHorizontalScrollbarEnabled(r_value);
			
			case kMCBrowserIsSecure:
				return GetIsSecure(r_value);

			case kMCBrowserAllowUserInteraction:
				return GetAllowUserInteraction(r_value);

			default:
				break;
		}
		
		return true;
	}
	
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value)
	{
		switch (p_property)
		{
			case kMCBrowserVerticalScrollbarEnabled:
				return SetVerticalScrollbarEnabled(p_value);
			
			case kMCBrowserHorizontalScrollbarEnabled:
				return SetHorizontalScrollbarEnabled(p_value);

			case kMCBrowserAllowUserInteraction:
				return SetAllowUserInteraction(p_value);

			default:
				break;
		}
		
		return true;
	}
	
	virtual bool GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string)
	{
		switch (p_property)
		{
			case kMCBrowserURL:
				return GetUrl(r_utf8_string);
			
			case kMCBrowserUserAgent:
				return GetUserAgent(r_utf8_string);
				
			case kMCBrowserJavaScriptHandlers:
				return GetJavaScriptHandlers(r_utf8_string);
				
			case kMCBrowserHTMLText:
				return GetHTMLText(r_utf8_string);
				
			default:
				break;
		}
		
		return true;
	}
	
	virtual bool SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string)
	{
		switch (p_property)
		{
			case kMCBrowserUserAgent:
				return SetUserAgent(p_utf8_string);
			
			case kMCBrowserJavaScriptHandlers:
				return SetJavaScriptHandlers(p_utf8_string);
				
			case kMCBrowserHTMLText:
				return SetHTMLText(p_utf8_string);
				
			default:
				break;
		}
		
		return true;
	}
	
	virtual bool SetIntegerProperty(MCBrowserProperty p_property, int32_t p_value)
	{
		switch (p_property)
		{
			default:
				break;
		}
		
		return true;
	}

	virtual bool GetIntegerProperty(MCBrowserProperty p_property, int32_t &r_value)
	{
		switch (p_property)
		{
			default:
				break;
		}
		
		return true;
	}

	virtual bool GoBack()
	{
		MCAndroidObjectRemoteCall(m_view, "goBack", "vi", nil, 1);
		return true;
	}
	
	virtual bool GoForward()
	{
		MCAndroidObjectRemoteCall(m_view, "goForward", "vi", nil, 1);
		return true;
	}
	
	virtual bool GoToURL(const char *p_url)
	{
		MCAndroidObjectRemoteCall(m_view, "setUrl", "vt", nil, p_url);
		return true;
	}
	
	virtual bool LoadHTMLText(const char *p_htmltext, const char *p_base_url)
	{
		MCAndroidObjectRemoteCall(m_view, "loadHtml", "vtt", nil, p_base_url, p_htmltext);
		return true;
	}

	virtual bool StopLoading(void)
	{
		MCAndroidObjectRemoteCall(m_view, "stopLoading", "v", nil);
		return true;
	}
	
	virtual bool Reload(void)
	{
		MCAndroidObjectRemoteCall(m_view, "reload", "v", nil);
		return true;
	}
	
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result)
	{
		if (m_js_tag != nil)
			return false;
			
		MCAndroidObjectRemoteCall(m_view, "executeJavaScript", "tt", &m_js_tag, p_script);
		
		// wait for result, timeout after 30 seconds    
		double t_current_time = MCBrowserTimeInSeconds();
		double t_timeout = t_current_time + 30.0;
		
		while (m_js_tag != nil && t_current_time < t_timeout)
		{
			MCBrowserRunloopWait();
			t_current_time = MCBrowserTimeInSeconds();
		}
		
		if (m_js_tag != nil)
		{
			// timeout
			MCCStringFree(m_js_tag);
			m_js_tag = nil;
			return false;
		}
		
		r_result = m_js_result;
		m_js_result = nil;
		
		return true;
	}
	
	void SetJavaScriptResult(const char *p_tag, const char *p_result)
	{
		if (p_tag != nil && m_js_tag != nil && MCCStringEqual(m_js_tag, p_tag))
		{
			MCCStringFree(m_js_tag);
			m_js_tag = nil;
			/* UNCHECKED */ MCCStringClone(p_result, m_js_result);
			
			MCBrowserRunloopBreakWait();
		}
	}
	
	//////////
	
private:
	bool GetVerticalScrollbarEnabled(bool &r_value)
	{
		MCAndroidObjectRemoteCall(m_view, "getVerticalScrollbarEnabled", "b", &r_value);
		return true;
	}
	
	bool SetVerticalScrollbarEnabled(bool p_value)
	{
		MCAndroidObjectRemoteCall(m_view, "setVerticalScrollbarEnabled", "vb", nil, p_value);
		return true;
	}
	
	bool GetHorizontalScrollbarEnabled(bool &r_value)
	{
		MCAndroidObjectRemoteCall(m_view, "getHorizontalScrollbarEnabled", "b", &r_value);
		return true;
	}
	
	bool SetHorizontalScrollbarEnabled(bool p_value)
	{
		MCAndroidObjectRemoteCall(m_view, "setHorizontalScrollbarEnabled", "vb", nil, p_value);
		return true;
	}
	
	bool GetUrl(char *&r_utf8_string)
	{
		char *t_url;
		t_url = nil;
		
		MCAndroidObjectRemoteCall(m_view, "getUrl", "t", &t_url);
		if (t_url == nil)
			return MCCStringClone("", r_utf8_string);
			
		r_utf8_string = t_url;
		return true;
	}
	
	bool GetHTMLText(char *&r_htmltext)
	{
		return EvaluateJavaScript("document.documentElement.outerHTML", r_htmltext);
	}
	
	bool SetHTMLText(const char *p_utf8_string)
	{
		return LoadHTMLText(p_utf8_string, LIBBROWSER_DUMMY_URL);
	}
	
	bool GetUserAgent(char *&r_useragent)
	{
		char *t_useragent;
		t_useragent = nil;
		MCAndroidObjectRemoteCall(m_view, "getUserAgent", "t", &t_useragent);

		if (t_useragent == nil)
			return false;
			
		r_useragent = t_useragent;
		return true;
	}
	
	bool SetUserAgent(const char *p_useragent)
	{
		MCAndroidObjectRemoteCall(m_view, "setUserAgent", "vt", nil, p_useragent);
		return true;
	}
	
	bool GetJavaScriptHandlers(char *&r_js_handlers)
	{
		char *t_handlers;
		t_handlers = nil;
		MCAndroidObjectRemoteCall(m_view, "getJavaScriptHandlers", "t", &t_handlers);
		
		if (t_handlers == nil)
			return false;
		
		r_js_handlers = t_handlers;
		return true;
	}
	
	bool SetJavaScriptHandlers(const char *p_js_handlers)
	{
		MCAndroidObjectRemoteCall(m_view, "setJavaScriptHandlers", "vt", nil, p_js_handlers);
		return true;
	}

	bool GetIsSecure(bool &r_value)
	{
		MCAndroidObjectRemoteCall(m_view, "getIsSecure", "b", &r_value);
		return true;
	}

	bool GetAllowUserInteraction(bool &r_value)
	{
		MCAndroidObjectRemoteCall(m_view, "getAllowUserInteraction", "b", &r_value);
		return true;
	}

	bool SetAllowUserInteraction(bool p_value)
	{
		MCAndroidObjectRemoteCall(m_view, "setAllowUserInteraction", "vb", nil, p_value);
		return true;
	}
};

//////////

struct __browser_find_with_java_view_context
{
	JNIEnv* env;
	jobject view;
	MCBrowser *browser;
};

static bool __browser_find_with_java_view_callback(MCBrowser *p_browser, void *p_context)
{
	__browser_find_with_java_view_context *context;
	context = (__browser_find_with_java_view_context*)p_context;
	
	if (context->env->IsSameObject(context->view, (jobject)p_browser->GetNativeLayer()))
	{
		context->browser = p_browser;
		return false; // End iterator once browser found
	}
	
	return true;
}

bool MCBrowserFindWithJavaView(JNIEnv *env, jobject p_view, MCBrowser *&r_browser)
{
	__browser_find_with_java_view_context context;
	context.env = env;
	context.view = p_view;
	context.browser = nil;
	
	MCBrowserBase::BrowserListIterate(__browser_find_with_java_view_callback, &context);
	
	if (context.browser == nil)
		return false;
		
	r_browser = context.browser;
	return true;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doCallJSHandler(JNIEnv *env, jobject object, jstring handler, jobject args) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doCallJSHandler(JNIEnv *env, jobject object, jstring handler, jobject args)
{
	bool t_success;
	t_success = true;
	
	char *t_handler;
	t_handler = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, handler, t_handler);
	
	MCBrowserListRef t_args;
	t_args = nil;
	if (t_success)
		t_success = MCBrowserJavaJSONArrayToMCBrowserList(env, args, t_args);
	
	MCBrowser *t_browser;
	if (t_success && MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->OnJavaScriptCall(t_handler, t_args);
	
	if (t_handler != nil)
		MCCStringFree(t_handler);
	if (t_args != nil)
		MCBrowserListRelease(t_args);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result)
{
	MCLog("doJSExecutionResult");
	
	bool t_success;
	t_success = true;
	
	char *t_tag;
	t_tag = nil;
    if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, tag, t_tag);
	
	char *t_result;
	t_result = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, result, t_result);
	
	MCBrowser *t_browser;
	if (t_success && MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->SetJavaScriptResult(t_tag, t_result);
	
	if (t_tag != nil)
		MCCStringFree(t_tag);
	if (t_result != nil)
		MCCStringFree(t_result);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doStartedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doStartedLoading(JNIEnv *env, jobject object, jstring url)
{
	bool t_success;
	t_success = true;
	
	char *t_url;
	t_url= nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, url, t_url);
    
	if (t_success && !MCCStringBeginsWith(t_url, LIBBROWSER_DUMMY_URL))
	{
		MCBrowser *t_browser;
		if (MCBrowserFindWithJavaView(env, object, t_browser))
		{
			((MCAndroidWebViewBrowser*)t_browser)->OnNavigationBegin(false, t_url);
			((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadBegin(false, t_url);
		}
	}

	if (t_url != nil)
		MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doFinishedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doFinishedLoading(JNIEnv *env, jobject object, jstring url)
{
	bool t_success;
	t_success = true;
	
	char *t_url;
	t_url= nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, url, t_url);
    
	if (t_success && !MCCStringBeginsWith(t_url, LIBBROWSER_DUMMY_URL))
	{
		MCBrowser *t_browser;
		if (MCBrowserFindWithJavaView(env, object, t_browser))
		{
			((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadComplete(false, t_url);
			((MCAndroidWebViewBrowser*)t_browser)->OnNavigationComplete(false, t_url);
		}
	}

	if (t_url != nil)
		MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error)
{
	bool t_success;
	t_success = true;
	
	char *t_url;
	t_url = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, url, t_url);
    
    char *t_error;
    t_error = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, error, t_error);
    
	if (t_success && !MCCStringBeginsWith(t_url, LIBBROWSER_DUMMY_URL))
	{
		MCBrowser *t_browser;
		if (MCBrowserFindWithJavaView(env, object, t_browser))
		{
			((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadFailed(false, t_url, t_error);
			((MCAndroidWebViewBrowser*)t_browser)->OnNavigationFailed(false, t_url, t_error);
		}
	}

	if (t_url != nil)
		MCCStringFree(t_url);
	if (t_error != nil)
		MCCStringFree(t_error);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doUnsupportedScheme(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doUnsupportedScheme(JNIEnv *env, jobject object, jstring url)
{
	bool t_success;
	t_success = true;
	
	char *t_url;
	t_url = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, url, t_url);
	
	if (t_success && !MCCStringBeginsWith(t_url, LIBBROWSER_DUMMY_URL))
	{
		MCBrowser *t_browser;
		if (MCBrowserFindWithJavaView(env, object, t_browser))
		{
			((MCAndroidWebViewBrowser*)t_browser)->OnNavigationRequestUnhandled(false, t_url);
		}
	}
	
	if (t_url != nil)
		MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doProgressChanged(JNIEnv *env, jobject object, jstring url, jint progress) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doProgressChanged(JNIEnv *env, jobject object, jstring url, jint progress)
{
	bool t_success = true;

	char *t_url = nil;
	if (t_success)
		t_success = MCBrowserJavaStringToUtf8String(env, url, t_url);

	if (t_success && !MCCStringBeginsWith(t_url, LIBBROWSER_DUMMY_URL))
	{
		MCBrowser *t_browser;
		if (MCBrowserFindWithJavaView(env, object, t_browser))
			((MCAndroidWebViewBrowser*)t_browser)->OnProgressChanged(t_url, progress);
	}

	if (t_url != nil)
		MCCStringFree(t_url);
}

////////////////////////////////////////////////////////////////////////////////

class MCAndroidWebViewBrowserFactory : public MCBrowserFactory
{
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
	{
		MCAndroidWebViewBrowser *t_browser;
		t_browser = new (nothrow) MCAndroidWebViewBrowser();
		
		if (t_browser == nil)
			return false;
			
		if (!t_browser->Init())
		{
			delete t_browser;
			return false;
		}
		
		r_browser = t_browser;
		
		return true;
	}
};

bool MCAndroidWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory)
{
	MCBrowserFactory *t_factory = new (nothrow) MCAndroidWebViewBrowserFactory();
	
	if (t_factory == nil)
		return false;
		
	r_factory = t_factory;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
