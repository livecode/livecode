/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

#include "libscript/script.h"
#include "script-private.h"

#include "ffi.h"

#include "foundation-auto.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#include <objc/message.h>
#endif

#include "script-execute.hpp"

typedef void (*__MCScriptValueDrop)(void *);

static void __MCScriptDropValueRef(void *p_value)
{
	MCValueRelease(*(MCValueRef *)p_value);
}

class MCScriptForeignInvocation
{
public:
    struct CallTrampolineContext
    {
        MCScriptForeignInvocation* invocation;
        MCScriptForeignHandlerDefinition *handler;
        MCTypeInfoRef signature;
        void *result_slot_ptr;
        bool return_value;
    };
    
    /**/

	MCScriptForeignInvocation(void)
		: m_argument_count(0),
		  m_storage_frontier(0)
	{
	}
	
	~MCScriptForeignInvocation(void)
	{
		// Drop any slots which have a drop method (those whose
		// value have been taken will have had their drop reset
		// to nil).
		for(uindex_t i = 0; i < m_argument_count; i++)
		{
			if (m_argument_drops[i] != nil)
			{
				m_argument_drops[i](m_argument_slots[i]);
			}
		}
	}
	
	// Allocate temporary storage for the invocation
	bool Allocate(size_t p_amount,
				  size_t p_align,
				  void*& r_ptr)
	{
		// Compute any padding needed for alignment (t_align_delta will
		// be the number of pad bytes).
		size_t t_align_delta;
		t_align_delta = p_align - (m_storage_frontier % p_align);
		
		// If there is not enough space then throw.
		if (sizeof(m_stack_storage) - m_storage_frontier < p_amount + t_align_delta)
		{
			return MCErrorThrowOutOfMemory();
		}
		
		// Round the frontier up to the required alignment.
		m_storage_frontier += t_align_delta;
		
		// Compute the returned pointer into the actual storage array
		r_ptr = m_stack_storage + m_storage_frontier;
		
		// Bump the frontier by the amount allocated.
		m_storage_frontier += p_amount;
		
		return true;
	}
	
	// Append a non-reference argument - this takes contents_ptr
	bool Argument(void *p_slot_ptr,
				  __MCScriptValueDrop p_slot_drop,
                  ffi_type* p_type)
	{
		if (m_argument_count >= kMaxArguments)
		{
			return MCErrorThrowOutOfMemory();
		}
		
		// For normal arguments, the slot is passed directly.
		m_argument_values[m_argument_count] = p_slot_ptr;
		
		// Store the slot ptr and its drop.
		m_argument_slots[m_argument_count] = p_slot_ptr;
		m_argument_drops[m_argument_count] = p_slot_drop;
        
        // Store the ffi_type.
        m_argument_types[m_argument_count] = p_type;
		
		// Bump the number of arguments
		m_argument_count++;
		
		return true;
	}
	
	// Append a reference argument - this takes contents_ptr
	bool ReferenceArgument(void *p_slot_ptr,
						   __MCScriptValueDrop p_slot_drop)
	{
		if (m_argument_count >= kMaxArguments ||
			!Allocate(sizeof(void *),
					  alignof(void *),
					  m_argument_values[m_argument_count]))
		{
			return MCErrorThrowOutOfMemory();
		}
		
		// Store the slot into the reference slot we just created.
		*(void **)m_argument_values[m_argument_count] = p_slot_ptr;
		
		// Store the slot ptr and its drop.
		m_argument_slots[m_argument_count] = p_slot_ptr;
		m_argument_drops[m_argument_count] = p_slot_drop;
        
        // Reference arguments always have type ffi_pointer.
        m_argument_types[m_argument_count] = &ffi_type_pointer;
		
		// Bump the number of arguments
		m_argument_count++;
		
		return true;
	}
	
    /**/
    
    bool CallJava(MCScriptForeignHandlerDefinition *p_handler,
                  MCTypeInfoRef p_handler_signature,
                  void *p_result_slot_ptr)
    {
        if (p_handler->thread_affinity != kMCScriptThreadAffinityDefault)
        {
#if defined(TARGET_SUBPLATFORM_ANDROID) && !defined(CROSS_COMPILE_HOST)
            extern void *MCAndroidGetSystemJavaEnv();
            extern bool MCJavaPrivateCallJNIMethodOnEnv(void *p_env, MCNameRef p_class, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count);
            
            return MCJavaPrivateCallJNIMethodOnEnv(MCAndroidGetSystemJavaEnv(),
                                                   p_handler->java.class_name,
                                                   p_handler->java.method_id,
                                                   p_handler->java.call_type,
                                                   p_handler_signature,
                                                   p_result_slot_ptr,
                                                   m_argument_values,
                                                   m_argument_count);
#endif
        }
        
        return MCJavaCallJNIMethod(p_handler->java.class_name,
                                   p_handler->java.method_id,
                                   p_handler->java.call_type,
                                   p_handler_signature,
                                   p_result_slot_ptr,
                                   m_argument_values,
                                   m_argument_count);
    }
    
