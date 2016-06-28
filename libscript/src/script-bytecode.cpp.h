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

#include "script-validate.cpp.h"
#include "script-execute.cpp.h"

/*
struct MCScriptBytecodeOp_Noop
{
	// The code used to specify the operation
	static const MCScriptBytecodeOpcode kCode = kMCScriptBytecodeOpcodeNoop;
 
	// The name of the operation, as used in bytecode blocks
	static const char *kName = "noop";
 
	// The parameter description of the operation:
	//   l - label
	//   r - register (param, local or temp)
	//   c - constant pool index
	//   d - fetchable definition (variable, constant, handler)
	//   h - handler definition
	//   v - variable definition
	// The final char of the description string can be one of:
	//   ? - the final parameter is optional
	//   * - the final parameter type is expected 0-n times
	//   % - the final parameter type is expected 0-2n times
	//
	static const char *kFormat = "";
	
	// Validate is performed when a module is loaded to ensure that the
	// operation is well-formed in terms of its argument.
	static bool Validate(MCScriptValidateContext& ctxt)
	{
		return true;
	}
	
	// Execute is performed during execution to actually run the given
	// operation.
	static bool Execute(MCScriptExecuteContext& ctxt)
	{
		return true;
	}
};
*/

#define MC_SCRIPT_DEFINE_BYTECODE(NameM, LabelM, FormatM) \
	static MCScriptBytecodeOp Code(void) { return kMCScriptBytecodeOp##NameM; } \
	static const char *Name(void) {return LabelM; } \
	static const char *Format(void) {return FormatM; }

struct MCScriptBytecodeOp_Jump
{
	// jump <offset>
	//
	// Jump <offset> bytes in the bytecode relative to the jump opcode's
	// address.
	
	MC_SCRIPT_DEFINE_BYTECODE(Jump, "jump", "l")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(1);
		ctxt.CheckAddress(ctxt.GetAddress() +
						  ctxt.GetSignedArgument(0));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		ctxt.Jump(ctxt.GetSignedArgument(0));
	}
};

template<bool IfTrue>
struct MCScriptBytecodeOp_JumpIf
{
	// jump_if* <cond-reg>, <offset>
	//
	// Test <cond-reg> is a bool, and if so jump <offset> bytes in the bytecode
	// relative to the jump_if*'s opcode's address if the value in <cond-reg>
	// is IfTrue.
	
	static bool Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(2);
		ctxt.CheckRegister(ctxt.GetArgument(0));
		ctxt.CheckAddress(ctxt.GetAddress() +
						  ctxt.GetSignedArgument(0));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		if (IfTrue != ctxt.CheckedFetchRegisterAsBool(ctxt.GetArgument(0)))
			return;
		
		ctxt.Jump(ctxt.GetSignedArgument(1));
	}
};

struct MCScriptBytecodeOp_JumpIfFalse: public MCScriptBytecodeOp_JumpIf<false>
{
	MC_SCRIPT_DEFINE_BYTECODE(JumpIfFalse, "jump_if_false", "rl")
};

struct MCScriptBytecodeOp_JumpIfTrue: public MCScriptBytecodeOp_JumpIf<true>
{
	MC_SCRIPT_DEFINE_BYTECODE(JumpIfTrue, "jump_if_true", "rl")
};

struct MCScriptBytecodeOp_AssignConstant
{
	// assign_constant <dst-reg>, <constant-idx>
	//
	// Fetch the constant pool value at <constant-idx> and store into
	// <dst-reg>. The constant must conform to the type of register.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignConstant, "assign_constant", "rc")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(2);
		ctxt.CheckRegister(ctxt.GetArgument(0));
		ctxt.CheckValue(ctxt.GetArgument(1));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		ctxt.CheckedStoreRegister(ctxt.GetArgument(0),
								  ctxt.FetchValue(ctxt.GetArgument(1)));
	}
};

