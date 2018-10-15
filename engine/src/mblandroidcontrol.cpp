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

#include <jni.h>

#include "mblandroidcontrol.h"
#include "mblandroidutil.h"
#include "mblandroidjava.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAndroidControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_RECTANGLE, Rectangle, MCAndroidControl, Rect)
    DEFINE_RW_CTRL_PROPERTY(P_VISIBLE, Bool, MCAndroidControl, Visible)
    DEFINE_RW_CTRL_PROPERTY(P_ALPHA, UInt16, MCAndroidControl, Alpha)
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_BACKGROUND_COLOR, NativeControlColor, MCAndroidControl, BackgroundColor)
};

MCObjectPropertyTable MCAndroidControl::kPropertyTable =
{
	&MCNativeControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidControl::kActions[] =
{
};

MCNativeControlActionTable MCAndroidControl::kActionTable =
{
    &MCNativeControl::kActionTable,
    0,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

MCAndroidControl::MCAndroidControl(void)
{
    m_view = nil;
}

bool MCAndroidControl::Create(void)
{
    m_view = CreateView();
    if (m_view == nil)
        return false;
    
    MCAndroidEngineRemoteCall("addNativeControl", "vo", nil, m_view);
    return true;
}

void MCAndroidControl::Delete(void)
{
    // MM-2012-06-12: [[ Bug 10203]] Flag controls as deleted so that they are removed from control lists (but still being retained elsewhere
    m_deleted = true;
    if (m_view != nil)
    {
        MCAndroidEngineRemoteCall("removeNativeControl", "vo", nil, m_view);
        DeleteView(m_view);
        m_view = nil;
    }
}

jobject MCAndroidControl::GetView(void)
{
    return m_view;
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidControl::GetViewRect(jobject p_view, int16_t &r_left, int16_t &r_top, int16_t &r_right, int16_t &r_bottom)
{
    MCGRectangle t_rect;
    
    int32_t t_value;
    MCAndroidObjectRemoteCall(p_view, "getLeft", "i", &t_value);
    t_rect . origin . x = (MCGFloat) t_value;
    MCAndroidObjectRemoteCall(p_view, "getTop", "i", &t_value);
    t_rect . origin . y = (MCGFloat) t_value;
    MCAndroidObjectRemoteCall(p_view, "getRight", "i", &t_value);
    t_rect . size . width  = (MCGFloat) t_value - t_rect . origin . x;
    MCAndroidObjectRemoteCall(p_view, "getBottom", "i", &t_value);
    t_rect . size . height  = (MCGFloat) t_value - t_rect . origin . y;
    
    // MM-2013-11-26: [[ Bug 11485 ]] The rect of the view is set in device space. The user expects the rect to be in user space, so convert before returning.
    t_rect = MCNativeControlUserRectFromDeviceRect(t_rect);
    
    r_left = (int16_t) roundf(t_rect . origin . x);
    r_top = (int16_t) roundf(t_rect . origin . y);
    r_right = (int16_t) roundf(t_rect . size . width) + r_left;
    r_bottom = (int16_t) roundf(t_rect . size . height) + r_top;
    
    return true;
}

bool MCAndroidControl::GetViewBackgroundColor(jobject p_view, uint16_t &r_red, uint16_t &r_green, uint16_t &r_blue, uint16_t &r_alpha)
{
    // in 32bit argb format
    int32_t t_color;
    MCAndroidObjectRemoteCall(p_view, "getBackgroundColor", "i", &t_color);
    MCJavaColorToComponents(t_color, r_red, r_green, r_blue, r_alpha);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidControl::SetRect(MCExecContext& ctxt, MCRectangle p_rect)
{
    int16_t i1, i2, i3, i4;

    // MM-2013-11-26: [[ Bug 11485 ]] The rect of the control is passed in user space. Convert to device space when setting on view.
    // AL-2014-06-16: [[ Bug 12588 ]] Actually use the passed in rect parameter
    MCGRectangle t_rect;
    t_rect = MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_rect));
    i1 = (int16_t) roundf(t_rect . origin . x);
    i2 = (int16_t) roundf(t_rect . origin . y);
    i3 = (int16_t) roundf(t_rect . size . width) + i1;
    i4 = (int16_t) roundf(t_rect . size . height) + i2;
    
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "setRect", "viiii", nil, i1, i2, i3, i4);
}

void MCAndroidControl::SetVisible(MCExecContext& ctxt, bool p_visible)
{
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "setVisible", "vb", nil, p_visible);
}

