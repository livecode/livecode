/* Copyright (C) 2003-2014 Runtime Revolution Ltd.
 
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

////////////////////////////////////////////////////////////////////////////////

#include <foundation.h>

#include <graphics.h>

////////////////////////////////////////////////////////////////////////////////

typedef struct _MCImageRep *MCImageRepRef;

MCImageRepRef MCImageRepRetain(MCImageRepRef p_image_rep);
void MCImageRepRelease(MCImageRepRef p_image_rep);
bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRepRef &r_image_rep);
bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRepRef &r_image_rep);
bool MCImageRepCreateWithPixels(uint32_t p_width, uint32_t p_height, MCDataRef p_pixels, MCImageRepRef &r_image_rep);
bool MCImageRepGetGeometry(MCImageRepRef p_image_rep, uint32_t &r_width, uint32_t &r_height);

////////////////////////////////////////////////////////////////////////////////

// Useful stuff

bool MCArrayStoreReal(MCArrayRef p_array, MCNameRef p_key, real64_t p_value)
{
	bool t_success;
	t_success = true;
	
	MCNumberRef t_number;
	t_number = nil;
	
	if (t_success)
		t_success = MCNumberCreateWithReal(p_value, t_number);
	
	if (t_success)
		t_success = MCArrayStoreValue(p_array, false, p_key, t_number);
	
	MCValueRelease(t_number);
	
	return t_success;
}

bool MCArrayFetchReal(MCArrayRef p_array, MCNameRef p_key, real64_t &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		t_success = MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber;
		// TODO - throw error on failure
	}
	
	if (t_success)
		r_value = MCNumberFetchAsReal((MCNumberRef)t_value);
	
	return t_success;
}

bool MCArrayFetchString(MCArrayRef p_array, MCNameRef p_key, MCStringRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeName)
			r_value = MCNameGetString((MCNameRef)t_value);
		else if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeString)
			r_value = (MCStringRef)t_value;
		else
			t_success = false;
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Type Definitions

// Color

struct MCCanvasColor
{
	MCGFloat red, green, blue, alpha;
};

uinteger_t MCCanvasColorType_Measure(void)
{
	return sizeof(MCCanvasColor);
}

void MCCanvasColorType_Initialize(MCCanvasColor *p_color)
{
	// Initialize color to default (opaque black)
	p_color->red = p_color->green = p_color->blue = 0.0;
	p_color->alpha = 1.0;
}

// Transform

typedef MCGAffineTransform MCCanvasTransform;

uinteger_t MCCanvasTransformType_Measure()
{
	return sizeof(MCCanvasTransform);
}

// Image

typedef MCImageRepRef MCCanvasImage;

uinteger_t MCCanvasImageType_Measure(void)
{
	return sizeof(MCCanvasImage);
}

void MCCanvasImageType_Finalize(MCCanvasImage *p_image)
{
	MCImageRepRelease(*p_image);
}

void MCCanvasImageType_Copy(MCCanvasImage *p_src_image, MCCanvasImage *p_dst_image)
{
	*p_dst_image = MCImageRepRetain(*p_src_image);
}

// Paint

enum MCCanvasPaintType
{
	kMCCanvasPaintTypeSolid,
	kMCCanvasPaintTypePattern,
	kMCCanvasPaintTypeGradient,
};

struct MCCanvasPaint
{
	MCCanvasPaintType type;
};

// Solid Paint

struct MCCanvasSolidPaint : public MCCanvasPaint
{
	MCCanvasColor color;
};

uinteger_t MCCanvasSolidPaintType_Measure()
{
	return sizeof(MCCanvasSolidPaint);
}

// Pattern

struct MCCanvasPattern : public MCCanvasPaint
{
	MCImageRepRef image;
	MCGAffineTransform transform;
};

uinteger_t MCCanvasPatternType_Measure(void)
{
	return sizeof(MCCanvasPaint);
}

void MCCanvasPatternType_Finalize(MCCanvasPattern *p_pattern)
{
	MCImageRepRelease(p_pattern->image);
}

void MCCanvasPatternType_Copy(MCCanvasPattern *p_src, MCCanvasPattern *p_dst)
{
	MCMemoryCopy(p_dst, p_src, sizeof(MCCanvasPattern));
	MCImageRepRetain(p_dst->image);
}

// Gradient

struct MCCanvasGradientStop
{
	MCGFloat offset;
	MCCanvasColor color;
};

uinteger_t MCCanvasGradientStopType_Measure(void)
{
	return sizeof(MCCanvasGradientStop);
}

struct MCCanvasGradient : public MCCanvasPaint
{
	MCGGradientFunction function;
	MCCanvasGradientStop *ramp;
	uint32_t ramp_length;
	bool mirror;
	bool wrap;
	uint32_t repeats;
	MCGAffineTransform transform;
	MCGImageFilter filter;
};

uinteger_t MCCanvasGradientType_Measure(void)
{
	return sizeof(MCCanvasGradient);
}

// Path

struct MCCanvasPath
{
	MCGPathRef path;
};

uinteger_t MCCanvasPathType_Measure(void)
{
	return sizeof(MCCanvasPath);
}

void MCCanvasPathType_Finalize(MCCanvasPath *p_path)
{
	MCGPathRelease(p_path->path);
}

void MCCanvasPathType_Copy(MCCanvasPath *p_src_path, MCCanvasPath *p_dst_path)
{
	/* UNCHECKED */ MCGPathMutableCopy(p_src_path->path, p_dst_path->path);
}

