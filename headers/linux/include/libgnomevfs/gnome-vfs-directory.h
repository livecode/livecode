/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-directory.h - Directory handling for the GNOME Virtual
   File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef GNOME_VFS_DIRECTORY_H
#define GNOME_VFS_DIRECTORY_H

#include <glib/gmacros.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

G_BEGIN_DECLS

typedef struct GnomeVFSDirectoryHandle GnomeVFSDirectoryHandle;

/**
 * GnomeVFSDirectoryVisitOptions:
 * @GNOME_VFS_DIRECTORY_VISIT_DEFAULT: Default options, i.e. call 
 * the specified #GnomeVFSDirectoryVisitFunc for each file.
 * @GNOME_VFS_DIRECTORY_VISIT_SAMEFS: Visit only directories on the same 
 * file system as the parent
 * @GNOME_VFS_DIRECTORY_VISIT_LOOPCHECK: Loop prevention. If this
 * is set, and a file is found to be a directory referencing a prefiously 
 * found directory inode (i.e. already used for one of it's parents), this
 * is considered a recursion loop, and #GnomeVFSDirectoryVisitFunc will
 * be notified using its @recursing_will_loop parameter. If this is not
 * set, the #GnomeVFSDirectoryVisitFunc's @recursing_will_loop parameter
 * will always be set to %FALSE.
 *
 * These options control the way in which directories are visited. They are
 * passed to gnome_vfs_directory_visit(), gnome_vfs_directory_visit_uri()
 * gnome_vfs_directory_visit_files() and
 * gnome_vfs_directory_visit_files_at_uri().
 **/
typedef enum {
	GNOME_VFS_DIRECTORY_VISIT_DEFAULT = 0,
	GNOME_VFS_DIRECTORY_VISIT_SAMEFS = 1 << 0,
	GNOME_VFS_DIRECTORY_VISIT_LOOPCHECK = 1 << 1
} GnomeVFSDirectoryVisitOptions;

/**
 * GnomeVFSDirectoryVisitFunc:
 * @rel_path: A char * specifying the path of the currently visited
 * 	      file relative to the base directory for the visit.
 * @info: The #GnomeVFSFileInfo of the currently visited file.
 * @recursing_will_loop: Whether setting *@recurse to %TRUE will cause
 * 			 a loop, i.e. whether this is a link
 * 			 pointing to a parent directory.
 * @user_data: The user data passed to gnome_vfs_directory_visit(),
 * 	       gnome_vfs_directory_visit_uri(), gnome_vfs_directory_visit_files() or
 * 	       gnome_vfs_directory_visit_files_at_uri().
 * @recurse: A valid pointer to a #gboolean, which points to %FALSE by
 * 	     default and can be modified to point to %TRUE, which indicates that
 * 	     the currently considered file should be visited recursively.
 * 	     The recursive visit will only be actually done if @info refers to a directory,
 * 	     *@recurse is %TRUE and the return value of the #GnomeVFSDirectoryVisitFunc is %TRUE.
 * 	     *@recurse should usually not be set to %TRUE if @recursing_will_loop is %TRUE.
 *
 * This function is passed to gnome_vfs_directory_visit(),
 * gnome_vfs_directory_visit_uri(), gnome_vfs_directory_visit_files() and
 * gnome_vfs_directory_visit_files_at_uri(), and is called for each
 * file in the specified directory.
 *
 * <note>
 *  When a recursive visit was requested for a particular directory, errors
 *  during the child visit will lead to a cancellation of the overall visit.
 *  Thus, you must ensure that the user has sufficient access rights to visit
 *  a directory before requesting a recursion.
 * </note>
 *
 * Returns: %TRUE if visit should be continued, %FALSE otherwise.
 **/
typedef gboolean (* GnomeVFSDirectoryVisitFunc)	 (const gchar *rel_path,
						  GnomeVFSFileInfo *info,
						  gboolean recursing_will_loop,
						  gpointer user_data,
						  gboolean *recurse);

GnomeVFSResult	gnome_vfs_directory_open
					(GnomeVFSDirectoryHandle **handle,
					 const gchar              *text_uri,
					 GnomeVFSFileInfoOptions   options);
GnomeVFSResult	gnome_vfs_directory_open_from_uri
					(GnomeVFSDirectoryHandle **handle,
					 GnomeVFSURI              *uri,
					 GnomeVFSFileInfoOptions   options);
GnomeVFSResult	gnome_vfs_directory_read_next
					(GnomeVFSDirectoryHandle  *handle,
					 GnomeVFSFileInfo         *file_info);
GnomeVFSResult	gnome_vfs_directory_close
					(GnomeVFSDirectoryHandle  *handle);


GnomeVFSResult  gnome_vfs_directory_visit
					(const gchar *text_uri,
					 GnomeVFSFileInfoOptions info_options,
					 GnomeVFSDirectoryVisitOptions visit_options,
					 GnomeVFSDirectoryVisitFunc callback,
					 gpointer data);

GnomeVFSResult  gnome_vfs_directory_visit_uri
					(GnomeVFSURI *uri,
					 GnomeVFSFileInfoOptions info_options,
					 GnomeVFSDirectoryVisitOptions visit_options,
					 GnomeVFSDirectoryVisitFunc callback,
					 gpointer data);

GnomeVFSResult	gnome_vfs_directory_visit_files
					(const gchar *text_uri,
					 GList *file_list,
					 GnomeVFSFileInfoOptions info_options,
					 GnomeVFSDirectoryVisitOptions visit_options,
					 GnomeVFSDirectoryVisitFunc callback,
					 gpointer data);

GnomeVFSResult	gnome_vfs_directory_visit_files_at_uri
					(GnomeVFSURI *uri,
					 GList *file_list,
					 GnomeVFSFileInfoOptions info_options,
					 GnomeVFSDirectoryVisitOptions visit_options,
					 GnomeVFSDirectoryVisitFunc callback,
					 gpointer data);

GnomeVFSResult gnome_vfs_directory_list_load
					(GList **list,
				         const gchar *text_uri,
				         GnomeVFSFileInfoOptions options);

G_END_DECLS

#endif
