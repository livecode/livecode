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

/*DBCURSOR_ODBC - CUSROR OBJECT FOR ODBC DATABASES CHILD OF DBCURSOR
Attributes:
ODBC_res - result set structure for ODBC database
isBOF - True if at beginning of resultset (inherited)
isEOF - True if at end of resultset (inherited)
recordNum - Number of current record -1 (inherited)
recordCount - Number of records in resultset (inherited)
fieldCount - Number of columns (inherited)
fields - Array of fields (inherited)
connection - Pointer to dbconnection_ODBC object that created cursor 
Methods:
Open - opens cursor and retrieves resultset from connection
Close - closes cursor and frees any resources used by cursor
getFieldsInformation - get column names, types, and info
getRowData - Retrieve the data from the current row.
first - move to first row of resultset
next - move to next row of resultset
prev - move to previous row of resultset
last - move to last row of resultset
*/

#include <stdio.h>
#include <stdint.h>
#include "dbodbc.h"

char DBCursor_ODBC::errmsg[512];

typedef unsigned short uint2;

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint2 * lpDestStr, int cchDest);
int UnicodeToUTF8(uint2 *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest);

static char *unidecode(char *unistring, int tsize)
{
  int length = tsize >> 1;
  char *sansiptr = new (nothrow) char[tsize];
  char *ansiptr = sansiptr;
  char *uniptr = unistring;  
  while (length--)
  {
    *ansiptr++ = *++uniptr;
    uniptr++;
  }
  *ansiptr++ = '\0';
  return sansiptr;
}
/*Open - opens cursor and retrieves resultset from connection
Output: False on error*/
Bool DBCursor_ODBC::open(DBConnection *p_connection, SQLHSTMT p_statement, int p_rows)
{
	if (!p_connection -> getIsConnected())
		return False;

	connection = p_connection;
	ODBC_res = p_statement;

	SQLSMALLINT t_column_count;
	SQLNumResultCols(ODBC_res, &t_column_count);

	// OK-2007-08-17 : Query the driver first to determine whether the row count
	// is available, if not the row count should be set to -1.
	SQLUINTEGER t_cursor_attributes;
	SQLGetInfoA(((DBConnection_ODBC *)p_connection)->getHDBC(), SQL_STATIC_CURSOR_ATTRIBUTES2, &t_cursor_attributes, 0, NULL);

	SQLLEN t_row_count;
	if (t_cursor_attributes & SQL_CA2_CRC_EXACT)
		SQLRowCount(ODBC_res, &t_row_count);
	else
		t_row_count = -1;	

	fieldCount = t_column_count;
	recordCount = t_row_count;

	maxrows = 1;

	if (!getFieldsInformation())
		return False;

	isBOF = False;
	isEOF = False;

	// OK-2008-01-18 : Bug 5440. If the user has chosen to use forward only cursors
	// we can't move to the first record here, so instead we just fetch the current record
	// (which will be the first one anyway).
	DBConnection_ODBC *t_connection;
	t_connection = (DBConnection_ODBC *)p_connection;

	m_type = t_connection -> getCursorType();

	if (m_type == kCursorTypeForward)
	{
		if (fieldCount != 0)
		{
			SQLRETURN t_result;
			t_result = SQLFetch(ODBC_res);
			if (t_result != SQL_SUCCESS)
				isEOF = True;

			// OK-2009-09-07 : [[Bug]] - Even if using a forward-only cursor, we know at this point
			// whether the recordset is empty or not.
			if (t_result == SQL_NO_DATA)
				recordCount = 0;

			isBOF = True;
			recordNum = 0;
			getRowData();
		}
	}
	else
		first();

	return True;
}

//Close - close cursor and free resources used by cursor
void DBCursor_ODBC::close()
{
	FreeFields();
	if (ODBC_res != NULL)
	{
		SQLFreeStmt(ODBC_res, SQL_DROP);
		ODBC_res = NULL;
	}

	isBOF = False;
	isEOF = True;
	recordNum = 0;
	recordCount = 0;
	fieldCount = 0;
}

