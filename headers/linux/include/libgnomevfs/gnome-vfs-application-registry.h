/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-application-registry.h
 *
 * Copyright (C) 1998 Miguel de Icaza
 * Copyright (C) 2000 Eazel, Inc
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
 * Authors: George Lebl
 * 	Based on original mime-info database code by Miguel de Icaza
 */

/* DEPRECATED 
 *
 * Do not use in new code.
 *
 * Use the api in gnome-vfs-mime-handlers.h instead.
 *
 */
 
#ifndef GNOME_VFS_DISABLE_DEPRECATED

#ifndef GNOME_VFS_APPLICATION_REGISTRY_H
#define GNOME_VFS_APPLICATION_REGISTRY_H

#include <libgnomevfs/gnome-vfs-mime-handlers.h>

G_BEGIN_DECLS
	
/**
 * GNOME_VFS_APPLICATION_REGISTRY_COMMAND:
 *
 * Application registry key for fetching the command to execute
 * an application.
 **/
#define GNOME_VFS_APPLICATION_REGISTRY_COMMAND "command"

/**
 * GNOME_VFS_APPLICATION_REGISTRY_NAME:
 *
 * Application registry key for fetching the name of an application.
 **/
#define GNOME_VFS_APPLICATION_REGISTRY_NAME "name"

/**
 * GNOME_VFS_APPLICATION_REGISTRY_CAN_OPEN_MULTIPLE_FILES:
 *
 * Application registry key for determining if an application 
 * can open multiple files in the same invocation.
 **/
#define GNOME_VFS_APPLICATION_REGISTRY_CAN_OPEN_MULTIPLE_FILES "can_open_multiple_files"

/**
 * GNOME_VFS_APPLICATION_REGISTRY_REQUIRES_TERMINAL:
 *
 * Application registry key for determining if an application
 * needs to run from within a terminal (for example, mpg123)
 **/
#define GNOME_VFS_APPLICATION_REGISTRY_REQUIRES_TERMINAL "requires_terminal"

/** 
 * GNOME_VFS_APPLICATION_REGISTRY_USES_GNOMEVFS
 * 
 * Application registry key for determining if an application
 * is using gnome-vfs and thus can open any URI supported by
 * gnome-vfs
 */
#define GNOME_VFS_APPLICATION_REGISTRY_USES_GNOMEVFS "uses_gnomevfs"

/**
 * GNOME_VFS_APPLICATION_REGISTRY_STARTUP_NOTIFY:
 *
 * Application registry key for determining if an application
 * supports freedesktop.org-style startup notification.
 **/
#define GNOME_VFS_APPLICATION_REGISTRY_STARTUP_NOTIFY "startup_notify"
	
/*
 * Existance check
 */
gboolean	gnome_vfs_application_registry_exists      	(const char *app_id);

/*
 * Getting arbitrary keys
 */
GList      	*gnome_vfs_application_registry_get_keys      	(const char *app_id);
const char 	*gnome_vfs_application_registry_peek_value     	(const char *app_id,
					  	 	 	 const char *key);
gboolean	gnome_vfs_application_registry_get_bool_value 	(const char *app_id,
					  	 	 	 const char *key,
								 gboolean *got_key);

/*
 * Setting stuff
 */
void		gnome_vfs_application_registry_remove_application(const char *app_id);
void		gnome_vfs_application_registry_set_value	(const char *app_id,
								 const char *key,
								 const char *value);
void		gnome_vfs_application_registry_set_bool_value	(const char *app_id,
								 const char *key,
								 gboolean value);
void		gnome_vfs_application_registry_unset_key	(const char *app_id,
								 const char *key);

/*
 * Query functions
 */
GList		*gnome_vfs_application_registry_get_applications(const char *mime_type);
GList		*gnome_vfs_application_registry_get_mime_types	(const char *app_id);

gboolean	gnome_vfs_application_registry_supports_mime_type  (const char *app_id,
								    const char *mime_type);
gboolean        gnome_vfs_application_registry_supports_uri_scheme (const char *app_id,
								    const char *uri_scheme);
gboolean	gnome_vfs_application_is_user_owned_application     (const GnomeVFSMimeApplication *application);

/*
 * Mime type functions
 * Note that mime_type can be a specific (image/png) or generic (image/<star>) type
 */

void		gnome_vfs_application_registry_clear_mime_types		(const char *app_id);
void		gnome_vfs_application_registry_add_mime_type		(const char *app_id,
								 	 const char *mime_type);
void		gnome_vfs_application_registry_remove_mime_type		(const char *app_id,
								 	 const char *mime_type);


/*
 * Commit function, should be called if ANY stuff changes have been made.
 * Stuff is saved into the user directory.
 */
GnomeVFSResult	gnome_vfs_application_registry_sync		(void);

void		gnome_vfs_application_registry_shutdown		(void);
void		gnome_vfs_application_registry_reload		(void);

/*
 * Integrating with gnome-vfs-mime-handlers
 */
GnomeVFSMimeApplication *
		gnome_vfs_application_registry_get_mime_application(const char *app_id);
void		gnome_vfs_application_registry_save_mime_application(const GnomeVFSMimeApplication *application);


G_END_DECLS

#endif /* GNOME_VFS_APPLICATION_REGISTRY_H */

#endif /* GNOME_VFS_DISABLE_DEPRECATED */
