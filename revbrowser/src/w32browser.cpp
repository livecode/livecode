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

#include "core.h"

#include "w32browser.h"
#include <activscp.h>
#include <revolution/support.h>

///////////////////////////////////////////////////////////////////////////////

int CWebBrowser::browseridcounter= 121;
HRESULT LoadWebBrowserFromStream(IWebBrowser2* pWebBrowser, HGLOBAL pText);
HRESULT LoadWebBrowserFromStreamDo(IDispatch *pHtmlDoc, HGLOBAL pText);
extern HINSTANCE theInstance;

char *BStrToChar(const BSTR bstr)
{
	int len = SysStringLen(bstr);
	char *str = (char *)malloc(len+1);
	str[len] = 0;
	WideCharToMultiByte(CP_ACP, NULL, bstr,-1,str, len, NULL,NULL);
	return str;
}

BSTR CharToBstr(const char *p_string)
{
	int t_required_buffer_size;
	t_required_buffer_size = MultiByteToWideChar(CP_ACP, NULL, p_string, -1, NULL, 0);

	BSTR t_output_string;

	t_output_string = SysAllocStringLen(NULL, t_required_buffer_size);
	if (t_output_string == NULL)
		return NULL;

	MultiByteToWideChar(CP_ACP, NULL, p_string, -1, t_output_string, t_required_buffer_size);
	return t_output_string;
}

typedef unsigned char uint1;
typedef unsigned short uint2;
typedef unsigned int uint4;

struct MYBITMAP
{
	uint1 *data;
	uint2 width;
	uint2 height;
	uint1 depth;
	uint4 bytes_per_line;
	HBITMAP bm;
};

MYBITMAP *createmybitmap(uint2 depth, uint2 width, uint2 height)
{
  MYBITMAP *image = new (nothrow) MYBITMAP;
  image->width = width;
  image->height = height;
  image->depth = (uint1)depth;
  image->bytes_per_line = ((width * depth + 31) >> 3) & 0xFFFFFFFC;
  image->data = NULL;
  image->bm = NULL;
  BITMAPINFO *bmi = NULL;
  bmi = (BITMAPINFO *)new char[sizeof(BITMAPV4HEADER)];
  memset(bmi, 0, sizeof(BITMAPV4HEADER));
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biCompression = BI_BITFIELDS;
  BITMAPV4HEADER *b4 = (BITMAPV4HEADER *)bmi;
  b4->bV4RedMask = 0xFF0000;
  b4->bV4GreenMask = 0xFF00;
  b4->bV4BlueMask = 0xFF;
  b4->bV4AlphaMask = 0xFF000000;
  bmi->bmiHeader.biWidth = width;
  bmi->bmiHeader.biHeight = -height;
  bmi->bmiHeader.biPlanes = 1;
  bmi->bmiHeader.biBitCount = depth;
  bmi->bmiHeader.biSizeImage = image->bytes_per_line * image->height;
  image->bm = CreateDIBSection(GetDC(NULL), bmi, DIB_RGB_COLORS,
	                       (void **)&image->data, NULL, 0);
  if (image->data == NULL || image->bm == NULL)
    image->data = (uint1 *)new char[image->bytes_per_line * image->height];
  delete bmi;
  return image;
}
	
void destroymybitmap(MYBITMAP *image)
{
  if (image->bm == NULL)
    delete image->data;
  else
    DeleteObject(image->bm);   
  delete image;
}

///////////////////////////////////////////////////////////////////////////////

HHOOK CWebBrowser::s_message_hook = NULL;
HHOOK CWebBrowser::s_window_hook = NULL;
CWebBrowser *CWebBrowser::s_browsers = NULL;
CWebBrowser *CWebBrowser::s_focused_browser = NULL;

CWebBrowserWindow::CWebBrowserWindow(void)
{
	m_browser = NULL;
	m_mouse_inside = false;
}

LRESULT CWebBrowserWindow::OnDestroy(UINT p_message, WPARAM p_wparam, LPARAM p_lparam, BOOL& r_handled)
{
	if (m_browser != NULL)
		m_browser -> WindowDestroyed();
	return DefWindowProc(p_message, p_wparam, p_lparam);
}

CWebBrowser::CWebBrowser(HWND hparent,  BOOL isvisible)
{ 
	m_in_callback = false;
	dummybrowser = NULL;
	hostwindow = hparent;
	browserid = browseridcounter++;
	browsertype = BT_IE;
	iwebbrowser2 = 0;
	zoom=3;
	instID=0;
	webui = NULL;
	contextmenuenabled = FALSE;
	scrollbarsenabled = TRUE;
	scaleenabled = TRUE;
	borderenabled = FALSE;
	allownewwindow = FALSE;
	advancedMessages = FALSE;
	inited = FALSE;
	DWORD flags = WS_CHILD;
	if (isvisible)
		flags |= WS_VISIBLE;

	browserwindow.Create(hostwindow, CWindow::rcDefault,
		browsertype == BT_IE? _T("{8856F961-340A-11D0-A96B-00C04FD705A2}") : _T("{1339B54C-3453-11D2-93B9-000000000000}"),
		flags,0,browserid);

	browserwindowsubclass . SetBrowser(this);
	browserwindowsubclass . SubclassWindow(browserwindow);

	HRESULT hr = AtlAxGetControl(browserwindow.m_hWnd,&iunknown);
	if (SUCCEEDED(hr)) 
		iunknown->QueryInterface(IID_IWebBrowser2,(void**)&iwebbrowser2); 
	if (!iwebbrowser2) return;
	browserevents = new (nothrow) CWebEvents(this); 
	browserevents->AddRef();
	webui = new (nothrow) CWebUI(this);
	webui->AddRef();
	browserwindow.SetExternalUIHandler(webui);
	AtlAdvise(iwebbrowser2, browserevents, DIID_DWebBrowserEvents2,
		&browserevents2_cookie);

	CComQIPtr<IOleInPlaceActiveObject> spl(iwebbrowser2);
	m_spIOleInPlaceActiveObject = spl;

	HRESULT mr = iwebbrowser2->get_HWND((SHANDLE_PTR *)&lhwnd);

	document_events = NULL;
	document_events_cookie = 0;

	m_current_find = NULL;

	inited = TRUE;

	if (s_message_hook == NULL)
		s_message_hook = SetWindowsHookExA(WH_GETMESSAGE, GetMessageHook, NULL, GetCurrentThreadId());
	if (s_window_hook == NULL)
		s_window_hook = SetWindowsHookExA(WH_CALLWNDPROC, CallWindowProcHook, NULL, GetCurrentThreadId());

	next = s_browsers;
	s_browsers = this;
}

CWebBrowser::~CWebBrowser()
{

	if (s_browsers == this)
		s_browsers = next;
	else
		for(CWebBrowser *t_browser = s_browsers; t_browser != NULL; t_browser = t_browser -> next)
			if (t_browser -> next == this)
				t_browser -> next = next;

	if (s_focused_browser == this)
		s_focused_browser = NULL;

	if (s_browsers == NULL)
	{
		UnhookWindowsHookEx(s_message_hook);
		s_message_hook = NULL;

		UnhookWindowsHookEx(s_window_hook);
		s_window_hook = NULL;
	}

	if (s_focused_browser == this)
		s_focused_browser = NULL;

	if (browserwindow.m_hWnd!=0) browserwindow.DestroyWindow();
	inited = FALSE;

}

