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

#include "date.h"
#include "mblsyntax.h"

#import <UIKit/UIKit.h>
#include "mbliphoneapp.h"

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
float MCIPhoneGetNativeControlScale(void);

UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////
@interface MCIPhonePickDateWheelDelegate : UIViewController <UIPickerViewDelegate, UIPickerViewDataSource, UIActionSheetDelegate, UITableViewDelegate> 
{
	bool iSiPad;
	bool m_running;
	bool m_selection_made;
    // HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed m_selected_date from uint to int.
	int32_t m_selected_date;
	UIActionSheet *actionSheet;
	UIPopoverController* popoverController;
	UIDatePicker *datePicker;
}

@end

@implementation MCIPhonePickDateWheelDelegate

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	iSiPad = false;
	m_running = false;
	m_selection_made = false;
	m_selected_date = 0;
	actionSheet = nil;
	popoverController = nil;
	return self;
}

- (void)dealloc
{
	[datePicker removeFromSuperview];
	[actionSheet release];
	[datePicker release];
	[super dealloc];
}

- (void)viewDidLoad
{
	[super viewDidLoad];
}

- (bool)running
{
	return m_running;
}

- (void)datePicker:(UIDatePicker *)datePicker didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	// return the object that has just been selected
	m_selection_made = true;
}

