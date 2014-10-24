/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "graphics.h"
#include "graphics-internal.h"

#include <SkShader.h>
#include <SkGradientShader.h>
#include <SkMallocPixelRef.h>
#include <SkData.h>

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsInitialize(void)
{
	MCGPlatformInitialize();
	MCGTextMeasureCacheInitialize();
	MCGBlendModesInitialize();
}

void MCGraphicsFinalize(void)
{
	MCGPlatformFinalize();
	MCGTextMeasureCacheFinalize();
	MCGBlendModesFinalize();
}

void MCGraphicsCompact(void)
{
	MCGTextMeasureCacheCompact();
}

////////////////////////////////////////////////////////////////////////////////


static void MCGPatternDestroy(MCGPatternRef self)
{
	if (self != NULL)
	{
		MCGImageRelease(self -> pattern);
		MCMemoryDelete(self);
	}
}

bool MCGPatternCreate(MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPatternRef& r_pattern)
{
	bool t_success;
	t_success = true;
	
	__MCGPattern *t_pattern;
	t_pattern = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_pattern);
	
	if (t_success)
	{
		t_pattern -> pattern = MCGImageRetain(p_image);
		t_pattern -> transform = p_transform;
		t_pattern -> filter = p_filter;
		t_pattern -> references = 1;
		r_pattern = t_pattern;
	}
	else
		MCMemoryDelete(t_pattern);
		
	return t_success;
}

MCGPatternRef MCGPatternRetain(MCGPatternRef self)
{
	if (self != NULL)
		self -> references++;
	return self;	
}

void MCGPatternRelease(MCGPatternRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGPatternDestroy(self);
	}
}

bool MCGPatternToSkShader(MCGPatternRef self, SkShader*& r_shader)
{
	bool t_success;
	t_success = true;
	
	SkShader *t_pattern_shader;
	t_pattern_shader = NULL;
	if (t_success)
	{
		t_pattern_shader = SkShader::CreateBitmapShader(*self -> pattern -> bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
		t_success = t_pattern_shader != NULL;
	}
	
	if (t_success)
	{
		SkMatrix t_transform;
		MCGAffineTransformToSkMatrix(self -> transform, t_transform);
		t_pattern_shader -> setLocalMatrix(t_transform);
		r_shader = t_pattern_shader;
	}
	else if (t_pattern_shader != NULL)
		t_pattern_shader -> unref();
	
	return t_success;	
}

////////////////////////////////////////////////////////////////////////////////

static void MCGGradientDestroy(MCGGradientRef self)
{
	if (self != NULL)
	{
		MCMemoryDeleteArray(self -> colors);
		MCMemoryDeleteArray(self -> stops);
		MCMemoryDelete(self);
	}
}

bool MCGGradientCreate(MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGGradientRef& r_gradient)
{
	bool t_success;
	t_success = true;
	
	__MCGGradient *t_gradient;
	t_gradient = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_gradient);	
	
	if (t_success)
		t_success = MCMemoryNewArray(p_ramp_length, t_gradient -> colors);
	
	if (t_success)
		t_success = MCMemoryNewArray(p_ramp_length, t_gradient -> stops);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < p_ramp_length; i++)
		{
			t_gradient -> stops[i] = p_stops[i];
			t_gradient -> colors[i] = p_colors[i];
		}
		
		t_gradient -> function = p_function;
		t_gradient -> ramp_length = p_ramp_length;
		t_gradient -> mirror = p_mirror;
		t_gradient -> wrap = p_wrap;
		t_gradient -> repeats = p_repeats;
		t_gradient -> transform = p_transform;
		t_gradient -> filter = p_filter;
		t_gradient -> references = 1;
		r_gradient = t_gradient;
	}
	else
		MCGGradientDestroy(t_gradient);
	
	return t_success;
}

MCGGradientRef MCGGradientRetain(MCGGradientRef self)
{
	if (self != NULL)
		self -> references++;
	return self;
}

void MCGGradientRelease(MCGGradientRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGGradientDestroy(self);
	}
}

