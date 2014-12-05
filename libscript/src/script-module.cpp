#include <foundation.h>
#include <foundation-auto.h>

#include "script.h"
#include "script-private.h"

#include <stddef.h>
#include <dlfcn.h>

////////////////////////////////////////////////////////////////////////////////

MC_PICKLE_BEGIN_RECORD(MCScriptDefinedType)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptForeignType)
    MC_PICKLE_STRINGREF(binding)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptOptionalType)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerTypeParameter)
    MC_PICKLE_INTENUM(MCScriptHandlerTypeParameterMode, mode)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerType)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptHandlerTypeParameter, parameters, parameter_count)
    MC_PICKLE_UINDEX(return_type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptRecordTypeField)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptRecordType)
    MC_PICKLE_UINDEX(base_type)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptRecordTypeField, fields, field_count)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_VARIANT(MCScriptType, kind)
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindDefined, MCScriptDefinedType)
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindForeign, MCScriptForeignType)
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindOptional, MCScriptOptionalType)
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindHandler, MCScriptHandlerType)
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindRecord, MCScriptRecordType)
MC_PICKLE_END_VARIANT()

//////////

MC_PICKLE_BEGIN_RECORD(MCScriptDependency)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_UINDEX(version)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptExportedDefinition)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptImportedDefinition)
    MC_PICKLE_UINDEX(module)
    MC_PICKLE_INTENUM(MCScriptDefinitionKind, kind)
    MC_PICKLE_NAMEREF(name)
    // MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptSyntaxMethod)
    MC_PICKLE_UINDEX(handler)
    MC_PICKLE_ARRAY_OF_UINDEX(arguments, argument_count)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptPosition)
    MC_PICKLE_UINDEX(address)
    MC_PICKLE_UINDEX(file)
    MC_PICKLE_UINDEX(line)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptExternalDefinition)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptTypeDefinition)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptConstantDefinition)
    MC_PICKLE_VALUEREF(value)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptVariableDefinition)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerDefinition)
    MC_PICKLE_UINDEX(type)
    MC_PICKLE_ARRAY_OF_UINDEX(locals, local_count)
    MC_PICKLE_UINDEX(start_address)
    MC_PICKLE_UINDEX(finish_address)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptForeignHandlerDefinition)
    MC_PICKLE_UINDEX(type)
    MC_PICKLE_STRINGREF(binding)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptPropertyDefinition)
    MC_PICKLE_UINDEX(getter)
    MC_PICKLE_UINDEX(setter)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptEventDefinition)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptSyntaxDefinition)
    MC_PICKLE_UINDEX(variable_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptSyntaxMethod, methods, method_count)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptDefinitionGroupDefinition)
    MC_PICKLE_ARRAY_OF_UINDEX(handlers, handler_count)
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
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindSyntax, MCScriptSyntaxDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindDefinitionGroup, MCScriptDefinitionGroupDefinition)
MC_PICKLE_END_VARIANT()

MC_PICKLE_BEGIN_RECORD(MCScriptModule)
    MC_PICKLE_INTENUM(MCScriptModuleKind, module_kind)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptDependency, dependencies, dependency_count)
    MC_PICKLE_ARRAY_OF_VALUEREF(values, value_count)
    MC_PICKLE_ARRAY_OF_VARIANT(MCScriptType, types, type_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptExportedDefinition, exported_definitions, exported_definition_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptImportedDefinition, imported_definitions, imported_definition_count)
    MC_PICKLE_ARRAY_OF_VARIANT(MCScriptDefinition, definitions, definition_count)
    MC_PICKLE_ARRAY_OF_BYTE(bytecode, bytecode_count)

    MC_PICKLE_ARRAY_OF_NAMEREF(definition_names, definition_name_count)
    MC_PICKLE_ARRAY_OF_NAMEREF(source_files, source_file_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptPosition, positions, position_count)
MC_PICKLE_END_RECORD()

////////////////////////////////////////////////////////////////////////////////

static MCScriptModule *s_modules = nil;

////////////////////////////////////////////////////////////////////////////////

void MCScriptDestroyModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    // Release the bits pickle-release doesn't touch.
    for(uindex_t i = 0; i < self -> dependency_count; i++)
        if (self -> dependencies[i] . instance != nil)
            MCScriptReleaseInstance(self -> dependencies[i] . instance);
    for(uindex_t i = 0; i < self -> type_count; i++)
        if (self -> types[i] -> typeinfo != nil)
            MCValueRelease(self -> types[i] -> typeinfo);
    for(uindex_t i = 0; i < self -> definition_count; i++)
        if (self -> definitions[i] -> kind == kMCScriptDefinitionKindForeignHandler)
        {
            MCScriptForeignHandlerDefinition *t_def;
            t_def = static_cast<MCScriptForeignHandlerDefinition *>(self -> definitions[i]);
            MCMemoryDeleteArray(t_def -> function_argtypes);
            MCMemoryDelete(t_def -> function_cif);
        }
    
    // Remove ourselves from the global module list.
    if (s_modules == self)
        s_modules = self -> next_module;
    else
        for(MCScriptModule *t_module = s_modules; t_module != nil; t_module = t_module -> next_module)
            if (t_module -> next_module == self)
            {
                t_module -> next_module = self -> next_module;
                break;
            }
    
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
            
            // Compute the temporary (register) count from scanning the bytecode.
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
                    case kMCScriptBytecodeOpJump:
                        // check arity == 1
                        // check resolved address is within handler
                        break;
                    case kMCScriptBytecodeOpJumpIfFalse:
                    case kMCScriptBytecodeOpJumpIfTrue:
                        // check arity == 2
                        // check resolved address is within handler
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpAssignConstant:
                        // check arity == 2
                        // check index argument is within value pool range
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpAssign:
                        // check arity == 2
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        t_temporary_count = MCMax(t_temporary_count, t_operands[1] + 1);
                        break;
                    case kMCScriptBytecodeOpReturn:
                        // check arity == 1
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpInvoke:
                        // check index operand is within definition range
                        // check definition[index] is handler or definition group
                        // check signature of defintion[index] conforms with invoke arity
                        for(uindex_t i = 1; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
                        break;
                    case kMCScriptBytecodeOpInvokeIndirect:
                        for(uindex_t i = 0; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
                        break;
                    case kMCScriptBytecodeOpFetchLocal:
                    case kMCScriptBytecodeOpStoreLocal:
                        // check arity is 2
                        // check local index is in range
                        // check definition[index] is variable
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpFetchGlobal:
                    case kMCScriptBytecodeOpStoreGlobal:
                        // check arity is 2
                        // check glob index is in definition range
                        // check definition[index] is variable
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                }
            }
            
            // check the last instruction is 'return'.
            
            // If we didn't reach the limit, the bytecode is malformed.
            if (t_bytecode != t_bytecode_limit)
                return false;
            
            // The total number of slots we need is params (inc result) + temps.
            MCTypeInfoRef t_signature;
            t_signature = self -> types[t_handler -> type] -> typeinfo;
            t_handler -> register_offset = MCHandlerTypeInfoGetParameterCount(t_signature) + t_handler -> local_count;
            t_handler -> slot_count = t_handler -> register_offset + t_temporary_count;
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
    
    // If the unpickling fails, there's nothing we can do.
    if (!MCPickleRead(stream, kMCScriptModulePickleInfo, t_module))
    {
        MCScriptDestroyObject(t_module);
        return false;
    }
    
    // If there is already a module with the same name in memory, there's nothing we can do.
    for(MCScriptModule *t_other_module = s_modules; t_other_module != nil; t_other_module = t_other_module -> next_module)
        if (MCNameIsEqualTo(t_other_module -> name, t_module -> name))
        {
            MCScriptDestroyObject(t_module);
            return false;
        }
    
    // Link our module into the global module list.
    t_module -> next_module = s_modules;
    s_modules = t_module;
    
    // Return the shiny new module.
    r_module = t_module;
    
    return true;
}

bool MCScriptLookupModule(MCNameRef p_name, MCScriptModuleRef& r_module)
{
    for(MCScriptModule *t_module = s_modules; t_module != nil; t_module = t_module -> next_module)
        if (MCNameIsEqualTo(p_name, t_module -> name))
        {
            r_module = t_module;
            return true;
        }
    
    return false;
}

bool MCScriptIsModuleALibrary(MCScriptModuleRef self)
{
    return self -> module_kind == kMCScriptModuleKindLibrary;
}

bool MCScriptIsModuleAWidget(MCScriptModuleRef self)
{
    return self -> module_kind == kMCScriptModuleKindWidget;
}

