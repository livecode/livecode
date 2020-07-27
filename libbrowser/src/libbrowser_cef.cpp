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

#include <stdlib.h>

#include "libbrowser_cef.h"

#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <include/wrapper/cef_scoped_temp_dir.h>
#include <list>
#include <set>

#if defined(TARGET_PLATFORM_POSIX)
#include "signal_restore_posix.h"
#include <sys/time.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#define CEF_DUMMY_URL "http://libbrowser_dummy_url/"

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
	{
		char *t_end;
		t_int = strtoul(t_tmp_string, &t_end, 10);
		
		t_success = t_end == t_tmp_string + MCCStringLength(t_tmp_string);
	}
	
	if (t_tmp_string != nil)
		MCCStringFree(t_tmp_string);
	
	if (t_success)
		r_int = t_int;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCefListToBrowserList(CefRefPtr<CefListValue> p_list, MCBrowserListRef &r_list)
{
	bool t_success;
	t_success = true;
	
	MCBrowserListRef t_list;
	t_list = nil;
	
	size_t t_size;
	t_size = p_list->GetSize();
	
	if (t_success)
		t_success = MCBrowserListCreate(t_list, t_size);
	
	for (uint32_t i = 0; t_success && i < t_size; i++)
	{
		switch (p_list->GetType(i))
		{
			case VTYPE_BOOL:
				t_success = MCBrowserListSetBoolean(t_list, i, p_list->GetBool(i));
				break;
			
			case VTYPE_INT:
				t_success = MCBrowserListSetInteger(t_list, i, p_list->GetInt(i));
				break;
			
			case VTYPE_DOUBLE:
				t_success = MCBrowserListSetDouble(t_list, i, p_list->GetDouble(i));
				break;
			
			case VTYPE_STRING:
			{
				char *t_string = nil;
				t_success = MCCefStringToUtf8String(p_list->GetString(i), t_string) && MCBrowserListSetUTF8String(t_list, i, t_string);
				if (t_string != nil)
					MCCStringFree(t_string);
				break;
			}
			
			case VTYPE_LIST:
			{
				MCBrowserListRef t_list_val = nil;
				t_success = MCCefListToBrowserList(p_list->GetList(i), t_list_val) && MCBrowserListSetList(t_list, i, t_list_val);
				MCBrowserListRelease(t_list_val);
				break;
			}
			
			case VTYPE_DICTIONARY:
			{
				MCBrowserDictionaryRef t_dict_val = nil;
				t_success = MCCefDictionaryToBrowserDictionary(p_list->GetDictionary(i), t_dict_val) && MCBrowserListSetDictionary(t_list, i, t_dict_val);
				MCBrowserDictionaryRelease(t_dict_val);
				break;
			}
			default:
				// unimplemented value type
				t_success = false;
		}
	}
	
	if (t_success)
		r_list = t_list;
	else
		MCBrowserListRelease(t_list);
	
	return t_success;
}

bool MCCefDictionaryToBrowserDictionary(CefRefPtr<CefDictionaryValue> p_dict, MCBrowserDictionaryRef &r_dict)
{
	bool t_success;
	t_success = true;
	
	MCBrowserDictionaryRef t_dict;
	t_dict = nil;
	
	CefDictionaryValue::KeyList t_key_list;
	if (t_success)
		t_success = p_dict->GetKeys(t_key_list);
	
	if (t_success)
		t_success = MCBrowserDictionaryCreate(t_dict, t_key_list.size());
	
	for (CefDictionaryValue::KeyList::iterator i = t_key_list.begin(); t_success && i < t_key_list.end(); i++)
	{
		char *t_key;
		t_key = nil;
		
		if (t_success)
			t_success = MCCefStringToUtf8String(*i, t_key);
		
		if (t_success)
		{
			switch (p_dict->GetType(*i))
			{
				case VTYPE_BOOL:
					t_success = MCBrowserDictionarySetBoolean(t_dict, t_key, p_dict->GetBool(*i));
					break;
				
				case VTYPE_INT:
					t_success = MCBrowserDictionarySetInteger(t_dict, t_key, p_dict->GetInt(*i));
					break;
				
				case VTYPE_DOUBLE:
					t_success = MCBrowserDictionarySetDouble(t_dict, t_key, p_dict->GetDouble(*i));
					break;
				
				case VTYPE_STRING:
				{
					char *t_string = nil;
					t_success = MCCefStringToUtf8String(p_dict->GetString(*i), t_string) && MCBrowserDictionarySetUTF8String(t_dict, t_key, t_string);
					if (t_string != nil)
						MCCStringFree(t_string);
					break;
				}
				
				case VTYPE_LIST:
				{
					MCBrowserListRef t_list_val = nil;
					t_success = MCCefListToBrowserList(p_dict->GetList(*i), t_list_val) && MCBrowserDictionarySetList(t_dict, t_key, t_list_val);
					MCBrowserListRelease(t_list_val);
					break;
				}
				
				case VTYPE_DICTIONARY:
				{
					MCBrowserDictionaryRef t_dict_val = nil;
					t_success = MCCefDictionaryToBrowserDictionary(p_dict->GetDictionary(*i), t_dict_val) && MCBrowserDictionarySetDictionary(t_dict, t_key, t_dict_val);
					MCBrowserDictionaryRelease(t_dict_val);
					break;
				}
				
				default:
					// unimplemented value type
					t_success = false;
			}
		}
		
		if (t_key != nil)
			MCCStringFree(t_key);
	}
	
	if (t_success)
		r_dict = t_dict;
	else
		MCBrowserDictionaryRelease(t_dict);
	
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

static CefScopedTempDir s_temp_cache;

void MCCefBrowserExternalInit(void)
{
	// set up static vars
	s_cef_initialised = false;
	s_cefbrowser_initialised = false;
	s_instance_count = 0;
}

void MCCefBrowserRunloopAction(void *p_context)
{
	CefDoMessageLoopWork();
}

class MCCefBrowserApp : public CefApp, CefBrowserProcessHandler
{
public:
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE { return this; }

	virtual void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> p_command_line) OVERRIDE
	{
		if (MCCefPlatformGetHiDPIEnabled())
			p_command_line->AppendSwitch(MC_CEF_HIDPI_SWITCH);
	}

	IMPLEMENT_REFCOUNTING(MCCefBrowserApp);
};

