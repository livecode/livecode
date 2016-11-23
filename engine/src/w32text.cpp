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

#include "w32text.h"
#include <foundation.h>

WideCString::WideCString(LPCSTR p_ansi_string, int p_length)
{
	int t_length;
	t_length = MultiByteToWideChar(CP_ACP, 0, p_ansi_string, p_length, NULL, 0);

	f_string = new (nothrow) WCHAR[t_length];
	if (f_string != NULL)
		MultiByteToWideChar(CP_ACP, 0, p_ansi_string, p_length, (LPWSTR)f_string, t_length);
}

AnsiCString::AnsiCString(LPCWSTR p_wide_string)
{
	int t_length;
	t_length = WideCharToMultiByte(CP_ACP, 0, p_wide_string, -1, NULL, 0, NULL, NULL);

	f_string = new (nothrow) CHAR[t_length];
	if (f_string != NULL)
		WideCharToMultiByte(CP_ACP, 0, p_wide_string, -1, (LPSTR)f_string, t_length, NULL, NULL);
}
