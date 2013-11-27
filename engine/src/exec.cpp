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
#include "variable.h"

#include "osspec.h"

#include "debug.h"
#include "param.h"

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

bool MCExecContext::ConvertToNumberOrArray(MCExecValue& x_value)
{
    switch(x_value . type)
    {
    case kMCExecValueTypeNone:
        return false;

    case kMCExecValueTypeValueRef:
    case kMCExecValueTypeBooleanRef:
    case kMCExecValueTypeStringRef:
    case kMCExecValueTypeNameRef:
    case kMCExecValueTypeDataRef:
    case kMCExecValueTypeNumberRef:
    {
        double t_real;
        if (!ConvertToReal(x_value . valueref_value, t_real))
            return false;

        MCValueRelease(x_value . valueref_value);
        MCExecValueTraits<double>::set(x_value, t_real);
        return true;
    }

    case kMCExecValueTypeUInt:
        MCExecValueTraits<double>::set(x_value, (double)x_value . uint_value);
        return true;

    case kMCExecValueTypeInt:
        MCExecValueTraits<double>::set(x_value, (double)x_value . int_value);
        return true;

    case kMCExecValueTypeFloat:
        MCExecValueTraits<double>::set(x_value, (double)x_value . float_value);
        return true;

    case kMCExecValueTypeArrayRef:
    case kMCExecValueTypeDouble:
        return true;

    case kMCExecValueTypeBool:
    case kMCExecValueTypeChar:
    case kMCExecValueTypePoint:
    case kMCExecValueTypeColor:
    case kMCExecValueTypeRectangle:
	default:
        return false;
    }
}

bool MCExecContext::ConvertToData(MCValueRef p_value, MCDataRef& r_data)
{
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string))
        return false;
    
	return MCDataCreateWithBytes((const byte_t *)MCStringGetNativeCharPtr(*t_string), MCStringGetLength(*t_string), r_data);
}

bool MCExecContext::ConvertToName(MCValueRef p_value, MCNameRef& r_name)
{
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string))
        return false;
    
    return MCNameCreate(*t_string, r_name);
}

bool MCExecContext::ConvertToChar(MCValueRef p_value, char_t& r_char)
{
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string) || MCStringGetLength(*t_string) > 1)
        return false;
    
    r_char = MCStringGetNativeCharAtIndex(*t_string, 0);
    return true;
}

bool MCExecContext::ConvertToLegacyColor(MCValueRef p_value, MCColor& r_color)
{
    MCAutoStringRef t_string;
	return ConvertToString(p_value, &t_string) && MCscreen -> parsecolor(*t_string, r_color);
}

bool MCExecContext::ConvertToBool(MCValueRef p_value, bool& r_bool)
{
    bool t = m_ep . convertvaluereftobool(p_value, r_bool);
    return t;
}

bool MCExecContext::ConvertToLegacyPoint(MCValueRef p_value, MCPoint& r_point)
{
    MCAutoStringRef t_string;
	return ConvertToString(p_value, &t_string) && MCU_stoi2x2(MCStringGetOldString(*t_string), r_point . x, r_point . y);
}