/*first - move to first row of resultset
Output - False on error*/
Bool DBCursor_ODBC::first()
{
	if (fieldCount == 0 || m_type == kCursorTypeForward)
		return False;

	isBOF = True;
	isEOF = False;

	SQLRETURN t_result;
	t_result = SQLFetchScroll(ODBC_res, SQL_FETCH_FIRST, 0);
	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
		isEOF = True;
		return False;
	}
	recordNum = 0;
	getRowData();
	return True;

}

/*last - move to last row of resultset
Output - False on error*/
Bool DBCursor_ODBC::last()
{
	if (fieldCount == 0 || m_type == kCursorTypeForward)
		return False;

	isBOF = False;
	isEOF = True;

	SQLRETURN t_result;
	t_result = SQLFetchScroll(ODBC_res, SQL_FETCH_LAST, 0);
	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
		isEOF = True;
		return False;
	}
	
	recordNum = recordCount - 1;
	getRowData();
	return True;
}

/*next - move to next row of resultset
Output - False on error*/
Bool DBCursor_ODBC::next()
{
	if (fieldCount == 0 || isEOF == True)
		return False;

	recordNum++;
	isBOF = False;

	if (recordNum == recordCount)
	{
		isEOF = True;
		recordNum = recordCount - 1;
		return False;
	}
	else
		isEOF = False;

	SQLRETURN t_result;
	if (m_type == kCursorTypeForward)
		t_result = SQLFetch(ODBC_res);
	else
		t_result = SQLFetchScroll(ODBC_res, SQL_FETCH_NEXT, 0);

	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
		isEOF = True;
		return False;
	}

	getRowData();
	return True;
}

Bool DBCursor_ODBC::move(int p_record_index)
{
	if (recordCount == 0)
		return False;

	if (p_record_index == recordNum)
		return True;

	// If we are using a forward only cursor, then we can't do this, however
	// it may be possible to emulate the movement, in particular if the required
	// record_index is greater than the current record number.
	if (m_type == kCursorTypeForward)
		return CDBCursor::move(p_record_index);

	if (p_record_index < 0)
		return False;
	else if (recordCount != -1 && p_record_index > recordCount - 1)
		return False;

	recordNum = p_record_index;

	SQLRETURN t_result;
	t_result = SQLFetchScroll(ODBC_res, SQL_FETCH_ABSOLUTE, recordNum + 1);
	if (t_result != SQL_SUCCESS && t_result != SQL_SUCCESS_WITH_INFO)
	{
		isEOF = True;
		return False;
	}
	else
	{
		isBOF = False;
		isEOF = False;
	}

	getRowData();
	return True;
}


/*prev - move to previous row of resultset
Output - False on error*/
Bool DBCursor_ODBC::prev()
{
	if ((fieldCount == 0) | (isBOF == True) || m_type == kCursorTypeForward)
		return False;

	isEOF = False;

	recordNum--;

	if (recordNum == -1)
	{
		isBOF = True;
		recordNum = 0;
		return False;
	}

	SQLRETURN t_result;
	t_result = SQLFetchScroll(ODBC_res, SQL_FETCH_PRIOR, 0);
	if (t_result == SQL_NO_DATA)
	{
		isEOF = True;
		return False;
	}

	getRowData();
	return True;
}


