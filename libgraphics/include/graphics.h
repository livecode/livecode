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

#ifndef __MC_GRAPHICS__
#define __MC_GRAPHICS__

#include "foundation.h"
#include "foundation-auto.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCGContext *MCGContextRef;
typedef struct __MCGPath *MCGPathRef;
typedef struct __MCGImage *MCGImageRef;
typedef struct __MCGMask *MCGMaskRef;

typedef struct __MCGDashes *MCGDashesRef;
typedef struct __MCGRegion *MCGRegionRef;

typedef class MCGPaint *MCGPaintRef;

////////////////////////////////////////////////////////////////////////////////

// Pixel format (32bit)

// OrderRGB flag - true if colors are ordered RGB rather than BGR
#define kMCGPixelOrderRGB (1 << 0)
// AlphaPositionFirst flag - true if alpha channel is in first byte rather than last
#define kMCGPixelAlphaPositionFirst (1 << 1)

// Recognised pixel formats
#define kMCGPixelFormatBGRA (0)
#define kMCGPixelFormatRGBA (kMCGPixelOrderRGB)
#define kMCGPixelFormatABGR (kMCGPixelAlphaPositionFirst)
#define kMCGPixelFormatARGB (kMCGPixelAlphaPositionFirst | kMCGPixelOrderRGB)

typedef uint32_t MCGPixelFormat;

#if defined(ANDROID) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(_LINUX) || defined(__EMSCRIPTEN__) || defined(__MAC__)
#  define kMCGPixelFormatNative kMCGPixelFormatRGBA
#elif defined(WIN32)
#  define kMCGPixelFormatNative kMCGPixelFormatBGRA
#else
#  error "Unknown pixel format for this platform"
#endif

// IM-2013-11-01: [[ RefactorGraphics ]] Reverse component shift values on big-endian architectures
static inline uint32_t __MCGPixelPackComponents(uint8_t p_1, uint8_t p_2, uint8_t p_3, uint8_t p_4)
{
#ifdef __LITTLE_ENDIAN__
	return p_1 | (uint32_t(p_2) << 8) | (uint32_t(p_3) << 16) | (uint32_t(p_4) << 24);
#else
	return (uint32_t(p_1) << 24) | (uint32_t(p_2) << 16) | (uint32_t(p_3) << 8) | uint32_t(p_4);
#endif
}

static inline uint32_t MCGPixelPack(MCGPixelFormat p_format, uint8_t p_red, uint8_t p_green, uint8_t p_blue, uint8_t p_alpha)
{
	switch (p_format)
	{
		case kMCGPixelFormatRGBA:
			return __MCGPixelPackComponents(p_red, p_green, p_blue, p_alpha);
			
		case kMCGPixelFormatBGRA:
			return __MCGPixelPackComponents(p_blue, p_green, p_red, p_alpha);
			
		case kMCGPixelFormatABGR:
			return __MCGPixelPackComponents(p_alpha, p_blue, p_green, p_red);
			
		case kMCGPixelFormatARGB:
			return __MCGPixelPackComponents(p_alpha, p_red, p_green, p_blue);
        default:
            MCUnreachableReturn(0);
	}
}

static inline uint32_t MCGPixelPackNative(uint8_t p_red, uint8_t p_green, uint8_t p_blue, uint8_t p_alpha)
{
	return MCGPixelPack(kMCGPixelFormatNative, p_red, p_green, p_blue, p_alpha);
}

// IM-2013-11-01: [[ RefactorGraphics ]] Reverse component shift values on big-endian architectures
static inline void __MCGPixelUnpackComponents(uint32_t p_pixel, uint8_t &r_1, uint8_t &r_2, uint8_t &r_3, uint8_t &r_4)
{
#ifdef __LITTLE_ENDIAN__
	r_1 = (p_pixel >>  0) & 0xFF;
	r_2 = (p_pixel >>  8) & 0xFF;
	r_3 = (p_pixel >> 16) & 0xFF;
	r_4 = (p_pixel >> 24) & 0xFF;
#else
	r_1 = (p_pixel >> 24) & 0xFF;
	r_2 = (p_pixel >> 16) & 0xFF;
	r_3 = (p_pixel >>  8) & 0xFF;
	r_4 = (p_pixel >>  0) & 0xFF;
#endif
}

static inline void MCGPixelUnpack(MCGPixelFormat p_format, uint32_t p_pixel, uint8_t &r_red, uint8_t &r_green, uint8_t &r_blue, uint8_t &r_alpha)
{
	switch (p_format)
	{
		case kMCGPixelFormatRGBA:
			__MCGPixelUnpackComponents(p_pixel, r_red, r_green, r_blue, r_alpha);
			return;
			
		case kMCGPixelFormatBGRA:
			__MCGPixelUnpackComponents(p_pixel, r_blue, r_green, r_red, r_alpha);
			return;
			
		case kMCGPixelFormatABGR:
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_blue, r_green, r_red);
			return;
			
		case kMCGPixelFormatARGB:
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_red, r_green, r_blue);
			return;
	}
	r_red = r_green = r_blue = r_alpha = 0;
	MCUnreachable();
}

static inline void MCGPixelUnpackNative(uint32_t p_pixel, uint8_t &r_red, uint8_t &r_green, uint8_t &r_blue, uint8_t &r_alpha)
{
	return MCGPixelUnpack(kMCGPixelFormatNative, p_pixel, r_red, r_green, r_blue, r_alpha);
}

// IM-2013-11-01: [[ RefactorGraphics ]] Reverse component shift values on big-endian architectures
static inline uint8_t MCGPixelGetAlpha(MCGPixelFormat p_format, uint32_t p_pixel)
{
#ifdef __LITTLE_ENDIAN__
	if (p_format & kMCGPixelAlphaPositionFirst)
		return p_pixel & 0xFF;
	else
		return p_pixel >> 24;
#else
	if ((p_format & kMCGPixelAlphaPositionFirst) == 0)
		return p_pixel & 0xFF;
	else
		return p_pixel >> 24;
#endif
}

