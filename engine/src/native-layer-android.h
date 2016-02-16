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


#ifndef __MC_NATIVE_LAYER_ANDROID__
#define __MC_NATIVE_LAYER_ANDROID__

#include "native-layer.h"

class MCNativeLayerAndroid : public MCNativeLayer
{
public:
    
	virtual bool GetCanRenderToContext();

    MCNativeLayerAndroid(MCObject *p_object, jobject p_view);
    ~MCNativeLayerAndroid();
    
	virtual bool GetNativeView(void *&r_view);

private:
    
    jobject m_view;
    
    // Returns the NSWindow* for the stack containing this widget
    //NSWindow* getStackWindow();
    
    // Performs the attach/detach operations
    virtual void doAttach();
    virtual void doDetach();
    
	virtual bool doPaint(MCGContextRef p_context);
	virtual void doSetGeometry(const MCRectangle &p_rect);
	virtual void doSetViewportGeometry(const MCRectangle &p_rect);
	virtual void doSetVisible(bool p_visible);
	
    // Performs a relayering operation
    virtual void doRelayer();
    
    // Show/hide operations
    void addToMainView();
	
	bool getParentView(jobject &r_view);
};

#endif // ifndef __MC_NATIVE_LAYER_ANDROID__
