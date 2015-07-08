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

bool MCHandlerCreate(MCTypeInfoRef p_typeinfo, const MCHandlerCallbacks *p_callbacks, void *p_context, MCHandlerRef& r_handler)
{
    // The context data for an MCHandler is stored after the common elements. The
    // start of this is a field 'context' in the struct so we must adjust for its
    // length.
    __MCHandler *self;
    if (!__MCValueCreate(kMCValueTypeCodeHandler, (sizeof(__MCHandler) - sizeof(self -> context)) + p_callbacks -> size, (__MCValue*&)self))
        return false;
    
    MCMemoryCopy(MCHandlerGetContext(self), p_context, p_callbacks -> size);
    
    self -> typeinfo = MCValueRetain(p_typeinfo);
    self -> callbacks = p_callbacks;
    
    r_handler = self;
    
    return true;
}

bool MCHandlerInvoke(MCHandlerRef self, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    return self -> callbacks -> invoke(MCHandlerGetContext(self), p_arguments, p_argument_count, r_value);
}

MCErrorRef MCHandlerTryToInvokeWithList(MCHandlerRef self, MCProperListRef& x_arguments, MCValueRef& r_value)
{
    MCAutoValueRefArray t_args;
    MCAutoProperListRef t_out_args;
    
    if (!t_args . New(MCProperListGetLength(x_arguments)))
        goto error_exit;
    
    for(uindex_t i = 0; i < MCProperListGetLength(x_arguments); i++)
        t_args[i] = MCValueRetain(MCProperListFetchElementAtIndex(x_arguments, i));
    
    if (!MCHandlerInvoke(self, t_args . Ptr(), t_args . Size(), r_value))
        goto error_exit;
    
    if (!t_args . TakeAsProperList(Out(t_out_args)))
        goto error_exit;
    
    MCValueAssign(x_arguments, t_out_args . Take());
    
    return nil;
    
error_exit:
    MCValueRelease(x_arguments);
    x_arguments = nil;
    r_value = nil;
    
    MCErrorRef t_error;
    if (!MCErrorCatch(t_error))
        return nil;
    
    return t_error;
}

void *MCHandlerGetContext(MCHandlerRef self)
{
    return (void *)self -> context;
}

const MCHandlerCallbacks *MCHandlerGetCallbacks(MCHandlerRef self)
{
    return self -> callbacks;
}

////////////////////////////////////////////////////////////////////////////////

void __MCHandlerDestroy(__MCHandler *self)
{
    if (self -> callbacks -> release != nil)
        self -> callbacks -> release(MCHandlerGetContext(self));
}

hash_t __MCHandlerHash(__MCHandler *self)
{
    return MCHashPointer(self);
}

bool __MCHandlerIsEqualTo(__MCHandler *self, __MCHandler *other_self)
{
    return self == other_self;
}

bool __MCHandlerCopyDescription(__MCHandler *self, MCStringRef& r_desc)
{
	if (NULL != self->callbacks->describe)
		return self->callbacks->describe(MCHandlerGetContext (self), r_desc);

	/* Default implementation. */
	/* FIXME Should include information about arguments and return
	 * values, extracted from the handler's typeinfo. */
	return MCStringCopy(MCSTR("<handler>"), r_desc);
}

////////////////////////////////////////////////////////////////////////////////
