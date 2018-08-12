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
#include "graphics_util.h"
#include "objptr.h"

#include "globals.h"
#include "context.h"

#include "native-layer.h"

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer::MCNativeLayer() :
	m_object(nil),
	m_attached(false),
	m_visible(false),
	m_show_for_tool(false),
	m_can_render_to_context(true),
	m_defer_geometry_changes(true)
{
	m_rect = m_viewport_rect = m_deferred_rect = m_deferred_viewport_rect =
		MCRectangleMake(0, 0, 0, 0);
}

MCNativeLayer::~MCNativeLayer()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

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
    /* We must make sure that any geometry changes are in sync if we are
     * rendering to context, but are not visible */
    if (m_can_render_to_context && m_defer_geometry_changes)
    {
        doSetViewportGeometry(m_deferred_viewport_rect);
        doSetGeometry(m_deferred_rect);
        m_viewport_rect = m_deferred_viewport_rect;
        m_rect = m_deferred_rect;
		m_defer_geometry_changes = false;
    }
	return doPaint(p_context);
}

void MCNativeLayer::OnViewTransformChanged()
{
	doSetGeometry(m_rect);
}

void MCNativeLayer::OnGeometryChanged(const MCRectangle &p_new_rect)
{
	if (!m_defer_geometry_changes)
	{
		doSetGeometry(p_new_rect);
		m_rect = p_new_rect;
	}
	else
		m_deferred_rect = p_new_rect;
}

void MCNativeLayer::OnViewportGeometryChanged(const MCRectangle &p_rect)
{
	if (!m_defer_geometry_changes)
	{
		doSetViewportGeometry(p_rect);
		m_viewport_rect = p_rect;
	}
	else
		m_deferred_viewport_rect = p_rect;
}

void MCNativeLayer::OnToolChanged(Tool p_new_tool)
{
	bool t_showing;
	t_showing = ShouldShowLayer();
	
	// IM-2016-02-23: [[ Bug 16980 ]] Determine visiblilty based on what the stack considers to be the tool for this object.
	Tool t_tool;
	t_tool = m_object->getstack()->gettool(m_object);

	m_show_for_tool = t_tool == T_BROWSE || t_tool == T_HELP;

	if (t_showing != (ShouldShowLayer()))
		UpdateVisibility();

	m_object->Redraw();
}

void MCNativeLayer::OnVisibilityChanged(bool p_visible)
{
	bool t_showing;
	t_showing = ShouldShowLayer();
	
	m_visible = p_visible;
	
	if (t_showing != (ShouldShowLayer()))
		UpdateVisibility();
}

void MCNativeLayer::UpdateVisibility()
{
	// IM-2016-03-16: [[ Bug 17138 ]] Always defer updates if the layer is not shown, and can't be drawn to the context.
	if (ShouldShowLayer() || m_can_render_to_context)
	{
		if (m_defer_geometry_changes)
		{
			doSetViewportGeometry(m_deferred_viewport_rect);
			doSetGeometry(m_deferred_rect);
			m_viewport_rect = m_deferred_viewport_rect;
			m_rect = m_deferred_rect;
		}
		m_defer_geometry_changes = false;
	}
	else
	{
		if (!m_defer_geometry_changes)
		{
			m_defer_geometry_changes = true;
			m_deferred_rect = m_rect;
			m_deferred_viewport_rect = m_viewport_rect;
		}
	}
	
	doSetVisible(ShouldShowLayer());
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

bool MCNativeLayer::ShouldShowLayer()
{
	return m_show_for_tool && m_visible;
}

struct MCNativeLayerFindObjectVisitor: public MCObjectVisitor
{
	MCObject *current_object;
	MCObject *found_object;
	
	bool find_above;
	bool reached_current;
	
	bool OnObject(MCObject *p_object)
	{
		// We are only looking for objects with a native layer
		if (p_object->getNativeLayer() != nil)
			return OnNativeLayerObject(p_object);
		
		return true;
	}
	
	bool OnNativeLayerObject(MCObject *p_object)
	{
		if (find_above)
		{
			if (reached_current)
			{
				found_object = p_object;
				return false;
			}
			else if (p_object == current_object)
			{
				reached_current = true;
				return true;
			}
			return true;
		}
		else
		{
			if (p_object == current_object)
			{
				reached_current = true;
				return false;
			}
			else
			{
				found_object = p_object;
				return true;
			}
		}
	}
};


MCObject* MCNativeLayer::findNextLayerAbove(MCObject* p_object)
{
	MCNativeLayerFindObjectVisitor t_visitor;
	t_visitor.current_object = p_object;
	t_visitor.found_object = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = true;
	
	p_object->getparent()->visit_children(0, 0, &t_visitor);
	
	return t_visitor.found_object;
}

MCObject* MCNativeLayer::findNextLayerBelow(MCObject* p_object)
{
	MCNativeLayerFindObjectVisitor t_visitor;
	t_visitor.current_object = p_object;
	t_visitor.found_object = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = false;
	
	p_object->getparent()->visit_children(0, 0, &t_visitor);
	
	return t_visitor.found_object;
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeLayer::SetCanRenderToContext(bool p_can_render)
{
	bool t_update;
	t_update = m_can_render_to_context != p_can_render;

	m_can_render_to_context = p_can_render;

	// IM-2016-03-16: [[ Bug 17138 ]] We may have to defer geometry updates, or apply deferred changes if this changes.
	//    This is handled by UpdateVisiblity()
	if (t_update)
		UpdateVisibility();
}

bool MCNativeLayer::GetCanRenderToContext()
{
	return m_can_render_to_context;
}

////////////////////////////////////////////////////////////////////////////////
