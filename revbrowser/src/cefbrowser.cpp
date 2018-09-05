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

#include <revolution/external.h>

#include "core.h"

#include "cefbrowser.h"
#include "cefbrowser_msg.h"

#include <include/cef_app.h>

#include <list>
#include <set>

#if defined(TARGET_PLATFORM_POSIX)
#include "signal_restore_posix.h"
#include <sys/time.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// String conversion

bool MCCefStringToUtf8String(const CefString &p_cef_string, char *&r_u8_string)
{
	if (p_cef_string.empty())
		return MCCStringClone("", r_u8_string);

	return MCCStringFromUnicode(p_cef_string.c_str(), r_u8_string);
}

bool MCCefStringFromUtf8String(const char *p_u8_string, cef_string_t *r_cef_string)
{
	return 0 != cef_string_from_utf8(p_u8_string, MCCStringLength(p_u8_string), r_cef_string);
}

bool MCCefStringFromUtf8String(const char *p_u8_string, CefString &r_cef_string)
{
	return MCCefStringFromUtf8String(p_u8_string, r_cef_string.GetWritableStruct());
}

bool MCCefStringToUInt(const CefString &p_string, uint32_t &r_int)
{
	char_t * t_tmp_string;
	t_tmp_string = nil;
	
	uint32_t t_int;
	
	bool t_success;
	t_success = MCCefStringToUtf8String(p_string, t_tmp_string);
	if (t_success)
		t_success = MCCStringToCardinal(t_tmp_string, t_int);
	
	if (t_tmp_string != nil)
		MCCStringFree(t_tmp_string);
	
	if (t_success)
		r_int = t_int;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static const char *s_auth_scheme_strings[] =
{
	"basic",
	"digest",
	"ntlm",
	"negotiate",
	"spdyproxy",
	
	nil
};

bool MCCefAuthSchemeFromCefString(const CefString &p_string, MCCefAuthScheme &r_scheme)
{
	
	for (uint32_t i = 0; s_auth_scheme_strings[i] != nil; i++)
	{
		if (p_string == s_auth_scheme_strings[i])
		{
			r_scheme = (MCCefAuthScheme)i;
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// Cef initialization / shutdown handling

static bool s_cef_initialised = false;
static bool s_cefbrowser_initialised = false;

static uint32_t s_instance_count = 0;
static MCRunloopActionRef s_runloop_action = nil;

void MCCefBrowserExternalInit(void)
{
	// set up static vars
	s_cef_initialised = false;
	s_cefbrowser_initialised = false;
	s_instance_count = 0;
	s_runloop_action = nil;
}

void MCCefBrowserRunloopAction(void *p_context)
{
	CefDoMessageLoopWork();
}

extern "C" int initialise_weak_link_cef(void);

#if defined(WIN32)
static const char *kCefProcessName = "revbrowser-cefprocess.exe";
static const char *kCefPathSeparatorStr = "\\";
static const char kCefPathSeparator = '\\';
#else
static const char *kCefProcessName = "revbrowser-cefprocess";
static const char *kCefPathSeparatorStr = "/";
static const char kCefPathSeparator = '/';
#endif

static bool __MCCefGetLibraryPath(char*& r_path)
{
    int t_retval = EXTERNAL_SUCCESS;

    void *t_module = nullptr;
    if (t_module == nullptr)
    {
        LoadModuleByName("./CEF/libcef", &t_module, &t_retval);
        if (t_retval != EXTERNAL_SUCCESS)
            t_module = nullptr;
    }
#if defined(WIN32) || defined(TARGET_PLATFORM_LINUX)
    if (t_module == nullptr)
    {
        LoadModuleByName("./Externals/CEF/libcef", &t_module, &t_retval);
        if (t_retval != EXTERNAL_SUCCESS)
            t_module = nullptr;
    }
#endif

    char *t_module_path =
        t_module != nullptr ? CopyNativePathOfModule(t_module, &t_retval)
        : nullptr;
    if (t_retval != EXTERNAL_SUCCESS)
        t_module_path = nullptr;

    UnloadModule(t_module, &t_retval);

    if (t_module_path == nullptr)
    {
        return false;
    }

    char *t_last_sep =
        strrchr(t_module_path,
            kCefPathSeparator);

    if (t_last_sep == nullptr)
    {
        free(t_module_path);
        return false;
    }

    *t_last_sep = '\0';

    r_path = t_module_path;

    return true;
}

static bool __MCCefAppendPath(const char *p_base, const char *p_path, char *&r_path)
{
    if (p_base == nil)
        return MCCStringClone(p_path, r_path);
    else if (MCCStringEndsWith(p_base, kCefPathSeparatorStr))
        return MCCStringFormat(r_path, "%s%s", p_base, p_path);
    else
        return MCCStringFormat(r_path, "%s%s%s", p_base, kCefPathSeparatorStr, p_path);
}

static bool __MCCefBuildPath(const char *p_library_path,
    const char *p_suffix,
    cef_string_t* r_string)
{
    char *t_process_path = nullptr;
    if (!__MCCefAppendPath(p_library_path,
        p_suffix,
        t_process_path) ||
        !MCCefStringFromUtf8String(t_process_path,
            r_string))
    {
        free(t_process_path);
        return false;
    }

    return true;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Initialisation of the CEF library
bool MCCefInitialise(void)
{
	if (s_cef_initialised)
		return true;

    ////////

    char *t_library_path = nullptr;
    if (!__MCCefGetLibraryPath(t_library_path))
        return false;

    ////////

	CefMainArgs t_args;
	CefSettings t_settings;
	t_settings.multi_threaded_message_loop = false;
	t_settings.command_line_args_disabled = true;
	t_settings.no_sandbox = true;
	t_settings.log_severity = LOGSEVERITY_VERBOSE;

    bool t_success = true;
    if (t_success)
        t_success = __MCCefBuildPath(t_library_path, kCefProcessName, &t_settings.browser_subprocess_path);
    if (t_success)
        t_success = __MCCefBuildPath(t_library_path, "locales", &t_settings.locales_dir_path);
    if (t_success)
        t_success = __MCCefBuildPath(t_library_path, "", &t_settings.resources_dir_path);


	CefRefPtr<CefApp> t_app = nil;

	if (t_success)
		t_success = -1 == CefExecuteProcess(t_args, t_app, nil);

	if (t_success)
	{
#if defined(TARGET_PLATFORM_POSIX)
		struct itimerval t_old_timer, t_new_timer;
		memset(&t_new_timer, 0, sizeof(t_new_timer));
		
		setitimer(ITIMER_REAL, &t_new_timer, &t_old_timer);
		BackupSignalHandlers();
#endif
		t_success = CefInitialize(t_args, t_settings, t_app, nil);
		
#if defined(TARGET_PLATFORM_POSIX)
		RestoreSignalHandlers();
		setitimer(ITIMER_REAL, &t_old_timer, nil);
#endif
	}

	s_cef_initialised = t_success;
	
	return s_cef_initialised;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Initialise CEF & install the runloop action
bool MCCefBrowserInitialise(void)
{
	if (s_cefbrowser_initialised)
		return true;
	
	bool t_success;
	t_success = true;

    if (t_success)
    {
        t_success = initialise_weak_link_cef();
    }

	if (t_success)
		t_success = MCCefInitialise();

	if (t_success)
	{
		int t_result;
		AddRunloopAction(MCCefBrowserRunloopAction, nil, &s_runloop_action, &t_result);
	}

	s_cefbrowser_initialised = t_success;

	return s_cefbrowser_initialised;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Shutdown the CEF library
void MCCefFinalise(void)
{
	if (!s_cef_initialised)
		return;

    //CefShutdown();

	s_cef_initialised = false;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Shut down the browser
void MCCefBrowserFinalise(void)
{
	if (!s_cefbrowser_initialised)
		return;
	
	// IM-2014-03-13: [[ revBrowserCEF ]] CEF library can't be cleanly shutdown and restarted - don't call finalise
	// MCCefFinalise();
	
	int t_result;
	/* UNCHECKED */ RemoveRunloopAction(s_runloop_action, &t_result);
	s_runloop_action = nil;
	
	s_cefbrowser_initialised = false;
}

void MCCefIncrementInstanceCount(void)
{
	if (s_instance_count == 0)
		/* UNCHECKED */ MCCefBrowserInitialise();

	s_instance_count++;
}

void MCCefDecrementInstanceCount(void)
{
	if (s_instance_count == 0)
		return;

	s_instance_count--;

	if (s_instance_count == 0)
		MCCefBrowserFinalise();
}

////////////////////////////////////////////////////////////////////////////////
// Utility class to hold a response from a request to the render process

class MCCefMessageResult
{
private:
	bool m_have_result;
	bool m_success;

	CefValueType m_result_type;

	CefString m_result_string;
	int m_result_int;

public:
	void SetResult(bool p_success, const CefString &p_result)
	{
		if (HaveResult())
			Clear();

		m_success = p_success;
		m_result_string = p_result;
		m_result_type = VTYPE_STRING;

		m_have_result = true;
	}

	void SetResult(bool p_success, int p_result)
	{
		if (HaveResult())
			Clear();

		m_success = p_success;
		m_result_int = p_result;
		m_result_type = VTYPE_INT;

		m_have_result = true;
	}

	bool HaveResult() { return m_have_result; }

	bool GetResult(CefString &r_result) const
	{
		if (!m_success)
			return false;

		if (m_result_type == VTYPE_STRING)
		{
			r_result = m_result_string;
			return true;
		}

		// convert to string

		bool t_converted;
		t_converted = false;

		switch (m_result_type)
		{
		case VTYPE_INT:
			{
				char *t_tmp;
				t_tmp = nil;

				t_converted = MCCStringFormat(t_tmp, "%d", m_result_int) &&
					MCCefStringFromUtf8String(t_tmp, r_result);

				if (t_tmp != nil)
					MCCStringFree(t_tmp);
			}
			break;
			case VTYPE_NULL:
			case VTYPE_BOOL:
			case VTYPE_DOUBLE:
			case VTYPE_BINARY:
			case VTYPE_STRING:
			case VTYPE_DICTIONARY:
			case VTYPE_LIST:
			case VTYPE_INVALID:
				break;
		}

		return t_converted;
	}

	bool GetResult(int &r_result) const
	{
		if (!m_success)
			return false;

		if (m_result_type == VTYPE_INT)
		{
			r_result = m_result_int;
			return true;
		}

		// convert to int
		return false;
	}

	void Clear()
	{
		m_have_result = false;
		m_result_string.ClearAndFree();
	}
};

////////////////////////////////////////////////////////////////////////////////
// Browser client - receives callback messages from the browser

struct MCCefErrorInfo
{
	CefString url;
	CefString error_message;
};

class MCCefBrowserClient : public CefClient, CefLifeSpanHandler, CefRequestHandler, CefDownloadHandler, CefLoadHandler, CefContextMenuHandler
{
private:
	int m_browser_id;

	MCCefBrowserBase *m_owner;

	MCCefMessageResult m_message_result;
	std::map<int64_t, MCCefErrorInfo> m_load_error_frames;
	
	// IM-2014-05-06: [[ Bug 12384 ]] Set of URLs for which callback messages will not be sent 
	std::set<CefString> m_ignore_urls;

	CefString m_last_request_url;

	// Error handling - we need to keep track of url that failed to load in a
	// frame so we can send the correct url in onLoadEnd()
	void AddLoadErrorFrame(int64_t p_id, const CefString &p_url, const CefString &p_error_msg)
	{
		m_load_error_frames[p_id].url = p_url;
		m_load_error_frames[p_id].error_message = p_error_msg;
	}

	bool RemoveLoadErrorFrame(int64_t p_id, CefString &r_error_url, CefString &r_error_msg)
	{
		std::map<int64_t, MCCefErrorInfo>::iterator t_iter;
		t_iter = m_load_error_frames.find(p_id);

		if (t_iter == m_load_error_frames.end())
			return false;

		r_error_url = t_iter->second.url;
		r_error_msg = t_iter->second.error_message;
		m_load_error_frames.erase(t_iter);

		return true;
	}

public:
	MCCefBrowserClient(MCCefBrowserBase *p_owner)
	{
		m_owner = p_owner;
		m_browser_id = 0;
	}

	// IM-2014-07-21: [[ Bug 12296 ]] Method to allow owner to notify client of its deletion
	void OnOwnerClosed(void)
	{
		m_owner = nil;
	}

	// Tell browser which callback interfaces we implement
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() OVERRIDE { return this; }

	void AddIgnoreUrl(const CefString &p_url)
	{
		m_ignore_urls.insert(p_url);
	}

	void RemoveIgnoreUrl(const CefString &p_url)
	{
		m_ignore_urls.erase(p_url);
	}

	// IM-2014-05-06: [[ Bug 12384 ]] Test if callback should be sent for URL
	bool IgnoreUrl(const CefString &p_url)
	{
		std::set<CefString>::iterator t_iter;
		t_iter = m_ignore_urls.find(p_url);

		return t_iter != m_ignore_urls.end();
	}

	MCCefMessageResult &GetMessageResult()
	{
		return m_message_result;
	}

	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> p_browser, CefProcessId p_source_process, CefRefPtr<CefProcessMessage> p_message) OVERRIDE
	{
		CefString t_message_name;
		t_message_name = p_message->GetName();

		if (t_message_name == MC_CEFMSG_RESULT)
		{
			// When we get a result message, the result is stored in the message result object

			CefRefPtr<CefListValue> t_args;
			t_args = p_message->GetArgumentList();

			switch (t_args->GetType(1))
			{
			case VTYPE_STRING:
				m_message_result.SetResult(t_args->GetBool(0), t_args->GetString(1));
				break;

			case VTYPE_INT:
				m_message_result.SetResult(t_args->GetBool(0), t_args->GetInt(1));
				break;

			default:
				MCAssert(false);
			}

			return true;
		}
		else if (t_message_name == MC_CEFMSG_JS_HANDLER)
		{
			// Handle JavaScript call to LiveCode handler

			CefRefPtr<CefListValue> t_args;
			t_args = p_message->GetArgumentList();

			char *t_handler;
			t_handler = nil;

			char **t_arg_strings;
			t_arg_strings = nil;

			bool t_success;
			t_success = true;
			
			// IM-2014-07-21: [[ Bug 12296 ]] Don't proceed with callback if browser has been closed
			if (t_success)
				t_success = nil != m_owner;
			
			if (t_success)
				t_success = MCCefStringToUtf8String(t_args->GetString(0), t_handler);

			uint32_t t_arg_count;
			t_arg_count = 0;

			// Convert message args to strings and pass to handler parameters
			if (t_success)
			{
				t_arg_count = t_args->GetSize() - 1;
				t_success = MCMemoryNewArray(t_arg_count, t_arg_strings);
			}

			for (uint32_t i = 0; i < t_arg_count; i++)
				t_success = MCCefStringToUtf8String(t_args->GetString(i + 1), t_arg_strings[i]);

			bool t_cancel;
			if (t_success)
				CB_Custom(m_owner->GetInst(), t_handler, t_arg_strings, t_arg_count, &t_cancel);

			if (t_handler)
				MCCStringFree(t_handler);

			if (t_arg_strings != nil)
			{
				for (uint32_t i = 0; i < t_arg_count; i++)
					if (t_arg_strings[i] != nil)
						MCCStringFree(t_arg_strings[i]);
				MCMemoryDeleteArray(t_arg_strings);
			}

			return true;
		}

		return CefClient::OnProcessMessageReceived(p_browser, p_source_process, p_message);
	}

	// CefLifeSpanHandler interface
	// Called on UI thread
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> p_browser) OVERRIDE
	{
		// Keep track of first created browser instance. Any others belong to pop-ups
		if (m_browser_id == 0)
		{
			m_browser_id = p_browser->GetIdentifier();
			// IM-2014-07-21: [[ Bug 12296 ]] Check owner has not been closed before invoking callback
			if (m_owner != nil)
				m_owner->OnCefBrowserCreated(p_browser);
		}

		MCCefIncrementInstanceCount();
	}

	virtual bool DoClose(CefRefPtr<CefBrowser> p_browser) OVERRIDE
	{
		// We handle browser closing here to stop CEF sending WM_CLOSE to the stack window
		if (p_browser->GetIdentifier() == m_browser_id)
		{
			// Close the browser window
			MCCefPlatformCloseBrowserWindow(p_browser);

			// return true to prevent default handling
			return true;
		}

		return CefLifeSpanHandler::DoClose(p_browser);
	}

	virtual void OnBeforeClose(CefRefPtr<CefBrowser> p_browser) OVERRIDE
	{
		if (m_owner != nil)
			m_owner->OnCefBrowserClosed(p_browser);
		
		MCCefDecrementInstanceCount();
	}

	virtual bool OnBeforePopup(
		CefRefPtr<CefBrowser> p_browser,
		CefRefPtr<CefFrame> p_frame,
		const CefString& p_target_url,
		const CefString& p_target_frame_name,
		CefLifeSpanHandler::WindowOpenDisposition p_target_disposition,
		bool p_user_gesture,
		const CefPopupFeatures& p_popup_features,
		CefWindowInfo& p_window_info,
		CefRefPtr<CefClient>& p_client,
		CefBrowserSettings& p_settings,
		bool* p_no_javascript_access ) OVERRIDE
	{
		// returning true here will prevent popup windows from opening
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return true;
		
		return !m_owner->GetNewWindow();
	}

	// CefRequestHandler interface

	// Called on UI thread
	virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefRequest> p_request, bool p_user_gesture, bool p_is_redirect) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return true;
		
		bool t_cancel;
		t_cancel = false;

		CefString t_url;
		t_url = p_request->GetURL();

		if (IgnoreUrl(t_url))
			return false;

		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);

		if (p_frame->IsMain())
		{
			CB_NavigateRequest(m_owner->GetInst(), t_url_str, &t_cancel);
		}
		else
		{
			CB_NavigateFrameRequest(m_owner->GetInst(), t_url_str, &t_cancel);
		}

		if (t_url_str != nil)
			MCCStringFree(t_url_str);

		return t_cancel;
	}

	virtual CefRequestHandler::ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefRequest> p_request, CefRefPtr<CefRequestCallback>) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return RV_CANCEL;
		
		CefString t_url;
		t_url = p_request->GetURL();

		if (IgnoreUrl(t_url))
			return RV_CONTINUE;

		m_last_request_url = t_url;
		
		CefString t_user_agent;
		if (!m_owner->GetUserAgent(t_user_agent))
			return RV_CONTINUE;

		// modify request headers to set user agent

		CefRequest::HeaderMap t_headers;
		p_request->GetHeaderMap(t_headers);

		CefString t_key = "User-Agent";
		// clear any current value
		t_headers.erase(t_key);

		// add new value
		t_headers.insert(std::pair<CefString, CefString>(t_key, t_user_agent));

		p_request->SetHeaderMap(t_headers);

		return RV_CONTINUE;
	}

	// IM-2014-04-28: [[ CefBrowser ]] Use platform-specific method to get credentials when requested
	virtual bool GetAuthCredentials( CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, bool p_is_proxy, const CefString &p_host, int p_port, const CefString &p_realm, const CefString &p_method, CefRefPtr<CefAuthCallback> p_callback) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return true;
		
		bool t_success;
		t_success = true;
		
		CefString t_user, t_pass;
		MCCefAuthScheme t_auth;
		
		if (t_success)
			t_success = MCCefAuthSchemeFromCefString(p_method, t_auth);
		
		if (t_success)
			t_success = m_owner->PlatformGetAuthCredentials(p_is_proxy, m_last_request_url, p_realm, t_auth, t_user, t_pass);
		
		if (t_success)
			t_success = !t_user.empty() && !t_pass.empty();
		
		if (t_success)
			p_callback->Continue(t_user, t_pass);
		
		return t_success;
	}

	// CefDownloadHandler interface
	// Methods called on UI thread

	virtual void OnBeforeDownload(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefDownloadItem> p_item, const CefString & p_suggested_name, CefRefPtr<CefBeforeDownloadCallback> p_callback) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		bool t_cancel;
		t_cancel = false;

		CefString t_url;
		t_url = p_item->GetURL();

		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);

		CB_DownloadRequest(m_owner->GetInst(), t_url_str, &t_cancel);

		if (t_url_str != nil)
			MCCStringFree(t_url_str);

		CefString t_download_path;

		if (!t_cancel)
			p_callback->Continue(t_download_path, false);
	}

	// CefLoadHandler interface
	// Methods called on UI thread or render process main thread

	virtual void OnLoadStart(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, cef_transition_type_t p_transition_type) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		CefString t_url;
		t_url = p_frame->GetURL();

		if (IgnoreUrl(t_url))
			return;

		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);

		if (p_frame->IsMain())
			CB_NavigateComplete(m_owner->GetInst(), t_url_str);
		else
			CB_NavigateFrameComplete(m_owner->GetInst(), t_url_str);

		if (t_url_str != nil)
			MCCStringFree(t_url_str);
	}

	virtual void OnLoadEnd(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, int p_http_status_code) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		CefString t_url, t_error;

		bool t_is_error;
		t_is_error = RemoveLoadErrorFrame(p_frame->GetIdentifier(), t_url, t_error);

		/* TODO - Load error handling */
		// For now we don't send a browser load error message - instead make sure documentComplete is sent with the correct url
		if (!t_is_error)
			t_url = p_frame->GetURL();

		if (IgnoreUrl(t_url))
			return CefLoadHandler::OnLoadEnd(p_browser, p_frame, p_http_status_code);

		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);

		if (t_is_error)
		{
			char *t_err_str;
			t_err_str = nil;
            /* UNCHECKED */ MCCefStringToUtf8String(t_error, t_err_str);
			
			if (p_frame->IsMain())
				CB_DocumentFailed(m_owner->GetInst(), t_url_str, t_err_str);
			else
				CB_DocumentFrameFailed(m_owner->GetInst(), t_url_str, t_err_str);
			
			if (t_err_str != nil)
				MCCStringFree(t_err_str);
		}
		else
		{
			if (p_frame->IsMain())
				CB_DocumentComplete(m_owner->GetInst(), t_url_str);
			else
				CB_DocumentFrameComplete(m_owner->GetInst(), t_url_str);
		}
		
		if (t_url_str != nil)
			MCCStringFree(t_url_str);
	}

	virtual void OnLoadError(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefLoadHandler::ErrorCode p_error_code, const CefString &p_error_text, const CefString &p_failed_url) OVERRIDE
	{
		if (IgnoreUrl(p_failed_url))
			return;

		AddLoadErrorFrame(p_frame->GetIdentifier(), p_failed_url, p_error_text);
	}

	// ContextMenuHandler interface
	// Methods called on UI thread

	virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefContextMenuParams> p_params, CefRefPtr<CefMenuModel> p_model) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		// clearing the menu model prevents the context menu from opening
		if (!m_owner->GetContextMenu())
			p_model->Clear();
	}

	IMPLEMENT_REFCOUNTING(MCCefBrowserClient);
};