extern "C" int initialise_weak_link_cef(void);

// These come from the engine we link to.
extern "C" void *MCSupportLibraryLoad(const char *p_path);
extern "C" void *MCSupportLibraryUnload(void *p_handle);
extern "C" char *MCSupportLibraryCopyNativePath(void *p_handle);

#if defined(WIN32)
static const char *kCefProcessName = "libbrowser-cefprocess.exe";
static const char *kCefPathSeparatorStr = "\\";
static const char kCefPathSeparator = '\\';
#else
static const char *kCefProcessName = "libbrowser-cefprocess";
static const char *kCefPathSeparatorStr = "/";
static const char kCefPathSeparator = '/';
#endif

#if defined(TARGET_PLATFORM_LINUX)

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

static bool get_link_size(const char *p_path, uint32_t &r_size)
{
    if (p_path == nil)
        return false;
    
    struct stat t_stat;
    if (lstat(p_path, &t_stat) == -1)
        return false;
    
    r_size = t_stat.st_size;
    return true;
}

static bool get_link_path(const char *p_link, char *&r_path)
{
    bool t_success;
    t_success = true;
    
    char *t_buffer;
    t_buffer = nil;
    uint32_t t_buffer_size;
    t_buffer_size = 0;
    
    uint32_t t_link_size;
    t_success = get_link_size(p_link, t_link_size);
    
    while (t_success && t_link_size + 1 > t_buffer_size)
    {
        t_buffer_size = t_link_size + 1;
        t_success = MCBrowserMemoryReallocate(t_buffer, t_buffer_size, t_buffer);
        
        if (t_success)
        {
            int32_t t_read;
            t_read = readlink(p_link, t_buffer, t_buffer_size);
            
            t_success = t_read >= 0;
            t_link_size = t_read;
        }
    }
    
    if (t_success)
    {
        t_buffer[t_link_size] = '\0';
        r_path = t_buffer;
    }
    else
        MCBrowserMemoryDeallocate(t_buffer);
    
    return t_success;
}

static bool get_exe_path_from_proc_fs(char *&r_path)
{
    return get_link_path("/proc/self/exe", r_path);
}

//////////

const char *__MCCefPlatformGetExecutableFolder(void)
{
    static char *s_exe_path = nil;
    
    if (s_exe_path == nil)
    {
        bool t_success;
        t_success = get_exe_path_from_proc_fs(s_exe_path);
        if (t_success)
        {
            // remove library component from path
            uint32_t t_index;
            if (MCCStringLastIndexOf(s_exe_path, '/', t_index))
                s_exe_path[t_index] = '\0';
        }
    }
    
    return s_exe_path;
}
#endif

