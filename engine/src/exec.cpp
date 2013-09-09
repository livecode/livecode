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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "uidc.h"
#include "securemode.h"
#include "exec.h"

#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::ForceToString(MCValueRef p_value, MCStringRef& r_string)
{
	return m_ep . convertvaluereftostring(p_value, r_string);
}

bool MCExecContext::ForceToBoolean(MCValueRef p_value, MCBooleanRef& r_boolean)
{
	return m_ep . convertvaluereftoboolean(p_value, r_boolean);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::ConvertToString(MCValueRef p_value, MCStringRef& r_string)
{
	return m_ep . convertvaluereftostring(p_value, r_string);
}

bool MCExecContext::ConvertToNumber(MCValueRef p_value, MCNumberRef& r_number)
{
	return m_ep . convertvaluereftonumber(p_value, r_number);
}

bool MCExecContext::ConvertToReal(MCValueRef p_value, real64_t& r_double)
{
	MCAutoNumberRef t_number;
	if (!m_ep . convertvaluereftonumber(p_value, &t_number))
		return false;
	r_double = MCNumberFetchAsReal(*t_number);
	return true;
}

bool MCExecContext::ConvertToInteger(MCValueRef p_value, integer_t& r_integer)
{
	MCAutoNumberRef t_number;
	if (!m_ep . convertvaluereftonumber(p_value, &t_number))
		return false;
	r_integer = MCNumberFetchAsInteger(*t_number);
	return true;
}

bool MCExecContext::ConvertToUnsignedInteger(MCValueRef p_value, uinteger_t& r_integer)
{
	MCAutoNumberRef t_number;
	if (!m_ep . convertvaluereftonumber(p_value, &t_number))
		return false;
	r_integer = MCNumberFetchAsUnsignedInteger(*t_number);
	return true;
}

bool MCExecContext::ConvertToBoolean(MCValueRef p_value, MCBooleanRef &r_boolean)
{
	return m_ep . convertvaluereftoboolean(p_value, r_boolean);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_value)
{
	return MCStringFormat(r_value, "%u", p_integer);
}

bool MCExecContext::FormatLegacyColor(MCColor p_color, MCStringRef& r_value)
{
	return MCStringFormat(r_value, "%d,%d,%d", p_color . red >> 8, p_color . green >> 8, p_color . blue >> 8);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::TryToConvertToUnsignedInteger(MCValueRef p_value, bool& r_converted, uinteger_t &r_integer)
{
	r_converted = m_ep . convertvaluereftouint(p_value, r_integer);
	return true;
}

bool MCExecContext::TryToConvertToReal(MCValueRef p_value, bool& r_converted, real64_t& r_real)
{
	r_converted = m_ep . convertvaluereftoreal(p_value, r_real);
	return true;
}

bool MCExecContext::TryToConvertToLegacyColor(MCValueRef p_value, bool& r_converted, MCColor& r_color)
{
	MCAutoStringRef t_string;
	if (m_ep . convertvaluereftostring(p_value, &t_string) &&
		MCscreen -> parsecolor(*t_string, r_color))
	{
		r_converted = true;
		return true;
	}
	else
		r_converted = false;
		
	return true;
}

bool MCExecContext::TryToConvertToLegacyPoint(MCValueRef p_value, bool& r_converted, MCPoint& r_point)
{
	MCAutoStringRef t_string;
	if (m_ep . convertvaluereftostring(p_value, &t_string) &&
		MCU_stoi2x2(MCStringGetOldString(*t_string), r_point . x, r_point . y))
	{
		r_converted = true;
		return true;
	}
	else
		r_converted = false;
		
	return true;
}

bool MCExecContext::TryToConvertToLegacyRectangle(MCValueRef p_value, bool& r_converted, MCRectangle& r_rect)
{
	MCAutoStringRef t_string;
	int16_t t_left, t_top, t_right, t_bottom;
	if (m_ep . convertvaluereftostring(p_value, &t_string) &&
		MCU_stoi2x4(MCStringGetOldString(*t_string), t_left, t_top, t_right, t_bottom))
	{
		r_rect . x = t_left;
		r_rect . y = t_top;
		r_rect . width = t_right - t_left;
		r_rect . height = t_bottom - t_top;
		
		r_converted = true;
		
		return true;
	}
	else
		r_converted = false;
		
	return true;
}

///////////////////////////////////////////////////////////////////////////////

bool MCExecContext::CopyElementAsBoolean(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCBooleanRef &r_boolean)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToBoolean(t_val, r_boolean));
}

