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

/*DBCURSOR_ORACLE - CUSROR OBJECT FOR ORACLE DATABASES CHILD OF DBCURSOR
Attributes:
ORACLE_res - result set structure for ORACLE database
isBOF - True if at beginning of resultset (inherited)
isEOF - True if at end of resultset (inherited)
recordNum - Number of current record -1 (inherited)
recordCount - Number of records in resultset (inherited)
fieldCount - Number of columns (inherited)
fields - Array of fields (inherited)
connection - Pointer to dbconnection_ORACLE object that created cursor 
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


#include "dboracle.h"

char DBCursor_ORACLE::errmsg[512];

/*Open - opens cursor and retrieves resultset from connection
Output: False on error*/
Bool DBCursor_ORACLE::open(DBConnection *newconnection, Cda_Def *oraclecursor, int p_rows)
{
	if (!newconnection->getIsConnected())
		return False;

	connection = newconnection;
	DBConnection_ORACLE *ORACLEconn = (DBConnection_ORACLE *)connection;
	ORACLE_res = oraclecursor;
	
	recordCount = -1;
	maxrows = 1;

	if (!maxrows)
		cacherows = True;
	rowsCached = 0;
	if (!getFieldsInformation())
		return False;
	isBOF = True;
	isEOF = False;
	FetchedNum = 0;
	recordNum = -1;
	next();
	//recordNum = 0;
	return True;
}

//Close - close cursor and free resources used by cursor
void DBCursor_ORACLE::close()
{
		//free column data
		FreeFields();

		//Make sure any stored query results are free.
		if (ORACLE_res != NULL) 
		{
			oclose(ORACLE_res);
			delete ORACLE_res;
			ORACLE_res = NULL;
		}

		isBOF = False;
		isEOF = True;  
		recordNum = recordCount = fieldCount = 0;
}

/*first - move to first row of resultset
Output - False on error*/
Bool DBCursor_ORACLE::first()
{
	return False;
}

/*last - move to last row of resultset
Output - False on error*/
Bool DBCursor_ORACLE::last()
{
	return False;
}

/*next - move to next row of resultset
Output - False on error*/
Bool DBCursor_ORACLE::next()
{
	if (fieldCount == 0 || isEOF == True)
		return False;

	isBOF = False;

	FetchedNum++;

	if (recordCount < 1 || FetchedNum == rowsCached)
	{
		ofen(ORACLE_res, maxrows);
		if (ORACLE_res->rc == NULL_VALUE_RETURNED)
				ORACLE_res->rc = 0;

		if (ORACLE_res->rc == 0 || (ORACLE_res->rc == NO_DATA_FOUND && (ORACLE_res->rpc - recordCount) > 0))
		{
			rowsCached = ORACLE_res->rpc - recordCount;
			
			if (ORACLE_res -> rc == NO_DATA_FOUND)
				recordCount = ORACLE_res->rpc;
			else
				recordCount = -1;

			FetchedNum = 0;
		}
		else
		{
			isEOF = True;
			return False;
		}
	}

	recordNum++;

	if (recordNum == recordCount)
	{
		isEOF = true;
		recordNum = recordCount - 1;
		return False;
	}

	if (recordNum == 0)
		isBOF = True;
	else
		isBOF = False;

	getRowData();
	return True;
}


/*prev - move to first row of resultset
Output - False on error*/
Bool DBCursor_ORACLE::prev()
{
	return False;
}