static bool __MCCefGetLibraryPath(char*& r_path)
{
    void *t_module = nullptr;
    if (t_module == nullptr)
        t_module = MCSupportLibraryLoad("./CEF/libcef");
    /* TODO[Bug 19381] On Linux and Windows, the in-git-checkout
     * location of CEF is "./CEF" but once LiveCode is installed it's
     * in "./Externals/CEF". */
#if defined(WIN32) || defined(TARGET_PLATFORM_LINUX)
    if (t_module == nullptr)
        t_module = MCSupportLibraryLoad("./Externals/CEF/libcef");
#endif

    char *t_module_path =
            t_module != nullptr ? MCSupportLibraryCopyNativePath(t_module)
                                : nullptr;
    
    MCSupportLibraryUnload(t_module);

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
    else if (MCCStringEndsWith(p_base, kCefPathSeparatorStr) ||
             *p_path == '\0')
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
	t_settings.multi_threaded_message_loop = MC_CEF_USE_MULTITHREADED_MESSAGELOOP;
	t_settings.command_line_args_disabled = true;
	t_settings.windowless_rendering_enabled = true;
	t_settings.no_sandbox = true;
#ifdef _DEBUG
	t_settings.log_severity = LOGSEVERITY_VERBOSE;
#else
	t_settings.log_severity = LOGSEVERITY_DISABLE;
#endif
	
    bool t_success = true;
#ifdef TARGET_PLATFORM_LINUX
    if (t_success)
        t_success = __MCCefBuildPath(__MCCefPlatformGetExecutableFolder(), kCefProcessName, &t_settings.browser_subprocess_path);
#else
    if (t_success)
        t_success = __MCCefBuildPath(t_library_path, kCefProcessName, &t_settings.browser_subprocess_path);
#endif

	if (t_success)
		t_success = __MCCefBuildPath(t_library_path, "locales", &t_settings.locales_dir_path);
	if (t_success)
		t_success = __MCCefBuildPath(t_library_path, "", &t_settings.resources_dir_path);

	if (t_success)
		t_success = s_temp_cache.CreateUniqueTempDir();
	
	if (t_success)
		CefString(&t_settings.cache_path).FromString(s_temp_cache.GetPath());

	CefRefPtr<CefApp> t_app = new (nothrow) MCCefBrowserApp();
	
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

    free(t_library_path);
	
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
	
	if (t_success && !MC_CEF_USE_MULTITHREADED_MESSAGELOOP)
		t_success = MCBrowserAddRunloopAction(MCCefBrowserRunloopAction, nil);
	
	s_cefbrowser_initialised = t_success;
	
	return s_cefbrowser_initialised;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Shutdown the CEF library
void MCCefFinalise(void)
{
	if (!s_cef_initialised)
		return;

	if (s_temp_cache.IsValid())
	{
		s_temp_cache.Delete();
	}
	
	CefShutdown();

	s_cef_initialised = false;
}

// IM-2014-03-13: [[ revBrowserCEF ]] Shut down the browser
void MCCefBrowserFinalise(void)
{
	if (!s_cefbrowser_initialised)
		return;
	
	if (!MC_CEF_USE_MULTITHREADED_MESSAGELOOP)
		MCBrowserRemoveRunloopAction(MCCefBrowserRunloopAction, nil);
	
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
				case VTYPE_STRING:
				case VTYPE_BINARY:
				case VTYPE_DICTIONARY:
				case VTYPE_LIST:
				case VTYPE_INVALID:
				/* UNIMPLEMENTED */
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
	CefLoadHandler::ErrorCode error_code;
};

class MCCefBrowserClient : public CefClient, CefLifeSpanHandler, CefRequestHandler, /* CefDownloadHandler ,*/ CefLoadHandler, CefContextMenuHandler, CefDragHandler
{
public:
	enum PageOrigin
	{
		kNone,
		kSetUrl,
		kSetSource,
		kBrowse,
	};

private:
	int m_browser_id;
	int m_popup_browser_id = 0;

	MCCefBrowserBase *m_owner;
	
	MCCefMessageResult m_message_result;
	std::map<int64_t, MCCefErrorInfo> m_load_error_frames;
	
	// Describes where loading url requests came from
	std::map<CefString, PageOrigin> m_load_url_origins;
	
	CefString m_last_request_url;
	
	PageOrigin m_displayed_page_origin;

	// Error handling - we need to keep track of url that failed to load in a
	// frame so we can send the correct url in onLoadEnd()
	void AddLoadErrorFrame(int64_t p_id, const CefString &p_url, const CefString &p_error_msg, CefLoadHandler::ErrorCode p_error_code)
	{
		m_load_error_frames[p_id].url = p_url;
		m_load_error_frames[p_id].error_message = p_error_msg;
		m_load_error_frames[p_id].error_code = p_error_code;
	}
	
	bool FindLoadErrorFrame(int64_t p_id, MCCefErrorInfo &r_info, bool p_delete)
	{
		std::map<int64_t, MCCefErrorInfo>::iterator t_iter;
		t_iter = m_load_error_frames.find(p_id);
		
		if (t_iter == m_load_error_frames.end())
			return false;
		
		r_info = t_iter->second;
		
		if (p_delete)
			m_load_error_frames.erase(t_iter);
		
		return true;
	}
	
	bool RemoveLoadErrorFrame(int64_t p_id, CefString &r_error_url, CefString &r_error_msg, CefLoadHandler::ErrorCode &r_error_code)
	{
		MCCefErrorInfo t_info;
		
		if (!FindLoadErrorFrame(p_id, t_info, true))
			return false;
		
		r_error_url = t_info.url;
		r_error_msg = t_info.error_message;
		r_error_code = t_info.error_code;
		
		return true;
	}
	
	bool FetchLoadErrorFrame(int64_t p_id, CefString &r_error_url, CefString &r_error_msg, CefLoadHandler::ErrorCode &r_error_code)
	{
		MCCefErrorInfo t_info;
		
		if (!FindLoadErrorFrame(p_id, t_info, false))
			return false;
		
		r_error_url = t_info.url;
		r_error_msg = t_info.error_message;
		r_error_code = t_info.error_code;
		
		return true;
	}
	
public:
	MCCefBrowserClient(MCCefBrowserBase *p_owner)
	{
		m_owner = p_owner;
		m_browser_id = 0;
		m_displayed_page_origin = PageOrigin::kNone;
	}
	
	// IM-2014-07-21: [[ Bug 12296 ]] Method to allow owner to notify client of its deletion
	void OnOwnerClosed(void)
	{
		m_owner = nil;
	}
	
	// Tell browser which callback interfaces we implement
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE { return this; }
//	virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefDragHandler> GetDragHandler() OVERRIDE { return this; }
	
	void AddLoadingUrl(const CefString &p_url, PageOrigin p_origin)
	{
		m_load_url_origins[p_url] = p_origin;
	}
	
	void RemoveLoadingUrl(const CefString &p_url)
	{
		m_load_url_origins.erase(p_url);
	}
	
	bool GetLoadingUrlOrigin(const CefString &p_url, PageOrigin &r_origin)
	{
		auto t_iter = m_load_url_origins.find(p_url);
		if (t_iter == m_load_url_origins.end())
			return false;

		r_origin = t_iter->second;
		return true;
	}

	PageOrigin GetDisplayedPageOrigin()
	{
		return m_displayed_page_origin;
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
			
			MCBrowserRunloopBreakWait();
			
			return true;
		}
		else if (t_message_name == MC_CEFMSG_JS_HANDLER)
		{
			// Handle JavaScript call to LiveCode handler
			
			CefRefPtr<CefListValue> t_args;
			t_args = p_message->GetArgumentList();
			
			char *t_handler;
			t_handler = nil;
			
			bool t_success;
			t_success = true;
			
			// IM-2014-07-21: [[ Bug 12296 ]] Don't proceed with callback if browser has been closed
			if (t_success)
				t_success = nil != m_owner;
			
			if (t_success)
				t_success = MCCefStringToUtf8String(t_args->GetString(0), t_handler);

			MCBrowserListRef t_param_list;
			t_param_list = nil;
			
			if (t_success)
				t_success = MCCefListToBrowserList(t_args->GetList(1), t_param_list);
			
			if (t_success)
				m_owner->OnJavaScriptCall(t_handler, t_param_list);
			
			if (t_handler)
				MCCStringFree(t_handler);
			
			if (t_param_list != nil)
				MCBrowserListRelease(t_param_list);
			
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
		else
		{
			m_popup_browser_id = p_browser->GetIdentifier();
		}

		MCCefIncrementInstanceCount();
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

		CefWindowInfo(t_window_info);
		t_window_info.SetAsWindowless(p_browser->GetHost()->GetWindowHandle());
		p_window_info = t_window_info;
		return false;

	}
	
	// CefDragHandler interface

	// Called on UI thread
	virtual bool OnDragEnter(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefDragData> p_drag_data, CefDragHandler::DragOperationsMask p_mask)
	{
		// cancel the drag event
		return true;
	}

	// CefRequestHandler interface
	
	// Called on UI thread
	virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefRequest> p_request, bool p_user_gesture, bool p_is_redirect) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return true;

		if (p_browser->GetIdentifier() == m_popup_browser_id)
		{
			char * t_url = nullptr;
			if (MCCefStringToUtf8String(p_request->GetURL(), t_url))
			{
				m_owner->GoToURL(t_url);
			}
			p_browser->GetHost()->CloseBrowser(true);
			return false;
		}

		bool t_cancel;
		t_cancel = false;
		
		CefString t_url;
		t_url = p_request->GetURL();
		
		PageOrigin t_origin = PageOrigin::kNone;

		if (p_user_gesture)
		{
			t_origin = PageOrigin::kBrowse;
			/* UNCHECKED */ AddLoadingUrl(t_url, t_origin);
		}
		else
			/* UNCHECKED */ GetLoadingUrlOrigin(t_url, t_origin);

		if (t_origin == PageOrigin::kSetSource)
			return false;
		
		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);
		
		if (t_url_str != nil)
			MCCStringFree(t_url_str);
		
		return t_cancel;
	}
	
	virtual CefRequestHandler::ReturnValue OnBeforeResourceLoad(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefRequest> p_request, CefRefPtr<CefRequestCallback> p_callback) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return RV_CANCEL;
		
		CefString t_url;
		t_url = p_request->GetURL();
		
		PageOrigin t_origin = PageOrigin::kNone;
		/* UNCHECKED */ GetLoadingUrlOrigin(t_url, t_origin);

		if (t_origin == PageOrigin::kSetSource)
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
	
