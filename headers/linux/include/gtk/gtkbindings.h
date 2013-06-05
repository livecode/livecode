/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkBindingSet: Keybinding manager for GtkObjects.
 * Copyright (C) 1998 Tim Janik
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

#ifndef __GTK_BINDINGS_H__
#define __GTK_BINDINGS_H__


#include <gdk/gdk.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkenums.h>


G_BEGIN_DECLS


/* Binding sets
 */

typedef struct _GtkBindingSet		GtkBindingSet;
typedef struct _GtkBindingEntry		GtkBindingEntry;
typedef struct _GtkBindingSignal	GtkBindingSignal;
typedef struct _GtkBindingArg		GtkBindingArg;

struct _GtkBindingSet
{
  gchar			*set_name;
  gint			 priority;
  GSList		*widget_path_pspecs;
  GSList		*widget_class_pspecs;
  GSList		*class_branch_pspecs;
  GtkBindingEntry	*entries;
  GtkBindingEntry	*current;
  guint                  parsed : 1; /* From RC content */
};

struct _GtkBindingEntry
{
  /* key portion
   */
  guint			 keyval;
  GdkModifierType	 modifiers;
  
  GtkBindingSet		*binding_set;
  guint			destroyed : 1;
  guint			in_emission : 1;
  GtkBindingEntry	*set_next;
  GtkBindingEntry	*hash_next;
  GtkBindingSignal	*signals;
};

struct _GtkBindingArg
{
  GType		 arg_type;
  union {
    glong	 long_data;
    gdouble	 double_data;
    gchar	*string_data;
  } d;
};

struct _GtkBindingSignal
{
  GtkBindingSignal	*next;
  gchar 		*signal_name;
  guint			 n_args;
  GtkBindingArg		*args;
};

/* Application-level methods */

GtkBindingSet*	gtk_binding_set_new	(const gchar	*set_name);
GtkBindingSet*	gtk_binding_set_by_class(gpointer	 object_class);
GtkBindingSet*	gtk_binding_set_find	(const gchar	*set_name);
gboolean gtk_bindings_activate		(GtkObject	*object,
					 guint		 keyval,
					 GdkModifierType modifiers);
gboolean gtk_bindings_activate_event    (GtkObject      *object,
					 GdkEventKey    *event);
gboolean gtk_binding_set_activate	(GtkBindingSet	*binding_set,
					 guint		 keyval,
					 GdkModifierType modifiers,
					 GtkObject	*object);
#define	 gtk_binding_entry_add		gtk_binding_entry_clear
void	 gtk_binding_entry_clear	(GtkBindingSet	*binding_set,
					 guint		 keyval,
					 GdkModifierType modifiers);
void	 gtk_binding_entry_add_signal	(GtkBindingSet	*binding_set,
					 guint		 keyval,
					 GdkModifierType modifiers,
					 const gchar	*signal_name,
					 guint		 n_args,
					 ...);
void	 gtk_binding_set_add_path	(GtkBindingSet	*binding_set,
					 GtkPathType	 path_type,
					 const gchar	*path_pattern,
					 GtkPathPriorityType priority);


/* Non-public methods */

void	 gtk_binding_entry_remove	(GtkBindingSet	*binding_set,
					 guint		 keyval,
					 GdkModifierType modifiers);
void	 gtk_binding_entry_add_signall	(GtkBindingSet	*binding_set,
					 guint		 keyval,
					 GdkModifierType modifiers,
					 const gchar	*signal_name,
					 GSList		*binding_args);
guint	 gtk_binding_parse_binding	(GScanner	*scanner);


void     _gtk_binding_reset_parsed    (void);

/* Creates a signal with a fixed callback instead of a class offset;
 * useful for key binding signals
 */
guint _gtk_binding_signal_new (const gchar       *signal_name,
			       GType		  itype,
			       GSignalFlags	  signal_flags,
			       GCallback          handler,
			       GSignalAccumulator accumulator,
			       gpointer		  accu_data,
			       GSignalCMarshaller c_marshaller,
			       GType		  return_type,
			       guint		  n_params,
			       ...);

G_END_DECLS


#endif /* __GTK_BINDINGS_H__ */
