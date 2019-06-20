#include "graphics.h"
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

#include "graphics-internal.h"

#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkPaint.h>
#include <SkBitmap.h>
#include <SkShader.h>
#include <SkLayerDrawLooper.h>
#include <SkBlurMaskFilter.h>
#include <SkColorFilter.h>
#include <SkDevice.h>
#include <SkImageFilter.h>
#include <SkOffsetImageFilter.h>
#include <SkBlurImageFilter.h>
#include <SkTypeface.h>
#include <SkColorPriv.h>
#include <SkSurface.h>

#include <time.h>

#include "SkStippleMaskFilter.h"

////////////////////////////////////////////////////////////////////////////////


struct MCGIRectangle
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

MCGIRectangle MCGIRectangleMake(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	MCGIRectangle r;
	r . left = left;
	r . top = top;
	r . right = right;
	r . bottom = bottom;
	return r;
}

MCGIRectangle MCGRectangleComputeHull(const MCGRectangle& self)
{
	return MCGIRectangleMake((int)floor(self . origin . x), (int)floor(self . origin . y), (int)ceil(self . origin . x + self . size . width), (int)ceil(self . origin . y + self . size . height));
}

MCGIRectangle MCGIRectangleUnion(const MCGIRectangle& left, const MCGIRectangle& right)
{
	return MCGIRectangleMake(SkMin32(left . left, right . left), SkMin32(left . top, right . top), SkMax32(left . right, right . right), SkMax32(left . bottom, right . bottom));
}

MCGIRectangle MCGIRectangleIntersect(const MCGIRectangle& left, const MCGIRectangle& right)
{
	return MCGIRectangleMake(SkMax32(left . left, right . left), SkMax32(left . top, right . top), SkMin32(left . right, right . right), SkMin32(left . bottom, right . bottom));
}

MCGIRectangle MCGIRectangleOffset(const MCGIRectangle& self, int32_t dx, int32_t dy)
{
	return MCGIRectangleMake(self . left + dx, self . top + dy, self . right + dx, self . bottom + dy);
}

MCGIRectangle MCGIRectangleExpand(const MCGIRectangle& self, int32_t dx, int32_t dy)
{
	return MCGIRectangleMake(self . left - dx, self . top - dy, self . right + dx, self . bottom + dy);
}

////////////////////////////////////////////////////////////////////////////////

static void MCGContextStateDestroy(MCGContextStateRef self)
{
	if (self != NULL)
	{
        MCGRelease(self->fill_paint);
        MCGRelease(self->stroke_paint);
		MCGDashesRelease(self -> stroke_attr . dashes);
	}
	MCMemoryDelete(self);		
}

static bool MCGContextStateCreate(MCGContextStateRef& r_state)
{
	bool t_success;
	t_success = true;
	
	__MCGContextState *t_state;
	t_state = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_state);
	
	if (t_success)
	{
        t_state -> base_transform = MCGAffineTransformMakeIdentity();
        t_state -> transform = MCGAffineTransformMakeIdentity();
        
		t_state -> opacity = 1.0f;
		t_state -> blend_mode = kMCGBlendModeSourceOver;		
		t_state -> flatness = 0.0f;
		t_state -> should_antialias = false;
		
		t_state -> fill_paint = MCGRetain(kMCGBlackSolidColor);
		t_state -> fill_opacity = 1.0f;
		t_state -> fill_rule = kMCGFillRuleNonZero;
		t_state -> fill_style = kMCGPaintStyleOpaque;
		
        t_state -> stroke_paint = nullptr;
		t_state -> stroke_opacity = 1.0f;
		t_state -> stroke_attr . width = 0.0f;
		t_state -> stroke_attr . join_style = kMCGJoinStyleBevel;
		t_state -> stroke_attr . cap_style = kMCGCapStyleButt;
		t_state -> stroke_attr . miter_limit = 0.0f;
		t_state -> stroke_attr . dashes = NULL;
		t_state -> stroke_style = kMCGPaintStyleOpaque;
		
		t_state -> is_layer_begin_pt = false;
		t_state -> parent = NULL;		
	}
	
	if (t_success)
		r_state = t_state;
	else
		MCGContextStateDestroy(t_state);
	
	return t_success;
}

static bool MCGContextStateCopy(MCGContextStateRef p_state, MCGContextStateRef& r_new_state)
{
	bool t_success;
	t_success = true;
	
	__MCGContextState *t_state;
	t_state = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_state);
	
	if (t_success)
	{
        t_state->base_transform = MCGAffineTransformMakeIdentity();
        t_state->transform = MCGAffineTransformMakeIdentity();
    
		t_state -> opacity = p_state -> opacity;
		t_state -> blend_mode = p_state -> blend_mode;		
		t_state -> flatness = p_state -> flatness;
		t_state -> should_antialias = p_state -> should_antialias;
		
		t_state -> fill_paint = MCGRetain(p_state->fill_paint);
		t_state -> fill_opacity = p_state -> fill_opacity;
		t_state -> fill_rule = p_state -> fill_rule;
		t_state -> fill_style = p_state -> fill_style;
		
		t_state -> stroke_paint = MCGRetain(p_state->stroke_paint);
		t_state -> stroke_opacity = p_state -> stroke_opacity;
		t_state -> stroke_attr . width = p_state -> stroke_attr . width;
		t_state -> stroke_attr . join_style = p_state -> stroke_attr . join_style;
		t_state -> stroke_attr . cap_style = p_state -> stroke_attr . cap_style;
		t_state -> stroke_attr . miter_limit = p_state -> stroke_attr . miter_limit;
		t_state -> stroke_attr . dashes = MCGDashesRetain(p_state -> stroke_attr . dashes);
		t_state -> stroke_style = p_state -> stroke_style;
		
		t_state -> is_layer_begin_pt = false;
		t_state -> parent = NULL;		
	}
	
	if (t_success)
		r_new_state = t_state;
	else
		MCGContextStateDestroy(t_state);
	
	return t_success;
}
////////////////////////////////////////////////////////////////////////////////

static void MCGContextLayerDestroy(MCGContextLayerRef self)
{
	if (self == nil)
		return;
		
	// If there is a surface, it manages the canvas' lifetime
	if (self->m_surface == nullptr)
		self -> canvas -> unref();

	self->m_surface.reset();
	
	MCMemoryDelete(self);
}

static bool MCGContextLayerCreateUnbound(MCGContextLayerRef& r_layer)
{
	bool t_success;
	t_success = true;

	__MCGContextLayer *t_layer;
	t_layer = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_layer);

	if (t_success)
	{
		t_layer->nesting = 0;
		t_layer->parent = nil;
	}

	if (t_success)
		r_layer = t_layer;
	else
		MCGContextLayerDestroy(t_layer);

	return t_success;
}

static bool MCGContextLayerCreateWithCanvas(SkCanvas *p_canvas, MCGContextLayerRef& r_layer)
{
	if (!MCGContextLayerCreateUnbound(r_layer))
		return false;

	r_layer->canvas = p_canvas;
	r_layer->canvas->ref();

	return true;
}

static bool MCGContextLayerCreateWithSurface(sk_sp<SkSurface> p_surface, MCGContextLayerRef& r_layer)
{
	if (!MCGContextLayerCreateUnbound(r_layer))
		return false;

	// Lifetime of the canvas is controlled by the surface
	r_layer->m_surface = p_surface;
	r_layer->canvas = p_surface->getCanvas();

	return true;
}

static bool MCGContextLayerCreateSoftware(uint32_t p_width, uint32_t p_height, bool p_alpha, MCGContextLayerRef& r_layer)
{
	// Create the image information for native-endian RGB with or without alpha
	SkImageInfo t_info = SkImageInfo::MakeN32(p_width, p_height, p_alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);

	// Create the bitmap and set its info
	SkBitmap t_bitmap;
	t_bitmap.setInfo(t_info);

	// Allocate the pixels for the bitmap
	if (t_bitmap.tryAllocPixels())
	{
		t_bitmap.eraseARGB(0, 0, 0, 0);
		sk_sp<SkCanvas> t_canvas(new (nothrow) SkCanvas(t_bitmap));
		return MCGContextLayerCreateWithCanvas(t_canvas.get(), r_layer);
	}
	else
		return false;
}

static bool MCGContextLayerCreate(uint32_t p_width, uint32_t p_height, bool p_alpha, MCGContextLayerRef& r_layer)
{
	// This function contains hooks for adding OpenGL-accelerated drawing support
    
    //if (!MCGPrepareOpenGL())
		return MCGContextLayerCreateSoftware(p_width, p_height, p_alpha, r_layer);

	// Get the GPU context
	//GrContext* t_context = MCGGetGrContext();

	// Create a GPU-backed surface as the render target
	//SkImageInfo t_info = SkImageInfo::MakeN32(p_width, p_height, p_alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
	//sk_sp<SkSurface> t_surface(SkSurface::MakeRenderTarget(t_context, SkBudgeted::kNo, t_info));
	//if (t_surface == nullptr)
	//	return false;

	// Create a context from the surface
	//return MCGContextLayerCreateWithSurface(t_surface, r_layer);
}

////////////////////////////////////////////////////////////////////////////////

static bool MCGContextPushState(MCGContextRef self)
{
	bool t_success;
	t_success = true;	
	
	MCGContextStateRef t_state;
	if (t_success)		
		t_success = MCGContextStateCopy(self -> state, t_state);
	
	if (t_success)
	{		
		t_state -> parent = self -> state;
        t_state->base_transform = MCGContextGetDeviceTransform(self);
		self -> state = t_state;
	}
	
	return t_success;
}

