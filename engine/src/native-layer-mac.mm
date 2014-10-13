#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"

#include "globals.h"
#include "context.h"

#include "widget-native-mac.h"

#import <AppKit/NSWindow.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSPanel.h>
#import <AppKit/NSMenu.h>
#import <AppKit/NSTextInputClient.h>
#import <AppKit/NSImage.h>
#include "platform.h"
#include "platform-internal.h"
#include "mac-internal.h"

#import <AppKit/NSSegmentedControl.h>

MCNativeWidgetMac::MCNativeWidgetMac() :
  m_view(nil),
  m_cached(nil)
{
    
}

MCNativeWidgetMac::MCNativeWidgetMac(const MCNativeWidgetMac& p_clone) :
  MCWidget(p_clone),
  m_view(nil),
  m_cached(nil)
{
    
}

MCNativeWidgetMac::~MCNativeWidgetMac()
{
    if (m_view != nil)
    {
        [m_view removeFromSuperview];
        [m_view release];
    }
    if (m_cached != nil)
        [m_cached release];
}

MCWidget *MCNativeWidgetMac::clone(Boolean p_attach, Object_pos p_position, bool invisible)
{
    MCWidget *t_new_widget;
    t_new_widget = new MCNativeWidgetMac(*this);
    if (p_attach)
        t_new_widget -> attach(p_position, invisible);
    return t_new_widget;
}

void MCNativeWidgetMac::toolchanged(Tool p_new_tool)
{
    if (p_new_tool == T_BROWSE || p_new_tool == T_HELP)
    {
        // In run mode. Make visible if requested and our card is current.
        if ((flags & F_VISIBLE) && getcard() == getstack()->getcurcard())
            [m_view setHidden:NO];
        Redraw();
    }
    else
    {
        // In edit mode
        [m_view setHidden:YES];
        Redraw();
    }
}

bool MCNativeWidgetMac::isNative() const
{
    return true;
}

void MCNativeWidgetMac::nativeOpen()
{
    // Unhide the widget, if required
    if (flags & F_VISIBLE && !inEditMode())
        [m_view setHidden:NO];
}

void MCNativeWidgetMac::nativeClose()
{
    [m_view setHidden:YES];
}

void MCNativeWidgetMac::nativePaint(MCDC* p_dc, const MCRectangle& p_dirty)
{
    realise();
    
    // If the widget is not in edit mode, we trust it to paint itself
    if (!inEditMode())
        return;

    // Get an image rep suitable for storing the cached bitmap
    if (m_cached == nil)
    {
        m_cached = [[m_view bitmapImageRepForCachingDisplayInRect:[m_view bounds]] retain];
    }
    
    // Draw the widget
    bzero([m_cached bitmapData], [m_cached bytesPerRow] * [m_cached pixelsHigh]);
    [m_view cacheDisplayInRect:[m_view bounds] toBitmapImageRep:m_cached];
    
    // Turn the NSBitmapImageRep into something we can actually draw
    MCGRaster t_raster;
    MCGImageRef t_gimage;
    MCImageDescriptor t_descriptor;
    t_raster.format = kMCGRasterFormat_ARGB;
    t_raster.width = [m_cached pixelsWide];
    t_raster.height = [m_cached pixelsHigh];
    t_raster.stride = [m_cached bytesPerRow];
    t_raster.pixels = [m_cached bitmapData];
    /* UNCHECKED */ MCGImageCreateWithRasterNoCopy(t_raster, t_gimage);
    memset(&t_descriptor, 0, sizeof(MCImageDescriptor));
    t_descriptor.image = t_gimage;
    t_descriptor.x_scale = t_descriptor.y_scale = 1.0;
    
    // Draw the image
    // FG-2014-10-10: a y offset of 1 is needed to keep things lined up, for some reason...
    p_dc->drawimage(t_descriptor, 0, 0, t_raster.width, t_raster.height, 0, 1);
    MCGImageRelease(t_gimage);
}

void MCNativeWidgetMac::nativeGeometryChanged(const MCRectangle& p_rect)
{
    realise();
    
    NSRect t_nsrect;
    MCRectangle t_cardrect;
    t_cardrect = getcard()->getrect();
    t_nsrect = NSMakeRect(rect.x, t_cardrect.height-rect.y-rect.height-1, rect.width, rect.height);
    [m_view setFrame:t_nsrect];
    [m_view setNeedsDisplay:YES];
    [m_cached release];
    m_cached = nil;
}

void MCNativeWidgetMac::nativeVisibilityChanged(bool p_visible)
{
    realise();
    
    [m_view setHidden:!p_visible];
}

void MCNativeWidgetMac::realise()
{
    if (m_view != nil)
        return;
    
    NSRect t_nsrect;
    MCRectangle t_cardrect;
    t_cardrect = getcard()->getrect();
    t_nsrect = NSMakeRect(rect.x, t_cardrect.height-rect.y-rect.height-1, rect.width, rect.height);
    
    NSButton *t_button;
    t_button = [[NSButton alloc] initWithFrame:t_nsrect];
    [t_button setTitle:@"Native button"];
    [t_button setButtonType:NSMomentaryPushInButton];
    [t_button setBezelStyle:NSRoundedBezelStyle];
    //[t_button setEnabled:NO];
    [t_button setHidden:YES];
    m_view = t_button;
    
    /*NSSegmentedControl *t_control;
    t_control = [[NSSegmentedControl alloc] initWithFrame:t_nsrect];
    [t_control setSegmentStyle:NSSegmentStyleRounded];
    [t_control setSegmentCount:4];
    [t_control setLabel:@"Foo" forSegment:0];
    [t_control setLabel:@"Bar" forSegment:1];
    [t_control setLabel:@"Baz" forSegment:2];
    [t_control setLabel:@"Quux" forSegment:3];
    m_view = t_control;*/
    
    NSWindow *t_window = getStackWindow();
    [[t_window contentView] addSubview:m_view];
}

NSWindow* MCNativeWidgetMac::getStackWindow()
{
    return ((MCMacPlatformWindow*)(getstack()->getwindow()))->GetHandle();
}