/*getFieldsInformation - get column names, types, and info
Output: False on error*/
Bool DBCursor_ODBC::getFieldsInformation()
{
	int totalsize = 0;
	SQLRETURN retcode;
	SQLSMALLINT buflen;
	SQLSMALLINT dbtype;
	SQLULEN dbsize;
	SQLSMALLINT decDigits;
	SQLSMALLINT nullablePtr;
	fields = new (nothrow) DBField *[fieldCount];
	for (unsigned int i=0; i < (unsigned int)fieldCount; i++)
	{
		DBField_ODBC *ofield = new (nothrow) DBField_ODBC();
		fields[i] = (DBField *)ofield;
		retcode = SQLDescribeColA(ODBC_res, i+1, (SQLCHAR *)ofield->fieldName, F_NAMESIZE, &buflen, &dbtype, &dbsize, &decDigits, &nullablePtr);
		if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		{
			return False;
		}
		ofield->fieldName[buflen] = '\0';
		ofield->fieldNum = i+1;
		ofield->maxlength = dbsize+1;
		
		switch (dbtype)
		{
#ifdef UTF8MODE
		case SQL_CHAR:
			ofield->fieldType = FT_WSTRING;
			ofield -> maxlength *= 2;
			break;
		case SQL_VARCHAR:
			ofield->fieldType = FT_WSTRING;
			ofield -> maxlength *= 2;
			break;
		case SQL_LONGVARCHAR:
			ofield->maxlength = LONG_CHUNK_SIZE;
			ofield->fieldType = FT_WSTRING;
			break;
#else
		case SQL_WCHAR:
			ofield -> maxlength *= 2;
			ofield->fieldType = FT_WSTRING;
			break;
		case SQL_WVARCHAR:
			ofield -> maxlength *= 2;
			ofield->fieldType = FT_WSTRING;
			break;
		case SQL_WLONGVARCHAR:
			ofield->maxlength = LONG_CHUNK_SIZE;
			ofield->fieldType = FT_WSTRING;
			break;

		case SQL_CHAR:
			ofield->fieldType = FT_STRING;
			break;
		case SQL_VARCHAR:
			ofield->fieldType = FT_STRING;
			break;
		case SQL_LONGVARCHAR:
			ofield->maxlength = LONG_CHUNK_SIZE;
			ofield->fieldType = FT_STRING;
			break;
#endif
		case SQL_DECIMAL:
			ofield->fieldType = FT_FLOAT;
			break;
		case SQL_NUMERIC:
			ofield->fieldType = FT_FLOAT;
			break;
		case SQL_SMALLINT:
			ofield->fieldType = FT_SMALLINT;
			break;
		case SQL_INTEGER:
			ofield->fieldType = FT_INTEGER;
			break;
		case SQL_REAL:
			ofield->fieldType = FT_FLOAT;
			break;
		case SQL_FLOAT:
			ofield->fieldType = FT_DOUBLE;
			break;
		case SQL_DOUBLE:
			ofield->fieldType = FT_DOUBLE;
			break;
		case SQL_BIT:
			ofield->fieldType = FT_BIT;
			break;
		case SQL_TINYINT:
			ofield->fieldType = FT_SMALLINT;
			break;
		case SQL_BIGINT:
			ofield->fieldType = FT_INTEGER;
			break;
		case SQL_BINARY:
			ofield->fieldType = FT_BLOB;
			break;
		case SQL_VARBINARY:
			ofield->fieldType = FT_BLOB;
			break;
		case SQL_LONGVARBINARY:
			ofield->fieldType = FT_BLOB;
			break;
		case SQL_DATE:
			ofield->fieldType = FT_DATE;
			ofield -> maxlength = 11;
			break;
		case SQL_TIME:
			ofield->fieldType = FT_TIME;
			ofield -> maxlength = 9;
			break;
		case SQL_TIMESTAMP:
			ofield->fieldType = FT_TIMESTAMP;
			ofield -> maxlength = 20;
			break;
		case SQL_GUID:
			ofield->fieldType = FT_STRING;
			break;
		default:
			ofield->fieldType = FT_NULL;
		}
	
		if (ofield->fieldType == FT_BLOB)
			ofield->maxlength = LONG_CHUNK_SIZE;
		else if (ofield->maxlength > RAWSIZE || !ofield->maxlength)
			ofield->maxlength = RAWSIZE;
		
		ofield->isAutoIncrement = False;
		ofield->isPrimaryKey = False;
		ofield->isUnique = False;
		ofield->isNotNull = nullablePtr & SQL_NO_NULLS;
		
		// OK-2010-02-08: [[Bug 7637]] - ActualTechnologies Driver appears to require an extra
		// byte in the output buffer. This can effect varchar and date / time columns at least.
		if (ofield -> fieldType != FT_BLOB)
			ofield -> maxlength = ofield -> maxlength + 1;

		if (ofield -> fieldType == FT_WSTRING)
			ofield -> data = new (nothrow) char[ofield -> maxlength + 2];
		else
			ofield -> data = new (nothrow) char[ofield -> maxlength + 1];
		totalsize += ofield->maxlength;
		ofield->freeBuffer = True;
	}
	
	maxrows = 1;
	SQLSetStmtOption(ODBC_res, SQL_ROWSET_SIZE, 1);
	
	return True;
}


