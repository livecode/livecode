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

#ifndef _MODULE_CANVAS_H_
#define _MODULE_CANVAS_H_

#include <foundation.h>
#include <graphics.h>

////////////////////////////////////////////////////////////////////////////////

// TODO - move to MCImageRep wrapper library

class MCImageRep;

MCImageRep *MCImageRepRetain(MCImageRep *p_image_rep);
void MCImageRepRelease(MCImageRep *p_image_rep);
bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithPixels(MCDataRef p_pixels, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied, MCImageRep *&r_image_rep);
bool MCImageRepGetGeometry(MCImageRep *p_image_rep, uint32_t &r_width, uint32_t &r_height);
bool MCImageRepGetFrameDuration(MCImageRep *p_image_rep, uint32_t p_frame, uint32_t &r_duration);

bool MCImageRepLockForTransform(MCImageRep *p_image_rep, const MCGAffineTransform &p_transform, uint32_t p_frame, MCGImageRef &r_image, MCGSize &r_scale);
void MCImageRepUnlock(MCImageRep *p_image_rep, MCGImageRef p_image);

bool MCImageRepLockRasterForTransform(MCImageRep *p_image_rep, const MCGAffineTransform &p_transform, uint32_t p_frame, MCGRaster &r_raster, MCGSize &r_scale);
void MCImageRepUnlockRaster(MCImageRep *p_image_rep, const MCGRaster &p_raster);

////////////////////////////////////////////////////////////////////////////////

// TODO - move to foundation library ?
template <class T>
struct MCCArray
{
	T *data;
	uint32_t length;
};

////////////////////////////////////////////////////////////////////////////////
// Type Definitions

typedef MCGFloat MCCanvasFloat;

// Rectangle record type
typedef MCGRectangle MCCanvasRectangle;

// Point record type
typedef MCGPoint MCCanvasPoint;

// Color custom value type

typedef struct __MCCanvasColor *MCCanvasColorRef;

extern MCTypeInfoRef kMCCanvasColorTypeInfo;

