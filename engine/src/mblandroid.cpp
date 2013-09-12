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

#include <android/log.h>
#include <dlfcn.h>

#include "prefix.h"

#include "system.h"
#include "mblandroid.h"
#include "filedefs.h"

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::Initialize(void)
{
    IO_stdin = MCsystem -> OpenFd(0, kMCSOpenFileModeRead);
    IO_stdout = MCsystem -> OpenFd(1, kMCSOpenFileModeWrite);
    IO_stderr = MCsystem -> OpenFd(2, kMCSOpenFileModeWrite);
    
	return true;
}

void MCAndroidSystem::Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

MCServiceInterface *MCAndroidSystem::QueryService(MCServiceType type)
{
    return nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidSystem::Debug(MCStringRef p_string)
{
	__android_log_print(ANDROID_LOG_INFO, "LiveCode", "%s", MCStringGetCString(p_string));
}

////////////////////////////////////////////////////////////////////////////////

MCSysModuleHandle MCAndroidSystem::LoadModule(MCStringRef p_path)
{
	void *t_result;
	t_result = dlopen(MCStringGetCString(p_path), RTLD_LAZY);
	MCLog("LoadModule(%s) - %p\n", MCStringGetCString(p_path), t_result);
	return (MCSysModuleHandle)t_result;
}

MCSysModuleHandle MCAndroidSystem::ResolveModuleSymbol(MCSysModuleHandle p_module, MCStringRef p_symbol)
{
	return (MCSysModuleHandle)dlsym((void*)p_module, MCStringGetCString(p_symbol));
}

void MCAndroidSystem::UnloadModule(MCSysModuleHandle p_module)
{
	dlclose((void*)p_module);
}

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCMobileCreateAndroidSystem(void)
{
	return new MCAndroidSystem;
}

////////////////////////////////////////////////////////////////////////////////
