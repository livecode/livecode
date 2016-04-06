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

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.opengles.GL10;

import android.util.*;
import android.view.*;
import android.content.Context;
import android.util.AttributeSet;

public class OpenGLView extends SurfaceView implements SurfaceHolder.Callback
{
	// Instance variables
	
	private EGL10 m_egl;
	private EGLDisplay m_egl_display;
	private EGLSurface m_egl_surface;
	private EGLConfig m_egl_config;
	private EGLContext m_egl_context;

	// Constructors
	
	public OpenGLView(Context context)
	{
        super(context);
        initialize();
    }
	
    public OpenGLView(Context context, AttributeSet attrs)
	{
        super(context, attrs);
        initialize();
    }
	
	// Event handling
	
	@Override
	public boolean onTouchEvent(MotionEvent p_event)
	{
		return false;
	}
		
	// Life-cycle management.
	
    private void initialize()
	{
        SurfaceHolder t_holder;
		t_holder = getHolder();
        t_holder . addCallback(this);
        t_holder . setType(SurfaceHolder.SURFACE_TYPE_GPU);
    }
	
	private EGLConfig findConfig(int r, int g, int b, int a)
	{
		int[] t_config_spec;
		t_config_spec = new int[] {
			EGL10 . EGL_RED_SIZE, r,
			EGL10 . EGL_GREEN_SIZE, g,
			EGL10 . EGL_BLUE_SIZE, b,
			EGL10 . EGL_ALPHA_SIZE, a,
			EGL10 . EGL_DEPTH_SIZE, 0,
			EGL10 . EGL_STENCIL_SIZE, 0,
			EGL10 . EGL_NONE };
		
		// Find out how many configs with the given rgb sizes there are.
		int[] t_num_config;
		t_num_config = new int[1];
		m_egl . eglChooseConfig(m_egl_display, t_config_spec, null, 0, t_num_config);
		
		// Fetch them all.
		EGLConfig[] t_configs;
		t_configs = new EGLConfig[t_num_config[0]];
		m_egl . eglChooseConfig(m_egl_display, t_config_spec, t_configs, t_num_config[0], null);
		
		// Default to returning the first in the list.
		return t_configs[0];
	}
	
	public void start()
	{
		Log.i("revandroid", "OpenGLView.start");

		// Get an EGL instance
		m_egl = (EGL10)EGLContext . getEGL();
		
		// Get the default display.
		m_egl_display = m_egl . eglGetDisplay(EGL10 . EGL_DEFAULT_DISPLAY);
		
		// Initialize the EGL for the display.
		int[] t_version;
		t_version = new int[2];
		m_egl . eglInitialize(m_egl_display, t_version);
		
		// Search for a suitable config.
		m_egl_config = findConfig(8, 8, 8, 8);
		if (m_egl_config == null)
			m_egl_config = findConfig(5, 6, 5, 0);
		if (m_egl_config == null)
			m_egl_config = findConfig(5, 5, 5, 0);
		if (m_egl_config == null)
			m_egl_config = findConfig(4, 4, 4, 4);
		if (m_egl_config == null)
			m_egl_config = findConfig(0, 0, 0, 0);
		
		// For debugging.
		dumpConfig("Using EGLConfig", m_egl, m_egl_display, m_egl_config);
		
		// Now create the OpenGL ES context.
		m_egl_context = m_egl . eglCreateContext(m_egl_display, m_egl_config, EGL10 . EGL_NO_CONTEXT, null);
		
		// We don't have a surface (just yet).
		m_egl_surface = null;
	}
	
	public void configure()
	{
		Log.i("revandroid", "OpenGLView.configure");
		
		// If we have a surface, destroy it.
		if (m_egl_surface != null)
		{
			m_egl . eglMakeCurrent(m_egl_display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
			m_egl . eglDestroySurface(m_egl_display, m_egl_surface);
		}
		
		// Create an EGL surface.
		m_egl_surface = m_egl . eglCreateWindowSurface(m_egl_display, m_egl_config, getHolder(), null);
		
		// Make our EGL context current.
		m_egl . eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_egl_context);
	}
	
