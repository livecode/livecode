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

#include "date.h"
#include "mblsyntax.h"

#import <UIKit/UIKit.h>
#include "mbliphoneapp.h"

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
float MCIPhoneGetNativeControlScale(void);

UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

// MM-2013-09-23: [[ iOS7 Support ]] Added missing delegates implemented in order to appease llvm 5.0.
@interface com_runrev_livecode_MCIPhonePickDateWheelDelegate : UIViewController <UIPickerViewDelegate, UIPickerViewDataSource, UIActionSheetDelegate, UITableViewDelegate, UIPopoverControllerDelegate>
{
	bool iSiPad;
	bool m_running;
	bool m_selection_made;
    // HC-2011-11-01 [[ BUG 9848 ]] Date Picker does not work with dates before the UNIX epoch. Changed m_selected_date from uint to int.
	int32_t m_selected_date;
	UIActionSheet *actionSheet;
	UIPopoverController* popoverController;
	UIDatePicker *datePicker;
    UIView *m_action_sheet_view;
    UIControl *m_blocking_view;
    bool m_should_show_keyboard;
}

@end

@implementation com_runrev_livecode_MCIPhonePickDateWheelDelegate

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
    m_action_sheet_view = nil;
    m_blocking_view = nil;
    m_should_show_keyboard = false;
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
        
        // need to make self as delegate otherwise overridden delegates are not called
        popoverController.delegate = self;
        
        [popoverController setPopoverContentSize:self.contentSizeForViewInPopover];

		[popoverController presentPopoverFromRect:MCUserRectToLogicalCGRect(p_button_rect)
										   inView:MCIPhoneGetView()
						 permittedArrowDirections:UIPopoverArrowDirectionAny
                                         animated:YES];
	}
	else
	{
		iSiPad = false;
		
		// compute orientation
		bool t_is_landscape;
		t_is_landscape = UIInterfaceOrientationIsLandscape(MCIPhoneGetOrientation());
        
        // PM-2015-03-25: [[ Bug 15070 ]] The actionSheet that contains the datePickWheel should be of fixed height
        CGFloat t_toolbar_portrait_height, t_toolbar_landscape_height;
        t_toolbar_portrait_height = 44;
        t_toolbar_landscape_height = 32;
		
		// create the pick wheel
        // If you create an instance with a width and height of 0, they will be overridden with the appropriate default width and height, which you can get by frame.size.width/height.
		datePicker = [[UIDatePicker alloc] initWithFrame: CGRectMake(0, (t_is_landscape ? t_toolbar_landscape_height : t_toolbar_portrait_height), 0, 0)];

		// set the locale
		NSLocale *t_locale = [NSLocale currentLocale];
		NSDateFormatter *t_dateFormatter = [[NSDateFormatter alloc] init]; 
		[t_dateFormatter setLocale:t_locale];
		[datePicker setLocale:t_locale];
		[datePicker setCalendar:[t_locale objectForKey:NSLocaleCalendar]];
		[datePicker setTimeZone:[NSTimeZone localTimeZone]];
        
       
        
        // PM-2014-10-22: [[ Bug 13750 ]] Make sure the view under the pickerView is not visible (iphone 4 only)
        NSString *t_device_model_name = MCIPhoneGetDeviceModelName();
        if ([t_device_model_name isEqualToString:@"iPhone 4"] || [t_device_model_name isEqualToString:@"iPhone 4(Rev A)"] || [t_device_model_name isEqualToString:@"iPhone 4(CDMA)"])
            datePicker.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.90];
		
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
        
        if (t_is_landscape)
            t_toolbar = [[UIToolbar alloc] initWithFrame: (CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_landscape_height))];
        else
            t_toolbar = [[UIToolbar alloc] initWithFrame: (CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_portrait_height))];
        
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
		
        if (MCmajorosversion < 800)
        {
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
        
        else
        {
            // PM-2014-09-25: [[ Bug 13484 ]] In iOS 8 and above, UIActionSheet is not working correctly
            
            CGRect t_rect;
            // PM-2015-04-13: [[ Bug 15070 ]] There are only three valid values (162.0, 180.0 and 216.0) for the height of the picker
            uint2 t_pick_wheel_height = datePicker.frame.size.height;
            
            if (!t_is_landscape)
            {
                [datePicker setFrame:CGRectMake(0, t_toolbar_portrait_height, [[UIScreen mainScreen] bounds] . size . width, t_pick_wheel_height)];
                t_rect = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_portrait_height - t_pick_wheel_height, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_portrait_height +t_pick_wheel_height);
            }
            else
            {
                [datePicker setFrame:CGRectMake(0, t_toolbar_landscape_height, [[UIScreen mainScreen] bounds] . size . width, t_pick_wheel_height)];
                t_rect = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_landscape_height - t_pick_wheel_height, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_landscape_height + t_pick_wheel_height);
            }
            
            m_action_sheet_view = [[UIView alloc] initWithFrame:t_rect];
            
            [m_action_sheet_view addSubview: t_toolbar];
            [m_action_sheet_view addSubview: datePicker];
			if ([UIColor respondsToSelector:@selector(secondarySystemBackgroundColor)])
			{
				m_action_sheet_view.backgroundColor = [UIColor secondarySystemBackgroundColor];
			}
			else
			{
				m_action_sheet_view.backgroundColor = [UIColor whiteColor];
			}
			[t_toolbar release];
            
            [MCIPhoneGetView() addSubview:m_action_sheet_view];
            
            // This is offscreen
            m_action_sheet_view.frame = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height , t_rect . size . width, t_rect. size . height);
            
            // Add animation to simulate old behaviour (slide from bottom)
            [UIView animateWithDuration:0.2 animations:^{m_action_sheet_view.frame = t_rect;}];
            
            
            // Add m_blocking_view to block any touch events in MCIPhoneGetView()
            CGRect t_blocking_rect;
            
            if (!t_is_landscape)
                t_blocking_rect = CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_portrait_height - t_pick_wheel_height);
            else
                t_blocking_rect = CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_landscape_height - t_pick_wheel_height);
            
            m_blocking_view = [[UIControl alloc] initWithFrame:t_blocking_rect];
            
            // Add effects and animation to simulate old behaviour
            m_blocking_view.backgroundColor = [UIColor blackColor];
            m_blocking_view.alpha = 0.4;
            m_blocking_view.userInteractionEnabled = YES;
            
