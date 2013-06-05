/* gnome-url.h
 * Copyright (C) 1998 James Henstridge <james@daa.com.au>
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#ifndef GNOME_URL_H
#define GNOME_URL_H

// MDW 2013-06-05 : error: #error "Only <glib.h> can be included directly."
#include <glib.h>
//#include <glib/gmacros.h>
//#include <glib/gerror.h>

G_BEGIN_DECLS

/**
 * GnomeURLError:
 * @GNOME_URL_ERROR_PARSE: The parsing of the handler failed.
 *
 * The errors that can be returned due to bad parameters being pass to
 * gnome_url_show().
 */
typedef enum {
  GNOME_URL_ERROR_PARSE,
  GNOME_URL_ERROR_LAUNCH,
  GNOME_URL_ERROR_URL,
  GNOME_URL_ERROR_NO_DEFAULT,
  GNOME_URL_ERROR_NOT_SUPPORTED,
  GNOME_URL_ERROR_VFS,
  GNOME_URL_ERROR_CANCELLED
} GnomeURLError;

#define GNOME_URL_ERROR (gnome_url_error_quark ())
GQuark   gnome_url_error_quark (void) G_GNUC_CONST;


gboolean gnome_url_show           (const char  *url,
				   GError     **error);

gboolean gnome_url_show_with_env  (const char  *url,
				   char       **envp,
				   GError     **error);

G_END_DECLS
#endif
