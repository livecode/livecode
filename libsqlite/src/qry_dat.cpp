/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project: C++ Dynamic Library
 * Module: FieldValue class realisation file
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlitedataset/qry_dat.h>
#include "ctype.h"

//Constructors 
field_value::field_value(){
  str_value = "";
  field_type = ft_String;
  is_null = false;
  };

field_value::field_value(const char *s) {
  str_value = s;
  field_type = ft_String;
  is_null = false;
}
  
field_value::field_value(const bool b) {
  bool_value = b; 
  field_type = ft_Boolean;
  is_null = false;
}

field_value::field_value(const char c) {
  char_value = c; 
  field_type = ft_Char;
  is_null = false;
}
  
field_value::field_value(const short s) {
  short_value = s; 
  field_type = ft_Short;
  is_null = false;
}
  
field_value::field_value(const unsigned short us) {
  ushort_value = us; 
  field_type = ft_UShort;
  is_null = false;
}
  
field_value::field_value(const long long l) {
  long_value = l; 
  field_type = ft_Long;
  is_null = false;
}

field_value::field_value(const int i) {
  long_value = (long)i; 
  field_type = ft_Long;
  is_null = false;
}
  
field_value::field_value(const unsigned long long ul) {
  ulong_value = ul; 
  field_type = ft_ULong;
  is_null = false;
}
  
field_value::field_value(const float f) {
  float_value = f; 
  field_type = ft_Float;
  is_null = false;
}
  
field_value::field_value(const double d) {
  double_value = d; 
  field_type = ft_Double;
  is_null = false;
}
  
field_value::field_value (const field_value & fv) {
  switch (fv.get_fType()) {
    case ft_String: {
      set_asString(fv.get_asString());
      break;
    }
    case ft_Boolean:{
      set_asBool(fv.get_asBool());
      break;     
    }
    case ft_Char: {
      set_asChar(fv.get_asChar());
      break;
    }
    case ft_Short: {
      set_asShort(fv.get_asShort());
      break;
    }
    case ft_UShort: {
      set_asUShort(fv.get_asUShort());
      break;
    }
    case ft_Long: {
      set_asLong(fv.get_asLong());
      break;
    }
    case ft_ULong: {
      set_asULong(fv.get_asULong());
      break;
    }
    case ft_Float: {
      set_asFloat(fv.get_asFloat());
      break;
    }
    case ft_Double: {
      set_asDouble(fv.get_asDouble());
      break;
    }
  }
  
  // OK-2010-02-17: [[Bug 8273]] - Copy is_null from fv, rather than assume its false.
  if (fv . get_isNull())
  	  set_isNull();
  else
	  is_null = false;
};


//empty destructor
field_value::~field_value(){

  }

  
//Conversations functions
string field_value::get_asString() const {
    string tmp;
    switch (field_type) {
    case ft_String: {
      tmp = str_value;
      return tmp;
    }
    case ft_Boolean:{
      if (bool_value) 
	return tmp = "True";
      else
	return tmp = "False";
    }
    case ft_Char: {
      return tmp = char_value;
    }
    case ft_Short: {
      char t[10];
      sprintf(t,"%i",short_value);
      return tmp = t;
    }
    case ft_UShort: {
      char t[10];
      sprintf(t,"%i",ushort_value);
      return tmp = t;
    }
    case ft_Long: {
      char t[32];
      sprintf(t,"%lli",long_value);
      return tmp = t;
    }
    case ft_ULong: {
      char t[32];
      sprintf(t,"%llu",ulong_value);
      return tmp = t;
    }
    case ft_Float: {
      char t[16];
      sprintf(t,"%f",float_value);
      return tmp = t;
    }
    case ft_Double: {
      char t[32];
      sprintf(t,"%f",double_value);
      return tmp = t;
    }
    }
  };



bool field_value::get_asBool() const {
    switch (field_type) {
    case ft_String: {
	  string bob = str_value;
	  transform(bob.begin(),bob.end(),bob.begin(),::tolower);
      if (!bob.compare("true") )
          return true;
      else
		if( !bob.compare("t") )
			return true;
		else
			if( !bob.compare("1"))
				return true;
			else
	           return false;
    }
    case ft_Boolean:{
      return bool_value;
      }
    case ft_Char: {
      if (char_value == 'T')
	return true;
      else
	return false;
    }
    case ft_Short: {
      return short_value != 0;
    }
    case ft_UShort: {
      return ushort_value != 0;
    }
    case ft_Long: {
      return long_value != 0;
    }
    case ft_ULong: {
      return ulong_value != 0;
    }
    case ft_Float: {
      return float_value != 0.0;
    }
    case ft_Double: {
      return double_value != 0.0;
    }
    }
  };
  