bool MCScriptEnsureModuleIsUsable(MCScriptModuleRef self)
{
    // If the module has already been ensured as usable, we are done.
    if (self -> is_usable)
        return true;
    
    // First ensure we can resolve all its external dependencies.
    for(uindex_t i = 0; i < self -> dependency_count; i++)
    {
        MCScriptModuleRef t_module;
        if (!MCScriptLookupModule(self -> dependencies[i] . name, t_module))
            return false;
        
        // A used module must not be a widget.
        if (t_module -> module_kind == kMCScriptModuleKindWidget)
            return false;
        
        // Check all the imported definitions from the module, and compute indicies.
        for(uindex_t t_import = 0; t_import < self -> imported_definition_count; t_import++)
        {
            MCScriptImportedDefinition *t_import_def;
            t_import_def = &self -> imported_definitions[t_import];
            if (t_import_def -> module != i)
                continue;
            
            MCScriptDefinition *t_def;
            if (!MCScriptLookupDefinitionInModule(t_module, t_import_def -> name, t_def))
                return false;
            
            if (t_def -> kind != t_import_def -> kind)
            {
                if (t_import_def -> kind != kMCScriptDefinitionKindHandler ||
                    t_def -> kind != kMCScriptDefinitionKindForeignHandler)
                    return false;
            }
            
            // Check that signatures match.
            
            t_import_def -> definition = t_def;
        }
        
        // A used module must be usable.
        if (!MCScriptEnsureModuleIsUsable(t_module))
            return false;
        
        // Now create the instance we need.
        if (!MCScriptCreateInstanceOfModule(t_module, self -> dependencies[i] . instance))
            return false;
    }

    // Now build all the typeinfo's
    for(uindex_t i = 0; i < self -> type_count; i++)
    {
        MCAutoTypeInfoRef t_typeinfo;
        switch(self -> types[i] -> kind)
        {
            case kMCScriptTypeKindDefined:
            {
                MCScriptDefinedType *t_type;
                t_type = static_cast<MCScriptDefinedType *>(self -> types[i]);
                
                MCScriptDefinition *t_def;
                t_def = self -> definitions[t_type -> index];
                if (t_def -> kind == kMCScriptDefinitionKindType)
                {
                    MCNameRef t_public_name;
                    t_public_name = nil;
                    for(uindex_t j = 0; j < self -> exported_definition_count; j++)
                        if (self -> exported_definitions[j] . index == t_type -> index)
                        {
                            t_public_name = self -> exported_definitions[j] . name;
                            break;
                        }
                    
                    MCScriptTypeDefinition *t_type_def;
                    t_type_def = static_cast<MCScriptTypeDefinition *>(t_def);
                    
                    if (t_public_name != nil)
                    {
                        MCAutoStringRef t_name_string;
                        if (!MCStringFormat(&t_name_string, "%@.%@", self -> name, t_public_name))
                            return false;
                        
                        MCNewAutoNameRef t_name;
                        if (!MCNameCreate(*t_name_string, &t_name))
                            return false;
                        
                        // If the target type is an alias, named type or optional type then
                        // we just create an alias. Otherwise it must be a record, foreign,
                        // handler type - this means it must be named.
                        MCTypeInfoRef t_target_type;
                        t_target_type = self -> types[t_type_def -> type] -> typeinfo;
                        if (MCTypeInfoIsAlias(t_target_type) ||
                            MCTypeInfoIsNamed(t_target_type) ||
                            MCTypeInfoIsOptional(t_target_type))
                        {
                            if (!MCAliasTypeInfoCreate(*t_name, t_target_type, &t_typeinfo))
                                return false;
                        }
                        else
                        {
                            if (!MCNamedTypeInfoCreate(*t_name, &t_typeinfo))
                                return false;
                            
                            if (!MCNamedTypeInfoBind(*t_typeinfo, t_target_type))
                                return false;
                        }
                    }
                    else
                    {
                        if (!MCAliasTypeInfoCreate(kMCEmptyName, self -> types[t_type_def -> type] -> typeinfo, &t_typeinfo))
                            return false;
                    }
                }
                else if (t_def -> kind == kMCScriptDefinitionKindExternal)
                {
                    MCScriptExternalDefinition *t_ext_def;
                    t_ext_def = static_cast<MCScriptExternalDefinition *>(t_def);
                    
                    MCScriptImportedDefinition *t_import;
                    t_import = &self -> imported_definitions[t_ext_def -> index];
                    
                    MCScriptModuleRef t_module;
                    t_module = self -> dependencies[t_import -> module] . instance -> module;
                    t_typeinfo = t_module -> types[static_cast<MCScriptTypeDefinition *>(t_import -> definition) -> type] -> typeinfo;
                }
                else
                    return false;
            }
            break;
            case kMCScriptTypeKindOptional:
            {
                MCScriptOptionalType *t_type;
                t_type = static_cast<MCScriptOptionalType *>(self -> types[i]);
                if (!MCOptionalTypeInfoCreate(self -> types[t_type -> type] -> typeinfo, &t_typeinfo))
                    return false;
            }
            break;
            case kMCScriptTypeKindForeign:
            {
                MCScriptForeignType *t_type;
                t_type = static_cast<MCScriptForeignType *>(self -> types[i]);
                
                void *t_symbol;
                t_symbol = dlsym(RTLD_DEFAULT, MCStringGetCString(t_type -> binding));
                if (t_symbol == nil)
                {
                    MCLog("Unable to resolve foreign type '%@'", t_type -> binding);
                    return false;
                }
                
                t_typeinfo = MCValueRetain(*(MCTypeInfoRef *)t_symbol);
            }
            break;
            case kMCScriptTypeKindRecord:
            {
                MCScriptRecordType *t_type;
                t_type = static_cast<MCScriptRecordType *>(self -> types[i]);
                
                MCAutoArray<MCRecordTypeFieldInfo> t_fields;
                for(uindex_t i = 0; i < t_type -> field_count; i++)
                {
                    MCRecordTypeFieldInfo t_field;
                    t_field . name = t_type -> fields[i] . name;
                    t_field . type = self -> types[t_type -> fields[i] . type] -> typeinfo;
                    if (!t_fields . Push(t_field))
                        return false;
                }
                
                if (!MCRecordTypeInfoCreate(t_fields . Ptr(), t_type -> field_count, self -> types[t_type -> base_type] -> typeinfo, &t_typeinfo))
                    return false;
            }
            break;
            case kMCScriptTypeKindHandler:
            {
                MCScriptHandlerType *t_type;
                t_type = static_cast<MCScriptHandlerType *>(self -> types[i]);
                
                MCAutoArray<MCHandlerTypeFieldInfo> t_parameters;
                for(uindex_t i = 0; i < t_type -> parameter_count; i++)
                {
                    MCHandlerTypeFieldInfo t_parameter;
                    t_parameter . mode = (MCHandlerTypeFieldMode)t_type -> parameters[i] . mode;
                    t_parameter . type = self -> types[t_type -> parameters[i] . type] -> typeinfo;
                    if (!t_parameters . Push(t_parameter))
                        return false;
                }
                
                if (!MCHandlerTypeInfoCreate(t_parameters . Ptr(), t_type -> parameter_count, self -> types[t_type -> return_type] -> typeinfo, &t_typeinfo))
                    return false;
            }
            break;
        }
        
        self -> types[i] -> typeinfo = MCValueRetain(*t_typeinfo);
    }
    
    // First validate the module - if this fails we do nothing more.
    if (!MCScriptValidateModule(self))
        return false;
    
    // Now bind all the public types.
    
    self -> is_usable = true;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCopyDependenciesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_module_names)
{
    MCAutoProperListRef t_modules;
    if (!MCProperListCreateMutable(&t_modules))
        return false;
    
    for(uindex_t i = 0; i < self -> dependency_count; i++)
        if (!MCProperListPushElementOntoBack(*t_modules, self -> dependencies[i] . name))
            return false;
    
    if (!MCProperListCopy(*t_modules, r_module_names))
        return false;
    
    return true;
}

bool MCScriptCopyPropertiesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_property_names)
{
    MCAutoProperListRef t_props;
    if (!MCProperListCreateMutable(&t_props))
        return false;
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind == kMCScriptDefinitionKindProperty)
            if (!MCProperListPushElementOntoBack(*t_props, self -> exported_definitions[i] . name))
                return false;
    
    if (!MCProperListCopy(*t_props, r_property_names))
        return false;
    
    return true;
}

bool MCScriptQueryPropertyOfModule(MCScriptModuleRef self, MCNameRef p_property, /* get */ MCTypeInfoRef& r_getter, /* get */ MCTypeInfoRef& r_setter)
{
    MCScriptPropertyDefinition *t_def;
    
    if (!self -> is_usable)
        return false; // TODO - throw error
    
    if (!MCScriptLookupPropertyDefinitionInModule(self, p_property, t_def))
        return false; // TODO - throw error
    
    MCScriptDefinition *t_getter;
    t_getter = t_def -> getter != 0 ? self -> definitions[t_def -> getter - 1] : nil;
    
    if (t_getter != nil)
    {
        if (t_getter -> kind == kMCScriptDefinitionKindVariable)
            r_getter = self -> types[static_cast<MCScriptVariableDefinition *>(t_getter) -> type] -> typeinfo;
        else
            r_getter = MCHandlerTypeInfoGetReturnType(self -> types[static_cast<MCScriptHandlerDefinition *>(t_getter) -> type] -> typeinfo);
    }
    else
        r_getter = nil;
    
    MCScriptDefinition *t_setter;
    t_setter = t_def -> setter != 0 ? self -> definitions[t_def -> setter - 1] : nil;
    
    if (t_setter != nil)
    {
        if (t_setter -> kind == kMCScriptDefinitionKindVariable)
            r_setter = self -> types[static_cast<MCScriptVariableDefinition *>(t_setter) -> type] -> typeinfo;
        else
            r_setter = MCHandlerTypeInfoGetParameterType(self -> types[static_cast<MCScriptHandlerDefinition *>(t_setter) -> type] -> typeinfo, 0);
    }
    else
        r_setter = nil;
    
    return true;
}