// HC-2011-09-28 [[ Picker Buttons ]] Added arguments to force the display of buttons
// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed r_current, p_start and p_end from uint to int.
- (void) startDatePicker:(NSString *)p_style andCurrent: (int32_t)r_current andStart: (int32_t)p_start andEnd: (int32_t)p_end andStep: (uint32_t)p_step andCancel: (bool)p_use_cancel andDone: (bool)p_use_done andButtonRect: (MCRectangle) p_button_rect
{	
	// set up the wait status
	m_running = true;
	// set up local variables for all supported devices here to prevent scoping issues in the if statement
	UISegmentedControl *t_closeButton;
	
	id t_popover = nil;
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
		t_popover = NSClassFromString(@"UIPopoverController");
	// create an action sheet on the iPhone/iPod and a popover on the iPad
	if (t_popover != nil)
	{
		iSiPad = true;
		// HC-2011-10-03 [[ Picker Buttons ]] Severl lines of the content in this part of the if statement has changed.
		// create the date pick wheel
		// the y-coordinate depends on whether or not we are displaying buttons
		if (p_use_done || p_use_cancel)
			datePicker = [[UIDatePicker alloc] initWithFrame: CGRectMake(0, 45, 320, 216)];
		else
			datePicker = [[UIDatePicker alloc] initWithFrame: CGRectMake(0, 0, 320, 216)];
		
		// set the locale
		NSLocale *t_locale = [NSLocale currentLocale];
		NSDateFormatter *t_dateFormatter = [[NSDateFormatter alloc] init]; 
		[t_dateFormatter setLocale:t_locale];
		[datePicker setLocale:t_locale];
		[datePicker setCalendar:[t_locale objectForKey:NSLocaleCalendar]];
		[datePicker setTimeZone:[NSTimeZone localTimeZone]];
		// set up the style and parameters for the date picker
		if (p_style == nil || MCCStringEqual([p_style cStringUsingEncoding:NSMacOSRomanStringEncoding], "dateTime"))
			[datePicker setDatePickerMode:UIDatePickerModeDateAndTime];
		else if	(MCCStringEqual([p_style cStringUsingEncoding:NSMacOSRomanStringEncoding], "date"))
			[datePicker setDatePickerMode:UIDatePickerModeDate];
		else
			[datePicker setDatePickerMode:UIDatePickerModeTime];
		
		// set up the current date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (r_current != 0)
            [datePicker setDate:[NSDate dateWithTimeIntervalSince1970:r_current] animated:YES];
		
		// set up the start date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (p_start != 0)
            [datePicker setMinimumDate:[NSDate dateWithTimeIntervalSince1970:p_start]];
		
		// set up the end date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (p_end != 0)
            [datePicker setMaximumDate:[NSDate dateWithTimeIntervalSince1970:p_end]];
		
		// set up the minute intervale of the datePicker display
		if (p_step > 0)
			[datePicker setMinuteInterval:p_step];
		
		[datePicker addTarget:self
					   action:@selector(dateChanged:)
			 forControlEvents:UIControlEventValueChanged];

		// make a toolbar
		UIToolbar *t_toolbar;
		NSMutableArray *t_toolbar_items;
		// HC-2011-10-03 [[ Picker Buttons ]] Only add a tool bar if we are going to use it.
		if (p_use_done || p_use_cancel)
		{
			t_toolbar = [[UIToolbar alloc] initWithFrame: CGRectMake(0, 0, 320, 44)];
			t_toolbar.barStyle = UIBarStyleBlack;
			t_toolbar.translucent = YES;
		
			t_toolbar_items = [[NSMutableArray alloc] init];
			// HC-2011-09-28 [[ Picker Buttons ]] Enable cancel button on request.
			if (p_use_cancel) 
				[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemCancel target: self action: @selector(cancelDatePickWheel:)]];
			[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace target: self action: nil]];
			// HC-2011-09-28 [[ Picker Buttons ]] Enable done button on request.
			if (p_use_done) 
				[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemDone target: self action: @selector(dismissDatePickWheel:)]];
		
			[t_toolbar setItems: t_toolbar_items animated: NO];
		}
		// create the action sheet that can contain the "Cancel" and "Done" buttons and date pick wheel
		actionSheet = [[UIActionSheet alloc] initWithTitle:nil
												  delegate:self
										 cancelButtonTitle:nil
									destructiveButtonTitle:nil
										 otherButtonTitles:nil];
		
		// set the style of the acionsheet
		[actionSheet setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		
		// add the subviews to the action sheet
		[actionSheet addSubview: datePicker];
		if (p_use_done || p_use_cancel)
		{
			[actionSheet addSubview: t_toolbar];
			[t_toolbar release];
		}	
        
		// set up the bounding box of the popover 
		// the height depends on whether or not we are displaying buttons
		if (p_use_done || p_use_cancel)
			self.contentSizeForViewInPopover = CGSizeMake(320,261);
		else
			self.contentSizeForViewInPopover = CGSizeMake(320,216);
		// create the popover controller
		popoverController = [[t_popover alloc] initWithContentViewController:self];
		[popoverController presentPopoverFromRect:MCUserRectToLogicalCGRect(p_button_rect)
										   inView:MCIPhoneGetView()
						 permittedArrowDirections:UIPopoverArrowDirectionAny
										 animated:YES];
		// need to make self as delegate otherwise overridden delegates are not called
		popoverController.delegate = self;
	}
	else
	{
		iSiPad = false;
		
		// compute orientation
		bool t_is_landscape;
		t_is_landscape = UIInterfaceOrientationIsLandscape(MCIPhoneGetOrientation());
		
		// create the pick wheel
		datePicker = [[UIDatePicker alloc] initWithFrame: CGRectMake(0, (t_is_landscape ? 32 : 44), 0, 0)];

		// set the locale
		NSLocale *t_locale = [NSLocale currentLocale];
		NSDateFormatter *t_dateFormatter = [[NSDateFormatter alloc] init]; 
		[t_dateFormatter setLocale:t_locale];
		[datePicker setLocale:t_locale];
		[datePicker setCalendar:[t_locale objectForKey:NSLocaleCalendar]];
		[datePicker setTimeZone:[NSTimeZone localTimeZone]];
		
		// set up the style and parameters for the date picker
		if (p_style == nil || MCCStringEqual([p_style cStringUsingEncoding:NSMacOSRomanStringEncoding], "dateTime"))
			[datePicker setDatePickerMode:UIDatePickerModeDateAndTime];
		else if	(MCCStringEqual([p_style cStringUsingEncoding:NSMacOSRomanStringEncoding], "date"))
			[datePicker setDatePickerMode:UIDatePickerModeDate];
		else
			[datePicker setDatePickerMode:UIDatePickerModeTime];
		
		// set up the current date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (r_current != 0)
            [datePicker setDate:[NSDate dateWithTimeIntervalSince1970:r_current] animated:YES];

		// set up the start date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (p_start != 0)
            [datePicker setMinimumDate:[NSDate dateWithTimeIntervalSince1970:p_start]];
		
		// set up the end date of the datePicker display
		// HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed if condition to allow for values other than 0.
        if (p_end != 0)
            [datePicker setMaximumDate:[NSDate dateWithTimeIntervalSince1970:p_end]];
		
		// set up the minute intervale of the datePicker display
		if (p_step > 0)
			[datePicker setMinuteInterval:p_step];
		
		// make a toolbar
        // MM-2012-10-15: [[ Bug 10463 ]] Make the picker scale to the width of the device rather than a hard coded value (fixes issue with landscape iPhone 5 being 568 not 480).
		UIToolbar *t_toolbar;
        t_toolbar = [[UIToolbar alloc] initWithFrame: (t_is_landscape ? CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . height, 32) : CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, 44))];
		t_toolbar.barStyle = UIBarStyleBlack;
		t_toolbar.translucent = YES;
		[t_toolbar sizeToFit];
		
		NSMutableArray *t_toolbar_items;
		t_toolbar_items = [[NSMutableArray alloc] init];
		// HC-2011-09-28 [[ Picker Buttons ]] Enable cancel button on request.
		if (r_current == 0 || p_use_cancel) 
			[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemCancel target: self action: @selector(cancelDatePickWheel:)]];
		[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace target: self action: nil]];
		[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemDone target: self action: @selector(dismissDatePickWheel:)]];
		
		[t_toolbar setItems: t_toolbar_items animated: NO];
		
		// create the action sheet that contains the "Done" button and date pick wheel
		actionSheet = [[UIActionSheet alloc] initWithTitle:nil
												  delegate:self
										 cancelButtonTitle:nil
									destructiveButtonTitle:nil
										 otherButtonTitles:nil];
		
		// set the style of the acionsheet
		[actionSheet setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		
		// add the subviews to the action sheet
		[actionSheet addSubview: t_toolbar];
		[actionSheet addSubview: datePicker];
		[t_toolbar release];
	
		// initialize the date pick wheel
		[actionSheet showInView: MCIPhoneGetView()];
		
		// set up the bounding box of the action sheet
        // MM-2012-10-15: [[ Bug 10463 ]] Make the picker scale to the width of the device rather than a hard coded value (fixes issue with landscape iPhone 5 being 568 not 480).
		if (!t_is_landscape)
			[actionSheet setBounds:CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, 496)];
		else
			[actionSheet setBounds:CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . height, 365)];
	}
}

