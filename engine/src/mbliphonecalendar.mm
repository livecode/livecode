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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "execpt.h"
#include "globals.h"

#include "exec.h"
#include "mblsyntax.h"
#include "mblcalendar.h"
#include "mbliphoneapp.h"

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import <EventKit/EventKit.h>
#import <EventKitUI/EventKitUI.h>
 
////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

#ifdef __IPHONE_4_0
@interface MCIPhonePickEventDelegate : UIViewController <EKEventEditViewDelegate>
#else
@interface MCIPhonePickEventDelegate : UIViewController <EKEventEditViewDelegate, EKEventViewDelegate>
#endif
{
    bool m_running;
	EKEventEditViewController *m_get_event;
	UINavigationController *m_navigation;
	EKEventEditViewController *m_update_event;
	EKEventViewController *m_view_event;
    NSString *m_selected_events;
    EKEventStore *m_event_store;
	bool m_finished;
}

- (id)init;
- (void)dealloc;

@end

@implementation MCIPhonePickEventDelegate

- (id)init
{
    // Is a view delegate running.
	m_running = true;

    // The view delegates.
	m_get_event = nil;
	m_navigation = nil;
    m_update_event = nil;
	m_view_event = nil;
    m_event_store = nil;
    
    // Returned values.
    m_selected_events = nil;
    
	return self;
}

- (void)dealloc
{
	if (m_get_event != nil)
		[m_get_event release];
	if (m_navigation != nil)
		[m_navigation release];   
	if (m_update_event != nil)
		[m_update_event release];
	if (m_view_event != nil)
		[m_view_event release];
    if (m_event_store != nil)
        [m_event_store release];
    [super dealloc];
}

-(void)createEvent: (MCCalendar) p_event_data withResult: (EKEvent*&) r_event
{
    MCExecPoint ep(nil, nil, nil);
    CFErrorRef t_an_error = NULL;
	bool t_did_add = true;

    // Set the calendar
    if (p_event_data.mccalendar.getlength() > 0 && t_an_error == NULL && t_did_add == true)
    {
        if (m_event_store == nil)
            m_event_store = [[EKEventStore alloc] init];
        
        // Fetch all calendars that are available.
        NSArray *t_calendars = [m_event_store calendars];
        
        if ((t_calendars != nil) && [t_calendars count])
        {
            // set the label item
            for (int t_i = 0; t_i < [t_calendars count]; t_i++)
            {
                if (MCCStringEqualCaseless([[[t_calendars objectAtIndex:t_i] title] UTF8String], p_event_data.mccalendar.getstring()))
                {
                    [r_event setCalendar:[t_calendars objectAtIndex:t_i]];
                    t_i = [t_calendars count];
                }
            }
        }
    }
    // A Boolean value that indicates whether the event is an all-day event.
    if (p_event_data.mcalldayset == true)
        r_event.allDay = p_event_data.mcallday;
    
    if (p_event_data.mctitle.getlength() > 0 && t_an_error == NULL && t_did_add == true)
        r_event.title = [NSString stringWithCString: p_event_data.mctitle.getstring() encoding:NSMacOSRomanStringEncoding];

    if (p_event_data.mcnote.getlength() > 0 && t_an_error == NULL && t_did_add == true)
        r_event.notes = [NSString stringWithCString: p_event_data.mcnote.getstring() encoding:NSMacOSRomanStringEncoding];

    if (p_event_data.mclocation.getlength() > 0 && t_an_error == NULL && t_did_add == true)
        r_event.location = [NSString stringWithCString: p_event_data.mclocation.getstring() encoding:NSMacOSRomanStringEncoding];
    
    // Set up the dates
    if (p_event_data.mcstartdateset == true && MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_event_data.mcstartdate))
    {
        r_event.startDate = [NSDate dateWithTimeIntervalSince1970:ep.getnvalue()];
    }

    if (p_event_data.mcenddateset == true && MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_event_data.mcenddate))
    {
        r_event.endDate = [NSDate dateWithTimeIntervalSince1970:ep.getnvalue()];
    }

    // Set up the alerts
    NSMutableArray *t_alerts_array = nil;
    EKAlarm *t_alarm_1 = nil;
    EKAlarm *t_alarm_2 = nil;
    
    if (p_event_data.mcalert1 >= 0)
    { 
        t_alerts_array = [[NSMutableArray alloc] init];
        t_alarm_1 = [EKAlarm alarmWithRelativeOffset:-p_event_data.mcalert1];
        [t_alerts_array addObject:t_alarm_1];
    }
    if (p_event_data.mcalert2 >= 0)
    {
        if (t_alerts_array == nil)
            t_alerts_array = [[NSMutableArray alloc] init];
        t_alarm_2 = [EKAlarm alarmWithRelativeOffset:-p_event_data.mcalert2];
        [t_alerts_array addObject:t_alarm_2];
    }
    if (t_alerts_array != nil)
    {
        r_event.alarms = t_alerts_array;
        [t_alerts_array release];
    }
}

