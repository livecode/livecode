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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"

#include "globals.h"
#include "stack.h"
#include "eventqueue.h"
#include "util.h"

#include "exec.h"

#import <UIKit/UIKit.h>

#include "mbliphonecontrol.h"
#include "mblcontrol.h"

////////////////////////////////////////////////////////////////////////////////

#define DELAYS_TOUCHES_WORKAROUND_MIN 700
#define DELAYS_TOUCHES_PROCESS_DELAY 0.3

////////////////////////////////////////////////////////////////////////////////
	
extern UIView *MCIPhoneGetView(void);

class MCiOSScrollerControl;

@interface com_runrev_livecode_MCiOSScrollViewDelegate : NSObject <UIScrollViewDelegate>
{
	MCiOSScrollerControl *m_instance;
}

- (id)initWithInstance:(MCiOSScrollerControl *)instance;
@end

@interface com_runrev_livecode_MCNativeViewEventForwarder : UIView
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

class MCiOSScrollerControl : public MCiOSControl
{
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
	MCiOSScrollerControl(void);
	
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    virtual void SetRect(MCExecContext& ctxt, MCRectangle p_rect);
    
    void SetContentRect(MCExecContext& ctxt, integer_t p_rect[4]);
    void GetContentRect(MCExecContext& ctxt, integer_t r_rect[4]);
    void SetHScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetHScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetVScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetCanBounce(MCExecContext& ctxt, bool p_value);
    void GetCanBounce(MCExecContext& ctxt, bool& r_value);
    void SetCanScrollToTop(MCExecContext& ctxt, bool p_value);
    void GetCanScrollToTop(MCExecContext& ctxt, bool& r_value);
    void SetCanCancelTouches(MCExecContext& ctxt, bool p_value);
    void GetCanCancelTouches(MCExecContext& ctxt, bool& r_value);
    void SetDelayTouches(MCExecContext& ctxt, bool p_value);
    void GetDelayTouches(MCExecContext& ctxt, bool& r_value);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void SetPagingEnabled(MCExecContext& ctxt, bool p_value);
    void GetPagingEnabled(MCExecContext& ctxt, bool& r_value);
    void SetDecelerationRate(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_rate);
    void GetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate);
    void SetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle p_style);
    void GetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle& r_style);
    void SetIndicatorInsets(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_insets);
    void GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets);
    void SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value);
    void SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value);
    void SetLockDirection(MCExecContext& ctxt, bool p_value);
    void GetLockDirection(MCExecContext& ctxt, bool& r_value);
    
    void GetTracking(MCExecContext& ctxt, bool& r_value);
    void GetDragging(MCExecContext& ctxt, bool& r_value);
    void GetDecelerating(MCExecContext& ctxt, bool& r_value);
    
	// Scroller-specific actions
	void ExecFlashScrollIndicators(MCExecContext& ctxt);
    
	void HandleEvent(MCNameRef message);

	void HandleScrollEvent(void);
	void HandleEndDragEvent(bool decelerate);
	
	virtual UIView* CreateView(void);
	virtual void DeleteView(UIView *view);
	
	bool m_post_scroll_event;
	void UpdateForwarderBounds(void);
protected:
	virtual ~MCiOSScrollerControl(void);
	