// Effect

enum MCCanvasEffectType
{
	kMCCanvasEffectTypeColorOverlay,
	kMCCanvasEffectTypeInnerShadow,
	kMCCanvasEffectTypeOuterShadow,
	kMCCanvasEffectTypeInnerGlow,
	kMCCanvasEffectTypeOuterGlow,
	
	_MCCanvasEffectTypeCount
};

enum MCCanvasEffectProperty
{
	kMCCanvasEffectPropertyColor,
	kMCCanvasEffectPropertyBlendMode,
	kMCCanvasEffectPropertyOpacity,
	
	//	kMCCanvasEffectPropertyFilter,
	kMCCanvasEffectPropertySize,
	kMCCanvasEffectPropertySpread,
	
	kMCCanvasEffectPropertyDistance,
	kMCCanvasEffectPropertyAngle,
	
	_MCCanvasEffectPropertyCount
};

struct MCCanvasEffect
{
	MCCanvasEffectType type;
	
	MCGFloat opacity;
	MCGBlendMode blend_mode;
	MCCanvasColor color;
	
	MCGFloat size;
	MCGFloat spread;
	
	MCGFloat distance;
	MCGFloat angle;
};

// Canvas

struct MCCanvas
{
	bool paint_changed : 1;
	bool fill_rule_changed : 1;
	bool antialias_changed : 1;
	bool opacity_changed : 1;
	bool blend_mode_changed : 1;
	bool stippled_changed : 1;
	bool graphic_effect_changed : 1;
	
	MCCanvasPaint paint;
	MCGFillRule fill_rule;
	bool antialias;
	MCGFloat opacity;
	MCGBlendMode blend_mode;
	bool stippled;
	MCCanvasEffect effect;
};

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode);
bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string);
bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string);
bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type);
bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string);

////////////////////////////////////////////////////////////////////////////////

static MCNameRef s_blend_mode_map[kMCGBlendModeCount];
static MCNameRef s_transform_matrix_keys[9];
static MCNameRef s_effect_type_map[_MCCanvasEffectTypeCount];
static MCNameRef s_effect_property_map[_MCCanvasEffectPropertyCount];
static MCNameRef s_gradient_type_map[kMCGGradientFunctionCount];

////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize();
void MCCanvasStringsFinalize();

void MCCanvasModuleInitialize()
{
	MCCanvasStringsInitialize();
}

void MCCanvasModuleFinalize()
{
	MCCanvasStringsFinalize();
}

////////////////////////////////////////////////////////////////////////////////

// Color

// Constructors

void MCCanvasColorMakeRGBA(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCCanvasColor &r_color)
{
	r_color.red = p_red;
	r_color.green = p_green;
	r_color.blue = p_blue;
	r_color.alpha = p_alpha;
}

//////////

// Properties

void MCCanvasColorGetRed(const MCCanvasColor &p_color, MCGFloat &r_red)
{
	r_red = p_color.red;
}

void MCCanvasColorSetRed(MCCanvasColor &x_color, MCGFloat p_red)
{
	x_color.red = p_red;
}

void MCCanvasColorGetGreen(const MCCanvasColor &p_color, MCGFloat &r_green)
{
	r_green = p_color.green;
}

void MCCanvasColorSetGreen(MCCanvasColor &x_color, MCGFloat p_green)
{
	x_color.green = p_green;
}

void MCCanvasColorGetBlue(const MCCanvasColor &p_color, MCGFloat &r_blue)
{
	r_blue = p_color.blue;
}

void MCCanvasColorSetBlue(MCCanvasColor &x_color, MCGFloat p_blue)
{
	x_color.blue = p_blue;
}

void MCCanvasColorGetAlpha(const MCCanvasColor &p_color, MCGFloat &r_alpha)
{
	r_alpha = p_color.alpha;
}

void MCCanvasColorSetAlpha(MCCanvasColor &x_color, MCGFloat p_alpha)
{
	x_color.alpha = p_alpha;
}

////////////////////////////////////////////////////////////////////////////////

// Transform

// Constructors

void MCCanvasTransformMakeIdentity(MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeIdentity();
}

void MCCanvasTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeScale(p_xscale, p_yscale);
}

void MCCanvasTransformMakeRotation(MCGFloat p_angle, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeRotation(p_angle);
}

void MCCanvasTransformMakeTranslation(MCGFloat p_x, MCGFloat p_y, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMakeTranslation(p_x, p_y);
}

void MCCanvasTransformMakeWithMatrix(MCGFloat p_a, MCGFloat p_b, MCGFloat p_c, MCGFloat p_d, MCGFloat p_tx, MCGFloat p_ty, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformMake(p_a, p_b, p_c, p_d, p_tx, p_ty);
}

//////////

// Properties