static bool MCGContextPopState(MCGContextRef self)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextStateRef t_parent_state;
		t_parent_state = self -> state -> parent;
		MCGContextStateDestroy(self -> state);
		self -> state = t_parent_state;
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static void MCGContextDestroy(MCGContextRef self)
{
	if (self != NULL)
	{
		while (self -> state != NULL)
		{
			MCGContextStateRef t_tmp_state;
			t_tmp_state = self -> state -> parent;
			MCGContextStateDestroy(self -> state);
			self -> state = t_tmp_state;
		}
		
		while(self -> layer != NULL)
		{
			MCGContextLayerRef t_tmp_layer;
			t_tmp_layer = self -> layer -> parent;
			MCGContextLayerDestroy(self -> layer);
			self -> layer = t_tmp_layer;
		}

		if (self -> path != NULL)
			MCGPathRelease(self -> path);
	}
	
	MCMemoryDelete(self);
}

static bool MCGContextCreateUnbound(MCGContextRef& r_context)
{
	bool t_success;
	t_success = true;

	__MCGContext *t_context;
	t_context = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_context);

	if (t_success)
		t_success = MCGContextStateCreate(t_context->state);

	if (t_success)
	{
		t_context->references = 1;
		t_context->path = NULL;
		t_context->is_valid = true;
	}

	if (t_success)
	{
		r_context = t_context;
	}
	else
		MCGContextDestroy(t_context);

	return t_success;
}

static bool MCGContextCreateWithBitmap(SkBitmap& p_bitmap, MCGContextRef& r_context)
{
	bool t_success = true;

	MCGContextRef t_context = nullptr;
	t_success = MCGContextCreateUnbound(t_context);

	SkCanvas *t_canvas = nil;

	if (t_success)
	{
		t_canvas = new (nothrow) SkCanvas(p_bitmap);
		t_success = t_canvas != nil;
	}
	
	if (t_success)
		t_success = MCGContextLayerCreateWithCanvas(t_canvas, t_context->layer);

	if (t_success)
		r_context = t_context;
	else
		MCGContextDestroy(t_context);
	
	if (t_canvas != nil)
		t_canvas -> unref();
	
	return t_success;
}

static bool MCGContextCreateWithSurface(sk_sp<SkSurface> p_surface, MCGContextRef& r_context)
{
	bool t_success = true;

	MCGContextRef t_context;
	t_success = MCGContextCreateUnbound(t_context);

	if (t_success)
		t_success = MCGContextLayerCreateWithSurface(p_surface, t_context->layer);

	if (t_success)
		r_context = t_context;
	else
		MCGContextDestroy(t_context);

	return t_success;
}

static bool MCGContextCreateSoftware(uint32_t p_width, uint32_t p_height, bool p_alpha, MCGContextRef& r_context)
{
	// Create the image information for native-endian RGB with or without alpha
	SkImageInfo t_info = SkImageInfo::MakeN32(p_width, p_height, p_alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);

	// Create the bitmap and set its info
	SkBitmap t_bitmap;
	t_bitmap.setInfo(t_info);

	// Allocate the pixels for the bitmap
	if (t_bitmap.tryAllocPixels())
	{
		t_bitmap.eraseARGB(0, 0, 0, 0);
		return MCGContextCreateWithBitmap(t_bitmap, r_context);
	}
	else
		return false;
}

bool MCGContextCreate(uint32_t p_width, uint32_t p_height, bool p_alpha, MCGContextRef& r_context)
{
	// This function contains hooks for adding OpenGL-accelerated drawing support
    
    //if (!MCGPrepareOpenGL())
		return MCGContextCreateSoftware(p_width, p_height, p_alpha, r_context);

	// Get the GPU context
	//GrContext* t_context = MCGGetGrContext();

	// Create a GPU-backed surface as the render target
	//SkImageInfo t_info = SkImageInfo::MakeN32(p_width, p_height, p_alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
	//sk_sp<SkSurface> t_surface(SkSurface::MakeRenderTarget(t_context, SkBudgeted::kNo, t_info));
	//if (t_surface == nullptr)
	//	return false;

	// Create a context from the surface
	//return MCGContextCreateWithSurface(t_surface, r_context);
}

bool MCGContextCreateWithRaster(const MCGRaster& p_raster, MCGContextRef& r_context)
{
	SkBitmap t_bitmap;
	if (MCGRasterToSkBitmap(p_raster, kMCGPixelOwnershipTypeBorrow, t_bitmap))
		return MCGContextCreateWithBitmap(t_bitmap, r_context);
	else
		return false;
}

bool MCGContextCreateWithPixels(uint32_t p_width, uint32_t p_height, uint32_t p_stride, void *p_pixels, bool p_alpha, MCGContextRef& r_context)
{	
	// Create the image information for native-endian RGB with or without alpha
	SkImageInfo t_info = SkImageInfo::MakeN32(p_width, p_height, p_alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);

	// Create the bitmap and set its info
	SkBitmap t_bitmap;
	t_bitmap.setInfo(t_info);

	// Tell the bitmap to use the given pixels
	t_bitmap.installPixels(t_info, p_pixels, p_stride);
	
	return MCGContextCreateWithBitmap(t_bitmap, r_context);	
}

void *MCGContextGetPixelPtr(MCGContextRef context)
{
    MCGContextLayerRef t_layer;
    t_layer = context -> layer;
    while(t_layer -> parent != nil)
        t_layer = t_layer -> parent;
    
	const SkBitmap& t_bitmap = t_layer -> canvas -> getTopDevice() -> accessBitmap(false);
    
    return t_bitmap . getPixels();
}

uint32_t MCGContextGetWidth(MCGContextRef context)
{
    MCGContextLayerRef t_layer;
    t_layer = context -> layer;
    while(t_layer -> parent != nil)
        t_layer = t_layer -> parent;
    
	const SkBitmap& t_bitmap = t_layer -> canvas -> getTopDevice() -> accessBitmap(false);
    
    return t_bitmap . width();
}

uint32_t MCGContextGetHeight(MCGContextRef context)
{
    MCGContextLayerRef t_layer;
    t_layer = context -> layer;
    while(t_layer -> parent != nil)
        t_layer = t_layer -> parent;
    
	const SkBitmap& t_bitmap = t_layer -> canvas -> getTopDevice() -> accessBitmap(false);
    
    return t_bitmap . height();
}

MCGContextRef MCGContextRetain(MCGContextRef self)
{
	if (self != NULL)
		self -> references++;
	return self;
}

void MCGContextRelease(MCGContextRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGContextDestroy(self);
	}
}

////////////////////////////////////////////////////////////////////////////////

// Returns whether the current context context is valid. If an error
// occurs when calling any method on a context context, it will become
// invalid and all further operations will be no-ops.
bool MCGContextIsValid(MCGContextRef self)
{
	return self != NULL && self -> is_valid;
}

bool MCGContextIsLayerOpaque(MCGContextRef self)
{
	return SkAlphaTypeIsOpaque(self->layer->canvas->imageInfo().alphaType());
}

////////////////////////////////////////////////////////////////////////////////
// Graphics state operations

void MCGContextSave(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
	{
		// we use skia to manage the clip and matrix between states, everything else is held in the state directly 		
		self -> layer -> canvas -> save();
		t_success = MCGContextPushState(self);
	}
	
	self -> is_valid = t_success;
}

void MCGContextRestore(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> state -> parent != NULL;
	
	// make sure we don't restore to a state before the current layer was begun
	if (t_success)
		if (!self -> state -> parent -> is_layer_begin_pt)
			t_success = MCGContextPopState(self);
	
	// we use skia to maintain a state's clip and transform, so calling restore ensures that the parent state's clip and CTM are restored properly 
	if (t_success)
		self -> layer -> canvas -> restore();	
	
	self -> is_valid = t_success;	
}

////////////////////////////////////////////////////////////////////////////////
// General attributes

void MCGContextSetFlatness(MCGContextRef self, MCGFloat p_flatness)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> flatness = p_flatness;
}

void MCGContextSetShouldAntialias(MCGContextRef self, bool p_should_antialias)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> should_antialias = p_should_antialias;
}

////////////////////////////////////////////////////////////////////////////////
// Layer attributes and manipulation - bitmap effect options would be added here also.

// Now replay the clip from the parent layer.
class CanvasClipVisitor: public SkCanvas::ClipVisitor
{
public:
	CanvasClipVisitor(SkCanvas *p_target_canvas)
	{
		m_target_canvas = p_target_canvas;
	}

	void clipRect(const SkRect& p_rect, SkRegion::Op p_op, bool p_antialias) override
	{
		m_target_canvas -> clipRect(p_rect, p_op, false);
	}

	void clipRRect(const SkRRect& p_rrect, SkRegion::Op p_op, bool p_antialias) override
	{
		// Todo: support rounded rect clip regions
		clipRect(p_rrect.getBounds(), p_op, p_antialias);
	}
	
	void clipPath(const SkPath& p_path, SkRegion::Op p_op, bool p_antialias) override
	{
		m_target_canvas -> clipPath(p_path, p_op, false);
	}
	
private:
	SkCanvas *m_target_canvas;
};
	
