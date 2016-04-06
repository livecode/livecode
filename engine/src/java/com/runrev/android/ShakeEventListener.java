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

import android.hardware.*;
import android.content.*;

// shake detection class

abstract class ShakeEventListener extends AccelerationChangeListener implements SensorEventListener
{
	public static final int SHAKE_BEGAN = 0;
	public static final int SHAKE_CANCELLED = 1;
	public static final int SHAKE_ENDED = 2;

	private long m_lastUpdate = -1;
	private float m_x, m_y, m_z;
	private float m_last_x, m_last_y, m_last_z;
	private boolean m_shaking = false;
	
	private static final int SHAKE_THRESHOLD = 800;
	
	public ShakeEventListener(Context p_context)
	{
		super(p_context, SensorManager.SENSOR_DELAY_NORMAL);
	}
	
	public void onAccelerationChanged(float p_x, float p_y, float p_z, long p_timestamp)
	{
		long t_curtime = System.currentTimeMillis();
		
		if (m_lastUpdate == -1)
		{
			m_last_x = p_x;
			m_last_y = p_y;
			m_last_z = p_z;
			m_lastUpdate = t_curtime;
		}
		
		m_x = p_x;
		m_y = p_y;
		m_z = p_z;
		
		if ((t_curtime - m_lastUpdate) > 100)
		{	
			float t_speed = (Math.abs(m_x - m_last_x) + Math.abs(m_y - m_last_y) + Math.abs(m_z- m_last_z)) / (t_curtime - m_lastUpdate) * 10000;
			
			m_lastUpdate = t_curtime;
			m_last_x = m_x;
			m_last_y = m_y;
			m_last_z = m_z;
			
			if (t_speed > SHAKE_THRESHOLD)
			{
				if (!m_shaking)
				{
					m_shaking = true;
					onShake(SHAKE_BEGAN, p_timestamp);
				}
			}
			else
			{
				if (m_shaking)
				{
					m_shaking = false;
					onShake(SHAKE_ENDED, p_timestamp);
				}
			}
		}
	}
	
	public abstract void onShake(int type, long timestamp);
}

