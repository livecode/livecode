/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-xfer.h - File transfers in the GNOME Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef GNOME_VFS_XFER_H
#define GNOME_VFS_XFER_H

#include <libgnomevfs/gnome-vfs-file-info.h>

G_BEGIN_DECLS

/*
 * FIXME bugzilla.eazel.com 1205:
 * Split these up into xfer options and xfer actions
 */


/**
 * GnomeVFSXferOptions:
 * @GNOME_VFS_XFER_DEFAULT: Default behavior, which is to do a straight one to one copy.
 * @GNOME_VFS_XFER_FOLLOW_LINKS: Follow symbolic links when copying or moving, i.e.
 * 				 the target of symbolic links are copied
 * 				 rather than the symbolic links themselves.
 * 				 Note that this just refers to top-level items.
 * 				 If you also want to follow symbolic links inside
 * 				 directories you operate on, you also have to specify
 * 				 #GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE.
 * @GNOME_VFS_XFER_RECURSIVE: Recursively copy source directories to the target. 
 * 			      Equivalent to the cp -r option in GNU cp.
 * @GNOME_VFS_XFER_SAMEFS: When copying recursively, this only picks up items on the same file
 * 			   system the same filesystem as their parent directory.
 * @GNOME_VFS_XFER_DELETE_ITEMS: This is equivalent to an rmdir() for source directories,
 * 				 and an unlink() operation for all source files.
 * 				 Requires %NULL target URIs.
 * @GNOME_VFS_XFER_EMPTY_DIRECTORIES: Remove the whole contents of the passed-in source
 * 				      directories. Requires %NULL target URIs.
 * @GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY: This will create a directory if it doesn't exist
 * 					 in the destination area (i.e. mkdir ()).
 * @GNOME_VFS_XFER_REMOVESOURCE: This makes a copy operation equivalent to a mv, i.e. the
 * 				 files will be moved rather than copied. If applicable, this
 * 				 will use rename(), otherwise (i.e. across file systems),
 * 				 it will fall back to a copy operation followed by a source
 * 				 file deletion.
 * @GNOME_VFS_XFER_USE_UNIQUE_NAMES: When this option is present, and a name collisions on
 * 				     the target occurs, the progress callback will be asked
 * 				     for a new name, until the newly provided name doesn't
 * 				     conflict or the request callback transfer cancellation.
 * @GNOME_VFS_XFER_LINK_ITEMS: Executes a symlink operation for each of the source/target URI pairs,
 * 			       i.e. similar to GNU ln -s source target.
 * 			       NB: The symlink target has to be specified as source URI,
 * 			       	   and the symlink itself as target URI.
 * @GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE: This means that when doing a copy operation, symbolic
 * 					   links in subdirectories are dereferenced. This is
 * 					   typically used together with #GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE.
 * @GNOME_VFS_XFER_TARGET_DEFAULT_PERMS: This means that the target file will not have the same
 * 					 permissions as the source file, but will instead have
 * 					 the default permissions of the destination location.
 * 					 This is useful when copying from read-only locations (CDs).
 * @GNOME_VFS_XFER_UNUSED_1: Unused.
 * @GNOME_VFS_XFER_UNUSED_2: Unused.
 *
 * These options control the way gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * gnome_vfs_xfer_delete_list() and gnome_vfs_async_xfer() work.
 *
 * At a first glance the #GnomeVFSXferOptions semantics are not very intuitive.
 *
 * There are two types of #GnomeVFSXferOptions: Those that define an operation,
 * i.e. describe what to do, and those that influence how to execute the
 * operation.
 *
 * <table frame="none">
 *  <title>Operation Selection</title>
 *  <tgroup cols="3" align="left">
 *   <?dbhtml cellpadding="10" ?>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <thead>
 *    <row>
 *     <entry>#GnomeVFSXferOptions entry</entry>
 *     <entry>Operation</entry>
 *     <entry>UNIX equivalent</entry>
 *     <entry>Comments</entry>
 *    </row>
 *   </thead>
 *   <tbody>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_DEFAULT</entry>
 *     <entry>Copy</entry>
 *     <entry><literal>cp</literal></entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_REMOVESOURCE</entry>
 *     <entry>Move</entry>
 *     <entry><literal>mv</literal></entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_LINK_ITEMS</entry>
 *     <entry>Link</entry>
 *     <entry><literal>ln -s</literal></entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY</entry>
 *     <entry>Make Unique Directory</entry>
 *     <entry><literal>mkdir</literal></entry>
 *     <entry>implies #GNOME_VFS_XFER_USE_UNIQUE_NAMES</entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_DELETE_ITEMS</entry>
 *     <entry>Remove</entry>
 *     <entry><literal>rm -r</literal></entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_EMPTY_DIRECTORIES</entry>
 *     <entry>Remove Directory Contents</entry>
 *     <entry>foreach file: <literal>( cd file && rm -rf * )</literal></entry>
 *     <entry>used to empty trash</entry>
 *    </row>
 *   </tbody>
 *  </tgroup>
 * </table>
 *
 * <note>
 *  <para>
 *    Because #GNOME_VFS_XFER_DEFAULT maps to %0, it will always be present.
 *    Thus, not explicitly specifying any operation at all, or just specifying
 *    #GNOME_VFS_XFER_DEFAULT will both execute a copy.
 *  </para>
 *  <para>
 *    If an operation other than #GNOME_VFS_XFER_DEFAULT is
 *    specified, it will override the copy operation, but you may only specify
 *    <emphasis>one</emphasis> of the other operations at a time.
 *  </para>
 *  <para>
 *    This unintuitive operation selection unfortunately poses an API weakness
 *    and an obstacle in client development, and will be modified in a later
 *    revision of this API.
 *  </para>
 * </note>
 */
