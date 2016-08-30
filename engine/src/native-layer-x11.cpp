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

#include "lnxdc.h"
#include "graphicscontext.h"
#include "graphics_util.h"

#include "native-layer-x11.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>


MCNativeLayerX11::MCNativeLayerX11(MCObject *p_object, x11::Window p_view) :
  m_child_window(NULL),
  m_input_shape(NULL),
  m_socket(NULL),
  m_widget_xid(p_view)
{
	m_object = p_object;
	m_intersect_rect = MCRectangleMake(0,0,0,0);
}

MCNativeLayerX11::~MCNativeLayerX11()
{
    if (m_socket != NULL)
    {
        g_object_unref(m_socket);
    }
    if (m_child_window != NULL)
    {
        gtk_widget_destroy(GTK_WIDGET(m_child_window));
    }
    if (m_input_shape != NULL)
    {
        gdk_region_destroy(m_input_shape);
    }
}

void MCNativeLayerX11::OnToolChanged(Tool p_new_tool)
{
    updateInputShape();
	MCNativeLayer::OnToolChanged(p_new_tool);
}

void MCNativeLayerX11::updateInputShape()
{
    if (!m_show_for_tool)
        // In edit mode. Mask out all input events
        gdk_window_input_shape_combine_region(gtk_widget_get_window(GTK_WIDGET(m_child_window)), m_input_shape, 0, 0);
    else
        // In run mode. Unset the input event mask
        gdk_window_input_shape_combine_region(gtk_widget_get_window(GTK_WIDGET(m_child_window)), NULL, 0, 0);
}

void MCNativeLayerX11::doAttach()
{
    if (m_socket == NULL)
    {
        // Create a new GTK socket to deal with the XEMBED protocol
        GtkSocket *t_socket;
		t_socket = GTK_SOCKET(gtk_socket_new());
        
        // Create a new GTK window to hold the socket
        MCRectangle t_rect;
        t_rect = m_object->getrect();
        m_child_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
        gtk_widget_set_parent_window(GTK_WIDGET(m_child_window), getStackGdkWindow());
        gtk_widget_realize(GTK_WIDGET(m_child_window));
        gdk_window_reparent(gtk_widget_get_window(GTK_WIDGET(m_child_window)), getStackGdkWindow(), t_rect.x, t_rect.y);
        
        // Add the socket to the window
        gtk_container_add(GTK_CONTAINER(m_child_window), GTK_WIDGET(t_socket));
        
        // The socket needs to be realised before going any further or any
        // operations on it will fail.
        gtk_widget_realize(GTK_WIDGET(t_socket));
        
        // Show the socket (we'll control visibility at the window level)
        gtk_widget_show(GTK_WIDGET(t_socket));
        
        // Create an empty region to act as an input mask while in edit mode
        m_input_shape = gdk_region_new();

		// Retain a reference to the socket
		m_socket = GTK_SOCKET(g_object_ref(G_OBJECT(t_socket)));
    }
    
    // Attach the X11 window to this socket
    if (gtk_socket_get_plug_window(m_socket) == NULL)
        gtk_socket_add_id(m_socket, m_widget_xid);
    //fprintf(stderr, "XID: %u\n", gtk_socket_get_id(m_socket));
    
    // Act as if there were a re-layer to put the widget in the right place
    doRelayer();
    doSetViewportGeometry(m_viewport_rect);
    doSetGeometry(m_rect);
    doSetVisible(ShouldShowLayer());
}

void MCNativeLayerX11::doDetach()
{
    // We don't really detach; just stop showing the socket
    gtk_widget_hide(GTK_WIDGET(m_child_window));
}

// We can't get a snapshot of X11 windows so override this to return false
bool MCNativeLayerX11::GetCanRenderToContext()
{
	return false;
}

bool MCNativeLayerX11::doPaint(MCGContextRef p_context)
{
    return false;
}

