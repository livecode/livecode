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
import android.widget.*;

class ScrollerControl extends NativeControl
{
    public static final String TAG = "revandroid.NativeScrollerControl";
    
    private View m_inner_hview, m_inner_vview;
    private ScrollView m_vscroll;
    private HorizontalScrollView m_hscroll;
    
    private boolean m_scrolling_enabled;
    
    private int m_left;
    private int m_top;
    private int m_width;
    private int m_height;
    
    private boolean m_dispatching;
    private int m_new_left;
    private int m_new_top;
    
    private boolean m_touch_canceled;
    private boolean m_dragging;
    private boolean m_tracking;
    
    public ScrollerControl(NativeControlModule p_module)
    {
        super(p_module);
        
        m_left = m_top = 0;
        m_new_left = m_new_top = 0;
        m_dispatching = false;
        m_dragging = false;
        m_tracking = false;
        m_touch_canceled = false;
        
        m_scrolling_enabled = true;
    }
    
    public View createView(Context p_context)
    {
        // we create a 2d (vertical & horizontal) scroll view by overlaying two 1-directional scroll views
        // heirarchy:
        // FrameLayout      - container for both scrollviews
        // |
        // |--ScrollView    - vertical scroller
        // |  |
        // |   --View       - catches touch events
        // |
        //  --HorizontalScrollView  - horizontal scroller
        //    |
        //     --View               - catches touch events
        
        m_vscroll = new ScrollView(p_context) {
            @Override
            public void onScrollChanged(int l, int t, int oldl, int oldt)
            {
                if (t != oldt)
                    ScrollerControl.this.updateScroll(m_dispatching ? m_new_left : m_left, t);
                super.onScrollChanged(l, t, oldl, oldt);
            }
            
            @Override
            public boolean onInterceptTouchEvent (MotionEvent ev)
            {
                return true;
            }
        };
        m_hscroll = new HorizontalScrollView(p_context) {
            @Override
            public void onScrollChanged(int l, int t, int oldl, int oldt)
            {
                if (l != oldl)
                    ScrollerControl.this.updateScroll(l, m_dispatching ? m_new_top : m_top);
                super.onScrollChanged(l, t, oldl, oldt);
            }
            
            @Override
            public boolean onInterceptTouchEvent (MotionEvent ev)
            {
                return true;
            }
        };
        
        m_inner_hview = new View(p_context) {
            @Override
            public boolean onTouchEvent(MotionEvent e)
            {
                if (m_dragging)
                    return false;
                
                if (MotionEvent.ACTION_CANCEL == (e.getAction() & MotionEvent.ACTION_MASK))
                    m_touch_canceled = true;
				
                return true;
            }
        };
        m_inner_vview = new View(p_context) {
            @Override
            public boolean onTouchEvent(MotionEvent e)
            {
                if (m_dragging)
                    return false;
                
                if (MotionEvent.ACTION_CANCEL == (e.getAction() & MotionEvent.ACTION_MASK))
                    m_touch_canceled = true;
                
                return true;
            }
        };
        
        m_hscroll.addView(m_inner_hview, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
        m_vscroll.addView(m_inner_vview, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        
        FrameLayout t_view = new FrameLayout(p_context) {
            @Override
            public boolean dispatchTouchEvent(MotionEvent e)
            {
                // handle dispatching of touch events so we can send them to both scrollviews
                if (!m_scrolling_enabled)
                {
                    NativeControlModule.getEngine().onTouchEvent(e);
                    return false;
                }
                
                m_dispatching = true;
                m_touch_canceled = false;
                m_new_left = m_left;
                m_new_top = m_top;
                
                if ((e.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_DOWN)
                    m_tracking = true;
                
                boolean t_hdispatch = m_hscroll.dispatchTouchEvent(MotionEvent.obtain(e));
                boolean t_vdispatch = m_vscroll.dispatchTouchEvent(MotionEvent.obtain(e));
                
                m_dispatching = false;
                
                ScrollerControl.this.updateScroll(m_new_left, m_new_top);
                
                if (!m_dragging)
                {
                    if (m_touch_canceled)
                    {
                        m_dragging = true;
                        m_tracking = false;
                        onScrollBeginDrag();
                        e.setAction(MotionEvent.ACTION_CANCEL);
                    }
                    else
                    {
                        e.offsetLocation(getLeft(), getTop());
                    }
                    NativeControlModule.getEngine().onTouchEvent(e);
                }
                else
                {
                    if (((e.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_UP) ||
                        ((e.getAction() & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_CANCEL))
                    {
                        m_dragging = false;
                        m_tracking = false;
                        onScrollEndDrag();
                    }
                }
                
                return  t_hdispatch || t_vdispatch;
            }
            
            @Override
            public void dispatchDraw(Canvas canvas)
            {
                // we wrap this method to watch for animated scrolling (after fling), which occurs during scrollview redraws
                m_dispatching = true;
                m_new_left = m_left;
                m_new_top = m_top;
                super.dispatchDraw(canvas);
                m_dispatching = false;
                ScrollerControl.this.updateScroll(m_new_left, m_new_top);
            }
            
            @Override
            public void requestDisallowInterceptTouchEvent(boolean disallow)
            {
                // IGNORE
            }
        };
        t_view.addView(m_vscroll, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        t_view.addView(m_hscroll, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        return t_view;
    }
    
    public void setContentSize(int width, int height)
    {
    	//Log.i(TAG, "setContentSize(" + width + "," + height + ")");
        m_inner_hview.setMinimumWidth(width);
        m_inner_vview.setMinimumHeight(height);
        
        m_vscroll.updateViewLayout(m_inner_vview, new FrameLayout.LayoutParams(width, height));
        m_hscroll.updateViewLayout(m_inner_hview, new FrameLayout.LayoutParams(width, height));
    }
    
    protected void updateScroll(int l, int t)
    {
        if (m_dispatching)
        {
            m_new_left = l;
            m_new_top = t;
        }
        else
        {
            if (m_left != l || m_top != t)
            {
                m_left = l;
                m_top = t;
                onScrollChanged(m_left, m_top);
            }
        }
    }
    
    protected void onScrollChanged(int l, int t)
    {
		//        Log.i(TAG, String.format("onScrollChanged(%d, %d)", l, t));
        doScrollChanged(l, t);
        m_module.getEngine().wakeEngineThread();
    }
    
    protected void onScrollBeginDrag()
    {
        doScrollBeginDrag();
        m_module.getEngine().wakeEngineThread();
    }
    
    protected void onScrollEndDrag()
    {
        doScrollEndDrag();
        m_module.getEngine().wakeEngineThread();
    }
    
    public int getHScroll() { return m_left; }
    public int getVScroll() { return m_top; }
    
    public void setHScroll(int h)
    {
        //Log.i(TAG, String.format("setHScroll(%d)", h));
		// PM-2015-09-23: [[ Bug 11709 ]] Make sure the hscroll is actually set
		m_left = h;
        m_hscroll.scrollTo(h, m_top);
    }
    
    public void setVScroll(int v)
    {
        //Log.i(TAG, String.format("setVScroll(%d)", v));
		// PM-2015-09-23: [[ Bug 11709 ]] Make sure the vscroll is actually set
		m_top = v;
        m_vscroll.scrollTo(m_left, v);
    }
    
    public boolean getHorizontalIndicator()
    {
        return m_hscroll.isHorizontalScrollBarEnabled();
    }
    
    public boolean getVerticalIndicator()
    {
        return m_vscroll.isVerticalScrollBarEnabled();
    }
    
    public void setHorizontalIndicator(boolean visible)
    {
        m_hscroll.setHorizontalScrollBarEnabled(visible);
    }
    
    public void setVerticalIndicator(boolean visible)
    {
        m_vscroll.setVerticalScrollBarEnabled(visible);
    }
    
    public boolean getScrollingEnabled()
    {
        return m_scrolling_enabled;
    }
    
    
    public void setScrollingEnabled(boolean p_enabled)
    {
        m_scrolling_enabled = p_enabled;
    }
    
    public boolean isDragging()
    {
        return m_dragging;
    }
    
    public boolean isTracking()
    {
        return m_tracking;
    }
    
    public native void doScrollChanged(int left, int top);
    public native void doScrollBeginDrag();
    public native void doScrollEndDrag();
}
