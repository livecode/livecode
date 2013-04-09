/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-async-ops.h - Asynchronous operations in the GNOME Virtual File
   System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef GNOME_VFS_ASYNC_OPS_H
#define GNOME_VFS_ASYNC_OPS_H

#include <glib/giochannel.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-find-directory.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-xfer.h>

G_BEGIN_DECLS

/**
 * GNOME_VFS_PRIORITY_MIN: 
 *
 * The minimuum priority a job can have.
 **/
/**
 * GNOME_VFS_PRIORITY_MAX: 
 *
 * The maximuum priority a job can have.
 **/
/**
 * GNOME_VFS_PRIORITY_DEFAULT:
 *
 * The default job priority. Its best to use this
 * unless you have a reason to do otherwise.
 **/

#define GNOME_VFS_PRIORITY_MIN     -10
#define GNOME_VFS_PRIORITY_MAX     10
#define GNOME_VFS_PRIORITY_DEFAULT 0

typedef struct GnomeVFSAsyncHandle GnomeVFSAsyncHandle;

/**
 * GnomeVFSAsyncCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Basic callback from an async operation that passes no data back,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer callback_data);

/**
 * GnomeVFSAsyncOpenCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_async_open() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncOpenCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer callback_data);

/**
 * GnomeVFSAsyncCreateCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_async_create() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncCreateCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer callback_data);

/**
 * GnomeVFSAsyncCloseCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_async_close() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncCloseCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer callback_data);

#ifndef GNOME_VFS_DISABLE_DEPRECATED
/**
 * GnomeVFSAsyncOpenAsChannelCallback:
 * @handle: handle of the operation generating the callback.
 * @channel: a #GIOChannel corresponding to the file opened
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_open_as_channel() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncOpenAsChannelCallback) (GnomeVFSAsyncHandle *handle,
							GIOChannel *channel,
							GnomeVFSResult result,
							gpointer callback_data);
#endif

#ifndef GNOME_VFS_DISABLE_DEPRECATED
/**
 * GnomeVFSAsyncCreateAsChannelCallback:
 * @handle: handle of the operation generating the callback.
 * @channel: a #GIOChannel corresponding to the file created
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_create_as_channel() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncCreateAsChannelCallback) (GnomeVFSAsyncHandle *handle,
							  GIOChannel *channel,
							  GnomeVFSResult result,
							  gpointer callback_data);
#endif

/**
 * GnomeVFSAsyncReadCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @buffer: buffer containing data read from @handle.
 * @bytes_requested: the number of bytes asked for in the call to
 * gnome_vfs_async_read().
 * @bytes_read: the number of bytes actually read from @handle into @buffer.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_async_read() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncReadCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer buffer,
						 GnomeVFSFileSize bytes_requested,
						 GnomeVFSFileSize bytes_read,
						 gpointer callback_data);

/**
 * GnomeVFSAsyncWriteCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @buffer: buffer containing data written to @handle.
 * @bytes_requested: the number of bytes asked to write in the call to
 * gnome_vfs_async_write().
 * @bytes_written: the number of bytes actually written to @handle from @buffer.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_write() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncWriteCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gconstpointer buffer,
						 GnomeVFSFileSize bytes_requested,
						 GnomeVFSFileSize bytes_written,
						 gpointer callback_data);


/**
 * GnomeVFSAsyncSeekCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_seek() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* GnomeVFSAsyncSeekCallback)	(GnomeVFSAsyncHandle *handle,
						 GnomeVFSResult result,
						 gpointer callback_data);


/**
 * GnomeVFSAsyncGetFileInfoCallback:
 * @handle: handle of the operation generating the callback
 * @results: #GList of #GnomeVFSFileInfoResult * items representing
 * the success of each gnome_vfs_get_file_info() and the data retrieved.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_get_file_info() function,
 * informing the user of the @results of the operation.
 **/
typedef void    (* GnomeVFSAsyncGetFileInfoCallback) (GnomeVFSAsyncHandle *handle,
						      GList *results,
						      gpointer callback_data);

/**
 * GnomeVFSAsyncSetFileInfoCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise a
 * #GnomeVFSResult error code.
 * @file_info: if @result is %GNOME_VFS_OK, a #GnomeVFSFileInfo struct containing
 * information about the file.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the gnome_vfs_async_set_file_info() function,
 * informing the user of the @result of the operation and
 * returning the new @file_info.
 *
 * <note>
 *  <para>
 *   Setting the file info sometimes changes more information than the
 *   caller specified; for example, if the name changes the MIME type might
 *   change, and if the owner changes the SUID & SGID bits might change. 
 *   Therefore the callback returns the new @file_info for the caller's
 *   convenience. The GnomeVFSFileInfoOptions passed here are those used 
 *   for the returned file info; they are not used when setting.
 *  </para>
 * </note>
 **/