typedef enum {
	GNOME_VFS_XFER_DEFAULT = 0,
	GNOME_VFS_XFER_UNUSED_1 = 1 << 0,
	GNOME_VFS_XFER_FOLLOW_LINKS = 1 << 1,
	GNOME_VFS_XFER_UNUSED_2 = 1 << 2,
	GNOME_VFS_XFER_RECURSIVE = 1 << 3,
	GNOME_VFS_XFER_SAMEFS = 1 << 4,
	GNOME_VFS_XFER_DELETE_ITEMS = 1 << 5,
	GNOME_VFS_XFER_EMPTY_DIRECTORIES = 1 << 6,
	GNOME_VFS_XFER_NEW_UNIQUE_DIRECTORY = 1 << 7,
	GNOME_VFS_XFER_REMOVESOURCE = 1 << 8,
	GNOME_VFS_XFER_USE_UNIQUE_NAMES = 1 << 9,
	GNOME_VFS_XFER_LINK_ITEMS = 1 << 10,
	GNOME_VFS_XFER_FOLLOW_LINKS_RECURSIVE = 1 << 11,
	GNOME_VFS_XFER_TARGET_DEFAULT_PERMS = 1 << 12
} GnomeVFSXferOptions;

/**
 * GnomeVFSXferProgressStatus:
 * @GNOME_VFS_XFER_PROGRESS_STATUS_OK: The file transfer is progressing normally.
 * @GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR: A VFS error was detected.
 * @GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE: The current target file specified by the
 * 					      #GnomeVFSXferProgressInfo's %target_name
 * 					      field already exists.
 * @GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE: The current target file specified by the
 * 					      #GnomeVFSXferProgressInfo's %target_name
 * 					      field already exists, and the progress
 * 					      callback is asked to supply a new unique name.
 *
 * The difference between #GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE and
 * #GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE is that they will be issued
 * at different occassions, and that the return value will be interpreted
 * differently. For details, see #GnomeVFSXferProgressCallback.
 **/
typedef enum {
	GNOME_VFS_XFER_PROGRESS_STATUS_OK = 0,
	GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR = 1,
	GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE = 2,
	GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE = 3
} GnomeVFSXferProgressStatus;