#if CEF_ON_DOWNLOAD_CALLBACK
	// TODO - Implement OnDownload callback
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
#endif
	
	// CefLoadHandler interface
	// Methods called on UI thread or render process main thread

	virtual void OnLoadStart(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, cef_transition_type_t p_transition_type) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		// render process may have restarted so resend js handler list
		m_owner->SyncJavaScriptHandlers();
		
		CefString t_url, t_error;
		CefLoadHandler::ErrorCode t_error_code;
		
		bool t_is_error;
		t_is_error = FetchLoadErrorFrame(p_frame->GetIdentifier(), t_url, t_error, t_error_code);
		
		if (!t_is_error)
			t_url = p_frame->GetURL();
		
		bool t_frame;
		t_frame = !p_frame->IsMain();

		PageOrigin t_origin = PageOrigin::kNone;
		/* UNCHECKED */ GetLoadingUrlOrigin(t_url, t_origin);

		if (!t_frame && !t_is_error)
			m_displayed_page_origin = t_origin;

		if (!t_frame && t_origin == PageOrigin::kSetSource)
			return;
		
		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);
		
		if (!t_is_error)
		{
			if (!t_frame)
				m_owner->OnNavigationBegin(t_frame, t_url_str);
			
			m_owner->OnDocumentLoadBegin(t_frame, t_url_str);
		}
		
		if (t_url_str != nil)
			MCCStringFree(t_url_str);
	}
	
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, int p_http_status_code) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		CefString t_url, t_error;
		CefLoadHandler::ErrorCode t_error_code;
		
		bool t_is_error;
		t_is_error = RemoveLoadErrorFrame(p_frame->GetIdentifier(), t_url, t_error, t_error_code);
		
		if (!t_is_error)
			t_url = p_frame->GetURL();
		
		bool t_frame;
		t_frame = !p_frame->IsMain();

		PageOrigin t_origin = PageOrigin::kNone;
		/* UNCHECKED */ GetLoadingUrlOrigin(t_url, t_origin);
		/* UNCHECKED */ RemoveLoadingUrl(t_url);

		if (!t_frame && t_origin == PageOrigin::kSetSource)
			return CefLoadHandler::OnLoadEnd(p_browser, p_frame, p_http_status_code);
		
		char *t_url_str;
		t_url_str = nil;
		/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);
		
		if (t_is_error)
		{
			if (t_error_code != ERR_UNKNOWN_URL_SCHEME)
			{
				char *t_err_str;
				t_err_str = nil;
				/* UNCHECKED */ MCCefStringToUtf8String(t_error, t_err_str);
				
				m_owner->OnDocumentLoadFailed(t_frame, t_url_str, t_err_str);
				
				if (!t_frame)
					m_owner->OnNavigationFailed(t_frame, t_url_str, t_err_str);
				
				if (t_err_str != nil)
					MCCStringFree(t_err_str);
			}
		}
		else
		{
			m_owner->OnDocumentLoadComplete(t_frame, t_url_str);
			
			if (!t_frame)
				m_owner->OnNavigationComplete(t_frame, t_url_str);
		}
		
		if (t_url_str != nil)
			MCCStringFree(t_url_str);
	}
	
	virtual void OnLoadError(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefLoadHandler::ErrorCode p_error_code, const CefString &p_error_text, const CefString &p_failed_url) OVERRIDE
	{
		if (p_error_code == ERR_UNKNOWN_URL_SCHEME)
		{
			bool t_frame;
			t_frame = !p_frame->IsMain();

			char *t_url_str = nullptr;
			if (MCCefStringToUtf8String(p_failed_url, t_url_str))
			{
				m_owner->OnNavigationRequestUnhandled(t_frame, t_url_str);
			}
		}
		else if (p_error_code != ERR_ABORTED)
		{
			// IM-2015-11-16: [[ Bug 16360 ]] Contrary to the CEF API docs, OnLoadEnd is NOT called after OnLoadError when the error code is ERR_ABORTED.
			//    This occurs when requesting a new url be loaded when in the middle of loading the previous url, or when the url load is otherwise cancelled.
			AddLoadErrorFrame(p_frame->GetIdentifier(), p_failed_url, p_error_text, p_error_code);
		}
	}
	
	// ContextMenuHandler interface
	// Methods called on UI thread
	
	virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefFrame> p_frame, CefRefPtr<CefContextMenuParams> p_params, CefRefPtr<CefMenuModel> p_model) OVERRIDE
	{
		// IM-2014-07-21: [[ Bug 12296 ]] If browser has been closed then exit
		if (nil == m_owner)
			return;
		
		// clearing the menu model prevents the context menu from opening
		if (!m_owner->GetEnableContextMenu())
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
	
	// Using a blank data url here forces the CEF render process to load,
	// allowing subsequent calls to MCCefBrowserBase::LoadHTMLText to succeed
	CefString t_url("data:text/html;charset=utf-8,");
	
	// IM-2014-05-06: [[ Bug 12384 ]] Prevent callback messages for dummy URL
	m_client->AddLoadingUrl(t_url, MCCefBrowserClient::PageOrigin::kSetSource);
	PlatformConfigureWindow(t_window_info);

	if (MC_CEF_USE_MULTITHREADED_MESSAGELOOP)
	{
		// need to use asnyc version when not on UI thread
		if (!CefBrowserHost::CreateBrowser(t_window_info, m_client.get(), t_url, t_settings, NULL))
			return false;

		while (m_browser == nil)
			MCBrowserRunloopWait();
	}
	else
		CefBrowserHost::CreateBrowserSync(t_window_info, m_client.get(), t_url, t_settings, NULL);
	
	return m_browser != nil;
}