#ifdef __IPHONE_8_0
            CATransition *applicationLoadViewIn =[CATransition animation];
            [applicationLoadViewIn setDuration:0.7];
            [applicationLoadViewIn setType:kCATransitionReveal];
            [applicationLoadViewIn setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn]];
            [[m_blocking_view layer]addAnimation:applicationLoadViewIn forKey:kCATransitionFromTop];
#endif
            // PM-2014-10-15: [[ Bug 13677 ]] If the keyboard is activated, hide it and show the picker. We should reactivate the keyboard once the picker hides
            if (MCIPhoneIsKeyboardVisible())
            {
                MCIPhoneDeactivateKeyboard();
                m_should_show_keyboard = true;
            }
            [MCIPhoneGetView() addSubview:m_blocking_view];
        }
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

- (NSInteger)getRoundedDate:(NSDate *)originalDate withStep:(NSInteger)step
{
    // The originalDate is not rounded down to the nearest "step" minutes.
    NSInteger t_original_date_in_seconds = [originalDate timeIntervalSince1970];
    
    // Get the "extra" minutes and convert them to seconds
    NSInteger t_extra_seconds = t_original_date_in_seconds % (step * 60);
    
    // Get the rounded date in seconds
    NSInteger t_seconds_rounded_down_to_step = t_original_date_in_seconds - t_extra_seconds;
        
    return t_seconds_rounded_down_to_step;
}

// called when the action sheet is dispmissed (iPhone, iPod)
- (void)dismissDatePickWheel:(NSObject *)controlButton
{
	// dismiss the action sheet programmatically
	m_selection_made = true;
    
    NSInteger t_step = [datePicker minuteInterval];
    m_selected_date = [self getRoundedDate: datePicker.date withStep:t_step];
    
    if (iSiPad)
    {
        [actionSheet dismissWithClickedButtonIndex:0 animated:YES];
        [popoverController dismissPopoverAnimated:YES];
    }
    else
    {
        // PM-2014-09-25: [[ Bug 13484 ]] In iOS 8 and above, UIActionSheet is not working properly
        if (MCmajorosversion >= 800)
        {
            [datePicker removeFromSuperview];
            
            [UIView animateWithDuration:0.5
                             animations:^{
                                 m_action_sheet_view.frame = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height, [[UIScreen mainScreen] bounds] . size . width, [[UIScreen mainScreen] bounds] . size . height);//move it out of screen
                             } completion:^(BOOL finished) {
                                 [m_action_sheet_view removeFromSuperview];
                             }];
            
            
            [m_action_sheet_view release];
            
            [m_blocking_view removeFromSuperview];
            [m_blocking_view release];
            
            m_running = false;
            MCscreen -> pingwait();
            
            // PM-2014-10-15: [[ Bug 13677 ]] Make sure we re-activate the keyboard if it was previously deactivated because of the picker
            if (m_should_show_keyboard)
            {
                // show the keyboard as in iOS 7
                [UIView animateWithDuration:0.9 animations:^{ MCIPhoneActivateKeyboard(); } completion:nil];
                m_should_show_keyboard = false;
            }
        }
        else
            [actionSheet dismissWithClickedButtonIndex:0 animated:YES];
    }
}

