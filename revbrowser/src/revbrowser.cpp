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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <revolution/external.h>

#include "revbrowser.h"


///////////////////////////////////////////////////////////////////////////////
//
//  UTILITY FUNCTIONS

#define TRUE true

#ifdef _WINDOWS
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

inline char *istrdup(const char *p_string)
{
	return strdup(p_string);
}

inline char *BoolToStr(bool b) 
{
	return b ? istrdup("TRUE") : istrdup("FALSE");
}

inline bool StrToBool(char *boolstr)
{
	return strncasecmp(boolstr, "TRUE", 4) == 0 ? true : false;
}

inline char *IntToStr(int p_value)
{
    char *t_string;
    t_string = nil;
    // AL-2013-11-01 [[ Bug 11289 ]] Use libcore methods to prevent potential buffer overflows in revbrowser
    MCCStringFormat(t_string, "%d", p_value);
	return t_string;
}

///////////////////////////////////////////////////////////////////////////////
//
//  INSTANCE HANDLING
//

class BrowserInstances
{
public:
	BrowserInstances(void);
	~BrowserInstances(void);

	void Add(CWebBrowserBase *p_browser, bool p_is_xbrowser);
	void Delete(CWebBrowserBase *p_browser);

	int GetActiveInstanceId(void);
	CWebBrowserBase *GetFirstInstance(void);
	CWebBrowserBase *GetActiveInstance(void);
	CWebBrowserBase *GetInstance(int p_id);

	int GetCallbackDepth(int p_id);
	
	bool Callback(int p_instance_id, const char *p_message, const char *p_url = NULL);
	bool Callback(int p_id, const char *p_message, char **p_args, uint32_t p_arg_count, bool &r_pass);

	void SetActiveInstanceId(int t_id);

	char *GetInstanceIds(void);

private:
	struct BrowserInstance
	{
		BrowserInstance *next;
		int instance_id;
		char *stack_id;
		CWebBrowserBase *browser;
		int callback_depth;
		bool xbrowser_callbacks;
	};

	BrowserInstance *m_instances;
	BrowserInstance *m_active_instance;
	int m_last_instance_id;

	bool FindInstanceById(int p_id, BrowserInstance *&r_instance);
	void SendMessage(BrowserInstance *p_instance, const char *p_message, bool &r_pass);
};

static BrowserInstances s_browsers;

BrowserInstances::BrowserInstances(void)
{
	m_instances = NULL;
	m_active_instance = NULL;
	m_last_instance_id = 0;
}

BrowserInstances::~BrowserInstances(void)
{
	while(m_instances != NULL)
	{
		BrowserInstance *t_next;
		t_next = m_instances -> next;
		
		if (m_instances -> stack_id != NULL)
			free(m_instances -> stack_id);

		delete m_instances -> browser;
		delete m_instances;
		
		m_instances = t_next;
	}
}

bool BrowserInstances::FindInstanceById(int p_id, BrowserInstance *&r_instance)
{
	for(BrowserInstance *t_instance = m_instances; t_instance != nil; t_instance = t_instance->next)
	{
		if (t_instance->instance_id == p_id)
		{
			r_instance = t_instance;
			return true;
		}
	}

	return false;
}

void BrowserInstances::Add(CWebBrowserBase *p_browser, bool p_is_xbrowser)
{
	BrowserInstance *t_instance;
	t_instance = new (nothrow) BrowserInstance;
	t_instance -> next = m_instances;
	t_instance -> instance_id = ++m_last_instance_id;
	t_instance -> stack_id = NULL;
	t_instance -> browser = p_browser;
	t_instance -> xbrowser_callbacks = p_is_xbrowser;
	t_instance -> callback_depth = 0;
	m_instances = t_instance;
	m_active_instance = t_instance;
	p_browser -> SetInst(t_instance -> instance_id);
}

void BrowserInstances::Delete(CWebBrowserBase *p_browser)
{
	BrowserInstance *t_instance, *t_last_instance;
	for(t_instance = m_instances, t_last_instance = NULL; t_instance != NULL; t_last_instance = t_instance, t_instance = t_instance -> next)
		if (t_instance -> browser == p_browser)
			break;

	if (t_instance != NULL)
	{
		if (t_last_instance != NULL)
			t_last_instance -> next = t_instance -> next;
		else
			m_instances = t_instance -> next;

		if (m_active_instance == t_instance)
			m_active_instance = m_instances;

		if (t_instance -> stack_id != NULL)
			free(t_instance -> stack_id);

		delete t_instance -> browser;
		delete t_instance;
	}
}

int BrowserInstances::GetActiveInstanceId(void)
{
	if (m_active_instance == NULL)
		return 0;

	return m_active_instance -> instance_id;
}

CWebBrowserBase *BrowserInstances::GetFirstInstance(void)
{
	if (m_instances == NULL)
		return NULL;
	return m_instances -> browser;
}

CWebBrowserBase *BrowserInstances::GetActiveInstance(void)
{
	if (m_active_instance == NULL)
		return NULL;

	return m_active_instance -> browser;
}

CWebBrowserBase *BrowserInstances::GetInstance(int p_id)
{
	BrowserInstance *t_instance;
	if (FindInstanceById(p_id, t_instance))
		return t_instance->browser;
	else
		return nil;
}

void BrowserInstances::SetActiveInstanceId(int p_id)
{
	BrowserInstance *t_instance;
	if (FindInstanceById(p_id, t_instance))
		m_active_instance = t_instance;
}

char *BrowserInstances::GetInstanceIds(void)
{
	char *t_buffer;
    t_buffer = nil;
	
	for(BrowserInstance *t_instance = m_instances; t_instance != NULL; t_instance = t_instance -> next)
	{
        // AL-2013-11-01 [[ Bug 11289 ]] Use libcore methods to prevent potential buffer overflows in revbrowser
        if (t_instance == m_instances)
            MCCStringFormat(t_buffer, "%d", t_instance -> instance_id);
        else
            MCCStringAppendFormat(t_buffer, ",%d", t_instance -> instance_id);
	}

	return t_buffer;
}

int BrowserInstances::GetCallbackDepth(int p_id)
{
	BrowserInstance *t_instance;
	if (FindInstanceById(p_id, t_instance))
		return t_instance->callback_depth;
	else
		return 0;
}

bool is_escape_char(char p_char)
{
	return p_char == '"';
}

