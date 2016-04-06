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

import android.content.*;
import android.graphics.*;
import android.util.*;
import android.view.*;
import android.widget.*;

import java.util.*;

import com.inneractive.api.ads.InneractiveAd;
import com.inneractive.api.ads.InneractiveAdListener;

public class AdModule
{        
    private Engine m_engine;
    private ViewGroup m_parent;    
    private ViewGroup m_layout;
        
    public AdModule(Engine p_engine, ViewGroup p_parent)
    {
        m_engine = p_engine;
        m_parent = p_parent;
    }
        
    public Object createInneractiveAd(String p_key, int p_type, int p_left, int p_top, int p_timeout, Map p_meta_data)
    {
        initContainer();
        InneractiveAdWrapper t_ad = new InneractiveAdWrapper(this, p_key, p_type, p_left, p_top, p_timeout, p_meta_data);
        if (t_ad.getView() == null)
            return null;
        return t_ad;
    }

    public Engine getEngine()
    {
        return m_engine;
    }
    
    public ViewGroup getLayout()
    {
        return m_layout;
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
        
    public void removeAd(Object p_ad)
    {
        if (p_ad != null)
        {
            InneractiveAdWrapper t_ad = (InneractiveAdWrapper) p_ad;
            m_layout.removeView(t_ad.getView());
        }
    }   
    
    public void setViewRect(View p_view, int left, int top, int width, int height)
    {
        final ViewGroup.LayoutParams t_layout = createLayoutParams(left, top, width, height);
        final View t_view = p_view;
        m_engine.post(new Runnable()
        {
            public void run()
            {
                m_layout.updateViewLayout(t_view, t_layout);
            }
        });
    }
    
    private static ViewGroup.LayoutParams createLayoutParams(int left, int top, int width, int height)
    {
        RelativeLayout.LayoutParams t_layout = new RelativeLayout.LayoutParams(width, height);
        t_layout.leftMargin = left;
        t_layout.topMargin = top;
        t_layout.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        t_layout.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        return t_layout;
    }
    
}

class InneractiveAdWrapper implements InneractiveAdListener
{        
    public static final String TAG = "revandroid.InneractiveAdWrapper";
    public native void doAdUpdate(int event);
    
    private AdModule m_ad_module;
    private View m_ad_view;
    private int m_type;
    private int m_left, m_top, m_width, m_height;
    private boolean m_visible;
        
    private static final int kMCAdEventTypeReceive = 0;
	private static final int kMCAdEventTypeReceiveDefault = 1;
	private static final int kMCAdEventTypeReceiveFailed = 2;
	private static final int kMCAdEventTypeClick = 3;
    private static final int kMCAdEventTypeResizeStart = 4;
	private static final int kMCAdEventTypeResizeEnd = 5;
	private static final int kMCAdEventTypeExpandStart = 6;
	private static final int kMCAdEventTypeExpandEnd = 7;
    
    private static final int kAdTypeUnknown = 0;
    private static final int kMCAdTypeBanner = 1;
	private static final int kMCAdTypeText = 2;
	private static final int kMCAdTypeFullscreen = 3;   
        
