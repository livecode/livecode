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

#include "mcerror.h"
//#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"

#include "mbldc.h"
#include "mbliphoneapp.h"
#include "mbliphoneview.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

////////////////////////////////////////////////////////////////////////////////

extern UIViewController *MCIPhoneGetViewController(void);
extern UIView *MCIPhoneGetView(void);
extern UITextView *MCIPhoneGetTextView(void);

////////////////////////////////////////////////////////////////////////////////

static bool s_in_modal = false;

@interface ModalDelegate : NSObject <UITextFieldDelegate>
{
	NSInteger m_index;
	UIAlertView *m_view;
}
@end

@implementation ModalDelegate

- (void)setView: (UIAlertView *)view
{
	m_view = view;
}

- (void)alertView: (UIAlertView *)alertView didDismissWithButtonIndex: (NSInteger)buttonIndex
{
	s_in_modal = false;
	
	NSRunLoop *t_run_loop;
	t_run_loop = [NSRunLoop mainRunLoop];
	CFRunLoopStop([t_run_loop getCFRunLoop]);
	
	m_index = buttonIndex;
}

// MW-2012-10-12: [[ Bug 10175 ]] Upon pressing return, make sure we dismiss the dialog.
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	[m_view dismissWithClickedButtonIndex: [m_view firstOtherButtonIndex] animated: YES];
	return YES;
}

- (NSInteger)index
{
	return m_index;
}

- (void)setIndex: (NSInteger)p_index
{
	m_index = p_index;
}

@end

struct popupanswerdialog_t
{
	MCStringRef *buttons;
	uint32_t button_count;
	uint32_t type;
	MCStringRef title;
	MCStringRef message;
	int32_t result;
	
	UIAlertView *alert_view;
	ModalDelegate *delegate;
};

static void dopopupanswerdialog_prewait(void *p_context)
{
	popupanswerdialog_t *ctxt;
	ctxt = (popupanswerdialog_t *)p_context;
	
	NSString *t_title;
	t_title = [NSString stringWithMCStringRef: ctxt -> title == nil ? kMCEmptyString : ctxt -> title ];
	NSString *t_prompt;
	t_prompt = [NSString stringWithMCStringRef: ctxt -> message == nil ? kMCEmptyString : ctxt -> message ];
	
	ctxt -> delegate = [[ModalDelegate alloc] init];
	ctxt -> alert_view = [[UIAlertView alloc] initWithTitle:t_title message:t_prompt delegate:ctxt -> delegate cancelButtonTitle:nil otherButtonTitles:nil];
	
	if (ctxt -> button_count == 0)
	{
		[ctxt -> delegate setIndex: 0];
		[ctxt -> alert_view addButtonWithTitle: @"OK"];
	}
	else
	{
		[ctxt -> delegate setIndex: ctxt -> button_count - 1];
		for(uint32_t i = 0; i < ctxt -> button_count; i++)
			[ctxt -> alert_view addButtonWithTitle: [ NSString stringWithMCStringRef: ctxt -> buttons[i] ]];
	}
	[ctxt -> alert_view show];
	
}

static void dopopupanswerdialog_postwait(void *p_context)
{
	popupanswerdialog_t *ctxt;
	ctxt = (popupanswerdialog_t *)p_context;
	
	int32_t t_result;
	ctxt -> result = [ctxt -> delegate index];
	
	[ctxt -> delegate release];
	[ctxt -> alert_view release];
}

