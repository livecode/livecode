/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 *               2001 SuSE Linux AG
 * All rights reserved.
 *
 * This file is part of GNOME 2.0.
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

#ifndef LIBGNOMEINIT_H
#define LIBGNOMEINIT_H

#include <libgnome/gnome-program.h>

G_BEGIN_DECLS

/* This is where the user specific files are stored under $HOME
 * (do not use these macros; use gnome_user_dir_get(); it's possible
 * to override .gnome2 via environment variable and this is
 * an important feature for environments that mix GNOME versions)
 */
#define GNOME_DOT_GNOME		".gnome2/"
#define GNOME_DOT_GNOME_PRIVATE	".gnome2_private/"

#define LIBGNOME_MODULE libgnome_module_info_get()
const GnomeModuleInfo *libgnome_module_info_get (void) G_GNUC_CONST;
#define GNOME_BONOBO_MODULE gnome_bonobo_module_info_get()
const GnomeModuleInfo * gnome_bonobo_module_info_get (void) G_GNUC_CONST;

const char *gnome_user_dir_get (void) G_GNUC_CONST;
const char *gnome_user_private_dir_get (void) G_GNUC_CONST;
const char *gnome_user_accels_dir_get (void) G_GNUC_CONST;

#ifdef G_OS_WIN32
void gnome_win32_get_prefixes   (gpointer    hmodule,
			         char      **full_prefix,
			         char      **cp_prefix);
#endif

G_END_DECLS

#endif /* LIBGNOMEINIT_H */
