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

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCOutOfMemoryErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCGenericErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUnboundTypeErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCUnimplementedErrorTypeInfo;

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

MC_DLLEXPORT_DEF bool
MCErrorCreateWithMessage (MCTypeInfoRef p_typeinfo,
                          MCStringRef p_message,
                          MCArrayRef p_info,
                          MCErrorRef & r_error)
{
	__MCAssertIsErrorTypeInfo(p_typeinfo);
	__MCAssertIsString(p_message);
	if (nil != p_info)
		__MCAssertIsArray(p_info);

    __MCError *self;
    if (!__MCValueCreate(kMCValueTypeCodeError, self))
        return false;
    
    if (!MCErrorFormatMessage(p_message, p_info, self -> message))
    {
        MCValueRelease(self);
        return false;
    }
        
    self -> typeinfo = MCValueRetain(p_typeinfo);
    if (p_info != nil)
        self -> info = MCValueRetain(p_info);
    
    self -> backtrace = nil;
    
    r_error = self;
    
    return true;
}

MC_DLLEXPORT_DEF
bool MCErrorCreateWithMessageV(MCErrorRef& r_error,
							   MCTypeInfoRef p_error_type,
							   MCStringRef p_message,
							   va_list p_args)
{
	__MCAssertIsErrorTypeInfo(p_error_type);
	__MCAssertIsString(p_message);
	MCAutoArrayRef t_info;
	if (!MCArrayCreateMutable(&t_info))
		return false;
	
	for(;;)
	{
		const char *t_key;
		t_key = va_arg(p_args, const char *);
		if (t_key == nil)
			break;
		
		MCValueRef t_value;
		t_value = va_arg(p_args, MCValueRef);
		
		// If a value is nil, then it means don't include it.
		if (t_value == nil)
			continue;
		
		MCNewAutoNameRef t_name;
		if (!MCNameCreateWithNativeChars((const char_t *)t_key,
										 strlen(t_key),
										 &t_name))
			return false;
		
		if (!MCArrayStoreValue(*t_info,
							   true,
							   *t_name,
							   t_value))
		{
			return false;
		}
	}
	
	if (!MCErrorCreateWithMessage(p_error_type,
								  p_message,
								  *t_info,
								  r_error))
	{
		return false;
	}
	
	return true;
}

MC_DLLEXPORT_DEF
bool MCErrorCreateWithMessageS(MCErrorRef& r_error,
							   MCTypeInfoRef p_error_type,
							   MCStringRef p_message,
							   ...)
{
	va_list t_args;
	va_start(t_args, p_message);
	
	bool t_result;
	t_result = MCErrorCreateWithMessageV(r_error,
										 p_error_type,
										 p_message,
										 t_args);
	
	va_end(t_args);
	
	return t_result;
	
}

MC_DLLEXPORT_DEF
bool MCErrorCreate(MCTypeInfoRef p_typeinfo, MCArrayRef p_info, MCErrorRef& r_error)
{
	return MCErrorCreateWithMessage (p_typeinfo,
	                                 MCErrorTypeInfoGetMessage (p_typeinfo),
	                                 p_info,
	                                 r_error);
}

MC_DLLEXPORT_DEF
bool MCErrorCreateV(MCErrorRef& r_error,
					MCTypeInfoRef p_typeinfo,
					va_list p_args)
{
	return MCErrorCreateWithMessageV(r_error,
									 p_typeinfo,
									 MCErrorTypeInfoGetMessage (p_typeinfo),
									 p_args);
}

MC_DLLEXPORT_DEF
bool MCErrorCreateS(MCErrorRef& r_error,
					MCTypeInfoRef p_typeinfo,
					...)
{
	va_list t_args;
	va_start(t_args, p_typeinfo);
	
	bool t_result;
	t_result = MCErrorCreateV(r_error,
							  p_typeinfo,
							  t_args);
	
	va_end(t_args);
	
	return t_result;
}

