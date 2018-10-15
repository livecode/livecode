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
import android.app.Notification.Builder;
import android.database.*;
import android.database.sqlite.*;
import android.content.*;
import android.util.*;
import android.os.Build;
import android.graphics.Color;

import java.io.*;
import java.util.*;


public class NotificationModule
{
    public static final String TAG = "revandroid.NotificationModule";
    
    public static final int NOTIFY_TYPE_LOCAL = 1;
    public static final int NOTIFY_TYPE_REMOTE = 2;
    
    public static final String ACTION_DISPATCH_NOTIFICATIONS = "com.runrev.android.DISPATCH_NOTIFICATIONS";
    public static final String ACTION_CANCEL_NOTIFICATION = "com.runrev.android.CANCEL_NOTIFICATION";
    
    protected Engine m_engine;

    protected static Class s_receiver_class = null;
    protected static AlarmManager s_manager = null;
    
    public NotificationModule(Engine p_engine)
    {
        m_engine = p_engine;
    }
    
    protected static AlarmManager getAlarmManager(Context context)
    {
        if (s_manager == null)
        {
            s_manager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        }
        
        return s_manager;
    }
    
    protected static Class getReceiverClass(Context context)
    {
        if (s_receiver_class == null)
        {
            String t_class_fqn = context.getPackageName() + ".AppReceiver";
            try
            {
                Class t_class = Class.forName(t_class_fqn);
                s_receiver_class = t_class;
            }
            catch (Exception e)
            {
                s_receiver_class = null;
            }
        }
        return s_receiver_class;
    }
    
    protected static void resetTimer(Context context)
    {
        Class t_receiver_class = getReceiverClass(context);
        if (t_receiver_class == null)
            return;
        
        Intent t_intent = new Intent(context, t_receiver_class);
        t_intent.setAction(ACTION_DISPATCH_NOTIFICATIONS);
        PendingIntent t_pending_intent = PendingIntent.getBroadcast(context, 0, t_intent, 0);

        long t_next_notification = getEarliestNotificationTime(context);
        
        if (t_next_notification == Long.MAX_VALUE)
        {
            getAlarmManager(context).cancel(t_pending_intent);
        }
        else
        {           
            getAlarmManager(context).set(AlarmManager.RTC, t_next_notification, t_pending_intent);
        }
    }

////////////////////////////////////////////////////////////////////////////////

    public long createLocalNotification(String p_body, String p_action, String p_user_info, long p_milliseconds, boolean p_play_sound, int p_badge_value)
    {
        // add notification to db
        long t_row_id;
        t_row_id = insertNotification(m_engine.getContext(), NOTIFY_TYPE_LOCAL, p_milliseconds, p_body, p_action, p_user_info, p_play_sound, p_badge_value);
        
        // if new notification occurs before existing ones reset alarm timer
        resetTimer(m_engine.getContext());
        
        return t_row_id;
    }
    
    public String getRegisteredNotifications()
    {
        long[] t_ids = getNotificationIds(m_engine.getContext());
        
        if (t_ids == null)
        {
            return null;
        }
        
        String t_notifications = "";
        for (int i = 0; i < t_ids.length; i++)
        {
            if (i == 0)
                t_notifications += t_ids[i];
            else
                t_notifications += "," + t_ids[i];
        }

        return t_notifications;
    }
    
