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

/*DBCURSOR_SQLITE - CUSROR OBJECT FOR MYSQL DATABASES CHILD OF DBCURSOR
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




#include "dbsqlite.h"

#include <sqlitedecode.h>

#define ENTER 

DBCursor_SQLITE::DBCursor_SQLITE(SqliteDatabase &db, bool enable_binary) 
	: mDB(db)
{
	ENTER;
	mDataset = mDB.CreateDataset();
	m_enable_binary = enable_binary;
}

DBCursor_SQLITE::~DBCursor_SQLITE()
{
	ENTER;
	close();
	delete mDataset;
}

/*Open - opens cursor and retrieves resultset from connection
Output: False on error*/
Bool DBCursor_SQLITE::open(DBConnection *newconnection)
{
	ENTER;
	
	if (!newconnection->getIsConnected())
		return False;

	connection = newconnection;

	try
	{

		recordCount = mDataset -> num_rows();
		fieldCount = mDataset -> field_count();

		if(!getFieldsInformation())
			return False;

		if(recordCount != 0)
			first();

	} catch(DbErrors &e)
	{
		MDEBUG("***** Caught error [%s]\n", e.getMsg());
		return False;
	}

	MDEBUG0("Cursor::open OK\n");
	return True;
}

//Close - close cursor and free resources used by cursor
void DBCursor_SQLITE::close()
{
	ENTER;
	MDEBUG0("\n--\n");
	FreeFields();
	isBOF = False;
	isEOF = True;  
	recordNum = recordCount = fieldCount = 0;
	MDEBUG0("\nBye cursor\n");
}

/*first - move to first row of resultset
Output - False on error*/
Bool DBCursor_SQLITE::first()
{
	ENTER;
	try {
		//exit if no more records
		if (recordCount == 0)
			return False;
		recordNum = 0;
		mDataset->first();
		if(!getRowData())
			return False;
		isBOF = True;
		isEOF = False;

	} catch(DbErrors &e) {
		MDEBUG("***** Caught error [%s]\n", e.getMsg());
		return False;
	}

	return True;
}

/*last - move to last row of resultset
Output - False on error*/
Bool DBCursor_SQLITE::last()
{
	ENTER;
	try {
		if (recordCount == 0)
			return False;
		recordNum = recordCount-1;
		mDataset->last();
		if (!getRowData())
			return False;
		isBOF = False;
		isEOF = True;

	} catch(DbErrors &e) {
		MDEBUG("***** Caught error [%s]\n", e.getMsg());
		return False;
	}

	return True;
}

/*next - move to next row of resultset
Output - False on error*/
Bool DBCursor_SQLITE::next()
{
	ENTER;

	try {
		//exit if at last record or there are no records
		if (recordCount == 0 || isEOF == True)
			return False;
		isBOF = False;
		recordNum++;
		if (recordNum == recordCount) {
			isEOF = true;
			recordNum = recordCount-1;
			return False;
		}
		mDataset->next();
		getRowData();

	} catch(DbErrors &e) {
		MDEBUG("***** Caught error [%s]\n", e.getMsg());
		return False;
	}

	return True;
}

/*prev - move to first row of resultset
Output - False on error*/
Bool DBCursor_SQLITE::prev()
{
	ENTER;
	try {
		if (recordCount == 0)
			return False;
		if (isBOF)
			return False;
		isEOF = false; 
		recordNum--;
		if (recordNum == -1) {
			isBOF =True;
			recordNum = 0;
			return False;
		}
		mDataset->prev();
		getRowData();

	} catch(DbErrors &e) {
		MDEBUG("***** Caught error [%s]\n", e.getMsg());
		return False;
	}

	return True;
}

Bool DBCursor_SQLITE::move(int p_record_index)
{
	if (recordCount == 0)
		return False;

	if (p_record_index < 0)
		return False;
	else if (p_record_index > recordCount - 1)
		return False;

	isBOF = False;
	isEOF = False;

	if (!mDataset -> seek(p_record_index))
		return False;

	recordNum = p_record_index;
	getRowData();

	return True;
}


