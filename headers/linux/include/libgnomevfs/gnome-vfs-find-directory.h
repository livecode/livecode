/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-find-directory.h - Special directory location functions for
   the GNOME Virtual File System.

   Copyright (C) 2000 Eazel, Inc.
   Copyright (C) 2001 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Pavel Cisler <pavel@eazel.com>
            Seth Nickell <snickell@stanford.edu>
*/

#ifndef GNOME_VFS_FIND_DIRECTORY_H
#define GNOME_VFS_FIND_DIRECTORY_H

#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-uri.h>

G_BEGIN_DECLS

/**
 * GnomeVFSFindDirectoryKind:
 * @GNOME_VFS_DIRECTORY_KIND_DESKTOP: Desktop directory.
 * @GNOME_VFS_DIRECTORY_KIND_TRASH: Trash directory.
 *
 * Specifies what directory gnome_vfs_find_directory() or
 * gnome_vfs_find_directory_cancellable() should find.
 **/
typedef enum {
	GNOME_VFS_DIRECTORY_KIND_DESKTOP = 1000,
	GNOME_VFS_DIRECTORY_KIND_TRASH = 1001
} GnomeVFSFindDirectoryKind;

GnomeVFSResult gnome_vfs_find_directory (GnomeVFSURI                *near_uri,
					 GnomeVFSFindDirectoryKind   kind,
					 GnomeVFSURI               **result,
					 gboolean                    create_if_needed,
					 gboolean                    find_if_needed,
					 guint                       permissions);

G_END_DECLS

#endif
