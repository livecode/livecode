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

package com.runrev.android;

import android.content.*;
import android.os.*;
import android.util.*;
import java.util.*;

import android.hardware.*;
import android.location.*;

class SensorModule
{
    public static final String TAG = "revandroid.SensorModule";
    private Engine m_engine;
    
    private SensorManager m_sensor_manager;

	private AccelerationTracker m_accel_tracker;
    private LocationTracker m_location_tracker;
    private HeadingTracker m_heading_tracker;
    private RotationRateTracker m_rotation_rate_tracker;


    abstract class Tracker
    {
        public static final int NO_TRACKING = 0;
        public static final int COARSE_TRACKING = 1;
        public static final int FINE_TRACKING = 2;
        
        protected boolean m_paused;
        protected int m_tracking_requested;
        protected int m_tracking_internal;
        
        protected boolean m_registered;
        
        public Tracker()
        {
            m_paused = false;
            m_registered = false;
            m_tracking_requested = NO_TRACKING;
            m_tracking_internal = NO_TRACKING;
        }
        
        protected abstract boolean register(boolean p_loosely);
        protected abstract boolean unregister();
        
        public abstract boolean isAvailable();
        
        public boolean startTracking(boolean p_loosely)
        {
            return setTracking(p_loosely ? COARSE_TRACKING : FINE_TRACKING, m_tracking_internal);
        }
        
        public boolean startTrackingInternal(boolean p_loosely)
        {
            return setTracking(m_tracking_requested, p_loosely ? COARSE_TRACKING : FINE_TRACKING);
        }
        
        public boolean setTracking(int p_requested, int p_internal)
        {
            int t_previous, t_new;
            
            t_previous = Math.max(m_tracking_requested, m_tracking_internal);
            t_new = Math.max(p_requested, p_internal);
            
            if (t_previous == t_new)
            {
                m_tracking_requested = p_requested;
                m_tracking_internal = p_internal;
                return true;
            }
            else if (t_new == NO_TRACKING)
            {
                if (unregister())
                {
                    m_tracking_requested = p_requested;
                    m_tracking_internal = p_internal;
                    return true;
                }
                else
                    return false;
            }
            else
            {
                if (unregister() && register(t_new == COARSE_TRACKING))
                {
                    m_tracking_requested = p_requested;
                    m_tracking_internal = p_internal;
                    return true;
                }
                else
                    return false;
            }
        }

        public boolean stopTracking()
        {
            return setTracking(NO_TRACKING, m_tracking_internal);
        }
        
        public boolean stopTrackingInternal()
        {
            return setTracking(m_tracking_requested, NO_TRACKING);
        }
        
        public boolean isTracking()
        {
            return m_tracking_requested != NO_TRACKING || m_tracking_internal != NO_TRACKING;
        }
        
        public void pause()
        {
            if (m_paused)
                return;
            
            m_paused = true;
            if (isTracking())
                unregister();
        }
        
        public void resume()
        {
            if (!m_paused)
                return;
            
            m_paused = false;
            if (isTracking())
                register(Math.max(m_tracking_requested, m_tracking_internal) == COARSE_TRACKING);
        }
    }
    
    class AccelerationTracker extends Tracker implements SensorEventListener
    {
        private Sensor m_accelerometer;
        private float m_x = 0, m_y = 0, m_z = 0;
        
        public AccelerationTracker()
        {
            super();
            
            m_accelerometer = m_sensor_manager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }
        
        public boolean isAvailable()
        {
            return m_accelerometer != null;
        }

        protected boolean register(boolean p_loosely)
        {
            if (m_registered)
                return true;

            int t_sensor_delay;
            if (p_loosely)
                t_sensor_delay = SensorManager.SENSOR_DELAY_NORMAL;
            else
                t_sensor_delay = SensorManager.SENSOR_DELAY_GAME;
            
            m_registered = m_sensor_manager.registerListener(this, m_accelerometer, t_sensor_delay);
            return m_registered;
        }
        
        protected boolean unregister()
        {
            if (!m_registered)
                return true;

            m_sensor_manager.unregisterListener(this);
            m_registered = false;
            
            return true;
        }
        
        // SensorEventListener methods
        public void onAccuracyChanged(Sensor sensor, int accuracy)
        {
        }
        
