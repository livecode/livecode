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

#include "graphics.h"
#include "graphics-internal.h"

#include <SkShader.h>
#include <SkGradientShader.h>
#include <SkMallocPixelRef.h>
#include <SkData.h>

////////////////////////////////////////////////////////////////////////////////

MCGSolidColorRef kMCGBlackSolidColor = nullptr;

#ifdef _DEBUG
size_t MCGObject::s_object_count = 0;
#endif

void MCGraphicsInitialize(void)
{
	MCGPlatformInitialize();
	MCGTextMeasureCacheInitialize();
	MCGBlendModesInitialize();
    
    MCGSolidColor::Create(0.0, 0.0, 0.0, 1.0, kMCGBlackSolidColor);
}

void MCGraphicsFinalize(void)
{
    MCGRelease(kMCGBlackSolidColor);
    
	MCGPlatformFinalize();
	MCGTextMeasureCacheFinalize();
	MCGBlendModesFinalize();
    
#ifdef _DEBUG
    MCLog("Graphic object count = %d", MCGObject::s_object_count);
#endif
}

void MCGraphicsCompact(void)
{
	MCGTextMeasureCacheCompact();
}

////////////////////////////////////////////////////////////////////////////////

bool MCGSolidColor::Apply(SkPaint& r_paint)
{
    r_paint.setColor(MCGColorToSkColor(m_color));
    return true;
}

