/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
  
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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "util.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "mblcontrol.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, CreateControl, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, DeleteControl, 1)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, SetProperty, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, GetProperty, 2)
MC_EXEC_DEFINE_EXEC_METHOD(NativeControl, Do, 0)
MC_EXEC_DEFINE_GET_METHOD(NativeControl, Target, 1)
MC_EXEC_DEFINE_GET_METHOD(NativeControl, ControlList, 1)

////////////////////////////////////////////////////////////////////////////////

void MCNativeControlExecCreateControl(MCExecContext& ctxt, MCStringRef p_type_name, MCStringRef p_control_name)
{
    ctxt . SetTheResultToEmpty();
    
    // Make sure the name is valid.
    if (MCStringIsEqualTo(p_control_name, kMCEmptyString, kMCCompareCaseless))
        return;
    
    int2 t_integer;
    if (MCU_stoi2(p_control_name, t_integer))
        return;
    
    // Make sure a control does not already exist with the name
    MCNativeControl *t_control;
    if (MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
        return;
    
    MCNativeControlType t_type;
    if (!MCNativeControl::LookupType(MCStringGetCString(p_type_name), t_type))
        return;
    
    MCNativeControl *t_new_control;
    t_new_control = nil;
    if (MCNativeControl::CreateWithType(t_type, t_new_control))
    {
        extern MCExecPoint *MCEPptr;
        t_control -> SetOwner(MCEPptr -> getobj());
        t_control -> SetName(p_control_name);
        ctxt . SetTheResultToNumber(t_new_control -> GetId());
        return;
    }
    
    if (t_control != nil)
        t_control -> Delete();
}

void MCNativeControlExecDeleteControl(MCExecContext& ctxt, MCStringRef p_control_name)
{
    MCNativeControl *t_control;
    if (!MCNativeControl::FindByNameOrId(MCStringGetCString(p_control_name), t_control))
        return;
    
    t_control -> Delete();
    t_control -> Release();
}

void MCNativeControlExecSetProperty(MCExecContext& ctxt, MCStringRef p_property, MCValueRef p_value)
{
    ctxt . Unimplemented();
}

void MCNativeControlExecGetProperty(MCExecContext& ctxt, MCStringRef p_property, MCValueRef& r_value)
{
    ctxt . Unimplemented();
}

void MCNativeControlExecDo(MCExecContext& ctxt)
{
    ctxt . Unimplemented();
}

void MCNativeControlGetTarget(MCExecContext& ctxt, MCStringRef& r_target)
{
    // UNION TYPE!
    ctxt . Unimplemented();
}

void MCNativeControlGetControlList(MCExecContext& ctxt, MCStringRef& r_list)
{
    if (MCNativeControl::GetControlList(r_list))
        return;
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////
