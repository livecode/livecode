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

// The state of the validate context is separated from the context for two
// reasons:
//    1. Validating bytecode should just mutate the state via the context
//       methods, they never need to set up new state and validate recursively
//    2. The state can be reused between each bytecode validation step by the
//       caller.
struct MCScriptValidateState
{
	// Whilst validation of bytecode is successful, the error var remains
	// false. When it becomes true, it means the bytecode at 'current_address'
	// within the handler is invalid.
	bool error;
	
	// The module containins the handler being validated.
	MCScriptModuleRef module;
	
	// The handler being validated.
	MCScriptHandlerDefinition *handler;
	
	// The offset from the beginning of the module's bytecode to the current
	// instruction being validated.
	uindex_t current_address;
	
	MCScriptBytecodeOp operation;
	
	// The current instruction's argument count.
	uindex_t argument_count;
	
	// The current instruction's list of (decoded) arguments.
	uindex_t arguments[256];
	
	// The total number of registers required to run the bytecode.
	uindex_t register_limit;
};

class MCScriptValidateContext
{
public:
	// Constructors
	//
	MCScriptValidateContext(MCScriptValidateState& state);
	
	// Return whether the context is valid.
	bool IsValid(void) const;
	
	// Return the maximum index of register used.
	uindex_t GetRegisterLimit(void) const;
	
	// Quering Methods
	//
	// These methods return information about the current bytecode operation
	// being processed.
	
	// Return the address of the current opcode
	uindex_t GetAddress(void) const;
	
	// Return the number of arguments to the current opcode
	uindex_t GetArity(void) const;
	
	// Get the argument at the given index as unsigned
	uindex_t GetArgument(uindex_t index) const;
	
	// Get the argument at the given index as signed
	index_t GetSignedArgument(uindex_t index) const;
	
	// Get the effective definition kind of the specified index
	MCScriptDefinitionKind GetEffectiveKindOfDefinition(uindex_t index) const;
	
	// Validating Methods
	//
	// These methods validate various aspects of the bytecode. The context
	// stores whether a previous validation failed, and takes appropriate
	// action. This means there is no need to check return values in a
	// Validate method.
	
	// Check the number of arguments is as specified.
	void CheckArity(uindex_t expected_arity);
	
	// Check the number of arguments is at least that specified.
	void CheckMinimumArity(uindex_t minimum_expected_arity);
	
	// Check the number of arguments is at most that specified.
	void CheckMaximumArity(uindex_t maximum_expected_arity);
	
	// Check the given address is valid - i.e. is within the current handler
	// and targets the start of an opcode.
	void CheckAddress(uindex_t target_address);
	
	// Check the given index is a valid register.
	// Note: At the point of checking, a maximum upper bound is used to check
	// the register index is 'sane'. At the end of validation an extra check
	// is performed to ensure that register indicies form a dense sequence from
	// 0..n where n is the maximum register index specified in the bytecode.
	void CheckRegister(uindex_t register_index);
	
	// Check the given index is a valid constant index
	void CheckValue(uindex_t constant_index);
	
	// Check the given index is a valid handler index. A valid handler index
	// can be of type Handler, ForeignHandler, or DefinitionGroup (of handlers).
	void CheckHandler(uindex_t handler_index);
	
	// Check the given index is a valid fetchable definition index. A fetchable
	// definition can be of type Constant, Variable or Handler.
	void CheckFetchable(uindex_t fetchable_index);
	
	// Check the given index is a valid variable definition index.
	void CheckVariable(uindex_t variable_index);
	
	// Reporting Methods
	//
	// These report the validation errors which can occur.
	
	void ReportInvalidArity(void);
	void ReportInvalidAddress(void);
	void ReportInvalidRegister(void);
	void ReportInvalidValue(void);
	void ReportInvalidDefinition(void);
	void ReportInvalidHandler(void);
	void ReportInvalidFetchable(void);
	void ReportInvalidVariable(void);
	
private:
	void ReportInvalid(void);
	
	MCScriptValidateState& m_state;
};

inline MCScriptValidateContext::MCScriptValidateContext(MCScriptValidateState& state)
	: m_state(state)
{
}

inline uindex_t MCScriptValidateContext::GetAddress(void) const
{
	return m_state.current_address;
}

inline uindex_t MCScriptValidateContext::GetArity(void) const
{
	return m_state.argument_count;
}

inline uindex_t MCScriptValidateContext::GetArgument(uindex_t p_index) const
{
	__MCScriptAssert__(p_index < m_state.argument_count, "invalid argument index");
	return m_state.arguments[p_index];
}

