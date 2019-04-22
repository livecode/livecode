/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include <libscript/script.h>

#include "script-private.h"

#include "script-bytecode.hpp"

////////////////////////////////////////////////////////////////////////////////

// Utility functor, used to fetch the bytecode info struct for the given
// opcode.
struct MCScriptDescribeBytecodeOp_Impl
{
public:
	template<typename OpStruct>
	bool Visit(const MCScriptBytecodeOpInfo*& r_info) const
	{
		r_info = &OpStruct::Describe();
		return true;
	}
};

static bool
MCScriptDescribeBytecodeOp(uindex_t p_opcode,
						   const MCScriptBytecodeOpInfo*& r_info)
{
	if (p_opcode > kMCScriptBytecodeOp__Last ||
		!MCScriptBytecodeDispatchR((MCScriptBytecodeOp)p_opcode,
								   MCScriptDescribeBytecodeOp_Impl(),
								   r_info))
	{
		return false;
	}
	
	return true;
}

//////////

class MCScriptCopyBytecodeNames_Impl
{
public:
	template<typename OpStruct>
	bool Visit(MCProperListRef x_bytecode_names) const
	{
		if (!MCProperListPushElementOntoBack(x_bytecode_names,
											 MCSTR(OpStruct::Describe().name)))
			return false;
		
		return true;
	}
};

bool MCScriptCopyBytecodeNames(MCProperListRef& r_bytecode_names)
{
	MCAutoProperListRef t_bytecode_names;
	if (!MCProperListCreateMutable(&t_bytecode_names))
		return false;
	
	MCProperListRef t_bytecode_names_arg = *t_bytecode_names;
	if (!MCScriptBytecodeForEach(MCScriptCopyBytecodeNames_Impl(),
								 t_bytecode_names_arg))
		return false;
	
	r_bytecode_names = t_bytecode_names.Take();
	
	return true;
}

//////////

struct MCScriptLookupBytecode_Impl
{
public:
	struct Args
	{
		const char *name;
		uindex_t& _opcode;
		
		Args(const char *p_name,
			 uindex_t& r_opcode)
			: name(p_name),
			  _opcode(r_opcode)
		{
		}
	};
	
	template<typename OpStruct>
	bool Visit(Args p_args) const
	{
		if (0 != strcmp(p_args.name, OpStruct::Describe().name))
			return true;
		
		p_args._opcode = (uindex_t)OpStruct::Describe().code;
		
		return false;
	}
};

bool MCScriptLookupBytecode(const char *p_name, uindex_t& r_opcode)
{
	// If we get all the way through the bytecodes without returning false, then
	// it means it wasn't found.
	MCScriptLookupBytecode_Impl::Args t_args(p_name,
											 r_opcode);
	if (MCScriptBytecodeForEach(MCScriptLookupBytecode_Impl(),
								t_args))
		return false;
	
	return true;
}

//////////

const char *MCScriptDescribeBytecode(uindex_t p_opcode)
{
	const MCScriptBytecodeOpInfo *t_info;
	if (!MCScriptDescribeBytecodeOp(p_opcode, t_info))
		return "<unknown>";
	return t_info->name;
}

//////////

MCScriptBytecodeParameterType MCScriptDescribeBytecodeParameter(uindex_t p_opcode, uindex_t p_index)
{
	const MCScriptBytecodeOpInfo *t_info;
	if (!MCScriptDescribeBytecodeOp(p_opcode, t_info))
		return kMCScriptBytecodeParameterTypeUnknown;
    
    const char *t_desc;
    t_desc = t_info->format;
    
    size_t t_desc_length;
    t_desc_length = strlen(t_desc);
    
    char t_param_type;
    switch(t_desc[t_desc_length - 1])
    {
        case '?':
            if (p_index >= t_desc_length - 1)
                return kMCScriptBytecodeParameterTypeUnknown;
            t_param_type = t_desc[p_index];
            break;
            
        case '*':
        case '%':
            if (p_index >= t_desc_length - 1)
                t_param_type = t_desc[t_desc_length - 2];
            else
                t_param_type = t_desc[p_index];
            break;
            
        default:
            if (p_index >= t_desc_length)
                return kMCScriptBytecodeParameterTypeUnknown;
            t_param_type = t_desc[p_index];
            break;
    }
    
    switch(t_param_type)
    {
        case 'l':
            return kMCScriptBytecodeParameterTypeLabel;
        case 'r':
            return kMCScriptBytecodeParameterTypeRegister;
        case 'c':
            return kMCScriptBytecodeParameterTypeConstant;
        case 'd':
            return kMCScriptBytecodeParameterTypeDefinition;
        case 'v':
            return kMCScriptBytecodeParameterTypeVariable;
        case 'h':
            return kMCScriptBytecodeParameterTypeHandler;
        default:
            break;
    }
    
    return kMCScriptBytecodeParameterTypeUnknown;
}