    public boolean getNotificationDetails(long p_id)
    {
        SQLiteDatabase t_db = null;
        try
        {
            t_db = new DBHelper(m_engine.getContext()).getReadableDatabase();
            
            // loop through db
            Cursor t_cursor = t_db.query(DB_TABLE, new String[] {NOTIFY_BODY, NOTIFY_ACTION, NOTIFY_INFO, NOTIFY_TIME, NOTIFY_PLAY_SOUND, NOTIFY_BADGE}, String.format("%s = %d", NOTIFY_ID, p_id), null, null, null, null);
            // check that id exists
            if (t_cursor.getCount() == 0)
                return false;
            
            t_cursor.moveToFirst();
            
            String t_body = t_cursor.getString(0);
            String t_action = t_cursor.getString(1);
            String t_info = t_cursor.getString(2);
            long t_time = t_cursor.getLong(3);
            boolean t_play_sound = t_cursor.getInt(4) != 0;
            int t_badge_value = t_cursor.getInt(5);
            
            t_cursor.close();
            t_db.close();
            
            return doReturnNotificationDetails(t_body, t_action, t_info, t_time, t_play_sound, t_badge_value);
        }
        catch (Exception e)
        {
            Log.i(TAG, e.toString());
            return false;
        }
    }
    
    public boolean cancelLocalNotification(long id)
    {
        removeNotification(m_engine.getContext(), id);
        resetTimer(m_engine.getContext());
        return true;
    }
    
    public boolean cancelAllLocalNotifications()
    {
        long[] t_ids = getNotificationIds(m_engine.getContext());
        if (t_ids == null)
            return false;
        
        for (int i = 0; i < t_ids.length; i++)
            removeNotification(m_engine.getContext(), t_ids[i]);
        
        resetTimer(m_engine.getContext());
        return true;
    }

////////////////////////////////////////////////////////////////////////////////

    public void dispatchNotifications()
    {
        SQLiteDatabase t_db = null;
        try
        {
            t_db = new DBHelper(m_engine.getContext()).getWritableDatabase();
            
            long t_current_time = System.currentTimeMillis();
            
            // loop through db
            // if any notifications are due, send them
            Cursor t_cursor = t_db.query(DB_TABLE, new String[] {NOTIFY_TYPE, NOTIFY_INFO}, String.format("%s < %d", NOTIFY_TIME, t_current_time), null, null, null, NOTIFY_TIME);
            t_cursor.moveToFirst();
            while (!t_cursor.isAfterLast())
            {
                int t_type = t_cursor.getInt(0);
                String t_info = t_cursor.getString(1);
                
                if (t_type == NOTIFY_TYPE_LOCAL)
                    doLocalNotification(t_info);
                else if (t_type == NOTIFY_TYPE_REMOTE)
                    doRemoteNotification(t_info);
                
                t_cursor.moveToNext();
            }
            t_cursor.close();
            // remove dispatched notifications from db
            t_db.delete(DB_TABLE, String.format("%s < %d", NOTIFY_TIME, t_current_time), null);
            t_db.close();
            
            // reset timer to time of next notification, or cancel if none
            resetTimer(m_engine.getContext());
            
            // remove any statusbar messages
            NotificationManager t_notification_manager = (NotificationManager)m_engine.getContext().getSystemService(Context.NOTIFICATION_SERVICE);
            t_notification_manager.cancelAll();

            m_engine.wakeEngineThread();
        }
        catch (Exception e)
        {
            
        }
    }
    
////////////////////////////////////////////////////////////////////////////////
    
    protected static long getEarliestNotificationTime(Context context)
    {
        long t_earliest = Long.MAX_VALUE;
        
        SQLiteDatabase t_db = null;
        try
        {
            // open db for read
            t_db = new DBHelper(context).getReadableDatabase();
            
            // query minimum timestamp
            Cursor t_cursor = t_db.rawQuery(String.format("SELECT min(%s) FROM %s WHERE NOT %s", NOTIFY_TIME, DB_TABLE, NOTIFY_SHOWN), null);
            
            t_cursor.moveToFirst();
            if (!t_cursor.isNull(0))
                t_earliest = t_cursor.getLong(0);
            
            t_cursor.close();
            t_db.close();
        }
        catch (Exception e)
        {
            Log.i(TAG, e.toString());
        }
        
        return t_earliest;
    }
    