bool MCExecContext::CopyElementAsString(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCStringRef &r_string)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToString(t_val, r_string));
}

bool MCExecContext::CopyElementAsNumber(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCNumberRef &r_number)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToNumber(t_val, r_number));
}

bool MCExecContext::CopyElementAsInteger(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, integer_t &r_integer)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToInteger(t_val, r_integer));
}

bool MCExecContext::CopyElementAsUnsignedInteger(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, uinteger_t &r_integer)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToUnsignedInteger(t_val, r_integer));
}

bool MCExecContext::CopyElementAsReal(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, real64_t &r_real)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToReal(t_val, r_real));
}

bool MCExecContext::CopyElementAsArray(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCArrayRef &r_array)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
	return (ConvertToArray(t_val, r_array));
}

bool MCExecContext::CopyElementAsStringArray(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCArrayRef &r_string_array)
{
	MCAutoArrayRef t_array;
	if (!CopyElementAsArray(p_array, p_key, p_case_sensitive, &t_array))
		return false;
	
	// Should be possible to add an optimization, if necessary:
	//	 don't copy the array if all elements are already strings
	
	MCAutoArrayRef t_string_array;
	if (!MCArrayCreateMutable(&t_string_array))
		return false;
		
	for (uindex_t i = 0; i < MCArrayGetCount(*t_array); i++)
	{
		MCValueRef t_val;
		MCAutoStringRef t_string;
		
		if (!MCArrayFetchValueAtIndex(*t_array, i, t_val))
			return false;
		
		if (!ConvertToString(t_val, &t_string))
			return false;
		
		if (!MCArrayStoreValueAtIndex(*t_array, i, *t_string))
			return false;
	}
	
	r_string_array = MCValueRetain(*t_string_array);
	return true;
}

bool MCExecContext::CopyElementAsFilepath(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCStringRef &r_path)
{
	MCAutoStringRef t_string, t_path;
	if (!CopyElementAsString(p_array, p_key, p_case_sensitive, &t_string))
		return false;
	
	if (!MCS_resolvepath(*t_string, &t_path))
		return false;
	
	r_path = MCValueRetain(*t_path);
	return true;
}

bool MCExecContext::CopyElementAsFilepathArray(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCArrayRef &r_path_array)
{
	MCAutoArrayRef t_array;
	if (!CopyElementAsStringArray(p_array, p_key, p_case_sensitive, &t_array))
		return false;
	
	MCAutoArrayRef t_path_array;
	if (!MCArrayCreateMutable(&t_path_array))
		return false;
	
	for (uindex_t i = 0; i < MCArrayGetCount(*t_array); i++)
	{
		MCValueRef t_val;
		if (!MCArrayFetchValueAtIndex(*t_array, i, t_val))
			return false;
		
		MCAutoStringRef t_path;
		if (!MCS_resolvepath((MCStringRef)t_val, &t_path))
			return false;
		
		if (!MCArrayStoreValueAtIndex(*t_path_array, i, *t_path))
			return false;
	}
	
	r_path_array = MCValueRetain(*t_path_array);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::CopyOptElementAsBoolean(MCArrayRef p_array, MCNameRef p_name, bool p_case_sensitive, MCBooleanRef &r_boolean)
{
	MCValueRef t_val;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_name, t_val))
	{
		r_boolean = MCValueRetain(kMCFalse);
		return true;
	}
	
	return CopyElementAsBoolean(p_array, p_name, p_case_sensitive, r_boolean);
}

bool MCExecContext::CopyOptElementAsString(MCArrayRef p_array, MCNameRef p_name, bool p_case_sensitive, MCStringRef &r_string)
{
	MCValueRef t_val;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_name, t_val))
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	return CopyElementAsString(p_array, p_name, p_case_sensitive, r_string);
}

bool MCExecContext::CopyOptElementAsStringArray(MCArrayRef p_array, MCNameRef p_name, bool p_case_sensitive, MCArrayRef &r_string_array)
{
	MCValueRef t_val;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_name, t_val))
	{
		r_string_array = MCValueRetain(kMCEmptyArray);
		return true;
	}
	
	return CopyElementAsStringArray(p_array, p_name, p_case_sensitive, r_string_array);
}

