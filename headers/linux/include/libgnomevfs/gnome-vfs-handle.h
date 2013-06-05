/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-handle.h - Handle object for GNOME VFS files.

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef GNOME_VFS_HANDLE_H
#define GNOME_VFS_HANDLE_H

#include <libgnomevfs/gnome-vfs-context.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

G_BEGIN_DECLS

/**
 * GnomeVFSMethodHandle:
 *
 * Generic handle (pointer) that encapsulates the method internals
 * associated with a specific resource.
 *
 * Methods are supposed to store all information and open resources
 * regarding a handle in a dynamically allocated structure, which
 * is casted into a #GnomeVFSMethodHandle (in fact a void pointer)
 * when it is passed around in generic GnomeVFS functions.
 * During the lifetime of a handle, GnomeVFS stores it as a
 * generic pointer. After closing a handle, it will not be
 * known to GnomeVFS anymore.
 *
 * There are file handles and file monitor handles.
 *
 * File handles are opened in the #GnomeVFSMethod's
 * #GnomeVFSMethodOpenFunc, #GnomeVFSMethodCreateFunc
 * and #GnomeVFSMethodOpenDirectoryFunc
 * and closed in its #GnomeVFSMethodCloseFunc and
 * #GnomeVFSMethodCloseDirectoryFunc.
 *
 * A number of #GnomeVFSMethod operations rely on file handles:
 *
 * #GnomeVFSMethodReadFunc
 * #GnomeVFSMethodWriteFunc
 * #GnomeVFSMethodSeekFunc
 * #GnomeVFSMethodTellFunc
 * #GnomeVFSMethodGetFileInfoFromHandleFunc
 * #GnomeVFSMethodTruncateHandleFunc
 * #GnomeVFSMethodFileControlFunc
 *
 * File monitor handles however are created using
 * #GnomeVFSMethodMonitorAddFunc
 * and cancelled using
 * #GnomeVFSMethodMonitorCancelFunc
 **/
typedef gpointer GnomeVFSMethodHandle;

/**
 * GnomeVFSHandle:
 *
 * Handle to a file.
 *
 * A handle is obtained using gnome_vfs_open() and gnome_vfs_create()
 * family of functions on the file. A handle represents a file stream, gnome_vfs_close(),
 * gnome_vfs_write(), gnome_vfs_read() and all the other I/O operations take a
 * GnomeVFSHandle * that identifies the file where the operation is going to be
 * performed.
 **/
typedef struct GnomeVFSHandle GnomeVFSHandle;

/**
 * GnomeVFSOpenMode:
 * @GNOME_VFS_OPEN_NONE: No access.
 * @GNOME_VFS_OPEN_READ: Read access.
 * @GNOME_VFS_OPEN_WRITE: Write access.
 * @GNOME_VFS_OPEN_RANDOM: Random access.
 * @GNOME_VFS_OPEN_TRUNCATE: Truncate file before accessing it, i.e. delete its contents.
 *
 * Mode in which files are opened. If GNOME_VFS_OPEN_RANDOM is not used, the
 * file will be have to be accessed sequentially.
 **/
typedef enum {
        GNOME_VFS_OPEN_NONE = 0,
        GNOME_VFS_OPEN_READ = 1 << 0,
        GNOME_VFS_OPEN_WRITE = 1 << 1,
        GNOME_VFS_OPEN_RANDOM = 1 << 2,
	GNOME_VFS_OPEN_TRUNCATE = 1 << 3
} GnomeVFSOpenMode;

/**
 * GnomeVFSSeekPosition:
 * @GNOME_VFS_SEEK_START: Start of the file.
 * @GNOME_VFS_SEEK_CURRENT: Current position.
 * @GNOME_VFS_SEEK_END: End of the file.
 *
 * This is used to specify the start position for seek operations.
 **/
typedef enum {
        GNOME_VFS_SEEK_START,
        GNOME_VFS_SEEK_CURRENT,
        GNOME_VFS_SEEK_END
} GnomeVFSSeekPosition;


G_END_DECLS

#endif /* GNOME_VFS_HANDLE_H */