    /**/
    
    bool CallC(MCScriptForeignHandlerDefinition *p_handler,
			   MCTypeInfoRef p_handler_signature,
			   void *p_result_slot_ptr)
    {
        MCAssert(p_handler->language == kMCScriptForeignHandlerLanguageC);
        
        if (!MCHandlerTypeInfoIsVariadic(p_handler_signature))
        {
            ffi_call((ffi_cif *)p_handler ->c.function_cif,
                     (void(*)())p_handler ->c.function,
                     p_result_slot_ptr,
                     m_argument_values);
        }
        else
        {
            ffi_cif *t_fixed_cif = (ffi_cif *)p_handler->c.function_cif;
            ffi_cif t_cif;
            if (ffi_prep_cif_var(&t_cif,
                                 t_fixed_cif->abi,
                                 t_fixed_cif->nargs,
                                 m_argument_count,
                                 t_fixed_cif->rtype,
                                 m_argument_types) != FFI_OK)
            {
                return MCErrorThrowGeneric(MCSTR("unexpected libffi failure"));
            }
            
            ffi_call(&t_cif,
                     (void(*)())p_handler ->c.function,
                     p_result_slot_ptr,
                     m_argument_values);
        }
        
        return true;
    }

    /**/
    
    bool CallObjC(MCScriptForeignHandlerDefinition *p_handler,
                  MCTypeInfoRef p_handler_signature,
                  void *p_result_slot_ptr)
    {
#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
        /* At this point we have a argument values array which will match
         * the LCB signature - depending on the call type we need to modify
         * it. */
        void *t_objc_values[kMaxArguments];
        if (p_handler->objc.call_type == kMCScriptForeignHandlerObjcCallTypeInstanceMethod)
        {
            /* In the case of an instance method call, we need to insert
             * the method pointer as the second argument. */
            if (m_argument_count + 1 >= kMaxArguments)
            {
                return MCErrorThrowOutOfMemory();
            }
            t_objc_values[0] = m_argument_values[0];
            t_objc_values[1] = &p_handler->objc.objc_selector;
            for(uindex_t i = 1; i < m_argument_count; i++)
                t_objc_values[i + 1] = m_argument_values[i];
        }
        else if (p_handler->objc.call_type == kMCScriptForeignHandlerObjcCallTypeClassMethod)
        {
            /* In the case of a class method call, we need to insert both
             * the class, and method pointer as the first two arguments. */
            if (m_argument_count + 2 >= kMaxArguments)
            {
                return MCErrorThrowOutOfMemory();
            }
            
            t_objc_values[0] = &p_handler->objc.objc_class;
            t_objc_values[1] = &p_handler->objc.objc_selector;
            for(uindex_t i = 0; i < m_argument_count; i++)
                t_objc_values[i + 2] = m_argument_values[i];
        }
        
        /* There are different variants of objc_msgSend we need to use
         * depending on architecture and return type:
         *   struct return
         *     arm64: only has objc_msgSend
         *     arm: _stret if struct size > 4 bytes
         *     x86-64: _stret if struct size > 16 bytes
         *     i386: _stret if struct size >= 8 bytes or
         *           exactly 1, 2, or 4 bytes
         *   float return
         *     arm: no difference
         *     i386: _fpret
         *     x86-64: no difference
         *   double return
         *     arm: no difference
         *     i386: _fpret
         *     x86-64: no difference
         *   long double
         *     arm: no difference
         *     i386: _fpret
         *     x86-64: _fpret
         *   _Complex long double (TODO)
         *     arm: no difference
         *     i386: no difference
         *     x86-64: _fp2ret
         */
         
        ffi_cif *t_cif = (ffi_cif *)p_handler->objc.function_cif;
        void (*t_objc_msgSend)() = nullptr;
        size_t t_rsize = t_cif->rtype->size;
#if defined(__ARM64__)
        t_objc_msgSend = (void(*)())objc_msgSend;
#elif defined(__ARM__)
        if (t_cif->rtype->type == FFI_TYPE_STRUCT &&
            t_rsize > 4)
            t_objc_msgSend = (void(*)())objc_msgSend_stret;
        else
            t_objc_msgSend = (void(*)())objc_msgSend;
#elif defined(__X86_64__)
        /* TODO: Investigate structs containing long doubles
         * as they will not work at the moment */
        if (t_cif->rtype->type == FFI_TYPE_STRUCT &&
            t_rsize > 16)
            t_objc_msgSend = (void(*)())objc_msgSend_stret;
        else if (t_cif->rtype->type == FFI_TYPE_LONGDOUBLE)
            t_objc_msgSend = (void(*)())objc_msgSend_fpret;
        else
            t_objc_msgSend = (void(*)())objc_msgSend;
#elif defined(__i386__)
        if (t_cif->rtype->type == FFI_TYPE_STRUCT &&
            (t_rsize >= 8 ||
             (t_rsize & (t_rsize - 1)) == 0))
            t_objc_msgSend = (void(*)())objc_msgSend_stret;
        else if (t_cif->rtype->type == FFI_TYPE_FLOAT ||
                 t_cif->rtype->type == FFI_TYPE_DOUBLE ||
                 t_cif->rtype->type == FFI_TYPE_LONGDOUBLE)
            t_objc_msgSend = (void(*)())objc_msgSend_fpret;
        else
            t_objc_msgSend = (void(*)())objc_msgSend;
#else
#error unknown architecture for objc_msgSend
#endif
        /* We must use a wrapper function written in an obj-c++ source file so that
         * we can capture any obj-c exceptions. */
        extern bool MCScriptCallObjCCatchingErrors(ffi_cif*, void (*)(), void *, void **);
        return MCScriptCallObjCCatchingErrors(t_cif,
                                              t_objc_msgSend,
                                              p_result_slot_ptr,
                                              t_objc_values);
#else
        return true;
#endif
    }

