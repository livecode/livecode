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


#include "globals.h"
#include "stack.h"
#include "system.h"
#include "player.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include "mblandroidjava.h"
#include "mblsyntax.h"

#include <jni.h>
#include "mblandroidjava.h"

#include "mblad.h"

///////////////////////////////////////////////////////////////////////////////

MCCameraFeaturesType MCSystemGetSpecificCameraFeatures(MCCameraSourceType p_source)
{
    MCAutoStringRef t_camera_dir;
	uindex_t t_offset = 0;
    
	MCAndroidEngineCall("getCameraDirections", "x", &(&t_camera_dir));
    
    bool t_cam_exists = false;

    if (p_source == kMCCameraSourceTypeFront)
        t_cam_exists = MCStringFirstIndexOfChar(*t_camera_dir, 'f', 0, kMCCompareCaseless, t_offset);
    else if (p_source == kMCCameraSourceTypeRear)
        t_cam_exists = MCStringFirstIndexOfChar(*t_camera_dir, 'b', 0, kMCCompareCaseless, t_offset);
    
    if (t_cam_exists)
        return kMCCameraFeaturePhoto;
    
    return 0;
}

MCCamerasFeaturesType MCSystemGetAllCameraFeatures()
{
    MCAutoStringRef t_camera_dir;
	uindex_t t_offset = 0;
    
	MCAndroidEngineCall("getCameraDirections", "x", &(&t_camera_dir));
    
    uint32_t t_features;
    t_features = 0;
    
    if (MCStringFirstIndexOfChar(*t_camera_dir, 'f', 0, kMCCompareCaseless, t_offset))
        t_features |= kMCCamerasFeatureFrontPhoto;
    if (MCStringFirstIndexOfChar(*t_camera_dir, 'b', 0, kMCCompareCaseless, t_offset))
        t_features |= kMCCamerasFeatureRearPhoto;
    
    return t_features;
}

extern bool MCAndroidCheckRuntimePermission(MCStringRef p_permission);
bool MCAndroidPickPhoto(const char *p_source, int32_t p_max_width, int32_t p_max_height)
{
    if (!MCAndroidCheckRuntimePermission(MCSTR("android.permission.CAMERA")))
        return false;
    
    MCAndroidEngineCall("showPhotoPicker", "vsiii", nil, p_source, p_max_width, p_max_height, MCjpegquality);
    // SN-2014-09-03: [[ Bug 13329 ]] MCAndroidPickPhoto's return value is ignored in 6.x,
    // but not in 7.0 - whence the failure in mobilePickPhoto
    return true;
}

static char *s_pick_photo_data = nil;
static uint32_t s_pick_photo_size = 0;
static char *s_pick_photo_err = nil;
static bool s_pick_photo_returned = false;

static const char *MCPhotoSourceTypeToCString(MCPhotoSourceType p_source)
{
    switch (p_source)
    {
        case kMCPhotoSourceTypeLibrary:
            return "library";
        case kMCPhotoSourceTypeAlbum:
            return "album";
        case kMCPhotoSourceTypeCamera:
			return "camera";
		case kMCPhotoSourceTypeRearCamera:
            return "rear camera";
		case kMCPhotoSourceTypeFrontCamera:
			return "front camera";
		default:
            MCUnreachableReturn("unknown");
    }

}

bool MCSystemAcquirePhoto(MCPhotoSourceType p_source, int32_t p_max_width, int32_t p_max_height, void*& r_image_data, size_t& r_image_data_size, MCStringRef& r_result)
{
    // SN-2014-09-03: [[ Bug 13329 ]] We need to initialise the photo_returned variable
    s_pick_photo_returned = false;
    
	if (!MCAndroidPickPhoto(MCPhotoSourceTypeToCString(p_source), p_max_width, p_max_height))
        return false;
    
	while (!s_pick_photo_returned)
		MCscreen->wait(60.0, False, True);

	if (s_pick_photo_data != nil)
	{
		r_image_data = (void *)s_pick_photo_data;
        r_image_data_size = s_pick_photo_size;
	}
	else
	{
        if (s_pick_photo_err != nil)
        {
			/* UNCHECKED */ MCStringCreateWithCString(s_pick_photo_err, r_result);
			MCCStringFree(s_pick_photo_err);
			s_pick_photo_err = nil;
        }
        else
        /* UNCHECKED */MCStringCreateWithCString("cancel", r_result);
	}
    
	return true;
}

bool MCSystemCanAcquirePhoto(MCPhotoSourceType p_source)
{
    if (p_source == kMCPhotoSourceTypeUnknown)
        return false;
    
    return true;
}

void MCAndroidPhotoPickDone(const char *p_data, uint32_t p_size)
{
	if (s_pick_photo_data != nil)
	{
		MCMemoryDeallocate(s_pick_photo_data);
		s_pick_photo_data = nil;
	}
    
	if (p_data != nil)
	{
		MCMemoryAllocateCopy(p_data, p_size, (void*&)s_pick_photo_data);
		s_pick_photo_size = p_size;
	}
	s_pick_photo_returned = true;
}

void MCAndroidPhotoPickError(const char *p_error)
{
	if (s_pick_photo_data != nil)
	{
		MCMemoryDeallocate(s_pick_photo_data);
		s_pick_photo_data = nil;
	}
	if (s_pick_photo_err != nil)
		MCCStringFree(s_pick_photo_err);
	MCCStringClone(p_error, s_pick_photo_err);
	s_pick_photo_returned = true;
}

void MCAndroidPhotoPickCanceled()
{
	MCAndroidPhotoPickError("cancel");
}
