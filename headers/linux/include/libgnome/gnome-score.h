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

#ifndef __GNOME_SCORE_H__
#define __GNOME_SCORE_H__ 1

#ifndef GNOME_DISABLE_DEPRECATED

#include <time.h>
#include <glib.h>

G_BEGIN_DECLS
/*
 * gnome_score_init()
 * creates a child process with which we communicate through a pair of pipes,
 * then drops privileges.
 * this should be called as the first statement in main().
 * returns 0 on success, drops privs and returns -1 on failure
 */

gint
gnome_score_init (const gchar * gamename);

/* Returns the position in the top-ten starting from 1, or 0 if it isn't in the table */
gint
gnome_score_log(gfloat score,
		const gchar *level, /* Pass in NULL unless you want to keep
				       per-level scores for the game */
		/* Pass in TRUE if higher scores are "better"
		   in the game */
		gboolean higher_to_lower_score_order);

/* Returns number of items in the arrays */
gint
gnome_score_get_notable(const gchar *gamename, /* Will be auto-determined if NULL */
			const gchar *level,
			gchar ***names,
			gfloat **scores,
			time_t **scoretimes);
G_END_DECLS

#endif /* __GNOME_SCORE_H__ */

#endif /* GNOME_DISABLE_DEPRECATED */
