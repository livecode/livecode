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

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::Initialize(void)
{
	return true;
}

void MCAndroidSystem::Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidSystem::Debug(const char *p_cstring)
{
	__android_log_print(ANDROID_LOG_INFO, "LiveCode", "%s", p_cstring);
}

////////////////////////////////////////////////////////////////////////////////

void *MCAndroidSystem::LoadModule(MCStringRef p_path)
{
	void *t_result;
	t_result = dlopen(MCStringGetCString(p_path), RTLD_LAZY);
	MCLog("LoadModule(%s) - %p\n", MCStringGetCString(p_path), t_result);
	return t_result;
}

void *MCAndroidSystem::ResolveModuleSymbol(void *p_module, MCStringRef p_symbol)
{
	return dlsym(p_module, MCStringGetCString(p_symbol));
}

void MCAndroidSystem::UnloadModule(void *p_module)
{
	dlclose(p_module);
}

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCMobileCreateSystem(void)
{
	return new MCAndroidSystem;
}

////////////////////////////////////////////////////////////////////////////////
