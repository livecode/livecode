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


#ifndef __MC_NATIVE_LAYER__
#define __MC_NATIVE_LAYER__


class MCNativeLayer
{
public:
    
    virtual void OnOpen() = 0;
    virtual void OnClose() = 0;
    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;
    virtual void OnPaint(MCDC* p_dc, const MCRectangle& p_dirty) = 0;
    virtual void OnGeometryChanged(const MCRectangle& p_old_rect) = 0;
    virtual void OnVisibilityChanged(bool p_visible) = 0;
    virtual void OnToolChanged(Tool p_new_tool) = 0;
    virtual void OnLayerChanged() = 0;
    
    virtual ~MCNativeLayer() = 0;
    
    // Returns true if the layer would be attached if its card were visible
    bool isAttached() const;
    
protected:
    
    bool m_attached;
    
    MCNativeLayer();
    
    // Utility function for subclasses: given a widget, finds the native layer
    // immediately below or above it. If none exist, returns nil.
    static MCWidget* findNextLayerBelow(MCWidget*);
    static MCWidget* findNextLayerAbove(MCWidget*);
};


#endif // ifndef __MC_NATIVE_LAYER__
