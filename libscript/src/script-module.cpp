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

#include "libscript/script.h"
#include <libscript/script-auto.h>
#include "script-private.h"

#include <stddef.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

////////////////////////////////////////////////////////////////////////////////

/* Using offsetof() on a non-POD type (e.g. structs with inheritance
 * and classes) is, strictly-speaking, undefined according to the C++
 * standard (because C++'s offsetof is only present for C
 * compatibility and is only defined for operations on C types).
 * Disable GCC's offsetof warnings for these macros */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

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
    MC_PICKLE_ARRAY_OF_NAMEREF(parameter_names, parameter_name_count)
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
    MC_PICKLE_VARIANT_CASE(kMCScriptTypeKindForeignHandler, MCScriptHandlerType)
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
    MC_PICKLE_UINDEX(value)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptVariableDefinition)
    MC_PICKLE_UINDEX(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptContextVariableDefinition)
    MC_PICKLE_UINDEX(type)
    MC_PICKLE_UINDEX(default_value)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerDefinition)
    MC_PICKLE_UINDEX(type)
    MC_PICKLE_ARRAY_OF_UINDEX(local_types, local_type_count)
    MC_PICKLE_ARRAY_OF_NAMEREF(local_names, local_name_count)
    MC_PICKLE_UINDEX(start_address)
    MC_PICKLE_UINDEX(finish_address)
    MC_PICKLE_INTENUM(MCScriptHandlerScope, scope)
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
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindContextVariable, MCScriptContextVariableDefinition)
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

/* Re-enable invalid-offsetof warnings */
#pragma GCC diagnostic pop

////////////////////////////////////////////////////////////////////////////////

static MCScriptModule *s_modules = nil;
static MCScriptModule **s_context_slot_owners = nil;
static uindex_t s_context_slot_count;

////////////////////////////////////////////////////////////////////////////////