bool MCGGradientToSkShader(MCGGradientRef self, MCGRectangle p_clip, SkShader*& r_shader)
{
	bool t_success;
	t_success = true;	
	
	// MM-2013-11-19: [[ Bug 11471 ]] Move to legacy gradient code for all gradient kinds - there was a loss of quality using Skia.
	SkShader *t_gradient_shader;
	t_gradient_shader = new MCGLegacyGradientShader(self, p_clip);
	t_success = t_gradient_shader != NULL;
	
	if (t_success)
		r_shader = t_gradient_shader;
	else if (t_gradient_shader != NULL)
		t_gradient_shader -> unref();		

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static void MCGDashesDestroy(MCGDashesRef self)
{
	if (self != NULL)
	{
		MCMemoryDeleteArray(self -> lengths);
		MCMemoryDelete(self);
	}
}

bool MCGDashesCreate(MCGFloat p_phase, const MCGFloat *p_lengths, uindex_t p_arity, MCGDashesRef& r_dashes)
{
	bool t_success;
	t_success = true;
	
	__MCGDashes *t_dashes;
	t_dashes = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_dashes);	
	
	MCGFloat *t_lengths;
	if (t_success)
		t_success = MCMemoryNewArray(p_arity, t_lengths);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < p_arity; i++)
			t_lengths[i] = p_lengths[i];
		
		t_dashes -> lengths = t_lengths;
		t_dashes -> count = p_arity;
		t_dashes -> phase = p_phase;
		t_dashes -> references = 1;
	}
	
	if (t_success)
		r_dashes = t_dashes;
	else
	{
		MCMemoryDeleteArray(t_lengths);
		MCMemoryDelete(t_dashes);
	}
		
	return t_success;
}

MCGDashesRef MCGDashesRetain(MCGDashesRef self)
{
	if (self != NULL)
		self -> references++;
	return self;	
}

void MCGDashesRelease(MCGDashesRef self)
{
	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCGDashesDestroy(self);
	}	
}

bool MCGDashesToSkDashPathEffect(MCGDashesRef self, SkDashPathEffect*& r_path_effect)
{
	if (self -> count <= 0)
	{
		r_path_effect = NULL;
		return true;
	}
	
	bool t_success;
	t_success = true;
	
	SkScalar *t_dashes;
	if (t_success)
		t_success = MCMemoryNewArray(self -> count, t_dashes);
	
	SkDashPathEffect *t_dash_effect;
	t_dash_effect = NULL;
	if (t_success)
	{
		for (uint32_t i = 0; i < self -> count; i++)
			t_dashes[i] = MCGFloatToSkScalar(self -> lengths[i]);	
	
		t_dash_effect = new SkDashPathEffect(t_dashes, self -> count, MCGFloatToSkScalar(self -> phase));
		t_success = t_dash_effect != NULL;
	}
	
	if (t_success)
		r_path_effect = t_dash_effect;
	
	MCMemoryDeleteArray(t_dashes);
	
	return t_success;	
}

////////////////////////////////////////////////////////////////////////////////

bool MCGMaskCreateWithInfoAndRelease(const MCGDeviceMaskInfo& p_info, MCGMaskRef& r_mask)
{
	bool t_success;
	t_success = true;
	
	__MCGMask *t_mask;
	t_mask = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_mask);	
	
	if (t_success)
	{
		switch (p_info . format)
		{
			case kMCGMaskFormat_A1:
				t_mask -> mask . fFormat = SkMask::kBW_Format;
				t_mask -> mask . fRowBytes = p_info . width;
				break;
				
			case kMCGMaskFormat_A8:
				t_mask -> mask . fFormat = SkMask::kA8_Format;
				t_mask -> mask . fRowBytes = p_info . width;
				break;
				
			case kMCGMaskFormat_LCD32:
				t_mask -> mask . fFormat = SkMask::kLCD32_Format;
				t_mask -> mask . fRowBytes = p_info . width * 4;
				break;				
		}		
		
		t_mask -> mask . fBounds . setXYWH(p_info . x, p_info . y, p_info . width, p_info . height);
		t_mask -> mask . fImage = (uint8_t *)p_info . data;
		
		r_mask = t_mask;
	}
	
	return t_success;
}

void MCGMaskRelease(MCGMaskRef self)
{
	if (self != NULL && self -> mask . fImage != NULL)
		SkMask::FreeImage(self -> mask . fImage);
	MCMemoryDelete(self);
}

MCGRectangle MCGMaskGetBounds(MCGMaskRef self)
{
	if (self == NULL)
		return MCGRectangleMake(0.0f, 0.0f, 0.0f, 0.0f);
	
	MCGRectangle t_bounds;
	t_bounds . origin . x = (MCGFloat)self -> mask . fBounds . x();
	t_bounds . origin . y = (MCGFloat)self -> mask . fBounds . y();
	t_bounds . size . width = (MCGFloat)self -> mask . fBounds . width();
	t_bounds . size . height = (MCGFloat)self -> mask . fBounds . height();
	
	return t_bounds;
}

