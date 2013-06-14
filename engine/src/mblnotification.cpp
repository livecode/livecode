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

//bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
//bool MCSystemCreateLocalNotification (const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id);
//bool MCSystemGetRegisteredNotifications (char *&r_registered_alerts);
//bool MCSystemGetNotificationDetails(int32_t p_id, MCNotification &r_notification);
//bool MCSystemCancelLocalNotification(uint32_t p_alert_descriptor); 
//bool MCSystemCancelAllLocalNotifications ();
//bool MCSystemGetNotificationBadgeValue (uint32_t &r_badge_value);
//bool MCSystemSetNotificationBadgeValue (uint32_t r_badge_value);
//bool MCSystemGetDeviceToken (char *&r_device_token);
//bool MCSystemGetLaunchUrl (char *&r_launch_url);

////////////////////////////////////////////////////////////////////////////////

void FreeNotification(MCNotification &p_notification)
{
    MCValueRelease(p_notification.body);
    MCValueRelease(p_notification.action);
    MCValueRelease(p_notification.user_info);
}

////////////////////////////////////////////////////////////////////////////////

//// MOVED TO exec-notofication.cpp
//void MCLocalNotificationExec (MCExecContext& p_ctxt, const char *p_alert_body, const char *p_alert_action, const char *p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value)
//{
//    bool t_success;
//    int32_t t_id = -1;
//    t_success = MCSystemCreateLocalNotification (p_alert_body, p_alert_action, p_user_info, p_date, p_play_sound, p_badge_value, t_id);
//    if (t_success)
//        p_ctxt.SetTheResultToNumber(t_id);
//    else
//        p_ctxt.SetTheResultToEmpty();
//}
//
//void MCGetRegisteredNotificationsExec (MCExecContext& p_ctxt) 
//{
//    char *t_registered_alerts = nil;
//    bool t_success;
//    t_success = MCSystemGetRegisteredNotifications (t_registered_alerts);
//    if (t_success)
//        p_ctxt.SetTheResultToCString(t_registered_alerts);
//    else
//        p_ctxt.SetTheResultToEmpty();
//}

//void MCNotificationGetDetails(MCExecContext &ctxt, int32_t p_id, MCArrayRef &r_details)
//{
// /*   MCNotification t_notification;
//    if (MCSystemGetNotificationDetails(p_id, t_notification))
//    {
//        MCVariableValue *t_details = nil;
//        t_details = new MCVariableValue();
//        
//        MCVariableValue *t_element = nil;
//        
//        t_details->lookup_element(ctxt.GetEP(), "body", t_element);
//        t_element->assign_string(t_notification.body);
//        
//        t_details->lookup_element(ctxt.GetEP(), "title", t_element);
//        t_element->assign_string(t_notification.action);
//        
//        t_details->lookup_element(ctxt.GetEP(), "payload", t_element);
//        t_element->assign_string(t_notification.user_info);
//        
//        t_details->lookup_element(ctxt.GetEP(), "time", t_element);
//        t_element->assign_real(t_notification.time);
//        
//        t_details->lookup_element(ctxt.GetEP(), "play sound", t_element);
//        t_element->assign_constant_string(MCU_btos(t_notification.play_sound));
//        
//        t_details->lookup_element(ctxt.GetEP(), "badge value", t_element);
//        t_element->assign_real(t_notification.badge_value);
//        
//        r_details = t_details;
//    }
//    
//    FreeNotification(t_notification);*/
//}

//void MCCancelLocalNotificationExec (MCExecContext& p_ctxt, int32_t p_id)
//{
//    bool t_success;
//    t_success = MCSystemCancelLocalNotification (p_id);
//    p_ctxt.SetTheResultToEmpty();
//}

//void MCCancelAllLocalNotificationsExec (MCExecContext& p_ctxt)
//{
//    bool t_success;
//    t_success = MCSystemCancelAllLocalNotifications ();
//    p_ctxt.SetTheResultToEmpty();
//}

//void MCGetNotificationBadgeValueExec (MCExecContext& p_ctxt)
//{
//    uint32_t r_badge_value = 0;
//    bool t_success;
//    t_success = MCSystemGetNotificationBadgeValue (r_badge_value);
//    if (t_success)
//        p_ctxt.SetTheResultToNumber(r_badge_value);
//    else
//        p_ctxt.SetTheResultToEmpty();
//}

//void MCSetNotificationBadgeValueExec (MCExecContext& p_ctxt, uint32_t p_badge_value)
//{ 
//    bool t_success;
//    t_success = MCSystemSetNotificationBadgeValue (p_badge_value);
//    p_ctxt.SetTheResultToEmpty();
//}

