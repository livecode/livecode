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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "osspec.h"

#include "uidc.h"
#include "mcerror.h"
#include "globals.h"

#include "exec.h"
#include "metacontext.h"
#include "printer.h"
#include "customprinter.h"
#include "license.h"
#include "path.h"
#include "pathprivate.h"
#include "gradient.h"
#include "textlayout.h"
#include "region.h"

#include "graphicscontext.h"
#include "font.h"

#include "graphics_util.h"

#ifdef _LINUX_DESKTOP
#include "flst.h"
#endif

////////////////////////////////////////////////////////////////////////////////

static bool MCCustomPrinterBlendModeFromInk(uint1 p_function, MCCustomPrinterBlendMode& r_mode)
{
	switch(p_function)
	{
	case GXcopy: r_mode = kMCCustomPrinterBlendSrcOver; break;
	case GXblendSrc: r_mode = kMCCustomPrinterBlendSrc; break;
	case GXblendDst: r_mode = kMCCustomPrinterBlendDst; break;
	case GXblendSrcOver: r_mode = kMCCustomPrinterBlendSrcOver; break;
	case GXblendDstOver: r_mode = kMCCustomPrinterBlendDstOver; break;
	case GXblendSrcIn: r_mode = kMCCustomPrinterBlendSrcIn; break;
	case GXblendDstIn: r_mode = kMCCustomPrinterBlendDstIn; break;
	case GXblendSrcOut: r_mode = kMCCustomPrinterBlendSrcOut; break;
	case GXblendDstOut: r_mode = kMCCustomPrinterBlendDstOut; break;
	case GXblendSrcAtop: r_mode = kMCCustomPrinterBlendSrcAtop; break;
	case GXblendDstAtop: r_mode = kMCCustomPrinterBlendDstAtop; break;
	case GXblendXor: r_mode = kMCCustomPrinterBlendXor; break;
	case GXblendPlus: r_mode = kMCCustomPrinterBlendPlus; break;
	case GXblendMultiply: r_mode = kMCCustomPrinterBlendMultiply; break;
	case GXblendScreen: r_mode = kMCCustomPrinterBlendScreen; break;
	case GXblendOverlay: r_mode = kMCCustomPrinterBlendOverlay; break;
	case GXblendDarken: r_mode = kMCCustomPrinterBlendDarken; break;
	case GXblendLighten: r_mode = kMCCustomPrinterBlendLighten; break;
	case GXblendDodge: r_mode = kMCCustomPrinterBlendDodge; break;
	case GXblendBurn: r_mode = kMCCustomPrinterBlendBurn; break;
	case GXblendHardLight: r_mode = kMCCustomPrinterBlendHardLight; break;
	case GXblendSoftLight: r_mode = kMCCustomPrinterBlendSoftLight; break;
	case GXblendDifference: r_mode = kMCCustomPrinterBlendDifference; break;
	case GXblendExclusion: r_mode = kMCCustomPrinterBlendExclusion; break;
	default:
		return false;
	}
	return true;
}