////////////////////////////////////////////////////////////////////////////////

static SkXfermode** s_skia_blend_modes = NULL;

void MCGBlendModesInitialize(void)
{
	/* UNCHECKED */ MCMemoryNewArray(kMCGBlendModeCount, s_skia_blend_modes);	
	
	s_skia_blend_modes[kMCGBlendModeClear] = SkXfermode::Create(SkXfermode::kClear_Mode);
	s_skia_blend_modes[kMCGBlendModeCopy] = SkXfermode::Create(SkXfermode::kSrcOver_Mode);
	s_skia_blend_modes[kMCGBlendModeSourceOver] = SkXfermode::Create(SkXfermode::kSrcOver_Mode);
	s_skia_blend_modes[kMCGBlendModeSourceIn] = SkXfermode::Create(SkXfermode::kSrcIn_Mode);
	s_skia_blend_modes[kMCGBlendModeSourceOut] = SkXfermode::Create(SkXfermode::kSrcOut_Mode);
	s_skia_blend_modes[kMCGBlendModeSourceAtop] = SkXfermode::Create(SkXfermode::kSrcATop_Mode);
	s_skia_blend_modes[kMCGBlendModeDestinationOver] = SkXfermode::Create(SkXfermode::kDstOver_Mode);
	s_skia_blend_modes[kMCGBlendModeDestinationIn] = SkXfermode::Create(SkXfermode::kDstIn_Mode);
	s_skia_blend_modes[kMCGBlendModeDestinationOut] = SkXfermode::Create(SkXfermode::kDstOut_Mode);
	s_skia_blend_modes[kMCGBlendModeDestinationAtop] = SkXfermode::Create(SkXfermode::kDstATop_Mode);
	s_skia_blend_modes[kMCGBlendModeXor] = SkXfermode::Create(SkXfermode::kXor_Mode);		
	
	// the non porter duff blend modes don't appear to be officially documented
	s_skia_blend_modes[kMCGBlendModePlusLighter] = SkXfermode::Create(SkXfermode::kPlus_Mode);
	s_skia_blend_modes[kMCGBlendModeScreen] = SkXfermode::Create(SkXfermode::kScreen_Mode);
	s_skia_blend_modes[kMCGBlendModeOverlay] = SkXfermode::Create(SkXfermode::kOverlay_Mode);
	s_skia_blend_modes[kMCGBlendModeDarken] = SkXfermode::Create(SkXfermode::kDarken_Mode);
	s_skia_blend_modes[kMCGBlendModeLighten] = SkXfermode::Create(SkXfermode::kLighten_Mode);		
	s_skia_blend_modes[kMCGBlendModeColorDodge] = SkXfermode::Create(SkXfermode::kColorDodge_Mode);
	s_skia_blend_modes[kMCGBlendModeColorBurn] = SkXfermode::Create(SkXfermode::kColorBurn_Mode);
	s_skia_blend_modes[kMCGBlendModeHardLight] = SkXfermode::Create(SkXfermode::kHardLight_Mode);
	s_skia_blend_modes[kMCGBlendModeSoftLight] = SkXfermode::Create(SkXfermode::kSoftLight_Mode);
	s_skia_blend_modes[kMCGBlendModeDifference] = SkXfermode::Create(SkXfermode::kDifference_Mode);
	s_skia_blend_modes[kMCGBlendModeExclusion] = SkXfermode::Create(SkXfermode::kExclusion_Mode);
	
	// legacy blend modes
	s_skia_blend_modes[kMCGBlendModeLegacyClear] = new MCGLegacyBlendMode(kMCGBlendModeLegacyClear);
	s_skia_blend_modes[kMCGBlendModeLegacyAnd] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAnd);
	s_skia_blend_modes[kMCGBlendModeLegacyAndReverse] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAndReverse);
	s_skia_blend_modes[kMCGBlendModeLegacyCopy] = new MCGLegacyBlendMode(kMCGBlendModeLegacyCopy);
	s_skia_blend_modes[kMCGBlendModeLegacyInverted] = new MCGLegacyBlendMode(kMCGBlendModeLegacyInverted);
	s_skia_blend_modes[kMCGBlendModeLegacyNoop] = new MCGLegacyBlendMode(kMCGBlendModeLegacyNoop);
	s_skia_blend_modes[kMCGBlendModeLegacyXor] = new MCGLegacyBlendMode(kMCGBlendModeLegacyXor);
	s_skia_blend_modes[kMCGBlendModeLegacyOr] = new MCGLegacyBlendMode(kMCGBlendModeLegacyOr);
	s_skia_blend_modes[kMCGBlendModeLegacyNor] = new MCGLegacyBlendMode(kMCGBlendModeLegacyNor);
	s_skia_blend_modes[kMCGBlendModeLegacyEquiv] = new MCGLegacyBlendMode(kMCGBlendModeLegacyEquiv);
	s_skia_blend_modes[kMCGBlendModeLegacyInvert] = new MCGLegacyBlendMode(kMCGBlendModeLegacyInvert);
	s_skia_blend_modes[kMCGBlendModeLegacyOrReverse] = new MCGLegacyBlendMode(kMCGBlendModeLegacyOrReverse);
	s_skia_blend_modes[kMCGBlendModeLegacyCopyInverted] = new MCGLegacyBlendMode(kMCGBlendModeLegacyCopyInverted);
	s_skia_blend_modes[kMCGBlendModeLegacyOrInverted] = new MCGLegacyBlendMode(kMCGBlendModeLegacyOrInverted);
	s_skia_blend_modes[kMCGBlendModeLegacyNand] = new MCGLegacyBlendMode(kMCGBlendModeLegacyNand);
	s_skia_blend_modes[kMCGBlendModeLegacySet] = new MCGLegacyBlendMode(kMCGBlendModeLegacySet);
	s_skia_blend_modes[kMCGBlendModeLegacyBlend] = new MCGLegacyBlendMode(kMCGBlendModeLegacyBlend);
	s_skia_blend_modes[kMCGBlendModeLegacyAddPin] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAddPin);
	s_skia_blend_modes[kMCGBlendModeLegacyAddOver] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAddOver);
	s_skia_blend_modes[kMCGBlendModeLegacySubPin] = new MCGLegacyBlendMode(kMCGBlendModeLegacySubPin);
	s_skia_blend_modes[kMCGBlendModeLegacyTransparent] = new MCGLegacyBlendMode(kMCGBlendModeLegacyTransparent);
	s_skia_blend_modes[kMCGBlendModeLegacyAdMax] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAdMax);
	s_skia_blend_modes[kMCGBlendModeLegacySubOver] = new MCGLegacyBlendMode(kMCGBlendModeLegacySubOver);
	s_skia_blend_modes[kMCGBlendModeLegacyAdMin] = new MCGLegacyBlendMode(kMCGBlendModeLegacyAdMin);		
	s_skia_blend_modes[kMCGBlendModeLegacyBlendSource] = new MCGLegacyBlendMode(kMCGBlendModeLegacyBlendSource);
	s_skia_blend_modes[kMCGBlendModeLegacyBlendDestination] = new MCGLegacyBlendMode(kMCGBlendModeLegacyBlendDestination);
	
	// MM-2013-11-11: [[ Bug 11422 ]] Use legacy multiply. Skia's multiply appears to blend differently.
	s_skia_blend_modes[kMCGBlendModeMultiply] = new MCGLegacyBlendMode(kMCGBlendModeMultiply);
	
	// TODO: non-separable blend modes and plus darker?
	// kMCGBlendModeHue
	// kMCGBlendModeSaturation
	// kMCGBlendModeColor
	// kMCGBlendModeLuminosity	
}

