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

/*
 * This file contains painting functions for each of the gtk2 widgets.
 * Adapted from the gtkdrawing.c, and gtk+2.0 source.
 */

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "globals.h"

#include "util.h"

#include "mctheme.h"
#include "lnxdc.h"
#include "lnxgtkthemedrawing.h"

#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


typedef void (*gtk_widget_style_getPTR) (GtkWidget *widget, const gchar *first_property_name, ...);
extern gtk_widget_style_getPTR gtk_widget_style_get_ptr;

#define MIN_ARROW_WIDTH 6
#define XTHICKNESS(style) (style->xthickness)
#define YTHICKNESS(style) (style->ythickness)

#define WINDOW_IS_MAPPED(window) ((window) && GDK_IS_WINDOW(window) && gdk_window_is_visible(window))

static GtkWidget *gButtonWidget = 0;
static GtkWidget *gProtoWindow = 0;
static GtkWidget *gProtoLayout = 0;
static GtkWidget *gCheckboxWidget = 0;
static GtkWidget *gRadiobuttonWidget = 0;
static GtkWidget *gHorizScrollbarWidget = 0;
static GtkWidget *gVertScrollbarWidget = 0;
static GtkWidget *gEntryWidget = 0;
static GtkWidget *gArrowWidget = 0;
static GtkWidget *gDropdownButtonWidget = 0;
static GtkWidget *gHandleBoxWidget = 0;
static GtkWidget *gFrameWidget = 0;
static GtkWidget *gProgressWidget = 0;
static GtkWidget *gTabWidget = 0;
static GtkWidget *gLabelWidget = 0;
static GtkTooltips *gTooltipWidget = 0;
static GtkWidget *gOptionbuttonWidget = 0;
static GtkWidget *gSpinbuttonWidget = 0;
static GtkWidget *gMenuitemWidget = 0;
static GtkWidget *gHScaleWidget = 0;
static GtkWidget *gVScaleWidget = 0;

static style_prop_t style_prop_func;

static gint
moz_gtk_label_paint(GdkDrawable * drawable, GdkRectangle * rect,
                    GdkRectangle * cliprect);

typedef struct _GtkOptionMenuProps GtkOptionMenuProps;

struct _GtkOptionMenuProps
{
	gboolean interior_focus;
	GtkRequisition indicator_size;
	GtkBorder indicator_spacing;
	gint focus_width;
	gint focus_pad;
};

static const GtkOptionMenuProps default_props =
    {
        TRUE,
        { 7, 13 },
        { 7, 5, 2, 2 },		/* Left, right, top, bottom */
        1,
        0

    };

static gint
moz_gtk_container_paint(GdkDrawable * drawable, GdkRectangle * rect,
                        GdkRectangle * cliprect, GtkWidgetState * state,
                        gboolean isradio);



gint moz_gtk_enable_style_props(style_prop_t styleGetProp)
{
	style_prop_func = styleGetProp;
	return MOZ_GTK_SUCCESS;
}






static gint setup_widget_prototype(GtkWidget * widget)
{
	GtkStyle *newstyle ;
	
	if (!gProtoWindow)
	{
		gProtoWindow = gtk_window_new(GTK_WINDOW_POPUP);
		
		uint4 screendepth;
		if ( MCscreen != NULL)
			screendepth = ((MCScreenDC*)MCscreen)->getdepth();
		else 
			screendepth = 24;
        
        GdkVisual * t_vis = gdk_visual_get_best_with_depth (screendepth);
		if (t_vis != NULL)
        {
            gtk_widget_set_colormap ( GTK_WIDGET(gProtoWindow), gdk_colormap_new (t_vis, False ));
            gtk_widget_set_colormap ( GTK_WIDGET(widget), gdk_colormap_new (t_vis, False ));
        }
                                 
        gtk_widget_realize(gProtoWindow);
		gtk_widget_set_name(widget, "MozillaGtkWidget");

		gProtoLayout = gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(gProtoWindow), gProtoLayout);
	}

	gtk_container_add(GTK_CONTAINER(gProtoLayout), widget);
	gtk_widget_set_style(widget, NULL);

	gtk_widget_realize(widget);

    g_object_set_data(G_OBJECT(widget), "transparent-bg-hint", GINT_TO_POINTER(TRUE));

	return MOZ_GTK_SUCCESS;
}

