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

#ifndef __W32BROWSER__
#define __W32BROWSER__

#include <atlbase.h>
#include <atlwin.h>
#include <atlcom.h>
#include <atlhost.h>
#include <mshtml.h>
#include <MsHtmdid.h>
#include <exdispid.h>
#include <string>

#include "revbrowser.h"

extern CComModule _Module;

#define DISPID_FILEDOWNLOAD 270

extern char *BStrToChar(const BSTR bstr);

using namespace std;

enum BrowserType
{
	BT_IE,
	BT_NETSCAPE
};

char *BStrToChar(const BSTR str);

class CWebEvents;
class CWebUI;
class CDummyBrowser;
class CWebBrowser;

class CWebBrowserWindow: public CWindowImpl<CWebBrowserWindow>
{
public:
	CWebBrowserWindow(void);

	void SetBrowser(CWebBrowser *p_browser) {m_browser = p_browser;}

	BEGIN_MSG_MAP(CWebBrowserWindow)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT p_message, WPARAM p_wparam, LPARAM p_lparam, BOOL& r_handled);

private:
	CWebBrowser *m_browser;
	bool m_mouse_inside;
};

class CWebBrowser: public CWebBrowserBase
{
public:
	CWebBrowser(HWND hparent, BOOL isvisible);
	CWebBrowser(HWND hparent, BOOL isvisible, BrowserType browser);
	virtual ~CWebBrowser(void);

	BOOL IsInited() {return inited;}

	virtual void BeforeNavigate2(const BSTR url, BOOL *cancel,IWebBrowser2 *whichbrowser);
	virtual void NavigateComplete2(const BSTR url, IWebBrowser2 *whichbrowser);
	virtual void DocumentComplete(const BSTR url, IWebBrowser2 *whichbrowser);
	virtual void NewWindow2(const BSTR url, LPDISPATCH *ppDisp, BOOL *Cancel);
	virtual void FileDownload(const BSTR url, BOOL *cancel, IWebBrowser2 * whichbrowser);

	void WindowDestroyed(void);

	static LRESULT CALLBACK LWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

	void GoURL(const char *URL, const char *target_frame);
	void GoBack(void);
	void GoForward(void);
	void Refresh(void);
	void Stop(void);
	void Print(void);
	void Redraw(void);
	void Focus(void);
	void Unfocus(void);
	void MakeTextBigger();
	void MakeTextSmaller();
	char *ExecuteScript(const char *p_javascript_string);
	bool FindString(const char *p_string, bool p_search_up);
	char *CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count);


	bool GetBusy();
	bool GetOffline();
	bool GetScrollbars();
	bool GetBorder();
	bool GetVisible();
	bool GetMessages();
	bool GetContextMenu();
	bool GetNewWindow();
	char *GetBrowser();
	char *GetURL();
	char *GetSource();
	char *GetSelectedText();
	char *GetTitle();
	bool GetScale();
	void GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom);
	int GetInst();
	bool GetImage(void*& r_data, int& r_length);
	int GetVScroll(void);
	int GetHScroll(void);
	int GetFormattedHeight(void);
	int GetFormattedWidth(void);
	void GetFormattedRect(int &r_left, int &t_top, int &r_right, int &r_bottom);
	uintptr_t GetWindowId(void);
	char *GetUserAgent(void);

	void SetScale(bool willscale);
	void SetRect(int p_left, int p_top, int p_right, int p_bottom);
	void SetScrollbars(bool usescrollbars);
	void SetBorder(bool showborder);
	void SetMessages(bool getsMessages);
	void SetVisible(bool isvisible);
	void SetOffline(bool isoffline);
	void SetContextMenu(bool isenabled);
	void SetSource(const char * hsource);
	void SetNewWindow(bool isenabled);
	void SetBrowser(const char *browser);
	void SetSelectedText(const char * selText );
	void SetInst(int linst);
	void SetVScroll(int p_vscroll_pixels);
	void SetHScroll(int p_hscroll_pixels);
	void SetWindowId(uintptr_t id);
	void SetUserAgent(const char *p_user_agent);

	virtual void AddJavaScriptHandler(const char *p_handler);
	virtual void RemoveJavaScriptHandler(const char *p_handler);

	HWND GetHWND();
	HWND GetHostWindow()
	{return inited == TRUE?  hostwindow: 0;}
	IUnknown *GetIUnknown() 
	{return inited == TRUE?  iunknown: NULL;}
	IWebBrowser2 *GetIWebBrowser2() 
	{return inited == TRUE?  iwebbrowser2: NULL;}
	void swapBrowser();
	BOOL Draw(int twidth,int theight, HDC hdcMem);
	BOOL Draw(HDC hdcMem);
	BOOL getAdvancedMessages();
	CComQIPtr<IOleInPlaceActiveObject> m_spIOleInPlaceActiveObject;

	void Init(void);
