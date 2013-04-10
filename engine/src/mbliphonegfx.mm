/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "stack.h"
#include "mbldc.h"
#include "util.h"
#include "region.h"
#include "redraw.h"
#include "tilecache.h"

#include <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

#include "mbliphonecontext.h"
#include "mbliphoneview.h"

////////////////////////////////////////////////////////////////////////////////

extern UIView *MCIPhoneGetView(void);
extern float MCIPhoneGetResolutionScale(void);

////////////////////////////////////////////////////////////////////////////////

extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);

static void do_update(void *p_dirty)
{
	[MCIPhoneGetDisplayView() renderInRegion: (MCRegionRef)p_dirty];
}

void MCStack::updatewindow(MCRegionRef p_dirty_rgn)
{
	// MW-2011-09-13: [[ Redraw ]] Only perform an update if the window should
	//   draw (i.e. if it is top-most).
	if (getextendedstate(ECS_DONTDRAW))
		return;

	// MW-2012-03-05: [[ ViewStack ]] If this stack is the same as the view's
	//   current stack, then tell it to render.
	if ((MCStack *)[MCIPhoneGetRootView() currentStack] == this)
		MCIPhoneCallOnMainFiber(do_update, p_dirty_rgn);
}

////////////////////////////////////////////////////////////////////////////////

class MCIPhoneStackSurface: public MCStackSurface
{
protected:
	MCRectangle m_region;
	MCRectangle m_locked_area;
	Pixmap m_locked_pixmap;
	MCContext *m_locked_context;
	void *m_locked_bits;
	
	virtual void FlushBits(void *p_bits, uint32_t p_stride) = 0;

public:
	MCIPhoneStackSurface(const MCRectangle& p_area)
	{
		m_region = p_area;
		m_locked_pixmap = nil;
		m_locked_context = nil;
		m_locked_bits = nil;
	}
	
	bool LockGraphics(MCRegionRef p_area, MCContext*& r_context)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), m_region);
		
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_pixmap = MCscreen -> createpixmap(t_actual_area . width, t_actual_area . height, 0, False);
		if (m_locked_pixmap != nil)
		{
			m_locked_context = MCscreen -> createcontext(m_locked_pixmap, False, False);
			if (m_locked_context != nil)
			{
				m_locked_context -> setorigin(t_actual_area . x, t_actual_area . y);
				m_locked_context -> setclip(t_actual_area);
				
				m_locked_area = t_actual_area;
				r_context = m_locked_context;
				
				return true;
			}
			
			MCscreen -> freepixmap(m_locked_pixmap);
		}
		
		return false;
	}
	
	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;
		
		MCscreen -> freecontext(m_locked_context);
		m_locked_context = nil;
		
		void *t_bits;
		uint32_t t_stride;
		MCscreen -> lockpixmap(m_locked_pixmap, t_bits, t_stride);
		FlushBits(t_bits, t_stride);
		MCscreen -> unlockpixmap(m_locked_pixmap, t_bits, t_stride);
		
		MCscreen -> freepixmap(m_locked_pixmap);
	}
	
	bool LockPixels(MCRegionRef p_area, void*& r_bits, uint32_t& r_stride)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), m_region);
		
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_bits = malloc(t_actual_area . width * t_actual_area . height * sizeof(uint32_t));
		if (m_locked_bits != nil)
		{
			m_locked_area = t_actual_area;
			
			r_bits = m_locked_bits;
			r_stride = t_actual_area . width * sizeof(uint32_t);
			return true;
		}
		
		return false;
	}
	
	void UnlockPixels(void)
	{
		if (m_locked_bits == nil)
			return;
		
		FlushBits(m_locked_bits, m_locked_area . width * sizeof(uint32_t));
		
		free(m_locked_bits);
		m_locked_bits = nil;
	}
};

////////////////////////////////////////////////////////////////////////////////

class MCUIKitStackSurface: public MCIPhoneStackSurface
{
	CGContextRef m_context;
	int32_t m_height;
	
public:
	MCUIKitStackSurface(MCRegionRef p_region, CGContextRef p_context, int32_t p_height)
		: MCIPhoneStackSurface(MCRegionGetBoundingBox(p_region))
	{
		m_context = p_context;
		m_height = p_height;
	}
	
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		if (p_type != kMCStackSurfaceTargetCoreGraphics)
			return false;
		
		// MW-2011-10-18: Turn off image interpolation to stop artifacts occuring
		//   when redrawing.
		CGContextSetInterpolationQuality(m_context, kCGInterpolationNone);
		