void MCGContextBegin(MCGContextRef self, bool p_need_layer)
{
	if (!MCGContextIsValid(self))
		return;
	
	// If we are blending sourceOver
	if (!p_need_layer && self -> state -> blend_mode == kMCGBlendModeSourceOver && self -> state -> opacity == 1.0)
	{
		//MCGContextSave(self);
		self -> layer -> nesting += 1;
		return;
	}
	
	// Fetch the bounds of the current clip in device co-ords and if the
	// clip is empty, then just increase the nesting level.
	SkIRect t_device_clip;
	if (!self -> layer -> canvas -> getClipDeviceBounds(&t_device_clip))
	{
		//MCGContextSave(self);
		self -> layer -> nesting += 1;
		return;
	}
	
	// Fetch the total matrix of the canvas.
	SkMatrix t_device_matrix;
	t_device_matrix = self -> layer -> canvas -> getTotalMatrix();
	
	// Create a suitable bitmap.
	SkBitmap t_new_bitmap;
	t_new_bitmap.setInfo(SkImageInfo::MakeN32Premul(t_device_clip.width(), t_device_clip.height()));
	if (!t_new_bitmap.tryAllocPixels())
	{
		self -> is_valid = false;
		return;
	}
	
	// Clear the pixel buffer.
	memset(t_new_bitmap . getPixels(), 0, t_new_bitmap . rowBytes() * t_new_bitmap . height());
	
	// Create a new layer the same size as the device clip
	MCGContextLayerRef t_new_layer;
	SkIRect t_clip_rect;
	self->layer->canvas->getClipDeviceBounds(&t_clip_rect);
	if (!MCGContextLayerCreate(t_clip_rect.width(), t_clip_rect.height(), true, t_new_layer))
	{
		self->is_valid = false;
		return;
	}

	// Get the canvas from the new layer
	SkCanvas* t_new_canvas = t_new_layer->canvas;

	// Next translate the canvas by the translation factor of the matrix.
	t_new_canvas -> translate(-t_device_clip . x(), -t_device_clip . y());
	
	// Replay the clip.
	CanvasClipVisitor t_clip_visitor(t_new_canvas);
	self -> layer -> canvas -> replayClips(&t_clip_visitor);
	
	// Set the matrix.
	t_new_canvas -> concat(t_device_matrix);
	
	// Make a save point in the new canvas.
	t_new_canvas -> save();
	
	// Set the current state as the layer being pt.
	self -> state -> is_layer_begin_pt = true;
	
	// Push the current state onto the attribute stack.
	MCGContextPushState(self);
	self -> state -> opacity = 1.0;
	self -> state -> blend_mode = kMCGBlendModeSourceOver;
	
	t_new_layer -> parent = self -> layer;
	t_new_layer -> origin_x = t_device_clip . x();
	t_new_layer -> origin_y = t_device_clip . y();
	t_new_layer -> has_effects = false;
	self -> layer = t_new_layer;
}

// The 'shape' parameter is the rectangle in user-space of the area to which the effect
// is to be applied.
//
// This shape is unrelated to the current clip, and the resulting rect of the layer which
// is to be rendered into must be big enough to ensure that any pixels rendered as a result
// of the bitmap effects have source pixels to operate on.
//
void MCGContextBeginWithEffects(MCGContextRef self, MCGRectangle p_shape, const MCGBitmapEffects& p_effects)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGAffineTransform t_device_transform;
	t_device_transform = MCGContextGetDeviceTransform(self);
	
	MCGIRectangle t_device_clip;
	t_device_clip = MCGRectangleComputeHull(MCGContextGetDeviceClipBounds(self));

	// First transform the shape rect by the total transform to get it's rectangle in device space.
	MCGIRectangle t_device_shape;
	t_device_shape = MCGRectangleComputeHull(MCGRectangleApplyAffineTransform(p_shape, t_device_transform));
	
	// This is the rectangle of pixels not related to bitmap effects which are needed.
	MCGIRectangle t_layer_clip;
	t_layer_clip = MCGIRectangleIntersect(t_device_clip, t_device_shape);
	
	// This calculates the rect of pixels needed from the layer to draw the drop shadow.
	// We offset the shape by the drop shadow x/y and intersect with the clip.
	// We then offset it back and expand by the blur radius.
	// We then intersect with the shape.
	// Finally add this rectangle to the layer clip (union).
	if (p_effects . has_drop_shadow)
	{
		MCGSize t_radii, t_offset;
		t_radii = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . drop_shadow . size, p_effects . drop_shadow . size), t_device_transform);
		t_offset = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . drop_shadow . x_offset, p_effects . drop_shadow . y_offset), t_device_transform);
		
		t_layer_clip = MCGIRectangleUnion(
							t_layer_clip,
							MCGIRectangleIntersect(
								t_device_shape,
								MCGIRectangleExpand(
									MCGIRectangleUnion(
										MCGIRectangleOffset(
											MCGIRectangleIntersect(
												t_device_clip,
												MCGIRectangleOffset(
													t_device_shape,
													floor(t_offset . width), floor(t_offset . height))),
											-floor(t_offset . width), -floor(t_offset . height)),
										MCGIRectangleOffset(
											MCGIRectangleIntersect(
												t_device_clip,
												MCGIRectangleOffset(
													t_device_shape,
													ceil(t_offset . width), ceil(t_offset . height))),
											-ceil(t_offset . width), -ceil(t_offset . height))),
										ceil(t_radii . width), ceil(t_radii . height))));
	}
	
	
	// Next process the inner shadow.
	// We intersect the shape with the clip to determine the visible pixels (as inner shadow only works internally).
	// We then offset it by the inner shadow x/y (to work out what source pixels are needed)
	// We then expand by the blur radius.
	// We then intersect with the shape.
	// Finally add this rectangle to the layer clip (union).
	if (p_effects . has_inner_shadow)
	{
		MCGSize t_radii, t_offset;
		t_radii = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . inner_shadow . size, p_effects . inner_shadow . size), t_device_transform);
		t_offset = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . inner_shadow . x_offset, p_effects . inner_shadow . y_offset), t_device_transform);
		
		// MW-2013-10-31: [[ Bug 11359 ]] Offset by -t_offset, not t_offset.
		t_layer_clip = MCGIRectangleUnion(
							t_layer_clip,
							MCGIRectangleIntersect(
								t_device_shape,
								MCGIRectangleExpand(
									MCGIRectangleUnion(
										MCGIRectangleOffset(
											MCGIRectangleIntersect(t_device_shape, t_device_clip),
											-floor(t_offset . width), -floor(t_offset . height)),
										MCGIRectangleOffset(
											MCGIRectangleIntersect(t_device_shape, t_device_clip),
											-ceil(t_offset . width), -ceil(t_offset . height))),
									ceil(t_radii . width), ceil(t_radii . height))));
	}
	
	// Next process outer glow.
	// We intersect the shape with the clip to determine visible pixels
	// We then expand by the radii.
	// We then intersect with the device shape to restrict to renderable pixels.
	// Finally we add this rectangle to the layer clip (union).
	if (p_effects . has_outer_glow)
	{
		MCGSize t_radii;
		t_radii = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . outer_glow . size, p_effects . outer_glow . size), t_device_transform);
		
		t_layer_clip = MCGIRectangleUnion(
							t_layer_clip,
							MCGIRectangleIntersect(
									t_device_shape,
									MCGIRectangleExpand(
										MCGIRectangleIntersect(t_device_shape, t_device_clip),
										ceil(t_radii . width), ceil(t_radii . height))));
	}
	
	// Next process inner glow.
	// We intersect the shape with the clip to determine visible pixels (inner glow only works internally).
	// We then expand by the radii.
	// We then intersect with the shape
	// Finally we add this rectangle to the layer clip (union).
	if (p_effects . has_inner_glow)
	{
		// MW-2013-10-31: [[ Bug 11359 ]] Make sure we use the inner_glow size size field, not the outer_glow one.
		MCGSize t_radii;
		t_radii = MCGSizeApplyAffineTransform(MCGSizeMake(p_effects . inner_glow . size, p_effects . inner_glow . size), t_device_transform);
		
		t_layer_clip = MCGIRectangleUnion(
							t_layer_clip,
							MCGIRectangleIntersect(
									t_device_shape,
									MCGIRectangleExpand(
											MCGIRectangleIntersect(t_device_clip, t_device_shape),
											ceil(t_radii . width), ceil(t_radii . height))));
	}

	t_layer_clip = MCGIRectangleIntersect(t_layer_clip, t_device_shape);

	// IM-2014-06-24: [[ GraphicsPerformance ]] If the clip is empty then don't try to create a new layer
	if ((t_layer_clip.left >= t_layer_clip.right) || (t_layer_clip.top >= t_layer_clip.bottom))
	{
		self -> layer -> nesting += 1;
		return;
	}
	
	// Allocate a new layer the same size as the device clip
	MCGContextLayerRef t_new_layer;
	if (!MCGContextLayerCreate(t_layer_clip.right - t_layer_clip.left, t_layer_clip.bottom - t_layer_clip.top, true, t_new_layer))
	{
		self->is_valid = false;
		return;
	}

	// Get the canvas from the new layer
	SkCanvas* t_new_canvas = t_new_layer->canvas;
	
	// Next translate the canvas by the translation factor of the matrix.
	t_new_canvas -> translate(-t_layer_clip . left, -t_layer_clip . top);
	
	// Set the matrix.
	t_new_canvas -> concat(self -> layer -> canvas -> getTotalMatrix());
	
	// Make a save point in the new canvas.
	t_new_canvas -> save();
	
	// Set the current state as the layer being pt.
	self -> state -> is_layer_begin_pt = true;
	
	// Push the current state onto the attribute stack.
	MCGContextPushState(self);
	self -> state -> opacity = 1.0;
	self -> state -> blend_mode = kMCGBlendModeSourceOver;
	
	t_new_layer -> parent = self -> layer;
	t_new_layer -> origin_x = t_layer_clip . left;
	t_new_layer -> origin_y = t_layer_clip . top;
	t_new_layer -> has_effects = true;
	t_new_layer -> effects = p_effects;
	self -> layer = t_new_layer;
}

// Utility methods for drawing against a raster mask
static void MCGMaskedDeviceFill(SkCanvas& p_canvas, const SkMask& p_mask, const SkPaint& p_paint)
{
	SkRect t_bounds = SkRect::Make(p_mask.fBounds);
	SkImageInfo t_info = SkImageInfo::MakeA8(p_mask.fBounds.width(), p_mask.fBounds.height());
	SkBitmap t_mask_bitmap;
	t_mask_bitmap.setInfo(t_info);
	t_mask_bitmap.installMaskPixels(p_mask);

	p_canvas.save();
	p_canvas.resetMatrix();
	p_canvas.drawBitmap(t_mask_bitmap, t_bounds.x(), t_bounds.y(), &p_paint);
	p_canvas.restore();
}