    protected static long[] getNotificationIds(Context context)
    {
        long[] t_ids = null;
        
        SQLiteDatabase t_db = null;
        try
        {
            t_db = new DBHelper(context).getReadableDatabase();
            
            // loop through db
            Cursor t_cursor = t_db.query(DB_TABLE, new String[] {NOTIFY_ID}, null, null, null, null, NOTIFY_ID);
            
            // allocate array
            t_ids = new long[t_cursor.getCount()];
            int i = 0;
            
            t_cursor.moveToFirst();
            while (!t_cursor.isAfterLast())
            {
                t_ids[i] = t_cursor.getLong(0);
                i += 1;
                t_cursor.moveToNext();
            }
            t_cursor.close();
            t_db.close();
        }
        catch (Exception e)
        {
            return null;
        }
        
        return t_ids;
    }
    
    protected static long insertNotification(Context context, int p_type, long p_milliseconds, String p_body, String p_action, String p_info, boolean p_play_sound, int p_badge_value)
    {
        try
        {
            // set up insertion values
            ContentValues t_values = new ContentValues();
            
            t_values.put(NOTIFY_TYPE, p_type);
            t_values.put(NOTIFY_TIME, p_milliseconds);
            t_values.put(NOTIFY_BODY, p_body);
            t_values.put(NOTIFY_ACTION, p_action);
            t_values.put(NOTIFY_INFO, p_info);
            t_values.put(NOTIFY_PLAY_SOUND, p_play_sound);
            t_values.put(NOTIFY_BADGE, p_badge_value);
            
            // open database for read/write
            SQLiteDatabase t_db = new DBHelper(context).getWritableDatabase();
            
            long t_row_id;
            t_row_id = t_db.insert(DB_TABLE, null, t_values);
            t_db.close();
            
            return t_row_id;
        }
        catch (Exception e)
        {
            return -1;
        }
    }
    
    protected static void removeNotification(Context context, long p_id)
    {
        // remove any statusbar notification for this id
        NotificationManager t_notification_manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        t_notification_manager.cancel((int)p_id);
        
        // remove notification from the DB
        try
        {
            SQLiteDatabase t_db = new DBHelper(context).getWritableDatabase();
            t_db.delete(DB_TABLE, String.format("%s = %d", NOTIFY_ID, p_id), null);
            t_db.close();
        }
        catch (Exception e)
        {
        }
    }
    
////////////////////////////////////////////////////////////////////////////////
    
    protected static final String DB_NAME = "lc_notification.db";
    protected static final int DB_VERSION = 1;
    
    protected static final String DB_TABLE = "notification";
    protected static final String NOTIFY_ID = "id";
    protected static final String NOTIFY_TYPE = "type";
    protected static final String NOTIFY_TIME = "time";
    protected static final String NOTIFY_BODY = "body";
    protected static final String NOTIFY_ACTION = "action";
    protected static final String NOTIFY_INFO = "info";
    protected static final String NOTIFY_PLAY_SOUND = "play_sound";
    protected static final String NOTIFY_BADGE = "badge";
    protected static final String NOTIFY_SHOWN = "shown";
    
    static class DBHelper extends SQLiteOpenHelper
    {
        DBHelper(Context context)
        {
            super(context, DB_NAME, null, DB_VERSION);
        }
        
        public void onCreate(SQLiteDatabase db)
        {
            String t_create_table = "CREATE TABLE " + DB_TABLE + " (" +
                NOTIFY_ID + " INTEGER PRIMARY KEY," +
                NOTIFY_TYPE + " INTEGER," +
                NOTIFY_TIME + " INTEGER," +
                NOTIFY_BODY + " VARCHAR," +
                NOTIFY_ACTION + " VARCHAR," +
                NOTIFY_INFO + " VARCHAR," +
                NOTIFY_PLAY_SOUND + " BOOLEAN," +
                NOTIFY_BADGE + " INTEGER," +
                NOTIFY_SHOWN + " BOOLEAN DEFAULT 0" +
            ");";
            
            db.execSQL(t_create_table);
        }
        
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)
        {
            db.execSQL("DROP TABLE IF EXISTS " + DB_TABLE);
            onCreate(db);
        }
    }

