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
#ifdef MACOS
#include <ctype.h>
#endif

const char DBNullValue[] = "";


char *longtostring(long inValue)
{
	static char numstring[32];
	sprintf(numstring, "%lu", inValue );
	return numstring;
}





void DBList::add(DBObject *newdbnode) {dblist.push_back(newdbnode);}

int DBList::getsize() {return dblist.size();}

void DBList::clear() 
{
	if (dblist.empty()) 
		return;
	DBObjectList::iterator theIterator;
	for (theIterator = dblist.begin(); theIterator != dblist.end(); theIterator++){
		DBObject *curobject = (DBObject *)(*theIterator);
		delete curobject;
	}
	dblist.clear();
}

Bool DBList::erase(const unsigned int fid)
{
	DBObjectList::iterator theIterator;
	for (theIterator = dblist.begin(); theIterator != dblist.end(); theIterator++){
		DBObject *curobject = (DBObject *)(*theIterator);
		if (curobject->GetID() == fid){
			delete curobject;
			dblist.erase(theIterator);
			return True;
		}
	}
	return False;
}


DBObject *DBList::findIndex(const int tindex)
{
	int i = 0;
	DBObjectList::iterator theIterator;
	for (theIterator = dblist.begin(); theIterator != dblist.end(); theIterator++){
		if (i++ == tindex){
			DBObject *curobject = (DBObject *)(*theIterator);
			return curobject;
		}
	}
	return NULL;
}

DBObject *DBList::find(const unsigned int fid)
{
	DBObjectList::iterator theIterator;
	for (theIterator = dblist.begin(); theIterator != dblist.end(); theIterator++){
		DBObject *curobject = (DBObject *)(*theIterator);
		if (curobject->GetID() == fid)
			return curobject;
	}		
	return NULL;
}


DBList::~DBList() {clear();}

DBObjectList *DBList::getList() {return &dblist;}

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

