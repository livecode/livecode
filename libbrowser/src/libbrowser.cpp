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
#include <memory.h>

#include <libbrowser.h>
#include "libbrowser_internal.h"

////////////////////////////////////////////////////////////////////////////////
// Browser base implementation

MCBrowserRefCounted::MCBrowserRefCounted()
{
	m_ref_count = 1;
}

void MCBrowserRefCounted::Retain()
{
	m_ref_count++;
}

void MCBrowserRefCounted::Release()
{
	if (m_ref_count-- > 1)
		return;
	
	Destroy();
}

void MCBrowserRefCounted::Destroy()
{
	delete this;
}

////////////////////////////////////////////////////////////////////////////////

void MCBrowserBase::SetEventHandler(MCBrowserEventHandler *p_handler)
{
	if (p_handler)
		p_handler->Retain();
	
	if (m_event_handler)
		m_event_handler->Release();
	
	m_event_handler = p_handler;
}

MCBrowserEventHandler *MCBrowserBase::GetEventHandler(void)
{
	return m_event_handler;
}

void MCBrowserBase::OnNavigationBegin(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnNavigationBegin(this, p_in_frame, p_url);
}

void MCBrowserBase::OnNavigationComplete(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnNavigationComplete(this, p_in_frame, p_url);
}

void MCBrowserBase::OnNavigationFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	if (m_event_handler)
		m_event_handler->OnNavigationFailed(this, p_in_frame, p_url, p_error);
}

void MCBrowserBase::OnDocumentLoadBegin(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadBegin(this, p_in_frame, p_url);
}

void MCBrowserBase::OnDocumentLoadComplete(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadComplete(this, p_in_frame, p_url);
}

void MCBrowserBase::OnDocumentLoadFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadFailed(this, p_in_frame, p_url, p_error);
}

void MCBrowserBase::OnNavigationRequestUnhandled(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnNavigationRequestUnhandled(this, p_in_frame, p_url);
}

void MCBrowserBase::SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler)
{
	if (p_handler)
		p_handler->Retain();
	
	if (m_javascript_handler)
		m_javascript_handler->Release();
	
	m_javascript_handler = p_handler;
}

MCBrowserJavaScriptHandler *MCBrowserBase::GetJavaScriptHandler(void)
{
	return m_javascript_handler;
}

void MCBrowserBase::OnJavaScriptCall(const char *p_handler, MCBrowserListRef p_params)
{
	if (m_javascript_handler)
		m_javascript_handler->OnJavaScriptCall(this, p_handler, p_params);
}

//////////

MCBrowserBase::MCBrowserListEntry *MCBrowserBase::s_browser_list = nil;

bool MCBrowserBase::BrowserListAdd(MCBrowser *p_browser)
{
	MCBrowserListEntry *t_entry;
	if (!MCBrowserMemoryNew(t_entry))
		return false;
	
	t_entry->browser = p_browser;
	t_entry->next = s_browser_list;
	
	s_browser_list = t_entry;
	
	return true;
}

void MCBrowserBase::BrowserListRemove(MCBrowser *p_browser)
{
	MCBrowserListEntry *t_entry;
	MCBrowserListEntry *t_previous = nil;

	for (t_entry = s_browser_list; t_entry != nil; t_entry = t_entry->next)
	{
		if (t_entry->browser == p_browser)
			break;
		t_previous = t_entry;
	}
	
	if (t_entry == nil)
		return;

	if (t_entry == s_browser_list)
		s_browser_list = t_entry->next;
	else
		t_previous->next = t_entry->next;
	
	MCBrowserMemoryDelete(t_entry);
}

