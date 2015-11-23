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

#ifndef __MC_WIDGET_NATIVE__
#define __MC_WIDGET_NATIVE__

#include "native-layer.h"

#include <windows.h>

class MCNativeLayerWin32 : public MCNativeLayer
{
public:
    
	virtual bool GetNativeView(void *&r_view);
	
    MCNativeLayerWin32(MCWidgetRef, HWND p_view);
    ~MCNativeLayerWin32();
    
private:
    
    HWND m_hwnd;
    HBITMAP m_cached;
    
    // Returns the HWND for the stack containing this widget
    HWND getStackWindow();
    
    // Performs the attach/detach operations
	virtual void doAttach();
	virtual void doDetach();
	
	virtual bool doPaint(MCGContextRef p_context);
	virtual void doSetGeometry(const MCRectangle &p_rect);
	virtual void doSetVisible(bool p_visible);
	
	// Performs a relayering operation
	virtual void doRelayer();
};

#endif // ifndef __MC_WIDGET_NATIVE__
