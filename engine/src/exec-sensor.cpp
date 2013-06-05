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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"
#include "mblsensor.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCSensorTypeElementInfo[] =
{
	{ "unknown", 0 },
};

static MCExecEnumTypeInfo _kMCSensorTypeTypeInfo =
{
	"Sensor.Type",
	sizeof(_kMCSensorTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCSensorTypeElementInfo
};

//////////

void MCSensorExecStartTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor, bool p_loosely)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            MCSystemStartTrackingLocation(p_loosely);
        case kMCSensorTypeHeading:
            MCSystemStartTrackingHeading(p_loosely);
        case kMCSensorTypeAcceleration:
            MCSystemStartTrackingAcceleration(p_loosely);
        case kMCSensorTypeRotationRate:
            MCSystemStartTrackingRotationRate(p_loosely);
    }    
}

void MCSensorExecStopTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            MCSystemStopTrackingLocation();
        case kMCSensorTypeHeading:
            MCSystemStopTrackingHeading();
        case kMCSensorTypeAcceleration:
            MCSystemStopTrackingAcceleration();
        case kMCSensorTypeRotationRate:
            MCSystemStopTrackingRotationRate();
    }
}

void MCSensorGetSensorAvailable(MCExecContext& ctxt, MCSensorType p_sensor, bool& r_available)
{
    MCSystemGetSensorAvailable(p_sensor, r_available);
}

void MCSensorGetDetailedLocationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_location)
{
	MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, true))
    {
        MCAutoArrayRef t_location_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_location_array);

		MCAutoNumberRef t_latitude;
		MCNewAutoNameRef t_latitude_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.latitude, &t_latitude);
		/* UNCHECKED */ MCNameCreateWithCString("latitude", &t_latitude_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_latitude_name, *t_latitude);
              
		MCAutoNumberRef t_longitude;
		MCNewAutoNameRef t_longitude_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.longitude, &t_longitude);
		/* UNCHECKED */ MCNameCreateWithCString("longitude", &t_longitude_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_longitude_name, *t_longitude);

		MCAutoNumberRef t_altitude;
		MCNewAutoNameRef t_altitude_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.altitude, &t_altitude);
		/* UNCHECKED */ MCNameCreateWithCString("altitude", &t_altitude_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_altitude_name, *t_altitude);
        
        // MM-2013-02-21: Add speed and course to detailed location readings.
        if (t_reading.speed >= 0.0f)
        {
			MCAutoNumberRef t_speed;
			MCNewAutoNameRef t_speed_name;
			/* UNCHECKED */ MCNumberCreateWithReal(t_reading.speed, &t_speed);
			/* UNCHECKED */ MCNameCreateWithCString("speed", &t_speed_name);
			/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_speed_name, *t_speed);
        }        
        if (t_reading.course >= 0.0f)
        {
			MCAutoNumberRef t_course;
			MCNewAutoNameRef t_course_name;
			/* UNCHECKED */ MCNumberCreateWithReal(t_reading.course, &t_course);
			/* UNCHECKED */ MCNameCreateWithCString("course", &t_course_name);
			/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_course_name, *t_course);
        }
        
		MCAutoNumberRef t_timestamp;
		MCNewAutoNameRef t_timestamp_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCNameCreateWithCString("timestamp", &t_timestamp_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_timestamp_name, *t_timestamp);
        
		MCAutoNumberRef t_horizontal_accuracy;
		MCNewAutoNameRef t_horizontal_accuracy_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.horizontal_accuracy, &t_horizontal_accuracy);
		/* UNCHECKED */ MCNameCreateWithCString("horizontal accuracy", &t_horizontal_accuracy_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_horizontal_accuracy_name, *t_horizontal_accuracy);
        
		MCAutoNumberRef t_vertical_accuracy;
		MCNewAutoNameRef t_vertical_accuracy_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.vertical_accuracy, &t_vertical_accuracy);
		/* UNCHECKED */ MCNameCreateWithCString("vertical accuracy", &t_vertical_accuracy_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_location_array, false, *t_vertical_accuracy_name, *t_vertical_accuracy);
        
        r_detailed_location = MCValueRetain(*t_location_array);
    }
}

void MCSensorGetLocationOfDevice(MCExecContext& ctxt, MCStringRef &r_location)
{
    MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, false))
        MCStringFormat(r_location, "%Lf,%Lf,%Lf", t_reading.latitude, t_reading.longitude, t_reading.altitude);
}

void MCSensorGetDetailedHeadingOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_heading)
{
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
    {
        MCAutoArrayRef t_heading_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_heading_array);

		MCAutoNumberRef t_heading;
		MCNewAutoNameRef t_heading_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.heading, &t_heading);
		/* UNCHECKED */ MCNameCreateWithCString("heading", &t_heading_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_heading_name, *t_heading);
              
		MCAutoNumberRef t_magnetic_heading;
		MCNewAutoNameRef t_magnetic_heading_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.magnetic_heading, &t_magnetic_heading);
		/* UNCHECKED */ MCNameCreateWithCString("magnetic heading", &t_magnetic_heading_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_magnetic_heading_name, *t_magnetic_heading);

		MCAutoNumberRef t_true_heading;
		MCNewAutoNameRef t_true_heading_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.true_heading, &t_true_heading);
		/* UNCHECKED */ MCNameCreateWithCString("true heading", &t_true_heading_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_true_heading_name, *t_true_heading);
        
		MCAutoNumberRef t_x;
		MCNewAutoNameRef t_x_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCNameCreateWithCString("x", &t_x_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_x_name, *t_x);
   
		MCAutoNumberRef t_y;
		MCNewAutoNameRef t_y_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCNameCreateWithCString("y", &t_y_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_y_name, *t_y);

		MCAutoNumberRef t_z;
		MCNewAutoNameRef t_z_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCNameCreateWithCString("z", &t_z_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_z_name, *t_z);
		
		MCAutoNumberRef t_timestamp;
		MCNewAutoNameRef t_timestamp_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCNameCreateWithCString("timestamp", &t_timestamp_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_timestamp_name, *t_timestamp);
              
		MCAutoNumberRef t_accuracy;
		MCNewAutoNameRef t_accuracy_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.accuracy, &t_accuracy);
		/* UNCHECKED */ MCNameCreateWithCString("accuracy", &t_accuracy_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_heading_array, false, *t_accuracy_name, *t_accuracy);
        
        r_detailed_heading = MCValueRetain(*t_heading_array);
    }
}

void MCSensorGetHeadingOfDevice(MCExecContext& ctxt, MCStringRef &r_heading)
{
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
        MCStringFormat(r_heading, "%Lf", t_reading.heading);
}

void MCSensorGetDetailedAccelerationOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_acceleration)
{
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
    {
		MCAutoArrayRef t_acceleration_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_acceleration_array);

		MCAutoNumberRef t_x;
		MCNewAutoNameRef t_x_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCNameCreateWithCString("x", &t_x_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, *t_x_name, *t_x);
   
		MCAutoNumberRef t_y;
		MCNewAutoNameRef t_y_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCNameCreateWithCString("y", &t_y_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, *t_y_name, *t_y);

		MCAutoNumberRef t_z;
		MCNewAutoNameRef t_z_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCNameCreateWithCString("z", &t_z_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, *t_z_name, *t_z);
		
		MCAutoNumberRef t_timestamp;
		MCNewAutoNameRef t_timestamp_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCNameCreateWithCString("timestamp", &t_timestamp_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_acceleration_array, false, *t_timestamp_name, *t_timestamp);
        
        r_detailed_acceleration = MCValueRetain(*t_acceleration_array);
	}
}

void MCSensorGetAccelerationOfDevice(MCExecContext& ctxt, MCStringRef &r_acceleration)
{
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
        MCStringFormat(r_acceleration, "%Lf,%Lf,%Lf", t_reading.x, t_reading.y, t_reading.z);
}

void MCSensorGetDetailedRotationRateOfDevice(MCExecContext& ctxt, MCArrayRef &r_detailed_rotation_rate)
{
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
		MCAutoArrayRef t_rotation_array;
		/* UNCHECKED */ MCArrayCreateMutable(&t_rotation_array);

		MCAutoNumberRef t_x;
		MCNewAutoNameRef t_x_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.x, &t_x);
		/* UNCHECKED */ MCNameCreateWithCString("x", &t_x_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, *t_x_name, *t_x);
   
		MCAutoNumberRef t_y;
		MCNewAutoNameRef t_y_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.y, &t_y);
		/* UNCHECKED */ MCNameCreateWithCString("y", &t_y_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, *t_y_name, *t_y);

		MCAutoNumberRef t_z;
		MCNewAutoNameRef t_z_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.z, &t_z);
		/* UNCHECKED */ MCNameCreateWithCString("z", &t_z_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, *t_z_name, *t_z);
		
		MCAutoNumberRef t_timestamp;
		MCNewAutoNameRef t_timestamp_name;
		/* UNCHECKED */ MCNumberCreateWithReal(t_reading.timestamp, &t_timestamp);
		/* UNCHECKED */ MCNameCreateWithCString("timestamp", &t_timestamp_name);
		/* UNCHECKED */ MCArrayStoreValue(*t_rotation_array, false, *t_timestamp_name, *t_timestamp);
        
        r_detailed_rotation_rate = MCValueRetain(*t_rotation_array);
	}
}

void MCSensorGetRotationRateOfDevice(MCExecContext& ctxt, MCStringRef &r_rotation_rate)
{
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
        r_rotation_rate = nil;
        MCStringFormat(r_rotation_rate, "%Lf,%Lf,%Lf", t_reading.x, t_reading.y, t_reading.z);
    }
}

// MM-2012-02-11: Added support for iPhoneGet/SetCalibrationTimeout
void MCSensorSetLocationCalibration(MCExecContext& ctxt, int32_t p_timeout)
{
    MCSystemSetLocationCalibrationTimeout(p_timeout);
}

void MCSensorGetLocationCalibration(MCExecContext& ctxt, int32_t& r_timeout)
{
    MCSystemGetLocationCalibrationTimeout(r_timeout);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////