        public void onSensorChanged(SensorEvent event)
        {
            if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
            {
				if (event.values[0] != m_x || event.values[1] != m_y || event.values[2] != m_z)
				{
                    m_x = event.values[0];
                    m_y = event.values[1];
                    m_z = event.values[2];
                    if (m_tracking_requested != NO_TRACKING)
                        m_engine.onAccelerationChanged(m_x, m_y, m_z, event.timestamp / 1.0e9f);
                }
            }
        }
        
        public float getX() { return m_x; }
        public float getY() { return m_y; }
        public float getZ() { return m_z; }
    }
    
    class LocationTracker extends Tracker
    {
        boolean m_use_gps;
        boolean m_gps_available;
        
        // Offset (in seconds) between GPS monotonic timestamps and wall-clock time
        double m_timestamp_offset;
        
        Location m_last_location;
        
        LocationManager m_location_manager;
        
        LocationListener m_location_listener;
        
        public LocationTracker()
        {
            // Get the number of seconds since the device was booted
            double t_seconds_since_boot;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
                t_seconds_since_boot = SystemClock.elapsedRealtimeNanos() / 1.0e9;
            else
                t_seconds_since_boot = SystemClock.elapsedRealtime() / 1.0e3;
            
            // Get the current wall-clock time in seconds since the Unix epoch
            double t_seconds_since_epoch = System.currentTimeMillis() / 1000.0;
            
            // Calculate the offset between the since-boot monotonic clock and
            // the current wall-clock time
            m_timestamp_offset = t_seconds_since_epoch - t_seconds_since_boot;
            
            m_location_manager = (LocationManager)m_engine.getContext().getSystemService(Context.LOCATION_SERVICE);
            m_location_listener = new LocationListener()
            {
                // LocationListener methods
                public void onLocationChanged(Location location)
                {
                    if (m_gps_available)
                        return;
                    LocationTracker.this.onLocationChanged(location);
                }
                
                public void onProviderDisabled(String provider)
                {
                    m_gps_available = false;
                }
                
                public void onProviderEnabled(String provider)
                {
                }
                
                public void onStatusChanged(String provider, int status, Bundle extras)
                {
                    m_gps_available = status == LocationProvider.AVAILABLE;
                }
            };
            
            m_last_location = null;
            m_gps_available = false;
        }
        
        public boolean isAvailable()
        {
            List<String> t_providers;
            t_providers = m_location_manager.getProviders(true);
            return t_providers != null && !t_providers.isEmpty();
        }
        
        protected boolean register(boolean p_loosely)
        {
            if (m_registered)
                return true;
            
            m_use_gps = !p_loosely;
            
            Criteria t_criteria = new Criteria();
            if (m_use_gps)
                t_criteria.setAccuracy(Criteria.ACCURACY_FINE);
            else
                t_criteria.setAccuracy(Criteria.ACCURACY_COARSE);
            
            try
            {
                m_location_manager.requestLocationUpdates(0, 0, t_criteria, m_location_listener, null);
                m_registered = true;
            }
            catch (SecurityException e)
            {
            }
            
            return m_registered;
        }
        
        protected boolean unregister()
        {
            if (!m_registered)
                return true;
            
            m_location_manager.removeUpdates(m_location_listener);
            // MM-2011-03-13: [[ Bug 10077 ]] Make sure we flag as unregistered or else we will never be able to register again.5
            m_registered = false;
            return true;
        }
        
        protected void onLocationChanged(Location location)
        {
            m_last_location = new Location(location);
            if (m_tracking_requested != NO_TRACKING)
            {
                // MM-2013-02-21: Added speed and course to location reading.
                double t_speed;
                if (location.hasSpeed())
                    t_speed = location.getSpeed();
                else
                    t_speed = -1.0f;
                double t_course;
                if (location.hasBearing())
                    t_course = location.getBearing();
                else
                    t_course = -1.0f;

                // Get an approximate wall-clock time of the fix. This can drift
                // over time if the wall-clock time is adjusted (forwards or
                // backwards) but for location-tracking purposes, a monotonic
                // timestamp is more important than one that matches the wall-
                // clock time.
                //
                // For devices before Jelly Bean, this isn't possible and so the
                // normal wall-clock time has to be used instead.
                double t_timestamp;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
                    t_timestamp = (location.getElapsedRealtimeNanos() / 1.0e9) + m_timestamp_offset;
                else
                    t_timestamp = location.getTime() / 1000.0;
                
                m_engine.onLocationChanged(location.getLatitude(), location.getLongitude(), location.getAltitude(),
                                           t_timestamp, location.getAccuracy(), t_speed, t_course);
            }
        }
        
