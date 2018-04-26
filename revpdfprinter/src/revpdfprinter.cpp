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

#include "revpdfprinter.h"

#include <cairo-pdf.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

////////////////////////////////////////////////////////////////////////////////

static cairo_user_data_key_t s_group_data_key;
static cairo_user_data_key_t s_image_data_key;

bool custom_printer_blend_to_cairo_operator(MCCustomPrinterBlendMode p_blend, cairo_operator_t &r_operator);
bool custom_printer_glyphs_to_cairo_glyphs(const MCCustomPrinterGlyph *p_cp_glyphs, uint32_t p_glyph_count,  cairo_glyph_t* &r_cairo_glyphs);
bool custom_printer_clusters_to_cairo_clusters(const uint32_t *p_cp_clusters, uint32_t p_count, uint32_t p_glyph_count, cairo_text_cluster_t*& r_clusters, uint32_t& r_cluster_count, bool& r_reverse_clusters);
void init_matrix(cairo_matrix_t &r_matrix, const MCCustomPrinterTransform &p_transform);

typedef struct
{
	cairo_operator_t m_operation;
	double m_opacity;
} group_user_data;

typedef struct
{
	const MCCustomPrinterImage *m_image;
	uint32_t m_offset;
} image_stream_data;

typedef struct
{
	MCCustomPrinterBlendMode blend;
	cairo_operator_t op;
} blend_to_op;

static blend_to_op blend_to_op_lt[] =
{
	{kMCCustomPrinterBlendSrc,			CAIRO_OPERATOR_SOURCE},
	{kMCCustomPrinterBlendDst,			CAIRO_OPERATOR_DEST},
	{kMCCustomPrinterBlendSrcOver,		CAIRO_OPERATOR_OVER},
	{kMCCustomPrinterBlendDstOver,		CAIRO_OPERATOR_DEST_OVER},
	{kMCCustomPrinterBlendSrcIn,		CAIRO_OPERATOR_IN},
	{kMCCustomPrinterBlendDstIn,		CAIRO_OPERATOR_DEST_IN},
	{kMCCustomPrinterBlendSrcOut,		CAIRO_OPERATOR_OUT},
	{kMCCustomPrinterBlendDstOut,		CAIRO_OPERATOR_COLOR_DODGE},
	{kMCCustomPrinterBlendSrcAtop,		CAIRO_OPERATOR_ATOP},
	{kMCCustomPrinterBlendDstAtop,		CAIRO_OPERATOR_DEST_ATOP},
	{kMCCustomPrinterBlendXor,			CAIRO_OPERATOR_XOR},
	{kMCCustomPrinterBlendPlus,			CAIRO_OPERATOR_ADD},
	{kMCCustomPrinterBlendMultiply,		CAIRO_OPERATOR_MULTIPLY},
	{kMCCustomPrinterBlendScreen,		CAIRO_OPERATOR_SCREEN},
	{kMCCustomPrinterBlendOverlay,		CAIRO_OPERATOR_OVERLAY},
	{kMCCustomPrinterBlendDarken,		CAIRO_OPERATOR_DARKEN},
	{kMCCustomPrinterBlendLighten,		CAIRO_OPERATOR_LIGHTEN},
	{kMCCustomPrinterBlendDodge,		CAIRO_OPERATOR_COLOR_DODGE},
	{kMCCustomPrinterBlendBurn,			CAIRO_OPERATOR_COLOR_BURN},
	{kMCCustomPrinterBlendHardLight,	CAIRO_OPERATOR_HARD_LIGHT},
	{kMCCustomPrinterBlendSoftLight,	CAIRO_OPERATOR_SOFT_LIGHT},
	{kMCCustomPrinterBlendDifference,	CAIRO_OPERATOR_DIFFERENCE},
	{kMCCustomPrinterBlendExclusion,	CAIRO_OPERATOR_EXCLUSION},
};

static const char *s_known_did_keys[] =
{
	"Title",
	"Author",
	"Subject",
	"Keywords",
	"Creator",
	"Producer",
	""
};

#ifdef _WINDOWS
#define PACKED_INLINE __forceinline
#else
#define PACKED_INLINE inline
#endif

PACKED_INLINE uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

////////////////////////////////////////////////////////////////////////////////

MCPDFPrintingDevice::MCPDFPrintingDevice()
{
	m_surface = nil;
	m_context = nil;
	m_status = CAIRO_STATUS_SUCCESS;
	m_filename = nil;
	
	// MW-2009-12-23: Font cache list
	m_fonts = nil;

	m_destinations = nil;

	m_option_count = 0;
	m_option_keys = nil;
	m_option_values = nil;

	m_bookmark_depth = 1;
}

//////////

void MCPDFPrintingDevice::Destroy(void)
{
	// MW-2009-12-23: Free the font cache list
	while(m_fonts != nil)
	{
		FontCache *t_font;
		t_font = MCListPopFront(m_fonts);
		cairo_font_face_destroy(t_font -> cairo_font);
		MCMemoryDelete(t_font);
	}
	
	while (m_destinations != nil)
	{
		DestCache *t_dest;
		t_dest = MCListPopFront(m_destinations);
		MCCStringFree(t_dest->name);
		MCMemoryDelete(t_dest);
	}

	if (m_context != nil)
	{
		cairo_destroy(m_context);
		m_context = nil;
	}

	if (m_surface != nil)
	{
		cairo_surface_destroy(m_surface);
		// TODO: Remove the file that could have been created - will need to
		//   keep the filename around in 'BeginDocument' to do that.
	}

	if (m_filename != nil)
	{
		MCCStringFree(m_filename);
		m_filename = nil;
	}

	if (m_option_keys != nil)
	{
		for (uint32_t i = 0; i < m_option_count; i++)
			MCCStringFree(m_option_keys[i]);
		MCMemoryDeleteArray(m_option_keys);
	}

	if (m_option_values != nil)
	{
		for (uint32_t i = 0; i < m_option_count; i++)
			MCCStringFree(m_option_values[i]);
		MCMemoryDeleteArray(m_option_values);
	}

	// Reset cairo to its virgin state - we need to do this to ensure that the
	// internal font map that cairo has gets emptied thus unlocking all our
	// PangoFcFont's FT_Faces.
	cairo_debug_reset_static_data();

	delete this;
}

