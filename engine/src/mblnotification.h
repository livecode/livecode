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

void MCNotificationPostLocalNotificationEvent(MCStringRef p_payload);
void MCNotificationPostPushNotificationEvent(MCStringRef p_payload);
void MCNotificationPostPushRegistered (MCStringRef p_registration_text);
void MCNotificationPostPushRegistrationError (MCStringRef p_error_text);
void MCNotificationPostUrlWakeUp (MCStringRef p_url_wake_up_text);
void MCNotificationPostLaunchDataChanged();
void MCNotificationPostSystemAppearanceChanged();
bool MCNotificationPostCustom(MCNameRef p_message, uint32_t p_param_count, ...);

#endif
