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

#ifndef _dbodbc
#define _dbodbc

#if defined(_MACOSX)
#include <sql.h>
#include <sqlext.h>
#elif defined(_LINUX)
#include <sql.h>
#include <sqlext.h>
#else
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x401
#endif
#define W32_EXTRA_LEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#endif

#include "dbdrivercommon.h"

#define DB_ODBC_STRING "odbc";

class DBField_ODBC:public DBField
{
public:
	DBField_ODBC() 
	{
			extraData = NULL; 
	}
	virtual ~DBField_ODBC() 
	{
		if (extraData) free(extraData);
	}
	char *extraData;
};


class DBCursor_ODBC:public CDBCursor
{
public:
	virtual ~DBCursor_ODBC() {close();}
	Bool open(DBConnection *newconnection, SQLHSTMT odbccursor, int p_rows);
	void close();
	Bool first();
	Bool last();
	Bool next();
	Bool prev();
	virtual char *getFieldDataBinary(unsigned int fieldnum,unsigned int &fdlength);
	virtual char *getErrorMessage();
	cursor_type_t m_type;
	Bool move(int p_record_index);
protected:
	Bool getRowData();
	Bool getFieldsInformation();
	SQLHSTMT ODBC_res;
	static char errmsg[512];
	int FetchedNum;
	int rowsCached;

};

class DBConnection_ODBC: public CDBConnection
{
public:
	~DBConnection_ODBC() {disconnect();}
	Bool connect( char **args, int numargs);
	void disconnect();
	Bool sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows);
	DBCursor *sqlQuery(char *query, DBString *args, int numargs, int p_rows);
	void getTables(char *buffer, int *bufsize);
	HDBC getHDBC() {return hdbc;}
	HENV getHENV() {return henv;}
	void transBegin();
	void transCommit();
	const char *getconnectionstring();
	void transRollback();
	char *getErrorMessage(Bool p_last);
	Bool IsError();
	cursor_type_t getCursorType(void) { return m_cursor_type; }
	int getVersion(void) { return 2; }
	int getConnectionType(void) { return -1; }
protected:
	void SetError(SQLHSTMT tcursor);
	Bool BindVariables(SQLHSTMT tcursor, DBString *args, int numargs, SQLLEN *paramsizes, PlaceholderMap *p_placeholder_map);
	bool ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count, SQLHSTMT &p_statement, SQLRETURN &p_result);
	bool handleDataAtExecutionParameters(SQLHSTMT p_statement);
	bool useDataAtExecution(void);
	HENV henv;
    HDBC hdbc;
	char connstring[300];
	static char errmsg[512];
	cursor_type_t m_cursor_type;
};

#endif
