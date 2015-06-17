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

#include <ffi.h>

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
    self -> function_ptr = nil;
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

static void __exec_closure(ffi_cif *cif, void *ret, void **args, void *user_data)
{
    MCHandlerRef t_handler;
    t_handler = (MCHandlerRef)user_data;
    
    MCTypeInfoRef t_signature;
    t_signature = t_handler -> typeinfo;
    
    uindex_t t_arity;
    t_arity = MCHandlerTypeInfoGetParameterCount(t_signature);
    
    MCValueRef t_value_result;
    MCValueRef t_value_args[16];
    uindex_t t_arg_index;
    t_arg_index = 0;
    t_value_result = nil;
    for(t_arg_index = 0; t_arg_index < t_arity; t_arg_index++)
    {
        MCHandlerTypeFieldMode t_mode;
        t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, t_arg_index);
        
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(t_signature, t_arg_index);
        
        if (t_mode != kMCHandlerTypeFieldModeIn)
            abort();
        
        MCResolvedTypeInfo t_resolved_type;
        if (!MCTypeInfoResolve(t_type, t_resolved_type))
        {
            MCErrorThrowGeneric(nil);
            goto error_exit;
        }
        
        if (MCTypeInfoIsForeign(t_resolved_type . type))
        {
            const MCForeignTypeDescriptor *t_descriptor;
            t_descriptor = MCForeignTypeInfoGetDescriptor(t_resolved_type . type);
            if (t_descriptor -> defined != nil &&
                !t_descriptor -> defined(args[t_arg_index]))
                t_value_args[t_arg_index] = MCValueRetain(kMCNull);
            else
            {
                if (t_descriptor -> bridgetype != kMCNullTypeInfo)
                {
                    if (!t_descriptor -> doimport(args[t_arg_index], false, t_value_args[t_arg_index]))
                        goto error_exit;
                }
                else
                {
                    if (!MCForeignValueCreateAndRelease(t_resolved_type . named_type, args[t_arg_index], (MCForeignValueRef&)t_value_args[t_arg_index]))
                        goto error_exit;
                }
            }
        }
        else
        {
            t_value_args[t_arg_index] = MCValueRetain((MCValueRef)args[t_arg_index]);
        }
    }
    
    if (!MCHandlerInvoke(t_handler, t_value_args, t_arity, t_value_result))
        goto error_exit;
    
    MCTypeInfoRef t_return_type;
    t_return_type = MCHandlerTypeInfoGetReturnType(t_signature);
    
    MCResolvedTypeInfo t_resolved_return_type;
    t_return_type = MCHandlerTypeInfoGetReturnType(t_signature);
    if (!MCTypeInfoResolve(t_return_type, t_resolved_return_type))
    {
        MCErrorThrowGeneric(nil);
        goto error_exit;
    }
    
    if (t_resolved_return_type . named_type != kMCNullTypeInfo)
    {
        if (MCTypeInfoIsForeign(t_resolved_return_type . type))
        {
            const MCForeignTypeDescriptor *t_descriptor;
            t_descriptor = MCForeignTypeInfoGetDescriptor(t_resolved_return_type . type);
            if (!t_descriptor -> doexport(t_value_result, true, ret))
                goto error_exit;
        }
        else
        {
            *(MCValueRef *)ret = t_value_result;
            t_value_result = nil;
        }
    }
    
    return;
    
error_exit:
    if (t_value_result != nil)
        MCValueRelease(t_value_result);
    for(uindex_t i = 0; i < t_arg_index; i++)
        MCValueRelease(t_value_args[i]);
}

bool MCHandlerGetFunctionPtr(MCHandlerRef self, void*& r_function_ptr)
{
    if (self -> function_ptr != nil)
    {
        r_function_ptr = self -> function_ptr;
        return true;
    }
    
    ffi_cif *t_cif;
    if (!MCHandlerTypeInfoGetLayoutType(self -> typeinfo, (int)FFI_DEFAULT_ABI, (void*&)t_cif))
        return false;
    
    self -> closure = ffi_closure_alloc(sizeof(ffi_closure), &self -> function_ptr);
    if (self -> closure == nil)
        return MCErrorThrowOutOfMemory();
    
    if (ffi_prep_closure_loc((ffi_closure *)self -> closure, t_cif, __exec_closure, self, self -> function_ptr) != FFI_OK)
    {
        ffi_closure_free(self -> closure);
        self -> closure = nil;
        return MCErrorThrowGeneric(nil);
    }
    
    r_function_ptr = self -> function_ptr;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCHandlerDestroy(__MCHandler *self)
{
    if (self -> callbacks -> release != nil)
        self -> callbacks -> release(MCHandlerGetContext(self));
    if (self -> function_ptr != nil)
        ffi_closure_free(self -> function_ptr);
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