static bool MCCustomPrinterGradientFromMCGradient(MCGradientFillKind p_kind, MCCustomPrinterGradientType& r_type)
{
	switch(p_kind)
	{
	case kMCGradientKindLinear: r_type = kMCCustomPrinterGradientLinear; break;
	case kMCGradientKindRadial: r_type = kMCCustomPrinterGradientRadial; break;
	case kMCGradientKindConical: r_type = kMCCustomPrinterGradientConical; break;
	case kMCGradientKindDiamond: r_type = kMCCustomPrinterGradientDiamond; break;
	case kMCGradientKindSpiral: r_type = kMCCustomPrinterGradientSpiral; break;
	case kMCGradientKindXY: r_type = kMCCustomPrinterGradientXY; break;
	case kMCGradientKindSqrtXY: r_type = kMCCustomPrinterGradientSqrtXY; break;
	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCCustomPrinterTransform MCCustomPrinterTransformFromMCGAffineTransform(const MCGAffineTransform &p_transform)
{
	MCCustomPrinterTransform t_transform;
	t_transform.scale_x = p_transform.a;
	t_transform.skew_x = p_transform.c;
	t_transform.translate_x = p_transform.tx;
	t_transform.skew_y = p_transform.b;
	t_transform.scale_y = p_transform.d;
	t_transform.translate_y = p_transform.ty;

	return t_transform;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCustomPrinterImageFromMCGImage(MCGImageRef p_image, MCCustomPrinterImage &r_image, void *&r_pixel_cache)
{
	MCGRaster t_raster;
	if (!MCGImageGetRaster(p_image, t_raster))
		return false;
	
	void *t_pixels;
	t_pixels = nil;
	
	void *t_pixel_cache;
	t_pixel_cache = nil;

#if kMCCustomPrinterImagePixelFormat == kMCGPixelFormatNative
    t_pixels = t_raster.pixels;
#else
    if (!MCMemoryAllocate(t_raster.stride * t_raster.height, t_pixel_cache))
        return false;
    
    uint8_t *t_src;
    t_src = (uint8_t*)t_raster.pixels;
    
    uint8_t *t_dst;
    t_dst = (uint8_t*)t_pixel_cache;
    
    for (uint32_t i = 0; i < t_raster.height; i++)
    {
        uint32_t *t_src_row;
        t_src_row = (uint32_t*)t_src;
        
        uint32_t *t_dst_row;
        t_dst_row = (uint32_t*)t_dst;
        
		for (uint32_t j = 0; j < t_raster.width; j++)
			*t_dst_row++ = MCGPixelFromNative(kMCCustomPrinterImagePixelFormat, *t_src_row++);
		
        t_src += t_raster.stride;
        t_dst += t_raster.stride;
    }
    
    t_pixels = t_pixel_cache;
#endif

	// Fill in the printer image info
	r_image.width = t_raster.width;
	r_image.height = t_raster.height;
	
	bool t_mask, t_alpha;
	t_mask = !MCGImageIsOpaque(p_image);
	t_alpha = t_mask && MCGImageHasPartialTransparency(p_image);
	// IM-2014-06-26: [[ Bug 12699 ]] Set image type appropriately.
	r_image . type = t_alpha ? kMCCustomPrinterImageRawARGB : (t_mask ? kMCCustomPrinterImageRawMRGB : kMCCustomPrinterImageRawXRGB);
	r_image . id = (uint32_t)(intptr_t)p_image;
	r_image . data = t_pixels;
	r_image . data_size = t_raster.stride * t_raster.height;
	
	r_pixel_cache = t_pixel_cache;
	
	return true;
}

class MCCustomMetaContext: public MCMetaContext
{
public:
	MCCustomMetaContext(const MCRectangle& page);
	~MCCustomMetaContext(void);

	bool render(MCCustomPrintingDevice *p_device, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, const MCPrinterRectangle& p_page_rect);

protected:
	bool candomark(MCMark *mark);
	void domark(MCMark *mark);
	bool begincomposite(const MCRectangle &region, MCGContextRef &r_context);
	MCContext *begincomposite(const MCRectangle& region);
	void endcomposite(MCRegionRef clip_region);

	void dopathmark(MCMark *mark, MCPath *path);
	void dorawpathmark(MCMark *mark, uint1 *commands, uint32_t command_count, int4 *ordinates, uint32_t ordinate_count, bool p_evenodd);
	void dotextmark(MCMark *mark);
	void doimagemark(MCMark *mark);
	void dolinkmark(MCMark *mark);

private:
	void transform_point(const MCPoint& in_point, MCCustomPrinterPoint& r_out_point);
	void transform_rect(const MCRectangle& in_rect, MCCustomPrinterRectangle& r_out_rect);

	void compute_clip(const MCRectangle& clip_rect, MCCustomPrinterRectangle& r_out_rect);

	// The device we are targetting (only valid during the 'render' method).
	MCCustomPrintingDevice *m_device;

	// These are the transform factors to map the src to dst rect
	double m_scale_x, m_scale_y;
	double m_translate_x, m_translate_y;

	// This is the rectangle that defines the maximum area in device space that
	// can be used.
	MCPrinterRectangle m_page_rect;

	// If this is true, an error has occurred during execute
	bool m_execute_error;

	// The untransformed rect and scale of current composite region
	MCRectangle m_composite_rect;
	uint32_t m_composite_scale;

	// The compositing graphics context used to render into
	MCGContextRef m_composite_context;
};

MCCustomMetaContext::MCCustomMetaContext(const MCRectangle& p_page)
	: MCMetaContext(p_page)
{
	m_composite_context = nil;
}

MCCustomMetaContext::~MCCustomMetaContext(void)
{
	MCGContextRelease(m_composite_context);
}

bool MCCustomMetaContext::render(MCCustomPrintingDevice *p_device, const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, const MCPrinterRectangle& p_page_rect)
{
	m_execute_error = false;
	m_device = p_device;
	m_scale_x = (p_dst_rect . right - p_dst_rect . left) / (p_src_rect . right - p_src_rect . left);
	m_scale_y = (p_dst_rect . bottom - p_dst_rect . top) / (p_src_rect . bottom - p_src_rect . top);
	m_translate_x = -p_src_rect . left * m_scale_x + p_dst_rect . left;
	m_translate_y = -p_src_rect . top * m_scale_y + p_dst_rect . top;

	m_page_rect = p_page_rect;

	// Run the display list
	execute();

	return !m_execute_error;
}

bool MCCustomMetaContext::candomark(MCMark *p_mark)
{
	// If an error has occurred during this execution, just return true to minimize
	// unnecessary rasterization (this is to make up for a lack of error handling
	// during the super-classes execution process).
	if (m_execute_error)
		return true;

	// This method is called for every mark to see if it needs rasterization.
	switch(p_mark -> type)
	{
	case MARK_TYPE_LINE:
	case MARK_TYPE_POLYGON:
	case MARK_TYPE_TEXT:
	case MARK_TYPE_RECTANGLE:
	case MARK_TYPE_ROUND_RECTANGLE:
	case MARK_TYPE_ARC:
	case MARK_TYPE_PATH:
		{
			MCCustomPrinterPaint t_paint;

			// Check to see if its a gradient
			if (p_mark -> fill -> gradient != nil)
			{
				if (!MCCustomPrinterGradientFromMCGradient((MCGradientFillKind)p_mark -> fill -> gradient -> kind, t_paint . gradient . type))
					return false;

				t_paint . type = kMCCustomPrinterPaintGradient;

				return m_device -> CanRenderPaint(t_paint);
			}

			// If its a solid color we are done
			if (p_mark -> fill -> style == FillSolid)
				return true;

			// Check to see if its a pattern
			if (p_mark -> fill -> style == FillTiled)
			{
				t_paint . type = kMCCustomPrinterPaintPattern;
				t_paint . pattern . image . type = kMCCustomPrinterImageRawARGB;
				return m_device -> CanRenderPaint(t_paint);
			}

			// Otherwise it is FillStippled or FillOpaqueStippled - neither of which
			// we support for this kind of printing (yet?)
			return true;
		}
	case MARK_TYPE_IMAGE:
		{
			// Devices have to support unmasked images (otherwise we couldn't
			// rasterize!).
			bool t_mask, t_alpha;
			t_mask = !MCGImageIsOpaque(p_mark->image.descriptor.image);
			t_alpha = t_mask && MCGImageHasPartialTransparency(p_mark->image.descriptor.image);

			if (!t_mask)
				return true;

			// Now check to see if the appropriate type of masked (raw) image
			// is supported.
			MCCustomPrinterImage t_image;
			if (t_alpha)
				t_image . type = kMCCustomPrinterImageRawARGB;
			else
				t_image . type = kMCCustomPrinterImageRawMRGB;

			return m_device -> CanRenderImage(t_image);
		}
	case MARK_TYPE_METAFILE:
	case MARK_TYPE_EPS:
	case MARK_TYPE_THEME:
		// These all have to be rasterized!
		return false;
	case MARK_TYPE_GROUP:
		{
			// If the group has effects we can't do it (yet!)
			if (p_mark -> group . effects != nil)
				return false;

			// If the blend mode isn't a custom printer supported one, we can't
			// do it.
			MCCustomPrinterBlendMode t_blend_mode;
			if (!MCCustomPrinterBlendModeFromInk(p_mark -> group . function, t_blend_mode))
				return false;

			// Otherwise fill in the custom group struct and ask our device
			MCCustomPrinterGroup t_group;
			t_group . blend_mode = t_blend_mode;
			t_group . opacity = p_mark -> group . opacity / 255.0;
			return m_device -> CanRenderGroup(t_group);
		}
	case MARK_TYPE_LINK:
		// We can always render links natively - even if this is a no-op.
		return true;
	case MARK_TYPE_END:
		// Unknown mark so return false.
		return false;
	}

	MCUnreachableReturn(false);
}

void MCCustomMetaContext::domark(MCMark *p_mark)
{
	// If an error has occurred, we do nothing.
	if (m_execute_error)
		return;

	switch(p_mark -> type)
	{
	case MARK_TYPE_LINE:
		{
			MCPath *t_path;
			t_path = MCPath::create_line(p_mark -> line . start . x, p_mark -> line . start . y, p_mark -> line . end . x, p_mark -> line . end . y, true);
			if (t_path != nil)
			{
				dopathmark(p_mark, t_path);
				t_path -> release();
			}
			else
				m_execute_error = true;
		}
		break;
	case MARK_TYPE_POLYGON:
		{
			MCPath *t_path;
			if (p_mark -> polygon . closed)
				t_path = MCPath::create_polygon(p_mark -> polygon . vertices, p_mark -> polygon . count, true);
			else
				t_path = MCPath::create_polyline(p_mark -> polygon . vertices, p_mark -> polygon . count, true);
			if (t_path != nil)
			{
				dopathmark(p_mark, t_path);
				t_path -> release();
			}
			else
				m_execute_error = true;
		}
		break;
	case MARK_TYPE_TEXT:
		dotextmark(p_mark);
		break;
	case MARK_TYPE_RECTANGLE:
		{
            // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since MCPath only accepts ints, if the inset value is uneven,
            // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
            // SN-2014-10-17: [[ Bug 13351 ]] Only round up existing inset
            if (p_mark -> rectangle . inset && !(p_mark -> rectangle . inset % 2))
				p_mark -> rectangle . inset ++;
            // SN-2014-10-17: [[ Bug 13351 ]] Be careful not to underflow the bounds
			p_mark -> rectangle . bounds = MCRectangleMake(p_mark -> rectangle . bounds . x + p_mark -> rectangle . inset / 2,
														   p_mark -> rectangle . bounds . y + p_mark -> rectangle . inset / 2, 
                                                           MCMin(p_mark -> rectangle . bounds . width, p_mark -> rectangle . bounds . width - p_mark -> rectangle . inset),
                                                           MCMin(p_mark -> rectangle . bounds . height, p_mark -> rectangle . bounds . height - p_mark -> rectangle . inset));
			
			MCPath *t_path;
			if (p_mark -> stroke != nil && p_mark -> rectangle . bounds . height == 1)
				t_path = MCPath::create_line(p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y, p_mark -> rectangle . bounds . x + p_mark -> rectangle . bounds . width - 1, p_mark -> rectangle . bounds . y, true);
			else if (p_mark -> stroke != nil && p_mark -> rectangle . bounds . width == 1)
				t_path = MCPath::create_line(p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y, p_mark -> rectangle . bounds . x, p_mark -> rectangle . bounds . y + p_mark -> rectangle . bounds . height - 1, true);
			else
				t_path = MCPath::create_rectangle(p_mark -> rectangle . bounds, p_mark -> stroke != nil);
			if (t_path != nil)
			{
				dopathmark(p_mark, t_path);
				t_path -> release();
			}
			else
				m_execute_error = true;
		}
		break;
	case MARK_TYPE_ROUND_RECTANGLE:
		{
            // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since MCPath only accepts ints, if the inset value is uneven,
            // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
            // SN-2014-10-17: [[ Bug 13351 ]] Only round up existing inset
			if (!(p_mark -> round_rectangle . inset % 2))
				p_mark -> round_rectangle . inset ++;
            // SN-2014-10-17: [[ Bug 13351 ]] Be careful not to underflow the bounds
			p_mark -> round_rectangle . bounds = MCRectangleMake(p_mark -> round_rectangle . bounds . x + p_mark -> round_rectangle . inset / 2,
														   p_mark -> round_rectangle . bounds . y + p_mark -> round_rectangle . inset / 2, 
                                                           MCMin(p_mark -> round_rectangle . bounds . width, p_mark -> round_rectangle . bounds . width - p_mark -> round_rectangle . inset),
                                                           MCMin(p_mark -> round_rectangle . bounds . height, p_mark -> round_rectangle . bounds . height - p_mark -> round_rectangle . inset));
			
			MCPath *t_path;
			t_path = MCPath::create_rounded_rectangle(p_mark -> round_rectangle . bounds, p_mark -> round_rectangle . radius / 2, p_mark -> stroke != nil);
			if (t_path != nil)
			{
				dopathmark(p_mark, t_path);
				t_path -> release();
			}
			else
				m_execute_error = true;
		}
		break;
	case MARK_TYPE_ARC:
		{
            // MM-2014-04-23: [[ Bug 11884 ]] Inset the bounds. Since MCPath only accepts ints, if the inset value is uneven,
            // round up to the nearest even value, keeping behaviour as close to that of the graphics context as possible.
            // SN-2014-10-17: [[ Bug 13351 ]] Only round up existing inset
			if (!(p_mark -> arc . inset % 2))
				p_mark -> arc . inset ++;
            // SN-2014-10-17: [[ Bug 13351 ]] Be careful not to underflow the bounds
			p_mark -> arc . bounds = MCRectangleMake(p_mark -> arc . bounds . x + p_mark -> arc . inset / 2,
														   p_mark -> arc . bounds . y + p_mark -> arc . inset / 2, 
                                                           MCMin(p_mark -> arc . bounds . width, p_mark -> arc . bounds . width - p_mark -> arc . inset),
                                                           MCMin(p_mark -> arc . bounds . height, p_mark -> arc . bounds . height - p_mark -> arc . inset));
			
			MCPath *t_path;
			if (p_mark -> arc . complete)
				t_path = MCPath::create_segment(p_mark -> arc . bounds, p_mark -> arc . start, p_mark -> arc . angle, p_mark -> stroke != nil);
			else
				t_path = MCPath::create_arc(p_mark -> arc . bounds, p_mark -> arc . start, p_mark -> arc . angle, p_mark -> stroke != nil);
			if (t_path != nil)
			{
				dopathmark(p_mark, t_path);
				t_path -> release();
			}
			else
				m_execute_error = true;
		}
		break;
	case MARK_TYPE_PATH:
		dorawpathmark(p_mark, p_mark -> path . commands, p_mark -> path . command_count, p_mark -> path . ordinates, p_mark -> path . ordinate_count, p_mark -> path . evenodd);
		break;

	case MARK_TYPE_IMAGE:
		doimagemark(p_mark);
		break;

	case MARK_TYPE_GROUP:
		{
			// Work out the custom group properties
			MCCustomPrinterGroup t_group;
			MCCustomPrinterBlendModeFromInk(p_mark -> group . function, t_group . blend_mode);
			t_group . opacity = p_mark -> group . opacity / 255.0;
			compute_clip(p_mark -> clip, t_group . region);

			// If the opacity if not 1, or the blend mode is not srcOver then we
			// generate a group. Otherwise, there's no need.
			if (t_group . opacity != 1.0 || t_group . blend_mode != kMCCustomPrinterBlendSrcOver)
			{
				if (!m_execute_error &&
					!m_device -> BeginGroup(t_group))
					m_execute_error = true;

				if (!m_execute_error)
					executegroup(p_mark);

				if (!m_execute_error &&
					!m_device -> EndGroup())
					m_execute_error = true;
			}
			else
				executegroup(p_mark);
		}
		break;

	case MARK_TYPE_LINK:
		dolinkmark(p_mark);
		break;

	default:
		break;
	}
}

void MCCustomMetaContext::doimagemark(MCMark *p_mark)
{
	// See if we can render the image using the original text
	MCCustomPrinterImageType t_image_type;
	t_image_type = kMCCustomPrinterImageNone;
	if (p_mark -> image . descriptor . data_type != kMCImageDataNone)
	{
		switch(p_mark -> image . descriptor . data_type)
		{
		case kMCImageDataGIF: t_image_type = kMCCustomPrinterImageGIF; break;
		case kMCImageDataPNG: t_image_type = kMCCustomPrinterImagePNG; break;
		case kMCImageDataJPEG: t_image_type = kMCCustomPrinterImageJPEG; break;
		default: assert(false);
		}

		MCCustomPrinterImage t_image;
		t_image . type = t_image_type;
		if (!m_device -> CanRenderImage(t_image))
			t_image_type = kMCCustomPrinterImageNone;
	}

	void *t_pixel_cache;
	t_pixel_cache = nil;
	
	// Fill in the printer image info
	MCCustomPrinterImage t_image;
	if (!m_execute_error)
	{
		if (t_image_type == kMCCustomPrinterImageNone)
		{
			if (!MCCustomPrinterImageFromMCGImage(p_mark -> image . descriptor . image, t_image, t_pixel_cache))
				m_execute_error = true;
		}
		else
		{
			t_image . type = t_image_type;
			t_image . id = (uint32_t)(intptr_t)p_mark -> image . descriptor . data_bits;
			t_image . data = p_mark -> image . descriptor . data_bits;
			t_image . data_size = p_mark -> image . descriptor . data_size;
			t_image . width = MCGImageGetWidth(p_mark -> image . descriptor . image);
			t_image . height = MCGImageGetHeight(p_mark -> image . descriptor . image);
		}
	}
	
	if (!m_execute_error)
	{
		// Compute the transform that is needed - this transform goes from image
		// space to page space.
		// IM-2014-06-26: [[ Bug 12699 ]] Rework to ensure transforms are applied in the correct order - page transform -> image offset -> image transform
		MCGAffineTransform t_transform;
		t_transform = MCGAffineTransformMake(m_scale_x, 0, 0, m_scale_y, m_translate_x, m_translate_y);
		t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeTranslation(p_mark -> image . dx - p_mark -> image . sx, p_mark -> image . dy - p_mark -> image . sy));
		if (p_mark -> image . descriptor . has_transform)
			t_transform = MCGAffineTransformConcat(t_transform, p_mark -> image . descriptor . transform);

		// Compute the clip that is needed - the mark alreay has the appropriate clip set.
		MCCustomPrinterRectangle t_clip;
		compute_clip(p_mark -> clip, t_clip);

		// Render the primitive
		if (!m_device -> DrawImage(t_image, MCCustomPrinterTransformFromMCGAffineTransform(t_transform), t_clip))
			m_execute_error = true;
	}
	
	if (t_pixel_cache != nil)
		MCMemoryDeallocate(t_pixel_cache);
}

void MCCustomMetaContext::dolinkmark(MCMark *p_mark)
{
	MCCustomPrinterRectangle t_region;
	compute_clip(p_mark -> link . region, t_region);
	if (!m_device -> MakeLink(t_region, p_mark -> link . text, kMCCustomPrinterLinkUnspecified))
		m_execute_error = true;
}

#define SCALE 4

bool MCCustomMetaContext::begincomposite(const MCRectangle &p_mark_clip, MCGContextRef &r_context)
{
	bool t_success = true;

	MCGContextRef t_gcontext = nil;

	// TODO: Make the rasterization scale depend in some way on current scaling,
	//   after all there's no point in making a large rasterized bitmap if it ends
	//   up being printed really really small!
	uint4 t_scale;
	t_scale = SCALE;

	uint32_t t_width = p_mark_clip.width * t_scale;
	uint32_t t_height = p_mark_clip.height * t_scale;

	if (t_success)
		t_success = MCGContextCreate(t_width, t_height, true, t_gcontext);

	if (t_success)
	{
		MCGContextScaleCTM(t_gcontext, t_scale, t_scale);
		MCGContextTranslateCTM(t_gcontext, -(MCGFloat)p_mark_clip . x, -(MCGFloat)p_mark_clip . y);
		
		m_composite_context = t_gcontext;
		m_composite_rect = p_mark_clip;
		m_composite_scale = t_scale;
		
		r_context = m_composite_context;
	}

	m_execute_error = !t_success;
	
	return t_success;
}

static void surface_merge_with_mask_preserve(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			t_pixel_ptr[x] = t_pixel_ptr[x] & 0xFFFFFF;
			if ((t_byte & t_bit) != 0)
				t_pixel_ptr[x] |= 0xFF000000;

			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes++;
		}
	}
}

