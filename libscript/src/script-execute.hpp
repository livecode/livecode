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

#include <libscript/script.h>

#include "script-private.h"

class MCScriptForeignInvocation;

class MCScriptExecuteContext
{
public:
	// Constructor.
	MCScriptExecuteContext(void);
	
	// Destructor.
	~MCScriptExecuteContext(void);
	
	// Returns true if an error has occurred
	bool IsError(void) const;
	
	// Return the current operation
	MCScriptBytecodeOp GetOperation(void) const;
	
	// Return the address of the current opcode
	uindex_t GetAddress(void) const;
	
	// Return the address of the next opcode
	uindex_t GetNextAddress(void) const;
	
	// Return the number of arguments to the current opcode
	uindex_t GetArgumentCount(void) const;
	
	// Return the argument at the given index as unsigned
	uindex_t GetArgument(uindex_t index) const;
	
	// Return the argument at the given index as signed
	index_t GetSignedArgument(uindex_t index) const;
	
	// Return the list of arguments to the current opcode
	const uindex_t *GetArgumentList(void) const;

	// Return the list of arguments to the current opcode
	MCSpan<const uindex_t> GetArguments() const;
	
	// Fetch the type of the given register. The type of the register might
	// be nil, if it has no assigned type.
	MCTypeInfoRef GetTypeOfRegister(uindex_t index) const;
    
    // Fetch the signature of the specified handler in the given instance.
    MCTypeInfoRef GetSignatureOfHandler(MCScriptInstanceRef instance,
                                        MCScriptCommonHandlerDefinition *definition) const;
	
	// Fetch the value with the given index from the module's value
	// pool.
	MCValueRef FetchValue(uindex_t index) const;
	
	// Fetch from the given register, the result could be nil if the register is
	// unassigned.
	MCValueRef FetchRegister(uindex_t index) const;
	
	// Store into the given register, the value can be nil to unassign the
	// register.
	void StoreRegister(uindex_t index, MCValueRef value);
	
	// Fetch the value from the given constant in the specified instance.
	MCValueRef FetchConstant(MCScriptInstanceRef instance,
							 MCScriptConstantDefinition *definition) const;
	
	// Fetch the handler value from the given definition in the specified
	// instance.
	MCValueRef FetchHandler(MCScriptInstanceRef instance,
							MCScriptHandlerDefinition *definition);
	
	// Fetch the foreign handler value from the given definition in the
	// specified instance.
	MCValueRef FetchForeignHandler(MCScriptInstanceRef instance,
								   MCScriptForeignHandlerDefinition *definition);
	
	// Fetch the value from the given variable in the specified instance.
	MCValueRef FetchVariable(MCScriptInstanceRef instance,
							 MCScriptVariableDefinition *definition) const;
	
	// Store the value into the given variable in the specified instance, the
	// value can be nil to unassign the variable.
	void StoreVariable(MCScriptInstanceRef instance,
					   MCScriptVariableDefinition *definition,
                       MCValueRef value);
	
	// Fetch from the given register, checking that it is assigned. If it is
	// unassigned a runtime error is reported, in this case 'nil' is returned.
	MCValueRef CheckedFetchRegister(uindex_t index);
	
	// Store into the given register, the value must convert to the register's
	// type. If it does not, a runtime error is reported.
	bool CheckedStoreRegister(uindex_t index,
							  MCValueRef value);
	
	// Fetch the given register's value as a bool. If the register is unassigned
	// or does not have a 'bool' or 'Boolean' type, a runtime error is reported.
	// In this case, 'false' is returned.
	bool CheckedFetchRegisterAsBool(uindex_t index);
	
	// Fetch the given variable in the given instance, checking that it is
	// assigned. If it is unassigned a runtime error is reporte and 'nil' is
	// returned.
	MCValueRef CheckedFetchVariable(MCScriptInstanceRef instance,
									MCScriptVariableDefinition *definition);
	
	// Store into the given variable in the given instance, the value must convert
	// to the register's type. If it does not, a runtime error is reported.
	void CheckedStoreVariable(MCScriptInstanceRef instance,
							  MCScriptVariableDefinition *definition,
							  MCValueRef value);
	
	// Change the instruction pointer to:
	//   GetAddress() + offset;
	void Jump(index_t offset);
	
	// Create a new activation frame for the given handler in the specified
	// instance, using the list of argument registers as parameters.
	void PushFrame(MCScriptInstanceRef instance,
				   MCScriptHandlerDefinition *handler,
				   uindex_t result_reg,
	               MCSpan<const uindex_t> argument_regs);
	
	// Pop the current activation frame and set the return value to the contents
	// of the given register. If result_reg is UINDEX_MAX, then it means there is
	// no return value.
	void PopFrame(uindex_t result_reg);
	
    // Accumulate a non-fixed var-arg arg into the invocation.
    bool InvokeForeignVarArgument(MCScriptForeignInvocation& invocation,
                                  MCScriptInstanceRef instance,
                                  MCScriptForeignHandlerDefinition *handler_def,
                                  uindex_t arg_index,
                                  uindex_t arg_reg);
    
