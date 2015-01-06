/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "script.h"
#include "script-private.h"

#include "foundation-auto.h"

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCScriptInParameterNotDefinedErrorTypeInfo;
MCTypeInfoRef kMCScriptOutParameterNotDefinedErrorTypeInfo;
MCTypeInfoRef kMCScriptVariableUsedBeforeDefinedErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidReturnValueErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidVariableValueErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidArgumentValueErrorTypeInfo;
MCTypeInfoRef kMCScriptNotABooleanValueErrorTypeInfo;
MCTypeInfoRef kMCScriptWrongNumberOfArgumentsErrorTypeInfo;
MCTypeInfoRef kMCScriptForeignHandlerBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptMultiInvokeBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptTypeBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptNoMatchingHandlerErrorTypeInfo;
MCTypeInfoRef kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidPropertyValueErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

struct MCBuiltinModule
{
    const char *name;
    unsigned char *data;
    unsigned long size;
};

static MCScriptModuleRef s_builtin_module = nil;
static MCScriptModuleRef *s_builtin_modules = nil;
static uindex_t s_builtin_module_count = 0;
static bool MCFetchBuiltinModuleSection(MCBuiltinModule**& r_modules, unsigned int& r_count);

static bool MCScriptCreateNamedErrorType(MCNameRef p_name, MCStringRef p_message, MCTypeInfoRef &r_error_type)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCErrorTypeInfoCreate(MCNAME("runtime"), p_message, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_error_type = MCValueRetain(*t_named_type);
	return true;
}

