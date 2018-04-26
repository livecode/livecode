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

// Due to licensing issues with the Inneractive SDK, support for mobile ads
// in community is disabled.

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mblad.h"

#if defined(FEATURE_INNERACTIVE) && !defined(__x86_64) && !defined(__arm64__)

#include "InneractiveAd.h"

float MCIPhoneGetNativeControlScale(void);
float MCIPhoneGetResolutionScale(void);
void MCIPhoneCallOnMainFiber(void (*handler)(void *), void *context);

////////////////////////////////////////////////////////////////////////////////

class MCiOSInneractiveAd;

@interface MCiOSInneractiveAdDelegate : NSObject
{
    MCiOSInneractiveAd *m_ad;
}

- (id)init;
- (id)initWithAd: (MCiOSInneractiveAd *)ad;
- (void)dealloc;

- (IBAction)iaAdReceived:(id)sender;
- (IBAction)iaDefaultAdReceived:(id)sender;
- (IBAction)iaAdFailed:(id)sender;
- (IBAction)iaAdClicked:(id)sender;
- (IBAction)iaAdWillShow:(id)sender;
- (IBAction)iaAdDidShow:(id)sender;
- (IBAction)iaAdWillHide:(id)sender;
- (IBAction)iaAdDidHide:(id)sender;
- (IBAction)iaAdWillClose:(id)sender;
- (IBAction)iaAdDidClose:(id)sender;
- (IBAction)iaAdWillResize:(id)sender;
- (IBAction)iaAdDidResize:(id)sender;
- (IBAction)iaAdWillExpand:(id)sender;
- (IBAction)iaAdDidExpand:(id)sender;
- (IBAction)iaAppShouldSuspend:(id)sender;
- (IBAction)iaAppShouldResume:(id)sender;

@end

class MCiOSInneractiveAd : public MCAd
{
public:
    MCiOSInneractiveAd(MCAdType p_type, uint32_t p_top_left_x, uint32_t p_top_left_y, uint32_t p_timeout, NSMutableDictionary *p_meta_data);
    
    // overridden methods
    bool Create(void);
    void Delete(void);
    
	// Get the native view of the instance.
	UIView *GetView(void);
    
    bool GetVisible(void);
    void SetVisible(bool p_visible);
    MCAdTopLeft GetTopLeft();
    void SetTopLeft(MCAdTopLeft p_top_left);    
	
private:
	// The instance's view
	UIView *m_view;
    MCAdType m_type;
    MCAdTopLeft m_top_left;
    MCiOSInneractiveAdDelegate *m_delegate;
    NSMutableDictionary *m_meta_data;
    uint32_t m_timeout;
    
    CGRect GetRect();
    IaAdType GetIaAdType();
    
};

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

static MCiOSInneractiveAdDelegate *s_ad_delegate = nil;

static void initialize_ad_delegate(void)
{
    if (s_ad_delegate == nil)
        s_ad_delegate = [[MCiOSInneractiveAdDelegate alloc] init];
}

@implementation MCiOSInneractiveAdDelegate

- (id)init
{
    self = [super init];
	if (self == nil)
		return nil;
    self -> m_ad = nil;
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdReceived:) name:@"IaAdReceived" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaDefaultAdReceived:) name:@"IaDefaultAdReceived" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdFailed:) name:@"IaAdFailed" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdClicked:) name:@"IaAdClicked" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdWillShow:) name:@"IaAdWillShow" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdDidShow:) name:@"IaAdDidShow" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdWillHide:) name:@"IaAdWillHide" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdDidHide:) name:@"IaAdDidHide" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdWillClose:) name:@"IaAdWillClose" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdDidClose:) name:@"IaAdDidClose" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdWillResize:) name:@"IaAdWillResize" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdDidResize:) name:@"IaAdDidResize" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdWillExpand:) name:@"IaAdWillExpand" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAdDidExpand:) name:@"IaAdDidExpand" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAppShouldSuspend:) name:@"IaAppShouldSuspend" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iaAppShouldResume:) name:@"IaAppShouldResume" object:nil];
    
	return self;
}

