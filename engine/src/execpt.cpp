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

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "object.h"
//#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "globals.h"
#include "handler.h"
#include "hndlrlst.h"
#include "osspec.h"
#include "uidc.h"

#include "osxprefix-legacy.h"

#ifdef MODE_SERVER
#include "srvscript.h"
#endif

//////////

#ifdef LEGACY_EXEC

bool MCExecPoint::isempty(void) const
{
	if (value == kMCEmptyName)
		return true;

	if (value == kMCEmptyArray)
		return true;

	if (MCValueGetTypeCode(value) == kMCValueTypeCodeString &&
		MCStringGetLength((MCStringRef)value) == 0)
		return true;

	return false;
}

bool MCExecPoint::isarray(void) const
{
	return MCValueGetTypeCode(value) == kMCValueTypeCodeArray;
}

bool MCExecPoint::isstring(void) const
{
	return MCValueGetTypeCode(value) == kMCValueTypeCodeString || MCValueGetTypeCode(value) == kMCValueTypeCodeName;
}

bool MCExecPoint::isnumber(void) const
{
	return MCValueGetTypeCode(value) == kMCValueTypeCodeNumber;
}

bool MCExecPoint::converttostring(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeString)
		return true;

	MCStringRef t_string;
	if (!convertvaluereftostring(value, t_string))
		return false;

	MCValueRelease(value);
	value = t_string;

	return true;
}

bool MCExecPoint::converttomutablestring(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeString &&
		MCStringIsMutable((MCStringRef)value))
		return true;

	converttostring();
	
	if (!MCStringIsMutable((MCStringRef)value))
	{
		MCStringRef t_new_value;
		if (!MCStringMutableCopyAndRelease((MCStringRef)value, t_new_value))
			return false;
		value = t_new_value;
	}

	return true;
}

bool MCExecPoint::converttonumber(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeNumber)
		return true;

	MCNumberRef t_number;
	if (!convertvaluereftonumber(value, t_number))
		return false;

	MCValueRelease(value);
	value = t_number;

	return true;
}

bool MCExecPoint::converttoboolean(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeBoolean)
		return true;

	MCBooleanRef t_boolean;
	if (!convertvaluereftoboolean(value, t_boolean))
		return false;

	MCValueRelease(value);
	value = t_boolean;

	return true;
}

bool MCExecPoint::converttoarray(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeArray)
		return true;

	MCValueRelease(value);
	value = MCValueRetain(kMCEmptyArray);

	return true;
}

bool MCExecPoint::converttomutablearray(void)
{
	MCArrayRef t_mutable_array;
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeArray)
	{
		if (MCArrayMutableCopyAndRelease((MCArrayRef)value, t_mutable_array))
		{
			MCValueRelease(value);
			value = t_mutable_array;
			return true;
		}

		return false;
	}

	if (MCArrayCreateMutable(t_mutable_array))
	{
		MCValueRelease(value);
		value = t_mutable_array;
		return true;
	}

	return false;
}

//////////

void MCExecPoint::clear(void)
{
	if (value == kMCEmptyString)
		return;

	MCValueRelease(value);
	value = MCValueRetain(kMCEmptyString);
}

void MCExecPoint::setsvalue(const MCString& p_string)
{
	MCStringRef t_string;
	if (!MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), t_string))
		return;

	MCValueRelease(value);
	value = t_string;
}

void MCExecPoint::copysvalue(const MCString& p_string)
{
	setsvalue(p_string);
}

void MCExecPoint::copysvalue(const char *p_string, uindex_t p_length)
{
	setsvalue(MCString(p_string, p_length));
}

void MCExecPoint::setnvalue(real8 p_number)
{
	MCNumberRef t_number;
	if (!MCNumberCreateWithReal(p_number, t_number))
		return;

	MCValueRelease(value);
	value = t_number;
}

void MCExecPoint::setnvalue(integer_t p_integer)
{
	MCNumberRef t_number;
	if (!MCNumberCreateWithInteger(p_integer, t_number))
		return;

	MCValueRelease(value);
	value = t_number;
}

void MCExecPoint::setnvalue(uinteger_t p_integer)
{
	if (p_integer <= INTEGER_MAX)
	{
		setnvalue((integer_t)p_integer);
		return;
	}
	setnvalue((real8)p_integer);
}

