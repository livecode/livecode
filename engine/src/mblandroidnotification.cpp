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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"

#include "globals.h"
#include "stack.h"

#include "exec.h"
#include "mblsyntax.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblnotification.h"

////////////////////////////////////////////////////////////////////////////////

bool MCSystemCreateLocalNotification (MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id)
{
    int64_t t_id = -1;
    int32_t t_seconds;
	MCExecContext ctxt(nil, nil, nil);
    
    MCAutoValueRef t_val;
	/* UNCHECKED */ MCD_convert_from_datetime(ctxt, p_date, CF_SECONDS, CF_UNDEFINED, &t_val);
    /* UNCHECKED */ ctxt.ConvertToInteger(*t_val, t_seconds);

    MCAndroidEngineRemoteCall("createLocalNotification", "jxxxibi", &t_id, p_alert_body, p_alert_action, p_user_info, t_seconds, p_play_sound, p_badge_value);
    r_id = (int32_t) t_id;
    return t_id >= 0;
}

bool MCSystemGetRegisteredNotifications (MCStringRef &r_registered_alerts)
{
    char *t_notifications = nil;
    
    MCAndroidEngineRemoteCall("getRegisteredNotifications", "s", &t_notifications);
    
    if (t_notifications != nil)
    {
//        MCLog("got list of notifications: %s", t_notifications);
        return MCStringCreateWithCString(t_notifications, r_registered_alerts);
    }
    else
        return false;
}

static MCNotification *s_notification = nil;
bool MCSystemGetNotificationDetails(int32_t p_id, MCNotification &r_notification)
{
    s_notification = &r_notification;
    bool t_success = true;
    
    MCAndroidEngineRemoteCall("getNotificationDetails", "bj", &t_success, (int64_t)p_id);
    return t_success;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doReturnNotificationDetails(JNIEnv *env, jobject object, jstring body, jstring action, jstring user_info, jlong time, jboolean play_sound, jint badge_value) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doReturnNotificationDetails(JNIEnv *env, jobject object, jstring body, jstring action, jstring user_info, jlong time, jboolean play_sound, jint badge_value)
{
    bool t_success = true;
    
    char *t_body = nil;
    char *t_action = nil;
    char *t_user_info = nil;
    
    t_success = MCJavaStringToNative(env, body, t_body) && MCJavaStringToNative(env, action, t_action) && MCJavaStringToNative(env, user_info, t_user_info);
    
    if (t_success)
    {
//        MCLog("doReturnNotificationDetails(%s, %s, %s, %lld, %d, %d", t_body, t_action, t_user_info, time, play_sound, badge_value);
        t_success = MCStringCreateWithCString(t_body, s_notification->body);
        t_success |= MCStringCreateWithCString(t_action, s_notification->action);
        t_success |= MCStringCreateWithCString(t_user_info, s_notification->user_info);
        s_notification->time = (uint32_t)(time / 1000);
        s_notification->play_sound = play_sound;
        s_notification->badge_value = badge_value;
    }
    
    MCCStringFree(t_body);
    MCCStringFree(t_action);
    MCCStringFree(t_user_info);
    
    return t_success;
}


bool MCSystemCancelLocalNotification(uint32_t p_alert_descriptor)
{
    bool t_success = true;
    MCAndroidEngineRemoteCall("cancelLocalNotification", "bj", &t_success, (int64_t)p_alert_descriptor);
    
    return t_success;
}

bool MCSystemCancelAllLocalNotifications()
{
    bool t_success = true;
    MCAndroidEngineRemoteCall("cancelAllLocalNotifications", "b", &t_success);
    
    return t_success;
}

bool MCSystemGetNotificationBadgeValue (uint32_t &r_badge_value)
{
    return false;
}

bool MCSystemSetNotificationBadgeValue (uint32_t p_badge_value)
{
    return false;
}

bool MCSystemGetDeviceToken (MCStringRef& r_device_token)
{
    bool t_success = true;
    MCAutoStringRef t_registration_id;
    
    MCAndroidEngineRemoteCall("getRemoteNotificationId", "x", &(&t_registration_id));
    
    t_success = *t_registration_id != nil;
    
    if (t_success)
        r_device_token = MCValueRetain(*t_registration_id);
    
    return t_success;
}

bool MCSystemGetLaunchUrl (MCStringRef& r_launch_url)
{
    bool t_success = true;
    char *t_launch_url = nil;
    
    MCAndroidEngineRemoteCall("getLaunchUri", "s", &t_launch_url);
    t_success = t_launch_url != nil;
    
    if (t_success)
        t_success = MCStringCreateWithCString(t_launch_url, r_launch_url);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doLocalNotification(JNIEnv *env, jobject object, jstring user_info) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doLocalNotification(JNIEnv *env, jobject object, jstring user_info)
{
    MCAutoStringRef t_user_info;
    if (!MCJavaStringToStringRef(env, user_info, &t_user_info))
        return false;

    MCNotificationPostLocalNotificationEvent(*t_user_info);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteNotification(JNIEnv *env, jobject object, jstring user_info) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteNotification(JNIEnv *env, jobject object, jstring user_info)
{
    MCAutoStringRef t_user_info;
    if (!MCJavaStringToStringRef(env, user_info, &t_user_info))
        return false;

    MCNotificationPostPushNotificationEvent(*t_user_info);
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistrationError(JNIEnv *env, jobject object, jstring error) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistrationError(JNIEnv *env, jobject object, jstring error)
{
	MCAutoStringRef t_error_str;
    if (!MCJavaStringToStringRef(env, error, &t_error_str))
        return false;

    MCNotificationPostPushRegistrationError(*t_error_str);
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistration(JNIEnv *env, jobject object, jstring registration_id) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistration(JNIEnv *env, jobject object, jstring registration_id)
{
	MCAutoStringRef t_id_str;
    if (!MCJavaStringToStringRef(env, registration_id, &t_id_str))
        return false;

    MCNotificationPostPushRegistered(*t_id_str);
    return true;
}
