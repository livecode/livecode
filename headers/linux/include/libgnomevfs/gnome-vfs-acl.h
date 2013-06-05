/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-acl.h - ACL Handling for the GNOME Virtual File System.
   Access Control List Object

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

#ifndef GNOME_VFS_ACL_H
#define GNOME_VFS_ACL_H

#include <glib.h>
#include <glib-object.h>

#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-ace.h>

G_BEGIN_DECLS

/* ************************************************************************** */

/* ACL Kinds */

const char *  gnome_vfs_acl_kind_to_string   (GnomeVFSACLKind kind);

enum {
	GNOME_VFS_ACL_KIND_NULL = 0,
	GNOME_VFS_ACL_USER,
	GNOME_VFS_ACL_GROUP,
	GNOME_VFS_ACL_OTHER,
	GNOME_VFS_ACL_MASK,
	GNOME_VFS_ACL_KIND_SYS_LAST
};

/* ACL Permissions */

const char *  gnome_vfs_acl_perm_to_string   (GnomeVFSACLPerm perm);

enum {
	GNOME_VFS_ACL_PERM_NULL = 0,
	GNOME_VFS_ACL_READ      = 1,
	GNOME_VFS_ACL_WRITE,
	GNOME_VFS_ACL_EXECUTE, 
	GNOME_VFS_ACL_PERM_SYS_LAST
};


/* ************************************************************************** */

#define GNOME_VFS_TYPE_ACL             (gnome_vfs_acl_get_type ())
#define GNOME_VFS_ACL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_VFS_TYPE_ACL, GnomeVFSACL))
#define GNOME_VFS_ACL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_VFS_TYPE_ACL, GnomeVFSACLClass))
#define GNOME_VFS_IS_ACL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_VFS_TYPE_ACL))
#define GNOME_VFS_IS_ACL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_VFS_TYPE_ACL))

typedef struct _GnomeVFSACL GnomeVFSACL;
typedef struct _GnomeVFSACLClass GnomeVFSACLClass;

typedef struct _GnomeVFSACLPrivate GnomeVFSACLPrivate;

struct _GnomeVFSACL {
	GObject parent;

	GnomeVFSACLPrivate *priv;
};

struct _GnomeVFSACLClass {
	GObjectClass parent_class;
};


GType         gnome_vfs_acl_get_type          (void) G_GNUC_CONST;
GnomeVFSACL * gnome_vfs_acl_new               (void);
void          gnome_vfs_acl_clear             (GnomeVFSACL *acl);
void          gnome_vfs_acl_set               (GnomeVFSACL *acl,
					       GnomeVFSACE *ace);

void          gnome_vfs_acl_unset             (GnomeVFSACL *acl,
					       GnomeVFSACE *ace);
                                              
GList *       gnome_vfs_acl_get_ace_list      (GnomeVFSACL *acl);
void          gnome_vfs_acl_free_ace_list     (GList       *ace_list);      

G_END_DECLS


#endif /*GNOME_VFS_ACL_H*/
