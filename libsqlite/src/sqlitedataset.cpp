/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project:SQLiteDataset C++ Dynamic Library
 * Module: SQLiteDataset class realisation file
 * Author: Leo Seib      E-Mail: leoseib@lycos.de 
 * Begin: 5/04/2002
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **********************************************************************/


#include <iostream>
#include <string>

#include <cstring>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <sqlitedataset/sqlitedataset.h>

typedef int (*MYPROC)(void *,int,const void *,int,const void *);

extern int sqlite_decode_binary(const unsigned char * in, int in_bufsize, unsigned char *out, int out_bufsize);
//**collation callback

// case-insensitive strstrs implementation
static const char *stristr(const char *haystack, const char *needle)
{
	if ( !*needle )
	{
		return haystack;
	}
	for ( ; *haystack; ++haystack )
	{
		if ( toupper(*haystack) == toupper(*needle) )
		{
			/*
			 * Matched starting char -- loop through remaining chars.
			 */
			const char *h, *n;
			for ( h = haystack, n = needle; *h && *n; ++h, ++n )
			{
				if ( toupper(*h) != toupper(*n) )
				{
					break;
				}
			}
			if ( !*n ) /* matched all of 'needle' to null termination */
			{
				return haystack; /* return the start of the match */
			}
		}
	}
	return 0;
}

static void lpadFunc(sqlite3_context* sqlcon, int nargs, sqlite3_value** argv)
{

    char * str;
    char * svalue;

   if ((str = (char *)sqlite3_value_text(argv[0])) == (char *)NULL) {
     sqlite3_result_null(sqlcon);
     return;
      }

    if (SQLITE_INTEGER != sqlite3_value_type(argv[1]) ) {
      sqlite3_result_null(sqlcon);
      return;
    }

    svalue= (char *)sqlite3_value_text(argv[2]);

  int padlen = sqlite3_value_int(argv[1]);
  int replen = strlen(str);
  char * padstr = (char *)malloc(sizeof(char) * padlen+1);
  memset(padstr,0,padlen+1);
  memset(padstr,(char)*svalue,padlen);
  memcpy(padstr+(padlen-replen),str,replen);
 

  sqlite3_result_text(sqlcon, padstr, -1, SQLITE_TRANSIENT);
  free(padstr);
} 
	
int collateCallback(void * first, int len1, const void *str1, int len2, const void * str2)
{
   if( *((int *)str1) > *((int *)str2) )
	   return 1;
   else
	   if( *((int *)str1)< *((int *)str2))
		   return -1;
	   else
           return 0;
}

#ifdef CUSTOM_COLLATION
void registerCollation( void * first, sqlite3* sqlconn, int textRep, const char * colName)
{
    MYPROC procADD;

	std::string libfile("col");
	libfile.append(colName);
#ifdef _WINDOWS
	libfile.append(".dll");
#else
	libfile.append(".so");
#endif

#ifdef _WINDOWS
	HINSTANCE hinst = LoadLibraryA(libfile.c_str());
#else
	void *hinst = dlopen(libfile.c_str(),RTLD_NOW);
#endif

  if(hinst != NULL)
  {
#ifdef _WINDOWS
	  procADD = (MYPROC)GetProcAddress(hinst, colName);
#else
    procADD = (MYPROC) dlsym(hinst, colName);
#endif
	if( procADD != NULL) {

	 int scollate = sqlite3_create_collation(
		sqlconn,
		colName,
		SQLITE_UTF8,
		NULL,
		procADD
		);
	}
  }
}
#endif

void SanitizeHeaders(result_set *r,int ncols)
{
  for( int i=0; i<ncols; i++)
  {
	  string colname = r->record_header[i].name;
	  int tpos = colname.find(".");
	  if( tpos != std::string::npos)
	  {
		  for( int j=0; j<ncols; j++)
		  {
            if( j != i )
			{
				if(1 == 0)
				{
				}
			
			}
		  }
		  r->record_header[i].name = "stuff here";
	  }
  }
}
//************* Callback function ***************************

