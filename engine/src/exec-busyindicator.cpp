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

MC_EXEC_DEFINE_EXEC_METHOD(BusyIndicator, StartActivityIndicator, 3)
MC_EXEC_DEFINE_EXEC_METHOD(BusyIndicator, StopActivityIndicator, 0)

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCBusyIndicatorElementInfo[] =
{
    { "in line", kMCBusyIndicatorInLine},
    { "square", kMCBusyIndicatorSquare},
    { "keyboard", kMCBusyIndicatorKeyboard}
};

static MCExecEnumTypeInfo _kMCBusyIndicatorTypeInfo =
{
    "BusyIndicator.BusyIndicator",
    sizeof(_kMCBusyIndicatorElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCBusyIndicatorElementInfo
};

MCExecEnumTypeInfo *kMCBusyIndicatorTypeInfo = &_kMCBusyIndicatorTypeInfo;

static MCExecEnumTypeElementInfo _kMCActivityIndicatorElementInfo[] =
{
    { "white", kMCActivityIndicatorWhite },
    { "large white", kMCActivityIndicatorWhiteLarge},
    { "gray", kMCActivityIndicatorGray}
};

static MCExecEnumTypeInfo _kMCActivityIndicatorTypeInfo =
{
    "BusyIndicator.ActivityIndicator",
    sizeof(_kMCActivityIndicatorElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCActivityIndicatorElementInfo
};

MCExecEnumTypeInfo* kMCActivityIndicatorTypeInfo = &_kMCActivityIndicatorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
void MCBusyIndicatorExecStartBusyIndicator(MCExecContext& ctxt, intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity)
{
#ifdef /* MCBusyIndicatorExecStart */ LEGACY_EXEC
    MCSystemBusyIndicatorStart(p_indicator, p_label, p_opacity);
#endif /* MCBusyIndicatorExecStart */
    if(MCSystemBusyIndicatorStart(p_indicator, p_label, p_opacity))
        return;
    
    ctxt.Throw();
}

void MCBusyIndicatorExecStopBusyIndicator(MCExecContext& ctxt)
{
#ifdef /* MCBusyIndicatorExecStop */ LEGACY_EXEC
    MCSystemBusyIndicatorStop();
#endif /* MCBusyIndicatorExecStop */
    if(MCSystemBusyIndicatorStop())
        return;
    
    ctxt.Throw();
}

void MCBusyIndicatorExecStartActivityIndicator(MCExecContext& ctxt, intenum_t p_indicator, integer_t* p_location_x, integer_t* p_location_y)
{
#ifdef /* MCActivityIndicatorExecStart */ LEGACY_EXEC
    MCSystemActivityIndicatorStart (p_indicator, p_location);
#endif /* MCActivityIndicatorExecStart */
    // Check whether the location is provided
    if (p_location_x == nil || p_location_y == nil)
        MCSystemActivityIndicatorStart(p_indicator, -1, -1);
    else
        MCSystemActivityIndicatorStart(p_indicator, *p_location_x, *p_location_y);
}

void MCBusyIndicatorExecStopActivityIndicator(MCExecContext& ctxt)
{
#ifdef /* MCActivityIndicatorExecStop */ LEGACY_EXEC
    MCSystemActivityIndicatorStop ();
#endif /* MCActivityIndicatorExecStop */
    MCSystemActivityIndicatorStop ();
}
