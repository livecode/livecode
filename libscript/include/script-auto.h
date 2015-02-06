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

class MCAutoScriptModuleRef
{
public:
	MCAutoScriptModuleRef (void)
	{
		m_value = nil;
	}

	~MCAutoScriptModuleRef (void)
	{
		MCScriptReleaseModule (m_value);
	}

	MCScriptModuleRef operator = (MCScriptModuleRef value)
	{
		MCAssert (nil == m_value);
		m_value = MCScriptRetainModule (value);
		return value;
	}

	MCScriptModuleRef& operator & (void)
	{
		MCAssert (nil == m_value);
		return m_value;
	}

	MCScriptModuleRef operator * (void) const
	{
		return m_value;
	}

private:
	MCScriptModuleRef m_value;

	MCAutoScriptModuleRef & operator = (MCAutoScriptModuleRef & x);
};

class MCAutoScriptInstanceRef
{
public:
	MCAutoScriptInstanceRef (void)
	{
		m_value = nil;
	}

	~MCAutoScriptInstanceRef (void)
	{
		MCScriptReleaseInstance (m_value);
	}

	MCScriptInstanceRef operator = (MCScriptInstanceRef value)
	{
		MCAssert (nil == m_value);
		m_value = MCScriptRetainInstance (value);
		return value;
	}

	MCScriptInstanceRef& operator & (void)
	{
		MCAssert (nil == m_value);
		return m_value;
	}

	MCScriptInstanceRef operator * (void) const
	{
		return m_value;
	}

private:
	MCScriptInstanceRef m_value;

	MCAutoScriptInstanceRef & operator = (MCAutoScriptInstanceRef & x);
};

/* ================================================================ */

#endif /* !__MC_SCRIPT_AUTO__ */