- (id)initWithAd: (MCiOSInneractiveAd *)ad
{
    self = [super init];
	if (self == nil)
		return nil;	
	self -> m_ad = ad;
    
    UIView *t_view;
    t_view = m_ad->GetView();
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onReceiveAd:) name:@"onReceiveAd" object:t_view];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onFailedToReceiveAd:) name:@"onFailedToReceiveAd" object:t_view];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onReceiveDefaultAd:) name:@"onReceiveDefaultAd" object:t_view];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onClickAd:) name:@"onClickAd" object:t_view];
        
	return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [super dealloc];
}

- (IBAction)iaAdReceived:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeReceive);
}
- (IBAction)iaDefaultAdReceived:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeReceiveDefault);
}
- (IBAction)iaAdFailed:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeReceiveFailed);
}
- (IBAction)iaAdClicked:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeClick);
}
- (IBAction)iaAdWillShow:(id)sender
{
    // The ad is about to show
}
- (IBAction)iaAdDidShow:(id)sender
{
    // The ad did show
}
- (IBAction)iaAdWillHide:(id)sender
{
    // The ad is about to hide
}
- (IBAction)iaAdDidHide:(id)sender
{
    // The ad did hide
}
- (IBAction)iaAdWillClose:(id)sender
{
    // The ad is about to close
}
- (IBAction)iaAdDidClose:(id)sender
{
    // The ad did close
}
- (IBAction)iaAdWillResize:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeResizeStart);
}
- (IBAction)iaAdDidResize:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeResizeEnd);
}
- (IBAction)iaAdWillExpand:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeExpandStart);
}
- (IBAction)iaAdDidExpand:(id)sender
{
    MCAdPostMessage(m_ad, kMCAdEventTypeExpandEnd);
}
- (IBAction)iaAppShouldSuspend:(id)sender
{
    // The app should suspend (for example, when the ad expands)
}
- (IBAction)iaAppShouldResume:(id)sender
{
    // The app should resume (for example, when the ad collapses)
}
@end

////////////////////////////////////////////////////////////////////////////////

MCiOSInneractiveAd::MCiOSInneractiveAd(MCAdType p_type, uint32_t p_top_left_x, uint32_t p_top_left_y, uint32_t p_timeout, NSMutableDictionary *p_meta_data)
{
    m_view = nil;
    m_delegate = nil;
    m_type = p_type;
    m_timeout = p_timeout;
    m_meta_data = p_meta_data;
    if (m_type != kMCAdTypeFullscreen)
    {
        m_top_left.x = p_top_left_x;
        m_top_left.y = p_top_left_y;
    }
    else
    {
        m_top_left.x = 0;
        m_top_left.y = 0;
    }
}

struct display_ad_context_t
{
    IaAdType type;
    UIView *view;
    uint32_t timeout;
    NSMutableDictionary *meta_data;
    bool result;
};

static void do_display_ad(void *p_context)
{
    display_ad_context_t *ctxt;
    ctxt = (display_ad_context_t *)p_context;
    
    ctxt -> result = [InneractiveAd DisplayAd:MCStringConvertToAutoreleasedNSString(MCAdGetInneractiveKey())
                                     withType:ctxt -> type 
                                     withRoot:ctxt -> view
                                   withReload:ctxt -> timeout
                                   withParams: ctxt -> meta_data];
}

bool MCiOSInneractiveAd::Create(void)
{
    initialize_ad_delegate();
    
	UIView *t_view;
	t_view = [[UIView alloc] initWithFrame: GetRect()];
	if (t_view == nil)
		return false;
    
	m_view = t_view;
    [MCIPhoneGetView() addSubview: m_view];
    
    // MM-2012-08-15: Make sure all ads are created on the main thread.
    display_ad_context_t t_context;
    t_context . type = GetIaAdType();
    t_context . view = m_view;
    t_context . timeout = m_timeout;
    t_context . meta_data = m_meta_data;
    MCIPhoneCallOnMainFiber(do_display_ad, &t_context);    
    if (!t_context . result)
    {
        Delete();
        return false;
    }
    
    [m_view setHidden: NO];
    
    if (m_meta_data != nil)
        [m_meta_data release];
    m_meta_data = nil;
    
    return true;
}