/*getFieldsInformation - get column names, types, and info
Output: False on error*/
Bool DBCursor_SQLITE::getFieldsInformation()
{
	ENTER;
	//create an array of columns
	fields = new (nothrow) DBField *[fieldCount];

	Fields *t_fields_object;
	t_fields_object = mDataset -> get_fields_object();

	for(int i = 0 ; i < fieldCount; i++) {
		DBField *tfield = new (nothrow) DBField();
		fields[i] = tfield;
		
		const char *name = mDataset->fieldName(i);
		if(name) {
			MDEBUG("Field name=[%s]\n", name);

			const field_value fv = mDataset->get_field_value_by_index(i);

			// OK-2008-10-29 : [[Bug 6588]] - Get the field type from the props structure instead of from the field value,
			// because we are returning the declared column type rather than the actual type of the data in the column.
			field_prop t_props;
			t_props = (*t_fields_object)[i].props;

			fType type;
			type = t_props . type;

			///////////////////
			// TYPE
			if (strlen(name) > F_NAMESIZE -6)
				strncpy(tfield->fieldName, name, F_NAMESIZE-6);
			else
				strcpy(tfield->fieldName, name);

			switch(type) {
				case ft_Short :
				case ft_UShort :
					MDEBUG0("Type: smallint\n");
					tfield->fieldType = FT_SMALLINT;
					break;
				case ft_Long :
				case ft_ULong :
					MDEBUG0("Type: integer\n");
					tfield->fieldType = FT_INTEGER;
					break;
				case ft_Float:
					MDEBUG0("Type: float\n");
					tfield->fieldType = FT_FLOAT;
					break;
				case ft_Double:
				case ft_LongDouble:
					MDEBUG0("Type: double\n");
					tfield->fieldType = FT_DOUBLE;
					break;
				case ft_Object :
					if (m_enable_binary)
						tfield -> fieldType = FT_BLOB;
					else
						tfield -> fieldType = FT_STRING;
					break;
				case ft_String :
				default:
					MDEBUG0("Type: string\n");
					tfield->fieldType = FT_STRING;
					break;
			} 

			//get field number
			tfield->fieldNum = i+1;

			//get max length
			tfield->maxlength = MAX_BYTES_PER_ROW;

			//TODO: get column characteristics
			tfield->isAutoIncrement = 0;
			tfield->isPrimaryKey = 0;
			tfield->isUnique = 0;
			tfield->isNotNull = 0;

			// OK-2009-05-26: [[Bug 8065]] - Field data must start out as NULL so we can tell
			// whether or not it needs to be freed.
			tfield -> data = NULL;

		}
	}
	return True;
}


/*getRowData - Retrieve the data from the current row.
Output: False on error*/
Bool DBCursor_SQLITE::getRowData()
{
	ENTER;

	Bool ret = true;

	try {
		for(int i = 0; i < fieldCount; i++) {

			const field_value fv = mDataset->get_field_value_by_index(i);

			// OK-2009-05-26: [[Bug 8065]] - Clear out any data from previous records, otherwise we no longer 
			// have a pointer to it and the memory is lost.
			if (fields[i] -> data != NULL)
			{
				delete fields[i] -> data;
				fields[i] -> data = NULL;
			}

			//distinguish between empty column and null column
			fields[i]->isNull = fv.get_isNull();
			
			if(!fields[i]->isNull) {
				
				std::string tmp = fv.get_asString();
				
				// MW-2014-01-29: [[ Sqlite382 ]] If the field type is BLOB and we aren't in binary mode,
				//   we must decode the data; otherwise we just use the contents directly.
				if (fv.get_fType() == ft_Object && !m_enable_binary)
				{
					// This code has moved from libsqlite/sqlitedataset.cpp:callback().
					int bufsize;
					bufsize = tmp.size();
					
					char *mybuff;
					mybuff = (char *)malloc(bufsize);
					memset(mybuff,0,bufsize);
					bufsize = sqlite_decode_binary((const unsigned char *)tmp.c_str(), bufsize, ( unsigned char *)mybuff, bufsize);
					if (bufsize == -1)
					{
						fields[i] -> data = NULL;
						fields[i] -> dataSize = 0;
						fields[i] -> isNull = True;
						free(mybuff);
					}
					else
					{
						fields[i] -> data = mybuff;
						fields[i] -> dataSize = bufsize;
					}
				}
				else
				{
					int bufsize = tmp.size();
					fields[i]->data = new (nothrow) char[tmp.size() + 1];

					memset(fields[i]->data,0,bufsize+1);
					memcpy(fields[i]->data, tmp.data(), tmp.size());
					fields[i]->dataSize = tmp.size();
				}
				
				// OK-2009-05-26: [[Bug 8065]] - Because we copied the data in the line above,
				// We have to set FreeBuffer otherwise it will not be freed when the cursor is closed,
				// leading to memory leaks.
				fields[i] -> freeBuffer = True;
			}

			MDEBUG("getRowData() -- fv=[%s]\n", fields[i]->data);
		}

	} catch(DbErrors &e) {
		ret = False;
	}

	return ret;
}
