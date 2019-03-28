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

#include "param.h"
#include "globals.h"
#include "eventqueue.h"
#include "osspec.h"
#include "mblsyntax.h"

#include "mbliphone.h"
#include "mbliphoneapp.h"

#import <Foundation/Foundation.h>
#import <MessageUI/MessageUI.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& parameters, const char *format, ...);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCIPhoneMailComposerDialog : MFMailComposeViewController <MFMailComposeViewControllerDelegate>
{
	bool m_running;
}

- (bool)isRunning;

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error;

- (void)preWait;
- (void)postWait;

@end

static com_runrev_livecode_MCIPhoneMailComposerDialog *s_mail_composer_dialog = nil;

@implementation com_runrev_livecode_MCIPhoneMailComposerDialog

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
/*	const char *to_addresses;
	const char *cc_addresses;
	const char *subject;
	const char *body;
	MCIPhoneMailComposerDialog *dialog; */

	MCStringRef to_addresses;
	MCStringRef cc_addresses;
	MCStringRef subject;
	MCStringRef body;
	com_runrev_livecode_MCIPhoneMailComposerDialog *dialog;
};

static void iphone_send_email_prewait(void *p_context)
{
/*	iphone_send_email_t *ctxt;
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
	
	[ctxt -> dialog preWait]; */

	iphone_send_email_t *ctxt;
	ctxt = (iphone_send_email_t *)p_context;
	
	ctxt -> dialog = [[com_runrev_livecode_MCIPhoneMailComposerDialog alloc ] init];
	[ ctxt -> dialog setMailComposeDelegate: ctxt -> dialog ];
	
	NSArray *t_recipients;
	t_recipients = nil;
	if (ctxt -> to_addresses != nil && !MCStringIsEmpty(ctxt -> to_addresses))
		t_recipients = [MCStringConvertToAutoreleasedNSString(ctxt -> to_addresses) componentsSeparatedByString: @","];
	
	NSArray *t_ccs;
	t_ccs = nil;
	if (ctxt -> cc_addresses != nil && !MCStringIsEqualTo(ctxt -> cc_addresses, kMCEmptyString, kMCCompareCaseless))
		t_ccs = [MCStringConvertToAutoreleasedNSString(ctxt -> cc_addresses) componentsSeparatedByString: @","];
	
	[ ctxt -> dialog setToRecipients: t_recipients ];
	
	[ ctxt -> dialog setCcRecipients: t_ccs ];
	
	if (ctxt -> subject != nil)
		[ ctxt -> dialog setSubject: MCStringConvertToAutoreleasedNSString(ctxt -> subject)];
	else
		[ ctxt -> dialog setSubject: @"" ];
	
	if (ctxt -> body != nil)
		[ ctxt -> dialog setMessageBody: MCStringConvertToAutoreleasedNSString(ctxt -> body) isHTML: NO ];
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

static NSArray *mcstringref_to_nsarray(MCStringRef p_string, NSCharacterSet* p_separator_set)
{
	return [MCStringConvertToAutoreleasedNSString(p_string) componentsSeparatedByCharactersInSet: p_separator_set];
}

static NSData *mcstringref_to_nsdata(MCStringRef p_string)
{
    MCAutoPointer<char> t_string;
    /* UNCHECKED */ MCStringConvertToCString(p_string, &t_string);
	return [[NSData alloc] initWithBytes: *t_string length: MCStringGetLength(p_string)];
}

struct compose_mail_t
{
/*	MCMailType type;
	MCParameter *parameters;
	MCIPhoneMailComposerDialog *dialog; */

	MCStringRef to;
	MCStringRef cc;
	MCStringRef bcc;
	MCStringRef subject;
	MCStringRef body;
	MCMailType type;
	MCAttachmentData *attachments;
	uindex_t attachment_count;
	com_runrev_livecode_MCIPhoneMailComposerDialog *dialog;
};

static void compose_mail_prewait(void *p_context)
{
	compose_mail_t *ctxt;
	ctxt = (compose_mail_t *)p_context;
	
	bool t_success;
	t_success = true;

	ctxt -> dialog = [[com_runrev_livecode_MCIPhoneMailComposerDialog alloc ] init];
	[ ctxt -> dialog setMailComposeDelegate: ctxt -> dialog ];

	if (ctxt -> attachments != nil)
	{
		for (uindex_t i = 0; i < ctxt -> attachment_count; i++)
		{
			NSData *t_data;
			NSString *t_type;
			NSString *t_name;

			if (ctxt -> attachments[i] . file == nil && ctxt -> attachments[i] . data == nil)
				t_data = [[NSData alloc] initWithBytes: nil length: 0];
			else if (ctxt -> attachments[i] . data != nil)
				t_data = MCDataConvertToAutoreleasedNSData(ctxt -> attachments[i] . data);
			else
			{
                MCAssert(ctxt -> attachments[i] . file != nil);
				MCAutoStringRef t_resolved_path;
				MCS_resolvepath(ctxt -> attachments[i] . file, &t_resolved_path);
				t_data = [[NSData alloc] initWithContentsOfMappedFile: MCStringConvertToAutoreleasedNSString(*t_resolved_path)];
			}
			
			if (ctxt -> attachments[i] . type == nil)
				t_type = @"application/octet-stream";
			else
				t_type = MCStringConvertToAutoreleasedNSString(ctxt -> attachments[i] . type);
			
            // "addAttachmentData:" requires that all three arguments are non-nil
			if (ctxt -> attachments[i] . name == nil)
				t_name = @"";
			else
				t_name = MCStringConvertToAutoreleasedNSString(ctxt -> attachments[i] . name);
				
            [ctxt -> dialog addAttachmentData: t_data mimeType: t_type fileName: t_name];
		}
	}

	NSCharacterSet *t_separator_set;
	t_separator_set = [NSCharacterSet characterSetWithCharactersInString: @","];
	
	NSString *t_ns_subject;
	t_ns_subject = MCStringConvertToAutoreleasedNSString(ctxt -> subject);
	
	NSString *t_ns_body;
    t_ns_body = nil;
    if (ctxt -> body != nil && !MCStringIsEmpty(ctxt -> body))
        t_ns_body = MCStringConvertToAutoreleasedNSString(ctxt -> body);
	
	NSArray *t_ns_to;
	t_ns_to = nil;
	if (ctxt -> to != nil && !MCStringIsEmpty(ctxt -> to))
		t_ns_to = mcstringref_to_nsarray(ctxt -> to, t_separator_set);
	
	NSArray *t_ns_cc;
	t_ns_cc = nil;
	if (ctxt -> cc != nil && !MCStringIsEmpty(ctxt -> cc))
		t_ns_cc = mcstringref_to_nsarray(ctxt -> cc, t_separator_set);

	NSArray *t_ns_bcc;
	t_ns_bcc = nil;
	if (ctxt -> bcc != nil && !MCStringIsEmpty(ctxt -> bcc))
		t_ns_bcc = mcstringref_to_nsarray(ctxt -> bcc, t_separator_set);	

	[ ctxt -> dialog setSubject: t_ns_subject ];
	[ ctxt -> dialog setToRecipients: t_ns_to ];
	[ ctxt -> dialog setCcRecipients: t_ns_cc ];
	[ ctxt -> dialog setBccRecipients: t_ns_bcc ];
	[ ctxt -> dialog setMessageBody: t_ns_body isHTML: ctxt -> type == kMCMailTypeHtml ];

	[ctxt -> dialog preWait];
}

static void compose_mail_postwait(void *p_context)
{
	compose_mail_t *ctxt;
	ctxt = (compose_mail_t *)p_context;
	
	[ctxt -> dialog postWait];
}

////////////////////////////////////////////////////////////////////////////////

void MCSystemSendMail(MCStringRef p_to, MCStringRef p_cc, MCStringRef p_subject, MCStringRef p_body, MCStringRef& r_result)
{
	iphone_send_email_t ctxt;
	ctxt . to_addresses = p_to;
	ctxt . cc_addresses = p_cc;
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

	MCSystemMailResult(r_result);
}

void MCSystemSendMailWithAttachments(MCStringRef p_to, MCStringRef p_cc, MCStringRef p_bcc, MCStringRef p_subject, MCStringRef p_body, MCMailType p_type, MCAttachmentData *p_attachments, uindex_t p_attachment_count, MCStringRef& r_result)
{
	compose_mail_t ctxt;

	ctxt . to = p_to;
	ctxt . cc = p_cc;
	ctxt . bcc = p_bcc;
	ctxt . subject = p_subject;
	ctxt . body = p_body;
	ctxt . type = p_type;
	ctxt . attachments = p_attachments;
	ctxt . attachment_count = p_attachment_count;
	
	MCIPhoneRunOnMainFiber(compose_mail_prewait, &ctxt);
	
	while([ctxt . dialog isRunning])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(compose_mail_postwait, &ctxt);
	
	while([ctxt . dialog retainCount] > 2)
		MCscreen -> wait(0.01, False, True);
	
	MCIPhoneCallSelectorOnMainFiber(ctxt . dialog, @selector(release));

	MCSystemMailResult(r_result);
}

void MCSystemGetCanSendMail(bool& r_result)
{
	r_result = [com_runrev_livecode_MCIPhoneMailComposerDialog canSendMail];
}

void MCSystemMailResult(MCStringRef& r_result)
{
	// PM-2016-02-29: [[ Bug 17031 ]] Make sure we return 'the result'
	// The result can be one of "cancel", "saved", "sent", "failed"
	MCAssert(MCValueGetTypeCode(MCresult -> getvalueref()) == kMCValueTypeCodeString);
	
	r_result = MCValueRetain((MCStringRef)MCresult -> getvalueref());
}