struct MCScriptBytecodeOp_Assign
{
	// assign <dst-reg>, <src-reg>
	//
	// Fetch the value from <src-reg> and store it into <dst-reg>. The value
	// being stored must conform to the type of the register.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignConstant, "assign", "rr")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(2);
		ctxt.CheckRegister(ctxt.GetArgument(0));
		ctxt.CheckRegister(ctxt.GetArgument(1));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		ctxt.CheckedStoreRegister(ctxt.GetArgument(0),
								  ctxt.CheckedFetchRegister(ctxt.GetArgument(1)));
	}
};

struct MCScriptBytecodeOp_Return
{
	// return [ <result-reg> ]
	//
	// Return from the current frame, setting the normal return value to the
	// contents of <result-reg> if specified, and nothing if not. The return
	// value must conform to the type of the handler's return value.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignConstant, "return", "r?")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		if (ctxt.GetArity() == 0)
		{
			// Nothing to check
		}
		else if (ctxt.GetArity() == 1)
		{
			ctxt.CheckRegister(ctxt.GetArgument(0));
		}
		else
		{
			ctxt.ReportInvalidArity();
		}
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		MCValueRef t_return_value = nil;
		if (ctxt.GetArgumentCount() == 0)
			t_return_value = kMCNull;
		else
			t_return_value = ctxt.CheckedFetchRegister(ctxt.GetArgument(0));
		
		ctxt.PopFrame(t_return_value);
	}
};

struct MCScriptBytecodeOp_Invoke
{
	// invoke <handler-idx>, <result-reg>, <arg_1-reg>, ..., <arg_n-reg>
	//
	// Invoke the given handler (which can be a normal handler, foreign handler
	// or a handler group), returning the value into result-reg and using the
	// arg_i-reg's as the in, out and inout parameters.
	
	MC_SCRIPT_DEFINE_BYTECODE(Invoke, "invoke", "hrr*")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		if (ctxt.GetArity() < 2)
			ctxt.ReportInvalidArity();
		
		ctxt.CheckHandler(ctxt.GetArgument(0));
		
		for(uindex_t i = 1; i < ctxt.GetArity(); i++)
			ctxt.CheckRegister(ctxt.GetArgument(i));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		// Resolve the import definition chain for the specified definition.
		MCScriptInstanceRef t_instance = nil;
		MCScriptDefinition *t_definition = nil;
		ctxt.ResolveDefinition(ctxt.GetArgument(0),
							   t_instance,
							   t_definition);
		
		// The result register is the second argument.
		uindex_t t_result_reg;
		t_result_reg = ctxt.GetArgument(1);
		
		// The argument list starts at the third argument.
		uindex_t *t_argument_regs;
		uindex_t t_argument_count;
		t_argument_regs = ctxt.GetArgumentList() + 2;
		t_argument_count = ctxt.GetArgumentCount() - 2;
		
		
		// If the definition kind is a group, then we must select which handler
		// in the group is appropriate based on the in-mode argument types.
		if (t_definition -> kind == kMCScriptDefinitionKindDefinitionGroup)
		{
			SelectDefinitionFromGroup(ctxt,
									  t_instance,
									  static_cast<MCScriptDefinitionGroupDefinition *>(t_definition),
									  t_argument_regs,
									  t_argument_count,
									  t_instance,
									  t_definition);
		}
		
		// If we have no definition (due to failure), we return,
		if (t_definition == nil)
			return;
		
		if (t_definition -> kind == kMCScriptDefinitionKindHandler)
		{
			ctxt.PushFrame(t_instance,
						   static_cast<MCScriptHandlerDefinition *>(t_definition),
						   t_result_reg,
						   t_argument_regs,
						   t_argument_count);
		}
		else if (t_definition -> kind == kMCScriptDefinitionKindForeignHandler)
		{
			ctxt.InvokeForeign(t_instance,
							   static_cast<MCScriptForeignHandlerDefinition *>(t_definition),
							   t_result_reg,
							   t_argument_regs,
							   t_argument_count);
		}
		else
		{
			__MCScriptUnreachable__("invalid definition kind in invoke");
		}
	}
	
