#include "script.h"
#include "script-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateInstanceOfModule(MCScriptModuleRef p_module, MCScriptInstanceRef& r_instance)
{
    bool t_success;
    t_success = true;
    
    MCScriptInstanceRef t_instance;
    t_instance = nil;
    
    if (t_success)
        t_success = MCScriptCreateObject(kMCScriptObjectKindInstance, sizeof(MCScriptInstance), (MCScriptObject*&)t_instance);
    
    if (t_success)
        ;
}

void MCScriptDestroyInstance(MCScriptInstanceRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    for(uindex_t i = 0; i < self -> module -> slot_count; i++)
        MCValueRelease(self -> slots[i]);
    MCMemoryDeleteArray(self -> slots);
}

////////////////////////////////////////////////////////////////////////////////
