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

#include "foundation.h"

#include <include/cef_app.h>

#include <map>

#include "cefbrowser_msg.h"

////////////////////////////////////////////////////////////////////////////////

// Use JS "String" function to convert values to string
bool MCCefV8ValueConvertToString(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> &x_value)
{
	if (x_value->IsString())
		return true;

	bool t_success;
	t_success = true;

	CefRefPtr<CefV8Exception> t_exception;

	CefRefPtr<CefV8Value> t_string_func;
	if (t_success)
		t_success = p_context->Eval("String", "", 1, t_string_func, t_exception);

	CefRefPtr<CefV8Value> t_string_value;
	if (t_success)
	{
		CefV8ValueList t_args;
		t_args.push_back(x_value);

		t_string_value = t_string_func->ExecuteFunctionWithContext(p_context, NULL, t_args);

		t_success = t_string_value != NULL;
	}

	if (t_success)
		x_value = t_string_value;

	return t_success;
}

bool MCCefHandleExecuteScript(CefRefPtr<CefBrowser> p_browser, const CefString &p_script, CefRefPtr<CefV8Value> &r_return_value)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefFrame> t_frame;
	t_frame = p_browser->GetMainFrame();

	CefRefPtr<CefV8Context> t_context;
	t_context = t_frame->GetV8Context();

	CefRefPtr<CefV8Value> t_return_value;
	CefRefPtr<CefV8Exception> t_exception;

	t_success = t_context != NULL;

	if (t_success)
		t_success = t_context->Eval(p_script, "", 1, t_return_value, t_exception);

	if (t_success)
		r_return_value = t_return_value;

	return t_success;
}

//////////

bool MCCefListToV8List(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefListValue> p_list, CefV8ValueList &x_list)
{
	bool t_success;
	t_success = true;

	p_context->Enter();

	size_t t_size;
	t_size = p_list->GetSize();

	x_list.clear();

	for (uint32_t i = 0; t_success && i < t_size; i++)
	{
		CefValueType t_type;
		t_type = p_list->GetType(i);

		CefRefPtr<CefV8Value> t_v8;

		switch (t_type)
		{
		case VTYPE_NULL:
			t_v8 = CefV8Value::CreateNull();
			break;

		case VTYPE_BOOL:
			t_v8 = CefV8Value::CreateBool(p_list->GetBool(i));
			break;

		case VTYPE_INT:
			t_v8 = CefV8Value::CreateInt(p_list->GetInt(i));
			break;

		case VTYPE_DOUBLE:
			t_v8 = CefV8Value::CreateDouble(p_list->GetDouble(i));
			break;

		case VTYPE_STRING:
			t_v8 = CefV8Value::CreateString(p_list->GetString(i));
			break;

		case VTYPE_BINARY:
		case VTYPE_DICTIONARY:
		case VTYPE_LIST:
			/* TODO - IMPLEMENT */
			t_success = false;
			break;
		case VTYPE_INVALID:
			t_success = false;
		}

		t_success = t_v8 != NULL;

		if (t_success)
			x_list.push_back(t_v8);
	}

	p_context->Exit();

	return t_success;
}

bool MCCefHandleCallScript(CefRefPtr<CefBrowser> p_browser, const CefString &p_function_name, CefRefPtr<CefListValue> p_args, CefString &r_return_value)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefFrame> t_frame;
	t_frame = p_browser->GetMainFrame();

	CefRefPtr<CefV8Context> t_context;
	t_context = t_frame->GetV8Context();

	CefRefPtr<CefV8Value> t_function;
	CefV8ValueList t_args;

	CefRefPtr<CefV8Value> t_return_value;
	CefRefPtr<CefV8Exception> t_exception;

	t_success = t_context != NULL;

	if (t_success)
		t_success = t_context->Eval(p_function_name, "", 1, t_function, t_exception);

	if (t_success)
		t_success = MCCefListToV8List(t_context, p_args, t_args);

	if (t_success)
	{
		t_return_value = t_function->ExecuteFunctionWithContext(t_context, NULL, t_args);
		t_success = t_return_value != NULL;
	}

	if (t_success && !t_return_value->IsString())
		t_success = MCCefV8ValueConvertToString(t_context, t_return_value);

	if (t_success)
		r_return_value = t_return_value->GetStringValue();

	return t_success;
}

