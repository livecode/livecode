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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "execpt.h"
#include "globals.h"
#include "param.h"

#include "mblandroidutil.h"

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

bool MCCanSendMail()
{
	bool t_can_send = false;
	MCAndroidEngineCall("canSendMail", "b", &t_can_send);
	return t_can_send;
}

#ifdef /* MCHandleCanSendMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	MCresult->sets(MCU_btos(MCCanSendMail()));
	return ES_NORMAL;
}
#endif /* MCHandleCanSendMailAndroid */

void MCAndroidSendEmail(const char *p_address, const char *p_cc_address, const char *p_subject, const char *p_message_body)
{
	MCAndroidEngineCall("sendEmail", "vssss", nil, p_address, p_cc_address, p_subject, p_message_body);
}

typedef enum
{
	kMCAndroidMailWaiting,
	kMCAndroidMailSent,
	kMCAndroidMailCanceled,
	kMCAndroidMailUnknown,
} MCAndroidMailStatus;

static MCAndroidMailStatus s_mail_status = kMCAndroidMailWaiting;

#ifdef /* MCHandleRevMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	char *t_address, *t_cc_address, *t_subject, *t_message_body;
	t_address = nil;
	t_cc_address = nil;
	t_subject = nil;
	t_message_body = nil;
	
	MCExecPoint ep(nil, nil, nil);
	
	MCParseParameters(p_parameters, "|ssss", &t_address, &t_cc_address, &t_subject, &t_message_body);
	
	s_mail_status = kMCAndroidMailWaiting;
	MCAndroidSendEmail(t_address, t_cc_address, t_subject, t_message_body);

	while (s_mail_status == kMCAndroidMailWaiting)
		MCscreen->wait(60.0, False, True);

	switch (s_mail_status)
	{
		case kMCAndroidMailSent:
			MCresult -> sets("sent");
			break;
			
		case kMCAndroidMailCanceled:
			MCresult -> sets("cancel");
			break;
			
		case kMCAndroidMailUnknown:
		default:
			MCresult -> sets("unknown");
			break;
	}
	
	delete t_address;
	delete t_cc_address;
	delete t_subject;
	delete t_message_body;
	
	return ES_NORMAL;
}
#endif /* MCHandleRevMailAndroid */

enum MCMailType
{
	kMCMailTypePlain,
	kMCMailTypeUnicode,
	kMCMailTypeHtml
};

#ifdef /* array_to_attachmentAndroid */ LEGACY_EXEC
static bool array_to_attachment(MCVariableArray *p_array, MCString &r_data, MCString &r_file, MCString &r_type, MCString &r_name)
{
	MCHashentry *t_data, *t_file, *t_type, *t_name;
	t_data = p_array -> lookuphash("data", False, False);
	t_file = p_array -> lookuphash("file", False, False);
	t_type = p_array -> lookuphash("type", False, False);
	t_name = p_array -> lookuphash("name", False, False);

	MCExecPoint ep;

	bool t_success = true;

	if (t_success && t_data != nil && !t_data->value.is_empty())
	{
		t_success = t_data->value.is_string();
		if (t_success)
			r_data = t_data->value.get_string();
	}
	if (t_success && t_file != nil && !t_file->value.is_empty())
	{
		t_success = t_file->value.is_string();
		if (t_success)
			r_file = t_file->value.get_string();
	}
	if (t_success && t_type != nil && !t_type->value.is_empty())
	{
		t_success = t_type->value.is_string();
		if (t_success)
			r_type = t_type->value.get_string();
	}
	if (t_success && t_name != nil && !t_name->value.is_empty())
	{
		t_success = t_name->value.is_string();
		if (t_success)
			r_name = t_name->value.get_string();
	}

	return t_success;
}
#endif /* array_to_attachmentAndroid */