static MCStringRef __MCScriptDefinitionKindToString(MCScriptDefinitionKind p_kind)
{
    switch(p_kind)
    {
        case kMCScriptDefinitionKindNone: return MCSTR("unknown");
        case kMCScriptDefinitionKindExternal: return MCSTR("external");
        case kMCScriptDefinitionKindType: return MCSTR("type");
        case kMCScriptDefinitionKindConstant: return MCSTR("constant");
        case kMCScriptDefinitionKindVariable: return MCSTR("variable");
        case kMCScriptDefinitionKindHandler: return MCSTR("handler");
        case kMCScriptDefinitionKindForeignHandler: return MCSTR("handler");
        case kMCScriptDefinitionKindProperty: return MCSTR("property");
        case kMCScriptDefinitionKindEvent: return MCSTR("event");
        case kMCScriptDefinitionKindSyntax: return MCSTR("syntax");
        case kMCScriptDefinitionKindDefinitionGroup: return MCSTR("group");
        case kMCScriptDefinitionKindContextVariable: return MCSTR("context variable");
        default:
            return MCSTR("unknown");
    }
    
    return kMCEmptyString;
}

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
        }
    
    // Remove ourselves from the context slot owners list.
    for(uindex_t i = 0; i < s_context_slot_count; i++)
        if (s_context_slot_owners[i] == self)
            s_context_slot_owners[i] = nil;
    
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
        else if (self -> definitions[i] -> kind == kMCScriptDefinitionKindContextVariable)
        {
            MCScriptContextVariableDefinition *t_variable;
            t_variable = static_cast<MCScriptContextVariableDefinition *>(self -> definitions[i]);
            
            uindex_t t_index;
            t_index = UINDEX_MAX;
            for(uindex_t i = 0; i < s_context_slot_count; i++)
                if (s_context_slot_owners[i] == nil)
                {
                    t_index = i;
                    break;
                }

            if (t_index == UINDEX_MAX)
            {
                if (!MCMemoryResizeArray(s_context_slot_count + 1, s_context_slot_owners, s_context_slot_count))
                    return false; // oom
                
                t_index = s_context_slot_count - 1;
            }
            
            s_context_slot_owners[t_index] = self;
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
                        // jump <offset>
                        if (t_arity != 1)
                            goto invalid_bytecode_error;
                        
                        // check resolved address is within handler
                        break;
                    case kMCScriptBytecodeOpJumpIfFalse:
                    case kMCScriptBytecodeOpJumpIfTrue:
                        // jumpiftrue <register>, <offset>
                        // jumpiffalse <register>, <offset>
                        if (t_arity != 2)
                            goto invalid_bytecode_error;
                        
                        // check resolved address is within handler
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpAssignConstant:
                        // assignconst <dst>, <index>
                        if (t_arity != 2)
                            goto invalid_bytecode_error;
                        
                        // check index argument is within value pool range
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpAssign:
                        // assign <dst>, <src>
                        if (t_arity != 2)
                            goto invalid_bytecode_error;
                        
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        t_temporary_count = MCMax(t_temporary_count, t_operands[1] + 1);
                        break;
                    case kMCScriptBytecodeOpReturn:
                        // return
                        // return <value>
                        if (t_arity != 0 && t_arity != 1)
                            goto invalid_bytecode_error;
                        
                        if (t_arity == 1)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpInvoke:
                        // invoke <index>, <result>, [ <arg_1>, ..., <arg_n> ]
                        if (t_arity < 2)
                            goto invalid_bytecode_error;
                        
                        // check index operand is within definition range
                        // check definition[index] is handler or definition group
                        // check signature of defintion[index] conforms with invoke arity
                        for(uindex_t i = 1; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
                        break;
                    case kMCScriptBytecodeOpInvokeIndirect:
                        // invoke *<src>, <result>, [ <arg_1>, ..., <arg_n> ]
                        if (t_arity < 2)
                            goto invalid_bytecode_error;
                        
                        for(uindex_t i = 0; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
                        break;
                    case kMCScriptBytecodeOpFetch:
                    case kMCScriptBytecodeOpStore:
                        // fetch <dst>, <index>
                        // store <src>, <index>
                        if (t_arity != 2)
                            goto invalid_bytecode_error;
                        
                        // check definition[index] is variable or handler
                        // check level is appropriate.
                        t_temporary_count = MCMax(t_temporary_count, t_operands[0] + 1);
                        break;
                    case kMCScriptBytecodeOpAssignList:
                        // assignlist <dst>, [ <elem_1>, ..., <elem_n> ]
                        if (t_arity < 1)
                            goto invalid_bytecode_error;
                        
                        for(uindex_t i = 0; i < t_arity; i++)
                            t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
                        break;
                    case kMCScriptBytecodeOpAssignArray:
	                    // assignarray <dst>, [ <key_1>, <value_1>, ..., <key_n>, <value_n> ]
	                    if (t_arity < 1)
		                    goto invalid_bytecode_error;
	                    if (t_arity % 2 != 1) // parameters must come in pairs
		                    goto invalid_bytecode_error;

	                    for (uindex_t i = 0; i < t_arity; ++i)
	                    {
		                    t_temporary_count = MCMax(t_temporary_count, t_operands[i] + 1);
	                    }
	                    break;
                }
            }
            
            // check the last instruction is 'return'.
            
            // If we didn't reach the limit, the bytecode is malformed.
            if (t_bytecode != t_bytecode_limit)
                goto invalid_bytecode_error;
            
            // The total number of slots we need is params (inc result) + temps.
            MCTypeInfoRef t_signature;
            t_signature = self -> types[t_handler -> type] -> typeinfo;
            t_handler -> slot_count = MCHandlerTypeInfoGetParameterCount(t_signature) + t_handler -> local_type_count + t_temporary_count;
        }
    
    // If the module has context
    
    return true;
    
invalid_bytecode_error:
    return MCErrorThrowGenericWithMessage(MCSTR("%{name} is not valid - malformed bytecode"),
                                          "name", self -> name,
                                          nil);
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef MCScriptRetainModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    return (MCScriptModuleRef)MCScriptRetainObject(self);
}

