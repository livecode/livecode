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

#import <UIKit/UIKit.h>
#import <MessageUI/MessageUI.h>
#import <QuartzCore/QuartzCore.h>

float MCIPhoneGetNativeControlScale(void);
UIView *MCIPhoneGetView(void);

static UIActivityIndicatorView *s_activity_indicator = nil;

bool MCSystemActivityIndicatorStart (intenum_t p_indicator, integer_t p_location_x, integer_t p_location_y)
{
    // Get the style of the activity indicator.
    UIActivityIndicatorViewStyle t_view_style;
	if (p_indicator == kMCActivityIndicatorWhiteLarge)
		t_view_style = UIActivityIndicatorViewStyleWhiteLarge;
	else if (p_indicator == kMCActivityIndicatorGray)
		t_view_style = UIActivityIndicatorViewStyleGray;
	else t_view_style = UIActivityIndicatorViewStyleWhite;
    // Set the default location of the activity indicator, if the user has not specified it.
    if (p_location_x == p_location_y && p_location_x == -1)
    {
        p_location_x = MCIPhoneGetView().bounds.size.width/2;
        p_location_y = MCIPhoneGetView().bounds.size.height/2;
    }
    else
    {
        // MM-2012-02-29: [[ BUG 9957 ]] - activity indicator placement ignores device resolution.
        float t_scale;
        t_scale = MCIPhoneGetNativeControlScale();
        p_location_x = p_location_x / t_scale;
        p_location_y = p_location_y / t_scale;
    }
    // If an activity indicator already exists, then terminate it first.
	if (s_activity_indicator != nil)
	{
		[s_activity_indicator stopAnimating];
		[s_activity_indicator removeFromSuperview];
		[s_activity_indicator release];
	}
    // Allocate, show and animate the activity indicator.
	s_activity_indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:t_view_style];
	[MCIPhoneGetView() addSubview:s_activity_indicator];
	[s_activity_indicator setFrame:CGRectMake(p_location_x - [s_activity_indicator frame].size.width/2,
                                              p_location_y - [s_activity_indicator frame].size.height/2,
                                              [s_activity_indicator frame].size.height, 
                                              [s_activity_indicator frame].size.width)];
	[s_activity_indicator startAnimating];
	return true;
}

bool MCSystemActivityIndicatorStop ()
{
	if (s_activity_indicator != nil)
	{
		[s_activity_indicator stopAnimating];
		[s_activity_indicator removeFromSuperview];
		[s_activity_indicator release];
		s_activity_indicator = nil;
	}
	return true;
}
