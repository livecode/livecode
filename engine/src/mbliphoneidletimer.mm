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

////////////////////////////////////////////////////////////////////////////////

// global counter for the iPhone idle timer
uint g_idle_timer = 0;



void MCSystemLockIdleTimer(void)
{
	g_idle_timer++;
	if (g_idle_timer == 1)
		[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

void MCSystemUnlockIdleTimer(void)
{
	if (g_idle_timer == 1)
		[[UIApplication sharedApplication] setIdleTimerDisabled:NO];
	if (g_idle_timer > 0)
		g_idle_timer--;
}

bool MCSystemIdleTimerLocked(void)
{
	return ([[UIApplication sharedApplication] isIdleTimerDisabled] == YES);
}