//////////

const char *MCPDFPrintingDevice::GetError(void)
{
	if (m_status == CAIRO_STATUS_SUCCESS)
		return nil;
	else
		return cairo_status_to_string(m_status);
}

//////////

bool MCPDFPrintingDevice::CanRenderPaint(const MCCustomPrinterPaint& p_paint)
{
	switch (p_paint.type)
	{
	case kMCCustomPrinterPaintSolid:
		return true;
		break;
	case kMCCustomPrinterPaintPattern:
		return CanRenderImage(p_paint.pattern.image);
		break;
	case kMCCustomPrinterPaintGradient:
		return p_paint.gradient.type == kMCCustomPrinterGradientLinear ||
			p_paint.gradient.type == kMCCustomPrinterGradientRadial;
		break;
	case kMCCustomPrinterPaintNone:
	default:
		return false;
	}
}

bool MCPDFPrintingDevice::CanRenderImage(const MCCustomPrinterImage& p_image)
{
	return p_image.type == kMCCustomPrinterImageJPEG ||
		p_image.type == kMCCustomPrinterImageRawARGB ||
		p_image.type == kMCCustomPrinterImageRawMRGB ||
		p_image.type == kMCCustomPrinterImageRawXRGB;
}

bool MCPDFPrintingDevice::CanRenderGroup(const MCCustomPrinterGroup& p_group)
{
	return true;
}

//////////