/**
 * GnomeVFSXferOverwriteMode:
 * @GNOME_VFS_XFER_OVERWRITE_MODE_ABORT: Abort the transfer when a target file already exists,
 * 					 returning the error #GNOME_VFS_ERROR_FILEEXISTS.
 * @GNOME_VFS_XFER_OVERWRITE_MODE_QUERY: Query the progress callback with the
 * 					 #GnomeVFSXferProgressInfo's status field
 * 					 set to #GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR when
 * 					 a target file already exists.
 * @GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE: Replace existing target files silently.
 * 					   Don't worry be happy.
 * @GNOME_VFS_XFER_OVERWRITE_MODE_SKIP: Skip source files when its target already exists.
 *
 * This is passed to gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * gnome_vfs_xfer_delete_list() and gnome_vfs_async_xfer() and specifies
 * what action should be taken when a target file already exists.
 **/
typedef enum {
	GNOME_VFS_XFER_OVERWRITE_MODE_ABORT = 0,
	GNOME_VFS_XFER_OVERWRITE_MODE_QUERY = 1,
	GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE = 2,
	GNOME_VFS_XFER_OVERWRITE_MODE_SKIP = 3
} GnomeVFSXferOverwriteMode;

/**
 * GnomeVFSXferOverwriteAction:
 * @GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT: abort the transfer
 * @GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE: replace the existing file
 * @GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL: replace the existing file, and all future files
 * without prompting the callback.
 * @GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP: don't copy over the existing file
 * @GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP_ALL: don't copy over the existing file, and all future
 * files without prompting the callback.
 * 
 * This defines the actions to perform before a file is being overwritten
 * (i.e., these are the answers that can be given to a replace query).  
 **/
typedef enum {
	GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT = 0,
	GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE = 1,
	GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL = 2,
	GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP = 3,
	GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP_ALL = 4
} GnomeVFSXferOverwriteAction;

/**
 * GnomeVFSXferErrorMode:
 * @GNOME_VFS_XFER_ERROR_MODE_ABORT: abort the transfer when an error occurs
 * @GNOME_VFS_XFER_ERROR_MODE_QUERY: query the progress callback with the
 * 				     #GnomeVFSXferProgressInfo's status field
 * 				     set to #GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR.
 *
 * This is passed to gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * gnome_vfs_xfer_delete_list() and gnome_vfs_async_xfer() and specifies
 * what action should be taken when transfer errors are detected.
 *
 * The progress callback is either a #GnomeVFSXferProgressCallback for synchronous
 * Xfer operations, or a #GnomeVFSAsyncXferProgressCallback for asynchronous operations.
 **/
typedef enum {
	GNOME_VFS_XFER_ERROR_MODE_ABORT = 0,
	GNOME_VFS_XFER_ERROR_MODE_QUERY = 1
} GnomeVFSXferErrorMode;

/**
 * GnomeVFSXferErrorAction:
 * @GNOME_VFS_XFER_ERROR_ACTION_ABORT: interrupt Xfer and return %GNOME_VFS_ERROR_INTERRUPTED.
 * @GNOME_VFS_XFER_ERROR_ACTION_RETRY: retry the failed operation.
 * @GNOME_VFS_XFER_ERROR_ACTION_SKIP: skip the failed operation, and continue Xfer normally.
 *
 * This defines the possible actions to be performed after a VFS error has occurred, i.e.
 * when a GnomeVFS file operation issued during the transfer returned a result that is not
 * equal to #GNOME_VFS_OK.
 *
 * It is returned by the progress callback which is either a #GnomeVFSXferProgressCallback
 * for synchronous Xfer operations, or a #GnomeVFSAsyncXferProgressCallback for asynchronous
 * operations.
 **/
typedef enum {
	GNOME_VFS_XFER_ERROR_ACTION_ABORT = 0,
	GNOME_VFS_XFER_ERROR_ACTION_RETRY = 1,
	GNOME_VFS_XFER_ERROR_ACTION_SKIP = 2
} GnomeVFSXferErrorAction;

