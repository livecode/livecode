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

#include "execpt.h"
#include "globals.h"
#include "eventqueue.h"

#include "exec.h"
#include "mblsyntax.h"
#include "mblsensor.h"

#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneMotionDelegate : NSObject
{
}
@end

static CMMotionManager *s_motion_manager = nil;
static MCIPhoneMotionDelegate *s_motion_delegate = nil;
static bool s_accelerometer_enabled = false;
static bool s_gyroscope_enabled = false;

@implementation MCIPhoneMotionDelegate
@end

////////////////////////////////////////////////////////////////////////////////

static void initialize_core_motion(void)
{
	if (s_motion_manager != nil)
		return;
    
	s_motion_manager = [[CMMotionManager alloc] init];
    s_motion_delegate = [[MCIPhoneMotionDelegate alloc] init];
	[s_motion_manager setDelegate: s_motion_delegate];
	s_accelerometer_enabled = false;
	s_gyroscope_enabled = false;
}

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
static int32_t s_calibration_timeout = 0;

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
		MCEventQueuePostLocationError();
	else if (s_heading_enabled)
		MCEventQueuePostHeadingError();
}

- (void)locationManager: (CLLocationManager *)manager didUpdateToLocation: (CLLocation *)newLocation fromLocation: (CLLocation *)oldLocation
{
	MCEventQueuePostLocationChanged();
}

