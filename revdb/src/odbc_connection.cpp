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

#include "dbodbc.h"

#include <revolution/support.h>

#if defined(_LINUX) || defined(_MACOSX)
#define stricmp strcasecmp
#endif

#if defined(_WINDOWS)
#pragma optimize("", off)
#endif

#if not defined(min)
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

char DBConnection_ODBC::errmsg[512];
typedef unsigned short uint2;

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint2 * lpDestStr, int cchDest);
int UnicodeToUTF8(uint2 *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest);

/*DBCONNECTION_ODBC - CONNECTION OBJECT FOR ODBC DATABASES CHILD OF DBCONNECTION*/

#define UTF8_TO_UNICODE_VAR(t_name) \
	unsigned short *w_##t_name; \
	unsigned int w_##t_name##_length; \
	w_##t_name##_length = UTF8ToUnicode(t_name, strlen(t_name), NULL, 0); \
	w_##t_name = (unsigned short *)_alloca((w_##t_name##_length + 1) * 2); \
	UTF8ToUnicode(t_name, strlen(t_name), w_##t_name, w_##t_name##_length);

#define UTF8_TO_UNICODE_VAR_N(t_name) \
	unsigned short *w_##t_name; \
	unsigned int w_##t_name##_length; \
	w_##t_name##_length = UTF8ToUnicode(t_name, t_name##_length, NULL, 0); \
	w_##t_name = (unsigned short *)_alloca((w_##t_name##_length + 1) * 2); \
	UTF8ToUnicode(t_name, t_name##_length, w_##t_name, w_##t_name##_length);

#ifndef UTF8MODE
#define SQL_NOUNICODEMAP
#endif

Bool DBConnection_ODBC::connect(char **args, int numargs)
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

	char *t_host;
	t_host = args[0];

	char *t_dbname;
	t_dbname = args[1];

	// OK-2008-01-18 : Bug 5440. Used the fifth argument to allow users to specify which type
	// of cursors should be used for ODBC. This is to solve the speed issues reported in this bug.
	char *t_cursor_type;
	if (numargs >= 5)
	{
		t_cursor_type = args[4];

		// The default cursor type is emulated static, which means a static cursor is emulated if needed.
		if (stricmp(t_cursor_type, "forward only") == 0)
			m_cursor_type = kCursorTypeForward;
		else if (stricmp(t_cursor_type, "static") == 0)
			m_cursor_type = kCursorTypeStatic;
		else if (stricmp(t_cursor_type, "emulated static") == 0)
			m_cursor_type = kCursorTypeEmulated;
		else
			m_cursor_type = kCursorTypeForward;

	}
	else
		m_cursor_type = kCursorTypeForward;

	SQLRETURN t_result;
	
	t_result = SQLAllocEnv(&henv);
	if (t_result != SQL_SUCCESS)
		return False;
	
	t_result = SQLAllocConnect(henv, &hdbc);
	if (t_result != SQL_SUCCESS)
		return False;

	// MW-2005-07-27: Because we are in no-prompt mode, this should be the same
	//   length as the input connection string.
	unsigned char szConnStrOut[2048];
	SWORD cb;

	// OK-2008-01-18 : Bug 5440. Added ability to choose cursor type on ODBC.
	// Only allow the ODBC cursor library to be used if we are using emulated static cursors,
	// otherwise attempt to use the user-specified cursor type and throw an error if not supported.
	if (m_cursor_type == kCursorTypeStatic || m_cursor_type == kCursorTypeForward)
		t_result = SQLSetConnectAttr(hdbc, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_DRIVER, 0);
	else
		t_result = SQLSetConnectAttr(hdbc, SQL_ATTR_ODBC_CURSORS, (SQLPOINTER)SQL_CUR_USE_IF_NEEDED, 0);

	if (t_result != SQL_SUCCESS)
	{
		SetError(SQL_NULL_HSTMT);
		return False;
	}

#ifdef UTF8MODE
	UTF8_TO_UNICODE_VAR(host);
	rc = SQLDriverConnectW(hdbc, 0, w_host, SQL_NTS, (SQLWCHAR *)szConnStrOut, sizeof(szConnStrOut), &cb, SQL_DRIVER_NOPROMPT);	
#else
	t_result = SQLDriverConnectA(hdbc, 0, (SQLCHAR *)t_host, SQL_NTS, szConnStrOut, sizeof(szConnStrOut), &cb, SQL_DRIVER_NOPROMPT);	
#endif

	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
#ifdef UTF8MODE
		UTF8_TO_UNICODE_VAR(user);
		UTF8_TO_UNICODE_VAR(password);
		rc = SQLConnectW(hdbc, w_host, SQL_NTS, (*user ? w_user : NULL), SQL_NTS, (*password ? w_password : NULL),  SQL_NTS);
#else
		t_result = SQLConnectA(hdbc, (SQLCHAR *)t_host, SQL_NTS, (SQLCHAR *)(*t_user ? t_user : NULL), SQL_NTS, (SQLCHAR *)(*t_password ? t_password : NULL), SQL_NTS);
#endif
	}

	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
		SetError(SQL_NULL_HSTMT);
		return False;
	}
	
	connectionType = CT_ODBC,
	isConnected = True;
	
	return True;
}