void MCScriptReleaseModule(MCScriptModuleRef self)
{
	if (nil == self)
		return;

    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    MCScriptReleaseObject(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateModuleFromStream(MCStreamRef stream, MCScriptModuleRef& r_module)
{
    uint8_t t_header[4];
    if (!MCStreamRead(stream, t_header, 4))
        return false;
    
    // If this fails, then it definitely isn't a LiveCode module.
    if (t_header[0] != 'L' ||
        t_header[1] != 'C')
        return MCErrorThrowGeneric(MCSTR("not a module"));
    
    // If this fails, then it is a potentially a LiveCode module, but in a format
    // we do not support.
    if (t_header[2] != ((kMCScriptCurrentModuleVersion >> 0) & 0xFF) ||
        t_header[3] != ((kMCScriptCurrentModuleVersion >> 8) & 0xFF))
        return MCErrorThrowGeneric(MCSTR("module format not supported"));
    
    // If this fails then we've run out of memory (oh well - not much to be done!).
    MCScriptModule *t_module;
    if (!MCScriptCreateObject(kMCScriptObjectKindModule, sizeof(MCScriptModule), (MCScriptObject*&)t_module))
        return false;
    
    // If the unpickling fails, there's nothing we can do.
    if (!MCPickleRead(stream, kMCScriptModulePickleInfo, t_module))
    {
        MCScriptDestroyObject(t_module);
        
        // If there is a pending error then its out of memory, otherwise report
        // that we haven't been able to unpickle.
        if (!MCErrorIsPending())
            return MCErrorThrowGeneric(MCSTR("error reading module"));
        
        return false;
    }
    
    // If there is already a module with the same name in memory, there's nothing we can do.
    for(MCScriptModule *t_other_module = s_modules; t_other_module != nil; t_other_module = t_other_module -> next_module)
        if (MCNameIsEqualTo(t_other_module -> name, t_module -> name))
        {
            MCScriptDestroyObject(t_module);
            return MCErrorThrowGeneric(MCSTR("module with that name already loaded"));
        }
    
    // Link our module into the global module list.
    t_module -> next_module = s_modules;
    s_modules = t_module;
    
    // Return the shiny new module.
    r_module = t_module;
    
    return true;
}

MC_DLLEXPORT_DEF bool
MCScriptCreateModuleFromData (MCDataRef data,
                              MCScriptModuleRef & r_module)
{
	MCAutoValueRefBase<MCStreamRef> t_stream;
	MCAutoScriptModuleRef t_module;

	if (!MCMemoryInputStreamCreate (MCDataGetBytePtr (data),
	                                MCDataGetLength (data),
	                                &t_stream))
		return false;

	if (!MCScriptCreateModuleFromStream (*t_stream, &t_module))
		return false;

	r_module = MCScriptRetainModule (*t_module);
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

MCNameRef MCScriptGetNameOfModule(MCScriptModuleRef self)
{
    return self -> name;
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

	/* If the module is already marked as being checked for usability,
	 * then there must be a cyclic module dependency. */
	if (self -> is_in_usable_check)
		return MCErrorThrowGeneric(MCSTR("cyclic module dependency"));
    
	self -> is_in_usable_check = true;
    
    // First ensure we can resolve all its external dependencies.
    for(uindex_t i = 0; i < self -> dependency_count; i++)
    {
        MCScriptModuleRef t_module;
        if (!MCScriptLookupModule(self -> dependencies[i] . name, t_module))
        {
            MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - dependent module not found %{dependency}"),
                                           "name", self -> name,
                                           "dependency", self -> dependencies[i] . name,
                                           nil);
			goto error_cleanup;
        }
        
        // A used module must not be a widget.
        if (t_module -> module_kind == kMCScriptModuleKindWidget)
        {
            MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - depends on widget module %{dependency}"),
                                           "name", self -> name,
                                           "dependency", self -> dependencies[i] . name,
                                           nil);
			goto error_cleanup;
        }
        
        // A used module must be usable - do this before resolving imports so
        // chained imports work.
        if (!MCScriptEnsureModuleIsUsable(t_module))
			goto error_cleanup;
        
        // Check all the imported definitions from the module, and compute indicies.
        for(uindex_t t_import = 0; t_import < self -> imported_definition_count; t_import++)
        {
            MCScriptImportedDefinition *t_import_def;
            t_import_def = &self -> imported_definitions[t_import];
            if (t_import_def -> module != i)
                continue;
            
            MCScriptDefinition *t_def;
            if (!MCScriptLookupDefinitionInModule(t_module, t_import_def -> name, t_def))
            {
                MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - %{dependency}.%{definition} not found"),
                                               "name", self -> name,
                                               "dependency", self -> dependencies[i] . name,
                                               "definition", t_import_def -> name,
                                               nil);
				goto error_cleanup;
            }
            
            MCScriptModuleRef t_mod;
            if (t_def -> kind == kMCScriptDefinitionKindExternal)
            {
                MCScriptExternalDefinition *t_ext_def;
                t_ext_def = static_cast<MCScriptExternalDefinition *>(t_def);
                t_mod = t_module -> imported_definitions[t_ext_def -> index] . resolved_module;
                t_def = t_module -> imported_definitions[t_ext_def -> index] . resolved_definition;
            }
            else
                t_mod = t_module;
            
            if (t_def -> kind != t_import_def -> kind)
            {
                if (t_import_def -> kind != kMCScriptDefinitionKindHandler ||
                    t_def -> kind != kMCScriptDefinitionKindForeignHandler)
                {
                    MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - %{dependency}.%{definition} expected to be %{expected}, but is %{actual}"),
                                                         "name", self -> name,
                                                         "dependency", self -> dependencies[i] . name,
                                                         "definition", t_import_def -> name,
                                                         "expected", __MCScriptDefinitionKindToString(t_import_def -> kind),
                                                         "actual", __MCScriptDefinitionKindToString(t_def -> kind),
                                                         nil);
                    goto error_cleanup;
                }
            }
            
            // Check that signatures match.
            
            t_import_def -> resolved_definition = t_def;
            t_import_def -> resolved_module = t_mod;
        }
        
        // Now create the instance we need - if this fails its due to out of memory.
        if (!MCScriptCreateInstanceOfModule(t_module, self -> dependencies[i] . instance))
			goto error_cleanup;
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
                            goto error_cleanup; // oom
                        
                        MCNewAutoNameRef t_name;
                        if (!MCNameCreate(*t_name_string, &t_name))
								goto error_cleanup; // oom
                        
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
								goto error_cleanup; // oom
                        }
                        else
                        {
                            if (!MCNamedTypeInfoCreate(*t_name, &t_typeinfo))
								goto error_cleanup; // oom
                            
                            if (!MCNamedTypeInfoBind(*t_typeinfo, t_target_type))
								goto error_cleanup; // oom
                        }
                    }
                    else
                    {
                        if (!MCAliasTypeInfoCreate(kMCEmptyName, self -> types[t_type_def -> type] -> typeinfo, &t_typeinfo))
							goto error_cleanup; // oom
                    }
                }
                else if (t_def -> kind == kMCScriptDefinitionKindExternal)
                {
                    MCScriptExternalDefinition *t_ext_def;
                    t_ext_def = static_cast<MCScriptExternalDefinition *>(t_def);
                    
                    MCScriptImportedDefinition *t_import;
                    t_import = &self -> imported_definitions[t_ext_def -> index];
                    
                    MCScriptModuleRef t_module;
                    t_module = t_import -> resolved_module;
                    t_typeinfo = t_module -> types[static_cast<MCScriptTypeDefinition *>(t_import -> resolved_definition) -> type] -> typeinfo;
                }
                else
					goto error_cleanup;
            }
            break;
            case kMCScriptTypeKindOptional:
            {
                MCScriptOptionalType *t_type;
                t_type = static_cast<MCScriptOptionalType *>(self -> types[i]);
                if (!MCOptionalTypeInfoCreate(self -> types[t_type -> type] -> typeinfo, &t_typeinfo))
					goto error_cleanup; // oom
            }
            break;
            case kMCScriptTypeKindForeign:
            {
                MCScriptForeignType *t_type;
                t_type = static_cast<MCScriptForeignType *>(self -> types[i]);
                
                void *t_symbol;
#ifdef _WIN32
				t_symbol = GetProcAddress(GetModuleHandle(NULL), MCStringGetCString(t_type -> binding));
#elif defined(__EMSCRIPTEN__)
				void *t_handle = dlopen(NULL, RTLD_LAZY);
				t_symbol = dlsym(t_handle, MCStringGetCString(t_type->binding));
				dlclose(t_handle);
#else
                t_symbol = dlsym(RTLD_DEFAULT, MCStringGetCString(t_type -> binding));
#endif
                if (t_symbol == nil)
                {
                    MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - unable to resolve foreign type '%{type}'"),
                                                   "type", t_type -> binding,
                                                   nil);
					goto error_cleanup;
                }

                /* The symbol is a function that returns a type info reference. */
                MCTypeInfoRef (*t_type_func)(void) = (MCTypeInfoRef (*)(void)) t_symbol;
                t_typeinfo = MCValueRetain(t_type_func());
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
						goto error_cleanup; // oom
                }
                
                if (!MCRecordTypeInfoCreate(t_fields . Ptr(), t_type -> field_count, self -> types[t_type -> base_type] -> typeinfo, &t_typeinfo))
					goto error_cleanup; // oom
            }
            break;
            case kMCScriptTypeKindHandler:
            case kMCScriptTypeKindForeignHandler:
            {
                MCScriptHandlerType *t_type;
                t_type = static_cast<MCScriptHandlerType *>(self -> types[i]);
                
                MCAutoArray<MCHandlerTypeFieldInfo> t_parameters;
                for(uindex_t j = 0; j < t_type -> parameter_count; j++)
                {
                    MCHandlerTypeFieldInfo t_parameter;
                    t_parameter . mode = (MCHandlerTypeFieldMode)t_type -> parameters[j] . mode;
                    t_parameter . type = self -> types[t_type -> parameters[j] . type] -> typeinfo;
                    if (!t_parameters . Push(t_parameter))
						goto error_cleanup;
                }
                
                if (t_type -> kind == kMCScriptTypeKindHandler)
                {
                    if (!MCHandlerTypeInfoCreate(t_parameters . Ptr(), t_type -> parameter_count, self -> types[t_type -> return_type] -> typeinfo, &t_typeinfo))
                        goto error_cleanup; // oom
                }
                else
                {
                    if (!MCForeignHandlerTypeInfoCreate(t_parameters . Ptr(), t_type -> parameter_count, self -> types[t_type -> return_type] -> typeinfo, &t_typeinfo))
                        goto error_cleanup; // oom
                }
            }
            break;
        }
        
        self -> types[i] -> typeinfo = MCValueRetain(*t_typeinfo);
    }
    
    // First validate the module - if this fails we do nothing more.
    if (!MCScriptValidateModule(self))
		goto error_cleanup;
    
    // Now bind all the public types.
    
    self -> is_usable = true;
	self -> is_in_usable_check = false;
    
    return true;

 error_cleanup:
	self -> is_in_usable_check = false;
	return false;
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
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind == kMCScriptDefinitionKindHandler)
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
        if (self -> definitions[self -> exported_definitions[i] . index] -> kind != kMCScriptDefinitionKindHandler &&
            self -> definitions[self -> exported_definitions[i] . index] -> kind != kMCScriptDefinitionKindForeignHandler)
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