typedef void	(* GnomeVFSAsyncSetFileInfoCallback) (GnomeVFSAsyncHandle *handle,
						      GnomeVFSResult result,
						      GnomeVFSFileInfo *file_info,
						      gpointer callback_data);


/**
 * GnomeVFSAsyncDirectoryLoadCallback:
 * @handle: handle of the operation generating the callback.
 * @result: %GNOME_VFS_OK if the operation was sucessful, 
 * %GNOME_VFS_ERROR_EOF if the last file in the directory 
 * has been read, otherwise a #GnomeVFSResult error code
 * @list: a #GList of #GnomeVFSFileInfo structs representing 
 * information about the files just loaded.
 * @entries_read: number of entries read from @handle for this instance of
 * the callback.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the gnome_vfs_async_directory_load() function.
 * informing the user of the @result of the operation and
 * providing a file @list if the @result is %GNOME_VFS_OK.
 **/
typedef void	(* GnomeVFSAsyncDirectoryLoadCallback) (GnomeVFSAsyncHandle *handle,
							GnomeVFSResult result,
							GList *list,
							guint entries_read,
							gpointer callback_data);

/**
 * GnomeVFSAsyncXferProgressCallback:
 * @handle: Handle of the Xfer operation generating the callback.
 * @info: Information on the current progress in the transfer.
 * @user_data: The user data that was passed to gnome_vfs_async_xfer().
 *
 * This callback is passed to gnome_vfs_async_xfer() and should
 * be used for user interaction. That said, it serves two purposes:
 * Informing the user about the progress of the operation, and
 * making decisions.
 *
 * On the one hand, when the transfer progresses normally,
 * i.e. when the @info's status is %GNOME_VFS_XFER_PROGRESS_STATUS_OK
 * it is called periodically whenever new progress information
 * is available, and it wasn't called already within the last
 * 100 milliseconds.
 *
 * On the other hand, it is called whenever a decision is
 * requested from the user, i.e. whenever the @info's %status
 * is not %GNOME_VFS_XFER_PROGRESS_STATUS_OK.
 *
 * Either way, it acts like #GnomeVFSXferProgressCallback
 * would act in non-asynchronous mode. The differences in
 * invocation are explained in the gnome_vfs_async_xfer()
 * documentation.
 *
 * Returns: %gint depending on the #GnomeVFSXferProgressInfo.
 * 	    Please consult #GnomeVFSXferProgressCallback for details.
 *
 **/
typedef gint    (* GnomeVFSAsyncXferProgressCallback) (GnomeVFSAsyncHandle *handle,
						       GnomeVFSXferProgressInfo *info,
						       gpointer user_data);

/**
 * GnomeVFSFindDirectoryResult:
 * @uri: The #GnomeVFSURI that was found.
 * @result: The #GnomeVFSResult that was obtained when finding @uri.
 *
 * This structure is passed to a #GnomeVFSAsyncFindDirectoryCallback
 * by gnome_vfs_async_find_directory() and contains the information
 * associated with a single #GnomeVFSURI matching the specified
 * find request.
 **/
struct _GnomeVFSFindDirectoryResult {
	/*< public >*/
	GnomeVFSURI *uri;
	GnomeVFSResult result;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
};

typedef struct _GnomeVFSFindDirectoryResult GnomeVFSFindDirectoryResult;

GType                        gnome_vfs_find_directory_result_get_type (void);
GnomeVFSFindDirectoryResult* gnome_vfs_find_directory_result_dup      (GnomeVFSFindDirectoryResult* result);
void                         gnome_vfs_find_directory_result_free     (GnomeVFSFindDirectoryResult* result);

/**
 * GnomeVFSAsyncFindDirectoryCallback:
 * @handle: handle of the operation generating the callback
 * @results: #GList of #GnomeVFSFindDirectoryResult *s containing
 * special directories matching the find criteria.
 * @data: user data defined when the operation was established
 *
 * Callback for the gnome_vfs_async_find_directory() function,
 * informing the user of the @results of the operation.
 **/
typedef void    (* GnomeVFSAsyncFindDirectoryCallback) (GnomeVFSAsyncHandle *handle,
							GList *results,
							gpointer data);

/**
 * GnomeVFSAsyncFileControlCallback:
 * @handle: handle of the operation generating the callback
 * @result: %GNOME_VFS_OK if the operation was successful, otherwise a
 * #GnomeVFSResult error code.
 * @operation_data: The data requested from the module if @result
 * is %GNOME_VFS_OK.
 * @callback_data: User data defined when the operation was established.
 *
 * Callback for the gnome_vfs_async_find_directory() function.
 * informing the user of the @result of the operation, and
 * providing the requested @operation_data.
 **/
typedef void	(* GnomeVFSAsyncFileControlCallback)	(GnomeVFSAsyncHandle *handle,
							 GnomeVFSResult result,
							 gpointer operation_data,
							 gpointer callback_data);

void           gnome_vfs_async_cancel                 (GnomeVFSAsyncHandle                   *handle);

