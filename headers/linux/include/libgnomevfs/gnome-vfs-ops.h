/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-ops.h - Synchronous operations for the GNOME Virtual File
   System.

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it> 
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef GNOME_VFS_OPS_H
#define GNOME_VFS_OPS_H

#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-monitor.h>

G_BEGIN_DECLS

GnomeVFSResult	 gnome_vfs_open			(GnomeVFSHandle **handle,
						 const gchar *text_uri,
						 GnomeVFSOpenMode open_mode);

GnomeVFSResult	 gnome_vfs_open_uri		(GnomeVFSHandle **handle,
						 GnomeVFSURI *uri,
						 GnomeVFSOpenMode open_mode);

GnomeVFSResult	 gnome_vfs_create		(GnomeVFSHandle **handle,
						 const gchar *text_uri,
						 GnomeVFSOpenMode open_mode,
						 gboolean exclusive,
						 guint perm);

GnomeVFSResult	 gnome_vfs_create_uri		(GnomeVFSHandle **handle,
						 GnomeVFSURI *uri,
						 GnomeVFSOpenMode open_mode,
						 gboolean exclusive,
						 guint perm);

GnomeVFSResult 	 gnome_vfs_close 		(GnomeVFSHandle *handle);

GnomeVFSResult	 gnome_vfs_read			(GnomeVFSHandle *handle,
						 gpointer buffer,
						 GnomeVFSFileSize bytes,
						 GnomeVFSFileSize *bytes_read);

GnomeVFSResult	 gnome_vfs_write 		(GnomeVFSHandle *handle,
						 gconstpointer buffer,
						 GnomeVFSFileSize bytes,
						 GnomeVFSFileSize *bytes_written);

GnomeVFSResult	 gnome_vfs_seek			(GnomeVFSHandle *handle,
						 GnomeVFSSeekPosition whence,
						 GnomeVFSFileOffset offset);

GnomeVFSResult	 gnome_vfs_tell			(GnomeVFSHandle *handle,
						 GnomeVFSFileSize *offset_return);

GnomeVFSResult	 gnome_vfs_get_file_info	(const gchar *text_uri,
						 GnomeVFSFileInfo *info,
						 GnomeVFSFileInfoOptions options);

GnomeVFSResult	 gnome_vfs_get_file_info_uri    (GnomeVFSURI *uri,
						 GnomeVFSFileInfo *info,
						 GnomeVFSFileInfoOptions options);

GnomeVFSResult	 gnome_vfs_get_file_info_from_handle
						(GnomeVFSHandle *handle,
						 GnomeVFSFileInfo *info,
						 GnomeVFSFileInfoOptions options);

GnomeVFSResult   gnome_vfs_truncate             (const gchar *text_uri,
						 GnomeVFSFileSize length);
GnomeVFSResult   gnome_vfs_truncate_uri         (GnomeVFSURI *uri,
						 GnomeVFSFileSize length);
GnomeVFSResult   gnome_vfs_truncate_handle      (GnomeVFSHandle *handle,
						 GnomeVFSFileSize length);

GnomeVFSResult	 gnome_vfs_make_directory_for_uri
						(GnomeVFSURI *uri, guint perm);
GnomeVFSResult	 gnome_vfs_make_directory	(const gchar *text_uri,
						 guint perm);

GnomeVFSResult	 gnome_vfs_remove_directory_from_uri
						(GnomeVFSURI *uri);
GnomeVFSResult	 gnome_vfs_remove_directory	(const gchar *text_uri);

GnomeVFSResult   gnome_vfs_unlink_from_uri      (GnomeVFSURI *uri);
GnomeVFSResult   gnome_vfs_create_symbolic_link (GnomeVFSURI *uri, 
						 const gchar *target_reference);
GnomeVFSResult   gnome_vfs_unlink               (const gchar *text_uri);

GnomeVFSResult   gnome_vfs_move_uri		(GnomeVFSURI *old_uri,
						 GnomeVFSURI *new_uri,
						 gboolean force_replace);
GnomeVFSResult   gnome_vfs_move 		(const gchar *old_text_uri,
						 const gchar *new_text_uri,
						 gboolean force_replace);

GnomeVFSResult	 gnome_vfs_check_same_fs_uris	(GnomeVFSURI *source_uri,
						 GnomeVFSURI *target_uri,
						 gboolean *same_fs_return);
GnomeVFSResult	 gnome_vfs_check_same_fs	(const gchar *source,
						 const gchar *target,
						 gboolean *same_fs_return);

gboolean	 gnome_vfs_uri_exists		(GnomeVFSURI *uri);

GnomeVFSResult   gnome_vfs_set_file_info_uri    (GnomeVFSURI *uri,
						 GnomeVFSFileInfo *info,
						 GnomeVFSSetFileInfoMask mask);
GnomeVFSResult   gnome_vfs_set_file_info        (const gchar *text_uri,
						 GnomeVFSFileInfo *info,
						 GnomeVFSSetFileInfoMask mask);

GnomeVFSResult gnome_vfs_monitor_add (GnomeVFSMonitorHandle **handle,
                                      const gchar *text_uri,
                                      GnomeVFSMonitorType monitor_type,
                                      GnomeVFSMonitorCallback callback,
                                      gpointer user_data);

GnomeVFSResult gnome_vfs_monitor_cancel (GnomeVFSMonitorHandle *handle);

GnomeVFSResult gnome_vfs_file_control   (GnomeVFSHandle *handle,
					 const char *operation,
					 gpointer operation_data);

GnomeVFSResult gnome_vfs_forget_cache (GnomeVFSHandle *handle,
				       GnomeVFSFileOffset offset,
				       GnomeVFSFileSize size);


G_END_DECLS

#endif /* GNOME_VFS_OPS_H */
