/*                                                                     -*-c++-*-
Copyright (C) 2018 LiveCode Ltd.

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

#include "system-private.h"

MC_DLLEXPORT_DEF bool
MCSInfoGetArchitecture(MCStringRef & r_string)
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
    
    return MCStringCreateWithCString (t_arch, r_string);
}