void           gnome_vfs_async_open                   (GnomeVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       GnomeVFSOpenMode                       open_mode,
						       int				      priority,
						       GnomeVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_open_uri               (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSOpenMode                       open_mode,
						       int				      priority,
						       GnomeVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);

#ifndef GNOME_VFS_DISABLE_DEPRECATED
void           gnome_vfs_async_open_as_channel        (GnomeVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       GnomeVFSOpenMode                       open_mode,
						       guint                                  advised_block_size,
						       int				      priority,
						       GnomeVFSAsyncOpenAsChannelCallback     callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_open_uri_as_channel    (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSOpenMode                       open_mode,
						       guint                                  advised_block_size,
						       int				      priority,
						       GnomeVFSAsyncOpenAsChannelCallback     callback,
						       gpointer                               callback_data);
#endif /* GNOME_VFS_DISABLE_DEPRECATED */

void           gnome_vfs_async_create                 (GnomeVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       GnomeVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       GnomeVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_create_uri             (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       GnomeVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_create_symbolic_link   (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       const gchar                           *uri_reference,
						       int				      priority,
						       GnomeVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
#ifndef GNOME_VFS_DISABLE_DEPRECATED
void           gnome_vfs_async_create_as_channel      (GnomeVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       GnomeVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       GnomeVFSAsyncCreateAsChannelCallback   callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_create_uri_as_channel  (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       GnomeVFSAsyncCreateAsChannelCallback   callback,
						       gpointer                               callback_data);
#endif /* GNOME_VFS_DISABLE_DEPRECATED */

void           gnome_vfs_async_close                  (GnomeVFSAsyncHandle                   *handle,
						       GnomeVFSAsyncCloseCallback             callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_read                   (GnomeVFSAsyncHandle                   *handle,
						       gpointer                               buffer,
						       guint                                  bytes,
						       GnomeVFSAsyncReadCallback              callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_write                  (GnomeVFSAsyncHandle                   *handle,
						       gconstpointer                          buffer,
						       guint                                  bytes,
						       GnomeVFSAsyncWriteCallback             callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_seek                   (GnomeVFSAsyncHandle                   *handle,
						       GnomeVFSSeekPosition                   whence,
						       GnomeVFSFileOffset                     offset,
						       GnomeVFSAsyncSeekCallback              callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_get_file_info          (GnomeVFSAsyncHandle                  **handle_return,
						       GList                                 *uri_list,
						       GnomeVFSFileInfoOptions                options,
						       int				      priority,
						       GnomeVFSAsyncGetFileInfoCallback       callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_set_file_info          (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSFileInfo                      *info,
						       GnomeVFSSetFileInfoMask                mask,
						       GnomeVFSFileInfoOptions                options,
						       int				      priority,
						       GnomeVFSAsyncSetFileInfoCallback       callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_load_directory         (GnomeVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       GnomeVFSFileInfoOptions                options,
						       guint                                  items_per_notification,
						       int				      priority,
						       GnomeVFSAsyncDirectoryLoadCallback     callback,
						       gpointer                               callback_data);
void           gnome_vfs_async_load_directory_uri     (GnomeVFSAsyncHandle                  **handle_return,
						       GnomeVFSURI                           *uri,
						       GnomeVFSFileInfoOptions                options,
						       guint                                  items_per_notification,
						       int				      priority,
						       GnomeVFSAsyncDirectoryLoadCallback     callback,
						       gpointer                               callback_data);
GnomeVFSResult gnome_vfs_async_xfer                   (GnomeVFSAsyncHandle                  **handle_return,
						       GList                                 *source_uri_list,
						       GList                                 *target_uri_list,
						       GnomeVFSXferOptions                    xfer_options,
						       GnomeVFSXferErrorMode                  error_mode,
						       GnomeVFSXferOverwriteMode              overwrite_mode,
						       int				      priority,
						       GnomeVFSAsyncXferProgressCallback      progress_update_callback,
						       gpointer                               update_callback_data,
						       GnomeVFSXferProgressCallback           progress_sync_callback,
						       gpointer                               sync_callback_data);
void           gnome_vfs_async_find_directory         (GnomeVFSAsyncHandle                  **handle_return,
						       GList                                 *near_uri_list,
						       GnomeVFSFindDirectoryKind              kind,
						       gboolean                               create_if_needed,
						       gboolean                               find_if_needed,
						       guint                                  permissions,
						       int				      priority,
						       GnomeVFSAsyncFindDirectoryCallback     callback,
						       gpointer                               user_data);

void           gnome_vfs_async_file_control           (GnomeVFSAsyncHandle                   *handle,
						       const char                            *operation,
						       gpointer                               operation_data,
						       GDestroyNotify                         operation_data_destroy_func,
						       GnomeVFSAsyncFileControlCallback       callback,
						       gpointer                               callback_data);

G_END_DECLS

#endif /* GNOME_VFS_ASYNC_OPS_H */
