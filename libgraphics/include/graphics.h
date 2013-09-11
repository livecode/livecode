#ifndef __MC_GRAPHICS__
#define __MC_GRAPHICS__

#include "core.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCGContext *MCGContextRef;
typedef struct __MCGPath *MCGPathRef;
typedef struct __MCGImage *MCGImageRef;
typedef struct __MCGMask *MCGMaskRef;

typedef struct __MCGDashes *MCGDashesRef;

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

// IM_2013-08-21: [[ RefactorGraphics ]] set iOS pixel format to RGBA
#if defined(ANDROID) || defined(TARGET_SUBPLATFORM_IPHONE)
#define kMCGPixelFormatNative kMCGPixelFormatRGBA
#else
#define kMCGPixelFormatNative kMCGPixelFormatBGRA
#endif

static inline uint32_t __MCGPixelPackComponents(uint8_t p_1, uint8_t p_2, uint8_t p_3, uint8_t p_4)
{
	return p_1 | (p_2 << 8) | (p_3 << 16) | (p_4 << 24);
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
	}
}

static inline uint32_t MCGPixelPackNative(uint8_t p_red, uint8_t p_green, uint8_t p_blue, uint8_t p_alpha)
{
	return MCGPixelPack(kMCGPixelFormatNative, p_red, p_green, p_blue, p_alpha);
}

static inline void __MCGPixelUnpackComponents(uint32_t p_pixel, uint8_t &r_1, uint8_t &r_2, uint8_t &r_3, uint8_t &r_4)
{
	r_1 = p_pixel & 0xFF;
	r_2 = (p_pixel >> 8) & 0xFF;
	r_3 = (p_pixel >> 16) & 0xFF;
	r_4 = (p_pixel >> 24) & 0xFF;
}

static inline void MCGPixelUnpack(MCGPixelFormat p_format, uint32_t p_pixel, uint8_t &r_red, uint8_t &r_green, uint8_t &r_blue, uint8_t &r_alpha)
{
	switch (p_format)
	{
		case kMCGPixelFormatRGBA:
			__MCGPixelUnpackComponents(p_pixel, r_red, r_green, r_blue, r_alpha);
			break;
			
		case kMCGPixelFormatBGRA:
			__MCGPixelUnpackComponents(p_pixel, r_blue, r_green, r_red, r_alpha);
			break;
			
		case kMCGPixelFormatABGR:
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_blue, r_green, r_red);
			break;
			
		case kMCGPixelFormatARGB:
			__MCGPixelUnpackComponents(p_pixel, r_alpha, r_red, r_green, r_blue);
			break;
	}
}

static inline void MCGPixelUnpackNative(uint32_t p_pixel, uint8_t &r_red, uint8_t &r_green, uint8_t &r_blue, uint8_t &r_alpha)
{
	return MCGPixelUnpack(kMCGPixelFormatNative, p_pixel, r_red, r_green, r_blue, r_alpha);
}

////////////////////////////////////////////////////////////////////////////////

typedef float MCGFloat;
typedef uint32_t MCGColor;

struct MCGPoint
{
	MCGFloat x, y;
};

struct MCGSize
{
	MCGFloat width, height;
};

struct MCGRectangle
{
	MCGPoint origin;
	MCGSize size;
};

struct MCGAffineTransform
{
	MCGFloat a, b, c, d;
	MCGFloat tx, ty;
};

////////////////////////////////////////////////////////////////////////////////

enum MCGFillRule
{
	kMCGFillRuleNonZero,
	kMCGFillRuleEvenOdd,
};

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
	
	kMCGBlendModeCount,
};

enum MCGJoinStyle
{
	kMCGJoinStyleBevel,
	kMCGJoinStyleRound,
	kMCGJoinStyleMiter,
};

enum MCGCapStyle
{
	kMCGCapStyleButt,
	kMCGCapStyleRound,
	kMCGCapStyleSquare
};

enum MCGRasterFormat
{
	kMCGRasterFormat_xRGB, // no alpha channel
	kMCGRasterFormat_ARGB, // premultiplied alpha channel
	kMCGRasterFormat_U_ARGB, // Unpremultipled alpha channel
	kMCGRasterFormat_A, // alpha mask
};

enum MCGImageFilter
{
	kMCGImageFilterNearest,
	kMCGImageFilterBilinear,
	kMCGImageFilterBicubic,
};

enum MCGGradientFunction
{
	kMCGGradientFunctionLinear,
	kMCGGradientFunctionRadial,
	kMCGGradientFunctionConical,
	kMCGGradientFunctionSweep,
};

enum MCGGradientTileMode
{
	kMCGGradientTileModeClamp,
	kMCGGradientTileModeRepeat,
	kMCGGradientTileModeMirror,	
};

enum MCGMaskFormat
{
	kMCGMaskFormat_A1,
	kMCGMaskFormat_A8,
	kMCGMaskFormat_LCD32,
};

struct MCGRaster
{
	MCGRasterFormat format;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	void *pixels;
};