/*Method to disconnect from ODBC server*/
void DBConnection_ODBC::disconnect()
{
	if (!isConnected)
		return;
		
	//close all open cursors from this connection
	closeCursors();
	
	//close odbc connection
	SQLDisconnect(hdbc);

	//free data associated with connection
	SQLFreeConnect(hdbc);
	SQLFreeEnv(henv);
	
	isConnected = False;
}

bool queryCallback(void *p_context, int p_placeholder, DBBuffer &p_output)
{
	p_output . append("?", 1);

	PlaceholderMap *t_placeholder_map;
	t_placeholder_map = (PlaceholderMap *)p_context;

	t_placeholder_map -> elements = (int *)realloc(t_placeholder_map -> elements, sizeof(int) * (t_placeholder_map -> length + 1));
	t_placeholder_map -> elements[t_placeholder_map -> length] = p_placeholder;
	t_placeholder_map -> length += 1;

	return true;
}

const char *DBConnection_ODBC::getconnectionstring()
{
	if (!isConnected)
		return DB_ODBC_STRING;

	char tmp[] = DB_ODBC_STRING;
	strcpy(connstring, tmp);

	char tbuffer[300] = "";
	SQLSMALLINT outlen;
	SQLGetInfoA(hdbc, SQL_DBMS_NAME, tbuffer, 255, &outlen);
	if (*tbuffer)
	{
		strcat(connstring,",");
		strcat(connstring,tbuffer);
	}
	return connstring;
}

/*Method to execute quick and fast sql statements like UPDATE and INSERT
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
affectedrows - will recieve number of rows updated or inserted
Output: False on error
*/
Bool DBConnection_ODBC::sqlExecute(char *p_query, DBString *p_arguments, int p_argument_count, unsigned int &p_affected_rows)
{
	bool t_success;
	t_success = true;

	Bool t_result;
	t_result = True;

	SQLHSTMT t_statement;
	SQLRETURN t_query_result;
	t_success = ExecuteQuery(p_query, p_arguments, p_argument_count, t_statement, t_query_result);

	DBCursor_ODBC *t_cursor;
	t_cursor = NULL;

	SQLLEN t_affected_rows;
	t_affected_rows = 0;

	if (t_success && (t_query_result == SQL_SUCCESS || t_query_result == SQL_SUCCESS_WITH_INFO))
		t_result = True;
	else
		t_result = False;

	if (t_success)
	{
		SQLSMALLINT t_column_count;
		SQLNumResultCols(t_statement, &t_column_count);
		if (t_column_count == 0)
		{
			SQLRowCount(t_statement, &t_affected_rows);
			if (t_affected_rows < 0)
				t_affected_rows = 0;

		}
	}

	// OK-2007-09-10 : Bug 5360
	if (!t_result) 
		SetError(t_statement);
	else
		SetError(NULL);

	SQLFreeStmt(t_statement, SQL_DROP);
	p_affected_rows = t_affected_rows;

	return t_result;
}

/*Method to execute sql statements like SELECT and return Cursor
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
Output: NULL cursor on error
*/
DBCursor *DBConnection_ODBC::sqlQuery(char *p_query, DBString *p_arguments, int p_argument_count, int p_rows)
{
	bool t_success;
	t_success = true;

	SQLHSTMT t_statement;
	SQLRETURN t_result;
	t_success = ExecuteQuery(p_query, p_arguments, p_argument_count, t_statement, t_result);

	DBCursor_ODBC *t_cursor;
	t_cursor = NULL;
	if (t_success && (t_result == SQL_SUCCESS || t_result == SQL_SUCCESS_WITH_INFO))
	{
		SQLSMALLINT t_column_count;
		SQLNumResultCols(t_statement, &t_column_count);
		if (t_column_count != 0)
		{
			t_cursor = new (nothrow) DBCursor_ODBC();
			if (!t_cursor -> open((DBConnection *)this, t_statement, p_rows))
			{
				delete t_cursor;
				t_cursor = NULL;

				SetError(t_statement);
				SQLFreeStmt(t_statement, SQL_DROP);
				t_statement = NULL;
			}
			else
			{
				addCursor(t_cursor);
				SetError(NULL);
			}
		}
	}
	else
		SetError(t_statement);

	return (DBCursor *)t_cursor;
}

