/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#ifndef __MINI_ATL_H
#define __MINI_ATL_H
#define __ATLBASE_H__

#include <malloc.h>

////////////////////////////////////////////////////////////////////////////////
// String Conversion

inline char *_ConvW2A(char *p_ansi, const WCHAR *p_wide, int p_ansi_chars)
{
	int t_conv;
	t_conv = WideCharToMultiByte(CP_ACP, 0, p_wide, -1, p_ansi, p_ansi_chars, NULL, NULL);
	if (t_conv == 0)
		return NULL;
	return p_ansi;
}

inline WCHAR *_ConvA2W(WCHAR *p_wide, const char *p_ansi, int p_wide_chars)
{
	int t_conv;
	t_conv = MultiByteToWideChar(CP_ACP, 0, p_ansi, -1, p_wide, p_wide_chars);
	if (t_conv == 0)
		return NULL;
	return p_wide;
}

#define USES_CONVERSION int _conv_chars; char *_conv_ansi_str; WCHAR *_conv_wide_str; (_conv_chars,_conv_ansi_str,_conv_wide_str);

#define W2A(w) (w == NULL ? NULL : (_conv_chars = WideCharToMultiByte(CP_ACP, 0, w, -1, NULL, 0, NULL, NULL), _conv_ansi_str = (char*)alloca(_conv_chars), _ConvW2A(_conv_ansi_str, w, _conv_chars)))
#define A2W(a) (a == NULL ? NULL : (_conv_chars = MultiByteToWideChar(CP_ACP, 0, a, -1, NULL, 0), _conv_wide_str = (WCHAR*)alloca(_conv_chars * sizeof(WCHAR)), _ConvA2W(_conv_wide_str, a, _conv_chars)))

#if defined(_UNICODE)
#define W2T(a) (a)
#define T2W(a) (a)
#else
#define W2T W2A
#define T2W A2W
#endif

////////////////////////////////////////////////////////////////////////////////
// CComPtr

template<class T>
class CComPtr
{
public:
	T** operator & (void)
	{
		return &m_ptr;
	}

	bool operator ! (void)
	{
		return m_ptr == NULL;
	}

	T* operator -> (void)
	{
		return m_ptr;
	}

	operator T* (void)
	{
		return m_ptr;
	}

	T* operator = (const CComPtr<T> &p_comptr)
	{
		if (m_ptr != p_comptr.m_ptr)
		{
			Release();
			m_ptr = p_comptr.m_ptr;

			if (m_ptr != NULL)
				m_ptr->AddRef();
		}

		return m_ptr;
	}

	CComPtr()
	{
		m_ptr = NULL;
	}

	//CComPtr(T*);
	//CComPtr(const CComPtr<T>&);

	//template<class S>
	//CComPtr(const CComPtr<S>&);

	CComPtr(int p_null)
	{
		// initialise to NULL
		m_ptr = NULL;
	}

	~CComPtr()
	{
		Release();
	}

	HRESULT CoCreateInstance(GUID p_id, IUnknown *p_outer = NULL, DWORD p_cls_context = CLSCTX_ALL)
	{
		HRESULT t_result;
		t_result = ::CoCreateInstance(p_id, p_outer, p_cls_context, __uuidof(T), (void**)&m_ptr);

		return t_result;
	}

	void Release(void)
	{
		if (m_ptr != NULL)
			m_ptr->Release();
		m_ptr = NULL;
	}

	T* Detach(void)
	{
		T* t_ptr = m_ptr;
		m_ptr = NULL;
		return t_ptr;
	}

protected:
	T *m_ptr;
};

template<class T, const GUID *I>
class CComQIPtr : public CComPtr<T>
{
public:
	T* operator = (IUnknown *p_unknown)
	{
		Query(p_unknown);
		return m_ptr;
	}

	CComQIPtr() : CComPtr()
	{
	}

	//CComQIPtr(T*);
	CComQIPtr(IUnknown *p_unknown)
	{
		Query(p_unknown);
	}

protected:
	void Query(IUnknown *p_unknown)
	{
		IUnknown *t_queried = NULL;
		if (p_unknown != NULL)
			p_unknown->QueryInterface(*I, (void**)&t_queried);

		Release();
		m_ptr = (T*)t_queried;
	}
};

////////////////////////////////////////////////////////////////////////////////

#endif
