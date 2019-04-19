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
#include <foundation-system.h>

#include <libscript/script.h>
#include <libscript/script-auto.h>

#include "script-private.h"
#include "script-bytecode.hpp"

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
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif

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

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerDefinition)
    MC_PICKLE_UINDEX(type)
    MC_PICKLE_ARRAY_OF_UINDEX(local_types, local_type_count)
    MC_PICKLE_ARRAY_OF_NAMEREF(local_names, local_name_count)
    MC_PICKLE_UINDEX(start_address)
    MC_PICKLE_UINDEX(finish_address)
    MC_PICKLE_INTSET(MCScriptHandlerAttributes, attributes)
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

/* Re-enable invalid-offsetof warnings */
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

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
        default:
            return MCSTR("unknown");
    }
    
    return kMCEmptyString;
}

////////////////////////////////////////////////////////////////////////////////

void MCScriptDestroyModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    // Call the finalizer, if any.
    if (self->finalizer != nullptr)
    {
        self->finalizer();
    }
    
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
            switch(t_def->language)
            {
            case kMCScriptForeignHandlerLanguageUnknown:
                break;
            case kMCScriptForeignHandlerLanguageC:
                break;
            case kMCScriptForeignHandlerLanguageBuiltinC:
                break;
            case kMCScriptForeignHandlerLanguageObjC:
                MCMemoryDelete(t_def->objc.function_cif);
                break;
            case kMCScriptForeignHandlerLanguageJava:
                MCValueRelease(t_def->java.class_name);
                break;
            }
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
    
    // Free any loaded libraries
    if (self->libraries != nullptr)
    {
        MCValueRelease(self->libraries);
    }
    
    // Free the compiled module representation
    MCPickleRelease(kMCScriptModulePickleInfo, self);
}

bool MCScriptValidateModule(MCScriptModuleRef self)
{
    for(uindex_t i = 0; i < self -> definition_count; i++)
	{
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
			
			MCScriptValidateState t_state;
			t_state.error = false;
			t_state.module = self;
			t_state.handler = t_handler;
			
			MCTypeInfoRef t_signature;
			t_signature = self -> types[t_handler -> type] -> typeinfo;
			t_state.register_limit = MCHandlerTypeInfoGetParameterCount(t_signature) +
										t_handler -> local_type_count;
            
			const byte_t *t_bytecode;
			const byte_t *t_bytecode_limit;
            t_bytecode = self -> bytecode + t_handler -> start_address;
            t_bytecode_limit = self -> bytecode + t_handler -> finish_address;
			
			// If there is no bytecode, the bytecode is malformed.
			if (t_bytecode == t_bytecode_limit)
				goto invalid_bytecode_error;
			
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
				goto invalid_bytecode_error;
			
			// If we didn't reach the limit, the bytecode is malformed.
			if (t_bytecode != t_bytecode_limit)
				goto invalid_bytecode_error;
			
            // If the last operation was not return, the bytecode is malformed.
			if (t_state.operation != kMCScriptBytecodeOpReturn)
				goto invalid_bytecode_error;
			
            // The total number of slots we need is recorded in register_limit.
            t_handler -> slot_count = t_state.register_limit;
        }
	}
	
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

