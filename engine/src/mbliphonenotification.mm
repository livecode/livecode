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

#include "exec.h"
#include "mblsyntax.h"

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>

#include "mbliphoneapp.h"
#include "mblnotification.h"

bool MCSystemCreateLocalNotification (MCStringRef p_alert_body, MCStringRef p_alert_action, MCStringRef p_user_info, MCDateTime p_date, bool p_play_sound, int32_t p_badge_value, int32_t &r_id)
{
	MCExecContext ctxt(nil, nil, nil);
    int t_message_id = 1;
	Class t_cls = NSClassFromString (@"UILocalNotification");
    if (t_cls == nil)
		return false;
    // Check if we have to change the message id, based on the presence of pending local notifications.
    NSArray *t_scheduled_local_notifications = [[UIApplication sharedApplication] scheduledLocalNotifications];
    for (NSUInteger i = 0; i < [t_scheduled_local_notifications count]; i++)
    {
        UILocalNotification* t_local_notification = [t_scheduled_local_notifications objectAtIndex:i];
        if (t_message_id <= atoi ([[t_local_notification.userInfo objectForKey:@"notificationId"] cStringUsingEncoding:NSMacOSRomanStringEncoding]))
            t_message_id =  atoi ([[t_local_notification.userInfo objectForKey:@"notificationId"] cStringUsingEncoding:NSMacOSRomanStringEncoding]) + 1;
    }
    // Build the notification.	
	UILocalNotification *t_local_notification = [[t_cls alloc] init];
    if (p_alert_body == nil || MCStringGetLength(p_alert_body) == 0)
        t_local_notification.alertBody = nil;
    else
        t_local_notification.alertBody = MCStringConvertToAutoreleasedNSString(p_alert_body);
    if (p_alert_action == nil || MCStringGetLength(p_alert_action) == 0)
    {
        t_local_notification.alertAction = nil;
        t_local_notification.hasAction = NO;
    }
    else
    {
        t_local_notification.alertAction = MCStringConvertToAutoreleasedNSString(p_alert_action);
        t_local_notification.hasAction = YES;
    }
    // Create the dictionary.
    NSDictionary *t_dictionary;
    if (MCStringGetLength(p_user_info) > 0)
        t_dictionary = [NSDictionary dictionaryWithObjectsAndKeys:MCStringConvertToAutoreleasedNSString(p_user_info),@"payload",[NSString stringWithFormat:@"%i", t_message_id],@"notificationId", nil];
    else
        t_dictionary = [NSDictionary dictionaryWithObject:[NSString stringWithFormat:@"%i", t_message_id] forKey: @"notificationId"];
    t_local_notification.userInfo = t_dictionary;
    if (p_play_sound)
    {
        t_local_notification.soundName = UILocalNotificationDefaultSoundName;
    }
    t_local_notification.applicationIconBadgeNumber = p_badge_value;
    // Convert the current datatime to an NSDate
	MCAutoValueRef t_date;
    if (!MCD_convert_from_datetime(ctxt, p_date, CF_SECONDS, CF_SECONDS, &t_date))
        return false;
	integer_t t_date_secs;
	if (!ctxt.ConvertToInteger(*t_date, t_date_secs))
		return false;
    t_local_notification.fireDate = [NSDate dateWithTimeIntervalSince1970:t_date_secs];
	[[UIApplication sharedApplication] scheduleLocalNotification: t_local_notification];
	[t_local_notification release];
    r_id = t_message_id;
    return true;
}

bool MCSystemGetRegisteredNotifications (MCStringRef& r_registered_alerts)
{
    // Get the array of UILocalNotification entries
    NSString *t_result = nil;
    NSArray *t_scheduled_local_notifications = [[UIApplication sharedApplication] scheduledLocalNotifications];
    for (int i = 0; i < [t_scheduled_local_notifications count]; i++)
    {
        UILocalNotification* t_local_notification = [t_scheduled_local_notifications objectAtIndex:i];
        if (t_result == nil)
            t_result = [t_local_notification.userInfo objectForKey:@"notificationId"];
        else
            t_result = [t_result stringByAppendingFormat:@",%@", [t_local_notification.userInfo objectForKey:@"notificationId"]];
    }
    if (t_result == nil)
        r_registered_alerts = MCValueRetain(kMCEmptyString);
    else
        return MCStringCreateWithCFStringRef((CFStringRef)t_result, r_registered_alerts);
    return true;
}

