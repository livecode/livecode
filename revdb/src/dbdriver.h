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

///////////////////////////////////////////////////////////////////////////////
//
//  Abstract interfaces exported by revDB
//

#ifndef __DBDRIVER__
#define __DBDRIVER__

#include <stdio.h>
#include <string.h>
#include <vector>


#if defined(_WINDOWS) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define _DEBUG_MEMORY
#undef new
#undef delete

inline void *operator new(size_t size, std::nothrow_t, const char *fnm, int line) throw () {return _malloc_dbg(size, _NORMAL_BLOCK, fnm, line);}
inline void *operator new[](size_t size, std::nothrow_t, const char *fnm, int line) throw () {return _malloc_dbg(size, _NORMAL_BLOCK, fnm, line);}

inline void *operator new(size_t, void *p, const char *, long)
{
	return p;
}

#define new(...) new(__VA_ARGS__, __FILE__, __LINE__ )
#define delete delete
#endif


#define MAXCOLBUF 65535
#define MAXROWNUM 32767
#define RAWSIZE 64000
#define LONG_CHUNK_SIZE 64000

#define align(x) (((x) / sizeof(char*) + 1) * sizeof(char*))

char *longtostring(long inValue);

using namespace std ;

#ifndef Bool
#define Bool int
#define True 1
#define False 0
#endif

class DBString;

char *StrEscape(char *);
char *StrSkipSpace(char *);
char *StrSkipWord(char *, int);
bool StrNextWordIs(char *, const char *);
char *StrCopyWord(char *, char *, int, int);
char *parse_data(char *cmd, char* data, unsigned int MaxLen);
char *parse_value(char *cmd, DBString* data, DBString *bindargs, unsigned int numargs);

#define F_NAMESIZE 255 

extern const char DBNullValue[];

//list of database column types
enum DBFieldType {
	FT_NULL,
	FT_BIT, 
	FT_CHAR,
	FT_STRING,
	FT_WSTRING,
	FT_BLOB,
	FT_TIMESTAMP,
	FT_DATE,
	FT_TIME,
	FT_DATETIME,
	FT_FLOAT,
	FT_DOUBLE,
	FT_INTEGER,
	FT_SMALLINT,
	FT_WORD,
	FT_BOOLEAN,
	FT_LONG
};

//list of database types
enum DBConnectionType {
	CT_NULL,
	CT_ORACLE,
	CT_MYSQL,
	CT_ODBC,
	CT_FOURD,
	CT_POSTGRESQL,
	CT_VALENTINA,
	CT_SQLITE
};

//groups together DBCursor and DBConnection so we can use them in a linked list. 
//Also generates unique id for use in linked list
class DBObject
{
public:
	DBObject() {id = ++*idcounter;}
	virtual ~DBObject() {};
	unsigned int GetID() {return id;}
	static unsigned int *idcounter;
	unsigned int id;
};


typedef vector<DBObject *> DBObjectList;

class DBCursor;
class DBConnection;

///////////////////////////////////////////////////////////////////////////////

class DBList
{
	public:
		~DBList();
		void add(DBObject *newdbnode);
		void clear();
		DBObjectList *getList();
		Bool erase(const unsigned int fid);
		DBObject *find(const unsigned int fid);
		int getsize();
		DBObject *findIndex(const int tindex);
		void show();
	protected:
		DBObjectList dblist;

};

///////////////////////////////////////////////////////////////////////////////
//
//  DBConnection abstract interface
//

class DBConnection: public DBObject
{
public:
	virtual ~DBConnection() {};

	virtual Bool getIsConnected() = 0; 
	virtual int getConnectionType() = 0; 
	virtual Bool IsError() = 0;

	virtual Bool connect(char **args, int numargs) = 0;
	virtual void disconnect() = 0;
	virtual const char *getconnectionstring() = 0;
	virtual	Bool sqlExecute(char *query, DBString *args, int numargs, unsigned int &affectedrows) = 0;
	virtual DBCursor *sqlQuery(char *query, DBString *args, int numargs, int p_rows) = 0;

	virtual void transBegin() = 0;
	virtual void transCommit() = 0;
	virtual void transRollback() = 0;
	virtual char *getErrorMessage(Bool last = False) = 0;

