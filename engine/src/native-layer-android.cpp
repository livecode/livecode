/* Copyright (C) 2014 Runtime Revolution Ltd.
 
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

#include "execpt.h"
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

#include <jni.h>

#include "mblandroidutil.h"
#include "mblandroidjava.h"
#include "mblcontrol.h"
#include "graphics_util.h"

#include "native-layer-android.h"

class MCNativeLayerAndroid::AndroidView
{
public:
    
    // Constructor. The Java object being wrapped must be supplied.
    AndroidView(jobject p_java_object);
    
    ~AndroidView();
    
    void setRect(const MCRectangle& p_rect);
    void placeViewBelow(AndroidView* p_other_view);
    void removeFromMainView();
    
private:
    
    // The Java object that is the view
    jobject m_java_object;
    
    // Whether the object is currently in the view or not
    bool m_in_view;
};

////////////////////////////////////////////////////////////////////////////////

MCNativeLayerAndroid::AndroidView::AndroidView(jobject p_java_object) :
  m_java_object(MCJavaGetThreadEnv()->NewGlobalRef(p_java_object)),
  m_in_view(false)
{
    ;
}

MCNativeLayerAndroid::AndroidView::~AndroidView()
{
    MCJavaGetThreadEnv()->DeleteGlobalRef(m_java_object);
}

void MCNativeLayerAndroid::AndroidView::setRect(const MCRectangle& p_rect)
{
    int16_t i1, i2, i3, i4;
    
    // MM-2013-11-26: [[ Bug 11485 ]] The rect of the control is passed in user space. Convert to device space when setting on view.
    // AL-2014-06-16: [[ Bug 12588 ]] Actually use the passed in rect parameter
    MCGRectangle t_rect;
    t_rect = MCNativeControlUserRectToDeviceRect(MCRectangleToMCGRectangle(p_rect));
    i1 = (int16_t) roundf(t_rect . origin . x);
    i2 = (int16_t) roundf(t_rect . origin . y);
    i3 = (int16_t) roundf(t_rect . size . width);
    i4 = (int16_t) roundf(t_rect . size . height);
    
    MCAndroidEngineRemoteCall("setNativeViewRect", "voiiii", nil, m_java_object, i1, i2, i3, i4);
}

void MCNativeLayerAndroid::AndroidView::placeViewBelow(AndroidView* p_other_view)
{
    MCAndroidEngineRemoteCall("placeNativeViewBelow", "voo", nil, m_java_object, p_other_view ? p_other_view->m_java_object : NULL);
    m_in_view = true;
    MCAndroidObjectRemoteCall(m_java_object, "setVisibility", "vi", nil, 0);
}

void MCNativeLayerAndroid::AndroidView::removeFromMainView()
{
    if (m_in_view)
        MCAndroidEngineRemoteCall("removeNativeView", "vo", nil, m_java_object);
    m_in_view = false;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayerAndroid::MCNativeLayerAndroid(MCWidget* p_widget) :
  m_widget(p_widget),
  m_view(NULL)
{
    ;
}

MCNativeLayerAndroid::~MCNativeLayerAndroid()
{
    delete m_view;
}

void MCNativeLayerAndroid::OnToolChanged(Tool p_new_tool)
{
    if (m_view == NULL)
        return;
    
    if (p_new_tool == T_BROWSE || p_new_tool == T_HELP)
    {
        // In run mode. Make visible if requested
        if (m_widget->getflags() & F_VISIBLE)
            addToMainView();
        m_widget->Redraw();
    }
    else
    {
        // In edit mode
        m_view->removeFromMainView();
        m_widget->Redraw();
    }
}

void MCNativeLayerAndroid::OnOpen()
{
    // Unhide the widget, if required
    if (isAttached() && m_widget->getopened() == 1)
        doAttach();
}

void MCNativeLayerAndroid::OnClose()
{
    if (isAttached() && m_widget->getopened() == 0)
        doDetach();
}

void MCNativeLayerAndroid::OnAttach()
{
    m_attached = true;
    doAttach();
}

void MCNativeLayerAndroid::doAttach()
{
    if (m_view == nil)
        return;
    
    // Restore the visibility state of the widget (in case it changed due to a
    // tool change while on another card - we don't get a message then)
    if ((m_widget->getflags() & F_VISIBLE) && !m_widget->inEditMode())
        addToMainView();
    else
        m_view->removeFromMainView();
}

void MCNativeLayerAndroid::OnDetach()
{
    m_attached = false;
    doDetach();
}

void MCNativeLayerAndroid::doDetach()
{
    if (m_view == NULL)
        return;
    
    // Remove the view from the stack's content view
    m_view->removeFromMainView();
}

void MCNativeLayerAndroid::OnPaint(MCDC* p_dc, const MCRectangle& p_dirty)
{
    if (m_view == NULL)
        return;
    
    // If the widget is not in edit mode, we trust it to paint itself
    if (!m_widget->inEditMode())
        return;
    
    // Android does not support edit mode
    return;
}

void MCNativeLayerAndroid::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    if (m_view == NULL)
        return;
    
    MCRectangle t_rect, t_cardrect;
    t_rect = m_widget->getrect();
    m_view->setRect(t_rect);
}

void MCNativeLayerAndroid::OnVisibilityChanged(bool p_visible)
{
    if (m_view == NULL)
        return;
    
    if (p_visible)
        addToMainView();
    else
        m_view->removeFromMainView();
}

void MCNativeLayerAndroid::OnLayerChanged()
{
    doRelayer();
}

void MCNativeLayerAndroid::doRelayer()
{
    if (m_view == NULL)
        return;
    
    // Find which native layer this should be inserted below
    MCWidget* t_before;
    t_before = findNextLayerAbove(m_widget);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_widget->getstack()->getcard() == m_widget->getstack()->getcurcard())
    {
        if (t_before != NULL)
        {
            MCNativeLayerAndroid* t_android_layer;
            t_android_layer = reinterpret_cast<MCNativeLayerAndroid*>(t_before->getNativeLayer());
            m_view->placeViewBelow(t_android_layer->m_view);
        }
        else
            m_view->placeViewBelow(NULL);
        m_view->setRect(m_widget->getrect());
    }
}

void MCNativeLayerAndroid::addToMainView()
{
    doRelayer();
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCWidget::createNativeLayer()
{
    return new MCNativeLayerAndroid(this);
}
