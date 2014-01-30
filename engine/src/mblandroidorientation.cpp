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
#include "system.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
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

extern bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
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
	nil
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

#ifdef /* MCHandleOrientationAndroid */ LEGACY_EXEC
Exec_stat MCHandleOrientation(void *context, MCParameter *p_parameters)
{
	int32_t t_rotation;
	MCAndroidDisplayFormat t_format;

	t_rotation = android_get_display_rotation();
	t_format = android_get_display_format();

//	MCLog("display format %d", t_format);
//	MCLog("display rotation %d", t_rotation);

	MCresult->sets(MCString(android_display_rotation_to_string(t_format, t_rotation)));
	return ES_NORMAL;
}
#endif /* MCHandleOrientationAndroid */

#ifdef /* MCHandleDeviceOrientationAndroid */ LEGACY_EXEC
Exec_stat MCHandleDeviceOrientation(void *context, MCParameter *p_parameters)
{
	int32_t t_dev_rotation;
	MCAndroidDisplayFormat t_dev_format;

	t_dev_rotation = android_get_device_rotation();
	t_dev_format = android_get_device_format();

//	MCLog("device format %d", t_dev_format);
//	MCLog("device rotation %d", t_dev_rotation);

	MCresult->sets(MCString(android_device_rotation_to_string(t_dev_format, t_dev_rotation)));
	return ES_NORMAL;
}
#endif /* MCHandleDeviceOrientationAndroid */

#ifdef /* MCHandleAllowedOrientationsAndroid */ LEGACY_EXEC
Exec_stat MCHandleAllowedOrientations(void *context, MCParameter *p_parameters)
{
	MCExecPoint ep(nil, nil, nil);
	for(uint32_t j = 0; s_orientation_names[j] != nil; j++)
		if ((s_allowed_orientations & (1 << j)) != 0)
			ep . concatcstring(s_orientation_names[j], EC_COMMA, ep . isempty());

	MCresult -> store(ep, True);
	
	return ES_NORMAL;
}
#endif /* MCHandleAllowedOrientationsAndroid */

#ifdef /* MCHandleSetAllowedOrientationsAndroid */ LEGACY_EXEC
Exec_stat MCHandleSetAllowedOrientations(void *context, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_orientations;
	t_orientations = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_orientations);
	
	char **t_orientations_array;
	uint32_t t_orientations_count;
	t_orientations_array = nil;
	t_orientations_count = 0;
	if (t_success)
		t_success = MCCStringSplit(t_orientations, ',', t_orientations_array, t_orientations_count);
	
	uint32_t t_orientations_set;
	t_orientations_set = 0;
	if (t_success)
		for(uint32_t i = 0; i < t_orientations_count; i++)
			for(uint32_t j = 0; s_orientation_names[j] != nil; j++)
				if (MCCStringEqualCaseless(t_orientations_array[i], s_orientation_names[j]))
					t_orientations_set |= (1 << j);
	
	s_allowed_orientations = t_orientations_set;
	
	for(uint32_t i = 0; i < t_orientations_count; i++)
		MCCStringFree(t_orientations_array[i]);
	MCMemoryDeleteArray(t_orientations_array);
	
	MCCStringFree(t_orientations);
	
	return ES_NORMAL;
}
#endif /* MCHandleSetAllowedOrientationsAndroid */

Exec_stat MCHandleLockOrientation(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleLockOrientation */ LEGACY_EXEC
	if (s_orientation_lock < MAXUINT4)
		s_orientation_lock++;
	return ES_NORMAL;
#endif /* MCHandleLockOrientation */
}

Exec_stat MCHandleUnlockOrientation(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleUnlockOrientation */ LEGACY_EXEC
	if (s_orientation_lock > 0)
		s_orientation_lock--;
	return ES_NORMAL;
#endif /* MCHandleUnlockOrientation */
}

#ifdef /* MCHandleOrientationLockedAndroid */ LEGACY_EXEC
Exec_stat MCHandleOrientationLocked(void *context, MCParameter *p_parameters)
{
	MCresult->sets(MCU_btos(s_orientation_lock > 0));
	return ES_NORMAL;
}
#endif /* MCHandleOrientationLockedAndroid */

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

		if ((1 << (int)t_dev_orientation) & s_allowed_orientations)
			android_set_display_orientation(t_dev_orientation);
	}
};

void MCAndroidOrientationChanged(int orientation)
{
//	MCLog("MCAndroidOrientationChanged(%d)", orientation);
	MCCustomEvent *t_orientation_event = new MCOrientationChangedEvent();
	MCEventQueuePostCustom(t_orientation_event);
}
