/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "player.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "mcerror.h"
#include "util.h"
#include "param.h"
#include "execpt.h"
#include "debug.h"
#include "globals.h"
#include "mode.h"
#include "image.h"
#include "redraw.h"
#include "license.h"
#include "context.h"
#include "region.h"
#include "eventqueue.h"

#include "graphicscontext.h"
#include "resolution.h"

#include "osxdc.h"

#include <Cocoa/Cocoa.h>

#include "osxdc-stack.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-13: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   HIView update.
// IM-2013-06-19: [[ RefactorGraphics ]] Now using callback function to update
//   the HIView instead of a Pixmap
static MCStackUpdateCallback s_update_callback = nil;
static void *s_update_context = nil;

extern bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGRasterToCGImage(const MCGRaster &p_raster, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);

////////////////////////////////////////////////////////////////////////////////

MCPoint NSPointToMCPointGlobal(NSPoint p)
{
	MCPoint r;
	r . x = p . x;
	r . y = 1440 - p . y;
	return r;
}

NSPoint NSPointFromMCPointGlobal(MCPoint p)
{
	NSPoint r;
	r . x = p . x;
	r . y = 1440 - p . y;
	return r;
}

MCPoint NSPointToMCPointLocal(NSView *view, NSPoint p)
{
	MCPoint r;
	r . x = p . x;
	r . y = [view bounds] . size . height - p . y;
	return r;
}

NSPoint NSPointFromMCPointLocal(NSView *view, MCPoint p)
{
	NSPoint r;
	r . x = p . x;
	r . y = [view bounds] . size . height - p . y;
	return r;
}

NSRect NSRectFromMCRectangleLocal(NSView *view, MCRectangle r)
{
	return NSMakeRect(r . x, [view bounds] . size . height - (r . y + r . height), r . width, r . height);
}

MCRectangle NSRectToMCRectangleLocal(NSView *view, NSRect r)
{
	return MCU_make_rect(r . origin . x, [view bounds] . size . height - (r . origin . y + r . size . height), r . size . width, r . size . height);
}

NSRect NSRectFromMCRectangleGlobal(MCRectangle r)
{
	return NSMakeRect(r . x, 1440 - (r . y + r . height), r . width, r . height);
}

MCRectangle NSRectToMCRectangleGlobal(NSRect r)
{
	return MCU_make_rect(r . origin . x, 1440 - (r . origin . y + r . size . height), r . size . width, r . size . height);
}

////////////////////////////////////////////////////////////////////////////////

static inline CGRect MCGRectangleToCGRect(MCGRectangle p_rect)
{
	CGRect t_rect;
	t_rect.origin.x = p_rect.origin.x;
	t_rect.origin.y = p_rect.origin.y;
	t_rect.size.width = p_rect.size.width;
	t_rect.size.height = p_rect.size.height;
	
	return t_rect;
}

static inline CGBlendMode MCGBlendModeToCGBlendMode(MCGBlendMode p_blend)
{
	// MM-2013-08-28: [[ RefactorGraphics ]] Tweak for 10.4 SDK support.
	switch (p_blend)
	{
		case kMCGBlendModeClear:
			return (CGBlendMode) 16; //kCGBlendModeClear;
		case kMCGBlendModeCopy:
			return (CGBlendMode) 17; //kCGBlendModeCopy;
		case kMCGBlendModeSourceIn:
			return (CGBlendMode) 18; //kCGBlendModeSourceIn;
		case kMCGBlendModeSourceOut:
			return (CGBlendMode) 19; //kCGBlendModeSourceOut;
		case kMCGBlendModeSourceAtop:
			return (CGBlendMode) 20; //kCGBlendModeSourceAtop;
		case kMCGBlendModeDestinationOver:
			return (CGBlendMode) 21; //kCGBlendModeDestinationOver;
		case kMCGBlendModeDestinationIn:
			return (CGBlendMode) 22; //kCGBlendModeDestinationIn;
		case kMCGBlendModeDestinationOut:
			return (CGBlendMode) 23; //kCGBlendModeDestinationOut;
		case kMCGBlendModeDestinationAtop:
			return (CGBlendMode) 24; //kCGBlendModeDestinationAtop;
		case kMCGBlendModeXor:
			return (CGBlendMode) 25; //kCGBlendModeXOR;
		case kMCGBlendModePlusDarker:
			return (CGBlendMode) 26; //kCGBlendModePlusDarker;
		case kMCGBlendModePlusLighter:
			return (CGBlendMode) 27; //kCGBlendModePlusLighter;
		case kMCGBlendModeSourceOver:
			return kCGBlendModeNormal;
		case kMCGBlendModeMultiply:
			return kCGBlendModeMultiply;
		case kMCGBlendModeScreen:
			return kCGBlendModeScreen;
		case kMCGBlendModeOverlay:
			return kCGBlendModeOverlay;
		case kMCGBlendModeDarken:
			return kCGBlendModeDarken;
		case kMCGBlendModeLighten:
			return kCGBlendModeLighten;
		case kMCGBlendModeColorDodge:
			return kCGBlendModeColorDodge;
		case kMCGBlendModeColorBurn:
			return kCGBlendModeColorBurn;
		case kMCGBlendModeSoftLight:
			return kCGBlendModeSoftLight;
		case kMCGBlendModeHardLight:
			return kCGBlendModeHardLight;
		case kMCGBlendModeDifference:
			return kCGBlendModeDifference;
		case kMCGBlendModeExclusion:
			return kCGBlendModeExclusion;
		case kMCGBlendModeHue:
			return kCGBlendModeHue;
		case kMCGBlendModeSaturation:
			return kCGBlendModeSaturation;
		case kMCGBlendModeColor:
			return kCGBlendModeColor;
		case kMCGBlendModeLuminosity:
			return kCGBlendModeLuminosity;
	}
	
	MCAssert(false); // unknown blend mode
}