		CGContextScaleCTM(m_context, 1.0, -1.0);
		CGContextTranslateCTM(m_context, 0.0, -(m_height / MCIPhoneGetResolutionScale()));
		CGContextScaleCTM(m_context, 1.0 / MCIPhoneGetResolutionScale(), 1.0 / MCIPhoneGetResolutionScale());
		CGContextSaveGState(m_context);
		
		r_context = m_context;
		
		return true;
	}
	
	void UnlockTarget(void)
	{
		CGContextRestoreGState(m_context);
	}
	
protected:
	void FlushBits(void *p_bits, uint32_t p_stride)
	{
		void *t_target;
		if (!LockTarget(kMCStackSurfaceTargetCoreGraphics, t_target))
			return;
		
		CGColorSpaceRef t_colorspace;
		t_colorspace = CGColorSpaceCreateDeviceRGB();
		
		CGContextRef t_context;
		t_context = CGBitmapContextCreate(p_bits, m_locked_area . width, m_locked_area . height, 8, p_stride, t_colorspace, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
		CGColorSpaceRelease(t_colorspace);
		
		if (t_context != nil)
		{
			CGImageRef t_image;
			t_image = CGBitmapContextCreateImage(t_context);
			CGContextRelease(t_context);
			
			if (t_image != nil)
			{
				CGContextDrawImage((CGContextRef)t_target, CGRectMake((float)m_locked_area . x, (float)(m_height - (m_locked_area . y + m_locked_area . height)), (float)m_locked_area . width, (float)m_locked_area . height), t_image);
				CGImageRelease(t_image);
			}
		}
		
		UnlockTarget();
	}
};

@implementation MCIPhoneUIKitDisplayView

- (id)initWithFrame:(CGRect)p_frame
{
	return [super initWithFrame: p_frame];
}

- (void)dealloc
{
	[super dealloc];
}

- (void) drawRect: (CGRect)rect
{	
	// MW-2012-03-05: [[ ViewStack ]] Fetch the current stack the view should use,
	//   and return if there is none.
	MCStack *t_stack;
	t_stack = (MCStack *)[[self superview] currentStack];
	if (t_stack == nil)
		return;
    
    MCRectangle t_hull;
    t_hull . x = (int)floor(rect . origin . x * MCIPhoneGetResolutionScale());
    t_hull . y = (int)floor(rect . origin . y * MCIPhoneGetResolutionScale());
    t_hull . width = (int)ceil((rect . origin . x + rect . size . width) * MCIPhoneGetResolutionScale()) - t_hull . x;
    t_hull . height = (int)ceil((rect . origin . y + rect . size . height) * MCIPhoneGetResolutionScale()) - t_hull . y;
    
	MCRegionRef t_dirty_rgn;
	MCRegionCreate(t_dirty_rgn);
	MCRegionSetRect(t_dirty_rgn, t_hull);
	MCUIKitStackSurface t_surface(t_dirty_rgn, UIGraphicsGetCurrentContext(), t_stack -> getrect() . height);
	t_stack -> redrawwindow(&t_surface, t_dirty_rgn);
	MCRegionDestroy(t_dirty_rgn);
}

- (void)renderInRegion: (MCRegionRef)p_region
{
	MCRectangle t_visible;
	t_visible = MCRegionGetBoundingBox(p_region);
	
	[ self setNeedsDisplayInRect: CGRectMake((float)t_visible . x / MCIPhoneGetResolutionScale(), (float)t_visible . y / MCIPhoneGetResolutionScale(),
											 (float)t_visible . width / MCIPhoneGetResolutionScale(), (float)t_visible . height / MCIPhoneGetResolutionScale()) ];
	[[self layer] display];
}

@end

////////////////////////////////////////////////////////////////////////////////

class MCOpenGLStackSurface: public MCIPhoneStackSurface
{
	CALayer *m_layer;
	int32_t m_width;
	int32_t m_height;
	
public:
	MCOpenGLStackSurface(CALayer *p_layer, int32_t p_width, int32_t p_height)
		: MCIPhoneStackSurface(MCU_make_rect(0, 0, p_width, p_height))
	{
		m_width = p_width;
		m_height = p_height;
		m_layer = p_layer;
	}
	
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		if (p_type != kMCStackSurfaceTargetEAGLContext)
			return false;
		
		return true;
	}
	
	void UnlockTarget(void)
	{
	}

