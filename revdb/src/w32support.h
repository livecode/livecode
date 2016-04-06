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
// Windows support function definitions

#include <windows.h>
#include "revdb.h"

#define PATH_MAX MAX_PATH

#if defined(DARWIN)
#define VXCMD_STRING "VXCMD_macho"
#else
#define VXCMD_STRING "VXCMD"
#endif

#define istrdup strdup

struct DATABASEREC
{
	char dbname[255];
	idcounterrefptr idcounterptr;
	new_connectionrefptr  newconnectionptr;
	release_connectionrefptr releaseconnectionptr;
    set_callbacksrefptr setcallbacksptr;
	HINSTANCE driverref;
};

char *GetModuleFolder(HINSTANCE p_module);
const char *GetExternalFolder(void);
const char *GetApplicationFolder(void);
DATABASEREC *DoLoadDatabaseDriver(const char *p_path);
void FreeDatabaseDriver( DATABASEREC *tdatabaserec);
void MCU_path2std(char *p_path);
void MCU_path2native(char *p_path);
void MCU_fix_path(char *cstr);
char *MCS_getcurdir(void);
char *MCS_resolvepath(const char *p_path);
BOOL WINAPI DllMain(HINSTANCE tInstance, DWORD dwReason, LPVOID /*lpReserved*/);

extern HINSTANCE hInstance;


