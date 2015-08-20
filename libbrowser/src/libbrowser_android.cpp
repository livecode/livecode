/* Copyright (C) 2015 Runtime Revolution Ltd.
 
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

bool MCAndroidWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

MCBrowserFactoryMap s_factory_list[] =
{
	{ "androidwebview", nil, MCAndroidWebViewBrowserFactoryCreate },
	{ nil, nil, nil },
};

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
			case kMCBrowserScrollbars:
				return GetScrollbarsEnabled(r_value);
			
			default:
				break;
		}
		
		return true;
	}
	
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value)
	{
		switch (p_property)
		{
			case kMCBrowserScrollbars:
				return SetScrollbarsEnabled(p_value);
			
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
		MCAndroidObjectRemoteCall(m_view, "setUrl", "vs", nil, p_url);
		return true;
	}
	
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result)
	{
		if (m_js_tag != nil)
			return false;
			
		MCAndroidObjectRemoteCall(m_view, "executeJavaScript", "ss", &m_js_tag, p_script);
		
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
		}
	}
	
	//////////
	
private:
	bool GetScrollbarsEnabled(bool &r_value)
	{
		MCAndroidObjectRemoteCall(m_view, "getScrollingEnabled", "b", &r_value);
		return true;
	}
	
	bool SetScrollbarsEnabled(bool p_value)
	{
		MCAndroidObjectRemoteCall(m_view, "setScrollingEnabled", "vb", nil, p_value);
		return true;
	}
	
	bool GetUrl(char *&r_utf8_string)
	{
		char *t_url;
		t_url = nil;
		
		MCAndroidObjectRemoteCall(m_view, "getUrl", "s", &t_url);
		if (t_url == nil)
			return false;
			
		r_utf8_string = t_url;
		return true;
	}
	
	bool GetUserAgent(char *&r_useragent)
	{
		char *t_useragent;
		t_useragent = nil;
		MCAndroidObjectRemoteCall(m_view, "getUserAgent", "s", &t_useragent);

		if (t_useragent == nil)
			return false;
			
		r_useragent = t_useragent;
		return true;
	}
	
	bool SetUserAgent(const char *p_useragent)
	{
		MCAndroidObjectRemoteCall(m_view, "setUserAgent", "vs", nil, p_useragent);
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

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result)
{
    char *t_tag = nil;
    /* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, tag, t_tag);
	
	char *t_result = nil;
	/* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, result, t_result);
	
	MCBrowser *t_browser;
	if (MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->SetJavaScriptResult(t_tag, t_result);
    
    MCCStringFree(t_tag);
    MCCStringFree(t_result);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doStartedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doStartedLoading(JNIEnv *env, jobject object, jstring url)
{
    char *t_url;
    t_url = nil;
    /* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, url, t_url);
    
	MCBrowser *t_browser;
	if (MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadBegin(false, t_url);

	MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doFinishedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doFinishedLoading(JNIEnv *env, jobject object, jstring url)
{
    char *t_url;
    t_url = nil;
    /* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, url, t_url);
    
	MCBrowser *t_browser;
	if (MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadComplete(false, t_url);

	MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_libraries_LibBrowserWebView_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error)
{
    char *t_url;
    t_url = nil;
    /* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, url, t_url);
    
    char *t_error;
    t_error = nil;
    /* UNCHECKED */ MCBrowserJavaStringToUtf8String(env, error, t_error);
    
	MCBrowser *t_browser;
	if (MCBrowserFindWithJavaView(env, object, t_browser))
		((MCAndroidWebViewBrowser*)t_browser)->OnDocumentLoadFailed(false, t_url, t_error);

	MCCStringFree(t_url);
	MCCStringFree(t_error);
}

////////////////////////////////////////////////////////////////////////////////

class MCAndroidWebViewBrowserFactory : public MCBrowserFactory
{
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
	{
		MCAndroidWebViewBrowser *t_browser;
		t_browser = new MCAndroidWebViewBrowser();
		
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
	MCBrowserFactory *t_factory = new MCAndroidWebViewBrowserFactory();
	
	if (t_factory == nil)
		return false;
		
	r_factory = (MCBrowserFactoryRef)t_factory;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
