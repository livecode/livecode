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

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include "date.h"

#include "mbldc.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblsyntax.h"

#include <string.h>

#include <jni.h>

typedef enum
{
    kMCAndroidTextWaiting,
	kMCAndroidTextSent,
	kMCAndroidTextCanceled,
} MCAndroidTextStatus;

static MCAndroidTextStatus s_text_status = kMCAndroidTextWaiting; 

void MCAndroidTextDone()
{
	s_text_status = kMCAndroidTextSent;
}

void MCAndroidTextCanceled()
{
	s_text_status = kMCAndroidTextCanceled;
}

bool MCSystemCanSendTextMessage()
{
    bool t_can_send = false;
	MCAndroidEngineCall("canSendTextMessage", "b", &t_can_send);
	return t_can_send;
}

bool MCSystemComposeTextMessage(MCStringRef p_recipients, MCStringRef p_body)
{
    s_text_status = kMCAndroidTextWaiting;

    MCAndroidEngineRemoteCall("composeTextMessage", "vxx", nil, p_recipients, p_body);
    while (s_text_status == kMCAndroidTextWaiting)
		MCscreen->wait(60.0, False, True);
	
	MCresult -> sets(s_text_status == kMCAndroidTextSent ? "sent" : "cancel");

    return true;
}
