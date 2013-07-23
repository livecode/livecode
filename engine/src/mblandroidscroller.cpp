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
#include "osspec.h"


#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidScrollerControl: public MCAndroidControl
{
public:
    MCAndroidScrollerControl(void);
    
	virtual MCNativeControlType GetType(void);
#ifdef LEGACY_EXEC    
    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
#endif    
    
    virtual Exec_Stat Do(MCExecContext& ctxt, MCNativeControlAction action, MCParameter *parameters);
    
    void SetContentRect(MCExecContext& ctxt, MCRectangle32* p_rect);
    void GetContentRect(MCExecContext& ctxt, MCRectangle32*& r_rect);
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
    void SetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate);
    void SetIndicatorType(MCExecContext& ctxt, intenum_t p_type);
    void GetIndicatorType(MCExecContext& ctxt, intenum_t& r_type);
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
    
    void HandleScrollEvent(void);
    void HandleEvent(MCNameRef p_message);
    
    bool CanPostScrollEvent(void) { return m_post_scroll_event; }
    void SetCanPostScrollEvent(bool p_post) { m_post_scroll_event = p_post; }
    
protected:
    virtual ~MCAndroidScrollerControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
private:
    MCRectangle32 m_content_rect;
    bool m_post_scroll_event;
};


////////////////////////////////////////////////////////////////////////////////

MCAndroidScrollerControl::MCAndroidScrollerControl(void)
{
    m_content_rect.width = m_content_rect.height = m_content_rect.x = m_content_rect.y = 0;
    m_post_scroll_event = true;
}

MCAndroidScrollerControl::~MCAndroidScrollerControl(void)
{
    
}

MCNativeControlType MCAndroidScrollerControl::GetType(void)
{
    return kMCNativeControlTypeScroller;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidScrollerControl::SetContentRect(MCExecContext& ctxt, MCRectangle32* p_rect)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(p_view, "setContentSize", "vii", nil, p_rect -> width, p_rect -> height);
}

void MCAndroidScrollerControl::GetContentRect(MCExecContext& ctxt, MCRectangle32*& r_rect)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        *r_rect = m_content_rect;
    else
        r_rect = nil;
}

void MCAndroidScrollerControl::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "setHScroll", "vi", nil, p_scroll);
}

void MCAndroidScrollerControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "getHScroll", "i", &r_scroll);
    else
        r_scroll = 0;
}

void MCAndroidScrollerControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "setVScroll", "vi", nil, p_scroll);
}

void MCAndroidScrollerControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "getVScroll", "i", &r_scroll);
    else
        r_scroll = 0;
}

void MCAndroidScrollerControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::SetCanScrollToTop(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetCanScrollToTop(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::SetCanCancelTouches(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetCanCancelTouches(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::SetDelayTouches(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetDelayTouches(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(t_view, "setScrollingEnabled", "vb", nil, p_value);
}

void MCAndroidScrollerControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "getScrollingEnabled", "b", &r_value);
}

void MCAndroidScrollerControl::SetPagingEnabled(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetPagingEnabled(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::SetDecelerationRate(MCExecContext& ctxt, const MCNativeControlDecelerationRate& p_rate)
{
    // NO-OP
}

void MCAndroidScrollerControl::SetDecelerationRate(MCExecContext& ctxt, MCNativeControlDecelerationRate& r_rate)
{

}

void MCAndroidScrollerControl::SetIndicatorType(MCExecContext& ctxt, intenum_t p_type)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetIndicatorType(MCExecContext& ctxt, intenum_t& r_type)
{
    r_type = 0;
}

void MCAndroidScrollerControl::SetIndicatorInsets(MCExecContext& ctxt, const MCNativeControlIndicatorInsets& p_insets)
{
    // NO-OP
}

void MCAndroidScrollerControl::GetIndicatorInsets(MCExecContext& ctxt, MCNativeControlIndicatorInsets& r_insets)
{
    
}

void MCAndroidScrollerControl::SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(t_view, "setHorizontalIndicator", "vb", nil, p_value);
}

void MCAndroidScrollerControl::GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "getHorizontalIndicator", "b", &r_value);
    else
        r_value = false;
}

void MCAndroidScrollerControl::SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(t_view, "setVerticalIndicator", "vb", nil, p_value);
}

void MCAndroidScrollerControl::GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "getVerticalIndicator", "b", &r_value);
    else
        r_value = false;
}

void MCAndroidScrollerControl::SetLockDirection(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}
void MCAndroidScrollerControl::GetLockDirection(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidScrollerControl::GetTracking(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "isTracking", "b", &r_value);
}

