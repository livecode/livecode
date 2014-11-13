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

typedef struct _MCImageRep *MCImageRep;

MCImageRep MCImageRepRetain(MCImageRep p_image_rep);
void MCImageRepRelease(MCImageRep p_image_rep);
bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRep &r_image_rep);
bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRep &r_image_rep);
bool MCImageRepCreateWithPixels(uint32_t p_width, uint32_t p_height, MCDataRef p_pixels, MCImageRep &r_image_rep);
bool MCImageRepGetGeometry(MCImageRep p_image_rep, uint32_t &r_width, uint32_t &r_height);

bool MCImageRepLockForTransform(MCImageRep p_image_rep, const MCGAffineTransform &p_transform, uint32_t p_frame, MCGImageRef &r_image, MCGSize &r_scale, uint32_t &r_duration);
bool MCImageRepUnlock(MCImageRep p_image_rep, MCGImageRef p_image);

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

// Rectangle record type
typedef MCGRectangle MCCanvasRectangle;

// Point record type
typedef MCGPoint MCCanvasPoint;

// Color record type
struct MCCanvasColor
{
	MCGFloat red, green, blue, alpha;
};

// Transform record type
typedef MCGAffineTransform MCCanvasTransform;

// Image opaque type
typedef MCImageRep MCCanvasImage;
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
	MCCanvasColor color;
} *MCCanvasSolidPaint;

void MCCanvasSolidPaintType_Copy(MCCanvasSolidPaint *p_src, MCCanvasSolidPaint *p_dst);
void MCCanvasSolidPaintType_Finalize(MCCanvasSolidPaint *p_paint);

// Pattern opaque type
typedef struct MCCanvasPatternStruct : public MCCanvasPaintStruct
{
	MCImageRep image;
	MCGAffineTransform transform;
} *MCCanvasPattern;

void MCCanvasPatternType_Finalize(MCCanvasPattern *p_pattern);
void MCCanvasPatternType_Copy(MCCanvasPattern *p_src, MCCanvasPattern *p_dst);

// Gradient Stop record type
struct MCCanvasGradientStop
{
	MCGFloat offset;
	MCCanvasColor color;
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
	MCGAffineTransform transform;
	MCGImageFilter filter;
} *MCCanvasGradient;

void MCCanvasGradientType_Finalize(MCCanvasGradient *p_gradient);
void MCCanvasGradientType_Copy(MCCanvasGradient *p_src, MCCanvasGradient *p_dst);

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
	
	MCGFloat opacity;
	MCGBlendMode blend_mode;
	MCCanvasColor color;
	
	MCGFloat size;
	MCGFloat spread;
	
	MCGFloat distance;
	MCGFloat angle;
};
uinteger_t MCCanvasEffectType_Measure(void);

