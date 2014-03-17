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
#include "core.h"

#include "eventqueue.h"

#include "mblsensor.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

bool MCSystemStartTrackingSensor(MCSensorType p_sensor, bool p_loosely);
bool MCSystemStopTrackingSensor(MCSensorType p_sensor);

bool MCSystemGetSensorAvailable(MCSensorType p_sensor, bool& r_available);

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

static MCSensorType MCSensorTypeFromCString(const char *p_string)
{
    if (MCCStringEqualCaseless(p_string, "location"))
        return kMCSensorTypeLocation;
    else if (MCCStringEqualCaseless(p_string, "heading"))
        return kMCSensorTypeHeading;
    else if (MCCStringEqualCaseless(p_string, "acceleration"))
        return kMCSensorTypeAcceleration;
    else if (MCCStringEqualCaseless(p_string, "rotation rate"))
        return kMCSensorTypeRotationRate;
    
    return kMCSensorTypeUnknown;
}

static bool MCSensorTypeToCString(MCSensorType p_sensor, char *&r_string)
{
    switch (p_sensor)
    {
        case kMCSensorTypeLocation:
            return MCCStringClone("location", r_string);
        case kMCSensorTypeHeading:
            return MCCStringClone("heading", r_string);
            break;
        case kMCSensorTypeAcceleration:
            return MCCStringClone("acceleration", r_string);
        case kMCSensorTypeRotationRate:
            return MCCStringClone("rotation rate", r_string);
        default:
           return MCCStringClone("unknown", r_string);
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCSensorExecStartTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor, bool p_loosely)
{
#ifdef /* MCSensorExecStartTrackingSensor */ LEGACY_EXEC
    MCSystemStartTrackingSensor(p_sensor, p_loosely);
#endif /* MCSensorExecStartTrackingSensor */
}

void MCSensorExecStopTrackingSensor(MCExecContext& ctxt, MCSensorType p_sensor)
{
#ifdef /* MCSensorExecStopTrackingSensor */ LEGACY_EXEC
    MCSystemStopTrackingSensor(p_sensor);
#endif /* MCSensorExecStopTrackingSensor */
}

void MCSensorGetSensorAvailable(MCExecContext& ctxt, MCSensorType p_sensor, bool& r_available)
{
#ifdef /* MCSensorGetSensorAvailable */ LEGACY_EXEC
    MCSystemGetSensorAvailable(p_sensor, r_available);
#endif /* MCSensorGetSensorAvailable */
}

void MCSensorGetDetailedLocationOfDevice(MCExecContext& ctxt, MCVariableValue *&r_detailed_location)
{
#ifdef /* MCSensorGetDetailedLocationOfDevice */ LEGACY_EXEC
    MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, true))
    {
        MCVariableValue *t_location = nil;
        t_location = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        t_location->lookup_element(ctxt.GetEP(), "latitude", t_element);
        t_element->assign_real(t_reading.latitude);
        
        t_location->lookup_element(ctxt.GetEP(), "longitude", t_element);
        t_element->assign_real(t_reading.longitude);

        t_location->lookup_element(ctxt.GetEP(), "altitude", t_element);
        t_element->assign_real(t_reading.altitude);
        
        // MM-2013-02-21: Add speed and course to detailed location readings.
        if (t_reading.speed >= 0.0f)
        {
            t_location->lookup_element(ctxt.GetEP(), "speed", t_element);
            t_element->assign_real(t_reading.speed);
        }        
        if (t_reading.course >= 0.0f)
        {
            t_location->lookup_element(ctxt.GetEP(), "course", t_element);
            t_element->assign_real(t_reading.course);
        }
        
        t_location->lookup_element(ctxt.GetEP(), "timestamp", t_element);
        t_element->assign_real(t_reading.timestamp);
        
        t_location->lookup_element(ctxt.GetEP(), "horizontal accuracy", t_element);
        t_element->assign_real(t_reading.horizontal_accuracy);
        
        t_location->lookup_element(ctxt.GetEP(), "vertical accuracy", t_element);
        t_element->assign_real(t_reading.vertical_accuracy);
        
        r_detailed_location = t_location;
    }
#endif /* MCSensorGetDetailedLocationOfDevice */
}