void MCCefBrowserBase::Finalize()
{
	if (m_browser != nil)
		m_browser->GetHost()->CloseBrowser(true);
	
	// IM-2014-07-21: [[ Bug 12296 ]] Notify client of browser being closed
	if (m_client)
		m_client->OnOwnerClosed();
    
    if (m_javascript_handlers != nil)
        MCCStringFree(m_javascript_handlers);
    
	m_browser = nil;
	m_client = nil;
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
	
	m_javascript_handlers = nil;
	m_js_handler_list = CefListValue::Create();
}

MCCefBrowserBase::~MCCefBrowserBase(void)
{
	MCCefDecrementInstanceCount();
}

void MCCefBrowserBase::Destroy()
{
	// Calling virtual methods from the destructor is A Bad Thing, so make sure
	//   we close the CEF browser (which results in a call to 
	//   virtual method PlatformCloseBrowserWindow()) before destruction.
	Finalize();
	MCBrowserRefCounted::Destroy();
}

////////////////////////////////////////////////////////////////////////////////

MCBrowser *MCCefBrowserInstantiate(void *p_display, void *p_parent_window)
{
	// IM-2014-03-18: [[ revBrowserCEF ]] Make sure cef library is loaded before trying to create browser
	if (!MCCefBrowserInitialise())
		return nil;
	
	MCCefBrowserBase *t_browser;
	if (!MCCefPlatformCreateBrowser(p_display, p_parent_window, t_browser))
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
	{
		m_browser = p_browser;
		// Make sure we break out of the wait loop if created asynchronously
		if (MC_CEF_USE_MULTITHREADED_MESSAGELOOP)
			MCBrowserRunloopBreakWait();
	}
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
	
	bool t_success;
	t_success = true;
	
	while (t_success && !t_result.HaveResult())
		t_success = MCBrowserRunloopWait();
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

bool MCCefBrowserBase::GetEnableContextMenu(void)
{
	return m_show_context_menu;
}

void MCCefBrowserBase::SetEnableContextMenu(bool p_menu)
{
	m_show_context_menu = p_menu;
}

bool MCCefBrowserBase::GetAllowNewWindow(void)
{
	return m_allow_new_window;
}

void MCCefBrowserBase::SetAllowNewWindow(bool p_new_window)
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

bool MCCefBrowserBase::LoadHTMLText(const char *p_htmltext, const char *p_base_url)
{
	// IM-2014-06-25: [[ Bug 12701 ]] CEF will crash if given an empty source string,
	// so replace here with the source of an empty page :)
	if (p_htmltext == nil || MCCStringLength(p_htmltext) == 0)
		p_htmltext = "<html><head></head><body></body></html>";
	
	CefString t_htmltext;
	if (!MCCefStringFromUtf8String(p_htmltext, t_htmltext))
		return false;
	
	CefString t_base_url;
	if (!MCCefStringFromUtf8String(p_base_url, t_base_url))
		return false;
	
	m_client->AddLoadingUrl(t_base_url, MCCefBrowserClient::PageOrigin::kSetSource);
	m_browser->GetMainFrame()->LoadString(t_htmltext, t_base_url);
	
	return true;
}

void MCCefBrowserBase::SetSource(const char *p_source)
{
	/* UNCHECKED */ LoadHTMLText(p_source, CEF_DUMMY_URL);
}

#define MCCEF_VERTICAL_OVERFLOW_PROPERTY "document.body.style.overflowY"
#define MCCEF_HORIZONTAL_OVERFLOW_PROPERTY "document.body.style.overflowX"
inline const char *scrollbar_property(MCCefScrollbarDirection p_direction)
{
	return p_direction == kMCCefScrollbarVertical ? MCCEF_VERTICAL_OVERFLOW_PROPERTY : MCCEF_HORIZONTAL_OVERFLOW_PROPERTY;
}

// IM-2014-08-25: [[ Bug 13272 ]] Implement CEF browser scrollbar property.
bool MCCefBrowserBase::GetOverflowHidden(MCCefScrollbarDirection p_direction)
{
	// property available through JavaScript
	bool t_success;
	t_success = true;
	
	CefString t_value;
	
	t_success = EvalJavaScript(scrollbar_property(p_direction), t_value);
	
	// assume scrollbars are visible if property fetch fails
	if (!t_success)
		return true;
	
	return t_value == L"hidden";
}

// IM-2014-08-25: [[ Bug 13272 ]] Implement CEF browser scrollbar property.
void MCCefBrowserBase::SetOverflowHidden(MCCefScrollbarDirection p_direction, bool p_hidden)
{
	// property available through JavaScript
	
	bool t_success;
	t_success = true;
	
	char *t_overflow_script;
	t_overflow_script = nil;
	
	t_success = MCCStringFormat(t_overflow_script, "%s = \"%s\"", scrollbar_property(p_direction), p_hidden ? "hidden" : "");
	
	CefString t_return_value;
	
	if (t_success)
		t_success = EvalJavaScript(t_overflow_script, t_return_value);
	
	MCCStringFree(t_overflow_script);
}

bool MCCefBrowserBase::GetVerticalScrollbarEnabled(void)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	return !GetOverflowHidden(kMCCefScrollbarVertical);
}

