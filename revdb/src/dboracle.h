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

#ifndef _dboracle
#define _dboracle

#ifdef WIN32
#include <windows.h>
#endif

#include "dbdrivercommon.h"

extern "C"
{
#include <oratypes.h>
#include <ocidfn.h>
#include <ociapr.h>
#include <ocidem.h>
}

#define DB_ORACLE_STRING "oracle"


/* oparse flags */
#define  DEFER_PARSE        1
#define  NATIVE             1
#define  VERSION_7          2
#define PARSE_NO_DEFER           0
#define PARSE_V7_LNG             2


/*  internal/external datatype codes */
#define VARCHAR2_TYPE            1
#define NUMBER_TYPE              2
#define INT_TYPE				 3
#define FLOAT_TYPE               4
#define STRING_TYPE              5
#define LONG_TYPE				8
#define ROWID_TYPE              11
#define DATE_TYPE               12
#define RAW_TYPE				23
#define	LRAW_TYPE				24
#define VRAW_TYPE				94
#define LVRAW_TYPE				95
#define CHAR_TYPE				96


/*  ORACLE error codes used in demonstration programs */
#define VAR_NOT_IN_LIST       1007
#define NO_DATA_FOUND         1403
#define NULL_VALUE_RETURNED   1405

/*  some SQL and OCI function codes */
#define FT_INSERT                3
#define FT_SELECT                4
#define FT_UPDATE                5
#define FT_DELETE                9

class DBField_ORACLE:public DBField
{
public:
	DBField_ORACLE() 
	{
			extraData = NULL; 
			externaltype = 0; 
			indp = NULL; errorcode = NULL;
			dsize = NULL; databuffer = NULL;
	}
	virtual ~DBField_ORACLE() 
	{
		if (databuffer) delete databuffer;
		if (extraData) free(extraData);
		if (indp) delete indp;
		if (dsize) delete dsize;
		if (errorcode) delete errorcode;
		extraData = NULL; 
		indp = NULL; errorcode = NULL;
		dsize = NULL; databuffer = NULL;

	}
	char *extraData;
	ub2 *indp;
	ub2 *errorcode;
	ub2 *dsize;
	char *databuffer;
	int externaltype;
};


class DBCursor_ORACLE:public CDBCursor
{
public:
	virtual ~DBCursor_ORACLE() {close();}
	Bool open(DBConnection *newconnection, Lda_Def *oracursor, int p_rows);
	void close();
	Bool first();
	Bool last();
	Bool next();
	Bool prev();
	virtual char *getErrorMessage();
	Bool move(int p_record_index) { return False; }
protected:
	Bool getRowData();
	Bool getFieldsInformation();
	Cda_Def *ORACLE_res;
	static char errmsg[512];
	int FetchedNum;
	int rowsCached;
};

class DBConnection_ORACLE: public CDBConnection
{
public:
	~DBConnection_ORACLE() {disconnect();}
	Bool connect(char **args, int numargs);
	void disconnect();
	Bool sqlExecute(char *p_query, DBString *p_arguments, int p_argument_count, unsigned int &p_affected_rows);
	DBCursor *sqlQuery(char *p_query, DBString *p_arguments, int p_argument_count, int p_rows);
	const char *getconnectionstring();
	Lda_Def *getLDA() {return &lda;}
	void transBegin();
	void transCommit();
	void transRollback();
	char *getErrorMessage(Bool p_last);
	Bool IsError();
	void getTables(char *buffer, int *bufsize);
	int getVersion(void) { return 2; }
	int getConnectionType(void) { return -1; }
protected:
	Cda_Def *ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count);
	Bool BindVariables(Cda_Def *p_cursor, DBString *p_arguments, int p_argument_count, PlaceholderMap *t_placeholder_map);
	Lda_Def lda;
    ub1 hda[HDA_SIZE];
	static char errmsg[512];
};
#endif
