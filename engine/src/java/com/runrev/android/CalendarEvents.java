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
import android.net.Uri;
import android.database.Cursor;

import java.util.*;

public class CalendarEvents
{
    private static final int PICK_CALENDAR_RESULT = 10;
    private static final int CREATE_CALENDAR_RESULT = 11;
    private static final int UPDATE_CALENDAR_RESULT = 12;
    private static final int SHOW_CALENDAR_RESULT = 13;
    
    protected Engine m_engine;
    protected Activity m_activity;
    
	public CalendarEvents(Engine p_engine, Activity p_activity)
	{
Log.i("revandroid", "CT Calendar");
        m_engine = p_engine;
        m_activity = p_activity;
	}

    public void showCalendarEvent(String p_calendar_event_id)
    {
Log.i("revandroid", "CT showCalendarEvent: " + p_calendar_event_id);
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
    
    public void createCalendarEvent()
    {
Log.i("revandroid", "CT createCalendarEvent");
        Intent t_create_intent = new Intent(Intent.ACTION_EDIT);  
        t_create_intent.setType("vnd.android.cursor.item/event");
        m_activity.startActivityForResult(t_create_intent, CREATE_CALENDAR_RESULT);
    }
    
    public void updateCalendarEvent(String p_new_calendar_event_data)
    {
Log.i("revandroid", "CT UpdateCalendarEvent");
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
    
    public void getCalendarEventData(String p_calendar_event_id)
    {
Log.i("revandroid", "CT getCalendarEventData: " + p_calendar_event_id);
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
    
    public void removeCalendarEvent(String p_calendar_event_id)
    {
Log.i("revandroid", "CT removeCalendarEvent: " + p_calendar_event_id);
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
    
    public void addCalendarEvent(String p_eventid, String p_title, String p_note, String p_location,
                                 boolean p_alldayset, boolean p_allday,
                                 boolean p_startdateset, 
                                 boolean p_enddateset, 
                                 int p_alert1, int p_alert2, String p_frequency, int p_frequencycount,
                                 int p_frequencyinterval, String p_calendar)
    {        
Log.i("revandroid", "CT addCalendarEvent: " + p_note);
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
    
    public void findCalendarEvent(String p_calendar_event_id)
    {
Log.i("revandroid", "CT findCalendarEvent: " + p_calendar_event_id);
// STILL TO BE IMPLEMENTED IPA LEVEL 14
    }
}