void MCGBlendModesFinalize(void)
{
	if (s_skia_blend_modes != NULL)
		for (uint32_t i = 0; i < kMCGBlendModeCount; i++)
			if (s_skia_blend_modes[i] != NULL)
				s_skia_blend_modes[i] -> unref();
	MCMemoryDeleteArray(s_skia_blend_modes);
}

SkXfermode* MCGBlendModeToSkXfermode(MCGBlendMode p_mode)
{
	SkXfermode *t_blend_mode;
	t_blend_mode = NULL;
	if (s_skia_blend_modes != NULL)
	{
		if (p_mode < kMCGBlendModeCount)
			t_blend_mode = s_skia_blend_modes[p_mode];
		if (t_blend_mode != NULL) 
			t_blend_mode -> ref();		
	}
	return t_blend_mode;
}

MCGBlendMode MCGBlendModeFromSkXfermode(SkXfermode *p_mode)
{
	// TODO: Implement
	return kMCGBlendModeSourceOver;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGRasterToSkBitmap(const MCGRaster& p_raster, MCGPixelOwnershipType p_ownership, SkBitmap& r_bitmap)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
	{		
		SkBitmap::Config t_config = MCGRasterFormatToSkBitmapConfig(p_raster . format);
		r_bitmap . setConfig(t_config, p_raster . width, p_raster . height, p_raster . stride);
        if (p_raster . format == kMCGRasterFormat_xRGB)
            r_bitmap . setAlphaType(kOpaque_SkAlphaType);
        else
            r_bitmap . setAlphaType(kPremul_SkAlphaType);
		
		// for non-premultiplied bitmaps, allocate the space then set pixels one by one
		// for premultiplied bitmaps, just set the pixels in the target directly
		// if the copy pixels flag is set, allocate space and copy the pixels from the raster first, then set in target

		// IM-2014-09-16: [[ Bug 13458 ]] Don't copy if we're taking ownership. instead we'll premultiply in place below
		if (p_raster . format == kMCGRasterFormat_U_ARGB && p_ownership != kMCGPixelOwnershipTypeTake)
			p_ownership = kMCGPixelOwnershipTypeCopy;
		
		switch (p_ownership)
		{
			case kMCGPixelOwnershipTypeBorrow:
				r_bitmap . setPixels(p_raster . pixels);
				break;
				
			case kMCGPixelOwnershipTypeTake:
            {
                SkImageInfo t_image_info;
                if (t_success)
                    t_success = r_bitmap  . asImageInfo(&t_image_info);
                
                SkData *t_data;
                if (t_success)
                {
                    t_data = SkData::NewFromMalloc(p_raster . pixels, p_raster . stride * p_raster . height);
                    t_success = t_data != NULL;
                }
                
				SkMallocPixelRef *t_pixelref;
                if (t_success)
                {
                    t_pixelref = SkMallocPixelRef::NewWithData(t_image_info, p_raster . stride, NULL, t_data, 0);
                    t_success = t_pixelref != NULL;
                }
                if (t_success)
				{
					r_bitmap . setPixelRef(t_pixelref);
					t_pixelref -> unref();
					// MM-2014-03-07: [[ Bug 11885 ]] Make sure we free up the data as well as pixel ref.
                    t_data -> unref();
				}
                
                if (!t_success)
                {
                    if (t_pixelref != NULL)
                        t_pixelref -> unref();
                    if (t_data != NULL)
                        t_data -> unref();
                }
                
                break;
            }
				
			case kMCGPixelOwnershipTypeCopy:
				t_success = r_bitmap . allocPixels();
				break;
		}
	}
	
	if (t_success)
	{
		if (p_raster . format == kMCGRasterFormat_U_ARGB)
		{
			r_bitmap.lockPixels();
			
			// for non-premultiplied bitmaps, loop through the source bitmap pixel by pixel
			// premultiplying each pixel before writing to destination bitmap
			uint8_t *t_dst_ptr;
			t_dst_ptr = (uint8_t*)r_bitmap.getPixels();
			uint32_t t_dst_stride;
			t_dst_stride = r_bitmap.rowBytes();
			
			uint8_t *t_src_ptr;
			uint32_t t_src_stride;
			// IM-2014-09-16: [[ Bug 13458 ]] If we're taking ownership then the bitmap pixels are
			//     both source and destination
			if (p_ownership == kMCGPixelOwnershipTypeTake)
			{
				t_src_ptr = t_dst_ptr;
				t_src_stride = t_dst_stride;
			}
			else
			{
				t_src_ptr = (uint8_t*)p_raster.pixels;
				t_src_stride = p_raster.stride;
			}
			
			for (uint32_t y = 0; y < p_raster . height; y++)
			{
				uint32_t *t_src_pixel;
				t_src_pixel = (uint32_t*)t_src_ptr;
				uint32_t *t_dst_pixel;
				t_dst_pixel = (uint32_t*)t_dst_ptr;
				
				// IM-2014-07-23: [[ Bug 12892 ]] Use MCGPixel function to premultiply native format pixels
				for (uint32_t x = 0; x < p_raster . width; x++)
					*t_dst_pixel++ = MCGPixelPreMultiplyNative(*t_src_pixel++);
                
				t_dst_ptr += t_dst_stride;
				t_src_ptr += t_src_stride;
			}
			
			r_bitmap.unlockPixels();
		}
		else if (p_ownership == kMCGPixelOwnershipTypeCopy)
			MCMemoryCopy(r_bitmap . getPixels(), p_raster . pixels, p_raster . height * p_raster . stride);
	}
	
	return t_success;	
}