static inline uint32_t MCGPixelSetAlpha(MCGPixelFormat p_format, uint32_t p_pixel, uint8_t p_new_alpha)
{
#ifdef __LITTLE_ENDIAN__
	if (p_format & kMCGPixelAlphaPositionFirst)
		return (p_pixel & 0xFFFFFF00) | p_new_alpha;
	else
		return (p_pixel & 0x00FFFFFF) | (uint32_t(p_new_alpha) << 24);
#else
	if ((p_format & kMCGPixelAlphaPositionFirst) == 0)
		return (p_pixel & 0xFFFFFF00) | p_new_alpha;
	else
		return (p_pixel & 0x00FFFFFF) | (uint32_t(p_new_alpha) << 24);
#endif
}

// IM-2013-11-01: [[ RefactorGraphics ]] Reverse component shift values on big-endian architectures
static inline uint8_t MCGPixelGetNativeAlpha(uint32_t p_pixel)
{
#ifdef __LITTLE_ENDIAN__
	#if kMCGPixelFormatNative & kMCGPixelAlphaPositionFirst
		return p_pixel & 0xFF;
	#else
		return p_pixel >> 24;
	#endif
#else
	#if (kMCGPixelFormatNative & kMCGPixelAlphaPositionFirst) == 0
		return p_pixel & 0xFF;
	#else
		return p_pixel >> 24;
	#endif
#endif
}

static inline uint32_t MCGPixelSetNativeAlpha(uint32_t p_pixel, uint8_t p_new_alpha)
{
#ifdef __LITTLE_ENDIAN__
	#if kMCGPixelFormatNative & kMCGPixelAlphaPositionFirst
		return (p_pixel & 0xFFFFFF00) | p_new_alpha;
	#else
		return (p_pixel & 0x00FFFFFF) | (uint32_t(p_new_alpha) << 24);
	#endif
#else
	#if (kMCGPixelFormatNative & kMCGPixelAlphaPositionFirst) == 0
		return (p_pixel & 0xFFFFFF00) | p_new_alpha;
	#else
		return (p_pixel & 0x00FFFFFF) | (uint32_t(p_new_alpha) << 24);
	#endif
#endif
}

static inline uint32_t MCGPixelToNative(MCGPixelFormat p_src_format, uint32_t p_src_pixel)
{
	uint8_t r, g, b, a;
	MCGPixelUnpack(p_src_format, p_src_pixel, r, g, b, a);
	
	return MCGPixelPackNative(r, g, b, a);
}

static inline uint32_t MCGPixelFromNative(MCGPixelFormat p_dst_format, uint32_t p_native_pixel)
{
	uint8_t r, g, b, a;
	MCGPixelUnpackNative(p_native_pixel, r, g, b, a);
	
	return MCGPixelPack(p_dst_format, r, g, b, a);
}

//////////

static inline uint32_t __mcgpixel_packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;
	
	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u + v;
}

// IM-2014-07-23: [[ Bug 12892 ]] Return the premultiplied pixel value assuming native pixel format
static inline uint32_t MCGPixelPreMultiplyNative(uint32_t p_pixel)
{
	uint8_t t_alpha;
	t_alpha = MCGPixelGetNativeAlpha(p_pixel);
	
	return MCGPixelSetNativeAlpha(__mcgpixel_packed_scale_bounded(p_pixel, t_alpha), t_alpha);
}

////////////////////////////////////////////////////////////////////////////////

typedef float MCGFloat;
typedef uint32_t MCGColor;

struct MCGColor4f
{
    MCGFloat red;
    MCGFloat green;
    MCGFloat blue;
    MCGFloat alpha;
};

struct MCGPoint
{
    /* TODO[C++14] In C++11, aggregate initialisation of object types
     * with member variables that have static initialisers is
     * forbidden.  In C++14, this restriction is relaxed (so these
     * constructors can be removed). */
    constexpr MCGPoint() = default;
    constexpr MCGPoint(MCGFloat p_x, MCGFloat p_y) : x(p_x), y(p_y) {}

    MCGFloat x = 0;
    MCGFloat y = 0;
};

struct MCGSize
{
    /* TODO[C++14] In C++11, aggregate initialisation of object types
     * with member variables that have static initialisers is
     * forbidden.  In C++14, this restriction is relaxed (so these
     * constructors can be removed). */
    constexpr MCGSize() = default;
    constexpr MCGSize(MCGFloat p_width, MCGFloat p_height)
        : width(p_width), height(p_height) {}

    MCGFloat width = 0;
    MCGFloat height = 0;
};

struct MCGRectangle
{
    /* TODO[C++14] In C++11, aggregate initialisation of object types
     * with member variables that have static initialisers is
     * forbidden.  In C++14, this restriction is relaxed (so these
     * constructors can be removed). */
    constexpr MCGRectangle() = default;
    constexpr MCGRectangle(MCGPoint p_origin, MCGSize p_size)
        : origin(p_origin), size(p_size) {}

    MCGPoint origin;
    MCGSize size;
};

struct MCGAffineTransform
{
	MCGFloat a = 0;
	MCGFloat b = 0;
	MCGFloat c = 0;
	MCGFloat d = 0;
	MCGFloat tx = 0;
	MCGFloat ty = 0;
};

struct MCGIntegerPoint
{
	int32_t x = 0;
	int32_t y = 0;
};

struct MCGIntegerSize
{
	uint32_t width = 0;
	uint32_t height = 0;
};

struct MCGIntegerRectangle
{
	MCGIntegerPoint origin;
	MCGIntegerSize size;
};

////////////////////////////////////////////////////////////////////////////////

enum MCGFillRule
{
	kMCGFillRuleNonZero,
	kMCGFillRuleEvenOdd,
};

static const intenum_t kMCGFillRuleCount = 2;


enum MCGPaintStyle
{
	kMCGPaintStyleOpaque,
	kMCGPaintStyleStippled,
};

