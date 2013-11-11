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
#include "field.h"

#include "osspec.h"

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

bool MCExecContext::ConvertToArray(MCValueRef p_value, MCArrayRef &r_array)
{
	if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray)
		r_array = MCValueRetain((MCArrayRef)p_value);
	else
		r_array = MCValueRetain(kMCEmptyArray);
	
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

bool MCExecContext::ConvertToMutableString(MCValueRef p_value, MCStringRef& r_string)
{
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string))
        return false;
    
    return MCStringMutableCopy(*t_string, r_string);
}

bool MCExecContext::ConvertToData(MCValueRef p_value, MCDataRef& r_data)
{
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string))
        return false;
    
	return MCDataCreateWithBytes((const byte_t *)MCStringGetNativeCharPtr(*t_string), MCStringGetLength(*t_string), r_data);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::FormatReal(real64_t p_real, MCStringRef& r_value)
{
	return MCU_r8tos(p_real, GetNumberFormatWidth(), GetNumberFormatTrailing(), GetNumberFormatForce(), r_value);
}

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
	MCAutoStringRef t_string;
	if (!CopyElementAsString(p_array, p_key, p_case_sensitive, &t_string))
		return false;
	
	MCAutoArrayRef t_string_array;
	if (!MCStringSplit(*t_string, MCSTR("\n"), nil, kMCStringOptionCompareExact, &t_string_array))
		return false;
		
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
	MCVariable *t_var;
	t_var = m_ep.getit() -> evalvar(m_ep);
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
    extern MCExecContext *MCECptr;
	return MCECptr->GetObject()->gethandle();
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

void MCExecContext::SetTheResultToBool(bool p_bool)
{
    MCresult -> sets(MCU_btos(p_bool));
}

////////////////////////////////////////////////////////////////////////////////

static bool MCPropertyFormatUIntList(uinteger_t *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
		if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppendFormat(*t_list, "%d", p_list[i]);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyFormatStringList(MCStringRef *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
        if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppend(*t_list, p_list[i]);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyFormatPointList(MCPoint *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
        if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppendFormat(*t_list, "%d,%d", p_list[i].x, p_list[i].y);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyParseUIntList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, uinteger_t*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<uinteger_t> t_list;
	
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_uint_string;
		uinteger_t t_d;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), &t_uint_string);
		
		if (t_success)
			t_success = MCU_stoui4(*t_uint_string, t_d);
		
		if (t_success)
			t_success = t_list . Push(t_d);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static bool MCPropertyParseStringList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, MCStringRef*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<MCStringRef> t_list;
    
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCStringRef t_string;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), t_string);
		
		if (t_success)
			t_success = t_list . Push(t_string);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static bool MCPropertyParsePointList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, MCPoint*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<MCPoint> t_list;
    
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_point_string;
        MCPoint t_point;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), &t_point_string);
        
        if (t_success)
            MCU_stoi2x2(*t_point_string, t_point . x, t_point . y);
        
		if (t_success)
			t_success = t_list . Push(t_point);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

void MCExecFetchProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCValueRef& r_value)
{
    MCExecPoint ep(nil,nil,nil);
    switch(prop -> type)
    {
        case kMCPropertyTypeAny:
        {
            MCAutoValueRef t_any;
            ((void(*)(MCExecContext&, void *, MCValueRef&))prop -> getter)(ctxt, mark, &t_any);
            if (!ctxt . HasError())
            {
                ep . setvalueref(*t_any);
            }
        }
            break;
            
        case kMCPropertyTypeBool:
        {
            bool t_value;
            ((void(*)(MCExecContext&, void *, bool&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setboolean(t_value ? True : False);
            }
        }
            break;
            
        case kMCPropertyTypeInt16:
        case kMCPropertyTypeInt32:
        {
            integer_t t_value;
            ((void(*)(MCExecContext&, void *, integer_t&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeUInt16:
        case kMCPropertyTypeUInt32:
        {
            uinteger_t t_value;
            ((void(*)(MCExecContext&, void *, uinteger_t&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setuint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            double t_value;
            ((void(*)(MCExecContext&, void *, double&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setnvalue(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeChar:
        {
            char_t t_value;
            ((void(*)(MCExecContext&, void *, char_t&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setchar((char)t_value);
            }
        }
            break;
            
        case kMCPropertyTypeString:
        case kMCPropertyTypeBinaryString:
        {
            MCAutoStringRef t_value;
            ((void(*)(MCExecContext&, void *, MCStringRef&))prop -> getter)(ctxt, mark, &t_value);
            if (!ctxt . HasError())
            {
                ep . setvalueref(*t_value);
            }
        }
            break;
            
        case kMCPropertyTypeName:
        {
            MCNewAutoNameRef t_value;
            ((void(*)(MCExecContext&, void *, MCNameRef&))prop->getter)(ctxt, mark, &t_value);
            if (!ctxt.HasError())
            {
                ep.setvalueref(*t_value);
            }
        }
            break;
            
        case kMCPropertyTypeColor:
        {
            MCColor t_value;
            ((void(*)(MCExecContext&, void *, MCColor&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setcolor(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            MCRectangle t_value;
            ((void(*)(MCExecContext&, void *, MCRectangle&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setrectangle(t_value);
            }
        }
            break;
            
        case kMCPropertyTypePoint:
        {
            MCPoint t_value;
            ((void(*)(MCExecContext&, void *, MCPoint&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setpoint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt16X2:
        {
            integer_t t_value[2];
            ((void(*)(MCExecContext&, void *, integer_t[2]))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setstringf("%d,%d", t_value[0], t_value[1]);
            }
        }
            break;
            
        case kMCPropertyTypeInt16X4:
        {
            integer_t t_value[4];
            ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                ep . setstringf("%d,%d,%d,%d", t_value[0], t_value[1], t_value[2], t_value[3]);
            }
        }
            break;
            
        case kMCPropertyTypeArray:
        {
            MCAutoArrayRef t_value;
            ((void(*)(MCExecContext&, void *, MCArrayRef&))prop -> getter)(ctxt, mark, &t_value);
            if (!ctxt . HasError())
            {
                if (*t_value != nil)
                    ep . setvalueref(*t_value);
                else
                    ep . clear();
            }
        }
            break;
            
        case kMCPropertyTypeEnum:
        {
            int t_value;
            ((void(*)(MCExecContext&, void *, int&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                bool t_found = false;
                MCExecEnumTypeInfo *t_enum_info;
                t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                for(uindex_t i = 0; i < t_enum_info -> count; i++)
                    if (t_enum_info -> elements[i] . value == t_value)
                    {
                        ep . setcstring(t_enum_info -> elements[i] . tag);
                        t_found = true;
                        break;
                    }
                
                if (!t_found)
                {
                    // THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
                    MCAssert(false);
                    return;
                }
            }
        }
            break;
            
        case kMCPropertyTypeOptionalEnum:
        {
            int t_value;
            int *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, int*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr == nil)
                    ep . clear();
                else
                {
                    bool t_found = false;
                    MCExecEnumTypeInfo *t_enum_info;
                    t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                    for(uindex_t i = 0; i < t_enum_info -> count; i++)
                        if (t_enum_info -> elements[i] . value == t_value)
                        {
                            ep . setcstring(t_enum_info -> elements[i] . tag);
                            t_found = true;
                            break;
                        }
                    
                    if (!t_found)
                    {
                        // THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
                        MCAssert(false);
                        return;
                    }
                }
            }
        }
            break;
            
        case kMCPropertyTypeSet:
        {
            unsigned int t_value;
            ((void(*)(MCExecContext&, void *, unsigned int&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                MCExecSetTypeInfo *t_seprop;
                t_seprop = (MCExecSetTypeInfo *)(prop -> type_info);
                
                bool t_first;
                t_first = true;
                
                ep . clear();
                for(uindex_t i = 0; i < t_seprop -> count; i++)
                    if (((1 << t_seprop -> elements[i] . bit) & t_value) != 0)
                    {
                        ep . concatcstring(t_seprop -> elements[i] . tag, EC_COMMA, t_first);
                        t_first = false;
                    }
                
            }
        }
            break;
            
        case kMCPropertyTypeCustom:
        {
            MCExecCustomTypeInfo *t_custom_info;
            t_custom_info = (MCExecCustomTypeInfo *)(prop -> type_info);
            
            MCAssert(t_custom_info -> size <= 64);
            
            char t_value[64];
            ((void(*)(MCExecContext&, void *, void *))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                MCAutoStringRef t_value_ref;
                ((MCExecCustomTypeFormatProc)t_custom_info -> format)(ctxt, t_value, &t_value_ref);
                ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
                if (!ctxt . HasError())
                {
                    ep . setvalueref(*t_value_ref);
                }
            }
            
        }
            break;
            
        case kMCPropertyTypeOptionalInt16:
        {
            integer_t t_value;
            integer_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, integer_t*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr == nil)
                    ep . clear();
                else
                    ep . setint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeOptionalUInt16:
        case kMCPropertyTypeOptionalUInt32:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, uinteger_t*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr == nil)
                    ep . clear();
                else
                    ep . setint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeOptionalString:
        {
            MCAutoStringRef t_value;
            ((void(*)(MCExecContext&, void *, MCStringRef&))prop -> getter)(ctxt, mark, &t_value);
            if (!ctxt . HasError())
            {
                if (*t_value == nil)
                    ep . clear();
                else
                    ep . setvalueref(*t_value);
            }
            
        }
            break;
            
        case kMCPropertyTypeOptionalPoint:
        {
            MCPoint t_value;
            MCPoint *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, MCPoint*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr == nil)
                    ep . clear();
                else
                    ep . setpoint(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeOptionalRectangle:
        {
            MCRectangle t_value;
            MCRectangle *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, MCRectangle*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr == nil)
                    ep . clear();
                else
                    ep . setrectangle(t_value);
            }
        }
			break;
            
        case kMCPropertyTypeLinesOfString:
        {
            MCStringRef* t_value;
            uindex_t t_count;
            ((void(*)(MCExecContext&, void *, uindex_t&, MCStringRef*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                MCAutoStringRef t_output;
                if (MCPropertyFormatStringList(t_value, t_count, '\n', &t_output))
                {
                    ep . setvalueref(*t_output);
                }
            }
			for(uindex_t i = 0; i < t_count; i++)
				MCValueRelease(t_value[i]);
			MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeLinesOfUInt:
        case kMCPropertyTypeItemsOfUInt:
        {
            uinteger_t* t_value;
            uindex_t t_count;
            ((void(*)(MCExecContext&, void *, uindex_t&, uinteger_t*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                MCAutoStringRef t_output;
                char_t t_delimiter;
                t_delimiter = prop -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
                if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, &t_output))
                {
                    ep . setvalueref(*t_output);
                }
            }
			MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeLinesOfPoint:
        {
            MCPoint* t_value;
            uindex_t t_count;
            ((void(*)(MCExecContext&, void *, uindex_t&, MCPoint*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                MCAutoStringRef t_output;
                if (MCPropertyFormatPointList(t_value, t_count, '\n', &t_output))
                {
                    ep . setvalueref(*t_output);
                }
            }
			MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeMixedBool:
        {
            bool t_mixed;
            bool t_value;
            bool *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, bool*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                    ep . setcstring(MCmixedstring);
                else
                    ep . setboolean(t_value ? True : False);
            }
        }
            break;
            
        case kMCPropertyTypeMixedUInt16:
        case kMCPropertyTypeMixedUInt32:
        {
            bool t_mixed;
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, uinteger_t*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                    ep . setcstring(MCmixedstring);
                else
                    ep . setuint(t_value);
            }
        }
            break;
    }
    
    ep . copyasvalueref(r_value);
}

void MCExecStoreProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCValueRef p_value)
{
    MCExecPoint ep(nil, nil, nil);
    if (p_value != nil)
        ep . setvalueref(p_value);
    else
        ep . clear();
    
    switch(prop -> type)
    {
        case kMCPropertyTypeAny:
        {
            MCAutoValueRef t_value;
            if (!ep . copyasvalueref(&t_value))
                return;
            ((void(*)(MCExecContext&, void *, MCValueRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeBool:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAB);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, bool))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt16:
        {
            integer_t t_value;
            if (!ep . copyasint(t_value) ||
                t_value < -32768 || t_value > 32767)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt32:
        {
            integer_t t_value;
            if (!ep . copyasint(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
         
        case kMCPropertyTypeUInt16:
        {
            uinteger_t t_value;
            if (!ep . copyasuint(t_value) ||
                t_value < 0 || t_value > 65535)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeUInt32:
        {
            uinteger_t t_value;
            if (!ep . copyasuint(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            double t_value;
            if (!ep . copyasdouble(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, double))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeChar:
        {
            char_t t_value;
            if (!ep . copyaschar(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAC);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, char_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeString:
        case kMCPropertyTypeBinaryString:
        {
            MCAutoStringRef t_value;
            if (!ep . copyasstringref(&t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAC);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeName:
        {
            MCNewAutoNameRef t_value;
            if (!ep.copyasnameref(&t_value))
                ctxt.LegacyThrow(EE_PROPERTY_NAC);
            if (!ctxt.HasError())
                ((void(*)(MCExecContext&, void *, MCNameRef))prop->setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeColor:
        {
            MCColor t_value;
            if (!ep . copyaslegacycolor(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NOTACOLOR);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCColor))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            MCRectangle t_value;
            if (!ep . copyaslegacyrectangle(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypePoint:
        {
            MCPoint t_value;
            if (!ep . copyaslegacypoint(t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAPOINT);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt16X2:
        {
            int2 a, b;
            if (!MCU_stoi2x2(ep . getsvalue(), a, b))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTPAIR);
            if (!ctxt . HasError())
            {
                integer_t t_value[2];
                t_value[0] = a;
                t_value[1] = b;
                ((void(*)(MCExecContext&, void *, integer_t[2]))prop -> setter)(ctxt, mark, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt16X4:
        {
            int2 a, b, c, d;
            if (!MCU_stoi2x4(ep . getsvalue(), a, b, c, d))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
            if (!ctxt . HasError())
            {
                integer_t t_value[4];
                t_value[0] = a;
                t_value[1] = b;
                t_value[2] = c;
                t_value[3] = d;
                ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> setter)(ctxt, mark, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt32X2:
        {
            int4 a, b;
            if (!MCU_stoi4x2(ep . getsvalue(), a, b))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTPAIR);
            if (!ctxt . HasError())
            {
                integer_t t_value[2];
                t_value[0] = a;
                t_value[1] = b;
                ((void(*)(MCExecContext&, void *, integer_t[2]))prop -> setter)(ctxt, mark, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt32X4:
        {
            int4 a, b, c, d;
            if (!MCU_stoi4x4(ep . getsvalue(), a, b, c, d))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
            if (!ctxt . HasError())
            {
                integer_t t_value[4];
                t_value[0] = a;
                t_value[1] = b;
                t_value[2] = c;
                t_value[3] = d;
                ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> setter)(ctxt, mark, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeArray:
        {
            MCAutoArrayRef t_value;
            if (!ep . copyasarrayref(&t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NOTANARRAY);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCArrayRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeEnum:
        {
            MCExecEnumTypeInfo *t_enum_info;
            t_enum_info = (MCExecEnumTypeInfo *)prop -> type_info;
            
            bool t_found;
            t_found = false;
            intenum_t t_value;
            for(uindex_t i = 0; i < t_enum_info -> count; i++)
                if (!t_enum_info -> elements[i] . read_only &&
                    MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
                {
                    t_found = true;
                    t_value = t_enum_info -> elements[i] . value;
                }
            
            if (!t_found)
                ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, int))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeOptionalEnum:
        {
            MCExecEnumTypeInfo *t_enum_info;
            t_enum_info = (MCExecEnumTypeInfo *)prop -> type_info;
            
            intenum_t t_value;
            intenum_t* t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                bool t_found;
                t_found = false;
                for(uindex_t i = 0; i < t_enum_info -> count; i++)
                    if (!t_enum_info -> elements[i] . read_only &&
                        MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
                    {
                        t_found = true;
                        t_value = t_enum_info -> elements[i] . value;
                    }
				
                if (!t_found)
                    ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, int*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeSet:
        {
            MCExecSetTypeInfo *t_seprop;
            t_seprop = (MCExecSetTypeInfo *)(prop -> type_info);
            
            intset_t t_value = 0;
            char **t_elements;
            uindex_t t_element_count;
            MCCStringSplit(ep . getcstring(), ',', t_elements, t_element_count);
            
            for (uindex_t i = 0; i < t_element_count; i++)
            {
                for (uindex_t j = 0; j < t_seprop -> count; j++)
                {
                    if (MCU_strcasecmp(t_elements[i], t_seprop -> elements[j] . tag) == 0)
                    {
                        t_value |= 1 << t_seprop -> elements[j] . bit;
                        break;
                    }
                }
            }
            
            MCCStringArrayFree(t_elements, t_element_count);
            ((void(*)(MCExecContext&, void *, unsigned int))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeCustom:
        {
            MCExecCustomTypeInfo *t_custom_info;
            t_custom_info = (MCExecCustomTypeInfo *)(prop -> type_info);
            
            MCAssert(t_custom_info -> size <= 64);
            
            MCAutoStringRef t_input_value;
            /* UNCHECKED */ ep . copyasstringref(&t_input_value);
            
            char t_value[64];
            ((MCExecCustomTypeParseProc)t_custom_info -> parse)(ctxt, *t_input_value, t_value);
            if (!ctxt . HasError())
            {
                ((void(*)(MCExecContext&, void *, void *))prop -> setter)(ctxt, mark, t_value);
                ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeOptionalInt16:
        {
            integer_t t_value;
            integer_t *t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                if (!ep . copyasint(t_value) ||
                    t_value < -32768 || t_value > 32767)
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedUInt16:
        case kMCPropertyTypeOptionalUInt16:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                if (!ep . copyasuint(t_value) ||
                    t_value < 0 || t_value > 65535)
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalUInt32:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                if (!ep . copyasuint(t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalString:
        {
            MCAutoStringRef t_value;
            if (!ep . isempty())
            {
                if (!ep . copyasstringref(&t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAS);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeOptionalPoint:
        {
            MCPoint t_value;
            MCPoint *t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                if (!ep . copyaslegacypoint(t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalRectangle:
        {
            MCRectangle t_value;
            MCRectangle *t_value_ptr;
            if (ep . isempty())
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                if (!ep . copyaslegacyrectangle(t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeLinesOfString:
        {
            MCAutoStringRef t_input;
            MCStringRef *t_value;
            uindex_t t_count;
            
            if (!ep . copyasstringref(&t_input) || !MCPropertyParseStringList(*t_input, '\n', t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
			{
                ((void(*)(MCExecContext&, void *, uindex_t, MCStringRef*))prop -> setter)(ctxt, mark, t_count, t_value);
				for(uindex_t i = 0; i < t_count; i++)
					MCValueRelease(t_value[i]);
				MCMemoryDeleteArray(t_value);
			}
        }
            break;
            
        case kMCPropertyTypeLinesOfUInt:
        case kMCPropertyTypeItemsOfUInt:
        {
            MCAutoStringRef t_input;
            uinteger_t* t_value;
            uindex_t t_count;
            
            char_t t_delimiter;
            t_delimiter = prop -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
            
            if (!ep . copyasstringref(&t_input) || !MCPropertyParseUIntList(*t_input, t_delimiter, t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, uinteger_t*))prop -> setter)(ctxt, mark, t_count, t_value);
			
			MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeLinesOfPoint:
        {
            MCAutoStringRef t_input;
            MCPoint *t_value;
            uindex_t t_count;
            
            if (!ep . copyasstringref(&t_input) || !MCPropertyParsePointList(*t_input, '\n', t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, MCPoint*))prop -> setter)(ctxt, mark, t_count, t_value);
			
			MCMemoryDeleteArray(t_value);
        }
            break;
            
        default:
            ctxt . Unimplemented();
            break;
    }
}

void MCExecResolveCharsOfField(MCField *p_field, uint32_t p_part, int32_t& x_start, int32_t& x_finish, uint32_t p_start, uint32_t p_count)
{
    p_field -> resolvechars(p_part, x_start, x_finish, p_start, p_count);
}
