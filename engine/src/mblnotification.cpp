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