bool MCGSolidColor::Create(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCGSolidColorRef& r_solid_color)
{
    MCGSolidColorRef t_color = new (nothrow) MCGSolidColor;
    if (t_color == nullptr)
    {
        return false;
    }
    
    t_color->m_color = MCGColorMakeRGBA(p_red, p_green, p_blue, p_alpha);
    
    r_solid_color = t_color;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

MCGPattern::~MCGPattern(void)
{
    MCGImageRelease(m_image);
}

bool MCGPattern::Create(MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPatternRef& r_pattern)
{
    MCGPatternRef t_pattern = new (nothrow) MCGPattern;
    if (t_pattern == nullptr)
    {
        return false;
    }
    
    t_pattern->m_image = MCGImageRetain(p_image);
    t_pattern->m_transform = p_transform;
    t_pattern->m_filter = p_filter;
    
    r_pattern = t_pattern;
    
    return true;
}

bool MCGPattern::Apply(SkPaint& r_paint)
{
	SkMatrix t_transform;
    MCGAffineTransformToSkMatrix(m_transform, t_transform);

    sk_sp<SkShader> t_shader =
            SkShader::MakeBitmapShader(*m_image->bitmap, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode, &t_transform);
    if (t_shader == nullptr)
    {
        return false;
    }

    switch (m_filter)
    {
        case kMCGImageFilterNone:
            r_paint.setFilterQuality(SkFilterQuality::kNone_SkFilterQuality);
            break;
        case kMCGImageFilterLow:
            r_paint.setFilterQuality(SkFilterQuality::kLow_SkFilterQuality);
            break;
        case kMCGImageFilterMedium:
            r_paint.setFilterQuality(SkFilterQuality::kMedium_SkFilterQuality);
            break;
        case kMCGImageFilterHigh:
            r_paint.setFilterQuality(SkFilterQuality::kHigh_SkFilterQuality);
            break;
    }

    r_paint.setShader(t_shader);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCGRamp::~MCGRamp(void)
{
    MCMemoryDeleteArray(m_stops);
    MCMemoryDeleteArray(m_colors);
}

bool MCGRamp::Create(const MCGFloat *p_stops, const MCGColor *p_colors, size_t p_ramp_length, MCGRampRef& r_ramp)
{
    MCGRampRef t_ramp = new (nothrow) MCGRamp;
    if (t_ramp == nullptr)
    {
        return false;
    }
    
    if (MCMemoryNewArray(p_ramp_length, t_ramp->m_stops) &&
        MCMemoryNewArray(p_ramp_length, t_ramp->m_colors))
    {
        for(size_t i = 0; i < p_ramp_length; i++)
        {
            t_ramp->m_stops[i] = p_stops[i];
            t_ramp->m_colors[i] = MCGColorToSkColor(p_colors[i]);
        }
        t_ramp->m_ramp_length = p_ramp_length;
    }
    else
    {
        MCGRelease(t_ramp);
        return false;
    }
    
    r_ramp = t_ramp;
    
    return true;
}

bool MCGRamp::Create4f(const MCGFloat *p_stops, const MCGColor4f *p_colors, size_t p_ramp_length, MCGRampRef& r_ramp)
{
    MCGRampRef t_ramp = new (nothrow) MCGRamp;
    if (t_ramp == nullptr)
    {
        return false;
    }
    
    if (MCMemoryNewArray(p_ramp_length, t_ramp->m_stops) &&
        MCMemoryNewArray(p_ramp_length, t_ramp->m_colors))
    {
        for(size_t i = 0; i < p_ramp_length; i++)
        {
            t_ramp->m_stops[i] = p_stops[i];
            t_ramp->m_colors[i] = MCGColorMakeRGBA(p_colors[i].red, p_colors[i].green, p_colors[i].blue, p_colors[i].alpha);
        }
        t_ramp->m_ramp_length = p_ramp_length;
    }
    else
    {
        MCGRelease(t_ramp);
        return false;
    }
    
    r_ramp = t_ramp;
    
    return true;
}

/**/

static SkShader::TileMode MCGGradientSpreadMethodToSkTileMode(MCGGradientSpreadMethod p_method)
{
    switch(p_method)
    {
    case kMCGGradientSpreadMethodPad:
        return SkShader::kClamp_TileMode;
    case kMCGGradientSpreadMethodRepeat:
        return SkShader::kRepeat_TileMode;
    case kMCGGradientSpreadMethodReflect:
        return SkShader::kMirror_TileMode;
    }
    
    MCUnreachableReturn(SkShader::kClamp_TileMode);
}

MCGGradient::~MCGGradient(void)
{
    MCGRelease(m_ramp);
}

/**/

bool MCGLinearGradient::Create(MCGPoint p_from, MCGPoint p_to, MCGRampRef p_ramp, MCGGradientSpreadMethod p_spread_method, MCGAffineTransform p_transform, MCGGradientRef& r_gradient)
{
    MCGLinearGradientRef t_gradient = new (nothrow) MCGLinearGradient;
    if (t_gradient == nullptr)
    {
        return false;
    }
    
    t_gradient->m_from = p_from;
    t_gradient->m_to = p_to;
    t_gradient->m_ramp = MCGRetain(p_ramp);
    t_gradient->m_spread_method = p_spread_method;
    t_gradient->m_transform = p_transform;
    
    r_gradient = t_gradient;
    
    return true;
}

bool MCGLinearGradient::Apply(SkPaint& r_paint)
{
	SkMatrix t_transform;
    MCGAffineTransformToSkMatrix(m_transform, t_transform);

    SkPoint t_points[2] = {MCGPointToSkPoint(m_from), MCGPointToSkPoint(m_to)};
    sk_sp<SkShader> t_shader =
            SkGradientShader::MakeLinear(t_points,
                                         m_ramp->GetColors(),
                                         m_ramp->GetStops(),
                                         m_ramp->GetLength(),
                                         MCGGradientSpreadMethodToSkTileMode(m_spread_method),
                                         0,
                                         &t_transform);
    if (t_shader == nullptr)
    {
        return false;
    }

    r_paint.setShader(t_shader);
    
    return true;
}

/**/

bool MCGRadialGradient::Create(MCGPoint p_focal_point, MCGFloat p_radius, MCGRampRef p_ramp, MCGGradientSpreadMethod p_spread_method, MCGAffineTransform p_transform, MCGGradientRef& r_gradient)
{
    MCGRadialGradientRef t_gradient = new (nothrow) MCGRadialGradient;
    if (t_gradient == nullptr)
    {
        return false;
    }
    
    t_gradient->m_focal_point = p_focal_point;
    t_gradient->m_radius = p_radius;
    t_gradient->m_ramp = MCGRetain(p_ramp);
    t_gradient->m_spread_method = p_spread_method;
    t_gradient->m_transform = p_transform;
    
    r_gradient = t_gradient;
    
    return true;
}

bool MCGRadialGradient::Apply(SkPaint& r_paint)
{
	SkMatrix t_transform;
    MCGAffineTransformToSkMatrix(m_transform, t_transform);

    sk_sp<SkShader> t_shader =
            SkGradientShader::MakeRadial(MCGPointToSkPoint(m_focal_point),
                                         m_radius,
                                         m_ramp->GetColors(),
                                         m_ramp->GetStops(),
                                         m_ramp->GetLength(),
                                         MCGGradientSpreadMethodToSkTileMode(m_spread_method),
                                         0,
                                         &t_transform);
    if (t_shader == nullptr)
    {
        return false;
    }

    r_paint.setShader(t_shader);
    
    return true;
}

/**/

bool MCGConicalGradient::Create(MCGPoint p_center_point, MCGFloat p_radius, MCGPoint p_focal_point, MCGFloat p_focal_radius, MCGRampRef p_ramp, MCGGradientSpreadMethod p_spread_method, MCGAffineTransform p_transform, MCGGradientRef& r_gradient)
{
    MCGConicalGradientRef t_gradient = new (nothrow) MCGConicalGradient;
    if (t_gradient == nullptr)
    {
        return false;
    }
    
    t_gradient->m_center_point = p_center_point;
    t_gradient->m_radius = p_radius;
    t_gradient->m_focal_point = p_focal_point;
    t_gradient->m_focal_radius = p_focal_radius;
    t_gradient->m_ramp = MCGRetain(p_ramp);
    t_gradient->m_spread_method = p_spread_method;
    t_gradient->m_transform = p_transform;
    
    r_gradient = t_gradient;
    
    return true;
}

bool MCGConicalGradient::Apply(SkPaint& r_paint)
{
	SkMatrix t_transform;
    MCGAffineTransformToSkMatrix(m_transform, t_transform);

    sk_sp<SkShader> t_shader =
            SkGradientShader::MakeTwoPointConical(MCGPointToSkPoint(m_center_point),
                                                  m_radius,
                                                  MCGPointToSkPoint(m_focal_point),
                                                  m_focal_radius,
                                                  m_ramp->GetColors(),
                                                  m_ramp->GetStops(),
                                                  m_ramp->GetLength(),
                                                  MCGGradientSpreadMethodToSkTileMode(m_spread_method),
                                                  0,
                                                  &t_transform);
    if (t_shader == nullptr)
    {
        return false;
    }

    r_paint.setShader(t_shader);
    
    return true;
}

/**/

bool MCGSweepGradient::Create(MCGPoint p_center, MCGRampRef p_ramp, MCGGradientSpreadMethod p_spread_method, MCGAffineTransform p_transform, MCGGradientRef& r_gradient)
{
    MCGSweepGradientRef t_gradient = new (nothrow) MCGSweepGradient;
    if (t_gradient == nullptr)
    {
        return false;
    }
    
    t_gradient->m_center = p_center;
    t_gradient->m_ramp = MCGRetain(p_ramp);
    t_gradient->m_spread_method = p_spread_method;
    t_gradient->m_transform = p_transform;
    
    r_gradient = t_gradient;
    
    return true;
}

bool MCGSweepGradient::Apply(SkPaint& r_paint)
{
	SkMatrix t_transform;
    MCGAffineTransformToSkMatrix(m_transform, t_transform);

    sk_sp<SkShader> t_shader =
            SkGradientShader::MakeSweep(m_center.x, m_center.y,
                                        m_ramp->GetColors(),
                                        m_ramp->GetStops(),
                                        m_ramp->GetLength(),
                                        0,
                                        &t_transform);
    if (t_shader == nullptr)
    {
        return false;
    }

    r_paint.setShader(t_shader);
    
    return true;
}

bool MCGGeneralizedGradient::Create(MCGGradientFunction p_function, MCGRampRef p_ramp, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGGradientRef& r_gradient)
{
    MCGGeneralizedGradientRef t_gradient = new (nothrow) MCGGeneralizedGradient;
    if (t_gradient == nullptr)
    {
        return false;
    }

    t_gradient->m_function = p_function;
    t_gradient->m_ramp = MCGRetain(p_ramp);
    t_gradient->m_mirror = p_mirror;
    t_gradient->m_wrap = p_wrap;
    t_gradient->m_repeats = p_repeats;
    t_gradient->m_filter = p_filter;
    t_gradient->m_transform = p_transform;

    r_gradient = t_gradient;
    
    return true;
}

bool MCGGeneralizedGradient::Apply(SkPaint& r_paint)
{
    return MCGGeneralizedGradientShaderApply(this, r_paint);
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
	
	MCGFloat *t_lengths = NULL;
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

bool MCGDashesToSkDashPathEffect(MCGDashesRef self, sk_sp<SkPathEffect>& r_path_effect)
{
	if (self -> count <= 0)
	{
		r_path_effect = nullptr;
		return true;
	}

    // Skia won't except odd numbers of dashes, so we must replicate in that case.
    uint32_t t_dash_count =
        (self -> count % 2) == 0 ? self -> count : self -> count * 2;

    MCAutoPointer<SkScalar[]> t_dashes = new (nothrow) SkScalar[t_dash_count];
    if (!t_dashes)
        return false;

    for (uint32_t i = 0; i < t_dash_count; i++)
        t_dashes[i] = MCGFloatToSkScalar(self -> lengths[i % self -> count]);
    
    sk_sp<SkPathEffect> t_dash_effect =
        SkDashPathEffect::Make(*t_dashes, (int)t_dash_count, MCGFloatToSkScalar(self -> phase));

    if (t_dash_effect == nullptr)
        return false;

    r_path_effect = t_dash_effect;
    return true;
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
                MCUnreachable();
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

void MCGBlendModesInitialize(void)
{
}

void MCGBlendModesFinalize(void)
{
}

SkBlendMode MCGBlendModeToSkBlendMode(MCGBlendMode p_mode)
{
	switch (p_mode)
	{
	case kMCGBlendModeClear:
		return SkBlendMode::kClear;
	case kMCGBlendModeCopy:
	case kMCGBlendModeSourceOver:
		return SkBlendMode::kSrcOver;
	case kMCGBlendModeSourceIn:
		return SkBlendMode::kSrcIn;
	case kMCGBlendModeSourceOut:
		return SkBlendMode::kSrcOut;
	case kMCGBlendModeSourceAtop:
		return SkBlendMode::kSrcATop;
	case kMCGBlendModeDestinationOver:
		return SkBlendMode::kDstOver;
	case kMCGBlendModeDestinationIn:
		return SkBlendMode::kDstIn;
	case kMCGBlendModeDestinationOut:
		return SkBlendMode::kDstOut;
	case kMCGBlendModeDestinationAtop:
		return SkBlendMode::kDstATop;
	case kMCGBlendModeXor:
		return SkBlendMode::kXor;
	case kMCGBlendModePlusDarker:
	case kMCGBlendModePlusLighter:
		return SkBlendMode::kPlus;
	case kMCGBlendModeMultiply:
		return SkBlendMode::kMultiply;
	case kMCGBlendModeScreen:
		return SkBlendMode::kScreen;
	case kMCGBlendModeOverlay:
		return SkBlendMode::kOverlay;
	case kMCGBlendModeDarken:
		return SkBlendMode::kDarken;
	case kMCGBlendModeLighten:
		return SkBlendMode::kLighten;
	case kMCGBlendModeColorDodge:
		return SkBlendMode::kColorDodge;
	case kMCGBlendModeColorBurn:
		return SkBlendMode::kColorBurn;
	case kMCGBlendModeSoftLight:
		return SkBlendMode::kSoftLight;
	case kMCGBlendModeHardLight:
		return SkBlendMode::kHardLight;
	case kMCGBlendModeDifference:
		return SkBlendMode::kDifference;
	case kMCGBlendModeExclusion:
		return SkBlendMode::kExclusion;
	case kMCGBlendModeHue:
		return SkBlendMode::kHue;
	case kMCGBlendModeSaturation:
		return SkBlendMode::kSaturation;
	case kMCGBlendModeColor:
		return SkBlendMode::kColor;
	case kMCGBlendModeLuminosity:
		return SkBlendMode::kLuminosity;

	default:
            return SkBlendMode::kSrcOver;
		MCUnreachableReturn(SkBlendMode::kSrcOver);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCGRasterToSkBitmap(const MCGRaster& p_raster, MCGPixelOwnershipType p_ownership, SkBitmap& r_bitmap)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
	{		
		SkColorType t_color_type = MCGRasterFormatToSkBitmapConfig(p_raster . format);
		SkImageInfo t_info = SkImageInfo::Make(p_raster.width, p_raster.height, t_color_type, p_raster.format == kMCGRasterFormat_xRGB ? kOpaque_SkAlphaType : kPremul_SkAlphaType);
		r_bitmap.setInfo(t_info);
		
		// for non-premultiplied bitmaps, allocate the space then set pixels one by one
		// for premultiplied bitmaps, just set the pixels in the target directly
		// if the copy pixels flag is set, allocate space and copy the pixels from the raster first, then set in target

		// IM-2014-09-16: [[ Bug 13458 ]] Don't copy if we're taking ownership. instead we'll premultiply in place below
		if (p_raster . format == kMCGRasterFormat_U_ARGB && p_ownership != kMCGPixelOwnershipTypeTake)
			p_ownership = kMCGPixelOwnershipTypeCopy;
		
		switch (p_ownership)
		{
			case kMCGPixelOwnershipTypeBorrow:
				r_bitmap.installPixels(t_info, p_raster.pixels, p_raster.stride);
				break;
				
			case kMCGPixelOwnershipTypeTake:
            {
                sk_sp<SkData> t_data;
                if (t_success)
                {
                    t_data = SkData::MakeFromMalloc(p_raster.pixels, p_raster.stride * p_raster.height);
                    t_success = t_data != nullptr;
                }
                
				SkMallocPixelRef *t_pixelref = nullptr;
                if (t_success)
                {
                    t_pixelref = SkMallocPixelRef::NewWithData(t_info, p_raster.stride, nullptr, t_data.get());
                    t_success = t_pixelref != nullptr;
                }
                if (t_success)
				{
					r_bitmap.setPixelRef(t_pixelref);
					t_pixelref -> unref();
				}
                
                if (!t_success)
                {
                    if (t_pixelref != NULL)
                        t_pixelref -> unref();
                }
                
                break;
            }
				
			case kMCGPixelOwnershipTypeCopy:
				t_success = r_bitmap.tryAllocPixels(t_info);
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

MCGRectangle MCGRectangleUnion(const MCGRectangle &p_rect_1, const MCGRectangle &p_rect_2)
{
	if (MCGRectangleIsEmpty(p_rect_1))
		return p_rect_2;
	else if (MCGRectangleIsEmpty(p_rect_2))
		return p_rect_1;
	
	MCGRectangle t_union;
	t_union . origin . x = MCMin(p_rect_1 . origin . x, p_rect_2 . origin . x);
	t_union . origin . y = MCMin(p_rect_1 . origin . y, p_rect_2 . origin . y);
	
	MCGFloat t_right, t_bottom;
	t_right = MCMax(p_rect_1 . origin . x + p_rect_1 . size . width, p_rect_2 . origin . x + p_rect_2 . size . width);
	t_bottom = MCMax(p_rect_1 . origin . y + p_rect_1 . size . height, p_rect_2 . origin . y + p_rect_2 . size . height);
	
	t_union . size . width = t_right - t_union . origin . x;
	t_union . size . height = t_bottom - t_union . origin . y;
	
	return t_union;
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

MCGAffineTransform MCGAffineTransformMakeSkew(MCGFloat p_xskew, MCGFloat p_yskew)
{
	return MCGAffineTransformMake(1, p_yskew, p_xskew, 1, 0, 0);
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

MCGAffineTransform MCGAffineTransformPreRotate(const MCGAffineTransform &p_transform, MCGFloat p_angle)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeRotation(p_angle), p_transform);
}

MCGAffineTransform MCGAffineTransformPostRotate(const MCGAffineTransform &p_transform, MCGFloat p_angle)
{
	return MCGAffineTransformConcat(p_transform, MCGAffineTransformMakeRotation(p_angle));
}

MCGAffineTransform MCGAffineTransformPreTranslate(const MCGAffineTransform &p_transform, MCGFloat p_xoffset, MCGFloat p_yoffset)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeTranslation(p_xoffset, p_yoffset), p_transform);
}

MCGAffineTransform MCGAffineTransformPostTranslate(const MCGAffineTransform &p_transform, MCGFloat p_xoffset, MCGFloat p_yoffset)
{
	return MCGAffineTransformConcat(p_transform, MCGAffineTransformMakeTranslation(p_xoffset, p_yoffset));
}

MCGAffineTransform MCGAffineTransformPreScale(const MCGAffineTransform &p_transform, MCGFloat p_xscale, MCGFloat p_yscale)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeScale(p_xscale, p_yscale), p_transform);
}

MCGAffineTransform MCGAffineTransformPostScale(const MCGAffineTransform &p_transform, MCGFloat p_xscale, MCGFloat p_yscale)
{
	return MCGAffineTransformConcat(p_transform, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

MCGAffineTransform MCGAffineTransformPreSkew(const MCGAffineTransform &p_transform, MCGFloat p_xskew, MCGFloat p_yskew)
{
	return MCGAffineTransformConcat(MCGAffineTransformMakeSkew(p_xskew, p_yskew), p_transform);
}

MCGAffineTransform MCGAffineTransformPostSkew(const MCGAffineTransform &p_transform, MCGFloat p_xskew, MCGFloat p_yskew)
{
	return MCGAffineTransformConcat(p_transform, MCGAffineTransformMakeSkew(p_xskew, p_yskew));
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

// solve simultaneous eqations in the form ax + by + k = 0. Equation input values are {x, y, k}. returns false if no unique solution found.
bool solve_simul_eq_2_vars(const MCGFloat p_eq_1[3], const MCGFloat p_eq_2[3], MCGFloat &r_a, MCGFloat &r_b)
{
	MCGFloat a, b, c;
	b = p_eq_1[1] * p_eq_2[0] - p_eq_2[1] * p_eq_1[0];
	c = p_eq_1[2] * p_eq_2[0] - p_eq_2[2] * p_eq_1[0];
	
	if (b == 0)
		return false;
	
	b = -c / b;
	
	MCGFloat a1, a2;
	if (p_eq_1[0] != 0)
	{
		a1 = -(b * p_eq_1[1] + p_eq_1[2]) / p_eq_1[0];
		if (a1 * p_eq_2[0] + b * p_eq_2[1] + p_eq_2[2] != 0)
			return false;
		a = a1;
	}
	else if (p_eq_2[0] != 0)
	{
		a2 = -(b * p_eq_2[1] + p_eq_2[2]) / p_eq_2[0];
		if (a2 * p_eq_1[0] + b * p_eq_2[1] + p_eq_2[2] != 0)
			return false;
        
        a = a2;
	}
	else
		return false;
	
	r_a = a;
	r_b = b;
	
	return true;
}

bool MCGAffineTransformFromPoints(const MCGPoint p_src[3], const MCGPoint p_dst[3], MCGAffineTransform &r_transform)
{
	MCGFloat a, b, c, d, tx, ty;
	
	MCGFloat t_eq1[3], t_eq2[3];
	t_eq1[0] = p_src[0].x - p_src[1].x;
	t_eq1[1] = p_src[0].y - p_src[1].y;
	t_eq1[2] = -(p_dst[0].x - p_dst[1].x);
	
	t_eq2[0] = p_src[0].x - p_src[2].x;
	t_eq2[1] = p_src[0].y - p_src[2].y;
	t_eq2[2] = -(p_dst[0].x - p_dst[2].x);
	
	if (!solve_simul_eq_2_vars(t_eq1, t_eq2, a, c))
		return false;
	
	t_eq1[2] = -(p_dst[0].y - p_dst[1].y);
	t_eq2[2] = -(p_dst[0].y - p_dst[2].y);
	
	if (!solve_simul_eq_2_vars(t_eq1, t_eq2, b, d))
		return false;
	
	tx = p_dst[0].x - a * p_src[0].x - c * p_src[0].y;
	ty = p_dst[0].y - b * p_src[0].x - d * p_src[0].y;
	
	if (tx != p_dst[1].x - a * p_src[1].x - c * p_src[1].y ||
		tx != p_dst[2].x - a * p_src[2].x - c * p_src[2].y ||
		ty != p_dst[1].y - b * p_src[1].x - d * p_src[1].y ||
		ty != p_dst[2].y - b * p_src[2].x - d * p_src[2].y)
		return false;
	
	r_transform = MCGAffineTransformMake(a, b, c, d, tx, ty);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPaintCreateWithNone(MCGPaintRef& r_paint)
{
    r_paint = nullptr;
    return true;
}

bool MCGPaintCreateWithSolidColor(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCGPaintRef& r_paint)
{
    return MCGSolidColor::Create(p_red, p_green, p_blue, p_alpha, (MCGSolidColorRef&)r_paint);
}

bool MCGPaintCreateWithPattern(MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPaintRef& r_paint)
{
    return MCGPattern::Create(p_image, p_transform, p_filter, (MCGPatternRef&)r_paint);
}

bool MCGPaintCreateWithGradient(MCGGradientFunction p_function, const MCGFloat* p_stops, const MCGColor* p_colors, uindex_t p_ramp_length, bool p_mirror, bool p_wrap, uint32_t p_repeats, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPaintRef& r_paint)
{
    bool t_success = true;
    MCGRampRef t_ramp = nullptr;
    MCGGradientRef t_gradient = nullptr;
    
    /* If the gradient is Skia-supported and the image filter is none, then use
     * Skia shaders, else use our generalized gradient shader. */
    if (p_function < kMCGLegacyGradientDiamond && p_filter == kMCGImageFilterNone)
    {
        MCGGradientSpreadMethod t_spread = kMCGGradientSpreadMethodPad;
        MCGFloat t_gradient_scale = 1.0;
        size_t t_repeats = MCMax(1, p_repeats);
        if (p_wrap)
        {
            t_spread = kMCGGradientSpreadMethodRepeat;
        }
        if (t_repeats == 1 && !p_mirror)
        {
            if (!MCGRamp::Create(p_stops, p_colors, p_ramp_length, t_ramp))
            {
                return false;
            }
        }
        else
        {
            if (p_wrap && p_mirror && p_function != kMCGGradientFunctionSweep)
            {
                t_gradient_scale = 2.0 / t_repeats;
                t_repeats = 2;
            }
            
            size_t t_length = p_ramp_length * t_repeats;
            MCGFloat t_scale = MCGFloat(1) / t_repeats;
            MCAutoArray<MCGColor> t_colors;
            MCAutoArray<MCGFloat> t_stops;
            if (!t_colors.Extend(t_length) ||
                !t_stops.Extend(t_length))
            {
                return false;
            }
            
            for(size_t i = 0; i < t_repeats; i++)
            {
                // Offset added to stops
                MCGFloat t_offset = t_scale * i;
                
                // If mirrored, odd-numbered repeats need to be handled specially
                if (p_mirror && (i % 2))
                {
                    // Copy the colours and stops in reverse order
                    for (uindex_t j = 0; j < p_ramp_length; j++)
                    {
                        t_colors[i * p_ramp_length + j] = p_colors[p_ramp_length - j - 1];
                        t_stops[i * p_ramp_length + j] = t_offset + t_scale * (1 - p_stops[p_ramp_length - j - 1]);
                    }
                }
                else
                {
                    // Copy the colours and stops in original order
                    for (uindex_t j = 0; j < p_ramp_length; j++)
                    {
                        t_colors[i * p_ramp_length + j] = p_colors[j];
                        t_stops[i * p_ramp_length + j] = t_offset + t_scale * p_stops[j];
                    }
                }
            }
            
            if (!MCGRamp::Create(t_stops.Ptr(), t_colors.Ptr(), t_length, t_ramp))
            {
                return false;
            }
        }
        
        switch(p_function)
        {
            case kMCGLegacyGradientXY:
            case kMCGLegacyGradientSpiral:
            case kMCGLegacyGradientSqrtXY:
            case kMCGLegacyGradientDiamond:
                t_success = false;
                break;
            case kMCGGradientFunctionLinear:
                t_success = MCGLinearGradient::Create({0, 0}, {t_gradient_scale, 0}, t_ramp, t_spread, p_transform, t_gradient);
                break;
            case kMCGGradientFunctionRadial:
                t_success = MCGRadialGradient::Create({0, 0}, t_gradient_scale, t_ramp, t_spread, p_transform, t_gradient);
                break;
            case kMCGGradientFunctionSweep:
                t_success = MCGSweepGradient::Create({0, 0}, t_ramp, t_spread, p_transform, t_gradient);
                break;
        }
    }
    else
    {
        if (!MCGRamp::Create(p_stops, p_colors, p_ramp_length, t_ramp))
        {
            return false;
        }
        
        t_success = MCGGeneralizedGradient::Create(p_function, t_ramp, p_mirror, p_wrap, p_repeats, p_transform, p_filter, t_gradient);
    }
    
    if (t_success)
    {
        r_paint = t_gradient;
    }
    
    MCGRelease(t_ramp);
    
    return t_success;
}

MCGPaintRef MCGPaintRetain(MCGPaintRef paint)
{
    MCGRetain(paint);
    return paint;
}

void MCGPaintRelease(MCGPaintRef paint)
{
    MCGRelease(paint);
}
