/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include <Foundation/Foundation.h>

#include "script-execute.hpp"

bool MCScriptCallObjCCatchingErrors(ffi_cif *p_cif, void (*p_function)(), void *p_result_ptr, void **p_arg_ptrs)
{
    @try
    {
        ffi_call(p_cif, p_function, p_result_ptr, p_arg_ptrs);
        return true;
    }
    @catch (NSException *exception)
    {
        MCAutoStringRef t_reason;
        if (!MCStringCreateWithCFStringRef((CFStringRef)[exception reason], &t_reason))
        {
            return false;
        }
        
        return MCScriptThrowForeignExceptionError(*t_reason);
    }
    
    return false;
}