char * getDiagnosticRecord(SQLHSTMT p_statement)
{
		SQLCHAR t_message_buffer[300];
		SQLRETURN t_result;
		SQLCHAR t_state[6];
		SQLINTEGER t_native_error;
		SQLSMALLINT t_text_length;

		t_result = SQLGetDiagRecA(SQL_HANDLE_STMT, p_statement, 1, t_state, &t_native_error, t_message_buffer, 300, &t_text_length);
		if (t_result != SQL_SUCCESS)
		{
			return NULL;
		}
		return NULL;
}


// @brief Executes an SQL query and returns the statement handle and ODBC return value.
// @param p_query : The query to execute.
// @param p_arguments : Array containing arguments to bind with the query.
// @param p_argument_count : The number of elements in p_arguments.
// @param p_statement : (Out) Reference to a statement handle, this will be set to the statement handle resulting from the execution.
// @param p_result : (Out) Reference to a result handle, this will be set the to the result handle resulting from the execution.
//
// @return True if successful, false otherwise.
bool DBConnection_ODBC::ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count, SQLHSTMT &p_statement, SQLRETURN &p_result)
{
	if (!isConnected)
		return false;
	
	unsigned int t_query_length;
	t_query_length = strlen(p_query);

	bool t_success;
	t_success = true;
	
	SQLHSTMT t_statement;
	SQLAllocStmt(hdbc,&t_statement);
	
	SQLRETURN t_result;
	if (t_success)
	{
		SQLUINTEGER t_cursor_type;

		// OK-2008-01-18 : Bug 5440. Set the required cursor type here.
		if (m_cursor_type == kCursorTypeStatic || m_cursor_type == kCursorTypeEmulated)
			t_cursor_type = SQL_CURSOR_STATIC;
		else
			t_cursor_type = SQL_CURSOR_FORWARD_ONLY;

		t_result = SQLSetStmtAttr(t_statement, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)t_cursor_type, 0);
		if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
		{
			SQLFreeStmt(t_statement, SQL_CLOSE);
			return false;
		}
	}

	char *t_parsed_query;
	t_parsed_query = p_query;

	// The placeholder map contains a mapping from bind to argument.
	PlaceholderMap t_placeholder_map;
	t_placeholder_map . length = 0;
	t_placeholder_map . elements = NULL;

	if (t_success)
	{
		if (p_argument_count != 0)
		{
			DBBuffer t_query_buffer(t_query_length + 1);
			t_success = processQuery(p_query, t_query_buffer, queryCallback, &t_placeholder_map);
				
			t_query_length = t_query_buffer . getSize();
			t_parsed_query = t_query_buffer . grab();
		}
	}

	if (t_success)
	{
#ifdef UTF8MODE
		UTF8_TO_UNICODE_VAR(t_parsed_query);
		err = SQLPrepareW(t_statement, (SQLWCHAR *)w_newquery , w_newquery_length);
#else
		t_result = SQLPrepareA(t_statement, (SQLCHAR *)t_parsed_query , t_query_length);
#endif
		if (t_result == SQL_SUCCESS || t_result == SQL_SUCCESS_WITH_INFO)
		{
			SQLLEN *t_argument_sizes = new (nothrow) SQLLEN[t_placeholder_map . length];
			if (BindVariables(t_statement, p_arguments, p_argument_count, t_argument_sizes, &t_placeholder_map))
			{
				t_result = SQLExecute(t_statement);
			}
			else
			{
				t_result = SQL_ERROR;
			}
			delete[] t_argument_sizes;
		}
		
		if (t_result == SQL_NEED_DATA && useDataAtExecution())
		{
			// This happens if one or more of the parameters bound in BindVariables required data-at-execution. What we need to do here
			// is pass the data to ODBC so it can populate these columns.
			t_success = handleDataAtExecutionParameters(t_statement);
			t_result = SQL_SUCCESS;
		}
		else if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
		{
			// OK-2008-01-16 : Bug 5725. Multi-line SQL statements would previously have appeared to fail
			t_success = false;
		}
	}

	if (p_argument_count != 0)
	{
		free(t_parsed_query);
		free(t_placeholder_map . elements);
	}

	p_statement = t_statement;
	p_result = t_result;
	return t_success;
}