protected:
	int instID;
	CDummyBrowser *dummybrowser;
	HWND hostwindow;
	HWND lhwnd;
	static int browseridcounter;
	CWebUI *webui;
	int browserid;
	CComBSTR htmlsource;
	BrowserType browsertype;
	DWORD browserevents2_cookie;
	CAxWindow browserwindow;
	CWebBrowserWindow browserwindowsubclass;
	CWebEvents *browserevents;
	IWebBrowser2 *iwebbrowser2;
	IUnknown *iunknown;
	int zoom;
	BOOL inited;
	BOOL allownewwindow;
	BOOL contextmenuenabled;
	BOOL scrollbarsenabled;
	BOOL borderenabled;
	BOOL scaleenabled;
	BOOL advancedMessages;
	BOOL wasvisible;

	IHTMLTxtRange *m_current_find;

	bool m_in_callback;

	CWebBrowser *next;

	IConnectionPoint *document_events;
	DWORD document_events_cookie;

	static HHOOK s_message_hook;
	static HHOOK s_window_hook;
	static CWebBrowser *s_focused_browser;
	static CWebBrowser *s_browsers;

	static LRESULT CALLBACK GetMessageHook(int p_code, WPARAM p_wparam, LPARAM p_lparam);
	static LRESULT CALLBACK CallWindowProcHook(int p_code, WPARAM p_wparam, LPARAM p_lparam);

	IHTMLWindow2 *GetHtmlWindow(void);
	IHTMLWindow2 *GetHtmlWindow(IHTMLDocument2 *);
	IHTMLDocument2 *GetHtmlDocument(void);

	IHTMLElement2 *GetBodyElement(void);
};

class CDummyBrowser: public CWebBrowser
{
public:
	CDummyBrowser(CWebBrowser *trealbrower);
	void BeforeNavigate2(const BSTR url, BOOL *cancel,IWebBrowser2 *whichbrowser);
	void NavigateComplete2(const BSTR url, IWebBrowser2 *whichbrowser);
	void DocumentComplete(const BSTR url, IWebBrowser2 *whichbrowser);
	void NewWindow2(const BSTR url, LPDISPATCH *ppDisp, BOOL *Cancel);
	void FileDownload(const BSTR url, BOOL *cancel, IWebBrowser2 * whichbrowser);

	void GoURL(const char *URL) {}
	void GoBack() {}
	void GoForward() {}
	void Refresh() {}
	void Stop() {}
	void Print() {}
	void Redraw() {}
	void Focus(void) {}
	void Unfocus(void) {}
	void MakeTextBigger() {}
	void MakeTextSmaller() {}

	bool GetBusy() {return false;}
	bool GetOffline() {return false;}
	bool GetScrollbars() {return false;}
	bool GetBorder() {return false;}
	bool GetVisible() {return false;}
	bool GetMessages() {return false;}
	bool GetContextMenu() {return false;}
	bool GetNewWindow() {return false;}
	bool GetScale() {return false;}
	char *GetBrowser() {return NULL;}
	char *GetURL() {return NULL;}
	char *GetSource() {return NULL;}
	char *GetSelectedText() {return NULL;}
	char *GetTitle() {return NULL;}
	void GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom) {}
	int GetInst() {return 0;}
	bool GetImage(void*& r_data, int& r_length) {return false;}

	void SetScale(bool willscale) {}
	void SetRect(int p_left, int p_top, int p_right, int p_bottom) {}
	void SetScrollbars(bool usescrollbars) {}
	void SetBorder(bool showborder) {}
	void SetMessages(bool getsMessages) {}
	void SetVisible(bool isvisible) {}
	void SetOffline(bool isoffline) {}
	void SetContextMenu(bool isenabled) {}
	void SetSource(const char * hsource) {}
	void SetNewWindow(bool isenabled) {}
	void SetBrowser(const char *p_browser) {}
	void SetSelectedText(const char * selText ) {}
	void SetInst(int linst) {}
private:
	CWebBrowser *realbrowser;
};

class CWebEvents : public IDispatch
{
public:
	DWORD advCookie;
	IConnectionPoint* connectionPoint;