int callback(void* res_ptr,int ncol, char** reslt,char** cols){

	result_set* r = (result_set*)res_ptr;
	int sz = r->records.size();


	//if (!r->record_header.size()) 
	//{
	//	for (int i=0; i < ncol; i++) {
	//		r->record_header[i].name = cols[i];
	//	}

	//	if( cols[ncol] != 0 )
	//	{
	//		for (int j=0; j< ncol; j++)
	//		{
	//		 if (cols[j + ncol] != NULL && (strstr(cols[j + ncol], "BLOB") != NULL || strstr(cols[j + ncol], "blob") != NULL))
	//			 r -> record_header[j] . type = ft_Object;
	//		 else
	//			 r -> record_header[j] . type = ft_String;
	//		}
	//	}
	//}


	sql_record rec;

	// OK-2010-03-17: [[Bug 8671]] - Don't re-use the same field_value for each iteration, because once we've
	// set isNull, we can't unset it, leading to columns incorrectly being identified as null.
	//field_value v;
	//char * outstr;
	int decres=0;

	if (reslt != NULL) 
	{
		for (int i=0; i<ncol; i++)
		{ 
			// OK-2010-03-17: [[Bug 8671]] - Create it here instead
			field_value v;
			if (reslt[i] == NULL)
			{
				v.set_asString("");
				v.set_isNull();
			}
			else 
			{
				// MM-2012002-29: [[ BUG 1022 ]] - Ignore typing of the actual column and always assume data is string.
				// The type of a SQLite column is determined by its contents rather than its schema (which is for guidance).
				v.set_asString(reslt[i]);
				switch( r->record_header[i].type)
				{
				case ft_Boolean:
					//v.set_asBool(v.get_asBool());
					break;
				case ft_Char:
					//v.set_asChar(v.get_asChar());
					break;
				case ft_WChar:
					break;
				case ft_WideString:
					break;
				case ft_Short:
					break;
				case ft_UShort:
					break;
				case ft_Long:
					//v.set_asLong(v.get_asLong());
					break;
				case ft_ULong:
					break;
				case ft_Float:
					//v.set_asFloat(v.get_asFloat());
					break;
				case ft_Double:
					//v.set_asDouble(v.get_asDouble());
					break;
				case ft_Object:
					{
						char *mybuff;
						int bufsize;
						bufsize = v.get_asString().size();

						mybuff = (char *)malloc(bufsize);
						memset(mybuff,0,bufsize);
						bufsize = sqlite_decode_binary((const unsigned char *)v.get_asString().c_str(), bufsize, ( unsigned char *)mybuff, bufsize);
						if (bufsize == -1)
						{
							v.set_asString("");
							v.set_isNull();
						}
						else
							v.set_asString((const char *)mybuff,bufsize);
						
						free(mybuff);
					}
					break;
				case ft_LongDouble:
					break;

				} 
				// v.set_asString(reslt[i]);
			}
			rec[i] = v;
		}
		r->records[sz] = rec;
		return 0; 
	}
	else
	{
		return 0;
	}
}


//************* SqliteDatabase implementation ***************

SqliteDatabase::SqliteDatabase()
{

	active = false;
	_in_transaction = false;		// for transaction

	error = "Unknown database error"; //S_NO_CONNECTION;
	host = "localhost";
	port = "";
	db = "sqlite.db";
	login = "root";
	passwd, "";
}

SqliteDatabase::~SqliteDatabase()
{
	disconnect();
}


Dataset* SqliteDatabase::CreateDataset() const {
	return new SqliteDataset((SqliteDatabase*)this); 
}

int SqliteDatabase::status(void) {
  if (active == false) return DB_CONNECTION_NONE;
  return DB_CONNECTION_OK;
}

