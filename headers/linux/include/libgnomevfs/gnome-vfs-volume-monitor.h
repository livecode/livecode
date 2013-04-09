/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor.h - Handling of volumes for the GNOME Virtual File System.

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

#ifndef GNOME_VFS_VOLUME_MONITOR_H
#define GNOME_VFS_VOLUME_MONITOR_H

#include <glib-object.h>
#include <libgnomevfs/gnome-vfs-volume.h>

G_BEGIN_DECLS

#define GNOME_VFS_TYPE_VOLUME_MONITOR        (gnome_vfs_volume_monitor_get_type ())
#define GNOME_VFS_VOLUME_MONITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_VFS_TYPE_VOLUME_MONITOR, GnomeVFSVolumeMonitor))
#define GNOME_VFS_VOLUME_MONITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_VFS_TYPE_VOLUME_MONITOR, GnomeVFSVolumeMonitorClass))
#define GNOME_IS_VFS_VOLUME_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_VFS_TYPE_VOLUME_MONITOR))
#define GNOME_IS_VFS_VOLUME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_VFS_TYPE_VOLUME_MONITOR))

typedef struct _GnomeVFSVolumeMonitorPrivate GnomeVFSVolumeMonitorPrivate;

typedef struct _GnomeVFSVolumeMonitor GnomeVFSVolumeMonitor;
typedef struct _GnomeVFSVolumeMonitorClass GnomeVFSVolumeMonitorClass;

struct _GnomeVFSVolumeMonitor {
	GObject parent;

        GnomeVFSVolumeMonitorPrivate *priv;
};

struct _GnomeVFSVolumeMonitorClass {
	GObjectClass parent_class;

	/*< public >*/
	/* signals */
	void (* volume_mounted)	  	(GnomeVFSVolumeMonitor *volume_monitor,
				   	 GnomeVFSVolume	       *volume);
	void (* volume_pre_unmount)	(GnomeVFSVolumeMonitor *volume_monitor,
				   	 GnomeVFSVolume	       *volume);
	void (* volume_unmounted)	(GnomeVFSVolumeMonitor *volume_monitor,
				   	 GnomeVFSVolume	       *volume);
	void (* drive_connected) 	(GnomeVFSVolumeMonitor *volume_monitor,
				   	 GnomeVFSDrive	       *drive);
	void (* drive_disconnected)	(GnomeVFSVolumeMonitor *volume_monitor,
				   	 GnomeVFSDrive         *drive);
};

GType gnome_vfs_volume_monitor_get_type (void) G_GNUC_CONST;

GnomeVFSVolumeMonitor *gnome_vfs_get_volume_monitor   (void);
GnomeVFSVolumeMonitor *gnome_vfs_volume_monitor_ref   (GnomeVFSVolumeMonitor *volume_monitor);
void                   gnome_vfs_volume_monitor_unref (GnomeVFSVolumeMonitor *volume_monitor);

GList *         gnome_vfs_volume_monitor_get_mounted_volumes  (GnomeVFSVolumeMonitor *volume_monitor);
GList *         gnome_vfs_volume_monitor_get_connected_drives (GnomeVFSVolumeMonitor *volume_monitor);
GnomeVFSVolume *gnome_vfs_volume_monitor_get_volume_for_path  (GnomeVFSVolumeMonitor *volume_monitor,
							       const char            *path);
GnomeVFSVolume *gnome_vfs_volume_monitor_get_volume_by_id     (GnomeVFSVolumeMonitor *volume_monitor,
							       gulong                 id);
GnomeVFSDrive * gnome_vfs_volume_monitor_get_drive_by_id      (GnomeVFSVolumeMonitor *volume_monitor,
							       gulong                 id);

G_END_DECLS

#endif /* GNOME_VFS_VOLUME_MONITOR_H */
