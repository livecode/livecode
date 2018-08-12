/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

// Warning: X headers define True and False!

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcstring.h"
#include "util.h"
#include "globals.h"
#include "stack.h"
#include "stacklst.h"
#include "redraw.h"

#include "mctheme.h"
#include "context.h"
#include "lnxgtkthemedrawing.h"
#include "lnxtheme.h"
#include "lnximagecache.h"

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#define FIXED_THUMB_SIZE 17

typedef struct _gtkinfo
{
	int flags ;
}
MCGtkWidgetInfo;

static int4 gtktracksize = 0;

extern void gtk_init(void);

//inline functions and structures

inline void gdk_copy_rect(const GdkRectangle &src, GdkRectangle &dest)
{
	dest.x = src.x;
	dest.y = src.y;
	dest.width = src.width;
	dest.height = src.height;
}


//UTILITY FUNCTIONS
static GdkRectangle MCRectangleToGdkRectangle(const MCRectangle &rect)
{

	GdkRectangle ret;
	ret.x = rect.x;
	ret.y = rect.y;
	ret.width = rect.width;
	ret.height = rect.height;
	return ret;
}

static GtkWidgetState WThemeStateToGtkWidgetState(const MCWidgetInfo &winfo)
{
	GtkWidgetState ret;
	ret.active = (winfo.state & WTHEME_STATE_PRESSED);
	ret.focused = (winfo.state & WTHEME_STATE_HASFOCUS);
	ret.inHover = (winfo.state & WTHEME_STATE_HOVER);
	ret.disabled = (winfo.state & WTHEME_STATE_DISABLED);
	ret.isDefault = (winfo.state & WTHEME_STATE_HASDEFAULT); // only used for buttons (unused)
	ret.canDefault = winfo.type == WTHEME_TYPE_PUSHBUTTON && (winfo.state & WTHEME_STATE_HASDEFAULT); // ditto...
	ret.curpos = 0; // scrollbar only
	ret.maxpos = 0; // ditto..
	return ret;
}

static GtkWidgetState getpartandstate(const MCWidgetInfo &winfo, GtkThemeWidgetType &moztype, gint &flags)
{
	flags = 0;
	GtkWidgetState state = WThemeStateToGtkWidgetState(winfo);
	switch(winfo.type)
	{
	case WTHEME_TYPE_PUSHBUTTON:
		moztype = MOZ_GTK_BUTTON;     //MOZ_GTK_MENUITEMHIGHLIGHT;//
		break;
	case WTHEME_TYPE_CHECKBOX:
		moztype = MOZ_GTK_CHECKBUTTON;
		flags = winfo.state & WTHEME_STATE_HILITED;
		break;
	case WTHEME_TYPE_RADIOBUTTON:
		moztype = MOZ_GTK_RADIOBUTTON;
		flags = winfo.state & WTHEME_STATE_HILITED ? TRUE: FALSE;
		break;
	case WTHEME_TYPE_GROUP_FRAME:
	case WTHEME_TYPE_GROUP_FILL:
		moztype = MOZ_GTK_FRAME;
		break;
	case WTHEME_TYPE_TABPANE:
		moztype = MOZ_GTK_TABPANELS;
		break;
	case WTHEME_TYPE_TAB:
		if(winfo.attributes & WTHEME_ATT_TABFIRSTSELECTED)
			flags |= MOZ_GTK_TAB_FIRST;
		if (winfo.attributes & WTHEME_ATT_TABLEFTEDGE)
			flags |= MOZ_GTK_TAB_BEFORE_SELECTED;
		if(winfo.state & WTHEME_STATE_HILITED)
		{
			flags |= MOZ_GTK_TAB_SELECTED;
			state.active = 0;
		}
		if(winfo.attributes & WTHEME_ATT_TABPOSBOTTOM)
			flags |= MOZ_GTK_TAB_POS_BOTTOM;
		else if(winfo.attributes & WTHEME_ATT_TABPOSLEFT)
			flags |= MOZ_GTK_TAB_POS_LEFT;
		else if(winfo.attributes & WTHEME_ATT_TABPOSRIGHT)
			flags |= MOZ_GTK_TAB_POS_RIGHT;
		else
			flags |= MOZ_GTK_TAB_POS_TOP;
		moztype = MOZ_GTK_TAB;
		break;
	case WTHEME_TYPE_COMBOBUTTON:
		moztype = MOZ_GTK_DROPDOWN_ARROW;
		break;
	case WTHEME_TYPE_COMBOTEXT://comboframes draw like textfields
	case WTHEME_TYPE_TEXTFIELD:
		moztype = MOZ_GTK_ENTRY;
		break;
	case WTHEME_TYPE_TEXTFIELD_FRAME:
		moztype = MOZ_GTK_ENTRY_FRAME;
		break;
	case WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL:
		moztype = MOZ_GTK_SCROLLBAR_TRACK_VERTICAL;
		break;
	case WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL:
		moztype = MOZ_GTK_SCROLLBAR_TRACK_HORIZONTAL;
		break;
	case WTHEME_TYPE_SCROLLBAR_BUTTON_UP:
		flags = GTK_ARROW_UP;
		moztype = MOZ_GTK_SCROLLBAR_BUTTON;
		break;
	case WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN:
		flags = GTK_ARROW_DOWN;
		moztype = MOZ_GTK_SCROLLBAR_BUTTON;
		break;
	case WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT:
		flags = GTK_ARROW_LEFT;
		moztype = MOZ_GTK_SCROLLBAR_BUTTON;
		break;
	case WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT:
		flags = GTK_ARROW_RIGHT;
		moztype =MOZ_GTK_SCROLLBAR_BUTTON;
		break;
	case WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL:
		flags = GTK_ARROW_RIGHT;
		moztype = MOZ_GTK_SCROLLBAR_THUMB_VERTICAL;
		break;
	case WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL:
		flags = GTK_ARROW_RIGHT;
		moztype = MOZ_GTK_SCROLLBAR_THUMB_HORIZONTAL;
		break;
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL:
		flags = GTK_ARROW_RIGHT;
		moztype = MOZ_GTK_GRIPPER;
		break;
	case WTHEME_TYPE_OPTIONBUTTON:
		moztype = MOZ_GTK_OPTIONBUTTON;
		break;
	case WTHEME_TYPE_TOOLTIP:
		moztype = MOZ_GTK_TOOLTIP;
		break;
	case WTHEME_TYPE_PROGRESSBAR_HORIZONTAL:
	case WTHEME_TYPE_PROGRESSBAR_VERTICAL:
		moztype = MOZ_GTK_PROGRESSBAR;
		break;
	case WTHEME_TYPE_PROGRESSBAR_CHUNK:
		flags = GTK_PROGRESS_LEFT_TO_RIGHT;
		moztype = MOZ_GTK_PROGRESS_CHUNK;
		break;
	case WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL:
		flags = GTK_PROGRESS_BOTTOM_TO_TOP;
		moztype = MOZ_GTK_PROGRESS_CHUNK;
		break;
	case WTHEME_TYPE_LISTBOX:
		moztype = MOZ_GTK_LISTBOX;
		break;
	case WTHEME_TYPE_SMALLSCROLLBAR:
		{
			// FG-2014-07-30: [[ Bugfix 13025 ]] Linux spinboxes were inverted
			moztype = MOZ_GTK_SPINBUTTON;
			if(winfo.part == WTHEME_PART_ARROW_DEC)
			{
				flags = GTK_POS_TOP;
			}
			else if(winfo.part == WTHEME_PART_ARROW_INC)
			{
				flags = GTK_POS_BOTTOM;
			}
			else
			{
				flags = 0;
			}
		}
		break;
	case WTHEME_TYPE_MENUITEMHIGHLIGHT:
		moztype = MOZ_GTK_MENUITEMHIGHLIGHT;
		break;
	case WTHEME_TYPE_SLIDER_TRACK_VERTICAL:
		moztype = MOZ_GTK_SCALE_TRACK_VERTICAL;
		break;
	case WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL:
		moztype = MOZ_GTK_SCALE_TRACK_HORIZONTAL;
		break;
	default:
		moztype = MOZ_GTK_LABEL;
	}
	return state;
}

