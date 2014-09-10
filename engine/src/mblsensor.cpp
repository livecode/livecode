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
#include "stack.h"
#include "image.h"
#include "param.h"
#include "card.h"

#include "exec.h"

#include "eventqueue.h"

#include "mblsensor.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

static bool s_sensor_message_pending[] = {false, false, false, false, false};

static MCSensorLocationReading *s_last_location_reading = nil;
static MCSensorHeadingReading *s_last_heading_reading = nil;
static MCSensorAccelerationReading *s_last_acceleration_reading = nil;
static MCSensorRotationRateReading *s_last_rotation_rate_reading = nil;

////////////////////////////////////////////////////////////////////////////////

// MM-2012-03-13: Added intialize and finalize calls to sensor module.
void MCSensorInitialize(void)
{
    s_last_location_reading = nil;
    s_last_heading_reading = nil;
    s_last_acceleration_reading = nil;
    s_last_rotation_rate_reading = nil;
    s_sensor_message_pending[kMCSensorTypeUnknown] = false;
    s_sensor_message_pending[kMCSensorTypeLocation] = false;
    s_sensor_message_pending[kMCSensorTypeHeading] = false;
    s_sensor_message_pending[kMCSensorTypeAcceleration] = false;
    s_sensor_message_pending[kMCSensorTypeRotationRate] = false;
    MCSystemSensorInitialize();
}

void MCSensorFinalize(void)
{
    /* UNCHECKED */ MCMemoryDelete(s_last_location_reading);
    /* UNCHECKED */ MCMemoryDelete(s_last_heading_reading);
    /* UNCHECKED */ MCMemoryDelete(s_last_acceleration_reading);
    /* UNCHECKED */ MCMemoryDelete(s_last_rotation_rate_reading);
    MCSystemSensorFinalize();
}

////////////////////////////////////////////////////////////////////////////////

MCSensorType MCSensorTypeFromString(MCStringRef p_string)
{
    if (MCStringIsEqualToCString(p_string, "location", kMCCompareCaseless))
        return kMCSensorTypeLocation;
    else if (MCStringIsEqualToCString(p_string, "heading", kMCCompareCaseless))
        return kMCSensorTypeHeading;
    else if (MCStringIsEqualToCString(p_string, "acceleration", kMCCompareCaseless))
        return kMCSensorTypeAcceleration;
    else if (MCStringIsEqualToCString(p_string, "rotation rate", kMCCompareCaseless))
        return kMCSensorTypeRotationRate;
    
    return kMCSensorTypeUnknown;
}

MCStringRef MCSensorTypeToStringRef(MCSensorType p_sensor)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return MCSTR("location");
        case kMCSensorTypeHeading:
            return MCSTR("heading");
        case kMCSensorTypeAcceleration:
            return MCSTR("acceleration");
        case kMCSensorTypeRotationRate:
            return MCSTR("rotation rate");
        default:
			return MCSTR("unknown");
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

class MCSensorErrorEvent: public MCCustomEvent
{
public:
	MCSensorErrorEvent(MCSensorType p_sensor, MCStringRef p_error)
	{
        m_sensor = p_sensor;
        m_error = MCValueRetain(p_error);
	}
	
	~MCSensorErrorEvent()
	{
		MCValueRelease(m_error);
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCStringRef t_sensor;
        t_sensor = MCSensorTypeToStringRef(m_sensor);   
        
        MCdefaultstackptr->getcurcard()->message_with_valueref_args(MCM_tracking_error, t_sensor, m_error);
	}
	
private:
    MCSensorType m_sensor;
    MCStringRef m_error;
};

static bool location_reading_changed(MCSensorLocationReading p_current_reading, MCSensorLocationReading p_last_reading, double p_range)
{
    if ((fabs(p_current_reading.latitude - p_last_reading.latitude) > p_range) || (fabs(p_current_reading.longitude - p_last_reading.longitude) > p_range) || (fabs(p_current_reading.altitude - p_last_reading.altitude) > p_range)) 
        return true;
    else
        return false;
    
}

static bool heading_reading_changed(MCSensorHeadingReading p_current_reading, MCSensorHeadingReading p_last_reading, double p_range)
{
    if (fabs(p_current_reading.heading - p_last_reading.heading) > p_range) 
        return true;
    else
        return false;
    
}

static bool acceleration_reading_changed(MCSensorAccelerationReading p_current_reading, MCSensorAccelerationReading p_last_reading, double p_range)
{
    if ((fabs(p_current_reading.x - p_last_reading.x) > p_range) || (fabs(p_current_reading.y - p_last_reading.y) > p_range) || (fabs(p_current_reading.z - p_last_reading.z) > p_range)) 
        return true;
    else
        return false;
    
}

static bool rotation_rate_reading_changed(MCSensorRotationRateReading p_current_reading, MCSensorRotationRateReading p_last_reading, double p_range)
{
    if ((fabs(p_current_reading.x - p_last_reading.x) > p_range) || (fabs(p_current_reading.y - p_last_reading.y) > p_range) || (fabs(p_current_reading.z - p_last_reading.z) > p_range)) 
        return true;
    else
        return false;
    
}

