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

#include "prefix.h"

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

#include "mbliphoneview.h"

#include "resolution.h"
#include "graphics_util.h"
#include "stacktile.h"

////////////////////////////////////////////////////////////////////////////////

extern UIView *MCIPhoneGetView(void);
extern float MCIPhoneGetResolutionScale(void);
extern float MCIPhoneGetDeviceScale(void);

extern bool MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);

////////////////////////////////////////////////////////////////////////////////

extern void MCIPhoneCallOnMainFiber(void (*)(void *), void *);

////////////////////////////////////////////////////////////////////////////////

static inline MCGRectangle MCGRectangleFromCGRect(CGRect p_rect)
{
	return MCGRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

static inline MCRectangle MCRectangleFromCGRect(const CGRect &p_rect)
{
	return MCRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

////////////////////////////////////////////////////////////////////////////////

static void do_update(void *p_dirty)
{
	[MCIPhoneGetDisplayView() renderInRegion: (MCRegionRef)p_dirty];
}

void MCStack::view_device_updatewindow(MCRegionRef p_dirty_rgn)
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

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
class MCIPhoneStackSurface: public MCStackSurface
{
protected:
	MCGRegionRef m_region;
	bool m_own_region;
	   
	virtual void FlushBits(MCGIntegerRectangle p_area, void *p_bits, uint32_t p_stride, bool &x_taken) = 0;

public:
	MCIPhoneStackSurface(MCGRegionRef p_region)
	{
		m_region = p_region;
		m_own_region = false;
	}
	
	MCIPhoneStackSurface(const MCGIntegerRectangle &p_rect)
	{
		/* UNCHECKED */ MCGRegionCreate(m_region);
		/* UNCHECKED */ MCGRegionSetRect(m_region, p_rect);
		m_own_region = true;
    }
	
	~MCIPhoneStackSurface()
	{
		if (m_own_region)
			MCGRegionDestroy(m_region);
	}
	
	bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef &r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
            if (MCGContextCreateWithRaster(t_raster, t_context))
			{
				// Set origin
                MCGContextTranslateCTM(t_context, -t_locked_area . origin . x, -t_locked_area . origin . y);
                
				// Set clipping rect
                MCGContextClipToRegion(t_context, m_region);
				MCGContextClipToRect(t_context, MCGIntegerRectangleToMCGRectangle(t_locked_area));
				
				r_context = t_context;
                r_raster = t_raster;
				
				return true;
			}
			
			UnlockPixels(t_locked_area, t_raster, false);
		}
		
		return false;
	}
	
	void UnlockGraphics(MCGIntegerRectangle p_area, MCGContextRef p_context, MCGRaster &p_raster)
	{
		if (p_context == nil)
			return;
		
		MCGContextRelease(p_context);
		UnlockPixels(p_area, p_raster, true);
	}
	
    bool LockPixels(MCGIntegerRectangle p_area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
    {
        MCGIntegerRectangle t_actual_area;
        t_actual_area = MCGIntegerRectangleIntersection(p_area, MCGRegionGetBounds(m_region));
        
        if (MCGIntegerRectangleIsEmpty(t_actual_area))
            return false;
        
        void *t_bits;
        t_bits = malloc(t_actual_area . size . height * t_actual_area . size . width * sizeof(uint32_t));
        if (t_bits != nil)
        {
            r_raster . width = t_actual_area . size . width ;
            r_raster . height = t_actual_area . size . height;
            r_raster . stride = r_raster . width * sizeof(uint32_t);
            r_raster . format = kMCGRasterFormat_xRGB;
            r_raster . pixels = t_bits;

			r_locked_area = t_actual_area;

            return true;
        }
        
        return false;
    }

	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
		UnlockPixels(p_area, p_raster, true);
	}
	
	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster, bool p_update)
	{
		if (p_raster . pixels == nil)
			return;

        bool t_taken = false;
        
		if (p_update)
			FlushBits(p_area, p_raster . pixels, p_raster . stride, t_taken);
        
        if (!t_taken)
        {
            free(p_raster.pixels);
        }
		
	}
	
	bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		bool t_success = true;
				
        MCGIntegerRectangle t_bounds;
        MCGContextRef t_context = nil;
        MCGRaster t_raster;
		if (t_success)
        {
            t_bounds = MCGRectangleGetBounds(p_dst_rect);
            t_success = LockGraphics(t_bounds, t_context, t_raster);
        }
		
		if (t_success)
		{
            // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNone);
		}
		
		UnlockGraphics(t_bounds, t_context, t_raster);
		
		return t_success;
	}

};

////////////////////////////////////////////////////////////////////////////////