- (void)cancelDatePickWheel: (NSObject *)sender
{
	m_selection_made = false;
	m_selected_date = 0;
    
    if (iSiPad)
    {
        [actionSheet dismissWithClickedButtonIndex:0 animated:YES];
        [popoverController dismissPopoverAnimated:YES];
    }
    
    else
    {
        // PM-2014-09-25: [[ Bug 13484 ]] In iOS 8 and above, UIActionSheet is not working properly
        if (MCmajorosversion >= 800)
        {
            [datePicker removeFromSuperview];
            
            [UIView animateWithDuration:0.5
                             animations:^{
                                 m_action_sheet_view.frame = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height, [[UIScreen mainScreen] bounds] . size . width, [[UIScreen mainScreen] bounds] . size . height);//move it out of screen
                             } completion:^(BOOL finished) {
                                 [m_action_sheet_view removeFromSuperview];
                             }];
            
            
            [m_action_sheet_view release];
            
            [m_blocking_view removeFromSuperview];
            [m_blocking_view release];
            
            m_running = false;
            MCscreen -> pingwait();
            
            // PM-2014-10-15: [[ Bug 13677 ]] Make sure we re-activate the keyboard if it was previously deactivated because of the picker
            if (m_should_show_keyboard)
            {
                // show the keyboard as in iOS 7
                [UIView animateWithDuration:0.9 animations:^{ MCIPhoneActivateKeyboard(); } completion:nil];
                m_should_show_keyboard = false;
            }
        }
        else
            [actionSheet dismissWithClickedButtonIndex:0 animated:YES];
    }
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
    
    NSInteger t_step = [datePicker minuteInterval];
    m_selected_date = [self getRoundedDate: datePicker.date withStep:t_step];
    
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

static com_runrev_livecode_MCIPhonePickDateWheelDelegate *s_pick_date_wheel_delegate = nil;


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
	com_runrev_livecode_MCIPhonePickDateWheelDelegate *picker;
	bool cancelled;
};

