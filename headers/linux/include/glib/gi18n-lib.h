/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 2002  Peter Mattis, Red Hat, Inc.
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
#ifndef __G_I18N_LIB_H__
#define __G_I18N_LIB_H__

#include <glib/gstrfuncs.h>

#include <libintl.h>

#ifndef GETTEXT_PACKAGE
#error You must define GETTEXT_PACKAGE before including gi18n-lib.h.
#endif

#define _(String) dgettext (GETTEXT_PACKAGE, String)
#define Q_(String) g_strip_context ((String), dgettext (GETTEXT_PACKAGE, String))
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif

#endif  /* __G_I18N_LIB_H__ */


