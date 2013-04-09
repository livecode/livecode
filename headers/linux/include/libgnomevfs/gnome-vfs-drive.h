/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-drive.h - Handling of drives for the GNOME Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef GNOME_VFS_DRIVE_H
#define GNOME_VFS_DRIVE_H

#include <glib-object.h>
#include <libgnomevfs/gnome-vfs-volume.h>

G_BEGIN_DECLS

#define GNOME_VFS_TYPE_DRIVE        (gnome_vfs_drive_get_type ())
#define GNOME_VFS_DRIVE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_VFS_TYPE_DRIVE, GnomeVFSDrive))
#define GNOME_VFS_DRIVE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_VFS_TYPE_DRIVE, GnomeVFSDriveClass))
#define GNOME_IS_VFS_DRIVE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_VFS_TYPE_DRIVE))
#define GNOME_IS_VFS_DRIVE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_VFS_TYPE_DRIVE))

typedef struct _GnomeVFSDrivePrivate GnomeVFSDrivePrivate;

struct _GnomeVFSDrive {
	GObject parent;

	/*< private >*/
        GnomeVFSDrivePrivate *priv;
};

typedef struct _GnomeVFSDriveClass GnomeVFSDriveClass;

struct _GnomeVFSDriveClass {
	GObjectClass parent_class;

	void (* volume_mounted)	  	(GnomeVFSDrive *drive,
				   	 GnomeVFSVolume	*volume);
	void (* volume_pre_unmount)	(GnomeVFSDrive *drive,
				   	 GnomeVFSVolume	*volume);
	void (* volume_unmounted)	(GnomeVFSDrive *drive,
				   	 GnomeVFSVolume	*volume);
};

GType gnome_vfs_drive_get_type (void) G_GNUC_CONST;

GnomeVFSDrive *gnome_vfs_drive_ref   (GnomeVFSDrive *drive);
void           gnome_vfs_drive_unref (GnomeVFSDrive *drive);
void           gnome_vfs_drive_volume_list_free (GList *volumes);


gulong             gnome_vfs_drive_get_id              (GnomeVFSDrive *drive);
GnomeVFSDeviceType gnome_vfs_drive_get_device_type     (GnomeVFSDrive *drive);

#ifndef GNOME_VFS_DISABLE_DEPRECATED
GnomeVFSVolume *   gnome_vfs_drive_get_mounted_volume  (GnomeVFSDrive *drive);
#endif /*GNOME_VFS_DISABLE_DEPRECATED*/

GList *            gnome_vfs_drive_get_mounted_volumes (GnomeVFSDrive *drive);
char *             gnome_vfs_drive_get_device_path     (GnomeVFSDrive *drive);
char *             gnome_vfs_drive_get_activation_uri  (GnomeVFSDrive *drive);
char *             gnome_vfs_drive_get_display_name    (GnomeVFSDrive *drive);
char *             gnome_vfs_drive_get_icon            (GnomeVFSDrive *drive);
char *             gnome_vfs_drive_get_hal_udi         (GnomeVFSDrive *drive);
gboolean           gnome_vfs_drive_is_user_visible     (GnomeVFSDrive *drive);
gboolean           gnome_vfs_drive_is_connected        (GnomeVFSDrive *drive);
gboolean           gnome_vfs_drive_is_mounted          (GnomeVFSDrive *drive);
gboolean           gnome_vfs_drive_needs_eject         (GnomeVFSDrive *drive);

gint               gnome_vfs_drive_compare             (GnomeVFSDrive *a,
							GnomeVFSDrive *b);

void gnome_vfs_drive_mount   (GnomeVFSDrive             *drive,
			      GnomeVFSVolumeOpCallback   callback,
			      gpointer                   user_data);
void gnome_vfs_drive_unmount (GnomeVFSDrive             *drive,
			      GnomeVFSVolumeOpCallback   callback,
			      gpointer                   user_data);
void gnome_vfs_drive_eject   (GnomeVFSDrive             *drive,
			      GnomeVFSVolumeOpCallback   callback,
			      gpointer                   user_data);

G_END_DECLS
#endif /* GNOME_VFS_DRIVE_H */

