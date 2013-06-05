/*
 * gdkdisplay.h
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GDK_DISPLAY_H__
#define __GDK_DISPLAY_H__

#include <gdk/gdktypes.h>
#include <gdk/gdkevents.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GdkDisplayClass GdkDisplayClass;
typedef struct _GdkDisplayPointerHooks GdkDisplayPointerHooks;

#define GDK_TYPE_DISPLAY              (gdk_display_get_type ())
#define GDK_DISPLAY_OBJECT(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DISPLAY, GdkDisplay))
#define GDK_DISPLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DISPLAY, GdkDisplayClass))
#define GDK_IS_DISPLAY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DISPLAY))
#define GDK_IS_DISPLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DISPLAY))
#define GDK_DISPLAY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DISPLAY, GdkDisplayClass))

struct _GdkDisplay
{
  GObject parent_instance;

  /*< private >*/
  GList *queued_events;
  GList *queued_tail;

  /* Information for determining if the latest button click
   * is part of a double-click or triple-click
   */
  guint32 button_click_time[2];	/* The last 2 button click times. */
  GdkWindow *button_window[2];  /* The last 2 windows to receive button presses. */
  gint button_number[2];        /* The last 2 buttons to be pressed. */

  guint double_click_time;	/* Maximum time between clicks in msecs */
  GdkDevice *core_pointer;	/* Core pointer device */

  const GdkDisplayPointerHooks *pointer_hooks; /* Current hooks for querying pointer */
  
  guint closed : 1;		/* Whether this display has been closed */

  guint double_click_distance;	/* Maximum distance between clicks in pixels */
  gint button_x[2];             /* The last 2 button click positions. */
  gint button_y[2];
};

struct _GdkDisplayClass
{
  GObjectClass parent_class;
  
  G_CONST_RETURN gchar *     (*get_display_name)   (GdkDisplay *display);
  gint			     (*get_n_screens)      (GdkDisplay *display);
  GdkScreen *		     (*get_screen)         (GdkDisplay *display,
						    gint        screen_num);
  GdkScreen *		     (*get_default_screen) (GdkDisplay *display);

  
  /* Signals */
  void (*closed) (GdkDisplay *display,
		  gboolean    is_error);
};

struct _GdkDisplayPointerHooks
{
  void (*get_pointer)              (GdkDisplay      *display,
				    GdkScreen      **screen,
				    gint            *x,
				    gint            *y,
				    GdkModifierType *mask);
  GdkWindow* (*window_get_pointer) (GdkDisplay      *display,
				    GdkWindow       *window,
				    gint            *x,
				    gint            *y,
				    GdkModifierType *mask);
  GdkWindow* (*window_at_pointer)  (GdkDisplay      *display,
				    gint            *win_x,
				    gint            *win_y);
};

GType       gdk_display_get_type (void) G_GNUC_CONST;
GdkDisplay *gdk_display_open                (const gchar *display_name);

G_CONST_RETURN gchar * gdk_display_get_name (GdkDisplay *display);

gint        gdk_display_get_n_screens      (GdkDisplay  *display);
GdkScreen * gdk_display_get_screen         (GdkDisplay  *display,
					    gint         screen_num);
GdkScreen * gdk_display_get_default_screen (GdkDisplay  *display);
void        gdk_display_pointer_ungrab     (GdkDisplay  *display,
					    guint32      time_);
void        gdk_display_keyboard_ungrab    (GdkDisplay  *display,
					    guint32      time_);
gboolean    gdk_display_pointer_is_grabbed (GdkDisplay  *display);
void        gdk_display_beep               (GdkDisplay  *display);
void        gdk_display_sync               (GdkDisplay  *display);
void        gdk_display_flush              (GdkDisplay  *display);

void	    gdk_display_close		   (GdkDisplay  *display);

GList *     gdk_display_list_devices       (GdkDisplay  *display);

GdkEvent* gdk_display_get_event  (GdkDisplay *display);
GdkEvent* gdk_display_peek_event (GdkDisplay *display);
void      gdk_display_put_event  (GdkDisplay *display,
				  GdkEvent   *event);

void gdk_display_add_client_message_filter (GdkDisplay   *display,
					    GdkAtom       message_type,
					    GdkFilterFunc func,
					    gpointer      data);

void gdk_display_set_double_click_time     (GdkDisplay   *display,
					    guint         msec);
void gdk_display_set_double_click_distance (GdkDisplay   *display,
					    guint         distance);

GdkDisplay *gdk_display_get_default (void);

GdkDevice  *gdk_display_get_core_pointer (GdkDisplay *display);

void             gdk_display_get_pointer           (GdkDisplay             *display,
						    GdkScreen             **screen,
						    gint                   *x,
						    gint                   *y,
						    GdkModifierType        *mask);
GdkWindow *      gdk_display_get_window_at_pointer (GdkDisplay             *display,
						    gint                   *win_x,
						    gint                   *win_y);
void             gdk_display_warp_pointer          (GdkDisplay             *display,
						    GdkScreen              *screen,
						    gint                   x,
						    gint                   y);

GdkDisplayPointerHooks *gdk_display_set_pointer_hooks (GdkDisplay                   *display,
						       const GdkDisplayPointerHooks *new_hooks);

GdkDisplay *gdk_display_open_default_libgtk_only (void);

gboolean gdk_display_supports_cursor_alpha     (GdkDisplay    *display);
gboolean gdk_display_supports_cursor_color     (GdkDisplay    *display);
guint    gdk_display_get_default_cursor_size   (GdkDisplay    *display);
void     gdk_display_get_maximal_cursor_size   (GdkDisplay    *display,
						guint         *width,
						guint         *height);

GdkWindow *gdk_display_get_default_group       (GdkDisplay *display); 

gboolean gdk_display_supports_selection_notification (GdkDisplay *display);
gboolean gdk_display_request_selection_notification  (GdkDisplay *display,
						      GdkAtom     selection);

gboolean gdk_display_supports_clipboard_persistence (GdkDisplay *display);
void     gdk_display_store_clipboard                (GdkDisplay *display,
						     GdkWindow  *clipboard_window,
						     guint32     time_,
						     GdkAtom    *targets,
						     gint        n_targets);

gboolean gdk_display_supports_shapes           (GdkDisplay    *display);
gboolean gdk_display_supports_input_shapes     (GdkDisplay    *display);

G_END_DECLS

#endif	/* __GDK_DISPLAY_H__ */