private:
	static void SelectDefinitionFromGroup(MCScriptExecuteContext& ctxt,
										  MCScriptInstanceRef p_instance,
										  MCScriptDefinitionGroupDefinition *p_group,
										  uindex_t *p_arguments,
										  uindex_t p_argument_count,
										  MCScriptInstanceRef& r_selected_instance,
										  MCScriptDefinition*& r_selected_definition)
	{
		// We use a simple scoring mechanism to determine which method to use. If
		// input type for an argument is equal to expected type, then this is a
		// score of zero. If input type for an argument conforms to expected type, then
		// this is a score of 1. The method with the lowest score is chosen, with
		// priority given to the first in the list in the case of a tie.
		
		uindex_t t_min_score = UINDEX_MAX;
		MCScriptDefinition *t_min_score_definition = nil;
		MCScriptInstanceRef t_min_score_instance = nil;
		for(uindex_t t_handler_idx = 0; t_handler_idx < p_group -> handler_count; t_handler_idx++)
		{
			MCScriptInstanceRef t_current_instance;
			MCScriptDefinition *t_current_definition;
			ctxt.ResolveDefinition(p_group -> handlers[t_handler_idx],
								   t_current_instance,
								   t_current_definition);
			
			MCScriptCommonHandlerDefinition *t_current_handler;
			t_current_handler = static_cast<MCScriptCommonHandlerDefinition *>(t_current_definition);
			
			// The signature of handler definitions is always a handler typeinfo.
			MCTypeInfoRef t_current_signature;
			t_current_signature = t_current_instance -> module -> types[t_current_handler -> type] -> typeinfo;
			
			if (MCHandlerTypeInfoGetParameterCount(t_current_signature) != p_argument_count)
				continue;
			
			uindex_t t_current_score = 0;
			for(uindex_t t_arg_idx = 0; t_arg_idx < p_argument_count; t_arg_idx++)
			{
				// We can't compare types of out arguments as they have no value
				// (yet).
				if (MCHandlerTypeInfoGetParameterMode(t_current_signature,
													  t_arg_idx) == kMCHandlerTypeFieldModeOut)
					continue;
				
				// Fetch the value in the register.
				MCValueRef t_arg_value;
				t_arg_value = ctxt.CheckedFetchRegister(p_arguments[t_arg_idx]);
				
				// If the fetch failed, there was an error.
				if (t_arg_value == nil)
					return;
				
				MCTypeInfoRef t_arg_type, t_param_type;
				t_arg_type = MCValueGetTypeInfo(t_arg_value);
				t_param_type = MCHandlerTypeInfoGetParameterType(t_current_signature,
																 t_arg_idx);
				
				// If the typeinfo's are the same, short-circuit to a score of 0
				if (t_arg_type == t_param_type)
					continue;
				
				// Resolve both the source and target types.
				MCResolvedTypeInfo t_resolved_arg_type, t_resolved_param_type;
				if (!ctxt.ResolveType(t_arg_type,
									  t_resolved_arg_type))
					return;
				
				if (!ctxt.ResolveType(t_param_type,
									  t_resolved_param_type))
					return;
				
				// If the resolved types are the same, then this argument matches.
				if (t_resolved_arg_type . type == t_resolved_param_type . type)
					continue;
				
				// If the types don't conform, then this method is no good.
				if (!MCResolvedTypeInfoConforms(t_resolved_arg_type,
												t_resolved_param_type))
				{
					t_current_score = UINDEX_MAX;
					break;
				}
				
				// Otherwise we have a score of 1
				t_current_score += 1;
			}

			if (t_current_score < t_min_score)
			{
				t_min_score = t_current_score;
				t_min_score_definition = t_current_definition;
				t_min_score_instance = t_current_instance;
			}
		}
		if (t_min_score_definition == NULL)
		{
			ctxt.ThrowUnableToResolveMultiInvoke(p_group, p_arguments, p_argument_count);
			return;
		}
		
		r_selected_instance = t_min_score_instance;
		r_selected_definition = t_min_score_definition;
	}
};

