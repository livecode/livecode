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

#include "system.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

//#include "execpt.h"
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
	kMCDisplayOrientationUnknown,
	kMCDisplayOrientationPortrait,
	kMCDisplayOrientationPortraitUpsideDown,
	kMCDisplayOrientationLandscapeRight,
	kMCDisplayOrientationLandscapeLeft,
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

static const char *android_display_rotation_to_string(MCAndroidDisplayFormat p_format, int p_rotation)
{
	if (p_format == kMCDisplayFormatUnknown)
		return "unknown";
	return s_orientation_names[android_display_orientation_from_rotation(p_format, p_rotation)];
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

static const char *android_device_rotation_to_string(MCAndroidDisplayFormat p_device_format, int p_rotation)
{
	if (p_device_format == kMCDisplayFormatUnknown)
		return "unknown";
	if (p_rotation == -1)
		return "face up";

	return s_orientation_names[android_device_orientation_from_rotation(p_device_format, p_rotation)];
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

        // SN-2014-08-05: [[ Bug 13057 ]] The orientations got from the android_device_orientation_from_rotation
        //  are [0;3], not [1;4] as in the orientations enum
		if ((1 << (int)(1 + t_dev_orientation)) & s_allowed_orientations)
			android_set_display_orientation(t_dev_orientation);
	}
};

void MCAndroidOrientationChanged(int orientation)
{
//	MCLog("MCAndroidOrientationChanged(%d)", orientation);
	MCCustomEvent *t_orientation_event = new MCOrientationChangedEvent();
	MCEventQueuePostCustom(t_orientation_event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MCOrientation get_orientation(MCAndroidDisplayOrientation p_orientation)
{
	switch (p_orientation)
	{
	case kMCDisplayOrientationPortrait:
		return ORIENTATION_PORTRAIT;
	case kMCDisplayOrientationPortraitUpsideDown:
		return ORIENTATION_PORTRAIT_UPSIDE_DOWN;
	case kMCDisplayOrientationLandscapeRight:
		return ORIENTATION_LANDSCAPE_RIGHT;
	case kMCDisplayOrientationLandscapeLeft:
		return ORIENTATION_LANDSCAPE_LEFT;
	case kMCDisplayOrientationFaceUp:
		return ORIENTATION_FACE_UP;
	default:
		return ORIENTATION_UNKNOWN;
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
}

void MCSystemGetOrientation(MCOrientation& r_orientation)
{
	int32_t t_rotation;
	MCAndroidDisplayFormat t_format;

	t_rotation = android_get_display_rotation();
	t_format = android_get_display_format();

	if (t_format == kMCDisplayFormatUnknown)
		r_orientation = ORIENTATION_UNKNOWN;
	else
		r_orientation = get_orientation(android_display_orientation_from_rotation(t_format, t_rotation));
}

void MCSystemGetDeviceOrientation(MCOrientation& r_orientation)
{
	int32_t t_dev_rotation;
	MCAndroidDisplayFormat t_dev_format;

	t_dev_rotation = android_get_device_rotation();
	t_dev_format = android_get_device_format();

	r_orientation = get_orientation(android_display_orientation_from_rotation(t_dev_format, t_dev_rotation));
}

void MCSystemLockOrientation()
{
#ifdef /* MCHandleLockOrientation */ LEGACY_EXEC
	if (s_orientation_lock < MAXUINT4)
		s_orientation_lock++;
	return ES_NORMAL;
#endif /* MCHandleLockOrientation */
	if (s_orientation_lock < MAXUINT4)
		s_orientation_lock++;
}

void MCSystemUnlockOrientation()
{
#ifdef /* MCHandleUnlockOrientation */ LEGACY_EXEC
	if (s_orientation_lock > 0)
		s_orientation_lock--;
	return ES_NORMAL;
#endif /* MCHandleUnlockOrientation */
	if (s_orientation_lock > 0)
		s_orientation_lock--;
}

void MCSystemGetOrientationLocked(bool &r_locked)
{
	r_locked = s_orientation_lock > 0;
}
