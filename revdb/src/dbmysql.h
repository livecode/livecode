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

#include "dbdrivercommon.h"
#include "large_buffer.h"

#include <mysql.h>
#include <mysql_com.h>
#include <mysql_version.h>
#include <errmsg.h>

#define DB_MYSQL_STRING "MYSQL";

#ifndef _dbMYSQL
#define _dbMYSQL

class DBCursor_MYSQL:public CDBCursor
{
public:
	virtual ~DBCursor_MYSQL() {close();}
	Bool open(DBConnection *newconnection);
	void close();
	Bool first();
	Bool last();
	Bool next();
	Bool prev();
	Bool move(int p_record_index);
protected:
	Bool getRowData();
	Bool getFieldsInformation();
	MYSQL_RES *mysql_res;
	int libraryRecordNum;
};

class DBConnection_MYSQL: public CDBConnection
{
public:
    DBConnection_MYSQL(): m_internal_buffer(nullptr) {}
	~DBConnection_MYSQL() {disconnect();}
	Bool connect(char **args, int numargs);
	void disconnect();
	Bool sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows);
	DBCursor *sqlQuery(char *query, DBString *args, int numargs, int p_rows);
	MYSQL *getMySQL() {return &mysql;}
	const char *getconnectionstring();
	void transBegin();
	void transCommit();
	void transRollback();
	char *getErrorMessage(Bool p_last);
	Bool IsError();
	void getTables(char *buffer, int *bufsize);
	int getConnectionType(void) { return -1; }
	int getVersion(void) { return 2; }
protected:
	bool BindVariables(MYSQL_STMT *p_statement, DBString *p_arguments, int p_argument_count, int *p_placeholders, int p_placeholder_count, MYSQL_BIND **p_bind);
	bool ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count);
	MYSQL mysql;
private:
    large_buffer_t *m_internal_buffer;
};
#endif
