/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __G_WIN32_H__
#define __G_WIN32_H__

#include <glib/gtypes.h>

#ifdef G_PLATFORM_WIN32

G_BEGIN_DECLS

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifdef G_OS_WIN32

/*
 * To get prototypes for the following POSIXish functions, you have to
 * include the indicated non-POSIX headers. The functions are defined
 * in OLDNAMES.LIB (MSVC) or -lmoldname-msvc (mingw32).
 *
 * getcwd: <direct.h> (MSVC), <io.h> (mingw32)
 * getpid: <process.h>
 * access: <io.h>
 * unlink: <stdio.h> or <io.h>
 * open, read, write, lseek, close: <io.h>
 * rmdir: <io.h>
 * pipe: <io.h>
 */

/* pipe is not in OLDNAMES.LIB or -lmoldname-msvc. */
#define pipe(phandles)	_pipe (phandles, 4096, _O_BINARY)

/* For some POSIX functions that are not provided by the MS runtime,
 * we provide emulation functions in glib, which are prefixed with
 * g_win32_. Or that was the idea at some time, but there is just one
 * of those:
 */
gint		g_win32_ftruncate	(gint		 f,
					 guint		 size);
#endif /* G_OS_WIN32 */

/* The MS setlocale uses locale names of the form "English_United
 * States.1252" etc. We want the Unixish standard form "en", "zh_TW"
 * etc. This function gets the current thread locale from Windows and
 * returns it as a string of the above form for use in forming file
 * names etc. The returned string should be deallocated with g_free().
 */
gchar* 		g_win32_getlocale  (void);

/* Translate a Win32 error code (as returned by GetLastError()) into
 * the corresponding message. The returned string should be deallocated
 * with g_free().
 */
gchar*          g_win32_error_message (gint error);

#define g_win32_get_package_installation_directory g_win32_get_package_installation_directory_utf8
#define g_win32_get_package_installation_subdirectory g_win32_get_package_installation_subdirectory_utf8

gchar*          g_win32_get_package_installation_directory (gchar *package,
							    gchar *dll_name);

gchar*          g_win32_get_package_installation_subdirectory (gchar *package,
							       gchar *dll_name,
							       gchar *subdir);

guint		g_win32_get_windows_version (void);

gchar*          g_win32_locale_filename_from_utf8 (const gchar *utf8filename);

#define G_WIN32_IS_NT_BASED() (g_win32_get_windows_version () < 0x80000000)
#define G_WIN32_HAVE_WIDECHAR_API() (G_WIN32_IS_NT_BASED ())

G_END_DECLS

#endif	 /* G_PLATFORM_WIN32 */

#endif /* __G_WIN32_H__ */
