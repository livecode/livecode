/* Copyright (C) 2018 LiveCode Ltd.

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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

#include <cairo-ft.h>
#include <time.h>

static cairo_user_data_key_t s_ft_face_key;

struct ft_face_user_data_t
{
	FT_Face face;
	FT_Size size;
	FT_Size old_size;
};

static void free_ft_face_func(void* p_face)
{
	ft_face_user_data_t *t_data;
	t_data = (ft_face_user_data_t*)p_face;
	
	if (t_data->old_size != nil)
		FT_Activate_Size(t_data->old_size);
	
	if (t_data->size != nil)
		FT_Done_Size(t_data->size);
	
	if (t_data->face != nil)
		FT_Done_Face(t_data->face);

	MCMemoryDelete(t_data);
}

bool MCPDFPrintingDevice::create_cairo_font_from_custom_printer_font(const MCCustomPrinterFont &p_cp_font, cairo_font_face_t* &r_cairo_font)
{
	bool t_success;
	t_success = true;
	
	ft_face_user_data_t *t_user_data;
	t_user_data = nil;
	if (t_success)
		t_success = MCMemoryNew(t_user_data);
	
	// Get the FreeType face
	if (t_success)
	{
		t_user_data->face = (FT_Face)p_cp_font.handle;
		t_success = t_user_data->face != nil;
	}
	
	// Retain a reference to the FreeType face
	if (t_success)
		FT_Reference_Face(t_user_data->face);

	// Store the old face size so it can be restored once cairo is finished
	if (t_success)
	{
		t_user_data->old_size = t_user_data->face->size;
		t_success = FT_Err_Ok == FT_New_Size(t_user_data->face, &t_user_data->size);
	}
	if (t_success)
		t_success = FT_Err_Ok == FT_Activate_Size(t_user_data->size);
	if (t_success)
		t_success = FT_Err_Ok == FT_Set_Char_Size(t_user_data->face, p_cp_font.size * 64, 0, 0, 0);
	
	bool t_free_user_data;
	t_free_user_data = true;
	
	// We can now create the cairo_font
	cairo_font_face_t *t_font;
	t_font = nil;
	if (t_success)
	{
		t_font = cairo_ft_font_face_create_for_ft_face(t_user_data->face, 0);
		t_success = t_font != nil && (m_status = cairo_font_face_status(t_font)) == CAIRO_STATUS_SUCCESS;
	}
	if (t_success)
	{
		// Finally, add a piece of user data to the font face. This will ensure
		// the FreeType font is restored to its original state and released when
		// the cairo font is destroyed.

		if (cairo_font_face_get_user_data(t_font, &s_ft_face_key) == nil)
		{
			m_status = cairo_font_face_set_user_data(t_font, &s_ft_face_key, t_user_data, free_ft_face_func);
			t_success = m_status == CAIRO_STATUS_SUCCESS;
			if (t_success)
				t_free_user_data = false;
		}
	}

	if (t_free_user_data && t_user_data != nil)
		free_ft_face_func(t_user_data);
	
	if (t_success)
		r_cairo_font = t_font;
	else
	{
		if (t_font != nil)
			cairo_font_face_destroy(t_font);
	}
	
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
