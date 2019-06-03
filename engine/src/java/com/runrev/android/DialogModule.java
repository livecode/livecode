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

import android.app.*;
import android.content.*;
import android.os.*;
import android.text.*;
import android.util.*;
import android.view.*;
import android.widget.*;
import java.lang.reflect.*;
import java.util.*;

class DialogModule
{
    public static final String TAG = "revandroid.DialogModule";
    protected Engine m_engine;
    
    public DialogModule(Engine p_engine)
    {
        m_engine = p_engine;
    }
    
    public void showAnswerDialog(String p_title, String p_message, String p_ok_button, String p_cancel_button, String p_other_button)
	{
		DialogInterface.OnClickListener t_listener;
		t_listener = new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface p_dialog, int p_which)
			{
				if (p_which == DialogInterface.BUTTON_POSITIVE)
					p_which = 0;
				else if (p_which == DialogInterface.BUTTON_NEGATIVE)
					p_which = 1;
				else
					p_which = 2;
				m_engine.onAnswerDialogDone(p_which);
			} };
		
		DialogInterface.OnCancelListener t_cancel_listener;
		t_cancel_listener = new DialogInterface.OnCancelListener() {
			public void onCancel(DialogInterface p_dialog)
			{
				m_engine.onAnswerDialogDone(1);
			}
		} ;
		
		AlertDialog.Builder t_dialog;
		t_dialog = new AlertDialog.Builder(m_engine.getContext());
		t_dialog . setTitle(p_title);
		t_dialog . setMessage(p_message);
		t_dialog . setPositiveButton(p_ok_button, t_listener);
		if (p_cancel_button != null)
			t_dialog . setNegativeButton(p_cancel_button, t_listener);
		if (p_other_button != null)
			t_dialog . setNeutralButton(p_other_button, t_listener);
		t_dialog . setOnCancelListener(t_cancel_listener);
		t_dialog . show();
	}
    
	public void showAskDialog(boolean p_is_password, String p_title, String p_message, String p_initial, boolean p_hint)
	{
		final EditText t_textview = new EditText(m_engine.getContext());
		if (p_hint)
			t_textview.setHint(p_initial);
		else
			t_textview.setText(p_initial);
		
		int t_input_type = m_engine.getInputType(p_is_password);
		t_textview.setInputType(t_input_type);
		
		DialogInterface.OnClickListener t_listener;
		t_listener = new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface p_dialog, int p_which)
			{
				String t_result = null;
				if (p_which == DialogInterface.BUTTON_POSITIVE)
				{
					CharSequence t_chars = t_textview.getText(); // get string from text box
					t_result = t_chars.toString();
				}
				else
					t_result = null; // return null string
				m_engine.onAskDialogDone(t_result);
			}
		};
        
		DialogInterface.OnCancelListener t_cancel_listener;
		t_cancel_listener = new DialogInterface.OnCancelListener() {
			public void onCancel(DialogInterface p_dialog)
			{
				m_engine.onAskDialogDone(null);
			}
		} ;
		
		AlertDialog.Builder t_dialog_builder;
		t_dialog_builder = new AlertDialog.Builder(m_engine.getContext());
		t_dialog_builder.setTitle(p_title);
		t_dialog_builder.setView(t_textview);
		t_dialog_builder.setMessage(p_message);
		t_dialog_builder.setPositiveButton("OK", t_listener);
		t_dialog_builder.setNegativeButton("Cancel", t_listener);
		t_dialog_builder.setOnCancelListener(t_cancel_listener);
		AlertDialog t_dialog = t_dialog_builder.show();
	}

    public void showDatePicker(boolean p_with_min, boolean p_with_max, long p_min, long p_max, long p_current)
    {
        DialogInterface.OnCancelListener t_cancel_listener;
        t_cancel_listener = new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface p_dialog)
            {
                m_engine.onDatePickerDone(0, 0, 0, false);
            }
        };
        
        DatePickerDialog.OnDateSetListener t_date_listener;
        t_date_listener = new DatePickerDialog.OnDateSetListener()
        {
            public void onDateSet(DatePicker p_view, int p_year, int p_month, int p_day)
            {
                // IM-2012-05-09 android returns month value between 0-11
                m_engine.onDatePickerDone(p_year, p_month + 1, p_day, true);
            }
        };
        
        DialogInterface.OnDismissListener t_dismiss_listener;
        t_dismiss_listener = new DialogInterface.OnDismissListener() {
            public void onDismiss(DialogInterface p_dialog)
            {
                m_engine.onDatePickerDone(0, 0, 0, false);
            }
        };
        
        Calendar t_calendar = Calendar.getInstance();
        t_calendar.setTimeInMillis(p_current * 1000);
        
        DatePickerDialog t_dialog = new DatePickerDialog(m_engine.getContext(), android.R.style.Theme_DeviceDefault_Dialog_Alert, t_date_listener,
                                                         t_calendar.get(Calendar.YEAR),
                                                         t_calendar.get(Calendar.MONTH),
                                                         t_calendar.get(Calendar.DAY_OF_MONTH));
        t_dialog.setOnCancelListener(t_cancel_listener);
        t_dialog.setOnDismissListener(t_dismiss_listener);
        
        if (Build.VERSION.SDK_INT >= 11)
        {
            // use reflection to access API 11 methods; can change this if we ever switch to compiling against API version >= 11
            // set min / max dates
            DatePicker t_date_picker = null;
            
            try
            {
                Method t_getDatePicker;
                t_getDatePicker = t_dialog.getClass().getMethod("getDatePicker", (Class[])null);
                t_date_picker = (DatePicker)t_getDatePicker.invoke(t_dialog, (Object[])null);
                
                if (p_with_min)
                {
                    Method t_setMinDate;
                    t_setMinDate = t_date_picker.getClass().getMethod("setMinDate", new Class[] {Long.TYPE});
                    t_setMinDate.invoke(t_date_picker, new Object[] {p_min * 1000});
                }

                if (p_with_max)
                {
                    Method t_setMaxDate;
                    t_setMaxDate = t_date_picker.getClass().getMethod("setMaxDate", new Class[] {Long.TYPE});
                    t_setMaxDate.invoke(t_date_picker, new Object[] {p_max * 1000});
                }
            }
            catch (Exception e)
            {
            }
        }
        t_dialog.show();
    }

    public void showTimePicker(int p_hour, int p_minute)
    {
        DialogInterface.OnCancelListener t_cancel_listener;
        t_cancel_listener = new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface p_dialog)
            {
                m_engine.onTimePickerDone(0, 0, false);
            }
        };
        
        DialogInterface.OnDismissListener t_dismiss_listener;
        t_dismiss_listener = new DialogInterface.OnDismissListener() {
            public void onDismiss(DialogInterface p_dialog)
            {
                m_engine.onTimePickerDone(0, 0, false);
            }
        };
        
        TimePickerDialog.OnTimeSetListener t_time_listener;
        t_time_listener = new TimePickerDialog.OnTimeSetListener()
        {
            public void onTimeSet(TimePicker p_view, int p_hour, int p_minute)
            {
                m_engine.onTimePickerDone(p_hour, p_minute, true);
            }
        };

        TimePickerDialog t_dialog = new TimePickerDialog(m_engine.getContext(), android.R.style.Theme_DeviceDefault_Dialog_Alert, t_time_listener,
                                                         p_hour, p_minute,
                                                         false);
        t_dialog.setOnCancelListener(t_cancel_listener);
        t_dialog.setOnDismissListener(t_dismiss_listener);
        
        t_dialog.show();
    }
    
    protected int m_list_selection;
    protected boolean m_list_close_on_selection;
    AlertDialog m_list_dialog;
    
    public void showListPicker(String p_items[], String p_title, boolean p_item_selected, int p_selection_index, boolean p_use_hilite, boolean p_use_cancel, boolean p_use_done)
    {
        m_list_selection = p_selection_index - 1;
        m_list_close_on_selection = !p_use_done || p_use_hilite;
        
        Log.i(TAG, String.format("showListPicker(items, title, %b, %d, %b, %b, %b)", p_item_selected, p_selection_index, p_use_hilite, p_use_cancel, p_use_done));
        DialogInterface.OnClickListener t_item_click_listener;
		t_item_click_listener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface p_dialog, int p_which)
			{
                Log.i(TAG, "clicked:" + p_which);
				if (m_list_close_on_selection)
                {
                    // AL-2013-11-08 [[ Bug 11278 ]] Dismiss dialog before calling onListPickerDone to prevent premature continuation of script execution
                    m_list_dialog.dismiss();
                    m_engine.onListPickerDone(p_which, true);
                }
                else
                    m_list_selection = p_which;
			}
        };
		
		DialogInterface.OnCancelListener t_cancel_listener;
		t_cancel_listener = new DialogInterface.OnCancelListener() {
			public void onCancel(DialogInterface p_dialog)
			{
				m_engine.onListPickerDone(0, false);
			}
		} ;
		
		DialogInterface.OnClickListener t_button_click_listener;
		t_button_click_listener = new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface p_dialog, int p_which)
			{
				if (p_which == DialogInterface.BUTTON_POSITIVE)
					m_engine.onListPickerDone(m_list_selection, true);
				else if (p_which == DialogInterface.BUTTON_NEGATIVE)
					m_engine.onListPickerDone(0, false);
			} };

		AlertDialog.Builder t_dialog_builder;
		t_dialog_builder = new AlertDialog.Builder(m_engine.getContext());
        if (p_title != null)
            t_dialog_builder . setTitle(p_title);

        if (!p_use_hilite)
            t_dialog_builder.setSingleChoiceItems(p_items, p_item_selected ? p_selection_index - 1 : -1, t_item_click_listener);
        else
            t_dialog_builder . setItems(p_items, t_item_click_listener);

        if (p_use_cancel)
            t_dialog_builder . setNegativeButton("Cancel", t_button_click_listener);
        if (p_use_done)
            t_dialog_builder . setPositiveButton("Done", t_button_click_listener);
        
		t_dialog_builder . setOnCancelListener(t_cancel_listener);
        
        m_list_dialog = t_dialog_builder.create();
        
        if (p_item_selected)
            m_list_dialog.getListView().setSelection(p_selection_index - 1);
		m_list_dialog.show();

    }
}
