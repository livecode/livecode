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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "eventqueue.h"
#include "util.h"

#import <UIKit/UIKit.h>

#include "mbliphonecontrol.h"

////////////////////////////////////////////////////////////////////////////////

#define DELAYS_TOUCHES_WORKAROUND_MIN 700
#define DELAYS_TOUCHES_PROCESS_DELAY 0.3

////////////////////////////////////////////////////////////////////////////////
	
extern UIView *MCIPhoneGetView(void);

class MCNativeScrollerControl;

@interface MCNativeScrollViewDelegate : NSObject <UIScrollViewDelegate>
{
	MCNativeScrollerControl *m_instance;
}

- (id)initWithInstance:(MCNativeScrollerControl *)instance;
@end

@interface MCNativeViewEventForwarder : UIView
{
	UIView *m_target;
	
	// MW-2013-11-05: [[ Bug 11242 ]] If true then use our own version of delayTouches.
	bool m_delay_touches : 1;
	
	// MW-2013-11-05: [[ Bug 11242 ]] The set of touches currently delayed.
	NSMutableSet *m_delayed_touches;
}

// MW-2013-11-05: [[ Bug 11242 ]] Getter for m_delay_touches.
- (BOOL)delayTouches;

// MW-2013-11-05: [[ Bug 11242 ]] Setter for m_delay_touches.
- (void)setDelayTouches:(BOOL)value;

@end

class MCNativeScrollerControl : public MCiOSControl
{
public:
	MCNativeScrollerControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
	void HandleEvent(MCNameRef message);

	void HandleScrollEvent(void);
	void HandleEndDragEvent(bool decelerate);
	
	virtual UIView* CreateView(void);
	virtual void DeleteView(UIView *view);
	
	bool m_post_scroll_event;
	void UpdateForwarderBounds(void);
protected:
	virtual ~MCNativeScrollerControl(void);
	
private:
	MCNativeScrollViewDelegate *m_delegate;
	MCNativeViewEventForwarder *m_forwarder;
	MCRectangle32 m_content_rect;
};

////////////////////////////////////////////////////////////////////////////////

MCNativeScrollerControl::MCNativeScrollerControl(void)
{
	m_delegate = nil;
	m_forwarder = nil;
	m_content_rect.x = 0;
	m_content_rect.y = 0;
	m_content_rect.width = 0;
	m_content_rect.height = 0;
	m_post_scroll_event = true;
}

MCNativeScrollerControl::~MCNativeScrollerControl(void)
{
	[m_forwarder release];
}

MCNativeControlType MCNativeScrollerControl::GetType(void)
{
	return kMCNativeControlTypeScroller;
}

////////////////////////////////////////////////////////////////////////////////

bool MCScrollViewGetContentOffset(UIScrollView *p_view, int32_t &r_x, int32_t &r_y)
{
    if (p_view == nil)
		return false;
	
	CGPoint t_offset;
	t_offset = [p_view contentOffset];
    
    // MM-2013-11-26: [[ Bug 11485 ]] The content offset of the view will be in device pixels, so convert to user pixels before returning.
    MCGPoint t_point;
    t_point = MCNativeControlUserPointFromDevicePoint(MCGPointMake(t_offset . x, t_offset . y));
    r_x = t_point . x;
    r_y = t_point . y;
	return true;
}

