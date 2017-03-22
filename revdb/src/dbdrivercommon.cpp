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

#include "dbdrivercommon.h"

#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
#define LIBRARY_EXPORT __declspec(dllexport)
#else
#define LIBRARY_EXPORT __attribute__((__visibility__("default")))
#endif

// Default implementations for DBField
DBField::DBField()
    : maxlength(),
      data()
{
	freeBuffer = False;
	isAutoIncrement = False;
	isPrimaryKey = False;
	isUnique = False;
	isNotNull = False;
	isNull = False;

	fieldNum = 0;
	dataSize = 0;
	fieldType = FT_NULL; strcpy(fieldName,"");
}

DBField::~DBField()
{
	if (freeBuffer)
		delete data;

	data = NULL;
}

// Default Implementations for CDBConnection
CDBConnection::CDBConnection()
{
	isConnected = False;
	connectionType = CT_NULL;
	m_error = NULL;
}

CDBConnection::~CDBConnection()
{
	errorMessageSet(NULL);
}

DBList *CDBConnection::getCursorList()
{
	return &cursorlist;
}

int CDBConnection::getConnectionType()
{
	return abs(connectionType);
}

Bool CDBConnection::getIsConnected()
{
	return isConnected;
}

void CDBConnection::deleteCursor(int p_id)
{
	cursorlist . erase(p_id);
}

DBCursor *CDBConnection::findCursor(int p_id)
{
	return (DBCursor *)cursorlist . find(p_id);
}

int CDBConnection::countCursors()
{
	return cursorlist . getsize();
}

DBCursor *CDBConnection::findCursorIndex(int p_id)
{
	return (DBCursor *)cursorlist . findIndex(p_id);
}

void CDBConnection::addCursor(DBCursor *p_new_cursor)
{
	cursorlist . add((DBObject *)p_new_cursor);
}

void CDBConnection::closeCursors()
{
	cursorlist . clear();
}

bool CDBConnection::isLegacy(void) 
{ 
	return getConnectionType() > 0; 
}

// p_input - input query
// p_output - output buffer (allocated by caller)
// p_callback - place-holder processing function (provided by caller)
// p_callback_context - state pointer for callback
bool CDBConnection::processQuery(const char *p_input, DBBuffer& p_output, ProcessQueryCallback p_callback, void *p_callback_context)
{
	const char *t_token;
	t_token = NULL;

	const char *t_input = p_input;

	const char *t_input_end;
	t_input_end = t_input + strlen(p_input) + 1;

	// Ensure that there is at least as much room as the query in our buffer.
	if (!p_output . ensure(t_input_end - t_input))
		return false;

	char *t_output_base;
	t_output_base = p_output . getFrontier();

	char *t_output;
	t_output = t_output_base;

	int t_state;
	t_state = STATE_NONE;

	char t_quoted_string_state_char;

	do
	{
		switch(t_state)
		{
		case STATE_NONE:
			switch(*t_input)
			{
			case '"':
			case '\'':
			case '`':
				t_state = STATE_QUOTED_STRING;
				t_quoted_string_state_char = *t_input;
				*t_output++ = *t_input;
			break;

			case ':':
				t_state = STATE_MARKER;
			break;
			
			default:
				*t_output++ = *t_input;
			break;
			}
		break;

		case STATE_MARKER:
			switch(*t_input)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				t_state = STATE_INTEGER;
				t_token = t_input - 1;
			break;

			default:
				t_state = STATE_NONE;
				*t_output++ = ':';
				*t_output++ = *t_input;
			break;
			}
		break;

		case STATE_QUOTED_STRING:
			if (*t_input == t_quoted_string_state_char)
				t_state = STATE_NONE;

			*t_output++ = *t_input;
		break;

		case STATE_QUOTED_STRING_ESCAPE:
			t_state = STATE_QUOTED_STRING;
			*t_output++ = *t_input;
		break;


		case STATE_INTEGER:
			switch(*t_input)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			break;

			default:
				int t_parameter_number;
				t_parameter_number = strtol((t_token + 1), NULL, 10);

				if (t_parameter_number != 0)
				{
					// Update the frontier of the buffer
					p_output . advance(t_output - t_output_base);
					p_callback(p_callback_context, t_parameter_number, p_output);

					// Resynchronize for our processing
					if (!p_output . ensure(t_input_end - t_input))
						return false;

					t_output = t_output_base = p_output . getFrontier();
				}

				t_state = STATE_NONE;
				*t_output++ = *t_input;
			break;
			}
		break;
		}
	}
	while(*t_input++ != '\0');
	
	// Update the frontier of the buffer
	p_output . advance(t_output - t_output_base);

	return true;
}

void CDBConnection::errorMessageSet(const char *p_message)
{
	if (m_error != NULL)
		free(m_error);

	if (p_message == NULL)
		m_error = NULL;
	else
		m_error = strdup(p_message);
}


// Default implementations for CDBCursor

CDBCursor::CDBCursor()
{
	isBOF = False;
	isEOF = True;
	recordNum = 0;
	recordCount = 0;
	fieldCount = 0;
	fields = NULL;
	connection = NULL;
	isTransaction = False;
	cacherows = False;
}

Bool CDBCursor::getIsOpen()
{
	return fieldCount != 0;
}