//////////

class MCGetSelectedTextDOMVisitor : public CefDOMVisitor
{
private:
	CefString m_selected_text;

public:
	CefString GetSelectedText() { return m_selected_text; };

	void Visit(CefRefPtr<CefDOMDocument> p_document) OVERRIDE
	{
		if (p_document->HasSelection())
			m_selected_text = p_document->GetSelectionAsText();
	}

	IMPLEMENT_REFCOUNTING(MCGetSelectedTextDOMVisitor);
};

bool MCCefHandleGetSelectedText(CefRefPtr<CefBrowser> p_browser, CefString &r_return_value)
{
	bool t_success;
	t_success = true;

	MCGetSelectedTextDOMVisitor *t_dom_visitor;
	t_dom_visitor = new (nothrow) MCGetSelectedTextDOMVisitor();
	t_success = t_dom_visitor != nil;

	CefRefPtr<CefDOMVisitor> t_visitor_ref;
	t_visitor_ref = t_dom_visitor;

	if (t_success)
	{
		p_browser->GetMainFrame()->VisitDOM(t_visitor_ref);

		r_return_value = t_dom_visitor->GetSelectedText();
	}

	return t_success;
}

//////////

class MCGetTitleDOMVisitor : public CefDOMVisitor
{
private:
	CefString m_title;

public:
	CefString GetTitle() { return m_title; };

	void Visit(CefRefPtr<CefDOMDocument> p_dom) OVERRIDE
	{
		m_title = p_dom->GetTitle();
	}

	IMPLEMENT_REFCOUNTING(MCGetTitleDOMVisitor);
};

bool MCCefHandleGetTitle(CefRefPtr<CefBrowser> p_browser, CefString &r_return_value)
{
	bool t_success;
	t_success = true;

	MCGetTitleDOMVisitor *t_dom_visitor;
	t_dom_visitor = new (nothrow) MCGetTitleDOMVisitor();
	t_success = t_dom_visitor != nil;

	CefRefPtr<CefDOMVisitor> t_visitor_ref;
	t_visitor_ref = t_dom_visitor;

	if (t_success)
	{
		p_browser->GetMainFrame()->VisitDOM(t_visitor_ref);

		r_return_value = t_dom_visitor->GetTitle();
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCefSendStringResult(CefRefPtr<CefBrowser> p_browser, bool p_success, const CefString &p_return_value)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_RESULT);
	t_success = t_message != nil;
	
	if (t_success)
	{
		CefRefPtr<CefListValue> t_args;

		t_args = t_message->GetArgumentList();

		t_success = t_args->SetBool(0, p_success) &&
			t_args->SetString(1, p_return_value);
	}

	if (t_success)
		t_success = p_browser->SendProcessMessage(PID_BROWSER, t_message);
	
	return t_success;
}

bool MCCefSendIntResult(CefRefPtr<CefBrowser> p_browser, bool p_success, int p_return_value)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_RESULT);
	t_success = t_message != nil;
	
	if (t_success)
	{
		CefRefPtr<CefListValue> t_args;

		t_args = t_message->GetArgumentList();

		t_success = t_args->SetBool(0, p_success) &&
			t_args->SetInt(1, p_return_value);
	}

	if (t_success)
		t_success = p_browser->SendProcessMessage(PID_BROWSER, t_message);
	
	return t_success;
}

bool MCCefSendHandlerMessage(CefRefPtr<CefBrowser> p_browser, const CefString &p_handler, const CefV8ValueList &p_args)
{
	bool t_success;
	t_success = true;

	CefRefPtr<CefProcessMessage> t_message;
	t_message = CefProcessMessage::Create(MC_CEFMSG_JS_HANDLER);
	t_success = t_message != nil;
	
	if (t_success)
	{
		CefRefPtr<CefListValue> t_args;

		t_args = t_message->GetArgumentList();

		t_success = t_args->SetString(0, p_handler);

		for (CefV8ValueList::const_iterator i = p_args.begin(); t_success && i != p_args.end(); i++)
		{
			CefRefPtr<CefV8Value> t_val;
			t_val = *i;

			t_success = MCCefV8ValueConvertToString(CefV8Context::GetCurrentContext(), t_val);
			if (t_success)
				t_success = t_args->SetString((i - p_args.begin()) + 1, t_val->GetStringValue());
		}
	}

	if (t_success)
		t_success = p_browser->SendProcessMessage(PID_BROWSER, t_message);
	
	return t_success;
}