uint32_t MCScriptGetRetainCountOfModule(MCScriptModuleRef self)
{
	if (nil == self)
		return 0;
	
	__MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
	
	return MCScriptGetRetainCountOfObject(self);
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
    if (!MCScriptCreateObject(kMCScriptObjectKindModule, t_module))
        return false;
    
    // initialize licensed state
    t_module->licensed = true;
    
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
        if (MCNameIsEqualToCaseless(t_other_module -> name, t_module -> name))
        {
            MCScriptDestroyObject(t_module);
            return MCErrorThrowGeneric(MCSTR("module already loaded"));
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

MC_DLLEXPORT_DEF bool
MCScriptCreateModulesFromStream(MCStreamRef p_stream,
                                MCAutoScriptModuleRefArray& x_modules)
{
    size_t t_available = 0;
    do
    {
        MCAutoScriptModuleRef t_module;

        if (!MCScriptCreateModuleFromStream(p_stream, &t_module))
            return false;

        if (!x_modules.Push(*t_module))
            return false;

        if (!MCStreamGetAvailableForRead(p_stream, t_available))
            return false;
    }
    while (t_available > 0);

    return true;
}

MC_DLLEXPORT_DEF bool
MCScriptCreateModulesFromData(MCDataRef p_data,
                              MCAutoScriptModuleRefArray& x_modules)
{
    MCAutoValueRefBase<MCStreamRef> t_stream;

    if (!MCMemoryInputStreamCreate(MCDataGetBytePtr(p_data),
                                   MCDataGetLength(p_data),
                                   &t_stream))
        return false;

    return MCScriptCreateModulesFromStream(*t_stream, x_modules);
}

void
MCScriptConfigureBuiltinModule(MCScriptModuleRef p_module,
                               bool (*p_initializer)(void),
                               void (*p_finalizer)(void),
                               void **p_builtins)
{
    p_module->initializer = p_initializer;
    p_module->finalizer = p_finalizer;
    p_module->builtins = p_builtins;
}

bool MCScriptLookupModule(MCNameRef p_name, MCScriptModuleRef& r_module)
{
    for(MCScriptModule *t_module = s_modules; t_module != nil; t_module = t_module -> next_module)
        if (MCNameIsEqualToCaseless(p_name, t_module -> name))
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

void
MCScriptSetModuleLicensed(MCScriptModuleRef self, bool p_licensed)
{
    self->licensed = p_licensed;
}

bool
MCScriptIsModuleLicensed(MCScriptModuleRef self)
{
    return self->licensed;
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

    // Now call the initializer, if any.
    if (self->initializer != nullptr)
    {
        if (!self->initializer())
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
                
                uindex_t t_offset = 0;
                if (!MCStringFirstIndexOfChar(t_type->binding, ':', 0, kMCStringOptionCompareExact, t_offset))
                {
                    bool t_is_builtin = false;
                    void *t_symbol = nullptr;
                    integer_t t_ordinal = 0;
                    if (self->builtins != nullptr &&
                        MCTypeConvertStringToLongInteger(t_type->binding, t_ordinal))
                    {
                        t_symbol = self->builtins[t_ordinal];
                        t_is_builtin = true;
                    }
                    else
                    {
                        t_symbol = MCSLibraryLookupSymbol(MCScriptGetLibrary(),
                                                          t_type->binding);
                        t_is_builtin = false;
                    }
                    
                    if (t_symbol == nullptr)
                    {
                        MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - unable to resolve foreign type '%{type}'"),
                                                       "name", self -> name,
                                                       "type", t_type -> binding,
                                                       nil);
                        goto error_cleanup;
                    }

                    /* The symbol is a function that returns a type info reference. */
                    if (t_is_builtin)
                    {
                        MCTypeInfoRef t_typeinfo_bare;
                        void (*t_type_func_builtin)(void*rv, void**av) = (void(*)(void*, void**))t_symbol;
                        t_type_func_builtin(&t_typeinfo_bare, nullptr);
                        t_typeinfo = t_typeinfo_bare;
                    }
                    else
                    {
                        MCTypeInfoRef (*t_type_func)(void) = (MCTypeInfoRef (*)(void)) t_symbol;
                        t_typeinfo = t_type_func();
                    }
                }
                else
                {
#if defined(__EMSCRIPTEN__) // Skip foreign type constructor check on emscripten
					t_typeinfo = kMCNullTypeInfo;
#else
                    MCAutoStringRef t_type_func, t_args;
                    if (!MCStringDivideAtChar(t_type->binding, ':', kMCStringOptionCompareExact, &t_type_func, &t_args))
                    {
                        goto error_cleanup;
                    }
                    
                    void *t_symbol =
                            MCSLibraryLookupSymbol(MCScriptGetLibrary(),
                                                   *t_type_func);
                    if (t_symbol == nullptr)
                    {
                        MCErrorThrowGenericWithMessage(MCSTR("%{name} not usable - unable to resolve foreign type constructor '%{type}'"),
                                                       "name", self -> name,
                                                       "type", *t_type_func,
                                                       nil);
                        goto error_cleanup;
                    }
                    
                    bool (*t_type_constructor)(MCStringRef p_binding, MCTypeInfoRef& r_typeinfo) =
                            (bool(*)(MCStringRef, MCTypeInfoRef&))t_symbol;
                    
                    if (!t_type_constructor(*t_args, &t_typeinfo))
                    {
                        goto error_cleanup;
                    }
 #endif
                }
            }
            break;
            case kMCScriptTypeKindRecord:
            {
                MCScriptRecordType *t_type;
                t_type = static_cast<MCScriptRecordType *>(self -> types[i]);
                
                MCAutoArray<MCRecordTypeFieldInfo> t_fields;
                for(uindex_t j = 0; j < t_type -> field_count; j++)
                {
                    MCRecordTypeFieldInfo t_field;
                    t_field . name = t_type -> fields[j] . name;
                    t_field . type = self -> types[t_type -> fields[j] . type] -> typeinfo;
                    if (!t_fields . Push(t_field))
						goto error_cleanup; // oom
                }
                
                if (!MCRecordTypeInfoCreate(t_fields . Ptr(), t_type -> field_count, &t_typeinfo))
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
                    t_parameter . mode = t_type -> parameters[j] . GetFieldMode();
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

bool
MCScriptLoadModuleLibrary(MCScriptModuleRef self,
                          MCStringRef p_library_name_string,
                          MCSLibraryRef& r_library)
{
    /* If the string is empty, then use the library containing libscript (i.e.
     * 'builtin'). */
    if (MCStringIsEmpty(p_library_name_string))
    {
        r_library = MCScriptGetLibrary();
        return true;
    }
    
    /* Create a nameref for the library so we can lookup up in the module's
     * library list. */
    MCNewAutoNameRef t_library_name;
    if (!MCNameCreate(p_library_name_string,
                      &t_library_name))
    {
        return false;
    }
    
    /* If there is a libraries array, then check whether the library has been
     * loaded before, and if so use that library. Otherwise create the libraries
     * array. */
    if (self->libraries != nullptr)
    {
        if (MCArrayFetchValue(self->libraries,
                              true,
                              *t_library_name,
                              (MCValueRef&)r_library))
        {
            return true;
        }
    }
    else
    {
        if (!MCArrayCreateMutable(self->libraries))
        {
            return false;
        }
    }
    
    /* Attempt to load the library */
    MCSAutoLibraryRef t_library;
    if (!MCScriptLoadLibrary(self,
                             p_library_name_string,
                             &t_library))
    {
        return false;
    }
    
    /* Store the library in the libraries array for the module. */
    if (!MCArrayStoreValue(self->libraries,
                           true,
                           *t_library_name,
                           *t_library))
    {
        return false;
    }
    
    /* Return the library (unretained - as the module holds a reference through
     * its array). */
    r_library = *t_library;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool
__MCScriptCopyDefinitionsOfModule(MCScriptModuleRef self,
								  MCScriptDefinitionKind p_kind,
								  /* copy */ MCProperListRef& r_names)
{
	MCAutoProperListRef t_names;
	if (!MCProperListCreateMutable(&t_names))
	{
		return false;
	}
	
	for(uindex_t i = 0; i < self -> exported_definition_count; i++)
	{
		MCScriptDefinition *t_definition =
			self->definitions[self->exported_definitions[i].index];
		
		if (p_kind != kMCScriptDefinitionKindNone &&
			t_definition->kind != p_kind)
		{
			continue;
		}
		
		if (!MCProperListPushElementOntoBack(*t_names,
											 self->exported_definitions[i].name))
		{
			return false;
		}
	}
	
	if (!t_names.MakeImmutable())
	{
		return false;
	}
	
	r_names = t_names.Take();
	
	return true;
}

bool MCScriptListDependenciesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_module_names)
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

bool MCScriptListConstantNamesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_constant_names)
{
	return __MCScriptCopyDefinitionsOfModule(self,
											 kMCScriptDefinitionKindConstant,
											 r_constant_names);
}

bool MCScriptQueryConstantOfModule(MCScriptModuleRef self, MCNameRef p_constant, MCValueRef& r_constant_value)
{
	MCScriptConstantDefinition *t_constant_def = nil;
	
    if (!self -> is_usable)
		return false;
	
	if (!MCScriptLookupConstantDefinitionInModule(self,
												  p_constant,
												  t_constant_def))
	{
		return false;
	}
	
	r_constant_value = self->values[t_constant_def->value];
	
	return true;
}

bool MCScriptListPropertyNamesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_property_names)
{
	return __MCScriptCopyDefinitionsOfModule(self,
											 kMCScriptDefinitionKindProperty,
											 r_property_names);
}

bool MCScriptQueryPropertyOfModule(MCScriptModuleRef self, MCNameRef p_property, /* get */ MCTypeInfoRef& r_getter, /* get */ MCTypeInfoRef& r_setter)
{
    MCScriptPropertyDefinition *t_def;
    
    if (!self -> is_usable)
		return false;
    
    if (!MCScriptLookupPropertyDefinitionInModule(self, p_property, t_def))
		return false;
    
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

bool MCScriptListEventNamesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_event_names)
{
	return __MCScriptCopyDefinitionsOfModule(self,
											 kMCScriptDefinitionKindEvent,
											 r_event_names);
}

bool MCScriptQueryEventOfModule(MCScriptModuleRef self, MCNameRef p_event, /* get */ MCTypeInfoRef& r_signature)
{
    MCScriptEventDefinition *t_def;
    
    if (!self -> is_usable)
		return false;
    
    if (!MCScriptLookupEventDefinitionInModule(self, p_event, t_def))
		return false;
    
    r_signature = self -> types[t_def -> type] -> typeinfo;
    
    return true;
}

bool MCScriptListHandlerNamesOfModule(MCScriptModuleRef self, /* copy */ MCProperListRef& r_handler_names)
{
	return __MCScriptCopyDefinitionsOfModule(self,
											 kMCScriptDefinitionKindHandler,
											 r_handler_names);
}

bool MCScriptQueryHandlerSignatureOfModule(MCScriptModuleRef self, MCNameRef p_handler, /* get */ MCTypeInfoRef& r_signature)
{
    MCScriptHandlerDefinition *t_def;
    
    if (!self -> is_usable)
		return false;
    
    if (!MCScriptLookupHandlerDefinitionInModule(self, p_handler, t_def))
		return false;
    
    r_signature = self -> types[t_def -> type] -> typeinfo;
    
    return true;
}

bool MCScriptListHandlerParameterNamesOfModule(MCScriptModuleRef self, MCNameRef p_handler, /* copy */ MCProperListRef& r_names)
{
    MCScriptHandlerDefinition *t_def;
    
    if (!self -> is_usable)
	{
		return false;
	}
	
    if (!MCScriptLookupHandlerDefinitionInModule(self, p_handler, t_def))
	{
		return false;
	}
	
	MCAutoProperListRef t_names;
	if (!MCProperListCreateMutable(&t_names))
	{
		return false;
	}
	
	for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(self->types[t_def->type]->typeinfo); i++)
	{
		if (!MCProperListPushElementOntoBack(*t_names,
											 MCScriptGetNameOfParameterInModule(self,
																				t_def,
																				i)))
		{
			return false;
		}
	}
	
	r_names = t_names.Take();
	
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

template <typename T, MCScriptDefinitionKind K> static bool
__MCScriptLookupDefinitionInModule(MCScriptModuleRef self,
								   MCNameRef p_name,
								   T*& r_definition)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
	
    for(uindex_t i = 0; i < self->exported_definition_count; i++)
    {
        if (K != kMCScriptDefinitionKindNone &&
			self->definitions[self->exported_definitions[i].index]->kind != K)
		{
            continue;
		}
        
        if (!MCNameIsEqualToCaseless(p_name,
							 self->exported_definitions[i].name))
		{
            continue;
        }
		
        r_definition = static_cast<T*>(self->definitions[self->exported_definitions[i].index]);
		
        return true;
    }
    
    return false;
	
}

bool MCScriptLookupConstantDefinitionInModule(MCScriptModuleRef self, MCNameRef p_constant, MCScriptConstantDefinition*& r_definition)
{
	return __MCScriptLookupDefinitionInModule<MCScriptConstantDefinition,
											  kMCScriptDefinitionKindConstant>(self,
																			   p_constant,
																			   r_definition);
}

bool MCScriptLookupPropertyDefinitionInModule(MCScriptModuleRef self, MCNameRef p_property, MCScriptPropertyDefinition*& r_definition)
{
	return __MCScriptLookupDefinitionInModule<MCScriptPropertyDefinition,
											  kMCScriptDefinitionKindProperty>(self,
																			   p_property,
																			   r_definition);
}

bool MCScriptLookupEventDefinitionInModule(MCScriptModuleRef self, MCNameRef p_event, MCScriptEventDefinition*& r_definition)
{
	return __MCScriptLookupDefinitionInModule<MCScriptEventDefinition,
											  kMCScriptDefinitionKindEvent>(self,
																			p_event,
																			r_definition);
}

bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef self, MCNameRef p_handler, MCScriptHandlerDefinition*& r_definition)
{
	return __MCScriptLookupDefinitionInModule<MCScriptHandlerDefinition,
											  kMCScriptDefinitionKindHandler>(self,
																			  p_handler,
																			  r_definition);
}