bool MCScriptCheckBytecodeParameterCount(uindex_t p_opcode, uindex_t p_proposed_count)
{
	const MCScriptBytecodeOpInfo *t_info;
	if (!MCScriptDescribeBytecodeOp(p_opcode, t_info))
		return kMCScriptBytecodeParameterTypeUnknown;

    const char *t_desc;
    t_desc = t_info->format;
    
    size_t t_desc_length;
    t_desc_length = strlen(t_desc);
    
    switch(t_desc[t_desc_length - 1])
    {
        case '?':
            if (p_proposed_count == t_desc_length - 2 ||
                p_proposed_count == t_desc_length - 1)
                return true;
            break;
            
        case '*':
            if (p_proposed_count >= t_desc_length - 2)
                return true;
            break;
            
        case '%':
            if (p_proposed_count >= t_desc_length - 2 &&
                ((p_proposed_count - (t_desc_length - 2)) % 2 == 0))
                return true;
            break;
            
        default:
            if (p_proposed_count == t_desc_length)
                return true;
            break;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

struct MCScriptBytecodeLabel
{
    uindex_t instruction;
};

struct MCScriptBytecodeInstruction
{
    MCScriptBytecodeOp operation;
    uindex_t arity;
    uindex_t operands;
    uindex_t address;
    uindex_t file;
    uindex_t line;
};

struct MCScriptModuleBuilder
{
    bool valid;
    
    MCScriptModule module;
    
    uindex_t current_handler;
    uindex_t current_param_count;
    uindex_t current_local_count;
    
    uindex_t *current_handler_group;
    uindex_t current_handler_group_size;
    
    uindex_t current_syntax;
    
    MCScriptType *current_type;
    
    MCScriptBytecodeLabel *labels;
    uindex_t label_count;
    MCScriptBytecodeInstruction *instructions;
    uindex_t instruction_count;
    uindex_t *operands;
    uindex_t operand_count;
    
    uindex_t current_file;
    uindex_t current_line;
    
    MCProperListRef current_list_value;
	MCArrayRef current_array_value;

    MCScriptDefinitionKind *definition_kinds;
    uindex_t definition_kind_count;
};

////////////////////////////////////////////////////////////////////////////////

static void __emit_constant(MCScriptModuleBuilderRef self, MCValueRef p_constant, uindex_t& r_index);

////////////////////////////////////////////////////////////////////////////////

static bool __append_definition_name(MCScriptModuleBuilderRef self, MCNameRef p_name)
{
    if (!MCMemoryResizeArray(self -> module . definition_name_count + 1, self -> module . definition_names, self -> module . definition_name_count))
        return false;
    
    self -> module . definition_names[self -> module . definition_name_count - 1] = MCValueRetain(p_name);
    
    return true;
}

static void __assign_definition_name(MCScriptModuleBuilderRef self, uindex_t p_index, MCNameRef p_name)
{
    MCValueRelease(self -> module . definition_names[p_index]);
    self -> module . definition_names[p_index] = MCValueRetain(p_name);
}

template<typename T> static bool __extend_array(MCScriptModuleBuilderRef self, T*& x_array, uindex_t& x_array_size, uindex_t& r_index)
{
    if (!MCMemoryResizeArray(x_array_size + 1, x_array, x_array_size))
    {
        r_index = 0;
        self -> valid = false;
        return false;
    }
    
    r_index = x_array_size - 1;
    
    return true;
}

void MCScriptBeginModule(MCScriptModuleKind p_kind, MCNameRef p_name, MCScriptModuleBuilderRef& r_builder)
{
    MCScriptModuleBuilder *self;
    if (!MCMemoryNew(self))
    {
        r_builder = nil;
        return;
    }

    self -> valid = true;
    self -> module . module_kind = p_kind;
    self -> module . name = MCValueRetain(p_name);
    self -> current_handler = UINDEX_MAX;
    
    self -> current_handler_group = nil;
    self -> current_handler_group_size = 0;
    
    self -> current_syntax = UINDEX_MAX;
    
    self -> current_file = 0;
    self -> current_line = 0;
    
    self -> current_list_value = nil;
    self -> current_array_value = nil;
    
    self -> definition_kind_count = 0;
    self -> definition_kinds = nil;
    
    r_builder = self;
}

static bool
MCScriptValidateModuleBytecode(MCScriptModule *self)
{
    for(uindex_t i = 0; i < self -> definition_count; i++)
	{
        if (self -> definitions[i] -> kind == kMCScriptDefinitionKindHandler)
        {
			MCScriptHandlerDefinition *t_handler;
			t_handler = static_cast<MCScriptHandlerDefinition *>(self -> definitions[i]);
			
			MCScriptValidateState t_state;
			t_state.error = false;
			t_state.module = self;
			t_state.handler = t_handler;
			
			MCScriptHandlerType *t_signature;
			t_signature = static_cast<MCScriptHandlerType *>(self -> types[t_handler -> type]);
			t_state.register_limit = t_signature->parameter_count +
										t_handler -> local_type_count;
            
			const byte_t *t_bytecode;
			const byte_t *t_bytecode_limit;
            t_bytecode = self -> bytecode + t_handler -> start_address;
            t_bytecode_limit = self -> bytecode + t_handler -> finish_address;
			
			// If there is no bytecode, the bytecode is malformed.
			if (t_bytecode == t_bytecode_limit)
				return false;
			
            while(t_bytecode != t_bytecode_limit)
			{
				t_state.current_address = uindex_t(t_bytecode - self-> bytecode);
				
                if (!MCScriptBytecodeIterate(t_bytecode,
											 t_bytecode_limit,
											 t_state.operation,
											 t_state.argument_count,
											 t_state.arguments))
				{
					t_state.error = true;
					break;
				}
				
				if (t_state.operation > kMCScriptBytecodeOp__Last)
				{
					t_state.error = true;
					break;
				}
				
				MCScriptBytecodeValidate(t_state);
				if (t_state.error)
				{
					break;
				}
            }
			
			// If validation failed for a single operation, the bytecode is
			// malformed.
			if (t_state.error)
				return false;
			
			// If we didn't reach the limit, the bytecode is malformed.
			if (t_bytecode != t_bytecode_limit)
				return false;
			
            // If the last operation was not return, the bytecode is malformed.
			if (t_state.operation != kMCScriptBytecodeOpReturn)
				return false;
        }
	}
    
    return true;
}

bool MCScriptEndModule(MCScriptModuleBuilderRef self, MCStreamRef p_stream)
{
    if (self == nil)
        return false;
    
    for(uindex_t i = 0; self -> valid && i < self -> module . definition_count; i++)
    {
        if (self -> module . definitions[i] != nil)
            continue;
        
        self -> valid = false;
    }
    
    if (self->valid)
    {
        if (!MCScriptValidateModuleBytecode(&self->module))
        {
            self->valid = false;
        }
    }
    
    bool t_success;
    if (self -> valid)
    {
        byte_t t_header[4];
        t_header[0] = 'L';
        t_header[1] = 'C';
        t_header[2] = ((kMCScriptCurrentModuleVersion >> 0) & 0xFF);
        t_header[3] = ((kMCScriptCurrentModuleVersion >> 8) & 0xFF);
        
        t_success = MCStreamWrite(p_stream, t_header, 4) &&
                    MCScriptWriteRawModule(p_stream, &self -> module);
    }
    else
        t_success = false;
    
    MCScriptReleaseRawModule(&self -> module);
    
    MCMemoryDeleteArray(self -> labels);
    MCMemoryDeleteArray(self -> instructions);
    MCMemoryDeleteArray(self -> operands);
    
    MCValueRelease(self -> current_list_value);
    MCValueRelease(self -> current_array_value);
    
    MCMemoryDeleteArray(self -> definition_kinds);
    
    MCMemoryDelete(self);

    return t_success;
}

void MCScriptAddDependencyToModule(MCScriptModuleBuilderRef self, MCNameRef p_dependency, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . dependency_count; i++)
        if (MCNameIsEqualToCaseless(p_dependency, self -> module . dependencies[i] . name))
        {
            r_index = i;
            return;
        }
    
    if (!__extend_array(self, self -> module . dependencies, self -> module . dependency_count, r_index))
        return;
    
    self -> module . dependencies[r_index] . name = MCValueRetain(p_dependency);
    self -> module . dependencies[r_index] . version = 0;
}

void MCScriptAddExportToModule(MCScriptModuleBuilderRef self, uindex_t p_definition)
{
    if (self == nil || !self -> valid)
        return;
    
    uindex_t t_index;
    if (!__extend_array(self, self -> module . exported_definitions, self -> module . exported_definition_count, t_index))
        return;
    
    self -> module  . exported_definitions[t_index] . name = MCValueRetain(self -> module . definition_names[p_definition]);
    self -> module  . exported_definitions[t_index] . index = p_definition;
    
    // If the definition is a type, then make sure we make it a 'defined' type otherwise
    // it won't get bound globally.
    if (self -> module . definitions[p_definition] -> kind == kMCScriptDefinitionKindType)
    {
        uindex_t t_type_index;
        MCScriptAddDefinedTypeToModule(self, p_definition, t_type_index);
    }
}

void MCScriptAddImportToModuleWithIndex(MCScriptModuleBuilderRef self, uindex_t p_module_index, MCNameRef p_name, MCScriptDefinitionKind p_kind, uindex_t p_type_index, uindex_t p_def_index)
{
    uindex_t t_imp_index;
    if (!__extend_array(self, self -> module . imported_definitions, self -> module . imported_definition_count, t_imp_index) ||
        !MCMemoryNew((MCScriptExternalDefinition*&)self -> module . definitions[p_def_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_def_index, p_name);
    
    MCScriptImportedDefinition *t_import;
    t_import = &self -> module . imported_definitions[t_imp_index];
    
    t_import -> module = p_module_index;
    t_import -> kind = p_kind;
    t_import -> name = MCValueRetain(p_name);
    
    MCScriptExternalDefinition *t_definition;
    t_definition = static_cast<MCScriptExternalDefinition *>(self -> module . definitions[p_def_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindExternal;
    t_definition -> index = t_imp_index;
}

void MCScriptAddImportToModule(MCScriptModuleBuilderRef self, uindex_t p_index, MCNameRef p_name, MCScriptDefinitionKind p_kind, uindex_t p_type_index, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
    {
        r_index = 0;
        return;
    }
    
    for(uindex_t i = 0; i < self -> module . imported_definition_count; i++)
        if (MCNameIsEqualToCaseless(p_name, self -> module . imported_definitions[i] . name) &&
            p_kind == self -> module . imported_definitions[i] . kind &&
            p_index == self -> module . imported_definitions[i] . module)
        {
            for(uindex_t j = 0; j < self -> module . definition_count; j++)
                if (self -> module . definitions[j] != nil &&
                    self -> module . definitions[j] -> kind == kMCScriptDefinitionKindExternal &&
                    static_cast<MCScriptExternalDefinition *>(self -> module . definitions[j]) -> index == i)
                {
                    r_index = j;
                    return;
                }
        }
    
    MCScriptAddDefinitionToModule(self, p_kind, r_index);
    MCScriptAddImportToModuleWithIndex(self, p_index, p_name, p_kind, p_type_index, r_index);
    
    if (!self -> valid)
        r_index = 0;
}

void MCScriptAddDefinitionToModule(MCScriptModuleBuilderRef self, MCScriptDefinitionKind p_kind, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    uindex_t t_dindex;
    if (!__extend_array(self, self -> module . definitions, self -> module . definition_count, r_index) ||
        !__extend_array(self, self -> definition_kinds, self -> definition_kind_count, t_dindex) ||
        !__append_definition_name(self, kMCEmptyName))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    self -> definition_kinds[t_dindex] = p_kind;
}

void MCScriptAddValueToModule(MCScriptModuleBuilderRef self, MCValueRef p_value, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_constant(self, p_value, r_index);
}

void MCScriptBeginListValueInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_list_value != nil ||
        !MCProperListCreateMutable(self -> current_list_value))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptContinueListValueInModule(MCScriptModuleBuilderRef self, uindex_t p_const_idx)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_list_value == nil ||
        !MCProperListPushElementOntoBack(self -> current_list_value, self -> module . values[p_const_idx]))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptEndListValueInModule(MCScriptModuleBuilderRef self, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    MCScriptAddValueToModule(self, self -> current_list_value, r_index);
    
    MCValueRelease(self -> current_list_value);
    self -> current_list_value = nil;
}

void MCScriptBeginArrayValueInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;

    if (self -> current_array_value != nil ||
        !MCArrayCreateMutable(self -> current_array_value))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptContinueArrayValueInModule(MCScriptModuleBuilderRef self,
                                        uindex_t p_key_idx,
                                        uindex_t p_value_idx)
{
	MCNewAutoNameRef t_key;

    if (self == nil || !self -> valid)
        return;

    if (self -> current_array_value == nil ||
        !MCNameCreate(reinterpret_cast<MCStringRef>(self->module.values[p_key_idx]), &t_key) ||
        !MCArrayStoreValue(self -> current_array_value, false,
                           *t_key, self -> module . values[p_value_idx]))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptEndArrayValueInModule(MCScriptModuleBuilderRef self, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;

    MCScriptAddValueToModule(self, self -> current_array_value, r_index);

    MCValueRelease(self -> current_array_value);
    self -> current_array_value = nil;
}

void MCScriptAddTypeToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptTypeDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptTypeDefinition *t_definition;
    t_definition = static_cast<MCScriptTypeDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindType;
    t_definition -> type = p_type;
}

void MCScriptAddConstantToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_const_idx, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptConstantDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptConstantDefinition *t_definition;
    t_definition = static_cast<MCScriptConstantDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindConstant;
    t_definition -> value = p_const_idx;
}

void MCScriptAddVariableToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptVariableDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptVariableDefinition *t_definition;
    t_definition = static_cast<MCScriptVariableDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindVariable;
    t_definition -> type = p_type;
}

void MCScriptAddForeignHandlerToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_signature, MCStringRef p_binding, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptForeignHandlerDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptForeignHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptForeignHandlerDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindForeignHandler;
    t_definition -> type = p_signature;
    t_definition -> binding = MCValueRetain(p_binding);
}

void MCScriptAddPropertyToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_getter, uindex_t p_setter, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptPropertyDefinition *&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptPropertyDefinition *t_definition;
    t_definition = static_cast<MCScriptPropertyDefinition *>(self -> module . definitions[p_index]);
    
    // The property getters and setter references are stored with +1 indices, 0 meaning
    // no getter/setter.
    t_definition -> kind = kMCScriptDefinitionKindProperty;
    t_definition -> getter = p_getter + 1;
    t_definition -> setter = p_setter == UINDEX_MAX ? 0 : p_setter + 1;
}

void MCScriptAddEventToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptEventDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptEventDefinition *t_definition;
    t_definition = static_cast<MCScriptEventDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindEvent;
    t_definition -> type = p_type;
}

///////////

static void __add_script_type(MCScriptModuleBuilderRef self, MCScriptType *p_type, uindex_t& r_index)
{
    if (!__extend_array(self, self -> module . types, self -> module . type_count, r_index))
        return;
    
    self -> module . types[r_index] = p_type;
}

void MCScriptAddDefinedTypeToModule(MCScriptModuleBuilderRef self, uindex_t p_index, uindex_t& r_type)
{
    for(uindex_t i = 0; i < self -> module . type_count; i++)
        if (self -> module . types[i] -> kind == kMCScriptTypeKindDefined)
            if (static_cast<MCScriptDefinedType *>(self -> module . types[i]) -> index == p_index)
            {
                r_type = i;
                return;
            }
    
    MCScriptDefinedType *t_type;
    if (!MCMemoryNew(t_type))
    {
        self -> valid = false;
        return;
    }
    
    t_type -> kind = kMCScriptTypeKindDefined;
    t_type -> index = p_index;
    
    __add_script_type(self, t_type, r_type);
}

void MCScriptAddForeignTypeToModule(MCScriptModuleBuilderRef self, MCStringRef p_binding, uindex_t& r_type)
{
    for(uindex_t i = 0; i < self -> module . type_count; i++)
        if (self -> module . types[i] -> kind == kMCScriptTypeKindForeign)
            if (MCStringIsEqualTo(static_cast<MCScriptForeignType *>(self -> module . types[i]) -> binding, p_binding, kMCStringOptionCompareExact))
            {
                r_type = i;
                return;
            }
    
    MCScriptForeignType *t_type;
    if (!MCMemoryNew(t_type))
    {
        self -> valid = false;
        return;
    }
    
    t_type -> kind = kMCScriptTypeKindForeign;
    t_type -> binding = MCValueRetain(p_binding);
    
    __add_script_type(self, t_type, r_type);
}

void MCScriptAddOptionalTypeToModule(MCScriptModuleBuilderRef self, uindex_t p_type, uindex_t& r_type)
{
    if (self -> module . types[p_type] -> kind == kMCScriptTypeKindOptional)
    {
        r_type = p_type;
        return;
    }
        
    for(uindex_t i = 0; i < self -> module . type_count; i++)
        if (self -> module . types[i] -> kind == kMCScriptTypeKindOptional)
            if (static_cast<MCScriptOptionalType *>(self -> module . types[i]) -> type == p_type)
            {
                r_type = i;
                return;
            }
    
    MCScriptOptionalType *t_type;
    if (!MCMemoryNew(t_type))
    {
        self -> valid = false;
        return;
    }
    
    t_type -> kind = kMCScriptTypeKindOptional;
    t_type -> type = p_type;
    
    __add_script_type(self, t_type, r_type);
}

