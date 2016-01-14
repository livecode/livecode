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

struct MCNativeLayerFindWidgetObjectVisitor: public MCObjectVisitor
{
	MCWidget *current_widget;
	MCWidget *found_widget;
	
	bool find_above;
	bool reached_current;
	
	bool OnWidget(MCWidget *p_widget)
	{
		// We are only looking for widgets with a native layer
		if (p_widget->getNativeLayer() != nil)
			return OnNativeLayerWidget(p_widget);
		
		return true;
	}
	
	bool OnNativeLayerWidget(MCWidget *p_widget)
	{
		if (find_above)
		{
			if (reached_current)
			{
				found_widget = p_widget;
				return false;
			}
			else if (p_widget == current_widget)
			{
				reached_current = true;
				return true;
			}
			return true;
		}
		else
		{
			if (p_widget == current_widget)
			{
				reached_current = true;
				return false;
			}
			else
			{
				found_widget = p_widget;
				return true;
			}
		}
	}
};


MCWidget* MCNativeLayer::findNextLayerAbove(MCWidget* p_widget)
{
	MCNativeLayerFindWidgetObjectVisitor t_visitor;
	t_visitor.current_widget = p_widget;
	t_visitor.found_widget = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = true;
	
	p_widget->getcard()->visit(kMCObjectVisitorRecursive, 0, &t_visitor);
	
	return t_visitor.found_widget;
}

MCWidget* MCNativeLayer::findNextLayerBelow(MCWidget* p_widget)
{
	MCNativeLayerFindWidgetObjectVisitor t_visitor;
	t_visitor.current_widget = p_widget;
	t_visitor.found_widget = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = false;
	
	p_widget->getcard()->visit(kMCObjectVisitorRecursive, 0, &t_visitor);
	
	return t_visitor.found_widget;
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