    bool CallAny(MCScriptForeignHandlerDefinition *p_handler,
                 MCTypeInfoRef p_handler_signature,
                 void *p_result_slot_ptr)
    {
        switch(p_handler->language)
        {
            case kMCScriptForeignHandlerLanguageJava:
            
                return CallJava(p_handler, p_handler_signature, p_result_slot_ptr);
            
            case kMCScriptForeignHandlerLanguageC:
                return CallC(p_handler, p_handler_signature, p_result_slot_ptr);
                
            case kMCScriptForeignHandlerLanguageBuiltinC:
                ((void(*)(void*, void**))p_handler->builtin_c.function)(p_result_slot_ptr,
                                                                        m_argument_values);
                return true;
                
            case kMCScriptForeignHandlerLanguageObjC:
                return CallObjC(p_handler, p_handler_signature, p_result_slot_ptr);
            
            case kMCScriptForeignHandlerLanguageUnknown:
                MCUnreachableReturn(false);
        }
        
        return false;
    }
    
    static void CallAnyTrampoline(void *p_context)
    {
        auto ctxt = static_cast<CallTrampolineContext *>(p_context);
        ctxt->return_value = ctxt->invocation->CallAny(ctxt->handler,
                                                       ctxt->signature,
                                                       ctxt->result_slot_ptr);
    }
    
	// Call the foreign handler, taking into account which thread it must run
    // on.
	bool Call(MCScriptForeignHandlerDefinition *p_handler,
			  MCTypeInfoRef p_handler_signature,
			  void *p_result_slot_ptr)
    {
        /* If the handler has a thread affinity which is non-default, then on split
         * thread engines (android / ios) we must do the actual call on the other
         * thread. */
        if (p_handler->thread_affinity != kMCScriptThreadAffinityDefault)
        {
#if defined(TARGET_SUBPLATFORM_IPHONE) && !defined(CROSS_COMPILE_HOST)
            extern void MCIPhoneRunOnMainFiber(void (*)(void *), void *);
            CallTrampolineContext t_context = { this, p_handler, p_handler_signature, p_result_slot_ptr, true };
            MCIPhoneRunOnMainFiber(CallAnyTrampoline, &t_context);
            return t_context.return_value;
#elif defined(TARGET_SUBPLATFORM_ANDROID) && !defined(CROSS_COMPILE_HOST)
            typedef void (*co_yield_callback_t)(void *);
            extern void co_yield_to_android_and_call(co_yield_callback_t callback, void *context);
            CallTrampolineContext t_context = { this, p_handler, p_handler_signature, p_result_slot_ptr, true };
            co_yield_to_android_and_call(CallAnyTrampoline, &t_context);
            return t_context.return_value;
#endif
        }
        
        /* Either thread affinity is default, or the platform does not have split
         * threads. */
        return CallAny(p_handler,
                       p_handler_signature,
                       p_result_slot_ptr);
    }
    
	// Take the contents of a slot - this stops the invocation
	// cleaning up the taken slot.
	void *TakeArgument(uindex_t p_index)
	{
		void *t_slot_ptr;
		t_slot_ptr = m_argument_slots[p_index];
		m_argument_slots[p_index] = nil;
		m_argument_drops[p_index] = nil;
		return t_slot_ptr;
	}
	
private:
	enum
	{
		kMaxArguments = 32,
		kMaxStorage = 4096,
	};
	
