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

//
// Script class declarations
//
#ifndef	EXECPOINT_H
#define	EXECPOINT_H

#include "exec.h"

#define EP_PAD 256
#define EP_MASK 0xFFFFFF00

class MCExecPoint
{
    MCExecContext m_ec;
    MCValueRef value;

public:
    MCExecPoint(MCExecContext ctxt):
        m_ec(ctxt)
    {
        value = MCValueRetain(kMCEmptyString);
	}

    MCExecPoint(const MCExecPoint &ep):
        m_ec(ep . m_ec)
	{
        *this = ep;
        value = MCValueRetain(ep . value);
	}

    MCExecPoint(MCObject *object, MCHandlerlist *hlist, MCHandler *handler):
        m_ec(object, hlist, handler)
    {
        value = MCValueRetain(kMCEmptyString);
    }

	~MCExecPoint()
    {
        MCValueRelease(value);
	}

    MCExecContext GetEC() const
    {
        return m_ec;
    }

	void sethlist(MCHandlerlist *hl)
	{
        m_ec . SetHandlerList(hl);
	}
	void sethandler(MCHandler *newhandler)
	{
        m_ec . SetHandler(newhandler);
	}

	MCObject *getobj() const
	{
        return m_ec . GetObject();
	}
	MCHandlerlist *gethlist() const
	{
        return m_ec . GetHandlerList();
	}
	MCHandler *gethandler() const
	{
        return m_ec . GetHandler();
	}
	uinteger_t getnffw() const
	{
        return m_ec . GetNumberFormatWidth();
	}
	uinteger_t getnftrailing() const
	{
        return m_ec . GetNumberFormatTrailing();
	}
	uinteger_t getnfforce() const
	{
        return m_ec . GetNumberFormatForce();
	}
	Boolean getcasesensitive() const
	{
        return m_ec . GetCaseSensitive();
	}
	uint2 getcutoff() const
	{
        return m_ec . GetCutOff();
	}
	Boolean getconvertoctals() const
	{
        return m_ec . GetConvertOctals();
	}
	MCStringRef getitemdel() const
	{
        return m_ec . GetItemDelimiter();
	}
	MCStringRef getcolumndel() const
	{
        return m_ec . GetColumnDelimiter();
	}
	MCStringRef getrowdel() const
	{
        return m_ec . GetRowDelimiter();
	}
	MCStringRef getlinedel()
	{
        return m_ec . GetLineDelimiter();
	}
	Boolean getwholematches() const
	{
        return m_ec . GetWholeMatches();
	}
	Boolean getusesystemdate() const
	{
        return m_ec . GetUseSystemDate();
	}
	Boolean getuseunicode() const
	{
        return m_ec . GetUseUnicode();
	}
    void setcasesensitive(bool p_value) { m_ec . SetCaseSensitive(p_value);}
    void setconvertoctals(bool p_value) { m_ec . SetConvertOctals(p_value);}
    void setwholematches(bool p_value) { m_ec . SetWholeMatches(p_value);}
    void setuseunicode(bool p_value) { m_ec . SetUseUnicode(p_value);}
    void setusesystemdate(bool p_value) { m_ec . SetUseSystemDate(p_value);}
    void setcutoff(uint2 p_value) { m_ec . SetCutOff(p_value);}
    void setitemdel(MCStringRef p_value) { m_ec . SetItemDelimiter(p_value);}
    void setcolumndel(MCStringRef p_value) { m_ec . SetColumnDelimiter(p_value);}
    void setlinedel(MCStringRef p_value) { m_ec . SetLineDelimiter(p_value);}
    void setrowdel(MCStringRef p_value) { m_ec . SetRowDelimiter(p_value);}
    void setnumberformat(uint2 fw, uint2 trailing, uint2 force) {m_ec . SetNumberFormat(fw, trailing, force);}

	void setobj(MCObject *p_object)
	{
        m_ec . SetObject(p_object);
	}

	void setline(uint2 l)
	{
        m_ec . SetLine(l);
	}
	uint2 getline()
	{
        return m_ec . GetLine();
	}

	MCParentScriptUse *getparentscript(void) const
	{
        return m_ec . GetParentScript();
	}

	void setparentscript(MCParentScriptUse *obj)
	{
        m_ec . SetParentScript(obj);
	}

	void setnumberformat();

	//////////

#if 0
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
	void copysvalue(const char *s, uint4 l);
	void copysvalue(const char *s) {copysvalue(s, strlen(s));}
	// Ensure that there is a string value available and the exec-point owns it.e
	void grabsvalue();
	// Ensure that the execpoint owns its array (if it has one)
	void grabarray();
	// Ensure that the execpoint owns its value
	void grab(void);
	void clear();
	// Take the memory pointed to by buffer as the value, and set the execpoint to a string
	void grabbuffer(char *p_buffer, uint4 p_length);

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
	Exec_stat ntos();
	Exec_stat ston();
	void lower();
	void upper();
	
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

    // MW-2013-11-08: [[ RefactorIt ]] Returns the 'it' varref for the current context.
    MCVarref *getit(void);
	
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
	void setbool(bool value);
	void setboolean(Boolean value);
	void setpoint(int16_t x, int16_t y);
	void setpoint(MCPoint point);
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

	void concatnameref(MCNameRef name, Exec_concat sep, bool first);
	bool copyasnameref(MCNameRef& r_name);

	void concatstringref(MCStringRef string, Exec_concat sep, bool first);
	bool copyasstringref(MCStringRef& r_string);
	bool copyasmutablestringref(MCStringRef& r_string);

	bool copyasdataref(MCDataRef& r_data);

	bool copyaslegacypoint(MCPoint& r_point);
	bool copyaslegacyrectangle(MCRectangle& r_rectangle);
	bool copyaslegacycolor(MCColor& r_color);

	//

	// Methods that work with MCObject::get/set*prop.
	bool copyasbool(bool& r_value);
	bool copyasint(integer_t& r_value);
	bool copyasuint(uinteger_t& r_value);
	bool copyasdouble(double& r_value);
	bool copyaschar(char_t& r_value);
	bool copyasnumber(MCNumberRef& r_value);
	bool copyasstring(MCStringRef& r_value);
	bool copyasarray(MCArrayRef& r_value);
	bool copyasvariant(MCValueRef& r_value);
	
	bool copyaspoint(MCPoint& r_value);
	bool copyasrect(MCRectangle& r_value);

#endif
	// Variant methods.

	// Set the ep to the contents of value.
	bool setvalueref(MCValueRef value);
	// Get the valueref from the ep.
	MCValueRef getvalueref(void);
	// Set the ep to the contents of value. If value is nil, then empty is used.
	bool setvalueref_nullable(MCValueRef value);
	// Make a copy of the contents of the ep as a value.
	bool copyasvalueref(MCValueRef& r_value);

};

#endif
