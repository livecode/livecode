/* Copyright (C) 2017 LiveCode Ltd.

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

import android.os.*;
import android.app.*;
import android.content.*;

public class LiveCodeService extends Service
{
	public static final String TAG = "revandroid.LiveCodeService";
    
    @Override
    public IBinder onBind(Intent p_intent)
    {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        return Engine.getEngine().handleStartService(this, intent, flags, startId);
    }
    
    @Override
    public void onDestroy()
    {
        Engine.getEngine().handleFinishService(this);
    }
}