//void MCGetDeviceTokenExec (MCExecContext& p_ctxt)
//{
//    char *r_device_token = nil;
//    bool t_success;
//    t_success = MCSystemGetDeviceToken (r_device_token);
//    if (t_success)
//        p_ctxt.GiveCStringToResult(r_device_token);
//    else
//        p_ctxt.SetTheResultToEmpty();
//}

//void MCGetLaunchUrlExec (MCExecContext& p_ctxt)
//{
//    char *t_launch_url = nil;
//    bool t_success;
//    t_success = MCSystemGetLaunchUrl (t_launch_url);
//    if (t_success)
//        p_ctxt.GiveCStringToResult(t_launch_url);
//    else
//        p_ctxt.SetTheResultToEmpty();
//}

////////////////////////////////////////////////////////////////////////////////

//// MOVED TO mblhandlers.cpp
//Exec_stat MCHandleCreateLocalNotification (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    
//    bool t_success = true;
//    MCAutoStringRef t_notification_body;
//    MCAutoStringRef t_notification_action;
//    MCAutoStringRef t_notification_user_info;
//    MCDateTime t_date;
//    bool t_play_sound_vibrate = true;
//    int32_t t_badge_value = 0;
//    
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//     
//    if (t_success && p_parameters != nil)
//		t_success = MCParseParameters (p_parameters, "xxx", &t_notification_body, &t_notification_action, &t_notification_user_info);
//	if (t_success && p_parameters != nil)
//    {
//        p_parameters->eval(ep);
//        if (!ep.isempty())
//        {
//            t_success = MCD_convert_to_datetime(ep, CF_UNDEFINED, CF_UNDEFINED, t_date);
//        }
//        p_parameters = p_parameters->getnext();
//    }
//    if (t_success && p_parameters != nil)
//		t_success = MCParseParameters(p_parameters, "b", &t_play_sound_vibrate);
//    if (t_success && p_parameters != nil)
//		t_success = MCParseParameters(p_parameters, "u", &t_badge_value);
//    
//	MCNotificationExecCreateLocalNotification (ctxt, *t_notification_body, *t_notification_action, *t_notification_user_info, t_date, t_play_sound_vibrate, t_badge_value);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleGetRegisteredNotifications(void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//
//    MCNotificationGetRegisteredNotifications(ctxt);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleGetNotificationDetails(void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    
//    bool t_success = true;
//    
//    int32_t t_id = -1;
//    MCAutoArrayRef t_details;
//    
//    if (t_success)
//        t_success = MCParseParameters(p_parameters, "i", &t_id);
//    
//    ctxt.SetTheResultToEmpty();
//    if (t_success)
//    {
//        MCNotificationGetDetails(ctxt, t_id, &t_details);
//        if (*t_details != nil)
//        {
//            ep.setvalueref(*t_details);
//			ctxt . SetTheResultToValue(*t_details);
//            return ES_NORMAL;
//        }
//    }
//    return ES_ERROR;
//}

//Exec_stat MCHandleCancelLocalNotification(void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    int32_t t_cancel_this;
//    bool t_success;
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    if (p_parameters != nil)
//		t_success = MCParseParameters (p_parameters, "i", &t_cancel_this);
//    
//    if (t_success)
//    {
//        MCNotificationExecCancelLocalNotification (ctxt, t_cancel_this);
//    }
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleCancelAllLocalNotifications (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    
//    MCNotificationExecCancelAllLocalNotifications(ctxt);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleGetNotificationBadgeValue (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    MCNotificationGetNotificationBadgeValue (ctxt);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleSetNotificationBadgeValue (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    uint32_t t_badge_value;
//    bool t_success = true;
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    
//    if (t_success && p_parameters != nil)
//		t_success = MCParseParameters (p_parameters, "i", &t_badge_value);
//    
//    if (t_success)
//        MCNotificationSetNotificationBadgeValue (ctxt, t_badge_value);
//    
//    if (t_success && !ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleGetDeviceToken (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    MCNotificationGetDeviceToken (ctxt);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

//Exec_stat MCHandleGetLaunchUrl (void *context, MCParameter *p_parameters)
//{
//    MCExecPoint ep(nil, nil, nil);
//    MCExecContext ctxt(ep);
//    ctxt.SetTheResultToEmpty();
//    MCNotificationGetLaunchUrl (ctxt);
//    
//    if (!ctxt.HasError())
//        return ES_NORMAL;
//    
//    return ES_ERROR;
//}

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