void MCCefBrowserBase::SetVerticalScrollbarEnabled(bool p_scrollbars)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	/* UNCHECKED */ SetOverflowHidden(kMCCefScrollbarVertical, !p_scrollbars);
}

bool MCCefBrowserBase::GetHorizontalScrollbarEnabled(void)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	return !GetOverflowHidden(kMCCefScrollbarHorizontal);
}

void MCCefBrowserBase::SetHorizontalScrollbarEnabled(bool p_scrollbars)
{
	// IM-2014-08-25: [[ Bug 13272 ]] Show / hide scrollbars by setting the overflow style to empty / "hidden".
	/* UNCHECKED */ SetOverflowHidden(kMCCefScrollbarHorizontal, !p_scrollbars);
}

bool MCCefBrowserBase::GetRect(MCBrowserRect &r_rect)
{
	return PlatformGetRect(r_rect);
}

bool MCCefBrowserBase::SetRect(const MCBrowserRect &p_rect)
{
	return PlatformSetRect(p_rect);
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

char *MCCefBrowserBase::GetURL(void)
{
	if (m_client->GetDisplayedPageOrigin() == MCCefBrowserClient::PageOrigin::kSetSource)
	{
		// return empty string if page was loaded from source
		char *t_url_string;
		t_url_string = nil;
		/* UNCHECKED */ MCCStringClone("", t_url_string);
		return t_url_string;
	}

	CefString t_url;
	t_url = m_browser->GetMainFrame()->GetURL();
	
	char *t_url_str;
	t_url_str = nil;
	
	/* UNCHECKED */ MCCefStringToUtf8String(t_url, t_url_str);
	
	return t_url_str;
}

bool MCCefBrowserBase::GetImage(void*& r_data, int& r_length)
{
	/* TODO - IMPLEMENT */
	return false;
}

bool MCCefBrowserBase::GetIsSecure(void)
{
	CefRefPtr<CefBrowser> t_browser = GetCefBrowser();
	if (t_browser == nil)
		return false;

	CefRefPtr<CefBrowserHost> t_host = t_browser->GetHost();
	if (t_host == nil)
		return false;

	CefRefPtr<CefNavigationEntry> t_navigation_entry = t_host->GetVisibleNavigationEntry();
	if (t_navigation_entry == nil)
		return false;

	CefRefPtr<CefSSLStatus> t_ssl_status = t_navigation_entry->GetSSLStatus();
	if (t_ssl_status == nil)
		return false;

	return t_ssl_status->IsSecureConnection();
}

bool MCCefBrowserBase::GetAllowUserInteraction(void)
{
	bool t_value;
	/* UNCHECKED */ PlatformGetAllowUserInteraction(t_value);
	return t_value;
}

void MCCefBrowserBase::SetAllowUserInteraction(bool p_value)
{
	/* UNCHECKED */ PlatformSetAllowUserInteraction(p_value);
}

////////////////////////////////////////////////////////////////////////////////

// Browser Actions

bool MCCefBrowserBase::EvaluateJavaScript(const char *p_script, char *&r_result)
{
	bool t_success;
	t_success = true;

	CefString t_script;
	t_success = MCCefStringFromUtf8String(p_script, t_script);
	
	CefString t_return_value;
	if (t_success)
		t_success = EvalJavaScript(t_script, t_return_value);
	
	char *t_return_value_str;
	t_return_value_str = nil;
	
	if (t_success)
		t_success = MCCefStringToUtf8String(t_return_value, t_return_value_str);
	
	if (t_success)
		r_result = t_return_value_str;
	else
		MCCStringFree(t_return_value_str);
	
	return t_success;
}

bool MCCefBrowserBase::GoToURL(const char *p_url)
{
	CefRefPtr<CefBrowser> t_browser = GetCefBrowser();
	if (t_browser == nil)
		return false;
	
	CefRefPtr<CefFrame> t_frame;
	t_frame = t_browser->GetMainFrame();
	
	if (t_frame == nil)
		return false;
	
	CefString t_url;
	if (!MCCefStringFromUtf8String(p_url, t_url))
		return false;
	
	m_client->AddLoadingUrl(t_url, MCCefBrowserClient::PageOrigin::kSetUrl);

	t_frame->LoadURL(t_url);
	return true;
}

bool MCCefBrowserBase::StopLoading(void)
{
	m_browser->StopLoad();
	return true;
}

bool MCCefBrowserBase::Reload(void)
{
	m_browser->Reload();
	return true;
}

bool MCCefBrowserBase::GoBack(void)
{
	m_browser->GoBack();
	return true;
}

bool MCCefBrowserBase::GoForward(void)
{
	m_browser->GoForward();
	return true;
}

bool MCCefBrowserBase::SyncJavaScriptHandlers()
{
	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_SET_JS_HANDLER_LIST);
	
	CefRefPtr<CefListValue> t_args;
	t_args = t_message->GetArgumentList();
	
	t_args->SetList(0, m_js_handler_list->Copy());
	
	m_browser->SendProcessMessage(PID_RENDERER, t_message);
	
	/* UNCHECKED */ return true;
}