bool MCCefBrowserBase::Initialize()
{
	// create client and browser
	m_client = new (nothrow) MCCefBrowserClient(this);

	CefWindowInfo t_window_info;
	CefBrowserSettings t_settings;

	// IM-2014-05-06: [[ Bug 12384 ]] Browser must be created with non-empty URL or setting
	// htmltext will not work
	CefString t_url("dummy:");

	// IM-2014-05-06: [[ Bug 12384 ]] Prevent callback messages for dummy URL
	m_client->AddIgnoreUrl(t_url);
	PlatformConfigureWindow(t_window_info);
	CefBrowserHost::CreateBrowserSync(t_window_info, m_client.get(), t_url, t_settings, NULL);

	return m_browser != nil;
}


MCCefBrowserBase::MCCefBrowserBase()
{
	MCCefIncrementInstanceCount();

	m_browser = nil;
	m_client = nil;
	m_instance_id = 0;

	m_send_advanced_messages = false;
	m_show_context_menu = false;
	m_allow_new_window = false;
}

MCCefBrowserBase::~MCCefBrowserBase(void)
{
	if (m_browser != nil)
		m_browser->GetHost()->CloseBrowser(false);

	// IM-2014-07-21: [[ Bug 12296 ]] Notify client of browser being closed
	if (m_client)
		m_client->OnOwnerClosed();
	
	m_browser = nil;
	m_client = nil;

	MCCefDecrementInstanceCount();
}

