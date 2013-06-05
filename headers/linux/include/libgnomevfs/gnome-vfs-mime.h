/*
 * Copyright (C) 1997-2001 Free Software Foundation
 * Copyright (C) 2000, 2001 Eazel, Inc.
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

#ifndef GNOME_VFS_MIME_H
#define GNOME_VFS_MIME_H

#include <sys/stat.h>

#include <libgnomevfs/gnome-vfs-uri.h>

G_BEGIN_DECLS

struct stat;

#ifndef GNOME_VFS_DISABLE_DEPRECATED
void 	     gnome_vfs_mime_shutdown 				(void);
#endif

#ifndef GNOME_VFS_DISABLE_DEPRECATED
const char  *gnome_vfs_mime_type_from_name			(const char        *filename);
#endif

const char  *gnome_vfs_mime_type_from_name_or_default	        (const char        *filename,
								 const char        *defaultv);

const char  *gnome_vfs_get_mime_type_common			(GnomeVFSURI       *uri);
/* FIXME: This function should be named more clearly, maybe
   to show that it gets the mime type only using the uri information. */
const char  *gnome_vfs_get_mime_type_from_uri			(GnomeVFSURI       *uri);
 
#ifndef GNOME_VFS_DISABLE_DEPRECATED
const char  *gnome_vfs_get_mime_type_from_file_data		(GnomeVFSURI       *uri);
#endif

const char * gnome_vfs_get_file_mime_type_fast      (const char        *path,
						     const struct stat *optional_stat_info);
const char  *gnome_vfs_get_file_mime_type           (const char        *path,
						     const struct stat *optional_stat_info,
						     gboolean           suffix_only);
gboolean     gnome_vfs_mime_type_is_supertype       (const char        *mime_type);
char	    *gnome_vfs_get_supertype_from_mime_type (const char        *mime_type);

void         gnome_vfs_mime_info_cache_reload (const char *dir);
void         gnome_vfs_mime_reload            (void);

G_END_DECLS

#endif
