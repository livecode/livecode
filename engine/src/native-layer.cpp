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

#include "native-layer.h"

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer::MCNativeLayer() :
	m_widget(nil), m_attached(false), m_can_render_to_context(true),
	m_defer_geometry_changes(false)
{
    ;
}

MCNativeLayer::~MCNativeLayer()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeLayer::OnOpen()
{
	// Unhide the widget, if required
	if (isAttached())
		doAttach();
}

void MCNativeLayer::OnClose()
{
	if (isAttached())
		doDetach();
}

void MCNativeLayer::OnAttach()
{
	m_attached = true;
	doAttach();
}

void MCNativeLayer::OnDetach()
{
	m_attached = false;
	doDetach();
}

bool MCNativeLayer::OnPaint(MCGContextRef p_context)
{
	return doPaint(p_context);
}

void MCNativeLayer::OnGeometryChanged(const MCRectangle &p_old_rect)
{
	if (!m_defer_geometry_changes)
		doSetGeometry(MCWidgetGetHost(m_widget)->getrect());
}

void MCNativeLayer::OnToolChanged(Tool p_new_tool)
{
	MCWidget* t_widget = MCWidgetGetHost(m_widget);
	
	OnVisibilityChanged(ShouldShowWidget(t_widget));
	t_widget->Redraw();
}

void MCNativeLayer::OnVisibilityChanged(bool p_visible)
{
	if (p_visible)
	{
		if (m_defer_geometry_changes)
			doSetGeometry(MCWidgetGetHost(m_widget)->getrect());
		m_defer_geometry_changes = false;
	}
	else
	{
		if (!GetCanRenderToContext())
			m_defer_geometry_changes = true;
	}
	
	doSetVisible(p_visible);
}

void MCNativeLayer::OnLayerChanged()
{
	doRelayer();
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeLayer::isAttached() const
{
    return m_attached;
}

bool MCNativeLayer::ShouldShowWidget(MCWidget *p_widget)
{
	return p_widget->getflag(F_VISIBLE) && p_widget->isInRunMode();
}

MCWidget* MCNativeLayer::findNextLayerAbove(MCWidget* p_widget)
{
	MCObjptr *t_first_object;
	t_first_object = p_widget->getcard()->getobjptrs();
	
	MCObjptr *t_object;
	t_object = p_widget->getcard()->getobjptrforcontrol(p_widget)->next();
	
	while (t_object != t_first_object)
	{
		MCControl *t_control;
		t_control = t_object->getref();
		// We are only looking for widgets with a native layer
		if (t_control->gettype() == CT_WIDGET && reinterpret_cast<MCWidget*>(t_control)->getNativeLayer() != nil)
		{
			// Found what we are looking for
			return reinterpret_cast<MCWidget*>(t_control);
		}
		
		// Next control
		t_object = t_object->next();
	}
	
	return nil;
}

MCWidget* MCNativeLayer::findNextLayerBelow(MCWidget* p_widget)
{
	MCObjptr *t_last_object;
	t_last_object = p_widget->getcard()->getobjptrs()->prev();
	
	MCObjptr *t_object;
	t_object = p_widget->getcard()->getobjptrforcontrol(p_widget)->prev();
	
	while (t_object != t_last_object)
	{
		MCControl *t_control;
		t_control = t_object->getref();
		// We are only looking for widgets with a native layer
		if (t_control->gettype() == CT_WIDGET && reinterpret_cast<MCWidget*>(t_control)->getNativeLayer() != nil)
		{
			// Found what we are looking for
			return reinterpret_cast<MCWidget*>(t_control);
		}
		
		// Previous control
		t_object = t_object->prev();
	}
	
	return nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeLayer::SetCanRenderToContext(bool p_can_render)
{
	m_can_render_to_context = p_can_render;
}

bool MCNativeLayer::GetCanRenderToContext()
{
	return m_can_render_to_context;
}

////////////////////////////////////////////////////////////////////////////////
