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

#include <pango/pango.h>
#include <pango/pangoxft.h>
#include <cairo-ft.h>
#include <time.h>

static cairo_user_data_key_t kPangoFcFontRef;

static void destroy_pango_fc_font_ref(void *p_data)
{
	PangoFcFont *t_font;
	t_font = (PangoFcFont *)p_data;

	pango_fc_font_unlock_face(t_font);
	g_object_unref(t_font);
}

bool MCPDFPrintingDevice::create_cairo_font_from_custom_printer_font(const MCCustomPrinterFont &p_cp_font, cairo_font_face_t* &r_cairo_font)
{
	bool t_success = true;

	t_success = false;

	// Our font handle is a PangoFcFont which we retain a reference to so it can
	// be freed when cairo is finished with the FT_Face.
	PangoFcFont *t_pango_font;
	t_pango_font = (PangoFcFont *)p_cp_font . handle;
	g_object_ref(t_pango_font);

	// Now lock the FT face that Cairo really wants
	FT_Face t_face;
	t_face = pango_fc_font_lock_face(t_pango_font);

	// We can now create the cairo_font
	cairo_font_face_t *t_font;
	t_font = cairo_ft_font_face_create_for_ft_face(t_face, 0);

	// Finally, add a piece of user data to the font face. This is the PangoFcFont
	// which will be cleaned up by our destroy func. (If not already set - cairo does
	// some uniquing itself).
	if (cairo_font_face_get_user_data(t_font, &kPangoFcFontRef) == nil)
	{
		cairo_font_face_set_user_data(t_font, &kPangoFcFontRef, t_pango_font, destroy_pango_fc_font_ref);
	}
	else
	{
		pango_fc_font_unlock_face(t_pango_font);
		g_object_unref(t_pango_font);
	}

	// Check that everything succeeded.
	t_success = (m_status = cairo_font_face_status(t_font)) == CAIRO_STATUS_SUCCESS;

	if (t_success)
		r_cairo_font = t_font;
	
	return t_success;
}

bool MCPDFPrintingDevice::set_cairo_pdf_datetime_to_now(cairo_pdf_datetime_t &r_datetime)
{
	time_t t_time;
	tm * t_datetime;

	time ( &t_time );

	t_datetime = gmtime ( &t_time );
	
	r_datetime.year = t_datetime->tm_year + 1900;
	r_datetime.month = t_datetime->tm_mon + 1;
	r_datetime.day = t_datetime->tm_mday;
	r_datetime.hour = t_datetime->tm_hour;
	r_datetime.minute = t_datetime->tm_min;
	r_datetime.second = t_datetime->tm_sec;

	r_datetime.utc_minute_offset = 0;

	return true;
}

// SN-2014-12-23: [[ Bug 14278 ]] Added system-specific to get the path.
bool MCPDFPrintingDevice::get_filename(const char* p_utf8_path, char *& r_system_path)
{
	return MCCStringClone(p_utf8_path, r_system_path);
}