static bool __index_of_definition(MCScriptModuleRef self, MCScriptDefinition *p_definition, uindex_t& r_index)
{
    for(uindex_t i = 0; i < self -> definition_count; i++)
        if (self -> definitions[i] == p_definition)
        {
            r_index = i;
            return true;
        }
    return false;
}

MCNameRef MCScriptGetNameOfDefinitionInModule(MCScriptModuleRef self, MCScriptDefinition *p_definition)
{
    uindex_t t_index;
    if (__index_of_definition(self, p_definition, t_index) &&
        self -> definition_name_count > 0)
        return self -> definition_names[t_index];
    return kMCEmptyName;
}

MCNameRef MCScriptGetNameOfParameterInModule(MCScriptModuleRef self, MCScriptDefinition *p_definition, uindex_t p_index)
{
    MCScriptHandlerDefinition *t_handler;
    t_handler = static_cast<MCScriptHandlerDefinition *>(p_definition);
    
    MCScriptHandlerType *t_type;
    t_type = static_cast<MCScriptHandlerType *>(self -> types[t_handler -> type]);
    
    if (t_type -> parameter_name_count != 0)
        return t_type -> parameter_names[p_index];
    
    return kMCEmptyName;
}

MCNameRef MCScriptGetNameOfLocalVariableInModule(MCScriptModuleRef self, MCScriptDefinition *p_definition, uindex_t p_index)
{
    MCScriptHandlerDefinition *t_handler;
    t_handler = static_cast<MCScriptHandlerDefinition *>(p_definition);
    
    MCScriptHandlerType *t_type;
    t_type = static_cast<MCScriptHandlerType *>(self -> types[t_handler -> type]);
    
    if (p_index < t_type -> parameter_name_count)
        return t_type -> parameter_names[p_index];
    
    return t_handler -> local_names[p_index - t_type -> parameter_name_count];
}