- (void)locationManager: (CLLocationManager *)manager didUpdateHeading: (CLHeading *)newHeading
{
	MCEventQueuePostHeadingChanged();
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
	if (m_calibration_timer)
	{
		[m_calibration_timer invalidate];
		[m_calibration_timer release];
		m_calibration_timer = nil;
	}
	
	if (s_calibration_timeout == 0)
	{
		return NO;
	}
	else
	{
		// timer to dismiss the message by default
		m_calibration_timer = [[NSTimer scheduledTimerWithTimeInterval:s_calibration_timeout
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

////////////////////////////////////////////////////////////////////////////////

static void initialize_core_location(void)
{
	if (s_location_manager != nil)
		return;
	
	s_location_manager = [[CLLocationManager alloc] init];
	s_location_delegate = [[MCIPhoneLocationDelegate alloc] init];
	[s_location_manager setDelegate: s_location_delegate];
	
	s_location_enabled = false;
	s_heading_enabled = false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingSensor(MCSensorType p_sensor, bool p_loosely)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
        {
            initialize_core_location();
            if (([CLLocationManager locationServicesEnabled] == YES) && (!s_location_enabled))
            {
                [ s_location_manager startUpdatingLocation ];
                s_location_enabled = true;
            }
            break;
        }
        case kMCSensorTypeHeading:
        {
            initialize_core_location();
            if (([CLLocationManager headingAvailable] == YES) && (!s_heading_enabled))
            {
                [ s_location_manager startUpdatingHeading ];
                s_heading_enabled = true;
            }
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            initialize_core_motion();
            if (([s_motion_manager isAccelerometerAvailable] == YES) && (!s_accelerometer_enabled))
            {
                [ s_motion_manager startAccelerometerUpdates ];
                s_accelerometer_enabled = true;
            }
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            initialize_core_motion();
            if (([s_motion_manager isGyroAvailable] == YES) && (!s_gyroscope_enabled))
            {
                [ s_motion_manager startGyroUpdates ];
                s_gyroscope_enabled = true;
            }
            break;
        }
    }
    return true;
}

bool MCSystemStopTrackingSensor(MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
        {
            initialize_core_location();
            if (([CLLocationManager locationServicesEnabled] == YES) && (s_location_enabled))
            {
                [ s_location_manager stopUpdatingLocation ];
                s_location_enabled = false;
            }
            break;
        }
        case kMCSensorTypeHeading:
        {
            initialize_core_location();
            if (([CLLocationManager headingAvailable] == YES) && (s_heading_enabled))
            {
                [ s_location_manager stopUpdatingHeading ];
                s_heading_enabled = false;
            }
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            initialize_core_motion();
            if (([s_motion_manager isAccelerometerAvailable] == YES) && (s_accelerometer_enabled))
            {
                [ s_motion_manager stopAccelerometerUpdates ];
                s_accelerometer_enabled = false;
            }
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            initialize_core_motion();
            if (([s_motion_manager isGyroAvailable] == YES) && (s_gyroscope_enabled))
            {
                [ s_motion_manager stopGyroUpdates ];
                s_gyroscope_enabled = false;
            }
            break;
        }
    }
    return true;
}

bool MCSystemGetSensorAvailable(MCSensorType p_sensor)
{
    MCExecPoint ep(nil, nil, nil);

    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
        {
            MCresult->sets(MCU_btos([CLLocationManager locationServicesEnabled] == YES));
            break;
        }
        case kMCSensorTypeHeading:
        {
            MCresult -> sets(MCU_btos([CLLocationManager headingAvailable] == YES));
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            initialize_core_motion();
            MCresult -> sets(MCU_btos([s_motion_manager isAccelerometerAvailable] == YES));
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            initialize_core_motion();
            MCresult->sets(MCU_btos([s_motion_manager isGyroAvailable] == YES));
            break;
        }
            
    }
    return true;
}

bool MCSystemGetLocationReading(MCSensorLocationReading &r_reading, bool p_detailed)
{
    MCresult -> clear();
	
	if (s_location_enabled)
	{
		CLLocation *t_location;
		t_location = [s_location_manager location];
		
		MCExecPoint ep(nil, nil, nil);
		if ([t_location horizontalAccuracy] >= 0.0)
		{
			ep . setnvalue([t_location horizontalAccuracy]);
			MCresult -> store_element(ep, "horizontal accuracy", False);
			ep . setnvalue([t_location coordinate] . latitude);
			MCresult -> store_element(ep, "latitude", False);
			ep . setnvalue([t_location coordinate] . longitude);
			MCresult -> store_element(ep, "longitude", False);
		}
		
		if ([t_location verticalAccuracy] >= 0.0)
		{
			ep . setnvalue([t_location verticalAccuracy]);
			MCresult -> store_element(ep, "vertical accuracy", False);
			ep . setnvalue([t_location altitude]);
			MCresult -> store_element(ep, "altitude", False);
		}
		
		ep . setnvalue([[t_location timestamp] timeIntervalSince1970]);
		MCresult -> store_element(ep, "timestamp", False);
	}
    return true;
}

bool MCSystemGetHeadingReading(MCSensorHeadingReading &r_reading, bool p_detailed)
{
	MCresult -> clear();
	
	if (s_heading_enabled)
	{
		CLHeading *t_heading;
		t_heading = [s_location_manager heading];
        
		MCExecPoint ep(nil, nil, nil);
        
		ep . setnvalue([t_heading magneticHeading]);
		MCresult -> store_element(ep, "magnetic heading", False);
		if ([t_heading trueHeading] != -1)
		{
			ep . setnvalue([t_heading trueHeading]);
			MCresult -> store_element(ep, "true heading", False);
			MCresult -> store_element(ep, "heading", False);
		}
		else
		{
			ep . setnvalue([t_heading magneticHeading]);
			MCresult -> store_element(ep, "heading", False);
		}
		ep . setnvalue([t_heading headingAccuracy]);
		MCresult -> store_element(ep, "accuracy", False);
		ep . setnvalue([t_heading x]);
		MCresult -> store_element(ep, "x", False);
		ep . setnvalue([t_heading y]);
		MCresult -> store_element(ep, "y", False);
		ep . setnvalue([t_heading z]);
		MCresult -> store_element(ep, "z", False);
		
		ep . setnvalue([[t_heading timestamp] timeIntervalSince1970]);
		MCresult -> store_element(ep, "timestamp", False);
	}
    return true;
}

bool MCSystemGetAccelerationReading(MCSensorAccelerationReading &r_reading, bool p_detailed)
{
	MCresult -> clear();
	
	if (s_accelerometer_enabled)
	{
		CMAccelerometerData *t_accelerometer;
        t_accelerometer = [s_motion_manager accelerometerData];
        
		MCExecPoint ep(nil, nil, nil);
        
		ep . setnvalue(t_accelerometer.acceleration.x);
		MCresult -> store_element(ep, "x", False);
		ep . setnvalue(t_accelerometer.acceleration.y);
		MCresult -> store_element(ep, "y", False);
		ep . setnvalue(t_accelerometer.acceleration.z);
		MCresult -> store_element(ep, "z", False);
		ep . setnvalue(0);
		MCresult -> store_element(ep, "timestamp", False);
	}
    return true;
}

bool MCSystemGetRotationRateReading(MCSensorRotationRateReading &r_reading, bool p_detailed)
{
	MCresult -> clear();
	
	if (s_gyroscope_enabled)
	{
		CMGyroData *t_gyro;
        t_gyro = [s_motion_manager gyroData];
        if (t_gyro == nil)
            return false;
		MCExecPoint ep(nil, nil, nil);
        
		ep . setnvalue(t_gyro.rotationRate.x);
		MCresult -> store_element(ep, "x", False);
		ep . setnvalue(t_gyro.rotationRate.y);
		MCresult -> store_element(ep, "y", False);
		ep . setnvalue(t_gyro.rotationRate.z);
		MCresult -> store_element(ep, "z", False);
		ep . setnvalue(0);
		MCresult -> store_element(ep, "timestamp", False);
	}
    return true;
}

////////////////////////////////////////////////////////////////////////////////

//Exec_stat MCHandleCanTrackLocation(void *context, MCParameter *p_parameters)
//{
//    MCSystemGetSensorAvailable(kMCSensorTypeLocation);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleLocationTrackingState(void *context, MCParameter *p_parameters)
//{
//    bool t_start;
//    t_start = (bool) context;
//    if (t_start)
//        MCSystemStartTrackingSensor(kMCSensorTypeLocation, true);
//    else
//        MCSystemStopTrackingSensor(kMCSensorTypeLocation);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleCurrentLocation(void *context, MCParameter *p_parameters)
//{
//    MCSensorLocationReading r_reading;
//    MCSystemGetLocationReading(r_reading, true);
//	return ES_NORMAL;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//Exec_stat MCHandleCanTrackHeading(void *context, MCParameter *p_parameters)
//{
//    MCSystemGetSensorAvailable(kMCSensorTypeHeading);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleHeadingTrackingState(void *context, MCParameter *p_parameters)
//{
//    bool t_start;
//    t_start = (bool) context;
//    if (t_start)
//        MCSystemStartTrackingSensor(kMCSensorTypeHeading, true);
//    else
//        MCSystemStopTrackingSensor(kMCSensorTypeHeading);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleCurrentHeading(void *context, MCParameter *p_parameters)
//{
//    MCSensorHeadingReading r_reading;
//    MCSystemGetHeadingReading(r_reading, true);
//	return ES_NORMAL;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//Exec_stat MCHandleCanTrackRotation(void *context, MCParameter *p_parameters)
//{
//    MCSystemGetSensorAvailable(kMCSensorTypeRotationRate);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleRotationTrackingState(void *context, MCParameter *p_parameters)
//{
//    bool t_start;
//    t_start = (bool) context;
//    if (t_start)
//        MCSystemStartTrackingSensor(kMCSensorTypeRotationRate, true);
//    else
//        MCSystemStopTrackingSensor(kMCSensorTypeRotationRate);
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleCurrentRotation(void *context, MCParameter *p_parameters)
//{
//    MCSensorRotationRateReading r_reading;
//    MCSystemGetRotationRateReading(r_reading, true);
//	return ES_NORMAL;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//Exec_stat MCHandleAccelerometerEnablement(void *context, MCParameter *p_parameters)
//{
//    bool t_start;
//    t_start = (bool) context;
//    if (t_start)
//        MCSystemStartTrackingSensor(kMCSensorTypeAcceleration, true);
//    else
//        MCSystemStopTrackingSensor(kMCSensorTypeAcceleration);
//	return ES_NORMAL;
//}
//
//////////////////////////////////////////////////////////////////////////////////
//
//Exec_stat MCHandleSetHeadingCalibrationTimeout(void *context, MCParameter *p_parameters)
//{
//	MCExecPoint ep(nil, nil, nil);
//	
//	if (p_parameters != nil)
//		MCParseParameters(p_parameters, "i", &s_calibration_timeout);
//	
//	return ES_NORMAL;
//}
//
//Exec_stat MCHandleHeadingCalibrationTimeout(void *context, MCParameter *p_parameters)
//{
//	MCresult->setnvalue(s_calibration_timeout);
//
//	return ES_NORMAL;
//}

////////////////////////////////////////////////////////////////////////////////