char field_value::get_asChar() const {
  switch (field_type) {
    case ft_String: {
      return str_value[0];
    }
    case ft_Boolean:{
      char c;
      if (bool_value) 
	return c='T';
      else
	return c='F';
    }
    case ft_Char: {
      return  char_value;
    }
    case ft_Short: {
      char t[10];
      sprintf(t,"%i",short_value);
      return t[0];
    }
    case ft_UShort: {
      char t[10];
      sprintf(t,"%i",ushort_value);
      return t[0];
    }
    case ft_Long: {
      char t[12];
      sprintf(t,"%lli",long_value);
      return t[0];
    }
    case ft_ULong: {
      char t[12];
      sprintf(t,"%llu",ulong_value);
      return t[0];
    }
    case ft_Float: {
      char t[16];
      sprintf(t,"%f",float_value);
      return t[0];
    }
    case ft_Double: {
      char t[32];
      sprintf(t,"%f",double_value);
      return t[0];
    }
    }
  };


short field_value::get_asShort() const {
    switch (field_type) {
    case ft_String: {
      return (short)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (short)bool_value;
    }
    case ft_Char: {
      return (short)char_value;
    }
    case ft_Short: {
       return short_value;
    }
    case ft_UShort: {
       return (short)ushort_value;
    }
    case ft_Long: {
      return (short)long_value;
    }
    case ft_ULong: {
      return (short)ulong_value;
    }
    case ft_Float: {
      return (short)float_value;
    }
    case ft_Double: {
      return (short)double_value;
    }
    }
  };


unsigned short field_value::get_asUShort() const {
    switch (field_type) {
    case ft_String: {
      return (unsigned short)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (unsigned short)bool_value;
    }
    case ft_Char: {
      return (unsigned short)char_value;
    }
    case ft_Short: {
       return (unsigned short)short_value;
    }
    case ft_UShort: {
       return ushort_value;
    }
    case ft_Long: {
      return (unsigned short)long_value;
    }
    case ft_ULong: {
      return (unsigned short)ulong_value;
    }
    case ft_Float: {
      return (unsigned short)float_value;
    }
    case ft_Double: {
      return (unsigned short)double_value;
    }
    }
  };

long long field_value::get_asLong() const {
    switch (field_type) {
    case ft_String: {
#ifdef WIN32
		return _strtoi64(str_value.c_str(), NULL, 10);
#else
		return strtoll(str_value.c_str(), NULL, 10);
#endif
    }
    case ft_Boolean:{
      return (long long)bool_value;
    }
    case ft_Char: {
      return (long long)char_value;
    }
    case ft_Short: {
       return (long long)short_value;
    }
    case ft_UShort: {
       return (long long)ushort_value;
    }
    case ft_Long: {
      return long_value;
    }
    case ft_ULong: {
      return (long long)ulong_value;
    }
    case ft_Float: {
      return (long long)float_value;
    }
    case ft_Double: {
      return (long long)double_value;
    }
    }
  };

int field_value::get_asInteger() const{
  return (int)get_asLong();
}

unsigned long long field_value::get_asULong() const {
    switch (field_type) {
    case ft_String: {
#ifdef WIN32
		return _strtoui64(str_value.c_str(), NULL, 10);
#else
		return strtoull(str_value.c_str(), NULL, 10);
#endif
    }
    case ft_Boolean:{
      return (unsigned long long)bool_value;
    }
    case ft_Char: {
      return (unsigned long long)char_value;
    }
    case ft_Short: {
       return (unsigned long long)short_value;
    }
    case ft_UShort: {
       return (unsigned long long)ushort_value;
    }
    case ft_Long: {
      return (unsigned long long)long_value;
    }
    case ft_ULong: {
      return ulong_value;
    }
    case ft_Float: {
      return (unsigned long long)float_value;
    }
    case ft_Double: {
      return (unsigned long long)double_value;
    }
    }
  };

float field_value::get_asFloat() const {
    switch (field_type) {
    case ft_String: {
      return atof(str_value.c_str());
    }
    case ft_Boolean:{
      return (float)bool_value;
    }
    case ft_Char: {
      return (float)char_value;
    }
    case ft_Short: {
       return (float)short_value;
    }
    case ft_UShort: {
       return (float)ushort_value;
    }
    case ft_Long: {
      return (float)long_value;
    }
    case ft_ULong: {
      return (float)ulong_value;
    }
    case ft_Float: {
      return float_value;
    }
    case ft_Double: {
      return (float)double_value;
    }
    }
  };