void MCiOSInneractiveAd::Delete(void)
{
	if (m_view != nil)
	{
		[m_view removeFromSuperview];
        if (m_view != nil)
            [m_view release];
        m_view = nil;
	}
    if (m_delegate != nil)
        [m_delegate release];
    m_delegate = nil;
    if (m_meta_data != nil)
        [m_meta_data release];
    m_meta_data = nil;
}

UIView *MCiOSInneractiveAd::GetView(void)
{
    if (m_view != nil)
    {
        NSArray *t_sub_views;
        t_sub_views = [m_view subviews];
        if (t_sub_views != nil && [t_sub_views count] != 0)
        {
            return (UIView *) [t_sub_views lastObject];
        }
    }
	return m_view;
}

////////////////////////////////////////////////////////////////////////////////

bool MCiOSInneractiveAd::GetVisible()
{
    if (m_view != nil)
        return ![m_view isHidden];
    return false;
}

void MCiOSInneractiveAd::SetVisible(bool p_visible)
{
    if (m_view != nil)
    {
        if (p_visible)
            [m_view setHidden: NO];
        else
            [m_view setHidden: YES];
    }        
}

MCAdTopLeft MCiOSInneractiveAd::GetTopLeft()
{
    return m_top_left;
}

void MCiOSInneractiveAd::SetTopLeft(MCAdTopLeft p_top_left)
{
    if (m_type != kMCAdTypeFullscreen)
    {
        m_top_left = p_top_left;
        if (m_view != nil)
            [m_view setFrame:GetRect()];
    }
}

////////////////////////////////////////////////////////////////////////////////

CGRect MCiOSInneractiveAd::GetRect()
{    
    float t_width, t_height;
    if (m_type == kMCAdTypeFullscreen)
        t_width = t_height = 1;
    else if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
    {
        t_width = 768;
        t_height = 66;
    }
    else
    {
        t_width = 320;
        t_height = 53;
    }
        
    float t_scale, t_device_scale;
    t_scale = MCIPhoneGetResolutionScale();
    t_device_scale = MCIPhoneGetNativeControlScale();
    return CGRectMake(m_top_left.x / t_device_scale, m_top_left.y / t_device_scale, t_width * t_scale, t_height * t_scale);
}

IaAdType MCiOSInneractiveAd::GetIaAdType()
{
    switch (m_type)
    {
        case kMCAdTypeText:
            return IaAdType_Text;
        case kMCAdTypeFullscreen:
            return IaAdType_FullScreen;
        default:
            return IaAdType_Banner;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCSystemInneractiveAdInit()
{
}

bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd*& r_ad, MCAdType p_type, uint32_t p_top_left_x, uint32_t p_top_left_y, uint32_t p_timeout, MCArrayRef p_meta_data)
{    
    bool t_success;
    t_success = true;
    
    NSMutableDictionary *t_meta_data;
    t_meta_data = [[NSMutableDictionary alloc] init];
    if (p_meta_data != nil)
    {
        MCValueRef t_value;
        
        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("age"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Age]];

        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("distribution id"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Distribution_Id]];
        
        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("gender"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Gender]];
        
        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("coordinates"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Gps_Coordinates]];

        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("keywords"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Keywords]];
        
        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("location"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Location]];
        
        if (t_success)
            if (MCArrayFetchValue(p_meta_data, false, MCNAME("phone number"), t_value))
                [t_meta_data setObject:MCStringConvertToAutoreleasedNSString(((MCStringRef)t_value)) forKey:[NSNumber numberWithInt:Key_Msisdn]];
    }
    
    if (t_success)
        r_ad = new MCiOSInneractiveAd(p_type, p_top_left_x, p_top_left_y, p_timeout, t_meta_data);
    
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#else

void MCSystemInneractiveAdInit()
{
}

bool MCSystemInneractiveAdCreate(MCExecContext &ctxt, MCAd*& r_ad, MCAdType p_type, uint32_t p_top_left_x, uint32_t p_top_left_y, uint32_t p_timeout, MCArrayRef p_meta_data)
{
	return false;
}

#endif

