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

class MCScriptExecuteContext
{
public:
	// Constructor.
	MCScriptExecuteContext(void);
	
	// Destructor.
	~MCScriptExecuteContext(void);
	
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
	void CheckedStoreRegister(uindex_t index,
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
				   const uindex_t *argument_regs,
				   uindex_t argument_count);
	
	// Pop the current activation frame and set the return value to the contents
	// of the given register. If result_reg is UINDEX_MAX, then it means there is
	// no return value.
	void PopFrame(uindex_t result_reg);
	
	// Invoke a foreign function with the given arguments, taken from registers
	// returning the value into the result register.
	void InvokeForeign(MCScriptInstanceRef instance,
					   MCScriptForeignHandlerDefinition *handler,
					   uindex_t result_reg,
					   const uindex_t *argument_regs,
					   uindex_t argument_count);
	
	// Enter the LCB VM to execute the specified handler in the given instance.
	void Enter(MCScriptInstanceRef instance,
			   MCScriptHandlerDefinition *handler,
			   MCValueRef *arguments,
			   uindex_t argument_count,
			   MCValueRef *r_result);
	
	// Decode the next bytecode to execute. If there is more execution to do,
	// then true is returned. If execution has finished or an error occurred,
	// false is returned.
	bool Step(void);
	
	// Leave the LCB VM. If a execution completed successfully then true is
	// returned, otherwise false is returned.
	bool Leave(void);
	
	// Resolve the given definition index to the instance and actual definition
	// it references (i.e. resolve the import-def chain).
	void ResolveDefinition(uindex_t index,
						   MCScriptInstanceRef& r_instance,
						   MCScriptDefinition*& r_definition) const;
	
	// Adopt the current thread's pending error as the context's error.
	void Rethrow(void);
	
	// Raise an 'unable to resolve multi-invoke' error.
	void ThrowUnableToResolveMultiInvoke(MCScriptDefinitionGroupDefinition *group,
										 const uindex_t *arguments,
										 uindex_t argument_count);
	
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

	// Raise an 'error expected' error.
	void ThrowErrorExpected(void);
	
	//////////
    
    struct InternalHandlerContext
    {
        MCScriptInstanceRef instance;
        MCScriptCommonHandlerDefinition *definition;
    };

    // Returns true if the handler-ref is of 'internal' type - i.e. it is
    // one created by the LCB VM.
    static bool IsInternalHandler(MCHandlerRef handler);
    
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
	
	bool TryToBindForeignHandler(MCScriptInstanceRef instance,
								 MCScriptForeignHandlerDefinition *handler_def,
								 bool& r_bound);
	
	bool EvaluateHandler(MCScriptInstanceRef instance,
						 MCScriptCommonHandlerDefinition *handler_def,
						 MCHandlerRef& r_handler);
	
	void Unwind(void);
	
	/////////
	
	bool m_error;
	Frame *m_frame;
    const byte_t *m_bytecode_ptr;
	const byte_t *m_next_bytecode_ptr;
	MCScriptBytecodeOp m_operation;
	uindex_t m_arguments[256];
	uindex_t m_argument_count;
	
	MCValueRef *m_root_arguments;
	MCValueRef *m_root_result;
    
	/////////
		
	static const MCHandlerCallbacks kInternalHandlerCallbacks;
	static bool InternalHandlerCreate(MCScriptInstanceRef instance,
									  MCScriptCommonHandlerDefinition *definition,
									  MCHandlerRef& r_handler);
	static void InternalHandlerRelease(void *context);
	static bool InternalHandlerInvoke(void *context,
									  MCValueRef *arguments,
									  uindex_t argument_count,
									  MCValueRef& r_return_value);
	static bool InternalHandlerDescribe(void *context,
										MCStringRef& r_description);
};

inline
MCScriptExecuteContext::MCScriptExecuteContext(void)
	: m_frame(nil),
	  m_bytecode_ptr(nil),
	  m_next_bytecode_ptr(nil),
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
	return m_argument_count;
}

inline uindex_t
MCScriptExecuteContext::GetArgument(uindex_t p_index) const
{
	return m_arguments[p_index];
}