// IM-2014-03-06: [[ revBrowserCEF ]] Handle double-quote in strings by generating LC expression
// that evaluates to the original string (using string concatenation with the "quote" constant)
bool MCCStringQuote(const char *p_string, char *&r_quoted)
{
	if (p_string == nil || p_string[0] == '\0')
		return MCCStringClone("\"\"", r_quoted);

	bool t_success;
	t_success = true;

	char *t_quoted;
	t_quoted = nil;

	while (t_success && *p_string)
	{
		if (!is_escape_char(*p_string))
		{
			const char *t_run_start;
			t_run_start = p_string;

			uint32_t t_run_length;
			t_run_length = 0;

			while (*p_string != '\0' && !is_escape_char(*p_string))
				p_string++;

			t_run_length = p_string - t_run_start;

			t_success = MCCStringAppendFormat(t_quoted, t_quoted == nil ? "\"%*.*s\"" : "&\"%*.*s\"", t_run_length, t_run_length, t_run_start);
		}
		else if (*p_string == '"')
		{
			t_success = MCCStringAppend(t_quoted, t_quoted == nil ? "quote" : "&quote");
			p_string++;
		}
	}

	if (t_success)
		r_quoted = t_quoted;
	else if (t_quoted != nil)
		MCCStringFree(t_quoted);

	return t_success;
}

#define MCSCRIPT_CALLBACK "\
local tID=%d;\
local tWinID=%d\
%s;%s\
global XBrowservar;\
if XBrowservar is empty or the windowId of XBrowservar is not tWinID then;\
repeat for each line tLine in the openStacks;\
if the windowId of stack tLine is tWinID then;\
put the long id of stack tLine into XBrowservar;\
exit repeat;\
end if;\
end repeat;\
end if;\
send \"%s tID%s\" to this card of XBrowservar"

// IM-2014-03-06: [[ revBrowserCEF ]] Create script to call handler with the given parameters
bool revBrowserCreateCallbackScript(int p_id, int p_window_id, const char *p_message, char **p_args, uint32_t p_arg_count, char *&r_script)
{
	bool t_success;
	t_success = true;

	char *t_assigns;
	t_assigns = nil;

	char *t_locals;
	t_locals = nil;

	for (uint32_t i = 0; t_success && i < p_arg_count; i++)
	{
		char *t_quoted_string;
		t_quoted_string = nil;

		t_success = MCCStringQuote(p_args[i], t_quoted_string);
		if (t_success)
		{
			t_success = MCCStringAppendFormat(t_locals, ", tArg%d", i);
			if (t_success)
				t_success = MCCStringAppendFormat(t_assigns, "put %s into tArg%d;", t_quoted_string, i);
		}

		if (t_quoted_string != nil)
			MCCStringFree(t_quoted_string);
	}

	char *t_script;
	t_script = nil;

	if (t_success)
	{
		// IM-2014-03-13: [[ revBrowserCEF ]] fix const ptr compile error
		const char *t_locals_str = t_locals ? t_locals : "";
		const char *t_assigns_str = t_assigns ? t_assigns : "";
		t_success = MCCStringFormat(t_script, MCSCRIPT_CALLBACK, p_id, p_window_id, t_locals_str, t_assigns_str, p_message, t_locals_str);
	}
	if (t_locals)
		MCCStringFree(t_locals);
	if (t_assigns)
		MCCStringFree(t_assigns);

	if (t_success)
		r_script = t_script;
	else
	{
		if (t_script)
			MCCStringFree(t_script);
	}

	return t_success;
}

bool BrowserInstances::Callback(int p_id, const char *p_message, char **p_args, uint32_t p_arg_count, bool &r_pass)
{
	bool t_success;
	t_success = true;

	BrowserInstance *t_instance;
	if (t_success)
		t_success = FindInstanceById(p_id, t_instance);

	if (t_success)
	{
		int t_retval;
		if (t_instance -> stack_id != NULL)
			SetGlobalUTF8("XBrowservar", t_instance -> stack_id, &t_retval);
		else
			SetGlobalUTF8("XBrowservar", "", &t_retval);
	}

	char *t_script;
	t_script = nil;

	if (t_success)
		t_success = revBrowserCreateCallbackScript(p_id, t_instance->browser->GetWindowId(), p_message, p_args, p_arg_count, t_script);

	bool t_pass;
	if (t_success)
		SendMessage(t_instance, t_script, t_pass);

	if (t_script)
		MCCStringFree(t_script);

	if (t_success)
		r_pass = t_pass;

	return t_success;
}

void BrowserInstances::SendMessage(BrowserInstance *p_instance, const char *p_message, bool &r_pass)
{
	int t_retval;
	SetGlobal(p_instance->xbrowser_callbacks ? "XBrowserCancel" : "browserCancel", "FALSE", &t_retval);

	p_instance -> callback_depth += 1;
	SendCardMessageUTF8(p_message, &t_retval);
	p_instance -> callback_depth -= 1;
	
	if (p_instance -> stack_id != NULL)
		free(p_instance -> stack_id);

	p_instance -> stack_id = GetGlobalUTF8("XBrowservar", &t_retval);

	bool t_pass;

	char *t_cancel;
	t_cancel = GetGlobalUTF8(p_instance -> xbrowser_callbacks ? "XBrowserCancel" : "browserCancel", &t_retval);
	if (t_cancel != NULL)
	{
		t_pass = !StrToBool(t_cancel);
		free(t_cancel);
	}
	else
		t_pass = true;

	r_pass = t_pass;
}

