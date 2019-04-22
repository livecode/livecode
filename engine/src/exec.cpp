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
#include "handler.h"
#include "hndlrlst.h"
#include "keywords.h"

#include "osspec.h"

#include "debug.h"
#include "param.h"

#include "statemnt.h"
#include "license.h"
#include "scriptpt.h"
#include "newobj.h"

// SN-2014-09-05: [[ Bug 13378 ]] Include the definition of MCServerScript
#ifdef _SERVER
#include "srvscript.h"
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::ForceToString(MCValueRef p_value, MCStringRef& r_string)
{
    return ConvertToString(p_value, r_string);
}

bool MCExecContext::ForceToBoolean(MCValueRef p_value, MCBooleanRef& r_boolean)
{
    return ConvertToBoolean(p_value, r_boolean);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::ConvertToString(MCValueRef p_value, MCStringRef& r_string)
{
    switch(MCValueGetTypeCode(p_value))
    {
    case kMCValueTypeCodeNull:
    case kMCValueTypeCodeArray:
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    case kMCValueTypeCodeBoolean:
        r_string = MCValueRetain(p_value == kMCTrue ? kMCTrueString : kMCFalseString);
        return true;
    case kMCValueTypeCodeName:
        r_string = MCValueRetain(MCNameGetString((MCNameRef)p_value));
        return true;
    case kMCValueTypeCodeString:
        return MCStringCopy((MCStringRef)p_value, r_string);
    case kMCValueTypeCodeData:
        return MCStringCreateWithNativeChars((const char_t *)MCDataGetBytePtr((MCDataRef)p_value), MCDataGetLength((MCDataRef)p_value), r_string);
    case kMCValueTypeCodeList:
        return MCListCopyAsString((MCListRef)p_value, r_string);
    case kMCValueTypeCodeNumber:
    {
	    MCNumberRef t_number = reinterpret_cast<MCNumberRef>(p_value);
	    if (MCNumberIsInteger(t_number))
            // SN-2014-04-28 [[ StonCache ]]
            // Stores the numeric value in the string
		    return
			    MCStringFormat(r_string, "%d", MCNumberFetchAsInteger(t_number)) &&
			    MCStringSetNumericValue(r_string, MCNumberFetchAsReal(t_number));
	    else
		    return MCU_r8tos(MCNumberFetchAsReal(t_number),
		                     m_nffw, m_nftrailing, m_nfforce, r_string);
    }
    break;
    default:
        break;
    }
    return false;
}

bool MCExecContext::ConvertToNumber(MCValueRef p_value, MCNumberRef& r_number)
{
    switch(MCValueGetTypeCode(p_value))
    {
    case kMCValueTypeCodeNull:
        return MCNumberCreateWithInteger(0, r_number);
    case kMCValueTypeCodeBoolean:
    case kMCValueTypeCodeArray:
    case kMCValueTypeCodeList:
        break;
    case kMCValueTypeCodeNumber:
        return MCValueCopy(p_value, (MCValueRef&)r_number);
    case kMCValueTypeCodeName:
        {
            double t_number;
            t_number = 0.0;
            // SN-2014-04-28 [[ StonCache ]]
            // Fetches the numeric value in case it exists, or stores the one therefore computed otherwise
            if (MCStringGetLength(MCNameGetString((MCNameRef)p_value)) != 0 &&
                    !MCStringGetNumericValue(MCNameGetString((MCNameRef)p_value), t_number))
            {
                if (!MCTypeConvertStringToReal(MCNameGetString((MCNameRef)p_value), t_number, m_convertoctals))
                    break;

                // Converting to octals doesn't generate the 10-based number stored in the string
                if (!m_convertoctals)
                    MCStringSetNumericValue(MCNameGetString((MCNameRef)p_value), t_number);
            }

            return MCNumberCreateWithReal(t_number, r_number);
        }
    case kMCValueTypeCodeString:
        {
            double t_number;
            t_number = 0.0;
            // SN-2014-04-28 [[ StonCache ]]
            // Fetches the numeric value in case it exists, or stores the one therefore computed otherwise
            if (MCStringGetLength((MCStringRef)p_value) != 0 && !MCStringGetNumericValue((MCStringRef)p_value, t_number))
            {
                if (!MCTypeConvertStringToReal((MCStringRef)p_value, t_number, m_convertoctals))
                    break;

                // Converting to octals doesn't generate the 10-based number stored in the string
                if (!m_convertoctals)
                    MCStringSetNumericValue((MCStringRef)p_value, t_number);
            }

            return MCNumberCreateWithReal(t_number, r_number);
        }
    case kMCValueTypeCodeData:
        {
            double t_number;
            t_number = 0.0;
            if (!MCTypeConvertDataToReal((MCDataRef)p_value, t_number, m_convertoctals))
                break;
            
            return MCNumberCreateWithReal(t_number, r_number);
        }
    default:
        break;
    }

    return false;
}

bool MCExecContext::ConvertToReal(MCValueRef p_value, real64_t& r_double)
{
	MCAutoNumberRef t_number;
    if (!ConvertToNumber(p_value, &t_number))
		return false;
	r_double = MCNumberFetchAsReal(*t_number);
	return true;
}

// SN-2014-12-03: [[ Bug 14147 ]] Array conversion is not always permissive, neither always strict
bool MCExecContext::ConvertToArray(MCValueRef p_value, MCArrayRef &r_array, bool p_strict)
{
    if (MCValueIsEmpty(p_value))
    {
        r_array = MCValueRetain(kMCEmptyArray);
        return true;
    }
    
	if (MCValueGetTypeCode(p_value) != kMCValueTypeCodeArray)
    {
        // FG-2014-10-21: [[ Bugfix 13724 ]] The legacy behavior requires that
        // anything that can be converted to a string will convert to an empty
        // array (for example, 'the extents of "foo"' should return empty
        // rather than throwing an error).
        MCAutoStringRef t_ignored;
        // SN-2014-12-03: [[ Bug 14147 ]] Do not try the string conversion if the
        if (!p_strict && ConvertToString(p_value, &t_ignored))
        {
            r_array = MCValueRetain(kMCEmptyArray);
            return true;
        }
        
        return false;
    }
    
    r_array = MCValueRetain((MCArrayRef)p_value);
	return true;
}

bool MCExecContext::ConvertToInteger(MCValueRef p_value, integer_t& r_integer)
{
	MCAutoNumberRef t_number;
    if (!ConvertToNumber(p_value, &t_number))
		return false;
	r_integer = MCNumberFetchAsInteger(*t_number);
	return true;
}

bool MCExecContext::ConvertToUnsignedInteger(MCValueRef p_value, uinteger_t& r_integer)
{
	MCAutoNumberRef t_number;
    if (!ConvertToNumber(p_value, &t_number))
		return false;
	r_integer = MCNumberFetchAsUnsignedInteger(*t_number);
	return true;
}

bool MCExecContext::ConvertToBoolean(MCValueRef p_value, MCBooleanRef &r_boolean)
{
    switch(MCValueGetTypeCode(p_value))
    {
    case kMCValueTypeCodeBoolean:
        r_boolean = MCValueRetain((MCBooleanRef)p_value);
        return true;
    case kMCValueTypeCodeNull:
    case kMCValueTypeCodeArray:
    case kMCValueTypeCodeNumber:
    case kMCValueTypeCodeList:
        break;
    case kMCValueTypeCodeName:
        if (MCStringIsEqualTo(MCNameGetString((MCNameRef)p_value), kMCTrueString, kMCStringOptionCompareCaseless))
        {
            r_boolean = MCValueRetain(kMCTrue);
            return true;
        }

        if (MCStringIsEqualTo(MCNameGetString((MCNameRef)p_value), kMCFalseString, kMCStringOptionCompareCaseless))
        {
            r_boolean = MCValueRetain(kMCFalse);
            return true;
        }
        break;
    case kMCValueTypeCodeString:
        if (MCStringIsEqualTo((MCStringRef)p_value, kMCTrueString, kMCStringOptionCompareCaseless))
        {
            r_boolean = MCValueRetain(kMCTrue);
            return true;
        }

        if (MCStringIsEqualTo((MCStringRef)p_value, kMCFalseString, kMCStringOptionCompareCaseless))
        {
            r_boolean = MCValueRetain(kMCFalse);
            return true;
        }
        break;
    case kMCValueTypeCodeData:
        {
            MCAutoStringRef t_string;
            if (MCStringDecode((MCDataRef)p_value, kMCStringEncodingNative, false, &t_string))
                return ConvertToBoolean(*t_string, r_boolean);
            else
                break;
        }
    }

    return false;
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
        // Returns 0 - since we are trying to get a number from something non-assigned
        MCExecTypeRelease(x_value);
        x_value . double_value = 0.0;
        x_value . type = kMCExecValueTypeDouble;
        return true;

    case kMCExecValueTypeValueRef:
    case kMCExecValueTypeBooleanRef:
    case kMCExecValueTypeStringRef:
    case kMCExecValueTypeNameRef:
    case kMCExecValueTypeDataRef:
    case kMCExecValueTypeNumberRef:
    {
        double t_real;
        if (!ConvertToReal(x_value . valueref_value, t_real))
        {
            MCAutoArrayRef t_array;
            // SN-2014-12-03: [[ Bug 14147 ]] An array should not be returned if the
            //  value is neither empty or an array.
            if (!ConvertToArray(x_value . valueref_value, &t_array, true))
                return false;
            
            MCValueRelease(x_value . valueref_value);
            MCExecValueTraits<MCArrayRef>::set(x_value, MCValueRetain(*t_array));
            return true;
        }

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
    if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeData)
    {
        r_data = MCValueRetain((MCDataRef)p_value);
        return true;
    }
    
    MCAutoStringRef t_string;
    if (!ConvertToString(p_value, &t_string))
        return false;
    
    // AL-2014-12-12: [[ Bug 14208 ]] Implement a specific function to aid conversion to data
    return MCDataConvertStringToData(*t_string, r_data);
}

bool MCExecContext::ConvertToName(MCValueRef p_value, MCNameRef& r_name)
{
    
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeName:
        {
            r_name = MCValueRetain((MCNameRef)p_value);
            return true;
        }
            
        case kMCValueTypeCodeString:
        {
            return MCNameCreate((MCStringRef)p_value,
                                r_name);
        }
            
        case kMCValueTypeCodeNumber:
        {
            index_t t_index;
            if (MCNumberStrictFetchAsIndex((MCNumberRef)p_value,
                                           t_index))
            {
                return MCNameCreateWithIndex(t_index,
                                             r_name);
            }
            else
                break;
        }
            
        default:
            break;
    }
    
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
    if ((MCBooleanRef)p_value == kMCTrue)
    {
        r_bool = true;
        return true;
    }

    MCAutoBooleanRef t_boolean;
    if (ConvertToBoolean(p_value, &t_boolean))
    {
        r_bool = *t_boolean == kMCTrue;
        return true;
    }

    return false;
}