// Canvas opaque type
struct MCCanvasProperties
{
	MCCanvasPaint paint;
	MCGFillRule fill_rule;
	bool antialias;
	MCGFloat opacity;
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
void MCCanvasColorMakeRGBA(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCCanvasColor &r_color);

// Properties
// TODO - don't think these are needed for a record type object

//////////

// Transform

// Constructors
void MCCanvasTransformMakeIdentity(MCCanvasTransform &r_transform);
void MCCanvasTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeRotation(MCGFloat p_angle, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeTranslation(MCGFloat p_x, MCGFloat p_y, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeSkew(MCGFloat p_x, MCGFloat p_y, MCCanvasTransform &r_transform);
void MCCanvasTransformMakeWithMatrix(MCGFloat p_a, MCGFloat p_b, MCGFloat p_c, MCGFloat p_d, MCGFloat p_tx, MCGFloat p_ty, MCCanvasTransform &r_transform);

// Properties
void MCCanvasTransformGetMatrix(const MCCanvasTransform &p_transform, MCArrayRef &r_matrix);
void MCCanvasTransformSetMatrix(MCCanvasTransform &x_transform, MCArrayRef p_matrix);
void MCCanvasTransformGetInverse(const MCCanvasTransform &p_transform, MCCanvasTransform &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
void MCCanvasTransformGetScale(const MCCanvasTransform &p_transform, MCGSize &r_scale);
void MCCanvasTransformSetScale(MCCanvasTransform &x_transform, const MCGSize &p_scale);
void MCCanvasTransformGetRotation(const MCCanvasTransform &p_transform, MCGFloat &r_rotation);
void MCCanvasTransformSetRotation(MCCanvasTransform &x_transform, MCGFloat p_rotation);
void MCCanvasTransformGetSkew(const MCCanvasTransform &p_transform, MCGSize &r_skew);
void MCCanvasTransformSetSkew(MCCanvasTransform &x_transform, const MCGSize &p_skew);
void MCCanvasTransformGetTranslation(const MCCanvasTransform &p_transform, MCGSize &r_translation);
void MCCanvasTransformSetTranslation(MCCanvasTransform &x_transform, const MCGSize &p_translation);

// Operations
void MCCanvasTransformConcat(MCCanvasTransform &x_transform_a, const MCCanvasTransform p_transform_b);
void MCCanvasTransformScale(MCCanvasTransform &x_transform, const MCGSize &p_scale);
void MCCanvasTransformRotate(MCCanvasTransform &x_transform, MCGFloat p_rotation);
void MCCanvasTransformTranslate(MCCanvasTransform &x_transform, const MCGSize &p_translation);
void MCCanvasTransformSkew(MCCanvasTransform &x_transform, MCGFloat p_xskew, MCGFloat p_yskew);

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
void MCCanvasImageGetMetadata(const MCCanvasImage &p_image, MCArrayRef &r_metadata);

// TODO - Image operations

//////////

// Solid Paint

// Constructors
void MCCanvasSolidPaintCreateWithColor(const MCCanvasColor &p_color, MCCanvasSolidPaint &x_paint);

// Properties
void MCCanvasSolidPaintGetColor(const MCCanvasSolidPaint &p_paint, MCCanvasColor &r_color);
void MCCanvasSolidPaintSetColor(MCCanvasSolidPaint &x_paint, const MCCanvasColor &p_color);

//////////

// Pattern

// Constructors
void MCCanvasPatternMakeWithTransformedImage(const MCCanvasImage &p_image, const MCCanvasTransform &p_transform, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithImage(const MCCanvasImage &p_image, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithScaledImage(const MCCanvasImage &p_image, MCGFloat p_xscale, MCGFloat p_yscale, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithRotatedImage(const MCCanvasImage &p_image, MCGFloat p_angle, MCCanvasPattern &r_pattern);
void MCCanvasPatternMakeWithTranslatedImage(const MCCanvasImage &p_image, MCGFloat p_x, MCGFloat p_y, MCCanvasPattern &r_pattern);

// Properties
void MCCanvasPatternGetImage(const MCCanvasPattern &p_pattern, MCCanvasImage &r_image);
void MCCanvasPatternSetImage(MCCanvasPattern &x_pattern, const MCCanvasImage &p_image);
void MCCanvasPatternGetTransform(const MCCanvasPattern &p_pattern, MCCanvasTransform &r_transform);
void MCCanvasPatternSetTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform);

// Operations
void MCCanvasPatternTransform(MCCanvasPattern &x_pattern, const MCCanvasTransform &p_transform);
void MCCanvasPatternScale(MCCanvasPattern &x_pattern, MCGFloat p_xscale, MCGFloat p_yscale);
void MCCanvasPatternRotate(MCCanvasPattern &x_pattern, MCGFloat p_angle);
void MCCanvasPatternTranslate(MCCanvasPattern &x_pattern, MCGFloat p_x, MCGFloat p_y);
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
void MCCanvasGradientGetFrom(const MCCanvasGradient &p_gradient, MCGPoint &r_from);
void MCCanvasGradientSetFrom(MCCanvasGradient &x_gradient, const MCGPoint &p_from);
void MCCanvasGradientGetTo(const MCCanvasGradient &p_gradient, MCGPoint &r_to);
void MCCanvasGradientSetTo(MCCanvasGradient &x_gradient, const MCGPoint &p_to);
void MCCanvasGradientGetVia(const MCCanvasGradient &p_gradient, MCGPoint &r_via);
void MCCanvasGradientSetVia(MCCanvasGradient &x_gradient, const MCGPoint &p_via);
void MCCanvasGradientGetTransform(const MCCanvasGradient &p_gradient, MCCanvasTransform &r_transform);
void MCCanvasGradientSetTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform);

// Operators
void MCCanvasGradientAddStop(MCCanvasGradient &x_gradient, const MCCanvasGradientStop &p_stop);
void MCCanvasGradientTransform(MCCanvasGradient &x_gradient, const MCCanvasTransform &p_transform);
void MCCanvasGradientScale(MCCanvasGradient &x_gradient, MCGFloat p_xscale, MCGFloat p_yscale);
void MCCanvasGradientRotate(MCCanvasGradient &x_gradient, MCGFloat p_angle);
void MCCanvasGradientTranslate(MCCanvasGradient &x_gradient, MCGFloat p_x, MCGFloat p_y);

//////////

// Path

// Constructors
void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPath &r_path);
void MCCanvasPathMakeWithRoundedRectangle(const MCGRectangle &p_rect, MCGFloat p_x_radius, MCGFloat p_y_radius, MCCanvasPath &r_path);
void MCCanvasPathMakeWithRectangle(const MCGRectangle &p_rect, MCCanvasPath &r_path);
void MCCanvasPathMakeWithEllipse(const MCGPoint &p_center, MCGFloat p_radius_x, MCGFloat p_radius_y, MCCanvasPath &r_path);
void MCCanvasPathMakeWithLine(const MCGPoint &p_start, const MCGPoint &p_end, MCCanvasPath &r_path);
void MCCanvasPathMakeWithPoints(const MCGPoint *p_points, uint32_t p_point_count, bool p_close, MCCanvasPath &r_path);

// Properties
void MCCanvasPathGetSubpaths(MCCanvasPath &p_path, integer_t p_start, integer_t p_end, MCCanvasPath &r_subpaths);
void MCCanvasPathGetBoundingBox(MCCanvasPath &p_path, MCGRectangle &r_bounds);
void MCCanvasPathGetInstructionsAsString(const MCCanvasPath &p_path, MCStringRef &r_instruction_string);

// Operations
void MCCanvasPathTransform(MCCanvasPath &x_path, const MCCanvasTransform &p_transform);
void MCCanvasPathScale(MCCanvasPath &x_path, MCGFloat p_xscale, MCGFloat p_yscale);
void MCCanvasPathRotate(MCCanvasPath &x_path, MCGFloat p_angle);
void MCCanvasPathTranslate(MCCanvasPath &x_path, MCGFloat p_x, MCGFloat p_y);
void MCCanvasPathAddPath(MCCanvasPath &x_path, const MCCanvasPath &p_to_add);
void MCCanvasPathMoveTo(MCCanvasPath &x_path, const MCGPoint &p_point);
void MCCanvasPathLineTo(MCCanvasPath &x_path, const MCGPoint &p_point);
void MCCanvasPathCurveThroughPoint(MCCanvasPath &x_path, const MCGPoint &p_through, const MCGPoint &p_to);
void MCCanvasPathCurveThroughPoints(MCCanvasPath &x_path, const MCGPoint &p_through_a, const MCGPoint &p_through_b, const MCGPoint &p_to);
void MCCanvasPathClosePath(MCCanvasPath &x_path);

//////////

// Effect

// Constructors
void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffect &r_effect);

// Properties
void MCCanvasGraphicEffectGetTypeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_type);
void MCCanvasGraphicEffectGetColor(const MCCanvasEffect &p_effect, MCCanvasColor &r_color);
void MCCanvasGraphicEffectSetColor(MCCanvasEffect &x_effect, const MCCanvasColor &p_color);
void MCCanvasEffectGetBlendModeAsString(const MCCanvasEffect &p_effect, MCStringRef &r_blend_mode);
void MCCanvasEffectSetBlendModeAsString(MCCanvasEffect &x_effect, MCStringRef p_blend_mode);
void MCCanvasEffectGetOpacity(const MCCanvasEffect &p_effect, MCGFloat &r_opacity);
void MCCanvasEffectSetOpacity(MCCanvasEffect &x_effect, MCGFloat p_opacity);
void MCCanvasEffectGetSize(const MCCanvasEffect &p_effect, MCGFloat &r_size);
void MCCanvasEffectSetSize(MCCanvasEffect &x_effect, MCGFloat p_size);
void MCCanvasEffectGetSpread(const MCCanvasEffect &p_effect, MCGFloat &r_spread);
void MCCanvasEffectSetSpread(MCCanvasEffect &x_effect, MCGFloat p_spread);
void MCCanvasEffectGetDistance(const MCCanvasEffect &p_effect, MCGFloat &r_distance);
void MCCanvasEffectSetDistance(MCCanvasEffect &x_effect, MCGFloat p_distance);
void MCCanvasEffectGetAngle(const MCCanvasEffect &p_effect, MCGFloat &r_angle);
void MCCanvasEffectSetAngle(MCCanvasEffect &x_effect, MCGFloat p_angle);

//////////

// Canvas

// Properties
void MCCanvasGetPaint(const MCCanvas &p_canvas, MCCanvasPaint &r_paint);
void MCCanvasSetPaint(MCCanvas &x_canvas, const MCCanvasPaint &p_paint);
void MCCanvasGetFillRuleAsString(const MCCanvas &p_canvas, MCStringRef &r_string);
void MCCanvasSetFillRuleAsString(MCCanvas &x_canvas, MCStringRef p_string);
void MCCanvasGetAntialias(const MCCanvas &p_canvas, bool &r_antialias);
void MCCanvasSetAntialias(MCCanvas &x_canvas, bool p_antialias);
void MCCanvasGetOpacity(const MCCanvas &p_canvas, MCGFloat &r_opacity);
void MCCanvasSetOpacity(MCCanvas &x_canvas, MCGFloat p_opacity);
void MCCanvasGetBlendModeAsString(const MCCanvas &p_canvas, MCStringRef &r_blend_mode);
void MCCanvasSetBlendModeAsString(MCCanvas &x_canvas, MCStringRef p_blend_mode);
void MCCanvasGetStippled(const MCCanvas &p_canvas, bool &r_stippled);
void MCCanvasSetStippled(MCCanvas &x_canvas, bool p_stippled);
void MCCanvasGetImageResizeQualityAsString(const MCCanvas &p_canvas, MCStringRef &r_quality);
void MCCanvasSetImageResizeQualityAsString(MCCanvas &x_canvas, MCStringRef p_quality);

// Operations
void MCCanvasCanvasTransform(MCCanvas &x_canvas, const MCCanvasTransform &p_transform);
void MCCanvasCanvasScale(MCCanvas &x_canvas, MCGFloat p_scale_x, MCGFloat p_scale_y);
void MCCanvasCanvasRotate(MCCanvas &x_canvas, MCGFloat p_angle);
void MCCanvasCanvasTranslate(MCCanvas &x_canvas, MCGFloat p_x, MCGFloat p_y);
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
