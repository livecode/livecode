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

struct MCGImageFrame
{
	MCGImageRef image;
	
	MCGFloat x_scale;
	MCGFloat y_scale;
};

// An MCImageBitmap is a non-premultiplied ARGB image, as decoded from the storage format
struct MCImageBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t *data;
	
	bool has_transparency;
	bool has_alpha;
};

class MCImageRep;

MCImageRep *MCImageRepRetain(MCImageRep *p_image_rep);
void MCImageRepRelease(MCImageRep *p_image_rep);
bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithPixels(MCDataRef p_pixels, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied, MCImageRep *&r_image_rep);
bool MCImageRepGetGeometry(MCImageRep *p_image_rep, uint32_t &r_width, uint32_t &r_height);
bool MCImageRepGetFrameDuration(MCImageRep *p_image_rep, uint32_t p_frame, uint32_t &r_duration);

bool MCImageRepLock(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCGImageFrame &r_frame);
void MCImageRepUnlock(MCImageRep *p_image_rep, uint32_t p_index, MCGImageFrame &p_frame);

bool MCImageRepLockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCImageBitmap *&r_raster);
void MCImageRepUnlockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCImageBitmap *p_raster);

//////////

bool MCImageRepGetTransformed(MCImageRep *p_image_rep, const MCGAffineTransform &p_transform, const MCGSize *p_output_size, MCGImageFilter p_resize_quality, MCImageRep *&r_transformed);

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

// Rectangle opaque type
typedef MCGRectangle MCCanvasRectangle;
uinteger_t MCCanvasRectangleType_Measure(void);

// Point opaque type
typedef MCGPoint MCCanvasPoint;
uinteger_t MCCanvasPointType_Measure(void);

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

// Transform opaque type
typedef MCGAffineTransform MCCanvasTransform;
uinteger_t MCCanvasTransformType_Measure(void);

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
void MCCanvasPaintType_Copy(const MCCanvasPaint *p_src, MCCanvasPaint *p_dst);

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

// Canvas custom value type

typedef struct __MCCanvas *MCCanvasRef;

extern MCTypeInfoRef kMCCanvasTypeInfo;

bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas);

////////////////////////////////////////////////////////////////////////////////

// Functions

// Module initialization
void MCCanvasModuleInitialize();
void MCCanvasModuleFinalize();

//////////

// Rectangle

// Constructors
void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangle &r_rect);

// Properties
void MCCanvasRectangleGetLeft(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_left);
void MCCanvasRectangleSetLeft(MCCanvasRectangle &x_rect, MCCanvasFloat p_left);
void MCCanvasRectangleGetTop(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_top);
void MCCanvasRectangleSetTop(MCCanvasRectangle &x_rect, MCCanvasFloat p_top);
void MCCanvasRectangleGetRight(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_right);
void MCCanvasRectangleSetRight(MCCanvasRectangle &x_rect, MCCanvasFloat p_right);
void MCCanvasRectangleGetBottom(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_bottom);
void MCCanvasRectangleSetBottom(MCCanvasRectangle &x_rect, MCCanvasFloat p_bottom);
void MCCanvasRectangleGetWidth(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_width);
void MCCanvasRectangleSetWidth(MCCanvasRectangle &x_rect, MCCanvasFloat p_width);
void MCCanvasRectangleGetHeight(const MCCanvasRectangle &p_rect, MCCanvasFloat &r_height);
void MCCanvasRectangleSetHeight(MCCanvasRectangle &x_rect, MCCanvasFloat p_height);

//////////

// Point

// Constructors
void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPoint &r_point);

// Properties
void MCCanvasPointGetX(const MCCanvasPoint &p_point, MCCanvasFloat &r_x);
void MCCanvasPointSetX(MCCanvasPoint &x_point, MCCanvasFloat p_x);
void MCCanvasPointGetY(const MCCanvasPoint &p_point, MCCanvasFloat &r_y);
void MCCanvasPointSetY(MCCanvasPoint &x_point, MCCanvasFloat p_y);

//////////

// Color

// Constructors
void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);

// Properties

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
void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaint &r_paint);
void MCCanvasSetPaint(MCCanvasRef &x_canvas, const MCCanvasPaint &p_paint);
void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string);
void MCCanvasSetFillRuleAsString(MCCanvasRef &x_canvas, MCStringRef p_string);
void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias);
void MCCanvasSetAntialias(MCCanvasRef &x_canvas, bool p_antialias);
void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity);
void MCCanvasSetOpacity(MCCanvasRef &x_canvas, MCCanvasFloat p_opacity);
void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode);
void MCCanvasSetBlendModeAsString(MCCanvasRef &x_canvas, MCStringRef p_blend_mode);
void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled);
void MCCanvasSetStippled(MCCanvasRef &x_canvas, bool p_stippled);
void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality);
void MCCanvasSetImageResizeQualityAsString(MCCanvasRef &x_canvas, MCStringRef p_quality);

// Operations
void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, const MCCanvasTransform &p_transform);
void MCCanvasCanvasScale(MCCanvasRef &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
void MCCanvasCanvasRotate(MCCanvasRef &x_canvas, MCCanvasFloat p_angle);
void MCCanvasCanvasTranslate(MCCanvasRef &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasCanvasSaveState(MCCanvasRef &x_canvas);
void MCCanvasCanvasRestore(MCCanvasRef &x_canvas);
void MCCanvasCanvasBeginLayer(MCCanvasRef &x_canvas);
// TODO - work out effect area rect
void MCCanvasCanvasBeginLayerWithEffect(MCCanvasRef &x_canvas, const MCCanvasEffect &p_effect, const MCCanvasRectangle &p_rect);
void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas);
void MCCanvasCanvasFill(MCCanvasRef &x_canvas);
void MCCanvasCanvasFillPath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasStroke(MCCanvasRef &x_canvas);
void MCCanvasCanvasStrokePath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasClipToRect(MCCanvasRef &x_canvas, const MCCanvasRectangle &p_rect);
void MCCanvasCanvasAddPath(MCCanvasRef &x_canvas, const MCCanvasPath &p_path);
void MCCanvasCanvasDrawRectOfImage(MCCanvasRef &x_canvas, const MCCanvasImage &p_image, const MCCanvasRectangle &p_src_rect, const MCCanvasRectangle &p_dst_rect, uint32_t p_quality);
void MCCanvasCanvasMoveTo(MCCanvasRef &x_canvas, const MCCanvasPoint &p_point);
void MCCanvasCanvasLineTo(MCCanvasRef &x_canvas, const MCCanvasPoint &p_point);
void MCCanvasCanvasCurveThroughPoint(MCCanvasRef &x_canvas, const MCCanvasPoint &p_through, const MCCanvasPoint &p_to);
void MCCanvasCanvasCurveThroughPoints(MCCanvasRef &x_canvas, const MCCanvasPoint &p_through_a, const MCCanvasPoint &p_through_b, const MCCanvasPoint &p_to);
void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
