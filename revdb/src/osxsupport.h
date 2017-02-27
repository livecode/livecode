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
// Mac OS X support function definitions

#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>
#include <sys/stat.h>
#include "revdb.h"

OSErr MCS_path2FSSpec(const char *fname, FSSpec *fspec);

#define VXCMD_STRING "VXCMD_macho"

void MCU_path2std(char *p_path);
void MCU_path2native(char *p_path);
void MCU_fix_path(char *cstr);

extern "C" 
{
// not used
#define ppcNoLocation 0
#include <pwd.h>
#include <time.h>

#ifdef __BIG_ENDIAN__
#define ntohl(x)       (x)
#define ntohs(x)       (x)
#define htonl(x)       (x)
#define htons(x)       (x)
#endif

#include <unistd.h>
#include <sys/fcntl.h>

extern int lstat(const char *path, struct stat *sb);
#define	S_ISLNK(m)	((m & 0170000) == 0120000)

#include <sys/stat.h>
#include <sys/utsname.h>
}


inline char *pStrcpy(unsigned char *dest, const unsigned char *src)
{
    memmove(dest, src, ((size_t)*src) + 1);
    return (char*)dest;
}

char *MCS_getcurdir();
void MCU_path2std(char *dptr);
void MCU_path2native(char *dptr);
void MCU_fix_path(char *cstr);
char *MCS_FSSpec2path(FSSpec *fSpec);


