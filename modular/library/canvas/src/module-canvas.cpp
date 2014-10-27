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

struct MCCanvasImage
{
	MCGImageRef image;
	MCImageRep *image_rep;
};

uinteger_t MCCanvasImageType_Measure(void)
{
	return sizeof(MCCanvasImage);
}

// Paint

enum MCCanvasPaintType
{
	kMCCanvasPaintTypeColor,
	kMCCanvasPaintTypePattern,
	kMCCanvasPaintTypeGradient,
};

struct MCCanvasPaint
{
	MCCanvasPaintType type;
};

// Color

struct MCCanvasSolidPaint : public MCCanvasPaint
{
	MCCanvasColor color;
};

uinteger_t MCCanvasSolidPaintType_Measure()
{
	return sizeof(MCCanvasSolidPaint);
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



////////////////////////////////////////////////////////////////////////////////

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode);
bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string);
bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string);

////////////////////////////////////////////////////////////////////////////////

static MCNameRef s_blend_mode_map[kMCGBlendModeCount];
static MCNameRef s_transform_matrix_keys[9];
static MCNameRef s_effect_type_map[_MCCanvasEffectTypeCount];
static MCNameRef s_effect_property_map[_MCCanvasEffectPropertyCount];

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
	r_color.type = kMCCanvasPaintTypeColor;

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

// TODO - Hmm, this would ideally be an imagerep so we can load from file or data
//        but then we have to decide on how to resolve filenames (in the usual way
//        i.e. relative to the stack) We may need to move the image loading stuff
//        into libgraphics.

// Type

// TODO
//typedef MCGImageRef MCCanvasImage;

// Constructors

// TODO

// Properties

// TODO

// Operations

// TODO

////////////////////////////////////////////////////////////////////////////////

// Pattern

// TODO

// Type

struct MCCanvasPattern : public MCCanvasPaint
{
//	MCGImageRef image;
	MCGAffineTransform transform;
};

// Constructor

//void MCCanvasPatternMakeWithImage(const MCCanvasImage &p_image, MCCanvasPattern &r_pattern)
//{
//	r_pattern.image = MCGImageRetain(p_image);
//	r_pattern.transform = MCGAffineTransformMakeIdentity();
//}

////////////////////////////////////////////////////////////////////////////////

// Gradient

// Constructor

// TODO - ramp type? (list or array)
void MCCanvasGradientMakeWithRamp(integer_t p_type, MCArrayRef p_ramp, MCCanvasGradient &r_gradient);

// Properties

// Operators

////////////////////////////////////////////////////////////////////////////////

// Path

// Type
// TODO - based on MCGPath - need to expose this in libgraphics

// Constructors

// Properties

// Operators

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

// TODO - color property
// void MCCanvasGraphicEffectGetColor

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
}

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode)
{
	for (uint32_t i = 0; i < kMCGBlendModeCount; i++)
	{
		if (MCStringIsEqualTo(p_string, MCNameGetString(s_blend_mode_map[i]), kMCStringOptionCompareCaseless))
		{
			r_blend_mode = (MCGBlendMode)i;
			return true;
		}
	}
	
	return false;
}

bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string)
{
	if (p_blend_mode >= kMCGBlendModeCount)
		return false;
	
	if (s_blend_mode_map[p_blend_mode] == nil)
		return false;
	
	r_string = MCValueRetain(MCNameGetString(s_blend_mode_map[p_blend_mode]));
	return true;
}

bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string)
{
	if (p_type >= _MCCanvasEffectTypeCount)
		return false;
	
	r_string = MCValueRetain(MCNameGetString(s_effect_type_map[p_type]));
	return true;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
