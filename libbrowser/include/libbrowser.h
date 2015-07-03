/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

#ifndef __LIBBROWSER_H__
#define __LIBBROWSER_H__

////////////////////////////////////////////////////////////////////////////////
// C++ Implementation Class API

class MCBrowserRefCounted
{
public:
	MCBrowserRefCounted();
	virtual ~MCBrowserRefCounted() {}
	
	void Retain();
	void Release();
	
	virtual void Destroy();
	
private:
	uint32_t m_ref_count;
};

class MCBrowser;
typedef struct __MCBrowserList *MCBrowserListRef;

// Event handler interface
class MCBrowserEventHandler : public MCBrowserRefCounted
{
public:
	virtual void OnNavigationBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url) = 0;
	virtual void OnNavigationComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url) = 0;
	virtual void OnNavigationFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error) = 0;
	virtual void OnDocumentLoadBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url) = 0;
	virtual void OnDocumentLoadComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url) = 0;
	virtual void OnDocumentLoadFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error) = 0;
};

// Javascript call handler
class MCBrowserJavaScriptHandler : public MCBrowserRefCounted
{
public:
	virtual void OnJavaScriptCall(MCBrowser *p_browser, const char *p_handler, MCBrowserListRef p_params) = 0;
};

// Properties
enum MCBrowserProperty
{
//	kMCBrowserRect,
	// Boolean properties
	kMCBrowserScrollbars,
	kMCBrowserAllowNewWindows,
	kMCBrowserEnableContextMenu,
	
	// String properties
	kMCBrowserURL,
	kMCBrowserHTMLText,
	kMCBrowserUserAgent,
	kMCBrowserJavaScriptHandlers,
};

// Convenience struct for rect properties
struct MCBrowserRect
{
	int32_t left, top, right, bottom;
};

// Browser interface
class MCBrowser : public MCBrowserRefCounted
{
public:
	virtual void SetEventHandler(MCBrowserEventHandler *p_handler) = 0;
	virtual void SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler) = 0;
	
	virtual void *GetNativeLayer() = 0;
	
	virtual bool GetRect(MCBrowserRect &r_rect) = 0;
	virtual bool SetRect(const MCBrowserRect &p_rect) = 0;
	
	virtual bool GetBoolProperty(MCBrowserProperty p_property, bool &r_value) = 0;
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value) = 0;
	
	virtual bool GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string) = 0;
	virtual bool SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string) = 0;
	
	virtual bool GoBack() = 0;
	virtual bool GoForward() = 0;
	virtual bool GoToURL(const char *p_url) = 0;
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result) = 0;
};

// Browser factory interface
class MCBrowserFactory : public MCBrowserRefCounted
{
public:
	virtual bool CreateBrowser(MCBrowser *&r_browser) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
//  SYMBOL EXPORTS
//
#ifdef _WIN32
#  ifdef _MSC_VER
#    define MC_DLLEXPORT __declspec(dllexport)
#  else
#    define MC_DLLEXPORT __attribute__((dllexport))
#  endif
#else
#  define MC_DLLEXPORT __attribute__((__visibility__("default")))
#endif

////////////////////////////////////////////////////////////////////////////////
// C API

extern "C"
{
	
MC_DLLEXPORT bool MCBrowserLibraryInitialize();
MC_DLLEXPORT void MCBrowserLibraryFinalize();
	
typedef bool (*MCBrowserAllocator)(size_t p_size, void *&r_mem);
typedef void (*MCBrowserDeallocator)(void *p_mem);
typedef bool (*MCBrowserReallocator)(void *p_mem, size_t p_new_size, void *&r_new_mem);
typedef bool (*MCBrowserWaitFunction)(void);
	
MC_DLLEXPORT void MCBrowserLibrarySetAllocator(MCBrowserAllocator p_alloc);
MC_DLLEXPORT void MCBrowserLibrarySetDeallocator(MCBrowserDeallocator p_dealloc);
MC_DLLEXPORT void MCBrowserLibrarySetReallocator(MCBrowserReallocator p_dealloc);
MC_DLLEXPORT void MCBrowserLibrarySetWaitFunction(MCBrowserWaitFunction p_wait);

typedef void (*MCBrowserRunloopCallback)(void *p_context);
	
MC_DLLEXPORT bool MCBrowserLibraryGetRunloopCallback(MCBrowserRunloopCallback &r_callback, void *&r_context);
	
typedef struct __MCBrowserList *MCBrowserListRef;
enum MCBrowserValueType
{
	kMCBrowserValueTypeNone,
	kMCBrowserValueTypeBoolean,
	kMCBrowserValueTypeInteger,
	kMCBrowserValueTypeDouble,
	kMCBrowserValueTypeUTF8String,
	kMCBrowserValueTypeList,
};
	
MC_DLLEXPORT bool MCBrowserListCreate(MCBrowserListRef &r_browser, uint32_t p_size = 0);
MC_DLLEXPORT MCBrowserListRef MCBrowserListRetain(MCBrowserListRef p_list);
MC_DLLEXPORT void MCBrowserListRelease(MCBrowserListRef p_list);
	
MC_DLLEXPORT bool MCBrowserListGetSize(MCBrowserListRef p_list, uint32_t &r_size);
MC_DLLEXPORT bool MCBrowserListGetType(MCBrowserListRef p_list, uint32_t p_index, MCBrowserValueType &r_type);
	
MC_DLLEXPORT bool MCBrowserListSetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool p_value);
MC_DLLEXPORT bool MCBrowserListSetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t p_value);
MC_DLLEXPORT bool MCBrowserListSetDouble(MCBrowserListRef p_list, uint32_t p_index, double_t p_value);
MC_DLLEXPORT bool MCBrowserListSetUTF8String(MCBrowserListRef p_list, uint32_t p_index, const char *p_value);
MC_DLLEXPORT bool MCBrowserListSetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef p_value);
	