    public InneractiveAdWrapper(AdModule p_module, String p_key, int p_type, int p_left, int p_top, int p_timeout, Map p_meta_data)
    {                
        m_ad_module = p_module;
        m_type = p_type;
        m_visible = true;
        
        // Attempt to guess the size of the ad based of the device dimensions and the info here: http://mmaglobal.com/files/mobileadvertising.pdf
        // We positon the ad by setting its rect.  Thus, we need to know its width and height.
        // It appears for phones, the ad is scaled to the (portrait) width of the device and the heigt is based on the type of add served.
        // For tablets, it appears we are always served 320 * 50 ads.
        WindowManager t_window_manager = (WindowManager) m_ad_module.getEngine().getContext().getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics t_metrics = new DisplayMetrics();
        t_window_manager.getDefaultDisplay().getMetrics(t_metrics);        
        int t_width = Math.min(t_metrics.widthPixels, t_metrics.heightPixels);
        if (m_type != kMCAdTypeFullscreen)
        {
            m_left = p_left;
            m_top = p_top;        
            if (t_width <= 240)
            {
                m_width = t_width;
                m_height = 36;
            }
            else if (t_width <= 480)
            {
                m_width = t_width;
                m_height = 75;   
            }
            else
            {
                m_width = 320;
                m_height = 50;
            }
        }
        else
        {
            m_left = 0;
            m_top = 0;        
            m_width = m_ad_module.getEngine().getWidth();
            m_height = m_ad_module.getEngine().getHeight();
        }
        
        Hashtable<InneractiveAd.IaOptionalParams, String> t_meta_data = new Hashtable<InneractiveAd.IaOptionalParams, String>();
        if (p_meta_data.containsKey("age"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_Age, (String) p_meta_data.get("age"));
        if (p_meta_data.containsKey("gender"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_Gender, (String) p_meta_data.get("gender"));
        if (p_meta_data.containsKey("external id"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_External_Id, (String) p_meta_data.get("external id"));
        if (p_meta_data.containsKey("distribution id"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_Distribution_Id, (String) p_meta_data.get("distribution id"));
        if (p_meta_data.containsKey("phone number"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_Msisdn, (String) p_meta_data.get("phone number"));
        if (p_meta_data.containsKey("keywords"))
            t_meta_data.put(InneractiveAd.IaOptionalParams.Key_Keywords, (String) p_meta_data.get("keywords"));
      
        InneractiveAd t_ad = new InneractiveAd(m_ad_module.getEngine().getContext(), p_key, getIAdType(), p_timeout, t_meta_data);
        if (t_ad == null)
            return;
        t_ad.setInneractiveListener(this);        
        m_ad_module.getLayout().addView(t_ad);
        m_ad_view = t_ad;
                 
        if (m_type != kMCAdTypeFullscreen)
            m_ad_module.setViewRect(m_ad_view, m_left, m_top, m_width, m_height);        
    }
        
    public View getView()
    {
        return m_ad_view;
    }    
    
    public void setVisible(boolean p_visible)
    {
        // Since the visibility property of the view appears to be managed by the ad api, we hide ad by setting its rect.
        m_visible = p_visible;
        if (!m_visible)
            m_ad_module.setViewRect(m_ad_view, 0, 0, 0, 0);
        else
            m_ad_module.setViewRect(m_ad_view, m_left, m_top, m_width, m_height);
    }
    
    public boolean getVisible()
    {
        return m_visible;
    }
    
    public void setTopLeft(int p_left, int p_top)
    {
        if (m_type != kMCAdTypeFullscreen)
        {
            m_top = p_top;
            m_left = p_left;
            // We use the rect to manage the ad's visibility, so only set the rect when the ad is visible.
            if (m_visible)
                m_ad_module.setViewRect(m_ad_view, m_left, m_top, m_width, m_height);
        }
    }
        
    public int getLeft()
    {
        return m_left;
    }
    
    public int getTop()
    {
        return m_top;
    }  
    
    private InneractiveAd.IaAdType getIAdType()
    {
        switch (m_type)
        {
            case kMCAdTypeText:
                return InneractiveAd.IaAdType.Text;
            case kMCAdTypeFullscreen:
                return InneractiveAd.IaAdType.Interstitial;
            case kAdTypeUnknown:
            case kMCAdTypeBanner:
            default:
                return InneractiveAd.IaAdType.Banner;
        }
    }
    
    public void onIaAdReceived() {
        doAdUpdate(kMCAdEventTypeReceive);
        m_ad_module.getEngine().wakeEngineThread();
    }
    
    public void onIaDefaultAdReceived() {
        doAdUpdate(kMCAdEventTypeReceiveDefault);
        m_ad_module.getEngine().wakeEngineThread();
    }
    
    public void onIaAdClicked() {
        doAdUpdate(kMCAdEventTypeClick);
        m_ad_module.getEngine().wakeEngineThread();
    }

    public void onIaAdFailed() {
        doAdUpdate(kMCAdEventTypeReceiveFailed);
        m_ad_module.getEngine().wakeEngineThread();
    }
    
    public void onIaAdResize() {
        doAdUpdate(kMCAdEventTypeResizeStart);
        m_ad_module.getEngine().wakeEngineThread();
    }

    public void onIaAdResizeClosed() {
        doAdUpdate(kMCAdEventTypeResizeEnd);
        m_ad_module.getEngine().wakeEngineThread();
    }

    public void onIaAdExpand() {
        doAdUpdate(kMCAdEventTypeExpandStart);
        m_ad_module.getEngine().wakeEngineThread();
    }

    public void onIaAdExpandClosed() {
        doAdUpdate(kMCAdEventTypeExpandEnd);
        m_ad_module.getEngine().wakeEngineThread();
    }
    
}
