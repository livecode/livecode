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

#include "dbmysql.h"

#if defined(_WINDOWS)
#define LIBRARY_EXPORT __declspec(dllexport)
#elif defined(_MACOSX)
#define LIBRARY_EXPORT
#elif defined(_LINUX)
#define LIBRARY_EXPORT
#elif defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID)
#define LIBRARY_EXPORT
#endif

unsigned int *DBObject::idcounter = NULL;

extern "C" LIBRARY_EXPORT DBConnection *newdbconnectionref() 
{
	DBConnection *ref = new DBConnection_MYSQL();
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

////////////////////////////////////////////////////////////////////////////////

// The static function export table for iOS external linkage requirements.

#ifdef TARGET_SUBPLATFORM_IPHONE
extern "C" {
	
	struct LibExport
	{
		const char *name;
		void *address;
	};
	
	struct LibInfo
	{
		const char **name;
		struct LibExport *exports;
	};
	
	static const char *__libname = "dbmysql";
	
	static struct LibExport __libexports[] =
	{
		{ "newdbconnectionref", (void *)newdbconnectionref },
		{ "releasedbconnectionref", (void *)releasedbconnectionref },
		{ "setidcounterref", (void *)setidcounterref },
		{ 0, 0 }
	};
	
	struct LibInfo __libinfo =
	{
		&__libname,
		__libexports
	};
	
	__attribute((section("__DATA,__libs"))) __attribute__((visibility("default"))) volatile struct LibInfo *__libinfoptr_dbmysql = &__libinfo;
}
#endif

////////////////////////////////////////////////////////////////////////////////
