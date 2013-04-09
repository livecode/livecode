/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-uri.h - URI handling for the GNOME Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef GNOME_VFS_URI_H
#define GNOME_VFS_URI_H

#include <glib/glist.h>
#include <glib-object.h> /* For GType */

G_BEGIN_DECLS

/**
 * GnomeVFSURI:
 * @ref_count: Reference count. The URI is freed when it drops to zero.
 * @text: A canonical representation of the path associated with this resource.
 * @fragment_id: Extra data identifying this resource.
 * @method_string: The @method's method associated with this resource.
 * One #GnomeVFSMethod can be used for multiple method strings.
 * @method: The #GnomeVFSMethod associated with this resource.
 * @parent: Pointer to the parent element, or %NULL for #GnomeVFSURI that
 * have no enclosing #GnomeVFSURI. The process of encapsulating one
 * URI in another one is called URI chaining.
 *
 * Holds information about the location of a particular resource.
 **/
typedef struct GnomeVFSURI {
	/*< public >*/
	guint ref_count;

	gchar *text;
	gchar *fragment_id;

	gchar *method_string;
	struct GnomeVFSMethod *method;

	struct GnomeVFSURI *parent;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSURI;

/**
 * GnomeVFSToplevelURI:
 * @host_name: The name of the host used to access this resource, o %NULL.
 * @host_port: The port used to access this resource, or %0.
 * @user_name: Unescaped user name used to access this resource, or %NULL.
 * @password: Unescaped password used to access this resource, or %NULL.
 * @urn: The parent URN, or %NULL if it doesn't exist.
 *
 * This is the toplevel URI element used to access ressources stored on
 * a remote server. Toplevel method implementations should cast the #GnomeVFSURI
 * argument to this type to get the additional host and authentication information.
 *
 * If any of the elements is 0 respectively %NULL, it is unspecified.
 **/
typedef struct {
	GnomeVFSURI uri;

	/*< public >*/
	gchar *host_name;
	guint host_port;

	gchar *user_name;
	gchar *password;

	gchar *urn;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} GnomeVFSToplevelURI;


/**
 * GnomeVFSURIHideOptions:
 * @GNOME_VFS_URI_HIDE_NONE: don't hide anything
 * @GNOME_VFS_URI_HIDE_USER_NAME: hide the user name
 * @GNOME_VFS_URI_HIDE_PASSWORD: hide the password
 * @GNOME_VFS_URI_HIDE_HOST_NAME: hide the host name
 * @GNOME_VFS_URI_HIDE_HOST_PORT: hide the port
 * @GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD: hide the method (e.g. http, file)
 * @GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER: hide the fragment identifier
 *
 * Packed boolean bitfield controlling hiding of various elements
 * of a #GnomeVFSURI when it is converted to a string using
 * gnome_vfs_uri_to_string().
 **/
typedef enum
{
	GNOME_VFS_URI_HIDE_NONE = 0,
	GNOME_VFS_URI_HIDE_USER_NAME = 1 << 0,
	GNOME_VFS_URI_HIDE_PASSWORD = 1 << 1,
	GNOME_VFS_URI_HIDE_HOST_NAME = 1 << 2,
	GNOME_VFS_URI_HIDE_HOST_PORT = 1 << 3,
	GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD = 1 << 4,
	GNOME_VFS_URI_HIDE_FRAGMENT_IDENTIFIER = 1 << 8
} GnomeVFSURIHideOptions; 

GType gnome_vfs_uri_hide_options_get_type (void);
#define GNOME_VFS_TYPE_VFS_URI_HIDE_OPTIONS (gnome_vfs_uri_hide_options_get_type())


/**
 * GNOME_VFS_URI_MAGIC_CHR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
/**
 * GNOME_VFS_URI_MAGIC_STR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
#define GNOME_VFS_URI_MAGIC_CHR	'#'
#define GNOME_VFS_URI_MAGIC_STR "#"

/**
 * GNOME_VFS_URI_PATH_CHR:
 *
 * Defines the path seperator character.
 **/
/**
 * GNOME_VFS_URI_PATH_STR:
 *
 * Defines the path seperator string.
 **/
#define GNOME_VFS_URI_PATH_CHR '/'
#define GNOME_VFS_URI_PATH_STR "/"

/* FUNCTIONS */
GnomeVFSURI 	     *gnome_vfs_uri_new                   (const gchar *text_uri);
GnomeVFSURI 	     *gnome_vfs_uri_resolve_relative      (const GnomeVFSURI *base,
							   const gchar *relative_reference);
GnomeVFSURI 	     *gnome_vfs_uri_resolve_symbolic_link (const GnomeVFSURI *base,
							   const gchar *relative_reference);
GnomeVFSURI 	     *gnome_vfs_uri_ref                   (GnomeVFSURI *uri);
void        	      gnome_vfs_uri_unref                 (GnomeVFSURI *uri);

GnomeVFSURI          *gnome_vfs_uri_append_string         (const GnomeVFSURI *uri,
						           const char *uri_fragment);
GnomeVFSURI          *gnome_vfs_uri_append_path           (const GnomeVFSURI *uri,
						           const char *path);
GnomeVFSURI          *gnome_vfs_uri_append_file_name      (const GnomeVFSURI *uri,
						           const gchar *filename);
gchar       	     *gnome_vfs_uri_to_string             (const GnomeVFSURI *uri,
						           GnomeVFSURIHideOptions hide_options);
GnomeVFSURI 	     *gnome_vfs_uri_dup                   (const GnomeVFSURI *uri);
gboolean    	      gnome_vfs_uri_is_local              (const GnomeVFSURI *uri);
gboolean	      gnome_vfs_uri_has_parent	          (const GnomeVFSURI *uri);
GnomeVFSURI	     *gnome_vfs_uri_get_parent            (const GnomeVFSURI *uri);

GnomeVFSToplevelURI *gnome_vfs_uri_get_toplevel           (const GnomeVFSURI *uri);

const gchar 	    *gnome_vfs_uri_get_host_name          (const GnomeVFSURI *uri);
const gchar         *gnome_vfs_uri_get_scheme             (const GnomeVFSURI *uri);
guint 	    	     gnome_vfs_uri_get_host_port          (const GnomeVFSURI *uri);
const gchar 	    *gnome_vfs_uri_get_user_name          (const GnomeVFSURI *uri);
const gchar	    *gnome_vfs_uri_get_password           (const GnomeVFSURI *uri);

void		     gnome_vfs_uri_set_host_name          (GnomeVFSURI *uri,
						           const gchar *host_name);
void 	    	     gnome_vfs_uri_set_host_port          (GnomeVFSURI *uri,
						           guint host_port);
void		     gnome_vfs_uri_set_user_name          (GnomeVFSURI *uri,
						           const gchar *user_name);
void		     gnome_vfs_uri_set_password           (GnomeVFSURI *uri,
						           const gchar *password);

gboolean	     gnome_vfs_uri_equal	          (const GnomeVFSURI *a,
						           const GnomeVFSURI *b);

gboolean	     gnome_vfs_uri_is_parent	          (const GnomeVFSURI *possible_parent,
						           const GnomeVFSURI *possible_child,
						           gboolean recursive);
				  
const gchar 	    *gnome_vfs_uri_get_path                (const GnomeVFSURI *uri);
const gchar 	    *gnome_vfs_uri_get_fragment_identifier (const GnomeVFSURI *uri);
gchar 		    *gnome_vfs_uri_extract_dirname         (const GnomeVFSURI *uri);
gchar		    *gnome_vfs_uri_extract_short_name      (const GnomeVFSURI *uri);
gchar		    *gnome_vfs_uri_extract_short_path_name (const GnomeVFSURI *uri);

gint		     gnome_vfs_uri_hequal 	           (gconstpointer a,
						            gconstpointer b);
guint		     gnome_vfs_uri_hash		           (gconstpointer p);

GList               *gnome_vfs_uri_list_parse              (const gchar* uri_list);
GList               *gnome_vfs_uri_list_ref                (GList *list);
GList               *gnome_vfs_uri_list_unref              (GList *list);
GList               *gnome_vfs_uri_list_copy               (GList *list);
void                 gnome_vfs_uri_list_free               (GList *list);

char                *gnome_vfs_uri_make_full_from_relative (const char *base_uri,
							    const char *relative_uri);


G_END_DECLS

#endif /* GNOME_VFS_URI_H */
