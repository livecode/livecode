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
#include "param.h"
#include "globals.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mbliphoneapp.h"

#import <Foundation/Foundation.h>
#import <MessageUI/MessageUI.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& parameters, const char *format, ...);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneMailComposerDialog : MFMailComposeViewController <MFMailComposeViewControllerDelegate>
{
	bool m_running;
}

- (bool)isRunning;

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error;

- (void)preWait;
- (void)postWait;

@end

static MCIPhoneMailComposerDialog *s_mail_composer_dialog = nil;

@implementation MCIPhoneMailComposerDialog

- (bool)isRunning
{
	return m_running;
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
	static const char *s_result_strings[] = {"cancel", "saved", "sent", "failed"};
	
	MCresult -> sets(s_result_strings[result]);
	
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)preWait
{
	[ MCIPhoneGetViewController() presentModalViewController: self animated: YES ];
	
	m_running = true;
}

- (void)postWait
{
	[ MCIPhoneGetViewController() dismissModalViewControllerAnimated: YES ];
}

@end

struct iphone_send_email_t
{
	const char *to_addresses;
	const char *cc_addresses;
	const char *subject;
	const char *body;
	MCIPhoneMailComposerDialog *dialog;
};

static void iphone_send_email_prewait(void *p_context)
{
	iphone_send_email_t *ctxt;
	ctxt = (iphone_send_email_t *)p_context;
	
	ctxt -> dialog = [[MCIPhoneMailComposerDialog alloc ] init];
	[ ctxt -> dialog setMailComposeDelegate: ctxt -> dialog ];
	
	NSArray *t_recipients;
	t_recipients = nil;
	if (ctxt -> to_addresses != nil && *ctxt -> to_addresses != '\0')
		t_recipients = [[NSString stringWithCString: ctxt -> to_addresses encoding: NSMacOSRomanStringEncoding] componentsSeparatedByString: @","];
	
	NSArray *t_ccs;
	t_ccs = nil;
	if (ctxt -> cc_addresses != nil && *ctxt -> cc_addresses != '\0')
		t_ccs = [[NSString stringWithCString: ctxt -> cc_addresses encoding: NSMacOSRomanStringEncoding] componentsSeparatedByString: @","];
	
	[ ctxt -> dialog setToRecipients: t_recipients ];
	
	[ ctxt -> dialog setCcRecipients: t_ccs ];
	
	if (ctxt -> subject != nil)
		[ ctxt -> dialog setSubject: [NSString stringWithCString: ctxt -> subject encoding: NSMacOSRomanStringEncoding]];
	else
		[ ctxt -> dialog setSubject: @"" ];
	
	if (ctxt -> body != nil)
		[ ctxt -> dialog setMessageBody: [NSString stringWithCString: ctxt -> body encoding: NSMacOSRomanStringEncoding] isHTML: NO ];
	else
		[ ctxt -> dialog setMessageBody: @"" isHTML: NO ];
	
	[ctxt -> dialog preWait];
}

static void iphone_send_email_postwait(void *p_context)
{
	iphone_send_email_t *ctxt;
	ctxt = (iphone_send_email_t *)p_context;
	
	[ctxt -> dialog postWait];
}

#ifdef /* MCIPhoneSendEmail */ LEGACY_EXEC
static void MCIPhoneSendEmail(const char *p_to_addresses, const char *p_cc_addresses, const char *p_subject, const char *p_body)
{
	if (![MCIPhoneMailComposerDialog canSendMail])
	{
		MCresult -> sets("not configured");
		return;
	}
	
	iphone_send_email_t ctxt;
	ctxt . to_addresses = p_to_addresses;
	ctxt . cc_addresses = p_cc_addresses;
	ctxt . subject = p_subject;
	ctxt . body = p_body;
	
	MCIPhoneRunOnMainFiber(iphone_send_email_prewait, &ctxt);
	
	while([ctxt . dialog isRunning])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(iphone_send_email_postwait, &ctxt);
	
	// Make sure we wait until only (presumably) the system has a reference to it.
	// (This ensures we can call mail multiple times in the same handler).
	/*while([ctxt . dialog retainCount] > 2)
		MCscreen -> wait(0.01, False, True);
	
	MCIPhoneCallSelectorOnMainFiber(ctxt . dialog, @selector(release));*/
}
#endif /* MCIPhoneSendEmail */