bool BrowserInstances::Callback(int p_id, const char *p_message, const char *p_argument)
{

	static const char *s_message_template =
"global XBrowservar;\
if XBrowservar is empty or the windowId of XBrowservar is not %d then;\
repeat for each line tLine in the openStacks;\
if the windowId of stack tLine is %d then;\
put the long id of stack tLine into XBrowservar;\
exit repeat;\
end if;\
end repeat;\
end if;\
send \"%s%s\" && %d to this card of XBrowservar";

	static const char *s_xbrowser_message_template_with_argument =
"global XBrowservar;\
if XBrowservar is empty or the windowId of XBrowservar is not %d then;\
repeat for each line tLine in the openStacks;\
if the windowId of stack tLine is %d then;\
put the long id of stack tLine into XBrowservar;\
exit repeat;\
end if;\
end repeat;\
end if;\
send \"XBrowser_%s\" && quote & \"%s\" & quote, %d to this card of XBrowservar";

	static const char *s_message_template_with_argument =
"global XBrowservar;\
if XBrowservar is empty or the windowId of XBrowservar is not %d then;\
repeat for each line tLine in the openStacks;\
if the windowId of stack tLine is %d then;\
put the long id of stack tLine into XBrowservar;\
exit repeat;\
end if;\
end repeat;\
end if;\
send \"browser%s\" && %d, quote & \"%s\" & quote to this card of XBrowservar";

	BrowserInstance *t_instance;
	if (!FindInstanceById(p_id, t_instance))
		return true;

	int t_retval;
	if (t_instance -> stack_id != NULL)
		SetGlobalUTF8("XBrowservar", t_instance -> stack_id, &t_retval);
	else
		SetGlobalUTF8("XBrowservar", "", &t_retval);

	int t_window_id;
	t_window_id = t_instance -> browser -> GetWindowId();

	char *t_message;
    t_message = nil;
    
    // AL-2013-11-01 [[ Bug 11289 ]] Use libcore methods to prevent potential buffer overflows in revbrowser
	if (p_argument == NULL)
		MCCStringFormat(t_message, s_message_template, t_window_id, t_window_id, t_instance -> xbrowser_callbacks ? "XBrowser_" : "browser", p_message, p_id);
	else
	{
		if (t_instance -> xbrowser_callbacks)
			MCCStringFormat(t_message, s_xbrowser_message_template_with_argument, t_window_id, t_window_id, p_message, p_argument, p_id);
		else
			MCCStringFormat(t_message, s_message_template_with_argument, t_window_id, t_window_id, p_message, p_id, p_argument);
	}

	SetGlobalUTF8(t_instance -> xbrowser_callbacks ? "XBrowserCancel" : "browserCancel", "FALSE", &t_retval);
	
	t_instance -> callback_depth += 1;
	SendCardMessageUTF8(t_message, &t_retval);
	t_instance -> callback_depth -= 1;
	
    MCCStringFree (t_message);
    
	if (t_instance -> stack_id != NULL)
		free(t_instance -> stack_id);

	t_instance -> stack_id = GetGlobalUTF8("XBrowservar", &t_retval);

	bool t_pass;

	char *t_cancel;
	t_cancel = GetGlobal(t_instance -> xbrowser_callbacks ? "XBrowserCancel" : "browserCancel", &t_retval);
	if (t_cancel != NULL)
	{
		t_pass = !StrToBool(t_cancel);
		free(t_cancel);
	}
	else
		t_pass = true;

	return t_pass;
}

///////////////////////////////////////////////////////////////////////////////
//
//  PROPERTY HANDLING
//

enum BrowserProperty
{
	BROWSERPROP_UNDEFINED,
	BROWSERPROP_BUSY,
	BROWSERPROP_HTMLTEXT,
	BROWSERPROP_RECT,
	BROWSERPROP_URL,
	BROWSERPROP_OFFLINE,
	BROWSERPROP_VISIBLE,
	BROWSERPROP_CONTEXTMENU,
	BROWSERPROP_HTMLIMAGE,
	BROWSERPROP_SCROLLBARS,
	BROWSERPROP_SHOWBORDER,
	BROWSERPROP_NEWWINDOW,
	BROWSERPROP_SCALE,
	BROWSERPROP_TITLE,
	BROWSERPROP_VERSION,
	BROWSERPROP_BROWSER,		/* cb added for plugin control */
	BROWSERPROP_INSTANCE,        /* cb added for multi instance support */
	BROWSERPROP_INSTLIST,
	BROWSERPROP_MESSAGES,
	BROWSERPROP_SELECTED,		/* cb added for accessing selected text */
	BROWSERPROP_VSCROLL,
	BROWSERPROP_HSCROLL,
	BROWSERPROP_FORMATTEDHEIGHT,
	BROWSERPROP_FORMATTEDWIDTH,
	BROWSERPROP_FORMATTEDRECT,
	BROWSERPROP_WINDOWID,
	BROWSERPROP_USERAGENT,
};

struct BrowserProp
{
	BrowserProperty prop;
	const char *str;
};

BrowserProp browserProperties[] =
{
	{BROWSERPROP_BUSY,"BUSY"},
	{BROWSERPROP_HTMLTEXT,"HTMLTEXT"},
	{BROWSERPROP_RECT,"RECT"},
	{BROWSERPROP_URL,"URL"},
	{BROWSERPROP_OFFLINE,"OFFLINE"},
	{BROWSERPROP_CONTEXTMENU,"CONTEXTMENU"},
	{BROWSERPROP_VISIBLE,"VISIBLE"},
	{BROWSERPROP_HTMLIMAGE,"HTMLIMAGE"},
	{BROWSERPROP_SCROLLBARS,"SCROLLBARS"},
	{BROWSERPROP_SHOWBORDER,"SHOWBORDER"},
	{BROWSERPROP_NEWWINDOW,"NEWWINDOW"},
	{BROWSERPROP_BROWSER, "BROWSER"},
	{BROWSERPROP_TITLE, "TITLE"},
	{BROWSERPROP_VERSION, "VERSION"},
	{BROWSERPROP_SCALE, "SCALE"},
	{BROWSERPROP_INSTANCE, "INSTANCE"},
	{BROWSERPROP_INSTLIST, "INSTANCES"},
	{BROWSERPROP_SELECTED, "SELECTED"},
	{BROWSERPROP_MESSAGES, "MESSAGES"},
	{BROWSERPROP_VSCROLL, "VSCROLL"},
	{BROWSERPROP_HSCROLL, "HSCROLL"},
	{BROWSERPROP_FORMATTEDHEIGHT, "FORMATTEDHEIGHT"},
	{BROWSERPROP_FORMATTEDWIDTH, "FORMATTEDWIDTH"},
	{BROWSERPROP_FORMATTEDRECT, "FORMATTEDRECT"},
	{BROWSERPROP_WINDOWID, "WINDOWID"},
	{BROWSERPROP_USERAGENT, "USERAGENT"},
};

///////////////////////////////////////////////////////////////////////////////
//
//  CALLBACKS
//

// IM-2014-03-06: [[ revBrowserCEF ]] Send a custom callback with the given params
void CB_Custom(int p_instance_id, const char *p_message, char **p_args, uint32_t p_arg_count, bool *r_cancel)
{
	bool t_success, t_pass;
	t_success = s_browsers.Callback(p_instance_id, p_message, p_args, p_arg_count, t_pass);

	*r_cancel = t_success && !t_pass;
}

// Callback:
//   XBrowser_BeforeNavigate pURL
// Description:
//   The browser sends this message before it navigates to a new URL.
//
//   To prevent the navigation from occuring, set the global variable
//   XBrowserCancel to true.
//
void CB_NavigateRequest(int p_instance_id, const char *p_url, bool *r_cancel)
{	
	*r_cancel = !s_browsers . Callback(p_instance_id, "BeforeNavigate", p_url);
}

