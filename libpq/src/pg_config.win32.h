#ifndef	pg_config_h_win32__
#define	pg_config_h_win32__
/*
 * Parts of pg_config.h that you get with autoconf on other systems
 */
#define PG_VERSION "8.1.3"
#define PG_VERSION_STR "8.1.3 (win32)"

#define DEF_PGPORT 5432
#define DEF_PGPORT_STR "5432"

#define MAXIMUM_ALIGNOF 4
#define ACCEPT_TYPE_ARG3 int

#define MAXPGPATH 1024

#define INDEX_MAX_KEYS 32

#define HAVE_ATEXIT
#define HAVE_MEMMOVE

#ifdef __BORLANDC__
#define HAVE_RANDOM
#endif

/* use _snprintf and _vsnprintf */
#define HAVE_DECL_SNPRINTF 1
#define snprintf        _snprintf
#define HAVE_DECL_VSNPRINTF 1
#define vsnprintf       _vsnprintf

/* defines for dynamic linking on Win32 platform */
#ifdef __CYGWIN__

#if __GNUC__ && ! defined (__declspec)
#error You need egcs 1.1 or newer for compiling!
#endif

#ifdef BUILDING_DLL
#define DLLIMPORT __declspec (dllexport)
#else							/* not BUILDING_DLL */
#define DLLIMPORT __declspec (dllimport)
#endif

#elif defined(WIN32) && defined(_MSC_VER)		/* not CYGWIN */

#if defined(_DLL)
#define DLLIMPORT __declspec (dllexport)
#else							/* not _DLL */
#define DLLIMPORT __declspec (dllimport)
#endif

#else							/* not CYGWIN, not MSVC */

#define DLLIMPORT

#endif

#undef _UNICODE
#undef UNICODE

#define _ERRCODE_DEFINED
typedef int errno_t;

#define FRONTEND

#ifndef __CYGWIN__
#include <windows.h>
#endif

#endif /* pg_config_h_win32__ */
