#ifndef __MC_MODULE__
#define __MC_MODULE__

#ifndef __MC_CORE__
#include "core.h"
#endif

typedef void *MCModuleRef;

bool MCModuleLoad(const char *p_filename, MCModuleRef& r_module);
void MCModuleUnload(MCModuleRef module);
bool MCModuleLookupSymbol(MCModuleRef module, const char *symbol, void** r_address);

bool MCModuleGetFilename(MCModuleRef module, char*& r_filename);

#endif
