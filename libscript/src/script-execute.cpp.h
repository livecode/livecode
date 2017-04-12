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
	// Return the address of the current opcode
	uindex_t GetAddress(void) const;
	
	// Return the number of arguments to the current opcode
	uindex_t GetArgumentCount(void) const;
	
	// Return the argument at the given index as unsigned
	uindex_t GetArgument(uindex_t index) const;
	
	// Return the list of arguments to the current opcode
	uindex_t *GetArgumentList(void) const;
	
	// Return the argument at the given index as signed
	index_t GetSignedArgument(uindex_t index) const;
	
	// Fetch the type of the given register. The type of the register might
	// be nil, if it has no assigned type.
	MCTypeInfoRef GetTypeOfRegister(uindex_t index) const;
	
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
					   MCScriptVariableDefinition *definition);
	
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
				   uindex_t *argument_regs,
				   uindex_t argument_count);
	
	// Pop the current activation frame and set the return value.
	// Note: The return_value will be copied before any frame is deleted, this
	// means that it can be an unretained value from a register in the current
	// frame.
	void PopFrame(MCValueRef return_value);
	
	// Invoke a foreign function with the given arguments, taken from registers
	// returning the value into the result register.
	void InvokeForeign(MCScriptInstanceRef instance,
					   MCScriptForeignHandlerDefinition *handler,
					   uindex_t result_reg,
					   uindex_t *argument_regs,
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
										 uindex_t *arguments,
										 uindex_t argument_count);
	
	// Raise a 'value is not a handler' error.
	void ThrowNotAHandlerValue(MCValueRef actual_value);
	
	// Raise a 'value is not a string' error.
	void ThrowNotAStringValue(MCValueRef actual_value);
	
	//////////
	
	struct InternalHandlerContext
	{
		MCScriptInstanceRef instance;
		MCScriptCommonHandlerDefinition *definition;
	};
	
	static const MCHandlerCallbacks kInternalHandlerCallbacks;
	static bool IsInternalHandler(MCHandlerRef handler);
	static bool InternalHandlerInvoke(void *context,
									  MCValueRef *arguments,
									  uindex_t argument_count,
									  MCValueRef& r_return_value);
	static void InternalHandlerRelease(void *context);
	static bool InternalHandlerDescribe(void *context,
										MCStringRef& r_description);
};
