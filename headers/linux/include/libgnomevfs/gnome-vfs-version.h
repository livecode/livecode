/* gnome-vfs-version.h - GnomeVFS version checking

   Copyright (C) 2006 Christian Kellner

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

#ifndef GNOME_VFS_VERSION_H
#define GNOME_VFS_VERSION_H

/* compile time versioning
 */
#define GNOME_VFS_MAJOR_VERSION			(2)
#define GNOME_VFS_MINOR_VERSION			(18)
#define GNOME_VFS_MICRO_VERSION			(1)

/* check whether a GnomeVFS version equal to or greater than
 * major.minor.micro is present.
 */
#define	GNOME_VFS_CHECK_VERSION(major,minor,micro)	\
    (GNOME_VFS_MAJOR_VERSION > (major) || \
     (GNOME_VFS_MAJOR_VERSION == (major) && \
      GNOME_VFS_MINOR_VERSION > (minor)) || \
     (GNOME_VFS_MAJOR_VERSION == (major) && \
      GNOME_VFS_MINOR_VERSION == (minor) && \
      GNOME_VFS_MICRO_VERSION >= (micro)))


#endif /* GNOME_VFS_VERSION_H */

