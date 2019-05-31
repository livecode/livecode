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

MCBrowserBase::MCBrowserBase(void)
    : m_event_handler(nil),
      m_javascript_handler(nil),
      m_progress_handler(nil),
	  m_file_dialog_handler(nil),
	  m_download_request_handler(nil),
	  m_download_progress_handler(nil),
	  m_file_dialog_have_response(false),
	  m_download_request_have_response(false)
{
	MCBrowserMemoryClear(m_file_dialog_response);
	MCBrowserMemoryClear(m_download_request_response);
}

MCBrowserBase::~MCBrowserBase(void)
{
    if (m_event_handler)
        m_event_handler->Release();
    
    if (m_javascript_handler)
        m_javascript_handler->Release();

	if (m_progress_handler)
		m_progress_handler->Release();

	if (m_file_dialog_handler)
		m_file_dialog_handler->Release();

	if (m_download_request_handler)
		m_download_request_handler->Release();

	if (m_download_progress_handler)
		m_download_progress_handler->Release();

	FileDialogClearResponse();
	DownloadClearResponse();
}

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

void MCBrowserBase::SetProgressHandler(MCBrowserProgressHandler *p_handler)
{
	if (p_handler != nil)
		p_handler->Retain();

	if (m_progress_handler != nil)
		m_progress_handler->Release();

	m_progress_handler = p_handler;
}

MCBrowserProgressHandler *MCBrowserBase::GetProgressHandler(void)
{
	return m_progress_handler;
}

void MCBrowserBase::OnProgressChanged(const char *p_url, uint32_t p_progress)
{
	if (m_progress_handler != nil)
		m_progress_handler->OnProgressChanged(this, p_url, p_progress);
}

//////////

void MCBrowserBase::SetFileDialogHandler(MCBrowserFileDialogHandler *p_handler)
{
	if (p_handler != nil)
		p_handler->Retain();

	if (m_file_dialog_handler != nil)
		m_file_dialog_handler->Release();

	m_file_dialog_handler = p_handler;
}

MCBrowserFileDialogHandler *MCBrowserBase::GetFileDialogHandler(void)
{
	return m_file_dialog_handler;
}

bool MCBrowserBase::OnFileDialog(
	MCBrowserFileDialogType p_type,
	MCBrowserFileDialogOptions p_options,
	const char *p_title,
	const char *p_default_path,
	const char *p_filters,
	uindex_t p_default_filter
)
{
	if (m_file_dialog_handler)
		return m_file_dialog_handler->OnFileDialog(this, p_type, p_options, p_title, p_default_path, p_filters, p_default_filter);

	return false;
}

//////////

void MCBrowserBase::SetDownloadRequestHandler(MCBrowserDownloadRequestHandler *p_handler)
{
	if (p_handler != nil)
		p_handler->Retain();

	if (m_download_request_handler != nil)
		m_download_request_handler->Release();

	m_download_request_handler = p_handler;
}

MCBrowserDownloadRequestHandler *MCBrowserBase::GetDownloadRequestHandler(void)
{
	return m_download_request_handler;
}

bool MCBrowserBase::OnDownloadRequest(const char *p_url, const char *p_suggested_name)
{
	if (m_download_request_handler != nil)
		return m_download_request_handler->OnDownloadRequest(this, p_url, p_suggested_name);

	return false;
}

//////////

void MCBrowserBase::SetDownloadProgressHandler(MCBrowserDownloadProgressHandler *p_handler)
{
	if (p_handler != nil)
		p_handler->Retain();

	if (m_download_progress_handler != nil)
		m_download_progress_handler->Release();

	m_download_progress_handler = p_handler;
}

MCBrowserDownloadProgressHandler *MCBrowserBase::GetDownloadProgressHandler(void)
{
	return m_download_progress_handler;
}

void MCBrowserBase::OnDownloadProgress(const char *p_url, MCBrowserDownloadState p_state, uint32_t p_bytes_received, int32_t p_total_bytes)
{
	if (m_download_progress_handler != nil)
		m_download_progress_handler->OnDownloadProgress(this, p_url, p_state, p_bytes_received, p_total_bytes);
}

//////////