Exec_stat scroller_set_property(UIScrollView *p_view, MCRectangle32 &x_content_rect, MCNativeControlProperty p_property, MCExecPoint&ep)
{
	Boolean t_bool;
	real8 t_double;
	
	switch (p_property)
	{
		case kMCNativeControlPropertyContentRectangle:
        {
            if (!MCiOSControl::ParseRectangle32(ep, x_content_rect))
				return ES_ERROR;
            
            // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
            MCGRectangle t_rect;
            t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(x_content_rect . x, x_content_rect . y, x_content_rect . width, x_content_rect . height));
            
            if (p_view != nil)
                [p_view setContentSize:CGSizeMake(t_rect . size . width, t_rect . size . height)];

            return ES_NORMAL;
        }
			
		case kMCNativeControlPropertyHScroll:
		{
			int32_t t_hscroll;
			if (!MCiOSControl::ParseInteger(ep, t_hscroll))
				return ES_ERROR;
			
			int32_t t_x, t_y;
			if (p_view != nil && MCScrollViewGetContentOffset(p_view, t_x, t_y))
            {
                // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
                MCGPoint t_offset;
                t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat)t_hscroll - x_content_rect . x, (MCGFloat) t_y));
				[p_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
            }
		}
		return ES_NORMAL;
			
		case kMCNativeControlPropertyVScroll:
		{
			int32_t t_vscroll;
			if (!MCiOSControl::ParseInteger(ep, t_vscroll))
				return ES_ERROR;
			
			int32_t t_x, t_y;
			if (p_view != nil && MCScrollViewGetContentOffset(p_view, t_x, t_y))
            {
                // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
                MCGPoint t_offset;
                t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat) t_x, (MCGFloat) t_vscroll - x_content_rect . y));
				[p_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
            }
		}
		return ES_NORMAL;

		case kMCNativeControlPropertyCanBounce:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setBounces: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyCanScrollToTop:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setScrollsToTop: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyCanCancelTouches:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
                {
					[p_view setCanCancelContentTouches: t_bool];
                    // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
                    // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
                    [p_view setDelaysContentTouches: !t_bool];
                }
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyDelayTouches:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
                {
					[p_view setDelaysContentTouches: t_bool];
                    // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
                    // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
                    [p_view setCanCancelContentTouches: !t_bool];
                }
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPagingEnabled:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setPagingEnabled: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyScrollingEnabled:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setScrollEnabled: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyDecelerationRate:
			float t_deceleration;
			if (ep.getsvalue() == "normal")
				t_deceleration = UIScrollViewDecelerationRateNormal;
			else if (ep.getsvalue() == "fast")
				t_deceleration = UIScrollViewDecelerationRateFast;
			else if (MCU_stor8(ep.getsvalue(), t_double))
				t_deceleration = t_double;
			else
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			if (p_view)
				[p_view setDecelerationRate: t_deceleration];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyIndicatorStyle:
			UIScrollViewIndicatorStyle t_style;
			if (ep.getsvalue() == "default")
				t_style = UIScrollViewIndicatorStyleDefault;
			else if (ep.getsvalue() == "white")
				t_style = UIScrollViewIndicatorStyleWhite;
			else if (ep.getsvalue() == "black")
				t_style = UIScrollViewIndicatorStyleBlack;
			else
			{
				MCeerror->add(EE_OBJECT_BADSTYLE, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			if (p_view)
				[p_view setIndicatorStyle: t_style];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyIndicatorInsets:
		{
			int2 t_left, t_top, t_right, t_bottom;
			if (MCU_stoi2x4(ep.getsvalue(), t_left, t_top, t_right, t_bottom))
			{
                // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
                MCGRectangle t_rect;
                t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top));
                
				if (p_view)
					[p_view setScrollIndicatorInsets: UIEdgeInsetsMake(t_rect . origin . y, t_rect . origin . x, t_rect . origin . y + t_rect . size . height, t_rect . origin . x + t_rect . size . width)];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
		}
		return ES_NORMAL;
			
		case kMCNativeControlPropertyShowHorizontalIndicator:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setShowsHorizontalScrollIndicator: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyShowVerticalIndicator:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setShowsVerticalScrollIndicator: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyLockDirection:
			if (MCU_stob(ep.getsvalue(), t_bool))
			{
				if (p_view)
					[p_view setDirectionalLockEnabled: t_bool];
			}
			else
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			return ES_NORMAL;
	}
	return ES_NOT_HANDLED;
}

