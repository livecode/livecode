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

#include "dbpostgresql.h"

extern bool load_ssl_library();

/*DBCONNECTION_POSTGRESQL - CONNECTION OBJECT FOR MYSQL DATABASES CHILD OF DBCONNECTION*/

char *strndup(const char *p_string, int p_length)
{
	char *t_result;
	t_result = new (nothrow) char[p_length + 1];
	memcpy(t_result, p_string, p_length);
	t_result[p_length] = '\0';
	return t_result;
}

Bool DBConnection_POSTGRESQL::connect(char **args, int numargs)
{
	if (isConnected)
		return True;
	
	// MW-2014-01-30: [[ Sqlite382 ]] Relaxed revdb_connect() so it only checks for at least
	//   one argument - we need at least 4 though.
	if (numargs < 4)
		return False;
	
	char *t_database;
	t_database = args[1];

	char *t_user;
	t_user = args[2];

	char *t_password;
	t_password = args[3];

	// OK-2007-06-25: Fix for bug 3232. Parse the host string to obtain the host and port parts
	char *t_delimiter;
	t_delimiter = strchr(args[0], ':');

	char *t_host;
	t_host = args[0];

	char *t_port;
	t_port = NULL;

	if (t_delimiter != NULL)
	{
		t_port = (t_delimiter + (1 * sizeof(char)));
		*t_delimiter = '\0';
	}

    const char *t_sslmode;
    t_sslmode = NULL;
    const char *t_sslcompression;
    t_sslcompression = NULL;
    const char *t_sslcert;
    t_sslcert = NULL;
    const char *t_sslkey;
    t_sslkey = NULL;
    const char *t_sslrootcert;
    t_sslrootcert = NULL;
    const char *t_sslcrl;
    t_sslcrl = NULL;

    // extract any ssl options spcified as key value pairs in the final args to revOpenDatabase
    // e.g. sslmode=require
    const char *t_ssl_opt_key;
    t_ssl_opt_key = NULL;
    const char *t_ssl_opt_val;
    t_ssl_opt_val = NULL;
    for (int i = 4; i < numargs; i++)
    {
        t_delimiter = strchr(args[i], '=');
        if (t_delimiter != NULL)
        {
            t_ssl_opt_key = args[i];
            t_ssl_opt_val = (t_delimiter + sizeof(char));

            size_t t_key_length;
            t_key_length = (size_t)(t_delimiter - args[i]);

            if (*t_ssl_opt_val != '\0')
            {
                if (strncmp(t_ssl_opt_key, "sslmode", t_key_length) == 0)
                    t_sslmode = t_ssl_opt_val;
                else if (strncmp(t_ssl_opt_key, "sslcompression", t_key_length) == 0)
                    t_sslcompression = t_ssl_opt_val;
                else if (strncmp(t_ssl_opt_key, "sslcert", t_key_length) == 0)
                    t_sslcert = t_ssl_opt_val;
                else if (strncmp(t_ssl_opt_key, "sslkey", t_key_length) == 0)
                    t_sslkey = t_ssl_opt_val;
                else if (strncmp(t_ssl_opt_key, "sslrootcert", t_key_length) == 0)
                    t_sslrootcert = t_ssl_opt_val;
                else if (strncmp(t_ssl_opt_key, "sslcrl", t_key_length) == 0)
                    t_sslcrl = t_ssl_opt_val;
            }
        }
    }

    bool t_have_ssl;
    t_have_ssl = load_ssl_library();

    // if an ssl mode (other than disable) has been passed, make sure we can load libopenssl
    // if no ssl mode has been passed, use prefer if we have libopenssl (try an ssl connection, if that fails try non-ssl)
    // if we don't have libopenssl, use disable (don't attempt an ssl connection)
    const char *t_sslmode_prefer = "prefer";
    const char *t_sslmode_disable = "disable";
    if (t_sslmode != NULL)
    {
        if (strcmp(t_sslmode, "disable") != 0 && !t_have_ssl)
        {
            errorMessageSet("revdb,unable to load SSL library");
            return false;
        }
    }
    else if (t_have_ssl)
        t_sslmode = t_sslmode_prefer;
    else
        t_sslmode = t_sslmode_disable;

    const char *t_connect_keys[] =
    {
        "host",
        "port",
        "dbname",
        "user",
        "password",

        "sslmode",
        "sslcompression",
        "sslcert",
        "sslkey",
        "sslrootcert",
        "sslcrl",

        NULL,
    };
    const char *t_connect_values[] =
    {
        t_host,
        t_port,
        t_database,
        t_user,
        t_password,

        t_sslmode,
        t_sslcompression,
        t_sslcert,
        t_sslkey,
        t_sslrootcert,
        t_sslcrl,

        NULL,
    };

    dbconn = NULL;
    dbconn = PQconnectdbParams(t_connect_keys, t_connect_values, 0);

	// OK-2008-05-16 : Bug where failed connections to postgres databases would
	// not return any error information. According to the postgres docs, dbconn
	// will only be null if there was not enough memory to allocate it.
	if (dbconn == NULL)
	{
		errorMessageSet("revdb,insufficient memory to connect to database");
		return false;
	}

	// If the connection failed, return the standard postgres error message.
	if (PQstatus(dbconn) == CONNECTION_BAD)
	{
		errorMessageSet(PQerrorMessage(dbconn));
		return false;
	}


	if (PQstatus(dbconn) == CONNECTION_BAD || !dbconn)
		return False;

	isConnected = true;  
	connectionType = CT_POSTGRESQL,
	isConnected = True;
	// OK-2007-09-10 : Bug 5360
	errorMessageSet(NULL);

	return True;
}

