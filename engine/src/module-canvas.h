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
bool MCCanvasModuleInitialize();
void MCCanvasModuleFinalize();

void MCCanvasPush(MCGContextRef gcontext);
void MCCanvasPop(void);

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
// Canvas Errors

extern MCTypeInfoRef kMCCanvasRectangleListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasPointListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasColorListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasScaleListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasTranslationListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasSkewListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasRadiiListFormatErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasImageSizeListFormatErrorTypeInfo;

extern MCTypeInfoRef kMCCanvasSolidPaintTypeErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasPatternPaintTypeErrorTypeInfo;
extern MCTypeInfoRef kMCCanvasGradientPaintTypeErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle);
void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rectangle, MCGRectangle &r_rect);

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point);
void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point);

bool MCCanvasColorCreateWithRGBA(MCCanvasFloat red, MCCanvasFloat green, MCCanvasFloat blue, MCCanvasFloat alpha, MCCanvasColorRef &r_color);

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


extern "C" bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas);

////////////////////////////////////////////////////////////////////////////////

// Rectangle

// Constructors
extern "C" void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect);
extern "C" void MCCanvasRectangleMakeWithList(MCProperListRef p_list, MCCanvasRectangleRef &r_point);

// Properties
extern "C" void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left);
extern "C" void MCCanvasRectangleSetLeft(MCCanvasFloat p_left, MCCanvasRectangleRef &x_rect);
extern "C" void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top);
extern "C" void MCCanvasRectangleSetTop(MCCanvasFloat p_top, MCCanvasRectangleRef &x_rect);
extern "C" void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right);
extern "C" void MCCanvasRectangleSetRight(MCCanvasFloat p_right, MCCanvasRectangleRef &x_rect);
extern "C" void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom);
extern "C" void MCCanvasRectangleSetBottom(MCCanvasFloat p_bottom, MCCanvasRectangleRef &x_rect);
extern "C" void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width);
extern "C" void MCCanvasRectangleSetWidth(MCCanvasFloat p_width, MCCanvasRectangleRef &x_rect);
extern "C" void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height);
extern "C" void MCCanvasRectangleSetHeight(MCCanvasFloat p_height, MCCanvasRectangleRef &x_rect);

//////////

// Point

// Constructors
extern "C" void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point);
extern "C" void MCCanvasPointMakeWithList(MCProperListRef p_list, MCCanvasPointRef &r_list);

// Properties
extern "C" void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x);
extern "C" void MCCanvasPointSetX(MCCanvasFloat p_x, MCCanvasPointRef &x_point);
extern "C" void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y);
extern "C" void MCCanvasPointSetY(MCCanvasFloat p_y, MCCanvasPointRef &x_point);

//////////

// Color

// Constructors
extern "C" void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);
extern "C" void MCCanvasColorMakeWithList(MCProperListRef p_list, MCCanvasColorRef &r_color);

// Properties
extern "C" void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red);
extern "C" void MCCanvasColorSetRed(MCCanvasFloat p_red, MCCanvasColorRef &x_color);
extern "C" void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green);
extern "C" void MCCanvasColorSetGreen(MCCanvasFloat p_green, MCCanvasColorRef &x_color);
extern "C" void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue);
extern "C" void MCCanvasColorSetBlue(MCCanvasFloat p_blue, MCCanvasColorRef &x_color);
extern "C" void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha);
extern "C" void MCCanvasColorSetAlpha(MCCanvasFloat p_alpha, MCCanvasColorRef &x_color);

//////////

// Transform

// Constructors
extern "C" void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeScaleWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeTranslationWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeSkewWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasTransformMakeWithMatrixValues(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform);

// Properties
extern "C" void MCCanvasTransformGetMatrix(MCCanvasTransformRef p_transform, MCArrayRef &r_matrix);
extern "C" void MCCanvasTransformSetMatrix(MCArrayRef p_matrix, MCCanvasTransformRef &x_transform);
extern "C" void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
extern "C" void MCCanvasTransformGetScale(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_scale);
extern "C" void MCCanvasTransformSetScale(MCCanvasPointRef p_scale, MCCanvasTransformRef &x_transform);
extern "C" void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation);
extern "C" void MCCanvasTransformSetRotation(MCCanvasFloat p_rotation, MCCanvasTransformRef &x_transform);
extern "C" void MCCanvasTransformGetSkew(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_skew);
extern "C" void MCCanvasTransformSetSkew(MCCanvasPointRef p_skew, MCCanvasTransformRef &x_transform);
extern "C" void MCCanvasTransformGetTranslation(MCCanvasTransformRef p_transform, MCCanvasPointRef &r_translation);
extern "C" void MCCanvasTransformSetTranslation(MCCanvasPointRef p_translation, MCCanvasTransformRef &x_transform);