-(MCCalendar)createEventData: (EKEvent*) p_event
{
    MCExecPoint ep(nil, nil, nil);
    MCCalendar t_event_data;
    MCString t_temp_string;
    
    t_event_data.mcalert1 = -1;
    t_event_data.mcalert2 = -1;
    t_event_data.mcallday = p_event.allDay;
    
    t_temp_string = [((NSString*) p_event.notes) UTF8String];
    t_event_data.mcnote = t_temp_string.clone();

    t_temp_string = [((NSString*) p_event.location) UTF8String];
    t_event_data.mclocation = t_temp_string.clone();
    
    t_temp_string = [((NSString*) p_event.title) UTF8String];
    t_event_data.mctitle = t_temp_string.clone();

    int32_t t_date_in_seconds;
    
    // Convert the seconds to date time        
    t_date_in_seconds = [p_event.startDate timeIntervalSince1970];
    ep.setnvalue(t_date_in_seconds);
    MCD_convert_to_datetime(ep, CF_SECONDS, CF_SECONDS, t_event_data.mcstartdate);
    
    // Convert the seconds to date time        
    t_date_in_seconds = [p_event.endDate timeIntervalSince1970];
    ep.setnvalue(t_date_in_seconds);
    MCD_convert_to_datetime(ep, CF_SECONDS, CF_SECONDS, t_event_data.mcenddate);

    // Get the alerts
    if (p_event.alarms != nil)
    {
        NSArray *t_alerts_array = p_event.alarms;
        if ([t_alerts_array count] >= 1)
            t_event_data.mcalert1 = abs([[t_alerts_array objectAtIndex:0] relativeOffset]);
        if ([t_alerts_array count] >= 2)
            t_event_data.mcalert2 = abs([[t_alerts_array objectAtIndex:1] relativeOffset]);
    }

    // Get the calendar
    if (p_event.calendar != nil)
    {
        t_temp_string = [((NSString*) p_event.calendar.title) UTF8String];
        t_event_data.mccalendar = t_temp_string;
    }
    return t_event_data;
}

- (void)doDismissController
{
	if (MCmajorosversion >= 500)
		[MCIPhoneGetViewController() dismissViewControllerAnimated:YES completion:^(){m_finished = true;}];
	else
        [MCIPhoneGetViewController() dismissModalViewControllerAnimated:YES];
}

- (void)dismissController
{
    // HSC-2012-05-14: [[ BZ 10213 ]] Delayed continuation until view disappeared. 
    if (MCmajorosversion >= 500)
    {
        m_finished = false;
		MCIPhoneCallSelectorOnMainFiber(self, @selector(doDismissController));
        while (!m_finished)
            MCscreen -> wait(1.0, False, True);
    }
    else
	{
		MCIPhoneCallSelectorOnMainFiber(self, @selector(doDismissController));
		while([self retainCount] > 2)
			MCscreen -> wait(0.01, False, True);
	}
}

- (void)doShowViewEvent: (NSString *)p_event_id
{
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    EKEvent *t_event = [m_event_store eventWithIdentifier:p_event_id];
    if (t_event != nil)
    {
        m_view_event = [[EKEventViewController alloc] init];
        m_view_event.event = t_event;
        m_view_event.allowsEditing = NO;
        m_navigation = [[UINavigationController alloc] initWithRootViewController: m_view_event];
        [m_navigation setToolbarHidden: NO];
        // Test if we are on an iPad ore on an iPhone
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
            [m_navigation setNavigationBarHidden:YES];
		m_running = true;
        [MCIPhoneGetViewController() presentModalViewController:m_navigation animated:YES];
        UIBarButtonItem *t_done_button = [[UIBarButtonItem alloc] initWithTitle:@"Done" style:UIBarButtonItemStyleDone target:self action:@selector(handleShowEventDone)];
        NSArray *t_items = [NSArray arrayWithObject: t_done_button];
        [m_navigation.toolbar setItems: t_items animated:YES];
        [t_done_button release];
	}
	else
		m_running = false;
}

