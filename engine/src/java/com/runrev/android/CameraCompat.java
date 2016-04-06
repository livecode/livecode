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

import android.util.Log;
import android.hardware.*;
import java.lang.reflect.*;

class CameraCompat
{
	private static Method mCamera_getNumberOfCameras;
	private static Method mCamera_getCameraInfo;
	private static Class mCameraInfo_class;
	private static Object mCameraInfo_instance;
	private static Field mCameraInfo_facing;
	private static Field mCameraInfo_orientation;
	static
	{
		initCompatibility();
	}
	
	static class CameraInfo
	{
		public static final int CAMERA_FACING_BACK = 0x00000000;
		public static final int CAMERA_FACING_FRONT = 0x00000001;
		
		public int facing;
		public int orientation;
	}
	private static void initCompatibility()
	{
		Log.i("revandroid", "initialising CameraCompat class");
		try
		{
			mCamera_getNumberOfCameras = Camera.class.getMethod("getNumberOfCameras", (Class[])null);
		}
		catch (Exception e)
		{
			mCamera_getNumberOfCameras = null;
		}
		
		try
		{
			mCameraInfo_class = ClassLoader.getSystemClassLoader().loadClass("android.hardware.Camera$CameraInfo");
			
			Constructor t_constructor = mCameraInfo_class.getConstructor((Class[])null);
			mCameraInfo_instance = t_constructor.newInstance((Object[])null);
			
			mCameraInfo_facing = mCameraInfo_class.getField("facing");
			mCameraInfo_orientation = mCameraInfo_class.getField("orientation");
		}
		catch (Exception e)
		{
			mCameraInfo_class = null;
			mCameraInfo_instance = null;
			
			mCameraInfo_facing = null;
			mCameraInfo_orientation = null;
		}
		
		try
		{
			mCamera_getCameraInfo = Camera.class.getMethod("getCameraInfo", new Class[] { Integer.TYPE, mCameraInfo_class });
		}
		catch (Exception e)
		{
			mCamera_getCameraInfo = null;
		}

	}
	
	public static int getNumberOfCameras()
	{
		if (mCamera_getNumberOfCameras == null)
		{
			try
			{
				Camera t_cam = Camera.open();
				if (t_cam == null)
					return 0;
				t_cam.release();
				return 1;
			}
			catch (Exception e)
			{
				Log.i("revandroid", "can't open camera: " + e.toString());
				return 0;
			}
		}
		else
		{
			Object t_result;
			try
			{
				t_result = mCamera_getNumberOfCameras.invoke(null, (Object[]) null);
				return (Integer)t_result;
			}
			catch (Exception e)
			{
				return 0;
			}
		}
	}
	
	public static void getCameraInfo(int i, CameraCompat.CameraInfo info)
	{
		if (mCamera_getCameraInfo == null)
		{
			info.facing = CameraCompat.CameraInfo.CAMERA_FACING_BACK;
			info.orientation = 0;
			
			return;
		}
		else
		{
			try
			{
				mCamera_getCameraInfo.invoke(null, new Object[] { new Integer(i), mCameraInfo_instance });
				info.facing = mCameraInfo_facing.getInt(mCameraInfo_instance);
				info.orientation = mCameraInfo_orientation.getInt(mCameraInfo_instance);
			}
			catch (Exception e)
			{
				return;
			}
		}
	}
}
