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

import java.util.*;
import java.io.*;

import android.media.MediaPlayer;
import android.util.*;
import android.content.res.*;
import android.os.*;

public class SoundModule
{    
    ////////////////////////////////////////////////////////////////////////////////
    // Native Funcitons
    ////////////////////////////////////////////////////////////////////////////////
    
    public static final String TAG = "revandroid.SoundModule";
    public static native void doSoundStopped();
    public static native void doSoundFinishedOnChannel(String channel, String sound, long object_handle);
    public static native void doSoundReleaseCallbackHandle(long object_handle);    
    
    ////////////////////////////////////////////////////////////////////////////////
    // Constants
    ////////////////////////////////////////////////////////////////////////////////  
    
    public static final int k_playback_now = 0;
    public static final int k_playback_next = 1;
    public static final int k_playback_looping = 2;    
    
    public static final int k_channel_status_stopped = 0;
    public static final int k_channel_status_paused = 1;
    public static final int k_channel_status_playing = 2;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Member variables
    ////////////////////////////////////////////////////////////////////////////////

    private Engine m_engine;
    
    private HashMap<String, SoundChannel> m_channel_map;
    
    private MediaPlayer m_audio_player;
    private int m_audio_volume;
    private boolean m_play_audio_player;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    public SoundModule(Engine p_engine)
    {
        m_engine = p_engine;
        m_channel_map = new HashMap();
        m_audio_player = new MediaPlayer();
		m_audio_volume = 100;
        m_play_audio_player = false;
    }    
    
    ////////////////////////////////////////////////////////////////////////////////
    // Public API
    ////////////////////////////////////////////////////////////////////////////////

    public void onPause()
    {
        m_play_audio_player = (m_audio_player . isPlaying());
		if (m_play_audio_player)
			m_audio_player . pause();
        
        Collection<SoundChannel> t_channels = m_channel_map.values();
        if (t_channels != null)
        {
            Iterator<SoundChannel> t_iterator = t_channels.iterator();
            while (t_iterator.hasNext())
                t_iterator.next().onPause();
        }
    }
    
    public void onResume()
    {
        if (m_play_audio_player)
			m_audio_player . start();
        
        Collection<SoundChannel> t_channels = m_channel_map.values();
        if (t_channels != null)
        {
            Iterator<SoundChannel> t_iterator = t_channels.iterator();
            while (t_iterator.hasNext())
                t_iterator.next().onResume();
        }
    }
    
    // play <soundFile> [looping]
    public boolean playSound(String p_path, boolean p_is_asset, boolean p_loop)
	{
		if (m_audio_player.isPlaying())
			m_audio_player.stop();
		m_audio_player.reset();
		
		if (p_path != null && p_path.length() > 0)
		{
			try
			{
				if (p_is_asset)
				{
					AssetFileDescriptor t_descriptor;
					t_descriptor = m_engine . getContext() . getAssets() . openFd(p_path);
					m_audio_player.setDataSource(t_descriptor.getFileDescriptor(), t_descriptor.getStartOffset(), t_descriptor.getLength());
				}
				else
				{
					FileInputStream t_filestream = new FileInputStream(p_path);
					m_audio_player.setDataSource(t_filestream.getFD());
				}
				setPlayLoudness(m_audio_volume);
				m_audio_player.setLooping(p_loop);
				m_audio_player.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
                {
					public void onCompletion(MediaPlayer mp)
					{
						doSoundStopped();
					}
				});
                
				m_audio_player.setOnInfoListener(new MediaPlayer.OnInfoListener()
                {
					public boolean onInfo(MediaPlayer mp, int what, int extra)
					{
						return false;
					}
				});
				m_audio_player.setOnErrorListener(new MediaPlayer.OnErrorListener()
                {
					public boolean onError(MediaPlayer mp, int what, int extra)
					{
						return false;
					}
				});
				m_audio_player.setOnPreparedListener(new MediaPlayer.OnPreparedListener()
                {
					public void onPrepared(MediaPlayer mp)
					{
						mp.start();
					}
				});
				m_audio_player.prepareAsync();
			}
			catch (Exception e)
			{
				return false;
			}
		}
		
