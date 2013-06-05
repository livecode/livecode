/*
 * Copyright (C) 1997-1998 Stuart Parmenter and Elliot Lee
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

#ifndef __GNOME_SOUND_H__
#define __GNOME_SOUND_H__ 1

#include <glib.h>

G_BEGIN_DECLS

/* Use this with the Esound functions */
int gnome_sound_connection_get (void);

/* Initialize esd connection */
void gnome_sound_init(const char *hostname);

/* Closes esd connection */
void gnome_sound_shutdown(void);

/* Returns the Esound sample ID for the sample */
int gnome_sound_sample_load(const char *sample_name, const char *filename);

/* Loads sample, plays sample, frees sample */
void gnome_sound_play (const char * filename);

G_END_DECLS

#endif /* __GNOME_SOUND_H__ */
