/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project: C++ Dynamic Library
 * Module: Dataset abstraction layer realisation file
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



#include <sqlitedataset/dataset.h>


//************* Database implementation ***************

Database::Database() {
  active = false;	// No connection yet
  error = "";//S_NO_CONNECTION;
  host = "";
  port = "";
  db = "";
  login = "";
  passwd = "";
  sequence_table = "db_sequence";
}

Database::~Database() {
  disconnect();		// Disconnect if connected to database
}

int Database::connectFull(const char *newHost, const char *newPort, const char *newDb, const char *newLogin, const char *newPasswd) {
  host = newHost;
  port = newPort;
  db = newDb;
  login = newLogin;
  passwd = newPasswd;
  return connect();
}




//************* Dataset implementation ***************

Dataset::Dataset() {

  db = NULL;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  select_sql = "";

  fields_object = new Fields();

  edit_object = new Fields();
}



Dataset::Dataset(Database *newDb) {

  db = newDb;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  select_sql = "";

  fields_object = new Fields();

  edit_object = new Fields();

}


Dataset::~Dataset() {
  update_sql.clear();
  insert_sql.clear();
  delete_sql.clear();


  delete fields_object;
  delete edit_object;

}


void Dataset::setSqlParams(const char *sqlFrmt, sqlType t, ...) {
  va_list ap;
  char sqlCmd[DB_BUFF_MAX+1];

  va_start(ap, t);
      vsnprintf(sqlCmd, DB_BUFF_MAX-1, sqlFrmt, ap);
  va_end(ap);

   switch (t) {
       case sqlSelect: set_select_sql(sqlCmd);
                       break;
       case sqlUpdate: add_update_sql(sqlCmd);
                       break;
       case sqlInsert: add_insert_sql(sqlCmd);
                       break;
       case sqlDelete: add_delete_sql(sqlCmd);
                       break;
       case sqlExec: sql = sqlCmd;
             	    break;

  }
}



void Dataset::set_select_sql(const char *sel_sql) {
 select_sql = sel_sql;
}

void Dataset::set_select_sql(const string &sel_sql) {
 select_sql = sel_sql;
}


void Dataset::parse_sql(string &sql) {
  string fpattern,by_what;
  int i;
  for (i=0;i< fields_object->size();i++) {
    fpattern = ":OLD_"+(*fields_object)[i].props.name;
    by_what = "'"+(*fields_object)[i].val.get_asString()+"'";
		int idx=0; int next_idx=0;
		while ((idx = sql.find(fpattern,next_idx))>=0) {
		       	   next_idx=idx+fpattern.size();
			       if (sql.length() > (next_idx))
			       if(isalnum(sql[next_idx])  || sql[next_idx]=='_') {
			       	   continue;
			       	}
			      sql.replace(idx,fpattern.size(),by_what); 	
		}//while
    }//for

  for ( i=0;i< edit_object->size();i++) {
    fpattern = ":NEW_"+(*edit_object)[i].props.name;
    by_what = "'"+(*edit_object)[i].val.get_asString()+"'";
		int idx=0; int next_idx=0;
		while ((idx = sql.find(fpattern,next_idx))>=0) {
		       	   next_idx=idx+fpattern.size();
			       if (sql.length() > (next_idx))
			       if(isalnum(sql[next_idx]) || sql[next_idx]=='_') {
			       	   continue;
			       	}
			      sql.replace(idx,fpattern.size(),by_what); 	
			}//while  
  } //for
}


void Dataset::close(void) {
  haveError  = false;
  frecno = 0;
  fbof = feof = true;
  active = false;
}


bool Dataset::seek(int pos) {
  frecno = (pos<num_rows()-1)? pos: num_rows()-1;
  frecno = (frecno<0)? 0: frecno;
  fbof = feof = (num_rows()==0)? true: false;
  return frecno;
}


void Dataset::refresh() {
  int row = frecno;
  if ((row != 0) && active) {
    close();
    open();
    seek(row);
  }
  else open();		
}