bool MCScriptCopyEventsOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_event_names)
{
    MCAutoProperListRef t_events;
    if (!MCProperListCreateMutable(&t_events))
        return false;
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind == kMCScriptDefinitionKindEvent)
            if (!MCProperListPushElementOntoBack(*t_events, self -> exported_definitions[i] . name))
                return false;
    
    if (!MCProperListCopy(*t_events, r_event_names))
        return false;
    
    return true;
}

bool MCScriptQueryEventOfModule(MCScriptModuleRef self, MCNameRef p_event, /* get */ MCTypeInfoRef& r_signature)
{
    MCScriptEventDefinition *t_def;
    
    if (!self -> is_usable)
        return false; // TODO - throw error
    
    if (!MCScriptLookupEventDefinitionInModule(self, p_event, t_def))
        return false; // TODO - throw error
    
    r_signature = self -> types[t_def -> type] -> typeinfo;
    
    return true;
}

bool MCScriptCopyHandlersOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_handler_names)
{
    MCAutoProperListRef t_handlers;
    if (!MCProperListCreateMutable(&t_handlers))
        return false;
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind == kMCScriptDefinitionKindHandler ||
            self -> definitions[self -> exported_definitions[i] . index] -> kind == kMCScriptDefinitionKindForeignHandler)
            if (!MCProperListPushElementOntoBack(*t_handlers, self -> exported_definitions[i] . name))
                return false;
    
    if (!MCProperListCopy(*t_handlers, r_handler_names))
        return false;
    
    return true;
}

bool MCScriptQueryHandlerOfModule(MCScriptModuleRef self, MCNameRef p_handler, /* get */ MCTypeInfoRef& r_signature)
{
    MCScriptHandlerDefinition *t_def;
    
    if (!self -> is_usable)
        return false; // TODO - throw error
    
    if (!MCScriptLookupHandlerDefinitionInModule(self, p_handler, t_def))
        return false; // TODO - throw error
    
    r_signature = self -> types[t_def -> type] -> typeinfo;
    
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
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind != kMCScriptDefinitionKindProperty)
            continue;
        
        if (!MCNameIsEqualTo(p_property, self -> exported_definitions[i] . name))
            continue;
        
        r_definition = static_cast<MCScriptPropertyDefinition *>(self -> definitions[self -> exported_definitions[i] . index]);
        return true;
    }
    
    return false;
}

bool MCScriptLookupEventDefinitionInModule(MCScriptModuleRef self, MCNameRef p_property, MCScriptEventDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
    {
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind != kMCScriptDefinitionKindEvent)
            continue;
        
        if (!MCNameIsEqualTo(p_property, self -> exported_definitions[i] . name))
            continue;
        
        r_definition = static_cast<MCScriptEventDefinition *>(self -> definitions[self -> exported_definitions[i] . index]);
        return true;
    }
    
    return false;
}

bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef self, MCNameRef p_handler, MCScriptHandlerDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
    {
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind != kMCScriptDefinitionKindHandler)
            continue;
        
        if (!MCNameIsEqualTo(p_handler, self -> exported_definitions[i] . name))
            continue;
        
        r_definition = static_cast<MCScriptHandlerDefinition *>(self -> definitions[self -> exported_definitions[i] . index]);
        return true;
    }
    
    return false;
}

bool MCScriptLookupDefinitionInModule(MCScriptModuleRef self, MCNameRef p_name, MCScriptDefinition*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
    {
        if (!MCNameIsEqualTo(p_name, self -> exported_definitions[i] . name))
            continue;
        
        r_definition = static_cast<MCScriptHandlerDefinition *>(self -> definitions[self -> exported_definitions[i] . index]);
        return true;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