#ifdef /* MCHandleRevMailIphone */ LEGACY_EXEC
Exec_stat MCHandleRevMail(void *context, MCParameter *p_parameters)
{
	char *t_address, *t_cc_address, *t_subject, *t_message_body;
	t_address = nil;
	t_cc_address = nil;
	t_subject = nil;
	t_message_body = nil;
	
	MCExecPoint ep(nil, nil, nil);
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_address = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_cc_address = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_subject = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	if (p_parameters != nil)
	{
		p_parameters -> eval_argument(ep);
		t_message_body = ep . getsvalue() . clone();
		p_parameters = p_parameters -> getnext();
	}
	
	MCIPhoneSendEmail(t_address, t_cc_address, t_subject, t_message_body);
	
	delete t_address;
	delete t_cc_address;
	delete t_subject;
	delete t_message_body;
	
	return ES_NORMAL;
}
#endif /* MCHandleRevMailIphone */

////////////////////////////////////////////////////////////////////////////////

// iphoneCompose[Html|Unicode]Mail subject, to, cc, bcc, body, attachments
//   to / cc / bcc : comma or return separated lists, or numeric array
//   body : string
//   attachments : single dim array or numeric array of single dim arrays
//     data - binary data
//     file - filename
//     type - mime-type
//     name - preferred filename
//

enum MCMailType
{
	kMCMailTypePlain,
	kMCMailTypeUnicode,
	kMCMailTypeHtml
};

static NSString *mcstring_to_nsstring(const MCString& p_string, bool p_unicode)
{
	if (p_unicode)
		return [NSString stringWithCharacters: (unichar*)p_string . getstring() length: p_string . getlength() / 2];
	return [[[NSString alloc] initWithBytes: p_string . getstring() length: p_string . getlength() encoding: NSMacOSRomanStringEncoding] autorelease];
}

#ifdef /* array_to_attachmentIphone */ LEGACY_EXEC
static bool array_to_attachment(MCVariableArray *p_array, NSData*& r_data, NSString*& r_type, NSString*& r_name)
{
	MCHashentry *t_data, *t_file, *t_type, *t_name;
	t_data = p_array -> lookuphash("data", False, False);
	t_file = p_array -> lookuphash("file", False, False);
	t_type = p_array -> lookuphash("type", False, False);
	t_name = p_array -> lookuphash("name", False, False);
	
	MCExecPoint ep(nil, nil, nil);
	if (t_file == nil && t_data == nil)
		r_data = [[NSData alloc] initWithBytes: nil length: 0];
	else if (t_data != nil)
	{
		t_data -> value . fetch(ep, False);
		r_data = [[NSData alloc] initWithBytes: ep . getsvalue() . getstring() length: ep . getsvalue() . getlength()];
	}
	else if (t_file != nil)
	{
		char *t_path;
		t_file -> value . fetch(ep, False);
		t_path = MCS_resolvepath(ep . getcstring());
		r_data = [[NSData alloc] initWithContentsOfMappedFile: [NSString stringWithCString: t_path encoding: NSMacOSRomanStringEncoding]];
	}
	
	if (t_type == nil)
		r_type = @"application/octet-stream";
	else
	{
		t_type -> value . fetch(ep, False);
		r_type = [NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding];
	}
	
	if (t_name == nil)
		r_name = nil;
	else
	{
		t_name -> value . fetch(ep, False);
		r_name = [NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding];
	}
		
	return true;
}
#endif /* array_to_attachmentIphone */

struct compose_mail_t
{
	MCMailType type;
	MCParameter *parameters;
	MCIPhoneMailComposerDialog *dialog;
};

