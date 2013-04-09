/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-result.h - Result handling for the GNOME Virtual File System.

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef GNOME_VFS_RESULT_H
#define GNOME_VFS_RESULT_H

#include <glib/gmacros.h>

G_BEGIN_DECLS

/* IMPORTANT NOTICE: If you add error types here, please also add the
   corresponding descriptions in `gnome-vfs-result.c'.  Moreover, *always* add
   new values at the end of the list, and *never* remove values.  */

/**
 * GnomeVFSResult:
 * @GNOME_VFS_OK: No error.
 * @GNOME_VFS_ERROR_NOT_FOUND: File not found.
 * @GNOME_VFS_ERROR_GENERIC: Generic error.
 * @GNOME_VFS_ERROR_INTERNAL: Internal error.
 * @GNOME_VFS_ERROR_BAD_PARAMETERS: Invalid parameters.
 * @GNOME_VFS_ERROR_NOT_SUPPORTED: Unsupported operation.
 * @GNOME_VFS_ERROR_IO: I/O error.
 * @GNOME_VFS_ERROR_CORRUPTED_DATA: Data corrupted.
 * @GNOME_VFS_ERROR_WRONG_FORMAT: Format not valid.
 * @GNOME_VFS_ERROR_BAD_FILE: Bad file handle.
 * @GNOME_VFS_ERROR_TOO_BIG: File too big.
 * @GNOME_VFS_ERROR_NO_SPACE: No space left on device.
 * @GNOME_VFS_ERROR_READ_ONLY: Read-only file system.
 * @GNOME_VFS_ERROR_INVALID_URI: Invalid URI.
 * @GNOME_VFS_ERROR_NOT_OPEN: File not open.
 * @GNOME_VFS_ERROR_INVALID_OPEN_MODE: Open mode not valid.
 * @GNOME_VFS_ERROR_ACCESS_DENIED: Access denied.
 * @GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES: Too many open files.
 * @GNOME_VFS_ERROR_EOF: End of file.
 * @GNOME_VFS_ERROR_NOT_A_DIRECTORY: Not a directory.
 * @GNOME_VFS_ERROR_IN_PROGRESS: Operation in progress.
 * @GNOME_VFS_ERROR_INTERRUPTED: Operation interrupted.
 * @GNOME_VFS_ERROR_FILE_EXISTS: File exists.
 * @GNOME_VFS_ERROR_LOOP: Looping links encountered.
 * @GNOME_VFS_ERROR_NOT_PERMITTED: Operation not permitted.
 * @GNOME_VFS_ERROR_IS_DIRECTORY: Is a directory.
 * @GNOME_VFS_ERROR_NO_MEMORY: Not enough memory.
 * @GNOME_VFS_ERROR_HOST_NOT_FOUND: Host not found.
 * @GNOME_VFS_ERROR_INVALID_HOST_NAME: Host name not valid.
 * @GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS: Host has no address.
 * @GNOME_VFS_ERROR_LOGIN_FAILED: Login failed.
 * @GNOME_VFS_ERROR_CANCELLED: Operation cancelled.
 * @GNOME_VFS_ERROR_DIRECTORY_BUSY: Directory busy.
 * @GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY: Directory not empty.
 * @GNOME_VFS_ERROR_TOO_MANY_LINKS: Too many links.
 * @GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM: Read only file system.
 * @GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM: Not on the same file system.
 * @GNOME_VFS_ERROR_NAME_TOO_LONG: Name too long.
 * @GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE: Service not available.
 * @GNOME_VFS_ERROR_SERVICE_OBSOLETE: Request obsoletes service's data.
 * @GNOME_VFS_ERROR_PROTOCOL_ERROR: Protocol error.
 * @GNOME_VFS_ERROR_NO_MASTER_BROWSER: Could not find master browser.
 * @GNOME_VFS_ERROR_NO_DEFAULT: No default action associated.
 * @GNOME_VFS_ERROR_NO_HANDLER: No handler for URL scheme.
 * @GNOME_VFS_ERROR_PARSE: Error parsing command line.
 * @GNOME_VFS_ERROR_LAUNCH: Error launching command.
 * @GNOME_VFS_ERROR_TIMEOUT: Timeout reached.
 * @GNOME_VFS_ERROR_NAMESERVER: Nameserver error.
 * @GNOME_VFS_ERROR_LOCKED: The resource is locked.
 * @GNOME_VFS_ERROR_DEPRECATED_FUNCTION: Function call deprecated.
 * @GNOME_VFS_ERROR_INVALID_FILENAME: The specified filename is invalid.
 * @GNOME_VFS_ERROR_NOT_A_SYMBOLIC_LINK: Not a symbolic link.
 *
 * A #GnomeVFSResult informs library clients about the result of a file operation.
 * Unless it is #GNOME_VFS_OK, it denotes that a problem occurred and the operation
 * could not be executed successfully.
 *
 * gnome_vfs_result_to_string() provides a textual representation of #GnomeVFSResults.
 **/
