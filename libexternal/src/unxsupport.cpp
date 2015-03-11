#include <cstring>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#if !defined(TARGET_SUBPLATFORM_ANDROID) && !defined(TARGET_SUBPLATFORM_IPHONE)
#include <pwd.h>
#endif
#include <sys/stat.h>

#include <revolution/support.h>

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
			t_utf8_string[j++] = 0xC0 | (v >> 5);
			t_utf8_string[j++] = 0x80 | (v & 63);
		}
	}

	t_utf8_string[j] = '\0';

	return t_utf8_string;
}

char *string_from_utf8(const char* p_utf8_string)
{
	char *t_iso_string;
	t_iso_string = (char*)malloc(strlen(p_utf8_string) + 1);

	int i, j, t_error;
	t_error = 0;

	for (i = 0, j = 0; p_string[i] != '\0' && !t_error;)
	{
		unsigned char t_first_char;
		t_first_char = ((unsigned char *)p_utf8_string)[i++];

		if (t_first_char < 128)
			t_iso_string[j++] = t_first_char;
		else if (p_string[i] != '\0')
		{
			unsigned char t_second_char;
			t_second_char = p_utf8_string[i++];

			t_iso_string[j++] = ((t_first_char & 0xBF) << 5)
								| ((t_second_char & 0x63); 
		}
		else
			t_error = true;
	}

	if (t_error)
		return NULL;
	else
	{
		t_iso_string[j] = '\0';
		return t_iso_string;
	}
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
		two = new char[strlen(one) + 1];
		strcpy(two, one);
	}
	return two;
}

char *get_current_directory()
{
	char *dptr = new char[PATH_MAX + 2];
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
		tildepath = new char[strlen(pw->pw_dir) + strlen(tptr) + 2];
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
	char *newname = new char[PATH_MAX + 2];
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
		char *fullpath = new char[strlen(p_native_path) + strlen(newname) + 2];
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


// SN-2015-03-10:[[ Bug 14413 ]] Added UTF-8 conversion functions

// Parameters:
//  p_utf8_string : pointer to UTF-8 encoded string.
// Returns:
//  a pointer to the native-encoded string. Must be freed by the caller
// Semantics:
//  Converts a UTF-8 encoded srting into a Native string
char *ConvertCStringFromUTF8ToNative(const char* p_utf8_path, int *r_success)
{
	char *t_native_string;
	t_native_string = string_from_utf8(p_utf8_path);

	*r_success = t_native_string != NULL;
	return t_native_string;
}

// Parameters:
//  p_native_string : pointer to native-encoded string.
// Returns:
//  a pointer to the UTF-8 encoded string. Must be freed by the caller
// Semantics:
//  Converts a native srting into a UTF-8 encoded string
char *ConvertCStringFromNativeToUTF8(const char* p_native_string, int *r_success)
{
	char *t_utf8_string;
	t_utf8_string = string_to_utf8(p_native_string);

	*r_success = t_utf8_string != NULL;
	return t_utf8_string;
}

