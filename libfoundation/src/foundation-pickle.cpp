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

static bool MCPickleReadCompactUInt(MCStreamRef stream, uint32_t& r_value)
{
    uint32_t t_value;
    t_value = 0;
    
    int t_shift;
    t_shift = 0;
    
    uint8_t t_byte;
    for(;;)
    {
        if (!MCStreamReadUInt8(stream, t_byte))
            return false;
        
        t_value |= (t_byte & 0x7f) << t_shift;
        
        if ((t_byte & 0x80) == 0)
            break;
        
        t_shift += 7;
    }
    
    r_value = t_value;
    
    return true;
}

static bool MCPickleReadStringRef(MCStreamRef stream, MCStringRef& r_value)
{
    uindex_t t_length;
    if (!MCPickleReadCompactUInt(stream, t_length))
        return false;
    
    uint8_t *t_bytes;
    if (!MCMemoryNewArray(t_length, t_bytes))
        return false;
    
    if (!MCStreamRead(stream, t_bytes, t_length) ||
        !MCStringCreateWithBytesAndRelease(t_bytes, t_length, kMCStringEncodingUTF8, false, r_value))
    {
        free(t_bytes);
        return false;
    }
    
    return true;
}

static bool MCPickleReadNameRef(MCStreamRef stream, MCNameRef& r_value)
{
    MCAutoStringRef t_string;
    if (!MCPickleReadStringRef(stream, &t_string))
        return false;
    return MCNameCreate(*t_string, r_value);
}

static bool MCPickleReadTypeInfoRef(MCStreamRef stream, uint8_t p_kind, MCTypeInfoRef& r_value)
{
    MCNewAutoNameRef t_name;
    uint8_t t_kind;
    if (p_kind == kMCEncodedValueKindNamedTypeInfo)
    {
        if (!MCPickleReadNameRef(stream, &t_name))
            return false;
        if (!MCStreamReadUInt8(stream, t_kind))
            return false;
    }
    else
        t_kind = p_kind;
    
    MCTypeInfoRef t_typeinfo;
    switch(t_kind)
    {
        case kMCEncodedValueKindNullTypeInfo:
            t_typeinfo = MCValueRetain(kMCNullTypeInfo);
            break;
        case kMCEncodedValueKindBooleanTypeInfo:
            t_typeinfo = MCValueRetain(kMCBooleanTypeInfo);
            break;
        case kMCEncodedValueKindNumberTypeInfo:
            t_typeinfo = MCValueRetain(kMCNumberTypeInfo);
            break;
            //case kMCEncodedValueKindNameTypeInfo:
            //    r_value = MCValueRetain(kMCNameTypeInfo);
            //    break;
        case kMCEncodedValueKindStringTypeInfo:
            t_typeinfo = MCValueRetain(kMCStringTypeInfo);
            break;
        case kMCEncodedValueKindDataTypeInfo:
            t_typeinfo = MCValueRetain(kMCDataTypeInfo);
            break;
        case kMCEncodedValueKindArrayTypeInfo:
            t_typeinfo = MCValueRetain(kMCArrayTypeInfo);
            break;
        case kMCEncodedValueKindProperListTypeInfo:
            t_typeinfo = MCValueRetain(kMCProperListTypeInfo);
            break;
            //case kMCEncodedValueKindRecordTypeInfo:
            //r_value = MCValueRetain(kMCRecordTypeInfo);
            //break;
        case kMCEncodedValueKindHandlerTypeInfo:
            {
                bool t_success;
                t_success = true;
                
                uindex_t t_param_count;
                if (t_success)
                    t_success = MCPickleReadCompactUInt(stream, t_param_count);
                
                MCAutoArray<MCHandlerTypeFieldInfo> t_param_info;
                if (t_success)
                    t_success = t_param_info . New(t_param_count);
                
                for(uindex_t i = 0; t_success && i < t_param_count; i++)
                {
                    uint8_t t_mode, t_type_kind;;
                    t_success = MCPickleReadNameRef(stream, t_param_info[i] . name) &&
                                    MCStreamReadUInt8(stream, t_mode) &&
                                    MCStreamReadUInt8(stream, t_type_kind) &&
                                    MCPickleReadTypeInfoRef(stream, t_type_kind, t_param_info[i] . type);
                    if (t_success)
                        t_param_info[i] . mode = (MCHandlerTypeFieldMode)t_mode;
                }
                
                MCAutoTypeInfoRef t_return_type;
                if (t_success)
                {
                    uint8_t t_type_kind;
                    t_success = MCStreamReadUInt8(stream, t_type_kind) &&
                                    MCPickleReadTypeInfoRef(stream, t_type_kind, &t_return_type);
                }
                
                if (t_success)
                    t_success = MCHandlerTypeInfoCreate(t_param_info . Ptr(), t_param_info . Size(), *t_return_type, t_typeinfo);
                
                for(uindex_t i = 0; i < t_param_count; i++)
                {
                    if (t_param_info[i] . name != nil)
                        MCValueRelease(t_param_info[i] . name);
                    if (t_param_info[i] . type != nil)
                        MCValueRelease(t_param_info[i] . type);
                }
            }
            break;
            //case kMCEncodedValueKindErrorTypeInfo:
            //r_value = MCValueRetain(kMCErrorTypeInfo);
            //break;
        default:
            return false;
    }
    
    if (*t_name != nil)
    {
        if (!MCTypeInfoBindAndRelease(*t_name, t_typeinfo, r_value))
        {
            MCValueRelease(t_typeinfo);
            return false;
        }
    }
    else
    {
        r_value = t_typeinfo;
    }
    
    return true;
}

