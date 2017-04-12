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

/*DBCURSOR_MYSQL - CUSROR OBJECT FOR MYSQL DATABASES CHILD OF DBCURSOR
Attributes:
mysql_res - result set structure for MYSQL database
isBOF - True if at beginning of resultset (inherited)
isEOF - True if at end of resultset (inherited)
recordNum - Number of current record -1 (inherited)
recordCount - Number of records in resultset (inherited)
fieldCount - Number of columns (inherited)
fields - Array of fields (inherited)
connection - Pointer to dbconnection_mysql object that created cursor 
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


#include "dbmysql.h"

/*Open - opens cursor and retrieves resultset from connection
Output: False on error*/
Bool DBCursor_MYSQL::open(DBConnection *newconnection)
{
	if (!newconnection->getIsConnected())
		return False;

	connection = newconnection;
	DBConnection_MYSQL *mysqlconn = (DBConnection_MYSQL *)connection;
	mysql_res = mysql_store_result(mysqlconn->getMySQL());
	
	if (mysql_res == NULL || mysql_errno(mysqlconn->getMySQL()))
		return False;

	recordCount = (unsigned int)mysql_num_rows(mysql_res);
	fieldCount = mysql_num_fields(mysql_res);
	
	if (!getFieldsInformation())
		return False;
	
	if (recordCount != 0)
		first();

	return True;
}

//Close - close cursor and free resources used by cursor
void DBCursor_MYSQL::close()
{
		FreeFields();

		if (mysql_res != NULL)
		{
			mysql_free_result(mysql_res);
			mysql_res = NULL;
		}

		isBOF = False;
		isEOF = True;  
		recordNum = 0;
		recordCount = 0;
		fieldCount = 0;
		libraryRecordNum = 0;
}

/*first - move to first row of resultset
Output - False on error*/
Bool DBCursor_MYSQL::first()
{
	if (recordCount == 0)
		return False;

	recordNum = 0;
	mysql_data_seek(mysql_res, recordNum);
	libraryRecordNum = 0;
	if (!getRowData())
		return False;

	isBOF = True;
	isEOF = False;
	return True;
}

/*last - move to last row of resultset
Output - False on error*/
Bool DBCursor_MYSQL::last()
{
	if (recordCount == 0)
		return False;

	recordNum = recordCount - 1;
	mysql_data_seek(mysql_res, recordNum);
	libraryRecordNum = recordNum;
	if (!getRowData())
		return False;

	isBOF = False;
	isEOF = False;
	return True;
}

Bool DBCursor_MYSQL::move(int p_record_index)
{
	if (recordCount == 0)
		return False;

	// Don't allow the recordNum to be set to something outside the range.
	if (p_record_index < 0)
		return False;
	else if (p_record_index > recordCount - 1)
		return False;

	isBOF = False;
	isEOF = False;

	recordNum = p_record_index;

	mysql_data_seek(mysql_res, recordNum);
	libraryRecordNum = recordNum;
	if (!getRowData())
		return False;

	return True;

}

/*next - move to next row of resultset
Output - False on error*/
Bool DBCursor_MYSQL::next()
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

	if (recordNum == 0)
		isBOF = True;
	else
		isBOF = False;

	if (libraryRecordNum != recordNum)
	{
		mysql_data_seek(mysql_res, recordNum);
		libraryRecordNum = recordNum;
	}

	getRowData();
	return True;
}

/*prev - move to first row of resultset
Output - False on error*/
Bool DBCursor_MYSQL::prev()
{
	if (recordCount == 0 || isBOF == True)
		return False;

	isEOF = False; 

	recordNum--;
	if (recordNum == -1)
	{
		isBOF = True;
		recordNum = 0;
		return False;
	}

	mysql_data_seek(mysql_res, recordNum);
	libraryRecordNum = recordNum;
	getRowData();
	return True;
}


