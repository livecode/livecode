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

#include "system.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

// MM-2015-06-08: [[ MobileSockets ]] curtime global is required by opensslsocket.cpp
real8 curtime;

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

bool MCAndroidSystem::GetAddress(MCStringRef& r_address)
{
	extern MCStringRef MCcmd;
    MCAutoStringRef t_address;
    bool t_success;
    t_success = MCStringFormat(&t_address, "android:%@", MCcmd);
    if (t_success)
        r_address = MCValueRetain(*t_address);
    
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidSystem::SetEnv(MCStringRef p_name, MCStringRef p_value)
{
}

bool MCAndroidSystem::GetEnv(MCStringRef p_name, MCStringRef& r_value)
{
    r_value = MCValueRetain(kMCEmptyString);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

real64_t MCAndroidSystem::GetCurrentTime(void)
{
    // MM-2015-06-08: [[ MobileSockets ]] Store the current time globally, required by opensslsocket.cpp
	struct timeval tv;
	gettimeofday(&tv, NULL);
	curtime = tv . tv_sec + tv . tv_usec / 1000000.0;
    return curtime;
}

void MCAndroidSystem::Alarm(real64_t p_when)
{
}

void MCAndroidSystem::Sleep(real64_t p_when)
{
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

int MCAndroidSystem::GetErrno(void)
{
    return errno;
}

void MCAndroidSystem::SetErrno(int p_errno)
{
    errno = p_errno;
}

uint32_t MCAndroidSystem::GetSystemError(void)
{
    return errno;
}

////////////////////////////////////////////////////////////////////////////////
