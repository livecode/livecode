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

///////////////////////////////////////////////////////////////////////////////
// Unix support function definitions

#ifndef DARWIN
#include <sys/time.h>
#endif

#ifdef SELECT

#ifndef LINUX
#include <sys/select.h>
#include <sys/stream.h>
#endif

#else

#ifndef DARWIN
#include <poll.h>
#endif

#endif

#include <ctype.h>
#include <dlfcn.h>

#if defined DARWIN
#define VXCMD_STRING "VXCMD_macho"
#else
#define VXCMD_STRING "VXCMD"
#endif

#include "revdb.h"

void MCU_path2std(char *p_path);
void MCU_path2native(char *p_path);
void MCU_fix_path(char *cstr);
char *MCS_getcurdir(void);
char *MCS_resolvepath(const char *path);

#if !defined(TARGET_SUBPLATFORM_ANDROID) && !defined(TARGET_SUBPLATFORM_IPHONE)
#include <pwd.h>
#endif
#include  <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