private:
	com_runrev_livecode_MCiOSScrollViewDelegate *m_delegate;
	com_runrev_livecode_MCNativeViewEventForwarder *m_forwarder;
	MCRectangle32 m_content_rect;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSScrollerControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT_RECT, Int32X4, MCiOSScrollerControl, ContentRect)
    DEFINE_RW_CTRL_PROPERTY(P_HSCROLL, Int32, MCiOSScrollerControl, HScroll)
    DEFINE_RW_CTRL_PROPERTY(P_VSCROLL, Int32, MCiOSScrollerControl, VScroll)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_BOUNCE, Bool, MCiOSScrollerControl, CanBounce)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_SCROLL_TO_TOP, Bool, MCiOSScrollerControl, CanScrollToTop)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_CANCEL_TOUCHES, Bool, MCiOSScrollerControl, CanCancelTouches)
    DEFINE_RW_CTRL_PROPERTY(P_DELAY_TOUCHES, Bool, MCiOSScrollerControl, DelayTouches)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCiOSScrollerControl, ScrollingEnabled)
    DEFINE_RW_CTRL_PROPERTY(P_PAGING_ENABLED, Bool, MCiOSScrollerControl, PagingEnabled)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_DECELERATION_RATE, NativeControlDecelerationRate, MCiOSScrollerControl, DecelerationRate)
    DEFINE_RW_CTRL_ENUM_PROPERTY(P_INDICATOR_STYLE, NativeControlIndicatorStyle, MCiOSScrollerControl, IndicatorStyle)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_INDICATOR_INSETS, NativeControlIndicatorInsets, MCiOSScrollerControl, IndicatorInsets)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_HORIZONTAL_INDICATOR, Bool, MCiOSScrollerControl, ShowHorizontalIndicator)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_VERTICAL_INDICATOR, Bool, MCiOSScrollerControl, ShowVerticalIndicator)
    DEFINE_RW_CTRL_PROPERTY(P_LOCK_DIRECTION, Bool, MCiOSScrollerControl, LockDirection)
    DEFINE_RO_CTRL_PROPERTY(P_TRACKING, Bool, MCiOSScrollerControl, Tracking)
    DEFINE_RO_CTRL_PROPERTY(P_DRAGGING, Bool, MCiOSScrollerControl, Dragging)
    DEFINE_RO_CTRL_PROPERTY(P_DECELERATING, Bool, MCiOSScrollerControl, Decelerating)
};

MCObjectPropertyTable MCiOSScrollerControl::kPropertyTable =
{
	&MCiOSControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSScrollerControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(FlashScrollIndicators, Void, MCiOSScrollerControl, FlashScrollIndicators)
};

MCNativeControlActionTable MCiOSScrollerControl::kActionTable =
{
    &MCiOSControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCiOSScrollerControl::MCiOSScrollerControl(void)
{
	m_delegate = nil;
	m_forwarder = nil;
	m_content_rect.x = 0;
	m_content_rect.y = 0;
	m_content_rect.width = 0;
	m_content_rect.height = 0;
	m_post_scroll_event = true;
}

MCiOSScrollerControl::~MCiOSScrollerControl(void)
{
	[m_forwarder release];
}

MCNativeControlType MCiOSScrollerControl::GetType(void)
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

void MCiOSScrollerControl::SetRect(MCExecContext& ctxt, MCRectangle p_rect)
{
    MCiOSControl::SetRect(ctxt, p_rect);
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (!ctxt . HasError() && t_view != nil && m_forwarder != nil)
        UpdateForwarderBounds();
}

void MCiOSScrollerControl::SetContentRect(MCExecContext& ctxt, integer_t p_rect[4])
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
    MCGRectangle t_rect;
    t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(p_rect[0], p_rect[1], p_rect[2] - p_rect[0], p_rect[3] - p_rect[1]));
	
	// PM-2016-01-14: [[ Bug 16722]] m_content_rect stores user-pixel values - make sure we update it
	m_content_rect . x = p_rect[0];
    m_content_rect . y = p_rect[1];
    m_content_rect . width = p_rect[2] - p_rect[0];
    m_content_rect . height = p_rect[3] - p_rect[1];
    
    if (t_view != nil)
        [t_view setContentSize:CGSizeMake(t_rect . size . width, t_rect . size . height)];
}

void MCiOSScrollerControl::GetContentRect(MCExecContext& ctxt, integer_t r_rect[4])
{
    if (GetView())
    {
        r_rect[0] = m_content_rect . x;
        r_rect[1] = m_content_rect . y;
        r_rect[2] = m_content_rect . x + m_content_rect . width;
        r_rect[3] = m_content_rect . y + m_content_rect . height;
    }
}

void MCiOSScrollerControl::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
    {
        // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
		MCGPoint t_offset;
        t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat)p_scroll, (MCGFloat) t_y));
        [t_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
    }
    
    UpdateForwarderBounds();
}

void MCiOSScrollerControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
    if (t_view)
        r_scroll = MCNativeControlUserXLocFromDeviceXLoc([t_view contentOffset].x);
    else
        r_scroll = 0;
}
void MCiOSScrollerControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
    {
        // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
		// PM-2016-03-04: [[ Bug 17038 ]] Pass the correct offset, relative to m_content_rect
        MCGPoint t_offset;
        t_offset = MCNativeControlUserPointToDevicePoint(MCGPointMake((MCGFloat) t_x, (MCGFloat) p_scroll));
        [t_view setContentOffset: CGPointMake(t_offset . x, t_offset . y)];
    }
    
    UpdateForwarderBounds();
}

void MCiOSScrollerControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
	// PM-2016-03-04: [[ Bug 17038 ]] Get the correct offset, relative to m_content_rect
    if (t_view)
        r_scroll = MCNativeControlUserYLocFromDeviceYLoc([t_view contentOffset].y);
    else
        r_scroll = 0;
}
void MCiOSScrollerControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setBounces: p_value];
}

void MCiOSScrollerControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view bounces];
    else
        r_value = false;
}

void MCiOSScrollerControl::SetCanScrollToTop(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollsToTop: p_value];
}
void MCiOSScrollerControl::GetCanScrollToTop(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view scrollsToTop];
    else
        r_value = false;
}
void MCiOSScrollerControl::SetCanCancelTouches(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        [t_view setCanCancelContentTouches: p_value];
        // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
        // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
        [t_view setDelaysContentTouches: !p_value];
    }
}
void MCiOSScrollerControl::GetCanCancelTouches(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view canCancelContentTouches];
    else
        r_value = false;
}
void MCiOSScrollerControl::SetDelayTouches(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MW-2013-11-05: [[ Bug 11242 ]] If on iOS7+ override the UIScrollView delayTouches
    //   property and use our own version.
    if (MCmajorosversion >= DELAYS_TOUCHES_WORKAROUND_MIN)
    {
        if (m_forwarder != nil)
            [m_forwarder setDelayTouches: p_value];
        return;
    }

    if (t_view)
    {
        [t_view setDelaysContentTouches: p_value];
        // SN-2014-01-14: [[ bugfix-11482 ]] DelayContentTouches must be set
        // to the opposite of CanCancelContentTouches to allow scrolling on iOS7
        [t_view setCanCancelContentTouches: !p_value];
    }
}

void MCiOSScrollerControl::GetDelayTouches(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MW-2013-11-05: [[ Bug 11242 ]] If on iOS7+ override the UIScrollView delayTouches
	//   property and use our own version.
	if (MCmajorosversion >= DELAYS_TOUCHES_WORKAROUND_MIN)
	{
		if (m_forwarder != nil)
			r_value = [m_forwarder delayTouches];
        else
            r_value = false;
        return;
    }
    
    if (t_view)
        r_value = [t_view delaysContentTouches];
    else
        r_value = false;
}
void MCiOSScrollerControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollEnabled: p_value];
}
void MCiOSScrollerControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isScrollEnabled];
    else
        r_value = false;
}

void MCiOSScrollerControl::SetPagingEnabled(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setPagingEnabled: p_value];
}

void MCiOSScrollerControl::GetPagingEnabled(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isPagingEnabled];
    else
        r_value = false;
}

void MCiOSScrollerControl::SetDecelerationRate(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_rate)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    float t_deceleration;
    switch (p_rate . type)
    {
        case kMCNativeControlDecelerationRateNormal:
            t_deceleration = UIScrollViewDecelerationRateNormal;
            break;
        case kMCNativeControlDecelerationRateFast:
                    t_deceleration = UIScrollViewDecelerationRateFast;
            break;
        case kMCNativeControlDecelerationRateCustom:
            t_deceleration = p_rate . rate;
            break;
    }
    
    if (t_view)
        [t_view setDecelerationRate: t_deceleration];
}

void MCiOSScrollerControl::GetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    r_rate . type = kMCNativeControlDecelerationRateCustom;
    
    if (t_view)
        r_rate . rate = [t_view decelerationRate];
    else
        r_rate . rate = 0;
}

