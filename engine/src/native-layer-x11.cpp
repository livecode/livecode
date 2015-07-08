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

#include "lnxdc.h"
#include "graphicscontext.h"

#include "native-layer-x11.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>


MCNativeLayerX11::MCNativeLayerX11(MCWidget* p_widget) :
  m_widget(p_widget),
  m_child_window(NULL),
  m_input_shape(NULL),
  m_socket(NULL),
  m_widget_xid(0)
{
    
}

MCNativeLayerX11::~MCNativeLayerX11()
{
    if (m_socket != NULL)
    {
        g_object_unref(m_socket);
    }
    if (m_child_window != NULL)
    {
        g_object_unref(m_child_window);
    }
    if (m_input_shape != NULL)
    {
        gdk_region_destroy(m_input_shape);
    }
}

void MCNativeLayerX11::OnToolChanged(Tool p_new_tool)
{
    updateInputShape();
}

void MCNativeLayerX11::updateInputShape()
{
    if (m_widget->inEditMode())
        // In edit mode. Mask out all input events
        gdk_window_input_shape_combine_region(gtk_widget_get_window(GTK_WIDGET(m_child_window)), m_input_shape, 0, 0);
    else
        // In run mode. Unset the input event mask
        gdk_window_input_shape_combine_region(gtk_widget_get_window(GTK_WIDGET(m_child_window)), NULL, 0, 0);
}

void MCNativeLayerX11::OnOpen()
{
    // Unhide the widget, if required
    if (isAttached() && m_widget->getopened() == 1)
        doAttach();
}

void MCNativeLayerX11::OnClose()
{
    if (isAttached() && m_widget->getopened() == 0)
        doDetach();
}

void MCNativeLayerX11::OnAttach()
{
    m_attached = true;
    doAttach();
}

void MCNativeLayerX11::doAttach()
{
    if (m_socket == NULL)
    {
        // Create a new GTK socket to deal with the XEMBED protocol
        m_socket = GTK_SOCKET(gtk_socket_new());
        
        // Create a new GTK window to hold the socket
        MCRectangle t_rect;
        t_rect = m_widget->getrect();
        m_child_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
        gtk_widget_set_parent_window(GTK_WIDGET(m_child_window), getStackGdkWindow());
        gtk_widget_realize(GTK_WIDGET(m_child_window));
        gdk_window_reparent(gtk_widget_get_window(GTK_WIDGET(m_child_window)), getStackGdkWindow(), t_rect.x, t_rect.y);
        
        // Add the socket to the window
        gtk_container_add(GTK_CONTAINER(m_child_window), GTK_WIDGET(m_socket));
        
        // The socket needs to be realised before going any further or any
        // operations on it will fail.
        gtk_widget_realize(GTK_WIDGET(m_socket));
        
        // Show the socket (we'll control visibility at the window level)
        gtk_widget_show(GTK_WIDGET(m_socket));
        
        // Create an empty region to act as an input mask while in edit mode
        m_input_shape = gdk_region_new();
        
        // TESTING
        
        /*GtkPlug *t_plug = GTK_PLUG(gtk_plug_new(0));
        gtk_widget_realize(GTK_WIDGET(t_plug));
        m_widget_xid = gtk_plug_get_id(t_plug);
        GtkButton* t_button = GTK_BUTTON(gtk_button_new_with_label("Native Button"));
        gtk_container_add(GTK_CONTAINER(t_plug), GTK_WIDGET(t_button));
        gtk_widget_show(GTK_WIDGET(t_button));
        gtk_widget_show(GTK_WIDGET(t_plug));*/
    }
    
    // Attach the X11 window to this socket
    if (gtk_socket_get_plug_window(m_socket) == NULL)
        gtk_socket_add_id(m_socket, m_widget_xid);
    //fprintf(stderr, "XID: %u\n", gtk_socket_get_id(m_socket));
    
    // Act as if there were a re-layer to put the widget in the right place
    doRelayer();
    
    // Restore the visibility state of the widget (in case it changed due to a
    // tool change while on another card - we don't get a message then)
    if ((m_widget->getflags() & F_VISIBLE) && !m_widget->inEditMode())
        gtk_widget_hide(GTK_WIDGET(m_child_window));
    else
        gtk_widget_show(GTK_WIDGET(m_child_window));
}

void MCNativeLayerX11::OnDetach()
{
    m_attached = false;
    doDetach();
}

void MCNativeLayerX11::doDetach()
{
    // We don't really detach; just stop showing the socket
    gtk_widget_hide(GTK_WIDGET(m_child_window));
}

void MCNativeLayerX11::OnPaint(MCDC* p_dc, const MCRectangle& p_dirty)
{
    // Do nothing. Painting is handled entirely by X11.
}

void MCNativeLayerX11::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    // Move the overlay window first, to ensure events don't get stolen
    MCRectangle t_rect;
    t_rect = m_widget->getrect();

    // Clear any minimum size parameters for the GTK widgets
    gtk_widget_set_size_request(GTK_WIDGET(m_socket), -1, -1);
    gtk_widget_set_size_request(GTK_WIDGET(m_child_window), -1, -1);
    
    // Resize by adjusting the widget's containing GtkWindow
    gdk_window_move_resize(gtk_widget_get_window(GTK_WIDGET(m_child_window)), t_rect.x, t_rect.y, t_rect.width, t_rect.height);
    
    // Resize the socket
    gdk_window_move_resize(gtk_widget_get_window(GTK_WIDGET(m_socket)), 0, 0, t_rect.width, t_rect.height);
    
    // We need to set the requested minimum size in order to get in-process GTK
    // widgets to re-size automatically. Unfortunately, that is the only widget
    // category that this works for... others need to do it themselves.
    gtk_widget_set_size_request(GTK_WIDGET(m_child_window), t_rect.width, t_rect.height);
    gtk_widget_set_size_request(GTK_WIDGET(m_socket), t_rect.width, t_rect.height);
    
    // Update the contained window too
    GdkWindow* t_remote;
    t_remote = gtk_socket_get_plug_window(m_socket);
    if (t_remote != NULL)
        gdk_window_move_resize(t_remote, 0, 0, t_rect.width, t_rect.height);
}

void MCNativeLayerX11::OnVisibilityChanged(bool p_visible)
{
    if (p_visible)
        gtk_widget_show(GTK_WIDGET(m_child_window));
    else
        gtk_widget_hide(GTK_WIDGET(m_child_window));
}

void MCNativeLayerX11::OnLayerChanged()
{
    doRelayer();
}

void MCNativeLayerX11::doRelayer()
{
    // Ensure that the input mask for the widget is up to date
    updateInputShape();
    
    // Find which native layer this should be inserted below
    MCWidget* t_before;
    t_before = findNextLayerAbove(m_widget);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_widget->getstack()->getcard() == m_widget->getstack()->getcurcard())
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
    
    // Make the widget visible, if appropriate
    if ((m_widget->getflags() & F_VISIBLE) && m_widget->getstack()->getcurcard() != m_widget->getstack()->getcard())
        gdk_window_show_unraised(gtk_widget_get_window(GTK_WIDGET(m_child_window)));
}

////////////////////////////////////////////////////////////////////////////////

x11::Window MCNativeLayerX11::getStackX11Window()
{
    return x11::gdk_x11_drawable_get_xid(getStackGdkWindow());
}

GdkWindow* MCNativeLayerX11::getStackGdkWindow()
{
    return m_widget->getstack()->getwindow();
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCWidget::createNativeLayer()
{
    return new MCNativeLayerX11(this);
}