bool MCBrowserBase::BrowserListIterate(MCBrowserIterateCallback p_callback, void *p_context)
{
	for (MCBrowserListEntry *t_entry = s_browser_list; t_entry != nil; t_entry = t_entry->next)
	{
		if (!p_callback(t_entry->browser, p_context))
			return false;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// Init functions

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserLibraryInitialize()
{
	return true;
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibraryFinalize()
{
	// IM-2016-03-10: [[ Bug 17029 ]] Delete any instantiated browser factories.
	if (s_factory_list != nil)
	{
		for (uint32_t i = 0; s_factory_list[i].factory_id != nil; i++)
		{
			if (s_factory_list[i].instance != nil)
			{
				delete s_factory_list[i].instance;
				s_factory_list[i].instance = nil;
			}
		}
	}
}

//////////

// Factory
bool MCBrowserFactoryEnsureAvailable(MCBrowserFactoryMap &p_map, MCBrowserFactoryRef &r_instance)
{
	if (p_map.instance != nil)
	{
		r_instance = p_map.instance;
		return true;
	}
	
	if (p_map.constructor == nil)
		return false;
	
	if (!p_map.constructor(p_map.instance))
		return false;
	
	r_instance = p_map.instance;
	return true;
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserFactoryGet(const char *p_factory, MCBrowserFactoryRef &r_factory)
{
    if (s_factory_list == nil)
        return false; // no browser factories available;
	
	if (p_factory == nil || MCCStringIsEmpty(p_factory) || MCCStringEqualCaseless(p_factory, "default"))
	{
		// use first available browser factory
		for (uint32_t i = 0; s_factory_list[i].factory_id != nil; i++)
		{
			if (MCBrowserFactoryEnsureAvailable(s_factory_list[i], r_factory))
				return true;
		}
        
        // no browser factories were available
        return false;
	}
	
	for (uint32_t i = 0; s_factory_list[i].factory_id != nil; i++)
		if (MCCStringEqualCaseless(p_factory, s_factory_list[i].factory_id))
			return MCBrowserFactoryEnsureAvailable(s_factory_list[i], r_factory);

	return false;
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, void *p_display, void *p_parent_window, MCBrowserRef &r_browser)
{
	if (p_factory == nil)
		return false;
	
	MCBrowser *t_browser;
	if (!p_factory->CreateBrowser(p_display, p_parent_window, t_browser))
		return false;
	
	r_browser = t_browser;
	return true;
}

//////////

MC_BROWSER_DLLEXPORT_DEF
MCBrowserRef MCBrowserRetain(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return nil;
	
	p_browser->Retain();
	return p_browser;
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserRelease(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return;
	
	p_browser->Release();
}

//////////

MC_BROWSER_DLLEXPORT_DEF
void *MCBrowserGetNativeLayer(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return nil;
	
	return p_browser->GetNativeLayer();
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGetRect(MCBrowserRef p_browser, MCBrowserRect &r_rect)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GetRect(r_rect);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetRect(MCBrowserRef p_browser, const MCBrowserRect &p_rect)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->SetRect(p_rect);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool &r_value)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GetBoolProperty(p_property, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool p_value)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->SetBoolProperty(p_property, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, char *&r_value)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GetStringProperty(p_property, r_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, const char *p_value)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->SetStringProperty(p_property, p_value);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGoBack(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GoBack();
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGoForward(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GoForward();
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserGoToURL(MCBrowserRef p_browser, const char *p_url)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->GoToURL(p_url);
}

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserEvaluateJavaScript(MCBrowserRef p_browser, const char *p_script, char *&r_result)
{
	if (p_browser == nil)
		return false;
	
	return p_browser->EvaluateJavaScript(p_script, r_result);
}

//////////

// Event handler c++ wrapper
class MCBrowserEventHandlerWrapper : public MCBrowserEventHandler
{
public:
	MCBrowserEventHandlerWrapper(MCBrowserRequestCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}
	
	virtual void OnNavigationBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateBegin, p_in_frame, p_url, nil);
	}
	
	virtual void OnNavigationComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateComplete, p_in_frame, p_url, nil);
	}
	
	virtual void OnNavigationFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateFailed, p_in_frame, p_url, p_error);
	}
	
	virtual void OnDocumentLoadBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateBegin, p_in_frame, p_url, nil);
	}
	
	virtual void OnDocumentLoadComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateComplete, p_in_frame, p_url, nil);
	}
	
	virtual void OnDocumentLoadFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateFailed, p_in_frame, p_url, p_error);
	}
	
	virtual void OnNavigationRequestUnhandled(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (m_callback)
			m_callback(m_context, p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateUnhandled, p_in_frame, p_url, nil);
	}
	
private:
	MCBrowserRequestCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetRequestHandler(MCBrowserRef p_browser, MCBrowserRequestCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;
	
	if (p_callback == nil)
	{
		p_browser->SetEventHandler(nil);
		return true;
	}
	
	MCBrowserEventHandlerWrapper *t_wrapper;
	t_wrapper = new MCBrowserEventHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	p_browser->SetEventHandler(t_wrapper);
	return true;
}

//////////

// JavaScript handler c++ wrapper
class MCBrowserJavaScriptHandlerWrapper : public MCBrowserJavaScriptHandler
{
public:
	MCBrowserJavaScriptHandlerWrapper(MCBrowserJavaScriptCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}
	