////////////////////////////////////////////////////////////////////////////////

CWebBrowserBase *MCCefBrowserInstantiate(int p_window_id)
{
	// IM-2014-03-18: [[ revBrowserCEF ]] Make sure cef library is loaded before trying to create browser
	if (!MCCefBrowserInitialise())
		return nil;

	MCCefBrowserBase *t_browser;
	if (!MCCefPlatformCreateBrowser(p_window_id, t_browser))
		return nil;

	if (!t_browser->Initialize())
	{
		delete t_browser;
		return nil;
	}

	return t_browser;
}

////////////////////////////////////////////////////////////////////////////////

void MCCefBrowserBase::OnCefBrowserCreated(CefRefPtr<CefBrowser> p_browser)
{
	if (m_browser == nil)
		m_browser = p_browser;
}

void MCCefBrowserBase::OnCefBrowserClosed(CefRefPtr<CefBrowser> p_browser)
{
}

////////////////////////////////////////////////////////////////////////////////

CefRefPtr<CefBrowser> MCCefBrowserBase::GetCefBrowser(void)
{
	return m_browser;
}

bool MCCefBrowserBase::GetUserAgent(CefString &r_user_agent)
{
	if (m_user_agent.empty())
		return false;

	r_user_agent = m_user_agent;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

// wait for result from async request
void MCCefBrowserBase::WaitOnResult()
{
	MCCefMessageResult &t_result = m_client->GetMessageResult();

	int t_success;
	t_success = EXTERNAL_SUCCESS;
	while ((t_success == EXTERNAL_SUCCESS )&& !t_result.HaveResult())
		RunloopWait(&t_success);
}

bool MCCefBrowserBase::WaitOnResultString(CefString &r_result)
{
	WaitOnResult();

	return m_client->GetMessageResult().GetResult(r_result);
}

// send the process message and wait for the result
bool MCCefBrowserBase::GetMessageResult(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, const MCCefMessageResult *&r_result)
{
	bool t_success;
	t_success = true;

	MCCefMessageResult &t_result = m_client->GetMessageResult();

	t_result.Clear();

	if (t_success)
		t_success = m_browser->SendProcessMessage(p_target, p_message);

	if (t_success)
	{
		WaitOnResult();
		r_result = &t_result;
	}

	return t_success;
}

bool MCCefBrowserBase::GetMessageResultString(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, CefString &r_result)
{
	bool t_success;
	t_success = true;

	const MCCefMessageResult *t_result;
	t_result = nil;
	
	t_success = GetMessageResult(p_target, p_message, t_result);

	if (t_success)
		t_success = t_result->GetResult(r_result);

	return t_success;
}

// evaluate the javascript and return the result
bool MCCefBrowserBase::EvalJavaScript(const CefString &p_script, const MCCefMessageResult *&r_result)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_EXECUTE_SCRIPT);

	t_success = t_message != nil;

	if (t_success)
	{
		CefRefPtr<CefListValue> t_args;
		t_args = t_message->GetArgumentList();

		t_success = t_args->SetString(0, p_script);
	}

	if (t_success)
		t_success = GetMessageResult(PID_RENDERER, t_message, r_result);

	return t_success;
}