		return true;
	}
	
    // get the playLoudness
	public int getPlayLoudness()
	{
		return m_audio_volume;
	}
	
    // set the playLoudness
	public boolean setPlayLoudness(int p_loudness)
	{
		m_audio_volume = p_loudness;
		m_audio_player.setVolume(m_audio_volume / 100.0f, m_audio_volume / 100.0f);		
		return true;
	}
    
    // mobilePlaySoundOnChannel
    public boolean playSoundOnChannel(String p_sound, String p_channel, int p_type, String p_sound_path, boolean p_is_asset, long p_callback_handle)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, true);
        if (t_channel != null)
            return t_channel.playSound(p_sound, p_type, p_sound_path, p_is_asset, p_callback_handle);
        return false;
    }
    
    // mobileStopPlayingOnChannel
    public boolean stopPlayingOnChannel(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.stop();
        return false;
    }
    
    // mobilePausePlayingOnChannel
    public boolean pausePlayingOnChannel(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.pause();
        return false;
    }
    
    // mobileResumePlayingOnChannel
    public boolean resumePlayingOnChannel(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.resume();
        return false;
    }
    
    // mobileSetSoundChannelVolume
    public boolean setChannelVolume(String p_channel, int p_volume)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, true);
        if (t_channel != null)
            return t_channel.setVolume(p_volume);
        return false;
    }
    
    // mobileSoundChannelVolume
    public int getChannelVolume(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.getVolume();
        return -1;
    }
    
    // mobileSoundOnChannel
    public String getSoundOnChannel(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.getSound();
        return null;
    }
    
    // mobileNextSoundOnChannel
    public String getNextSoundOnChannel(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.getNextSound();
        return null; 
    }
    
    // mobileSoundChannelStatus
    public int getChannelStatus(String p_channel)
    {
        SoundChannel t_channel = getSoundChannel(p_channel, false);
        if (t_channel != null)
            return t_channel.getStatus();
        return -1; 
    }
    
    // mobileSoundChannels
    public String getSoundChannels()
    {
        String t_channel_names = new String();
        Set<String> t_channels = m_channel_map.keySet();
        if (t_channels != null)
        {
            Iterator t_iterator = t_channels.iterator();
            while (t_iterator.hasNext())
            {
                if (t_channel_names.length() != 0)
                    t_channel_names += '\n';
                t_channel_names += t_iterator.next();
            }
        }
        return t_channel_names;
    }
    
    // mobileDeleteSoundChannel
    public boolean deleteSoundChannel(String p_channel)
    {
        SoundChannel t_channel = m_channel_map.remove(p_channel);
        if (t_channel != null)
        {
            t_channel.release();
            return true;
        }
        return false;
    }    
    
    ////////////////////////////////////////////////////////////////////////////////
    // Internals
    ////////////////////////////////////////////////////////////////////////////////
    
    private SoundChannel getSoundChannel(String p_channel, boolean p_create)
    {
        SoundChannel t_channel = m_channel_map.get(p_channel);
        if (t_channel == null && p_create)
        {
            t_channel = new SoundChannel(p_channel);
            m_channel_map.put(p_channel, t_channel);
        }
        return t_channel;
    }
    
    private class SoundChannel
    {                
        private String m_name;
        private int m_volume;        
        private SoundPlayer m_current_player;
        private SoundPlayer m_next_player;
        private boolean m_resume_channel;
                
        
        public SoundChannel(String p_name)
        {
            m_name = p_name;
            m_current_player = null;
            m_next_player = null;
            m_volume = 100;
            m_resume_channel = false;
        }
                
        public void release()
        {
            if (m_current_player != null)
                m_current_player.release();
            if (m_next_player != null)
                m_next_player.release();
        }
        
        public void onPause()
        {
            m_resume_channel = (m_current_player != null && m_current_player.m_player != null && m_current_player.m_player.isPlaying());
            if (m_resume_channel)
                m_current_player.m_player.pause();
        }
        
        public void onResume()
        {
            if (m_resume_channel && m_current_player != null && m_current_player.m_player != null)
                m_current_player.m_player.start();
        }
        
        public boolean playSound(String p_sound, int p_type, String p_sound_path, boolean p_is_asset, long p_callback_handle)
        {            
            switch (p_type)
            {
                case SoundModule.k_playback_now:
                case SoundModule.k_playback_looping:
                    // if we have an existing player, stop its activity and use this to play the new sound
                    // otherwise create a new player
                    if (m_current_player != null)
                        m_current_player.reset();
                    else
                    {
                        m_current_player = new SoundPlayer();
                        if (m_current_player == null)
                            return false;
                    }
                    
                    // when playing a new sound, any queued sounds are removed from channel
                    // may aswell remove the next player rather than have it hanging around
                    if (m_next_player != null)
                    {
                        m_next_player.release();
                        m_next_player = null;
                    }
                                        
                    return m_current_player.setSound(p_sound, p_type == k_playback_looping, true, p_sound_path, p_is_asset, p_callback_handle);
                    
                case SoundModule.k_playback_next:
				{
                    // if there is no current player or the current player is not playing
                    // then queue the sound on the current player but don't play it (the sound is prepared but the channel is paused)
                    // otherwise set up the sound on the next player ready for when the current player completes
					
					// To check if the current player is not playing we cannot rely *only* on the isPlaying() method of the MediaPlayer class,
					// as this returns false until the player actually starts playing. For our purposes, the current player is "playing" if:
					// 1. Either MediaPlayer.isPlaying() returns true
					// 2. OR there is a pending and unprepared sound, waiting to be played

                    SoundPlayer t_player = null;
                    if (m_current_player != null && !(m_current_player.m_player.isPlaying() || (m_current_player.isPending() && !m_current_player.isPrepared())))
                    {
                        m_current_player.reset();
                        t_player = m_current_player;
                    }
                    else if (m_current_player != null && m_next_player != null)
                    {
                        m_next_player.reset();
                        t_player = m_next_player;
                    }
                    else
                    {
                        t_player = new SoundPlayer();
                        if (m_current_player == null)
                            m_current_player = t_player;
                        else
                            m_next_player = t_player;
                    }
                    
                    if (t_player == null)
                        return false;
                    return t_player.setSound(p_sound, false, false, p_sound_path, p_is_asset, p_callback_handle);
				}
				default:
					return false;
            }
        }
        
        public boolean stop()
        {
            // when we want to stop a channel, clear the current player (but keep ready for a new sound)
            // and completely remove the next player in an effort to minimise the number of unecessary player objects 
            if (m_current_player != null)
            {
                m_current_player.reset();                
                if (m_next_player != null)
                {
                    m_next_player.release();
                    m_next_player = null;
                }
                return true;
            }
            return true;
        }
        
        public boolean pause()
        {
            if (m_current_player != null)
                return m_current_player.pause();
            return true;
        }
        
        
        public boolean resume()
        {
            if (m_current_player != null)
                return m_current_player.play();
            return true;
        }
        
        public boolean setVolume(int p_volume)
        {
            m_volume = p_volume;
            if (m_current_player != null)
                m_current_player.m_player.setVolume(m_volume / 100.0f, m_volume / 100.0f);
            return true;
        }
        
        public int getVolume()
        {
            return m_volume;
        }
        
        public String getSound()
        {
            if (m_current_player != null)
                return m_current_player.getSound();
            return null;
        }
        
        public String getNextSound()
        {
            if (m_next_player != null)
                return m_next_player.getSound();
            return null;
        }
        
        public int getStatus()
        {
            // playing: the current player is playing, or there is an unprepared sound waiting to be played
            // paused:  the current player is not playing but there is a sound ready (or being readied) in the player
            // stopped: all other cases
            if (m_current_player != null)
            {
                if (m_current_player.m_player.isPlaying() || (m_current_player.isPending() && !m_current_player.isPrepared()))
                    return k_channel_status_playing;
                else if (!m_current_player.m_player.isPlaying() && (m_current_player.isPrepared() || m_current_player.getSound() != null))
                    return k_channel_status_paused;
            }
            return k_channel_status_stopped;
        }
        
        private void playerComplete(long p_callback_handle)
        {
            if (m_current_player != null)
            {
                doSoundFinishedOnChannel(m_name, m_current_player.getSound(), p_callback_handle);
                m_engine.wakeEngineThread();
            }
            
            // when the current player finishes playing, start up the next player and remove the current player
            // if there is no next player, just reset the current player so that it is in the stopped state
            if (m_next_player != null && m_next_player.getSound() != null)
            {
                m_next_player.play();
                if (m_current_player != null)
                    m_current_player.release();
                m_current_player = m_next_player;
                m_next_player = null;
            }
            else if (m_current_player != null)
                m_current_player.reset();
        }
        
        private class SoundPlayer
        {
            public MediaPlayer m_player;
            
            private boolean m_prepared;
            private boolean m_pending;
            private String m_sound;
            private long m_callback_handle;
                        
            public SoundPlayer()
            {
                m_prepared = false;
                m_pending = false;
                m_sound = null;
                m_callback_handle = -1;
                
                // MM-2012-10-15: [[ Bug 10437 ]] On the Kindle Fire, use the patched MediaPlayer, which invokes the completion listener at the correct point.
                if (android.os.Build.MANUFACTURER.equals("Amazon") && android.os.Build.MODEL.equals("Kindle Fire"))
                    m_player = new PatchedMediaPlayer();
                else
                    m_player = new MediaPlayer();
                if (m_player == null)
                    return;

                m_player.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
                {
                    public void onCompletion(MediaPlayer p_player)
                    {
                        playerComplete(m_callback_handle);
                    }
                });                
                m_player.setOnInfoListener(new MediaPlayer.OnInfoListener()
                {
                    public boolean onInfo(MediaPlayer mp, int what, int extra)
                    {
                        return false;
                    }
                });                
                m_player.setOnErrorListener(new MediaPlayer.OnErrorListener()
                {
                    public boolean onError(MediaPlayer mp, int what, int extra)
                    {
                        Log.i("revandroid", "MultiChannelSound Error(" + what + " " + extra + ")");
                        return false;
                    }
                });
                m_player.setOnPreparedListener(new MediaPlayer.OnPreparedListener()
                {
                    public void onPrepared(MediaPlayer p_player)
                    {
                        m_prepared = true;
                        if (m_pending)
                        {
                            m_pending = false;
                            p_player.start();
                        }
                    }
                });
            }
            
            public boolean setSound(String p_sound, boolean p_looping, boolean p_play, String p_sound_path, boolean p_is_asset, long p_callback_handle)
            {
                m_pending = p_play;
                m_prepared = false;
                m_sound = p_sound_path;
                m_callback_handle = p_callback_handle;
                m_player.setLooping(p_looping);
                m_player.setVolume(m_volume / 100.0f, m_volume / 100.0f);
                try {
                    if (p_is_asset)
                    {
                        AssetFileDescriptor t_descriptor;
                        t_descriptor = m_engine.getContext().getAssets().openFd(p_sound);
                        m_player.setDataSource(t_descriptor.getFileDescriptor(), t_descriptor.getStartOffset(), t_descriptor.getLength());
                    }
                    else
                    {
                        FileInputStream t_filestream = new FileInputStream(p_sound);
                        m_player.setDataSource(t_filestream.getFD());
                    }                    
                    m_player.prepareAsync();
                } catch (Exception e) {
                    Log.i("revandroid", e.toString());
					// IM-2013-11-13: [[ Bug 11428 ]] Unset callback handle to prevent early release of callback object when resetting
					m_callback_handle = -1;
                    reset();
                    return false;
                }
                return true;
            }
            
            public String getSound()
            {
                return m_sound;
            }
            
            // reset the player, putting it in a freshly created state
            public void reset()
            {
                if (m_player.isPlaying())
                    m_player.stop();
                m_sound = null;
                m_pending = false;
                m_prepared = false;
                m_player.reset();
                if (m_callback_handle >= 0)
                    doSoundReleaseCallbackHandle(m_callback_handle);
                m_callback_handle = -1;
            }
            
            // release all resources associated with this player
            public void release()
            {
                if (m_player != null)
                    m_player.release();
                m_player = null;
                m_sound = null;
                if (m_callback_handle >= 0)
                    doSoundReleaseCallbackHandle(m_callback_handle);
            }
            
            public boolean play()
            {
                // if the sound is not yet prepared, set the player to pending so that when the sound
                // is prepared we know to begin playing it
                if (m_player != null && !m_player.isPlaying())
                {
                    if (m_prepared)
                        m_player.start();
                    else
                        m_pending = true;
                    return true;
                }
                return true;
            }
                        
            public boolean pause()
            {
                if (m_player != null && m_player.isPlaying())
                {
                    m_player.pause();
                    return true;
                }
                return true;
            }
            
            public boolean isPending()
            {
                return m_pending;
            }
            
            public boolean isPrepared()
            {
                return m_prepared;
            }                        
    
            // MM-2012-10-15: [[ Bug 10437 ]] Patched MediaPlayer for use on Kindle Fire.  It appears that the completion listerner
            //   is triggered to early on the Fire. Instead, invoke the listerner ourselves, using the duration of the audio clip.
            // MM-2102-10-17: Updated to patch the isPlaying method to make sure it only returns false when the new completion callback is fired.
            private class PatchedMediaPlayer extends MediaPlayer
            {
                private MediaPlayer.OnCompletionListener m_completion_listener;
                private Runnable m_completion_callback;
                private boolean m_playing;
                
                public PatchedMediaPlayer()
                {
                    super();
                    m_playing = false;
                    final MediaPlayer t_player = this;
                    m_completion_callback = new Runnable() {
                        public void run()
                        {
                            m_playing = false;
                            m_completion_listener.onCompletion(t_player);
                        }
                    };
                }
                
                public void start()
                {
                    m_playing = true;
                    m_engine.getHandler().postDelayed(m_completion_callback, getDuration() - getCurrentPosition());
                    super.start();
                }
                
                public void pause()
                {
                    m_playing = false;
                    m_engine.getHandler().removeCallbacks(m_completion_callback);
                    super.pause();
                }
                
                public void stop()
                {
                    m_playing = false;
                    m_engine.getHandler().removeCallbacks(m_completion_callback);
                    super.stop();
                }
                
                public boolean isPlaying()
                {
                    return m_playing;
                }
                
                public void setOnCompletionListener(MediaPlayer.OnCompletionListener p_listener)
                {
                    m_completion_listener = p_listener;
                }
            }
            
        }

    }
}