	virtual void OnJavaScriptCall(MCBrowser *p_browser, const char *p_handler, MCBrowserListRef p_params)
	{
		if (m_callback)
			m_callback(m_context, p_browser, p_handler, p_params);
	}
	
private:
	MCBrowserJavaScriptCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetJavaScriptHandler(MCBrowserRef p_browser, MCBrowserJavaScriptCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;
	
	if (p_callback == nil)
	{
		p_browser->SetJavaScriptHandler(nil);
		return true;
	}
	
	MCBrowserJavaScriptHandlerWrapper *t_wrapper;
	t_wrapper = new MCBrowserJavaScriptHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	p_browser->SetJavaScriptHandler(t_wrapper);
	return true;
}

//////////

static MCBrowserWaitFunction s_browser_wait_func = nil;

extern "C" MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibrarySetWaitFunction(MCBrowserWaitFunction p_wait)
{
	s_browser_wait_func = p_wait;
}

static MCBrowserBreakWaitFunction s_browser_breakwait_func = nil;

extern "C" MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibrarySetBreakWaitFunction(MCBrowserBreakWaitFunction p_breakwait)
{
	s_browser_breakwait_func = p_breakwait;
}

////////////////////////////////////////////////////////////////////////////////

struct MCBrowserRunloopAction
{
	MCBrowserRunloopCallback callback;
	void *context;
	
	uint32_t references;
	bool remove;
	
	MCBrowserRunloopAction *next;
};

static MCBrowserRunloopAction *s_runloop_actions = nil;
static uint32_t s_in_runloop_callback = 0;

//////////

void MCBrowserLibraryRunloopCallback(void *p_context)
{
	s_in_runloop_callback++;
	
	MCBrowserRunloopAction **t_current_ptr;
	t_current_ptr = &s_runloop_actions;
	
	while (*t_current_ptr != nil)
	{
		MCBrowserRunloopAction *t_action;
		t_action = *t_current_ptr;
		
		if (t_action->remove)
		{
			// perform deferred delete.
			*t_current_ptr = t_action->next;
			
			MCBrowserMemoryDelete(t_action);
		}
		else
		{
			t_action->callback(t_action->context);
			t_current_ptr = &t_action->next;
		}
	}
	
	s_in_runloop_callback--;
}

extern "C" MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserLibraryGetRunloopCallback(MCBrowserRunloopCallback &r_callback, void *&r_context)
{
	r_callback = MCBrowserLibraryRunloopCallback;
	r_context = nil;
	
	return true;
}

//////////

bool MCBrowserFindRunloopAction(MCBrowserRunloopCallback p_callback, void *p_context, MCBrowserRunloopAction *&r_action)
{
	for (MCBrowserRunloopAction *t_action = s_runloop_actions; t_action != nil; t_action = t_action->next)
		if (t_action->callback == p_callback && t_action->context == p_context && !t_action->remove)
		{
			r_action = t_action;
			return true;
		}
	
	return false;
}

bool MCBrowserAddRunloopAction(MCBrowserRunloopCallback p_callback, void *p_context)
{
	MCBrowserRunloopAction *t_action = nil;
	if (MCBrowserFindRunloopAction(p_callback, p_context, t_action))
	{
		t_action->references++;
		return true;
	}
	
	if (!MCBrowserMemoryNew(t_action))
		return false;
	
	t_action->callback = p_callback;
	t_action->context = p_context;
	t_action->references = 1;
	t_action->remove = false;
	
	t_action->next = s_runloop_actions;
	s_runloop_actions = t_action;
	
	return true;
}

void MCBrowserRemoveRunloopAction(MCBrowserRunloopCallback p_callback, void *p_context)
{
	MCBrowserRunloopAction *t_action = nil;
	if (!MCBrowserFindRunloopAction(p_callback, p_context, t_action))
		return;
	
	if (t_action->remove)
		return;
	
	if (t_action->references-- > 1)
		return;
	
	// defer delete if called from within the browser runloop callback
	if (s_in_runloop_callback)
	{
		t_action->remove = true;
		return;
	}
	
	if (t_action == s_runloop_actions)
		s_runloop_actions = t_action->next;
	else
	{
		for (MCBrowserRunloopAction *t_prev = s_runloop_actions; t_prev != nil; t_prev = t_prev->next)
		{
			if (t_prev->next == t_action)
			{
				t_prev->next = t_action->next;
				break;
			}
		}
	}
	
	MCBrowserMemoryDelete(t_action);
}

////////////////////////////////////////////////////////////////////////////////

bool MCBrowserRunloopWait()
{
	if (s_browser_wait_func)
		return s_browser_wait_func();
	
	return true;
}

void MCBrowserRunloopBreakWait()
{
	MCLog("break wait", nil);
	if (s_browser_breakwait_func)
		s_browser_breakwait_func();
}

////////////////////////////////////////////////////////////////////////////////
