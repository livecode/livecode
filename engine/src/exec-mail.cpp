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
#include "debug.h"
#include "handler.h"

#include "mblsyntax.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

void MCMailDoComposeMail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCArrayRef p_attachments, MCMailType p_type)
{	
	bool t_can_send;
	MCMailGetCanSendMail(ctxt, t_can_send);

	if (!t_can_send)
		return;

	MCAutoArray<MCAttachmentData> t_attachments;

	if (p_attachments != nil && !MCArrayIsEmpty(p_attachments))
	{
		MCValueRef t_data;
		MCValueRef t_file;
		MCValueRef t_type;
		MCValueRef t_name;
		MCAttachmentData t_attachment;
		
		if (MCArrayIsSequence(p_attachments))
		{
			for(uindex_t i = 0; i < MCArrayGetCount(p_attachments); i++)
			{
				MCValueRef t_value;
				MCArrayFetchValueAtIndex(p_attachments, i + 1, t_value);
				if (!MCValueIsArray(t_value))
					continue;

                if (!MCArrayFetchValue((MCArrayRef)t_value, false, MCNAME("data"), t_data) ||
                    !ctxt . ConvertToData(t_data, t_attachment . data))
                    t_attachment . data = nil;
                
                if (!MCArrayFetchValue((MCArrayRef)t_value, false, MCNAME("file"), t_file) ||
                    !ctxt . ConvertToString(t_file, t_attachment . file))
                    t_attachment . file = nil;
                
                if (!MCArrayFetchValue((MCArrayRef)t_value, false, MCNAME("type"), t_type) ||
                    !ctxt . ConvertToString(t_type, t_attachment . type))
                    t_attachment . type = nil;
                
                if (!MCArrayFetchValue((MCArrayRef)t_value, false, MCNAME("name"), t_name) ||
                    !ctxt . ConvertToString(t_name, t_attachment . name))
                    t_attachment . name = nil;

				t_attachments . Push(t_attachment);
			}	
		}
		else
		{
			if (!MCArrayFetchValue(p_attachments, false, MCNAME("data"), t_data) ||
                !ctxt . ConvertToData(t_data, t_attachment . data))
                t_attachment . data = nil;
            
            if (!MCArrayFetchValue(p_attachments, false, MCNAME("file"), t_file) ||
                !ctxt . ConvertToString(t_file, t_attachment . file))
                t_attachment . file = nil;
            
            if (!MCArrayFetchValue(p_attachments, false, MCNAME("type"), t_type) ||
                !ctxt . ConvertToString(t_type, t_attachment . type))
                t_attachment . type = nil;
            
            if (!MCArrayFetchValue(p_attachments, false, MCNAME("name"), t_name) ||
                !ctxt . ConvertToString(t_name, t_attachment . name))
                t_attachment . name = nil;
		
			t_attachments . Push(t_attachment);
		}
	}

	MCAutoStringRef t_result;

	MCSystemSendMailWithAttachments(p_to, p_cc, p_bcc, p_subject, p_body, p_type, t_attachments . Ptr(), t_attachments . Size(), &t_result);

	ctxt . SetTheResultToValue(*t_result);
}

void MCMailExecSendEmail(MCExecContext& ctxt, MCStringRef p_to, MCStringRef p_cc, MCStringRef p_subject, MCStringRef p_body)
{
    bool t_can_send;
    MCMailGetCanSendMail(ctxt, t_can_send);
    
    MCAutoStringRef t_result;
    if (t_can_send)
        MCSystemSendMail(p_to, p_cc, p_subject, p_body, &t_result);
    
    if (*t_result != nil)
        ctxt . SetTheResultToValue(*t_result);
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
