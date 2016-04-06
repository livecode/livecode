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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class PushReceiver extends BroadcastReceiver
{
    public static final String TAG = "revandroid.PushReceiver";
    
    public void onReceive(Context context, Intent intent)
    {
        Log.i(TAG, "received action:" + intent.getAction());
        if (intent.getAction().equals("com.google.android.c2dm.intent.REGISTRATION"))
        {
            handleRegistration(context, intent);
        }
        else if (intent.getAction().equals("com.google.android.c2dm.intent.RECEIVE"))
        {
            handleMessage(context, intent);
        }
        else
        {
            Log.i(TAG, "unhandled intent: " + intent);
        }
    }
    
    private void handleRegistration(Context context, Intent intent)
    {
        String t_error = intent.getStringExtra("error");
        String t_unregistered = intent.getStringExtra("unregistered");
        String t_registration_id = intent.getStringExtra("registration_id");
        if (t_error != null)
        {
            // error
            Log.i(TAG, "registration error: " + t_error);
            NotificationModule.handleRemoteRegistrationError(context, t_error);
        }
        else if (t_unregistered != null)
        {
            // unregistered
            NotificationModule.handleRemoteUnregistration(context);
        }
        else if (t_registration_id != null)
        {
            // registered!
            NotificationModule.handleRemoteRegistration(context, t_registration_id);
        }
    }
    
    private void handleMessage(Context context, Intent intent)
    {
        String t_body = intent.getStringExtra("body");
        String t_title = intent.getStringExtra("title");
        String t_user_info = intent.getStringExtra("payload");

        if (t_body == null)
            t_body = "Notification received";
        if (t_title == null)
            t_title = Utils.getLabel(context);
        if (t_user_info == null)
            t_user_info = "";
        
        boolean t_play_sound = false;
        int t_badge_value = 0;

        String t_extra = intent.getStringExtra("play_sound");

        if (t_extra != null)
            t_play_sound = Boolean.valueOf(t_extra);
        
        try
        {
            t_extra = intent.getStringExtra("badge_value");
            if (t_extra != null)
                t_badge_value = Integer.valueOf(t_extra);
        }
        catch (NumberFormatException e)
        {
            t_badge_value = 0;
        }
        
        NotificationModule.handleRemoteMessage(context, t_body, t_title, t_user_info, t_play_sound, t_badge_value);
    }
}