// Operations
extern "C" void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform_a, MCCanvasTransformRef p_transform_b);
extern "C" void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
extern "C" void MCCanvasTransformScaleWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);
extern "C" void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation);
extern "C" void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy);
extern "C" void MCCanvasTransformTranslateWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);
extern "C" void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_skew, MCCanvasFloat p_y_skew);
extern "C" void MCCanvasTransformSkewWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);

//////////

// Image

// Constructors
extern "C" void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &x_image);
extern "C" void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &x_image);
extern "C" void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &x_image);
extern "C" void MCCanvasImageMakeWithPixelsWithSizeAsList(MCProperListRef p_size, MCDataRef p_pixels, MCCanvasImageRef &x_image);

// Properties
extern "C" void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width);
extern "C" void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height);
extern "C" void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels);
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
extern "C" void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint);

// Properties
extern "C" void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color);
extern "C" void MCCanvasSolidPaintSetColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &x_paint);

//////////

// Pattern

// Constructors
extern "C" void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithImageScaledWithList(MCCanvasImageRef p_image, MCProperListRef p_list, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern);
extern "C" void MCCanvasPatternMakeWithImageTranslatedWithList(MCCanvasImageRef p_image, MCProperListRef p_list, MCCanvasPatternRef &r_pattern);

// Properties
extern "C" void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image);
extern "C" void MCCanvasPatternSetImage(MCCanvasImageRef p_image, MCCanvasPatternRef &x_pattern);
extern "C" void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasPatternSetTransform(MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern);

// Operations
extern "C" void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform);
extern "C" void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" void MCCanvasPatternScaleWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_scale);
extern "C" void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle);
extern "C" void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" void MCCanvasPatternTranslateWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_translation);
// TODO - add skew?"

//////////

// Gradient Stop

// Constructors
extern "C" void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop);

// Properties
extern "C" void MCCanvasGradientStopGetOffset(MCCanvasGradientStopRef p_stop, MCCanvasFloat &r_offset);
extern "C" void MCCanvasGradientStopSetOffset(MCCanvasFloat p_offset, MCCanvasGradientStopRef &x_stop);
extern "C" void MCCanvasGradientStopGetColor(MCCanvasGradientStopRef p_stop, MCCanvasColorRef &r_color);
extern "C" void MCCanvasGradientStopSetColor(MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop);

// Gradient

// Constructors
extern "C" void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient);

// Properties
extern "C" void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp);
extern "C" void MCCanvasGradientSetRamp(MCProperListRef p_ramp, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string);
extern "C" void MCCanvasGradientSetTypeAsString(MCStringRef p_string, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat);
extern "C" void MCCanvasGradientSetRepeat(integer_t p_repeat, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap);
extern "C" void MCCanvasGradientSetWrap(bool p_wrap, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror);
extern "C" void MCCanvasGradientSetMirror(bool p_mirror, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from);
extern "C" void MCCanvasGradientSetFrom(MCCanvasPointRef p_from, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to);
extern "C" void MCCanvasGradientSetTo(MCCanvasPointRef p_to, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via);
extern "C" void MCCanvasGradientSetVia(MCCanvasPointRef p_via, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform);
extern "C" void MCCanvasGradientSetTransform(MCCanvasTransformRef p_transform, MCCanvasGradientRef &x_gradient);

// Operators
extern "C" void MCCanvasGradientAddStop(MCCanvasGradientStopRef p_stop, MCCanvasGradientRef &x_gradient);
extern "C" void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform);
extern "C" void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" void MCCanvasGradientScaleWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_scale);
extern "C" void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle);
extern "C" void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" void MCCanvasGradientTranslateWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_translation);

//////////

// Path

