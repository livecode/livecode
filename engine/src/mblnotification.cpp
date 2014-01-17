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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "param.h"
#include "exec.h"

#include "eventqueue.h"

#include "mblsyntax.h"
#include "mblnotification.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
bool MCSystemCreateLocalNotification (const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id);
bool MCSystemGetRegisteredNotifications (char *&r_registered_alerts);
bool MCSystemGetNotificationDetails(int32_t p_id, MCNotification &r_notification);
bool MCSystemCancelLocalNotification(uint32_t p_alert_descriptor); 
bool MCSystemCancelAllLocalNotifications ();
bool MCSystemGetNotificationBadgeValue (uint32_t &r_badge_value);
bool MCSystemSetNotificationBadgeValue (uint32_t r_badge_value);
bool MCSystemGetDeviceToken (char *&r_device_token);
bool MCSystemGetLaunchUrl (char *&r_launch_url);

////////////////////////////////////////////////////////////////////////////////

void FreeNotification(MCNotification &p_notification)
{
    MCCStringFree(p_notification.body);
    MCCStringFree(p_notification.action);
    MCCStringFree(p_notification.user_info);
}

////////////////////////////////////////////////////////////////////////////////


void MCLocalNotificationExec (MCExecContext& p_ctxt, const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value)
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
}

void MCGetRegisteredNotificationsExec (MCExecContext& p_ctxt) 
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
}

void MCNotificationGetDetails(MCExecContext &ctxt, int32_t p_id, MCVariableValue *&r_details)
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
}

void MCCancelLocalNotificationExec (MCExecContext& p_ctxt, int32_t p_id) 
{
#ifdef /* MCCancelLocalNotificationExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemCancelLocalNotification (p_id);
    p_ctxt.SetTheResultToEmpty();
#endif /* MCCancelLocalNotificationExec */
}

void MCCancelAllLocalNotificationsExec (MCExecContext& p_ctxt)
{
#ifdef /* MCCancelAllLocalNotificationsExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemCancelAllLocalNotifications ();
    p_ctxt.SetTheResultToEmpty();
#endif /* MCCancelAllLocalNotificationsExec */
}

void MCGetNotificationBadgeValueExec (MCExecContext& p_ctxt)
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
}

void MCSetNotificationBadgeValueExec (MCExecContext& p_ctxt, uint32_t p_badge_value)
{ 
#ifdef /* MCSetNotificationBadgeValueExec */ LEGACY_EXEC
    bool t_success;
    t_success = MCSystemSetNotificationBadgeValue (p_badge_value);
    p_ctxt.SetTheResultToEmpty();
#endif /* MCSetNotificationBadgeValueExec */
}

void MCGetDeviceTokenExec (MCExecContext& p_ctxt)
{
#ifdef /* MCGetDeviceTokenExec */ LEGACY_EXEC
    char *r_device_token = nil;
    bool t_success;
    t_success = MCSystemGetDeviceToken (r_device_token);
    if (t_success)
        p_ctxt.GiveCStringToResult(r_device_token);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCGetDeviceTokenExec */
}

void MCGetLaunchUrlExec (MCExecContext& p_ctxt)
{
#ifdef /* MCGetLaunchUrlExec */ LEGACY_EXEC
    char *t_launch_url = nil;
    bool t_success;
    t_success = MCSystemGetLaunchUrl (t_launch_url);
    if (t_success)
        p_ctxt.GiveCStringToResult(t_launch_url);
    else
        p_ctxt.SetTheResultToEmpty();
#endif /* MCGetLaunchUrlExec */
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandleCreateLocalNotification (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCreateLocalNotification */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    
    bool t_success = true;
    char *t_notification_body = nil;
    char *t_notification_action = nil;
    char *t_notification_user_info = nil;
    MCDateTime t_date;
    bool t_play_sound_vibrate = true;
    int32_t t_badge_value = 0;
    
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
     
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "sss", &t_notification_body, &t_notification_action, &t_notification_user_info);
	if (t_success && p_parameters != nil)
    {
        p_parameters->eval(ep);
        if (!ep.isempty())
        {
            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_date);
        }
        p_parameters = p_parameters->getnext();
    }
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "b", &t_play_sound_vibrate);
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters(p_parameters, "u", &t_badge_value);
    
	MCLocalNotificationExec (t_ctxt, t_notification_body, t_notification_action, t_notification_user_info, t_date, t_play_sound_vibrate, t_badge_value);
    
    return ES_NORMAL;
