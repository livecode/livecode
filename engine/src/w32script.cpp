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

#include "prefix.h"

#include <objbase.h>
#include <oaidl.h>
#include <activscp.h>

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "mcerror.h"
#include "ans.h"
#include "stack.h"
#include "stacklst.h"
#include "dispatch.h"
#include "globals.h"
#include "util.h"

#include "scriptenvironment.h"

#include "w32dc.h"

///////////////////////////////////////////////////////////////////////////////

BSTR ConvertUTF8ToBSTR(const char *p_string)
{
	unsigned int t_length;
	t_length = UTF8ToUnicode(p_string, strlen(p_string), NULL, 0);

	BSTR t_result;
	t_result = SysAllocStringLen(NULL, t_length / 2);
	if (t_result != NULL)
		UTF8ToUnicode(p_string, strlen(p_string), (unsigned short *)t_result, t_length);

	return t_result;
}

LPOLESTR ConvertUTF8ToOLESTR(const char *p_string)
{
	unsigned int t_length;
	t_length = UTF8ToUnicode(p_string, strlen(p_string), NULL, 0);

	LPOLESTR t_result;
	t_result = new (nothrow) OLECHAR[t_length / 2 + 1];
	if (t_result != NULL)
		UTF8ToUnicode(p_string, strlen(p_string) + 1, (unsigned short *)t_result, t_length + 2);

	return t_result;
}

char *ConvertBSTRToUTF8(BSTR p_string)
{
	unsigned int t_length;
	t_length = UnicodeToUTF8((unsigned short*)p_string, SysStringLen(p_string) * 2, NULL, 0);

	char *t_result;
	t_result = new (nothrow) char[t_length + 1];
	if (t_result != NULL)
	{
		UnicodeToUTF8((unsigned short*)p_string, SysStringLen(p_string) * 2, t_result, t_length);
		t_result[t_length] = '\0';
	}

	return t_result;
}

///////////////////////////////////////////////////////////////////////////////

struct ActiveScriptMethod
{
	LPOLESTR name;
	MCScriptEnvironmentCallback callback;
};

class ActiveScriptDispatch: public IDispatch
{
public:
	ActiveScriptDispatch(void);
	~ActiveScriptDispatch(void);

	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP GetTypeInfoCount(unsigned int* r_count);
	STDMETHODIMP GetTypeInfo(unsigned int p_info_index, LCID p_locale, ITypeInfo** r_type_info);
	STDMETHODIMP GetIDsOfNames(REFIID p_interface, LPOLESTR* r_names, UINT p_name_count, LCID p_locale, DISPID* r_disp_ids);
	STDMETHODIMP Invoke(DISPID p_member, REFIID p_interface, LCID p_locale, WORD p_flags, DISPPARAMS* p_parameters, VARIANT* r_result, EXCEPINFO* r_execption, unsigned int* r_error);

	bool Define(const char *p_function, MCScriptEnvironmentCallback p_callback);

private:
	ULONG m_references;

	ActiveScriptMethod *m_methods;
	unsigned int m_method_count;
};

ActiveScriptDispatch::ActiveScriptDispatch(void)
{
	m_references = 0;
	m_methods = NULL;
	m_method_count = 0;
}

ActiveScriptDispatch::~ActiveScriptDispatch(void)
{
	for(unsigned int i = 0; i < m_method_count; ++i)
		delete m_methods[i] . name;

	delete m_methods;
}

// The standard COM QueryInterface method. In this case, we only implement the
// IDispatch interface, which in turn inherits from IUnknown, therefore there
// is no need to check for any others here.
//
HRESULT ActiveScriptDispatch::QueryInterface(REFIID p_iid, void** r_interface)
{
	if (IsEqualIID(p_iid, IID_IUnknown) || IsEqualIID(p_iid, IID_IDispatch))
	{
		*r_interface = this;
		AddRef();
		return S_OK;
	}

	*r_interface = NULL;
	return E_NOINTERFACE;
}

// Increase our reference count.
//
ULONG ActiveScriptDispatch::AddRef(void)
{
	return ++m_references;
}

// Decrease our reference count and destroy the object if it reaches zero.
//
ULONG ActiveScriptDispatch::Release(void)
{
	if (--m_references == 0)
	{
		delete this;
		return 0;
	}

	return m_references;
}

