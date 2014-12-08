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

#include "mcerror.h"
#include "execpt.h"
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

#include "mbliphone.h"
#include "mbliphonecontrol.h"
#include "mbliphoneapp.h"

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

Exec_stat MCiOSControl::ParseColor(MCExecPoint& ep, UIColor*& r_color)
{
	float t_red, t_green, t_blue, t_alpha;
	uint16_t t_r16, t_g16, t_b16, t_a16;
	bool t_parsed = true;
    
    t_parsed = MCNativeControl::ParseColor(ep, t_r16, t_g16, t_b16, t_a16);
    
	if (!t_parsed)
	{
		MCeerror->add(EE_OBJECT_BADCOLOR, 0, 0, ep.getsvalue());
		return ES_ERROR;
	}
	
    t_red = MCColorComponentToFloat(t_r16);
    t_green = MCColorComponentToFloat(t_g16);
    t_blue = MCColorComponentToFloat(t_b16);
    t_alpha = MCColorComponentToFloat(t_a16);
	
	r_color = [UIColor colorWithRed:t_red green:t_green blue:t_blue alpha:t_alpha];
	
	return ES_NORMAL;
}

Exec_stat MCiOSControl::FormatColor(MCExecPoint& ep, UIColor *p_color)
{
	CGColorRef t_bgcolor;
	uint8_t t_red, t_green, t_blue, t_alpha;
	t_bgcolor = [p_color CGColor];
	const CGFloat *t_components;

	if (t_bgcolor == nil)
		ep.clear();
	else if (CGColorSpaceGetModel(CGColorGetColorSpace(t_bgcolor)) == kCGColorSpaceModelRGB)
	{
		t_components = CGColorGetComponents(t_bgcolor);
		t_red = MCFloatToColorComponent(t_components[0]);
		t_green = MCFloatToColorComponent(t_components[1]);
		t_blue = MCFloatToColorComponent(t_components[2]);
		t_alpha = MCFloatToColorComponent(t_components[3]);
        
        MCNativeControl::FormatColor(ep, t_red, t_green, t_blue, t_alpha);
	}
	else
		ep.clear();
	
	return ES_NORMAL;
}

bool MCiOSControl::ParseString(MCExecPoint& ep, NSString*& r_string)
{
	r_string = [NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding];
	return true;
}

bool MCiOSControl::FormatString(MCExecPoint& ep, NSString *p_string)
{
	// MW-2011-05-31: [[ Bug 9564 ]] It is possible for this to be called with a nil p_string.
	if (p_string == nil)
		p_string = @"";
	// MW-2013-03-12: [[ Bug 10730 ]] Make sure we use our nativeCString conversion method
	//   ( default cStringUsingEncoding method is not lossy :-( )
	ep . copysvalue([p_string nativeCString]);
	return true;
}

bool MCiOSControl::ParseUnicodeString(MCExecPoint& ep, NSString*& r_string)
{
	r_string = [NSString stringWithCharacters: (unichar*)ep . getsvalue() . getstring()
									   length: ep . getsvalue() . getlength() / 2];
	return true;
}

bool MCiOSControl::FormatUnicodeString(MCExecPoint& ep, NSString *p_string)
{
	unichar *t_buffer;
	t_buffer = (unichar*)ep.getbuffer([p_string length] * 2);
	[p_string getCharacters: t_buffer range: NSMakeRange(0, [p_string length])];
	ep.setlength([p_string length] * 2);
	return true;
}

bool MCiOSControl::ParseRange(MCExecPoint &ep, NSRange &r_range)
{
	uint32_t d1, d2;
    if (!MCNativeControl::ParseRange(ep, d1, d2))
        return false;
    
	r_range = NSMakeRange(d1 - 1, d2);
	return true;
}

