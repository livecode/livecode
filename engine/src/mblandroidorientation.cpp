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

#include "system.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "debug.h"

#include "mblandroid.h"
#include "mblandroidutil.h"
#include "mblsyntax.h"

extern MCAndroidDeviceConfiguration s_device_configuration;

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	kMCDisplayFormatUnknown,
	kMCDisplayFormatPortrait,
	kMCDisplayFormatLandscape,
	kMCDisplayFormatSquare,
} MCAndroidDisplayFormat;

typedef enum
{
    // AL-2014-10-24 : [[ Bug 13780 ]] Make sure Android orientation enum matches the orientation map
	kMCDisplayOrientationPortrait,
    kMCDisplayOrientationLandscapeLeft,
	kMCDisplayOrientationPortraitUpsideDown,
	kMCDisplayOrientationLandscapeRight,
	kMCDisplayOrientationFaceUp,
} MCAndroidDisplayOrientation;

static uint32_t s_allowed_orientations = 0;
static uint32_t s_orientation_lock = 0;

static const char *s_orientation_names[] = {
	"portrait",
	"landscape left",
	"portrait upside down",
	"landscape right",
	""
//	"face up",
};

////////////////////////////////////////////////////////////////////////////////

static MCAndroidDisplayFormat android_get_display_format()
{
	int32_t t_display_format = 0;
	MCAndroidEngineCall("getDisplayOrientation", "i", &t_display_format);
//	MCLog("getDisplayOrientation() == %d", t_display_format);
	return (MCAndroidDisplayFormat)t_display_format;
}

static int android_get_display_rotation()
{
	int32_t t_display_rotation = 0;
	MCAndroidEngineCall("getDisplayRotation", "i", &t_display_rotation);
//	MCLog("getDisplayRotation() == %d", t_display_rotation);
	return t_display_rotation;
}

static int android_get_device_rotation()
{
	int32_t t_dev_rotation = 0;
	MCAndroidEngineCall("getDeviceRotation", "i", &t_dev_rotation);
//	MCLog("getDeviceRotation() == %d", t_dev_rotation);
	return t_dev_rotation;
}

static void android_set_display_orientation(MCAndroidDisplayOrientation p_orientation)
{
//	MCLog("setDisplayOrientation(%d)", p_orientation);
	MCAndroidEngineCall("setDisplayOrientation", "vi", nil, (int)p_orientation);
}

////////////////////////////////////////////////////////////////////////////////

// natural display format of device
static MCAndroidDisplayFormat android_device_format_from_rotation(MCAndroidDisplayFormat p_screen_format, int p_rotation)
{
	if (p_rotation == 0 || p_rotation == 180)
		return p_screen_format;

	switch (p_screen_format)
	{
	case kMCDisplayFormatPortrait:
		return kMCDisplayFormatLandscape;
	case kMCDisplayFormatLandscape:
		return kMCDisplayFormatPortrait;
	default:
		return p_screen_format;
	}
}

static MCAndroidDisplayFormat android_get_device_format()
{
	int32_t t_ui_rotation;
	MCAndroidDisplayFormat t_ui_format;

	t_ui_rotation = android_get_display_rotation();
	t_ui_format = android_get_display_format();

	return android_device_format_from_rotation(t_ui_format, t_ui_rotation);
}

////////////////////////////////////////////////////////////////////////////////

static MCAndroidDisplayOrientation s_portrait_display_map[] = {
	kMCDisplayOrientationPortrait,
	kMCDisplayOrientationPortrait,
	kMCDisplayOrientationPortraitUpsideDown,
	kMCDisplayOrientationPortraitUpsideDown,
};
static MCAndroidDisplayOrientation s_landscape_display_map[] = {
	kMCDisplayOrientationLandscapeRight,
	kMCDisplayOrientationLandscapeLeft,
	kMCDisplayOrientationLandscapeLeft,
	kMCDisplayOrientationLandscapeRight
};

static MCAndroidDisplayOrientation android_display_orientation_from_rotation(MCAndroidDisplayFormat p_format, int p_rotation)
{
	if (s_device_configuration.have_orientation_map)
	{
		for (uint32_t i = 0; i < 4; i++)
			if (s_device_configuration.orientation_map[i] == p_rotation)
				return (MCAndroidDisplayOrientation) i;
	}
    
	switch (p_format)
	{
        case kMCDisplayFormatPortrait:
            return s_portrait_display_map[p_rotation / 90];
        case kMCDisplayFormatLandscape:
            return s_landscape_display_map[p_rotation / 90];
        case kMCDisplayFormatSquare:
        case kMCDisplayFormatUnknown:
            return (MCAndroidDisplayOrientation) (p_rotation / 90);
	}
}

