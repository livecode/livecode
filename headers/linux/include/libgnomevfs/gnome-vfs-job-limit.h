/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-job-limit.h - Job queue limit manipulation for asynchronous 
   operations of the GNOME Virtual File System (version for POSIX threads).

   Copyright (C) 2001 Free Software Foundation

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

   Author: Laszlo Peter <laca@ireland.sun.com>
*/

#ifndef GNOME_VFS_JOB_LIMIT_H
#define GNOME_VFS_JOB_LIMIT_H

void	      gnome_vfs_async_set_job_limit  (int              limit);
int	      gnome_vfs_async_get_job_limit  (void);

#endif /* GNOME_VFS_JOB_LIMIT_H */
