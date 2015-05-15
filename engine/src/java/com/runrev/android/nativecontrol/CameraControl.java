/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

package com.runrev.android.nativecontrol;

import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import java.io.IOException;
import java.util.List;

class CameraControl extends NativeControl
{
	public static final String TAG = "revandroid.NativeCameraControl";
	
	public static final int DEVICE_DEFAULT = 1;
	public static final int DEVICE_FRONT = 2;
	public static final int DEVICE_BACK = 4;
	
	public static final int FLASH_OFF = 0;
	public static final int FLASH_ON = 1;
	public static final int FLASH_AUTO = 2;
	
	public static final int FEATURE_FLASH = 1;
	public static final int FEATURE_FLASH_MODE = 2;
	
	private int m_devices;
	private int m_default;
	private Camera.CameraInfo[] m_camera_info;
	private int[] m_features;
	
	private int m_device;
	private int m_flash_mode;
	
	private boolean m_surface_ready;
	private boolean m_record;
	private boolean m_is_recording;
	private Camera m_camera;
	
	public CameraControl(NativeControlModule p_module)
	{
		super(p_module);
		
		Log.i(TAG, "new");

		m_device = DEVICE_DEFAULT;

		m_camera_info = null;
		m_devices = 0;
		m_flash_mode = FLASH_AUTO;
		
		m_surface_ready = false;
		m_record = false;
		m_is_recording = false;
		m_camera = null;
	}
	
	@Override
	public View createView(Context p_context)
	{
		SurfaceView t_view;
		t_view = new SurfaceView(p_context);
		t_view.getHolder().addCallback(new SurfaceHolder.Callback() {
			@Override
			public void surfaceCreated(SurfaceHolder p_holder)
			{
				m_surface_ready = true;
				enablePreview();
			}
			
			@Override
			public void surfaceDestroyed(SurfaceHolder p_holder)
			{
				m_surface_ready = false;
				disablePreview();
			}
			
			@Override
			public void surfaceChanged(SurfaceHolder p_holder, int p_format, int p_width, int p_height)
			{
				
			}
		});
		
		t_view.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		
		return t_view;
	}
	
	//////////
	
	private int getCameraOrientationSetting()
	{
		fetchCameraInfo();
		
		int t_id;
		t_id = currentCameraId();
		
		if (t_id == -1)
			return 0;
		
		int t_rotation;
		t_rotation = NativeControlModule.getActivity().getWindowManager().getDefaultDisplay().getRotation();
		
		switch (t_rotation)
		{
			case Surface.ROTATION_0:
				t_rotation = 0;
				break;
			case Surface.ROTATION_90:
				t_rotation = 90;
				break;
			case Surface.ROTATION_180:
				t_rotation = 180;
				break;
			case Surface.ROTATION_270:
				t_rotation = 270;
				break;
		}
		
		// Reverse direction for front-facing camera
		if (m_camera_info[t_id].facing == CameraInfo.CAMERA_FACING_FRONT)
			t_rotation = 360 - t_rotation;
		
		return (m_camera_info[t_id].orientation + t_rotation) % 360;
	}
	
	private void enablePreview()
	{
		if (m_is_recording)
			return;
		
		if (!(m_surface_ready && m_record))
			return;
		
		int t_id;
		t_id = currentCameraId();
		
		if (t_id == -1)
			return;
		
		Camera t_cam;
		try
		{
			t_cam = Camera.open(t_id);
		}
		catch (Exception e)
		{
			return;
		}
		
		SurfaceView t_preview;
		t_preview = (SurfaceView)getView();
		
		try
		{
			t_cam.setDisplayOrientation(getCameraOrientationSetting());
			t_cam.setPreviewDisplay(t_preview.getHolder());
			t_cam.startPreview();
		}
		catch (IOException e)
		{
			t_cam.release();
			return;
		}
		
		m_camera = t_cam;
		m_is_recording = true;
	}
	
	private void disablePreview()
	{
		if (!m_is_recording)
			return;
		
		if (m_camera == null)
			return;
		
		m_camera.stopPreview();
		m_camera.release();
		m_camera = null;
		m_is_recording = false;
	}
	
	//////////

