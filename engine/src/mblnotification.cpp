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

void FreeNotification(MCNotification &p_notification)
{
    MCValueRelease(p_notification.body);
    MCValueRelease(p_notification.action);
    MCValueRelease(p_notification.user_info);
}

////////////////////////////////////////////////////////////////////////////////

class MCNotificationEvent: public MCCustomEvent
{
private:
    MCNameRef m_message;
	MCStringRef m_notification;
    
public:
	MCNotificationEvent (MCNameRef p_message, MCStringRef p_notification)
	{
        m_message = p_message;
        m_notification = MCValueRetain(p_notification);
	}
	
	~MCNotificationEvent()
	{
		MCValueRelease(m_notification);
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message_with_valueref_args (m_message, m_notification);
	}
};

void MCNotificationPostLocalNotificationEvent(MCStringRef p_payload)
{
	MCEventQueuePostCustom(new MCNotificationEvent(MCM_local_notification_received, p_payload));
}

void MCNotificationPostPushNotificationEvent(MCStringRef p_payload)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_received, p_payload));
}

void MCNotificationPostPushRegistered (MCStringRef p_registration_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_registered, p_registration_text));
}

void MCNotificationPostPushRegistrationError (MCStringRef p_error_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_push_notification_registration_error, p_error_text));
}

void MCNotificationPostUrlWakeUp (MCStringRef p_url_wake_up_text)
{
	MCEventQueuePostCustom(new MCNotificationEvent (MCM_url_wake_up, p_url_wake_up_text));
}