int32_t MCScreenDC::popupanswerdialog(MCStringRef p_buttons[], uint32_t p_button_count, uint32_t p_type, MCStringRef p_title, MCStringRef p_message)
{
	// MW-2010-12-18: You cannot nest alertviews on iOS, so we return an immediate cancel if we are in one
	if (s_in_modal)
		return -1;

	popupanswerdialog_t ctxt;
    /* UNCHECKED */ MCMemoryAllocate(p_button_count * sizeof(MCStringRef), ctxt . buttons);
	ctxt . buttons = p_buttons;
	for (uindex_t i = 0; i < p_button_count; i++)
		ctxt . buttons[i] = MCValueRetain(p_buttons[i]);
	ctxt . button_count = p_button_count;
	ctxt . type = p_type;
    // may be nil
	ctxt . title = p_title;
	ctxt . message = p_message;
	
	MCIPhoneRunOnMainFiber(dopopupanswerdialog_prewait, &ctxt);
	
	s_in_modal = true;
	while(s_in_modal)
		MCscreen -> wait(1.0, True, True);
	
	MCIPhoneRunOnMainFiber(dopopupanswerdialog_postwait, &ctxt);
	
    for (uindex_t i = 0; i < p_button_count; i++)
		MCValueRelease(ctxt . buttons[i]);
    /* UNCHECKED */ MCMemoryDeallocate(ctxt . buttons);
    
    // dont release, owned by the caller
    //MCValueRelease(ctxt . title);
    //MCValueRelease(ctxt . message);
	return ctxt . result;
}

#define kUITextFieldHeight 30.0
#define kUITextFieldXPadding 12.0
#define kUIAlertOffset 100.0

@interface TextAlertView : UIAlertView
{
	NSInteger m_index;
	UITextField *m_textResult;
	uint32_t m_adjust_count;
	BOOL m_initially_landscape;
}

- (id)initWithTitle:(NSString *)title message:(NSString *)message delegate:(id)delegate type:(uint32_t)p_type
  cancelButtonTitle:(NSString *)cancelButtonTitle otherButtonTitles:(NSString *)otherButtonTitles, ...;

- (UITextField *)textField;

@end

@implementation TextAlertView

// UIAlertView/TextAlertView delegate method(s) 
- (void)alertView:(UIAlertView *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
	s_in_modal = false;
	m_index = buttonIndex;
}

//	Initialize view with maximum of two buttons
- (id)initWithTitle:(NSString *)p_title 
			message:(NSString *)p_message
		   delegate:(id)p_delegate
			   type:(uint32_t)p_type
  cancelButtonTitle:(NSString *)p_cancelButtonTitle 
  otherButtonTitles:(NSString *)p_otherButtonTitles, ...
{
	self = [super initWithTitle:p_title 
						message:p_message
					   delegate:p_delegate
			  cancelButtonTitle:nil
			  otherButtonTitles:nil];
			[self addButtonWithTitle:p_cancelButtonTitle];
			[self addButtonWithTitle:p_otherButtonTitles];
	if (self)
	{
		// Create and add UITextField to UIAlertView
		UITextField *t_textField = [[[UITextField alloc] initWithFrame:CGRectZero] retain];
		
		t_textField.alpha = 0.75;
		t_textField.borderStyle = UITextBorderStyleBezel;
		t_textField.backgroundColor = [UIColor whiteColor];
        
		m_textResult = t_textField;
		
		// set up the input mode, either password or not
		if (p_type == AT_PASSWORD)
			t_textField.secureTextEntry = YES;
		else
			t_textField.secureTextEntry = NO;
		
		// insert UITextField before first button
		BOOL t_inserted = NO;
		for (UIView *t_view in self.subviews)
		{
			if (!t_inserted && ![t_view isKindOfClass:[UILabel class]])
			{
				[self insertSubview:t_textField belowSubview:t_view];
				break;
			}
		}
		
		m_adjust_count = 0;
		m_initially_landscape = NO;
	}
	return self;
}

// Show alert view and make keyboard visible
- (void) show
{
	[super show];
	[m_textResult becomeFirstResponder];
}