    // Accumulate a fixed arg into the invocation.
    bool InvokeForeignArgument(MCScriptForeignInvocation& p_invocation,
                               MCScriptInstanceRef p_instance,
                               MCScriptForeignHandlerDefinition *p_handler_def,
                               uindex_t arg_index,
                               MCHandlerTypeFieldMode mode,
                               MCTypeInfoRef type,
                               uindex_t arg_reg);
    
	// Invoke a foreign function with the given arguments, taken from registers
	// returning the value into the result register.
	void InvokeForeign(MCScriptInstanceRef instance,
					   MCScriptForeignHandlerDefinition *handler,
					   uindex_t result_reg,
	                   MCSpan<const uindex_t> argument_regs);
	
	// Enter the LCB VM to execute the specified handler in the given instance.
	void Enter(MCScriptInstanceRef instance,
			   MCScriptHandlerDefinition *handler,
	           MCSpan<MCValueRef> arguments,
			   MCValueRef *r_result);
	
	// Decode the next bytecode to execute. If there is more execution to do,
	// then true is returned. If execution has finished or an error occurred,
	// false is returned.
	bool Step(void);
	
	// Leave the LCB VM. If a execution completed successfully then true is
	// returned, otherwise false is returned.
	bool Leave(void);
    
    // Attempt to bridge the input value. If the input value is foreign and
    // the foreign type is bridgeable, doimport is used to create the output
    // value. Otherwise, the input value is retained and returned as the output
    // value.
    // Note: This is a specialization of Convert where the to_type is 'optional
    // any'.
    bool Bridge(MCValueRef value,
                MCValueRef& r_bridged_value);
    
	// Attempt to convert the input value to the specified type. If the conversion
	// cannot be performed because the type does not conform, then 'true' will
	// be returned, but r_new_value will be nil; allowing the context to throw
	// a more appropriate error.
	bool Convert(MCValueRef value,
				 MCTypeInfoRef to_type,
				 MCValueRef& r_new_value);
	bool ConvertToResolvedType(MCValueRef value,
							   const MCResolvedTypeInfo& t_to_type,
							   MCValueRef& r_new_value);
	
	// Attempt to unbox the given valueref. Unboxing will only succeed for types
	// which represent native types (e.g. foreign values, foreign handler types).
	// If value is nil, then it will initialize the slot appropriately to work
	// correctly with the slot type's drop. If a typecheck error occurs, then
	// slot_ptr is set to nil.
	bool UnboxingConvert(MCValueRef value,
						 const MCResolvedTypeInfo& slot_type,
						 void*& x_slot_ptr);
	
	// Attempt to box the given slot as a valueref.
	bool BoxingConvert(const MCResolvedTypeInfo& slot_type,
					   void *slot_ptr,
					   MCValueRef& r_value);
	
	// Resolve the typeinfo, throws an error if that fails.
	bool ResolveTypeInfo(MCTypeInfoRef typeinfo,
						 MCResolvedTypeInfo& r_resolved_typeinfo);
	
	// Resolve the given definition index to the instance and actual definition
	// it references (i.e. resolve the import-def chain).
	void ResolveDefinition(uindex_t index,
						   MCScriptInstanceRef& r_instance,
						   MCScriptDefinition*& r_definition) const;
	
	// Adopt the current thread's pending error as the context's error.
	void Rethrow(bool error = true);
	
	// Raise an 'unable to resolve multi-invoke' error.
	void ThrowUnableToResolveMultiInvoke(MCScriptDefinitionGroupDefinition *group,
	                                     MCSpan<const uindex_t> arguments);
	
	// Raise a 'value is not a handler' error.
	void ThrowNotAHandlerValue(MCValueRef actual_value);
	
	// Raise a 'value is not a string' error.
	void ThrowNotAStringValue(MCValueRef actual_value);
    
    // Raise a 'value is not a boolean or a bool' error.
    void ThrowNotABooleanOrBoolValue(MCValueRef actual_value);
    
    // Raise a 'local variable used before assigned' error.
    void ThrowLocalVariableUsedBeforeAssigned(uindex_t index);
    
    // Raise a 'invalid value for local variable' error.
    void ThrowInvalidValueForLocalVariable(uindex_t index,
                                           MCValueRef value);
    
    // Raise a 'local variable used before assigned' error.
    void ThrowGlobalVariableUsedBeforeAssigned(MCScriptInstanceRef instance,
                                               MCScriptVariableDefinition *definition);
    
    // Raise a 'invalid value for local variable' error.
    void ThrowInvalidValueForGlobalVariable(MCScriptInstanceRef instance,
                                            MCScriptVariableDefinition *definition,
                                            MCValueRef value);
    
    // Raise a 'wrong number of arguments' error.
    void ThrowWrongNumberOfArguments(MCScriptInstanceRef instance,
                                     MCScriptCommonHandlerDefinition *handler_def,
                                     uindex_t argument_count);
    