void MCCustomMetaContext::endcomposite(MCRegionRef p_clip_region)
{
	bool t_success = true;
	
	MCGImageRef t_image;
	t_image = nil;
	
	t_success = nil != m_composite_context;
	
	if (t_success)
		t_success = MCGContextCopyImage(m_composite_context, t_image);
	
	MCGContextRelease(m_composite_context);
	m_composite_context = nil;
	
	void *t_pixel_cache;
	t_pixel_cache = nil;
	
	if (t_success)
	{
		/* OVERHAUL - REVISIT: Disabling the mask stuff for now, just treat the composite image as ARGB */
		
		// Make sure the region is in logical coords.
		//MCRegionOffset(p_clip_region, -(signed)m_composite_rect . x * m_composite_scale, -(signed)m_composite_rect . y * m_composite_scale);
		
		// We now need to merge the region into the surface as a 1-bit mask... So first
		// get the region as such a mask
		//MCBitmap *t_mask;
		//if (MCRegionCalculateMask(p_clip_region, t_image -> width, t_image -> height, t_mask))
		//{
		//	// And then merge in the mask - note we preserve the pixel data of the pixels
		//	// behind the mask... This is to eliminate rendering artifacts when the image
		//	// is resampled for display... Hmm - although the artifacts may be a cairo
		//	// problem - doing this doesn't seem to solve the issue!
		//	surface_merge_with_mask_preserve(t_image -> data, t_image -> bytes_per_line, t_mask -> data, t_mask -> bytes_per_line, 0, t_image -> width, t_image -> height);
		
		// Now we have a masked image, issue an appropriate image rendering call to the
		// device
		MCCustomPrinterImage t_img_data;
		
		t_success = MCCustomPrinterImageFromMCGImage(t_image, t_img_data, t_pixel_cache);
		
		MCCustomPrinterTransform t_img_transform;
		t_img_transform . scale_x = m_scale_x / m_composite_scale;
		t_img_transform . scale_y = m_scale_y / m_composite_scale;
		t_img_transform . skew_x = 0.0;
		t_img_transform . skew_y = 0.0;
		t_img_transform . translate_x = m_scale_x * m_composite_rect . x + m_translate_x;
		t_img_transform . translate_y = m_scale_y * m_composite_rect . y + m_translate_y;
		
		MCCustomPrinterRectangle t_img_clip;
		compute_clip(m_composite_rect, t_img_clip);
		
		t_success = m_device -> DrawImage(t_img_data, t_img_transform, t_img_clip);
		
		// Destroy our mask image
		//	MCscreen -> destroyimage(t_mask);
		//}
		//else
		//	m_execute_error = true;
	}

	if (t_image != nil)
		MCGImageRelease(t_image);
	
	if (t_pixel_cache != nil)
		MCMemoryDeallocate(t_pixel_cache);
	
	m_execute_error = !t_success;
	
	// Delete the region
	MCRegionDestroy(p_clip_region);
}

void MCCustomMetaContext::dopathmark(MCMark *p_mark, MCPath *p_path)
{
	uint32_t t_command_count, t_ordinate_count;
	p_path -> get_lengths(t_command_count, t_ordinate_count);
	dorawpathmark(p_mark, p_path -> get_commands(), t_command_count, p_path -> get_ordinates(), t_ordinate_count, false);
}

