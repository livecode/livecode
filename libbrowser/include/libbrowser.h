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

#include "core.h"

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
	virtual void OnJavaScriptCall(MCBrowser *p_browser, const char *p_handler, uint32_t p_arg_count, const char **p_args) = 0;
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
	MCBrowser() {}
	virtual ~MCBrowser() {}
	
	void SetEventHandler(MCBrowserEventHandler *p_handler);
	void SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler);
	
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
	
protected:
	void OnNavigationBegin(bool p_in_frame, const char *p_url);
	void OnNavigationComplete(bool p_in_frame, const char *p_url);
	void OnNavigationFailed(bool p_in_frame, const char *p_url, const char *p_error);
	void OnDocumentLoadBegin(bool p_in_frame, const char *p_url);
	void OnDocumentLoadComplete(bool p_in_frame, const char *p_url);
	void OnDocumentLoadFailed(bool p_in_frame, const char *p_url, const char *p_error);
	
	void OnJavaScriptCall(const char *p_handler, uint32_t p_arg_count, const char **p_args);
	
private:
	MCBrowserEventHandler *m_event_handler;
	MCBrowserJavaScriptHandler *m_javascript_handler;
};

// Browser factory interface
class MCBrowserFactory : public MCBrowserRefCounted
{
public:
	MCBrowserFactory() {}
	virtual ~MCBrowserFactory() {}
	
	virtual bool CreateBrowser(MCBrowser *&r_browser) = 0;
};

// C API

extern "C"
{
	
bool MCBrowserLibraryInitialize();
void MCBrowserLibraryFinalize();
	
	typedef void *(MCBrowserAllocator)(size_t p_size);
	typedef void (MCBrowserDeallocator)(void *p_mem);
	
	void MCBrowserLibrarySetAllocator(MCBrowserAllocator p_alloc);
	void MCBrowserSetDeallocator(MCBrowserDeallocator p_dealloc);

typedef struct __MCBrowser *MCBrowserRef;
typedef struct __MCBrowserFactory *MCBrowserFactoryRef;

bool MCBrowserFactoryGet(const char *p_factory_id, MCBrowserFactoryRef &r_factory);
bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, MCBrowserRef &r_browser);

MCBrowserRef MCBrowserRetain(MCBrowserRef p_browser);
void MCBrowserRelease(MCBrowserRef p_browser);

void *MCBrowserGetNativeLayer(MCBrowserRef p_browser);

bool MCBrowserGetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool &r_value);
bool MCBrowserSetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool p_value);

bool MCBrowserGetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, char *&r_utf8_string);
bool MCBrowserSetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, const char *p_utf8_string);

bool MCBrowserGoBack(MCBrowserRef p_browser);
bool MCBrowserGoForward(MCBrowserRef p_browser);
bool MCBrowserGoToURL(MCBrowserRef p_browser);
bool MCBrowserEvaluateJavaScript(MCBrowserRef p_browser, const char *p_script, char *&r_result);

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
typedef void (*MCBrowserJavaScriptCallback)(void *p_context, MCBrowserRef p_browser, const char *p_handler, uint32_t p_arg_count,const char **p_args);

bool MCBrowserSetRequestHandler(MCBrowserRef p_browser, MCBrowserRequestCallback p_callback, void *p_context);
bool MCBrowserSetJavaScriptHandler(MCBrowserRef p_browser, MCBrowserJavaScriptCallback p_callback, void *p_context);

}

#endif//__LIBBROWSER_H__