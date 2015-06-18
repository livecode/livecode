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

#include "libbrowser.h"

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

void MCBrowser::SetEventHandler(MCBrowserEventHandler *p_handler)
{
	if (p_handler)
		p_handler->Retain();
	
	if (m_event_handler)
		m_event_handler->Release();
	
	m_event_handler = p_handler;
}

void MCBrowser::OnNavigationBegin(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnNavigationBegin(this, p_in_frame, p_url);
}

void MCBrowser::OnNavigationComplete(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnNavigationComplete(this, p_in_frame, p_url);
}

void MCBrowser::OnNavigationFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	if (m_event_handler)
		m_event_handler->OnNavigationFailed(this, p_in_frame, p_url, p_error);
}

void MCBrowser::OnDocumentLoadBegin(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadBegin(this, p_in_frame, p_url);
}

void MCBrowser::OnDocumentLoadComplete(bool p_in_frame, const char *p_url)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadComplete(this, p_in_frame, p_url);
}

void MCBrowser::OnDocumentLoadFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	if (m_event_handler)
		m_event_handler->OnDocumentLoadFailed(this, p_in_frame, p_url, p_error);
}

void MCBrowser::SetJavaScriptHandler(MCBrowserJavaScriptHandler *p_handler)
{
	if (p_handler)
		p_handler->Retain();
	
	if (m_javascript_handler)
		m_javascript_handler->Release();
	
	m_javascript_handler = p_handler;
}

void MCBrowser::OnJavaScriptCall(const char *p_handler, MCBrowserListRef p_params)
{
	if (m_javascript_handler)
		m_javascript_handler->OnJavaScriptCall(this, p_handler, p_params);
}

// Init functions

static MCBrowserFactoryRef s_browser_factory = nil;

extern bool MCCefBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);
bool MCBrowserLibraryInitialize()
{
	s_browser_factory = nil;
	return MCCefBrowserFactoryCreate(s_browser_factory);
}

void MCBrowserLibraryFinalize()
{
	if (s_browser_factory)
		delete (MCBrowserFactory*)s_browser_factory;
}

// Factory

bool MCBrowserFactoryGet(const char *p_factory, MCBrowserFactoryRef &r_factory)
{
	// TODO - platform specific implementation
	if (s_browser_factory == nil)
		return false;
	
	r_factory = s_browser_factory;
	return true;
}

bool MCBrowserFactoryCreateBrowser(MCBrowserFactoryRef p_factory, MCBrowserRef &r_browser)
{
	if (p_factory == nil)
		return false;
	
	MCBrowser *t_browser;
	if (!((MCBrowserFactory*)p_factory)->CreateBrowser(t_browser))
		return false;
	
	r_browser = (MCBrowserRef)t_browser;
	return true;
}

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
