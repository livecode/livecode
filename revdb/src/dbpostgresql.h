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

#ifdef WIN32
#include <windows.h>
#endif

#include "database.h"

#ifndef _dbPOSTGRESQL
#define _dbPOSTGRESQL


#include <libpq-fe.h>
#include "dbdrivercommon.h"
#include "large_buffer.h"

#define DB_POSTGRESQL_STRING "POSTGRESQL";

#define PG_TYPE_BOOL         16
#define PG_TYPE_BYTEA        17
#define PG_TYPE_CHAR         18
#define PG_TYPE_NAME         19
#define PG_TYPE_CHAR16       20
#define PG_TYPE_INT2         21
#define PG_TYPE_INT28        22
#define PG_TYPE_INT4         23
#define PG_TYPE_REGPROC      24
#define PG_TYPE_TEXT         25
#define PG_TYPE_OID          26
#define PG_TYPE_TID          27
#define PG_TYPE_XID          28
#define PG_TYPE_CID          29
#define PG_TYPE_OID8         30
#define PG_TYPE_SET          32
#define PG_TYPE_CHAR2       409
#define PG_TYPE_CHAR4       410
#define PG_TYPE_CHAR8       411
#define PG_TYPE_POINT       600
#define PG_TYPE_LSEG        601
#define PG_TYPE_PATH        602
#define PG_TYPE_BOX         603
#define PG_TYPE_POLYGON     604
#define PG_TYPE_FILENAME    605
#define PG_TYPE_FLOAT4      700
#define PG_TYPE_FLOAT8      701
#define PG_TYPE_ABSTIME     702
#define PG_TYPE_RELTIME     703
#define PG_TYPE_TINTERVAL   704
#define PG_TYPE_UNKNOWN     705
#define PG_TYPE_MONEY       790
#define PG_TYPE_OIDINT2     810
#define PG_TYPE_OIDINT4     910
#define PG_TYPE_OIDNAME     911
// Array Types. Not easy to find in the .h files. They are defined in pg_type.h.
#define PG_TYPE_ARR_BOOL    1000
#define PG_TYPE_ARR_BYTEA   1001
#define PG_TYPE_ARR_CHAR    1002
#define PG_TYPE_ARR_NAME    1003
#define PG_TYPE_ARR_INT2    1005
#define PG_TYPE_ARR_INT4    1007
#define PG_TYPE_ARR_REGPROC 1008
#define PG_TYPE_ARR_TEXT    1009
#define PG_TYPE_ARR_OID     1028
#define PG_TYPE_ARR_TID     1010
#define PG_TYPE_ARR_XID     1011
#define PG_TYPE_ARR_CID     1012
#define PG_TYPE_ARR_VARCHAR 1015
#define PG_TYPE_ARR_INT8    1016
#define PG_TYPE_ARR_POINT   1017
#define PG_TYPE_ARR_LSEG    1018
#define PG_TYPE_ARR_PATH    1019
#define PG_TYPE_ARR_BOX     1020
#define PG_TYPE_ARR_FLOAT4  1021
#define PG_TYPE_ARR_FLOAT8  1022
#define PG_TYPE_ARR_ABSTIME 1023
#define PG_TYPE_ARR_RELTIME 1024
#define PG_TYPE_ARR_TINTERVAL 1025
#define PG_TYPE_ARR_POLYGON 1027
#define PG_TYPE_ARR_NUMERIC 1231

#define PG_TYPE_BPCHAR     1042
#define PG_TYPE_VARCHAR    1043
#define PG_TYPE_DATE       1082
#define PG_TYPE_TIME       1083
#define PG_TYPE_DATETIME   1184
#define PG_TYPE_TIMESPAN   1186        //This is not defined and was added.
#define PG_TYPE_TIMESTAMP  1296
#define PG_TYPE_ZPBIT      1560
#define PG_TYPE_VARBIT     1562
#define PG_TYPE_NUMERIC    1700

class DBCursor_POSTGRESQL:public CDBCursor
{
public:
	virtual ~DBCursor_POSTGRESQL() {close();}
	Bool open(DBConnection *newconnection,PGresult *querycursor);
	void close();
	Bool first();
	Bool last();
	Bool next();
	Bool prev();
	Bool move(int p_record_index);
protected:
	Bool getRowData();
	Bool getFieldsInformation();
	PGresult *POSTGRESQL_res;
};

class DBConnection_POSTGRESQL: public CDBConnection
{
public:
    DBConnection_POSTGRESQL(): m_internal_buffer(nullptr) {}
	~DBConnection_POSTGRESQL() {disconnect();}
	Bool connect(char **args, int numargs);
	void disconnect();
	Bool sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows);
	DBCursor *sqlQuery(char *query, DBString *args, int numargs, int p_rows);
	void getTables(char *buffer, int *bufsize);
	const char *getconnectionstring();
	void transBegin();
	void transCommit();
	void transRollback();
	char *getErrorMessage(Bool p_last);
	Bool IsError();
	int getConnectionType(void) { return -1; }
	int getVersion(void) { return 2; }
protected:
	PGconn *dbconn;
	PGresult *ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count);
private:
    large_buffer_t *m_internal_buffer;
};
#endif
