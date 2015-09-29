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

#include "dboracle.h"

#define LONGVARCHAR 94
#define LONGVARRAW	95 

char DBConnection_ORACLE::errmsg[512];

/*DBCONNECTION_ORACLE - CONNECTION OBJECT FOR ORACLE DATABASES CHILD OF DBCONNECTION*/

Bool DBConnection_ORACLE::connect(char **args, int numargs)
{
	if (isConnected)
		return True;
	
	// MW-2014-01-30: [[ Sqlite382 ]] Relaxed revdb_connect() so it only checks for at least
	//   one argument - we need at least 4 though.
	if (numargs < 4)
		return False;

	char *t_user;
	t_user = args[2];

	char *t_password;
	t_password = args[3];

	char *t_database;
	t_database = args[1];

	char *t_host;
	t_host = args[0];

	if (t_user[0] == '\0')
		t_user = NULL;

	if (t_password[0] == '\0')
		t_password = NULL;

	memset(hda,0,HDA_SIZE);

	sword t_error;
	t_error = olog(&lda, hda, (ub1 *)t_user, -1, (ub1 *)t_password, -1, (ub1 *)t_host, -1, OCI_LM_DEF);
	if (t_error != 0)
	{
		// OK-2010-02-24: Oracle connection should return an error message if it failed.
		// The 600 char buffer size was taken from example code on Oracle's site.
		char *t_error_message;
		t_error_message = (char *)malloc(600);
		oerhms(&lda, lda .rc, (OraText *)t_error_message, 600);
		m_error = t_error_message;

		return False;
	}

	connectionType = CT_ORACLE,
	isConnected = True;
	return True;
}

/*Method to disconnect from ORACLE server*/
void DBConnection_ORACLE::disconnect()
{
	if (!isConnected)
		return;

	closeCursors();
	ologof(&lda);
	isConnected = False;
}


const char *DBConnection_ORACLE::getconnectionstring()
{
	return DB_ORACLE_STRING;
}

bool queryCallback(void *p_context, int p_placeholder, DBBuffer &p_output)
{
	p_output . append(":", 1);
	p_output . ensure(1);

	p_output . ensure(12);
	sprintf(p_output . getFrontier(), "%d", p_placeholder);
	p_output . advance(strlen(p_output . getFrontier()));

	PlaceholderMap *t_placeholder_map;
	t_placeholder_map = (PlaceholderMap *)p_context;

	t_placeholder_map -> elements[t_placeholder_map -> length] = p_placeholder;
	t_placeholder_map -> length ++;

	return true;
}

Cda_Def *DBConnection_ORACLE::ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count)
{
	int t_affected_rows;
	t_affected_rows = 0;

	unsigned int t_query_length;
	t_query_length = strlen(p_query);

	Cda_Def *t_cursor;
	t_cursor = new Cda_Def;

	bool t_success;
	t_success = true;

	sword t_error;
	if (t_success)
	{
		t_error = oopen(t_cursor, getLDA(), (text *)0, -1, -1, NULL, -1);
		if (t_error != 0)
			t_success = false;
	}

	char *t_parsed_query;
	t_parsed_query = p_query;

	PlaceholderMap t_placeholder_map;

	if (p_argument_count != 0)
	{
		t_placeholder_map . length = 0;
		t_placeholder_map . elements = new int[p_argument_count + 1];

		DBBuffer t_query_buffer(t_query_length + 1);
		t_success = processQuery(p_query, t_query_buffer, queryCallback, &t_placeholder_map);

		t_query_length = t_query_buffer . getSize();
		t_parsed_query = t_query_buffer . grab();
	}

	if (t_success)
	{
		if (oparse(t_cursor, (text *)t_parsed_query, t_query_length, DEFER_PARSE, PARSE_V7_LNG) != 0)
			t_success = false;
	}

	if (t_success)
	{
		if (!BindVariables(t_cursor, p_arguments, p_argument_count, &t_placeholder_map))
			t_success = false;
	}

	if (t_success)
	{
		t_error = oexec(t_cursor);
		if (t_error != 0)
			t_success = false;
	}

	if (!t_success)
	{
		delete t_cursor;
		t_cursor = NULL;
	}

	return t_cursor;
}

/*Method to execute quick and fast sql statements like UPDATE and INSERT
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
affectedrows - will recieve number of rows updated or inserted
Output: False on error
*/
Bool DBConnection_ORACLE::sqlExecute(char *p_query, DBString *p_arguments, int p_argument_count, unsigned int &p_affected_rows)
{
	if (!isConnected)
		return False;

	Cda_Def *t_cursor;
	t_cursor = ExecuteQuery(p_query, p_arguments, p_argument_count);
	if (t_cursor == NULL)
	{
		// OK-2007-09-10 : Bug 5360
		char t_error_message[512];
		oerhms((cda_def *)getLDA(), lda.rc, (text *)t_error_message, (sword) sizeof(t_error_message));
		errorMessageSet(t_error_message);
		return False;
	}

	int t_affected_rows;
	t_affected_rows = (int)t_cursor -> rpc;
	p_affected_rows = t_affected_rows;

	oclose(t_cursor);
	errorMessageSet(NULL);
	return True;
}