LRESULT CALLBACK CWebBrowser::GetMessageHook(int p_code, WPARAM p_wparam, LPARAM p_lparam)
{
	if (p_code >= 0)
	{
		MSG *t_message;
		t_message = (MSG *)p_lparam;
		if (s_focused_browser != NULL && t_message -> message >= WM_KEYFIRST && t_message -> message <= WM_KEYLAST)
		{
			if (s_focused_browser -> m_spIOleInPlaceActiveObject -> TranslateAccelerator(t_message) == S_OK)
				t_message -> message = WM_NULL;
		}
	}

	return CallNextHookEx(s_message_hook, p_code, p_wparam, p_lparam);
}

LRESULT CALLBACK CWebBrowser::CallWindowProcHook(int p_code, WPARAM p_wparam, LPARAM p_lparam)
{
	if (p_code >= 0)
	{
		CWPSTRUCT *t_message = (CWPSTRUCT *)p_lparam;

		if (t_message -> message == WM_KILLFOCUS)
			s_focused_browser = NULL;
		else if (t_message -> message == WM_SETFOCUS)
		{
			HWND t_window;
			t_window = t_message -> hwnd;

			CWebBrowser *t_new_browser;
			for(t_new_browser = s_browsers; t_new_browser != NULL; t_new_browser = t_new_browser -> next)
			{
				HWND t_parent;
				for(t_parent = t_window; t_parent != NULL; t_parent = GetParent(t_parent))
					if (t_parent == t_new_browser -> browserwindow)
						break;

				if (t_parent != NULL)
					break;
			}

			s_focused_browser = t_new_browser;
		}
	}

	return CallNextHookEx(s_window_hook, p_code, p_wparam, p_lparam);
}


void CWebBrowser::WindowDestroyed(void)
{
	browserwindow . Detach();
	inited = FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
//  COMMAND HANDLING
//

void CWebBrowser::GoURL(const char *URL, const char *target_frame)
{
	if (m_in_callback)
		return;

	if (!inited) return;
	CComVariant vtEmpty;
	CComVariant zlevel(zoom);
	CComVariant vtURL(URL);

	if (target_frame == NULL)
		iwebbrowser2->Navigate2(&vtURL, &vtEmpty, &vtEmpty, &vtEmpty, &vtEmpty);
	else
	{
		CComVariant t_frame(target_frame);
		iwebbrowser2->Navigate2(&vtURL, &vtEmpty, &t_frame, &vtEmpty, &vtEmpty);
	}
	
	iwebbrowser2->get_HWND((SHANDLE_PTR *)&lhwnd);

}

void CWebBrowser::GoForward() 
{
	if (m_in_callback)
		return;

	if (!inited) return;
	iwebbrowser2->GoForward();
}


void CWebBrowser::GoBack() 
{
	if (m_in_callback)
		return;

	if (!inited) return;
	iwebbrowser2->GoBack();
}


void CWebBrowser::Stop() 
{
	if (m_in_callback)
		return;

	if (!inited) return;
	iwebbrowser2->Stop();
}


void CWebBrowser::Refresh() 
{
	if (m_in_callback)
		return;

	if (!inited) return;
	CComVariant va(3);
	iwebbrowser2->Refresh2(&va);
}

void CWebBrowser::Focus(void)
{
	s_focused_browser = this;
}

void CWebBrowser::Unfocus(void)
{
	s_focused_browser = NULL;
}

void CWebBrowser::SetScale(bool willscale)
{
	if( willscale )
	{
		scaleenabled = TRUE;
		iwebbrowser2->put_Resizable(VARIANT_TRUE);
	}
	else
	{
		scaleenabled = FALSE;
		iwebbrowser2->put_Resizable(VARIANT_FALSE);
	}
}

bool CWebBrowser::GetScale()
{
	return scaleenabled == TRUE;
}

void CWebBrowser::MakeTextSmaller()
{
	if (m_in_callback)
		return;

	CComVariant zlevel(zoom);
	if( zoom > 1 )
	{
		iwebbrowser2->ExecWB(OLECMDID_ZOOM,OLECMDEXECOPT_DONTPROMPTUSER,(VARIANT *)&zlevel,NULL);
		zoom = zoom -1;
	}
}

void CWebBrowser::Redraw()
{
	browserwindow.SetRedraw();
}

void CWebBrowser::Print()
{
	iwebbrowser2->ExecWB(OLECMDID_PRINT, OLECMDEXECOPT_DODEFAULT, NULL,NULL);
}



void CWebBrowser::MakeTextBigger()
{
	if (m_in_callback)
		return;

	CComVariant zlevel(zoom);
	if( zoom < 5)
	{
		iwebbrowser2->ExecWB(OLECMDID_ZOOM,OLECMDEXECOPT_DONTPROMPTUSER,(VARIANT *)&zlevel,NULL);
		zoom = zoom + 1;
	}
}

char *CWebBrowser::GetTitle()
{
	CComBSTR  bstrTitle;
	iwebbrowser2->get_LocationName(&bstrTitle);
	return BStrToChar(bstrTitle);
}


// Calls the specified active script function, passing the supplied arguments. Returns the function's value.
char *CWebBrowser::CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count)
{
	HRESULT t_result;
	t_result = S_OK;

	char *t_return_string;
	t_return_string = NULL;

	IHTMLDocument2 *t_document;
	t_document = GetHtmlDocument();
	if (t_document == NULL)
		t_result = S_FALSE;

	IHTMLWindow2 *t_window;
	t_window = NULL;
	if (t_result == S_OK)
		t_window = GetHtmlWindow(t_document);

	if (t_window == NULL)
		t_result = S_FALSE;

	IDispatch *t_script;
	t_script = NULL;
	if (t_result == S_OK)
		t_result = t_document -> get_Script(&t_script);

	LPOLESTR t_ole_method;
	t_ole_method = NULL;
	if (t_result == S_OK)
	{
		t_ole_method = CharToBstr(p_function_name);
		if (t_ole_method == NULL)
			t_result = E_OUTOFMEMORY;
	}

	DISPID t_method_id;
	if (t_result == S_OK)
		t_result = t_script ->GetIDsOfNames(IID_NULL, &t_ole_method, 1, LOCALE_NEUTRAL, &t_method_id);

	VARIANTARG *t_ole_arguments;
	t_ole_arguments = NULL;
	if (t_result == S_OK)
	{
		t_ole_arguments = new (nothrow) VARIANTARG[p_argument_count];
		if (t_ole_arguments != NULL)
			memset(t_ole_arguments, 0, sizeof(VARIANTARG) * p_argument_count);
		else
			t_result = E_OUTOFMEMORY;
	}

	if (t_result == S_OK)
	{
		for(unsigned int i = 0; i < p_argument_count; ++i)
		{
			VariantInit(&t_ole_arguments[i]);
			t_ole_arguments[i] . vt = VT_BSTR;
			t_ole_arguments[i] . bstrVal = CharToBstr(p_arguments[p_argument_count - i - 1]);
			if (t_ole_arguments[i] . bstrVal == NULL)
				t_result = E_OUTOFMEMORY;
		}
	}

	char *t_return_value;
	t_return_value = NULL;
	if (t_result == S_OK)
	{
		DISPPARAMS t_params;
		t_params . rgvarg = t_ole_arguments;
		t_params . rgdispidNamedArgs = NULL;
		t_params . cArgs = p_argument_count;
		t_params . cNamedArgs = 0;

		VARIANT t_function_result;
		VariantInit(&t_function_result);
		t_result = t_script -> Invoke(t_method_id, IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &t_params, &t_function_result, NULL, NULL);
		if (t_result == S_OK)
		{
			t_result = VariantChangeType(&t_function_result, &t_function_result, VARIANT_ALPHABOOL, VT_BSTR);
			if (t_result == S_OK)
			{
				t_return_value = BStrToChar(t_function_result . bstrVal);
				if (t_return_value == NULL)
					t_result = E_OUTOFMEMORY;
			}
			VariantClear(&t_function_result);
		}
	}

	if (t_ole_arguments != NULL)
	{
		for(unsigned int i = 0; i < p_argument_count; ++i)
			VariantClear(&t_ole_arguments[i]);

		delete t_ole_arguments;
	}

	if (t_ole_method != NULL)
		 SysFreeString(t_ole_method);

	if (t_script != NULL)
		t_script -> Release();

	if (t_window != NULL)
		t_window -> Release();

	if (t_document != NULL)
		t_document -> Release();

	return t_return_value;
}