- (int32_t)finishDatePicker
{
	uint32_t t_date;
	t_date = [[datePicker date] timeIntervalSince1970];
	
	// device specific release
	// HC-2011-10-03 [[ Picker Buttons ]] Added buttons to iPad via action sheet.
	if (iSiPad)
		[popoverController release];

	if (m_selection_made)
		return m_selected_date;

	return 0;
}

-(void)dateChanged:(id)sender
{
	NSDate *t_ns_date;
	t_ns_date = [sender date];
	int t_date;
	t_date = [t_ns_date timeIntervalSince1970];
	NSLog(@"Date = %d\n", t_date);
}

// called when the action sheet is dispmissed (iPhone, iPod)
- (void)dismissDatePickWheel:(NSObject *)controlButton
{
	// dismiss the action sheet programmatically
	m_selection_made = true;
	m_selected_date = [[datePicker date] timeIntervalSince1970];
	[actionSheet dismissWithClickedButtonIndex:0 animated:YES];
	[popoverController dismissPopoverAnimated:YES];
}

- (void)cancelDatePickWheel: (NSObject *)sender
{
	m_selection_made = false;
	m_selected_date = 0;
	[actionSheet dismissWithClickedButtonIndex:0 animated:YES];
	[popoverController dismissPopoverAnimated:YES];
}

