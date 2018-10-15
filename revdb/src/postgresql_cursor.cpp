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

/*DBCURSOR_POSTGRESQL - CUSROR OBJECT FOR POSTGRESQL DATABASES CHILD OF DBCURSOR
Attributes:
POSTGRESQL_res - result set structure for POSTGRESQL database
isBOF - True if at beginning of resultset (inherited)
isEOF - True if at end of resultset (inherited)
recordNum - Number of current record -1 (inherited)
recordCount - Number of records in resultset (inherited)
fieldCount - Number of columns (inherited)
fields - Array of fields (inherited)
connection - Pointer to dbconnection_POSTGRESQL object that created cursor 
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

#include "dbpostgresql.h"

/*Open - opens cursor and retrieves resultset from connection
Output: False on error*/
Bool DBCursor_POSTGRESQL::open(DBConnection *newconnection,  PGresult *querycursor)
{
	if (!newconnection->getIsConnected())
		return False;

	//store result set
	connection = newconnection;
	POSTGRESQL_res = querycursor;

	//if no result set (could be no records) and there is an error then exit
	if (POSTGRESQL_res == NULL)
		return False;

	//get number of columns and rows
	recordCount = (unsigned int)PQntuples(POSTGRESQL_res);
	fieldCount = PQnfields(POSTGRESQL_res);

	//get column info
	if (!getFieldsInformation())
		return False;

	//fetch first row in result set
	if (recordCount != 0)
		first();

	return True;
}

//Close - close cursor and free resources used by cursor
void DBCursor_POSTGRESQL::close()
{
		FreeFields();
		
		if (POSTGRESQL_res != NULL)
		{
			PQclear(POSTGRESQL_res);
			POSTGRESQL_res = NULL;
		}
		isBOF = False;
		isEOF = True;  
		recordNum = recordCount = fieldCount = 0;
}

/// Move to start of query, setting isBOF to true and recordNum to 0
Bool DBCursor_POSTGRESQL::first()
{
	//exit if no more records
	if (recordCount == 0)
		return False;

	recordNum = 0;
	if (!getRowData())
		return False;

	isBOF = True;
	isEOF = False;
	return True;
}

/// Move to end of query, setting isEOF to true and recordNum to recordCount - 1 (the last record)
Bool DBCursor_POSTGRESQL::last()
{
	if (recordCount == 0)
		return False;

	recordNum = recordCount - 1;
	if (!getRowData())
		return False;

	isBOF = False;
	isEOF = True;
	return True;
}

/// Move to next record if one exists. If moving to the new record would take us beyond
/// the last record then set isEOF to true, and remain on the last record. 
Bool DBCursor_POSTGRESQL::next()
{
	if (recordCount == 0 || isEOF == True)
		return False;

	isBOF = False;
	recordNum++;
	if (recordNum == recordCount)
	{
		isEOF = true;
		recordNum = recordCount - 1;
		return False;
	}
	getRowData();
	return True;
}


Bool DBCursor_POSTGRESQL::move(int p_record_index)
{
	if (recordCount == 0)
		return False;

	if (p_record_index < 0)
		return False;
	else if (p_record_index > recordCount - 1)
		return False;

	// Not sure how these should behavior here, for now we just do the minimum
	// and make them false if we know they should be.
	isBOF = False;
	isEOF = False;

	recordNum = p_record_index;
	getRowData();

	return True;
}

/// Move to previous record if one exists. If already at start of query, do nothing. 
/// If the new record would take us before the first record then set isBOF to true
/// and set recordNum to 0 (the first record).
Bool DBCursor_POSTGRESQL::prev()
{
	if (recordCount == 0)
		return False;

	if (isBOF)
		return False;

	isEOF = false; 
	recordNum--;
	if (recordNum == -1)
	{
		isBOF =True;
		recordNum = 0;
		return False;
	}
	getRowData();
	return True;
}


