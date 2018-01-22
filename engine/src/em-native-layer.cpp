/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "em-native-layer.h"

////////////////////////////////////////////////////////////////////////////////

extern "C" void MCEmscriptenElementSetRect(int p_element, int p_left, int p_top, int p_right, int p_bottom);
extern "C" void MCEmscriptenElementSetClip(int p_element, int p_left, int p_top, int p_right, int p_bottom);
extern "C" void MCEmscriptenElementSetVisible(int p_element, bool p_visible);
extern "C" void MCEmscriptenElementAddToWindow(int p_element, int p_container);
extern "C" void MCEmscriptenElementRemoveFromWindow(int p_element, int p_container);
extern "C" void MCEmscriptenElementPlaceAbove(int p_element, int p_above, int p_container);

////////////////////////////////////////////////////////////////////////////////

inline void MCEmscriptenElementSetRect(int p_element, const MCRectangle &p_rect)
{
	MCEmscriptenElementSetRect(p_element, p_rect.x, p_rect.y, p_rect.x + p_rect.width, p_rect.y + p_rect.height);
}

inline void MCEmscriptenElementSetClip(int p_element, const MCRectangle &p_rect)
{
	MCEmscriptenElementSetClip(p_element, p_rect.x, p_rect.y, p_rect.x + p_rect.width, p_rect.y + p_rect.height);
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayerEmscripten::MCNativeLayerEmscripten(MCObject *p_object, int p_element)
{
	MCLog("new native layer for %p, %d", p_object, p_element);
	m_object = p_object;
	m_element = p_element;
}

MCNativeLayerEmscripten::~MCNativeLayerEmscripten()
{
}

void MCNativeLayerEmscripten::doAttach()
{
    if (!m_element)
        return;
	
	MCLog("attach element %d", m_element);
	MCEmscriptenElementAddToWindow(m_element, getStackWindow());

    // Act as if there were a re-layer to put the widget in the right place
    doRelayer();
    doSetViewportGeometry(m_viewport_rect);
    doSetGeometry(m_rect);
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayerEmscripten::doDetach()
{
    if (!m_element)
        return;
    
    // Remove the element from the stack's content view
    MCLog("detach element %d");
	MCEmscriptenElementRemoveFromWindow(m_element, getStackWindow());
}

// Rendering view to context not supported in HTML5.
bool MCNativeLayerEmscripten::GetCanRenderToContext()
{
	return false;
}

bool MCNativeLayerEmscripten::doPaint(MCGContextRef p_context)
{
	return false;
}

void MCNativeLayerEmscripten::doSetViewportGeometry(const MCRectangle& p_rect)
{
	if (!m_element)
		return;
		
	MCLog("set element rect");
	MCEmscriptenElementSetClip(m_element, p_rect);
}

void MCNativeLayerEmscripten::doSetGeometry(const MCRectangle &p_rect)
{
    if (!m_element)
        return;
	
	MCEmscriptenElementSetRect(m_element, p_rect);
}

void MCNativeLayerEmscripten::doSetVisible(bool p_visible)
{
    if (!m_element)
        return;
    
    MCEmscriptenElementSetVisible(m_element, p_visible);
}

void MCNativeLayerEmscripten::doRelayer()
{
    // Find which native layer this should be inserted above
    MCObject *t_previous;
    t_previous = findNextLayerBelow(m_object);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getstack()->getcard() == m_object->getstack()->getcurcard())
    {
        // If t_previous_element == 0, this will put the element on the bottom layer
        int t_previous_element = 0;

        if (t_previous != nil)
        {
			MCNativeLayerEmscripten *t_previous_layer = nil;
            t_previous_layer = reinterpret_cast<MCNativeLayerEmscripten*>(t_previous->getNativeLayer());
            t_previous_element = t_previous_layer->m_element;
		}

		MCEmscriptenElementPlaceAbove(m_element, t_previous_element, getStackWindow());
    }
}

bool MCNativeLayerEmscripten::GetNativeView(void *&r_view)
{
	if (!m_element)
		return false;
	
	r_view = (void*)m_element;
	
	return true;
}

int MCNativeLayerEmscripten::getStackWindow()
{
    return (int)m_object->getstack()->getwindow();
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_view)
{
    return new MCNativeLayerEmscripten(p_object, (int)p_view);
}

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
	return false;
}

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
}

////////////////////////////////////////////////////////////////////////////////
