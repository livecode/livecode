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
#include "globals.h"

#include "exec.h"
#include "mblsyntax.h"
#include "mblsensor.h"

#include <Foundation/NSOperation.h>

#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

////////////////////////////////////////////////////////////////////////////////

// MM-2012-03-13: Added intialize and finalize calls to sensor module.
//  Only really needed for Android.
void MCSystemSensorInitialize(void)
{
}

void MCSystemSensorFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

static NSTimeInterval timestamp_from_time_since_boot(NSTimeInterval p_time_since_boot)
{
    NSDate *t_time;
    t_time = [NSDate date];
    
    NSProcessInfo *t_process_info;
    t_process_info = [NSProcessInfo processInfo];
    
    return [t_time timeIntervalSince1970] - [t_process_info systemUptime] + p_time_since_boot;
}

////////////////////////////////////////////////////////////////////////////////
// CORE MOTION
////////////////////////////////////////////////////////////////////////////////

static CMMotionManager *s_motion_manager = nil;
static bool s_acceleration_enabled = false;
static bool s_rotation_rate_enabled = false;

static void initialize_core_motion(void)
{
	if (s_motion_manager != nil)
		return;
	
	s_motion_manager = [[CMMotionManager alloc] init];
	s_acceleration_enabled = false;
	s_rotation_rate_enabled = false;
}

////////////////////////////////////////////////////////////////////////////////
// CORE LOCATION
////////////////////////////////////////////////////////////////////////////////


@interface MCIPhoneLocationDelegate : NSObject <CLLocationManagerDelegate>
{
	NSTimer *m_calibration_timer;
}
@end

static CLLocationManager *s_location_manager = nil;
static MCIPhoneLocationDelegate *s_location_delegate = nil;
static bool s_location_enabled = false;
static bool s_heading_enabled = false;
static bool s_tracking_heading_loosely = false;
static int32_t s_location_calibration_timeout = 0;

@implementation MCIPhoneLocationDelegate 

- (void)dealloc
{
	if (m_calibration_timer != nil)
	{
		[m_calibration_timer invalidate];
		[m_calibration_timer release];
	}
	
	[super dealloc];
}

// TODO: Determine difference between location and heading error properly
- (void)locationManager: (CLLocationManager *)manager didFailWithError: (NSError *)error
{
	if (s_location_enabled)
	{
		MCAutoStringRef t_error;
		/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[error localizedDescription], &t_error);
		MCSensorPostErrorMessage(kMCSensorTypeLocation, *t_error);
	}
	else if (s_heading_enabled)
	{
        MCAutoStringRef t_error;
		/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[error localizedDescription], &t_error);
		MCSensorPostErrorMessage(kMCSensorTypeHeading, *t_error);
	}
}

- (void)locationManager: (CLLocationManager *)manager didUpdateToLocation: (CLLocation *)newLocation fromLocation: (CLLocation *)oldLocation
{
    if (s_location_enabled)
        MCSensorPostChangeMessage(kMCSensorTypeLocation);
}

- (void)locationManager: (CLLocationManager *)manager didUpdateHeading: (CLHeading *)newHeading
{
    if (s_heading_enabled)
        MCSensorPostChangeMessage(kMCSensorTypeHeading);
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
	if (m_calibration_timer)
	{
		[m_calibration_timer invalidate];
		[m_calibration_timer release];
		m_calibration_timer = nil;
	}
	
	if (s_location_calibration_timeout == 0)
	{
		return NO;
	}
	else
	{
		// timer to dismiss the message by default
		m_calibration_timer = [[NSTimer scheduledTimerWithTimeInterval:s_location_calibration_timeout
																target: self
															  selector:@selector(calibrationTimeout)
															  userInfo:nil
															   repeats:NO] retain];
		return YES;
	}
}

- (void)calibrationTimeout
{
	[s_location_manager dismissHeadingCalibrationDisplay];
	[m_calibration_timer release];
	m_calibration_timer = nil;
}
@end

