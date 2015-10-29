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

package com.runrev.android.nativecontrol;

import com.runrev.android.Engine;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.util.*;
import android.view.*;
import android.widget.*;

import java.util.*;
import java.lang.reflect.*;

public class NativeControlModule
{
    public static final String TAG = "revandroid.NativeControlModule";
    protected static Engine m_engine;
    protected static RelativeLayout m_layout;
    protected ViewGroup m_parent;
	protected ArrayList<NativeControl> m_controls;
    
    public NativeControlModule(Engine p_engine, ViewGroup p_container_view)
    {
        /* IMPLEMENT */
        m_engine = p_engine;
        m_parent = p_container_view;
		m_layout = null;
        m_controls = new ArrayList<NativeControl>();
    }
    
    public void initContainer()
    {
        if (m_layout == null)
        {
            m_layout = new RelativeLayout(m_engine.getContext());
            
            m_parent . addView(m_layout,
                                  new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                                                               FrameLayout.LayoutParams.MATCH_PARENT));
            m_parent . bringChildToFront(m_layout);
        }
    }
    
    public void addControl(Object p_control)
    {
        NativeControl t_control = (NativeControl)p_control;
        
        //Log.i(TAG, String.format("native control layer: %d", ((ViewGroup)m_layout.getParent()).indexOfChild(m_layout)));
        m_layout.addView(t_control.getView(), new RelativeLayout.LayoutParams(0, 0));
		
		m_controls.add(t_control);
    }
    
    public void removeControl(Object p_control)
    {
        NativeControl t_control = (NativeControl)p_control;
        
        m_layout.removeView(t_control.getView());
		
		m_controls.remove(t_control);
    }
	
	public void onPause()
	{
		for (NativeControl t_control : m_controls)
			t_control.onPause();
	}
	
	public void onResume()
	{
		for (NativeControl t_control : m_controls)
			t_control.onResume();
	}
    
    public Object createControl(String p_class_name)
    {
        initContainer();
        
        // First lookup the class with the given name.
        Class t_class;
        try
        {
            t_class = Class.forName(p_class_name);
        }
        catch(Exception e)
        {
            return null;
        }
        
        // Check that the class is a subclass of NativeControl.
        if (t_class.getSuperclass() != NativeControl.class)
            return null;
        
        // Lookup a constructor with signature (NativeControl).
        Constructor<NativeControl> t_constructor;
        try
        {
            t_constructor = t_class.getDeclaredConstructor(new Class[] { NativeControlModule.class });
        }
        catch(Exception e)
        {
            return null;
        }
        
        // Finally attempt to construct a new instance of the object.
        NativeControl t_control;
        try
        {
            t_control = t_constructor.newInstance(new Object[] { this });
        }
        catch(Exception e)
        {
            return null;
        }
        
        // Success!
        return t_control;
    }
        
    public static ViewGroup.LayoutParams createLayoutParams(int left, int top, int right, int bottom)
    {
        RelativeLayout.LayoutParams t_layout = new RelativeLayout.LayoutParams(right - left, bottom - top);
        t_layout.leftMargin = left;
        t_layout.topMargin = top;
        t_layout.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        t_layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        return t_layout;
    }
    
    public void setNativeControlRect(View p_view, int left, int top, int right, int bottom)
    {
        final ViewGroup.LayoutParams t_layout = createLayoutParams(left, top, right, bottom);
        final View t_view = p_view;
        
		t_view.setLayoutParams(t_layout);
    }
    
    public static Engine getEngine()
    {
        return m_engine;
    }
    
    public static Context getContext()
    {
        return m_engine.getContext();
    }
	
	public static Activity getActivity()
	{
		return (Activity)getContext();
	}
	
	public static RelativeLayout getNativeControlContainer()
	{
		return m_layout;
	}
	
	public boolean handleBackPressed()
	{
		for (NativeControl t_control : m_controls)
		{
			if (t_control.handleBackPressed())
				return true;
		}
		
		return false;
	}
}
