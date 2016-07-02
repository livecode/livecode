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
	// Default constructor.
	MCScriptExecuteContext(void);
	
	// Return the address of the current opcode
	uindex_t GetAddress(void) const;
	
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
	
	// Pop the current activation frame and set the return value to the

	// Note: The return_value will be copied before any frame is deleted, this
	// means that it can be an unretained value from a register in the current
	// frame.
	void PopFrame(MCValueRef return_value);
	
	// Invoke a foreign function with the given arguments, taken from registers
	// returning the value into the result register.
	void InvokeForeign(MCScriptInstanceRef instance,
					   MCScriptForeignHandlerDefinition *handler,
					   uindex_t result_reg,
					   const uindex_t *argument_regs,
					   uindex_t argument_count);
	
	// Resolve the given definition index to the instance and actual definition
	// it references (i.e. resolve the import-def chain).
	void ResolveDefinition(uindex_t index,
						   MCScriptInstanceRef& r_instance,
						   MCScriptDefinition*& r_definition) const;
	
	// Resolve a type (follow the alias / naming chain to derive the root type
	// and optionality). If resolution fails, then an error will be thrown and
	// the function will return false.
	bool ResolveType(MCTypeInfoRef type,
					 MCResolvedTypeInfo& r_resolved_type);
	
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
	
	/////////
	
	bool m_error;
	Frame *m_frame;
    byte_t *m_bytecode_ptr;
	MCScriptBytecodeOp m_operation;
	uindex_t m_arguments[256];
	uindex_t m_argument_count;
    
	/////////
		
	static const MCHandlerCallbacks kInternalHandlerCallbacks;
	static bool InternalHandlerCreate(MCScriptInstanceRef instance,
									  MCScriptCommonHandlerDefinition *definition,
									  MCHandlerRef& r_handler);
	static bool InternalHandlerInvoke(void *context,
									  MCValueRef *arguments,
									  uindex_t argument_count,
									  MCValueRef& r_return_value);
	static bool InternalHandlerDescribe(void *context,
										MCStringRef& r_description);
};

inline
MCScriptExecuteContext::MCScriptExecuteContext(void)
	: m_frame(nil)
{
}

inline uindex_t
MCScriptExecuteContext::GetAddress(void) const
{
    return uindex_t(m_bytecode_ptr - m_frame->instance->module->bytecode);
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
        return;
    
    m_bytecode_ptr += p_offset;
}

inline void
MCScriptExecuteContext::PushFrame(MCScriptInstanceRef p_instance,
                                  MCScriptHandlerDefinition *p_handler_def,
                                  uindex_t p_result_reg,
                                  const uindex_t *p_argument_regs,
                                  uindex_t p_argument_count)
{
    if (m_error)
        return;
    
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
    t_new_frame->return_address = GetAddress();
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
MCScriptExecuteContext::PopFrame(MCValueRef p_value)
{
    // Unlink the frame.
    MCAutoPointer<Frame> t_popped_frame(m_frame);
    m_frame = m_frame->caller;
    
    // Get the handler signature.
    MCTypeInfoRef t_signature =
        GetSignatureOfHandler(t_popped_frame->instance,
                              t_popped_frame->handler);
    
    // Check the return value type conforms, and copy it back if it does.
    if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value),
                            MCHandlerTypeInfoGetReturnType(t_signature)))
    {
        ThrowInvalidValueForReturnValue(t_popped_frame->instance,
                                        t_popped_frame->handler,
                                        p_value);
        return;
    }
    CheckedStoreRegister(t_popped_frame->result, p_value);
    
    // Now copy back the out parameters.
    for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
    {
        
    }
}

#if 0
	
	MCTypeInfoRef t_handler_signature;
	t_handler_signature = p_instance -> module -> types[p_handler_def -> type] -> typeinfo;
	
	__MCScriptHandlerContext t_context;
	t_context.instance = p_instance;
	t_context.definiiton = p_handler_def;
#endif