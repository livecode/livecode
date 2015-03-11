#include <windows.h>
#include <cstring>
#include <revolution/support.h>

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

char *string_from_utf8(const char *p_utf8_string)
{
	int t_length;
	// Make sure that the length includes the NULL terminating char
	t_length = strlen(p_utf8_string) + 1;

	WCHAR *t_utf16_path;
	t_utf16_path = (WCHAR *)malloc(sizeof(WCHAR) * t_length);
	MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, p_utf8_string, t_length, t_utf16_path, t_length);

	int t_native_length;
	t_native_length = WideCharToMultiByte(CP_ACP, 0, t_utf16_path, t_length, NULL, 0, NULL, NULL);

	char *t_native_string;
	t_native_string = (char *)malloc(t_native_length);

	WideCharToMultiByte(CP_ACP, 0, t_utf16_path, t_length, t_native_string, t_native_length, NULL, NULL);

	free(t_utf16_path);

	return t_native_string;
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
		two = new char[strlen(one) + 1];
		strcpy(two, one);
	}
	return two;
}

char *get_currrent_directory()
{
	char *dptr = new char[PATH_MAX + 2];
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