static void MCScriptBeginCommonHandlerTypeInModule(MCScriptModuleBuilderRef self, bool p_is_foreign, uindex_t p_return_type)
{
    if (self == nil ||
        !self -> valid)
        return;
    
    MCScriptHandlerType *t_type;
    if (!MCMemoryNew(t_type))
    {
        self -> valid = false;
        return;
    }
    
    t_type -> kind = p_is_foreign ? kMCScriptTypeKindForeignHandler : kMCScriptTypeKindHandler;
    t_type -> return_type = p_return_type;
    
    self -> current_type = t_type;
}

void MCScriptBeginHandlerTypeInModule(MCScriptModuleBuilderRef self, uindex_t p_return_type)
{
    MCScriptBeginCommonHandlerTypeInModule(self, false, p_return_type);
}

void MCScriptBeginForeignHandlerTypeInModule(MCScriptModuleBuilderRef self, uindex_t p_return_type)
{
    MCScriptBeginCommonHandlerTypeInModule(self, true, p_return_type);
}

void MCScriptContinueHandlerTypeInModule(MCScriptModuleBuilderRef self, MCScriptHandlerTypeParameterMode p_mode, MCNameRef p_name, uindex_t p_type)
{
    if (self == nil ||
        !self -> valid ||
        self -> current_type == nil)
        return;
    
    MCScriptHandlerType *t_type;
    t_type = static_cast<MCScriptHandlerType *>(self -> current_type);
    
    uindex_t t_param_index, t_name_index;
    if (!__extend_array(self, t_type -> parameters, t_type -> parameter_count, t_param_index) ||
        !__extend_array(self, t_type -> parameter_names, t_type -> parameter_name_count, t_name_index))
        return;
    
    t_type -> parameters[t_param_index] . mode = p_mode;
    t_type -> parameters[t_param_index] . type = p_type;
    t_type -> parameter_names[t_name_index] = MCValueRetain(p_name);
}

void MCScriptEndHandlerTypeInModule(MCScriptModuleBuilderRef self, uindex_t& r_new_type)
{
    if (self == nil ||
        !self -> valid ||
        self -> current_type == nil)
        return;
    
    MCScriptHandlerType *t_type;
    t_type = static_cast<MCScriptHandlerType *>(self -> current_type);
    
    for(uindex_t i = 0; i < self -> module . type_count; i++)
    {
        if (self -> module . types[i] -> kind != t_type -> kind)
            continue;
        
        MCScriptHandlerType *t_other_type;
        t_other_type = static_cast<MCScriptHandlerType *>(self -> module . types[i]);
        
        if (t_type -> parameter_count != t_other_type -> parameter_count)
            continue;
        
        if (t_type -> return_type != t_other_type -> return_type)
            continue;
        
        bool t_equal;
        t_equal = true;
        for(uindex_t j = 0; j < t_type -> parameter_count; j++)
            if (t_type -> parameters[j] . mode != t_other_type -> parameters[j] . mode ||
                t_type -> parameters[j] . type != t_other_type -> parameters[j] . type)
            {
                t_equal = false;
                break;
            }
        
        if (!t_equal)
            continue;
        
        MCMemoryDeleteArray(t_type -> parameters);
        MCMemoryDelete(t_type);
        
        r_new_type = i;
        return;
    }
    
    __add_script_type(self, t_type, r_new_type);
}

void MCScriptBeginRecordTypeInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil ||
        !self -> valid)
        return;
    
    MCScriptRecordType *t_type;
    if (!MCMemoryNew(t_type))
    {
        self -> valid = false;
        return;
    }
    
    t_type -> kind = kMCScriptTypeKindRecord;
    
    self -> current_type = t_type;
}

void MCScriptContinueRecordTypeInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type)
{
    if (self == nil ||
        !self -> valid ||
        self -> current_type == nil)
        return;
    
    MCScriptRecordType *t_type;
    t_type = static_cast<MCScriptRecordType *>(self -> current_type);
    
    uindex_t t_index;
    if (!__extend_array(self, t_type -> fields, t_type -> field_count, t_index))
        return;
    
    t_type -> fields[t_index] . name = MCValueRetain(p_name);
    t_type -> fields[t_index] . type = p_type;
}

void MCScriptEndRecordTypeInModule(MCScriptModuleBuilderRef self, uindex_t& r_new_type)
{
    if (self == nil ||
        !self -> valid ||
        self -> current_type == nil)
        return;
    
    MCScriptRecordType *t_type;
    t_type = static_cast<MCScriptRecordType *>(self -> current_type);
    
    for(uindex_t i = 0; i < self -> module . type_count; i++)
    {
        if (self -> module . types[i] -> kind != kMCScriptTypeKindRecord)
            continue;
        
        MCScriptRecordType *t_other_type;
        t_other_type = static_cast<MCScriptRecordType *>(self -> module . types[i]);
        
        if (t_type -> field_count != t_other_type -> field_count)
            continue;
        
        bool t_equal;
        t_equal = true;
        for(uindex_t j = 0; j < t_type -> field_count; j++)
            if (!MCNameIsEqualToCaseless(t_type -> fields[j] . name, t_other_type -> fields[j] . name) ||
                t_type -> fields[j] . type != t_other_type -> fields[j] . type)
            {
                t_equal = false;
                break;
            }
        
        if (!t_equal)
            continue;
        
        for(uindex_t j = 0; j < t_type -> field_count; j++)
            MCValueRelease(t_type -> fields[i] . name);
        MCMemoryDeleteArray(t_type -> fields);
        MCMemoryDelete(t_type);
        
        r_new_type = i;
        return;
    }
    
    __add_script_type(self, t_type, r_new_type);
}


///////////

void MCScriptBeginSyntaxInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptSyntaxDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptSyntaxDefinition *t_definition;
    t_definition = static_cast<MCScriptSyntaxDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindSyntax;
    
    self -> current_syntax = p_index;
}

void MCScriptBeginSyntaxMethodInModule(MCScriptModuleBuilderRef self, uindex_t p_handler)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_syntax == UINDEX_MAX)
    {
        self -> valid = false;
        return;
    }
    
    MCScriptSyntaxDefinition *t_syntax;
    t_syntax = static_cast<MCScriptSyntaxDefinition *>(self -> module . definitions[self -> current_syntax]);
    
    uindex_t t_index;
    if (!__extend_array(self, t_syntax -> methods, t_syntax -> method_count, t_index))
    {
        self -> valid = false;
        return;
    }
    
    t_syntax -> methods[t_index] . handler = p_handler;
}

static void MCScriptAddArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_syntax == UINDEX_MAX)
    {
        self -> valid = false;
        return;
    }
    
    MCScriptSyntaxDefinition *t_syntax;
    t_syntax = static_cast<MCScriptSyntaxDefinition *>(self -> module . definitions[self -> current_syntax]);
    
    MCScriptSyntaxMethod *t_method;
    t_method = &t_syntax -> methods[t_syntax -> method_count - 1];
    
    uindex_t t_arg_index;
    if (!__extend_array(self, t_method -> arguments, t_method -> argument_count, t_arg_index))
        return;
    
    t_method -> arguments[t_arg_index] = p_index;
}

void MCScriptAddBuiltinArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    // Encode builtin arguments 0, 1, 2, 3
    p_index = (p_index + 1) << 1;
    MCScriptAddArgumentToSyntaxMethodInModule(self, p_index);
}

void MCScriptAddConstantArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef self, MCValueRef p_value)
{
    uindex_t t_index;
    __emit_constant(self, p_value, t_index);
    
    if (self == nil || !self -> valid)
        return;
    
    // Encode constants as 4 + (index * 2)
    MCScriptAddArgumentToSyntaxMethodInModule(self, (t_index + 2) << 1);
}

void MCScriptAddVariableArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    // Encode variables as 4 + (index * 2) + 1
    MCScriptAddArgumentToSyntaxMethodInModule(self, (p_index << 2));
}

void MCScriptAddIndexedVariableArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef self, uindex_t p_var_index, uindex_t p_element_index)
{
    // Encode indexed variables as bottom bit 0, subsequent bit 1, next 2 bits index.
    MCScriptAddArgumentToSyntaxMethodInModule(self, (1 << 1) | ((p_element_index & 0x3) << 2) | (p_var_index << 4));
}

void MCScriptEndSyntaxMethodInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_syntax == UINDEX_MAX)
    {
        self -> valid = false;
        return;
    }
}

void MCScriptEndSyntaxInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_syntax == UINDEX_MAX)
    {
        self -> valid = false;
        return;
    }

    self -> current_syntax = UINDEX_MAX;
}

///////////

void MCScriptBeginDefinitionGroupInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(0, self -> current_handler_group, self -> current_handler_group_size))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptAddHandlerToDefinitionGroupInModule(MCScriptModuleBuilderRef self, uindex_t index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> current_handler_group_size; i++)
        if (self -> current_handler_group[i] == index)
            return;
    
    uindex_t t_gindex;
    if (!__extend_array(self, self -> current_handler_group, self -> current_handler_group_size, t_gindex))
        return;
    
    self -> current_handler_group[t_gindex] = index;
}

void MCScriptEndDefinitionGroupInModule(MCScriptModuleBuilderRef self, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . definition_count; i++)
    {
        if (self -> module . definitions[i] == nil)
            continue;
        
        if (self -> module . definitions[i] -> kind != kMCScriptDefinitionKindDefinitionGroup)
            continue;
        
        MCScriptDefinitionGroupDefinition *t_group;
        t_group = (MCScriptDefinitionGroupDefinition *)self -> module . definitions[i];
        if (self -> current_handler_group_size != t_group -> handler_count)
            continue;
        
        if (MCMemoryCompare(t_group -> handlers, self -> current_handler_group, sizeof(uindex_t) * self -> current_handler_group_size) != 0)
            continue;
        
        r_index = i;
        return;
    }
    
    uindex_t t_index;
    MCScriptAddDefinitionToModule(self, kMCScriptDefinitionKindDefinitionGroup, t_index);
    if (!self -> valid)
        return;
    
    if (!MCMemoryNew((MCScriptDefinitionGroupDefinition *&)self -> module . definitions[t_index]))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptDefinitionGroupDefinition *t_group;
    t_group = (MCScriptDefinitionGroupDefinition *)self -> module . definitions[t_index];
    
    t_group -> kind = kMCScriptDefinitionKindDefinitionGroup;
    t_group -> handler_count = self -> current_handler_group_size;
    t_group -> handlers = self -> current_handler_group;
    
    self -> current_handler_group_size = 0;
    self -> current_handler_group = nil;
    
    r_index = t_index;
}

///////////

static uindex_t __measure_uint(uindex_t p_value)
{
    if ((p_value & (1 << 31)) != 0)
        return 2;
    
    if (p_value < (1 << 7))
        return 1;
    if (p_value < (1 << 14))
        return 2;
    if (p_value < (1 << 21))
        return 3;
    if (p_value < (1 << 28))
        return 4;
    return 5;
}

static uindex_t __measure_instruction(MCScriptModuleBuilderRef self, MCScriptBytecodeInstruction *p_instruction)
{
    uindex_t t_size;
    t_size = 1;
    
    if (p_instruction -> arity >= kMCScriptBytecodeOpArityMax)
        t_size += 1;
    
    if (p_instruction -> operation == kMCScriptBytecodeOpJump)
        t_size += 2;
    else if (p_instruction -> operation >= kMCScriptBytecodeOpJumpIfFalse && p_instruction -> operation <= kMCScriptBytecodeOpJumpIfTrue)
    {
        t_size += __measure_uint(self -> operands[p_instruction -> operands]);
        t_size += 2;
    }
    else
    {
        for(uindex_t i = 0; i < p_instruction -> arity; i++)
            t_size += __measure_uint(self -> operands[p_instruction -> operands + i]);
    }
    
    return t_size;
}

static void __emit_bytecode_byte(MCScriptModuleBuilderRef self, uint8_t p_byte)
{
    if (self == nil || !self -> valid)
        return;
    
    uindex_t t_index;
    if (!__extend_array(self, self -> module . bytecode, self -> module . bytecode_count, t_index))
        return;
    
    self -> module . bytecode[t_index] = p_byte;
}

static void __emit_bytecode_uint(MCScriptModuleBuilderRef self, uindex_t p_value)
{
    if (self == nil || !self -> valid)
        return;
    
    uint8_t t_bytes[5];
    uindex_t t_index;
    if ((p_value & (1 << 31)) != 0)
    {
        p_value &= ~(1 << 31);
        
        t_bytes[0] = (p_value & 0x7f) | 0x80;
        t_bytes[1] = ((p_value >> 7) & 0x7f);
        t_index = 2;
        
        if ((p_value >> 14) != 0)
            MCAssert(false);
    }
    else
    {
        t_index = 0;
        do
        {
            // Fetch the next 7 bits.
            uint8_t t_byte;
            t_byte = p_value & 0x7f;
            
            // Remove from the value.
            p_value = p_value >> 7;
            
            // If there is anything left in the value, mark the top-bit.
            if (p_value != 0)
                t_byte |= 1 << 7;
            
            t_bytes[t_index++] = t_byte;
        }
        while(p_value != 0);
    }
    
    if (!MCMemoryResizeArray(self -> module . bytecode_count + t_index, self -> module . bytecode, self -> module . bytecode_count))
    {
        self -> valid = false;
        return;
    }
    
    MCMemoryCopy(self -> module . bytecode + self -> module . bytecode_count - t_index, t_bytes, t_index);
}

static void __emit_constant(MCScriptModuleBuilderRef self, MCValueRef p_constant, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . value_count; i++)
        if (MCValueIsEqualTo(p_constant, self -> module . values[i]))
        {
            r_index = i;
            return;
        }
    
    uindex_t t_vindex;
    if (!__extend_array(self, self -> module . values, self -> module . value_count, t_vindex))
        return;
    
    self -> module . values[t_vindex] = MCValueRetain(p_constant);
    
    r_index = t_vindex;
}

static void __begin_instruction(MCScriptModuleBuilderRef self, MCScriptBytecodeOp p_operation)
{
    uindex_t t_index;
    if (!__extend_array(self, self -> instructions, self -> instruction_count, t_index))
        return;
    
    self -> instructions[t_index] . file = self -> current_file;
    self -> instructions[t_index] . line = self -> current_line;
    
    self -> instructions[t_index] . operation = p_operation;
    self -> instructions[t_index] . arity = 0;
    self -> instructions[t_index] . operands = self -> operand_count;
}

static void __continue_instruction(MCScriptModuleBuilderRef self, uindex_t p_argument)
{
    uindex_t t_op_index;
    if (!__extend_array(self, self -> operands, self -> operand_count, t_op_index))
        return;
    
    if (self -> instructions[self -> instruction_count - 1] . arity == 256)
    {
        self -> valid = false;
        return;
    }
    
    self -> instructions[self -> instruction_count - 1] . arity += 1;
    self -> operands[t_op_index] = p_argument;
}

static void __end_instruction(MCScriptModuleBuilderRef self)
{
    // Nothing to do!
}

