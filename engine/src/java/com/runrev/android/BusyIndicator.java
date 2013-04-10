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

package com.runrev.android;

import android.util.Log;
import android.media.*;
import android.app.*;
import android.content.*;
import android.view.KeyEvent;

public class BusyIndicator
{
    
    protected Engine m_engine;
    protected ProgressDialog m_dialog = null;
    
	public BusyIndicator(Engine p_engine)
	{
        m_engine = p_engine;
	}

    public void showBusyIndicator(String p_label)
    {
        m_dialog = ProgressDialog.show(m_engine.getContext(), "", p_label, true);
        m_dialog.setOnKeyListener(new DialogInterface.OnKeyListener() 
        {
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event)
            {
                if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0)
                {
                    m_engine.doBackPressed();
                    return true;
                }
                return false;
            }
        });
    }

    public void hideBusyIndicator()
    {
        if (m_dialog != null)
        {
            m_dialog.dismiss();
            m_dialog = null;
        }
 	}
}