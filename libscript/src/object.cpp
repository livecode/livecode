#include "script.h"
#include "script-private.h"

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
        case kMCScriptObjectKindError:
            MCScriptDestroyError((MCScriptError *)self);
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