////////////////////////////////////////////////////////////////////////////////

MCGPoint MCGPointApplyAffineTransform(const MCGPoint& p_point, const MCGAffineTransform& p_transform)
{
	MCGPoint t_transformed_pt;
	t_transformed_pt . x = p_transform . a * p_point . x + p_transform . c * p_point . y + p_transform . tx;
	t_transformed_pt . y = p_transform . b * p_point . x + p_transform . d * p_point . y + p_transform . ty;
	return t_transformed_pt;
}

MCGSize MCGSizeApplyAffineTransform(const MCGSize& p_point, const MCGAffineTransform& p_transform)
{
	MCGSize t_transformed_pt;
	t_transformed_pt . width = p_transform . a * p_point . width + p_transform . c * p_point . height;
	t_transformed_pt . height = p_transform . b * p_point . width + p_transform . d * p_point . height;
	return t_transformed_pt;
}

MCGRectangle MCGRectangleApplyAffineTransform(const MCGRectangle& p_rect, const MCGAffineTransform& p_transform)
{
	MCGRectangle t_transformed_rect;
	if (p_transform . b == 0.0f && p_transform . c == 0.0f)
	{
		// we are just translating or scaling (or both) the rectangle
		// so the basic rectangle shape remains the same, we just need to translate the origin and scale the width and height
		t_transformed_rect . origin = MCGPointApplyAffineTransform(p_rect . origin, p_transform);
		t_transformed_rect . size = MCGSizeApplyAffineTransform(p_rect . size, p_transform);
	}
	else
	{
		// we are skewing the rectangle
		// so we'll need to work out where the four new corners of the skewed rectangle lie
		// and then return the rectangle which bounds the new skewed rectangle
		MCGPoint t_top_left;
		t_top_left = MCGPointApplyAffineTransform(p_rect . origin, p_transform);
		
		MCGPoint t_top_right;
		t_top_right . x = p_rect . origin . x + p_rect . size . width;
		t_top_right . y = p_rect . origin . y;
		t_top_right = MCGPointApplyAffineTransform(t_top_right, p_transform);
		
		MCGPoint t_bottom_left;
		t_bottom_left . x = p_rect . origin . x;
		t_bottom_left . y = p_rect . origin . y + p_rect . size . height;
		t_bottom_left = MCGPointApplyAffineTransform(t_bottom_left, p_transform);
		
		MCGPoint t_bottom_right;
		t_bottom_right . x = p_rect . origin . x + p_rect . size . width;
		t_bottom_right . y = p_rect . origin . y + p_rect . size . height;
		t_bottom_right = MCGPointApplyAffineTransform(t_bottom_right, p_transform);
		
		MCGFloat t_max_x;
		t_max_x = MCMax(MCMax(MCMax(t_top_left . x, t_top_right . x), t_bottom_left . x), t_bottom_right . x);
		
		MCGFloat t_min_x;
		t_min_x = MCMin(MCMin(MCMin(t_top_left . x, t_top_right . x), t_bottom_left . x), t_bottom_right . x);
		
		MCGFloat t_max_y;
		t_max_y = MCMax(MCMax(MCMax(t_top_left . y, t_top_right . y), t_bottom_left . y), t_bottom_right . y);
		
		MCGFloat t_min_y;
		t_min_y = MCMin(MCMin(MCMin(t_top_left . y, t_top_right . y), t_bottom_left . y), t_bottom_right . y);
		
		t_transformed_rect . origin . x = t_min_x;
		t_transformed_rect . origin . y = t_min_y;
		t_transformed_rect . size . width = t_max_x - t_min_x;
		t_transformed_rect . size . height = t_max_y - t_min_y;
	}
	return t_transformed_rect;
}

