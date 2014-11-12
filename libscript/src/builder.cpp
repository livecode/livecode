#include <foundation.h>

#include "script.h"
#include "script-private.h"

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
};

struct MCScriptModuleBuilder
{
    bool valid;
    
    MCScriptModule module;

    MCProperListRef definition_names;
    
    uindex_t current_handler;
    
    MCScriptBytecodeLabel *labels;
    uindex_t label_count;
    MCScriptBytecodeInstruction *instructions;
    uindex_t instruction_count;
    uindex_t *operands;
    uindex_t operand_count;
};

////////////////////////////////////////////////////////////////////////////////

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
    
    if (self -> valid)
        self -> valid = MCProperListCreateMutable(self -> definition_names);
    
    r_builder = self;
}

bool MCScriptEndModule(MCScriptModuleBuilderRef self, MCStreamRef p_stream)
{
    if (self == nil)
        return false;
    
    bool t_success;
    if (self -> valid)
        t_success = MCScriptWriteRawModule(p_stream, &self -> module);
    else
        t_success = false;
    
    MCScriptReleaseRawModule(&self -> module);
    
    MCValueRelease(self -> definition_names);
    
    MCMemoryDeleteArray(self -> labels);
    MCMemoryDeleteArray(self -> instructions);
    MCMemoryDeleteArray(self -> operands);
    
    MCMemoryDelete(self);

    return t_success;
}

void MCScriptAddDependencyToModule(MCScriptModuleBuilderRef self, MCNameRef p_dependency, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . dependency_count; i++)
        if (MCNameIsEqualTo(p_dependency, self -> module . dependencies[i] . name))
            return;
    
    if (!MCMemoryResizeArray(self -> module . dependency_count + 1, self -> module . dependencies, self -> module . dependency_count))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    self -> module . dependencies[self -> module . dependency_count - 1] . name = MCValueRetain(p_dependency);
    self -> module . dependencies[self -> module . dependency_count - 1] . version = 0;
    
    r_index = self -> module . dependency_count;
}

void MCScriptAddExportToModule(MCScriptModuleBuilderRef self, uindex_t p_definition)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . exported_definition_count + 1, self -> module . exported_definitions, self -> module . exported_definition_count))
    {
        self -> valid = false;
        return;
    }
    
    self -> module  . exported_definitions[self -> module . exported_definition_count - 1] . name = (MCNameRef)MCProperListFetchElementAtIndex(self -> definition_names, p_definition);
    self -> module  . exported_definitions[self -> module . exported_definition_count - 1] . index = p_definition;
}

void MCScriptAddImportToModule(MCScriptModuleBuilderRef self, uindex_t p_index, MCNameRef p_name, MCScriptDefinitionKind p_kind, MCTypeInfoRef p_type, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . imported_definition_count + 1, self -> module . imported_definitions, self -> module . imported_definition_count) ||
        !MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptExternalDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptImportedDefinition *t_import;
    t_import = &self -> module . imported_definitions[self -> module . imported_definition_count - 1];
    
    t_import -> module = p_index;
    t_import -> kind = p_kind;
    t_import -> name = MCValueRetain(p_name);
    t_import -> type = MCValueRetain(p_type);
    
    MCScriptExternalDefinition *t_definition;
    t_definition = static_cast<MCScriptExternalDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindExternal;
    t_definition -> index = self -> module . imported_definition_count;

    r_index = self -> module . imported_definition_count;
}

void MCScriptAddTypeToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptTypeDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
        
    MCScriptTypeDefinition *t_definition;
    t_definition = static_cast<MCScriptTypeDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindType;
    t_definition -> type = MCValueRetain(p_type);
    
    r_index = self -> module . imported_definition_count;
}

void MCScriptAddConstantToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCValueRef p_value, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptConstantDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptConstantDefinition *t_definition;
    t_definition = static_cast<MCScriptConstantDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindConstant;
    t_definition -> value = MCValueRetain(p_value);
    
    r_index = self -> module . imported_definition_count;
}

void MCScriptAddVariableToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptVariableDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptVariableDefinition *t_definition;
    t_definition = static_cast<MCScriptVariableDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindVariable;
    t_definition -> type = MCValueRetain(p_type);
    
    r_index = self -> module . imported_definition_count;
}

void MCScriptAddForeignHandlerToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, MCStringRef p_binding, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptForeignHandlerDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptForeignHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptForeignHandlerDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindForeignHandler;
    t_definition -> signature = MCValueRetain(p_signature);
    t_definition -> binding = MCValueRetain(p_binding);
    
    r_index = self -> module . imported_definition_count;
    
}

void MCScriptAddPropertyToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_getter, uindex_t p_setter, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptPropertyDefinition *&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptPropertyDefinition *t_definition;
    t_definition = static_cast<MCScriptPropertyDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindProperty;
    t_definition -> getter = p_getter;
    t_definition -> setter = p_setter;
    
    r_index = self -> module . imported_definition_count;
}

void MCScriptAddEventToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptEventDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptEventDefinition *t_definition;
    t_definition = static_cast<MCScriptEventDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindEvent;
    t_definition -> signature = MCValueRetain(p_signature);
    
    r_index = self -> module . imported_definition_count;
}

///////////

static uindex_t __measure_uint(uindex_t p_value)
{
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
    
    if (p_instruction -> arity >= 15)
        t_size += 1;
    
    if (p_instruction -> operation == kMCScriptBytecodeOpJump)
        t_size += 3;
    else if (p_instruction -> operation >= kMCScriptBytecodeOpJumpIfUndefined && p_instruction -> operation <= kMCScriptBytecodeOpJumpIfFalse)
    {
        t_size += __measure_uint(self -> operands[p_instruction -> operands]);
        t_size += 3;
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
    
    if (!MCMemoryResizeArray(self -> module . bytecode_count + 1, self -> module . bytecode, self -> module . bytecode_count))
    {
        self -> valid = false;
        return;
    }
    
    self -> module . bytecode[self -> module . bytecode_count - 1] = p_byte;
}

static void __emit_bytecode_uint(MCScriptModuleBuilderRef self, uindex_t p_value)
{
    if (self == nil || !self -> valid)
        return;
    
    uint8_t t_bytes[5];
    uindex_t t_index;
    t_index = 0;
    while(p_value != 0)
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
    
    if (!MCMemoryResizeArray(self -> module . bytecode_count + t_index, self -> module . bytecode, self -> module . bytecode_count))
    {
        self -> valid = false;
        return;
    }
    
    MCMemoryCopy(self -> module . bytecode - t_index - 1, t_bytes, t_index);
}

static void __emit_constant(MCScriptModuleBuilderRef self, MCValueRef p_constant, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . value_count; i++)
        if (MCValueIsEqualTo(p_constant, self -> module . values[i]))
        {
            r_index = i + 1;
            return;
        }
    
    if (!MCMemoryResizeArray(self -> module . value_count + 1, self -> module . values, self -> module . value_count))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    self -> module . values[self -> module . value_count - 1] = MCValueRetain(p_constant);
    
    r_index = self -> module . value_count;
}

static void __begin_instruction(MCScriptModuleBuilderRef self, MCScriptBytecodeOp p_operation)
{
    if (!MCMemoryResizeArray(self -> instruction_count + 1, self -> instructions, self -> instruction_count))
    {
        self -> valid = false;
        return;
    }
    
    self -> instructions[self -> instruction_count - 1] . operation = p_operation;
    self -> instructions[self -> instruction_count - 1] . arity = 0;
    self -> instructions[self -> instruction_count - 1] . arity = self -> operand_count;
}

static void __continue_instruction(MCScriptModuleBuilderRef self, uindex_t p_argument)
{
    if (!MCMemoryResizeArray(self -> operand_count + 1, self -> operands, self -> operand_count))
    {
        self -> valid = false;
        return;
    }
    
    self -> instructions[self -> instruction_count - 1] . arity += 1;
    self -> operands[self -> operand_count - 1] = p_argument;
}

static void __end_instruction(MCScriptModuleBuilderRef self)
{
    // Nothing to do!
}

static void __emit_instruction(MCScriptModuleBuilderRef self, MCScriptBytecodeOp p_operation, uindex_t p_arity, ...)
{
    __begin_instruction(self, p_operation);
    
    va_list t_args;
    va_start(t_args, p_arity);
    __continue_instruction(self, va_arg(t_args, uindex_t));
    va_end(t_args);
    
    __end_instruction(self);
}

void MCScriptBeginHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count) ||
        !MCMemoryNew((MCScriptHandlerDefinition*&)self -> module . definitions[self -> module . definition_count - 1]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindHandler;
    t_definition -> signature = MCValueRetain(p_signature);
    t_definition -> address = self -> module . bytecode_count;
    
    self -> current_handler = self -> module . imported_definition_count;
    
    r_index = self -> module . imported_definition_count;
}

void MCScriptEndHandlerInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (self -> current_handler == 0)
    {
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerDefinition *t_handler;
    t_handler = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[self -> current_handler - 1]);

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
        else if (t_op >= kMCScriptBytecodeOpJumpIfUndefined && t_op <= kMCScriptBytecodeOpJumpIfFalse)
            t_address_index = 1;
        else
            continue;
        
        index_t t_target_address;
        t_target_address = self -> instructions[self -> labels[t_operands[t_address_index]] . instruction] . address - t_address;
        
        uindex_t t_encoded_target_address;
        if (t_target_address > 0)
            t_encoded_target_address = t_target_address * 2;
        else if (t_target_address < 0)
            t_encoded_target_address = (-t_target_address) * 2 + 1;
        
        t_operands[t_address_index] = t_encoded_target_address;
    }
    
    for(uindex_t i = 0; i < self -> instruction_count; i++)
    {
        MCScriptBytecodeOp t_op;
        uindex_t t_arity;
        uindex_t *t_operands;
        
        t_op = self -> instructions[i] . operation;
        t_address = self -> instructions[i] . address;
        t_arity = self -> instructions[i] . arity;
        t_operands = &self -> operands[self -> instructions[i] . operands];
        
        __emit_bytecode_byte(self, t_op | (MCMin(t_arity, 15) << 4));
        
        if (t_arity >= 15)
            __emit_bytecode_byte(self, t_arity - 15);
        
        for(uindex_t i = 0; i < t_arity; i++)
            __emit_bytecode_uint(self, t_operands[i]);
    }
    
    self -> instruction_count = 0;
    self -> operand_count = 0;
    self -> label_count = 0;
}

