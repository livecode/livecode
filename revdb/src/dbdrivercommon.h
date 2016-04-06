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

// Common exports for drivers, containing default implementations of abstract classes exported by revDB
#include "dbdriver.h"
#include <stdlib.h>

enum
{
	STATE_NONE,
	STATE_INTEGER,
	STATE_MARKER,
	STATE_QUOTED_STRING,
	STATE_QUOTED_STRING_ESCAPE
};

//class for a database field in a row
class DBField
{
public:
	DBField();
	virtual ~DBField();
	char fieldName[F_NAMESIZE];
	unsigned int fieldNum;
	unsigned long int maxlength;
	signed long int dataSize;
	char *data;
	DBFieldType fieldType;
	Bool freeBuffer;
	Bool isAutoIncrement;
	Bool isPrimaryKey;
	Bool isUnique;
	Bool isNotNull;
	Bool isNull;
};

struct PlaceholderMap
{
	int length;
	int *elements;
};

struct QueryMetadata
{
	DBString *arguments;
	int argument_count;
	void *connection;
};

enum cursor_type_t
{
	kCursorTypeStatic,
	kCursorTypeEmulated,
	kCursorTypeForward
};

///////////////////////////////////////////////////////////////////////////////

// Simple extensible buffer class.
// To use:
//   DBBuffer t_buffer;
//
//   // I want to append 100 bytes so:
//   if (!t_buffer . ensure(100))
//     .. error ..
//   // Buffer now has 100 bytes available beyond the frontier so:
//   do_processing_requiring_100_bytes(t_buffer . getFrontier());
//   t_buffer . advance(100);
//
//   // I need to pass the pointer to somewhere else so I 'grab' it:
//   pass_to_thing_that_adopts_pointer(t_buffer . grab());

class DBBuffer
{
public:
	DBBuffer(void);
	DBBuffer(int p_capacity);
	~DBBuffer(void);

	// Ensure that there is at least p_amount bytes beyond the frontier.
	bool ensure(int p_amount);

	// Take ownership of the buffer's backing-store.
	char *grab(void);

	// Borrow the buffer's backing-store 
	const char *borrow(void);

	// Advance the frontier by count bytes
	void advance(int p_count);

	// Append the given bytes to the buffer
	bool append(const char *p_bytes, int p_length);

	// Get a pointer to next position
	char *getFrontier(void);

	int getSize(void);

private:
	char *m_base, *m_frontier, *m_limit;
};

inline DBBuffer::DBBuffer(void)
	: m_base(NULL), m_frontier(NULL), m_limit(NULL)
{
}

inline DBBuffer::DBBuffer(int p_capacity)
	: m_base(NULL), m_frontier(NULL), m_limit(NULL)
{
	ensure(p_capacity);
}

inline DBBuffer::~DBBuffer(void)
{
	if (m_base != NULL)
		free(m_base);
}

inline bool DBBuffer::ensure(int p_amount)
{
	if (m_limit - m_frontier < p_amount)
	{
		char *t_new_base;
		t_new_base = (char *)realloc(m_base, ((m_frontier - m_base) + p_amount + 255) & ~255);
		if (t_new_base == NULL)
			return false;

		m_limit += t_new_base - m_base;
		m_frontier += t_new_base - m_base;
		m_base = t_new_base;
	}

	return true;
}

inline char *DBBuffer::grab(void)
{
	char *t_store;
	t_store = (char *)realloc(m_base, m_frontier - m_base);
	m_base = NULL;
	m_frontier = NULL;
	m_limit = NULL;
	return t_store;
}

inline const char *DBBuffer::borrow(void)
{
	return m_base;
}

inline int DBBuffer::getSize(void)
{
	return m_frontier - m_base;
}

inline char *DBBuffer::getFrontier(void)
{
	return m_frontier;
}

inline void DBBuffer::advance(int p_count)
{
	m_frontier += p_count;
}

inline bool DBBuffer::append(const char *p_bytes, int p_count)
{
	if (!ensure(p_count))
		return false;
	memcpy(m_frontier, p_bytes, p_count);
	advance(p_count);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

class CDBConnection: public DBConnection2
{
public:
	CDBConnection();
	virtual ~CDBConnection();

	DBList *getCursorList();
	int getConnectionType();
	Bool getIsConnected();
	void deleteCursor(int p_id);
	DBCursor *findCursor(int fid);
	int countCursors();
	DBCursor *findCursorIndex(int fid);

	typedef bool (*ProcessQueryCallback)(void *p_context, int p_placeholder, DBBuffer& p_output);
	static bool processQuery(const char *p_input, DBBuffer& p_output, ProcessQueryCallback p_callback, void *p_callback_context);
	bool isLegacy(void);

protected:
	void addCursor(DBCursor *newcursor);
	void closeCursors();

	void errorMessageSet(const char *p_message);

	Bool isConnected;
	int connectionType;
	DBList cursorlist;

	char *m_error;
};


// Base Class for database cursor of mySQL, Oracle, ...
class CDBCursor: public DBCursor3
{
public:
 	CDBCursor();

	virtual Bool getIsOpen();

	virtual Bool IsError();
	virtual char* getErrorMessage();

	virtual int	getFieldCount();
	virtual int	getRecordCount();
	virtual	int	getRecordNumber();
	virtual	Bool getEOF();
	virtual	Bool getBOF();

	virtual DBFieldType getFieldType(unsigned int fieldnum);
	virtual unsigned int getFieldLength(unsigned int fieldnum);
	virtual char* getFieldName(unsigned int fieldnum);
	virtual char* getFieldDataBinary(unsigned int fieldnum, unsigned int &fdlength);
	virtual char* getFieldDataString(unsigned int fieldnum);
	virtual Bool getFieldIsNull(unsigned int fieldnum);
	virtual	DBConnection* getConnection();
	int getVersion(void) { return 3;}
	virtual int move(int p_record_index);

protected:
	virtual void FreeFields();
	Bool isBOF; 
	Bool isEOF;
	Bool isTransaction;
	int recordNum;
	int recordCount;
	int fieldCount;
	DBField **fields;
	DBConnection *connection;
	int maxrows;
	Bool cacherows;
};