static gboolean reload_theme(void)
{
	Boolean reload = True;
	if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)
	{
		// We have changed themes, so remove the image cache and replace with new one
		if ( MCimagecache != NULL)
			delete MCimagecache ;
		MCimagecache = new (nothrow) MCXImageCache ;

		MCcurtheme->unload();
		MCcurtheme->load();

		// MW-2011-08-17: [[ Redraw ]] The theme has changed so redraw everything.
		MCRedrawDirtyScreen();
	}
	return (TRUE);
}



static int4 getscrollbarmintracksize()
{
	int sliderWidth = 0 , troughBorder = 0, stepperSize = 0, stepperSpacing = 0,
	                                     minSliderSize = 0 , focusLineWidth = 0, focusPadding = 0;
	Boolean slider = False;

	moz_gtk_get_scrollbar_metrics(&sliderWidth,
	                              &troughBorder,
	                              &stepperSize,
	                              &stepperSpacing,
	                              &minSliderSize,
	                              &focusLineWidth,
	                              &focusPadding);


	MCRectangle minrect;
	int n_steppers = (slider ? 0 : 2);
	static const int slider_length = 1;
	minrect.x = minrect.y = 0;

	minrect.width = troughBorder * 2 + sliderWidth;
	minrect.height = stepperSize * n_steppers + stepperSpacing * 2 + troughBorder * 2 + slider_length;
	int stepper_width = minrect.width;
	if(stepper_width < 1)
		stepper_width = minrect.width;
	return stepper_width;
}


//core theme class functions
Boolean MCNativeTheme::load()
{
	static Boolean initialised = False;

	if (!MCscreen -> hasfeature(PLATFORM_FEATURE_NATIVE_THEMES))
		return false;
	
	if (!initialised)
	{
		gtk_init();
		
		initialised = True;
		GtkSettings *settings = gtk_settings_get_default();
		if (settings)
			g_signal_connect_data( settings, "notify::gtk-theme-name", G_CALLBACK(reload_theme),
			                         NULL, NULL, (GConnectFlags)0);
	}
	gtkpix = NULL;
	mNeedNewGC = true;

	if (MCscreen)
	{
		MCColor tbackcolor;
		moz_gtk_get_widget_color(GTK_STATE_NORMAL,
		                         tbackcolor.red,tbackcolor.green,tbackcolor.blue) ;
		MCscreen->background_pixel = tbackcolor;//tcolor = zcolor;
		
		// MW-2012-01-27: [[ Bug 9511 ]] Set the hilite color based on the current GTK theme.
		MCColor thilitecolor;
		moz_gtk_get_widget_color(GTK_STATE_SELECTED, thilitecolor.red, thilitecolor.green, thilitecolor.blue);
		MChilitecolor = thilitecolor;
	}

	return true;
}




void MCNativeTheme::unload()
{
		
	//unload gtk libraries at runtime and do deinit stuff
	if (gtkpix != NULL)
		g_object_unref(gtkpix);

	//make sure that we call moz_gtk_shutdown first in case it uses gtk
	moz_gtk_shutdown();
	mNeedNewGC = true;
}

uint2 MCNativeTheme::getthemeid()
{
	return LF_NATIVEGTK; //it's a native windows theme
}

uint2 MCNativeTheme::getthemefamilyid()
{
	return LF_WIN95; //gtk inherits from the win95 theme
}