void MCCustomMetaContext::dorawpathmark(MCMark *p_mark, uint1 *p_commands, uint32_t p_command_count, int4 *p_ordinates, uint32_t p_ordinate_count, bool p_evenodd)
{
	MCCustomPrinterPathCommand *t_out_commands;
	t_out_commands = new (nothrow) MCCustomPrinterPathCommand[p_command_count];

	MCCustomPrinterPoint *t_out_coords;
	t_out_coords = new (nothrow) MCCustomPrinterPoint[p_ordinate_count];

	if (t_out_commands != nil && t_out_coords != nil)
	{
		for(uint32_t i = 0, j = 0; i < p_command_count; i++)
			switch(p_commands[i])
			{
			case PATH_COMMAND_END:
				t_out_commands[i] = kMCCustomPrinterPathEnd;
				break;
			case PATH_COMMAND_MOVE_TO:
				t_out_commands[i] = kMCCustomPrinterPathMoveTo;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				break;
			case PATH_COMMAND_LINE_TO:
				t_out_commands[i] = kMCCustomPrinterPathLineTo;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				break;
			case PATH_COMMAND_CUBIC_TO:
				t_out_commands[i] = kMCCustomPrinterPathCubicTo;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				break;
			case PATH_COMMAND_QUADRATIC_TO:
				t_out_commands[i] = kMCCustomPrinterPathQuadraticTo;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				t_out_coords[j] . x = p_ordinates[j * 2] / 256.0;
				t_out_coords[j] . y = p_ordinates[j * 2 + 1] / 256.0;
				j++;
				break;
			case PATH_COMMAND_CLOSE:
				t_out_commands[i] = kMCCustomPrinterPathClose;
				break;
			default:
				break;
			}

		// First construct the printer path.
		MCCustomPrinterPath t_path;
		t_path . commands = t_out_commands;
		t_path . coords = t_out_coords;

		// Now construct the printer paint.
		MCCustomPrinterPaint t_paint;
		t_paint . type = kMCCustomPrinterPaintNone;

		// This is a temporary array containing the gradient stops (if any).
		MCCustomPrinterGradientStop *t_paint_stops;
		t_paint_stops = nil;

		MCGImageRef t_image;
		t_image = nil;
		
		void *t_pixel_cache;
		t_pixel_cache = nil;
		
		// Note we have to check the fill in this order since 'gradient' is not
		// a fill style and is indicated by the gradient field not being nil.
		if (p_mark -> fill -> gradient != nil)
		{
			t_paint . type = kMCCustomPrinterPaintGradient;
			t_paint . gradient . mirror = p_mark -> fill -> gradient -> mirror != 0;
			t_paint . gradient . wrap = p_mark -> fill -> gradient -> wrap != 0;
			t_paint . gradient . repeat = p_mark -> fill -> gradient -> repeat;

			MCCustomPrinterGradientFromMCGradient((MCGradientFillKind)p_mark->fill->gradient->kind, t_paint.gradient.type);

			// compute the affine transform of the gradient from the origin/primary/secondary points.

			t_paint . gradient . transform . scale_x = p_mark->fill->gradient->primary.x - p_mark->fill->gradient->origin.x;
			t_paint . gradient . transform . scale_y = p_mark->fill->gradient->secondary.y - p_mark->fill->gradient->origin.y;
			t_paint . gradient . transform . skew_x = p_mark->fill->gradient->secondary.x - p_mark->fill->gradient->origin.x;
			t_paint . gradient . transform . skew_y = p_mark->fill->gradient->primary.y - p_mark->fill->gradient->origin.y;
			t_paint . gradient . transform . translate_x = p_mark->fill->gradient->origin.x;
			t_paint . gradient . transform . translate_y = p_mark->fill->gradient->origin.y;

			// Map the paint stops appropriately
			t_paint_stops = new (nothrow) MCCustomPrinterGradientStop[p_mark -> fill -> gradient -> ramp_length];
			if (t_paint_stops != nil)
			{
				for(uint32_t i = 0; i < p_mark -> fill -> gradient -> ramp_length; i++)
				{
					t_paint_stops[i] . color . red = ((p_mark -> fill -> gradient -> ramp[i] . color >> 16) & 0xff) / 255.0;
					t_paint_stops[i] . color . green = ((p_mark -> fill -> gradient -> ramp[i] . color >> 8) & 0xff) / 255.0;
					t_paint_stops[i] . color . blue = (p_mark -> fill -> gradient -> ramp[i] . color & 0xff) / 255.0;
					t_paint_stops[i] . alpha = (p_mark -> fill -> gradient -> ramp[i] . color >> 24) / 255.0;
					t_paint_stops[i] . offset = p_mark -> fill -> gradient -> ramp[i] . offset / 65535.0;
				}
				t_paint . gradient . stops = t_paint_stops;
				t_paint . gradient . stop_count = p_mark -> fill -> gradient -> ramp_length;
			}
			else
				m_execute_error = true;
		}
		else if (p_mark -> fill -> style == FillSolid)
		{
			t_paint . type = kMCCustomPrinterPaintSolid;
			t_paint . solid . red = p_mark -> fill -> colour . red / 65535.0;
			t_paint . solid . green = p_mark -> fill -> colour . green / 65535.0;
			t_paint . solid . blue = p_mark -> fill -> colour . blue / 65535.0;
		}
		else if (p_mark -> fill -> style == FillTiled)
		{
			// Fetch the size of the tile, and its data.
			
			MCGAffineTransform t_transform;
			
			// IM-2014-05-13: [[ HiResPatterns ]] Update pattern access to use lock function
			if (MCPatternLockForContextTransform(p_mark->fill->pattern, MCGAffineTransformMakeIdentity(), t_image, t_transform))
			{
				MCGRaster t_tile_raster;
				/* UNCHECKED */ MCGImageGetRaster(t_image, t_tile_raster);
				
				t_transform = MCGAffineTransformPreTranslate(t_transform, p_mark->fill->origin.x, p_mark->fill->origin.y);
				
				// Construct the paint pattern.
				t_paint . type = kMCCustomPrinterPaintPattern;
				t_paint . pattern . transform = MCCustomPrinterTransformFromMCGAffineTransform(t_transform);
				if (!MCCustomPrinterImageFromMCGImage(t_image, t_paint . pattern . image, t_pixel_cache))
					m_execute_error = true;
				
				MCGImageRetain(t_image);
				MCPatternUnlock(p_mark->fill->pattern, t_image);
			}
			else
				m_execute_error = true;
		}

		if (!m_execute_error)
		{
			// Compute the transform for the path. The path must be transformed by
			// the printing device because it needs to happen *after* a stroke has
			// been applied.
			MCCustomPrinterTransform t_transform;
			t_transform . scale_x = m_scale_x;
			t_transform . scale_y = m_scale_y;
			t_transform . skew_x = 0.0;
			t_transform . skew_y = 0.0;
			t_transform . translate_x = m_translate_x;
			t_transform . translate_y = m_translate_y;

			// Compute the clip for the path - the clip is in page-space, so we
			// transform here.
			MCCustomPrinterRectangle t_clip;
			compute_clip(p_mark -> clip, t_clip);

			if (p_mark -> stroke == nil)
			{
				if (!m_device -> FillPath(t_path, p_evenodd ? kMCCustomPrinterFillEvenOdd : kMCCustomPrinterFillNonZero, t_paint, t_transform, t_clip))
					m_execute_error = true;
			}
			else
			{
				MCCustomPrinterStroke t_stroke;

				// MW-2010-02-01: [[ Bug 8564 ]] If the width of the stroke is zero width, then the stroke
				//   parameters are: thickness == 1, cap == butt, join == miter.
				if (p_mark -> stroke -> width != 0)
				{
					t_stroke . thickness = p_mark -> stroke -> width;
					t_stroke . cap_style = p_mark -> stroke -> cap == CapButt ? kMCCustomPrinterCapButt :
											(p_mark -> stroke -> cap == CapRound ? kMCCustomPrinterCapRound :
												kMCCustomPrinterCapSquare);
					t_stroke . join_style = p_mark -> stroke -> join == JoinBevel ? kMCCustomPrinterJoinBevel :
												(p_mark -> stroke -> join == JoinRound ? kMCCustomPrinterJoinRound :
													kMCCustomPrinterJoinMiter);
					t_stroke . miter_limit = p_mark -> stroke -> miter_limit;
				}
				else
				{
					t_stroke . thickness = 1.0;
					t_stroke . cap_style = kMCCustomPrinterCapButt;
					t_stroke . join_style = kMCCustomPrinterJoinMiter;
					t_stroke . miter_limit = 10;
				}

				if (p_mark -> stroke -> dash . length != 0)
				{
					t_stroke . dash_count = p_mark -> stroke -> dash . length;
					t_stroke . dash_offset = p_mark -> stroke -> dash . offset;
					t_stroke . dashes = new (nothrow) double[p_mark -> stroke -> dash . length];
					if (t_stroke . dashes != nil)
					{
						for(uint32_t i = 0; i < p_mark -> stroke -> dash . length; i++)
							t_stroke . dashes[i] = p_mark -> stroke -> dash . data[i];
					}
					else
						m_execute_error = true;
				}
				else
				{
					t_stroke . dashes = nil;
					t_stroke . dash_count = 0;
					t_stroke . dash_offset = 0.0;
				}
				
				if (!m_execute_error)
					if (!m_device -> StrokePath(t_path, t_stroke, t_paint, t_transform, t_clip))
						m_execute_error = true;

				delete[] t_stroke . dashes;
			}
		}

		if (t_paint_stops != nil)
			delete[] t_paint_stops;
		
		if (t_image != nil)
			MCGImageRelease(t_image);
		
		if (t_pixel_cache != nil)
			MCMemoryDeallocate(t_pixel_cache);
	}
	else
		m_execute_error = true;

	delete[] t_out_coords;
	delete[] t_out_commands;
}

