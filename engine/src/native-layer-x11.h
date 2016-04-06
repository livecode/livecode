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

#ifndef __MC_NATIVE_LAYER_X11__
#define __MC_NATIVE_LAYER_X11__

#include "native-layer.h"

#include <gtk/gtk.h>

namespace x11
{
#include <gdk/gdkx.h>
}

class MCNativeLayerX11 : public MCNativeLayer
{
public:
    virtual void OnToolChanged(Tool p_new_tool);
	
	virtual bool GetCanRenderToContext();
    
    virtual bool GetNativeView(void *&r_view);
    
    MCNativeLayerX11(MCObject *p_object, x11::Window p_view);
    ~MCNativeLayerX11();
    
private:
    
    GtkWindow* m_child_window;
    GdkRegion* m_input_shape;
    GtkSocket* m_socket;
    x11::Window m_widget_xid;
	MCRectangle m_intersect_rect;
    
    // Returns the handle for the stack containing this widget
    x11::Window getStackX11Window();
    GdkWindow* getStackGdkWindow();
    
    // Returns the GtkFixed used for layouts within the stack
    GtkFixed* getStackLayout();
    
    // Performs the attach/detach operations
    virtual void doAttach();
    virtual void doDetach();
	
	virtual bool doPaint(MCGContextRef p_context);
	virtual void doSetGeometry(const MCRectangle &p_rect);
	virtual void doSetViewportGeometry(const MCRectangle &p_rect);
	virtual void doSetVisible(bool p_visible);
    
    // Performs a relayering operation
    virtual void doRelayer();
    
    // Updates the input mask for the widget (used to implement edit mode)
    void updateInputShape();
    
	void updateContainerGeometry();
};

#endif // ifndef __MC_NATIVE_LAYER_X11__
