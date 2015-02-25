/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

/* ================================================================ */

template <typename T, T (*REF)(T), void (*UNREF)(T)>
class MCAutoScriptObjectRefBase
{
public:
	MCAutoScriptObjectRefBase (void)
		: m_value(nil)
	{}

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

private:
	T m_value;
	MCAutoScriptObjectRefBase<T,REF,UNREF> & operator = (MCAutoScriptObjectRefBase<T,REF,UNREF> & x);
};

typedef MCAutoScriptObjectRefBase<MCScriptModuleRef, MCScriptRetainModule, MCScriptReleaseModule> MCAutoScriptModuleRef;
typedef MCAutoScriptObjectRefBase<MCScriptInstanceRef, MCScriptRetainInstance, MCScriptReleaseInstance> MCAutoScriptInstanceRef;

/* ================================================================ */

#endif /* !__MC_SCRIPT_AUTO__ */