MCNameRef MCScriptGetNameOfGlobalVariableInModule(MCScriptModuleRef self, uindex_t p_index)
{
    if (self -> definition_name_count > 0)
        return self -> definition_names[p_index];
    return kMCEmptyName;
}

MCNameRef MCScriptGetNameOfContextVariableInModule(MCScriptModuleRef self, uindex_t p_index)
{
    if (self -> definition_name_count > 0)
        return self -> definition_names[p_index];
    return kMCEmptyName;
}

////////////////////////////////////////////////////////////////////////////////

static int s_current_indent = 0;

static void __write(MCStreamRef stream, int p_indent, bool p_newline, const char *p_format, va_list p_args)
{
    MCAutoStringRef t_formatted_string;
    MCStringFormatV(&t_formatted_string, p_format, p_args);
    MCAutoStringRef t_string;
    MCStringFormat(&t_string, "%.*s%@%s", p_indent * 2, "          ", *t_formatted_string, p_newline ? "\n" : "");
    MCAutoStringRefAsUTF8String t_utf8_string;
    t_utf8_string . Lock(*t_string);
    MCStreamWrite(stream, *t_utf8_string, t_utf8_string . Size());
}

static void __enterln(MCStreamRef stream, const char *format, ...)
{
    va_list t_args;
    va_start(t_args, format);
    __write(stream, s_current_indent, true, format, t_args);
    va_end(t_args);
    s_current_indent += 1;
}