Exec_stat MCNativeScrollerControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();

	Exec_stat t_status;
	
	switch (p_property)
	{
		case kMCNativeControlPropertyRectangle:
			Exec_stat t_status;
			t_status = MCiOSControl::Set(p_property, ep);
			if (t_status == ES_NORMAL && t_view != nil && m_forwarder != nil)
			{
				UpdateForwarderBounds();
			}
			return t_status;
			
		case kMCNativeControlPropertyHScroll:
		case kMCNativeControlPropertyVScroll:
			t_status = scroller_set_property(t_view, m_content_rect, p_property, ep);
			if (t_status == ES_NORMAL)
				UpdateForwarderBounds();
			return t_status;
			
		case kMCNativeControlPropertyDelayTouches:
			// MW-2013-11-05: [[ Bug 11242 ]] If on iOS7+ override the UIScrollView delayTouches
			//   property and use our own version.
			if (MCmajorosversion >= DELAYS_TOUCHES_WORKAROUND_MIN)
			{
				Boolean t_bool;
				if (MCU_stob(ep.getsvalue(), t_bool))
				{
					if (m_forwarder != nil)
						[m_forwarder setDelayTouches: t_bool];
				}
				else
				{
					MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
					return ES_ERROR;
				}
				
				return ES_NORMAL;
			}
			break;
	}
	
	Exec_stat t_state;
	t_state = scroller_set_property(t_view, m_content_rect, p_property, ep);

	if (t_state == ES_NOT_HANDLED)
		return MCiOSControl::Set(p_property, ep);
	else
		return t_state;
}

Exec_stat scroller_get_property(UIScrollView *p_view, const MCRectangle32 &p_content_rect, MCNativeControlProperty p_property, MCExecPoint &ep)
{
	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();
	
	switch (p_property)
	{
		case kMCNativeControlPropertyContentRectangle:
			if (p_view != nil)
				ep.setrectangle(p_content_rect);
			else
				ep.clear();
			return ES_NORMAL;
		case kMCNativeControlPropertyHScroll:
            // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
			ep.setnvalue(p_view != nil ? p_content_rect.x + MCNativeControlUserXLocFromDeviceXLoc([p_view contentOffset].x) : 0);
			return ES_NORMAL;
		case kMCNativeControlPropertyVScroll:
            // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
			ep.setnvalue(p_view != nil ? p_content_rect.y + MCNativeControlUserYLocFromDeviceYLoc([p_view contentOffset].y) : 0);
			return ES_NORMAL;
		case kMCNativeControlPropertyCanBounce:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view bounces] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyCanScrollToTop:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view scrollsToTop] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyCanCancelTouches:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view canCancelContentTouches] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyDelayTouches:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view delaysContentTouches] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyPagingEnabled:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isPagingEnabled] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyScrollingEnabled:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isScrollEnabled] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyDecelerationRate:
			ep.setnvalue(p_view != nil ? [p_view decelerationRate] : 0);
			return ES_NORMAL;
		case kMCNativeControlPropertyIndicatorStyle:
			switch ([p_view indicatorStyle])
		{
			case UIScrollViewIndicatorStyleDefault:
				ep.setsvalue("default");
				break;
			case UIScrollViewIndicatorStyleBlack:
				ep.setsvalue("black");
				break;
			case UIScrollViewIndicatorStyleWhite:
				ep.setsvalue("white");
				break;
		}
			return ES_NORMAL;
		case kMCNativeControlPropertyIndicatorInsets:
			if (p_view != nil)
			{
				UIEdgeInsets t_insets;
				t_insets = [p_view scrollIndicatorInsets];
                
                // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
                MCGRectangle t_rect;
                t_rect = MCNativeControlUserRectFromDeviceRect(MCGRectangleMake(t_insets . left, t_insets . top, t_insets . right - t_insets . left, t_insets . bottom - t_insets . top));
                
				sprintf(ep.getbuffer(I2L * 4 + 4), "%d,%d,%d,%d", (int16_t)(t_rect . origin . x), (int16_t)(t_rect . origin . y), (int16_t)(t_rect . origin . x + t_rect . size . width), (int16_t)(t_rect . origin . y + t_rect . size . height));
				ep.setstrlen();
			}
			else
				ep.clear();
			return ES_NORMAL;
		case kMCNativeControlPropertyShowVerticalIndicator:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view showsVerticalScrollIndicator] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyShowHorizontalIndicator:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view showsHorizontalScrollIndicator] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyLockDirection:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isDirectionalLockEnabled] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyTracking:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isTracking] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyDragging:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isDragging] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyDecelerating:
			ep.setsvalue(MCU_btos(p_view != nil ? [p_view isDecelerating] == YES : NO));
			return ES_NORMAL;
			
	}
	return ES_NOT_HANDLED;
}