//////////

struct dotextmark_callback_state
{
	MCCustomPrintingDevice *device;
	MCCustomPrinterPaint paint;
	MCCustomPrinterTransform transform;
	MCCustomPrinterRectangle clip;
	double font_size;
};

static bool dotextmark_callback(void *p_context, const MCTextLayoutSpan *p_span)
{
	dotextmark_callback_state *context;
	context = (dotextmark_callback_state *)p_context;
	
	MCAutoStringRef t_string;
    // SN-2014-06-17 [[ Bug 12595 ]] Not properly causing the bug, but it never hurts to get to use the right encoding
    MCStringCreateWithBytes((byte_t*)p_span->chars, p_span->char_count * 2, kMCStringEncodingUTF16, false, &t_string);
    
	byte_t *t_bytes;
	uindex_t t_byte_count;
	/* UNCHECKED */ MCStringConvertToBytes(*t_string, kMCStringEncodingUTF8, false, t_bytes, t_byte_count);
	
	// Allocate a cluster index for every UTF-8 byte
	uint32_t *t_clusters;
	t_clusters = nil;
	if (!MCMemoryNewArray(t_byte_count, t_clusters))
    {
        MCMemoryDeleteArray(t_bytes);
		return false;
    }
	
	// Now loop through and build up the cluster array. Notice we keep track of
	// UTF-16 codepoint index by taking note of leading UTF-8 bytes.
	uint32_t t_char_index, t_next_char_index;
	t_char_index = t_next_char_index = 0;
	for(uint32_t i = 0; i < t_byte_count; i++)
	{
		if ((t_bytes[i] & 0x80) == 0)
		{
			// This is a single byte char - so it comes from a single UTF-16 code point
			t_char_index = t_next_char_index;
			t_next_char_index += 1;
		}
		else if ((t_bytes[i] & 0xC0) == 0xC0)
		{
			t_char_index = t_next_char_index;
			
			if ((t_bytes[i] & 0xf0) != 0xf0)
			{
				// This is a 2 or 3 byte char - so it comes from a single UTF-16 code point
				t_next_char_index += 1;
			}
			else
			{
				// This is a 4 byte char - so it must come from a surrogate pair, i.e. 2 UTF-16 code points
				t_next_char_index += 2;
			}
		}
		
		t_clusters[i] = p_span -> clusters[t_char_index];
	}
	
	double t_font_size;
	void *t_font_handle;
#if defined(_WINDOWS_DESKTOP)
	// MW-2013-11-07: [[ Bug 10508 ]] Pass the font size into computefontsize so it can
	//   be scaled (layout uses 256px fonts).
	extern int32_t MCCustomPrinterComputeFontSize(double size, void *font);
	t_font_size = MCCustomPrinterComputeFontSize(context -> font_size, p_span -> font);
	t_font_handle = p_span -> font;
#elif defined(_MAC_DESKTOP)
	t_font_size = context -> font_size;
	t_font_handle = p_span -> font;
#elif defined(_LINUX_DESKTOP)
	t_font_size = context -> font_size;
	t_font_handle = p_span -> font;
#elif defined(_IOS_MOBILE)
	extern int32_t MCCustomPrinterComputeFontSize(void *font);
	t_font_size = MCCustomPrinterComputeFontSize(p_span -> font);
	t_font_handle = p_span -> font;
#elif defined(ANDROID)
	t_font_size = context->font_size;
	t_font_handle = p_span->font;
#else
    // Neither servers nor Android have an implementation
	t_font_size = 0;
	t_font_handle = NULL;
#endif

	MCCustomPrinterFont t_font;
	t_font . size = t_font_size;
	t_font . handle = t_font_handle;

	bool t_success;
	t_success = context -> device -> DrawText((const MCCustomPrinterGlyph *)p_span -> glyphs, p_span -> glyph_count, (const char *)t_bytes, t_byte_count, t_clusters, t_font, context -> paint, context -> transform, context -> clip);

	MCMemoryDeleteArray(t_clusters);
    MCMemoryDeleteArray(t_bytes);
	
	return t_success;
}

void MCCustomMetaContext::dotextmark(MCMark *p_mark)
{
    MCFontStruct *f = MCFontGetFontStruct(p_mark -> text . font);
    
	MCAutoStringRef t_text_str;
    if (p_mark -> text . unicode_override)
        /* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)p_mark -> text . data, p_mark -> text . length, &t_text_str);
    else
        /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t*)p_mark -> text . data, p_mark -> text . length, &t_text_str);

	unichar_t *t_chars;
	uindex_t t_char_count;
	/* UNCHECKED */ MCStringConvertToUnicode(*t_text_str, t_chars, t_char_count);
	
	dotextmark_callback_state t_state;
	t_state . device = m_device;

	t_state . paint . type = kMCCustomPrinterPaintSolid;
	t_state . paint . solid . red = p_mark -> fill -> colour . red / 65535.0;
	t_state . paint . solid . green = p_mark -> fill -> colour . green / 65535.0;
	t_state . paint . solid . blue = p_mark -> fill -> colour . blue / 65535.0;

	t_state . transform . scale_x = m_scale_x;
	t_state . transform . scale_y = m_scale_y;
	t_state . transform . skew_x = 0;
	t_state . transform . skew_y = 0;
	t_state . transform . translate_x = m_translate_x + m_scale_x * p_mark -> text . position . x;
	t_state . transform . translate_y = m_translate_y + m_scale_y * p_mark -> text . position . y;

#if defined(_MACOSX) || defined(_WINDOWS)
	t_state . font_size = f -> size;
#elif defined(_LINUX)
	extern MCFontlist *MCFontlistGetCurrent(void);
	MCNameRef t_name;
	uint2 t_size, t_style;
    MCFontlistGetCurrent() -> getfontreqs(f, t_name, t_size, t_style);
	t_state . font_size = t_size;
#elif defined(ANDROID)
	t_state . font_size = f -> size;
#endif

	compute_clip(p_mark -> clip, t_state . clip);

	if (!MCTextLayout(t_chars, t_char_count, f, dotextmark_callback, &t_state))
		m_execute_error = true;
}

//////////

void MCCustomMetaContext::transform_point(const MCPoint& p_point, MCCustomPrinterPoint& r_out_point)
{
	r_out_point . x = p_point . x * m_scale_x + m_translate_x;
	r_out_point . y = p_point . y * m_scale_y + m_translate_y;
}

void MCCustomMetaContext::transform_rect(const MCRectangle& p_rect, MCCustomPrinterRectangle& r_out_rect)
{
	r_out_rect . left = p_rect . x * m_scale_x + m_translate_x;
	r_out_rect . top = p_rect . y * m_scale_y + m_translate_y;
	r_out_rect . right = r_out_rect . left + p_rect . width * m_scale_x;
	r_out_rect . bottom = r_out_rect . top + p_rect . height * m_scale_y;
}

void MCCustomMetaContext::compute_clip(const MCRectangle& p_rect, MCCustomPrinterRectangle& r_out_rect)
{
	transform_rect(p_rect, r_out_rect);

	r_out_rect . left = MCMax(r_out_rect . left, m_page_rect . left);
	r_out_rect . top = MCMax(r_out_rect . top, m_page_rect . top);
	r_out_rect . right = MCMin(r_out_rect . right, m_page_rect . right);
	r_out_rect . bottom = MCMin(r_out_rect . bottom, m_page_rect . bottom);
}

////////////////////////////////////////////////////////////////////////////////

class MCCustomPrinterDevice: public MCPrinterDevice
{
public:
	MCCustomPrinterDevice(MCCustomPrintingDevice *device);
	~MCCustomPrinterDevice(void);

	const char *Error(void) const;

	MCPrinterResult Start(const char *p_title, MCArrayRef p_options);
	MCPrinterResult Finish(void);

	MCPrinterResult Cancel(void);
	MCPrinterResult Show(void);

	MCPrinterResult Begin(const MCPrinterRectangle& src_rect, const MCPrinterRectangle& dst_rect, MCContext*& r_context);
	MCPrinterResult End(MCContext *context);

	MCPrinterResult Anchor(const char *name, double x, double y);
	MCPrinterResult Link(const char *dest, const MCPrinterRectangle& area, MCPrinterLinkType type);
	MCPrinterResult Bookmark(const char *title, double x, double y, int depth, bool closed);

private:
	bool StartPage(void);

	bool m_cancelled;
	MCCustomPrintingDevice *m_device;

	bool m_page_started;

	MCPrinterRectangle m_page_rect;
	MCPrinterRectangle m_src_rect;
	MCPrinterRectangle m_dst_rect;
};

MCCustomPrinterDevice::MCCustomPrinterDevice(MCCustomPrintingDevice *p_device)
{
	m_cancelled = false;
	m_page_started = false;
	m_device = p_device;
}

MCCustomPrinterDevice::~MCCustomPrinterDevice(void)
{
	// Regardless of anything else having happened, make sure we clear out the
	// text layout system.
	MCTextLayoutFinalize();
}

const char *MCCustomPrinterDevice::Error(void) const
{
	return m_device -> GetError();
}

struct convert_options_array_t
{
	uindex_t index;
	char **option_keys;
	char **option_values;
};

