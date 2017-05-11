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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


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

#include "group.h"
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

#include "graphics_util.h"


MCNativeLayerMac::MCNativeLayerMac(MCObject *p_object, NSView *p_view) :
  m_view(p_view),
  m_cached(nil)
{
	m_object = p_object;
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

void MCNativeLayerMac::doAttach()
{
    // Act as if there was a re-layer to put the widget in the right place
    // *** Can we assume open happens in back-to-front order? ***
    doRelayer();
    
    // Restore the visibility state of the widget (in case it changed due to a
    // tool change while on another card - we don't get a message then)
	
	doSetGeometry(m_rect);
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayerMac::doDetach()
{
    // Remove the view from the stack's content view
    [m_view removeFromSuperview];
}

bool MCNativeLayerMac::doPaint(MCGContextRef p_context)
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

NSRect MCNativeLayerMac::calculateFrameRect(const MCRectangle &p_rect)
{
	// IM-2017-04-20: [[ Bug 19327 ]] Transform rect to ui view coords
	MCRectangle t_view_rect;
	t_view_rect = MCRectangleGetTransformedBounds(p_rect, m_object->getstack()->getviewtransform());
	
	int32_t t_gp_height;
	
	NSWindow *t_window = getStackWindow();
	if (t_window != nil)
		t_gp_height = (int32_t)[[t_window contentView] frame].size.height;
	else
		t_gp_height = m_object->getcard()->getrect().height;
	
	NSRect t_rect;
	t_rect = NSMakeRect(t_view_rect.x, t_gp_height - (t_view_rect.y + t_view_rect.height), t_view_rect.width, t_view_rect.height);
	
	return t_rect;
}

void MCNativeLayerMac::doSetViewportGeometry(const MCRectangle &p_rect)
{
}

void MCNativeLayerMac::doSetGeometry(const MCRectangle &p_rect)
{
    [m_view setFrame:calculateFrameRect(p_rect)];
    [m_view setNeedsDisplay:YES];
    [m_cached release];
    m_cached = nil;
}

void MCNativeLayerMac::doSetVisible(bool p_visible)
{
    [m_view setHidden:!p_visible];
}

void MCNativeLayerMac::doRelayer()
{
    // Find which native layer this should be inserted below
    MCObject *t_before;
    t_before = findNextLayerAbove(m_object);
	
	NSView *t_parent_view;
	t_parent_view = nil;
	
	if (!getParentView(t_parent_view))
		return;
	
    // Insert the widget in the correct place (but only if the card is current)
    if (isAttached() && m_object->getcard() == m_object->getstack()->getcurcard())
    {
        [m_view removeFromSuperview];
        if (t_before != nil)
        {
            // There is another native layer above this one
			NSView *t_before_view;
			/* UNCHECKED */ t_before->GetNativeView((void*&)t_before_view);
            [t_parent_view addSubview:m_view positioned:NSWindowBelow relativeTo:t_before_view];
        }
        else
        {
            // This is the top-most native layer
            [t_parent_view addSubview:m_view];
        }
        [t_parent_view setNeedsDisplay:YES];
    }
}

NSWindow* MCNativeLayerMac::getStackWindow()
{
    MCMacPlatformWindow *t_window;
    t_window = (MCMacPlatformWindow*)(m_object->getstack()->getwindow());
    
    if (t_window != nil)
        return t_window -> GetHandle();
    
    return nil;
}

bool MCNativeLayerMac::getParentView(NSView *&r_view)
{
	if (m_object->getparent()->gettype() == CT_GROUP)
	{
		MCNativeLayer *t_container;
		t_container = nil;
		
		if (((MCGroup*)m_object->getparent())->getNativeContainerLayer(t_container))
            return t_container->GetNativeView((void*&)r_view);
	}
	else
	{
        NSWindow* t_window;
        t_window = getStackWindow();
        
        if (t_window != nil)
        {
            r_view = [t_window contentView];
            return true;
        }
	}

    return false;
}

bool MCNativeLayerMac::GetNativeView(void *&r_view)
{
	r_view = m_view;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer* MCNativeLayer::CreateNativeLayer(MCObject *p_object, void *p_view)
{
	if (p_view == nil)
		return nil;
	
    return new MCNativeLayerMac(p_object, (NSView*)p_view);
}

//////////

// IM-2015-12-16: [[ NativeLayer ]] Keep the coordinate system of group contents the same as
//                the top-level window view by keeping its bounds the same as its frame.
//                This allows us to place contents in terms of window coords without having to
//                adjust for the location of the group container.
@interface com_runrev_livecode_MCContainerView: NSView

- (void)setFrameOrigin:(NSPoint)newOrigin;
- (void)setFrameSize:(NSSize)newSize;

@end

@compatibility_alias MCContainerView com_runrev_livecode_MCContainerView;

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

bool MCNativeLayer::CreateNativeContainer(MCObject *p_object, void *&r_view)
{
	NSView *t_view;
	t_view = [[[ MCContainerView alloc] init] autorelease];
	
	if (t_view == nil)
		return false;
	
	[t_view setAutoresizesSubviews:NO];
	r_view = t_view;
	
	return true;
}

//////////

void MCNativeLayer::ReleaseNativeView(void *p_view)
{
	if (p_view == nil)
		return;
	
	[(NSView*)p_view release];
}