void MCNativeTheme::getscrollbarrects(const MCWidgetInfo & winfo,
                                      const MCRectangle & srect,
                                      MCRectangle & sbincarrowrect,
                                      MCRectangle & sbdecarrowrect,

                                      MCRectangle & sbthumbrect,
                                      MCRectangle & sbinctrackrect,
                                      MCRectangle & sbdectrackrect)
{
	int sliderWidth = 0 , troughBorder = 0, stepperSize = 0, stepperSpacing = 0,
	                                     minSliderSize = 0 , focusLineWidth = 0, focusPadding = 0;
	Boolean isVertical = ((winfo.attributes & WTHEME_ATT_SBVERTICAL) != 0);
	Boolean slider = winfo.type == WTHEME_TYPE_SLIDER;

#define CLEAR_RECT(rect) do { rect.x = rect.y = rect.width = rect.height = 0; } while(0)

	CLEAR_RECT(sbincarrowrect);
	CLEAR_RECT(sbdecarrowrect);
	CLEAR_RECT(sbthumbrect);
	CLEAR_RECT(sbinctrackrect);
	CLEAR_RECT(sbdectrackrect);


	if(slider)
		moz_gtk_get_slider_metrics(&sliderWidth,
		                           &troughBorder,
		                           &stepperSize,
		                           &stepperSpacing,
		                           &minSliderSize,
		                           &focusLineWidth,
		                           &focusPadding);
	else
		moz_gtk_get_scrollbar_metrics(&sliderWidth,
		                              &troughBorder,
		                              &stepperSize,
		                              &stepperSpacing,
		                              &minSliderSize,
		                              &focusLineWidth,
		                              &focusPadding);

	if (winfo.datatype != WTHEME_DATA_SCROLLBAR)
		return;

	MCRectangle minrect;

	int n_steppers = (slider ? 0 : 2);
	static const int slider_length = 1;

	minrect.x = minrect.y = 0;

	if (isVertical)
	{
		minrect.width = troughBorder * 2 + sliderWidth;
		minrect.height = stepperSize * n_steppers + stepperSpacing * 2 + troughBorder * 2 + slider_length;
	}
	else
	{
		minrect.width = stepperSize * n_steppers + stepperSpacing * 2 + troughBorder * 2 + slider_length;
		minrect.height = troughBorder * 2 + sliderWidth;
	}


	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *) winfo.data;
	uint2 trackwidth,trackheight;
	if (slider)
		sbinfo->thumbsize = 1;

	// get bounds for arrow buttons
	sbdecarrowrect.x = srect.x + troughBorder;
	sbdecarrowrect.y = srect.y + troughBorder;

	if(isVertical)
	{
		int stepper_width = minrect.width - troughBorder * 2;
		int stepper_height = 0;

		if(n_steppers > 0)
			stepper_height = MIN(stepperSize, (minrect.height / n_steppers));

		if(stepper_width < 1)
			stepper_width = minrect.width;


		sbdecarrowrect.width = sbincarrowrect.width = stepper_width;
		sbdecarrowrect.height = sbincarrowrect.height = stepper_height;
		trackwidth = stepper_width;
		trackheight = stepper_height;
		sbincarrowrect.x = sbdecarrowrect.x;
		sbincarrowrect.y = srect.y + srect.height - sbincarrowrect.height - troughBorder;

	}
	else
	{
		int stepper_height = minrect.height - troughBorder * 2;
		int stepper_width = 0;
		if(n_steppers > 0)

			stepper_width = MIN(stepperSize, (minrect.width / n_steppers));

		if(stepper_height < 1)
			stepper_height = minrect.height;

		sbdecarrowrect.width = sbincarrowrect.width = stepper_width;
		sbdecarrowrect.height = sbincarrowrect.height = stepper_height;
		trackwidth = stepper_width;
		trackheight = stepper_height;
		sbincarrowrect.x = srect.x + srect.width - sbdecarrowrect.width - troughBorder;
		sbincarrowrect.y = sbdecarrowrect.y;
	}

	{
		//get bounds for thumb 
		real8 range = sbinfo->endvalue - sbinfo->startvalue;
		if (winfo.attributes & WTHEME_ATT_SBVERTICAL)
		{
            // AL-2014-01-16: [[ Bug 11677 ]] No need to make slider thumb disappear at specific width/height ratio
			if (slider || ((sbinfo->thumbsize != 0 ) && srect.height > srect.width * 3))
			{
				sbthumbrect.x = srect.x + troughBorder;
				sbthumbrect.width = trackwidth;
				if (MCproportionalthumbs)
				{
					sbthumbrect.y = sbdecarrowrect.y + sbdecarrowrect.height;
					real8 height = srect.height - (sbdecarrowrect.height + sbincarrowrect.height) - (troughBorder * 2);
					if (range != 0.0 && fabs(sbinfo->endvalue - sbinfo->startvalue) != sbinfo->thumbsize)
					{
						int2 miny = sbthumbrect.y;


						real8 offset = height * (sbinfo->thumbpos - sbinfo->startvalue);
						sbthumbrect.y += (int2) (offset / range);
						range = fabs(range);
						sbthumbrect.height = (uint2)(sbinfo->thumbsize * height / range);
						uint2 minsize = srect.width;
						if (sbthumbrect.height < minsize)
						{
							uint2 diff =
								minsize -
								sbthumbrect.height;
							sbthumbrect.height = minsize;
							sbthumbrect.y -=
								(int2) (diff *
										(sbinfo->
										 thumbpos +
										 sbinfo->
										 thumbsize -
										 sbinfo->
										 startvalue) /
										range);
							if (sbthumbrect.y < miny)
								sbthumbrect.y = miny;
						}
					}
					else
						sbthumbrect.height = (int2) height - 1;
				}
				else

				{

					real8 height =
						sbthumbrect.height -
						(srect.width << 1) - FIXED_THUMB_SIZE;
					real8 offset =
						height * (sbinfo->thumbpos -
								  sbinfo->startvalue);
					if (range < 0)
						range += sbinfo->thumbsize;
					else
						range -= sbinfo->thumbsize;
					sbthumbrect.y = srect.y + srect.width;
					if (range != 0.0)
						sbthumbrect.y +=
							(int2) (offset / range);
					sbthumbrect.height = FIXED_THUMB_SIZE;
				}

				sbdectrackrect.x = sbinctrackrect.x = srect.x;

				sbdectrackrect.width = sbinctrackrect.width
									   = trackwidth + (troughBorder * 2);

				sbinctrackrect.y = sbthumbrect.y + sbthumbrect.height;
				sbinctrackrect.height = sbincarrowrect.y -
										(sbthumbrect.y + sbthumbrect.height);

				sbdectrackrect.y = sbdecarrowrect.y + sbdecarrowrect.height;
				sbdectrackrect.height = sbthumbrect.y -
										(sbdecarrowrect.y + sbdecarrowrect.height);
			}
			else
			{
				sbthumbrect.height = 0;
				sbinctrackrect = srect;
				sbinctrackrect.width = trackwidth;
				sbdectrackrect = sbinctrackrect;
			}

		}
		else
		{
            // AL-2014-01-16: [[ Bug 11677 ]] No need to make slider thumb disappear at specific width/height ratio
			if (slider || ((sbinfo->thumbsize != 0 ) && srect.width > srect.height * 3))
			{
				sbthumbrect.y = srect.y + troughBorder;
				sbthumbrect.height = trackheight;
				if (MCproportionalthumbs)
				{
					sbthumbrect.x = sbdecarrowrect.x + sbdecarrowrect.width;
					real8 width = srect.width - (sbdecarrowrect.width + sbincarrowrect.width) - (troughBorder * 2);
					if (range != 0.0 && fabs(sbinfo->endvalue - sbinfo->startvalue) != sbinfo->thumbsize)
					{

						int2 minx = sbthumbrect.x;
						real8 offset = (width * (sbinfo->thumbpos - sbinfo->startvalue));
						sbthumbrect.x += (int2)(offset / range);
						range = fabs(range);
						sbthumbrect.width = (uint2) ((slider?0:sbinfo->thumbsize) * width / range);
						uint2 minsize = srect.height;
						if (sbthumbrect.width < minsize)
						{
							uint2 diff =
								minsize -
								sbthumbrect.width;
							sbthumbrect.width = minsize;
							sbthumbrect.x -=
								(int2) (diff *
										(sbinfo->
										 thumbpos +
										 (slider?0:sbinfo->
										  thumbsize) -
										 sbinfo->
										 startvalue) /
										range);
							if (sbthumbrect.x < minx)
								sbthumbrect.x = minx;
						}

					}
					else
						sbthumbrect.width = (int2) width - 1;
				}
				else
				{
					real8 width =
						srect.width - (srect.height << 1) -
						FIXED_THUMB_SIZE;
					real8 offset =
						width * (sbinfo->thumbpos -
								 sbinfo->startvalue);
					sbthumbrect.x = srect.x + srect.height;

					if (range < 0)
						range += sbinfo->thumbsize;
					else
						range -= sbinfo->thumbsize;
					if (range != 0.0)
						sbthumbrect.x +=
							(int2) (offset / range);
					sbthumbrect.width = FIXED_THUMB_SIZE;
				}
				sbdectrackrect.y = sbinctrackrect.y = srect.y;
				sbdectrackrect.height = sbinctrackrect.height =
											trackheight + (troughBorder * 2);

				sbinctrackrect.x = sbthumbrect.x + sbthumbrect.width;
				sbinctrackrect.width = sbincarrowrect.x -
									   (sbthumbrect.x + sbthumbrect.width);

				sbdectrackrect.x = sbdecarrowrect.x + sbdecarrowrect.width;
				sbdectrackrect.width = sbthumbrect.x -
									   (sbdecarrowrect.x + sbdecarrowrect.width);

			}
			else
			{
				sbthumbrect.width = 0;
				sbinctrackrect = srect;
				sbinctrackrect.height = trackheight;
				sbdectrackrect = sbinctrackrect;
			}
		}
	}
}








void MCNativeTheme::getwidgetrect(const MCWidgetInfo &winfo,
                                  Widget_Metric wmetric,
                                  const MCRectangle &srect,
                                  MCRectangle &drect)