void MCiOSScrollerControl::SetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle p_style)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    UIScrollViewIndicatorStyle t_style;
    
    if (t_view)
    {
        switch (p_style)
        {
            case kMCNativeControlIndicatorStyleDefault:
            case kMCNativeControlIndicatorStyleEmpty:
                t_style = UIScrollViewIndicatorStyleDefault;
                break;
            case kMCNativeControlIndicatorStyleBlack:
                t_style = UIScrollViewIndicatorStyleBlack;
                break;
            case kMCNativeControlIndicatorStyleWhite:
                t_style = UIScrollViewIndicatorStyleWhite;
                break;
        }
    
        [t_view setIndicatorStyle: t_style];
    }
}

void MCiOSScrollerControl::GetIndicatorStyle(MCExecContext& ctxt, MCNativeControlIndicatorStyle& r_style)
{
        UIScrollView *t_view;
        t_view = (UIScrollView*)GetView();
    
        if (t_view)
        {
            switch ([t_view indicatorStyle])
            {
                case UIScrollViewIndicatorStyleDefault:
                    r_style = kMCNativeControlIndicatorStyleDefault;
                    break;
                case UIScrollViewIndicatorStyleBlack:
                    r_style = kMCNativeControlIndicatorStyleBlack;
                    break;
                case UIScrollViewIndicatorStyleWhite:
                    r_style = kMCNativeControlIndicatorStyleWhite;
                    break;
            }
        }
        else
            r_style = kMCNativeControlIndicatorStyleEmpty;
}

void MCiOSScrollerControl::SetIndicatorInsets(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_insets)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    // MM-2013-11-26: [[ Bug 11485 ]] The user passes the properties of the scroller in user space, so must converted to device space before setting.
    MCGRectangle t_rect;
    t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(p_insets . left, p_insets . top, p_insets . right - p_insets . left, p_insets . bottom - p_insets . top));
    
    if (t_view)
        [t_view setScrollIndicatorInsets: UIEdgeInsetsMake(t_rect . origin . y, t_rect . origin . x, t_rect . origin . y + t_rect . size . height, t_rect . origin . x + t_rect . size . width)];
}
void MCiOSScrollerControl::GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view != nil)
    {
        UIEdgeInsets t_insets;
        t_insets = [t_view scrollIndicatorInsets];
        
        // MM-2013-11-26: [[ Bug 11485 ]] The user expects the properties of the scroller in user space, so must converted to device space before returning.
        MCGRectangle t_rect;
        t_rect = MCNativeControlUserRectFromDeviceRect(MCGRectangleMake(t_insets . left, t_insets . top, t_insets . right - t_insets . left, t_insets . bottom - t_insets . top));
        
        r_insets . left = (int16_t)(t_rect . origin . x);
        r_insets . top = (int16_t)(t_rect . origin . y);
        r_insets . right = (int16_t)(t_rect . origin . x + t_rect . size . width);
        r_insets . bottom = (int16_t)(t_rect . origin . y + t_rect . size . height);
        r_insets . has_insets = true;
    }
    else
        r_insets . has_insets = false;
}

void MCiOSScrollerControl::SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setShowsHorizontalScrollIndicator: p_value];
}
void MCiOSScrollerControl::GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view showsHorizontalScrollIndicator];
    else
        r_value = false;
}

void MCiOSScrollerControl::SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setShowsVerticalScrollIndicator: p_value];
}

void MCiOSScrollerControl::GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view showsVerticalScrollIndicator];
    else
        r_value = false;
}

void MCiOSScrollerControl::SetLockDirection(MCExecContext& ctxt, bool p_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setDirectionalLockEnabled: p_value];
}
void MCiOSScrollerControl::GetLockDirection(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDirectionalLockEnabled];
    else
        r_value = false;
}

void MCiOSScrollerControl::GetTracking(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isTracking];
    else
        r_value = false;
}

void MCiOSScrollerControl::GetDragging(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDragging];
    else
        r_value = false;
}

void MCiOSScrollerControl::GetDecelerating(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        r_value = [t_view isDecelerating];
    else
        r_value = false;
}

void MCiOSScrollerControl::ExecFlashScrollIndicators(MCExecContext& ctxt)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view == nil)
        return;
    
    [t_view flashScrollIndicators];
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSScrollerControl::HandleEvent(MCNameRef p_message)
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

