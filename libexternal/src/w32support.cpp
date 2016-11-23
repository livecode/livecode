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

#include <windows.h>
#include <cstring>
#include <revolution/support.h>
#include <core.h>

char *string_from_utf16(const unsigned short *p_utf16_string, int p_utf16_length)
{
	char *t_string;
	t_string = (char *)malloc(p_utf16_length + 1);
	if (t_string == NULL)
		return NULL;

	WideCharToMultiByte(1252, 0, (LPCWSTR)p_utf16_string, p_utf16_length, t_string, p_utf16_length, NULL, NULL);
	t_string[p_utf16_length] = '\0';
	return t_string;
}

char *string_to_utf8(const char *p_native_path)
{
	int t_length;
	t_length = strlen(p_native_path) + 1;

	WCHAR *t_utf16_path;
	t_utf16_path = (WCHAR *)malloc(sizeof(WCHAR) * t_length);
	MultiByteToWideChar(1252, MB_PRECOMPOSED, p_native_path, t_length, t_utf16_path, t_length);

	int t_utf8_length;
	t_utf8_length = WideCharToMultiByte(CP_UTF8, 0, t_utf16_path, t_length, NULL, 0, NULL, NULL);

	char *t_utf8_path;
	t_utf8_path = (char *)malloc(t_utf8_length);

	WideCharToMultiByte(CP_UTF8, 0, t_utf16_path, t_length, t_utf8_path, t_utf8_length, NULL, NULL);

	free(t_utf16_path);

	return t_utf8_path;
}

char *os_path_to_native_utf8(const char *p_path)
{
	char *t_native_path;
	t_native_path = os_path_to_native(p_path);

	char *t_native_utf8_path;
	t_native_utf8_path = string_to_utf8(t_native_path);
	free(t_native_path);

	return t_native_utf8_path;
}

char *os_path_to_native(const char *p_path)
{
	char *t_path;
	t_path = strdup(p_path);

	char *dptr;
	dptr = t_path;

	if (!*dptr)
		return dptr;

	do {
		if (*dptr == '/')
			*dptr = '\\';
	} while (*++dptr);

	return t_path;
}


char *os_path_from_native(const char *p_native_path)
{
	char *t_path;
	t_path = strdup(p_native_path);

	char *dptr;
	dptr = t_path;

	if (!*dptr)
		return dptr;

	do {
		if (*dptr == '\\')
			*dptr = '/';
	} while (*++dptr);

	return t_path;
}


char *strclone(const char *one)
{
	char *two = NULL;
	if (one != NULL)
	{
		two = new (nothrow) char[strlen(one) + 1];
		strcpy(two, one);
	}
	return two;
}

char *get_currrent_directory()
{
	char *dptr = new (nothrow) char[PATH_MAX + 2];
	GetCurrentDirectoryA(PATH_MAX +1, (LPSTR)dptr);
	dptr = os_path_to_native(dptr);
	return dptr;
}

char *os_path_resolve(const char *p_native_path)
{
	if (p_native_path == NULL)
	{
		char *tpath = get_currrent_directory();
		tpath = os_path_to_native(tpath);
		return tpath;
	}
	char *cstr = strclone(p_native_path);
	cstr = os_path_to_native(cstr);
	return cstr;
}