static bool MCPickleReadValueRef(MCStreamRef stream, MCValueRef& r_value)
{
    uint8_t t_kind;
    if (!MCStreamReadUInt8(stream, t_kind))
        return false;
    
    switch(t_kind)
    {
        case kMCEncodedValueKindNull:
            r_value = MCValueRetain(kMCNull);
            break;
            
        case kMCEncodedValueKindTrue:
            r_value = MCValueRetain(kMCTrue);
            break;
        
        case kMCEncodedValueKindFalse:
            r_value = MCValueRetain(kMCFalse);
            break;
            
        case kMCEncodedValueKindIntegerZero:
            r_value = MCValueRetain(kMCZero);
            break;
            
        case kMCEncodedValueKindIntegerOne:
            r_value = MCValueRetain(kMCOne);
            break;
            
        case kMCEncodedValueKindIntegerMinusOne:
            r_value = MCValueRetain(kMCMinusOne);
            break;
            
        case kMCEncodedValueKindIntegerPositive:
            {
                uint32_t t_value;
                if (!MCPickleReadCompactUInt(stream, t_value))
                    return false;
                if (!MCNumberCreateWithInteger(t_value, (MCNumberRef&)t_value))
                    return false;
            }
            break;
        
        case kMCEncodedValueKindIntegerNegative:
            {
                uint32_t t_value;
                if (!MCPickleReadCompactUInt(stream, t_value))
                    return false;
                if (!MCNumberCreateWithInteger(-t_value, (MCNumberRef&)t_value))
                    return false;
            }
            break;
            
        case kMCEncodedValueKindRealZero:
            r_value = MCValueRetain(kMCZero); // TODO - this needs to be real zero
            break;
            
        case kMCEncodedValueKindRealOne:
            r_value = MCValueRetain(kMCOne); // TODO - this needs to be real one
            break;
            
        case kMCEncodedValueKindRealMinusOne:
            r_value = MCValueRetain(kMCMinusOne); // TODO - this needs to be real minus one
            break;
            
        case kMCEncodedValueKindReal:
            {
                double t_value;
                if (!MCStreamReadDouble(stream, t_value))
                    return false;
                if (!MCNumberCreateWithReal(t_value, (MCNumberRef&)t_value))
                    return false;
            }
            break;
            
        case kMCEncodedValueKindName:
            if (!MCPickleReadNameRef(stream, (MCNameRef&)r_value))
                return false;
            break;
            
        case kMCEncodedValueKindString:
            if (!MCPickleReadStringRef(stream, (MCStringRef&)r_value))
                return false;
            break;
            
        case kMCEncodedValueKindData:
            {
                uindex_t t_size;
                if (!MCPickleReadCompactUInt(stream, t_size))
                    return false;
                
                uint8_t *t_bytes;
                if (!MCMemoryNewArray(t_size, t_bytes))
                    return false;
                
                if (!MCStreamRead(stream, t_bytes, t_size) ||
                    !MCDataCreateWithBytesAndRelease(t_bytes, t_size, (MCDataRef&)r_value))
                {
                    free(t_bytes);
                    return false;
                }
            }
            break;
            
        case kMCEncodedValueKindArray:
            {
                uindex_t t_size;
                if (!MCPickleReadCompactUInt(stream, t_size))
                    return false;
                
                MCArrayRef t_array;
                if (!MCArrayCreateMutable(t_array))
                    return false;
                
                while(t_size > 0)
                {
                    MCNewAutoNameRef t_key;
                    MCAutoValueRef t_element;
                    if (!MCPickleReadNameRef(stream, &t_key) ||
                        !MCPickleReadValueRef(stream, &t_element) ||
                        !MCArrayStoreValue(t_array, true, *t_key, *t_element))
                    {
                        MCValueRelease(t_array);
                        return false;
                    }
                    
                    t_size -= 1;
                }
                
                r_value = t_array;
            }
            break;
            
        case kMCEncodedValueKindProperList:
            {
                uindex_t t_size;
                if (!MCPickleReadCompactUInt(stream, t_size))
                    return false;
                
                MCProperListRef t_list;
                if (!MCProperListCreateMutable(t_list))
                    return false;
                
                while(t_size > 0)
                {
                    MCAutoValueRef t_value;
                    if (!MCPickleReadValueRef(stream, &t_value) ||
                        !MCProperListPushElementOntoBack(t_list, *t_value))
                    {
                        MCValueRelease(t_list);
                        return false;
                    }
                    
                    t_size -= 1;
                }
                
                r_value = t_list;
            }
            break;
            
        case kMCEncodedValueKindNamedTypeInfo:
        case kMCEncodedValueKindNullTypeInfo:
        case kMCEncodedValueKindBooleanTypeInfo:
        case kMCEncodedValueKindNumberTypeInfo:
        case kMCEncodedValueKindNameTypeInfo:
        case kMCEncodedValueKindStringTypeInfo:
        case kMCEncodedValueKindDataTypeInfo:
        case kMCEncodedValueKindArrayTypeInfo:
        case kMCEncodedValueKindProperListTypeInfo:
        case kMCEncodedValueKindRecordTypeInfo:
        case kMCEncodedValueKindHandlerTypeInfo:
        case kMCEncodedValueKindErrorTypeInfo:
            if (!MCPickleReadTypeInfoRef(stream, t_kind, (MCTypeInfoRef&)r_value))
                return false;
            break;
    }

    return true;
}

