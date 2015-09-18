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
#include "mcio.h"

#include "mcerror.h"
#include "globals.h"
#include "exec.h"
#include "param.h"

#include "mblnotification.h"
#include "mblsyntax.h"


////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(Notification, CreateLocalNotification, 6)
MC_EXEC_DEFINE_GET_METHOD(Notification, RegisteredNotification, 0)
MC_EXEC_DEFINE_GET_METHOD(Notification, Details, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Notification, CancelLocalNotification, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Notification, CancelAllNotifications, 0)
MC_EXEC_DEFINE_GET_METHOD(Notification, NotificationBadgeValue, 0)
MC_EXEC_DEFINE_SET_METHOD(Notification, NotificationBadgeValue, 1)

////////////////////////////////////////////////////////////////////////////////


void MCNotificationExecCreateLocalNotification(MCExecContext& ctxt, MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value)
{
#ifdef /* MCLocalNotificationExec */ LEGACY_EXEC
    bool t_success;
    int32_t t_id = -1;
    t_success = MCSystemCreateLocalNotification (p_alert_body, p_alert_action, p_user_info, p_date, p_play_sound, p_badge_value, t_id);
    if (t_success)
        p_ctxt.SetTheResultToNumber(t_id);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCLocalNotificationExec */
    bool t_success;
    int32_t t_id = -1;
    t_success = MCSystemCreateLocalNotification (p_alert_body, p_alert_action, p_user_info, p_date, p_play_sound, p_badge_value, t_id);
    
    if (t_success)
        ctxt.SetTheResultToNumber(t_id);
    else
        ctxt.SetTheResultToEmpty();
}

void MCNotificationGetRegisteredNotifications(MCExecContext& ctxt)
{
#ifdef /* MCGetRegisteredNotificationsExec */ LEGACY_EXEC
    char *t_registered_alerts = nil;
    bool t_success;
    t_success = MCSystemGetRegisteredNotifications (t_registered_alerts);
    if (t_success)
        p_ctxt.SetTheResultToCString(t_registered_alerts);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCGetRegisteredNotificationsExec */
    MCAutoStringRef t_registered_alerts;
    bool t_success;
    t_success = MCSystemGetRegisteredNotifications (&t_registered_alerts);
    
    if (t_success)
        ctxt.SetTheResultToValue(*t_registered_alerts);
    else
        ctxt.SetTheResultToEmpty();
}

