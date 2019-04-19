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
    
    kMCEncodedValueKindAliasTypeInfo,
    kMCEncodedValueKindNamedTypeInfo,
    kMCEncodedValueKindOptionalTypeInfo,
    
    kMCEncodedValueKindAnyTypeInfo,
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

static bool failed(void)
{
    return false;
}

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
            return failed();
        
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
        return failed();
    
    uint8_t *t_bytes;
    if (!MCMemoryNewArray(t_length, t_bytes))
        return failed();
    
    if (!MCStreamRead(stream, t_bytes, t_length) ||
        !MCStringCreateWithBytesAndRelease(t_bytes, t_length, kMCStringEncodingUTF8, false, r_value))
    {
        free(t_bytes);
        return failed();
    }
    
    return true;
}

static bool MCPickleReadNameRef(MCStreamRef stream, MCNameRef& r_value)
{
    MCAutoStringRef t_string;
    if (!MCPickleReadStringRef(stream, &t_string))
        return failed();
    return MCNameCreate(*t_string, r_value);
}

static bool MCPickleReadTypeInfoRefContents(MCStreamRef stream, uint8_t p_kind, MCTypeInfoRef& r_value);
static bool MCPickleReadTypeInfoRef(MCStreamRef stream, MCTypeInfoRef& r_typeinfo)
{
    uint8_t t_kind;
    if (!MCStreamReadUInt8(stream, t_kind))
        return failed();
    return MCPickleReadTypeInfoRefContents(stream, t_kind, r_typeinfo);
}

static bool MCPickleReadTypeInfoRefContents(MCStreamRef stream, uint8_t p_kind, MCTypeInfoRef& r_value)
{
    if (p_kind == kMCEncodedValueKindAliasTypeInfo)
    {
        MCNewAutoNameRef t_name;
        if (!MCPickleReadNameRef(stream, &t_name))
            return failed();
        MCAutoTypeInfoRef t_base;
        if (!MCPickleReadTypeInfoRef(stream, &t_base))
            return failed();
        if (!MCAliasTypeInfoCreate(*t_name, *t_base, r_value))
            return failed();
    }
    else if (p_kind == kMCEncodedValueKindNamedTypeInfo)
    {
        MCNewAutoNameRef t_name;
        if (!MCPickleReadNameRef(stream, &t_name))
            return failed();
        if (!MCNamedTypeInfoCreate(*t_name, r_value))
            return failed();
    }
    else if (p_kind == kMCEncodedValueKindOptionalTypeInfo)
    {
        MCAutoTypeInfoRef t_base;
        if (!MCPickleReadTypeInfoRef(stream, &t_base))
            return failed();
        if (!MCOptionalTypeInfoCreate(*t_base, r_value))
            return failed();
    }
    else if (p_kind == kMCEncodedValueKindAnyTypeInfo)
        r_value = MCValueRetain(kMCAnyTypeInfo);
    else if (p_kind == kMCEncodedValueKindNullTypeInfo)
        r_value = MCValueRetain(kMCNullTypeInfo);
    else if (p_kind == kMCEncodedValueKindBooleanTypeInfo)
        r_value = MCValueRetain(kMCBooleanTypeInfo);
    else if (p_kind == kMCEncodedValueKindNumberTypeInfo)
        r_value = MCValueRetain(kMCNumberTypeInfo);
    else if (p_kind == kMCEncodedValueKindNameTypeInfo)
        r_value = MCValueRetain(kMCNameTypeInfo);
    else if (p_kind == kMCEncodedValueKindStringTypeInfo)
        r_value = MCValueRetain(kMCStringTypeInfo);
    else if (p_kind == kMCEncodedValueKindDataTypeInfo)
        r_value = MCValueRetain(kMCDataTypeInfo);
    else if (p_kind == kMCEncodedValueKindArrayTypeInfo)
        r_value = MCValueRetain(kMCArrayTypeInfo);
    else if (p_kind == kMCEncodedValueKindProperListTypeInfo)
        r_value = MCValueRetain(kMCProperListTypeInfo);
    else if (p_kind == kMCEncodedValueKindRecordTypeInfo)
    {
        // TODO: Implement record typeinfo reader.
    }
    else if (p_kind == kMCEncodedValueKindHandlerTypeInfo)
    {
        bool t_success;
        t_success = true;
        
        uindex_t t_param_count = 0;
        if (t_success)
            t_success = MCPickleReadCompactUInt(stream, t_param_count);
        
        MCAutoArray<MCHandlerTypeFieldInfo> t_param_info;
        if (t_success)
            t_success = t_param_info . New(t_param_count);
        
        for(uindex_t i = 0; t_success && i < t_param_count; i++)
        {
            uint8_t t_mode;
            t_success = MCStreamReadUInt8(stream, t_mode) &&
                        MCPickleReadTypeInfoRef(stream, t_param_info[i] . type);
            if (t_success)
                t_param_info[i] . mode = (MCHandlerTypeFieldMode)t_mode;
        }
        
        MCAutoTypeInfoRef t_return_type;
        if (t_success)
            t_success = MCPickleReadTypeInfoRef(stream, &t_return_type);
        
        if (t_success)
            t_success = MCHandlerTypeInfoCreate(t_param_info . Ptr(), t_param_info . Size(), *t_return_type, r_value);
        
        for(uindex_t i = 0; i < t_param_count; i++)
        {
            if (t_param_info[i] . type != nil)
                MCValueRelease(t_param_info[i] . type);
        }
        
        if (!t_success)
            return failed();
    }
    else if (p_kind == kMCEncodedValueKindErrorTypeInfo)
    {
        // TODO: Implement error typeinfo reader.
    }
    else
        return failed();
    
    return true;
}