    // Raise an 'invalid value for argument' error
    void ThrowInvalidValueForArgument(MCScriptInstanceRef instance,
                                      MCScriptCommonHandlerDefinition *handler_def,
                                      uindex_t argument,
                                      MCValueRef value);
    
    // Raise an 'invalid value for return value' error
    void ThrowInvalidValueForReturnValue(MCScriptInstanceRef instance,
                                         MCScriptCommonHandlerDefinition *handler_def,
                                         MCValueRef value);
    
private:
	struct Frame
	{
		Frame *caller;
		
		MCScriptInstanceRef instance;
		
		MCScriptHandlerDefinition *handler;
		
		uindex_t return_address;
		
		MCValueRef *slots;
		
		uindex_t result;
		uindex_t *mapping;
        
        ~Frame(void);
	};
	
	/////////
	
	bool m_error;
	Frame *m_frame;
    const byte_t *m_bytecode_ptr;
	const byte_t *m_next_bytecode_ptr;
	bool m_operation_ready;
	MCScriptBytecodeOp m_operation;
	uindex_t m_arguments[256];
	uindex_t m_argument_count;

	// Owned by parent context
	MCSpan<MCValueRef> m_root_arguments;
	MCValueRef *m_root_result;
};

inline
MCScriptExecuteContext::MCScriptExecuteContext(void)
	: m_error(false),
	  m_frame(nil),
	  m_bytecode_ptr(nil),
	  m_next_bytecode_ptr(nil),
	  m_operation_ready(false),
	  m_operation(kMCScriptBytecodeOp__First),
	  m_argument_count(0),
	  m_root_arguments(nil),
      m_root_result(nil)
{
}

inline
MCScriptExecuteContext::~MCScriptExecuteContext(void)
{
	if (m_frame != nil)
	{
		while(m_frame != nil)
		{
			Frame *t_frame_to_delete = m_frame;
			m_frame = m_frame -> caller;
			delete t_frame_to_delete;
		}
	}
}

inline MCScriptBytecodeOp
MCScriptExecuteContext::GetOperation(void) const
{
	MCAssert(m_operation_ready);
	return m_operation;
}

inline uindex_t
MCScriptExecuteContext::GetAddress(void) const
{
    return uindex_t(m_bytecode_ptr - m_frame->instance->module->bytecode);
}

inline uindex_t
MCScriptExecuteContext::GetNextAddress(void) const
{
	return uindex_t(m_next_bytecode_ptr - m_frame->instance->module->bytecode);
}

inline uindex_t
MCScriptExecuteContext::GetArgumentCount(void) const
{
	return GetArguments().size();
}

inline uindex_t
MCScriptExecuteContext::GetArgument(uindex_t p_index) const
{
	return GetArguments()[p_index];
}

inline MCSpan<const uindex_t>
MCScriptExecuteContext::GetArguments() const
{
	MCAssert(m_operation_ready);
	return MCMakeSpan(m_arguments, m_argument_count);
}

inline index_t
MCScriptExecuteContext::GetSignedArgument(uindex_t p_index) const
{
	return MCScriptBytecodeDecodeSignedArgument(GetArgument(p_index));
}

inline MCTypeInfoRef
MCScriptExecuteContext::GetTypeOfRegister(uindex_t p_index) const
{
	MCScriptType **t_module_types;
	t_module_types = m_frame->instance->module->types;
	
	MCTypeInfoRef t_handler_signature;
	t_handler_signature = t_module_types[m_frame->handler->type]->typeinfo;
	
	uindex_t t_parameter_count;
	t_parameter_count = MCHandlerTypeInfoGetParameterCount(t_handler_signature);
	
	if (p_index < t_parameter_count)
	{
		return MCHandlerTypeInfoGetParameterType(t_handler_signature,
												 p_index);
	}
	else if (p_index < t_parameter_count + m_frame->handler->local_type_count)
	{
		uindex_t t_local_index;
		t_local_index = p_index - t_parameter_count;
		
		uindex_t t_local_type_index;
		t_local_type_index = m_frame->handler->local_types[t_local_index];
		
		return t_module_types[t_local_type_index]->typeinfo;
	}
	
	return nil;
}

inline MCTypeInfoRef
MCScriptExecuteContext::GetSignatureOfHandler(MCScriptInstanceRef p_instance,
											  MCScriptCommonHandlerDefinition *p_handler_def) const
{
	return p_instance->module->types[p_handler_def->type]->typeinfo;
}

inline MCValueRef
MCScriptExecuteContext::FetchValue(uindex_t p_index) const
{
	return m_frame->instance->module->values[p_index];
}

inline MCValueRef
MCScriptExecuteContext::FetchRegister(uindex_t p_index) const
{
	return m_frame->slots[p_index];
}

