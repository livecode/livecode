#ifndef __MC_WIDGET_NATIVE__
#define __MC_WIDGET_NATIVE__

#include "widget.h"

#include <windows.h>

class MCNativeWidgetWin32 : public MCWidget
{
public:
    
    MCNativeWidgetWin32();
    MCNativeWidgetWin32(const MCNativeWidgetWin32&);
    ~MCNativeWidgetWin32();
    virtual MCWidget *clone(Boolean p_attach, Object_pos p_position, bool invisible);
    
    // Native widgets need to be hidden when edit mode is entered
    virtual void toolchanged(Tool p_new_tool);
    
protected:
    
    virtual bool isNative() const;
    virtual void nativeOpen();
    virtual void nativeClose();
    virtual void nativePaint(MCDC* p_dc, const MCRectangle& p_dirty);
    virtual void nativeGeometryChanged(const MCRectangle& p_old_rect);
    virtual void nativeVisibilityChanged(bool p_visible);
    
private:
    
    HWND m_hwnd;
    HBITMAP m_cached;
    
    // Returns the HWND for the stack containing this widget
    HWND getStackWindow();
};

#endif // ifndef __MC_WIDGET_NATIVE__