struct MCScriptBytecodeOp_InvokeIndirect
{
	// invoke_indirect <handler-reg>, <result-reg>, <arg_1-reg>, ..., <arg_n-reg>
	//
	// Invoke the handler value in handler-reg, returning the value into
	// result-reg and using the arg_i-reg's as the in, out and inout parameters.
	
	MC_SCRIPT_DEFINE_BYTECODE(InvokeIndirect, "invoke_indirect", "rrr*")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		if (ctxt.GetArity() < 2)
		{
			ctxt.ReportInvalidArity();
			return;
		}
			
		ctxt.CheckRegister(ctxt.GetArgument(0));
		
		for(uindex_t i = 1; i < ctxt.GetArity(); i++)
			ctxt.CheckRegister(ctxt.GetArgument(i));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		// The result register is the second argument.
		uindex_t t_result_reg;
		t_result_reg = ctxt.GetArgument(1);
		
		// The argument list starts at the third argument.
		uindex_t *t_argument_regs;
		uindex_t t_argument_count;
		t_argument_regs = ctxt.GetArgumentList() + 2;
		t_argument_count = ctxt.GetArgumentCount() - 2;
		
		MCValueRef t_handler;
		t_handler = ctxt.CheckedFetchRegister(ctxt.GetArgument(0));
		if (t_handler == nil)
			return;
		
		if (MCValueGetTypeCode(t_handler) != kMCValueTypeCodeHandler)
		{
			ctxt.ThrowNotAHandlerValue(t_handler);
			return;
		}
		
		// If the handler-ref is of 'internal' kind (i.e. a instance/definition
		// pair) then we can invoke directly; otherwise the handler-ref must
		// be wrapping some other sort of invokable handler and thus we must
		// use the MCHandlerRef API.
		if (ctxt.IsInternalHandler((MCHandlerRef)t_handler))
		{
			InternalInvoke(ctxt,
						   (MCHandlerRef)t_handler,
						   t_result_reg,
						   t_argument_regs,
						   t_argument_count);
		}
		else
		{
			ExternalInvoke(ctxt,
						   (MCHandlerRef)t_handler,
						   t_result_reg,
						   t_argument_regs,
						   t_argument_count);
		}
	}
	
private:
	static void InternalInvoke(MCScriptExecuteContext& ctxt,
							   MCHandlerRef p_handler,
							   uindex_t p_result_reg,
							   uindex_t *p_argument_regs,
							   uindex_t p_argument_count)
	{
		// An 'internal' handler-ref is nothing more than an instance/definition
		// pair, so we can execute directly inside this execution context. This
		// avoids packing/unpacking the argument list as MCValueRefs and we can
		// use the register file directly.
		
		MCScriptExecuteContext::InternalHandlerContext *t_context;
		t_context = (MCScriptExecuteContext::InternalHandlerContext *)MCHandlerGetContext((MCHandlerRef)p_handler);
		
		if (t_context -> definition -> kind == kMCScriptDefinitionKindHandler)
		{
			ctxt.PushFrame(t_context -> instance,
						   static_cast<MCScriptHandlerDefinition *>(t_context -> definition),
						   p_result_reg,
						   p_argument_regs,
						   p_argument_count);
		}
		else if (t_context -> definition -> kind == kMCScriptDefinitionKindForeignHandler)
		{
			ctxt.InvokeForeign(t_context -> instance,
							   static_cast<MCScriptForeignHandlerDefinition *>(t_context -> definition),
							   p_result_reg,
							   p_argument_regs,
							   p_argument_count);
		}
		else
		{
			__MCScriptUnreachable__("invalid definition kind in internal handler-ref");
		}
	}
	
	static void ExternalInvoke(MCScriptExecuteContext& ctxt,
							   MCHandlerRef p_handler,
							   uindex_t p_result_reg,
							   uindex_t *p_argument_regs,
							   uindex_t p_argument_count)
	{
		// An 'external' handler-ref could be from anywhere and so we must use
		// the handler-ref API. Unfortunately, this does mean we have to pack
		// up the values into a list of valuerefs, then unpack them again after-
		// wards.
		
		MCAutoValueRefArray t_linear_args;
		if (!t_linear_args.Resize(p_argument_count))
		{
			ctxt.Rethrow();
			return;
		}
		
		for(uindex_t t_arg_index = 0; t_arg_index < p_argument_count; t_arg_index++)
		{
			t_linear_args[t_arg_index] = ctxt.CheckedFetchRegister(p_argument_regs[t_arg_index]);
			if (t_linear_args[t_arg_index] == nil)
				return;
		}
		
		MCAutoValueRef t_result;
		if (!MCHandlerInvoke(p_handler,
							 t_linear_args.Ptr(),
							 p_argument_count,
							 &t_result))
		{
			ctxt.Rethrow();
			return;
		}
		
		MCTypeInfoRef t_signature;
		t_signature = MCValueGetTypeInfo(p_handler);
		
		for(uindex_t t_arg_index = 0; t_arg_index < p_argument_count; t_arg_index++)
		{
			if (MCHandlerTypeInfoGetParameterMode(t_signature,
												 t_arg_index) == kMCHandlerTypeFieldModeIn)
				continue;
			
			ctxt.CheckedStoreRegister(p_argument_regs[t_arg_index],
									  t_linear_args[t_arg_index]);
		}
		
		ctxt.CheckedStoreRegister(p_result_reg,
								  *t_result);
	}
};

