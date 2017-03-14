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

#include "dbdriver.h"

typedef DBConnection *(*new_connectionrefptr) ();
typedef void (*release_connectionrefptr) (DBConnection *dbref);
typedef void (*idcounterrefptr) (unsigned int *tidcounter);
typedef void (*set_callbacksrefptr)(const DBcallbacks *callbacks);

struct DATABASEREC
{
    char dbname[255];
    idcounterrefptr idcounterptr;
    new_connectionrefptr  newconnectionptr;
    release_connectionrefptr releaseconnectionptr;
    set_callbacksrefptr setcallbacksptr;
    void *driverref;
};

typedef vector<DATABASEREC *> DATABASERECList;
