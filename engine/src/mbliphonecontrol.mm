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

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "exec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbliphone.h"
#include "mbliphonecontrol.h"
#include "mbliphoneapp.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_RECTANGLE, Rectangle, MCiOSControl, Rect)
    DEFINE_RW_CTRL_PROPERTY(P_VISIBLE, Bool, MCiOSControl, Visible)
    DEFINE_RW_CTRL_PROPERTY(P_OPAQUE, Bool, MCiOSControl, Opaque)
    DEFINE_RW_CTRL_PROPERTY(P_ALPHA, UInt16, MCiOSControl, Alpha)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_BACKGROUND_COLOR, NativeControlColor, MCiOSControl, BackgroundColor)
    // SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]] Add property for ignoring voice over sensitivity
    DEFINE_RW_CTRL_PROPERTY(P_IGNORE_VOICE_OVER_SENSITIVITY, Bool, MCiOSControl, IgnoreVoiceOverSensitivity)
};

MCObjectPropertyTable MCiOSControl::kPropertyTable =
{
	&MCNativeControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSControl::kActions[] =
{
};

MCNativeControlActionTable MCiOSControl::kActionTable =
{
    &MCNativeControl::kActionTable,
    0,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

MCiOSControl::MCiOSControl(void)
{
    m_view = nil;
}

bool MCiOSControl::Create(void)
{
	m_view = CreateView();
	if (m_view == nil)
		return false;
	
	// MW-2011-09-28: Native control views layer inside the 'main view' which is
	//   used for event collection (the display view sits below it).
	[MCIPhoneGetView() addSubview: m_view];
	return true;
}

void MCiOSControl::Delete(void)
{
    // MM-2012-06-12: [[ Bug 10203]] Flag controls as deleted so that they are removed from control lists (but still being retained elsewhere)
    m_deleted = true;
	if (m_view != nil)
	{
		[m_view removeFromSuperview];
		DeleteView(m_view);
		m_view = nil;
	}
}

UIView *MCiOSControl::GetView(void)
{
	return m_view;
}

////////////////////////////////////////////////////////////////////////////////


#define MCColorComponentToFloat(c) ((c) / 65535.0)
#define MCFloatToColorComponent(f) ((f) * 65535)

bool MCiOSControl::ParseColor(const MCNativeControlColor& p_color, UIColor*& r_color)
{
	float t_red, t_green, t_blue, t_alpha;
    
    t_red = MCColorComponentToFloat(p_color . r);
    t_green = MCColorComponentToFloat(p_color . g);
    t_blue = MCColorComponentToFloat(p_color . b);
    t_alpha = MCColorComponentToFloat(p_color . a);
	
	r_color = [UIColor colorWithRed:t_red green:t_green blue:t_blue alpha:t_alpha];
	
	return true;
}

bool MCiOSControl::FormatColor(const UIColor* p_color, MCNativeControlColor& r_color)
{
	CGColorRef t_bgcolor;
	t_bgcolor = [p_color CGColor];
	const CGFloat *t_components;
    
	if (t_bgcolor != nil && CGColorSpaceGetModel(CGColorGetColorSpace(t_bgcolor)) == kCGColorSpaceModelRGB)
	{
		t_components = CGColorGetComponents(t_bgcolor);
		r_color . r = MCFloatToColorComponent(t_components[0]);
		r_color . g = MCFloatToColorComponent(t_components[1]);
		r_color . b = MCFloatToColorComponent(t_components[2]);
		r_color . a = MCFloatToColorComponent(t_components[3]);
	}
	
	return true;
}

#define MCColorComponentToFloat(c) ((c) / 65535.0)
#define MCFloatToColorComponent(f) ((f) * 65535)


////////////////////////////////////////////////////////////////////////////////

void MCiOSControl::SetRect(MCExecContext& ctxt, MCRectangle p_rect)
{
	// IM-2014-10-21: [[ Bug 13450 ]] Merge previous bugfix into refactor branch.
	// MM-2013-11-26: [[ Bug 11485 ]] The rect of the control is passed in user space. Convert to device space when setting on view.
	MCGRectangle t_rect;
	t_rect = MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_rect));
	
    
    if (m_view != nil)
        [m_view setFrame: CGRectMake(t_rect.origin.x, t_rect.origin.y, t_rect.size.width, t_rect.size.height)];
}

void MCiOSControl::SetVisible(MCExecContext& ctxt, bool p_visible)
{
    if (m_view != nil)
        [m_view setHidden: !p_visible];
}

void MCiOSControl::SetOpaque(MCExecContext& ctxt, bool p_opaque)
{
    if (m_view != nil)
        [m_view setOpaque: p_opaque];
}

