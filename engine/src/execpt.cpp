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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "object.h"
#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "globals.h"
#include "handler.h"
#include "hndlrlst.h"
#include "osspec.h"

#include "unicode.h"

#include "scriptpt.h"
#include "statemnt.h"
#include "newobj.h"
#include "license.h"
#include "uidc.h"

void MCExecPoint::clear()
{
	format = VF_STRING;
	svalue.set(buffer, 0);
	if (deletearray)
	{
		delete array;
		array = NULL;
	}
}

void MCExecPoint::setarray(MCVariableValue *a, Boolean d)
{
	clear();
	array = a;
	deletearray = d;
	format = VF_ARRAY;
}

char *MCExecPoint::getbuffer(uint4 l)
{
	if (size <  l)
	{
		delete buffer;
		size = l + EP_PAD & EP_MASK;
		buffer = new char[size];
	}
	return buffer;
}

void MCExecPoint::copysvalue(const char *s, uint4 l)
{
	memcpy(getbuffer(l), s, l);
	svalue.set(buffer, l);
	format = VF_STRING;
}

void MCExecPoint::grabsvalue()
{
	if (format == VF_NUMBER)
		tos();
	if (svalue.getstring() != buffer)
	{
		uint4 l = svalue.getlength();
		memmove(getbuffer(l), svalue.getstring(), l);
		svalue.set(buffer, l);
	}
}

void MCExecPoint::grabarray()
{
	// MW-2008-06-16: Only grab the array if we don't own it (deletearray == False).
	if (!deletearray)
	{
		array = new MCVariableValue(*array);
		deletearray = True;
	}
}

void MCExecPoint::grabbuffer(char *p_buffer, uint4 p_length)
{
	if (buffer != NULL)
		delete buffer;
	if (deletearray)
	{
		delete array;
		deletearray = False;
	}
	buffer = p_buffer;
	size = p_length;
	svalue . set(buffer, size);
	format = VF_STRING;
}

void MCExecPoint::grab(void)
{
	if (format == VF_ARRAY)
		grabarray();
	else if (format != VF_NUMBER && svalue . getstring() != buffer)
	{
		uint4 l = svalue.getlength();
		memmove(getbuffer(l), svalue.getstring(), l);
		svalue.set(buffer, l);
	}
}

void MCExecPoint::setint(int4 i)
{
	sprintf(getbuffer(U4L), "%d", i);
	svalue.set(buffer, strlen(buffer));
	nvalue = (real8)i;
	format = VF_BOTH;
}

void MCExecPoint::setuint(uint4 i)
{
	sprintf(getbuffer(U4L), "%u", i);
	svalue.set(buffer, strlen(buffer));
	nvalue = (real8)i;
	format = VF_BOTH;
}

void MCExecPoint::setint64(int64_t i)
{
    sprintf(getbuffer(U8L), "%lld", i);
    svalue.set(buffer, strlen(buffer));
    nvalue = (real8)i;
    format = VF_BOTH;
}

void MCExecPoint::setuint64(uint64_t i)
{
    sprintf(getbuffer(U8L), "%llu", i);
    svalue.set(buffer, strlen(buffer));
    nvalue = (real8)i;
    format = VF_BOTH;
}

void MCExecPoint::setr8(real8 n, uint2 fw, uint2 trailing, uint2 force)
{
	sprintf(getbuffer(R8L), "%0*.*f", fw, trailing, n);
	MCU_strip(buffer, trailing, force);
	svalue.set(buffer, strlen(buffer));
	nvalue = n;
	format = VF_BOTH;
}