// OK-2009-02-18: Added to allow data at execution to be turned off.
bool DBConnection_ODBC::useDataAtExecution(void)
{
	return false;
}


// OK-2009-02-18: Added to support data-at-execution. This function should be called if execution of p_statement resulted in
// SQL_NEED_DATA. It sends the required data for all parameters.
bool DBConnection_ODBC::handleDataAtExecutionParameters(SQLHSTMT p_statement)
{
	SQLRETURN t_result;
	t_result = SQL_NEED_DATA;

	DBString t_default("", 0, False);

	DBString *t_value;
	t_value = NULL;

	while (t_result == SQL_NEED_DATA)
	{
		t_result = SQLParamData(p_statement, (SQLPOINTER *)&t_value);
		if (t_result != SQL_NEED_DATA)
			continue;

		if (t_value -> length > 8000)
		{
			int t_test;
			t_test = 1;
		}

		char *t_current_position;
		const char *t_end;
		t_current_position = (char *)t_value -> sptr;
		t_end = t_value -> sptr + t_value -> length;

		// TODO: adjust this for putting data-at-execution into action.
		unsigned int t_block_size;
		t_block_size = 4096;

		SQLRETURN t_put_result;
		do 
		{
			int t_length;
			t_length = min(t_block_size, t_end - t_current_position);
			t_put_result = SQLPutData(p_statement, (SQLPOINTER)t_current_position, t_length);
			t_current_position = t_current_position + t_block_size;

		} while (t_current_position < t_end);

	
		
		if (t_put_result != SQL_SUCCESS)
		{
			char *t_error;
			t_error = getDiagnosticRecord(p_statement);

			return false;
		}
	}

	return (t_result == SQL_SUCCESS);
}


/*IsError-True on error*/
Bool DBConnection_ODBC::IsError()
{
		return errmsg[0] != '\0';
}

void DBConnection_ODBC::SetError(SQLHSTMT p_statement)
{
		SQLINTEGER t_error;
		SWORD t_errormsgsize;
		SQLCHAR t_state[6];

		SQLRETURN err = SQLErrorA(henv, hdbc, p_statement, t_state, &t_error, (SQLCHAR *)errmsg, SQL_MAX_MESSAGE_LENGTH-1, &t_errormsgsize);
		if (err != SQL_SUCCESS)
			errmsg[0] = '\0';
}

/*getErrorMessage- return error string*/
char *DBConnection_ODBC::getErrorMessage(Bool p_last)
{
	if (IsError() == True) return errmsg;
	else return (char *)DBNullValue;
}