struct MCScriptBytecodeOp_Fetch
{
	// fetch <dst-reg>, <index>
	//
	// Fetch the value from definition index <index> and place it into <dst-reg>.
	// The definition can be a variable, constant, handler or foreign handler.
	
	MC_SCRIPT_DEFINE_BYTECODE(Invoke, "fetch", "rd")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(2);
		ctxt.CheckRegister(ctxt.GetArgument(0));
		ctxt.CheckFetchable(ctxt.GetArgument(1));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		// Resolve the import definition chain for the specified definition.
		MCScriptInstanceRef t_instance = nil;
		MCScriptDefinition *t_definition = nil;
		ctxt.ResolveDefinition(ctxt.GetArgument(1),
							   t_instance,
							   t_definition);
		
		MCValueRef t_value = nil;
		if (t_definition -> kind == kMCScriptDefinitionKindVariable)
		{
			MCScriptVariableDefinition *t_variable_def;
			t_variable_def = static_cast<MCScriptVariableDefinition *>(t_definition);
			
			t_value = ctxt.CheckedFetchVariable(t_instance,
												t_variable_def);
		}
		else if (t_definition -> kind == kMCScriptDefinitionKindConstant)
		{
			MCScriptConstantDefinition *t_constant_def;
			t_constant_def = static_cast<MCScriptConstantDefinition *>(t_definition);
			
			t_value = ctxt.FetchConstant(t_instance,
										 t_constant_def);
		}
		else if (t_definition -> kind == kMCScriptDefinitionKindHandler)
		{
			MCScriptHandlerDefinition *t_handler_def;
			t_handler_def = static_cast<MCScriptHandlerDefinition *>(t_definition);
			
			t_value = ctxt.FetchHandler(t_instance,
										t_handler_def);
		}
		else if (t_definition -> kind == kMCScriptDefinitionKindForeignHandler)
		{
			MCScriptForeignHandlerDefinition *t_foreign_handler_def;
			t_foreign_handler_def = static_cast<MCScriptForeignHandlerDefinition *>(t_definition);
			
			t_value = ctxt.FetchForeignHandler(t_instance,
											   t_foreign_handler_def);
		}
		
		ctxt.CheckedStoreRegister(ctxt.GetArgument(0),
								  t_value);
	}
};

struct MCScriptBytecodeOp_Store
{
	// store <src-reg>, <index>
	//
	// Store the value from <src-reg> into the variable definition <index>.
	
	MC_SCRIPT_DEFINE_BYTECODE(Invoke, "store", "rv")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		ctxt.CheckArity(2);
		ctxt.CheckRegister(ctxt.GetArgument(0));
		ctxt.CheckVariable(ctxt.GetArgument(1));
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		// Resolve the import definition chain for the specified definition.
		MCScriptInstanceRef t_instance = nil;
		MCScriptDefinition *t_definition = nil;
		ctxt.ResolveDefinition(ctxt.GetArgument(1),
							   t_instance,
							   t_definition);
		
