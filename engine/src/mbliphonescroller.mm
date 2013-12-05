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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
//#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "eventqueue.h"
#include "util.h"

#include "exec.h"

#import <UIKit/UIKit.h>

#include "mbliphonecontrol.h"
#include "mblcontrol.h"

////////////////////////////////////////////////////////////////////////////////
	
extern UIView *MCIPhoneGetView(void);

class MCiOSScrollerControl;

@interface MCiOSScrollViewDelegate : NSObject <UIScrollViewDelegate>
{
	MCiOSScrollerControl *m_instance;
}

- (id)initWithInstance:(MCiOSScrollerControl *)instance;
@end

@interface MCNativeViewEventForwarder : UIView
{
	UIView *m_target;
}
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
#ifdef LEGACY_EXEC	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
#endif

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
	MCiOSScrollViewDelegate *m_delegate;
	MCNativeViewEventForwarder *m_forwarder;
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
    DEFINE_CTRL_EXEC_METHOD(FlashScrollIndicators, MCiOSScrollerControl, FlashScrollIndicators)
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

bool MCScrollViewGetContentOffset(UIScrollView *p_view, int32_t &r_x, int32_t &r_y)
{
	if (p_view == nil)
		return false;
	
	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();
	
	CGPoint t_offset;
	t_offset = [p_view contentOffset];
	r_x = t_offset.x * t_scale;
	r_y = t_offset.y * t_scale;
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
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view != nil)
        [t_view setContentSize:CGSizeMake((float)(p_rect[2] - p_rect[0]) / t_scale, (float)(p_rect[3] - p_rect[1]) / t_scale)];
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
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
        [t_view setContentOffset: CGPointMake((float)(p_scroll - m_content_rect.x) / t_scale, (float)t_y / t_scale)];
    
    UpdateForwarderBounds();
}

void MCiOSScrollerControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        float t_scale;
        t_scale = MCIPhoneGetNativeControlScale();
        
        r_scroll = m_content_rect.x + [t_view contentOffset].x * t_scale;
    }
    else
        r_scroll = 0;
}
void MCiOSScrollerControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    int32_t t_x, t_y;
    if (t_view != nil && MCScrollViewGetContentOffset(t_view, t_x, t_y))
        [t_view setContentOffset: CGPointMake((float)t_x / t_scale, (float)(p_scroll - m_content_rect.y) / t_scale)];
}

void MCiOSScrollerControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
    {
        float t_scale;
        t_scale = MCIPhoneGetNativeControlScale();
        
        r_scroll = m_content_rect.y + [t_view contentOffset].y * t_scale;
    }
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
        [t_view setCanCancelContentTouches: p_value];
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
    
    if (t_view)
        [t_view setDelaysContentTouches: p_value];
}

