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

import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.Ndef;

import java.util.HashMap;
import java.util.Map;

public class NFCModule
{    
	public static final String TAG = "revandroid.NFCModule";
	
    ////////////////////////////////////////////////////////////////////////////////
    // Native Functions
    ////////////////////////////////////////////////////////////////////////////////
	
	public static native void doTagReceived(Map<String, Object> p_tag);
	
    ////////////////////////////////////////////////////////////////////////////////
    // Constants
    ////////////////////////////////////////////////////////////////////////////////  
	
	
    ////////////////////////////////////////////////////////////////////////////////
    // Member variables
    ////////////////////////////////////////////////////////////////////////////////

    private Engine m_engine;
	
	private NfcAdapter m_nfc;
	private boolean m_enable_dispatch;
	
    ////////////////////////////////////////////////////////////////////////////////
    // Constructor
    ////////////////////////////////////////////////////////////////////////////////

    public NFCModule(Engine p_engine)
    {
        m_engine = p_engine;
		m_nfc = NfcAdapter.getDefaultAdapter(p_engine.getContext());
		m_enable_dispatch = false;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // Public API
    ////////////////////////////////////////////////////////////////////////////////

    public void onPause()
    {
		if (m_enable_dispatch)
		{
			stopDispatch();
		}
    }
	
    public void onResume()
    {
		if (m_enable_dispatch)
			startDispatch();
    }
	
	public void onNewIntent(Intent p_intent)
	{
        if (isAvailable())
        {
            handleIntent(p_intent);
        }
	}
	
	public boolean isAvailable()
	{
		return m_nfc != null;
	}
	
	public boolean isEnabled()
	{
		return m_nfc != null && m_nfc.isEnabled();
	}
	
	public void setDispatchEnabled(boolean p_enabled)
	{
		if (m_enable_dispatch == p_enabled)
			return;
		
		m_enable_dispatch = p_enabled;
		if (m_enable_dispatch)
			startDispatch();
		else
			stopDispatch();
	}
	
    ////////////////////////////////////////////////////////////////////////////////
    // Internals
    ////////////////////////////////////////////////////////////////////////////////
    
	private void startDispatch()
	{
		if (isEnabled())
		{
			try
			{
				Intent intent = new Intent(m_engine.getActivity().getApplicationContext(), m_engine.getActivity().getClass());
				intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
				
				PendingIntent pendingIntent = PendingIntent.getActivity(m_engine.getActivity().getApplicationContext(), 0, intent, 0);
				m_nfc.enableForegroundDispatch(m_engine.getActivity(), pendingIntent, null, null);
			}
			catch (SecurityException e)
			{
				// don't have NFC permissions
			}
		}
	}
	
	private void stopDispatch()
	{
		if (isEnabled())
		{
			try
			{
				m_nfc.disableForegroundDispatch(m_engine.getActivity());
			}
			catch (SecurityException e)
			{
				// don't have NFC permissions
			}
		}
	}
	
	private void handleIntent(Intent p_intent)
	{
		String t_action = p_intent.getAction();
        
        // t_action can be NULL
		if (NfcAdapter.ACTION_NDEF_DISCOVERED.equals(t_action) ||
			NfcAdapter.ACTION_TECH_DISCOVERED.equals(t_action) ||
			NfcAdapter.ACTION_TAG_DISCOVERED.equals(t_action))
		{
			Map<String, Object> t_tag_map = new HashMap<String, Object>();
			
			// Todo - expand this to include more information from the tag
			Tag t_tag = p_intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
			
			t_tag_map.put("id", t_tag.getId());
			
			Ndef t_ndef = Ndef.get(t_tag);
			// check if tag contains Ndef data
			if (t_ndef != null)
			{
				NdefMessage t_ndef_message = t_ndef.getCachedNdefMessage();
				if (t_ndef_message != null)
				{
					Map<String, Object> t_ndef_map = new HashMap<String, Object>();
					
					int t_index = 1;
					for (NdefRecord t_record : t_ndef_message.getRecords())
					{
						Map<String, Object> t_record_map = new HashMap<String, Object>();
						t_record_map.put("tnf", Integer.valueOf(t_record.getTnf()));
						
						if (t_record.getId().length != 0)
							t_record_map.put("id", t_record.getId());
						if (t_record.getType().length != 0)
							t_record_map.put("type", t_record.getType());
						if (t_record.getPayload().length != 0)
							t_record_map.put("payload", t_record.getPayload());
						
						t_ndef_map.put(Integer.toString(t_index), t_record_map);
					}
					
					t_tag_map.put("ndef", t_ndef_map);
				}
			}
			
			doTagReceived(t_tag_map);
		}
	}
}
