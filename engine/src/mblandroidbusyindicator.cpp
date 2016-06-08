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

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include "date.h"

#include "mbldc.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblsyntax.h"

#include <string.h>

#include <jni.h>

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
//   Not implemented on Android.
bool MCSystemBusyIndicatorStart (intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity)
{
    MCAndroidEngineRemoteCall("showBusyIndicator", "vx", nil, p_label);
    return true;
}

bool MCSystemBusyIndicatorStop ()
{
    MCAndroidEngineRemoteCall("hideBusyIndicator", "v", nil);
    return true;
}

bool MCSystemActivityIndicatorStart (intenum_t p_indicator, integer_t p_location_x, integer_t p_location_y)
{
    // UNIMPLEMENTED
    return false;
}

bool MCSystemActivityIndicatorStop ()
{
    // UNIMPLEMENTED
    return false;
}
