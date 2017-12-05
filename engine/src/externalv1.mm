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

#import "Foundation/Foundation.h"

extern "C"
{
    MCExternalError MCValueFromObjValue(id, MCValueRef&);
    MCExternalError MCArrayToObjcArray(MCArrayRef, NSArray*&);
    MCExternalError MCArrayFromObjcDictionary(NSDictionary*, MCArrayRef&);
    MCExternalError MCArrayFromObjcArray(NSArray*, MCArrayRef&);
    MCExternalError MCValueToObjcValue(MCValueRef, id&);
}

extern "C" MCExternalError MCValueFromObjcValue(id src, MCValueRef &var)
{
    if (src == nil || [src isKindOfClass: [NSNull class]])
    {
        var = MCValueRetain(kMCEmptyString);
    }
    else if ((CFBooleanRef)src == kCFBooleanTrue || (CFBooleanRef)src == kCFBooleanFalse)
    {
        if ((CFBooleanRef)src == kCFBooleanTrue)
            var = MCValueRetain(kMCTrue);
        else
            var = MCValueRetain(kMCFalse);
    }
    else if ([src isKindOfClass: [NSNumber class]])
    {
        CFNumberRef t_number;
        t_number = (CFNumberRef)src;
        
        if (CFNumberIsFloatType(t_number))
        {
            real64_t t_double;
            if (!CFNumberGetValue(t_number, kCFNumberFloat64Type, &t_double))
                return kMCExternalErrorNotANumber;
            else if (!MCNumberCreateWithReal(t_double, (MCNumberRef&)var))
                return kMCExternalErrorNotANumber;
        }
        else
        {
            int32_t t_integer;
            if (!CFNumberGetValue(t_number, kCFNumberIntType, &t_integer))
                return kMCExternalErrorNotANumber;
            else if (!MCNumberCreateWithInteger(t_integer, (MCNumberRef&)var))
                return kMCExternalErrorNotAnInteger;
        }
    }
    else if ([src isKindOfClass: [NSString class]])
    {
        MCAutoStringRef t_string;
        
        if (!MCStringCreateWithCFStringRef((CFStringRef)src, &t_string))
            return kMCExternalErrorNotAString;
        
        var = MCValueRetain(*t_string);
    }
    else if ([src isKindOfClass: [NSData class]])
    {
        MCAutoStringRef t_string;
        CFDataRef t_data;
        t_data = (CFDataRef)src;
        
        if (!MCStringCreateWithBytes(CFDataGetBytePtr(t_data), CFDataGetLength(t_data), kMCStringEncodingNative, false, &t_string))
            return kMCExternalErrorOutOfMemory;
        
        var = MCValueRetain(*t_string);
    }
    else if ([src isKindOfClass: [NSArray class]])
        return MCArrayFromObjcArray((NSArray*)src, (MCArrayRef&)var);
    
    else if ([src isKindOfClass: [NSDictionary class]])
        return MCArrayFromObjcDictionary((NSDictionary*)src, (MCArrayRef&)var);
    else
    {
        MCAutoStringRef t_string;
        NSString *t_as_string;
        t_as_string = [src description];
        if (!MCStringCreateWithCFStringRef((CFStringRef)t_as_string, &t_string))
            return kMCExternalErrorOutOfMemory;
        
        var = MCValueRetain(*t_string);
    }
    
    return kMCExternalErrorNone;
}