bool MCCefBrowserBase::EvalJavaScript(const CefString &p_script, CefString &r_value)
{
	bool t_success;
	t_success = true;

	const MCCefMessageResult *t_result;
	t_result = nil;

	t_success = EvalJavaScript(p_script, t_result);

	if (t_success)
		t_success = t_result->GetResult(r_value);

	return t_success;
}

bool MCCefBrowserBase::EvalJavaScript(const CefString &p_script, int &r_value)
{
	bool t_success;
	t_success = true;

	const MCCefMessageResult *t_result;
	t_result = nil;

	t_success = EvalJavaScript(p_script, t_result);

	if (t_success)
		t_success = t_result->GetResult(r_value);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// Browser Properties

bool MCCefBrowserBase::GetVisible(void)
{
	bool t_visible;
	/* UNCHECKED */ PlatformGetVisible(t_visible);

	return t_visible;
}

void MCCefBrowserBase::SetVisible(bool p_visible)
{
	/* UNCHECKED */ PlatformSetVisible(p_visible);
}

//////////

char *MCCefBrowserBase::GetBrowser(void)
{
	char *t_browser;
	t_browser = nil;

	/* UNCHECKED */ MCCStringClone("Chromium", t_browser);

	return t_browser;
}

void MCCefBrowserBase::SetBrowser(const char *p_type)
{
}

//////////

/* TODO - implement advanced event messages */
bool MCCefBrowserBase::GetMessages(void)
{
	return m_send_advanced_messages;
}

void MCCefBrowserBase::SetMessages(bool p_value)
{
	m_send_advanced_messages = p_value;
}

//////////

char *MCCefBrowserBase::GetSelectedText(void)
{
	bool t_success;
	t_success = true;

	// Need to call out to the browser subprocess
	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_GET_SELECTED_TEXT);

	CefString t_return_value;
	t_success = GetMessageResultString(PID_RENDERER, t_message, t_return_value);

	char *t_return_value_str;
	t_return_value_str = nil;

	if (t_success)
		t_success = MCCefStringToUtf8String(t_return_value, t_return_value_str);

	return t_return_value_str;
}