void MCCanvasTransformGetMatrix(const MCCanvasTransform &p_transform, MCArrayRef &r_matrix)
{
	bool t_success;
	t_success = true;
	
	MCArrayRef t_matrix;
	t_matrix = nil;
	
	if (t_success)
		t_success = MCArrayCreateMutable(t_matrix);
	
	if (t_success)
		t_success = MCArrayStoreReal(t_matrix, s_transform_matrix_keys[0], p_transform.a) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[1], p_transform.b) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[2], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[3], p_transform.c) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[4], p_transform.d) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[5], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[6], p_transform.tx) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[7], p_transform.ty) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[8], 1);
		
	if (t_success)
		r_matrix = t_matrix;
	else
		MCValueRelease(t_matrix);
	
	// return t_success;
}

void MCCanvasTransformSetMatrix(MCCanvasTransform &x_transform, MCArrayRef p_matrix)
{
	bool t_success;
	t_success = true;
	
	real64_t a, b, c, d, tx, ty;
	a = b = c = d = tx = ty = 0.0;
	t_success =
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[0], a) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[1], b) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[3], c) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[4], d) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[6], tx) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[7], ty);

	if (t_success)
		x_transform = MCGAffineTransformMake(a, b, c, d, tx, ty);
}

void MCCanvasTransformGetInverse(const MCCanvasTransform &p_transform, MCCanvasTransform &r_transform)
{
	r_transform = MCGAffineTransformInvert(p_transform);
}

// TODO - Decomposed transform properties - decide on order of components then work out how to calculate
bool MCCanvasTransformDecompose(const MCGAffineTransform &p_transform, MCGSize &r_scale, MCGFloat &r_rotation, MCGSize &r_skew, MCGSize &r_translation)
{
	return false;
}

bool MCCanvasTransformCompose(const MCGSize &p_scale, MCGFloat p_rotation, const MCGSize &p_skew, const MCGSize &p_translation, MCGAffineTransform &r_transform)
{
	return false;
}

// TODO - need output type?
void MCCanvasTransformGetScale(const MCCanvasTransform &p_transform, MCGSize &r_scale)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_scale = t_scale;
}

void MCCanvasTransformSetScale(MCCanvasTransform &x_transform, const MCGSize &p_scale)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	MCGAffineTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(p_scale, t_rotation, t_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetRotation(const MCCanvasTransform &p_transform, MCGFloat &r_rotation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_rotation = t_rotation;
}

void MCCanvasTransformSetRotation(MCCanvasTransform &x_transform, MCGFloat p_rotation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	MCGAffineTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, p_rotation, t_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetSkew(const MCCanvasTransform &p_transform, MCGSize &r_skew)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_skew = t_skew;
}

void MCCanvasTransformSetSkew(MCCanvasTransform &x_transform, const MCGSize &p_skew)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	MCGAffineTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, t_rotation, p_skew, t_translation, t_transform))
		x_transform = t_transform;
}

void MCCanvasTransformGetTranslation(const MCCanvasTransform &p_transform, MCGSize &r_translation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	if (MCCanvasTransformDecompose(p_transform, t_scale, t_rotation, t_skew, t_translation))
		r_translation = t_translation;
}

void MCCanvasTransformSetTranslation(MCCanvasTransform &x_transform, const MCGSize &p_translation)
{
	MCGSize t_scale, t_skew, t_translation;
	MCGFloat t_rotation;
	
	MCGAffineTransform t_transform;
	
	if (MCCanvasTransformDecompose(x_transform, t_scale, t_rotation, t_skew, t_translation) &&
		MCCanvasTransformCompose(t_scale, t_rotation, t_skew, p_translation, t_transform))
		x_transform = t_transform;
}

//////////

// Operations

void MCCanvasTransformConcat(MCCanvasTransform &x_transform_a, const MCCanvasTransform p_transform_b)
{
	x_transform_a = MCGAffineTransformConcat(x_transform_a, p_transform_b);
}

void MCCanvasTransformScale(MCCanvasTransform &x_transform, const MCGSize &p_scale)
{
	x_transform = MCGAffineTransformScale(x_transform, p_scale.width, p_scale.height);
}

void MCCanvasTransformRotate(MCCanvasTransform &x_transform, MCGFloat p_rotation)
{
	x_transform = MCGAffineTransformRotate(x_transform, p_rotation);
}

void MCCanvasTransformTranslate(MCCanvasTransform &x_transform, const MCGSize &p_translation)
{
	x_transform = MCGAffineTransformTranslate(x_transform, p_translation.width, p_translation.height);
}

////////////////////////////////////////////////////////////////////////////////

// Image

// Constructors

void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImage &x_image)
{
	/* UNCHECKED */ MCImageRepCreateWithPath(p_path, x_image);
}

void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImage &x_image)
{
	/* UNCHECKED */ MCImageRepCreateWithData(p_data, x_image);
}

void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImage &x_image)
{
	/* UNCHECKED */ MCImageRepCreateWithPixels(p_width, p_height, p_pixels, x_image);
}

// Properties

void MCCanvasImageGetWidth(const MCCanvasImage &p_image, integer_t &r_width)
{
	uint32_t t_width, t_height;
	/* UNCHECKED */ MCImageRepGetGeometry(p_image, t_width, t_height);
	r_width = t_width;
}

void MCCanvasImageGetHeight(const MCCanvasImage &p_image, integer_t &r_height)
{
	uint32_t t_width, t_height;
	/* UNCHECKED */ MCImageRepGetGeometry(p_image, t_width, t_height);
	r_height = t_height;
}

