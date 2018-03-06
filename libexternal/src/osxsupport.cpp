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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <CoreServices/CoreServices.h>

#include <revolution/support.h>

char *string_to_utf8(const char *p_string)
{
	static TextToUnicodeInfo s_text_to_unicode_info = NULL;
	if (s_text_to_unicode_info == NULL)
	{
		UnicodeMapping t_mapping;
		t_mapping . unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format);
		t_mapping . otherEncoding = kTextEncodingMacRoman;
		t_mapping . mappingVersion = -1;
		CreateTextToUnicodeInfo(&t_mapping, &s_text_to_unicode_info);
	}
	
	char *t_result;
	
	if (s_text_to_unicode_info != NULL)
	{
		unsigned int t_string_length;
		t_string_length = strlen(p_string);
		
		t_result = (char *)malloc(t_string_length * 2 + 1);
		
		ByteCount t_utf8_length, t_processed;
		ConvertFromTextToUnicode(s_text_to_unicode_info, t_string_length, p_string, kUnicodeLooseMappingsMask, 0, NULL, NULL, NULL, t_string_length * 2, &t_processed, &t_utf8_length, (UniChar *)t_result);
		t_result[t_utf8_length] = '\0';
		t_result = (char *)realloc(t_result, t_utf8_length + 1);
	}
	else
		t_result = strdup(p_string);
		
	return t_result;
}

char *string_from_utf8(const char *p_utf8_string)
{
	static UnicodeToTextInfo s_unicode_to_text_info = NULL;
	if (s_unicode_to_text_info == NULL)
	{
		UnicodeMapping t_mapping;
		t_mapping . unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format);
		t_mapping . otherEncoding = kTextEncodingMacRoman;
		t_mapping . mappingVersion = -1;
		CreateUnicodeToTextInfo(&t_mapping, &s_unicode_to_text_info);
	}
	
	char *t_result;
	if (s_unicode_to_text_info != NULL)
	{
		unsigned int t_utf8_length;
		t_utf8_length = strlen(p_utf8_string);
		
		t_result = (char *)malloc(t_utf8_length + 1);
		
		ByteCount t_string_length, t_processed;
		ConvertFromUnicodeToText(s_unicode_to_text_info, t_utf8_length, (UniChar *)p_utf8_string, kUnicodeUseFallbacksMask | kUnicodeLooseMappingsMask, 0, NULL, 0, NULL, t_utf8_length, &t_processed, &t_string_length, t_result);
		t_result[t_string_length] = '\0';
		t_result = (char *)realloc(t_result, t_string_length + 1);
	}
	else
		t_result = strdup(p_utf8_string);
		
	return t_result;
}

char *string_from_utf16(const unsigned short *p_utf16_string, int p_length)
{
	static UnicodeToTextInfo s_unicode_converter = NULL;
	if (s_unicode_converter == NULL)
	{
		TextEncoding scriptEncoding;
		UpgradeScriptInfoToTextEncoding(smRoman, kTextLanguageDontCare,
		                                kTextRegionDontCare, NULL,
		                                &scriptEncoding);
		CreateUnicodeToTextInfoByEncoding(scriptEncoding,
		                                  &s_unicode_converter);
	}
	
	UniChar *s = (UniChar *)p_utf16_string;
	int len = p_length * 2;
	char *d = (char *)malloc(p_length);
	int destlen = 0;
	ByteCount processedbytes, outlength;
    
    // Use separate pointer to d string so that we can return the original d
    char *dptr = d;
	while(len > 1)
	{
		ConvertFromUnicodeToText(s_unicode_converter, len, (UniChar *)s,
								 kUnicodeLooseMappingsMask
								 | kUnicodeStringUnterminatedBit
								 | kUnicodeUseFallbacksBit, 0, NULL, 0, NULL,
								 p_length - destlen, &processedbytes,
								 &outlength, (LogicalAddress)dptr);
		if (processedbytes == 0)
		{
			*dptr = '?';
			processedbytes = 2;
			outlength = 1;
		}

		len -= processedbytes;
		destlen += outlength;
		s += processedbytes;
		dptr += outlength;
	}

	return d;
}

static char *strclone(const char *one)
{
	char *two = NULL;
	if (one != NULL)
	{
		two = (char *)malloc(strlen(one) + 1);
		strcpy(two, one);
	}
	return two;
}

char *os_path_to_native(const char *p_path)
{
	return string_to_utf8(p_path);
}

char *os_path_to_native_utf8(const char *p_path)
{
	return os_path_to_native(p_path);
}

char *os_path_from_native(const char *p_native_path)
{
	return string_from_utf8(p_native_path);
}


static char *get_current_directory(void)
{
	char namebuf[PATH_MAX + 2];
	char *dptr = (char *)malloc(PATH_MAX + 2);
	namebuf[0] = '\0';
	getcwd(namebuf, PATH_MAX);
	char *t_utf8_dpath;
	t_utf8_dpath = string_from_utf8(namebuf);
	strcpy(dptr, t_utf8_dpath);
	free(t_utf8_dpath);
	return dptr;
}

char *os_path_resolve(const char *p_native_path)
{
	if (p_native_path == NULL)
		return get_current_directory();

	char *tildepath;
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
		tildepath = (char *)malloc(strlen(pw->pw_dir) + strlen(tptr) + 2);
		strcpy(tildepath, pw->pw_dir);
		if (*tptr)
		{
			strcat(tildepath, "/");
			strcat(tildepath, tptr);
		}
		free(tpath);
	}
	else
		tildepath = strclone(p_native_path);
		
	if (tildepath[0] != '/')
	{
		char *cstr = get_current_directory();
		if (strlen(cstr) + strlen(tildepath) + 2 < PATH_MAX)
		{
			strcat(cstr, "/");
			strcat(cstr, tildepath);
		}
		free(tildepath);
		tildepath = cstr;
	}
	struct stat buf;
	if (lstat(tildepath, &buf) != 0 || !S_ISLNK(buf.st_mode))
		return tildepath;
	int4 size;
	char *newname = (char *)malloc(PATH_MAX + 2);
	if ((size = readlink(tildepath, newname, PATH_MAX)) < 0)
	{
		free(tildepath);
		free(newname);
		return NULL;
	}
	delete tildepath;
	newname[size] = '\0';
	if (newname[0] != '/')
	{
		char *fullpath = (char *)malloc(strlen(p_native_path) + strlen(newname) + 2);
		strcpy(fullpath, p_native_path);
		char *sptr = strrchr(fullpath, '/');
		if (sptr == NULL)
			sptr = fullpath;
		else
			sptr++;
		strcpy(sptr, newname);
		free(newname);
		newname = os_path_resolve(fullpath);
		free(fullpath);
	}
	return newname;
}
