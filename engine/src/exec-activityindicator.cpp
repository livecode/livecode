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

MC_EXEC_DEFINE_EXEC_METHOD(ActivityIndicator, StartActivityIndicator, 3)
MC_EXEC_DEFINE_EXEC_METHOD(ActivityIndicator, StopActivityIndicator, 0)

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCActivityIndicatorElementInfo[] =
{
    { "white", kMCActivityIndicatorWhite },
    { "large white", kMCActivityIndicatorWhiteLarge},
    { "gray", kMCActivityIndicatorGray}
};

static MCExecEnumTypeInfo _kMCBusyIndicatorTypeInfo =
{
    "ActivityIndicator.Indicator",
    sizeof(_kMCActivityIndicatorElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCActivityIndicatorElementInfo
};

MCExecEnumTypeInfo* kMCActivityIndicatorTypeInfo = &_kMCBusyIndicatorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCSystemActivityIndicatorStart (intenum_t p_indicator, integer_t p_location_x, integer_t p_location_y);
bool MCSystemActivityIndicatorStop ();

////////////////////////////////////////////////////////////////////////////////

void MCBusyIndicatorExecStartActivityIndicator(MCExecContext& ctxt, intenum_t p_indicator, integer_t p_location_x, integer_t p_location_y)
{
    MCSystemActivityIndicatorStart(p_indicator, p_location_x, p_location_y);
}

void MCBusyIndicatorExecStopActivityIndicator(MCExecContext& ctxt)
{
    MCSystemActivityIndicatorStop ();
}