-(void)showViewEvent: (NSString*) p_event_id withResult: (NSString*&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiberWithObject(self, @selector(doShowViewEvent:), p_event_id);
	if (m_running)
	{
        while (m_running)
            MCscreen -> wait(1.0, False, True);
		
		[self dismissController];

        // Return the result
        r_chosen = p_event_id;
    }
    else
    {
        // Return the result
        r_chosen = nil;
    }
}

-(void)doShowCreateEvent
{
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    m_get_event = [[EKEventEditViewController alloc] init];
    m_get_event.eventStore = m_event_store;
    m_get_event.editViewDelegate = self;
	m_running = true;
    [MCIPhoneGetViewController() presentModalViewController:m_get_event animated:YES];
}

-(void)showCreateEvent: (NSString*&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiber(self, @selector(doShowCreateEvent));
	
	while (m_running)
		MCscreen -> wait(1.0, False, True);
	
	[self dismissController];
	
    // Return the result
    r_chosen = m_selected_events;
}

-(void)doShowUpdateEvent: (NSString *)p_event_id
{
    bool t_did_add = YES;
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    EKEvent *t_event = [m_event_store eventWithIdentifier:p_event_id];
    if (t_event != nil)
    {
        m_update_event = [[EKEventEditViewController alloc] init];
        m_update_event.eventStore = m_event_store;
        m_update_event.event = t_event;
        m_update_event.editViewDelegate = self;
		m_running = true;
        [MCIPhoneGetViewController() presentModalViewController:m_update_event animated:YES];
	}
	else
		m_running = false;
}

-(void)showUpdateEvent: (NSString*) p_event_id withResult: (NSString*&) r_chosen
{
	MCIPhoneCallSelectorOnMainFiberWithObject(self, @selector(doShowUpdateEvent:), p_event_id);
	
	if (m_running)
	{
	    while (m_running)
            MCscreen -> wait(1.0, False, True);
		
        [MCIPhoneGetViewController() dismissModalViewControllerAnimated:YES];
		
        // Return the result
        r_chosen = m_selected_events;
    }
    else
    {
        // Return the result
        r_chosen = nil;
    }
 }

-(void)addEvent: (MCCalendar) p_new_event_data withResult: (NSString*&) r_chosen
{
    // Get an event value to update
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    EKEvent *t_event;
    if (p_new_event_data.mceventid.getlength () > 0)
    {
        t_event = [m_event_store eventWithIdentifier:[NSString stringWithCString: p_new_event_data.mceventid.getstring() encoding:NSMacOSRomanStringEncoding]];
        const char *t_temp_string = [((NSString*) t_event.calendar.title) UTF8String];
    }
    else
    {
        t_event = [EKEvent eventWithEventStore:m_event_store];
        [t_event setCalendar:[m_event_store defaultCalendarForNewEvents]];
    }
    
    if (t_event != nil)
    {
        [self createEvent:p_new_event_data withResult:t_event];
        NSError *err;
        [m_event_store saveEvent:t_event span:EKSpanThisEvent error:&err];
        if (t_event.eventIdentifier != nil)
            r_chosen = [[NSString alloc] initWithString: t_event.eventIdentifier];
    }
    else
    {
        r_chosen = nil;
    }
}

-(void)getCalendarsEvent: (NSString*&) r_chosen
{
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];

    // Fetch all calendars that are available.
    NSArray *t_calendars;
#ifdef __IPHONE_6_0
    if (MCmajorosversion >= 600)
        t_calendars = [m_event_store calendarsForEntityType: EKEntityTypeEvent];
    else
#endif
    t_calendars = [m_event_store calendars];
    
    if ((t_calendars != nil) && [t_calendars count])
    {
		r_chosen = [[NSString alloc] initWithString:[[t_calendars objectAtIndex:0] title]];
        // set the label item
        for (int t_i = 1; t_i < [t_calendars count]; t_i++)
        {
            r_chosen = [r_chosen stringByAppendingFormat:@"\n%@", [[t_calendars objectAtIndex:t_i] title]];
        }
	}
}