static void __emit_position(MCScriptModuleBuilderRef self, uindex_t p_address, uindex_t p_file, uindex_t p_line)
{
    MCScriptPosition *t_last_pos;
    if (self -> module . position_count > 0)
        t_last_pos = &self -> module . positions[self -> module . position_count - 1];
    else
        t_last_pos = nil;
    
    if (t_last_pos != nil &&
        t_last_pos -> file == p_file &&
        t_last_pos -> line == p_line)
        return;
    
    uindex_t t_pindex;
    if (!__extend_array(self, self -> module . positions, self -> module . position_count, t_pindex))
        return;
    
    self -> module . positions[t_pindex] . address = p_address;
    self -> module . positions[t_pindex] . file = p_file;
    self -> module . positions[t_pindex] . line = p_line;
}

void MCScriptBeginHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, MCScriptHandlerAttributes p_attributes, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptHandlerDefinition*&)self -> module . definitions[p_index]))
    {
        self -> valid = false;
        return;
    }
    
    __assign_definition_name(self, p_index, p_name);
    
    MCScriptHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindHandler;
    t_definition -> type = p_type;
    t_definition -> attributes = p_attributes;
    t_definition -> start_address = self -> module . bytecode_count;
    
    self -> current_handler = p_index;
    self -> current_param_count = 0;
}

void MCScriptEndHandlerInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_handler == UINDEX_MAX)
    {
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerDefinition *t_handler;
    t_handler = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[self -> current_handler]);

    // Compute addresses for each instruction.
    uindex_t t_address;
    t_address = 0;
    for(uindex_t i = 0; i < self -> instruction_count; i++)
    {
        self -> instructions[i] . address = t_address;
        t_address += __measure_instruction(self, &self -> instructions[i]);
    }

    // Compute real addresses for jumps.
    for(uindex_t i = 0; i < self -> instruction_count; i++)
    {
        MCScriptBytecodeOp t_op;
        uindex_t *t_operands;
        
        t_address = self -> instructions[i] . address;
        t_op = self -> instructions[i] . operation;
        t_operands = &self -> operands[self -> instructions[i] . operands];
        
        uindex_t t_address_index;
        if (t_op == kMCScriptBytecodeOpJump)
            t_address_index = 0;
        else if (t_op == kMCScriptBytecodeOpJumpIfFalse || t_op == kMCScriptBytecodeOpJumpIfTrue)
            t_address_index = 1;
        else
            continue;
        
        index_t t_target_address;
        t_target_address = self -> instructions[self -> labels[t_operands[t_address_index] - 1] . instruction] . address - t_address;
        
        uindex_t t_encoded_target_address;
        if (t_target_address >= 0)
            t_encoded_target_address = t_target_address * 2;
        else
            t_encoded_target_address = (-t_target_address) * 2 + 1;
        
        t_operands[t_address_index] = (1 << 31) | t_encoded_target_address;
    }
    
    uindex_t t_pos_address_offset;
    t_pos_address_offset = self -> module . bytecode_count;
    for(uindex_t i = 0; i < self -> instruction_count; i++)
    {
        MCScriptBytecodeOp t_op;
        uindex_t t_arity;
        uindex_t *t_operands;
        
        t_op = self -> instructions[i] . operation;
        t_address = self -> instructions[i] . address;
        t_arity = self -> instructions[i] . arity;
        t_operands = &self -> operands[self -> instructions[i] . operands];
        
        __emit_position(self, t_pos_address_offset + t_address, self -> instructions[i] . file, self -> instructions[i] . line);
        
        __emit_bytecode_byte(self, (uint8_t)(t_op | (MCMin(t_arity, kMCScriptBytecodeOpArityMax) << kMCScriptBytecodeOpArityShift)));
        
        if (t_arity >= kMCScriptBytecodeOpArityMax)
            __emit_bytecode_byte(self, uint8_t(t_arity - kMCScriptBytecodeOpArityMax));
        
        for(uindex_t j = 0; j < t_arity; j++)
            __emit_bytecode_uint(self, t_operands[j]);
    }
    
    t_handler -> finish_address = self -> module . bytecode_count;
    
    self -> instruction_count = 0;
    self -> operand_count = 0;
    self -> label_count = 0;
    self -> current_handler = UINDEX_MAX;
}

void MCScriptAddParameterToHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_handler == UINDEX_MAX)
    {
        r_index = 0;
        self -> valid = false;
        return;
    }

    r_index = self -> current_param_count;
    
    self -> current_param_count += 1;
}

void MCScriptAddVariableToHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_type, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_handler == UINDEX_MAX)
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerDefinition *t_handler;
    t_handler = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[self -> current_handler]);
    
    uindex_t t_tindex, t_nindex;
    if (!__extend_array(self, t_handler -> local_types, t_handler -> local_type_count, t_tindex) ||
        !__extend_array(self, t_handler -> local_names, t_handler -> local_name_count, t_nindex))
    {
        r_index = 0;
        return;
    }

    t_handler -> local_types[t_tindex] = p_type;
    t_handler -> local_names[t_nindex] = MCValueRetain(p_name);
    
    r_index = self -> current_param_count + t_handler -> local_type_count - 1;
}

void MCScriptDeferLabelForBytecodeInModule(MCScriptModuleBuilderRef self, uindex_t& r_label)
{
    if (self == nil || !self -> valid)
        return;
 
    uindex_t t_lindex;
    if (!__extend_array(self, self -> labels, self -> label_count, t_lindex))
    {
        r_label = 0;
        return;
    }
    
    self -> labels[t_lindex] . instruction = UINDEX_MAX;
    
    r_label = t_lindex + 1;
}

void MCScriptResolveLabelForBytecodeInModule(MCScriptModuleBuilderRef self, uindex_t p_label)
{
    if (self == nil || !self -> valid)
        return;
 
    self -> labels[p_label - 1] . instruction = self -> instruction_count;
}

static MCScriptDefinitionKind __kind_of_definition(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    if (self -> module . definitions[p_index] == nil)
        return self -> definition_kinds[p_index];
    
    if (self -> module . definitions[p_index] -> kind == kMCScriptDefinitionKindExternal)
    {
        MCScriptExternalDefinition *t_ext_definition;
        t_ext_definition = static_cast<MCScriptExternalDefinition *>(self -> module . definitions[p_index]);
        return self -> module . imported_definitions[t_ext_definition -> index] . kind;
    }
    
    return self -> module . definitions[p_index] -> kind;
}

static bool __is_valid_definition(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    if (p_index >= self -> module . definition_count)
        return false;
    
    switch(__kind_of_definition(self, p_index))
    {
        case kMCScriptDefinitionKindVariable:
        case kMCScriptDefinitionKindHandler:
        case kMCScriptDefinitionKindForeignHandler:
        case kMCScriptDefinitionKindConstant:
            return true;
            
        default:
            break;
    }
    
    return false;
}

static bool __is_valid_variable_definition(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    if (p_index >= self -> module . definition_count)
        return false;
    
    switch(__kind_of_definition(self, p_index))
    {
        case kMCScriptDefinitionKindVariable:
            return true;
            
        default:
            break;
    }
    
    return false;
}

static bool __is_valid_handler_definition(MCScriptModuleBuilderRef self, uindex_t p_index)
{
    if (p_index >= self -> module . definition_count)
        return false;
    
    switch(__kind_of_definition(self, p_index))
    {
        case kMCScriptDefinitionKindHandler:
        case kMCScriptDefinitionKindForeignHandler:
        case kMCScriptDefinitionKindDefinitionGroup:
            return true;
            
        default:
            break;
    }
    
    return false;
}

