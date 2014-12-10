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

MCTypeInfoRef kMCOutOfMemoryErrorTypeInfo;
MCTypeInfoRef kMCGenericErrorTypeInfo;

static MCErrorRef s_last_error = nil;

static MCErrorRef s_out_of_memory_error = nil;

////////////////////////////////////////////////////////////////////////////////

bool MCErrorCreate(MCTypeInfoRef p_typeinfo, MCArrayRef p_info, MCErrorRef& r_error)
{
    __MCError *self;
    if (!__MCValueCreate(kMCValueTypeCodeError, self))
        return false;
    
    self -> typeinfo = MCValueRetain(p_typeinfo);
    self -> message = MCValueRetain(MCErrorTypeInfoGetMessage(p_typeinfo));
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

bool MCErrorThrowGeneric(void)
{
    MCErrorRef t_error;
    if (!MCErrorCreate(kMCGenericErrorTypeInfo, nil, t_error))
        return false;
    
    MCErrorThrow(t_error);
    MCValueRelease(t_error);
    
    return false;
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
    if (!MCErrorTypeInfoCreate(MCNAME("runtime"), MCSTR("unknown"), &t_ge_typeinfo))
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