void MCCanvasImageGetPixels(const MCCanvasImage &p_image, MCDataRef &r_pixels)
{
	// TODO - implement
}

void MCCanvasImageGetMetadata(const MCCanvasImage &p_image, MCArrayRef &r_metadata)
{
	// TODO - implement
}

////////////////////////////////////////////////////////////////////////////////

// Solid Paint

// Constructor

void MCCanvasSolidPaintCreateWithColor(const MCCanvasColor &p_color, MCCanvasSolidPaint &x_paint)
{
	x_paint.type = kMCCanvasPaintTypeSolid;
	x_paint.color = p_color;
}

// Properties

void MCCanvasSolidPaintGetColor(const MCCanvasSolidPaint &p_paint, MCCanvasColor &r_color)
{
	r_color = p_paint.color;
}

void MCCanvasSolidPaintSetColor(MCCanvasSolidPaint &x_paint, const MCCanvasColor &p_color)
{
	x_paint.color = p_color;
}

////////////////////////////////////////////////////////////////////////////////

// Pattern

// Constructor

void MCCanvasPatternMakeWithTransformedImage(const MCCanvasImage &p_image, const MCCanvasTransform &p_transform, MCCanvasPattern &r_pattern)
{
	r_pattern.type = kMCCanvasPaintTypePattern;
	r_pattern.image = MCImageRepRetain(p_image);
	r_pattern.transform = p_transform;
}

void MCCanvasPatternMakeWithImage(const MCCanvasImage &p_image, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeIdentity(), r_pattern);
}

void MCCanvasPatternMakeWithScaledImage(const MCCanvasImage &p_image, MCGFloat p_xscale, MCGFloat p_yscale, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeScale(p_xscale, p_yscale), r_pattern);
}

void MCCanvasPatternMakeWithRotatedImage(const MCCanvasImage &p_image, MCGFloat p_angle, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeRotation(p_angle), r_pattern);
}

void MCCanvasPatternMakeWithTranslatedImage(const MCCanvasImage &p_image, MCGFloat p_x, MCGFloat p_y, MCCanvasPattern &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeTranslation(p_x, p_y), r_pattern);
}

// Properties

void MCCanvasPatternGetImage(const MCCanvasPattern &p_pattern, MCCanvasImage &r_image)
{
	r_image = MCImageRepRetain(p_pattern.image);
}

void MCCanvasPatternSetImage(MCCanvasPattern &x_pattern, const MCCanvasImage &p_image)
{
	MCImageRepRef t_rep;
	t_rep = MCImageRepRetain(p_image);
	MCImageRepRelease(x_pattern.image);
	x_pattern.image = t_rep;
}

void MCCanvasPatternGetTransform(const MCCanvasPattern &p_pattern, MCCanvasTransform &r_transform)
{
	r_transform = p_pattern.transform;
}

void MCCanvasPatternSetTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform)
{
	x_pattern.transform = p_transform;
}

// Operators

void MCCanvasPatternTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform)
{
	MCCanvasTransformConcat(x_pattern.transform, p_transform);
}

void MCCanvasPatternScale(MCCanvasPattern &x_pattern, MCGFloat p_xscale, MCGFloat p_yscale)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPatternRotate(MCCanvasPattern &x_pattern, MCGFloat p_angle)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPatternTranslate(MCCanvasPattern &x_pattern, MCGFloat p_x, MCGFloat p_y)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Gradient

// Constructor

void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradient &r_gradient)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientStop *t_stops;
	t_stops = nil;
	
	uint32_t t_stop_count;
	t_stop_count = 0;
	
	// TODO - check list type with Mark
//	if (t_success)
//	{
//		t_stop_count = MCProperListGetLength(p_ramp);
//		t_success = MCMemoryNewArray(t_stop_count, t_stops);
//	}
//	
//	for (uint32_t i = 0; t_success && i < t_stop_count; i++)
//	{
//		MCValueRef t_value;
//		t_value = MCProperListFetchElementAtIndex(p_ramp, i);
//		
//		t_success = t_value != nil && MCValueGetTypeCode(t_value) == kMCValueTypeCodeCustom;
//		// TODO - determine custom type is MCCanvasGradientStop
//		
//		MCCanvasGradientStop *t_stop;
//		t_stop = nil;
//		
//		if (t_success)
//		{
//			t_stop = (MCCanvasGradientStop*)MCValueGetExtraBytesPtr(t_value);
//			t_stops[i] = *t_stop;
//		}
//	}
	
	if (t_success)
	{
		r_gradient.type = kMCCanvasPaintTypeGradient;

		r_gradient.function = (MCGGradientFunction)p_type;
		r_gradient.ramp = t_stops;
		r_gradient.ramp_length = t_stop_count;
		r_gradient.filter = kMCGImageFilterNone;
		r_gradient.mirror = false;
		r_gradient.repeats = 1;
		r_gradient.wrap = false;
		r_gradient.transform = MCGAffineTransformMakeIdentity();
	}
	else
	{
		MCMemoryDeleteArray(t_stops);
	}
}

// Properties

void MCCanvasGradientGetTypeAsString(const MCCanvasGradient &p_gradient, MCStringRef &r_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeToString(p_gradient.function, r_string);
}

