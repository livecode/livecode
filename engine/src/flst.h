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

#ifndef __MC_FONT_LIST__
#define __MC_FONT_LIST__

#if defined(_WINDOWS_DESKTOP)
#include "w32flst.h"
#elif defined(_MAC_DESKTOP)
#include "osxflst.h"
#elif defined(_LINUX_DESKTOP)
#include "lnxflst.h"
#elif defined(_SERVER)
#include "srvflst.h"
#elif defined(_MOBILE)
#include "mblflst.h"
#endif

#endif
