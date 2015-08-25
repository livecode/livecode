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

#include <cairo-win32.h>

#include <windows.h>

#include <cairo-pdf.h>

bool MCPDFPrintingDevice::create_cairo_font_from_custom_printer_font(const MCCustomPrinterFont &p_cp_font, cairo_font_face_t* &r_cairo_font)
{
	bool t_success = true;

	cairo_font_face_t *t_font;
	t_font = cairo_win32_font_face_create_for_hfont((HFONT)(p_cp_font.handle));
	t_success = (m_status = cairo_font_face_status(t_font)) == CAIRO_STATUS_SUCCESS;
	if (t_success)
		r_cairo_font = t_font;
	return t_success;
}

bool MCPDFPrintingDevice::set_cairo_pdf_datetime_to_now(cairo_pdf_datetime_t &r_datetime)
{
	SYSTEMTIME t_time;

	GetSystemTime(&t_time);

	r_datetime.year = t_time.wYear;
	r_datetime.month = t_time.wMonth;
	r_datetime.day = t_time.wDay;
	r_datetime.hour = t_time.wHour;
	r_datetime.minute = t_time.wMinute;
	r_datetime.second = t_time.wSecond;

	r_datetime.utc_minute_offset = 0;

	return true;
}