// Callback:
//   XBrowser_Over pElementId, pInstanceId
// Description:
//   The browser sends this message when the mouse is moving over the
//   given instance.
//
void CB_ElementEnter(int p_instance_id, const char *p_element)
{
	s_browsers . Callback(p_instance_id, "Over", p_element);
 }

// Callback:
//   XBrowser_Out pElementId, pInstanceId
// Description:
//   The browser sends this message when the mouse moves out of the
//   given instance.
//
void CB_ElementLeave(int p_instance_id, const char *p_element)
{
	s_browsers . Callback(p_instance_id, "Out", p_element);
 }

// Callback:
//   XBrowser_Click pElementId, pInstanceId
// Description:
//   The browser sends this message when the mouse is clicked in the
//   given instance.
//
void CB_ElementClick(int p_instance_id, const char *p_element)
{
	s_browsers . Callback(p_instance_id, "Click", p_element);
 }

// Callback:
//   XBrowser_New pInstanceId
// Description:
//   The browser sends this message when a new instance is created
//   with the given instance id.
//
void CB_CreateInstance(int p_instance_id)
{
	s_browsers . Callback(p_instance_id, "NewInstance");
 }

// Callback:
//   XBrowser_Closing pInstanceId
// Description:
//   The browser sends this message when an instance is being
//   destroyed.
//
void CB_DestroyInstance(int p_instance_id)
{
	s_browsers . Callback(p_instance_id, "Closing");
}

// Callback:
//   XBrowser_RequestDownload pURL, pInstanceId
// Description:
//   The browser sends this message when a download has been
//   requested.
//
//   To prevent the navigation from occuring, set the global variable
//   XBrowserCancel to true.
//
void CB_DownloadRequest(int p_instance_id, const char *p_url, bool *r_cancel)
{
	*r_cancel = !s_browsers . Callback(p_instance_id, "DownloadRequest", p_url);
}

// Callback:
//   XBrowser_BeforeNavigateFrame pURL, pInstanceId
// Description:
//   The browser sends this message before it navigates to a new URL
//   in a frame.
//
//   To prevent the navigation from occuring, set the global variable
//   XBrowserCancel to true.
//
void CB_NavigateFrameRequest(int p_instance_id, const char *p_url, bool *r_cancel)
{	
	*r_cancel = !s_browsers . Callback(p_instance_id, "BeforeNavigateFrame", p_url);
}

// Callback:
//   XBrowser_NavigateComplete pURL, pInstanceId
// Description:
//   The browser sends this message when a given URL has been navigated
//   to, but not necessarily finished loading.
//
void CB_NavigateComplete(int p_instance_id, const char *p_url)
{
	s_browsers . Callback(p_instance_id, "NavigateComplete", p_url);
}

// Callback:
//   XBrowser_NavigateCompleteFrame pURL, pInstanceId
// Description:
//   The browser sends this message when a given URL has been navigated
//   to in a frame, but not necessarily finished loading.
//
void CB_NavigateFrameComplete(int p_instance_id, const char *p_url)
{
	s_browsers . Callback(p_instance_id, "NavigateCompleteFrame", p_url);
}

// Callback:
//   XBrowser_DocumentComplete pURL, pInstanceId
// Description:
//   The browser sends this message when a given URL has finished
//   loading.
//
void CB_DocumentComplete(int p_instance_id, const char *p_url)
{
	s_browsers . Callback(p_instance_id, "DocumentComplete", p_url);
}

// Callback:
//   browserDocumentFailed pInstanceId, pURL, pErrorMessage
// Description:
//   The browser sends this message when a given URL has failed to load.
//
void CB_DocumentFailed(int p_instance_id, const char *p_url, const char *p_error)
{
	const char *t_params[2];
	t_params[0] = p_url;
	t_params[1] = p_error;
	
	bool t_pass;
	s_browsers . Callback(p_instance_id, "browserDocumentFailed", (char**)t_params, 2, t_pass);
}

// Callback:
//   XBrowser_DocumentCompleteFrame pURL, pInstanceId
// Description:
//   The browser sends this message when a given URL has finished
//   loading into a frame.
//
void CB_DocumentFrameComplete(int p_instance_id, const char *p_url)
{
	s_browsers . Callback(p_instance_id, "DocumentCompleteFrame", p_url);
}

// Callback:
//   browserDocumentFailedFrame pInstanceId, pURL, pErrorMessage
// Description:
//   The browser sends this message when a given URL has failed to load into a frame.
//
void CB_DocumentFrameFailed(int p_instance_id, const char *p_url, const char *p_error)
{
	const char *t_params[2];
	t_params[0] = p_url;
	t_params[1] = p_error;
	
	bool t_pass;
	s_browsers . Callback(p_instance_id, "browserDocumentFailedFrame", (char**)t_params, 2, t_pass);
}

// Callback:
//   XBrowser_NewURLWindow pURL, pInstanceId
// Description:
//   The browser sends this message when a given URL has been opened
//   into a new window.
//
void CB_NewWindow(int p_instance_id, const char *p_url)
{
	s_browsers . Callback(p_instance_id, "NewURLWindow", p_url); 
}

///////////////////////////////////////////////////////////////////////////////
//
//  COMMAND HANDLING
//

// Command:
//   revBrowserOpen(pWindowID, [ pURL ])
//   XBrowser_Open pWindowId, [ pURL ]
// Description:
//   Open a new browser in the window with the given windowId, and navigate to
//   the given URL.
// Result:
//   The instance ID of the new browser.
//
void commonBrowserOpen(bool p_is_xbrowser, bool p_is_cef_browser, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	char inID[255];

	if( nargs > 0  )
	{
		CWebBrowserBase *t_browser;
		if (p_is_cef_browser)
			t_browser = MCCefBrowserInstantiate(atoi(args[0]));
		else
			t_browser = InstantiateBrowser(atoi(args[0]));


		if (t_browser != NULL)
		{
			s_browsers . Add(t_browser, p_is_xbrowser);
			sprintf(inID, "%i", s_browsers . GetActiveInstanceId());
			result = istrdup(inID);
			CB_CreateInstance(s_browsers . GetActiveInstanceId());

			if (nargs == 2)
			{
				t_browser -> GoURL(args[1]);
				t_browser -> SetVisible(TRUE);
			}
		}
		else
		{
			*retstring = strdup("creation failed");
			*error = True;
			*pass = False;
			return;
		}
	}

	*error = False;
	*pass = False;

	*retstring = result != NULL ? result : (char *)calloc(1,0);
}

void revBrowserOpen(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	commonBrowserOpen(false, false, args, nargs, retstring, pass, error);
}

// IM-2014-03-18: [[ revBrowserCEF ]] create new browser instance using CEF
void revBrowserOpenCef(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	commonBrowserOpen(false, true, args, nargs, retstring, pass, error);
}

