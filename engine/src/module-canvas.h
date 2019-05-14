/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#include "foundation.h"
#include "graphics.h"
#include "font.h"

////////////////////////////////////////////////////////////////////////////////

// TODO - move to MCImageRep wrapper library
#include "imagebitmap.h"
#include "image.h"
#include "image_rep.h"

/*struct MCGImageFrame
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
};*/

class MCImageRep;

MCImageRep *MCImageRepRetain(MCImageRep *p_image_rep);
void MCImageRepRelease(MCImageRep *p_image_rep);
bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRep *&r_image_rep);
bool MCImageRepCreateWithPixels(MCDataRef p_pixels, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied, MCImageRep *&r_image_rep);
bool MCImageRepGetGeometry(MCImageRep *p_image_rep, uint32_t &r_width, uint32_t &r_height);
bool MCImageRepGetMetadata(MCImageRep *p_image_rep, MCArrayRef &r_metadata);
bool MCImageRepGetDensity(MCImageRep *p_image_rep, double &r_density);
bool MCImageRepGetFrameDuration(MCImageRep *p_image_rep, uint32_t p_frame, uint32_t &r_duration);
bool MCImageRepGetMetadata(MCImageRep *p_image_rep, MCArrayRef &r_metadata);

bool MCImageRepLock(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCGImageFrame &r_frame);
void MCImageRepUnlock(MCImageRep *p_image_rep, uint32_t p_index, MCGImageFrame &p_frame);

bool MCImageRepLockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCImageBitmap *&r_raster);
void MCImageRepUnlockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCImageBitmap *p_raster);

void MCImageRepRender(MCImageRep *p_image_rep, MCGContextRef p_gcontext, uint32_t p_index, MCGRectangle p_src_rect, MCGRectangle p_dst_rect, MCGImageFilter p_filter, MCGPaintRef p_current_color);

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

void MCCanvasPush(MCGContextRef gcontext, uintptr_t& r_cookie);
void MCCanvasPop(uintptr_t p_cookie);

////////////////////////////////////////////////////////////////////////////////
// Type Definitions

extern "C"
{

typedef MCGFloat MCCanvasFloat;

typedef struct __MCCanvasRectangle *MCCanvasRectangleRef;
typedef struct __MCCanvasPoint *MCCanvasPointRef;
typedef struct __MCCanvasColor *MCCanvasColorRef;
typedef struct __MCCanvasTransform *MCCanvasTransformRef;
typedef struct __MCCanvasImage *MCCanvasImageRef;
typedef struct __MCCanvasPaint *MCCanvasPaintRef;
typedef struct __MCCanvasSolidPaint *MCCanvasSolidPaintRef;
typedef struct __MCCanvasPattern *MCCanvasPatternRef;
typedef struct __MCCanvasGradient *MCCanvasGradientRef;
typedef struct __MCCanvasGradientStop *MCCanvasGradientStopRef;
typedef struct __MCCanvasPath *MCCanvasPathRef;
typedef struct __MCCanvasEffect *MCCanvasEffectRef;
typedef struct __MCCanvasFont *MCCanvasFontRef;
typedef struct __MCCanvas *MCCanvasRef;

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasRectangleTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPointTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasColorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasTransformTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPaintTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasSolidPaintTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPatternTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasGradientTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasGradientStopTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPathTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasEffectTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasFontTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasTypeInfo;

// Constant refs
extern MCCanvasTransformRef kMCCanvasIdentityTransform;
extern MCCanvasColorRef kMCCanvasColorBlack;
//extern MCCanvasFontRef kMCCanvasFont12PtHelvetica;
extern MCCanvasPathRef kMCCanvasEmptyPath;
	
////////////////////////////////////////////////////////////////////////////////
// Canvas Errors

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasRectangleListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPointListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasColorListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasScaleListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasTranslationListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasSkewListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasRadiiListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageSizeListFormatErrorTypeInfo;

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasTransformMatrixListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasTransformDecomposeErrorTypeInfo;

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepReferencedErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepDataErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepPixelsErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepGetGeometryErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepGetMetadataErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepGetDensityErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasImageRepLockErrorTypeInfo;
	
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasGradientStopRangeErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasGradientStopOrderErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasGradientTypeErrorTypeInfo;

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasEffectInvalidPropertyErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasEffectPropertyNotAvailableErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasEffectPropertyInvalidValueErrorTypeInfo;

extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasPathPointListFormatErrorTypeInfo;
extern MC_DLLEXPORT MCTypeInfoRef kMCCanvasSVGPathParseErrorTypeInfo;

}

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle);
void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rectangle, MCGRectangle &r_rect);

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point);
void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point);

