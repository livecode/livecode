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


#include "param.h"
#include "scriptpt.h"
#include "chunk.h"
#include "handler.h"
#include "license.h"
#include "util.h"
#include "mcerror.h"
#include "osspec.h"
#include "globals.h"
#include "object.h"
#include "mccontrol.h"
#include "notify.h"
#include "stack.h"
#include "card.h"
#include "eventqueue.h"
#include "debug.h"

#include "external.h"
#include "externalv1.h"

////////////////////////////////////////////////////////////////////////////////

// This static variable is set to the currently executing external during
// any calls to its exported functions (startup, shutdown etc.). It is used by
// the license fail API to mark the external's was_licensed member as false.
static MCExternalV1 *s_current_external = nil;

////////////////////////////////////////////////////////////////////////////////

static bool number_to_string(double p_number, MCExternalValueOptions p_options, MCStringRef &r_buffer)
{
    bool t_success;
    t_success = false;
	switch(p_options & kMCExternalValueOptionNumberFormatMask)
	{
		case kMCExternalValueOptionDefaultNumberFormat:
            t_success = MCU_r8tos(p_number, MCECptr -> GetNumberFormatWidth(), MCECptr -> GetNumberFormatTrailing(), MCECptr -> GetNumberFormatForce(), r_buffer);
			break;
		case kMCExternalValueOptionDecimalNumberFormat:
			t_success = MCStringFormat(r_buffer, "%f", p_number);
			break;
		case kMCExternalValueOptionScientificNumberFormat:
			t_success = MCStringFormat(r_buffer, "%e", p_number);
			break;
		case kMCExternalValueOptionCompactNumberFormat:
			t_success = MCStringFormat(r_buffer, "%0.16g", p_number);
			break;
	}
    
    return t_success;
}

static bool options_get_convert_octals(MCExternalValueOptions p_options)
{
	switch(p_options & kMCExternalValueOptionConvertOctalsMask)
	{
		case kMCExternalValueOptionDefaultConvertOctals:
			return MCECptr -> GetConvertOctals();
		case kMCExternalValueOptionConvertOctals:
			return true;
		case kMCExternalValueOptionDoNotConvertOctals:
			return false;
		default:
			break;
	}
	return false;
}

static MCExternalError string_to_boolean(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	if (MCStringIsEqualTo(p_string, kMCTrueString, kMCStringOptionCompareCaseless))
		*(bool *)r_value = true;
	else if (MCStringIsEqualTo(p_string, kMCFalseString, kMCStringOptionCompareCaseless))
		*(bool *)r_value = false;
	else
		return kMCExternalErrorNotABoolean;
	
	return kMCExternalErrorNone;
}

static MCExternalError string_to_integer(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
	uint32_t l;
    MCAutoPointer<char> t_chars;
    
	/* UNCHECKED */ MCStringConvertToNative(p_string, (char_t*&)&t_chars, l);
    s = (const char*)*t_chars;
    
	// Skip any whitespace before the number.
	MCU_skip_spaces(s, l);
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Check to see if we have a sign.
	bool t_negative;
	t_negative = false;
	if (*s == '-' || *s == '+')
	{
		t_negative = (*s == '-');
		s++;
		l--;
	}
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Check to see what base we are using.
	uint32_t t_base;
	t_base = 10;
	if (*s == '0')
	{
		if (l >= 2 && (s[1] == 'X' || s[1] == 'x'))
		{
			t_base = 16;
			s += 2;
			l -= 2;
		}
		else if (options_get_convert_octals(p_options))
		{
			t_base = 8;
			s += 1;
			l -= 1;
		}
	}
	
	uint32_t t_value;
	t_value = 0;
	
	bool t_is_integer;
	t_is_integer = true;
	
	if (t_base == 10)
	{
		uint32_t sl;
		sl = l;
		while(l != 0 && isdigit(*s))
		{
			uint32_t t_new_value;
			t_new_value = t_value * 10 + (*s - '0');
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
			
			s += 1;
			l -= 1;
		}
		
		if (l != 0 && *s == '.')
		{
			if (sl <= 1)
				return kMCExternalErrorNotANumber;
			
			do
			{
				s += 1;
				l -= 1;
				if (l != 0 && *s != '0')
					t_is_integer = false;
			}
			while(l != 0 && isdigit(*s));
		}
	}
	else if (t_base == 16)
	{
		while(l != 0 && isxdigit(*s))
		{
			uint32_t t_new_value;
			if (isdigit(*s))
				t_new_value = t_value * 16 + (*s - '0');
			else
				t_new_value = t_value * 16 + (((*s) & ~32) - 'A');
			
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
			
			s += 1;
			l -= 1;
		}
	}
	else
	{
		while(l != 0 && isdigit(*s) && *s < '8')
		{
			uint32_t t_new_value;
			t_new_value = t_value * 8 + (*s - '0');
			if (t_new_value < t_value)
				return kMCExternalErrorNumericOverflow;
			
			t_value = t_new_value;
		}
	}
	
	MCU_skip_spaces(s, l);
	if (l != 0)
		return kMCExternalErrorNotANumber;
	
	if (!t_is_integer)
		return kMCExternalErrorNotAnInteger;
	
	if ((p_options & 0xf) == kMCExternalValueOptionAsInteger)
	{
		int32_t t_as_value;
		if (t_negative)
		{
			if (t_value > 0x80000000U)
				return kMCExternalErrorNumericOverflow;
			t_as_value = -(int32_t)t_value;
		}
		else
		{
			if (t_value > MAXINT4)
				return kMCExternalErrorNumericOverflow;
			t_as_value = t_value;
		}
		*(int32_t *)r_value = t_as_value;
	}
	else
	{
		if (t_negative)
			return kMCExternalErrorNumericOverflow;
		*(uint32_t *)r_value = t_value;
	}
	
	return kMCExternalErrorNone;
}

static MCExternalError string_to_real(MCStringRef p_string, MCExternalValueOptions p_options, void *r_value)
{
	const char *s;
    MCAutoPointer<char> t_chars;
	uint32_t l;
    uint32_t t_length;
    
	/* UNCHECKED */ MCStringConvertToNative(p_string, (char_t*&)&t_chars, l);
    t_length = l;
    s = (const char*)*t_chars;
	
	// Skip space before the number.
	MCU_skip_spaces(s, l);
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// See if the number is negative.
	bool t_negative;
	t_negative = false;
	if (*s == '-' || *s == '+')
	{
		t_negative = (*s == '-');
		s++;
		l--;
	}
	if (l == 0)
		return kMCExternalErrorNotANumber;
	
	// Now see if it has to be interpreted as an integer (0x or 0 prefix).
	if (*s == '0' &&
		((l >= 2 && ((s[1] == 'X') || (s[1] == 'x'))) ||
		options_get_convert_octals(p_options)))
	{
		MCExternalError t_error;
        MCAutoStringRef t_substring;
        /* UNCHECKED */ MCStringCopySubstring(p_string, MCRangeMake(t_length - l, l), &t_substring);
		uint32_t t_value;
		t_error = string_to_integer(*t_substring, (p_options & ~0xf) | kMCExternalValueOptionAsCardinal, &t_value);
		if (t_error != kMCExternalErrorNone)
			return t_error;
		
		*(double *)r_value = !t_negative ? (double)t_value : -(double)t_value;
		return kMCExternalErrorNone;
	}
	
	// Otherwise we convert as a double - note that we need a NUL terminated
	// string here so temporarily copy into a buffer...
	char t_tmp_s[R8L];
	uint32_t t_tmp_l;
	t_tmp_l = MCU_min(R8L - 1U, l);
	memcpy(t_tmp_s, s, t_tmp_l);
	t_tmp_s[t_tmp_l] = '\0';
	
	double t_value;
	char *t_tmp_end;
	t_value = strtod(t_tmp_s, &t_tmp_end);
	
	s += t_tmp_end - t_tmp_s;
	l -= t_tmp_end - t_tmp_s;
	MCU_skip_spaces(s, l);
	
	if (l != 0)
		return kMCExternalErrorNotANumber;
	
	*(double *)r_value = !t_negative ? t_value : -t_value;
	
	return kMCExternalErrorNone;
}

static MCExternalError number_to_integer(double p_number, MCExternalValueOptions p_options, void *r_value)
{
	bool t_negative;
	if (p_number >= 0.0)
		t_negative = false;
	else
	{
		t_negative = true;
		p_number = -p_number;
	}
	
	double t_integer, t_fraction;
	t_fraction = modf(p_number, &t_integer);
	
	uint32_t t_value;
	if (t_fraction < MC_EPSILON)
		t_value = (uint32_t)t_integer;
	else if ((1.0 - t_fraction) < MC_EPSILON)
		t_value = (uint32_t)t_integer + 1;
	else
		return kMCExternalErrorNotAnInteger;
	
	if ((p_options & 0xf) == kMCExternalValueOptionAsInteger)
	{
		int32_t t_as_value;
		if (t_negative)
		{
			if (t_value > 0x80000000U)
				return kMCExternalErrorNumericOverflow;
			t_as_value = -(int32_t)t_value;
		}
		else
		{
			if (t_value > MAXINT4)
				return kMCExternalErrorNumericOverflow;
			t_as_value = t_value;
		}
		*(int32_t *)r_value = t_as_value;
	}
	else
	{
		if (t_negative)
			return kMCExternalErrorNumericOverflow;
		*(uint32_t *)r_value = t_value;
	}
	
	return kMCExternalErrorNone;
}

static MCExternalError number_to_real(double p_number, MCExternalValueOptions p_options, void *r_value)
{
	*(double *)r_value = p_number;
	return kMCExternalErrorNone;
}

#ifdef __HAS_CORE_FOUNDATION__

#include <CoreFoundation/CoreFoundation.h>

typedef const void* id;
extern void MCCFAutorelease(id);

// MW-2013-06-14: [[ ExternalsApiV5 ]] New methods to convert to/from objc-arrays
//   and dictionaries.
// SN-2015-01-14: [[ Bug 14057 ]] Update the prototype to use MCArrays directly
extern "C" MCExternalError MCArrayFromObjcArray(CFArrayRef p_src, MCArrayRef& r_array);
extern "C" MCExternalError MCArrayToObjcArray(MCArrayRef p_src, CFArrayRef& r_array);
extern "C" MCExternalError MCArrayFromObjcDictionary(CFDictionaryRef p_src, MCArrayRef &r_array);
extern "C" MCExternalError MCArrayToObjcDictionary(MCArrayRef p_src, CFDictionaryRef& r_dictionary);

// SN-2015-01-14: [[ Bug 14057 ]] Added conversion functions
extern "C" MCExternalError MCValueFromObjcValue(id src, MCValueRef &var);
extern "C" MCExternalError MCValueToObjcValue(MCValueRef p_input, id& r_dst);

#endif

////////////////////////////////////////////////////////////////////////////////