inline void
MCScriptExecuteContext::StoreRegister(uindex_t p_index,
									  MCValueRef p_value)
{
	if (m_error)
	{
		return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Store %p into register %u", p_value, p_index);
#endif
	
    MCValueRef& t_slot_ref = m_frame->slots[p_index];
	if (t_slot_ref == p_value)
        return;
    
    MCValueRelease(t_slot_ref);
    if (p_value != nil)
        MCValueRetain(p_value);
    t_slot_ref = p_value;
}

inline MCValueRef
MCScriptExecuteContext::FetchConstant(MCScriptInstanceRef p_instance,
									  MCScriptConstantDefinition *p_constant_def) const
{
	return p_instance->module->values[p_constant_def->value];
}

inline MCValueRef
MCScriptExecuteContext::FetchHandler(MCScriptInstanceRef p_instance,
									 MCScriptHandlerDefinition *p_handler_def)
{
	if (m_error)
	{
		return nil;
	}
	
	MCHandlerRef t_handler;
	if (!MCScriptEvaluateHandlerInInstanceInternal(p_instance,
												   p_handler_def,
												   t_handler))
	{
		Rethrow();
		return nil;
	}
	
	return t_handler;
}

inline MCValueRef
MCScriptExecuteContext::FetchForeignHandler(MCScriptInstanceRef p_instance,
											MCScriptForeignHandlerDefinition *p_handler_def)
{
	if (m_error)
	{
		return nil;
	}
	
	bool t_bound;
	if (!MCScriptTryToBindForeignHandlerInInstanceInternal(p_instance,
														   p_handler_def,
														   t_bound))
	{
		Rethrow();
		return nil;
	}

	if (!t_bound)
	{
		return kMCNull;
	}
	
	MCHandlerRef t_handler;
	if (!MCScriptEvaluateHandlerInInstanceInternal(p_instance,
												   p_handler_def,
												   t_handler))
	{
		Rethrow();
		return nil;
	}
	
	return t_handler;
}

inline MCValueRef
MCScriptExecuteContext::FetchVariable(MCScriptInstanceRef p_instance,
									  MCScriptVariableDefinition *p_variable_def) const
{
    return p_instance->slots[p_variable_def->slot_index];
}

inline void
MCScriptExecuteContext::StoreVariable(MCScriptInstanceRef p_instance,
                                      MCScriptVariableDefinition *p_variable_def,
                                      MCValueRef p_value)
{
	if (m_error)
	{
		return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Store %p into variable %u", p_value, p_variable_def->slot_index);
#endif
	
    MCValueRef& t_slot_ref = p_instance->slots[p_variable_def->slot_index];
    if (t_slot_ref == p_value)
        return;
    
    MCValueRelease(t_slot_ref);
    if (p_value != nil)
        MCValueRetain(p_value);
    t_slot_ref = p_value;
}

inline MCValueRef
MCScriptExecuteContext::CheckedFetchRegister(uindex_t p_index)
{
	if (m_error)
	{
		return nil;
	}
	
    MCValueRef t_value;
    t_value = FetchRegister(p_index);
    
    if (t_value == nil)
    {
        ThrowLocalVariableUsedBeforeAssigned(p_index);
        return nil;
    }
    
    return t_value;
}

inline bool
MCScriptExecuteContext::CheckedStoreRegister(uindex_t p_index,
                                             MCValueRef p_value)
{
	if (m_error)
	{
		return false;
	}
	
    MCTypeInfoRef t_type;
    t_type = GetTypeOfRegister(p_index);
	
	MCAutoValueRef t_converted_value;
    if (!Convert(p_value,
				 t_type,
				 &t_converted_value))
    {
		return false;
	}
	
	if (*t_converted_value == nil)
	{
        ThrowInvalidValueForLocalVariable(p_index,
										  p_value);
        return false;
    }
    
    StoreRegister(p_index,
                  *t_converted_value);
	
	return true;
}

inline bool
MCScriptExecuteContext::CheckedFetchRegisterAsBool(uindex_t p_index)
{
    MCValueRef t_value;
    t_value = CheckedFetchRegister(p_index);
    if (t_value == nil)
        return false;
    
    bool t_value_as_bool;
    if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
    {
        t_value_as_bool = (t_value == kMCTrue);
    }
    else if (MCValueGetTypeInfo(t_value) == kMCBoolTypeInfo ||
             MCValueGetTypeInfo(t_value) == kMCCBoolTypeInfo)
    {
        t_value_as_bool = *(bool *)MCForeignValueGetContentsPtr(t_value);
    }
    else
    {
        ThrowNotABooleanOrBoolValue(t_value);
        return false;
    }
    
    return t_value_as_bool;
}

inline MCValueRef
MCScriptExecuteContext::CheckedFetchVariable(MCScriptInstanceRef p_instance,
                                             MCScriptVariableDefinition *p_definition)
{
	if (m_error)
	{
		return nil;
	}
	
    MCValueRef t_value;
    t_value = FetchVariable(p_instance,
                            p_definition);
    
    if (t_value == nil)
    {
        ThrowGlobalVariableUsedBeforeAssigned(p_instance,
                                              p_definition);
        return nil;
    }
    
    return t_value;
}

inline void
MCScriptExecuteContext::CheckedStoreVariable(MCScriptInstanceRef p_instance,
                                             MCScriptVariableDefinition *p_definition,
                                             MCValueRef p_value)
{
	if (m_error)
	{
		return;
	}
	
    MCTypeInfoRef t_type;
    t_type = p_instance->module->types[p_definition->type]->typeinfo;
	
	MCAutoValueRef t_converted_value;
	if (!Convert(p_value,
				 t_type,
				 &t_converted_value))
	{
		return;
	}
	
	if (*t_converted_value == nil)
	{

        ThrowInvalidValueForGlobalVariable(p_instance,
                                           p_definition,
                                           p_value);
        return;
    }
    
    StoreVariable(p_instance,
                  p_definition,
                  *t_converted_value);
}

inline void
MCScriptExecuteContext::Jump(index_t p_offset)
{
    if (m_error)
	{
        return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Jump from %u to %u", GetAddress(), GetAddress() + p_offset);
#endif
	
    m_next_bytecode_ptr = m_bytecode_ptr + p_offset;
}

inline void
MCScriptExecuteContext::PushFrame(MCScriptInstanceRef p_instance,
                                  MCScriptHandlerDefinition *p_handler_def,
                                  uindex_t p_result_reg,
                                  MCSpan<const uindex_t> p_argument_regs)
{
    if (m_error)
	{
        return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Push frame for handler %u", p_handler_def->index);
#endif
	
    MCAutoPointer<Frame> t_new_frame = new (nothrow) Frame;
    if (*t_new_frame == nil ||
        !MCMemoryNewArray(p_handler_def->slot_count,
                          t_new_frame->slots))
    {
        Rethrow();
        return;
    }
	
    // Set up the new frame. Notice that we don't retain the instance - this is
    // because there is no need. If we are executing a handler in an instance then
    // either it will be in a module used by the current module - in which case the
    // reference retained by the entry point to the VM is sufficient - or it will
    // be a via retained handler ref in a register.
    t_new_frame->caller = m_frame;
    t_new_frame->instance = p_instance;
    t_new_frame->handler = p_handler_def;
    t_new_frame->return_address = GetNextAddress();
    t_new_frame->result = p_result_reg;
    t_new_frame->mapping = nil;
    
    // Fetch the handler signature.
    MCTypeInfoRef t_signature =
        GetSignatureOfHandler(p_instance,
                              p_handler_def);
    
    // Check the parameter count.
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_argument_regs.size())
    {
        ThrowWrongNumberOfArguments(p_instance,
                                    p_handler_def,
                                    p_argument_regs.size());
        return;
    }
    
    // Now initialize the parameter slots, taking note of whether we need
    // a mapping array.
    bool t_needs_mapping = false;
    for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); ++i)
    {
        MCHandlerTypeFieldMode t_param_mode =
            MCHandlerTypeInfoGetParameterMode(t_signature,
                                              i);
        
        MCTypeInfoRef t_param_type =
            MCHandlerTypeInfoGetParameterType(t_signature,
                                              i);
        
        // If we have an out or inout argument we need a mapping array.
        if (t_param_mode != kMCHandlerTypeFieldModeIn)
        {
            t_needs_mapping = true;
        }
		
		// If we have an out parameter, then there is no initial value to
		// assign - default init is done in bytecode directly.
		if (t_param_mode == kMCHandlerTypeFieldModeOut)
		{
			continue;
		}
		
		// Ensure the value we pass into the new frame is of the correct
		// type.
		MCValueRef t_incoming_value =
				CheckedFetchRegister(p_argument_regs[i]);
		
		// If nil, then the value is unassigned and thus an error has already
		// occurred.
		if (t_incoming_value == nil)
		{
			return;
		}
		
		// Attempt to convert the value to the required slot type.
		MCAutoValueRef t_initial_value;
		if (!Convert(t_incoming_value,
					 t_param_type,
					 &t_initial_value))
		{
			return;
		}
		
		// If the converted value is nil, then it is an invalid conversion.
		if (*t_initial_value == nil)
		{
			ThrowInvalidValueForArgument(p_instance,
										 p_handler_def,
										 i,
										 t_incoming_value);
			return;
        }
        
        // We now have the initial value, in the type required for the target
		// parameter slot.
		t_new_frame->slots[i] = t_initial_value.Take();
    }
    
    //
    
    // If there are any out or inout parameters, we need to store the
    // caller registers that were passed so we can copy back at the end.
    if (t_needs_mapping)
    {
	    if (!MCMemoryNewArray(p_argument_regs.size(),
                              t_new_frame->mapping))
        {
            Rethrow();
            return;
        }
        
        MCMemoryCopy(t_new_frame->mapping,
                     p_argument_regs.data(),
                     p_argument_regs.sizeBytes());
    }
    
    // Finally, make the new frame the current frame and set the
    // new bytecode ptr.
    t_new_frame.Take(m_frame);
    m_next_bytecode_ptr = p_instance->module->bytecode + p_handler_def->start_address;
}

inline void
MCScriptExecuteContext::PopFrame(uindex_t p_result_reg)
{
	if (m_error)
	{
		return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Pop frame");
#endif
	
	///// Process callee side of the frame.
	
	// Get the signature of the frame.
	MCTypeInfoRef t_signature =
		GetSignatureOfHandler(m_frame->instance,
							  m_frame->handler);
	
	// Get the parameter count.
	uindex_t t_parameter_count =
		MCHandlerTypeInfoGetParameterCount(t_signature);
	
	// First fetch the return value, if any. This will throw an error if
	// the slot is unassigned - type checking should occur in the caller
	// frame (after the pop).
	MCValueRef t_return_value;
	if (p_result_reg != UINDEX_MAX)
	{
		t_return_value = CheckedFetchRegister(p_result_reg);
		if (t_return_value == nil)
			return;
	}
	else
	{
		t_return_value = kMCNull;
	}
	
	// Now double check that all the out parameters are assigned - if any.
	if (m_frame->mapping != nil)
	{
		for(uindex_t i = 0; i < t_parameter_count; i++)
		{
			MCHandlerTypeFieldMode t_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
			
			// Ignore any in mode parameters.
			if (t_mode == kMCHandlerTypeFieldModeIn)
			{
				continue;
			}
			
			// Now try to fetch the value.
			MCValueRef t_out_value =
				CheckedFetchRegister(i);
			if (t_out_value == nil)
			{
				return;
			}
		}
	}
	
	// Convert the return value to the required type.
	MCAutoValueRef t_converted_return_value;
	if (!Convert(t_return_value,
				 MCHandlerTypeInfoGetReturnType(t_signature),
				 &t_converted_return_value))
	{
		return;
	}
	
	if (*t_converted_return_value == nil)
	{
		ThrowInvalidValueForReturnValue(m_frame->instance,
										m_frame->handler,
										t_return_value);
		return;
	}
	
	///// Process caller side of the frame.
	
    // Unlink the frame - this means subsequent errors will be reported against
	// the caller.
    MCAutoPointer<Frame> t_popped_frame(m_frame);
    m_frame = m_frame->caller;
	
	// What we do now depends on whether this is the root frame or not.
	if (m_frame != nil)
	{
		// Copy back the return value.
		if (!CheckedStoreRegister(t_popped_frame->result,
								  *t_converted_return_value))
		{
			return;
		}
		
		// Now copy back the out parameters - if any.
		if (t_popped_frame->mapping != nil)
		{
			for(uindex_t i = 0; i < t_parameter_count; i++)
			{
				MCHandlerTypeFieldMode t_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
				
				// Ignore any in mode parameters.
				if (t_mode == kMCHandlerTypeFieldModeIn)
				{
					continue;
				}
				
				if (!CheckedStoreRegister(t_popped_frame->mapping[i],
										   t_popped_frame->slots[i]))
				{
					return;
				}
			}
		}
		
		m_next_bytecode_ptr = m_frame->instance->module->bytecode + t_popped_frame->return_address;
	}
	else
	{
		if (m_root_result != nil)
			*m_root_result = MCValueRetain(t_return_value);
		
		if (!m_root_arguments.empty())
		{
			for(uindex_t i = 0; i < t_parameter_count; i++)
			{
				MCHandlerTypeFieldMode t_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
				
				// Ignore any in mode parameters.
				if (t_mode == kMCHandlerTypeFieldModeIn)
				{
					continue;
				}
				
				MCValueAssign(m_root_arguments[i],
							  t_popped_frame->slots[i]);
			}
		}
		
		m_next_bytecode_ptr = nil;
	}
}

inline void
MCScriptExecuteContext::Enter(MCScriptInstanceRef p_instance,
							  MCScriptHandlerDefinition *p_handler_def,
                              MCSpan<MCValueRef> p_arguments,
							  MCValueRef *r_value)
{
	if (m_error)
	{
		return;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Enter frame for handler %u", p_handler_def->index);
#endif
	
	MCAutoPointer<Frame> t_new_frame = new (nothrow) Frame;
	if (*t_new_frame == nil ||
		!MCMemoryNewArray(p_handler_def->slot_count,
						  t_new_frame->slots))
	{
		Rethrow();
		return;
	}
	
	// Set up the root frame.
	t_new_frame->caller = nil;
	t_new_frame->instance = p_instance;
	t_new_frame->handler = p_handler_def;
	t_new_frame->return_address = UINDEX_MAX;
	t_new_frame->result = 0;
	t_new_frame->mapping = nil;
	
	// Fetch the handler signature.
	MCTypeInfoRef t_signature =
			GetSignatureOfHandler(p_instance,
								  p_handler_def);
	
	// Check the parameter count.
	if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_arguments.size())
	{
		ThrowWrongNumberOfArguments(p_instance,
									p_handler_def,
		                            p_arguments.size());
		return;
	}

	// Copy the arguments into the parameter slots.
	for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); ++i)
	{
		MCHandlerTypeFieldMode t_param_mode =
				MCHandlerTypeInfoGetParameterMode(t_signature,
												  i);
	
		if (t_param_mode == kMCHandlerTypeFieldModeOut)
		{
			continue;
		}
		
		MCTypeInfoRef t_param_type =
				MCHandlerTypeInfoGetParameterType(t_signature,
												  i);
		
		// Attempt to convert the value to the required slot type.
		MCAutoValueRef t_initial_value;
		if (!Convert(p_arguments[i],
					 t_param_type,
					 &t_initial_value))
		{
			return;
		}
		
		// If the converted value is nil, then it is an invalid conversion.
		if (*t_initial_value == nil)
		{
			ThrowInvalidValueForArgument(p_instance,
										 p_handler_def,
										 i,
										 p_arguments[i]);
			return;
		}
		
		// We now have the initial value, in the type required for the target
		// parameter slot.
		t_new_frame->slots[i] = t_initial_value.Take();
	}
	
	// Setup the execution state.
	t_new_frame.Take(m_frame);
	m_bytecode_ptr = nil;
	m_next_bytecode_ptr = p_instance->module->bytecode + p_handler_def->start_address;

	m_root_arguments = p_arguments;
	m_root_result = r_value;
	
	return;
}