void MCiOSScrollerControl::HandleEndDragEvent(bool p_decelerate)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_valueref_args(MCM_scroller_end_drag, p_decelerate ? kMCTrue : kMCFalse);
		ChangeTarget(t_old_target);
	}
}

void MCiOSScrollerControl::HandleScrollEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	
	int32_t t_x, t_y;
	m_post_scroll_event = true;
	if (t_target != nil && MCScrollViewGetContentOffset(GetView(), t_x, t_y))
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		// PM-2016-01-14: [[Bug 16705]] Pass the correct offset - relative to the contentRect
		t_target->message_with_args(MCM_scroller_did_scroll, t_x, t_y);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSScrollerControl::CreateView(void)
{
	UIScrollView *t_view;
	t_view = [[UIScrollView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;

#if __IPHONE_OS_VERSION_MAX_ALLOWED > 110000
    if (@available(iOS 11.0, *)) {
        [t_view setContentInsetAdjustmentBehavior: UIScrollViewContentInsetAdjustmentNever];
    }
#endif
    [t_view setContentInset: UIEdgeInsetsMake(0, 0, 0, 0)];

	[t_view setHidden: YES];
	
	m_delegate = [[com_runrev_livecode_MCiOSScrollViewDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	m_forwarder = [[com_runrev_livecode_MCNativeViewEventForwarder alloc] initWithFrame: CGRectMake(0,0,0,0)];
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

void MCiOSScrollerControl::DeleteView(UIView *p_view)
{
	[((UIScrollView*)p_view) setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
}

void MCiOSScrollerControl::UpdateForwarderBounds(void)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
	[m_forwarder setFrame: CGRectMake([t_view contentOffset].x, [t_view contentOffset].y, [t_view bounds].size.width, [t_view bounds].size.height)];
}

////////////////////////////////////////////////////////////////////////////////

class MCiOSScrollerEvent : public MCCustomEvent
{
public:
	MCiOSScrollerEvent(MCiOSScrollerControl *p_target, MCNameRef p_message)
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
	MCiOSScrollerControl *m_target;
	MCNameRef m_message;
};
	
class MCiOSScrollerEndDragEvent : public MCCustomEvent
{
public:
	MCiOSScrollerEndDragEvent(MCiOSScrollerControl *p_target, bool p_decelerate)
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
	MCiOSScrollerControl *m_target;
	bool m_decelerate;
};


class MCiOSScrollerScrollEvent : public MCCustomEvent
{
public:
	MCiOSScrollerScrollEvent(MCiOSScrollerControl *p_target)
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
	MCiOSScrollerControl *m_target;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCiOSScrollViewDelegate

- (id)initWithInstance:(MCiOSScrollerControl*)instance
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
	t_event = new MCiOSScrollerEvent(m_instance, MCM_scroller_begin_drag);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDragging: (UIScrollView*)scrollView willDecelerate:(BOOL)decelerate
{
	MCCustomEvent *t_event;
	t_event = new MCiOSScrollerEndDragEvent(m_instance, decelerate);
	MCEventQueuePostCustom(t_event);

	m_instance->UpdateForwarderBounds();
}

- (void)scrollViewDidScroll: (UIScrollView*)scrollView
{
	if (m_instance != nil && m_instance->m_post_scroll_event)
	{
		m_instance->m_post_scroll_event = false;
		MCCustomEvent *t_event;
		t_event = new MCiOSScrollerScrollEvent(m_instance);
		MCEventQueuePostCustom(t_event);
	}
}

- (void)scrollViewDidScrollToTop: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSScrollerEvent(m_instance, MCM_scroller_scroll_to_top);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewWillBeginDecelerating:(UIScrollView *)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSScrollerEvent(m_instance, MCM_scroller_begin_decelerate);
	MCEventQueuePostCustom(t_event);
}

- (void)scrollViewDidEndDecelerating: (UIScrollView*)scrollView
{
	MCCustomEvent *t_event;
	t_event = new MCiOSScrollerEvent(m_instance, MCM_scroller_end_decelerate);
	MCEventQueuePostCustom(t_event);

	m_instance->UpdateForwarderBounds();
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativeScrollerControlCreate(MCNativeControl *&r_control)
{
	r_control = new MCiOSScrollerControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCNativeViewEventForwarder

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