// Convert a LiveCode array into an NSArray. The returned NSArray is alloc'd.
extern "C" MCExternalError MCArrayToObjcArray(MCArrayRef src, NSArray*& r_dst)
{
    MCExternalError t_error;
    t_error = kMCExternalErrorNone;
    
    if (t_error == kMCExternalErrorNone)
    {
        bool t_is_sequence;
        t_is_sequence = MCArrayIsSequence(src);
        if (t_error == kMCExternalErrorNone && !t_is_sequence)
            t_error = kMCExternalErrorNotASequence;
    }
    
    uint32_t t_count;
    t_count = 0;
    if (t_error == kMCExternalErrorNone)
        t_count = MCArrayGetCount(src);
    
    id *t_objects;
    t_objects = nil;
    if (t_error == kMCExternalErrorNone)
    {
        t_objects = (id *)calloc(sizeof(id), t_count);
        if (t_objects == nil)
            t_error = kMCExternalErrorOutOfMemory;
    }
    
    uintptr_t t_iterator;
    t_iterator = 0;
    for(uintptr_t i = 0; i < t_count && t_error == kMCExternalErrorNone; ++i)
    {
        // Fetch the key and value.
        MCNameRef t_key;
        MCValueRef t_value;
        
        if (t_error == kMCExternalErrorNone)
        {
            if (!MCArrayIterate(src, t_iterator, t_key, t_value))
                t_error = kMCExternalErrorNotAnArray;
        }
        
        // Now convert the value - remembering that LC sequences are 1 based, and
        // Objc arrays are 0 based. Note that we don't have to validate the key as
        // its guaranteed to be of the correct form as we checked the array was a
        // sequence.
        int4 t_index;
        if (t_error == kMCExternalErrorNone
            && !MCU_strtol(MCNameGetString(t_key), t_index))
            t_error= kMCExternalErrorNotASequence;
        
        if (t_error == kMCExternalErrorNone)
            t_error = MCValueToObjcValue(t_value, t_objects[t_index - 1]);
    }
    
    // If we succeeded, then try to build an NSArray.
    NSArray *t_array;
    if (t_error == kMCExternalErrorNone)
    {
        t_array = [[NSArray alloc] initWithObjects: t_objects count: t_count];
        if (t_array == nil)
            t_error = kMCExternalErrorOutOfMemory;
    }
    
    if (t_error == kMCExternalErrorNone)
        r_dst = t_array;
    
    // We free the objects array since its copied by NSArray.
    for(uint32_t i = 0; i < t_count; i++)
        [t_objects[i] release];
    free(t_objects);
    
    return t_error;
}

extern "C" MCExternalError MCArrayFromObjcArray(NSArray *src, MCArrayRef &r_array)
{
    MCExternalError t_error;
    t_error = kMCExternalErrorNone;
    
    MCAutoArrayRef t_array;
    
    if (!MCArrayCreateMutable(&t_array))
        return kMCExternalErrorOutOfMemory;
    
    for(unsigned int t_index = 0; t_index < [src count] && t_error == kMCExternalErrorNone; t_index++)
    {
        // SN-2014-11-24: [[ Bug 14057 ]] Function rewritten as ExternalVariable are not anymore the values
        //  stored in the arrays.
        MCAutoValueRef t_value;
        if (t_error == kMCExternalErrorNone)
            t_error = MCValueFromObjcValue([src objectAtIndex: t_index], &t_value);
        
        if (t_error == kMCExternalErrorNone)
        {
            if (!MCArrayStoreValueAtIndex(*t_array, t_index + 1, *t_value))
                t_error = kMCExternalErrorOutOfMemory;
        }
    }
    
    if (t_error == kMCExternalErrorNone)
    {
        if (!MCArrayCopy(*t_array, r_array))
            t_error = kMCExternalErrorOutOfMemory;
    }
    
    return t_error;
}

extern "C" MCExternalError MCArrayToObjcDictionary(MCArrayRef p_input, NSDictionary*& r_dst)
{
    MCExternalError t_error;
    t_error = kMCExternalErrorNone;
    
    uint32_t t_count;
    t_count = 0;
    if (t_error == kMCExternalErrorNone)
        t_count = MCArrayGetCount(p_input);
    
    id *t_keys, *t_values;
    t_keys = t_values = nil;
    if (t_error == kMCExternalErrorNone)
    {
        t_keys = (id *)calloc(sizeof(id), t_count);
        t_values = (id *)calloc(sizeof(id), t_count);
        if (t_keys == nil || t_values == nil)
            t_error = kMCExternalErrorOutOfMemory;
    }
    
    uintptr_t t_iterator;
    t_iterator = 0;
    for(uint32_t i = 0; i < t_count && t_error == kMCExternalErrorNone; ++i)
    {
        // Fetch the key and value.
        MCNameRef t_key;
        MCValueRef t_value;
        if (t_error == kMCExternalErrorNone)
            if (!MCArrayIterate(p_input, t_iterator, t_key, t_value))
                t_error = kMCExternalErrorNotAnArray;
        
        // Convert the key.
        if (t_error == kMCExternalErrorNone)
        {
            if (!MCStringConvertToCFStringRef(MCNameGetString(t_key), (CFStringRef&)t_keys[i]))
                t_error = kMCExternalErrorOutOfMemory;
        }
        
        // Now convert the value.
        if (t_error == kMCExternalErrorNone)
            t_error = MCValueToObjcValue(t_value, t_values[i]);
    }
    
    // If we succeeded then build the dictionary.
    NSDictionary *t_dictionary;
    if (t_error == kMCExternalErrorNone)
    {
        t_dictionary = [[NSDictionary alloc] initWithObjects: t_values forKeys: t_keys count: t_count];
        if (t_dictionary == nil)
            t_error = kMCExternalErrorOutOfMemory;
    }
    
    if (t_error == kMCExternalErrorNone)
        r_dst = t_dictionary;
    
    // Only release the created
    for(uint32_t i = 0; i < t_count; i++)
    {
        [t_keys[i] release];
        [t_values[i] release];
    }
    free(t_keys);
    free(t_values);
    
    return t_error;
}