void MCAndroidScrollerControl::GetDragging(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(p_view, "isDragging", "b", &r_value);
}

void MCAndroidScrollerControl::GetDecelerating(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}


bool MCScrollViewGetHScroll(jobject p_view, int32_t &r_hscroll)
{
	if (p_view == nil)
		return false;
	
	MCAndroidObjectRemoteCall(p_view, "getHScroll", "i", &r_hscroll);
	return true;
}

bool MCScrollViewGetVScroll(jobject p_view, int32_t &r_vscroll)
{
	if (p_view == nil)
		return false;
	
	MCAndroidObjectRemoteCall(p_view, "getVScroll", "i", &r_vscroll);
	return true;
}

bool MCScrollViewSetHScroll(jobject p_view, int32_t p_hscroll)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "setHScroll", "vi", nil, p_hscroll);
    return true;
}

bool MCScrollViewSetVScroll(jobject p_view, int32_t p_vscroll)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "setVScroll", "vi", nil, p_vscroll);
    return true;
}

bool MCScrollViewGetContentOffset(jobject p_view, int32_t &r_x, int32_t &r_y)
{
    return MCScrollViewGetHScroll(p_view, r_x) && MCScrollViewGetVScroll(p_view, r_y);
}

bool MCScrollViewGetHorizontalIndicator(jobject p_view, bool &r_show_indicator)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "getHorizontalIndicator", "b", &r_show_indicator);
    return true;
}

bool MCScrollViewGetVerticalIndicator(jobject p_view, bool &r_show_indicator)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "getVerticalIndicator", "b", &r_show_indicator);
    return true;
}

bool MCScrollViewSetHorizontalIndicator(jobject p_view, bool p_show_indicator)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "setHorizontalIndicator", "vb", nil, p_show_indicator);
    return true;
}

bool MCScrollViewSetVerticalIndicator(jobject p_view, bool p_show_indicator)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "setVerticalIndicator", "vb", nil, p_show_indicator);
    return true;
}

bool MCScrollViewGetScrollingEnabled(jobject p_view, bool &r_enabled)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "getScrollingEnabled", "b", &r_enabled);
    return true;
}

bool MCScrollViewSetScrollingEnabled(jobject p_view, bool p_enabled)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "setScrollingEnabled", "vb", nil, p_enabled);
    return true;
}

bool MCScrollViewIsTracking(jobject p_view, bool &r_tracking)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "isTracking", "b", &r_tracking);
    return true;
}