/* UNCHECKED */
static void MCMacRenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	
	t_success = MCGImageToCGImage(p_src, p_src_rect, false, false, t_image);
	if (t_success)
	{
		CGContextSaveGState(p_target);
		
		CGContextClipToRect(p_target, p_dst_rect);
		CGContextSetAlpha(p_target, p_alpha);
		CGContextSetBlendMode(p_target, MCGBlendModeToCGBlendMode(p_blend));
		CGContextDrawImage(p_target, p_dst_rect, t_image);
		
		CGContextRestoreGState(p_target);
		CGImageRelease(t_image);
	}
}

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, const void *p_bits, uint32_t p_stride, bool p_has_alpha)
{
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	if (t_colorspace != nil)
	{
		MCGRaster t_raster;
		t_raster.width = p_area.size.width;
		t_raster.height = p_area.size.height;
		t_raster.pixels = const_cast<void*>(p_bits);
		t_raster.stride = p_stride;
		t_raster.format = p_has_alpha ? kMCGRasterFormat_ARGB : kMCGRasterFormat_xRGB;
		
		CGImageRef t_image;
		t_image = nil;
		
		if (MCGRasterToCGImage(t_raster, MCGRectangleMake(0, 0, t_raster.width, t_raster.height), t_colorspace, false, false, t_image))
		{
			CGContextClipToRect((CGContextRef)p_target, p_area);
			CGContextDrawImage((CGContextRef)p_target, p_area, t_image);
			CGImageRelease(t_image);
		}
		
		CGColorSpaceRelease(t_colorspace);
	}
}

class MCMacStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCRegionRef m_region;
	CGContextRef m_context;
	
	int32_t m_surface_height;
	
	MCRectangle m_locked_area;
	MCGContextRef m_locked_context;
	void *m_locked_bits;
	uint32_t m_locked_stride;
	