{
	//metric part size is to get size of a part in a widget
	//metric draw size is to get the actual size that the part will draw
	//in given a rect. Useful for fixed sized widgets (combobox, scrollbars)
	//optimum size is the default size that a part should be drawn at using the standard
	//hig guidlines. minimum size if the minimum size that a part can draw at..
	//some widgets will crash in some themes if you try to draw smaller!
	switch (wmetric)
	{
	case WTHEME_METRIC_CONTENTSIZE:
		{
			gint xthickness,ythickness;
			GtkThemeWidgetType  moztype;
			gint flags = 0 ;
			getpartandstate(winfo, moztype, flags);
			if (moz_gtk_get_widget_border(moztype, &xthickness,
			                              &ythickness) == MOZ_GTK_SUCCESS)
			{
				drect = srect;
				drect.width -= (xthickness * 2);
				drect.height -= (ythickness * 2);
				drect.x += xthickness;
				drect.y += ythickness;
			}
			return;
		}
		break;
	case WTHEME_METRIC_DRAWSIZE:
		//size that widget will draw at. needed for controls that cannot scale (scrollbars).
		if (winfo.type == WTHEME_TYPE_SCROLLBAR || winfo.type == WTHEME_TYPE_SLIDER)
		{

			bool isVertical = ((winfo.attributes & WTHEME_ATT_SBVERTICAL) != 0);
			int sliderWidth, troughBorder, stepperSize, stepperSpacing,
			minSliderSize, focusLineWidth, focusPadding;
			moz_gtk_get_scrollbar_metrics(&sliderWidth,
			                              &troughBorder,
			                              &stepperSize,
			                              &stepperSpacing,
			                              &minSliderSize,

			                              &focusLineWidth,
			                              &focusPadding);

			MCRectangle sbincarrowrect,sbdecarrowrect,sbthumbrect,sbinctrackrect,sbdectrackrect;
			getscrollbarrects(winfo, srect,
			                  sbincarrowrect,
			                  sbdecarrowrect, sbthumbrect,
			                  sbinctrackrect,
			                  sbdectrackrect);
			drect.x = srect.x;
			drect.y = srect.y;
			if(isVertical)
			{
				drect.width = sbinctrackrect.width;
				drect.height = sbincarrowrect.height +
				               sbdecarrowrect.height +

				               sbthumbrect.height +
				               sbinctrackrect.height +
				               sbdectrackrect.height +

				               (troughBorder * 2);
			}
			else
			{
				drect.width = sbincarrowrect.width +
				              sbdecarrowrect.width +
				              sbthumbrect.width +
				              sbinctrackrect.width +
				              sbdectrackrect.width +
				              (troughBorder * 2);

				drect.height = sbinctrackrect.height;
			}
			return;
		}
	case WTHEME_METRIC_PARTSIZE:

		switch (winfo.type)
		{
		case WTHEME_TYPE_SCROLLBAR:
		case WTHEME_TYPE_PROGRESSBAR:
		case WTHEME_TYPE_SLIDER:
			{
				MCRectangle sbincarrowrect, sbdecarrowrect,
				sbthumbrect, sbinctrackrect,
				sbdectrackrect;

				getscrollbarrects(winfo, srect,
				                  sbincarrowrect,
				                  sbdecarrowrect, sbthumbrect,
				                  sbinctrackrect,
				                  sbdectrackrect);
				switch (winfo.part)
				{
				case WTHEME_PART_ARROW_DEC:
					drect = sbdecarrowrect;
					break;
				case WTHEME_PART_ARROW_INC:
					drect = sbincarrowrect;
					break;
				case WTHEME_PART_TRACK_DEC:

					drect = sbdectrackrect;
					break;
				case WTHEME_PART_TRACK_INC:
					drect = sbinctrackrect;
					break;
				case WTHEME_PART_THUMB:
					drect = sbthumbrect;
					break;
				default:
					break;
				}

				return;
			}
		case WTHEME_TYPE_COMBO:
			{
				gint tcombobtnwidth = srect.width;
				gint tcombobtnheight = srect.height;
				MCRectangle combobuttonrect;
				moz_gtk_get_dropdown_arrow_size(&tcombobtnwidth,&tcombobtnheight);
				real8 theightscale 	= tcombobtnheight/tcombobtnwidth;
				uint1 comboframesize = 0;//we should query comboframe value

				tcombobtnwidth = srect.height - (comboframesize * 2);
				tcombobtnheight = (uint2)(tcombobtnwidth * theightscale);
				combobuttonrect.x = srect.x + (srect.width - tcombobtnwidth - comboframesize) ;
				combobuttonrect.y = srect.y + comboframesize;
				combobuttonrect.width = tcombobtnwidth;
				combobuttonrect.height = tcombobtnheight;
				if (winfo.part == WTHEME_PART_COMBOTEXT)
					drect = MCU_subtract_rect(srect,combobuttonrect);
				else if (winfo.part == WTHEME_PART_COMBOBUTTON)
					drect = combobuttonrect;
				return;
			}
		default:
			break;
		}
		break;
	default:
		break;
	}
	//pass through to base class or when themes inherit from each other
	//base class
	MCTheme::getwidgetrect(winfo, wmetric, srect, drect);
}

Boolean MCNativeTheme::getthemepropbool(Widget_ThemeProps themeprop)
{
	//some themes support hovering and those themes need to redraw controls
	//when mouse is over control. GTK supports this.
	if (themeprop == WTHEME_PROP_SUPPORTHOVERING)

		return True;
	if (themeprop == WTHEME_PROP_ALWAYSBUFFER)
		return True;
	return False;
}

// MW-2011-09-14: [[ Bug 9719 ]] Return the actual size of the (opaque) MCThemeDrawInfo struct.
uint32_t MCNativeTheme::getthemedrawinfosize(void)
{
	return sizeof(MCThemeDrawInfo);
}

//hit testing
Widget_Part MCNativeTheme::hittestcombobutton(const MCWidgetInfo &winfo,
        int2 mx, int2 my, const MCRectangle &drect)
{
	Widget_Part wpart = WTHEME_PART_UNDEFINED;
	const uint2 btnWidth = MCClamp(getmetric(WTHEME_METRIC_COMBOSIZE),
								   0, drect.width);
	
	uint2 t_text_width = drect.width - btnWidth;
	int2 t_btn_left = drect.x + t_text_width;
	
	MCRectangle btnRect = {	t_btn_left, drect.y,
							btnWidth, drect.height };
	MCRectangle txtRect = { drect.x, drect.y,
		                    t_text_width, drect.height };

	if(MCU_point_in_rect(btnRect, mx, my))
		wpart = WTHEME_PART_COMBOBUTTON;
	else if(MCU_point_in_rect(txtRect, mx, my))
		wpart = WTHEME_PART_COMBOTEXT;

	return wpart;
}



Widget_Part MCNativeTheme::hittestspinbutton(const MCWidgetInfo &winfo,
        int2 mx, int2 my, const MCRectangle &drect)
{
	Widget_Part wpart = WTHEME_PART_UNDEFINED;
	GdkRectangle buttonrect, arrowrect;
	MCRectangle brect;
	GdkRectangle rect = {drect.x, drect.y, drect.width, drect.height };
	spinbutton_get_rects(GTK_ARROW_UP, &rect, buttonrect, arrowrect);
	brect.x = buttonrect.x;
	brect.y = buttonrect.y;
	brect.width = buttonrect.width;

	// FG-2014-07-30: [[ Bugfix 13025 ]] Spinbox arrows were inverted
	brect.height = buttonrect.height;
	if(MCU_point_in_rect(brect, mx, my))
		wpart = WTHEME_PART_ARROW_DEC;
	if(wpart == WTHEME_PART_UNDEFINED)
	{
		spinbutton_get_rects(GTK_ARROW_DOWN, &rect, buttonrect, arrowrect);
		brect.x = buttonrect.x;
		brect.y = buttonrect.y;
		brect.width = buttonrect.width;
		brect.height = buttonrect.height;
		if(MCU_point_in_rect(brect, mx, my))
			wpart = WTHEME_PART_ARROW_INC;
	}
	return wpart;
}


