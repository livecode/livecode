/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

//
// Script class declarations
//
#ifndef	EXECPOINT_H
#define	EXECPOINT_H

#ifndef __MC_VARIABLE__
#include "variable.h"
#endif

#define EP_PAD 256
#define EP_MASK 0xFFFFFF00

class MCExecPoint
{
	MCObject *curobj;

	// MW-2009-01-30: [[ Inherited parentScripts ]]
	// We store a reference to the parentScript use which is the current context
	// so we can retrieve the correct script locals. If this is NULL, then we
	// are not in parentScript context.
	MCParentScriptUse *parentscript;

	MCHandlerlist *curhlist;
	MCHandler *curhandler;
	MCVariableValue *array;
	char *buffer;
	uint4 size;
	MCString svalue;
	real8 nvalue;
	Value_format format;
	uint2 nffw;
	uint2 nftrailing;
	uint2 nfforce;
	uint2 cutoff;
	uint2 line;
	Boolean convertoctals;
	Boolean casesensitive;
	Boolean wholematches;
	Boolean usesystemdate;
	Boolean useunicode;
	Boolean deletearray;
	char itemdel;
	char columndel;
	char linedel;
	char rowdel;

public:
	MCExecPoint()
	{
		memset(this, 0, sizeof(MCExecPoint));
		itemdel = ',';
		columndel = '\t';
		linedel = '\n';
		rowdel = '\n';
		nffw = 8;
		nftrailing = 6;
		cutoff = 35;
	}
	MCExecPoint(const MCExecPoint &ep)
	{
		*this = ep;
		array = NULL;
		deletearray = False;
		buffer = NULL;
		size = 0;
	}
	MCExecPoint(MCObject *object, MCHandlerlist *hlist, MCHandler *handler)
	{
		memset(this, 0, sizeof(MCExecPoint));
		curobj = object;
		curhlist = hlist;
		curhandler = handler;
		itemdel = ',';
		columndel = '\t';
		linedel = '\n';
		rowdel = '\n';
		nffw = 8;
		nftrailing = 6;
		cutoff = 35;
	}
	void restore(const MCExecPoint &ep)
	{
		curobj = ep.curobj;
		curhlist = ep.curhlist;
		curhandler = ep.curhandler;
	}
	~MCExecPoint()
	{
		delete buffer;
		if (deletearray)
			delete array;
	}
	void sethlist(MCHandlerlist *hl)
	{
		curhlist = hl;
	}
	void sethandler(MCHandler *newhandler)
	{
		curhandler = newhandler;
	}

	// Return a pointer to the array value contained in the exec-point
	MCVariableValue *getarray(void)
	{
		// MW-2014-03-12: [[ Bug 11867 ]] If the format is array return the array
		//   pointer. Otherwise return nil (since array is not necessarily set to
		//   nil if it changes format).
		if (format == VF_ARRAY) 
			return array;
		return nil;
	}

	Boolean getdeletearray()
	{
		return deletearray;
	}

	// Take the current array value of the ExecPoint away from it
	void takearray(MCVariableValue*& r_array, Boolean& r_delete)
	{
		r_array = array;
		r_delete = deletearray;

		array = NULL;
		deletearray = False;
	}

	// Set the current exec-points value to the given array, if d is
	// true, then this will manage the lifetime.
	void setarray(MCVariableValue *a, Boolean d);

	Boolean isempty(void) const
	{
		return format == VF_UNDEFINED || format != VF_ARRAY && format != VF_NUMBER && svalue . getlength() == 0;
	}
	
