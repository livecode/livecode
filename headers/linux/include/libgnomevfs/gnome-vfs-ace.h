/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-acl.h - ACL Handling for the GNOME Virtual File System.
   Access Control Entry Class

   Copyright (C) 2005 Christian Kellner

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

   Author: Christian Kellner <gicmo@gnome.org>
*/

#ifndef GNOME_VFS_ACE_H
#define GNOME_VFS_ACE_H

#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS


#define GNOME_VFS_TYPE_ACE             (gnome_vfs_ace_get_type ())
#define GNOME_VFS_ACE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj),  GNOME_VFS_TYPE_ACE, GnomeVFSACE))
#define GNOME_VFS_ACE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),   GNOME_VFS_TYPE_ACE, GnomeVFSACEClass))
#define GNOME_VFS_IS_ACE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  GNOME_VFS_TYPE_ACE))
#define GNOME_VFS_IS_ACE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),   GNOME_VFS_TYPE_ACE))

typedef struct _GnomeVFSACE        GnomeVFSACE;
typedef struct _GnomeVFSACEClass   GnomeVFSACEClass;

typedef struct _GnomeVFSACEPrivate GnomeVFSACEPrivate;

struct _GnomeVFSACE {
	GObject parent;
	
	GnomeVFSACEPrivate *priv;
};

struct _GnomeVFSACEClass {
	GObjectClass parent_class;
    
	void (*reserved1) (void);
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);
};


typedef guint32 GnomeVFSACLKind;
typedef guint32 GnomeVFSACLPerm;


GType                   gnome_vfs_ace_get_type       (void) G_GNUC_CONST;

GnomeVFSACE *           gnome_vfs_ace_new            (GnomeVFSACLKind  kind,
                                                      const char      *id,
                                                      GnomeVFSACLPerm *perms);

GnomeVFSACLKind         gnome_vfs_ace_get_kind       (GnomeVFSACE      *entry);
void                    gnome_vfs_ace_set_kind       (GnomeVFSACE      *entry,
                                                      GnomeVFSACLKind   kind);

const char *            gnome_vfs_ace_get_id         (GnomeVFSACE      *entry);
void                    gnome_vfs_ace_set_id         (GnomeVFSACE      *entry,
                                                      const char       *id);

gboolean                gnome_vfs_ace_get_inherit    (GnomeVFSACE *entry);
void                    gnome_vfs_ace_set_inherit    (GnomeVFSACE *entry,
						      gboolean     inherit);

gboolean                gnome_vfs_ace_get_negative   (GnomeVFSACE *entry);
void                    gnome_vfs_ace_set_negative   (GnomeVFSACE *entry,
						      gboolean     negative);

const GnomeVFSACLPerm * gnome_vfs_ace_get_perms      (GnomeVFSACE      *entry);
void                    gnome_vfs_ace_set_perms      (GnomeVFSACE      *entry,
                                                      GnomeVFSACLPerm  *perms);
void                    gnome_vfs_ace_add_perm       (GnomeVFSACE      *entry,
                                                      GnomeVFSACLPerm   perm);
void                    gnome_vfs_ace_del_perm       (GnomeVFSACE      *entry,
						      GnomeVFSACLPerm   perm);
gboolean                gnome_vfs_ace_check_perm     (GnomeVFSACE      *entry,
                                                      GnomeVFSACLPerm   perm);
void                    gnome_vfs_ace_copy_perms     (GnomeVFSACE      *source,
                                                      GnomeVFSACE      *dest);

gboolean                gnome_vfs_ace_equal          (GnomeVFSACE  *entry_a,
                                                      GnomeVFSACE  *entry_b);

G_END_DECLS

#endif /*GNOME_VFS_ACE_H*/