Widget_Part MCNativeTheme::hittestscrollcontrols(const MCWidgetInfo &winfo,
        int2 mx, int2 my, const MCRectangle &drect)
{
	Widget_Part wpart = WTHEME_PART_UNDEFINED;

	MCRectangle sbincarrowrect, sbdecarrowrect, sbthumbrect,
	sbinctrackrect, sbdectrackrect;

	getscrollbarrects(winfo, drect, sbincarrowrect, sbdecarrowrect,
	                  sbthumbrect, sbinctrackrect, sbdectrackrect);

	if(MCU_point_in_rect(sbdecarrowrect, mx, my))
		wpart = WTHEME_PART_ARROW_DEC;
	else if(MCU_point_in_rect(sbincarrowrect, mx, my))
		wpart = WTHEME_PART_ARROW_INC;
	else if(MCU_point_in_rect(sbinctrackrect, mx, my))
		wpart = WTHEME_PART_TRACK_INC;
	else if(MCU_point_in_rect(sbdectrackrect, mx, my))
		wpart = WTHEME_PART_TRACK_DEC;
	else if(MCU_point_in_rect(sbthumbrect, mx, my))
		wpart = WTHEME_PART_THUMB;


	return wpart;
}



Widget_Part MCNativeTheme::hittest(const MCWidgetInfo &winfo,
                                   int2 mx, int2 my, const MCRectangle &drect)
{
	Widget_Part ret = WTHEME_PART_UNDEFINED;

	switch (winfo.type)
	{
	case WTHEME_TYPE_COMBOBUTTON:
		return hittestcombobutton(winfo, mx, my, drect);
		break;
	case WTHEME_TYPE_SMALLSCROLLBAR:
		return hittestspinbutton(winfo, mx, my, drect);
		break;
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_SCROLLBAR:
		return hittestscrollcontrols(winfo, mx, my, drect);
		break;
	default:
		return MCU_point_in_rect(drect, mx,
		                         my) ? WTHEME_PART_ALL : WTHEME_PART_UNDEFINED;
		break;
	}

	return ret;
}


Boolean MCNativeTheme::iswidgetsupported(Widget_Type wtype)
{
	//return whether widget part is supported
	Boolean ret = False;

	switch(wtype)
	{
	case WTHEME_TYPE_UNDEFINED:
	case WTHEME_TYPE_CHECKBOX:
	case WTHEME_TYPE_PUSHBUTTON:
	case WTHEME_TYPE_RADIOBUTTON:
	case WTHEME_TYPE_OPTIONBUTTON:
	case WTHEME_TYPE_COMBO:
	case WTHEME_TYPE_COMBOBUTTON:
	case WTHEME_TYPE_COMBOTEXT:
	case WTHEME_TYPE_COMBOFRAME:
	case WTHEME_TYPE_PULLDOWN:
	case WTHEME_TYPE_TABPANE:
	case WTHEME_TYPE_TAB:
	case WTHEME_TYPE_TEXTFIELD:
	case WTHEME_TYPE_TEXTFIELD_FRAME:
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL:

	case WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_UP:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT:
	case WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL:
		// "pane separators"
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL:
	case WTHEME_TYPE_LISTBOX:
	case WTHEME_TYPE_LISTBOX_LISTITEM:
	case WTHEME_TYPE_TREEVIEW:
	case WTHEME_TYPE_SMALLSCROLLBAR:
	case WTHEME_TYPE_MENUITEMHIGHLIGHT:
	case WTHEME_TYPE_SLIDER_TRACK_VERTICAL:
	case WTHEME_TYPE_PROGRESSBAR:
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_PROGRESSBAR_HORIZONTAL:
	case WTHEME_TYPE_PROGRESSBAR_VERTICAL:
	case WTHEME_TYPE_PROGRESSBAR_CHUNK:
	case WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL:
	case WTHEME_TYPE_TOOLTIP:
		ret = True;
		break;
	default:
		ret = False;
	}

	return ret;
}

int4 MCNativeTheme::getmetric(Widget_Metric wmetric)
{
	//get metric for widget type without state info
	int4 ret = 0;
	switch(wmetric)
	{
	case WTHEME_METRIC_TABSTARTOFFSET:
		return 0; //offset from x to tab pane to x of first tab
		break;
	case WTHEME_METRIC_TABNONSELECTEDOFFSET:
		ret = 1;
		break;
	case WTHEME_METRIC_TABOVERLAP:
		ret = -1; // hardcoded
		break;
	case WTHEME_METRIC_TABRIGHTMARGIN:
	case WTHEME_METRIC_TABLEFTMARGIN:
		ret = 12;
		break;
	case WTHEME_METRIC_MINIMUMSIZE:
	case WTHEME_METRIC_OPTIMUMSIZE:
	case WTHEME_METRIC_DRAWSIZE:
	case WTHEME_METRIC_PARTSIZE:
		ret = 0;
		break;
	case WTHEME_METRIC_TRACKSIZE:
		if ( gtktracksize == 0 )
        {
            if (gtktracksize == 0)
                gtktracksize = getscrollbarmintracksize();
        }
		return gtktracksize;
		break;
	case WTHEME_METRIC_CHECKBUTTON_INDICATORSIZE:
        moz_gtk_checkbox_get_metrics(&ret, 0);
		break;
        case WTHEME_METRIC_CHECKBUTTON_INDICATORSPACING:
        moz_gtk_checkbox_get_metrics(0, &ret);
		break;
        case WTHEME_METRIC_RADIOBUTTON_INDICATORSIZE:
        moz_gtk_radiobutton_get_metrics(&ret, 0);
		break;
        case WTHEME_METRIC_RADIOBUTTON_INDICATORSPACING:
        moz_gtk_radiobutton_get_metrics(0, &ret);
		break;
	default:
		break;
	}

	return ret;
}


int4 MCNativeTheme::getwidgetmetric(const MCWidgetInfo & winfo,
                                    Widget_Metric wmetric)
{

	//get metric for widget type with state info
	return 0;
}

Boolean MCNativeTheme::drawprogressbar(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	Boolean vertical = winfo.attributes & WTHEME_ATT_SBVERTICAL ? True : False;
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR)
		return False;
	uint4 pbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED
	                           ? WTHEME_STATE_DISABLED
	                           : WTHEME_STATE_CLEAR;
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
	MCWidgetInfo twinfo = winfo;
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
	              ? WTHEME_TYPE_PROGRESSBAR_VERTICAL
	              : WTHEME_TYPE_PROGRESSBAR_HORIZONTAL;
	twinfo.state = pbpartdefaultstate;
	drawwidget(dc, twinfo, drect);
	MCRectangle progressbarrect;

	getwidgetrect(twinfo,WTHEME_METRIC_CONTENTSIZE,drect,progressbarrect);
	uint1 bordersize = drect.width - progressbarrect.width;
	if(vertical)
	{
		progressbarrect.height = drect.height;
	}
	else
	{
		progressbarrect.width = drect.width;
	}
	if (progressbarrect.width && progressbarrect.height)
	{
		int2 endpos = 0;
		if (winfo.attributes & WTHEME_ATT_SBVERTICAL)
		{
			endpos = (int2)(sbinfo->thumbpos
			                / (sbinfo->endvalue - sbinfo->startvalue)
			                * (real8)progressbarrect.height) + progressbarrect.y;
			uint2 ty = (progressbarrect.y+progressbarrect.height) - (endpos - progressbarrect.y);
			progressbarrect.height =  endpos - progressbarrect.y - bordersize;
			
			//TS-2007-08-10 Added to stop underflow if progress bar's current value = 0 
			if ( (endpos - progressbarrect.y - bordersize)  < 0 ) 
				progressbarrect . height = 0 ;
			progressbarrect.y = ty;
		}
		else
		{
			endpos = (int2)(sbinfo->thumbpos / (sbinfo->endvalue - sbinfo->startvalue)
			                * (real8)progressbarrect.width) + progressbarrect.x;
			progressbarrect.width =  endpos - progressbarrect.x - bordersize;
			
			//TS-2007-08-10 Added to stop underflow if progress bar's current value = 0 
			if ( ( endpos - progressbarrect.x - bordersize ) < 0 ) 
				progressbarrect . width = 0 ;
		}
		twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
		              ? WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL
		              : WTHEME_TYPE_PROGRESSBAR_CHUNK;
		twinfo.state = pbpartdefaultstate;

		drawwidget(dc, twinfo, progressbarrect);
	}
	return True;
}