bool MCiOSControl::FormatRange(MCExecPoint &ep, NSRange r_range)
{
    return MCNativeControl::FormatRange(ep, r_range.location + 1, r_range.length);
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCiOSControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	switch(p_property)
	{
		case kMCNativeControlPropertyRectangle:
		{
			int2 i1, i2, i3, i4;
			if (MCU_stoi2x4(ep . getsvalue(), i1, i2, i3, i4))
			{
                // MM-2013-11-26: [[ Bug 11485 ]] The rect of the control is passed in user space. Convert to device space when setting on view.
                MCGRectangle t_rect;
                t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(i1, i2, i3 - i1, i4 -i2));
                
                if (m_view != nil)
					[m_view setFrame: CGRectMake(t_rect . origin . x, t_rect . origin . y, t_rect . size . width, t_rect . size . height)];
            }
			else
			{
				MCeerror->add(EE_OBJECT_NAR, 0, 0, ep . getsvalue());
				return ES_ERROR;
			}
		}
		return ES_NORMAL;
			
		case kMCNativeControlPropertyVisible:
		{
			Boolean t_value;
			if (MCU_stob(ep . getsvalue(), t_value))
			{
				if (m_view != nil)
					[m_view setHidden: !t_value];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep . getsvalue());
				return ES_ERROR;
			}
		}
		return ES_NORMAL;
		
		case kMCNativeControlPropertyOpaque:
		{
			Boolean t_value;
			if (MCU_stob(ep.getsvalue(), t_value))
			{
				if (m_view != nil)
					[m_view setOpaque: t_value];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
		}
		return ES_NORMAL;
		
		case kMCNativeControlPropertyAlpha:
		{
			uint2 t_alpha;
			if (MCU_stoui2(ep.getsvalue(), t_alpha))
			{
				if (m_view != nil)
					[m_view setAlpha: t_alpha / 255.0];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
		}
		return ES_NORMAL;
		
		case kMCNativeControlPropertyBackgroundColor:
		{
			UIColor *t_color;
			if (ParseColor(ep, t_color) == ES_NORMAL)
				if (m_view != nil)
					[m_view setBackgroundColor: t_color];
		}
		return ES_NORMAL;
        
		// PM-2014-12-08: [[ Bug 13659 ]] New property of iOS native controls to allow them interact with Voice Over.
        // Note that in order to make a UIWebView accessible, we have to mark its parent view as 'not-accessible'
        case kMCNativeControlPropertyIgnoreVoiceOverSensitivity:
        {
            if (m_view != nil)
            {
                if (ep . getsvalue() == MCtruemcstring)
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
        return ES_NORMAL;
            
		default:
			break;
	}
	
	return ES_NOT_HANDLED;
}

Exec_stat MCiOSControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	switch (p_property)
	{
		case kMCNativeControlPropertyId:
			ep . setnvalue(GetId());
			return ES_NORMAL;
		
		case kMCNativeControlPropertyName:
			ep . copysvalue(GetName() == nil ? "" : GetName());
			return ES_NORMAL;
			
		case kMCNativeControlPropertyRectangle:
		{
			if (m_view != nil)
			{
                // MM-2013-11-26: [[ Bug 11485 ]] The user expects the rect of the control to be returned in user space, so convert the views rect from device space.
				CGRect t_dev_rect;
				t_dev_rect = [m_view frame];
                
                MCGRectangle t_user_rect;
                t_user_rect = MCNativeControlUserRectFromDeviceRect(MCGRectangleMake(t_dev_rect . origin . x, t_dev_rect . origin . y, t_dev_rect . size. width, t_dev_rect . size . height));
                
				ep . setrectangle((int32_t) roundf(t_user_rect.origin.x), (int32_t) roundf(t_user_rect.origin.y),
                                  (int32_t) (roundf(t_user_rect.origin.x) + roundf(t_user_rect.size.width)),
                                  (int32_t) (roundf(t_user_rect.origin.y) + roundf(t_user_rect.size.height)));
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyVisible:
			if (m_view != nil)
				ep.setsvalue(MCU_btos(![m_view isHidden]));
			return ES_NORMAL;
			
		case kMCNativeControlPropertyOpaque:
			if (m_view != nil)
				ep.setsvalue(MCU_btos([m_view isOpaque]));
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAlpha:
			if (m_view != nil)
				ep.setnvalue(255 * [m_view alpha]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyBackgroundColor:
			if (m_view != nil)
				FormatColor(ep, [m_view backgroundColor]);
			return ES_NORMAL;
          
        // PM-2014-12-08: [[ Bug 13659 ]] New property of iOS native controls to allow them interact with Voice Over
        case kMCNativeControlPropertyIgnoreVoiceOverSensitivity:
            ep.setsvalue(MCU_btos(m_view != nil ? MCignorevoiceoversensitivity == True : NO));
            return ES_NORMAL;
            
        default:
            break;
		}
	}
	return ES_ERROR;
}

Exec_stat MCiOSControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	return ES_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2013-11-26: [[ Bug 11485 ]] When converting between user and device space on iOS,
//   we must take into account if the user wants their coord to be scaled (iPhoneUseDeviceResolution) and and scale resulting from fullscreen mode.

MCGAffineTransform MCNativeControlUserToDeviceTransform()
{
    float t_scale;
    t_scale = 1 / MCIPhoneGetNativeControlScale();
    return MCGAffineTransformScale(MCdefaultstackptr -> getviewtransform(), t_scale, t_scale);
}

MCGAffineTransform MCNativeControlUserFromDeviceTransform()
{
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    return MCGAffineTransformScale(MCGAffineTransformInvert(MCdefaultstackptr -> getviewtransform()), t_scale, t_scale);
}

////////////////////////////////////////////////////////////////////////////////