/**
 * GnomeVFSXferPhase:
 * @GNOME_VFS_XFER_PHASE_INITIAL: initial phase.
 * @GNOME_VFS_XFER_CHECKING_DESTINATION: destination is checked for being able to handle copy/move.
 * @GNOME_VFS_XFER_PHASE_COLLECTING: source file list is collected.
 * @GNOME_VFS_XFER_PHASE_READYTOGO: source file list has been collected (*).
 * @GNOME_VFS_XFER_PHASE_OPENSOURCE: source file is opened for reading.
 * @GNOME_VFS_XFER_PHASE_OPENTARGET: target file, directory or symlink is created, or opened for copying.
 * @GNOME_VFS_XFER_PHASE_COPYING: data is copied from source file to target file (*).
 * @GNOME_VFS_XFER_PHASE_MOVING: source file is moved to target (M).
 * @GNOME_VFS_XFER_PHASE_READSOURCE: data is read from a source file, when copying.
 * @GNOME_VFS_XFER_PHASE_WRITETARGET: data is written to a target file, when copying.
 * @GNOME_VFS_XFER_PHASE_CLOSESOURCE: source file is closed, when copying
 * @GNOME_VFS_XFER_PHASE_CLOSETARGET: target file is closed, when copying.
 * @GNOME_VFS_XFER_PHASE_DELETESOURCE: source file is deleted.
 * @GNOME_VFS_XFER_PHASE_SETATTRIBUTES: target file attributes are set.
 * @GNOME_VFS_XFER_PHASE_FILECOMPLETED: one file was completed, ready for next file.
 * @GNOME_VFS_XFER_PHASE_CLEANUP: cleanup after moving (i.e. source files deletion).
 * @GNOME_VFS_XFER_PHASE_COMPLETED: operation finished (*).
 *
 * Specifies the current phase of an Xfer operation that was
 * initiated using gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * gnome_vfs_xfer_delete_list() or gnome_vfs_async_xfer().
 *
 * Whenever the Xfer phase is in a phase that is highlighted with a
 * (*), the #GnomeVFSXferProgressCallback respectively
 * #GnomeVFSAsyncXferProgressCallback is never invoked with a
 * #GnomeVFSXferProgressStatus other than %GNOME_VFS_XFER_PROGRESS_STATUS_OK.
 *
 **/
typedef enum {
	GNOME_VFS_XFER_PHASE_INITIAL,
	GNOME_VFS_XFER_CHECKING_DESTINATION,
	GNOME_VFS_XFER_PHASE_COLLECTING,
	GNOME_VFS_XFER_PHASE_READYTOGO,
	GNOME_VFS_XFER_PHASE_OPENSOURCE,
	GNOME_VFS_XFER_PHASE_OPENTARGET,
	GNOME_VFS_XFER_PHASE_COPYING,
	GNOME_VFS_XFER_PHASE_MOVING,
	GNOME_VFS_XFER_PHASE_READSOURCE,
	GNOME_VFS_XFER_PHASE_WRITETARGET,
	GNOME_VFS_XFER_PHASE_CLOSESOURCE,
	GNOME_VFS_XFER_PHASE_CLOSETARGET,
	GNOME_VFS_XFER_PHASE_DELETESOURCE,
	GNOME_VFS_XFER_PHASE_SETATTRIBUTES,
	GNOME_VFS_XFER_PHASE_FILECOMPLETED,
	GNOME_VFS_XFER_PHASE_CLEANUP,
	GNOME_VFS_XFER_PHASE_COMPLETED,
	GNOME_VFS_XFER_NUM_PHASES
} GnomeVFSXferPhase;

