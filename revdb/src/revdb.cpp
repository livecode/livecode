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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "large_buffer.h"

#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
#include "w32support.h"
#elif defined(_LINUX) || defined(_LINUX_SERVER) || defined(TARGET_SUBPLATFORM_ANDROID)
#include "unxsupport.h"
#elif defined(_MACOSX) || defined (_MAC_SERVER)
#include "osxsupport.h"
#elif defined(TARGET_SUBPLATFORM_IPHONE)
#include "iossupport.h"
#endif

#if defined(_WINDOWS)
#pragma optimize("", off)
#endif

#include <revolution/external.h>
#include <revolution/support.h>

#include "dbdriver.h"
#include "dbdrivercommon.h"

#define INTSTRSIZE 16
#define STDRESULTSIZE 32
#define DEMOSIZE 64000
#define REVDB_VERSIONSTRING "3.0.0"

#define istrdup strdup

static unsigned int idcounter = 0;
char *MCS_resolvepath(const char *path);
static char *revdbdriverpaths = NULL;
static Bool REVDBinited = True;
unsigned int *DBObject::idcounter = NULL;

enum RevDBErrs
{
	REVDBERR_LICENSE = 0,
	REVDBERR_SYNTAX,
	REVDBERR_DBTYPE,
	REVDBERR_BADCONNECTION,
	REVDBERR_BADCURSOR,
	REVDBERR_BADCOLUMNNUM,
	REVDBERR_BADCOLUMNNAME,
	REVDBERR_BADTABLE,
	REVDBERR_NOT_SUPPORTED,
	REVDBERR_NOFILEPERMS,
	REVDBERR_NONETPERMS,
};

const char *errors[] = {
	"revdberr, restricted under current license",
	"revdberr,syntax error",
	"revdberr,invalid database type",
	"revdberr,invalid connection id",
	"revdberr,invalid cursor id",
	"revdberr,invalid column number",
	"revdberr,invalid column name",
	"revdberr,invalid table name",
	"revdberr,not supported by driver",
	"revdberr,file access not permitted",
	"revdberr,network access not permitted",
};

#define REVDB_PERMISSION_NONE		(0)
#define REVDB_PERMISSION_FILE		(1<<0)
#define REVDB_PERMISSION_NETWORK	(1<<1)

enum REVDBDatabaseType
{
	kREVDBDatabaseTypeOracle,
	kREVDBDatabaseTypeMySQL,
	kREVDBDatabaseTypePostgreSQL,
	kREVDBDatabaseTypeSQLite,
	kREVDBDatabaseTypeODBC,
	kREVDBDatabaseTypeValentina,
};

static const char *REVDBdatabasetypestrings[] = {
	"oracle",
	"mysql",
	"postgresql",
	"sqlite",
	"odbc",
	"valentina",
};

#define REVDB_DATABASETYPECOUNT (sizeof(REVDBdatabasetypestrings) / sizeof(char *))

// don't handle permissions for oracle, valentina as these drivers won't be included in plugin
static int REVDBdatabasepermissions[] = {
	REVDB_PERMISSION_NONE,
	REVDB_PERMISSION_NETWORK,
	REVDB_PERMISSION_NETWORK,
	REVDB_PERMISSION_FILE,
	REVDB_PERMISSION_FILE | REVDB_PERMISSION_NETWORK,
	REVDB_PERMISSION_NONE,
};

const char *dbtypestrings[] = {
	"NULL",
	"BIT", 
	"CHAR",
	"STRING",
	"WSTRING",
	"BLOB",
	"TIMESTAMP",
	"DATE",
	"TIME",
	"DATETIME",
	"FLOAT",
	"DOUBLE",
	"INTEGER",
	"SMALLINT",
	"WORD",
	"BOOLEAN",
	"LONG"

};

static void *DBcallback_loadmodule(const char *p_path)
{
    int t_success;
    void *t_handle;
    LoadModuleByName(p_path, &t_handle, &t_success);
    if (t_success == EXTERNAL_FAILURE)
        return NULL;
    return t_handle;
}

static void DBcallback_unloadmodule(void *p_handle)
{
    int t_success;
    UnloadModule(p_handle, &t_success);
}

static void *DBcallback_resolvesymbol(void *p_handle, const char *p_symbol)
{
    int t_success;
    void *t_address;
    ResolveSymbolInModule(p_handle, p_symbol, &t_address, &t_success);
    if (t_success == EXTERNAL_FAILURE)
        return NULL;
    return t_address;
}

static DBcallbacks dbcallbacks = {
    DBcallbacks_version,
    DBcallback_loadmodule,
    DBcallback_unloadmodule,
    DBcallback_resolvesymbol,
};

DATABASERECList databaselist;
DBList connectionlist;

#define simpleparse(a,b,c) (((b > a) | (c < a))?True:False)

static char *strlwr(char *str)
{
    while (*str)
    {
        *str = tolower(*str);
        str++;
    }
    return str;
}

void REVDB_Init(char *args[], int nargs, char **retstring,
		   Bool *pass, Bool *error)
{
	*error = False;
	*pass = False;

	*retstring = (char *)calloc(1, 1);
}