static void MCGContextRenderEffect(MCGContextRef self, const SkMask& p_mask, MCGSize p_radii, MCGSize p_offset, MCGFloat p_spread, MCGBlurType p_attenuation, MCGColor p_color, MCGBlendMode p_blend)
{
	// Get the device transform.
	MCGAffineTransform t_transform;
	t_transform = MCGContextGetDeviceTransform(self);
	
	// Compute the transforms of the radii and offset.
	MCGSize t_transformed_radii, t_transformed_offset;
	t_transformed_radii = MCGSizeApplyAffineTransform(p_radii, t_transform);
	t_transformed_offset = MCGSizeApplyAffineTransform(p_offset, t_transform);
	
	// Now blur the mask.
	SkMask t_blurred_mask;
	if (!MCGBlurBox(p_mask, t_transformed_radii . width, t_transformed_radii . height, p_spread, p_spread, t_blurred_mask))
		return;
	
	// Offset the blur mask appropriately.
	// TODO: Handle sub-pixel case!
	t_blurred_mask . fBounds . offset(t_transformed_offset . width, t_transformed_offset . height);
	
	// Compute the intersection.
	SkIRect t_inside;
	bool t_overlap;
	t_overlap = t_inside . intersect(p_mask . fBounds, t_blurred_mask . fBounds);
	
	// MW-2013-10-31: [[ Bug 11325 ]] An outer blur with no intersection is just a normal
	//   blur.
	if (!t_overlap && (p_attenuation == kMCGBlurTypeOuter))
		p_attenuation = kMCGBlurTypeNormal;
	
	// Now process the mask according to the attenuation.
	uint8_t *t_old_blurred_mask_fImage;
	t_old_blurred_mask_fImage = t_blurred_mask . fImage;
	switch(p_attenuation)
	{
		case kMCGBlurTypeNormal:
		{
			uint8_t *t_blur_ptr;
			t_blur_ptr = t_blurred_mask . fImage;
			
			// MW-2013-10-31: [[ Bug 11325 ]] Attenuate the mask appropriately, including
			//   applying the opacity.
			for(int y = 0; y < t_blurred_mask . fBounds . height(); y++)
			{
				for(int x = 0; x < t_blurred_mask . fBounds . width(); x++)
					t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
				
				t_blur_ptr += t_blurred_mask . fRowBytes;
			}
		}
		break;
			
		case kMCGBlurTypeInner:
		{
			// We process the generated mask so that the mask consists only of pixels
			// in the intersection of blur-mask and mask.
			if (t_overlap)
			{
				uint8_t *t_blur_ptr, *t_mask_ptr;
				t_blur_ptr = t_blurred_mask . getAddr8(t_inside . x(), t_inside . y());
				t_mask_ptr = p_mask . getAddr8(t_inside . x(), t_inside . y());
				
				// MW-2013-10-31: [[ Bug 11325 ]] Attenuate the mask appropriately, including
				//   applying the opacity.
				for(int y = 0; y < t_inside . height(); y++)
				{
					for(int x = 0; x < t_inside . width(); x++)
					{
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(t_mask_ptr[x]));
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
					}
					
					t_blur_ptr += t_blurred_mask . fRowBytes;
					t_mask_ptr += p_mask . fRowBytes;
				}
				
				t_blurred_mask . fImage = t_blurred_mask . getAddr8(t_inside . x(), t_inside . y());
				t_blurred_mask . fBounds = t_inside;
			}
		}
		break;
			
		case kMCGBlurTypeInvertedInner:
		{
			// We process the generated mask so that the mask consists only of pixels
			// in the intersection of not(blur-mask) and mask.
			SkMask t_tmp_mask;
			t_tmp_mask . fFormat = p_mask . fFormat;
			t_tmp_mask . fBounds = p_mask . fBounds;
			t_tmp_mask . fRowBytes = p_mask . fRowBytes;
			t_tmp_mask . fImage = SkMask::AllocImage(p_mask . computeImageSize());
			
			uint8_t *t_mask_ptr = nullptr;
			uint8_t *t_shape_ptr =
				(p_mask . fImage - p_mask . fBounds . left());
			uint8_t *t_blur_ptr =
				(t_tmp_mask . fImage - t_tmp_mask . fBounds . left());
			if (t_overlap)
			{
				t_mask_ptr = t_blurred_mask . getAddr8(t_inside . x(), t_inside . y());
				t_mask_ptr -= t_inside . left();
			}
			
			// MW-2013-10-31: [[ Bug 11325 ]] Attenuate the mask appropriately, including
			//   applying the opacity.
			for(int y = t_tmp_mask . fBounds . top(); y < t_tmp_mask . fBounds . bottom(); y++)
			{
				if (!t_overlap || y < t_inside . top() || y >= t_inside . bottom())
				{
					for(int x = t_tmp_mask . fBounds . left(); x < t_tmp_mask . fBounds . right(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_shape_ptr[x], SkAlpha255To256(p_color >> 24));
				}
				else
				{
					int x;
					for(x = t_tmp_mask . fBounds . left(); x < t_inside . left(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_shape_ptr[x], SkAlpha255To256(p_color >> 24));
					
					for(x = t_inside . left(); x < t_inside . right(); x++)
					{
						t_blur_ptr[x] = SkAlphaMul(t_shape_ptr[x], SkAlpha255To256(255 - t_mask_ptr[x]));
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
					}
					
					for(x = t_inside . right(); x < t_tmp_mask . fBounds . right(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_shape_ptr[x], SkAlpha255To256(p_color >> 24));
					
					t_mask_ptr += t_blurred_mask . fRowBytes;
				}
				
				t_blur_ptr += t_tmp_mask . fRowBytes;
				t_shape_ptr += p_mask . fRowBytes;
			}
			
			/*if (t_overlap)
			{
				uint8_t *t_blur_ptr, *t_tmp_mask_ptr;
				t_blur_ptr = t_blurred_mask . getAddr8(t_inside . x(), t_inside . y());
				t_tmp_mask_ptr = t_tmp_mask . getAddr8(t_inside . x(), t_inside . y());
				for(int y = 0; y < t_inside . height(); y++)
				{
					for(int x = 0; x < t_inside . width(); x++)
						t_tmp_mask_ptr[x] = SkAlphaMul(255 - t_blur_ptr[x], SkAlpha255To256(t_tmp_mask_ptr[x]));
					
					t_blur_ptr += t_blurred_mask . fRowBytes;
					t_tmp_mask_ptr += t_tmp_mask . fRowBytes;
				}
			}*/
			
			SkMask::FreeImage(t_blurred_mask . fImage);
			
			t_blurred_mask . fImage = t_tmp_mask . fImage;
			t_blurred_mask . fBounds = t_tmp_mask . fBounds;
			t_blurred_mask . fRowBytes = t_tmp_mask . fRowBytes;
			t_old_blurred_mask_fImage = t_blurred_mask . fImage;
		}
		break;
			
		case kMCGBlurTypeOuter:
		{
			// We process the generated mask so that the mask consists only of pixels
			// in the intersection of blur-mask and not(mask).
			
			uint8_t *t_blur_ptr, *t_mask_ptr;
			t_blur_ptr = t_blurred_mask . fImage;
			t_blur_ptr -= t_blurred_mask . fBounds . left();
			t_mask_ptr = p_mask . getAddr8(t_inside . x(), t_inside . y());
			t_mask_ptr -= t_inside . left();
			
			// MW-2013-10-31: [[ Bug 11325 ]] Attenuate the mask appropriately, including
			//   applying the opacity.
			for(int y = t_blurred_mask . fBounds . top(); y < t_blurred_mask . fBounds . bottom(); y++)
			{
				if (y < t_inside . top() || y >= t_inside . bottom())
				{
					for(int x = t_blurred_mask . fBounds . left(); x < t_blurred_mask . fBounds . right(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
				}
				else
				{
					int x;
					for(x = t_blurred_mask . fBounds . left(); x < t_inside . left(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
					
					for(x = t_inside . left(); x < t_inside . right(); x++)
					{
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(255 - t_mask_ptr[x]));
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
					}
					
					for(x = t_inside . right(); x < t_blurred_mask . fBounds . right(); x++)
						t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(p_color >> 24));
					
					t_mask_ptr += p_mask . fRowBytes;
				}
				
				t_blur_ptr += t_blurred_mask . fRowBytes;
			}
		}
		break;
			
		default:
			break;
	}
		
	// MW-2013-10-31: [[ Bug 11325 ]] We fill the mask with a solid color.
	p_color |= 0xff000000;
	
	// Configure the paint.
	SkPaint t_paint;
	t_paint . setStyle(SkPaint::kFill_Style);
	t_paint . setColor(MCGColorToSkColor(p_color));
	//t_paint . setShader(&t_shader);
	
	t_paint.setBlendMode(MCGBlendModeToSkBlendMode(p_blend));

	// Now paint.
	MCGMaskedDeviceFill(*self->layer->canvas, t_blurred_mask, t_paint);
	
	// Free the blurred mask.
	SkMask::FreeImage(t_old_blurred_mask_fImage);
}

static void MCGContextRenderEffects(MCGContextRef self, MCGContextLayerRef p_child, const MCGBitmapEffects& p_effects)
{
	const SkBitmap& t_child_bitmap = p_child -> canvas -> getTopDevice() -> accessBitmap(false);
	
	SkMask t_child_mask;
	t_child_mask . fFormat = SkMask::kA8_Format;
	t_child_mask . fBounds . set(p_child -> origin_x, p_child -> origin_y, p_child -> origin_x + t_child_bitmap . width(), p_child -> origin_y + t_child_bitmap . height());
	t_child_mask . fRowBytes = t_child_mask . fBounds . width();
	t_child_mask . fImage = SkMask::AllocImage(t_child_mask . computeImageSize());
	for(int y = 0; y < t_child_bitmap . height(); y++)
		for(int x = 0; x < t_child_bitmap . width(); x++)
			t_child_mask . fImage[y * t_child_mask . fRowBytes + x] = *(((uint32_t *)t_child_bitmap . getPixels()) + y * t_child_bitmap . rowBytes() / 4 + x) >> 24;
	
	if (p_effects . has_drop_shadow)
		MCGContextRenderEffect(self,
							   t_child_mask,
							   MCGSizeMake(p_effects . drop_shadow . size, p_effects . drop_shadow . size),
							   MCGSizeMake(p_effects . drop_shadow . x_offset, p_effects . drop_shadow . y_offset),
							   p_effects . drop_shadow . spread,
							   p_effects . drop_shadow . knockout ? kMCGBlurTypeOuter : kMCGBlurTypeNormal,
							   p_effects . drop_shadow . color,
							   p_effects . drop_shadow . blend_mode);
	
	if (p_effects . has_outer_glow)
		MCGContextRenderEffect(self,
							   t_child_mask,
							   MCGSizeMake(p_effects . outer_glow . size, p_effects . outer_glow . size),
							   MCGSizeMake(0.0, 0.0),
							   p_effects . outer_glow . spread,
							   kMCGBlurTypeNormal,
							   p_effects . outer_glow . color,
							   p_effects . outer_glow . blend_mode);
							   
	if (!p_effects . isolated)
	{
		// Render the layer itself (using the layer's alpha and blend mode - well, if we can agree that's a good change!).
		self->layer->canvas->save();
		self->layer->canvas->resetMatrix();
		self->layer->canvas->drawBitmap(t_child_bitmap, p_child->origin_x, p_child->origin_y, NULL);
		self->layer->canvas->restore();
	}
	
	if (p_effects . has_inner_shadow)
		MCGContextRenderEffect(self,
							   t_child_mask,
							   MCGSizeMake(p_effects . inner_shadow . size, p_effects . inner_shadow . size),
							   MCGSizeMake(p_effects . inner_shadow . x_offset, p_effects . inner_shadow . y_offset),
							   p_effects . inner_shadow . spread,
							   kMCGBlurTypeInvertedInner,
							   p_effects . inner_shadow . color,
							   p_effects . inner_shadow . blend_mode);
	
	if (p_effects . has_inner_glow)
		MCGContextRenderEffect(self,
							   t_child_mask,
							   MCGSizeMake(p_effects . inner_glow . size, p_effects . inner_glow . size),
							   MCGSizeMake(0.0, 0.0),
							   p_effects . inner_glow . spread,
							   p_effects . inner_glow . inverted ? kMCGBlurTypeInvertedInner : kMCGBlurTypeInner,
							   p_effects . inner_glow . color,
							   p_effects . inner_glow . blend_mode);
	
	if (p_effects . has_color_overlay)
	{
		MCGColor t_color;
		t_color = p_effects . color_overlay . color;
		
		// MW-2013-10-31: [[ Bug 11325 ]] Apply the opacity to the mask.
		uint8_t *t_blur_ptr;
		t_blur_ptr = t_child_mask . fImage;
		for(int y = 0; y < t_child_mask . fBounds . height(); y++)
		{
			for(int x = 0; x < t_child_mask . fBounds . width(); x++)
				t_blur_ptr[x] = SkAlphaMul(t_blur_ptr[x], SkAlpha255To256(t_color >> 24));
			
			t_blur_ptr += t_child_mask . fRowBytes;
		}
		
		t_color |= 0xff000000;
		
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);
		t_paint . setColor(MCGColorToSkColor(t_color));
		
		t_paint.setBlendMode(MCGBlendModeToSkBlendMode(p_effects.color_overlay.blend_mode));

		MCGMaskedDeviceFill(*self->layer->canvas, t_child_mask, t_paint);
	}
	
	SkMask::FreeImage(t_child_mask . fImage);
}

void MCGContextEnd(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
		
	if (self -> layer -> nesting > 0)
	{
		//MCGContextRestore(self);
		self -> layer -> nesting -= 1;
		return;
	}
	
	if (self -> layer -> parent == nil)
		return;
		
	// Pop pack the attribute stack to the top state for the previous layer.
	while(!self -> state -> is_layer_begin_pt)
		MCGContextPopState(self);
	self -> state -> is_layer_begin_pt = false;
	
	MCGContextLayerRef t_child_layer;
	t_child_layer = self -> layer;
	self -> layer = self -> layer -> parent;
	
	SkPaint t_paint;
	t_paint . setAlpha((U8CPU)(self -> state -> opacity * 255));
	
	t_paint.setBlendMode(MCGBlendModeToSkBlendMode(self->state->blend_mode));
	
	if (t_child_layer -> has_effects)
	{
		bool t_in_layer;
		t_in_layer = false;	
		if (self -> state -> opacity != 1.0f || self -> state -> blend_mode != kMCGBlendModeSourceOver)
		{
			self -> layer -> canvas -> saveLayer(NULL, &t_paint);
			t_in_layer = true;
		}
		MCGContextRenderEffects(self, t_child_layer, t_child_layer -> effects);
		if (t_in_layer)
			self -> layer -> canvas -> restore();
	}
	else if (t_child_layer->m_surface == nil)
	{
		// Software-backed layer
		const SkBitmap& t_child_bitmap = t_child_layer->canvas->getTopDevice()->accessBitmap(false);
		self->layer->canvas->save();
		self->layer->canvas->resetMatrix();
		self->layer->canvas->drawBitmap(t_child_bitmap, t_child_layer->origin_x, t_child_layer->origin_y, &t_paint);
		self->layer->canvas->restore();
	}
	else
	{
		// Hardware-backed layer - copy it into RAM
		self->layer->canvas->save();
		self->layer->canvas->resetMatrix();
		t_child_layer->m_surface->draw(self->layer->canvas, t_child_layer->origin_x, t_child_layer->origin_y, &t_paint);
		self->layer->canvas->restore();
	}
	
	MCGContextLayerDestroy(t_child_layer);
}

void MCGContextSetOpacity(MCGContextRef self, MCGFloat p_opacity)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> opacity = MCClamp(p_opacity, 0.0f, 1.0f);
}

void MCGContextSetBlendMode(MCGContextRef self, MCGBlendMode p_mode)
{	
	if (!MCGContextIsValid(self))
		return;

	self -> state -> blend_mode = p_mode;	
}

// IM-2013-09-03: [[ RefactorGraphics ]] add new function to replace the context clip rect
void MCGContextSetClipToRect(MCGContextRef self, MCGRectangle p_rect)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> layer ->canvas -> clipRect(MCGRectangleToSkRect(p_rect), SkRegion::kReplace_Op, self -> state -> should_antialias);
}

void MCGContextClipToRect(MCGContextRef self, MCGRectangle p_rect)
{
	if (!MCGContextIsValid(self))
		return;
	
	// IM-2013-09-03: [[ RefactorGraphics ]] revert clipping region op to intersect
	
	// we use skia to manage the clip entirely rather than storing in state
	// this means any transforms to the clip are handled by skia allowing for more complex clips (rather than just a rect)
	self -> layer -> canvas -> clipRect(MCGRectangleToSkRect(p_rect), SkRegion::kIntersect_Op, self -> state -> should_antialias);
}

void MCGContextClipToPath(MCGContextRef self, MCGPathRef p_path)
{
    if (!MCGContextIsValid(self) || !MCGPathIsValid(p_path))
        return;
    
    self -> layer -> canvas -> clipPath(*p_path -> path, SkRegion::kIntersect_Op, self -> state -> should_antialias);
}

void MCGContextSetClipToDeviceRegion(MCGContextRef self, MCGRegionRef p_region)
{
	if (!MCGContextIsValid(self) || p_region == nil)
		return;
	
	self->layer->canvas->clipRegion(p_region->region, SkRegion::kReplace_Op);
}

void MCGContextClipToDeviceRegion(MCGContextRef self, MCGRegionRef p_region)
{
	if (!MCGContextIsValid(self) || p_region == nil)
		return;
	
	self -> layer -> canvas -> clipRegion(p_region->region, SkRegion::kIntersect_Op);
}

void MCGContextClipToRegion(MCGContextRef self, MCGRegionRef p_region)
{
	MCGAffineTransform t_transform;
	t_transform = MCGContextGetDeviceTransform(self);
	
	MCGRegionRef t_transformed;
	t_transformed = nil;
	
	if (MCGAffineTransformIsIdentity(t_transform))
	{
		MCGContextClipToDeviceRegion(self, p_region);
		return;
	}
	
	/* UNCHECKED */ MCGRegionCopyWithTransform(p_region, MCGContextGetDeviceTransform(self), t_transformed);
	
	MCGContextClipToDeviceRegion(self, t_transformed);
	
	MCGRegionDestroy(t_transformed);
}

MCGRectangle MCGContextGetClipBounds(MCGContextRef self)
{	
	// MM-2013-11-25: [[ Bug 11496 ]] When using getClipBounds, Skia outsets the clip by 1 pixel to take account of antialiasing.
	//  This is worked out by get the device clip, outsetting and revesing the transform on the canvas, meanining if there is a scale factor, the outset is scaled.
	//  Instead, we just fetch the device clip and reverse the transform ourselves.
	SkIRect t_dev_i_clip;
	self -> layer -> canvas -> getClipDeviceBounds(&t_dev_i_clip);	
	SkRect t_dev_clip;
	t_dev_clip = SkRect::MakeLTRB(SkIntToScalar(t_dev_i_clip . left()), SkIntToScalar(t_dev_i_clip . top()), SkIntToScalar(t_dev_i_clip . right()), SkIntToScalar(t_dev_i_clip . bottom()));
	
	const SkMatrix t_transfom = self -> layer -> canvas -> getTotalMatrix();
	SkMatrix t_inverse_transform;
	if (t_transfom . invert(&t_inverse_transform))
	{
		SkRect t_clip;
		t_inverse_transform . mapRect(&t_clip, t_dev_clip);
		return MCGRectangleMake(t_clip . x(), t_clip . y(), t_clip . width(), t_clip . height());
	}
	else
		return MCGRectangleMake(0.0f, 0.0f, 0.0f, 0.0f);
}

MCGRectangle MCGContextGetDeviceClipBounds(MCGContextRef self)
{
	SkIRect t_clip;
	self -> layer -> canvas -> getClipDeviceBounds(&t_clip);
	return MCGRectangleMake(t_clip . x(), t_clip . y(), t_clip . width(), t_clip . height());
}

////////////////////////////////////////////////////////////////////////////////

static inline void
MCGContextSetPaintAndRelease(MCGContextRef self, MCGPaintRef p_paint, MCGPaintRef& x_target)
{
    if (!self->is_valid)
    {
        MCGRelease(p_paint);
        return;
    }
    
    MCGAssignAndRelease(x_target, p_paint);
}

static inline void
MCGContextSetNonePaint(MCGContextRef self, MCGPaintRef& x_target)
{
    MCGContextSetPaintAndRelease(self, nullptr, x_target);
}

static inline void
MCGContextSetRGBAColorPaint(MCGContextRef self, MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCGPaintRef& x_target)
{
    MCGPaintRef t_solid_color = nullptr;
    if (!MCGPaintCreateWithSolidColor(p_red, p_green, p_blue, p_alpha, t_solid_color))
    {
        self->is_valid = false;
        return;
    }
    MCGContextSetPaintAndRelease(self, t_solid_color, x_target);
}

static inline void
MCGContextSetPatternPaint(MCGContextRef self, MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPaintRef& x_target)
{
    MCGPaintRef t_pattern = nullptr;
    if (!MCGPaintCreateWithPattern(p_image, p_transform, p_filter, t_pattern))
    {
        self->is_valid = false;
        return;
    }
    MCGContextSetPaintAndRelease(self, t_pattern, x_target);
}

static inline void
MCGContextSetGradientPaint(MCGContextRef self, MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPaintRef& x_target)
{
    MCGPaintRef t_gradient = nullptr;
    if (!MCGPaintCreateWithGradient(p_function, p_stops, p_colors, p_ramp_length, p_mirror, p_wrap, p_repeats, p_transform, p_filter, t_gradient))
    {
        self->is_valid = false;
        return;
    }
    MCGContextSetPaintAndRelease(self, t_gradient, x_target);
}

////////////////////////////////////////////////////////////////////////////////
// Fill attributes

void MCGContextSetFillRule(MCGContextRef self, MCGFillRule p_rule)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> fill_rule = p_rule;	
}

