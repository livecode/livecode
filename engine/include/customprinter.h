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

////////////////////////////////////////////////////////////////////////////////
//
//  Public Header File:
//    customprinter.h
//
//  Description:
//    This file contains the definiton of the custom printer interface and
//    related definitions.
//
//  Changes:
//    2009-11-25 MW Created.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_CUSTOM_PRINTER__
#define __MC_CUSTOM_PRINTER__

////////////////////////////////////////////////////////////////////////////////

struct MCCustomPrinterDocument
{
    MCCustomPrinterDocument()
    : title(nullptr), filename(nullptr),
        option_count(0),
        option_keys(nullptr), option_values(nullptr)
    {}

	const char *title;
	const char *filename;

	uint32_t option_count;
	char * const *option_keys;
	char * const *option_values;
};

struct MCCustomPrinterPage
{
	double width;
	double height;
	double scale;
};

////////////////////////////////////////////////////////////////////////////////

struct MCCustomPrinterPoint
{
	double x;
	double y;
};

struct MCCustomPrinterRectangle
{
	double left;
	double top;
	double right;
	double bottom;
};

// A standard affine transformation:
//   x' = scale_x * x + skew_x * y + translate_x;
//   y' = skew_y * x + scale_y * y + translate_y;
//
struct MCCustomPrinterTransform
{
	double scale_x, skew_x;
	double skew_y, scale_y;
	double translate_x, translate_y;
};

////////////////////////////////////////////////////////////////////////////////