/**
 * GnomeVFSXferProgressInfo:
 * @status: A #GnomeVFSXferProgressStatus describing the current status.
 * @vfs_status: A #GnomeVFSResult describing the current VFS status.
 * @phase: A #GnomeVFSXferPhase describing the current transfer phase.
 * @source_name: The Currently processed source URI.
 * @target_name: The Currently processed target URI.
 * @file_index: The index of the currently processed file.
 * @files_total: The total number of processed files.
 * @file_size: The size of the currently processed file in bytes.
 * @bytes_total: The total size of all files to transfer in bytes.
 * @bytes_copied: The number of bytes that has been transferred
 * 		  from the current file.
 * @total_bytes_copied: The total number of bytes that has been transferred.
 * @duplicate_name: The name specifying a duplicate filename.
 * 		    It acts as pointer to both input and output
 * 		    data. It is only valid input data if @status is
 * 		    GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE,
 * 		    in which case @duplicate_count and @duplicate_name
 * 		    should be used by the #GnomeVFSXferProgressCallback
 * 		    to pick a new unique target name.
 * 		    If the callback wants to retry with a new unique name
 * 		    it is supposed to free the old @duplicate_name
 * 		    set it to a valid string describing the new file name.
 * @duplicate_count: The number of conflicts that ocurred when the
 * 		     current @duplicate_name was set.
 * @top_level_item: This flag signals that the currently
 * 		    processed file is a top level item.
 * 		    If it is %TRUE, one of the files passed to
 * 		    gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * 		    gnome_vfs_xfer_delete_list() or gnome_vfs_async_xfer()
 * 		    is currently processed.
 * 		    If it is %FALSE, a file or directory that is inside
 * 		    a directory specified by the passed in source list
 * 		    is currently processed.
 *
 * Provides progress information for the transfer operation.
 * This is especially useful for interactive programs.
 **/
typedef struct {
	/*< public > */
	GnomeVFSXferProgressStatus status;

	GnomeVFSResult vfs_status;

	GnomeVFSXferPhase phase;

	/* Source URI. FIXME bugzilla.eazel.com 1206: change name? */
	gchar *source_name;

	/* Destination URI. FIXME bugzilla.eazel.com 1206: change name? */
	gchar *target_name;

	gulong file_index;

	gulong files_total;

	GnomeVFSFileSize bytes_total;

	GnomeVFSFileSize file_size;

	GnomeVFSFileSize bytes_copied;

	GnomeVFSFileSize total_bytes_copied;

	gchar *duplicate_name;

	int duplicate_count;

	gboolean top_level_item;

	/* Reserved for future expansions to GnomeVFSXferProgressInfo
	 * without having to break ABI compatibility */
	/*< private >*/
	void *reserved1;
	void *reserved2;
} GnomeVFSXferProgressInfo;

/**
 * GnomeVFSXferProgressCallback:
 * @info: The #GnomeVFSXferProgressInfo associated with this transfer operation.
 * @user_data: The user data passed to gnome_vfs_xfer_uri(), gnome_vfs_xfer_uri_list(),
 * 	       gnome_vfs_xfer_delete_list() or gnome_vfs_async_xfer().
 *
 * This is the prototype for functions called during a transfer operation to
 * report progress.
 *
 * The interpretation of the return value of the callback depends on the
 * GnomeVFSXferProgressStaus %status field of GnomeVFSXferProgressInfo,
 * some status/return value combinations require modification of
 * particular @info fields.
 *
 * <table frame="none">
 *  <title>Status/Return Value Overview</title>
 *  <tgroup cols="3" align="left">
 *   <?dbhtml cellpadding="10" ?>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <thead>
 *    <row>
 *     <entry>#GnomeVFSXferProgressStatus status</entry>
 *     <entry>Status</entry>
 *     <entry>Only If</entry>
 *     <entry>Return Value Interpretation</entry>
 *    </row>
 *   </thead>
 *   <tbody>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_PROGRESS_STATUS_OK</entry>
 *     <entry>OK</entry>
 *     <entry></entry>
 *     <entry>%0: abort, otherwise continue</entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR</entry>
 *     <entry>VFS Error Ocurred</entry>
 *     <entry>#GnomeVFSXferErrorMode is #GNOME_VFS_XFER_ERROR_MODE_QUERY</entry>
 *     <entry>GnomeVFSXferErrorAction</entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE</entry>
 *     <entry>Conflict Ocurred, Overwrite?</entry>
 *     <entry>
 *            #GnomeVFSXferOverwriteMode is #GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
 *            #GnomeVFSXferOptions does not have #GNOME_VFS_XFER_USE_UNIQUE_NAMES.
 *     </entry>
 *     <entry>GnomeVFSXferOverwriteAction</entry>
 *    </row>
 *    <row>
 *     <entry>#GNOME_VFS_XFER_PROGRESS_STATUS_DUPLICATE</entry>
 *     <entry>Conflict Ocurred, New Target Name?</entry>
 *     <entry>#GnomeVFSXferOptions does have #GNOME_VFS_XFER_USE_UNIQUE_NAMES.</entry>
 *     <entry>%0: abort, otherwise retry with new %duplicate_name in @info (free the old one!).</entry>
 *    </row>
 *   </tbody>
 *  </tgroup>
 * </table>
 *
 * <note>
 * Each #GnomeVFSXferProgressStatus provides one value signalling abortion that maps to %0.
 * Therefore, returning %0 will always abort the Xfer. On abortion, if the @info's %vfs_status
 * is #GNOME_VFS_OK, the Xfer operation result will be set to #GNOME_VFS_ERROR_INTERRUPTED,
 * otherwise the operation result will be set to %vfs_status to distinguish completely
 * user-driven aborts from those involving a problem during the Xfer.
 * </note>
 *
 * Returns: Whether the process should be aborted, or whether a special action should be taken.
 **/
