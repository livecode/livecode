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

#include <cstring>
#include <sys/stat.h>

#include <windows.h>

#include "revfont.h"

FontLoadStatus FontLoad(const char *p_filename)
{
	FontLoadStatus t_status;
	t_status = kFontLoadStatusSuccess;

	if (t_status == kFontLoadStatusSuccess)
	{
		struct _stat t_info;
		if (_stat(p_filename, &t_info) != 0 || (t_info . st_mode & _S_IFDIR) != 0)
			t_status = kFontLoadStatusNotFound;
	}
	
	if (t_status == kFontLoadStatusSuccess)
	{
		if (AddFontResourceA(p_filename) == 0)
			t_status = kFontLoadStatusBadFont;
	}

	if (t_status == kFontLoadStatusSuccess)
		PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

	return t_status;
}

bool FontUnload(const char *p_filename)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = RemoveFontResourceA(p_filename) != 0;

	if (t_success)
		PostMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);

	return t_success;
}