typedef enum {
	GNOME_VFS_OK,
	GNOME_VFS_ERROR_NOT_FOUND,
	GNOME_VFS_ERROR_GENERIC,
	GNOME_VFS_ERROR_INTERNAL,
	GNOME_VFS_ERROR_BAD_PARAMETERS,
	GNOME_VFS_ERROR_NOT_SUPPORTED,
	GNOME_VFS_ERROR_IO,
	GNOME_VFS_ERROR_CORRUPTED_DATA,
	GNOME_VFS_ERROR_WRONG_FORMAT,
	GNOME_VFS_ERROR_BAD_FILE,
	GNOME_VFS_ERROR_TOO_BIG,
	GNOME_VFS_ERROR_NO_SPACE,
	GNOME_VFS_ERROR_READ_ONLY,
	GNOME_VFS_ERROR_INVALID_URI,
	GNOME_VFS_ERROR_NOT_OPEN,
	GNOME_VFS_ERROR_INVALID_OPEN_MODE,
	GNOME_VFS_ERROR_ACCESS_DENIED,
	GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES,
	GNOME_VFS_ERROR_EOF,
	GNOME_VFS_ERROR_NOT_A_DIRECTORY,
	GNOME_VFS_ERROR_IN_PROGRESS,
	GNOME_VFS_ERROR_INTERRUPTED,
	GNOME_VFS_ERROR_FILE_EXISTS,
	GNOME_VFS_ERROR_LOOP,
	GNOME_VFS_ERROR_NOT_PERMITTED,
	GNOME_VFS_ERROR_IS_DIRECTORY,
	GNOME_VFS_ERROR_NO_MEMORY,
	GNOME_VFS_ERROR_HOST_NOT_FOUND,
	GNOME_VFS_ERROR_INVALID_HOST_NAME,
	GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS,
	GNOME_VFS_ERROR_LOGIN_FAILED,
	GNOME_VFS_ERROR_CANCELLED,
	GNOME_VFS_ERROR_DIRECTORY_BUSY,
	GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY,
	GNOME_VFS_ERROR_TOO_MANY_LINKS,
	GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM,
	GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM,
	GNOME_VFS_ERROR_NAME_TOO_LONG,
	GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE,
	GNOME_VFS_ERROR_SERVICE_OBSOLETE,
	GNOME_VFS_ERROR_PROTOCOL_ERROR,
	GNOME_VFS_ERROR_NO_MASTER_BROWSER,
	GNOME_VFS_ERROR_NO_DEFAULT,
	GNOME_VFS_ERROR_NO_HANDLER,
	GNOME_VFS_ERROR_PARSE,
	GNOME_VFS_ERROR_LAUNCH,
	GNOME_VFS_ERROR_TIMEOUT,
	GNOME_VFS_ERROR_NAMESERVER,
	GNOME_VFS_ERROR_LOCKED,
	GNOME_VFS_ERROR_DEPRECATED_FUNCTION,
	GNOME_VFS_ERROR_INVALID_FILENAME,
	GNOME_VFS_ERROR_NOT_A_SYMBOLIC_LINK,
	GNOME_VFS_NUM_ERRORS
} GnomeVFSResult;

const char	*gnome_vfs_result_to_string	    (GnomeVFSResult result);
GnomeVFSResult   gnome_vfs_result_from_errno_code   (int errno_code);
GnomeVFSResult	 gnome_vfs_result_from_errno	    (void);
GnomeVFSResult   gnome_vfs_result_from_h_errno_val  (int h_errno_code);
GnomeVFSResult   gnome_vfs_result_from_h_errno      (void);

G_END_DECLS

#endif /* GNOME_VFS_RESULT_H */