static void compose_mail_prewait(void *p_context)
{
#ifdef /* compose_mail_prewait */ LEGACY_EXEC
	compose_mail_t *ctxt;
	ctxt = (compose_mail_t *)p_context;
	
	bool t_success;
	t_success = true;
	
	char *t_to, *t_cc, *t_bcc;
	MCString t_subject, t_body;
	MCVariableValue *t_attachments;
	t_to = t_cc = t_bcc = nil;
	t_attachments = nil;
	
	if (t_success)
		t_success = MCParseParameters(ctxt -> parameters, "|dsssda", &t_subject, &t_to, &t_cc, &t_bcc, &t_body, &t_attachments);
	
	if (t_success)
	{
		ctxt -> dialog = [[MCIPhoneMailComposerDialog alloc ] init];
		[ ctxt -> dialog setMailComposeDelegate: ctxt -> dialog ];
		
		NSCharacterSet *t_separator_set;
		t_separator_set = [NSCharacterSet characterSetWithCharactersInString: @","];
		
		NSString *t_ns_subject;
		t_ns_subject = mcstring_to_nsstring(t_subject, ctxt -> type == kMCMailTypeUnicode);
		
		NSString *t_ns_body;
		t_ns_body = mcstring_to_nsstring(t_body, ctxt -> type == kMCMailTypeUnicode);
		
		NSArray *t_ns_to;
		t_ns_to = nil;
		if (t_to != nil && *t_to != '\0')
			t_ns_to = [[NSString stringWithCString: t_to encoding: NSMacOSRomanStringEncoding] componentsSeparatedByCharactersInSet: t_separator_set];
		
		NSArray *t_ns_cc;
		t_ns_cc = nil;
		if (t_cc != nil && *t_cc != '\0')
			t_ns_cc = [[NSString stringWithCString: t_cc encoding: NSMacOSRomanStringEncoding] componentsSeparatedByCharactersInSet: t_separator_set];
		
		NSArray *t_ns_bcc;
		t_ns_bcc = nil;
		if (t_bcc != nil && *t_bcc != '\0')
			t_ns_bcc = [[NSString stringWithCString: t_bcc encoding: NSMacOSRomanStringEncoding] componentsSeparatedByCharactersInSet: t_separator_set];
		
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
					
					NSData *t_data;
					NSString *t_type;
					NSString *t_name;
					if (array_to_attachment(t_entry -> value . get_array(), t_data, t_type, t_name))
					{
						[ctxt -> dialog addAttachmentData: t_data mimeType: t_type fileName: t_name];
						[t_data release];
					}
				}
			}
			else
			{
				NSData *t_data;
				NSString *t_type;
				NSString *t_name;
				if (array_to_attachment(t_array, t_data, t_type, t_name))
				{
					[ctxt -> dialog addAttachmentData: t_data mimeType: t_type fileName: t_name];
					[t_data release];
				}
			}
		}
		
		[ ctxt -> dialog setSubject: t_ns_subject ];
		[ ctxt -> dialog setToRecipients: t_ns_to ];
		[ ctxt -> dialog setCcRecipients: t_ns_cc ];
		[ ctxt -> dialog setBccRecipients: t_ns_bcc ];
		[ ctxt -> dialog setMessageBody: t_ns_body isHTML: ctxt -> type == kMCMailTypeHtml ];
		
		[ ctxt -> dialog preWait ];
	}
	
	delete t_subject . getstring();
	MCCStringFree(t_to);
	MCCStringFree(t_cc);
	MCCStringFree(t_bcc);
	delete t_body . getstring();
#endif /* compose_mail_prewait */
}

static void compose_mail_postwait(void *p_context)
{
	compose_mail_t *ctxt;
	ctxt = (compose_mail_t *)p_context;
	
	[ctxt -> dialog postWait];
}

#ifdef /* MCHandleComposeMailIphone */ LEGACY_EXEC
Exec_stat MCHandleComposeMail(MCMailType p_type, MCParameter *p_parameters)
{
	compose_mail_t ctxt;
	ctxt . type = p_type;
	ctxt . parameters = p_parameters;
	
	MCIPhoneRunOnMainFiber(compose_mail_prewait, &ctxt);
	
	while([ctxt . dialog isRunning])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(compose_mail_postwait, &ctxt);
	
	while([ctxt . dialog retainCount] > 2)
		MCscreen -> wait(0.01, False, True);
	
	MCIPhoneCallSelectorOnMainFiber(ctxt . dialog, @selector(release));

	return ES_NORMAL;
}
#endif /* MCHandleComposeMailIphone */

#ifdef /* MCHandleComposePlainMailIphone */ LEGACY_EXEC
Exec_stat MCHandleComposePlainMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypePlain, p_parameters);
}
#endif /* MCHandleComposePlainMailIphone */

#ifdef /* MCHandleComposeUnicodeMailIphone */ LEGACY_EXEC
Exec_stat MCHandleComposeUnicodeMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypeUnicode, p_parameters);
}
#endif /* MCHandleComposeUnicodeMailIphone */

#ifdef /* MCHandleComposeHtmlMailIphone */ LEGACY_EXEC
Exec_stat MCHandleComposeHtmlMail(void *context, MCParameter *p_parameters)
{
	return MCHandleComposeMail(kMCMailTypeHtml, p_parameters);
}
#endif /* MCHandleComposeHtmlMailIphone */

#ifdef /* MCHandleCanSendMailIphone */ LEGACY_EXEC
Exec_stat MCHandleCanSendMail(void *context, MCParameter *p_parameters)
{
	if (![MCIPhoneMailComposerDialog canSendMail])
	{
		MCresult -> sets(MCfalsestring);
		return ES_NORMAL;
	}
	
	MCresult -> sets(MCtruestring);
	
	return ES_NORMAL;
}
#endif /* MCHandleCanSendMailIphone */

////////////////////////////////////////////////////////////////////////////////
