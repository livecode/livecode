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

#ifndef __MOBILE_NOTIFICATION__
#define __MOBILE_NOTIFICATION__

#include "mblsyntax.h"

typedef struct
{
    char *body;
    char *action;
    char *user_info;
    uint32_t time; // in seconds
    uint32_t badge_value;
    bool play_sound;
} MCNotification;

void MCNotificationPostLocalNotificationEvent(MCString pPayload);
void MCNotificationPostPushNotificationEvent(MCString p_payload);
void MCNotificationPostPushRegistered (MCString p_registration_text);
void MCNotificationPostPushRegistrationError (MCString p_error_text);
void MCNotificationPostUrlWakeUp (MCString p_url_wake_up_text);

#endif
