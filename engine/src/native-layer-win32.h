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
};

#endif // ifndef __MC_WIDGET_NATIVE__