void MCExecPoint::grabbuffer(char *p_buffer, uindex_t p_length)
{
	copysvalue(p_buffer, p_length);
	delete p_buffer;
}

bool MCExecPoint::reserve(uindex_t p_capacity, char*& r_buffer)
{
	MCDataRef t_string;
	if (!MCDataCreateMutable(p_capacity, t_string))
		return false;

	MCDataPad(t_string, 0, p_capacity);
	
	MCValueRelease(value);
	value = t_string;
	r_buffer = (char *)MCDataGetBytePtr(t_string);
	return true;
}

void MCExecPoint::commit(uindex_t p_size)
{
	MCDataRemove((MCDataRef)value, MCRangeMake(p_size, UINDEX_MAX));
}

/*bool MCExecPoint::modify(char*& r_buffer, uindex_t& r_length)
{
	converttostring();
	
	MCDataRef t_data;
	MCDataCreateWithBytes(MCStringGetNativeCharPtr((MCStringRef)value), MCStringGetLength((MCStringRef)value), t_data);

	MCValueRelease(value);
	MCDataMutableCopyAndRelease(t_data, (MCDataRef&)value);
	
	r_buffer = (char *)MCDataGetBytePtr((MCDataRef)value);
	r_length = MCDataGetLength((MCDataRef)value);

	return true;
}

void MCExecPoint::resize(uindex_t p_size)
{
	MCDataRemove((MCDataRef)value, MCRangeMake(p_size, UINDEX_MAX));
}*/

//////////

const char *MCExecPoint::getcstring(void)
{
	return getsvalue() . getstring();
}

MCString MCExecPoint::getsvalue0(void)
{
	return getsvalue();
}

MCString MCExecPoint::getsvalue(void)
{
	converttostring();

	return MCStringGetOldString((MCStringRef)value);
}

real8 MCExecPoint::getnvalue(void)
{
	if (!converttonumber())
		return 0.0;

	return MCNumberFetchAsReal((MCNumberRef)value);
}

///////////

Exec_stat MCExecPoint::tos()
{
	if (isarray())
		return ES_ERROR;

	if (!converttostring())
		return ES_ERROR;

	return ES_NORMAL;
}

Exec_stat MCExecPoint::ton()
{
	if (isarray())
		return ES_ERROR;

	if (!converttonumber())
		return ES_ERROR;

	return ES_NORMAL;
}

Exec_stat MCExecPoint::tona(void)
{
	if (!isarray() &&
		!converttonumber())
		return ES_ERROR;

	return ES_NORMAL;
}

uint4 MCExecPoint::getuint4()
{
	if (!converttonumber())
		return 0;
	return MCNumberFetchAsUnsignedInteger((MCNumberRef)value);
}

uint2 MCExecPoint::getuint2()
{
	if (!converttonumber())
		return 0;
	return MCNumberFetchAsInteger((MCNumberRef)value);
}

int4 MCExecPoint::getint4()
{
	if (!converttonumber())
		return 0;
	return MCNumberFetchAsInteger((MCNumberRef)value);
}

