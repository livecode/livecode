/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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


/*
 * Handles all of the internationalization configuration options.
 * Author: Tom Tromey <tromey@creche.cygnus.com>
 */

#ifndef __GNOME_I18N_H__
#define __GNOME_I18N_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <glib.h>
#ifdef GNOME_EXPLICIT_TRANSLATION_DOMAIN
#  define BONOBO_EXPLICIT_TRANSLATION_DOMAIN GNOME_EXPLICIT_TRANSLATION_DOMAIN
#endif
#include <bonobo/bonobo-i18n.h>

G_BEGIN_DECLS

/* 'gnome_i18n_get_language_list' returns a list of language strings.
 *
 * It searches for one of following environment variables:
 * LANGUAGE
 * LC_ALL
 * 'category_name'
 * LANG
 *
 * If one of these environment variables was found, it is split into
 * pieces, whereever a ':' is found. When the environment variable included
 * no C locale, the C locale is appended to the list of languages.
 *
 * Assume, you have the following environment variables set:
 *
 * LC_MONETARY="de_DE:es"
 * LANG="de_DE:de:C:en"
 *
 * In this case 'gnome_i18n_get_language_list ("LC_COLLATE")' returns the
 * list: ("de_DE" "de" "C" "en").
 *
 * 'gnome_i18n_get_language_list ("LC_MONETARY")' returns:
 * ("de_DE" "es" "C")
 *
 * The returned list must not be changed.
 */

const GList *	gnome_i18n_get_language_list		(const gchar *category_name);

/* Push "C" numeric locale.  Do this before doing any floating
 * point to/from string conversions, if those are to be done in
 * a portable manner.  This is a hack really, and there is
 * no need to generalize it to other cathegories.  But it is
 * needed whenever things like printing scanning floats from or
 * to files or other places where you'd like to read them back
 * later. */
void		gnome_i18n_push_c_numeric_locale	(void);
void		gnome_i18n_pop_c_numeric_locale		(void);

G_END_DECLS

#endif /* __GNOME_I18N_H__ */

#endif /* GNOME_DISABLE_DEPRECATED */
