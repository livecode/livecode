/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-monitor.h - File Monitoring for the GNOME Virtual File System.

   Copyright (C) 2001 Ian McKellar

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

   Author: Ian McKellar <yakk@yakk.net>
*/

#ifndef GNOME_VFS_MONITOR_H
#define GNOME_VFS_MONITOR_H

#include <glib.h>

/**
 * GnomeVFSMonitorType:
 * @GNOME_VFS_MONITOR_FILE: the monitor is registered for a single file.
 * @GNOME_VFS_MONITOR_DIRECTORY: the monitor is registered for all files in a directory,
 * 				 and the directory itself.
 *
 * Type of resources that can be monitored.
 **/

typedef enum {
  GNOME_VFS_MONITOR_FILE,
  GNOME_VFS_MONITOR_DIRECTORY
} GnomeVFSMonitorType;

/**
 * GnomeVFSMonitorEventType:
 * @GNOME_VFS_MONITOR_EVENT_CHANGED: file data changed (FAM, inotify).
 * @GNOME_VFS_MONITOR_EVENT_DELETED: file deleted event (FAM, inotify).
 * @GNOME_VFS_MONITOR_EVENT_STARTEXECUTING: file was executed (FAM only).
 * @GNOME_VFS_MONITOR_EVENT_STOPEXECUTING: executed file isn't executed anymore (FAM only).
 * @GNOME_VFS_MONITOR_EVENT_CREATED: file created event (FAM, inotify).
 * @GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED: file metadata changed (inotify only).
 * 
 * Types of events that can be monitored.
 **/

typedef enum {
  GNOME_VFS_MONITOR_EVENT_CHANGED,
  GNOME_VFS_MONITOR_EVENT_DELETED,
  GNOME_VFS_MONITOR_EVENT_STARTEXECUTING,
  GNOME_VFS_MONITOR_EVENT_STOPEXECUTING,
  GNOME_VFS_MONITOR_EVENT_CREATED,
  GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED
} GnomeVFSMonitorEventType;

/**
 * GnomeVFSMonitorHandle:
 *
 * a handle representing a file or directory monitor that
 * was registered using gnome_vfs_monitor_add() and that
 * can be cancelled using gnome_vfs_monitor_cancel().
 **/
typedef struct GnomeVFSMonitorHandle GnomeVFSMonitorHandle;

/**
 * GnomeVFSMonitorCallback:
 * @handle: the handle of the monitor that created the event
 * @monitor_uri: the URI of the monitor that was triggered
 * @info_uri: the URI of the actual file this event is concerned with (this can be different
 * from @monitor_uri if it was a directory monitor)
 * @event_type: what happened to @info_uri
 * @user_data: user data passed to gnome_vfs_monitor_add() when the monitor was created
 *
 * Function called when a monitor detects a change.
 *
 **/
typedef void (* GnomeVFSMonitorCallback) (GnomeVFSMonitorHandle *handle,
                                          const gchar *monitor_uri,
                                          const gchar *info_uri,
                                          GnomeVFSMonitorEventType event_type,
                                          gpointer user_data);
#endif /* GNOME_VFS_MONITOR_H */