void MCCanvasGradientSetTypeAsString(MCCanvasGradient &x_gradient, MCStringRef p_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeFromString(p_string, x_gradient.function);
}

void MCCanvasGradientGetRepeat(const MCCanvasGradient &p_gradient, integer_t &r_repeat)
{
	r_repeat = p_gradient.repeats;
}

void MCCanvasGradientSetRepeat(MCCanvasGradient &x_gradient, integer_t p_repeat)
{
	x_gradient.repeats = p_repeat;
}

void MCCanvasGradientGetWrap(const MCCanvasGradient &p_gradient, bool &r_wrap)
{
	r_wrap = p_gradient.wrap;
}

void MCCanvasGradientSetWrap(MCCanvasGradient &x_gradient, bool p_wrap)
{
	x_gradient.wrap = p_wrap;
}

void MCCanvasGradientGetMirror(const MCCanvasGradient &p_gradient, bool &r_mirror)
{
	r_mirror = p_gradient.mirror;
}

void MCCanvasGradientSetMirror(MCCanvasGradient &x_gradient, bool p_mirror)
{
	x_gradient.mirror = p_mirror;
}

void MCCanvasGradientTransformToPoints(const MCGAffineTransform &p_transform, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	r_from = MCGPointApplyAffineTransform(MCGPointMake(0, 0), p_transform);
	r_to = MCGPointApplyAffineTransform(MCGPointMake(0, 1), p_transform);
	r_via = MCGPointApplyAffineTransform(MCGPointMake(1, 0), p_transform);
}

bool MCCanvasGradientTransformFromPoints(const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via, MCGAffineTransform &r_transform)
{
	MCGPoint t_src[3], t_dst[3];
	t_src[0] = MCGPointMake(0, 0);
	t_src[1] = MCGPointMake(0, 1);
	t_src[2] = MCGPointMake(1, 0);
	
	t_dst[0] = p_from;
	t_dst[1] = p_to;
	t_dst[2] = p_via;
	
	return MCGAffineTransformFromPoints(t_src, t_dst, r_transform);
}

void MCCanvasGradientGetPoints(const MCCanvasGradient &p_gradient, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	MCCanvasGradientTransformToPoints(p_gradient.transform, r_from, r_to, r_via);
}
void MCCanvasGradientSetPoints(MCCanvasGradient &x_gradient, const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via)
{
	if (!MCCanvasGradientTransformFromPoints(p_from, p_to, p_via, x_gradient.transform))
	{
		// TODO - throw error
	}
}

void MCCanvasGradientGetFrom(const MCCanvasGradient &p_gradient, MCGPoint &r_from)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_from = t_from;
}

void MCCanvasGradientGetTo(const MCCanvasGradient &p_gradient, MCGPoint &r_to)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_to = t_to;
}

void MCCanvasGradientGetVia(const MCCanvasGradient &p_gradient, MCGPoint &r_via)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	r_via = t_via;
}

void MCCanvasGradientSetFrom(MCCanvasGradient &x_gradient, const MCGPoint &p_from)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient.transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, p_from, t_to, t_via);
}

void MCCanvasGradientSetTo(MCCanvasGradient &x_gradient, const MCGPoint &p_to)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient.transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, p_to, t_via);
}

void MCCanvasGradientSetVia(MCCanvasGradient &x_gradient, const MCGPoint &p_via)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientTransformToPoints(x_gradient.transform, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, t_to, p_via);
}

void MCCanvasGradientGetTransform(const MCCanvasGradient &p_gradient, MCCanvasTransform &r_transform)
{
	r_transform = p_gradient.transform;
}

void MCCanvasGradientSetTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform)
{
	x_gradient.transform = p_transform;
}

// Operators

void MCCanvasGradientAddStop(MCCanvasGradient &x_gradient, const MCCanvasGradientStop &p_stop)
{
	if (p_stop.offset < 0 || p_stop.offset > 1)
	{
		// TODO - throw offset range error
		return;
	}
	
	if (x_gradient.ramp_length > 0 && p_stop.offset < x_gradient.ramp[0].offset)
	{
		// TODO - throw stop order error
		return;
	}
	
	if (!MCMemoryResizeArray(x_gradient.ramp_length + 1, x_gradient.ramp, x_gradient.ramp_length))
	{
		// TODO - throw memory error
		return;
	}
	
	x_gradient.ramp[x_gradient.ramp_length - 1] = p_stop;
}

void MCCanvasGradientTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform)
{
	MCCanvasTransformConcat(x_gradient.transform, p_transform);
}

void MCCanvasGradientScale(MCCanvasGradient &x_gradient, MCGFloat p_xscale, MCGFloat p_yscale)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasGradientRotate(MCCanvasGradient &x_gradient, MCGFloat p_angle)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasGradientTranslate(MCCanvasGradient &x_gradient, MCGFloat p_x, MCGFloat p_y)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeTranslation(p_x, p_y));
}

////////////////////////////////////////////////////////////////////////////////

// Path

// Constructors

void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPath &r_path)
{
	// TODO - parse path instructions
}

void MCCanvasPathMakeWithRoundedRectangle(const MCGRectangle &p_rect, MCGFloat p_x_radius, MCGFloat p_y_radius, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddRoundedRectangle(r_path.path, p_rect, MCGSizeMake(p_x_radius, p_y_radius));
}