// Constructors
extern "C" void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_radius, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithRoundedRectangleWithRadii(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithRoundedRectangleWithRadiiAsList(MCCanvasRectangleRef p_rect, MCProperListRef p_radii, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithEllipseWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithCircle(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path);
extern "C" void MCCanvasPathMakeWithPoints(bool p_close, MCProperListRef p_points, MCCanvasPathRef &r_path);

// Properties
extern "C" void MCCanvasPathGetSubpaths(integer_t p_start, integer_t p_end, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths);
extern "C" void MCCanvasPathGetSubpath(integer_t p_index, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths);
extern "C" void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds);
extern "C" void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string);

// Operations
extern "C" void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform);
extern "C" void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" void MCCanvasPathScaleWithList(MCCanvasPathRef &x_path, MCProperListRef p_scale);
extern "C" void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle);
extern "C" void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" void MCCanvasPathTranslateWithList(MCCanvasPathRef &x_path, MCProperListRef p_translation);
extern "C" void MCCanvasPathAddPath(MCCanvasPathRef p_source, MCCanvasPathRef &x_dest);
extern "C" void MCCanvasPathMoveTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
extern "C" void MCCanvasPathLineTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
extern "C" void MCCanvasPathCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
extern "C" void MCCanvasPathCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
extern "C" void MCCanvasPathClosePath(MCCanvasPathRef &x_path);

//////////

// Effect

// Constructors
extern "C" void MCCanvasGraphicEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);

// Properties
extern "C" void MCCanvasGraphicEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type);
extern "C" void MCCanvasGraphicEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color);
extern "C" void MCCanvasGraphicEffectSetColor(MCCanvasColorRef p_color, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode);
extern "C" void MCCanvasEffectSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity);
extern "C" void MCCanvasEffectSetOpacity(MCCanvasFloat p_opacity, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size);
extern "C" void MCCanvasEffectSetSize(MCCanvasFloat p_size, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread);
extern "C" void MCCanvasEffectSetSpread(MCCanvasFloat p_spread, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance);
extern "C" void MCCanvasEffectSetDistance(MCCanvasFloat p_distance, MCCanvasEffectRef &x_effect);
extern "C" void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle);
extern "C" void MCCanvasEffectSetAngle(MCCanvasFloat p_angle, MCCanvasEffectRef &x_effect);

//////////

// Canvas

// Properties
extern "C" void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint);
extern "C" void MCCanvasSetPaint(MCCanvasPaintRef p_paint, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string);
extern "C" void MCCanvasSetFillRuleAsString(MCStringRef p_string, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias);
extern "C" void MCCanvasSetAntialias(bool p_antialias, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity);
extern "C" void MCCanvasSetOpacity(MCCanvasFloat p_opacity, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode);
extern "C" void MCCanvasSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled);
extern "C" void MCCanvasSetStippled(bool p_stippled, MCCanvasRef &x_canvas);
extern "C" void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality);
extern "C" void MCCanvasSetImageResizeQualityAsString(MCStringRef p_quality, MCCanvasRef &x_canvas);

// Operations
extern "C" void MCCanvasCanvasTransform(MCCanvasRef &x_canvas, MCCanvasTransformRef p_transform);
extern "C" void MCCanvasCanvasScale(MCCanvasRef &x_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
extern "C" void MCCanvasCanvasScaleWithList(MCCanvasRef &x_canvas, MCProperListRef p_scale);
extern "C" void MCCanvasCanvasRotate(MCCanvasRef &x_canvas, MCCanvasFloat p_angle);
extern "C" void MCCanvasCanvasTranslate(MCCanvasRef &x_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" void MCCanvasCanvasTranslateWithList(MCCanvasRef &x_canvas, MCProperListRef p_translation);
extern "C" void MCCanvasCanvasSaveState(MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasRestore(MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasBeginLayer(MCCanvasRef &x_canvas);
// TODO - work out effect area rect
extern "C" void MCCanvasCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRectangleRef p_rect, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasEndLayer(MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasFill(MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasFillPath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasStroke(MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasStrokePath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasClipToRect(MCCanvasRectangleRef p_rect, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasAddPath(MCCanvasPathRef p_path, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasDrawRectOfImage(MCCanvasRectangleRef p_src_rect, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasMoveTo(MCCanvasPointRef p_point, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasLineTo(MCCanvasPointRef p_point, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasRef &x_canvas);
extern "C" void MCCanvasCanvasClosePath(MCCanvasRef &x_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