		ctxt.CheckedStoreVariable(t_instance,
								  static_cast<MCScriptVariableDefinition *>(t_definition),
								  ctxt.CheckedFetchRegister(ctxt.GetArgument(0)));
	}
};

struct MCScriptBytecodeOp_AssignList
{
	// assign-list <dst-reg>, <element_1-reg>, ..., <element_n-reg>
	//
	// Construct a (proper) list value from <element_i-reg>'s and assign to
	// <dst-reg>.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignList, "assign_list", "rr*")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		if (ctxt.GetArity() < 1)
		{
			ctxt.ReportInvalidArity();
			return;
		}
		
		ctxt.CheckRegister(ctxt.GetArgument(0));
		
		for(uindex_t t_element_idx = 1; t_element_idx < ctxt.GetArity(); t_element_idx++)
		{
			ctxt.CheckRegister(ctxt.GetArgument(t_element_idx));
		}
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		MCAutoArray<MCValueRef> t_elements;
		if (!t_elements.Resize(ctxt.GetArgumentCount() - 1))
		{
			ctxt.Rethrow();
			return;
		}
		
		for(uindex_t t_element_idx = 0; t_element_idx < t_elements.Size(); t_element_idx++)
		{
			t_elements[t_element_idx] = ctxt.CheckedFetchRegister(ctxt.GetArgument(t_element_idx + 1));
			if (t_elements[t_element_idx] == nil)
				return;
		}
		
		MCAutoProperListRef t_list;
		if (!MCProperListCreate(t_elements.Ptr(),
								t_elements.Size(),
								&t_list))
		{
			ctxt.Rethrow();
			return;
		}
		
		ctxt.CheckedStoreRegister(ctxt.GetArgument(0),
								  *t_list);
	}
};

struct MCScriptBytecodeOp_AssignArray
{
	// assign-array <dst-reg>, <key_1-reg>, <value_1-reg>, ..., <key_n-reg>, <value_n-reg>
	//
	// Construct an array value from the <key_i-reg>, <value_i-reg> pairs and
	// assign to <dst-reg>.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignArray, "assign_array", "rr%")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		if (ctxt.GetArity() < 1 ||
			((ctxt.GetArity() - 1) % 2) != 0)
		{
			ctxt.ReportInvalidArity();
			return;
		}
		
		ctxt.CheckRegister(ctxt.GetArgument(0));
		
		for(uindex_t t_src_idx = 1; t_src_idx < ctxt.GetArity(); t_src_idx++)
		{
			ctxt.CheckRegister(ctxt.GetArgument(t_src_idx));
		}
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		MCAutoArrayRef t_array;
		if (!MCArrayCreateMutable(&t_array))
		{
			ctxt.Rethrow();
			return;
		}
		
		for(uindex_t t_arg_idx = 1; t_arg_idx < ctxt.GetArgumentCount(); t_arg_idx += 2)
		{
			MCValueRef t_raw_key;
			t_raw_key = ctxt.CheckedFetchRegister(ctxt.GetArgument(t_arg_idx));
			if (t_raw_key == nil)
			{
				return;
			}
			if (MCValueGetTypeCode(t_raw_key) != kMCValueTypeCodeString)
			{
				ctxt.ThrowNotAStringValue(t_raw_key);
				return;
			}
			
			MCNewAutoNameRef t_key;
			if (!MCNameCreate(reinterpret_cast<MCStringRef>(t_raw_key),
							  &t_key))
			{
				ctxt.Rethrow();
				return;
			}
			
			MCValueRef t_value;
			t_value = ctxt.CheckedFetchRegister(ctxt.GetArgument(t_arg_idx + 1));
			if (t_value == nil)
			{
				return;
			}
			
			if (!MCArrayStoreValue(*t_array,
								   false,
								   *t_key,
								   t_value))
			{
				ctxt.Rethrow();
				return;
			}
		}
		
		if (!t_array.MakeImmutable())
		{
			ctxt.Rethrow();
			return;
		}
		
		ctxt.CheckedStoreRegister(ctxt.GetArgument(0),
								  *t_array);
	}
};