	// The number of arguments currently accumulated.
	uindex_t m_argument_count;
	// The array of pointers which will be passed to libffi - this will
	// be the pointers in m_argument_slots for normal parameters and
	// a pointer to the pointers in m_argument_slots for reference
	// parameters.
	void *m_argument_values[kMaxArguments];
	// The drops for the slot values in m_argument_slots.
	__MCScriptValueDrop m_argument_drops[kMaxArguments];
	// The actual values in the slots (not indirected for references).
	void *m_argument_slots[kMaxArguments];
    // The ffi_types of the arguments (used for var-args)
    ffi_type *m_argument_types[kMaxArguments];
	
	size_t m_storage_frontier;
	char m_stack_storage[kMaxStorage];
};

inline void
__MCScriptComputeSlotAttributes(MCTypeInfoRef p_slot_type,
								size_t& r_slot_size,
								size_t& r_slot_align,
								__MCScriptValueDrop& r_slot_drop)
{
	// If the slot is foreign then we interrogate the foreign descriptor.
	if (MCTypeInfoIsForeign(p_slot_type))
	{
		const MCForeignTypeDescriptor *t_desc =
				MCForeignTypeInfoGetDescriptor(p_slot_type);
		
		r_slot_size = t_desc->size;
		r_slot_align = t_desc->size;
		r_slot_drop = t_desc->finalize;
		
		return;
	}
	
	// If the slot is a foreign handler then it is pointer sized.
	if (MCTypeInfoIsHandler(p_slot_type) &&
		MCHandlerTypeInfoIsForeign(p_slot_type))
	{
		r_slot_size = sizeof(void*);
		r_slot_align = alignof(void*);
		r_slot_drop = nil;
		return;
	}

	r_slot_size = sizeof(void*);
	r_slot_align = alignof(void*);
	r_slot_drop = __MCScriptDropValueRef;
}

bool
MCScriptExecuteContext::InvokeForeignVarArgument(MCScriptForeignInvocation& p_invocation,
                                                 MCScriptInstanceRef p_instance,
                                                 MCScriptForeignHandlerDefinition *p_handler_def,
                                                 uindex_t p_arg_index,
                                                 uindex_t p_arg_reg)
{
    // Get the value in the register, this determines the type.
    MCValueRef t_arg_value =
            CheckedFetchRegister(p_arg_reg);
    if (t_arg_value == nil)
    {
        return false;
    }

    // Resolve the type of the value.
	MCResolvedTypeInfo t_resolved_type;
	if (!ResolveTypeInfo(MCValueGetTypeInfo(t_arg_value),
						 t_resolved_type))
	{
		return false;
	}
    
    // Store the actual type of the value.
    MCTypeInfoRef t_actual_type = t_resolved_type.type;
    
    // Fetch the promoted type of arg type (sub ints promote to int, and float
    // promotes to double) - but only if the actual type is foreign.
    const MCForeignTypeDescriptor *t_desc = nullptr;
    if (MCTypeInfoIsForeign(t_actual_type))
    {
        t_desc = MCForeignTypeInfoGetDescriptor(t_actual_type);
    }
    
    MCTypeInfoRef t_arg_type = t_actual_type;
    if (t_desc != nullptr)
    {
        if (t_desc->promote != nullptr)
        {
            MCResolvedTypeInfo t_resolved_arg_type;
            if (!ResolveTypeInfo(t_desc->promotedtype,
                                 t_resolved_arg_type))
            {
                return false;
            }
            
            t_arg_type = t_resolved_arg_type.type;
        }
    }
    
    // Compute the slot attributes - using the (potentially) promoted type.
    size_t t_slot_size = 0;
    size_t t_slot_align = 0;
    __MCScriptValueDrop t_slot_drop = nil;
    __MCScriptComputeSlotAttributes(t_arg_type,
                                    t_slot_size,
                                    t_slot_align,
                                    t_slot_drop);
    
    // Allocate room for the slot in the invocation.
    void *t_slot_ptr = nil;
    if (!p_invocation.Allocate(t_slot_size,
                               t_slot_align,
                               t_slot_ptr))
    {
        Rethrow();
        return false;
    }
    
    // Convert the value to an unboxed one - we use the actual type of the argument
    // here.
    // TODO: Split UnboxingConvert so that we don't resolve types twice.
    if (!UnboxingConvert(t_arg_value,
                         t_resolved_type,
                         t_slot_ptr))
    {
        return false;
    }
    
    // If we don't get a slot out, then it failed.
    if (t_slot_ptr == nil)
    {
        ThrowInvalidValueForArgument(p_instance,
                                     p_handler_def,
                                     p_arg_index,
                                     t_arg_value);
        return false;
    }

    // If we must promote the type, do it next. Note that the slot ptr is big
    // enough to hold the promoted value, and we do this 'in place.
    if (t_arg_type != t_actual_type)
    {
        t_desc->promote(t_slot_ptr);
    }

    // Fetch the ffi_type to use - notice that we must use the *promoted* type
    // here.
    ffi_type *t_type = nullptr;
    if (t_desc != nullptr)
    {
        t_type = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_arg_type);
    }
    else
    {
        t_type = &ffi_type_pointer;
    }
    
    if (!p_invocation.Argument(t_slot_ptr,
                               t_slot_drop,
                               t_type))
    {
        Rethrow();
        return false;
    }
    
    return true;
}

