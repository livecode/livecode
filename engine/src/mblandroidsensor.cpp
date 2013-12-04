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
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "object.h"
#include "mbldc.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include "mblsyntax.h"
#include "mblsensor.h"

#include <jni.h>

static MCSensorAccelerationReading *s_acceleration_reading = nil;
static MCSensorLocationReading *s_location_reading = nil;
static MCSensorHeadingReading *s_heading_reading = nil;
static MCSensorRotationRateReading *s_rotation_rate_reading = nil;

////////////////////////////////////////////////////////////////////////////////

// MM-2012-03-13: Added intialize and finalize calls to sensor module.
void MCSystemSensorInitialize(void)
{
    s_acceleration_reading = nil;
    s_location_reading = nil;
    s_heading_reading = nil;
    s_rotation_rate_reading = nil;
}

void MCSystemSensorFinalize(void)
{
    /* UNCHECKED */ MCMemoryDelete(s_acceleration_reading);
    /* UNCHECKED */ MCMemoryDelete(s_location_reading);
    /* UNCHECKED */ MCMemoryDelete(s_heading_reading);
    /* UNCHECKED */ MCMemoryDelete(s_rotation_rate_reading);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidStartTrackingLocation(bool p_loosely);
bool MCAndroidStopTrackingLocation();

bool MCAndroidStartTrackingHeading(bool p_loosely);
bool MCAndroidStopTrackingHeading();

bool MCAndroidStartTrackingAcceleration(bool p_loosely);
bool MCAndroidStopTrackingAcceleration();

bool MCAndroidStartTrackingRotationRate(bool p_loosely);
bool MCAndroidStopTrackingRotationRate();

////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetSensorAvailable(MCSensorType p_sensor, bool& r_available)
{    
    MCAndroidEngineRemoteCall("isSensorAvailable", "bi", &r_available, (int32_t)p_sensor);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingSensor(MCSensorType p_sensor, bool p_loosely)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return MCAndroidStartTrackingLocation(p_loosely);
        case kMCSensorTypeHeading:
            return MCAndroidStartTrackingHeading(p_loosely);
        case kMCSensorTypeAcceleration:
            return MCAndroidStartTrackingAcceleration(p_loosely);
        case kMCSensorTypeRotationRate:
            return MCAndroidStartTrackingRotationRate(p_loosely);
            
        default:
            // unknown
            return false;
    }
}

bool MCSystemStopTrackingSensor(MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return MCAndroidStopTrackingLocation();
        case kMCSensorTypeHeading:
            return MCAndroidStopTrackingHeading();
        case kMCSensorTypeAcceleration:
            return MCAndroidStopTrackingAcceleration();
        case kMCSensorTypeRotationRate:
            return MCAndroidStopTrackingRotationRate();
            
        default:
            // unknown
            return false;
    }
}

bool MCSystemGetLocationReading(MCSensorLocationReading &r_reading, bool p_detailed)
{
    if (s_location_reading == nil)
        return false;
    
    MCMemoryCopy(&r_reading, s_location_reading, sizeof(MCSensorLocationReading));
    return true;
}

bool MCSystemGetHeadingReading(MCSensorHeadingReading &r_reading, bool p_detailed)
{
    if (s_heading_reading == nil)
        return false;
    
    MCMemoryCopy(&r_reading, s_heading_reading, sizeof(MCSensorHeadingReading));
    return true;
}

bool MCSystemGetAccelerationReading(MCSensorAccelerationReading &r_reading, bool p_detailed)
{
    if (s_acceleration_reading == nil)
        return false;
    
    MCMemoryCopy(&r_reading, s_acceleration_reading, sizeof(MCSensorAccelerationReading));
    return true;
}