/// Returns the version of revdb.
void REVDB_Version(char *args[], int nargs, char **retstring,
		   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	 result = istrdup(REVDB_VERSIONSTRING);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


int util_stringcompare(const char *s1, const char *s2, int n)
{
    int i;
    char c1, c2;
    for (i=0; i<n; i++)
    {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (!c1) return 0;
    }
    return 0;
}

//utility function for iterating over connectionlist to find cursor
DBCursor *findcursor(int cursid)
{
	DBObjectList *connlist = connectionlist.getList();
	DBObjectList::iterator theIterator;
	for (theIterator = connlist->begin(); theIterator != connlist->end(); theIterator++){
		DBConnection *curconnection = (DBConnection *)(*theIterator);
		DBCursor *tcursor = curconnection->findCursor(cursid);
		if (tcursor) return tcursor;
	}
	return NULL;
}

// AL-2015-02-10: [[ SB Inclusions ]] Add function to load database driver using new module loading callbacks
DATABASEREC *LoadDatabaseDriverFromName(const char *p_type)
{
    int t_retvalue;
    void *t_handle;
    t_handle = NULL;
    LoadModuleByName(p_type, &t_handle, &t_retvalue);
    
    if (t_handle == NULL)
        return NULL;
    
    DATABASEREC *t_result;
    t_result = new (nothrow) DATABASEREC;
	t_result -> driverref = t_handle;
    
    void *id_counterref_ptr, *new_connectionref_ptr, *release_connectionref_ptr;
    void *set_callbacksref_ptr;
    id_counterref_ptr = NULL;
    new_connectionref_ptr = NULL;
    release_connectionref_ptr = NULL;
    set_callbacksref_ptr = NULL;
    
    ResolveSymbolInModule(t_handle, "setidcounterref", &id_counterref_ptr, &t_retvalue);
    ResolveSymbolInModule(t_handle, "newdbconnectionref", &new_connectionref_ptr, &t_retvalue);
    ResolveSymbolInModule(t_handle, "releasedbconnectionref", &release_connectionref_ptr, &t_retvalue);
    ResolveSymbolInModule(t_handle, "setcallbacksref", &set_callbacksref_ptr, &t_retvalue);
    
    t_result -> idcounterptr = (idcounterrefptr)id_counterref_ptr;
    t_result -> newconnectionptr = (new_connectionrefptr)new_connectionref_ptr;
    t_result -> releaseconnectionptr = (release_connectionrefptr)release_connectionref_ptr;
    t_result -> setcallbacksptr = (set_callbacksrefptr)set_callbacksref_ptr;
    
    return t_result;
}

DATABASEREC *LoadDatabaseDriverInFolder(const char *p_folder,
                                        const char *p_type)
{
    DATABASEREC *t_database_rec = nullptr;
    
    char t_libname[256];
    if (0 > snprintf(t_libname,
                     sizeof(t_libname),
                     "%s/db%s",
                     p_folder,
                     p_type))
    {
        return nullptr;
    }
    
    t_database_rec = LoadDatabaseDriverFromName(t_libname);
    
    return t_database_rec;
}

DATABASEREC *LoadDatabaseDriver(const char *p_type)
{
    DATABASEREC *t_database_rec = nullptr;
    
    char t_type[32];
    strcpy(t_type, p_type);
    strlwr(t_type);

    if (revdbdriverpaths != nullptr)
    {
        t_database_rec = LoadDatabaseDriverInFolder(revdbdriverpaths, p_type);
    }
    else
    {
        t_database_rec = LoadDatabaseDriverInFolder(".", t_type);
        
        
        if (t_database_rec == nullptr)
            t_database_rec = LoadDatabaseDriverInFolder("./Database Drivers",
                                                        t_type);

        if (t_database_rec == nullptr)
            t_database_rec = LoadDatabaseDriverInFolder("./database_drivers",
                                                        t_type);

        if (t_database_rec == nullptr)
            t_database_rec = LoadDatabaseDriverInFolder("./drivers",
                                                        t_type);
    }
    
    if (t_database_rec != nullptr)
    {
        databaselist . push_back(t_database_rec);
        strcpy(t_database_rec -> dbname, p_type);
        if (t_database_rec -> idcounterptr)
            (*t_database_rec -> idcounterptr)(&idcounter);
        if (t_database_rec -> setcallbacksptr)
            (*t_database_rec -> setcallbacksptr)(&dbcallbacks);
    }
	
	return t_database_rec;
}

void UnloadDatabaseDriver(DATABASEREC *rec)
{
    int t_retvalue;
    UnloadModule(rec->driverref, &t_retvalue);
    delete rec;
}

void REVDB_INIT()
{
}

void REVDB_QUIT()
{
	DBObjectList::iterator theIterator;
	DBObjectList *connlist = connectionlist.getList();
	for (theIterator = connlist->begin(); theIterator != connlist->end(); theIterator++){
		DBObject *curobject = (DBObject *)(*theIterator);
		DATABASEREC *databaserec = NULL;
		DATABASERECList::iterator theIterator2;

		DBConnection *tconnection = (DBConnection *)curobject;
		for (theIterator2 = databaselist.begin(); theIterator2 != databaselist.end(); theIterator2++){
			DATABASEREC *tdatabaserec = (DATABASEREC *)(*theIterator2);
			if (util_stringcompare(tdatabaserec->dbname,tconnection->getconnectionstring(),
				strlen(tconnection->getconnectionstring())) == 0){
				databaserec = tdatabaserec;
				break;
			}
		}
		if (databaserec != NULL && databaserec ->releaseconnectionptr != NULL)
			(*databaserec->releaseconnectionptr)(tconnection);
	}
	connlist->clear();
	DATABASERECList::iterator theIterator2;
	for (theIterator2 = databaselist.begin(); theIterator2 != databaselist.end(); theIterator2++){
		DATABASEREC *tdatabaserec = (DATABASEREC *)(*theIterator2);
		UnloadDatabaseDriver(tdatabaserec);
	}
	databaselist.clear();
}

//get column number by name
int findcolumn(DBCursor *thecursor, char *colname)
{
	int i;
	for (i = 1; i <= thecursor->getFieldCount();i++)
		if (util_stringcompare(colname,thecursor->getFieldName(i),strlen(colname)+1) == 0)
			return i;
	return 0;
}

/// @brief Utility function to parse a variable name that might be an array reference
/// @param p_full_name null terminated string containing the full variable name to parse
/// @param p_variable_name (output) a pointer that will be set to the address of a buffer containing the variable name
/// @param p_key_name (output) a pointer that will be set to the address of a buffer containing the key name.
/// The variable name can be something like "tVariable" in which case p_key_name will be set to null
/// and p_variable_name to "tVariable. Alternatively the variable name can be something like "tArray[key]", in which
/// case p_variable_name will be set to "tArray" and p_key_name will be set to "key". Both p_variable_name and p_key_name
/// should be free by the caller.
void parseVariableName(char *p_full_name, char *&p_variable_name, char *&p_key_name)
{
	char *t_key_start;
	t_key_start = strstr(p_full_name, "[");

	if (t_key_start != NULL)
	{
		char *t_end;
		t_end = &p_full_name[strlen(p_full_name) - 1];

		p_variable_name = (char *)malloc(sizeof(char) * (t_key_start - p_full_name) + 1);
		memcpy(p_variable_name, p_full_name, ((t_key_start - p_full_name)));
		p_variable_name[t_key_start - p_full_name] = '\0';

		p_key_name = (char *)malloc(sizeof(char) * (t_end - t_key_start));
		memcpy(p_key_name, t_key_start + 1, t_end - t_key_start - 1);
		p_key_name[t_end - t_key_start - 1] = '\0';
	}
	else
	{
		p_key_name = NULL;
		p_variable_name = (char *)malloc(sizeof(char) * strlen(p_full_name) + 1);
		strcpy(p_variable_name, p_full_name);
	}
}


/// @brief Utility function to get the value of a variable that may be an array element. Also returns whether the value is binary.
/// @param p_variable_name null terminated string containing the name of the variable or array.
/// @param p_key_name null terminated string containing the name of the array key or NULL if not an array element.
/// @param r_value The value and length of the variable is placed into this return parameter
/// @param r_is_binary This is set to true if the variable is binary, false otherwise.
void processInputArray(char *p_variable_name, char *p_key_name, ExternalString &r_value, Bool &r_is_binary)
{
	// OK-2007-10-05 : Changed behavior to use the variable name / key name to determine if the value is binary.
	// Previous behavior checked the first two chars of the actual value. The variable and key name are adjusted if appropriate
	// to remove the *b from them in order to allow their values to be retrieved.
	r_is_binary = False;

	char *t_adjusted_variable_name;
	t_adjusted_variable_name = p_variable_name;

	char *t_adjusted_key_name;
	t_adjusted_key_name = p_key_name;

	// If the variable / array name is prefixed by *b then the value is binary. (If the variable is an array prefixed by *b then
	// all elements are treated as binary).
	if (p_variable_name != NULL && strlen(p_variable_name) >= 2 && p_variable_name[0] == '*' && p_variable_name[1] == 'b')
	{
		r_is_binary = True;

		// Remove the first two chars
		t_adjusted_variable_name = t_adjusted_variable_name + 2;
	}

	// If the variable is an array element and the key name is prefixed by *b then it is binary.
	if (p_key_name != NULL && strlen(p_key_name) >= 2 && p_key_name[0] == '*' && p_key_name[1] == 'b')
	{
		r_is_binary = True;

		// Remove the first two chars
		t_adjusted_key_name = t_adjusted_key_name + 2;
	}

	ExternalString t_value;
	t_value . buffer = NULL;
	t_value . length = 0;

	int t_return_value;

	GetVariableEx(t_adjusted_variable_name, t_adjusted_key_name == NULL ? "" : t_adjusted_key_name, &t_value, &t_return_value);

	r_value = t_value;
}


char **s_sort_keys;
static int SortKeysCallback(const void *a, const void *b)
{
	unsigned int t_index_a, t_index_b;
	t_index_a = *(unsigned int *)a;
	t_index_b = *(unsigned int *)b;

	char *t_key_a, *t_key_b;
	t_key_a = s_sort_keys[t_index_a];
	t_key_b = s_sort_keys[t_index_b];

	if (t_key_a[0] == '*' && t_key_a[1] == 'b')
		t_key_a += 2;

	if (t_key_b[0] == '*' && t_key_b[1] == 'b')
		t_key_b += 2;

	int t_key_index_a, t_key_index_b;
	t_key_index_a = atoi(t_key_a);
	t_key_index_b = atoi(t_key_b);

	return t_key_index_a - t_key_index_b;
}

//extract arguments from variable list or array
DBString *BindVariables(char *p_arguments[],int p_argument_count, int &r_value_count)
{
	DBString *t_values;
	t_values = NULL;
	r_value_count = 0;

	// p_argument_count includes the connection id and the query. Therefore if it's 2 or less this means
	// there are no parameters to bind.
	if (p_argument_count <= 2)
		return t_values;

	int t_return_value;

	// If there is only a single argument to bind, it may be an array. If there are more than 3 arguments, none of them
	// can be arrays.
	if (p_argument_count == 3)
	{
		int t_element_count;
		t_element_count = 0;

		// Work out if the third argument is an array or not
		GetArray(p_arguments[2], &t_element_count, NULL, NULL, &t_return_value);

		char **t_array_keys;
		ExternalString *t_array_values;
		
		if (t_element_count != 0)
		{
			t_array_keys = (char **)malloc(sizeof(char *) * t_element_count);
			t_array_values = (ExternalString *)malloc(sizeof(ExternalString) * t_element_count);
			GetArray(p_arguments[2], &t_element_count, t_array_values, t_array_keys, &t_return_value);
			
			// MW-2008-02-29: [[ Bug 5893 ]] We need to sort the list of keys and values before binding
			//   as keys are unordered. To do this we create a temporary sequence of integers, and use
			//   qsort. This 'map' is then used to fetch keys and values from the array.
			unsigned int *t_map;
			t_map = (unsigned int *)malloc(sizeof(unsigned int) * t_element_count);
			for(int i = 0; i < t_element_count; ++i)
				t_map[i] = i;

			s_sort_keys = t_array_keys;
			qsort(t_map, t_element_count, sizeof(unsigned int), SortKeysCallback);

			// If there are multiple elements...
			t_values = new (nothrow) DBString[t_element_count];
			for (int i = 0; i < t_element_count; i++)
			{
				char *t_key_buffer;
				t_key_buffer = t_array_keys[t_map[i]];

				// Ensure that the key is an integer (and at the same time determine if the element is binary)
				char *t_end_pointer;
				Bool t_is_binary;
				t_is_binary = False;
				strtol(t_key_buffer, &t_end_pointer, 10);
				if (*t_end_pointer != 0)
				{
					if (strlen(t_key_buffer) >= 2 && t_key_buffer[0] == '*' && t_key_buffer[1] == 'b')
					{
						strtol((t_key_buffer + 2), &t_end_pointer, 10);
						if (*t_end_pointer != 0)
						{
							// This array element is neither an integer or an integer prefixed by "*b"
							// therefore we ignore it and skip onto the next element.
							continue;
						}
						else
							t_is_binary = True;
					}
					else
						t_is_binary = False;
				}

				// OK-2008-12-09: Because the engine retains ownership of t_value . buffer, using it can lead
				// to problems where the engine overwrites the buffer, leading to the wrong values being
				// inserted into databases. The problem is fixed by creating duplicates of any buffers returned.
				char *t_new_buffer;
				t_new_buffer = (char *)malloc(t_array_values[t_map[i]] . length);
				memcpy((void *)t_new_buffer, t_array_values[t_map[i]] . buffer, t_array_values[t_map[i]] . length);

				t_values[r_value_count] . Set(t_new_buffer, t_array_values[t_map[i]] . length, t_is_binary);
				r_value_count += 1;

			}

			free(t_map);

			free(t_array_keys);
			free(t_array_values);

			return t_values;
		}
	}

	t_values = new (nothrow) DBString[p_argument_count - 2];
	for (int i = 2; i < p_argument_count; i++)
	{
		Bool t_is_binary;
		t_is_binary = False;

		char *t_name;
		t_name = p_arguments[i];
		if (strlen(t_name) >= 2 && (t_name[0] == '*' && t_name[1] == 'b'))
		{
			t_is_binary = True;
			t_name += 2;
		}	

		ExternalString t_value;
		t_value . buffer = NULL;
		t_value . length = 0;
		
		// OK-2007-07-16: Enhancement 3603, allow for variable names in the form "pArray[key]"
		char *t_variable_name;
		char *t_key_name;

		parseVariableName(t_name, t_variable_name, t_key_name);

		GetVariableEx(t_variable_name, t_key_name != NULL ? t_key_name : "", &t_value, &t_return_value);

		free(t_variable_name);
		free(t_key_name);

		if (t_value . buffer != NULL) 
		{
            // OK-2008-12-09: Because the engine retains ownership of t_value . buffer, using it can lead
            // to problems where the engine overwrites the buffer, leading to the wrong values being
            // inserted into databases. The problem is fixed by creating duplicates of any buffers returned.
            char *t_new_buffer;
            t_new_buffer = (char *)malloc(t_value . length);
            memcpy(t_new_buffer, t_value . buffer, t_value . length);

			t_values[r_value_count] . Set(t_new_buffer, t_value . length, t_is_binary);
			r_value_count += 1;
		}
	}

	return t_values;
}


//utility function to get column data by number
void GetColumnByNumber(DBCursor *thecursor, char *&result, int columnid, char *varname)
{
	unsigned int colsize;
	colsize = 0;

	// OK-2007-06-18 : Part of fix for bug 4211
	char *coldata;
	coldata = NULL;

	if (thecursor -> getRecordCount() != 0)
		coldata = thecursor -> getFieldDataBinary(columnid, colsize);
	else
	{
		coldata = (char *)malloc(sizeof(char) * 1);
		*coldata = '\0';
	}
	
	if (colsize == 0xFFFFFFFF)
		colsize = strlen(coldata);

	if (coldata)
	{
		if (varname)
		{
			// OK-2007-07-04: Bug 3602. Enhancement request to allow variable name to be an array key.
			char *t_variable;
			char *t_key;

			parseVariableName(varname, t_variable, t_key);

			int retvalue;
			ExternalString val;
			val.buffer = coldata;
			val.length = colsize;

			if (!REVDBinited && thecursor -> getConnection() -> getConnectionType() == CT_ORACLE && val.length > 64000)
				result = istrdup(errors[REVDBERR_LICENSE]);
			else
				SetVariableEx(t_variable, t_key == NULL ? "" : t_key, &val, &retvalue);
			
			free(t_variable);
			free(t_key);
		}
		else
		{
			result = (char *)malloc(colsize + 1);
			memcpy(result, coldata, colsize);
			result[colsize] = '\0'; 
		}
	}
	else 
		result = istrdup(errors[REVDBERR_BADCOLUMNNUM]);

	if (thecursor -> getRecordCount() == 0)
		free(coldata);

}


inline const char *BooltoStr(Bool b) {return b == True?"True":"False";}

/// @brief Sets the location that revdb should look for drivers next time a connection is attempted.
/// @param driverPath The folder path to look for drivers in. This should be a path in LiveCode format.
/// No input checking is carried out, if the path is invalid, then the intended drivers will just not be found.
void REVDB_SetDriverPath(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	*error = False;
	*pass = False;
	if (nargs == 1)
	{
		if (revdbdriverpaths != NULL)
			free(revdbdriverpaths);
		revdbdriverpaths = istrdup(args[0]);
	}
	*retstring = static_cast<char*>(calloc(1,1));
}

void REVDB_GetDriverPath(char *p_arguments[], int p_argument_count, char **r_return_string, Bool *r_pass, Bool *r_error)
{
	*r_error = False;
	*r_pass = False;
	*r_return_string = istrdup(revdbdriverpaths);
}

/// @brief Opens a connection to a database.
/// @param databaseType String used to determine which database driver is loaded.
/// @param host The host to connect to in the format address:port.
/// @param databaseName The name of the database to use.
/// @param username The username to log into the database with.
/// @param password The password to log into the database with.
/// @param useSSL Whether to use SSL or not. This parameter is ignored by all drivers except MySQL. The default for MySQL, if this parameter is empty is to use SSL.
/// @param valentinaCacheSize Ignored by all other drivers.
/// @param valentinaMacSerial Ignored by all other drivers.
/// @param valentinaWindowsSerial Ignored by all other drivers.
///
/// @return An integer connection id if successful. Otherwise the return value is an error string from the driver that describes the problem. The SQLite driver returns
/// empty if there was an error connecting. Throws an error if less than 5 arguments are given.
///
/// Finds and loads the appropriate database driver, then obtains access to a connection object from the driver.
/// Calls the connect() method of the connection object, passing all the parameters except the database type.
/// The only difference in semantics between drivers is that MySQL takes the useSSL parameter and Valentina takes the valentinaCacheSize, 
/// valentinaMacSerial and valentinaWindowsSerial parameters.
void REVDB_Connect(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	
	// MW-2014-01-30: [[ Sqlite382 ]] Make this a little more flexible - only require at least
	//   one argument.
	if (nargs >= 1) 
	{
		*error = False;
		char *dbtype = args[0];
		DBConnection *newconnection = NULL;
		DATABASEREC *databaserec = NULL;
		DATABASERECList::iterator theIterator;
		for (theIterator = databaselist.begin(); theIterator != databaselist.end(); theIterator++)
		{
			DATABASEREC *tdatabaserec = (DATABASEREC *)(*theIterator);
			if (util_stringcompare(tdatabaserec->dbname,dbtype,strlen(dbtype)) == 0)
			{
				databaserec = tdatabaserec;
				break;
			}
		}

		// check access permissions of known database types
		size_t t_dbtype_index;
		for (t_dbtype_index = 0; t_dbtype_index < REVDB_DATABASETYPECOUNT; t_dbtype_index++)
		{
			if (util_stringcompare(REVDBdatabasetypestrings[t_dbtype_index], dbtype, strlen(dbtype)) == 0)
				break;
		}
		if (t_dbtype_index < REVDB_DATABASETYPECOUNT)
		{
			if ((REVDBdatabasepermissions[t_dbtype_index] & REVDB_PERMISSION_FILE) && !SecurityCanAccessFile(args[1]))
			{
				*error = True;
				result = istrdup(errors[REVDBERR_NOFILEPERMS]);
			}
			else if ((REVDBdatabasepermissions[t_dbtype_index] & REVDB_PERMISSION_NETWORK) && !SecurityCanAccessHost(args[1]))
			{
				*error = True;
				result = istrdup(errors[REVDBERR_NONETPERMS]);
			}
		}

		if (!*error)
		{
			if (!databaserec)
				databaserec = LoadDatabaseDriver(dbtype);

			if (databaserec && databaserec ->newconnectionptr) 
				newconnection = (*databaserec->newconnectionptr)();

			if (newconnection != NULL)
			{
				if (newconnection->connect(&args[1],nargs-1))
				{
					connectionlist.add(newconnection);
					unsigned int connid = newconnection->GetID();
					result = (char *)malloc(INTSTRSIZE);
					sprintf(result,"%d",connid);
				}
				else 
				{
					result = istrdup(newconnection->getErrorMessage());
					if (databaserec && databaserec ->releaseconnectionptr)
					   (*databaserec->releaseconnectionptr)(newconnection);
				}
			}
			else result = istrdup(errors[REVDBERR_DBTYPE]);
		}
	}
	else
	{
		result = istrdup(errors[REVDBERR_SYNTAX]);
		*error = True;
	}
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

/// @brief Closes a connection to a database
/// @param connectionId The integer id of the connection to close.
/// @return Empty if successful, an error string otherwise.
///
/// Locates the appropriate connection object by its id, then uses the DATABASEREC::releaseconnectionptr method
/// to release the connection. Throws an error if the wrong number of parameters is given. If pConnectionId is invalid
/// then REVDB_Disconnect returns an error string.
void REVDB_Disconnect(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	*error = True;
	*pass = False;

	if (nargs != 1)
	{
		*retstring = istrdup(errors[REVDBERR_SYNTAX]);
		*error = True;
		return;
	}

	*error = False;
	unsigned int connectionid = strtoul (*args, NULL, 10);

	if (!connectionlist . find(connectionid))
	{	
		*retstring = istrdup(errors[REVDBERR_BADCONNECTION]);
		*error = True;
		return;
	}

	DBObjectList::iterator t_iterator;
	DBObjectList *connlist = connectionlist.getList();
	for (t_iterator = connlist -> begin(); t_iterator != connlist -> end(); t_iterator++)
	{
		DBObject *curobject = (DBObject *)(*t_iterator);
		if (curobject -> GetID() == connectionid)
		{
			DATABASEREC *databaserec = NULL;
			DATABASERECList::iterator theIterator2;
			DBConnection *tconnection = (DBConnection *)curobject;
			for (theIterator2 = databaselist.begin(); theIterator2 != databaselist.end(); theIterator2++)
			{
				DATABASEREC *tdatabaserec = (DATABASEREC *)(*theIterator2);
				if ((util_stringcompare(tdatabaserec->dbname,tconnection->getconnectionstring(), strlen(tconnection->getconnectionstring())) == 0) || (util_stringcompare(tdatabaserec->dbname,"odbc", strlen("odbc")) == 0))
				{
					databaserec = tdatabaserec;
					break;
				}
			}
			if (databaserec && databaserec->releaseconnectionptr)
			{
				(*databaserec -> releaseconnectionptr)(tconnection);
            }
		
			connlist->erase(t_iterator);
			break;
		}
	}

	*retstring = static_cast<char*>(calloc(1,1));
}

/// @brief Commit the last transaction.
/// @param connectionId The integer id of the connection to use.
/// @return Empty if successful, an error string otherwise.
///
/// Throws an error if the wrong number of parameters is given. Returns an error string
/// If pConnectionId is invalid then an error string is returned, otherwise will always return empty.
/// The behavior of this command depends on the driver being used in the following manner.
/// @par MySQL
/// Has no affect.
/// @par ODBC
/// Has no affect.
/// @par Oracle
/// Commits the last transaction using ocom()
/// @par Postgresql
/// Commits the last transaction using PQExec(..., "COMMIT").
/// @par SQLite
/// Commits the last transaction using basicExec("commit").
void REVDB_Commit(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);

		if (theconnection) 
			theconnection->transCommit();
		else 
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else 
	{
		result = istrdup(errors[REVDBERR_SYNTAX]);
		*error = True;
	}

	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


/// @brief Rollback the last transaction
/// @param connectionId The integer id of the connection to use.
///
/// Throws an error if the wrong number of parameters are given. Returns an error string if an invalid
/// connection id is given, otherwise returns empty.
/// The behavior of this command depends on the driving being used in the following manner.
/// @par MySQL
/// Has no affect.
/// @par ODBC
/// Has no affect.
/// @par Oracle
/// Rolls back the last transaction using orol().
/// @par Postgresql
/// Rolls back the last trasaction using PQExec(..., "ROLLBACK").
/// @par SQLite
/// Rolls back the last transaction using basicExec("rollback").
/// \return Empty if successful, an error string otherwise.
void REVDB_Rollback(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);

		if (theconnection)
			theconnection->transCommit();
		else 
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else
		result = istrdup(errors[REVDBERR_SYNTAX]);

	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

/// @brief Returns the most recent connection error for a database.
/// @param connectionId The integer id of the connection to use.
///
/// Throws an error if the wrong number of parameters is given. Returns an error string if
/// an invalid connection id is given. Otherwise, the return value of this function depends on the current driver in the following manner.
/// @par MySQL
/// Returns error message for the most recent invoked API call that failed.
/// @par ODBC
/// Returns the first 512 characters of the error message for the most recent API call that failed.
/// @par Oracle
/// Returns the first 512 characters of the error message for the most recent API call that failed.
/// @par Postgresql
/// Returns the error message most recently generated by an operation on the connection.
/// @par SQLite
/// Returns error message for the last API call that failed.
void REVDB_ConnectionErr(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1) 
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);

		if (theconnection)
            // AL-2013-11-08 [[ Bug 11149 ]] Make sure most recent error string is available to revDatabaseConnectResult
			result = istrdup(theconnection->getErrorMessage(True));
		else
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else 
		result = istrdup(errors[REVDBERR_SYNTAX]);

	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

/// @brief Executes an SQL query
/// @param connectionId The integer connection id to use.
/// @param query The SQL query to execute
/// @return Either an error string or an integer representing the number of rows affected.
///
/// Throws an error if less than two parameters are given. Returns an error string if an invalid connection id is given.
/// If an error occurs in executing the query, the driver specific error messsage is returned (See REVDB_ConnectionErr).
/// Otherwise the number of affected rows is returned. This will be 0 for any query that is not SELECT, INSERT, UPDATE or DELETE.
void REVDB_Execute(char *p_arguments[], int p_argument_count, char **p_return_string, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count < 2) 
	{
		*p_return_string = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;
	int t_connection_id;
	t_connection_id = atoi(p_arguments[0]);

	DBConnection *t_connection;
	t_connection = (DBConnection *)connectionlist.find(t_connection_id);

	char *t_query;
	t_query = p_arguments[1];

	if (t_connection == NULL)
	{
		*p_return_string = istrdup(errors[REVDBERR_BADCONNECTION]);
		*p_error = True;
		return;
	}

	int t_values_count;
	t_values_count = 0;

	DBString *t_values;
	t_values = BindVariables(p_arguments, p_argument_count, t_values_count);

	unsigned int t_affected_rows;
	Bool t_result;
	t_result = t_connection -> sqlExecute(t_query, t_values, t_values_count, t_affected_rows);
	if (t_result)
	{
		char *t_return_string;
		t_return_string = (char *)malloc(INTSTRSIZE);
		sprintf(t_return_string, "%d", t_affected_rows);
		*p_return_string = t_return_string;
	}
	else 
		*p_return_string = istrdup(t_connection -> getErrorMessage());

	if (t_values) 
	{
		// OK-2008-12-09: BindVariables modified to duplicate value buffers, so they
		// must be freed here.
		for (int i = 0; i < t_values_count; i++)
			free((void *)t_values[i] . sptr);

		delete[] t_values;
	}
}


/// @brief Executes an sql query and returns a result set id
/// @param connectionId The integer connection id to use
/// @param query The SQL query to execute.
/// @param variablesList Either a list of variable names or an array name
///
/// @return An integer result set id.
void REVDB_Query(char *p_arguments[], int p_argument_count, char **p_return_string, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count < 2)
	{
		*p_return_string = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;
	int t_connection_id;
	t_connection_id = atoi(p_arguments[0]);

	DBConnection *t_connection;
	t_connection = (DBConnection *)connectionlist . find(t_connection_id);

	if (t_connection == NULL)
	{
		*p_return_string = istrdup(errors[REVDBERR_BADCONNECTION]);
		*p_error = True;
		return;
	}

	int t_values_count;
	t_values_count = 0;

	DBString *t_values;
	t_values = BindVariables(p_arguments, p_argument_count, t_values_count);

	char *t_query;
	t_query = p_arguments[1];

	DBCursor *t_cursor;
	t_cursor = t_connection -> sqlQuery(t_query, t_values, t_values_count, 0);

	char *t_result;
	if (t_cursor != NULL)
	{
		t_result = (char *)malloc(INTSTRSIZE);
		sprintf(t_result, "%d", t_cursor -> GetID());
	}
	else 
		t_result = istrdup(t_connection -> getErrorMessage());

	if (t_values)
	{
		// OK-2008-12-09: BindVariables modified to duplicate value buffers, so they
		// must be freed here.
		for (int i = 0; i < t_values_count; i++)
			free((void *)t_values[i] . sptr);

		delete[] t_values;
	}

	*p_return_string = (t_result != NULL ? t_result : (char *)calloc(1,1));
}

void REVDB_QueryList(char *p_arguments[], int p_argument_count, char **p_return_string, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count < 4)
	{
		*p_return_string = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;

	// Setup the column and row delimiters and work out their lengths
	const char *t_default_column_delimiter = "\t";
	const char *t_default_row_delimiter = "\n";

	const char *t_column_delimiter;
	t_column_delimiter = p_arguments[0];

	const char *t_row_delimiter;
	t_row_delimiter = p_arguments[1];

	if (t_column_delimiter[0] == '\0') 
		t_column_delimiter = t_default_column_delimiter;

	if (t_row_delimiter[0] == '\0')
		t_row_delimiter = t_default_row_delimiter;

	unsigned int t_column_delimiter_length;
	t_column_delimiter_length = strlen(t_column_delimiter);

	unsigned int t_row_delimiter_length;
	t_row_delimiter_length = strlen(t_row_delimiter);

	// Find the connection object
	int t_connection_id;
	t_connection_id = atoi(p_arguments[2]);

	DBConnection *t_connection;
	t_connection = (DBConnection *)connectionlist . find(t_connection_id);
	if (!t_connection)
	{
		*p_return_string = istrdup(errors[REVDBERR_BADCONNECTION]);
		*p_error = True;
		return;
	}

	// Bind the variables and execute the query
	int t_values_count;
	t_values_count = 0;

	DBString *t_values;
	t_values = BindVariables(&p_arguments[2], p_argument_count - 2, t_values_count);

	char *t_query;
	t_query = p_arguments[3];
	DBCursor *t_cursor;
	t_cursor = t_connection -> sqlQuery(t_query, t_values, t_values_count, 0);

	if (t_cursor == NULL)
	{
		char *t_result;
		t_result = (char *)malloc(266);
		t_result[0] = '\0';
		strcat(t_result, "revdberr,");
		strncat(t_result, t_connection -> getErrorMessage(), 255);
		*p_return_string = t_result;
		return;
	}

	// Build up the return data
	int t_field_count;
	t_field_count = t_cursor -> getFieldCount();

	large_buffer_t t_result;

	if (!t_cursor -> getEOF())
	{
		while (True)
		{
			for (int i = 1; i <= t_field_count; i++)
			{
				unsigned int t_column_size;
				char *t_column_data;
				t_column_data = t_cursor -> getFieldDataBinary(i, t_column_size);

				if (t_cursor -> getFieldType(i) != FT_WSTRING)
					t_result . append(t_column_data, t_column_size);
				else
				{
					char *t_converted_string;
					t_converted_string = string_from_utf16((unsigned short *)t_column_data, t_column_size / 2);
					t_result . append(t_converted_string, t_column_size / 2);
					free(t_converted_string);
				}

				if (i != t_field_count)
					t_result . append(t_column_delimiter, t_column_delimiter_length);

			}
			t_cursor -> next();
			if (t_cursor -> getEOF())
				break;

			t_result . append(t_row_delimiter, t_row_delimiter_length);
		}
	}

	t_cursor -> getConnection() -> deleteCursor(t_cursor -> GetID());
	
	if (t_values)
	{
		// OK-2008-12-09: BindVariables modified to duplicate value buffers, so they
		// must be freed here.
		for(int i = 0; i < t_values_count; i++)
			free((void *)t_values[i] . sptr);

		delete[] t_values;
	}

	t_result . append('\0');

	void *t_data;
	unsigned int t_data_length;
	t_result . grab(t_data, t_data_length);

	// Make sure we null terminate the buffer at the last byte - we need to do this
	// in case the memory allocation in t_result fails (and so subsequent appends
	// fail).
	((char *)t_data)[t_data_length - 1] = '\0';

	*p_return_string = (char *)t_data;
}

//revdb_closecursor(cursorid) - close database cursor
void REVDB_CloseCursor(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
			thecursor->getConnection()->deleteCursor(cursorid);
		else
		{
			*error = True;
			result = istrdup(errors[REVDBERR_BADCURSOR]);
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_movenext(cursorid) - move to next row in resultset, return true if eof
void REVDB_MoveNext(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1) {
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			Bool eof = thecursor->next();
			result = istrdup(BooltoStr(eof));
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_moveprev(cursorid) - move to previous row in resultset, return true if bof
void REVDB_MovePrev(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{

	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			Bool eof = thecursor->prev();
			result = istrdup(BooltoStr(eof));
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


//revdb_movefirst(cursorid) - move to first row in resultset, return false if no records
void REVDB_MoveFirst(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			Bool eof = thecursor->first();
			result = istrdup(BooltoStr(eof));
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


//revdb_movelast(cursorid) - move to last row in resultset, return false if no records
void REVDB_MoveLast(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			Bool eof = thecursor->last();
			result = istrdup(BooltoStr(eof));
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

bool emulate_move_to(DBCursor *p_cursor, int p_record_index)
{
    // This is the default implementation of move, which will be called if the driver doesn't support it.
	// Here we attempt to emulate random cursor access, although in some cases this won't be possible.
	if (p_record_index < 0)
		return False;
	else if (p_record_index > p_cursor -> getRecordCount() - 1)
		return False;

	// Calculate the difference between the current record number and where we want to move to
	// This will be positive if we are moving forward and negative if moving backwards.
	int t_gap;
	t_gap = p_record_index - p_cursor -> getRecordNumber();

	if (t_gap == 0)
		return True;

	// The absolute value of the difference gives us the number of moves needed to reach the required record.
	for (int i = 0; i < abs(t_gap); i++)
	{
		Bool t_result;
		if (t_gap > 0)
			t_result = p_cursor -> next();
		else
			t_result = p_cursor -> prev();

		if (!t_result)
			return False;
	}

	return True;
}


void REVDB_MoveTo(char *argv[], int argc, char **r_result, Bool* r_pass, Bool* r_error)
{
	*r_error = True;
	*r_result = NULL;
	*r_pass = False;

	if (argc != 2)
	{
		*r_result = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}
	
	int t_cursor_id;
	t_cursor_id = atoi(argv[0]);
	
	int t_record_index;
	t_record_index = atoi(argv[1]);
	
	DBCursor *t_cursor;
	t_cursor = findcursor(t_cursor_id);
	if (t_cursor == NULL)
	{
		*r_result = istrdup(errors[REVDBERR_BADCURSOR]);
		return;
	}
	
	DBCursor2 *t_cursor_2;
	CDBConnection *t_connection;
	t_connection = (CDBConnection * )t_cursor -> getConnection();

	if (!t_connection -> isLegacy())
		t_cursor_2 = static_cast<DBCursor2 *>(t_cursor);
	else
		t_cursor_2 = NULL;
	
	if (t_cursor_2 == NULL || t_cursor_2 -> getVersion() < 3)
	{
		// Throw a not supported error (or emulate if possible)
		//...

		bool t_emulation_success;
		t_emulation_success = emulate_move_to(t_cursor, t_record_index);

		if (t_emulation_success)
		{
			*r_result = istrdup("True");
			*r_error = False;
			return;
		}
		else
		{
			*r_result = istrdup(errors[REVDBERR_NOT_SUPPORTED]);
			return;
		}
	}
	
	DBCursor3 *t_cursor_3;
	t_cursor_3 = static_cast<DBCursor3 *>(t_cursor_2);
	
	bool t_success;
	t_success = t_cursor_3 -> move(t_record_index) != 0;
	
	if (!t_success)
	{
		*r_result = istrdup(BooltoStr(t_success));

		// Failing to navigate is not a user error, to be consistent with other
		// cursor navigation commands, we report the failure using the result instead.
		*r_error = False;
		return;
	}
	
	// Everything went as expected :o)
	*r_result = strdup("True");
	*r_error = False;
}


/// @brief Whether the cursor is at end of file.
/// @param pRecordSetId An integer record set id.
/// @return True if at EOF, false otherwise.
///
/// Returns true if the cursor is *after* the last record.
void REVDB_IsEOF(char *p_arguments[], int p_argument_count, char **retstring, Bool *pass, Bool *error)
{
	*error = True;
	*pass = False;

	if (p_argument_count != 1)
	{
		*retstring = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*error = False;

	int cursorid;
	cursorid = atoi(*p_arguments);

	DBCursor *t_cursor = findcursor(cursorid);
	if (t_cursor == NULL)
	{
		*retstring = istrdup(errors[REVDBERR_BADCURSOR]);
		*error = True;
		return;
	}
		
	char *t_result;
	t_result = NULL;

	Bool t_eof;
	t_eof = t_cursor -> getEOF();

	if (!REVDBinited && t_cursor -> getConnection() -> getConnectionType() == CT_ORACLE && t_cursor -> getRecordNumber() > 20)
		t_result = istrdup(BooltoStr(True));
	else
		t_result = istrdup(BooltoStr(t_eof));
	
	
	*retstring = (t_result != NULL ? t_result : (char *)calloc(1,1));
}

/// @brief Whether the cursor is pointing to the first record
/// @param pRecordSetId An integer record set id
/// @return true if the cursor is on the first record
void REVDB_IsBOF(char *p_arguments[], int p_argument_count, char **retstring, Bool *pass, Bool *error)
{
	*error = True;
	*pass = False;

	if (p_argument_count != 1)
	{
		*retstring = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*error = False;

	int cursorid;
	cursorid = atoi(*p_arguments);

	DBCursor *t_cursor;
	t_cursor = findcursor(cursorid);

	char *t_result;
	t_result = NULL;
	if (t_cursor)
		t_result = istrdup(BooltoStr(t_cursor -> getBOF()));
	else
	{
		t_result = istrdup(errors[REVDBERR_BADCURSOR]);
		*error = True;
	}
	
	*retstring = (t_result != NULL ? t_result : (char *)calloc(1,1));
}

/// @brief Whether the cursor is at the start of the record set.
/// @param pRecordSetId An integer record set id
/// @return true if the cursor is at the start of the record set, false otherwise
/// The cursor is at the start of the record set if prev() was called when the cursor
/// was pointing to the first record. 
void REVDB_CurrentRecordIsFirst(char *p_arguments[], int p_argument_count, char **p_result, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count != 1)
	{
		*p_result = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;

	int t_cursor_id;
	t_cursor_id = atoi(*p_arguments);

	DBCursor *t_cursor;
	t_cursor = findcursor(t_cursor_id);

	if (t_cursor == NULL)
	{
		*p_result = istrdup(errors[REVDBERR_BADCURSOR]);
		*p_error = True;
	}
	else
	{
		Bool t_result;
		t_result = (t_cursor -> getRecordNumber() == 0);
		*p_result = istrdup(BooltoStr(t_result));
	}
}

/// @brief Whether the cursor is pointing at the last record.
/// @param pRecordSetId An integer record set id.
/// @return true if the cursor is pointing at the last record, false otherwise.
void REVDB_CurrentRecordIsLast(char *p_arguments[], int p_argument_count, char **p_result, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count != 1)
	{
		*p_result = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;

	int t_cursor_id;
	t_cursor_id = atoi(*p_arguments);

	DBCursor *t_cursor;
	t_cursor = findcursor(t_cursor_id);

	if (t_cursor == NULL)
	{
		*p_result = istrdup(errors[REVDBERR_BADCURSOR]);
		*p_error = True;
	}
	else
	{
		Bool t_result;
		t_result = (t_cursor -> getRecordNumber() == t_cursor -> getRecordCount() - 1);
		*p_result = istrdup(BooltoStr(t_result));
	}
}

//revdb_recordcount(cursorid) - return number of rows in resultset
void REVDB_RecordCount(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			unsigned int recordcount = thecursor->getRecordCount();
			result = (char *)malloc(INTSTRSIZE);
			sprintf(result,"%d",recordcount);
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_currentrecord(cursorid) - return current record
void REVDB_CurrentRecord(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			unsigned int currecord = thecursor->getRecordNumber();
			result = (char *)malloc(INTSTRSIZE);
			sprintf(result,"%d",currecord);
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_columncount(cursorid) - return number of records
void REVDB_ColumnCount(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			unsigned int columncount = thecursor->getFieldCount();
			result = (char *)malloc(INTSTRSIZE);
			sprintf(result,"%d",columncount);
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


//revdb_cursorerr(cursorid) - returns cursor error message
void REVDB_CursorErr(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
			result = istrdup(thecursor->getErrorMessage());
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

/// @brief Lists the number of columns in a table or recordset
/// @param pCursorOrConnectionId This is either interpreted as a cursor id or a connection id, depending on whether a second parameter was given
/// @param pTableName If this parameter is given, then the first parameter is interpreted as a connection id and the columns in the table are returned
void REVDB_ColumnNames(char *p_arguments[], int p_argument_count, char **p_return_string, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count < 1 || p_argument_count > 2)
	{
		*p_return_string = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;

	int t_id;
	t_id = atoi(p_arguments[0]);

	// MW-2008-08-18: [[ Bug 6928 ]] Make sure we record whether we need to free the cursor
	//   when we've finished with it. This is necessary if the 'table' parameter is passed.
	bool t_free_cursor;
	t_free_cursor = false;

	DBCursor *t_cursor;
	t_cursor = NULL;

	if (p_argument_count == 2)
	{
		DBConnection *t_connection;
		t_connection = (DBConnection *)connectionlist . find(t_id);
		if (t_connection == NULL)
		{
			*p_return_string = istrdup(errors[REVDBERR_BADCONNECTION]);
			*p_error = True;
			return;
		}
		
		char *t_table_name;
		t_table_name = p_arguments[1];

		char *t_query;
		t_query = (char *)malloc(sizeof(char) * (15 + strlen(t_table_name)));
		sprintf(t_query, "SELECT * FROM %s", t_table_name);

		t_cursor = t_connection -> sqlQuery(t_query, NULL, 0, 0);
		free(t_query);

		if (t_cursor == NULL)
		{
			*p_return_string = istrdup(errors[REVDBERR_BADTABLE]);
			return;
		}
		
		t_free_cursor = true;
	}
	else
	{
		t_cursor = findcursor(t_id);
		if (t_cursor == NULL)
		{
			*p_error = True;
			*p_return_string = istrdup(errors[REVDBERR_BADCURSOR]);
			return;
		}
	}

	char *t_result;
	t_result = NULL;

	int t_field_count;
	t_field_count = t_cursor -> getFieldCount(); 
	if (t_field_count != 0)
	{
		t_result = (char *)malloc(t_field_count * F_NAMESIZE);
		t_result[0] = '\0';
		for (int i = 1; i <= t_field_count; i++)
		{
			strcat(t_result, t_cursor -> getFieldName(i));
			if (i != t_field_count) strcat(t_result,",");
		}
	}
	
	// MW-2008-08-28: [[ Bug 7044 ]] Make sure we use the correct destruction method.
	//   When a cursor is created it automagically gets added to the connection's
	//   cursor list, and thus we must delete it using a connection method rather than
	//   just 'delete'.
	if (t_free_cursor)
		t_cursor -> getConnection() -> deleteCursor(t_cursor -> GetID());
	
	*p_return_string = (t_result != NULL ? t_result : (char *)calloc(1,1));
}


//revdb_columntypes(cursorid) - returns comma delimited list of data type for each column
void REVDB_ColumnTypes(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			int fcount = thecursor->getFieldCount(); 
			if (fcount)
			{
				result = (char *)malloc(fcount * 16);
				result[0] = '\0';
				for (int i = 1; i <= fcount; i++){
					strcat(result,dbtypestrings[thecursor->getFieldType(i)]);
					if (i != fcount) strcat(result,",");
				}
			}
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_columnlengths(cursorid) - returns comma delimited list of column sizes
void REVDB_ColumnLengths(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			int fcount = thecursor->getFieldCount(); 
			if (fcount)
			{
				result = (char *)malloc(fcount * INTSTRSIZE);
				result[0] = '\0';
				for (int i = 1; i <= fcount; i++){
					char idbuffer[INTSTRSIZE];
					sprintf(idbuffer,"%d",thecursor->getFieldLength(i));
					strcat(result,idbuffer);
					if (i != fcount) strcat(result,",");
				}
			}
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCURSOR]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_cursorconnection(cursorid) - return associated connection with cursor
void REVDB_CursorConnection(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int cursorid = atoi(*args);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			unsigned int connid = thecursor->getConnection()->GetID();
			result = (char *)malloc(INTSTRSIZE);
			sprintf(result,"%d",connid);
		}
		else
		{
			*error = True;
			result = istrdup(errors[REVDBERR_BADCURSOR]);
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


//revdb_columnbynum(cursorid,columnnumber,[variable]) 
//- returns data in column, set variable to binary data if passed
void REVDB_ColumnByNumber(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	*error = True;
	*pass = False;

	if (nargs < 2)
	{
		*retstring = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*error = False;

	int t_cursor_id;
	t_cursor_id = atoi(args[0]);

	DBCursor *t_cursor;
	t_cursor = findcursor(t_cursor_id);

	if (t_cursor == NULL)
	{
		*error = True;
		*retstring = istrdup(errors[REVDBERR_BADCURSOR]);
		return;
	}
		
	int t_column_id;
	t_column_id = atoi(args[1]);

	if (t_column_id < 1 || t_column_id > (t_cursor -> getFieldCount()))
	{
		*retstring = istrdup(errors[REVDBERR_BADCOLUMNNUM]);
		return;
	}

	char *t_result;
	t_result = NULL;
	GetColumnByNumber(t_cursor, t_result, t_column_id, nargs == 3? args[2]: NULL);
	*retstring = (t_result != NULL ? t_result : (char *)calloc(1,1));
}


/// @brief Returns data from a column.
/// @param pCursorId The integer id of the cursor to retrieve the data from
/// @param pColumnName The name of the column to retrieve the data from
/// @param pVariableName (optional) The data will be placed into this variable if given
/// 
void REVDB_ColumnByName(char *p_arguments[], int p_argument_count, char **p_return_string, Bool *p_pass, Bool *p_error)
{
	*p_error = True;
	*p_pass = False;

	if (p_argument_count < 2) 
	{
		*p_return_string = istrdup(errors[REVDBERR_SYNTAX]);
		return;
	}

	*p_error = False;

	int t_cursor_id;
	t_cursor_id = atoi(p_arguments[0]);

	DBCursor *t_cursor;
	t_cursor = findcursor(t_cursor_id);

	if (t_cursor == NULL)
	{
		*p_error = True;
		*p_return_string = istrdup(errors[REVDBERR_BADCURSOR]);
		return;
	}

	int t_column_id;
	t_column_id = findcolumn(t_cursor, p_arguments[1]);
	if (t_column_id == 0)
	{
		*p_return_string = istrdup(errors[REVDBERR_BADCOLUMNNAME]);
		return;
	}

	char *t_result;
	t_result = NULL;
	GetColumnByNumber(t_cursor, t_result, t_column_id, p_argument_count == 3 ? p_arguments[2]: NULL);

	*p_return_string = (t_result != NULL ? t_result : (char *)calloc(1,1));
}

//revdb_connections - returns item delimited list of connections
void REVDB_Connections(char *args[], int nargs, char **retstring,
					   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	DBObjectList *connlist = connectionlist.getList();
	DBObjectList::iterator theIterator;
	if (!connlist->empty())
	{
		result = (char *)malloc(connlist->size() * INTSTRSIZE);
		result[0] = '\0';
		DBObjectList::size_type numconnections = 0;
		for (theIterator = connlist->begin(); theIterator != connlist->end(); theIterator++)
		{
			DBConnection *curconnection = (DBConnection *)(*theIterator);
			char idbuffer[INTSTRSIZE];
			sprintf(idbuffer,"%d",curconnection->GetID());
			strcat(result,idbuffer);
			if (++numconnections != connlist->size())
				strcat(result,",");
		}
	}
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

//revdb_connections(cursorid) - returns item delimited list of cursors per connection
void REVDB_Cursors(char *args[], int nargs, char **retstring,
				   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);
		if (theconnection)
		{
			int numcursors = theconnection->countCursors();
			if (numcursors)
			{
				result = (char *)malloc(numcursors * INTSTRSIZE);
				result[0] = '\0';
				int i;
				for (i = 0; i < numcursors; i++)
				{
					DBCursor *curcursor = theconnection->findCursorIndex(i);
					char idbuffer[INTSTRSIZE];
					sprintf(idbuffer,"%d",curcursor->GetID());
					strcat(result,idbuffer);

					// OK-2010-02-22: [[Bug 7666]] - Don't add trailing comma
					if (i != numcursors - 1)
						strcat(result,",");
				}
			}
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


//revdb_connections(cursorid) - returns item delimited list of cursors per connection
void REVDB_DBType(char *args[], int nargs, char **retstring,
				   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1)
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);
		if (theconnection) 
			result = istrdup(theconnection->getconnectionstring());
		else
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

void REVDB_TableNames(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs == 1) 
	{
		*error = False;
		int connectionid = atoi(*args);
		DBConnection *theconnection = (DBConnection *)connectionlist.find(connectionid);
		if (theconnection)
		{
			int bufsize;
			theconnection->getTables( NULL, &bufsize);
			result = (char *)malloc(bufsize);
			result[0] = 0;
			theconnection->getTables(result, &bufsize);
		}
		else
		{
			result = istrdup(errors[REVDBERR_BADCONNECTION]);
			*error = True;
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


void REVDB_ValentinaCursorRef(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{

}


void REVDB_ValentinaConnectionRef(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{

}

//- returns data in column, set variable to binary data if passed
void REVDB_ColumnIsNull(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = True;
	*pass = False;
	if (nargs >= 2)
	{
		*error = False;
		int cursorid = atoi(args[0]);
		int columnid = atoi(args[1]);
		DBCursor *thecursor = findcursor(cursorid);
		if (thecursor)
		{
			Bool isnull = thecursor->getFieldIsNull(columnid);
			result = istrdup(BooltoStr(isnull));
		}
		else
		{
			*error = True;
			result = istrdup(errors[REVDBERR_BADCURSOR]);
		}
	}
	else result = istrdup(errors[REVDBERR_SYNTAX]);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}


void REVDB_ValentinaDBRefToConnection(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	
}


void REVDB_GetValentinaDBRef(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{

}

void REVDB_Valentina(char *args[], int nargs, char **retstring, Bool *pass, Bool *error) 
{

}

extern "C"
{
#ifdef WIN32
    void __declspec(dllexport) shutdownXtable(void);
#else
    void shutdownXtable(void) __attribute__((visibility("default")));
#endif
}

void MCCefFinalise(void);
void shutdownXtable(void)
{
    REVDB_QUIT();
}

EXTERNAL_BEGIN_DECLARATIONS("revDB")
	EXTERNAL_DECLARE_FUNCTION("revdb_init", REVDB_Init)
	EXTERNAL_DECLARE_FUNCTION("revdb_connect", REVDB_Connect)
	EXTERNAL_DECLARE_FUNCTION("revdb_disconnect", REVDB_Disconnect)
	EXTERNAL_DECLARE_FUNCTION("revdb_connectionerr", REVDB_ConnectionErr)
	EXTERNAL_DECLARE_FUNCTION("revdb_commit", REVDB_Commit)
	EXTERNAL_DECLARE_FUNCTION("revdb_rollback", REVDB_Rollback)
	EXTERNAL_DECLARE_FUNCTION("revdb_execute", REVDB_Execute)
	EXTERNAL_DECLARE_FUNCTION("revdb_query", REVDB_Query)
	EXTERNAL_DECLARE_FUNCTION("revdb_queryblob", REVDB_Query)
	EXTERNAL_DECLARE_FUNCTION("revdb_closecursor", REVDB_CloseCursor)
	EXTERNAL_DECLARE_FUNCTION("revdb_movenext", REVDB_MoveNext)
	EXTERNAL_DECLARE_FUNCTION("revdb_moveprev", REVDB_MovePrev)
	EXTERNAL_DECLARE_FUNCTION("revdb_movefirst", REVDB_MoveFirst)
	EXTERNAL_DECLARE_FUNCTION("revdb_movelast", REVDB_MoveLast)
	EXTERNAL_DECLARE_FUNCTION("revdb_iseof", REVDB_IsEOF)
	EXTERNAL_DECLARE_FUNCTION("revdb_isbof", REVDB_IsBOF)
	EXTERNAL_DECLARE_FUNCTION("revdb_recordcount", REVDB_RecordCount)
	EXTERNAL_DECLARE_FUNCTION("revdb_currentrecord", REVDB_CurrentRecord)
	EXTERNAL_DECLARE_FUNCTION("revdb_columncount", REVDB_ColumnCount)
	EXTERNAL_DECLARE_FUNCTION("revdb_cursorerr", REVDB_CursorErr)
	EXTERNAL_DECLARE_FUNCTION("revdb_columnnames", REVDB_ColumnNames)
	EXTERNAL_DECLARE_FUNCTION("revdb_columnbynumber", REVDB_ColumnByNumber)
	EXTERNAL_DECLARE_FUNCTION("revdb_columnlengths", REVDB_ColumnLengths)
	EXTERNAL_DECLARE_FUNCTION("revdb_cursorconnection", REVDB_CursorConnection)
	EXTERNAL_DECLARE_FUNCTION("revdb_columnbyname", REVDB_ColumnByName)
	EXTERNAL_DECLARE_FUNCTION("revdb_connections", REVDB_Connections)
	EXTERNAL_DECLARE_FUNCTION("revdb_cursors", REVDB_Cursors)
	EXTERNAL_DECLARE_FUNCTION("revdb_dbtype", REVDB_DBType)
	EXTERNAL_DECLARE_FUNCTION("revdb_columntypes", REVDB_ColumnTypes)
	EXTERNAL_DECLARE_FUNCTION("revdb_columnisnull", REVDB_ColumnIsNull)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentinadbref", REVDB_ValentinaConnectionRef)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentinacursorref", REVDB_ValentinaCursorRef)
	EXTERNAL_DECLARE_FUNCTION("revdb_querylist", REVDB_QueryList)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentinadbreftoconnection", REVDB_ValentinaDBRefToConnection)
	EXTERNAL_DECLARE_FUNCTION("revdb_getvalentinadbref", REVDB_GetValentinaDBRef)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentina", REVDB_Valentina)
	EXTERNAL_DECLARE_COMMAND("revdb_setdriverpath", REVDB_SetDriverPath)
	EXTERNAL_DECLARE_FUNCTION("revdb_tablenames", REVDB_TableNames)
	EXTERNAL_DECLARE_FUNCTION("revdb_version", REVDB_Version)

	EXTERNAL_DECLARE_FUNCTION("revOpenDatabase", REVDB_Connect)
	EXTERNAL_DECLARE_COMMAND("revCloseDatabase", REVDB_Disconnect)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseConnectResult", REVDB_ConnectionErr)
	EXTERNAL_DECLARE_COMMAND("revCommitDatabase", REVDB_Commit)
	EXTERNAL_DECLARE_COMMAND("revRollBackDatabase", REVDB_Rollback)
	EXTERNAL_DECLARE_COMMAND("revExecuteSQL", REVDB_Execute)
	EXTERNAL_DECLARE_FUNCTION("revQueryDatabase", REVDB_Query)
	EXTERNAL_DECLARE_FUNCTION("revQueryDatabaseBLOB", REVDB_Query)
	EXTERNAL_DECLARE_COMMAND("revCloseCursor", REVDB_CloseCursor)
	EXTERNAL_DECLARE_COMMAND("revMoveToNextRecord", REVDB_MoveNext)
	EXTERNAL_DECLARE_COMMAND("revMoveToPreviousRecord", REVDB_MovePrev)
	EXTERNAL_DECLARE_COMMAND("revMoveToFirstRecord", REVDB_MoveFirst)
	EXTERNAL_DECLARE_COMMAND("revMoveToLastRecord", REVDB_MoveLast)
	EXTERNAL_DECLARE_COMMAND("revMoveToRecord", REVDB_MoveTo)
	EXTERNAL_DECLARE_FUNCTION("revCurrentRecordIsFirst", REVDB_CurrentRecordIsFirst)
	EXTERNAL_DECLARE_FUNCTION("revCurrentRecordIsLast", REVDB_CurrentRecordIsLast)
	EXTERNAL_DECLARE_FUNCTION("revQueryIsAtEnd", REVDB_IsEOF)
	EXTERNAL_DECLARE_FUNCTION("revQueryIsAtStart", REVDB_IsBOF)
	EXTERNAL_DECLARE_FUNCTION("revNumberOfRecords", REVDB_RecordCount)
	EXTERNAL_DECLARE_FUNCTION("revCurrentRecord", REVDB_CurrentRecord)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnCount", REVDB_ColumnCount)
	EXTERNAL_DECLARE_FUNCTION("revQueryResult", REVDB_CursorErr)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnNames", REVDB_ColumnNames)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseTableNames", REVDB_TableNames)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnNumbered", REVDB_ColumnByNumber)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnLengths", REVDB_ColumnLengths)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseID", REVDB_CursorConnection)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnNamed", REVDB_ColumnByName)
	EXTERNAL_DECLARE_FUNCTION("revOpenDatabases", REVDB_Connections)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseCursors", REVDB_Cursors)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseType", REVDB_DBType)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnTypes", REVDB_ColumnTypes)
	EXTERNAL_DECLARE_FUNCTION("revDatabaseColumnIsNull", REVDB_ColumnIsNull)
	EXTERNAL_DECLARE_COMMAND("revSetDatabaseDriverPath", REVDB_SetDriverPath)
	EXTERNAL_DECLARE_FUNCTION("revGetDatabaseDriverPath", REVDB_GetDriverPath)

	EXTERNAL_DECLARE_FUNCTION("revdb_valentinadbref", REVDB_ValentinaConnectionRef)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentinacursorref", REVDB_ValentinaCursorRef)
	EXTERNAL_DECLARE_FUNCTION("revDataFromQuery", REVDB_QueryList)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentinadbreftoconnection", REVDB_ValentinaDBRefToConnection)
	EXTERNAL_DECLARE_FUNCTION("revdb_getvalentinadbref", REVDB_GetValentinaDBRef)
	EXTERNAL_DECLARE_FUNCTION("revdb_valentina", REVDB_Valentina)
EXTERNAL_END_DECLARATIONS

#ifdef TARGET_SUBPLATFORM_IPHONE
extern "C"
{
	extern struct LibInfo __libinfo;
	__attribute((section("__DATA,__libs"))) volatile struct LibInfo *__libinfoptr_revdb __attribute__((__visibility__("default"))) = &__libinfo;
}
#endif
