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

MCTypeInfoRef kMCScriptVariableUsedBeforeAssignedErrorTypeInfo;
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
MCTypeInfoRef kMCScriptPropertyUsedBeforeAssignedErrorTypeInfo;
MCTypeInfoRef kMCScriptInvalidPropertyValueErrorTypeInfo;
MCTypeInfoRef kMCScriptNotAHandlerValueErrorTypeInfo;
MCTypeInfoRef kMCScriptHandlerNotFoundErrorTypeInfo;
MCTypeInfoRef kMCScriptPropertyNotFoundErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

struct MCBuiltinModule
{
    MCBuiltinModule *next;
    MCScriptModuleRef handle;
    const unsigned char *data;
    long size;
    bool (*initializer)(void);
    void (*finalizer)(void);
    void **builtins;
};

static MCBuiltinModule *s_builtin_modules = nil;
static MCScriptModuleRef s_builtin_module = nil;
static MCScriptLoadLibraryCallback s_load_library_callback = nil;

extern "C" MC_DLLEXPORT_DEF void MCScriptRegisterBuiltinModule(MCBuiltinModule *p_module)
{
    if (p_module -> next != nullptr)
    {
        return;
    }
    
    p_module->next = s_builtin_modules;
    s_builtin_modules = p_module;
}

static MCSLibraryRef s_libscript_library = nullptr;

bool MCScriptInitialize(void)
{
    if (!MCSLibraryCreateWithAddress(reinterpret_cast<void *>(MCScriptInitialize),
                                     s_libscript_library))
    {
        return false;
    }

    for(MCBuiltinModule *t_module = s_builtin_modules; t_module != nullptr; t_module = t_module->next)
    {
        MCStreamRef t_stream;
        if (!MCMemoryInputStreamCreate(t_module->data, t_module->size, t_stream))
            return false;
        
        if (!MCScriptCreateModuleFromStream(t_stream, t_module->handle))
            return false;
        
        MCScriptConfigureBuiltinModule(t_module->handle,
                                       t_module->initializer,
                                       t_module->finalizer,
                                       t_module->builtins);
        
        MCValueRelease(t_stream);
    }
    
    // This block creates all the default errors
    {
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.VariableUsedBeforeAssignedError"), MCNAME("runtime"), MCSTR("Variables must be assigned before being used - variable %{variable} in %{module}.%{handler}"), kMCScriptVariableUsedBeforeAssignedErrorTypeInfo))
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
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PropertyUsedBeforeAssignedError"), MCNAME("runtime"), MCSTR("Properties must be set before begin used - property %{module}.%{property}"), kMCScriptPropertyUsedBeforeAssignedErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PropertyValueTypeError"), MCNAME("runtime"), MCSTR("Value is not of correct type for setting property - expected type %{type} for setting property %{module}.%{property}"), kMCScriptInvalidPropertyValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.NotAHandlerValueError"), MCNAME("runtime"), MCSTR("Value is not a handler"), kMCScriptNotAHandlerValueErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.HandlerNotFoundError"), MCNAME("runtime"), MCSTR("No handler %{handler} in module %{module}"), kMCScriptHandlerNotFoundErrorTypeInfo))
			return false;
		if (!MCNamedErrorTypeInfoCreate(MCNAME("livecode.lang.PropertyNotFoundError"), MCNAME("runtime"), MCSTR("No property %{property} in module %{module}"), kMCScriptPropertyNotFoundErrorTypeInfo))
			return false;
    }

    for(MCBuiltinModule *t_module = s_builtin_modules; t_module != nullptr; t_module = t_module->next)
    {
        /* UNCHECKED */ MCScriptEnsureModuleIsUsable(t_module->handle);
    }
    
    s_load_library_callback = nullptr;
    
    return true;
}

extern "C" bool __builtin___Initialize(void)
{
    return true;
}

extern "C" void __builtin___Finalize(void)
{
}

void MCScriptFinalize(void)
{
    for(MCBuiltinModule *t_module = s_builtin_modules; t_module != nullptr; t_module = t_module->next)
    {
        MCScriptReleaseModule(t_module->handle);
    }
    MCScriptReleaseModule(s_builtin_module);
    
    MCValueRelease(s_libscript_library);
}

void MCScriptSetLoadLibraryCallback(MCScriptLoadLibraryCallback p_callback)
{
    s_load_library_callback = p_callback;
}

bool MCScriptLoadLibrary(MCScriptModuleRef p_module, MCStringRef p_name, MCSLibraryRef& r_library)
{
    // If the library name is empty, then we want libscript's library.
    if (MCStringIsEmpty(p_name))
    {
        r_library = MCValueRetain(s_libscript_library);
        return true;
    }
    
    // If there is a callback, then use that.
    if (s_load_library_callback != nullptr)
    {
        return s_load_library_callback(p_module,
                                       p_name,
                                       r_library);
    }
    
    // Otherwise, just use the name as is.
    return MCSLibraryCreateWithPath(p_name,
                                    r_library);
}

bool
MCScriptForEachBuiltinModule(MCScriptForEachBuiltinModuleCallback p_callback,
                             void *p_context)
{
    for(MCBuiltinModule *t_module = s_builtin_modules; t_module != nullptr; t_module = t_module -> next)
    {
        if (!p_callback(p_context, t_module->handle))
        {
            return false;
        }
    }
    
    return true;
}

MCSLibraryRef MCScriptGetLibrary(void)
{
    return s_libscript_library;
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
    
    MCMemoryDeallocate(self);
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

uint32_t MCScriptGetRetainCountOfObject(MCScriptObject *self)
{
	__MCScriptValidateObject__(self);
	
	return self -> references;
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
