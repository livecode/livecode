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

#include <include/cef_app.h>

#include <map>

#include <core.h>

#include "libbrowser_cef.h"

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

bool MCCefV8ArrayToList(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> p_array, CefRefPtr<CefListValue> &r_list);
bool MCCefV8ObjectToDictionary(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> p_obj, CefRefPtr<CefDictionaryValue> &r_dict);

bool MCCefV8ArrayToList(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> p_array, CefRefPtr<CefListValue> &r_list)
{
	bool t_success;
	t_success = true;
	
	p_context->Enter();
	
	CefRefPtr<CefListValue> t_list;
	if (t_success)
		t_list = CefListValue::Create();
	
	uint32_t t_length;
	t_length = p_array->GetArrayLength();
	
	if (t_success)
		t_success = t_list->SetSize(t_length);
	
	for (uint32_t i = 0; i < t_length; i++)
	{
		CefRefPtr<CefV8Value> t_val;
		t_val = p_array->GetValue(i);
		
		if (t_val->IsBool())
			t_success = t_list->SetBool(i, t_val->GetBoolValue());
		else if (t_val->IsInt())
			t_success = t_list->SetInt(i, t_val->GetIntValue());
		else if (t_val->IsUInt())
			t_success = t_list->SetInt(i, t_val->GetUIntValue());
		else if (t_val->IsDouble())
			t_success = t_list->SetDouble(i, t_val->GetDoubleValue());
		else if (t_val->IsString())
			t_success = t_list->SetString(i, t_val->GetStringValue());
		else if (t_val->IsArray())
		{
			CefRefPtr<CefListValue> t_list_val;
			t_success = MCCefV8ArrayToList(p_context, t_val, t_list_val);
			if (t_success)
				t_success = t_list->SetList(i, t_list_val);
		}
		else if (t_val->IsObject())
		{
			CefRefPtr<CefDictionaryValue> t_dict_val;
			t_success = MCCefV8ObjectToDictionary(p_context, t_val, t_dict_val);
			if (t_success)
				t_success = t_list->SetDictionary(i, t_dict_val);
		}
		else
		{
			t_success = MCCefV8ValueConvertToString(p_context, t_val);
			if (t_success)
				t_success = t_list->SetString(i, t_val->GetStringValue());
		}
	}
	
	p_context->Exit();
	
	if (t_success)
		r_list = t_list;

	return t_success;
}

bool MCCefV8ObjectToDictionary(CefRefPtr<CefV8Context> p_context, CefRefPtr<CefV8Value> p_obj, CefRefPtr<CefDictionaryValue> &r_dict)
{
	bool t_success;
	t_success = true;
	
	p_context->Enter();
	
	CefRefPtr<CefDictionaryValue> t_dict;
	if (t_success)
		t_dict = CefDictionaryValue::Create();
	
	std::vector<CefString> t_keys;
	if (t_success)
		t_success = p_obj->GetKeys(t_keys);
	
	for (std::vector<CefString>::const_iterator i = t_keys.begin(); t_success && i != t_keys.end(); i++)
	{
		CefRefPtr<CefV8Value> t_val;
		t_val = p_obj->GetValue(*i);

		if (t_val->IsBool())
			t_success = t_dict->SetBool(*i, t_val->GetBoolValue());
		else if (t_val->IsInt())
			t_success = t_dict->SetInt(*i, t_val->GetIntValue());
		else if (t_val->IsUInt())
			t_success = t_dict->SetInt(*i, t_val->GetUIntValue());
		else if (t_val->IsDouble())
			t_success = t_dict->SetDouble(*i, t_val->GetDoubleValue());
		else if (t_val->IsString())
			t_success = t_dict->SetString(*i, t_val->GetStringValue());
		else if (t_val->IsArray())
		{
			CefRefPtr<CefListValue> t_list_val;
			t_success = MCCefV8ArrayToList(p_context, t_val, t_list_val);
			if (t_success)
				t_success = t_dict->SetList(*i, t_list_val);
		}
		else if (t_val->IsObject())
		{
			CefRefPtr<CefDictionaryValue> t_dict_val;
			t_success = MCCefV8ObjectToDictionary(p_context, t_val, t_dict_val);
			if (t_success)
				t_success = t_dict->SetDictionary(*i, t_dict_val);
		}
		else
		{
			t_success = MCCefV8ValueConvertToString(p_context, t_val);
			if (t_success)
				t_success = t_dict->SetString(*i, t_val->GetStringValue());
		}
	}
	
	p_context->Exit();
	
	if (t_success)
		r_dict = t_dict;

	return t_success;
}

