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

#ifndef __MOBILE_SENSOR__
#define __MOBILE_SENSOR__

#include "mblsyntax.h"

typedef struct
{
    double latitude;
    double longitude;
    double altitude;
    
    // detailed
    double timestamp;
    double horizontal_accuracy;
    double vertical_accuracy;
    
    // MM-2013-02-21: Add speed and course to detailed location reading.
    double speed;
    double course;
} MCSensorLocationReading;

typedef struct
{
    double heading;
    
    // detailed
    double magnetic_heading;
    double true_heading;
    double timestamp;
    double x, y, z;
    double accuracy;
} MCSensorHeadingReading;

typedef struct
{
    double x, y, z;
    
    // detailed
    double timestamp;
} MCSensorAccelerationReading;

typedef struct
{
    double x, y, z;
    
    // detailed
    double timestamp;
} MCSensorRotationRateReading;

void MCSensorPostChangeMessage(MCSensorType p_sensor);
void MCSensorPostErrorMessage(MCSensorType p_sensor, const char *p_error);

// MM-2012-03-13: Added intialize and finalize calls to sensor module.
void MCSensorInitialize(void);
void MCSensorFinalize(void);


#endif
