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

#define REVDB_EXPORTS 1
#ifdef REVDB_EXPORTS
#ifdef X11
#define REVDB_API
#endif
#ifdef WIN32
#define REVDB_API __declspec(dllexport)
#endif
#ifdef MACOS 
#ifdef MACHO
#define REVDB_API
#else
#define REVDB_API __declspec(dllexport)
#endif
#endif
#endif

#include "database.h"


#ifdef __cplusplus
extern "C" {
#endif
REVDB_API DBConnection *newdbconnectionref();
REVDB_API void releasedbconnectionref(DBConnection *dbref);
REVDB_API void setidcounterref(unsigned int *tidcounter);
#ifdef __cplusplus
}
#endif
