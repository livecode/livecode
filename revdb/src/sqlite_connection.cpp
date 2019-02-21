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

// MM-2014-02-04: [[ Sqlite382 ]] Define string cmp functions for Windows.
#ifdef _WINDOWS
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#include "dbsqlite.h"
#include <sqlitedataset/sqlitedataset.h>

#include <sqlitedecode.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include <sstream>

#include <revolution/support.h>

DBConnection_SQLITE::DBConnection_SQLITE() :
	mErrorStr(0),
	mIsError(false)
{
	connectionType = CT_SQLITE;
	
	// MW-2014-01-29: [[ Sqlite382 ]] Make sure options are set to defaults (false).
	m_enable_binary = false;
	m_enable_extensions = false;
}

DBConnection_SQLITE::~DBConnection_SQLITE()
{
	setErrorStr(0);
	disconnect();
}

bool DBConnection_SQLITE::IsBinaryEnabled(void)
{
	return m_enable_binary;
}

// Only 1 arguments are expected.
//  args[0]: --unused--
//  args[1]: filename of database
//  args[2]: --unused--
//  args[..]: --unused--

Bool DBConnection_SQLITE::connect(char **args, int numargs)
{
	Bool ret = False;


	// MW-2014-01-29: [[ Sqlite382 ]] We only need a minimum of one argument.
	if(!isConnected && numargs >= 1) {
		char *fname;
		string bhash;
		string mash;

		// MW-2008-07-29: [[ Bug 6429 ]] Can't connect to databases with accents in the path.
		//   This occurs because the SQLite functions expect UTF-8 and we are currently passing
		//   then native encoded strings. The os_path_to_native_utf8 function returns a native
		//   path encoded as UTF-8.
		fname = os_path_to_native_utf8(args[0]);
		
        bool t_use_uri = false;
        
		// MW-2014-01-29: [[ Sqlite382 ]] If there's a second argument, then interpret
		//   it as an options string.
		if (numargs >= 2)
		{
			const char *t_start;
			t_start = args[1];
			for(;;)
			{
				// Find the end of the item (delimited by ',').
				const char *t_end;
				t_end = strchr(t_start, ',');
				if (t_end == NULL)
					t_end = t_start + strlen(t_start);
				
				// Check to see if we recognise the option (ignoring ones we don't know
				// anything about).
				if ((t_end - t_start) == 6 && strncasecmp(t_start, "binary", 6) == 0)
					m_enable_binary = true;
				if ((t_end - t_start) == 10 && strncasecmp(t_start, "extensions", 10) == 0)
					m_enable_extensions = true;
                if ((t_end - t_start) == 3 && strncasecmp(t_start, "uri", 3) == 0)
                    t_use_uri = true;
				
				// If the end points to NUL we are done.
				if (*t_end == '\0')
					break;
				
				// Start is the char after the separating ','.
				t_start = t_end + 1;
			}
		}

		try
		{
			mDB.setDatabase(fname);
			if(mDB.connect(t_use_uri) == DB_CONNECTION_NONE)
			{
				ret = False;
			}
			else
			{
				ret = True;
				isConnected = True;
				
				// MW-2014-01-29: [[ Sqlite382 ]] Now we have a handle, configure extension
				//   loading.
				sqlite3_enable_load_extension(mDB . getHandle(), m_enable_extensions ? 1 : 0);
			}
		}
		catch(DbErrors &e)
		{
			mIsError = true;
			setErrorStr(e.getMsg());
		}

		free(fname);
	}
	return ret;
}

const char *DBConnection_SQLITE::getconnectionstring()
{
	return DB_SQLITE_STRING;
}

void DBConnection_SQLITE::disconnect()
{
	MDEBUG0("SQLite::disconnect\n");
	if(isConnected) {
		//close all open cursors from this connection
		closeCursors();

		//close mysql connection
		mDB.disconnect();
		isConnected = False;
	}
}