// Override layoutSubviews to correctly handle the UITextField
- (void)layoutSubviews
{
	[super layoutSubviews];
	
	CGRect t_bounds;
	t_bounds = [self bounds];
	
	CGFloat t_labels_top, t_labels_bottom, t_buttons_top, t_buttons_bottom;
	t_labels_top = FLT_MAX;
	t_labels_bottom = 0.0f;
	t_buttons_top = FLT_MAX;
	t_buttons_bottom = 0.0f;
	
	bool t_found_textview;
	t_found_textview = false;

	// Loop through all subclasses to determine the required parameter values for positioning them
	for(UIView *t_view in self.subviews)
	{
		if ([t_view isKindOfClass: [UILabel class]])
		{
			CGRect t_frame;
			t_frame = [t_view frame];
			if (t_frame . origin . y < t_labels_top)
				t_labels_top = t_frame . origin . y;
			else if (t_frame . origin . y + t_frame . origin . y + t_frame . size . height > t_labels_bottom)
				t_labels_bottom = t_frame . origin . y + t_frame . size . height;
		}
		else if ([t_view isKindOfClass: [UITextField class]])
		{
			t_found_textview = true;
		}
		else if (t_found_textview)
		{
			CGRect t_frame;
			t_frame = [t_view frame];
			if (t_frame . origin . y < t_buttons_top)
				t_buttons_top = t_frame . origin . y;
			else if (t_frame . origin . y + t_frame . origin . y + t_frame . size . height > t_buttons_bottom)
				t_buttons_bottom = t_frame . origin . y + t_frame . size . height;
		}
	}
	
	CGFloat t_label_margin, t_label_end;
	t_label_margin = t_labels_top;
	t_label_end = t_labels_bottom;
	
	t_bounds . size . height += (2 * t_label_margin + kUITextFieldHeight) - (t_buttons_top - t_labels_bottom);
	[self setBounds: t_bounds];
	
	t_found_textview = false;

	// set the position of the relevant subclasses
	for(UIView *t_view in self.subviews)
	{
		if ([t_view isKindOfClass: [UITextField class]])
		{
			t_found_textview = true;
			
			CGRect t_viewFrame = CGRectMake(kUITextFieldXPadding, t_label_end + t_label_margin, 
											t_bounds.size.width - 2.0 * kUITextFieldXPadding, kUITextFieldHeight);
			[t_view setFrame:t_viewFrame];
		}
		else if (t_found_textview)
		{
			CGRect t_frame;
			t_frame = [t_view frame];
			t_frame . origin . y = t_label_end + 2 * t_label_margin + kUITextFieldHeight;
			[t_view setFrame: t_frame];
		}
	}
}

- (NSInteger)index
{
	return m_index;
}

- (void)setIndex: (NSInteger)p_index
{
	m_index = p_index;
}

- (const char *)getText
{
	return [m_textResult.text cStringUsingEncoding:NSMacOSRomanStringEncoding];
}

- (UITextField *)textField
{
	return m_textResult;
}

@end

struct popupaskdialog_t
{
	uint32_t type;
	MCStringRef title;
	MCStringRef message;
	MCStringRef initial;
	bool hint;
	char *result;
	
	TextAlertView *alert;
	ModalDelegate *delegate;
	UIAlertView *alert_view;
	UITextField *text_field;
};

static void dopopupaskdialog_prewait(void *p_context)
{
	popupaskdialog_t *ctxt;
	ctxt = (popupaskdialog_t *)p_context;
	
    NSString *t_title;
	t_title = [NSString stringWithMCStringRef: (ctxt -> title == nil ? kMCEmptyString : ctxt -> title) ];
	NSString *t_message;
	t_message = [NSString stringWithMCStringRef: (ctxt -> message == nil ? kMCEmptyString : ctxt -> message) ];
	
    // MM-2012-03-14: [[ Bug 10084 ]] Intial text was being set to space by default meaning a space was prepended to anby data returned.
	NSString *t_initial;
	t_initial = [NSString stringWithMCStringRef: (ctxt -> initial == nil ? kMCEmptyString : ctxt -> initial) ];
    
	ctxt -> delegate = [[ModalDelegate alloc] init];
		
	UITextField *t_text_field;
	UIAlertView *t_alert;
    if (MCmajorosversion < 500)
    {        
        ctxt-> alert = [[TextAlertView alloc] initWithTitle:t_title 
                                                              message:t_message
                                                             delegate:ctxt -> delegate
                                                                 type:ctxt -> type
                                                    cancelButtonTitle:@"Cancel"
                                                    otherButtonTitles:@"OK", nil];

		t_text_field = [ctxt -> alert textField];
		t_alert = ctxt -> alert;
	}
	else
	{
#ifdef __IPHONE_5_0
        ctxt -> alert_view = [[UIAlertView alloc] initWithTitle:t_title 
                                                  message:t_message
                                                 delegate:ctxt -> delegate
                                        cancelButtonTitle:nil
                                        otherButtonTitles:nil];
        if (ctxt -> type == AT_PASSWORD)
            [ctxt -> alert_view setAlertViewStyle:UIAlertViewStyleSecureTextInput];
        else
            [ctxt -> alert_view setAlertViewStyle:UIAlertViewStylePlainTextInput];
        [ctxt -> alert_view addButtonWithTitle:@"Cancel"];
        [ctxt -> alert_view addButtonWithTitle:@"OK"];
        
        ctxt -> text_field = [ctxt -> alert_view textFieldAtIndex:0];
		
		t_text_field = ctxt -> text_field;        
		t_alert = ctxt -> alert_view;
#endif		
	}
	
	// MW-2012-10-12: [[ Bug 10175 ]] Refactored textField manipulations.
	if (t_text_field != nil)
	{
		[ctxt -> delegate setView: t_alert];
		
		[t_text_field setAutocorrectionType:UITextAutocorrectionTypeNo];
		[t_text_field setKeyboardType:MCIPhoneGetKeyboardType()];
		// MW-2012-10-12: [[ Bug 10377 ]] If we want a hint, set placeholder not text.
		if (ctxt -> hint)
			[t_text_field setPlaceholder:t_initial];
		else
			[t_text_field setText:t_initial];
		[t_text_field setDelegate: ctxt -> delegate];
	}

	[t_alert show];
}

