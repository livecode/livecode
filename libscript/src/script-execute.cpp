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

#include "script-execute.hpp"

typedef void (*__MCScriptValueDrop)(void *);

class MCScriptForeignInvocation
{
public:
	MCScriptForeignInvocation(void)
		: m_argument_count(0),
		  m_storage_frontier(0),
		  m_storage_limit(sizeof(m_stack_storage))
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
		if (m_storage_limit - m_storage_frontier < p_amount + t_align_delta)
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
				  __MCScriptValueDrop p_slot_drop)
	{
		if (m_argument_count == kMaxArguments)
		{
			return MCErrorThrowOutOfMemory();
		}
		
		// For normal arguments, the slot is passed directly.
		m_argument_values[m_argument_count] = p_slot_ptr;
		
		// Store the slot ptr and its drop.
		m_argument_slots[m_argument_count] = p_slot_ptr;
		m_argument_drops[m_argument_count] = p_slot_drop;
		
		// Bump the number of arguments
		m_argument_count++;
		
		return true;
	}
	
	// Append a reference argument - this takes contents_ptr
	bool ReferenceArgument(void *p_slot_ptr,
						   __MCScriptValueDrop p_slot_drop)
	{
		if (m_argument_count == kMaxArguments ||
			!Allocate(sizeof(void *),
					  __alignof__(void *),
					  m_argument_values[m_argument_count]))
		{
			return MCErrorThrowOutOfMemory();
		}
		
		// Store the slot ptr and its drop.
		m_argument_slots[m_argument_count] = p_slot_ptr;
		m_argument_drops[m_argument_count] = p_slot_drop;
		
		return true;
	}
	
	// Call
	bool Call(void *p_function_cif,
			  void *p_function,
			  void *p_result_slot_ptr)
	{
		
		ffi_call((ffi_cif *)p_function_cif,
				 (void(*)())p_function,
				 p_result_slot_ptr,
				 m_argument_values);
		
		return true;
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
	// a pointer to the points in m_argument_slots for reference
	// parameters.
	void *m_argument_values[kMaxArguments];
	// The drops for the slot values in m_argument_slots.
	__MCScriptValueDrop m_argument_drops[kMaxArguments];
	// The actual values in the slots (not indirected for references).
	void *m_argument_slots[kMaxArguments];
	
	size_t m_storage_frontier;
	size_t m_storage_limit;
	char m_stack_storage[kMaxStorage];
};

inline void
__MCScriptComputeSlotAttributes(MCTypeInfoRef p_slot_type,
								size_t& r_slot_size,
								size_t& r_slot_align,
								__MCScriptValueDrop& r_slot_drop)
{
	MCResolvedTypeInfo t_resolved_slot_type;
	r_slot_size = 0;
	r_slot_align = sizeof(void*);
	r_slot_drop = nil;
}

