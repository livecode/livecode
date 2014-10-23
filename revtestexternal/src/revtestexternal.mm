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

#include <cstdio>
#include <cstdarg>

#import <Foundation/Foundation.h>

#include <LiveCode.h>

////////////////////////////////////////////////////////////////////////////////

NSArray *revTestExternalTestObjcArrays(NSArray *array)
{
    [array setValue: @"Hello" forKey: @"greetings"];
    return array;
}

NSDictionary *revTestExternalTestObjcDictionaries(NSDictionary *dictionary)
{
    [dictionary setValue: @"Hello" forKey: @"greetings"];
    return dictionary;
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
