/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@netscape.com>  (Original Author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * gtkdrawing.h: GTK widget rendering utilities
 *
 * gtkdrawing provides an API for rendering GTK widgets in the
 * current theme to a pixmap or window, without requiring an actual
 * widget instantiation, similar to the Macintosh Appearance Manager
 * or Windows XP's DrawThemeBackground() API.
 */

#ifndef _GTK_DRAWING_H_
#define _GTK_DRAWING_H_

#include <X11/Xlib.h>

#include <gdk/gdk.h>
#include <gtk/gtkstyle.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

	/*** type definitions ***/
	typedef struct
	{
		guint8 active;
		guint8 focused;
		guint8 inHover;
		guint8 disabled;
		guint8 isDefault;
		guint8 canDefault;
		gint32 curpos; /* curpos and maxpos are used for scrollbars */
		gint32 maxpos; /* NOW they are used also for the tabpanel gap :) */
	}
	GtkWidgetState;

	/** flags for tab state **/
	typedef enum {
	    /* the first tab in the group */
	    MOZ_GTK_TAB_FIRST           = 1 << 0,
	    /* the tab just before the selected tab */
	    MOZ_GTK_TAB_BEFORE_SELECTED = 1 << 1,
	    /* the selected tab */
	    MOZ_GTK_TAB_SELECTED        = 1 << 2,
	    /* tabs along top */
	    MOZ_GTK_TAB_POS_TOP= 1 << 3,
	    /* tabs along bottom */
	    MOZ_GTK_TAB_POS_BOTTOM= 1 << 4,
	    /* tabs along left */
	    MOZ_GTK_TAB_POS_LEFT= 1 << 5,
	    /* tabs along right */
	    MOZ_GTK_TAB_POS_RIGHT= 1 << 6

	} GtkTabFlags;

	/* function type for moz_gtk_enable_style_props */
	typedef gint (*style_prop_t)(GtkStyle*, const gchar*, gint);

	/*** result/error codes ***/
#define MOZ_GTK_SUCCESS 0
#define MOZ_GTK_UNKNOWN_WIDGET -1
#define MOZ_GTK_UNSAFE_THEME -2

	/*** widget type constants ***/
	typedef enum {
	    /* Paints a GtkButton. flags is a GtkReliefStyle. */
	    MOZ_GTK_BUTTON,
	    /* Paints a GtkCheckButton. flags is a boolean, 1=checked, 0=not checked. */
	    MOZ_GTK_CHECKBUTTON,
	    /* Paints a GtkRadioButton. flags is a boolean, 1=checked, 0=not checked. */
	    MOZ_GTK_RADIOBUTTON,
	    /**
	     * Paints the button of a GtkScrollbar. flags is a GtkArrowType giving
	     * the arrow direction.
	     */
	    MOZ_GTK_SCROLLBAR_BUTTON,
	    /* Paints the trough (track) of a GtkScrollbar. */
	    MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL,
	    MOZ_GTK_SCROLLBAR_TRACK_VERTICAL,
	    /* Paints the slider (thumb) of a GtkScrollbar. */
	    MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL,
	    MOZ_GTK_SCROLLBAR_THUMB_VERTICAL,
	    /* Paints the gripper of a GtkHandleBox. */
	    MOZ_GTK_GRIPPER,
	    /* Paints a GtkEntry. */
	    MOZ_GTK_ENTRY,
	    MOZ_GTK_ENTRY_FRAME,
	    /* Paints a dropdown arrow (a GtkButton containing a down GtkArrow). */
	    MOZ_GTK_DROPDOWN_ARROW,
	    /* Paints the container part of a GtkCheckButton. */
	    MOZ_GTK_CHECKBUTTON_CONTAINER,
	    /* Paints the container part of a GtkRadioButton. */
	    MOZ_GTK_RADIOBUTTON_CONTAINER,
	    /* Paints the background of a GtkHandleBox. */
	    MOZ_GTK_TOOLBAR,
	    /* Paints a GtkToolTip */
	    MOZ_GTK_TOOLTIP,
	    /* Paints a GtkFrame (e.g. a status bar panel). */
	    MOZ_GTK_FRAME,
	    /* Paints a GtkProgressBar. */
	    MOZ_GTK_PROGRESSBAR,
	    /* Paints a progress chunk of a GtkProgressBar. */
	    MOZ_GTK_PROGRESS_CHUNK,
	    /* Paints a tab of a GtkNotebook. flags is a GtkTabFlags, defined above. */
	    MOZ_GTK_TAB,
	    MOZ_GTK_LABEL,
	    /* Paints the background and border of a GtkNotebook. */
	    MOZ_GTK_TABPANELS,
	    MOZ_GTK_OPTIONBUTTON,
	    MOZ_GTK_LISTBOX,
	    MOZ_GTK_SPINBUTTON,
	    MOZ_GTK_MENUITEMHIGHLIGHT,
	    MOZ_GTK_SCALE_TRACK_VERTICAL,
	    MOZ_GTK_SCALE_TRACK_HORIZONTAL
	} GtkThemeWidgetType;

	/*** General library functions ***/
	/**
	 * Initializes the drawing library.  You must call this function
	 * prior to using any other functionality.
	 * returns: MOZ_GTK_SUCCESS if there were no errors
	 *          MOZ_GTK_UNSAFE_THEME if the current theme engine is known
	 *                               to crash with gtkdrawing.
	 */
	gint moz_gtk_init();

	/**
	 * Enable GTK+ 1.2.9+ theme enhancements. You must provide a pointer
	 * to the GTK+ 1.2.9+ function "gtk_style_get_prop_experimental".
	 * styleGetProp:  pointer to gtk_style_get_prop_experimental
	 * 
	 * returns: MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint moz_gtk_enable_style_props(style_prop_t styleGetProp);

	/**
	 * Perform cleanup of the drawing library. You should call this function
	 * when your program exits, or you no longer need the library.
	 *
	 * returns: MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint moz_gtk_shutdown();


	/*** Widget drawing ***/
	/**
	 * Paint a widget in the current theme.
	 * widget:   a constant giving the widget to paint
	 * rect:     the bounding rectangle for the widget
	 * cliprect: a clipprect rectangle for this painting operation
	 * state:    the state of the widget.  ignored for some widgets.
	 * flags:    widget-dependant flags; see the GtkThemeWidgetType definition.
	 */
	gint
	moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable* drawable,
	                     GdkRectangle* rect, GdkRectangle* cliprect,
	                     GtkWidgetState* state, gint flags);


	/*** Widget metrics ***/
	/**
	 * Get the border size of a widget
	 * xthickness:  [OUT] the widget's left/right border
	 * ythickness:  [OUT] the widget's top/bottom border
	 *
	 * returns:    MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint* xthickness,
	                               gint* ythickness);

	/**
	 * Get the desired size of a GtkCheckButton
	 * indicator_size:     [OUT] the indicator size
	 * indicator_spacing:  [OUT] the spacing between the indicator and its
	 *                     container
	 *
	 * returns:    MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint
	moz_gtk_checkbox_get_metrics(gint* indicator_size, gint* indicator_spacing);

	gint moz_gtk_radiobutton_get_metrics(gint * indicator_size,
	                                     gint * indicator_spacing);

	/**
	 * Get the desired size of a GtkRadioButton
	 * indicator_size:     [OUT] the indicator size
	 * indicator_spacing:  [OUT] the spacing between the indicator and its
	 *                     container
	 *
	 * returns:    MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
#define moz_gtk_radio_get_metrics(x, y) moz_gtk_checkbox_get_metrics(x, y)

	/**
	 * Get the desired metrics for a GtkScrollbar
	 * slider_width:     [OUT] the width of the slider (thumb)
	 * trough_border:    [OUT] the border of the trough (outside the thumb)
	 * stepper_size:     [OUT] the size of an arrow button
	 * stepper_spacing:  [OUT] the minimum space between the thumb and the arrow
	 * min_slider_size:  [OUT] the minimum thumb size
	 *
	 * returns:    MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint
	moz_gtk_get_scrollbar_metrics(gint* slider_width, gint* trough_border,
	                              gint* stepper_size, gint* stepper_spacing,
	                              gint* min_slider_size, gint*, gint*);

	gint
	moz_gtk_get_slider_metrics(gint* slider_width, gint* trough_border,
	                           gint* stepper_size, gint* stepper_spacing,
	                           gint* min_slider_size, gint*, gint*);

	/**
	 * Get the desired size of a dropdown arrow button
	 * width:   [OUT] the desired width
	 * height:  [OUT] the desired height
	 *
	 * returns:    MOZ_GTK_SUCCESS if there was no error, an error code otherwise
	 */
	gint moz_gtk_get_dropdown_arrow_size(gint* width, gint* height);

	void spinbutton_get_rects(GtkArrowType type, GdkRectangle *rect,
	                          GdkRectangle &framerect, GdkRectangle &btnrect);

	void moz_gtk_get_widget_color(GtkStateType widgettype,
	                              uint2 &red,uint2 &blue,uint2 &green);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