bool MCCanvasColorCreateRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);
MCCanvasFloat MCCanvasColorGetRed(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetGreen(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetBlue(MCCanvasColorRef color);
MCCanvasFloat MCCanvasColorGetAlpha(MCCanvasColorRef color);
void MCCanvasColorGetRGBA(MCCanvasColorRef color, MCCanvasFloat &r_red, MCCanvasFloat &r_green, MCCanvasFloat &r_blue, MCCanvasFloat &r_alpha);

bool MCCanvasColorCopy(MCCanvasColorRef p_color, MCCanvasColorRef &r_copy);
void MCCanvasColorDelete(MCCanvasColorRef p_color);

// Transform record type
typedef MCGAffineTransform MCCanvasTransform;

// Image opaque type
typedef MCImageRep *MCCanvasImage;
uinteger_t MCCanvasImageType_Measure(void);
void MCCanvasImageType_Finalize(MCCanvasImage *p_image);
void MCCanvasImageType_Copy(MCCanvasImage *p_src_image, MCCanvasImage *p_dst_image);

// Paint opaque type
enum MCCanvasPaintType
{
	kMCCanvasPaintTypeSolid,
	kMCCanvasPaintTypePattern,
	kMCCanvasPaintTypeGradient,
};

typedef struct MCCanvasPaintStruct
{
	MCCanvasPaintType type;
} *MCCanvasPaint;

uinteger_t MCCanvasPaintType_Measure(void);
void MCCanvasPaintType_Finalize(MCCanvasPaint *p_paint);
void MCCanvasPaintType_Copy(MCCanvasPaint *p_src, MCCanvasPaint *p_dst);

// Solid Paint opaque type
typedef struct MCCanvasSolidPaintStruct : public MCCanvasPaintStruct
{
	MCCanvasColorRef color;
} *MCCanvasSolidPaint;

void MCCanvasSolidPaintType_Copy(MCCanvasSolidPaint *p_src, MCCanvasSolidPaint *p_dst);
void MCCanvasSolidPaintType_Finalize(MCCanvasSolidPaint *p_paint);
bool MCCanvasSolidPaintType_TypeCheck(MCCanvasPaint *p_paint);

// Pattern opaque type
typedef struct MCCanvasPatternStruct : public MCCanvasPaintStruct
{
	MCCanvasImage image;
	MCCanvasTransform transform;
} *MCCanvasPattern;

void MCCanvasPatternType_Finalize(MCCanvasPattern *p_pattern);
void MCCanvasPatternType_Copy(MCCanvasPattern *p_src, MCCanvasPattern *p_dst);
bool MCCanvasPatternType_TypeCheck(MCCanvasPaint *p_paint);

// Gradient Stop record type
struct MCCanvasGradientStop
{
	MCCanvasFloat offset;
	MCCanvasColorRef color;
};

// Gradient opaque type
typedef struct MCCanvasGradientStruct : public MCCanvasPaintStruct
{
	MCGGradientFunction function;
	MCCanvasGradientStop *ramp;
	uint32_t ramp_length;
	bool mirror;
	bool wrap;
	uint32_t repeats;
	MCCanvasTransform transform;
	MCGImageFilter filter;
} *MCCanvasGradient;

void MCCanvasGradientType_Finalize(MCCanvasGradient *p_gradient);
void MCCanvasGradientType_Copy(MCCanvasGradient *p_src, MCCanvasGradient *p_dst);
bool MCCanvasGradientType_TypeCheck(MCCanvasPaint *p_paint);

// Path opaque type
struct MCCanvasPath
{
	MCGPathRef path;
};

uinteger_t MCCanvasPathType_Measure(void);
void MCCanvasPathType_Finalize(MCCanvasPath *p_path);
void MCCanvasPathType_Copy(MCCanvasPath *p_src_path, MCCanvasPath *p_dst_path);

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

struct MCCanvasEffect
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

uinteger_t MCCanvasEffectType_Measure(void);
void MCCanvasEffectType_Finalize(MCCanvasEffect *p_effect);
void MCCanvasEffectType_Copy(MCCanvasEffect *p_src, MCCanvasEffect *p_dst);

// Canvas opaque type
struct MCCanvasProperties
{
	MCCanvasPaint paint;
	MCGFillRule fill_rule;
	bool antialias;
	MCCanvasFloat opacity;
	MCGBlendMode blend_mode;
	bool stippled;
	MCGImageFilter image_filter;
};

struct MCCanvas
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

////////////////////////////////////////////////////////////////////////////////

// Functions

// Module initialization
void MCCanvasModuleInitialize();
void MCCanvasModuleFinalize();

//////////

// Color

// Constructors
void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);

// Properties
// TODO - don't think these are needed for a record type object

//////////

// Transform

// Constructors
void MCCanvasTransformMakeIdentity(MCCanvasTransform &r_transform);
void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeWithMatrix(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransform &r_transform);

// Properties
void MCCanvasTransformGetMatrix(const MCCanvasTransform &p_transform, MCArrayRef &r_matrix);
void MCCanvasTransformSetMatrix(MCCanvasTransform &x_transform, MCArrayRef p_matrix);
void MCCanvasTransformGetInverse(const MCCanvasTransform &p_transform, MCCanvasTransform &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
// TODO - scale / skew / translation return type?
void MCCanvasTransformGetScale(const MCCanvasTransform &p_transform, MCGSize &r_scale);
void MCCanvasTransformSetScale(MCCanvasTransform &x_transform, const MCGSize &p_scale);
void MCCanvasTransformGetRotation(const MCCanvasTransform &p_transform, MCCanvasFloat &r_rotation);
void MCCanvasTransformSetRotation(MCCanvasTransform &x_transform, MCCanvasFloat p_rotation);
void MCCanvasTransformGetSkew(const MCCanvasTransform &p_transform, MCGSize &r_skew);
void MCCanvasTransformSetSkew(MCCanvasTransform &x_transform, const MCGSize &p_skew);
void MCCanvasTransformGetTranslation(const MCCanvasTransform &p_transform, MCGSize &r_translation);
void MCCanvasTransformSetTranslation(MCCanvasTransform &x_transform, const MCGSize &p_translation);

// Operations
void MCCanvasTransformConcat(MCCanvasTransform &x_transform_a, const MCCanvasTransform p_transform_b);
void MCCanvasTransformScale(MCCanvasTransform &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
void MCCanvasTransformRotate(MCCanvasTransform &x_transform, MCCanvasFloat p_rotation);
void MCCanvasTransformTranslate(MCCanvasTransform &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy);
void MCCanvasTransformSkew(MCCanvasTransform &x_transform, MCCanvasFloat p_x_skew, MCCanvasFloat p_y_skew);

//////////

// Image

// Constructors
void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImage &x_image);
void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImage &x_image);
void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImage &x_image);