bool MCScrollViewIsDragging(jobject p_view, bool &r_dragging)
{
    if (p_view == nil)
        return false;
    
    MCAndroidObjectRemoteCall(p_view, "isDragging", "b", &r_dragging);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat scroller_set_property(jobject p_view, MCRectangle32 &x_content_rect, MCNativeControlProperty p_property, MCExecPoint&ep)
{
	Boolean t_bool;
	real8 t_double;
	
	switch (p_property)
	{
		case kMCNativeControlPropertyContentRectangle:
			if (!MCNativeControl::ParseRectangle32(ep, x_content_rect))
				return ES_ERROR;
			if (p_view != nil)
                MCAndroidObjectRemoteCall(p_view, "setContentSize", "vii", nil, x_content_rect.width, x_content_rect.height);
			return ES_NORMAL;
            
        case kMCNativeControlPropertyHScroll:
            int32_t t_hscroll;
            if (!MCNativeControl::ParseInteger(ep, t_hscroll))
                return ES_ERROR;
            
            MCScrollViewSetHScroll(p_view, t_hscroll - x_content_rect.x);
            return ES_NORMAL;
            
        case kMCNativeControlPropertyVScroll:
            int32_t t_vscroll;
            if (!MCNativeControl::ParseInteger(ep, t_vscroll))
                return ES_ERROR;
            
            MCScrollViewSetVScroll(p_view, t_vscroll - x_content_rect.y);
            return ES_NORMAL;
            
        case kMCNativeControlPropertyShowHorizontalIndicator:
            if (MCU_stob(ep.getsvalue(), t_bool))
                MCScrollViewSetHorizontalIndicator(p_view, t_bool);
            else
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
            
        case kMCNativeControlPropertyShowVerticalIndicator:
            if (MCU_stob(ep.getsvalue(), t_bool))
                MCScrollViewSetVerticalIndicator(p_view, t_bool);
            else
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
            
        case kMCNativeControlPropertyScrollingEnabled:
            if (MCU_stob(ep.getsvalue(), t_bool))
                MCScrollViewSetScrollingEnabled(p_view, t_bool);
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

#ifdef /* MCAndroidScrollerControl::Set */ LEGACY_EXEC
Exec_stat MCAndroidScrollerControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    
	Exec_stat t_state;
	t_state = scroller_set_property(t_view, m_content_rect, p_property, ep);
    
	if (t_state == ES_NOT_HANDLED)
        return MCAndroidControl::Set(p_property, ep);
	else
		return t_state;

}
#endif /* MCAndroidScrollerControl::Set */

void MCAndroidScrollerControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
    switch (p_property)
	{
        case kMCNativeControlPropertyContentRectangle:
        {
            MCAutoStringRef t_string;
            int32_t t_1, t_2, t_3, t_4;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            if (!MCU_stoi4x4(MCStringGetOldString(*t_string), t_1, t_2, t_3, t_4))
                ctxt . LegacyThrow(EE_OBJECT_NAR);
            else
            {
                MCRectangle32 t_rect;
                t_rect . x = t_1;
                t_rect . y = t_2;
                t_rect . width = t_3 - t_1;
                t_rect . height = t_4 - t_2;
                SetContentRect(ctxt, t_rect);
            }
            return;
        }
        case kMCNativeControlPropertyHScroll:
		{
			int32_t t_hscroll;
			if (!ep . copyasint(t_hscroll))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetHScroll(ctxt, t_hscroll);
            return;
		}
		case kMCNativeControlPropertyVScroll:
		{
			int32_t t_vscroll;
			if (!ep . copyasint(t_vscroll))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetVScroll(ctxt, t_vscroll);
            return;
		}
            
		case kMCNativeControlPropertyScrollingEnabled:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetScrollingEnabled(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyShowHorizontalIndicator:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetShowHorizontalIndicator(ctxt, t_value);
            return;
        }
			
		case kMCNativeControlPropertyShowVerticalIndicator:
		{
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetShowVerticalIndicator(ctxt, t_value);
            return;
        }
            
        default:
            break;
    }
    
    MCAndroidControl::Set(ctxt, p_property);
}

Exec_stat scroller_get_property(jobject p_view, const MCRectangle32 &p_content_rect, MCNativeControlProperty p_property, MCExecPoint &ep)
{
	switch (p_property)
	{
		case kMCNativeControlPropertyContentRectangle:
			if (p_view != nil)
				ep.setrectangle(p_content_rect);
			else
				ep.clear();
			return ES_NORMAL;
            
        case kMCNativeControlPropertyHScroll:
        {
            int32_t t_hscroll = 0;
            if (MCScrollViewGetHScroll(p_view, t_hscroll))
                ep.setnvalue(p_content_rect.x + t_hscroll);
            else
                ep.setnvalue(0);
            return ES_NORMAL;
        }
        case kMCNativeControlPropertyVScroll:
        {
            int32_t t_vscroll = 0;
            if (MCScrollViewGetVScroll(p_view, t_vscroll))
                ep.setnvalue(p_content_rect.y + t_vscroll);
            else
                ep.setnvalue(0);
            return ES_NORMAL;
        }
        
        case kMCNativeControlPropertyShowHorizontalIndicator:
        {
            bool t_show = false;
            if (MCScrollViewGetHorizontalIndicator(p_view, t_show))
                ep.setsvalue(MCU_btos(t_show));
            else
                ep.setempty();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyShowVerticalIndicator:
        {
            bool t_show = false;
            if (MCScrollViewGetVerticalIndicator(p_view, t_show))
                ep.setsvalue(MCU_btos(t_show));
            else
                ep.setempty();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyScrollingEnabled:
        {
            bool t_show = false;
            if (MCScrollViewGetScrollingEnabled(p_view, t_show))
                ep.setsvalue(MCU_btos(t_show));
            else
                ep.setempty();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyTracking:
        {
            bool t_tracking = false;
            if (MCScrollViewIsTracking(p_view, t_tracking))
                ep.setsvalue(MCU_btos(t_tracking));
            else
                ep.setempty();
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyDragging:
        {
            bool t_dragging = false;
            if (MCScrollViewIsDragging(p_view, t_dragging))
                ep.setsvalue(MCU_btos(t_dragging));
            else
                ep.setempty();
            return ES_NORMAL;
        }
            
        default:
            break;
	}
	return ES_NOT_HANDLED;
}

#ifdef /* MCAndroidScrollerControl::Get */ LEGACY_EXEC
Exec_stat MCAndroidScrollerControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    
	Exec_stat t_state;
	t_state = scroller_get_property(t_view, m_content_rect, p_property, ep);
    
	if (t_state == ES_NOT_HANDLED)
        return MCAndroidControl::Get(p_property, ep);
	else
		return t_state;
}
#endif /* MCAndroidScrollerControl::Get */

void MCAndroidScrollerControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
	switch(p_property)
	{
        case kMCNativeControlPropertyContentRectangle:
        {
            MCRectangle32 t_rect;
            GetContentRect(ctxt, t_rect);
            ep.setstringf("%d,%d,%d,%d", t_rect . x, t_rect . y, t_rect . width - t_rect . x, t_rect . height - t_rect . y);
            return;
        }
            
        case kMCNativeControlPropertyHScroll:
        {
            int32_t t_hscroll;
            GetHScroll(ctxt, t_hscroll);
            ep . setnvalue(t_hscroll);
            return;
        }
        case kMCNativeControlPropertyVScroll:
        {
            int32_t t_vscroll;
            GetVScroll(ctxt, t_vscroll);
            ep . setnvalue(t_vscroll);
            return;
        }
            
        case kMCNativeControlPropertyScrollingEnabled:
        {
            bool t_value;
            GetScrollingEnabled(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
            
        case kMCNativeControlPropertyShowHorizontalIndicator:
        {
            bool t_value;
            GetShowHorizontalIndicator(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
            
        case kMCNativeControlPropertyShowVerticalIndicator:
        {
            bool t_value;
            GetShowVerticalIndicator(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }

        case kMCNativeControlPropertyTracking:
        {
            bool t_value;
            GetTracking(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
        case kMCNativeControlPropertyDragging:
        {
            bool t_value;
            GetDragging(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
            
        default:
            break;
    }
    
    MCAndroidControl::Get(ctxt, p_property);
}

#ifdef /* MCAndroidScrollerControl::Do */ LEGACY_EXEC
Exec_stat MCAndroidScrollerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_action)
    {
        default:
            break;
    }
    
    return MCAndroidControl::Do(p_action, p_parameters);
}
#endif /* MCAndroidScrollerControl::Do */

Exec_stat MCAndroidScrollerControl::Do(MCExecContext& ctxt, MCNativeControlAction p_action, MCParameter *p_parameters)
{    
    switch (p_action)
    {
        default:
            break;
    }
    
    return MCAndroidControl::Do(ctxt, p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

class MCNativeScrollerScrollEvent : public MCCustomEvent
{
public:
	MCNativeScrollerScrollEvent(MCAndroidScrollerControl *p_target)
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
	MCAndroidScrollerControl *m_target;
};

////////////////////////////////////////////////////////////////////////////////

void MCAndroidScrollerControl::HandleScrollEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	
	int32_t t_x, t_y;
    SetCanPostScrollEvent(true);
	if (t_target != nil && MCScrollViewGetContentOffset(GetView(), t_x, t_y))
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target->message_with_args(MCM_scroller_did_scroll, m_content_rect.x + t_x, m_content_rect.y + t_y);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollChanged(JNIEnv *env, jobject object, jint left, jint top) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollChanged(JNIEnv *env, jobject object, jint left, jint top)
{
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
    {
        MCAndroidScrollerControl *t_scroller = (MCAndroidScrollerControl*)t_control;
        if (t_scroller->CanPostScrollEvent())
        {
            t_scroller->SetCanPostScrollEvent(false);
            MCCustomEvent *t_event;
            t_event = new MCNativeScrollerScrollEvent(t_scroller);
            MCEventQueuePostCustom(t_event);
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollBeginDrag(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollBeginDrag(JNIEnv *env, jobject object)
{
    MCLog("scrollViewBeginDrag", nil);
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_scroller_begin_drag);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollEndDrag(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollEndDrag(JNIEnv *env, jobject object)
{
    MCLog("scrollViewEndDrag", nil);
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_scroller_end_drag);
}

////////////////////////////////////////////////////////////////////////////////

jobject MCAndroidScrollerControl::CreateView(void)
{
    jobject t_view;
    MCAndroidEngineRemoteCall("createScrollerControl", "o", &t_view);
    return t_view;
}

void MCAndroidScrollerControl::DeleteView(jobject p_view)
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();
    
    env->DeleteGlobalRef(p_view);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeScrollerControlCreate(MCNativeControl *&r_control)
{
    r_control = new MCAndroidScrollerControl();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