// Return the number of 'ITypeInfo' records for this object. This can be used by
// users of the object to accelerate dispatch. Since we are in no way interested
// in speed here, there is no need to do anything with this.
//
HRESULT ActiveScriptDispatch::GetTypeInfoCount(unsigned int* r_count)
{
	if (r_count == NULL)
		return E_INVALIDARG;

	*r_count = 0;
	return S_OK;
}

// Return the given ITypeInfo interface for the requested item. This routine is
// only ever called if GetTypeInfoCount() > 0. In our case, it is zero so we
// leave this routine unimplemented.
//
HRESULT ActiveScriptDispatch::GetTypeInfo(unsigned p_info_index, LCID p_locale, ITypeInfo** r_type_info)
{
	return E_NOTIMPL;
}

// Map a list of names to fixed integers. This enables faster execution of the
// main Invoke method as the caller can cache the name lookups.
//
HRESULT ActiveScriptDispatch::GetIDsOfNames(REFIID p_interface, LPOLESTR* p_names, UINT p_name_count, LCID p_locale, DISPID* r_disp_ids)
{
	HRESULT t_result;
	t_result = S_OK;

	for(unsigned int i = 0; i < p_name_count; ++i)
	{
		unsigned int t_method_index;
		
		for(t_method_index = 0; t_method_index < m_method_count; ++t_method_index)
			if (wcscmp(p_names[i], m_methods[t_method_index] . name) == 0)
				break;

		if (t_method_index == m_method_count)
		{
			r_disp_ids[i] = DISPID_UNKNOWN;
			t_result = DISP_E_UNKNOWNNAME;
		}
		else
			r_disp_ids[i] = t_method_index;
	}

	return t_result;
}

// The main method for invoking a method on this object. Here p_member will be an ID
// returned by GetIDsOfNames for a given name string.
//
HRESULT ActiveScriptDispatch::Invoke(DISPID p_member, REFIID p_interface, LCID p_locale, WORD p_flags, DISPPARAMS* p_parameters, VARIANT* r_result, EXCEPINFO* r_exception, unsigned int *r_arg_error)
{
	if ((p_flags & DISPATCH_METHOD) == 0)
		return DISP_E_MEMBERNOTFOUND;

	if (p_parameters -> cNamedArgs != 0)
		return DISP_E_NONAMEDARGS;

	if ((unsigned)p_member >= m_method_count)
		return DISP_E_MEMBERNOTFOUND;

	HRESULT t_result;
	t_result = S_OK;

	char **t_str_parameters;
	t_str_parameters = new (nothrow) char *[p_parameters -> cArgs];
	memset(t_str_parameters, 0, sizeof(char *) * p_parameters -> cArgs);
	for(unsigned int i = 0; i < p_parameters -> cArgs; ++i)
	{
		VARIANT t_dst_variant;
		VariantInit(&t_dst_variant);
		
		HRESULT t_conv_result;
		t_conv_result = VariantChangeType(&t_dst_variant, &p_parameters -> rgvarg[i], VARIANT_ALPHABOOL, VT_BSTR);
		if (t_conv_result != S_OK)
		{
			t_result = t_conv_result;
			*r_arg_error = i;
		}
		else
		{
			t_str_parameters[p_parameters -> cArgs - i - 1] = ConvertBSTRToUTF8(t_dst_variant . bstrVal);
			VariantClear(&t_dst_variant);
		}
	}

	if (t_result == S_OK)
	{
		char *t_str_result;
		t_str_result = m_methods[p_member] . callback(t_str_parameters, p_parameters -> cArgs);

		VariantInit(r_result);
		if (t_str_result != NULL)
		{
			r_result -> vt = VT_BSTR;
			r_result -> bstrVal = ConvertUTF8ToBSTR(t_str_result);
		}
	}

	for(unsigned int i = 0; i < p_parameters -> cArgs; ++i)
		delete t_str_parameters[i];

	delete t_str_parameters;

	return t_result;
}

