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

#ifndef __GLOBDEFS_H
#define __GLOBDEFS_H

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_PLATFORM_WINDOWS)
#define _DESKTOP
#define _WINDOWS_DESKTOP
#ifndef _WINDOWS
#define _WINDOWS
#endif
#elif defined(TARGET_PLATFORM_MACOS_X)
#define _DESKTOP
#define _MAC_DESKTOP
#ifndef _MACOSX
#define _MACOSX
#endif
#elif defined(TARGET_PLATFORM_LINUX)
#define _DESKTOP
#define _LINUX_DESKTOP
#ifndef _LINUX
#define _LINUX
#endif
#elif defined(TARGET_PLATFORM_MOBILE)

#ifndef _MOBILE
#define _MOBILE
#endif

#if defined(TARGET_SUBPLATFORM_IPHONE)
#define _IOS_MOBILE
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#define _ANDROID_MOBILE
#endif

#endif

////////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"

////////////////////////////////////////////////////////////////////////////////

#ifndef __MCUTILITY_H
#include "mcutility.h"
#endif

////////////////////////////////////////////////////////////////////////////////

#endif
