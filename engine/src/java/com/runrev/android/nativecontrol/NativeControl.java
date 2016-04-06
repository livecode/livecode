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

import android.content.*;
import android.graphics.*;
import android.view.*;

abstract class NativeControl
{
    public static final String TAG = "revandroid.NativeControl";
    NativeControlModule m_module;
    protected View m_control_view;
    protected int m_alpha;
    
    // MM-2013-12-17: [[ Bug 11609 ]] Store the rect of the control rather than trying to calculate when requested.
    private int m_left;
    private int m_top;
    private int m_right;
    private int m_bottom;
    
    public NativeControl(NativeControlModule p_module)
    {
        m_alpha = 255;
        
        m_module = p_module;
        m_control_view = createView(m_module.getEngine().getContext());
        
        m_control_view.setVisibility(View.INVISIBLE);
    }
    
    public View getView()
    {
        return m_control_view;
    }
    
    public abstract View createView(Context p_context);
    
	public void onPause()
	{
	}
	
	public void onResume()
	{
	}
	
    public void setRect(int left, int top, int right, int bottom)
    {
        m_module.setNativeControlRect(getView(), left, top, right, bottom);
        m_left = left;
        m_top = top;
        m_right = right;
        m_bottom = bottom;
    }
    
    public void setVisible(boolean p_visible)
    {
        m_control_view.setVisibility(p_visible ? View.VISIBLE : View.INVISIBLE);
    }
    
    public void setAlpha(int p_alpha)
    {
        m_alpha = p_alpha;
        m_control_view.getBackground().setAlpha(p_alpha);
    }
    
    public void setBackgroundColor(int red, int green, int blue, int alpha)
    {
        m_control_view.setBackgroundColor(Color.argb(alpha, red, green, blue));
        // TODO - rework and implement
    }
    
    public int getLeft()
    {
        return m_left;
    }
    
    public int getTop()
    {
        return m_top;
    }
    
    public int getRight()
    {
        return m_right;
    }
    
    public int getBottom()
    {
        return m_bottom;
    }
    
    public boolean getVisible()
    {
        return m_control_view.getVisibility() == View.VISIBLE;
    }
    
    public int getBackgroundColor()
    {
        return m_control_view.getDrawingCacheBackgroundColor();
    }
    
    public int getAlpha()
    {
        return m_alpha;
    }
    
	public boolean handleBackPressed()
	{
		return false;
	}
}