bool MCExecContext::CopyOptElementAsFilepath(MCArrayRef p_array, MCNameRef p_name, bool p_case_sensitive, MCStringRef &r_path)
{
	MCValueRef t_val;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_name, t_val))
	{
		r_path = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	return CopyElementAsFilepath(p_array, p_name, p_case_sensitive, r_path);
}

bool MCExecContext::CopyOptElementAsFilepathArray(MCArrayRef p_array, MCNameRef p_name, bool p_case_sensitive, MCArrayRef &r_path_array)
{
	MCValueRef t_val;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_name, t_val))
	{
		r_path_array = MCValueRetain(kMCEmptyArray);
		return true;
	}
	
	return CopyElementAsFilepathArray(p_array, p_name, p_case_sensitive, r_path_array); 
}

////////////////////////////////////////////////////////////////////////////////

bool FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_output)
{
	return MCStringFormat(r_output, "%d", p_integer);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::EncodeStringAsUTF8(MCStringRef self, char*& r_utf8_string)
{
	MCExecPoint ep;
	ep . setsvalue(MCStringGetOldString(self));
	ep . nativetoutf8();
	r_utf8_string = ep . getsvalue() . clone();
	return true;
}

bool MCExecContext::EncodeUnicodeStringAsUTF8(MCStringRef self, char*& r_utf8_string)
{
	MCExecPoint ep;
	ep . setsvalue(MCStringGetOldString(self));
	ep . utf16toutf8();
	r_utf8_string = ep . getsvalue() . clone();
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::EvaluateExpression(MCExpression *p_expr, MCValueRef& r_result)
{
	if (p_expr -> eval(m_ep) != ES_NORMAL)
	{
		LegacyThrow(EE_EXPR_EVALERROR);
		return false;
	}

	if (!m_ep . copyasvalueref(r_result))
	{
		Throw();
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::EnsurePrintingIsAllowed(void)
{
	if (MCSecureModeCheckPrinter(0, 0))
		return true;
		
	Throw();
	
	return false;
}

bool MCExecContext::EnsureDiskAccessIsAllowed(void)
{
	if (MCSecureModeCheckDisk(0, 0))
		return true;
		
	Throw();
	
	return false;
}

bool MCExecContext::EnsureProcessIsAllowed(void)
{
	if (MCSecureModeCheckProcess(0, 0))
		return true;

	Throw();

	return false;
}

bool MCExecContext::EnsureNetworkAccessIsAllowed(void)
{
	if (MCSecureModeCheckNetwork(0, 0))
		return true;

	Throw();

	return false;
}

bool MCExecContext::EnsurePrivacyIsAllowed(void)
{
	if (MCSecureModeCheckPrivacy(0, 0))
		return true;

	Throw();

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCExecContext::SetItToValue(MCValueRef p_value)
{
	assert(m_it != nil);
	
	MCVariable *t_var;
	t_var = m_it -> evalvar(m_ep);
	
	t_var -> setvalueref(p_value);
}

void MCExecContext::SetItToEmpty(void)
{
	SetItToValue(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

void MCExecContext::LegacyThrow(Exec_errors p_error, MCValueRef p_hint)
{
	MCeerror -> add(p_error, 0, 0, p_hint);
	m_stat = ES_ERROR;
}

void MCExecContext::LegacyThrow(Exec_errors p_error, uint32_t p_hint)
{
	MCeerror -> add(p_error, 0, 0, p_hint);
	m_stat = ES_ERROR;
}

MCObjectHandle *MCExecContext::GetObjectHandle(void)
{
    extern MCExecPoint *MCEPptr;
	return MCEPptr->getobj()->gethandle();
}

Exec_stat MCExecContext::Catch(uint2 p_line, uint2 p_pos)
{
	return ES_ERROR;
}

void MCExecContext::SetTheResultToEmpty(void)
{
	MCresult -> clear();
}

void MCExecContext::SetTheResultToValue(MCValueRef p_value)
{
	MCresult -> setvalueref(p_value);
}

void MCExecContext::SetTheResultToStaticCString(const char *p_cstring)
{
	MCresult -> sets(p_cstring);
}

void MCExecContext::SetTheResultToNumber(real64_t p_value)
{
    MCresult -> setnvalue(p_value);
}

void MCExecContext::GiveCStringToResult(char *p_cstring)
{
    MCresult -> grab(p_cstring, MCCStringLength(p_cstring));
}

void MCExecContext::SetTheResultToCString(const char *p_string)
{
    MCresult -> copysvalue(p_string);
}

////////////////////////////////////////////////////////////////////////////////
