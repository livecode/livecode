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

#ifndef __MC_FOUNDATION_OBJC__
#define __MC_FOUNDATION_OBJC__

#ifndef __MC_FOUNDATION__
#include <foundation.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/NSString.h>

@interface NSString (com_runrev_livecode_foundation_NSStringAdditions)
	+ (NSString *)stringWithMCStringRef: (MCStringRef)value;
    + (NSString *)stringWithMCNameRef: (MCNameRef)value;
@end

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/NSData.h>

@interface NSData (com_runrev_livecode_foundation_NSDataAdditions)
+ (NSData *)dataWithMCDataRef: (MCDataRef)value;
@end

////////////////////////////////////////////////////////////////////////////////

#endif