/*getFieldsInformation - get column names, types, and info
Output: False on error*/
Bool DBCursor_ORACLE::getFieldsInformation()
{
	sb4 dbsize;
	int totalsize = 0;
	fieldCount = 0;
	while (True)
	{
		if (odescr(ORACLE_res, fieldCount + 1, &dbsize, 0, 0, 0, 0, 0, 0, 0))
			if (ORACLE_res->rc == VAR_NOT_IN_LIST)
				break;

			fieldCount++;
	}    
	fields = new DBField *[fieldCount];
	unsigned int i=0;
	for (i=0; i< fieldCount; i++)
	{
		DBField_ORACLE *ofield = new DBField_ORACLE();
		fields[i] = (DBField *)ofield;
		sb4 buflen = F_NAMESIZE;
		sb2 scale, nullok, dbtype;

		odescr(ORACLE_res, i+1, &dbsize, &dbtype, (sb1 *)ofield->fieldName, &buflen, 0, 0, &scale, &nullok);
		ofield->fieldName[buflen] = '\0';
		
		ofield->fieldNum = i + 1;
		
		ofield->maxlength = dbsize+1;
		
		ofield->externaltype = STRING_TYPE;
		switch (dbtype)
		{
		case CHAR_TYPE:
		case VARCHAR2_TYPE:
			ofield->fieldType = FT_CHAR;
			break;
		case NUMBER_TYPE:
			if (scale != 0)
				ofield->fieldType = FT_FLOAT;
			else
				ofield->fieldType = FT_INTEGER;
			break;
		case LONG_TYPE:
			ofield->fieldType = FT_STRING;
			ofield->externaltype = LRAW_TYPE;
			ofield->maxlength = LONG_CHUNK_SIZE;
			break;
		case ROWID_TYPE:
			ofield->fieldType = FT_STRING;
			break;
		case DATE_TYPE :
			ofield->fieldType = FT_DATE;
			break;
		case RAW_TYPE :
		case LRAW_TYPE:
			ofield->fieldType = FT_BLOB;
			ofield->externaltype = LRAW_TYPE;
			ofield->maxlength = LONG_CHUNK_SIZE;
			break;
		default:
			ofield -> fieldType = FT_STRING;
		} 
		
		ofield->isAutoIncrement = False;
		ofield->isPrimaryKey = False;
		ofield->isUnique = False;
		ofield->isNotNull = nullok == 0;
		//allocate for data and bind 
		ofield->maxlength = align(ofield->maxlength); 
		totalsize += ofield->maxlength;
		ofield->freeBuffer = False;
	}
	if (cacherows)
	{
		maxrows = MAXCOLBUF / totalsize;
		if (maxrows > MAXROWNUM) 
			maxrows = MAXROWNUM;
	}
	else
		maxrows = 1;

	for (i = 0; i < fieldCount; i++)
	{
		DBField_ORACLE *ofield = (DBField_ORACLE *)fields[i];
		ofield -> databuffer = new char[maxrows * ofield -> maxlength];
		ofield -> indp = new ub2[maxrows];
		ofield -> dsize = new ub2[maxrows];
		ofield -> errorcode = new ub2[maxrows];
		if (odefin(ORACLE_res, i + 1, (ub1 *)ofield -> databuffer, ofield -> maxlength, ofield -> externaltype, -1, (sb2 *)ofield -> indp, 0, -1, -1, (ub2 *)ofield -> dsize, (ub2 *)ofield -> errorcode))
			return False;
	}
	return True;
}


/*getRowData - Retrieve the data from the current row.
Output: False on error*/
Bool DBCursor_ORACLE::getRowData()
{
	for (unsigned int i=0; i<fieldCount; i++)
	{
		DBField_ORACLE *ofield = (DBField_ORACLE *)fields[i];

		ofield -> data = ofield -> databuffer + (ofield -> maxlength * FetchedNum);
		ofield -> dataSize = ofield -> dsize[FetchedNum];
		
		if (ofield -> extraData)
		{
			free(ofield -> extraData);
			ofield -> extraData = NULL;
		}

		if (ofield -> errorcode[FetchedNum] != 1405)
			ofield -> isNull = False;

		if (ofield->errorcode[FetchedNum] == 1405)
			ofield -> isNull = True;
		else if (!cacherows && ofield->errorcode[FetchedNum] == 1406 && (ofield->externaltype == RAW_TYPE || ofield->externaltype == LRAW_TYPE))
		{
			int lflength = (size_t)ofield->indp[FetchedNum];
			ofield->extraData = (char *)malloc(RAWSIZE*2);
			memcpy(ofield->extraData,ofield->data,RAWSIZE);
			sb4 offset = RAWSIZE;
			sb2 result;
			ub4 ret_len;
			while (True)
			{
				result = oflng(ORACLE_res, i+1, (ub1 *)(ofield->extraData + offset), RAWSIZE, ofield->externaltype, &ret_len,offset);
				if (result || ret_len <= 0)
					break;

				offset += ret_len;
				ofield->extraData = (char *)realloc(ofield->extraData,offset + RAWSIZE);
			}
			ofield->data = (char *)(ofield->extraData);
			ofield->dataSize = offset;
		}
		else if (ofield->errorcode[FetchedNum] != 0)
			ofield -> isNull = True;
	}
	return True;
}

char *DBCursor_ORACLE::getErrorMessage()
{
	if (ORACLE_res->rc != 0)
		oerhms(ORACLE_res, ORACLE_res->rc, (text *)errmsg, 
			(sword) sizeof(errmsg));
	return errmsg;
}