#endif /* MCHandleCreateLocalNotification */
}

Exec_stat MCHandleGetRegisteredNotifications(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetRegisteredNotifications */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetRegisteredNotificationsExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetRegisteredNotifications */
}

Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetNotificationDetails */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext ctxt(ep);
    
    bool t_success = true;
    
    int32_t t_id = -1;
    MCVariableValue *t_details = nil;
    
    if (t_success)
        t_success = MCParseParameters(p_parameters, "i", &t_id);
    
    ctxt.SetTheResultToEmpty();
    if (t_success)
    {
        MCNotificationGetDetails(ctxt, t_id, t_details);
        if (t_details != nil)
        {
            ep.setarray(t_details, True);
            MCresult->store(ep, False);
        }
    }
    
    return ES_NORMAL;
#endif /* MCHandleGetNotificationDetails */
}

Exec_stat MCHandleCancelLocalNotification(void *context, MCParameter *p_parameters) 
{
#ifdef /* MCHandleCancelLocalNotification */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    int32_t t_cancel_this;
    bool t_success;
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    if (p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
    MCCancelLocalNotificationExec (t_ctxt, t_cancel_this);
    
    return ES_NORMAL;
#endif /* MCHandleCancelLocalNotification */
}

Exec_stat MCHandleCancelAllLocalNotifications (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleCancelAllLocalNotifications */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCCancelAllLocalNotificationsExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleCancelAllLocalNotifications */
}

Exec_stat MCHandleGetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetNotificationBadgeValue */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetNotificationBadgeValueExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetNotificationBadgeValue */
}

Exec_stat MCHandleSetNotificationBadgeValue (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleSetNotificationBadgeValue */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    uint32_t t_badge_value;
    bool t_success = true;
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    if (t_success && p_parameters != nil)
		t_success = MCParseParameters (p_parameters, "i", &t_badge_value);
    if (t_success)
        MCSetNotificationBadgeValueExec (t_ctxt, t_badge_value);
    
    return ES_NORMAL;
#endif /* MCHandleSetNotificationBadgeValue */
}

Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetDeviceToken */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetDeviceTokenExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetDeviceToken */
}

Exec_stat MCHandleGetLaunchUrl (void *context, MCParameter *p_parameters)
{
#ifdef /* MCHandleGetLaunchUrl */ LEGACY_EXEC
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
    t_ctxt.SetTheResultToEmpty();
    MCGetLaunchUrlExec (t_ctxt);
    
    return ES_NORMAL;
#endif /* MCHandleGetLaunchUrl */
}

////////////////////////////////////////////////////////////////////////////////

class MCNotificationEvent: public MCCustomEvent
{
private:
    MCNameRef m_message;
	char *m_notification;
    uint32_t m_notification_length;
    
public:
	MCNotificationEvent (MCNameRef p_message, const MCString &p_notification)
	{
        m_message = p_message;
        char *t_notification = nil;
        MCMemoryAllocateCopy(p_notification.getstring(), p_notification.getlength(), t_notification);
        m_notification = t_notification;
        m_notification_length = p_notification.getlength();
	}
	
	void Destroy(void)
	{
        MCMemoryDeallocate(m_notification);
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message_with_args (m_message, MCString(m_notification, m_notification_length));
	}
};

void MCNotificationPostLocalNotificationEvent(MCString pPayload)
{
	MCEventQueuePostCustom(new MCNotificationEvent(MCM_local_notification_received, pPayload));
}

void MCNotificationPostPushNotificationEvent(MCString p_payload)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_received, p_payload));
}

void MCNotificationPostPushRegistered (MCString p_registration_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_registered, p_registration_text));
}

void MCNotificationPostPushRegistrationError (MCString p_error_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_registration_error, p_error_text));
}

void MCNotificationPostUrlWakeUp (MCString p_url_wake_up_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_url_wake_up, p_url_wake_up_text));
}
