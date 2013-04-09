/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   gnome-vfs-mime-monitor.h: Class for noticing changes in MIME data.
 
   Copyright (C) 2000 Eazel, Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
  
   You should have received a copy of the GNU Library General Public
   License along with this program; see the file COPYING.LIB. If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: John Sullivan <sullivan@eazel.com>
*/

#ifndef GNOME_VFS_MIME_MONITOR_H
#define GNOME_VFS_MIME_MONITOR_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GNOME_VFS_MIME_MONITOR_TYPE        (gnome_vfs_mime_monitor_get_type ())
#define GNOME_VFS_MIME_MONITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_VFS_MIME_MONITOR_TYPE, GnomeVFSMIMEMonitor))
#define GNOME_VFS_MIME_MONITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_VFS_MIME_MONITOR_TYPE, GnomeVFSMIMEMonitorClass))
#define GNOME_VFS_IS_MIME_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_VFS_MIME_MONITOR_TYPE))
#define GNOME_VFS_IS_MIME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_VFS_MIME_MONITOR_TYPE))

typedef struct _GnomeVFSMIMEMonitorPrivate GnomeVFSMIMEMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject object;

	GnomeVFSMIMEMonitorPrivate *priv;
} GnomeVFSMIMEMonitor;

typedef struct {
	GObjectClass parent_class;

	/* signals */
	void (*data_changed) (GnomeVFSMIMEMonitor *monitor);
} GnomeVFSMIMEMonitorClass;

GType                gnome_vfs_mime_monitor_get_type (void);

/* There's a single GnomeVFSMIMEMonitor object.
 * The only thing you need it for is to connect to its signals.
 */
GnomeVFSMIMEMonitor *gnome_vfs_mime_monitor_get      (void);

G_END_DECLS

#endif /* GNOME_VFS_MIME_MONITOR_H */
