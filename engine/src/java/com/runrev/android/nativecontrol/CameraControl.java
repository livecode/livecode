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
import android.graphics.Color;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PictureCallback;
import android.util.Log;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;
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
	private CameraView m_camera_view;
	
	public class CameraView extends SurfaceView
	{
		private int m_camera_id;
		private String m_flash_mode;

		private Camera m_camera;
		private CameraInfo m_info;
		
		private boolean m_previewing;
		private boolean m_surface_ready;
		
		public CameraView(Context context, int p_camera_id)
		{
			super(context);
			
			m_camera = null;
			m_info = null;
			
			m_camera_id = -1;

			m_previewing = false;
			m_surface_ready = false;
			
			getHolder().addCallback(new SurfaceHolder.Callback() {
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
					closeCamera();
				}
				
				@Override
				public void surfaceChanged(SurfaceHolder p_holder, int p_format, int p_width, int p_height)
				{
					// reconfigure preview
					disablePreview();
					enablePreview();
				}
			});
			
			getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
			
			setCamera(p_camera_id);
		}
		
		private Camera.Size getPreviewSize()
		{
			if (!openCamera())
				return null;
			
			Camera.Size t_size = m_camera.getParameters().getPreviewSize();
			
			int t_rotation = getCameraOrientationSetting();
			if ((t_rotation % 180) == 90)
			{
				int t_tmp = t_size.width;
				t_size.width = t_size.height;
				t_size.height = t_tmp;
			}
			
			return t_size;
		}
		
		public void setCamera(int p_camera_id)
		{
			if (p_camera_id == m_camera_id)
				return;
			
			disablePreview();
			closeCamera();

			m_camera_id = p_camera_id;
			
			if (!openCamera())
				return;

			enablePreview();
		}
		
		private boolean openCamera()
		{
			if (m_camera != null)
				return true;
			
			if (m_camera_id == -1)
				return false;
			
			Camera t_camera;
			CameraInfo t_info;
			
			try
			{
				t_info = new CameraInfo();
				Camera.getCameraInfo(m_camera_id, t_info);
				
				t_camera = Camera.open(m_camera_id);
			}
			catch (Exception e)
			{
				return false;
			}
			
			m_camera = t_camera;
			m_info = t_info;
			
			Camera.Size t_size = getPreviewSize();
			
			getHolder().setFixedSize(t_size.width, t_size.height);
			
			return true;
		}
		
		private void closeCamera()
		{
			if (m_camera == null)
				return;
			
			m_camera.release();
			m_camera = null;
		}
		
		private void enablePreview()
		{
			if (m_previewing)
				return;
			
			if (!m_surface_ready)
				return;
			
			if (!openCamera())
				return;
			
			try
			{
				Camera.Parameters t_params = m_camera.getParameters();
				
				if (m_flash_mode != null)
				{
					List<String> t_modes = t_params.getSupportedFlashModes();
					if (t_modes != null && t_modes.contains(m_flash_mode))
						t_params.setFlashMode(m_flash_mode);
				}
				
				m_camera.setParameters(t_params);
				m_camera.setDisplayOrientation(getCameraOrientationSetting());
				m_camera.setPreviewDisplay(getHolder());
				m_camera.startPreview();
			}
			catch (IOException e)
			{
				return;
			}
			
			m_previewing = true;
		}
		
		private void disablePreview()
		{
			if (!m_previewing)
				return;
			
			m_camera.stopPreview();
			m_previewing = false;
		}
		
		private int getCameraOrientationSetting()
		{
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
			if (m_info.facing == CameraInfo.CAMERA_FACING_FRONT)
				t_rotation = (720 - (m_info.orientation + t_rotation)) % 360;
			else
				t_rotation = (m_info.orientation + 360 - t_rotation) % 360;
			
			return t_rotation;
		}
		
		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			Camera.Size t_size = getPreviewSize();
			if (t_size == null)
			{
				setMeasuredDimension(0, 0);
				return;
			}
			
			int t_width = getDefaultSize(t_size.width, widthMeasureSpec);
			int t_height = getDefaultSize(t_size.height, heightMeasureSpec);
			
			if (t_size.width > 0 && t_size.height > 0)
			{
				if (t_size.width * t_height > t_width * t_size.height)
					t_height = t_width * t_size.height / t_size.width;
				else if (t_size.width * t_height < t_width * t_size.height)
					t_width = t_height * t_size.width / t_size.height;
			}
			
			setMeasuredDimension(t_width, t_height);
		}
		
		public void setFlashMode(int p_mode)
		{
			String t_mode = null;
			switch (p_mode)
			{
				case FLASH_OFF:
					t_mode = Camera.Parameters.FLASH_MODE_OFF;
					break;
				case FLASH_ON:
					t_mode = Camera.Parameters.FLASH_MODE_ON;
					break;
				case FLASH_AUTO:
					t_mode = Camera.Parameters.FLASH_MODE_AUTO;
					break;
			}
			
			if (t_mode == m_flash_mode)
				return;
			
			disablePreview();
			m_flash_mode = t_mode;
			enablePreview();
		}
	}
	
	public CameraControl(NativeControlModule p_module)
	{
		super(p_module);
	}
	
	@Override
	public View createView(Context p_context)
	{
		m_device = DEVICE_DEFAULT;
		
		m_camera_info = null;
		m_devices = 0;
		m_flash_mode = FLASH_AUTO;

		fetchCameraInfo();
		
		int t_id;
		t_id = currentCameraId();
		
		if (t_id == -1)
			return null;
		
		try
		{
			m_camera_view = new CameraView(p_context, t_id);
		}
		catch (Exception e)
		{
			StringWriter t_out = new StringWriter();
			e.printStackTrace(new PrintWriter(t_out));
			Log.i(TAG, e.toString());
			Log.i(TAG, t_out.toString());
			return null;
		}
		
		FrameLayout t_container;
		t_container = new FrameLayout(p_context);
		
		t_container.setBackgroundColor(Color.BLACK);
		t_container.addView(m_camera_view,
							new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
														 FrameLayout.LayoutParams.MATCH_PARENT,
														 Gravity.CENTER));
		
		return t_container;
	}
	
	public void onPause()
	{
		m_camera_view.disablePreview();
		m_camera_view.closeCamera();
	}
	
	public void onResume()
	{
		m_camera_view.enablePreview();
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
		m_camera_view.setCamera(currentCameraId());
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
	}
	
	public void stopRecording()
	{
	}
	
	public void takePicture()
	{
	}
}
