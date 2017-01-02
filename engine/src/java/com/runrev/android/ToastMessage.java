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

import android.widget.Toast;
import android.os.CountDownTimer;

public class ToastMessage
{
    protected Engine m_engine;

    private Toast m_toast;

    public ToastMessage(Engine p_engine)
    {
        m_engine = p_engine;
    }

    public void toast(String p_msg, int p_duration)
    {
        m_toast = Toast.makeText(m_engine.getContext(), p_msg, Toast.LENGTH_LONG);

        CountDownTimer t_countdown;
   		t_countdown = new CountDownTimer(p_duration, 1000) 
   		{
      		public void onTick(long millisUntilFinished) 
      		{
         		m_toast.show();
      		}
      		public void onFinish() 
      		{
        		m_toast.cancel();
        	}
   		};
   m_toast.show();
   t_countdown.start();
   }

}
