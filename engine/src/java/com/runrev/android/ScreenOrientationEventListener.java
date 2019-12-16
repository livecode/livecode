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

import android.view.*;
import android.content.*;
import android.util.*;

public abstract class ScreenOrientationEventListener extends OrientationEventListener
{
	int m_orientation = 0;
	boolean m_orientation_known = false;
	
	public int getOrientation()
	{
		return m_orientation;
	}
	
	public ScreenOrientationEventListener(Context p_context, int p_initial_orientation)
	{
		super(p_context);
		m_orientation = p_initial_orientation;
	}
	
	public void onOrientationChanged(int orientation)
	{
		if (m_orientation_known && orientation != -1 && m_orientation != -1)
		{
			int t_angle;
			
			t_angle = orientation - m_orientation;
			if (t_angle < 0)
				t_angle += 360;
			if (t_angle <= 60 || t_angle >= 300)
				return;
		}
		
		int t_new_orientation;
		if (orientation == -1)
			t_new_orientation = -1;
		else if (orientation < 45)
			t_new_orientation = 0;
		else if (orientation < 135)
			t_new_orientation = 90;
		else if (orientation < 225)
			t_new_orientation = 180;
		else if (orientation < 315)
			t_new_orientation = 270;
		else
			t_new_orientation = 0;
		
		m_orientation = t_new_orientation;

		if (m_orientation_known)
			onScreenOrientationChanged(t_new_orientation);
		else
			m_orientation_known = true;
	}
	
	public abstract void onScreenOrientationChanged(int orientation);
}