inline bool
MCScriptExecuteContext::Leave(void)
{
	// If an error occurred, we must unwind the LCB stack.
	if (m_error)
	{
		// Catch the pending error on the thread. If there is no pending error
		// it means that something has failed to actually throw an error when
		// the error state arose.
		MCAutoErrorRef t_error;
		if (!MCErrorCatch(&t_error))
		{
			// If this call fails then it will be due to oom, there is nothing
			// more we can do so we just return.
			if (!MCScriptCreateErrorExpectedError(&t_error))
			{
				return false;
			}
		}
		
		// Now we loop back through the frames, adding the backtrace
		// information as we go.
		
		// Position information is stored in modules as a list of triples
		// (address, file, line) sorted by address. The address is that
		// of the first instruction generated for a given source line. Each
		// frame stores the return_address which is the address of the
		// instruction after the invoke which caused the new frame. Therefore
		// to find the position corresponding to each frame, we look
		// for the last position whose address is strictly less than the
		// return_address.
		
		// As the address of the current frame is stored in the context,
		// we fetch it here, and replace by the return_address as we step
		// back up the frames.
		uindex_t t_address;
		t_address = m_frame != nil ? GetNextAddress() : 0;
		while(m_frame != nil)
		{
			const MCScriptModuleRef t_module =
			m_frame->instance->module;
			
			// If the module of the frame has debug info then we add the
			// source info, otherwise we just skip the frame.
			if (t_module->position_count > 0)
			{
				uindex_t t_index;
				for(t_index = 0; t_index < t_module->position_count - 1; t_index++)
				{
					if (t_module->positions[t_index + 1].address >= t_address)
					{
						break;
					}
				}
				
				const MCScriptPosition *t_position =
				&t_module->positions[t_index];
				
				// If unwinding fails (due to oom), then we cease processing
				// and drop the error, allowing the new error to be propagate.
				if (!MCErrorUnwind(*t_error,
								   t_module->source_files[t_position->file],
								   t_position->line,
								   1))
				{
					t_error.Reset();
					break;
				}
			}
			
			// Move to the calling frame.
			Frame *t_current_frame = m_frame;
			m_frame = m_frame->caller;
			t_address = t_current_frame->return_address;
			delete t_current_frame;
		}
		
		// If an error occurred whilst unwinding, another error will have been
		// thrown, so we just return.
		if (*t_error == nil)
		{
			return false;
		}
		
		// Rethrow the error that we have unwound.
		MCErrorThrow(*t_error);
		
		return false;
	}
	
#ifdef MCSCRIPT_DEBUG_EXECUTE
	MCLog("Leave frame for handler %u", p_handler_def->index);
#endif
	
	return true;
}