IHTMLDocument2 *CWebBrowser::GetHtmlDocument(void)
{
	bool t_success;
	t_success = true;

	HRESULT t_result;
	
	char *t_return_string;
	t_return_string = NULL;

	// First we obtain the "active" document's dispatch interface, this will give us access to its
	// scripting capabilities.
	IDispatch *t_dispatch;
	t_dispatch = NULL;
	if (t_success)
	{
		t_result = iwebbrowser2 -> get_Document(&t_dispatch);
		t_success = (SUCCEEDED(t_result));
	}

	if (t_dispatch == NULL || t_result == S_FALSE)
	{
		// According to the Microsoft docs at http://msdn.microsoft.com/en-us/library/aa752116(VS.85).aspx,
		// If the document is not "safe for scripting", then null will be returned here, dispite the result being
		// set to S_OK. Alternatively with IE7 and later, t_result will equal S_FALSE in this case.
		return NULL;
	}

	// Get the document to give us a pointer to its IHTMLDocument2 interface. We can use this to obtain
	// access to the document's parent window.
	IHTMLDocument2 *t_document;
	t_document = NULL;
	if (t_success)
	{
		t_result = t_dispatch -> QueryInterface(IID_IHTMLDocument2, (void **)&t_document);
		t_success = (SUCCEEDED(t_result));
	}

	t_dispatch -> Release();

	if (t_success)
		return t_document;
	else
		return NULL;
}

IHTMLWindow2 *CWebBrowser::GetHtmlWindow(IHTMLDocument2 *p_document)
{
	if (p_document == NULL)
		return NULL;

	// Get the parent window, this interface should allow us to execute scripts in the document.
	IHTMLWindow2 *t_window;
	t_window = NULL;
	
	HRESULT t_result;
	t_result = p_document -> get_parentWindow(&t_window);
	if (!SUCCEEDED(t_result))
		t_window = NULL;

	return t_window;
}

IHTMLWindow2 *CWebBrowser::GetHtmlWindow(void)
{
	IHTMLDocument2 *t_document;
	t_document = GetHtmlDocument();
	if (t_document == NULL)
		return NULL;

	// Get the parent window, this interface should allow us to execute scripts in the document.
	IHTMLWindow2 *t_window;
	t_window = GetHtmlWindow(t_document);
	
	t_document -> Release();

	return t_window;
}

// Executes the specified script and returns the value of the "result" global variable, allowing users to return values
char *CWebBrowser::ExecuteScript(const char *p_javascript_string)
{
	bool t_success;
	t_success = true;

	HRESULT t_result;
	
	char *t_return_string;
	t_return_string = NULL;

	// Get the document to give us a pointer to its IHTMLDocument2 interface. We can use this to obtain
	// access to the document's parent window, and its scripting object.
	IHTMLDocument2 *t_document;
	t_document = NULL;
	if (t_success)
		t_document = GetHtmlDocument();

	if (t_document == NULL)
	{
		t_return_string = strdup("can't run script");
		t_success = false;
	}

	// Get the HTML window interface, we need this in order too execute the script.
	IHTMLWindow2 *t_window;
	t_window = NULL;
	if (t_success)
		t_window = GetHtmlWindow(t_document);

	if (t_window == NULL)
	{
		t_return_string = strdup("can't run script");
		t_success = false;
	}

	// Convert the javascript code into the correct format for the API
	BSTR t_code;
	t_code = NULL;
	if (t_success)
		t_code = CharToBstr(p_javascript_string);

	if (t_code == NULL)
	{
		t_return_string = strdup("out of memory");
		t_success = false;
	}

	// Execute the code
	VARIANT t_script_return;
	VariantInit(&t_script_return);
	if (t_success)
	{
		t_result = t_window -> execScript(t_code, L"JScript", &t_script_return);
		t_success = (SUCCEEDED(t_result));
	}

	// Get the document's scripting environment, we need this in order to retrieve the value of the "result" global variable.
	IDispatch *t_script;
	t_script = NULL;
	if (t_success)
	{
		t_result = t_document -> get_Script(&t_script);
		t_success = (SUCCEEDED(t_result));
	}

	// Lookup the ID of the "result" variable
	DISPID t_var_id;
	LPOLESTR t_ole_var;
	t_ole_var = OLESTR("result");
	if (t_success)
		t_result = t_script -> GetIDsOfNames(IID_NULL, &t_ole_var, 1, LOCALE_NEUTRAL, &t_var_id);
	
	char *t_return_value;
	t_return_value = NULL;
	if (t_result == S_OK && t_success)
	{
		DISPPARAMS t_params;
		t_params.cArgs = 0;
		t_params.cNamedArgs = 0;
		t_params.rgdispidNamedArgs = NULL;
		t_params.rgvarg = NULL;

		VARIANT t_function_result;
		VariantInit(&t_function_result);
		t_result = t_script -> Invoke(t_var_id, IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &t_params, &t_function_result, NULL, NULL);
		if (t_result == S_OK)
		{
			t_result = VariantChangeType(&t_function_result, &t_function_result, VARIANT_ALPHABOOL, VT_BSTR);
			if (t_result == S_OK)
			{
				t_return_value = string_from_utf16((const unsigned short *)t_function_result . bstrVal, SysStringLen(t_function_result . bstrVal));
				if (t_return_value == NULL)
				{
					t_result = E_OUTOFMEMORY;
					t_success = false;
				}
			}
			VariantClear(&t_function_result);
		}
	}
	else
		t_return_value = strdup("");

	if (!t_success)
	{
		if (t_return_value != NULL)
		{
			delete t_return_value;
			t_return_value = NULL;
		}
	}

	if (t_code != NULL)
		SysFreeString(t_code);

	if (t_script != NULL)
		t_script -> Release();

	if (t_window != NULL)
		t_window -> Release();

	if (t_document != NULL)
		t_document -> Release();

	VariantClear(&t_script_return);

	return t_return_value;
}

bool CWebBrowser::FindString(const char *p_string, bool p_search_up)
{
	return false;
}


BOOL CWebBrowser::Draw(int twidth,int theight, HDC hdcMem)
{
	if (!inited) 
		return FALSE;

	HRESULT hr;
	IViewObject2* pViewObject = NULL;
	hr = iwebbrowser2->QueryInterface(IID_IViewObject2, (void**)&pViewObject);
	if (FAILED(hr)) return FALSE;
	HDC hdcMain = GetDC(NULL);
	RECTL rcBounds = { 0, 0, twidth, theight };
	DWORD freezeindex;
	pViewObject->Freeze(DVASPECT_CONTENT, -1,NULL, &freezeindex);
	hr = pViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, hdcMain, hdcMem, 
		&rcBounds, NULL, NULL, 0);
	pViewObject->Unfreeze(freezeindex);
	pViewObject->Release();
	ReleaseDC(NULL,hdcMain);
	if (SUCCEEDED(hr))
		return TRUE;
	return FALSE;
}