int CDBCursor::getFieldCount()
{
	return fieldCount;
}

int CDBCursor::getRecordCount()
{
	return recordCount;
}

int CDBCursor::getRecordNumber()
{
	return recordNum;
}

Bool CDBCursor::getEOF()
{
	return isEOF;
}

Bool CDBCursor::getBOF()
{
	return isBOF;
}

DBConnection *CDBCursor::getConnection()
{
	return connection;
}


char *CDBCursor::getErrorMessage() 
{
	return connection -> getErrorMessage();
}

Bool CDBCursor::IsError()
{
	return connection -> IsError();
}

Bool CDBCursor::move(int p_record_index)
{
	// This is the default implementation of move, which will be called if the driver doesn't support it.
	// Here we attempt to emulate random cursor access, although in some cases this won't be possible.
	if (p_record_index < 0)
		return False;
	else if (p_record_index > recordCount - 1)
		return False;

	// Calculate the difference between the current record number and where we want to move to
	// This will be positive if we are moving forward and negative if moving backwards.
	int t_gap;
	t_gap = p_record_index - recordNum;

	if (t_gap == 0)
		return True;

	// The absolute value of the difference gives us the number of moves needed to reach the required record.
	for (int i = 0; i < abs(t_gap); i++)
	{
		Bool t_result;
		if (t_gap > 0)
			t_result = next();
		else
			t_result = prev();

		if (!t_result)
			return False;
	}

	// If the assumption holds that either all the calls will suceed or the first will fail, then
	// the cursor should be in a consistent state, so we're ok to return. However we should set
	// isBOF and isEOF to false first.
	isEOF = False;
	isBOF = False;

	return True;
}


unsigned int CDBCursor::getFieldLength(unsigned int fieldnum)
{
	fieldnum--;
	if ((int)fieldnum >= fieldCount)
		return 0;

	return (unsigned int)fields[fieldnum] -> maxlength;
}
/*getFieldName - get name of a field by field number (1 based)
Input: fieldnum - number of field to get name of
Output: name of field, NULL on error*/ 
char *CDBCursor::getFieldName(unsigned int fieldnum)
{
	fieldnum--;
	//check that field is in range
	if ((int)fieldnum >= fieldCount) 
		return NULL;

	return fields[fieldnum] -> fieldName;
}

DBFieldType CDBCursor::getFieldType(unsigned int fieldnum)
{
	fieldnum--;
	if ((int)fieldnum >= fieldCount) 
		return FT_NULL;

	return fields[fieldnum] -> fieldType;
}

/*getFieldDataBinary - get data of a column
Input: 
fieldnum - number of field to get data from
fdlength - will recieve length of data
Output: pointer to fields data, NULL on error*/ 
char *CDBCursor::getFieldDataBinary(unsigned int fieldnum,unsigned int &fdlength)
{
	fieldnum--;
	if ((int)fieldnum >= fieldCount)
		return NULL;

	// MW-2010-10-01: [[ Bug 8980 ]] If the field is NULL, always return 0 length ""
	if (fields[fieldnum] -> isNull)
	{
		fdlength = 0;
		return (char *)DBNullValue;
	}

	fdlength = fields[fieldnum] -> dataSize;

	if (fdlength == 0)
		return (char *)DBNullValue;
	
	return fields[fieldnum] -> data;
}

/*getFieldDataString - Return the stored data as a string.
Input: 
fieldnum - number of field to get name of
Output: copy of fields data as null terminated string, 
NULL on error*/ 
char *CDBCursor::getFieldDataString(unsigned int fieldnum)
{
	unsigned int fdlength = 0;
	char *fdata = getFieldDataBinary(fieldnum, fdlength);
	if (fdata == NULL)
		return NULL;

	if (fdata != DBNullValue)
		fdata[fdlength] = '\0'; 

	return fdata;
}

Bool CDBCursor::getFieldIsNull(unsigned int fieldnum)
{
	// OK-2010-02-17: [[Crash]] - If 0 is passed in fieldnum, this would crash.
	if (fieldnum > 0)
		fieldnum--;

	if ((int)fieldnum >= fieldCount) 
		return False;

	return fields[fieldnum] -> isNull;
}
void CDBCursor::FreeFields()
{
	if (fields)
	{
		for (int i = 0; i < fieldCount; i++)
		{
			delete fields[i];
		}
		delete[] fields;
	}
	fields = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static DBcallbacks *dbcallbacks = NULL;

extern "C" LIBRARY_EXPORT void setcallbacksref(DBcallbacks *callbacks)
{
    dbcallbacks = callbacks;
}

#ifndef REVDB_BUILD
extern "C" void *MCSupportLibraryLoad(const char *p_path)
{
    if (dbcallbacks == NULL)
        return NULL;
    return dbcallbacks -> load_module(p_path);
}

extern "C" void MCSupportLibraryUnload(void *p_handle)
{
    if (dbcallbacks == NULL)
        return;
    dbcallbacks -> unload_module(p_handle);
}

extern "C" void *MCSupportLibraryLookupSymbol(void *p_handle, const char *p_symbol)
{
    if (dbcallbacks == NULL)
        return NULL;
    return dbcallbacks -> resolve_symbol_in_module(p_handle, p_symbol);
}
#endif

////////////////////////////////////////////////////////////////////////////////