static void pickdate_prewait(void *p_context)
{
	datepicker_t *ctxt;
	ctxt = (datepicker_t *)p_context;
	
	ctxt -> picker = [[com_runrev_livecode_MCIPhonePickDateWheelDelegate alloc] init];
	
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
	MCExecContext ctxt(nil, nil, nil);
    
    // Get the display style
    NSString *t_style;
    t_style = [NSString stringWithCString: "date" encoding: NSMacOSRomanStringEncoding];
    
    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        MCAutoValueRef t_date;
		if (!MCD_convert_from_datetime(ctxt, *p_current, CF_SECONDS, CF_SECONDS, &t_date))
            return false;
		
		if (!ctxt.ConvertToInteger(*t_date, r_current))
			return false;
    }

    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        MCAutoValueRef t_min_val;
		if (!MCD_convert_from_datetime(ctxt, *p_min, CF_SECONDS, CF_SECONDS, &t_min_val))
            return false;
		
		if (!ctxt.ConvertToInteger(*t_min_val, t_min))
			return false;
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
		MCAutoValueRef t_max_val;
		if (!MCD_convert_from_datetime(ctxt, *p_max, CF_SECONDS, CF_SECONDS, &t_max_val))
            return false;
        
		if (!ctxt.ConvertToInteger(*t_max_val, t_max))
			return false;
    } 

	datepicker_t date_ctxt;
	date_ctxt . style = t_style;
	date_ctxt . current = r_current;
	date_ctxt . min = t_min;
	date_ctxt . max = t_max;
	date_ctxt . step = 0;
	date_ctxt . use_cancel = p_use_cancel;
	date_ctxt . use_done = p_use_done;
	date_ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &date_ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([date_ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &date_ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = date_ctxt . cancelled;
    if (!r_canceled)
    {
        // Convert the seconds to date time
		MCAutoNumberRef t_secs;
		if (!MCNumberCreateWithInteger(date_ctxt.current, &t_secs))
			return false;
        
        if (!MCD_convert_to_datetime(ctxt, *t_secs, CF_SECONDS, CF_SECONDS, *r_result))
            return false;
    }
    return true;
}

bool MCSystemPickTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecContext ctxt(nil, nil, nil);

    NSString *t_style;
    t_style = [NSString stringWithCString: "time" encoding: NSMacOSRomanStringEncoding];

    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        MCAutoValueRef t_current;
		if (!MCD_convert_from_datetime(ctxt, *p_current, CF_SECONDS, CF_SECONDS, &t_current))
            return false;
        
		if (!ctxt.ConvertToInteger(*t_current, r_current))
			return false;
    }
    
    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        MCAutoValueRef t_min_val;
		if (!MCD_convert_from_datetime(ctxt, *p_min, CF_SECONDS, CF_SECONDS, &t_min_val))
			return false;
		
		if (!ctxt.ConvertToInteger(*t_min_val, t_min))
			return false;
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
        MCAutoValueRef t_max_val;
		if (!MCD_convert_from_datetime(ctxt, *p_max, CF_SECONDS, CF_SECONDS, &t_max_val))
			return false;
		
		if (!ctxt.ConvertToInteger(*t_max_val, t_max))
			return false;
    } 
    
	datepicker_t date_ctxt;
	date_ctxt . style = t_style;
	date_ctxt . current = r_current;
	date_ctxt . min = t_min;
	date_ctxt . max = t_max;
	date_ctxt . step = p_step;
	date_ctxt . use_cancel = p_use_cancel;
	date_ctxt . use_done = p_use_done;
	date_ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &date_ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([date_ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &date_ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = date_ctxt . cancelled;    
    if (!r_canceled)
    {
        // Convert the seconds to date time
		MCAutoNumberRef t_number;
		if (!MCNumberCreateWithInteger(date_ctxt.current, &t_number))
			return false;
		
		if (!MCD_convert_to_datetime(ctxt, *t_number, CF_SECONDS, CF_SECONDS, *r_result))
			return false;
    }
	return true;
}

bool MCSystemPickDateAndTime(MCDateTime *p_current, MCDateTime *p_min, MCDateTime *p_max, int32_t p_step, bool p_use_cancel, bool p_use_done, MCDateTime *r_result, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecContext ctxt(nil, nil, nil);

    NSString *t_style;
    t_style = [NSString stringWithCString: "dateTime" encoding: NSMacOSRomanStringEncoding];

    // Convert the current datatime to seconds
    int32_t r_current;
    if (p_current == nil)
        r_current = 0;
    else
    {
        MCAutoValueRef t_current;
		if (!MCD_convert_from_datetime(ctxt, *p_current, CF_SECONDS, CF_SECONDS, &t_current))
            return false;
        
		if (!ctxt.ConvertToInteger(*t_current, r_current))
			return false;
    }
    
    // Convert the min datatime to seconds
    int32_t t_min;
    if (p_min == nil)
        t_min = 0;
    else
    {
        MCAutoValueRef t_min_val;
		if (!MCD_convert_from_datetime(ctxt, *p_min, CF_SECONDS, CF_SECONDS, &t_min_val))
			return false;
		
		if (!ctxt.ConvertToInteger(*t_min_val, t_min))
			return false;
    }
    
    // Convert the max datatime to seconds
    int32_t t_max;
    if (p_max == nil)
        t_max = 0;
    else
    {
        MCAutoValueRef t_max_val;
		if (!MCD_convert_from_datetime(ctxt, *p_max, CF_SECONDS, CF_SECONDS, &t_max_val))
			return false;
		
		if (!ctxt.ConvertToInteger(*t_max_val, t_max))
			return false;
    } 
    
	datepicker_t date_ctxt;
	date_ctxt . style = t_style;
	date_ctxt . current = r_current;
	date_ctxt . min = t_min;
	date_ctxt . max = t_max;
	date_ctxt . step = p_step;
	date_ctxt . use_cancel = p_use_cancel;
	date_ctxt . use_done = p_use_done;
	date_ctxt . button_rect = p_button_rect;
	
	// call the date picker with the label and options list
	MCIPhoneRunOnMainFiber(pickdate_prewait, &date_ctxt);
	
	// block until actionSheet releases the date pick wheel
	while([date_ctxt . picker running])
		MCscreen -> wait(60.0, False, True);
	
	MCIPhoneRunOnMainFiber(pickdate_postwait, &date_ctxt);
	
    // MM-2012-10-24: [[ Bug 10494 ]] Make sure we check to see if the picker has been cancelled.
    r_canceled = date_ctxt . cancelled;
    if (!r_canceled)
    {
        // Convert the seconds to date time
		MCAutoNumberRef t_number;
		if (!MCNumberCreateWithInteger(date_ctxt.current, &t_number))
			return false;
		
		if (!MCD_convert_to_datetime(ctxt, *t_number, CF_SECONDS, CF_SECONDS, *r_result))
			return false;
    }
	return true;
}