////////////////////////////////////////////////////////////////////////////////

MCGRectangle MCGRectangleIntersection(const MCGRectangle &p_rect_1, const MCGRectangle &p_rect_2)
{
	MCGRectangle t_intersection;
	t_intersection . origin . x = MCMax(p_rect_1 . origin . x, p_rect_2 . origin . x);
	t_intersection . origin . y = MCMax(p_rect_1 . origin . y, p_rect_2 . origin . y);
	
	MCGFloat t_right, t_bottom;
	t_right = MCMin(p_rect_1 . origin . x + p_rect_1 . size . width, p_rect_2 . origin . x + p_rect_2 . size . width);
	t_bottom = MCMin(p_rect_1 . origin . y + p_rect_1 . size . height, p_rect_2 . origin . y + p_rect_2 . size . height);
	
	t_intersection . size . width = MCMax(0.0f, t_right - t_intersection . origin . x);
	t_intersection . size . height = MCMax(0.0f, t_bottom - t_intersection . origin . y);
	
	return t_intersection;
}

MCGIntegerRectangle MCGIntegerRectangleIntersection(const MCGIntegerRectangle &p_rect_1, const MCGIntegerRectangle &p_rect_2)
{
	int32_t t_left, t_top;
	t_left = MCMax(p_rect_1.origin.x, p_rect_2.origin.x);
	t_top = MCMax(p_rect_1.origin.y, p_rect_2.origin.y);
	
	// IM-2014-10-22: [[ Bug 13746 ]] Cast to signed ints to fix unsigned arithmetic overflow
	int32_t t_right, t_bottom;
	t_right = MCMin(p_rect_1.origin.x + (int32_t)p_rect_1.size.width, p_rect_2.origin.x + (int32_t)p_rect_2.size.width);
	t_bottom = MCMin(p_rect_1.origin.y + (int32_t)p_rect_1.size.height, p_rect_2.origin.y + (int32_t)p_rect_2.size.height);
	
	t_right = MCMax(t_left, t_right);
	t_bottom = MCMax(t_top, t_bottom);
	
	return MCGIntegerRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
}