void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_handler_def,
									  uindex_t p_result_reg,
									  const uindex_t *p_argument_regs,
									  uindex_t p_argument_count)
{
	if (m_error)
	{
		return;
	}
	
	if (p_handler_def->function == nil)
	{
		if (!MCScriptBindForeignHandlerOfInstanceInternal(p_instance,
														  p_handler_def))
		{
			Rethrow();
			return;
		}
	}
	
	// Fetch the handler signature.
	MCTypeInfoRef t_signature =
			GetSignatureOfHandler(p_instance,
								  p_handler_def);
	
	// Check the parameter count.
	if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_argument_count)
	{
		ThrowWrongNumberOfArguments(p_instance,
									p_handler_def,
									p_argument_count);
		return;
	}
	
	MCScriptForeignInvocation t_invocation;
	for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); ++i)
	{
		// Fetch the parameter mode and type
		
		MCHandlerTypeFieldMode t_param_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
		
		MCTypeInfoRef t_param_type =
				MCHandlerTypeInfoGetParameterType(t_signature,
												  i);
		
		// Compute the parameter's slot size and allocate storage for it
		
		size_t t_slot_size = 0;
		size_t t_slot_align = 0;
		__MCScriptValueDrop t_slot_drop = nil;
		__MCScriptComputeSlotAttributes(t_param_type,
										t_slot_size,
										t_slot_align,
										t_slot_drop);
		
		void *t_slot_ptr = nil;
		if (!t_invocation.Allocate(t_slot_size,
								   t_slot_align,
								   t_slot_ptr))
		{
			Rethrow();
			return;
		}
		
		// If the mode is not out, then we have an initial value to initialize
		// it with; otherwise we must initialize it directly.
		MCValueRef t_arg_value = nil;
		if (t_param_mode != kMCHandlerTypeFieldModeOut)
		{
			t_arg_value = CheckedFetchRegister(i);
			
			if (t_arg_value == nil)
			{
				return;
			}
		}

		// Convert the value to an unboxed one.
		if (!UnboxingConvert(t_arg_value,
							 t_param_type,
							 t_slot_ptr))
		{
			return;
		}
		
		if (t_slot_ptr == nil)
		{
			ThrowInvalidValueForArgument(p_instance,
										 p_handler_def,
										 i,
										 t_arg_value);
			return;
		}
		
		if (t_param_mode == kMCHandlerTypeFieldModeIn)
		{
			if (!t_invocation.Argument(t_slot_ptr,
									   t_slot_drop))
			{
				Rethrow();
				return;
			}
		}
		else
		{
			if (!t_invocation.ReferenceArgument(t_slot_ptr,
												t_slot_drop))
			{
				Rethrow();
				return;
			}
		}
	}
	
	// Fetch the return value type.
	MCTypeInfoRef t_return_value_type =
			MCHandlerTypeInfoGetReturnType(t_signature);
	
	// Compute the return value slot size (which will be zero for nothing).
	size_t t_return_value_slot_size = 0;
	size_t t_return_value_slot_align = 0;
	__MCScriptValueDrop t_return_value_slot_drop = nil;
	__MCScriptComputeSlotAttributes(t_return_value_type,
									t_return_value_slot_size,
									t_return_value_slot_align,
									t_return_value_slot_drop);
	
	// Alloate the return value slot storage (which will do nothing for
	// zero size).
	void *t_return_value_slot_ptr = nil;
	if (!t_invocation.Allocate(t_return_value_slot_size,
							   t_return_value_slot_align,
							   t_return_value_slot_ptr))
	{
		Rethrow();
		return;
	}
	
	// Call the function - we assume here that if the invocation succeeds
	// then the result slot will be valid unless an LC error has been
	// raised.
	if (!t_invocation.Call(p_handler_def->function_cif,
						   p_handler_def->function,
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
		
		// Do a boxing convert - note that we take the slot value from the
		// invocation as BoxingConvert will release regardless of successs.
		MCAutoValueRef t_arg_value;
		if (!BoxingConvert(t_param_type,
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
MCScriptExecuteContext::Convert(MCValueRef p_value,
								MCTypeInfoRef p_to_type,
								MCValueRef& r_new_value)
{
	// If the to type is nil, then its a pass through conversion
	if (p_to_type == nil)
	{
		r_new_value = MCValueRetain(p_value);
		return true;
	}
	
	// Get resolved typeinfos.
	
	MCResolvedTypeInfo t_resolved_from_type;
	if (!ResolveTypeInfo(MCValueGetTypeInfo(p_value),
						 t_resolved_from_type))
	{
		return false;
	}
	
	MCResolvedTypeInfo t_resolved_to_type;
	if (!ResolveTypeInfo(p_to_type,
						 t_resolved_to_type))
	{
		return false;
	}
	
	// Check conformance - if this fails, then there is no conversion.
	if (!MCResolvedTypeInfoConforms(t_resolved_from_type,
									t_resolved_to_type))
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
	if (MCTypeInfoIsForeign(t_resolved_to_type.type))
	{
		t_to_desc = MCForeignTypeInfoGetDescriptor(t_resolved_to_type.type);
	}
	
	if (t_from_desc != nil)
	{
		// Import the contents of the foreign value as its bridge type.
		if (!t_from_desc->doimport(MCForeignValueGetContentsPtr(p_value),
								   false,
								   r_new_value))
		{
			Rethrow();
			return false;
		}
	}
	else if (t_to_desc != nil)
	{
		// Export the foreign value
		if (!MCForeignValueExport(t_resolved_to_type.type,
								  p_value,
								  (MCForeignValueRef&)r_new_value))
		{
			Rethrow();
			return false;
		}
	}
	else
	{
		r_new_value = MCValueRetain(p_value);
	}
	
	return true;
}

bool
MCScriptExecuteContext::BoxingConvert(MCTypeInfoRef p_slot_type,
									  void *p_slot_ptr,
									  MCValueRef& r_new_value)
{
	MCResolvedTypeInfo t_resolved_slot_type;
	if (!ResolveTypeInfo(p_slot_type,
						 t_resolved_slot_type))
	{
		return false;
	}
	
	if (MCTypeInfoIsForeign(t_resolved_slot_type.type))
	{
		// This is a foreign slot type.
		if (!MCForeignValueCreateAndRelease(p_slot_type,
											p_slot_ptr,
											(MCForeignValueRef&)r_new_value))
		{
			Rethrow();
			return false;
		}
	}
	else
	{
		// This is not a foreign slot type, so we just take the valueref value.
		r_new_value = *(MCValueRef *)p_slot_ptr;
	}
	
	return true;
}

bool
MCScriptExecuteContext::UnboxingConvert(MCValueRef p_value,
										MCTypeInfoRef p_slot_type,
										void*& x_slot_ptr)
{
	MCResolvedTypeInfo t_resolved_slot_type;
	if (!ResolveTypeInfo(p_slot_type,
						 t_resolved_slot_type))
	{
		return false;
	}
	
	// If the input value is nil then this is a slot init.
	if (p_value == nil)
	{
		if (MCTypeInfoIsForeign(t_resolved_slot_type.type))
		{
			// The value is foreign, so if it has an initializer - use it.
			const MCForeignTypeDescriptor *t_desc =
					MCForeignTypeInfoGetDescriptor(t_resolved_slot_type.type);
			
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
									t_resolved_slot_type))
	{
		x_slot_ptr = nil;
		return true;
	}
	
	if (MCTypeInfoIsForeign(t_resolved_slot_type.type))
	{
		const MCForeignTypeDescriptor *t_slot_desc =
				MCForeignTypeInfoGetDescriptor(t_resolved_slot_type.type);
		
		// If the source is foreign then we just copy the contents, otherwise
		// it is a bridging conversion so we must export.
		if (MCTypeInfoIsForeign(t_resolved_from_type.type))
		{
			if (!t_slot_desc->copy(MCForeignValueGetContentsPtr(p_value),
								   x_slot_ptr))
			{
				Rethrow();
				return false;
			}
		}
		else
		{
			if (!t_slot_desc->doexport(p_value,
									   false,
									   x_slot_ptr))
			{
				Rethrow();
				return false;
			}
		}
		
	}
	else if (MCTypeInfoIsHandler(t_resolved_slot_type.type) &&
			 MCHandlerTypeInfoIsForeign(t_resolved_slot_type.type))
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
		*(MCValueRef *)x_slot_ptr = MCValueRetain(p_value);
	}
	
	return true;
}

#if 0
// Bridging rules:
//   ForeignValue -> ForeignValue
//	   foreign value contents
//   ForeignValue -> ValueRef
//     if src has import, then import else box contents
//   ValueRef -> ForeignValue
//      export
//   ValueRef -> ValueRef
//      if dst is foreign handler type then export function ptr
//      else copy box

typedef void (*__MCScriptValueDrop)(void *);

class MCScriptForeignInvocation
{
public:
	MCScriptForeignInvocation(void);
	~MCScriptForeignInvocation(void);
	
	// Allocate temporary storage for the invocation
	bool Allocate(size_t amount,
				  void*& r_ptr);
	
	// Append a non-reference argument - this takes contents_ptr
	bool Argument(void *contents_ptr,
				  __MCScriptValueDrop contents_drop);
	
	// Append a reference argument - this takes contents_ptr
	bool ReferenceArgument(void *contents_ptr,
						   __MCScriptValueDrop contents_drop);
	
	// Call
	bool Call(ffi_cif *function_cif,
			  void *function,
			  void *result_ptr);
};

static bool
__MCScriptBridgeValue(MCValueRef p_src_value,
					  MCTypeInfoRef p_src_type,
					  const MCForeignTypeDescriptor* p_src_value_desc,
					  MCTypeInfoRef p_dst_type,
					  const MCForeignTypeDescriptor* p_dst_value_desc,
					  void *p_contents_ptr,
					  __MCScriptValueDrop& r_contents_drop)
{
	if (p_src_value_desc == nil &&
		p_dst_value_desc == nil)
	{
		// Both sides are non-foreign, so if this isn't a special handler
		// conversion, we just retain and copy the valueref pointer itself.
		
		if (!MCTypeInfoIsHandler(p_dst_type) ||
			!MCHandlerTypeInfoIsForeign(p_dst_type))
		{
			MCValueRef *t_typed_contents =
			reinterpret_cast<MCValueRef *>(p_contents_ptr);
			*t_typed_contents = MCValueRetain(p_src_value);
			r_contents_drop = MCValueRelease;
		}
		else
		{
			void *t_function_ptr;
			if (!MCHandlerGetFunctionPtr((MCHandlerRef)p_src_value,
										 t_function_ptr))
			{
				return false;
			}
			
			*(void **)p_contents_ptr = t_function_ptr;
			r_contents_drop = nil;
		}
	}
	else if (p_src_value_desc == nil &&
			 p_dst_value_desc != nil)
	{
		// The source is a valueref, and the dest is a foreign value. This
		// means that if the source value is non-null, it must be exported.
		
		if (p_src_value != kMCNull)
		{
			if (!p_dst_value_desc->doexport(p_src_value,
											false,
											p_contents_ptr))
			{
				return false;
			}
		}
		else
		{
			if (!p_dst_value_desc->initialize(p_contents_ptr))
			{
				return false;
			}
		}
		
		r_contents_drop = p_dst_value_desc->finalize;
	}
	else if (p_src_value_desc != nil &&
			 p_dst_value_desc == nil)
	{
		// The source value is a foreign value, but the dest is a valueref.
		// If the src can be imported then we do that, otherwise we assume
		// it is a boxed foreign value which is required.
		
		if (p_src_value_desc->doimport != nil)
		{
			if (!p_src_value_desc->doimport(MCForeignValueGetContentsPtr(p_src_value),
											false,
											p_contents_ptr))
			{
				return false;
			}
			
			r_contents_drop = p_dst_value_desc->finalize;
		}
		else
		{
			MCValueRef *t_typed_contents =
			reinterpret_cast<MCValueRef *>(p_contents_ptr);
			*t_typed_contents = MCValueRetain(p_src_value);
			r_contents_drop = MCValueRelease;
		}
	}
	else
	{
		// Both source and dst are foreign.
		if (!p_dst_value_desc->copy(MCForeignValueGetContentsPtr(p_src_value),
									p_contents_ptr))
		{
			return false;
		}
		
		r_contents_drop = p_dst_value_desc->finalize;
	}
	
	return true;
}

inline void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_handler_def,
									  uindex_t p_result_reg,
									  const uindex_t *p_argument_regs,
									  uindex_t p_argument_count)
{
	if (m_error)
	{
		return;
	}
	
	if (p_handler_def->function == nil)
	{
		if (!MCScriptBindForeignHandlerOfInstanceInternal(p_instance,
														  p_handler_def))
		{
			Rethrow();
			return;
		}
	}
	
	// Fetch the handler signature.
	MCTypeInfoRef t_signature =
	GetSignatureOfHandler(p_instance,
						  p_handler_def);
	
	// Check the parameter count.
	if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_argument_count)
	{
		ThrowWrongNumberOfArguments(p_instance,
									p_handler_def,
									p_argument_count);
		return;
	}
	
	MCScriptForeignInvocation t_invocation;
	for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); ++i)
	{
		MCHandlerTypeFieldMode t_param_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
		
		MCResolvedTypeInfo t_param_type;
		if (!MCTypeInfoResolve(MCHandlerTypeInfoGetParameterType(t_signature,
																 i),
							   t_param_type))
		{
			Rethrow();
			return;
		}
		
		MCValueRef t_arg_value =
				CheckedFetchRegister(i);
		
		if (IsError())
		{
			return;
		}
		
		MCResolvedTypeInfo t_arg_type;
		if (!MCTypeInfoResolve(MCValueGetTypeInfo(t_arg_value),
							   t_arg_type))
		{
			Rethrow();
			return;
		}
		
		if (!MCResolvedTypeInfoConforms(t_arg_type,
										t_param_type))
		{
			ThrowInvalidValueForArgument(p_instance,
										 p_handler_def,
										 i,
										 t_arg_value);
			return;
		}
		
		const MCForeignTypeDescriptor *t_param_desc = nil;
		if (MCTypeInfoIsForeign(t_param_type.type))
		{
			t_param_desc = MCForeignTypeInfoGetDescriptor(t_param_type.type);
		}
		
		const MCForeignTypeDescriptor *t_arg_desc = nil;
		if (MCTypeInfoIsForeign(t_arg_type.type))
		{
			t_arg_desc = MCForeignTypeInfoGetDescriptor(t_arg_type.type);
		}
		
		// If the parameter type is not foreign, it must be a valueref, otherwise
		// it is managed by the foreign descriptor.
		size_t t_contents_size =
				sizeof(MCValueRef);
		if (t_param_desc != nil)
		{
			t_contents_size = t_param_desc->size;
		}
		
		void *t_contents_ptr = nil;
		if (!t_invocation.Allocate(t_contents_size,
								   t_contents_ptr))
		{
			Rethrow();
			return;
		}
		
		if (t_param_mode != kMCHandlerTypeFieldModeOut)
		{
			if (!__MCScriptBridgeValue(t_arg_value,
									   t_arg_desc,
									   t_param_desc,
									   t_contents_ptr,
									   t_contents_drop))
			{
				Rethrow();
				return;
			}
		}
		
		if (t_param_mode == kMCHandlerTypeFieldModeIn)
		{
			if (!t_invocation.Argument(t_contents_ptr,
									   t_contents_drop))
			{
				Rethrow();
				return;
			}
		}
		else
		{
			if (!t_invocation.ReferenceArgument(t_contents_ptr,
												t_contents_drop))
			{
				Rethrow();
				return;
			}
		}
	}
	
	MCResolvedTypeInfo t_return_type;
	if (!MCTypeInfoResolve(MCHandlerTypeGetReturnType(t_signature),
						   t_return_type))
	{
		Rethrow();
		return;
	}
	
	MCForeignTypeDescriptor *t_return_value_desc = nil;
	if (MCTypeInfoIsForeign(t_return_type))
	{
		t_return_value_desc = MCForeignTypeInfoGetDescriptor(t_return_type);
	}
	
	size_t t_return_type_size =
			sizeof(MCValueRef)
	if (t_return_type_desc != nil)
	{
		t_return_type_size = t_return_type_desc->size;
	}
	else if (t_return_type.named_type == kMCNullTypeInfo)
	{
		t_return_value_size = 0;
	}

	void *t_return_type_ptr = nil;
	if (!t_invocation.Allocate(t_return_type_size,
							   t_return_type_ptr))
	{
		Rethrow();
		return;
	}
	
	if (!t_invocation.Call((ffi_cif *)p_handler->function_cif,
						   (void(*)())p_handler->function,
						   t_return_type_ptr) ||
		MCErrorIsPending())
	{
		Rethrow();
		return;
	}
	
	
}
#endif