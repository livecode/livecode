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

void MCU_path2std(char *p_path);
void MCU_path2native(char *p_path);
void MCU_fix_path(char *cstr);
char *MCS_getcurdir(void);
char *MCS_resolvepath(const char *p_path);

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;
    
    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);
    
    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;
    
    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);
    
    return count;
}

#endif
