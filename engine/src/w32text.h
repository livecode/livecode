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

#ifndef __W32TEXT__
#define __W32TEXT__

class WideCString
{
	LPCWSTR f_string;

public:
	WideCString(LPCSTR p_ansi_string, int t_length = -1);
	~WideCString(void);

	operator LPCWSTR (void) const;
};

inline WideCString::~WideCString(void)
{
	if (f_string != NULL)
		delete[] f_string;
}

inline WideCString::operator LPCWSTR(void) const
{
	return f_string;
}

class AnsiCString
{
	LPCSTR f_string;

public:
	AnsiCString(LPCWSTR p_wide_string);
	~AnsiCString(void);

	operator LPCSTR (void) const;
};

inline AnsiCString::~AnsiCString(void)
{
	if (f_string != NULL)
		delete[] f_string;
}

inline AnsiCString::operator LPCSTR(void) const
{
	return f_string;
}

#endif
