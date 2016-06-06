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
#include "param.h"
#include "mblsyntax.h"

#include "mblandroidutil.h"

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

bool MCCanSendMail()
{
	bool t_can_send = false;
	MCAndroidEngineCall("canSendMail", "b", &t_can_send);
	return t_can_send;
}

void MCAndroidSendEmail(MCStringRef p_address, MCStringRef p_cc_address, MCStringRef p_subject, MCStringRef p_message_body)
{
	MCAndroidEngineCall("sendEmail", "vxxxx", nil, p_address, p_cc_address, p_subject, p_message_body);
}

typedef enum
{
	kMCAndroidMailWaiting,
	kMCAndroidMailSent,
	kMCAndroidMailCanceled,
	kMCAndroidMailUnknown,
} MCAndroidMailStatus;

static MCAndroidMailStatus s_mail_status = kMCAndroidMailWaiting;


void MCAndroidMailDone()
{
	s_mail_status = kMCAndroidMailSent;
}

void MCAndroidMailCanceled()
{
//	s_mail_status = kMCAndroidMailCanceled;
	// IM-2012-10-22 - [[ BZ 10486 ]] - android mail activity always returns canceled
	// regardless of what the user does so for now we just return unknown
	s_mail_status = kMCAndroidMailUnknown;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidMailResult(MCStringRef& r_result)
{
	while (s_mail_status == kMCAndroidMailWaiting)
		MCscreen->wait(60.0, False, True);

	switch (s_mail_status)
	{
		case kMCAndroidMailSent:
			/* UNCHECKED */ MCStringCreateWithCString("sent", r_result);
			return;
	
		case kMCAndroidMailCanceled:
			/* UNCHECKED */ MCStringCreateWithCString("cancel", r_result);
			return;
			
		case kMCAndroidMailUnknown:
		default:
			/* UNCHECKED */ MCStringCreateWithCString("unknown", r_result);
			return;
	}
}

void MCSystemSendMail(MCStringRef p_address, MCStringRef p_cc_address, MCStringRef p_subject, MCStringRef p_message_body, MCStringRef& r_result)
{
	s_mail_status = kMCAndroidMailWaiting;
	MCAndroidSendEmail(p_address, p_cc_address, p_subject, p_message_body);
	MCAndroidMailResult(r_result);
}

void MCSystemSendMailWithAttachments(MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCMailType p_type, MCAttachmentData *p_attachments, uindex_t p_attachment_count, MCStringRef& r_result)
{
	const char *t_prep_sig;
	t_prep_sig = "vxxxxxb";

	MCAndroidEngineCall("prepareEmail", t_prep_sig, nil, p_to, p_cc, p_bcc, p_subject, p_body, p_type == kMCMailTypeHtml);

	for (uindex_t i = 0; i < p_attachment_count; i++)
	{
		if (p_attachments[i] . file != nil && MCStringGetLength(p_attachments[i] . file) > 0)
			MCAndroidEngineCall("addAttachment", "vxxx", nil, p_attachments[i] . file, p_attachments[i] . type, p_attachments[i] . name);
		else
			MCAndroidEngineCall("addAttachment", "vdxx", nil, p_attachments[i] . data, p_attachments[i] . type, p_attachments[i] . name);
	}

    // MW-2014-02-12: [[ Bug 11789 ]] Make sure we reset the state to 'waiting'
    //   so we don't fall through the wait loop.
    s_mail_status = kMCAndroidMailWaiting;
    
	MCAndroidEngineCall("sendEmail", "v", nil);
	
	MCAndroidMailResult(r_result);
}

void MCSystemGetCanSendMail(bool& r_result)
{
	r_result = MCCanSendMail();
}
