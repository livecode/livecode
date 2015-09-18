/* Copyright (C) 2009-2015 LiveCode Ltd.

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
