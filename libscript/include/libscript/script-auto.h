/*                                                                     -*-c++-*-
Copyright (C) 2015-2016 LiveCode Ltd.

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

#ifndef __MC_SCRIPT_AUTO__
#define __MC_SCRIPT_AUTO__

#include "script.h"
#include "foundation-span.h"

/* ================================================================ */

template <typename T, T (*REF)(T), void (*UNREF)(T)>
class MCAutoScriptObjectRefBase
{
public:
	MCAutoScriptObjectRefBase (void)
		: m_value(nil)
	{}

	explicit MCAutoScriptObjectRefBase (T p_value)
		: m_value(nil)
	{
		if (nil != p_value)
			m_value = REF (p_value);
	}

	~MCAutoScriptObjectRefBase (void)
	{
		UNREF (m_value);
	}

	MCAutoScriptObjectRefBase operator = (T value)
	{
		MCAssert (nil == m_value);
		m_value = REF (value);
		return value;
	}

	T & operator & (void)
	{
		MCAssert (nil == m_value);
		return m_value;
	}

	T operator * (void) const
	{
		return m_value;
	}

	T Release(void)
	{
		T t_value = m_value;
		m_value = nil;
		return t_value;
	}

private:
	T m_value;
	MCAutoScriptObjectRefBase<T,REF,UNREF> & operator = (MCAutoScriptObjectRefBase<T,REF,UNREF> & x);
};

typedef MCAutoScriptObjectRefBase<MCScriptModuleRef, MCScriptRetainModule, MCScriptReleaseModule> MCAutoScriptModuleRef;
typedef MCAutoScriptObjectRefBase<MCScriptInstanceRef, MCScriptRetainInstance, MCScriptReleaseInstance> MCAutoScriptInstanceRef;

/* ================================================================ */

template <typename T, T (*REF)(T), void (*UNREF)(T)>
class MCAutoScriptObjectRefArrayBase
{
public:
	MCAutoScriptObjectRefArrayBase (void)
		: m_values(nil), m_count(0)
	{}

	~MCAutoScriptObjectRefArrayBase (void)
	{
		if (nil == m_values) return;
		for (size_t i = 0; i < m_count; ++i)
		{
			UNREF(m_values[i]);
		}
		MCMemoryDeleteArray (m_values);
		m_values = 0;
		m_count = 0;
	}

	bool New (uindex_t p_size)
	{
		MCAssert (nil == m_values);
		return MCMemoryNewArray (p_size, m_values, m_count);
	}

	T* Ptr ()
	{
		return m_values;
	}

	uindex_t Size() const
	{
		return m_count;
	}

	bool Resize(uindex_t p_new_count)
	{
		return MCMemoryResizeArray (p_new_count, m_values, m_count);
	}

	bool Extend(uindex_t p_new_count)
	{
		MCAssert(p_new_count >= m_count);
		return Resize(p_new_count);
	}

	bool Push(T p_value)
	{
		if (!(nil == m_values ? New(1) : Extend(m_count + 1)))
			return false;
		m_values[m_count - 1] = REF(p_value);
		return true;
	}

	MCSpan<T> Span()
	{
		return MCMakeSpan(Ptr(), Size());
	}

	T & operator [] (const int p_index)
	{
		MCAssert (nil != m_values);
		MCAssert (p_index >= 0);
		return m_values[p_index];
	}

    MCSpan<T> Release(void)
    {
        MCSpan<T> t_value = Span();
        m_values = nullptr;
        m_count = 0;
        return t_value;
    }

private:
	T *m_values;
	uindex_t m_count;
};

typedef MCAutoScriptObjectRefArrayBase<MCScriptModuleRef, MCScriptRetainModule, MCScriptReleaseModule> MCAutoScriptModuleRefArray;
typedef MCAutoScriptObjectRefArrayBase<MCScriptInstanceRef, MCScriptRetainInstance, MCScriptReleaseInstance> MCAutoScriptInstanceRefArray;

/* ================================================================
 * libscript functions relying on managed-lifetime types
 * ================================================================
 *
 * This header declares some libscript functions in addition to those
 * declared in "script.h".  These functions rely on the managed
 * lifetime types declared in this header but not available in
 * "script.h".
 */

/* ----------------------------------------------------------------
 * Module-related functions
 * ---------------------------------------------------------------- */
MC_DLLEXPORT bool MCScriptCreateModulesFromStream(MCStreamRef p_stream,
                                                  MCAutoScriptModuleRefArray& x_modules);
MC_DLLEXPORT bool MCScriptCreateModulesFromData(MCDataRef p_data,
                                                MCAutoScriptModuleRefArray& x_modules);

#endif /* !__MC_SCRIPT_AUTO__ */