enum MCGBlendMode
{
	kMCGBlendModeClear,
	kMCGBlendModeCopy,
	kMCGBlendModeSourceOver,
	kMCGBlendModeSourceIn,
	kMCGBlendModeSourceOut,
	kMCGBlendModeSourceAtop,
	kMCGBlendModeDestinationOver,
	kMCGBlendModeDestinationIn,
	kMCGBlendModeDestinationOut,
	kMCGBlendModeDestinationAtop,
	kMCGBlendModeXor,
	kMCGBlendModePlusDarker,
	kMCGBlendModePlusLighter,
	kMCGBlendModeMultiply,
	kMCGBlendModeScreen,
	kMCGBlendModeOverlay,
	kMCGBlendModeDarken,
	kMCGBlendModeLighten,
	kMCGBlendModeColorDodge,
	kMCGBlendModeColorBurn,
	kMCGBlendModeSoftLight,
	kMCGBlendModeHardLight,
	kMCGBlendModeDifference,
	kMCGBlendModeExclusion,
	kMCGBlendModeHue,
	kMCGBlendModeSaturation,
	kMCGBlendModeColor,
	kMCGBlendModeLuminosity,
	
	// legacy blend modes	
	kMCGBlendModeLegacyClear,
	kMCGBlendModeLegacyAnd,
	kMCGBlendModeLegacyAndReverse,
	kMCGBlendModeLegacyCopy,
	kMCGBlendModeLegacyInverted,
	kMCGBlendModeLegacyNoop,
	kMCGBlendModeLegacyXor,
	kMCGBlendModeLegacyOr,
	kMCGBlendModeLegacyNor,
	kMCGBlendModeLegacyEquiv,
	kMCGBlendModeLegacyInvert,
	kMCGBlendModeLegacyOrReverse,
	kMCGBlendModeLegacyCopyInverted,
	kMCGBlendModeLegacyOrInverted,
	kMCGBlendModeLegacyNand,
	kMCGBlendModeLegacySet,
	kMCGBlendModeLegacyBlend,
	kMCGBlendModeLegacyAddPin,
	kMCGBlendModeLegacyAddOver,
	kMCGBlendModeLegacySubPin,
	kMCGBlendModeLegacyTransparent,
	kMCGBlendModeLegacyAdMax,
	kMCGBlendModeLegacySubOver,
	kMCGBlendModeLegacyAdMin,
	kMCGBlendModeLegacyBlendSource,
	kMCGBlendModeLegacyBlendDestination,
};

static const intenum_t kMCGBlendModeCount = 54;


enum MCGJoinStyle
{
	kMCGJoinStyleBevel,
	kMCGJoinStyleRound,
	kMCGJoinStyleMiter,
};

static const intenum_t kMCGJoinStyleCount = 3;


enum MCGCapStyle
{
	kMCGCapStyleButt,
	kMCGCapStyleRound,
	kMCGCapStyleSquare,
};

static const intenum_t kMCGCapStyleCount = 3;


enum MCGRasterFormat
{
	kMCGRasterFormat_xRGB, // no alpha channel
	kMCGRasterFormat_ARGB, // premultiplied alpha channel
	kMCGRasterFormat_U_ARGB, // Unpremultipled alpha channel
	kMCGRasterFormat_A, // alpha mask
};

// MM-2014-01-09: [[ ImageFilterUpdate ]] Updated filters to use Skia's new filter levels.
enum MCGImageFilter
{
	kMCGImageFilterNone,
	kMCGImageFilterLow,
	kMCGImageFilterMedium,
    kMCGImageFilterHigh,
};

static const intenum_t kMCGImageFilterCount = 4;


enum MCGGradientFunction
{
	kMCGGradientFunctionLinear,
	kMCGGradientFunctionRadial,
	kMCGGradientFunctionSweep,
	
	kMCGLegacyGradientDiamond,
	kMCGLegacyGradientSpiral,
	kMCGLegacyGradientXY,
	kMCGLegacyGradientSqrtXY,
};

static const intenum_t kMCGGradientFunctionCount = 7;


enum MCGGradientSpreadMethod
{
	kMCGGradientSpreadMethodPad,
	kMCGGradientSpreadMethodReflect,
	kMCGGradientSpreadMethodRepeat,
};

enum MCGMaskFormat
{
	kMCGMaskFormat_A1,
	kMCGMaskFormat_A8,
	kMCGMaskFormat_LCD32,
};

struct MCGRaster
{
	MCGRasterFormat format = kMCGRasterFormat_xRGB;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t stride = 0;
	void *pixels = nullptr;
};

struct MCGStrokeAttr
{
	MCGFloat width = 0;
	MCGJoinStyle join_style = kMCGJoinStyleBevel;
	MCGCapStyle cap_style = kMCGCapStyleButt;
	MCGFloat miter_limit = 0;
	MCGDashesRef dashes = nullptr;
};

struct MCGLayerEffect
{
	MCGColor color = 0;
	MCGBlendMode blend_mode = kMCGBlendModeClear;
};

struct MCGShadowEffect
{
	MCGColor color = 0;
	MCGBlendMode blend_mode = kMCGBlendModeClear;
	MCGFloat size = 0;
	MCGFloat spread = 0;
	MCGFloat x_offset = 0;
	MCGFloat y_offset = 0;
	bool knockout = false;
};

struct MCGGlowEffect
{
	MCGColor color = 9;
	MCGBlendMode blend_mode = kMCGBlendModeClear;
	MCGFloat size = 0;
	MCGFloat spread = 0;
	bool inverted = false;
};

struct MCGBitmapEffects
{
	MCGLayerEffect color_overlay;
	MCGGlowEffect inner_glow;
	MCGShadowEffect inner_shadow;
	MCGGlowEffect outer_glow;
	MCGShadowEffect drop_shadow;

	bool has_color_overlay = false;
	bool has_inner_glow = false;
	bool has_inner_shadow = false;
	bool has_outer_glow = false;
	bool has_drop_shadow = false;
	
	bool isolated = false;
};

struct MCGDeviceMaskInfo
{
	MCGMaskFormat format = kMCGMaskFormat_A1;
	int32_t x = 0;
	int32_t y = 0;
	int32_t width = 0;
	int32_t height = 0;
	void *data = nullptr;
};

struct MCGFont
{
	void		*fid = nullptr;
	uint16_t	size = 0;
	uint16_t	fixed_advance = 0;
	MCGFloat	m_ascent = 0;
	MCGFloat	m_descent = 0;
    MCGFloat    m_leading = 0;
    bool		ideal = false;
};