bool MCCefBrowserBase::SetJavaScriptHandlers(const char *p_handlers)
{
	bool t_success;
	t_success = true;
	
	char *t_new_handlers;
	t_new_handlers = nil;
	
	if (t_success)
		t_success = MCCStringClone(p_handlers, t_new_handlers);
	
	char **t_handlers;
	t_handlers = nil;
	
	uint32_t t_handler_count;
	t_handler_count = 0;
	
	CefRefPtr<CefListValue> t_handler_list;
	
	if (t_success)
		t_success = nil != (t_handler_list = CefListValue::Create());
	
	if (!MCCStringIsEmpty(t_new_handlers))
	{
		if (t_success)
			t_success = MCCStringSplit(t_new_handlers, '\n', t_handlers, t_handler_count);
		
		if (t_success)
			t_success = t_handler_list->SetSize(t_handler_count);
		
		for (uint32_t i = 0; t_success && i < t_handler_count; i++)
		{
			CefString t_string;
			t_success = MCCefStringFromUtf8String(t_handlers[i], t_string);
			if (t_success)
				t_success = t_handler_list->SetString(i, t_string);
		}
	}
	
	if (t_success)
	{
		m_js_handler_list = t_handler_list;
		/* UNCHECKED */ SyncJavaScriptHandlers();
	}
	
	if (t_success)
	{
		if (m_javascript_handlers != nil)
			MCCStringFree(m_javascript_handlers);
		m_javascript_handlers = t_new_handlers;
		t_new_handlers = nil;
	}
	
	if (t_handlers != nil)
		MCCStringArrayFree(t_handlers, t_handler_count);
	
	if (t_new_handlers != nil)
		MCCStringFree(t_new_handlers);

	return t_success;
}