void MCScriptEmitBytecodeInModuleA(MCScriptModuleBuilderRef self, uindex_t p_opcode, uindex_t *p_arguments, uindex_t p_argument_count)
{
    if (self == nil || !self -> valid)
        return;
	
	const MCScriptBytecodeOpInfo *t_info;
	if (!MCScriptDescribeBytecodeOp(p_opcode, t_info) ||
		!MCScriptCheckBytecodeParameterCount(p_opcode, p_argument_count))
	{
		self -> valid = false;
		return;
	}
    
    for(uindex_t i = 0; i < p_argument_count; i++)
    {
        MCScriptBytecodeParameterType t_param_type;
        t_param_type = MCScriptDescribeBytecodeParameter(p_opcode,
                                                         i);
        switch(t_param_type)
        {
			case kMCScriptBytecodeParameterTypeUnknown:
                self -> valid = false;
                return;
                
            case kMCScriptBytecodeParameterTypeLabel:
                if (p_arguments[i] == 0 ||
                    p_arguments[i] > self -> label_count)
				{
                    self -> valid = false;
                    return;
                }
                break;
                
            case kMCScriptBytecodeParameterTypeRegister:
                // Register file is open ended
                break;
                
            case kMCScriptBytecodeParameterTypeConstant:
                if (p_arguments[i] >= self -> module . value_count)
				{
                    self -> valid = false;
                    return;
                }
                break;
                
            case kMCScriptBytecodeParameterTypeDefinition:
                if (!__is_valid_definition(self, p_arguments[i]))
				{
                    self -> valid = false;
                    return;
                }
                break;
                
            case kMCScriptBytecodeParameterTypeVariable:
                if (!__is_valid_variable_definition(self, p_arguments[i]))
				{
                    self -> valid = false;
                    return;
                }
                break;
                
            case kMCScriptBytecodeParameterTypeHandler:
                if (!__is_valid_handler_definition(self, p_arguments[i]))
				{
                    self -> valid = false;
                    return;
                }
                break;
        }
    }
    
    __begin_instruction(self,
                        t_info->code);
    for(uindex_t i = 0; i < p_argument_count; i++)
        __continue_instruction(self,
                               p_arguments[i]);
    __end_instruction(self);
}

void MCScriptEmitBytecodeInModuleV(MCScriptModuleBuilderRef self, uindex_t p_opcode, va_list p_args)
{
    MCAutoArray<uindex_t> t_args;
    for(;;)
    {
        uindex_t t_arg;
        t_arg = va_arg(p_args, uindex_t);
        if (t_arg == UINDEX_MAX)
            break;
        
        if (!t_args.Push(t_arg))
        {
            self -> valid = false;
            return;
        }
    }
    
    MCScriptEmitBytecodeInModuleA(self,
                                  p_opcode,
                                  t_args.Ptr(),
                                  t_args.Size());
}

void MCScriptEmitBytecodeInModule(MCScriptModuleBuilderRef self, uindex_t p_opcode, ...)
{
    va_list t_args;
    va_start(t_args, p_opcode);
    MCScriptEmitBytecodeInModuleV(self,
                                  p_opcode,
                                  t_args);
    va_end(t_args);
}

void MCScriptEmitPositionForBytecodeInModule(MCScriptModuleBuilderRef self, MCNameRef p_file, uindex_t p_line)
{
    if (self == nil || !self -> valid)
        return;

    self -> current_line = p_line;
    
    for(uindex_t i = 0; i < self -> module . source_file_count; i++)
        if (MCNameIsEqualToCaseless(p_file, self -> module . source_files[i]))
        {
            self -> current_file = i;
            return;
        }
    
    uindex_t t_findex;
    if (!__extend_array(self, self -> module . source_files, self -> module . source_file_count, t_findex))
        return;
    
    self -> module . source_files[t_findex] = MCValueRetain(p_file);
    self -> current_file = t_findex;
}

////////////////////////////////////////////////////////////////////////////////