const char *DBConnection_POSTGRESQL::getconnectionstring()
{
	return DB_POSTGRESQL_STRING;
}

/*Method to disconnect from MySQL server*/
void DBConnection_POSTGRESQL::disconnect()
{
	if (!isConnected)
		return;
	closeCursors();
	PQfinish(dbconn);
	isConnected = False;
	errorMessageSet(NULL);
}

bool queryCallback(void *p_context, int p_placeholder, DBBuffer &p_output)
{
	QueryMetadata *t_query_metadata;
	t_query_metadata = (QueryMetadata *)p_context;

	DBString t_parameter_value;
	t_parameter_value = t_query_metadata -> arguments[p_placeholder - 1];

	void *t_escaped_string;
	t_escaped_string = NULL;

	size_t t_escaped_string_length;
	t_escaped_string_length = 0;
	
	if (t_parameter_value . isbinary)
	{
		t_escaped_string = PQescapeBytea((const unsigned char *)t_parameter_value . sptr, t_parameter_value . length, &t_escaped_string_length);
		if (t_escaped_string == NULL)
			return false;

		// PQescapeBytea appends an extra null char to the end of the escaped string, disregard this.
		t_escaped_string_length--;
	}
	else
	{
		if (t_parameter_value . length != 0)
		{
			t_escaped_string = malloc((t_parameter_value . length * 2) + 1);
			t_escaped_string_length = PQescapeString((char *)t_escaped_string, t_parameter_value . sptr, t_parameter_value . length);
		}
	}

	p_output . ensure(t_escaped_string_length + 2);
	memcpy(p_output . getFrontier(), "'", 1);
	p_output . advance(1);

	if (t_escaped_string != NULL)
	{
		memcpy(p_output . getFrontier(), t_escaped_string, t_escaped_string_length);
		p_output . advance(t_escaped_string_length);
	}

	memcpy(p_output . getFrontier(), "'", 1);
	p_output . advance(1);

	if (t_parameter_value . isbinary)
		PQfreemem(t_escaped_string);
	else
		free(t_escaped_string);
	
	return true;
}

PGresult *DBConnection_POSTGRESQL::ExecuteQuery(char *p_query, DBString *p_arguments, int p_argument_count)
{
	if (!isConnected)
		return NULL;

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
		t_query_metadata . connection = dbconn;

		DBBuffer t_query_buffer(t_query_length + 1);

		t_success = processQuery(p_query, t_query_buffer, queryCallback, &t_query_metadata);

		t_query_buffer . ensure(1);
		*t_query_buffer . getFrontier() = '\0';
		t_query_buffer . advance(1);

		t_query_length = t_query_buffer . getSize();
		t_parsed_query = t_query_buffer . grab();
	}

	PGresult *t_query_result;
	t_query_result = PQexec(dbconn, t_parsed_query);

	if (p_argument_count != 0)
	{
		free(t_parsed_query);
	}

	return t_query_result;
}


/*Method to execute quick and fast sql statements like UPDATE and INSERT
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
affectedrows - will recieve number of rows updated or inserted
Output: False on error
*/
Bool DBConnection_POSTGRESQL::sqlExecute(char *p_query, DBString *p_arguments, int p_argument_count, unsigned int &r_affected_rows)
{
	PGresult *t_postgres_result;
	t_postgres_result = ExecuteQuery(p_query, p_arguments, p_argument_count);
	if (t_postgres_result == NULL)
		return False;

	ExecStatusType t_status;
	t_status = PQresultStatus(t_postgres_result);

	Bool t_result;
	t_result = False;

	if (t_status == PGRES_COMMAND_OK)
	{
		t_result = True;
		char *t_affected_rows;
		t_affected_rows = PQcmdTuples(t_postgres_result);

		if (strcmp(t_affected_rows, "") == 0)
			r_affected_rows = 0;
		else
			r_affected_rows = atol(t_affected_rows);
	}

	if (t_status == PGRES_TUPLES_OK)
	{
		// This means the query was a select query. No rows would have been "affected"
		// so the result should be zero.
		t_result = True;
		r_affected_rows = 0;
	}

	// OK-2007-09-10 : Bug 5360, if the execution succeeded, we clear the error message, otherwise we set it.
	if (t_result)
		errorMessageSet(NULL);
	else
		errorMessageSet(PQerrorMessage(dbconn));

	PQclear(t_postgres_result);
	return t_result;
}