bool MCCefBrowserBase::GetJavaScriptHandlers(char *&r_handlers)
{
	if (m_javascript_handlers == nil)
		return MCCStringClone("", r_handlers);
	else
		return MCCStringClone(m_javascript_handlers, r_handlers);
}

////////////////////////////////////////////////////////////////////////////////

void *MCCefBrowserBase::GetNativeLayer()
{
	void *t_layer;
	if (!PlatformGetNativeLayer(t_layer))
		return nil;
	
	return t_layer;
}

bool MCCefBrowserBase::GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
{
	switch (p_property)
	{
		case kMCBrowserAllowNewWindows:
			r_value = GetAllowNewWindow();
			return true;
			
		case kMCBrowserEnableContextMenu:
			r_value = GetEnableContextMenu();
			return true;
			
		case kMCBrowserVerticalScrollbarEnabled:
			r_value = GetVerticalScrollbarEnabled();
			return true;
			
		case kMCBrowserHorizontalScrollbarEnabled:
			r_value = GetHorizontalScrollbarEnabled();
			return true;

		case kMCBrowserIsSecure:
			r_value = GetIsSecure();
			return true;

		case kMCBrowserAllowUserInteraction:
			r_value = GetAllowUserInteraction();
			return true;

		default:
			break;
	}
	
	return true;
}

bool MCCefBrowserBase::SetBoolProperty(MCBrowserProperty p_property, bool p_value)
{
	switch (p_property)
	{
		case kMCBrowserAllowNewWindows:
			SetAllowNewWindow(p_value);
			return true;
			
		case kMCBrowserEnableContextMenu:
			SetEnableContextMenu(p_value);
			return true;
			
		case kMCBrowserVerticalScrollbarEnabled:
			SetVerticalScrollbarEnabled(p_value);
			return true;
			
		case kMCBrowserHorizontalScrollbarEnabled:
			SetHorizontalScrollbarEnabled(p_value);
			return true;

		case kMCBrowserAllowUserInteraction:
			SetAllowUserInteraction(p_value);
			return true;

		default:
			break;
	}
	
	return true;
}

bool MCCefBrowserBase::SetStringProperty(MCBrowserProperty p_property, const char *p_value)
{
	switch (p_property)
	{
		case kMCBrowserHTMLText:
			SetSource(p_value);
			return true;
			
		case kMCBrowserJavaScriptHandlers:
			return SetJavaScriptHandlers(p_value);
			
		case kMCBrowserUserAgent:
			SetUserAgent(p_value);
			return true;
			
		default:
			break;
	}
	
	return true;
}

bool MCCefBrowserBase::GetStringProperty(MCBrowserProperty p_property, char *&r_value)
{
	switch (p_property)
	{
		case kMCBrowserHTMLText:
			r_value = GetSource();
			return true;
			
		case kMCBrowserJavaScriptHandlers:
			return GetJavaScriptHandlers(r_value);
			
		case kMCBrowserUserAgent:
			r_value = GetUserAgent();
			return true;
			
		case kMCBrowserURL:
			r_value = GetURL();
			return true;
			
		default:
			break;
	}
	
	return true;
}

bool MCCefBrowserBase::SetIntegerProperty(MCBrowserProperty p_property, int32_t p_value)
{
	switch (p_property)
	{
		default:
			break;
	}
	
	return true;
}

bool MCCefBrowserBase::GetIntegerProperty(MCBrowserProperty p_property, int32_t &r_value)
{
	switch (p_property)
	{
		default:
			break;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Cef browser factory

MCCefBrowserFactory::MCCefBrowserFactory()
{
	MCCefBrowserExternalInit();
}

MCCefBrowserFactory::~MCCefBrowserFactory()
{
	// IM-2016-03-10: [[ Bug 17029 ]] Shutdown CEF library when factory deleted
	Finalize();
}

bool MCCefBrowserFactory::Initialize()
{
	return MCCefBrowserInitialise();
}

void MCCefBrowserFactory::Finalize()
{
	MCCefBrowserFinalise();
	MCCefFinalise();
}

bool MCCefBrowserFactory::CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
{
	if (p_parent_view == nil)
		return false;
	
	MCBrowser *t_browser;
	t_browser = MCCefBrowserInstantiate(p_display, p_parent_view);
	
	if (t_browser == nil)
		return false;
	
	r_browser = t_browser;
	
	return true;
}

bool MCCefBrowserFactoryCreate(MCBrowserFactoryRef &r_factory)
{
	MCCefBrowserFactory *t_factory;
	t_factory = new (nothrow) MCCefBrowserFactory();
	
	if (t_factory == nil)
		return false;
	
	if (!t_factory->Initialize())
	{
		delete t_factory;
		return false;
	}
	
	r_factory = t_factory;
	return true;
}