void MCSensorGetLocationOfDevice(MCExecContext& ctxt, char *&r_location)
{
#ifdef /* MCSensorGetLocationOfDevice */ LEGACY_EXEC
    MCSensorLocationReading t_reading;
    if (MCSystemGetLocationReading(t_reading, false))
    {
        r_location = nil;
        MCCStringFormat(r_location, "%Lf,%Lf,%Lf", t_reading.latitude, t_reading.longitude, t_reading.altitude);
    }
#endif /* MCSensorGetLocationOfDevice */
}

void MCSensorGetDetailedHeadingOfDevice(MCExecContext& ctxt, MCVariableValue *&r_detailed_heading)
{
#ifdef /* MCSensorGetDetailedHeadingOfDevice */ LEGACY_EXEC
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
    {
        MCVariableValue *t_heading = nil;
        t_heading = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        t_heading->lookup_element(ctxt.GetEP(), "heading", t_element);
        t_element->assign_real(t_reading.heading);
        
        t_heading->lookup_element(ctxt.GetEP(), "magnetic heading", t_element);
        t_element->assign_real(t_reading.magnetic_heading);
        
        t_heading->lookup_element(ctxt.GetEP(), "true heading", t_element);
        t_element->assign_real(t_reading.true_heading);
        
        t_heading->lookup_element(ctxt.GetEP(), "x", t_element);
        t_element->assign_real(t_reading.x);
        
        t_heading->lookup_element(ctxt.GetEP(), "y", t_element);
        t_element->assign_real(t_reading.y);
        
        t_heading->lookup_element(ctxt.GetEP(), "z", t_element);
        t_element->assign_real(t_reading.z);
        
        t_heading->lookup_element(ctxt.GetEP(), "timestamp", t_element);
        t_element->assign_real(t_reading.timestamp);
        
        t_heading->lookup_element(ctxt.GetEP(), "accuracy", t_element);
        t_element->assign_real(t_reading.accuracy);

        r_detailed_heading = t_heading;
    }    
#endif /* MCSensorGetDetailedHeadingOfDevice */
}

void MCSensorGetHeadingOfDevice(MCExecContext& ctxt, char *&r_heading)
{
#ifdef /* MCSensorGetHeadingOfDevice */ LEGACY_EXEC
    MCSensorHeadingReading t_reading;
    if (MCSystemGetHeadingReading(t_reading, true))
    {
        r_heading = nil;
        MCCStringFormat(r_heading, "%Lf", t_reading.heading);
    }
#endif /* MCSensorGetHeadingOfDevice */
}

void MCSensorGetDetailedAccelerationOfDevice(MCExecContext& ctxt, MCVariableValue *&r_detailed_acceleration)
{
#ifdef /* MCSensorGetDetailedAccelerationOfDevice */ LEGACY_EXEC
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
    {
        MCVariableValue *t_acceleration = nil;
        t_acceleration = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        t_acceleration->lookup_element(ctxt.GetEP(), "x", t_element);
        t_element->assign_real(t_reading.x);
        
        t_acceleration->lookup_element(ctxt.GetEP(), "y", t_element);
        t_element->assign_real(t_reading.y);
        
        t_acceleration->lookup_element(ctxt.GetEP(), "z", t_element);
        t_element->assign_real(t_reading.z);
        
        t_acceleration->lookup_element(ctxt.GetEP(), "timestamp", t_element);
        t_element->assign_real(t_reading.timestamp);
        
        r_detailed_acceleration = t_acceleration;
    }    
#endif /* MCSensorGetDetailedAccelerationOfDevice */
}

void MCSensorGetAccelerationOfDevice(MCExecContext& ctxt, char *&r_acceleration)
{
#ifdef /* MCSensorGetAccelerationOfDevice */ LEGACY_EXEC
    MCSensorAccelerationReading t_reading;
    if (MCSystemGetAccelerationReading(t_reading, true))
    {
        r_acceleration = nil;
        MCCStringFormat(r_acceleration, "%Lf,%Lf,%Lf", t_reading.x, t_reading.y, t_reading.z);
    }
#endif /* MCSensorGetAccelerationOfDevice */
}

