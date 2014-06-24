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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
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

bool MCSystemBeep (int32_t p_number_of_beeps)
{
#ifdef /* MCSystemBeepAndroid */ LEGACY_EXEC
    MCAndroidEngineRemoteCall("doBeep", "vi", nil, p_number_of_beeps);
    return true;
#endif /* MCSystemBeepAndroid */
}

bool MCSystemVibrate (int32_t p_number_of_vibrates)
{
#ifdef /* MCSystemVibrateAndroid */ LEGACY_EXEC
    MCAndroidEngineRemoteCall("doVibrate", "vi", nil, p_number_of_vibrates);
    return true;
#endif /* MCSystemVibrateAndroid */
}