inline bool
MCScriptExecuteContext::Step(void)
{
	if (m_error)
	{
		return false;
	}
	
	if (m_frame == nil)
	{
		return false;
	}
	
	m_bytecode_ptr = m_next_bytecode_ptr;
	
	MCScriptBytecodeDecode(m_next_bytecode_ptr,
						   m_operation,
						   m_arguments,
						   m_argument_count);

	m_operation_ready = !m_error;
	return m_operation_ready;
}

inline bool
MCScriptExecuteContext::ResolveTypeInfo(MCTypeInfoRef p_typeinfo,
										MCResolvedTypeInfo& r_resolved_typeinfo)
{
	if (!MCTypeInfoResolve(p_typeinfo, r_resolved_typeinfo))
	{
		Rethrow(MCErrorThrowUnboundType(p_typeinfo));
		return false;
	}
	
	return true;
}

inline void
MCScriptExecuteContext::ResolveDefinition(uindex_t p_index,
										  MCScriptInstanceRef& r_instance,
										  MCScriptDefinition*& r_definition) const
{
	MCScriptDefinition *t_definition;
	t_definition = m_frame->instance->module->definitions[p_index];
	
	MCScriptInstanceRef t_instance;
	if (t_definition->kind != kMCScriptDefinitionKindExternal)
	{
		t_instance = m_frame->instance;
	}
	else
	{
		MCScriptExternalDefinition *t_ext_def;
		t_ext_def = static_cast<MCScriptExternalDefinition *>(t_definition);
		
		MCScriptImportedDefinition *t_import_def;
		t_import_def = &m_frame->instance->module->imported_definitions[t_ext_def->index];
		
		t_instance = t_import_def->resolved_module->shared_instance;
		t_definition = t_import_def->resolved_definition;
	}
	
	r_instance = t_instance;
	r_definition = t_definition;
}