void MCAndroidControl::SetAlpha(MCExecContext& ctxt, uinteger_t p_alpha)
{
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "setAlpha", "vi", nil, p_alpha);
}

void MCAndroidControl::SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "setBackgroundColor", "viiii", nil, p_color . r >> 8, p_color . g >> 8, p_color . b >> 8, p_color . a >> 8);
}

void MCAndroidControl::GetRect(MCExecContext& ctxt, MCRectangle& r_rect)
{
    if (m_view != nil)
    {
        int16_t i1, i2, i3, i4;
        GetViewRect(m_view, i1, i2, i3, i4);
        
        r_rect . x = i1;
        r_rect . y = i2;
        r_rect . width = i3 - i1;
        r_rect . height = i4 - i2;
    }
}

void MCAndroidControl::GetVisible(MCExecContext& ctxt, bool& r_visible)
{
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "getVisible", "b", &r_visible);
    else
        r_visible = false;
}

void MCAndroidControl::GetAlpha(MCExecContext& ctxt, uinteger_t& r_alpha)
{
    if (m_view != nil)
        MCAndroidObjectRemoteCall(m_view, "getAlpha", "i", &r_alpha);
    else
        r_alpha = 0;
}

void MCAndroidControl::GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& p_color)
{
    if (m_view != nil)
        GetViewBackgroundColor(m_view, p_color . r, p_color . g, p_color . b, p_color . a);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    JNIEnv *env;
    jobject target;
    MCAndroidControl *match;
} MCAndroidSearchContext;

bool filter_on_control_view(void *p_context, MCNativeControl *p_control)
{
    MCAndroidSearchContext *t_context = (MCAndroidSearchContext*)p_context;
    MCAndroidControl *t_control = (MCAndroidControl*)p_control;
    
    if (t_context->env->IsSameObject(t_context->target, t_control->GetView()))
    {
        t_context->match = t_control;
        return false;
    }
    
    return true;
}

bool MCAndroidControl::FindByView(jobject p_view, MCAndroidControl *&r_control)
{
    MCAndroidSearchContext t_context;
    t_context . target = p_view;
    t_context . match = nil;
    t_context . env = MCJavaGetThreadEnv();
    
    bool t_found;
    t_found = !List(filter_on_control_view, &t_context);
    
    if (t_found)
        r_control = t_context . match;
    
    return t_found;
}

////////////////////////////////////////////////////////////////////////////////

class MCAndroidControlNotifyEvent: public MCCustomEvent
{
public:
	MCAndroidControlNotifyEvent(MCAndroidControl *p_target, MCNameRef p_notification)
	{
		m_target = p_target;
		m_target -> Retain();
		m_notification = p_notification;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleNotifyEvent(m_notification);
	}
	
private:
	MCAndroidControl *m_target;
	MCNameRef m_notification;
};

void MCAndroidControl::PostNotifyEvent(MCNameRef p_message)
{
    MCCustomEvent *t_event;
    t_event = new (nothrow) MCAndroidControlNotifyEvent(this, p_message);
    MCEventQueuePostCustom(t_event);
}

void MCAndroidControl::HandleNotifyEvent(MCNameRef p_message)
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

////////////////////////////////////////////////////////////////////////////////

// MM-2013-11-26: [[ Bug 11485 ]] When positioning native controls on Android,
//   we must take into account the pixel density of the device and any scaling by way of the fullscreen mode.

MCGAffineTransform MCNativeControlUserToDeviceTransform()
{
	// IM-2014-02-25: [[ Bug 11816 ]] Use scaled stack->view transform as view backing scale may not have been set yet
    float t_scale;
    t_scale = MCResGetPixelScale();
    return MCGAffineTransformPreScale(MCdefaultstackptr -> getviewtransform(), t_scale, t_scale);
}

MCGAffineTransform MCNativeControlUserFromDeviceTransform()
{
    return MCGAffineTransformInvert(MCNativeControlUserToDeviceTransform());
}

////////////////////////////////////////////////////////////////////////////////