-(void)deleteEvent: (NSString*) p_event_id withInstances: (Boolean) p_reocurring withResult: (NSString*&) r_chosen
{
    NSError **t_an_error = nil; 
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    
    EKEvent *t_event = [m_event_store eventWithIdentifier:p_event_id];
    if (t_event != nil)
    {
        r_chosen = @"Deleted";
        if (p_reocurring == true)
            [m_event_store removeEvent:t_event span:EKSpanFutureEvents error:t_an_error];
        else
            [m_event_store removeEvent:t_event span:EKSpanThisEvent error:t_an_error];
    }
}

-(MCCalendar)getEventData: (NSString*) p_event_id withGotData: (bool&) p_found_data
{
    MCCalendar t_event_data;
    
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    
    EKEvent *t_event = [m_event_store eventWithIdentifier:p_event_id];

    if (t_event == nil)
    {
        p_found_data = false;
    }
    else
    {
        t_event_data = [self createEventData:t_event];
        p_found_data = true;
    }
    return t_event_data;
}

-(void)findEvent: (NSDate*) p_start_date andEnd: (NSDate*) p_end_date withResult: (NSString*&) r_chosen
{
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    
    NSPredicate *t_predicate = [m_event_store predicateForEventsWithStartDate:p_start_date endDate:p_end_date calendars:nil];
    
    // Fetch all events that match the predicate.
    NSArray *t_events = [m_event_store eventsMatchingPredicate:t_predicate];

    if ((t_events != nil) && [t_events count])
    {
		r_chosen = [[NSString alloc] initWithString:[[t_events objectAtIndex:0] eventIdentifier]];
        // set the label item
        for (int t_i = 1; t_i < [t_events count]; t_i++)
        {
            r_chosen = [r_chosen stringByAppendingFormat:@"\n%@", [[t_events objectAtIndex:t_i] eventIdentifier]];
        }
	}
}

- (EKCalendar *)eventEditViewControllerDefaultCalendarForNewEvents:(EKEventEditViewController *)controller
{
    if (m_event_store == nil)
        m_event_store = [[EKEventStore alloc] init];
    return [m_event_store defaultCalendarForNewEvents];
}

- (void)eventEditViewController:(EKEventEditViewController *)controller didCompleteWithAction:(EKEventEditViewAction)action
{
    m_selected_events = nil;
    #ifndef __i386__
    if (action == EKEventEditViewActionSaved && controller != nil && controller.event != nil && controller.event.eventIdentifier != nil)
    {
        m_selected_events = [[NSString alloc] initWithString: controller.event.eventIdentifier];
    }
    else if (action == EKEventEditViewActionDeleted)
    {
        m_selected_events = @"Deleted";
    }
    else if (action == EKEventEditViewActionCanceled)
    {
        m_selected_events = @"Canceled";
    }
    #endif
	m_running = false;
}

#ifndef __IPHONE_4_0
- (void)eventViewController:(EKEventViewController *)controller didCompleteWithAction:(EKEventViewAction)action
{
	m_running = false;
}
#endif

-(void) handleShowEventDone
{
	m_running = false;
}

@end

bool MCSystemShowEvent(const char* p_event_id, char*& r_result)
{
    bool t_result = true;
    NSString* t_ns_result = nil;
    NSString* t_ns_event = [[NSString alloc] initWithUTF8String:p_event_id];
    MCIPhonePickEventDelegate *t_show_event;
    t_show_event = [[MCIPhonePickEventDelegate alloc] init];
	[t_show_event showViewEvent:t_ns_event withResult: t_ns_result];
    if (t_ns_result != nil)
		MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    [t_ns_result release];
    [t_show_event release];
    return t_result;
}

bool MCSystemCreateEvent(char*& r_result)
{
    bool t_result = true;
    NSString* t_ns_result = nil;
    MCIPhonePickEventDelegate *t_create_event;
    t_create_event = [[MCIPhonePickEventDelegate alloc] init];
	[t_create_event showCreateEvent: t_ns_result];
    if (t_ns_result.length > 0)
		MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    [t_ns_result release];
    return t_result;
}