	private int fetchFeatures(int p_cam_id)
	{
		Camera t_cam;
		
		try
		{
			t_cam = Camera.open(p_cam_id);
		}
		catch (Exception e)
		{
			return -1;
		}
		
		Camera.Parameters t_params;
		t_params = t_cam.getParameters();
		
		int t_features;
		t_features = 0;
		
		// TODO - revisit: can we assume that all cameras support on/off/auto modes?
		//    Also, does no flash modes mean no flash unit?
		List<String> t_flash_modes;
		t_flash_modes = t_params.getSupportedFlashModes();
		
		if (t_flash_modes != null)
		{
			t_features |= FEATURE_FLASH;
			t_features |= FEATURE_FLASH_MODE;
		}
		
		t_cam.release();
		
		return t_features;
	}
	
	private void fetchCameraInfo()
	{
		if (m_camera_info == null)
		{
			int t_num_devices;
			t_num_devices = Camera.getNumberOfCameras();
			
			if (t_num_devices > 0)
				m_devices |= DEVICE_DEFAULT;
			
			m_camera_info = new CameraInfo[t_num_devices];
			m_features = new int[t_num_devices];
			
			for (int i = 0; i < t_num_devices; i++)
			{
				m_camera_info[i] = new CameraInfo();
				Camera.getCameraInfo(i, m_camera_info[i]);

				if (m_camera_info[i].facing == CameraInfo.CAMERA_FACING_FRONT)
					m_devices |= DEVICE_FRONT;
				else if (m_camera_info[i].facing == CameraInfo.CAMERA_FACING_BACK)
					m_devices |= DEVICE_BACK;
				
				m_features[i] = fetchFeatures(i);
			}
		}
	}
	
	public int currentCameraId()
	{
		fetchCameraInfo();
		
		int t_cam;
		t_cam = -1;
		
		for (int i = 0; i < m_camera_info.length; i++)
		{
			switch (m_device)
			{
				case DEVICE_BACK:
					// return first back-facing camera
					if (m_camera_info[i].facing == CameraInfo.CAMERA_FACING_BACK)
						return i;
					break;
					
				case DEVICE_FRONT:
					// return first front-facing camera
					if (m_camera_info[i].facing == CameraInfo.CAMERA_FACING_FRONT)
						return i;
					break;
				
				case DEVICE_DEFAULT:
					// return first back-facing camera, or first front-facing otherwise
					if (m_camera_info[i].facing == CameraInfo.CAMERA_FACING_BACK)
						return i;
					else if (t_cam == -1)
						t_cam = i;
					break;
			}
		}
		
		return t_cam;
	}
	
	public int getDevices()
	{
		fetchCameraInfo();
		return m_devices;
	}
	
	public void setDevice(int p_device)
	{
		fetchCameraInfo();
		
		if (p_device != DEVICE_DEFAULT && p_device != DEVICE_FRONT && p_device != DEVICE_BACK)
		{
			// TODO - error: invalid device parameter
			return;
		}
		
		if ((m_devices & p_device) == 0)
		{
			// TODO - error: device not available
			return;
		}
		
		m_device = p_device;
	}
	
	public int getDevice()
	{
		return m_device;
	}
	
	public int getFeatures()
	{
		int t_cam;
		t_cam = currentCameraId();
		
		if (t_cam == -1)
			return -1;
		
		return m_features[t_cam];
	}
	
	public void setFlashMode(int p_mode)
	{
		int t_cam;
		t_cam = currentCameraId();
		
		if (t_cam == -1)
		{
			// TODO - error: selected camera device unavailable
			return;
		}
		
		if ((m_features[t_cam] & FEATURE_FLASH_MODE) == 0)
		{
			// TODO - error: cannot set flash mode of selected camera device
			return;
		}
		
		m_flash_mode = p_mode;
	}
	
	public int getFlashMode()
	{
		int t_cam;
		t_cam = currentCameraId();
		
		if (t_cam == -1)
			return -1;
		
		if ((m_features[t_cam] & FEATURE_FLASH_MODE) == 0)
			return -1;
		
		return m_flash_mode;
	}
	
	//////////
	
	public void startRecording()
	{
		m_record = true;
		enablePreview();
	}
	
	public void stopRecording()
	{
		m_record = false;
		disablePreview();
	}
	
	public void takePicture()
	{
		
	}
}
