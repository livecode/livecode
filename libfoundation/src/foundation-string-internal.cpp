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
#include <foundation-unicode.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static bool __MCStringCreateIndirect(__MCString *string, __MCString*& r_string)
{
    MCStringRef self;
    if (!__MCValueCreate(kMCValueTypeCodeString, self))
        return false;
    
    self -> string = MCValueRetain(string);
    self -> flags |= kMCStringFlagIsIndirect | kMCStringFlagIsMutable;
    
    r_string = self;
    return true;
}

// Returns true if the string is indirect.
static bool __MCStringIsIndirect(__MCString *self)
{
    return (self -> flags & kMCStringFlagIsIndirect) != 0;
}

static bool __MCStringMakeImmutable(__MCString *self)
{
    // Shrink the char buffer to be just long enough for the characters plus
    // an implicit NUL.
    if (MCStringIsNative(self))
    {
        if (!MCMemoryResizeArray(self -> char_count + 1, self -> native_chars, self -> char_count))
            return false;
    }
    else
    {
        if (!MCMemoryResizeArray(self -> char_count + 1, self -> chars, self -> char_count))
            return false;
    }
    
    self -> char_count -= 1;
    return true;
}

// Creates an immutable string from this one, changing 'self' to indirect.
static bool __MCStringMakeIndirect(__MCString *self)
{
    // If we are already indirect, there's nothing to do.
	if (__MCStringIsIndirect(self))
		return true;
    
    if (!__MCStringMakeImmutable(self))
        return false;
    
	// Create a new direct string for self to reference
	MCStringRef t_string;
	if (!__MCValueCreate(kMCValueTypeCodeString, t_string))
		return false;
    
	// Share the buffer and assign flags & count
	t_string -> flags |= self -> flags;
    t_string -> flags &= ~kMCStringFlagIsMutable;
	t_string -> char_count = self -> char_count;
    
    if (__MCStringIsNative(self))
        t_string -> native_chars = self -> native_chars;
    else
    {
        t_string -> chars = self -> chars;
        t_string -> flags |= kMCStringFlagIsNotNative;
        // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making a string indirect
        if (__MCStringCanBeNative(self))
            t_string -> flags |= kMCStringFlagCanBeNative;
    }
    
	// 'self' now becomes indirect with a reference to the new string.
	self -> flags |= kMCStringFlagIsIndirect;
	self -> string = t_string;
	return true;
}

// Ensures the given mutable but indirect string is direct.
static bool __MCStringResolveIndirect(__MCString *self)
{
    // Make sure we are indirect.
	MCAssert(__MCStringIsIndirect(self));
    
	// Fetch the reference.
	MCStringRef t_string;
	t_string = self -> string;
    
	// If the string only has a single reference, then re-absorb; otherwise
	// copy.
	if (self -> string -> references == 1)
	{
        self -> char_count = t_string -> char_count;
        self -> capacity = t_string -> capacity;
        self -> flags |= t_string -> flags;
        
        if (__MCStringIsNative(t_string))
            self -> native_chars = t_string -> native_chars;
        else
        {
            self -> chars = t_string -> chars;
            self -> flags |= kMCStringFlagIsNotNative;
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making resolving an indirect string.
            if (__MCStringCanBeNative(t_string))
                self -> flags |= kMCStringFlagCanBeNative;
        }
        
		t_string -> char_count = 0;
		t_string -> chars = nil;
        t_string -> native_chars = nil;
        MCValueRelease(t_string);
	}
	else
	{
        // SN-2015-01-13: [[ Bug 14354 ]] We don't want to release the string we reference
        // before being sure that we can clone its buffer.
        unichar_t *t_chars;
        uint32_t t_char_count;
        
        if (__MCStringIsNative(t_string))
        {
            char_t *t_native_chars;
            if (!__MCStringCloneNativeBuffer(t_string, t_native_chars, t_char_count))
                return false;
            
            t_chars = (unichar_t*)t_native_chars;
        }
        else
        {
            unichar_t *t_uni_chars;
            if (!__MCStringCloneBuffer(t_string, t_chars, t_char_count))
                return false;
            
            self -> flags |= kMCStringFlagIsNotNative;
            
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making resolving an indirect string.
            if (__MCStringCanBeNative(t_string))
                self -> flags |= kMCStringFlagCanBeNative;
        }
        
        // SN-2015-01-13: [[ Bug 14354 ]] We can release now release the string,
        // and then change the value of the string attributes
        MCValueRelease(self -> string);
        
        self -> chars = t_chars;
        self -> char_count = t_char_count;
        
        self -> capacity = t_string -> char_count;
	}
    
	self -> flags &= ~kMCStringFlagIsIndirect;
    
	return true;
}

static bool __MCStringCopyMutable(__MCString *self, __MCString*& r_new_string)
{
    if (!__MCStringMakeImmutable(self))
        return false;
    
    __MCString *t_string;
	t_string = nil;
	
    if (self -> char_count == 0)
    {
        t_string = MCValueRetain(kMCEmptyString);
        MCMemoryDeleteArray(self -> native_chars);
    }
    else
    {
        if (!__MCValueCreate(kMCValueTypeCodeString, t_string))
            return false;
        
        t_string -> char_count = self -> char_count;
        if (__MCStringIsNative(self))
            t_string -> native_chars = self -> native_chars;
        else
        {
            t_string -> chars = self -> chars;
            t_string -> flags |= kMCStringFlagIsNotNative;
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making a new direct string.
            if (__MCStringCanBeNative(self))
                t_string -> flags |= kMCStringFlagCanBeNative;
        }
        t_string -> capacity = 0;
    }
    
    self -> char_count = 0;
    self -> chars = nil;
    self -> native_chars = nil;
    self -> string = MCValueRetain(t_string);
    self -> flags |= kMCStringFlagIsIndirect;
    
    r_new_string = t_string;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
