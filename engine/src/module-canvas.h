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

// Module initialization
void MCCanvasModuleInitialize();
void MCCanvasModuleFinalize();

////////////////////////////////////////////////////////////////////////////////
// Type Definitions

typedef MCGFloat MCCanvasFloat;

typedef struct __MCCanvasRectangle *MCCanvasRectangleRef;
typedef struct __MCCanvasPoint *MCCanvasPointRef;
typedef struct __MCCanvasColor *MCCanvasColorRef;
typedef struct __MCCanvasTransform *MCCanvasTransformRef;
typedef struct __MCCanvasImage *MCCanvasImageRef;
typedef struct __MCCanvasPaint *MCCanvasPaintRef;
typedef struct __MCCanvasGradientStop *MCCanvasGradientStopRef;
typedef struct __MCCanvasPath *MCCanvasPathRef;
typedef struct __MCCanvasEffect *MCCanvasEffectRef;
typedef struct __MCCanvas *MCCanvasRef;

extern MCTypeInfoRef kMCCanvasRectangleTypeInfo;
extern MCTypeInfoRef kMCCanvasPointTypeInfo;
extern MCTypeInfoRef kMCCanvasColorTypeInfo;
extern MCTypeInfoRef kMCCanvasTransformTypeInfo;
extern MCTypeInfoRef kMCCanvasImageTypeInfo;
extern MCTypeInfoRef kMCCanvasPaintTypeInfo;
extern MCTypeInfoRef kMCCanvasGradientStopTypeInfo;
extern MCTypeInfoRef kMCCanvasPathTypeInfo;
extern MCTypeInfoRef kMCCanvasEffectTypeInfo;
extern MCTypeInfoRef kMCCanvasTypeInfo;

typedef MCCanvasPaintRef MCCanvasSolidPaintRef, MCCanvasPatternRef, MCCanvasGradientRef;

// Constant refs
extern MCCanvasTransformRef kMCCanvasIdentityTransform;

extern MCCanvasColorRef kMCCanvasColorBlack;

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle);
void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rectangle, MCGRectangle &r_rect);

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point);
void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point);

bool MCCanvasColorCreateWithRGBA(MCCanvasFloat red, MCCanvasFloat green, MCCanvasFloat blue, MCCanvasFloat alpha, MCCanvasColorRef &r_color);
void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red);
void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green);
void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue);
void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha);

bool MCCanvasTransformCreateWithMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform);
void MCCanvasTransformGetMCGAffineTransform(MCCanvasTransformRef p_transform, MCGAffineTransform &r_transform);

bool MCCanvasImageCreateWithImageRep(MCImageRep *p_rep, MCCanvasImageRef &r_image);
MCImageRep *MCCanvasImageGetImageRep(MCCanvasImageRef p_image);

bool MCCanvasPaintIsSolid(MCCanvasPaintRef p_paint);
bool MCCanvasPaintIsPattern(MCCanvasPaintRef p_paint);
bool MCCanvasPaintIsGradient(MCCanvasPaintRef p_paint);

bool MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasPaintRef &r_paint);
bool MCCanvasPatternCreateWithImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern);
bool MCCanvasGradientStopCreate(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop);
bool MCCanvasGradientCreateWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient);

bool MCCanvasPathCreateWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path);
MCGPathRef MCCanvasPathGetMCGPath(MCCanvasPathRef p_path);

bool MCCanvasGraphicEffectCreateWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);


bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas);

////////////////////////////////////////////////////////////////////////////////

// Rectangle

// Constructors
void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect);

// Properties
void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left);
void MCCanvasRectangleSetLeft(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_left);
void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top);
void MCCanvasRectangleSetTop(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_top);
void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right);
void MCCanvasRectangleSetRight(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_right);
void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom);
void MCCanvasRectangleSetBottom(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_bottom);
void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width);
void MCCanvasRectangleSetWidth(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_width);
void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height);
void MCCanvasRectangleSetHeight(MCCanvasRectangleRef &x_rect, MCCanvasFloat p_height);

