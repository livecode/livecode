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

struct DATABASEREC
{
	char dbname[255];
	idcounterrefptr idcounterptr;
	new_connectionrefptr  newconnectionptr;
	release_connectionrefptr releaseconnectionptr;
    set_callbacksrefptr setcallbacksptr;
	void *driverref;
};


// Implimented by TS.
const char *GetExternalFolder(void);
const char *GetApplicationFolder(void);
DATABASEREC *DoLoadDatabaseDriver(const char *p_path);


void FreeDatabaseDriver( DATABASEREC *tdatabaserec);
DATABASEREC *DoLoadDatabaseDriver(const char *p_path);
const char *GetExternalFolder(void);
const char *GetApplicationFolder(void);
void MCU_path2std(char *p_path);
void MCU_path2native(char *p_path);
void MCU_fix_path(char *cstr);
char *MCS_getcurdir(void);
char *MCS_resolvepath(const char *path);

#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>