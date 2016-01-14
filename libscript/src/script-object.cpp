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

#include "libscript/script.h"
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
MCTypeInfoRef kMCScriptNotAStringValueErrorTypeInfo;
MCTypeInfoRef kMCScriptWrongNumberOfArgumentsErrorTypeInfo;
MCTypeInfoRef kMCScriptForeignHandlerBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptMultiInvokeBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptTypeBindingErrorTypeInfo;
MCTypeInfoRef kMCScriptNoMatchingHandlerErrorTypeInfo;
MCTypeInfoRef kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidPropertyValueErrorTypeInfo;
MCTypeInfoRef kMCScriptNotAHandlerValueErrorTypeInfo;
MCTypeInfoRef kMCScriptCannotCallContextHandlerErrorTypeInfo;
MCTypeInfoRef kMCScriptHandlerNotFoundErrorTypeInfo;
MCTypeInfoRef kMCScriptPropertyNotFoundErrorTypeInfo;

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

static MCScriptResolveSharedLibraryCallback s_resolve_shared_library_callback = nil;

static bool MCFetchBuiltinModuleSection(MCBuiltinModule**& r_modules, unsigned int& r_count);

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
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCAnyTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("any"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_null_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_null_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCNullTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("undefined"), t_type_index, t_null_type_index);
        MCScriptAddExportToModule(t_builder, t_null_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCBooleanTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("boolean"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCNumberTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("number"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_string_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_string_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCStringTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("string"), t_type_index, t_string_type_index);
        MCScriptAddExportToModule(t_builder, t_string_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCDataTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("data"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCArrayTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("array"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCProperListTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("list"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        ////
        
        uindex_t t_bool_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_bool_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignBoolTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("bool"), t_type_index, t_bool_type_index);
        MCScriptAddExportToModule(t_builder, t_bool_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignIntTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("int"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_uint_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_uint_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignUIntTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("uint"), t_type_index, t_uint_type_index);
        MCScriptAddExportToModule(t_builder, t_uint_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignFloatTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("float"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        uindex_t t_double_type_index;
        MCScriptAddDefinitionToModule(t_builder, t_double_type_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignDoubleTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("double"), t_type_index, t_double_type_index);
        MCScriptAddExportToModule(t_builder, t_double_type_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("MCForeignPointerTypeInfo"), t_type_index);
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
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatDownToCondition"), t_type_index, MCSTR("MCScriptBuiltinRepeatDownToCondition"), t_def_index);
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
        
		if (!MCMemoryInputStreamCreate(t_buffer, t_size, t_stream))
			return false;
        if (!MCScriptCreateModuleFromStream(t_stream, s_builtin_module))
            return false;
        
        MCValueRelease(t_stream);
    }
    
    // This block creates all the default errors
    {
        if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.InParameterNotDefinedError"), MCNAME("runtime"), MCSTR("In parameters must be defined before calling - parameter %{parameter} of %{module}.%{handler}"), kMCScriptInParameterNotDefinedErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.OutParameterNotDefinedError"), MCNAME("runtime"), MCSTR("Out parameters must be defined before returning - parameter %{parameter} of %{module}.%{handler}"), kMCScriptOutParameterNotDefinedErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.VariableUsedBeforeDefinedError"), MCNAME("runtime"), MCSTR("Variables must be defined before being used - variable %{variable} in %{module}.%{handler}"), kMCScriptVariableUsedBeforeDefinedErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.ReturnValueTypeError"), MCNAME("runtime"), MCSTR("Value is not of correct type for return - expected type %{type} when returning from %{module}.%{handler}"), kMCScriptInvalidReturnValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.VariableValueTypeError"), MCNAME("runtime"), MCSTR("Value is not of correct type for assignment to variable - expected type %{type} for assigning to variable %{variable} in %{module}.%{handler}"), kMCScriptInvalidVariableValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.ArgumentValueTypeError"), MCNAME("runtime"), MCSTR("Value is not of correct type for passing as argument - expected type %{type} for passing to parameter %{parameter} of %{module}.%{handler}"), kMCScriptInvalidArgumentValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.NotABooleanValueError"), MCNAME("runtime"), MCSTR("Value is not a boolean"), kMCScriptNotABooleanValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.NotAStringValueError"), MCNAME("runtime"), MCSTR("Value is not a string"), kMCScriptNotAStringValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.WrongNumberOfArgumentsError"), MCNAME("runtime"), MCSTR("Wrong number of arguments passed to handler %{module}.%{handler}"), kMCScriptWrongNumberOfArgumentsErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.ForeignHandlerBindingError"), MCNAME("runtime"), MCSTR("Unable to bind foreign handler %{module}.%{handler}"), kMCScriptForeignHandlerBindingErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PolymorphicHandlerBindingError"), MCNAME("runtime"), MCSTR("Unable to bind appropriate handler"), kMCScriptMultiInvokeBindingErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.NoMatchingHandlerError"), MCNAME("runtime"), MCSTR("No matching handler for arguments with types (%{types}) - possible handlers (%{handlers})"), kMCScriptNoMatchingHandlerErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.CannotSetReadOnlyPropertyError"), MCNAME("runtime"), MCSTR("Cannot set read-only property %{module}.%{property}"), kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PropertyValueTypeError"), MCNAME("runtime"), MCSTR("Value is not of correct type for setting property - expected type %{type} for setting property %{module}.%{property}"), kMCScriptInvalidPropertyValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.NotAHandlerValueError"), MCNAME("runtime"), MCSTR("Value is not a handler"), kMCScriptNotAHandlerValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.CannotCallContextHandlerError"), MCNAME("runtime"), MCSTR("Cannot call context handler"), kMCScriptCannotCallContextHandlerErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.HandlerNotFoundError"), MCNAME("runtime"), MCSTR("No handler %{handler} in module %{module}"), kMCScriptHandlerNotFoundErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PropertyNotFoundError"), MCNAME("runtime"), MCSTR("No property %{property} in module %{module}"), kMCScriptPropertyNotFoundErrorTypeInfo))
			return false;
    }

    s_resolve_shared_library_callback = nil;
    
    return true;
}

void MCScriptFinalize(void)
{
    for(uindex_t i = 0; i < s_builtin_module_count; i++)
        MCScriptReleaseModule(s_builtin_modules[i]);
    MCMemoryDeleteArray(s_builtin_modules);
    MCValueRelease(s_builtin_module);
}

void MCScriptSetResolveSharedLibraryCallback(MCScriptResolveSharedLibraryCallback p_callback)
{
    s_resolve_shared_library_callback = p_callback;
}

bool MCScriptResolveSharedLibrary(MCScriptModuleRef p_module, MCStringRef p_name, MCStringRef& r_path)
{
    if (s_resolve_shared_library_callback == nil)
        return false;
    
    return s_resolve_shared_library_callback(p_module, p_name, r_path);
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
