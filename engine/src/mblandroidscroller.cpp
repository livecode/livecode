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
#include "osspec.h"
#include "exec.h"


#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidScrollerControl: public MCAndroidControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
    MCAndroidScrollerControl(void);
    
	virtual MCNativeControlType GetType(void);
    
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

    void SetContentRect(MCExecContext& ctxt, integer_t p_rect[4]);
    void GetContentRect(MCExecContext& ctxt, integer_t r_rect[4]);
  
    void SetHScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetHScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetVScroll(MCExecContext& ctxt, integer_t p_scroll);
    void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void SetShowHorizontalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowHorizontalIndicator(MCExecContext& ctxt, bool& r_value);
    void SetShowVerticalIndicator(MCExecContext& ctxt, bool p_value);
    void GetShowVerticalIndicator(MCExecContext& ctxt, bool& r_value);
    
    void GetTracking(MCExecContext& ctxt, bool& r_value);
    void GetDragging(MCExecContext& ctxt, bool& r_value);
    
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

MCPropertyInfo MCAndroidScrollerControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT_RECT, Int32X4, MCAndroidScrollerControl, ContentRect)
    DEFINE_RW_CTRL_PROPERTY(P_HSCROLL, Int32, MCAndroidScrollerControl, HScroll)
    DEFINE_RW_CTRL_PROPERTY(P_VSCROLL, Int32, MCAndroidScrollerControl, VScroll)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCAndroidScrollerControl, ScrollingEnabled)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_HORIZONTAL_INDICATOR, Bool, MCAndroidScrollerControl, ShowHorizontalIndicator)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_VERTICAL_INDICATOR, Bool, MCAndroidScrollerControl, ShowVerticalIndicator)
    DEFINE_RO_CTRL_PROPERTY(P_TRACKING, Bool, MCAndroidScrollerControl, Tracking)
    DEFINE_RO_CTRL_PROPERTY(P_DRAGGING, Bool, MCAndroidScrollerControl, Dragging)
};

MCObjectPropertyTable MCAndroidScrollerControl::kPropertyTable =
{
	&MCAndroidControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidScrollerControl::kActions[] =
{
};

MCNativeControlActionTable MCAndroidScrollerControl::kActionTable =
{
    &MCAndroidControl::kActionTable,
    0,
    nil,
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

// SN-2014-11-27: [[ Bug 14046 ]] Add declarations for the MCScrollView* functions
bool MCScrollViewGetHScroll(jobject p_view, int32_t &r_hscroll);
bool MCScrollViewGetVScroll(jobject p_view, int32_t &r_vscroll);
bool MCScrollViewSetHScroll(jobject p_view, int32_t p_hscroll);
bool MCScrollViewSetVScroll(jobject p_view, int32_t p_vscroll);

void MCAndroidScrollerControl::SetContentRect(MCExecContext& ctxt, integer_t p_rect[4])
{
    jobject t_view;
    t_view = GetView();
	
    // SN-2014-11-27: [[ Bug 14046 ]] m_content_rect stores user-pixel values
    m_content_rect . x = p_rect[0];
    m_content_rect . y = p_rect[1];
    m_content_rect . width = p_rect[2] - p_rect[0];
    m_content_rect . height = p_rect[3] - p_rect[1];
    
	// PM-2016-01-14: [[Bug 16705]] Pass the correct width and height
    if (t_view != nil)
	{
		// SN-2014-11-27: [[ Bug 14046 ]] Apply the fix for the bug 11485.
		MCGRectangle t_rect;
		t_rect = MCNativeControlUserRectToDeviceRect(MCGRectangleMake(p_rect[0], p_rect[1], p_rect[2], p_rect[3]));
        MCAndroidObjectRemoteCall(t_view, "setContentSize", "vii", nil, (integer_t)(t_rect.size.width), (integer_t)(t_rect.size.height));
	}
}

void MCAndroidScrollerControl::GetContentRect(MCExecContext& ctxt, integer_t r_rect[4])
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
    {
        // SN-2014-11-27: [[ Bug 14046 ]] m_content_rect stores user-pixel values
        r_rect[0] = m_content_rect . x;
        r_rect[1] = m_content_rect . y;
        r_rect[2] = m_content_rect . x + m_content_rect . width;
        r_rect[3] = m_content_rect . y + m_content_rect . height;
    }
}

void MCAndroidScrollerControl::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    // SN-2014-11-27: [[ Bug 14046 ]] Apply the fix for the bug 11485.
    if (t_view)
        MCScrollViewSetHScroll(t_view, p_scroll);
}