public:
	MCMacStackSurface(MCStack *p_stack, int32_t p_surface_height, MCRegionRef p_region, CGContextRef p_context)
	{
		m_stack = p_stack;
		m_surface_height = p_surface_height;
		m_region = p_region;
		m_context = p_context;
		
		m_locked_context = nil;
		m_locked_bits = nil;
	}
	
	bool Lock(void)
	{
		CGImageRef t_mask;
		t_mask = nil;
		if (m_stack -> getwindowshape() != nil)
			t_mask = (CGImageRef)m_stack -> getwindowshape() -> handle;
		
		if (t_mask != nil)
		{
			MCRectangle t_rect;
			t_rect = MCRegionGetBoundingBox(m_region);
			CGContextClearRect(m_context, CGRectMake(t_rect . x, m_surface_height - (t_rect . y + t_rect . height), t_rect . width, t_rect . height));
			
			// IM-2013-08-29: [[ ResIndependence ]] scale mask to device coords
			MCGFloat t_scale;
			t_scale = MCResGetDeviceScale();
			
			MCGFloat t_mask_height, t_mask_width;
			t_mask_width = CGImageGetWidth(t_mask) * t_scale;
			t_mask_height = CGImageGetHeight(t_mask) * t_scale;
			
			CGRect t_dst_rect;
			t_dst_rect . origin . x = 0;
			t_dst_rect . origin . y = m_surface_height - t_mask_height;
			t_dst_rect . size . width = t_mask_width;
			t_dst_rect . size . height = t_mask_height;
			CGContextClipToMask(m_context, t_dst_rect, t_mask);
		}
		
		CGContextSaveGState(m_context);
		return true;
	}
	
	void Unlock(void)
	{
		CGContextRestoreGState(m_context);
	}
	
	bool LockGraphics(MCRegionRef p_area, MCGContextRef& r_context)
	{
		MCGRaster t_raster;
		if (LockPixels(p_area, t_raster))
		{
			if (MCGContextCreateWithRaster(t_raster, m_locked_context))
			{
				// Set origin
				MCGContextTranslateCTM(m_locked_context, -m_locked_area.x, -m_locked_area.y);
				// Set clipping rect
				MCGContextClipToRect(m_locked_context, MCRectangleToMCGRectangle(m_locked_area));
				
				r_context = m_locked_context;
				
				return true;
			}
			
			UnlockPixels(false);
		}
		
		return false;
	}
	
	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;
		
		MCGContextRelease(m_locked_context);
		m_locked_context = nil;
		
		UnlockPixels(true);
	}
	
	bool LockPixels(MCRegionRef p_area, MCGRaster &r_raster)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), MCRegionGetBoundingBox(m_region));
		
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_stride = t_actual_area . width * sizeof(uint32_t);
		m_locked_bits = malloc(t_actual_area . height * m_locked_stride);
		if (m_locked_bits != nil)
		{
			m_locked_area = t_actual_area;
			
			r_raster . width = t_actual_area . width;
			r_raster . height = t_actual_area . height;
			r_raster . stride = m_locked_stride;
			r_raster . pixels = m_locked_bits;
			r_raster . format = kMCGRasterFormat_ARGB;
			return true;
		}
		
		return false;
	}
	
	void UnlockPixels()
	{
		UnlockPixels(true);
	}
	
	void UnlockPixels(bool p_update)
	{
		if (m_locked_bits == nil)
			return;
		
		if (p_update)
			FlushBits(m_locked_bits, m_locked_area . width * sizeof(uint32_t));
		
		free(m_locked_bits);
		m_locked_bits = nil;
	}
	
	void FlushBits(void *p_bits, uint32_t p_stride)
	{
		void *t_target;
		if (!LockTarget(kMCStackSurfaceTargetCoreGraphics, t_target))
			return;
		
		CGRect t_dst_rect;
		t_dst_rect = CGRectMake(m_locked_area . x, m_surface_height - (m_locked_area . y + m_locked_area . height), m_locked_area . width, m_locked_area . height);
		
		MCMacRenderBitsToCG(m_context, t_dst_rect, p_bits, p_stride, false);
		
		UnlockTarget();
	}
	
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		if (p_type != kMCStackSurfaceTargetCoreGraphics)
			return false;
		
		CGContextSaveGState(m_context);
		
		r_context = m_context;
		
		return true;
	}
	
	void UnlockTarget(void)
	{
		CGContextRestoreGState(m_context);
	}
	
	bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		// IM-2013-08-21: [[ RefactorGraphics]] Rework to fix positioning of composited src image
		// compute transform from src rect to dst rect
		MCGFloat t_sx, t_sy, t_dx, t_dy;
		t_sx = p_dst_rect.size.width / p_src_rect.size.width;
		t_sy = p_dst_rect.size.height / p_src_rect.size.height;
		
		t_dx = p_dst_rect.origin.x - (p_src_rect.origin.x * t_sx);
		t_dy = p_dst_rect.origin.y - (p_src_rect.origin.y * t_sy);
		
		// apply transformation to rect (0, 0, image width, image height)
		MCGRectangle t_dst_rect, t_src_rect;
		t_src_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(p_src), MCGImageGetHeight(p_src));
		t_dst_rect = MCGRectangleMake(t_dx, t_dy, t_src_rect.size.width * t_sx, t_src_rect.size.height * t_sy);
		
		CGContextRef t_context = nil;
		if (!LockTarget(kMCStackSurfaceTargetCoreGraphics, (void*&)t_context))
			return false;
		
		// clip to dst rect
		MCRectangle t_bounds;
		t_bounds = MCGRectangleGetIntegerBounds(p_dst_rect);
		CGRect t_dst_clip;
		t_dst_clip = CGRectMake(t_bounds . x, m_surface_height - (t_bounds . y + t_bounds . height), t_bounds . width, t_bounds . height);
		CGContextClipToRect(t_context, t_dst_clip);
		
		// render image to transformed rect
		CGRect t_dst_cgrect;
		t_dst_cgrect = CGRectMake(t_dst_rect . origin . x, m_surface_height - (t_dst_rect . origin . y + t_dst_rect . size . height), t_dst_rect . size . width, t_dst_rect . size . height);
		MCMacRenderImageToCG(t_context, t_dst_cgrect, p_src, t_src_rect, p_alpha, p_blend);
		
		UnlockTarget();
		
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCStackWindowDelegate

- (id)init
{
	m_stack = nil;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)setStack: (MCStack *)stack
{
	m_stack = stack;
}

- (MCStack *)stack
{
	return m_stack;
}

- (NSSize)windowWillResize: (NSWindow *)window toSize: (NSSize)frameSize
{
/*	if (m_stack == nil)
		return frameSize;
	
	NSRect t_frame;
	t_frame = [window frame];
	t_frame . size = frameSize;
	
	NSRect t_content;
	t_content = [window contentRectForFrameRect: t_frame];
	
	MCRectangle t_rect;
	t_rect = m_stack -> getrect();
	t_rect . width = t_content . size . width;
	t_rect . height = t_content . size . height;
	
	m_stack -> view_configure_with_rect(true, t_rect);*/
	return frameSize;
}