Exec_stat MCExecPoint::getreal8(real8 &d, uint2 l, uint2 p, Exec_errors e)
{
	if (format == VF_STRING)
		if (ston() != ES_NORMAL)
		{
			MCeerror->add
			(EE_VARIABLE_NAN, l, p, svalue);
			MCeerror->add
			(e, l, p, svalue);
			return ES_ERROR;
		}
	d = nvalue;
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getuint2(uint2 &d, uint2 l, uint2 p, Exec_errors e)
{
	if (format == VF_STRING)
		if (ston() != ES_NORMAL)
		{
			MCeerror->add
			(EE_VARIABLE_NAN, l, p, svalue);
			MCeerror->add
			(e, l, p, svalue);
			return ES_ERROR;
		}
	if (nvalue < 0.0)
		d = (uint2)(nvalue - 0.5);
	else
		d = (uint2)(nvalue + 0.5);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getint4(int4 &d, uint2 l, uint2 p, Exec_errors e)
{
	if (format == VF_STRING)
		if (ston() != ES_NORMAL)
		{
			MCeerror->add
			(EE_VARIABLE_NAN, l, p, svalue);
			MCeerror->add
			(e, l, p, svalue);
			return ES_ERROR;
		}
	if (nvalue < 0.0)
		d = (int4)(nvalue - 0.5);
	else
		d = (int4)(nvalue + 0.5);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getuint4(uint4 &d, uint2 l, uint2 p, Exec_errors e)
{
	if (format == VF_STRING)
		if (ston() != ES_NORMAL)
		{
			MCeerror->add
			(EE_VARIABLE_NAN, l, p, svalue);
			MCeerror->add
			(e, l, p, svalue);
			return ES_ERROR;
		}
	if (nvalue < 0.0)
		d = (uint4)(nvalue - 0.5);
	else
		d = (uint4)(nvalue + 0.5);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getboolean(Boolean &d, uint2 l, uint2 p, Exec_errors e)
{
	if (format == VF_UNDEFINED || format == VF_NUMBER
	        || !MCU_stob(svalue, d))
	{
		if (format == VF_UNDEFINED)
			clear();
		if (format == VF_NUMBER)
			tos();
		MCeerror->add(e, l, p, svalue);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

Exec_stat MCExecPoint::ntos()
{
	if (nvalue == BAD_NUMERIC)
		return ES_ERROR;

	uint4 length = MCU_r8tos(buffer, size, nvalue, nffw, nftrailing, nfforce);
	svalue.set(buffer, length);

	format = VF_BOTH;

	return ES_NORMAL;
}

Exec_stat MCExecPoint::ston()
{
	if (svalue.getlength() == 0)
		nvalue = 0.0;
	else
		if (!MCU_stor8(svalue, nvalue, convertoctals))
			return ES_ERROR;

	format = VF_BOTH;

	return ES_NORMAL;
}

void MCExecPoint::lower()
{
	uint4 length = svalue.getlength();
	MCU_lower(getbuffer(length), svalue);
	svalue.set(buffer, length);
}

void MCExecPoint::upper()
{
	uint4 length = svalue.getlength();
	MCU_upper(getbuffer(length), svalue);
	svalue.set(buffer, length);
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setitemdel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || svalue.getlength() != 1)
	{
		MCeerror->add
		(EE_VARIABLE_NAC, l, p, svalue);
		return ES_ERROR;
	}
	itemdel = svalue.getstring()[0];
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setcolumndel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || svalue.getlength() != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, svalue);
		return ES_ERROR;
	}
	columndel = svalue.getstring()[0];
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setrowdel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || svalue.getlength() != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, svalue);
		return ES_ERROR;
	}
	rowdel = svalue.getstring()[0];
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setlinedel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || svalue.getlength() != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, svalue);
		return ES_ERROR;
	}
	linedel = svalue.getstring()[0];
	return ES_NORMAL;
}

void MCExecPoint::setnumberformat()
{
	MCU_setnumberformat(svalue, nffw, nftrailing, nfforce);
}