void MCBrowserBase::FileDialogClearResponse(void)
{
	if (!m_file_dialog_have_response)
		return;

	m_file_dialog_response.cancelled = false;
	if (m_file_dialog_response.selected_paths != nil)
		MCCStringFree(const_cast<char*>(m_file_dialog_response.selected_paths));
	m_file_dialog_response.selected_paths = nil;
	m_file_dialog_response.selected_filter = 0;

	m_file_dialog_have_response = false;
}

bool MCBrowserBase::FileDialogGetResponse(MCBrowserFileDialogResponse &r_response)
{
	if (!m_file_dialog_have_response)
		return false;

	r_response = m_file_dialog_response;
	return true;
}

void MCBrowserBase::FileDialogCancel(void)
{
	if (m_file_dialog_have_response)
		return;

	m_file_dialog_response.cancelled = true;
	m_file_dialog_have_response = true;
}

void MCBrowserBase::FileDialogSelectPaths(const char *p_paths, uindex_t p_selected_filter)
{
	if (m_file_dialog_have_response)
		return;

	char *t_paths = nil;
	/* UNCHECKED */ MCCStringClone(p_paths, t_paths);
	m_file_dialog_response.selected_paths = t_paths;
	m_file_dialog_response.selected_filter = p_selected_filter;
	m_file_dialog_have_response = true;
}

//////////

void MCBrowserBase::DownloadClearResponse()
{
	if (!m_download_request_have_response)
		return;

	m_download_request_response.cancelled = false;
	if (m_download_request_response.save_path)
		MCCStringFree(const_cast<char*>(m_download_request_response.save_path));
	m_download_request_response.save_path = nil;

	m_download_request_have_response = false;
}

bool MCBrowserBase::DownloadGetResponse(MCBrowserDownloadRequestResponse &r_response)
{
	if (!m_download_request_have_response)
		return false;

	r_response = m_download_request_response;
	return true;
}

void MCBrowserBase::DownloadCancel()
{
	if (m_download_request_have_response)
		return;

	m_download_request_response.cancelled = true;

	m_download_request_have_response = true;
}

void MCBrowserBase::DownloadContinueWithSavePath(const char *p_save_path)
{
	if (m_download_request_have_response)
		return;

	char *t_save_path = nil;
	/* UNCHECKED */ MCCStringClone(p_save_path, t_save_path);
	m_download_request_response.save_path = t_save_path;

	m_download_request_have_response = true;
}

