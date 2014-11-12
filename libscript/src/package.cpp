#include "script.h"
#include "script-private.h"

////////////////////////////////////////////////////////////////////////////////

void MCScriptDestroyPackage(MCScriptPackageRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindPackage);
    
    MCValueRelease(self -> filename);
    MCValueRelease(self -> name);
    MCScriptReleaseArray(self -> modules, self -> module_count);
}

////////////////////////////////////////////////////////////////////////////////