// Properties
void MCCanvasImageGetWidth(const MCCanvasImage &p_image, uint32_t &r_width);
void MCCanvasImageGetHeight(const MCCanvasImage &p_image, uint32_t &r_height);
void MCCanvasImageGetPixels(const MCCanvasImage &p_image, MCDataRef &r_pixels);
// TODO - Add support for image metadata
//void MCCanvasImageGetMetadata(const MCCanvasImage &p_image, MCArrayRef &r_metadata);

// TODO - Implement image operations
//void MCCanvasImageTransform(MCCanvasImage &x_image, const MCCanvasTransform &p_transform);
//void MCCanvasImageScale(MCCanvasImage &x_image, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
//void MCCanvasImageRotate(MCCanvasImage &x_image, MCCanvasFloat p_angle);
//void MCCanvasImageCrop(MCCanvasImage &x_image, const MCCanvasRectangle &p_rect);

//////////

// Solid Paint

// Constructors
void MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaint &x_paint);

// Properties
void MCCanvasSolidPaintGetColor(const MCCanvasSolidPaint &p_paint, MCCanvasColorRef &r_color);
void MCCanvasSolidPaintSetColor(MCCanvasSolidPaint &x_paint, MCCanvasColorRef p_color);

//////////

// Pattern

// Constructors
void MCCanvasPatternMakeWithTransformedImage(const MCCanvasImage &p_image, const MCCanvasTransform &p_transform, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithImage(const MCCanvasImage &p_image, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithScaledImage(const MCCanvasImage &p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithRotatedImage(const MCCanvasImage &p_image, MCCanvasFloat p_angle, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithTranslatedImage(const MCCanvasImage &p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPattern &r_pattern);

// Properties
void MCCanvasPatternGetImage(const MCCanvasPattern &p_pattern, MCCanvasImage &r_image);
void MCCanvasPatternSetImage(MCCanvasPattern &x_pattern, const MCCanvasImage &p_image);
void MCCanvasPatternGetTransform(const MCCanvasPattern &p_pattern, MCCanvasTransform &r_transform);
void MCCanvasPatternSetTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform);

// Operations
void MCCanvasPatternTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform);
void MCCanvasPatternScale(MCCanvasPattern &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasPatternRotate(MCCanvasPattern &x_pattern, MCCanvasFloat p_angle);
void MCCanvasPatternTranslate(MCCanvasPattern &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y);
// TODO - add skew?"

//////////

// Gradient

// Constructors
void MCCanvasGradientMakeWithRamp(integer_t p_type, MCCArray<MCCanvasGradientStop> &p_ramp, MCCanvasGradient &r_gradient);

// Properties
void MCCanvasGradientGetRamp(const MCCanvasGradient &p_gradient, MCCArray<MCCanvasGradientStop> &r_ramp);
void MCCanvasGradientSetRamp(MCCanvasGradient &x_gradient, const MCCArray<MCCanvasGradientStop> &p_ramp);
void MCCanvasGradientGetTypeAsString(const MCCanvasGradient &p_gradient, MCStringRef &r_string);
void MCCanvasGradientSetTypeAsString(MCCanvasGradient &x_gradient, MCStringRef p_string);
void MCCanvasGradientGetRepeat(const MCCanvasGradient &p_gradient, integer_t &r_repeat);
void MCCanvasGradientSetRepeat(MCCanvasGradient &x_gradient, integer_t p_repeat);
void MCCanvasGradientGetWrap(const MCCanvasGradient &p_gradient, bool &r_wrap);
void MCCanvasGradientSetWrap(MCCanvasGradient &x_gradient, bool p_wrap);
void MCCanvasGradientGetMirror(const MCCanvasGradient &p_gradient, bool &r_mirror);
void MCCanvasGradientSetMirror(MCCanvasGradient &x_gradient, bool p_mirror);
void MCCanvasGradientGetFrom(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_from);
void MCCanvasGradientSetFrom(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_from);
void MCCanvasGradientGetTo(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_to);
void MCCanvasGradientSetTo(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_to);
void MCCanvasGradientGetVia(const MCCanvasGradient &p_gradient, MCCanvasPoint &r_via);
void MCCanvasGradientSetVia(MCCanvasGradient &x_gradient, const MCCanvasPoint &p_via);
void MCCanvasGradientGetTransform(const MCCanvasGradient &p_gradient, MCCanvasTransform &r_transform);
void MCCanvasGradientSetTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform);

// Operators
void MCCanvasGradientAddStop(MCCanvasGradient &x_gradient, const MCCanvasGradientStop &p_stop);
void MCCanvasGradientTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform);
void MCCanvasGradientScale(MCCanvasGradient &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasGradientRotate(MCCanvasGradient &x_gradient, MCCanvasFloat p_angle);
void MCCanvasGradientTranslate(MCCanvasGradient &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y);

//////////

// Path

// Constructors
void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPath &r_path);
void MCCanvasPathMakeWithRoundedRectangle(const MCCanvasRectangle &p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPath &r_path);
void MCCanvasPathMakeWithRectangle(const MCCanvasRectangle &p_rect, MCCanvasPath &r_path);
void MCCanvasPathMakeWithEllipse(const MCCanvasPoint &p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPath &r_path);
void MCCanvasPathMakeWithLine(const MCCanvasPoint &p_start, const MCCanvasPoint &p_end, MCCanvasPath &r_path);
void MCCanvasPathMakeWithPoints(const MCCArray<MCCanvasPoint> &p_points, bool p_close, MCCanvasPath &r_path);

// Properties
void MCCanvasPathGetSubpaths(MCCanvasPath &p_path, integer_t p_start, integer_t p_end, MCCanvasPath &r_subpaths);
void MCCanvasPathGetBoundingBox(MCCanvasPath &p_path, MCCanvasRectangle &r_bounds);
void MCCanvasPathGetInstructionsAsString(const MCCanvasPath &p_path, MCStringRef &r_instruction_string);

// Operations
void MCCanvasPathTransform(MCCanvasPath &x_path, const MCCanvasTransform &p_transform);
void MCCanvasPathScale(MCCanvasPath &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasPathRotate(MCCanvasPath &x_path, MCCanvasFloat p_angle);
void MCCanvasPathTranslate(MCCanvasPath &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasPathAddPath(MCCanvasPath &x_path, const MCCanvasPath &p_to_add);
void MCCanvasPathMoveTo(MCCanvasPath &x_path, const MCCanvasPoint &p_point);
void MCCanvasPathLineTo(MCCanvasPath &x_path, const MCCanvasPoint &p_point);
void MCCanvasPathCurveThroughPoint(MCCanvasPath &x_path, const MCCanvasPoint &p_through, const MCCanvasPoint &p_to);
void MCCanvasPathCurveThroughPoints(MCCanvasPath &x_path, const MCCanvasPoint &p_through_a, const MCCanvasPoint &p_through_b, const MCCanvasPoint &p_to);
void MCCanvasPathClosePath(MCCanvasPath &x_path);

//////////

// Effect

// Constructors
void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffect &r_effect);

// Properties
void MCCanvasGraphicEffectGetTypeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_type);
void MCCanvasGraphicEffectGetColor(const MCCanvasEffect &p_effect, MCCanvasColorRef &r_color);
void MCCanvasGraphicEffectSetColor(MCCanvasEffect &x_effect, MCCanvasColorRef p_color);
void MCCanvasEffectGetBlendModeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_blend_mode);
void MCCanvasEffectSetBlendModeAsString(MCCanvasEffect &x_effect, MCStringRef p_blend_mode);
void MCCanvasEffectGetOpacity(const MCCanvasEffect &p_effect, MCCanvasFloat &r_opacity);
void MCCanvasEffectSetOpacity(MCCanvasEffect &x_effect, MCCanvasFloat p_opacity);
void MCCanvasEffectGetSize(const MCCanvasEffect &p_effect, MCCanvasFloat &r_size);
void MCCanvasEffectSetSize(MCCanvasEffect &x_effect, MCCanvasFloat p_size);
void MCCanvasEffectGetSpread(const MCCanvasEffect &p_effect, MCCanvasFloat &r_spread);
void MCCanvasEffectSetSpread(MCCanvasEffect &x_effect, MCCanvasFloat p_spread);
void MCCanvasEffectGetDistance(const MCCanvasEffect &p_effect, MCCanvasFloat &r_distance);
void MCCanvasEffectSetDistance(MCCanvasEffect &x_effect, MCCanvasFloat p_distance);
void MCCanvasEffectGetAngle(const MCCanvasEffect &p_effect, MCCanvasFloat &r_angle);
void MCCanvasEffectSetAngle(MCCanvasEffect &x_effect, MCCanvasFloat p_angle);

