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

#ifndef __MC_LINUX_THEME__
#define __MC_LINUX_THEME__

#ifndef __MC_THEME__
#include "mctheme.h"
#endif

struct MCThemeDrawInfo
{
	GtkThemeWidgetType moztype ;
	GdkPixmap *pm ;
	GdkRectangle drect ;
	GdkRectangle cliprect ;
	GtkWidgetState state ;
	gint flags ;
	MCRectangle crect;
};

class MCNativeTheme: public MCTheme
{
public:
	Boolean load();
	uint2 getthemeid();
	uint2 getthemefamilyid();
	Boolean iswidgetsupported(Widget_Type wtype);
	virtual int4 getmetric(Widget_Metric wmetric);
	int4 getwidgetmetric(const MCWidgetInfo &winfo,Widget_Metric wmetric);
	virtual void getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect,
	                           MCRectangle &drect);
	Boolean getthemepropbool(Widget_ThemeProps themeprop);
	Boolean drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &d);
	Widget_Part hittest(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect);
	void unload();

	// MW-2011-09-14: [[ Bug 9719 ]] Override to return the actual sizeof(MCThemeDrawInfo).
	virtual uint32_t getthemedrawinfosize(void);

protected:
	void make_theme_info(MCThemeDrawInfo& ret, GtkThemeWidgetType widget, 
						 GdkDrawable * drawable,
						 GdkRectangle * rect, 
						 GdkRectangle * cliprect,
						 GtkWidgetState state, 
						 gint flags,
						 MCRectangle crect );
	
	
	void drawTab(MCDC* t_dc, const MCWidgetInfo &winfo, const MCRectangle &drect,
	             GdkPixmap *pix);
	void drawScrollbar(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
	void drawSlider(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
	void getscrollbarrects(const MCWidgetInfo &winfo,
	                       const MCRectangle &srect,
	                       MCRectangle &sbincarrowrect, MCRectangle &sbdecarrowrect,
	                       MCRectangle &sbthumbrect,
	                       MCRectangle &sbinctrackrect, MCRectangle &sbdectrackrect);
	Boolean drawprogressbar(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
	Widget_Part hittestspinbutton(const MCWidgetInfo &winfo,
	                              int2 mx, int2 my, const MCRectangle &drect);
	Widget_Part hittestscrollcontrols(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect);
	Widget_Part hittestcombobutton(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect);
	
private:
	Boolean mNeedNewGC;
	GdkPixmap *gtkpix;
};

#endif
