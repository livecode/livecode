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


#ifndef __MC_NATIVE_LAYER_MAC__
#define __MC_NATIVE_LAYER_MAC__

#include "native-layer.h"

#import <AppKit/NSView.h>
#import <AppKit/NSImageView.h>

class MCNativeLayerMac : public MCNativeLayer
{
public:
    
    virtual void OnOpen();
    virtual void OnClose();
    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnPaint(MCDC* p_dc, const MCRectangle& p_dirty);
    virtual void OnGeometryChanged(const MCRectangle& p_old_rect);
    virtual void OnVisibilityChanged(bool p_visible);
    virtual void OnToolChanged(Tool p_new_tool);
    virtual void OnLayerChanged();
    
	virtual bool GetNativeView(void *&r_view);
	
    MCNativeLayerMac(MCWidget*, NSView *p_view);
    ~MCNativeLayerMac();
    
private:
    
    MCWidget* m_widget;
    NSView* m_view;
    NSBitmapImageRep *m_cached;

    // Returns the NSWindow* for the stack containing this widget
    NSWindow* getStackWindow();
    
    // Performs the attach/detach operations
    void doAttach();
    void doDetach();
    
    // Performs a relayering operation
    void doRelayer();
};

#endif // ifndef __MC_NATIVE_LAYER_MAC__