void MCiOSControl::SetAlpha(MCExecContext& ctxt, uinteger_t p_alpha)
{
    if (m_view != nil)
        [m_view setAlpha: (float)p_alpha / 255.0];
}

void MCiOSControl::SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    UIColor *t_color;
    if (ParseColor(p_color, t_color) == ES_NORMAL)
        if (m_view != nil)
            [m_view setBackgroundColor: t_color];
}

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
// PM-2014-12-08: [[ Bug 13659 ]] New property of iOS native controls to allow them interact with Voice Over.
// Note that in order to make a UIWebView accessible, we have to mark its parent view as 'not-accessible'
void MCiOSControl::SetIgnoreVoiceOverSensitivity(MCExecContext& ctxt, bool p_ignore_vos)
{
    if (m_view != nil)
    {
        if (p_ignore_vos)
        {
            [m_view.superview setAccessibilityTraits:UIAccessibilityTraitNone];
            m_view.superview.isAccessibilityElement = NO;
            MCignorevoiceoversensitivity = True;
        }
        else
        {
            m_view.superview.isAccessibilityElement = YES;
#ifdef __IPHONE_5_0
            [m_view.superview setAccessibilityTraits:UIAccessibilityTraitAllowsDirectInteraction];
#endif
            MCignorevoiceoversensitivity = False;
        }

    }
}

void MCiOSControl::GetRect(MCExecContext& ctxt, MCRectangle& r_rect)
{
    if (m_view != nil)
    {
        // MM-2013-11-26: [[ Bug 11485 ]] The user expects the rect of the control to be returned in user space, so convert the views rect from device space.
        CGRect t_dev_rect;
        t_dev_rect = [m_view frame];
        
        MCGRectangle t_user_rect;
        t_user_rect = MCNativeControlUserRectFromDeviceRect(MCGRectangleMake(t_dev_rect . origin . x, t_dev_rect . origin . y, t_dev_rect . size. width, t_dev_rect . size . height));
        
        r_rect . x = (int32_t) roundf(t_user_rect.origin.x);
        r_rect . y = (int32_t) roundf(t_user_rect.origin.y);
		// PM-2015-11-23: [[ Bug 16467 ]] We want width and height, NOT right and bottom
        r_rect . width = (int32_t) roundf(t_user_rect.size.width);
        r_rect . height = (int32_t) roundf(t_user_rect.size.height);
    }
}

void MCiOSControl::GetVisible(MCExecContext& ctxt, bool& r_visible)
{
    if (m_view != nil)
        r_visible = [m_view isHidden] != True;
    else
        r_visible = false;
}

void MCiOSControl::GetOpaque(MCExecContext& ctxt, bool& r_opaque)
{
    if (m_view != nil)
        r_opaque = [m_view isOpaque] == True;
    else
        r_opaque = false;
}

void MCiOSControl::GetAlpha(MCExecContext& ctxt, uinteger_t& r_alpha)
{
    if (m_view != nil)
        r_alpha = 255 * [m_view alpha];
    else
        r_alpha = 0;
}

void MCiOSControl::GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& r_color)
{
    if (m_view != nil)
    {
        if (FormatColor([m_view backgroundColor], r_color))
            return;
    }
    else
        return;
    
    ctxt . Throw();
}

// SN-2014-12-11: [[ Merge-6.7.1-rc-4 ]]
// PM-2014-12-08: [[ Bug 13659 ]] New property of iOS native controls to allow them interact with Voice Over
void MCiOSControl::GetIgnoreVoiceOverSensitivity(MCExecContext &ctxt, bool &r_ignore_vos)
{
    if (m_view != nil)
        r_ignore_vos = MCignorevoiceoversensitivity;
    else
        r_ignore_vos = false;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2013-11-26: [[ Bug 11485 ]] When converting between user and device space on iOS,
//   we must take into account if the user wants their coord to be scaled (iPhoneUseDeviceResolution) and and scale resulting from fullscreen mode.

MCGAffineTransform MCNativeControlUserToDeviceTransform()
{
    float t_scale;
    t_scale = 1 / MCIPhoneGetNativeControlScale();
    return MCGAffineTransformPreScale(MCdefaultstackptr -> getviewtransform(), t_scale, t_scale);
}

MCGAffineTransform MCNativeControlUserFromDeviceTransform()
{
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    return MCGAffineTransformPreScale(MCGAffineTransformInvert(MCdefaultstackptr -> getviewtransform()), t_scale, t_scale);
}

////////////////////////////////////////////////////////////////////////////////