void MCGContextSetFillOpacity(MCGContextRef self, MCGFloat p_opacity)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> fill_opacity = MCClamp(p_opacity, 0.0f, 1.0f);
}

void MCGContextSetFillPaint(MCGContextRef self, MCGPaintRef p_paint)
{
    MCGRetain(p_paint);
    MCGContextSetPaintAndRelease(self, p_paint, self->state->fill_paint);
}

void MCGContextSetFillNone(MCGContextRef self)
{
    MCGContextSetNonePaint(self, self->state->fill_paint);
}

void MCGContextSetFillRGBAColor(MCGContextRef self, MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{
    MCGContextSetRGBAColorPaint(self, p_red, p_green, p_blue, p_alpha, self->state->fill_paint);
}

void MCGContextSetFillPattern(MCGContextRef self, MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
    MCGContextSetPatternPaint(self, p_image, p_transform, p_filter, self->state->fill_paint);
}

void MCGContextSetFillGradient(MCGContextRef self, MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
    MCGContextSetGradientPaint(self, p_function, p_stops, p_colors, p_ramp_length, p_mirror, p_wrap, p_repeats, p_transform, p_filter, self->state->fill_paint);
}

void MCGContextSetFillPaintStyle(MCGContextRef self, MCGPaintStyle p_paint_style)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> fill_style = p_paint_style;
}

