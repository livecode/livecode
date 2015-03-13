#ifndef __REVOLUTION_EXTERNAL_SUPPORT__
#define __REVOLUTION_EXTERNAL_SUPPORT__

#ifdef _WINDOWS
#define strcasecmp _stricmp
#define PATH_MAX MAX_PATH
#endif

#ifdef _MACOSX
#define stricmp strcasecmp

typedef unsigned int uint4;
typedef int int4;
#endif

// Parameters:
//   p_in_string : pointer to a string in UTF-16
//   p_length : the length of the string in characters
// Returns
//   a pointer to a string in the native encoding. Must be free by the caller
char *string_from_utf16(const unsigned short *p_in_string, int p_length);

// Parameters:
//   p_path : pointer to path in Revolution format. May be relative or absolute
// Returns:
//   a pointer to the converted path. Must be freed by the caller.
// Semantics:
//   Converts a Revolution path into a native path.
//
//   The UTF8 variant returns the path encoded as UTF-8 rather than in the native
//   (filename) platform encoding.
//
char *os_path_to_native(const char *p_path);
char *os_path_to_native_utf8(const char *p_path);

// Parameters:
//   p_native_path : pointer to path in native format, may be relative or absolute
// Returns:
//   a pointer to the converted path. Must be freed by the caller
// Semantics:
//   Converts a native path into a Revolution path
char *os_path_from_native(const char *p_native_path);

// Parameters:
//   p_native_path : pointer to a native path. May be relative or absolute
// Returns:
//   a pointer to the resolved path. Must be freed by the caller
// Semantics:
//   Resolves a native path into an absolute path, e.g. by expanding "~" etc.
char *os_path_resolve(const char *p_native_path);

#endif
