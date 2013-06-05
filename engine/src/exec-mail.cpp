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
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EXEC_METHOD(Mail, SendEmail, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Mail, ComposeMail, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Mail, ComposeHtmlMail, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Mail, ComposeUnicodeMail, 6)
MC_EXEC_DEFINE_GET_METHOD(Mail, CanSendMail, 1)

////////////////////////////////////////////////////////////////////////////////

void MCMailDoComposeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments, MCMailType p_type)
{	
	void *dialog_ptr = nil;
	MCSystemPrepareMail(p_to, p_cc, p_bcc, p_subject, p_body, p_type, dialog_ptr);

	MCNewAutoNameRef t_data_name, t_file_name, t_type_name, t_name_name;
	MCNameCreateWithCString("data", &t_data_name);
	MCNameCreateWithCString("file", &t_file_name);
	MCNameCreateWithCString("type", &t_type_name);
	MCNameCreateWithCString("name", &t_name_name);

	if (p_attachments != nil)
	{
		if (MCArrayIsSequence(p_attachments))
		{
			MCValueRef t_data;
			MCValueRef t_file;
			MCValueRef t_type;
			MCValueRef t_name;

			for(uindex_t i = 0; i < MCArrayGetCount(p_attachments); i++)
			{
				MCValueRef t_value;
				MCArrayFetchValueAtIndex(p_attachments, i + 1, t_value);
				if (!MCValueIsArray(t_value))
					continue;
				
				MCArrayFetchValue((MCArrayRef)t_value, false, &t_data_name, t_data);
				MCArrayFetchValue((MCArrayRef)t_value, false, &t_file_name, t_file);
				MCArrayFetchValue((MCArrayRef)t_value, false, &t_type_name, t_type);
				MCArrayFetchValue((MCArrayRef)t_value, false, &t_name_name, t_name);
				MCSystemAddAttachment((MCStringRef)t_data, (MCStringRef)t_file, (MCStringRef)t_type, (MCStringRef)t_name, dialog_ptr);
			}	
		}
		else
		{
			MCValueRef t_data;
			MCValueRef t_file;
			MCValueRef t_type;
			MCValueRef t_name;

			MCArrayFetchValue(p_attachments, false, &t_data_name, t_data);
			MCArrayFetchValue(p_attachments, false, &t_file_name, t_file);
			MCArrayFetchValue(p_attachments, false, &t_type_name, t_type);
			MCArrayFetchValue(p_attachments, false, &t_name_name, t_name);

			MCSystemAddAttachment((MCStringRef)t_data, (MCStringRef)t_file, (MCStringRef)t_type, (MCStringRef)t_name, dialog_ptr);	
		}
	}

	MCSystemSendPreparedMail(&dialog_ptr);

	MCAutoStringRef t_result;
	MCSystemMailResult(&t_result);
	ctxt . SetTheResultToValue(*t_result);
}

void MCMailExecSendEmail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_subject, MCStringRef p_body)
{
    bool t_can_send;
    MCMailGetCanSendMail(ctxt, t_can_send);
    
    if (t_can_send)
        MCSystemSendMail(p_to, p_cc, p_subject, p_body);
    else
        ctxt . SetTheResultToStaticCString("not supported");
}

void MCMailExecComposeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments)
{
	MCMailDoComposeMail(ctxt, p_to, p_cc, p_bcc, p_subject, p_body, p_attachments, kMCMailTypePlain);
}

void MCMailExecComposeUnicodeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments)
{
	MCMailDoComposeMail(ctxt, p_to, p_cc, p_bcc, p_subject, p_body, p_attachments, kMCMailTypeUnicode);
}

void MCMailExecComposeHtmlMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments)
{
	MCMailDoComposeMail(ctxt, p_to, p_cc, p_bcc, p_subject, p_body, p_attachments, kMCMailTypeHtml);
}

void MCMailGetCanSendMail(MCExecContext& ctxt, bool& r_result)
{
	MCSystemGetCanSendMail(r_result);
}

////////////////////////////////////////////////////////////////////////////////