/*Method to execute quick and fast sql statements like UPDATE and INSERT
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
affectedrows - will recieve number of rows updated or inserted
Output: False on error
*/
Bool DBConnection_SQLITE::sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows)
{
	MDEBUG0("SQLite::sqlExecute\n");
	Bool ret = True;

	if (!isConnected)
		return ret;
	else
	{
		char *newquery = query;
		int qlength = strlen(query);

		MDEBUG("args=%d, numargs=%d\n", args != 0);

		if(numargs > 0)
		{
			int newsize;
			newquery = BindVariables(query, qlength, args, numargs, newsize);
			qlength = newsize;
		}

		int rv = basicExec(newquery, &affectedrows);
        
        if (numargs > 0)
            free(newquery);

		if(rv != SQLITE_OK)
		{
			// MW-2008-07-29: [[ Bug 6639 ]] Executing a query doesn't return meaningful error messages.
			//   Make sure we only use a generic string if an error hasn't been set.
			if (!mIsError)
			{
				mIsError = true;
				setErrorStr("Unable to execute query");
			}
			ret = False;
		}
		else
			mIsError = false;
	}
	
	return ret;
}


int query_callback(void* res_ptr, int ncol, char** reslt, char** cols) 
{
	int *i = (int*)res_ptr;
	*i++;
	return 0;
}

/*Method to execute sql statements like SELECT and return Cursor
Inputs:
query- string containing sql query
qlength - length of query (for binary data). if 0 then assume null terminated.
Output: NULL cursor on error
*/
DBCursor *DBConnection_SQLITE::sqlQuery(char *query, DBString *args, int numargs, int p_rows)
{
	DBCursor_SQLITE *ret = 0;
	unsigned int qlength;
	char *newquery = (char *)query;

#ifndef NDEBUG
	MDEBUG0("SQLite::sqlQuery\n");
	MDEBUG("Numargs=[%d]\n", numargs);
	MDEBUG("Query=[%s]\n", query);
	
	for(int i = 0; i < numargs; i++) {
		MDEBUG("Args[%d]=[%s]\n", i);
	}
#endif
	
	if (!isConnected)
		return NULL;

	//if null terminated (qlength = 0) then calculate length of query
	qlength = strlen(query);
	//execute query and check for error

	if(numargs) {
		int newsize;
		newquery = BindVariables(query, qlength, args, numargs, newsize);
		qlength = newsize;
	}

	try {
		ret = new (nothrow) DBCursor_SQLITE(mDB, m_enable_binary);
		Dataset *ds = ret->getDataset();

		ds->query(newquery);
		//try to open cursor..on error delete invalid cursor object and exit with error
		if(!ret->open((DBConnection *)this)) {
			delete ret;
			ret = 0;
			mIsError = true;
			setErrorStr("Unable to open query");
		} else
			addCursor(ret); //add to cursor list

	} catch(DbErrors &e) {
		MDEBUG0("\n\n --- CAUGHT ERROR --- \n");
		mIsError = true;
		setErrorStr(e.getMsg());
		delete ret;
		ret = 0;
	}

	if (numargs)
		free(newquery);

	return ret;
}

/*IsError-True on error*/
Bool DBConnection_SQLITE::IsError()
{
	return mIsError;
}

/*getErrorMessage- return error string*/
char *DBConnection_SQLITE::getErrorMessage(Bool p_last)
{
    // AL-2013-11-08 [[ Bug 11149 ]] Make sure most recent error string is available to revDatabaseConnectResult
	if(p_last || mIsError)
    {
        if (mErrorStr != 0)
        {
            mIsError = false;
            return mErrorStr;
        }
    }
    
    return (char*)DBNullValue;
}

