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

#define integer_t cf_integer_t
#include <CoreFoundation/CoreFoundation.h>
#undef integer_t

////////////////////////////////////////////////////////////////////////////////

template<typename T>
static inline void CFDeleter(T type)
{
    if (type == nullptr)
        return;
    CFRelease(type);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCBooleanCreateWithCFBooleanRef(CFBooleanRef p_cf_boolean, MCBooleanRef& r_boolean)
{
    if (p_cf_boolean == kCFBooleanTrue)
    {
        r_boolean = MCValueRetain(kMCTrue);
    }
    else
    {
        r_boolean = MCValueRetain(kMCFalse);
    }
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCBooleanConvertToCFBooleanRef(MCBooleanRef p_boolean, CFBooleanRef& r_boolean_ref)
{
    if (p_boolean == kMCTrue)
    {
        r_boolean_ref = (CFBooleanRef)CFRetain(kCFBooleanTrue);
    }
    else
    {
        r_boolean_ref = (CFBooleanRef)CFRetain(kCFBooleanFalse);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNumberCreateWithCFNumberRef(CFNumberRef p_cf_number, MCNumberRef& r_number)
{
    if (!CFNumberIsFloatType(p_cf_number))
    {
        int64_t t_integer;
        CFNumberGetValue(p_cf_number, kCFNumberSInt64Type, &t_integer);
        if (t_integer >= INTEGER_MIN && t_integer <= INTEGER_MAX)
        {
            return MCNumberCreateWithInteger((integer_t)t_integer, r_number);
        }
        else if (t_integer >= UINTEGER_MIN && t_integer <= UINTEGER_MAX)
        {
            return MCNumberCreateWithUnsignedInteger((uinteger_t)t_integer, r_number);
        }
    }
    
    double t_real;
    CFNumberGetValue(p_cf_number, kCFNumberDoubleType, &t_real);
    return MCNumberCreateWithReal(t_real, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberConvertToCFNumberRef(MCNumberRef p_number, CFNumberRef& r_number_ref)
{
    CFNumberRef t_number_ref;
    if (MCNumberIsInteger(p_number))
    {
        t_number_ref = CFNumberCreate(nullptr, kCFNumberSInt32Type, &p_number->integer);
    }
    else
    {
        t_number_ref = CFNumberCreate(nullptr, kCFNumberDoubleType, &p_number->real);
    }
    
    if (t_number_ref == nullptr)
    {
        return false;
    }
    
    r_number_ref = t_number_ref;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCStringCreateWithCFStringRef(CFStringRef p_cf_string, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;
	
	CFIndex t_string_length;
	t_string_length = CFStringGetLength(p_cf_string);
	
	CFIndex t_buffer_size;
	t_buffer_size = CFStringGetMaximumSizeForEncoding(t_string_length, kCFStringEncodingUnicode) + 1;
	
	MCAutoPointer<byte_t> t_buffer;
	t_success = MCMemoryNewArray(t_buffer_size, &t_buffer);
	
	CFIndex t_used_size;
	if (t_success)
		CFStringGetBytes(p_cf_string, CFRangeMake(0, t_string_length), kCFStringEncodingUnicode, '?', false, (UInt8*)*t_buffer, t_buffer_size, &t_used_size);
	
	if (t_success)
		t_success = MCStringCreateWithChars((unichar_t *)*t_buffer, t_used_size / 2, r_string);
	
	return t_success;
}

MC_DLLEXPORT_DEF
bool MCStringConvertToCFStringRef(MCStringRef p_string, CFStringRef& r_cfstring)
{
    __MCAssertIsString(p_string);
    
    uindex_t t_length;
    unichar_t* t_chars;
    
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, t_chars))
        return false;
    
    MCStringGetChars(p_string, MCRangeMake(0, t_length), t_chars);
    r_cfstring = CFStringCreateWithCharacters(nil, t_chars, t_length);
    
    MCMemoryDeleteArray(t_chars);
    return r_cfstring != nil;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCDataCreateWithCFDataRef(CFDataRef p_cf_data, MCDataRef& r_data)
{
    return MCDataCreateWithBytes(CFDataGetBytePtr(p_cf_data), CFDataGetLength(p_cf_data), r_data);
}

MC_DLLEXPORT_DEF
bool MCDataConvertToCFDataRef(MCDataRef p_data, CFDataRef& r_cf_data)
{
    CFDataRef t_cf_data = CFDataCreate(nil, MCDataGetBytePtr(p_data), MCDataGetLength(p_data));
    if (t_cf_data == nullptr)
    {
        return false;
    }
    r_cf_data = t_cf_data;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCProperListCreateWithCFArrayRef(CFArrayRef p_cf_array, bool p_use_lists, MCProperListRef& r_list)
{
    MCAutoProperListRef t_list;
    if (!MCProperListCreateMutable(&t_list))
    {
        return false;
    }
    
    for(CFIndex t_index = 0; t_index < CFArrayGetCount(p_cf_array); t_index++)
    {
        MCAutoValueRef t_value;
        if (!MCValueCreateWithCFTypeRef(CFArrayGetValueAtIndex(p_cf_array, t_index), p_use_lists, &t_value) ||
            !MCProperListPushElementOntoBack(*t_list, *t_value))
        {
            return false;
        }
    }
    
    if (!t_list.MakeImmutable())
    {
        return false;
    }
    
    r_list = t_list.Take();
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListConvertToCFArrayRef(MCProperListRef p_list, bool p_use_lists, CFArrayRef& r_cf_array)
{
    MCAutoCustomPointer<const void, CFDeleter> t_cf_array =
            CFArrayCreateMutable(nullptr, MCProperListGetLength(p_list), &kCFTypeArrayCallBacks);
    
    if (!t_cf_array)
    {
        return MCErrorThrowOutOfMemory();
    }
    
    for(uindex_t t_index = 0; t_index < MCProperListGetLength(p_list); t_index++)
    {
        MCAutoCustomPointer<const void, CFDeleter> t_cf_value;
        if (!MCValueConvertToCFTypeRef(MCProperListFetchElementAtIndex(p_list, t_index), p_use_lists, &t_cf_value))
        {
            return false;
        }
        CFArraySetValueAtIndex((CFMutableArrayRef)*t_cf_array, t_index, *t_cf_value);
    }
    
    r_cf_array = (CFArrayRef)t_cf_array.Release();
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCProperListCreateWithCFDictionaryRef(CFDictionaryRef p_cf_dictionary, bool p_use_lists, MCProperListRef& r_proper_list)
{
    MCAutoArrayRef t_array;
    if (!MCArrayCreateWithCFDictionaryRef(p_cf_dictionary, p_use_lists, &t_array))
    {
        return false;
    }
    
    return MCArrayConvertToProperList(*t_array, r_proper_list);
}

MC_DLLEXPORT_DEF
bool MCProperListConvertToCFDictionaryRef(MCProperListRef p_list, bool p_use_lists, CFDictionaryRef& r_cf_dictionary)
{
    MCAutoArrayRef t_array;
    if (!MCProperListConvertToArray(p_list, &t_array))
    {
        return false;
    }
    
    return MCArrayConvertToCFDictionaryRef(*t_array, p_use_lists, r_cf_dictionary);
}

////////////////////////////////////////////////////////////////////////////////

struct MCArrayCreateWithCFDictionaryRefContext
{
    bool use_lists;
    MCArrayRef array;
    bool result;
};

static void MCArrayCreateWithCFDictionaryRefApplier(const void *p_key, const void *p_value, void *p_context)
{
    MCArrayCreateWithCFDictionaryRefContext *context =
            static_cast<MCArrayCreateWithCFDictionaryRefContext *>(p_context);
    
    CFTypeRef t_cf_key = (CFTypeRef)p_key;
    CFTypeRef t_cf_value = (CFTypeRef)p_value;
    
    if (p_value == nullptr)
    {
        return;
    }
    
    if (CFGetTypeID(p_key) != CFStringGetTypeID())
    {
        context->result = false;
        return;
    }
    
    MCAutoStringRef t_key_string;
    MCNewAutoNameRef t_key;
    MCAutoValueRef t_value;
    if (!MCStringCreateWithCFStringRef((CFStringRef)t_cf_key, &t_key_string) ||
        !MCNameCreate(*t_key_string, &t_key) ||
        !MCValueCreateWithCFTypeRef(t_cf_value, context->use_lists, &t_value) ||
        !MCArrayStoreValue(context->array, true, *t_key, *t_value))
    {
        context->result = false;
        return;
    }
}

MC_DLLEXPORT_DEF
bool MCArrayCreateWithCFDictionaryRef(CFDictionaryRef p_cf_dict, bool p_use_lists, MCArrayRef& r_array)
{
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array))
    {
        return false;
    }
    
    MCArrayCreateWithCFDictionaryRefContext t_context;
    t_context.use_lists = p_use_lists;
    t_context.array = *t_array;
    t_context.result = true;
    CFDictionaryApplyFunction(p_cf_dict, MCArrayCreateWithCFDictionaryRefApplier, &t_context);
    
    if (!t_context.result)
    {
        return false;
    }
    
    if (!t_array.MakeImmutable())
    {
        return false;
    }
    
    r_array = t_array.Take();
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCArrayConvertToCFDictionaryRef(MCArrayRef p_array, bool p_use_lists, CFDictionaryRef& r_cf_dict)
{
    MCAutoCustomPointer<const void, CFDeleter> t_cf_dict =
            CFDictionaryCreateMutable(nullptr, MCArrayGetCount(p_array),
                                      &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks);
    
    uintptr_t t_iterator = 0;
    MCNameRef t_key = nullptr;
    MCValueRef t_value = nullptr;
    while(MCArrayIterate(p_array, t_iterator, t_key, t_value))
    {
        MCAutoCustomPointer<const void, CFDeleter> t_cf_key;
        if (!MCStringConvertToCFStringRef(MCNameGetString(t_key), (CFStringRef&)t_cf_key))
        {
            return false;
        }
        
        MCAutoCustomPointer<const void, CFDeleter> t_cf_value;
        if (!MCValueConvertToCFTypeRef(t_value, p_use_lists, &t_cf_value))
        {
            return false;
        }
        
        CFDictionarySetValue((CFMutableDictionaryRef)*t_cf_dict, *t_cf_key, *t_cf_value);
    }
    
    r_cf_dict = (CFDictionaryRef)t_cf_dict.Release();
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCArrayCreateWithCFArrayRef(CFArrayRef p_cf_array, bool p_use_lists, MCArrayRef& r_array)
{
    MCAutoProperListRef t_list;
    if (!MCProperListCreateWithCFArrayRef(p_cf_array, p_use_lists, &t_list))
    {
        return false;
    }
    
    return MCProperListConvertToArray(*t_list, r_array);
}

MC_DLLEXPORT_DEF
bool MCArrayConvertToCFArrayRef(MCArrayRef p_array, bool p_use_lists, CFArrayRef& r_cf_array)
{
    MCAutoProperListRef t_list;
    if (!MCArrayConvertToProperList(p_array, &t_list))
    {
        return false;
    }
    
    if (*t_list == nullptr)
    {
        r_cf_array = nullptr;
        return true;
    }
    
    return MCProperListConvertToCFArrayRef(*t_list, p_use_lists, r_cf_array);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCValueCreateWithCFTypeRef(CFTypeRef p_cf_type, bool p_use_lists, MCValueRef& r_value)
{
    CFTypeID t_type_id = CFGetTypeID(p_cf_type);
    if ((CFNullRef)p_cf_type == kCFNull)
    {
        r_value = MCValueRetain(kMCNull);
        return true;
    }
    else if ((CFBooleanRef)p_cf_type == kCFBooleanTrue)
    {
        r_value = MCValueRetain(kMCTrue);
        return true;
    }
    else if ((CFBooleanRef)p_cf_type == kCFBooleanFalse)
    {
        r_value = MCValueRetain(kMCFalse);
        return true;
    }
    else if (t_type_id == CFNumberGetTypeID())
    {
        return MCNumberCreateWithCFNumberRef((CFNumberRef)p_cf_type, (MCNumberRef&)r_value);
    }
    else if (t_type_id == CFDataGetTypeID())
    {
        return MCDataCreateWithCFDataRef((CFDataRef)p_cf_type, (MCDataRef&)r_value);
    }
    else if (t_type_id == CFStringGetTypeID())
    {
        return MCStringCreateWithCFStringRef((CFStringRef)p_cf_type, (MCStringRef&)r_value);
    }
    else if (t_type_id == CFArrayGetTypeID())
    {
        if (p_use_lists)
        {
            return MCProperListCreateWithCFArrayRef((CFArrayRef)p_cf_type, p_use_lists, (MCProperListRef&)r_value);
        }
        return MCArrayCreateWithCFArrayRef((CFArrayRef)p_cf_type, p_use_lists, (MCArrayRef&)r_value);
    }
    else if (t_type_id == CFDictionaryGetTypeID())
    {
        MCAutoArrayRef t_array;
        if (!MCArrayCreateWithCFDictionaryRef((CFDictionaryRef)p_cf_type, p_use_lists, &t_array))
        {
            return false;
        }
        
        MCValueRef t_value;
        if (p_use_lists &&
            MCArrayFetchValueAtIndex(*t_array, 1, t_value))
        {
            MCAutoProperListRef t_maybe_list;
            if (!MCArrayConvertToProperList(*t_array, &t_maybe_list))
            {
                return false;
            }
            if (*t_maybe_list != nullptr)
            {
                r_value = t_maybe_list.Take();
                return true;
            }
        }
        
        r_value = t_array.Take();
        
        return true;
    }
    
    return MCObjcObjectCreateWithId((void *)p_cf_type, (MCObjcObjectRef&)r_value);
}

MC_DLLEXPORT_DEF
bool MCValueConvertToCFTypeRef(MCValueRef p_value, bool p_use_lists, CFTypeRef& r_cf_type)
{
    switch(MCValueGetTypeCode(p_value))
    {
        case kMCValueTypeCodeNull:
            r_cf_type = CFRetain(kCFNull);
            return true;
            
        case kMCValueTypeCodeBoolean:
            return MCBooleanConvertToCFBooleanRef((MCBooleanRef)p_value, (CFBooleanRef&)r_cf_type);
            
        case kMCValueTypeCodeNumber:
            return MCNumberConvertToCFNumberRef((MCNumberRef)p_value, (CFNumberRef&)r_cf_type);
            
        case kMCValueTypeCodeData:
            return MCDataConvertToCFDataRef((MCDataRef)p_value, (CFDataRef&)r_cf_type);
            
        case kMCValueTypeCodeString:
            return MCStringConvertToCFStringRef((MCStringRef)p_value, (CFStringRef&)r_cf_type);
            
        case kMCValueTypeCodeName:
            return MCStringConvertToCFStringRef(MCNameGetString((MCNameRef)p_value), (CFStringRef&)r_cf_type);
                                                
        case kMCValueTypeCodeArray:
            MCValueRef t_value;
            if (p_use_lists &&
                MCArrayFetchValueAtIndex((MCArrayRef)p_value, 1, t_value))
            {
                CFTypeRef t_cf_type;
                if (!MCArrayConvertToCFArrayRef((MCArrayRef)p_value, p_use_lists, (CFArrayRef&)t_cf_type))
                {
                    return false;
                }
                if (t_cf_type != nullptr)
                {
                    r_cf_type = t_cf_type;
                    return true;
                }
            }
            return MCArrayConvertToCFDictionaryRef((MCArrayRef)p_value, p_use_lists, (CFDictionaryRef&)r_cf_type);
            
        case kMCValueTypeCodeProperList:
            return MCProperListConvertToCFArrayRef((MCProperListRef)p_value, p_use_lists, (CFArrayRef&)r_cf_type);
            
        default:
            if (MCValueGetTypeInfo(p_value) == kMCObjcObjectTypeInfo)
            {
                r_cf_type = CFRetain(MCObjcObjectGetId((MCObjcObjectRef)p_value));
                return true;
            }
            break;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
