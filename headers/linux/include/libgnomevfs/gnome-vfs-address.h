/* gnome-vfs-address.h - Address functions

   Copyright (C) 2004 Christian Kellner

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

*/

#ifndef GNOME_VFS_ADDRESS_H
#define GNOME_VFS_ADDRESS_H

#include <libgnomevfs/gnome-vfs-result.h>

#include <glib.h>
#include <glib-object.h>
#ifndef G_OS_WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#undef interface
#endif

G_BEGIN_DECLS

#define GNOME_VFS_TYPE_ADDRESS  (gnome_vfs_address_get_type ())

typedef struct _GnomeVFSAddress GnomeVFSAddress;

GType            gnome_vfs_address_get_type          (void);

GnomeVFSAddress *gnome_vfs_address_new_from_string   (const char      *address);
GnomeVFSAddress *gnome_vfs_address_new_from_ipv4     (guint32          ipv4_address);
GnomeVFSAddress *gnome_vfs_address_new_from_sockaddr (struct sockaddr *sa,
						      int              len);

int              gnome_vfs_address_get_family_type   (GnomeVFSAddress *address);
char *           gnome_vfs_address_to_string         (GnomeVFSAddress *address);
guint32          gnome_vfs_address_get_ipv4          (GnomeVFSAddress *address);
struct sockaddr *gnome_vfs_address_get_sockaddr      (GnomeVFSAddress *address,
						      guint16          port,
						      int             *len);

gboolean         gnome_vfs_address_equal             (const GnomeVFSAddress *a,
						      const GnomeVFSAddress *b);

gboolean         gnome_vfs_address_match             (const GnomeVFSAddress *a,
						      const GnomeVFSAddress *b,
						      guint             prefix);

GnomeVFSAddress *gnome_vfs_address_dup               (GnomeVFSAddress *address);
void             gnome_vfs_address_free              (GnomeVFSAddress *address);

G_END_DECLS

#endif
