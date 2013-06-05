/* gnome-vfs-resolve.h - Resolver API
 *
 * Copyright (C) 2004 Christian Kellner
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.

 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GNOME_VFS_RESOLVE_H
#define GNOME_VFS_RESOLVE_H

#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-address.h>

G_BEGIN_DECLS

typedef struct GnomeVFSResolveHandle_ GnomeVFSResolveHandle;

GnomeVFSResult gnome_vfs_resolve              (const char              *hostname,
									  GnomeVFSResolveHandle  **handle);
gboolean       gnome_vfs_resolve_next_address (GnomeVFSResolveHandle   *handle,
									  GnomeVFSAddress        **address);
void           gnome_vfs_resolve_reset_to_beginning
                                              (GnomeVFSResolveHandle   *handle);
void           gnome_vfs_resolve_free         (GnomeVFSResolveHandle   *handle);

G_END_DECLS

#endif /* ! GNOME_VFS_RESOLVE_H */