void MCNotificationGetDetails(MCExecContext& ctxt, int32_t p_id, MCArrayRef& r_details)
{
#ifdef /* MCNotificationGetDetails */ LEGACY_EXEC
	 MCNotification t_notification;
     if (MCSystemGetNotificationDetails(p_id, t_notification))
     {
     MCVariableValue *t_details = nil;
     t_details = new MCVariableValue();
     
     MCVariableValue *t_element = nil;
     
     t_details->lookup_element(ctxt.GetEP(), "body", t_element);
     t_element->assign_string(t_notification.body);
     
     t_details->lookup_element(ctxt.GetEP(), "title", t_element);
     t_element->assign_string(t_notification.action);
     
     t_details->lookup_element(ctxt.GetEP(), "payload", t_element);
     t_element->assign_string(t_notification.user_info);
     
     t_details->lookup_element(ctxt.GetEP(), "time", t_element);
     t_element->assign_real(t_notification.time);
     
     t_details->lookup_element(ctxt.GetEP(), "play sound", t_element);
     t_element->assign_constant_string(MCU_btos(t_notification.play_sound));
     
     t_details->lookup_element(ctxt.GetEP(), "badge value", t_element);
     t_element->assign_real(t_notification.badge_value);
     
     r_details = t_details;
     }
     
     FreeNotification(t_notification);
#endif /* MCNotificationGetDetails */
    
    MCNotification t_notification;
    if (MCSystemGetNotificationDetails(p_id, t_notification))
    {
        MCAutoArrayRef t_details;
        /* UNCHECKED */ MCArrayCreateMutable(&t_details);
        
        MCNewAutoNameRef t_key_body;
        MCNameCreateWithCString("body", &t_key_body);        
        MCArrayStoreValue(*t_details, false, *t_key_body, t_notification.body);
        
        MCNewAutoNameRef t_key_title;
        MCNameCreateWithCString("title", &t_key_title);
        MCArrayStoreValue(*t_details, false, *t_key_title, t_notification.action);
        
        MCNewAutoNameRef t_key_payload;
        MCNameCreateWithCString("payload", &t_key_payload);
        MCArrayStoreValue(*t_details, false, *t_key_payload, t_notification.user_info);
        
        MCNewAutoNameRef t_key_time;
        MCNameCreateWithCString("time", &t_key_time);
        MCAutoNumberRef t_notification_time;
        MCNumberCreateWithUnsignedInteger(t_notification.time, &t_notification_time);
        MCArrayStoreValue(*t_details, false, *t_key_time, *t_notification_time);
        
        MCNewAutoNameRef t_key_badge_value;
        MCNameCreateWithCString("badge value", &t_key_badge_value);
        MCAutoNumberRef t_notification_badge_value;
        MCNumberCreateWithUnsignedInteger(t_notification.badge_value, &t_notification_badge_value);
        MCArrayStoreValue(*t_details, false, *t_key_badge_value, *t_notification_badge_value);
        
        MCNewAutoNameRef t_key_play_sound;
        MCNameCreateWithCString("play sound", &t_key_play_sound);
        if(t_notification.play_sound)
            MCArrayStoreValue(*t_details, false, *t_key_play_sound, kMCTrue);
        else
            MCArrayStoreValue(*t_details, false, *t_key_play_sound, kMCFalse);
        
        MCArrayCopy(*t_details, r_details);
        
        return;
    }

    ctxt.Throw();
}

void MCNotificationExecCancelLocalNotification(MCExecContext& ctxt, int32_t p_id)
{
#ifdef /* MCCancelLocalNotificationExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemCancelLocalNotification (p_id);
    p_ctxt.SetTheResultToEmpty();
#endif /* MCCancelLocalNotificationExec */
    bool t_success;
    t_success = MCSystemCancelLocalNotification (p_id);
    ctxt.SetTheResultToEmpty();
}

void MCNotificationExecCancelAllLocalNotifications(MCExecContext& ctxt)
{
#ifdef /* MCCancelAllLocalNotificationsExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemCancelAllLocalNotifications ();
    p_ctxt.SetTheResultToEmpty();
#endif /* MCCancelAllLocalNotificationsExec */
    bool t_success;
    t_success = MCSystemCancelAllLocalNotifications ();
    ctxt.SetTheResultToEmpty();
}

void MCNotificationGetNotificationBadgeValue(MCExecContext& ctxt)
{
#ifdef /* MCGetNotificationBadgeValueExec */ LEGACY_EXEC
    uint32_t r_badge_value = 0;
    bool t_success;
    t_success = MCSystemGetNotificationBadgeValue (r_badge_value);
    if (t_success)
        p_ctxt.SetTheResultToNumber(r_badge_value);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCGetNotificationBadgeValueExec */
    uint32_t r_badge_value = 0;
    bool t_success;
    t_success = MCSystemGetNotificationBadgeValue (r_badge_value);
    if (t_success)
        ctxt.SetTheResultToNumber(r_badge_value);
    else
        ctxt.SetTheResultToEmpty();
}

void MCNotificationSetNotificationBadgeValue(MCExecContext& ctxt, uint32_t p_badge_value)
{
#ifdef /* MCSetNotificationBadgeValueExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemSetNotificationBadgeValue (p_badge_value);
    p_ctxt.SetTheResultToEmpty();
#endif /* MCSetNotificationBadgeValueExec */
    bool t_success;
    t_success = MCSystemSetNotificationBadgeValue (p_badge_value);
    ctxt.SetTheResultToEmpty();
}
