/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "platform-internal.h"
#include "mac-extern.h"
#include "globals.h"

////////////////////////////////////////////////////////////////////////////////


// SN-2014-07-23: [[ Bug 12907 ]]
//  Update as well MCSreenDC::createscriptenvironment (and callees)
void MCPlatformScriptEnvironmentCreate(MCStringRef language, MCPlatformScriptEnvironmentRef& r_env)
{
    r_env = MCplatform -> CreateScriptEnvironment();
}

void MCPlatformScriptEnvironmentRetain(MCPlatformScriptEnvironmentRef env)
{
    env -> Retain();
}

void MCPlatformScriptEnvironmentRelease(MCPlatformScriptEnvironmentRef env)
{
    env -> Release();
}

bool MCPlatformScriptEnvironmentDefine(MCPlatformScriptEnvironmentRef env, const char *function, MCPlatformScriptEnvironmentCallback callback)
{
    return env -> Define(function, callback);
}

void MCPlatformScriptEnvironmentRun(MCPlatformScriptEnvironmentRef env, MCStringRef script, MCStringRef& r_result)
{
    env -> Run(script, r_result);
}

void MCPlatformScriptEnvironmentCall(MCPlatformScriptEnvironmentRef env, const char *method, const char **arguments, uindex_t argument_count, char*& r_result)
{
    r_result = env -> Call(method, arguments, argument_count);
}

////////////////////////////////////////////////////////////////////////////////