void MCSensorGetDetailedRotationRateOfDevice(MCExecContext& ctxt, MCVariableValue *&r_detailed_rotation_rate)
{
#ifdef /* MCSensorGetDetailedRotationRateOfDevice */ LEGACY_EXEC
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
        MCVariableValue *t_rotation_rate = nil;
        t_rotation_rate = new MCVariableValue();
        
        MCVariableValue *t_element = nil;
        
        t_rotation_rate->lookup_element(ctxt.GetEP(), "x", t_element);
        t_element->assign_real(t_reading.x);
        
        t_rotation_rate->lookup_element(ctxt.GetEP(), "y", t_element);
        t_element->assign_real(t_reading.y);
        
        t_rotation_rate->lookup_element(ctxt.GetEP(), "z", t_element);
        t_element->assign_real(t_reading.x);
        
        t_rotation_rate->lookup_element(ctxt.GetEP(), "timestamp", t_element);
        t_element->assign_real(t_reading.timestamp);
        
        r_detailed_rotation_rate = t_rotation_rate;
    }    
#endif /* MCSensorGetDetailedRotationRateOfDevice */
}

void MCSensorGetRotationRateOfDevice(MCExecContext& ctxt, char *&r_rotation_rate)
{
#ifdef /* MCSensorGetRotationRateOfDevice */ LEGACY_EXEC
    MCSensorRotationRateReading t_reading;
    if (MCSystemGetRotationRateReading(t_reading, true))
    {
        r_rotation_rate = nil;
        MCCStringFormat(r_rotation_rate, "%Lf,%Lf,%Lf", t_reading.x, t_reading.y, t_reading.z);
    }
#endif /* MCSensorGetRotationRateOfDevice */
}

// MM-2012-02-11: Added support for iPhoneGet/SetCalibrationTimeout
void MCSensorSetLocationCalibration(MCExecContext& ctxt, int32_t p_timeout)
{
#ifdef /* MCSensorSetLocationCalibration */ LEGACY_EXEC
    MCSystemSetLocationCalibrationTimeout(p_timeout);
#endif /* MCSensorSetLocationCalibration */
}

void MCSensorGetLocationCalibration(MCExecContext& ctxt, int32_t& r_timeout)
{
#ifdef /* MCSensorGetLocationCalibration */ LEGACY_EXEC
    MCSystemGetLocationCalibrationTimeout(r_timeout);
#endif /* MCSensorGetLocationCalibration */
}

////////////////////////////////////////////////////////////////////////////////

class MCSensorErrorEvent: public MCCustomEvent
{
public:
	MCSensorErrorEvent(MCSensorType p_sensor, const char *p_error)
	{
        m_sensor = p_sensor;
        /* UNCHECKED */ MCCStringClone(p_error, m_error);
	}
	
	void Destroy(void)
	{
        MCCStringFree(m_error);
		delete this;
	}
	
	void Dispatch(void)
	{
        char *t_sensor_name = nil;
        /* UNCHECKED */ MCSensorTypeToCString(m_sensor, t_sensor_name);   
        
        MCParameter p1, p2;
        p1.sets_argument(t_sensor_name);
        p1.setnext(&p2);
        p2.sets_argument(m_error);        
        
        MCdefaultstackptr->getcurcard()->message(MCM_tracking_error, &p1);
        /* UNCHECKED */ MCCStringFree(t_sensor_name);
	}
	
private:
    MCSensorType m_sensor;
    char *m_error;
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

void MCSensorPostErrorMessage(MCSensorType p_sensor, const char *p_error)
{
    MCCustomEvent *t_event = nil;
	t_event = new MCSensorErrorEvent(p_sensor, p_error);
	MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleStartTrackingSensor(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStartTrackingSensor */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_loosely = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_loosely = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStartTrackingSensor(t_ctxt, t_sensor, t_loosely);
    }
    
    return t_ctxt.GetStat();
#endif /* MCHandleStartTrackingSensor */
}

Exec_stat MCHandleStopTrackingSensor(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleStopTrackingSensor */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();

    if (t_sensor != kMCSensorTypeUnknown)
    {
        MCSensorExecStopTrackingSensor(t_ctxt, t_sensor);
    }
    
    return t_ctxt.GetStat();
#endif /* MCHandleStopTrackingSensor */
}

// MM-2012-02-11: Added support old style senseor syntax (iPhoneEnableAcceleromter etc)
Exec_stat MCHandleAccelerometerEnablement(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleAccelerometerEnablement */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeAcceleration, false);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeAcceleration);
    
    return t_ctxt.GetStat();
#endif /* MCHandleAccelerometerEnablement */
}

Exec_stat MCHandleLocationTrackingState(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleLocationTrackingState */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeLocation, false);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeLocation);
    
    return t_ctxt.GetStat();
