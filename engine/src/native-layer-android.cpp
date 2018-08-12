/* Copyright (C) 2015 LiveCode Ltd.
 
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


#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"
#include "objptr.h"

#include "globals.h"
#include "context.h"

#include "group.h"

#include <jni.h>

#include "mblandroidutil.h"
#include "mblandroidjava.h"
#include "mblcontrol.h"
#include "graphics_util.h"

#include "native-layer-android.h"

////////////////////////////////////////////////////////////////////////////////

void MCAndroidViewSetRect(jobject p_view, const MCGRectangle &p_rect)
{
	int16_t x, y, w, h;
	x = (int16_t) roundf(p_rect . origin . x);
	y = (int16_t) roundf(p_rect . origin . y);
	w = (int16_t) roundf(p_rect . size . width);
	h = (int16_t) roundf(p_rect . size . height);

	MCAndroidEngineRemoteCall("setNativeViewRect", "voiiii", nil, p_view, x, y, w, h);
}

void MCAndroidViewSetRect(jobject p_view, const MCRectangle& p_rect)
{
    // MM-2013-11-26: [[ Bug 11485 ]] The rect of the control is passed in user space. Convert to device space when setting on view.
	MCAndroidViewSetRect(p_view, MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_rect)));
}

void MCAndroidViewSetRect(jobject p_view, const MCRectangle &p_rect, const MCRectangle &p_parent_rect)
{
	MCGRectangle t_rect = MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_rect));
	MCGRectangle t_parent_rect = MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_parent_rect));
	
	t_rect.origin.x -= t_parent_rect.origin.x;
	t_rect.origin.y -= t_parent_rect.origin.y;

	MCAndroidViewSetRect(p_view, t_rect);
}

void MCAndroidViewAddToContainer(jobject p_view, jobject p_view_above, jobject p_container)
{
    MCAndroidEngineRemoteCall("addNativeViewToContainer", "vooo", nil, p_view, p_view_above, p_container);
    MCAndroidObjectRemoteCall(p_view, "setVisibility", "vi", nil, 0);
}

void MCAndroidViewRemoveFromContainer(jobject p_view)
{
	MCAndroidEngineRemoteCall("removeNativeViewFromContainer", "vo", nil, p_view);
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayerAndroid::MCNativeLayerAndroid(MCObject *p_object, jobject p_view) :
  m_view(NULL)
{
	m_object = p_object;
	m_view = MCJavaGetThreadEnv()->NewGlobalRef(p_view);
}

MCNativeLayerAndroid::~MCNativeLayerAndroid()
{
	if (m_view != nil)
		MCJavaGetThreadEnv()->DeleteGlobalRef(m_view);
}

void MCNativeLayerAndroid::doAttach()
{
    if (m_view == nil)
        return;
	
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayerAndroid::doDetach()
{
    if (m_view == NULL)
        return;
    
    // Remove the view from the stack's content view
    MCAndroidViewRemoveFromContainer(m_view);
}

// Rendering view to context not supported on Android.
bool MCNativeLayerAndroid::GetCanRenderToContext()
{
	return false;
}

bool MCNativeLayerAndroid::doPaint(MCGContextRef p_context)
{
	return false;
}

void MCNativeLayerAndroid::doSetViewportGeometry(const MCRectangle& p_rect)
{
}

void MCNativeLayerAndroid::doSetGeometry(const MCRectangle &p_rect)
{
    if (m_view == NULL)
        return;
	
	if (m_object->getparent()->gettype() == CT_GROUP)
	{
		// Set rectangle relative to parent object
		MCAndroidViewSetRect(m_view, p_rect, m_object->getparent()->getrect());
	}
	else
		MCAndroidViewSetRect(m_view, p_rect);
}

void MCNativeLayerAndroid::doSetVisible(bool p_visible)
{
    if (m_view == NULL)
        return;
    
    if (p_visible)
	{
        addToMainView();
		doSetGeometry(m_object->getrect());
	}
    else
		MCAndroidViewRemoveFromContainer(m_view);
}

void MCNativeLayerAndroid::doRelayer()
{
    // Find which native layer this should be inserted below
    MCObject *t_before;
    t_before = findNextLayerAbove(m_object);
	
	jobject t_parent_view;
	t_parent_view = nil;
	
	if (!getParentView(t_parent_view))
		return;
	
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getstack()->getcard() == m_object->getstack()->getcurcard())
    {
		MCAndroidViewRemoveFromContainer(m_view);
		
		jobject t_before_view;
		t_before_view = nil;
		
        if (t_before != NULL)
			/* UNCHECKED */t_before->GetNativeView((void*&)t_before_view);
		
		MCAndroidViewAddToContainer(m_view, t_before_view, t_parent_view);
		doSetGeometry(m_object->getrect());
    }
}

bool MCNativeLayerAndroid::getParentView(jobject &r_view)
{
	if (m_object->getparent()->gettype() == CT_GROUP)
	{
		MCNativeLayer *t_container;
		t_container = nil;
		
		if (!((MCGroup*)m_object->getparent())->getNativeContainerLayer(t_container))
			return false;
		
		return t_container->GetNativeView((void*&)r_view);
	}
	else
	{
		jobject t_view;
		t_view = nil;
		MCAndroidEngineRemoteCall("getNativeLayerContainer", "o", &t_view);
		if (t_view == nil)
			return false;
		
		r_view = t_view;
		return true;
	}
}

void MCNativeLayerAndroid::addToMainView()
{
    doRelayer();
}

bool MCNativeLayerAndroid::GetNativeView(void *&r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_view)
{
    return new MCNativeLayerAndroid(p_object, (jobject)p_view);
}

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
	jobject t_view;
	t_view = nil;
	
	MCAndroidEngineRemoteCall("createNativeLayerContainer", "o", &t_view);
	
	if (t_view == nil)
		return false;
	
	r_view = MCJavaGetThreadEnv()->NewGlobalRef(t_view);
	return true;
}

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
	if (p_view != nil)
		MCJavaGetThreadEnv()->DeleteGlobalRef((jobject)p_view);
}

////////////////////////////////////////////////////////////////////////////////