bool MCSystemUpdateEvent(const char* p_event_id, char*& r_result)
{
    bool t_result = true;
    NSString* t_ns_result = nil; 
    NSString* t_ns_event = [[NSString alloc] initWithUTF8String:p_event_id];
    MCIPhonePickEventDelegate *t_update_event;
    t_update_event = [[MCIPhonePickEventDelegate alloc] init];

	if (t_ns_event != nil)
    {
        // Allow the user to update the event data
        [t_update_event showUpdateEvent: t_ns_event withResult:t_ns_result];
        if (t_ns_result != nil)
            MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    }
    
    [t_ns_result release];
    [t_ns_event release];
    return t_result;
}

bool MCSystemGetEventData(MCExecContext &r_ctxt, const char* p_event_id, MCVariableValue *&r_event_data)
{
    bool t_result;
    NSString* t_ns_event = [[NSString alloc] initWithUTF8String:p_event_id];
    MCCalendar t_event_result;
    MCIPhonePickEventDelegate *t_get_event;
    t_get_event = [[MCIPhonePickEventDelegate alloc] init];
    t_event_result = [t_get_event getEventData: t_ns_event withGotData:t_result];
    // Convert the event structure to an array of pairs
    if (t_result == true)
        MCCalendarToArrayData (r_ctxt, t_event_result, r_event_data);
    [t_ns_event release];
    return t_result;
}

bool MCSystemRemoveEvent(const char* p_event_id, bool p_reocurring, char*& r_event_id_deleted)
{
    bool t_result = true;
    NSString* t_ns_result = NULL;
    NSString* t_ns_event = [[NSString alloc] initWithUTF8String:p_event_id];
    MCIPhonePickEventDelegate *t_delete_event;
    t_delete_event = [[MCIPhonePickEventDelegate alloc] init];
    [t_delete_event deleteEvent: t_ns_event withInstances:p_reocurring withResult: t_ns_result];
    if (t_ns_result != NULL)
		MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_event_id_deleted);
    [t_ns_result release];
    [t_ns_event release];
    return t_result;
}

bool MCSystemAddEvent(MCCalendar p_new_calendar_data, char*& r_result)
{
    bool t_result = true;
    NSString* t_ns_result = NULL;
    MCIPhonePickEventDelegate *t_add_event;
    t_add_event = [[MCIPhonePickEventDelegate alloc] init];
	[t_add_event addEvent: p_new_calendar_data withResult: t_ns_result];
	if (t_ns_result != NULL)
		MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    [t_ns_result release];
    return t_result;
}

bool MCSystemGetCalendarsEvent(char*& r_result)
{
    bool t_result = true;
    NSString* t_ns_result = NULL;
    MCIPhonePickEventDelegate *t_get_calendars_event;
    t_get_calendars_event = [[MCIPhonePickEventDelegate alloc] init];
	[t_get_calendars_event getCalendarsEvent: t_ns_result];
	if (t_ns_result != NULL)
		MCCStringClone ([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    [t_ns_result release];
    return t_result;
}

bool MCSystemFindEvent(MCDateTime p_start_date, MCDateTime p_end_date, char*& r_result)
{
    MCExecPoint ep(nil, nil, nil);
    bool t_result = true;  
	NSString *t_ns_result = NULL;
    NSDate *t_start_date = NULL;
    NSDate *t_end_date = NULL;
    if (MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_start_date))
    {
        t_start_date = [NSDate dateWithTimeIntervalSince1970:ep.getnvalue()];
    }
    if (MCD_convert_from_datetime(ep, CF_SECONDS, CF_SECONDS, p_end_date))
    {
        t_end_date = [NSDate dateWithTimeIntervalSince1970:ep.getnvalue()];
    }
    if (t_start_date != NULL && t_end_date != NULL)
    {
        MCIPhonePickEventDelegate *t_find_event;
        t_find_event = [[MCIPhonePickEventDelegate alloc] init];
    	[t_find_event findEvent:t_start_date andEnd: t_end_date withResult: t_ns_result];
	    if (t_ns_result != NULL)
		      MCCStringClone([t_ns_result cStringUsingEncoding:NSMacOSRomanStringEncoding], r_result);
    }
    else 
        t_result = false;
    [t_ns_result release];
    return t_result;
    
}
