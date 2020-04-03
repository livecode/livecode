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
#include "mblsyntax.h"

#import <UIKit/UIKit.h>
#include "mbliphoneapp.h"
#include "variable.h"
////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
float MCIPhoneGetNativeControlScale(void);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

// MM-2013-09-23: [[ iOS7 Support ]] Added missing delegates implemented in order to appease llvm 5.0.
@interface com_runrev_livecode_MCIPhonePickWheelDelegate : UIViewController <UIPickerViewDelegate, UIPickerViewDataSource, UIActionSheetDelegate, UITableViewDelegate, UIPopoverControllerDelegate, UITableViewDataSource>
{
	bool iSiPad;
	bool m_running;
	bool m_use_checkmark;
	bool m_selection_made;
	// HC-2011-10-03 [[ Picker Buttons ]] We need to know if we are displying a table view and if a highlight indicator is visible.
	bool m_use_table_view;
	bool m_bar_visible;
	bool m_use_picker;
	NSIndexPath *m_selected_index_path;
	NSMutableArray *m_selected_index;
	NSMutableArray *column_widths;
	NSArray *viewArray;
	UIPickerView *pickerView;
	UITableView *tableView;
	UIActionSheet *actionSheet;
	UIPopoverController* popoverController;
    UIView *m_action_sheet_view;
    UIControl *m_blocking_view;
    bool m_should_show_keyboard;
}

- (id)init;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCIPhonePickWheelDelegate

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	viewArray = nil;
	actionSheet = nil;
	popoverController = nil;
	iSiPad = false;
	m_running = false;
	m_selected_index_path = nil;
	m_selected_index = nil;
	m_selection_made = false;
	m_use_table_view = false;
	m_bar_visible = false;
	column_widths = nil;
	pickerView = nil;
	tableView = nil;
    m_action_sheet_view = nil;
    m_blocking_view = nil;
    m_should_show_keyboard = false;
	return self;
}

- (void)dealloc
{
	// MW-2011-05-25: [[ Bug 9529 ]] Make sure the UIPickerView is removed from the superview so
	//   the 'tracking mode' detection works properly!
	[pickerView removeFromSuperview];
	[viewArray release];
	[column_widths release];
	[m_selected_index release];
	[actionSheet release];
	[pickerView release];
	[tableView release];
	[super dealloc];
}

- (bool)isRunning
{
	return m_running;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
}

- (void)setUseCheckmark: (bool)p_use
{
	m_use_checkmark = p_use;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	// HC-2011-10-03 [[ Picker Buttons ]] Showing the bar dynamicly, to indicate if any initial selections have been made.
	// ensure that we are showing the translucent bar across the pick wheel if we are displaying the picker wheel
	pickerView.showsSelectionIndicator = YES;
	m_bar_visible = true;
	
	// return the object that has just been selected
	[m_selected_index replaceObjectAtIndex:component withObject: [NSNumber numberWithInt: row]];
	m_selection_made = true;
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
	// return the object that has just been selected
	return (NSString*)[[viewArray objectAtIndex:component]objectAtIndex:row];
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	// return the number of components in the pick wheel
	// LiveCode only allows one at the moment, so this is constant
	return [viewArray count];
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	// return the number of rows in the picker wheel
	return [[viewArray objectAtIndex:component] count];
}

- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component
{
	return [[column_widths objectAtIndex:component] intValue];	
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *t_cellIdentifier = nil;
	UITableViewCell *t_cell = [tableView dequeueReusableCellWithIdentifier: t_cellIdentifier];
	if (t_cell == nil)
		t_cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:t_cellIdentifier] autorelease];
	[t_cell setText: [[viewArray objectAtIndex:0]objectAtIndex:indexPath.row]];
	
	if (m_use_checkmark)
		[t_cell setAccessoryType: [indexPath row] == [[m_selected_index objectAtIndex:0] integerValue] ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone];
	else
	{
		if ([indexPath row] == [[m_selected_index objectAtIndex:0] integerValue])
			[tableView selectRowAtIndexPath: indexPath animated: NO scrollPosition: UITableViewScrollPositionNone];
	}
	
	return t_cell;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *) indexPath
{
	int32_t	t_old_index;
	t_old_index = [[m_selected_index objectAtIndex:0] intValue];
	[m_selected_index replaceObjectAtIndex:0 withObject: [NSNumber numberWithInt:[indexPath row]]];
	if (m_use_checkmark)
	{
		[[tableView cellForRowAtIndexPath: [NSIndexPath indexPathForRow: t_old_index inSection:0]] setAccessoryType: UITableViewCellAccessoryNone];
		[[tableView cellForRowAtIndexPath: indexPath] setAccessoryType: UITableViewCellAccessoryCheckmark];
	}
    // HC-30-2011-30 [[ Bug 10036 ]] iPad pick list only returns 0.
    m_selection_made = true;
	return indexPath;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willDeselectRowAtIndexPath:(NSIndexPath *) indexPath
{
	return indexPath;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath
{
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *) indexPath
{
	[popoverController dismissPopoverAnimated:YES];

	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	// return the number of rows in the table view
	return [[viewArray objectAtIndex:0] count];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
return 1;
}

// HC-2011-09-28 [[ Picker Buttons ]] Added arguments to force the display of buttons
// HC-30-2011-30 [[ Bug 9773 ]] Changed using char*& to NSString*& for return value r_chosen
- (void) startPicking: (NSArray *)p_pickerOptions andInitial: (NSArray *)p_initial  andCancel: (bool)p_use_cancel andDone: (bool)p_use_done andPicker: (bool)p_use_picker andButtonRect: (MCRectangle) p_button_rect
{
	bool t_no_initial = true;
	int t_i, t_ii, t_max_columns = 0;
	CGFloat t_totalDataWidth;
	double t_scalingFactor;
	CGFloat t_horizontal = 0.0;
	CGFloat t_vertical;

	// set up the wait status
	m_running = true;
	
	m_use_picker = p_use_picker;
	
	viewArray = [[NSArray alloc] initWithArray: p_pickerOptions];
	//[viewArray retain];

	m_selected_index = [[NSMutableArray alloc] initWithArray: p_initial];
	//[m_selected_index retain];
	
	for (t_i = 0; t_i < [m_selected_index count]; t_i++)
	{
		// test if we need a cancel button on the iPhone
        if ([[m_selected_index objectAtIndex:t_i] intValue] != 0)
			t_no_initial = false;
		// select the item that matches the label
		[m_selected_index replaceObjectAtIndex:t_i withObject:[NSNumber numberWithInt:[[m_selected_index objectAtIndex:t_i] intValue] - 1]];
		// ensure the label index is within legal limits
		if ([[m_selected_index objectAtIndex:t_i] intValue] != -1)
		{
			if ([[m_selected_index objectAtIndex:t_i] intValue] < 0)
				[m_selected_index replaceObjectAtIndex:t_i withObject: [NSNumber numberWithInt:0]];
			if ([[m_selected_index objectAtIndex:t_i] intValue] >= [[viewArray objectAtIndex:t_i] count])
				[m_selected_index replaceObjectAtIndex:t_i withObject: [NSNumber numberWithInt:[viewArray count] - 1]];
		}
	}
	m_selection_made = false;
	
	// set up local variables for all supported devices here to prevent scoping issues in the if statement
	UISegmentedControl *t_closeButton;
	
	// set up defaults for colum witdth scaling
	t_totalDataWidth = 0.0;
	t_scalingFactor = 1.0;
	column_widths = [[NSMutableArray alloc] initWithCapacity:[viewArray count]];
	
	// calculate the column widths
	UITableViewCell *t_cell = [tableView dequeueReusableCellWithIdentifier: nil];
	for (t_i = 0; t_i < [viewArray count]; t_i++)
	{
		if (t_max_columns < [[viewArray objectAtIndex:t_i] count])
			t_max_columns = [[viewArray objectAtIndex:t_i] count];
		[column_widths insertObject:[NSNumber numberWithInt: 0] atIndex:t_i];
		for (t_ii = 0; t_ii < [[viewArray objectAtIndex:t_i] count]; t_ii++)
		{
			if (t_cell == nil)
			{
				t_cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:nil] autorelease];
			}
			t_cell.text = [[viewArray objectAtIndex:t_i] objectAtIndex:t_ii];
			CGSize t_textLabelSize = [t_cell.text sizeWithFont:t_cell.font];
			if ([[column_widths objectAtIndex:t_i] intValue] < t_textLabelSize.width)
			{
				[column_widths replaceObjectAtIndex:t_i withObject: [NSNumber numberWithInt:t_textLabelSize.width]];
			}				
		}
		[column_widths replaceObjectAtIndex:t_i withObject:[NSNumber numberWithInt:[[column_widths objectAtIndex:t_i] intValue]+30]];
		t_totalDataWidth += [[column_widths objectAtIndex:t_i] intValue];
	}
	t_horizontal = t_totalDataWidth + ([viewArray count]-1) * 3 + 21; // add this for the pick wheel seperators
    int tViewWidth = MCIPhoneGetView().bounds.size.width;	
	// calculate scaling factor if the pick wheel is too wide
	if (t_horizontal > MCIPhoneGetView().bounds.size.width - 70)
	{
		double t_realDataWidth = t_totalDataWidth - [viewArray count] * 30.0;
		t_scalingFactor = MCIPhoneGetView().bounds.size.width - 70.0 - [viewArray count] * 30.0;
		t_totalDataWidth = 0.0;
		// apply scaling to widths
		for (t_i = 0; t_i < [viewArray count]; t_i++)
		{
			[column_widths replaceObjectAtIndex:t_i withObject:[NSNumber numberWithInt: (([[column_widths objectAtIndex:t_i] doubleValue] - 30.0) / t_realDataWidth) * t_scalingFactor + 30.0]];
			t_totalDataWidth += [[column_widths objectAtIndex:t_i] intValue];
		}
		t_horizontal = t_totalDataWidth + ([viewArray count]-1) * 3 + 21; // add this for the pick wheel seperators
	}
	// HC-2011-10-03 [[ Picker Buttons ]] Check that the popover width is sufficient for one or two buttons, if we are using them.
	if (p_use_done && p_use_cancel && t_horizontal < 140)
		t_horizontal = 140;
	else if ((p_use_done || p_use_cancel) && t_horizontal < 80)
		t_horizontal = 80;
	id t_popover = nil;
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
		t_popover = NSClassFromString(@"UIPopoverController");
	
	// create an action sheet on the iPhone/iPod and a popover on the iPad
	if (t_popover != nil)
	{
		iSiPad = true;
		// heuristic for popover dimensions
		// create between 1 and 7 rows for display, depending on how many items are in viewArray
		t_vertical = t_max_columns * 45;
		if (t_vertical < 45)
			t_vertical = 45;
		else if (t_vertical > 315) 
			t_vertical = 315;

		//currentSelection = t_labelIndex;
		if (([viewArray count] == 1) && !p_use_picker)
		{
			m_use_table_view = true;
			// create the table view
			if (p_use_done || p_use_cancel)
				tableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 45, t_horizontal, t_vertical) style:(UITableViewStylePlain)];
			else
				tableView = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, t_horizontal, t_vertical) style:(UITableViewStylePlain)];
			tableView.delegate = self;
            tableView.dataSource = (id<UITableViewDataSource>)self;

			// allow the user to select items
			tableView.allowsSelection = YES;
			
			// set up the bounding box of the popover box
			self.contentSizeForViewInPopover = CGSizeMake(t_horizontal,t_vertical);
			// scroll to the cell that is selected, if any
			if (t_no_initial == false)
			{
				if (m_selected_index_path == nil)
				{
					m_selected_index_path = [[NSIndexPath alloc] initWithIndex:0];
				}
				m_selected_index_path = [NSIndexPath indexPathForRow:[[m_selected_index objectAtIndex:0] intValue] inSection:0];
        
                // PM-2014-12-11: [[ Bug 12899 ]] Scroll only if there is a record
                if ([tableView numberOfRowsInSection:0] > 0)
                    [tableView scrollToRowAtIndexPath: m_selected_index_path atScrollPosition: UITableViewScrollPositionMiddle animated:NO];				
				m_bar_visible = true;
			}
		}
		else
		{
			// create the pick wheel
			// HC-2011-10-03 [[ Picker Buttons ]] Severl lines of the content in this part of the if statement has changed.
			// the y-coordinate depends on whether or not we are displaying buttons
			if (p_use_done || p_use_cancel)
				pickerView = [[UIPickerView alloc] initWithFrame: CGRectMake(0, 45, t_horizontal, 216)];
			else
				pickerView = [[UIPickerView alloc] initWithFrame: CGRectMake(0, 0, t_horizontal, 216)];
			pickerView.delegate = self;
			// HC-2011-10-03 [[ Picker Buttons ]] Showing the bar dynamicly, to indicate if any initial selections have been made.
			// show the translucent bar across the pick wheel
			if (t_no_initial)
				pickerView.showsSelectionIndicator = NO;
			else
			{
				pickerView.showsSelectionIndicator = YES;
				m_bar_visible = true;
			}
			// set the label item
			for (t_i = 0; t_i < [m_selected_index count]; t_i++)
				[pickerView selectRow:[[m_selected_index objectAtIndex:t_i] integerValue] inComponent:t_i animated:NO];
		}
		// make a toolbar
		UIToolbar *t_toolbar;
		NSMutableArray *t_toolbar_items;
		// HC-2011-10-03 [[ Picker Buttons ]] Only add a tool bar if we are going to use it.
		if (p_use_done || p_use_cancel)
		{
			t_toolbar = [[UIToolbar alloc] initWithFrame: CGRectMake(0, 0, t_horizontal, 44)];
			t_toolbar.barStyle = UIBarStyleBlack;
			t_toolbar.translucent = YES;
			
			t_toolbar_items = [[NSMutableArray alloc] init];
			// HC-2011-09-28 [[ Picker Buttons ]] Enable cancel button on request.
			if (p_use_cancel) 
				[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemCancel target: self action: @selector(cancelPickWheel:)]];
			[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace target: self action: nil]];
			// HC-2011-09-28 [[ Picker Buttons ]] Enable done button on request.
			if (p_use_done) 
				[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemDone target: self action: @selector(dismissPickWheel:)]];
			[t_toolbar setItems: t_toolbar_items animated: NO];
		}
        
        
		// create the action sheet that can contain the "Cancel" and "Done" buttons and date pick wheel
        
        
		actionSheet = [[UIActionSheet alloc] initWithTitle:nil //[pickerArray objectAtIndex:0]
												  delegate:self
										 cancelButtonTitle:nil //@"Done"
									destructiveButtonTitle:nil
										 otherButtonTitles:nil];
		
		// set the style of the actionsheet
		[actionSheet setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
		
		// add the subviews to the action sheet
		if (m_use_table_view)
			[actionSheet addSubview: tableView];
		else
		[actionSheet addSubview: pickerView];
		if (p_use_done || p_use_cancel)
		{
			[actionSheet addSubview: t_toolbar];
			[t_toolbar release];
		}	
        

		// set up the bounding box of the popover
		// the height depends on whether or not we are displaying buttons
		if (m_use_table_view)
		{
			if (p_use_done || p_use_cancel)
				self.contentSizeForViewInPopover = CGSizeMake(t_horizontal, t_vertical+45);
			else
				self.contentSizeForViewInPopover = CGSizeMake(t_horizontal, t_vertical);			
		}
		else
		{
			if (p_use_done || p_use_cancel)
				self.contentSizeForViewInPopover = CGSizeMake(t_horizontal, 261);
			else
				self.contentSizeForViewInPopover = CGSizeMake(t_horizontal, 216);
		}
        
		// create the popover controller
		popoverController = [[t_popover alloc] initWithContentViewController:self];
        
        // need to make self as delegate otherwise overridden delegates are not called
        popoverController.delegate = self;
        
        [popoverController setPopoverContentSize:self.contentSizeForViewInPopover];
		[popoverController presentPopoverFromRect:MCUserRectToLogicalCGRect(p_button_rect)
										   inView:MCIPhoneGetView()
                         permittedArrowDirections:UIPopoverArrowDirectionAny
										 animated:YES];
        
		
        // The following line creates problem on iOS 8 - Remove it
		//[popoverController setContentViewController:self];
	}
	else
	{
		iSiPad = false;
		
		// compute orientation
		bool t_is_landscape;
		t_is_landscape = UIInterfaceOrientationIsLandscape(MCIPhoneGetOrientation());
        
        // PM-2015-03-25: [[ Bug 15070 ]] The actionSheet that contains the pickWheel should be of fixed height
        CGFloat t_toolbar_portrait_height, t_toolbar_landscape_height;
        t_toolbar_portrait_height = 44;
        t_toolbar_landscape_height = 32;
		
		// create the pick wheel
        // If you create an instance with a width and height of 0, they will be overridden with the appropriate default width and height, which you can get by frame.size.width/height.
		pickerView = [[UIPickerView alloc] initWithFrame: CGRectMake(0, (t_is_landscape ? t_toolbar_landscape_height : t_toolbar_portrait_height), 0, 0)];

		pickerView.delegate = self;
		
		// HC-2011-10-03 [[ Picker Buttons ]] Showing the bar dynamicaly, to indicate if any initial selections have been made.
		// show the translucent bar across the pick wheel
		if (t_no_initial)
			pickerView.showsSelectionIndicator = NO;
		else
		{
			pickerView.showsSelectionIndicator = YES;
			m_bar_visible = true;
		}

        // PM-2014-10-22: [[ Bug 13750 ]] Make sure the view under the pickerView is not visible (iphone 4 only)
        NSString *t_device_model_name = MCIPhoneGetDeviceModelName();
        if ([t_device_model_name isEqualToString:@"iPhone 4"] || [t_device_model_name isEqualToString:@"iPhone 4(Rev A)"] || [t_device_model_name isEqualToString:@"iPhone 4(CDMA)"])
            pickerView.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.90];
        
		// set the label item
		for (t_i = 0; t_i < [m_selected_index count]; t_i++)
			[pickerView selectRow:[[m_selected_index objectAtIndex:t_i] integerValue] inComponent:t_i animated:NO];
		
		// make a toolbar
        // MM-2012-10-15: [[ Bug 10463 ]] Make the picker scale to the width of the device rather than a hard coded value (fixes issue with landscape iPhone 5 being 568 not 480).
		UIToolbar *t_toolbar;
        
        if (t_is_landscape)
            t_toolbar = [[UIToolbar alloc] initWithFrame: CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_landscape_height)];
        else
            t_toolbar = [[UIToolbar alloc] initWithFrame: CGRectMake(0, 0, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_portrait_height)];
        
		t_toolbar.barStyle = UIBarStyleBlack;
		t_toolbar.translucent = YES;
		[t_toolbar sizeToFit];

		NSMutableArray *t_toolbar_items;
		t_toolbar_items = [[NSMutableArray alloc] init];
		// HC-2011-09-28 [[ Picker Buttons ]] Enable cancel button on request.
		if (t_no_initial == true || p_use_cancel)
			[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemCancel target: self action: @selector(cancelPickWheel:)]];
		[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace target: self action: nil]];
		[t_toolbar_items addObject: [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemDone target: self action: @selector(dismissPickWheel:)]];
		
		[t_toolbar setItems: t_toolbar_items animated: NO];
        
        if (MCmajorosversion < 800)
        {
            // create the action sheet that contains the "Done" button and pick wheel
            actionSheet = [[UIActionSheet alloc] initWithTitle:nil
                                                      delegate:self
                                             cancelButtonTitle:nil
                                        destructiveButtonTitle:nil
                                             otherButtonTitles:nil];
            // set the style of the acionsheet
            [actionSheet setActionSheetStyle:UIActionSheetStyleBlackTranslucent];
            // add the subviews to the action sheet
            [actionSheet addSubview: t_toolbar];
            [actionSheet addSubview: pickerView];
            [t_toolbar release];
            
            // initialize the pick wheel, this also calls viewDidLoad and populates pickerArray
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
            // PM-2014-09-25: [[ Bug 13484 ]] In iOS 8 and above, UIActionSheet is not working properly
            
            CGRect t_rect;
            // PM-2015-04-13: [[ Bug 15070 ]] There are only three valid values (162.0, 180.0 and 216.0) for the height of the picker
            uint2 t_pick_wheel_height = pickerView.frame.size.height;
            
            // PM-2015-03-25: [[ Bug 15070 ]] Make the rect of the m_action_sheet_view to be of fixed height
            if (!t_is_landscape)
            {
                [pickerView setFrame:CGRectMake(0, t_toolbar_portrait_height, [[UIScreen mainScreen] bounds] . size . width, t_pick_wheel_height)];
                t_rect = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_portrait_height - t_pick_wheel_height, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_portrait_height + t_pick_wheel_height);
            }
            else
            {
                [pickerView setFrame:CGRectMake(0, t_toolbar_landscape_height, [[UIScreen mainScreen] bounds] . size . width, t_pick_wheel_height)];
                t_rect = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height - t_toolbar_landscape_height - t_pick_wheel_height, [[UIScreen mainScreen] bounds] . size . width, t_toolbar_landscape_height + t_pick_wheel_height);
            }
            
            m_action_sheet_view = [[UIView alloc] initWithFrame:t_rect];

            [m_action_sheet_view addSubview: t_toolbar];
            [m_action_sheet_view addSubview: pickerView];
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
            if (!t_is_landscape)
                m_action_sheet_view.frame = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . height , t_rect . size . width, t_rect. size . height);
            else
                m_action_sheet_view.frame = CGRectMake(0, [[UIScreen mainScreen] bounds] . size . width , t_rect . size . width, t_rect. size . height);
            
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

