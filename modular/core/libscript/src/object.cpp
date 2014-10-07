#include "script.h"
#include "script-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateObject(MCScriptObjectKind p_kind, size_t p_size, MCScriptObject*& r_object)
{
    return false;
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

void MCScriptRetainObject(MCScriptObject *self)
{
    __MCScriptValidateObject__(self);
    
    self -> references += 1;
}

void MCScriptReleaseObject(MCScriptObject *self)
{
    __MCScriptValidateObject__(self);
    
    __MCScriptAssert__(self -> references > 0, "invalid reference count");
    
}

////////////////////////////////////////////////////////////////////////////////

void MCscriptReleaseObjectArray(MCScriptObject **p_elements, uindex_t p_count)
{
    for(uindex_t i = 0; i < p_count; i++)
        MCScriptReleaseObject(p_elements[i]);
}

////////////////////////////////////////////////////////////////////////////////