struct MCGStrokeAttr
{
	MCGFloat width;
	MCGJoinStyle join_style;
	MCGCapStyle cap_style;
	MCGFloat miter_limit;
	MCGDashesRef dashes;
};

struct MCGLayerEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
};

struct MCGShadowEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
	MCGFloat size;
	MCGFloat spread;
	MCGFloat x_offset;
	MCGFloat y_offset;
	bool knockout : 1;
};

struct MCGGlowEffect
{
	MCGColor color;
	MCGBlendMode blend_mode;
	MCGFloat size;
	MCGFloat spread;
	bool inverted : 1;
};

struct MCGBitmapEffects
{
	bool has_color_overlay : 1;
	bool has_inner_glow : 1;
	bool has_inner_shadow : 1;
	bool has_outer_glow : 1;
	bool has_drop_shadow : 1;
	
	MCGLayerEffect color_overlay;
	MCGGlowEffect inner_glow;
	MCGShadowEffect inner_shadow;
	MCGGlowEffect outer_glow;
	MCGShadowEffect drop_shadow;
};

struct MCGDeviceMaskInfo
{
	MCGMaskFormat format;
	int32_t x, y, width, height;
	void *data;
};

struct MCGFont
{
	void		*fid;
	uint16_t	size;
	int32_t		ascent;
	int32_t		descent;
	uint8_t		style;
};

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

inline MCGRectangle MCGRectangleScale(MCGRectangle p_rect, MCGFloat p_scale)
{
	MCGRectangle t_rect;
	t_rect.origin.x = p_rect.origin.x * p_scale;
	t_rect.origin.y = p_rect.origin.y * p_scale;
	t_rect.size.width = p_rect.size.width * p_scale;
	t_rect.size.height = p_rect.size.height * p_scale;
	
	return t_rect;
}

MCGRectangle MCGRectangleIntersection(const MCGRectangle &rect_1, const MCGRectangle &rect_2);

inline MCGPoint MCGPointMake(MCGFloat p_x, MCGFloat p_y)
{
	MCGPoint t_point;
	t_point . x = p_x;
	t_point . y = p_y;
	return t_point;
}

