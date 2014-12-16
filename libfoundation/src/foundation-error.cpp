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

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCOutOfMemoryErrorTypeInfo;
MCTypeInfoRef kMCGenericErrorTypeInfo;

static MCErrorRef s_last_error = nil;

static MCErrorRef s_out_of_memory_error = nil;

////////////////////////////////////////////////////////////////////////////////

static bool MCErrorFormatMessage(MCStringRef p_format, MCArrayRef p_info, MCStringRef& r_message)
{
    MCAutoStringRef t_message;
    if (!MCStringCreateMutable(0, &t_message))
        return false;
    
    uindex_t t_limit;
    t_limit = MCStringGetLength(p_format);
    
    uindex_t t_index;
    t_index = 0;
    while(t_index < t_limit)
    {
        unichar_t t_char;
        t_char = MCStringGetCharAtIndex(p_format, t_index);
        
        // If the sequence is '%{' and there is at least one more char after
        // the '%{' then assume it is a key index.
        if (t_char == '%' &&
            t_index + 2 < t_limit &&
            MCStringGetCharAtIndex(p_format, t_index + 1) == '{')
        {
            MCAutoStringRef t_key_string;
            if (!MCStringCreateMutable(0, &t_key_string))
                return false;
            
            t_index += 2;
            while(t_index < t_limit)
            {
                t_char = MCStringGetCharAtIndex(p_format, t_index);
                if (t_char == '}')
                    break;
                
                if (!MCStringAppendChar(*t_key_string, t_char))
                    return false;
                
                t_index += 1;
            }
            
            // If it is a well-formed %{...} sequence then process it as a key
            // of the info array. Otherwise just append the accumulated string.
            if (t_char == '}')
            {
                MCNewAutoNameRef t_key;
                if (!MCNameCreate(*t_key_string, &t_key))
                    return false;
                
                MCValueRef t_value;
                if (p_info != nil &&
                    MCArrayFetchValue(p_info, false, *t_key, t_value))
                {
                    MCAutoStringRef t_formatted_value;
                    if (!MCStringFormat(&t_formatted_value, "%@", t_value))
                        return false;
                    
                    if (!MCStringAppend(*t_message, *t_formatted_value))
                        return false;
                }
            }
            else
            {
                if (!MCStringAppend(*t_message, *t_key_string))
                    return false;
            }
        }
        else if (t_char == '%' &&
                 t_index + 1 < t_limit &&
                 MCStringGetCharAtIndex(p_format, t_index + 1) == '%')
        {
            if (!MCStringAppendChar(*t_message, '%'))
                return false;
            
            t_index += 1;
        }
        else
        {
            if (!MCStringAppendChar(*t_message, t_char))
                return false;
        }
        
        t_index += 1;
    }
    
    r_message = MCValueRetain(*t_message);
    
    return true;
}

bool MCErrorCreate(MCTypeInfoRef p_typeinfo, MCArrayRef p_info, MCErrorRef& r_error)
{
    __MCError *self;
    if (!__MCValueCreate(kMCValueTypeCodeError, self))
        return false;
    
    if (!MCErrorFormatMessage(MCErrorTypeInfoGetMessage(p_typeinfo), p_info, self -> message))
    {
        MCValueRelease(self);
        return false;
    }
        
    self -> typeinfo = MCValueRetain(p_typeinfo);
    if (p_info != nil)
        self -> info = MCValueRetain(p_info);
    
    self -> target = nil;
    self -> row = 0;
    self -> column = 0;
    
    r_error = self;
    
    return true;
}

bool MCErrorUnwind(MCErrorRef p_error, MCValueRef p_target, uindex_t p_row, uindex_t p_column)
{
    if (p_error -> target != nil)
        return true;
    
    p_error -> target = MCValueRetain(p_target);
    p_error -> row = p_row;
    p_error -> column = p_column;
    
    return true;
}

MCNameRef MCErrorGetDomain(MCErrorRef self)
{
    return MCErrorTypeInfoGetDomain(self -> typeinfo);
}

MCArrayRef MCErrorGetInfo(MCErrorRef self)
{
    return self -> info;
}

MCStringRef MCErrorGetMessage(MCErrorRef self)
{
    return self -> message;
}

uindex_t MCErrorGetDepth(MCErrorRef self)
{
    return self -> target != nil ? 1 : 0;
}

MCValueRef MCErrorGetTargetAtLevel(MCErrorRef self, uindex_t level)
{
    return self -> target;
}

uindex_t MCErrorGetRowAtLevel(MCErrorRef self, uindex_t row)
{
    return self -> row;
}