- (void)windowDidMove: (NSNotification *)notification
{
	if (m_stack == nil)
		return;
	/* UNCHECKED */ MCEventQueuePostWindowReshape(m_stack);
}

- (void)windowDidMiniaturize: (NSNotification *)notification
{
}

- (void)windowDidDeminiaturize: (NSNotification *)notification
{
}

- (void)windowShouldClose: (id)sender
{
}

- (void)windowDidBecomeKey: (NSNotification *)notification
{
	if (m_stack == nil)
		return;
	
	/* UNCHECKED */ MCEventQueuePostKeyFocus(m_stack, true);
}

- (void)windowDidResignKey: (NSNotification *)notification
{
	if (m_stack == nil)
		return;
	
	/* UNCHECKED */ MCEventQueuePostKeyFocus(m_stack, false);
}

@end

@implementation com_runrev_livecode_MCStackView

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame: frameRect];
	if (self == nil)
		return nil;
	
	m_stack = nil;
	m_tracking_area = nil;
	m_use_input_method = false;
	
	return self;
}

- (void)dealloc
{
	[m_tracking_area release];
	[super dealloc];
}

- (void)setStack: (MCStack *)stack
{
	m_stack = stack;
}

- (MCStack *)stack
{
	return m_stack;
}

- (void)setUseInputMethod: (BOOL)useInputMethod
{
	m_use_input_method = (useInputMethod == YES);
}

- (BOOL)useInputMethod
{
	return m_use_input_method;
}

//////////

- (void)updateTrackingAreas
{
	[super updateTrackingAreas];
	
	// COCOA-TODO: Make sure this is necessary, apparantly things should
	//   automagically resize with InVisibleRect.
	
	if (m_tracking_area != nil)
	{
		[self removeTrackingArea: m_tracking_area];
		[m_tracking_area release];
		m_tracking_area = nil;
	}
	
	m_tracking_area = [[NSTrackingArea alloc] initWithRect: [self bounds]
												   options: (NSTrackingMouseEnteredAndExited | 
															 NSTrackingMouseMoved | 
															 NSTrackingActiveAlways | 
															 NSTrackingInVisibleRect | 
															 NSTrackingEnabledDuringMouseDrag)
													 owner: self
												  userInfo: nil];
	[self addTrackingArea: m_tracking_area];
}

- (BOOL)isFlipped
{
	return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

- (void)mouseDown: (NSEvent *)event
{
	[self handleMousePress: event pressed: YES];
}

- (void)mouseUp: (NSEvent *)event
{
	[self handleMousePress: event pressed: NO];
}

- (void)mouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)mouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)rightMouseDown: (NSEvent *)event
{
	[self handleMousePress: event pressed: YES];
}

- (void)rightMouseUp: (NSEvent *)event
{
	[self handleMousePress: event pressed: NO];
}

- (void)rightMouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)rightMouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)otherMouseDown: (NSEvent *)event
{
	[self handleMousePress: event pressed: YES];
}

- (void)otherMouseUp: (NSEvent *)event
{
	[self handleMousePress: event pressed: NO];
}

- (void)otherMouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)otherMouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event forceOutside: NO];
}

- (void)mouseEntered: (NSEvent *)event
{
	[self handleMouseFocus: event inside: YES];
}

- (void)mouseExited: (NSEvent *)event
{
	[self handleMouseFocus: event inside: NO];
}

- (void)flagsChanged: (NSEvent *)event
{
	[self handleFlagsChanged: event];
}

- (void)keyDown: (NSEvent *)event
{
	if (m_use_input_method)
	{
		if ([[self inputContext] handleEvent: event])
			return;
	}
	
	[self handleKeyPress: event pressed: YES];
}

- (void)keyUp: (NSEvent *)event
{
#if NOT_USED
	if (m_use_input_method)
	{
		if ([[self inputContext] handleEvent: event])
			return;
	}
	
	[self handleKeyPress: event pressed: NO];
#endif
}

- (void)setFrameSize: (NSSize)size
{
	[super setFrameSize: size];
	
	if (m_stack == nil)
		return;
	
	MCRectangle t_rect;
	t_rect = m_stack -> getrect();
	t_rect . width = size . width;
	t_rect . height = size . height;
	
	// COCOA-TODO: Only dispatch if we are not in a blocking wait
	m_stack -> view_configure_with_rect(true, t_rect);
	
	if ([[self window] inLiveResize])
		[NSApp stop: self];
}

