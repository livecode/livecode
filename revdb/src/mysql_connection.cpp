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

#include "dbmysql.h"

extern bool load_ssl_library();

#if defined(_WINDOWS)
#define strcasecmp stricmp
#endif

Bool DBConnection_MYSQL::connect(char **args, int numargs)
{
	if (isConnected)
		return True;

	if (numargs < 4)
    	return False;

	char *t_dbname;
	t_dbname = args[1];

	char *t_user;
    t_user = args[2];

	char *t_password;
    t_password = args[3];

	char *t_use_ssl_string;
	t_use_ssl_string = NULL;

	if (numargs >= 5)
		t_use_ssl_string = args[4];

	// Parse args[0] to obtain the host and port.
	char *t_delimiter;
	t_delimiter = strchr(args[0], ':');

	char *t_host;
	t_host = args[0];

	char *t_port_string;
	t_port_string = NULL;

	if (t_delimiter != NULL)
	{
		t_port_string = (t_delimiter + (1 * sizeof(char)));
		*t_delimiter = '\0';
	}

	int t_port;
	t_port = 0;

	if (t_port_string != NULL)
		t_port = atoi(t_port_string);

	int t_use_ssl;
	t_use_ssl = 0;

	if (t_use_ssl_string != NULL && (strlen(t_use_ssl_string) != 0))
		if (strcasecmp(t_use_ssl_string, "true") == 0)
			t_use_ssl = 1;
		else
			t_use_ssl = atoi(t_use_ssl_string);
	else
		t_use_ssl = 0;
		
	if (t_use_ssl && !load_ssl_library())
	{
		errorMessageSet("Unable to load SSL library");
		return false;
	}
	
	//initialize mysql data structure for connection
	if (!mysql_init(getMySQL()))
		return False;

	// MM-2011-09-09: [[ BZ 9712]] Allow the user to specify the socket or named pipe to connect with
	char *t_socket;
	if (numargs >= 6 && strlen(args[5]) != 0)
		t_socket = args[5];
	else
		t_socket = NULL;	
	
	// Set timeout value for TCP/IP connections to 20 seconds.
	// According to API docs, total effective timeout is 3 times this due to retries,
	// but testing on windows shows connections timing out after 20 seconds
	// MM-2011-09-09: Updated to allow the user to specify the read/write timeout
	int t_read_write_timeout;
	if(numargs < 7 || args[6] == NULL || strlen(args[6]) == 0 || sscanf(args[6], "%d", &t_read_write_timeout) == 0 || t_read_write_timeout < 0)
		t_read_write_timeout = 20;
	if (mysql_options(getMySQL(), MYSQL_OPT_READ_TIMEOUT, &t_read_write_timeout) ||
		mysql_options(getMySQL(), MYSQL_OPT_WRITE_TIMEOUT, &t_read_write_timeout))
		return false;
	
	// MM-2011-09-09: Allow the user to specify the auto-reconnect option.  If connection to the server has been lost,
	//	the driver will automatically try to reconnect.  To prevent the reconnect hanging, set the reconnect timeout also.
	bool t_auto_reconnect;
	t_auto_reconnect = (numargs >= 8 && args[7] != NULL && strlen(args[7]) != 0 && strcasecmp(args[7], "true") == 0);
	if (t_auto_reconnect)
		if (mysql_options(getMySQL(), MYSQL_OPT_RECONNECT, &t_auto_reconnect) ||
			mysql_options(getMySQL(), MYSQL_OPT_CONNECT_TIMEOUT, &t_read_write_timeout))
			return false;	
	
	//connect to mysql database.
	if (!mysql_real_connect(getMySQL(), t_host, t_user, t_password, t_dbname, t_port, t_socket, t_use_ssl > 0 ? CLIENT_SSL : 0))
		return False;

	connectionType = CT_MYSQL,
	isConnected = True;
	
	return True;
}

const char *DBConnection_MYSQL::getconnectionstring()
{
	return DB_MYSQL_STRING;
}

/*Method to disconnect from MySQL server*/
void DBConnection_MYSQL::disconnect()
{
	//if not connected then exit early
	if (!isConnected)
		return;
	//close all open cursors from this connection
	closeCursors();
	//close mysql connection

	mysql_close(getMySQL());
	isConnected = False;
}

