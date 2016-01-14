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

#ifndef __MC_OSX_THEME__
#define __MC_OSX_THEME__

#ifndef __MC_THEME__
#include "mctheme.h"
#endif

struct MCThemeDrawInfo
{
	MCRectangle dest;
	union
	{
		struct
		{
			HIThemeTrackDrawInfo info;
			ItemCount count;
		} slider;
		
		struct
		{
			HIThemeTrackDrawInfo info;
		} progress;
		
		struct
		{
			HIThemeTrackDrawInfo info;
			Boolean horizontal;
		} scrollbar;
		
		struct
		{
			HIRect bounds;
			HIThemeButtonDrawInfo info;
		} button;
		
		struct
		{
			Rect bounds;
			ThemeDrawState state;
			bool is_filled;
			bool is_secondary;
		} group;
		
		struct
		{
			Rect bounds;
			ThemeDrawState state;
			bool is_list;
		} frame;
		
		struct
		{
			Rect bounds;
			ThemeDrawState state;
		} tab_pane, background;
		
		struct
		{
			Rect bounds;
			bool is_disabled;
			bool is_hilited;
			bool is_pressed;
			bool is_first;
			bool is_last;
		} tab;
		
		struct
		{
			Rect bounds;
			Boolean focused;
		} focus_rect;
	};
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

	bool drawfocusborder(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect);
	bool drawmetalbackground(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect, MCObject *p_object);
};

#endif