inline index_t MCScriptValidateContext::GetSignedArgument(uindex_t p_index) const
{
	return MCScriptBytecodeDecodeSignedArgument(GetArgument(p_index));
}

inline MCScriptDefinitionKind MCScriptValidateContext::GetEffectiveKindOfDefinition(uindex_t p_index) const
{
	__MCScriptAssert__(p_index < m_state.module->definition_count, "invalid definition index");
	
	if (m_state.module->definitions[p_index]->kind == kMCScriptDefinitionKindExternal)
	{
		MCScriptExternalDefinition *t_ext_def =
			static_cast<MCScriptExternalDefinition *>(m_state.module->definitions[p_index]);
		
		return m_state.module->imported_definitions[t_ext_def->index].kind;
	}
	
	return m_state.module->definitions[p_index]->kind;
}

inline void MCScriptValidateContext::CheckArity(uindex_t p_expected_arity)
{
	if (m_state.error)
		return;
	
	if (GetArity() != p_expected_arity)
	{
		ReportInvalidArity();
		return;
	}
}

inline void MCScriptValidateContext::CheckMinimumArity(uindex_t p_minimum_expected_arity)
{
	if (m_state.error)
		return;
	
	if (GetArity() < p_minimum_expected_arity)
	{
		ReportInvalidArity();
		return;
	}
}

inline void MCScriptValidateContext::CheckMaximumArity(uindex_t p_maximum_expected_arity)
{
	if (m_state.error)
		return;
	
	if (GetArity() > p_maximum_expected_arity)
	{
		ReportInvalidArity();
		return;
	}
}

inline void MCScriptValidateContext::CheckAddress(uindex_t p_address)
{
	if (m_state.error)
		return;
	
	if (p_address < m_state.handler->start_address)
	{
		ReportInvalidAddress();
		return;
	}
	
	if (p_address >= m_state.handler->finish_address)
	{
		ReportInvalidAddress();
		return;
	}
	
	// TODO: Validate that p_address is the start of an opcode.
}

inline void MCScriptValidateContext::CheckRegister(uindex_t p_register)
{
	if (m_state.error)
	{
		return;
	}
	
	if (p_register == UINDEX_MAX)
	{
		ReportInvalidRegister();
		return;
	}
	
	if (p_register >= m_state.register_limit)
	{
		m_state.register_limit = p_register + 1;
	}
}

inline void MCScriptValidateContext::CheckValue(uindex_t p_index)
{
	if (m_state.error)
	{
		return;
	}
	
	if (p_index > m_state.module->value_count)
	{
		ReportInvalidValue();
		return;
	}
}

inline void MCScriptValidateContext::CheckHandler(uindex_t p_index)
{
	if (m_state.error)
	{
		return;
	}
	
	if (p_index > m_state.module->definition_count)
	{
		ReportInvalidDefinition();
		return;
	}
	
	MCScriptDefinitionKind t_kind =
		GetEffectiveKindOfDefinition(p_index);
	if (t_kind != kMCScriptDefinitionKindHandler &&
		t_kind != kMCScriptDefinitionKindForeignHandler &&
		t_kind != kMCScriptDefinitionKindDefinitionGroup)
	{
		ReportInvalidHandler();
		return;
	}
}

inline void MCScriptValidateContext::CheckFetchable(uindex_t p_index)
{
	if (m_state.error)
	{
		return;
	}
	
	if (p_index > m_state.module->definition_count)
	{
		ReportInvalidDefinition();
		return;
	}
	
	MCScriptDefinitionKind t_kind =
	GetEffectiveKindOfDefinition(p_index);
	if (t_kind != kMCScriptDefinitionKindConstant &&
		t_kind != kMCScriptDefinitionKindVariable &&
		t_kind != kMCScriptDefinitionKindHandler &&
		t_kind != kMCScriptDefinitionKindForeignHandler)
	{
		ReportInvalidFetchable();
		return;
	}
}

inline void MCScriptValidateContext::CheckVariable(uindex_t p_index)
{
	if (m_state.error)
	{
		return;
	}
	
	if (p_index > m_state.module->definition_count)
	{
		ReportInvalidDefinition();
		return;
	}

	if (GetEffectiveKindOfDefinition(p_index) != kMCScriptDefinitionKindVariable)
	{
		ReportInvalidVariable();
		return;
	}
}

inline void MCScriptValidateContext::ReportInvalidArity(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidAddress(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidRegister(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidValue(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidDefinition(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidHandler(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidFetchable(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalidVariable(void)
{
	ReportInvalid();
}

inline void MCScriptValidateContext::ReportInvalid(void)
{
	m_state.error = true;
}
