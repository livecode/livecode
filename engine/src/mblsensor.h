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

struct MCSensorLocationReading
{
    MCSensorLocationReading *next;
    
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
};

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

////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingLocation(bool p_loosely);
bool MCSystemStopTrackingLocation();

bool MCSystemStartTrackingHeading(bool p_loosely);
bool MCSystemStopTrackingHeading();

bool MCSystemStartTrackingAcceleration(bool p_loosely);
bool MCSystemStopTrackingAcceleration();

bool MCSystemStartTrackingRotationRate(bool p_loosely);
bool MCSystemStopTrackingRotationRate();

bool MCSystemGetSensorAvailable(MCSensorType p_sensor, bool& r_available);

bool MCSystemStartTrackingSensor(MCSensorType p_sensor, bool p_loosely);
bool MCSystemStopTrackingSensor(MCSensorType p_sensor);

double MCSystemGetSensorDispatchThreshold(MCSensorType p_sensor);

bool MCSystemGetLocationReading(MCSensorLocationReading &r_reading, bool p_detailed);
bool MCSystemGetHeadingReading(MCSensorHeadingReading &r_reading, bool p_detailed);
bool MCSystemGetAccelerationReading(MCSensorAccelerationReading &r_reading, bool p_detailed);
bool MCSystemGetRotationRateReading(MCSensorRotationRateReading &r_reading, bool p_detailed);

// MM-2012-02-11: Added support for iPhoneGet/SetCalibrationTimeout
bool MCSystemGetLocationCalibrationTimeout(int32_t&);
bool MCSystemSetLocationCalibrationTimeout(int32_t);

void MCSystemSensorInitialize(void);
void MCSystemSensorFinalize(void);

// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
bool MCSystemGetLocationAuthorizationStatus(MCStringRef& r_status);

void MCSensorAddLocationSample(const MCSensorLocationReading& p_reading);
bool MCSensorPopLocationSample(MCSensorLocationReading& r_reading);
size_t MCSensorGetLocationSampleLimit(void);
void MCSensorSetLocationSampleLimit(size_t p_limit);

////////////////////////////////////////////////////////////////////////////////

void MCSensorPostChangeMessage(MCSensorType p_sensor);
void MCSensorPostErrorMessage(MCSensorType p_sensor, MCStringRef p_error);

// MM-2012-03-13: Added intialize and finalize calls to sensor module.
void MCSensorInitialize(void);
void MCSensorFinalize(void);

bool MCSensorTypeToCString(MCSensorType p_sensor, char *&r_string);
MCSensorType MCSensorTypeFromString(MCStringRef p_string);

#endif