static MCScriptForeignPrimitiveType
MCScriptMapTypeToForeignPrimitiveTypeInModule(MCScriptModuleBuilderRef self, uindex_t p_type_index, bool p_is_return_type = false)
{
    MCScriptType *t_type = self->module.types[p_type_index];
    if (t_type->kind == kMCScriptTypeKindDefined)
    {
        auto t_def_type = static_cast<MCScriptDefinedType *>(t_type);
        
        MCNameRef t_name = nullptr;

        MCScriptDefinition *t_def = self->module.definitions[t_def_type->index];
        if (t_def->kind == kMCScriptDefinitionKindType)
        {
            uindex_t t_other_type_index = static_cast<MCScriptTypeDefinition *>(t_def)->type;
            MCScriptType *t_other_type = self->module.types[t_other_type_index];
            if (t_other_type->kind != kMCScriptTypeKindForeign)
            {
                return MCScriptMapTypeToForeignPrimitiveTypeInModule(self, t_other_type_index);
            }
            
            t_name = self->module.definition_names[t_def_type->index];
        }
        else if (t_def->kind == kMCScriptDefinitionKindExternal)
        {
            const MCScriptImportedDefinition& t_imp_def = self->module.imported_definitions[static_cast<MCScriptExternalDefinition *>(t_def)->index];
            t_name = t_imp_def.name;
        }
        else
        {
            MCLog("can't handle type of kind %d", t_def->kind);
            return kMCScriptForeignPrimitiveTypeUnknown;
        }
        
        if (p_is_return_type &&
            MCStringIsEqualToCString(MCNameGetString(t_name),
                                     "undefined",
                                     kMCStringOptionCompareCaseless))
        {
            return kMCScriptForeignPrimitiveTypeVoid;
        }
        
        /* This list must be kept up to date with all types which we bind to
         * to in foreign handlers in the modules embedded in the engine. */
        static struct { const char *name; MCScriptForeignPrimitiveType type; } s_ext_mappings[] =
        {
            /* Foundation Types */
            { "undefined", kMCScriptForeignPrimitiveTypePointer },
            { "any", kMCScriptForeignPrimitiveTypePointer },
            { "bool", kMCScriptForeignPrimitiveTypeCBool },
            { "double", kMCScriptForeignPrimitiveTypeCDouble },
            { "sint", kMCScriptForeignPrimitiveTypeSInt },
            { "uint", kMCScriptForeignPrimitiveTypeUInt },
            
            { "Boolean", kMCScriptForeignPrimitiveTypePointer },
            { "Number", kMCScriptForeignPrimitiveTypePointer },
            { "String", kMCScriptForeignPrimitiveTypePointer },
            { "Data", kMCScriptForeignPrimitiveTypePointer },
            { "Array", kMCScriptForeignPrimitiveTypePointer },
            { "List", kMCScriptForeignPrimitiveTypePointer },
            
            /* Foreign C Types */
            { "SInt8", kMCScriptForeignPrimitiveTypeSInt8 },
            { "UInt8", kMCScriptForeignPrimitiveTypeUInt8 },
            { "SInt16", kMCScriptForeignPrimitiveTypeSInt16 },
            { "UInt16", kMCScriptForeignPrimitiveTypeUInt16 },
            { "SInt32", kMCScriptForeignPrimitiveTypeSInt32 },
            { "UInt32", kMCScriptForeignPrimitiveTypeUInt32 },
            { "SInt64", kMCScriptForeignPrimitiveTypeSInt64 },
            { "UInt64", kMCScriptForeignPrimitiveTypeUInt64 },
            
            { "SIntSize", kMCScriptForeignPrimitiveTypeSIntSize },
            { "UIntSize", kMCScriptForeignPrimitiveTypeUIntSize },
            { "SIntPtr", kMCScriptForeignPrimitiveTypeSIntPtr },
            { "UIntPtr", kMCScriptForeignPrimitiveTypeUIntPtr },
            
            { "CBool", kMCScriptForeignPrimitiveTypeCBool },
            { "CChar", kMCScriptForeignPrimitiveTypeCChar },
            { "CSChar", kMCScriptForeignPrimitiveTypeCSChar },
            { "CUChar", kMCScriptForeignPrimitiveTypeCUChar },
            { "CSShort", kMCScriptForeignPrimitiveTypeCSShort },
            { "CShort", kMCScriptForeignPrimitiveTypeCSShort },
            { "CUShort", kMCScriptForeignPrimitiveTypeCUShort },
            { "CSInt", kMCScriptForeignPrimitiveTypeCSInt },
            { "CInt", kMCScriptForeignPrimitiveTypeCSInt },
            { "CUInt", kMCScriptForeignPrimitiveTypeCUInt },
            { "CSLong", kMCScriptForeignPrimitiveTypeCSLong },
            { "CLong", kMCScriptForeignPrimitiveTypeCSLong },
            { "CULong", kMCScriptForeignPrimitiveTypeCULong },
            { "CSLongLong", kMCScriptForeignPrimitiveTypeCSLongLong },
            { "CLongLong", kMCScriptForeignPrimitiveTypeCSLongLong },
            { "CULongLong", kMCScriptForeignPrimitiveTypeCULongLong },
            { "CDouble", kMCScriptForeignPrimitiveTypeCDouble },
            { "CFloat", kMCScriptForeignPrimitiveTypeCFloat },
            { "LCSInt", kMCScriptForeignPrimitiveTypeSInt },
            { "LCInt", kMCScriptForeignPrimitiveTypeSInt },
            { "LCIndex", kMCScriptForeignPrimitiveTypeSInt },
            { "LCUInt", kMCScriptForeignPrimitiveTypeUInt },
            { "LCUIndex", kMCScriptForeignPrimitiveTypeUInt },
            
            { "Float32", kMCScriptForeignPrimitiveTypeFloat32 },
            { "Float64", kMCScriptForeignPrimitiveTypeFloat64 },
            
            { "Pointer", kMCScriptForeignPrimitiveTypePointer },
            
            { "ZStringNative", kMCScriptForeignPrimitiveTypePointer },
            { "ZStringUTF8", kMCScriptForeignPrimitiveTypePointer },
            { "ZStringUTF16", kMCScriptForeignPrimitiveTypePointer },
            
            /* Natural Types */
            { "NaturalUInt", kMCScriptForeignPrimitiveTypeNaturalUInt },
            { "NaturalSInt", kMCScriptForeignPrimitiveTypeNaturalSInt },
            { "NaturalFloat", kMCScriptForeignPrimitiveTypeNaturalFloat },
            
            /* Java FFI Types */
            { "JObject", kMCScriptForeignPrimitiveTypePointer },
            
            /* ObjC FFI Types */
            { "ObjcObject", kMCScriptForeignPrimitiveTypePointer },
            { "ObjcId", kMCScriptForeignPrimitiveTypePointer },
            { "ObjcRetainedId", kMCScriptForeignPrimitiveTypePointer },
            { "ObjcAutoreleasedId", kMCScriptForeignPrimitiveTypePointer },
            
            /* JavaScript FFI Types */
            { "JSObject", kMCScriptForeignPrimitiveTypePointer },
            
            /* Extra Foundation Types */
            { "Stream", kMCScriptForeignPrimitiveTypePointer },
            
            /* Canvas Types */
            { "Rectangle", kMCScriptForeignPrimitiveTypePointer },
            { "Point", kMCScriptForeignPrimitiveTypePointer },
            { "Image", kMCScriptForeignPrimitiveTypePointer },
            { "Color", kMCScriptForeignPrimitiveTypePointer },
            { "Paint", kMCScriptForeignPrimitiveTypePointer },
            { "SolidPaint", kMCScriptForeignPrimitiveTypePointer },
            { "Pattern", kMCScriptForeignPrimitiveTypePointer },
            { "Gradient", kMCScriptForeignPrimitiveTypePointer },
            { "GradientStop", kMCScriptForeignPrimitiveTypePointer },
            { "Path", kMCScriptForeignPrimitiveTypePointer },
            { "Effect", kMCScriptForeignPrimitiveTypePointer },
            { "Font", kMCScriptForeignPrimitiveTypePointer },
            { "Canvas", kMCScriptForeignPrimitiveTypePointer },
            { "Transform", kMCScriptForeignPrimitiveTypePointer },
            
            /* Engine Types */
            { "Widget", kMCScriptForeignPrimitiveTypePointer },
            { "ScriptObject", kMCScriptForeignPrimitiveTypePointer },
        };
        
        for(const auto t_mapping : s_ext_mappings)
        {
            if (MCStringIsEqualToCString(MCNameGetString(t_name), t_mapping.name, kMCStringOptionCompareCaseless))
            {
                return t_mapping.type;
            }
        }

        return kMCScriptForeignPrimitiveTypeUnknown;
    }
    else if (t_type->kind == kMCScriptTypeKindForeign)
    {
        return kMCScriptForeignPrimitiveTypeUnknown;
    }
    else if (t_type->kind == kMCScriptTypeKindOptional)
    {
        return kMCScriptForeignPrimitiveTypePointer;
    }
    else if (t_type->kind == kMCScriptTypeKindForeignHandler ||
             t_type->kind == kMCScriptTypeKindHandler)
    {
        return kMCScriptForeignPrimitiveTypePointer;
    }
    
    return kMCScriptForeignPrimitiveTypeUnknown;
}

MCScriptForeignPrimitiveType
MCScriptQueryForeignHandlerReturnTypeInModule(MCScriptModuleBuilderRef self, uindex_t p_type_index)
{
    if (self->module.types[p_type_index]->kind != kMCScriptTypeKindForeignHandler)
    {
        return kMCScriptForeignPrimitiveTypeUnknown;
    }
    
    auto t_handler_type = static_cast<MCScriptHandlerType *>(self -> module . types[p_type_index]);
    
    return MCScriptMapTypeToForeignPrimitiveTypeInModule(self, t_handler_type->return_type, true);
}

uindex_t
MCScriptQueryForeignHandlerParameterCountInModule(MCScriptModuleBuilderRef self, uindex_t p_type_index)
{
    if (self->module.types[p_type_index]->kind != kMCScriptTypeKindForeignHandler)
    {
        return 0;
    }
    
    auto t_handler_type = static_cast<MCScriptHandlerType *>(self -> module . types[p_type_index]);
    
    return t_handler_type->parameter_count;

}

MCScriptForeignPrimitiveType
MCScriptQueryForeignHandlerParameterTypeInModule(MCScriptModuleBuilderRef self, uindex_t p_type_index, uindex_t p_arg_index)
{
    if (self->module.types[p_type_index]->kind != kMCScriptTypeKindForeignHandler)
    {
        return kMCScriptForeignPrimitiveTypeUnknown;
    }
    
    auto t_handler_type = static_cast<MCScriptHandlerType *>(self -> module . types[p_type_index]);
    if (p_arg_index >= t_handler_type->parameter_count)
    {
        return kMCScriptForeignPrimitiveTypeUnknown;
    }
    
    /* Non-in modes map to a primitive pointer type from the point of view of
       FFI. */
    if (t_handler_type->parameters[p_arg_index].mode != kMCScriptHandlerTypeParameterModeIn)
    {
        return kMCScriptForeignPrimitiveTypePointer;
    }
    
    return MCScriptMapTypeToForeignPrimitiveTypeInModule(self, t_handler_type->parameters[p_arg_index].type);
}

////////////////////////////////////////////////////////////////////////////////
