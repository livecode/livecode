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

#ifndef __MC_WIDGET_NATIVE__
#define __MC_WIDGET_NATIVE__

#include "native-layer.h"

#include <windows.h>

class MCNativeLayerWin32 : public MCNativeLayer
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
    
    MCNativeLayerWin32(MCWidget*);
    ~MCNativeLayerWin32();
    
private:
    
    MCWidget* m_widget;
    HWND m_hwnd;
    HBITMAP m_cached;
    
    // Returns the HWND for the stack containing this widget
    HWND getStackWindow();
    
    // Performs the attach/detach operations
    void doAttach();
    void doDetach();

	// Performs a relayering operation
	void doRelayer();
};

#endif // ifndef __MC_WIDGET_NATIVE__
