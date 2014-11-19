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
    uindex_t current_param_count;
    uindex_t current_local_count;
    
    uindex_t *current_handler_group;
    uindex_t current_handler_group_size;
    
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
    self -> current_handler = UINDEX_MAX;
    
    self -> current_handler_group = nil;
    self -> current_handler_group_size = 0;
    
    if (self -> valid)
        self -> valid = MCProperListCreateMutable(self -> definition_names);
    
    r_builder = self;
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
    
    bool t_success;
    if (self -> valid)
    {
        byte_t t_header[4];
        t_header[0] = 'L';
        t_header[1] = 'C';
        t_header[2] = 0;
        t_header[3] = 0;
        
        t_success = MCStreamWrite(p_stream, t_header, 4) &&
                    MCScriptWriteRawModule(p_stream, &self -> module);
    }
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
    
    r_index = self -> module . dependency_count - 1;
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
    
    self -> module  . exported_definitions[self -> module . exported_definition_count - 1] . name = MCValueRetain((MCNameRef)MCProperListFetchElementAtIndex(self -> definition_names, p_definition));
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
    // t_import -> type = MCValueRetain(p_type);
    
    MCScriptExternalDefinition *t_definition;
    t_definition = static_cast<MCScriptExternalDefinition *>(self -> module . definitions[self -> module . definition_count - 1]);
    
    t_definition -> kind = kMCScriptDefinitionKindExternal;
    t_definition -> index = self -> module . imported_definition_count - 1;

    r_index = self -> module . definition_count - 1;
}

void MCScriptAddDefinitionToModule(MCScriptModuleBuilderRef self, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(self -> module . definition_count + 1, self -> module . definitions, self -> module . definition_count))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }
    
    r_index = self -> module . definition_count - 1;
}

void MCScriptAddTypeToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptTypeDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
        
    MCScriptTypeDefinition *t_definition;
    t_definition = static_cast<MCScriptTypeDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindType;
    t_definition -> type = MCValueRetain(p_type);
}

void MCScriptAddConstantToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCValueRef p_value, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptConstantDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptConstantDefinition *t_definition;
    t_definition = static_cast<MCScriptConstantDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindConstant;
    t_definition -> value = MCValueRetain(p_value);
}

void MCScriptAddVariableToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptVariableDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptVariableDefinition *t_definition;
    t_definition = static_cast<MCScriptVariableDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindVariable;
    t_definition -> type = MCValueRetain(p_type);
}

void MCScriptAddForeignHandlerToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, MCStringRef p_binding, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptForeignHandlerDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptForeignHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptForeignHandlerDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindForeignHandler;
    t_definition -> signature = MCValueRetain(p_signature);
    t_definition -> binding = MCValueRetain(p_binding);
}

void MCScriptAddPropertyToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, uindex_t p_getter, uindex_t p_setter, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptPropertyDefinition *&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptPropertyDefinition *t_definition;
    t_definition = static_cast<MCScriptPropertyDefinition *>(self -> module . definitions[p_index]);
    
    // The property getters and setter references are stored with +1 indices, 0 meaning
    // no getter/setter.
    t_definition -> kind = kMCScriptDefinitionKindProperty;
    t_definition -> getter = p_getter + 1;
    t_definition -> setter = p_setter == UINDEX_MAX ? 0 : p_setter + 1;
}

void MCScriptAddEventToModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptEventDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptEventDefinition *t_definition;
    t_definition = static_cast<MCScriptEventDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindEvent;
    t_definition -> signature = MCValueRetain(p_signature);
}

///////////

void MCScriptBeginHandlerGroupInModule(MCScriptModuleBuilderRef self)
{
    if (self == nil || !self -> valid)
        return;
    
    if (!MCMemoryResizeArray(0, self -> current_handler_group, self -> current_handler_group_size))
    {
        self -> valid = false;
        return;
    }
}

void MCScriptContinueHandlerGroupInModule(MCScriptModuleBuilderRef self, uindex_t index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> current_handler_group_size; i++)
        if (self -> current_handler_group[i] == index)
            return;
    
    if (!MCMemoryResizeArray(self -> current_handler_group_size + 1, self -> current_handler_group, self -> current_handler_group_size))
    {
        self -> valid = false;
        return;
    }
    
    self -> current_handler_group[self -> current_handler_group_size - 1] = index;
}

void MCScriptEndHandlerGroupInModule(MCScriptModuleBuilderRef self, uindex_t& r_index)
{
    if (self == nil || !self -> valid)
        return;
    
    for(uindex_t i = 0; i < self -> module . definition_count; i++)
    {
        if (self -> module . definitions[i] -> kind != kMCScriptDefinitionKindHandlerGroup)
            continue;
        
        MCScriptHandlerGroupDefinition *t_group;
        t_group = (MCScriptHandlerGroupDefinition *)self -> module . definitions[i];
        if (self -> current_handler_group_size != t_group -> handler_count)
            continue;
        
        if (MCMemoryCompare(t_group -> handlers, self -> current_handler_group, sizeof(uindex_t) * self -> current_handler_group_size) != 0)
            continue;
        
        r_index = i;
        return;
    }
    
    uindex_t t_index;
    MCScriptAddDefinitionToModule(self, t_index);
    if (!self -> valid)
        return;
    
    if (!MCMemoryNew((MCScriptHandlerGroupDefinition *&)self -> module . definitions[t_index]))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerGroupDefinition *t_group;
    t_group = (MCScriptHandlerGroupDefinition *)self -> module . definitions[t_index];
    
    t_group -> handler_count = self -> current_handler_group_size;
    t_group -> handlers = self -> current_handler_group;
    
    self -> current_handler_group_size = 0;
    self -> current_handler_group = nil;
}

///////////

