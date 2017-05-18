/*                                                                     -*-c++-*-
 Copyright (C) 2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "mac-platform.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCContainerView

- (void)setFrameOrigin:(NSPoint)newOrigin
{
   /* By default, NSViews don't adjust the positions or sizes of their
    * contents when their geometry changes.  There is an autoresizing
    * behaviour that we could turn on, but we don't because it doesn't have
    * the properties that we require for a LiveCode group.  It's therefore
    * necessary to manually nudge subviews around when the position of a
    * container view is adjusted. */
   NSPoint t_origin = [self frame].origin;
   NSPoint t_delta = {newOrigin.x - t_origin.x, newOrigin.y - t_origin.y};

   for (NSView *t_subview in [self subviews])
   {
       NSPoint t_suborigin = [t_subview frame].origin;
       [t_subview setFrameOrigin: {t_suborigin.x + t_delta.x,
                                   t_suborigin.y + t_delta.y}];
   }

    [super setFrameOrigin:newOrigin];
    [self setBoundsOrigin:newOrigin];
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    [self setBoundsSize:newSize];
}

@end


////////////////////////////////////////////////////////////////////////////////

MCMacPlatformNativeLayer::~MCMacPlatformNativeLayer()
{
    if (m_view != nil)
    {
        Detach();
        [m_view release];
    }
    
    if (m_cached != nil)
        [m_cached release];
    
    if (m_window != nil)
        m_window -> Release();
}

void MCMacPlatformNativeLayer::Attach(MCPlatformWindowRef p_window, void *p_container_view, void *p_view_above, bool p_visible)
{
    if (m_window != nil)
        m_window -> Release();
    
    m_window = p_window;
    m_window -> Retain();
    
    Relayer(p_container_view, p_view_above);
    SetVisible(p_visible);
}

void MCMacPlatformNativeLayer::Detach()
{
    // Remove the view from the stack's content view
    [m_view removeFromSuperview];
}

bool MCMacPlatformNativeLayer::Paint(MCGContextRef p_context)
{
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
    
    if (!MCGImageCreateWithRasterNoCopy(t_raster, t_gimage))
        return false;
    
    // Draw the image
    // FG-2014-10-10: a y offset of 1 is needed to keep things lined up, for some reason...
    MCGRectangle rect = {{0, 1}, {MCGFloat(t_raster.width), MCGFloat(t_raster.height)}};
    MCGContextDrawImage(p_context, t_gimage, rect, kMCGImageFilterNone);
    MCGImageRelease(t_gimage);
    
    return true;
}

NSRect MCMacPlatformNativeLayer::calculateFrameRect(const MCRectangle &p_rect, const MCRectangle &p_parent_rect, const MCGAffineTransform p_stack_transform)
{
   // IM-2017-04-20: [[ Bug 19327 ]] Transform rect to ui view coords
  MCRectangle t_view_rect;
  t_view_rect = MCRectangleGetTransformedBounds(p_rect, p_stack_transform);
  
   MCRectangle t_parent_view_rect;
   t_parent_view_rect = MCRectangleGetTransformedBounds(p_parent_rect, p_stack_transform);
   
   // First compute rect of this object relative to parent
   t_view_rect.y -= t_parent_view_rect.y;
   t_view_rect.x -= t_parent_view_rect.x;
   
   // Now compute the (y-inverted) NSView rect
  NSRect t_rect;
  t_rect = NSMakeRect(t_view_rect.x, t_parent_view_rect.height - (t_view_rect.y + t_view_rect.height), t_view_rect.width, t_view_rect.height);
    
    return t_rect;
}

void MCMacPlatformNativeLayer::SetGeometry(const MCRectangle &p_rect, const MCRectangle &p_parent_rect, const MCGAffineTransform p_stack_transform)
{
    [m_view setFrame:calculateFrameRect(p_rect, p_parent_rect, p_stack_transform)];
    [m_view setNeedsDisplay:YES];
    [m_cached release];
    m_cached = nil;
}

void MCMacPlatformNativeLayer::SetVisible(bool p_visible)
{
    [m_view setHidden:!p_visible];
}

void MCMacPlatformNativeLayer::Relayer(void *p_container_view, void *p_view_above)
{
    NSView * t_parent_view;
    
    if (p_container_view == nil && m_window != nil)
    {
        t_parent_view = [static_cast<MCMacPlatformWindow*>(m_window) -> GetHandle() contentView];
    }
    else
        t_parent_view = (NSView*)p_container_view;
    
    // Insert the widget in the correct place (but only if the card is current)
    if (m_window != nil)
    {
        [m_view removeFromSuperview];
        if (p_view_above != nil)
        {
            [t_parent_view addSubview:m_view positioned:NSWindowBelow relativeTo:(NSView*)p_view_above];
        }
        else
        {
            // This is the top-most native layer
            [t_parent_view addSubview:m_view];
        }
        [t_parent_view setNeedsDisplay:YES];
    }
}

bool MCMacPlatformNativeLayer::GetNativeView(void *&r_view)
{
    r_view = m_view;
    return true;
}

void MCMacPlatformNativeLayer::SetNativeView(void *p_view)
{
    if (m_view != nil)
        [m_view release];
    
    m_view = (NSView*)p_view;
    [m_view retain];
}

////////////////////////////////////////////////////////////////////////////////

bool MCMacPlatformCore::CreateNativeContainer(void *&r_view)
{
    NSView *t_view;
    t_view = [[[ MCContainerView alloc] init] autorelease];
    
    if (t_view == nil)
        return false;
    
    [t_view setAutoresizesSubviews:NO];
    r_view = t_view;
    
    return true;
}

void MCMacPlatformCore::ReleaseNativeView(void *p_view)
{
    if (p_view == nil)
        return;
    
    [(NSView*)p_view release];
}

////////////////////////////////////////////////////////////////////////////////

MCPlatformNativeLayerRef MCMacPlatformCore::CreateNativeLayer()
{
    MCPlatform::Ref<MCPlatformNativeLayer> t_ref = MCPlatform::makeRef<MCMacPlatformNativeLayer>(this);
    
    return t_ref.unsafeTake();
}

////////////////////////////////////////////////////////////////////////////////