void MCScriptDeferLabelForBytecodeInModule(MCScriptModuleBuilderRef self, uindex_t& r_label)
{
    if (self == nil || !self -> valid)
        return;
 
    if (!MCMemoryResizeArray(self -> label_count + 1, self -> labels, self -> label_count))
    {
        r_label = 0;
        self -> valid = false;
        return;
    }
    
    self -> labels[self -> label_count - 1] . instruction = UINDEX_MAX;
    
    r_label = self -> label_count;
}

void MCScriptResolveLabelForBytecodeInModule(MCScriptModuleBuilderRef self, uindex_t p_label)
{
    if (self == nil || !self -> valid)
        return;
 
    self -> labels[p_label - 1] . instruction = self -> instruction_count;
}

void MCScriptEmitJumpInModule(MCScriptModuleBuilderRef self, uindex_t p_target_label)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpJump, 1, p_target_label);
}

void MCScriptEmitJumpIfUndefinedInModule(MCScriptModuleBuilderRef self, uindex_t p_value_reg, uindex_t p_target_label)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpJumpIfUndefined, 2, p_value_reg, p_target_label);
}

void MCScriptEmitJumpIfDefinedInModule(MCScriptModuleBuilderRef self, uindex_t p_value_reg, uindex_t p_target_label)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpJumpIfDefined, 2, p_value_reg, p_target_label);
}

void MCScriptEmitJumpIfFalseInModule(MCScriptModuleBuilderRef self, uindex_t p_value_reg, uindex_t p_target_label)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpJumpIfFalse, 2, p_value_reg, p_target_label);
}

void MCScriptEmitJumpIfTrueInModule(MCScriptModuleBuilderRef self, uindex_t p_value_reg, uindex_t p_target_label)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpJumpIfTrue, 2, p_value_reg, p_target_label);
}

void MCScriptEmitAssignConstantInModule(MCScriptModuleBuilderRef self, uindex_t p_reg, MCValueRef p_constant)
{
    if (self == nil || !self -> valid)
        return;
    
    uindex_t t_constant_index;
    __emit_constant(self, p_constant, t_constant_index);
    
    __emit_instruction(self, kMCScriptBytecodeOpAssignConstant, 2, p_reg, t_constant_index);
}

void MCScriptEmitAssignInModule(MCScriptModuleBuilderRef self, uindex_t p_dst_reg, uindex_t p_src_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpAssign, 2, p_dst_reg, p_src_reg);
}

void MCScriptEmitTypecheckInModule(MCScriptModuleBuilderRef self, uindex_t p_reg, MCValueRef p_typeinfo)
{
    if (self == nil || !self -> valid)
        return;
    
    uindex_t t_constant_index;
    __emit_constant(self, p_typeinfo, t_constant_index);
    
    __emit_instruction(self, kMCScriptBytecodeOpTypecheck, 2, p_reg, t_constant_index);
}

void MCScriptEmitReturnInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpReturn, 0);
}

void MCScriptBeginInvokeInModule(MCScriptModuleBuilderRef self, uindex_t p_handler_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __begin_instruction(self, kMCScriptBytecodeOpInvoke);
    __continue_instruction(self, p_handler_index);
}

void MCScriptBeginIndirectInvokeInModule(MCScriptModuleBuilderRef self, uindex_t p_handler_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __begin_instruction(self, kMCScriptBytecodeOpInvokeIndirect);
    __continue_instruction(self, p_handler_reg);
}

void MCScriptContinueInvokeInModule(MCScriptModuleBuilderRef self, uindex_t p_arg_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __continue_instruction(self, p_arg_reg);
}

void MCScriptEndInvokeInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    __end_instruction(self);
}

void MCScriptEmitFetchGlobalInModule(MCScriptModuleBuilderRef self, uindex_t p_dst_reg, uindex_t p_glob_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpFetchGlobal, 2, p_dst_reg, p_glob_index);
}

void MCScriptEmitStoreGlobalInModule(MCScriptModuleBuilderRef self, uindex_t p_src_reg, uindex_t p_glob_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpFetchGlobal, 2, p_src_reg, p_glob_index);
}

////////////////////////////////////////////////////////////////////////////////
