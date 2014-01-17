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

#ifndef _dbSQLITE
#define _dbSQLITE


#define DB_SQLITE_STRING "SQLITE"

// as defined in sqlite/src/sqliteInt.h
#define MAX_BYTES_PER_ROW 1048576

#ifndef NDEBUG
#	define MDEBUG0(s) ;
#	define MDEBUG(s, x) ;
#	define MDEBUG2(mfile,s, x) fprintf(mfile, s, x);
#   define MDEBUG3(s, x) fprintf(myfile, s, x);
#else
#define MDEBUG0(s)
#define MDEBUG(s, x)
#define MDEBUG2(s, x, L)
#define MDEBUG3(s, x)
#endif

#include <stdio.h>
#include <sqlitedataset/sqlitedataset.h>
#include <sqlite3.h>
#include "dbdrivercommon.h"


class DBCursor_SQLITE : public CDBCursor
{
	public:
		DBCursor_SQLITE(SqliteDatabase &db);
		virtual ~DBCursor_SQLITE();
		
		Bool open(DBConnection *newconnection);
		void close();
		Bool first();
		Bool last();
		Bool next();
		Bool prev();

		Dataset *getDataset() { return mDataset; }
		Bool move(int p_record_index);

	protected:
		Bool getRowData();
		Bool getFieldsInformation();
		SqliteDatabase &mDB;
		Dataset *mDataset;
};

class DBConnection_SQLITE : public CDBConnection
{
	public:
		DBConnection_SQLITE();
		~DBConnection_SQLITE();

		Bool IsError();

		Bool connect(char **args, int numargs);

		void disconnect();

		void getTables(char *buffer, int *bufsize);

		Bool sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows);

		DBCursor *sqlQuery(char *query, DBString *args, int numargs, int p_rows);

		void transBegin();
		void transCommit();
		void transRollback();
		char *getErrorMessage(Bool p_last);

		int basicExec(const char *q, unsigned int *rows = 0);

		const char *getconnectionstring();

		int getConnectionType(void) { return -1; }
		int getVersion(void) { return 2; }

	protected:
		char *BindVariables(char *query, int oldsize, DBString *args, int numargs, int &newsize);
		void setErrorStr(const char *msg);

		SqliteDatabase mDB;
		char *mErrorStr;
		bool mIsError;
};
#endif
