/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "system.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

uint32_t MCAndroidSystem::GetProcessId(void)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::GetVersion(MCStringRef& r_string)
{
	MCAndroidEngineCall("getSystemVersion", "x", &r_string);
	return true;
}

bool MCAndroidSystem::GetMachine(MCStringRef& r_string)
{
	MCAndroidEngineCall("getMachine", "x", &r_string);
	return true;
}

MCNameRef MCAndroidSystem::GetProcessor(void)
{
#ifdef __i386__
    return MCN_i386;
#else
    return MCN_arm;
#endif
}

char *MCAndroidSystem::GetAddress(void)
{
	extern MCStringRef MCcmd;
	char *t_address;
	t_address = new char[MCStringGetLength(MCcmd) + strlen("android:") + 1];
	sprintf(t_address, "android:%s", MCStringGetCString(MCcmd));
	return t_address;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidSystem::SetEnv(const char *name, const char *value)
{
}

char *MCAndroidSystem::GetEnv(const char *name)
{
	return "";
}

////////////////////////////////////////////////////////////////////////////////

real64_t MCAndroidSystem::GetCurrentTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv . tv_sec + tv . tv_usec / 1000000.0;
}

void MCAndroidSystem::Alarm(real64_t p_when)
{
}

void MCAndroidSystem::Sleep(real64_t p_when)
{
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