	Value_format getformat()
	{
		return format;
	}
	void setsvalue(const MCString &s)
	{
		svalue = s;
		format = VF_STRING;
	}
	void setnvalue(const real8 &n)
	{
		nvalue = n;
		format = VF_NUMBER;
	}
	void setnvalue(uint4 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(int4 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(uint2 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setnvalue(int2 n)
	{
		nvalue = (real8)n;
		format = VF_NUMBER;
	}
	void setboth(const MCString &s, const real8 &n)
	{
		svalue = s;
		nvalue = n;
		format = VF_BOTH;
	}
	void clear();
	char *getbuffer(uint4 newsize);
	uint4 getbuffersize()
	{
		return size;
	}
	void setbuffer(char *s, uint4 l)
	{
		buffer = s;
		size = l;
	}
	Boolean changed()
	{
		return svalue.getstring() == buffer;
	}
	void copysvalue(const char *s, uint4 l);
	void copysvalue(const char *s) {copysvalue(s, strlen(s));}

	// Ensure that there is a string value available and the exec-point owns it.
	void grabsvalue();

	// Ensure that the execpoint owns its array (if it has one)
	void grabarray();

	// Take the memory pointed to by buffer as the value, and set the execpoint to a string
	void grabbuffer(char *p_buffer, uint4 p_length);

	// Ensure that the execpoint owns its value
	void grab(void);

	void setint(int4);
	void setuint(uint4);
    void setint64(int64_t);
    void setuint64(uint64_t);
	void setr8(real8 n, uint2 fw, uint2 trailing, uint2 force);
	Exec_stat getreal8(real8 &, uint2, uint2, Exec_errors);
	Exec_stat getuint2(uint2 &, uint2, uint2, Exec_errors);
	Exec_stat getint4(int4 &, uint2, uint2, Exec_errors);
	Exec_stat getuint4(uint4 &, uint2, uint2, Exec_errors);
	Exec_stat getboolean(Boolean &, uint2, uint2, Exec_errors);
	Exec_stat ntos();
	Exec_stat ston();
	void lower();
	void upper();
	
	// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
	Exec_stat tos()
	{
		return format == VF_NUMBER ? ntos() : (format != VF_ARRAY ? ES_NORMAL : ES_ERROR);
	}
	
	// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
	Exec_stat ton()
	{
		return format == VF_STRING ? ston() : (format != VF_ARRAY ? ES_NORMAL : ES_ERROR);
	}
	
	// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
	Exec_stat tona(void)
	{
		return format == VF_STRING ? ston() : ES_NORMAL;
	}

	// Ensure that the value in the ExecPoint is a NUL-terminated string and return
	// a pointer to it.
	const char *getcstring(void);

	// Ensure that the value in the ExecPoint is NUL-terminated then return it as
	// a MCString.
	const MCString& getsvalue0(void)
	{
		getcstring();
		return svalue;
	}

	const MCString &getsvalue()
	{
		if (format==VF_NUMBER)
			tos();
		return svalue;
	}
	real8 getnvalue() const
	{
		return nvalue;
	}
	uint4 getuint4() const
	{
		return (uint4)(nvalue < 0.0 ? 0 : nvalue + 0.5);
	}
	uint2 getuint2() const
	{
		return (uint2)(nvalue < 0.0 ? 0 : nvalue + 0.5);
	}
	int4 getint4() const
	{
		return (int4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	int64_t getint8() const
	{
		return (int4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	uint64_t getuint8() const
	{
		return (uint4)(nvalue < 0.0?nvalue - 0.5:nvalue + 0.5);
	}
	void setstrlen()
	{
		svalue.set(buffer, strlen(buffer));
		format = VF_STRING;
	}
	void setlength(uint4 l)
	{
		svalue.set(buffer, l);
		format = VF_STRING;
	}
	MCObject *getobj() const
	{
		return curobj;
	}
	MCHandlerlist *gethlist() const
	{
		return curhlist;
	}
	MCHandler *gethandler() const
	{
		return curhandler;
	}
	uint2 getnffw() const
	{
		return nffw;
	}
	uint2 getnftrailing() const
	{
		return nftrailing;
	}
	uint2 getnfforce() const
	{
		return nfforce;
	}
	Boolean getcasesensitive() const
	{
		return casesensitive;
	}
	uint2 getcutoff() const
	{
		return cutoff;
	}
	Boolean getconvertoctals() const
	{
		return convertoctals;
	}
	char getitemdel() const
	{
		return itemdel;
	}
	char getcolumndel() const
	{
		return columndel;
	}
	char getrowdel() const
	{
		return rowdel;
	}
	char getlinedel()
	{
		return linedel;
	}
	Boolean getwholematches() const
	{
		return wholematches;
	}
	Boolean getusesystemdate() const
	{
		return usesystemdate;
	}
	Boolean getuseunicode() const
	{
		return useunicode;
	}
	Exec_stat setcasesensitive(uint2 line, uint2 pos)
	{
		return getboolean(casesensitive, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setcutoff(uint2 line, uint2 pos)
	{
		return getuint2(cutoff, line, pos, EE_PROPERTY_NOTANUM);
	}
	Exec_stat setconvertoctals(uint2 line, uint2 pos)
	{
		return getboolean(convertoctals, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setusesystemdate(uint2 line, uint2 pos)
	{
		return getboolean(usesystemdate, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setuseunicode(uint2 line, uint2 pos)
	{
		return getboolean(useunicode, line, pos, EE_PROPERTY_NAB);
	}
	Exec_stat setitemdel(uint2 line, uint2 pos);
	Exec_stat setcolumndel(uint2 line, uint2 pos);
	Exec_stat setlinedel(uint2 line, uint2 pos);
	Exec_stat setrowdel(uint2 line, uint2 pos);
	Exec_stat setwholematches(uint2 line, uint2 pos)
	{
		return getboolean(wholematches, line, pos, EE_PROPERTY_NAB);
	}

	void setobj(MCObject *p_object)
	{
		curobj = p_object;
	}

	void setline(uint2 l)
	{
		line = l;
	}
	uint2 getline()
	{
		return line;
	}

	MCParentScriptUse *getparentscript(void) const
	{
		return parentscript;
	}

	void setparentscript(MCParentScriptUse *obj)
	{
		parentscript = obj;
	}

	void setnumberformat();
	void insert(const MCString &, uint4 s, uint4 e);
	uint1 *pad(char value, uint4 count);
	void substring(uint4 s, uint4 e);
	void tail(uint4 s);
	Boolean usingbuffer();
	void fill(uint4 s, char c, uint4 n);
	void texttobinary();
	void binarytotext();
	void parseURL(Url_type &urltype, const char *&hostname, const char *&rname, uint4 &port, const char *&auth);

	void utf16toutf8(void);
	void utf8toutf16(void);

	void utf16tonative(void);
	void nativetoutf16(void);
	
	void utf8tonative(void);
	void nativetoutf8(void);

	// MW-2012-02-16: [[ IntrinsicUnicode ]] Maps the content to unicode or native
	//   depending on 'want_unicode'. Content is assumed to be in unicode if 'is_unicode'
	//   is true; otherwise it is assumed to be native.
	void mapunicode(bool is_unicode, bool want_unicode);
	
	// MW-2011-06-22: [[ SERVER ]] Provides augmented functionality for finding
	//   variables if there is no handler (i.e. global server scope).
	Parse_stat findvar(MCNameRef p_name, MCVarref** r_var);

	//////////

	void setstaticcstring(const char *string);
	void setstaticchars(const char *string, uindex_t length);
	void setstaticbytes(const void *bytes, uindex_t length);
	void setstaticmcstring(const MCString& string);

	void setempty(void);
	void setcstring(const char *string);
	void setmcstring(const MCString& string);
	void setstringf(const char *spec, ...);
	void setchars(const char *string, uindex_t length);
	void setchar(char character);
	void setbytes(const void *bytes, uindex_t length);
	void setbyte(uint8_t byte);
	void setboolean(Boolean value);
	void setpoint(int16_t x, int16_t y);
	void setrectangle(int32_t left, int32_t top, int32_t right, int32_t bottom);
	void setrectangle(const MCRectangle& rect);
	void setrectangle(const MCRectangle32& rect);
	void setcolor(uint32_t r, uint32_t g, uint32_t b);
	void setcolor(const MCColor& color);
	void setcolor(const MCColor& color, const char *colorname);
	void setpixel(uint32_t pixel);
	void setnewline(void);

	void appendcstring(const char *string);
	void appendmcstring(const MCString& string);
	void appendstringf(const char *spec, ...);
	void appendchars(const char *string, uindex_t length);
	void appendchar(char character);
	void appendunichars(const uint2 *string, uindex_t length);
	void appendunichar(uint2 character);
	void appendbytes(const void *bytes, uindex_t length);
	void appendbyte(uint8_t byte);
	void appenduint(uint32_t integer);
	void appendint(int32_t integer);
	void appendreal(double real);
	void appendnewline(void);
	void appendnewline(bool unicode);

	void concatcstring(const char *string, Exec_concat sep, bool first);
	void concatchars(const char *chars, uindex_t count, Exec_concat sep, bool first);
	void concatmcstring(const MCString& string, Exec_concat sep, bool first);
	void concatuint(uint32_t value, Exec_concat sep, bool first);
	void concatint(int32_t value, Exec_concat sep, bool first);
	void concatreal(double real, Exec_concat sep, bool first);

	void replacechar(char from, char to);

	void setnameref_unsafe(MCNameRef name);
	void appendnameref(MCNameRef name);
	void concatnameref(MCNameRef name, Exec_concat sep, bool first);

	bool copyasnameref(MCNameRef& name);
    
	// Attempts to convert the contents of the ep from UTF-16 to native,
	// returning true if successful and replacing the contents.
    bool trytoconvertutf16tonative();
    
    // SN-2015-06-03: [[ Bug 11277 ]] Refactor similar functions from MCHandler
    //  and MCHandlerlist
    static void deletestatements(MCStatement *p_statements);
    Exec_stat eval(MCExecPoint &ep);
    Exec_stat doscript(MCExecPoint &ep, uint2 line, uint2 pos);

private:
	void dounicodetomultibyte(bool p_native, bool p_reverse);

	void concat(uint4 n, Exec_concat, Boolean);
	void concat(int4 n, Exec_concat, Boolean);
	void concat(const MCString &, Exec_concat, Boolean);
};

inline void MCExecPoint::utf16toutf8(void)
{
	dounicodetomultibyte(false, false);
}

inline void MCExecPoint::utf8toutf16(void)
{
	dounicodetomultibyte(false, true);
}

inline void MCExecPoint::utf16tonative(void)
{
	dounicodetomultibyte(true, false);
}

inline void MCExecPoint::nativetoutf16(void)
{
	dounicodetomultibyte(true, true);
}

inline void MCExecPoint::utf8tonative(void)
{
	utf8toutf16();
	utf16tonative();
}

inline void MCExecPoint::nativetoutf8(void)
{
	nativetoutf16();
	utf16toutf8();
}

#ifndef __MC_VARIABLE_IMPLEMENTATION__
#include "variable_impl.h"
#endif

#endif
