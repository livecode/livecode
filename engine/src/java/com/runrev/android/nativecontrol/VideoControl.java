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
import android.content.res.*;
import android.graphics.*;
import android.media.*;
import android.view.*;
import android.widget.*;


import java.io.*;

public class VideoControl extends NativeControl
{
    public static final int AVAILABLE_PROPERTY_DURATION = 1;
    public static final int AVAILABLE_PROPERTY_NATURALSIZE = 2;
    
    ExtVideoView m_video_view;
    MediaController m_video_controller;
    FrameLayout m_container;
    
	public VideoControl(NativeControlModule p_module)
	{
		super(p_module);
	}
    
    public View createView(Context context)
    {
        m_video_view = new ExtVideoView(context);
        
        m_video_controller = new MediaController(context){
		// PM-2015-10-19: [[ Bug 16027 ]] Make sure the controller does not disappear every
		// time a control (i.e. Pause button) is clicked. This happened because when touching
		// the controls, MediaController called show(sDefaultTimeout);
			@Override
			public void show(int timeout) {
				super.show(0);
			}
		};
        m_video_view.setMediaController(m_video_controller);
        
        setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
           @Override
            public void onCompletion(MediaPlayer mp)
            {
                doPlayerFinished();
                m_module.getEngine().wakeEngineThread();
            }
        });
		
        setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra)
            {
                doPlayerError();
                m_module.getEngine().wakeEngineThread();
                return false;
            }
        });
        
        m_video_view.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mp)
            {
                doPropertyAvailable(AVAILABLE_PROPERTY_DURATION);
                m_module.getEngine().wakeEngineThread();
            }
        });
        
        m_video_view.setOnVideoSizeChangedListener(new MediaPlayer.OnVideoSizeChangedListener() {
            @Override
            public void onVideoSizeChanged(MediaPlayer mp, int width, int height)
            {
                doPropertyAvailable(AVAILABLE_PROPERTY_NATURALSIZE);
                m_module.getEngine().wakeEngineThread();
            }
        });
        
        m_video_view.setOnMovieTouchedListener(new ExtVideoView.OnMovieTouchedListener() {
            @Override
            public void onMovieTouched()
            {
                doMovieTouched();
            }
        });
        
        m_container = new FrameLayout(context) {
            @Override
            public boolean onTouchEvent(MotionEvent ev)
            {
				if (ev.getActionMasked() == MotionEvent.ACTION_DOWN)
					dispatchMovieTouched();
                return true;
            }
        };
        
        m_container.setBackgroundColor(Color.BLACK);
        m_container.addView(m_video_view,
                            new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                                                         FrameLayout.LayoutParams.MATCH_PARENT,
                                                         Gravity.CENTER));

        return m_container;
    }
    
    public boolean setUrl(String url)
    {
        m_video_view.setVideoPath(url);
        return true;
    }
    
    public boolean setFile(String path, boolean isAsset)
    {
        try
        {
            FileDescriptor t_descriptor = null;
            long t_fd_start = 0, t_fd_length = 0;
            if (isAsset)
            {
                AssetFileDescriptor t_asset_descriptor = m_video_view.getContext().getAssets().openFd(path);
                t_descriptor = t_asset_descriptor.getFileDescriptor();
                t_fd_start = t_asset_descriptor.getStartOffset();
                t_fd_length = t_asset_descriptor.getLength();
                
                m_video_view.setVideoFileDescriptor(t_descriptor, t_fd_start, t_fd_length);
            }
            else
            {
                FileInputStream t_filestream = new FileInputStream(path);
                t_descriptor = t_filestream.getFD();
                
                m_video_view.setVideoFileDescriptor(t_descriptor);
            }
        }
        catch (Exception e)
        {
            return false;
        }
        
        return true;
    }
    
    public void setShowController(boolean show)
    {
        if (show)
		{
            m_video_view.setMediaController(m_video_controller);
			m_video_view.setControllerVisible(getVisible());
		}
        else
            m_video_view.setMediaController(null);
    }
	
	// PM-2015-11-05: [[ Bug 16368 ]] Toggling the visibility of the android player should show/hide the controller (if any)
	// Override setVisible() of NativeControl 
	public void setVisible(boolean p_visible)
	{
		m_video_view.setControllerVisible(p_visible);
		super.setVisible(p_visible);
	}
	
    public void setCurrentTime(int msec)
    {
        m_video_view.seekTo(msec);
    }
    
    public void setLooping(boolean loop)
    {
        m_video_view.setLooping(loop);
    }

    public boolean getShowController()
    {
        return m_video_view.getMediaController() != null;
    }
    
    public boolean getLooping()
    {
        return m_video_view.isLooping();
    }

    public int getDuration()
    {
        return m_video_view.getDuration();
    }
	
	public int getPlayableDuration()
	{
		return m_video_view.getPlayableDuration();
	}
	
    public int getCurrentTime()
    {
        return m_video_view.getCurrentPosition();
    }
    
    public int getVideoWidth()
    {
        return m_video_view.getVideoWidth();
    }
    
    public int getVideoHeight()
    {
        return m_video_view.getVideoHeight();
    }
    
    public void start()
    {
        m_video_view.start();
    }
    
    public void stop()
    {
        m_video_view.stop();
    }
    
    public void pause()
    {
        m_video_view.pause();
    }
    
    
    public void suspend()
    {
        m_video_view.suspend();
    }
    
    public void resume()
    {
        m_video_view.resume();
    }
    
    public void setOnCompletionListener(MediaPlayer.OnCompletionListener listener)
    {
        m_video_view.setOnCompletionListener(listener);
    }
    
    public void setOnErrorListener(MediaPlayer.OnErrorListener listener)
    {
        m_video_view.setOnErrorListener(listener);
    }
	
    public void setOnVideoSizeChangedListener(MediaPlayer.OnVideoSizeChangedListener listener)
    {
        m_video_view.setOnVideoSizeChangedListener(listener);
    }
    
    public void setOnPreparedListener(MediaPlayer.OnPreparedListener listener)
    {
        m_video_view.setOnPreparedListener(listener);
    }
    
////////////////////////////////////////////////////////////////////////////////
	
	public interface OnMovieTouchedListener
        {
            void onMovieTouched();
        }
    
    protected OnMovieTouchedListener m_on_movie_touched_listener;
    
    public void setOnMovieTouchedListener(OnMovieTouchedListener p_listener)
    {
        m_on_movie_touched_listener = p_listener;
    }
    
    protected void dispatchMovieTouched()
    {
        if (m_on_movie_touched_listener != null)
            m_on_movie_touched_listener.onMovieTouched();
    }

    	
////////////////////////////////////////////////////////////////////////////////
    public native void doPlayerFinished();
    public native void doPlayerError();
    public native void doPropertyAvailable(int property);
    public native void doMovieTouched();
}
