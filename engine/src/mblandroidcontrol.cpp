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

#include <jni.h>

#include "mblandroidcontrol.h"
#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "resolution.h"

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
    int32_t t_value;
    MCAndroidObjectRemoteCall(p_view, "getLeft", "i", &t_value);
    r_left = t_value;
    MCAndroidObjectRemoteCall(p_view, "getTop", "i", &t_value);
    r_top = t_value;
    MCAndroidObjectRemoteCall(p_view, "getRight", "i", &t_value);
    r_right = t_value;
    MCAndroidObjectRemoteCall(p_view, "getBottom", "i", &t_value);
    r_bottom = t_value;
    
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

Exec_stat MCAndroidControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    switch (p_property)
    {
        case kMCNativeControlPropertyRectangle:
        {
            int16_t i1, i2, i3, i4;
            if (MCU_stoi2x4(ep.getsvalue(), i1, i2, i3, i4))
            {
                // MM-2013-09-30: [[ Bug 11227 ]] Make sure we take into account device scale when positioning native controls.
                //   We take into account the scale at this point as it's most convenient. This way, we assume that the on the Java
                //   side everything is in pixels. A better abtraction may be needed going forward (if there is to be more drawing on the java side).
                MCGFloat t_device_scale;
                t_device_scale = MCResGetDeviceScale();
                i1 = (int16_t) i1 * t_device_scale;
                i2 = (int16_t) i2 * t_device_scale;
                i3 = (int16_t) i3 * t_device_scale;
                i4 = (int16_t) i4 * t_device_scale;

                if (m_view != nil)
                    MCAndroidObjectRemoteCall(m_view, "setRect", "viiii", nil, i1, i2, i3, i4);
            }
            else
            {
                MCeerror->add(EE_OBJECT_NAR, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
        }
        
        case kMCNativeControlPropertyVisible:
        {
            Boolean t_value;
            if (MCU_stob(ep.getsvalue(), t_value))
            {
                if (m_view != nil)
                    MCAndroidObjectRemoteCall(m_view, "setVisible", "vb", nil, t_value);
            }
            else
            {
                MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAlpha:
        {
            uint16_t t_alpha;
            if (MCU_stoui2(ep.getsvalue(), t_alpha))
            {
                if (m_view != nil)
                    MCAndroidObjectRemoteCall(m_view, "setAlpha", "vi", nil, t_alpha);
            }
            else
            {
                MCeerror->add(EE_OBJECT_NAN, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyBackgroundColor:
        {
            uint16_t t_r, t_g, t_b, t_a;
            if (MCNativeControl::ParseColor(ep, t_r, t_g, t_b, t_a))
            {
                if (m_view != nil)
                    MCAndroidObjectRemoteCall(m_view, "setBackgroundColor", "viiii", nil, t_r >> 8, t_g >> 8, t_b >> 8, t_a >> 8);
            }
            else
            {
                MCeerror->add(EE_OBJECT_BADCOLOR, 0, 0, ep.getsvalue());
                return ES_ERROR;
            }
            return ES_NORMAL;
        }
        default:
            break;
    }
    
    return ES_NOT_HANDLED;
}

Exec_stat MCAndroidControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    switch (p_property)
    {
        case kMCNativeControlPropertyId:
            ep.setnvalue(GetId());
            return ES_NORMAL;
            
        case kMCNativeControlPropertyName:
            ep.copysvalue(GetName() == nil ? "" : GetName());
            return ES_NORMAL;
            
        case kMCNativeControlPropertyRectangle:
        {
            if (m_view != nil)
            {
                int16_t i1, i2, i3, i4;
                GetViewRect(m_view, i1, i2, i3, i4);
                
                // MM-2013-09-30: [[ Bug 11227 ]] Make sure we take into account device scale when positioning native controls.
                MCGFloat t_device_scale;
                t_device_scale = MCResGetDeviceScale();
                i1 = (int16_t) i1 / t_device_scale;
                i2 = (int16_t) i2 / t_device_scale;
                i3 = (int16_t) i3 / t_device_scale;
                i4 = (int16_t) i4 / t_device_scale;
                
                MCExecPointSetRect(ep, i1, i2, i3, i4);
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyVisible:
        {
            if (m_view != nil)
            {
                bool t_visible;
                MCAndroidObjectRemoteCall(m_view, "getVisible", "b", &t_visible);
                ep.setsvalue(MCU_btos(t_visible));
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyAlpha:
        {
            if (m_view != nil)
            {
                int32_t t_alpha;
                MCAndroidObjectRemoteCall(m_view, "getAlpha", "i", &t_alpha);
                ep.setnvalue(t_alpha);
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyBackgroundColor:
        {
            uint16_t t_red, t_green, t_blue, t_alpha;
            if (m_view != nil)
            {
                GetViewBackgroundColor(m_view, t_red, t_green, t_blue, t_alpha);
                FormatColor(ep, t_red, t_blue, t_green, t_alpha);
            }
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return ES_ERROR;
}

Exec_stat MCAndroidControl::Do(MCNativeControlAction p_action, MCParameter *p_params)
{
    return ES_ERROR;
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
    t_event = new MCAndroidControlNotifyEvent(this, p_message);
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