	virtual DBList *getCursorList() = 0; 
	virtual void deleteCursor(int fid) = 0;
	virtual DBCursor *findCursor(int fid) = 0;
	virtual DBCursor *findCursorIndex(int fid) = 0;
	virtual int countCursors() = 0;
	virtual	void getTables(char *buffer, int *bufsize) = 0;

protected:
	virtual void addCursor(DBCursor *newcursor) = 0;
	virtual void closeCursors() = 0;
};

class DBConnection2: public DBConnection
{
public:
	virtual int getVersion(void) = 0;
};



///////////////////////////////////////////////////////////////////////////////

// abstract class for database cursor
class DBCursor: public DBObject
{
public:
	virtual ~DBCursor() {}
	virtual void close() = 0;
	virtual	Bool getIsOpen() = 0;
	virtual DBConnection* getConnection() = 0;

	virtual	Bool IsError() = 0;
	virtual char* getErrorMessage() = 0;

	virtual	Bool getEOF() = 0;
	virtual	Bool getBOF() = 0;
	virtual Bool first() = 0;
	virtual Bool last() = 0;
	virtual Bool next() = 0;
	virtual Bool prev() = 0;

	virtual int getFieldCount()  = 0;
	virtual int getRecordCount()  = 0;
	virtual	int getRecordNumber()  = 0;

	virtual DBFieldType getFieldType(unsigned int fieldnum)  = 0;
	virtual unsigned int getFieldLength(unsigned int fieldnum) = 0;
	virtual char* getFieldName(unsigned int fieldnum) = 0;

	virtual char* getFieldDataBinary(unsigned int fieldnum,unsigned int &fdlength) = 0;
	virtual char* getFieldDataString(unsigned int fieldnum) = 0;
	virtual Bool getFieldIsNull(unsigned int fieldnum) = 0;
};

class DBCursor2: public DBCursor
{
public:
	virtual int getVersion(void) = 0;
};

 class DBCursor3: public DBCursor2
  {
  public:
  	// This method attempts to move to the record of index <p_record_index>.
  	// If the index is out of bounds, or the movement isn't possible (for example if the cursor is forward only), it returns false.
  	virtual Bool move(int p_record_index) = 0;
  };


///////////////////////////////////////////////////////////////////////////////

class DBString
{
public:
	DBString(void);
	DBString(char *somechar);
	DBString(char *somechar, int tlength, Bool tbinary);
	void Set(char *somechar);
	void Set(char *somechar, int tlength, Bool tbinary);

	const char *sptr;
	int length;
	Bool isbinary;
};

inline DBString::DBString(void)
{
	sptr = NULL;
	length = 0;
	isbinary = False;
}

inline DBString::DBString(char *p_string)
{
	Set(p_string);
}

inline DBString::DBString(char *p_string, int p_length, Bool p_binary)
{
	Set(p_string, p_length, p_binary);
}

inline void DBString::Set(char *p_string)
{
	sptr = p_string;
	length = strlen(p_string);
}

inline void DBString::Set(char *p_string, int p_length, Bool p_binary)
{
	sptr = p_string;
	length = p_length;
	isbinary = p_binary;
}


///////////////////////////////////////////////////////////////////////////////

#define DBcallbacks_version 0

struct DBcallbacks
{
    unsigned int version;

    // V0 callbacks
    void *(*load_module)(const char *module);
    void (*unload_module)(void *module);
    void *(*resolve_symbol_in_module)(void *module, const char *symbol);
};

///////////////////////////////////////////////////////////////////////////////

// These are the standard exported functions for all db-drivers. We predeclare
// them here with appropriate visibility so we don't have to do this in all
// driver files.

#if defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_ANDROID)
extern "C" DBConnection *newdbconnectionref() __attribute__((visibility("default")));
extern "C" void releasedbconnectionref(DBConnection *dbref) __attribute__((visibility("default")));
extern "C" void setidcounterref(unsigned int *tidcounter) __attribute__((visibility("default")));
extern "C" void setcallbacksref(DBcallbacks *callbacks) __attribute__((visibility("default")));
#endif

///////////////////////////////////////////////////////////////////////////////

#endif