/*getFieldsInformation - get column names, types, and info
Output: False on error*/
Bool DBCursor_MYSQL::getFieldsInformation()
{
	MYSQL_FIELD *field_prop;
	fields = new (nothrow) DBField *[fieldCount];
	for (unsigned int i=0; i< fieldCount; i++)
	{
		DBField *tfield = new (nothrow) DBField();
		fields[i] = tfield;
		field_prop = mysql_fetch_field(mysql_res);
		if (field_prop != NULL) 
		{
			if (strlen(field_prop->name) > F_NAMESIZE -6)
				strncpy(tfield->fieldName, field_prop->name, F_NAMESIZE-6);
			else
				strcpy(tfield->fieldName, field_prop->name);
			
			tfield->fieldNum = i+1;
			tfield->maxlength = field_prop->length;
			
			// OK-2007-06-14: Note there is a problem here where mysql_fetch_field() appears to be returning the wrong type for certain fields.
			// For example fields of type TEXT seem to be returning a type 252, which is FIELD_TYPE_BLOB. I think this could be due to a bug in MySQL
			// ( http://bugs.mysql.com/bug.php?id=27475 ).

			switch (field_prop->type) {
				case FIELD_TYPE_TINY :
					tfield->fieldType = FT_SMALLINT;
					break;
				case FIELD_TYPE_SHORT :
					tfield->fieldType = FT_SMALLINT;
					break;
				case FIELD_TYPE_LONG :
					tfield->fieldType = FT_INTEGER;
					break;
				case FIELD_TYPE_INT24 :
					tfield->fieldType = FT_INTEGER;
					break;
				case FIELD_TYPE_LONGLONG :
					tfield->fieldType = FT_INTEGER;
					break;
				case FIELD_TYPE_DECIMAL :
					tfield->fieldType = FT_FLOAT;
					break;
				case FIELD_TYPE_FLOAT :
					tfield->fieldType = FT_FLOAT;
					break;
				case FIELD_TYPE_DOUBLE :
					tfield->fieldType = FT_DOUBLE;
					break;
				case FIELD_TYPE_TIMESTAMP :
					tfield->fieldType = FT_TIMESTAMP;
					break;
				case FIELD_TYPE_DATE :
					tfield->fieldType = FT_DATE;
					break;
				case FIELD_TYPE_TIME :
					tfield->fieldType = FT_TIME;
					break;
				case FIELD_TYPE_DATETIME :
					tfield->fieldType = FT_DATETIME;
					break;
				case FIELD_TYPE_YEAR :
					tfield->fieldType = FT_DATE;
					break;
				case FIELD_TYPE_VAR_STRING :
					tfield->fieldType = FT_STRING;
					break;
				case FIELD_TYPE_ENUM :
					tfield->fieldType = FT_STRING;
					break;
				case FIELD_TYPE_STRING :
					tfield->fieldType = FT_STRING;
					break;
				case FIELD_TYPE_BLOB :
					tfield->fieldType = FT_BLOB;
					break;
				case FIELD_TYPE_NULL :
					tfield->fieldType = FT_NULL;
					break;
				default:
					tfield->fieldType = FT_STRING;
			} 
			
			tfield -> isAutoIncrement = field_prop -> flags & AUTO_INCREMENT_FLAG;
			tfield -> isPrimaryKey = field_prop -> flags & PRI_KEY_FLAG;
			tfield -> isUnique = field_prop -> flags & UNIQUE_KEY_FLAG;
			tfield -> isNotNull = field_prop -> flags & NOT_NULL_FLAG;
		}
	}
	return True;
}


/*getRowData - Retrieve the data from the current row.
Output: False on error*/
Bool DBCursor_MYSQL::getRowData()
{

	MYSQL_ROW row_data;
	unsigned long *row_lengths;

	row_data = mysql_fetch_row(mysql_res);
	if (!row_data)
		return False;

	libraryRecordNum = recordNum + 1;
	
	row_lengths = mysql_fetch_lengths(mysql_res);

	// MW-2008-07-29: [[ Bug 6853 ]] Make sure we set isNull to true for columns
	//   that are truely NULL and not just empty.
	for(int i = 0; i < fieldCount; i++)
	{
		if (row_data[i] == NULL)
		{
			fields[i] -> data = (char *)DBNullValue;
			fields[i] -> freeBuffer = False;
			fields[i] -> dataSize = 0;
			fields[i] -> isNull = True;
		}
		else
		{
			fields[i] -> data = row_data[i];
			fields[i] -> freeBuffer = False;
			fields[i] -> dataSize = row_lengths[i];
			fields[i] -> isNull = False;
		}
	}

	return True;
}