////////////////////////////////////////////////////////////////////////////////
// Stroke attributes

void MCGContextSetStrokeOpacity(MCGContextRef self, MCGFloat p_opacity)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_opacity = MCClamp(p_opacity, 0.0f, 1.0f);
}

void MCGContextSetStrokePaint(MCGContextRef self, MCGPaintRef p_paint)
{
    MCGRetain(p_paint);
    MCGContextSetPaintAndRelease(self, p_paint, self->state->stroke_paint);
}

void MCGContextSetStrokeNone(MCGContextRef self)
{
    MCGContextSetNonePaint(self, self->state->stroke_paint);
}

void MCGContextSetStrokeRGBAColor(MCGContextRef self, MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{	
    MCGContextSetRGBAColorPaint(self, p_red, p_green, p_blue, p_alpha, self->state->stroke_paint);
}

void MCGContextSetStrokePattern(MCGContextRef self, MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
    MCGContextSetPatternPaint(self, p_image, p_transform, p_filter, self->state->stroke_paint);
}

void MCGContextSetStrokeGradient(MCGContextRef self, MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
    MCGContextSetGradientPaint(self, p_function, p_stops, p_colors, p_ramp_length, p_mirror, p_wrap, p_repeats, p_transform, p_filter, self->state->stroke_paint);
}

void MCGContextSetStrokeWidth(MCGContextRef self, MCGFloat p_width)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_attr . width = p_width;
}

void MCGContextSetStrokeMiterLimit(MCGContextRef self, MCGFloat p_limit)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_attr . miter_limit = p_limit;
}

void MCGContextSetStrokeJoinStyle(MCGContextRef self, MCGJoinStyle p_style)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_attr . join_style = p_style;
}

void MCGContextSetStrokeCapStyle(MCGContextRef self, MCGCapStyle p_style)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_attr . cap_style = p_style;
}

void MCGContextSetStrokeDashOffset(MCGContextRef self, MCGFloat p_offset)
{
	if (self->state->stroke_attr.dashes != nullptr)
	{
		MCGContextSetStrokeDashes(self,
								  p_offset,
								  self->state->stroke_attr.dashes->lengths,
								  self->state->stroke_attr.dashes->count);
	}
	else
	{
		MCGContextSetStrokeDashes(self, p_offset, nullptr, 0);
	}
}

void MCGContextSetStrokeDashArray(MCGContextRef self, const MCGFloat *p_lengths, uindex_t p_arity)
{
    MCGContextSetStrokeDashes(self, self->state->stroke_attr.dashes != nullptr ? self->state->stroke_attr.dashes->phase : 0, p_lengths, p_arity);
}

void MCGContextSetStrokeDashes(MCGContextRef self, MCGFloat p_phase, const MCGFloat *p_lengths, uindex_t p_arity)
{
	if (!MCGContextIsValid(self))
		return;

	bool t_success = true;

	MCGDashesRef t_dashes = nullptr;
	if (t_success)
	{
		t_success = MCGDashesCreate(p_phase, p_lengths, p_arity, t_dashes);
	}

	if (t_success)
    {
		MCGDashesRelease(self -> state -> stroke_attr . dashes);
		self -> state -> stroke_attr . dashes = t_dashes;
    }

	self -> is_valid = t_success;
}

void MCGContextSetStrokePaintStyle(MCGContextRef self, MCGPaintStyle p_paint_style)
{
	if (!MCGContextIsValid(self))
		return;

	self -> state -> stroke_style = p_paint_style;
}

////////////////////////////////////////////////////////////////////////////////
// Transforms - concatenated with the current CTM.

inline static bool MCGContextSetCTM(MCGContextRef self, MCGAffineTransform p_transform)
{
    self->state->transform = p_transform;
	
	SkMatrix t_matrix;
	MCGAffineTransformToSkMatrix(MCGAffineTransformConcat(self->state->base_transform, p_transform), t_matrix);
	self -> layer -> canvas -> setMatrix(t_matrix);
    
	// no need to transform the clip at this point as this is handled internally by skia
	return true;
}

void MCGContextSetTransform(MCGContextRef self, MCGAffineTransform p_transform)
{
	if (!MCGContextIsValid(self))
		return;
    
    MCGContextSetCTM(self, p_transform);
}

void MCGContextConcatCTM(MCGContextRef self, MCGAffineTransform p_transform)
{
	if (!MCGContextIsValid(self))
		return;
	
    MCGContextSetCTM(self, MCGAffineTransformConcat(self->state->transform, p_transform));
}

void MCGContextRotateCTM(MCGContextRef self, MCGFloat p_angle)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGContextConcatCTM(self, MCGAffineTransformMakeRotation(p_angle));
}

void MCGContextTranslateCTM(MCGContextRef self, MCGFloat p_xoffset, MCGFloat p_yoffset)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGContextConcatCTM(self, MCGAffineTransformMakeTranslation(p_xoffset, p_yoffset));
}

void MCGContextScaleCTM(MCGContextRef self, MCGFloat p_xscale, MCGFloat p_yscale)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGContextConcatCTM(self, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCGContextResetCTM(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> is_valid = MCGContextSetCTM(self, MCGAffineTransformMakeIdentity());	
}

MCGAffineTransform MCGContextGetDeviceTransform(MCGContextRef self)
{
	MCGAffineTransform t_transform;
	MCGAffineTransformFromSkMatrix(self -> layer -> canvas -> getTotalMatrix(), t_transform);
	return t_transform;
}

////////////////////////////////////////////////////////////////////////////////
// Path primitives

static void MCGContextEnsurePath(MCGContextRef self)
{
	if (self -> path == NULL)
		MCGContextBeginPath(self);
}

void MCGContextBeginPath(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = self -> path == NULL;
	
	if (t_success)
		t_success = MCGPathCreateMutable(self -> path);
	
	self -> is_valid = t_success;	
}

void MCGContextMoveTo(MCGContextRef self, MCGPoint p_end_point)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathMoveTo(self -> path, p_end_point);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextLineTo(MCGContextRef self, MCGPoint p_end_point)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathLineTo(self -> path, p_end_point);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextQuadraticTo(MCGContextRef self, MCGPoint p_control_point, MCGPoint p_end_point)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathQuadraticTo(self -> path, p_control_point, p_end_point);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextCubicTo(MCGContextRef self, MCGPoint p_first_control_point, MCGPoint p_second_control_point, MCGPoint p_end_point)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathCubicTo(self -> path, p_first_control_point, p_second_control_point, p_end_point);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;	
}