/*BindVariables-parses querystring and binds variables*/
Bool DBConnection_ODBC::BindVariables(SQLHSTMT p_cursor, DBString *p_arguments, int p_argument_count, SQLLEN *p_argument_sizes, PlaceholderMap *p_placeholder_map)
{
	if (p_argument_count == 0)
		return True;
	
	DBString t_default("", 0, False);

	int t_length = p_placeholder_map->length;
	for(int i = 0; i < t_length; ++i)
	{
		DBString *t_value;
		if (p_placeholder_map -> elements[i] > p_argument_count)
			t_value = &t_default;
		else
			t_value = &p_arguments[p_placeholder_map -> elements[i] - 1];

		// OK-2009-02-18: [[Bug 7722]] - If non-binary data is being passed that is over 8000 bytes, we use the SQL_LONGVARCHAR type
		// as otherwise we get a data-truncation error.
		SQLSMALLINT t_data_type;
		if (!t_value -> isbinary)
			t_data_type = (t_value -> length > 8000 ? SQL_LONGVARCHAR : SQL_CHAR);
		else
			t_data_type = SQL_LONGVARBINARY;

		// OK-2009-02-18: The ability to use data-at-execution was added to fix bug 7722
		bool t_use_data_at_execution;
		t_use_data_at_execution = useDataAtExecution();
		
		SQLRETURN t_result;

		if (!t_use_data_at_execution) 
		{
			p_argument_sizes[i] = t_value -> length;

			// MW-2008-07-30: [[ Bug 6415 ]] Inserting data into SQL Server doesn't work.
			//   This is because the bind parameter call was only being issued for binary columns!
            
            SQLLEN * t_argument_size = &p_argument_sizes[i];
            t_result = SQLBindParameter(p_cursor, i + 1, SQL_PARAM_INPUT,
				t_value -> isbinary ? SQL_C_BINARY : SQL_C_CHAR, t_data_type,
				t_value -> length, 0, (void *)t_value -> sptr, t_value -> length, t_argument_size);

			if (!(t_result == SQL_SUCCESS || t_result == SQL_SUCCESS_WITH_INFO))
				return False;
		}
		else
		{
			// To use data_at_execution, it seems that the only thing required is to put the result of this macro
			// into the strlen_or_indptr parameter (the last one). As p_argument_sizes is not used
			// by anything else, and is deleted after this method returns, it should be safe to change it.
			p_argument_sizes[i] = SQL_LEN_DATA_AT_EXEC(t_value -> length);

			// The parameters are binded slightly differently here. Notice that we pass the actual DBString pointer instead
			// of the value buffer. This pointer will be returned by ODBC for us later, meaning that we don't need to look up
			// which parameter is needed again.
			SQLLEN t_argument_size;
			t_result = SQLBindParameter(p_cursor, i + 1, SQL_PARAM_INPUT,
				t_value -> isbinary ? SQL_C_BINARY : SQL_C_CHAR, t_data_type,
				t_value -> length, 0, (void *)t_value, t_value -> length, &t_argument_size);
			
			if (!(t_result == SQL_SUCCESS || t_result == SQL_SUCCESS_WITH_INFO))
				return False;
			
			p_argument_sizes[i] = t_argument_size;
		}
	}

	return True;
}

/*ODBC doesn't support transactions so leave blank for now*/
void DBConnection_ODBC::transBegin() {}
void DBConnection_ODBC::transCommit()
{
	SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT); 
}
void DBConnection_ODBC::transRollback()
{
	SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_ROLLBACK);
}

void DBConnection_ODBC::getTables(char *buffer, int *bufsize)
{
	int rowseplen = 1;
	char rowsep[] = "\n";

	if (!buffer)
		*bufsize = 0;

	SQLHSTMT querycursor;
	SQLAllocStmt(hdbc,&querycursor);
	DBCursor_ODBC *newcursor = NULL;
	SQLRETURN err;
    err = SQLTablesA(querycursor, NULL ,0, NULL ,0, (SQLCHAR *)"%" , SQL_NTS, (SQLCHAR *)"TABLE" ,SQL_NTS);
	if (err == SQL_SUCCESS || err == SQL_SUCCESS_WITH_INFO)
	{
		SQLSMALLINT ncolumns;
		SQLNumResultCols(querycursor,&ncolumns);
		char *result = buffer;
		char *resultptr = result;
		if (ncolumns)
		{
			newcursor = new (nothrow) DBCursor_ODBC();
			if (newcursor->open((DBConnection *)this, querycursor, 1))
			{
				if (!newcursor->getEOF())
				{
					while (True)
					{
						unsigned int colsize;
						char *coldata = newcursor -> getFieldDataBinary(3, colsize);
						if (newcursor -> getFieldType(3) == FT_WSTRING)
						{
							coldata = string_from_utf16((unsigned short *)coldata, colsize);
							colsize = colsize / 2;
						}
						else
							coldata = strdup(coldata);

						if (buffer != NULL && ((resultptr - result) + (int)colsize + rowseplen ) > *bufsize)
						{
							free(coldata);
							break;
						}

						if (buffer != NULL)
							memcpy(resultptr, coldata, colsize);
						resultptr += colsize;
						newcursor -> next();
						if (newcursor -> getEOF())
						{
							free(coldata);
							break;
						}

						free(coldata);
						if (buffer != NULL)
							memcpy(resultptr, rowsep, rowseplen);
						resultptr += rowseplen;
					}
				}
				
			}
			if (buffer != NULL)
				*resultptr++ = '\0';
			else
				resultptr += 1;
			*bufsize = resultptr-result;
			delete newcursor;
			newcursor = NULL;
			querycursor = NULL;
		}
	}
}



#define ASCII             0x007f

#define SHIFT_IN          '+'     // beginning of a shift sequence
#define SHIFT_OUT         '-'     // end       of a shift sequence