BOOL CWebBrowser::Draw( HDC hdcMem)
{
	IHTMLElementRender * pRender;
	IHTMLDocument2* pDoc;
	IHTMLElement* pBodyElem = 0;
	IDispatch * pdisp;
	HRESULT hr;

	if( SUCCEEDED(iwebbrowser2->get_Document(&pdisp)) )
	{
		if( pdisp != NULL )
		{
			hr = pdisp->QueryInterface(IID_IHTMLDocument2,(void **)&pDoc);
			if( hr != S_OK )
				return FALSE;
		}
		// got a doc lets get the element
		HRESULT hr = pDoc->get_body(&pBodyElem);
		if (SUCCEEDED(hr)) {
			hr = pBodyElem->QueryInterface(IID_IHTMLElementRender, (void**)&pRender);
			if (SUCCEEDED(hr)) {
				hr = pRender->DrawToDC(hdcMem);
			}
			pRender->Release();
		}
		pBodyElem->Release();
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
//  PROPERTY HANDLING
//

bool CWebBrowser::GetBusy()
{
	if (!inited) return FALSE;
	VARIANT_BOOL vBool;
	iwebbrowser2->get_Busy(&vBool);
	return vBool == VARIANT_TRUE;
}

bool CWebBrowser::GetOffline()
{
	if (!inited) return FALSE;
	VARIANT_BOOL vBool;
	iwebbrowser2->get_Offline(&vBool);
	return vBool == VARIANT_TRUE;
}

void CWebBrowser::SetOffline(bool isoffline)
{
	if (m_in_callback)
		return;

	if (!inited) return;
	iwebbrowser2->put_Offline(isoffline == TRUE? VARIANT_TRUE: VARIANT_FALSE);
}

bool CWebBrowser::GetContextMenu()
{
	if (!inited) return false;
	return contextmenuenabled == TRUE;
}

void CWebBrowser::SetContextMenu(bool isenabled)
{
	contextmenuenabled = isenabled;
}

void CWebBrowser::SetNewWindow(bool isenabled)
{
	allownewwindow = isenabled;
}

bool CWebBrowser::GetNewWindow()
{
	return allownewwindow == TRUE;
}

void CWebBrowser::SetScrollbars(bool usescrollbars)
{
	if (m_in_callback)
		return;

	scrollbarsenabled = usescrollbars;
	iwebbrowser2->Refresh();
}

void CWebBrowser::SetMessages(bool getsMessages)
{
	advancedMessages = getsMessages;
}

bool CWebBrowser::GetMessages()
{
	return advancedMessages == TRUE;
}

bool CWebBrowser::GetScrollbars()
{
	return scrollbarsenabled == TRUE;
}

void CWebBrowser::SetBorder(bool showborder)
{
	if (m_in_callback)
		return;

	borderenabled = showborder;
	iwebbrowser2->Refresh();
}

bool CWebBrowser::GetBorder()
{
	return borderenabled == TRUE;
}

void CWebBrowser::SetVisible(bool isvisible)
{
	if (!inited) return;
	browserwindow.ShowWindow(isvisible == TRUE? SW_SHOW:SW_HIDE);
}

bool CWebBrowser::GetVisible()
{
	if (!inited) return false;
	return browserwindow.IsWindowVisible() == TRUE;
}

void CWebBrowser::SetRect(int p_left, int p_top, int p_right, int p_bottom)
{
	if (!inited) return;

	browserwindow.MoveWindow(p_left, p_top, p_right - p_left, p_bottom - p_top, FALSE);

	iwebbrowser2->get_HWND((SHANDLE_PTR *)&lhwnd);
}

void CWebBrowser::GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom)
{
	RECT r;
	if (!inited) return;
	browserwindow.GetWindowRect(&r);
	POINT p;
	p.x = r.left;
	p.y = r.top;
	browserwindow.ScreenToClient(&r);
	ScreenToClient(hostwindow,&p);
	r.left = p.x;
	r.top = p.y;
	r.right += p.x;
	r.bottom += p.y;

	r_left = r . left;
	r_top = r . top;
	r_right = r . right;
	r_bottom = r . bottom;
}

char *CWebBrowser::GetURL()
{
	if (!inited) return NULL;
	CComBSTR bstrURL;
	if (SUCCEEDED(iwebbrowser2->get_LocationURL(&bstrURL)))
		return BStrToChar(bstrURL);
	else  return NULL;
}


void CWebBrowser::SetBrowser(const char *browser)
{
	if (m_in_callback)
		return;

	BrowserType t_new_type;

	if (_strnicmp(browser, "IE", strlen(browser)) == 0)
		t_new_type = BT_IE;
	else
		t_new_type = BT_NETSCAPE;

	if (t_new_type != browsertype)
	{
		browsertype = t_new_type;
		swapBrowser();
	}
}

char *CWebBrowser::GetBrowser()
{
	if(!inited) return NULL;
	CComBSTR bstrReturn;
	if( browsertype == BT_IE)
		bstrReturn = "IE";
	else
		bstrReturn = "Netscape";

	return BStrToChar(bstrReturn);
}

// Returns
//   A pointer to an object that represents the body of the active document in the browser.
//   Returns NULL if failed.
// Description
//   Use this to obtain access to the body element of the active document. Note that the returned
//   object must be freed by the caller when no longer needed.
IHTMLElement2 *CWebBrowser::GetBodyElement(void)
{
	bool t_success;
	t_success = true;

	HRESULT t_result;

	IDispatch *t_dispatch;
	if (t_success)
	{
		t_result = iwebbrowser2 -> get_Document(&t_dispatch);
		t_success = (SUCCEEDED(t_result));
	}

	IHTMLDocument2 *t_document;
	if (t_success)
	{
		t_result = t_dispatch -> QueryInterface(IID_IHTMLDocument2, (void **)&t_document);
		t_success = (SUCCEEDED(t_result));
	}

	IHTMLElement *t_body;
	if (t_success)
	{
		t_result = t_document -> get_body(&t_body);
		t_success = (SUCCEEDED(t_result));
	}

	IHTMLElement2 *t_body_2;
	t_body_2 = NULL;
	if (t_success)
	{
		t_result = t_body -> QueryInterface(IID_IHTMLElement2, (void **)&t_body_2);
		t_success = (SUCCEEDED(t_result));
	}

	if (t_body != NULL)
		t_body -> Release();

	if (t_document != NULL)
		t_document -> Release();

	if (t_dispatch != NULL)
		t_dispatch -> Release();

	return t_body_2;
}


// Returns
//   The horizontal scroll extent of the browser in pixels
//   Returns 0 if unable to get value.
int CWebBrowser::GetHScroll(void)
{
	IHTMLElement2 *t_body;
	t_body = GetBodyElement();
	if (t_body == NULL)
		return 0;

	long t_scroll;
	t_body -> get_scrollLeft(&t_scroll);
	t_body -> Release();

	return (int)t_scroll;
}


// Returns
//   The vertical scroll extent of the browser in pixels
//   Returns 0 if unable to get value.
int CWebBrowser::GetVScroll(void)
{
	IHTMLElement2 *t_body;
	t_body = GetBodyElement();
	if (t_body == NULL)
		return 0;

	long t_scroll;
	t_body -> get_scrollTop(&t_scroll);
	t_body -> Release();

	return (int)t_scroll;
}


// Parameters
//   p_hscroll_pixels : Value in pixels to set the horizontal scroll to
// Description
//   Sets the horizontal scroll extent of the browser to p_hscroll_pixels.
//   If p_hscroll_pixels < 0 then the scroll is set to zero.
//   If p_hscroll_pixels > MAXSCROLL then sets the scroll to MAXSCROLL
void CWebBrowser::SetHScroll(int p_hscroll_pixels)
{
	IHTMLWindow2 *t_window;
	t_window = GetHtmlWindow();

	if (t_window != NULL)
	{
		t_window -> scrollTo(p_hscroll_pixels, GetVScroll());
		t_window -> Release();
	}
}

// Parameters
//   p_vscroll_pixels : Value in pixels to set the vertical scroll to
// Description
//   Sets the vertical scroll extent of the browser to p_vscroll_pixels.
//   If p_vscroll_pixels < 0 then the scroll is set to zero.
//   If p_vscroll_pixels > MAXSCROLL then sets the scroll to MAXSCROLL
void CWebBrowser::SetVScroll(int p_vscroll_pixels)
{
	IHTMLWindow2 *t_window;
	t_window = GetHtmlWindow();

	if (t_window != NULL)
	{
		t_window -> scrollTo(GetHScroll(), p_vscroll_pixels);
		t_window -> Release();
	}
}

// Returns
//   The height of the active document in the browser, in pixels
//   Returns 0 if unable to get value.
int CWebBrowser::GetFormattedHeight(void)
{
	IHTMLElement2 *t_body;
	t_body = GetBodyElement();
	if (t_body == NULL)
		return 0;

	long t_height;
	t_body -> get_scrollHeight(&t_height);

	t_body -> Release();

	return t_height;
}

// Returns
//   The width of the active document in the browser, in pixels
//   Returns 0 if unable to get value.
int CWebBrowser::GetFormattedWidth(void)
{
	IHTMLElement2 *t_body;
	t_body = GetBodyElement();
	if (t_body == NULL)
		return 0;

	long t_width;
	t_body -> get_scrollWidth(&t_width);

	t_body -> Release();

	return t_width;
}

void CWebBrowser::GetFormattedRect(int &r_left, int &r_top, int &r_right, int &r_bottom)
{
	int t_browser_left, t_browser_top, t_browser_right, t_browser_bottom;
	GetRect(t_browser_left, t_browser_top, t_browser_right, t_browser_bottom);

	r_left = t_browser_left - GetHScroll();
	r_top = t_browser_top - GetVScroll();
	r_right = r_left + GetFormattedWidth();
	r_bottom = r_top + GetFormattedHeight();
}

void CWebBrowser::SetSelectedText(const char * selText )
{
	if (m_in_callback)
		return;

	IHTMLDocument2* pDoc;
	IHTMLElement* pBodyElem = 0;
	IDispatch * pdisp;
	HRESULT hr;

	if (strlen(selText) == 0)
	{
		if (m_current_find != NULL)
		{
			m_current_find -> Release();
			m_current_find = NULL;
		}

		return;
	}

	if (m_current_find != NULL)
	{
		HRESULT hr;
		VARIANT_BOOL bSuccess;
		CComBSTR sText = selText;
		hr = m_current_find -> findText(sText, INT_MAX, 0, &bSuccess);
		if (SUCCEEDED(hr) && bSuccess)
		{
			m_current_find -> select();
			m_current_find -> collapse(VARIANT_FALSE);
		}
		else
		{
			m_current_find -> Release();
			m_current_find = NULL;
		}
	}
	
	if (m_current_find == NULL && SUCCEEDED(iwebbrowser2->get_Document(&pdisp)))
	{
		if (pdisp != NULL)
		{
			hr = pdisp->QueryInterface(IID_IHTMLDocument2,(void **)&pDoc);
			if (hr != S_OK)
				return;
		}
		// got a doc lets get the element
		HRESULT hr = pDoc->get_body(&pBodyElem);
		if (SUCCEEDED(hr)) {
			IHTMLBodyElement* pBody = 0;
			hr = pBodyElem->QueryInterface(IID_IHTMLBodyElement, (void**)&pBody);
			if (SUCCEEDED(hr)) {
				IHTMLTxtRange* pTextRange = 0;
				hr = pBody->createTextRange(&pTextRange);
				if (SUCCEEDED(hr)) {
					CComBSTR sText = selText;
					VARIANT_BOOL bSuccess;
					hr = pTextRange->findText(sText, 0, 0, &bSuccess);
					if (bSuccess == VARIANT_TRUE)
					{
						pTextRange->select();
						m_current_find = pTextRange;
						m_current_find -> AddRef();
						m_current_find -> collapse(VARIANT_FALSE);
					}
					else
					{
						IHTMLSelectionObject *t_selection;
						t_selection = NULL;
						pDoc -> get_selection(&t_selection);
						if (t_selection != NULL)
						{
							t_selection -> empty();
							t_selection -> Release();
						}
					}

					pTextRange->Release();
				}
				pBody->Release();
			}
			pBodyElem->Release();
		}

		pDoc->Release();
	}
}

char *CWebBrowser::GetSelectedText()
{
	IDispatch * pdisp;
	IHTMLDocument2 * pdoc;
	IHTMLSelectionObject* pSelection = 0;
	HRESULT  hr;

	IWebBrowser2 * whichbrowser = iwebbrowser2;
	if( SUCCEEDED(whichbrowser->get_Document(&pdisp)) )
	{
		if( pdisp != NULL )
		{
			hr = pdisp->QueryInterface(IID_IHTMLDocument2,(void **)&pdoc);
			if( hr == 0 )
			{
				hr = pdoc->get_selection(&pSelection);
				if (SUCCEEDED(hr)) {
					IDispatch* pDispRange = 0;
					hr = pSelection->createRange(&pDispRange);
					if (SUCCEEDED(hr)) {
						IHTMLTxtRange* pTextRange = 0;
						hr = pDispRange->QueryInterface(IID_IHTMLTxtRange, (void**)&pTextRange);
						if (SUCCEEDED(hr)) {
							CComBSTR sText;
							pTextRange->get_text(&sText);
							char * retval = BStrToChar((BSTR)sText);
							pTextRange->Release();
							return retval;
					 }
						pDispRange->Release();
				 }
					pSelection->Release();
			 }
				pdisp->Release();
			}
		}
	}
	return strdup("");
}


void CWebBrowser::SetSource(const char * hsource)
{
	if (m_in_callback)
		return;

	HGLOBAL hHtmlText;
	hHtmlText = GlobalAlloc(GPTR, strlen(hsource) + 1);
	if( hHtmlText )
	{
		strcpy((char *)hHtmlText, hsource);
		LoadWebBrowserFromStream(iwebbrowser2, hHtmlText);
	}
	else
		GlobalFree(hHtmlText);
}

char *CWebBrowser::GetSource()
{
	return BStrToChar(htmlsource);
}

bool CWebBrowser::GetImage(void*& r_data, int& r_length)
{
	bool t_success;
	t_success = false;

	int t_right, t_left, t_bottom, t_top;
	GetRect(t_left, t_top, t_right, t_bottom);

	int twidth = t_right - t_left;
	int theight = t_bottom - t_top;

	HDC desktophdc,desthdc;	
	MYBITMAP *browserimage = createmybitmap(32, twidth,theight);
	if (browserimage)
	{
		//copy from cardpixmap to image pixmap (which  maps to bit buffer)
		desktophdc = GetDC(NULL);
		desthdc = CreateCompatibleDC(desktophdc);
		ReleaseDC(NULL, desktophdc);
		HBITMAP odbm = (HBITMAP)SelectObject(desthdc, browserimage->bm);
		// MW-2014-04-30: [[ Bug 12210 ]] Use IViewObject to render rather than DrawToDC as the latter
		//   has been deprecated in IE9+.
		BOOL res = Draw(twidth, theight, desthdc);
		SelectObject(desthdc, odbm);
		DeleteDC(desthdc);
		if (res)
		{
			uint1 * pixArray = browserimage->data;
			for(unsigned int j=0;j<(browserimage->bytes_per_line*browserimage->height);j=j+4)
			{
				DWORD * pix = (DWORD *)pixArray+3;
				*(pixArray + (j+3)) = *(pixArray+j);

				uint1  tmp = *(pixArray+(j+2));
				*(pixArray+(j+2)) =  *(pixArray+j+1);
				*(pixArray+(j+1)) = tmp;
				*(pixArray+j)= 0;
			}
			r_length = browserimage -> bytes_per_line * browserimage -> height;
			r_data = malloc(r_length);
			memcpy(r_data, browserimage -> data, r_length);
			t_success = true;
		}
		destroymybitmap(browserimage);
	}

	return t_success;
}

uintptr_t CWebBrowser::GetWindowId(void)
{
	return uintptr_t(hostwindow);
}

void CWebBrowser::SetWindowId(uintptr_t p_new_id)
{
	HWND t_new_window;
	t_new_window = (HWND)p_new_id;
	if (p_new_id == 0 || !IsWindow(t_new_window))
	{
		wasvisible = IsWindowVisible(browserwindow);
		browserwindow . ShowWindow(SW_HIDE);
		browserwindow . SetParent(NULL);
		hostwindow = NULL;
	}
	else
	{
		browserwindow . SetParent(t_new_window);
		if (wasvisible)
			browserwindow . ShowWindow(SW_SHOW);
		hostwindow = t_new_window;
	}
}

char *CWebBrowser::GetUserAgent(void)
{
	return strdup("");
}

void CWebBrowser::SetUserAgent(const char *p_user_agent)
{
}

//////////

void CWebBrowser::AddJavaScriptHandler(const char *p_handler)
{
}

void CWebBrowser::RemoveJavaScriptHandler(const char *p_handler)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  EVENT HANDLING
//

void CWebBrowser::BeforeNavigate2(const BSTR url, BOOL *cancel, IWebBrowser2 *whichbrowser)
{	 
	char *urlstring =  BStrToChar(url);

	bool t_cancel;

	m_in_callback = true;
	if (whichbrowser == iwebbrowser2)
		CB_NavigateRequest(GetInst(), urlstring, &t_cancel);
	else
		CB_NavigateFrameRequest(GetInst(), urlstring, &t_cancel);
	m_in_callback = false;

	*cancel = t_cancel;
	free(urlstring);
}

void CWebBrowser::FileDownload(const BSTR url, BOOL *cancel, IWebBrowser2 *whichbrowser)
{
	char *urlstring;
	urlstring = BStrToChar(url);

	bool cflag;
	m_in_callback = true;
	if(whichbrowser == iwebbrowser2)
	{
		CB_DownloadRequest(GetInst(), urlstring, &cflag);
		*cancel = cflag;
	}
	m_in_callback = false;

	free(urlstring);
}


void CWebBrowser::NavigateComplete2(const BSTR url,IWebBrowser2 *whichbrowser)
{
	char *urlstring =  BStrToChar(url);

	m_in_callback = true;
	if (whichbrowser == iwebbrowser2)
		CB_NavigateComplete(GetInst(), urlstring);
	else
		CB_NavigateFrameComplete(GetInst(), urlstring);
	m_in_callback = false;

	free(urlstring);
}

void CWebBrowser::DocumentComplete(const BSTR url,IWebBrowser2 *whichbrowser)
{
	IDispatch * pdisp;
	IHTMLDocument3 * pdoc;
	IHTMLElement * pelem;
	CComBSTR lsrc;
	HRESULT  hr;

	if (dummybrowser){
		delete dummybrowser;
		dummybrowser = NULL;
	}

	if (m_current_find != NULL)
	{
		m_current_find -> Release();
		m_current_find = NULL;
	}

	char *urlstring =  BStrToChar(url);

	m_in_callback = true;
	if (whichbrowser == iwebbrowser2)
		CB_DocumentComplete(GetInst(), urlstring);
	else
		CB_DocumentFrameComplete(GetInst(), urlstring);
	m_in_callback = false;

	free(urlstring);

	//populate the source string with the html source
	if( SUCCEEDED(whichbrowser->get_Document(&pdisp)) )
	{
		if(pdisp != NULL)
		{
			hr = pdisp->QueryInterface(IID_IHTMLDocument3,(void **)&pdoc);
			//now get the element
			if( hr == 0)
			{
				pdoc->get_documentElement(&pelem);
				pelem->get_outerHTML(&lsrc);
				htmlsource = lsrc;

				pelem->Release();
				pdoc->Release();
			}
			else
				htmlsource = "";

			pdisp->Release();
		}
	}
	else 
		htmlsource ="";
}

///////////////////////////////////////////////////////////////////////////////
//
//  MISC STUFF
//

int CWebBrowser::GetInst()
{
	return instID;
}

HWND CWebBrowser::GetHWND()
{
	if( inited )
	{
		return browserwindow.m_hWnd;
	}
	else
		return 0;
}

void CWebBrowser::SetInst(int linst)
{
	instID = linst;
}

BOOL CWebBrowser::getAdvancedMessages()
{
	return advancedMessages;
}

void CWebBrowser::swapBrowser()
{
	//unadvise and destroy

	if (browserevents) 
	{
		AtlUnadvise(iwebbrowser2, DIID_DWebBrowserEvents2, browserevents2_cookie); 
		browserevents->Release();
	}

	if (webui) webui->Release();
	if (iwebbrowser2!=NULL) {
		iwebbrowser2->Stop(); 
		iwebbrowser2->Release();

	}	

	browserwindow.DestroyWindow();
	inited = FALSE;
	//create new
	DWORD flags = WS_CHILD;
	browserwindow.Create(hostwindow, CWindow::rcDefault,
		browsertype == BT_IE? _T("{8856F961-340A-11D0-A96B-00C04FD705A2}") : _T("{1339B54C-3453-11D2-93B9-000000000000}"),
		flags,0,browserid);
	HRESULT hr = AtlAxGetControl(browserwindow.m_hWnd,&iunknown);
	if (SUCCEEDED(hr)) 
		iunknown->QueryInterface(IID_IWebBrowser2,(void**)&iwebbrowser2); 
	if (!iwebbrowser2) return;
	browserevents = new (nothrow) CWebEvents(this); 
	browserevents->AddRef();
	iwebbrowser2->put_Silent(VARIANT_TRUE);
	webui = new (nothrow) CWebUI(this); webui->AddRef();
	browserwindow.SetExternalUIHandler(webui);
	AtlAdvise(iwebbrowser2, browserevents, DIID_DWebBrowserEvents2,
		&browserevents2_cookie);
	inited = TRUE;
}

void CWebBrowser::NewWindow2(const BSTR p_url, LPDISPATCH *ppDisp, BOOL *Cancel)
{
	if (!GetNewWindow())
	{
		char *urlstring =  BStrToChar(p_url);
		CB_NewWindow(GetInst(), urlstring);
		free(urlstring);
		*Cancel = TRUE;
	}
}


HRESULT LoadWebBrowserFromStreamDo(IDispatch *pHtmlDoc, HGLOBAL pText)
{
	HRESULT hr;
	IStream *pstream = NULL;
	CreateStreamOnHGlobal(pText, TRUE, &pstream);
	if (pstream == NULL)
	{
		GlobalFree(pText);
		return -1;
	}

	IPersistStreamInit* pPersistStreamInit = NULL;
	hr = pHtmlDoc->QueryInterface( IID_IPersistStreamInit,  (void**)&pPersistStreamInit );
	if ( SUCCEEDED(hr) )
	{
		// Initialize the document.
		hr = pPersistStreamInit->InitNew();
		if ( SUCCEEDED(hr) )
		{
			// Load the contents of the stream.
			hr = pPersistStreamInit->Load( pstream );
		}
		pPersistStreamInit->Release();
	}

	pstream -> Release();

	return hr;
}


HGLOBAL s_navigation_text = NULL;
char *s_internal_navigation_url = NULL;

HRESULT LoadWebBrowserFromStream(IWebBrowser2* pWebBrowser, HGLOBAL pText)
{
	HRESULT hr;
	IDispatch* pHtmlDoc = NULL;

	// Retrieve the document object.
	hr = pWebBrowser->get_Document( &pHtmlDoc );

	// OK-2010-02-11: [[Bug 8549]] - Can't set the htmlText without having a page loaded, 
	// so if there isn't one, we load about:blank.
	if (pHtmlDoc == NULL)
	{
		// Save pText so we can retrieve it once the blank page is loaded
		s_navigation_text = pText;

		CComVariant vtEmpty;
		CComVariant vtURL("about:blank");

		// Store the url that we are navigating to so that we can disable callbacks 
		// for this url until its finished.
		s_internal_navigation_url = "about:blank";

		pWebBrowser -> Navigate2(&vtURL, &vtEmpty, &vtEmpty, &vtEmpty, &vtEmpty);
		return hr;
	}
	else
		return LoadWebBrowserFromStreamDo(pHtmlDoc, pText);
}


///////////////////////////////////////////////////////////////////////////////
//
// Dummy Browser Stuff
//

CDummyBrowser::CDummyBrowser(CWebBrowser *trealbrowser)
	: CWebBrowser(trealbrowser->GetHostWindow(),FALSE)
{
	realbrowser = trealbrowser;
	iwebbrowser2->put_RegisterAsBrowser(VARIANT_TRUE);
}

void CDummyBrowser::BeforeNavigate2(const BSTR url, BOOL *cancel,IWebBrowser2 *whichbrowser)
{
	char *urlstring =  BStrToChar(url);
	if (realbrowser->GetNewWindow())
		realbrowser->GoURL(urlstring, NULL);
	else
	{
		CB_NewWindow(GetInst(), urlstring);
		*cancel = TRUE;
	}
	free(urlstring);
}

void CDummyBrowser::NavigateComplete2(const BSTR url,IWebBrowser2 *whichbrowser)
{
}

void CDummyBrowser::DocumentComplete(const BSTR url,IWebBrowser2 *whichbrowser)
{
}

void CDummyBrowser::NewWindow2(const BSTR url, LPDISPATCH *ppDisp, BOOL *Cancel)
{
}

void CDummyBrowser::FileDownload(const BSTR url, BOOL *cancel, IWebBrowser2 * whichbrowser)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  BROWSER EVENT CALLBACK
//

CWebEvents::CWebEvents(CWebBrowser *p_browser)
	: ref(0),
	  webbrowser(p_browser),
	  connectionPoint(NULL),
	  advCookie(0),
	  m_target_browser(NULL),
	  m_target_dispatch(NULL)
{
}

CWebEvents::~CWebEvents(void)
{
	if (connectionPoint != NULL)
	{
		connectionPoint -> Unadvise(advCookie);
		connectionPoint -> Release();
	}
}

HRESULT STDMETHODCALLTYPE CWebEvents::QueryInterface(REFIID riid, void **ppv)
{
	if (riid==IID_IUnknown || riid==IID_IDispatch || riid==DIID_DWebBrowserEvents2) 
	{
		*ppv=this;
		AddRef();
		return S_OK;
	}
	
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CWebEvents::AddRef()
{
	return InterlockedIncrement(&ref);
}

ULONG STDMETHODCALLTYPE CWebEvents::Release() 
{
	int tmp = InterlockedDecrement(&ref);
	if (tmp==0)
		delete this;
	return tmp;
}
HRESULT STDMETHODCALLTYPE CWebEvents::GetTypeInfoCount(UINT __RPC_FAR *pctinfo)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWebEvents::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWebEvents::GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWebEvents::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,  WORD wFlags, DISPPARAMS *Params, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,UINT *puArgErr)
{ 
	if (s_internal_navigation_url != NULL)
	{
		if (dispIdMember == DISPID_BEFORENAVIGATE2 || dispIdMember ==  DISPID_NAVIGATECOMPLETE2)
			return S_OK;
	}

	switch (dispIdMember)
	{
	case DISPID_BEFORENAVIGATE2:
		BeforeNavigate2(Params -> rgvarg[6] . pdispVal, Params -> rgvarg[5] . pvarVal -> bstrVal, Params -> rgvarg[4] . pvarVal -> lVal, Params -> rgvarg[3] . pvarVal -> bstrVal, Params -> rgvarg[2] . pvarVal, Params -> rgvarg[1] . pvarVal, *(Params -> rgvarg[0] . pboolVal));
		break;
	case DISPID_FILEDOWNLOAD:
		FileDownload(Params -> rgvarg[1] . boolVal, *(Params -> rgvarg[0] . pboolVal));
		break;
	case DISPID_NEWWINDOW3:
		NewWindow3(Params -> rgvarg[0] . bstrVal, *(Params -> rgvarg[4] . ppdispVal), *(Params -> rgvarg[3] . pboolVal));
		break;
	case DISPID_NAVIGATECOMPLETE2:
		NavigateComplete2(Params -> rgvarg[1] . pdispVal, Params -> rgvarg[0] . pvarVal -> bstrVal);
		break;
	case DISPID_DOCUMENTCOMPLETE:
		DocumentComplete(Params -> rgvarg[1] . pdispVal, Params -> rgvarg[0] . pvarVal -> bstrVal);
		break;
	case DISPID_ONVISIBLE:
		 webbrowser->Refresh();
		 break;
	case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOUT:
	{
		HRESULT hr;
		 VARIANTARG v;
		 v = (Params->rgvarg)[0];
		 IDispatch * i;
		 i= v.pdispVal;
		 //should be an IHTMLElement
		 IHTMLEventObj * pEvent;
		 IHTMLElement * pElement;
		 hr = i->QueryInterface(IID_IHTMLEventObj, (void **)&pEvent);
		 if( SUCCEEDED(hr) && pEvent != NULL)
		 {
			CComBSTR sText;
			pEvent->get_srcElement(&pElement);
			if (pElement != NULL)
			{
				pElement->get_id(&sText);
				if( sText != NULL )
				{
					char *t_element;
					t_element = BStrToChar(sText);
					CB_ElementLeave(webbrowser -> GetInst(), t_element);
					free(t_element);
				}
			}

			pEvent->Release();
		 }
	}
	break;
	case DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOVER:
	{
		HRESULT hr;
		 VARIANTARG v;
		 v = (Params->rgvarg)[0];
		 IDispatch * i;
		 i= v.pdispVal;
		 //should be an IHTMLElement
		 IHTMLEventObj * pEvent;
		 IHTMLElement * pElement;
		 hr = i->QueryInterface(IID_IHTMLEventObj, (void **)&pEvent);
		 if( SUCCEEDED(hr) && pEvent != NULL)
		 {
			CComBSTR sText;
			pEvent->get_srcElement(&pElement);
			if (pElement != NULL)
			{
				pElement->get_id(&sText);
				if( sText != NULL )
				{
					char *t_element;
					t_element = BStrToChar(sText);
					CB_ElementEnter(webbrowser -> GetInst(), t_element);
					free(t_element);
				}
			}

			pEvent->Release();
		 }
	}
	break;
	case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
	{
		HRESULT hr;
		 VARIANTARG v;
		 v = (Params->rgvarg)[0];

		 IDispatch *i;
		 i = v.pdispVal;
		 
		 IHTMLEventObj * pEvent;
		 hr = i->QueryInterface(IID_IHTMLEventObj, (void **)&pEvent);
		 if( SUCCEEDED(hr) && pEvent != NULL)
		 {
			IHTMLElement * pElement;
			CComBSTR sText;
			hr = pEvent->get_srcElement(&pElement);
			if( pElement != NULL)
			{
			  pElement->get_id(&sText);
			  if( sText != NULL )
				{
					char *t_element;
					t_element = BStrToChar(sText);
					CB_ElementClick(webbrowser -> GetInst(), t_element);
					free(t_element);
				}
			}
			pEvent->Release();
		 }
	}
	break;
	}
	return S_OK;
}

void CWebEvents::BeforeNavigate2(IDispatch *p_browser_dispatch, const BSTR p_url, int p_flags, const BSTR p_target_frame, VARIANT*& p_post_data, VARIANT*& p_headers, VARIANT_BOOL& r_cancel)
{
	IWebBrowser2 *t_browser;

	p_browser_dispatch -> QueryInterface(IID_IWebBrowser2, (void **)&t_browser);
	m_target_url = p_url;
	m_target_browser = t_browser;

	BOOL t_bool_cancel;
	t_bool_cancel = r_cancel;
	webbrowser -> BeforeNavigate2(m_target_url, &t_bool_cancel, m_target_browser);
	r_cancel = t_bool_cancel;
}

// This event seems to be invoked whenever a file download occurs, of
// course what the definition of a 'FileDownload' is cannot be
// clearly determined.
//
void CWebEvents::FileDownload(VARIANT_BOOL p_active_document, VARIANT_BOOL& r_cancel)
{
	BOOL t_bool_cancel;
	t_bool_cancel = r_cancel;
	if (!p_active_document)
		webbrowser -> FileDownload(m_target_url, &t_bool_cancel, m_target_browser);
	r_cancel = t_bool_cancel;
}

// This event is invoked when a new window is about to be created.
// You can choose to:
//   1) provide a new window to open it into
//   2) cancel the navigation
//   3) let the system do whatever it desires
//
void CWebEvents::NewWindow2(IDispatch*& r_target_object, VARIANT_BOOL& r_cancel)
{
	BOOL t_bool_cancel;
	t_bool_cancel = r_cancel;
	webbrowser -> NewWindow2(NULL, &r_target_object, &t_bool_cancel);
	r_cancel = t_bool_cancel;
}

void CWebEvents::NewWindow3(const BSTR url, IDispatch*& r_target_object, VARIANT_BOOL& r_cancel)
{
	BOOL t_bool_cancel;
	t_bool_cancel = r_cancel;
	webbrowser -> NewWindow2(url, &r_target_object, &t_bool_cancel);
	r_cancel = t_bool_cancel;
}

void CWebEvents::NavigateComplete2(IDispatch *p_browser_dispatch, const BSTR p_url)
{
	IWebBrowser2 *t_browser;
	p_browser_dispatch -> QueryInterface(IID_IWebBrowser2, (void **)&t_browser);
	webbrowser -> NavigateComplete2(p_url, t_browser);
	t_browser -> Release();

	if (m_target_dispatch == NULL)
		m_target_dispatch = p_browser_dispatch;

	if (m_target_browser != NULL)
	{
		m_target_url . Empty();
		m_target_browser = NULL;
	}
}

void CWebEvents::DocumentComplete(IDispatch *p_browser_dispatch, const BSTR p_url)
{
	IWebBrowser2 *t_browser;
	p_browser_dispatch -> QueryInterface(IID_IWebBrowser2, (void **)&t_browser);

	HRESULT hr;
	if (s_navigation_text != NULL)
	{
		IDispatch *t_html_doc;
		hr = t_browser -> get_Document(&t_html_doc);

		if (t_html_doc == NULL)
			return;
	
		LoadWebBrowserFromStreamDo(t_html_doc, s_navigation_text);
		s_navigation_text = NULL;
		return;
	}

	if (webbrowser->getAdvancedMessages()) 
	{
		IDispatch *t_html_dispatch;
		hr = t_browser->get_Document( &t_html_dispatch );
		if (SUCCEEDED(hr))
		{
			IHTMLDocument2 *pHtmlDoc;
			t_html_dispatch -> QueryInterface(IID_IHTMLDocument2, (void **)&pHtmlDoc);
			t_html_dispatch -> Release();

			IConnectionPointContainer *connectionPointContainer;
			hr = pHtmlDoc->QueryInterface(IID_IConnectionPointContainer, (void**) &connectionPointContainer);

			IConnectionPoint *newConnectionPoint;
			hr = connectionPointContainer->FindConnectionPoint(DIID_HTMLDocumentEvents2, &newConnectionPoint);
			if (connectionPoint != newConnectionPoint)
			{
				if (connectionPoint != NULL)
				{
					connectionPoint -> Unadvise(advCookie);
					connectionPoint -> Release();
				}
				hr = newConnectionPoint->Advise(static_cast<IDispatch*>(this), &advCookie);
				connectionPoint = newConnectionPoint;
			}
			else
				newConnectionPoint -> Release();

			connectionPointContainer->Release();
			pHtmlDoc->Release();
		}
	}

	if (m_target_dispatch != NULL && m_target_dispatch == p_browser_dispatch || t_browser != webbrowser -> GetIWebBrowser2())
	{
		webbrowser -> DocumentComplete(p_url, t_browser);
		m_target_dispatch = NULL;
	}

	t_browser -> Release();
}

///////////////////////////////////////////////////////////////////////////////
//
// System Stuff
//

CWebBrowserBase *InstantiateBrowser(int p_window_id)
{
	AtlAxWinInit();
	return new CWebBrowser((HWND)p_window_id, FALSE);
}

class CWebBrowserModule: public CAtlDllModuleT<CWebBrowserModule>
{
};

CWebBrowserModule _AtlModule;

HINSTANCE theInstance;

HINSTANCE MCWin32BrowserGetHINSTANCE()
{
	return theInstance;
}

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	theInstance = hInstance;
	return _AtlModule.DllMain(dwReason, lpReserved); 
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _AtlModule.DllRegisterServer();
	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
