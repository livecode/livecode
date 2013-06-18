#include "graphics.h"
#include "graphics-internal.h"

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkBitmap.h>
#include <SkShader.h>

////////////////////////////////////////////////////////////////////////////////

static void MCGContextStateDestroy(MCGContextStateRef self)
{
	if (self != NULL)
	{
		if (self -> fill_pattern != NULL)
			MCGPatternRelease(self -> fill_pattern);
		if (self -> fill_gradient != NULL)
			MCGGradientRelease(self -> fill_gradient);
		if (self -> stroke_pattern != NULL)
			MCGPatternRelease(self -> stroke_pattern);
		if (self -> stroke_gradient != NULL)
			MCGGradientRelease(self -> stroke_gradient);
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
		t_state -> opacity = 1.0f;
		t_state -> blend_mode = kMCGBlendModeSourceOver;		
		t_state -> flatness = 0.0f;
		t_state -> should_antialias = false;
		
		t_state -> fill_color = MCGColorMakeRGBA(0.0f, 0.0f, 0.0f, 1.0f);
		t_state -> fill_opacity = 0.0f;
		t_state -> fill_rule = kMCGFillRuleNonZero;
		t_state -> fill_pattern = NULL;
		t_state -> fill_gradient= NULL;
		
		t_state -> stroke_color = MCGColorMakeRGBA(0.0f, 0.0f, 0.0f, 1.0f);
		t_state -> stroke_opacity = 0.0f;
		t_state -> stroke_pattern = NULL;
		t_state -> stroke_gradient= NULL;
		t_state -> stroke_attr . width = 0.0f;
		t_state -> stroke_attr . join_style = kMCGJoinStyleBevel;
		t_state -> stroke_attr . cap_style = kMCGCapStyleButt;
		t_state -> stroke_attr . miter_limit = 0.0f;
		t_state -> stroke_attr . dashes = NULL;
		
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
		t_state -> opacity = p_state -> opacity;
		t_state -> blend_mode = p_state -> blend_mode;		
		t_state -> flatness = p_state -> flatness;
		t_state -> should_antialias = p_state -> should_antialias;
		
		t_state -> fill_color = p_state -> fill_color;
		t_state -> fill_opacity = p_state -> fill_opacity;
		t_state -> fill_rule = p_state -> fill_rule;
		t_state -> fill_pattern = MCGPatternRetain(p_state -> fill_pattern);
		t_state -> fill_gradient = MCGGradientRetain(p_state -> fill_gradient);
		
		t_state -> stroke_color = p_state -> stroke_color;
		t_state -> stroke_opacity = p_state -> stroke_opacity;
		t_state -> stroke_pattern = MCGPatternRetain(p_state -> stroke_pattern);
		t_state -> stroke_gradient = MCGGradientRetain(p_state -> stroke_gradient);
		t_state -> stroke_attr . width = p_state -> stroke_attr . width;
		t_state -> stroke_attr . join_style = p_state -> stroke_attr . join_style;
		t_state -> stroke_attr . cap_style = p_state -> stroke_attr . cap_style;
		t_state -> stroke_attr . miter_limit = p_state -> stroke_attr . miter_limit;
		t_state -> stroke_attr . dashes = MCGDashesRetain(p_state -> stroke_attr . dashes);
		
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
		if (self -> canvas != NULL)
			delete self -> canvas;
		if (self -> path != NULL)
			MCGPathRelease(self -> path);
	}
	MCMemoryDelete(self);
}

static bool MCGContextCreateWithBitmap(SkBitmap& p_bitmap, MCGContextRef& r_context)
{
	bool t_success;
	t_success = true;
	
	__MCGContext *t_context;
	t_context = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_context);
	
	if (t_success)
	{
		t_context -> canvas = new SkCanvas(p_bitmap);
		t_success = t_context -> canvas != NULL;
	}
	
	if (t_success)
		t_success = MCGContextStateCreate(t_context -> state);
	
	if (t_success)
	{
		t_context -> width = p_bitmap . width();
		t_context -> height = p_bitmap . height();
		t_context -> references = 1;
		t_context -> path = NULL;
		t_context -> is_valid = true;		
		MCGContextResetClip(t_context);
	}
	
	if (t_success)
		r_context = t_context;
	else
		MCGContextDestroy(t_context);
	
	return t_success;
}