static bool convert_options_array(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	convert_options_array_t *ctxt;
	ctxt = (convert_options_array_t *)p_context;

	if (!MCStringConvertToCString(MCNameGetString(p_key), ctxt -> option_keys[ctxt -> index]))
		return false;
    
    // SN-2014-08-11: [[ Bug 13146 ]] Also allow NameRef to be passed as options.
	MCAutoStringRef t_value;
    if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeName)
        t_value = MCNameGetString((MCNameRef)p_value);
    else if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString)
        t_value = (MCStringRef)p_value;
    else
    {
        ctxt -> option_values[ctxt -> index] = NULL;
        return false;
    }
	
	if (!MCStringConvertToCString(*t_value, ctxt -> option_values[ctxt -> index]))
		return false;
	
	ctxt -> index += 1;
	return true;
}

MCPrinterResult MCCustomPrinterDevice::Start(const char *p_title, MCArrayRef p_options)
{
	MCPrinterResult t_result;
	t_result = PRINTER_RESULT_SUCCESS;

	// Make sure the text system will initialize
	if (!MCTextLayoutInitialize())
		return PRINTER_RESULT_FAILURE;

	// Now begin the document.
	MCCustomPrinterDocument t_document;
	t_document . title = p_title;
	t_document . filename = MCprinter -> GetDeviceOutputLocation();

	// Extract the option strings
	char **t_option_keys, **t_option_values;
	uint32_t t_option_count;
	t_option_keys = t_option_values = nil;
	t_option_count = 0;
	if (t_result == PRINTER_RESULT_SUCCESS && p_options != nil)
	{
		t_option_count = MCArrayGetCount(p_options);
		if (!MCMemoryNewArray(t_option_count, t_option_keys) ||
			!MCMemoryNewArray(t_option_count, t_option_values))
			t_result = PRINTER_RESULT_ERROR;
	}
	
	if (t_result == PRINTER_RESULT_SUCCESS)
	{
		convert_options_array_t ctxt;
		ctxt . index = 0;
		ctxt . option_keys = t_option_keys;
		ctxt . option_values = t_option_values;
        
        // FG-2014-05-23: [[ Bugfix 12502 ]] Don't try to convert empty options
        if (p_options != nil && !MCArrayIsEmpty(p_options))
        {
            if (!MCArrayApply(p_options, convert_options_array, &ctxt))
                t_result = PRINTER_RESULT_ERROR;
        }
	}

	if (t_result == PRINTER_RESULT_SUCCESS)
	{
		t_document . option_count = t_option_count;
		t_document . option_keys = t_option_keys;
		t_document . option_values = t_option_values;
		if (!m_device -> BeginDocument(t_document))
			t_result = PRINTER_RESULT_ERROR;
	}

	if (t_option_values != nil)
		for(uint32_t i = 0; i < t_document . option_count; i++)
		{
			delete(t_option_keys[i]);
			delete(t_option_values[i]);
		}

	MCMemoryDeleteArray(t_option_values);
	MCMemoryDeleteArray(t_option_keys);

	// No page has been started yet.
	m_page_started = false;

	return t_result;
}

MCPrinterResult MCCustomPrinterDevice::Finish(void)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// Finish the current page, if we are inside one
	if (m_page_started &&
		!m_device -> EndPage())
		return PRINTER_RESULT_ERROR;

	// Now finish the document
	if (!m_device -> EndDocument())
		return PRINTER_RESULT_ERROR;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCCustomPrinterDevice::Cancel(void)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	m_device -> AbortDocument();

	// Make sure don't call any more methods on the device
	m_cancelled = true;

	return PRINTER_RESULT_CANCEL;
}

MCPrinterResult MCCustomPrinterDevice::Show(void)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// If we aren't currently in a page, start one
	if (!m_page_started &&
		!StartPage())
		return PRINTER_RESULT_ERROR;

	// End the current page.
	if (!m_device -> EndPage())
		return PRINTER_RESULT_ERROR;

	m_page_started = false;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCCustomPrinterDevice::Begin(const MCPrinterRectangle& p_src_rect, const MCPrinterRectangle& p_dst_rect, MCContext*& r_context)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// Start a page if we aren't already in one
	if (!m_page_started)
		if (!StartPage())
			return PRINTER_RESULT_ERROR;

    // MW-2014-07-30: [[ Bug 12804 ]] Make sure we get left / top round the right way else clipping
    //   is wrong.
	// Calculate the convex integer hull of the source rectangle.
	MCRectangle t_src_rect_hull;
	t_src_rect_hull . x = (int2)floor(p_src_rect . left);
	t_src_rect_hull . y = (int2)floor(p_src_rect . top);
	t_src_rect_hull . width = (uint2)(ceil(p_src_rect . right) - floor(p_src_rect . left));
	t_src_rect_hull . height = (uint2)(ceil(p_src_rect . bottom) - floor(p_src_rect . top));

	// Now create a custom meta context, targeting our device
	MCCustomMetaContext *t_context;
	t_context = new (nothrow) MCCustomMetaContext(t_src_rect_hull);
	if (t_context == nil)
		return PRINTER_RESULT_ERROR;

	m_src_rect = p_src_rect;
	m_dst_rect = p_dst_rect;

	// MW-2010-09-27: [[ Bug 8999 ]] Make sure we use the correct rectangle.
	MCRectangle t_page_rect;
	t_page_rect = MCprinter -> GetPageRectangle();
	m_page_rect . left = t_page_rect . x;
	m_page_rect . top = t_page_rect . y;
	m_page_rect . right = t_page_rect . x + t_page_rect . width;
	m_page_rect . bottom = t_page_rect . y + t_page_rect . height;

	r_context = t_context;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCCustomPrinterDevice::End(MCContext *p_raw_context)
{
	MCCustomMetaContext *t_context;
	t_context = static_cast<MCCustomMetaContext *>(p_raw_context);

	MCPrinterResult t_result;
	t_result = PRINTER_RESULT_SUCCESS;

	// If we are in an error state, do nothing
	if (t_result == PRINTER_RESULT_SUCCESS &&
		m_cancelled)
		t_result = PRINTER_RESULT_CANCEL;

	if (t_result == PRINTER_RESULT_SUCCESS &&
		m_device -> GetError() != nil)
		t_result = PRINTER_RESULT_ERROR;

	// Attempt to render the meta surface
	if (t_result == PRINTER_RESULT_SUCCESS &&
		!t_context -> render(m_device, m_src_rect, m_dst_rect, m_page_rect))
		t_result = PRINTER_RESULT_ERROR;

	// Delete the context (since we 'own' it by virtue of creating it
	// in ::Begin)
	delete t_context;

	// Return the result
	return t_result;
}

MCPrinterResult MCCustomPrinterDevice::Anchor(const char *p_name, double p_x, double p_y)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// Start a page if we aren't already in one
	if (!m_page_started)
		if (!StartPage())
			return PRINTER_RESULT_ERROR;

	MCCustomPrinterPoint t_location;
	t_location . x = p_x;
	t_location . y = p_y;
	if (!m_device -> MakeAnchor(t_location, p_name))
		return PRINTER_RESULT_ERROR;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCCustomPrinterDevice::Link(const char *p_dest, const MCPrinterRectangle& p_area, MCPrinterLinkType p_type)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// Start a page if we aren't already in one
	if (!m_page_started)
		if (!StartPage())
			return PRINTER_RESULT_ERROR;

	MCCustomPrinterRectangle t_rect;
	t_rect . left = p_area . left;
	t_rect . top = p_area . top;
	t_rect . right = p_area . right;
	t_rect . bottom = p_area . bottom;
	if (!m_device -> MakeLink(t_rect, p_dest, (MCCustomPrinterLinkType)p_type))
		return PRINTER_RESULT_ERROR;

	return PRINTER_RESULT_SUCCESS;
}

MCPrinterResult MCCustomPrinterDevice::Bookmark(const char *p_title, double p_x, double p_y, int p_depth, bool p_closed)
{
	// If we are cancelled, do nothing
	if (m_cancelled)
		return PRINTER_RESULT_CANCEL;

	// If we are in an error state, do nothing
	if (m_device -> GetError() != nil)
		return PRINTER_RESULT_ERROR;

	// Start a page if we aren't already in one
	if (!m_page_started)
		if (!StartPage())
			return PRINTER_RESULT_ERROR;

	MCCustomPrinterPoint t_location;
	t_location . x = p_x;
	t_location . y = p_y;
	if (!m_device -> MakeBookmark(t_location, p_title, p_depth, p_closed))
		return PRINTER_RESULT_ERROR;

	return PRINTER_RESULT_SUCCESS;
}