//////////

// Canvas

// Properties
void MCCanvasGetPaint(const MCCanvas &p_canvas, MCCanvasPaint &r_paint);
void MCCanvasSetPaint(MCCanvas &x_canvas, const MCCanvasPaint &p_paint);
void MCCanvasGetFillRuleAsString(const MCCanvas &p_canvas, MCStringRef &r_string);
void MCCanvasSetFillRuleAsString(MCCanvas &x_canvas, MCStringRef p_string);
void MCCanvasGetAntialias(const MCCanvas &p_canvas, bool &r_antialias);
void MCCanvasSetAntialias(MCCanvas &x_canvas, bool p_antialias);
void MCCanvasGetOpacity(const MCCanvas &p_canvas, MCCanvasFloat &r_opacity);
void MCCanvasSetOpacity(MCCanvas &x_canvas, MCCanvasFloat p_opacity);
void MCCanvasGetBlendModeAsString(const MCCanvas &p_canvas, MCStringRef &r_blend_mode);
void MCCanvasSetBlendModeAsString(MCCanvas &x_canvas, MCStringRef p_blend_mode);
void MCCanvasGetStippled(const MCCanvas &p_canvas, bool &r_stippled);
void MCCanvasSetStippled(MCCanvas &x_canvas, bool p_stippled);
void MCCanvasGetImageResizeQualityAsString(const MCCanvas &p_canvas, MCStringRef &r_quality);
void MCCanvasSetImageResizeQualityAsString(MCCanvas &x_canvas, MCStringRef p_quality);