bool
MCScriptExecuteContext::InvokeForeignArgument(MCScriptForeignInvocation& p_invocation,
                                              MCScriptInstanceRef p_instance,
                                              MCScriptForeignHandlerDefinition *p_handler_def,
                                              uindex_t p_arg_index,
                                              MCHandlerTypeFieldMode p_mode,
                                              MCTypeInfoRef p_param_type,
                                              uindex_t p_arg_reg)
{
    // Compute the parameter's slot size and allocate storage for it
    MCResolvedTypeInfo t_resolved_param_type;
    if (!ResolveTypeInfo(p_param_type,
                         t_resolved_param_type))
    {
        return false;
    }
    
    size_t t_slot_size = 0;
    size_t t_slot_align = 0;
    __MCScriptValueDrop t_slot_drop = nil;
    __MCScriptComputeSlotAttributes(t_resolved_param_type.type,
                                    t_slot_size,
                                    t_slot_align,
                                    t_slot_drop);
    
    void *t_slot_ptr = nil;
    if (!p_invocation.Allocate(t_slot_size,
                               t_slot_align,
                               t_slot_ptr))
    {
        Rethrow();
        return false;
    }
    
    // If the mode is not out, then we have an initial value to initialize
    // it with; otherwise we must initialize it directly.
    MCValueRef t_arg_value = nil;
    if (p_mode != kMCHandlerTypeFieldModeOut)
    {
        t_arg_value = CheckedFetchRegister(p_arg_reg);
        
        if (t_arg_value == nil)
        {
            return false;
        }
    }

    // Convert the value to an unboxed one.
    if (!UnboxingConvert(t_arg_value,
                         t_resolved_param_type,
                         t_slot_ptr))
    {
        return false;
    }
    
    if (t_slot_ptr == nil)
    {
        ThrowInvalidValueForArgument(p_instance,
                                     p_handler_def,
                                     p_arg_index,
                                     t_arg_value);
        return false;
    }
    
    if (p_mode == kMCHandlerTypeFieldModeIn)
    {
        ffi_type *t_type = nullptr;
        if (MCTypeInfoIsForeign(t_resolved_param_type.type))
        {
            t_type = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_resolved_param_type.type);
        }
        else
        {
            t_type = &ffi_type_pointer;
        }
        
        if (!p_invocation.Argument(t_slot_ptr,
                                   t_slot_drop,
                                   t_type))
        {
            Rethrow();
            return false;
        }
    }
    else
    {
        if (!p_invocation.ReferenceArgument(t_slot_ptr,
                                            t_slot_drop))
        {
            Rethrow();
            return false;
        }
    }
    
    return true;
}

