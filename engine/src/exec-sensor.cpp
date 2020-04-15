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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"
#include "mblsensor.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCSensorTypeElementInfo[] =
{
	{ "unknown", kMCSensorTypeUnknown, true },
    { "location", kMCSensorTypeLocation, false },
    { "heading", kMCSensorTypeHeading, false },
    { "acceleration", kMCSensorTypeAcceleration, false },
    { "rotation rate", kMCSensorTypeRotationRate, false }
};

static MCExecEnumTypeInfo _kMCSensorTypeTypeInfo =
{
	"Sensor.Type",
	sizeof(_kMCSensorTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCSensorTypeElementInfo
};

//////////

MCExecEnumTypeInfo *kMCSensorTypeTypeInfo = &_kMCSensorTypeTypeInfo;

//////////

void MCSensorExecStartTrackingSensor(MCExecContext& ctxt, intenum_t p_sensor, bool p_loosely)
{
    MCSensorType t_sensor;
    t_sensor = (MCSensorType)p_sensor;
    
    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
            MCSystemStartTrackingLocation(p_loosely);
            break;
        case kMCSensorTypeHeading:
            MCSystemStartTrackingHeading(p_loosely);
            break;
        case kMCSensorTypeAcceleration:
            MCSystemStartTrackingAcceleration(p_loosely);
            break;
        case kMCSensorTypeRotationRate:
            MCSystemStartTrackingRotationRate(p_loosely);
            break;
        default:
            break;
    }
}

void MCSensorExecStopTrackingSensor(MCExecContext& ctxt, intenum_t p_sensor)
{
    MCSensorType t_sensor;
    t_sensor = (MCSensorType)p_sensor;
    
    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
            MCSystemStopTrackingLocation();
            break;
        case kMCSensorTypeHeading:
            MCSystemStopTrackingHeading();
            break;
        case kMCSensorTypeAcceleration:
            MCSystemStopTrackingAcceleration();
            break;
        case kMCSensorTypeRotationRate:
            MCSystemStopTrackingRotationRate();
            break;
        default:
            break;
    }
}