void MCGContextArcTo(MCGContextRef self, MCGSize p_radii, MCGFloat p_rotation, bool p_large_arc, bool p_sweep, MCGPoint end_point)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathArcTo(self -> path, p_radii, p_rotation, p_large_arc, p_sweep, end_point);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextCloseSubpath(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathCloseSubpath(self -> path);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

////////////////////////////////////////////////////////////////////////////////
// Shape primitives - these add to the current path.

void MCGContextAddRectangle(MCGContextRef self, MCGRectangle p_bounds)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddRectangle(self -> path, p_bounds);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;	
}

void MCGContextAddRoundedRectangle(MCGContextRef self, MCGRectangle p_bounds, MCGSize p_corner_radii)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddRoundedRectangle(self -> path, p_bounds, p_corner_radii);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;
}

void MCGContextAddEllipse(MCGContextRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddEllipse(self -> path, p_center, p_radii, p_rotation);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;
}

void MCGContextAddArc(MCGContextRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddArc(self -> path, p_center, p_radii, p_rotation, p_start_angle, p_finish_angle);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;
}

void MCGContextAddSector(MCGContextRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddSector(self -> path, p_center, p_radii, p_rotation, p_start_angle, p_finish_angle);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;	
}

void MCGContextAddSegment(MCGContextRef self, MCGPoint p_center, MCGSize p_radii, MCGFloat p_rotation, MCGFloat p_start_angle, MCGFloat p_finish_angle)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddSegment(self -> path, p_center, p_radii, p_rotation, p_start_angle, p_finish_angle);
		t_success = MCGPathIsValid(self -> path);
	}
	
	self -> is_valid = t_success;	
}

void MCGContextAddLine(MCGContextRef self, MCGPoint p_start, MCGPoint p_finish)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddLine(self -> path, p_start, p_finish);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;
}

void MCGContextAddPolygon(MCGContextRef self, const MCGPoint *p_points, uindex_t p_arity)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddPolygon(self -> path, p_points, p_arity);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;
}

void MCGContextAddPolyline(MCGContextRef self, const MCGPoint *p_points, uindex_t p_arity)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddPolyline(self -> path, p_points, p_arity);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextAddDot(MCGContextRef self, MCGPoint p_location)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{		
		// MM-2013-11-25: [[ Bug 11395 ]] Updated dot drawing to be half pixel rectangle.
		MCGContextEnsurePath(self);
		MCGPathAddRectangle(self -> path, MCGRectangleMake(p_location . x, p_location. y, 0.25f, 0.25f));
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

void MCGContextAddPath(MCGContextRef self, MCGPathRef p_path)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGContextEnsurePath(self);
		MCGPathAddPath(self -> path, p_path);
		t_success = MCGPathIsValid(self -> path);
	}	
	
	self -> is_valid = t_success;	
}

////////////////////////////////////////////////////////////////////////////////
// Paint setup

static bool
MCGContextSetupCommon(MCGContextRef self, MCGFloat p_opacity, MCGPaintRef p_paint, MCGPaintStyle p_paint_style, SkPaint &r_paint)
{
    if (!p_paint->Apply(r_paint))
    {
        return false;
    }
    
    switch(p_paint_style)
    {
    case kMCGPaintStyleOpaque:
        break;
    case kMCGPaintStyleStippled:
        sk_sp<SkMaskFilter> t_stipple(new (nothrow) SkStippleMaskFilter());
        if (t_stipple == nullptr)
        {
            return false;
        }
        r_paint.setMaskFilter(t_stipple);
        break;
    }
    
    r_paint.setAlpha((U8CPU)(r_paint.getAlpha() * p_opacity));
    r_paint.setAntiAlias(self -> state -> should_antialias);
    r_paint.setBlendMode(MCGBlendModeToSkBlendMode(self->state->blend_mode));
    
    return true;
}
    

bool MCGContextSetupFill(MCGContextRef self, SkPaint &r_paint)
{
    if (!MCGContextSetupCommon(self, self->state->fill_opacity, self->state->fill_paint, self->state->fill_style, r_paint))
    {
        return false;
    }
	
    r_paint . setStyle(SkPaint::kFill_Style);
	
    return true;
}

bool MCGContextSetupStroke(MCGContextRef self, SkPaint &r_paint)
{
    if (!MCGContextSetupCommon(self, self->state->stroke_opacity, self->state->stroke_paint, self->state->stroke_style, r_paint))
    {
        return false;
    }
    
    if (self -> state -> stroke_attr . dashes != NULL)
    {
        sk_sp<SkPathEffect> t_dash_effect;
        if (!MCGDashesToSkDashPathEffect(self -> state -> stroke_attr . dashes, t_dash_effect))
        {
            return false;
        }
        r_paint.setPathEffect(t_dash_effect);
    }
        
    r_paint . setStrokeWidth(MCGFloatToSkScalar(self -> state -> stroke_attr . width));
    r_paint . setStrokeMiter(MCGFloatToSkScalar(self -> state -> stroke_attr . miter_limit));
    r_paint . setStrokeJoin(MCGJoinStyleToSkJoinStyle(self -> state -> stroke_attr . join_style));
    r_paint . setStrokeCap(MCGCapStyleToSkCapStyle(self -> state -> stroke_attr . cap_style));
    
    r_paint . setStyle(SkPaint::kStroke_Style);
	
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Operations

static bool MCGContextFillPath(MCGContextRef self, MCGPathRef p_path)
{
    if (self->state->fill_paint == nullptr)
    {
        return true;
    }
    
	SkPaint t_paint;
	if (!MCGContextSetupFill(self, t_paint))
    {
        return false;
    }

    // should probably be careful of setting the fill type and path mutability here
    p_path -> path -> setFillType(MCGFillRuleToSkFillType(self -> state -> fill_rule));
    self -> layer -> canvas -> drawPath(*p_path -> path, t_paint);

    return true;
}

static bool MCGContextStrokePath(MCGContextRef self, MCGPathRef p_path)
{	
    if (self->state->stroke_paint == nullptr)
    {
        return true;
    }
		
	SkPaint t_paint;
	if (!MCGContextSetupStroke(self, t_paint))
    {
        return false;
    }

	self -> layer -> canvas -> drawPath(*p_path -> path, t_paint);
		
	return true;
}

// Copy the current path as an (immutable) path object. If the context is invalid, or
// the path could not be copied the (empty) error path is returned. Note that failure
// to copy the path does *not* cause the context to become invalid, rather the returned
// path is.
void MCGContextCopyPath(MCGContextRef self, MCGPathRef& r_path)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		MCGPathCopy(self -> path, r_path);
		t_success = MCGPathIsValid(r_path);
	}
	
	self -> is_valid = t_success;
}

// Fill the current path using the current paint and fill rule. This discards the path.
void MCGContextFill(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
    if (self -> path == nil)
        return;
    
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCGContextFillPath(self, self -> path);
	
	if (t_success)
	{
		MCGPathRelease(self -> path);
		self -> path = NULL;
	}
	
	self -> is_valid = t_success;	
}

// Thicken the current path using the current stroke attributes and fill the resulting
// path using the non-zero rule. This discards the path.
void MCGContextStroke(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
    if (self -> path == nil)
        return;
    
	bool t_success;
	t_success = true;
    
	if (t_success)
		t_success = MCGContextStrokePath(self, self -> path);
	
	if (t_success)
	{
		MCGPathRelease(self -> path);
		self -> path = NULL;
	}	
	
	self -> is_valid = t_success;
}

// Fills the current path using the current paint and fill rule. Then thicken the current
// path using the current stroke attributes and fill the resulting path using the non-zero
// rule. This discards the path.
void MCGContextFillAndStroke(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
    if (self -> path == nil)
        return;
    
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCGContextFillPath(self, self -> path);
	
	if (t_success)
		t_success = MCGContextStrokePath(self, self -> path);
	
	if (t_success)
	{
		MCGPathRelease(self -> path);
		self -> path = NULL;
	}
	
	self -> is_valid = t_success;
}