MC_DLLEXPORT_DEF
bool MCErrorUnwind(MCErrorRef p_error, MCValueRef p_target, uindex_t p_row, uindex_t p_column)
{
	__MCAssertIsError(p_error);
    MCErrorFrame *t_frame;
    if (!MCMemoryNew(t_frame))
        return false;
    
    t_frame -> caller = nil;
    t_frame -> target = MCValueRetain(p_target);
    t_frame -> row = p_row;
    t_frame -> column = p_column;
    
    if (p_error -> backtrace == nil)
        p_error -> backtrace = t_frame;
    else
    {
        MCErrorFrame *t_other_frame;
        for(t_other_frame = p_error -> backtrace; t_other_frame -> caller != nil; t_other_frame = t_other_frame -> caller)
            ;
        t_other_frame -> caller = t_frame;
    }
    
    return true;
}

MC_DLLEXPORT_DEF
MCNameRef MCErrorGetDomain(MCErrorRef self)
{
	__MCAssertIsError(self);
    return MCErrorTypeInfoGetDomain(self -> typeinfo);
}

MC_DLLEXPORT_DEF
MCArrayRef MCErrorGetInfo(MCErrorRef self)
{
	__MCAssertIsError(self);
    return self -> info;
}

MC_DLLEXPORT_DEF
MCStringRef MCErrorGetMessage(MCErrorRef self)
{
	__MCAssertIsError(self);
    return self -> message;
}

MC_DLLEXPORT_DEF
uindex_t MCErrorGetDepth(MCErrorRef self)
{
	__MCAssertIsError(self);

    if (self -> backtrace == nil)
        return 0;
    
    uindex_t t_depth;
    t_depth = 0;
    for(MCErrorFrame *t_frame = self -> backtrace; t_frame != nil; t_frame = t_frame -> caller)
        t_depth += 1;
    
    return t_depth;
}

////////////////////////////////////////////////////////////////////////////////

static MCErrorFrame *__MCErrorGetFrameAtLevel(MCErrorRef self, uindex_t p_level)
{
    MCErrorFrame *t_frame;
    for(t_frame = self -> backtrace; t_frame != nil && p_level != 0; t_frame = t_frame -> caller)
        p_level -= 1;
    
    if (p_level != 0)
        return nil;
    
    return t_frame;
}

MC_DLLEXPORT_DEF
MCValueRef MCErrorGetTargetAtLevel(MCErrorRef self, uindex_t p_level)
{
	__MCAssertIsError(self);

    MCErrorFrame *t_frame;
    t_frame = __MCErrorGetFrameAtLevel(self, p_level);
    if (t_frame == nil)
        return nil;
    
    return t_frame -> target;
}

MC_DLLEXPORT_DEF
uindex_t MCErrorGetRowAtLevel(MCErrorRef self, uindex_t p_level)
{
	__MCAssertIsError(self);

    MCErrorFrame *t_frame;
    t_frame = __MCErrorGetFrameAtLevel(self, p_level);
    if (t_frame == nil)
        return 0;
    
    return t_frame -> row;
}