	public void finish()
	{
		Log.i("revandroid", "OpenGLView.finish");
		
		if (m_egl_surface != null)
		{
			m_egl . eglMakeCurrent(m_egl_display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
			m_egl . eglDestroySurface(m_egl_display, m_egl_surface);
			m_egl_surface = null;
		}
		
		if (m_egl_context != null)
		{
			m_egl . eglDestroyContext(m_egl_display, m_egl_context);
			m_egl_context = null;
		}
		
		if (m_egl_display != null)
		{
			m_egl . eglTerminate(m_egl_display);
			m_egl_display = null;
		}
	}
	
	public void swap()
	{
		m_egl . eglSwapBuffers(m_egl_display, m_egl_surface);
	}
	
	// These are the SurfaceHolder.Callback methods.
	
    public void surfaceCreated(SurfaceHolder holder)
	{
		Log.i("revandroid", "OpenGLView.surfaceCreated");
		doSurfaceCreated(this);
    }
	
    public void surfaceDestroyed(SurfaceHolder holder)
	{
		Log.i("revandroid", "OpenGLView.surfaceDestroyed");
		doSurfaceDestroyed(this);
    }
	
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		Log.i("revandroid", "OpenGLView.surfaceChanged");
		doSurfaceChanged(this);
    }
	
	public static void dumpConfig(String msg, EGL10 egl, EGLDisplay display, EGLConfig config)
	{
		int t_red, t_green, t_blue, t_alpha, t_depth, t_stencil;
		
		int[] t_attr_value;
		t_attr_value = new int[1];
		
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_RED_SIZE, t_attr_value);
		t_red = t_attr_value[0];
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_GREEN_SIZE, t_attr_value);
		t_green = t_attr_value[0];
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_BLUE_SIZE, t_attr_value);
		t_blue = t_attr_value[0];
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_ALPHA_SIZE, t_attr_value);
		t_alpha = t_attr_value[0];
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_DEPTH_SIZE, t_attr_value);
		t_depth = t_attr_value[0];
		egl . eglGetConfigAttrib(display, config, EGL10 . EGL_STENCIL_SIZE, t_attr_value);
		t_stencil = t_attr_value[0];
		
		Log.i("revandroid", String.format("OpenGLView: %s (%d, %d, %d, %d, %d, %d)", msg, t_red, t_green, t_blue, t_alpha, t_depth, t_stencil));
	}
	
	public static void listConfigs()
	{
		EGL10 t_egl;
		t_egl = (EGL10)EGLContext . getEGL();
		
		EGLDisplay t_display;
		t_display = t_egl . eglGetDisplay(EGL10 . EGL_DEFAULT_DISPLAY);
		
		int[] t_version = new int[2];
		t_egl.eglInitialize(t_display, t_version);
		
		int[] t_num_config;
		t_num_config = new int[1];
		t_egl . eglGetConfigs(t_display, null, 0, t_num_config);
		
		Log.i("revandroid", "OpenGLView: Found " + t_num_config[0] + " configurations.");
		
		EGLConfig t_configs[];
		t_configs = new EGLConfig[t_num_config[0]];
		t_egl . eglGetConfigs(t_display, t_configs, t_num_config[0], null);
		
		for(int i = 0; i < t_num_config[0]; i++)
			dumpConfig("Found EGLConfig ", t_egl, t_display, t_configs[i]);
	}
	
	// These are the native methods which are used to notify of surface changes.
	// In turn, these call the start/finish/configure/swap methods on the *engine*
	// thread as necessary.
	public static native void doSurfaceCreated(OpenGLView view);
	public static native void doSurfaceDestroyed(OpenGLView view);
	public static native void doSurfaceChanged(OpenGLView view);
}