- (void)actionSheet: (UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

// called when the popover controller is dismissed (iPad)
- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController
{
	m_running = false;
	m_selection_made = true;
	m_selected_date = [[datePicker date] timeIntervalSince1970];
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

// called when including the UIDatePicker in the UIViewController
- (void)loadView
{
	// HC-2011-10-03 [[ Picker Buttons ]] Added buttons to iPad via action sheet.
	if (iSiPad)
		self.view = actionSheet;
	else
		self.view = datePicker;
}

- (bool)canceled
{
	// HC-2011-10-03 [[ Picker Buttons ]] Added buttons to iPad via action sheet.
	return !m_selection_made;
}

@end

static MCIPhonePickDateWheelDelegate *s_pick_date_wheel_delegate = nil;


////////////////////////////////////////////////////////////////////////////////

struct datepicker_t
{
	NSString *style;
	int32_t current;
	int32_t min;
	int32_t max;
	int32_t step;
	bool use_cancel;
	bool use_done;
	MCRectangle button_rect;
	MCIPhonePickDateWheelDelegate *picker;
	bool cancelled;
};

static void pickdate_prewait(void *p_context)
{
	datepicker_t *ctxt;
	ctxt = (datepicker_t *)p_context;
	
	ctxt -> picker = [[MCIPhonePickDateWheelDelegate alloc] init];
	
	// MW-2012-09-21: [[ Bug 10402 ]] Make sure we pass the 'step' parameter through.
	[ctxt -> picker startDatePicker: ctxt -> style andCurrent: ctxt -> current andStart: ctxt -> min andEnd: ctxt -> max andStep: ctxt -> step andCancel: ctxt -> use_cancel andDone: ctxt -> use_done andButtonRect: ctxt -> button_rect];
}

static void pickdate_postwait(void *p_context)
{
	datepicker_t *ctxt;
	ctxt = (datepicker_t *)p_context;

	ctxt -> current = [ctxt -> picker finishDatePicker];
	ctxt -> cancelled = [ctxt -> picker canceled];
	[ctxt -> picker release];
}

bool MCSystemPickDate(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecPoint ep(nil, nil, nil);
    
    // Get the display style
    NSString *t_style;
    t_style = [NSString stringWithCString: "date" encoding: NSMacOSRomanStringEncoding];
    
    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_current))
            return false;
        r_current = ep.getnvalue();
    }

    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_min))
            return false;
        t_min = ep.getnvalue();
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_max))
            return false;
        t_max = ep.getnvalue();
    } 

	datepicker_t ctxt;
	ctxt . style = t_style;
	ctxt . current = r_current;
	ctxt . min = t_min;
	ctxt . max = t_max;
	ctxt . step = 0;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = ctxt . cancelled;
    if (!r_canceled)
    {
        // Convert the seconds to date time
        ep.setnvalue(ctxt . current);
        if (!MCD_convert_to_datetime(ep, CF_SECONDS, CF_SECONDS, *r_result))
            return false;
    }
    return true;
}

bool MCSystemPickTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecPoint ep(nil, nil, nil);

    NSString *t_style;
    t_style = [NSString stringWithCString: "time" encoding: NSMacOSRomanStringEncoding];

    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_current))
            return false;
        r_current = ep.getnvalue();
    }
    
    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_min))
            return false;
        t_min = ep.getnvalue();
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_max))
            return false;
        t_max = ep.getnvalue();
    } 
    
	datepicker_t ctxt;
	ctxt . style = t_style;
	ctxt . current = r_current;
	ctxt . min = t_min;
	ctxt . max = t_max;
	ctxt . step = p_step;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = ctxt . cancelled;    
    if (!r_canceled)
    {
        // Convert the seconds to date time
        ep.setnvalue(ctxt . current);
        if (!MCD_convert_to_datetime(ep, CF_SECONDS, CF_SECONDS, *r_result))
            return false;
    }
	return true;
}

bool MCSystemPickDateAndTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecPoint ep(nil, nil, nil);

    NSString *t_style;
    t_style = [NSString stringWithCString: "dateTime" encoding: NSMacOSRomanStringEncoding];

    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_current))
            return false;
        r_current = ep.getnvalue();
    }
    
    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_min))
            return false;
        t_min = ep.getnvalue();
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
        if (!MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, *p_max))
            return false;
        t_max = ep.getnvalue();
    } 
    
	datepicker_t ctxt;
	ctxt . style = t_style;
	ctxt . current = r_current;
	ctxt . min = t_min;
	ctxt . max = t_max;
	ctxt . step = p_step;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = ctxt . cancelled;
    if (!r_canceled)
    {
        // Convert the seconds to date time
        ep.setnvalue(ctxt . current);
        if (!MCD_convert_to_datetime(ep, CF_SECONDS, CF_SECONDS, *r_result))
            return false;
    }
	return true;
}