bool MCGContextCreate(uint32_t p_width, uint32_t p_height, bool p_alpha, MCGContextRef& r_context)
{
	SkBitmap t_bitmap;
	t_bitmap . setConfig(SkBitmap::kARGB_8888_Config, p_width, p_height);
	t_bitmap . setIsOpaque(!p_alpha);
	
	if (t_bitmap . allocPixels())	
		return MCGContextCreateWithBitmap(t_bitmap, r_context);
	else
		return false;
}

bool MCGContextCreateWithRaster(MCGRaster &p_raster, MCGContextRef &r_context)
{
	SkBitmap t_bitmap;
	if (MCGRasterToSkBitmap(p_raster, kMCGPixelOwnershipTypeBorrow, t_bitmap))
		return MCGContextCreateWithBitmap(t_bitmap, r_context);
	else
		return false;
}

bool MCGContextCreateWithPixels(uint32_t p_width, uint32_t p_height, uint32_t p_stride, void *p_pixels, bool p_alpha, MCGContextRef& r_context)
{	
	SkBitmap t_bitmap;
	t_bitmap . setConfig(SkBitmap::kARGB_8888_Config, p_width, p_height, p_stride);
	t_bitmap . setIsOpaque(!p_alpha);
	t_bitmap . setPixels(p_pixels);
	
	return MCGContextCreateWithBitmap(t_bitmap, r_context);	
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
		self -> canvas -> save((SkCanvas::SaveFlags) (SkCanvas::kMatrix_SaveFlag | SkCanvas::kClip_SaveFlag));		
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
		self -> canvas -> restore();	
	
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

void MCGContextBegin(MCGContextRef self)
{	
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
	{		
		// use the current state's blending properties to composite the layer before creating a new state
		SkPaint t_paint;
		t_paint . setAntiAlias(self -> state -> should_antialias);
		t_paint . setAlpha((U8CPU) (self -> state -> opacity * 255));
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		if (t_blend_mode != NULL)
		{
			t_paint . setXfermode(t_blend_mode);
			t_blend_mode -> unref();
		}		
		self -> canvas -> saveLayer(NULL, &t_paint, (SkCanvas::SaveFlags) (SkCanvas::kHasAlphaLayer_SaveFlag | SkCanvas::kFullColorLayer_SaveFlag |
																		   SkCanvas::kMatrix_SaveFlag | SkCanvas::kClip_SaveFlag));	
		
		// flag the current state so that we know point to restore to on ending the layer
		self -> state -> is_layer_begin_pt = true;
		t_success = MCGContextPushState(self);
	}
	
	self -> is_valid = t_success;
}

void MCGContextEnd(MCGContextRef self)
{	
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
		t_success = self -> state -> parent != NULL;
	
	// keep on popping off states until we reach the state we were in when the layer was begun
	if (t_success)
		while (t_success && !self -> state -> is_layer_begin_pt)
		{
			t_success = MCGContextPopState(self);

			// we use skia to maintain a state's clip and transform, so calling restore ensures that the parent state's clip and CTM are restored properly 
			if (t_success)
				self -> canvas -> restore();			
		}
	
	if (t_success)
		self -> state -> is_layer_begin_pt = false;
	
	self -> is_valid = t_success;	
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

void MCGContextClipToRect(MCGContextRef self, MCGRectangle p_rect)
{
	if (!MCGContextIsValid(self))
		return;
	
	// TODO: ClipToRect should intersect with the current clip (rather than replace).
	//	Leaving as replace for the moment, as that's how it's used in the context.
	
	// we use skia to manage the clip entirely rather than storing in state
	// this means any transforms to the clip are handled by skia allowing for more complex clips (rather than just a rect)
	self -> canvas -> clipRect(MCGRectangleToSkRect(p_rect), SkRegion::kReplace_Op, self -> state -> should_antialias);
}

void MCGContextResetClip(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	// TODO: Take into account the state's tranformation.
	//	(we store the clip in local coords so for a transformed state, 0,0 is no longer the origin).
	self -> canvas -> clipRect(SkRect::MakeXYWH(SkIntToScalar(0), SkIntToScalar(0), SkIntToScalar(self -> width), SkIntToScalar(self -> height)),
							   SkRegion::kReplace_Op, self -> state -> should_antialias);
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

void MCGContextSetFillRGBAColor(MCGContextRef self, MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{
	if (!MCGContextIsValid(self))
		return;
	
	// make sure we have only one painting style set at a time by reseting all current paint styles
	// this should be refatored into a single paint object that contains either a color, pattern or gradient
	// TODO: Refactor color, pattern and gradients into ref counted (copy on write?) paint type.	
	MCGPatternRelease(self -> state -> fill_pattern);
	self -> state -> fill_pattern = NULL;
	MCGGradientRelease(self -> state -> fill_gradient);
	self -> state -> fill_gradient = NULL;
	
	p_red = MCClamp(p_red, 0.0f, 1.0f);
	p_green = MCClamp(p_green, 0.0f, 1.0f);
	p_blue = MCClamp(p_blue, 0.0f, 1.0f);
	p_alpha = MCClamp(p_alpha, 0.0f, 1.0f);
	self -> state -> fill_color = MCGColorMakeRGBA(p_red, p_green, p_blue, p_alpha);	
}

void MCGContextSetFillPattern(MCGContextRef self, MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
	if (!MCGContextIsValid(self))
		return;

	MCGPatternRelease(self -> state -> fill_pattern);
	self -> state -> fill_pattern = NULL;
	MCGGradientRelease(self -> state -> fill_gradient);
	self -> state -> fill_gradient = NULL;
	
	if (MCGImageIsValid(p_image))
		self -> is_valid = MCGPatternCreate(p_image, p_transform, p_filter, self -> state -> fill_pattern);
}

void MCGContextSetFillGradient(MCGContextRef self, MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGPatternRelease(self -> state -> fill_pattern);
	self -> state -> fill_pattern = NULL;
	MCGGradientRelease(self -> state -> fill_gradient);
	self -> state -> fill_gradient = NULL;	
	
	if (p_stops != NULL && p_colors != NULL)
		self -> is_valid = MCGGradientCreate(p_function, p_stops, p_colors, p_ramp_length, p_mirror, p_wrap, p_repeats, p_transform, p_filter, self -> state -> fill_gradient);	
}

////////////////////////////////////////////////////////////////////////////////
// Stroke attributes

void MCGContextSetStrokeOpacity(MCGContextRef self, MCGFloat p_opacity)
{	
	if (!MCGContextIsValid(self))
		return;
	
	self -> state -> stroke_opacity = MCClamp(p_opacity, 0.0f, 1.0f);
}

void MCGContextSetStrokeRGBAColor(MCGContextRef self, MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{	
	if (!MCGContextIsValid(self))
		return;
	
	MCGPatternRelease(self -> state -> stroke_pattern);
	self -> state -> stroke_pattern = NULL;
	MCGGradientRelease(self -> state -> stroke_gradient);
	self -> state -> stroke_gradient = NULL;		
	
	p_red = MCClamp(p_red, 0.0f, 1.0f);
	p_green = MCClamp(p_green, 0.0f, 1.0f);
	p_blue = MCClamp(p_blue, 0.0f, 1.0f);
	p_alpha = MCClamp(p_alpha, 0.0f, 1.0f);
	self -> state -> stroke_color = MCGColorMakeRGBA(p_red, p_green, p_blue, p_alpha);
}

void MCGContextSetStrokePattern(MCGContextRef self, MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
	if (!MCGContextIsValid(self))
		return;

	MCGPatternRelease(self -> state -> stroke_pattern);
	self -> state -> stroke_pattern = NULL;
	MCGGradientRelease(self -> state -> stroke_gradient);
	self -> state -> stroke_gradient = NULL;	
	
	if (MCGImageIsValid(p_image))
		self -> is_valid = MCGPatternCreate(p_image, p_transform, p_filter, self -> state -> stroke_pattern);	
}

void MCGContextSetStrokeGradient(MCGContextRef self, MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter)
{
	if (!MCGContextIsValid(self))
		return;
	
	MCGPatternRelease(self -> state -> stroke_pattern);
	self -> state -> stroke_pattern = NULL;
	MCGGradientRelease(self -> state -> stroke_gradient);
	self -> state -> stroke_gradient = NULL;	
	
	if (p_stops != NULL && p_colors != NULL)
		self -> is_valid = MCGGradientCreate(p_function, p_stops, p_colors, p_ramp_length, p_mirror, p_wrap, p_repeats, p_transform, p_filter, self -> state -> stroke_gradient);	
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

void MCGContextSetStrokeDashes(MCGContextRef self, MCGFloat p_phase, const MCGFloat *p_lengths, uindex_t p_arity)
{	
	if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;	
	
	if (t_success)
	{
		MCGDashesRelease(self -> state -> stroke_attr . dashes);
		self -> state -> stroke_attr . dashes = NULL;
		
		if (p_arity > 0)
			t_success = MCGDashesCreate(p_phase, p_lengths, p_arity, self -> state -> stroke_attr . dashes);
	}	
		
	self -> is_valid = t_success;	
}

////////////////////////////////////////////////////////////////////////////////
// Transforms - concatenated with the current CTM.

inline static bool MCGContextSetCTM(MCGContextRef self, MCGAffineTransform p_transform)
{
	SkMatrix t_matrix;
	MCGAffineTransformToSkMatrix(p_transform, t_matrix);
	self -> canvas -> setMatrix(t_matrix);
	
	// no need to transform the clip at this point as this is handled internally by skia
	return true;
}

void MCGContextConcatCTM(MCGContextRef self, MCGAffineTransform p_transform)
{
	if (!MCGContextIsValid(self))
		return;
	
	SkMatrix t_matrix;
	MCGAffineTransformToSkMatrix(p_transform, t_matrix);
	self -> canvas -> concat(t_matrix);
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
		MCGContextEnsurePath(self);
		
		MCGSize t_radii;
		t_radii . width = 1.0f;
		t_radii . height = 1.0f;
		MCGPathAddEllipse(self -> path, p_location, t_radii, 0.0f);
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

static bool MCGContextApplyPaintSettingsToSkPaint(MCGColor p_color, MCGPatternRef p_pattern, MCGGradientRef p_gradient, SkPaint &r_paint)
{
	// TODO: Refactor color, pattern and gradients into ref counted (copy on write?) paint type.
	bool t_success;
	t_success = true;
	
	SkShader *t_shader;
	t_shader = NULL;
	MCGImageFilter t_filter;
	t_filter = kMCGImageFilterNearest;
	if (t_success)
		if (p_gradient != NULL)
		{
			t_filter = p_gradient -> filter;
			t_success = MCGGradientToSkShader(p_gradient, t_shader);
		}
		else if (p_pattern != NULL)
		{
			t_filter = p_pattern -> filter;
			t_success = MCGPatternToSkShader(p_pattern, t_shader);
		}
	
	if (t_success)
	{
		r_paint . setColor(MCGColorToSkColor(p_color));
		r_paint . setShader(t_shader);
		
		switch (t_filter)
		{
			case kMCGImageFilterNearest:
				r_paint . setFilterBitmap(false);
				break;
			case kMCGImageFilterBilinear:
				r_paint . setFilterBitmap(true);
				break;
			case kMCGImageFilterBicubic:
				// TODO: Set filter explicitly based on filter settings.
				r_paint . setFilterBitmap(true);
				break;
		}		
	}
	
	if (t_shader != NULL)
		t_shader -> unref();
	
	return t_success;
}

static bool MCGContextSetupFillPaint(MCGContextRef self, SkPaint &r_paint)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = MCGContextApplyPaintSettingsToSkPaint(self -> state -> fill_color, self -> state -> fill_pattern, self -> state -> fill_gradient, r_paint);
	
	if (t_success)
	{		
		r_paint . setStyle(SkPaint::kFill_Style);
		r_paint . setAntiAlias(self -> state -> should_antialias);
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);				
		r_paint . setXfermode(t_blend_mode);		
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();
	}
	
	return t_success;
}

static bool MCGContextSetupStrokePaint(MCGContextRef self, SkPaint &r_paint)
{
	bool t_success;
	t_success = true;
		
	if (t_success)
		t_success = MCGContextApplyPaintSettingsToSkPaint(self -> state -> stroke_color, self -> state -> stroke_pattern, self -> state -> stroke_gradient, r_paint);
	
	SkDashPathEffect *t_dash_effect;
	t_dash_effect = NULL;
	if (t_success)
		if (self -> state -> stroke_attr . dashes != NULL)
			t_success = MCGDashesToSkDashPathEffect(self -> state -> stroke_attr . dashes, t_dash_effect);
	
	if (t_success)
	{				
		r_paint . setStyle(SkPaint::kStroke_Style);
		r_paint . setAntiAlias(self -> state -> should_antialias);
		r_paint . setStrokeWidth(MCGFloatToSkScalar(self -> state -> stroke_attr . width));
		r_paint . setStrokeMiter(MCGFloatToSkScalar(self -> state -> stroke_attr . miter_limit));
		r_paint . setStrokeJoin(MCGJoinStyleToSkJoinStyle(self -> state -> stroke_attr . join_style));
		r_paint . setStrokeCap(MCGCapStyleToSkCapStyle(self -> state -> stroke_attr . cap_style));	
		r_paint . setPathEffect(t_dash_effect);

		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		r_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();		
	}
	
	if (t_dash_effect != NULL)
		t_dash_effect -> unref();
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
// Operations

static bool MCGContextFillPath(MCGContextRef self, MCGPathRef p_path)
{	
	bool t_success;
	t_success = true;

	SkPaint t_paint;
	t_success = MCGContextSetupFillPaint(self, t_paint);

	if (t_success)
	{
		// should probably be careful of setting the fill type and path mutability here
		p_path -> path -> setFillType(MCGFillRuleToSkFillType(self -> state -> fill_rule));	
		self -> canvas -> drawPath(*p_path -> path, t_paint);		
	}
	
	return t_success;
}

static bool MCGContextStrokePath(MCGContextRef self, MCGPathRef p_path)
{	
	bool t_success;
	t_success = true;
		
	SkPaint t_paint;
	t_success = MCGContextSetupStrokePaint(self, t_paint);

	if (t_success)
		self -> canvas -> drawPath(*p_path -> path, t_paint);
		
	return t_success;
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
	
	// TODO: Implement
}

// Replace the current path by one thickened using the current stroke attributes.
void MCGContextThicken(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	// TODO: Implement
}

// Replace the current path by one entirely consisting of moveto, lineto and close commands.
void MCGContextFlatten(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	// TODO: Implement
}

// Replace the current path by one consisting of no overlapping subpaths or self
// intersections. Interior is determined by current fill rule.
void MCGContextSimplify(MCGContextRef self)
{
	if (!MCGContextIsValid(self))
		return;
	
	// TODO: Implement
}

////////////////////////////////////////////////////////////////////////////////

static bool MCGContextDrawSkBitmap(MCGContextRef self, const SkBitmap &p_bitmap, const MCGRectangle &p_dst, MCGImageFilter p_filter)
{
	bool t_success;
	t_success = true;
	
	SkPaint t_paint;
	if (t_success)
	{
		if (p_bitmap . config() == SkBitmap::kA8_Config)
			t_success = MCGContextSetupFillPaint(self, t_paint);
		else
		{			
			SkXfermode *t_blend_mode;
			t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		
			t_paint . setAlpha((U8CPU) (self -> state -> opacity * 255));
			t_paint . setXfermode(t_blend_mode);
			
			if (t_blend_mode != NULL)
				t_blend_mode -> unref();
		}
	}

	if (t_success)
	{
		switch (p_filter)
		{
		case kMCGImageFilterNearest:
			t_paint . setAntiAlias(false);
			t_paint . setFilterBitmap(false);
			break;
		case kMCGImageFilterBilinear:
			t_paint . setAntiAlias(true);
			t_paint . setFilterBitmap(true);
			break;
		case kMCGImageFilterBicubic:
			// TODO: Set filter explicitly based on filter settings.
			t_paint . setAntiAlias(true);
			t_paint . setFilterBitmap(true);
			break;
		}

		self -> canvas -> drawBitmapRectToRect(p_bitmap, NULL, MCGRectangleToSkRect(p_dst), &t_paint);
	}
	
	return t_success;
}

void MCGContextDrawImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_dst, MCGImageFilter p_filter)
{
	if (!MCGImageIsValid(p_image))
		return;

	self -> is_valid = MCGContextDrawSkBitmap(self, *p_image->bitmap, p_dst, p_filter);
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
		t_success = MCGContextDrawSkBitmap(self, t_bitmap, p_dst, p_filter);
	
	self -> is_valid = t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawText(MCGContextRef self, const char *p_text, uindex_t p_length, MCGPoint p_location, uint32_t p_font_size)
{	
	if (!MCGContextIsValid(self))
		return;	
	
	SkPaint t_paint;
	t_paint . setStyle(SkPaint::kFill_Style);	
	t_paint . setAntiAlias(true);
	t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
	t_paint . setTextSize(p_font_size);

	SkXfermode *t_blend_mode;
	t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
	if (t_blend_mode != NULL)
	{
		t_paint . setXfermode(t_blend_mode);
		t_blend_mode -> unref();
	}		
	
	self -> canvas -> drawText(p_text, p_length, MCGCoordToSkCoord(p_location . x), MCGCoordToSkCoord(p_location . y), t_paint);
}

MCGFloat MCGContextMeasureText(MCGContextRef self, const char *p_text, uindex_t p_length, uint32_t p_font_size)
{
	MCGFloat t_width;
	t_width = 0.0f;
	
	if (!MCGContextIsValid(self))
		return t_width;
	
	SkPaint t_paint;
	t_paint . setTextSize(p_font_size);
	t_width = (MCGFloat) t_paint . measureText(p_text, p_length);
	return t_width;
}

////////////////////////////////////////////////////////////////////////////////