MCExternalVariable::MCExternalVariable(void)
{
	m_references = 1;
	m_string_conversion = nil;
}

MCExternalVariable::~MCExternalVariable(void)
{
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
}

uint32_t MCExternalVariable::GetReferenceCount(void)
{
	return m_references;
}

void MCExternalVariable::Retain(void)
{
	m_references += 1;
}

void MCExternalVariable::Release(void)
{
	m_references -= 1;
	if (m_references > 0)
		return;
	
	delete this;
}

Value_format MCExternalVariable::GetFormat(void)
{
	switch(MCValueGetTypeCode(GetValueRef()))
	{
		case kMCValueTypeCodeNull:
			return VF_STRING;
		case kMCValueTypeCodeBoolean:
			return VF_STRING;
		case kMCValueTypeCodeNumber:
			return VF_NUMBER;
		case kMCValueTypeCodeName:
			return VF_STRING;
		case kMCValueTypeCodeString:
			return VF_STRING;
		case kMCValueTypeCodeData:
			return VF_STRING;
		case kMCValueTypeCodeArray:
			return VF_ARRAY;
		default:
			assert(false);
	}
	
	return VF_UNDEFINED;
}

MCExternalError MCExternalVariable::Set(MCExternalVariable *p_other)
{
	SetValueRef(p_other -> GetValueRef());
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetBoolean(bool p_value)
{
	SetValueRef(p_value ? kMCTrue : kMCFalse);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetInteger(int32_t p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithInteger(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetCardinal(uint32_t p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithUnsignedInteger(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetReal(double p_value)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithReal(p_value, &t_number))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_number);
	return kMCExternalErrorNone;
}

// SN-2014-07-01 [[ ExternalsApiV6 ]] Update to use a stringRef
MCExternalError MCExternalVariable::SetString(MCStringRef p_value)
{
	SetValueRef(p_value);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::SetCString(const char *p_value)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithCString(p_value, &t_string))
		return kMCExternalErrorOutOfMemory;
	SetValueRef(*t_string);
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::Append(MCExternalValueOptions p_options, MCExternalVariable *p_value)
{
	MCExternalError t_error;
	MCAutoStringRef t_string;
	t_error = p_value -> GetString(p_options, &t_string);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendBoolean(MCExternalValueOptions p_options, bool p_value)
{
	return AppendString(p_options, p_value ? kMCTrueString : kMCFalseString);
}

MCExternalError MCExternalVariable::AppendInteger(MCExternalValueOptions p_options, int32_t p_value)
{
    MCAutoStringRef t_string;
    
    if (!MCStringFormat(&t_string, "%d", p_value))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendCardinal(MCExternalValueOptions p_options, uint32_t p_value)
{
    MCAutoStringRef t_string;
	if (!MCStringFormat(&t_string, "%u", p_value))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendReal(MCExternalValueOptions p_options, real64_t p_value)
{
    MCAutoStringRef t_string;
	if (!number_to_string(p_value, p_options, &t_string))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::AppendString(MCExternalValueOptions p_options, MCStringRef p_value)
{
	MCExternalError t_error;
	MCAutoStringRef t_current_value;
	t_error = GetString(p_options, &t_current_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	
	MCAutoStringRef t_new_value;
    if (!MCStringCreateWithStrings(&t_new_value, *t_current_value, p_value))
		return kMCExternalErrorOutOfMemory;
	
	SetValueRef(*t_new_value);	
	return t_error;
}

MCExternalError MCExternalVariable::AppendCString(MCExternalValueOptions p_options, const char *p_value)
{
    MCAutoStringRef t_string;
    if (!MCStringCreateWithCString(p_value, &t_string))
        return kMCExternalErrorOutOfMemory;
    
	return AppendString(p_options, *t_string);
}

MCExternalError MCExternalVariable::GetBoolean(MCExternalValueOptions p_options, bool& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
		r_value = t_value == kMCTrue;
	else
	{
		MCExternalError t_error;
		MCAutoStringRef t_string_value;
		t_error = GetString(p_options, &t_string_value);
		if (t_error != kMCExternalErrorNone)
			return t_error;
		
		return string_to_boolean(*t_string_value, p_options, &r_value);
	}
	
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::GetInteger(MCExternalValueOptions p_options, int32_t& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber)
		return number_to_integer(MCNumberFetchAsReal((MCNumberRef)t_value), p_options, &r_value);
	
	MCExternalError t_error;
	MCAutoStringRef t_string_value;
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
        return t_error;
	
	return string_to_integer(*t_string_value, p_options, &r_value);
}

MCExternalError MCExternalVariable::GetCardinal(MCExternalValueOptions p_options, uint32_t& r_value)
{
	return GetInteger(p_options, (int32_t&)r_value);
}

MCExternalError MCExternalVariable::GetReal(MCExternalValueOptions p_options, real64_t& r_value)
{
	MCValueRef t_value;
	t_value = GetValueRef();
	if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber)
		return number_to_real(MCNumberFetchAsReal((MCNumberRef)t_value), p_options, &r_value);
	
	MCExternalError t_error;
	MCAutoStringRef t_string_value;
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
	
	return string_to_real(*t_string_value, p_options, &r_value);
}

MCExternalError MCExternalVariable::GetString(MCExternalValueOptions p_options, MCStringRef& r_value)
{
    return ConvertToString(GetValueRef(), p_options, r_value);
}

MCExternalError MCExternalVariable::ConvertToString(MCValueRef p_value, MCExternalValueOptions p_options, MCStringRef &r_string)
{
    MCAutoStringRef t_string_value;
    
    // Avoid null pointer dereferences in externals
    if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeNull)
        p_value = kMCEmptyString;
    
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeBoolean:
            t_string_value = (p_value == kMCTrue ? kMCTrueString : kMCFalseString);
            break;
        case kMCValueTypeCodeNumber:
        {
            // Use the externalv1 method to convert.
            double t_number;
            t_number = MCNumberFetchAsReal((MCNumberRef)p_value);
            
            if (!number_to_string(t_number, p_options, &t_string_value))
                return kMCExternalErrorOutOfMemory;
        }
            break;
        case kMCValueTypeCodeName:
        {
            t_string_value = MCNameGetString((MCNameRef)p_value);
        }
            break;
        case kMCValueTypeCodeString:
        {
            t_string_value = (MCStringRef)p_value;
        }
            break;
        case kMCValueTypeCodeData:
        {
            MCDataRef t_dataref;
            t_dataref = (MCDataRef)p_value;
            if (!MCStringCreateWithBytes(MCDataGetBytePtr(t_dataref), MCDataGetLength(t_dataref), kMCStringEncodingNative, false, &t_string_value))
                return kMCExternalErrorOutOfMemory;
        }
            break;
        case kMCValueTypeCodeArray:
            // An array is never a string (from the point of view of the externals API).
            return kMCExternalErrorNotAString;
        default:
            assert(false);
    }
    
    r_string = MCValueRetain(*t_string_value);
    
    return kMCExternalErrorNone;
}
// SN-2014-07-16: [[ ExternalsApiV6 ]] Function to get the CData type - allowing nil bytes in the string
MCExternalError MCExternalVariable::GetCData(MCExternalValueOptions p_options, void *r_value)
{
	MCAutoStringRef t_string_value;
	MCExternalError t_error;
    MCString t_string;
    uindex_t t_length;
    
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
    
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
	
    if (!MCStringNormalizeAndConvertToNative(*t_string_value, (char_t*&)m_string_conversion, t_length))
        return kMCExternalErrorOutOfMemory;
	
	t_string . set(m_string_conversion, t_length);
    *(MCString*)r_value = t_string;
	return kMCExternalErrorNone;
}

MCExternalError MCExternalVariable::GetCString(MCExternalValueOptions p_options, const char*& r_value)
{
	MCAutoStringRef t_string_value;
	MCExternalError t_error;
    uindex_t t_length;
    
	t_error = GetString(p_options, &t_string_value);
	if (t_error != kMCExternalErrorNone)
		return t_error;
    
    if (m_string_conversion != nil)
        MCMemoryDeleteArray(m_string_conversion);
	
    if (!MCStringNormalizeAndConvertToNative(*t_string_value, (char_t*&)m_string_conversion, t_length))
        return kMCExternalErrorOutOfMemory;
	
	if (memchr(m_string_conversion, '\0', t_length) != nil)
    {
        MCMemoryDeleteArray(m_string_conversion);
        m_string_conversion = nil;
		return kMCExternalErrorNotACString;
    }
	
	r_value = m_string_conversion;
	return kMCExternalErrorNone;
}

//////////

MCTransientExternalVariable::MCTransientExternalVariable(MCValueRef p_value)
{
	m_value = MCValueRetain(p_value);
}

MCTransientExternalVariable::~MCTransientExternalVariable(void)
{
	MCValueRelease(m_value);
}

bool MCTransientExternalVariable::IsTemporary(void)
{
	return false;
}

bool MCTransientExternalVariable::IsTransient(void)
{
	return true;
}

MCValueRef MCTransientExternalVariable::GetValueRef(void)
{
	return m_value;
}

void MCTransientExternalVariable::SetValueRef(MCValueRef p_value)
{
	MCValueRetain(p_value);
	MCValueRelease(m_value);
	m_value = p_value;
}

//////////

MCTemporaryExternalVariable::MCTemporaryExternalVariable(MCValueRef p_value)
	: MCTransientExternalVariable(p_value)
{
}

bool MCTemporaryExternalVariable::IsTemporary(void)
{
	return true;
}

//////////

MCReferenceExternalVariable::MCReferenceExternalVariable(MCContainer& p_container)
    : m_container(p_container)
{
}

MCReferenceExternalVariable::~MCReferenceExternalVariable(void)
{
}

bool MCReferenceExternalVariable::IsTemporary(void)
{
	return false;
}

bool MCReferenceExternalVariable::IsTransient(void)
{
	return false;
}

MCValueRef MCReferenceExternalVariable::GetValueRef(void)
{
	return m_container.get_valueref();
}

void MCReferenceExternalVariable::SetValueRef(MCValueRef p_value)
{
	m_container.set_valueref(p_value);
}

////////////////////////////////////////////////////////////////////////////////

MCExternalV1::MCExternalV1(void)
	: m_info(nil),
	  m_licensed(false),
	  m_was_licensed(false)
{
}

MCExternalV1::~MCExternalV1(void)
{
}

bool MCExternalV1::Prepare(void)
{
	// Fetch the description callback - this symbol has to exist since a V1
	// external is only created if it does!
	MCExternalDescribeProc t_describe;
	t_describe = (MCExternalDescribeProc)MCU_library_lookup(*m_module,
                                                            MCSTR("MCExternalDescribe"));
	
	// Update the current external var.
	s_current_external = this;
	
	// Query the info record - if this returns nil something odd is going on!
	m_info = t_describe();
	
	// Unset the current external var.
	s_current_external = nil;
    
	if (m_info == nil)
		return false;

    return true;
}

bool MCExternalV1::Initialize(void)
{
	// Fetch the initialize entry point.
	MCExternalInitializeProc t_initialize;
	t_initialize = (MCExternalInitializeProc)MCU_library_lookup(*m_module,
                                                                MCSTR("MCExternalInitialize"));
	if (t_initialize == nil)
		return true;
	
	// Make sure the 'was_licensed' instance var is true. A call to LicenseFail
	// during startup will set it to false.
	m_was_licensed = true;
	
	// Update the current external var.
	s_current_external = this;
	
	// See if initialization succeeds.
	bool t_success;
	t_success = t_initialize(&g_external_interface);
	
	// Unset the current external var.
	s_current_external = nil;
	
	if (!t_success)
		return false;

    // If license fail was invoked during startup, we mark the whole external
    // as unlicensed which means all calls to it will throw an error.
    m_licensed = m_was_licensed;
    
	return true;
}

void MCExternalV1::Finalize(void)
{
	// Fetch the finalize entry point.
	MCExternalFinalizeProc t_finalize;
	t_finalize = (MCExternalFinalizeProc)MCU_library_lookup(*m_module,
                                                            MCSTR("MCExternalFinalize"));
	if (t_finalize == nil)
		return;
	
	// Update the current external var.
	s_current_external = this;
	
	t_finalize();
	
	// Unset the current external var.
	s_current_external = nil;
}

const char *MCExternalV1::GetName(void) const
{
	return m_info -> name;
}

Handler_type MCExternalV1::GetHandlerType(uint32_t p_index) const
{
	if (m_info -> handlers[p_index] . type == kMCExternalHandlerTypeCommand)
		return HT_MESSAGE;
	return HT_FUNCTION;
		}

bool MCExternalV1::ListHandlers(MCExternalListHandlersCallback p_callback, void *p_state)
{
	for(uint32_t i = 0; m_info -> handlers[i] . type != kMCExternalHandlerTypeNone; i++)
		if (!p_callback(p_state, m_info -> handlers[i] . type == kMCExternalHandlerTypeCommand ? HT_MESSAGE : HT_FUNCTION, m_info -> handlers[i] . name, i))
			return false;

	return true;
}

Exec_stat MCExternalV1::Handle(MCObject *p_context, Handler_type p_type, uint32_t p_index, MCParameter *p_parameters)
{
	MCExternalHandlerType t_type;
	if (p_type == HT_FUNCTION)
		t_type = kMCExternalHandlerTypeFunction;
		else
		t_type = kMCExternalHandlerTypeCommand;

	MCExternalHandler *t_handler;
	t_handler = &m_info -> handlers[p_index];
	if (t_handler -> type != t_type)
		return ES_NOT_HANDLED;
	
	// If the external is not licensed (as a whole) then we throw the unlicensed error.
	if (!m_licensed)
	{
		MCAutoStringRef t_name;
		if (!MCStringCreateWithCString(m_info -> name, &t_name))
			return ES_ERROR;
		MCeerror -> add(EE_EXTERNAL_UNLICENSED, 0, 0, *t_name);
		return ES_ERROR;
	}
	
	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	// Count the number of parameters.
	uint32_t t_parameter_count;
	t_parameter_count = 0;
	for(MCParameter *t_param = p_parameters; t_param != nil; t_param = t_param -> getnext())
		t_parameter_count += 1;
		
	// Allocate an array of values.
	MCExternalVariableRef *t_parameter_vars;
	t_parameter_vars = NULL;
	if (t_parameter_count != 0)
	{
		t_parameter_vars = new (nothrow) MCExternalVariableRef[t_parameter_count];
		if (t_parameter_vars == nil)
			return ES_ERROR;
	}
	
	// Now iterate through the parameters, fetching the parameter values as we go.
	for(uint32_t i = 0; p_parameters != nil && t_stat == ES_NORMAL; i++, p_parameters = p_parameters -> getnext())
    {
        MCContainer *t_container
                = p_parameters -> eval_argument_container();
        if (t_container != nil)
        {
            t_parameter_vars[i] = new (nothrow) MCReferenceExternalVariable(*t_container);
        }
        else
        {
            t_parameter_vars[i] = new (nothrow) MCTransientExternalVariable(p_parameters->getvalueref_argument());
        }
	}

	// We have our list of parameters (hopefully), so now call - passing a temporary
	// result.
	if (t_stat == ES_NORMAL)
	{
		// MW-2014-01-22: [[ CompatV1 ]] Initialize a temporary var to empty.
		MCTransientExternalVariable t_result(kMCEmptyString);
		
		// MW-2014-01-22: [[ CompatV1 ]] Make a reference var to hold it (should it be needed).
		//   We then store the previous global value of the it extvar, and set this one.
		//   As external calls are recursive, this should be fine :)
        MCContainer t_it_container;
        MCECptr->GetIt()->evalcontainer(*MCECptr, t_it_container);
		MCReferenceExternalVariable t_it(t_it_container);
        
		MCReferenceExternalVariable *t_old_it;
		t_old_it = s_external_v1_current_it;
		s_external_v1_current_it = &t_it;
		
		// CW-2016-06-21: [[ Bug 17891 ]] Make sure the 'was_licensed' instance var is true,
		//   to avoid detecting a previous license check failure.
		m_was_licensed = true;
		
		// Update the current external var.
		s_current_external = this;
		
		// Invoke the external handler. If 'false' is returned, treat the result as a
		// string value containing an error hint.
		bool t_success;
		t_success = (t_handler -> handler)(t_parameter_vars, t_parameter_count, &t_result);
		
		// Unset the current external var.
		s_current_external = nil;
		
		// If a license check failed during the call, then we always throw an error.
		if (!m_was_licensed)
		{
			MCAutoStringRef t_name;
			if (!MCStringCreateWithCString(m_info -> name, &t_name))
				return ES_ERROR;
			MCeerror -> add(EE_EXTERNAL_UNLICENSED, 0, 0, *t_name);
			t_stat = ES_ERROR;
		}
		else if (t_success)
		{
			MCresult -> setvalueref(t_result . GetValueRef());
		}
		else
		{
			MCeerror -> add(EE_EXTERNAL_EXCEPTION, 0, 0, t_result . GetValueRef());
			t_stat = ES_ERROR;
		}
		
		// Restore the old it.
		s_external_v1_current_it = t_old_it;
	}

	// Finally, loop through and free the parameters as necessary.
	for(uint32_t i = 0; i < t_parameter_count; i++)
		delete t_parameter_vars[i];

	// SN-2015-03-25: [[ CID 15424 ]] Delete an array: use delete []
	delete[] t_parameter_vars;

	return t_stat;
}

void MCExternalV1::SetWasLicensed(bool p_value)
{
	m_was_licensed = p_value;
}

////////////////////////////////////////////////////////////////////////////////

MCExternal *MCExternalCreateV1(void)
{
	return new MCExternalV1;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_SUBPLATFORM_IPHONE)
extern bool iphone_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
	if (!iphone_run_on_main_thread(p_callback, p_callback_state, p_options))
		return kMCExternalErrorNotImplemented;
	return kMCExternalErrorNone;
}
#elif defined(TARGET_SUBPLATFORM_ANDROID)
extern bool android_run_on_main_thread(void *p_callback, void *p_callback_state, int p_options);
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
	if (!android_run_on_main_thread(p_callback, p_callback_state, p_options))
		return kMCExternalErrorNotImplemented;
	return kMCExternalErrorNone;
}
#else
static MCExternalError MCExternalEngineRunOnMainThread(void *p_callback, void *p_callback_state, MCExternalRunOnMainThreadOptions p_options)
{
    // MW-2014-10-30: [[ Bug 13875 ]] If either 'JumpTo' flag is specified, we just execute the callback direct.
    if ((p_options & kMCExternalRunOnMainThreadJumpTo) != 0)
    {
        // The 'JumpTo' option cannot have any other flags set.
        if ((p_options & ~kMCExternalRunOnMainThreadJumpTo) != 0)
            return kMCExternalErrorNotImplemented;
        
        ((MCExternalThreadOptionalCallback)p_callback)(p_callback_state);
        return kMCExternalErrorNone;
    }
    
	// MW-2013-06-25: [[ DesktopPingWait ]] Pass the correct parameters through
	//   to MCNotifyPush so that LCObjectPost works.
    // MW-2014-10-23: [[ Bug 13721 ]] Correctly compute the notify flags to pass - in particular
    //   compute the 'required' flag and pass that as that determines the signature of the
    //   callback.
    bool t_block, t_safe, t_required;
    t_block = (p_options & kMCExternalRunOnMainThreadPost) == kMCExternalRunOnMainThreadSend;
    t_safe = (p_options & kMCExternalRunOnMainThreadUnsafe) == kMCExternalRunOnMainThreadSafe;
    t_required = (p_options & kMCExternalRunOnMainThreadRequired) == kMCExternalRunOnMainThreadRequired;
    
    // MW-2014-10-30: [[ Bug 13875 ]] Make sure we return an appropriate error for invalid combinations of flags.
    if (t_block && t_safe)
        return kMCExternalErrorNotImplemented;
    
	if (!MCNotifyPush((MCExternalThreadOptionalCallback)p_callback, p_callback_state, t_block, t_safe, t_required))
		return kMCExternalErrorOutOfMemory;

	return kMCExternalErrorNone;
}
#endif

////////////////////////////////////////////////////////////////////////////////

// SN-2015-01-26: [[ Bug 14057 ]] Function added to have a consistent conversion from a string.
static MCExternalError MCExternalConvertStringToValueType(MCStringRef p_string, MCExternalValueOptions p_option, void *r_result)
{
    MCString t_string;
    
    // Any of the MCStringConvertTo* generate NULL-terminated strings.
    switch(p_option & 0xff)
    {
        case kMCExternalValueOptionAsCChar:
            *(char*)r_result = MCStringGetNativeCharAtIndex(p_string, 0);
            break;
            
        case kMCExternalValueOptionAsCString:
        case kMCExternalValueOptionAsString:
        {
            char *t_cstring;
            if (!MCStringConvertToCString(p_string, t_cstring))
                return kMCExternalErrorOutOfMemory;
            
            if ((p_option & 0xff) == kMCExternalValueOptionAsCString)
                *(char**)r_result = t_cstring;
            else
                ((MCString*) r_result) -> setstring(t_cstring);
            break;
        }
            
        case kMCExternalValueOptionAsUTF16CString:
        case kMCExternalValueOptionAsUTF16String:
        {
            unichar_t *t_chars;
            uint32_t t_count;
            if (!MCStringConvertToUnicode(p_string, t_chars, t_count))
                return kMCExternalErrorOutOfMemory;
            
            if ((p_option & 0xff) == kMCExternalValueOptionAsUTF16CString)
                *(unichar_t**)r_result = t_chars;
            else
                ((MCString*)r_result) -> set ((char*)t_chars, t_count);
            
            break;
        }
            
        case kMCExternalValueOptionAsUTF8CString:
        case kMCExternalValueOptionAsUTF8String:
        {
            char *t_chars;
            uint32_t t_count;
            if (!MCStringConvertToUTF8(p_string, t_chars, t_count))
                return kMCExternalErrorOutOfMemory;
            
            if ((p_option & 0xff) == kMCExternalValueOptionAsUTF8CString)
                *(char**)r_result = t_chars;
            else
                ((MCString*)r_result) -> set(t_chars, t_count);
            
            break;
        }
            
#ifdef __OBJC__
        case kMCExternalValueOptionAsNSString:
        {
            CFStringRef t_string;
            if (!MCStringConvertToCFStringRef(p_string, t_string))
                return kMCExternalErrorOutOfMemory;
            
            *(NSString**)r_result = (NSString*)t_string;
        }
#endif
        default:
            return kMCExternalErrorInvalidValueType;
    }
    
    return kMCExternalErrorNone;
}

// SN-2015-01-26: [[ Bug 14057 ]] New context query function, to allow the users to choose their type for the delimiters
static MCExternalError MCExternalContextQuery(MCExternalContextQueryTag op, MCExternalValueOptions p_option, void *result)
{
    switch(op)
    {
        case kMCExternalContextQueryMe:
        {
            MCObjectHandle t_handle;
            t_handle = MCECptr -> GetObject() -> GetHandle();
            if (!t_handle)
                return kMCExternalErrorOutOfMemory;
            
            *(static_cast<MCExternalObjectRef*>(result)) = t_handle.ExternalRetain();
        }
            break;
        case kMCExternalContextQueryTarget:
        {
            if (!MCtargetptr)
                return kMCExternalErrorNoObject;
            
            *(static_cast<MCExternalObjectRef*>(result)) = MCtargetptr.ExternalRetain();
        }
            break;
        case kMCExternalContextQueryResult:
            // MW-2014-01-22: [[ CompatV1 ]] If the result shim hasn't been made, make it.
            if (s_external_v1_result == nil)
            {
                s_external_v1_result_container = new (nothrow) MCContainer(MCresult);
                s_external_v1_result = new (nothrow) MCReferenceExternalVariable(*s_external_v1_result_container);
            }
            *(MCExternalVariableRef *)result = s_external_v1_result;
            break;
        case kMCExternalContextQueryIt:
        {
            // MW-2014-01-22: [[ CompatV1 ]] Use the current it shim (initialized before
            //   a new handler is invoked).
            *(MCExternalVariableRef *)result = s_external_v1_current_it;
        }
            break;
        case kMCExternalContextQueryCaseSensitive:
            *(bool *)result = MCECptr -> GetCaseSensitive();
            break;
        case kMCExternalContextQueryConvertOctals:
            *(bool *)result = MCECptr -> GetConvertOctals();
            break;
            // MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of 'the wholeMatches' query.
        case kMCExternalContextQueryWholeMatches:
            *(bool *)result = MCECptr -> GetWholeMatches();
            break;
        case kMCExternalContextQueryDefaultStack:
        {
            if (!MCdefaultstackptr)
                return kMCExternalErrorNoDefaultStack;
            
            MCObjectHandle t_handle;
            t_handle = MCdefaultstackptr -> GetHandleAs<MCObject>();
            if (!t_handle)
                return kMCExternalErrorOutOfMemory;
            
            *(static_cast<MCExternalObjectRef*>(result)) = t_handle.ExternalRetain();
        }
            break;
        case kMCExternalContextQueryDefaultCard:
        {
            if (!MCdefaultstackptr)
                return kMCExternalErrorNoDefaultStack;
            
            MCObjectHandle t_handle;
            t_handle = MCdefaultstackptr -> getcurcard() -> GetHandleAs<MCObject>();
            if (!t_handle)
                return kMCExternalErrorOutOfMemory;
            
            *(static_cast<MCExternalObjectRef*>(result)) = t_handle.ExternalRetain();
        }
            break;
            
            // SN-2015-01-26: [[ Bug 14057 ]] Delimiters query can now return more types than a C-char
        case kMCExternalContextQueryItemDelimiter:
            return MCExternalConvertStringToValueType(MCECptr -> GetItemDelimiter(), p_option, result);
        case kMCExternalContextQueryLineDelimiter:
            return MCExternalConvertStringToValueType(MCECptr -> GetLineDelimiter(), p_option, result);
        case kMCExternalContextQueryColumnDelimiter:
            return MCExternalConvertStringToValueType(MCECptr -> GetColumnDelimiter(), p_option, result);
        case kMCExternalContextQueryRowDelimiter:
            return MCExternalConvertStringToValueType(MCECptr -> GetRowDelimiter(), p_option, result);
			
		case kMCExternalContextQueryHasLicenseCheck:
			*(bool *)result = true;
			break;
			
        default:
            return kMCExternalErrorInvalidContextQuery;
    }
    return kMCExternalErrorNone;
}

// SN-2015-01-26: [[ Bug 14057 ]] ContextQueryLegacy does only apply specific, fixed treatment for the delimiters
static MCExternalError MCExternalContextQueryLegacy(MCExternalContextQueryTag op, void *result)
{
    switch(op)
    {
        case kMCExternalContextQueryItemDelimiter:
            *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetItemDelimiter(), 0);
            break;
        case kMCExternalContextQueryLineDelimiter:
            *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetLineDelimiter(), 0);
            break;
        case kMCExternalContextQueryColumnDelimiter:
            *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetColumnDelimiter(), 0);
            break;
        case kMCExternalContextQueryRowDelimiter:
            *(char *)result = MCStringGetNativeCharAtIndex(MCECptr -> GetRowDelimiter(), 0);
            break;
            // Back up to the legacy context query if we are something which can be a string.
        default:
            return MCExternalContextQuery(op, 0, result);
    }
    return kMCExternalErrorNone;
}

// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of context_evaluate method.
MCExternalError MCExternalContextEvaluate(const char *p_expression, unsigned int p_options, MCExternalVariableRef *p_binds, unsigned int p_bind_count, MCExternalVariableRef p_result)
{
    MCAutoStringRef t_expr;
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_expression now evaluated as UTF-8
	if (!MCStringCreateWithBytes((byte_t*)p_expression, strlen(p_expression), kMCStringEncodingUTF8, false, &t_expr))
		return kMCExternalErrorOutOfMemory;
	
	MCAutoValueRef t_value;
    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::eval refactored
    MCECptr -> eval(*MCECptr, *t_expr, &t_value);
	if (MCECptr -> HasError())
	{	
		if (MCECptr -> GetExecStat() == ES_ERROR)
			return kMCExternalErrorFailed;
	
		if (MCECptr -> GetExecStat() == ES_EXIT_ALL)
			return kMCExternalErrorExited;
	}
	
	p_result -> SetValueRef(*t_value);
	
	return kMCExternalErrorNone;
}

// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of context_execute method.
MCExternalError MCExternalContextExecute(const char *p_commands, unsigned int p_options, MCExternalVariableRef *p_binds, unsigned int p_bind_count)
{
	MCAutoStringRef t_expr;
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_commands now evaluated as UTF-8
	if (!MCStringCreateWithBytes((byte_t*)p_commands, strlen(p_commands), kMCStringEncodingUTF8, false, &t_expr))
		return kMCExternalErrorOutOfMemory;
	
    MCECptr -> doscript(*MCECptr, *t_expr, 0, 0);
	if (MCECptr -> HasError())
	{	
		if (MCECptr -> GetExecStat() == ES_ERROR)
			return kMCExternalErrorFailed;
		
		if (MCECptr -> GetExecStat() == ES_EXIT_ALL)
			return kMCExternalErrorExited;
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalVariableCreate(MCExternalVariableRef* r_var)
{
    // SN-2015-03-25: [[ CID 16536 ]] Check that we have a variable.
    if (r_var == NULL)
        return kMCExternalErrorNoVariable;

	*r_var = new (nothrow) MCTemporaryExternalVariable(kMCEmptyString);
    // SN-2015-06-02: [[ CID 90609 ]] Check that the pointed value has been allocated
	if (*r_var == nil)
		return kMCExternalErrorOutOfMemory;

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRetain(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> Retain();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableRelease(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> Release();
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableQuery(MCExternalVariableRef var, MCExternalVariableQueryTag p_query, void *r_result)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;
	
	switch(p_query)
	{
	case kMCExternalVariableQueryIsTemporary:
		*(bool *)r_result = var -> IsTemporary();
		break;
	case kMCExternalVariableQueryIsTransient:
		*(bool *)r_result = var -> IsTransient();
		break;
	case kMCExternalVariableQueryFormat:
		*(Value_format *)r_result = var -> GetFormat();
		break;
	case kMCExternalVariableQueryRetention:
		if (!var -> IsTemporary())
			*(uint32_t *)r_result = 0;
		else
			*(uint32_t *)r_result = var -> GetReferenceCount();
		break;
	case kMCExternalVariableQueryIsAnArray:
		*(bool *)r_result = MCValueIsArray(var -> GetValueRef()) || MCValueIsEmpty(var -> GetValueRef());
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsEmpty variable query.
	case kMCExternalVariableQueryIsEmpty:
		*(bool *)r_result = MCValueIsEmpty(var -> GetValueRef());
		break;
			
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of IsASequence variable query.
	case kMCExternalVariableQueryIsASequence:
			*(bool *)r_result = MCValueIsEmpty(var -> GetValueRef()) || (MCValueIsArray(var -> GetValueRef()) && MCArrayIsSequence((MCArrayRef)var -> GetValueRef()));
		break;

	default:
		return kMCExternalErrorInvalidVariableQuery;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableClear(MCExternalVariableRef var)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	var -> SetValueRef(kMCEmptyString);

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableExchange(MCExternalVariableRef var_a, MCExternalVariableRef var_b)
{
	if (var_a == nil || var_b == nil)
		return kMCExternalErrorNoVariable;

	MCValueRef t_a_value;
	t_a_value = MCValueRetain(var_a -> GetValueRef());
    // SN-2014-11-24: [[ Bug 14057 ]] Actually process the exchange
	var_a -> SetValueRef(var_b -> GetValueRef());
	var_b -> SetValueRef(t_a_value);
	MCValueRelease(t_a_value);

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableStore(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;
    
#ifdef __HAS_CORE_FOUNDATION__
    MCExternalError t_error;
#endif

	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return var -> Set((MCExternalVariableRef)p_value);
	case kMCExternalValueOptionAsBoolean:
		return var -> SetBoolean(*(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> SetInteger(*(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> SetCardinal(*(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
		return var -> SetReal(*(real64_t *)p_value);
	case kMCExternalValueOptionAsString:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((const byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingNative, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsCString:
        return var -> SetCString(*(const char **)p_value);
        
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Storing the new types
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;

        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        
        if (!MCStringCreateWithBytes(*(byte_t**)p_value, strlen(*(char**)p_value), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), 2 * t_string->getlength(), kMCStringEncodingUTF16, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        uint16_t *t_chars;
        uindex_t t_char_count;
        
        t_chars = *(uint16_t**)p_value;

        for (t_char_count = 0 ; *t_chars != 0; ++t_char_count)
            ++t_chars;
        
        if (!MCStringCreateWithChars(*(const unichar_t**)p_value, t_char_count, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> SetString(*t_stringref);
    }
    // SN-2015-01-19: [[ Bug 14057 ]] Added forgotten C-char value type, used to
    //   get the pre-7.0, non-Unicode, lone-character delimiters
    case kMCExternalValueOptionAsCChar:
    {
        char_t t_chars;
        MCAutoStringRef t_stringref;
            
        t_chars = *(char_t*)p_value;
            
        if (!MCStringCreateWithNativeChars(&t_chars, 1, &t_stringref))
            return kMCExternalErrorOutOfMemory;
            
        return var -> SetString(*t_stringref);
    }
            
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsNSNumber:
    {
        MCAutoValueRef t_value;
        
        t_error = MCValueFromObjcValue(*(CFNumberRef*)p_value, &t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        return var -> SetReal(MCNumberFetchAsReal((MCNumberRef)*t_value));
    }
    case kMCExternalValueOptionAsNSString:
    {
        MCAutoValueRef t_value;
        
        t_error = MCValueFromObjcValue(*(CFStringRef*)p_value, &t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        return var -> SetString((MCStringRef)*t_value);
    }
    case kMCExternalValueOptionAsNSData:
    {
        MCAutoValueRef t_value;
        
        t_error = MCValueFromObjcValue(*(CFDataRef*)p_value, &t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        return var -> SetString((MCStringRef)*t_value);
    }
    case kMCExternalValueOptionAsNSArray:
    {
        // For efficiency, we use 'exchange' - this prevents copying a temporary array.
        MCExternalVariableRef t_tmp_array;
        MCAutoArrayRef t_tmp_array_ref;
        
        t_error = kMCExternalErrorNone;
        t_tmp_array = nil;
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalVariableCreate(&t_tmp_array);
        // SN-2015-01-14: [[ Bug 14057 ]] Use the new engine function
        if (t_error == kMCExternalErrorNone)
            t_error = MCArrayFromObjcArray(*(CFArrayRef*)p_value, &t_tmp_array_ref);
        if (t_error == kMCExternalErrorNone)
        {
            t_tmp_array -> SetValueRef(*t_tmp_array_ref);
            t_error = MCExternalVariableExchange(var, t_tmp_array);
        }
        if (t_tmp_array != nil)
            MCExternalVariableRelease(t_tmp_array);
        
        return t_error;
    }
    case kMCExternalValueOptionAsNSDictionary:
    {
        // For efficiency, we use 'exchange' - this prevents copying a temporary array.
        MCExternalVariableRef t_tmp_array;
        MCAutoArrayRef t_tmp_array_ref;
        
        t_error = kMCExternalErrorNone;
        t_tmp_array = nil;
        if (t_error == kMCExternalErrorNone)
            t_error = MCExternalVariableCreate(&t_tmp_array);
        // SN-2015-01-14: [[ Bug 14057 ]] Use the new engine function
        if (t_error == kMCExternalErrorNone)
            t_error = MCArrayFromObjcDictionary(*(CFDictionaryRef*)p_value, &t_tmp_array_ref);
        if (t_error == kMCExternalErrorNone)
        {
            t_tmp_array -> SetValueRef(*t_tmp_array_ref);
            t_error = MCExternalVariableExchange(var, t_tmp_array);
        }
        if (t_tmp_array != nil)
            MCExternalVariableRelease(t_tmp_array);
        return t_error;
    }
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableFetch(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;
    
    MCExternalError t_error;
    
	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return ((MCExternalVariableRef)p_value) -> Set(var);
	case kMCExternalValueOptionAsBoolean:
		return var -> GetBoolean(p_options, *(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> GetInteger(p_options, *(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> GetCardinal(p_options, *(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
        return var -> GetReal(p_options, *(real64_t *)p_value);
    case kMCExternalValueOptionAsString:
        return var -> GetCData(p_options, p_value);
    case kMCExternalValueOptionAsCString:
        return var -> GetCString(p_options, *(const char **)p_value);
        
    // SN-2014-07-01: [[ ExternalsApiV6 ]] Fetching the new types
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        uindex_t t_length;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUTF8(*t_stringref, t_chars, t_length))
            return kMCExternalErrorOutOfMemory;
        
        ((MCString*)p_value) -> set(t_chars, t_length);
        break;
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUTF8String(*t_stringref, t_chars))
            return kMCExternalErrorOutOfMemory;
        
        (*(char**)p_value) = t_chars;
        break;
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUnicode(*t_stringref, t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        ((MCString*)p_value) -> set((char*)t_chars, t_char_count);
        break;
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToUnicode(*t_stringref, t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        (*(unichar_t**)p_value) = t_chars;
        break;
    }
    // SN-2015-01-19: [[ Bug 14057 ]] Added forgotten C-char value type
    case kMCExternalValueOptionAsCChar:
    {
        MCAutoStringRef t_stringref;
            
        t_error = var -> GetString(p_options, &t_stringref);
            
        if (t_error != kMCExternalErrorNone)
            return t_error;
            
        (*(char_t*)p_value) = MCStringGetNativeCharAtIndex(*t_stringref, 0);
        break;
    }
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsNSNumber:
    case kMCExternalValueOptionAsCFNumber:
    {
        real64_t t_real;
        
        t_error = var -> GetReal(p_options, t_real);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        *(CFNumberRef*)p_value = CFNumberCreate(NULL, kCFNumberFloat64Type, &t_real);
        
        // SN-2014-11-24: [[ Bug 14057 ]] NS types are allocated in an autrelease pool
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSNumber)
            MCCFAutorelease(*(CFNumberRef*)p_value);
        
        break;
    }
    case kMCExternalValueOptionAsNSString:
    case kMCExternalValueOptionAsCFString:
    {
        MCAutoStringRef t_stringref;
        
        t_error = var -> GetString(p_options, &t_stringref);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToCFStringRef(*t_stringref, *(CFStringRef*)p_value))
            return kMCExternalErrorOutOfMemory;
        
        // SN-2014-11-24: [[ Bug 14057 ]] NS types are allocated in an autrelease pool
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSString)
            MCCFAutorelease(*(CFStringRef*)p_value);
        break;
    }
    case kMCExternalValueOptionAsNSData:
    case kMCExternalValueOptionAsCFData:
    {
        MCAutoStringRef t_stringref;
        char *t_chars;
        uindex_t t_char_count;
        
        t_error = var -> GetString(p_options, &t_stringref);
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToNative(*t_stringref, (char_t*&)t_chars, t_char_count))
            return kMCExternalErrorOutOfMemory;
        
        *(CFDataRef*)p_value = CFDataCreateWithBytesNoCopy(NULL, (UInt8*)t_chars, t_char_count, NULL);
        
        // SN-2014-11-24: [[ Bug 14057 ]] NS types are allocated in an autrelease pool
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSData)
            MCCFAutorelease(*(CFDataRef*)p_value);
        break;
    }
    case kMCExternalValueOptionAsNSArray:
    case kMCExternalValueOptionAsCFArray:
    {
        CFArrayRef t_value;
        
        t_error = kMCExternalErrorNone;
        
        MCValueRef t_valueref;
        t_valueref = var -> GetValueRef();
        if (MCValueGetTypeCode(t_valueref) != kMCValueTypeCodeArray)
            t_error = kMCExternalErrorNotAnArray;
        
        if (t_error == kMCExternalErrorNone)
            t_error = MCArrayToObjcArray((MCArrayRef)var -> GetValueRef(), t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        // SN-2014-11-24: [[ Bug 14057 ]] Sets the return value.
        *(CFArrayRef*)p_value = t_value;
        
        // SN-2014-11-24: [[ Bug 14057 ]] NS types are allocated in an autrelease pool
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSArray)
            MCCFAutorelease(*(CFArrayRef*)p_value);
        return t_error;
    }
    case kMCExternalValueOptionAsNSDictionary:
    case kMCExternalValueOptionAsCFDictionary:
    {
        CFDictionaryRef t_value;
        
        t_error = kMCExternalErrorNone;
        
        MCValueRef t_valueref;
        t_valueref = var -> GetValueRef();
        
        if (MCValueGetTypeCode(t_valueref) != kMCValueTypeCodeArray)
            t_error = kMCExternalErrorNotAnArray;
        
        if (t_error == kMCExternalErrorNone)
            t_error = MCArrayToObjcDictionary((MCArrayRef)var -> GetValueRef(), t_value);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        // SN-2014-11-24: [[ Bug 14057 ]] Sets the return value.
        *(CFDictionaryRef*)p_value = t_value;
        
        // SN-2014-11-24: [[ Bug 14057 ]] NS types are allocated in an autrelease pool
        if ((p_options & 0xff) == kMCExternalValueOptionAsNSDictionary)
            MCCFAutorelease(*(CFDictionaryRef*)p_value);
        
        return t_error;
    }
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableAppend(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_value)
{
	if (var == nil)
		return kMCExternalErrorNoVariable;

	if (p_value == nil)
		return kMCExternalErrorNoValue;

	switch(p_options & 0xff)
	{
	case kMCExternalValueOptionAsVariable:
		return var -> Append(p_options, (MCExternalVariableRef)p_value);
	case kMCExternalValueOptionAsBoolean:
		return var -> AppendBoolean(p_options, *(bool *)p_value);
	case kMCExternalValueOptionAsInteger:
		return var -> AppendInteger(p_options, *(int32_t *)p_value);
	case kMCExternalValueOptionAsCardinal:
		return var -> AppendCardinal(p_options, *(uint32_t *)p_value);
	case kMCExternalValueOptionAsReal:
		return var -> AppendReal(p_options, *(real64_t *)p_value);
	case kMCExternalValueOptionAsString:
        {
            MCAutoStringRef t_stringref;
            MCString* t_string;
            t_string = (MCString*)p_value;
            if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingNative, false, &t_stringref))
                return kMCExternalErrorOutOfMemory;
            
            return var -> AppendString(p_options, *t_stringref);
        }
	case kMCExternalValueOptionAsCString:
        return var -> AppendCString(p_options, *(const char **)p_value);

    // SN-2014-07-01: [[ ExternalsApiV6 ]] Appending new types (same conversion as in MCExternalVariableStore)
    case kMCExternalValueOptionAsUTF8String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), t_string->getlength(), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF8CString:
    {
        MCAutoStringRef t_stringref;
        
        if (!MCStringCreateWithBytes(*(byte_t**)p_value, strlen(*(char**)p_value), kMCStringEncodingUTF8, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF16String:
    {
        MCAutoStringRef t_stringref;
        MCString* t_string;

        // SN-2015-03-25: [[ External Fix ]] Actually set t_string to the param
        t_string = (MCString*)p_value;
        if (!MCStringCreateWithBytes((byte_t*)t_string->getstring(), 2 * t_string->getlength(), kMCStringEncodingUTF16, false, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    case kMCExternalValueOptionAsUTF16CString:
    {
        MCAutoStringRef t_stringref;
        unichar_t *t_chars;
        uindex_t t_char_count;
        
        t_chars = *(unichar_t**)p_value;
        for (t_char_count = 0 ; *t_chars; ++t_char_count)
            ++t_chars;
        
        if (!MCStringCreateWithChars(*(const unichar_t**)p_value, t_char_count, &t_stringref))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_stringref);
    }
    // SN-2015-01-19: [[ Bug 14057 ]] Added forgotten C-char value type
    case kMCExternalValueOptionAsCChar:
    {
        MCAutoStringRef t_stringref;
        char_t t_char;
            
        t_char = *(char_t*) p_value;
            
        if (!MCStringCreateWithNativeChars(&t_char, 1, &t_stringref))
            return kMCExternalErrorOutOfMemory;
            
        return var -> AppendString(p_options, *t_stringref);
    }
#ifdef __HAS_CORE_FOUNDATION__
    case kMCExternalValueOptionAsNSNumber:
    {
        CFNumberRef t_number;
        t_number = *(CFNumberRef*)p_value;
        
        if (CFNumberIsFloatType(t_number))
        {
            real64_t t_double;
            if (!CFNumberGetValue(t_number, kCFNumberFloat64Type, &t_double))
                return kMCExternalErrorNotANumber;
            else
                return var -> AppendReal(p_options, t_double);
        }
        else
        {
            int32_t t_integer;
            if (!CFNumberGetValue(t_number, kCFNumberIntType, &t_integer))
                return kMCExternalErrorNotANumber;
            else
                return var -> AppendInteger(p_options, t_integer);
        }
    }
    case kMCExternalValueOptionAsNSString:
    {
        MCAutoStringRef t_string;
        
        if (!MCStringCreateWithCFStringRef(*(CFStringRef*)p_value, &t_string))
            return kMCExternalErrorNotAString;
        
        return var -> AppendString(p_options, *t_string);
    }
    case kMCExternalValueOptionAsNSData:
    {
        MCAutoStringRef t_string;
        CFDataRef t_data;
        t_data = *(CFDataRef*)p_value;
        
        if (!MCStringCreateWithBytes(CFDataGetBytePtr(t_data), CFDataGetLength(t_data), kMCStringEncodingNative, false, &t_string))
            return kMCExternalErrorOutOfMemory;
        
        return var -> AppendString(p_options, *t_string);
    }
    // SN-2014-07-01: [[ ExternalsApiV6 ]] CFArray and CFDictionary can't be appended.
    case kMCExternalValueOptionAsNSArray:
    case kMCExternalValueOptionAsNSDictionary:
#endif
	default:
		return kMCExternalErrorInvalidValueType;
	}

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariablePrepend(MCExternalVariableRef var, MCExternalValueOptions p_options, void *value)
{
	return kMCExternalErrorNotImplemented;
}
    
////////////////////////////////////////////////////////////////////////////////


// This method was never exposed.
static MCExternalError MCExternalVariableEdit(MCExternalVariableRef var, MCExternalValueOptions p_options, uint32_t p_required_length, void **r_buffer, uint32_t *r_length)
{
	return kMCExternalErrorNotImplemented;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalVariableCountKeys(MCExternalVariableRef var, uint32_t* r_count)
{
    // SN-2014-11-24: [[ Bug 14057 ]] Implement the function
    if (var == nil)
        return kMCExternalErrorNoVariable;
    
    MCValueRef t_value;
    t_value = var -> GetValueRef();
    
    if (t_value == nil)
        return kMCExternalErrorNoValue;
    
    if (MCValueIsArray(t_value))
        *r_count = MCArrayGetCount((MCArrayRef)t_value);
    else if (MCValueIsEmpty(t_value))
        *r_count = 0;
    else
        return kMCExternalErrorNotAnArray;
    
    return kMCExternalErrorNone;
}

static MCExternalError MCExternalVariableIterateKeys(MCExternalVariableRef var, MCExternalVariableIteratorRef *p_iterator, MCExternalValueOptions p_options, void *p_key, MCExternalVariableRef *r_value)
{
    
    // SN-2014-11-24: [[ Bug 14057 ]] Implement the function
    if (var == nil)
        return kMCExternalErrorNoVariable;
    
    if (p_iterator == nil)
        return kMCExternalErrorNoIterator;
    
    MCValueRef t_value;
    MCArrayRef t_array;
    t_value = var -> GetValueRef();
    
    // If the var is not an array, we set the iterator to nil to indicate
    // there are no elements.
    if (!MCValueIsArray(t_value))
    {
        *p_iterator = nil;
        return MCValueIsEmpty(t_value) ? kMCExternalErrorNone : kMCExternalErrorNotAnArray;
    }
    else
        t_array = (MCArrayRef)t_value;
    
    // If both key and value are nil, then the iteration is being cleaned up.
    if (p_key == nil && r_value == nil)
    {
        // We don't have anything to clean up at the moment...
        return kMCExternalErrorNone;
    }
    
    // SN-2015-02-26: [[ CID 37860 ]] Giving a NULL key should return an error
    if (p_key == nil)
        return kMCExternalErrorNoVariable;
    
    MCNameRef t_key;
    MCValueRef t_array_value;
    uintptr_t t_iterator = *(uintptr_t*)p_iterator;
    
    // If we have an entry, then extract the key in the form that was requested
    // and return its value.
    if (MCArrayIterate(t_array, t_iterator, t_key, t_array_value))
    {
        MCExternalVariableRef t_variable;
        if (MCExternalVariableCreate(&t_variable) != kMCExternalErrorNone)
            return kMCExternalErrorOutOfMemory;
        t_variable -> SetValueRef(t_array_value);
        
        *r_value = t_variable;
        *(uintptr_t*)p_iterator = t_iterator;
        
        MCStringRef t_key_as_string;
        t_key_as_string = MCNameGetString(t_key);
        char *t_key_as_cstring;
        
        switch(p_options & 0xff)
        {
            case kMCExternalValueOptionAsVariable:
                ((MCExternalVariableRef)p_key) -> SetValueRef(t_key);
                return kMCExternalErrorNone;
                
            case kMCExternalValueOptionAsBoolean:
                return string_to_boolean(t_key_as_string, p_options, p_key);
                
            case kMCExternalValueOptionAsInteger:
            case kMCExternalValueOptionAsCardinal:
                return string_to_integer(t_key_as_string, p_options, p_key);
                
            case kMCExternalValueOptionAsReal:
                return string_to_real(t_key_as_string, p_options, p_key);
                
            case kMCExternalValueOptionAsString:
                if (!MCStringConvertToCString(t_key_as_string, t_key_as_cstring))
                    return kMCExternalErrorOutOfMemory;
                *(MCString *)p_key = MCString(t_key_as_cstring, strlen(t_key_as_cstring));
                return kMCExternalErrorNone;
                
            case kMCExternalValueOptionAsCString:
                if (!MCStringConvertToCString(t_key_as_string, t_key_as_cstring))
                    return kMCExternalErrorOutOfMemory;
                *(char **)p_key = t_key_as_cstring;
                return kMCExternalErrorNone;
                
            case kMCExternalValueOptionAsUTF8CString:
                if (!MCStringConvertToUTF8String(t_key_as_string, t_key_as_cstring))
                    return kMCExternalErrorOutOfMemory;
                *(char**)p_key = t_key_as_cstring;
                return kMCExternalErrorNone;
                
            default:
                return kMCExternalErrorInvalidValueType;
        }
    }
    
    return kMCExternalErrorNone;
}

static bool MCExternalIsCaseSensitive(MCExternalValueOptions p_options)
{
	switch(intenum_t(p_options) & kMCExternalValueOptionCaseSensitiveMask)
    {
        case kMCExternalValueOptionDefaultCaseSensitive:
            return MCECptr -> GetCaseSensitive();
        case kMCExternalValueOptionCaseSensitive:
            return true;
        case kMCExternalValueOptionNotCaseSensitive:
            return false;
        default:
            return false;
    }
}

static MCExternalError MCExternalVariableRemoveKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key)
{
    
    // SN-2014-11-24: [[ Bug 14057 ]] Implement the function
    if (var == nil)
        return kMCExternalErrorNoVariable;
    
    MCValueRef t_value;
    t_value = var -> GetValueRef();
    
    if (t_value == nil)
        return kMCExternalErrorNoValue;
    
    MCArrayRef t_array;
    if (MCValueIsArray(t_value))
        t_array = (MCArrayRef)t_value;
    else
        return kMCExternalErrorNotAnArray;
    
    MCNewAutoNameRef t_name;
    MCStringEncoding t_encoding;
    // Only accept the CString keys or UTF8CString
    if ((p_options & 0xff) == kMCExternalValueOptionAsCString)
        t_encoding = kMCStringEncodingNative;
    else if ((p_options & 0xff) == kMCExternalValueOptionAsUTF8CString)
        t_encoding = kMCStringEncodingUTF8;
    else
        return kMCExternalErrorInvalidValueType;
    
    MCAutoStringRef t_key_as_string;
    if (!MCStringCreateWithBytes(*(byte_t**)p_key, strlen(*(char**)p_key), t_encoding, false, &t_key_as_string)
            || !MCNameCreate(*t_key_as_string, &t_name))
        return kMCExternalErrorOutOfMemory;
    
    if (MCArrayRemoveValue(t_array, MCExternalIsCaseSensitive(p_options), *t_name))
        return kMCExternalErrorNone;
    else
        return kMCExternalErrorFailed;
}

static MCExternalError MCExternalVariableLookupKey(MCExternalVariableRef var, MCExternalValueOptions p_options, void *p_key, bool p_ensure, MCExternalVariableRef *r_var)
{
    
    // SN-2014-11-24: [[ Bug 14057 ]] Implement the function
    if (var == nil)
        return kMCExternalErrorNoVariable;
    
    MCValueRef t_value;
    t_value = var -> GetValueRef();
    
    if (t_value == nil)
        return kMCExternalErrorNoValue;
    
    MCArrayRef t_fetched_array;
    if (!MCValueIsArray(t_value))
    {
        if (MCValueIsEmpty(t_value))
        {
            if (p_ensure)
            {
                // Create a new array for this external variable, since empty
                MCAutoArrayRef t_array;
                MCArrayCreateMutable(&t_array);
                var -> SetValueRef(*t_array);
                
                // Fetch the array (as it is done from if the value was an array)
                t_fetched_array = (MCArrayRef)var -> GetValueRef();
            }
            else
            {
                *r_var = nil;
                return kMCExternalErrorNone;
            }
        }
        else
            return kMCExternalErrorNotAnArray;
    }
    else
        t_fetched_array = (MCArrayRef)t_value;
    
    
    MCNewAutoNameRef t_name;
    MCStringEncoding t_encoding;
    // Only accept the CString keys or UTF8CString
    if ((p_options & 0xff) == kMCExternalValueOptionAsCString)
        t_encoding = kMCStringEncodingNative;
    else if ((p_options & 0xff) == kMCExternalValueOptionAsUTF8CString)
        t_encoding = kMCStringEncodingUTF8;
    else
        return kMCExternalErrorInvalidValueType;

    MCAutoStringRef t_key_as_string;
    if (!MCStringCreateWithBytes(*(byte_t**)p_key, strlen(*(char**)p_key), t_encoding, false, &t_key_as_string)
            || !MCNameCreate(*t_key_as_string, &t_name))
        return kMCExternalErrorOutOfMemory;
    
    MCValueRef t_fetched_value;
    MCExternalVariableRef t_var_ref;
    
    if (MCExternalVariableCreate(&t_var_ref) != kMCExternalErrorNone)
        return kMCExternalErrorOutOfMemory;
    
    if (MCArrayFetchValue(t_fetched_array, MCExternalIsCaseSensitive(p_options), *t_name, t_fetched_value))
        t_var_ref -> SetValueRef(t_fetched_value);
    else if (p_ensure)
    {
        // p_ensure forces the creation of an empty value for the key, in case the key didn't exist beforehand
        MCArrayStoreValue(t_fetched_array, MCExternalIsCaseSensitive(p_options), *t_name, kMCEmptyString);
        t_var_ref -> SetValueRef(kMCEmptyString);
    }
    else
    {
        t_var_ref -> Release();
        t_var_ref = nil;
    }
    
    *r_var = t_var_ref;
    
    return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalObjectResolve(const char *p_long_id, MCExternalObjectRef *r_handle)
{
	// If we haven't been given a long id, its an error.
	if (p_long_id == nil)
		return kMCExternalErrorNoObjectId;

	// If we haven't been given a result parameter, do nothing.
	if (r_handle == nil)
		return kMCExternalErrorNone;

	MCExternalError t_error;
	t_error = kMCExternalErrorNone;

	// MW-2014-01-22: [[ CompatV1 ]] Convert the long id to a stringref.
    // SN-2014-07-01: [[ ExternalsApiV6 ]] p_long_id now UTF8-encoded
	MCAutoStringRef t_long_id_ref;
	if (!MCStringCreateWithBytes((byte_t*)p_long_id, strlen(p_long_id), kMCStringEncodingUTF8, false, &t_long_id_ref))
		return kMCExternalErrorOutOfMemory;
	
	// Create a script point with the value are setting the property to
	// as source text.
	MCScriptPoint sp(*t_long_id_ref);

	// Create a new chunk object to parse the reference into
	MCChunk *t_chunk;
	t_chunk = new (nothrow) MCChunk(False);
	if (t_chunk == nil)
		t_error = kMCExternalErrorOutOfMemory;

	// Attempt to parse a chunk. We also check that there is no 'junk' at
	// the end of the string - if there is, its an error. Note the errorlock
	// here - it stops parse errors being pushed onto MCperror.
	Symbol_type t_next_type;
	MCerrorlock++;
	if (t_error == kMCExternalErrorNone)
		if (t_chunk -> parse(sp, False) != PS_NORMAL || sp.next(t_next_type) != PS_EOF)
			t_error = kMCExternalErrorMalformedObjectChunk;

	// Now attempt to evaluate the object reference - this will only succeed
	// if the object exists.
	MCExecContext ep2(*MCECptr);
	MCObject *t_object;
	uint32_t t_part_id;
	if (t_error == kMCExternalErrorNone)
		if (t_chunk -> getobj(ep2, t_object, t_part_id, False) != ES_NORMAL)
			t_error = kMCExternalErrorCouldNotResolveObject;

	MCerrorlock--;

	// If we found the object, attempt to create a handle.
	if (t_error == kMCExternalErrorNone)
	{
		MCObjectHandle t_handle;
		t_handle = t_object -> GetHandle();
		if (t_handle)
			*(MCExternalObjectRef *)r_handle = t_handle.ExternalRetain();
		else
			t_error = kMCExternalErrorOutOfMemory;
	}

	// Cleanup
	delete t_chunk;

	return t_error;
}

static MCExternalError MCExternalObjectExists(MCExternalObjectRef p_handle, bool *r_exists)
{
	MCObjectHandle t_handle = p_handle;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;

	*r_exists = t_handle.IsValid();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectRetain(MCExternalObjectRef p_handle)
{
	MCObjectHandle t_handle = p_handle;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;

	t_handle.ExternalRetain();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectRelease(MCExternalObjectRef p_handle)
{
	MCObjectHandle t_handle = p_handle;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;

	t_handle.ExternalRelease();

	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectDispatch(MCExternalObjectRef p_object, MCExternalDispatchType p_type, const char *p_message, MCExternalVariableRef *p_argv, uint32_t p_argc, MCExternalDispatchStatus *r_status)
{
	MCObjectHandle t_object = p_object;
    
    if (!t_object.IsBound())
		return kMCExternalErrorNoObject;

	if (p_message == nil)
		return kMCExternalErrorNoObjectMessage;

	if (p_argv == nil && p_argc > 0)
		return kMCExternalErrorNoObjectArguments;

	if (!t_object.IsValid())
		return kMCExternalErrorObjectDoesNotExist;

	MCExternalError t_error;
	t_error = kMCExternalErrorNone;

	MCParameter *t_params, *t_last_param;
	t_params = t_last_param = nil;
	for(uint32_t i = 0; i < p_argc; i++)
	{
		MCParameter *t_param;
		t_param = new (nothrow) MCParameter;
		t_param -> setvalueref_argument(p_argv[i] -> GetValueRef());

		if (t_last_param == nil)
			t_params = t_param;
		else
			t_last_param -> setnext(t_param);

		t_last_param = t_param;
	}

	MCNameRef t_message_as_name;
    MCAutoStringRef t_message_as_string;
	t_message_as_name = nil;
	if (t_error == kMCExternalErrorNone)
        // SN-2014-07-01: [[ ExternalsApiV6 ]] p_message is now UTF8-encoded
		if (!MCStringCreateWithBytes((byte_t*)p_message, strlen(p_message), kMCStringEncodingUTF8, false, &t_message_as_string)
                || !MCNameCreate(*t_message_as_string, t_message_as_name))
			t_error = kMCExternalErrorOutOfMemory;

	if (t_error == kMCExternalErrorNone)
	{
		Exec_stat t_stat;
		t_stat = t_object -> dispatch(p_type == kMCExternalDispatchCommand ? HT_MESSAGE : HT_FUNCTION, t_message_as_name, t_params);
		if (r_status != nil)
			switch(t_stat)
			{
			case ES_NORMAL:
				*r_status = MCexitall == False ? kMCExternalDispatchStatusHandled : kMCExternalDispatchStatusExit;
				break;
			case ES_NOT_HANDLED:
				*r_status = kMCExternalDispatchStatusNotHandled;
				break;
			case ES_PASS:
				*r_status = kMCExternalDispatchStatusPassed;
				break;
			case ES_ERROR:
				*r_status = kMCExternalDispatchStatusError;
				break;
			case ES_NEXT_REPEAT:
			case ES_EXIT_REPEAT:
			case ES_EXIT_HANDLER:
			case ES_EXIT_SWITCH:
			case ES_EXIT_ALL:
			case ES_RETURN_HANDLER:
			case ES_PASS_ALL:
			case ES_NOT_FOUND:
				MCUnreachable();
				break;
 			}
	}

	MCValueRelease(t_message_as_name);

	while(t_params != nil)
	{
		MCParameter *t_param;
		t_param = t_params;
		t_params = t_params -> getnext();
		delete t_param;
	}

	return kMCExternalErrorNone;
}

static Properties parse_property_name(MCStringRef p_name)
{
	MCScriptPoint t_sp(p_name);
	Symbol_type t_type;
	const LT *t_literal;
	if (t_sp . next(t_type) &&
		t_sp . lookup(SP_FACTOR, t_literal) == PS_NORMAL &&
		t_literal -> type == TT_PROPERTY &&
		t_sp . next(t_type) == PS_EOF)
		return (Properties)t_literal -> which;
	
	return P_CUSTOM;
}

// SN-2014-07-01: [[ ExternalsApiV6 ]] p_name and p_key can now be UTF8-encoded
static MCExternalError MCExternalObjectSet(MCExternalObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, MCExternalVariableRef p_value)
{
	MCObjectHandle t_handle = p_object;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;
	
	if (p_name == nil)
		return kMCExternalErrorNoObjectProperty;

	if (p_value == nil)
		return kMCExternalErrorNoObjectPropertyValue;
	
	if (!t_handle.IsValid())
		return kMCExternalErrorObjectDoesNotExist;
    
    MCAutoStringRef t_name;
    MCAutoStringRef t_key;
    if (!MCStringCreateWithBytes((byte_t*)p_name, strlen(p_name), kMCStringEncodingUTF8, false, &t_name))
        return kMCExternalErrorOutOfMemory;
    if (p_key != nil && MCStringCreateWithBytes((byte_t*)p_key, strlen(p_key), kMCStringEncodingUTF8, false, &t_key))
        return kMCExternalErrorOutOfMemory;
	
	Properties t_prop;
	t_prop = parse_property_name(*t_name);
	
	MCObject *t_object = t_handle;
	
	MCExecContext t_ctxt;
	
	MCExecValue t_value;
	t_value . type = kMCExecValueTypeValueRef;
	t_value . valueref_value = p_value -> GetValueRef();
	
	Exec_stat t_stat;
    t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCNewAutoNameRef t_propset_name, t_propset_key;
		if (*t_key == nil)
		{
			t_propset_name = t_object -> getdefaultpropsetname();
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_key);
		}
		else
		{
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_name);
			/* UNCHECKED */ MCNameCreate(*t_key, &t_propset_key);
		}
		if (!t_object -> setcustomprop(t_ctxt, *t_propset_name, *t_propset_key, nil, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	else
	{
		MCNewAutoNameRef t_index;
		if (*t_key != nil)
		{
			if (!MCNameCreate(*t_key, &t_index))
				return kMCExternalErrorOutOfMemory;
		}
		
		if (!t_object -> setprop(t_ctxt, 0, t_prop, *t_index, False, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	return kMCExternalErrorNone;
}

// SN-2014-07-01: [[ ExternalsApiV6 ]] p_name and p_key can now be UTF8-encoded
static MCExternalError MCExternalObjectGet(MCExternalObjectRef p_object, unsigned int p_options, const char *p_name, const char *p_key, MCExternalVariableRef p_value)
{
	MCObjectHandle t_handle = p_object;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;
	
	if (p_name == nil)
		return kMCExternalErrorNoObjectProperty;
	
	if (p_value == nil)
		return kMCExternalErrorNoObjectPropertyValue;
	
	if (!t_handle.IsValid())
		return kMCExternalErrorObjectDoesNotExist;
    
    MCAutoStringRef t_name;
    MCAutoStringRef t_key;
    if (!MCStringCreateWithBytes((byte_t*)p_name, strlen(p_name), kMCStringEncodingUTF8, false, &t_name))
        return kMCExternalErrorOutOfMemory;
    if (p_key != nil && MCStringCreateWithBytes((byte_t*)p_key, strlen(p_key), kMCStringEncodingUTF8, false, &t_key))
        return kMCExternalErrorOutOfMemory;
	
	Properties t_prop;
	t_prop = parse_property_name(*t_name);
	
	MCObject *t_object = t_handle;
	
	MCExecContext t_ctxt;
	MCExecValue t_value;
	
	Exec_stat t_stat;
    t_stat = ES_NORMAL;
	if (t_prop == P_CUSTOM)
	{
		MCNewAutoNameRef t_propset_name, t_propset_key;
		if (*t_key == nil)
		{
            t_propset_name = t_object -> getdefaultpropsetname();
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_key);
		}
		else
		{
			/* UNCHECKED */ MCNameCreate(*t_name, &t_propset_name);
			/* UNCHECKED */ MCNameCreate(*t_key, &t_propset_key);
		}
		if (!t_object -> getcustomprop(t_ctxt, *t_propset_name, *t_propset_key, nil, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	else
	{
		MCNewAutoNameRef t_index;
		if (*t_key != nil)
		{
			if (!MCNameCreate(*t_key, &t_index))
				return kMCExternalErrorOutOfMemory;
		}
		
		if (!t_object -> getprop(t_ctxt, 0, t_prop, *t_index, False, t_value))
			t_stat = t_ctxt . GetExecStat();
	}
	
	if (t_stat == ES_ERROR)
		return kMCExternalErrorFailed;
	else if (t_stat == ES_EXIT_ALL)
		return kMCExternalErrorExited;
	
	MCAutoValueRef t_value_ref;
    // SN-2014-11-14: [[ Bug 14026 ]] We need to get the address of the pointer, not the pointer itself
	MCExecTypeConvertAndReleaseAlways(t_ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &(&t_value_ref));
	if (t_ctxt . HasError())
		return kMCExternalErrorOutOfMemory;
	
	p_value -> SetValueRef(*t_value_ref);
	
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalObjectUpdate(MCExternalObjectRef p_object, unsigned int p_options, void *p_region)
{
	MCObjectHandle t_handle = p_object;
    
    if (!t_handle.IsBound())
		return kMCExternalErrorNoObject;
	
	if (!t_handle.IsValid())
		return kMCExternalErrorObjectDoesNotExist;
	
	MCObject *t_object = t_handle;

	// MW-2011-08-19: [[ Layers ]] Nothing to do if object not a control.
	if (t_object -> gettype() < CT_GROUP)
		return kMCExternalErrorNone;

	if ((p_options & kMCExternalObjectUpdateOptionRect) == 0)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		static_cast<MCControl *>(t_object) -> layer_redrawall();
	}
	else
	{
		MCRectangle t_obj_rect;
		t_obj_rect = t_object -> getrect();
		
		MCRectangle t_dirty_rect;
		t_dirty_rect . x = t_obj_rect . x + ((int *)p_region)[0];
		t_dirty_rect . y = t_obj_rect . y + ((int *)p_region)[1];
		t_dirty_rect . width = ((int *)p_region)[2] - ((int *)p_region)[0];
		t_dirty_rect . height = ((int *)p_region)[3] - ((int *)p_region)[1];

		// MW-2011-08-18: [[ Layers ]] Invalidate part of the object.
		static_cast<MCControl *>(t_object) -> layer_redrawrect(t_dirty_rect);
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

static MCExternalError MCExternalWaitRun(void *unused, unsigned int p_options)
{
	MCscreen -> wait(60.0, (p_options & kMCExternalWaitOptionDispatching) != 0 ? True : False, True);
	return kMCExternalErrorNone;
}

static MCExternalError MCExternalWaitBreak(void *unused, unsigned int p_options)
{
	// MW-2013-06-25: [[ DesktopPingWait ]] Do a 'pingwait()' on all platforms so
	//   that wait's and such work on all platforms.
	MCscreen -> pingwait();
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

extern MCGFloat MCResGetPixelScale(void);
extern MCGFloat MCResGetUIScale(void);

extern void *MCIPhoneGetView(void);
extern void *MCIPhoneGetViewController(void);
extern void *MCAndroidGetActivity(void);
extern void *MCAndroidGetContainer(void);

// MW-2013-06-13: [[ ExternalsApiV5 ]] Methods to get the JavaEnv's.
extern void *MCAndroidGetScriptJavaEnv(void);
extern void *MCAndroidGetSystemJavaEnv(void);

// MW-2013-07-25: [[ ExternalsApiV5 ]] Method to get the Engine object.
extern void *MCAndroidGetEngine(void);

static MCExternalError MCExternalInterfaceQuery(MCExternalInterfaceQueryTag op, void *r_value)
{
    
	switch(op)
	{
		case kMCExternalInterfaceQueryViewScale:
			// IM-2014-03-14: [[ HiDPI ]] Return the inverse of the logical -> ui coords scale
			*(double *)r_value = 1.0 / MCResGetUIScale();
			break;
			
#if defined(TARGET_SUBPLATFORM_IPHONE)
		case kMCExternalInterfaceQueryView:
			*(void **)r_value = MCIPhoneGetView();
			break;
		case kMCExternalInterfaceQueryViewController:
			*(void **)r_value = MCIPhoneGetViewController();
			break;
#endif
			
#if defined(TARGET_SUBPLATFORM_ANDROID)
		case kMCExternalInterfaceQueryActivity:
			*(void **)r_value = MCAndroidGetActivity();
			break;
		case kMCExternalInterfaceQueryContainer:
			*(void **)r_value = MCAndroidGetContainer();
			break;
			
		// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of the script JavaEnv accessor.
		case kMCExternalInterfaceQueryScriptJavaEnv:
			*(void **)r_value = MCAndroidGetScriptJavaEnv();
			break;
			
		// MW-2013-06-13: [[ ExternalsApiV5 ]] Implementation of the systen JavaEnv accessor.
		case kMCExternalInterfaceQuerySystemJavaEnv:
			*(void **)r_value = MCAndroidGetSystemJavaEnv();
			break;

		// MW-2013-07-25: [[ ExternalsApiV5 ]] Implementation of the Engine accessor.
		case kMCExternalInterfaceQueryEngine:
			*(void **)r_value = MCAndroidGetEngine();
			break;
#endif

		default:
			return kMCExternalErrorInvalidInterfaceQuery;
	}
	
	return kMCExternalErrorNone;
}


////////////////////////////////////////////////////////////////////////////////

MCExternalError MCExternalLicenseCheckEdition(unsigned int p_options, unsigned int p_min_edition)
{
    MCAutoStringRef t_key;
    
    uint32_t t_index;
    if (MCCStringFirstIndexOf(s_current_external->GetName(), '.', t_index))
    {
        if (!MCStringCreateWithCString(s_current_external->GetName(), &t_key))
        {
            return kMCExternalErrorFailed;
        }
    }
    else
    {
        if (!MCStringFormat(&t_key, "com.livecode.external.%s", s_current_external->GetName()))
        {
            return kMCExternalErrorFailed;
        }
    }
    
    MCNewAutoNameRef t_key_as_nameref;
    if (!MCNameCreate(*t_key, &t_key_as_nameref))
    {
        return kMCExternalErrorFailed;
    }
    
    MCValueRef t_value;
    if (MClicenseparameters . addons != nil &&
        MCArrayFetchValue(MClicenseparameters . addons, false, *t_key_as_nameref, t_value))
    {
        return kMCExternalErrorNone;
    }
    
	unsigned int t_current_edition;
	switch(MClicenseparameters . license_class)
	{
		case kMCLicenseClassNone:
			t_current_edition = kMCExternalLicenseTypeNone;
			break;
			
		case kMCLicenseClassCommunity:
			t_current_edition = kMCExternalLicenseTypeCommunity;
			break;
		
        case kMCLicenseClassCommunityPlus:
            t_current_edition = kMCExternalLicenseTypeCommunityPlus;
            break;
            
        case kMCLicenseClassEvaluation:
		case kMCLicenseClassCommercial:
			t_current_edition = kMCExternalLicenseTypeIndy;
			break;
			
		case kMCLicenseClassProfessionalEvaluation:
		case kMCLicenseClassProfessional:
			t_current_edition = kMCExternalLicenseTypeBusiness;
			break;
			
		default:
			MCUnreachableReturn(kMCExternalErrorUnlicensed);
	}
	
	if (kMCExternalLicenseTypeNone == p_min_edition ||
        t_current_edition < p_min_edition)
	{
		s_current_external -> SetWasLicensed(false);
		return kMCExternalErrorUnlicensed;
	}
	
	return kMCExternalErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

MCExternalInterface g_external_interface =
{
	kMCExternalInterfaceVersion,

	MCExternalEngineRunOnMainThread,

    // SN-2015-01-26: [[ Bug 14057 ]] Former ContextQuery now set as legacy.
	MCExternalContextQueryLegacy,

	MCExternalVariableCreate,
	MCExternalVariableRetain,
	MCExternalVariableRelease,
	MCExternalVariableQuery,
	MCExternalVariableClear,
	MCExternalVariableExchange,
	MCExternalVariableStore,
	MCExternalVariableFetch,
	MCExternalVariableAppend,
	MCExternalVariablePrepend,
	MCExternalVariableEdit,
	MCExternalVariableCountKeys,
	MCExternalVariableIterateKeys,
	MCExternalVariableRemoveKey,
	MCExternalVariableLookupKey,

	MCExternalObjectResolve,
	MCExternalObjectExists,
	MCExternalObjectRetain,
	MCExternalObjectRelease,
	MCExternalObjectDispatch,
	
	MCExternalWaitRun,
	MCExternalWaitBreak,
	
	MCExternalObjectGet,
	MCExternalObjectSet,
	
	MCExternalInterfaceQuery,
	
	MCExternalObjectUpdate,
	
	// MW-2013-06-13: [[ ExternalsApiV5 ]] Declare the evaluate and execute methods
	//   for the outside world.
	MCExternalContextEvaluate,
	MCExternalContextExecute,
	
    MCExternalContextQuery,

	// MW-2016-02-17: [[ LicenseCheck ]] Declare the check call.
	MCExternalLicenseCheckEdition,
};

////////////////////////////////////////////////////////////////////////////////

