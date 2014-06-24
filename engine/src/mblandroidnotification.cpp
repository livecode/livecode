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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "execpt.h"
#include "globals.h"
#include "stack.h"

#include "exec.h"
#include "mblsyntax.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblnotification.h"

////////////////////////////////////////////////////////////////////////////////

bool MCSystemCreateLocalNotification (const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id)
{
    int64_t t_id = -1;
    int32_t t_seconds;
   MCExecPoint ep(nil, nil, nil);
    
    MCD_convert_from_datetime(ep, CF_SECONDS, CF_UNDEFINED, p_date);
    t_seconds = ep.getint4();

    MCAndroidEngineRemoteCall("createLocalNotification", "jsssibi", &t_id, p_alert_body, p_alert_action, p_user_info, t_seconds, p_play_sound, p_badge_value);
    r_id = (int32_t) t_id;
    return t_id >= 0;
}

bool MCSystemGetRegisteredNotifications (char *&r_registered_alerts)
{
    char *t_notifications = nil;
    
    MCAndroidEngineRemoteCall("getRegisteredNotifications", "s", &t_notifications);
    
    if (t_notifications != nil)
    {
//        MCLog("got list of notifications: %s", t_notifications);
        r_registered_alerts = t_notifications;
        return true;
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
        s_notification->body = t_body;
        s_notification->action = t_action;
        s_notification->user_info = t_user_info;
        s_notification->time = (uint32_t)(time / 1000);
        s_notification->play_sound = play_sound;
        s_notification->badge_value = badge_value;
    }
    else
    {
        MCCStringFree(t_body);
        MCCStringFree(t_action);
        MCCStringFree(t_user_info);
    }
    
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

bool MCSystemGetDeviceToken (char *&r_device_token)
{
    bool t_success = true;
    char *t_registration_id = nil;
    
    MCAndroidEngineRemoteCall("getRemoteNotificationId", "s", &t_registration_id);
    t_success = t_registration_id != nil;
    
    if (t_success)
        r_device_token = t_registration_id;
    
    return t_success;
}

bool MCSystemGetLaunchUrl (char *&r_launch_url)
{
    bool t_success = true;
    char *t_launch_url = nil;
    
    MCAndroidEngineRemoteCall("getLaunchUri", "s", &t_launch_url);
    t_success = t_launch_url != nil;
    
    if (t_success)
        r_launch_url = t_launch_url;
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doLocalNotification(JNIEnv *env, jobject object, jstring user_info) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doLocalNotification(JNIEnv *env, jobject object, jstring user_info)
{
    char *t_user_info = nil;
    if (MCJavaStringToNative(env, user_info, t_user_info))
        MCNotificationPostLocalNotificationEvent(MCString(t_user_info));
    MCCStringFree(t_user_info);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteNotification(JNIEnv *env, jobject object, jstring user_info) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteNotification(JNIEnv *env, jobject object, jstring user_info)
{
    char *t_user_info = nil;
    if (MCJavaStringToNative(env, user_info, t_user_info))
        MCNotificationPostPushNotificationEvent(MCString(t_user_info));
    MCCStringFree(t_user_info);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistrationError(JNIEnv *env, jobject object, jstring error) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistrationError(JNIEnv *env, jobject object, jstring error)
{
    char *t_error = nil;
    if (MCJavaStringToNative(env, error, t_error))
        MCNotificationPostPushRegistrationError(MCString(t_error));
    MCCStringFree(t_error);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistration(JNIEnv *env, jobject object, jstring registration_id) __attribute__((visibility("default")));
JNIEXPORT jboolean JNICALL Java_com_runrev_android_NotificationModule_doRemoteRegistration(JNIEnv *env, jobject object, jstring registration_id)
{
    char *t_registration_id = nil;
    if (MCJavaStringToNative(env, registration_id, t_registration_id))
        MCNotificationPostPushRegistered(MCString(t_registration_id));
    MCCStringFree(t_registration_id);
}