inline MCGSize MCGSizeMake(MCGFloat p_w, MCGFloat p_h)
{
	MCGSize t_size;
	t_size . width = p_w;
	t_size . height = p_h;
	return t_size;
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphicsInitialize(void);
void MCGraphicsFinalize(void);
void MCGraphicsCompact(void);

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

MCGImageRef MCGImageRetain(MCGImageRef image);
void MCGImageRelease(MCGImageRef image);

bool MCGImageIsValid(MCGImageRef image);

int32_t MCGImageGetWidth(MCGImageRef image);
int32_t MCGImageGetHeight(MCGImageRef image);

MCGSize MCImageGetSize(MCGImageRef image);

////////////////////////////////////////////////////////////////////////////////

bool MCGMaskCreateWithInfoAndRelease(const MCGDeviceMaskInfo& info, MCGMaskRef& r_mask);
void MCGMaskRelease(MCGMaskRef mask);
MCGRectangle MCGMaskGetBounds(MCGMaskRef mask);

////////////////////////////////////////////////////////////////////////////////

typedef uint8_t MCGPathCommand;
enum
{
	kMCGPathCommandEnd,
	kMCGPathCommandMoveTo,
	kMCGPathCommandLineTo,
	kMCGPathCommandCurveTo,
	kMCGPathCommandQuadCurveTo,
	kMCGPathCommandCloseSubpath,
};

bool MCGPathCreate(const MCGPathCommand *commands, const MCGFloat *parameters, MCGPathRef& r_path);
bool MCGPathCreateMutable(MCGPathRef& r_path);

MCGPathRef MCGPathRetain(MCGPathRef path);
void MCGPathRelease(MCGPathRef path);

bool MCGPathIsValid(MCGPathRef path);

void MCGPathCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopy(MCGPathRef path, MCGPathRef& r_new_path);
void MCGPathMutableCopyAndRelease(MCGPathRef path, MCGPathRef& r_new_path);

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
void MCGPathCloseSubpath(MCGPathRef path);

void MCGPathThicken(MCGPathRef path, const MCGStrokeAttr& attr, MCGPathRef& r_thick_path);
void MCGPathFlatten(MCGPathRef path, MCGFloat flatness, MCGPathRef& r_flat_path);
void MCGPathSimplify(MCGPathRef path, MCGPathRef& r_simple_path);

////////////////////////////////////////////////////////////////////////////////

bool MCGContextCreate(uint32_t width, uint32_t height, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithPixels(uint32_t width, uint32_t height, uint32_t stride, void *pixels, bool alpha, MCGContextRef& r_context);
bool MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context);

MCGContextRef MCGContextRetain(MCGContextRef context);
void MCGContextRelease(MCGContextRef context);

// Returns whether the current context context is valid. If an error
// occurs when calling any method on a context context, it will become
// invalid and all further operations will be no-ops.
bool MCGContextIsValid(MCGContextRef context);

// Graphics state operations
void MCGContextSave(MCGContextRef context);
void MCGContextRestore(MCGContextRef context);

// General attributes
void MCGContextSetFlatness(MCGContextRef context, MCGFloat flatness);
void MCGContextSetShouldAntialias(MCGContextRef context, bool should_antialias);

// Layer attributes and manipulation - bitmap effect options would be added here also.
void MCGContextSetOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetBlendMode(MCGContextRef context, MCGBlendMode mode);

void MCGContextBegin(MCGContextRef context, bool p_need_layer);
void MCGContextBeginWithEffects(MCGContextRef context, MCGRectangle shape, const MCGBitmapEffects &effects);
void MCGContextEnd(MCGContextRef context);

void MCGContextClipToRect(MCGContextRef context, MCGRectangle rect);
void MCGContextSetClipToRect(MCGContextRef context, MCGRectangle rect);
MCGRectangle MCGContextGetClipBounds(MCGContextRef context);
MCGRectangle MCGContextGetDeviceClipBounds(MCGContextRef context);

// Fill attributes
void MCGContextSetFillRule(MCGContextRef context, MCGFillRule rule);
void MCGContextSetFillOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetFillRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetFillPattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetFillGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetFillPaintStyle(MCGContextRef context, MCGPaintStyle style);

// Stroke attributes
void MCGContextSetStrokeOpacity(MCGContextRef context, MCGFloat opacity);
void MCGContextSetStrokeRGBAColor(MCGContextRef context, MCGFloat red, MCGFloat green, MCGFloat blue, MCGFloat alpha);
void MCGContextSetStrokePattern(MCGContextRef context, MCGImageRef image, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeGradient(MCGContextRef context, MCGGradientFunction function, const MCGFloat* stops, const MCGColor* colors, uindex_t ramp_length, bool mirror, bool wrap, uint32_t repeats, MCGAffineTransform transform, MCGImageFilter filter);
void MCGContextSetStrokeWidth(MCGContextRef context, MCGFloat width);
void MCGContextSetStrokeMiterLimit(MCGContextRef context, MCGFloat limit);
void MCGContextSetStrokeJoinStyle(MCGContextRef context, MCGJoinStyle style);
void MCGContextSetStrokeCapStyle(MCGContextRef context, MCGCapStyle style);
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
// Replace the current path by one thickened using the current stroke attributes.
void MCGContextThicken(MCGContextRef context);
// Replace the current path by one entirely consisting of moveto, lineto and close commands.
void MCGContextFlatten(MCGContextRef context);
// Replace the current path by one consisting of no overlapping subpaths or self
// intersections. Interior is determined by current fill rule.
void MCGContextSimplify(MCGContextRef context);

void MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter);
void MCGContextDrawRectOfImage(MCGContextRef self, MCGImageRef p_image, MCGRectangle p_src, MCGRectangle p_dst, MCGImageFilter p_filter);
void MCGContextDrawDeviceMask(MCGContextRef context, MCGMaskRef mask, int32_t tx, int32_t ty);

bool MCGContextCopyImage(MCGContextRef context, MCGImageRef &r_image);

void MCGContextDrawText(MCGContextRef context, const char* text, uindex_t length, MCGPoint location, uint32_t font_size, void *typeface);
MCGFloat MCGContextMeasureText(MCGContextRef context, const char *text, uindex_t length, uint32_t font_size, void *typeface);
void MCGContextDrawPlatformText(MCGContextRef context, const unichar_t *text, uindex_t length, MCGPoint location, const MCGFont &font);
MCGFloat MCGContextMeasurePlatformText(MCGContextRef context, const unichar_t *text, uindex_t length, const MCGFont &p_font);

////////////////////////////////////////////////////////////////////////////////

// Transforms
MCGAffineTransform MCGAffineTransformMakeIdentity(void);
MCGAffineTransform MCGAffineTransformMakeRotation(MCGFloat p_angle);
MCGAffineTransform MCGAffineTransformMakeTranslation(MCGFloat p_xoffset, MCGFloat p_yoffset);
MCGAffineTransform MCGAffineTransformMakeScale(MCGFloat p_xscale, MCGFloat p_yscale);

MCGAffineTransform MCGAffineTransformConcat(const MCGAffineTransform& transform_1, const MCGAffineTransform& transform_2);
MCGAffineTransform MCGAffineTransformRotate(const MCGAffineTransform& transform, MCGFloat angle);
MCGAffineTransform MCGAffineTransformTranslate(const MCGAffineTransform& transform, MCGFloat xoffset, MCGFloat yoffset);
MCGAffineTransform MCGAffineTransformScale(const MCGAffineTransform& transform, MCGFloat xscale, MCGFloat yscale);
MCGAffineTransform MCGAffineTransformInvert(const MCGAffineTransform& transform);

MCGPoint MCGPointApplyAffineTransform(const MCGPoint& p_point, const MCGAffineTransform& p_transform);
MCGRectangle MCGRectangleApplyAffineTransform(const MCGRectangle& p_rect, const MCGAffineTransform& p_transform);
MCGSize MCGSizeApplyAffineTransform(const MCGSize& p_size, const MCGAffineTransform& p_transform);

////////////////////////////////////////////////////////////////////////////////

#endif
