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

#ifndef __MC_SCRIPT_ENVIRONMENT__
#define __MC_SCRIPT_ENVIRONMENT__

typedef char *(*MCScriptEnvironmentCallback)(const char* const* p_arguments, unsigned int p_argument_count);

class MCScriptEnvironment
{
public:
	virtual ~MCScriptEnvironment() {}

	virtual void Retain(void) = 0;
	virtual void Release(void) = 0;

	virtual bool Define(const char *p_function, MCScriptEnvironmentCallback p_callback) = 0;

	virtual void Run(MCStringRef p_script, MCStringRef& r_output) = 0;

	virtual char *Call(const char *p_method, const char** p_arguments, unsigned int p_argument_count) = 0;
};

#endif
