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

#include "dbpostgresql.h"

#if defined(_WINDOWS)
#define LIBRARY_EXPORT __declspec(dllexport)
#else
#define LIBRARY_EXPORT __attribute__((__visibility__("default")))
#endif

unsigned int *DBObject::idcounter = NULL;

extern "C" LIBRARY_EXPORT DBConnection *newdbconnectionref() 
{
	DBConnection *ref = new (nothrow) DBConnection_POSTGRESQL();
	return ref;
}

extern "C" LIBRARY_EXPORT void releasedbconnectionref(DBConnection *dbref) 
{
	if(dbref) 
		delete dbref;
}

extern "C" LIBRARY_EXPORT void setidcounterref(unsigned int *tidcounter)
{
	DBObject::idcounter = tidcounter;
}
