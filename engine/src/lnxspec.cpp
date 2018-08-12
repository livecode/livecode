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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Linux Source File:
//    unixspec.cpp
//
//  Description:
//    This file contains the implementations of the platform abstraction methods
//    defined in osspec.h and elsewhere for Linux.
//
//  Changes:
//    2009-06-25 MW Added implementation of MCS_fakeopencustom and related
//                  changes to stream handling methods.
//    2009-06-30 MW Refactored fake custom implementation to mcio.cpp and added
//                  new hooks (tell, seek_cur).
//
////////////////////////////////////////////////////////////////////////////////

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "object.h"
#include "stack.h"
#include "card.h"
#include "mcerror.h"

#include "param.h"
#include "handler.h"
#include "util.h"
#include "globals.h"
#include "ports.cpp"
#include "socket.h"
#include "mcssl.h"
#include "securemode.h"
#include "mode.h"
#include "player.h"
#include "osspec.h"

#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/dir.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <termios.h>
#include <langinfo.h>
#include <locale.h>
#include <pwd.h>

#include <libgnome/gnome-url.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-init.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

