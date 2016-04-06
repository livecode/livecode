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
import android.view.KeyEvent;
import java.util.ArrayList;

public class BusyIndicator
{
    protected Engine m_engine;
    // PM-2014-12-10: [[ Bug 13253 ]] Allow multiple instances of mobileBusyIndicator instead of a single one
    protected ArrayList<ProgressDialog> m_dialog;
    protected int m_progress_dialog_array_pos;
    
	public BusyIndicator(Engine p_engine)
	{
        m_engine = p_engine;
        m_progress_dialog_array_pos = -1;
        m_dialog = new ArrayList<ProgressDialog>();
	}

    public void showBusyIndicator(String p_label)
    {
        // PM-2014-12-10: [[ Bug 13253 ]] Dynamically populate the mobileBusyIndicator array
        m_progress_dialog_array_pos++;
        m_dialog.add(new ProgressDialog(m_engine.getContext()));
        
        ProgressDialog t_dialog = ProgressDialog.show(m_engine.getContext(), "", p_label, true);
        m_dialog.set(m_progress_dialog_array_pos, t_dialog);
        
        t_dialog.setOnKeyListener(new DialogInterface.OnKeyListener()
        {
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event)
            {
                if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0)
                {
                    // If backKey is pressed, dismiss all mobileBusyIndicators being in progress before exiting to prevent a leak.
                    for (ProgressDialog t_dialog : m_dialog)
                        t_dialog.dismiss();
                    
                    m_engine.doBackPressed();
                    return true;
                }
                return false;
            }
        });
    }

    public void hideBusyIndicator()
    {
        if (m_dialog.get(m_progress_dialog_array_pos) != null)
        {
            // PM-2014-12-10: [[ Bug 13253 ]] Dismiss the most recent mobileBusyIndicator
            m_dialog.get(m_progress_dialog_array_pos).dismiss();
            m_progress_dialog_array_pos--;
        }
 	}
}
