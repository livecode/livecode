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


#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "chunk.h"
#include "scriptpt.h"
#include "eventqueue.h"
#include "redraw.h"
#include "mbldc.h"
#include "text.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIGraphics.h>
#import <UIKit/UIImage.h>
#import <UIKit/UIImagePickerController.h>
#import <UIKit/UIAccelerometer.h>
// HC-2011-10-12 [[ Media Picker ]] Included relevant library.
#import <MediaPlayer/MPMediaPickerController.h>
#import <MessageUI/MessageUI.h>

// HC-2011-10-12 [[ Media Picker ]] Included relevant library.
#include "mbliphonecontrol.h"
#include "mbliphone.h"
#include "mbliphoneview.h"

#include "mblstore.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

// AL-2014-09-22: [[ Bug 13426 ]] Don't use bit-shifted values for orientation state enum
MCOrientation get_orientation(UIInterfaceOrientation p_orientation)
{
	switch (p_orientation)
	{
	case UIInterfaceOrientationPortrait:
		return ORIENTATION_PORTRAIT_BIT;
	case UIInterfaceOrientationPortraitUpsideDown:
		return ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT;
	case UIInterfaceOrientationLandscapeLeft:
		return ORIENTATION_LANDSCAPE_LEFT_BIT;
	case UIInterfaceOrientationLandscapeRight:
		return ORIENTATION_LANDSCAPE_RIGHT_BIT;
    case UIDeviceOrientationUnknown:
    default:
        return ORIENTATION_UNKNOWN_BIT;
    }
}

uint32_t get_orientation_set(uint32_t p_orientations)
{
	uint32_t t_orientations = 0;

	if (p_orientations & (1 << UIInterfaceOrientationPortrait))
		t_orientations |= ORIENTATION_PORTRAIT;
	if (p_orientations & (1 << UIInterfaceOrientationPortraitUpsideDown))
		t_orientations |= ORIENTATION_PORTRAIT_UPSIDE_DOWN;
	if (p_orientations & (1 << UIInterfaceOrientationLandscapeRight))
		t_orientations |= ORIENTATION_LANDSCAPE_RIGHT;
	if (p_orientations & (1 << UIInterfaceOrientationLandscapeLeft))
		t_orientations |= ORIENTATION_LANDSCAPE_LEFT;

	return t_orientations;
}

uint32_t get_iphone_orientations(uint32_t p_orientations)
{
	uint32_t t_orientations = 0;
	if (p_orientations & ORIENTATION_PORTRAIT)
		t_orientations |= 1 << UIInterfaceOrientationPortrait;
	if (p_orientations & ORIENTATION_PORTRAIT_UPSIDE_DOWN)
		t_orientations |= 1 << UIInterfaceOrientationPortraitUpsideDown;
	if (p_orientations & ORIENTATION_LANDSCAPE_RIGHT)
		t_orientations |= 1 << UIInterfaceOrientationLandscapeRight;
	if (p_orientations & ORIENTATION_LANDSCAPE_LEFT)
		t_orientations |= 1 << UIInterfaceOrientationLandscapeLeft;

	return t_orientations;
}

void MCSystemGetDeviceOrientation(MCOrientation& r_orientation)
{
	r_orientation = get_orientation((UIInterfaceOrientation)[[UIDevice currentDevice] orientation]);
}

void MCSystemGetOrientation(MCOrientation& r_orientation)
{
	r_orientation = get_orientation(MCIPhoneGetOrientation());
}

void MCSystemGetAllowedOrientations(uint32_t& r_orientations)
{
	r_orientations = get_orientation_set([MCIPhoneGetApplication() allowedOrientations]);
}

void MCSystemSetAllowedOrientations(uint32_t p_orientations)
{
	[MCIPhoneGetApplication() setAllowedOrientations: get_iphone_orientations(p_orientations)];
}

void MCSystemGetOrientationLocked(bool& r_locked)
{
	r_locked = (int)[MCIPhoneGetApplication() orientationLocked] == YES;
}

void MCSystemLockOrientation()
{
	[MCIPhoneGetApplication() lockOrientation];
}

void MCSystemUnlockOrientation()
{
	[MCIPhoneGetApplication() unlockOrientation];
}