void MCCanvasPathMakeWithRectangle(const MCGRectangle &p_rect, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddRectangle(r_path.path, p_rect);
}

void MCCanvasPathMakeWithEllipse(const MCGPoint &p_center, MCGFloat p_radius_x, MCGFloat p_radius_y, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddEllipse(r_path.path, p_center, MCGSizeMake(p_radius_x, p_radius_y), 0);
}

void MCCanvasPathMakeWithLine(const MCGPoint &p_start, const MCGPoint &p_end, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	/* UNCHECKED */ MCGPathAddLine(r_path.path, p_start, p_end);
}

void MCCanvasPathMakeWithPoints(const MCGPoint *p_points, uint32_t p_point_count, bool p_close, MCCanvasPath &r_path)
{
	/* UNCHECKED */ MCGPathCreateMutable(r_path.path);
	if (p_close)
		/* UNCHECKED */ MCGPathAddPolygon(r_path.path, p_points, p_point_count);
	else
		/* UNCHECKED */ MCGPathAddPolyline(r_path.path, p_points, p_point_count);
}

// Properties

void MCCanvasPathGetSubpaths(MCCanvasPath &p_path, integer_t p_start, integer_t p_end, MCCanvasPath &r_subpaths)
{
	MCGPathRef t_path;
	t_path = nil;
	if (!MCGPathMutableCopySubpaths(p_path.path, p_start, p_end, t_path))
	{
		// TODO - throw error
		return;
	}
	
	r_subpaths.path = t_path;
}

void MCCanvasPathGetBoundingBox(MCCanvasPath &p_path, MCGRectangle &r_bounds)
{
	/* UNCHECKED */ MCGPathGetBoundingBox(p_path.path, r_bounds);
}

void MCCanvasPathGetInstructionsAsString(const MCCanvasPath &p_path, MCStringRef &r_instruction_string)
{
	// TODO - unparse instructions
}

// Operators

void MCCanvasPathTransform(MCCanvasPath &x_path, const MCCanvasTransform &p_transform)
{
	// Path transformations are applied immediately
	/* UNCHECKED */ MCGPathTransform(x_path.path, p_transform);
}

void MCCanvasPathScale(MCCanvasPath &x_path, MCGFloat p_xscale, MCGFloat p_yscale)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPathRotate(MCCanvasPath &x_path, MCGFloat p_angle)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeRotation(p_angle));
}

void MCCanvasPathTranslate(MCCanvasPath &x_path, MCGFloat p_x, MCGFloat p_y)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasPathAddPath(MCCanvasPath &x_path, const MCCanvasPath &p_to_add)
{
	/* UNCHECKED */ MCGPathAddPath(x_path.path, p_to_add.path);
}

void MCCanvasPathMoveTo(MCCanvasPath &x_path, const MCGPoint &p_point)
{
	/* UNCHECKED */ MCGPathMoveTo(x_path.path, p_point);
}

void MCCanvasPathLineTo(MCCanvasPath &x_path, const MCGPoint &p_point)
{
	/* UNCHECKED */ MCGPathLineTo(x_path.path, p_point);
}

void MCCanvasPathCurveThroughPoint(MCCanvasPath &x_path, const MCGPoint &p_through, const MCGPoint &p_to)
{
	/* UNCHECKED */ MCGPathQuadraticTo(x_path.path, p_through, p_to);
}

void MCCanvasPathCurveThroughPoints(MCCanvasPath &x_path, const MCGPoint &p_through_a, const MCGPoint &p_through_b, const MCGPoint &p_to)
{
	/* UNCHECKED */ MCGPathCubicTo(x_path.path, p_through_a, p_through_b, p_to);
}

void MCCanvasPathClosePath(MCCanvasPath &x_path)
{
	/* UNCHECKED */ MCGPathCloseSubpath(x_path.path);
}

////////////////////////////////////////////////////////////////////////////////

// Effect

// Constructors

void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffect &r_effect)
{
	// TODO - defaults for missing properties?
	bool t_success;
	t_success = true;
	
	MCCanvasEffect t_effect;
	t_effect.type = (MCCanvasEffectType)p_type;
	
	real64_t t_opacity;
	
	if (t_success)
		t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyOpacity], t_opacity);
	
	MCStringRef t_blend_mode;
	t_blend_mode = nil;
	
	// Blend Mode
	if (t_success)
		t_success = MCArrayFetchString(p_properties, s_effect_property_map[kMCCanvasEffectPropertyBlendMode], t_blend_mode) &&
		MCCanvasBlendModeFromString(t_blend_mode, t_effect.blend_mode);
	
	// TODO - fetch color property (as MCCanvasColor?)
	// read properties for each effect type
	switch (t_effect.type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			// no other properties
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
		{
			real64_t t_distance, t_angle;
			// distance
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyDistance], t_distance);
			// angle
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyAngle], t_angle);
			
			t_effect.distance = t_distance;
			t_effect.angle = t_angle;
		}
			// Fall through to get size & spread
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
		{
			real64_t t_size, t_spread;
			// size
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySize], t_size);
			// spread
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySpread], t_spread);
			
			t_effect.size = t_size;
			t_effect.spread = t_spread;
		}
			break;
			
		default:
			t_success = false;
	}
	
	
}

