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

#include <core.h>

#include <stdlib.h>
#include <memory.h>

#include <libbrowser.h>
#include "libbrowser_internal.h"

////////////////////////////////////////////////////////////////////////////////
// Browser base implementation

MCBrowserRefCounted::MCBrowserRefCounted()
{
	m_ref_count = 0;
}

void MCBrowserRefCounted::Retain()
{
	m_ref_count++;
}

void MCBrowserRefCounted::Release()
{
	if (--m_ref_count > 1)
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

void MCBrowserBase::SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler)
{
	if (p_handler)
		p_handler->Retain();
	
	if (m_javascript_handler)
		m_javascript_handler->Release();
	
	m_javascript_handler = p_handler;
}

void MCBrowserBase::OnJavaScriptCall(const char *p_handler, MCBrowserListRef p_params)
{
	if (m_javascript_handler)
		m_javascript_handler->OnJavaScriptCall(this, p_handler, p_params);
}

////////////////////////////////////////////////////////////////////////////////

// Init functions

static MCBrowserFactoryRef s_browser_factory = nil;

bool MCBrowserLibraryInitialize()
{
	return true;
}

void MCBrowserLibraryFinalize()
{
	if (s_browser_factory)
		delete (MCBrowserFactory*)s_browser_factory;
}

//////////

// Factory

extern bool MCCefBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);
bool MCBrowserFactoryGet(const char *p_factory, MCBrowserFactoryRef &r_factory)
{
	if (s_browser_factory == nil && !MCCefBrowserFactoryCreate(s_browser_factory))
		return false;
	
	r_factory = s_browser_factory;
	return true;
}

bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, void *p_display, void *p_parent_window, MCBrowserRef &r_browser)
{
	if (p_factory == nil)
		return false;
	
	MCBrowser *t_browser;
	if (!((MCBrowserFactory*)p_factory)->CreateBrowser(p_display, p_parent_window, t_browser))
		return false;
	
	r_browser = (MCBrowserRef)t_browser;
	return true;
}

//////////

MCBrowserRef MCBrowserRetain(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return nil;
	
	((MCBrowser*)p_browser)->Retain();
	return p_browser;
}

void MCBrowserRelease(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return;
	
	((MCBrowser*)p_browser)->Release();
}

//////////

void *MCBrowserGetNativeLayer(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return nil;
	
	return ((MCBrowser*)p_browser)->GetNativeLayer();
}

bool MCBrowserGetRect(MCBrowserRef p_browser, MCBrowserRect &r_rect)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GetRect(r_rect);
}

bool MCBrowserSetRect(MCBrowserRef p_browser, const MCBrowserRect &p_rect)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->SetRect(p_rect);
}

bool MCBrowserGetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool &r_value)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GetBoolProperty(p_property, r_value);
}

bool MCBrowserSetBoolProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, bool p_value)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->SetBoolProperty(p_property, p_value);
}

bool MCBrowserGetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, char *&r_value)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GetStringProperty(p_property, r_value);
}

bool MCBrowserSetStringProperty(MCBrowserRef p_browser, MCBrowserProperty p_property, const char *p_value)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->SetStringProperty(p_property, p_value);
}

bool MCBrowserGoBack(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GoBack();
}

bool MCBrowserGoForward(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GoForward();
}

bool MCBrowserGoToURL(MCBrowserRef p_browser, const char *p_url)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->GoToURL(p_url);
}

bool MCBrowserEvaluateJavaScript(MCBrowserRef p_browser, const char *p_script, char *&r_result)
{
	if (p_browser == nil)
		return false;
	
	return ((MCBrowser*)p_browser)->EvaluateJavaScript(p_script, r_result);
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
	
	virtual void OnNavigationBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateBegin, p_in_frame, p_url, nil);
	}
	
	virtual void OnNavigationComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateComplete, p_in_frame, p_url, nil);
	}
	
	virtual void OnNavigationFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeNavigate, kMCBrowserRequestStateFailed, p_in_frame, p_url, p_error);
	}
	
	virtual void OnDocumentLoadBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateBegin, p_in_frame, p_url, nil);
	}
	
	virtual void OnDocumentLoadComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateComplete, p_in_frame, p_url, nil);
	}
	
	virtual void OnDocumentLoadFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, kMCBrowserRequestTypeDocumentLoad, kMCBrowserRequestStateFailed, p_in_frame, p_url, p_error);
	}
	
private:
	MCBrowserRequestCallback m_callback;
	void *m_context;
};

bool MCBrowserSetRequestHandler(MCBrowserRef p_browser, MCBrowserRequestCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;
	
	if (p_callback == nil)
	{
		((MCBrowser*)p_browser)->SetEventHandler(nil);
		return true;
	}
	
	MCBrowserEventHandlerWrapper *t_wrapper;
	t_wrapper = new MCBrowserEventHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	((MCBrowser*)p_browser)->SetEventHandler(t_wrapper);
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
	
	virtual void OnJavaScriptCall(MCBrowser *p_browser, const char *p_handler, MCBrowserListRef p_params) override
	{
		if (m_callback)
			m_callback(m_context, (MCBrowserRef)p_browser, p_handler, p_params);
	}
	
private:
	MCBrowserJavaScriptCallback m_callback;
	void *m_context;
};

bool MCBrowserSetJavaScriptHandler(MCBrowserRef p_browser, MCBrowserJavaScriptCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;
	
	if (p_callback == nil)
	{
		((MCBrowser*)p_browser)->SetJavaScriptHandler(nil);
		return true;
	}
	
	MCBrowserJavaScriptHandlerWrapper *t_wrapper;
	t_wrapper = new MCBrowserJavaScriptHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	((MCBrowser*)p_browser)->SetJavaScriptHandler(t_wrapper);
	return true;
}

//////////

static MCBrowserWaitFunction s_browser_wait_func = nil;

extern "C" void MCBrowserLibrarySetWaitFunction(MCBrowserWaitFunction p_wait)
{
	s_browser_wait_func = p_wait;
}

////////////////////////////////////////////////////////////////////////////////

struct MCBrowserRunloopAction
{
	MCBrowserRunloopCallback callback;
	void *context;
	
	uint32_t references;
	
	MCBrowserRunloopAction *next;
};

static MCBrowserRunloopAction *s_runloop_actions = nil;
static uint32_t s_runloop_action_count = 0;

//////////

void MCBrowserLibraryRunloopCallback(void *p_context)
{
	for (MCBrowserRunloopAction *t_action = s_runloop_actions; t_action != nil; t_action = t_action->next)
		t_action->callback(t_action->context);
}

extern "C" bool MCBrowserLibraryGetRunloopCallback(MCBrowserRunloopCallback &r_callback, void *&r_context)
{
	r_callback = MCBrowserLibraryRunloopCallback;
	r_context = nil;
	
	return true;
}

//////////

bool MCBrowserFindRunloopAction(MCBrowserRunloopCallback p_callback, void *p_context, MCBrowserRunloopAction *&r_action)
{
	for (MCBrowserRunloopAction *t_action = s_runloop_actions; t_action != nil; t_action = t_action->next)
		if (t_action->callback == p_callback && t_action->context == p_context)
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
	
	t_action->next = s_runloop_actions;
	s_runloop_actions = t_action;
	
	return true;
}

void MCBrowserRemoveRunloopAction(MCBrowserRunloopCallback p_callback, void *p_context)
{
	MCBrowserRunloopAction *t_action = nil;
	if (!MCBrowserFindRunloopAction(p_callback, p_context, t_action))
		return;
	
	if (t_action->references-- > 1)
		return;
	
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

////////////////////////////////////////////////////////////////////////////////