- (NSString *)finishPicking
{
	NSString *t_chosen;
	
    if (iSiPad && ([m_selected_index count] == 1) && !m_use_picker)
    {
        [m_selected_index replaceObjectAtIndex:0 withObject:[NSNumber numberWithInt:[[m_selected_index objectAtIndex:0] intValue] + 1]];
        
        if (m_selection_made && ([[m_selected_index objectAtIndex:0] intValue] == 0))
            [m_selected_index replaceObjectAtIndex:0 withObject:[NSNumber numberWithInt:1]];
        // HC-30-2011-30 [[ Bug 9773 ]] Changed using char* to NSString*& for return value r_chosen
        t_chosen = [[NSString alloc] initWithString:[[m_selected_index objectAtIndex:0] stringValue]];
    }
    else
    {
		NSMutableString *t_string;
		t_string = [NSMutableString stringWithCapacity: 0];
        [t_string appendFormat:@"%d", [pickerView selectedRowInComponent: 0] + 1];
        // set the label item
        for (uint32_t t_i = 1; t_i < [m_selected_index count]; t_i++)
        {
            // HC-30-2011-30 [[ Bug 9773 ]] Changed using char* to NSString*& for return value r_chosen
            [t_string appendFormat:@",%d", [pickerView selectedRowInComponent: t_i] + 1];
        }
		t_chosen = [[NSString alloc] initWithString: t_string];
    }	
	// device specific release
	if (iSiPad)
		[popoverController release];
	
	return t_chosen;
}

