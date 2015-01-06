/* Copyright (C) 2003-2014 Runtime Revolution Ltd.
 
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

#ifndef _MODULE_ENGINE_H_
#define _MODULE_ENGINE_H_

#include <foundation.h>

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCScriptObject *MCScriptObjectRef;

extern "C"
{
    extern MC_DLLEXPORT MCTypeInfoRef kMCEngineScriptObjectTypeInfo;
}

bool MCScriptObjectCreate(MCObject *p_object, uint32_t p_part_id, MCScriptObjectRef& r_object);

////////////////////////////////////////////////////////////////////////////////

#endif