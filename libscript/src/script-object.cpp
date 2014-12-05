#include "script.h"
#include "script-private.h"

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
static bool MCFetchBuiltinModuleSection(MCBuiltinModule*& r_modules, unsigned int& r_count);

bool MCScriptInitialize(void)
{
    MCBuiltinModule *t_modules;
    unsigned int t_module_count;
    if (!MCFetchBuiltinModuleSection(t_modules, t_module_count))
        return true;
    
    if (!MCMemoryNewArray(t_module_count, s_builtin_modules))
        return false;
    
    for(uindex_t i = 0; i < t_module_count; i++)
    {
        MCStreamRef t_stream;
        if (!MCMemoryInputStreamCreate(t_modules[i] . data, t_modules[i] . size, t_stream))
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
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCNullTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("undefined"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCBooleanTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("boolean"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCNumberTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("number"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCStringTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("string"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
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
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCDoubleTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("double"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddForeignTypeToModule(t_builder, MCSTR("kMCPointerTypeInfo"), t_type_index);
        MCScriptAddTypeToModule(t_builder, MCNAME("pointer"), t_type_index, t_def_index);
        MCScriptAddExportToModule(t_builder, t_def_index);
        
        ////
        
        MCScriptAddDefinitionToModule(t_builder, t_def_index);
        MCScriptAddDefinedTypeToModule(t_builder, t_bool_type_index, t_type_index);
        MCScriptBeginHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddDefinedTypeToModule(t_builder, t_uint_type_index, t_type_index);
        MCScriptContinueHandlerTypeInModule(t_builder, kMCScriptHandlerTypeParameterModeInOut, MCNAME("count"), t_type_index);
        MCScriptEndHandlerTypeInModule(t_builder, t_type_index);
        MCScriptAddForeignHandlerToModule(t_builder, MCNAME("RepeatCounted"), t_type_index, MCSTR("MCScriptBuiltinRepeatCounted"), t_def_index);
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

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)

#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>

static bool MCFetchBuiltinModuleSection(MCBuiltinModule*& r_modules, unsigned int& r_count)
{
    
    unsigned long t_section_data_size;
    char *t_section_data;
    t_section_data = getsectdata("__MODULES", "__modules", &t_section_data_size);
    if (t_section_data != nil)
    {
        t_section_data += (unsigned long)_dyld_get_image_vmaddr_slide(0);
        r_modules = (MCBuiltinModule *)t_section_data;
        r_count = t_section_data_size / sizeof(MCBuiltinModule);
        return true;
    }
    
    return false;
}
#endif
