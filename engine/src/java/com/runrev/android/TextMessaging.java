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
import android.net.*;
import android.content.*;
import android.telephony.SmsManager;


public class TextMessaging
{
    protected Engine m_engine;
    static SmsManager m_sms_manager = null;
    
	public TextMessaging(Engine p_engine)
	{
        m_engine = p_engine;
	}
	
    public Intent canSendTextMessage()
    {
        Intent t_send_intent = new Intent(Intent.ACTION_VIEW);
        t_send_intent.putExtra("address", "555");
        t_send_intent.putExtra("sms_body", "message");
        t_send_intent.setData(Uri.parse("smsto:555"));
        return t_send_intent;
    }

    public Intent composeTextMessage(String p_recipients, String p_body)
    {
        Intent t_send_intent = new Intent(Intent.ACTION_VIEW);
        t_send_intent.putExtra("address", p_recipients);
        t_send_intent.putExtra("sms_body", p_body);
        t_send_intent.setData(Uri.parse("smsto:" + p_recipients));
        return t_send_intent;
	}    
}