void MCAndroidScrollerControl::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    // SN-2014-11-27: [[ Bug 14046 ]] Apply the fix for the bug 11485.
    if (t_view)
        MCScrollViewGetHScroll(t_view, r_scroll);
    else
        r_scroll = 0;
}

void MCAndroidScrollerControl::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    // SN-2014-11-27: [[ Bug 14046 ]] Apply the fix for the bug 11485.
    if (t_view)
        MCScrollViewSetVScroll(t_view, p_scroll);
}

void MCAndroidScrollerControl::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
    jobject t_view;
    t_view = GetView();
    
    // SN-2014-11-27: [[ Bug 14046 ]] Apply the fix for the bug 11485.
    if (t_view)
        MCScrollViewGetVScroll(t_view, r_scroll);
    else
        r_scroll = 0;
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
        MCAndroidObjectRemoteCall(t_view, "getScrollingEnabled", "b", &r_value);
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
        MCAndroidObjectRemoteCall(t_view, "getHorizontalIndicator", "b", &r_value);
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
        MCAndroidObjectRemoteCall(t_view, "getVerticalIndicator", "b", &r_value);
    else
        r_value = false;
}

void MCAndroidScrollerControl::GetTracking(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(t_view, "isTracking", "b", &r_value);
}

void MCAndroidScrollerControl::GetDragging(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view)
        MCAndroidObjectRemoteCall(t_view, "isDragging", "b", &r_value);
}

bool MCScrollViewGetHScroll(jobject p_view, int32_t &r_hscroll)
{
    if (p_view == nil)
		return false;

    // MM-2013-11-26: [[ Bug 11485 ]] The native control stores the scroll in device pixels, but the user expects in user pixels, so convert.
    int32_t t_hscroll;
    MCAndroidObjectRemoteCall(p_view, "getHScroll", "i", &t_hscroll);
    r_hscroll = MCNativeControlUserXLocFromDeviceXLoc(t_hscroll);
	return true;
}

bool MCScrollViewGetVScroll(jobject p_view, int32_t &r_vscroll)
{
    if (p_view == nil)
		return false;

    // MM-2013-11-26: [[ Bug 11485 ]] The native control stores the scroll in device pixels, but the user expects in user pixels, so convert.
    int32_t t_vscroll;
    MCAndroidObjectRemoteCall(p_view, "getVScroll", "i", &t_vscroll);
    r_vscroll = MCNativeControlUserYLocFromDeviceYLoc(t_vscroll);
	return true;
}

bool MCScrollViewSetHScroll(jobject p_view, int32_t p_hscroll)
{
    if (p_view == nil)
        return false;
    
    // MM-2013-11-26: [[ Bug 11485 ]] The native control expects the scroll in device pixels, but the user expects in user pixels, so convert.
    int32_t t_hscroll;
    t_hscroll = MCNativeControlUserXLocToDeviceXLoc(p_hscroll);
    MCAndroidObjectRemoteCall(p_view, "setHScroll", "vi", nil, t_hscroll);
    return true;
}

bool MCScrollViewSetVScroll(jobject p_view, int32_t p_vscroll)
{
    if (p_view == nil)
        return false;
    
    // MM-2013-11-26: [[ Bug 11485 ]] The native control expects the scroll in device pixels, but the user expects in user pixels, so convert.
    int32_t t_vscroll;
    t_vscroll = MCNativeControlUserYLocToDeviceYLoc(p_vscroll);
    MCAndroidObjectRemoteCall(p_view, "setVScroll", "vi", nil, t_vscroll);
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
		// PM-2016-01-14: [[Bug 16705]] Pass the correct offset - relative to the contentRect
		t_target->message_with_args(MCM_scroller_did_scroll, t_x, t_y);
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
            t_event = new (nothrow) MCNativeScrollerScrollEvent(t_scroller);
            MCEventQueuePostCustom(t_event);
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollBeginDrag(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollBeginDrag(JNIEnv *env, jobject object)
{
    MCLog("scrollViewBeginDrag");
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_scroller_begin_drag);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollEndDrag(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_ScrollerControl_doScrollEndDrag(JNIEnv *env, jobject object)
{
    MCLog("scrollViewEndDrag");
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
    r_control = new (nothrow) MCAndroidScrollerControl();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