bool MCExecContext::ConvertToLegacyRectangle(MCValueRef p_value, MCRectangle& r_rect)
{
    MCAutoStringRef t_string;
	int16_t t_left, t_top, t_right, t_bottom;
	if (ConvertToString(p_value, &t_string) &&
		MCU_stoi2x4(MCStringGetOldString(*t_string), t_left, t_top, t_right, t_bottom))
	{
		r_rect . x = t_left;
		r_rect . y = t_top;
		r_rect . width = t_right - t_left;
		r_rect . height = t_bottom - t_top;
    
		return true;
	}
    
	return false;
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

bool MCExecContext::TryToEvaluateExpression(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef& r_result)
{
    MCAssert(p_expr != nil);
	
    bool t_success;
    t_success = false;
    
    do
    {
        p_expr -> eval_valueref(*this, r_result);
        if (!HasError())
            t_success = true;
        else
            MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
	while (!t_success && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors);
        
	if (t_success)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToEvaluateParameter(MCParameter *p_param, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef& r_result)
{
    MCAssert(p_param != nil);
	
    bool t_success;
    t_success = false;
    
    do
    {
        if (p_param -> eval(*this, r_result))
            t_success = true;
        else
            MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
	while (!t_success && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors);
    
	if (t_success)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToEvaluateExpressionAsNonStrictBool(MCExpression * p_expr, uint2 line, uint2 pos, Exec_errors p_error, bool& r_value)
{
    MCAssert(p_expr != nil);
    MCExecValue t_value;
    
    bool t_success;
    t_success = false;
    
    do
    {
        if (EvalExprAsNonStrictBool(p_expr, p_error, r_value))
            t_success = true;
        else
            MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
	while (!t_success && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors);
    
    if (t_success)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToSetVariable(MCVarref *p_var, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef p_value)
{
    bool t_success;
    t_success = false;
    
    do
    {
        p_var -> set(*this, p_value);
        if (!HasError())
            t_success = true;
        else
            MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
	while (!t_success && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors);
    
	if (t_success)
		return true;
	
	LegacyThrow(p_error);
	return false;
}
//////////

bool MCExecContext::EvalExprAsStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_stringref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsStringRef(MCExpression *p_expr, MCStringRef p_default, Exec_errors p_error, MCStringRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}

    return EvalExprAsStringRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsNullableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef &r_value)
{
    if (p_expr == nil)
    {
        r_value = nil;
        return true;
    }

    return EvalExprAsStringRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsBooleanRef(MCExpression *p_expr, Exec_errors p_error, MCBooleanRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_booleanref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsBooleanRef(MCExpression *p_expr, MCBooleanRef p_default, Exec_errors p_error, MCBooleanRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}
	
	return EvalExprAsBooleanRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_dataref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsDataRef(MCExpression *p_expr, MCDataRef p_default, Exec_errors p_error, MCDataRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}

    return EvalExprAsDataRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsNullableDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef &r_value)
{
    if (p_expr == nil)
    {
        r_value = nil;
        return true;
    }

    return EvalExprAsDataRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_nameref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);

    return false;
}

bool MCExecContext::EvalOptionalExprAsNullableNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef &r_value)
{
    if (p_expr == nil)
    {
        r_value = nil;
        return true;
    }

    return EvalExprAsNameRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsNameRef(MCExpression *p_expr, MCNameRef p_default, Exec_errors p_error, MCNameRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}
	
	return EvalExprAsNameRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_arrayref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsArrayRef(MCExpression *p_expr, MCArrayRef p_default, Exec_errors p_error, MCArrayRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}

    return EvalExprAsArrayRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsNullableArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef &r_value)
{
    if (p_expr == nil)
    {
        r_value = nil;
        return true;
    }

    return EvalExprAsArrayRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsNumberRef(MCExpression *p_expr, Exec_errors p_error, MCNumberRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_numberref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsNumberRef(MCExpression *p_expr, MCNumberRef p_default, Exec_errors p_error, MCNumberRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}
	
	return EvalExprAsNumberRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsValueRef(MCExpression *p_expr, Exec_errors p_error, MCValueRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_valueref(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsValueRef(MCExpression *p_expr, MCValueRef p_default, Exec_errors p_error, MCValueRef& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCValueRetain(p_default);
		return true;
	}
	
	return EvalExprAsValueRef(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsUInt(MCExpression *p_expr, Exec_errors p_error, uinteger_t& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_uint(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsUInt(MCExpression *p_expr, uinteger_t p_default, Exec_errors p_error, uinteger_t& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
	}
	
	return EvalExprAsUInt(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsInt(MCExpression *p_expr, Exec_errors p_error, integer_t& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_int(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsInt(MCExpression *p_expr, integer_t p_default, Exec_errors p_error, integer_t& r_value)
{
	if (p_expr == nil)
	{
        r_value = p_default;
		return true;
	}
	
	return EvalExprAsInt(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsBool(MCExpression *p_expr, Exec_errors p_error, bool& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_bool(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalExprAsNonStrictBool(MCExpression *p_expr, Exec_errors p_error, bool& r_value)
{
    MCAssert(p_expr != nil);
	
	MCAutoStringRef t_value;
    p_expr -> eval_stringref(*this, &t_value);

    if (!HasError())
	{
		r_value = MCStringIsEqualTo(*t_value, kMCTrueString, kMCStringOptionCompareCaseless);
		return true;
	}
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsBool(MCExpression *p_expr, bool p_default, Exec_errors p_error, bool& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
	}
	
	return EvalExprAsBool(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsDouble(MCExpression *p_expr, Exec_errors p_error, double& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_double(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsDouble(MCExpression *p_expr, double p_default, Exec_errors p_error, double& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
	}
	
	return EvalExprAsDouble(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsChar(MCExpression *p_expr, Exec_errors p_error, char_t& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_char(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsChar(MCExpression *p_expr, char_t p_default, Exec_errors p_error, char_t& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
	}
	
	return EvalExprAsChar(p_expr, p_error, r_value);
}

bool MCExecContext::EvalExprAsPoint(MCExpression *p_expr, Exec_errors p_error, MCPoint& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_point(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalOptionalExprAsPoint(MCExpression *p_expr, MCPoint* p_default, Exec_errors p_error, MCPoint*& r_value)
{
	if (p_expr == nil)
	{
        r_value = p_default;
		return true;
	}

    // Makes sure the return parameter isn't a nil pointer
    MCAssert(r_value != nil);

    return EvalExprAsPoint(p_expr, p_error, *r_value);
}

bool MCExecContext::EvalExprAsColor(MCExpression *p_expr, Exec_errors p_error, MCColor& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_color(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}


bool MCExecContext::EvalOptionalExprAsColor(MCExpression *p_expr, MCColor *p_default, Exec_errors p_error, MCColor *&r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
    }

    // Makes sure the return parameter isn't a nil pointer
    MCAssert(r_value != nil);

    return EvalExprAsColor(p_expr, p_error, *r_value);
}

bool MCExecContext::EvalExprAsRectangle(MCExpression *p_expr, Exec_errors p_error, MCRectangle& r_value)
{
	MCAssert(p_expr != nil);

    p_expr -> eval_rectangle(*this, r_value);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
    return false;
}

bool MCExecContext::EvalOptionalExprAsRectangle(MCExpression *p_expr, MCRectangle* p_default, Exec_errors p_error, MCRectangle*& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
    }

    // Makes sure the return parameter isn't a nil pointer
    MCAssert(r_value != nil);

    return EvalExprAsRectangle(p_expr, p_error, *r_value);
}

bool MCExecContext::EvalExprAsMutableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_mutable_string)
{
    MCAutoStringRef t_string;
    if (!EvalExprAsStringRef(p_expr, p_error, &t_string))
        return false;
    
    return MCStringMutableCopy(*t_string, r_mutable_string);
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

void MCExecContext::UserThrow(MCStringRef p_error)
{
	MCeerror -> copystringref(p_error, True);
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
        r_list = nil;
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
        r_list = nil;
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
        r_list = nil;
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

void MCExecFetchProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCExecValue& r_value)
{
    switch(prop -> type)
    {
        case kMCPropertyTypeAny:
        {
            ((void(*)(MCExecContext&, void *, MCValueRef&))prop -> getter)(ctxt, mark, r_value . valueref_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeValueRef;
            }
        }
            break;
            
        case kMCPropertyTypeBool:
        {
            ((void(*)(MCExecContext&, void *, bool&))prop -> getter)(ctxt, mark, r_value . bool_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeBool;
            }
        }
            break;
            
        case kMCPropertyTypeInt16:
        case kMCPropertyTypeInt32:
        {
            integer_t t_value;
            ((void(*)(MCExecContext&, void *, integer_t&))prop -> getter)(ctxt, mark, r_value . int_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeInt;
            }
        }
            break;
            
        case kMCPropertyTypeUInt8:
        case kMCPropertyTypeUInt16:
        case kMCPropertyTypeUInt32:
        {
            uinteger_t t_value;
            ((void(*)(MCExecContext&, void *, uinteger_t&))prop -> getter)(ctxt, mark, r_value . uint_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeUInt;
            }
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            double t_value;
            ((void(*)(MCExecContext&, void *, double&))prop -> getter)(ctxt, mark, r_value . double_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeDouble;
            }
        }
            break;
            
        case kMCPropertyTypeChar:
        {
            char_t t_value;
            ((void(*)(MCExecContext&, void *, char_t&))prop -> getter)(ctxt, mark, r_value . char_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeChar;
            }
        }
            break;
            
        case kMCPropertyTypeString:
        {
            ((void(*)(MCExecContext&, void *, MCStringRef&))prop -> getter)(ctxt, mark, r_value . stringref_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeStringRef;
            }
        }
            break;
            
        case kMCPropertyTypeBinaryString:
        {
            ((void(*)(MCExecContext&, void *, MCDataRef&))prop -> getter)(ctxt, mark, r_value . dataref_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeDataRef;
            }
        }
            break;
            
        case kMCPropertyTypeName:
        {
            MCNewAutoNameRef t_value;
            ((void(*)(MCExecContext&, void *, MCNameRef&))prop->getter)(ctxt, mark, r_value . nameref_value);
            if (!ctxt.HasError())
            {
                r_value . type = kMCExecValueTypeNameRef;
            }
        }
            break;
            
        case kMCPropertyTypeColor:
        {
            MCColor t_value;
            ((void(*)(MCExecContext&, void *, MCColor&))prop -> getter)(ctxt, mark, r_value . color_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeColor;
            }
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            MCRectangle t_value;
            ((void(*)(MCExecContext&, void *, MCRectangle&))prop -> getter)(ctxt, mark, r_value . rectangle_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeRectangle;
            }
        }
            break;
            
        case kMCPropertyTypePoint:
        {
            MCPoint t_value;
            ((void(*)(MCExecContext&, void *, MCPoint&))prop -> getter)(ctxt, mark, r_value . point_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypePoint;
            }
        }
            break;
            
        case kMCPropertyTypeInt16X2:
        {
            integer_t t_value[2];
            ((void(*)(MCExecContext&, void *, integer_t[2]))prop -> getter)(ctxt, mark, t_value);
            
            MCAutoStringRef t_string;
            if (!ctxt . HasError())
            {
                MCStringFormat(&t_string, "%d,%d", t_value[0], t_value[1]);
                r_value . stringref_value = MCValueRetain(*t_string);
                r_value . type = kMCExecValueTypeStringRef;
            }
        }
            break;
            
        case kMCPropertyTypeInt16X4:
        {
            integer_t t_value[4];
            ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> getter)(ctxt, mark, t_value);
            
            MCAutoStringRef t_string;
            if (!ctxt . HasError())
            {
                MCStringFormat(&t_string, "%d,%d,%d,%d", t_value[0], t_value[1], t_value[2], t_value[3]);
                r_value . stringref_value = MCValueRetain(*t_string);
                r_value . type = kMCExecValueTypeStringRef;
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
                {
                    r_value . arrayref_value = MCValueRetain(*t_value);
                    r_value . type = kMCExecValueTypeArrayRef;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeEnum:
        {
            int t_value;
            ((void(*)(MCExecContext&, void *, int&))prop -> getter)(ctxt, mark, t_value);
            if (!ctxt . HasError())
            {
                MCExecEnumTypeInfo *t_enum_info;
                t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                MCExecFormatEnum(ctxt, t_enum_info, t_value, r_value);
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
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else
                {
                    MCExecEnumTypeInfo *t_enum_info;
                    t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                    MCExecFormatEnum(ctxt, t_enum_info, t_value, r_value);
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
                MCExecFormatSet(ctxt, t_seprop, t_value, r_value);
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
                ((MCExecCustomTypeFormatProc)t_custom_info -> format)(ctxt, t_value, r_value . stringref_value);
                ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
                if (!ctxt . HasError())
                {
                    r_value . type = kMCExecValueTypeStringRef;
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
                if (t_value_ptr != nil)
                {
                    r_value . int_value = t_value;
                    r_value . type = kMCExecValueTypeInt;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeOptionalUInt8:           
        case kMCPropertyTypeOptionalUInt16:
        case kMCPropertyTypeOptionalUInt32:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, uinteger_t*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr != nil)
                {
                    r_value . uint_value = t_value;
                    r_value . type = kMCExecValueTypeUInt;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeOptionalString:
        {
            MCAutoStringRef t_value;
            ((void(*)(MCExecContext&, void *, MCStringRef&))prop -> getter)(ctxt, mark, &t_value);
            if (!ctxt . HasError())
            {
                if (*t_value != nil)
                    r_value . stringref_value = MCValueRetain(*t_value);
                else
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                
                r_value . type = kMCExecValueTypeStringRef;
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
                if (t_value_ptr != nil)
                {
                    r_value . point_value = t_value;
                    r_value . type = kMCExecValueTypePoint;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
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
                if (t_value_ptr != nil)
                {
                    r_value . rectangle_value = t_value;
                    r_value . type = kMCExecValueTypeRectangle;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeOptionalColor:
        {
            MCColor t_value;
            MCColor *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, MCColor*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr != nil)
                {
                    r_value . color_value = t_value;
                    r_value . type = kMCExecValueTypeColor;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
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
                if (MCPropertyFormatStringList(t_value, t_count, '\n', r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
                for (int i = 0; i < t_count; ++i)
                    MCValueRelease(t_value[i]);
            }
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
                char_t t_delimiter;
                t_delimiter = prop -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
                if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeLinesOfPoint:
        {
            MCPoint* t_value;
            uindex_t t_count;
            ((void(*)(MCExecContext&, void *, uindex_t&, MCPoint*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                if (MCPropertyFormatPointList(t_value, t_count, '\n', r_value . stringref_value))
                {
                      r_value . type = kMCExecValueTypeStringRef;
                }
            }
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
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else
                {
                    r_value . bool_value = t_value;
                    r_value . type = kMCExecValueTypeBool;
                }
            }
        }
            break;
            
        case kMCPropertyTypeMixedUInt8:            
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
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else
                {
                    r_value . uint_value = t_value;
                    r_value . type = kMCExecValueTypeUInt;
                }
            }
        }
            break;
        
        case kMCPropertyTypeMixedOptionalBool:
        {
            bool t_mixed;
            bool t_value;
            bool *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, bool*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else if (*t_value_ptr != nil)
                {
                    r_value . bool_value = t_value;
                    r_value . type = kMCExecValueTypeBool;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeMixedOptionalInt16:
        case kMCPropertyTypeMixedOptionalInt32:
        {
            bool t_mixed;
            integer_t t_value;
            integer_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, integer_t*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else if (*t_value_ptr != nil)
                {
                    r_value . int_value = t_value;
                    r_value . type = kMCExecValueTypeInt;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeMixedOptionalUInt8:
        case kMCPropertyTypeMixedOptionalUInt16:
        case kMCPropertyTypeMixedOptionalUInt32:
        {
            bool t_mixed;
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, uinteger_t*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else if (*t_value_ptr != nil)
                {
                    r_value . uint_value = t_value;
                    r_value . type = kMCExecValueTypeUInt;
                }
                else
                {
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                    r_value . type = kMCExecValueTypeStringRef;
                }
            }
        }
            break;
            
        case kMCPropertyTypeMixedOptionalString:
        {
            MCAutoStringRef t_value;
            bool t_mixed;
            ((void(*)(MCExecContext&, void *, bool&, MCStringRef&))prop -> getter)(ctxt, mark, t_mixed, &t_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeStringRef;
                if (t_mixed)
                    r_value . stringref_value = MCSTR(MCmixedstring);
                else if (*t_value != nil)
                    r_value . stringref_value = MCValueRetain(*t_value);
                else
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
            }
            
        }
            break;
            
        case kMCPropertyTypeMixedCustom:
        {
            MCExecCustomTypeInfo *t_custom_info;
            t_custom_info = (MCExecCustomTypeInfo *)(prop -> type_info);
            
            MCAssert(t_custom_info -> size <= 64);
            
            char t_value[64];
            bool t_mixed;
            ((void(*)(MCExecContext&, void*, bool&, void*))prop -> getter)(ctxt, mark, t_mixed, t_value);
            
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . type = kMCExecValueTypeStringRef;
                    r_value . stringref_value = MCSTR(MCmixedstring);
                }
                else
                {
                    MCAutoStringRef t_value_ref;
                    ((MCExecCustomTypeFormatProc)t_custom_info -> format)(ctxt, t_value, &t_value_ref);
                    ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
                    if (!ctxt . HasError())
                    {
                        r_value . stringref_value = MCValueRetain(*t_value_ref);
                        r_value . type = kMCExecValueTypeStringRef;
                    }
                }
            }
        }
            break;
            
        case kMCPropertyTypeMixedEnum:
        {
            int t_value;
            bool t_mixed;
            ((void(*)(MCExecContext&, void *, bool&, int&))prop -> getter)(ctxt, mark, t_mixed, t_value);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . type = kMCExecValueTypeStringRef;
                    r_value . stringref_value = MCSTR(MCmixedstring);
                }
                else
                {
                    MCExecEnumTypeInfo *t_enum_info;
                    t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                    MCExecFormatEnum(ctxt, t_enum_info, t_value, r_value);
                }
            }
        }
            
        case kMCPropertyTypeMixedLinesOfUInt:
        case kMCPropertyTypeMixedItemsOfUInt:
        {
            bool t_mixed;
            uinteger_t* t_value;
            uindex_t t_count;
            ((void(*)(MCExecContext&, void *, bool&, uindex_t&, uinteger_t*&))prop -> getter)(ctxt, mark, t_mixed, t_count, t_value);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . type = kMCExecValueTypeStringRef;
                    r_value . stringref_value = MCSTR(MCmixedstring);
                }
                else
                {
                    char_t t_delimiter;
                    t_delimiter = prop -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
                    if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, r_value . stringref_value))
                    {
                        r_value . type = kMCExecValueTypeStringRef;
                    }
                }
                MCMemoryDeleteArray(t_value);
            }
        }
            break;    
        
        case kMCPropertyTypeRecord:
        {
            ((void(*)(MCExecContext&, void *, MCExecValue&))prop -> getter)(ctxt, mark, r_value);
        }
            break;
            
        default:
            ctxt . Unimplemented();
            break;
    }
}

void MCExecStoreProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCExecValue p_value)
{
    switch(prop -> type)
    {
        case kMCPropertyTypeAny:
        {
            MCAutoValueRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeValueRef, &(&t_value));
            ((void(*)(MCExecContext&, void *, MCValueRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeMixedBool:
        case kMCPropertyTypeBool:
        {
            bool t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeBool, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, bool))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeMixedInt16:
        case kMCPropertyTypeInt16:
        {
            integer_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeInt, &t_value);
            if (t_value < -32768 || t_value > 32767)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt32:
        {
            integer_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeInt, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
         
        case kMCPropertyTypeMixedUInt16:
        case kMCPropertyTypeUInt16:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeUInt, &t_value);
            if (t_value < 0 || t_value > 65535)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeUInt32:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeUInt, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            double t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeDouble, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, double))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeChar:
        {
            char_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeChar, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, char_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeString:
        {
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_value));
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeBinaryString:
        {
            MCAutoDataRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeDataRef, &(&t_value));
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCDataRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeName:
        {
            MCNewAutoNameRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeNameRef, &(&t_value));
            if (!ctxt.HasError())
                ((void(*)(MCExecContext&, void *, MCNameRef))prop->setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeColor:
        {
            MCColor t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeColor, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCColor))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            MCRectangle t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeRectangle, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypePoint:
        {
            MCPoint t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypePoint, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt16X2:
        {
            int2 a, b;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi2x2(*t_value, a, b))
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
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi2x4(*t_value, a, b, c, d))
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
            
        case kMCPropertyTypeInt32X4:
        {
            int4 a, b, c, d;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi4x4(*t_value, a, b, c, d))
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
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeArrayRef, &(&t_value));
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCArrayRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeMixedEnum:
        case kMCPropertyTypeEnum:
        {
            MCExecEnumTypeInfo *t_enum_info;
            t_enum_info = (MCExecEnumTypeInfo *)prop -> type_info;
            
            intenum_t t_value;
            MCExecParseEnum(ctxt, t_enum_info, p_value, t_value);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, int))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalEnum:
        case kMCPropertyTypeOptionalEnum:
        {
            MCExecEnumTypeInfo *t_enum_info;
            t_enum_info = (MCExecEnumTypeInfo *)prop -> type_info;
            
            MCAutoStringRef t_string;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_string));
            
            intenum_t t_value;
            intenum_t* t_value_ptr;
            if (MCStringIsEmpty(*t_string))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecParseEnum(ctxt, t_enum_info, p_value, t_value);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, int*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeSet:
        {
            MCExecSetTypeInfo *t_seprop;
            t_seprop = (MCExecSetTypeInfo *)(prop -> type_info);
            intset_t t_value;
            MCExecParseSet(ctxt, t_seprop, p_value, t_value);
            ((void(*)(MCExecContext&, void *, unsigned int))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeMixedCustom:
        case kMCPropertyTypeCustom:
        {
            MCExecCustomTypeInfo *t_custom_info;
            t_custom_info = (MCExecCustomTypeInfo *)(prop -> type_info);
            
            MCAssert(t_custom_info -> size <= 64);
            
            MCAutoStringRef t_input_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_input_value));
            
            char t_value[64];
            ((MCExecCustomTypeParseProc)t_custom_info -> parse)(ctxt, *t_input_value, t_value);
            if (!ctxt . HasError())
            {
                ((void(*)(MCExecContext&, void *, void *))prop -> setter)(ctxt, mark, t_value);
                ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeMixedOptionalInt16:
        case kMCPropertyTypeOptionalInt16:
        {
            integer_t t_value;
            integer_t *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeInt, &t_value);
                if (t_value < -32768 || t_value > 32767)
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalUInt16:
        case kMCPropertyTypeOptionalUInt16:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeUInt, &t_value);
                if (t_value < 0 || t_value > 65535)
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalUInt32:
        case kMCPropertyTypeOptionalUInt32:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeUInt, &t_value);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalString:
        case kMCPropertyTypeOptionalString:
        {
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_value));
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeOptionalPoint:
        {
            MCPoint t_value;
            MCPoint *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypePoint, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalRectangle:
        {
            MCRectangle t_value;
            MCRectangle *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeRectangle, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalColor:
        {
            MCColor t_value;
            MCColor *t_value_ptr;
            if (p_value . type == kMCExecValueTypeStringRef && MCStringIsEmpty(p_value . stringref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeColor, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCColor*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeLinesOfString:
        {
            MCAutoStringRef t_input;
            MCStringRef *t_value;
            uindex_t t_count;
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_input));
            
            if (!MCPropertyParseStringList(*t_input, '\n', t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, MCStringRef*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            for(uindex_t i = 0; i < t_count; i++)
                MCValueRelease(t_value[i]);
            MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeMixedItemsOfUInt:            
        case kMCPropertyTypeLinesOfUInt:
        case kMCPropertyTypeItemsOfUInt:
        {
            MCAutoStringRef t_input;
            uinteger_t* t_value;
            uindex_t t_count;
            
            char_t t_delimiter;
            t_delimiter = prop -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_input));
            if (!MCPropertyParseUIntList(*t_input, t_delimiter, t_count, t_value))
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
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_input));
            if (!MCPropertyParsePointList(*t_input, '\n', t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, MCPoint*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeRecord:
        {
            ((void(*)(MCExecContext&, void *, MCExecValue))prop -> setter)(ctxt, mark, p_value);
        }
            break;
            
        default:
            ctxt . Unimplemented();
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCExecTypeConvertToValueRefAndReleaseAlways(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, MCValueRef& r_value)
{
	switch(p_from_type)
	{
		case kMCExecValueTypeValueRef:
        case kMCExecValueTypeArrayRef:
        case kMCExecValueTypeDataRef:
        case kMCExecValueTypeStringRef:
        case kMCExecValueTypeNameRef:
        case kMCExecValueTypeBooleanRef:
        case kMCExecValueTypeNumberRef:
			r_value = *(MCValueRef *)p_from_value;
			break;
			
        case kMCExecValueTypeUInt:
			if (!MCNumberCreateWithUnsignedInteger(*(uinteger_t *)p_from_value, (MCNumberRef&)r_value))
				ctxt . Throw();
			break;
			
		case kMCExecValueTypeInt:
			if (!MCNumberCreateWithInteger(*(integer_t *)p_from_value, (MCNumberRef&)r_value))
					ctxt . Throw();
			break;
            
        case kMCExecValueTypeDouble:
			if (!MCNumberCreateWithReal(*(double *)p_from_value, (MCNumberRef&)r_value))
				ctxt . Throw();
			break;
            
        case kMCExecValueTypeColor:
            if(!MCStringFormat((MCStringRef&)r_value, "%u,%u,%u", (((MCColor *)p_from_value) -> red >> 8) & 0xff, (((MCColor *)p_from_value) -> green >> 8) & 0xff, (((MCColor *)p_from_value) -> blue >> 8) & 0xff))
                ctxt . Throw();
			break;
			
        case kMCExecValueTypePoint:
            if(!MCStringFormat((MCStringRef&)r_value, "%d,%d", ((MCPoint *)p_from_value) -> x, ((MCPoint *)p_from_value) -> y))
                ctxt . Throw();
            break;
            
        case kMCExecValueTypeRectangle:
            if(!MCStringFormat((MCStringRef&)r_value, "%d,%d,%d,%d", ((MCRectangle*)p_from_value) -> x, ((MCRectangle *)p_from_value) -> y, ((MCRectangle*)p_from_value) -> x + ((MCRectangle*)p_from_value) -> width, ((MCRectangle *)p_from_value) -> y + ((MCRectangle *)p_from_value) -> height))
                ctxt . Throw();
            break;
        
        case kMCExecValueTypeBool:
            r_value = MCValueRetain(*(bool *)p_from_value ? kMCTrue : kMCFalse);
            break;
            
        case kMCExecValueTypeChar:
            if (!MCStringCreateWithNativeChars((const char_t *)p_from_value, 1, (MCStringRef&)r_value))
                ctxt . Throw();
            break;

		default:
			ctxt . Unimplemented();
			break;
	}
}

void MCExecTypeConvertFromValueRefAndReleaseAlways(MCExecContext& ctxt, MCValueRef p_from_value, MCExecValueType p_to_type, void *p_to_value)
{
    bool t_success = true;

	switch(p_to_type)
	{
		case kMCExecValueTypeValueRef:
			*(MCValueRef *)p_to_value = p_from_value;
			return;
		case kMCExecValueTypeArrayRef:
            t_success = ctxt . ConvertToArray(p_from_value, *(MCArrayRef *)p_to_value);
			break;
		case kMCExecValueTypeDataRef:
            t_success = ctxt . ConvertToData(p_from_value, *(MCDataRef *)p_to_value);
			break;
		case kMCExecValueTypeStringRef:
            t_success = ctxt . ConvertToString(p_from_value, *(MCStringRef *)p_to_value);
			break;
		case kMCExecValueTypeNameRef:
            t_success = ctxt . ConvertToName(p_from_value, *(MCNameRef *)p_to_value);
			break;
		case kMCExecValueTypeNumberRef:
            t_success = ctxt . ConvertToNumber(p_from_value, *(MCNumberRef *)p_to_value);
			break;
		case kMCExecValueTypeBooleanRef:
            t_success = ctxt . ConvertToBoolean(p_from_value, *(MCBooleanRef *)p_to_value);
			break;
		case kMCExecValueTypeUInt:
            t_success = ctxt . ConvertToUnsignedInteger(p_from_value, *(uinteger_t *)p_to_value);
			break;
		case kMCExecValueTypeInt:
            t_success = ctxt . ConvertToInteger(p_from_value, *(integer_t *)p_to_value);
			break;
		case kMCExecValueTypeBool:
            t_success = ctxt . ConvertToBool(p_from_value, *(bool *)p_to_value);
			break;
		case kMCExecValueTypeDouble:
            t_success = ctxt . ConvertToReal(p_from_value, *(double *)p_to_value);
			break;
		case kMCExecValueTypePoint:
            t_success = ctxt . ConvertToLegacyPoint(p_from_value, *(MCPoint *)p_to_value);
			break;
		case kMCExecValueTypeRectangle:
            t_success = ctxt . ConvertToLegacyRectangle(p_from_value, *(MCRectangle *)p_to_value);
			break;
		case kMCExecValueTypeChar:
            t_success = ctxt . ConvertToChar(p_from_value, *(char_t *)p_to_value);
			break;
        case kMCExecValueTypeColor:
            t_success = ctxt . ConvertToLegacyColor(p_from_value, *(MCColor *)p_to_value);
			break;
		default:
			ctxt . Unimplemented();
			break;
	}
	
	MCValueRelease(p_from_value);
    if (!t_success)
        ctxt . Throw();
}

void MCExecTypeConvertAndReleaseAlways(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, MCExecValueType p_to_type, void *p_to_value)
{
	MCValueRef t_pivot;
	MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, p_from_type, p_from_value, t_pivot);
	if (ctxt . HasError())
		return;
	MCExecTypeConvertFromValueRefAndReleaseAlways(ctxt, t_pivot, p_to_type, p_to_value);
}

void MCExecTypeRelease(MCExecValue &self)
{
    switch (self . type)
    {
    case kMCExecValueTypeValueRef:
    case kMCExecValueTypeBooleanRef:
    case kMCExecValueTypeStringRef:
    case kMCExecValueTypeNameRef:
    case kMCExecValueTypeDataRef:
    case kMCExecValueTypeArrayRef:
    case kMCExecValueTypeNumberRef:
        MCValueRelease(self . valueref_value);
        break;
    case kMCExecValueTypeUInt:
    case kMCExecValueTypeInt:
    case kMCExecValueTypeBool:
    case kMCExecValueTypeDouble:
    case kMCExecValueTypeFloat:
    case kMCExecValueTypeChar:
    case kMCExecValueTypePoint:
    case kMCExecValueTypeColor:
    case kMCExecValueTypeRectangle:
    case kMCExecValueTypeEnum:
    case kMCExecValueTypeSet:
    case kMCExecValueTypeNone:
        break;
    }
}

void MCExecResolveCharsOfField(MCField *p_field, uint32_t p_part, int32_t& x_start, int32_t& x_finish, uint32_t p_start, uint32_t p_count)
{
    findex_t t_start = x_start;
    findex_t t_finish = x_finish;
    p_field -> resolvechars(p_part, t_start, t_finish, p_start, p_count);
    x_start = t_start;
    x_finish = t_finish;
}

void MCExecParseSet(MCExecContext& ctxt, MCExecSetTypeInfo *p_info, MCExecValue p_value, intset_t& r_value)
{
    MCAutoStringRef t_string;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_string));
    
    intset_t t_value = 0;
    char **t_elements;
    uindex_t t_element_count;
    MCCStringSplit(MCStringGetCString(*t_string), ',', t_elements, t_element_count);
    
    for (uindex_t i = 0; i < t_element_count; i++)
    {
        for (uindex_t j = 0; j < p_info -> count; j++)
        {
            if (MCU_strcasecmp(t_elements[i], p_info -> elements[j] . tag) == 0)
            {
                t_value |= 1 << p_info -> elements[j] . bit;
                break;
            }
        }
    }
    
    MCCStringArrayFree(t_elements, t_element_count);
    r_value = t_value;
}

void MCExecParseEnum(MCExecContext& ctxt, MCExecEnumTypeInfo *p_info, MCExecValue p_value, intenum_t& r_value)
{
    MCAutoStringRef t_string;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value . type + 1, kMCExecValueTypeStringRef, &(&t_string));
    if (!ctxt . HasError())
    {    
        bool t_found;
        t_found = false;
        for(uindex_t i = 0; i < p_info -> count; i++)
            if (!p_info -> elements[i] . read_only &&
                MCStringIsEqualTo(*t_string, MCSTR(p_info -> elements[i] . tag), kMCStringOptionCompareCaseless))
            {
                t_found = true;
                r_value = p_info -> elements[i] . value;
            }
        
        if (!t_found)
            ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
    }
}

void MCExecFormatSet(MCExecContext& ctxt, MCExecSetTypeInfo *p_info, intset_t t_value, MCExecValue& r_value)
{
    MCAutoListRef t_list;
    MCListCreateMutable(',', &t_list);
    for(uindex_t i = 0; i < p_info -> count; i++)
        if (((1 << p_info -> elements[i] . bit) & t_value) != 0)
            MCListAppendCString(*t_list, p_info -> elements[i] . tag);
    if (MCListCopyAsString(*t_list, r_value . stringref_value))
        r_value . type = kMCExecValueTypeStringRef;
    else
        ctxt . Throw();
}

void MCExecFormatEnum(MCExecContext& ctxt, MCExecEnumTypeInfo *p_info, intenum_t p_value, MCExecValue& r_value)
{
    bool t_found = false;
    for(uindex_t i = 0; i < p_info -> count; i++)
        if (p_info -> elements[i] . value == p_value)
        {
            MCStringCreateWithCString(p_info -> elements[i] . tag, r_value . stringref_value);
            r_value . type = kMCExecValueTypeStringRef;
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

////////////////////////////////////////////////////////////////////////////////