struct MCScriptBytecodeOp_Reset
{
	// reset <reg-1>, ..., <reg-n>
	//
	// Reset registers to their default (typed) value. If the type does not have
	// a default value, the register becomes unassigned.
	
	MC_SCRIPT_DEFINE_BYTECODE(AssignArray, "reset", "r*")
	
	static void Validate(MCScriptValidateContext& ctxt)
	{
		for(uindex_t t_reg_idx = 0; t_reg_idx < ctxt.GetArity(); t_reg_idx++)
		{
			ctxt.CheckRegister(ctxt.GetArgument(t_reg_idx));
		}
	}
	
	static void Execute(MCScriptExecuteContext& ctxt)
	{
		for(uindex_t t_arg = 0; t_arg < ctxt.GetArgumentCount(); t_arg++)
		{
			MCTypeInfoRef t_type =
				ctxt.GetTypeOfRegister(ctxt.GetArgument(t_arg));
			
			MCValueRef t_default_value = nil;
			if (t_type != nil)
			{
				t_default_value = MCTypeInfoGetDefault(t_type);
			}
			
			ctxt.StoreRegister(ctxt.GetArgument(t_arg),
							   t_default_value);
		}
	}
};

#define MC_SCRIPT_DISPATCH_BYTECODE_OP(Name) \
	case kMCScriptBytecodeOp##Name: \
		if (!ctxt.template operator()<MCScriptBytecodeOp_##Name>()) \
			return false; \
		break;

template<
class Context
>
inline bool MCScriptBytecodeDispatch(MCScriptBytecodeOp op,
									 Context& ctxt)
{
	_Pragma("GCC diagnostic push")
	_Pragma("GCC diagnostic error \"-Wswitch\"")
	switch(op)
	{
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Jump)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(JumpIfFalse)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(JumpIfTrue)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignConstant)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Assign)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Return)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Invoke)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(InvokeIndirect)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Fetch)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Store)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignList)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignArray)
			MC_SCRIPT_DISPATCH_BYTECODE_OP(Reset)
	}
	_Pragma("GCC diagnostic pop")
	
	return true;
}

template<
typename Context
>
inline bool MCScriptBytecodeForEach(Context& ctxt)
{
	for(MCScriptBytecodeOp t_op = kMCScriptBytecodeOp__First; t_op <= kMCScriptBytecodeOp__Last; t_op = (MCScriptBytecodeOp)((int)t_op + 1))
	{
		if (MCScriptBytecodeDispatch<Context>(t_op,
											  ctxt))
			return true;
	}
	
	return false;
}

#if 0
template<
typename Context,
template<typename OpStruct> class Dispatch
>
inline bool MCScriptBytecodeDispatch(MCScriptBytecodeOp op,
									 Context& ctxt)
{
	_Pragma("GCC diagnostic push")
	_Pragma("GCC diagnostic error \"-Wswitch\"")
	switch(op)
	{
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Jump)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(JumpIfFalse)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(JumpIfTrue)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignConstant)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Assign)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Return)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Invoke)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(InvokeIndirect)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Fetch)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Store)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignList)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(AssignArray)
		MC_SCRIPT_DISPATCH_BYTECODE_OP(Reset)
	}
	_Pragma("GCC diagnostic pop")
	
	return true;
}

template<
typename Context,
template<typename OpStruct> class ForEach
>
inline bool MCScriptBytecodeForEach(Context& ctxt)
{
	for(MCScriptBytecodeOp t_op = kMCScriptBytecodeOp__First; t_op <= kMCScriptBytecodeOp__Last; t_op = (MCScriptBytecodeOp)((int)t_op + 1))
	{
		if (MCScriptBytecodeDispatch<Context, ForEach>(t_op,
													   ctxt))
			return true;
	}
	
	return false;
}
#endif