char *replaceString(char *p_string, char *p_find, char *p_replace)
{


	// Loop through p_string and work out how many ocurrences of p_find there are. From this, calculate
	// the amount of memory required for the output buffer
	char *t_current_string;
	t_current_string = p_string;

	char *t_matched_string;
	t_matched_string = NULL;

	int t_matches;
	t_matches = 0;

	while (t_current_string != NULL)
	{
		t_matched_string = strstr(t_current_string, p_find);
		if (t_matched_string == NULL)
			break;

		t_matches++;
		t_current_string = t_matched_string + strlen(p_find);
	}

	int t_buffer_length;
	char *t_output_buffer;
	if (t_matches == 0)
	{
		t_buffer_length = strlen(p_string + 1);
		t_output_buffer = strdup(p_string);
	}
	else
	{
		t_buffer_length = strlen(p_string) - (t_matches * strlen(p_find)) + (t_matches * strlen(p_replace)) + 1;
		t_output_buffer = (char *)malloc(t_buffer_length);
		t_output_buffer[t_buffer_length - 1] = '\0';
	}

    // Set these back to their original values to begin the find loop again
	t_matched_string = NULL;
	t_current_string = p_string;
	
	// Keep a copy of the output buffer so that the original variable can be incremented to keep track of where we are.
	char *t_output_buffer_copy;
	t_output_buffer_copy = t_output_buffer;
	
	bool t_matches_present;
	t_matches_present = (t_matches != 0);
	
	while (t_matches > 0)
	{
		// Find the string to match in the current string
		t_matched_string = strstr(t_current_string, p_find);
		
		// Copy from the beginning of the current string to where the match was found into the output buffer
		memcpy(t_output_buffer, t_current_string, (t_matched_string - t_current_string));
		
		// Update the current string and output buffer so that they point to the string to be replaced
		t_output_buffer = t_output_buffer + (t_matched_string - t_current_string);
		t_current_string = t_matched_string;
		
		// Copy the string to replace into the output buffer, overwriting the string to find
		strcpy(t_output_buffer, p_replace);
		
		// Update the output buffer by the length of the string we just copied into it
		t_output_buffer = t_output_buffer + strlen(p_replace);
		
		// Update the current string by the length of the find string so that the search can resume
		t_current_string = t_current_string + strlen(p_find);
		
		t_matches--;
	}
	
	// If there were any matches present, the bit of the string after the last match will not have been copied, do this here.
	if (t_matches_present)
	{
		strcpy(t_output_buffer, t_matched_string + 1);
	}

	return t_output_buffer_copy;
}

static char num2nibble(int p_nibble)
{
	if (p_nibble < 10)
		return '0' + p_nibble;
	return 'A' + (p_nibble - 10);
}

