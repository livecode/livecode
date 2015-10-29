/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#include "sserialize.h"
#include "sserialize_w32.h"

bool deserialize_data_to_hglobal(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, HGLOBAL &r_hglobal)
{
	bool t_success = true;
	uint32_t t_size;
	uint32_t t_offset = r_offset;
	t_success = deserialize_uint32(p_stream, p_stream_size, t_offset, t_size);
	if (t_success)
	{
		if (r_hglobal == NULL)
			r_hglobal = GlobalAlloc(GMEM_MOVEABLE, t_size);
		else if (GlobalSize(r_hglobal) < t_size)
			r_hglobal = GlobalReAlloc(r_hglobal, t_size, 0);
		t_success = (r_hglobal != NULL);
	}
	if (t_success)
	{
		void *t_ptr = GlobalLock(r_hglobal);
		t_success = deserialize_data(p_stream, p_stream_size, r_offset, t_ptr, t_size);
		GlobalUnlock(r_hglobal);
	}
	return t_success;
}

bool serialize_printdlg_data(char *&r_buffer, uint32_t &r_size, PRINTDLGEXA &p_data)
{
	bool t_success = true;

	uint32_t t_size, t_devmode_size, t_devnames_size, t_pagerange_size;
	t_devmode_size = GlobalSize(p_data.hDevMode);
	t_devnames_size = GlobalSize(p_data.hDevNames);
	t_pagerange_size = p_data.nMaxPageRanges * sizeof(PRINTPAGERANGE);
	t_size = p_data.lStructSize + t_devmode_size + t_devnames_size + t_pagerange_size;
	// reserve space for byte counts
	t_size += sizeof(p_data.lStructSize) + sizeof(t_devmode_size) + sizeof(t_devnames_size) + sizeof(t_pagerange_size);

	char *t_buffer = NULL;
	uint32_t t_offset = 0;
	t_buffer = (char*)malloc(t_size);

	t_success = (t_buffer != NULL);

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data, p_data.lStructSize);

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevMode), t_devmode_size);
		GlobalUnlock(p_data.hDevMode);
	}

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevNames), t_devnames_size);
		GlobalUnlock(p_data.hDevNames);
	}

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data.lpPageRanges, t_pagerange_size);

	if (t_success)
	{
		r_buffer = t_buffer;
		r_size = t_size;
	}

	return t_success;
}

bool deserialize_printdlg_data(const char *p_buffer, uint32_t p_size, PRINTDLGEXA *&x_data)
{
	bool t_success = true;

	HGLOBAL t_devmode, t_devnames;
	PRINTPAGERANGE *t_ranges;
	uint32_t t_ranges_size = 0;
	uint32_t t_dlg_size = 0;

	uint32_t t_offset = 0;
	void *t_ptr;

	if (x_data == NULL)
	{
		t_devmode = t_devnames = t_ranges = NULL;
	}
	else
	{
		t_devmode = x_data->hDevMode;
		t_devnames = x_data->hDevNames;
		t_ranges = x_data->lpPageRanges;
		t_ranges_size = x_data->nMaxPageRanges * sizeof(PRINTPAGERANGE);
		t_dlg_size = sizeof(PRINTDLGEXA);
	}

	// copy dialog data
	t_ptr = x_data;
	t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_dlg_size);
	x_data = (PRINTDLGEXA*)t_ptr;

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devmode);

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devnames);

	if (t_success)
	{
		t_ptr = t_ranges;
		t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_ranges_size);
		t_ranges = (PRINTPAGERANGE*)t_ptr;
	}

	if (t_success)
	{
		x_data->hDevMode = t_devmode;
		x_data->hDevNames = t_devnames;
		x_data->lpPageRanges = t_ranges;
	}

	return t_success;
}

bool serialize_pagedlg_data(char *&r_buffer, uint32_t &r_size, PAGESETUPDLGA &p_data)
{
	bool t_success = true;

	uint32_t t_size, t_devmode_size, t_devnames_size;
	t_devmode_size = GlobalSize(p_data.hDevMode);
	t_devnames_size = GlobalSize(p_data.hDevNames);
	t_size = p_data.lStructSize + t_devmode_size + t_devnames_size;
	// reserve space for byte counts
	t_size += sizeof(p_data.lStructSize) + sizeof(t_devmode_size) + sizeof(t_devnames_size);

	char *t_buffer = NULL;
	uint32_t t_offset = 0;
	t_buffer = (char*)malloc(t_size);

	t_success = (t_buffer != NULL);

	if (t_success)
		t_success = serialize_data(t_buffer, t_size, t_offset, &p_data, p_data.lStructSize);

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevMode), t_devmode_size);
		GlobalUnlock(p_data.hDevMode);
	}

	if (t_success)
	{
		t_success = serialize_data(t_buffer, t_size, t_offset, GlobalLock(p_data.hDevNames), t_devnames_size);
		GlobalUnlock(p_data.hDevNames);
	}

	if (t_success)
	{
		r_buffer = t_buffer;
		r_size = t_size;
	}

	return t_success;
}

bool deserialize_pagedlg_data(const char *p_buffer, uint32_t p_size, PAGESETUPDLGA *&x_data)
{
	bool t_success = true;

	HGLOBAL t_devmode, t_devnames;
	uint32_t t_dlg_size = 0;

	uint32_t t_offset = 0;
	void *t_ptr;

	if (x_data == NULL)
	{
		t_devmode = t_devnames = NULL;
	}
	else
	{
		t_devmode = x_data->hDevMode;
		t_devnames = x_data->hDevNames;
		t_dlg_size = sizeof(PAGESETUPDLGA);
	}

	// copy dialog data
	t_ptr = x_data;
	t_success = deserialize_data(p_buffer, p_size, t_offset, t_ptr, t_dlg_size);
	x_data = (PAGESETUPDLGA*)t_ptr;

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devmode);

	if (t_success)
		t_success = deserialize_data_to_hglobal(p_buffer, p_size, t_offset, t_devnames);

	if (t_success)
	{
		x_data->hDevMode = t_devmode;
		x_data->hDevNames = t_devnames;
	}

	return t_success;
}
