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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

enum
{
    kMCEncodedValueKindNull,
    kMCEncodedValueKindTrue,
    kMCEncodedValueKindFalse,
    kMCEncodedValueKindIntegerZero,
    kMCEncodedValueKindIntegerOne,
    kMCEncodedValueKindIntegerMinusOne,
    kMCEncodedValueKindIntegerPositive,
    kMCEncodedValueKindIntegerNegative,
    kMCEncodedValueKindRealZero,
    kMCEncodedValueKindRealOne,
    kMCEncodedValueKindRealMinusOne,
    kMCEncodedValueKindReal,
    kMCEncodedValueKindName,
    kMCEncodedValueKindString,
    kMCEncodedValueKindData,
    kMCEncodedValueKindArray,
    kMCEncodedValueKindProperList,
    
    kMCEncodedValueKindNamedTypeInfo,
    kMCEncodedValueKindNullTypeInfo,
    kMCEncodedValueKindBooleanTypeInfo,
    kMCEncodedValueKindNumberTypeInfo,
    kMCEncodedValueKindNameTypeInfo,
    kMCEncodedValueKindStringTypeInfo,
    kMCEncodedValueKindDataTypeInfo,
    kMCEncodedValueKindArrayTypeInfo,
    kMCEncodedValueKindProperListTypeInfo,
    kMCEncodedValueKindRecordTypeInfo,
    kMCEncodedValueKindHandlerTypeInfo,
    kMCEncodedValueKindErrorTypeInfo,
};

////////////////////////////////////////////////////////////////////////////////

bool MCPickleRead(MCStreamRef stream, MCPickleRecordInfo *p_info, MCValueRef*& r_value_pool, uindex_t& r_value_pool_size, void *r_record)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

// We encode uint's as a sequence of bytes, each byte containing 7-bits of the
// final value. The last byte in the sequence has 0 as the top-bit, the rest have
// 1 as the top-bit.
static bool MCPickleWriteCompactUInt(MCStreamRef stream, uint32_t p_value)
{
    // We encode 7 bits per byte, meaning a maximum of 5 bytes to encode a 32-bit
    // integer.
    uint8_t t_bytes[5];
    uindex_t t_index;
    t_index = 0;
    while(p_value != 0)
    {
        // Fetch the next 7 bits.
        uint8_t t_byte;
        t_byte = p_value & 0x7f;
        
        // Remove from the value.
        p_value = p_value >> 7;
        
        // If there is anything left in the value, mark the top-bit.
        if (p_value != 0)
            t_byte |= 1 << 7;
        
        t_bytes[t_index++] = t_byte;
    }
    
    return MCStreamWrite(stream, t_bytes, t_index);
}

static bool MCPickleWriteStringRef(MCStreamRef stream, MCStringRef p_value)
{
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsUTF8String t_utf8_string;
    if (!t_utf8_string . Lock(p_value))
        return false;
    
    return MCPickleWriteCompactUInt(stream, t_utf8_string . Size()) &&
            MCStreamWrite(stream, *t_utf8_string, t_utf8_string . Size());
}

static bool MCPickleWriteTypeInfoRef(MCStreamRef stream, MCTypeInfoRef p_value)
{
    bool t_success;
    t_success = true;
    
    switch(MCTypeInfoGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNullTypeInfo);
            break;
        case kMCValueTypeCodeBoolean:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindBooleanTypeInfo);
            break;
        case kMCValueTypeCodeNumber:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNumberTypeInfo);
            break;
        case kMCValueTypeCodeName:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNameTypeInfo);
            break;
        case kMCValueTypeCodeString:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindStringTypeInfo);
            break;
        case kMCValueTypeCodeData:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindDataTypeInfo);
            break;
        case kMCValueTypeCodeArray:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindArrayTypeInfo);
            break;
        case kMCValueTypeCodeProperList:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindProperListTypeInfo);
            break;
        case kMCValueTypeCodeRecord:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindRecordTypeInfo);
            // TODO: Encode the rest of the record info
            break;
        case kMCValueTypeCodeHandler:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindHandlerTypeInfo);
            // TODO: Encode the rest of the handler info
            break;
        case kMCValueTypeCodeError:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindErrorTypeInfo);
            // TODO: Encode the rest of the error info
            break;
        default:
            // Unsupported typeinfo for pickle.
            t_success = false;
            break;
    }
    
    return t_success;
}

