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

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(BusyIndicator, StartBusyIndicator, 3)
MC_EXEC_DEFINE_EXEC_METHOD(BusyIndicator, StopBusyIndicator, 0)

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCBusyIndicatorElementInfo[] =
{
    { "in line", kMCBusyIndicatorInLine},
    { "square", kMCBusyIndicatorSquare},
    { "keyboard", kMCBusyIndicatorKeyboard}
};

static MCExecEnumTypeInfo _kMCBusyIndicatorTypeInfo =
{
    "BusyIndicator.Indicator",
    sizeof(_kMCBusyIndicatorElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCBusyIndicatorElementInfo
};

MCExecEnumTypeInfo *kMCBusyIndicatorTypeInfo = &_kMCBusyIndicatorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCSystemBusyIndicatorStart (intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity);
bool MCSystemBusyIndicatorStop ();

////////////////////////////////////////////////////////////////////////////////

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
void MCBusyIndicatorExecStartBusyIndicator(MCExecContext& ctxt, intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity)
{
    if(MCSystemBusyIndicatorStart(p_indicator, p_label, p_opacity))
        return;
    
    ctxt.Throw();
}

void MCBusyIndicatorExecStopBusyIndicator(MCExecContext& ctxt)
{
    if(MCSystemBusyIndicatorStop())
        return;
    
    ctxt.Throw();
}