// called when the action sheet is dismissed (iPhone, iPod)

// MW-2010-12-20: [[ Bug 9254 ]] Don't stop running here as the animation has yet to end.
- (void)dismissPickWheel:(NSObject *)controlButton
{
	int t_i;
	// dismiss the action sheet programmatically
	// HC-2011-10-03 [[ Picker Buttons ]] If the selection bar is not visible then treat the dismiss in the same way as cancel i.e. all 0.
	if (m_bar_visible)
	{
		m_selection_made = true;
		// HC-2011-10-03 [[ Picker Buttons ]] A button dismiss can be caused by picker or table view. Make sure we do not access the picker in the wrong mode.
		if (!m_use_table_view)
		{	
			for (t_i = 0; t_i < [m_selected_index count]; t_i++)
				[m_selected_index replaceObjectAtIndex:t_i withObject: [NSNumber numberWithInt:[pickerView selectedRowInComponent: t_i]]];
		}
	}
	else
	{
		m_selection_made = false;
		for (t_i = 0; t_i < [m_selected_index count]; t_i++)
			[m_selected_index replaceObjectAtIndex:t_i withObject: [NSNumber numberWithInt:-1]];
	}
    
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
            [pickerView removeFromSuperview];
        
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

- (void)cancelPickWheel: (NSObject *)sender
{
	int t_i;
	m_selection_made = false;
	for (t_i = 0; t_i < [m_selected_index count]; t_i++)
		[m_selected_index replaceObjectAtIndex:t_i withObject:[NSNumber numberWithInt:-1]];
    
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
            [pickerView removeFromSuperview];
            
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

// MW-2010-12-20: [[ Bug 9254 ]] Stop running the action sheet here, after the animation
//   is done otherwise we crash as the sheet gets freed prematurely.
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
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

// called when including the UIPickerView in the UIViewController
- (void)loadView
{
	if (iSiPad)
	{
		// HC-2011-09-28 [[ Picker Buttons ]] We are now using an actionSheet on the iPad.
        self.view = actionSheet;
		//self.view = actionSheet.view;
        
	}
	else
		self.view = pickerView;
}

- (bool)selectionMade
{
    return m_selection_made;
}
@end

static com_runrev_livecode_MCIPhonePickWheelDelegate *s_pick_wheel_delegate = nil;

////////////////////////////////////////////////////////////////////////////////

// MM-2013-09-23: [[ iOS7 Support ]] Made arrays non-const to appease llvm 5.0.
struct picker_t
{
	NSArray *option_list_array;
	bool use_checkmark;
	bool use_cancel;
	bool use_done;
	bool use_picker;
	bool use_hilited;
	NSArray *initial_index_array;
	NSString *return_index;
	MCRectangle button_rect;
	com_runrev_livecode_MCIPhonePickWheelDelegate *picker;
	bool cancelled;
};

static void do_pickn_prewait(void *p_context)
{
	picker_t *ctxt;
	ctxt = (picker_t *)p_context;
	
	// call the picker with the label and options list
	ctxt -> picker = [[com_runrev_livecode_MCIPhonePickWheelDelegate alloc] init];
	[ctxt -> picker setUseCheckmark: ctxt -> use_checkmark];

	// HC-2011-09-28 [[ Picker Buttons ]] Added arguments to force the display of buttons and picker
	[ctxt -> picker startPicking: ctxt -> option_list_array andInitial: ctxt -> initial_index_array  andCancel: ctxt -> use_cancel andDone: ctxt -> use_done andPicker: ctxt -> use_picker andButtonRect: ctxt -> button_rect];

}

static void do_pickn_postwait(void *p_context)
{
	picker_t *ctxt;
	ctxt = (picker_t *)p_context;
	
	NSString *t_result;
	ctxt -> return_index = [ctxt -> picker finishPicking];
	
	[ctxt -> picker release];
}

// HC-2011-09-28 [[ Picker Buttons ]] Added arguments to force the display of buttons
// HC-2011-09-30 [[ Bug 9773 ]] Changed using char*& argument to NSString*& for return value
bool MCSystemPickN(NSArray *p_option_list_array, bool p_use_checkmark, bool p_use_cancel, bool p_use_done, bool p_use_picker, NSArray *p_initial_index_array, NSString*& r_return_index, MCRectangle p_button_rect)
{
	// ensure that the options list array and initial index array are populated and have the same number of elements
	if (p_option_list_array == nil || p_initial_index_array == nil || ([p_option_list_array count] != [p_initial_index_array count]))
		return false;
	
	picker_t ctxt;
	ctxt . option_list_array = p_option_list_array;
	ctxt . use_checkmark = p_use_checkmark;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . use_picker = p_use_picker;
	ctxt . initial_index_array = p_initial_index_array;
	ctxt . button_rect = p_button_rect;
	MCIPhoneRunOnMainFiber(do_pickn_prewait, &ctxt);
	
	while([ctxt . picker isRunning])
		MCscreen -> wait(1.0, False, True);
	
	MCIPhoneRunOnMainFiber(do_pickn_postwait, &ctxt);
	
	r_return_index = [ctxt . return_index autorelease];
    
	return true;
}

bool MCSystemPick(MCStringRef p_options, bool p_use_checkmark, uint32_t p_initial_index, uint32_t& r_chosen_index, MCRectangle p_button_rect)
{
	bool t_success;
	t_success = true;
	
	CFStringRef t_options;
	NSArray *t_option_list_array;
	
	NSArray *t_initial_index_array;
	
	int t_max_result_memory = 0;
	// HC-30-2011-30 [[ Bug 9773 ]] Changed using char*& argument to NSString*& for return value
	NSString *t_return_index = nil;
	
	// provide the correct encoding for the options list
	/* UNCHECKED */ MCStringConvertToCFStringRef(p_options, t_options);
	
	// convert the \n delimited item string into a pick wheel array
	t_option_list_array = [NSArray arrayWithObject: [(NSString *)t_options componentsSeparatedByString:@"\n"]];
	CFRelease(t_options);

	// convert the initial index for each component into an array entry
	t_initial_index_array = [NSArray arrayWithObject: [NSNumber numberWithInt: p_initial_index]];
	// get the maximum number of digits needed for the entry column that was just added
	t_max_result_memory+=5;
	
	// HC-2011-09-28 [[ Picker Buttons ]] Added arguments so default behaviour occurs when calling MCSystemPick as a button.
	if (t_success)
		t_success = MCSystemPickN(t_option_list_array, p_use_checkmark, false, false, false, t_initial_index_array, t_return_index, p_button_rect);
	
	MCAutoStringRef t_result;
	/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_return_index, &t_result);
	if (t_success)
		MCresult -> setvalueref (*t_result);
	r_chosen_index = atoi ([t_return_index cStringUsingEncoding:NSMacOSRomanStringEncoding]);
	
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

static void pick_option_prewait(void *p_context)
{
	picker_t *ctxt;
	ctxt = (picker_t *)p_context;
	
	ctxt -> picker = [[com_runrev_livecode_MCIPhonePickWheelDelegate alloc] init];
	[ctxt -> picker setUseCheckmark: !ctxt->use_hilited];
	// HC-2011-09-28 [[ Picker Buttons ]] Added arguments to force the display of buttons and picker
	[ctxt -> picker startPicking: ctxt->option_list_array andInitial: ctxt->initial_index_array andCancel: ctxt->use_cancel andDone: ctxt->use_done andPicker: ctxt->use_picker andButtonRect: ctxt->button_rect];

}

static void pick_option_postwait(void *p_context)
{
	picker_t *ctxt;
	ctxt = (picker_t *)p_context;
	
	ctxt -> return_index = [ctxt -> picker finishPicking];
	
    // HC-2012-02-15 [[ BUG 9999 ]] Date Picker should return 0 if cancel was selected.
    ctxt -> cancelled = ![ctxt -> picker selectionMade];
	
	[ctxt-> picker release];
}

bool MCSystemPickOption(MCPickList *p_pick_lists, uindex_t p_pick_list_count, uindex_t *&r_result, uindex_t &r_result_count, bool p_use_hilited, bool p_use_picker, bool p_use_cancel, bool p_use_done, bool &r_canceled, MCRectangle p_button_rect)
{
    bool t_success;
	t_success = true;
	
	NSString *t_options;
	NSArray *t_option_list_array;
	NSArray *t_initial_index_array;
    NSArray *t_temp_picker_array;
  	
	int t_max_result_memory = 0;
	// HC-30-2011-30 [[ Bug 9773 ]] Changed using char*& argument to NSString*& for return value
	NSString *t_return_index = nil;
	
    if (t_success)
	{
		t_option_list_array = [[NSArray alloc] init];
		t_initial_index_array = [[NSArray alloc] init];
		[t_option_list_array retain];
		[t_initial_index_array retain];
	}
    
    // convert the input data from a c-style array of MCPickLists to an NSArray
    for (int i = 0; i < p_pick_list_count; i++)
    {
        t_temp_picker_array = [[NSArray alloc] init];
        [t_temp_picker_array retain];
        for (int j = 0; j < p_pick_lists[i] . option_count; j++)
        {
            t_options = MCStringConvertToAutoreleasedNSString(p_pick_lists[i] . options[j]);
            t_temp_picker_array = [t_temp_picker_array arrayByAddingObject: t_options];
        }
        t_option_list_array = [t_option_list_array arrayByAddingObject: t_temp_picker_array];
    }
    
    // convert the input indexes data from const_cstring_array_t to an NSArray
    for (int i = 0; i < p_pick_list_count; i++)
    {
        t_initial_index_array = [t_initial_index_array arrayByAddingObject: [NSNumber numberWithInt: p_pick_lists[i] . initial]];
    }
    
	// get the maximum number of digits needed for the entry column that was just added
	t_max_result_memory+=5;
	
	picker_t ctxt;
	ctxt . option_list_array = t_option_list_array;
	ctxt . use_hilited = p_use_hilited;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . use_picker = p_use_picker;
	ctxt . initial_index_array = t_initial_index_array;
	ctxt . button_rect = p_button_rect;
	ctxt . cancelled = false;
	
	// call the picker with the label and options list
	MCIPhoneRunOnMainFiber(pick_option_prewait, &ctxt);
	
	while([ctxt . picker isRunning])
		MCscreen -> wait(1.0, False, True);
	
	MCIPhoneRunOnMainFiber(pick_option_postwait, &ctxt);
    
	r_canceled = ctxt . cancelled;
	
    // Create the result information here
    if (ctxt . return_index == nil)
    {
        r_result = nil;
        r_result_count = 0;
    }
    else
    {
        MCAutoArray<uindex_t> t_indices;
		char *t_ptr;
		MCCStringClone ([ctxt . return_index cStringUsingEncoding:NSMacOSRomanStringEncoding], t_ptr);
		
		const char *t_token;
		t_token = strtok (t_ptr, ",");
        
        while (t_ptr != nil)
        {
            t_indices . Push(atoi (t_ptr));
            t_ptr = strtok (nil, ",");
                
        }
        t_indices . Take(r_result, r_result_count);
    }

	[ctxt . return_index release];
	
    return t_success;
    
}
/*
bool MCSystemPickOption(const_cstring_array_t **p_expression, const_int32_array_t *p_indexes, uint32_t p_expression_cnt, const_int32_array_t *&r_result, bool p_use_hilited, bool p_use_picker, bool p_use_cancel, bool p_use_done, bool &r_canceled, MCRectangle p_button_rect)
{
	MCExecPoint ep(nil, nil, nil);
	
    bool t_success;
	t_success = true;
	
	NSString *t_options;
	NSArray *t_option_list_array;
	NSArray *t_initial_index_array;
    NSArray *t_temp_picker_array;
  	
	int t_max_result_memory = 0;
	// HC-30-2011-30 [[ Bug 9773 ]] Changed using char*& argument to NSString*& for return value
	NSString *t_return_index = nil;
	
    if (t_success)
	{
		t_option_list_array = [[NSArray alloc] init];
		t_initial_index_array = [[NSArray alloc] init];
		[t_option_list_array retain];
		[t_initial_index_array retain];
	}
    
    // convert the input expression data from const_cstring_array_t to an NSArray
    for (int i = 0; i < p_expression_cnt; i++)
    {
        t_temp_picker_array = [[NSArray alloc] init];
        [t_temp_picker_array retain];
        for (int j = 0; j < p_expression[i]->length; j++)
        {
			// TODO - update to support unicode
            t_options = [NSString stringWithCString: p_expression[i]->elements[j] encoding: NSMacOSRomanStringEncoding];
            t_temp_picker_array = [t_temp_picker_array arrayByAddingObject: t_options];
        }
        t_option_list_array = [t_option_list_array arrayByAddingObject: t_temp_picker_array];
    }
    
    // convert the input indexes data from const_cstring_array_t to an NSArray
    for (int i = 0; i < p_indexes->length; i++)
    {
        t_initial_index_array = [t_initial_index_array arrayByAddingObject: [NSNumber numberWithInt: p_indexes->elements[i]]];
    }
    
	// get the maximum number of digits needed for the entry column that was just added
	t_max_result_memory+=5;
	
	picker_t ctxt;
	ctxt . option_list_array = t_option_list_array;
	ctxt . use_hilited = p_use_hilited;
	ctxt . use_cancel = p_use_cancel;
	ctxt . use_done = p_use_done;
	ctxt . use_picker = p_use_picker;
	ctxt . initial_index_array = t_initial_index_array;
	ctxt . button_rect = p_button_rect;
	ctxt . cancelled = false;
	
	// call the picker with the label and options list
	MCIPhoneRunOnMainFiber(pick_option_prewait, &ctxt);
	
	while([ctxt . picker isRunning])
		MCscreen -> wait(1.0, False, True);
	
	MCIPhoneRunOnMainFiber(pick_option_postwait, &ctxt);

	r_canceled = ctxt . cancelled;
	
    // Create the result information here
    if (ctxt . return_index == nil)
        r_result = nil;
    else
    {
		char *t_ptr;
		MCCStringClone ([ctxt . return_index cStringUsingEncoding:NSMacOSRomanStringEncoding], t_ptr);
		
		const char *t_token;
		t_token = strtok (t_ptr, ",");

        t_success = MCMemoryAllocate(sizeof (const_int32_array_t), r_result);
        if (t_success)
        {
            r_result->length = 0;
            r_result->elements = nil;

            while (t_ptr != nil && t_success)
            {
                t_success = MCMemoryResizeArray(r_result->length + 1, r_result->elements, r_result->length);
                if (t_success)
                {
                    r_result->elements[r_result->length - 1] = atoi (t_ptr); 
                    t_ptr = strtok (nil, ",");

                }
            }
        }            
    }

	[ctxt . return_index release];
	
    return t_success;
} */

////////////////////////////////////////////////////////////////////////////////