double field_value::get_asDouble() const {
    switch (field_type) {
    case ft_String: {
      return atof(str_value.c_str());
    }
    case ft_Boolean:{
      return (double)bool_value;
    }
    case ft_Char: {
      return (double)char_value;
    }
    case ft_Short: {
       return (double)short_value;
    }
    case ft_UShort: {
       return (double)ushort_value;
    }
    case ft_Long: {
      return (double)long_value;
    }
    case ft_ULong: {
      return (double)ulong_value;
    }
    case ft_Float: {
      return (double)float_value;
    }
    case ft_Double: {
      return (double)double_value;
    }
    }
  };



field_value& field_value::operator= (const field_value & fv) {
  if ( this == &fv ) return *this;
  
  bool t_type_matched;
  t_type_matched = false;

  switch (fv.get_fType()) {
    case ft_String: {
      set_asString(fv.get_asString());
      t_type_matched = true;
      break;
    }
    case ft_Boolean:{
      set_asBool(fv.get_asBool());
      t_type_matched = true;
      break;     
    }
    case ft_Char: {
      set_asChar(fv.get_asChar());
      t_type_matched = true;
      break;
    }
    case ft_Short: {
      set_asShort(fv.get_asShort());
      t_type_matched = false;
      break;
    }
    case ft_UShort: {
      set_asUShort(fv.get_asUShort());
      t_type_matched = false;
      break;
    }
    case ft_Long: {
      set_asLong(fv.get_asLong());
      t_type_matched = false;
      break;
    }
    case ft_ULong: {
      set_asULong(fv.get_asULong());
      t_type_matched = false;
      break;
    }
    case ft_Float: {
      set_asFloat(fv.get_asFloat());
      t_type_matched = false;
      break;
    }
    case ft_Double: {
      set_asDouble(fv.get_asDouble());
      t_type_matched = false;
      break;
    }
    }

  // OK-2010-02-17: [[Bug 8273]] - Copy is_null from fv.
  if (fv.get_isNull())
	set_isNull();
  else
	is_null = false;

  // Preserve previous behavior.
  if (t_type_matched)
	  return *this;
};



//Set functions
void field_value::set_asString(const char *s) {
  str_value = s;
  field_type = ft_String;};

void field_value::set_asString(const string & s) {
  str_value = s;
  field_type = ft_String;};

void field_value::set_asString(const char *s, int lsize){
	str_value.resize(lsize);
	memmove((void *)str_value.c_str(),s,lsize);
	field_type = ft_String;
}
void field_value::set_asBool(const bool b) {
  bool_value = b; 
  field_type = ft_Boolean;};
  
void field_value::set_asChar(const char c) {
  char_value = c; 
  field_type = ft_Char;};
  
void field_value::set_asShort(const short s) {
  short_value = s; 
  field_type = ft_Short;};
  
void field_value::set_asUShort(const unsigned short us) {
  ushort_value = us; 
  field_type = ft_UShort;};
  
void field_value::set_asLong(const long long l) {
  long_value = l; 
  field_type = ft_Long;};

void field_value::set_asInteger(const int i) {
  long_value = (long)i; 
  field_type = ft_Long;};
  
void field_value::set_asULong(const unsigned long long ul) {
  long_value = ul; 
  field_type = ft_ULong;};
  
void field_value::set_asFloat(const float f) {
  float_value = f; 
  field_type = ft_Float;};
  
void field_value::set_asDouble(const double d) {
  double_value = d; 
  field_type = ft_Double;};

  
fType field_value::get_field_type() {
  return field_type;}

  
string field_value::gft() {
    string tmp;
    switch (field_type) {
    case ft_String: {
      tmp.assign("string");
      return tmp;
    }
    case ft_Boolean:{
      tmp.assign("bool");
      return tmp;
    }
    case ft_Char: {
      tmp.assign("char");
      return tmp;
    }
    case ft_Short: {
      tmp.assign("short");
      return tmp;
    }
    case ft_Long: {
      tmp.assign("long");
      return tmp;
    }
    case ft_Float: {
      tmp.assign("float");
      return tmp;
    }
    case ft_Double: {
      tmp.assign("double");
      return tmp;
    }
    }
  }

fType strToFType( char * ltype )
{
 if( ltype != NULL)
 {
   if( !strcmp("integer",(const char *)ltype) )
	   return ft_Long;
   if( !strcmp("boolean",(const char *)ltype) )
	   return ft_Boolean;
   if( !strcmp("float",(const char *)ltype) )
	   return ft_Float;
   if( !strcmp("char",(const char *)ltype) )
	   return ft_Char;
   if( !strcmp("double",(const char *)ltype) )
	   return ft_Double;
   if( !strcmp("long",(const char *)ltype) )
	   return ft_Long;
   if( !strcmp("blob",(const char *)ltype) )
	   return ft_Object;
  }

   return ft_String;

}
