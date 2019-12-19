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

#ifndef __LIBBROWSER_H__
#define __LIBBROWSER_H__

////////////////////////////////////////////////////////////////////////////////
// C++ Implementation Class API

enum MCBrowserNavigationType
{
	kMCBrowserNavigationTypeFollowLink,
	kMCBrowserNavigationTypeSubmitForm,
	kMCBrowserNavigationTypeBackForward,
	kMCBrowserNavigationTypeReload,
	kMCBrowserNavigationTypeResubmitForm,
	kMCBrowserNavigationTypeOther,
};

enum MCBrowserDataDetectorType
{
	kMCBrowserDataDetectorTypeNone = 0,
	
	kMCBrowserDataDetectorTypePhoneNumberBit = 0,
	kMCBrowserDataDetectorTypeLinkBit = 1,
	kMCBrowserDataDetectorTypeEmailAddressBit = 2,
	kMCBrowserDataDetectorTypeMapAddressBit = 3,
	kMCBrowserDataDetectorTypeCalendarEventBit = 4,
	kMCBrowserDataDetectorTypeTrackingNumberBit = 5,
	kMCBrowserDataDetectorTypeFlightNumberBit = 6,
	kMCBrowserDataDetectorTypeLookupSuggestionBit = 7,
	
	kMCBrowserDataDetectorTypePhoneNumber = (1 << kMCBrowserDataDetectorTypePhoneNumberBit),
	kMCBrowserDataDetectorTypeLink = (1 << kMCBrowserDataDetectorTypeLinkBit),
	kMCBrowserDataDetectorTypeEmailAddress = (1 << kMCBrowserDataDetectorTypeEmailAddressBit),
	kMCBrowserDataDetectorTypeMapAddress = (1 << kMCBrowserDataDetectorTypeMapAddressBit),
	kMCBrowserDataDetectorTypeCalendarEvent = (1 << kMCBrowserDataDetectorTypeCalendarEventBit),
	kMCBrowserDataDetectorTypeTrackingNumber = (1 << kMCBrowserDataDetectorTypeTrackingNumberBit),
	kMCBrowserDataDetectorTypeFlightNumber = (1 << kMCBrowserDataDetectorTypeFlightNumberBit),
	kMCBrowserDataDetectorTypeLookupSuggestion = (1 << kMCBrowserDataDetectorTypeLookupSuggestionBit),
	
	kMCBrowserDataDetectorTypeAll =
		kMCBrowserDataDetectorTypePhoneNumber |
		kMCBrowserDataDetectorTypeLink |
		kMCBrowserDataDetectorTypeEmailAddress |
		kMCBrowserDataDetectorTypeMapAddress |
		kMCBrowserDataDetectorTypeCalendarEvent |
		kMCBrowserDataDetectorTypeTrackingNumber |
		kMCBrowserDataDetectorTypeFlightNumber |
		kMCBrowserDataDetectorTypeLookupSuggestion,
};

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

class MCBrowserNavigationRequest : public MCBrowserRefCounted
{
public:
	virtual const char *GetURL(void) = 0;
	virtual bool IsFrame(void) = 0;
	virtual MCBrowserNavigationType GetNavigationType(void) = 0;
	
	virtual void Continue(void) = 0;
	virtual void Cancel(void) = 0;
};

class MCBrowser;
typedef class MCBrowserList *MCBrowserListRef;

// Navigation request handler interface
class MCBrowserNavigationRequestHandler : public MCBrowserRefCounted
{
public:
	virtual bool OnNavigationRequest(MCBrowser *p_browser, MCBrowserNavigationRequest *p_request) = 0;
};

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

	virtual void OnNavigationRequestUnhandled(MCBrowser *p_browser, bool p_in_frame, const char *p_url) = 0;
};

// Javascript call handler
class MCBrowserJavaScriptHandler : public MCBrowserRefCounted
{
public:
	virtual void OnJavaScriptCall(MCBrowser *p_browser, const char *p_handler, MCBrowserListRef p_params) = 0;
};