////////////////////////////////////////////////////////////////////////////////

inline MCGFont MCGFontMake(void *fid, uint16_t size, uint16_t fixed_advance, MCGFloat ascent, MCGFloat descent, MCGFloat leading, bool ideal)
{
    MCGFont t_font;
    t_font . fid = fid;
	t_font . size = size;
	t_font . fixed_advance = fixed_advance;
	t_font . m_ascent = ascent;
	t_font . m_descent = descent;
    t_font . m_leading = leading;
	t_font . ideal = ideal;
    
    return t_font;
}

// Make the font resources contained in the specified file available for use.
bool MCGFontAddPlatformFileResource(MCStringRef p_file_resource_path);
bool MCGFontRemovePlatformFileResource(MCStringRef p_file_resource_path);

// Get list of available font families
bool MCGFontGetPlatformFontList(MCProperListRef &r_fonts);

////////////////////////////////////////////////////////////////////////////////

struct MCGGlyphInfo
{
	uindex_t codepoint;
	uindex_t cluster;

	MCGFloat x_offset;
	MCGFloat y_offset;
	MCGFloat x_advance;
	MCGFloat y_advance;
};

typedef bool (*MCGFontLayoutTextCallback)(void *context, const MCGFont &p_font, const MCGGlyphInfo *p_glyphs, uindex_t p_glyph_count, const unichar_t *p_chars, uindex_t p_char_count);

bool MCGFontLayoutText(const MCGFont &p_font, const unichar_t *p_text, uindex_t p_char_count, bool p_rtl, MCGFontLayoutTextCallback p_callback, void *p_context);

////////////////////////////////////////////////////////////////////////////////

inline bool MCGPointIsEqual(const MCGPoint &p_a, const MCGPoint &p_b)
{
	return p_a.x == p_b.x && p_a.y == p_b.y;
}

inline bool MCGSizeIsEqual(const MCGSize &p_a, const MCGSize &p_b)
{
	return p_a.width == p_b.width && p_a.height == p_b.height;
}

////////////////////////////////////////////////////////////////////////////////

inline MCGColor MCGColorComponentFromFloat(MCGFloat p_component)
{
    return MCGColor(MCClamp(p_component * UINT8_MAX, 0, UINT8_MAX));
}

inline MCGFloat MCGColorComponentToFloat(MCGColor p_component)
{
    return (p_component & UINT8_MAX) / MCGFloat(UINT8_MAX);
}

inline MCGColor MCGColorMakeRGBA(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha)
{
    return ((MCGColorComponentFromFloat(p_alpha) << 24) |
            (MCGColorComponentFromFloat(p_red)   << 16) |
            (MCGColorComponentFromFloat(p_green) <<  8) |
            (MCGColorComponentFromFloat(p_blue)  <<  0));
}

inline void MCGColorSetRed(MCGColor& x_color, MCGFloat p_red) {
    x_color = (x_color & 0xFF00FFFF) | (MCGColorComponentFromFloat(p_red) << 16);
}

inline void MCGColorSetGreen(MCGColor& x_color, MCGFloat p_green) {
    x_color = (x_color & 0xFFFF00FF) | (MCGColorComponentFromFloat(p_green) << 8);
}

inline void MCGColorSetBlue(MCGColor& x_color, MCGFloat p_blue) {
    x_color = (x_color & 0xFFFFFF00) | (MCGColorComponentFromFloat(p_blue) << 0);
}

inline void MCGColorSetAlpha(MCGColor& x_color, MCGFloat p_alpha) {
    x_color = (x_color & 0x00FFFFFF) | (MCGColorComponentFromFloat(p_alpha) << 24);
}

inline MCGFloat MCGColorGetRed(MCGColor p_color) {
    return MCGColorComponentToFloat(p_color >> 16);
}

inline MCGFloat MCGColorGetGreen(MCGColor p_color) {
    return MCGColorComponentToFloat(p_color >> 8);
}

inline MCGFloat MCGColorGetBlue(MCGColor p_color) {
    return MCGColorComponentToFloat(p_color >> 0);
}

inline MCGFloat MCGColorGetAlpha(MCGColor p_color) {
    return MCGColorComponentToFloat(p_color >> 24);
}

////////////////////////////////////////////////////////////////////////////////

inline MCGRectangle MCGRectangleMake(MCGFloat p_x, MCGFloat p_y, MCGFloat p_width, MCGFloat p_height)
{
	MCGRectangle t_rect;
	t_rect . origin . x = p_x;
	t_rect . origin . y = p_y;
	t_rect . size . width = p_width;
	t_rect . size . height = p_height;
	return t_rect;
}

inline MCGRectangle MCGRectangleTranslate(MCGRectangle p_rect, MCGFloat p_dx, MCGFloat p_dy)
{
	MCGRectangle t_rect = p_rect;
	t_rect.origin.x += p_dx;
	t_rect.origin.y += p_dy;
	
	return t_rect;
}

inline MCGRectangle MCGRectangleScale(const MCGRectangle &p_rect, MCGFloat p_h_scale, MCGFloat p_v_scale)
{
	return MCGRectangleMake(p_rect.origin.x * p_h_scale, p_rect.origin.y * p_v_scale, p_rect.size.width * p_h_scale, p_rect.size.height * p_v_scale);
}

inline MCGRectangle MCGRectangleScale(const MCGRectangle &p_rect, MCGFloat p_scale)
{
	return MCGRectangleScale(p_rect, p_scale, p_scale);
}

inline bool MCGRectangleIsEmpty(const MCGRectangle &p_rect)
{
	return p_rect.size.width == 0.0 || p_rect.size.height == 0.0;
}

inline bool MCGRectangleIsEqual(const MCGRectangle &p_a, const MCGRectangle &p_b)
{
	return MCGPointIsEqual(p_a.origin, p_b.origin) && MCGSizeIsEqual(p_a.size, p_b.size);
}

MCGRectangle MCGRectangleIntersection(const MCGRectangle &rect_1, const MCGRectangle &rect_2);
MCGRectangle MCGRectangleUnion(const MCGRectangle &rect_1, const MCGRectangle &rect_2);

