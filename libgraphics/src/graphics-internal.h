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

#ifndef __MC_GRAPHICS_INTERNAL__
#define __MC_GRAPHICS_INTERNAL__

#include "graphics.h"
#include <SkCanvas.h>
#include <SkDashPathEffect.h>
#include <SkMask.h>
#include <SkShader.h>
#include <SkSurface.h>

#ifdef __MOBILE
#define kMCGTextMeasureCacheTableSize 16384
#else
#define kMCGTextMeasureCacheTableSize 65535
#endif

#define kMCGTextMeasureCacheByteSize kMCGTextMeasureCacheTableSize * 256
#define kMCGTextMeasureCacheMaxOccupancy kMCGTextMeasureCacheTableSize * 0.5
#define kMCGTextMeasureCacheMaxStringLength 65536

////////////////////////////////////////////////////////////////////////////////

typedef class MCGObject *MCGObjectRef;

class MCGObject
{
public:
#ifdef _DEBUG
    static size_t s_object_count;
#endif

    MCGObject(void)
        : m_references(1)
    {
#ifdef _DEBUG
        s_object_count += 1;
#endif
    }

protected:
    virtual ~MCGObject(void)
    {
#ifdef _DEBUG
        s_object_count -= 1;
#endif
    }

private:
    virtual void Retain(void)
    {
        m_references += 1;
    }
    
    virtual void Release(void)
    {
        m_references -= 1;
        if (m_references == 0)
        {
            delete this;
        }
    }
    
    template<typename T>
    friend T MCGRetain(T p_object)
    {
        if (p_object == nullptr)
        {
            return nullptr;
        }
        
        p_object->Retain();
        
        return p_object;
    }
    
    template<typename T>
    friend void MCGRelease(T p_object)
    {
        if (p_object == nullptr)
        {
            return;
        }
        
        p_object->Release();
    }
    
    template<typename T>
    friend void MCGAssignAndRelease(T& x_target, T p_source)
    {
        if (x_target != nullptr)
        {
            x_target->Release();
        }
        
        x_target = p_source;
    }
    
    uint32_t m_references;
};

////////////////////////////////////////////////////////////////////////////////

typedef class MCGPaint *MCGPaintRef;

class MCGPaint: public MCGObject
{
public:
    virtual bool Apply(SkPaint& r_paint) = 0;
};

////////////////////////////////////////////////////////////////////////////////

typedef class MCGSolidColor *MCGSolidColorRef;

class MCGSolidColor: public MCGPaint
{
public:
    virtual bool Apply(SkPaint& r_paint);
    
    static bool Create(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCGSolidColorRef& r_solid_color);
    
private:
    MCGColor m_color;
};

extern MCGSolidColorRef kMCGBlackSolidColor;

////////////////////////////////////////////////////////////////////////////////

typedef class MCGPattern *MCGPatternRef;

class MCGPattern: public MCGPaint
{
public:
    virtual ~MCGPattern(void);
    
    virtual bool Apply(SkPaint& r_paint);
    
    static bool Create(MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter, MCGPatternRef& r_pattern);
    
private:
	MCGImageRef			m_image;
	MCGAffineTransform	m_transform;
	MCGImageFilter		m_filter;
};

////////////////////////////////////////////////////////////////////////////////

typedef class MCGRamp *MCGRampRef;

class MCGRamp: public MCGObject
{
public:
    virtual ~MCGRamp(void);

    static bool Create(const MCGFloat *p_stops, const MCGColor *p_colors, size_t p_ramp_length, MCGRampRef& r_ramp);

    static bool Create4f(const MCGFloat *p_stops, const MCGColor4f *p_colors, size_t p_ramp_length, MCGRampRef& r_ramp);

    const SkScalar* GetStops(void) const { return m_stops; }
    const SkColor* GetColors(void) const { return m_colors; }
    size_t GetLength(void) const { return m_ramp_length; }

private:
    SkScalar *m_stops = nullptr;
    SkColor *m_colors = nullptr;
    size_t m_ramp_length = 0;
};