bool MCExecContext::ConvertToLegacyPoint(MCValueRef p_value, MCPoint& r_point)
{
    MCAutoStringRef t_string;
	return ConvertToString(p_value, &t_string) && MCU_stoi2x2(*t_string, r_point . x, r_point . y);
}

bool MCExecContext::ConvertToLegacyRectangle(MCValueRef p_value, MCRectangle& r_rect)
{
    MCAutoStringRef t_string;
	int16_t t_left, t_top, t_right, t_bottom;
	if (ConvertToString(p_value, &t_string) &&
		MCU_stoi2x4(*t_string, t_left, t_top, t_right, t_bottom))
	{
		r_rect . x = t_left;
		r_rect . y = t_top;
        // AL-2014-05-13: [[ Bug 12288 ]] Ensure width and height don't underflow.
		r_rect . width = MCU_max(1, t_right - t_left);
		r_rect . height = MCU_max(1, t_bottom - t_top);
    
		return true;
	}
    
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::FormatReal(real64_t p_real, MCStringRef& r_value)
{
	return MCU_r8tos(p_real, GetNumberFormatWidth(), GetNumberFormatTrailing(), GetNumberFormatForce(), r_value);
}

bool MCExecContext::FormatInteger(integer_t p_integer, MCStringRef& r_value)
{
	return MCStringFormat(r_value, "%d", p_integer);
}

bool MCExecContext::FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_value)
{
	return MCStringFormat(r_value, "%u", p_integer);
}

bool MCExecContext::FormatLegacyColor(MCColor p_color, MCStringRef& r_value)
{
    return MCU_format_color(p_color, r_value);
}

////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::TryToConvertToUnsignedInteger(MCValueRef p_value, bool& r_converted, uinteger_t &r_integer)
{
    r_converted = ConvertToUnsignedInteger(p_value, r_integer);
	return true;
}

bool MCExecContext::TryToConvertToReal(MCValueRef p_value, bool& r_converted, real64_t& r_real)
{
    r_converted = ConvertToReal(p_value, r_real);
	return true;
}

bool MCExecContext::TryToConvertToLegacyColor(MCValueRef p_value, bool& r_converted, MCColor& r_color)
{
	MCAutoStringRef t_string;
    if (ConvertToString(p_value, &t_string) &&
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
    if (ConvertToString(p_value, &t_string) &&
		MCU_stoi2x2(*t_string, r_point . x, r_point . y))
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
    if (ConvertToString(p_value, &t_string) &&
		MCU_stoi2x4(*t_string, t_left, t_top, t_right, t_bottom))
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
        if (!MCArrayFetchValueAtIndex(*t_array, i + 1, t_val))
			return false;
		
		MCAutoStringRef t_path;
		if (!MCS_resolvepath((MCStringRef)t_val, &t_path))
			return false;
		
        if (!MCArrayStoreValueAtIndex(*t_path_array, i + 1, *t_path))
			return false;
	}
	
	r_path_array = MCValueRetain(*t_path_array);
	return true;
}


bool MCExecContext::CopyElementAsEnum(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCExecEnumTypeInfo *p_enum_type_info, intenum_t &r_intenum)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
		return false;
		
	MCExecValue t_value;
	t_value . valueref_value = MCValueRetain(t_val);
	t_value . type = kMCExecValueTypeValueRef;
	
	MCExecParseEnum(*this, p_enum_type_info, t_value, r_intenum);
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

bool MCExecContext::CopyOptElementAsArray(MCArrayRef p_array, MCNameRef p_key, bool p_case_sensitive, MCArrayRef &r_array)
{
	MCValueRef t_val = nil;
	if (!MCArrayFetchValue(p_array, p_case_sensitive, p_key, t_val))
    {
        r_array = MCValueRetain(kMCEmptyArray);
        return true;
    }
	return (ConvertToArray(t_val, r_array));
}

////////////////////////////////////////////////////////////////////////////////

bool FormatUnsignedInteger(uinteger_t p_integer, MCStringRef& r_output)
{
	return MCStringFormat(r_output, "%d", p_integer);
}


////////////////////////////////////////////////////////////////////////////////