// Operations
void MCCanvasCanvasTransform(MCCanvas &x_canvas, const MCCanvasTransform &p_transform);
void MCCanvasCanvasScale(MCCanvas &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
void MCCanvasCanvasRotate(MCCanvas &x_canvas, MCCanvasFloat p_angle);
void MCCanvasCanvasTranslate(MCCanvas &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasCanvasSaveState(MCCanvas &x_canvas);
void MCCanvasCanvasRestore(MCCanvas &x_canvas);
void MCCanvasCanvasBeginLayer(MCCanvas &x_canvas);
// TODO - work out effect area rect
void MCCanvasCanvasBeginLayerWithEffect(MCCanvas &x_canvas, const MCCanvasEffect &p_effect, const MCCanvasRectangle &p_rect);
void MCCanvasCanvasEndLayer(MCCanvas &x_canvas);
void MCCanvasCanvasFill(MCCanvas &x_canvas);
void MCCanvasCanvasFillPath(MCCanvas &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasStroke(MCCanvas &x_canvas);
void MCCanvasCanvasStrokePath(MCCanvas &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasClipToRect(MCCanvas &x_canvas, const MCCanvasRectangle &p_rect);
void MCCanvasCanvasAddPath(MCCanvas &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasDrawRectOfImage(MCCanvas &x_canvas, const MCCanvasImage &p_image, const MCCanvasRectangle &p_src_rect, const MCCanvasRectangle &p_dst_rect, uint32_t p_quality);
void MCCanvasCanvasMoveTo(MCCanvas &x_canvas, const MCCanvasPoint &p_point);
void MCCanvasCanvasLineTo(MCCanvas &x_canvas, const MCCanvasPoint &p_point);
void MCCanvasCanvasCurveThroughPoint(MCCanvas &x_canvas, const MCCanvasPoint &p_through, const MCCanvasPoint &p_to);
void MCCanvasCanvasCurveThroughPoints(MCCanvas &x_canvas, const MCCanvasPoint &p_through_a, const MCCanvasPoint &p_through_b, const MCCanvasPoint &p_to);
void MCCanvasCanvasClosePath(MCCanvas &x_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
