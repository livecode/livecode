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

#include "execpt.h"
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

MCOrientation get_orientation(UIInterfaceOrientation p_orientation)
{
	switch (p_orientation)
	{
	case UIInterfaceOrientationPortrait:
		return ORIENTATION_PORTRAIT;
	case UIInterfaceOrientationPortraitUpsideDown:
		return ORIENTATION_PORTRAIT_UPSIDE_DOWN;
	case UIInterfaceOrientationLandscapeLeft:
		return ORIENTATION_LANDSCAPE_LEFT;
	case UIInterfaceOrientationLandscapeRight:
		return ORIENTATION_LANDSCAPE_RIGHT;
    }
}

MCOrientationSet get_orientation_set(uint32_t p_orientations)
{
	//UNIMPLEMENTED
	return ORIENTATION_UNKNOWN_BIT;
}

uint32_t get_iphone_orientation(MCOrientationSet p_orientations)
{
	//UNIMPLEMENTED
	return 0;
}

void MCSystemGetDeviceOrientation(MCOrientation& r_orientation)
{
	r_orientation = get_orientation([[UIDevice currentDevice] orientation]);
}

void MCSystemGetOrientation(MCOrientation& r_orientation)
{
	r_orientation = get_orientation(MCIPhoneGetOrientation());
}

void MCSystemGetAllowedOrientations(MCOrientationSet& r_orientations)
{
	r_orientations = get_orientation_set([MCIPhoneGetApplication() allowedOrientations]);
}

void MCSystemSetAllowedOrientations(MCOrientationSet p_orientations)
{
	[MCIPhoneGetApplication() setAllowedOrientations: get_iphone_orientation(p_orientations)];
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