// COCOA-TODO: Improve dirty rect calc
- (void)drawRect: (NSRect)dirtyRect
{
	if (m_stack == nil)
		return;
	
	CGContextRef t_graphics;
	t_graphics = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	
	MCRegionRef t_dirty_rgn;
	MCRegionCreate(t_dirty_rgn);
	MCRegionSetRect(t_dirty_rgn, NSRectToMCRectangleLocal(self, dirtyRect));
	
	// IM-2013-08-23: [[ ResIndependence ]] provide surface height in device scale
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	// IM-2013-10-10: [[ FullscreenMode ]] Use height of view instead of card
	int32_t t_surface_height;
	t_surface_height = floor(m_stack->view_getrect().height * t_scale);

	// Save the context state
	CGContextSaveGState(t_graphics);
	
	MCMacStackSurface t_surface(m_stack, t_surface_height, (MCRegionRef)t_dirty_rgn, t_graphics);
	
	if (t_surface.Lock())
	{
		// If we don't have an update pixmap, then use redrawwindow.
		if (s_update_callback == nil)
			m_stack -> device_redrawwindow(&t_surface, (MCRegionRef)t_dirty_rgn);
		else
			s_update_callback(&t_surface, (MCRegionRef)t_dirty_rgn, s_update_context);
		t_surface.Unlock();
	}
	
	// Restore the context state
	CGContextRestoreGState(t_graphics);
}

//////////

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
	MCLog("insertText('', (%d, %d))", replacementRange . location, replacementRange . length);
	if (replacementRange . location == NSNotFound)
	{
		NSRange t_marked_range;
		t_marked_range = [self markedRange];
		if (t_marked_range . location != NSNotFound)
			replacementRange = t_marked_range;
		else
			replacementRange = [self selectedRange];
	}
	
	int32_t si, ei;
	si = 0;
	ei = INT32_MAX;
	MCactivefield -> resolvechars(0, si, ei, replacementRange . location, replacementRange . length);
	
	NSString *t_string;
	if ([aString isKindOfClass: [NSAttributedString class]])
		t_string = [aString string];
	else
		t_string = aString;
	
	NSUInteger t_length;
	t_length = [t_string length];
	
	unichar *t_chars;
	t_chars = new unichar[t_length];
	
	[t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];
	
	MCactivefield -> settextindex(0,
								  si,
								  ei,
								  MCString((char *)t_chars, t_length * 2),
								  True,
								  true);
    
	delete t_chars;
	
	[self unmarkText];
	[[self inputContext] invalidateCharacterCoordinates];
}

- (void)doCommandBySelector:(SEL)aSelector
{
	MCLog("doCommandBySelector:", 0);
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)newSelection replacementRange:(NSRange)replacementRange
{
	MCLog("setMarkedText('', (%d, %d), (%d, %d)", newSelection . location, newSelection . length, replacementRange . location, replacementRange . length);
	
	/*if (replacementRange . location == NSNotFound)
	{
		NSRange t_marked_range;
		t_marked_range = [self markedRange];
		if (t_marked_range . location != NSNotFound)
			replacementRange = t_marked_range;
		else
			replacementRange = [self selectedRange];
	}
	
	int32_t si, ei;
	si = ei = 0;
	MCactivefield -> resolvechars(0, si, ei, replacementRange . location, replacementRange . length);
	
	NSString *t_string;
	if ([aString isKindOfClass: [NSAttributedString class]])
		t_string = [aString string];
	else
		t_string = aString;
	
	NSUInteger t_length;
	t_length = [t_string length];
	
	unichar *t_chars;
	t_chars = new unichar[t_length];
	
	[t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];
	
	MCactivefield -> stopcomposition(True, False);
	
	if ([t_string length] == 0)
		[self unmarkText];
	else
	{
		MCactivefield -> startcomposition();
		MCactivefield -> finsertnew(FT_IMEINSERT, MCString((char *)t_chars, t_length * 2), 0, true);
	}
	
	MCactivefield -> seltext(replacementRange . location + newSelection . location,
							 replacementRange . location + newSelection . location + newSelection . length,
							 False);
	
	[[self inputContext] invalidateCharacterCoordinates];*/
}

- (void)unmarkText
{
	MCLog("unmarkText", 0);
	MCactivefield -> stopcomposition(False, False);
	[[self inputContext] discardMarkedText];
}

- (NSRange)selectedRange
{
	if (MCactivefield == nil)
		return NSMakeRange(NSNotFound, 0);
	
	int4 si, ei;
	MCactivefield -> selectedmark(False, si, ei, False, False);
	MCLog("selectedRange() = (%d, %d)", si, ei - si);
	return NSMakeRange(si, ei - si);
}