void XBrowserOpen(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	commonBrowserOpen(true, false, args, nargs, retstring, pass, error);
}

// Command:
//   XBrowser_Init
// Description:
//   No longer needed.
//
void XBrowserInit(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result : (char *)calloc(1,0);
}

// Command:
//   revBrowserNavigate pInstanceId, pURL
//   XBrowser_Navigate pURL, [ pInstanceId ]
// Description:
//   Navigate to the given URL in the browser identified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserNavigate(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;
	
	if (t_browser != NULL)
	{
		if (nargs == 2)
			t_browser -> GoURL(args[0], args[1]);
		else
			t_browser -> GoURL(args[0]);
	}

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserBack pInstanceId
//   XBrowser_Back [ pInstanceId ]
// Description:
//   Go back in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserBack(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> GoBack();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

void revBrowserFind(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	const char *t_result;
	t_result = NULL;
	
	CWebBrowserBase *t_browser;
	if (t_result == NULL)		
		t_browser = p_instance;
	
	if (t_result == NULL)
		if (nargs != 2)
			t_result = "incorrect number of arguments";
	
	bool t_search_up;
	t_search_up = false;
	if (t_result == NULL)
		t_search_up = strcmp(args[1], "up") == 0;
	
	if (t_result == NULL && t_browser != NULL)
		if (!t_browser -> FindString(args[0], t_search_up))
			t_result = "not found";
			
	*error = False;		
	*pass = False;		
	if (t_result == NULL)
		*retstring = strdup("");
	else
		*retstring = strdup(t_result);
}


void revBrowserCallScript(CWebBrowserBase *p_instance, char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_error = False;

	// We must be given either at least 1 argument. The first argument is the function name to call,
	// any subsequent arguments are parameters to the function.
	if (!*r_error)
	{
		if (p_argument_count < 1)
		{
			*r_result = strdup("incorrect number of arguments");
			*r_error = True;
		}
	}


	CWebBrowserBase *t_browser;
	if (!*r_error)
		t_browser = p_instance;

	char *t_result;
	t_result = NULL;

	if (!*r_error)
	{
		t_result = t_browser -> CallScript(p_arguments[0], &p_arguments[1], (p_argument_count - 1));
		if (t_result == NULL)
		{
			*r_result = strdup("error in script");
			*r_error = True;
		}
		else
			*r_result = strdup(t_result);
	}

	if (t_result != NULL)
		free(t_result);

	*r_pass = False;

}


void revBrowserExecuteScript(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	*error = False;
	
	if (!*error)
		if (nargs != 1)
		{
			*retstring = strdup("incorrect number of arguments");
			*error = True;
		}
		
	CWebBrowserBase *t_browser;
	if (!*error)
		t_browser = p_instance;

	char *t_result;
	t_result = NULL;
	if (!*error)
	{
		t_result = t_browser -> ExecuteScript(args[0]);
		if (t_result == NULL)
		{
			*retstring = strdup("error in script");
			*error = True;
		}
		else
			*retstring = strdup(t_result);
	}

	if (t_result != NULL)
		free(t_result);
	
	*pass = False;
}


// Command:
//   revBrowserForward pInstanceId
//   XBrowser_Forward [ pInstanceId ]
// Description:
//   Go forward in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserForward(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> GoForward();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserRefresh pInstanceId
//   XBrowser_Refresh [ pInstanceId ]
// Description:
//   Refresh the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserRefresh(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Refresh();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserStop pInstanceId
//   XBrowser_Stop [ pInstanceId ]
// Description:
//   Stop the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserStop(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Stop();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserClose pInstanceId
//   XBrowser_Close [ pInstanceId ]
// Description:
//   Close the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserClose(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;	

	if (t_browser != NULL)
	{
		if (s_browsers . GetCallbackDepth(t_browser -> GetInst()) > 0)
		{
			*error = True;
			*pass = False;
			*retstring = strdup("can't close browser inside callback");
			return;
		}
		
		CB_DestroyInstance(t_browser -> GetInst());
		s_browsers . Delete(t_browser);
	}

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserFocus pInstanceId
//   XBrowser_Focus [ pInstanceId ]
// Description:
//   Focus the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserFocus(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	
	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Focus();

	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserUnfocus pInstanceId
//   XBrowser_Unfocus [ pInstanceId ]
// Description:
//   Unfocus the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserUnfocus(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	
	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Unfocus();

	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserPrint pInstanceId
