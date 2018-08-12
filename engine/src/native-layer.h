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


#ifndef __MC_NATIVE_LAYER__
#define __MC_NATIVE_LAYER__


class MCObject;

class MCNativeLayer
{
public:
    
    virtual void OnAttach();
    virtual void OnDetach();
	virtual bool OnPaint(MCGContextRef p_context);
    virtual void OnGeometryChanged(const MCRectangle& p_new_rect);
	virtual void OnViewTransformChanged();
	virtual void OnViewportGeometryChanged(const MCRectangle &p_rect);
    virtual void OnVisibilityChanged(bool p_visible);
    virtual void OnToolChanged(Tool p_new_tool);
    virtual void OnLayerChanged();
    
    virtual ~MCNativeLayer() = 0;
    
    // Returns true if the layer would be attached if its card were visible
    bool isAttached() const;
	
	virtual bool GetNativeView(void *&r_view) = 0;
	
	// Implemented by the platform-specific native layers: creates a new layer
	static MCNativeLayer *CreateNativeLayer(MCObject *p_object, void *p_native_view);
	static bool CreateNativeContainer(MCObject *p_object, void *&r_view);
	static void ReleaseNativeView(void *p_view);

	void SetCanRenderToContext(bool p_can_render);
	virtual bool GetCanRenderToContext();

protected:
	
	void UpdateVisibility();
	
	// Platform-specific implementations
	virtual void doAttach() = 0;
	virtual void doDetach() = 0;
	virtual bool doPaint(MCGContextRef p_context) = 0;
	virtual void doSetVisible(bool p_visible) = 0;
	virtual void doSetGeometry(const MCRectangle &p_rect) = 0;
	virtual void doSetViewportGeometry(const MCRectangle &p_rect) = 0;
	virtual void doRelayer() = 0;
	
	MCObject *m_object;
	
    bool m_attached;
	bool m_visible;
	bool m_show_for_tool;
    bool m_can_render_to_context;

	MCRectangle m_rect;
	MCRectangle m_viewport_rect;

	bool m_defer_geometry_changes;
	MCRectangle m_deferred_rect;
	MCRectangle m_deferred_viewport_rect;

    MCNativeLayer();
    
	// Returns true if the layer should be currently visible
	virtual bool ShouldShowLayer();
	
    // Utility function for subclasses: given a widget, finds the native layer
    // immediately below or above it. If none exist, returns nil.
    static MCObject* findNextLayerBelow(MCObject*);
    static MCObject* findNextLayerAbove(MCObject*);
};


#endif // ifndef __MC_NATIVE_LAYER__