/*getFieldsInformation - get column names, types, and info
Output: False on error*/
Bool DBCursor_POSTGRESQL::getFieldsInformation()
{
	fields = new (nothrow) DBField *[fieldCount];
	for (unsigned int i=0; i< fieldCount; i++) 
	{
		DBField *tfield = new (nothrow) DBField();
		fields[i] = tfield;
		char* tmpStr;
		tmpStr = PQfname(POSTGRESQL_res, i);
			//get field name..truncate if necessary
			if (strlen(tmpStr) > F_NAMESIZE -6)
				strncpy(tfield->fieldName, tmpStr, F_NAMESIZE-6);
			else
				strcpy(tfield->fieldName, tmpStr);
			//get field number
			tfield->fieldNum = i+1;
			//get max length
			tfield->maxlength = PQftype(POSTGRESQL_res,i);
			//get field type..convert database specific type to generic
			switch (PQftype(POSTGRESQL_res, i)) 
			{
			case PG_TYPE_BOOL:
				tfield->fieldType = FT_BOOLEAN;
				break;
			case PG_TYPE_CHAR:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_NAME:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_CHAR16:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_INT2:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_INT28:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_INT4:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_TEXT:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_CHAR2:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_CHAR4:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_CHAR8:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_PATH:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_FILENAME:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_FLOAT4:
				tfield->fieldType = FT_FLOAT;
				break;
			case PG_TYPE_FLOAT8:
				tfield->fieldType = FT_DOUBLE;
				break;
			case PG_TYPE_NUMERIC:
				tfield->fieldType = FT_DOUBLE;
				break;
			case PG_TYPE_MONEY:
				tfield->fieldType = FT_DOUBLE;
				break;
			case PG_TYPE_BPCHAR:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_VARCHAR:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_DATE:
				tfield->fieldType = FT_DATE;
				break;
			case PG_TYPE_TIME:
				tfield->fieldType = FT_TIME;
				break;
			case PG_TYPE_DATETIME:
				tfield->fieldType = FT_DATETIME;
				break; 
			case PG_TYPE_TIMESTAMP:
				tfield->fieldType = FT_TIMESTAMP;
				break;  
			case PG_TYPE_TIMESPAN:
				tfield->fieldType = FT_TIME;
				break;
			case PG_TYPE_RELTIME:
				tfield->fieldType = FT_TIME;
				break;
			case PG_TYPE_ABSTIME:
				tfield->fieldType = FT_DATETIME;
				break;
				//Array Types (Changes the type category to array)
			case PG_TYPE_ARR_BOOL:
				tfield->fieldType = FT_BOOLEAN;
				break;
			case PG_TYPE_ARR_CHAR:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_ARR_NAME:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_ARR_INT2:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_ARR_INT4:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_ARR_TEXT:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_ARR_VARCHAR:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_ARR_INT8:
				tfield->fieldType = FT_INTEGER;
				break;
			case PG_TYPE_ARR_PATH:
				tfield->fieldType = FT_STRING;
				break;
			case PG_TYPE_ARR_FLOAT4:
				tfield->fieldType = FT_FLOAT;
				break;
			case PG_TYPE_ARR_FLOAT8:
				tfield->fieldType = FT_DOUBLE;
				break;
			case PG_TYPE_ARR_RELTIME:
				tfield->fieldType = FT_TIME;
				break;
			case PG_TYPE_ARR_ABSTIME:
				tfield->fieldType = FT_DATETIME;
				break;
			case PG_TYPE_ARR_NUMERIC:
				tfield->fieldType = FT_DOUBLE;
				break;
			// OK-2007-06-19
			case PG_TYPE_ARR_BYTEA:
				tfield->fieldType = FT_BLOB;
				break;
			case PG_TYPE_BYTEA:
				tfield->fieldType = FT_BLOB;
				break;
			default:
				tfield->fieldType = FT_STRING;
		}
			//get column characteristics
			tfield->isAutoIncrement = False;
			tfield->isPrimaryKey = False;
			tfield->isUnique = False;
			tfield->isNotNull = False;
		}
	return True;
}

/*getRowData - Retrieve the data from the current row.
Output: False on error*/
Bool DBCursor_POSTGRESQL::getRowData()
{
	for (unsigned int i=0; i<fieldCount; i++)
	{
		fields[i] -> dataSize = PQgetlength(POSTGRESQL_res, recordNum, i); 
		fields[i] -> isNull = PQgetisnull(POSTGRESQL_res, recordNum, i);

		if (fields[i] -> isNull)
			fields[i] -> dataSize = 0;
		
		fields[i] -> freeBuffer = False; 
		if (fields[i] -> dataSize == 0)
			continue;

		if (fields[i] -> fieldType == FT_BLOB)
			fields[i] -> data = (char *)PQunescapeBytea((unsigned char *)PQgetvalue(POSTGRESQL_res, recordNum, i), (size_t *)&fields[i] -> dataSize);
		else
			fields[i] -> data = PQgetvalue(POSTGRESQL_res, recordNum, i);
	}
	return True;
}