static void __leaveln(MCStreamRef stream, const char *format, ...)
{
    s_current_indent -= 1;
    va_list t_args;
    va_start(t_args, format);
    __write(stream, s_current_indent, true, format, t_args);
    va_end(t_args);
}

static void __writeln(MCStreamRef stream, const char *format, ...)
{
    va_list t_args;
    va_start(t_args, format);
    __write(stream, s_current_indent, true, format, t_args);
    va_end(t_args);
}

static void def_to_name(MCScriptModuleRef self, uindex_t p_index, MCStringRef& r_string)
{
    MCScriptDefinition *t_def;
    t_def = self -> definitions[p_index];
    
    MCNameRef t_name;
    if (t_def -> kind == kMCScriptDefinitionKindExternal)
    {
        MCScriptExternalDefinition *t_ext_def;
        t_ext_def = static_cast<MCScriptExternalDefinition *>(t_def);
        
        t_name = self -> imported_definitions[t_ext_def -> index] . name;
    }
    else
        t_name = self -> definition_names[p_index];
    
    if (!MCNameIsEqualTo(t_name, MCNAME("undefined")))
        MCStringFormat(r_string, "%@", t_name);
    else
        MCStringFormat(r_string, "nothing");
}

static void type_to_string(MCScriptModuleRef self, uindex_t p_type, MCStringRef& r_string)
{
    MCScriptType *t_type;
    t_type = self -> types[p_type];
    switch(t_type -> kind)
    {
        case kMCScriptTypeKindDefined:
            def_to_name(self, static_cast<MCScriptDefinedType *>(t_type) -> index, r_string);
            break;
        case kMCScriptTypeKindOptional:
        {
            MCAutoStringRef t_target_name;
            type_to_string(self, static_cast<MCScriptOptionalType *>(t_type) -> type, &t_target_name);
            MCStringFormat(r_string, "optional %@", *t_target_name);
        }
        break;
        case kMCScriptTypeKindForeignHandler:
        case kMCScriptTypeKindHandler:
        {
            MCAutoStringRef t_sig;
            MCScriptHandlerType *t_htype;
            t_htype = static_cast<MCScriptHandlerType *>(t_type);
            MCStringCreateMutable(0, &t_sig);
            MCStringAppendChar(*t_sig, '(');
            for(uindex_t i = 0; i < t_htype -> parameter_count; i++)
            {
                if (i != 0)
                    MCStringAppendNativeChars(*t_sig, (const char_t *)", ", 2);
                static const char *s_mode_strings[] = {"in", "out", "inout"};
                MCAutoStringRef t_ptype;
                type_to_string(self, t_htype -> parameters[i] . type, &t_ptype);
                MCStringAppendFormat(*t_sig, "%s %@ as %@", s_mode_strings[t_htype -> parameters[i] . mode], t_htype -> parameter_names[i], *t_ptype);
            }
            MCAutoStringRef t_rtype;
            type_to_string(self, t_htype -> return_type, &t_rtype);
            if (MCStringIsEqualToCString(*t_rtype, "undefined", kMCStringOptionCompareCaseless))
                MCStringAppendFormat(*t_sig, ") returns nothing");
            else
                MCStringAppendFormat(*t_sig, ") returns %@", *t_rtype);
            MCStringCopy(*t_sig, r_string);
        }
        break;
        default:
            __MCScriptUnreachable__("inappropriate type found in definition");
            break;
    }
}