MC_DLLEXPORT bool MCBrowserListAppendBoolean(MCBrowserListRef p_list, bool p_value);
MC_DLLEXPORT bool MCBrowserListAppendInteger(MCBrowserListRef p_list, int32_t p_value);
MC_DLLEXPORT bool MCBrowserListAppendDouble(MCBrowserListRef p_list, double_t p_value);
MC_DLLEXPORT bool MCBrowserListAppendUTF8String(MCBrowserListRef p_list, const char *p_value);
MC_DLLEXPORT bool MCBrowserListAppendList(MCBrowserListRef p_list, MCBrowserListRef p_value);
	
MC_DLLEXPORT bool MCBrowserListGetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool &r_value);
MC_DLLEXPORT bool MCBrowserListGetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t &r_value);
MC_DLLEXPORT bool MCBrowserListGetDouble(MCBrowserListRef p_list, uint32_t p_index, double_t &r_value);
MC_DLLEXPORT bool MCBrowserListGetUTF8String(MCBrowserListRef p_list, uint32_t p_index, char *&r_value);
MC_DLLEXPORT bool MCBrowserListGetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef &r_value);
	
typedef struct __MCBrowser *MCBrowserRef;
typedef struct __MCBrowserFactory *MCBrowserFactoryRef;

MC_DLLEXPORT bool MCBrowserFactoryGet(const char *p_factory_id, MCBrowserFactoryRef &r_factory);
MC_DLLEXPORT bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, MCBrowserRef &r_browser);

MC_DLLEXPORT MCBrowserRef MCBrowserRetain(MCBrowserRef p_browser);
MC_DLLEXPORT void MCBrowserRelease(MCBrowserRef p_browser);

MC_DLLEXPORT void *MCBrowserGetNativeLayer(MCBrowserRef p_browser);

MC_DLLEXPORT bool MCBrowserGetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool &r_value);
MC_DLLEXPORT bool MCBrowserSetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool p_value);

MC_DLLEXPORT bool MCBrowserGetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, char *&r_utf8_string);
MC_DLLEXPORT bool MCBrowserSetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, const char *p_utf8_string);

MC_DLLEXPORT bool MCBrowserGoBack(MCBrowserRef p_browser);
MC_DLLEXPORT bool MCBrowserGoForward(MCBrowserRef p_browser);
MC_DLLEXPORT bool MCBrowserGoToURL(MCBrowserRef p_browser);
MC_DLLEXPORT bool MCBrowserEvaluateJavaScript(MCBrowserRef p_browser, const char *p_script, char *&r_result);

enum MCBrowserRequestType
{
	kMCBrowserRequestTypeNavigate,
	kMCBrowserRequestTypeDocumentLoad,
};

enum MCBrowserRequestState
{
	kMCBrowserRequestStateBegin,
	kMCBrowserRequestStateComplete,
	kMCBrowserRequestStateFailed,
};

typedef void (*MCBrowserRequestCallback)(void *p_context, MCBrowserRef p_browser, MCBrowserRequestType p_type, MCBrowserRequestState p_state, bool p_in_frame, const char *p_url, const char *p_error);
typedef void (*MCBrowserJavaScriptCallback)(void *p_context, MCBrowserRef p_browser, const char *p_handler, MCBrowserListRef p_params);

MC_DLLEXPORT bool MCBrowserSetRequestHandler(MCBrowserRef p_browser, MCBrowserRequestCallback p_callback, void *p_context);
MC_DLLEXPORT bool MCBrowserSetJavaScriptHandler(MCBrowserRef p_browser, MCBrowserJavaScriptCallback p_callback, void *p_context);

}

////////////////////////////////////////////////////////////////////////////////

#endif//__LIBBROWSER_H__