MCGIntegerRectangle MCGRectangleGetBounds(const MCGRectangle &p_rect)
{
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = floor(p_rect.origin.x);
	t_top = floor(p_rect.origin.y);
	t_right = ceil(p_rect.origin.x + p_rect.size.width);
	t_bottom = ceil(p_rect.origin.y + p_rect.size.height);
	
	int32_t t_width, t_height;
	t_width = t_right - t_left;
	t_height = t_bottom - t_top;
	
	// [[ Bug 11349 ]] Out of bounds content displayed since getting integer
	//   bounds of an empty rect is not empty.
	if (p_rect . size . width == 0.0f || p_rect . size . height == 0.0f)
	{
		t_width = 0;
		t_height = 0;
	}
	
	return MCGIntegerRectangleMake(t_left, t_top, t_width, t_height);
}

MCGIntegerRectangle MCGIntegerRectangleGetTransformedBounds(const MCGIntegerRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetBounds(MCGRectangleApplyAffineTransform(MCGIntegerRectangleToMCGRectangle(p_rect), p_transform));
}

////////////////////////////////////////////////////////////////////////////////

void MCGAffineTransformToSkMatrix(const MCGAffineTransform &p_transform, SkMatrix &r_matrix)
{
	r_matrix . setAll(MCGFloatToSkScalar(p_transform . a),
					  MCGFloatToSkScalar(p_transform . c),
					  MCGFloatToSkScalar(p_transform . tx),
					  MCGFloatToSkScalar(p_transform . b),
					  MCGFloatToSkScalar(p_transform . d),
					  MCGFloatToSkScalar(p_transform . ty),
					  SkIntToScalar(0),
					  SkIntToScalar(0),
					  SK_Scalar1);
}

void MCGAffineTransformFromSkMatrix(const SkMatrix &p_matrix, MCGAffineTransform &r_transform)
{
	r_transform . a = p_matrix[0];
	r_transform . c = p_matrix[1];
	r_transform . tx = p_matrix[2];
	r_transform . b = p_matrix[3];
	r_transform . d = p_matrix[4];
	r_transform . ty = p_matrix[5];
}

MCGAffineTransform MCGAffineTransformMakeIdentity(void)
{
	MCGAffineTransform t_transform;
	t_transform . a = 1.0f;
	t_transform . b = 0.0f;
	t_transform . c = 0.0f;
	t_transform . d = 1.0f;
	t_transform . tx = 0.0f;
	t_transform . ty = 0.0f;
	return t_transform;
}

MCGAffineTransform MCGAffineTransformMakeRotation(MCGFloat p_angle)
{
	MCGAffineTransform t_transform;
	p_angle = MCGFloatDegreesToRadians(p_angle);
	t_transform . a = MCGFloatCos(p_angle);
	t_transform . b = MCGFloatSin(p_angle);
	t_transform . c = -1.0f * t_transform . b;
	t_transform . d = t_transform . a;
	t_transform . tx = 0.0f;
	t_transform . ty = 0.0f;
	return t_transform;
}