bool MCSystemGetRotationRateReading(MCSensorRotationRateReading &r_reading, bool p_detailed)
{
    if (s_rotation_rate_reading == nil)
        return false;
    
    MCMemoryCopy(&r_reading, s_rotation_rate_reading, sizeof(MCSensorRotationRateReading));
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidStartTrackingLocation(bool p_loosely)
{
    bool t_success = true;
        
    if (t_success)
        MCAndroidEngineRemoteCall("startTrackingLocation", "bb", &t_success, p_loosely);
    
    return t_success;
}

bool MCAndroidStopTrackingLocation()
{
    bool t_success = true;
    
    MCAndroidEngineRemoteCall("stopTrackingLocation", "b", &t_success);
    return t_success;
}

bool MCAndroidStartTrackingHeading(bool p_loosely)
{
    bool t_success = true;
    
    if (s_heading_reading == nil)
        t_success = MCMemoryNew(s_heading_reading);
    
    if (t_success)
        MCAndroidEngineRemoteCall("startTrackingHeading", "bb", &t_success, p_loosely);
    
    return t_success;
}

bool MCAndroidStopTrackingHeading()
{
    bool t_success = true;
    
    MCAndroidEngineRemoteCall("stopTrackingHeading", "b", &t_success);
    return t_success;
}

bool MCAndroidStartTrackingAcceleration(bool p_loosely)
{
    bool t_success = true;
    
    if (t_success)
        MCAndroidEngineRemoteCall("startTrackingAcceleration", "bb", &t_success, p_loosely);
    
    return t_success;
}

bool MCAndroidStopTrackingAcceleration()
{
    bool t_success = true;
    
    MCAndroidEngineRemoteCall("stopTrackingAcceleration", "b", &t_success);
    return t_success;
}

bool MCAndroidStartTrackingRotationRate(bool p_loosely)
{
    bool t_success = true;
        
    if (t_success)
        MCAndroidEngineRemoteCall("startTrackingRotationRate", "bb", &t_success, p_loosely);
    
    return t_success;
}

bool MCAndroidStopTrackingRotationRate()
{
    bool t_success = true;
    
    MCAndroidEngineRemoteCall("stopTrackingRotationRate", "b", &t_success);
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAccelerationChanged(JNIEnv *env, jobject object, jfloat x, jfloat y, jfloat z, jfloat timestamp) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doAccelerationChanged(JNIEnv *env, jobject object, jfloat x, jfloat y, jfloat z, jfloat timestamp)
{
    // MM-2012-03-13: Create first reading value only when we get a callback.  
    //     This means we can properly handle the case where the user requests a reading before one has been taken.
    if (s_acceleration_reading == nil)
        if (!MCMemoryNew(s_acceleration_reading))
            return;
    
    s_acceleration_reading->x = x;
    s_acceleration_reading->y = y;
    s_acceleration_reading->z = z;
    s_acceleration_reading->timestamp = timestamp;
    
    MCSensorPostChangeMessage(kMCSensorTypeAcceleration);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLocationChanged(JNIEnv *env, jobject object, jdouble latitude, jdouble longitude, jdouble altitude, jfloat timestamp, jfloat accuracy, jdouble speed, jdouble course) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doLocationChanged(JNIEnv *env, jobject object, jdouble latitude, jdouble longitude, jdouble altitude, jfloat timestamp, jfloat accuracy, jdouble speed, jdouble course)
{
    // MM-2012-03-13: Create first reading value only when we get a callback.  
    //     This means we can properly handle the case where the user requests a reading before one has been taken.
    if (s_location_reading == nil)
        if (!MCMemoryNew(s_location_reading))
            return;
    
    s_location_reading->latitude = latitude;
    s_location_reading->longitude = longitude;
    s_location_reading->altitude = altitude;
    s_location_reading->timestamp = timestamp;
    s_location_reading->horizontal_accuracy = accuracy;
    s_location_reading->vertical_accuracy = 0;
    
    // MM-2013-02-21: Added speed and course to location sensor readings.
    s_location_reading->speed = speed;
    s_location_reading->course = course;
    
    MCSensorPostChangeMessage(kMCSensorTypeLocation);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doHeadingChanged(JNIEnv *env, jobject object, jdouble heading, jdouble magnetic_heading, jdouble true_heading, jfloat timestamp, jfloat x, jfloat y, jfloat z, jfloat accuracy) __attribute__((visibility("default")));

JNIEXPORT void JNICALL JNICALL Java_com_runrev_android_Engine_doHeadingChanged(JNIEnv *env, jobject object, jdouble heading, jdouble magnetic_heading, jdouble true_heading, jfloat timestamp, jfloat x, jfloat y, jfloat z, jfloat accuracy)
{
    // MM-2012-03-13: Create first reading value only when we get a callback.  
    //     This means we can properly handle the case where the user requests a reading before one has been taken.
    if (s_heading_reading == nil)
        if (!MCMemoryNew(s_heading_reading))
            return;

    s_heading_reading->heading = heading;
    s_heading_reading->magnetic_heading = magnetic_heading;
    s_heading_reading->true_heading = true_heading;
    s_heading_reading->timestamp = timestamp;
    s_heading_reading->x = x;
    s_heading_reading->y = y;
    s_heading_reading->z = z;
    s_heading_reading->accuracy = accuracy;
    
    MCSensorPostChangeMessage(kMCSensorTypeHeading);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRotationRateChanged(JNIEnv *env, jobject object, jfloat x, jfloat y, jfloat z, jfloat timestamp) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doRotationRateChanged(JNIEnv *env, jobject object, jfloat x, jfloat y, jfloat z, jfloat timestamp)
{
    // MM-2012-03-13: Create first reading value only when we get a callback.  
    //     This means we can properly handle the case where the user requests a reading before one has been taken.
    if (s_rotation_rate_reading == nil)
        if (!MCMemoryNew(s_rotation_rate_reading))
            return;
    
    s_rotation_rate_reading->x = x;
    s_rotation_rate_reading->y = y;
    s_rotation_rate_reading->z = z;
    s_rotation_rate_reading->timestamp = timestamp;
    
    MCSensorPostChangeMessage(kMCSensorTypeRotationRate);
}

////////////////////////////////////////////////////////////////////////////////

double MCSystemGetSensorDispatchThreshold(MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return 0.00001;
        case kMCSensorTypeHeading:
            return 0.0;
        case kMCSensorTypeAcceleration:
            return 0.04; // 0.01 iPad, 0.04 iPhone 3G
        case kMCSensorTypeRotationRate:
            return 0.05;
    }
    return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// COMPATIBILITY

bool MCSystemSetLocationCalibrationTimeout(int32_t p_timeout)
{
    return true;
}

bool MCSystemGetLocationCalibrationTimeout(int32_t& r_timeout)
{
    return true;
}