int SqliteDatabase::setErr(int err_code, const char * qry){
  switch (err_code) {
  case SQLITE_OK: error = "Successful result";
    break;
  case SQLITE_ERROR: error = "SQL error or missing database";
    break;
  case SQLITE_INTERNAL: error = "An internal logic error in SQLite";
    break;
  case SQLITE_PERM: error = "Access permission denied";
    break;
  case SQLITE_ABORT: error = "Callback routine requested an abort";
    break;
  case SQLITE_BUSY: error = "The database file is locked";
    break;
  case SQLITE_LOCKED: error = "A table in the database is locked";
    break;
  case SQLITE_NOMEM: error = "A malloc() failed";
    break;
  case SQLITE_READONLY: error = "Attempt to write a readonly database";
    break;
  case SQLITE_INTERRUPT: error = "Operation terminated by sqlite_interrupt()";
    break;
  case  SQLITE_IOERR: error = "Some kind of disk I/O error occurred";
    break;
  case  SQLITE_CORRUPT: error = "The database disk image is malformed";
    break;
  case SQLITE_NOTFOUND: error = "(Internal Only) Table or record not found";
    break;
  case SQLITE_FULL: error = "Insertion failed because database is full";
    break;
  case SQLITE_CANTOPEN: error = "Unable to open the database file";
    break;
  case SQLITE_PROTOCOL: error = "Database lock protocol error";
    break;
  case SQLITE_EMPTY:  error = "(Internal Only) Database table is empty";
    break;
  case SQLITE_SCHEMA: error = "The database schema changed";
    break;
  case SQLITE_TOOBIG: error = "Too much data for one row of a table";
    break;
  case SQLITE_CONSTRAINT: error = "Abort due to constraint violation";
    break;
  case SQLITE_MISMATCH:  error = "Data type mismatch";
    break;
  default : error = "Undefined SQLite error";
  }
  if (qry != NULL)
  {
	error += "\nQuery: ";
	error += qry;
	error += "\n";
  }
  return err_code;
}

const char *SqliteDatabase::getErrorMsg() {
   return error.c_str();
}

int SqliteDatabase::connect()
{
  disconnect();
  int result = setErr(sqlite3_open(db.c_str(),&conn), NULL);
  if (!result)
  {
    char* err=NULL;
    if (setErr(sqlite3_exec(getHandle(),"PRAGMA empty_result_callbacks=ON",NULL,NULL,&err),"PRAGMA empty_result_callbacks=ON") != SQLITE_OK)
      throw DbErrors(getErrorMsg());

	if (setErr(sqlite3_exec(getHandle(),"PRAGMA SHOW_DATATYPES=ON",NULL,NULL,&err),"PRAGMA SHOW_DATATYPES=ON") != SQLITE_OK)
      throw DbErrors(getErrorMsg());

	//load any  collations
#ifdef CUSTOM_COLLATION
	int dynLoadCollation = sqlite3_collation_needed(conn,NULL,registerCollation);
#endif

	// create custom functions
	int dynCreate = sqlite3_create_function(conn,"lpad",3,SQLITE_UTF8,NULL,lpadFunc,NULL,NULL);
   
    active = true;
    return DB_CONNECTION_OK;
  }
  else
	  throw DbErrors(getErrorMsg());

  return DB_CONNECTION_NONE;
};

void SqliteDatabase::disconnect(void) {
  if (active == false) return;
  sqlite3_close(conn);
  active = false;
};

int SqliteDatabase::create() {
  return connect();
};

int SqliteDatabase::drop() {
  if (active == false) throw DbErrors("Can't drop database: no active connection...");
  disconnect();
  if (!unlink(db.c_str())) {
     throw DbErrors("Can't drop database: can't unlink the file %s,\nError: %s",db.c_str());
     }
  return DB_COMMAND_OK;
};