inline void
MCScriptExecuteContext::Rethrow(bool /*ignored*/)
{
	m_error = true;
}

//////////

inline void
MCScriptExecuteContext::ThrowUnableToResolveMultiInvoke(MCScriptDefinitionGroupDefinition *p_group,
                                                        MCSpan<const uindex_t> p_arguments)
{
	MCAutoProperListRef t_args;
	if (!MCProperListCreateMutable(&t_args))
		return;

    for (const uindex_t t_argument : p_arguments)
    {
		MCValueRef t_value;
		t_value = FetchRegister(t_argument);
		
		if (t_value == nil)
		{
			t_value = kMCNull;
		}
		
		if (!MCProperListPushElementOntoBack(*t_args,
											 t_value))
			return;
	}
	
	Rethrow(MCScriptThrowUnableToResolveMultiInvokeError(m_frame->instance,
														 p_group,
														 *t_args));
}

inline void
MCScriptExecuteContext::ThrowNotAHandlerValue(MCValueRef p_actual_value)
{
	Rethrow(MCScriptThrowNotAHandlerValueError(p_actual_value));
}

inline void
MCScriptExecuteContext::ThrowNotAStringValue(MCValueRef p_actual_value)
{
	Rethrow(MCScriptThrowNotAStringValueError(p_actual_value));
}