MC_DLLEXPORT_DEF
uindex_t MCErrorGetColumnAtLevel(MCErrorRef self, uindex_t p_level)
{
	__MCAssertIsError(self);

    MCErrorFrame *t_frame;
    t_frame = __MCErrorGetFrameAtLevel(self, p_level);
    if (t_frame == nil)
        return 0;
    
    return t_frame -> column;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCErrorThrow(MCErrorRef p_error)
{
	__MCAssertIsError(p_error);

    if (s_last_error != nil)
        MCValueRelease(s_last_error);
    
    s_last_error = MCValueRetain(p_error);
	
    return false;
}

MC_DLLEXPORT_DEF
bool MCErrorCatch(MCErrorRef& r_error)
{
    if (s_last_error == nil)
        return false;
    
    r_error = s_last_error;
    s_last_error = nil;
    
    return true;
}

MC_DLLEXPORT_DEF
void MCErrorReset(void)
{
    MCAutoErrorRef t_error;
    MCErrorCatch(&t_error);
}

MC_DLLEXPORT_DEF
MCErrorRef MCErrorPeek(void)
{
    return s_last_error;
}

MC_DLLEXPORT_DEF
bool MCErrorIsPending(void)
{
    return s_last_error != nil;
}

////////////////////////////////////////////////////////////////////////////////

static bool
MCErrorCreateAndThrowWithMessageV (MCTypeInfoRef p_error_type,
                                   MCStringRef p_message,
                                   va_list p_args)
{
	
	MCAutoErrorRef t_error;
	if (!MCErrorCreateWithMessageV(&t_error,
								   p_error_type,
								   p_message,
								   p_args))
	{
		return false;
	}
	
    return MCErrorThrow(*t_error);
}

MC_DLLEXPORT_DEF bool
MCErrorCreateAndThrowWithMessage (MCTypeInfoRef p_error_type,
                                  MCStringRef p_message,
                                  ...)
{
	va_list t_args;
	va_start (t_args, p_message);

	bool t_result;
	t_result = MCErrorCreateAndThrowWithMessageV (p_error_type,
	                                              p_message,
	                                              t_args);

	va_end (t_args);
	return t_result;
}

MC_DLLEXPORT_DEF bool
MCErrorCreateAndThrow (MCTypeInfoRef p_error_type, ...)
{
	va_list t_args;
	va_start (t_args, p_error_type);

	bool t_result;
	t_result = MCErrorCreateAndThrowWithMessageV (p_error_type,
	                                              MCErrorTypeInfoGetMessage (p_error_type),
	                                              t_args);

	va_end (t_args);
	return t_result;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCErrorThrowOutOfMemory(void)
{
    if (s_out_of_memory_error == nil)
    {
	    /* This function may be being called from within
	     * MCMemoryNew().  If there is no error structure already
	     * allocated, calling MCErrorCreate() to obtain one will call
	     * MCMemoryNew()... which will re-enter this function,
	     * probably recursively until the stack overflows. */
	    abort();
    }
    
    return MCErrorThrow(s_out_of_memory_error);
}

MC_DLLEXPORT_DEF
bool MCErrorThrowUnboundType(MCTypeInfoRef p_type)
{
	__MCAssertIsTypeInfo(p_type);
    return MCErrorCreateAndThrow(kMCUnboundTypeErrorTypeInfo, "type", MCNamedTypeInfoGetName(p_type), nil);
}

MC_DLLEXPORT_DEF
bool MCErrorThrowUnimplemented(MCStringRef p_reason)
{
	__MCAssertIsString(p_reason);
    return MCErrorCreateAndThrow(kMCUnimplementedErrorTypeInfo, "reason", p_reason, nil);
}

MC_DLLEXPORT_DEF
bool MCErrorThrowGeneric(MCStringRef p_reason)
{
	__MCAssertIsString(p_reason);
    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", p_reason, nil);
}

MC_DLLEXPORT_DEF
bool MCErrorThrowGenericWithMessage(MCStringRef p_message, ...)
{
    va_list t_args;
    va_start(t_args, p_message);
    
    bool t_success;
    t_success = MCErrorCreateAndThrowWithMessageV(kMCGenericErrorTypeInfo, p_message, t_args);
    
    va_end(t_args);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void __MCErrorDestroy(__MCError *self)
{
    MCValueRelease(self -> typeinfo);
    MCValueRelease(self -> message);
    MCValueRelease(self -> info);
    while(self -> backtrace != nil)
    {
        MCErrorFrame *t_frame;
        t_frame = self -> backtrace;
        self -> backtrace = self -> backtrace -> caller;
        
        MCValueRelease(t_frame -> target);
        MCMemoryDelete(t_frame);
    }
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
	return MCStringCopy (MCErrorGetMessage (self), r_string);
}

bool __MCErrorInitialize(void)
{
	if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.OutOfMemoryError"), MCNAME("runtime"), MCSTR("out of memory"), kMCOutOfMemoryErrorTypeInfo))
        return false;
	
	if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.GenericError"), MCNAME("runtime"), MCSTR("%{reason}"), kMCGenericErrorTypeInfo))
        return false;
	
	if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.UnboundTypeError"), MCNAME("runtime"), MCSTR("attempt to use unbound named type %{type}"), kMCUnboundTypeErrorTypeInfo))
        return false;
    
	if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.UnimplementedError"), MCNAME("runtime"), MCSTR("%{reason}"), kMCUnimplementedErrorTypeInfo))
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
    MCValueRelease(kMCGenericErrorTypeInfo);
    MCValueRelease(kMCUnboundTypeErrorTypeInfo);
    MCValueRelease(kMCUnimplementedErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////