static gint ensure_button_widget()
{
	if (!gButtonWidget)
	{
		gButtonWidget = gtk_button_new_with_label("M");
		setup_widget_prototype(gButtonWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_scale_widget()
{
	if (!gHScaleWidget)
	{
		GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(1, 1, 1, 1, 1, 1);
		gHScaleWidget = gtk_hscale_new(adj);
		gVScaleWidget = gtk_vscale_new(adj);

		setup_widget_prototype(gVScaleWidget);
		setup_widget_prototype(gHScaleWidget);

		g_object_unref(adj);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_menuitem_widget()
{
	if (!gMenuitemWidget)
	{
		gMenuitemWidget = gtk_menu_item_new_with_label("M");
		setup_widget_prototype(gMenuitemWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_spinbutton_widget()
{
	if (!gSpinbuttonWidget)
	{
		GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(1, 1, 1, 1, 1, 1);
		gSpinbuttonWidget = gtk_spin_button_new(adj, 1, 1);
		setup_widget_prototype(gSpinbuttonWidget);
		g_object_unref(adj);
	}
	return MOZ_GTK_SUCCESS;
}


static gint ensure_checkbox_widget()
{
	if (!gCheckboxWidget)
	{
		gCheckboxWidget = gtk_check_button_new_with_label("M");
		setup_widget_prototype(gCheckboxWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_radiobutton_widget()
{
	if (!gRadiobuttonWidget)
	{
		gRadiobuttonWidget =
		    gtk_radio_button_new_with_label(NULL, "M");
		setup_widget_prototype(gRadiobuttonWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_optionbutton_widget()
{
	if (!gOptionbuttonWidget)
	{
		gOptionbuttonWidget = gtk_option_menu_new();
		setup_widget_prototype(gOptionbuttonWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_scrollbar_widget()
{
	if (!gVertScrollbarWidget)
	{
		gVertScrollbarWidget = gtk_vscrollbar_new(NULL);
		setup_widget_prototype(gVertScrollbarWidget);
	}
	if (!gHorizScrollbarWidget)
	{
		gHorizScrollbarWidget = gtk_hscrollbar_new(NULL);
		setup_widget_prototype(gHorizScrollbarWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_entry_widget()
{
	if (!gEntryWidget)
	{
		gEntryWidget = gtk_entry_new();

		setup_widget_prototype(gEntryWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_arrow_widget()
{
	if (!gArrowWidget)
	{
		gDropdownButtonWidget = gtk_button_new();
		setup_widget_prototype(gDropdownButtonWidget);
		gArrowWidget = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
		gtk_container_add(GTK_CONTAINER(gDropdownButtonWidget),
		                    gArrowWidget);
		gtk_widget_set_style(gArrowWidget,NULL);
		gtk_widget_realize(gArrowWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_handlebox_widget()
{
	if (!gHandleBoxWidget)
	{
		gHandleBoxWidget = gtk_handle_box_new();
		setup_widget_prototype(gHandleBoxWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_tooltip_widget()
{
	if (!gTooltipWidget)
	{
		gTooltipWidget = gtk_tooltips_new();

		/* take ownership of the tooltips object */
		g_object_ref(gTooltipWidget);
		gtk_object_sink(GTK_OBJECT(gTooltipWidget));

		gtk_tooltips_force_window(gTooltipWidget);
		gtk_widget_set_style(gTooltipWidget->tip_window, NULL);
		gtk_widget_realize(gTooltipWidget->tip_window);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_tab_widget()
{
	if (!gTabWidget)
	{
		gTabWidget = gtk_notebook_new();
		setup_widget_prototype(gTabWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_label_widget()
{
	if (!gLabelWidget)
	{
		gLabelWidget = gtk_label_new("M");
		setup_widget_prototype(gLabelWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_progress_widget()
{

	if (!gProgressWidget)
	{
		gProgressWidget = gtk_progress_bar_new();
		setup_widget_prototype(gProgressWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static gint ensure_frame_widget()
{
	if (!gFrameWidget)
	{
		gFrameWidget = gtk_frame_new(NULL);
		setup_widget_prototype(gFrameWidget);
	}
	return MOZ_GTK_SUCCESS;
}

static GtkStateType ConvertGtkState(GtkWidgetState * state)
{
	if (state->disabled)
		return GTK_STATE_INSENSITIVE;
	else if (state->inHover)
		return (state->active ? GTK_STATE_ACTIVE : GTK_STATE_PRELIGHT);
	else if(state->active)
		return GTK_STATE_ACTIVE;
	else
		return GTK_STATE_NORMAL;
}

static gint TSOffsetStyleGCArray(GdkGC ** gcs, gint xorigin, gint yorigin)
{
	int i;
	/* there are 5 gc's in each array, for each of the widget states */
	for (i = 0; i < 5; ++i)
		gdk_gc_set_ts_origin(gcs[i], xorigin, yorigin);
	return MOZ_GTK_SUCCESS;
}

static gint TSOffsetStyleGCs(GtkStyle * style, gint xorigin, gint yorigin)
{
	TSOffsetStyleGCArray(style->fg_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->bg_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->light_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->dark_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->mid_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->text_gc, xorigin, yorigin);
	TSOffsetStyleGCArray(style->base_gc, xorigin, yorigin);
	gdk_gc_set_ts_origin(style->black_gc, xorigin, yorigin);
	gdk_gc_set_ts_origin(style->white_gc, xorigin, yorigin);
	return MOZ_GTK_SUCCESS;
}

static int moz_gtk_generic_container_paint(GdkDrawable * drawable,
        GdkRectangle * rect,
        GdkRectangle * cliprect,
        GtkWidgetState * state,
        GtkWidget *widget,
        const gchar *name)
{
	GtkStateType state_type;
	GtkStyle *style = widget->style;
	

	if(state)
		state_type = ConvertGtkState(state);
	else
		state_type = GTK_STATE_NORMAL;

	if (state_type != GTK_STATE_NORMAL
	        && state_type != GTK_STATE_PRELIGHT)
		state_type = GTK_STATE_NORMAL;

	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_flat_box(style, drawable, state_type,
	                     GTK_SHADOW_ETCHED_OUT, cliprect,
	                     widget,
	                     name,
	                     rect->x, rect->y, rect->width,
	                     rect->height);

	return MOZ_GTK_SUCCESS;
}


gint
moz_gtk_widget_get_focus(GtkWidget* widget, gboolean* interior_focus,
                         gint* focus_width, gint* focus_pad) 
{
    gtk_widget_style_get_ptr (widget,
                          "interior-focus", interior_focus,
                          "focus-line-width", focus_width,
                          "focus-padding", focus_pad,
                          NULL);

    return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_button_get_default_overflow(gint* border_top, gint* border_left,
                                    gint* border_bottom, gint* border_right)
{
    GtkBorder* default_outside_border;

    ensure_button_widget();
    gtk_widget_style_get_ptr(gButtonWidget,
                         "default-outside-border", &default_outside_border,
                         NULL);

    if (default_outside_border) {
        *border_top = default_outside_border->top;
        *border_left = default_outside_border->left;
        *border_bottom = default_outside_border->bottom;
        *border_right = default_outside_border->right;
        gtk_border_free(default_outside_border);
    } else {
        *border_top = *border_left = *border_bottom = *border_right = 0;
    }
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_button_get_default_border(gint* border_top, gint* border_left,
                                  gint* border_bottom, gint* border_right)
{
    GtkBorder* default_border;

    ensure_button_widget();
    gtk_widget_style_get_ptr(gButtonWidget,
                         "default-border", &default_border,
                         NULL);

    if (default_border) {
        *border_top = default_border->top;
        *border_left = default_border->left;
        *border_bottom = default_border->bottom;
        *border_right = default_border->right;
        gtk_border_free(default_border);
    } else {
        *border_top = *border_left = *border_bottom = *border_right = 1;
    }
    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_button_paint(GdkDrawable * drawable, GdkRectangle * rect,
                     GdkRectangle * cliprect, GtkWidgetState * state,
                     GtkReliefStyle relief, GtkWidget * widget)
{
    GtkShadowType shadow_type;
    GtkStyle* style = widget->style;
    GtkStateType button_state = ConvertGtkState(state);
    gint x = rect->x, y=rect->y, width=rect->width, height=rect->height;

    gboolean interior_focus;
    gint focus_width, focus_pad;

    moz_gtk_widget_get_focus(widget, &interior_focus, &focus_width, &focus_pad);

    if (WINDOW_IS_MAPPED(drawable)) {
        gdk_window_set_back_pixmap(drawable, NULL, TRUE);
        gdk_window_clear_area(drawable, cliprect->x, cliprect->y,
                              cliprect->width, cliprect->height);
    }

    gtk_widget_set_state(widget, button_state);

    if (state->isDefault)
        GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_DEFAULT);

    GTK_BUTTON(widget)->relief = relief;

    /* Some theme engines are troublesome in that gtk_paint_focus is a
       no-op on buttons and button-like widgets. They only listen to this flag. */
    if (state->focused && !state->disabled)
        GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);

    if (!interior_focus && state->focused) {
        x += focus_width + focus_pad;
        y += focus_width + focus_pad;
        width -= 2 * (focus_width + focus_pad);
        height -= 2 * (focus_width + focus_pad);
    }

    shadow_type = button_state == GTK_STATE_ACTIVE ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
 
    if (state->isDefault && relief == GTK_RELIEF_NORMAL) {
        /* handle default borders both outside and inside the button */
        gint default_top, default_left, default_bottom, default_right;
        moz_gtk_button_get_default_overflow(&default_top, &default_left,
                                            &default_bottom, &default_right);
        x -= default_left;
        y -= default_top;
        width += default_left + default_right;
        height += default_top + default_bottom;
        gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, cliprect,
                      widget, "buttondefault", x, y, width, height);

        moz_gtk_button_get_default_border(&default_top, &default_left,
                                          &default_bottom, &default_right);
        x += default_left;
        y += default_top;
        width -= (default_left + default_right);
        height -= (default_top + default_bottom);
    }
 
    if (relief != GTK_RELIEF_NONE ||
        (button_state != GTK_STATE_NORMAL &&
         button_state != GTK_STATE_INSENSITIVE)) {
        TSOffsetStyleGCs(style, x, y);
        /* the following line can trigger an assertion (Crux theme)
           file ../../gdk/gdkwindow.c: line 1846 (gdk_window_clear_area):
           assertion `GDK_IS_WINDOW (window)' failed */
        gtk_paint_box(style, drawable, button_state, shadow_type, cliprect,
                      widget, "button", x, y, width, height);
    }

    if (state->focused) {
        if (interior_focus) {
            x += widget->style->xthickness + focus_pad;
            y += widget->style->ythickness + focus_pad;
            width -= 2 * (widget->style->xthickness + focus_pad);
            height -= 2 * (widget->style->ythickness + focus_pad);
        } else {
            x -= focus_width + focus_pad;
            y -= focus_width + focus_pad;
            width += 2 * (focus_width + focus_pad);
            height += 2 * (focus_width + focus_pad);
        }

        TSOffsetStyleGCs(style, x, y);
        gtk_paint_focus(style, drawable, button_state, cliprect,
                        widget, "button", x, y, width, height);
    }

    GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_DEFAULT);
    GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);
    return MOZ_GTK_SUCCESS;
}

gint moz_gtk_initDL()
{
	return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_checkbox_get_metrics(gint * indicator_size, gint * indicator_spacing)
{
	ensure_checkbox_widget();

	if (indicator_size)
	{
		gtk_widget_style_get_ptr(gCheckboxWidget, "indicator-size",
		                       indicator_size, NULL);
	}

	if (indicator_spacing)
	{
		gtk_widget_style_get_ptr(gCheckboxWidget, "indicator-spacing",
		                       indicator_spacing, NULL);
	}

	return MOZ_GTK_SUCCESS;
}

gint moz_gtk_radiobutton_get_metrics(gint * indicator_size,
                                     gint * indicator_spacing)
{
	ensure_radiobutton_widget();

	if (indicator_size)
	{
		gtk_widget_style_get_ptr(gRadiobuttonWidget, "indicator-size",
		                       indicator_size, NULL);
	}

	if (indicator_spacing)
	{
		gtk_widget_style_get_ptr(gRadiobuttonWidget, "indicator-spacing",
		                       indicator_spacing, NULL);
	}

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toggle_paint(GdkDrawable * drawable, GdkRectangle * rect,
                     GdkRectangle * cliprect, GtkWidgetState * state,
                     gboolean selected, gboolean isradio)
{
GtkStateType state_type = ConvertGtkState(state);
    GtkShadowType shadow_type = (selected)?GTK_SHADOW_IN:GTK_SHADOW_OUT;
    gint indicator_size, indicator_spacing;
    gint x, y, width, height;
    gint focus_x, focus_y, focus_width, focus_height;
    GtkWidget *w;
    GtkStyle *style;

    if (isradio) {
        moz_gtk_radio_get_metrics(&indicator_size, &indicator_spacing);
        w = gRadiobuttonWidget;
    } else {
        moz_gtk_checkbox_get_metrics(&indicator_size, &indicator_spacing);
        w = gCheckboxWidget;
    }

    /*
     * vertically center in the box, since XUL sometimes ignores our
     * GetMinimumWidgetSize in the vertical dimension
     */
    x = rect->x + indicator_spacing;
    y = rect->y + (rect->height - indicator_size) / 2;
    width = indicator_size;
    height = indicator_size;

    focus_x = x - indicator_spacing;
    focus_y = y - indicator_spacing;
    focus_width = width + 2 * indicator_spacing;
    focus_height = height + 2 * indicator_spacing;
  
    style = w->style;
    TSOffsetStyleGCs(style, x, y);

    gtk_widget_set_sensitive(w, !state->disabled);
    GTK_TOGGLE_BUTTON(w)->active = selected;
      
    if (isradio) {
        gtk_paint_option(style, drawable, state_type, shadow_type, cliprect,
                         gRadiobuttonWidget, "radiobutton", x, y,
                         width, height);
        if (state->focused) {
            gtk_paint_focus(style, drawable, GTK_STATE_ACTIVE, cliprect,
                            gRadiobuttonWidget, "radiobutton", focus_x, focus_y,
                            focus_width, focus_height);
        }
    }
    else {

        gtk_paint_check(style, drawable, state_type, shadow_type, cliprect, 
                        gCheckboxWidget, "checkbutton", x, y, width, height);
        if (state->focused) {
            gtk_paint_focus(style, drawable, GTK_STATE_ACTIVE, cliprect,
                            gCheckboxWidget, "checkbutton", focus_x, focus_y,
                            focus_width, focus_height);
        }
    }

    return MOZ_GTK_SUCCESS;
}

static gint
calculate_arrow_dimensions(GdkRectangle * rect, GdkRectangle * arrow_rect)
{
	GtkMisc *misc = GTK_MISC(gArrowWidget);

	gint extent = MIN(rect->width - misc->xpad * 2,
	                  rect->height - misc->ypad * 2);

	arrow_rect->x = (gint)(
	                    (rect->x + misc->xpad) *
	                    (1.0 - misc->xalign) +
	                    (rect->x + rect->width - extent - misc->xpad) *
	                    misc->xalign);

	arrow_rect->y = (gint)(
	                    (rect->y + misc->ypad) *
	                    (1.0 - misc->yalign) +
	                    (rect->y + rect->height - extent - misc->ypad) *
	                    misc->yalign);

	arrow_rect->width = arrow_rect->height = extent;

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_scrollbar_button_paint(GdkDrawable * drawable, GdkRectangle * rect,
                               GdkRectangle * cliprect,
                               GtkWidgetState * state, GtkArrowType type)
{
	GtkStateType state_type = ConvertGtkState(state);
	GtkShadowType shadow_type = (state->active) ?
	                            GTK_SHADOW_IN : GTK_SHADOW_OUT;
	GdkRectangle button_rect;
	GdkRectangle arrow_rect;
	GtkStyle *style;
	GtkScrollbar *scrollbar;

	ensure_scrollbar_widget();

	if (type < 2)
		scrollbar = GTK_SCROLLBAR(gVertScrollbarWidget);
	else
		scrollbar = GTK_SCROLLBAR(gHorizScrollbarWidget);


	style = GTK_WIDGET(scrollbar)->style;

	ensure_arrow_widget();


	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_box(style, drawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN,
	                cliprect, GTK_WIDGET(scrollbar), "trough",
	                rect->x,
	                rect->y,
	                rect->width,
	                rect->height);

	calculate_arrow_dimensions(rect, &button_rect);
	TSOffsetStyleGCs(style, button_rect.x, button_rect.y);

	gtk_paint_box(style, drawable, state_type, shadow_type, cliprect,
	                GTK_WIDGET(scrollbar),
	                (type < 2) ? "vscrollbar" : "hscrollbar",
	                button_rect.x, button_rect.y, button_rect.width,
	                button_rect.height);

	arrow_rect.width = button_rect.width / 2;
	arrow_rect.height = button_rect.height / 2;
	arrow_rect.x =
	    button_rect.x + (button_rect.width - arrow_rect.width) / 2;
	arrow_rect.y =
	    button_rect.y + (button_rect.height - arrow_rect.height) / 2;

	gtk_paint_arrow(style, drawable, state_type, shadow_type, cliprect,
	                  GTK_WIDGET(scrollbar), (type < 2) ?
	                  "vscrollbar" : "hscrollbar",
	                  type, TRUE, arrow_rect.x, arrow_rect.y,
	                  arrow_rect.width, arrow_rect.height);

	return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_scrollbar_trough_paint(GtkThemeWidgetType widget,
                               GdkDrawable * drawable, GdkRectangle * rect,
                               GdkRectangle * cliprect,
                               GtkWidgetState * state)
{

	GtkStyle *style;
	GtkScrollbar *scrollbar;

	ensure_scrollbar_widget();

	if (widget == MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL)
		scrollbar = GTK_SCROLLBAR(gHorizScrollbarWidget);
	else
		scrollbar = GTK_SCROLLBAR(gVertScrollbarWidget);



	style = GTK_WIDGET(scrollbar)->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_box(style, drawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN,
	                cliprect, GTK_WIDGET(scrollbar), "trough",
	                rect->x,
	                rect->y,
	                rect->width,
	                rect->height);

	if (state->focused)
	{
		gtk_paint_focus(style, drawable, GTK_STATE_ACTIVE, cliprect,
		                  GTK_WIDGET(scrollbar), "trough",
		                  rect->x, rect->y, rect->width, rect->height);
	}

	return MOZ_GTK_SUCCESS;
}

static gint

moz_gtk_scrollbar_thumb_paint(GtkThemeWidgetType widget,
                              GdkDrawable * drawable, GdkRectangle * rect,
                              GdkRectangle * cliprect, GtkWidgetState * state)
{
	GtkStateType state_type = (state->inHover || state->active) ?
	                          GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;
	GtkStyle *style;
	GtkScrollbar *scrollbar;
	GtkAdjustment *adj;

	ensure_scrollbar_widget();

	if (widget == MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL)
		scrollbar = GTK_SCROLLBAR(gHorizScrollbarWidget);
	else
		scrollbar = GTK_SCROLLBAR(gVertScrollbarWidget);
	style = GTK_WIDGET(scrollbar)->style;

	adj = gtk_range_get_adjustment(GTK_RANGE(scrollbar));

	TSOffsetStyleGCs(style, rect->x, rect->y);

	int thumbborder = 1;

	if (widget == MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL)
	{
		rect->x += thumbborder;
		rect->width -= thumbborder * 2;
		adj->page_size = rect->width - 2;
	}
	else
	{
		rect->y += thumbborder;
		rect->height -= thumbborder * 2;
		adj->page_size = rect->height - 2;
	}


	adj->lower = 0;
	adj->value = state->curpos;
	adj->upper = state->maxpos;
	gtk_adjustment_changed(adj);


	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_slider(style, drawable, state_type, GTK_SHADOW_OUT,
	                   cliprect, GTK_WIDGET(scrollbar), "slider", rect->x,
	                   rect->y, rect->width, rect->height,
	                   (widget ==
	                    MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL) ?
	                   GTK_ORIENTATION_HORIZONTAL :
	                   GTK_ORIENTATION_VERTICAL);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_gripper_paint(GdkDrawable * drawable, GdkRectangle * rect,
                      GdkRectangle * cliprect, GtkWidgetState * state)
{
	GtkStateType state_type = ConvertGtkState(state);
	GtkShadowType shadow_type;
	GtkStyle *style;

	ensure_handlebox_widget();
	style = gHandleBoxWidget->style;
	shadow_type = GTK_HANDLE_BOX(gHandleBoxWidget)->shadow_type;

	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_box(style, drawable, state_type, shadow_type, cliprect,
	                gHandleBoxWidget, "handlebox_bin", rect->x, rect->y,
	                rect->width, rect->height);


	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_entry_frame_paint(GdkDrawable * drawable, GdkRectangle * rect,
                          GdkRectangle * cliprect, GtkWidgetState * state)
{
    GtkStateType bg_state = state->disabled ?
                                GTK_STATE_INSENSITIVE : GTK_STATE_NORMAL;
    gint x, y, width = rect->width, height = rect->height;
    GtkStyle* style;
    gboolean interior_focus;
    gboolean theme_honors_transparency = FALSE;
    gint focus_width;

	GtkWidget *widget;
	ensure_entry_widget();
	widget = gEntryWidget;

    style = widget->style;

    gtk_widget_style_get_ptr(widget,
                         "interior-focus", &interior_focus,
                         "focus-line-width", &focus_width,
                         NULL);

    /* gtkentry.c uses two windows, one for the entire widget and one for the
     * text area inside it. The background of both windows is set to the "base"
     * color of the new state in gtk_entry_state_changed, but only the inner
     * textarea window uses gtk_paint_flat_box when exposed */

    TSOffsetStyleGCs(style, rect->x, rect->y);

    /* This gets us a lovely greyish disabledish look */
    gtk_widget_set_sensitive(widget, !state->disabled);

    /* GTK fills the outer widget window with the base color before drawing the widget.
     * Some older themes rely on this behavior, but many themes nowadays use rounded
     * corners on their widgets. While most GTK apps are blissfully unaware of this
     * problem due to their use of the default window background, we render widgets on
     * many kinds of backgrounds on the web.
     * If the theme is able to cope with transparency, then we can skip pre-filling
     * and notify the theme it will paint directly on the canvas. */

    /* Get the position of the inner window, see _gtk_entry_get_borders */
    x = XTHICKNESS(style);
    y = YTHICKNESS(style);

    if (!interior_focus) {
        x += focus_width;
        y += focus_width;
    }

    /* Now paint the shadow and focus border.
     * We do like in gtk_entry_draw_frame, we first draw the shadow, a tad
     * smaller when focused if the focus is not interior, then the focus. */
    x = rect->x;
    y = rect->y;

    if (state->focused && !state->disabled) {
        /* This will get us the lit borders that focused textboxes enjoy on
         * some themes. */
        GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);

        if (!interior_focus) {
            /* Indent the border a little bit if we have exterior focus 
               (this is what GTK does to draw native entries) */
            x += focus_width;
            y += focus_width;
            width -= 2 * focus_width;
            height -= 2 * focus_width;
        }
    }

    gtk_paint_shadow(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                     cliprect, widget, "entry", x, y, width, height);

    if (state->focused && !state->disabled) {
        if (!interior_focus) {
            gtk_paint_focus(style, drawable,  GTK_STATE_NORMAL, cliprect,
                            widget, "entry",
                            rect->x, rect->y, rect->width, rect->height);
        }

        /* Now unset the focus flag. We don't want other entries to look
         * like they're focused too! */
        GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);
    }

    return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_entry_paint(GdkDrawable * drawable, GdkRectangle * rect,
                    GdkRectangle * cliprect, GtkWidgetState * state)
{
	gint x, y;
	GtkStyle *style;

	ensure_entry_widget();
	style = gEntryWidget->style;

	moz_gtk_generic_container_paint(drawable, rect, cliprect, state,
	                                gEntryWidget, "viewportbin");

	/* paint the background first */
	x = XTHICKNESS(style);
	y = YTHICKNESS(style);

	TSOffsetStyleGCs(style, rect->x + x, rect->y + y);
	gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
	                     cliprect, gEntryWidget, "entry_bg", rect->x + x,
	                     rect->y + y, rect->width - 2 * x,
	                     rect->height - 2 * y);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_dropdown_arrow_paint(GdkDrawable * drawable, GdkRectangle * rect,
                             GdkRectangle * cliprect, GtkWidgetState * state)
{
	GdkRectangle arrow_rect, real_arrow_rect;
	GtkStateType state_type = ConvertGtkState(state);
	GtkShadowType shadow_type =
	    state->active ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
	GtkStyle *style;

	ensure_arrow_widget();
	moz_gtk_button_paint(drawable, rect, cliprect, state,
	                     GTK_RELIEF_NORMAL, gDropdownButtonWidget);

	/* This mirrors gtkbutton's child positioning */
	style = gDropdownButtonWidget->style;
	arrow_rect.x = rect->x + 1 + XTHICKNESS(gDropdownButtonWidget->style);
	arrow_rect.y = rect->y + 1 + YTHICKNESS(gDropdownButtonWidget->style);
	arrow_rect.width = MAX(1, rect->width - (arrow_rect.x - rect->x) * 2);
	arrow_rect.height =
	    MAX(1, rect->height - (arrow_rect.y - rect->y) * 2);

	calculate_arrow_dimensions(&arrow_rect, &real_arrow_rect);
	style = gArrowWidget->style;
	TSOffsetStyleGCs(style, real_arrow_rect.x, real_arrow_rect.y);

	real_arrow_rect.width = real_arrow_rect.height = (int)
	                        (MIN(real_arrow_rect.width, real_arrow_rect.height) * 0.9);

	real_arrow_rect.x = (gint)
	                    floor(arrow_rect.x +
	                          ((arrow_rect.width - real_arrow_rect.width) / 2) + 0.5);
	real_arrow_rect.y = (gint)
	                    floor(arrow_rect.y +
	                          ((arrow_rect.height - real_arrow_rect.height) / 2) +
	                          0.5);

	gtk_paint_arrow(style, drawable, state_type, shadow_type, cliprect,
	                  gHorizScrollbarWidget, "arrow", GTK_ARROW_DOWN, TRUE,
	                  real_arrow_rect.x, real_arrow_rect.y,
	                  real_arrow_rect.width, real_arrow_rect.height);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_container_paint(GdkDrawable * drawable, GdkRectangle * rect,
                        GdkRectangle * cliprect, GtkWidgetState * state,
                        gboolean isradio)
{
	GtkStateType state_type = ConvertGtkState(state);
	GtkStyle *style;

	if (isradio)
	{
		ensure_radiobutton_widget();
		style = gRadiobuttonWidget->style;
	}
	else
	{
		ensure_checkbox_widget();
		style = gCheckboxWidget->style;
	}

	if (state_type != GTK_STATE_NORMAL
	        && state_type != GTK_STATE_PRELIGHT)
		state_type = GTK_STATE_NORMAL;

	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_flat_box(style, drawable, state_type,
	                     GTK_SHADOW_ETCHED_OUT, cliprect,
	                     (isradio ? gRadiobuttonWidget : gCheckboxWidget),
	                     isradio ? "radiobutton" : "checkbutton",
	                     rect->x, rect->y, rect->width,
	                     rect->height);


	if (state->focused)
	{
		gtk_paint_focus(style, drawable, state_type, cliprect,
		                  (isradio ? gRadiobuttonWidget : gCheckboxWidget),
		                  isradio ? "radiobutton" : "checkbutton",
		                  rect->x, rect->y, rect->width, rect->height);
	}

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_toolbar_paint(GdkDrawable * drawable, GdkRectangle * rect,
                      GdkRectangle * cliprect)
{
	GtkStyle *style;

	ensure_handlebox_widget();
	style = gHandleBoxWidget->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);

	if (style->bg_pixmap[GTK_STATE_NORMAL])
	{
		gtk_style_apply_default_background(style, drawable, TRUE,
		                                     GTK_STATE_NORMAL,
		                                     cliprect, rect->x, rect->y,
		                                     rect->width, rect->height);
	}
	else
	{
		gtk_paint_box(style, drawable, GTK_STATE_NORMAL,
		                GTK_SHADOW_OUT, cliprect, gHandleBoxWidget,
		                "dockitem_bin", rect->x, rect->y, rect->width,
		                rect->height);
	}

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tooltip_paint(GdkDrawable * drawable, GdkRectangle * rect,
                      GdkRectangle * cliprect)
{
	GtkStyle *style;

	ensure_tooltip_widget();
	style = gTooltipWidget->tip_window->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
	                     cliprect, gTooltipWidget->tip_window, "tooltip",
	                     rect->x, rect->y, rect->width, rect->height);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_frame_paint(GdkDrawable * drawable, GdkRectangle * rect,
                    GdkRectangle * cliprect)
{
	GtkStyle *style = gProtoWindow->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
	                     NULL, gProtoWindow, "base", rect->x, rect->y,
	                     rect->width, rect->height);

	ensure_frame_widget();
	style = gFrameWidget->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_shadow(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
	                   cliprect, gFrameWidget, "frame", rect->x, rect->y,
	                   rect->width, rect->height);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_listbox_paint(GdkDrawable * drawable, GdkRectangle * rect,
                      GdkRectangle * cliprect)
{

	int xw, yw;
	ensure_entry_widget();
	GtkStyle *style = gEntryWidget->style;

	moz_gtk_get_widget_border(MOZ_GTK_FRAME, &xw, &yw);

	moz_gtk_frame_paint(drawable, rect, cliprect);

	rect->x += xw;
	rect->y += yw;
	rect->width -= (xw * 2);
	rect->height -= (yw * 2);

	gdk_draw_rectangle(drawable, style->white_gc, TRUE,
	                     rect->x, rect->y,
	                     rect->width, rect->height);

	return MOZ_GTK_SUCCESS;
}

void spinbutton_get_rects(GtkArrowType type, GdkRectangle *rect,
                          GdkRectangle &buttonrect, GdkRectangle &arrowrect)
{
	gint arrow_size;
	int x, y, width, height;
	int h, w;

	ensure_spinbutton_widget();

	arrow_size = rect->width - (2 * XTHICKNESS(gSpinbuttonWidget->style));

	width = arrow_size + 2 * gSpinbuttonWidget->style->xthickness;

	if(type == GTK_ARROW_UP)
	{
		x = rect->x;
		y = rect->y;
		height = (int)floor(rect->height / 2);

	}
	else
	{
		x = rect->x;
		y = rect->y + (int)floor(rect->height / 2);
		height = (rect->height + 1) / 2;
	}

	buttonrect.x = x;
	buttonrect.y = y;
	buttonrect.width = width;
	buttonrect.height = height;


	////
	height = rect->height;

	if (type == GTK_ARROW_DOWN)
	{
		y = height / 2;
		height = height - y - 2;
	}
	else
	{
		y = 2;
		height = height / 2 - 2;
	}

	width -= 3;

	x = 1;

	w = width / 2;
	w -= w % 2 - 1; /* force odd */
	h = (w + 1) / 2;

	x += (width - w) / 2;
	y += (height - h) / 2;

	arrowrect.x = x;
	arrowrect.y = y;
	arrowrect.width = w;
	arrowrect.height = h;
}

static void
spinbutton_arrow_paint(GtkArrowType type, GdkRectangle *rect, GdkDrawable *d,
                       GtkStateType state_type, GtkShadowType shadow_type)
{
	GdkRectangle buttonrect, arrowrect;

	spinbutton_get_rects(type, rect, buttonrect, arrowrect);

	gtk_paint_box(gSpinbuttonWidget->style, d,
	                state_type, shadow_type,
	                NULL, gSpinbuttonWidget,
	                (type == GTK_ARROW_UP) ? "spinbutton_up" : "spinbutton_down",
	                buttonrect.x, buttonrect.y, buttonrect.width, buttonrect.height);


	gtk_paint_arrow (gSpinbuttonWidget->style, d,
	                   state_type, shadow_type,
	                   NULL, gSpinbuttonWidget, "spinbutton",
	                   type, TRUE,
	                   arrowrect.x, arrowrect.y, arrowrect.width, arrowrect.height);


}

static gint
moz_gtk_scale_track_paint(GtkThemeWidgetType type,
                          GdkDrawable * drawable,
                          GdkRectangle * rect,
                          GdkRectangle * cliprect,
                          gint flags)
{
	ensure_scale_widget();

	GtkStyle *style;
	GtkScale *scale;

	if (type == MOZ_GTK_SCALE_TRACK_HORIZONTAL)
		scale = GTK_SCALE(gHScaleWidget);
	else
		scale = GTK_SCALE(gVScaleWidget);


	style = GTK_WIDGET(scale)->style;

    // AL-2014-01-16: [[ Bug 11656 ]] Don't paint box around slider trough.
	// moz_gtk_label_paint(drawable, rect, cliprect);

	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_box(style, drawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN,
	                cliprect, GTK_WIDGET(scale), "trough",
	                rect->x,
	                rect->y,
	                rect->width,
	                rect->height);
	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_menuitem_paint(GdkDrawable * drawable, GdkRectangle * rect,
                       GdkRectangle * cliprect)
{
	GtkWidget *widget;
	GtkShadowType selected_shadow_type;

	ensure_menuitem_widget();

	widget = gMenuitemWidget;

	gtk_widget_style_get_ptr (widget,
	                        "selected_shadow_type", &selected_shadow_type,
	                        NULL);

	gtk_paint_box (widget->style,
	                 drawable,
	                 GTK_STATE_PRELIGHT,
	                 selected_shadow_type,
	                 cliprect, widget, "menuitem",
	                 rect->x, rect->y, rect->width, rect->height);


	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_spinbutton_paint(GdkDrawable * drawable, GdkRectangle * rect,
                         GdkRectangle * cliprect, GtkWidgetState *state,
                         gint flags)
{
	ensure_spinbutton_widget();

	GtkShadowType shadow_type;
	int xw, yw;
	GtkStyle *style = gSpinbuttonWidget->style;

	gtk_widget_style_get_ptr (GTK_WIDGET (gSpinbuttonWidget),
	                        "shadow_type", &shadow_type, NULL);

	moz_gtk_get_widget_border(MOZ_GTK_FRAME, &xw, &yw);


	if(shadow_type != GTK_SHADOW_NONE)
	{
		gtk_paint_box (style, drawable,
		                 GTK_STATE_NORMAL, shadow_type, NULL, gSpinbuttonWidget,
		                 "spinbutton",
		                 rect->x, rect->y,
		                 rect->width, rect->height);
	}

	shadow_type = GTK_SHADOW_OUT;
	GtkStateType state_type = GTK_STATE_NORMAL;

	if(state->active)
	{
		shadow_type = GTK_SHADOW_IN;
	}

	if(flags == GTK_POS_TOP)
	{
		state_type = ConvertGtkState(state);
		spinbutton_arrow_paint(GTK_ARROW_UP, rect, drawable,
		                       state_type, shadow_type);
	}
	else
		spinbutton_arrow_paint(GTK_ARROW_UP, rect, drawable,
		                       GTK_STATE_NORMAL, GTK_SHADOW_OUT);


	if(flags == GTK_POS_BOTTOM)
	{
		state_type = ConvertGtkState(state);
		spinbutton_arrow_paint(GTK_ARROW_DOWN, rect, drawable,
		                       state_type, shadow_type);

	}
	else
		spinbutton_arrow_paint(GTK_ARROW_DOWN, rect, drawable,
		                       GTK_STATE_NORMAL, GTK_SHADOW_OUT);


	return MOZ_GTK_SUCCESS;
}


static gint
moz_gtk_progressbar_paint(GdkDrawable * drawable, GdkRectangle * rect,
                          GdkRectangle * cliprect)
{
	GtkStyle *style;

	ensure_progress_widget();
	style = gProgressWidget->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
	                cliprect, gProgressWidget, "trough", rect->x, rect->y,
	                rect->width, rect->height);

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_progress_chunk_paint(GdkDrawable * drawable, GdkRectangle * rect,
                             GdkRectangle * cliprect, gint flags)
{
	GtkStyle *style;

	ensure_progress_widget();
	style = gProgressWidget->style;

	GtkProgressBarOrientation orientation;
	

	if(flags & GTK_PROGRESS_TOP_TO_BOTTOM)
		orientation = GTK_PROGRESS_TOP_TO_BOTTOM;
	else if(flags & GTK_PROGRESS_BOTTOM_TO_TOP)
		orientation = GTK_PROGRESS_BOTTOM_TO_TOP;
	else if(flags & GTK_PROGRESS_RIGHT_TO_LEFT)
		orientation = GTK_PROGRESS_RIGHT_TO_LEFT;
	else
		orientation = GTK_PROGRESS_LEFT_TO_RIGHT;


	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(gProgressWidget), orientation);


	int border = style->xthickness;

	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_IN,
	                cliprect, gProgressWidget, "trough",
	                rect->x - border, rect->y - border,
	                rect->width + border, rect->height + border);

	gtk_paint_box(style, drawable, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT,
	                NULL, gProgressWidget, "bar", rect->x, rect->y,
	                rect->width, rect->height);


	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_label_paint(GdkDrawable * drawable, GdkRectangle * rect,
                    GdkRectangle * cliprect)
{
	GtkStyle *style;

	ensure_label_widget();
	style = gLabelWidget->style;
	
	TSOffsetStyleGCs(style, rect->x, rect->y);
	gtk_paint_flat_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
	                     cliprect, gLabelWidget, "button", rect->x, rect->y,
	                     rect->width, rect->height);

	return MOZ_GTK_SUCCESS;
}

static void
gtk_option_menu_get_props (GtkOptionMenu       *option_menu,
                           GtkOptionMenuProps  *props)
{
	GtkRequisition *indicator_size;
	GtkBorder *indicator_spacing;

	gtk_widget_style_get_ptr(GTK_WIDGET (option_menu),
	                        "indicator_size", &indicator_size,
	                        "indicator_spacing", &indicator_spacing,
	                        "interior_focus", &props->interior_focus,
	                        "focus_line_width", &props->focus_width,
	                        "focus_padding", &props->focus_pad,
	                        NULL);

	if (indicator_size)
		props->indicator_size = *indicator_size;
	else
		props->indicator_size = default_props.indicator_size;

	if (indicator_spacing)
		props->indicator_spacing = *indicator_spacing;
	else
		props->indicator_spacing = default_props.indicator_spacing;

	gtk_requisition_free (indicator_size);
	gtk_border_free (indicator_spacing);
}

static gint
moz_gtk_optionbutton_paint(GdkDrawable * drawable, GdkRectangle * area,
                           GdkRectangle * cliprect, GtkWidgetState *state)
{
	ensure_optionbutton_widget();
	GtkStateType state_type;

	if(state)
		state_type = ConvertGtkState(state);
	else
		state_type = GTK_STATE_NORMAL;

	GdkRectangle button_area;
	GtkOptionMenuProps props;
	GtkWidget *widget = gOptionbuttonWidget;
	gint border_width;
	gint tab_x;



	border_width = GTK_CONTAINER (widget)->border_width;
	gtk_option_menu_get_props (GTK_OPTION_MENU (widget), &props);

	widget->allocation.x = cliprect->x;
	widget->allocation.y = cliprect->y;
	widget->allocation.width = cliprect->width;
	widget->allocation.height = cliprect->height;

	button_area.x = widget->allocation.x + border_width;

	button_area.y = widget->allocation.y + border_width;
	button_area.width = widget->allocation.width - 2 * border_width;
	button_area.height = widget->allocation.height - 2 * border_width;

	if (!props.interior_focus && GTK_WIDGET_HAS_FOCUS (widget))
	{
		button_area.x += props.focus_width + props.focus_pad;
		button_area.y += props.focus_width + props.focus_pad;
		button_area.width -= 2 * (props.focus_width + props.focus_pad);
		button_area.height -= 2 * (props.focus_width + props.focus_pad);
	}


	gtk_paint_box(widget->style, drawable,
	                 state_type, GTK_SHADOW_OUT,
	                 area, widget, "optionmenu",
	                 button_area.x, button_area.y,
	                 button_area.width, button_area.height);


	tab_x = button_area.x + button_area.width -
	        props.indicator_size.width - props.indicator_spacing.right -
	        widget->style->xthickness;

	gtk_paint_tab(widget->style, drawable,
	                 state_type, GTK_SHADOW_OUT,
	                 area, widget, "optionmenutab",
	                 tab_x,
	                 button_area.y + (button_area.height - props.indicator_size.height) / 2,
	                 props.indicator_size.width, props.indicator_size.height);

	if(state->focused)
	{
		if (props.interior_focus)
		{
			button_area.x += widget->style->xthickness + props.focus_pad;
			button_area.y += widget->style->ythickness + props.focus_pad;
			button_area.width -= 2 * (widget->style->xthickness + props.focus_pad) +
			                     props.indicator_spacing.left +
			                     props.indicator_spacing.right +
			                     props.indicator_size.width;
			button_area.height -= 2 * (widget->style->ythickness + props.focus_pad);
		}
		else
		{
			button_area.x -= props.focus_width + props.focus_pad;
			button_area.y -= props.focus_width + props.focus_pad;
			button_area.width += 2 * (props.focus_width + props.focus_pad);
			button_area.height += 2 * (props.focus_width + props.focus_pad);
		}

		gtk_paint_focus(widget->style, drawable, state_type,
		                   area, widget, "button",
		                   button_area.x,
		                   button_area.y,
		                   button_area.width,
		                   button_area.height);


	}

	return MOZ_GTK_SUCCESS;
}

static gint
moz_gtk_tab_paint(GdkDrawable * drawable, GdkRectangle * rect,
                  GdkRectangle * cliprect, gint flags)
{
	/*
	 * In order to get the correct shadows and highlights, GTK paints
	 * tabs right-to-left (end-to-beginning, to be generic), leaving
	 * out the active tab, and then paints the current tab once
	 * everything else is painted.  In addition, GTK uses a 2-pixel
	 * overlap between adjacent tabs (this value is hard-coded in
	 * gtknotebook.c).  For purposes of mapping to gecko's frame
	 * positions, we put this overlap on the far edge of the frame
	 * (i.e., for a horizontal/top tab strip, we shift the left side
	 * of each tab 2px to the left, into the neighboring tab's frame
	 * rect.  The right 2px * of a tab's frame will be referred to as
	 * the "overlap area".
	 *
	 * Since we can't guarantee painting order with gecko, we need to
	 * manage the overlap area manually. There are three types of tab
	 * boundaries we need to handle:
	 *
	 * * two non-active tabs: In this case, we just have both tabs
	 *   paint normally.
	 *
	 * * non-active to active tab: Here, we need the tab on the left to paint
	 *                             itself normally, then paint the edge of the
	 *                             active tab in its overlap area.
	 *
	 * * active to non-active tab: In this case, we just have both tabs paint
	 *                             normally.
	 *
	 * We need to make an exception for the first tab - since there is

	 * no tab to the left to paint the overlap area, we do _not_ shift
	 * the tab left by 2px.
	 */

	GtkStyle *style;
	ensure_tab_widget();

	style = gTabWidget->style;
	TSOffsetStyleGCs(style, rect->x, rect->y);

	gtk_paint_box(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
	                cliprect, gTabWidget, "notebook",
	                rect->x, rect->y,
	                rect->width, rect->height);


	if (!(flags & MOZ_GTK_TAB_FIRST))
	{
		rect->x -= 2;
		rect->width += 2;
	}

	GtkPositionType t;

	if(flags & MOZ_GTK_TAB_POS_BOTTOM)
		t = GTK_POS_TOP;
	else if(flags & MOZ_GTK_TAB_POS_LEFT)
		t = GTK_POS_RIGHT;
	else if(flags & MOZ_GTK_TAB_POS_RIGHT)
		t = GTK_POS_LEFT;
	else
		t = GTK_POS_BOTTOM;

	gtk_paint_extension(style, drawable,
	                      ((flags & MOZ_GTK_TAB_SELECTED)
	                       ? GTK_STATE_NORMAL
	                       : GTK_STATE_ACTIVE),
	                      GTK_SHADOW_OUT, cliprect, gTabWidget, "tab",
	                      rect->x, rect->y, rect->width, rect->height,
	                      t);

	return MOZ_GTK_SUCCESS;
}


void moz_gtk_get_widget_color(GtkStateType state,
                              uint2 &red,uint2 &green,uint2 &blue)
{
	ensure_label_widget();
	GtkStyle *style = gProtoWindow->style;
	GdkColor c = style->bg[state];
	red = c.red;
	blue = c.blue;
	green = c.green;
}

static gint
moz_gtk_tabpanels_paint(GdkDrawable * drawable, GdkRectangle * rect,
                        GdkRectangle * cliprect, int y, int w)
{
	GtkStyle *style;

	ensure_tab_widget();
	style = gTabWidget->style;

	TSOffsetStyleGCs(style, rect->x, rect->y);


	gtk_paint_box_gap(style, drawable, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
	                    cliprect, gTabWidget, "notebook",
	                    rect->x, rect->y,
	                    rect->width, rect->height,
	                    GTK_POS_TOP,
	                    y, w);

	return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_widget_border(GtkThemeWidgetType widget, gint * xthickness,
                          gint * ythickness)
{
	GtkWidget *w;
	switch (widget)
	{
	case MOZ_GTK_BUTTON:
		ensure_button_widget();
		w = gButtonWidget;
		break;
	case MOZ_GTK_TOOLBAR:
		ensure_handlebox_widget();
		w = gHandleBoxWidget;
		break;
	case MOZ_GTK_ENTRY:
		ensure_entry_widget();
		w = gEntryWidget;
		break;
	case MOZ_GTK_DROPDOWN_ARROW:
		ensure_arrow_widget();
		w = gDropdownButtonWidget;
		break;
	case MOZ_GTK_TABPANELS:
		ensure_tab_widget();
		w = gTabWidget;
		break;
	case MOZ_GTK_PROGRESSBAR:
		ensure_progress_widget();
		w = gProgressWidget;
		break;
	case MOZ_GTK_FRAME:
		ensure_frame_widget();
		w = gFrameWidget;
		break;
	case MOZ_GTK_CHECKBUTTON_CONTAINER:
	case MOZ_GTK_RADIOBUTTON_CONTAINER:
		/* This is a hardcoded value. */
		if (xthickness)
			*xthickness = 1;
		if (ythickness)
			*ythickness = 1;
		return MOZ_GTK_SUCCESS;
		break;
	case MOZ_GTK_CHECKBUTTON:
	case MOZ_GTK_RADIOBUTTON:
	case MOZ_GTK_MENUITEMHIGHLIGHT:
	case MOZ_GTK_SCROLLBAR_BUTTON:
	case MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL:
	case MOZ_GTK_SCROLLBAR_TRACK_VERTICAL:
	case MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL:
	case MOZ_GTK_SCROLLBAR_THUMB_VERTICAL:

	case MOZ_GTK_GRIPPER:
	case MOZ_GTK_TOOLTIP:
	case MOZ_GTK_LABEL:
	case MOZ_GTK_PROGRESS_CHUNK:
	case MOZ_GTK_TAB:
		/* These widgets have no borders, since they are not containers. */
		if (xthickness)
			*xthickness = 0;
		if (ythickness)
			*ythickness = 0;
		return MOZ_GTK_SUCCESS;
	default:
		//g_warning("Unsupported widget type: %d", widget);
		return MOZ_GTK_UNKNOWN_WIDGET;
	}

	if (xthickness)
		*xthickness = XTHICKNESS(w->style);
	if (ythickness)
		*ythickness = YTHICKNESS(w->style);

	return MOZ_GTK_SUCCESS;
}

gint moz_gtk_get_dropdown_arrow_size(gint * width, gint * height)
{
	ensure_arrow_widget();

	/*
	 * First get the border of the dropdown arrow, then add in the requested
	 * size of the arrow.  Note that the minimum arrow size is fixed at
	 * 11 pixels.
	 */

	if (width)
	{
		*width = 2 * (1 + XTHICKNESS(gDropdownButtonWidget->style));
		*width += 11 + GTK_MISC(gArrowWidget)->xpad * 2;
	}
	if (height)
	{
		*height = 2 * (1 + YTHICKNESS(gDropdownButtonWidget->style));
		*height += 11 + GTK_MISC(gArrowWidget)->ypad * 2;
	}

	return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_get_slider_metrics(gint * slider_width, gint * trough_border,
                           gint * stepper_size, gint * stepper_spacing,
                           gint * min_slider_size, gint *focus_line_width,
                           gint * focus_padding)
{
	ensure_scale_widget();

	if(focus_line_width)
		gtk_widget_style_get_ptr (GTK_WIDGET(gHScaleWidget),
		                        "focus-line-width", &focus_line_width, NULL);

	if(focus_padding)
		gtk_widget_style_get_ptr (GTK_WIDGET(gHScaleWidget),
		                        "focus-padding", &focus_padding,
		                        NULL);


	if (slider_width)
	{
		gtk_widget_style_get_ptr(gHScaleWidget, "slider_width",
		                       slider_width, NULL);
	}

	if (trough_border)
	{
		gtk_widget_style_get_ptr(gHScaleWidget, "trough_border",
		                       trough_border, NULL);
	}

	if (stepper_size)
	{
		gtk_widget_style_get_ptr(gHScaleWidget, "stepper_size",
		                       stepper_size, NULL);
	}

	if (stepper_spacing)
	{
		gtk_widget_style_get_ptr(gHScaleWidget, "stepper_spacing",
		                       stepper_spacing, NULL);
	}

	if (min_slider_size)
	{
		*min_slider_size =
		    GTK_RANGE(gHScaleWidget)->min_slider_size;
	}

	return MOZ_GTK_SUCCESS;
}


gint
moz_gtk_get_scrollbar_metrics(gint * slider_width, gint * trough_border,
                              gint * stepper_size, gint * stepper_spacing,
                              gint * min_slider_size, gint *focus_line_width,
                              gint * focus_padding)
{
	ensure_scrollbar_widget();

	if(focus_line_width)
		gtk_widget_style_get_ptr(GTK_WIDGET(gHorizScrollbarWidget),
		                        "focus-line-width", &focus_line_width, NULL);

	if(focus_padding)
		gtk_widget_style_get_ptr(GTK_WIDGET(gHorizScrollbarWidget),
		                        "focus-padding", &focus_padding,
		                        NULL);


	if (slider_width)
	{
		gtk_widget_style_get_ptr(gHorizScrollbarWidget, "slider_width",
		                       slider_width, NULL);
	}

	if (trough_border)
	{
		gtk_widget_style_get_ptr(gHorizScrollbarWidget, "trough_border",
		                       trough_border, NULL);
	}

	if (stepper_size)
	{
		gtk_widget_style_get_ptr(gHorizScrollbarWidget, "stepper_size",
		                       stepper_size, NULL);
	}

	if (stepper_spacing)
	{
		gtk_widget_style_get_ptr(gHorizScrollbarWidget, "stepper_spacing",
		                       stepper_spacing, NULL);
	}

	if (min_slider_size)
	{
		*min_slider_size =
		    GTK_RANGE(gHorizScrollbarWidget)->min_slider_size;
	}

	return MOZ_GTK_SUCCESS;
}

gint
moz_gtk_widget_paint(GtkThemeWidgetType widget, GdkDrawable * drawable,
                     GdkRectangle * rect, GdkRectangle * cliprect,
                     GtkWidgetState * state, gint flags)
{
	switch (widget)
	{
	case MOZ_GTK_LABEL:
		ensure_label_widget();
		return moz_gtk_label_paint(drawable, rect, cliprect);
		break;
	case MOZ_GTK_BUTTON:
		ensure_button_widget();
		return moz_gtk_button_paint(drawable, rect, cliprect,
		                            state,
		                            (GtkReliefStyle) flags,
		                            gButtonWidget);
		break;
	case MOZ_GTK_CHECKBUTTON:
	case MOZ_GTK_RADIOBUTTON:
		ensure_radiobutton_widget();
		ensure_checkbox_widget();

		return moz_gtk_toggle_paint(drawable, rect, cliprect,
		                            state, (gboolean) flags,
		                            (widget ==
		                             MOZ_GTK_RADIOBUTTON));
		break;
	case MOZ_GTK_SCROLLBAR_BUTTON:
		return moz_gtk_scrollbar_button_paint(drawable, rect,
		                                      cliprect, state,
		                                      (GtkArrowType)
		                                      flags);
		break;
	case MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL:
	case MOZ_GTK_SCROLLBAR_TRACK_VERTICAL:
		return moz_gtk_scrollbar_trough_paint(widget,
		                                      drawable, rect,
		                                      cliprect,
		                                      state);
		break;
	case MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL:
	case MOZ_GTK_SCROLLBAR_THUMB_VERTICAL:
		return moz_gtk_scrollbar_thumb_paint(widget, drawable,
		                                     rect, cliprect,
		                                     state);
		break;
	case MOZ_GTK_GRIPPER:
		return moz_gtk_gripper_paint(drawable, rect, cliprect,
		                             state);
		break;
	case MOZ_GTK_ENTRY:
		return moz_gtk_entry_paint(drawable, rect, cliprect,
		                           state);
		break;
	case MOZ_GTK_ENTRY_FRAME:
		return moz_gtk_entry_frame_paint(drawable, rect, cliprect,
		                                 state);
		break;
	case MOZ_GTK_DROPDOWN_ARROW:
		return moz_gtk_dropdown_arrow_paint(drawable, rect,
		                                    cliprect, state);
		break;
	case MOZ_GTK_CHECKBUTTON_CONTAINER:
	case MOZ_GTK_RADIOBUTTON_CONTAINER:
		return moz_gtk_container_paint(drawable, rect,
		                               cliprect, state,
		                               (widget ==
		                                MOZ_GTK_RADIOBUTTON_CONTAINER));
		break;
	case MOZ_GTK_TOOLBAR:
		return moz_gtk_toolbar_paint(drawable, rect,
		                             cliprect);
		break;
	case MOZ_GTK_TOOLTIP:
		return moz_gtk_tooltip_paint(drawable, rect,
		                             cliprect);
		break;
	case MOZ_GTK_FRAME:
		return moz_gtk_frame_paint(drawable, rect, cliprect);
		break;
	case MOZ_GTK_PROGRESSBAR:
		return moz_gtk_progressbar_paint(drawable, rect,
		                                 cliprect);
		break;
	case MOZ_GTK_PROGRESS_CHUNK:
		return moz_gtk_progress_chunk_paint(drawable, rect,
		                                    cliprect, flags);
		break;
	case MOZ_GTK_TABPANELS:
		{
			return moz_gtk_tabpanels_paint(drawable, rect,
			                               cliprect, state->curpos,
			                               state->maxpos);
		}
		break;
	case MOZ_GTK_OPTIONBUTTON:
		return moz_gtk_optionbutton_paint(drawable, rect,
		                                  cliprect, state);
		break;
	case MOZ_GTK_TAB:
		return moz_gtk_tab_paint(drawable, rect, cliprect,
		                         flags);
		break;
	case MOZ_GTK_LISTBOX:
		return moz_gtk_listbox_paint(drawable, rect, cliprect);
		break;
	case MOZ_GTK_SPINBUTTON:
		return moz_gtk_spinbutton_paint(drawable, rect, cliprect, state, flags);
		break;
	case MOZ_GTK_MENUITEMHIGHLIGHT:
		return moz_gtk_menuitem_paint(drawable, rect, cliprect);
		break;
	case MOZ_GTK_SCALE_TRACK_VERTICAL:
	case MOZ_GTK_SCALE_TRACK_HORIZONTAL:
		return moz_gtk_scale_track_paint(widget, drawable, rect, cliprect, flags);
		break;

	default:
		//g_warning("Unknown widget type: %d", widget);
		break;
	}

	return MOZ_GTK_UNKNOWN_WIDGET;
}

gint moz_gtk_shutdown()
{
	return MOZ_GTK_SUCCESS;
}