void MCBrowserBase::DownloadContinueWithSaveDialog(void)
{
	if (m_download_request_have_response)
		return;

	m_download_request_have_response = true;
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

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserFileDialogCancel(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return;

	p_browser->FileDialogCancel();
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserFileDialogSelectPaths(MCBrowserRef p_browser, const char *p_paths, uindex_t p_selected_filter)
{
	if (p_browser == nil)
		return;

	p_browser->FileDialogSelectPaths(p_paths, p_selected_filter);
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserDownloadCancel(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return;

	p_browser->DownloadCancel();
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserDownloadContinueWithSavePath(MCBrowserRef p_browser, const char *p_save_path)
{
	if (p_browser == nil)
		return;

	p_browser->DownloadContinueWithSavePath(p_save_path);
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserDownloadContinueWithSaveDialog(MCBrowserRef p_browser)
{
	if (p_browser == nil)
		return;

	p_browser->DownloadContinueWithSaveDialog();
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
	t_wrapper = new (nothrow) MCBrowserEventHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	p_browser->SetEventHandler(t_wrapper);
    
    t_wrapper->Release();
    
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
	t_wrapper = new (nothrow) MCBrowserJavaScriptHandlerWrapper(p_callback, p_context);
	
	if (t_wrapper == nil)
		return false;
	
	p_browser->SetJavaScriptHandler(t_wrapper);
    
    t_wrapper->Release();
    
	return true;
}

//////////

class MCBrowserProgressHandlerWrapper : public MCBrowserProgressHandler
{
public:
	MCBrowserProgressHandlerWrapper(MCBrowserProgressCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}

	virtual void OnProgressChanged(MCBrowser *p_browser, const char *p_url, uint32_t p_progress)
	{
		if (m_callback)
			m_callback(m_context, p_browser, p_url, p_progress);
	}

private:
	MCBrowserProgressCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetProgressHandler(MCBrowserRef p_browser, MCBrowserProgressCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;

	if (p_callback == nil)
	{
		p_browser->SetProgressHandler(nil);
		return true;
	}

	MCBrowserProgressHandlerWrapper *t_wrapper;
	t_wrapper = new (nothrow) MCBrowserProgressHandlerWrapper(p_callback, p_context);

	if (t_wrapper == nil)
		return false;

	p_browser->SetProgressHandler(t_wrapper);

	t_wrapper->Release();

	return true;
}

//////////

class MCBrowserFileDialogHandlerWrapper : public MCBrowserFileDialogHandler
{
public:
	MCBrowserFileDialogHandlerWrapper(MCBrowserFileDialogCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}

	virtual bool OnFileDialog(
		MCBrowser *p_browser,
		MCBrowserFileDialogType p_type,
		MCBrowserFileDialogOptions p_options,
		const char *p_title,
		const char *p_default_path,
		const char *p_filters,
		uindex_t p_default_filter
	)
	{
		if (m_callback)
			return m_callback(m_context, p_browser, p_type, p_options, p_title, p_default_path, p_filters, p_default_filter);

		return false;
	}

private:
	MCBrowserFileDialogCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetFileDialogHandler(MCBrowserRef p_browser, MCBrowserFileDialogCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;

	if (p_callback == nil)
	{
		p_browser->SetFileDialogHandler(nil);
		return true;
	}

	MCBrowserFileDialogHandlerWrapper *t_wrapper;
	t_wrapper = new (nothrow) MCBrowserFileDialogHandlerWrapper(p_callback, p_context);

	if (t_wrapper == nil)
		return false;

	p_browser->SetFileDialogHandler(t_wrapper);

	t_wrapper->Release();

	return true;
}

//////////

class MCBrowserDownloadRequestWrapper : public MCBrowserDownloadRequestHandler
{
public:
	MCBrowserDownloadRequestWrapper(MCBrowserDownloadRequestCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}

	virtual bool OnDownloadRequest(MCBrowser *p_browser, const char *p_url, const char *p_suggested_name)
	{
		if (m_callback)
			return m_callback(m_context, p_browser, p_url, p_suggested_name);

		return false;
	}

private:
	MCBrowserDownloadRequestCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetDownloadRequestHandler(MCBrowserRef p_browser, MCBrowserDownloadRequestCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;

	if (p_callback == nil)
	{
		p_browser->SetDownloadRequestHandler(nil);
		return true;
	}

	MCBrowserDownloadRequestWrapper *t_wrapper;
	t_wrapper = new (nothrow) MCBrowserDownloadRequestWrapper(p_callback, p_context);

	if (t_wrapper == nil)
		return false;

	p_browser->SetDownloadRequestHandler(t_wrapper);

	t_wrapper->Release();

	return true;
}

//////////

class MCBrowserDownloadProgressWrapper : public MCBrowserDownloadProgressHandler
{
public:
	MCBrowserDownloadProgressWrapper(MCBrowserDownloadProgressCallback p_callback, void *p_context)
	{
		m_callback = p_callback;
		m_context = p_context;
	}

	virtual void OnDownloadProgress(MCBrowser *p_browser, const char *p_url, MCBrowserDownloadState p_state, uint32_t p_bytes_downloaded, int32_t p_total_bytes)
	{
		if (m_callback)
			m_callback(m_context, p_browser, p_url, p_state, p_bytes_downloaded, p_total_bytes);
	}

private:
	MCBrowserDownloadProgressCallback m_callback;
	void *m_context;
};

MC_BROWSER_DLLEXPORT_DEF
bool MCBrowserSetDownloadProgressHandler(MCBrowserRef p_browser, MCBrowserDownloadProgressCallback p_callback, void *p_context)
{
	if (p_browser == nil)
		return false;

	if (p_callback == nil)
	{
		p_browser->SetDownloadProgressHandler(nil);
		return true;
	}

	MCBrowserDownloadProgressWrapper *t_wrapper;
	t_wrapper = new (nothrow) MCBrowserDownloadProgressWrapper(p_callback, p_context);

	if (t_wrapper == nil)
		return false;

	p_browser->SetDownloadProgressHandler(t_wrapper);

	t_wrapper->Release();

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
	if (s_browser_breakwait_func)
		s_browser_breakwait_func();
}

////////////////////////////////////////////////////////////////////////////////