bool MCScriptWriteInterfaceOfModule(MCScriptModuleRef self, MCStreamRef stream)
{
    __enterln(stream, "import module %@", self -> name);
    for(uindex_t i = 0; i < self -> dependency_count; i++)
    {
        if (MCNameIsEqualTo(self -> dependencies[i] . name, MCNAME("__builtin__")))
            continue;
        
        MCStringRef t_string;
        t_string = MCNameGetString(self -> dependencies[i] . name);
        
        __writeln(stream, "use %@", self -> dependencies[i] . name);
    }
    for(uindex_t i = 0; i < self -> exported_definition_count; i++)
    {
        MCNameRef t_def_name;
        MCScriptDefinition *t_def;
        t_def_name = self -> exported_definitions[i] . name;
        t_def = self -> definitions[self -> exported_definitions[i] . index];
        switch(t_def -> kind)
        {
            case kMCScriptDefinitionKindType:
            {
                MCScriptType *t_type;
                t_type = self -> types[static_cast<MCScriptTypeDefinition *>(t_def) -> type];
                switch(t_type -> kind)
                {
                    case kMCScriptTypeKindForeign:
                    {
                        __writeln(stream, "foreign type %@", t_def_name);
                    }
                    break;
                    case kMCScriptTypeKindForeignHandler:
                    {
                        MCAutoStringRef t_sig;
                        type_to_string(self, static_cast<MCScriptTypeDefinition *>(t_def) -> type, &t_sig);
                        __writeln(stream, "foreign handler type %@%@", t_def_name, *t_sig);
                    }
                    break;
                    case kMCScriptTypeKindHandler:
                    {
                        MCAutoStringRef t_sig;
                        type_to_string(self, static_cast<MCScriptTypeDefinition *>(t_def) -> type, &t_sig);
                        __writeln(stream, "handler type %@%@", t_def_name, *t_sig);
                    }
                    break;
                    case kMCScriptTypeKindRecord:
                        // TODO - Records not yet supported
                        break;
                    default:
                    {
                        MCAutoStringRef t_sig;
                        type_to_string(self, static_cast<MCScriptTypeDefinition *>(t_def) -> type, &t_sig);
                        __writeln(stream, "type %@ is %@", t_def_name, *t_sig);
                    }
                    break;
                }
            }
            break;
            case kMCScriptDefinitionKindConstant:
            {
                __writeln(stream, "constant %@", t_def_name);
            }
            break;
            case kMCScriptDefinitionKindVariable:
            {
                MCAutoStringRef t_type_string;
                type_to_string(self, static_cast<MCScriptVariableDefinition *>(t_def) -> type, &t_type_string);
                __writeln(stream, "variable %@ as %@", t_def_name, *t_type_string);
            }
            break;
            case kMCScriptDefinitionKindHandler:
            {
                MCAutoStringRef t_sig;
                type_to_string(self, static_cast<MCScriptHandlerDefinition *>(t_def) -> type, &t_sig);
                __writeln(stream, "handler %@%@", t_def_name, *t_sig);
            }
            break;
            case kMCScriptDefinitionKindForeignHandler:
            {
                MCAutoStringRef t_sig;
                type_to_string(self, static_cast<MCScriptForeignHandlerDefinition *>(t_def) -> type, &t_sig);
                __writeln(stream, "foreign handler %@%@", t_def_name, *t_sig);
            }
            break;
            default:
                break;
        }
    }
    __leaveln(stream, "end module");
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
