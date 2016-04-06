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

#include <cairo-quartz.h>
#include <CoreFoundation/CFDate.h>

#ifdef __IPHONE_3_2
#include <CoreText/CoreText.h>
#endif

bool MCPDFPrintingDevice::create_cairo_font_from_custom_printer_font(const MCCustomPrinterFont &p_cp_font, cairo_font_face_t* &r_cairo_font)
{
#ifdef __IPHONE_3_2
	bool t_success = true;
	
	CGFontRef t_cg_font;
	t_cg_font = CTFontCopyGraphicsFont((CTFontRef)p_cp_font . handle, NULL);
	
	cairo_font_face_t *t_font;
	t_font = cairo_quartz_font_face_create_for_cgfont(t_cg_font);
	t_success = (m_status = cairo_font_face_status(t_font)) == CAIRO_STATUS_SUCCESS;
	if (t_success)
		r_cairo_font = t_font;
	
	CGFontRelease(t_cg_font);
	
	return t_success;
#else
	return false;
#endif
}

bool MCPDFPrintingDevice::set_cairo_pdf_datetime_to_now(cairo_pdf_datetime_t &r_datetime)
{
	CFAbsoluteTime t_system_time;
	t_system_time = CFAbsoluteTimeGetCurrent();

	CFGregorianDate t_datetime;
	t_datetime = CFAbsoluteTimeGetGregorianDate(t_system_time, NULL);
	
	r_datetime.year = t_datetime.year;
	r_datetime.month = t_datetime.month;
	r_datetime.day = t_datetime.day;
	r_datetime.hour = t_datetime.hour;
	r_datetime.minute = t_datetime.minute;
	r_datetime.second = t_datetime.second;

	r_datetime.utc_minute_offset = 0;

	return true;
}