////////////////////////////////////////////////////////////////////////////////

    public static boolean onReceive(Context context, Intent intent)
    {
        if (intent.getAction().equals(ACTION_DISPATCH_NOTIFICATIONS))
        {
            if (Engine.isRunning())
            {
                // send message to engine service
                Engine.getEngine().post(new Runnable() {
                    public void run()
                    {
                        Engine.getEngine().dispatchNotifications();
                    }
                });
            }
            else
            {
                setupStatusBarNotification(context);
            }
            
            return true;
        }
        else if (intent.getAction().equals(ACTION_CANCEL_NOTIFICATION))
        {
            if (!Engine.isRunning())
            {
                int t_id = intent.getIntExtra(NOTIFY_ID, -1);
                if (t_id != -1)
                {
                    removeNotification(context, t_id);
                }
            }
            return true;
        }
        
        return false;
    }
    
    public static void setupStatusBarNotification(Context context)
    {
        try
        {
            SQLiteDatabase t_db = new DBHelper(context).getReadableDatabase();
            
            long t_current_time = System.currentTimeMillis();
            
            // loop through db
            // if any notifications are due, send them
            Cursor t_cursor = t_db.query(DB_TABLE, new String[] {NOTIFY_ID, NOTIFY_BODY, NOTIFY_ACTION, NOTIFY_PLAY_SOUND, NOTIFY_BADGE}, String.format("%s < %d AND NOT %s", NOTIFY_TIME, t_current_time, NOTIFY_SHOWN), null, null, null, NOTIFY_TIME);
            t_cursor.moveToFirst();
            
            boolean t_shown = false;

            while (!t_cursor.isAfterLast())
            {
                t_shown = true;
                
                int t_id = t_cursor.getInt(0);
                String t_body = t_cursor.getString(1);
                String t_action = t_cursor.getString(2);
                boolean t_play_sound = 0 != t_cursor.getInt(3);
                int t_badge_value = t_cursor.getInt(4);
                
                showStatusBarNotification(context, t_id, t_body, t_action, t_play_sound, t_badge_value);
                
                t_cursor.moveToNext();
            }
            
            t_cursor.close();
            t_db.close();
            
            // reset timer to time of next notification, or cancel if none
            if (t_shown)
            {
                ContentValues t_values = new ContentValues();
                t_values.put(NOTIFY_SHOWN, true);

                t_db = new DBHelper(context).getWritableDatabase();
                t_db.update(DB_TABLE, t_values, String.format("%s < %d", NOTIFY_TIME, t_current_time), null);
                t_db.close();
                
                resetTimer(context);
            }
        }
        catch (Exception e)
        {
            
        }
    }
    
    private static PendingIntent createNotificationContentIntent(Context context)
    {
        Class t_activity_class = null;
        
        String t_class_fqn = context.getPackageName() + ".mblandroid";
        try
        {
            Class t_class = Class.forName(t_class_fqn);
            t_activity_class = t_class;
        }
        catch (Exception e)
        {
            t_activity_class = null;
        }
        
        Intent t_intent = new Intent(context, t_activity_class);
        
        return PendingIntent.getActivity(context, 0, t_intent, 0);
    }
    
    private static PendingIntent createNotificationCancelIntent(Context context, int p_id)
    {
        Intent t_intent = new Intent(context, getReceiverClass(context));
        t_intent.setAction(ACTION_CANCEL_NOTIFICATION);
        t_intent.putExtra(NOTIFY_ID, p_id);
        
        return PendingIntent.getBroadcast(context, 0, t_intent, 0);
    }
    
    public static void showStatusBarNotification(Context context, int p_id, String p_body, String p_title, boolean p_play_sound, int p_badge_value)
    {
        // create status bar notification for activity
        int t_icon;
        t_icon = context.getResources().getIdentifier("drawable/notify_icon", null, context.getPackageName());
          
        NotificationManager t_notification_manager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        
        Notification.Builder t_builder = new Notification.Builder(context);
        t_builder.setSmallIcon(t_icon);
        t_builder.setAutoCancel(true);
        t_builder.setNumber(p_badge_value);
        
        if (p_play_sound)
        	t_builder.setDefaults(Notification.DEFAULT_SOUND);
        
        t_builder.setContentIntent(createNotificationContentIntent(context));
        t_builder.setContentText(p_body);
        t_builder.setContentTitle(p_title);
        
        // If the device runs Android OREO use NotificationChannel - Added in API level 26
        // Note the constant value of Build.VERSION_CODES.O is 26
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
        {
            String t_channel_id = "android_channel_id";
            String t_channel_description = "Notification Channel";
            CharSequence t_channel_name = "My Notifications";
            
            NotificationChannel t_notification_channel = new NotificationChannel(t_channel_id, t_channel_name, NotificationManager.IMPORTANCE_MAX);
            
            // Configure the notification channel.
            t_notification_channel.setDescription(t_channel_description);
            t_notification_channel.enableLights(true);
            t_notification_channel.enableVibration(true);
            t_notification_manager.createNotificationChannel(t_notification_channel);
            
            t_builder.setChannelId(t_channel_id);
        }
		
        t_builder.setDeleteIntent(createNotificationCancelIntent(context, p_id));

        t_notification_manager.notify(p_id, t_builder.build());
    }
    