void MCExecPoint::concat(const MCString &two, Exec_concat ec, Boolean first)
{
	if (format == VF_NUMBER)
		tos();
	uint4 oldlength = svalue.getlength();
	uint4 newlength = oldlength + two.getlength();
	if (!first && ec != EC_NONE)
		newlength++;
	if (newlength > size)
	{
		// MW-2012-01-25: [[ Bug 9956 ]] Small optimization to improve large
		//   concatenations. Using 'realloc' means that no copying of data is
		//   needed in the best cases.
		size = newlength + EP_PAD & EP_MASK;
		if (svalue.getstring() != buffer)
		{
			char *newbuffer = new char[size];
			memcpy(newbuffer, svalue.getstring(), oldlength);
			delete buffer;
			buffer = newbuffer;
		}
		else
		{
			char *newbuffer = (char *)realloc(buffer, size);
			if (newbuffer == nil)
				return;
			buffer = newbuffer;
		}
		svalue.setstring(buffer);
	}
	else
		if (svalue.getstring() != buffer)
		{
			memmove(buffer, svalue.getstring(), oldlength);
			svalue.setstring(buffer);
		}
	if (!first)
		switch (ec)
		{
		case EC_NONE:
			break;
		case EC_SPACE:
			buffer[oldlength++] = ' ';
			break;
		case EC_COMMA:
			buffer[oldlength++] = ',';
			break;
		case EC_NULL:
			buffer[oldlength++] = '\0';
			break;
		case EC_RETURN:
			buffer[oldlength++] = '\n';
			break;

		// MW-2009-06-17: Can now concatenate with tab into an EP.
		case EC_TAB:
			buffer[oldlength++] = '\t';
			break;
		}
	if (two.getlength() == 1)
		buffer[oldlength] = two.getstring()[0];
	else
		memcpy(&buffer[oldlength], two.getstring(), two.getlength());
	svalue.setlength(newlength);
	format = VF_STRING;
}

void MCExecPoint::concat(uint4 n, Exec_concat ec, Boolean first)
{
	char buffer[U4L];
	sprintf(buffer, "%u", n);
	concat(buffer, ec, first);
}

void MCExecPoint::concat(int4 n, Exec_concat ec, Boolean first)
{
	char buffer[I4L];
	sprintf(buffer, "%d", n);
	concat(buffer, ec, first);
}

void MCExecPoint::insert(const MCString &istring, uint4 s, uint4 e)
{
	if (format == VF_NUMBER)
		tos();
	uint4 oldlength = svalue.getlength();
	uint4 ilength = istring.getlength();
	uint4 newlength = oldlength - (e - s) + ilength;
	const char *sptr = svalue.getstring();
	const char *isptr = istring.getstring();
	char *oldbuffer = NULL;
	if (newlength > size || sptr >= buffer && sptr < buffer + newlength
	        || isptr >= buffer && isptr < buffer + newlength)
	{
		oldbuffer = buffer;
		size = newlength + EP_PAD & EP_MASK;
		buffer = new char[size];
		memcpy(buffer, sptr, s);
		memcpy(&buffer[s], isptr, ilength);
		memcpy(&buffer[s + ilength], &sptr[e], oldlength - e);
	}
	else
	{
		memmove(buffer, sptr, s);
		memmove(&buffer[s], isptr, ilength);
		memmove(&buffer[s + ilength], &sptr[e], oldlength - e);
	}
	delete oldbuffer;
	svalue.set(buffer, newlength);
	format = VF_STRING;
}

uint1 *MCExecPoint::pad(char value, uint4 count)
{
	uint4 oldlength = svalue.getlength();
	uint4 newlength = oldlength + count;
	if (newlength > size)
	{
		size = newlength + EP_PAD & EP_MASK;
		char *newbuffer = new char[size];
		memcpy(newbuffer, svalue.getstring(), oldlength);
		delete buffer;
		buffer = newbuffer;
		svalue.setstring(buffer);
	}
	else
		if (svalue.getstring() != buffer)
		{
			memmove(buffer, svalue.getstring(), oldlength);
			svalue.setstring(buffer);
		}
	memset(buffer + oldlength, value, count);
	svalue.setlength(newlength);
	return (uint1 *)buffer + oldlength;
}

void MCExecPoint::substring(uint4 s, uint4 e)
{
	svalue.set(&svalue.getstring()[s], e - s);
	format = VF_STRING;
}