CGRect MCGIntegerRectangleToCGRect(const MCGIntegerRectangle &p_rect)
{
	return CGRectMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

CGRect MCMacFlipCGRect(const CGRect &p_rect, uint32_t p_surface_height)
{
	return CGRectMake(p_rect.origin.x, p_surface_height - (p_rect.origin.y + p_rect.size.height), p_rect.size.width, p_rect.size.height);
}

struct MCGRegionConvertToCGRectsState
{
	CGRect *rects;
	uint32_t count;
};

static bool MCGRegionConvertToCGRectsCallback(void *p_state, const MCGIntegerRectangle &p_rect)
{
	MCGRegionConvertToCGRectsState *state;
	state = static_cast<MCGRegionConvertToCGRectsState*>(p_state);
	
	if (!MCMemoryResizeArray(state -> count + 1, state -> rects, state -> count))
		return false;
	
	state -> rects[state -> count - 1] = MCGIntegerRectangleToCGRect(p_rect);
	
	return true;
}

bool MCGRegionConvertToCGRects(MCGRegionRef self, CGRect *&r_cgrects, uint32_t& r_cgrect_count)
{
	MCGRegionConvertToCGRectsState t_state;
	t_state . rects = nil;
	t_state . count = 0;
	
	if (!MCGRegionIterate(self, MCGRegionConvertToCGRectsCallback, &t_state))
	{
		MCMemoryDeleteArray(t_state . rects);
		return false;
	}
	
	r_cgrects = t_state . rects;
	r_cgrect_count = t_state . count;
	
	return true;
}

static void MCMacClipCGContextToRegion(CGContextRef p_context, MCGRegionRef p_region, uint32_t p_surface_height)
{
	CGRect *t_rects;
	t_rects = nil;
	
	uint32_t t_count;
	
	if (!MCGRegionConvertToCGRects(p_region, t_rects, t_count))
		return;
	
	for (uint32_t i = 0; i < t_count; i++)
		t_rects[i] = MCMacFlipCGRect(t_rects[i], p_surface_height);
	
	CGContextClipToRects(p_context, t_rects, t_count);
	MCMemoryDeleteArray(t_rects);
}

class MCUIKitStackSurface: public MCIPhoneStackSurface
{
	CGContextRef m_context;
	int32_t m_height;
	
public:
	// IM-2013-08-23: [[ RefactorGraphics ]] Reinstate surface height parameter
	MCUIKitStackSurface(MCGRegionRef p_region, int32_t p_height, CGContextRef p_context)
		: MCIPhoneStackSurface(p_region)
	{
		m_context = p_context;
		m_height = p_height;
	}
	
	bool Lock(void)
	{
		return true;
	}
	
	void Unlock(void)
	{
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
	
protected:
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to pass in the area we wish to draw.
	void FlushBits(MCGIntegerRectangle p_area, void *p_bits, uint32_t p_stride, bool &x_taken)
	{
		void *t_target;
		if (!LockTarget(kMCStackSurfaceTargetCoreGraphics, t_target))
			return;
		
		CGContextRef t_context;
		t_context = (CGContextRef)t_target;
		
		// IM-2013-07-18: [[ RefactorGraphics ]] remove previous image flip transformation as now entire
		// CGContext will be flipped. Instead, we draw the image at an offset from the bottom
		
		CGColorSpaceRef t_colorspace;
		t_colorspace = nil;
		/* UNCHECKED */ MCImageGetCGColorSpace(t_colorspace);
		
		CGImageRef t_image;
		t_image = nil;
		
		MCGRaster t_raster;
		t_raster.width = p_area.size.width;
		t_raster.height = p_area.size.height;
		t_raster.pixels = p_bits;
		t_raster.stride = p_stride;
		t_raster.format = kMCGRasterFormat_xRGB;
		
		// IM-2014-07-01: [[ GraphicsPerformance ]] Clip the output context to only those areas we've had to redraw
		MCMacClipCGContextToRegion(t_context, m_region, m_height);
		
		if (MCGRasterToCGImage(t_raster, MCGIntegerRectangleMake(0, 0, p_area.size.width, p_area.size.height), t_colorspace, false, false, t_image))
		{
            x_taken = true;
            
			CGContextDrawImage(t_context, CGRectMake((float)p_area.origin.x, (float)(m_height - (p_area.origin.y + p_area.size.height)), (float)p_area.size.width, (float)p_area.size.height), t_image);
			CGImageRelease(t_image);
		}
		
		CGColorSpaceRelease(t_colorspace);
		
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

static MCGRegionRef s_redraw_region = nil;

- (void) drawRect: (CGRect)rect
{	
	// MW-2012-03-05: [[ ViewStack ]] Fetch the current stack the view should use,
	//   and return if there is none.
	MCStack *t_stack;
	t_stack = (MCStack *)[[self superview] currentStack];
	if (t_stack == nil)
		return;
	
	// IM-2014-01-30: [[ HiDPI ]] Ensure stack backing scale is set
	t_stack->view_setbackingscale(MCResGetPixelScale());
    
	MCGFloat t_scale;
	
	// IM-2014-01-30: [[ HiDPI ]] Convert screen to surface coords
	t_scale = MCIPhoneGetResolutionScale();
	
	MCGAffineTransform t_scale_transform;
	t_scale_transform = MCGAffineTransformMakeScale(t_scale, t_scale);
	
	MCGRegionRef t_region;
	t_region = nil;
	
    // MW-2014-07-31: [[ Bug ]] The clipping region iOS wants is not what we ask for
    //   and there's no way to get it, so the best we can do is just use the rect it
    //   gives us.
	//if (s_redraw_region == nil)
	//{
		MCGIntegerRectangle t_hull;
		t_hull = MCGRectangleGetBounds(MCGRectangleScale(MCGRectangleFromCGRect(rect), t_scale));
		
		MCGRegionCreate(t_region);
		MCGRegionSetRect(t_region, t_hull);
	/*}
	else
	{
		MCGRegionCopyWithTransform(s_redraw_region, t_scale_transform, t_region);
		MCGRegionSetEmpty(s_redraw_region);
	}*/
	
	// IM-2013-08-23: [[ RefactorGraphics ]] pass scaled surface height to stack surface constructor
	// IM-2014-08-18: [[ Bug 13163 ]] The device rect needs to be based on the view rect rather than the stack rect.
	MCRectangle t_device_rect;
	t_device_rect = MCRectangleGetScaledBounds(t_stack->view_getrect(), t_scale);
    
    CGContext *t_cgcontext;
	t_cgcontext = UIGraphicsGetCurrentContext();
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Moved context configuration out of stack surface, to ensure it only occurs once.
	CGContextScaleCTM(t_cgcontext, 1.0 / t_scale, 1.0 / t_scale);
    
    // MW-2011-10-18: Turn off image interpolation to stop artifacts occuring
    //   when redrawing.
    CGContextSetInterpolationQuality(t_cgcontext, kCGInterpolationNone);
    
    // IM-2013-08-23: [[ RefactorGraphics ]] Flip the surface so the origin is the
    // bottom-left, as expected by the MCTileCacheCoreGraphicsCompositor
    CGContextScaleCTM(t_cgcontext, 1.0, -1.0);
    CGContextTranslateCTM(t_cgcontext, 0.0, -t_device_rect . height);

    MCUIKitStackSurface t_surface(t_region, t_device_rect.height, t_cgcontext);
    if (t_surface . Lock())
    {
        t_stack -> view_surface_redrawwindow(&t_surface, t_region);
        t_surface . Unlock();
    }
	
	MCGRegionDestroy(t_region);
}

bool MCMacDoUpdateRegionCallback(void *p_context, const MCRectangle &p_rect)
{
	UIView *t_view = static_cast<UIView*>(p_context);
	[t_view setNeedsDisplayInRect: MCRectangleToCGRect(p_rect)];
	
	return true;
}
- (void)renderInRegion: (MCRegionRef)p_region
{
	if (s_redraw_region == nil)
		MCGRegionCreate(s_redraw_region);
	
	MCGRegionAddRegion(s_redraw_region, (MCGRegionRef)p_region);
	MCRegionForEachRect(p_region, MCMacDoUpdateRegionCallback, self);
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
		: MCIPhoneStackSurface(MCGIntegerRectangleMake(0, 0, p_width, p_height))
	{
		m_width = p_width;
		m_height = p_height;
		m_layer = p_layer;
	}
	
	bool Lock(void)
	{
		return true;
	}
	
	void Unlock(void)
	{
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
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to pass in the area we wish to draw.
	void FlushBits(MCGIntegerRectangle p_area, void *p_bits, uint32_t p_stride, bool& x_taken)
	{
		GLuint t_texture;
		glGenTextures(1, &t_texture);
		glBindTexture(GL_TEXTURE_2D, t_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
		// IM_2013-08-21: [[ RefactorGraphics ]] set iOS pixel format to RGBA
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		
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
					// IM_2013-08-21: [[ RefactorGraphics ]] set iOS pixel format to RGBA
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, s, t_tw, 1, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t *)p_bits + (y * 256 + s) * p_stride + x * 256 * sizeof(uint32_t));
				
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
	   
	MCGRegionRef t_dirty_rgn;
	MCGRegionCreate(t_dirty_rgn);
	MCGRegionSetRect(t_dirty_rgn, MCGIntegerRectangleMake(0, 0, m_backing_width, m_backing_height));

    if (t_surface . Lock())
	{
		t_stack -> view_surface_redrawwindow(&t_surface, t_dirty_rgn);
		t_surface . Unlock();
	}
	
	MCGRegionDestroy(t_dirty_rgn);
	
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_renderbuffer);
    [m_context presentRenderbuffer:GL_RENDERBUFFER_OES];
}

@end

////////////////////////////////////////////////////////////////////////////////