static void dopopupaskdialog_postwait(void *p_context)
{
	popupaskdialog_t *ctxt;
	ctxt = (popupaskdialog_t *)p_context;
	
	if (MCmajorosversion < 500)
	{
        const char *t_messageText;
        
        t_messageText = [ctxt -> alert getText];
        
		// MW-2012-10-24: [[ Bug 10491 ]] The delegate now holds the button index, not the alert view.
        if ([ctxt -> delegate index] == 0 || t_messageText == nil || *t_messageText == '\0')
            ctxt -> result = nil;
        else
        {
            ctxt -> result = (char *) malloc (strlen (t_messageText) + 1);
            strcpy (ctxt -> result, t_messageText);
        }
        
        [ctxt -> alert release];
	}
	else
	{
#ifdef __IPHONE_5_0
        const char* t_message_text;
        if (ctxt -> text_field != nil)
            t_message_text = [[ctxt -> text_field text] cStringUsingEncoding:NSMacOSRomanStringEncoding];
        else
            t_message_text = nil;
        
        if ([ctxt -> delegate index] == 0 || t_message_text == nil || *t_message_text == '\0')
            ctxt -> result = nil;
        else
        {
            MCMemoryAllocate(MCCStringLength(t_message_text) + 1, ctxt -> result);
            MCCStringClone(t_message_text, ctxt -> result);
        }
        
        [ctxt -> delegate release];
        [ctxt -> alert_view release];
#endif		
	}
}

// MM-2011-09-20: [[ BZ 9730 ]] Updated ask dialogs for iOS 5 to use updated UIALertView features. Fixes layout bug in iOS 5.
bool MCScreenDC::popupaskdialog(uint32_t p_type, MCStringRef p_title, MCStringRef p_message, MCStringRef p_initial, bool p_hint, MCStringRef& r_result)
{
	if (s_in_modal)
		return nil;
	
	popupaskdialog_t ctxt;
	ctxt . type = p_type;
	ctxt . title = MCValueRetain(p_title);
	ctxt . message = MCValueRetain(p_message);
	ctxt . initial = MCValueRetain(p_initial);
	ctxt . hint = p_hint;
 
	MCIPhoneRunOnMainFiber(dopopupaskdialog_prewait, &ctxt);

	s_in_modal = true;
	while(s_in_modal)
		MCscreen -> wait(1.0, True, True);

	MCIPhoneRunOnMainFiber(dopopupaskdialog_postwait, &ctxt);
	
    MCValueRelease(ctxt . title);
    MCValueRelease(ctxt . message);
    MCValueRelease(ctxt . initial);
	return MCStringCreateWithCString(ctxt . result, r_result);
}

////////////////////////////////////////////////////////////////////////////////
