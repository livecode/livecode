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
#include "image.h"
#include "param.h"

#include "exec.h"
#include "mblsyntax.h"

#include "mbliphoneapp.h"

#import <MessageUI/MessageUI.h>

UIViewController *MCIPhoneGetViewController(void);

@interface com_runrev_livecode_MCIPhoneSmsComposerDialog : MFMessageComposeViewController <MFMessageComposeViewControllerDelegate>
{
	bool m_running;
}

- (bool)isRunning;

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result;
- (void)preWait;
- (void)postWait;
@end

static com_runrev_livecode_MCIPhoneSmsComposerDialog *s_sms_composer_dialog = nil;

@implementation com_runrev_livecode_MCIPhoneSmsComposerDialog

- (bool)isRunning
{
	return m_running;
}

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
	static const char *s_result_strings[] = {"cancel", "sent", "failed"};
	
	MCresult -> sets(s_result_strings[result]);
	
	m_running = false;
	
	MCscreen -> pingwait();
}

- (void)preWait
{
	[ MCIPhoneGetViewController() presentModalViewController: self animated: YES ];

	m_running = true;
}

- (void)postWait
{
	[MCIPhoneGetViewController() dismissModalViewControllerAnimated: YES];
	
	[self release];
}

@end

static void do_can_send_text_message(void *p_context)
{  
	MFMessageComposeViewController *t_sms_controller = [[[MFMessageComposeViewController alloc] init] autorelease];
	if([MFMessageComposeViewController canSendText])
		*(bool *)p_context = true;
	else
		*(bool *)p_context = false;
}

bool MCSystemCanSendTextMessage(void)
{
	bool t_result;
	MCIPhoneRunOnMainFiber(do_can_send_text_message, &t_result);
	return t_result;
}

struct compose_text_message_t
{
	MCStringRef recipients;
	MCStringRef body;
	com_runrev_livecode_MCIPhoneSmsComposerDialog *dialog;
	bool success;
};

static void compose_text_message_prewait(void *p_context)
{
	compose_text_message_t *ctxt;
	ctxt = (compose_text_message_t *)p_context;
	
    MFMessageComposeViewController *t_sms_controller = [[[MFMessageComposeViewController alloc] init] autorelease];
	if (![MFMessageComposeViewController canSendText])
	{
		ctxt -> success = false;
		return;
	}
	
	ctxt -> dialog = [[com_runrev_livecode_MCIPhoneSmsComposerDialog alloc] init];
    [ctxt -> dialog setMessageComposeDelegate: ctxt -> dialog];
    [ctxt -> dialog setRecipients: [MCStringConvertToAutoreleasedNSString(ctxt -> recipients) componentsSeparatedByString:@","]];
    [ctxt -> dialog setBody: MCStringConvertToAutoreleasedNSString(ctxt -> body)];
	[ctxt -> dialog preWait];
	
	ctxt -> success = true;
}

static void compose_text_message_postwait(void *p_context)
{
	compose_text_message_t *ctxt;
	ctxt = (compose_text_message_t *)p_context;
	[ctxt -> dialog postWait];
}

bool MCSystemComposeTextMessage(MCStringRef p_recipients, MCStringRef p_body)
{
	compose_text_message_t ctxt;
	ctxt . recipients = p_recipients;
	ctxt . body = p_body;
	MCIPhoneRunOnMainFiber(compose_text_message_prewait, &ctxt);
	while([ctxt . dialog isRunning])
		MCscreen -> wait(60.0, False, True);
	MCIPhoneRunOnMainFiber(compose_text_message_postwait, &ctxt);
    return true;
}