void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_handler_def,
									  uindex_t p_result_reg,
                                      MCSpan<const uindex_t> p_argument_regs)
{
	if (m_error)
	{
		return;
	}
	
	if (p_handler_def->language == kMCScriptForeignHandlerLanguageUnknown)
	{
		if (!MCScriptBindForeignHandlerInInstanceInternal(p_instance,
														  p_handler_def))
		{
			Rethrow();
			return;
		}
	}
	
    /* If we get here then then handler *must* have been bound successfully, and
     * so have a non 'unknown' language. */
    MCAssert(p_handler_def->language != kMCScriptForeignHandlerLanguageUnknown);
    
	// Fetch the handler signature.
	MCTypeInfoRef t_signature =
			GetSignatureOfHandler(p_instance,
								  p_handler_def);
    
    // Determine whether the handler is variadic
    bool t_is_variadic =
            MCHandlerTypeInfoIsVariadic(t_signature);
	
    // Fetch the minimum parameter count (the fixed arg count for variadiac
    // handlers).
    uindex_t t_fixed_arg_count =
            MCHandlerTypeInfoGetParameterCount(t_signature);
    
	// Check the parameter count.
    if ((!t_is_variadic && t_fixed_arg_count != p_argument_regs.size()) ||
        (t_is_variadic && t_fixed_arg_count > p_argument_regs.size()))
    {
        ThrowWrongNumberOfArguments(p_instance,
                                    p_handler_def,
                                    p_argument_regs.size());
        return;
    }
	
    // Process the fixed arguments
	MCScriptForeignInvocation t_invocation;
	for(uindex_t i = 0; i < t_fixed_arg_count; ++i)
	{
		// Fetch the parameter mode and type
		MCHandlerTypeFieldMode t_param_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
		
		MCTypeInfoRef t_param_type =
				MCHandlerTypeInfoGetParameterType(t_signature,
												  i);
		
        // Process the argument
        if (!InvokeForeignArgument(t_invocation,
                                   p_instance,
                                   p_handler_def,
                                   i,
                                   t_param_mode,
                                   t_param_type,
                                   p_argument_regs[i]))
        {
            return;
        }
	}
	
    // If variadic, process the non-fixed arguments.
    if (t_is_variadic)
    {
        for(uindex_t i = t_fixed_arg_count; i < p_argument_regs.size(); i++)
        {
            if (!InvokeForeignVarArgument(t_invocation,
                                          p_instance,
                                          p_handler_def,
                                          i,
                                          p_argument_regs[i]))
            {
                return;
            }
        }
    }
    
	// Fetch the return value type.
	MCResolvedTypeInfo t_return_value_type;
	if (!ResolveTypeInfo(MCHandlerTypeInfoGetReturnType(t_signature),
						 t_return_value_type))
	{
		return;
	}
	
	// Compute the return value slot size, but only if the return type is not
	// nothing.
	size_t t_return_value_slot_size = 0;
	size_t t_return_value_slot_align = 0;
	__MCScriptValueDrop t_return_value_slot_drop = nil;
	void *t_return_value_slot_ptr = nil;
	if (t_return_value_type.named_type != kMCNullTypeInfo)
	{
		__MCScriptComputeSlotAttributes(t_return_value_type.type,
										t_return_value_slot_size,
										t_return_value_slot_align,
										t_return_value_slot_drop);
		
		// Alloate the return value slot storage (which will do nothing for
		// zero size).
		if (!t_invocation.Allocate(t_return_value_slot_size,
								   t_return_value_slot_align,
								   t_return_value_slot_ptr))
		{
			Rethrow();
			return;
		}
	}
	
	// Call the function - we assume here that if the invocation succeeds
	// then the result slot will be valid unless an LC error has been
	// raised.
	if (!t_invocation.Call(p_handler_def,
						   t_signature,
						   t_return_value_slot_ptr) ||
		MCErrorIsPending())
	{
		Rethrow();
		return;
	}
	
	// Box the return value - this operation 'releases' the contents of
	// the slot (i.e. the value is taken by the box).
	MCAutoValueRef t_return_value;
	if (!BoxingConvert(t_return_value_type,
					   t_return_value_slot_ptr,
					   &t_return_value))
	{
		return;
	}
	
	// Store the boxed return value into the result reg (if required).
	if (p_result_reg != UINDEX_MAX)
	{
		if (!CheckedStoreRegister(p_result_reg,
								  *t_return_value))
		{
			return;
		}
	}
	
	for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); ++i)
	{
		// Fetch the parameter mode and type
		
		MCHandlerTypeFieldMode t_param_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
		
		MCTypeInfoRef t_param_type =
				MCHandlerTypeInfoGetParameterType(t_signature,
												  i);
		
		// If the mode is in, there is nothing to do for this parameter.
		if (t_param_mode == kMCHandlerTypeFieldModeIn)
		{
			continue;
		}
		
		MCResolvedTypeInfo t_resolved_param_type;
		if (!ResolveTypeInfo(t_param_type,
							 t_resolved_param_type))
		{
			return;
		}
		
		// Do a boxing convert - note that we take the slot value from the
		// invocation as BoxingConvert will release regardless of successs.
		MCAutoValueRef t_arg_value;
		if (!BoxingConvert(t_resolved_param_type,
						   t_invocation.TakeArgument(i),
						   &t_arg_value))
		{
			return;
		}
		
		// Finally do a checked store to register.
		if (!CheckedStoreRegister(p_argument_regs[i],
								  *t_arg_value))
		{
			return;
		}
	}
}

bool
MCScriptExecuteContext::Bridge(MCValueRef p_value,
                               MCValueRef& r_output_value)
{
    // Get resolved typeinfo for the value.
    MCResolvedTypeInfo t_resolved_type;
    if (!ResolveTypeInfo(MCValueGetTypeInfo(p_value),
                         t_resolved_type))
    {
        return false;
    }
 
    // Get the foreign type descriptor for the value's type, if any.
    const MCForeignTypeDescriptor *t_desc = nullptr;
    if (MCTypeInfoIsForeign(t_resolved_type.type))
    {
        t_desc = MCForeignTypeInfoGetDescriptor(t_resolved_type.type);
    }
    
    // If the type is not foreign or is not bridgeable foreign, just retain;
    // otherwise use doimport.
    if (t_desc == nullptr ||
        t_desc->doimport == nullptr)
    {
        r_output_value = MCValueRetain(p_value);
    }
    else
    {
        if (!t_desc->doimport(t_desc,
                              MCForeignValueGetContentsPtr(p_value),
                              false,
                              r_output_value))
        {
            Rethrow();
            return false;
        }
    }
    
    return true;
}

