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


#ifndef __MC_NATIVE_LAYER_EMSCRIPTEN__
#define __MC_NATIVE_LAYER_EMSCRIPTEN__

#include "native-layer.h"
#include "jsobject.h"

class MCNativeLayerEmscripten : public MCNativeLayer
{
public:
    MCNativeLayerEmscripten(MCObject *p_object, MCJSObjectRef p_element);
    ~MCNativeLayerEmscripten();
    
	virtual bool GetCanRenderToContext();
	virtual bool GetNativeView(void *&r_view);

private:
    // Performs the attach/detach operations
    virtual void doAttach();
    virtual void doDetach();
    
	virtual bool doPaint(MCGContextRef p_context);
	virtual void doSetGeometry(const MCRectangle &p_rect);
	virtual void doSetViewportGeometry(const MCRectangle &p_rect);
	virtual void doSetVisible(bool p_visible);
	
    // Performs a relayering operation
    virtual void doRelayer();
    
    MCJSObjectID getStackWindow();

    MCJSObjectRef  m_element;
};

#endif // ifndef __MC_NATIVE_LAYER_EMSCRIPTEN__