void Dataset::first() {
  if (ds_state == dsSelect) {
    frecno = 0;
    feof = fbof = (num_rows()>0)? false : true;
  }
}

void Dataset::next() {
  if (ds_state == dsSelect) {
    fbof = false;
    if (frecno<num_rows()-1) {
      frecno++;
      feof = false;
    } else feof = true;
    if (num_rows()<=0) fbof = feof = true;
  }
}

void Dataset::prev() {
  if (ds_state == dsSelect) {
    feof = false;
    if (frecno) {
      frecno--;
      fbof = false;
    } else fbof = true;
    if (num_rows()<=0) fbof = feof = true;
  }
}

void Dataset::last() {
  if (ds_state == dsSelect) {
    frecno = (num_rows()>0)? num_rows()-1: 0;
    feof = fbof = (num_rows()>0)? false : true;
  }
}

bool Dataset::goto_rec(int pos) {
  if (ds_state == dsSelect) {
    return seek(pos - 1);
  }
  return false;
}


void Dataset::insert() {
   for (int i=0; i<field_count(); i++) {
     (*fields_object)[i].val = "";
     (*edit_object)[i].val = "";
   }
  ds_state = dsInsert;
}


void Dataset::edit() {
  if (ds_state != dsSelect) {
    throw DbErrors("Editing is possible only when query exists!");
  }
  for (unsigned int i=0; i<fields_object->size(); i++) {
       (*edit_object)[i].val = (*fields_object)[i].val; 
  }
  ds_state = dsEdit;
}


void Dataset::post() {
  if (ds_state == dsInsert) make_insert();
  else if (ds_state == dsEdit) make_edit();
}


void Dataset::deletion() {
  if (ds_state == dsSelect) make_deletion();
}


bool Dataset::set_field_value(const char *f_name, const field_value &value) {
  bool found = false;
  if ((ds_state == dsInsert) || (ds_state == dsEdit)) {
      for (unsigned int i=0; i < fields_object->size(); i++) 
	if (str_compare((*edit_object)[i].props.name.c_str(), f_name)==0) {
			     (*edit_object)[i].val = value;
			     found = true;
	}
      if (!found) throw DbErrors("Field not found: %s",f_name);
    return true;
  }
  throw DbErrors("Not in Insert or Edit state");
  //  return false;
}

const field_value Dataset::get_field_value(const char *f_name) {
  if (ds_state != dsInactive) {
    if (ds_state == dsEdit || ds_state == dsInsert){
      for (unsigned int i=0; i < edit_object->size(); i++)
	  
		if (str_compare((*edit_object)[i].props.name.c_str(), f_name)==0) {
	  		return (*edit_object)[i].val;
			}

      throw DbErrors("Field not found: %s",f_name);
       }
    else
      for (unsigned int i=0; i < fields_object->size(); i++) 
			if (str_compare((*fields_object)[i].props.name.c_str(), f_name)==0) {
	  			return (*fields_object)[i].val;
			}
      throw DbErrors("Field not found: %s",f_name);
       }
  throw DbErrors("Dataset state is Inactive");
  //field_value fv;
  //return fv;
}

const field_value Dataset::get_field_value_by_index(int p_index)
{
	if (ds_state != dsInactive) 
	{
		if (ds_state == dsEdit || ds_state == dsInsert)
		{
			return (*edit_object)[p_index] . val;

			/*for (unsigned int i=0; i < edit_object->size(); i++)
			{
				int t_index;
				t_index = (*edit_object)[i].props.index;

				if ((*edit_object)[i].props.index == p_index)
					return (*edit_object)[i].val;
			}
			throw DbErrors("Field not found: %i", p_index);*/
		}
		else
		{
			return (*fields_object)[p_index] . val;

/*			for (unsigned int i=0; i < fields_object->size(); i++)
			{
				int t_index;
				t_index = (*edit_object)[i].props.index;

				if ((*edit_object)[i].props.index == p_index)
					return (*edit_object)[i] . val;
			}*/
		}
      throw DbErrors("Field not found: %i", p_index);
	}

  throw DbErrors("Dataset state is Inactive");
}


