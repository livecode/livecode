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

#include "exec.h"
#include "mblsyntax.h"

#include "mbliphoneapp.h"

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import <QuartzCore/QuartzCore.h>

UIView *MCIPhoneGetView(void);

@interface com_runrev_livecode_MCBusyIndicator : NSObject <UIApplicationDelegate, UIActionSheetDelegate>
{
    UIView* m_indicator_view;
    UIView* m_view;
    UIView* m_alpha;
    UILabel* m_label;
    UIActivityIndicatorView* m_busy_icon;
 }
@end

static com_runrev_livecode_MCBusyIndicator *s_busy_indicator = nil;

@implementation com_runrev_livecode_MCBusyIndicator

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
- (void) showBusy: (NSString*) p_title withOpacity: (int32_t) p_opacity
{
	// MW-2012-10-08: [[ Bug 10441 ]] Use GetViewBounds() rather than the bounds of the
	//   actual view since this can be called before the view has been positioned.
    // Grab application frames
    CGRect t_app_frame = MCIPhoneGetViewBounds();
    
    // Base screen
    m_view = [[UIView alloc] initWithFrame:CGRectMake (0, 0, t_app_frame.size.width, t_app_frame.size.height)];
    m_view.backgroundColor = [UIColor clearColor];
    [m_view retain];
    
    // Alpha screen
    m_alpha = [[UIView alloc] initWithFrame:CGRectMake (0, 0, t_app_frame.size.width, t_app_frame.size.height)];
    m_alpha.backgroundColor = [UIColor blackColor];
    m_alpha.alpha = 0.35;

    [m_view addSubview:m_alpha];
    [m_alpha retain];
    
    // Create the indicator
    CGSize t_busy_size;
    int32_t t_icon_offset;
    if ([p_title length] == 0)
    {
        t_busy_size = CGSizeMake (120, 120);
        t_icon_offset = 0;
    }
    else
    {
        t_busy_size = CGSizeMake (160, 160);
        t_icon_offset = 25;
    }
    m_indicator_view = [[UIView alloc] initWithFrame:CGRectMake (0, 0, t_busy_size.width, t_busy_size.height)];
    m_indicator_view.backgroundColor = [UIColor blackColor];
    m_indicator_view.alpha = (p_opacity > 0) ? (float) p_opacity / 100 : 0.42;
    
    // Round corners the corners of the m_indicator_view
    [[m_indicator_view layer] setCornerRadius:16];
    
    // Clip bounds
    [m_indicator_view setClipsToBounds:YES];
    
    [m_view addSubview:m_indicator_view];
    [m_indicator_view retain];
    
    // Create the activity indicator
    m_busy_icon = [[UIActivityIndicatorView alloc] 
                   initWithActivityIndicatorStyle:
                   UIActivityIndicatorViewStyleWhiteLarge];
    [m_busy_icon startAnimating];
    
    // Set the frame for the indicator
    m_busy_icon.frame =  CGRectMake (t_busy_size.width/2 - m_busy_icon.frame.size.width/2,
                                     t_busy_size.height/2 - m_busy_icon.frame.size.height/2 - t_icon_offset,
                                     m_busy_icon.frame.size.width,
                                     m_busy_icon.frame.size.height);
    
    [m_indicator_view addSubview:m_busy_icon];
    [m_busy_icon retain];
    
    // Create the text
    m_label = [[UILabel alloc] initWithFrame:CGRectMake (10, t_busy_size.height - 85, t_busy_size.width - 20, 70)];
    m_label.textColor = [UIColor whiteColor];
    m_label.textAlignment = NSTextAlignmentCenter;
    
    // PM-2015-03-16: [[ Bug 14946 ]] Allow up to 3 lines for the text
    m_label.numberOfLines = 3;
#ifdef __IPHONE_6_0
    m_label.lineBreakMode = NSLineBreakByWordWrapping;
#else
    m_label.lineBreakMode = UILineBreakModeWordWrap;
#endif
    
    m_label.backgroundColor = [UIColor clearColor];
    [m_indicator_view addSubview:m_label];
    [m_label retain];
    
    // Calculate the indicator view frame
    CGRect t_busy_frame = CGRectMake (t_app_frame.size.width/2 - t_busy_size.width/2, 
                                      t_app_frame.size.height/2 - t_busy_size.height/2, 
                                      t_busy_size.width, t_busy_size.height);
    m_indicator_view.frame = t_busy_frame;
    m_label.text = p_title;
        
    // Stop user interaction with the application
    MCIPhoneGetView().userInteractionEnabled = NO;
    [MCIPhoneGetView() addSubview: m_view];
}

- (void) hideBusy
{
    [m_busy_icon stopAnimating];
    [m_busy_icon removeFromSuperview];
    [m_busy_icon release];
    
    [m_label removeFromSuperview];
    [m_label release];
    
    [m_indicator_view removeFromSuperview];
    [m_indicator_view release];
    
    [m_alpha removeFromSuperview];
    [m_alpha release];

    [m_view removeFromSuperview];
    [m_view release];
    
    // Allow user interaction with the application
    MCIPhoneGetView().userInteractionEnabled = YES;
}

@end

// MM-2013-02-04: [[ Bug 10642 ]] Added new optional opacity parameter to busy indicator.
bool MCSystemBusyIndicatorStart (intenum_t p_indicator, MCStringRef p_label, int32_t p_opacity)
{
    switch (p_indicator)
    {
        case kMCBusyIndicatorInLine:      
            return true;
        case kMCBusyIndicatorSquare:
        {
            if (s_busy_indicator != nil)
            {
                [s_busy_indicator hideBusy];
                [s_busy_indicator release];
            }
            s_busy_indicator = [[com_runrev_livecode_MCBusyIndicator alloc] init];
            if (p_label == nil)
                [s_busy_indicator showBusy:@"" withOpacity:p_opacity];
            else
                // TODO - update for unicode. Change false to the appropriate value.
                [s_busy_indicator showBusy:MCStringConvertToAutoreleasedNSString(p_label) withOpacity:p_opacity];
                
            return true;
        }
        case kMCBusyIndicatorKeyboard:
            return true;
        default:
            return false;
    }
}

bool MCSystemBusyIndicatorStop ()
{
    if (s_busy_indicator != nil)
    {
        [s_busy_indicator hideBusy];
        [s_busy_indicator release];
        s_busy_indicator = nil;
    }
    return true;
}