struct MCCustomPrinterColor
{
	double red;
	double green;
	double blue;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterImageType
{
	// Null image type
	kMCCustomPrinterImageNone,

	// Raw 32-bit RGB image data, no mask
	kMCCustomPrinterImageRawXRGB,

	// Raw 32-bit RGB image data, sharp mask
	kMCCustomPrinterImageRawMRGB,

	// Raw 32-bit RGB image data, soft mask (unpremultiplied) 
	kMCCustomPrinterImageRawARGB,

	// GIF image data
	kMCCustomPrinterImageGIF,

	// JPEG image data
	kMCCustomPrinterImageJPEG,

	// PNG image data
	kMCCustomPrinterImagePNG
};

#ifdef __LITTLE_ENDIAN__
#define kMCCustomPrinterImagePixelFormat kMCGPixelFormatBGRA
#else
#define kMCCustomPrinterImagePixelFormat kMCGPixelFormatARGB
#endif

struct MCCustomPrinterImage
{
	MCCustomPrinterImageType type;
	uint32_t id;
	uint32_t width;
	uint32_t height;
	void *data;
	uint32_t data_size;
};

////////////////////////////////////////////////////////////////////////////////

struct MCCustomPrinterPattern
{
	MCCustomPrinterImage image;
	MCCustomPrinterTransform transform;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterGradientType
{
	kMCCustomPrinterGradientNone,
	kMCCustomPrinterGradientLinear,
	kMCCustomPrinterGradientRadial,
	kMCCustomPrinterGradientConical,
	kMCCustomPrinterGradientDiamond,
	kMCCustomPrinterGradientSpiral,
	kMCCustomPrinterGradientXY,
	kMCCustomPrinterGradientSqrtXY
};

struct MCCustomPrinterGradientStop
{
	MCCustomPrinterColor color;
	double alpha;
	double offset;
};

struct MCCustomPrinterGradient
{
	MCCustomPrinterGradientType type;
	bool mirror;
	bool wrap;
	uint32_t repeat;
	uint32_t stop_count;
	MCCustomPrinterGradientStop *stops;
	MCCustomPrinterTransform transform;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterPaintType
{
	kMCCustomPrinterPaintNone,
	kMCCustomPrinterPaintSolid,
	kMCCustomPrinterPaintPattern,
	kMCCustomPrinterPaintGradient
};

struct MCCustomPrinterPaint
{
	MCCustomPrinterPaintType type;
	union
	{
		MCCustomPrinterColor solid;
		MCCustomPrinterPattern pattern;
		MCCustomPrinterGradient gradient;
	};
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterCapStyle
{
	kMCCustomPrinterCapButt,
	kMCCustomPrinterCapRound,
	kMCCustomPrinterCapSquare
};

enum MCCustomPrinterJoinStyle
{
	kMCCustomPrinterJoinBevel,
	kMCCustomPrinterJoinRound,
	kMCCustomPrinterJoinMiter
};

struct MCCustomPrinterStroke
{
	double thickness;
	MCCustomPrinterCapStyle cap_style;
	MCCustomPrinterJoinStyle join_style;
	uint32_t dash_count;
	double dash_offset;
	double *dashes;
	double miter_limit;
	double aspect;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterBlendMode
{
	kMCCustomPrinterBlendSrc,
	kMCCustomPrinterBlendDst,
	kMCCustomPrinterBlendSrcOver,
	kMCCustomPrinterBlendDstOver,
	kMCCustomPrinterBlendSrcIn,
	kMCCustomPrinterBlendDstIn,
	kMCCustomPrinterBlendSrcOut,
	kMCCustomPrinterBlendDstOut,
	kMCCustomPrinterBlendSrcAtop,
	kMCCustomPrinterBlendDstAtop,
	kMCCustomPrinterBlendXor,
	kMCCustomPrinterBlendPlus,
	kMCCustomPrinterBlendMultiply,
	kMCCustomPrinterBlendScreen,
	kMCCustomPrinterBlendOverlay,
	kMCCustomPrinterBlendDarken,
	kMCCustomPrinterBlendLighten,
	kMCCustomPrinterBlendDodge,
	kMCCustomPrinterBlendBurn,
	kMCCustomPrinterBlendHardLight,
	kMCCustomPrinterBlendSoftLight,
	kMCCustomPrinterBlendDifference,
	kMCCustomPrinterBlendExclusion
};

struct MCCustomPrinterGroup
{
	MCCustomPrinterBlendMode blend_mode;
	double opacity;
	MCCustomPrinterRectangle region;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterFillRule
{
	kMCCustomPrinterFillNonZero,
	kMCCustomPrinterFillEvenOdd
};

enum MCCustomPrinterPathCommand
{
	kMCCustomPrinterPathEnd,
	kMCCustomPrinterPathClose,
	kMCCustomPrinterPathMoveTo,
	kMCCustomPrinterPathLineTo,
	kMCCustomPrinterPathCubicTo,
	kMCCustomPrinterPathQuadraticTo
};

struct MCCustomPrinterPath
{
	MCCustomPrinterPathCommand *commands;
	MCCustomPrinterPoint *coords;
};

////////////////////////////////////////////////////////////////////////////////

struct MCCustomPrinterFont
{
	double size;
	void *handle;
};

struct MCCustomPrinterGlyph
{
	uint32_t id;
	double x;
	double y;
};

////////////////////////////////////////////////////////////////////////////////

enum MCCustomPrinterLinkType
{
	kMCCustomPrinterLinkUnspecified,
	kMCCustomPrinterLinkAnchor,
	kMCCustomPrinterLinkURI,
};

////////////////////////////////////////////////////////////////////////////////

// The MCCustomPrintingDevice interface encapsulats an alternative output method for
// printing in Rev. If any methods return 'false' the print job is assumed
// to be in an inconsistent state and cancelled. When this occurs the implementation
// of the printing device should clean up after itself - removing any partially
// created output (if possible).
//
class MCCustomPrintingDevice
{
public:
	// Destroy the instance of the custom printer.
	virtual void Destroy(void) = 0;

	// Should return 'true' if the printer can natively draw using the given
	// paint. It is assumed that any printer can at least paint primitives with
	// a solid (non-alpha) color.
	// Note that only the paint.type, paint.gradient.type, paint.image.type and
	// paint.pattern.image.type fields will be set on entry to this call.
	virtual bool CanRenderPaint(const MCCustomPrinterPaint& paint) = 0;

