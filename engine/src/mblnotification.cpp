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

#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////

void FreeNotification(MCNotification &p_notification)
{
    MCValueRelease(p_notification.body);
    MCValueRelease(p_notification.action);
    MCValueRelease(p_notification.user_info);
}

////////////////////////////////////////////////////////////////////////////////

static void MCParameterDeleteList(MCParameter *p_params)
{
	while (p_params != nil)
	{
		MCParameter *t_param;
		t_param = p_params;
		p_params = p_params->getnext();
		delete t_param;
	}
}

class MCNotificationEvent: public MCCustomEvent
{
private:
    MCNameRef m_message;
	MCParameter *m_params;
	
public:
	MCNotificationEvent (MCNameRef p_message, MCParameter *p_params)
	{
        m_message = MCValueRetain(p_message);
		m_params = p_params;
	}
	
	~MCNotificationEvent()
	{
		MCValueRelease(m_message);
		MCParameterDeleteList(m_params);
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message(m_message, m_params);
	}
};

bool MCNotificationPostCustom(MCNameRef p_name, uint32_t p_param_count, ...)
{
	bool t_success;
	t_success = true;
	
	MCParameter *t_param_list;
	t_param_list = nil;
	
	MCParameter *t_param;
	t_param = nil;
	
	va_list t_args;
	va_start(t_args, p_param_count);
	
	for (uint32_t i = 0; t_success && i < p_param_count; i++)
	{
		MCValueRef t_value;
		t_value = va_arg(t_args, MCValueRef);
		
		MCParameter *t_new_param;
		t_new_param = new (nothrow) MCParameter();
		t_success = t_new_param != nil;
		
		if (t_success)
		{
			t_new_param->setvalueref_argument(t_value);
			if (t_param != nil)
				t_param->setnext(t_new_param);
			t_param = t_new_param;
			
			if (t_param_list == nil)
				t_param_list = t_param;
		}
	}
	
	va_end(t_args);
	
	MCNotificationEvent *t_event;
	t_event = nil;
	
	if (t_success)
	{
		t_event = new (nothrow) MCNotificationEvent(p_name, t_param_list);
		t_success = t_event != nil;
	}
	
	if (t_success)
		t_success = MCEventQueuePostCustom(t_event);

	if (!t_success)
	{
		if (t_event != nil)
			delete t_event;
		else
			MCParameterDeleteList(t_param_list);
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCNotificationPostLocalNotificationEvent(MCStringRef p_payload)
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_local_notification_received, 1, p_payload);
}

void MCNotificationPostPushNotificationEvent(MCStringRef p_payload)
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_push_notification_received, 1, p_payload);
}

void MCNotificationPostPushRegistered (MCStringRef p_registration_text)
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_push_notification_registered, 1, p_registration_text);
}

void MCNotificationPostPushRegistrationError (MCStringRef p_error_text)
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_push_notification_registration_error, 1, p_error_text);
}

void MCNotificationPostUrlWakeUp (MCStringRef p_url_wake_up_text)
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_url_wake_up, 1, p_url_wake_up_text);
}

void MCNotificationPostLaunchDataChanged()
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_launch_data_changed, 0);
}

void MCNotificationPostSystemAppearanceChanged()
{
	/* UNCHECKED */ MCNotificationPostCustom(MCM_system_appearance_changed, 0);
}

////////////////////////////////////////////////////////////////////////////////