class MCCefLCFuncHandler : public CefV8Handler
{
private:
	CefString m_name;

public:
	MCCefLCFuncHandler(const CefString &p_name)
	{
		m_name = p_name;
	}

	virtual bool Execute(const CefString &p_name, CefRefPtr<CefV8Value> p_object, const CefV8ValueList &p_args, CefRefPtr<CefV8Value> &r_retval, CefString &r_exception) OVERRIDE
	{
		if (p_name == m_name)
		{
			/* UNCHECKED */ MCCefSendHandlerMessage(CefV8Context::GetCurrentContext()->GetBrowser(), m_name, p_args);

			r_retval = CefV8Value::CreateNull();

			return true;
		}

		return false;
	}

	IMPLEMENT_REFCOUNTING(MCCefLCFuncHandler);
};

class MCCefRenderApp : public CefApp, CefRenderProcessHandler
{
private:
	bool GetV8Context(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefV8Context> &r_context)
	{
		CefRefPtr<CefFrame> t_frame;
		t_frame = p_browser->GetMainFrame();
		
		if (t_frame == nil)
			return false;
		
		CefRefPtr<CefV8Context> t_context;
		t_context = t_frame->GetV8Context();
		
		if (t_context == nil)
			return false;
		
		r_context = t_context;
		
		return true;
	}
	
	bool GetLiveCodeGlobalObj(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> &r_global_obj)
	{
		CefString t_obj_name("liveCode");
		
		if (!p_context->Enter())
			return false;
		
		bool t_success;
		t_success = true;
		
		CefRefPtr<CefV8Value> t_global;
		t_global = p_context->GetGlobal();
		
		t_success = t_global != nil;
		
		CefRefPtr<CefV8Value> t_obj;
		
		if (t_success)
		{
			if (t_global->HasValue(t_obj_name))
			{
				t_obj = t_global->GetValue(t_obj_name);
				t_success = t_obj != nil;
			}
			else
			{
				t_obj = CefV8Value::CreateObject(nullptr, nullptr);
				t_success = t_obj != nil;
				
				if (t_success)
					t_success = t_global->SetValue(t_obj_name, t_obj, V8_PROPERTY_ATTRIBUTE_READONLY);
			}
		}
		
		if (t_success)
			r_global_obj = t_obj;
		
		p_context->Exit();
		
		return t_success;
	}
	
	// IM-2014-03-13: [[ revBrowserCEF ]] Add the named handler to the liveCode object browser's JavaScript context
	bool AddLiveCodeHandler(CefRefPtr<CefBrowser> p_browser, const CefString &p_name)
	{
		bool t_success;
		t_success = true;
		
		bool t_entered;
		t_entered = false;
		
		CefRefPtr<CefV8Context> t_context;
		t_success = GetV8Context(p_browser, t_context);
		
		if (t_success)
			t_success = t_entered = t_context->Enter();
		
		CefRefPtr<CefV8Value> t_livecode_obj;
		if (t_success)
			t_success = GetLiveCodeGlobalObj(t_context, t_livecode_obj);
		
		if (t_success)
		{
			if (!t_livecode_obj->HasValue(p_name))
			{
				CefRefPtr<CefV8Handler> t_handler;
				t_success = nil != (t_handler = new (nothrow) MCCefLCFuncHandler(p_name));
				
				CefRefPtr<CefV8Value> t_func;
				if (t_success)
					t_success = nil != (t_func = CefV8Value::CreateFunction(p_name, t_handler));
				
				if (t_success)
					t_success = t_livecode_obj->SetValue(p_name, t_func, V8_PROPERTY_ATTRIBUTE_NONE);
			}
		}
		
		if (t_entered)
			t_context->Exit();
		
		return t_success;
	}
	
