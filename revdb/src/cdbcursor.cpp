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

#include "dbdrivercommon.cpp"


//DBCURSOR STUFF
char *CDBCursor::getErrorMessage() {return connection->getErrorMessage();}

Bool CDBCursor::IsError() {return connection->IsError();}


unsigned int CDBCursor::getFieldLength(unsigned int fieldnum)
{
	//offset by zero
	fieldnum--;
	//check that field is in range
	if (fieldnum >= fieldCount) return NULL;
	return (unsigned int)fields[fieldnum]->maxlength;
}
/*getFieldName - get name of a field by field number (1 based)
Input: fieldnum - number of field to get name of
Output: name of field, NULL on error*/ 
char *CDBCursor::getFieldName(unsigned int fieldnum)
{
	//offset by zero
	fieldnum--;
	//check that field is in range
	if (fieldnum >= fieldCount) return NULL;
	return fields[fieldnum]->fieldName;
}

DBFieldType CDBCursor::getFieldType(unsigned int fieldnum)
{
fieldnum--;
if (fieldnum >= fieldCount) return FT_NULL;
return fields[fieldnum]->fieldType;
}

/*getFieldDataBinary - get data of a column
Input: 
fieldnum - number of field to get data from
fdlength - will recieve length of data
Output: pointer to fields data, NULL on error*/ 
char *CDBCursor::getFieldDataBinary(unsigned int fieldnum,unsigned int &fdlength)
{

	fieldnum--;
	if (fieldnum >= fieldCount) return NULL;
	//fill in data size
	fdlength = fields[fieldnum]->dataSize;
	//return pointer to data..if NULL return pointer to blank value
	return fields[fieldnum]->isNull == True? (char *)DBNullValue: fields[fieldnum]->data;
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
	if (fdata == NULL) return NULL;
	//allocate space for copy of data plus the trailing null
	if (fdata != DBNullValue)
	fdata[fdlength] = '\0'; 
	return fdata;
}

Bool	CDBCursor::getFieldIsNull(unsigned int fieldnum)
{
	fieldnum--;
	if (fieldnum >= fieldCount) return False;
	return fields[fieldnum]->isNull;
}
void CDBCursor::FreeFields()
{
	if (fields){
		for (unsigned int i = 0; i < fieldCount; i++)
			delete fields[i];
		delete fields;
	}
	fields = NULL;
}