void MCCefBrowserBase::SetSelectedText(const char *p_text)
{
	/* TODO - IMPLEMENT */
}

bool MCCefBrowserBase::GetOffline(void)
{
	/* TODO - IMPLEMENT */
	return false;
}

void MCCefBrowserBase::SetOffline(bool p_value)
{
	/* TODO - IMPLEMENT */
}

bool MCCefBrowserBase::GetContextMenu(void)
{
	return m_show_context_menu;
}

void MCCefBrowserBase::SetContextMenu(bool p_menu)
{
	m_show_context_menu = p_menu;
}

bool MCCefBrowserBase::GetNewWindow(void)
{
	return m_allow_new_window;
}

void MCCefBrowserBase::SetNewWindow(bool p_new_window)
{
	m_allow_new_window = p_new_window;
}

class MCStringVisitor : public CefStringVisitor
{
private:
	MCCefMessageResult *m_result;

public:
	MCStringVisitor(MCCefMessageResult &p_result)
	{
		m_result = &p_result;
	}

	void Visit(const CefString &p_string) OVERRIDE
	{
		m_result->SetResult(true, p_string);
	}

	IMPLEMENT_REFCOUNTING(MCStringVisitor);
};

char *MCCefBrowserBase::GetSource(void)
{
	MCCefMessageResult &t_result = m_client->GetMessageResult();
	t_result.Clear();

	CefRefPtr<CefStringVisitor> t_visitor;
	t_visitor = new (nothrow) MCStringVisitor(t_result);

	m_browser->GetMainFrame()->GetSource(t_visitor);

	bool t_success;
	t_success = true;

	CefString t_src;

	t_success = WaitOnResultString(t_src);

	char *t_src_str;
	t_src_str = nil;

	if (t_success)
		t_success = MCCefStringToUtf8String(t_src, t_src_str);

	return t_src_str;
}