bool queryCallback(void *p_context, int p_placeholder, DBBuffer& p_output)
{
	QueryMetadata *t_query_metadata;
	t_query_metadata = (QueryMetadata *)p_context;
    
    
    // SN-2015-06-01: [[ Bug 15416 ]] Make sure that we don't access an out-of-
    //  bounds placeholder.
    if (p_placeholder > t_query_metadata -> argument_count)
        return false;

	DBString t_parameter_value;
	t_parameter_value = t_query_metadata -> arguments[p_placeholder - 1];

	void *t_escaped_string;
	t_escaped_string = NULL;

	size_t t_escaped_string_length;
	t_escaped_string_length = 0;
	
	// MW-2014-01-29: [[ Sqlite382 ]] If true the value needs quoting, otherwise it is
	//   already quoted appropriately.
	bool t_needs_quotes;
	t_needs_quotes = true;
	if (t_parameter_value . isbinary)
	{
		DBConnection_SQLITE *t_conn;
		t_conn = (DBConnection_SQLITE *)t_query_metadata -> connection;
		
		if (t_conn -> IsBinaryEnabled())
		{
			// MW-2014-01-29: [[ Sqlite382 ]] Encode the binary as BLOB literal - X'<hex>'. Thus
			//   the length of the escaped string is 3 + 2 * size.;
			t_escaped_string_length = 3 + 2 * t_parameter_value . length;
			t_escaped_string = malloc(t_escaped_string_length);
			t_needs_quotes = false;
			
			// Quote the binary!
			char *t_buffer;
			t_buffer = (char *)t_escaped_string;
			
			*t_buffer++ = 'X';
			*t_buffer++ = '\'';
			for(size_t i = 0; i < t_parameter_value . length; i++)
			{
				char t_high_nibble, t_low_nibble;
				t_high_nibble = num2nibble(((unsigned)t_parameter_value . sptr[i] & 0xff) >> 4);
				t_low_nibble = num2nibble(((unsigned)t_parameter_value . sptr[i]) & 0xf);
				*t_buffer++ = t_high_nibble;
				*t_buffer++ = t_low_nibble;
			}
			*t_buffer++ = '\'';
		}
		else
		{
			// According to documentation in sqlitedecode.cpp, this is the required size of output buffer
			t_escaped_string = malloc(2 + (257 * (int64_t)t_parameter_value . length) / 254);
			t_escaped_string_length = sqlite_encode_binary((const unsigned char *)t_parameter_value . sptr, t_parameter_value . length, (unsigned char *)t_escaped_string);
		}
	}
	else
	{
		if (t_parameter_value . length != 0)
		{
			// Null terminate the value
			char *t_value;
			t_value = (char *)malloc(t_parameter_value . length + 1);
			memcpy(t_value, t_parameter_value . sptr, t_parameter_value . length);
			t_value[t_parameter_value . length] = '\0';

			// Escape quotes by manually replacing then with the string "''"
			t_escaped_string = replaceString(t_value, "\'\0", "\'\'\0");
			t_escaped_string_length = strlen((const char *)t_escaped_string);

			free(t_value);
		}
	}

	if (t_needs_quotes)
	{
		p_output . ensure(t_escaped_string_length + 2);
		memcpy(p_output . getFrontier(), "'", 1);
		p_output . advance(1);
	}
	else
		p_output . ensure(t_escaped_string_length);

	if (t_escaped_string != NULL)
	{
		memcpy(p_output . getFrontier(), t_escaped_string, t_escaped_string_length);
		p_output . advance(t_escaped_string_length);
	}

	if (t_needs_quotes)
	{
		memcpy(p_output . getFrontier(), "'", 1);
		p_output . advance(1);
	}
	
	free(t_escaped_string);
	return true;
}


/*BindVariables-parses querystring and binds variables*/
char *DBConnection_SQLITE::BindVariables(char *p_query, int p_query_length, DBString *p_arguments, int p_argument_count, int &r_new_query_length)
{
	char *t_parsed_query;
	t_parsed_query = p_query;

	int t_parsed_query_length;
	t_parsed_query_length = p_query_length;

	bool t_success;
	t_success = true;

	if (p_argument_count != 0)
	{
		QueryMetadata t_query_metadata;
		t_query_metadata . argument_count = p_argument_count;
		t_query_metadata . arguments = p_arguments;
		t_query_metadata . connection = this;

		DBBuffer t_query_buffer(t_parsed_query_length + 1);

		t_success = processQuery(p_query, t_query_buffer, queryCallback, &t_query_metadata);

		t_query_buffer . ensure(1);
		*t_query_buffer . getFrontier() = '\0';
		t_query_buffer . advance(1);

		t_parsed_query_length = t_query_buffer . getSize();
		t_parsed_query = t_query_buffer . grab();
	}

	r_new_query_length = t_parsed_query_length;
	return t_parsed_query;
}