bool MCExecContext::EvaluateExpression(MCExpression *p_expr, Exec_errors p_error, MCExecValue& r_result)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval_ctxt(*this, r_result);
	
	if (!HasError())
		return true;
	
	LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::TryToEvaluateExpression(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, MCValueRef& r_result)
{
    MCAssert(p_expr != nil);
	
    bool t_failure, t_can_debug;
    t_can_debug = true;
    
    // AL-2014-11-06: [[ Bug 13930 ]] Make sure all the 'TryTo...' functions do the correct thing
    p_expr -> eval(*this, r_result);
    
    while (t_can_debug && (t_failure = HasError()) &&
           (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
    {
        t_can_debug = MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
        
        if (t_can_debug)
            p_expr -> eval(*this, r_result);
    }
    
	if (!t_failure)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToEvaluateExpressionAsDouble(MCExpression *p_expr, uint2 line, uint2 pos, Exec_errors p_error, double& r_result)
{
    MCAssert(p_expr != nil);
	
    bool t_failure, t_can_debug;
    t_can_debug = true;

    MCExecValue t_value;
    double t_result;

    // AL-2014-11-06: [[ Bug 13930 ]] Make sure all the 'TryTo...' functions do the correct thing
    // SN-2014-12-22: [[ Bug 14277 ]] Check whether the evaluation went well before doing any conversion
    p_expr -> eval_ctxt(*this, t_value);
    if (!HasError() && !MCExecTypeIsNumber(t_value . type))
        MCExecTypeConvertAndReleaseAlways(*this, t_value . type, &t_value, kMCExecValueTypeDouble, &t_result);
    
    while (t_can_debug && (t_failure = HasError()) &&
           (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
    {
        t_can_debug = MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
        
        if (t_can_debug)
        {
            p_expr -> eval_ctxt(*this, t_value);
            // SN-2014-12-22: [[ Bug 14277 ]] Check whether the evaluation went well before doing any conversion
            if (!HasError() && !MCExecTypeIsNumber(t_value . type))
                MCExecTypeConvertAndReleaseAlways(*this, t_value . type, &t_value, kMCExecValueTypeDouble, &t_result);
        }
    }
    
	if (!t_failure)
    {
        if (!MCExecTypeIsNumber(t_value . type))
            r_result = t_result;
        else if (t_value . type == kMCExecValueTypeInt)
            r_result = t_value . int_value;
        else if (t_value . type == kMCExecValueTypeUInt)
            r_result = t_value . uint_value;
        else if (t_value . type == kMCExecValueTypeFloat)
            r_result = t_value . float_value;
        else
            r_result = t_value . double_value;
		
		return true;
    }
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToEvaluateParameter(MCParameter *p_param, uint2 line, uint2 pos, Exec_errors p_error, MCExecValue& r_result)
{
    MCAssert(p_param != nil);
	
    bool t_failure, t_can_debug;
    t_can_debug = true;
    
    // AL-2014-11-06: [[ Bug 13930 ]] Make sure all the 'TryTo...' functions do the correct thing
    while (t_can_debug && (t_failure = (!p_param -> eval_ctxt(*this, r_result) || HasError())) &&
           (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
    {
        t_can_debug = MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
	
    
	if (!t_failure)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToEvaluateExpressionAsNonStrictBool(MCExpression * p_expr, uint2 line, uint2 pos, Exec_errors p_error, bool& r_value)
{
    MCAssert(p_expr != nil);
    
    bool t_failure, t_can_debug;
    t_can_debug = true;
    
    // AL-2014-11-06: [[ Bug 13930 ]] Make sure all the 'TryTo...' functions do the correct thing
    while (t_can_debug && (t_failure = !EvalExprAsNonStrictBool(p_expr, p_error, r_value)) &&
           (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
    {
        t_can_debug = MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
    
    if (!t_failure)
		return true;
	
	LegacyThrow(p_error);
	return false;
}

bool MCExecContext::TryToSetVariable(MCVarref *p_var, uint2 line, uint2 pos, Exec_errors p_error, MCExecValue p_value)
{
    bool t_failure, t_can_debug;
    t_can_debug = true;
    
    // AL-2014-11-06: [[ Bug 13930 ]] Make sure all the 'TryTo...' functions do the correct thing
    while (t_can_debug && (t_failure = (!p_var -> give_value(*this, p_value) || HasError())) &&
           (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
    {
        t_can_debug = MCB_error(*this, line, pos, p_error);
        IgnoreLastError();
    }
    
	if (!t_failure)
		return true;
	
	LegacyThrow(p_error);
	return false;
}
//////////

template <typename T>
static bool EvalExprAs(MCExecContext* self, MCExpression *p_expr, Exec_errors p_error, T& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval(*self, r_value);
	
	if (!self->HasError())
		return true;
	
	self->LegacyThrow(p_error);
	
	return false;
}

template <typename T>
static bool EvalExprAsStrictNumber(MCExecContext* self, MCExpression *p_expr, Exec_errors p_error, MCExecValueType p_type, T& r_value)
{
	MCAssert(p_expr != nil);
	
    MCExecValue t_value;
	
	p_expr -> eval_ctxt(*self, t_value);
    
    if (t_value . type == kMCExecValueTypeNone
        || (MCExecTypeIsValueRef(t_value . type) && MCValueIsEmpty(t_value . valueref_value)))
    {
        self -> LegacyThrow(p_error);
        return false;
    }
    
    if (!self -> HasError())
        MCExecTypeConvertAndReleaseAlways(*self, t_value . type, &t_value, p_type, &r_value);
	
	if (!self -> HasError())
		return true;
	
	self -> LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalExprAsStrictUInt(MCExpression *p_expr, Exec_errors p_error, uinteger_t& r_value) { return EvalExprAsStrictNumber(this, p_expr, p_error, kMCExecValueTypeUInt, r_value); }

bool MCExecContext::EvalExprAsStrictInt(MCExpression *p_expr, Exec_errors p_error, integer_t& r_value) { return EvalExprAsStrictNumber(this, p_expr, p_error, kMCExecValueTypeInt, r_value); }

template <typename T>
static bool EvalExprAsNumber(MCExecContext* self, MCExpression *p_expr, Exec_errors p_error, MCExecValueType p_type, T& r_value)
{
	MCAssert(p_expr != nil);
	
    MCExecValue t_value;
    
	p_expr -> eval_ctxt(*self, t_value);
    
    if (!self -> HasError())
        MCExecTypeConvertAndReleaseAlways(*self, t_value . type, &t_value, p_type, &r_value);
	
	if (!self -> HasError())
		return true;
	
	self -> LegacyThrow(p_error);
	
	return false;
}

bool MCExecContext::EvalExprAsValueRef(MCExpression *p_expr, Exec_errors p_error, MCValueRef& r_value)     { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsBooleanRef(MCExpression *p_expr, Exec_errors p_error, MCBooleanRef& r_value) { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_value)   { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef& r_value)       { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef& r_value)       { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value)     { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsNumberRef(MCExpression *p_expr, Exec_errors p_error, MCNumberRef& r_value)   { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsBool(MCExpression *p_expr, Exec_errors p_error, bool& r_value)               { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsChar(MCExpression *p_expr, Exec_errors p_error, char_t& r_value)             { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsPoint(MCExpression *p_expr, Exec_errors p_error, MCPoint& r_value)           { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsColor(MCExpression *p_expr, Exec_errors p_error, MCColor& r_value)           { return EvalExprAs(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalExprAsRectangle(MCExpression *p_expr, Exec_errors p_error, MCRectangle& r_value)   { return EvalExprAs(this, p_expr, p_error, r_value); }


template <typename T>
static bool EvalOptionalExprAs(MCExecContext* self, MCExpression *p_expr, T p_default, Exec_errors p_error, T& r_value)
{
	if (p_expr == nil)
	{
		r_value = MCExecValueTraits<T>::retain(p_default);
		return true;
	}

    return EvalExprAs(self, p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsValueRef(MCExpression *p_expr, MCValueRef p_default, Exec_errors p_error, MCValueRef& r_value)       { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsBooleanRef(MCExpression *p_expr, MCBooleanRef p_default, Exec_errors p_error, MCBooleanRef& r_value) { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsStringRef(MCExpression *p_expr, MCStringRef p_default, Exec_errors p_error, MCStringRef& r_value)    { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsNameRef(MCExpression *p_expr, MCNameRef p_default, Exec_errors p_error, MCNameRef& r_value)          { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsDataRef(MCExpression *p_expr, MCDataRef p_default, Exec_errors p_error, MCDataRef& r_value)          { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsArrayRef(MCExpression *p_expr, MCArrayRef p_default, Exec_errors p_error, MCArrayRef& r_value)       { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsNumberRef(MCExpression *p_expr, MCNumberRef p_default, Exec_errors p_error, MCNumberRef& r_value)    { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsUInt(MCExpression *p_expr, uinteger_t p_default, Exec_errors p_error, uinteger_t& r_value)           { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsInt(MCExpression *p_expr, integer_t p_default, Exec_errors p_error, integer_t& r_value)              { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsBool(MCExpression *p_expr, bool p_default, Exec_errors p_error, bool& r_value)                       { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsDouble(MCExpression *p_expr, double p_default, Exec_errors p_error, double& r_value)                 { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsChar(MCExpression *p_expr, char_t p_default, Exec_errors p_error, char_t& r_value)                   { return EvalOptionalExprAs(this, p_expr, p_default, p_error, r_value); }

template <typename T>
static bool EvalOptionalExprAsPtr(MCExecContext* self, MCExpression *p_expr, T* p_default, Exec_errors p_error, T*& r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return true;
	}

	// Makes sure the return parameter isn't a nil pointer
    MCAssert(r_value != nil);

    return EvalExprAs(self, p_expr, p_error, *r_value);
}

bool MCExecContext::EvalOptionalExprAsPoint(MCExpression *p_expr, MCPoint *p_default, Exec_errors p_error, MCPoint*& r_value)              { return EvalOptionalExprAsPtr(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsColor(MCExpression *p_expr, MCColor *p_default, Exec_errors p_error, MCColor*& r_value)              { return EvalOptionalExprAsPtr(this, p_expr, p_default, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsRectangle(MCExpression *p_expr, MCRectangle* p_default, Exec_errors p_error, MCRectangle*& r_value)  { return EvalOptionalExprAsPtr(this, p_expr, p_default, p_error, r_value); }


template <typename T>
static bool EvalOptionalExprAsNullable(MCExecContext* self, MCExpression *p_expr, Exec_errors p_error, T& r_value)
{
    if (p_expr == nil)
    {
        r_value = nil;
        return true;
    }

    return EvalExprAs(self, p_expr, p_error, r_value);
}

bool MCExecContext::EvalOptionalExprAsNullableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef &r_value) { return EvalOptionalExprAsNullable(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsNullableDataRef(MCExpression *p_expr, Exec_errors p_error, MCDataRef &r_value)     { return EvalOptionalExprAsNullable(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsNullableNameRef(MCExpression *p_expr, Exec_errors p_error, MCNameRef &r_value)     { return EvalOptionalExprAsNullable(this, p_expr, p_error, r_value); }
bool MCExecContext::EvalOptionalExprAsNullableArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef &r_value)   { return EvalOptionalExprAsNullable(this, p_expr, p_error, r_value); }


void MCExecContext::TryToEvalExprAsArrayRef(MCExpression *p_expr, Exec_errors p_error, MCArrayRef& r_value)
{
	MCAssert(p_expr != nil);
	
	p_expr -> eval(*this, r_value);
	
	if (!HasError())
		return;
	
	IgnoreLastError();
	r_value = MCValueRetain(kMCEmptyArray);
}

bool MCExecContext::EvalExprAsUInt(MCExpression *p_expr, Exec_errors p_error, uinteger_t& r_value) { return EvalExprAsNumber(this, p_expr, p_error, kMCExecValueTypeUInt, r_value); }
bool MCExecContext::EvalExprAsInt(MCExpression *p_expr, Exec_errors p_error, integer_t& r_value) { return EvalExprAsNumber(this, p_expr, p_error, kMCExecValueTypeInt, r_value); }
bool MCExecContext::EvalExprAsDouble(MCExpression *p_expr, Exec_errors p_error, double& r_value) { return EvalExprAsNumber(this, p_expr, p_error, kMCExecValueTypeDouble, r_value); }

bool MCExecContext::EvalExprAsNonStrictBool(MCExpression *p_expr, Exec_errors p_error, bool& r_value)
{
    MCAssert(p_expr != nil);
	
	MCAutoStringRef t_value;
    p_expr -> eval(*this, &t_value);

    if (!HasError())
	{
		r_value = MCStringIsEqualTo(*t_value, kMCTrueString, kMCStringOptionCompareCaseless);
		return true;
	}
	
	LegacyThrow(p_error);

	return false;
}

// AL-2014-04-01: [[ Bug 12071 ]] Need to be able to fail to eval color without throwing an error.
void MCExecContext::TryToEvalOptionalExprAsColor(MCExpression *p_expr, MCColor *p_default, Exec_errors p_error, MCColor *&r_value)
{
	if (p_expr == nil)
	{
		r_value = p_default;
		return;
    }
    
    // Makes sure the return parameter isn't a nil pointer
    MCAssert(r_value != nil);
    
    if (EvalExprAs(this, p_expr, p_error, *r_value))
        return;
    
    IgnoreLastError();
    r_value = nil;
}


bool MCExecContext::EvalExprAsMutableStringRef(MCExpression *p_expr, Exec_errors p_error, MCStringRef& r_mutable_string)
{
    MCAutoStringRef t_string;
    if (!EvalExprAs(this, p_expr, p_error, &t_string))
        return false;
    
    return MCStringMutableCopy(*t_string, r_mutable_string);
}

////////////////////////////////////////////////////////////////////////////////

template<bool (&check)(uint2, uint2)>
static bool EnsureIsAllowed(MCExecContext* self)
{
	if (check(0, 0))
		return true;
		
	self -> Throw();
	
	return false;
}

bool MCExecContext::EnsurePrintingIsAllowed(void)      { return EnsureIsAllowed<MCSecureModeCheckPrinter>(this); }
bool MCExecContext::EnsureDiskAccessIsAllowed(void)    { return EnsureIsAllowed<MCSecureModeCheckDisk>(this); }
bool MCExecContext::EnsureProcessIsAllowed(void)       { return EnsureIsAllowed<MCSecureModeCheckProcess>(this); }
bool MCExecContext::EnsureNetworkAccessIsAllowed(void) { return EnsureIsAllowed<MCSecureModeCheckNetwork>(this); }
bool MCExecContext::EnsurePrivacyIsAllowed(void)       { return EnsureIsAllowed<MCSecureModeCheckPrivacy>(this); }

////////////////////////////////////////////////////////////////////////////////

// MW-2013-11-08: [[ RefactorIt ]] Returns the it var for the current context.
MCVarref* MCExecContext::GetIt() const
{
    // If we have a handler, then get it from there.
    if (m_curhandler != nil)
        return m_curhandler -> getit();

    // SN-2014-09-05: [[ Bug 13378 ]] Changed the #ifdef to be _SERVER, and updated the member name
#ifdef _SERVER
    // If we are here it means we must be in global scope, executing in a
    // MCServerScript object.
    return static_cast<MCServerScript *>(m_object . object) -> GetIt();
#else
    // We should never get here as execution only occurs within handlers unless
    // in server mode.
    assert(false);
    return nil;
#endif
}

void MCExecContext::SetItToValue(MCValueRef p_value)
{
    GetIt() -> set(*this, p_value);
}

void MCExecContext::GiveValueToIt(/* take */ MCExecValue& p_value)
{
    GetIt() -> give_value(*this, p_value);
}

void MCExecContext::SetItToEmpty(void)
{
	SetItToValue(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2011-06-22: [[ SERVER ]] Provides augmented functionality for finding
//   variables if there is no handler (i.e. global server scope).
Parse_stat MCExecContext::FindVar(MCNameRef p_name, MCVarref **r_var)
{
    Parse_stat t_stat;
    t_stat = PS_ERROR;

    if (m_curhandler != NULL)
        t_stat = m_curhandler -> findvar(p_name, r_var);
    else if (m_hlist != NULL)
    {
        // MW-2011-08-23: [[ UQL ]] We are searching in global context, so do include UQLs.
        t_stat = m_hlist -> findvar(p_name, false, r_var);
    }

    return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

void MCExecContext::LegacyThrow(Exec_errors p_error, MCValueRef p_hint)
{
	MCeerror -> add(p_error, m_line, m_pos, p_hint);
	m_stat = ES_ERROR;
}

void MCExecContext::LegacyThrow(Exec_errors p_error, uint32_t p_hint)
{
	MCeerror -> add(p_error, m_line, m_pos, p_hint);
	m_stat = ES_ERROR;
}

void MCExecContext::UserThrow(MCStringRef p_error)
{
	MCeerror -> copystringref(p_error, True);
	m_stat = ES_ERROR;
}

MCObjectHandle MCExecContext::GetObjectHandle(void) const
{
    extern MCExecContext *MCECptr;
	return MCECptr->GetObject()->GetHandle();
}

Exec_stat MCExecContext::Catch(uint2 p_line, uint2 p_pos)
{
	return ES_ERROR;
}

void MCExecContext::SetTheResultToEmpty(void)
{
	MCresult -> clear();
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheResultToValue(MCValueRef p_value)
{
    MCresult -> setvalueref(p_value);
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheResultToStaticCString(const char *p_cstring)
{
    MCresult -> sets(p_cstring);
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheResultToNumber(real64_t p_value)
{
    MCresult -> setnvalue(p_value);
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::GiveCStringToResult(char *p_cstring)
{
    MCresult -> grab(p_cstring, MCCStringLength(p_cstring));
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheResultToCString(const char *p_string)
{
    MCresult -> copysvalue(p_string);
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheResultToBool(bool p_bool)
{
    MCresult -> sets(MCU_btos(p_bool));
    MCresultmode = kMCExecResultModeReturn;
}

void MCExecContext::SetTheReturnError(MCValueRef p_value)
{
    MCresult -> setvalueref(p_value);
    MCresultmode = kMCExecResultModeReturnError;
}

void MCExecContext::SetTheReturnValue(MCValueRef p_value)
{
    MCresult -> setvalueref(p_value);
    MCresultmode = kMCExecResultModeReturnValue;
}

// SN-2015-06-03: [[ Bug 11277 ]] Refactor MCExecPoint update
void MCExecContext::deletestatements(MCStatement* p_statements)
{
    while (p_statements != NULL)
    {
        MCStatement *tsptr = p_statements;
        p_statements = p_statements->getnext();
        delete tsptr;
    }
}

void MCExecContext::eval(MCExecContext &ctxt, MCStringRef p_expression, MCValueRef &r_value)
{
    MCScriptPoint sp(ctxt, p_expression);
    // SN-2015-06-03: [[ Bug 11277 ]] When we are out of handler, then it simply
    //  sets the ScriptPoint handler to NULL (same as post-constructor state).
    sp.sethandler(ctxt . GetHandler());
    MCExpression *exp = NULL;
    Symbol_type type;

    if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
        ctxt . EvalExprAsValueRef(exp, EE_HANDLER_BADEXP, r_value);
    else
        ctxt . Throw();

    delete exp;
}

void MCExecContext::eval_ctxt(MCExecContext &ctxt, MCStringRef p_expression, MCExecValue& r_value)
{
    MCScriptPoint sp(ctxt, p_expression);
    // SN-2015-06-03: [[ Bug 11277 ]] When we are out of handler, then it simply
    //  sets the ScriptPoint handler to NULL (same as post-constructor state).
    sp.sethandler(ctxt . GetHandler());
    MCExpression *exp = NULL;
    Symbol_type type;

    if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
        ctxt . EvaluateExpression(exp, EE_HANDLER_BADEXP, r_value);
    else
        ctxt . Throw();

    delete exp;
}

void MCExecContext::doscript(MCExecContext &ctxt, MCStringRef p_script, uinteger_t p_line, uinteger_t p_pos)
{
    MCScriptPoint sp(ctxt, p_script);
    MCStatement *curstatement = NULL;
    MCStatement *statements = NULL;
    MCStatement *newstatement = NULL;
    Symbol_type type;
    const LT *te;
    Exec_stat stat = ES_NORMAL;
    Boolean oldexplicit = MCexplicitvariables;
    MCexplicitvariables = False;
    uint4 count = 0;
    sp.setline(p_line - 1);
    while (stat == ES_NORMAL)
    {
        switch (sp.next(type))
        {
        case PS_NORMAL:
            if (type == ST_ID)
                if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
                    newstatement = new (nothrow) MCComref(sp.gettoken_nameref());
                else
                {
                    if (te->type != TT_STATEMENT)
                    {
                        MCeerror->add(EE_DO_NOTCOMMAND, p_line, p_pos, sp.gettoken_stringref());
                        stat = ES_ERROR;
                    }
                    else
                        newstatement = MCN_new_statement(te->which);
                }
            else
            {
                MCeerror->add(EE_DO_NOCOMMAND, p_line, p_pos, sp.gettoken_stringref());
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
                    MCeerror->add(EE_DO_BADCOMMAND, p_line, p_pos, p_script);
                    stat = ES_ERROR;
                }
                count += curstatement->linecount();
            }
            break;
        case PS_EOL:
            if (sp.skip_eol() != PS_NORMAL)
            {
                MCeerror->add(EE_DO_BADLINE, p_line, p_pos, p_script);
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
        MCeerror -> add(EE_DO_NOTLICENSED, p_line, p_pos, p_script);
        stat = ES_ERROR;
    }

    if (stat == ES_ERROR)
    {
        deletestatements(statements);
        ctxt.Throw();
        return;
    }

    MCExecContext ctxt2(ctxt);
    while (statements != NULL)
    {
        statements->exec_ctxt(ctxt2);
        stat = ctxt2 . GetExecStat();
        if (stat == ES_ERROR)
        {
            deletestatements(statements);
            MCeerror->add(EE_DO_BADEXEC, p_line, p_pos, p_script);
            ctxt.Throw();
            return;
        }
        if (MCexitall || stat != ES_NORMAL)
        {
            deletestatements(statements);
            if (stat == ES_ERROR)
                ctxt.Throw();
            return;
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
        MCeerror->add(EE_DO_ABORT, p_line, p_pos);
        ctxt.Throw();
        return;
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////

template<typename Formatter, typename Element>
static bool MCPropertyFormatList(Formatter p_format,
                                 Element *p_list,
                                 uindex_t p_count,
                                 char_t p_delimiter,
                                 MCStringRef &r_string)
{
    if (p_count == 0)
        return MCStringCopy(kMCEmptyString, r_string);

    MCAutoListRef t_list;
    if (!MCListCreateMutable(p_delimiter, &t_list))
        return false;

	for (uindex_t i = 0; i < p_count; ++i)
	{
		MCAutoStringRef t_formatted;
		if (!p_format(p_list[i], &t_formatted))
			return false;
		if (!MCListAppend(*t_list, *t_formatted))
			return false;
	}

    return MCListCopyAsString(*t_list, r_string);
}

static bool MCPropertyFormatUIntList(uinteger_t *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    auto t_format =
        [](uinteger_t& x, MCStringRef& s) { return MCStringFormat(s, "%d", x); };
    return MCPropertyFormatList(t_format, p_list, p_count, p_delimiter, r_string);
}

static bool MCPropertyFormatDoubleList(double *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    auto t_format =
        [](double& x, MCStringRef& s) { return MCStringFormat(s, "%f", x); };
    return MCPropertyFormatList(t_format, p_list, p_count, p_delimiter, r_string);
}

static bool MCPropertyFormatStringList(MCStringRef *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    return MCPropertyFormatList(MCStringCopy, p_list, p_count, p_delimiter, r_string);
}

static bool MCPropertyFormatPointList(MCPoint *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
	auto t_format = [](MCPoint& p, MCStringRef& s)
	{
		if  (p.x == MININT2 && p.y == MININT2)
		{
			s = MCValueRetain(kMCEmptyString);
			return s != nullptr;
		}
		else
			return MCStringFormat(s, "%d,%d", p.x, p.y);
	};
    return MCPropertyFormatList(t_format, p_list, p_count, p_delimiter, r_string);
}

static bool MCPropertyParseLooseUIntList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, uinteger_t*& r_list)
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
		// PM-2015-10-13: [[ Bug 16203 ]] Replace any "empty" elements with 0 in the list, ignoring trailing delimiters
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareExact, t_new_offset))
		{
			if (t_old_offset == t_length)
				break;
			t_new_offset = t_length;
		}
		
		if (t_new_offset == t_old_offset)
			t_success = t_list . Push(0);
		else
		{
			MCAutoStringRef t_uint_string;
			uinteger_t t_d;
        
			if (t_success)
				t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_uint_string);
			
			if (t_success)
				t_success = MCU_stoui4(*t_uint_string, t_d);
			
			if (t_success)
				t_success = t_list . Push(t_d);
		}
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static bool MCPropertyParseLooseDoubleList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, double*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        r_list = nil;
        return true;
    }
    
	MCAutoArray<double> t_list;
	
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
		
	while (t_success && t_old_offset <= t_length)
	{
		// PM-2015-10-13: [[ Bug 16203 ]] Replace any "empty" elements with 0.0 in the list, ignoring trailing delimiters
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareExact, t_new_offset))
		{
			if (t_old_offset == t_length)
			break;
			t_new_offset = t_length;
		}

		if (t_new_offset == t_old_offset)
			t_success = t_list . Push(0.0);
		else
		{
			MCAutoStringRef t_double_string;
			double t_d;
			
            if (t_success)
                t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_double_string);
            
            if (t_success)
                t_success = MCTypeConvertStringToReal(*t_double_string, t_d);
            
            if (t_success)
                t_success = t_list . Push(t_d);
		}
		
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
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), t_string);
		
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
	
    // AL-2014-05-19: [[ Bug 12428 ]] Don't do the loop if the next offset is after the end of the string.
	while (t_success && t_old_offset < t_length)
	{
		MCAutoStringRef t_point_string;
        MCPoint t_point;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset < t_old_offset)
            break;
        
        if (t_new_offset == t_old_offset)
        {
            // Special case: we have 2 times in a row the delimiter,
            // the next point is not link to the previous one - we add a
            // {MIN,MIN} point to ensure this information is passed to the property setter
            t_point.x = MININT2;
            t_point.y = MININT2;
        }
        else
        {
            if (t_success)
                t_success = MCStringCopySubstring(p_input, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_point_string);
            
            if (t_success)
                MCU_stoi2x2(*t_point_string, t_point . x, t_point . y);
        }
        
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
    MCAssert(prop -> getter != nil);
    
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
            ((void(*)(MCExecContext&, void *, uinteger_t&))prop -> getter)(ctxt, mark, r_value . uint_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeUInt;
            }
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            ((void(*)(MCExecContext&, void *, double&))prop -> getter)(ctxt, mark, r_value . double_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeDouble;
            }
        }
            break;
            
        case kMCPropertyTypeChar:
        {
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
                if (r_value . stringref_value == nil)
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
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
            ((void(*)(MCExecContext&, void *, MCNameRef&))prop->getter)(ctxt, mark, r_value . nameref_value);
            if (!ctxt.HasError())
            {
                r_value . type = kMCExecValueTypeNameRef;
            }
        }
        break;
            
        case kMCPropertyTypeColor:
        {
            ((void(*)(MCExecContext&, void *, MCColor&))prop -> getter)(ctxt, mark, r_value . color_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeColor;
            }
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            ((void(*)(MCExecContext&, void *, MCRectangle&))prop -> getter)(ctxt, mark, r_value . rectangle_value);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeRectangle;
            }
        }
            break;
        
        case kMCPropertyTypeRectangle32:
        {
            MCRectangle32 t_value;
            ((void(*)(MCExecContext&, void *, MCRectangle32&))prop -> getter)(ctxt, mark, t_value);
            
            MCAutoStringRef t_string;
            if (!ctxt . HasError())
            {
                if (MCStringFormat(&t_string, "%d,%d,%d,%d", t_value.x, t_value.y, t_value.x + t_value.width, t_value.y + t_value.height))
                {
                    r_value . stringref_value = t_string.Take();
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else
                {
                    ctxt.Throw();
                }
            }
        }
        break;
            
        case kMCPropertyTypePoint:
        {
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
        case kMCPropertyTypeInt32X4:
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
            
        case kMCPropertyTypeOptionalBool:
        {
            bool t_value;
            bool *t_value_ptr;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool*&))prop -> getter)(ctxt, mark, t_value_ptr);
            if (!ctxt . HasError())
            {
                if (t_value_ptr != nil)
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
            
		case kMCPropertyTypeOptionalDouble:
		{
			double t_value;
			double *t_value_ptr;
			t_value_ptr = &t_value;
			((void(*)(MCExecContext&, void *, double *&))prop -> getter)(ctxt, mark, t_value_ptr);
			if (!ctxt . HasError())
			{
				if (t_value_ptr != nil)
				{
					r_value . double_value = t_value;
					r_value . type = kMCExecValueTypeDouble;
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
        case kMCPropertyTypeItemsOfString:
        {
            MCStringRef* t_value = nil;
            uindex_t t_count = 0;
            ((void(*)(MCExecContext&, void *, uindex_t&, MCStringRef*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                if (MCPropertyFormatStringList(t_value, t_count, '\n', r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
                for (uindex_t i = 0; i < t_count; ++i)
                    MCValueRelease(t_value[i]);
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeLinesOfLooseUInt:
        case kMCPropertyTypeItemsOfLooseUInt:
        {
            uinteger_t* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            ((void(*)(MCExecContext&, void *, uindex_t&, uinteger_t*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                char_t t_delimiter;
                t_delimiter = prop -> type == kMCPropertyTypeLinesOfLooseUInt ? '\n' : ',';
                if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeLinesOfLooseDouble:
        {
            double* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            ((void(*)(MCExecContext&, void *, uindex_t&, double*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                char_t t_delimiter;
                t_delimiter = prop -> type == kMCPropertyTypeLinesOfLooseDouble ? '\n' : ',';
                if (MCPropertyFormatDoubleList(t_value, t_count, t_delimiter, r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeLinesOfPoint:
        case kMCPropertyTypeLegacyPoints:
        {
            MCPoint* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            ((void(*)(MCExecContext&, void *, uindex_t&, MCPoint*&))prop -> getter)(ctxt, mark, t_count, t_value);
            if (!ctxt . HasError())
            {
                if (MCPropertyFormatPointList(t_value, t_count, '\n', r_value . stringref_value))
                {
                    r_value . type = kMCExecValueTypeStringRef;
                }
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeMixedBool:
        {
            bool t_mixed;
            bool t_value;
            ((void(*)(MCExecContext&, void *, bool&, bool&))prop -> getter)(ctxt, mark, t_mixed, t_value);
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
            ((void(*)(MCExecContext&, void *, bool&, uinteger_t&))prop -> getter)(ctxt, mark, t_mixed, t_value);
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
		
		// PM-2016-05-18: [[ Bug 17666 ]] Handle case of kMCPropertyTypeMixedInt16
		case kMCPropertyTypeMixedInt16:
        {
            bool t_mixed;
            integer_t t_value;
            ((void(*)(MCExecContext&, void *, bool&, integer_t&))prop -> getter)(ctxt, mark, t_mixed, t_value);
            if (!ctxt . HasError())
            {
                if (t_mixed)
                {
                    r_value . stringref_value = MCSTR(MCmixedstring);
                    r_value . type = kMCExecValueTypeStringRef;
                }
                else
                {
                    r_value . int_value = t_value;
                    r_value . type = kMCExecValueTypeInt;
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
                else if (t_value_ptr != nil)
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
                else if (t_value_ptr != nil)
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
                else if (t_value_ptr != nil)
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
            
            break;
        }
            
        case kMCPropertyTypeMixedOptionalEnum:
        {
            int t_value;
            int *t_value_ptr;
            bool t_mixed;
            t_value_ptr = &t_value;
            ((void(*)(MCExecContext&, void *, bool&, int*&))prop -> getter)(ctxt, mark, t_mixed, t_value_ptr);
            if (!ctxt . HasError())
            {
                r_value . type = kMCExecValueTypeStringRef;
                if (t_mixed)
                    r_value . stringref_value = MCSTR(MCmixedstring);
                else if (t_value_ptr == nil)
                    r_value . stringref_value = MCValueRetain(kMCEmptyString);
                else
                {
                    MCExecEnumTypeInfo *t_enum_info;
                    t_enum_info = (MCExecEnumTypeInfo *)(prop -> type_info);
                    MCExecFormatEnum(ctxt, t_enum_info, t_value, r_value);
                }
            }
        }
            break;
        
        case kMCPropertyTypeMixedItemsOfString:
        {
            bool t_mixed;
            MCStringRef* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            ((void(*)(MCExecContext&, void *, bool&, uindex_t&, MCStringRef*&))prop -> getter)(ctxt, mark, t_mixed, t_count, t_value);
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
                    t_delimiter = ',';
                    if (MCPropertyFormatStringList(t_value, t_count, t_delimiter, r_value . stringref_value))
                    {
                        r_value . type = kMCExecValueTypeStringRef;
                    }
                }
                for (uinteger_t i = 0; i < t_count; ++i)
                    MCValueRelease(t_value[i]);
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;
            
        case kMCPropertyTypeMixedLinesOfLooseUInt:
        case kMCPropertyTypeMixedItemsOfLooseUInt:
        {
            bool t_mixed;
            uinteger_t* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
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
                    t_delimiter = prop -> type == kMCPropertyTypeMixedLinesOfLooseUInt ? '\n' : ',';
                    if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, r_value . stringref_value))
                    {
                        r_value . type = kMCExecValueTypeStringRef;
                    }
                }
                // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
                if (t_count > 0)
                    MCMemoryDeleteArray(t_value);
            }
        }
            break;    
        
        case kMCPropertyTypeRecord:
        {
            ((void(*)(MCExecContext&, void *, MCExecValue&))prop -> getter)(ctxt, mark, r_value);
        }
            break;
         
        case kMCPropertyTypeProperItemsOfString:
        case kMCPropertyTypeProperLinesOfString:
        {
            MCAutoProperListRef t_proper_list;
            ((void(*)(MCExecContext&, void *, MCProperListRef&))prop -> getter)(ctxt, mark, &t_proper_list);
            if (!ctxt . HasError())
            {
                MCListRef t_list;
                /* UNCHECKED */ MCListCreateMutable(prop -> type == kMCPropertyTypeProperLinesOfString ? '\n' : ',', t_list);
                uintptr_t t_iterator;
                t_iterator = 0;
                MCValueRef t_element;
                while(MCProperListIterate(*t_proper_list, t_iterator, t_element))
                    /* UNCHECKED */ MCListAppend(t_list, t_element);
                
                r_value . type = kMCExecValueTypeStringRef;
                /* UNCHECKED */ MCListCopyAsStringAndRelease(t_list, r_value . stringref_value);
            }
        }
        break;
            
        default:
            ctxt . Unimplemented();
            break;
    }
}

void MCExecStoreProperty(MCExecContext& ctxt, const MCPropertyInfo *prop, void *mark, MCExecValue p_value)
{
    MCAssert(prop -> setter != nil);
    
    switch(prop -> type)
    {
        case kMCPropertyTypeAny:
        {
            MCAutoValueRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeValueRef, &(&t_value));
            ((void(*)(MCExecContext&, void *, MCValueRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeMixedBool:
        case kMCPropertyTypeBool:
        {
            bool t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeBool, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, bool))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeMixedInt16:
        case kMCPropertyTypeInt16:
        {
            integer_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeInt, &t_value);
            if (t_value < -32768 || t_value > 32767)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt32:
        {
            integer_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeInt, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, integer_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
         
        case kMCPropertyTypeMixedUInt16:
        case kMCPropertyTypeUInt16:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeUInt, &t_value);
            if (t_value > 65535)
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeUInt32:
        {
            uinteger_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeUInt, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeDouble:
        {
            double t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDouble, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, double))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeChar:
        {
            char_t t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeChar, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, char_t))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeString:
        {
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeBinaryString:
        {
            MCAutoDataRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDataRef, &(&t_value));
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCDataRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeName:
        {
            MCNewAutoNameRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeNameRef, &(&t_value));
            if (!ctxt.HasError())
                ((void(*)(MCExecContext&, void *, MCNameRef))prop->setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeColor:
        {
            MCColor t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeColor, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCColor))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeRectangle:
        {
            MCRectangle t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeRectangle, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeRectangle32:
        {
            int4 a, b, c, d;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi4x4(*t_value, a, b, c, d))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
            if (!ctxt . HasError())
            {
                MCRectangle32 t_rect;
                t_rect.x = a;
                t_rect.y = b;
                t_rect.width = c - a;
                t_rect.height = d - b;
                ((void(*)(MCExecContext&, void *, MCRectangle32))prop -> setter)(ctxt, mark, t_rect);
            }
        }
        break;
            
        case kMCPropertyTypePoint:
        {
            MCPoint t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypePoint, &t_value);
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint))prop -> setter)(ctxt, mark, t_value);
        }
            break;
            
        case kMCPropertyTypeInt16X2:
        {
            int2 a, b;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi2x2(*t_value, a, b))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTPAIR);
            if (!ctxt . HasError())
            {
                integer_t t_int_value[2];
                t_int_value[0] = a;
                t_int_value[1] = b;
                ((void(*)(MCExecContext&, void *, integer_t[2]))prop -> setter)(ctxt, mark, t_int_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt16X4:
        {
            int2 a, b, c, d;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi2x4(*t_value, a, b, c, d))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
            if (!ctxt . HasError())
            {
                integer_t t_int_value[4];
                t_int_value[0] = a;
                t_int_value[1] = b;
                t_int_value[2] = c;
                t_int_value[3] = d;
                ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> setter)(ctxt, mark, t_int_value);
            }
        }
            break;
            
        case kMCPropertyTypeInt32X4:
        {
            int4 a, b, c, d;
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            if (!MCU_stoi4x4(*t_value, a, b, c, d))
                ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
            if (!ctxt . HasError())
            {
                integer_t t_int_value[4];
                t_int_value[0] = a;
                t_int_value[1] = b;
                t_int_value[2] = c;
                t_int_value[3] = d;
                ((void(*)(MCExecContext&, void *, integer_t[4]))prop -> setter)(ctxt, mark, t_int_value);
            }
        }
            break;
            
        case kMCPropertyTypeArray:
        {
            MCAutoArrayRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeArrayRef, &(&t_value));
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

            intenum_t t_value;
            intenum_t* t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
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
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input_value));
            
            char t_value[64];
            ((MCExecCustomTypeParseProc)t_custom_info -> parse)(ctxt, *t_input_value, t_value);
            if (!ctxt . HasError())
            {
                ((void(*)(MCExecContext&, void *, void *))prop -> setter)(ctxt, mark, t_value);
                ((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
            }
        }
            break;
            
        case kMCPropertyTypeMixedOptionalBool:
        case kMCPropertyTypeOptionalBool:
        {
            bool t_value;
            bool *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeBool, &t_value);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, bool*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalUInt8:
        case kMCPropertyTypeOptionalUInt8:
        {
            uinteger_t t_value;
            uinteger_t *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeUInt, &t_value);
                if (t_value > 255)
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedOptionalInt16:
        case kMCPropertyTypeOptionalInt16:
        {
            integer_t t_value;
            integer_t *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeInt, &t_value);
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
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeUInt, &t_value);
                if (t_value > 65535)
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
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeUInt, &t_value);
            }
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uinteger_t*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
		case kMCPropertyTypeOptionalDouble:
		{
			double t_value;
			double *t_value_ptr;
			if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
				t_value_ptr = nil;
			else
			{
				t_value_ptr = &t_value;
				MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDouble, &t_value);
			}

			if (!ctxt . HasError())
				((void(*)(MCExecContext&, void *, double*))prop -> setter)(ctxt, mark, t_value_ptr);
		}
			break;

        case kMCPropertyTypeMixedOptionalString:
        case kMCPropertyTypeOptionalString:
        {
            MCAutoStringRef t_value;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_value));
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCStringRef))prop -> setter)(ctxt, mark, *t_value);
        }
            break;
            
        case kMCPropertyTypeOptionalPoint:
        {
            MCPoint t_value;
            MCPoint *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypePoint, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCPoint*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalRectangle:
        {
            MCRectangle t_value;
            MCRectangle *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeRectangle, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCRectangle*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeOptionalColor:
        {
            MCColor t_value;
            MCColor *t_value_ptr;
            if (p_value . type == kMCExecValueTypeValueRef && MCValueIsEmpty(p_value . valueref_value))
                t_value_ptr = nil;
            else
            {
                t_value_ptr = &t_value;
                MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeColor, &t_value);
            }
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, MCColor*))prop -> setter)(ctxt, mark, t_value_ptr);
        }
            break;
            
        case kMCPropertyTypeMixedItemsOfString:
        case kMCPropertyTypeLinesOfString:
        case kMCPropertyTypeItemsOfString:
        {
            MCAutoStringRef t_input;
            MCStringRef *t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            
            char_t t_delimiter;
            t_delimiter = prop -> type == kMCPropertyTypeLinesOfString ? '\n' : ',';
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input));
            
            if (!MCPropertyParseStringList(*t_input, t_delimiter, t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, MCStringRef*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            for(uindex_t i = 0; i < t_count; i++)
                MCValueRelease(t_value[i]);
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            if (t_count > 0)
                MCMemoryDeleteArray(t_value);
        }
            break;
                      
        case kMCPropertyTypeLinesOfLooseUInt:
        case kMCPropertyTypeItemsOfLooseUInt:
        // AL-2014-09-24: [[ Bug 13529 ]] Handle mixed items of uint case
        case kMCPropertyTypeMixedItemsOfLooseUInt:
        {
            MCAutoStringRef t_input;
            uinteger_t* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            
            char_t t_delimiter;
            t_delimiter = prop -> type == kMCPropertyTypeLinesOfLooseUInt ? '\n' : ',';
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input));
            if (!MCPropertyParseLooseUIntList(*t_input, t_delimiter, t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, uinteger_t*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            if (t_count > 0)
                MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeLinesOfLooseDouble:
        {
            MCAutoStringRef t_input;
            double* t_value;
            uindex_t t_count;
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            t_count = 0;
            
            char_t t_delimiter;
            t_delimiter = prop -> type == kMCPropertyTypeLinesOfLooseDouble ? '\n' : ',';
            
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input));
            if (!MCPropertyParseLooseDoubleList(*t_input, t_delimiter, t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAN);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, double*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            // SN-2014-07-25: [[ Bug 12945 ]] Make sure not to deallocate un-allocated memory
            if (t_count > 0)
                MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeLinesOfPoint:
        {
            MCAutoStringRef t_input;
            MCPoint *t_value;
            uindex_t t_count = 0;
        
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input));
            
            if (!MCPropertyParsePointList(*t_input, '\n', t_count, t_value))
                ctxt . LegacyThrow(EE_PROPERTY_NAS);
            
            if (!ctxt . HasError())
                ((void(*)(MCExecContext&, void *, uindex_t, MCPoint*))prop -> setter)(ctxt, mark, t_count, t_value);
            
            if (t_count > 0)
                MCMemoryDeleteArray(t_value);
        }
            break;
            
        case kMCPropertyTypeRecord:
        {
            ((void(*)(MCExecContext&, void *, MCExecValue))prop -> setter)(ctxt, mark, p_value);
        }
            break;
            
        case kMCPropertyTypeLegacyPoints:
        {
            MCAutoStringRef t_input;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_input));
            if (!ctxt . HasError())
            {
                MCPoint *t_value;
                uindex_t t_count;
                t_value = nil;
                t_count = 0;
                MCU_parsepoints(t_value, t_count, *t_input);
                
                ((void(*)(MCExecContext&, void *, uindex_t, MCPoint*))prop -> setter)(ctxt, mark, t_count, t_value);
                
                MCMemoryDeleteArray(t_value);
            }
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
            
        case kMCExecValueTypeFloat:
			if (!MCNumberCreateWithReal(*(float *)p_from_value, (MCNumberRef&)r_value))
				ctxt . Throw();
			break;
            
        case kMCExecValueTypeColor:
            if (!MCU_format_color(*(MCColor *)p_from_value, (MCStringRef &)r_value))
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
        case kMCExecValueTypeNone:
            r_value = MCValueRetain(kMCNull);
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

static void MCExecTypeAssign(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, void *p_to_value)
{
    switch (p_from_type)
    {
        case kMCExecValueTypeNone:
            *(MCNullRef *)p_to_value = kMCNull;
            break;
        case kMCExecValueTypeValueRef:
            *(MCValueRef *)p_to_value = *(MCValueRef *)p_from_value;
            break;
        case kMCExecValueTypeBooleanRef:
            *(MCBooleanRef *)p_to_value = *(MCBooleanRef *)p_from_value;
            break;
        case kMCExecValueTypeStringRef:
            *(MCStringRef *)p_to_value = *(MCStringRef *)p_from_value;
            break;
        case kMCExecValueTypeNameRef:
            *(MCNameRef *)p_to_value = *(MCNameRef *)p_from_value;
            break;
        case kMCExecValueTypeDataRef:
            *(MCDataRef *)p_to_value = *(MCDataRef *)p_from_value;
            break;
        case kMCExecValueTypeArrayRef:
            *(MCArrayRef *)p_to_value = *(MCArrayRef *)p_from_value;
            break;
        case kMCExecValueTypeNumberRef:
            *(MCNumberRef *)p_to_value = *(MCNumberRef *)p_from_value;
            break;
        case kMCExecValueTypeUInt:
            *(uinteger_t *)p_to_value =  *(uinteger_t *)p_from_value;
            break;
        case kMCExecValueTypeInt:
            *(integer_t *)p_to_value =  *(integer_t *)p_from_value;
            break;
        case kMCExecValueTypeBool:
            *(bool *)p_to_value =  *(bool *)p_from_value;
            break;
        case kMCExecValueTypeDouble:
            *(double *)p_to_value =  *(double *)p_from_value;
            break;
        case kMCExecValueTypeFloat:
            *(float *)p_to_value =  *(float *)p_from_value;
            break;
        case kMCExecValueTypeChar:
            *(char *)p_to_value =  *(char *)p_from_value;
            break;
        case kMCExecValueTypePoint:
            *(MCPoint *)p_to_value =  *(MCPoint *)p_from_value;
            break;
        case kMCExecValueTypeColor:
            *(MCColor *)p_to_value =  *(MCColor *)p_from_value;
            break;
        case kMCExecValueTypeRectangle:
            *(MCRectangle *)p_to_value =  *(MCRectangle *)p_from_value;
            break;
    }
}

void MCExecTypeConvertNumbers(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, MCExecValueType p_to_type, void *p_to_value)
{
    if (p_from_type == kMCExecValueTypeDouble)
    {
        double t_from = *(double*)p_from_value;
        
        if (p_to_type == kMCExecValueTypeInt)
            *(integer_t*)p_to_value = (integer_t)(t_from < 0.0 ? t_from - 0.5 : t_from + 0.5);
        else if (p_to_type == kMCExecValueTypeUInt)
            *(uinteger_t*)p_to_value = (uinteger_t)(t_from < 0.0 ? 0 : t_from + 0.5);
        else if (p_to_type == kMCExecValueTypeFloat)
            *(float*)p_to_value = (float)t_from;
        else
            ctxt . Throw();
    }
    else if (p_from_type == kMCExecValueTypeUInt)
    {
        uinteger_t t_from = *(uinteger_t*)p_from_value;
        
        if (p_to_type == kMCExecValueTypeDouble)
            *(double*)p_to_value = (double)t_from;
        else if (p_to_type == kMCExecValueTypeInt)
            *(integer_t*)p_to_value = (integer_t)t_from;
        else if (p_to_type == kMCExecValueTypeFloat)
            *(float*)p_to_value = (float)t_from;
        else
            ctxt . Throw();
    }
    else if (p_from_type == kMCExecValueTypeInt)
    {
        integer_t t_from = *(integer_t*)p_from_value;
        
        if (p_to_type == kMCExecValueTypeDouble)
            *(double*)p_to_value = (double)t_from;
        else if (p_to_type == kMCExecValueTypeUInt)
            *(uinteger_t*)p_to_value = (uinteger_t)t_from;
        else if (p_to_type == kMCExecValueTypeFloat)
            *(float*)p_to_value = (float)t_from;
        else
            ctxt . Throw();
    }
    else if (p_from_type == kMCExecValueTypeFloat)
    {
        float t_from = *(float*)p_from_value;
        
        if (p_to_type == kMCExecValueTypeDouble)
            *(double*)p_to_value = (double)t_from;
        else if (p_to_type == kMCExecValueTypeInt)
            *(integer_t*)p_to_value = (integer_t)(t_from < 0.0 ? t_from - 0.5 : t_from + 0.5);
        else if (p_to_type == kMCExecValueTypeUInt)
            *(uinteger_t*)p_to_value = (uinteger_t)(t_from < 0.0 ? 0 : t_from + 0.5);
        else
            ctxt . Throw();
    }
    else
        ctxt . Throw();
}

void MCExecTypeConvertStringToNumber(MCExecContext &ctxt, MCStringRef p_from, MCExecValueType p_to_type, void* r_to_value)
{
    bool t_success;
    double t_real_value;
    if (MCStringGetNumericValue(p_from, t_real_value))
    {
        t_success = true;
        switch (p_to_type)
        {
        case kMCExecValueTypeDouble:
            *(double*) r_to_value = t_real_value;
            break;
        case kMCExecValueTypeInt:
            *(integer_t*) r_to_value = (integer_t)t_real_value;
            break;
        case kMCExecValueTypeUInt:
            *(uinteger_t*) r_to_value = (uinteger_t)t_real_value;
            break;
        case kMCExecValueTypeFloat:
            *(float*) r_to_value = (float)t_real_value;
            break;
        default:
            t_success = false;
        }
    }
    else
    {
        switch (p_to_type)
        {
        case kMCExecValueTypeInt:
        {
            integer_t t_value;
            t_success = ctxt . ConvertToInteger((MCValueRef)p_from, t_value);
            if (t_success)
            {
                MCStringSetNumericValue(p_from, (double)t_value);
                *(integer_t*)r_to_value = t_value;
            }
            break;
        }
        case kMCExecValueTypeUInt:
        {
            uinteger_t t_value;
            t_success =  ctxt . ConvertToUnsignedInteger((MCValueRef)p_from, t_value);
            if (t_success)
            {
                MCStringSetNumericValue(p_from, (double)t_value);
                *(uinteger_t*)r_to_value = t_value;
            }
            break;
        }
        case kMCExecValueTypeFloat:
        {
            double t_value;
            t_success = ctxt . ConvertToReal((MCValueRef)p_from, t_value);
            if (t_success)
            {
                MCStringSetNumericValue(p_from, (double)t_value);
                *(float*) r_to_value = (float)t_value;
            }
            break;
        }
        case kMCExecValueTypeDouble:
        {
            double t_value;
            t_success = ctxt . ConvertToReal((MCValueRef)p_from, t_value);
            if (t_success)
            {
                MCStringSetNumericValue(p_from, t_value);
                *(double*)r_to_value = t_value;
            }
            break;
        }
        default:
            t_success = false;
        }
    }

    if (!t_success)
        ctxt . Throw();
}

void MCExecTypeConvertAndReleaseAlways(MCExecContext& ctxt, MCExecValueType p_from_type, void *p_from_value, MCExecValueType p_to_type, void *p_to_value)
{
    if (p_from_type == p_to_type)
    {
        MCExecTypeAssign(ctxt, p_from_type, p_from_value, p_to_value);
        return;
    }
    else if(MCExecTypeIsNumber(p_to_type))
    {
        if (MCExecTypeIsNumber(p_from_type))
        {
            MCExecTypeConvertNumbers(ctxt, p_from_type, p_from_value, p_to_type, p_to_value);
            return;
        }
        else if (p_from_type == kMCExecValueTypeStringRef)
        {
            MCExecTypeConvertStringToNumber(ctxt, *(MCStringRef*)p_from_value, p_to_type, p_to_value);
            MCValueRelease(*(MCStringRef*)p_from_value);
            return;
        }
        else if (p_from_type == kMCExecValueTypeNameRef)
        {
            MCExecTypeConvertStringToNumber(ctxt, MCNameGetString(*(MCNameRef*)p_from_value), p_to_type, p_to_value);
            MCValueRelease(*(MCNameRef*)p_from_value);
            return;
        }
    }

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
    case kMCExecValueTypeNone:
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
        break;
    }
    self . type = kMCExecValueTypeNone;
    self . valueref_value = nil;
}

void MCExecTypeCopy(const MCExecValue &self, MCExecValue &r_dest)
{
    // Retain the value if one is stored
    if (MCExecTypeIsValueRef(self . type)
            || (self . type == kMCExecValueTypeNone && self . valueref_value != nil))
        MCValueCopy(self . valueref_value, r_dest . valueref_value);
    else
        r_dest = self;
    
    r_dest . type = self . type;
}

// Set a valueref in the exec value, taking in account its type
void MCExecTypeSetValueRef(MCExecValue &self, MCValueRef p_value)
{
    MCExecValueType t_type;
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeArray:
            t_type = kMCExecValueTypeArrayRef;
            break;
        case kMCValueTypeCodeData:
            t_type = kMCExecValueTypeDataRef;
            break;
        case kMCValueTypeCodeString:
            t_type = kMCExecValueTypeStringRef;
            break;
        case kMCValueTypeCodeBoolean:
            t_type = kMCExecValueTypeBooleanRef;
            break;
        case kMCValueTypeCodeNumber:
            t_type = kMCExecValueTypeNumberRef;
            break;
        case kMCValueTypeCodeName:
            t_type = kMCExecValueTypeNameRef;
            break;
        case kMCValueTypeCodeList:
        case kMCValueTypeCodeSet:
            t_type = kMCExecValueTypeValueRef;
            break;
        case kMCValueTypeCodeNull:
            t_type = kMCExecValueTypeNone;
            break;
        default:
            t_type = kMCExecValueTypeValueRef;
            break;
            
    }
    self . type = t_type;
    self . valueref_value = p_value;
}

bool MCExecTypeIsValueRef(MCExecValueType p_type)
{
    return p_type > kMCExecValueTypeNone && p_type < kMCExecValueTypeUInt;
}

bool MCExecTypeIsNumber(MCExecValueType p_type)
{
    return p_type > kMCExecValueTypeNumberRef && p_type < kMCExecValueTypeChar && p_type != kMCExecValueTypeBool;
}


// SN-2014-09-02: [[ Bug 13314 ]] Resolving the chars of a field should also take in account the changes brought
// to the marked text (chiefly being chunk delimiters added).
void MCExecResolveCharsOfField(MCExecContext& ctxt, MCField *p_field, uint32_t p_part, MCMarkedText p_mark, int32_t& r_start, int32_t& r_finish)
{
    r_start = p_mark . start;
    r_finish = p_mark . finish;
    
    // MCMarkedText::changed is only accessed by Interface / Variable-setting function.
    // Putting the replacement of the field content should not interleave with another
    if (p_mark.changed != 0)
    {
        MCAutoStringRef t_mark_as_string;
        if (!ctxt . ConvertToString(p_mark . text, &t_mark_as_string))
            ctxt . Throw();
        else
        {
            // We only want to append the forced delimiters added, not to reset the whole field's string
            // which leads to a loss of all the blocks.
            MCAutoStringRef t_forced_delimiters;
            /* UNCHECKED */ MCStringCopySubstring(*t_mark_as_string, MCRangeMake(MCStringGetLength(*t_mark_as_string) - p_mark . changed, p_mark . changed), &t_forced_delimiters);
            
            // INT32_MAX is PARAGRAPH_MAX_LEN (asking settextindex to append to the field)
            p_field -> settextindex(p_part, INT32_MAX, INT32_MAX, *t_forced_delimiters, false);
        }
    }
    
    /*
    findex_t t_start = x_start;
    findex_t t_finish = x_finish;
    p_field -> resolvechars(p_part, t_start, t_finish, p_start, p_count);
    x_start = t_start;
    x_finish = t_finish; */
}

void MCExecParseSet(MCExecContext& ctxt, MCExecSetTypeInfo *p_info, MCExecValue p_value, intset_t& r_value)
{
    // AL-2014-04-02: [[ Bug 12097 ]] No need to use c-string array (and indeed delete an unallocated one)
    MCAutoStringRef t_string;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_string));
    
    intset_t t_value = 0;
    MCAutoArrayRef t_split_strings;
    
    MCStringSplit(*t_string, MCSTR(","), nil, kMCStringOptionCompareExact, &t_split_strings);
    
    for (uindex_t i = 0; i < MCArrayGetCount(*t_split_strings); i++)
    {
        for (uindex_t j = 0; j < p_info -> count; j++)
        {
            MCValueRef t_current_string;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_split_strings, i + 1, t_current_string);
            if (MCStringIsEqualToCString((MCStringRef)t_current_string, p_info -> elements[j] . tag, kMCCompareExact))
            {
                t_value |= 1 << p_info -> elements[j] . bit;
                break;
            }
        }
    }
    
    r_value = t_value;
}

void MCExecParseEnum(MCExecContext& ctxt, MCExecEnumTypeInfo *p_info, MCExecValue p_value, intenum_t& r_value)
{
    MCAutoStringRef t_string;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_string));
    if (!ctxt . HasError())
    {    
        for(uindex_t i = 0; i < p_info -> count; i++)
            if (!p_info -> elements[i] . read_only &&
                MCStringIsEqualTo(*t_string, MCSTR(p_info -> elements[i] . tag), kMCStringOptionCompareCaseless))
            {
                r_value = p_info -> elements[i] . value;
                return;
            }
        
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
    for(uindex_t i = 0; i < p_info -> count; i++)
        if (p_info -> elements[i] . value == p_value)
        {
            MCStringCreateWithCString(p_info -> elements[i] . tag, r_value . stringref_value);
            r_value . type = kMCExecValueTypeStringRef;
            return;
        }

    // GETTING HERE MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
    MCAssert(false);
    return;
}

////////////////////////////////////////////////////////////////////////////////