inline MCGPoint MCGPointMake(MCGFloat p_x, MCGFloat p_y)
{
	MCGPoint t_point;
	t_point . x = p_x;
	t_point . y = p_y;
	return t_point;
}

inline MCGPoint MCGPointTranslate(const MCGPoint &p_point, MCGFloat p_dx, MCGFloat p_dy)
{
	return MCGPointMake(p_point.x + p_dx, p_point.y + p_dy);
}

inline MCGPoint MCGPointScale(const MCGPoint &p_point, MCGFloat p_h_scale, MCGFloat p_v_scale)
{
	return MCGPointMake(p_point.x * p_h_scale, p_point.y * p_v_scale);
}

inline MCGPoint MCGPointScale(const MCGPoint &p_point, MCGFloat p_scale)
{
	return MCGPointScale(p_point, p_scale, p_scale);
}

inline bool MCGPointInRectangle(MCGPoint p, MCGRectangle r)
{
    return p . x >= r . origin . x &&
            p . y >= r . origin . y &&
             p . x < (r . origin . x + r . size . width) &&
              p . y < (r . origin . y + r . size . height);
}

inline MCGSize MCGSizeMake(MCGFloat p_w, MCGFloat p_h)
{
	MCGSize t_size;
	t_size . width = p_w;
	t_size . height = p_h;
	return t_size;
}

inline MCGIntegerPoint MCGIntegerPointMake(int32_t x, int32_t y)
{
	MCGIntegerPoint t_point;
	t_point.x = x;
	t_point.y = y;

	return t_point;
}

inline MCGIntegerSize MCGIntegerSizeMake(uint32_t width, uint32_t height)
{
	MCGIntegerSize t_size;
	t_size.width = width;
	t_size.height = height;

	return t_size;
}

inline MCGIntegerRectangle MCGIntegerRectangleMake(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	MCGIntegerRectangle t_rect;
	t_rect.origin.x = x;
	t_rect.origin.y = y;
	t_rect.size.width = width;
	t_rect.size.height = height;
	
	return t_rect;
}

// IM-2014-10-22: [[ Bug 13746 ]] Add convenience function to construct rectangle from left, top, right, bottom coords
inline MCGIntegerRectangle MCGIntegerRectangleMakeLTRB(int32_t l, int32_t t, int32_t r, int32_t b)
{
	return MCGIntegerRectangleMake(l, t, uint32_t(r - l), uint32_t(b - t));
}

inline bool MCGIntegerRectangleIsEmpty(const MCGIntegerRectangle &p_rect)
{
	return p_rect.size.width == 0 || p_rect.size.height == 0;
}

MCGIntegerRectangle MCGIntegerRectangleIntersection(const MCGIntegerRectangle &rect_1, const MCGIntegerRectangle &rect_2);

inline MCGRectangle MCGIntegerRectangleToMCGRectangle(const MCGIntegerRectangle &p_rect)
{
	/* Possible loss of precision */
	return MCGRectangleMake(MCGFloat(p_rect.origin.x),
	                        MCGFloat(p_rect.origin.y),
	                        MCGFloat(p_rect.size.width),
	                        MCGFloat(p_rect.size.height));
}

MCGIntegerRectangle MCGRectangleGetBounds(const MCGRectangle &p_rect);

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsInitialize(void);
void MCGraphicsFinalize(void);
void MCGraphicsCompact(void);

////////////////////////////////////////////////////////////////////////////////

bool MCGPaintCreateWithNone(MCGPaintRef& r_paint);
bool MCGPaintCreateWithSolidColor(MCGFloat p_red, MCGFloat p_green, MCGFloat p_blue, MCGFloat p_alpha, MCGPaintRef& r_paint);
bool MCGPaintCreateWithPattern(MCGImageRef p_image, MCGAffineTransform p_transform, MCGImageFilter p_filter, MCGPaintRef& r_paint);
bool MCGPaintCreateWithGradient(MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter, MCGPaintRef& r_paint);

MCGPaintRef MCGPaintRetain(MCGPaintRef paint);
void MCGPaintRelease(MCGPaintRef paint);

////////////////////////////////////////////////////////////////////////////////

// Create new image with pixel data copied from raster
bool MCGImageCreateWithRaster(const MCGRaster& raster, MCGImageRef& r_image);

// Create new image with pixel data provided by raster - raster retains ownership of pixel data
bool MCGImageCreateWithRasterNoCopy(const MCGRaster &raster, MCGImageRef &r_image);

// Create new image with pixel data provided by raster - new image takes ownership of pixel data
bool MCGImageCreateWithRasterAndRelease(const MCGRaster &raster, MCGImageRef &r_image);

bool MCGImageCreateWithData(const void *bytes, uindex_t byte_count, MCGImageRef& r_image);
bool MCGImageCreateWithFilename(const char *filename, MCGImageRef& r_image);

bool MCGImageGetRaster(MCGImageRef image, MCGRaster &r_raster);
bool MCGImageGetPixel(MCGImageRef image, uint32_t x, uint32_t y, uint32_t &r_pixel);

MCGImageRef MCGImageRetain(MCGImageRef image);
void MCGImageRelease(MCGImageRef image);

bool MCGImageIsValid(MCGImageRef image);

int32_t MCGImageGetWidth(MCGImageRef image);
int32_t MCGImageGetHeight(MCGImageRef image);

MCGSize MCImageGetSize(MCGImageRef image);

bool MCGImageIsOpaque(MCGImageRef image);
bool MCGImageHasPartialTransparency(MCGImageRef image);

////////////////////////////////////////////////////////////////////////////////

bool MCGMaskCreateWithInfoAndRelease(const MCGDeviceMaskInfo& info, MCGMaskRef& r_mask);
void MCGMaskRelease(MCGMaskRef mask);
MCGRectangle MCGMaskGetBounds(MCGMaskRef mask);

////////////////////////////////////////////////////////////////////////////////

enum MCGPathCommand
{
	kMCGPathCommandEnd,
	kMCGPathCommandMoveTo,
	kMCGPathCommandLineTo,
	kMCGPathCommandCubicCurveTo,
	kMCGPathCommandQuadCurveTo,
	kMCGPathCommandCloseSubpath,
	
	kMCGPathCommandCount
};