bool MCCanvasColorCreateWithRGBA(MCCanvasFloat red, MCCanvasFloat green, MCCanvasFloat blue, MCCanvasFloat alpha, MCCanvasColorRef &r_color);

bool MCCanvasTransformCreateWithMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform);

bool MCCanvasImageCreateWithImageRep(MCImageRep *p_rep, MCCanvasImageRef &r_image);
MCImageRep *MCCanvasImageGetImageRep(MCCanvasImageRef p_image);

bool MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint);
bool MCCanvasPatternCreateWithImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern);
bool MCCanvasGradientStopCreate(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop);
bool MCCanvasGradientCreateWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient);

bool MCCanvasPathCreateWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path);
MCGPathRef MCCanvasPathGetMCGPath(MCCanvasPathRef p_path);

bool MCCanvasEffectCreateWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);

bool MCCanvasFontCreateWithMCFont(MCFontRef p_font, MCCanvasFontRef &r_font);
MCFontRef MCCanvasFontGetMCFont(MCCanvasFontRef p_font);
bool MCCanvasFontGetDefault(MCFontRef &r_font);

extern "C" MC_DLLEXPORT bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas);

////////////////////////////////////////////////////////////////////////////////

// Rectangle

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleMakeWithList(MCProperListRef p_list, MCCanvasRectangleRef &r_point);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetLeft(MCCanvasFloat p_left, MCCanvasRectangleRef &x_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetTop(MCCanvasFloat p_top, MCCanvasRectangleRef &x_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetRight(MCCanvasFloat p_right, MCCanvasRectangleRef &x_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetBottom(MCCanvasFloat p_bottom, MCCanvasRectangleRef &x_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetWidth(MCCanvasFloat p_width, MCCanvasRectangleRef &x_rect);
extern "C" MC_DLLEXPORT void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height);
extern "C" MC_DLLEXPORT void MCCanvasRectangleSetHeight(MCCanvasFloat p_height, MCCanvasRectangleRef &x_rect);

//////////

// Point

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point);
extern "C" MC_DLLEXPORT void MCCanvasPointMakeWithList(MCProperListRef p_list, MCCanvasPointRef &r_list);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x);
extern "C" MC_DLLEXPORT void MCCanvasPointSetX(MCCanvasFloat p_x, MCCanvasPointRef &x_point);
extern "C" MC_DLLEXPORT void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y);
extern "C" MC_DLLEXPORT void MCCanvasPointSetY(MCCanvasFloat p_y, MCCanvasPointRef &x_point);

//////////

// Color

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color);
extern "C" MC_DLLEXPORT void MCCanvasColorMakeWithList(MCProperListRef p_list, MCCanvasColorRef &r_color);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red);
extern "C" MC_DLLEXPORT void MCCanvasColorSetRed(MCCanvasFloat p_red, MCCanvasColorRef &x_color);
extern "C" MC_DLLEXPORT void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green);
extern "C" MC_DLLEXPORT void MCCanvasColorSetGreen(MCCanvasFloat p_green, MCCanvasColorRef &x_color);
extern "C" MC_DLLEXPORT void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue);
extern "C" MC_DLLEXPORT void MCCanvasColorSetBlue(MCCanvasFloat p_blue, MCCanvasColorRef &x_color);
extern "C" MC_DLLEXPORT void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha);
extern "C" MC_DLLEXPORT void MCCanvasColorSetAlpha(MCCanvasFloat p_alpha, MCCanvasColorRef &x_color);

//////////