bool ActiveScriptDispatch::Define(const char *p_function, MCScriptEnvironmentCallback p_callback)
{
	ActiveScriptMethod *t_new_methods;
	t_new_methods = (ActiveScriptMethod*)realloc(m_methods, sizeof(ActiveScriptMethod) * (m_method_count + 1));
	if (t_new_methods == NULL)
		return false;

	t_new_methods[m_method_count] . name = ConvertUTF8ToOLESTR(p_function);
	t_new_methods[m_method_count] . callback = p_callback;

	m_methods = t_new_methods;
	m_method_count += 1;
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////

class ActiveScriptSite: public IActiveScriptSite, public IActiveScriptSiteWindow
{
public:
	ActiveScriptSite(ActiveScriptDispatch *p_dispatch);
	~ActiveScriptSite(void);

	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP GetLCID(LCID* r_locale);
	STDMETHODIMP GetItemInfo(LPCOLESTR p_name, DWORD p_mask, IUnknown **r_unknown, ITypeInfo **r_type_info);
	STDMETHODIMP GetDocVersionString(BSTR* r_version_string);
	STDMETHODIMP OnScriptTerminate(const VARIANT* p_result, const EXCEPINFO* p_exception);
	STDMETHODIMP OnStateChange(SCRIPTSTATE p_script_state);
	STDMETHODIMP OnScriptError(IActiveScriptError* p_error);
	STDMETHODIMP OnEnterScript(void);
	STDMETHODIMP OnLeaveScript(void);

	STDMETHODIMP GetWindow(HWND *r_window);
	STDMETHODIMP EnableModeless(BOOL p_enable);

private:
	ULONG m_references;

	ActiveScriptDispatch *m_dispatch;
};

ActiveScriptSite::ActiveScriptSite(ActiveScriptDispatch *p_dispatch)
{
	m_references = 0;
	m_dispatch = p_dispatch;
	m_dispatch -> AddRef();
}

ActiveScriptSite::~ActiveScriptSite(void)
{
	m_dispatch -> Release();
}

HRESULT ActiveScriptSite::QueryInterface(REFIID p_iid, void** r_interface)
{
	if (IsEqualIID(p_iid, IID_IUnknown))
	{
		*r_interface = this;
		AddRef();
		return S_OK;
	}
	else if (IsEqualIID(p_iid, IID_IActiveScriptSite))
	{
		*r_interface = static_cast<IActiveScriptSite *>(this);
		AddRef();
		return S_OK;
	}
	else if (IsEqualIID(p_iid, IID_IActiveScriptSiteWindow))
	{
		*r_interface = static_cast<IActiveScriptSiteWindow *>(this);
		AddRef();
		return S_OK;
	}

	*r_interface = NULL;
	return E_NOINTERFACE;
}

ULONG ActiveScriptSite::AddRef(void)
{
	return ++m_references;
}

ULONG ActiveScriptSite::Release(void)
{
	if (--m_references == 0)
	{
		delete this;
		return 0;
	}

	return m_references;
}

HRESULT ActiveScriptSite::GetLCID(LCID* r_locale)
{
	return r_locale == NULL ? E_POINTER : E_NOTIMPL;
}

HRESULT ActiveScriptSite::GetItemInfo(LPCOLESTR p_name, DWORD p_mask, IUnknown** r_unknown, ITypeInfo** r_type_info)
{
	if ((p_mask & SCRIPTINFO_IUNKNOWN) != 0)
	{
		if (r_unknown == NULL)
			return E_INVALIDARG;

		if (wcscmp(OLESTR("__REVOLUTION_HOST__"), p_name) == 0)
		{
			m_dispatch -> QueryInterface(IID_IUnknown, (void **)r_unknown);
			return S_OK;
		}
		else
			*r_unknown = NULL;
	}

	if ((p_mask & SCRIPTINFO_ITYPEINFO) != 0)
	{
		if (r_type_info == NULL)
			return E_INVALIDARG;

		*r_type_info = NULL;
	}

	return TYPE_E_ELEMENTNOTFOUND;
}

HRESULT ActiveScriptSite::GetDocVersionString(BSTR* r_version_string)
{
	return r_version_string == NULL ? E_POINTER : E_NOTIMPL;
}

HRESULT ActiveScriptSite::OnScriptTerminate(const VARIANT* p_result, const EXCEPINFO* p_exception)
{
	return S_OK;
}

HRESULT ActiveScriptSite::OnStateChange(SCRIPTSTATE p_state)
{
	return S_OK;
}

HRESULT ActiveScriptSite::OnScriptError(IActiveScriptError *p_error)
{
	EXCEPINFO t_info;
	p_error -> GetExceptionInfo(&t_info);
	return S_OK;
}

HRESULT ActiveScriptSite::OnEnterScript(void)
{
	return S_OK;
}

HRESULT ActiveScriptSite::OnLeaveScript(void)
{
	return S_OK;
}

HRESULT ActiveScriptSite::GetWindow(HWND *r_window)
{
	if (MCdefaultstackptr != NULL)
		*r_window = (HWND)MCdefaultstackptr -> getwindow() -> handle . window;
	else if (MCtopstackptr != NULL)
		*r_window = (HWND)MCtopstackptr -> getwindow() -> handle . window;
	else
		*r_window = NULL;

	return S_OK;
}

HRESULT ActiveScriptSite::EnableModeless(BOOL p_enable)
{
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

class MCWindowsActiveScriptEnvironment: public MCScriptEnvironment
{
public:
	MCWindowsActiveScriptEnvironment(void);

	void Retain(void);
	void Release(void);

	bool Initialize(OLECHAR *p_engine);
	void Finalize(void);

	bool Define(const char* p_function, MCScriptEnvironmentCallback p_callback);

	void Run(MCStringRef p_script, MCStringRef& r_out);
	char *Call(const char* p_method, const char** p_arguments, unsigned int p_argument_count);

private:
	unsigned int m_references;

	IActiveScript *m_environment;
	IActiveScriptParse *m_parser;
	ActiveScriptDispatch *m_dispatch;
};

MCWindowsActiveScriptEnvironment::MCWindowsActiveScriptEnvironment(void)
{
	m_environment = NULL;
	m_parser = NULL;
	m_dispatch = NULL;
	m_references = 0;
}

void MCWindowsActiveScriptEnvironment::Retain(void)
{
	m_references += 1;
}

void MCWindowsActiveScriptEnvironment::Release(void)
{
	if (--m_references == 0)
	{
		Finalize();
		delete this;
	}
}

bool MCWindowsActiveScriptEnvironment::Initialize(OLECHAR *p_language)
{
	HRESULT t_result;
	t_result = S_OK;

	ActiveScriptDispatch *t_dispatch;
	t_dispatch = NULL;
	if (t_result == S_OK)
	{
		t_dispatch = new (nothrow) ActiveScriptDispatch;
		if (t_dispatch != NULL)
			t_dispatch -> AddRef();
		else
			t_result = E_OUTOFMEMORY;
	}

	ActiveScriptSite *t_site;
	t_site = NULL;
	if (t_result == S_OK)
	{
		t_site = new (nothrow) ActiveScriptSite(t_dispatch);
		if (t_site != NULL)
			t_site -> AddRef();
		else
			t_result = E_OUTOFMEMORY;
	}

	CLSID t_class;
	if (t_result == S_OK)
		t_result = CLSIDFromProgID(p_language, &t_class);

	IActiveScript* t_environment;
	t_environment = NULL;
	if (t_result == S_OK)
		t_result = CoCreateInstance(t_class, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&t_environment);

	IActiveScriptParse* t_parser;
	t_parser = NULL;
	if (t_result == S_OK)
		t_result = t_environment -> QueryInterface(IID_IActiveScriptParse, (void **)&t_parser);

	if (t_result == S_OK)
		t_result = t_environment -> SetScriptSite(t_site);

	if (t_result == S_OK)
		t_result = t_parser -> InitNew();

	if (t_result == S_OK)
		t_result = t_environment -> AddNamedItem(OLESTR("__REVOLUTION_HOST__"), SCRIPTITEM_GLOBALMEMBERS | SCRIPTITEM_ISVISIBLE);

	if (t_result == S_OK)
		t_result = t_environment -> SetScriptState(SCRIPTSTATE_STARTED);

	if (t_result == S_OK)
	{
		m_environment = t_environment;
		m_parser = t_parser;
		m_dispatch = t_dispatch;
	}
	else
	{
		if (t_dispatch != NULL)
			t_dispatch -> Release();

		if (t_parser != NULL)
			t_parser -> Release();

		if (t_environment != NULL)
			t_environment -> Release();
	}

	if (t_site != NULL)
		t_site -> Release();
	
	return t_result == S_OK;
}

void MCWindowsActiveScriptEnvironment::Finalize(void)
{
	m_parser -> Release();
	m_parser = NULL;

	m_environment -> Close();
	m_environment -> Release();
	m_environment = NULL;

	m_dispatch -> Release();
	m_dispatch = NULL;
}

void MCWindowsActiveScriptEnvironment::Run(MCStringRef p_script, MCStringRef& r_out)
{
    MCAutoStringRefAsWString t_script;
    if (!t_script.Lock(p_script))
        return;

	EXCEPINFO t_exception = { 0 };

	HRESULT t_result;
	t_result = S_OK;

	if (t_result == S_OK)
		t_result = m_parser -> ParseScriptText(*t_script, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISVISIBLE, NULL, &t_exception);

	IDispatch *t_lang_dispatch;
	t_lang_dispatch = NULL;
	if (t_result == S_OK)
		t_result = m_environment -> GetScriptDispatch(NULL, &t_lang_dispatch);

    MCAutoStringRef t_return_value;
	if (t_result == S_OK)
	{
		HRESULT t_var_result;

		DISPID t_var_id;
		LPOLESTR t_ole_var;
		t_ole_var = OLESTR("result");
		t_var_result = t_lang_dispatch -> GetIDsOfNames(IID_NULL, &t_ole_var, 1, LOCALE_NEUTRAL, &t_var_id);

		if (t_var_result == S_OK)
		{
			DISPPARAMS t_params;
			t_params.cArgs = 0;
			t_params.cNamedArgs = 0;
			t_params.rgdispidNamedArgs = NULL;
			t_params.rgvarg = NULL;

			VARIANT t_function_result;

			// OK-2009-02-12 : Variant must be initialized or certain values will fail to return correctly.
			VariantInit(&t_function_result);
			t_result = t_lang_dispatch -> Invoke(t_var_id, IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &t_params, &t_function_result, NULL, NULL);
			if (t_result == S_OK)
			{
				t_result = VariantChangeType(&t_function_result, &t_function_result, VARIANT_ALPHABOOL, VT_BSTR);
				if (t_result == S_OK)
				{
                    if (!MCStringCreateWithBSTR(t_function_result . bstrVal,
                                                &t_return_value))
                        t_result = E_OUTOFMEMORY;
				}
				VariantClear(&t_function_result);
			}
		}
		else
			t_return_value = kMCEmptyString;
	}


	if (t_result != S_OK)
	{
        t_return_value = kMCEmptyString;
	}

	if (t_lang_dispatch != NULL)
		t_lang_dispatch -> Release();

    r_out = t_return_value.Take();
	return;
}

bool MCWindowsActiveScriptEnvironment::Define(const char* p_function, MCScriptEnvironmentCallback p_callback)
{
	return m_dispatch -> Define(p_function, p_callback);
}

char *MCWindowsActiveScriptEnvironment::Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count)
{
	HRESULT t_result;
	t_result = S_OK;

	IDispatch *t_lang_dispatch;
	t_lang_dispatch = NULL;
	if (t_result == S_OK)
		t_result = m_environment -> GetScriptDispatch(NULL, &t_lang_dispatch);

	LPOLESTR t_ole_method;
	t_ole_method = NULL;
	if (t_result == S_OK)
	{
		t_ole_method = ConvertUTF8ToOLESTR(p_method);
		if (t_ole_method == NULL)
			t_result = E_OUTOFMEMORY;
	}

	DISPID t_method_id;
	if (t_result == S_OK)
		t_result = t_lang_dispatch -> GetIDsOfNames(IID_NULL, &t_ole_method, 1, LOCALE_NEUTRAL, &t_method_id);

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
			t_ole_arguments[i] . bstrVal = ConvertUTF8ToBSTR(p_arguments[p_argument_count - i - 1]);
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
		// OK-2009-02-12
		VariantInit(&t_function_result);
		t_result = t_lang_dispatch -> Invoke(t_method_id, IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &t_params, &t_function_result, NULL, NULL);
		if (t_result == S_OK)
		{
			t_result = VariantChangeType(&t_function_result, &t_function_result, VARIANT_ALPHABOOL, VT_BSTR);
			if (t_result == S_OK)
			{
				t_return_value = ConvertBSTRToUTF8(t_function_result . bstrVal);
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
		delete t_ole_method;

	if (t_lang_dispatch != NULL)
		t_lang_dispatch -> Release();

	return t_return_value;
}

MCScriptEnvironment *MCScreenDC::createscriptenvironment(MCStringRef p_language)
{
    MCAutoStringRefAsUTF8String t_language;
    /* UNCHECKED */ t_language . Lock(p_language);
	LPOLESTR t_ole_language;
	t_ole_language = ConvertUTF8ToOLESTR(*t_language);

	MCWindowsActiveScriptEnvironment *t_environment;
	t_environment = new (nothrow) MCWindowsActiveScriptEnvironment;

	if (t_environment -> Initialize(t_ole_language))
		t_environment -> Retain();
	else
	{
		delete t_environment;
		t_environment = NULL;
	}

	delete t_ole_language;

	return t_environment;
}