#ifdef /* MCHandleComposeMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleComposeMail(MCMailType p_type, MCParameter *p_parameters)
{
	bool t_success;
	t_success = true;
	
	char *t_to, *t_cc, *t_bcc;
	MCString t_subject, t_body;
	MCVariableValue *t_attachments;
	t_to = t_cc = t_bcc = nil;
	t_attachments = nil;
	
	if (t_success)
		t_success = MCParseParameters(p_parameters, "|dsssda", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);

	if (t_success)
	{
		//MCLog("mail type: %d", p_type);
		//MCLog("subject: %d: \"%s\"", t_subject.getlength(), t_subject.getstring());
		//MCLog("body: %d: \"%s\"", t_body.getlength(), t_body.getstring());
		const char *t_prep_sig;
		if (p_type == kMCMailTypeUnicode)
			t_prep_sig = "vsssUUb";
		else
			t_prep_sig = "vsssSSb";

		MCAndroidEngineCall("prepareEmail", t_prep_sig, nil, t_to, t_cc, t_bcc, &t_subject, &t_body, p_type == kMCMailTypeHtml);

		// add attachments
		if (t_attachments != nil)
		{
			MCVariableArray *t_array;
			t_array = t_attachments -> get_array();
			if (t_array -> isnumeric())
			{
				for(uint32_t i = 0; i < t_array -> getnfilled(); i++)
				{
					MCHashentry *t_entry;
					t_entry = t_array -> lookupindex(i + 1, False);
					if (t_entry == nil)
						continue;
					if (!t_entry -> value . is_array())
						continue;
						
					MCString t_data;
					MCString t_file;
					MCString t_type;
					MCString t_name;
					if (array_to_attachment(t_entry -> value . get_array(), t_data, t_file, t_type, t_name))
					{
						//MCLog("file: %p: %d: \"%s\"", &t_file, t_file.getlength(), t_file.getstring() != nil ? t_file.getstring() : "NULL");
						//MCLog("data: %p: %d: \"...\"", &t_data, t_data.getlength());
						//MCLog("type: %p: %d: \"%s\"", &t_type, t_type.getlength(), t_type.getstring() != nil ? t_type.getstring() : "NULL");
						//MCLog("name: %p: %d: \"%s\"", &t_name, t_name.getlength(), t_name.getstring() != nil ? t_name.getstring() : "NULL");
						if (t_file.getlength() > 0)
							MCAndroidEngineCall("addAttachment", "vSSS", nil, &t_file, &t_type, &t_name);
						else
							MCAndroidEngineCall("addAttachment", "vdSS", nil, &t_data, &t_type, &t_name);
					}
				}
			}
			else
			{
				MCString t_data;
				MCString t_file;
				MCString t_type;
				MCString t_name;
				if (array_to_attachment(t_array, t_data, t_file, t_type, t_name))
				{
					if (t_file.getlength() > 0)
						MCAndroidEngineCall("addAttachment", "vSSS", nil, &t_file, &t_type, &t_name);
					else
						MCAndroidEngineCall("addAttachment", "vdSS", nil, &t_data, &t_type, &t_name);
				}
			}
		}

		MCAndroidEngineCall("sendEmail", "v", nil);
	}
	
	while (s_mail_status == kMCAndroidMailWaiting)
		MCscreen->wait(60.0, False, True);
	
	switch (s_mail_status)
	{
		case kMCAndroidMailSent:
			MCresult -> sets("sent");
			break;
			
		case kMCAndroidMailCanceled:
			MCresult -> sets("cancel");
			break;
			
		case kMCAndroidMailUnknown:
		default:
			MCresult -> sets("unknown");
			break;
	}
	
	delete t_subject . getstring();
	MCCStringFree(t_to);
	MCCStringFree(t_cc);
	MCCStringFree(t_bcc);
	delete t_body . getstring();
	
	return ES_NORMAL;
}
#endif /* MCHandleComposeMailAndroid */

#ifdef /* MCHandleComposePlainMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleComposePlainMail(void *context, MCParameter *parameters)
{
	return MCHandleComposeMail(kMCMailTypePlain, parameters);
}
#endif /* MCHandleComposePlainMailAndroid */

#ifdef /* MCHandleComposeUnicodeMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *parameters)
{
	//MCLog("MCHandleComposeUnicodeMail", nil);
	return MCHandleComposeMail(kMCMailTypeUnicode, parameters);
}
#endif /* MCHandleComposeUnicodeMailAndroid */

#ifdef /* MCHandleComposeHtmlMailAndroid */ LEGACY_EXEC
Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *parameters)
{
	return MCHandleComposeMail(kMCMailTypeHtml, parameters);
}
#endif /* MCHandleComposeHtmlMailAndroid */

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