#endif /* MCHandleLocationTrackingState */
}

Exec_stat MCHandleHeadingTrackingState(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleHeadingTrackingState */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	if ((bool)p_context)
        MCSensorExecStartTrackingSensor(t_ctxt, kMCSensorTypeHeading, true);
    else
        MCSensorExecStopTrackingSensor(t_ctxt, kMCSensorTypeHeading);
    
    return t_ctxt.GetStat();
#endif /* MCHandleHeadingTrackingState */
}

Exec_stat MCHandleSensorReading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSensorReading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    MCSensorType t_sensor = kMCSensorTypeUnknown;
    bool t_detailed = false;
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }
    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_detailed = ep . getsvalue() == MCtruemcstring;
    }
    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCAutoRawCString t_reading;

    switch (t_sensor)
    {
        case kMCSensorTypeLocation:
        {
            if (t_detailed)
                MCSensorGetDetailedLocationOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetLocationOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeHeading:
        {
            if (t_detailed)
                MCSensorGetDetailedHeadingOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetHeadingOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeAcceleration:
        {
            if (t_detailed)
                MCSensorGetDetailedAccelerationOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetAccelerationOfDevice(t_ctxt, t_reading);
            break;
        }
        case kMCSensorTypeRotationRate:
        {
            if (t_detailed)
                MCSensorGetDetailedRotationRateOfDevice(t_ctxt, t_detailed_reading);
            else
                MCSensorGetRotationRateOfDevice(t_ctxt, t_reading);
            break;
        }
    }
    
    if (t_detailed)
    {
        if (t_detailed_reading != nil)
            ep.setarray(t_detailed_reading, True);
    }
    else
    {
        if (t_reading.Borrow() != nil)
            ep.copysvalue(t_reading.Borrow());
    }
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleSensorReading */
}

// MM-2012-02-11: Added support old style senseor syntax (iPhoneGetCurrentLocation etc)
Exec_stat MCHandleCurrentLocation(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCurrentLocation */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCSensorGetDetailedLocationOfDevice(t_ctxt, t_detailed_reading);
    if (t_detailed_reading != nil)
        ep.setarray(t_detailed_reading, True);
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleCurrentLocation */
}

Exec_stat MCHandleCurrentHeading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCurrentHeading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    MCVariableValue *t_detailed_reading = nil;
    MCSensorGetDetailedHeadingOfDevice(t_ctxt, t_detailed_reading);
    if (t_detailed_reading != nil)
        ep.setarray(t_detailed_reading, True);
    
    MCresult->store(ep, False);
    return t_ctxt.GetStat();
#endif /* MCHandleCurrentHeading */
}

Exec_stat MCHandleSetHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetHeadingCalibrationTimeout */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_timeout = atoi(ep.getcstring());
    }
    MCSensorSetLocationCalibration(t_ctxt, t_timeout);
    
    return t_ctxt.GetStat();
#endif /* MCHandleSetHeadingCalibrationTimeout */
}

Exec_stat MCHandleHeadingCalibrationTimeout(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleHeadingCalibrationTimeout */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    int t_timeout;
    MCSensorGetLocationCalibration(t_ctxt, t_timeout);
    MCresult->setnvalue(t_timeout);
    
    t_ctxt . SetTheResultToEmpty();
    return t_ctxt.GetStat();
#endif /* MCHandleHeadingCalibrationTimeout */
}

Exec_stat MCHandleSensorAvailable(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSensorAvailable */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();

    MCSensorType t_sensor;
    t_sensor = kMCSensorTypeUnknown;    
    if (p_parameters)
    {
        p_parameters->eval(ep);
        t_sensor = MCSensorTypeFromCString(ep.getcstring());
        p_parameters = p_parameters->getnext();
    }    
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, t_sensor, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleSensorAvailable */
}

Exec_stat MCHandleCanTrackLocation(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanTrackLocation */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
        
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, kMCSensorTypeLocation, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleCanTrackLocation */
}

Exec_stat MCHandleCanTrackHeading(void *p_context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCanTrackHeading */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);    
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
    bool t_available;
    t_available = false;
    MCSensorGetSensorAvailable(t_ctxt, kMCSensorTypeHeading, t_available);
    
    MCresult->sets(MCU_btos(t_available));
    return t_ctxt.GetStat();
#endif /* MCHandleCanTrackHeading */
}

////////////////////////////////////////////////////////////////////////////////