bool
MCScriptExecuteContext::Convert(MCValueRef p_value,
								MCTypeInfoRef p_to_type,
								MCValueRef& r_new_value)
{
	if (p_to_type == nil)
	{
		r_new_value = MCValueRetain(p_value);
		return true;
	}
	
	MCResolvedTypeInfo t_resolved_to_type;
	if (!ResolveTypeInfo(p_to_type,
						 t_resolved_to_type))
	{
		return false;
	}
	
	return ConvertToResolvedType(p_value,
								 t_resolved_to_type,
								 r_new_value);
}

bool
MCScriptExecuteContext::ConvertToResolvedType(MCValueRef p_value,
											  const MCResolvedTypeInfo& p_to_type,
											  MCValueRef& r_new_value)
{
	// Get resolved typeinfos.
	MCResolvedTypeInfo t_resolved_from_type;
	if (!ResolveTypeInfo(MCValueGetTypeInfo(p_value),
						 t_resolved_from_type))
	{
		return false;
	}
	
	// Check conformance - if this fails, then there is no conversion.
	if (!MCResolvedTypeInfoConforms(t_resolved_from_type,
									p_to_type))
	{
		r_new_value = nil;
		return true;
	}
	
	// The only case which requires a value conversion is when
	// one side is foreign - which is a bridging operation. In
	// other cases, it is a cast towards root which doesn't require
	// a value conversion.
	
	const MCForeignTypeDescriptor *t_from_desc = nil;
	if (MCTypeInfoIsForeign(t_resolved_from_type.type))
	{
		t_from_desc = MCForeignTypeInfoGetDescriptor(t_resolved_from_type.type);
	}
	
	const MCForeignTypeDescriptor *t_to_desc = nil;
	if (MCTypeInfoIsForeign(p_to_type.type))
	{
		t_to_desc = MCForeignTypeInfoGetDescriptor(p_to_type.type);
	}
	
    if (t_from_desc == t_to_desc)
    {
        // The two types are the same or are both non-foreign - no conversion is
        // needed at this stage.
        r_new_value = MCValueRetain(p_value);
    }
	else if (t_from_desc != nil)
	{
        // Import the contents of the foreign value as its bridge type. Note
        // that not all types can be imported (i.e there is no bridging type)
		if (t_from_desc->doimport == nil ||
            !t_from_desc->doimport(t_from_desc,
                                   MCForeignValueGetContentsPtr(p_value),
								   false,
								   r_new_value))
		{
			Rethrow();
			return false;
		}
	}
	else if (t_to_desc != nil)
	{
        // If the foreign-typed slot is optional and the value is undefined,
        // don't try to export. This allows optional foreign slots to be set to
        // undefined when the type has no bridging type; without this, it is not
        // possible to set these slots to undefined!
        if (p_to_type.is_optional &&
            p_value == kMCNull)
        {
            r_new_value = MCValueRetain(kMCNull);
        }
        else
        {
            // Export the foreign value
            if (!MCForeignValueExport(p_to_type.named_type,
                                      p_value,
                                      (MCForeignValueRef&)r_new_value))
            {
                Rethrow();
                return false;
            }
        }
	}
	
	return true;
}

bool
MCScriptExecuteContext::BoxingConvert(const MCResolvedTypeInfo& p_slot_type,
									  void *p_slot_ptr,
									  MCValueRef& r_new_value)
{
	if (MCTypeInfoIsForeign(p_slot_type.type))
	{
		// This is a foreign slot type. If the foreign value has the notion
		// of 'defined', then we map to Null if it is not defined.
		
		const MCForeignTypeDescriptor *t_desc =
				MCForeignTypeInfoGetDescriptor(p_slot_type.type);
		
		if (t_desc->defined != nil &&
			!t_desc->defined(p_slot_ptr))
		{
			r_new_value = MCValueRetain(kMCNull);
		}
		else if (!MCForeignValueCreateAndRelease(p_slot_type.named_type,
												 p_slot_ptr,
												 (MCForeignValueRef&)r_new_value))
		{
			Rethrow();
			return false;
		}
	}
	else if (p_slot_type.named_type == kMCNullTypeInfo)
	{
		r_new_value = MCValueRetain(kMCNull);
	}
	else
	{
		// This is not a foreign slot type, so we just take the valueref value.
		// Since we bridge null to nil, we need to check that here.
		if (*(MCValueRef *)p_slot_ptr != nil)
		{
			r_new_value = *(MCValueRef *)p_slot_ptr;
		}
		else
		{
			r_new_value = MCValueRetain(kMCNull);
		}
	}
	
	return true;
}