void MCExecPoint::tail(uint4 s)
{
	svalue.set(svalue.getstring() + s, svalue.getlength() - s);
	format = VF_STRING;
}

Boolean MCExecPoint::usingbuffer()
{
	return svalue.getstring() >= buffer && svalue.getstring() < buffer + size;
}

void MCExecPoint::fill(uint4 s, char c, uint4 n)
{
	uint4 oldlength = svalue.getlength();
	uint4 newlength = oldlength + n;
	if (newlength > size)
	{
		size = newlength + EP_PAD & EP_MASK;
		char *newbuffer = new char[size];
		memcpy(newbuffer, svalue.getstring(), s);
		memcpy(newbuffer + s + n, svalue.getstring() + s, oldlength - s);
		delete buffer;
		buffer = newbuffer;
		svalue.setstring(buffer);
	}
	else
	{
		if (svalue.getstring() != buffer)
		{
			memmove(buffer, svalue.getstring(), oldlength);
			svalue.setstring(buffer);
		}
		memmove(buffer + s + n, buffer + s, oldlength - s);
	}
	memset(buffer + s, c, n);
	svalue.setlength(newlength);
}

void MCExecPoint::texttobinary()
{
	grabsvalue();
	char *sptr = buffer;
	char *dptr = buffer;
	uint4 l = svalue.getlength();
	char *eptr = buffer + l;
	while (sptr < eptr)
	{
		if (*sptr == '\r')
		{
			if (sptr < eptr - 1 &&  *(sptr + 1) == '\n')
				l--;
			else
				*dptr++ = '\n';
			sptr++;
		}
		else
			if (!*sptr)
			{
				*dptr++ = ' ';
				sptr++;
			}
			else
				*dptr++ = *sptr++;
	}
	svalue.set(buffer, l);
}

void MCExecPoint::binarytotext()
{
	if (format == VF_NUMBER)
		tos();
#ifdef __CRLF__
	uint4 pad = 0;
	const char *sptr = svalue.getstring();
	uint4 i;
	for (i = 0 ; i < svalue.getlength() ; i++)
		if (*sptr++ == '\n')
			pad++;
	if (pad != 0)
	{
		uint4 newsize = svalue.getlength() + pad;
		char *newbuffer = new char[newsize];
		sptr = svalue.getstring() + svalue.getlength();
		char *dptr = newbuffer + newsize;
		while (dptr > newbuffer)
		{
			*--dptr = *--sptr;
			if (*sptr == '\n')
				*--dptr = '\r';
		}
		delete buffer;
		buffer = newbuffer;
		size = newsize;
		svalue.set(buffer, newsize);
		format = VF_STRING;
	}
#elif defined __CR__
	grabsvalue();
	char *sptr = buffer;
	uint4 i;
	for (i = 0 ; i < svalue.getlength() ; i++)
	{
		if (*sptr == '\n')
			*sptr = '\r';
		sptr++;
	}
	format = VF_STRING;
#endif
}

const char *MCExecPoint::getcstring(void)
{
	pad('\0', 1);
	svalue . set(svalue . getstring(), svalue . getlength() - 1);
	return svalue . getstring();
}

// MW-2011-06-22: [[ SERVER ]] Provides augmented functionality for finding
//   variables if there is no handler (i.e. global server scope).
Parse_stat MCExecPoint::findvar(MCNameRef p_name, MCVarref** r_var)
{
	Parse_stat t_stat;
	t_stat = PS_ERROR;
	
	if (curhandler != NULL)
		t_stat = curhandler -> findvar(p_name, r_var);
	else if (curhlist != NULL)
	{
		// MW-2011-08-23: [[ UQL ]] We are searching in global context, so do include UQLs.
		t_stat = curhlist -> findvar(p_name, false, r_var);
	}
	
	return t_stat;
}

///////////////////////////////////////////////////////////////////////////////

extern int UnicodeToUTF8(const uint2 *p_source_str, int p_source, char *p_dest_str, int p_dest);
extern int UTF8ToUnicode(const char *p_source_str, int p_source, uint2 *p_dest_str, int p_dest);