static uindex_t __measure_uint(uindex_t p_value)
{
    return 2;
    
    /*if (p_value < (1 << 7))
        return 1;
    if (p_value < (1 << 14))
        return 2;
    if (p_value < (1 << 21))
        return 3;
    if (p_value < (1 << 28))
        return 4;
    return 5;*/
}

static uindex_t __measure_instruction(MCScriptModuleBuilderRef self, MCScriptBytecodeInstruction *p_instruction)
{
    uindex_t t_size;
    t_size = 1;
    
    if (p_instruction -> arity >= 15)
        t_size += 1;
    
    /*if (p_instruction -> operation == kMCScriptBytecodeOpJump)
        t_size += 3;
    else if (p_instruction -> operation >= kMCScriptBytecodeOpJumpIfUndefined && p_instruction -> operation <= kMCScriptBytecodeOpJumpIfFalse)
    {
        t_size += __measure_uint(self -> operands[p_instruction -> operands]);
        t_size += 3;
    }
    else
    {*/
        for(uindex_t i = 0; i < p_instruction -> arity; i++)
            t_size += __measure_uint(self -> operands[p_instruction -> operands + i]);
    /*}*/
    
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
    
    /*uint8_t t_bytes[5];
    uindex_t t_index;
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
    while(p_value != 0);*/
    
    uint8_t t_bytes[2];
    uindex_t t_index;
    t_bytes[0] = ((p_value >> 7) & 0x7f) | 0x80;
    t_bytes[1] = (p_value & 0x7f);
    t_index = 2;
    
    if ((p_value >> 14) != 0)
        MCAssert(false);
    
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
    self -> instructions[self -> instruction_count - 1] . operands = self -> operand_count;
}

static void __continue_instruction(MCScriptModuleBuilderRef self, uindex_t p_argument)
{
    if (!MCMemoryResizeArray(self -> operand_count + 1, self -> operands, self -> operand_count))
    {
        self -> valid = false;
        return;
    }
    
    if (self -> instructions[self -> instruction_count - 1] . arity == 256)
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
    for(uindex_t i = 0; i < p_arity; i++)
        __continue_instruction(self, va_arg(t_args, uindex_t));
    va_end(t_args);
    
    __end_instruction(self);
}

void MCScriptBeginHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_signature, uindex_t p_index)
{
    if (self == nil || !self -> valid)
        return;
    
    if (p_index >= self -> module . definition_count ||
        self -> module . definitions[p_index] != nil ||
        !MCMemoryNew((MCScriptHandlerDefinition*&)self -> module . definitions[p_index]) ||
        !MCProperListPushElementOntoBack(self -> definition_names, p_name))
    {
        self -> valid = false;
        return;
    }
    
    MCScriptHandlerDefinition *t_definition;
    t_definition = static_cast<MCScriptHandlerDefinition *>(self -> module . definitions[p_index]);
    
    t_definition -> kind = kMCScriptDefinitionKindHandler;
    t_definition -> signature = MCValueRetain(p_signature);
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
    
    t_handler -> finish_address = self -> module . bytecode_count;
    
    self -> instruction_count = 0;
    self -> operand_count = 0;
    self -> label_count = 0;
    self -> current_handler = UINDEX_MAX;
}

void MCScriptAddParameterToHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t& r_index)
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

void MCScriptAddVariableToHandlerInModule(MCScriptModuleBuilderRef self, MCNameRef p_name, MCTypeInfoRef p_type, uindex_t& r_index)
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
    
    if (!MCMemoryResizeArray(t_handler -> local_count + 1, t_handler -> locals, t_handler -> local_count))
    {
        r_index = 0;
        self -> valid = false;
        return;
    }

    t_handler -> locals[t_handler -> local_count - 1] = MCValueRetain(p_type);
    
    r_index = self -> current_param_count + t_handler -> local_count - 1;
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
    
    __emit_instruction(self, kMCScriptBytecodeOpAssignConstant, 2, p_reg, t_constant_index - 1);
}

void MCScriptEmitAssignInModule(MCScriptModuleBuilderRef self, uindex_t p_dst_reg, uindex_t p_src_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpAssign, 2, p_dst_reg, p_src_reg);
}

void MCScriptEmitReturnInModule(MCScriptModuleBuilderRef self, uindex_t p_src_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpReturn, 1, p_src_reg);
}

void MCScriptBeginInvokeInModule(MCScriptModuleBuilderRef self, uindex_t p_handler_index, uindex_t p_result_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __begin_instruction(self, kMCScriptBytecodeOpInvoke);
    __continue_instruction(self, p_handler_index);
    __continue_instruction(self, p_result_reg);
}

void MCScriptBeginIndirectInvokeInModule(MCScriptModuleBuilderRef self, uindex_t p_handler_reg, uindex_t p_result_reg)
{
    if (self == nil || !self -> valid)
        return;
    
    __begin_instruction(self, kMCScriptBytecodeOpInvokeIndirect);
    __continue_instruction(self, p_handler_reg);
    __continue_instruction(self, p_result_reg);
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

void MCScriptEmitFetchLocalInModule(MCScriptModuleBuilderRef self, uindex_t p_dst_reg, uindex_t p_local_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpFetchLocal, 2, p_dst_reg, p_local_index);
}

void MCScriptEmitStoreLocalInModule(MCScriptModuleBuilderRef self, uindex_t p_src_reg, uindex_t p_local_index)
{
    if (self == nil || !self -> valid)
        return;
    
    __emit_instruction(self, kMCScriptBytecodeOpStoreLocal, 2, p_src_reg, p_local_index);
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
    
    __emit_instruction(self, kMCScriptBytecodeOpStoreGlobal, 2, p_src_reg, p_glob_index);
}

////////////////////////////////////////////////////////////////////////////////