- (NSRange)markedRange
{
	int4 si, ei;
	if (MCactivefield -> getcompositionrange(si, ei))
		return NSMakeRange(si, ei - si);
	return NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText
{
	MCLog("hasMarkedText", 0);
	return [self markedRange] . location != NSNotFound;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	MCLog("attributedSubstringForProposedRange(%d, %d -> %d, %d) = '%s'", aRange . location, aRange . length, 0, 0, "");
	
	
	
	return NULL;
}

- (NSArray*)validAttributesForMarkedText
{
	MCLog("validAttributesForMarkedText() = []", nil);
	return [NSArray array];
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	int32_t t_si, t_ei;
	t_si = aRange . location;
	t_ei = aRange . location + aRange . length;
	
	MCRectangle t_rect;
	t_rect = MCactivefield -> firstRectForCharacterRange(t_si, t_ei);
	
	if (actualRange != nil)
		*actualRange = NSMakeRange(t_si, t_ei - t_si);
	
	MCLog("firstRectForCharacterRange(%d, %d -> %d, %d) = %d, %d, %d, %d", aRange . location, aRange . length, t_si, t_ei - t_si, t_rect . x, t_rect . y, t_rect . width, t_rect . height);
	
	return NSRectFromMCRectangleGlobal([self localRectToGlobal: t_rect]);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
	MCPoint t_location;
	t_location = [self localToGlobal: NSPointToMCPointGlobal(aPoint)];
	
	int32_t si, ei;
	
	if (!MCactivefield -> locmarkpoint(t_location, False, False, False, False, si, ei))
		si = 0;
	
	MCLog("characterIndexForPoint(%d, %d) = %d", t_location . x, t_location . y, si);
	
	return si;
}

//////////

- (void)handleMousePress: (NSEvent *)event pressed: (BOOL)pressed
{
	[self handleMouseMove: event forceOutside: NO];
	((MCScreenDC *)MCscreen) -> event_mousepress([event buttonNumber], pressed == YES);
}

- (void)handleMouseMove: (NSEvent *)event forceOutside: (BOOL)forceOutside
{
	NSPoint t_location;
	t_location = [event locationInWindow];
	if ([event window] != nil)
		t_location = [[event window] convertBaseToScreen: t_location];
	((MCScreenDC *)MCscreen) -> event_mousemove(NSPointToMCPointGlobal(t_location));
}

- (void)handleMouseFocus: (NSEvent *)event inside: (BOOL)inside
{
	[self handleMouseMove: event forceOutside: inside == NO];
}

- (void)handleFlagsChanged: (NSEvent *)event
{
	((MCScreenDC *)MCscreen) -> event_modifierschanged([event modifierFlags]);
}

- (void)handleKeyPress: (NSEvent *)event pressed: (BOOL)pressed
{
	if (!pressed)
		return;
	
	if (m_stack == nil)
		return;
	
	uint32_t t_char_code, t_key_code;
	t_key_code = [event keyCode];
	if ([[event characters] length] >= 1)
		t_char_code = [[event characters] characterAtIndex: 0];
	
	/* UNCHECKED */ MCEventQueuePostKeyPress(m_stack, 0, t_char_code, t_key_code);
}

@end

@implementation NSView (com_runrev_livecode_NSViewAdditions)

- (MCPoint)localToGlobal: (MCPoint)location
{
	return NSPointToMCPointGlobal([[self window] convertBaseToScreen: [self convertPoint: NSPointFromMCPointLocal(self, location) toView: nil]]);
}

- (MCPoint)globalToLocal: (MCPoint)location
{
	return NSPointToMCPointLocal(self, [self convertPoint: [[self window] convertScreenToBase: NSPointFromMCPointGlobal(location)] fromView: nil]);
}

- (MCRectangle)localRectToGlobal: (MCRectangle)rect
{
	NSRect t_win_rect;
	t_win_rect = [self convertRect: NSRectFromMCRectangleLocal(self, rect) toView: nil];
	
	NSRect t_screen_rect;
	t_screen_rect . origin = [[self window] convertBaseToScreen: t_win_rect . origin];
	t_screen_rect . size = t_win_rect . size;
	
	return NSRectToMCRectangleGlobal(t_screen_rect);
}

- (MCRectangle)globalRectToLocal: (MCRectangle)rect
{
	NSRect t_screen_rect;
	t_screen_rect = NSRectFromMCRectangleGlobal(rect);
	
	NSRect t_win_rect;
	t_win_rect . origin = [[self window] convertScreenToBase: t_screen_rect . origin];
	t_win_rect . size = t_screen_rect . size;
	return NSRectToMCRectangleLocal(self, [self convertRect: t_win_rect fromView: nil]);
}

- (MCStack *)stack
{
	return nil;
}

@end

////////////////////////////////////////////////////////////////////////////////

// We have:
//   topLevel
//   topLevelLocked
//   modeless
//   closed
//			=> document
//   palette => floating
//
//   modal => modal
//
//   drawer => drawer
//
//   sheet => sheet
//
// decorations & UTILITY => system
// decorations & CLOSE
// decorations & METAL
// decorations & NOSHADOW
// decorations & MINIMIZE
// decorations & MAXIMIZE
// decorations & LIVERESIZING
// decorations & RESIZABLE

// TITLE
// MENU

void MCStack::realize(void)
{
	if (!MCnoui)
	{
		// Sort out fullscreen mode
		
		// IM-2013-08-01: [[ ResIndependence ]] scale stack rect to device coords
		MCRectangle t_device_rect;
		t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(rect));
	
		// Sort out name ?
		
		loadwindowshape();
		
		// Compute the level of the window
		int32_t t_window_level;
		if (getflag(F_DECORATIONS) && (decorations & WD_UTILITY) != 0)
			t_window_level = kCGUtilityWindowLevelKey;
		else if (mode == WM_PALETTE)
			t_window_level = kCGFloatingWindowLevelKey;
		else if (mode == WM_MODAL || mode == WM_SHEET)
			t_window_level = kCGModalPanelWindowLevelKey;
		else if (mode == WM_PULLDOWN || mode == WM_OPTION || mode == WM_COMBO)
			t_window_level = kCGPopUpMenuWindowLevelKey;
		else if (mode == WM_CASCADE)
			t_window_level = kCGPopUpMenuWindowLevelKey;
		else if (mode == WM_TOOLTIP)
			t_window_level = kCGStatusWindowLevelKey;
		else if (mode == WM_SHEET)
			; // COCOA-TODO
		else if (mode == WM_DRAWER)
			; // COCOA-TODO
		else
			t_window_level = kCGNormalWindowLevelKey;
		
		bool t_has_titlebox, t_has_closebox, t_has_collapsebox, t_has_zoombox, t_has_sizebox;
		if (getflag(F_DECORATIONS))
		{
			t_has_titlebox = (decorations & WD_TITLE) != 0;
			t_has_closebox = (decorations & WD_CLOSE) != 0;
			t_has_collapsebox = (decorations & WD_MINIMIZE) != 0;
			t_has_zoombox = (decorations & WD_MAXIMIZE) == 0;
			t_has_sizebox = getflag(F_RESIZABLE);
		}
		else
		{
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
			if (t_window_level == kCGNormalWindowLevelKey)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_zoombox = true;
				t_has_collapsebox = true;
				t_has_sizebox = true;
			}
			else if (t_window_level == kCGFloatingWindowLevelKey ||
					 t_window_level == kCGUtilityWindowLevelKey)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_collapsebox = true;
			}
		}
		
		if (getflag(F_DECORATIONS) && ((decorations & (WD_TITLE | WD_MENU | WD_CLOSE | WD_MINIMIZE | WD_MAXIMIZE)) == 0))
			t_has_sizebox = false;
		
		// If the window has a windowshape, we don't have any decorations.
		if (m_window_shape != nil)
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
		
		// If the window is not normal, utility or floating we don't have close or zoom boxes.
		if (t_window_level != kCGNormalWindowLevelKey &&
			t_window_level != kCGUtilityWindowLevelKey &&
			t_window_level != kCGFloatingWindowLevelKey)
		{
			t_has_closebox = false;
			t_has_zoombox = false;
		}
		
		// If the window is not normal level, we don't have a collapse box.
		if (t_window_level != kCGNormalWindowLevelKey)
			t_has_collapsebox = false;
		
		// If the window is not one that would be expected to be resizable, don't give it
		// a size box.
		if (t_window_level != kCGNormalWindowLevelKey &&
			t_window_level != kCGFloatingWindowLevelKey &&
			t_window_level != kCGUtilityWindowLevelKey &&
			t_window_level != kCGModalPanelWindowLevelKey)
			/*t_window_level != kCGSheet / Drawer*/
			t_has_sizebox = false;
		
		// Compute the style of the window
		NSUInteger t_window_style;
		t_window_style = NSBorderlessWindowMask;
		if (t_has_titlebox)
			t_window_style |= NSTitledWindowMask;
		if (t_has_closebox)
			t_window_style |= NSClosableWindowMask;
		if (t_has_collapsebox)
			t_window_style |= NSMiniaturizableWindowMask;
		if (t_has_sizebox)
			t_window_style |= NSResizableWindowMask;
		if (t_window_level == kCGFloatingWindowLevelKey)
			t_window_style |= NSUtilityWindowMask;
		
		NSRect t_rect;
		t_rect = NSRectFromMCRectangleGlobal(t_device_rect);

		NSWindow *t_window;
		if (t_window_level != kCGFloatingWindowLevelKey)
			t_window = [[NSWindow alloc] initWithContentRect: t_rect styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
		else
			t_window = [[NSPanel alloc] initWithContentRect: t_rect styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
		
		com_runrev_livecode_MCStackWindowDelegate *t_delegate;
		t_delegate = [[com_runrev_livecode_MCStackWindowDelegate alloc] init];
		[t_window setDelegate: t_delegate];
		
		// Set the stack window
		window = (MCSysWindowHandle)t_window;
	
		// Create the content view
		[t_window setContentView: [[[com_runrev_livecode_MCStackView alloc] initWithFrame: NSZeroRect] autorelease]];
		
		// Configure properties of the window now its been created.
		if (m_window_shape != nil)
			[t_window setOpaque: NO];
		
		if ((decorations & WD_NOSHADOW) != 0)
			[t_window setHasShadow: NO];
		
		if ((decorations & WD_LIVERESIZING) != 0)
			; // COCOA-TODO
		
		if (t_has_zoombox)
			[[t_window standardWindowButton: NSWindowZoomButton] setEnabled: NO];
		
		[t_window setLevel: t_window_level];
		
		setopacity(blendlevel * 255 / 100);
		
		// Sort out drawers
		
		updatemodifiedmark();
	}
	
	start_externals();
}