static MCAndroidDisplayOrientation android_device_orientation_from_rotation(MCAndroidDisplayFormat p_device_format, int p_rotation)
{
	if (p_rotation == -1)
		return kMCDisplayOrientationFaceUp;

	if (s_device_configuration.have_orientation_map)
	{
		for (uint32_t i = 0; i < 4; i++)
			if (s_device_configuration.orientation_map[i] == p_rotation)
				return (MCAndroidDisplayOrientation) i;
	}

	switch (p_device_format)
	{
	case kMCDisplayFormatPortrait:
	case kMCDisplayFormatSquare:
	case kMCDisplayFormatUnknown:
        return (MCAndroidDisplayOrientation) (p_rotation / 90);
    case kMCDisplayFormatLandscape:
        return (MCAndroidDisplayOrientation) (((270 + p_rotation) % 360) / 90);
	}
}

////////////////////////////////////////////////////////////////////////////////

struct MCOrientationChangedEvent: public MCCustomEvent
{
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message(MCM_orientation_changed);

		if (s_orientation_lock > 0)
			return;

		MCAndroidDisplayFormat t_dev_format = android_get_device_format();
		int t_dev_rotation = android_get_device_rotation();
		MCAndroidDisplayOrientation t_dev_orientation = android_device_orientation_from_rotation(t_dev_format, t_dev_rotation);
        // AL-2014-09-22: [[ Bug 13426 ]] Revert bugfix 13057 now that orientation enum is as before
		if ((1 << (int)(t_dev_orientation)) & s_allowed_orientations)
			android_set_display_orientation(t_dev_orientation);
	}
};