bool MCCustomPrinterDevice::StartPage(void)
{
	MCCustomPrinterPage t_page;

	// MW-2010-06-07: [[ Bug 8662 ]] If the page is meant to be landscape, switch
	//   the width and height appropriately.
	if (MCprinter -> GetPageOrientation() == PRINTER_ORIENTATION_LANDSCAPE ||
		MCprinter -> GetPageOrientation() == PRINTER_ORIENTATION_REVERSE_LANDSCAPE)
	{
		t_page . width = MCprinter -> GetPageHeight();
		t_page . height = MCprinter -> GetPageWidth();
	}
	else
	{
		t_page . width = MCprinter -> GetPageWidth();
		t_page . height = MCprinter -> GetPageHeight();
	}

	t_page . scale = MCprinter -> GetPageScale();
	
	if (!m_device -> BeginPage(t_page))
		return false;

	m_page_started = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCCustomPrinter::MCCustomPrinter(MCStringRef p_name, MCCustomPrintingDevice *p_device)
{
	m_device_options = nil;
	m_device_name = MCValueRetain(p_name);
	m_device = p_device;
}

MCCustomPrinter::~MCCustomPrinter(void)
{
	m_device -> Destroy();

	// We do this here since the MCCustomPrinter instance has a limited lifetime - the
	// scope of a single print loop.
	Finalize();

	MCValueRelease(m_device_name);
	MCValueRelease(m_device_options);
}

void MCCustomPrinter::SetDeviceOptions(MCArrayRef p_options)
{
	MCValueRelease(m_device_options);
	m_device_options = nil;

	if (p_options != nil)
		/* UNCHECKED */ MCArrayCopy(p_options, m_device_options);
}

void MCCustomPrinter::DoInitialize(void)
{
}

void MCCustomPrinter::DoFinalize(void)
{
}

bool MCCustomPrinter::DoReset(MCStringRef p_name)
{
	return MCStringIsEqualTo(p_name, m_device_name, kMCStringOptionCompareCaseless);
}

bool MCCustomPrinter::DoResetSettings(MCDataRef p_settings)
{
	return MCDataIsEmpty(p_settings);
}

const char *MCCustomPrinter::DoFetchName(void)
{
    char *t_device_name;
    /* UNCHECKED */ MCStringConvertToCString(m_device_name, t_device_name);
    return t_device_name;
}

void MCCustomPrinter::DoResync(void)
{
}

void MCCustomPrinter::DoFetchSettings(void*& r_buffer, uint4& r_length)
{
	r_buffer = nil;
	r_length = 0;
}

MCPrinterDialogResult MCCustomPrinter::DoPrinterSetup(bool p_window_modal, Window p_owner)
{
	return PRINTER_DIALOG_RESULT_ERROR;
}

MCPrinterDialogResult MCCustomPrinter::DoPageSetup(bool p_window_modal, Window p_owner)
{
	return PRINTER_DIALOG_RESULT_ERROR;
}

MCPrinterResult MCCustomPrinter::DoBeginPrint(MCStringRef p_document, MCPrinterDevice*& r_device)
{
	MCPrinterResult t_result;
	t_result = PRINTER_RESULT_SUCCESS;

	MCCustomPrinterDevice *t_printer_device;
	t_printer_device = nil;
	if (t_result == PRINTER_RESULT_SUCCESS)
	{
		t_printer_device = new (nothrow) MCCustomPrinterDevice(m_device);
		if (t_printer_device == nil)
			t_result = PRINTER_RESULT_ERROR;
	}
	
	if (t_result == PRINTER_RESULT_SUCCESS)
    {
        MCAutoPointer<char> t_doc;
        /* UNCHECKED */ MCStringConvertToCString(p_document, &t_doc);
        t_result = t_printer_device -> Start(*t_doc, m_device_options);
    }

	if (t_result == PRINTER_RESULT_SUCCESS)
		r_device = t_printer_device;
	else
		delete t_printer_device;

	return t_result;
}

MCPrinterResult MCCustomPrinter::DoEndPrint(MCPrinterDevice* p_device)
{
	MCCustomPrinterDevice *t_device;
	t_device = static_cast<MCCustomPrinterDevice *>(p_device);

	MCPrinterResult t_result;
	t_result = t_device -> Finish();

	delete t_device;

	return t_result;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

class MCLoggingPrintingDevice: public MCCustomPrintingDevice
{
public:
	MCLoggingPrintingDevice(MCCustomPrintingDevice *p_target)
	{
		m_target = p_target;
	}

	virtual ~MCLoggingPrintingDevice(void) {}

	////////

	void Destroy(void)
	{
		m_target -> Destroy();
		delete this;
	}

	const char *GetError(void)
	{
		return m_target -> GetError();
	}

	bool CanRenderPaint(const MCCustomPrinterPaint& p_paint)
	{
		return m_target -> CanRenderPaint(p_paint);
	}

	bool CanRenderImage(const MCCustomPrinterImage& p_image)
	{
		return m_target -> CanRenderImage(p_image);
	}

	bool CanRenderGroup(const MCCustomPrinterGroup& p_group)
	{
		return m_target -> CanRenderGroup(p_group);
	}

	bool BeginDocument(const MCCustomPrinterDocument& p_document)
	{
		if (!m_target -> BeginDocument(p_document))
			return Failed("BeginDocument");
		return true;
	}

	void AbortDocument(void)
	{
	}

	bool EndDocument(void)
	{
		if (!m_target -> EndDocument())
			return Failed("EndDocument");
		return true;
	}

	bool BeginPage(const MCCustomPrinterPage& p_page)
	{
		if (!m_target -> BeginPage(p_page))
			return Failed("BeginPage");
		return true;
	}

	bool EndPage(void)
	{
		if (!m_target -> EndPage())
			return Failed("EndPage");
		return true;
	}

	bool BeginGroup(const MCCustomPrinterGroup& p_group)
	{
		if (!m_target -> BeginGroup(p_group))
			return Failed("BeginPage");
		return true;
	}

	bool EndGroup(void)
	{
		if (!m_target -> EndGroup())
			return Failed("EndGroup");
		return true;
	}

	bool FillPath(const MCCustomPrinterPath& p_path, MCCustomPrinterFillRule p_rule, const MCCustomPrinterPaint& p_paint, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		if (!m_target -> FillPath(p_path, p_rule, p_paint, p_transform, p_clip))
			return Failed("FillPath");
		return true;
	}

	bool StrokePath(const MCCustomPrinterPath& p_path, const MCCustomPrinterStroke& p_stroke, const MCCustomPrinterPaint& p_paint, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		if (!m_target -> StrokePath(p_path, p_stroke, p_paint, p_transform, p_clip))
			return Failed("StrokePath");
		return true;
	}

	bool DrawImage(const MCCustomPrinterImage& p_image, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		if (!m_target -> DrawImage(p_image, p_transform, p_clip))
			return Failed("DrawImage");
		return true;
	}

	bool DrawText(const MCCustomPrinterGlyph *glyphs, uint32_t glyph_count, const char *text_bytes, uint32_t text_byte_count, const uint32_t *clusters, const MCCustomPrinterFont& font, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& p_clip)
	{
		if (!m_target -> DrawText(glyphs, glyph_count, text_bytes, text_byte_count, clusters, font, paint, transform, p_clip))
			return Failed("DrawText");
		return true;
	}

	bool MakeAnchor(const MCCustomPrinterPoint& p_position, const char *p_name)
	{
		if (!m_target -> MakeAnchor(p_position, p_name))
			return Failed("MakeAnchor");
		return true;
	}

	bool MakeLink(const MCCustomPrinterRectangle& p_area, const char *p_link, MCCustomPrinterLinkType p_type)
	{
		if (!m_target -> MakeLink(p_area, p_link, p_type))
			return Failed("MakeLink");
		return true;
	}

	bool MakeBookmark(const MCCustomPrinterPoint& p_position, const char *p_title, uint32_t p_depth, bool p_closed)
	{
		if (!m_target -> MakeBookmark(p_position, p_title, p_depth, p_closed))
			return Failed("MakeBookmark");
		return true;
	}
	
private:

	bool Failed(const char *p_proc)
	{
		fprintf(stderr, "Call to %s failed!\n", p_proc);
		return false;
	}

	MCCustomPrintingDevice *m_target;
};

class MCDebugPrintingDevice: public MCCustomPrintingDevice
{
public:
	MCDebugPrintingDevice(void)
	{
		m_error = nil;
		m_indent = 0;
		m_stream = nil;
	}

	virtual ~MCDebugPrintingDevice(void) {}

	void Destroy(void)
	{
		delete this;
	}

	const char *GetError(void)
	{
		return m_error;
	}

	bool CanRenderPaint(const MCCustomPrinterPaint& p_paint)
	{
		return true;
	}

	bool CanRenderImage(const MCCustomPrinterImage& p_image)
	{
		return true;
	}

	bool CanRenderGroup(const MCCustomPrinterGroup& p_group)
	{
		return true;
	}

	bool BeginDocument(const MCCustomPrinterDocument& p_document)
	{
		m_stream = fopen(p_document . filename, "w");
		if (m_stream == nil)
		{
			m_error = "could not open output file";
			return false;
		}

		Enter("begin document '%s'", p_document . title);
		return true;
	}

	void AbortDocument(void)
	{
		Print("**** ABORTED ****");
	}

	bool EndDocument(void)
	{
		Leave("end document");
		fclose(m_stream);
		m_stream = nil;
		return true;
	}

	bool BeginPage(const MCCustomPrinterPage& p_page)
	{
		Enter("begin page (%lf, %lf)", p_page . width, p_page . height);
		return true;
	}

	bool EndPage(void)
	{
		Leave("end page");
		return true;
	}

	bool BeginGroup(const MCCustomPrinterGroup& p_group)
	{
		static const char *s_blend_mode_names[] =
		{
			"blendSrc",
			"blendDst",
			"blendSrcOver",
			"blendDstOver",
			"blendSrcIn",
			"blendDstIn",
			"blendSrcOut",
			"blendDstOut",
			"blendSrcAtop",
			"blendDstAtop",
			"blendXor",
			"blendPlus",
			"blendMultiply",
			"blendScreen",
			"blendOverlay",
			"blendDarken",
			"blendLighten",
			"blendDodge",
			"blendBurn",
			"blendHardLight",
			"blendSoftLight",
			"blendDifference",
			"blendExclusion"
		};
		Enter("begin group with clip (%f, %f)-(%f, %f) and opacity %f and mode %s",
				p_group . region . left, p_group . region . top, p_group . region . right, p_group . region . bottom,
				p_group . opacity, s_blend_mode_names[p_group . blend_mode]);
		return true;
	}

	bool EndGroup(void)
	{
		Leave("end group");
		return true;
	}

	bool FillPath(const MCCustomPrinterPath& p_path, MCCustomPrinterFillRule p_rule, const MCCustomPrinterPaint& p_paint, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		Enter("begin fill path with clip (%f, %f)-(%f, %f) and rule %s", 
				p_clip . left, p_clip . top, p_clip . right, p_clip . bottom,
				p_rule == kMCCustomPrinterFillEvenOdd ? "even-odd" : "non-zero");
		PrintPath(p_path);
		Leave("end fill path");
		return true;
	}

	bool StrokePath(const MCCustomPrinterPath& p_path, const MCCustomPrinterStroke& p_stroke, const MCCustomPrinterPaint& p_paint, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		Enter("begin stroke path with clip (%f, %f)-(%f, %f)", 
				p_clip . left, p_clip . top, p_clip . right, p_clip . bottom);
		PrintPath(p_path);
		Leave("end stroke path");
		return true;
	}

	bool DrawImage(const MCCustomPrinterImage& p_image, const MCCustomPrinterTransform& p_transform, const MCCustomPrinterRectangle& p_clip)
	{
		static const char *s_image_types[] =
		{
			"none",
			"unmasked",
			"sharp masked",
			"soft masked",
			"gif",
			"jpeg",
			"png"
		};
		Enter("begin image with clip (%f, %f)-(%f, %f) of type %s", 
				p_clip . left, p_clip . top, p_clip . right, p_clip . bottom, s_image_types[p_image . type]);
		Leave("end image");
		return true;
	}

	bool DrawText(const MCCustomPrinterGlyph *glyphs, uint32_t glyph_count, const char *text_bytes, uint32_t text_byte_count, const uint32_t *clusters, const MCCustomPrinterFont& font, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& p_clip)
	{
		Enter("begin text '%s' with clip (%f, %f)-(%f, %f)", text_bytes, 
				p_clip . left, p_clip . top, p_clip . right, p_clip . bottom);
		for(uint32_t i = 0; i < glyph_count; i++)
			Print("glyph %d at (%f, %f)", glyphs[i] . id, glyphs[i] . x, glyphs[i] . y);
		Leave("end text");
		return true;
	}

	bool MakeAnchor(const MCCustomPrinterPoint& p_position, const char *p_name)
	{
		Print("anchor '%s' at (%f, %f)", p_name, p_position . x, p_position . y);
		return true;
	}

	bool MakeLink(const MCCustomPrinterRectangle& p_area, const char *p_link, MCCustomPrinterLinkType p_type)
	{
		Print("%s link to '%s' at (%f, %f, %f, %f)",
			p_type == kMCCustomPrinterLinkUnspecified ? "unspecified" :
			p_type == kMCCustomPrinterLinkAnchor ? "anchor" :
			p_type == kMCCustomPrinterLinkURI ? "uri" : "invalid",
			p_link, p_area . left, p_area . top, p_area . right, p_area . bottom);
		return true;
	}

	bool MakeBookmark(const MCCustomPrinterPoint& p_position, const char *p_title, uint32_t p_depth, bool p_closed)
	{
		Print("bookmark '%s' at (%f, %f) with depth (%d) %s", p_title, p_position.x, p_position.y, p_depth, p_closed ? "closed" : "");
		return true;
	}

private:
	void PrintPath(const MCCustomPrinterPath& p_path)
	{
		uint32_t i, j;
		i = j = 0;
		while(p_path . commands[i] != kMCCustomPrinterPathEnd)
		{
			switch(p_path . commands[i++])
			{
			case kMCCustomPrinterPathMoveTo:
				Print("move to (%f, %f)", p_path . coords[j] . x, p_path . coords[j] . y);
				j++;
				break;
			case kMCCustomPrinterPathLineTo:
				Print("line to (%f, %f)", p_path . coords[j] . x, p_path . coords[j] . y);
				j++;
				break;
			case kMCCustomPrinterPathQuadraticTo:
				Print("quadratic via (%f, %f) to (%f, %f)",
							p_path . coords[j] . x, p_path . coords[j] . y,
							p_path . coords[j + 1] . x, p_path . coords[j + 1] . y);
				j += 2;
				break;
			case kMCCustomPrinterPathCubicTo:
				Print("quadratic via (%f, %f) and (%f, %f) to (%f, %f)",
							p_path . coords[j] . x, p_path . coords[j] . y,
							p_path . coords[j + 1] . x, p_path . coords[j + 1] . y,
							p_path . coords[j + 2] . x, p_path . coords[j + 2] . y);
				j += 3;
				break;
			case kMCCustomPrinterPathClose:
				Print("close");
				break;
			case kMCCustomPrinterPathEnd:
				MCUnreachable();
			}
		}
	}

	void Enter(const char *p_format, ...)
	{
		va_list args;
		va_start(args, p_format);
		PrintWithArgs(p_format, args);
		va_end(args);

		m_indent += 1;
	}

	void Print(const char *p_format, ...)
	{
		va_list args;
		va_start(args, p_format);
		PrintWithArgs(p_format, args);
		va_end(args);
	}

	void Leave(const char *p_format, ...)
	{
		m_indent -= 1;

		va_list args;
		va_start(args, p_format);
		PrintWithArgs(p_format, args);
		va_end(args);
	}

	void PrintWithArgs(const char *p_format, va_list args)
	{
		static const char *s_spaces = "                                                                            ";
		fprintf(m_stream, "%.*s", m_indent * 2, s_spaces);
		vfprintf(m_stream, p_format, args);
		fprintf(m_stream, "\n");
	}

	FILE *m_stream;
	const char *m_error;
	uint32_t m_indent;
};

#endif

////////////////////////////////////////////////////////////////////////////////

typedef MCCustomPrintingDevice *(*MCCustomPrinterCreateProc)(void);

bool MCCustomPrinterCreate(MCStringRef p_destination, MCStringRef p_filename, MCArrayRef p_options, MCCustomPrinter*& r_printer)
{
	MCCustomPrintingDevice *t_device;
	t_device = nil;
	if (MCStringIsEqualToCString(p_destination, "pdf", kMCCompareCaseless))
	{
		// To generalize/improve in the future if we open up the custom printing
		// device interface :o)
		static MCCustomPrinterCreateProc s_revpdfprinter_create = nil;
        
        static MCSAutoLibraryRef s_revpdfprinter;
		if (!s_revpdfprinter.IsSet())
		{
            &s_revpdfprinter = MCU_library_load(MCSTR("./revpdfprinter"));
            
			if (s_revpdfprinter.IsSet())
			{
				s_revpdfprinter_create = (MCCustomPrinterCreateProc)MCU_library_lookup(*s_revpdfprinter,
                                                                                       MCSTR("MCCustomPrinterCreate"));
			}
		}

		if (s_revpdfprinter_create != nil)
			t_device = s_revpdfprinter_create();
		else
			t_device = nil;
	}
#ifdef _DEBUG
	else if (MCStringIsEqualToCString(p_destination, "debug", kMCCompareCaseless))
		t_device = new (nothrow) MCDebugPrintingDevice;
#endif

#ifdef _DEBUG
	if (t_device != nil)
		t_device = new (nothrow) MCLoggingPrintingDevice(t_device);
#endif
	
	if (t_device == nil)
    {
        return false;
    }

	MCAutoStringRef t_native_path;
	if (p_filename != nil)
		/* UNCHECKED */ MCS_pathtonative(p_filename, &t_native_path);

	MCCustomPrinter *t_printer;
	t_printer = new (nothrow) MCCustomPrinter(p_destination, t_device);
	t_printer -> Initialize();
	t_printer -> SetDeviceName(p_destination);
	t_printer -> SetDeviceOutput(PRINTER_OUTPUT_FILE, *t_native_path);
	t_printer -> SetDeviceOptions(p_options);
	t_printer -> SetPageSize(MCprinter -> GetPageWidth(), MCprinter -> GetPageHeight());
	t_printer -> SetPageOrientation(MCprinter -> GetPageOrientation());
	t_printer -> SetPageMargins(MCprinter -> GetPageLeftMargin(), MCprinter -> GetPageTopMargin(), MCprinter -> GetPageRightMargin(), MCprinter -> GetPageBottomMargin());
	t_printer -> SetPageScale(MCprinter -> GetPageScale());
	t_printer -> SetLayoutShowBorders(MCprinter -> GetLayoutShowBorders());
	t_printer -> SetLayoutSpacing(MCprinter -> GetLayoutRowSpacing(), MCprinter -> GetLayoutColumnSpacing());
	t_printer -> SetLayoutRowsFirst(MCprinter -> GetLayoutRowsFirst());
	t_printer -> SetLayoutScale(MCprinter -> GetLayoutScale());
	
	// MW-2010-09-27: [[ Bug 8999 ]] Make sure the device rectangle reflects the page rectangle.
	t_printer -> SetDeviceRectangle(t_printer -> GetPageRectangle());

	r_printer = t_printer;

	return true;
}