static int NativeToUnicode(const char *p_source_str, int p_source, uint2 *p_dest_str, int p_dest)
{
    if (p_dest_str == NULL)
        return p_source * 2;
    
    for(int i = 0; i < p_source; i++)
        p_dest_str[i] = MCUnicodeMapFromNative(p_source_str[i]);
    
    return p_source * 2;
}

void MCExecPoint::dounicodetomultibyte(bool p_native, bool p_reverse)
{
	const char *t_input;
	t_input = getsvalue() . getstring();

	uint4 t_input_length;
	t_input_length = getsvalue() . getlength();

	uint4 t_output_length;
	if (p_reverse)
	{
		if (p_native)
			t_output_length = NativeToUnicode(t_input, t_input_length, NULL, 0);
		else
			t_output_length = UTF8ToUnicode(t_input, t_input_length, NULL, 0);
	}
	else
	{
		if (p_native)
			MCS_unicodetomultibyte(t_input, t_input_length, NULL, 0, t_output_length, LCH_ROMAN);
		else
			t_output_length = UnicodeToUTF8((uint2 *)t_input, t_input_length, NULL, 0);
	}

	char *t_buffer;
	uint4 t_buffer_length;
	t_buffer_length = (t_output_length + EP_PAD) & EP_MASK;
	t_buffer = new char[t_buffer_length];

	if (p_reverse)
	{
		if (p_native)
			t_output_length = NativeToUnicode(t_input, t_input_length, (uint2 *)t_buffer, t_output_length);
		else
			t_output_length = UTF8ToUnicode(t_input, t_input_length, (uint2 *)t_buffer, t_output_length);
	}
	else
	{
		if (p_native)
			MCS_unicodetomultibyte(t_input, t_input_length, t_buffer, t_output_length, t_output_length, LCH_ROMAN);
		else
			t_output_length = UnicodeToUTF8((uint2 *)t_input, t_input_length, t_buffer, t_output_length);
	}

	delete buffer;

	buffer = t_buffer;
	size = t_buffer_length;

	svalue . set(buffer, t_output_length);
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] Switches the encoding of the ep to
//   unicode if 'to_unicode' is true, native otherwise. If the current encoding
//   matches (is_unicode) then nothing happens.
void MCExecPoint::mapunicode(bool p_is_unicode, bool p_to_unicode)
{
	if (p_is_unicode == p_to_unicode)
		return;

	if (p_to_unicode)
		nativetoutf16();
	else
		utf16tonative();
}

///////////////////////////////////////////////////////////////////////////////

void MCExecPoint::setempty(void)
{
	clear();
}

/////////

void MCExecPoint::setstaticcstring(const char *p_string)
{
	setsvalue(p_string);
}

void MCExecPoint::setstaticbytes(const void *p_bytes, uindex_t p_length)
{
	setsvalue(MCString((const char *)p_bytes, p_length));
}

void MCExecPoint::setstaticmcstring(const MCString& p_mcstring)
{
	setsvalue(p_mcstring);
}


/////////

void MCExecPoint::setcstring(const char *p_cstring)
{
	copysvalue(p_cstring, strlen(p_cstring));
}

void MCExecPoint::setmcstring(const MCString& p_mcstring)
{
	copysvalue(p_mcstring . getstring(), p_mcstring . getlength());
}

void MCExecPoint::setchars(const char *p_cstring, uindex_t p_length)
{
	copysvalue(p_cstring, p_length);
}

void MCExecPoint::setchar(char p_char)
{
	copysvalue(&p_char, 1);
}

void MCExecPoint::setstringf(const char *p_spec, ...)
{
	va_list t_args;
	int t_count;

#if defined(_HAS_VSCPRINTF)
	va_start(t_args, p_spec);
	t_count = _vscprintf(p_spec, t_args);
	va_end(t_args);
#elif defined(_HAS_VSNPRINTF)
	va_start(t_args, p_spec);
	t_count = vsnprintf(nil, 0, p_spec, t_args);
	va_end(t_args);
#else
#error MCExecPoint::setstringf not implemented
#endif

	va_start(t_args, p_spec);
	vsprintf(getbuffer(t_count + 1), p_spec, t_args);
	va_end(t_args);

	setstrlen();
}

