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

void MCNotificationExecCreateLocalNotification(MCExecContext& ctxt, MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value)
{
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
    
    MCNotification t_notification;
    if (MCSystemGetNotificationDetails(p_id, t_notification))
    {
        MCAutoArrayRef t_details;
        /* UNCHECKED */ MCArrayCreateMutable(&t_details);
        
        MCArrayStoreValue(*t_details, false, MCNAME("body"), t_notification.body);
        MCArrayStoreValue(*t_details, false, MCNAME("title"), t_notification.action);
        MCArrayStoreValue(*t_details, false, MCNAME("payload"), t_notification.user_info);
        
        MCAutoNumberRef t_notification_time;
        MCNumberCreateWithUnsignedInteger(t_notification.time, &t_notification_time);
        MCArrayStoreValue(*t_details, false, MCNAME("time"), *t_notification_time);
        
        MCAutoNumberRef t_notification_badge_value;
        MCNumberCreateWithUnsignedInteger(t_notification.badge_value, &t_notification_badge_value);
        MCArrayStoreValue(*t_details, false, MCNAME("badge value"), *t_notification_badge_value);
        
        MCArrayStoreValue(*t_details, false, MCNAME("play sound"), t_notification.play_sound ? kMCTrue : kMCFalse);
        
        MCArrayCopy(*t_details, r_details);
        
        return;
    }

    ctxt.Throw();
}

void MCNotificationExecCancelLocalNotification(MCExecContext& ctxt, int32_t p_id)
{
    bool t_success;
    t_success = MCSystemCancelLocalNotification (p_id);
    ctxt.SetTheResultToEmpty();
}

void MCNotificationExecCancelAllLocalNotifications(MCExecContext& ctxt)
{
    bool t_success;
    t_success = MCSystemCancelAllLocalNotifications ();
    ctxt.SetTheResultToEmpty();
}

void MCNotificationGetNotificationBadgeValue(MCExecContext& ctxt)
{
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
    bool t_success;
    t_success = MCSystemSetNotificationBadgeValue (p_badge_value);
    ctxt.SetTheResultToEmpty();
}