void MCNativeTheme::drawSlider(MCDC *dc, const MCWidgetInfo &winfo,
                               const MCRectangle &drect)
{
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR &&
	        winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return;

	//draw arrows
	MCWidgetInfo twinfo = winfo;
	MCRectangle sbincarrowrect, sbdecarrowrect, sbthumbrect,
	sbinctrackrect, sbdectrackrect;
	MCRectangle rangerect;
	getscrollbarrects(winfo, drect,
	                  sbincarrowrect, sbdecarrowrect,
	                  sbthumbrect,
	                  sbinctrackrect, sbdectrackrect);
	getwidgetrect(winfo, WTHEME_METRIC_DRAWSIZE, drect, rangerect);
	uint4 sbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED ? WTHEME_STATE_DISABLED : WTHEME_STATE_CLEAR;
	memset(&twinfo, 0, sizeof(MCWidgetInfo));	//clear widget info
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
	              ? WTHEME_TYPE_SLIDER_TRACK_VERTICAL
	              : WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL;
	twinfo.state = sbpartdefaultstate;
	if (winfo.part == WTHEME_PART_TRACK_DEC)
		twinfo.state = winfo.state;
	drawwidget(dc, twinfo, rangerect);
	if(sbthumbrect.height && sbthumbrect.width)
	{
		//draw thumb
		twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
		              ? WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL
		              : WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL;
		twinfo.state = sbpartdefaultstate;
		if (winfo.part == WTHEME_PART_THUMB)
			twinfo.state = winfo.state;
		drawwidget(dc, twinfo, sbthumbrect);
	}

}


void MCNativeTheme::drawScrollbar(MCDC *dc, const MCWidgetInfo &winfo,
                                  const MCRectangle &drect)
{
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR &&
	        winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return;

	//draw arrows
	MCWidgetInfo twinfo = winfo;
	MCRectangle sbincarrowrect, sbdecarrowrect, sbthumbrect,
	sbinctrackrect, sbdectrackrect;
	MCRectangle rangerect;
	int sliderWidth, troughBorder, stepperSize, stepperSpacing,
	minSliderSize, focusLineWidth, focusPadding;

	moz_gtk_get_scrollbar_metrics(&sliderWidth,
	                              &troughBorder,
	                              &stepperSize,
	                              &stepperSpacing,
	                              &minSliderSize,
	                              &focusLineWidth,
	                              &focusPadding);


	getscrollbarrects(winfo, drect,
	                  sbincarrowrect, sbdecarrowrect,
	                  sbthumbrect,
	                  sbinctrackrect, sbdectrackrect);


	getwidgetrect(winfo, WTHEME_METRIC_DRAWSIZE, drect, rangerect);

	uint4 sbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED ? WTHEME_STATE_DISABLED : WTHEME_STATE_CLEAR;
	memset(&twinfo, 0, sizeof(MCWidgetInfo));	//clear widget info

	// Crux needs the track to be the same size as the actual scroll bar, so we
	// paint that first.
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
	              ? WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL
	              : WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL;
	twinfo.state = sbpartdefaultstate;
	if (winfo.part == WTHEME_PART_TRACK_DEC)
		twinfo.state = winfo.state;

	drawwidget(dc, twinfo, rangerect);



	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
	              ? WTHEME_TYPE_SCROLLBAR_BUTTON_UP
	              : WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT;
	twinfo.state = sbpartdefaultstate;
	if (winfo.part == WTHEME_PART_ARROW_DEC)
		twinfo.state = winfo.state;

	drawwidget(dc, twinfo, sbdecarrowrect);

	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
	              ? WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN
	              : WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT;
	twinfo.state = sbpartdefaultstate;
	if (winfo.part == WTHEME_PART_ARROW_INC)
		twinfo.state = winfo.state;

	drawwidget(dc, twinfo, sbincarrowrect);

	//TS - 2007-10-25 - theme adjustments
	if ( winfo.attributes & WTHEME_ATT_SBVERTICAL )
		sbthumbrect.height ++  ;
	else 
		sbthumbrect.width ++ ;
	
	
	if (sbthumbrect.height && sbthumbrect.width)
	{
		//draw thumb
		twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL
		              ? WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL
		              : WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL;
		twinfo.state = sbpartdefaultstate;
		if (winfo.part == WTHEME_PART_THUMB)
			twinfo.state = winfo.state;

		drawwidget(dc, twinfo, sbthumbrect);

	}
}


void MCNativeTheme::make_theme_info(MCThemeDrawInfo& ret, GtkThemeWidgetType widget, 
									 GdkDrawable * drawable,
                     				 GdkRectangle * rect, 
									 GdkRectangle * cliprect,
                     				 GtkWidgetState state, 
									 gint flags,
									 MCRectangle crect )

{
	ret . pm = drawable;
	ret . moztype = widget ;
	ret . cliprect = *cliprect;
	ret . drect = *rect;
	ret . state = state ;
	ret . flags = flags ;
	ret . crect = crect ;
}


void MCNativeTheme::drawTab(MCDC *t_dc, 
							const MCWidgetInfo &winfo,
                            const MCRectangle &drect, GdkPixmap *tpix)
{
	int flags = 0;
	GdkRectangle rect, cliprect;
	GtkWidgetState state = WThemeStateToGtkWidgetState(winfo);
	rect = MCRectangleToGdkRectangle(drect);

	cliprect = MCRectangleToGdkRectangle(drect);
	rect.x = cliprect.x = 0;
	rect.y = cliprect.y = 0;
	if(winfo.attributes & WTHEME_ATT_TABFIRSTSELECTED)
		flags |= MOZ_GTK_TAB_FIRST;

	//if(winfo.attributes & WTHEME_ATT_TABBEFORESELECTED)
	if (winfo.attributes & WTHEME_ATT_TABLEFTEDGE)
		flags |= MOZ_GTK_TAB_BEFORE_SELECTED;
	if(winfo.state & WTHEME_STATE_HILITED)
	{
		flags |= MOZ_GTK_TAB_SELECTED;
		state.active = 0;
	}
	if(winfo.attributes & WTHEME_ATT_TABPOSBOTTOM)
		flags |= MOZ_GTK_TAB_POS_BOTTOM;
	else if(winfo.attributes & WTHEME_ATT_TABPOSLEFT)
		flags |= MOZ_GTK_TAB_POS_LEFT;
	else if(winfo.attributes & WTHEME_ATT_TABPOSRIGHT)
		flags |= MOZ_GTK_TAB_POS_RIGHT;
	else
		flags |= MOZ_GTK_TAB_POS_TOP;
	
	moz_gtk_widget_paint(MOZ_GTK_TAB,

	                     tpix,
	                     &rect,
	                     &cliprect,
	                     &state,
	                     flags);
}







