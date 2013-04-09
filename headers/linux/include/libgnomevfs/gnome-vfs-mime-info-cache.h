/* gnome-vfs-mime-info-cache.h
 *
 * Copyright (C) 2004 Red Hat, Inc
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

#ifndef GNOME_VFS_DISABLE_DEPRECATED

#ifndef GNOME_VFS_MIME_INFO_CACHE_H
#define GNOME_VFS_MIME_INFO_CACHE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * DESKTOP_ENTRY_GROUP:
 *
 * The #GKeyFile group used for desktop entries.
 **/
#define DESKTOP_ENTRY_GROUP "Desktop Entry"
	
GList              *gnome_vfs_mime_get_all_desktop_entries (const char *mime_type);
gchar              *gnome_vfs_mime_get_default_desktop_entry (const char *mime_type);

G_END_DECLS

#endif /* GNOME_VFS_MIME_INFO_CACHE_H */

#endif /* GNOME_VFS_DISABLE_DEPRECATED */
