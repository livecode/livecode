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
#include "objptr.h"

#include "globals.h"
#include "context.h"

#include "widget.h"
#include "native-layer-mac.h"

#import <AppKit/NSWindow.h>
#import <AppKit/NSPanel.h>
#import <AppKit/NSMenu.h>
#import <AppKit/NSTextInputClient.h>
#import <AppKit/NSImage.h>
#include "platform.h"
#include "platform-internal.h"
#include "mac-internal.h"


MCNativeLayerMac::MCNativeLayerMac(MCWidgetRef p_widget, NSView *p_view) :
  m_widget(p_widget),
  m_view(p_view),
  m_cached(nil)
{
	[m_view retain];
}

MCNativeLayerMac::~MCNativeLayerMac()
{
    if (m_view != nil)
    {
        doDetach();
        [m_view release];
    }
    if (m_cached != nil)
    {
        [m_cached release];
    }
}

void MCNativeLayerMac::OnToolChanged(Tool p_new_tool)
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    if (p_new_tool == T_BROWSE || p_new_tool == T_HELP)
    {
        // In run mode. Make visible if requested
        if (t_widget->getflags() & F_VISIBLE)
            [m_view setHidden:NO];
        t_widget->Redraw();
    }
    else
    {
        // In edit mode
        [m_view setHidden:YES];
        t_widget->Redraw();
    }
}

void MCNativeLayerMac::OnOpen()
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    // Unhide the widget, if required
    if (isAttached() && t_widget->getopened() == 1)
        doAttach();
}

void MCNativeLayerMac::OnClose()
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    if (isAttached() && t_widget->getopened() == 1)
        doDetach();
}

#import <AppKit/NSButton.h>

void MCNativeLayerMac::OnAttach()
{
    m_attached = true;
    doAttach();
}

void MCNativeLayerMac::doAttach()
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    // Act as if there was a re-layer to put the widget in the right place
    // *** Can we assume open happens in back-to-front order? ***
    doRelayer();
    
    // Restore the visibility state of the widget (in case it changed due to a
    // tool change while on another card - we don't get a message then)
    if ((t_widget->getflags() & F_VISIBLE) && t_widget->isInRunMode())
        [m_view setHidden:NO];
    else
        [m_view setHidden:YES];
}

void MCNativeLayerMac::OnDetach()
{
    m_attached = false;
    doDetach();
}

void MCNativeLayerMac::doDetach()
{
    // Remove the view from the stack's content view
    [m_view removeFromSuperview];
}

void MCNativeLayerMac::OnPaint(MCGContextRef p_context)
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    // If the widget is not in edit mode, we trust it to paint itself
    if (t_widget->isInRunMode())
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
    t_raster.format = kMCGRasterFormat_ARGB;
    t_raster.width = [m_cached pixelsWide];
    t_raster.height = [m_cached pixelsHigh];
    t_raster.stride = [m_cached bytesPerRow];
    t_raster.pixels = [m_cached bitmapData];
    /* UNCHECKED */ MCGImageCreateWithRasterNoCopy(t_raster, t_gimage);
    
    // Draw the image
    // FG-2014-10-10: a y offset of 1 is needed to keep things lined up, for some reason...
    MCGRectangle rect = {{0, 1}, {t_raster.width, t_raster.height}};
    MCGContextDrawImage(p_context, t_gimage, rect, kMCGImageFilterNone);
    MCGImageRelease(t_gimage);
}

void MCNativeLayerMac::OnGeometryChanged(const MCRectangle& p_old_rect)
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    NSRect t_nsrect;
    MCRectangle t_rect, t_cardrect;
    t_rect = t_widget->getrect();
    t_cardrect = t_widget->getcard()->getrect();
    t_nsrect = NSMakeRect(t_rect.x, t_cardrect.height-t_rect.y-t_rect.height-1, t_rect.width, t_rect.height);
    [m_view setFrame:t_nsrect];
    [m_view setNeedsDisplay:YES];
    [m_cached release];
    m_cached = nil;
}

void MCNativeLayerMac::OnVisibilityChanged(bool p_visible)
{
    [m_view setHidden:!p_visible];
}

void MCNativeLayerMac::OnLayerChanged()
{
    doRelayer();
}

void MCNativeLayerMac::doRelayer()
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    
    // Find which native layer this should be inserted below
    MCWidget* t_before;
    t_before = findNextLayerAbove(t_widget);
    
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && t_widget->getstack()->getcard() == t_widget->getstack()->getcurcard())
    {
        [m_view removeFromSuperview];
        if (t_before != nil)
        {
            // There is another native layer above this one
            MCNativeLayerMac *t_before_layer;
            t_before_layer = reinterpret_cast<MCNativeLayerMac*>(t_before->getNativeLayer());
            [[getStackWindow() contentView] addSubview:m_view positioned:NSWindowBelow relativeTo:t_before_layer->m_view];
        }
        else
        {
            // This is the top-most native layer
            [[getStackWindow() contentView] addSubview:m_view];
        }
        [[getStackWindow() contentView] setNeedsDisplay:YES];
    }
}

NSWindow* MCNativeLayerMac::getStackWindow()
{
    MCWidget* t_widget = MCWidgetGetHost(m_widget);
    return ((MCMacPlatformWindow*)(t_widget->getstack()->getwindow()))->GetHandle();
}

bool MCNativeLayerMac::GetNativeView(void *&r_view)
{
	r_view = m_view;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCWidgetRef p_widget, void *p_view)
{
	if (p_view == nil)
		return nil;
	
    return new MCNativeLayerMac(p_widget, (NSView*)p_view);
}