void MCSensorGetSensorAvailable(MCExecContext& ctxt, intenum_t p_sensor, bool& r_available)
{
    MCSystemGetSensorAvailable((MCSensorType)p_sensor, r_available);
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCSensorGetDetailedLocationArray(const MCSensorLocationReading& p_reading, MCArrayRef& r_detailed_location)
{
    MCAutoArrayRef t_location_array;
    /* UNCHECKED */ MCArrayCreateMutable(&t_location_array);
    
    MCAutoNumberRef t_latitude;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.latitude, &t_latitude);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("latitude"), *t_latitude);
    
    MCAutoNumberRef t_longitude;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.longitude, &t_longitude);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("longitude"), *t_longitude);
    
    MCAutoNumberRef t_altitude;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.altitude, &t_altitude);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("altitude"), *t_altitude);
    
    // MM-2013-02-21: Add speed and course to detailed location readings.
    if (p_reading.speed >= 0.0f)
    {
        MCAutoNumberRef t_speed;
        /* UNCHECKED */ MCNumberCreateWithReal(p_reading.speed, &t_speed);
        /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("speed"), *t_speed);
    }
    if (p_reading.course >= 0.0f)
    {
        MCAutoNumberRef t_course;
        /* UNCHECKED */ MCNumberCreateWithReal(p_reading.course, &t_course);
        /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("course"), *t_course);
    }
    
    MCAutoNumberRef t_timestamp;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.timestamp, &t_timestamp);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("timestamp"), *t_timestamp);
    
    MCAutoNumberRef t_horizontal_accuracy;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.horizontal_accuracy, &t_horizontal_accuracy);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("horizontal accuracy"), *t_horizontal_accuracy);
    
    MCAutoNumberRef t_vertical_accuracy;
    /* UNCHECKED */ MCNumberCreateWithReal(p_reading.vertical_accuracy, &t_vertical_accuracy);
    /* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, MCNAME("vertical accuracy"), *t_vertical_accuracy);
    
    r_detailed_location = t_location_array.Take();
    
    return true;
}

void MCSensorGetDetailedLocationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_location)
{
	MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, true))
    {
        __MCSensorGetDetailedLocationArray(t_reading, r_detailed_location);
    }
}

void MCSensorGetLocationHistoryOfDevice(MCExecContext& ctxt, MCArrayRef& r_location_history)
{
    MCAutoArrayRef t_history;
    if (!MCArrayCreateMutable(&t_history))
        return;
    
    uindex_t t_index;
    t_index = 1;
    for(;;)
    {
        MCSensorLocationReading t_reading;
        if (!MCSensorPopLocationSample(t_reading))
            break;
        
        MCAutoArrayRef t_sample;
        if (!__MCSensorGetDetailedLocationArray(t_reading,
                                                &t_sample))
            return;
        
        if (!MCArrayStoreValueAtIndex(*t_history,
                                      t_index,
                                      *t_sample))
            return;
        
        t_index += 1;
    }
    
    r_location_history = t_history.Take();
}

void MCSensorGetLocationOfDevice(MCExecContext& ctxt, MCStringRef &r_location)
{
    MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, false))
    {
        bool t_success = true;
        MCAutoStringRef t_location(kMCEmptyString);
        t_success = t_location.MakeMutable();
        if (t_success)
        {
            if (isfinite(t_reading.latitude))
                t_success = MCStringAppendFormat(*t_location, "%lf,", t_reading.latitude);
            else
                t_success = MCStringAppendChar(*t_location, ',');
        }
        if (t_success)
        {
            if (isfinite(t_reading.longitude))
                t_success = MCStringAppendFormat(*t_location, "%lf,", t_reading.longitude);
            else
                t_success = MCStringAppendChar(*t_location, ',');
        }
        if (t_success)
        {
            if (isfinite(t_reading.altitude))
                t_success = MCStringAppendFormat(*t_location, "%lf", t_reading.altitude);
        }
        
        if (t_success)
            t_success = t_location.MakeImmutable();
        
        if (t_success)
            r_location = MCValueRetain(*t_location);
        else
            ctxt.Throw();
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCSensorGetDetailedHeadingOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_heading)
{
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
    {
        MCAutoArrayRef t_heading_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_heading_array);

		MCAutoNumberRef t_heading;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.heading, &t_heading);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("heading"), *t_heading);
              
		MCAutoNumberRef t_magnetic_heading;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.magnetic_heading, &t_magnetic_heading);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("magnetic heading"), *t_magnetic_heading);

		MCAutoNumberRef t_true_heading;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.true_heading, &t_true_heading);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("true heading"), *t_true_heading);
        
		MCAutoNumberRef t_x;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("x"), *t_x);
   
		MCAutoNumberRef t_y;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("y"), *t_y);

		MCAutoNumberRef t_z;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("z"), *t_z);
		
		MCAutoNumberRef t_timestamp;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("timestamp"), *t_timestamp);
              
		MCAutoNumberRef t_accuracy;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.accuracy, &t_accuracy);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, MCNAME("accuracy"), *t_accuracy);
        
        r_detailed_heading = MCValueRetain(*t_heading_array);
    }
}

void MCSensorGetHeadingOfDevice(MCExecContext& ctxt, MCStringRef &r_heading)
{
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
        // PM-2014-10-09: [[ Bug 12142 ]] The old %Lf format worked for device but failed on simulator.
        //  Currently, simulator does not support heading, acceleration or rotation, but since this might
        //  change in the future, use %lf instead
        MCStringFormat(r_heading, "%lf", t_reading.heading);
}

void MCSensorGetDetailedAccelerationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_acceleration)
{
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
    {
		MCAutoArrayRef t_acceleration_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_acceleration_array);

		MCAutoNumberRef t_x;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, MCNAME("x"), *t_x);
   
		MCAutoNumberRef t_y;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, MCNAME("y"), *t_y);

		MCAutoNumberRef t_z;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, MCNAME("z"), *t_z);
		
		MCAutoNumberRef t_timestamp;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, MCNAME("timestamp"), *t_timestamp);
        
        r_detailed_acceleration = MCValueRetain(*t_acceleration_array);
	}
}

void MCSensorGetAccelerationOfDevice(MCExecContext& ctxt, MCStringRef &r_acceleration)
{
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
        // PM-2014-10-09: [[ Bug 12142 ]] The old %Lf format worked for device but failed on simulator.
        //  Currently, simulator does not support heading, acceleration or rotation, but since this might
        //  change in the future, use %lf instead
        MCStringFormat(r_acceleration, "%lf,%lf,%lf", t_reading.x, t_reading.y, t_reading.z);
}

void MCSensorGetDetailedRotationRateOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_rotation_rate)
{
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
		MCAutoArrayRef t_rotation_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_rotation_array);

		MCAutoNumberRef t_x;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, MCNAME("x"), *t_x);
   
		MCAutoNumberRef t_y;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, MCNAME("y"), *t_y);

		MCAutoNumberRef t_z;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, MCNAME("z"), *t_z);
		
		MCAutoNumberRef t_timestamp;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, MCNAME("timestamp"), *t_timestamp);
        
        r_detailed_rotation_rate = MCValueRetain(*t_rotation_array);
	}
}

void MCSensorGetRotationRateOfDevice(MCExecContext& ctxt, MCStringRef &r_rotation_rate)
{
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
        r_rotation_rate = nil;
        // PM-2014-10-09: [[ Bug 12142 ]] The old %Lf format worked for device but failed on simulator.
        //  Currently, simulator does not support heading, acceleration or rotation, but since this might
        //  change in the future, use %lf instead
        MCStringFormat(r_rotation_rate, "%lf,%lf,%lf", t_reading.x, t_reading.y, t_reading.z);
    }
}

// MM-2012-02-11: Added support for iPhoneGet/SetCalibrationTimeout
void MCSensorSetLocationCalibrationTimeout(MCExecContext& ctxt, int32_t p_timeout)
{
    MCSystemSetLocationCalibrationTimeout(p_timeout);
}

void MCSensorGetLocationCalibrationTimeout(MCExecContext& ctxt, int32_t& r_timeout)
{
    MCSystemGetLocationCalibrationTimeout(r_timeout);
}

// SN-2014-10-15: [[ Merge-6.7.0-rc-3 ]]
void MCSensorGetLocationAuthorizationStatus(MCExecContext& ctxt, MCStringRef &r_status)
{
    if (MCSystemGetLocationAuthorizationStatus(r_status))
        return;

    ctxt . Throw();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