static bool MCPickleReadField(MCStreamRef stream, MCPickleFieldType p_kind, void *p_base_ptr, void *p_field_ptr, const void *p_aux_ptr, const void *p_extra)
{
    bool t_success;
    t_success = true;
    
    
    switch(p_kind)
    {
        case kMCPickleFieldTypeNone:
            break;
        case kMCPickleFieldTypeByte:
            t_success = MCStreamReadUInt8(stream, *(uint8_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeUIndex:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeIntEnum:
            t_success = MCPickleReadCompactUInt(stream, *(uint32_t *)p_field_ptr);
            break;
        case kMCPickleFieldTypeValueRef:
            t_success = MCPickleReadValueRef(stream, *(MCValueRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeStringRef:
            t_success = MCPickleReadStringRef(stream, *(MCStringRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeNameRef:
            t_success = MCPickleReadNameRef(stream, *(MCNameRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeTypeInfoRef:
            {
                uint8_t t_kind;
                t_success = MCStreamReadUInt8(stream, t_kind);
                if (t_success)
                    t_success = MCPickleReadTypeInfoRef(stream, t_kind, *(MCTypeInfoRef *)p_field_ptr);
            }
            break;
        case kMCPickleFieldTypeArrayOfByte:

            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(uint8_t **)p_field_ptr) &&
                            MCStreamRead(stream, *(void **)p_field_ptr, *(uindex_t *)p_aux_ptr);
            break;
        case kMCPickleFieldTypeArrayOfValueRef:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(MCValueRef **)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleReadValueRef(stream, (*(MCValueRef **)p_field_ptr)[i]);
            break;
        case kMCPickleFieldTypeArrayOfNameRef:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(MCValueRef **)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleReadNameRef(stream, (*(MCNameRef **)p_field_ptr)[i]);
            break;
        case kMCPickleFieldTypeArrayOfRecord:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, ((MCPickleRecordInfo *)p_extra) -> size, *(void **)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleRead(stream, (MCPickleRecordInfo *)p_extra, *((uint8_t **)p_field_ptr) + i * ((MCPickleRecordInfo *)p_extra) -> size);
            break;
        case kMCPickleFieldTypeArrayOfVariant:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(void ***)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
            {
                MCPickleVariantInfo *t_info;
                t_info = (MCPickleVariantInfo *)p_extra;
                
                uint32_t t_kind;
                t_success = MCPickleReadCompactUInt(stream, t_kind);
                if (!t_success)
                    break;
                
                MCPickleRecordInfo *t_record_info;
                t_record_info = nil;
                for(int i = 0; t_info -> cases[i] . kind != -1; i++)
                    if (t_kind == t_info -> cases[i] . kind)
                    {
                        t_record_info = t_info -> cases[i] . record;
                        break;
                    }
                
                if (t_record_info != nil)
                {
                    void *t_new_record;
                    t_success = MCMemoryNew(t_record_info -> size, t_new_record);
                    if (t_success)
                        t_success = MCPickleRead(stream, t_record_info, t_new_record);
                    if (t_success)
                    {
                        *(uint32_t *)(((uint8_t *)t_new_record) + t_info -> kind_offset) = t_kind;
                        (*(void ***)p_field_ptr)[i] = t_new_record;
                    }
                    else
                        free(t_new_record);
                }
                else
                    t_success = false;
            }
            break;
    }
    
    return t_success;
}

bool MCPickleRead(MCStreamRef stream, MCPickleRecordInfo *p_info, void* r_record)
{
    bool t_success;
    t_success = true;
    
    for(uindex_t i = 0; t_success && p_info -> fields[i] . kind != kMCPickleFieldTypeNone; i++)
    {
        MCPickleRecordFieldInfo *t_field;
        t_field = &p_info -> fields[i];
        
        void *t_field_ptr, *t_extra_field_ptr;
        t_field_ptr = static_cast<uint8_t *>(r_record) + t_field -> field_offset;
        t_extra_field_ptr = static_cast<uint8_t *>(r_record) + t_field -> aux_field_offset;
        t_success = MCPickleReadField(stream, t_field -> kind, r_record, t_field_ptr, t_extra_field_ptr, t_field -> extra);
    }

    if (!t_success)
        MCPickleRelease(p_info, r_record);

    return t_success;
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
    do
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
    while(p_value != 0);
    
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
    
    if (MCTypeInfoGetName(p_value) != nil)
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNamedTypeInfo);
        if (t_success)
            t_success = MCPickleWriteStringRef(stream, MCNameGetString(MCTypeInfoGetName((MCTypeInfoRef)p_value)));
    }
    
    if (t_success)
    {
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
                t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindHandlerTypeInfo) &&
                                MCPickleWriteCompactUInt(stream, MCHandlerTypeInfoGetParameterCount(p_value));
                for(uindex_t i = 0; t_success && i < MCHandlerTypeInfoGetParameterCount(p_value); i++)
                    t_success = MCPickleWriteStringRef(stream, MCNameGetString(MCHandlerTypeInfoGetParameterName(p_value, i))) &&
                                    MCStreamWriteUInt8(stream, MCHandlerTypeInfoGetParameterMode(p_value, i)) &&
                                    MCPickleWriteTypeInfoRef(stream, MCHandlerTypeInfoGetParameterType(p_value, i));
                if (t_success)
                    t_success = MCPickleWriteTypeInfoRef(stream, MCHandlerTypeInfoGetReturnType(p_value));
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
            if (t_success)
                t_success = MCPickleWriteTypeInfoRef(stream, (MCTypeInfoRef)p_value);
            break;
        default:
            // Unsupported value for pickle.
            return false;
    }
    return t_success;
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
        case kMCPickleFieldTypeArrayOfValueRef:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWriteValueRef(stream, (*(MCValueRef **)p_field_ptr)[i]);
            break;
        case kMCPickleFieldTypeArrayOfNameRef:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWriteStringRef(stream, MCNameGetString((*(MCNameRef **)p_field_ptr)[i]));
            break;
        case kMCPickleFieldTypeArrayOfRecord:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWrite(stream, (MCPickleRecordInfo *)p_extra, *((uint8_t **)p_field_ptr) + i * ((MCPickleRecordInfo *)p_extra) -> size);
            break;
        case kMCPickleFieldTypeArrayOfVariant:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
            {
                void *t_variant;
                t_variant = (*(void ***)p_field_ptr)[i];
                
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
                {
                    t_success = MCPickleWriteCompactUInt(stream, t_kind) &&
                                    MCPickleWrite(stream, t_record_info, t_variant);
                }
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
    
    for(uindex_t i = 0; t_success && p_info -> fields[i] . kind != kMCPickleFieldTypeNone; i++)
    {
        MCPickleRecordFieldInfo *t_field;
        t_field = &p_info -> fields[i];
        
        void *t_field_ptr, *t_extra_field_ptr;
        t_field_ptr = static_cast<uint8_t *>(p_record) + t_field -> field_offset;
        t_extra_field_ptr = static_cast<uint8_t *>(p_record) + t_field -> aux_field_offset;
        t_success = MCPickleWriteField(stream, t_field -> kind, p_record, t_field_ptr, t_extra_field_ptr, t_field -> extra);
    }
        
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static void MCPickleReleaseField(MCPickleFieldType p_kind, void *p_base_ptr, void *p_field_ptr, const void *p_aux_ptr, const void *p_extra)
{
    switch(p_kind)
    {
        case kMCPickleFieldTypeValueRef:
        case kMCPickleFieldTypeStringRef:
        case kMCPickleFieldTypeNameRef:
        case kMCPickleFieldTypeTypeInfoRef:
            MCValueRelease(*(MCValueRef *)p_field_ptr);
            *(MCValueRef *)p_field_ptr = nil;
            break;
            
        case kMCPickleFieldTypeArrayOfByte:
            free(*(void **)p_field_ptr);
            *(MCValueRef *)p_field_ptr = nil;
            break;
            
        case kMCPickleFieldTypeArrayOfValueRef:
        case kMCPickleFieldTypeArrayOfNameRef:
            if (*(MCValueRef **)p_field_ptr != nil)
            {
                for(uindex_t i = 0; i < *(uindex_t *)p_aux_ptr; i++)
                    MCValueRelease((*(MCValueRef **)p_field_ptr)[i]);
                free(*(MCValueRef **)p_field_ptr);
                *(uindex_t *)p_aux_ptr = 0;
                *(MCValueRef *)p_field_ptr = nil;
            }
            break;
        
        case kMCPickleFieldTypeArrayOfRecord:
            if (*(void **)p_field_ptr != nil)
            {
                for(uindex_t i = 0; i < *(uindex_t *)p_aux_ptr; i++)
                    MCPickleRelease((MCPickleRecordInfo *)p_extra, *((uint8_t **)p_field_ptr) + i * ((MCPickleRecordInfo *)p_extra) -> size);
                free(*(void **)p_field_ptr);
                *(uindex_t *)p_aux_ptr = 0;
                *(MCValueRef *)p_field_ptr = nil;
            }
            break;
            
        case kMCPickleFieldTypeArrayOfVariant:
            if (*(void **)p_field_ptr != nil)
            {
                for(uindex_t i = 0; i < *(uindex_t *)p_aux_ptr; i++)
                {
                    void *t_variant;
                    t_variant = (*(void ***)p_field_ptr)[i];
                
                    if (t_variant != nil)
                    {
                        MCPickleVariantInfo *t_info;
                        t_info = (MCPickleVariantInfo *)p_extra;
                        
                        int t_kind;
                        t_kind = *((int *)((uint8_t *)t_variant + t_info -> kind_offset));

                        for(int i = 0; t_info -> cases[i] . kind != -1; i++)
                            if (t_kind == t_info -> cases[i] . kind)
                            {
                                MCPickleRelease(t_info -> cases[i] . record, t_variant);
                                break;
                            }
                    }
                }
                free(*(void **)p_field_ptr);
                *(uindex_t *)p_aux_ptr = 0;
                *(MCValueRef *)p_field_ptr = nil;
            }
            break;
            
        default:
            break;
    }
}

void MCPickleRelease(MCPickleRecordInfo *p_info, void *p_record)
{
    for(uindex_t i = 0; p_info -> fields[i] . kind != kMCPickleFieldTypeNone; i++)
    {
        void *t_field_ptr, *t_extra_field_ptr;
        t_field_ptr = static_cast<uint8_t *>(p_record) + p_info -> fields[i] . field_offset;
        t_extra_field_ptr = static_cast<uint8_t *>(p_record) + p_info -> fields[i] . aux_field_offset;
        MCPickleReleaseField(p_info -> fields[i] . kind, p_record, t_field_ptr, t_extra_field_ptr, p_info -> fields[i] . extra);
    }
}

////////////////////////////////////////////////////////////////////////////////
