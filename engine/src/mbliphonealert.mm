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

#include "uidc.h"
#include "execpt.h"
#include "globals.h"

#include "exec.h"
#include "mblsyntax.h"

#include "mbliphoneapp.h"

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import <AudioToolbox/AudioServices.h>

// We do not need this in iOS, as beep is already implemented and handled.
bool MCSystemBeep (int32_t p_number_of_beeps)
{
#ifdef /* MCSystemBeepIphone */ LEGACY_EXEC
    return true;
#endif /* MCSystemBeepIphone */ LEGACY_EXEC
}

bool MCSystemVibrate (int32_t p_number_of_vibrates)
{
#ifdef /* MCSystemVibrateIphone */ LEGACY_EXEC
    for (int32_t i = 0; i < p_number_of_vibrates; i++)
    {
		// MW-2012-08-06: [[ Fibers ]] Invoke the system call on the main fiber.
		MCIPhoneRunBlockOnMainFiber(^(void) {
			AudioServicesPlayAlertSound(kSystemSoundID_Vibrate); // Vibrates and beeps if no vibrate is supported
		});
		MCscreen->wait(BEEP_INTERVAL, False, False);
    }
    return true;
#endif /* MCSystemVibrateIphone */ LEGACY_EXEC
}