typedef class MCGGradient *MCGGradientRef;

class MCGGradient: public MCGPaint
{
public:
    virtual ~MCGGradient(void);

protected:
    MCGRampRef m_ramp = nullptr;
    MCGGradientSpreadMethod m_spread_method;
	MCGAffineTransform m_transform;
};

typedef class MCGGeneralizedGradient *MCGGeneralizedGradientRef;

class MCGGeneralizedGradient: public MCGGradient
{
public:
    virtual bool Apply(SkPaint& r_paint);

    static bool Create(MCGGradientFunction function, MCGRampRef ramp, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter, MCGGradientRef& r_gradient);
    
private:
	MCGGradientFunction	m_function;
	uint32_t			m_repeats;
    MCGImageFilter		m_filter;
    bool m_mirror;
    bool m_wrap;
    
    friend class MCGGeneralizedGradientShader;
};

extern bool MCGGeneralizedGradientShaderApply(MCGGeneralizedGradientRef p_gradient, SkPaint& r_paint);

typedef class MCGLinearGradient *MCGLinearGradientRef;

class MCGLinearGradient: public MCGGradient
{
public:
    virtual bool Apply(SkPaint& r_paint);
    
    static bool Create(MCGPoint p_from, MCGPoint p_to, MCGRampRef p_ramp, MCGGradientSpreadMethod p_spread_method, MCGAffineTransform p_transform, MCGGradientRef& r_gradient);
    
private:
    MCGPoint m_from;
    MCGPoint m_to;
};

typedef class MCGRadialGradient *MCGRadialGradientRef;

class MCGRadialGradient: public MCGGradient
{
public:
    virtual bool Apply(SkPaint& r_paint);

    static bool Create(MCGPoint p_focal_point, MCGFloat p_radius, MCGRampRef p_ramp, MCGGradientSpreadMethod p_tile_mode, MCGAffineTransform p_transform, MCGGradientRef& r_gradient);
    
private:
    MCGPoint m_focal_point;
    MCGFloat m_radius;
};

typedef class MCGConicalGradient *MCGConicalGradientRef;

class MCGConicalGradient: public MCGGradient
{
public:
    virtual bool Apply(SkPaint& r_paint);
    
    static bool Create(MCGPoint p_center_point, MCGFloat p_radius, MCGPoint p_focal_point, MCGFloat p_focal_radius, MCGRampRef p_ramp, MCGGradientSpreadMethod p_tile_mode, MCGAffineTransform p_transform, MCGGradientRef& r_gradient);
    
private:
    MCGPoint m_center_point;
    MCGFloat m_radius;
    MCGPoint m_focal_point;
    MCGFloat m_focal_radius;
};

typedef class MCGSweepGradient *MCGSweepGradientRef;

class MCGSweepGradient: public MCGGradient
{
public:
    virtual bool Apply(SkPaint& r_paint);

    static bool Create(MCGPoint p_center, MCGRampRef p_ramp, MCGGradientSpreadMethod p_tile_mode, MCGAffineTransform p_transform, MCGGradientRef& r_gradient);
    
private:
    MCGPoint m_center;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCGContextState *MCGContextStateRef;

struct __MCGContextState
{
    MCGAffineTransform  base_transform;
    MCGAffineTransform  transform;

	MCGFloat			opacity;
	MCGBlendMode		blend_mode;
	MCGFloat			flatness;
	bool				should_antialias;
	
    MCGPaintRef         fill_paint;
	MCGFloat			fill_opacity;
	MCGFillRule			fill_rule;
	MCGPaintStyle		fill_style;
	
    MCGPaintRef         stroke_paint;
	MCGFloat			stroke_opacity;
	MCGStrokeAttr		stroke_attr;
	MCGPaintStyle		stroke_style;
	
	bool				is_layer_begin_pt;
	MCGContextStateRef	parent;
};

typedef struct __MCGContextLayer *MCGContextLayerRef;

struct __MCGContextLayer
{
	SkCanvas *canvas;
	sk_sp<SkSurface> m_surface;
	