/*Method to execute quick and fast sql statements like UPDATE and INSERT
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
affectedrows - will recieve number of rows updated or inserted
Output: False on error
*/
Bool DBConnection_MYSQL::sqlExecute(char *p_query, DBString *p_arguments, int p_argument_count, unsigned int &p_affected_rows)
{
	if (!ExecuteQuery(p_query, p_arguments, p_argument_count) || mysql_errno(getMySQL()))
	{
		// OK-2007-09-10 : Bug 5360
		errorMessageSet(mysql_error(getMySQL()));
		return False;
	}

	bool t_success = true;
	bool t_first = true;
	
	// we need to iterate through all result sets to avoid errors
	int t_status = 0;
	
	while (t_success && (t_status == 0))
	{
		MYSQL_RES *t_result_set;
		t_result_set = NULL;

		int t_affected_rows;
		t_affected_rows = 0;

		// First we need to establish if the query returned a result set. This will be the case if it was 
		// a select query. If there is a result set, we need to store it temporarily and free it to avoid
		// errors ocurring in subsequent queries.
		t_result_set = mysql_store_result(getMySQL());
		t_success = (t_result_set != NULL) || (mysql_field_count(getMySQL()) == 0);
		
		if (t_success && (t_result_set == NULL))
		{
			t_affected_rows = mysql_affected_rows(getMySQL());
			t_success = -1 != t_affected_rows;
		}

		if (t_result_set != NULL)
			mysql_free_result(t_result_set);

		if (t_success && t_first)
		{
			p_affected_rows = t_affected_rows;
			t_first = false;
		}
		
		if (t_success)
		{
			t_status = mysql_next_result(getMySQL());
			t_success = t_status <= 0;
		}
	}

	// OK-2007-09-10 : Bug 5360
	if (t_success)
		errorMessageSet(NULL);
	else
		errorMessageSet(mysql_error(getMySQL()));

	return t_success;
}


/*Method to execute sql statements like SELECT and return Cursor
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
Output: NULL cursor on error
*/
DBCursor *DBConnection_MYSQL::sqlQuery(char *p_query, DBString *p_arguments, int p_argument_count, int p_rows)
{
	DBCursor_MYSQL *t_cursor;
	t_cursor = NULL;

	bool t_success = true;
	
	if (t_success)
		t_success = ExecuteQuery(p_query, p_arguments, p_argument_count);
		
	int t_status = 0;
	while (t_success && (t_status == 0))
	{
		if (t_cursor == NULL)
		{
			t_cursor = new (nothrow) DBCursor_MYSQL();
			t_success = (t_cursor != NULL) && t_cursor->open((DBConnection*)this);
		}
		else
		{
			// we need to fetch all available result sets, even if we do nothing with them.
			MYSQL_RES *t_result_set;
			t_result_set = mysql_store_result(getMySQL());
			
			t_success = (t_result_set != NULL) || (mysql_field_count(getMySQL()) == 0);
			if (t_result_set != NULL)
				mysql_free_result(t_result_set);
		}
		
		if (t_success)
		{
			t_status = mysql_next_result(getMySQL());
			t_success = t_status <= 0;
		}
	}
	
	if (t_cursor != NULL)
	{
		if (t_success)
			addCursor(t_cursor);
		else
		{
			delete t_cursor;
			t_cursor = NULL;
		}
	}

	// OK-2007-09-10 : Bug 5360
	if (t_success)
		errorMessageSet(NULL);
	else
		errorMessageSet(mysql_error(getMySQL()));

	return (DBCursor *)t_cursor;
}

bool queryCallback(void *p_context, int p_placeholder, DBBuffer &p_output)
{	
	QueryMetadata *t_query_metadata;
	t_query_metadata = (QueryMetadata *)p_context;

	if (p_placeholder > t_query_metadata -> argument_count)
	{
		// This means that the user has specified more placeholders in their query than they have provided variable names
		// to bind with, an error is the result
		return false;
	}

	DBString t_parameter_value;
	t_parameter_value = t_query_metadata -> arguments[p_placeholder - 1];

	char *t_escaped_string;
	t_escaped_string = (char *)malloc((t_parameter_value . length * 2) + 1);
	if (t_escaped_string == NULL)
		return false;

	unsigned long t_escaped_string_length;
	t_escaped_string_length = mysql_real_escape_string((MYSQL *)t_query_metadata -> connection, t_escaped_string, t_parameter_value . sptr, t_parameter_value . length);

	p_output . ensure(t_escaped_string_length + 2);
	memcpy(p_output . getFrontier(), "\"", 1);
	p_output . advance(1);
	memcpy(p_output . getFrontier(), t_escaped_string, t_escaped_string_length);
	p_output . advance(t_escaped_string_length);
	memcpy(p_output . getFrontier(), "\"", 1);
	p_output . advance(1);

	free(t_escaped_string);

	return true;
}

