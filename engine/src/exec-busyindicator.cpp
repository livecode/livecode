/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

static MCExecEnumTypeElementInfo _kMCBusyIndicatorElementInfo[] =
{
	{ "in line", kMCBusyIndicatorInLine, false },
	{ "square", kMCBusyIndicatorSquare, false },
	{ "keyboard", kMCBusyIndicatorKeyboard, false },
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
	{ "white", kMCActivityIndicatorWhite, false },
	{ "large white", kMCActivityIndicatorWhiteLarge, false },
	{ "gray", kMCActivityIndicatorGray, false },
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

void MCBusyIndicatorExecStartActivityIndicator(MCExecContext& ctxt, intenum_t p_indicator, integer_t* p_location_x, integer_t* p_location_y)
{
    // Check whether the location is provided
    if (p_location_x == nil || p_location_y == nil)
        MCSystemActivityIndicatorStart(p_indicator, -1, -1);
    else
        MCSystemActivityIndicatorStart(p_indicator, *p_location_x, *p_location_y);
}

void MCBusyIndicatorExecStopActivityIndicator(MCExecContext& ctxt)
{
    MCSystemActivityIndicatorStop ();
}