////////////////////////////////////////////////////////////////////////////////

    // remote notification
    
    private static String s_registration_id = null;
    
    public static String getRemoteNotificationId()
    {
        return s_registration_id;
    }
    
    public boolean registerForRemoteNotifications(String senderEmail)
    {
        try
        {
            Intent t_registration_intent = new Intent("com.google.android.c2dm.intent.REGISTER");
            
            // Needed for registering devices that run Android Oreo+
            t_registration_intent.setPackage("com.google.android.gms");
            
            t_registration_intent.putExtra("app", PendingIntent.getBroadcast(m_engine.getContext(), 0, new Intent(), 0));
            t_registration_intent.putExtra("sender", senderEmail);
            m_engine.getContext().startService(t_registration_intent);
        }
        catch (SecurityException e)
        {
            // don't have permission to start c2dm registration service
            return  false;
        }
        
        return true;
    }

    public static void handleRemoteRegistrationError(Context context, String errorMessage)
    {
        if (Engine.isRunning())
        {
            final String t_error = errorMessage;
            Engine.getEngine().post(new Runnable() {
                public void run()
                {
                    doRemoteRegistrationError(t_error);
                    Engine.getEngine().wakeEngineThread();
                }
            });
        }
    }
    
    public static void handleRemoteRegistration(Context context, String registration_id)
    {
        // keep note of reg id, notify engine of registration success
        s_registration_id = registration_id;
        if (Engine.isRunning())
        {
            Engine.getEngine().post(new Runnable() {
                public void run()
                {
                    doRemoteRegistration(s_registration_id);
                    Engine.getEngine().wakeEngineThread();
                }
            });
        }
    }
    
    public static void handleRemoteUnregistration(Context context)
    {
        handleRemoteRegistration(context, null);
    }
    
    public static void handleRemoteMessage(Context context, String body, String title, String user_info, boolean play_sound, int badge_value)
    {
        long t_row_id;
        t_row_id = insertNotification(context, NOTIFY_TYPE_REMOTE, System.currentTimeMillis(), body, title, user_info, play_sound, badge_value);
        resetTimer(context);
    }

////////////////////////////////////////////////////////////////////////////////

    public static native boolean doReturnNotificationDetails(String body, String action, String info, long time, boolean play_sound, int badge_value);
    public static native void doLocalNotification(String p_message);
    
    public static native void doRemoteNotification(String p_message);
    public static native void doRemoteRegistrationError(String p_error);
    public static native void doRemoteRegistration(String p_registration_id);
}