void MCNativeLayerX11::updateContainerGeometry()
{
	m_intersect_rect = MCU_intersect_rect(m_viewport_rect, m_rect);

    // Clear any minimum size parameters for the GTK widgets
    gtk_widget_set_size_request(GTK_WIDGET(m_child_window), -1, -1);

    // Resize by adjusting the widget's containing GtkWindow
    gdk_window_move_resize(gtk_widget_get_window(GTK_WIDGET(m_child_window)), m_intersect_rect.x, m_intersect_rect.y, m_intersect_rect.width, m_intersect_rect.height);

    // We need to set the requested minimum size in order to get in-process GTK
    // widgets to re-size automatically. Unfortunately, that is the only widget
    // category that this works for... others need to do it themselves.
    gtk_widget_set_size_request(GTK_WIDGET(m_child_window), m_intersect_rect.width, m_intersect_rect.height);
}

void MCNativeLayerX11::doSetViewportGeometry(const MCRectangle &p_rect)
{
	m_viewport_rect = p_rect;
	updateContainerGeometry();
}

// IM-2016-01-21: [[ NativeLayer ]] Place the socket window relative to its
//    container, so only the visible area (clipped by any containing groups)
//    is displayed.
void MCNativeLayerX11::doSetGeometry(const MCRectangle& p_rect)
{
	m_rect = p_rect;
	updateContainerGeometry();
	
	MCRectangle t_rect;
	t_rect = m_rect;
	t_rect.x -= m_intersect_rect.x;
	t_rect.y -= m_intersect_rect.y;
	
    // Move the overlay window first, to ensure events don't get stolen

    // Clear any minimum size parameters for the GTK widgets
    gtk_widget_set_size_request(GTK_WIDGET(m_socket), -1, -1);
    
    // Resize the socket
    gdk_window_move_resize(gtk_widget_get_window(GTK_WIDGET(m_socket)), t_rect.x, t_rect.y, t_rect.width, t_rect.height);
    
    // We need to set the requested minimum size in order to get in-process GTK
    // widgets to re-size automatically. Unfortunately, that is the only widget
    // category that this works for... others need to do it themselves.
    gtk_widget_set_size_request(GTK_WIDGET(m_socket), t_rect.width, t_rect.height);
    
    // Update the contained window too
    GdkWindow* t_remote;
    t_remote = gtk_socket_get_plug_window(m_socket);
    if (t_remote != NULL)
        gdk_window_move_resize(t_remote, t_rect.x, t_rect.y, t_rect.width, t_rect.height);
}

void MCNativeLayerX11::doSetVisible(bool p_visible)
{
    if (p_visible)
        gtk_widget_show(GTK_WIDGET(m_child_window));
    else
        gtk_widget_hide(GTK_WIDGET(m_child_window));

	if (p_visible)
		doSetGeometry(m_object->getrect());
		
	updateInputShape();
}

void MCNativeLayerX11::doRelayer()
{
    // Ensure that the input mask for the widget is up to date
    updateInputShape();
    
    // Find which native layer this should be inserted below
    MCObject *t_before;
    t_before = findNextLayerAbove(m_object);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getstack()->getcard() == m_object->getstack()->getcurcard())
    {
        // If t_before_window == NULL, this will put the widget on the bottom layer
        MCNativeLayerX11 *t_before_layer;
        GdkWindow* t_before_window;
        if (t_before != NULL)
        {
            t_before_layer = reinterpret_cast<MCNativeLayerX11*>(t_before->getNativeLayer());
            t_before_window = gtk_widget_get_window(GTK_WIDGET(t_before_layer->m_child_window));
        }
        else
        {
            t_before_layer = NULL;
            t_before_window = NULL;
        }
        gdk_window_restack(gtk_widget_get_window(GTK_WIDGET(m_child_window)), t_before_window, FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeLayerX11::GetNativeView(void *&r_view)
{
    r_view = (void*)m_widget_xid;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

x11::Window MCNativeLayerX11::getStackX11Window()
{
    return x11::gdk_x11_drawable_get_xid(getStackGdkWindow());
}

GdkWindow* MCNativeLayerX11::getStackGdkWindow()
{
    return m_object->getstack()->getwindow();
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_native_view)
{
    return new MCNativeLayerX11(p_object, (x11::Window)p_native_view);
}

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
	return false;
}

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
}

////////////////////////////////////////////////////////////////////////////////