bool MCScriptLookupDefinitionInModule(MCScriptModuleRef self, MCNameRef p_name, MCScriptDefinition*& r_definition)
{
	return __MCScriptLookupDefinitionInModule<MCScriptDefinition,
											  kMCScriptDefinitionKindNone>(self,
																		   p_name,
																		   r_definition);
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

static MCTypeInfoRef
__MCScriptGetTypeWithIndexInModule(MCScriptModuleRef self,
								   uindex_t p_index)
{
	return self->types[p_index]->typeinfo;
}

MCNameRef
MCScriptGetNameOfDefinitionInModule(MCScriptModuleRef self,
									MCScriptDefinition *p_definition)
{
    uindex_t t_index;
    if (__index_of_definition(self, p_definition, t_index) &&
        self->definition_name_count > 0)
        return self->definition_names[t_index];
    return kMCEmptyName;
}

MCNameRef
MCScriptGetNameOfParameterInModule(MCScriptModuleRef self,
								   MCScriptCommonHandlerDefinition *p_handler_def,
								   uindex_t p_index)
{
    MCScriptHandlerType *t_type =
			static_cast<MCScriptHandlerType *>(self->types[p_handler_def->type]);
    
    if (t_type->parameter_name_count == 0)
	{
		return kMCEmptyName;
		
	}
	
	return t_type->parameter_names[p_index];
}

MCTypeInfoRef
MCScriptGetTypeOfParameterInModule(MCScriptModuleRef self,
									   MCScriptCommonHandlerDefinition *p_handler_def,
									   uindex_t p_index)
{
	MCScriptHandlerType *t_handler_type =
			static_cast<MCScriptHandlerType *>(self->types[p_handler_def->type]);
	
	if (p_index >= t_handler_type->parameter_count)
	{
		return kMCNullTypeInfo;
	}
	
	return __MCScriptGetTypeWithIndexInModule(self,
											  t_handler_type->parameters[p_index].type);
}

MCTypeInfoRef
MCScriptGetTypeOfReturnValueInModule(MCScriptModuleRef self,
										 MCScriptCommonHandlerDefinition *p_handler_def)
{
	MCScriptHandlerType *t_handler_type =
			static_cast<MCScriptHandlerType *>(self->types[p_handler_def->type]);
	
	return __MCScriptGetTypeWithIndexInModule(self,
												t_handler_type->return_type);
}

MCNameRef
MCScriptGetNameOfLocalVariableInModule(MCScriptModuleRef self,
									   MCScriptHandlerDefinition *p_handler_def,
									   uindex_t p_index)
{
    MCScriptHandlerType *t_type =
			static_cast<MCScriptHandlerType *>(self -> types[p_handler_def -> type]);
    
    if (p_index < t_type->parameter_count)
        return t_type->parameter_names[p_index];
    
    if (p_index - t_type->parameter_count < p_handler_def->local_name_count)
        return p_handler_def->local_names[p_index - t_type->parameter_count];
    
    return kMCEmptyName;
}

MCTypeInfoRef
MCScriptGetTypeOfLocalVariableInModule(MCScriptModuleRef self,
										   MCScriptHandlerDefinition *p_handler_def,
										   uindex_t p_index)
{
	MCScriptHandlerType *t_type =
			static_cast<MCScriptHandlerType *>(self->types[p_handler_def->type]);
	
	if (p_index < t_type->parameter_count)
	{
		return __MCScriptGetTypeWithIndexInModule(self,
												  t_type->parameters[p_index].type);
	}
	
	if (p_index - t_type->parameter_count < p_handler_def->local_type_count)
	{
		return __MCScriptGetTypeWithIndexInModule(self,
												  p_handler_def->local_types[p_index - t_type->parameter_count]);
	}
	
	return kMCNullTypeInfo;
}

MCNameRef
MCScriptGetNameOfGlobalVariableInModule(MCScriptModuleRef self,
										MCScriptVariableDefinition *p_variable_def)
{
	return MCScriptGetNameOfDefinitionInModule(self,
											   p_variable_def);
}

MCTypeInfoRef
MCScriptGetTypeOfGlobalVariableInModule(MCScriptModuleRef self,
										MCScriptVariableDefinition *p_variable_def)
{
	return __MCScriptGetTypeWithIndexInModule(self,
											  p_variable_def->type);
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
    
    if (!MCNameIsEqualToCaseless(t_name, MCNAME("undefined")))
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
        if (MCNameIsEqualToCaseless(self -> dependencies[i] . name, MCNAME("__builtin__")))
            continue;
        
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
                    {
                        auto t_record_type =
                            static_cast<MCScriptRecordType *>(t_type);
                        __enterln(stream, "record type %@", t_def_name);
                        for (uindex_t t_field = 0;
                             t_field < t_record_type->field_count;
                             ++t_field)
                        {
                            MCAutoStringRef t_ftype_name;
                            type_to_string(self, t_record_type->fields[t_field].type,
                                           &t_ftype_name);
                            __writeln(stream, "%@ as %@",
                                      t_record_type->fields[t_field].name,
                                      *t_ftype_name);
                        }
                        __leaveln(stream, "end type");
                    }
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
                MCScriptHandlerDefinition *t_handler;
                t_handler = static_cast<MCScriptHandlerDefinition *>(t_def);
                MCAutoStringRef t_sig;
                type_to_string(self, t_handler -> type, &t_sig);
                if ((t_handler -> attributes & kMCScriptHandlerAttributeUnsafe) == 0)
                    __writeln(stream, "handler %@%@", t_def_name, *t_sig);
                else
                    __writeln(stream, "unsafe handler %@%@", t_def_name, *t_sig);
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