static bool MCPickleReadValueRef(MCStreamRef stream, MCValueRef& r_value)
{
    uint8_t t_kind;
    if (!MCStreamReadUInt8(stream, t_kind))
        return failed();
    
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
                    return failed();
                if (!MCNumberCreateWithInteger(t_value, (MCNumberRef&)r_value))
                    return failed();
            }
            break;
        
        case kMCEncodedValueKindIntegerNegative:
            {
                uint32_t t_value;
                if (!MCPickleReadCompactUInt(stream, t_value))
                    return failed();
                if (!MCNumberCreateWithInteger(-t_value, (MCNumberRef&)r_value))
                    return failed();
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
                    return failed();
                if (!MCNumberCreateWithReal(t_value, (MCNumberRef&)r_value))
                    return failed();
            }
            break;
            
        case kMCEncodedValueKindName:
            if (!MCPickleReadNameRef(stream, (MCNameRef&)r_value))
                return failed();
            break;
            
        case kMCEncodedValueKindString:
            if (!MCPickleReadStringRef(stream, (MCStringRef&)r_value))
                return failed();
            break;
            
        case kMCEncodedValueKindData:
            {
                uindex_t t_size;
                if (!MCPickleReadCompactUInt(stream, t_size))
                    return failed();
                
                uint8_t *t_bytes;
                if (!MCMemoryNewArray(t_size, t_bytes))
                    return failed();
                
                if (!MCStreamRead(stream, t_bytes, t_size) ||
                    !MCDataCreateWithBytesAndRelease(t_bytes, t_size, (MCDataRef&)r_value))
                {
                    free(t_bytes);
                    return failed();
                }
            }
            break;
            
        case kMCEncodedValueKindArray:
            {
                uindex_t t_size;
                if (!MCPickleReadCompactUInt(stream, t_size))
                    return failed();
                
                MCArrayRef t_array;
                if (!MCArrayCreateMutable(t_array))
                    return failed();
                
                while(t_size > 0)
                {
                    MCNewAutoNameRef t_key;
                    MCAutoValueRef t_element;
                    if (!MCPickleReadNameRef(stream, &t_key) ||
                        !MCPickleReadValueRef(stream, &t_element) ||
                        !MCArrayStoreValue(t_array, true, *t_key, *t_element))
                    {
                        MCValueRelease(t_array);
                        return failed();
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
                    return failed();
                
                MCProperListRef t_list;
                if (!MCProperListCreateMutable(t_list))
                    return failed();
                
                while(t_size > 0)
                {
                    MCAutoValueRef t_value;
                    if (!MCPickleReadValueRef(stream, &t_value) ||
                        !MCProperListPushElementOntoBack(t_list, *t_value))
                    {
                        MCValueRelease(t_list);
                        return failed();
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
            if (!MCPickleReadTypeInfoRefContents(stream, t_kind, (MCTypeInfoRef&)r_value))
                return failed();
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
            t_success = MCPickleReadTypeInfoRef(stream, *(MCTypeInfoRef *)p_field_ptr);
            break;
        case kMCPickleFieldTypeArrayOfByte:

            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(uint8_t **)p_field_ptr) &&
                            MCStreamRead(stream, *(void **)p_field_ptr, *(uindex_t *)p_aux_ptr);
            break;
        case kMCPickleFieldTypeArrayOfUIndex:
            
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                        MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(uindex_t **)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleReadCompactUInt(stream, (*(uindex_t **)p_field_ptr)[i]);
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
        case kMCPickleFieldTypeArrayOfTypeInfoRef:
            t_success = MCPickleReadCompactUInt(stream, *(uindex_t *)p_aux_ptr) &&
                            MCMemoryNewArray(*(uindex_t *)p_aux_ptr, *(MCValueRef **)p_field_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleReadTypeInfoRef(stream, (*(MCTypeInfoRef **)p_field_ptr)[i]);
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
                for(int j = 0; t_info -> cases[j] . kind != -1; j++)
	                if (t_kind < INT_MAX &&
	                    int(t_kind) == t_info -> cases[j] . kind)
                    {
                        t_record_info = t_info -> cases[j] . record;
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
    
    if (!t_success)
        return failed();
    
    return t_success;
}

MC_DLLEXPORT_DEF
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
    MCAutoStringRefAsUTF8String t_utf8_string;
    if (!t_utf8_string . Lock(p_value))
        return failed();
    
    return MCPickleWriteCompactUInt(stream, t_utf8_string . Size()) &&
            MCStreamWrite(stream, *t_utf8_string, t_utf8_string . Size());
}

static bool MCPickleWriteTypeInfoRef(MCStreamRef stream, MCTypeInfoRef p_value)
{
    bool t_success;
    t_success = true;
    
    if (MCTypeInfoIsAlias(p_value))
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindAliasTypeInfo);
        if (t_success)
            t_success = MCPickleWriteStringRef(stream, MCNameGetString(MCAliasTypeInfoGetName(p_value)));
        if (t_success)
            t_success = MCPickleWriteTypeInfoRef(stream, MCAliasTypeInfoGetTarget(p_value));
    }
    else if (MCTypeInfoIsNamed(p_value))
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNamedTypeInfo);
        if (t_success)
            t_success = MCPickleWriteStringRef(stream, MCNameGetString(MCAliasTypeInfoGetName(p_value)));
    }
    else if (MCTypeInfoIsOptional(p_value))
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindOptionalTypeInfo);
        if (t_success)
            t_success = MCPickleWriteTypeInfoRef(stream, MCOptionalTypeInfoGetBaseTypeInfo(p_value));
    }
    else if (MCTypeInfoIsRecord(p_value))
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindRecordTypeInfo);
        // TODO: Write out record typeinfo.
    }
    else if (MCTypeInfoIsHandler(p_value))
    {
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindHandlerTypeInfo) &&
                        MCPickleWriteCompactUInt(stream, MCHandlerTypeInfoGetParameterCount(p_value));
        for(uindex_t i = 0; t_success && i < MCHandlerTypeInfoGetParameterCount(p_value); i++)
            t_success = MCStreamWriteUInt8(stream, MCHandlerTypeInfoGetParameterMode(p_value, i)) &&
                            MCPickleWriteTypeInfoRef(stream, MCHandlerTypeInfoGetParameterType(p_value, i));
        if (t_success)
            t_success = MCPickleWriteTypeInfoRef(stream, MCHandlerTypeInfoGetReturnType(p_value));
    }
    else if (MCTypeInfoIsError(p_value))
    {
        if (t_success)
            t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindErrorTypeInfo);
        // TODO: Write out error typeinfo.
    }
    else if (p_value == kMCAnyTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindAnyTypeInfo);
    else if (p_value == kMCNullTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNullTypeInfo);
    else if (p_value == kMCBooleanTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindBooleanTypeInfo);
    else if (p_value == kMCNumberTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNumberTypeInfo);
    else if (p_value == kMCStringTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindStringTypeInfo);
    else if (p_value == kMCNameTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindNameTypeInfo);
    else if (p_value == kMCDataTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindDataTypeInfo);
    else if (p_value == kMCArrayTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindArrayTypeInfo);
    else if (p_value == kMCProperListTypeInfo)
        t_success = MCStreamWriteUInt8(stream, kMCEncodedValueKindProperListTypeInfo);
    
    if (!t_success)
        return failed();
    
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
            break;
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
                t_success = MCPickleWriteStringRef(stream, (MCStringRef)p_value);
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
            return failed();
    }
    
    if (!t_success)
        return failed();
    
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
        case kMCPickleFieldTypeArrayOfUIndex:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWriteCompactUInt(stream, (*(uindex_t **)p_field_ptr)[i]);
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
        case kMCPickleFieldTypeArrayOfTypeInfoRef:
            t_success = MCPickleWriteCompactUInt(stream, *(uindex_t *)p_aux_ptr);
            for(uindex_t i = 0; t_success && i < *(uindex_t *)p_aux_ptr; i++)
                t_success = MCPickleWriteTypeInfoRef(stream, (*(MCTypeInfoRef **)p_field_ptr)[i]);
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
                for(int t_cases = 0; t_info -> cases[t_cases] . kind != -1; t_cases++)
                    if (t_kind == t_info -> cases[t_cases] . kind)
                    {
                        t_record_info = t_info -> cases[t_cases] . record;
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
    
    if (!t_success)
        return failed();
    
    return t_success;
}

MC_DLLEXPORT_DEF
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
        case kMCPickleFieldTypeArrayOfUIndex:
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

                        for(int t_case = 0; t_info -> cases[t_case] . kind != -1; t_case++)
                            if (t_kind == t_info -> cases[t_case] . kind)
                            {
                                MCPickleRelease(t_info -> cases[t_case] . record, t_variant);
                                break;
                            }
                        
                        free(t_variant);
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

MC_DLLEXPORT_DEF
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