static void initialize_core_location(void)
{
	if (s_location_manager != nil)
		return;
	
	s_location_manager = [[CLLocationManager alloc] init];
	s_location_delegate = [[MCIPhoneLocationDelegate alloc] init];
	[s_location_manager setDelegate: s_location_delegate];
	
	s_location_enabled = false;
	s_heading_enabled = false;
    s_tracking_heading_loosely = false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemGetSensorAvailable(MCSensorType p_sensor, bool& r_available)
{    
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
        {
            r_available = ([CLLocationManager locationServicesEnabled] == YES);
            break;
        }
        case kMCSensorTypeHeading:
        {
            r_available = ([CLLocationManager headingAvailable] == YES);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            initialize_core_motion();
            r_available = ([s_motion_manager isAccelerometerAvailable] == YES);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            initialize_core_motion();
            r_available = ([s_motion_manager isGyroAvailable] == YES);
            break;
        }
        default:
            r_available = false;            
    }
    return true;
}

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
// LOCATION SENSEOR
////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingLocation(bool p_loosely)
{
    if ([CLLocationManager locationServicesEnabled] == YES)
    {
        if (!s_location_enabled)
        {
            initialize_core_location();
            [s_location_manager startUpdatingLocation];
            s_location_enabled = true;
        }
        if (p_loosely)
            [s_location_manager setDesiredAccuracy: kCLLocationAccuracyKilometer];            
        else
            [s_location_manager setDesiredAccuracy: kCLLocationAccuracyBest];
        return true;
    }
    return false;
}

bool MCSystemStopTrackingLocation()
{
    if (s_location_enabled)
    {
        if (!s_tracking_heading_loosely)
            [s_location_manager stopUpdatingLocation];
        s_location_enabled = false;
    }
    return true;
}

bool MCSystemGetLocationReading(MCSensorLocationReading &r_reading, bool p_detailed)
{
	if (s_location_enabled)
	{
		CLLocation *t_location;
		t_location = [s_location_manager location];        
        if(t_location == nil)
            return false;
		
		if ([t_location horizontalAccuracy] >= 0.0)
		{
            r_reading.latitude = [t_location coordinate].latitude;
            r_reading.longitude = [t_location coordinate].longitude;            
            if (p_detailed)
                r_reading.horizontal_accuracy = [t_location horizontalAccuracy];
		}
		
		if ([t_location verticalAccuracy] >= 0.0)
		{
            r_reading.altitude = [t_location altitude];
            if (p_detailed)
                r_reading.vertical_accuracy = [t_location verticalAccuracy];
		}
		
        // MM-2013-02-21: Added speed and course to the detailed readings.
        if (p_detailed)
        {
            r_reading.timestamp = [[t_location timestamp] timeIntervalSince1970];
            r_reading.speed = [t_location speed];
            r_reading.course = [t_location course];
        }
        
        return true;
	}
    return false;
}

// MM-2012-02-11: Added iPhoneGet/SetCalibrationTimeout
bool MCSystemSetLocationCalibrationTimeout(int32_t p_timeout)
{
    s_location_calibration_timeout = p_timeout;
    return true;
}

