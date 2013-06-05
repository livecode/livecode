/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_PRIVATE_H__
#define __GTK_PRIVATE_H__


#include <gtk/gtkobject.h>
#include <gtk/gtkwidget.h>


G_BEGIN_DECLS

/* The private flags that are used in the private_flags member of GtkWidget.
 */
typedef enum
{
  PRIVATE_GTK_USER_STYLE	= 1 <<  0,
  PRIVATE_GTK_RESIZE_PENDING	= 1 <<  2,
  PRIVATE_GTK_LEAVE_PENDING	= 1 <<  4,
  PRIVATE_GTK_HAS_SHAPE_MASK	= 1 <<  5,
  PRIVATE_GTK_IN_REPARENT       = 1 <<  6,
  PRIVATE_GTK_DIRECTION_SET     = 1 <<  7,   /* If the reading direction is not DIR_NONE */
  PRIVATE_GTK_DIRECTION_LTR     = 1 <<  8,   /* If the reading direction is DIR_LTR */
  PRIVATE_GTK_ANCHORED          = 1 <<  9,   /* If widget has a GtkWindow ancestor */
  PRIVATE_GTK_CHILD_VISIBLE     = 1 <<  10,  /* If widget should be mapped when parent is mapped */
  PRIVATE_GTK_REDRAW_ON_ALLOC   = 1 <<  11,  /* If we should queue a draw on the entire widget when it is reallocated */
  PRIVATE_GTK_ALLOC_NEEDED      = 1 <<  12,  /* If we we should allocate even if the allocation is the same */
  PRIVATE_GTK_REQUEST_NEEDED    = 1 <<  13   /* Whether we need to call gtk_widget_size_request */
} GtkPrivateFlags;

/* Macros for extracting a widgets private_flags from GtkWidget.
 */
#define GTK_PRIVATE_FLAGS(wid)            (GTK_WIDGET (wid)->private_flags)
#define GTK_WIDGET_USER_STYLE(obj)	  ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_USER_STYLE) != 0)
#define GTK_CONTAINER_RESIZE_PENDING(obj) ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_RESIZE_PENDING) != 0)
#define GTK_WIDGET_LEAVE_PENDING(obj)	  ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_LEAVE_PENDING) != 0)
#define GTK_WIDGET_HAS_SHAPE_MASK(obj)	  ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_HAS_SHAPE_MASK) != 0)
#define GTK_WIDGET_IN_REPARENT(obj)	  ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_IN_REPARENT) != 0)
#define GTK_WIDGET_DIRECTION_SET(obj)	  ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_DIRECTION_SET) != 0)
#define GTK_WIDGET_DIRECTION_LTR(obj)     ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_DIRECTION_LTR) != 0)
#define GTK_WIDGET_ANCHORED(obj)          ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_ANCHORED) != 0)
#define GTK_WIDGET_CHILD_VISIBLE(obj)     ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_CHILD_VISIBLE) != 0)
#define GTK_WIDGET_REDRAW_ON_ALLOC(obj)   ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_REDRAW_ON_ALLOC) != 0)
#define GTK_WIDGET_ALLOC_NEEDED(obj)      ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_ALLOC_NEEDED) != 0)
#define GTK_WIDGET_REQUEST_NEEDED(obj)    ((GTK_PRIVATE_FLAGS (obj) & PRIVATE_GTK_REQUEST_NEEDED) != 0)

/* Macros for setting and clearing private widget flags.
 * we use a preprocessor string concatenation here for a clear
 * flags/private_flags distinction at the cost of single flag operations.
 */
#define GTK_PRIVATE_SET_FLAG(wid,flag)    G_STMT_START{ (GTK_PRIVATE_FLAGS (wid) |= (PRIVATE_ ## flag)); }G_STMT_END
#define GTK_PRIVATE_UNSET_FLAG(wid,flag)  G_STMT_START{ (GTK_PRIVATE_FLAGS (wid) &= ~(PRIVATE_ ## flag)); }G_STMT_END

#ifdef G_OS_WIN32

const gchar *_gtk_get_datadir ();
const gchar *_gtk_get_libdir ();
const gchar *_gtk_get_sysconfdir ();
const gchar *_gtk_get_localedir ();
const gchar *_gtk_get_data_prefix ();

#undef GTK_DATADIR
#define GTK_DATADIR _gtk_get_datadir ()
#undef GTK_LIBDIR
#define GTK_LIBDIR _gtk_get_libdir ()
#undef GTK_LOCALEDIR
#define GTK_LOCALEDIR _gtk_get_localedir ()
#undef GTK_SYSCONFDIR
#define GTK_SYSCONFDIR _gtk_get_sysconfdir ()
#undef GTK_DATA_PREFIX
#define GTK_DATA_PREFIX _gtk_get_data_prefix ()

#endif /* G_OS_WIN32 */

#if defined(__linux__) && defined (__i386__)
const gchar *_gtk_get_libdir ();

#undef GTK_LIBDIR
#define GTK_LIBDIR _gtk_get_libdir ()
#endif

gboolean _gtk_fnmatch (const char *pattern,
		       const char *string,
		       gboolean    no_leading_period);

#define GTK_PARAM_READABLE G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GTK_PARAM_WRITABLE G_PARAM_WRITABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

G_END_DECLS

#endif /* __GTK_PRIVATE_H__ */