//////////

// Point

// Constructors
void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point);

// Properties
void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x);
void MCCanvasPointSetX(MCCanvasPointRef &x_point, MCCanvasFloat p_x);
void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y);
void MCCanvasPointSetY(MCCanvasPointRef &x_point, MCCanvasFloat p_y);

//////////

// Color

// Constructors
void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);

//////////

// Transform

// Constructors
void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform);
void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform);
void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform);
void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
void MCCanvasTransformMakeWithMatrix(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform);

// Properties
void MCCanvasTransformGetMatrix(MCCanvasTransformRef p_transform, MCArrayRef &r_matrix);
void MCCanvasTransformSetMatrix(MCCanvasTransformRef &x_transform, MCArrayRef p_matrix);
void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
void MCCanvasTransformGetScale(MCCanvasTransformRef p_transform, MCGSize &r_scale);
void MCCanvasTransformSetScale(MCCanvasTransformRef &x_transform, const MCGSize &p_scale);
void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation);
void MCCanvasTransformSetRotation(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation);
void MCCanvasTransformGetSkew(MCCanvasTransformRef p_transform, MCGSize &r_skew);
void MCCanvasTransformSetSkew(MCCanvasTransformRef &x_transform, const MCGSize &p_skew);
void MCCanvasTransformGetTranslation(MCCanvasTransformRef p_transform, MCGSize &r_translation);
void MCCanvasTransformSetTranslation(MCCanvasTransformRef &x_transform, const MCGSize &p_translation);

// Operations
void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform_a, MCCanvasTransformRef p_transform_b);
void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation);
void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy);
void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_skew, MCCanvasFloat p_y_skew);

//////////

// Image

// Constructors
void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &x_image);
void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &x_image);
void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &x_image);

// Properties
void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width);
void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height);
void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels);
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
void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint);

// Properties
void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color);
void MCCanvasSolidPaintSetColor(MCCanvasSolidPaintRef &x_paint, MCCanvasColorRef p_color);

//////////

// Pattern

// Constructors
void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern);
void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern);
void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern);
void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern);
void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern);

// Properties
void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image);
void MCCanvasPatternSetImage(MCCanvasPatternRef &x_pattern, MCCanvasImageRef p_image);
void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform);
void MCCanvasPatternSetTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform);

// Operations
void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform);
void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle);
void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y);
// TODO - add skew?"

//////////

// Gradient Stop

// Constructors

void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop);

// Properties
// TODO - add gradient stop properties

// Gradient

// Constructors
void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient);

// Properties
void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp);
void MCCanvasGradientSetRamp(MCCanvasGradientRef &x_gradient, MCProperListRef p_ramp);
void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string);
void MCCanvasGradientSetTypeAsString(MCCanvasGradientRef &x_gradient, MCStringRef p_string);
void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat);
void MCCanvasGradientSetRepeat(MCCanvasGradientRef &x_gradient, integer_t p_repeat);
void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap);
void MCCanvasGradientSetWrap(MCCanvasGradientRef &x_gradient, bool p_wrap);
void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror);
void MCCanvasGradientSetMirror(MCCanvasGradientRef &x_gradient, bool p_mirror);
void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from);
void MCCanvasGradientSetFrom(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_from);
void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to);
void MCCanvasGradientSetTo(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_to);
void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via);
void MCCanvasGradientSetVia(MCCanvasGradientRef &x_gradient, MCCanvasPointRef p_via);
void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform);
void MCCanvasGradientSetTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform);

// Operators
void MCCanvasGradientAddStop(MCCanvasGradientRef &x_gradient, MCCanvasGradientStopRef p_stop);
void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform);
void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle);
void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y);

//////////

// Path

// Constructors
void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path);
void MCCanvasPathMakeWithPoints(MCProperListRef p_points, bool p_close, MCCanvasPathRef &r_path);

// Properties
void MCCanvasPathGetSubpaths(MCCanvasPathRef p_path, integer_t p_start, integer_t p_end, MCCanvasPathRef &r_subpaths);
void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds);
void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string);