void DBConnection_SQLITE::getTables(char *buffer, int *bufsize)
{
	int rowseplen = 1;
	char rowsep[] = "\n";
	char **result;
	int rows;
	int rv;
	char *errmsg;
	int maxlen = *bufsize;
	int usedlen = 0;
	char *bufptr = buffer;
	
	// OK-2010-02-18: [[Bug 8620]] - Calculate the buffer size properly...
	if (buffer)
		memset(buffer,0,*bufsize);

	rv = sqlite3_get_table(mDB.getHandle(),
		"SELECT name FROM sqlite_master "
		"WHERE type IN ('table','view') "
		"UNION ALL "
		"SELECT name FROM sqlite_temp_master "
		"WHERE type IN ('table','view') "
		"ORDER BY 1",
		&result, &rows, 0, &errmsg);

	if (errmsg)
	{
		mIsError = true;
		setErrorStr(errmsg);
		sqlite3_free(errmsg);
	}

	
	if (rv == SQLITE_OK)
	{
		for(int i = 1; i <= rows; i++)
		{
			if (result[i] == 0) 
				i = rows + 1; // break
			else
			{
				int l = strlen(result[i]);
				if (buffer && ((l + usedlen + rowseplen + 1) > maxlen))
					i = rows + 1; // break
				else
				{
					if (!buffer)
					{
						usedlen = usedlen + l + rowseplen;
					}
					else
					{
						memcpy(bufptr, result[i], l);
						bufptr += l;
						usedlen += l;

						memcpy(bufptr, rowsep, rowseplen);
						bufptr += rowseplen;
						usedlen += rowseplen;
					}
				}
			}
		}

		// OK-2010-02-18: [[Bug 8621]] - Replace trailing return with null.
		if ((rows != 0) && buffer && (usedlen < maxlen))
			*(bufptr - 1) = '\0';
	}
	sqlite3_free_table(result);
	*bufsize = usedlen + 1;
}

int exec_callback(void *p_context, int p_column_count, char **p_values, char **p_names) 
{
	if (p_values != NULL)
	{
		int *i = (int *)p_context;
		*i = *i +1;
	}
	return 0;
}

void dataChangeCallback(void *p_context, int p_statement_type, char const *p_database, char const *p_table, sqlite_int64 p_row_id)
{
	int *t_context;
	t_context = (int *)p_context;

	*t_context = *t_context + 1;
}

int DBConnection_SQLITE::basicExec(const char *q, unsigned int *rows)
{
	char *err;
	int t_return_value = 0;

	if (rows == NULL)
		t_return_value = sqlite3_exec(mDB.getHandle(), q, 0, 0, &err);
	else 
	{
		int t_changed_row_count;
		t_changed_row_count = 0;
		sqlite3_update_hook(mDB.getHandle(),dataChangeCallback, &t_changed_row_count);

		*rows = 0;
		t_return_value = sqlite3_exec(mDB.getHandle(), q, exec_callback, rows, &err);

		int t_changed_rows;
		t_changed_rows = sqlite3_changes(mDB.getHandle());

		sqlite3_update_hook(mDB.getHandle(), NULL, NULL);
		
		// OK-2007-07-13: NOTE: When executing a delete query with no WHERE clause, SQLite deletes
		// and recreates the table, meaning that the row count will end up being 0 when it shouldnt be.

		// If *rows != 0 then rows was populated by the exec_callback function, which implies that the query
		// has returned a result set. As we are only executing the query here, we return 0 as the number of affected rows.
		if (*rows != 0)
			*rows = 0;
		else
			*rows = t_changed_row_count;
	}

	if (t_return_value != SQLITE_OK) 
	{
		mIsError = true;
		setErrorStr(err);
		sqlite3_free(err);
	}

	return t_return_value;
}

void DBConnection_SQLITE::setErrorStr(const char *msg)
{
	MDEBUG("\nsetErrorStr(%s)\n", msg);

	if(mErrorStr != 0) {
		free(mErrorStr);
		mErrorStr = 0;
	}

	if(msg) {
		mErrorStr = strdup(msg);
	}
}

void DBConnection_SQLITE::transBegin() 
{
	MDEBUG0("SQLite::transBegin\n");
	if(isConnected)
		basicExec("begin");
}

void DBConnection_SQLITE::transCommit()
{
	MDEBUG0("SQLite::transCommit\n");
	if(isConnected)
		basicExec("commit");
}

void DBConnection_SQLITE::transRollback()
{
	MDEBUG0("SQLite::transRollback\n");
	if(isConnected)
		basicExec("rollback");
}


