/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-types.h - Types used by the GNOME Virtual File System.

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

   Author: Ettore Perazzoli <ettore@gnu.org>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef GNOME_VFS_TYPES_H
#define GNOME_VFS_TYPES_H

#ifndef GNOME_VFS_DISABLE_DEPRECATED

/* see gnome-vfs-file-size.h for GNOME_VFS_SIZE_IS_<type> */
/* see gnome-vfs-file-size.h for GNOME_VFS_OFFSET_IS_<type> */
/* see gnome-vfs-file-size.h for GNOME_VFS_SIZE_FORMAT_STR */
/* see gnome-vfs-file-size.h for GNOME_VFS_OSFFSET_FORMAT_STR */
/* see gnome-vfs-file-size.h for GnomeVFSFileSize */
/* see gnome-vfs-file-size.h for GnomeVFSFileOffset */
/* see gnome-vfs-result.h for GnomeVFSResult */
/* see gnome-vfs-method.h for GnomeVFSOpenMode */
/* see gnome-vfs-method.h for GnomeVFSSeekPosition */
/* see gnome-vfs-file-info.h for GnomeVFSFileType */
/* see gnome-vfs-file-info.h for GnomeVFSFilePermissions */
/* see gnome-vfs-handle.h for GnomeVFSHandle */
/* see gnome-vfs-uri.h for GnomeVFSURI */
/* see gnome-vfs-uri.h for GnomeVFSToplevelURI */
/* see gnome-vfs-uri.h for GnomeVFSURIHideOptions */
/* see gnome-vfs-file-info.h for GnomeVFSFileFlags */
/* see gnome-vfs-file-info.h for GnomeVFSFileInfoFields */
/* see gnome-vfs-file-info.h for GnomeVFSFileInfo */
/* see gnome-vfs-file-info.h for GnomeVFSFileInfoOptions */
/* see gnome-vfs-file-info.h for GnomeVFSFileInfoMask */
/* see gnome-vfs-find-directory.h for GnomeVFSFindDirectoryKind */
/* see gnome-vfs-xfer.h for GnomeVFSXferOptions */
/* see gnome-vfs-xfer.h for GnomeVFSXferProgressStatus */
/* see gnome-vfs-xfer.h for GnomeVFSXferOverwriteMode */
/* see gnome-vfs-xfer.h for GnomeVFSXferOverwriteAction */
/* see gnome-vfs-xfer.h for GnomeVFSXferErrorMode */
/* see gnome-vfs-xfer.h for GnomeVFSXferErrorAction */
/* see gnome-vfs-xfer.h for GnomeVFSXferPhase */
/* see gnome-vfs-xfer.h for GnomeVFSXferProgressInfo */
/* see gnome-vfs-xfer.h for GnomeVFSXferProgressCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncHandle */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncOpenCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncCreateCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncOpenAsChannelCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncCloseCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncReadCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncWriteCallback */
/* see gnome-vfs-file-info.h for GnomeVFSFileInfoResult */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncGetFileInfoCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncSetFileInfoCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncDirectoryLoadCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncXferProgressCallback */
/* see gnome-vfs-async-ops.h for GnomeVFSFindDirectoryResult */
/* see gnome-vfs-async-ops.h for GnomeVFSAsyncFindDirectoryCallback */
/* see gnome-vfs-module-callback.h for GnomeVFSModuleCallback */

/* Includes to provide compatibility with programs that
   still include gnome-vfs-types.h directly */
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-find-directory.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-xfer.h>

#endif /* GNOME_VFS_DISABLE_DEPRECATED */

#endif /* GNOME_VFS_TYPES_H */