bool MCGPathCreate(const MCGPathCommand *commands, const MCGFloat *parameters, MCGPathRef& r_path);
bool MCGPathCreateMutable(MCGPathRef& r_path);

MCGPathRef MCGPathRetain(MCGPathRef path);
void MCGPathRelease(MCGPathRef path);

bool MCGPathIsValid(MCGPathRef path);
bool MCGPathIsEmpty(MCGPathRef path);
bool MCGPathIsEqualTo(MCGPathRef a, MCGPathRef b);

void MCGPathCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);

bool MCGPathMutableCopySubpaths(MCGPathRef self, uint32_t p_first, uint32_t p_last, MCGPathRef &r_subpaths);

void MCGPathAddRectangle(MCGPathRef path, MCGRectangle bounds);
void MCGPathAddRoundedRectangle(MCGPathRef path, MCGRectangle bounds, MCGSize corner_radii);
void MCGPathAddEllipse(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation);
void MCGPathAddArc(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddSector(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddSegment(MCGPathRef path, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGPathAddLine(MCGPathRef path, MCGPoint start, MCGPoint finish);
void MCGPathAddPolygon(MCGPathRef path, const MCGPoint *points, uindex_t arity);
void MCGPathAddPolyline(MCGPathRef path, const MCGPoint *points, uindex_t arity);
void MCGPathAddPath(MCGPathRef self, MCGPathRef path);

void MCGPathMoveTo(MCGPathRef path, MCGPoint end_point);
void MCGPathLineTo(MCGPathRef path, MCGPoint end_point);
void MCGPathQuadraticTo(MCGPathRef path, MCGPoint control_point, MCGPoint end_point);
void MCGPathCubicTo(MCGPathRef path, MCGPoint first_control_point, MCGPoint second_control_point, MCGPoint end_point);
void MCGPathArcTo(MCGPathRef path, MCGSize radii, MCGFloat rotation, bool large_arc, bool sweep, MCGPoint end_point);
void MCGPathArcToBestFit(MCGPathRef path, MCGSize p_radii, MCGFloat p_rotation, MCGPoint p_end_point);
void MCGPathArcToTangent(MCGPathRef path, const MCGPoint &p_tangent, const MCGPoint &p_end, MCGFloat p_radius);
void MCGPathCloseSubpath(MCGPathRef path);

void MCGPathThicken(MCGPathRef path, const MCGStrokeAttr& attr, MCGPathRef& r_thick_path);
void MCGPathFlatten(MCGPathRef path, MCGFloat flatness, MCGPathRef& r_flat_path);
void MCGPathSimplify(MCGPathRef path, MCGPathRef& r_simple_path);

bool MCGPathTransform(MCGPathRef path, const MCGAffineTransform &p_transform);

bool MCGPathGetCurrentPoint(MCGPathRef self, MCGPoint &r_current);
bool MCGPathGetPreviousPoint(MCGPathRef self, MCGPoint &r_last);
bool MCGPathGetBoundingBox(MCGPathRef path, MCGRectangle &r_bounds);

typedef bool (*MCGPathIterateCallback)(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count);
bool MCGPathIterate(MCGPathRef p_path, MCGPathIterateCallback p_callback, void *p_context);

////////////////////////////////////////////////////////////////////////////////

extern "C" bool MCGContextCreate(uint32_t width, uint32_t height, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithPixels(uint32_t width, uint32_t height, uint32_t stride, void *pixels, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context);

MCGContextRef MCGContextRetain(MCGContextRef context);
void MCGContextRelease(MCGContextRef context);

// Return a pointer to the underlying pixel data
void *MCGContextGetPixelPtr(MCGContextRef context);

// Return the width, and height.
uint32_t MCGContextGetWidth(MCGContextRef context);
uint32_t MCGContextGetHeight(MCGContextRef context);

// Returns whether the current context context is valid. If an error
// occurs when calling any method on a context context, it will become
// invalid and all further operations will be no-ops.
bool MCGContextIsValid(MCGContextRef context);

// Returns whether the current context layer is fully opaque or contains a transparency channel.
bool MCGContextIsLayerOpaque(MCGContextRef context);

// Graphics state operations
void MCGContextSave(MCGContextRef context);
void MCGContextRestore(MCGContextRef context);

// General attributes
void MCGContextSetFlatness(MCGContextRef context, MCGFloat flatness);
void MCGContextSetShouldAntialias(MCGContextRef context, bool should_antialias);

// Transform attribute
void MCGContextSetTransform(MCGContextRef context, MCGAffineTransform p_transform);

// Layer attributes and manipulation - bitmap effect options would be added here also.
void MCGContextSetOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetBlendMode(MCGContextRef context, MCGBlendMode mode);

void MCGContextBegin(MCGContextRef context, bool p_need_layer);
void MCGContextBeginWithEffects(MCGContextRef context, MCGRectangle shape, const MCGBitmapEffects &effects);
void MCGContextEnd(MCGContextRef context);

void MCGContextClipToRect(MCGContextRef context, MCGRectangle rect);
void MCGContextClipToPath(MCGContextRef context, MCGPathRef path);
void MCGContextSetClipToRect(MCGContextRef context, MCGRectangle rect);
MCGRectangle MCGContextGetClipBounds(MCGContextRef context);
MCGRectangle MCGContextGetDeviceClipBounds(MCGContextRef context);

void MCGContextSetClipToDeviceRegion(MCGContextRef self, MCGRegionRef p_region);
void MCGContextClipToDeviceRegion(MCGContextRef self, MCGRegionRef p_region);
void MCGContextSetClipToRegion(MCGContextRef self, MCGRegionRef p_region);
void MCGContextClipToRegion(MCGContextRef self, MCGRegionRef p_region);

// Fill attributes
void MCGContextSetFillRule(MCGContextRef context, MCGFillRule rule);
void MCGContextSetFillOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetFillPaint(MCGContextRef context, MCGPaintRef paint);
void MCGContextSetFillNone(MCGContextRef context);
void MCGContextSetFillRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetFillPattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetFillGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetFillPaintStyle(MCGContextRef context, MCGPaintStyle style);

// Stroke attributes
void MCGContextSetStrokeOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetStrokePaint(MCGContextRef context, MCGPaintRef paint);
void MCGContextSetStrokeNone(MCGContextRef context);
void MCGContextSetStrokeRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetStrokePattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeWidth(MCGContextRef context, MCGFloat width);
void MCGContextSetStrokeMiterLimit(MCGContextRef context, MCGFloat limit);
void MCGContextSetStrokeJoinStyle(MCGContextRef context, MCGJoinStyle style);
void MCGContextSetStrokeCapStyle(MCGContextRef context, MCGCapStyle style);
void MCGContextSetStrokeDashOffset(MCGContextRef context, MCGFloat offset);
void MCGContextSetStrokeDashArray(MCGContextRef context, const MCGFloat* lengths, uindex_t length_count);
void MCGContextSetStrokeDashes(MCGContextRef context, MCGFloat phase, const MCGFloat *lengths, uindex_t arity);
void MCGContextSetStrokePaintStyle(MCGContextRef context, MCGPaintStyle style);

// Transforms - concatenated with the current CTM.
void MCGContextConcatCTM(MCGContextRef context, MCGAffineTransform transform);
void MCGContextRotateCTM(MCGContextRef context, MCGFloat angle);
void MCGContextTranslateCTM(MCGContextRef context, MCGFloat xoffset, MCGFloat yoffset);
void MCGContextScaleCTM(MCGContextRef context, MCGFloat xscale, MCGFloat yscale);
void MCGContextResetCTM(MCGContextRef context);
MCGAffineTransform MCGContextGetDeviceTransform(MCGContextRef context);

// Shape primitives - these add to the current path.
void MCGContextAddRectangle(MCGContextRef context, MCGRectangle bounds);
void MCGContextAddRoundedRectangle(MCGContextRef context, MCGRectangle bounds, MCGSize corner_radii);
void MCGContextAddEllipse(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation);
void MCGContextAddArc(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat p_rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddSector(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddSegment(MCGContextRef context, MCGPoint center, MCGSize radii, MCGFloat rotation, MCGFloat start_angle, MCGFloat finish_angle);
void MCGContextAddLine(MCGContextRef context, MCGPoint start, MCGPoint finish);
void MCGContextAddPolygon(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddPolyline(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddRoundedPolygon(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddRoundedPolyline(MCGContextRef context, const MCGPoint *points, uindex_t arity);
void MCGContextAddDot(MCGContextRef context, MCGPoint location);
void MCGContextAddPath(MCGContextRef context, MCGPathRef path);

// Path primitives
void MCGContextBeginPath(MCGContextRef context);
void MCGContextMoveTo(MCGContextRef context, MCGPoint end_point);
void MCGContextLineTo(MCGContextRef context, MCGPoint end_point);
void MCGContextQuadraticTo(MCGContextRef context, MCGPoint control_point, MCGPoint end_point);
void MCGContextCubicTo(MCGContextRef context, MCGPoint first_control_point, MCGPoint second_control_point, MCGPoint end_point);
void MCGContextArcTo(MCGContextRef context, MCGSize radii, MCGFloat rotation, bool large_arc, bool sweep, MCGPoint end_point);
void MCGContextCloseSubpath(MCGContextRef context);

// Operations

// Copy the current path as an (immutable) path object. If the context is invalid, or
// the path could not be copied the (empty) error path is returned. Note that failure
// to copy the path does *not* cause the context to become invalid, rather the returned
// path is.
void MCGContextCopyPath(MCGContextRef context, MCGPathRef& r_path);

// Fill the current path using the current paint and fill rule. This discards the path.
void MCGContextFill(MCGContextRef context);
// Thicken the current path using the current stroke attributes and fill the resulting
// path using the non-zero rule. This discards the path.
void MCGContextStroke(MCGContextRef context);
// Fills the current path using the current paint and fill rule. Then thicken the current
// path using the current stroke attributes and fill the resulting path using the non-zero
// rule. This discards the path.
void MCGContextFillAndStroke(MCGContextRef context);
// Intersects the current clipping path with the current path; the inside of the current
// path is determined with the current fill rule. This discards the path.
void MCGContextClip(MCGContextRef context);

void MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawImageWithCenter(MCGContextRef context, MCGImageRef image, MCGRectangle image_center, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawRectOfImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_src, MCGRectangle p_dst, MCGImageFilter p_filter);

bool MCGContextCopyImage(MCGContextRef context, MCGImageRef &r_image);

void MCGContextDrawPlatformText(MCGContextRef context, const unichar_t *text, uindex_t length, MCGPoint location, const MCGFont &font, bool p_rtl);
// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat MCGContextMeasurePlatformText(MCGContextRef context, const unichar_t *text, uindex_t length, const MCGFont &p_font, const MCGAffineTransform &p_transform);
bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef context, const unichar_t *text, uindex_t length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds);

void MCGContextPlaybackRectOfDrawing(MCGContextRef context, MCSpan<const byte_t> p_drawing, MCGRectangle p_src, MCGRectangle p_dst, MCGPaintRef p_current_color);

////////////////////////////////////////////////////////////////////////////////

// Transforms

static inline void MCGAffineTransformSet(MCGAffineTransform &x_transform, MCGFloat a, MCGFloat b, MCGFloat c, MCGFloat d, MCGFloat tx, MCGFloat ty)
{
	x_transform.a = a;
	x_transform.b = b;
	x_transform.c = c;
	x_transform.d = d;
	x_transform.tx = tx;
	x_transform.ty = ty;
}

static inline MCGAffineTransform MCGAffineTransformMake(MCGFloat a, MCGFloat b, MCGFloat c, MCGFloat d, MCGFloat tx, MCGFloat ty)
{
	MCGAffineTransform t_transform;
	MCGAffineTransformSet(t_transform, a, b, c, d, tx, ty);
	
	return t_transform;
}

MCGAffineTransform MCGAffineTransformMakeIdentity(void);
MCGAffineTransform MCGAffineTransformMakeRotation(MCGFloat p_angle);
MCGAffineTransform MCGAffineTransformMakeTranslation(MCGFloat p_xoffset, MCGFloat p_yoffset);
MCGAffineTransform MCGAffineTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale);
MCGAffineTransform MCGAffineTransformMakeSkew(MCGFloat p_xskew, MCGFloat p_yskew);

MCGAffineTransform MCGAffineTransformConcat(const MCGAffineTransform& transform_1, const MCGAffineTransform& transform_2);
MCGAffineTransform MCGAffineTransformPreRotate(const MCGAffineTransform& transform, MCGFloat angle);
MCGAffineTransform MCGAffineTransformPostRotate(const MCGAffineTransform& transform, MCGFloat angle);
MCGAffineTransform MCGAffineTransformPreTranslate(const MCGAffineTransform& transform, MCGFloat xoffset, MCGFloat yoffset);
MCGAffineTransform MCGAffineTransformPostTranslate(const MCGAffineTransform& transform, MCGFloat xoffset, MCGFloat yoffset);
MCGAffineTransform MCGAffineTransformPreScale(const MCGAffineTransform& transform, MCGFloat xscale, MCGFloat yscale);
MCGAffineTransform MCGAffineTransformPostScale(const MCGAffineTransform& transform, MCGFloat xscale, MCGFloat yscale);
MCGAffineTransform MCGAffineTransformPreSkew(const MCGAffineTransform &p_transform, MCGFloat p_xskew, MCGFloat p_yskew);
MCGAffineTransform MCGAffineTransformPostSkew(const MCGAffineTransform &p_transform, MCGFloat p_xskew, MCGFloat p_yskew);
MCGAffineTransform MCGAffineTransformInvert(const MCGAffineTransform& transform);

// IM-2014-06-11: [[ Bug 12557 ]] Returns transform that would convert rectangle a to rectangle b by scaling + translating
MCGAffineTransform MCGAffineTransformFromRectangles(const MCGRectangle &p_a, const MCGRectangle &p_b);
bool MCGAffineTransformFromPoints(const MCGPoint p_src[3], const MCGPoint p_dst[3], MCGAffineTransform &r_transform);

MCGPoint MCGPointApplyAffineTransform(const MCGPoint& p_point, const MCGAffineTransform& p_transform);
MCGRectangle MCGRectangleApplyAffineTransform(const MCGRectangle& p_rect, const MCGAffineTransform& p_transform);
MCGSize MCGSizeApplyAffineTransform(const MCGSize& p_size, const MCGAffineTransform& p_transform);

static inline bool MCGAffineTransformHasTranslation(const MCGAffineTransform &p_transform)
{
	return p_transform.tx != 0.0 || p_transform.ty != 0.0;
}

static inline bool MCGAffineTransformHasScale(const MCGAffineTransform &p_transform)
{
	return p_transform.a != 1.0 || p_transform.d != 1.0;
}

static inline bool MCGAffineTransformHasSkew(const MCGAffineTransform &p_transform)
{
	return p_transform.b != 0.0 || p_transform.c != 0.0;
}

static inline bool MCGAffineTransformHasTranslationOnly(const MCGAffineTransform &p_transform)
{
	return !(MCGAffineTransformHasScale(p_transform) || MCGAffineTransformHasSkew(p_transform));
}

//////////

static inline bool MCGAffineTransformIsEqual(const MCGAffineTransform &p_left, const MCGAffineTransform &p_right)
{
	return p_left.a == p_right.a && p_left.b == p_right.b && p_left.c == p_right.c && p_left.d == p_right.d && p_left.tx == p_right.tx && p_left.ty == p_right.ty;
}

static inline bool MCGAffineTransformIsRectangular(const MCGAffineTransform &p_transform)
{
	return !MCGAffineTransformHasSkew(p_transform);
}

static inline bool MCGAffineTransformIsIdentity(const MCGAffineTransform &p_transform)
{
	return p_transform.a == 1.0 && p_transform.b == 0.0 && p_transform.c == 0.0 && p_transform.d == 1.0 && p_transform.tx == 0.0 && p_transform.ty == 0.0;
}

////////////////////////////////////////////////////////////////////////////////

MCGIntegerRectangle MCGIntegerRectangleGetTransformedBounds(const MCGIntegerRectangle &p_rect, const MCGAffineTransform &p_transform);

////////////////////////////////////////////////////////////////////////////////

// Regions

extern bool MCGRegionCreate(MCGRegionRef &r_region);
extern void MCGRegionDestroy(MCGRegionRef p_region);

extern bool MCGRegionIsEmpty(MCGRegionRef p_region);
extern bool MCGRegionIsRect(MCGRegionRef p_region);
extern bool MCGRegionIsComplex(MCGRegionRef p_region);

extern MCGIntegerRectangle MCGRegionGetBounds(MCGRegionRef p_region);

extern bool MCGRegionSetEmpty(MCGRegionRef p_region);
extern bool MCGRegionSetRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);
extern bool MCGRegionSetRegion(MCGRegionRef p_region, MCGRegionRef p_other);

extern bool MCGRegionIntersectsRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);
extern bool MCGRegionContainsRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);

extern bool MCGRegionIntersectsRegion(MCGRegionRef p_region, MCGRegionRef p_other);
extern bool MCGRegionContainsRegion(MCGRegionRef p_region, MCGRegionRef p_other);

extern bool MCGRegionAddRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);
extern bool MCGRegionAddRegion(MCGRegionRef p_region, MCGRegionRef p_other);

extern bool MCGRegionIntersectRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);
extern bool MCGRegionIntersectRegion(MCGRegionRef p_region, MCGRegionRef p_other);

extern bool MCGRegionTranslate(MCGRegionRef p_region, int32_t p_dx, int32_t p_dy);

typedef bool (*MCGRegionIterateCallback)(void *p_context, const MCGIntegerRectangle &p_rect);
extern bool MCGRegionIterate(MCGRegionRef p_region, MCGRegionIterateCallback p_callback, void *p_context);

extern bool MCGRegionCopy(MCGRegionRef p_region, MCGRegionRef &r_copy);
extern bool MCGRegionCopyWithTransform(MCGRegionRef p_region, const MCGAffineTransform &p_transform, MCGRegionRef &r_copy);

////////////////////////////////////////////////////////////////////////////////

#endif