	uint32_t nesting;
	
	int32_t origin_x, origin_y;
	
	bool has_effects : 1;
	MCGBitmapEffects effects;
	
	MCGContextLayerRef parent;
};

////////////////////////////////////////////////////////////////////////////////

struct __MCGImage
{
	SkBitmap	*bitmap;
	bool		is_valid;
	uint32_t	references;
};

struct __MCGMask
{
	SkMask	mask;
};

struct __MCGPath
{
	SkPath		*path;
	bool		is_valid;
	bool		is_mutable;
	uint32_t	references;
};

struct __MCGContext
{
	//uint32_t			width;
	//uint32_t			height;
	MCGContextStateRef	state;
	MCGContextLayerRef  layer;
	MCGPathRef			path;
	
	bool				is_valid;
	uint32_t			references;
};

////////////////////////////////////////////////////////////////////////////////

struct __MCGDashes
{
	MCGFloat	phase;
	MCGFloat	*lengths;
	uindex_t	count;
	uint32_t	references;
};

bool MCGDashesCreate(MCGFloat phase, const MCGFloat *lengths, uindex_t arity, MCGDashesRef& r_dashes);
MCGDashesRef MCGDashesRetain(MCGDashesRef dashes);
void MCGDashesRelease(MCGDashesRef dashes);
bool MCGDashesToSkDashPathEffect(MCGDashesRef dashes, sk_sp<SkPathEffect>& r_path_effect);

////////////////////////////////////////////////////////////////////////////////

inline bool MCGRectangleIntersects(MCGRectangle p_rect_1, MCGRectangle p_rect_2)
{
	return (p_rect_1 . origin . x < (p_rect_2 . origin . x +  p_rect_2 . size . width)) && 
	(p_rect_2 . origin . x < (p_rect_1 . origin . x +  p_rect_1 . size . width)) &&
	(p_rect_1 . origin . y < (p_rect_2 . origin . y +  p_rect_2 . size . height)) &&
	(p_rect_2 . origin . y < (p_rect_1 . origin . y +  p_rect_1 . size . height));	
}

struct MCGIntRectangle
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

inline MCGIntRectangle MCGRectangleIntegerBounds(const MCGRectangle &p_rect)
{
	MCGIntRectangle t_bounds;
	t_bounds .  x = floor(p_rect . origin . x);
	t_bounds .  y = floor(p_rect . origin . y);
	// IM-2013-09-02: [[ RefactorGraphics ]] don't expand size if width or height is zero
	if (p_rect . size . width == 0.0)
		t_bounds . width = 0;
	else
		t_bounds . width = ceil(p_rect . origin . x + p_rect . size . width) - t_bounds . x;
	if (p_rect . size . height == 0.0)
		t_bounds . height = 0;
	else
		t_bounds . height = ceil(p_rect . origin . y + p_rect . size . height) - t_bounds . y;
	return t_bounds;
}

////////////////////////////////////////////////////////////////////////////////

enum MCGPixelOwnershipType
{
	kMCGPixelOwnershipTypeBorrow,
	kMCGPixelOwnershipTypeTake,
	kMCGPixelOwnershipTypeCopy,
};

bool MCGRasterToSkBitmap(const MCGRaster& raster, MCGPixelOwnershipType p_ownership, SkBitmap& bitmap);
SkBlendMode MCGBlendModeToSkBlendMode(MCGBlendMode);
void MCGAffineTransformToSkMatrix(const MCGAffineTransform& transform, SkMatrix& r_matrix);
void MCGAffineTransformFromSkMatrix(const SkMatrix &matrix, MCGAffineTransform &r_transform);

////////////////////////////////////////////////////////////////////////////////

inline SkColor MCGColorToSkColor(MCGColor p_color)
{
	return p_color;
}

inline MCGColor MCGColorFromSkColor(SkColor p_color)
{
	return p_color;
}

////////////////////////////////////////////////////////////////////////////////

inline MCGFloat MCGFloatDegreesToRadians(MCGFloat p_degrees)
{
	return p_degrees * M_PI / 180;
}

inline MCGFloat MCGFloatDegreesFromRadians(MCGFloat p_radians)
{
	return p_radians * 180 / M_PI;
}

inline MCGFloat MCGFloatCos(MCGFloat p_angle)
{
	return cosf(p_angle);
}

inline MCGFloat MCGFloatSin(MCGFloat p_angle)
{
	return sinf(p_angle);
}

////////////////////////////////////////////////////////////////////////////////

inline SkScalar MCGFloatToSkScalar(MCGFloat p_ploat)
{
	return p_ploat;
}

inline MCGFloat MCGFloatFromSkScalar(SkScalar p_scalar)
{
	return p_scalar;
}

inline SkScalar MCGCoordToSkCoord(MCGFloat p_coord)
{
	// TODO: Shift coordinate appropriately 
	return p_coord;
}

inline MCGFloat MCGCoordFromSkCoord(SkScalar p_coord)
{
	// TODO: Shift coordinate appropriately 
	return p_coord;
}

inline SkPaint::Join MCGJoinStyleToSkJoinStyle(MCGJoinStyle p_style)
{
	switch (p_style)
	{
		case kMCGJoinStyleBevel:
			return SkPaint::kBevel_Join;
		case kMCGJoinStyleRound:
			return SkPaint::kRound_Join;
		case kMCGJoinStyleMiter:
            return SkPaint::kMiter_Join;
        default:
            MCUnreachableReturn(SkPaint::kMiter_Join);
	}
}

inline MCGJoinStyle MCGJoinStyleFromSkJoinStyle(SkPaint::Join p_style)
{
	switch (p_style)
	{
		case SkPaint::kBevel_Join:
			return kMCGJoinStyleBevel;
		case SkPaint::kRound_Join:
			return kMCGJoinStyleRound;
		case SkPaint::kMiter_Join:
		default:
			return kMCGJoinStyleMiter;
	}
}

inline SkPaint::Cap MCGCapStyleToSkCapStyle(MCGCapStyle p_style)
{
	switch (p_style)
	{
		case kMCGCapStyleButt:
			return SkPaint::kButt_Cap;
		case kMCGCapStyleRound:
			return SkPaint::kRound_Cap;
		case kMCGCapStyleSquare:
			return SkPaint::kSquare_Cap;
        default:
            MCUnreachableReturn(SkPaint::kButt_Cap);
	}
}

inline MCGCapStyle MCGCapStyleFromSkCapStyle(SkPaint::Cap p_style)
{
	switch (p_style)
	{
		case SkPaint::kRound_Cap:
			return kMCGCapStyleRound;
		case SkPaint::kSquare_Cap:
			return kMCGCapStyleSquare;
		default:
		case SkPaint::kButt_Cap:
			return kMCGCapStyleButt;			
	}
}

inline SkRect MCGRectangleToSkRect(MCGRectangle p_rect)
{
	return SkRect::MakeXYWH(MCGCoordToSkCoord(p_rect . origin . x), MCGCoordToSkCoord(p_rect . origin . y), MCGFloatToSkScalar(p_rect . size . width), MCGFloatToSkScalar(p_rect . size . height));
}

inline MCGRectangle MCGRectangleFromSkRect(SkRect p_rect)
{
	return MCGRectangleMake(MCGCoordFromSkCoord(p_rect . x()), MCGCoordFromSkCoord(p_rect . y()), p_rect . width(), p_rect . height());
}

inline SkPoint MCGPointToSkPoint(MCGPoint p_point)
{
	return SkPoint::Make(MCGCoordToSkCoord(p_point . x), MCGCoordToSkCoord(p_point . y));
}

inline MCGPoint MCGPointFromSkPoint(SkPoint p_point)
{
	MCGPoint t_point;
	t_point . x = MCGCoordFromSkCoord(p_point . x());
	t_point . y = MCGCoordFromSkCoord(p_point . y());
	return t_point;
}

inline SkPath::FillType MCGFillRuleToSkFillType(MCGFillRule p_rule)
{
	switch (p_rule)
	{
		case kMCGFillRuleNonZero:
			return SkPath::kWinding_FillType;
		case kMCGFillRuleEvenOdd:
			return SkPath::kEvenOdd_FillType;
        default:
            MCUnreachableReturn(SkPath::kEvenOdd_FillType);
	}
}

inline MCGFillRule MCGFillRuleToSkFillType(SkPath::FillType p_fill_type)
{
	switch (p_fill_type)
	{
		case SkPath::kWinding_FillType:
			return kMCGFillRuleNonZero;
		case SkPath::kEvenOdd_FillType:
		default:
			return kMCGFillRuleEvenOdd;
	}
}

////////////////////////////////////////////////////////////////////////////////

inline SkColorType MCGRasterFormatToSkBitmapConfig(MCGRasterFormat p_format)
{
	switch (p_format)
	{
	case kMCGRasterFormat_A:
		return kAlpha_8_SkColorType;
	case kMCGRasterFormat_ARGB:
	case kMCGRasterFormat_U_ARGB:
	case kMCGRasterFormat_xRGB:
		return kN32_SkColorType;
    default:
        MCUnreachableReturn(kAlpha_8_SkColorType);
	}
}

// IM-2014-05-20: [[ GraphicsPerformance ]] Use bitmap opaqueness when determining raster format
inline MCGRasterFormat MCGRasterFormatFromSkImageInfo(const SkImageInfo& p_config, bool p_opaque)
{
	switch (p_config.colorType())
	{
	case kAlpha_8_SkColorType:
		return kMCGRasterFormat_A;
	case kBGRA_8888_SkColorType:
	case kRGBA_8888_SkColorType:
		return p_opaque ? kMCGRasterFormat_xRGB : kMCGRasterFormat_ARGB;
    default:
        MCUnreachableReturn(kMCGRasterFormat_A);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCGImageCreateWithSkBitmap(const SkBitmap &p_bitmap, MCGImageRef &r_image);

////////////////////////////////////////////////////////////////////////////////

enum MCGBlurType
{
	kMCGBlurTypeNormal,
	kMCGBlurTypeInner,
	kMCGBlurTypeInvertedInner,
	kMCGBlurTypeOuter,
};

bool MCGBlurBox(const SkMask& p_src, SkScalar p_x_radius, SkScalar p_y_radius, SkScalar p_x_spread, SkScalar p_y_spread, SkMask& r_dst);

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCGCacheTable *MCGCacheTableRef;

bool MCGCacheTableCreate(uindex_t size, uindex_t max_occupancy, uindex_t max_bytes, MCGCacheTableRef &r_cache_table);
void MCGCacheTableDestroy(MCGCacheTableRef cache_table);
void MCGCacheTableCompact(MCGCacheTableRef cache_table);
void MCGCacheTableSet(MCGCacheTableRef cache_table, void *key, uint32_t key_length, void *value, uint32_t value_length);
void *MCGCacheTableGet(MCGCacheTableRef cache_table, void *key, uint32_t key_length);

////////////////////////////////////////////////////////////////////////////////

void MCGContextSetFillPaint(MCGContextRef p_context, MCGPaintRef p_paint);
void MCGContextSetStrokePaint(MCGContextRef p_context, MCGPaintRef p_paint);

bool MCGContextSetupFill(MCGContextRef p_context, SkPaint& r_paint);

// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform);

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void);
void MCGPlatformFinalize(void);

void MCGTextMeasureCacheInitialize(void);
void MCGTextMeasureCacheFinalize(void);
void MCGTextMeasureCacheCompact(void);

void MCGBlendModesInitialize(void);
void MCGBlendModesFinalize(void);

////////////////////////////////////////////////////////////////////////////////

struct __MCGRegion
{
	SkRegion region;
};

////////////////////////////////////////////////////////////////////////////////

#endif