Boolean MCNativeTheme::drawwidget(MCDC *dc, const MCWidgetInfo & winfo,
                                  const MCRectangle & drect)
{
	GdkGC *gc ;
	
	MCThemeDrawInfo di ;

	MCDC * t_dc = dc ;	
	Boolean ret = False;
	
	static MCRectangle gtkpixrect = {0,0,0,0};

	GdkDisplay *display = MCdpy;

	GdkRectangle rect;
	GdkRectangle cliprect;

	MCRectangle trect = drect;
	MCRectangle crect = drect;

	if (winfo.type == WTHEME_TYPE_TAB)
		crect.height--; //make sure tab doesn't draw over tab pane
	

	
	rect = MCRectangleToGdkRectangle(trect);
	cliprect = MCRectangleToGdkRectangle(crect);
	rect.x = cliprect.x = 0;
	rect.y = cliprect.y = 0;


	if (rect.width <= 0 || rect.height <= 0)
		return False;
	GtkThemeWidgetType  moztype;
	gint flags = 0;
	GtkWidgetState state = getpartandstate(winfo, moztype, flags);

	
	
	switch(winfo.type)
	{
	case WTHEME_TYPE_TABPANE:
		state.curpos = 0;
		if(winfo.attributes & WTHEME_ATT_TABPOSBOTTOM
		        || winfo.attributes & WTHEME_ATT_TABPOSTOP)
			state.maxpos = rect.width;
		else if(winfo.attributes & WTHEME_ATT_TABPOSLEFT
		        || winfo.attributes & WTHEME_ATT_TABPOSRIGHT)
			state.maxpos = rect.height;
		else
			state.maxpos = rect.width; // XXX therefore default tabpos is top

		if(winfo.datatype == WTHEME_DATA_TABPANE)
		{

			MCWidgetTabPaneInfo *inf = (MCWidgetTabPaneInfo*)winfo.data;

			state.curpos = inf->gap_start - 1;
			state.maxpos = inf->gap_length + 2;

		}
		
		make_theme_info ( di, moztype, gtkpix, &rect, &cliprect, state, flags, crect);
		t_dc -> drawtheme ( THEME_DRAW_TYPE_GTK, &di ) ;
		
		ret = True;
		break;
		
		
	case WTHEME_TYPE_COMBO:
		{
			MCWidgetInfo twinfo = winfo;
			MCRectangle comboentryrect,combobuttonrect;
			//draw text box
			twinfo.part = WTHEME_PART_COMBOTEXT;
			getwidgetrect(twinfo, WTHEME_METRIC_PARTSIZE,drect,comboentryrect);
			twinfo.part = WTHEME_PART_COMBOBUTTON;
			getwidgetrect(twinfo, WTHEME_METRIC_PARTSIZE,drect,combobuttonrect);
			twinfo.type = WTHEME_TYPE_TEXTFIELD_FRAME;
			drawwidget(dc, twinfo, comboentryrect);
			twinfo.type = WTHEME_TYPE_COMBOBUTTON;
			drawwidget(dc, twinfo, combobuttonrect);
			return True;
		}
		
	
	
#define B_WIDTH 3
		
	case WTHEME_TYPE_TEXTFIELD_FRAME:
	{
		GtkWidgetState t_old_state;
		t_old_state = state;

		// crect is the target rectangle for the whole control
		// cliprect and rect are essentially the same
		
		GdkRectangle t_bounds;
		MCRectangle t_dst_bounds;
		MCRectangle t_clip;
		
		// First render the top and bottom borders. We render a control 8 pixels
		// high, but clipped to the border width.

		t_bounds . x = 0;
		t_bounds . y = 0;
		t_bounds . width = rect . width;
		t_bounds . height = 8;

		MCU_set_rect(t_dst_bounds, crect . x, crect . y, crect . width, 8);
		MCU_set_rect(t_clip, crect . x, crect . y, crect . width, B_WIDTH);
		make_theme_info(di, moztype, gtkpix, &t_bounds, &t_bounds, state, flags, t_dst_bounds);
		
		dc->save();
		dc->cliprect(t_clip);

		dc -> drawtheme(THEME_DRAW_TYPE_GTK, &di);
		
		dc->restore();
		
		MCU_set_rect(t_dst_bounds, crect . x, crect . y + crect . height - 8, crect . width, 8);
		MCU_set_rect(t_clip, crect . x, crect . y + crect . height - B_WIDTH, crect . width, B_WIDTH);
		make_theme_info(di, moztype, gtkpix, &t_bounds, &t_bounds, state, flags, t_dst_bounds);
		
		dc->save();
		dc->cliprect(t_clip);
		
		dc -> drawtheme(THEME_DRAW_TYPE_GTK, &di);
		
		dc->restore();
		
		// Now render the left and right borders. We render a control 8 pixels
		// wide, but clipped to the border width

		t_bounds . x = 0;
		t_bounds . y = 0;
		t_bounds . width = 8;
		t_bounds . height = rect . height;

		MCU_set_rect(t_dst_bounds, crect . x, crect . y, 8, crect . height);
		MCU_set_rect(t_clip, crect . x, crect . y + B_WIDTH, B_WIDTH, crect . height - 2 * B_WIDTH);
		make_theme_info(di, moztype, gtkpix, &t_bounds, &t_bounds, state, flags, t_dst_bounds);
		
		dc->save();
		dc->cliprect(t_clip);

		dc -> drawtheme(THEME_DRAW_TYPE_GTK, &di);
		
		dc->restore();
		
		MCU_set_rect(t_dst_bounds, crect . x + crect . width - 8, crect . y, 8, crect . height);
		MCU_set_rect(t_clip, crect . x + crect . width - B_WIDTH, crect . y + B_WIDTH, B_WIDTH, crect . height - 2 * B_WIDTH);
		make_theme_info(di, moztype, gtkpix, &t_bounds, &t_bounds, state, flags, t_dst_bounds);
		
		dc->save();
		dc->cliprect(t_clip);
		
		dc -> drawtheme(THEME_DRAW_TYPE_GTK, &di);

		dc->restore();
	}
	break ;

		
	case WTHEME_TYPE_SLIDER:
		drawSlider(dc, winfo, drect);
		return True;
		break;
		
		
	case WTHEME_TYPE_SCROLLBAR:
		drawScrollbar(dc, winfo, drect);
		return True;
		break;
		
		
	case WTHEME_TYPE_PROGRESSBAR:
		ret = drawprogressbar(dc, winfo, drect);
		return ret;
		break;
		
		
	case WTHEME_TYPE_GROUP_FRAME:
		
		make_theme_info ( di, moztype, gtkpix, &rect, &cliprect, state, flags, crect);
		t_dc -> drawtheme ( THEME_DRAW_TYPE_GTK, &di ) ;
		ret = True;
		
	break;
		
		
	default:

		make_theme_info ( di, moztype, gtkpix, &rect, &cliprect, state, flags, crect);
		t_dc -> drawtheme ( THEME_DRAW_TYPE_GTK, &di ) ;
		ret = True;
	}
	
	return ret;
}

MCTheme *MCThemeCreateNative(void)
{
	return new MCNativeTheme;
}

////////////////////////////////////////////////////////////////////////////////

