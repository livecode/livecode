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

#include "mcerror.h"
#include "execpt.h"
#include "printer.h"
#include "globals.h"

#include "mbldc.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblsyntax.h"

///////////////////////////////////////////////////////////////////////////////

MCCameraFeaturesType MCSystemGetSpecificCameraFeatures(MCCameraSourceType p_source)
{
    MCAutoStringRef t_camera_dir;
	uindex_t t_offset = 0;
    
	MCAndroidEngineCall("getCameraDirections", "x", &t_camera_dir);
    
    bool t_cam_exists = false;

    if (p_source == kMCCameraSourceTypeFront)
        t_cam_exists = MCStringFirstIndexOfChar(*t_camera_dir, 'f', 0, kMCCompareCaseless, t_offset);
    else if (p_source == kMCCAmeraSourceTypeRear)
        t_cam_exists = MCStringFirstIndexOfChar(*t_camera_dir, 'b', 0, kMCCompareCaseless, t_offset);
    
    if (t_cam_exists)
        return kMCCameraFeaturePhoto;
    
    return 0;
}

MCCamerasFeaturesType MCSystemGetAllCameraFeatures()
{
    MCAutoStringRef t_camera_dir;
	uindex_t t_offset = 0;
    
	MCAndroidEngineCall("getCameraDirections", "x", &t_camera_dir);
    
    uint32_t t_features;
    t_features = 0;
    
    if (MCStringFirstIndexOfChar(*t_camera_dir, 'f', 0, kMCCompareCaseless, t_offset))
        t_features |= kMCCamerasFeatureFrontPhoto;
    if (MCStringFirstIndexOfChar(*t_camera_dir, 'b', 0, kMCCompareCaseless, t_offset))
        t_features |= kMCCAmerasFeatureRearPhoto;
    
    return t_features;
}