bool DBConnection_MYSQL::ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count)
{
	if (!isConnected)
		return false;
	
	unsigned int t_query_length;
	t_query_length = strlen(p_query);
	
	char *t_parsed_query;
	t_parsed_query = p_query;

	int *t_placeholders;
	t_placeholders = NULL;

	int t_placeholder_count;
	t_placeholder_count = 0;

	bool t_success;
	t_success = true;

	if (p_argument_count != 0)
	{
		QueryMetadata t_query_metadata;
		t_query_metadata . argument_count = p_argument_count;
		t_query_metadata . arguments = p_arguments;
		t_query_metadata . connection = getMySQL();

		DBBuffer t_query_buffer(t_query_length + 1);

		t_success = processQuery(p_query, t_query_buffer, queryCallback, &t_query_metadata);
		t_query_length = t_query_buffer . getSize();
		t_parsed_query = t_query_buffer . grab();
	}

	int t_error_code;
	if (t_success)
	{
		if ((t_error_code = mysql_real_query(getMySQL(), t_parsed_query, t_query_length)))
			t_success = false;
	}

	if (p_argument_count != 0)
		free(t_parsed_query);

	return t_success;
}


/*IsError-True on error*/
Bool DBConnection_MYSQL::IsError()
{
	if  (m_error != NULL || mysql_errno(getMySQL()))
		return True;
	else
		return False;
}

/*getErrorMessage- return error string*/
char *DBConnection_MYSQL::getErrorMessage(Bool p_last)
{
    // AL-2013-11-08 [[ Bug 11149 ]] Make sure most recent error string is available to revDatabaseConnectResult
	if (p_last || IsError() == True)
	{
		if (m_error != NULL)
			return m_error;
		else
			return (char *)mysql_error(getMySQL());
	}
	else
		return (char *)DBNullValue;
}


bool DBConnection_MYSQL::BindVariables(MYSQL_STMT *p_statement, DBString *p_arguments, int p_argument_count, int *p_placeholders, int p_placeholders_count, MYSQL_BIND **p_bind)
{
	if (p_placeholders_count == 0)
		return true;

	MYSQL_BIND *t_bind;
	t_bind = *p_bind;

	for (int i = 0; i < p_placeholders_count; i++)
	{
		if (p_placeholders[i] < 1 || p_placeholders[i] > p_argument_count)
			return false;

		DBString *t_parameter_value;
		t_parameter_value = &(p_arguments[p_placeholders[i] - 1]);
		
		t_bind[i] . buffer = (void *)t_parameter_value -> sptr;
		t_bind[i] . buffer_length = t_parameter_value -> length;
		t_bind[i] . length = (unsigned long *)&t_parameter_value -> length;

		if (t_parameter_value -> isbinary)
			t_bind[i] . buffer_type = MYSQL_TYPE_BLOB;
		else
			t_bind[i] . buffer_type = MYSQL_TYPE_STRING;
	}

	bool t_result;
	t_result = (mysql_stmt_bind_param(p_statement, t_bind) == 0);
	return t_result;
}

void DBConnection_MYSQL::getTables(char *buffer, int *bufsize)
{
	if (!buffer) {
        if (!m_internal_buffer)
        {
            m_internal_buffer = new large_buffer_t;
            
        }
        long buffersize = 0;
        char tsql[] = "SHOW tables";
        DBCursor *newcursor = sqlQuery(tsql, NULL, 0, 0);
        if (newcursor) {
            if (!newcursor->getEOF()){
                while (True){
                    unsigned int colsize;
                    char *coldata = newcursor->getFieldDataBinary(1,colsize);
                    colsize = strlen(coldata);
                    m_internal_buffer->append(coldata, colsize);
                    m_internal_buffer->append('\n');
                    buffersize += colsize + 1;
                    newcursor->next();
                    if (newcursor->getEOF()) break;
                }
            }
            deleteCursor(newcursor->GetID());
            *bufsize = buffersize+1;
            m_internal_buffer->append('\0');
            fprintf(stderr, "\n%d\n", *bufsize);
        }
		return;
	}
    memcpy((void *) buffer, m_internal_buffer->ptr(), m_internal_buffer->length());
    delete m_internal_buffer;
    m_internal_buffer = nullptr;
}

/*MYSQL doesn't support transactions so leave blank for now*/
void DBConnection_MYSQL::transBegin() {}
void DBConnection_MYSQL::transCommit(){}
void DBConnection_MYSQL::transRollback(){}
