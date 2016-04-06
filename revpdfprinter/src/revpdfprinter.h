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

#ifndef __REVPDFPRINTER_H__
#define __REVPDFPRINTER_H__

#ifndef CAIRO_H
#include <cairo.h>
#endif

#ifndef __MC_CORE__
#include "core.h"
#endif

#ifndef __MC_CUSTOM_PRINTER__
#include "customprinter.h"
#endif

#include <cairo-pdf.h>

////////////////////////////////////////////////////////////////////////////////

class MCPDFPrintingDevice: public MCCustomPrintingDevice
{
public:
	MCPDFPrintingDevice();
	virtual ~MCPDFPrintingDevice() {};

	void Destroy(void);

	const char *GetError(void);

	bool CanRenderPaint(const MCCustomPrinterPaint& paint);
	bool CanRenderImage(const MCCustomPrinterImage& image);
	bool CanRenderGroup(const MCCustomPrinterGroup& group);

	bool BeginDocument(const MCCustomPrinterDocument& document);
	void AbortDocument(void);
	bool EndDocument(void);

	bool BeginPage(const MCCustomPrinterPage& page);
	bool EndPage(void);

	bool BeginGroup(const MCCustomPrinterGroup& group);
	bool EndGroup(void);

	bool FillPath(const MCCustomPrinterPath& path, MCCustomPrinterFillRule rule, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip);
	bool StrokePath(const MCCustomPrinterPath& path, const MCCustomPrinterStroke& stroke, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip);
	bool DrawImage(const MCCustomPrinterImage& image, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip);
	bool DrawText(const MCCustomPrinterGlyph *glyphs, uint32_t glyph_count, const char *text_bytes, uint32_t text_byte_count, const uint32_t *clusters, const MCCustomPrinterFont& font, const MCCustomPrinterPaint& paint, const MCCustomPrinterTransform& transform, const MCCustomPrinterRectangle& clip);

	bool MakeAnchor(const MCCustomPrinterPoint& position, const char *name);
	bool MakeLink(const MCCustomPrinterRectangle& area, const char *link, MCCustomPrinterLinkType type);
	bool MakeBookmark(const MCCustomPrinterPoint& position, const char *title, uint32_t depth, bool closed);

private:
	bool apply_paint(const MCCustomPrinterPaint &p_paint);
	bool apply_stroke(const MCCustomPrinterStroke &p_stroke);
	bool draw_path(const MCCustomPrinterPath &p_path);
	bool create_surface_from_image(const MCCustomPrinterImage &p_image, cairo_surface_t* &r_surface, bool p_exclude_alpha = true, bool p_premultiply = true);
	bool create_mask_surface_from_image(const MCCustomPrinterImage &p_image, cairo_surface_t* &r_surface);
	bool create_cairo_font_from_custom_printer_font(const MCCustomPrinterFont &p_cp_font, cairo_font_face_t* &r_cairo_font);
	bool set_cairo_pdf_datetime_to_now(cairo_pdf_datetime_t &r_datetime);
	bool get_filename(const char* p_utf8_path, char *& r_system_path);

private:
	struct FontCache
	{
		FontCache *next;
		void *handle;
		cairo_font_face_t *cairo_font;
	};

	struct DestCache
	{
		DestCache *next;
		char *name;
		bool is_referenced;
		bool is_defined;
		bool is_url;
	};

	bool get_cached_destination(DestCache *p_head, const char *p_name, DestCache *&r_dest);
	bool new_cached_destination(DestCache *&x_head, const char *p_name, DestCache *&r_dest);
	
	cairo_surface_t *	m_surface;
	cairo_t *			m_context;
	cairo_status_t		m_status;
	char *				m_filename;
	
	FontCache *m_fonts;

	DestCache *m_destinations;

	uint32_t			m_option_count;
	char **		m_option_keys;
	char **		m_option_values;

	uint32_t m_bookmark_depth;
};

#endif