	CWebEvents(CWebBrowser *thebrowser);
	~CWebEvents(void);

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT __RPC_FAR *pctinfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId);
	
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,  WORD wFlags, DISPPARAMS *Params, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

	void BeforeNavigate2(IDispatch *p_browser, const BSTR p_url, int p_flags, const BSTR p_target_frame, VARIANT*& p_post_data, VARIANT*& p_headers, VARIANT_BOOL& r_cancel);
	void FileDownload(VARIANT_BOOL p_active_document, VARIANT_BOOL& r_cancel);
	void NewWindow2(IDispatch*& r_target_object, VARIANT_BOOL& r_cancel);
	void NewWindow3(const BSTR p_url, IDispatch*& r_target_object, VARIANT_BOOL& r_cancel);
	void NavigateComplete2(IDispatch *p_browser, const BSTR p_url);
	void DocumentComplete(IDispatch *p_browser, const BSTR p_url);

protected:
	CWebBrowser *webbrowser;
	IDispatch *m_target_dispatch;
	IWebBrowser2 *m_target_browser;
	CComBSTR  m_target_url;

private:
	long ref;
};

class CWebUI : public IDocHostUIHandlerDispatch
{
public:
	CWebUI(CWebBrowser *thebrowser) {ref=0;webbrowser = thebrowser;}
	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) {if (riid==IID_IUnknown || riid==IID_IDispatch || riid==IID_IDocHostUIHandlerDispatch) {*ppv=this; AddRef(); return S_OK;} else return E_NOINTERFACE;}
	ULONG STDMETHODCALLTYPE AddRef() {return InterlockedIncrement(&ref);}
	ULONG STDMETHODCALLTYPE Release() {int tmp = InterlockedDecrement(&ref); if (tmp==0) delete this; return tmp;}
	// IDispatch
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT __RPC_FAR *pctinfo) {return S_OK;}
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo) {return S_OK;}
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId) {return S_OK;}
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr) {return S_OK;}
	// IDocHostUIHandlerDispatch
	HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, DWORD x, DWORD y, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved, HRESULT __RPC_FAR *dwRetVal) 
	{
		if (!webbrowser->GetContextMenu())*dwRetVal=S_OK;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetHostInfo(DWORD __RPC_FAR *pdwFlags, DWORD __RPC_FAR *pdwDoubleClick) 
	{
		if (!webbrowser->GetScrollbars())
			*pdwFlags |=  DOCHOSTUIFLAG_SCROLL_NO;
		if (!webbrowser->GetBorder())
			*pdwFlags |= DOCHOSTUIFLAG_NO3DBORDER;       
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, IUnknown __RPC_FAR *pActiveObject, IUnknown __RPC_FAR *pCommandTarget, IUnknown __RPC_FAR *pFrame, IUnknown __RPC_FAR *pDoc, HRESULT __RPC_FAR *dwRetVal) {return S_OK;}
	HRESULT STDMETHODCALLTYPE HideUI( void) {return S_OK;}
	HRESULT STDMETHODCALLTYPE UpdateUI( void) {return S_OK;}
	HRESULT STDMETHODCALLTYPE EnableModeless(VARIANT_BOOL fEnable) {return S_OK;}
	HRESULT STDMETHODCALLTYPE OnDocWindowActivate(VARIANT_BOOL fActivate) {return S_OK;}
	HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(VARIANT_BOOL fActivate) {return S_OK;}
	HRESULT STDMETHODCALLTYPE ResizeBorder(long left, long top, long right, long bottom, IUnknown __RPC_FAR *pUIWindow, VARIANT_BOOL fFrameWindow) {return S_OK;}
	HRESULT STDMETHODCALLTYPE TranslateAccelerator(DWORD_PTR hWnd, DWORD nMessage, DWORD_PTR wParam, DWORD_PTR lParam, BSTR bstrGuidCmdGroup, DWORD nCmdID, HRESULT __RPC_FAR *dwRetVal)
	{return S_OK;}
	HRESULT STDMETHODCALLTYPE GetOptionKeyPath(BSTR __RPC_FAR *pbstrKey, DWORD dw) {return S_OK;}
	HRESULT STDMETHODCALLTYPE GetDropTarget(IUnknown __RPC_FAR *pDropTarget, IUnknown __RPC_FAR *__RPC_FAR *ppDropTarget) {return S_OK;}
	HRESULT STDMETHODCALLTYPE GetExternal(IDispatch __RPC_FAR *__RPC_FAR *ppDispatch) {return S_OK;}
	HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, BSTR bstrURLIn, BSTR __RPC_FAR *pbstrURLOut) {return S_OK;}
	HRESULT STDMETHODCALLTYPE FilterDataObject(IUnknown __RPC_FAR *pDO, IUnknown __RPC_FAR *__RPC_FAR *ppDORet) {return S_OK;}
protected:
	long ref;
private:
	CWebBrowser *webbrowser;
};

#endif