	// IM-2014-03-13: [[ revBrowserCEF ]] Remove the named handler from the liveCode object browser's JavaScript context
	bool RemoveLiveCodeHandler(CefRefPtr<CefBrowser> p_browser, const CefString &p_name)
	{
		bool t_success;
		t_success = true;
		
		bool t_entered;
		t_entered = false;
		
		CefRefPtr<CefV8Context> t_context;
		t_success = GetV8Context(p_browser, t_context);
		
		if (t_success)
			t_success = t_entered = t_context->Enter();
		
		CefRefPtr<CefV8Value> t_livecode_obj;
		if (t_success)
			t_success = GetLiveCodeGlobalObj(t_context, t_livecode_obj);
		
		if (t_success)
		{
			if (t_livecode_obj->HasValue(p_name))
				t_success = t_livecode_obj->DeleteValue(p_name);
		}
		
		if (t_entered)
			t_context->Exit();
		
		return t_success;
	}

public:
	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE { return this; }

	// RenderProcessHandler interface
	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> p_browser, CefProcessId p_source_process, CefRefPtr<CefProcessMessage> p_message) OVERRIDE
	{
		CefString t_message_name;
		t_message_name = p_message->GetName();

		if (t_message_name == MC_CEFMSG_EXECUTE_SCRIPT)
		{
			bool t_success;
			CefRefPtr<CefV8Value> t_return_value;

			t_success = MCCefHandleExecuteScript(p_browser, p_message->GetArgumentList()->GetString(0), t_return_value);

			if (t_success && t_return_value->IsInt())
			{
				/* UNCHECKED */ MCCefSendIntResult(p_browser, t_success, t_return_value->GetIntValue());
				return true;
			}

			CefString t_return_string;
			if (t_success)
				t_success = MCCefV8ValueConvertToString(p_browser->GetMainFrame()->GetV8Context(), t_return_value);

			if (t_success)
				t_return_string = t_return_value->GetStringValue();
			/* UNCHECKED */ MCCefSendStringResult(p_browser, t_success, t_return_string);

			return true;
		}
		else if (t_message_name == MC_CEFMSG_CALL_SCRIPT)
		{
			bool t_success;
			CefString t_return_value;

			CefRefPtr<CefListValue> t_args_list;
			t_args_list = p_message->GetArgumentList();
			t_success = MCCefHandleCallScript(p_browser, t_args_list->GetString(0), t_args_list->GetList(1), t_return_value);

			/* UNCHECKED */ MCCefSendStringResult(p_browser, t_success, t_return_value);

			return true;
		}
		else if (t_message_name == MC_CEFMSG_GET_SELECTED_TEXT)
		{
			bool t_success;
			CefString t_return_value;

			t_success = MCCefHandleGetSelectedText(p_browser, t_return_value);

			/* UNCHECKED */ MCCefSendStringResult(p_browser, t_success, t_return_value);

			return true;
		}
		else if (t_message_name == MC_CEFMSG_GET_TITLE)
		{
			bool t_success;
			CefString t_return_value;

			t_success = MCCefHandleGetTitle(p_browser, t_return_value);

			/* UNCHECKED */ MCCefSendStringResult(p_browser, t_success, t_return_value);

			return true;
		}
		else if (t_message_name == MC_CEFMSG_ENABLE_JS_HANDLER)
		{
			bool t_success;

			CefString t_handler;
			t_handler = p_message->GetArgumentList()->GetString(0);

			bool t_enable;
			t_enable = p_message->GetArgumentList()->GetBool(1);

			if (t_enable)
				t_success = AddLiveCodeHandler(p_browser, t_handler);
			else
				t_success = RemoveLiveCodeHandler(p_browser, t_handler);

			///* UNCHECKED */ MCCefSendIntResult(p_browser, t_success, 0);

			return true;
		}
		else
			return CefRenderProcessHandler::OnProcessMessageReceived(p_browser, p_source_process, p_message);
	}

	IMPLEMENT_REFCOUNTING(MCCefRenderApp);
};

////////////////////////////////////////////////////////////////////////////////

// IM-2014-03-13: [[ revBrowserCEF ]] create the app handler
bool MCCefCreateApp(CefRefPtr<CefApp> &r_app)
{
	MCCefRenderApp *t_app;
	t_app = new (nothrow) MCCefRenderApp();
	
	if (t_app == nil)
	{
		//MCLog("failed to create app", nil);
		return false;
	}
	
	r_app = t_app;
	
	return true;
}