uindex_t MCErrorGetColumnAtLevel(MCErrorRef self, uindex_t column)
{
    return self -> column;
}

////////////////////////////////////////////////////////////////////////////////

bool MCErrorThrow(MCErrorRef p_error)
{
    if (s_last_error != nil)
        MCValueRelease(s_last_error);
    
    s_last_error = MCValueRetain(p_error);
	
    return false;
}

bool MCErrorCatch(MCErrorRef& r_error)
{
    if (s_last_error == nil)
        return false;
    
    r_error = s_last_error;
    s_last_error = nil;
    
    return true;
}

MCErrorRef MCErrorPeek(void)
{
    return s_last_error;
}

bool MCErrorIsPending(void)
{
    return s_last_error != nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCErrorCreateAndThrow(MCTypeInfoRef p_error_type, ...)
{
    MCAutoArrayRef t_info;
    if (!MCArrayCreateMutable(&t_info))
        return false;
    
    va_list t_args;
    va_start(t_args, p_error_type);
    for(;;)
    {
        const char *t_key;
        t_key = va_arg(t_args, const char *);
        if (t_key == nil)
            break;
        
        MCValueRef t_value;
        t_value = va_arg(t_args, MCValueRef);
        
        // If a value is nil, then it means don't include it.
        if (t_value == nil)
            continue;
        
        MCNewAutoNameRef t_name;
        if (!MCNameCreateWithNativeChars((const char_t *)t_key, strlen(t_key), &t_name))
            return false;
        
        if (!MCArrayStoreValue(*t_info, true, *t_name, t_value))
            return false;
    }
    va_end(t_args);
    
    MCAutoErrorRef t_error;
    if (!MCErrorCreate(p_error_type, *t_info, &t_error))
        return false;
    
    return MCErrorThrow(*t_error);
}

////////////////////////////////////////////////////////////////////////////////

bool MCErrorThrowOutOfMemory(void)
{
    if (s_out_of_memory_error == nil &&
        !MCErrorCreate(kMCOutOfMemoryErrorTypeInfo, nil, s_out_of_memory_error))
    {
        exit(-1);
        return false;
    }
    
    MCErrorThrow(s_out_of_memory_error);
    MCValueRelease(s_out_of_memory_error);
    s_out_of_memory_error = nil;
    
    return false;
}

bool MCErrorThrowGeneric(MCStringRef p_reason)
{
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", p_reason, nil);
}

////////////////////////////////////////////////////////////////////////////////

void __MCErrorDestroy(__MCError *self)
{
    MCValueRelease(self -> typeinfo);
    MCValueRelease(self -> message);
    MCValueRelease(self -> info);
    MCValueRelease(self -> target);
}

hash_t __MCErrorHash(__MCError *self)
{
    return 0;
}

bool __MCErrorIsEqualTo(__MCError *self, __MCError *other_error)
{
    return MCValueIsEqualTo(self -> typeinfo, other_error -> typeinfo);
}

bool __MCErrorCopyDescription(__MCError *self, MCStringRef& r_string)
{
    return false;
}

bool __MCErrorInitialize(void)
{
    MCAutoTypeInfoRef t_oom_typeinfo;
    if (!MCErrorTypeInfoCreate(MCNAME("runtime"), MCSTR("out of memory"), &t_oom_typeinfo))
        return false;
    if (!MCNamedTypeInfoCreate(MCNAME("livecode.lang.OutOfMemoryError"), kMCOutOfMemoryErrorTypeInfo))
        return false;
    if (!MCNamedTypeInfoBind(kMCOutOfMemoryErrorTypeInfo, *t_oom_typeinfo))
        return false;
    
    MCAutoTypeInfoRef t_ge_typeinfo;
    if (!MCErrorTypeInfoCreate(MCNAME("runtime"), MCSTR("%{reason}"), &t_ge_typeinfo))
        return false;
    if (!MCNamedTypeInfoCreate(MCNAME("livecode.lang.GenericError"), kMCGenericErrorTypeInfo))
        return false;
    if (!MCNamedTypeInfoBind(kMCGenericErrorTypeInfo, *t_ge_typeinfo))
        return false;
    
    if (!MCErrorCreate(kMCOutOfMemoryErrorTypeInfo, nil, s_out_of_memory_error))
        return false;
    
    return true;
}

void __MCErrorFinalize(void)
{
    MCValueRelease(s_last_error);
    MCValueRelease(s_out_of_memory_error);
    MCValueRelease(kMCOutOfMemoryErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