bool MCScriptInitialize(void)
{
    MCBuiltinModule **t_modules;
    unsigned int t_module_count;
    if (!MCFetchBuiltinModuleSection(t_modules, t_module_count))
        return true;
    
    if (!MCMemoryNewArray(t_module_count, s_builtin_modules))
        return false;
    
    for(uindex_t i = 0; i < t_module_count; i++)
    {
        MCLog("Loading builtin module - %s", t_modules[i] -> name);
        
        MCStreamRef t_stream;
        if (!MCMemoryInputStreamCreate(t_modules[i] -> data, t_modules[i] -> size, t_stream))
            return false;
        
        if (!MCScriptCreateModuleFromStream(t_stream, s_builtin_modules[i]))
            return false;
        
        MCValueRelease(t_stream);
    }
    
    s_builtin_module_count = t_module_count;
    
    // This block builds the builtin module - which isn't possible to compile from
    // source as the names of the types we are defining are currently part of the
    // syntax of the language.
    {
        MCScriptModuleBuilderRef t_builder;
        MCScriptBeginModule(kMCScriptModuleKindNone, MCNAME("__builtin__"), t_builder);
        
        uindex_t t_def_index, t_type_index;
            
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCAnyTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("any"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_null_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_null_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCNullTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("undefined"), t_type_index, t_null_type_index);
        MCScriptAddExportToModule(t_builder, t_null_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCBooleanTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("boolean"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCNumberTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("number"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_string_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_string_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCStringTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("string"), t_type_index, t_string_type_index);
        MCScriptAddExportToModule(t_builder, t_string_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCDataTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("data"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCArrayTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("array"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCProperListTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("list"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        ////
        
        uindex_t t_bool_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_bool_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCBoolTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("bool"), t_type_index, t_bool_type_index);
        MCScriptAddExportToModule(t_builder, t_bool_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCIntTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("int"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_uint_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_uint_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCUIntTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("uint"), t_type_index, t_uint_type_index);
        MCScriptAddExportToModule(t_builder, t_uint_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCFloatTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("float"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_double_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_double_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCDoubleTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("double"), t_type_index, t_double_type_index);
        MCScriptAddExportToModule(t_builder, t_double_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCPointerTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("pointer"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        ////
        
        uindex_t t_bool_type_def, t_uint_type_def, t_double_type_def, t_string_type_def, t_null_type_def;
        MCScriptAddDefinedTypeToModule(t_builder, t_bool_type_index, t_bool_type_def);
        MCScriptAddDefinedTypeToModule(t_builder, t_uint_type_index, t_uint_type_def);
        MCScriptAddDefinedTypeToModule(t_builder, t_double_type_index, t_double_type_def);
        MCScriptAddDefinedTypeToModule(t_builder, t_null_type_index, t_null_type_def);
        MCScriptAddDefinedTypeToModule(t_builder, t_string_type_index, t_string_type_def);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_bool_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeInOut, MCNAME("count"), t_uint_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatCounted"), t_type_index, MCSTR("MCScriptBuiltinRepeatCounted"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_bool_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("counter"), t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("limit"), t_double_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatUpToCondition"), t_type_index, MCSTR("MCScriptBuiltinRepeatUpToCondition"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("counter"), t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("step"), t_double_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatUpToIterate"), t_type_index, MCSTR("MCScriptBuiltinRepeatUpToIterate"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_bool_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("counter"), t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("limit"), t_double_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatDownToCondition"), t_type_index, MCSTR("MCScriptBuiltinRepeatUpToCondition"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("counter"), t_double_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("step"), t_double_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatDownToIterate"), t_type_index, MCSTR("MCScriptBuiltinRepeatDownToIterate"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_null_type_def);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeIn, MCNAME("reason"), t_string_type_def);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("Throw"), t_type_index, MCSTR("MCScriptBuiltinThrow"), t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        ////
        
        MCStreamRef t_stream;
        MCMemoryOutputStreamCreate(t_stream);
        MCScriptEndModule(t_builder, t_stream);
        
        void *t_buffer;
        size_t t_size;
        MCMemoryOutputStreamFinish(t_stream, t_buffer, t_size);
        MCValueRelease(t_stream);
        
        MCMemoryInputStreamCreate(t_buffer, t_size, t_stream);
        if (!MCScriptCreateModuleFromStream(t_stream, s_builtin_module))
            return false;
        
        MCValueRelease(t_stream);
    }
    
    // This block creates all the default errors
    {
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.InParameterNotDefinedError"), MCSTR("In parameters must be defined before calling - parameter %{parameter} of %{module}.%{handler}"), kMCScriptInParameterNotDefinedErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.OutParameterNotDefinedError"), MCSTR("Out parameters must be defined before returning - parameter %{parameter} of %{module}.%{handler}"), kMCScriptOutParameterNotDefinedErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.VariableUsedBeforeDefinedError"), MCSTR("Variables must be defined before being used - variable %{variable} in %{module}.%{handler}"), kMCScriptVariableUsedBeforeDefinedErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.ReturnValueTypeError"), MCSTR("Value is not of correct type for return - expected type %{type} when returning from %{module}.%{handler}"), kMCScriptInvalidReturnValueErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.VariableValueTypeError"), MCSTR("Value is not of correct type for assignment to variable - expected type %{type} for assigning to variable %{variable} in %{module}.%{handler}"), kMCScriptInvalidVariableValueErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.ArgumentValueTypeError"), MCSTR("Value is not of correct type for passing as argument - expected type %{type} for passing to parameter %{parameter} of %{module}.%{handler}"), kMCScriptInvalidArgumentValueErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.NotABooleanValueError"), MCSTR("Value is not a boolean"), kMCScriptNotABooleanValueErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.WrongNumberOfArgumentsError"), MCSTR("Wrong number of arguments passed to handler %{module}.%{handler}"), kMCScriptWrongNumberOfArgumentsErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.ForeignHandlerBindingError"), MCSTR("Unable to bind foreign handler %{module}.%{handler}"), kMCScriptForeignHandlerBindingErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.PolymorphicHandlerBindingError"), MCSTR("Unable to bind appropriate handler"), kMCScriptMultiInvokeBindingErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.TypeBindingError"), MCSTR("Attempt to use unbound named type %{type}"), kMCScriptTypeBindingErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.NoMatchingHandlerError"), MCSTR("No matching handler for arguments with types (%{types}) - possible handlers (%{handlers})"), kMCScriptNoMatchingHandlerErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.CannotSetReadOnlyPropertyError"), MCSTR("Cannot set read-only property %{module}.%{property}"), kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo);
        MCScriptCreateNamedErrorType(MCNAME("livecode.lang.PropertyValueTypeError"), MCSTR("Value is not of correct type for setting property - expected type %{type} for setting property %{module}.%{property}"), kMCScriptInvalidPropertyValueErrorTypeInfo);
    }

    return true;
}

void MCScriptFinalize(void)
{
    for(uindex_t i = 0; i < s_builtin_module_count; i++)
        MCScriptReleaseModule(s_builtin_modules[i]);
    MCMemoryDeleteArray(s_builtin_modules);
    MCValueRelease(s_builtin_module);
}

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateObject(MCScriptObjectKind p_kind, size_t p_size, MCScriptObject*& r_object)
{
    MCScriptObject *self;
    if (!MCMemoryAllocate(p_size, self))
        return false;
    
#ifdef _DEBUG
    self -> __object_marker__ = __MCSCRIPTOBJECT_MARKER__;
#endif
    
    self -> references = 1;
    self -> kind = p_kind;
    
    MCMemoryClear(self + 1, p_size - sizeof(MCScriptObject));
    
    r_object = self;
    
    return true;
}

void MCScriptDestroyObject(MCScriptObject *self)
{
    __MCScriptValidateObject__(self);
    
    switch(self -> kind)
    {
        case kMCScriptObjectKindNone:
            break;
        case kMCScriptObjectKindPackage:
            MCScriptDestroyPackage((MCScriptPackage *)self);
            break;
        case kMCScriptObjectKindModule:
            MCScriptDestroyModule((MCScriptModule *)self);
            break;
        case kMCScriptObjectKindInstance:
            MCScriptDestroyInstance((MCScriptInstance *)self);
            break;
            
        default:
            __MCScriptAssert__(false, "invalid kind");
            break;
    }
}

MCScriptObject *MCScriptRetainObject(MCScriptObject *self)
{
    __MCScriptValidateObject__(self);
    
    self -> references += 1;

    return self;
}

void MCScriptReleaseObject(MCScriptObject *self)
{
    __MCScriptValidateObject__(self);
    
    __MCScriptAssert__(self -> references > 0, "invalid reference count");
    
    self -> references -= 1;
    
    if (self -> references == 0)
        MCScriptDestroyObject(self);
}

////////////////////////////////////////////////////////////////////////////////

void MCScriptReleaseObjectArray(MCScriptObject **p_elements, uindex_t p_count)
{
    for(uindex_t i = 0; i < p_count; i++)
        MCScriptReleaseObject(p_elements[i]);
}

////////////////////////////////////////////////////////////////////////////////

void __MCScriptValidateObjectFailed__(MCScriptObject *object, const char *function, const char *file, int line)
{
    MCLog("NOT A SCRIPT OBJECT - %p, %s, %s, %d", object, function, file, line);
    abort();
}

void __MCScriptValidateObjectAndKindFailed__(MCScriptObject *object, MCScriptObjectKind kind, const char *function, const char *file, int line)
{
    MCLog("NOT A CORRECT OBJECT - %p, %d, %s, %s, %d", object, kind, function, file, line);
    abort();
}

void __MCScriptAssertFailed__(const char *label, const char *expr, const char *function, const char *file, int line)
{
    MCLog("FAILURE - %s, %s, %s, %s, %d", label, expr, function, file, line);
    abort();
}

////////////////////////////////////////////////////////////////////////////////

extern "C"
{
    extern MCBuiltinModule* g_builtin_modules[];
    extern unsigned int g_builtin_module_count;
}

static bool MCFetchBuiltinModuleSection(MCBuiltinModule**& r_modules, unsigned int& r_count)
{
	// Use the array defined in the module-helper.cpp file
    r_modules = g_builtin_modules;
    r_count = g_builtin_module_count;
    return true;
}
