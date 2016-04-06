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

#include <Shlwapi.h>

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

// SN-2014-12-23: [[ Bug 14178 ]] Windows doesn't support UTF-8 filepaths
bool MCPDFPrintingDevice::get_filename(const char* p_utf8_path, char *& r_system_path)
{
	bool t_success;
	t_success = true;

	LPWSTR t_wstring;
	DWORD t_wlength;
	t_wstring = NULL;

	LPWSTR t_shortpath;
	DWORD t_shortpath_length;
	t_shortpath = NULL;

	LPSTR t_system_path;
	DWORD t_sys_length;

	uint32_t t_size = strlen(p_utf8_path);

	t_wlength = MultiByteToWideChar(CP_UTF8, 0, p_utf8_path, strlen(p_utf8_path), 0, 0);

	t_success = t_wlength != 0;

	if (t_success)
	{
		t_wstring = (LPWSTR)calloc(t_wlength + 1, sizeof(WCHAR));
		MultiByteToWideChar(CP_UTF8, 0, p_utf8_path, strlen(p_utf8_path), t_wstring, t_wlength);
	}

	// Create a file if needed, otherwise GetShortPathName does not work.
	DWORD t_attrs;
	t_attrs = GetFileAttributesW(t_wstring);

	bool t_file_created;
	t_file_created = false;

	if (t_success && t_attrs == INVALID_FILE_ATTRIBUTES)
	{
		HANDLE t_file_handle = CreateFileW(t_wstring, GENERIC_WRITE, 0, NULL,
							 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (t_file_handle == NULL)
			t_success = false;
		else
		{
			CloseHandle(t_file_handle);
			t_file_created = true;
		}
	}

	if (t_success)
	{
		t_shortpath_length = GetShortPathNameW(t_wstring, NULL, 0);

		if (t_shortpath_length == 0)
			t_success = false;
	}

	if (t_success)
	{
		t_shortpath = (LPWSTR)calloc(t_shortpath_length + 1, sizeof(WCHAR));
		GetShortPathNameW(t_wstring, t_shortpath, t_shortpath_length);
	}

	if (t_success)
	{
		t_sys_length = WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, t_shortpath, t_shortpath_length, 0, 0, 0, 0);
		t_success = t_sys_length != 0;
	}

	if (t_success)
	{
		t_system_path = (LPSTR)calloc(t_sys_length + 1, sizeof(CHAR));
		WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS, t_shortpath, t_shortpath_length, t_system_path, t_sys_length, 0, 0);
	}

	// Memory cleanup
	if (!t_success && t_file_created)
		DeleteFileW(t_wstring);

	delete t_wstring;
	delete t_shortpath;

	if (t_success)
		r_system_path = (char*)t_system_path;

	return t_success;
}