MCRectangle MCStack::device_getwindowrect() const
{
	NSRect t_frame;
	t_frame = [(NSWindow *)window frame];
	return NSRectToMCRectangleGlobal(t_frame);
}

// IM-2013-09-23: [[ FullscreenMode ]] Factor out device-specific window sizing
MCRectangle MCStack::device_setgeom(const MCRectangle &p_rect)
{
	NSRect t_content;
	t_content = [(NSWindow *)window contentRectForFrameRect: [(NSWindow *)window frame]];
	
	MCRectangle t_win_rect;
	t_win_rect = NSRectToMCRectangleGlobal(t_content); //MCU_make_rect(t_content . origin . x, t_content . origin . y, t_content . size . width, t_content . size . height);
	
	/*if ([(NSWindow *)window isVisible])
	{
		if (mode != WM_SHEET && mode != WM_DRAWER &&
			(p_rect.x != t_win_rect.x || p_rect.y != t_win_rect.y))
			[(NSWindow *)window setFrameTopLeftPoint: NSMakePoint(p_rect.x, p_rect.y)];
	}
	else
	{
		if (mode != WM_SHEET && mode != WM_DRAWER)
			[(NSWindow *)window setFrameTopLeftPoint: NSMakePoint(p_rect.x, p_rect.y)];
	}
	
	if (p_rect.width != t_win_rect.width || p_rect.height != t_win_rect.height)
		[(NSWindow *)window setContentSize:	NSMakeSize(p_rect . width, p_rect . height)];*/
	
	if (mode != WM_SHEET && mode != WM_DRAWER &&
		!MCU_equal_rect(p_rect, t_win_rect))
		[(NSWindow *)window setFrame: [(NSWindow *)window frameRectForContentRect: NSRectFromMCRectangleGlobal(p_rect)] display: YES];
}