const field_value Dataset::f_old(const char *f_name) {
  if (ds_state != dsInactive)
    for (int unsigned i=0; i < fields_object->size(); i++) 
      if ((*fields_object)[i].props.name == f_name)
	return (*fields_object)[i].val;
  field_value fv;
  return fv;
}

int Dataset::str_compare(const char * s1, const char * s2) {
 	string ts1 = s1; 
 	string ts2 = s2;
 	string::const_iterator p = ts1.begin();
 	string::const_iterator p2 = ts2.begin();
 	while (p!=ts1.end() && p2 != ts2.end()) {
 	if (toupper(*p)!=toupper(*p2))
 		return (toupper(*p)<toupper(*p2)) ? -1 : 1;
 		++p;
 		++p2;		
 	}	
 	return (ts2.size() == ts1.size())? 0:
 		(ts1.size()<ts2.size())? -1 : 1;
 }


void Dataset::setParamList(const ParamList &params){
  plist = params;
}


bool Dataset::locate(){
  bool result;
  if (plist.empty()) return false;

  std::map<string,field_value>::const_iterator i;
  first();
  while (!eof()) {
    result = true;
    for (i=plist.begin();i!=plist.end();++i)
     if (fv(i->first.c_str()).get_asString() == i->second.get_asString()) {
	 
	continue;
      }
      else {result = false; break;}
    if (result) { return result;}
    next();
  }
  return false;
}

bool Dataset::locate(const ParamList &params) {
  plist = params;
  return locate();
}

bool Dataset::findNext(void) {
  bool result;
  if (plist.empty()) return false;

  std::map<string,field_value>::const_iterator i;
  while (!eof()) {
    result = true;
    for (i=plist.begin();i!=plist.end();++i)
      if (fv(i->first.c_str()).get_asString() == i->second.get_asString()) {
	continue;
      }
      else {result = false; break;}
    if (result) { return result;}
    next();
  }
  return false;
}


void Dataset::add_update_sql(const char *upd_sql){
  string s = upd_sql;
  update_sql.push_back(s);
}


void Dataset::add_update_sql(const string &upd_sql){
  update_sql.push_back(upd_sql);
}

void Dataset::add_insert_sql(const char *ins_sql){
  string s = ins_sql;
  insert_sql.push_back(s);
}


void Dataset::add_insert_sql(const string &ins_sql){
  insert_sql.push_back(ins_sql);
}

void Dataset::add_delete_sql(const char *del_sql){
  string s = del_sql;
  delete_sql.push_back(s);
}


void Dataset::add_delete_sql(const string &del_sql){
  delete_sql.push_back(del_sql);
}

void Dataset::clear_update_sql(){
  update_sql.clear();
}

void Dataset::clear_insert_sql(){
  insert_sql.clear();
}

void Dataset::clear_delete_sql(){
  delete_sql.clear();
}

int Dataset::field_count()
{ 
	return fields_object->size();
}
int Dataset::fieldCount() { return fields_object->size();}

const char *Dataset::fieldName(int n) {
  if ( n < field_count() && n >= 0)
    return (*fields_object)[n].props.name.c_str();
  else
    return NULL;
}

int Dataset::fieldSize(int n) {
  if ( n < field_count() && n >= 0)
    return (*fields_object)[n].props.field_len;
  else
    return 0;
}

int Dataset::fieldIndex(const char *fn) {
for (unsigned int i=0; i < fields_object->size(); i++)
      if ((*fields_object)[i].props.name == fn)
	return i;
  return -1;
}



//************* DbErrors implementation ***************

DbErrors::DbErrors() {
  msg_ = "Unknown Database Error";
}


DbErrors::DbErrors(const char *msg, ...) {
  va_list vl;
  va_start(vl, msg);
  char buf[DB_BUFF_MAX]="";
  vsnprintf(buf, DB_BUFF_MAX-1, msg, vl);
  va_end(vl);
  msg_ =   "Database Error: ";
  msg_ += buf;
}

const char * DbErrors::getMsg() {
	return msg_.c_str();
	
}