void MCCefBrowserBase::SetSource(const char *p_source)
{
	// IM-2014-06-25: [[ Bug 12701 ]] CEF will crash if given an empty source string,
	// so replace here with the source of an empty page :)
	if (p_source == nil || MCCStringLength(p_source) == 0)
		p_source = "<html><head></head><body></body></html>";
	
	CefString t_source;
	/* UNCHECKED */ MCCefStringFromUtf8String(p_source, t_source);

	// LoadString requires a valid url
	CefString t_url;
	t_url = "http://revbrowser_dummy_url";

	m_browser->GetMainFrame()->LoadString(t_source, t_url);
}

bool MCCefBrowserBase::GetScale(void)
{
	/* TODO - IMPLEMENT */
	return false;
}

void MCCefBrowserBase::SetScale(bool p_scale)
{
	/* TODO - IMPLEMENT */
}

bool MCCefBrowserBase::GetBorder(void)
{
	/* TODO - IMPLEMENT */
	return false;
}

void MCCefBrowserBase::SetBorder(bool p_border)
{
	/* TODO - IMPLEMENT */
}

// IM-2014-08-25: [[ Bug 13272 ]] Implement CEF browser scrollbar property.
bool MCCefBrowserBase::GetOverflowHidden()
{
	// property available through JavaScript
	bool t_success;
	t_success = true;
	
	CefString t_value;
	
	t_success = EvalJavaScript("document.body.style.overflow", t_value);
	
	// assume scrollbars are visible if property fetch fails
	if (!t_success)
		return true;
	
	return t_value == L"hidden";
}