inline void
MCScriptExecuteContext::ThrowNotABooleanOrBoolValue(MCValueRef p_actual_value)
{
	Rethrow(MCScriptThrowNotABooleanOrBoolValueError(p_actual_value));
}

inline void
MCScriptExecuteContext::ThrowLocalVariableUsedBeforeAssigned(uindex_t p_index)
{
	Rethrow(MCScriptThrowLocalVariableUsedBeforeAssignedError(m_frame->instance,
															  m_frame->handler,
															  p_index));
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForLocalVariable(uindex_t p_index,
														  MCValueRef p_value)
{
	Rethrow(MCScriptThrowInvalidValueForLocalVariableError(m_frame->instance,
														   m_frame->handler,
														   p_index,
														   p_value));
}


inline void
MCScriptExecuteContext::ThrowGlobalVariableUsedBeforeAssigned(MCScriptInstanceRef p_instance,
															  MCScriptVariableDefinition *p_variable_def)
{
	Rethrow(MCScriptThrowGlobalVariableUsedBeforeAssignedError(p_instance,
															   p_variable_def));
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForGlobalVariable(MCScriptInstanceRef p_instance,
														   MCScriptVariableDefinition *p_variable_def,
														   MCValueRef p_value)
{
	Rethrow(MCScriptThrowInvalidValueForGlobalVariableError(p_instance,
															p_variable_def,
															p_value));
}

inline void
MCScriptExecuteContext::ThrowWrongNumberOfArguments(MCScriptInstanceRef p_instance,
													MCScriptCommonHandlerDefinition *p_handler_def,
													uindex_t p_argument_count)
{
	Rethrow(MCScriptThrowWrongNumberOfArgumentsError(p_instance,
													 p_handler_def,
													 p_argument_count));
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForArgument(MCScriptInstanceRef p_instance,
													 MCScriptCommonHandlerDefinition *p_handler_def,
													 uindex_t p_argument,
													 MCValueRef p_value)
{
	Rethrow(MCScriptThrowInvalidValueForArgumentError(p_instance,
													  p_handler_def,
													  p_argument,
													  p_value));
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForReturnValue(MCScriptInstanceRef p_instance,
														MCScriptCommonHandlerDefinition *p_handler_def,
														MCValueRef p_value)
{
	Rethrow(MCScriptThrowInvalidValueForReturnValueError(p_instance,
														 p_handler_def,
														 p_value));
}

//////////

inline
MCScriptExecuteContext::Frame::~Frame(void)
{
	if (slots != nil)
	{
		for(uindex_t i = 0; i < handler->slot_count; i++)
		{
			MCValueRelease(slots[i]);
		}
		MCMemoryDeleteArray(slots);
	}
	
	if (mapping != nil)
	{
		MCMemoryDeleteArray(mapping);
	}
}

//////////