bool
MCScriptExecuteContext::UnboxingConvert(MCValueRef p_value,
										const MCResolvedTypeInfo& p_slot_type,
										void*& x_slot_ptr)
{
	// If the input value is nil then this is a slot init.
	if (p_value == nil)
	{
		if (MCTypeInfoIsForeign(p_slot_type.type))
		{
			// The value is foreign, so if it has an initializer - use it.
			const MCForeignTypeDescriptor *t_desc =
					MCForeignTypeInfoGetDescriptor(p_slot_type.type);
			
			if (t_desc->initialize != nil)
			{
				if (!t_desc->initialize(x_slot_ptr))
				{
					Rethrow();
					return false;
				}
			}
		}
		else
		{
			// If it is not foreign, then it is a valueref which always
			// initialize to unassigned.
			*(MCValueRef *)x_slot_ptr = nil;
		}
		
		return true;
	}
	
	MCResolvedTypeInfo t_resolved_from_type;
	if (!ResolveTypeInfo(MCValueGetTypeInfo(p_value),
						 t_resolved_from_type))
	{
		return false;
	}
	
	// Check conformance - if this fails, then there is no conversion.
	if (!MCResolvedTypeInfoConforms(t_resolved_from_type,
									p_slot_type))
	{
		x_slot_ptr = nil;
		return true;
	}
	
	if (MCTypeInfoIsForeign(p_slot_type.type))
	{
		const MCForeignTypeDescriptor *t_slot_desc =
				MCForeignTypeInfoGetDescriptor(p_slot_type.type);
		
		// If the source is foreign then we just copy the contents, otherwise
		// it is a bridging conversion so we must export.
		if (MCTypeInfoIsForeign(t_resolved_from_type.type))
		{
            const MCForeignTypeDescriptor *t_from_desc =
                MCForeignTypeInfoGetDescriptor(t_resolved_from_type.type);
            
            // If the two foreign types are the same, copy the contents
            if (t_slot_desc == t_from_desc)
            {
                if (!t_slot_desc->copy(t_slot_desc,
                                       MCForeignValueGetContentsPtr(p_value),
                                       x_slot_ptr))
                {
                    Rethrow();
                    return false;
                }
            }
            else
            {
                // Bridging to the common bridging type and back again is required
                // Note that we can only get here if the bridging type is common
                // (otherwise the types wouldn't conform).
                MCAssert(t_slot_desc->bridgetype == t_from_desc->bridgetype);
                
                MCAutoValueRef t_bridged_value;
                if (!t_from_desc->doimport(t_from_desc, MCForeignValueGetContentsPtr(p_value), false, &t_bridged_value) ||
                    !t_slot_desc->doexport(t_slot_desc, *t_bridged_value, false, x_slot_ptr))
                {
                    Rethrow();
                    return false;
                }
            }
		}
		else
		{
			// If the source value is null, then it must mean the target type
			// is nullable.
			if (p_value == kMCNull)
			{
				if (!t_slot_desc->initialize(x_slot_ptr))
				{
					Rethrow();
					return false;
				}
			}
			else if (!t_slot_desc->doexport(t_slot_desc,
                                            p_value,
											false,
											x_slot_ptr))
			{
				Rethrow();
				return false;
			}
		}
	}
	else if (MCTypeInfoIsForeign(t_resolved_from_type.type))
	{
		const MCForeignTypeDescriptor *t_from_desc =
				MCForeignTypeInfoGetDescriptor(t_resolved_from_type.type);
		
        // If the type of the destination slot is not exactly the foreign type,
        // import as the bridge type.
        if (t_from_desc->bridgetype != kMCNullTypeInfo &&
            t_resolved_from_type.type != p_slot_type.type)
        {
            MCValueRef t_bridged_value;
            if (!t_from_desc->doimport(t_from_desc,
                                       MCForeignValueGetContentsPtr(p_value),
                                       false,
                                       t_bridged_value))
            {
                Rethrow();
                return false;
            }
            
            *(MCValueRef *)x_slot_ptr = t_bridged_value;
        }
		else
        {
            *(MCValueRef *)x_slot_ptr = MCValueRetain(p_value);
        }
	}
	else if (MCTypeInfoIsHandler(p_slot_type.type) &&
			 MCHandlerTypeInfoIsForeign(p_slot_type.type) &&
             p_value != kMCNull)
	{
		// If the slot type is a foreign handler, then we fetch a closure
		// from the handler value.
		void *t_function_ptr;
		if (!MCHandlerGetFunctionPtr((MCHandlerRef)p_value,
									 t_function_ptr))
		{
			Rethrow();
			return false;
		}
		
		*(void **)x_slot_ptr = t_function_ptr;
	}
	else
	{
		// If the valueref is Null, then we map to nil.
		if (p_value != kMCNull)
		{
			*(MCValueRef *)x_slot_ptr = MCValueRetain(p_value);
		}
		else
		{
			*(MCValueRef *)x_slot_ptr = nil;
		}
	}
	
	return true;
}