Exec_stat MCExecPoint::getreal8(real8& d, uint2 l, uint2 p, Exec_errors e)
{
	if (!converttonumber())
	{
		MCeerror->add(EE_VARIABLE_NAN, l, p, value);
		MCeerror->add(e, l, p, value);
		return ES_ERROR;
	}
	d = MCNumberFetchAsReal((MCNumberRef)value);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getuint2(uint2& d, uint2 l, uint2 p, Exec_errors e)
{
	if (!converttonumber())
	{
		MCeerror->add(EE_VARIABLE_NAN, l, p, value);
		MCeerror->add(e, l, p, value);
		return ES_ERROR;
	}
	d = MCNumberFetchAsInteger((MCNumberRef)value);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getint4(int4& d, uint2 l, uint2 p, Exec_errors e)
{
	if (!converttonumber())
	{
		MCeerror->add(EE_VARIABLE_NAN, l, p, value);
		MCeerror->add(e, l, p, value);
		return ES_ERROR;
	}
	d = MCNumberFetchAsInteger((MCNumberRef)value);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getuint4(uint4& d, uint2 l, uint2 p, Exec_errors e)
{
	if (!converttonumber())
	{
		MCeerror->add(EE_VARIABLE_NAN, l, p, value);
		MCeerror->add(e, l, p, value);
		return ES_ERROR;
	}
	d = MCNumberFetchAsUnsignedInteger((MCNumberRef)value);
	return ES_NORMAL;
}

Exec_stat MCExecPoint::getboolean(Boolean& d, uint2 l, uint2 p, Exec_errors e)
{
	if (!converttoboolean())
	{
		MCeerror -> add(e, l, p, value);
		return ES_ERROR;
	}

	d = (value == kMCTrue ? True : False);
	return ES_NORMAL;
}

void MCExecPoint::setint(int4 i)
{
	setnvalue(i);
}

void MCExecPoint::setuint(uint4 i)
{
	setnvalue(i);
}

void MCExecPoint::setint64(int64_t i)
{
	if (i >= INTEGER_MIN && i <= INTEGER_MAX)
	{
		setnvalue((integer_t)i);
		return;
	}
	setnvalue((real8)i);
}

void MCExecPoint::setuint64(uint64_t i)
{
	if (i < INTEGER_MAX)
	{
		setnvalue((integer_t)i);
		return;
	}
	setnvalue((real8)i);
}

void MCExecPoint::setr8(real8 n, uint2 fw, uint2 trailing, uint2 force)
{
	setnvalue(n);
}

//////////

Exec_stat MCExecPoint::ntos(void)
{
	if (!converttostring())
		return ES_ERROR;
	return ES_NORMAL;
}

Exec_stat MCExecPoint::ston(void)
{
	if (!converttonumber())
		return ES_ERROR;
	return ES_NORMAL;
}


// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setitemdel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || MCStringGetLength((MCStringRef)value) != 1)
	{
		MCeerror->add
		(EE_VARIABLE_NAC, l, p, value);
		return ES_ERROR;
	}
	itemdel = MCStringGetNativeCharAtIndex((MCStringRef)value, 0);
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setcolumndel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || MCStringGetLength((MCStringRef)value) != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, value);
		return ES_ERROR;
	}
	columndel = MCStringGetNativeCharAtIndex((MCStringRef)value, 0);
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setrowdel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || MCStringGetLength((MCStringRef)value) != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, value);
		return ES_ERROR;
	}
	rowdel = MCStringGetNativeCharAtIndex((MCStringRef)value, 0);
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 5123 ]] - Strict array checking modification
Exec_stat MCExecPoint::setlinedel(uint2 l, uint2 p)
{
	if (tos() != ES_NORMAL || MCStringGetLength((MCStringRef)value) != 1)
	{
		MCeerror->add(EE_VARIABLE_NAC, l, p, value);
		return ES_ERROR;
	}
	linedel = MCStringGetNativeCharAtIndex((MCStringRef)value, 0);
	return ES_NORMAL;
}

void MCExecPoint::setnumberformat()
{
    MCAutoStringRef t_value;
    copyasstringref(&t_value);
	MCU_setnumberformat(*t_value, nffw, nftrailing, nfforce);
}

//////////

void MCExecPoint::insert(const MCString& p_string, uint32_t p_start, uint32_t p_finish)
{
	converttomutablestring();

	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), &t_string);
	MCStringReplace((MCStringRef)value, MCRangeMake(p_start, p_finish - p_start), *t_string);
}

uint1 *MCExecPoint::pad(char p_value, uint32_t p_count)
{
	converttomutablestring();

	uindex_t t_length;
	t_length = MCStringGetLength((MCStringRef)value);

	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars((const char_t *)&p_value, 1, &t_string);
	MCStringPad((MCStringRef)value, MCStringGetLength((MCStringRef)value), p_count, *t_string);

	return (uint1 *)MCStringGetNativeCharPtr((MCStringRef)value) + t_length;
}

void MCExecPoint::substring(uint32_t p_start, uint32_t p_finish)
{
	converttostring();

	MCStringRef t_new_string;
	if (!MCStringCopySubstringAndRelease((MCStringRef)value, MCRangeMake(p_start, p_finish - p_start), t_new_string))
		return;
	value = t_new_string;
}

void MCExecPoint::tail(uint32_t p_count)
{
	converttostring();

	uindex_t t_length;
	t_length = MCStringGetLength((MCStringRef)value);

	substring(p_count, t_length);
}