// Intersects the current clipping path with the current path; the inside of the current
// path is determined with the current fill rule. This discards the path.
void MCGContextClip(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	self -> path -> path -> setFillType(MCGFillRuleToSkFillType(self -> state -> fill_rule));
	self -> layer -> canvas -> clipPath(*self -> path -> path, SkRegion::kIntersect_Op, self -> state -> should_antialias);
	MCGPathRelease(self -> path);
	self -> path = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCGContextDrawSkBitmap(MCGContextRef self, const SkBitmap &p_bitmap, const MCGRectangle *p_center, const MCGRectangle *p_src, const MCGRectangle &p_dst, MCGImageFilter p_filter)
{
	bool t_success;
	t_success = true;
	
	SkPaint t_paint;
	if (t_success)
	{
		if (p_bitmap . colorType() == SkColorType::kAlpha_8_SkColorType)
			t_success = MCGContextSetupFill(self, t_paint);
		else
		{			
			t_paint . setAlpha((U8CPU) (self -> state -> opacity * 255));
			t_paint . setBlendMode(MCGBlendModeToSkBlendMode(self->state->blend_mode));
		}
	}

	if (t_success)
	{
		// MM-2014-03-12: [[ Bug 11892 ]] If we are not transforming the image, there's no need to apply any filtering.
		//  Was causing issues in Skia with non null blend modes.

		// IM-2014-06-19: [[ Bug 12557 ]] More rigourous check to see if we need a filter - Skia will complain if a filter
		// is used for translation-only transforms or scaling transforms close to the identity.
		SkMatrix t_matrix;
		t_matrix = self->layer->canvas->getTotalMatrix();

		MCGRectangle t_tmp_src;
		if (p_src != nil)
			t_tmp_src = *p_src;
		else
			t_tmp_src = MCGRectangleMake(0, 0, p_bitmap.width(), p_bitmap.height());

		// preconcat the current transform with the transformation from source -> dest rect to obtain the total transformation.
		SkMatrix t_rect_to_rect;
		t_rect_to_rect.setRectToRect(MCGRectangleToSkRect(t_tmp_src), MCGRectangleToSkRect(p_dst), SkMatrix::kFill_ScaleToFit);

		t_matrix.preConcat(t_rect_to_rect);

		bool t_no_filter = false;

		// Translation only - no filter
		if ((t_matrix.getType() & ~SkMatrix::kTranslate_Mask) == 0)
			t_no_filter = true;
		else if ((t_matrix.getType() & ~(SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) == 0)
		{
			// Translate and scale - if the transformed & rounded destination is the same size as the source then don't filter
			SkRect src, dst;
			src.setXYWH(t_tmp_src.origin.x, t_tmp_src.origin.y, t_tmp_src.size.width, t_tmp_src.size.height);

			t_matrix.mapPoints(SkTCast<SkPoint*>(&dst),
				SkTCast<const SkPoint*>(&src),
				2);

			SkIRect idst;
			dst.round(&idst);
			t_no_filter = idst.width() == t_tmp_src.size.width && idst.height() == t_tmp_src.size.height;
		}

		if (t_no_filter)
		{
			t_paint . setAntiAlias(false);
			t_paint . setFilterQuality(SkFilterQuality::kNone_SkFilterQuality);
		}
		else
		{
			// MM-2014-01-09: [[ ImageFilterUpdate ]] Updated filters to use Skia's new filter levels.
			switch (p_filter)
			{
				case kMCGImageFilterNone:
					t_paint . setAntiAlias(false);
					t_paint . setFilterQuality(SkFilterQuality::kNone_SkFilterQuality);
					break;
				case kMCGImageFilterLow:
					t_paint . setAntiAlias(false);
					t_paint . setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
					break;
				case kMCGImageFilterMedium:
					t_paint . setAntiAlias(true);
					t_paint . setFilterQuality(SkFilterQuality::kMedium_SkFilterQuality);
					break;
				case kMCGImageFilterHigh:
					t_paint . setAntiAlias(true);
					t_paint  .setFilterQuality(SkFilterQuality::kHigh_SkFilterQuality);
					break;
			}
		}
		
		SkRect t_src_rect = SkRect::Make(p_bitmap.bounds());
		if (p_src != nil)
		{
			t_src_rect = MCGRectangleToSkRect(*p_src);
		}

        if (p_src != nil || p_center == nil)
            self -> layer -> canvas -> drawBitmapRect(p_bitmap, t_src_rect, MCGRectangleToSkRect(p_dst), &t_paint);
        else
        {
            SkIRect t_center;
            t_center . fLeft = ceilf(p_center -> origin . x);
            t_center . fTop = ceilf(p_center -> origin . y);
            t_center . fRight = floorf(p_center -> origin . x + p_center -> size . width);
            t_center . fBottom = floorf(p_center -> origin . y + p_center -> size . height);
            self -> layer -> canvas -> drawBitmapNine(p_bitmap, t_center, MCGRectangleToSkRect(p_dst), &t_paint);
        }
	}
	
	return t_success;
}

void MCGContextDrawImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_dst, MCGImageFilter p_filter)
{
	if (!MCGImageIsValid(p_image))
		return;

	self -> is_valid = MCGContextDrawSkBitmap(self, *p_image->bitmap, nil, nil, p_dst, p_filter);
}

void MCGContextDrawImageWithCenter(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_center, MCGRectangle p_dst, MCGImageFilter p_filter)
{
	if (!MCGImageIsValid(p_image))
		return;
    
	self -> is_valid = MCGContextDrawSkBitmap(self, *p_image->bitmap, &p_center, nil, p_dst, p_filter);
}
void MCGContextDrawRectOfImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_src, MCGRectangle p_dst, MCGImageFilter p_filter)
{
	if (!MCGImageIsValid(p_image))
		return;

	self -> is_valid = MCGContextDrawSkBitmap(self, *p_image->bitmap, nil, &p_src, p_dst, p_filter);
}

void MCGContextDrawPixels(MCGContextRef self, const MCGRaster& p_raster, MCGRectangle p_dst, MCGImageFilter p_filter)
{
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
	
	SkBitmap t_bitmap;
	if (t_success)
		t_success = MCGRasterToSkBitmap(p_raster, kMCGPixelOwnershipTypeBorrow, t_bitmap);

	if (t_success)
		t_success = MCGContextDrawSkBitmap(self, t_bitmap, nil, nil, p_dst, p_filter);
	
	self -> is_valid = t_success;
}

////////////////////////////////////////////////////////////////////////////////

static MCGCacheTableRef s_measure_cache = NULL;

void MCGTextMeasureCacheInitialize(void)
{
    // MM-2014-01-09: [[ Bug 11623 ]] Make sure we initialise globals otherwise old values will be present on Android after restart.
    s_measure_cache = NULL;
	srand(time(NULL));
	/* UNCHECKED */ MCGCacheTableCreate(kMCGTextMeasureCacheTableSize, kMCGTextMeasureCacheMaxOccupancy, kMCGTextMeasureCacheByteSize, s_measure_cache);
}

void MCGTextMeasureCacheFinalize(void)
{
	MCGCacheTableDestroy(s_measure_cache);
    s_measure_cache = NULL;
}

void MCGTextMeasureCacheCompact(void)
{
	MCGCacheTableCompact(s_measure_cache);
}

// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
//  The transform is used when we are measuring text with the intention of drawing it scaled. The width is still returned in logical units.
//  (i.e. we measure the text as if scaled, the transform back to logical units - needed where text doesn't scale linearly).
MCGFloat MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
	if (p_length == 0 || p_text == NULL)
		return 0.0;
	
	if (p_length >= kMCGTextMeasureCacheMaxStringLength)
		return __MCGContextMeasurePlatformText(self, p_text, p_length, p_font, p_transform);
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = s_measure_cache != NULL;
	
	void *t_key;
	t_key = NULL;
	uint32_t t_key_length;
	if (t_success)
	{
        // MM-2014-06-02: [[ CoreText ]] We no no longer need to store the style - was only needed by Mac/ATSUI.
		t_key_length = p_length + sizeof(p_length) + sizeof(p_font . fid) + sizeof(p_font . size) + sizeof(p_transform . a) * 4;
		t_success = MCMemoryNew(t_key_length, t_key);
	}
	
	if (t_success)
	{
		uint8_t *t_key_ptr;
		t_key_ptr = (uint8_t *)t_key;
		
		MCMemoryCopy(t_key_ptr, p_text, p_length);
		t_key_ptr += p_length;

		MCMemoryCopy(t_key_ptr, &p_length, sizeof(p_length));
		t_key_ptr += sizeof(p_length);

		MCMemoryCopy(t_key_ptr, &p_font . fid, sizeof(p_font . fid));
		t_key_ptr += sizeof(p_font . fid);

		// MW-2013-11-07: [[ Bug 11393 ]] Make sure we take into account the 'ideal' flag
		//   when looking up metrics.
		// MW-2013-11-12: [[ Bug 11415 ]] Make sure we use p_font.size and not t_size if not
		//   ideal - otherwise uninitialized values abound...
		uint16_t t_size;
		t_size = p_font . size & 0x3fff;
		if (p_font . ideal)
			t_size |= 1 << 15;
		// MW-2013-12-05: [[ Bug 11535 ]] Set the bit to indicate its got a fixed advance
		//   (fixed advance never varies for a given font at the moment, so this is safe).
		if (p_font . fixed_advance != 0)
			t_size |= 1 << 14;
		MCMemoryCopy(t_key_ptr, &t_size, sizeof(p_font . size));
		t_key_ptr += sizeof(p_font . size);

		// MM-2014-04-16: [[ Bug 11964 ]] Store the scale of the transform in the key.
		// Don't store translation component of transform as it has no effect on bounds
		MCMemoryCopy(t_key_ptr, &p_transform . a, sizeof(p_transform . a));
		t_key_ptr += sizeof(p_transform . a);
		MCMemoryCopy(t_key_ptr, &p_transform . b, sizeof(p_transform . b));
		t_key_ptr += sizeof(p_transform . b);
		MCMemoryCopy(t_key_ptr, &p_transform . c, sizeof(p_transform . c));
		t_key_ptr += sizeof(p_transform . c);
		MCMemoryCopy(t_key_ptr, &p_transform . d, sizeof(p_transform . d));
		t_key_ptr += sizeof(p_transform . d);
		
		MCGFloat *t_width_ptr;
		t_width_ptr = (MCGFloat *) MCGCacheTableGet(s_measure_cache, t_key, t_key_length);		
		if (t_width_ptr != NULL)
		{
			MCMemoryDelete(t_key);
			return *t_width_ptr;
		}		
	
		MCGFloat t_width;
		t_width = __MCGContextMeasurePlatformText(self, p_text, p_length, p_font, p_transform);
        
        // MM-2014-08-05: [[ Bug 13101 ]] Make sure only 1 thread mutates the measure cache at the time.
		MCGCacheTableSet(s_measure_cache, t_key, t_key_length, &t_width, sizeof(MCGFloat));
        
        return t_width;
	}
	
	if (!t_success)
		MCMemoryDelete(t_key);
	return 0.0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGContextCopyImage(MCGContextRef self, MCGImageRef &r_image)
{
	if (!MCGContextIsValid(self))
		return false;
	
	return MCGImageCreateWithSkBitmap(self->layer->canvas->getDevice()->accessBitmap(false), r_image);
}

////////////////////////////////////////////////////////////////////////////////

