/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-dns-sd.h - DNS-SD functions

   Copyright (C) 2004 Red Hat, Inc

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

#ifndef GNOME_VFS_DNS_SD_H
#define GNOME_VFS_DNS_SD_H

#include <sys/types.h>
#include <glib.h>
#include <glib-object.h>
#include <libgnomevfs/gnome-vfs-result.h>

G_BEGIN_DECLS

/* TODO:
 * a way to get default browse domain for domain
 * async listing of browse domains
 * some way to publish. Using dns update for unicast?
 */
   

typedef struct {
	char *name;
	char *type;
	char *domain;
} GnomeVFSDNSSDService;

typedef enum {
	GNOME_VFS_DNS_SD_SERVICE_ADDED,
	GNOME_VFS_DNS_SD_SERVICE_REMOVED
} GnomeVFSDNSSDServiceStatus;

GType gnome_vfs_dns_sd_service_status_get_type (void);
#define GNOME_VFS_TYPE_VFS_DNS_SD_SERVICE_STATUS (gnome_vfs_dns_sd_service_status_get_type())

typedef struct GnomeVFSDNSSDBrowseHandle GnomeVFSDNSSDBrowseHandle;
typedef struct GnomeVFSDNSSDResolveHandle GnomeVFSDNSSDResolveHandle;

/**
 * GnomeVFSDNSSDBrowseCallback:
 * @handle: handle of the operation generating the callback
 * @status: whether a service addition or removal was detected
 * @service: the service that was discovered or removed
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_dns_sd_browse() function that informs
 * the user of services that are added or removed.
 **/
typedef void (* GnomeVFSDNSSDBrowseCallback) (GnomeVFSDNSSDBrowseHandle *handle,
					      GnomeVFSDNSSDServiceStatus status,
					      const GnomeVFSDNSSDService *service,
					      gpointer callback_data);

/**
 * GnomeVFSDNSSDResolveCallback:
 * @handle: handle of the operation generating the callback
 * @result: whether the resolve succeeded or not
 * @service: the service that was resolved
 * @host: the host name or ip of the host hosting the service
 * @port: the port number to use for the service
 * @text: a hash table giving additional options about the service
 * @text_raw_len: length of @text_raw
 * @text_raw: raw version of the additional options in @text
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_dns_sd_resolve() function that is called
 * when a service has been resolved.
 *
 * The @host and @port can be used to contact the requested service, and
 * @text contains additional options as defined for the type requested.
 * 
 * To check for options being set in @text without any value ("key" as
 * opposed to "key=value") you must use g_hash_table_lookup_extended(), since
 * they are stored in the hash table with a NULL value, and g_hash_table_lookup()
 * can't tell that from the case where the key is not in the hash table.
 **/
typedef void (* GnomeVFSDNSSDResolveCallback) (GnomeVFSDNSSDResolveHandle *handle,
					       GnomeVFSResult result,
					       const GnomeVFSDNSSDService *service,
					       const char *host,
					       int port,
					       const GHashTable *text,
					       int text_raw_len,
					       const char *text_raw,
					       gpointer callback_data);

/* Async versions */


GnomeVFSResult
gnome_vfs_dns_sd_browse (GnomeVFSDNSSDBrowseHandle **handle,
			 const char *domain,
			 const char *type,
			 GnomeVFSDNSSDBrowseCallback callback,
			 gpointer callback_data,
			 GDestroyNotify callback_data_destroy_func);

GnomeVFSResult
gnome_vfs_dns_sd_stop_browse (GnomeVFSDNSSDBrowseHandle *handle);


GnomeVFSResult			      
gnome_vfs_dns_sd_resolve (GnomeVFSDNSSDResolveHandle **handle,
			  const char *name,
			  const char *type,
			  const char *domain,
			  int timeout,
			  GnomeVFSDNSSDResolveCallback callback,
			  gpointer callback_data,
			  GDestroyNotify callback_data_destroy_func);

GnomeVFSResult
gnome_vfs_dns_sd_cancel_resolve (GnomeVFSDNSSDResolveHandle *handle);


/* Sync versions */

GnomeVFSResult
gnome_vfs_dns_sd_browse_sync (const char *domain,
			      const char *type,
			      int timeout_msec,
			      int *n_services,
			      GnomeVFSDNSSDService **services);


GnomeVFSResult			      
gnome_vfs_dns_sd_resolve_sync (const char *name,
			       const char *type,
			       const char *domain,
			       int timeout_msec,
			       char **host, int *port,
			       GHashTable **text,
			       int *text_raw_len_out,
			       char **text_raw_out);

void
gnome_vfs_dns_sd_service_list_free (GnomeVFSDNSSDService *services,
				    int n_services);

GnomeVFSResult
gnome_vfs_dns_sd_list_browse_domains_sync (const char *domain,
					   int timeout_msec,
					   GList **domains);


GList *
gnome_vfs_get_default_browse_domains (void);


G_END_DECLS

#endif /* GNOME_VFS_DNS_SD_H */