MCGAffineTransform MCGAffineTransformMakeTranslation(MCGFloat p_xoffset, MCGFloat p_yoffset)
{
	MCGAffineTransform t_transform;
	t_transform . a = 1.0f;
	t_transform . b = 0.0f;
	t_transform . c = 0.0f;
	t_transform . d = 1.0f;
	t_transform . tx = p_xoffset;
	t_transform . ty = p_yoffset;
	return t_transform;
}

MCGAffineTransform MCGAffineTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale)
{
	MCGAffineTransform t_transform;
	t_transform . a = p_xscale;
	t_transform . b = 0.0f;
	t_transform . c = 0.0f;
	t_transform . d = p_yscale;
	t_transform . tx = 0.0f;
	t_transform . ty = 0.0f;
	return t_transform;
}

MCGAffineTransform MCGAffineTransformConcat(const MCGAffineTransform& p_transform_1, const MCGAffineTransform& p_transform_2)
{
	MCGAffineTransform t_result;
	t_result . a = p_transform_1 . a * p_transform_2 . a + p_transform_1 . c * p_transform_2 . b;
	t_result . b = p_transform_1 . b * p_transform_2 . a + p_transform_1 . d * p_transform_2 . b;
	t_result . c = p_transform_1 . a * p_transform_2 . c + p_transform_1 . c * p_transform_2 . d;
	t_result . d = p_transform_1 . b * p_transform_2 . c + p_transform_1 . d * p_transform_2 . d;
	t_result . tx = p_transform_1 . a * p_transform_2 . tx + p_transform_1 . c * p_transform_2 . ty + p_transform_1 . tx;
	t_result . ty = p_transform_1 . b * p_transform_2 . tx + p_transform_1 . d * p_transform_2 . ty + p_transform_1 . ty;
	return t_result;
}

MCGAffineTransform MCGAffineTransformRotate(const MCGAffineTransform &p_transform, MCGFloat p_angle)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeRotation(p_angle), p_transform);
}

MCGAffineTransform MCGAffineTransformTranslate(const MCGAffineTransform &p_transform, MCGFloat p_xoffset, MCGFloat p_yoffset)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeTranslation(p_xoffset, p_yoffset), p_transform);
}

MCGAffineTransform MCGAffineTransformScale(const MCGAffineTransform &p_transform, MCGFloat p_xscale, MCGFloat p_yscale)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeScale(p_xscale, p_yscale), p_transform);
}

MCGAffineTransform MCGAffineTransformInvert(const MCGAffineTransform& p_transform)
{
	MCGFloat t_det = p_transform.a * p_transform.d - p_transform.c * p_transform.b;

	MCAssert(t_det != 0.0);

	MCGAffineTransform t_result;
	//t_result.a = p_transform.d / t_det;
	//t_result.b = -p_transform.b / t_det;
	//t_result.c = -p_transform.c / t_det;
	//t_result.d = p_transform.a / t_det;
	//t_result.tx = (p_transform.c * p_transform.ty - p_transform.tx * p_transform.d) / t_det;
	//t_result.ty = (p_transform.tx * p_transform.b - p_transform.a * p_transform.ty) / t_det;

	MCGFloat t_a = p_transform.a / t_det;
	MCGFloat t_b = p_transform.b / t_det;
	MCGFloat t_c = p_transform.c / t_det;
	MCGFloat t_d = p_transform.d / t_det;

	t_result.a = t_d;
	t_result.b = -t_b;
	t_result.c = -t_c;
	t_result.d = t_a;
	t_result.tx = t_c * p_transform.ty - p_transform.tx * t_d;
	t_result.ty = p_transform.tx * t_b - t_a * p_transform.ty;
	return t_result;
}

//////////

MCGAffineTransform MCGAffineTransformFromRectangles(const MCGRectangle &p_a, const MCGRectangle &p_b)
{
	MCGFloat t_x_scale, t_y_scale;
	t_x_scale = p_b.size.width / p_a.size.width;
	t_y_scale = p_b.size.height / p_a.size.height;
	
	MCGFloat t_dx, t_dy;
	t_dx = p_b.origin.x - (p_a.origin.x * t_x_scale);
	t_dy = p_b.origin.y - (p_a.origin.y * t_y_scale);
	
	return MCGAffineTransformMake(t_x_scale, 0, 0, t_y_scale, t_dx, t_dy);
}

////////////////////////////////////////////////////////////////////////////////