inline const uindex_t *
MCScriptExecuteContext::GetArgumentList(void) const
{
	return m_arguments;
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
	if (!EvaluateHandler(p_instance,
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
	if (!TryToBindForeignHandler(p_instance,
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
	if (!EvaluateHandler(p_instance,
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

inline void
MCScriptExecuteContext::CheckedStoreRegister(uindex_t p_index,
                                             MCValueRef p_value)
{
	if (m_error)
	{
		return;
	}
	
    MCTypeInfoRef t_type;
    t_type = GetTypeOfRegister(p_index);
    
    if (t_type != nil &&
        !MCTypeInfoConforms(MCValueGetTypeInfo(p_value),
                            t_type))
    {
        ThrowInvalidValueForLocalVariable(p_index, p_value);
        return;
    }
    
    StoreRegister(p_index,
                  p_value);
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
    else if (MCValueGetTypeInfo(t_value) == kMCBoolTypeInfo)
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
    
    if (t_type != nil &&
        !MCTypeInfoConforms(MCValueGetTypeInfo(p_value),
                            t_type))
    {
        ThrowInvalidValueForGlobalVariable(p_instance,
                                           p_definition,
                                           p_value);
        return;
    }
    
    StoreVariable(p_instance,
                  p_definition,
                  p_value);
}

inline void
MCScriptExecuteContext::Jump(index_t p_offset)
{
    if (m_error)
	{
        return;
	}
	
    m_next_bytecode_ptr = m_bytecode_ptr + p_offset;
}

inline void
MCScriptExecuteContext::PushFrame(MCScriptInstanceRef p_instance,
                                  MCScriptHandlerDefinition *p_handler_def,
                                  uindex_t p_result_reg,
                                  const uindex_t *p_argument_regs,
                                  uindex_t p_argument_count)
{
    if (m_error)
	{
        return;
	}
	
    MCAutoPointer<Frame> t_new_frame;
    if (!MCMemoryNew(&t_new_frame) ||
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
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_argument_count)
    {
        ThrowWrongNumberOfArguments(p_instance,
                                    p_handler_def,
                                    p_argument_count);
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
        
        // If we have an in or inout argument we need to check the incoming
        // value type conforms, and that is used to initialize the slot.
        // Otherwise, we do nothing as the initialization of out parameters
        // and local variables is done in bytecode directly.
        MCValueRef t_initial_value = nil;
        if (t_param_mode != kMCHandlerTypeFieldModeOut)
        {
            MCValueRef t_incoming_value =
                CheckedFetchRegister(p_argument_regs[i]);
            if (t_incoming_value == nil)
            {
                return;
            }
            
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_incoming_value),
                                    t_param_type))
            {
                ThrowInvalidValueForArgument(p_instance,
                                             p_handler_def,
                                             i,
                                             t_incoming_value);
                return;
            }
        }
        else
        {
            continue;
        }
        
        // We now have the initial value, the type of which we *know*
        // conforms to the parameter type, so we can store into the
        // slot.
		// BRIDGE: This should bridge (convert) the type if necessary.
        t_new_frame->slots[i] = MCValueRetain(t_initial_value);
    }
    
    //
    
    // If there are any out or inout parameters, we need to store the
    // caller registers that were passed so we can copy back at the end.
    if (t_needs_mapping)
    {
        if (!MCMemoryNewArray(p_argument_count,
                              t_new_frame->mapping))
        {
            Rethrow();
            return;
        }
        
        MCMemoryCopy(t_new_frame->mapping,
                     p_argument_regs,
                     p_argument_count * sizeof(p_argument_regs[0]));
    }
    
    // Finally, make the new frame the current frame and set the
    // new bytecode ptr.
    t_new_frame.Take(m_frame);
    m_bytecode_ptr = p_instance->module->bytecode + p_handler_def->start_address;
}

inline void
MCScriptExecuteContext::PopFrame(uindex_t p_result_reg)
{
	if (m_error)
	{
		return;
	}
	
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
	
	// Check the return value type conforms, and copy it back if it does.
	if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_return_value),
							MCHandlerTypeInfoGetReturnType(t_signature)))
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
		// BRIDGE: This should bridge (convert) the type if necessary.
		CheckedStoreRegister(t_popped_frame->result, t_return_value);
		
		// Now copy back the out parameters - if any.
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
				
				// BRIDGE: This should bridge (convert) the type if necessary.
				CheckedStoreRegister(t_popped_frame->mapping[i],
									 t_popped_frame->slots[i]);
			}
		}
		
		m_bytecode_ptr = m_frame->instance->module->bytecode + t_popped_frame->return_address;
	}
	else
	{
		if (m_root_result != nil)
			*m_root_result = MCValueRetain(t_return_value);
		
		if (m_root_arguments != nil)
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
		
		m_bytecode_ptr = nil;
	}
}

inline void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_definition,
									  uindex_t p_result_reg,
									  const uindex_t *p_argument_regs,
									  uindex_t p_argument_count)
{
}

