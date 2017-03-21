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
#include "widget.h"
#include "native-layer-mac.h"

#include "platform.h"

#include "graphics_util.h"


MCNativeLayerMac::MCNativeLayerMac(MCObject *p_object, void *p_view)
{
	m_object = p_object;
    
    m_layer = MCplatform -> CreateNativeLayer();
    
    m_layer -> SetNativeView(p_view);
}

MCNativeLayerMac::~MCNativeLayerMac()
{
    if (m_layer)
        m_layer -> Release();
}

void MCNativeLayerMac::doAttach()
{
    MCPlatformWindowRef t_window = m_object -> getstack() -> getwindow();
   
    void *t_parent_view;
    t_parent_view = nil;
    /* UNCHECKED */ getParentView(t_parent_view);
    
    void * t_before_view = nil;
    /* UNCHECKED */ getViewAbove(t_before_view);
    
    m_layer -> Attach(t_window, t_parent_view, t_before_view, ShouldShowLayer());
}

void MCNativeLayerMac::doDetach()
{
    m_layer -> Detach();
}

bool MCNativeLayerMac::doPaint(MCGContextRef p_context)
{
    return m_layer -> Paint(p_context);
}

void MCNativeLayerMac::doSetViewportGeometry(const MCRectangle &p_rect)
{
}

void MCNativeLayerMac::doSetGeometry(const MCRectangle &p_rect)
{
    m_layer -> SetGeometry(p_rect);
}

void MCNativeLayerMac::doSetVisible(bool p_visible)
{
    m_layer -> SetVisible(p_visible);
}

void MCNativeLayerMac::doRelayer()
{
    void *t_parent_view;
	t_parent_view = nil;
	/* UNCHECKED */ getParentView(t_parent_view);
    
    void * t_before_view = nil;
    /* UNCHECKED */ getViewAbove(t_before_view);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getcard() == m_object->getstack()->getcurcard())
    {
        m_layer -> Relayer(t_parent_view, t_before_view);
    }
}

bool MCNativeLayerMac::getParentView(void *&r_view)
{
	if (m_object->getparent()->gettype() == CT_GROUP)
	{
		MCNativeLayer *t_container;
		t_container = nil;
		
		if (((MCGroup*)m_object->getparent())->getNativeContainerLayer(t_container))
            return t_container->GetNativeView((void*&)r_view);
	}
	
    return false;
}

bool MCNativeLayerMac::getViewAbove(void *&r_view)
{
    // Find which native layer this should be inserted below
    MCObject *t_before;
    t_before = findNextLayerAbove(m_object);
    
    if (t_before != nil)
    {
        /* UNCHECKED */ t_before->GetNativeView((void*&)r_view);
    }
    
    return false;
}

bool MCNativeLayerMac::GetNativeView(void *&r_view)
{
	return m_layer -> GetNativeView(r_view);
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_view)
{
	if (p_view == nil)
		return nil;
	
    return new MCNativeLayerMac(p_object, p_view);
}

//////////


bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
    return MCplatform -> CreateNativeContainer(r_view);
}

//////////

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
    MCplatform -> ReleaseNativeView(p_view);
}