/*Method to execute sql statements like SELECT and return Cursor
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
Output: NULL cursor on error
*/
DBCursor *DBConnection_ORACLE::sqlQuery(char *p_query, DBString *p_arguments, int p_argument_count, int p_rows)
{
	if (!isConnected)
		return NULL;

	Cda_Def *t_cursor;
	t_cursor = ExecuteQuery(p_query, p_arguments, p_argument_count);
	if (t_cursor == NULL)
		return NULL;

	DBCursor_ORACLE *t_rev_cursor;
	t_rev_cursor = new DBCursor_ORACLE();
	if (!t_rev_cursor -> open((DBConnection *)this, t_cursor, p_rows))
	{
		// OK-2007-09-10 : Bug 5360
		delete t_rev_cursor;
		t_rev_cursor = NULL;
		char t_error_message[512];
		oerhms((cda_def *)getLDA(), lda.rc, (text *)t_error_message, (sword) sizeof(t_error_message));
		errorMessageSet(t_error_message);
	}
	else
	{
		// OK-2007-09-10 : Bug 5360
		errorMessageSet(NULL);
		addCursor(t_rev_cursor);
	}

	lda . rc = t_cursor -> rc;

	return (DBCursor *)t_rev_cursor;
}

/*IsError-True on error*/
Bool DBConnection_ORACLE::IsError()
{
	if (lda.rc != 0)
		return True;

	return False;	
}

/*getErrorMessage- return error string*/
char *DBConnection_ORACLE::getErrorMessage(Bool p_last)
{
    // AL-2013-11-08 [[ Bug 11149 ]] Make sure most recent error string is available to revDatabaseConnectResult
    if (p_last || IsError())
    {
        // OK-2007-09-10 : Bug 5360
        if (m_error != NULL)
            return m_error;
    }
		
    return (char *)DBNullValue;
}

/*BindVariables-parses querystring and binds variables*/
Bool DBConnection_ORACLE::BindVariables(Cda_Def *p_cursor, DBString *p_arguments, int p_argument_count, PlaceholderMap *p_placeholder_map)
{
	if (p_argument_count == 0)
		return True;

	int t_param_count;
	t_param_count = p_placeholder_map -> length;

	if (t_param_count <= 0)
		return True;

	for (int i = 0; i < t_param_count; i++)
	{
		int t_placeholder_number;
		t_placeholder_number = i + 1;

		int t_parameter_number;
		t_parameter_number = p_placeholder_map -> elements[i] - 1;

		DBString t_parameter_value;
		t_parameter_value = p_arguments[t_parameter_number];
	
		sword t_datatype;
		if (t_parameter_value . isbinary)
			t_datatype = LRAW_TYPE;
		else
			t_datatype = VARCHAR2_TYPE;

		sword t_datalength;
		t_datalength = t_parameter_value . length;

		if (obndrn(p_cursor, t_placeholder_number, (ub1 *)t_parameter_value . sptr, t_datalength, t_datatype, -1, 0, 0, -1, -1) != 0)
			return False;
	}
	return True;
}

void DBConnection_ORACLE::getTables(char *p_buffer, int *p_buffer_size)
{
	int t_rowdelimiter_length;
	t_rowdelimiter_length = 1;

	char t_rowdelimiter[1];
	*t_rowdelimiter = '\n';

	char t_query[] = "select tname from tab";

	DBCursor *t_cursor;
	t_cursor = sqlQuery(t_query, NULL, 0, 0);
	if (t_cursor == NULL)
		return;

	char *t_result;
	t_result = p_buffer;

	char *t_result_ptr;
	t_result_ptr = t_result;

	if (p_buffer == NULL)
	{	
		unsigned int t_total_size;
		t_total_size = 0;
		while (1)
		{
			unsigned int t_column_size;
			t_column_size = 0;

			char *t_column_data;
			t_column_data = t_cursor -> getFieldDataBinary(1, t_column_size);
			t_column_size = strlen(t_column_data);
			
			t_total_size += t_column_size + t_rowdelimiter_length;

			t_cursor -> next();
			if (t_cursor -> getEOF())
				break;
		}

		t_cursor -> getConnection() -> deleteCursor(t_cursor -> GetID());
		*p_buffer_size = t_total_size;
		return;
	}
	else
	{
		while (1)
		{
			unsigned int t_column_size;
			t_column_size = 0;

			char *t_column_data;
			t_column_data = t_cursor -> getFieldDataBinary(1, t_column_size);
			t_column_size = strlen(t_column_data);
			if (((t_result_ptr - t_result) + (int)t_column_size + t_rowdelimiter_length + 1) >= *p_buffer_size)
				break;


			memcpy(t_result_ptr, t_column_data, t_column_size);
			t_result_ptr += t_column_size;
			t_cursor -> next();

			if (t_cursor -> getEOF())
				break;

			memcpy(t_result_ptr, t_rowdelimiter, t_rowdelimiter_length);
			t_result_ptr += t_rowdelimiter_length;
		}
		*t_result_ptr = '\0';
	}

	t_cursor -> getConnection() -> deleteCursor(t_cursor -> GetID());
	t_cursor = NULL;
}

/*ORACLE doesn't support transactions so leave blank for now*/
void DBConnection_ORACLE::transBegin() {}
void DBConnection_ORACLE::transCommit(){ocom(getLDA());}
void DBConnection_ORACLE::transRollback(){orol(getLDA());}