void MCExecPoint::fill(uint32_t p_start, char p_char, uint32_t p_count)
{
	converttomutablestring();

	uindex_t t_length;
	t_length = MCStringGetLength((MCStringRef)value);

	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars((const char_t *)&p_char, 1, &t_string);
	MCStringPad((MCStringRef)value, p_start, p_count, *t_string);
}

void MCExecPoint::concat(const MCString &two, Exec_concat ec, Boolean first)
{
	converttomutablestring();

	if (first || ec == EC_NONE)
		MCStringAppendNativeChars((MCStringRef)value, (const char_t *)two . getstring(), two . getlength());
	else
	{	
		char_t t_del;
		switch (ec)
		{
		case EC_SPACE:
			t_del = ' ';
			break;
		case EC_COMMA:
			t_del = ',';
			break;
		case EC_NULL:
			t_del = '\0';
			break;
		case EC_RETURN:
			t_del = '\n';
			break;

		// MW-2009-06-17: Can now concatenate with tab into an EP.
		case EC_TAB:
			t_del = '\t';
			break;
		}
		MCStringAppendNativeChars((MCStringRef)value, &t_del, 1);
		MCStringAppendNativeChars((MCStringRef)value, (const char_t *)two . getstring(), two . getlength());
	}
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

void MCExecPoint::texttobinary(void)
{
	MCDataRef t_data;
	copyasdataref(t_data);
	
	MCDataMutableCopyAndRelease(t_data, t_data);
	
	char *s;
	uint32_t l;
	s = (char *)MCDataGetBytePtr(t_data);
	l = MCDataGetLength(t_data);

	char *sptr, *dptr, *eptr;
	sptr = s;
	dptr = s;
	eptr = s + l;

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

	MCDataRemove(t_data, MCRangeMake(l, UINDEX_MAX));
	
	setvalueref(t_data);
}

void MCExecPoint::binarytotext(void)
{
#ifdef __CRLF__
	MCDataRef t_data;
	copyasdataref(t_data);
	
	MCDataMutableCopyAndRelease(t_data, t_data);

	char *sptr;
	uint32_t l;
	sptr = (char*)MCDataGetBytePtr(t_data);
	l = MCDataGetLength(t_data);

	uint32_t pad;
	pad = 0;
	for(uint32_t i = 0; i < l; i++)
		if (*sptr++ == '\n')
			pad++;

	if (pad != 0)
	{
		uint4 newsize;
		MCStringPad((MCStringRef)value, MCStringGetLength((MCStringRef)value), pad, nil);
		newsize = MCDataGetLength(t_data);

		char *newbuffer = sptr;
		sptr += l;
		char *dptr = newbuffer + newsize;
		while (dptr > sptr)
		{
			*--dptr = *--sptr;
			if (*sptr == '\n')
				*--dptr = '\r';
		}
	}
	setvalueref(t_data);
	MCValueRelease(t_data);

#elif defined(__CR__)
	MCDataRef t_data;
	copyasdataref(t_data);
	
	MCDataMutableCopyAndRelease(t_data, t_data);
	
	char *sptr;
	uint32_t l;
	sptr = (char *)MCDataGetBytePtr(t_data);
	l = MCDataGetLength(t_data);
	for (uint32_t i = 0 ; i < l ; i++)
	{
		if (*sptr == '\n')
			*sptr = '\r';
		sptr++;
	}
	setvalueref(t_data);
#endif
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

// MW-2013-11-08: [[ RefactorIt ]] Returns the it var for the current context.
MCVarref *MCExecPoint::getit(void)
{
	// If we have a handler, then get it from there.
	if (curhandler != nil)
		return curhandler -> getit();
	
#ifdef MODE_SERVER
	// If we are here it means we must be in global scope, executing in a
	// MCServerScript object.
	return static_cast<MCServerScript *>(curobj) -> getit();
#else
	// We should never get here as execution only occurs within handlers unless
	// in server mode.
	assert(false);
	return nil;
#endif
}

///////////////////////////////////////////////////////////////////////////////

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
            MCU_multibytetounicode(t_input, t_input_length, NULL, 0, t_output_length, LCH_ROMAN);
		else
			t_output_length = UTF8ToUnicode(t_input, t_input_length, NULL, 0);
	}
	else
	{
		if (p_native)
            MCU_unicodetomultibyte(t_input, t_input_length, NULL, 0, t_output_length, LCH_ROMAN);
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
            MCU_multibytetounicode(t_input, t_input_length, t_buffer, t_output_length, t_output_length, LCH_ROMAN);
		else
			t_output_length = UTF8ToUnicode(t_input, t_input_length, (uint2 *)t_buffer, t_output_length);
	}
	else
	{
		if (p_native)
            MCU_unicodetomultibyte(t_input, t_input_length, t_buffer, t_output_length, t_output_length, LCH_ROMAN);
		else
			t_output_length = UnicodeToUTF8((uint2 *)t_input, t_input_length, t_buffer, t_output_length);
	}

	grabbuffer(t_buffer, t_output_length);
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
	MCStringRef t_string;
	
	va_list t_args;
	va_start(t_args, p_spec);
	MCStringFormatV(t_string, p_spec, t_args);
	va_end(t_args);

	MCValueRelease(value);
	value = t_string;
}

