/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
   Copyright (C) 2000 Eazel, Inc.

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

   Author: Maciej Stachowiak <mjs@eazel.com> */

#ifndef GNOME_VFS_DISABLE_DEPRECATED

#ifndef GNOME_VFS_MIME_DEPRECATED_H
#define GNOME_VFS_MIME_DEPRECATED_H

G_BEGIN_DECLS

/* ------------------------------
 * From gnome-vfs-mime-handlers.h 
 * ------------------------------
 */

/**
 * GnomeVFSMimeActionType:
 * @GNOME_VFS_MIME_ACTION_TYPE_NONE: neither an application nor a component.
 * @GNOME_VFS_MIME_ACTION_TYPE_APPLICATION: an application.
 * @GNOME_VFS_MIME_ACTION_TYPE_COMPONENT: a component.
 *
 * This is used to specify the %type of a #GnomeVFSMimeAction.
 **/
typedef enum {
	GNOME_VFS_MIME_ACTION_TYPE_NONE,
	GNOME_VFS_MIME_ACTION_TYPE_APPLICATION,
	GNOME_VFS_MIME_ACTION_TYPE_COMPONENT
} GnomeVFSMimeActionType;

/**
 * GnomeVFSMimeAction:
 * @action_type: The #GnomeVFSMimeActionType describing the type of this action.
 *
 * This data structure describes an action that can be done 
 * on a file.
 **/
typedef struct _GnomeVFSMimeAction GnomeVFSMimeAction;

struct _GnomeVFSMimeAction {
	/* <public >*/
	GnomeVFSMimeActionType action_type;
	union {
		void *component;
		void *dummy_component;
		GnomeVFSMimeApplication *application;
	} action;

	/*< private >*/
	/* Padded to avoid future breaks in ABI compatibility */
	void *reserved1;
};

GnomeVFSMimeActionType   gnome_vfs_mime_get_default_action_type            (const char              *mime_type);
GnomeVFSMimeAction *     gnome_vfs_mime_get_default_action                 (const char              *mime_type);
GnomeVFSMimeApplication *gnome_vfs_mime_application_new_from_id            (const char              *id);
void                     gnome_vfs_mime_action_free                        (GnomeVFSMimeAction      *action);

GnomeVFSResult           gnome_vfs_mime_action_launch                      (GnomeVFSMimeAction      *action,
									    GList                   *uris);
GnomeVFSResult           gnome_vfs_mime_action_launch_with_env             (GnomeVFSMimeAction      *action,
									    GList                   *uris,
									    char                   **envp);
									    
const char  		*gnome_vfs_mime_get_icon 			   (const char 		    *mime_type);

/* List manipulation helper functions */
gboolean                 gnome_vfs_mime_id_in_application_list             (const char              *id,
									    GList                   *applications);
GList *                  gnome_vfs_mime_remove_application_from_list       (GList                   *applications,
									    const char              *application_id,
									    gboolean                *did_remove);
GList *                  gnome_vfs_mime_id_list_from_application_list      (GList                   *applications);

/* Stored as delta to current user level - API function computes delta and stores in prefs */
GnomeVFSResult           gnome_vfs_mime_add_extension                      (const char              *mime_type,
									    const char              *extension);
GnomeVFSResult           gnome_vfs_mime_remove_extension                   (const char              *mime_type,
									    const char              *extension);
GnomeVFSResult           gnome_vfs_mime_set_default_action_type            (const char              *mime_type,
									    GnomeVFSMimeActionType   action_type);
GnomeVFSResult           gnome_vfs_mime_set_default_application            (const char              *mime_type,
									    const char              *application_id);
GnomeVFSResult           gnome_vfs_mime_set_default_component              (const char              *mime_type,
									    const char              *component_iid);
GnomeVFSResult  	 gnome_vfs_mime_set_icon 			   (const char 		    *mime_type,
									    const char		    *filename);
GnomeVFSResult		 gnome_vfs_mime_set_description			   (const char		    *mime_type,
									    const char		    *description);

GnomeVFSResult	 	 gnome_vfs_mime_set_can_be_executable   	   (const char              *mime_type,
									    gboolean		     new_value);


/* No way to override system list; can only add. */
GnomeVFSResult           gnome_vfs_mime_extend_all_applications            (const char              *mime_type,
									    GList                   *application_ids);
/* Only "user" entries may be removed. */
GnomeVFSResult           gnome_vfs_mime_remove_from_all_applications       (const char              *mime_type,
									    GList                   *application_ids);


GList *                  gnome_vfs_mime_get_short_list_applications        (const char              *mime_type);
GnomeVFSResult           gnome_vfs_mime_set_short_list_applications        (const char              *mime_type,
									    GList                   *application_ids);
GnomeVFSResult           gnome_vfs_mime_set_short_list_components          (const char              *mime_type,
									    GList                   *component_iids);
GnomeVFSResult           gnome_vfs_mime_add_application_to_short_list      (const char              *mime_type,
									    const char              *application_id);
GnomeVFSResult           gnome_vfs_mime_remove_application_from_short_list (const char              *mime_type,
									    const char              *application_id);
GnomeVFSResult           gnome_vfs_mime_add_component_to_short_list        (const char              *mime_type,
									    const char              *iid);
GnomeVFSResult           gnome_vfs_mime_remove_component_from_short_list   (const char              *mime_type,
									    const char              *iid);


/* There are actually in bonobo-activation, but defined here */
#if defined(GNOME_VFS_INCLUDE_BONOBO) || defined(_Bonobo_ServerInfo_defined)
#include <bonobo-activation/bonobo-activation-server-info.h>
Bonobo_ServerInfo *gnome_vfs_mime_get_default_component (const char *mime_type);
#else
void *gnome_vfs_mime_get_default_component (const char *mime_type);
#endif

GList *  gnome_vfs_mime_get_all_components          (const char *mime_type);
void     gnome_vfs_mime_component_list_free         (GList      *list);
GList *  gnome_vfs_mime_remove_component_from_list  (GList      *components,
						     const char *iid,
						     gboolean   *did_remove);
GList *  gnome_vfs_mime_id_list_from_component_list (GList      *components);
gboolean gnome_vfs_mime_id_in_component_list        (const char *iid,
						     GList      *components);
GList *  gnome_vfs_mime_get_short_list_components   (const char *mime_type);

G_END_DECLS

#endif /* GNOME_VFS_MIME_DEPRECATED_H */

#endif /* GNOME_VFS_DISABLE_DEPRECATED */
