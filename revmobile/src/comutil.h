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

#ifndef __COMUTIL__
#define __COMUTIL__

#ifndef __UTIL__
#include "util.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class ComString
{
public:
	ComString(void)
	{
		m_string = nil;
	}

	~ComString(void)
	{
		if (m_string != nil)
			SysFreeString(m_string);
	}

	operator BSTR (void)
	{
		return m_string;
	}

	BSTR * operator & (void)
	{
		return &m_string;
	}

private:
	BSTR m_string;
};

template<typename T> class ComPointer
{
public:
	ComPointer(void)
	{
		m_object = nil;
	}

	ComPointer(ComPointer<T>& other)
	{
		m_object = other . m_object;
		if (m_object != nil)
			m_object -> AddRef();
	}

	~ComPointer(void)
	{
		if (m_object != nil)
			m_object -> Release();
	}

	operator T*(void)
	{
		return m_object;
	}

	T& operator *(void)
	{
		return *p;
	}

	T** operator & (void)
	{
		return &m_object;
	}

	T *operator -> (void)
	{
		return m_object;
	}

	bool Attached(void) const
	{
		return m_object != nil;
	}

	T *Take(void)
	{
		T *t_object;
		t_object = m_object;
		m_object = nil;
		return t_object;
	}

	bool CreateInstance(const IID& clsid)
	{
		HRESULT t_result;
		t_result = CoCreateInstance(clsid, nil, CLSCTX_ALL, __uuidof(T), (void **)&m_object);
		if (t_result == S_OK)
			return true;
		return false;
	}

	template<typename Class> bool CreateInstance(void)
	{
		return CreateInstance(__uuidof(Class));
	}

	bool QueryInterface(IUnknown *unk)
	{
		HRESULT t_result;
		t_result = unk -> QueryInterface(__uuidof(T), (void **)&m_object);
		if (t_result == S_OK)
			return true;
		return false;
	}

private:
	T *m_object;
};

inline bool ComCheck(HRESULT hr)
{
	if (hr == S_OK)
		return true;
	return Throw("error");
}

////////////////////////////////////////////////////////////////////////////////

#endif
