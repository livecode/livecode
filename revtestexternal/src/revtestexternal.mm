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
    return array;
}

NSDictionary *revTestExternalTestObjcDictionaries(NSDictionary *dictionary)
{
    return dictionary;
}

NSNumber * revtestExternalTestObjcNumber(NSNumber *number)
{
    double t_double;
    t_double = [number doubleValue];
    
    t_double += 3.14;
    
    NSNumber * t_nsnumber;
    [t_nsnumber initWithDouble:t_double];
    return t_nsnumber;
}

NSData* revTestExternalTestObbjcData(NSData* data)
{
    return data;
}

////////////////////////////////////////////////////////////////////////////////