// Transform

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeScaleWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeTranslationWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeSkewWithList(MCProperListRef p_list, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeWithMatrixAsList(MCProperListRef p_matrix, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformMakeWithMatrixValues(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasTransformGetMatrixAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_matrix);
extern "C" MC_DLLEXPORT void MCCanvasTransformSetMatrixAsList(MCProperListRef p_matrix, MCCanvasTransformRef &x_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform);
// T = Tscale * Trotate * Tskew * Ttranslate
extern "C" MC_DLLEXPORT void MCCanvasTransformGetScaleAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_scale);
extern "C" MC_DLLEXPORT void MCCanvasTransformSetScaleAsList(MCProperListRef p_scale, MCCanvasTransformRef &x_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation);
extern "C" MC_DLLEXPORT void MCCanvasTransformSetRotation(MCCanvasFloat p_rotation, MCCanvasTransformRef &x_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformGetSkewAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_skew);
extern "C" MC_DLLEXPORT void MCCanvasTransformSetSkewAsList(MCProperListRef p_skew, MCCanvasTransformRef &x_transform);
extern "C" MC_DLLEXPORT void MCCanvasTransformGetTranslationAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_translation);
extern "C" MC_DLLEXPORT void MCCanvasTransformSetTranslationAsList(MCProperListRef p_translation, MCCanvasTransformRef &x_transform);

// Operations
extern "C" MC_DLLEXPORT void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform_a, MCCanvasTransformRef p_transform_b);
extern "C" MC_DLLEXPORT void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
extern "C" MC_DLLEXPORT void MCCanvasTransformScaleWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);
extern "C" MC_DLLEXPORT void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation);
extern "C" MC_DLLEXPORT void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy);
extern "C" MC_DLLEXPORT void MCCanvasTransformTranslateWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);
extern "C" MC_DLLEXPORT void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_skew, MCCanvasFloat p_y_skew);
extern "C" MC_DLLEXPORT void MCCanvasTransformSkewWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_list);
extern "C" MC_DLLEXPORT void MCCanvasTransformMultiply(MCCanvasTransformRef p_left, MCCanvasTransformRef p_right, MCCanvasTransformRef &r_transform);

//////////

// Image

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &x_image);
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &x_image);
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &x_image);
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithPixelsInFormat(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCGPixelFormat p_format, MCCanvasImageRef &x_image);
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithPixelsWithSizeAsList(MCProperListRef p_size, MCDataRef p_pixels, MCCanvasImageRef &x_image);
extern "C" MC_DLLEXPORT void MCCanvasImageMakeWithResourceFile(MCStringRef p_resource, MCCanvasImageRef &r_image);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width);
extern "C" MC_DLLEXPORT void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height);
extern "C" MC_DLLEXPORT void MCCanvasImageGetMetadata(MCCanvasImageRef p_image, MCArrayRef &r_metadata);
extern "C" MC_DLLEXPORT void MCCanvasImageGetDensity(MCCanvasImageRef p_image, double &r_density);
extern "C" MC_DLLEXPORT void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels);

// TODO - Implement image operations
//void MCCanvasImageTransform(MCCanvasImage &x_image, const MCCanvasTransform &p_transform);
//void MCCanvasImageScale(MCCanvasImage &x_image, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale);
//void MCCanvasImageRotate(MCCanvasImage &x_image, MCCanvasFloat p_angle);
//void MCCanvasImageCrop(MCCanvasImage &x_image, const MCCanvasRectangle &p_rect);

//////////

// Solid Paint

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color);
extern "C" MC_DLLEXPORT void MCCanvasSolidPaintSetColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &x_paint);

//////////

// Pattern

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithImageScaledWithList(MCCanvasImageRef p_image, MCProperListRef p_list, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternMakeWithImageTranslatedWithList(MCCanvasImageRef p_image, MCProperListRef p_list, MCCanvasPatternRef &r_pattern);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image);
extern "C" MC_DLLEXPORT void MCCanvasPatternSetImage(MCCanvasImageRef p_image, MCCanvasPatternRef &x_pattern);
extern "C" MC_DLLEXPORT void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasPatternSetTransform(MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern);

// Operations
extern "C" MC_DLLEXPORT void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform);
extern "C" MC_DLLEXPORT void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" MC_DLLEXPORT void MCCanvasPatternScaleWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_scale);
extern "C" MC_DLLEXPORT void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle);
extern "C" MC_DLLEXPORT void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" MC_DLLEXPORT void MCCanvasPatternTranslateWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_translation);
// TODO - add skew?"