/////////

void MCExecPoint::setboolean(Boolean p_value)
{
	setsvalue(p_value ? MCtruemcstring : MCfalsemcstring);
}

void MCExecPoint::setpoint(int16_t x, int16_t y)
{
	sprintf(getbuffer(I2L * 2 + 2), "%d,%d", x, y);
	setstrlen();
}

void MCExecPoint::setrectangle(const MCRectangle& p_rect)
{
	sprintf(getbuffer(I4L * 4 + 4), "%d,%d,%d,%d", p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
	setstrlen();
}

void MCExecPoint::setrectangle(const MCRectangle32& p_rect)
{
	sprintf(getbuffer(I4L * 4 + 4), "%d,%d,%d,%d", p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
	setstrlen();
}

void MCExecPoint::setrectangle(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	sprintf(getbuffer(I4L * 4 + 4), "%d,%d,%d,%d", p_left, p_top, p_right, p_bottom);
	setstrlen();
}

void MCExecPoint::setcolor(const MCColor& p_color, const char *p_color_name)
{
	if (p_color_name != nil)
		copysvalue(p_color_name);
	else
		setcolor(p_color);
}

void MCExecPoint::setcolor(const MCColor& p_color)
{
	setcolor(p_color . red >> 8, p_color . green >> 8, p_color . blue >> 8);
}

void MCExecPoint::setcolor(uint32_t r, uint32_t g, uint32_t b)
{
	sprintf(getbuffer(U1L * 3 + 3), "%u,%u,%u", r & 0xff, g & 0xff, b & 0xff);
	setstrlen();
}

void MCExecPoint::setpixel(uint32_t p_pixel)
{
	setcolor((p_pixel >> 16) & 0xFF, (p_pixel >> 8) & 0xFF, (p_pixel >> 0) & 0xFF);
}

////////

void MCExecPoint::appendcstring(const char *p_string)
{
	concat(p_string, EC_NONE, True);
}

void MCExecPoint::appendmcstring(const MCString& p_string)
{
	concat(p_string, EC_NONE, True);
}

void MCExecPoint::appendstringf(const char *p_spec, ...)
{
	va_list t_args;
	int t_count;

#if defined(_HAS_VSCPRINTF)
	va_start(t_args, p_spec);
	t_count = _vscprintf(p_spec, t_args);
	va_end(t_args);
#elif defined(_HAS_VSNPRINTF)
	va_start(t_args, p_spec);
	t_count = vsnprintf(nil, 0, p_spec, t_args);
	va_end(t_args);
#else
#error MCExecPoint::setstringf not implemented
#endif

	if (t_count < 256)
	{
		char t_buffer[256];
		va_start(t_args, p_spec);
		vsprintf(t_buffer, p_spec, t_args);
		va_end(t_args);

		concat(t_buffer, EC_NONE, True);
	}
	else
	{
		char *t_buffer;
		t_buffer = (char *)malloc(t_count + 1);
		if (t_buffer != nil)
		{
			va_start(t_args, p_spec);
			vsprintf(t_buffer, p_spec, t_args);
			va_end(t_args);
			concat(t_buffer, EC_NONE, True);
			free(t_buffer);
		}
	}
}

void MCExecPoint::appendchars(const char *p_chars, uindex_t p_count)
{
	concat(MCString(p_chars, p_count), EC_NONE, True);
}

void MCExecPoint::appendchar(char p_char)
{
	appendchars(&p_char, 1);
}

void MCExecPoint::appendbytes(const void *p_bytes, uindex_t p_count)
{
	concat(MCString((const char *)p_bytes, p_count), EC_NONE, True);
}

void MCExecPoint::appendbyte(uint8_t p_byte)
{
	appendbytes(&p_byte, 1);
}

void MCExecPoint::appenduint(uint32_t p_integer)
{
	concat(p_integer, EC_NONE, True);
}

void MCExecPoint::appendint(int32_t p_integer)
{
	concat(p_integer, EC_NONE, True);
}

void MCExecPoint::appendnewline(void)
{
	appendchar('\n');
}

/////////

void MCExecPoint::appendnewline(bool unicode)
{
	if (!unicode)
		appendchar('\n');
	else
	{
		uint2 t_char;
		t_char = 10;
		appendbytes(&t_char, 2);
	}
}

/////////

void MCExecPoint::concatcstring(const char *p_string, Exec_concat p_sep, bool p_first)
{
	if (p_string == nil)
		p_string = "";

	concat(MCString(p_string, strlen(p_string)), p_sep, p_first);
}

void MCExecPoint::concatchars(const char *p_chars, uindex_t p_count, Exec_concat p_sep, bool p_first)
{
	concat(MCString(p_chars, p_count), p_sep, p_first);
}

void MCExecPoint::concatmcstring(const MCString& p_string, Exec_concat p_sep, bool p_first)
{
	concat(p_string, p_sep, p_first);
}

void MCExecPoint::concatuint(uint32_t p_value, Exec_concat p_sep, bool p_first)
{
	concat(p_value, p_sep, p_first);
}

void MCExecPoint::concatint(int32_t p_value, Exec_concat p_sep, bool p_first)
{
	concat(p_value, p_sep, p_first);
}

void MCExecPoint::concatreal(double p_value, Exec_concat p_sep, bool p_first)
{
	char *t_buffer;
	uint32_t t_buffer_size;
	t_buffer = nil;
	t_buffer_size = 0;

	uint32_t t_length;
	t_length = MCU_r8tos(t_buffer, t_buffer_size, p_value, nffw, nftrailing, nfforce);
	concat(MCString(t_buffer, t_length), p_sep, p_first);

	delete[] t_buffer;
}

/////////

void MCExecPoint::replacechar(char p_from, char p_to)
{
	grabsvalue();
	for(uint32_t i = 0; i < svalue . getlength(); i++)
		if (buffer[i] == p_from)
			buffer[i] = p_to;
}

/////////

void MCExecPoint::setnameref_unsafe(MCNameRef p_name)
{
	setstaticmcstring(MCNameGetOldString(p_name));
}

void MCExecPoint::concatnameref(MCNameRef p_name, Exec_concat p_sep, bool p_first)
{
	concatmcstring(MCNameGetOldString(p_name), p_sep, p_first);
}

bool MCExecPoint::copyasnameref(MCNameRef& r_name)
{
	return MCNameCreateWithOldString(getsvalue(), r_name);
}

// Assumes the contents of the ep is UTF-16 and attempts conversion to native. If
// successful, the new contents is native and the method returns true, otherwise
// false and the contents remains unchanged.
bool MCExecPoint::trytoconvertutf16tonative()
{
    // if empty then it can be native obviously but this also avoids getsvalue returning nil
    if (isempty())
        return true;
    
    MCExecPoint t_other_ep;
    t_other_ep . setsvalue(getsvalue());
    t_other_ep . utf16tonative();
    
    // MERG-2013-05-07: [[ Bug 8884 ]] If returning true the function setting back to utf16 instead of to native
    MCExecPoint t_temp_ep;
    t_temp_ep . setsvalue(t_other_ep . getsvalue());
    t_temp_ep . nativetoutf16();
    
    if (getsvalue() . getlength() == t_temp_ep . getsvalue() . getlength()
        && memcmp(getsvalue() . getstring(), t_temp_ep . getsvalue() . getstring(), getsvalue() . getlength()) == 0)
        {
            copysvalue(t_other_ep . getsvalue(). getstring(), t_other_ep . getsvalue(). getlength());
            return true;
        }

    return false;
}

// SN-2015-06-03: [[ Bug 11277 ]] Refactor similar functions from MCHandler and
//  MCHandlerlist
void MCExecPoint::deletestatements(MCStatement *p_statements)
{
    while (p_statements != NULL)
    {
        MCStatement *tsptr = p_statements;
        p_statements = p_statements->getnext();
        delete tsptr;
    }
}

Exec_stat MCExecPoint::eval(MCExecPoint &ep)
{
    MCScriptPoint sp(ep);
    
    // SN-2015-06-03: [[ Bug 11277 ]] When we are out of handler, then it simply
    //  sets the ScriptPoint handler to NULL (post-constructor state).
    sp.sethandler(gethandler());
    
    MCExpression *exp = NULL;
    Symbol_type type;
    Exec_stat stat = ES_ERROR;
    if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
    {
        stat = exp->eval(*this);
        grabsvalue();
    }
    delete exp;
    return stat;
}

Exec_stat MCExecPoint::doscript(MCExecPoint& ep, uint2 line, uint2 pos)
{
    MCScriptPoint sp(ep);
    MCStatement *curstatement = NULL;
    MCStatement *statements = NULL;
    MCStatement *newstatement = NULL;
    Symbol_type type;
    const LT *te;
    Exec_stat stat = ES_NORMAL;
    Boolean oldexplicit = MCexplicitvariables;
    MCexplicitvariables = False;
    uint4 count = 0;
    sp.setline(line - 1);
    while (stat == ES_NORMAL)
    {
        switch (sp.next(type))
        {
            case PS_NORMAL:
                if (type == ST_ID)
                    if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
                        newstatement = new MCComref(sp.gettoken_nameref());
                    else
                    {
                        if (te->type != TT_STATEMENT)
                        {
                            MCeerror->add(EE_DO_NOTCOMMAND, line, pos, sp.gettoken());
                            stat = ES_ERROR;
                        }
                        else
                            newstatement = MCN_new_statement(te->which);
                    }
                    else
                    {
                        MCeerror->add(EE_DO_NOCOMMAND, line, pos, sp.gettoken());
                        stat = ES_ERROR;
                    }
                if (stat == ES_NORMAL)
                {
                    if (curstatement == NULL)
                        statements = curstatement = newstatement;
                    else
                    {
                        curstatement->setnext(newstatement);
                        curstatement = newstatement;
                    }
                    if (curstatement->parse(sp) != PS_NORMAL)
                    {
                        MCeerror->add(EE_DO_BADCOMMAND, line, pos, ep.getsvalue());
                        stat = ES_ERROR;
                    }
                    count += curstatement->linecount();
                }
                break;
            case PS_EOL:
                if (sp.skip_eol() != PS_NORMAL)
                {
                    MCeerror->add(EE_DO_BADLINE, line, pos, ep.getsvalue());
                    stat = ES_ERROR;
                }
                break;
            case PS_EOF:
                stat = ES_PASS;
                break;
            default:
                stat = ES_ERROR;
        }
    }
    MCexplicitvariables = oldexplicit;
    
    if (MClicenseparameters . do_limit > 0 && count >= MClicenseparameters . do_limit)
    {
        MCeerror -> add(EE_DO_NOTLICENSED, line, pos, ep . getsvalue());
        stat = ES_ERROR;
    }
    
    if (stat == ES_ERROR)
    {
        deletestatements(statements);
        return ES_ERROR;
    }
    MCExecPoint ep2(ep);
    while (statements != NULL)
    {
        Exec_stat stat = statements->exec(ep2);
        if (stat == ES_ERROR)
        {
            deletestatements(statements);
            MCeerror->add(EE_DO_BADEXEC, line, pos, ep.getsvalue());
            return ES_ERROR;
        }
        if (MCexitall || stat != ES_NORMAL)
        {
            deletestatements(statements);
            if (stat != ES_ERROR)
                stat = ES_NORMAL;
            return stat;
        }
        else
        {
            MCStatement *tsptr = statements;
            statements = statements->getnext();
            delete tsptr;
        }
    }
    if (MCscreen->abortkey())
    {
        MCeerror->add(EE_DO_ABORT, line, pos);
        return ES_ERROR;
    }
    return ES_NORMAL;
}
        