bool MCCefV8ListToList(CefRefPtr<CefV8Context> p_context, const CefV8ValueList &p_list, CefRefPtr<CefListValue> &r_list)
{
	bool t_success;
	t_success = true;
	
	p_context->Enter();
	
	CefRefPtr<CefListValue> t_list;
	if (t_success)
		t_list = CefListValue::Create();
	
	if (t_success)
		t_success = t_list->SetSize(p_list.size());
	
	for (CefV8ValueList::const_iterator i = p_list.begin(); t_success && i != p_list.end(); i++)
	{
		CefRefPtr<CefV8Value> t_val;
		t_val = *i;
		
		uint32_t t_index;
		t_index = i - p_list.begin();
		
		if (t_val->IsBool())
			t_success = t_list->SetBool(t_index, t_val->GetBoolValue());
		else if (t_val->IsInt())
			t_success = t_list->SetInt(t_index, t_val->GetIntValue());
		else if (t_val->IsUInt())
			t_success = t_list->SetInt(t_index, t_val->GetUIntValue());
		else if (t_val->IsDouble())
			t_success = t_list->SetDouble(t_index, t_val->GetDoubleValue());
		else if (t_val->IsString())
			t_success = t_list->SetString(t_index, t_val->GetStringValue());
		else if (t_val->IsArray())
		{
			CefRefPtr<CefListValue> t_list_val;
			t_success = MCCefV8ArrayToList(p_context, t_val, t_list_val);
			if (t_success)
				t_success = t_list->SetList(t_index, t_list_val);
		}
		else if (t_val->IsObject())
		{
			CefRefPtr<CefDictionaryValue> t_dict_val;
			t_success = MCCefV8ObjectToDictionary(p_context, t_val, t_dict_val);
			if (t_success)
				t_success = t_list->SetDictionary(t_index, t_dict_val);
		}
		else
		{
			t_success = MCCefV8ValueConvertToString(p_context, t_val);
			if (t_success)
				t_success = t_list->SetString(t_index, t_val->GetStringValue());
		}
	}
	
	p_context->Exit();
	
	if (t_success)
		r_list = t_list;

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
		
		CefRefPtr<CefListValue> t_params;
		if (t_success)
			t_success = MCCefV8ListToList(CefV8Context::GetCurrentContext(), p_args, t_params);
		
		if (t_success)
			t_success = t_args->SetList(1, t_params);
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
public:
	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE { return this; }
	
	virtual void OnBeforeCommandLineProcessing(const CefString &p_process_type, CefRefPtr<CefCommandLine> p_command_line) OVERRIDE
	{
		// Turn on hi-dpi support if enabled by command-line switch
		if (p_command_line->HasSwitch(MC_CEF_HIDPI_SWITCH))
			MCCefPlatformEnableHiDPI();
	}

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
		else if (t_message_name == MC_CEFMSG_SET_JS_HANDLER_LIST)
		{
			bool t_success;
			
			t_success = SetLiveCodeHandlers(p_browser, p_message->GetArgumentList()->GetList(0));
			
			/* UNCHECKED */
			
			return true;
		}
		else
			return CefRenderProcessHandler::OnProcessMessageReceived(p_browser, p_source_process, p_message);
	}
	
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
		
		return t_success;
	}
	
	bool DeleteLiveCodeGlobalObj(CefRefPtr<CefV8Context> p_context)
	{
		CefString t_obj_name("liveCode");
		
		bool t_success;
		t_success = true;
		
		CefRefPtr<CefV8Value> t_global;
		t_global = p_context->GetGlobal();
		
		t_success = t_global != nil;
		
		if (t_success)
		{
			if (t_global->HasValue(t_obj_name))
				t_success = t_global->DeleteValue(t_obj_name);
		}
		
		return t_success;
	}
	
	bool AddHandler(CefRefPtr<CefV8Value> p_container, const CefString &p_name)
	{
		bool t_success;
		t_success = true;
		
		if (!p_container->HasValue(p_name))
		{
			CefRefPtr<CefV8Handler> t_handler;
			t_success = nil != (t_handler = new (nothrow) MCCefLCFuncHandler(p_name));
			
			CefRefPtr<CefV8Value> t_func;
			if (t_success)
				t_success = nil != (t_func = CefV8Value::CreateFunction(p_name, t_handler));
			
			if (t_success)
				t_success = p_container->SetValue(p_name, t_func, V8_PROPERTY_ATTRIBUTE_NONE);
		}
		
		return t_success;
	}
	
	bool AddHandlerList(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefV8Value> p_container, CefRefPtr<CefListValue> p_list)
	{
		if (p_list == nil)
			return true;
		
		size_t t_size;
		t_size = p_list->GetSize();
		
		for (uint32_t i = 0; i < t_size; i++)
		{
			if (p_list->GetType(i) != VTYPE_STRING)
				return false;
			if (p_list->GetString(i).empty())
				return false;
			if (!AddHandler(p_container, p_list->GetString(i)))
				return false;
		}
		
		return true;
	}
	
	bool SetLiveCodeHandlers(CefRefPtr<CefBrowser> p_browser, CefRefPtr<CefListValue> p_handlers)
	{
		bool t_success;
		t_success = true;
		
		bool t_entered;
		t_entered = false;
		
		CefRefPtr<CefV8Context> t_context;
		t_success = GetV8Context(p_browser, t_context);
		
		if (t_success)
			t_success = t_entered = t_context->Enter();
		
		if (t_success)
			t_success = DeleteLiveCodeGlobalObj(t_context);
		
		CefRefPtr<CefV8Value> t_livecode_obj;
		if (t_success)
			t_success = GetLiveCodeGlobalObj(t_context, t_livecode_obj);
		
		// Add new handlers
		if (t_success)
			if (p_handlers != nil && p_handlers->GetSize() > 0)
				t_success = AddHandlerList(p_browser, t_livecode_obj, p_handlers);
		
		if (t_entered)
			t_context->Exit();
		
		return t_success;
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