//////////

// Gradient Stop

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasGradientStopGetOffset(MCCanvasGradientStopRef p_stop, MCCanvasFloat &r_offset);
extern "C" MC_DLLEXPORT void MCCanvasGradientStopSetOffset(MCCanvasFloat p_offset, MCCanvasGradientStopRef &x_stop);
extern "C" MC_DLLEXPORT void MCCanvasGradientStopGetColor(MCCanvasGradientStopRef p_stop, MCCanvasColorRef &r_color);
extern "C" MC_DLLEXPORT void MCCanvasGradientStopSetColor(MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop);

// Gradient

extern "C" MC_DLLEXPORT void MCCanvasGradientEvaluateType(integer_t p_type, integer_t& r_type);

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetRamp(MCProperListRef p_ramp, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetTypeAsString(MCStringRef p_string, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetRepeat(integer_t p_repeat, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetWrap(bool p_wrap, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetMirror(bool p_mirror, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetFrom(MCCanvasPointRef p_from, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetTo(MCCanvasPointRef p_to, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetVia(MCCanvasPointRef p_via, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform);
extern "C" MC_DLLEXPORT void MCCanvasGradientSetTransform(MCCanvasTransformRef p_transform, MCCanvasGradientRef &x_gradient);

// Operators
extern "C" MC_DLLEXPORT void MCCanvasGradientAddStop(MCCanvasGradientStopRef p_stop, MCCanvasGradientRef &x_gradient);
extern "C" MC_DLLEXPORT void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform);
extern "C" MC_DLLEXPORT void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" MC_DLLEXPORT void MCCanvasGradientScaleWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_scale);
extern "C" MC_DLLEXPORT void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle);
extern "C" MC_DLLEXPORT void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" MC_DLLEXPORT void MCCanvasGradientTranslateWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_translation);

//////////

// Path

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasPathMakeEmpty(MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_radius, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithRoundedRectangleWithRadii(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithRoundedRectangleWithRadiiAsList(MCCanvasRectangleRef p_rect, MCProperListRef p_radii, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithEllipseWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithCircle(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithPoints(bool p_close, MCProperListRef p_points, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithArcWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithArcWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithSectorWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithSectorWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithSegmentWithRadius(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);
extern "C" MC_DLLEXPORT void MCCanvasPathMakeWithSegmentWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasFloat p_start_angle, MCCanvasFloat p_end_angle, MCCanvasPathRef &r_path);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasPathGetSubpaths(integer_t p_start, integer_t p_end, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths);
extern "C" MC_DLLEXPORT void MCCanvasPathGetSubpath(integer_t p_index, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths);
extern "C" MC_DLLEXPORT void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds);
extern "C" MC_DLLEXPORT void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string);

// Operations
extern "C" MC_DLLEXPORT void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform);
extern "C" MC_DLLEXPORT void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale);
extern "C" MC_DLLEXPORT void MCCanvasPathScaleWithList(MCCanvasPathRef &x_path, MCProperListRef p_scale);
extern "C" MC_DLLEXPORT void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle);
extern "C" MC_DLLEXPORT void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" MC_DLLEXPORT void MCCanvasPathTranslateWithList(MCCanvasPathRef &x_path, MCProperListRef p_translation);
extern "C" MC_DLLEXPORT void MCCanvasPathAddPath(MCCanvasPathRef p_source, MCCanvasPathRef &x_dest);
extern "C" MC_DLLEXPORT void MCCanvasPathMoveTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathLineTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathClosePath(MCCanvasPathRef &x_path);

extern "C" MC_DLLEXPORT void MCCanvasPathArcTo(MCCanvasPointRef p_tangent, MCCanvasPointRef p_to, MCCanvasFloat p_radius, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathEllipticArcToWithFlagsWithRadiiAsList(MCCanvasPointRef p_to, MCProperListRef p_radii, MCCanvasFloat p_rotation, bool p_largest, bool p_clockwise, MCCanvasPathRef &x_path);
extern "C" MC_DLLEXPORT void MCCanvasPathEllipticArcToWithRadiiAsList(MCCanvasPointRef p_to, MCProperListRef p_radii, MCCanvasFloat p_rotation, MCCanvasPathRef &x_path);

//////////

// Effect

extern "C" MC_DLLEXPORT void MCCanvasEffectEvaluateType(integer_t p_type, integer_t &r_type);

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasEffectMake(integer_t p_type, MCCanvasEffectRef &r_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetColor(MCCanvasColorRef p_color, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetOpacity(MCCanvasFloat p_opacity, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetSize(MCCanvasFloat p_size, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetSpread(MCCanvasFloat p_spread, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetDistance(MCCanvasFloat p_distance, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetAngle(MCCanvasFloat p_angle, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetKnockOut(MCCanvasEffectRef p_effect, bool &r_knockout);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetKnockOut(bool p_knockout, MCCanvasEffectRef &x_effect);
extern "C" MC_DLLEXPORT void MCCanvasEffectGetSourceAsString(MCCanvasEffectRef p_effect, MCStringRef &r_source);
extern "C" MC_DLLEXPORT void MCCanvasEffectSetSourceAsString(MCStringRef p_source, MCCanvasEffectRef &x_effect);

//////////

// Font

// Constructors
extern "C" MC_DLLEXPORT void MCCanvasFontMake(MCStringRef p_name, MCCanvasFontRef &r_font);
extern "C" MC_DLLEXPORT void MCCanvasFontMakeWithStyle(MCStringRef p_name, bool p_bold, bool p_italic, MCCanvasFontRef &r_font);
extern "C" MC_DLLEXPORT void MCCanvasFontMakeWithSize(MCStringRef p_name, bool p_bold, bool p_italic, integer_t p_size, MCCanvasFontRef &r_font);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasFontGetName(MCCanvasFontRef p_font, MCStringRef &r_name);
extern "C" MC_DLLEXPORT void MCCanvasFontSetName(MCStringRef p_name, MCCanvasFontRef &x_font);
extern "C" MC_DLLEXPORT void MCCanvasFontGetBold(MCCanvasFontRef p_font, bool &r_bold);
extern "C" MC_DLLEXPORT void MCCanvasFontSetBold(bool p_bold, MCCanvasFontRef &x_font);
extern "C" MC_DLLEXPORT void MCCanvasFontGetItalic(MCCanvasFontRef p_font, bool &r_italic);
extern "C" MC_DLLEXPORT void MCCanvasFontSetItalic(bool p_italic, MCCanvasFontRef &x_font);
extern "C" MC_DLLEXPORT void MCCanvasFontGetSize(MCCanvasFontRef p_font, uinteger_t &r_size);
extern "C" MC_DLLEXPORT void MCCanvasFontSetSize(uinteger_t p_size, MCCanvasFontRef &x_font);
extern "C" MC_DLLEXPORT void MCCanvasFontGetHandle(MCCanvasFontRef p_font, void*& r_handle);

// Operations
extern "C" MC_DLLEXPORT void MCCanvasFontMeasureTextTypographicBounds(MCStringRef p_text, MCCanvasFontRef p_font, MCCanvasRectangleRef& r_rect);
extern "C" MC_DLLEXPORT void MCCanvasFontMeasureTextTypographicBoundsOnCanvas(MCStringRef p_text, MCCanvasRef p_canvas, MCCanvasRectangleRef& r_rect);
extern "C" MC_DLLEXPORT void MCCanvasFontMeasureTextImageBounds(MCStringRef p_text, MCCanvasFontRef p_font, MCCanvasRectangleRef& r_rect);
extern "C" MC_DLLEXPORT void MCCanvasFontMeasureTextImageBoundsOnCanvas(MCStringRef p_text, MCCanvasRef p_canvas, MCCanvasRectangleRef& r_rect);

//////////

// Canvas

extern "C" MC_DLLEXPORT void MCCanvasAlignmentEvaluate(integer_t p_h_align, integer_t p_v_align, integer_t &r_align);

// Properties
extern "C" MC_DLLEXPORT void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint);
extern "C" MC_DLLEXPORT void MCCanvasSetPaint(MCCanvasPaintRef p_paint, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string);
extern "C" MC_DLLEXPORT void MCCanvasSetFillRuleAsString(MCStringRef p_string, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias);
extern "C" MC_DLLEXPORT void MCCanvasSetAntialias(bool p_antialias, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity);
extern "C" MC_DLLEXPORT void MCCanvasSetOpacity(MCCanvasFloat p_opacity, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode);
extern "C" MC_DLLEXPORT void MCCanvasSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled);
extern "C" MC_DLLEXPORT void MCCanvasSetStippled(bool p_stippled, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality);
extern "C" MC_DLLEXPORT void MCCanvasSetImageResizeQualityAsString(MCStringRef p_quality, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetStrokeWidth(MCCanvasRef p_canvas, MCGFloat& r_stroke_width);
extern "C" MC_DLLEXPORT void MCCanvasSetStrokeWidth(MCGFloat p_stroke_width, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetFont(MCCanvasRef p_canvas, MCCanvasFontRef &r_font);
extern "C" MC_DLLEXPORT void MCCanvasSetFont(MCCanvasFontRef p_font, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetStrokeWidth(MCCanvasRef p_canvas, MCGFloat& r_stroke_width);
extern "C" MC_DLLEXPORT void MCCanvasSetStrokeWidth(MCGFloat p_stroke_width, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetFont(MCCanvasRef p_canvas, MCCanvasFontRef &r_font);
extern "C" MC_DLLEXPORT void MCCanvasSetFont(MCCanvasFontRef p_font, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetJoinStyleAsString(MCCanvasRef p_canvas, MCStringRef &r_join_style);
extern "C" MC_DLLEXPORT void MCCanvasSetJoinStyleAsString(MCStringRef p_join_style, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetCapStyleAsString(MCCanvasRef p_canvas, MCStringRef &r_cap_style);
extern "C" MC_DLLEXPORT void MCCanvasSetCapStyleAsString(MCStringRef p_cap_style, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetMiterLimit(MCCanvasRef p_canvas, MCCanvasFloat &r_limit);
extern "C" MC_DLLEXPORT void MCCanvasSetMiterLimit(MCCanvasFloat p_limit, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetDashes(MCCanvasRef p_canvas, MCProperListRef &r_dashes);
extern "C" MC_DLLEXPORT void MCCanvasSetDashes(MCProperListRef p_dashes, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetDashPhase(MCCanvasRef p_canvas, MCCanvasFloat &r_phase);
extern "C" MC_DLLEXPORT void MCCanvasSetDashPhase(MCCanvasFloat p_phase, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasGetClipBounds(MCCanvasRef p_canvas, MCCanvasRectangleRef &r_bounds);

// Operations
extern "C" MC_DLLEXPORT void MCCanvasTransform(MCCanvasRef p_canvas, MCCanvasTransformRef p_transform);
extern "C" MC_DLLEXPORT void MCCanvasScale(MCCanvasRef p_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y);
extern "C" MC_DLLEXPORT void MCCanvasScaleWithList(MCCanvasRef p_canvas, MCProperListRef p_scale);
extern "C" MC_DLLEXPORT void MCCanvasRotate(MCCanvasRef p_canvas, MCCanvasFloat p_angle);
extern "C" MC_DLLEXPORT void MCCanvasTranslate(MCCanvasRef p_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y);
extern "C" MC_DLLEXPORT void MCCanvasTranslateWithList(MCCanvasRef p_canvas, MCProperListRef p_translation);
extern "C" MC_DLLEXPORT void MCCanvasSaveState(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasRestoreState(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasBeginLayer(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasBeginEffectOnlyLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasEndLayer(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasFill(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasFillPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasStroke(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasStrokePath(MCCanvasPathRef p_path, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasClipToRect(MCCanvasRectangleRef p_rect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasClipToPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasAddPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasDrawImage(MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasDrawRectOfImage(MCCanvasRectangleRef p_src_rect, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasMoveTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasLineTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasClosePath(MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasFillTextAligned(MCStringRef p_text, integer_t p_align, MCCanvasRectangleRef p_rect, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT void MCCanvasFillText(MCStringRef p_text, MCCanvasPointRef p_point, MCCanvasRef p_canvas);
extern "C" MC_DLLEXPORT MCCanvasRectangleRef MCCanvasMeasureText(MCStringRef p_text, MCCanvasRef p_canvas);

////////////////////////////////////////////////////////////////////////////////

#endif//_MODULE_CANVAS_H_