Exec_stat MCNativeScrollerControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
	UIScrollView *t_view;
	t_view = (UIScrollView *)GetView();
	
	// MW-2013-11-05: [[ Bug 11242 ]] If on iOS7+ override the UIScrollView delayTouches
	//   property and use our own version.
	if (p_property == kMCNativeControlPropertyDelayTouches &&
		MCmajorosversion >= DELAYS_TOUCHES_WORKAROUND_MIN)
	{
		if (m_forwarder != nil)
			ep . setboolean([m_forwarder delayTouches]);
		else
			ep . setboolean(False);
		
		return ES_NORMAL;
	}
	
	Exec_stat t_status;
	t_status = scroller_get_property(t_view, m_content_rect, p_property, ep);
	if (t_status == ES_NOT_HANDLED)
		return MCiOSControl::Get(p_property, ep);

	return t_status;
}

Exec_stat MCNativeScrollerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
	switch (p_action)
	{
		case kMCNativeControlActionFlashScrollIndicators:
			if (t_view)
				[t_view flashScrollIndicators];
			return ES_NORMAL;
			
	}
	return MCiOSControl::Do(p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeScrollerControl::HandleEvent(MCNameRef p_message)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message(p_message);
		ChangeTarget(t_old_target);
	}
}

void MCNativeScrollerControl::HandleEndDragEvent(bool p_decelerate)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_args(MCM_scroller_end_drag, p_decelerate ? MCtruestring : MCfalsestring);
		ChangeTarget(t_old_target);
	}
}