// Progress update handler
class MCBrowserProgressHandler : public MCBrowserRefCounted
{
public:
	virtual void OnProgressChanged(MCBrowser *p_browser, const char *p_url, uint32_t p_progress) = 0;
};

// Properties
enum MCBrowserProperty
{
	kMCBrowserVerticalScrollbarEnabled, /* BOOLEAN */
	kMCBrowserHorizontalScrollbarEnabled, /* BOOLEAN */
	kMCBrowserAllowNewWindows, /* BOOLEAN */
	kMCBrowserEnableContextMenu, /* BOOLEAN */
	kMCBrowserAllowUserInteraction, /* BOOLEAN */
	kMCBrowserIsSecure, /* BOOLEAN */
	kMCBrowserURL, /* STRING */
	kMCBrowserHTMLText, /* STRING */
	kMCBrowserUserAgent, /* STRING */
	kMCBrowserJavaScriptHandlers, /* STRING */
	
	kMCBrowseriOSDelayRequests, /* BOOLEAN */
	kMCBrowseriOSAllowsInlineMediaPlayback, /* BOOLEAN */
	kMCBrowseriOSMediaPlaybackRequiresUserAction, /* BOOLEAN */
	kMCBrowseriOSAutoFit, /* BOOLEAN */

	kMCBrowserCanGoForward, /* BOOLEAN */
	kMCBrowserCanGoBack, /* BOOLEAN */
	kMCBrowserDataDetectorTypes, /* INTEGER */
	kMCBrowserScrollEnabled, /* BOOLEAN */
	kMCBrowserScrollCanBounce, /* BOOLEAN */
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
	virtual void SetNavigationRequestHandler(MCBrowserNavigationRequestHandler *p_handler) = 0;
	virtual void SetEventHandler(MCBrowserEventHandler *p_handler) = 0;
	virtual void SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler) = 0;
	virtual void SetProgressHandler(MCBrowserProgressHandler *p_handler) = 0;

	virtual void *GetNativeLayer() = 0;
	
	virtual bool GetRect(MCBrowserRect &r_rect) = 0;
	virtual bool SetRect(const MCBrowserRect &p_rect) = 0;
	
	virtual bool GetBoolProperty(MCBrowserProperty p_property, bool &r_value) = 0;
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value) = 0;
	
	virtual bool GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string) = 0;
	virtual bool SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string) = 0;
	
	virtual bool GetIntegerProperty(MCBrowserProperty p_property, int32_t &r_value) = 0;
	virtual bool SetIntegerProperty(MCBrowserProperty p_property, int32_t p_value) = 0;
	
	virtual bool GoBack() = 0;
	virtual bool GoForward() = 0;
	virtual bool GoToURL(const char *p_url) = 0;
	virtual bool LoadHTMLText(const char *p_htmltext, const char *p_base_url) = 0;
	virtual bool StopLoading() = 0;
	virtual bool Reload() = 0;
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result) = 0;
};

// Browser factory interface
class MCBrowserFactory : public MCBrowserRefCounted
{
public:
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser) = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
//  SYMBOL EXPORTS
//

/* MC_BROWSER_DLLEXPORT should be applied to declarations.  
 * MC_BROWSER_DLLEXPORT_DEF should be applied to definitions. These MUST 
 * be applied to functions for which there is a binding in browser.lcb
 * to avoid the symbols being dead stripped. */
#ifdef _WIN32
/* On Windows, declaring something as having "dllexport" storage
 * modifies the naming of the corresponding symbol, so the export
 * attribute must be attached to declarations (and possibly to the
 * definition *as well* if no separate declaration appears) */
#  ifdef _MSC_VER
#    define MC_BROWSER_DLLEXPORT __declspec(dllexport)
#  else
#    define MC_BROWSER_DLLEXPORT __attribute__((dllexport))
#  endif
#  define MC_BROWSER_DLLEXPORT_DEF MC_BROWSER_DLLEXPORT
#else
/* On non-Windows platforms, the external visibility of a symbol is
 * simply a property of its definition (i.e. whether or not it should
 * appear in the list of exported symbols). */