void MCiOSScrollerControl::GetDelayTouches(MCExecContext& ctxt, bool& r_value)
{
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
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
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    if (t_view)
        [t_view setScrollIndicatorInsets: UIEdgeInsetsMake((float)p_insets . top / t_scale, (float)p_insets . left / t_scale, (float)p_insets . bottom / t_scale, (float)p_insets . right / t_scale)];
}
void MCiOSScrollerControl::GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets)
{
    float t_scale;
    t_scale = MCIPhoneGetNativeControlScale();
    
    UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();
    
    UIEdgeInsets t_insets;
    t_insets = [t_view scrollIndicatorInsets];
    
    r_insets . left = (int16_t)(t_insets.left * t_scale);
    r_insets . top = (int16_t)(t_insets.top * t_scale);
    r_insets . right = (int16_t)(t_insets.right * t_scale);
    r_insets . bottom = (int16_t)(t_insets.bottom * t_scale);
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

#ifdef LEGACY_EXEC
Exec_stat scroller_set_property(UIScrollView *p_view, MCRectangle32 &x_content_rect, MCNativeControlProperty p_property, MCExecPoint&ep)
{
	Boolean t_bool;
	real8 t_double;
	
	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();

	switch (p_property)
	{
		case kMCNativeControlPropertyContentRectangle:
			if (!MCiOSControl::ParseRectangle32(ep, x_content_rect))
				return ES_ERROR;
			if (p_view != nil)
				[p_view setContentSize:CGSizeMake((float)x_content_rect.width / t_scale, (float)x_content_rect.height / t_scale)];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyHScroll:
		{
			int32_t t_hscroll;
			if (!MCiOSControl::ParseInteger(ep, t_hscroll))
				return ES_ERROR;
			
			int32_t t_x, t_y;
			if (p_view != nil && MCScrollViewGetContentOffset(p_view, t_x, t_y))
				[p_view setContentOffset: CGPointMake((float)(t_hscroll - x_content_rect.x) / t_scale, (float)t_y / t_scale)];
		}
		return ES_NORMAL;
			
		case kMCNativeControlPropertyVScroll:
		{
			int32_t t_vscroll;
			if (!MCiOSControl::ParseInteger(ep, t_vscroll))
				return ES_ERROR;
			
			int32_t t_x, t_y;
			if (p_view != nil && MCScrollViewGetContentOffset(p_view, t_x, t_y))
				[p_view setContentOffset: CGPointMake((float)t_x / t_scale, (float)(t_vscroll - x_content_rect.y) / t_scale)];
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
					[p_view setCanCancelContentTouches: t_bool];
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
					[p_view setDelaysContentTouches: t_bool];
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
				if (p_view)
					[p_view setScrollIndicatorInsets: UIEdgeInsetsMake((float)t_top / t_scale, (float)t_left / t_scale, (float)t_bottom / t_scale, (float)t_right / t_scale)];				
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
        default:
            break;
	}
	return ES_NOT_HANDLED;
}
#endif

#ifdef /* MCNativeScrollerControl::Set */ LEGACY_EXEC
Exec_stat MCiOSScrollerControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
	UIScrollView *t_view;
	t_view = (UIScrollView*)GetView();

	float t_scale;
	t_scale = MCIPhoneGetNativeControlScale();

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
        default:
            break;
	}
	
	Exec_stat t_state;
	t_state = scroller_set_property(t_view, m_content_rect, p_property, ep);

	if (t_state == ES_NOT_HANDLED)
		return MCiOSControl::Set(p_property, ep);
	else
		return t_state;
}
#endif /* MCNativeScrollerControl::Set */

#ifdef LEGACY_EXEC
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
			ep.setnvalue(p_view != nil ? p_content_rect.x + [p_view contentOffset].x * t_scale : 0);
			return ES_NORMAL;
		case kMCNativeControlPropertyVScroll:
			ep.setnvalue(p_view != nil ? p_content_rect.y + [p_view contentOffset].y * t_scale : 0);
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
                ep.setstringf("%d,%d,%d,%d", (int16_t)(t_insets.left * t_scale), (int16_t)(t_insets.top * t_scale), (int16_t)(t_insets.right * t_scale), (int16_t)(t_insets.bottom * t_scale));
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
#endif

#ifdef /* MCiOSScrollerControl::Get */ LEGACY_EXEC
Exec_stat MCiOSScrollerControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
	UIScrollView *t_view;
	t_view = (UIScrollView *)GetView();

	Exec_stat t_status;
	t_status = scroller_get_property(t_view, m_content_rect, p_property, ep);
	if (t_status == ES_NOT_HANDLED)
		return MCiOSControl::Get(p_property, ep);
	else
		return t_status;
}
#endif /* MCiOSScrollerControl::Get */

#ifdef /* MCiOSScrollerControl::Do */ LEGACY_EXEC
Exec_stat MCiOSScrollerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
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
#endif /* MCiOSScrollerControl::Do */

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
		t_target->message_with_args(MCM_scroller_did_scroll, m_content_rect.x + t_x, m_content_rect.y + t_y);
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
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCiOSScrollViewDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	m_forwarder = [[MCNativeViewEventForwarder alloc] initWithFrame: CGRectMake(0,0,0,0)];
	[t_view addSubview: m_forwarder];
	
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

@implementation MCiOSScrollViewDelegate

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
	}
	return self;
}

- (void) touchesBegan: (NSSet*)touches withEvent: (UIEvent*)event
{
	if (m_target != nil)
		[m_target touchesBegan: touches withEvent: event];
}

- (void)touchesCancelled: (NSSet*) touches withEvent: (UIEvent*)event
{
	if (m_target != nil)
		[m_target touchesCancelled: touches withEvent: event];
}

- (void)touchesEnded: (NSSet*) touches withEvent: (UIEvent*)event
{
	if (m_target != nil)
		[m_target touchesEnded: touches withEvent: event];
}

- (void)touchesMoved: (NSSet*) touches withEvent: (UIEvent*)event
{
	if (m_target != nil)
		[m_target touchesMoved: touches withEvent: event];
}
@end

