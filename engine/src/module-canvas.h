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
void MCCanvasRectangleSetLeft(MCCanvasFloat p_left, MCCanvasRectangleRef &x_rect);
void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top);
void MCCanvasRectangleSetTop(MCCanvasFloat p_top, MCCanvasRectangleRef &x_rect);
void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right);
void MCCanvasRectangleSetRight(MCCanvasFloat p_right, MCCanvasRectangleRef &x_rect);
void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom);
void MCCanvasRectangleSetBottom(MCCanvasFloat p_bottom, MCCanvasRectangleRef &x_rect);
void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width);
void MCCanvasRectangleSetWidth(MCCanvasFloat p_width, MCCanvasRectangleRef &x_rect);
void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height);
void MCCanvasRectangleSetHeight(MCCanvasFloat p_height, MCCanvasRectangleRef &x_rect);

//////////

// Point

// Constructors
void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point);

// Properties
void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x);
void MCCanvasPointSetX(MCCanvasFloat p_x, MCCanvasPointRef &x_point);
void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y);
void MCCanvasPointSetY(MCCanvasFloat p_y, MCCanvasPointRef &x_point);

//////////

// Color

// Constructors
void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);

// Properties
void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red);
void MCCanvasColorSetRed(MCCanvasFloat p_red, MCCanvasColorRef &x_color);
void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green);
void MCCanvasColorSetGreen(MCCanvasFloat p_green, MCCanvasColorRef &x_color);
void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue);
void MCCanvasColorSetBlue(MCCanvasFloat p_blue, MCCanvasColorRef &x_color);
void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha);
void MCCanvasColorSetAlpha(MCCanvasFloat p_alpha, MCCanvasColorRef &x_color);

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
void MCCanvasTransformSetMatrix(MCArrayRef p_matrix, MCCanvasTransformRef &x_transform);
void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
void MCCanvasTransformGetScale(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_scale);
void MCCanvasTransformSetScale(MCCanvasPointRef p_scale, MCCanvasTransformRef &x_transform);
void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation);
void MCCanvasTransformSetRotation(MCCanvasFloat p_rotation, MCCanvasTransformRef &x_transform);
void MCCanvasTransformGetSkew(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_skew);
void MCCanvasTransformSetSkew(MCCanvasPointRef p_skew, MCCanvasTransformRef &x_transform);
void MCCanvasTransformGetTranslation(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_translation);
void MCCanvasTransformSetTranslation(MCCanvasPointRef p_translation, MCCanvasTransformRef &x_transform);

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
void MCCanvasSolidPaintSetColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &x_paint);

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
void MCCanvasPatternSetImage(MCCanvasImageRef p_image, MCCanvasPatternRef &x_pattern);
void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform);
void MCCanvasPatternSetTransform(MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern);

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
void MCCanvasGradientSetRamp(MCProperListRef p_ramp, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string);
void MCCanvasGradientSetTypeAsString(MCStringRef p_string, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat);
void MCCanvasGradientSetRepeat(integer_t p_repeat, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap);
void MCCanvasGradientSetWrap(bool p_wrap, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror);
void MCCanvasGradientSetMirror(bool p_mirror, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from);
void MCCanvasGradientSetFrom(MCCanvasPointRef p_from, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to);
void MCCanvasGradientSetTo(MCCanvasPointRef p_to, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via);
void MCCanvasGradientSetVia(MCCanvasPointRef p_via, MCCanvasGradientRef &x_gradient);
void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform);
void MCCanvasGradientSetTransform(MCCanvasTransformRef p_transform, MCCanvasGradientRef &x_gradient);

// Operators
void MCCanvasGradientAddStop(MCCanvasGradientStopRef p_stop, MCCanvasGradientRef &x_gradient);
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
void MCCanvasPathAddPath(MCCanvasPathRef p_source, MCCanvasPathRef &x_dest);
void MCCanvasPathMoveTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
void MCCanvasPathLineTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
void MCCanvasPathCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
void MCCanvasPathCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
void MCCanvasPathClosePath(MCCanvasPathRef &x_path);

//////////

// Effect

// Constructors
void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);

// Properties
void MCCanvasGraphicEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type);
void MCCanvasGraphicEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color);
void MCCanvasGraphicEffectSetColor(MCCanvasColorRef p_color, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode);
void MCCanvasEffectSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity);
void MCCanvasEffectSetOpacity(MCCanvasFloat p_opacity, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size);
void MCCanvasEffectSetSize(MCCanvasFloat p_size, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread);
void MCCanvasEffectSetSpread(MCCanvasFloat p_spread, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance);
void MCCanvasEffectSetDistance(MCCanvasFloat p_distance, MCCanvasEffectRef &x_effect);
void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle);
void MCCanvasEffectSetAngle(MCCanvasFloat p_angle, MCCanvasEffectRef &x_effect);

//////////

// Canvas

// Properties
void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint);
void MCCanvasSetPaint(MCCanvasPaintRef p_paint, MCCanvasRef &x_canvas);
void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string);
void MCCanvasSetFillRuleAsString(MCStringRef p_string, MCCanvasRef &x_canvas);
void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias);
void MCCanvasSetAntialias(bool p_antialias, MCCanvasRef &x_canvas);
void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity);
void MCCanvasSetOpacity(MCCanvasFloat p_opacity, MCCanvasRef &x_canvas);
void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode);
void MCCanvasSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasRef &x_canvas);
void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled);
void MCCanvasSetStippled(bool p_stippled, MCCanvasRef &x_canvas);
void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality);
void MCCanvasSetImageResizeQualityAsString(MCStringRef p_quality, MCCanvasRef &x_canvas);

// Operations
void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, MCCanvasTransformRef p_transform);
void MCCanvasCanvasScale(MCCanvasRef &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
void MCCanvasCanvasRotate(MCCanvasRef &x_canvas, MCCanvasFloat p_angle);
void MCCanvasCanvasTranslate(MCCanvasRef &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
void MCCanvasCanvasSaveState(MCCanvasRef &x_canvas);
void MCCanvasCanvasRestore(MCCanvasRef &x_canvas);
void MCCanvasCanvasBeginLayer(MCCanvasRef &x_canvas);
// TODO - work out effect area rect
void MCCanvasCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRectangleRef p_rect, MCCanvasRef &x_canvas);
void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas);
void MCCanvasCanvasFill(MCCanvasRef &x_canvas);
void MCCanvasCanvasFillPath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
void MCCanvasCanvasStroke(MCCanvasRef &x_canvas);
void MCCanvasCanvasStrokePath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
void MCCanvasCanvasClipToRect(MCCanvasRectangleRef p_rect, MCCanvasRef &x_canvas);
void MCCanvasCanvasAddPath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
void MCCanvasCanvasDrawRectOfImage(MCCanvasRectangleRef p_src_rect, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef &x_canvas);
void MCCanvasCanvasMoveTo(MCCanvasPointRef p_point, MCCanvasRef &x_canvas);
void MCCanvasCanvasLineTo(MCCanvasPointRef p_point, MCCanvasRef &x_canvas);
void MCCanvasCanvasCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasRef &x_canvas);
void MCCanvasCanvasCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasRef &x_canvas);
void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