#  define MC_BROWSER_DLLEXPORT
#  define MC_BROWSER_DLLEXPORT_DEF __attribute__((__visibility__("default"), __used__))
#endif

////////////////////////////////////////////////////////////////////////////////
// C API

extern "C"
{
	
MC_BROWSER_DLLEXPORT bool MCBrowserLibraryInitialize();
MC_BROWSER_DLLEXPORT void MCBrowserLibraryFinalize();
	
typedef bool (*MCBrowserAllocator)(size_t p_size, void *&r_mem);
typedef void (*MCBrowserDeallocator)(void *p_mem);
typedef bool (*MCBrowserReallocator)(void *p_mem, size_t p_new_size, void *&r_new_mem);
typedef bool (*MCBrowserWaitFunction)(void);
typedef void (*MCBrowserBreakWaitFunction)(void);
	
MC_BROWSER_DLLEXPORT void MCBrowserLibrarySetAllocator(MCBrowserAllocator p_alloc);
MC_BROWSER_DLLEXPORT void MCBrowserLibrarySetDeallocator(MCBrowserDeallocator p_dealloc);
MC_BROWSER_DLLEXPORT void MCBrowserLibrarySetReallocator(MCBrowserReallocator p_realloc);
MC_BROWSER_DLLEXPORT void MCBrowserLibrarySetWaitFunction(MCBrowserWaitFunction p_wait);
MC_BROWSER_DLLEXPORT void MCBrowserLibrarySetBreakWaitFunction(MCBrowserBreakWaitFunction p_breakwait);

typedef void (*MCBrowserRunloopCallback)(void *p_context);
	
MC_BROWSER_DLLEXPORT bool MCBrowserLibraryGetRunloopCallback(MCBrowserRunloopCallback &r_callback, void *&r_context);

//////////

typedef class MCBrowserList *MCBrowserListRef;
typedef class MCBrowserDictionary *MCBrowserDictionaryRef;

enum MCBrowserValueType
{
	kMCBrowserValueTypeNone,
	kMCBrowserValueTypeBoolean,
	kMCBrowserValueTypeInteger,
	kMCBrowserValueTypeDouble,
	kMCBrowserValueTypeUTF8String,
	kMCBrowserValueTypeList,
	kMCBrowserValueTypeDictionary,
};
	
MC_BROWSER_DLLEXPORT bool MCBrowserListCreate(MCBrowserListRef &r_browser, uint32_t p_size = 0);
MC_BROWSER_DLLEXPORT MCBrowserListRef MCBrowserListRetain(MCBrowserListRef p_list);
MC_BROWSER_DLLEXPORT void MCBrowserListRelease(MCBrowserListRef p_list);
	
MC_BROWSER_DLLEXPORT bool MCBrowserListGetSize(MCBrowserListRef p_list, uint32_t &r_size);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetType(MCBrowserListRef p_list, uint32_t p_index, MCBrowserValueType &r_type);
	
MC_BROWSER_DLLEXPORT bool MCBrowserListSetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListSetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListSetDouble(MCBrowserListRef p_list, uint32_t p_index, double p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListSetUTF8String(MCBrowserListRef p_list, uint32_t p_index, const char *p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListSetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListSetDictionary(MCBrowserListRef p_list, uint32_t p_index, MCBrowserDictionaryRef p_value);
	
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendBoolean(MCBrowserListRef p_list, bool p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendInteger(MCBrowserListRef p_list, int32_t p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendDouble(MCBrowserListRef p_list, double p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendUTF8String(MCBrowserListRef p_list, const char *p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendList(MCBrowserListRef p_list, MCBrowserListRef p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListAppendDictionary(MCBrowserListRef p_list, MCBrowserDictionaryRef p_value);
	
MC_BROWSER_DLLEXPORT bool MCBrowserListGetBoolean(MCBrowserListRef p_list, uint32_t p_index, bool &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetInteger(MCBrowserListRef p_list, uint32_t p_index, int32_t &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetDouble(MCBrowserListRef p_list, uint32_t p_index, double &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetUTF8String(MCBrowserListRef p_list, uint32_t p_index, char *&r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetList(MCBrowserListRef p_list, uint32_t p_index, MCBrowserListRef &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserListGetDictionary(MCBrowserListRef p_list, uint32_t p_index, MCBrowserDictionaryRef &r_value);
	
//////////

MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryCreate(MCBrowserDictionaryRef &r_dict, uint32_t p_size = 0);
MC_BROWSER_DLLEXPORT MCBrowserDictionaryRef MCBrowserDictionaryRetain(MCBrowserDictionaryRef p_dict);
MC_BROWSER_DLLEXPORT void MCBrowserDictionaryRelease(MCBrowserDictionaryRef p_dict);
	
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetType(MCBrowserDictionaryRef p_dict, const char *p_key, MCBrowserValueType &r_type);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetKeys(MCBrowserDictionaryRef p_dict, char **&r_keys, uint32_t &r_count);

/* WORKAROUND - Can't currently dereference a Pointer-to-CString at the moment so need to provide key accessor functions */
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetKeyCount(MCBrowserDictionaryRef p_dict, uint32_t &r_count);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetKey(MCBrowserDictionaryRef p_dict, uint32_t p_index, char *&r_key);

MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetBoolean(MCBrowserDictionaryRef p_dict, const char *p_key, bool p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetInteger(MCBrowserDictionaryRef p_dict, const char *p_key, int32_t p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetDouble(MCBrowserDictionaryRef p_dict, const char *p_key, double p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetUTF8String(MCBrowserDictionaryRef p_dict, const char *p_key, const char *p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetList(MCBrowserDictionaryRef p_dict, const char *p_key, MCBrowserListRef p_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionarySetDictionary(MCBrowserDictionaryRef p_dict, const char *p_key, MCBrowserDictionaryRef p_value);
	
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetBoolean(MCBrowserDictionaryRef p_dict, const char *p_key, bool &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetInteger(MCBrowserDictionaryRef p_dict, const char *p_key, int32_t &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetDouble(MCBrowserDictionaryRef p_dict, const char *p_key, double &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetUTF8String(MCBrowserDictionaryRef p_dict, const char *p_key, char *&r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetList(MCBrowserDictionaryRef p_dict, const char *p_key, MCBrowserListRef &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserDictionaryGetDictionary(MCBrowserDictionaryRef p_dict, const char *p_key, MCBrowserDictionaryRef &r_value);
	
//////////

typedef class MCBrowserNavigationRequest *MCBrowserNavigationRequestRef;

MC_BROWSER_DLLEXPORT MCBrowserNavigationRequestRef MCBrowserNavigationRequestRetain(MCBrowserNavigationRequestRef p_request);
MC_BROWSER_DLLEXPORT void MCBrowserNavigationRequestRelease(MCBrowserNavigationRequestRef p_request);

MC_BROWSER_DLLEXPORT bool MCBrowserNavigationRequestGetURL(MCBrowserNavigationRequestRef p_request, char* &r_url);
MC_BROWSER_DLLEXPORT bool MCBrowserNavigationRequestGetNavigationType(MCBrowserNavigationRequestRef p_request, MCBrowserNavigationType &r_type);
MC_BROWSER_DLLEXPORT bool MCBrowserNavigationRequestIsFrame(MCBrowserNavigationRequestRef p_request, bool &r_frame);
MC_BROWSER_DLLEXPORT bool MCBrowserNavigationRequestContinue(MCBrowserNavigationRequestRef p_request);
MC_BROWSER_DLLEXPORT bool MCBrowserNavigationRequestCancel(MCBrowserNavigationRequestRef p_request);

//////////

typedef class MCBrowser *MCBrowserRef;
typedef class MCBrowserFactory *MCBrowserFactoryRef;

MC_BROWSER_DLLEXPORT bool MCBrowserFactoryGet(const char *p_factory_id, MCBrowserFactoryRef &r_factory);
MC_BROWSER_DLLEXPORT bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, void *p_display, void *p_parent_view, MCBrowserRef &r_browser);

MC_BROWSER_DLLEXPORT MCBrowserRef MCBrowserRetain(MCBrowserRef p_browser);
MC_BROWSER_DLLEXPORT void MCBrowserRelease(MCBrowserRef p_browser);

MC_BROWSER_DLLEXPORT void *MCBrowserGetNativeLayer(MCBrowserRef p_browser);

MC_BROWSER_DLLEXPORT bool MCBrowserGetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserSetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool p_value);

MC_BROWSER_DLLEXPORT bool MCBrowserGetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, char *&r_utf8_string);
MC_BROWSER_DLLEXPORT bool MCBrowserSetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, const char *p_utf8_string);

MC_BROWSER_DLLEXPORT bool MCBrowserGetIntegerProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, int32_t &r_value);
MC_BROWSER_DLLEXPORT bool MCBrowserSetIntegerProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, int32_t p_value);

MC_BROWSER_DLLEXPORT bool MCBrowserGoBack(MCBrowserRef p_browser);
MC_BROWSER_DLLEXPORT bool MCBrowserGoForward(MCBrowserRef p_browser);
MC_BROWSER_DLLEXPORT bool MCBrowserGoToURL(MCBrowserRef p_browser, const char *p_url);
MC_BROWSER_DLLEXPORT bool MCBrowserLoadHTMLText(MCBrowserRef p_browser, const char *p_htmltext, const char *p_baseurl);
MC_BROWSER_DLLEXPORT bool MCBrowserStopLoading(MCBrowserRef p_browser);
MC_BROWSER_DLLEXPORT bool MCBrowserReload(MCBrowserRef p_browser);
MC_BROWSER_DLLEXPORT bool MCBrowserEvaluateJavaScript(MCBrowserRef p_browser, const char *p_script, char *&r_result);

enum MCBrowserNavigationEventType
{
	kMCBrowserNavigationEventTypeNavigate,
	kMCBrowserNavigationEventTypeDocumentLoad,
};

enum MCBrowserNavigationState
{
	kMCBrowserNavigationStateBegin,
	kMCBrowserNavigationStateComplete,
	kMCBrowserNavigationStateFailed,
	kMCBrowserNavigationStateUnhandled,
};

typedef bool (*MCBrowserNavigationRequestCallback)(void *p_context, MCBrowserRef p_browser, MCBrowserNavigationRequestRef p_request);
typedef void (*MCBrowserNavigationCallback)(void *p_context, MCBrowserRef p_browser, MCBrowserNavigationEventType p_type, MCBrowserNavigationState p_state, bool p_in_frame, const char *p_url, const char *p_error);
typedef void (*MCBrowserJavaScriptCallback)(void *p_context, MCBrowserRef p_browser, const char *p_handler, MCBrowserListRef p_params);
typedef void (*MCBrowserProgressCallback)(void *p_context, MCBrowserRef p_browser, const char *p_url, uint32_t p_progress);

MC_BROWSER_DLLEXPORT bool MCBrowserSetNavigationRequestHandler(MCBrowserRef p_browser, MCBrowserNavigationRequestCallback p_callback, void *p_context);
MC_BROWSER_DLLEXPORT bool MCBrowserSetNavigationHandler(MCBrowserRef p_browser, MCBrowserNavigationCallback p_callback, void *p_context);
MC_BROWSER_DLLEXPORT bool MCBrowserSetJavaScriptHandler(MCBrowserRef p_browser, MCBrowserJavaScriptCallback p_callback, void *p_context);
MC_BROWSER_DLLEXPORT bool MCBrowserSetProgressHandler(MCBrowserRef p_browser, MCBrowserProgressCallback p_callback, void *p_context);

}

////////////////////////////////////////////////////////////////////////////////

#endif//__LIBBROWSER_H__