typedef gint (* GnomeVFSXferProgressCallback) 	(GnomeVFSXferProgressInfo *info,
						 gpointer user_data);

/**
 * GnomeVFSProgressCallbackState:
 *
 * Private structure encapsulating the entire state information of the xfer process.
 **/
typedef struct _GnomeVFSProgressCallbackState {
	/*< private >*/

	/* xfer state */
	GnomeVFSXferProgressInfo *progress_info;	

	/* Callback called for every xfer operation. For async calls called 
	   in async xfer context. */
	GnomeVFSXferProgressCallback sync_callback;

	/* Callback called periodically every few hundred miliseconds
	   and whenever user interaction is needed. For async calls
	   called in the context of the async call caller. */
	GnomeVFSXferProgressCallback update_callback;

	/* User data passed to sync_callback. */
	gpointer user_data;

	/* Async job state passed to the update callback. */
	gpointer async_job_data;

	/* When will update_callback be called next. */
	gint64 next_update_callback_time;

	/* When will update_callback be called next. */
	gint64 next_text_update_callback_time;

	/* Period at which the update_callback will be called. */
	gint64 update_callback_period;


	/* Reserved for future expansions to GnomeVFSProgressCallbackState
	 * without having to break ABI compatibility */
	void *reserved1;
	void *reserved2;

} GnomeVFSProgressCallbackState;

GnomeVFSResult gnome_vfs_xfer_uri_list    (const GList                  *source_uri_list,
					   const GList                  *target_uri_list,
					   GnomeVFSXferOptions           xfer_options,
					   GnomeVFSXferErrorMode         error_mode,
					   GnomeVFSXferOverwriteMode     overwrite_mode,
					   GnomeVFSXferProgressCallback  progress_callback,
					   gpointer                      data);
GnomeVFSResult gnome_vfs_xfer_uri         (const GnomeVFSURI            *source_uri,
					   const GnomeVFSURI            *target_uri,
					   GnomeVFSXferOptions           xfer_options,
					   GnomeVFSXferErrorMode         error_mode,
					   GnomeVFSXferOverwriteMode     overwrite_mode,
					   GnomeVFSXferProgressCallback  progress_callback,
					   gpointer                      data);
GnomeVFSResult gnome_vfs_xfer_delete_list (const GList                  *source_uri_list,
					   GnomeVFSXferErrorMode         error_mode,
					   GnomeVFSXferOptions           xfer_options,
					   GnomeVFSXferProgressCallback  progress_callback,
					   gpointer                      data);

G_END_DECLS

#endif /* GNOME_VFS_XFER_H */