extern "C" MCExternalError MCArrayFromObjcDictionary(NSDictionary *p_src, MCArrayRef &r_array)
{
    MCExternalError t_error;
    t_error = kMCExternalErrorNone;
    
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array))
        return kMCExternalErrorOutOfMemory;
    
    NSAutoreleasePool *t_pool;
    t_pool = [[NSAutoreleasePool alloc] init];
#ifndef __OBJC2__
    NSEnumerator *t_enumerator;
    t_enumerator = [p_src keyEnumerator];
    for(;;)
    {
        id t_key;
        t_key = [t_enumerator nextObject];
        if (t_key == nil)
            break;
#else
        for(id t_key in p_src)
        {
#endif
            if (t_error == kMCExternalErrorNone && ![t_key isKindOfClass: [NSString class]])
                t_error = kMCExternalErrorCannotEncodeMap;
            
            // SN-2014-11-24: [[ Bug 14057 ]] Function rewritten as ExternalVariable are not anymore the values
            //  stored in the arrays.
            MCAutoStringRef t_key_stringref;
            MCNewAutoNameRef t_key_nameref;
            
            if (t_error == kMCExternalErrorNone)
            {
                if (!MCStringCreateWithCFStringRef((CFStringRef)t_key, &t_key_stringref) ||
                    !MCNameCreate(*t_key_stringref, &t_key_nameref))
                    t_error = kMCExternalErrorOutOfMemory;
            }
            
            MCAutoValueRef t_value;
            
            if (t_error == kMCExternalErrorNone)
                t_error = MCValueFromObjcValue([p_src objectForKey: t_key], &t_value);
            
            if (t_error == kMCExternalErrorNone)
            {
                if (!MCArrayStoreValue(*t_array, false, *t_key_nameref, *t_value))
                    t_error = kMCExternalErrorOutOfMemory;
            }
        }
        [t_pool release];
        
        if (t_error == kMCExternalErrorNone
            && !MCArrayCopy(*t_array, r_array))
            t_error = kMCExternalErrorOutOfMemory;
        
        return t_error;
    }

// Convert a LiveCode value into an element of an Objc Array/Dictionary. This
// converts all non-array values to strings, arrays which are sequences to
// NSArray, and arrays which are maps to NSDictionary.
extern "C" MCExternalError MCValueToObjcValue(MCValueRef p_input, id& r_dst)
{
    MCExternalError t_error;
    t_error = kMCExternalErrorNone;
    
    if (MCValueIsEmpty(p_input))
    {
        r_dst = @"";
    }
    else if (MCValueGetTypeCode(p_input) != kMCValueTypeCodeArray)
    {
        MCAutoStringRef t_stringref;
        
        t_error = MCExternalVariable::ConvertToString(p_input, kMCExternalValueOptionDefaultNumberFormat, &t_stringref);
        
        if (t_error != kMCExternalErrorNone)
            return t_error;
        
        if (!MCStringConvertToCFStringRef(*t_stringref, (CFStringRef&)r_dst))
            return kMCExternalErrorOutOfMemory;
    }
    // p_input is an MCArrayRef
    else if (MCArrayIsSequence((MCArrayRef)p_input))
        return MCArrayToObjcArray((MCArrayRef)p_input, (NSArray*&)r_dst);
    else
        return MCArrayToObjcDictionary((MCArrayRef)p_input, (NSDictionary*&)r_dst);
    
    return (MCExternalError)t_error;
}
    