/*getRowData - Retrieve the data from the current row.
Output: False on error*/
Bool DBCursor_ODBC::getRowData()
{
	SQLRETURN retcode;
	for (unsigned int i = 0; i < (unsigned int)fieldCount; i++)
	{
		int offset;
		SQLLEN buf_ind;
		DBField_ODBC *ofield = (DBField_ODBC *)fields[i];
		if (ofield->extraData)
		{
			free(ofield->extraData);
			ofield->extraData = NULL;
		}

		buf_ind = 0;
		retcode = SQLGetData(ODBC_res, i + 1, ofield -> fieldType == FT_BLOB ? SQL_C_BINARY : (ofield -> fieldType == FT_WSTRING ? SQL_C_WCHAR : SQL_C_CHAR), (SQLPOINTER) ofield -> data, ofield -> maxlength, &buf_ind);
		if (buf_ind == SQL_NULL_DATA)
			ofield -> isNull = true;
		else
		{
			ofield -> isNull = false;
			
			// MW-2008-08-01: [[ Bug 5967 ]] Work-around issue with Actual Tech. drivers on OS X returning bad length info for
			//   these column types (as far as I can see!).

			// OK-2010-02-23: MS SQL Server returns wrong data length for converted numeric types when "Use regional settings when outputting currency, numbers, dates and times" is checked.
			// Seems that the only way around this is to ignore the buf_ind for all non-binary types and instead search for nulls.

			if (!(ofield -> fieldType == FT_BIT || ofield -> fieldType == FT_BLOB || ofield -> fieldType == FT_WSTRING || ofield -> fieldType == FT_WORD))
			{
				int t_length;
				for(t_length = 0; t_length < ofield -> maxlength; ++t_length)
					if (ofield -> data[t_length] == '\0')
						break;
				ofield -> dataSize = t_length;
			}
			else
				ofield -> dataSize = buf_ind;

			if (retcode != SQL_SUCCESS)
			{
				// OK-2009-11-16: [[Bug 8368]]
				// Before assuming that the return code of SQL_SUCCESS_WITH_INFO is due to 
				// data truncation, get the errorState to make sure this is the case, otherwise
				// if the driver returns a general warning, this may infinite loop.
				SQLINTEGER t_error;
				SWORD t_errormsgsize;
				SQLCHAR t_state[6];
				SQLCHAR t_error_message[SQL_MAX_MESSAGE_LENGTH];

				DBConnection_ODBC * odbcconn = (DBConnection_ODBC *)connection;

				SQLRETURN t_error_result;
				t_error_result = SQLErrorA(odbcconn -> getHENV(), odbcconn -> getHDBC(), ODBC_res, t_state, &t_error, t_error_message, SQL_MAX_MESSAGE_LENGTH - 1, &t_errormsgsize);

				if (t_error_result == SQL_SUCCESS && strcmp((const char *)t_state, "01004") == 0)
				{
					// MM-2012-02-28: [[ Bug ]] Make sure there is enough space in the data buffer.
					ofield -> data = (char *)realloc(ofield -> data, RAWSIZE + 1);

					// Allocate 64k buffer initially
					ofield -> extraData = (char *)malloc(RAWSIZE + 1);
					offset = 0;
					unsigned int bytestocopy;
					while (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
					{   
						// If the amount of data is greater than the buffer allocated, then put the buffer size into bytestocopy,
						// otherwise put the data length into it
						if (buf_ind > ofield -> maxlength)
							bytestocopy = ofield->maxlength;
						else
							bytestocopy = buf_ind;

						// Copy bytestocopy data from data to extraData
						memcpy((char*)(ofield->extraData + offset), ofield->data, bytestocopy);

						// Calculate the offset for the next lump of data to be copied to, overwriting null terminating chars if appropriate.
						if (ofield -> fieldType == FT_BLOB)
							offset += bytestocopy;
						else if (ofield -> fieldType == FT_WSTRING)
							offset += bytestocopy - 2;
						else
							offset += bytestocopy - 1;

						// Reallocate another offset bytes in extraData
						ofield->extraData = (char *)realloc((char *)ofield->extraData,  offset + RAWSIZE +1);

						retcode = SQLGetData(ODBC_res, i+1, ofield->fieldType == FT_BLOB ? SQL_C_BINARY : (ofield -> fieldType == FT_WSTRING ? SQL_C_WCHAR : SQL_C_CHAR),  ofield->data, RAWSIZE, &buf_ind);
					}
					ofield->dataSize = offset;
				}
			}
            
            if (ofield -> fieldType == FT_WSTRING && sizeof(SQLWCHAR) != sizeof(uint16_t))
            {
                uint16_t * t_buffer = static_cast<uint16_t *>(malloc(ofield -> dataSize));
            
                size_t t_in_offset;
                size_t t_out_offset = 0;
                for (t_in_offset = 0; t_in_offset < size_t(ofield -> dataSize); t_in_offset += sizeof(uint32_t))
                {
                    uint32_t t_codepoint = *(uint32_t*)&ofield -> data[t_in_offset];
                    if (t_codepoint < 0x10000)
                    {
                        t_buffer[t_out_offset] = uint16_t(t_codepoint);
                        t_out_offset += 1;
                    }
                    else
                    {
                        uint16_t t_lead, t_trail;
                        t_lead =  uint16_t((t_codepoint - 0x10000) >> 10) + 0xD800;
                        t_trail = uint16_t((t_codepoint - 0x10000) & 0x3FF) + 0xDC00;
                        t_buffer[t_out_offset] = t_lead;
                        t_buffer[t_out_offset + 1] = t_trail;
                        t_out_offset += 2;
                    }
                }
                
                memcpy(ofield->data, t_buffer, t_out_offset * sizeof(uint16_t));
                free(t_buffer);
                
                ofield -> dataSize = t_out_offset * sizeof(uint16_t);
            }
		}
	}
	return True;
}

char *DBCursor_ODBC::getErrorMessage()
{
    SQLINTEGER	error;        /* Not used */
    SWORD	errormsgsize; /* Not used */
    DBConnection_ODBC * odbcconn = (DBConnection_ODBC *)connection;
    SQLRETURN err = SQLErrorA(odbcconn->getHENV(), odbcconn->getHDBC(), ODBC_res, NULL, &error, (SQLCHAR *)errmsg, SQL_MAX_MESSAGE_LENGTH-1, &errormsgsize);
    
    if (err == SQL_SUCCESS)
		return errmsg;
    else
        return (char *)DBNullValue;
    
}


char *DBCursor_ODBC::getFieldDataBinary(unsigned int fieldnum,unsigned int &fdlength)
{
	fieldnum--;
	if (fieldnum >= fieldCount)
		return NULL;

	DBField_ODBC *t_field;
	t_field = (DBField_ODBC *)fields[fieldnum];

	fdlength = t_field -> dataSize;
	if (t_field -> isNull == True)
	{
		fdlength = 0;
		return (char *)DBNullValue;
	}

	if (t_field -> extraData != NULL)
		return t_field -> extraData;
	else
		return t_field -> data;
		
}