#include "lnxdc.h"
static GdkPixbuf* calc_alpha_from_pixbufs(GdkPixbuf *p_pb_black, GdkPixbuf *p_pb_white)
{
	guchar* t_black_ptr;
	guchar* t_white_ptr;
	int t_black_stride;
	int t_white_stride;
    int t_black_channels;
    int t_white_channels;
	
	int t_w, t_h;
	
	t_w = gdk_pixbuf_get_width(p_pb_black);
    t_h = gdk_pixbuf_get_height(p_pb_black);
    
    t_black_stride = gdk_pixbuf_get_rowstride(p_pb_black);
    t_white_stride = gdk_pixbuf_get_rowstride(p_pb_white);
    
    t_black_channels = gdk_pixbuf_get_n_channels(p_pb_black);
    t_white_channels = gdk_pixbuf_get_n_channels(p_pb_white);
	
	/*
		Formula for calculating the alpha of the source, by 
		rendering it twice - once against black, the second time
		agaist white.
	
			Dc' = Sc.Sa + (1 - Sa) * Dc

			composite against black (Dc == 0):
			Dc'b = Sc.Sa => Dc'b <= Sa

			composite against white (Dc == 1.0)
			Dc'w = Sc.Sa + (1 - Sa)

			Sa = 1 - (Dc'w - Dc'b)
			Sc.Sa = Dc'b
	
			As Sc=Dc'b all we need to actually do is recalculate the alpha byte for the black image.
	*/
	
	uint8_t rb, rw;
	uint8_t na;
	int x, y;
	
	for ( y = 0 ; y < t_h; y ++ )
	{
		for ( x = 0 ; x < t_w ; x++ )
		{	
			t_white_ptr = gdk_pixbuf_get_pixels(p_pb_white) + (t_white_stride * y) + (t_white_channels * x);
            t_black_ptr = gdk_pixbuf_get_pixels(p_pb_black) + (t_black_stride * y) + (t_black_channels * x);

			rb = *(t_black_ptr);
			rw = *(t_white_ptr);
			
			na = uint8_t(255 - rw + rb);
			*(t_black_ptr + 3) = na;
		}
	}
	
	g_object_unref(p_pb_white);
	
	return p_pb_black;
}
	
static void fill_gdk_drawable(GdkDrawable *p_drawable, GdkColormap *p_colormap, int p_red, int p_green, int p_blue, int p_width, int p_height)
{
	GdkGC *t_gc;
	t_gc = gdk_gc_new(p_drawable);
	gdk_gc_set_colormap(t_gc, p_colormap);
	
	GdkColor t_color;
	t_color . red = p_red;
	t_color . green = p_green;
	t_color . blue = p_blue;
	
	gdk_gc_set_rgb_fg_color(t_gc, &t_color);
	gdk_draw_rectangle(p_drawable, t_gc, TRUE, 0, 0, p_width, p_height);
	g_object_unref(t_gc);
}

static GdkPixbuf* drawtheme_calc_alpha (MCThemeDrawInfo &p_info)
{
	GdkPixbuf *t_pb_black;
    GdkPixbuf *t_pb_white;
    
	GdkPixmap *t_black ;
	GdkPixmap *t_white ;

	GdkColormap *cm ;
	GdkVisual *best_vis ;
	
	uint4	t_w ;
	uint4	t_h ;	
		
	t_w = p_info.drect.width ;
	t_h = p_info.drect.height ;

	// MM-2013-11-06: [[ Bug 11360 ]] Make sure we take into account the screen depth when creating pixmaps.
	uint4 t_screen_depth;
	t_screen_depth = ((MCScreenDC*) MCscreen) -> getdepth();
	
	// Create two new pixmaps
	t_black = gdk_pixmap_new(NULL, t_w, t_h, t_screen_depth);
	t_white = gdk_pixmap_new(NULL, t_w, t_h, t_screen_depth);
	
	// We need to attach a colourmap to the Drawables in GDK
	best_vis = gdk_visual_get_best_with_depth(t_screen_depth);
    if (best_vis == NULL)
        return NULL;
    
	cm = gdk_colormap_new(best_vis, FALSE) ;
	gdk_drawable_set_colormap(t_black, cm);
	gdk_drawable_set_colormap(t_white, cm);

	// Render solid black into one and white into the other.
	fill_gdk_drawable(t_black, cm, 0, 0, 0, t_w, t_h);
	fill_gdk_drawable(t_white, cm, 65535, 65535, 65535, t_w, t_h);
	
	MCThemeDrawInfo t_info;
	
	t_info = p_info;
	moz_gtk_widget_paint ( p_info.moztype, t_white , &t_info.drect, &t_info.cliprect, &t_info.state, t_info.flags ) ;
	
	t_info = p_info;
	moz_gtk_widget_paint ( p_info.moztype, t_black , &t_info.drect, &t_info.cliprect, &t_info.state, t_info.flags ) ;

	gdk_flush();
	
    // Convert the server-side pixmaps into client-side pixbufs. The black
    // pixbuf will need to have an alpha channel so that we can fill it in.
    t_pb_black = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, t_w, t_h);
    if (t_pb_black == NULL)
        return NULL;
        
    t_pb_white = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, t_w, t_h);
    if (t_pb_white == NULL)
        return NULL;
    
    t_pb_black = gdk_pixbuf_get_from_drawable(t_pb_black, t_black, NULL, 0, 0, 0, 0, t_w, t_h);
    if (t_pb_black == NULL)
        return NULL;
    
    t_pb_white = gdk_pixbuf_get_from_drawable(t_pb_white, t_white, NULL, 0, 0, 0, 0, t_w, t_h);
    if (t_pb_white == NULL)
        return NULL;
    
	// Calculate the alpha from these two bitmaps --- the t_bm_black image now has full ARGB
    // Note that this also frees the t_pb_white pixbuf
	calc_alpha_from_pixbufs(t_pb_black, t_pb_white);
	
	// clean up.
	g_object_unref(t_black);
	g_object_unref(t_white);
	g_object_unref(cm);
		
	return t_pb_black;
}

bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info)
{
	MCXImageCacheNode *cache_node = NULL ;
	GdkPixbuf* t_argb_image ;
	bool t_cached ;
    
	if ( ( p_info -> moztype != MOZ_GTK_CHECKBUTTON ) && ( p_info -> moztype != MOZ_GTK_RADIOBUTTON ) )
		cache_node = MCimagecache -> find_cached_image ( p_info -> drect.width, p_info -> drect.height, p_info -> moztype, &p_info -> state, p_info -> flags ) ;
	
	if ( cache_node != NULL ) 
	{
		t_argb_image = MCimagecache -> get_from_cache( cache_node ) ;
		t_cached = true ;
    }
	else
	{
		// Calculate the alpha for the rendered widget, by rendering against white & black.
		t_argb_image = drawtheme_calc_alpha (*p_info) ;
        if (t_argb_image == NULL)
            return false;
        t_cached = MCimagecache -> add_to_cache (t_argb_image, *p_info) ;
    }

    MCGRaster t_raster;
	t_raster.width = gdk_pixbuf_get_width(t_argb_image);
	t_raster.height = gdk_pixbuf_get_height(t_argb_image);
	t_raster.stride = gdk_pixbuf_get_rowstride(t_argb_image);
	t_raster.pixels = gdk_pixbuf_get_pixels(t_argb_image);
	t_raster.format = kMCGRasterFormat_ARGB;
	
	MCGRectangle t_dest;
	t_dest.origin.x = p_info->crect.x;
	t_dest.origin.y = p_info->crect.y;
	t_dest.size.width = p_info->crect.width;
	t_dest.size.height = p_info->crect.height;

	// MM-2013-12-16: [[ Bug 11567 ]] Use bilinear filter when drawing theme elements.
    // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was bilinear).
	MCGContextDrawPixels(p_context, t_raster, t_dest, kMCGImageFilterMedium);
	
    if (!t_cached)
        g_object_unref(t_argb_image);
    
	return true;
}

////////////////////////////////////////////////////////////////////////////////