#define UTF8_2_MAX        0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define UTF8_1ST_OF_2     0xc0    // 110x xxxx
#define UTF8_1ST_OF_3     0xe0    // 1110 xxxx
#define UTF8_TRAIL        0x80    // 10xx xxxx

#define HIGER_6_BIT(u)    ((u) >> 12)
#define MIDDLE_6_BIT(u)   (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)    ((u) & 0x003f)

#define BIT7(a)           ((a) & 0x80)
#define BIT6(a)           ((a) & 0x40)

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint2 * lpDestStr, int cchDest)
{
	int nTB = 0;                   // # trail bytes to follow
	int cchWC = 0;                 // # of Unicode code points generated
	char * pUTF8 = (char *)lpSrcStr;
	char UTF8;

	while ((cchSrc--) && ((cchDest == 0) || ((cchWC << 1) < cchDest)))
	{
		//  See if there are any trail bytes.
		if (BIT7(*pUTF8) == 0)
		{
			//  Found ASCII.
			if (cchDest)
			{
				lpDestStr[cchWC] = (uint2)*pUTF8;
			}
			cchWC++;
		}
		else
			if (BIT6(*pUTF8) == 0)
			{
				//  Found a trail byte.
				//  Note : Ignore the trail byte if there was no lead byte.
				if (nTB != 0)
				{
					//  Decrement the trail byte counter.
					nTB--;
					//  Make room for the trail byte and add the trail byte value.
					if (cchDest)
					{
						lpDestStr[cchWC] <<= 6;
						lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
					}
					if (nTB == 0)
					{
						//  End of sequence.  Advance the output counter.
						cchWC++;
					}
				}
			}
			else
			{
				//  Found a lead byte.
				if (nTB > 0)
				{
					//  Error - previous sequence not finished.
					nTB = 0;
					cchWC++;
				}
				else
				{
					//  Calculate the number of bytes to follow.
					//  Look for the first 0 from left to right.
					UTF8 = *pUTF8;
					while (BIT7(UTF8) != 0)
					{
						UTF8 <<= 1;
						nTB++;
					}
					//  Store the value from the first byte and decrement
					//  the number of bytes to follow.
					if (cchDest)
					{
						lpDestStr[cchWC] = UTF8 >> nTB;
					}
					nTB--;
				}
			}
		pUTF8++;
	}
	//  Make sure the destination buffer was large enough.
	if (cchDest && (cchSrc >= 0))
		return 0;
	//  Return the number of Unicode characters written.
	if (lpDestStr)
		lpDestStr[cchWC] = 0;
	return cchWC << 1;
}

int UnicodeToUTF8(uint2 *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest)
{
	uint2 * lpWC = lpSrcStr;
	int cchU8 = 0;                // # of UTF8 chars generated

	while ((cchSrc--) && (cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
	{
		if (*lpWC <= ASCII)
		{
			//  Found ASCII.
			if (cchDest)
			{
				lpDestStr[cchU8] = (char)*lpWC;
			}
			cchU8++;
		}
		else
			if (*lpWC <= UTF8_2_MAX)
			{
				//  Found 2 byte sequence if < 0x07ff (11 bits).
				if (cchDest)
				{
					if ((cchU8 + 1) < cchDest)
					{
						//  Use upper 5 bits in first byte.
						//  Use lower 6 bits in second byte.
						lpDestStr[cchU8++] = UTF8_1ST_OF_2 | (*lpWC >> 6);
						lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
					}
					else
					{
						//  Error - buffer too small.
						cchSrc++;
						break;
					}
				}
				else
					cchU8 += 2;
			}
			else
			{
				//  Found 3 byte sequence.
				if (cchDest)
				{
					if ((cchU8 + 2) < cchDest)
					{
						//  Use upper  4 bits in first byte.
						//  Use middle 6 bits in second byte.
						//  Use lower  6 bits in third byte.
						lpDestStr[cchU8++] = UTF8_1ST_OF_3 | (*lpWC >> 12);
						lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC);
						lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
					}
					else
					{
						//  Error - buffer too small.
						cchSrc++;
						break;
					}
				}
				else
					cchU8 += 3;
			}
		lpWC++;
	}
	//  Make sure the destination buffer was large enough.
	if (cchDest && (cchSrc >= 0))
		return 0;
	if (lpDestStr)
		lpDestStr[cchU8] = 0;
	//  Return the number of UTF-8 characters written.
	return cchU8;
}