//   XBrowser_Print [ pInstanceId ]
// Description:
//   Print the page displayed in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserPrint(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Print();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserRedraw pInstanceId
//   XBrowser_Redraw [ pInstanceId ]
// Description:
//   Redraw the page displayed in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserRedraw(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> Redraw();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserMakeTextBigger pInstanceId
//   XBrowser_MakeTextBigger [ pInstanceId ]
// Description:
//   Make text bigger in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserMakeTextBigger(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> MakeTextBigger();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserMakeTextSmaller pInstanceId
//   XBrowser_MakeTextSmaller [ pInstanceId ]
// Description:
//   Make text smaller in the browser specified by pInstanceId. If no instance
//   id is specified, the last created browser is used.
//
void revBrowserMakeTextSmaller(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	if (t_browser != NULL)
		t_browser -> MakeTextSmaller();

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// Command:
//   revBrowserSnapshot pInstanceId, pReturnVariableName
// Description:
//   Take a snapshot of the browser with id pInstanceId and place the imagedata
//   in pReturnVariableName.
//
void revBrowserSnapshot(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	if (nargs != 1)
	{
		*retstring = istrdup("wrong number of parameters");
		*error = True;
		*pass = False;
		return;
	}

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	void *t_data;
	int t_length;
	if (t_browser -> GetImage(t_data, t_length))
	{
		ExternalString t_image_data;
		t_image_data . buffer = (char *)t_data;
		t_image_data . length = t_length;

		int t_ret_value;
		SetVariableEx(args[0], "", &t_image_data, &t_ret_value);

		free(t_data);

		if (t_ret_value != EXTERNAL_SUCCESS)
		{
			*retstring = istrdup("unknown variable");
			*error = True;
			*pass = False;
			return;
		}
	}
	else
	{
		*retstring = istrdup("snapshot failed");
		*error = True;
		*pass = False;
		return;
	}

	*error = False;
	*pass = False;
	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

// IM-2014-03-06: [[ revBrowserCEF ]] Allows a LiveCode handler to be called from within JavaScript
// creates a JS function with the same name as the handler within a global liveCode JS object
void revBrowserAddJavaScriptHandler(CWebBrowserBase *p_instance, char *p_args[], int p_arg_count, char **r_result, Bool *r_pass, Bool *r_error)
{
    bool t_success;
	t_success = true;

	// We must be given 1 argument. The argument is the handler name to add.
	if (t_success)
	{
		if (p_arg_count != 1)
		{
			/* UNCHECKED */ MCCStringClone("incorrect number of arguments", *r_result);
			t_success = false;
		}
	}

	if (t_success)
		p_instance->AddJavaScriptHandler(p_args[0]);

	*r_error = !t_success;
	*r_pass = False;
	if (*r_result == nil)
		/* UNCHECKED */ MCCStringClone("", *r_result);
}

// IM-2014-03-06: [[ revBrowserCEF ]] Removes the named handler function from the global liveCode JS object
void revBrowserRemoveJavaScriptHandler(CWebBrowserBase *p_instance, char *p_args[], int p_arg_count, char **r_result, Bool *r_pass, Bool *r_error)
{
    bool t_success;
	t_success = true;

	// We must be given 1 argument. The argument is the handler name to add.
	if (t_success)
	{
		if (p_arg_count != 1)
		{
			/* UNCHECKED */ MCCStringClone("incorrect number of arguments", *r_result);
			t_success = false;
		}
	}

	if (t_success)
		p_instance->RemoveJavaScriptHandler(p_args[0]);

	*r_error = !t_success;
	*r_pass = False;
	if (*r_result == nil)
		/* UNCHECKED */ MCCStringClone("", *r_result);
}

// Function:
//   revBrowserGet(pInstanceId, pProperty)
// Description:
//   - "busy" : Returns true if the browser is in the middle of an operation
//   - "rect" : Returns the rect of the browser, relative to the parent window
//   - "messages" : Returns true if the browser will send 'advanced' callback messages
//   - "url" : Returns the browser current URL
//   - "htmltext" : Returns the HTML source of the target browser
//   - "offline" : Returns true if the browser is set to not retrieve pages from the internet
//   - "contextmenu" : Returns true if the browser will display right-click context menu 
//   - "visible" : Returns true if the browser is visible
//   - "scrollbars" : Returns true if the browser has scrollbars showing
//   - "title" : Returns the title of the current page in the browser
//   - "newwindow" : Returns whether the browser will allow new windows to be opened
//   - "selected" : Returns the currently selected text in the browser

void revBrowserGetProp(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
    *error = False;
    *pass = False;

	BrowserProperty whichprop = BROWSERPROP_UNDEFINED;

	for (int i = 0; i < sizeof(browserProperties) / sizeof(BrowserProp); i++)
	{
		if (strcasecmp(args[0], browserProperties[i].str) == 0)
		{
			whichprop = browserProperties[i].prop;
			break;
		}	
	}

	CWebBrowserBase *t_browser;
	t_browser = p_instance;

	switch (whichprop)
	{
	case BROWSERPROP_BUSY:
		result = BoolToStr(t_browser -> GetBusy());
	break;

	case BROWSERPROP_RECT:
	{
		int t_left, t_top, t_right, t_bottom;
		t_browser -> GetRect(t_left, t_top, t_right, t_bottom);

		// IM-2014-07-09: [[ Bug 12225 ]] Convert browser rect to stack coords
		MCRectangle32 t_rect;
		t_rect.x = t_left;
		t_rect.y = t_top;
		t_rect.width = t_right - t_left;
		t_rect.height = t_bottom - t_top;

		int t_success;
		/* UNCHECKED */ WindowToStackRect(t_browser->GetWindowId(), &t_rect, &t_success);

        // AL-2013-11-01 [[ Bug 11289 ]] Use libcore methods to prevent potential buffer overflows in revbrowser
        MCCStringFormat(result,"%d,%d,%d,%d", t_rect.x, t_rect.y, t_rect.x + t_rect.width, t_rect.y + t_rect.height);
	}
	break;
	
	case BROWSERPROP_FORMATTEDRECT:
	{
		int t_left, t_top, t_right, t_bottom;
		t_browser -> GetFormattedRect(t_left, t_top, t_right, t_bottom);

		// IM-2014-07-09: [[ Bug 12225 ]] Convert browser rect to stack coords
		MCRectangle32 t_rect;
		t_rect.x = t_left;
		t_rect.y = t_top;
		t_rect.width = t_right - t_left;
		t_rect.height = t_bottom - t_top;

		int t_success;
		/* UNCHECKED */ WindowToStackRect(t_browser->GetWindowId(), &t_rect, &t_success);

        // AL-2013-11-01 [[ Bug 11289 ]] Use libcore methods to prevent potential buffer overflows in revbrowser
        MCCStringFormat(result,"%d,%d,%d,%d", t_rect.x, t_rect.y, t_rect.x + t_rect.width, t_rect.y + t_rect.height);
	}
	break;

	case BROWSERPROP_MESSAGES:
		result = BoolToStr(t_browser -> GetMessages());
	break;

	case BROWSERPROP_URL:
		result = t_browser -> GetURL();
	break;

	case BROWSERPROP_HTMLTEXT:
		result = t_browser -> GetSource();
	break;

	case BROWSERPROP_OFFLINE:
		result = BoolToStr(t_browser -> GetOffline());
	break;

	case BROWSERPROP_CONTEXTMENU:
		result = BoolToStr(t_browser -> GetContextMenu());
	break;

	case BROWSERPROP_VISIBLE:
		result = BoolToStr(t_browser -> GetVisible());
	break;

	case BROWSERPROP_VERSION:
		result = istrdup("2.0");
	break;

	case BROWSERPROP_SCROLLBARS:
		result = BoolToStr(t_browser -> GetScrollbars());
	break;

	case BROWSERPROP_SHOWBORDER:
		result = BoolToStr(t_browser -> GetBorder());
	break;

	case BROWSERPROP_SCALE:
		result = BoolToStr(t_browser -> GetScale());
	break;

	case BROWSERPROP_TITLE:
		result = t_browser -> GetTitle();
	break;

	case BROWSERPROP_NEWWINDOW:
		result = BoolToStr(t_browser -> GetNewWindow());
	break;

	case BROWSERPROP_SELECTED:
		result = t_browser -> GetSelectedText();
	break;

	case BROWSERPROP_BROWSER:
		result = t_browser -> GetBrowser();
	break;
	
	case BROWSERPROP_HSCROLL:
		result = IntToStr(t_browser -> GetHScroll());
	break;
	
	case BROWSERPROP_VSCROLL:
		result = IntToStr(t_browser -> GetVScroll());
	break;
	
	case BROWSERPROP_FORMATTEDHEIGHT:
		result = IntToStr(t_browser -> GetFormattedHeight());
	break;
	
	case BROWSERPROP_FORMATTEDWIDTH:
		result = IntToStr(t_browser -> GetFormattedWidth());
	break;
		
	case BROWSERPROP_WINDOWID:
		result = IntToStr(t_browser -> GetWindowId());
	break;
			
	case BROWSERPROP_USERAGENT:
		result = t_browser -> GetUserAgent();
	break;

	default:
		result = istrdup("unknown property");
		*error = True;
	break;
	}

	*retstring = result != NULL ? result: (char *)calloc(1,1);
}

// Property:
//   XBrowser_Get("instances")
// Description:
//   Return the list of created browser instances, one per line.
//
// Property:
//   XBrowser_Get("instance")
// Description:
//   Return the id of the active browser instance.
//
// Property:
//   XBrowser_Get("version")
// Description:
//   Returns the version of the browser external.
//
// Property:
//   XBrowser_Get("htmltext", [ pInstanceId ])
// Description:
//   Returns the HTML source of the target browser.
//
// Property:
//   XBrowser_Get("htmlimage", [ pInstanceId ])
// Description:
//   Returns the image data of what is being displayed in the browser.
//
// Otherwise the same as revBrowserGet
//
void XBrowserGetProp(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	if (nargs != 0)
	{
		if (strcasecmp(args[0], "instance") == 0)
		{
			char minst[255];
			sprintf(minst, "%i", s_browsers . GetActiveInstanceId());
			*retstring = istrdup(minst);
			*error = False;
			*pass = False;
			return;
		}
		else if (strcasecmp(args[0], "instances") == 0)
		{
			*retstring = s_browsers . GetInstanceIds();
			*error = False;
			*pass = False;
			return;
		}
		else if (strcasecmp(args[0], "htmlimage") == 0)
		{
			revBrowserSnapshot(nargs == 3 ? s_browsers . GetInstance(atoi(args[2])) : s_browsers . GetActiveInstance(), &args[1], 1, retstring, pass, error);
			return;
		}
		else if (nargs == 2)
		{
			revBrowserGetProp(s_browsers . GetInstance(atoi(args[1])), args, nargs - 1, retstring, pass, error);
			return;
		}
		else if (nargs == 1)
		{
			revBrowserGetProp(s_browsers . GetActiveInstance(), args, nargs, retstring, pass, error);
			return;
		}
	}

	*retstring = strdup("wrong number of parameters");
	*error = True;
	*pass = False;
	return;
}

void revBrowserSetProp(CWebBrowserBase *p_instance, char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;

	int numelements = sizeof(browserProperties)/sizeof(BrowserProp);
	int i;

	CWebBrowserBase *t_browser;
	t_browser = p_instance;
	
	if (nargs >= 2)
	{
		BrowserProperty whichprop  = BROWSERPROP_UNDEFINED;
		int proplen = strlen(args[0]);
		for (i = 0; i < numelements; i++)
		{
			if (strncasecmp(args[0], browserProperties[i].str,proplen) == 0 )
			{
				whichprop = browserProperties[i].prop;
				break;
			}	
		}

		switch (whichprop)
		{
		case BROWSERPROP_RECT: 
		{
			double t_left, t_top, t_right, t_bottom;

			if (sscanf(args[1], "%lf,%lf,%lf,%lf", &t_left, &t_top, &t_right, &t_bottom) == 4)
			{
				// IM-2014-07-09: [[ Bug 12225 ]] Convert stack rect to window coords
				MCRectangle32 t_rect;
				t_rect.x = (int)(t_left + 0.00000001);
				t_rect.y = (int)(t_top + 0.00000001);
				t_rect.width = ((int)(t_right + 0.00000001)) - t_rect.x;
				t_rect.height = ((int)(t_bottom + 0.00000001)) - t_rect.y;

				int t_success;
				/* UNCHECKED */ StackToWindowRect(t_browser->GetWindowId(), &t_rect, &t_success);

				t_browser -> SetRect(t_rect.x, t_rect.y, t_rect.x + t_rect.width, t_rect.y + t_rect.height);
			}
			else
			{
				result = strdup("invalid rectangle");
				*error = True;
			}
		}
		break;

		case BROWSERPROP_URL:
			t_browser -> GoURL(args[1]);
		break;
		
		case BROWSERPROP_MESSAGES:
			t_browser -> SetMessages(StrToBool(args[1]));
		break;

		case BROWSERPROP_SELECTED:
			t_browser -> SetSelectedText(args[1]);
		break;

		case BROWSERPROP_OFFLINE:
			t_browser -> SetOffline(StrToBool(args[1]));
		break;

		case BROWSERPROP_CONTEXTMENU:
			t_browser -> SetContextMenu(StrToBool(args[1]));
		break;

		case BROWSERPROP_VISIBLE:
			t_browser -> SetVisible(StrToBool(args[1]));
		break;

		case BROWSERPROP_NEWWINDOW:
			t_browser -> SetNewWindow(StrToBool(args[1]));
		break;

		case BROWSERPROP_HTMLTEXT:
			t_browser -> SetSource(args[1]);
		break;

		case BROWSERPROP_SCROLLBARS:
			t_browser -> SetScrollbars(StrToBool(args[1]));
		break;

		case BROWSERPROP_SCALE:
			t_browser -> SetScale(StrToBool(args[1]));
		break;

		case BROWSERPROP_SHOWBORDER:
			t_browser -> SetBorder(StrToBool(args[1]));
		break;

		case BROWSERPROP_BROWSER:
			t_browser -> SetBrowser(args[1]);
		break;
		
		case BROWSERPROP_VSCROLL:
			t_browser -> SetVScroll(atoi(args[1]));
		break;
		
		case BROWSERPROP_HSCROLL:
			t_browser -> SetHScroll(atoi(args[1]));
		break;

		case BROWSERPROP_WINDOWID:
			t_browser -> SetWindowId(atoi(args[1]));
		break;
		
		case BROWSERPROP_USERAGENT:
			t_browser -> SetUserAgent(args[1]);
		break;

		default:
			result = istrdup("unknown property");
			*error = True;
		break;
		}
	}
	else
	{
		result = strdup("wrong number of parameters");
		*error = True;
	}

	*retstring = result != NULL ? result : strdup("");
}

void XBrowserSetProp(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	if (nargs < 2 || nargs > 3)
	{
		*retstring = strdup("wrong number of parameters");
		*error = True;
		*pass = False;
		return;
	}

	if (strcasecmp(args[1], "instance") == 0)
	{
		s_browsers . SetActiveInstanceId(atoi(args[1]));
		*retstring = strdup("");
		*error = False;
		*pass = False;
		return;
	}

	CWebBrowserBase *t_browser;
	if (nargs == 3)
	{
		t_browser = s_browsers . GetInstance(atoi(args[2]));
		if (t_browser == NULL)
		{
			*retstring = strdup("unknown browser id");
			*error = True;
			*pass = False;
			return;
		}
	}
	else
	{
        MCAssert(nargs == 2);
		t_browser = s_browsers . GetActiveInstance();
		if (t_browser == NULL)
		{
			*retstring = strdup("no active browser");
			*error = True;
			*pass = False;
			return;
		}
	}

	revBrowserSetProp(t_browser, args, nargs, retstring, pass, error);
}

void revBrowserInstances(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_error)
{
	*r_result = s_browsers . GetInstanceIds();
	*r_pass = False;
	*r_error = False;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SHUTDOWN
//

extern "C"
{
#ifdef _WINDOWS
	void __declspec(dllexport) shutdownXtable(void);
#else
	void shutdownXtable(void) __attribute__((visibility("default")));
#endif
}

void MCCefFinalise(void);
void shutdownXtable(void)
{
	for(;;)
	{
		CWebBrowserBase *t_browser;
		t_browser = s_browsers . GetFirstInstance();
		if (t_browser == NULL)
			break;
		s_browsers . Delete(t_browser);
	}

	MCCefFinalise();
}

///////////////////////////////////////////////////////////////////////////////
//
//  DECLARATIONS
//

typedef void (*BrowserHandler)(CWebBrowserBase *p_browser, char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err);

template<BrowserHandler u_handler> void XBrowserWrapper(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	CWebBrowserBase *t_browser;
	if (p_argument_count < 1)
	{
		t_browser = s_browsers . GetActiveInstance();
		if (t_browser == NULL)
		{
			*r_result = strdup("no active browser");
			*r_err = True;
			*r_pass = False;
			return;
		}
	}	
	else
	{
		t_browser = s_browsers . GetInstance(atoi(p_arguments[0]));
		p_argument_count -= 1;
		if (t_browser == NULL)
		{
			*r_result = strdup("unknown browser id");
			*r_err = True;
			*r_pass = False;
			return;
		}
	}

	u_handler(s_browsers . GetActiveInstance(), &p_arguments[0], p_argument_count, r_result, r_pass, r_err);
}

template<BrowserHandler u_handler> void XBrowserWrapper2(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	CWebBrowserBase *t_browser;
	if (p_argument_count < 2)
	{
		t_browser = s_browsers . GetActiveInstance();
		if (t_browser == NULL)
		{
			*r_result = strdup("no active browser");
			*r_err = True;
			*r_pass = False;
			return;
		}
	}	
	else
	{
		t_browser = s_browsers . GetInstance(atoi(p_arguments[1]));
		p_argument_count -= 1;
		if (t_browser == NULL)
		{
			*r_result = strdup("unknown browser id");
			*r_err = True;
			*r_pass = False;
			return;
		}
	}

	u_handler(s_browsers . GetActiveInstance(), &p_arguments[0], p_argument_count, r_result, r_pass, r_err);
}

template<BrowserHandler u_handler> void revBrowserWrapper(char *p_arguments[], int p_argument_count, char **r_result, Bool *r_pass, Bool *r_err)
{
	if (p_argument_count < 1)
	{
		*r_result = strdup("wrong number of parameters");
		*r_err = True;
		*r_pass = False;
		return;
	}

	CWebBrowserBase *t_browser;
	t_browser = s_browsers . GetInstance(atoi(p_arguments[0]));
	if (t_browser != NULL)
		u_handler(t_browser, &p_arguments[1], p_argument_count - 1, r_result, r_pass, r_err);
	else
	{
		*r_result = strdup("unknown browser id");
		*r_err = True;
		*r_pass = True;
	}
}

EXTERNAL_BEGIN_DECLARATIONS("revBrowser")
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserOpen", revBrowserOpen)
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserOpenCef", revBrowserOpenCef)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserClose", revBrowserWrapper<revBrowserClose>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserStop", revBrowserWrapper<revBrowserStop>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserRefresh", revBrowserWrapper<revBrowserRefresh>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserNavigate", revBrowserWrapper<revBrowserNavigate>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserForward", revBrowserWrapper<revBrowserForward>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserBack", revBrowserWrapper<revBrowserBack>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserPrint", revBrowserWrapper<revBrowserPrint>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserRedraw", revBrowserWrapper<revBrowserRedraw>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserMakeTextBigger", revBrowserWrapper<revBrowserMakeTextBigger>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserMakeTextSmaller", revBrowserWrapper<revBrowserMakeTextSmaller>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserSet", revBrowserWrapper<revBrowserSetProp>)
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserGet", revBrowserWrapper<revBrowserGetProp>)
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserInstances", revBrowserInstances)
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserExecuteScript", revBrowserWrapper<revBrowserExecuteScript>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserSnapshot", revBrowserWrapper<revBrowserSnapshot>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserFind", revBrowserWrapper<revBrowserFind>)
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("revBrowserCallScript", revBrowserWrapper<revBrowserCallScript>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserAddJavaScriptHandler", revBrowserWrapper<revBrowserAddJavaScriptHandler>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("revBrowserRemoveJavaScriptHandler", revBrowserWrapper<revBrowserRemoveJavaScriptHandler>)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Open", XBrowserOpen)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Close", XBrowserWrapper<revBrowserClose> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Stop", XBrowserWrapper<revBrowserStop> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Halt", XBrowserWrapper<revBrowserStop> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Refresh", XBrowserWrapper<revBrowserRefresh> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Navigate", XBrowserWrapper2<revBrowserNavigate> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Forward", XBrowserWrapper<revBrowserForward> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Back", XBrowserWrapper<revBrowserBack> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Set", XBrowserSetProp)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Focus", XBrowserWrapper<revBrowserFocus> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Unfocus", XBrowserWrapper<revBrowserUnfocus> )
	EXTERNAL_DECLARE_FUNCTION_OBJC_UTF8("XBrowser_Get", XBrowserGetProp)
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Print", XBrowserWrapper<revBrowserPrint> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Redraw", XBrowserWrapper<revBrowserRedraw> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_MakeTextBigger", XBrowserWrapper<revBrowserMakeTextBigger> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_MakeTextSmaller", XBrowserWrapper<revBrowserMakeTextSmaller> )
	EXTERNAL_DECLARE_COMMAND_OBJC_UTF8("XBrowser_Init", XBrowserInit)
EXTERNAL_END_DECLARATIONS