void MCStack::syncscroll(void)
{
	// COCOA-TODO: Do something with scroll
}

void MCStack::start_externals()
{
	[[(NSWindow *)window contentView] setStack: this];
	[[(NSWindow *)window delegate] setStack: this];
	loadexternals();
}

void MCStack::stop_externals()
{
	Boolean oldlock = MClockmessages;
	MClockmessages = True;
	
	MCPlayer *tptr = MCplayers;
	
	while (tptr != NULL)
	{
		if (tptr->getstack() == this)
		{
			if (tptr->playstop())
				tptr = MCplayers; // was removed, start search over
		}
		else
			tptr = tptr->getnextplayer();
	}
	destroywindowshape();
	
	MClockmessages = oldlock;
	
	unloadexternals();
	
	[[(NSWindow *)window contentView] setStack: nil];
	[[(NSWindow *)window delegate] setStack: nil];
}

void MCStack::setopacity(uint1 p_level)
{
	if (window == nil)
		return;
	
	[(NSWindow *)window setAlphaValue: p_level / 255.0f];
}

void MCStack::updatemodifiedmark(void)
{
	if (window == nil)
		return;
	
	[(NSWindow *)window setDocumentEdited: getextendedstate(ECS_MODIFIED_MARK) == True];
}

// MW-2011-09-11: [[ Redraw ]] Force an immediate update of the window within the given
//   region. The actual rendering is done by deferring to the 'redrawwindow' method.
void MCStack::device_updatewindow(MCRegionRef p_region)
{
	if (window == nil)
		return;
	
	NSView *t_view;
	t_view = [(NSWindow *)window contentView];
	
	if (!getextendedstate(ECS_MASK_CHANGED) || s_update_callback != nil)
		[t_view setNeedsDisplayInRect: NSRectFromMCRectangleLocal(t_view, MCRegionGetBoundingBox(p_region))];
	else
	{
		[t_view setNeedsDisplay: YES];
		NSDisableScreenUpdates();
	}
	
	[t_view display];
	
	if (getextendedstate(ECS_MASK_CHANGED))
	{
		[(NSWindow *)window invalidateShadow];
		
		NSEnableScreenUpdates();
		
		setextendedstate(False, ECS_MASK_CHANGED);
	}
}

void MCStack::device_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	// Set the file-local static to the callback to use (stacksurface picks this up!)
	s_update_callback = p_callback;
	s_update_context = p_context;
	// IM-2013-08-29: [[ RefactorGraphics ]] simplify by calling device_updatewindow, which performs the same actions
	device_updatewindow(p_region);
	// Unset the file-local static.
	s_update_callback = nil;
	s_update_context = nil;
}

////////////////////////////////////////////////////////////////////////////////
