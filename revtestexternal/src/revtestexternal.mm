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

#include <cstdio>
#include <cstdarg>

#import <Foundation/Foundation.h>

#include <LiveCode.h>

////////////////////////////////////////////////////////////////////////////////

NSArray *revTestExternalTestObjcArrays(NSArray *array)
{
    NSMutableArray *t_array = [array mutableCopy];
    
    [t_array addObject:@"Hello"];
    return array;
}

NSDictionary *revTestExternalTestObjcDictionaries(NSDictionary *dictionary)
{
    id t_content;
    t_content = [dictionary objectForKey: @"changeThisValue"];
    
    NSMutableDictionary *t_dictionary = [dictionary mutableCopy];
    if (t_content != nil)
        [t_dictionary setValue:@"Value Changed!" forKey:@"changeThisValue"];
    
    [t_dictionary setValue: @"Hello" forKey: @"greetings"];
    return t_dictionary;
}

NSDictionary *revTestExternalTestSameDictionary(NSDictionary *dictionary)
{
    return dictionary;
}

NSDictionary *revTestExternalTestArrayToDictionaryCopy(NSDictionary *p_array)
{
    char** t_keys = (char**)malloc(1 * sizeof(char*));
    
    LCArrayRef t_array;
    LCArrayRef t_innerarray;
    LCArrayCreate(kLCValueOptionCaseSensitiveFalse, &t_array);
    LCArrayCreate(kLCValueOptionCaseSensitiveFalse, &t_innerarray);
    
    char *t_value = (char*)malloc(7);
    
    strcpy(t_value, "hidden");
    t_value[6] = 0;
    
    LCArrayStoreKey(t_innerarray, kLCValueOptionAsCString, "state", &(t_value));
    LCArrayStoreKey(t_array, kLCValueOptionAsLCArray, "innerValue", &t_innerarray);
    
    LCArrayListKeys(t_array, 0, t_keys, 2);
    
    NSMutableDictionary *t_dictionary = [p_array mutableCopy];
    
    for (uint32_t i = 0; i < 1; ++i)
    {
        bool t_exist;
        char *t_value;
        LCArrayLookupKey(t_array, kLCValueOptionAsCString, t_keys[i], &t_exist, &t_value);
        
        NSString* t_value_nsstring = [[NSString alloc] initWithCString:t_value encoding:NSMacOSRomanStringEncoding];
        NSString* t_key_nsstring = [[NSString alloc] initWithCString:t_keys[i] encoding:NSMacOSRomanStringEncoding];
        
        [t_dictionary setValue:t_value_nsstring forKey:t_key_nsstring];
        
        [t_value_nsstring release];
        [t_key_nsstring release];
    }
    
    free(t_value);
    free(t_keys);
    return revTestExternalTestObjcDictionaries(t_dictionary);
}


NSNumber * revTestExternalTestObjcNumber(NSNumber *number)
{
    double t_double;
    t_double = [number doubleValue];
    
    t_double += 3.14;
    
    return [NSNumber numberWithDouble: t_double];
}

NSData* revTestExternalTestObjcData(NSData* data)
{
    char* t_bytes;
    uint32_t t_length;
    
    fprintf(stderr, "revTestExternalTestObjcData\n");
    
    t_length = [data length];
    t_bytes  = (char*)malloc(t_length);
    
    fprintf(stderr, "\tlength: %u\n", t_length);
    [data getBytes:t_bytes length:t_length];
    
    for (uint32_t i = 0; i < t_length; ++i)
        t_bytes[i]++;
    
    return [NSData dataWithBytes:t_bytes length:t_length];
}

NSString* revTestExternalTestObjcString(NSString* string)
{
    return [string stringByAppendingString: string];
}

////////////////////////////////////////////////////////////////////////////////