void MCAndroidOrientationChanged(int orientation)
{
//	MCLog("MCAndroidOrientationChanged(%d)", orientation);
	MCCustomEvent *t_orientation_event = new (nothrow) MCOrientationChangedEvent();
	MCEventQueuePostCustom(t_orientation_event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MCOrientation get_orientation(MCAndroidDisplayOrientation p_orientation)
{
    // AL-2014-09-19; [[ Bug 13426 ]] Return the bits for orientation enum, rather than the bit-shifted values
	switch (p_orientation)
	{
	case kMCDisplayOrientationPortrait:
		return ORIENTATION_PORTRAIT_BIT;
	case kMCDisplayOrientationPortraitUpsideDown:
		return ORIENTATION_PORTRAIT_UPSIDE_DOWN_BIT;
	case kMCDisplayOrientationLandscapeRight:
		return ORIENTATION_LANDSCAPE_RIGHT_BIT;
	case kMCDisplayOrientationLandscapeLeft:
		return ORIENTATION_LANDSCAPE_LEFT_BIT;
	case kMCDisplayOrientationFaceUp:
		return ORIENTATION_FACE_UP_BIT;
	default:
		return ORIENTATION_UNKNOWN_BIT;
	}
}

uint32_t get_orientation_set(uint32_t p_orientations)
{
	uint32_t t_orientations = 0;
	if (p_orientations & (1 << kMCDisplayOrientationPortrait))
		t_orientations |= ORIENTATION_PORTRAIT;
	if (p_orientations & (1 << kMCDisplayOrientationPortraitUpsideDown))
		t_orientations |= ORIENTATION_PORTRAIT_UPSIDE_DOWN;
	if (p_orientations & (1 << kMCDisplayOrientationLandscapeRight))
		t_orientations |= ORIENTATION_LANDSCAPE_RIGHT;
	if (p_orientations & (1 << kMCDisplayOrientationLandscapeLeft))
		t_orientations |= ORIENTATION_LANDSCAPE_LEFT;
	if (p_orientations & (1 << kMCDisplayOrientationFaceUp))
		t_orientations |= ORIENTATION_FACE_UP;

	return t_orientations;
}

uint32_t get_android_orientations(uint32_t p_orientations)
{
	uint32_t t_orientations = 0;
	if (p_orientations & (ORIENTATION_PORTRAIT))
		t_orientations |= 1 << kMCDisplayOrientationPortrait;
	if (p_orientations & (ORIENTATION_PORTRAIT_UPSIDE_DOWN))
		t_orientations |= 1 << kMCDisplayOrientationPortraitUpsideDown;
	if (p_orientations & (ORIENTATION_LANDSCAPE_RIGHT))
		t_orientations |= 1 << kMCDisplayOrientationLandscapeRight;
	if (p_orientations & (ORIENTATION_LANDSCAPE_LEFT))
		t_orientations |= 1 << kMCDisplayOrientationLandscapeLeft;
	if (p_orientations & (ORIENTATION_FACE_UP))
		t_orientations |= 1 << kMCDisplayOrientationFaceUp;

	return t_orientations;
}

void MCSystemGetAllowedOrientations(uint32_t& r_orientations)
{
	r_orientations = get_orientation_set(s_allowed_orientations);
}

void MCSystemSetAllowedOrientations(uint32_t p_orientations)
{
	s_allowed_orientations = get_android_orientations(p_orientations);
    
    MCAndroidDisplayFormat t_dev_format = android_get_device_format();
    int t_dev_rotation = android_get_device_rotation();
    MCAndroidDisplayOrientation t_dev_orientation = android_device_orientation_from_rotation(t_dev_format, t_dev_rotation);
    MCAndroidDisplayOrientation t_new_orientation = t_dev_orientation;
    
    if (!((1 << (int)(t_new_orientation)) & s_allowed_orientations))
    {
        // check landscape v portrait first
        if (t_dev_orientation == kMCDisplayOrientationLandscapeLeft ||
            t_dev_orientation == kMCDisplayOrientationLandscapeRight)
        {
            if ((1 << (int)(kMCDisplayOrientationLandscapeLeft)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationLandscapeLeft;
            }
            else if ((1 << (int)(kMCDisplayOrientationLandscapeRight)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationLandscapeRight;
            }
        }
        else if (t_dev_orientation == kMCDisplayOrientationPortrait ||
            t_dev_orientation == kMCDisplayOrientationPortraitUpsideDown)
        {
            if ((1 << (int)(kMCDisplayOrientationPortrait)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationPortrait;
            }
            else if ((1 << (int)(kMCDisplayOrientationPortraitUpsideDown)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationPortraitUpsideDown;
            }
        }
        
        // if we have not found an appropriate orientation then try them in order
        if (t_new_orientation == t_dev_orientation)
        {
            if ((1 << (int)(kMCDisplayOrientationPortrait)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationPortrait;
            }
            else if ((1 << (int)(kMCDisplayOrientationLandscapeLeft)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationLandscapeLeft;
            }
            else if ((1 << (int)(kMCDisplayOrientationLandscapeRight)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationLandscapeRight;
            }
            else if ((1 << (int)(kMCDisplayOrientationPortraitUpsideDown)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationPortraitUpsideDown;
            }
            else if ((1 << (int)(kMCDisplayOrientationFaceUp)) & s_allowed_orientations)
            {
                t_new_orientation = kMCDisplayOrientationFaceUp;
            }
        }
    }
    
    // set orientation regardless of whether we have changed it because
    // it may have already been in a supported orientation but not
    // set to it because it was previouslu not allowed
    android_set_display_orientation(t_new_orientation);
}

void MCSystemGetOrientation(MCOrientation& r_orientation)
{
	int32_t t_rotation;
	MCAndroidDisplayFormat t_format;

	t_rotation = android_get_display_rotation();
	t_format = android_get_display_format();

	if (t_format == kMCDisplayFormatUnknown)
		r_orientation = ORIENTATION_UNKNOWN_BIT;
	else
		r_orientation = get_orientation(android_display_orientation_from_rotation(t_format, t_rotation));
}

void MCSystemGetDeviceOrientation(MCOrientation& r_orientation)
{
	int32_t t_dev_rotation;
	MCAndroidDisplayFormat t_dev_format;

	t_dev_rotation = android_get_device_rotation();
	t_dev_format = android_get_device_format();

	r_orientation = get_orientation(android_device_orientation_from_rotation(t_dev_format, t_dev_rotation));
}

void MCSystemLockOrientation()
{
	if (s_orientation_lock < MAXUINT4)
		s_orientation_lock++;
}

void MCSystemUnlockOrientation()
{
	if (s_orientation_lock > 0)
		s_orientation_lock--;
}

void MCSystemGetOrientationLocked(bool &r_locked)
{
	r_locked = s_orientation_lock > 0;
}