// IM-2014-08-25: [[ Bug 13272 ]] Implement CEF browser scrollbar property.
void MCCefBrowserBase::SetOverflowHidden(bool p_hidden)
{
	// property available through JavaScript
	
	bool t_success;
	t_success = true;
	
	char *t_overflow_script;
	t_overflow_script = nil;
	
	t_success = MCCStringFormat(t_overflow_script, "document.body.style.overflow = \"%s\"", p_hidden ? "hidden" : "");
	
	CefString t_return_value;
	
	if (t_success)
		t_success = EvalJavaScript(t_overflow_script, t_return_value);
	
	MCCStringFree(t_overflow_script);
}

bool MCCefBrowserBase::GetScrollbars(void)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	return !GetOverflowHidden();
}

void MCCefBrowserBase::SetScrollbars(bool p_scrollbars)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	/* UNCHECKED */ SetOverflowHidden(!p_scrollbars);
}

void MCCefBrowserBase::GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom)
{
	/* UNCHECKED */ PlatformGetRect(r_left, r_top, r_right, r_bottom);
}

void MCCefBrowserBase::SetRect(int p_left, int p_top, int p_right, int p_bottom)
{
	/* UNCHECKED */ PlatformSetRect(p_left, p_top, p_right, p_bottom);
}

int MCCefBrowserBase::GetInst(void)
{
	return m_instance_id;
}

void MCCefBrowserBase::SetInst(int p_id)
{
	m_instance_id = p_id;
}

int MCCefBrowserBase::GetVScroll(void)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	int t_int_value;
	t_int_value = 0;

	t_success = EvalJavaScript("document.body.scrollTop", t_int_value);

	return t_int_value;
}

void MCCefBrowserBase::SetVScroll(int p_vscroll_pixels)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	char *t_scroll_script;
	t_scroll_script = nil;

	t_success = MCCStringFormat(t_scroll_script, "window.scroll(%d,%d)", GetHScroll(), p_vscroll_pixels);

	CefString t_return_value;

	if (t_success)
		t_success = EvalJavaScript(t_scroll_script, t_return_value);

	MCCStringFree(t_scroll_script);
}

int MCCefBrowserBase::GetHScroll(void)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	int t_int_value;
	t_int_value = 0;

	t_success = EvalJavaScript("document.body.scrollLeft", t_int_value);

	return t_int_value;
}

void MCCefBrowserBase::SetHScroll(int p_hscroll_pixels)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	char *t_scroll_script;
	t_scroll_script = nil;

	t_success = MCCStringFormat(t_scroll_script, "window.scroll(%d,%d)", p_hscroll_pixels, GetVScroll());

	CefString t_return_value;

	if (t_success)
		t_success = EvalJavaScript(t_scroll_script, t_return_value);

	MCCStringFree(t_scroll_script);
}

uintptr_t MCCefBrowserBase::GetWindowId(void)
{
	uintptr_t t_id;
	t_id = 0;

	/* UNCHECKED */ PlatformGetWindowID(t_id);

	return t_id;
}

void MCCefBrowserBase::SetWindowId(uintptr_t p_new_id)
{
	/* TODO - IMPLEMENT */
}

char *MCCefBrowserBase::GetUserAgent(void)
{
	char *t_string;
	t_string = nil;

	/* UNCHECKED */ MCCefStringToUtf8String(m_user_agent, t_string);
	return t_string;
}

void MCCefBrowserBase::SetUserAgent(const char *p_user_agent)
{
	/* UNCHECKED */ MCCefStringFromUtf8String(p_user_agent, m_user_agent);
}

bool MCCefBrowserBase::GetBusy(void)
{
	return m_browser->IsLoading();
}

char *MCCefBrowserBase::GetURL(void)
{
	CefString t_url;
	t_url = m_browser->GetMainFrame()->GetURL();

	char *t_url_str;
	t_url_str = nil;

	/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);

	return t_url_str;
}

char *MCCefBrowserBase::GetTitle(void)
{
	// Need to call out to the browser subprocess

	bool t_success;
	t_success = true;

	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_GET_TITLE);

	CefString t_return_value;
	t_success = GetMessageResultString(PID_RENDERER, t_message, t_return_value);

	char *t_return_value_str;
	t_return_value_str = nil;

	if (t_success)
		t_success = MCCefStringToUtf8String(t_return_value, t_return_value_str);

	return t_return_value_str;
}

bool MCCefBrowserBase::GetImage(void*& r_data, int& r_length)
{
	/* TODO - IMPLEMENT */
	return false;
}

int MCCefBrowserBase::GetFormattedHeight(void)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	int t_int_value;
	t_int_value = 0;

	t_success = EvalJavaScript("document.body.scrollHeight", t_int_value);

	return t_int_value;
}

int MCCefBrowserBase::GetFormattedWidth(void)
{
	// property available through JavaScript

	bool t_success;
	t_success = true;

	int t_int_value;
	t_int_value = 0;

	t_success = EvalJavaScript("document.body.scrollWidth", t_int_value);

	return t_int_value;
}

void MCCefBrowserBase::GetFormattedRect(int& r_left, int& r_top, int& r_right, int& r_bottom)
{
	int t_browser_left, t_browser_top, t_browser_right, t_browser_bottom;
	GetRect(t_browser_left, t_browser_top, t_browser_right, t_browser_bottom);

	r_left = t_browser_left - GetHScroll();
	r_top = t_browser_top - GetVScroll();
	r_right = r_left + GetFormattedWidth();
	r_bottom = r_top + GetFormattedHeight();
}