static bool MCPickleWriteValueRef(MCStreamRef stream, MCValueRef p_value)
{
    bool t_success;
    t_success = true;
    
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNull);
        case kMCValueTypeCodeBoolean:
            t_success = MCStreamWriteUInt8(stream, (MCBooleanRef)p_value == kMCTrue ? kMCEncodedValueKindTrue : kMCEncodedValueKindFalse);
            break;
        case kMCValueTypeCodeNumber:
            if (MCNumberIsInteger((MCNumberRef)p_value))
            {
                integer_t t_integer;
                t_integer = MCNumberFetchAsInteger((MCNumberRef)p_value);
                if (MCAbs(t_integer) <= 1)
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream,
                                                       t_integer == 0 ? kMCEncodedValueKindIntegerZero :
                                                        (t_integer == 1 ? kMCEncodedValueKindIntegerOne : kMCEncodedValueKindIntegerMinusOne));
                }
                else
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream, t_integer > 0 ? kMCEncodedValueKindIntegerPositive : kMCEncodedValueKindIntegerNegative);
                    if (t_success)
                        t_success = MCPickleWriteCompactUInt(stream, t_integer > 0 ? t_integer : -t_integer);
                }
            }
            else
            {
                double t_real;
                t_real = MCNumberFetchAsReal((MCNumberRef)p_value);
                if (t_real == 0.0)
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindRealZero);
                }
                else if (t_real == 1.0)
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindRealOne);
                }
                else if (t_real == -1.0)
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindRealMinusOne);
                }
                else
                {
                    if (t_success)
                        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindReal);
                    if (t_success)
                        t_success = MCStreamWriteDouble(stream, t_real);
                }
            }
            break;
        case kMCValueTypeCodeName:
            if (t_success)
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindName);
            if (t_success)
                t_success = MCPickleWriteStringRef(stream, MCNameGetString((MCNameRef)p_value));
            break;
        case kMCValueTypeCodeString:
            if (t_success)
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindString);
            if (t_success)
                t_success = MCPickleWriteStringRef(stream, MCNameGetString((MCNameRef)p_value));
            break;
        case kMCValueTypeCodeData:
            if (t_success)
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindData);
            if (t_success)
                t_success = MCPickleWriteCompactUInt(stream, MCDataGetLength((MCDataRef)p_value));
            if (t_success)
                t_success = MCStreamWrite(stream, MCDataGetBytePtr((MCDataRef)p_value), MCDataGetLength((MCDataRef)p_value));
            break;
        case kMCValueTypeCodeArray:
            if (t_success)
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindArray);
            if (t_success)
                t_success = MCPickleWriteCompactUInt(stream, MCArrayGetCount((MCArrayRef)p_value));
            if (t_success)
            {
                MCNameRef t_key;
                MCValueRef t_element;
                uintptr_t t_iterator;
                t_iterator = 0;
                while(t_success &&
                      MCArrayIterate((MCArrayRef)p_value, t_iterator, t_key, t_element))
                    t_success = MCPickleWriteStringRef(stream, MCNameGetString(t_key)) &&
                                    MCPickleWriteValueRef(stream, t_element);
                
            }
            break;
        case kMCValueTypeCodeProperList:
            if (t_success)
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindProperList);
            if (t_success)
                t_success = MCPickleWriteCompactUInt(stream, MCProperListGetLength((MCProperListRef)p_value));
            if (t_success)
            {
                MCValueRef t_element;
                uintptr_t t_iterator;
                t_iterator = 0;
                while(t_success &&
                      MCProperListIterate((MCProperListRef)p_value, t_iterator, t_element))
                    t_success = MCPickleWriteValueRef(stream, t_element);
            }
            break;
        case kMCValueTypeCodeTypeInfo:
            if (MCTypeInfoGetName((MCTypeInfoRef)p_value) != nil)
            {
                if (t_success)
                    t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNamedTypeInfo);
                if (t_success)
                    t_success = MCPickleWriteStringRef(stream, MCNameGetString(MCTypeInfoGetName((MCTypeInfoRef)p_value)));
                if (t_success)
                    t_success = MCPickleWriteTypeInfoRef(stream, (MCTypeInfoRef)p_value);
            }
            else
            {
                if (t_success)
                    t_success = MCPickleWriteTypeInfoRef(stream, (MCTypeInfoRef)p_value);
            }
            break;
        default:
            // Unsupported value for pickle.
            return false;
    }
    return false;
}