inline void
MCScriptExecuteContext::Enter(MCScriptInstanceRef p_instance,
							  MCScriptHandlerDefinition *p_handler_def,
							  MCValueRef *p_arguments,
							  uindex_t p_argument_count,
							  MCValueRef *r_value)
{
	if (m_error)
	{
		return;
	}
	
	MCAutoPointer<Frame> t_new_frame;
	if (!MCMemoryNew(&t_new_frame) ||
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
	if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_argument_count)
	{
		ThrowWrongNumberOfArguments(p_instance,
									p_handler_def,
									p_argument_count);
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
		
		if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_arguments[i]),
								t_param_type))
		{
			ThrowInvalidValueForArgument(p_instance,
										 p_handler_def,
										 i,
										 p_arguments[i]);
			return;
		}
		
		// BRIDGE: This should bridge (convert) the type if necessary.
		MCValueAssign(t_new_frame->slots[i],
					  p_arguments[i]);
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
		Unwind();
		return false;
	}
	
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
	
	return m_error;
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
MCScriptExecuteContext::Rethrow(void)
{
	m_error = true;
}

inline bool
MCScriptExecuteContext::IsInternalHandler(MCHandlerRef p_handler)
{
	return MCHandlerGetCallbacks(p_handler) == &kInternalHandlerCallbacks;
}

inline void
MCScriptExecuteContext::ThrowUnableToResolveMultiInvoke(MCScriptDefinitionGroupDefinition *p_group,
														const uindex_t *p_arguments,
														uindex_t p_argument_count)
{
	m_error = MCScriptThrowUnableToResolveMultiInvokeError(m_frame->instance,
														   p_group,
														   p_arguments,
														   p_argument_count);
}

inline void
MCScriptExecuteContext::ThrowNotAHandlerValue(MCValueRef p_actual_value)
{
	m_error = MCScriptThrowNotAHandlerValueError(p_actual_value);
}

inline void
MCScriptExecuteContext::ThrowNotAStringValue(MCValueRef p_actual_value)
{
	m_error = MCScriptThrowNotAStringValueError(p_actual_value);
}

inline void
MCScriptExecuteContext::ThrowNotABooleanOrBoolValue(MCValueRef p_actual_value)
{
	m_error = MCScriptThrowNotABooleanOrBoolValueError(p_actual_value);
}

inline void
MCScriptExecuteContext::ThrowLocalVariableUsedBeforeAssigned(uindex_t p_index)
{
	m_error = MCScriptThrowLocalVariableUsedBeforeAssignedError(m_frame->instance,
																p_index);
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForLocalVariable(uindex_t p_index,
														  MCValueRef p_value)
{
	m_error = MCScriptThrowInvalidValueForLocalVariableError(m_frame->instance,
															 p_index,
															 p_value);
}


inline void
MCScriptExecuteContext::ThrowGlobalVariableUsedBeforeAssigned(MCScriptInstanceRef p_instance,
															  MCScriptVariableDefinition *p_variable_def)
{
	m_error = MCScriptThrowGlobalVariableUsedBeforeAssignedError(p_instance,
																 p_variable_def);
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForGlobalVariable(MCScriptInstanceRef p_instance,
														   MCScriptVariableDefinition *p_variable_def,
														   MCValueRef p_value)
{
	m_error = MCScriptThrowInvalidValueForGlobalVariableError(p_instance,
															  p_variable_def);
}

inline void
MCScriptExecuteContext::ThrowWrongNumberOfArguments(MCScriptInstanceRef p_instance,
													MCScriptCommonHandlerDefinition *p_handler_def,
													uindex_t p_argument_count)
{
	m_error = MCScriptThrowWrongNumberOfArgumentsError(p_instance,
													   p_handler_def,
													   p_argument_count);
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForArgument(MCScriptInstanceRef p_instance,
													 MCScriptCommonHandlerDefinition *p_handler_def,
													 uindex_t p_argument,
													 MCValueRef p_value)
{
	m_error = MCScriptThrowInvalidValueForArgumentError(p_instance,
														p_handler_def,
														p_argument,
														p_value);
}

inline void
MCScriptExecuteContext::ThrowInvalidValueForReturnValue(MCScriptInstanceRef p_instance,
														MCScriptCommonHandlerDefinition *p_handler_def,
														MCValueRef p_value)
{
	m_error = MCScriptThrowInvalidValueForReturnValueError(p_instance,
														   p_handler_def,
														   p_value);
}

inline void
MCScriptExecuteContext::ThrowErrorExpected(void)
{
	m_error = MCScriptThrowErrorExpectedError();
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