long SqliteDatabase::nextid(const char* sname) {
  if (!active) return DB_UNEXPECTED_RESULT;
  int id;
  result_set res;
  char sqlcmd[512];
  sprintf(sqlcmd,"select nextid from %s where seq_name = '%s'",sequence_table.c_str(), sname);
  if (last_err = sqlite3_exec(getHandle(),sqlcmd,&callback,&res,NULL) != SQLITE_OK) {
    return DB_UNEXPECTED_RESULT;
    }
  if (res.records.size() == 0) {
    id = 1;
    sprintf(sqlcmd,"insert into %s (nextid,seq_name) values (%d,'%s')",sequence_table.c_str(),id,sname);
    if (last_err = sqlite3_exec(conn,sqlcmd,NULL,NULL,NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;
  }
  else {
    id = res.records[0][0].get_asInteger()+1;
    sprintf(sqlcmd,"update %s set nextid=%d where seq_name = '%s'",sequence_table.c_str(),id,sname);
    if (last_err = sqlite3_exec(conn,sqlcmd,NULL,NULL,NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;    
  }
  return DB_UNEXPECTED_RESULT;
}


// methods for transactions
// ---------------------------------------------
void SqliteDatabase::start_transaction() {
  if (active) {
    sqlite3_exec(conn,"begin",NULL,NULL,NULL);
    _in_transaction = true;
  }
}

void SqliteDatabase::commit_transaction() {
  if (active) {
    sqlite3_exec(conn,"commit",NULL,NULL,NULL);
    _in_transaction = false;
  }
}

void SqliteDatabase::rollback_transaction() {
  if (active) {
    sqlite3_exec(conn,"rollback",NULL,NULL,NULL);
    _in_transaction = false;
  }  
}



//************* SqliteDataset implementation ***************

SqliteDataset::SqliteDataset():Dataset() {
  haveError = false;
  db = NULL;
}


SqliteDataset::SqliteDataset(SqliteDatabase *newDb):Dataset(newDb) {
  haveError = false;
  db = newDb;
}

 SqliteDataset::~SqliteDataset(){
//   if (errmsg) sqlite_free_table(&errmsg);
 }



//--------- protected functions implementation -----------------//

sqlite3* SqliteDataset::handle(){
  if (db != NULL){
  SqliteDatabase *mydb = (SqliteDatabase *)db;
    return mydb->getHandle();
      }
  else return NULL;
}

void SqliteDataset::make_query(StringList &_sql) {
  string query;

 try {

  if (autocommit) db->start_transaction();


  if (db == NULL) throw DbErrors("No Database Connection");


  for (list<string>::iterator i =_sql.begin(); i!=_sql.end(); i++) {
	query = *i;
	char* err=NULL; 
	Dataset::parse_sql(query);
	if (db->setErr(sqlite3_exec(this->handle(),query.c_str(),NULL,NULL,&err),query.c_str())!=SQLITE_OK) {
	  throw DbErrors(db->getErrorMsg());
	}
  } // end of for


  if (db->in_transaction() && autocommit) db->commit_transaction();

  active = true;
  ds_state = dsSelect;		
  refresh();

 } // end of try
 catch(...) {
  if (db->in_transaction()) db->rollback_transaction();
  throw;
 }

}


void SqliteDataset::make_insert() {
  make_query(insert_sql);
  last();
}


void SqliteDataset::make_edit() {
  make_query(update_sql);
}


void SqliteDataset::make_deletion() {
  make_query(delete_sql);
}


void SqliteDataset::fill_fields() {
  //cout <<"rr "<<result.records.size()<<"|" << frecno <<"\n";
  if ((db == NULL) || (result.record_header.size() == 0) || (result.records.size() < frecno)) return;
  if (fields_object->size() == 0) // Filling columns name
    for (int i = 0; i < result.record_header.size(); i++) {
      (*fields_object)[i].props = result.record_header[i];
      (*edit_object)[i].props = result.record_header[i];
    }

  //Filling result
  if (result.records.size() != 0) {
   for (int i = 0; i < result.records[frecno].size(); i++){
    (*fields_object)[i].val = result.records[frecno][i];
    (*edit_object)[i].val = result.records[frecno][i];
   }
  }
  else
   for (int i = 0; i < result.record_header.size(); i++){
    (*fields_object)[i].val = "";
    (*edit_object)[i].val = "";
   }    

}

static int sqlite3_query_exec(sqlite3 *db, const char *zSql, sqlite3_callback xCallback, void *pArg, char **pzErrMsg)
{
	int rc = SQLITE_OK;
	const char *zLeftover;
	sqlite3_stmt *pStmt = 0;
	char **azCols = 0;

	int nRetry = 0;
	int nChange = 0;

	if( zSql==0 ) return SQLITE_OK;
	while( (rc==SQLITE_OK || (rc==SQLITE_SCHEMA && (++nRetry)<2)) && zSql[0] )
	{
		int nCol;
		char **azVals = 0;

		pStmt = 0;
		rc = sqlite3_prepare(db, zSql, -1, &pStmt, &zLeftover);
		if(rc != SQLITE_OK )
			continue;

		if( !pStmt )
		{
			/* this happens for a comment or white-space */
			zSql = zLeftover;
			continue;
		}
	
		int nCallback;
		nCallback = 0;
		nCol = sqlite3_column_count(pStmt);
		azCols = (char **)malloc(3*nCol*sizeof(const char *) + 1);
		memset(azCols, 0, 3*nCol*sizeof(const char *) + 1);

		if( 0 == nCallback )
		{
			for(int i = 0; i < nCol; i++)
			{
				azCols[i] = (char *)sqlite3_column_name(pStmt, i);
			}
			nCallback++;
		}

		if( rc == SQLITE_OK )
		{
			azVals = &azCols[nCol * 2];
			for(int i = 0; i < nCol; i++)
			{
				azCols[i + nCol] = (char *)sqlite3_column_decltype(pStmt, i);
			}
		}

		result_set* r = (result_set*)pArg;
		int sz = r->records.size();

		if (!r->record_header.size()) 
		{
			for (int i=0; i < nCol; i++)
			{
				r->record_header[i].name = azCols[i];
				r->record_header[i].type = ft_String;
			}

			if( nCol != 0 && azCols[nCol] != 0 )
			{
				for (int j=0; j< nCol; j++)
				{
					// OK-2008-10-29 : [[Bug 6588]] - Integer and real columns need to also be checked for
					// IM-2009-08-27 : [[Bug 8112]] - switch to case-insensitive search for type names
					if (azCols[j + nCol] != NULL && (stristr(azCols[j + nCol], "blob") != NULL))
						r -> record_header[j] . type = ft_Object;
					else if (azCols[j + nCol] != NULL && (stristr(azCols[j + nCol], "integer") != NULL))
						r -> record_header[j] . type = ft_Long;
					else if (azCols[j + nCol] != NULL && (stristr(azCols[j + nCol], "real") != NULL))
						r -> record_header[j] . type = ft_Float;
					else
						r -> record_header[j] . type = ft_String;
				}
				
			}
		}

		if( azCols==0 )
		{
			goto exec_out;
		}

		while( 1 )
		{
			int i;
			rc = sqlite3_step(pStmt);

			/* Invoke the callback function if required */
			if( xCallback && SQLITE_ROW==rc )
			{
			/*	if( 0==nCallback )
				{
					for(i=0; i<nCol; i++)
					{
						azCols[i] = (char *)sqlite3_column_name(pStmt, i);
					}
					nCallback++;
				}*/
					azVals = &azCols[nCol * 2];
					for(i=0; i<nCol; i++)
					{
						azVals[i] = (char *)sqlite3_column_text(pStmt, i);
					}
				if( xCallback(pArg, nCol, azVals, azCols) )
				{
					rc = SQLITE_ABORT;
					goto exec_out;
				}
			}

			if( rc!=SQLITE_ROW )
			{
				rc = sqlite3_finalize(pStmt);
				pStmt = 0;
				if( rc!=SQLITE_SCHEMA )
				{
					nRetry = 0;
					zSql = zLeftover;
					while( isspace((unsigned char)zSql[0]) ) zSql++;
				}
				break;
			}
		}

		free(azCols);
		azCols = 0;
	}

exec_out:
	if( pStmt ) sqlite3_finalize(pStmt);
	if( azCols ) free(azCols);

	if( rc!=SQLITE_OK && rc==sqlite3_errcode(db) && pzErrMsg )
	{
		*pzErrMsg = (char *)sqlite3_malloc(1+strlen(sqlite3_errmsg(db)));
		if( *pzErrMsg )
		{
			strcpy(*pzErrMsg, sqlite3_errmsg(db));
		}
	}
	else if( pzErrMsg )
	{
		*pzErrMsg = 0;
	}

	return rc;
}

#if 0
static int my_sqlite3_exec(
  sqlite3 *db,                /* The database on which the SQL executes */
  const char *zSql,           /* The SQL to be executed */
  sqlite3_callback xCallback, /* Invoke this callback routine */
  void *pArg,                 /* First argument to xCallback() */
  char **pzErrMsg             /* Write error messages here */
){
  int rc = SQLITE_OK;
  const char *zLeftover;
  sqlite3_stmt *pStmt = 0;
  char **azCols = 0;

  int nRetry = 0;
  int nChange = 0;
  int nCallback;

  if( zSql==0 ) return SQLITE_OK;
  while( (rc==SQLITE_OK || (rc==SQLITE_SCHEMA && (++nRetry)<2)) && zSql[0] ){
    int nCol;
    char **azVals = 0;

    pStmt = 0;
    rc = sqlite3_prepare(db, zSql, -1, &pStmt, &zLeftover);
    if( rc!=SQLITE_OK ){
      continue;
    }
    if( !pStmt ){
      /* this happens for a comment or white-space */
      zSql = zLeftover;
      continue;
    }

    db->nChange += nChange;
    nCallback = 0;

    nCol = sqlite3_column_count(pStmt);
    azCols = sqliteMalloc(2*nCol*sizeof(const char *) + 1);
    if( azCols==0 ){
      goto exec_out;
    }

    while( 1 ){
      int i;
      rc = sqlite3_step(pStmt);

      /* Invoke the callback function if required */
      if( xCallback && (SQLITE_ROW==rc || 
          (SQLITE_DONE==rc && !nCallback && db->flags&SQLITE_NullCallback)) ){
        if( 0==nCallback ){
          for(i=0; i<nCol; i++){
            azCols[i] = (char *)sqlite3_column_name(pStmt, i);
          }
          nCallback++;
        }
        if( rc==SQLITE_ROW ){
          azVals = &azCols[nCol];
          for(i=0; i<nCol; i++){
            azVals[i] = (char *)sqlite3_column_text(pStmt, i);
          }
        }
        if( xCallback(pArg, nCol, azVals, azCols) ){
          rc = SQLITE_ABORT;
          goto exec_out;
        }
      }

      if( rc!=SQLITE_ROW ){
        rc = sqlite3_finalize(pStmt);
        pStmt = 0;
        if( db->pVdbe==0 ){
          nChange = db->nChange;
        }
        if( rc!=SQLITE_SCHEMA ){
          nRetry = 0;
          zSql = zLeftover;
          while( isspace((unsigned char)zSql[0]) ) zSql++;
        }
        break;
      }
    }

    sqliteFree(azCols);
    azCols = 0;
  }

exec_out:
  if( pStmt ) sqlite3_finalize(pStmt);
  if( azCols ) sqliteFree(azCols);

  rc = sqlite3ApiExit(0, rc);
  if( rc!=SQLITE_OK && rc==sqlite3_errcode(db) && pzErrMsg ){
    *pzErrMsg = sqlite3_malloc(1+strlen(sqlite3_errmsg(db)));
    if( *pzErrMsg ){
      strcpy(*pzErrMsg, sqlite3_errmsg(db));
    }
  }else if( pzErrMsg ){
    *pzErrMsg = 0;
  }

  assert( (rc&db->errMask)==rc );
  return rc;
}
#endif

//------------- public functions implementation -----------------//

int SqliteDataset::exec(const string &sql) {
  if (!handle()) throw DbErrors("No Database Connection");
  int res;
  exec_res.record_header.clear();
  exec_res.records.clear();
  if(res = db->setErr(sqlite3_query_exec(handle(),sql.c_str(),&callback,&exec_res,&errmsg),sql.c_str()) == SQLITE_OK)
    return res;
  else
    {
      throw DbErrors(db->getErrorMsg());
    }
}

int SqliteDataset::exec() {
	return exec(sql);
}

const void* SqliteDataset::getExecRes() {
  return &exec_res;
}


bool SqliteDataset::query(const char *query)
{
	if (db == NULL) throw DbErrors("Database is not Defined");

	//   (SqliteDatabase *)(db)
	// if(((SqliteDatabase *)(db)->getHandle()) == NULL) throw DbErrors("No Database Connection");
	std::string qry = query;
	//   int fs = qry.find("select");
	// int fS = qry.find("SELECT");
	// if (!( fs >= 0 || fS >=0))                                 
	//    throw DbErrors("MUST be select SQL!"); 

	close();
	// std::string lquery = "PRAGMA SHOW_DATATYPES=ON;";
	// lquery.append(query);

	int t_query_result;
	t_query_result = sqlite3_query_exec(handle(), query, &callback, &result, &errmsg);
	db -> setErr(t_query_result, query);
	if (t_query_result == SQLITE_OK)
	{
		active = true;
		ds_state = dsSelect;
		this->first();
		return true;
	}
	else
	{
		// OK-2010-03-01: [[Bug 8351]] - If SQLITE_ERROR is returned, don't use generic error message,
		// instead use the string that SQLite returned.
		if (t_query_result == SQLITE_ERROR)
			db -> setErrDirect(errmsg);
		
		throw DbErrors(db->getErrorMsg());
	}
}

bool SqliteDataset::query(const string &q){
  return query(q.c_str());
}

void SqliteDataset::open(const string &sql) {
	set_select_sql(sql);
	open();
}

void SqliteDataset::open() {
  if (select_sql.size()) {
    query(select_sql.c_str()); 
  }
  else {
    ds_state = dsInactive;
  }
}


void SqliteDataset::close() {
  Dataset::close();
  result.record_header.clear();
  result.records.clear();
  edit_object->clear();
  fields_object->clear();
  ds_state = dsInactive;
  active = false;
}


void SqliteDataset::cancel() {
  if ((ds_state == dsInsert) || (ds_state==dsEdit))
    if (result.record_header.size()) ds_state = dsSelect;
    else ds_state = dsInactive;
}


int SqliteDataset::num_rows() {
  return result.records.size();
}


bool SqliteDataset::eof() {
  return feof;
}


bool SqliteDataset::bof() {
  return fbof;
}


void SqliteDataset::first() {
  Dataset::first();
  this->fill_fields();
}

void SqliteDataset::last() {
  Dataset::last();
  fill_fields();
}

void SqliteDataset::prev(void) {
  Dataset::prev();
  fill_fields();
}

void SqliteDataset::next(void) {
  Dataset::next();
  if (!eof()) 
      fill_fields();
}


bool SqliteDataset::seek(int pos) {
  if (ds_state == dsSelect) {
    Dataset::seek(pos);
    fill_fields();
    return true;	
    }
  return false;
}



long SqliteDataset::nextid(const char *seq_name) {
  if (handle()) return db->nextid(seq_name);
  else return DB_UNEXPECTED_RESULT;
}

