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

#ifndef __MC_FONT_LIST__
#define __MC_FONT_LIST__

// MM-2013-09-13: [[ RefactorGraphics ]] Updated to include platform specific font lists for server font support.
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
#include "w32flst.h"
#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER)
#include "osxflst.h"
#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)
#include "lnxflst.h"
#elif defined(_SERVER)
#include "srvflst.h"
#elif defined(_MOBILE)
#include "mblflst.h"
#elif defined(__EMSCRIPTEN__)
#include "em-fontlist.h"
#endif

#endif