protected:
	void FlushBits(void *p_bits, uint32_t p_stride)
	{
		GLuint t_texture;
		glGenTextures(1, &t_texture);
		glBindTexture(GL_TEXTURE_2D, t_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // MW-2011-10-07: iOS 5 doesn't like an inconsistency in input format between
        //   TexImage2D and TexSubImage2D.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glColor4ub(255, 255, 255, 255);
		
		GLfloat t_vertices[8];
		
		GLfloat t_coords[8] =
		{
			0, 0,
			1.0, 0.0,
			0.0, 1.0,
			1.0, 1.0
		};
		
		glVertexPointer(2, GL_FLOAT, 0, t_vertices);
		glTexCoordPointer(2, GL_FLOAT, 0, t_coords);
		
		for(int32_t y = 0; y < (m_height + 255) / 256; y++)
			for(int32_t x = 0; x < (m_width + 255) / 256; x++)
			{
				int32_t t_tw, t_th;
				t_tw = MCMin(256, m_width - x * 256);
				t_th = MCMin(256, m_height - y * 256);
				
				// Fill the texture scanline by scanline
				for(int32_t s = 0; s < t_th; s++)
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, s, t_tw, 1, GL_BGRA, GL_UNSIGNED_BYTE, (uint8_t *)p_bits + (y * 256 + s) * p_stride + x * 256 * sizeof(uint32_t));
				
				int32_t t_px, t_py;
				t_px = x * 256;
				t_py = m_height - y * 256 - 256;
				
				// Setup co-ords.
				t_vertices[0] = t_px, t_vertices[1] = t_py + 256;
				t_vertices[2] = t_px + 256, t_vertices[3] = t_py + 256;
				t_vertices[4] = t_px, t_vertices[5] = t_py;
				t_vertices[6] = t_px + 256, t_vertices[7] = t_py;
				
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		
		glDeleteTextures(1, &t_texture);
	}
};

@implementation MCIPhoneOpenGLDisplayView

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)p_frame
{
	self = [super initWithFrame: p_frame];
	if (self == nil)
		return nil;
	
    // Turn off screen updates until we are ready.
    MCRedrawDisableScreenUpdates();
    
	// Initialize the layer
	
	CAEAGLLayer *t_layer;
	t_layer = (CAEAGLLayer *)[self layer];
	
	[t_layer setOpaque: YES];
	[t_layer setDrawableProperties:
				[NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil]];
	
	// Initialize the context
	
	m_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	[EAGLContext setCurrentContext: m_context];
	
	glGenFramebuffersOES(1, &m_framebuffer);
	glGenRenderbuffersOES(1, &m_renderbuffer);
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffer);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_renderbuffer);

	glDisable(GL_DEPTH_TEST);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
	return self;
}

- (void)dealloc
{
    [m_context release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[self resizeFromLayer:(CAEAGLLayer*)[self layer]];
    
    // Turn screen updates back on.
    MCRedrawEnableScreenUpdates();
    
	// Make sure the current window is completely redrawn.
	static_cast<MCScreenDC *>(MCscreen) -> redraw_current_window();
	
	MCRedrawUpdateScreen();
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffer);
    [m_context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &m_backing_width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &m_backing_height);
	
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }
    
    return YES;
}

- (void)renderInRegion: (MCRegionRef)p_region
{	
	// MW-2012-03-05: [[ ViewStack ]] Fetch the current stack the view should use,
	//   and return if there is none.
	MCStack *t_stack;
	t_stack = (MCStack *)[[self superview] currentStack];
	if (t_stack == nil)
		return;
	
    [EAGLContext setCurrentContext: m_context];
	
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, m_framebuffer);
    glClear(GL_COLOR_BUFFER_BIT);
	
	glViewport(0, 0, m_backing_width, m_backing_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, (GLfloat)m_backing_width, 0, (GLfloat)m_backing_height, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	MCOpenGLStackSurface t_surface([self layer], m_backing_width, m_backing_height);
	
	MCRegionRef t_dirty_rgn;
	MCRegionCreate(t_dirty_rgn);
	MCRegionSetRect(t_dirty_rgn, MCU_make_rect(0, 0, m_backing_width, m_backing_height));
	t_stack -> redrawwindow(&t_surface, t_dirty_rgn);
	MCRegionDestroy(t_dirty_rgn);
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffer);
    [m_context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

@end

////////////////////////////////////////////////////////////////////////////////