// Operations
void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform);
void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle);
void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasPathAddPath(MCCanvasPathRef &x_path, MCCanvasPathRef p_to_add);
void MCCanvasPathMoveTo(MCCanvasPathRef &x_path, MCCanvasPointRef p_point);
void MCCanvasPathLineTo(MCCanvasPathRef &x_path, MCCanvasPointRef p_point);
void MCCanvasPathCurveThroughPoint(MCCanvasPathRef &x_path, MCCanvasPointRef p_through, MCCanvasPointRef p_to);
void MCCanvasPathCurveThroughPoints(MCCanvasPathRef &x_path, MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to);
void MCCanvasPathClosePath(MCCanvasPathRef &x_path);

//////////

// Effect

// Constructors
void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);

// Properties
void MCCanvasGraphicEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type);
void MCCanvasGraphicEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color);
void MCCanvasGraphicEffectSetColor(MCCanvasEffectRef &x_effect, MCCanvasColorRef p_color);
void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode);
void MCCanvasEffectSetBlendModeAsString(MCCanvasEffectRef &x_effect, MCStringRef p_blend_mode);
void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity);
void MCCanvasEffectSetOpacity(MCCanvasEffectRef &x_effect, MCCanvasFloat p_opacity);
void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size);
void MCCanvasEffectSetSize(MCCanvasEffectRef &x_effect, MCCanvasFloat p_size);
void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread);
void MCCanvasEffectSetSpread(MCCanvasEffectRef &x_effect, MCCanvasFloat p_spread);
void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance);
void MCCanvasEffectSetDistance(MCCanvasEffectRef &x_effect, MCCanvasFloat p_distance);
void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle);
void MCCanvasEffectSetAngle(MCCanvasEffectRef &x_effect, MCCanvasFloat p_angle);

//////////

// Canvas

// Properties
void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint);
void MCCanvasSetPaint(MCCanvasRef &x_canvas, MCCanvasPaintRef p_paint);
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
void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, MCCanvasTransformRef p_transform);
void MCCanvasCanvasScale(MCCanvasRef &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
void MCCanvasCanvasRotate(MCCanvasRef &x_canvas, MCCanvasFloat p_angle);
void MCCanvasCanvasTranslate(MCCanvasRef &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasCanvasSaveState(MCCanvasRef &x_canvas);
void MCCanvasCanvasRestore(MCCanvasRef &x_canvas);
void MCCanvasCanvasBeginLayer(MCCanvasRef &x_canvas);
// TODO - work out effect area rect
void MCCanvasCanvasBeginLayerWithEffect(MCCanvasRef &x_canvas, MCCanvasEffectRef p_effect, MCCanvasRectangleRef p_rect);
void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas);
void MCCanvasCanvasFill(MCCanvasRef &x_canvas);
void MCCanvasCanvasFillPath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path);
void MCCanvasCanvasStroke(MCCanvasRef &x_canvas);
void MCCanvasCanvasStrokePath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path);
void MCCanvasCanvasClipToRect(MCCanvasRef &x_canvas, MCCanvasRectangleRef p_rect);
void MCCanvasCanvasAddPath(MCCanvasRef &x_canvas, MCCanvasPathRef p_path);
void MCCanvasCanvasDrawRectOfImage(MCCanvasRef &x_canvas, MCCanvasImageRef p_image, MCCanvasRectangleRef p_src_rect, MCCanvasRectangleRef p_dst_rect, uint32_t p_quality);
void MCCanvasCanvasMoveTo(MCCanvasRef &x_canvas, MCCanvasPointRef p_point);
void MCCanvasCanvasLineTo(MCCanvasRef &x_canvas, MCCanvasPointRef p_point);
void MCCanvasCanvasCurveThroughPoint(MCCanvasRef &x_canvas, MCCanvasPointRef p_through, MCCanvasPointRef p_to);
void MCCanvasCanvasCurveThroughPoints(MCCanvasRef &x_canvas, MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to);
void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
