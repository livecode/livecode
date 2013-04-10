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
    
    public void setRect(int left, int top, int right, int bottom)
    {
        m_module.setNativeControlRect(getView(), left, top, right, bottom);
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
        return m_control_view.getLeft();
    }
    
    public int getTop()
    {
        return m_control_view.getTop();
    }
    
    public int getRight()
    {
        return m_control_view.getRight();
    }
    
    public int getBottom()
    {
        return m_control_view.getBottom();
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

