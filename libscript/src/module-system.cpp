/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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

#include <foundation.h>
#include <foundation-system.h>

/* ================================================================
 * System identification
 * ================================================================ */

extern "C" MC_DLLEXPORT_DEF void
MCSystemExecGetOperatingSystem (MCStringRef & r_string)
{
	const char t_os[] =
#if defined(__IOS__)
		"ios"
#elif defined(__MAC__)
		"mac"
#elif defined(__WINDOWS__)
		"windows"
#elif defined(__ANDROID__)
		"android"
#elif defined(__LINUX__)
		"linux"
#elif defined(__EMSCRIPTEN__)
		"emscripten"
#else
#  error "Unrecognized operating system"
#endif
		;

	/* UNCHECKED */ MCStringCreateWithCString (t_os, r_string);
}

extern "C" MC_DLLEXPORT_DEF void
MCSystemExecGetArchitecture(MCStringRef & r_string)
{
    const char t_arch[] =
#if defined(__X86__) || defined(__i386__)
    "x86"
#elif defined(__X86_64__)
    "x86_64"
#elif defined(__ARM64__)
    "arm64"
#elif defined(__ARM__)
    "arm"
#elif defined(__EMSCRIPTEN__)
    "js"
#else
#  error "Unrecognized architecture"
#endif
    ;
    
    /* UNCHECKED */ MCStringCreateWithCString (t_arch, r_string);
}

/* ================================================================
 * Command-line information
 * ================================================================ */

extern "C" MC_DLLEXPORT_DEF void
MCSystemExecGetCommandName (MCStringRef & r_string)
{
	/* UNCHECKED */ MCSCommandLineGetName (r_string);
}

extern "C" MC_DLLEXPORT_DEF void
MCSystemExecGetCommandArguments (MCProperListRef & r_list)
{
	/* UNCHECKED */ MCSCommandLineGetArguments (r_list);
}

////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_system_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_system_Finalize (void)
{
}

////////////////////////////////////////////////////////////////