	// Should return 'true' if the printer can natively draw the given image.
	// It is assumed that any printer can at least paint sharp-masked raw images.
	// Note that only the image.type field will be set on entry to this call.
	virtual bool CanRenderImage(const MCCustomPrinterImage& image) = 0;

	// Should return 'true' if the printer can natively draw a grouped
	// collection of objects with the given options. It is assumed that any
	// printer can at least paint a group with the SrcOver blend mode and
	// full opacity.
	// Note that only the group.blend and group.opacity will be set on entry
	// to this call.
	virtual bool CanRenderGroup(const MCCustomPrinterGroup& group) = 0;

	// Return the error string describing the reason for 'false' being returned
	// from one of the action methods. 'nil' should be returned if no error
	// has occurred.
	virtual const char *GetError(void) = 0;

	// Start a new document with the given details as present in the
	// passed structure.
	virtual bool BeginDocument(const MCCustomPrinterDocument& document) = 0;

	// Cancel the current document - this should have the same effect as if
	// an error occurred, but obviously no error should be set.
	virtual void AbortDocument(void) = 0;

	// Mark the end of a document.
	virtual bool EndDocument(void) = 0;

	// Start a new page with the given configuration.
	virtual bool BeginPage(const MCCustomPrinterPage& page) = 0;

	// End the current page.
	virtual bool EndPage(void) = 0;
	
	// Start a group with the given options. Note that this will only
	// ever be called for a group where CanRenderGroup() returns true.
	virtual bool BeginGroup(const MCCustomPrinterGroup& group) = 0;

	// End the current group.
	virtual bool EndGroup(void) = 0;

	// Fill the given path using the specified fill rule and paint. The
	// path should be transformed using the given matrix and then clipped
	// to the given rectangle. Note that paint is specified in path space,
	// before the transform is applied.
	virtual bool FillPath(const MCCustomPrinterPath& path, MCCustomPrinterFillRule rule, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip) = 0;

	// Stroke the given path using the specified stroke settings and paint.
	// The path should be stroked, then transformed and finally clipped. Note
	// that paint is specified in path space, before the transform is applied.
	virtual bool StrokePath(const MCCustomPrinterPath& path, const MCCustomPrinterStroke& stroke, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip) = 0;

	// Draw the given image with the specified transformation matrix and
	// clipped to the given rectangle.
	virtual bool DrawImage(const MCCustomPrinterImage& image, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip) = 0;

	// Draw text using the given system font face. The 'text' is the UTF-8
	// characters to which the given glyphs correspond. The 'font' is a
	// specific font face that exists on the current system, and in which
	// all the glyphs exist. The given glyphs should be transformed and then
	// clipped. Note that the paint is specified in text space, before
	// transformation. Additionally, note that there is not a 1-1
	// correspondence between glyph and character - the sequence of glyphs
	// should be considered to map to the text 'in one piece'. The clusters
	// array maps each byte of the text to the index of the first glyph in
	// its cluster.
	virtual bool DrawText(const MCCustomPrinterGlyph *glyphs, uint32_t glyph_count, const char *text_bytes, uint32_t text_byte_count, const uint32_t *clusters, const MCCustomPrinterFont& font, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip) = 0;

	// Make an anchor with the given name at the specified position - the name must
	// not have the form of a URI.
	virtual bool MakeAnchor(const MCCustomPrinterPoint& position, const char *name) = 0;

	// Make a link covering the given area. The link is interpreted as either the name
	// of an anchor, or a URI if it is of that form.
	virtual bool MakeLink(const MCCustomPrinterRectangle& area, const char *link, MCCustomPrinterLinkType type) = 0;

	// Make a bookmark with the given title, nested at the given depth.
	// If the device does not support bookmarks, this should be a no-op.
	virtual bool MakeBookmark(const MCCustomPrinterPoint& position, const char *title, uint32_t depth, bool closed) = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif
