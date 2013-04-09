/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef __GNOME_UTIL_H__
#define __GNOME_UTIL_H__

#include <stdlib.h>
#include <glib.h>
#include <libgnome/gnome-init.h>
#include <libgnome/gnome-program.h>

G_BEGIN_DECLS

/* Return pointer to the character after the last .,
   or "" if no dot. */
const char * g_extension_pointer (const char * path);


/* pass in a string, and it will add the users home dir ie,
 * pass in .gnome/bookmarks.html and it will return
 * /home/imain/.gnome2/bookmarks.html
 *
 * Remember to g_free() returned value! */
#define gnome_util_prepend_user_home(x) (g_build_filename (g_get_home_dir(), (x), NULL))

/* very similar to above, but adds $HOME/.gnome2/ to beginning
 * This is meant to be the most useful version.
 */
#define gnome_util_home_file(afile) (g_build_filename(g_get_home_dir(), GNOME_DOT_GNOME, (afile), NULL))

/* Find the name of the user's shell.  */
char *gnome_util_user_shell (void);

/* Portable versions of setenv/unsetenv */

/* Note: setenv will leak on some systems (those without setenv) so
 * do NOT use inside a loop.  Semantics are the same as those in glibc */
int	gnome_setenv (const char *name, const char *value, gboolean overwrite);
void	gnome_unsetenv (const char *name);
void	gnome_clearenv (void);

/* Some deprecated functions macroed to their new equivalents */
#ifndef GNOME_DISABLE_DEPRECATED

#define g_file_exists(filename)		g_file_test ((filename), G_FILE_TEST_EXISTS)
#define g_unix_error_string(error_num)	g_strerror ((error_num))
#define gnome_util_user_home()		g_get_home_dir ()
#define g_copy_vector(vec)		g_strdupv ((vec))
#define g_concat_dir_and_file(dir,file)	g_build_filename ((dir), (file), NULL)

#define gnome_is_program_in_path(program)	g_find_program_in_path((program))

#define gnome_libdir_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_LIBDIR,  (f), TRUE, NULL))
#define gnome_datadir_file(f) (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, (f), TRUE, NULL))
#define gnome_sound_file(f)   (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_SOUND,   (f), TRUE, NULL))
#define gnome_pixmap_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,  (f), TRUE, NULL))
#define gnome_config_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_CONFIG,  (f), TRUE, NULL))

#define gnome_unconditional_libdir_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_LIBDIR,  (f), FALSE, NULL))
#define gnome_unconditional_datadir_file(f) (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, (f), FALSE, NULL))
#define gnome_unconditional_sound_file(f)   (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_SOUND,   (f), FALSE, NULL))
#define gnome_unconditional_pixmap_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,  (f), FALSE, NULL))
#define gnome_unconditional_config_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_CONFIG,  (f), FALSE, NULL))

#endif /* GNOME_DISABLE_DEPRECATED */


G_END_DECLS

#endif