bool MCPDFPrintingDevice::BeginDocument(const MCCustomPrinterDocument& p_document)
{
	bool t_success = true;

    // SN-2014-12-22: [[ Bug 14278 ]] p_document.filename is now a UTF-8 string.
	t_success = get_filename(p_document.filename, m_filename);

	if (t_success)
	{
		// Create a surface with a large area to begin with. Failure to do this seems to
		// cause cairo to clip the first page to the size specified here, rather than
		// that set by the 'set_page_size' call that occurs subsequently.
		m_surface = cairo_pdf_surface_create(m_filename, 1000.0 * 72.0, 1000.0 * 72.0);
		m_status = cairo_surface_status(m_surface);
		if (m_status != CAIRO_STATUS_SUCCESS)
			t_success = false;
	}

	if (t_success)
	{
		cairo_pdf_value_t t_value;
		t_value.type = CAIRO_PDF_VALUE_TYPE_STRING;
		if (p_document.option_count > 0)
		{
			t_success = MCMemoryNewArray(p_document.option_count, m_option_keys);
			if (t_success)
				t_success = MCMemoryNewArray(p_document.option_count, m_option_values);
			if (t_success)
			{
				m_option_count = p_document.option_count;
				for (uint32_t i = 0; i < m_option_count && t_success; i++)
				{
					// MW-2011-01-28: [[ Bug 9311 ]] Make the DID keys auto-casing for ones we know about.
					const char *t_key;
					t_key = p_document . option_keys[i];
					for(uint32_t j = 0; s_known_did_keys[j] != nil; j++)
						if (MCCStringEqualCaseless(p_document . option_keys[i], s_known_did_keys[j]))
						{
							t_key = s_known_did_keys[j];
							break;
						}
					
					t_success = MCCStringClone(t_key, m_option_keys[i]) &&
								MCCStringClone(p_document.option_values[i], m_option_values[i]);
					if (t_success)
					{
						t_value.string.data = m_option_values[i];
						t_value.string.length = MCCStringLength(m_option_values[i]);
						cairo_pdf_surface_set_metadata(m_surface, m_option_keys[i], &t_value);
					}
				}
			}
		}
	}

	if (t_success)
	{
		cairo_pdf_value_t t_date_object;
		t_date_object.type = CAIRO_PDF_VALUE_TYPE_DATE;
		t_success = set_cairo_pdf_datetime_to_now(t_date_object.date);
		if (t_success)
			cairo_pdf_surface_set_metadata(m_surface, "CreationDate", &t_date_object);
	}

	if (t_success)
		t_success = (m_status = cairo_surface_status(m_surface)) == CAIRO_STATUS_SUCCESS;

	if (t_success)
	{
		cairo_pdf_surface_set_premultiplied_alpha(m_surface, false);
		m_context = cairo_create(m_surface);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	return t_success;
}

void MCPDFPrintingDevice::AbortDocument(void)
{
	// Do nothing on abort since the next thing called will just be 'Destroy'.
}

bool MCPDFPrintingDevice::EndDocument(void)
{
	bool t_success = true;
	// Destroy the graphics context associated with the PDF surface
	cairo_destroy(m_context);
	m_context = nil;

	// call finish on the surface explicitly so we can query the status
	// (the PDF surface may attempt to write to the file on finish)
	cairo_surface_finish(m_surface);
	t_success = (m_status = cairo_surface_status(m_surface)) == CAIRO_STATUS_SUCCESS;

	// Destroy the surface and set it to nil - this will stop the default
	// behavior in 'Destroy' of deleting the file.
	cairo_surface_destroy(m_surface);
	m_surface = nil;

	return t_success;
}

/////////

bool MCPDFPrintingDevice::BeginPage(const MCCustomPrinterPage& p_page)
{
	bool t_success = true;
	cairo_pdf_surface_set_size(m_surface, p_page.width, p_page.height);

	t_success = (m_status = cairo_surface_status(m_surface)) == CAIRO_STATUS_SUCCESS;
	return t_success;
}

bool MCPDFPrintingDevice::EndPage(void)
{
	bool t_success = true;
	cairo_surface_show_page(m_surface);

	t_success = (m_status = cairo_surface_status(m_surface)) == CAIRO_STATUS_SUCCESS;
	return t_success;
}

//////////
void deallocation_callback(void *p_data)
{
	MCMemoryDeallocate(p_data);
}

bool MCPDFPrintingDevice::BeginGroup(const MCCustomPrinterGroup& p_group)
{
	bool t_success = true;

	// we need to save the blend mode & opacity, then when the group ends, use
	// those values to paint the created pattern object
	cairo_push_group(m_context);

	cairo_rectangle(m_context, p_group.region.left, p_group.region.top,
		p_group.region.right - p_group.region.left, p_group.region.bottom - p_group.region.top);
	cairo_clip(m_context);

	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;

	group_user_data *t_group_data = nil;
	if (t_success)
		t_success = MCMemoryNew<group_user_data>(t_group_data);

	if (t_success)
	{
		custom_printer_blend_to_cairo_operator(p_group.blend_mode, t_group_data->m_operation);
		t_group_data->m_opacity = p_group.opacity;

		cairo_set_user_data(m_context, &s_group_data_key, (void*) t_group_data, deallocation_callback);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
		if (!t_success)
			MCMemoryDeallocate(t_group_data);
	}

	return t_success;
}

bool MCPDFPrintingDevice::EndGroup(void)
{
	bool t_success = true;
	group_user_data *t_group_data;
	t_group_data = (group_user_data*) cairo_get_user_data(m_context, &s_group_data_key);

	t_success = t_group_data != nil;

	if (t_success)
	{
		cairo_pop_group_to_source(m_context);

		cairo_save(m_context);
		cairo_set_operator(m_context, t_group_data->m_operation);
		cairo_paint_with_alpha(m_context, t_group_data->m_opacity);
		cairo_restore(m_context);

		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}
	return true;
}

/////////

bool MCPDFPrintingDevice::FillPath(const MCCustomPrinterPath& path, MCCustomPrinterFillRule rule, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip)
{
	bool t_success = true;
	cairo_save(m_context);
	cairo_set_fill_rule(m_context, rule == kMCCustomPrinterFillNonZero ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD);

	cairo_matrix_t t_transform;
	init_matrix(t_transform, transform);

	cairo_rectangle(m_context, clip.left, clip.top, clip.right - clip.left, clip.bottom - clip.top);
	cairo_clip(m_context);

	cairo_transform(m_context, &t_transform);

	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	if (t_success)
		t_success = apply_paint(paint);

	if (t_success)
		t_success = draw_path(path);

	if (t_success)
	{
		cairo_fill(m_context);
		cairo_restore(m_context);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}
	return t_success;
}

bool MCPDFPrintingDevice::StrokePath(const MCCustomPrinterPath& path, const MCCustomPrinterStroke& stroke, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip)
{
	bool t_success = true;
	cairo_save(m_context);

	cairo_matrix_t t_transform;
	init_matrix(t_transform, transform);

	cairo_rectangle(m_context, clip.left, clip.top, clip.right - clip.left, clip.bottom - clip.top);
	cairo_clip(m_context);

	cairo_transform(m_context, &t_transform);

	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	if (t_success)
		t_success = apply_paint(paint);
	if (t_success)
		t_success = apply_stroke(stroke);

	draw_path(path);

	if (t_success)
	{
		cairo_stroke(m_context);
		cairo_restore(m_context);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}
	return t_success;
}

bool MCPDFPrintingDevice::DrawImage(const MCCustomPrinterImage& image, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip)
{
	bool t_success;

	cairo_save(m_context);
	cairo_rectangle(m_context, clip.left, clip.top, clip.right - clip.left, clip.bottom - clip.top);
	cairo_clip(m_context);

	cairo_matrix_t t_matrix;
	init_matrix(t_matrix, transform);

	cairo_transform(m_context, &t_matrix);

	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;

	cairo_surface_t *t_img_surface = nil;
	cairo_surface_t *t_mask_surface = nil;
	if (t_success)
		t_success = create_surface_from_image(image, t_img_surface, false, false);

	//if (t_success && image.type == kMCCustomPrinterImageRawMRGB)
	//	t_success = create_mask_surface_from_image(image, t_mask_surface);

	if (t_success)
	{
		cairo_set_source_surface(m_context, t_img_surface, 0, 0);

		if (t_mask_surface != nil)
			cairo_mask_surface(m_context, t_mask_surface, 0, 0);
		else
			cairo_paint(m_context);

		cairo_restore(m_context);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	cairo_surface_destroy(t_img_surface);
	cairo_surface_destroy(t_mask_surface);

	return t_success;
}

bool MCPDFPrintingDevice::DrawText(const MCCustomPrinterGlyph *glyphs, uint32_t glyph_count, const char *text_bytes, uint32_t text_byte_count, const uint32_t *clusters, const MCCustomPrinterFont& font, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip)
{
	bool t_success = true;
	cairo_save(m_context);
	cairo_rectangle(m_context, clip.left, clip.top, clip.right - clip.left, clip.bottom - clip.top);
	cairo_clip(m_context);

	cairo_matrix_t t_matrix;
	init_matrix(t_matrix, transform);
	cairo_transform(m_context, &t_matrix);

	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	
	// MW-2009-12-23: Check the font cache for the font first - probably should be refactored
	//   into separate routine :o)
	cairo_font_face_t *t_font = nil;
	if (t_success)
	{
		for(FontCache *t_cached_font = m_fonts; t_cached_font != nil; t_cached_font = t_cached_font -> next)
			if (t_cached_font -> handle == font . handle)
			{
				t_font = t_cached_font -> cairo_font;
				break;
			}
		
		if (t_font == nil)
		{
			FontCache *t_cached_font;
			t_success = MCMemoryNew(t_cached_font);
			
			if (t_success)
				t_success = create_cairo_font_from_custom_printer_font(font, t_font);
			
			if (t_success)
			{
				t_cached_font -> handle = font . handle;
				t_cached_font -> cairo_font = t_font;
				
				MCListPushFront(m_fonts, t_cached_font);
			}
			else
				MCMemoryDelete(t_cached_font);
		}
	}

	cairo_glyph_t *t_glyphs = nil;
	if (t_success)
		t_success = custom_printer_glyphs_to_cairo_glyphs(glyphs, glyph_count, t_glyphs);

	cairo_text_cluster_t *t_clusters = nil;
	uint32_t t_cluster_count;
	bool t_reverse_clusters;
	if (t_success)
		t_success = custom_printer_clusters_to_cairo_clusters(clusters, text_byte_count, glyph_count, t_clusters, t_cluster_count, t_reverse_clusters);
	
	if (t_success)
		t_success = apply_paint(paint);
	
	if (t_success)
	{
		cairo_set_font_face(m_context, t_font);
		cairo_set_font_size(m_context, font . size);

		cairo_show_text_glyphs(m_context, text_bytes, text_byte_count, t_glyphs, glyph_count, t_clusters, t_cluster_count, t_reverse_clusters ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : (cairo_text_cluster_flags_t)0);
		cairo_restore(m_context);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	MCMemoryDeleteArray(t_clusters);
	MCMemoryDeleteArray(t_glyphs);

	return t_success;
}

/////////

static bool dest_is_url(const char *p_dest)
{
	const char *t_ptr = p_dest;

	while (*t_ptr != '\0')
	{
		if (*t_ptr == ':')
			return t_ptr != p_dest;

		if ((*t_ptr < 'a' || *t_ptr > 'z') && (*t_ptr < 'A' || *t_ptr > 'Z'))
			return false;
		t_ptr++;
	}

	return false;
}

bool MCPDFPrintingDevice::get_cached_destination(DestCache *p_head, const char *p_name, DestCache *&r_dest)
{
	DestCache *t_dest = nil;
	for (DestCache *t_cached_dest = p_head; t_cached_dest != nil; t_cached_dest = t_cached_dest -> next)
	{
		if (MCCStringEqualCaseless(t_cached_dest -> name, p_name))
		{
			t_dest = t_cached_dest;
			break;
		}
	}

	if (t_dest != nil)
		r_dest = t_dest;

	return (t_dest != nil);
}

bool MCPDFPrintingDevice::new_cached_destination(DestCache *&x_head, const char *p_name, DestCache *&r_dest)
{
	bool t_success;
	DestCache *t_cached_dest;
	t_success = MCMemoryNew(t_cached_dest);
	
	if (t_success)
		t_success = MCCStringClone(p_name, t_cached_dest->name);
	
	if (t_success)
	{
		t_cached_dest -> is_defined = false;
		t_cached_dest -> is_referenced = false;
		
		t_cached_dest->is_url = dest_is_url(t_cached_dest->name);
		MCListPushFront(x_head, t_cached_dest);

		r_dest = t_cached_dest;
	}
	else
		MCMemoryDelete(t_cached_dest);

	return t_success;
}

bool MCPDFPrintingDevice::MakeAnchor(const MCCustomPrinterPoint& position, const char *name)
{
	bool t_success = true;

	DestCache *t_dest = nil;
		
	if (!get_cached_destination(m_destinations, name, t_dest))
	{
		t_success = new_cached_destination(m_destinations, name, t_dest);
	}

	if (t_success)
	{
		t_dest -> is_defined = true;

		cairo_pdf_surface_add_destination(m_surface, t_dest->name, position.x, position.y);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	return t_success;
}

bool MCPDFPrintingDevice::MakeLink(const MCCustomPrinterRectangle& area, const char *link, MCCustomPrinterLinkType p_type)
{
	DestCache *t_dest = nil;
	bool t_success = true;

	if (!get_cached_destination(m_destinations, link, t_dest))
	{
		t_success = new_cached_destination(m_destinations, link, t_dest);
	}

	if (t_success)
	{
		t_dest -> is_referenced = true;

		cairo_rectangle_t t_rect;
		t_rect.x = area.left;
		t_rect.y = area.top;
		t_rect.width = area.right - area.left;
		t_rect.height = area.bottom - area.top;

		if (p_type == kMCCustomPrinterLinkUnspecified)
			p_type = t_dest->is_url ? kMCCustomPrinterLinkURI : kMCCustomPrinterLinkAnchor;
		if (p_type == kMCCustomPrinterLinkURI)
			cairo_pdf_surface_add_uri_link(m_surface, t_rect, t_dest->name);
		else
			cairo_pdf_surface_add_goto_link(m_surface, t_rect, t_dest->name);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	return t_success;
}

bool MCPDFPrintingDevice::MakeBookmark(const MCCustomPrinterPoint &position, const char *title, uint32_t depth, bool closed)
{
	bool t_success = true;

	if (depth == 0)
		depth = m_bookmark_depth;
	else
		m_bookmark_depth = depth;

	cairo_pdf_surface_add_outline_entry(m_surface, title, position.x, position.y, depth, closed);
	t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
extern "C" MCCustomPrintingDevice *MCCustomPrinterCreate(void) __attribute__((visibility("default")));
#endif

extern "C" MCCustomPrintingDevice _EXPORT *MCCustomPrinterCreate(void)
{
	return new MCPDFPrintingDevice;
}

////////////////////////////////////////////////////////////////////////////////

void transform_point(double &x, double &y, const MCCustomPrinterTransform &p_transform)
{
	double t_x;
	t_x = p_transform.scale_x * x + p_transform.skew_x * y + p_transform.translate_x;
	y = p_transform.skew_y * x + p_transform.scale_y * y + p_transform.translate_y;
	x = t_x;
}

// MW-2014-08-19: [[ Bug 13220 ]] It seems Cairo doesn't like (M p) (L _)* (L p) (C)
//   as it ends up treating it is as a degenerate point. Therefore we clean up this
//   case - if there is a lineTo which returns to the original moveTo then a close
//   it ignores the final lineTo.
bool MCPDFPrintingDevice::draw_path(const MCCustomPrinterPath &p_path)
{
	bool t_success = true;
	
	MCCustomPrinterPathCommand *t_commands = p_path.commands;
	MCCustomPrinterPoint *t_points = p_path.coords;

    double t_first_x, t_first_y;
    t_first_x = t_first_y = DBL_MAX;
    
    double t_last_x, t_last_y;
    t_last_x = t_last_y = DBL_MAX;
    
	while (t_success && *t_commands != kMCCustomPrinterPathEnd)
	{
		switch (*t_commands++)
		{
		case kMCCustomPrinterPathMoveTo:
			{
				cairo_move_to(m_context, t_points->x, t_points->y);
                t_first_x = t_points -> x;
                t_first_y = t_points -> y;
				t_points++;
			}
			break;
		case kMCCustomPrinterPathLineTo:
			{
                if (t_last_x != t_points -> x || t_last_y != t_points -> y)
                {
                    if (*t_commands != kMCCustomPrinterPathClose ||
                        (t_first_x != t_points -> x || t_first_y != t_points -> y))
                    {
                        cairo_line_to(m_context, t_points->x, t_points->y);
                        t_last_x = t_points -> x;
                        t_last_y = t_points -> y;
                    }
                }
				t_points++;
			}
			break;
		case kMCCustomPrinterPathClose:
			cairo_close_path(m_context);
			break;
		case kMCCustomPrinterPathQuadraticTo:
			//TODO implement this
			t_success = false;
			break;
		case kMCCustomPrinterPathCubicTo:
			{
				cairo_curve_to(m_context,
					t_points[0].x, t_points[0].y,
					t_points[1].x, t_points[1].y,
					t_points[2].x, t_points[2].y);
				t_points += 3;
			}
			break;
		case kMCCustomPrinterPathEnd:
			/* Shouldn't ever be reached */
			abort();
		}
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
	}

	return t_success;
}

bool MCPDFPrintingDevice::apply_paint(const MCCustomPrinterPaint &p_paint)
{
	cairo_pattern_t *t_paint_pattern = nil;
	cairo_matrix_t t_transform;
	cairo_extend_t t_extend_mode = CAIRO_EXTEND_NONE;
	bool t_success = true;

	switch (p_paint.type)
	{
	case kMCCustomPrinterPaintSolid:
		cairo_set_source_rgb(m_context, p_paint.solid.red, p_paint.solid.green, p_paint.solid.blue);
		t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
		break;
	case kMCCustomPrinterPaintPattern:
		{
			cairo_surface_t *t_surface;
			t_success = create_surface_from_image(p_paint.pattern.image, t_surface, true, false);
			if (t_success)
			{
				t_paint_pattern = cairo_pattern_create_for_surface(t_surface);
				cairo_surface_destroy(t_surface);
				t_success = (m_status = cairo_pattern_status(t_paint_pattern)) == CAIRO_STATUS_SUCCESS;
				if (!t_success)
					cairo_pattern_destroy(t_paint_pattern);
			}
			init_matrix(t_transform, p_paint.pattern.transform);
			t_extend_mode = CAIRO_EXTEND_REPEAT;
		}
		break;
	case kMCCustomPrinterPaintGradient:
		{
			double t_extent = 1.0;

			t_extend_mode = CAIRO_EXTEND_PAD;
			if (p_paint.gradient.wrap)
			{
				t_extent /= p_paint.gradient.repeat;
				if (p_paint.gradient.mirror)
					t_extend_mode = CAIRO_EXTEND_REFLECT;
				else
					t_extend_mode = CAIRO_EXTEND_REPEAT;
			}

			switch (p_paint.gradient.type)
			{
			case kMCCustomPrinterGradientLinear:
				t_paint_pattern = cairo_pattern_create_linear(0, 0, t_extent, 0);
				break;
			case kMCCustomPrinterGradientRadial:
				t_paint_pattern = cairo_pattern_create_radial(0, 0, 0, 0, 0, t_extent);
				break;
			default:
				break;
			}
			if (p_paint.gradient.repeat > 1 && !p_paint.gradient.wrap)
			{
				bool t_reverse = false;
				for (uint32_t r=0; r<p_paint.gradient.repeat; r++)
				{
					int32_t t_stops, t_direction, t_index, t_base_offset;
					t_stops = p_paint.gradient.stop_count;
					if (!t_reverse)
					{
						t_index = 0;
						t_base_offset = 0;
						t_direction = 1;
					}
					else
					{
						t_index = t_stops - 1;
						t_base_offset = 1;
						t_direction = -1;
					}
					// we need to ensure that gradient stops start at 0 and end at 1
					if (p_paint.gradient.stops[t_index].offset != t_base_offset)
					{
						if (r == 0 || !p_paint.gradient.mirror)
						{
							cairo_pattern_add_color_stop_rgba(t_paint_pattern, (double)r / p_paint.gradient.repeat,
								p_paint.gradient.stops[t_index].color.red,
								p_paint.gradient.stops[t_index].color.green,
								p_paint.gradient.stops[t_index].color.blue,
								p_paint.gradient.stops[t_index].alpha);
						}
					}
					else
					{
						// skip first stop if following reversed stop list
						if (r > 0 && p_paint.gradient.mirror)
						{
							t_index += t_direction;
							t_stops -= 1;
						}
					}
					while (t_stops--)
					{
						double t_offset = (r + t_base_offset + p_paint.gradient.stops[t_index].offset * t_direction) / p_paint.gradient.repeat;
						cairo_pattern_add_color_stop_rgba(t_paint_pattern, t_offset,
							p_paint.gradient.stops[t_index].color.red,
							p_paint.gradient.stops[t_index].color.green,
							p_paint.gradient.stops[t_index].color.blue,
							p_paint.gradient.stops[t_index].alpha);
						t_index += t_direction;
					}
					if (p_paint.gradient.stops[t_index - t_direction].offset != 1 - t_base_offset)
					{
						double t_offset = (double)(r + 1) / p_paint.gradient.repeat;
						cairo_pattern_add_color_stop_rgba(t_paint_pattern, t_offset,
							p_paint.gradient.stops[t_index - t_direction].color.red,
							p_paint.gradient.stops[t_index - t_direction].color.green,
							p_paint.gradient.stops[t_index - t_direction].color.blue,
							p_paint.gradient.stops[t_index - t_direction].alpha);
					}
					if (p_paint.gradient.mirror)
						t_reverse = !t_reverse;
				}
			}
			else
			{
				if (p_paint.gradient.stops[0].offset != 0.0)
				{
					cairo_pattern_add_color_stop_rgba(t_paint_pattern, 0.0,
						p_paint.gradient.stops[0].color.red, p_paint.gradient.stops[0].color.green, p_paint.gradient.stops[0].color.blue,
						p_paint.gradient.stops[0].alpha);
				}
				for (uint32_t i=0; i<p_paint.gradient.stop_count; i++)
				{
					cairo_pattern_add_color_stop_rgba(t_paint_pattern, p_paint.gradient.stops[i].offset,
						p_paint.gradient.stops[i].color.red, p_paint.gradient.stops[i].color.green, p_paint.gradient.stops[i].color.blue,
						p_paint.gradient.stops[i].alpha);
				}
				if (p_paint.gradient.stops[p_paint.gradient.stop_count - 1].offset != 1.0)
				{
					cairo_pattern_add_color_stop_rgba(t_paint_pattern, 1.0,
						p_paint.gradient.stops[p_paint.gradient.stop_count - 1].color.red,
						p_paint.gradient.stops[p_paint.gradient.stop_count - 1].color.green,
						p_paint.gradient.stops[p_paint.gradient.stop_count - 1].color.blue,
						p_paint.gradient.stops[p_paint.gradient.stop_count - 1].alpha);
				}
			}

			init_matrix(t_transform, p_paint.gradient.transform);
		}
		break;
	case kMCCustomPrinterPaintNone:
		break;
	}
	if (t_paint_pattern != nil && t_success)
	{
		t_success = (m_status = cairo_matrix_invert(&t_transform)) == CAIRO_STATUS_SUCCESS;
		if (t_success)
		{
			cairo_pattern_set_matrix(t_paint_pattern, &t_transform);
			cairo_pattern_set_extend(t_paint_pattern, t_extend_mode);

			t_success = (m_status = cairo_pattern_status(t_paint_pattern)) == CAIRO_STATUS_SUCCESS;
		}
		if (t_success)
		{
			cairo_set_source(m_context, t_paint_pattern);
			t_success = (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
		}
		cairo_pattern_destroy(t_paint_pattern);
	}

	return t_success;
}

bool MCPDFPrintingDevice::apply_stroke(const MCCustomPrinterStroke &p_stroke)
{
	cairo_set_line_width(m_context, p_stroke.thickness);
	switch (p_stroke.cap_style)
	{
	case kMCCustomPrinterCapButt:
		cairo_set_line_cap(m_context, CAIRO_LINE_CAP_BUTT);
		break;
	case kMCCustomPrinterCapRound:
		cairo_set_line_cap(m_context, CAIRO_LINE_CAP_ROUND);
		break;
	case kMCCustomPrinterCapSquare:
		cairo_set_line_cap(m_context, CAIRO_LINE_CAP_SQUARE);
		break;
	}

	switch (p_stroke.join_style)
	{
	case kMCCustomPrinterJoinBevel:
		cairo_set_line_join(m_context, CAIRO_LINE_JOIN_BEVEL);
		break;
	case kMCCustomPrinterJoinRound:
		cairo_set_line_join(m_context, CAIRO_LINE_JOIN_ROUND);
		break;
	case kMCCustomPrinterJoinMiter:
		cairo_set_line_join(m_context, CAIRO_LINE_JOIN_MITER);
		break;
	}

	cairo_set_miter_limit(m_context, p_stroke.miter_limit);

	cairo_set_dash(m_context, p_stroke.dashes, p_stroke.dash_count, p_stroke.dash_offset);
	return (m_status = cairo_status(m_context)) == CAIRO_STATUS_SUCCESS;
}

bool custom_printer_blend_to_cairo_operator(MCCustomPrinterBlendMode p_blend, cairo_operator_t &r_ref)
{
	for (uint32_t i=0; i<(sizeof(blend_to_op_lt) / sizeof(blend_to_op)); i++)
	{
		if (blend_to_op_lt[i].blend == p_blend)
		{
			r_ref = blend_to_op_lt[i].op;
			return true;
		}
	}
	return false;
}

cairo_status_t image_buffer_stream_read(void *p_image, unsigned char *p_data, unsigned int p_length)
{
	image_stream_data *t_image_data = (image_stream_data*)p_image;
	if (t_image_data->m_offset + p_length > t_image_data->m_image->data_size)
		return CAIRO_STATUS_READ_ERROR;

	MCMemoryCopy(p_data, (uint8_t*)t_image_data->m_image->data + t_image_data->m_offset, p_length);
	t_image_data->m_offset += p_length;
	return CAIRO_STATUS_SUCCESS;
}

bool MCPDFPrintingDevice::create_surface_from_image(const MCCustomPrinterImage &p_image, cairo_surface_t* &r_surface, bool p_exclude_alpha, bool p_premultiply)
{
	bool t_success = true;
	cairo_surface_t *t_image_surface = nil;
	switch (p_image.type)
	{
	// Raw 32-bit RGB image data, no mask
	case kMCCustomPrinterImageRawXRGB:
		t_image_surface = cairo_image_surface_create_for_data((unsigned char *)p_image.data, CAIRO_FORMAT_RGB24, p_image.width, p_image.height, p_image.data_size / p_image . height);
		break;

	// Raw 32-bit RGB image data, sharp mask
	case kMCCustomPrinterImageRawMRGB:
		//t_image_surface = cairo_image_surface_create_for_data((unsigned char *)p_image.data, CAIRO_FORMAT_RGB24, p_image.width, p_image.height, p_image.data_size / p_image . height);
		//break;
	// Raw 32-bit RGB image data, soft mask (unpremultiplied) 
	case kMCCustomPrinterImageRawARGB:
		if (p_exclude_alpha)
			t_image_surface = cairo_image_surface_create_for_data((unsigned char *)p_image.data, CAIRO_FORMAT_RGB24, p_image.width, p_image.height, p_image.data_size / p_image . height);
		else if (!p_premultiply)
			t_image_surface = cairo_image_surface_create_for_data((unsigned char *)p_image.data, CAIRO_FORMAT_ARGB32, p_image.width, p_image.height, p_image.data_size / p_image.height);
		else
		{
			uint32_t t_stride, t_data_size;
			t_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, p_image.width);
			t_data_size = p_image.height * t_stride;

			unsigned char *t_data;
			t_success = MCMemoryAllocate<unsigned char>(t_data_size, t_data);
			if (t_success)
			{
				for (uint32_t y = 0; y < p_image.height; y++)
				{
					uint32_t *t_in_ptr, *t_out_ptr;
					t_in_ptr = (uint32_t*) p_image.data + y * p_image.width;
					t_out_ptr = (uint32_t*) (t_data + y * t_stride);

					uint32_t t_width;
					t_width = p_image.width;

					while (t_width--)
					{
						uint32_t t_alpha;
						t_alpha = *t_in_ptr >> 24;
						*t_out_ptr++ = (packed_scale_bounded(*t_in_ptr++, t_alpha) & 0x00FFFFFF) | (t_alpha << 24);
					}
				}
				t_image_surface = cairo_image_surface_create_for_data(t_data, CAIRO_FORMAT_ARGB32, p_image.width, p_image.height, t_stride);
				cairo_surface_set_user_data(t_image_surface, &s_image_data_key, t_data, deallocation_callback);
				t_success = (m_status = cairo_surface_status(t_image_surface)) == CAIRO_STATUS_SUCCESS;
				if (!t_success)
					MCMemoryDeallocate(t_data);
			}
		}
		break;

	// GIF image data
	case kMCCustomPrinterImageGIF:
		t_success = false;
		break;

	// JPEG image data
	case kMCCustomPrinterImageJPEG:
		{
			t_image_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, p_image.width, p_image.height);

			// MW-2012-11-29: [[ Bug 10574 ]] Copy the JPEG data to pass to cairo as the pointer we get is only
			//   guaranteed to be valid for this invocation.
			void *t_data;
			if (MCMemoryAllocate(p_image . data_size, t_data))
			{
				MCMemoryCopy(t_data, p_image . data, p_image . data_size);
				cairo_surface_set_mime_data(t_image_surface, "image/jpeg", (uint8_t*)t_data, p_image.data_size, MCMemoryDeallocate, t_data);
			}
		}
		break;

	// PNG image data
	case kMCCustomPrinterImagePNG:
		t_success = false;
		break;
			
	// [[ Bug 12699 ]] Handle unrecognised image type
	default:
		t_success = false;
		break;
	}

	if (t_image_surface != nil)
	{
		m_status = cairo_surface_status(t_image_surface);
		if (m_status != CAIRO_STATUS_SUCCESS)
			t_success = false;
	}

	if (t_success)
		r_surface = t_image_surface;
	else
		cairo_surface_destroy(t_image_surface);

	return t_success;
}

bool MCPDFPrintingDevice::create_mask_surface_from_image(const MCCustomPrinterImage &p_image, cairo_surface_t* &r_surface)
{
	bool t_success = true;
	cairo_surface_t *t_image_surface = nil;
	switch (p_image.type)
	{
	// Raw 32-bit RGB image data, sharp mask
	case kMCCustomPrinterImageRawMRGB:
		{
			uint32_t t_stride, t_data_size;
			t_stride = cairo_format_stride_for_width(CAIRO_FORMAT_A1, p_image.width);
			t_data_size = p_image.height * t_stride;

			unsigned char *t_data;
			t_success = MCMemoryAllocate<unsigned char>(t_data_size, t_data);
			if (t_success)
			{
				for (uint32_t y = 0; y < p_image.height; y++)
				{
					uint32_t *t_in_ptr;
					uint8_t *t_out_ptr;
					t_in_ptr = (uint32_t*) p_image.data + y * (p_image.data_size / p_image . height) / 4;
					t_out_ptr = (uint8_t*) (t_data + y * t_stride);

					uint32_t t_bit = 0x01;
					uint32_t t_width;
					t_width = p_image.width;

					while (t_width--)
					{
						uint32_t t_alpha;
						t_alpha = *t_in_ptr++ >> 24;
						*t_out_ptr = t_alpha ? (*t_out_ptr | t_bit) : (*t_out_ptr & ~t_bit);
						t_bit <<=  1;
						if ((t_bit & 0xFF) == 0)
						{
							t_bit = 0x01;
							t_out_ptr++;
						}

					}
				}
				t_image_surface = cairo_image_surface_create_for_data(t_data, CAIRO_FORMAT_A1, p_image.width, p_image.height, t_stride);
				cairo_surface_set_user_data(t_image_surface, &s_image_data_key, t_data, deallocation_callback);
				t_success = (m_status = cairo_surface_status(t_image_surface)) == CAIRO_STATUS_SUCCESS;
				if (!t_success)
					MCMemoryDeallocate(t_data);
			}
		}
		break;
#if 1
	// Raw 32-bit RGB image data, soft mask (unpremultiplied) 
	case kMCCustomPrinterImageRawARGB:
		{
			uint32_t t_stride, t_data_size;
			t_stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, p_image.width);
			t_data_size = p_image.height * t_stride;

			unsigned char *t_data;
			t_success = MCMemoryAllocate<unsigned char>(t_data_size, t_data);
			if (t_success)
			{
				for (uint32_t y = 0; y < p_image.height; y++)
				{
					uint32_t *t_in_ptr;
					uint8_t *t_out_ptr;
					t_in_ptr = (uint32_t*) p_image.data + y * (p_image.data_size / p_image . height) / 4;
					t_out_ptr = (uint8_t*) (t_data + y * t_stride);

					uint32_t t_width;
					t_width = p_image.width;

					while (t_width--)
					{
						uint32_t t_alpha;
						t_alpha = *t_in_ptr++ >> 24;
						*t_out_ptr++ = t_alpha;
					}
				}
				t_image_surface = cairo_image_surface_create_for_data(t_data, CAIRO_FORMAT_A8, p_image.width, p_image.height, t_stride);
				cairo_surface_set_user_data(t_image_surface, &s_image_data_key, t_data, deallocation_callback);
				t_success = (m_status = cairo_surface_status(t_image_surface)) == CAIRO_STATUS_SUCCESS;
				if (!t_success)
					MCMemoryDeallocate(t_data);
			}
		}
		break;
#endif
	default:
		r_surface = nil;
		break;

	}

	if (t_success && t_image_surface != nil)
		t_success = (m_status = cairo_surface_status(t_image_surface)) == CAIRO_STATUS_SUCCESS;

	if (t_success)
		r_surface = t_image_surface;
	else
		cairo_surface_destroy(t_image_surface);

	return t_success;
}

void init_matrix(cairo_matrix_t &r_matrix, const MCCustomPrinterTransform &p_transform)
{
	cairo_matrix_init(&r_matrix, p_transform.scale_x, p_transform.skew_y,
		p_transform.skew_x, p_transform.scale_y,
		p_transform.translate_x, p_transform.translate_y);
}

bool custom_printer_glyphs_to_cairo_glyphs(const MCCustomPrinterGlyph *p_cp_glyphs, uint32_t p_glyph_count,  cairo_glyph_t* &r_cairo_glyphs)
{
	bool t_success = true;

	cairo_glyph_t *t_glyphs;
	t_success = MCMemoryNewArray<cairo_glyph_t>(p_glyph_count, t_glyphs);

	if (t_success)
	{
		for (uint32_t i = 0; i < p_glyph_count; i++)
		{
			t_glyphs[i].index = p_cp_glyphs[i].id;
			t_glyphs[i].x = p_cp_glyphs[i].x;
			t_glyphs[i].y = p_cp_glyphs[i].y;
		}
		r_cairo_glyphs = t_glyphs;
	}
	return t_success;
}

bool custom_printer_clusters_to_cairo_clusters(const uint32_t *p_cp_clusters, uint32_t p_count, uint32_t p_glyph_count, cairo_text_cluster_t*& r_clusters, uint32_t& r_cluster_count, bool& r_reverse_clusters)
{
	// Slightly different handling depending on whether it is an increasing
	// or decreasing sequence of clusters.
	bool t_reverse;
	t_reverse = false;
	
	// The number of cairo clusters is the number of changes in glyph index
	// in the cp clusters.
	uint32_t t_cluster_count;
	t_cluster_count = 1;
	for(uint32_t i = 1; i < p_count; i++)
		if (p_cp_clusters[i] != p_cp_clusters[i - 1])
		{
			if (p_cp_clusters[i] < p_cp_clusters[i - 1])
				t_reverse = true;
			t_cluster_count += 1;
		}
	
	cairo_text_cluster_t *t_clusters;
	t_clusters = nil;
	if (!MCMemoryNewArray(t_cluster_count, t_clusters))
		return false;
	
	uint32_t t_index, t_cluster;
	t_index = 0;
	t_cluster = 0;
	while(t_index < p_count)
	{
		uint32_t t_next_index;
		for(t_next_index = t_index; t_next_index < p_count; t_next_index++)
			if (p_cp_clusters[t_next_index] != p_cp_clusters[t_index])
				break;
		
		t_clusters[t_cluster] . num_bytes = t_next_index - t_index;
		if (!t_reverse)
		{
			if (t_next_index < p_count)
				t_clusters[t_cluster] . num_glyphs = p_cp_clusters[t_next_index] - p_cp_clusters[t_index];
			else 
				t_clusters[t_cluster] . num_glyphs = p_glyph_count - p_cp_clusters[t_index];
		}
		else
		{
			if (t_index == 0)
				t_clusters[t_cluster] . num_glyphs = p_glyph_count - p_cp_clusters[t_index];
			else
				t_clusters[t_cluster] . num_glyphs = p_cp_clusters[t_index - 1] - p_cp_clusters[t_index];

		}
		
		t_cluster += 1;
		t_index = t_next_index;
	}
	
	r_clusters = t_clusters;
	r_cluster_count = t_cluster_count;
	r_reverse_clusters = t_reverse;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// The static function export table for iOS external linkage requirements.

#ifdef TARGET_SUBPLATFORM_IPHONE
extern "C" {
	
struct LibExport
{
	const char *name;
	void *address;
};

struct LibInfo
{
	const char **name;
	struct LibExport *exports;
};

static const char *__libname = "revpdfprinter";

static struct LibExport __libexports[] =
{
	{ "MCCustomPrinterCreate", (void *)MCCustomPrinterCreate },
	{ 0, 0 }
};

struct LibInfo __libinfo =
{
	&__libname,
	__libexports
};

__attribute((section("__DATA,__libs"))) volatile struct LibInfo *__libinfoptr_revpdfprinter __attribute__((__visibility__("default"))) = &__libinfo;
}
#endif

////////////////////////////////////////////////////////////////////////////////