bool MCSystemGetLocationCalibrationTimeout(int32_t& r_timeout)
{
    r_timeout = s_location_calibration_timeout;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// HEADING SENSEOR
////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingHeading(bool p_loosely)
{
    if ([CLLocationManager headingAvailable] == YES)
    {
        if (!s_heading_enabled)
        {
            initialize_core_location();
            [s_location_manager startUpdatingHeading];
            s_heading_enabled = true;
        }
        s_tracking_heading_loosely = p_loosely;
        if (!p_loosely && !s_location_enabled)
        {
            [s_location_manager startUpdatingLocation];
            [s_location_manager setDesiredAccuracy: kCLLocationAccuracyKilometer];            
        }
        return true;
    }
    return false;
}

bool MCSystemStopTrackingHeading()
{
    if (s_heading_enabled)
    {
        [s_location_manager stopUpdatingHeading];
        s_heading_enabled = false;
        if (!s_tracking_heading_loosely && !s_location_enabled)
            [s_location_manager stopUpdatingLocation];        
    }
    return true;
}

bool MCSystemGetHeadingReading(MCSensorHeadingReading &r_reading, bool p_detailed)
{
	if (s_heading_enabled)
	{
		CLHeading *t_heading;
		t_heading = [s_location_manager heading];        
        if(t_heading == nil)
            return false;

        if ([t_heading trueHeading] != -1)
            r_reading.heading = [t_heading trueHeading];
        else
            r_reading.heading = [t_heading magneticHeading];
            
        if (p_detailed)
        {
            if ([t_heading trueHeading] != -1)
                r_reading.true_heading = [t_heading trueHeading];
            r_reading.magnetic_heading = [t_heading magneticHeading];
            r_reading.accuracy = [t_heading headingAccuracy];
            r_reading.x = [t_heading x];
            r_reading.y = [t_heading y];
            r_reading.z = [t_heading z];
            r_reading.timestamp = [[t_heading timestamp] timeIntervalSince1970];
        }
		
        return true;
	}
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// ACCELERATION SENSOR
////////////////////////////////////////////////////////////////////////////////

static void (^acceleration_update)(CMAccelerometerData *, NSError *) = ^(CMAccelerometerData *accelerometerData, NSError *error)
{
	if (s_acceleration_enabled)
	{
		if (error == nil)
			MCSensorPostChangeMessage(kMCSensorTypeAcceleration);
		else
		{
			MCAutoStringRef t_error;
			/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[error localizedDescription], &t_error);
			MCSensorPostErrorMessage(kMCSensorTypeAcceleration, *t_error);
		}
	}
};

bool MCSystemStartTrackingAcceleration(bool p_loosely)
{    
    initialize_core_motion();
    if ([s_motion_manager isAccelerometerAvailable] == YES)
    {
        if (!s_acceleration_enabled)
        {
            [s_motion_manager startAccelerometerUpdatesToQueue: [NSOperationQueue mainQueue] withHandler: acceleration_update];
            [s_motion_manager setAccelerometerUpdateInterval: MCidleRate / 1000];
			s_acceleration_enabled = true;
        }
        return true;
    }
    return false;
}

bool MCSystemStopTrackingAcceleration()
{
    if (s_acceleration_enabled)
    {
        [s_motion_manager stopAccelerometerUpdates];
		s_acceleration_enabled = false;
    }
    return true;
}

bool MCSystemGetAccelerationReading(MCSensorAccelerationReading &r_reading, bool p_detailed)
{
	if (s_acceleration_enabled)
	{
		CMAccelerometerData *t_acceleration;
		t_acceleration = [s_motion_manager accelerometerData];        
        if (t_acceleration == nil)
            return false;

        r_reading.x = [t_acceleration acceleration].x;
        r_reading.y = [t_acceleration acceleration].y;
        r_reading.z = [t_acceleration acceleration].z;		
        
        if (p_detailed)
            r_reading.timestamp = timestamp_from_time_since_boot([t_acceleration timestamp]);
        
        return true;
	}    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// ROTATION RATE SENSOR
////////////////////////////////////////////////////////////////////////////////

static void (^rotation_rate_update)(CMGyroData *, NSError *) = ^(CMGyroData *gyroData, NSError *error)
{
	if (s_rotation_rate_enabled)
	{
		if (error == nil)
            MCSensorPostChangeMessage(kMCSensorTypeRotationRate);
		else
		{
			MCAutoStringRef t_error;
			/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[error localizedDescription], &t_error);
			MCSensorPostErrorMessage(kMCSensorTypeRotationRate, *t_error);
		}
	}
};

bool MCSystemStartTrackingRotationRate(bool p_loosely)
{    
    initialize_core_motion();
    if ([s_motion_manager isGyroAvailable] == YES)
    {
        if (!s_rotation_rate_enabled)
        {
            [s_motion_manager startGyroUpdatesToQueue: [NSOperationQueue mainQueue] withHandler: rotation_rate_update];
            [s_motion_manager setGyroUpdateInterval: MCidleRate / 1000];
            s_rotation_rate_enabled = true;
        }
        return true;
    }
    return false;
}

bool MCSystemStopTrackingRotationRate()
{
    if (s_rotation_rate_enabled)
    {
        [s_motion_manager stopGyroUpdates];
        s_rotation_rate_enabled = false;
    }
    return true;
}

bool MCSystemGetRotationRateReading(MCSensorRotationRateReading &r_reading, bool p_detailed)
{
	if (s_rotation_rate_enabled)
	{
		CMGyroData *t_rotation_rate;
		t_rotation_rate = [s_motion_manager gyroData];        
        if (t_rotation_rate == nil)
            return false;
        
        r_reading.x = [t_rotation_rate rotationRate].x;
        r_reading.y = [t_rotation_rate rotationRate].y;
        r_reading.z = [t_rotation_rate rotationRate].z;		
        
        if (p_detailed)
            r_reading.timestamp = timestamp_from_time_since_boot([t_rotation_rate timestamp]);
        
        return true;
	}    
    return false;
}

////////////////////////////////////////////////////////////////////////////////
/*
bool MCSystemStartTrackingSensor(MCSensorType p_sensor, bool p_loosely)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return start_tracking_location(p_loosely);
        case kMCSensorTypeHeading:
            return start_tracking_heading(p_loosely);
        case kMCSensorTypeAcceleration:
            return start_tracking_acceleration(p_loosely);
        case kMCSensorTypeRotationRate:
            return start_tracking_rotation_rate(p_loosely);
    }    
    return false;
}

bool MCSystemStopTrackingSensor(MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return stop_tracking_location();
        case kMCSensorTypeHeading:
            return stop_tracking_heading();
        case kMCSensorTypeAcceleration:
            return stop_tracking_acceleration();
        case kMCSensorTypeRotationRate:
            return stop_tracking_rotation_rate();
    }
    return false;
}
*/
////////////////////////////////////////////////////////////////////////////////
