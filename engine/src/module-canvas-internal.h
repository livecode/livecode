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

#ifndef _MODULE_CANVAS_INTERNAL_H_
#define _MODULE_CANVAS_INTERNAL_H_

#include "module-canvas.h"

// Rectangle type
typedef MCGRectangle __MCCanvasRectangleImpl;
__MCCanvasRectangleImpl *MCCanvasRectangleGet(MCCanvasRectangleRef p_rect);

// Point type
typedef MCGPoint __MCCanvasPointImpl;
__MCCanvasPointImpl *MCCanvasPointGet(MCCanvasPointRef p_point);

// Color type

struct __MCCanvasColorImpl
{
	MCCanvasFloat red, green, blue, alpha;
};

__MCCanvasColorImpl *MCCanvasColorGet(MCCanvasColorRef p_color);

// Transform type
typedef MCGAffineTransform __MCCanvasTransformImpl;
__MCCanvasTransformImpl *MCCanvasTransformGet(MCCanvasTransformRef p_transform);

// Image type
typedef MCImageRep *__MCCanvasImageImpl;
__MCCanvasImageImpl *MCCanvasImageGet(MCCanvasImageRef p_image);

// Paint type
enum MCCanvasPaintType
{
	kMCCanvasPaintTypeSolid,
	kMCCanvasPaintTypePattern,
	kMCCanvasPaintTypeGradient,
};

struct __MCCanvasPaintImpl
{
	MCCanvasPaintType type;
};

__MCCanvasPaintImpl *MCCanvasPaintGet(MCCanvasPaintRef p_paint);

// Solid Paint type
struct __MCCanvasSolidPaintImpl : public __MCCanvasPaintImpl
{
	MCCanvasColorRef color;
};

void MCCanvasSolidPaintDelete(__MCCanvasSolidPaintImpl *p_paint);
bool MCCanvasSolidPaintEqual(__MCCanvasSolidPaintImpl *p_left, __MCCanvasSolidPaintImpl *p_right);
hash_t MCCanvasSolidPaintHash(__MCCanvasSolidPaintImpl *p_paint);

__MCCanvasSolidPaintImpl *MCCanvasSolidPaintGet(MCCanvasSolidPaintRef p_paint);

// Pattern type
struct __MCCanvasPatternImpl : public __MCCanvasPaintImpl
{
	MCCanvasImageRef image;
	MCCanvasTransformRef transform;
};

void MCCanvasPatternDelete(__MCCanvasPatternImpl *p_paint);
bool MCCanvasPatternEqual(__MCCanvasPatternImpl *p_left, __MCCanvasPatternImpl *p_right);
hash_t MCCanvasPatternHash(__MCCanvasPatternImpl *p_paint);

__MCCanvasPatternImpl *MCCanvasPatternGet(MCCanvasPatternRef p_paint);

// Gradient Stop type
struct __MCCanvasGradientStopImpl
{
	MCCanvasFloat offset;
	MCCanvasColorRef color;
};

__MCCanvasGradientStopImpl *MCCanvasGradientStopGet(MCCanvasGradientStopRef p_stop);

// Gradient type
struct __MCCanvasGradientImpl : public __MCCanvasPaintImpl
{
	MCGGradientFunction function;
	MCProperListRef ramp; // List of MCGradientStopRef;
	bool mirror:1;
	bool wrap:1;
	uint32_t repeats;
	MCCanvasTransformRef transform;
	MCGImageFilter filter;
};

void MCCanvasGradientDelete(__MCCanvasGradientImpl *p_paint);
bool MCCanvasGradientEqual(__MCCanvasGradientImpl *p_left, __MCCanvasGradientImpl *p_right);
hash_t MCCanvasGradientHash(__MCCanvasGradientImpl *p_paint);

__MCCanvasGradientImpl *MCCanvasGradientGet(MCCanvasGradientRef p_gradient);

// Path type
typedef MCGPathRef __MCCanvasPathImpl;
__MCCanvasPathImpl *MCCanvasPathGet(MCCanvasPathRef p_path);

// Effect opaque type
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

struct __MCCanvasEffectImpl
{
	MCCanvasEffectType type;
	
	MCCanvasFloat opacity;
	MCGBlendMode blend_mode;
	MCCanvasColorRef color;
	
	MCCanvasFloat size;
	MCCanvasFloat spread;
	
	MCCanvasFloat distance;
	MCCanvasFloat angle;
};

__MCCanvasEffectImpl *MCCanvasEffectGet(MCCanvasEffectRef);

// Canvas
struct MCCanvasProperties
{
	MCCanvasPaintRef paint;
	MCGFillRule fill_rule;
	bool antialias;
	MCCanvasFloat opacity;
	MCGBlendMode blend_mode;
	bool stippled;
	MCGImageFilter image_filter;
};

struct __MCCanvasImpl
{
	bool paint_changed : 1;
	bool fill_rule_changed : 1;
	bool antialias_changed : 1;
	bool opacity_changed : 1;
	bool blend_mode_changed : 1;
	bool stippled_changed : 1;
	
	MCCanvasProperties *prop_stack;
	uint32_t prop_max;
	uint32_t prop_index;
	
	MCCanvasProperties &props() { return prop_stack[prop_index]; }
	const MCCanvasProperties &props() const { return prop_stack[prop_index]; }
	
	MCGContextRef context;
};

__MCCanvasImpl *MCCanvasGet(MCCanvasRef p_canvas);

//////////

bool MCCanvasThrowError(MCTypeInfoRef p_error_type);

//////////

#define CANVAS_ANGLE_RADIANS 0
#define CANVAS_ANGLE_DEGREES 1

#define CANVAS_ANGLE_TYPE CANVAS_ANGLE_DEGREES

inline MCCanvasFloat MCCanvasDegreesToRadians(MCCanvasFloat p_degrees)
{
	return p_degrees * M_PI / 180;
}

inline MCCanvasFloat MCCanvasRadiansToDegrees(MCCanvasFloat p_radians)
{
	return p_radians * 180 / M_PI;
}

#if CANVAS_ANGLE_TYPE == CANVAS_ANGLE_DEGREES

inline MCCanvasFloat MCCanvasAngleToDegrees(MCCanvasFloat p_angle)
{
	return p_angle;
}

inline MCCanvasFloat MCCanvasAngleToRadians(MCCanvasFloat p_angle)
{
	return MCCanvasDegreesToRadians(p_angle);
}

inline MCCanvasFloat MCCanvasAngleFromDegrees(MCCanvasFloat p_degrees)
{
	return p_degrees;
}

inline MCCanvasFloat MCCanvasAngleFromRadians(MCCanvasFloat p_radians)
{
	return MCCanvasRadiansToDegrees(p_radians);
}

#else

inline MCCanvasFloat MCCanvasAngleToDegrees(MCCanvasFloat p_angle)
{
	return MCCanvasRadiansToDegrees(p_angle);
}

inline MCCanvasFloat MCCanvasAngleToRadians(MCCanvasFloat p_angle)
{
	return p_angle;
}

inline MCCanvasFloat MCCanvasAngleFromDegrees(MCCanvasFloat p_degrees)
{
	return MCCanvasDegreesToRadians(p_degrees);
}

inline MCCanvasFloat MCCanvasAngleFromRadians(MCCanvasFloat p_radians)
{
	return p_radians;
}

#endif

#endif//_MODULE_CANVAS_INTERNAL_H_