bool MCSystemGetNotificationDetails(int32_t p_id, MCNotification &r_notification)
{
    NSArray *t_scheduled_local_notifications = [[UIApplication sharedApplication] scheduledLocalNotifications];
    for (int i = 0; i < [t_scheduled_local_notifications count]; i++)
    {
        UILocalNotification* t_local_notification = [t_scheduled_local_notifications objectAtIndex:i];
        if (atoi ([[t_local_notification.userInfo objectForKey:@"notificationId"] cStringUsingEncoding:NSMacOSRomanStringEncoding]) == p_id) 
        {
			if ([t_local_notification alertBody] == nil)
				r_notification.body = MCValueRetain(kMCEmptyString);
			else
				MCStringCreateWithCFStringRef ((CFStringRef)[t_local_notification alertBody], r_notification.body); // or ""
			
			if ([t_local_notification alertAction] == nil)
				r_notification.action = MCValueRetain(kMCEmptyString);
			else
				MCStringCreateWithCFStringRef ((CFStringRef)[t_local_notification alertAction], r_notification.action); // or ""
			
			if ([t_local_notification.userInfo objectForKey:@"payload"] == nil)
				r_notification.user_info = MCValueRetain(kMCEmptyString);
			else
				MCStringCreateWithCFStringRef ((CFStringRef)[t_local_notification.userInfo objectForKey:@"payload"], r_notification.user_info);
            r_notification.time = [[t_local_notification fireDate] timeIntervalSince1970];
            r_notification.badge_value = t_local_notification.applicationIconBadgeNumber;
            r_notification.play_sound = t_local_notification.soundName != nil ? true : false;
            return true;
        }
    }
    MCMemoryClear(&r_notification, sizeof(MCNotification));
    return false;
}

bool MCSystemCancelLocalNotification(uint32_t p_alert_descriptor) 
{
    // Cancel a UILocalNotification object
    bool t_result = false;
    NSArray *t_scheduled_local_notifications = [[UIApplication sharedApplication] scheduledLocalNotifications];
    for (int i = 0; i < [t_scheduled_local_notifications count]; i++)
    {
        UILocalNotification* t_local_notification = [t_scheduled_local_notifications objectAtIndex:i];
        if (p_alert_descriptor == atoi ([[t_local_notification.userInfo objectForKey:@"notificationId"] cStringUsingEncoding:NSMacOSRomanStringEncoding]))
        {
            [[UIApplication sharedApplication] cancelLocalNotification:t_local_notification];
            t_result = true;
        }
    }
    return t_result;
}

bool MCSystemCancelAllLocalNotifications ()
{
    [[UIApplication sharedApplication] cancelAllLocalNotifications];
    return true;
}

bool MCSystemGetNotificationBadgeValue (uint32_t &r_badge_value)
{
    r_badge_value = [[UIApplication sharedApplication] applicationIconBadgeNumber];
    return true;
}

bool MCSystemSetNotificationBadgeValue (uint32_t r_badge_value)
{
    [UIApplication sharedApplication].applicationIconBadgeNumber = r_badge_value;
    return true;
}

bool MCSystemGetDeviceToken (MCStringRef& r_device_token)
{
    r_device_token = MCValueRetain([MCIPhoneGetApplication() fetchDeviceToken]);
	return true;
}

bool MCSystemGetLaunchUrl (MCStringRef& r_launch_url)
{
	r_launch_url = MCValueRetain([MCIPhoneGetApplication() fetchLaunchUrl]);
	return true;
}
