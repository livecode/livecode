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
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#if !defined(TARGET_SUBPLATFORM_ANDROID) && !defined(TARGET_SUBPLATFORM_IPHONE)
#include <pwd.h>
#endif
#include <sys/stat.h>
#include <new>

#include <revolution/support.h>

using std::nothrow;

#define PATH_MAX 4096
#define stricmp strcasecmp

typedef int int4;

char *string_from_utf16(const unsigned short *p_string, int p_length)
{
	char *t_result;
	t_result = (char *)malloc(p_length);
	for(int i = 0; i < p_length; ++i)
		t_result[i] = p_string[i];
	return t_result;
}

// Turn an ISO8859-1 encoded string as UTF-8.
char *string_to_utf8(const char *p_string)
{
	char *t_utf8_string;
	t_utf8_string = (char *)malloc(strlen(p_string) * 2 + 1);
	
	int i, j;
	for(i = 0, j = 0; p_string[i] != '\0'; ++i)
	{
		unsigned int v;
		v = ((unsigned char *)p_string)[i];

        if (v < 128)
			t_utf8_string[j++] = v;
		else
		{
            // SN-2015-03-11: [[ Bug 14413 ]] Do the expected shift
            t_utf8_string[j++] = (0xC0 | (v >> 6));
            t_utf8_string[j++] = (0x80 | (v & 63));
		}
    }

	t_utf8_string[j] = '\0';

	return t_utf8_string;
}

// LINUX implimentation of externals.
char *os_path_to_native(const char *p_path)
{
	return (strdup(p_path));
}

char *os_path_to_native_utf8(const char *p_path)
{
	return string_to_utf8(p_path);
}

char *os_path_from_native(const char *p_native_path)
{
	return (strdup(p_native_path));
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

char *get_current_directory()
{
	char *dptr = new (nothrow) char[PATH_MAX + 2];
	getcwd(dptr, PATH_MAX);
	return dptr;
}

char *os_path_resolve(const char *p_native_path)
{
	if (p_native_path == NULL)
		return get_current_directory();
	char *tildepath;
	
#if !defined(TARGET_SUBPLATFORM_ANDROID) && !defined(TARGET_SUBPLATFORM_IPHONE)
	if (p_native_path[0] == '~')
	{
		char *tpath = strclone(p_native_path);
		char *tptr = strchr(tpath, '/');
		if (tptr == NULL)
		{
			tpath[0] = '\0';
			tptr = tpath;
		}
		else
			*tptr++ = '\0';

		struct passwd *pw;
		if (*(tpath + 1) == '\0')
			pw = getpwuid(getuid());
		else
			pw = getpwnam(tpath + 1);
		if (pw == NULL)
			return NULL;
		tildepath = new (nothrow) char[strlen(pw->pw_dir) + strlen(tptr) + 2];
		strcpy(tildepath, pw->pw_dir);
		if (*tptr)
		{
			strcat(tildepath, "/");
			strcat(tildepath, tptr);
		}
		delete tpath;
	}
	else
#endif
		tildepath = strclone(p_native_path);

	struct stat buf;
	if (lstat(tildepath, &buf) != 0 || !S_ISLNK(buf.st_mode))
		return tildepath;
	int4 size;
	char *newname = new (nothrow) char[PATH_MAX + 2];
	if ((size = readlink(tildepath, newname, PATH_MAX)) < 0)
	{
		delete tildepath;
		delete newname;
		return NULL;
	}
	delete tildepath;
	newname[size] = '\0';
	if (newname[0] != '/')
	{
		char *fullpath = new (nothrow) char[strlen(p_native_path) + strlen(newname) + 2];
		strcpy(fullpath, p_native_path);
		char *sptr = strrchr(fullpath, '/');
		if (sptr == NULL)
			sptr = fullpath;
		else
			sptr++;
		strcpy(sptr, newname);
		delete newname;
		newname = os_path_resolve(fullpath);
		delete fullpath;
	}
	return newname;
}