static bool MCPickleWriteField(MCStreamRef stream, MCPickleFieldType p_kind, void *p_base_ptr, void *p_field_ptr, const void *p_aux_ptr, const void *p_extra)
{
    bool t_success;
    t_success = true;
    
    switch(p_kind)
    {
        case kMCPickleFieldTypeNone:
            break;
        case kMCPickleFieldTypeByte:
            t_success = MCStreamWriteUInt8(stream, *(uint8_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeUIndex:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeIntEnum:
            t_success = MCPickleWriteCompactUInt(stream, *(intenum_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeValueRef:
            t_success = MCPickleWriteValueRef(stream, *(MCValueRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeStringRef:
            t_success = MCPickleWriteStringRef(stream, *(MCStringRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeNameRef:
            t_success = MCPickleWriteStringRef(stream, MCNameGetString(*(MCNameRef *)p_field_ptr));
            break;
        case kMCPickleFieldTypeTypeInfoRef:
            t_success = MCPickleWriteTypeInfoRef(stream, *(MCTypeInfoRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeArrayOfByte:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCStreamWrite(stream, *(const uint8_t **)p_field_ptr, *(uindex_t *)p_aux_ptr);
            break;
        case kMCPickleFieldTypeArrayOfNameRef:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWriteStringRef(stream, MCNameGetString((*(MCNameRef **)p_field_ptr)[i]));
            break;
        case kMCPickleFieldTypeArrayOfRecord:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWrite(stream, (MCPickleRecordInfo *)p_extra, &((void **)p_field_ptr)[i]);
            break;
        case kMCPickleFieldTypeArrayOfVariant:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
            {
                void *t_variant;
                t_variant = ((void **)p_field_ptr)[i];
                
                MCPickleVariantInfo *t_info;
                t_info = (MCPickleVariantInfo *)p_extra;
                
                int t_kind;
                t_kind = *((int *)((uint8_t *)t_variant + t_info -> kind_offset));
                
                MCPickleRecordInfo *t_record_info;
                t_record_info = nil;
                for(int i = 0; t_info -> cases[i] . kind != -1; i++)
                    if (t_kind == t_info -> cases[i] . kind)
                    {
                        t_record_info = t_info -> cases[i] . record;
                        break;
                    }
                
                if (t_record_info != nil)
                    t_success = MCPickleWrite(stream, t_record_info, t_variant);
                else
                    t_success = false;
            }
            break;
    }
    
    return t_success;
}

bool MCPickleWrite(MCStreamRef stream, MCPickleRecordInfo *p_info, void *p_record)
{
    bool t_success;
    t_success = true;
    
    for(uindex_t i = 0; t_success && i < p_info -> fields[i] . kind != kMCPickleFieldTypeNone; i++)
    {
        void *t_field_ptr, *t_extra_field_ptr;
        t_field_ptr = static_cast<uint8_t *>(p_record) + p_info -> fields[i] . field_offset;
        t_extra_field_ptr = static_cast<uint8_t *>(p_record) + p_info -> fields[i] . aux_field_offset;
        if (t_success)
            t_success = MCPickleWriteField(stream, p_info -> fields[i] . kind, p_record, t_field_ptr, t_extra_field_ptr, p_info -> fields[i] . extra);
    }
        
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////