/////////

void MCExecPoint::setbool(bool p_value)
{
	setboolean(p_value);
}

void MCExecPoint::setboolean(Boolean p_value)
{
	MCValueRelease(value);
	value = MCValueRetain(p_value == True ? kMCTrue : kMCFalse);
	//setsvalue(p_value ? MCtruemcstring : MCfalsemcstring);
}

void MCExecPoint::setpoint(int16_t x, int16_t y)
{
	setstringf("%d,%d", x, y);
}

void MCExecPoint::setpoint(MCPoint p_point)
{
	setpoint(p_point . x, p_point . y);
}

void MCExecPoint::setrectangle(const MCRectangle& p_rect)
{
	setstringf("%d,%d,%d,%d", p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
}

void MCExecPoint::setrectangle(const MCRectangle32& p_rect)
{
	setstringf("%d,%d,%d,%d", p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
}

void MCExecPoint::setrectangle(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	setstringf("%d,%d,%d,%d", p_left, p_top, p_right, p_bottom);
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
	setstringf("%u,%u,%u", r & 0xff, g & 0xff, b & 0xff);
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
	converttomutablestring();

	va_list t_args;
	va_start(t_args, p_spec);
	MCStringAppendFormatV((MCStringRef)value, p_spec, t_args);
	va_end(t_args);
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

////////////////////////////////////////////////////////////////////////////////

bool MCExecPoint::copyasbool(bool& r_value)
{
	if (!converttoboolean())
		return false;

	r_value = (value == kMCTrue);
	return true;
}

bool MCExecPoint::copyasint(integer_t& r_value)
{
	if (!converttonumber())
		return false;

	r_value = MCNumberFetchAsInteger((MCNumberRef)value);
	return true;
}

bool MCExecPoint::copyasuint(uinteger_t& r_value)
{
	if (!converttonumber())
		return false;

	r_value = MCNumberFetchAsUnsignedInteger((MCNumberRef)value);
	return true;
}

bool MCExecPoint::copyasdouble(double& r_value)
{
	if (!converttonumber())
		return false;

	r_value = MCNumberFetchAsReal((MCNumberRef)value);
	return true;
}

bool MCExecPoint::copyaschar(char_t& r_value)
{
	if (!converttostring())
		return false;
	
	if (MCStringGetLength((MCStringRef)value) != 1)
		return false;
	
	r_value = MCStringGetNativeCharAtIndex((MCStringRef)value, 0);
	return true;
}

bool MCExecPoint::copyasnumber(MCNumberRef& r_value)
{
	if (!converttonumber())
		return false;

	r_value = MCValueRetain((MCNumberRef)value);
	return true;
}

bool MCExecPoint::copyasstring(MCStringRef& r_value)
{
	if (!converttostring())
		return false;

	return MCStringCopy((MCStringRef)value, r_value);
}

bool MCExecPoint::copyasarray(MCArrayRef& r_value)
{
	if (!converttoarray())
		return false;

	return MCArrayCopy((MCArrayRef)value, r_value);
}

bool MCExecPoint::copyasvariant(MCValueRef& r_value)
{
	return MCValueCopy(value, r_value);
}

bool MCExecPoint::copyaspoint(MCPoint& r_value)
{
	if (!converttostring())
		return false;

	return MCU_stoi2x2((MCStringRef)value, r_value . x, r_value . y) == True;
}

////////////////////////////////////////////////////////////////////////////////

void MCExecPoint::concatnameref(MCNameRef p_name, Exec_concat p_sep, bool p_first)
{
	concatstringref(MCNameGetString(p_name), p_sep, p_first);
}

bool MCExecPoint::copyasnameref(MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!convertvaluereftostring(value, t_string))
		return false;
	return MCNameCreateAndRelease(t_string, r_name);
}

void MCExecPoint::concatstringref(MCStringRef p_string, Exec_concat p_sep, bool p_first)
{
	concatmcstring(MCStringGetOldString(p_string), p_sep, p_first);
}

bool MCExecPoint::copyasstringref(MCStringRef& r_string)
{
	return convertvaluereftostring(value, r_string);
}

bool MCExecPoint::copyasmutablestringref(MCStringRef& r_string)
{
	if (!converttostring())
		return false;

	return MCStringMutableCopy((MCStringRef)value, r_string);
}

bool MCExecPoint::copyasdataref(MCDataRef& r_data)
{
	MCAutoStringRef t_string;
	if (!copyasstringref(&t_string))
		return false;
    
	return MCDataCreateWithBytes((const byte_t *)MCStringGetNativeCharPtr(*t_string), MCStringGetLength(*t_string), r_data);
}
#endif

MCValueRef MCExecPoint::getvalueref(void)
{
	return value;
}

bool MCExecPoint::setvalueref(MCValueRef p_value)
{
	MCValueRef t_new_value;
	if (!MCValueCopy(p_value, t_new_value))
		return false;
	MCValueRelease(value);
	value = t_new_value;
	return true;
}

bool MCExecPoint::setvalueref_nullable(MCValueRef p_value)
{
	if (p_value == nil)
    {
        setvalueref(kMCEmptyString);
		return true;
	}
	return setvalueref(p_value);
}

bool MCExecPoint::copyasvalueref(MCValueRef& r_value)
{
	return MCValueCopy(value, r_value);
}

#ifdef LEGACY_EXEC
bool MCExecPoint::convertvaluereftostring(MCValueRef p_value, MCStringRef& r_string)
{
    return m_ec . ConvertToString(p_value, r_string);
}

bool MCExecPoint::convertvaluereftonumber(MCValueRef p_value, MCNumberRef& r_number)
{
    return m_ec . ConvertToNumber(p_value, r_number);
}

bool MCExecPoint::convertvaluereftobool(MCValueRef p_value, bool& r_bool)
{
    if ((MCBooleanRef)p_value == kMCTrue)
    {
        r_bool = true;
        return true;
    }
    
    MCAutoBooleanRef t_boolean;
    if (convertvaluereftoboolean(p_value, &t_boolean))
    {
        r_bool = *t_boolean == kMCTrue;
        return true;
    }
    
    return false;
}

bool MCExecPoint::convertvaluereftouint(MCValueRef p_value, uinteger_t& r_uinteger)
{
	MCAutoNumberRef t_number;
	if (!convertvaluereftonumber(p_value, &t_number))
		return false;

	r_uinteger = MCNumberFetchAsUnsignedInteger(*t_number);

	return true;
}

bool MCExecPoint::convertvaluereftoint(MCValueRef p_value, integer_t& r_integer)
{
	MCAutoNumberRef t_number;
	if (!convertvaluereftonumber(p_value, &t_number))
		return false;

	r_integer = MCNumberFetchAsInteger(*t_number);

	return true;
}

bool MCExecPoint::convertvaluereftoreal(MCValueRef p_value, real64_t& r_real)
{
	switch(MCValueGetTypeCode(p_value))
	{
	case kMCValueTypeCodeNull:
		{
			r_real = 0.0;
			return true;
		}
	case kMCValueTypeCodeBoolean:
	case kMCValueTypeCodeArray:
		break;
	case kMCValueTypeCodeNumber:
		{
			r_real = MCNumberFetchAsReal((MCNumberRef)p_value);
			return true;
		}
	case kMCValueTypeCodeName:
		{
			double t_number;
			t_number = 0.0;
			if (MCStringGetLength(MCNameGetString((MCNameRef)p_value)) != 0)
				if (!MCU_stor8(MCStringGetOldString(MCNameGetString((MCNameRef)p_value)), t_number, convertoctals))
					break;
			r_real = t_number;
			return true;
		}
	case kMCValueTypeCodeString:
		{
			double t_number;
			t_number = 0.0;
			if (MCStringGetLength((MCStringRef)p_value) != 0)
				if (!MCU_stor8(MCStringGetOldString((MCStringRef)p_value), t_number, convertoctals))
					break;
			r_real = t_number;
			return true;
		}
	default:
		break;
	}

	return false;
}

bool MCExecPoint::convertvaluereftoboolean(MCValueRef p_value, MCBooleanRef& r_boolean)
{
    return m_ec . ConvertToBoolean(p_value, r_boolean);
}

//////////

MCArrayRef MCExecPoint::getarrayref(void)
{
	return (MCArrayRef)value;
}

bool MCExecPoint::copyasarrayref(MCArrayRef& r_array)
{
	if (!converttoarray())
		return false;

	return MCArrayCopy((MCArrayRef)value, r_array);
}

bool MCExecPoint::copyasmutablearrayref(MCArrayRef& r_array)
{
	if (!converttoarray())
		return false;

	return MCArrayMutableCopy((MCArrayRef)value, r_array);
}


bool MCExecPoint::listarraykeys(MCArrayRef p_array, char p_delimiter)
{
	MCStringRef t_string;
	if (!MCArrayListKeys(p_array, p_delimiter, t_string))
		return false;

	MCValueRelease(value);
	value = t_string;
	return true;
}

// MW-2012-07-26: This should only return 'false' if copying the value failed.
//   Otherwise, it should just set the ep to empty.
bool MCExecPoint::fetcharrayelement(MCArrayRef p_array, MCNameRef p_key)
{
	MCValueRef t_value;
	if (!MCArrayFetchValue(p_array, getcasesensitive() == True, p_key, t_value))
	{
		clear();
		return true;
	}

	if (MCValueCopy(t_value, t_value))
	{
		MCValueRelease(value);
		value = t_value;
		return true;
	}

	return false;
}

bool MCExecPoint::storearrayelement(MCArrayRef p_array, MCNameRef p_key)
{
	MCAutoValueRef t_value;
	if (!MCValueCopy(value, &t_value))
		return false;
	return MCArrayStoreValue(p_array, getcasesensitive() == True, p_key, *t_value);
}

bool MCExecPoint::appendarrayelement(MCArrayRef p_array, MCNameRef p_key)
{
	MCValueRef t_value;
	if (!MCArrayFetchValue(p_array, getcasesensitive() == True, p_key, t_value))
		return false;

	MCAutoStringRef t_suffix;
	if (!convertvaluereftostring(value, &t_suffix))
		return false;

	MCStringRef t_current_value;
	if (!convertvaluereftostring(t_value, t_current_value))
		return false;
	
	bool t_success;
	t_success = MCStringMutableCopyAndRelease(t_current_value, t_current_value) &&
					MCStringAppend(t_current_value, *t_suffix) &&
						MCArrayStoreValue(p_array, getcasesensitive() == True, p_key, t_current_value);

	MCValueRelease(t_current_value);

	return t_success;
}

// MW-2012-07-26: This should only return 'false' if copying the value failed.
//   Otherwise, it should just set the ep to empty.
bool MCExecPoint::fetcharrayindex(MCArrayRef p_array, index_t p_index)
{
	MCValueRef t_value;
	t_value = nil;
	if (!MCArrayFetchValueAtIndex(p_array, p_index, t_value))
	{
		clear();
		return true;
	}
	
	if (MCValueCopy(t_value, t_value))
	{
		MCValueRelease(value);
		value = t_value;
		return true;
	}

	return false;
}

bool MCExecPoint::storearrayindex(MCArrayRef p_array, index_t p_index)
{
	MCAutoValueRef t_value;
	if (!MCValueCopy(value, &t_value))
		return false;
	return MCArrayStoreValueAtIndex(p_array, p_index, *t_value);
}

bool MCExecPoint::fetcharrayelement_cstring(MCArrayRef p_array, const char *p_key)
{
	return fetcharrayelement_oldstring(p_array, p_key);
}

bool MCExecPoint::storearrayelement_cstring(MCArrayRef p_array, const char *p_key)
{
	return storearrayelement_oldstring(p_array, p_key);
}

bool MCExecPoint::hasarrayelement_cstring(MCArrayRef p_array, const char *p_key)
{
	return hasarrayelement_oldstring(p_array, p_key);
}

bool MCExecPoint::appendarrayelement_cstring(MCArrayRef p_array, const char *p_key)
{
	return hasarrayelement_oldstring(p_array, p_key);
}

bool MCExecPoint::fetcharrayelement_oldstring(MCArrayRef p_array, const MCString& p_key)
{
	MCNewAutoNameRef t_key;
	if (!MCNameCreateWithNativeChars((const char_t *)p_key . getstring(), p_key . getlength(), &t_key))
		return false;
	return fetcharrayelement(p_array, *t_key);
}

bool MCExecPoint::storearrayelement_oldstring(MCArrayRef p_array, const MCString& p_key)
{
	MCNewAutoNameRef t_key;
	if (!MCNameCreateWithNativeChars((const char_t *)p_key . getstring(), p_key . getlength(), &t_key))
		return false;
	return storearrayelement(p_array, *t_key);
}

bool MCExecPoint::hasarrayelement_oldstring(MCArrayRef p_array, const MCString& p_key)
{
	MCNewAutoNameRef t_key;
	if (!MCNameCreateWithNativeChars((const char_t *)p_key . getstring(), p_key . getlength(), &t_key))
		return false;
	MCValueRef t_value;
	return MCArrayFetchValue(p_array, getcasesensitive() == True, *t_key, t_value);
}

bool MCExecPoint::appendarrayelement_oldstring(MCArrayRef p_array, const MCString& p_key)
{
	MCNewAutoNameRef t_key;
	if (!MCNameCreateWithNativeChars((const char_t *)p_key . getstring(), p_key . getlength(), &t_key))
		return false;
	return appendarrayelement(p_array, *t_key);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCExecPoint::factorarray(MCExecPoint& p_other, Operators p_op)
{
	MCAssert(false);
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecPoint::copyaslegacypoint(MCPoint& r_point)
{
	if (!converttostring())
		return false;
	return MCU_stoi2x2(MCStringGetOldString((MCStringRef)value), r_point . x, r_point . y) == True;
}

bool MCExecPoint::copyaslegacyrectangle(MCRectangle& r_rectangle)
{
	if (!converttostring())
		return false;
		
	int16_t t_left, t_top, t_right, t_bottom;
	if (!MCU_stoi2x4(MCStringGetOldString((MCStringRef)value), t_left, t_top, t_right, t_bottom))
		return false;
		
	r_rectangle . x = t_left;
	r_rectangle . y = t_top;
	r_rectangle . width = MCU_max(t_right - t_left, 1);
	r_rectangle . height = MCU_max(t_bottom - t_top, 1);
	
	return true;
}

bool MCExecPoint::copyaslegacycolor(MCColor& r_color)
{
	if (!converttostring())
		return false;
	
	if (!MCscreen -> parsecolor((MCStringRef)value, r_color))
		return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if 0

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
			MCS_multibytetounicode(t_input, t_input_length, NULL, 0, t_output_length, LCH_ROMAN);
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
			MCS_multibytetounicode(t_input, t_input_length, t_buffer, t_output_length, t_output_length, LCH_ROMAN);
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

/////////

void MCExecPoint::setstringref_unsafe(MCStringRef p_string)
{
	setstaticmcstring(MCStringGetOldString(p_string));
}

void MCExecPoint::setstringref_nullable_unsafe(MCStringRef p_string)
{
	if (p_string != nil)
		setstaticmcstring(MCStringGetOldString(p_string));
	else
		clear();
}

void MCExecPoint::concatstringref(MCStringRef p_string, Exec_concat p_sep, bool p_first)
{
	concatmcstring(MCStringGetOldString(p_string), p_sep, p_first);
}

bool MCExecPoint::copyasstringref(MCStringRef& r_string)
{
	return MCStringCreateWithNativeChars(getsvalue() . getstring(), getsvalue() . getlength(), r_string);
}

#endif

/////////

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
        stat = exp->eval(ep);
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
#endif