void MCNativeScrollerControl::HandleScrollEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	
	int32_t t_x, t_y;
	m_post_scroll_event = true;
	if (t_target != nil && MCScrollViewGetContentOffset(GetView(), t_x, t_y))
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_args(MCM_scroller_did_scroll, m_content_rect.x + t_x, m_content_rect.y + t_y);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCNativeScrollerControl::CreateView(void)
{
	UIScrollView *t_view;
	t_view = [[UIScrollView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCNativeScrollViewDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	m_forwarder = [[MCNativeViewEventForwarder alloc] initWithFrame: CGRectMake(0,0,0,0)];
	[t_view addSubview: m_forwarder];
	
	if (MCmajorosversion >= DELAYS_TOUCHES_WORKAROUND_MIN)
    {
		[t_view setDelaysContentTouches: NO];
        // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
        // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
        [t_view setCanCancelContentTouches: YES];
    }
	
	return t_view;
}

void MCNativeScrollerControl::DeleteView(UIView *p_view)
{
	[((UIScrollView*)p_view) setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
}

void MCNativeScrollerControl::UpdateForwarderBounds(void)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
	[m_forwarder setFrame: CGRectMake([t_view contentOffset].x, [t_view contentOffset].y, [t_view bounds].size.width, [t_view bounds].size.height)];
}

////////////////////////////////////////////////////////////////////////////////

class MCNativeScrollerEvent : public MCCustomEvent
{
public:
	MCNativeScrollerEvent(MCNativeScrollerControl *p_target, MCNameRef p_message)
	{
		m_target = p_target;
		m_target->Retain();
		m_message = p_message;
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleEvent(m_message);
	}
	
private:
	MCNativeScrollerControl *m_target;
	MCNameRef m_message;
};
	
class MCNativeScrollerEndDragEvent : public MCCustomEvent
{
public:
	MCNativeScrollerEndDragEvent(MCNativeScrollerControl *p_target, bool p_decelerate)
	{
		m_target = p_target;
		m_target->Retain();
		m_decelerate = p_decelerate;
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleEndDragEvent(m_decelerate);
	}
	
private:
	MCNativeScrollerControl *m_target;
	bool m_decelerate;
};


class MCNativeScrollerScrollEvent : public MCCustomEvent
{
public:
	MCNativeScrollerScrollEvent(MCNativeScrollerControl *p_target)
	{
		m_target = p_target;
		m_target->Retain();
	}
	
	void Destroy(void)
	{
		m_target->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target->HandleScrollEvent();
	}
	
private:
	MCNativeScrollerControl *m_target;
};

////////////////////////////////////////////////////////////////////////////////

@implementation MCNativeScrollViewDelegate

- (id)initWithInstance:(MCNativeScrollerControl*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	
	return self;
}

- (void)scrollViewWillBeginDragging: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeScrollerEvent(m_instance, MCM_scroller_begin_drag);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDragging: (UIScrollView*)scrollView willDecelerate:(BOOL)decelerate
{
	MCCustomEvent *t_event;
	t_event = new MCNativeScrollerEndDragEvent(m_instance, decelerate);
	MCEventQueuePostCustom(t_event);

	m_instance->UpdateForwarderBounds();
}

- (void)scrollViewDidScroll: (UIScrollView*)scrollView
{
	if (m_instance != nil && m_instance->m_post_scroll_event)
	{
		m_instance->m_post_scroll_event = false;
		MCCustomEvent *t_event;
		t_event = new MCNativeScrollerScrollEvent(m_instance);
		MCEventQueuePostCustom(t_event);
	}
}

- (void)scrollViewDidScrollToTop: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeScrollerEvent(m_instance, MCM_scroller_scroll_to_top);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeScrollerEvent(m_instance, MCM_scroller_begin_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDecelerating: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeScrollerEvent(m_instance, MCM_scroller_end_decelerate);
	MCEventQueuePostCustom(t_event);

	m_instance->UpdateForwarderBounds();
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativeScrollerControlCreate(MCNativeControl *&r_control)
{
	r_control = new MCNativeScrollerControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

@implementation MCNativeViewEventForwarder

- (id) initWithFrame: (CGRect)withFrame
{
	self = [super initWithFrame: withFrame];
	if (self != nil)
	{
		// MW-2011-09-21: [[ Bug 9738 ]] We can't put native controls inside the
		//   'main' UIView anymore because it affects OpenGLness. Thus we now just
		//   set target to be our main view.		
		m_target = MCIPhoneGetView();
		
		// MW-2013-11-05: [[ Bug 11242 ]] The default value of delaysContentTouches in
		//   the UIScrollView is true, so mimic that.
		m_delay_touches = true;
		
		// MW-2013-11-05: [[ Bug 11242 ]] This set holds the currently delayed touches.
		m_delayed_touches = [[NSMutableSet alloc] initWithCapacity: 0];
	}
	return self;
}

- (void) dealloc
{
	// MW-2013-11-05: [[ Bug 11242 ]] Make sure we have no outstanding requests on the
	//   event loop.
	[NSObject cancelPreviousPerformRequestsWithTarget: self];
	
	// MW-2013-11-05: [[ Bug 11242 ]] Dispose of the delayed touches set.
	[m_delayed_touches release];
	
	[super dealloc];
}

- (void)setDelayTouches: (BOOL)value
{
	m_delay_touches = (value == YES);
}

- (BOOL)delayTouches
{
	return m_delay_touches;
}

// MW-2013-11-05: [[ Bug 11242 ]] Processes a touch after a short delay.
- (void)processTouch: (UITouch *)touch
{
	// If the touch has already been cancelled, ignore.
	if (![m_delayed_touches containsObject: touch])
		return;
	
	// Remove the touch from the delayed set.
	[m_delayed_touches removeObject: touch];
	
	if (m_target != nil)
	{
		NSSet *t_touch_set;
		t_touch_set = [[NSSet alloc] initWithObjects: touch, nil];
		
		// Begin the touch.
		[m_target touchesBegan: t_touch_set withEvent: nil];
		
		// If its already ended, then end it.
		if ([touch phase] == UITouchPhaseEnded)
			[m_target touchesEnded: t_touch_set withEvent: nil];
		
		[t_touch_set release];
	}
}

- (void) touchesBegan: (NSSet*)touches withEvent: (UIEvent*)event
{
	// MW-2013-11-05: [[ Bug 11242 ]] If we are delaying touches, then defer processing.
	if (m_delay_touches)
	{
		for(UITouch *t_touch in touches)
		{
			[m_delayed_touches addObject: t_touch];
			[self performSelector: @selector(processTouch:) withObject: t_touch afterDelay: DELAYS_TOUCHES_PROCESS_DELAY];
		}

		return;
	}
	
	if (m_target != nil)
		[m_target touchesBegan: touches withEvent: event];
}

- (void)touchesCancelled: (NSSet*) touches withEvent: (UIEvent*)event
{
	// MW-2013-11-05: [[ Bug 11242 ]] If we are delaying touches, then remove all cancelled
	//   touches from the delayed set, and process the rest.
	if (m_delay_touches)
	{
		NSMutableSet *t_nondelayed_touches;
		t_nondelayed_touches = [[NSMutableSet alloc] initWithSet: touches];
		[t_nondelayed_touches minusSet: m_delayed_touches];
		[m_delayed_touches minusSet: touches];
		if (m_target != nil && [t_nondelayed_touches count] != 0)
			[m_target touchesCancelled: t_nondelayed_touches withEvent: event];
		[t_nondelayed_touches release];
		return;
	}
	
	if (m_target != nil)
		[m_target touchesCancelled: touches withEvent: event];
}

- (void)touchesEnded: (NSSet*) touches withEvent: (UIEvent*)event
{
	// MW-2013-11-05: [[ Bug 11242 ]] If we are delaying touches, ignore any which
	//   are in the delayed set.
	if (m_delay_touches)
	{
		NSMutableSet *t_nondelayed_touches;
		t_nondelayed_touches = [[NSMutableSet alloc] initWithSet: touches];
		[t_nondelayed_touches minusSet: m_delayed_touches];
		if (m_target != nil && [t_nondelayed_touches count] != 0)
			[m_target touchesEnded: t_nondelayed_touches withEvent: event];
		[t_nondelayed_touches release];
		return;
	}
	
	if (m_target != nil)
		[m_target touchesEnded: touches withEvent: event];
}

- (void)touchesMoved: (NSSet*) touches withEvent: (UIEvent*)event
{
	// MW-2013-11-05: [[ Bug 11242 ]] If we are delaying touches, ignore any which
	//   are in the delayed set.
	if (m_delay_touches)
	{
		NSMutableSet *t_nondelayed_touches;
		t_nondelayed_touches = [[NSMutableSet alloc] initWithSet: touches];
		[t_nondelayed_touches minusSet: m_delayed_touches];
		if (m_target != nil && [t_nondelayed_touches count] != 0)
			[m_target touchesMoved: t_nondelayed_touches withEvent: event];
		[t_nondelayed_touches release];
		return;
	}
	
	if (m_target != nil)
		[m_target touchesMoved: touches withEvent: event];
}

@end

