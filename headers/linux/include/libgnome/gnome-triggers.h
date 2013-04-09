/*
 * Copyright (C) 1997, 1998 Elliot Lee
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

#ifndef __GNOME_TRIGGERS_H__
#define __GNOME_TRIGGERS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GTRIG_NONE,
	GTRIG_FUNCTION,
	GTRIG_COMMAND,
	GTRIG_MEDIAPLAY
} GnomeTriggerType;

typedef void (*GnomeTriggerActionFunction)(char *msg, char *level, char *supinfo[]);

struct _GnomeTrigger {
	GnomeTriggerType type;
	union {
		/*
		 * These will be passed the same info as
		 * gnome_triggers_do was given.
		 */
		GnomeTriggerActionFunction function;
		gchar *command;
		struct {
			gchar *file;
			int cache_id;
		} media;
	} u;
        gchar *level;
};
typedef struct _GnomeTrigger GnomeTrigger;

/*
 * The optional arguments in some of these functions are just
 * a list of strings that help us know
 * what type of event happened. For example,
 *
 * gnome_triggers_do("System is out of disk space on /dev/hda1!",
 *	             "warning", "system", "device", "disk", "/dev/hda1");
 */

void gnome_triggers_add_trigger  (GnomeTrigger *nt, ...);
void gnome_triggers_vadd_trigger (GnomeTrigger *nt,
				  char *supinfo[]);

void gnome_triggers_do           (const char *msg,
				  const char *level, ...);

void gnome_triggers_vdo          (const char *msg, const char *level,
				  const char *supinfo[]);

G_END_DECLS

#endif /* __GNOME_TRIGGERS_H__ */
