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

import android.util.Log;
import android.media.*;
import android.app.*;
import android.content.*;
import android.os.Vibrator;
import android.net.Uri;
import android.content.*;

import java.io.IOException;

public class Alert
{
    protected Engine m_engine;
    static MediaPlayer m_beep_manager = null;
    static Vibrator m_vibrate_manager = null;
    Uri m_alert_sound = null;
    static boolean m_prepared = false;
    static int m_beep_count = 0;
    
	public Alert(Engine p_engine)
	{
        m_engine = p_engine;
        // Set up the beep playing environment
        if (m_beep_manager == null)
        {
            m_beep_manager = new MediaPlayer ();
           
            m_beep_manager.setOnCompletionListener(new MediaPlayer.OnCompletionListener()
            {
                public void onCompletion(MediaPlayer p_player)
                {
                    m_beep_count--;
                    if (m_beep_count > 0)
                        m_beep_manager.start ();
                }
            });

            m_beep_manager.setOnErrorListener(new MediaPlayer.OnErrorListener()
            {
                public boolean onError(MediaPlayer mp, int what, int extra)
                {
                    return false;
                }
            });

            m_beep_manager.setOnPreparedListener(new MediaPlayer.OnPreparedListener()
            {
                public void onPrepared(MediaPlayer p_player)
                {
                    m_prepared = true;
                    m_beep_manager.start();
                }
            });
        } 
	}

    public void doBeep(int p_number_of_beeps) throws IOException
    {
        // Specify how many times we have to beep
        if (p_number_of_beeps > 0)
            m_beep_count += p_number_of_beeps;

        if (m_beep_manager != null)
        {
            // Get the beep sound
            if (m_alert_sound == null)
            {
// We are not trying to play the Alarm Type.
// This beep type does not terminate until the application has been uninstalled or the device reset.
// This behaviour is reported on a number of devices and applications. Check out Google.

                m_alert_sound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
                if(m_alert_sound == null)
                {
                    m_alert_sound = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_RINGTONE); 
                }
            }
            if ((m_alert_sound != null) && (m_beep_count > 0) && (!m_beep_manager.isPlaying ()))
            {
                if (m_prepared == false)
                {
                    m_beep_manager.setAudioStreamType (AudioManager.STREAM_ALARM);
                    m_beep_manager.setDataSource (m_engine.getContext(), m_alert_sound);
                    m_beep_manager.setLooping(false);
                    m_beep_manager.prepare();
                }
                else
                {
                    m_beep_manager.start();
                }
            }
        }
    }
   
    public void doVibrate(int p_number_of_vibrations)
    {
        long[] t_vibrate_pattern;
        if (m_vibrate_manager == null)
            m_vibrate_manager = (Vibrator) m_engine.getContext().getSystemService(Context.VIBRATOR_SERVICE);// Vibrator.getDefault();
        if (m_vibrate_manager != null)
        {
            try
            {
                t_vibrate_pattern = new long [p_number_of_vibrations * 2];
                for (int i = 0; i < p_number_of_vibrations; i++)
                {
                    t_vibrate_pattern[i*2] = 100;
                    t_vibrate_pattern[i*2+1] = 400;
                }
                m_vibrate_manager.vibrate (t_vibrate_pattern, -1);
            }
            catch (Exception e)
            {
                Log.i("revandroid", e.toString());
            }
        }
    }
}