/*Method to execute sql statements like SELECT and return Cursor
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
Output: NULL cursor on error
*/
DBCursor *DBConnection_POSTGRESQL::sqlQuery(char *p_query, DBString *p_arguments, int p_argument_count, int p_rows)
{
	PGresult *t_postgres_result;
	t_postgres_result = ExecuteQuery(p_query, p_arguments, p_argument_count);

	ExecStatusType t_status;
	t_status = PQresultStatus(t_postgres_result);

	DBCursor_POSTGRESQL *t_cursor;
	t_cursor = NULL;

	if (t_status == PGRES_TUPLES_OK)
	{
		int t_column_count;
		t_column_count = PQnfields(t_postgres_result);
		if (t_column_count != 0)
		{
			t_cursor = new (nothrow) DBCursor_POSTGRESQL();
			if (!t_cursor -> open((DBConnection *)this, t_postgres_result))
			{
				delete t_cursor;
				t_cursor = NULL;
			}
			else 
				addCursor(t_cursor); 
		}
		t_postgres_result = NULL;
	}

	// OK-2007-09-10 : Bug 5360, if the query succeeded, we clear the error message, otherwise we set it.
	if (t_cursor != NULL)
		errorMessageSet(NULL);
	else
		errorMessageSet(PQerrorMessage(dbconn));

	return (DBCursor *)t_cursor;
}

/*IsError-True on error*/
Bool DBConnection_POSTGRESQL::IsError()
{
	char *errmsg = PQerrorMessage(dbconn);
	return *errmsg != '\0';
}

/*getErrorMessage- return error string*/
char *DBConnection_POSTGRESQL::getErrorMessage(Bool p_last)
{
    // AL-2013-11-08 [[ Bug 11149 ]] Make sure most recent error string is available to revDatabaseConnectResult
    if (p_last || IsError())
    {
        // OK-2007-09-10 : Bug 5360, modified to return m_error instead
        if (m_error != NULL)
            return m_error;
    }
    
    return (char *)DBNullValue;
}


void DBConnection_POSTGRESQL::getTables(char *buffer, int *bufsize)
{
	if (!buffer)
    {
        if (!m_internal_buffer)
        {
            m_internal_buffer = new large_buffer_t;
        }
        
        long buffersize = 0;
        char tsql[] = "SELECT tablename FROM pg_tables WHERE tablename NOT LIKE 'pg%'";
        DBCursor *newcursor = sqlQuery(tsql, NULL, 0, 0);
        if (newcursor) 
        {
            if (!newcursor->getEOF())
            {
                while (True)
                {
                    unsigned int colsize;
                    char *coldata = newcursor->getFieldDataBinary(1,colsize);
                    colsize = strlen(coldata);
                    m_internal_buffer->append(coldata, colsize);
                    m_internal_buffer->append('\n');
                    buffersize += colsize + 1;
                    newcursor->next();
                    if (newcursor->getEOF()) 
                        break;
                }
            }
            deleteCursor(newcursor->GetID());
            *bufsize = buffersize+1;
            m_internal_buffer->append('\0');
        }
        return;
    }
    memcpy((void *) buffer, m_internal_buffer->ptr(), m_internal_buffer->length());
    delete m_internal_buffer;
    m_internal_buffer = nullptr;
}

void DBConnection_POSTGRESQL::transBegin()
{
	/* start a transaction block */
	if (!isConnected)
		return;

	PGresult *pgres = NULL;
	pgres = PQexec(dbconn, "BEGIN");
	PQclear(pgres);
}

void DBConnection_POSTGRESQL::transCommit()
{
	/* start a transaction block */
	if (!isConnected)
		return;

	PGresult *pgres = NULL;
	pgres = PQexec(dbconn, "COMMIT");
	PQclear(pgres);
}

void DBConnection_POSTGRESQL::transRollback()
{
	if (!isConnected)
		return;

	PGresult *pgres = NULL;
	pgres = PQexec(dbconn, "ROLLBACK");
	PQclear(pgres);
}
