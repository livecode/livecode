#ifndef __MC_WIDGET_NATIVE__
#define __MC_WIDGET_NATIVE__

#include "widget.h"

#import <AppKit/NSView.h>
#import <AppKit/NSImageView.h>

class MCNativeWidgetMac : public MCWidget
{
public:
    
    MCNativeWidgetMac();
    MCNativeWidgetMac(const MCNativeWidgetMac&);
    ~MCNativeWidgetMac();
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
    
    NSView* m_view;
    NSBitmapImageRep *m_cached;
    
    void realise();
    
    // Returns the NSWindow* for the stack containing this widget
    NSWindow* getStackWindow();
};

#endif // ifndef __MC_WIDGET_NATIVE__