        public boolean hasLocation() { return m_last_location != null; }
        
        public double getLatitude() { return m_last_location.getLatitude(); }
        public double getLongitude() { return m_last_location.getLongitude(); }
        public double getAltitude() { return m_last_location.getAltitude(); }
        
        //override to make sure currently known location is sent if already started by heading tracker
        public boolean startTracking(boolean p_loosely)
        {
            boolean t_result;
            t_result = super.startTracking(p_loosely);
            if (t_result && m_last_location != null)
                onLocationChanged(m_last_location);
            return t_result;
        }
    }
    
    class HeadingTracker extends Tracker implements SensorEventListener
    {
        private Sensor m_magnetometer;
        private float m_x = 0, m_y = 0, m_z = 0;
        
        public HeadingTracker()
        {
            super();
            
            m_magnetometer = m_sensor_manager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        }
        
        public boolean isAvailable()
        {
            return m_magnetometer != null;
        }
        
        protected boolean register(boolean p_loosely)
        {
            if (m_registered)
                return true;
            
            int t_sensor_delay;
            if (p_loosely)
                t_sensor_delay = SensorManager.SENSOR_DELAY_NORMAL;
            else
                t_sensor_delay = SensorManager.SENSOR_DELAY_GAME;
            
            if (p_loosely)
                m_registered = m_accel_tracker.startTrackingInternal(true) &&
                m_sensor_manager.registerListener(this, m_magnetometer, t_sensor_delay);
            else
                m_registered = m_accel_tracker.startTrackingInternal(true) && m_location_tracker.startTrackingInternal(true) &&
                m_sensor_manager.registerListener(this, m_magnetometer, t_sensor_delay);
                
            return m_registered;
        }
        
        protected boolean unregister()
        {
            m_accel_tracker.stopTrackingInternal();
            m_location_tracker.stopTrackingInternal();
            
            if (!m_registered)
                return true;
            
            m_sensor_manager.unregisterListener(this);
            m_registered = false;
            
            return true;
        }
        
        // SensorEventListener methods
        public void onAccuracyChanged(Sensor sensor, int accuracy)
        {
        }
        
        private double toHeading(double degrees)
        {
            while (degrees < 0.0)
                degrees += 360.0;
            while (degrees >= 360.0)
                degrees -= 360.0;
            
            return degrees;
        }
        
        public void onSensorChanged(SensorEvent event)
        {
            if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
            {
                if (!m_accel_tracker.isTracking())
                    return;
                
                float x, y, z;
                double t_magnetic_heading = 0;
                double t_true_heading = 0;
                double t_heading = 0;
                
                x = event.values[0];
                y = event.values[1];
                z = event.values[2];
                
                float t_gravity[] = new float[3];
                t_gravity[0] = m_accel_tracker.getX();
                t_gravity[1] = m_accel_tracker.getY();
                t_gravity[2] = m_accel_tracker.getZ();
                
                float t_inclination[] = new float[9];
                float t_rotation[] = new float[9];
                
                m_sensor_manager.getRotationMatrix(t_rotation, t_inclination, t_gravity, event.values);
                
                float t_orientation[] = new float[3];
                
                m_sensor_manager.getOrientation(t_rotation, t_orientation);
                                
                t_magnetic_heading = toHeading(Math.toDegrees(t_orientation[0]));
                
                if (m_location_tracker.hasLocation())
                {
                    GeomagneticField t_gmf = new GeomagneticField((float)m_location_tracker.getLatitude(),
                                                                  (float)m_location_tracker.getLongitude(),
                                                                  (float)m_location_tracker.getAltitude(),
                                                                  System.currentTimeMillis());
                    t_true_heading = toHeading(Math.toDegrees(t_orientation[0]) + t_gmf.getDeclination());
                    
                    t_heading = t_true_heading;
                }
                else
                    t_heading = t_magnetic_heading;
                
                m_engine.onHeadingChanged(t_heading, t_magnetic_heading, t_true_heading,
                                          event.timestamp / 1.0e9f, x, y, z, event.accuracy);
            }
        }
    }
    
    class RotationRateTracker extends Tracker implements SensorEventListener
    {
        private Sensor m_gyroscope;
        private float m_x = 0, m_y = 0, m_z = 0;
        
        public RotationRateTracker()
        {
            super();
            
            m_gyroscope = m_sensor_manager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
            Log.i(TAG, "gyroscope sensor: " + m_gyroscope);
        }
        
