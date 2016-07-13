/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

#include "ffi.h"

#include "foundation-auto.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "script-execute.hpp"

//////////

void
MCScriptExecuteContext::InvokeForeign(MCScriptInstanceRef p_instance,
									  MCScriptForeignHandlerDefinition *p_definition,
									  uindex_t p_result_reg,
									  const uindex_t *p_argument_regs,
									  uindex_t p_argument_count)
{
}