//////////

// Properties

bool MCCanvasEffectHasSizeAndSpread(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow || p_type == kMCCanvasEffectTypeInnerGlow || p_type == kMCCanvasEffectTypeOuterGlow;
}

bool MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow;
}

void MCCanvasGraphicEffectGetTypeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_type)
{
	/* UNCHECKED */ MCCanvasEffectTypeToString(p_effect.type, r_type);
}

void MCCanvasGraphicEffectGetColor(const MCCanvasEffect &p_effect, MCCanvasColor &r_color)
{
	r_color = p_effect.color;
}

void MCCanvasGraphicEffectSetColor(MCCanvasEffect &x_effect, const MCCanvasColor &p_color)
{
	x_effect.color = p_color;
}

void MCCanvasEffectGetBlendModeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(p_effect.blend_mode, r_blend_mode);
}

void MCCanvasEffectSetBlendModeAsString(MCCanvasEffect &x_effect, MCStringRef p_blend_mode)
{
	MCGBlendMode t_blend_mode;
	if (MCCanvasBlendModeFromString(p_blend_mode, t_blend_mode))
		x_effect.blend_mode = t_blend_mode;
}

void MCCanvasEffectGetOpacity(const MCCanvasEffect &p_effect, MCGFloat &r_opacity)
{
	r_opacity = p_effect.opacity;
}

void MCCanvasEffectSetOpacity(MCCanvasEffect &x_effect, MCGFloat p_opacity)
{
	x_effect.opacity = p_opacity;
}

void MCCanvasEffectGetSize(const MCCanvasEffect &p_effect, MCGFloat &r_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_size = p_effect.size;
}

void MCCanvasEffectSetSize(MCCanvasEffect &x_effect, MCGFloat p_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
	{
		// TODO - throw error
		return;
	}

	x_effect.size = p_size;
}

void MCCanvasEffectGetSpread(const MCCanvasEffect &p_effect, MCGFloat &r_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_spread = p_effect.spread;
}

void MCCanvasEffectSetSpread(MCCanvasEffect &x_effect, MCGFloat p_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.spread = p_spread;
}

void MCCanvasEffectGetDistance(const MCCanvasEffect &p_effect, MCGFloat &r_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_distance = p_effect.distance;
}

void MCCanvasEffectSetDistance(MCCanvasEffect &x_effect, MCGFloat p_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.distance = p_distance;
}

void MCCanvasEffectGetAngle(const MCCanvasEffect &p_effect, MCGFloat &r_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(p_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	r_angle = p_effect.angle;
}

void MCCanvasEffectSetAngle(MCCanvasEffect &x_effect, MCGFloat p_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(x_effect.type))
	{
		// TODO - throw error
		return;
	}
	
	x_effect.angle = p_angle;
}

////////////////////////////////////////////////////////////////////////////////

// Canvas