        public boolean isAvailable()
        {
            return m_gyroscope != null;
        }
        
        protected boolean register(boolean p_loosely)
        {
            if (m_registered)
                return true;
            
            int t_sensor_delay;
            if (p_loosely)
                t_sensor_delay = SensorManager.SENSOR_DELAY_NORMAL;
            else
                t_sensor_delay = SensorManager.SENSOR_DELAY_GAME;
            
            m_registered = m_sensor_manager.registerListener(this, m_gyroscope, t_sensor_delay);
            Log.i(TAG, "gyroscope registered: " + m_registered);
            return m_registered;
        }
        
        protected boolean unregister()
        {
            if (!m_registered)
                return true;
            
            m_sensor_manager.unregisterListener(this);
            m_registered = false;
            
            return true;
        }
        
        // SensorEventListener methods
        public void onAccuracyChanged(Sensor sensor, int accuracy)
        {
        }
        
        public void onSensorChanged(SensorEvent event)
        {
            if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE)
            {
                Log.i(TAG, "gyroscope event");
				if (event.values[0] != m_x || event.values[1] != m_y || event.values[2] != m_z)
				{
                    m_x = event.values[0];
                    m_y = event.values[1];
                    m_z = event.values[2];
                    if (m_tracking_requested != NO_TRACKING)
                        m_engine.onRotationRateChanged(m_x, m_y, m_z, event.timestamp / 1.0e9f);
                }
            }
        }
        
        public float getX() { return m_x; }
        public float getY() { return m_y; }
        public float getZ() { return m_z; }
    }

    public SensorModule(Engine p_engine)
    {
        m_engine = p_engine;
        
        m_sensor_manager = (SensorManager)m_engine.getContext().getSystemService(Context.SENSOR_SERVICE);
        
		// Create listeners for acceleration & shake events
		m_accel_tracker = new AccelerationTracker();
        m_location_tracker = new LocationTracker();
        m_heading_tracker = new HeadingTracker();
        m_rotation_rate_tracker = new RotationRateTracker();
    }
    
    public static final int SENSOR_UNKNOWN=0;
    public static final int SENSOR_LOCATION=1;
    public static final int SENSOR_HEADING=2;
    public static final int SENSOR_ACCELERATION=3;
    public static final int SENSOR_ROTATION_RATE=4;
    
    public boolean isSensorAvailable(int p_sensor)
    {
        int t_type = 0;
        switch (p_sensor)
        {
            case SENSOR_LOCATION:
                return m_location_tracker.isAvailable();
            case SENSOR_HEADING:
                return m_heading_tracker.isAvailable();
            case SENSOR_ACCELERATION:
                return m_accel_tracker.isAvailable();
            case SENSOR_ROTATION_RATE:
                return m_rotation_rate_tracker.isAvailable();
            default:
                return false;
        }
    }
    
    public boolean isLocationAvailable()
    {
        return true;
    }
    
    public boolean startTrackingLocation(boolean p_loosely)
    {
        return m_location_tracker.startTracking(p_loosely);
    }
    
    public boolean stopTrackingLocation()
    {
        return m_location_tracker.stopTracking();
    }
    
    public boolean startTrackingHeading(boolean p_loosely)
    {
        return m_heading_tracker.startTracking(p_loosely);
    }
    
    public boolean stopTrackingHeading()
    {
        return m_heading_tracker.stopTracking();
    }
    
    public boolean startTrackingAcceleration(boolean p_loosely)
    {
        return m_accel_tracker.startTracking(p_loosely);
    }
    
    public boolean stopTrackingAcceleration()
    {
        return m_accel_tracker.stopTracking();
    }
    
    public boolean startTrackingRotationRate(boolean p_loosely)
    {
        return m_rotation_rate_tracker.startTracking(p_loosely);
    }
    
    public boolean stopTrackingRotationRate()
    {
        return m_rotation_rate_tracker.stopTracking();
    }
    
    public void onPause()
    {
        m_accel_tracker.pause();
        m_location_tracker.pause();
    }
    
    public void onResume()
    {
        m_accel_tracker.resume();
        m_location_tracker.resume();
    }
    
    // MM-2012-03-19: [[ Bug 10104 ]] Stop tracking any sensors on shutdown - not doing so prevents a restart for some reason.
    public void finish()
    {
        m_rotation_rate_tracker.stopTracking();
        m_heading_tracker.stopTracking(); 
        m_accel_tracker.stopTracking();
        m_location_tracker.stopTracking();
    }
}
