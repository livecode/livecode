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