////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize()
{
	MCMemoryClear(s_blend_mode_map, sizeof(s_blend_mode_map));
	MCMemoryClear(s_transform_matrix_keys, sizeof(s_transform_matrix_keys));
	MCMemoryClear(s_effect_type_map, sizeof(s_effect_type_map));
	MCMemoryClear(s_effect_property_map, sizeof(s_effect_property_map));
	MCMemoryClear(s_gradient_type_map, sizeof(s_gradient_type_map));
	
	/* UNCHECKED */
	s_blend_mode_map[kMCGBlendModeClear] = MCNAME("clear");
	s_blend_mode_map[kMCGBlendModeCopy] = MCNAME("copy");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source over");
	s_blend_mode_map[kMCGBlendModeSourceIn] = MCNAME("source in");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source out");
	s_blend_mode_map[kMCGBlendModeSourceAtop] = MCNAME("source atop");
	s_blend_mode_map[kMCGBlendModeDestinationOver] = MCNAME("destination over");
	s_blend_mode_map[kMCGBlendModeDestinationIn] = MCNAME("destination in");
	s_blend_mode_map[kMCGBlendModeDestinationOut] = MCNAME("destination out");
	s_blend_mode_map[kMCGBlendModeDestinationAtop] = MCNAME("destination atop");
	s_blend_mode_map[kMCGBlendModeXor] = MCNAME("xor");
	s_blend_mode_map[kMCGBlendModePlusDarker] = MCNAME("plus darker");
	s_blend_mode_map[kMCGBlendModePlusLighter] = MCNAME("plus lighter");
	s_blend_mode_map[kMCGBlendModeMultiply] = MCNAME("multiply");
	s_blend_mode_map[kMCGBlendModeScreen] = MCNAME("screen");
	s_blend_mode_map[kMCGBlendModeOverlay] = MCNAME("overlay");
	s_blend_mode_map[kMCGBlendModeDarken] = MCNAME("darken");
	s_blend_mode_map[kMCGBlendModeLighten] = MCNAME("lighten");
	s_blend_mode_map[kMCGBlendModeColorDodge] = MCNAME("color dodge");
	s_blend_mode_map[kMCGBlendModeColorBurn] = MCNAME("color burn");
	s_blend_mode_map[kMCGBlendModeSoftLight] = MCNAME("soft light");
	s_blend_mode_map[kMCGBlendModeHardLight] = MCNAME("hard light");
	s_blend_mode_map[kMCGBlendModeDifference] = MCNAME("difference");
	s_blend_mode_map[kMCGBlendModeExclusion] = MCNAME("exclusion");
	s_blend_mode_map[kMCGBlendModeHue] = MCNAME("hue");
	s_blend_mode_map[kMCGBlendModeSaturation] = MCNAME("saturation");
	s_blend_mode_map[kMCGBlendModeColor] = MCNAME("color");
	s_blend_mode_map[kMCGBlendModeLuminosity] = MCNAME("luminosity");
	
	s_transform_matrix_keys[0] = MCNAME("0,0");
	s_transform_matrix_keys[1] = MCNAME("1,0");
	s_transform_matrix_keys[2] = MCNAME("2,0");
	s_transform_matrix_keys[3] = MCNAME("0,1");
	s_transform_matrix_keys[4] = MCNAME("1,1");
	s_transform_matrix_keys[5] = MCNAME("2,1");
	s_transform_matrix_keys[6] = MCNAME("0,2");
	s_transform_matrix_keys[7] = MCNAME("1,2");
	s_transform_matrix_keys[8] = MCNAME("2,2");

	s_effect_type_map[kMCCanvasEffectTypeColorOverlay] = MCNAME("color overlay");
	s_effect_type_map[kMCCanvasEffectTypeInnerShadow] = MCNAME("inner shadow");
	s_effect_type_map[kMCCanvasEffectTypeOuterShadow] = MCNAME("outer shadow");
	s_effect_type_map[kMCCanvasEffectTypeInnerGlow] = MCNAME("inner glow");
	s_effect_type_map[kMCCanvasEffectTypeOuterGlow] = MCNAME("outer glow");
	
	s_effect_property_map[kMCCanvasEffectPropertyColor] = MCNAME("color");
	s_effect_property_map[kMCCanvasEffectPropertyBlendMode] = MCNAME("blend mode");
	s_effect_property_map[kMCCanvasEffectPropertyOpacity] = MCNAME("opacity");
	s_effect_property_map[kMCCanvasEffectPropertySize] = MCNAME("size");
	s_effect_property_map[kMCCanvasEffectPropertySpread] = MCNAME("spread");
	s_effect_property_map[kMCCanvasEffectPropertyDistance] = MCNAME("distance");
	s_effect_property_map[kMCCanvasEffectPropertyAngle] = MCNAME("angle");
	
	s_gradient_type_map[kMCGGradientFunctionLinear] = MCNAME("linear");
	s_gradient_type_map[kMCGGradientFunctionRadial] = MCNAME("radial");
	s_gradient_type_map[kMCGGradientFunctionSweep] = MCNAME("conical");
	s_gradient_type_map[kMCGLegacyGradientDiamond] = MCNAME("diamond");
	s_gradient_type_map[kMCGLegacyGradientSpiral] = MCNAME("spiral");
	s_gradient_type_map[kMCGLegacyGradientXY] = MCNAME("xy");
	s_gradient_type_map[kMCGLegacyGradientSqrtXY] = MCNAME("sqrtxy");
/* UNCHECKED */
}

void MCCanvasStringsFinalize()
{
	for (uint32_t i = 0; i < kMCGBlendModeCount; i++)
		MCValueRelease(s_blend_mode_map[i]);
	
	for (uint32_t i = 0; i < 9; i++)
		MCValueRelease(s_transform_matrix_keys[i]);

	for (uint32_t i = 0; i < _MCCanvasEffectTypeCount; i++)
		MCValueRelease(s_effect_type_map[i]);
	
	for (uint32_t i = 0; i < _MCCanvasEffectPropertyCount; i++)
		MCValueRelease(s_effect_property_map[i]);
	
	for (uint32_t i = 0; i < kMCGGradientFunctionCount; i++)
		MCValueRelease(s_gradient_type_map[i]);
}

template <typename T, MCNameRef *N, int C>
bool _mcenumfromstring(MCStringRef p_string, T &r_value)
{
	for (uint32_t i = 0; i < C; i++)
	{
		if (MCStringIsEqualTo(p_string, MCNameGetString(N[i]), kMCStringOptionCompareCaseless))
		{
			r_value = (T)i;
			return true;
		}
	}
	
	return false;
}

template <typename T, MCNameRef *N, int C>
bool _mcenumtostring(T p_value, MCStringRef &r_string)
{
	if (p_value >= C)
		return false;
	
	if (N[p_value] == nil)
		return false;
	
	return MCStringCopy(MCNameGetString(N[p_value]), r_string);
}

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode)
{
	return _mcenumfromstring<MCGBlendMode, s_blend_mode_map, kMCGBlendModeCount>(p_string, r_blend_mode);
}

bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string)
{
	return _mcenumtostring<MCGBlendMode, s_blend_mode_map, kMCGBlendModeCount>(p_blend_mode, r_string);
}

bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type)
{
	return _mcenumfromstring<MCGGradientFunction, s_gradient_type_map, kMCGGradientFunctionCount>(p_string, r_type);
}

bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCGGradientFunction, s_gradient_type_map, kMCGGradientFunctionCount>(p_type, r_string);
}

bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectType, s_effect_type_map, _MCCanvasEffectTypeCount>(p_type, r_string);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