////////////////////////////////////////////////////////////////////////////////

// Browser Actions

char *MCCefBrowserBase::ExecuteScript(const char *p_javascript_string)
{
	bool t_success;
	t_success = true;

	CefString t_script;
	t_success = MCCefStringFromUtf8String(p_javascript_string, t_script);

	CefString t_return_value;
	if (t_success)
		t_success = EvalJavaScript(t_script, t_return_value);

	char *t_return_value_str;
	t_return_value_str = nil;

	if (t_success)
		t_success = MCCefStringToUtf8String(t_return_value, t_return_value_str);

	return t_return_value_str;
}

char *MCCefBrowserBase::CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count)
{
	bool t_success;
	t_success = true;

	CefString t_function_name;
	t_success = MCCefStringFromUtf8String(p_function_name, t_function_name);

	CefRefPtr<CefListValue> t_args_list;
	if (t_success)
	{
		t_args_list = CefListValue::Create();
		t_success = t_args_list != nil;
	}

	for (uint32_t i = 0; t_success && i < p_argument_count; i++)
	{
		CefString t_arg_string;
		t_success = MCCefStringFromUtf8String(p_arguments[i], t_arg_string);
		if (t_success)
			t_success = t_args_list->SetString(i, t_arg_string);
	}

	CefRefPtr<CefProcessMessage> t_message;
	if (t_success)
	{
		t_message = CefProcessMessage::Create(MC_CEFMSG_CALL_SCRIPT);
		t_success = t_message != nil;
	}

	CefRefPtr<CefListValue> t_args;
	if (t_success)
	{
		t_args = t_message->GetArgumentList();
		t_success = t_args->SetString(0, t_function_name) &&
			t_args->SetList(1, t_args_list);
	}

	CefString t_return_value;
	if (t_success)
		t_success = GetMessageResultString(PID_RENDERER, t_message, t_return_value);

	char *t_return_value_str;
	t_return_value_str = nil;

	if (t_success)
		t_success = MCCefStringToUtf8String(t_return_value, t_return_value_str);

	return t_return_value_str;
}

bool MCCefBrowserBase::FindString(const char *p_string, bool p_search_up)
{
	CefString t_searchstring;
	/* UNCHECKED */ MCCefStringFromUtf8String(p_string, t_searchstring);

	int t_identifier;
	t_identifier = 0;

	bool t_forward;
	t_forward = !p_search_up;

	bool t_match_case;
	t_match_case = false;

	bool t_find_next;
	t_find_next = false;

	bool t_found;
	/* TODO - get result of search */
	t_found = true;

	m_browser->GetHost()->Find(t_identifier, t_searchstring, t_forward, t_match_case, t_find_next);

	return t_found;
}

void MCCefBrowserBase::GoURL(const char *p_url, const char *p_target_frame)
{
	CefRefPtr<CefBrowser> t_browser = GetCefBrowser();
	if (t_browser == nil)
		return;

	CefRefPtr<CefFrame> t_frame;
	if (p_target_frame == nil)
		t_frame = t_browser->GetMainFrame();
	else
	{
		CefString t_frame_name;
		if (MCCefStringFromUtf8String(p_target_frame, t_frame_name))
			t_frame = t_browser->GetFrame(t_frame_name);
	}

	if (t_frame == nil)
		return;

	CefString t_url;
	if (MCCefStringFromUtf8String(p_url, t_url))
		t_frame->LoadURL(t_url);
}

void MCCefBrowserBase::GoBack(void)
{
	m_browser->GoBack();
}

void MCCefBrowserBase::GoForward(void)
{
	m_browser->GoForward();
}

void MCCefBrowserBase::Focus(void)
{
	/* TODO - IMPLEMENT */
}

void MCCefBrowserBase::Unfocus(void)
{
	/* TODO - IMPLEMENT */
}

void MCCefBrowserBase::Refresh(void)
{
	m_browser->Reload();
}

void MCCefBrowserBase::Stop(void)
{
	m_browser->StopLoad();
}

void MCCefBrowserBase::Print(void)
{
	m_browser->GetHost()->Print();
}

void MCCefBrowserBase::Redraw(void)
{
	/* TODO - IMPLEMENT */
}

void MCCefBrowserBase::MakeTextBigger(void)
{
	/* TODO - IMPLEMENT */
}

void MCCefBrowserBase::MakeTextSmaller(void)
{
	/* TODO - IMPLEMENT */
}

bool MCCefBrowserBase::SetJavaScriptHandlerEnabled(const CefString &p_handler, bool p_enabled)
{
	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_ENABLE_JS_HANDLER);

	CefRefPtr<CefListValue> t_args;
	t_args = t_message->GetArgumentList();

	t_args->SetString(0, p_handler);
	t_args->SetBool(1, p_enabled);

	m_browser->SendProcessMessage(PID_RENDERER, t_message);

	/* UNCHECKED */ return true;
}

void MCCefBrowserBase::AddJavaScriptHandler(const char *p_handler)
{
	CefString t_handler;
	/* UNCHECKED */ MCCefStringFromUtf8String(p_handler, t_handler);

	/* UNCHECKED */ SetJavaScriptHandlerEnabled(t_handler, true);
}

void MCCefBrowserBase::RemoveJavaScriptHandler(const char *p_handler)
{
	CefString t_handler;
	/* UNCHECKED */ MCCefStringFromUtf8String(p_handler, t_handler);

	/* UNCHECKED */ SetJavaScriptHandlerEnabled(t_handler, false);
}
