#include "script.h"
#include "script-private.h"

#include <stddef.h>

////////////////////////////////////////////////////////////////////////////////

MC_PICKLE_BEGIN_RECORD(MCScriptExportedDefinition)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptImportedDefinition)
    MC_PICKLE_UINDEX(module)
    MC_PICKLE_INTENUM(MCScriptDefinitionKind, kind)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptExternalDefinition)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptTypeDefinition)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptConstantDefinition)
    MC_PICKLE_VALUEREF(value)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptVariableDefinition)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
    MC_PICKLE_UINDEX(start_address)
    MC_PICKLE_UINDEX(finish_address)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptForeignHandlerDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
    MC_PICKLE_STRINGREF(binding)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptPropertyDefinition)
    MC_PICKLE_UINDEX(getter)
    MC_PICKLE_UINDEX(setter)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptEventDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_VARIANT(MCScriptDefinition, kind)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindExternal, MCScriptExternalDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindType, MCScriptTypeDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindConstant, MCScriptConstantDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindVariable, MCScriptVariableDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindHandler, MCScriptHandlerDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindForeignHandler, MCScriptForeignHandlerDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindProperty, MCScriptPropertyDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindEvent, MCScriptEventDefinition)
MC_PICKLE_END_VARIANT()

MC_PICKLE_BEGIN_RECORD(MCScriptModule)
    MC_PICKLE_INTENUM(MCScriptModuleKind, module_kind)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_ARRAY_OF_NAMEREF(dependencies, dependency_count)
    MC_PICKLE_ARRAY_OF_VALUEREF(values, value_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptImportedDefinition, imported_definitions, imported_definition_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptExportedDefinition, exported_definitions, exported_definition_count)
    MC_PICKLE_ARRAY_OF_VARIANT(MCScriptDefinition, definitions, definition_count)
    MC_PICKLE_ARRAY_OF_BYTE(bytecode, bytecode_count)
MC_PICKLE_END_RECORD()

////////////////////////////////////////////////////////////////////////////////

void MCScriptDestroyModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    MCPickleRelease(kMCScriptModulePickleInfo, self);
}


bool MCScriptValidateModule(MCScriptModuleRef self)
{
    for(uindex_t i = 0; i < self -> definition_count; i++)
        if (self -> definitions[i] -> kind == kMCScriptDefinitionKindVariable)
        {
            MCScriptVariableDefinition *t_variable;
            t_variable = static_cast<MCScriptVariableDefinition *>(self -> definitions[i]);
            t_variable -> slot_index = self -> slot_count++;
        }
        else if (self -> definitions[i] -> kind == kMCScriptDefinitionKindHandler)
        {
            MCScriptHandlerDefinition *t_handler;
            t_handler = static_cast<MCScriptHandlerDefinition *>(self -> definitions[i]);
            
            // Compute the parameter count from the typeinfo.
            uindex_t t_parameter_count;
            t_parameter_count = MCHandlerTypeInfoGetParameterCount(t_handler -> signature);
            if (MCHandlerTypeInfoGetReturnType(t_handler -> signature) != kMCNullTypeInfo)
                t_parameter_count += 1;
            
            // Compute variable and parameter count from scanning the bytecode.
            uindex_t t_temporary_count;
            t_temporary_count = 0;
            
            byte_t *t_bytecode, *t_bytecode_limit;
            t_bytecode = self -> bytecode + t_handler -> start_address;
            t_bytecode_limit = self -> bytecode + t_handler -> finish_address;
            
            while(t_bytecode != t_bytecode_limit)
            {
                MCScriptBytecodeOp t_operation;
                uindex_t t_arity;
                uindex_t t_operands[256];
                if (!MCScriptBytecodeIterate(t_bytecode, t_bytecode_limit, t_operation, t_arity, t_operands))
                    break;
                
                switch(t_operation)
                {
                    case kMCScriptBytecodeOpNone:
                        // check arity == 0
                        break;
                    case kMCScriptBytecodeOpJump:
                        // check arity == 1
                        // check resolved address is within handler
                        break;
                    case kMCScriptBytecodeOpJumpIfUndefined:
                    case kMCScriptBytecodeOpJumpIfDefined:
                    case kMCScriptBytecodeOpJumpIfFalse:
                    case kMCScriptBytecodeOpJumpIfTrue:
                        // check arity == 2
                        // check resolved address is within handler
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0]);
                        break;
                    case kMCScriptBytecodeOpAssignConstant:
                        // check arity == 2
                        // check index argument is within value pool range
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0]);
                        break;
                    case kMCScriptBytecodeOpAssign:
                        // check arity == 2
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0]);
                        t_temporary_count = MCMax(t_temporary_count, t_operands[1]);
                        break;
                    case kMCScriptBytecodeOpTypecheck:
                        // check arity == 2
                        // check typeinfo argument is within value pool range
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0]);
                        break;
                    case kMCScriptBytecodeOpReturn:
                        // check arity == 0
                        break;
                    case kMCScriptBytecodeOpInvoke:
                        // check index operand is within definition range
                        // check definition[index] is handler
                        // check signature of defintion[index] conforms with invoke arity
                        for(uindex_t i = 1; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i]);
                        break;
                    case kMCScriptBytecodeOpInvokeIndirect:
                        for(uindex_t i = 0; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i]);
                        break;
                    case kMCScriptBytecodeOpFetchGlobal:
                    case kMCScriptBytecodeOpStoreGlobal:
                        // check arity is 2
                        // check glob index is in definition range
                        // check definition[index] is variable
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0]);
                        break;
                }
            }
            
            // check the last instruction is 'return'.
            
            // If we didn't reach the limit, the bytecode is malformed.
            if (t_bytecode != t_bytecode_limit)
                return false;
            
            // The total number of slots we need is params (inc result) + temps.
            t_handler -> slot_count = t_parameter_count + t_temporary_count;
        }
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef MCScriptRetainModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    return (MCScriptModuleRef)MCScriptRetainObject(self);
}

void MCScriptReleaseModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    MCScriptReleaseObject(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateModuleFromStream(MCStreamRef stream, MCScriptModuleRef& r_module)
{
    uint8_t t_header[4];
    if (!MCStreamRead(stream, t_header, 4))
        return false;
    
    if (t_header[0] != 'L' ||
        t_header[1] != 'C' ||
        t_header[2] != 0x00 ||
        t_header[3] != 0x00)
        return false;
    
    MCScriptModule *t_module;
    if (!MCScriptCreateObject(kMCScriptObjectKindModule, sizeof(MCScriptModule), (MCScriptObject*&)t_module))
        return false;
    
    if (!MCPickleRead(stream, kMCScriptModulePickleInfo, t_module) ||
        !MCScriptValidateModule(t_module))
    {
        MCScriptDestroyObject(t_module);
        return false;
    }
    
    r_module = t_module;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptWriteRawModule(MCStreamRef stream, MCScriptModule *p_module)
{
    return MCPickleWrite(stream, kMCScriptModulePickleInfo, p_module);
}

bool MCScriptReadRawModule(MCStreamRef stream, MCScriptModule *p_module)
{
    return MCPickleRead(stream, kMCScriptModulePickleInfo, p_module);
}

void MCScriptReleaseRawModule(MCScriptModule *p_module)
{
    MCPickleRelease(kMCScriptModulePickleInfo, p_module);
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptLookupPropertyDefinitionInModule(MCScriptModuleRef self, MCNameRef p_property, MCScriptPropertyDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
    {
        if (self -> definitions[self -> exported_definitions[i] . index - 1] -> kind != kMCScriptDefinitionKindProperty)
            continue;
        
        if (!MCNameIsEqualTo(p_property, self -> exported_definitions[i] . name))
            continue;
        
        r_definition = static_cast<MCScriptPropertyDefinition *>(self -> definitions[self -> exported_definitions[i] . index - 1]);
        return true;
    }
    
    return false;
}

bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef self, MCNameRef handler, MCScriptHandlerDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);

    return false;
}

bool MCScriptResolveDefinitionInModule(MCScriptModuleRef self, uindex_t index, MCScriptInstanceRef& r_instance, MCScriptDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