class MCSensorUpdateEvent: public MCCustomEvent
{
public:    
	MCSensorUpdateEvent(MCSensorType p_sensor)
	{
        m_sensor = p_sensor;
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
        switch (m_sensor)
        {
            case kMCSensorTypeLocation:
            {
                MCSensorLocationReading t_reading;
                if (MCSystemGetLocationReading(t_reading, false))
				{                    
                    if (s_last_location_reading == nil ||
                        location_reading_changed(t_reading, *s_last_location_reading, MCSystemGetSensorDispatchThreshold(m_sensor)))
                    {
                        MCParameter p1, p2, p3;
                        p1.setn_argument(t_reading.latitude);
                        p1.setnext(&p2);
                        p2.setn_argument(t_reading.longitude);
                        p2.setnext(&p3);
                        p3.setn_argument(t_reading.altitude);
                        
                        MCdefaultstackptr->getcurcard()->message(MCM_location_changed, &p1);
                        
                        if (s_last_location_reading == nil)
                            /* UNCHECKED */ MCMemoryNew(s_last_location_reading);
                        
                        *s_last_location_reading = t_reading;
                    }         
				}
                break;
            }                
            case kMCSensorTypeHeading:
            {
                MCSensorHeadingReading t_reading;
                if (MCSystemGetHeadingReading(t_reading, false))
				{                
                    if (s_last_heading_reading == nil ||
                        heading_reading_changed(t_reading, *s_last_heading_reading, MCSystemGetSensorDispatchThreshold(m_sensor)))
                    {
                        MCParameter p1;
                        p1.setn_argument(t_reading.heading);
					
                        MCdefaultstackptr->getcurcard()->message(MCM_heading_changed, &p1);

                        if (s_last_heading_reading == nil)
                            /* UNCHECKED */ MCMemoryNew(s_last_heading_reading);
                    
                        *s_last_heading_reading = t_reading;
                    }
				}
                break;
            }                
           case kMCSensorTypeAcceleration:
            {
                MCSensorAccelerationReading t_reading;
                if (MCSystemGetAccelerationReading(t_reading, false))
				{                
                    if (s_last_acceleration_reading == nil ||
                        acceleration_reading_changed(t_reading, *s_last_acceleration_reading, MCSystemGetSensorDispatchThreshold(m_sensor)))
                    {
                        MCParameter p1, p2, p3;
                        p1.setn_argument(t_reading.x);
                        p1.setnext(&p2);
                        p2.setn_argument(t_reading.y);
                        p2.setnext(&p3);
                        p3.setn_argument(t_reading.z);
					
                        MCdefaultstackptr->getcurcard()->message(MCM_acceleration_changed, &p1);

                        if (s_last_acceleration_reading == nil)
                        /* UNCHECKED */ MCMemoryNew(s_last_acceleration_reading);
                        
                        *s_last_acceleration_reading = t_reading;
                    }
				}
                break;
            }                
            case kMCSensorTypeRotationRate:
            {
                MCSensorRotationRateReading t_reading;
                if (MCSystemGetRotationRateReading(t_reading, true))
				{                
                    if (s_last_rotation_rate_reading == nil || 
                        rotation_rate_reading_changed(t_reading, *s_last_rotation_rate_reading, MCSystemGetSensorDispatchThreshold(m_sensor)))
                    {
                        MCParameter p1, p2, p3;
                        p1.setn_argument(t_reading.x);
                        p1.setnext(&p2);
                        p2.setn_argument(t_reading.y);
                        p2.setnext(&p3);
                        p3.setn_argument(t_reading.z);
					
                        MCdefaultstackptr->getcurcard()->message(MCM_rotation_rate_changed, &p1);
                        
                        if (s_last_rotation_rate_reading == nil)
                        /* UNCHECKED */ MCMemoryNew(s_last_rotation_rate_reading);
                        
                        *s_last_rotation_rate_reading = t_reading;
                    }
				}
                break;
            }
        }
        s_sensor_message_pending[m_sensor] = false;
	}
	
private:
    MCSensorType m_sensor;    
    
};

static MCSensorUpdateEvent * FetchSensorEvent(MCSensorType p_sensor)
{
    MCSensorUpdateEvent *t_event = nil;
    if (!s_sensor_message_pending[p_sensor])
    {
        s_sensor_message_pending[p_sensor] = true;
        t_event = new MCSensorUpdateEvent(p_sensor);
    }
    return t_event;        
}

////////////////////////////////////////////////////////////////////////////////

void MCSensorPostChangeMessage(MCSensorType p_sensor)
{
    MCCustomEvent *t_event = nil;
    t_event = FetchSensorEvent(p_sensor);
    if (t_event != nil)
        MCEventQueuePostCustom(t_event);
}

void MCSensorPostErrorMessage(MCSensorType p_sensor, MCStringRef p_error)
{
    MCCustomEvent *t_event = nil;
	t_event = new MCSensorErrorEvent(p_sensor, p_error);
	MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////
