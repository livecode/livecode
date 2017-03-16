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

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcstring.h"
#include "globals.h"
#include "mctheme.h"
#include "util.h"
#include "object.h"
#include "stack.h"

#include "context.h"
#include "osxtheme.h"

#include "graphics_util.h"

#include "globals.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

Boolean MCNativeTheme::load()
{	
	return MCplatform -> LoadTheme();
}

void MCNativeTheme::unload()
{
    MCplatform -> UnloadTheme();
}

Boolean MCNativeTheme::iswidgetsupported(Widget_Type w)
{
    return MCplatform -> IsThemeWidgetSupported(w);
}

int4 MCNativeTheme::getmetric(Widget_Metric wmetric)
{
    return MCplatform -> GetThemeMetric(wmetric);
}

int4 MCNativeTheme::getwidgetmetric(const MCWidgetInfo &winfo, Widget_Metric wmetric)
{
    return MCplatform -> GetThemeWidgetMetric(winfo, wmetric);
}

Boolean MCNativeTheme::getthemepropbool(Widget_ThemeProps themeprop)
{
    return MCplatform -> GetThemePropBool(themeprop);
}


void MCNativeTheme::getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect,
                                  MCRectangle &drect)
{

    MCplatform -> GetThemeWidgetRect(winfo, wmetric, srect, drect);
	MCTheme::getwidgetrect(winfo,wmetric,srect,drect);
}

uint2 MCNativeTheme::getthemeid()
{
	return MCplatform -> GetThemeId();
}

uint2 MCNativeTheme::getthemefamilyid()
{
	return MCplatform -> GetThemeFamilyId();
}

Boolean MCNativeTheme::drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
    return MCplatform -> DrawThemeWidget(dc, winfo, drect);
}

Widget_Part MCNativeTheme::hittest(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect)
{
    return MCplatform -> HitTestTheme(winfo, mx, my, drect);
}

// MW-2011-09-14: [[ Bug 9719 ]] Override to return the actual sizeof(MCThemeDrawInfo).
uint32_t MCNativeTheme::getthemedrawinfosize(void)
{
	return sizeof(MCThemeDrawInfo);
}

bool MCNativeTheme::drawfocusborder(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect)
{
    return MCplatform -> DrawThemeFocusBorder(p_context, p_dirty, p_rect);
}

bool MCNativeTheme::drawmetalbackground(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect, MCObject *p_object)
{
    return MCplatform -> DrawThemeMetalBackground(p_context, p_dirty, p_rect, p_object);
}

////////////////////////////////////////////////////////////////////////////////

MCTheme *MCThemeCreateNative(void)
{
	return new MCNativeTheme;
}

////////////////////////////////////////////////////////////////////////////////


bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr)
{
    return MCplatform -> DrawTheme(p_context, p_type, p_info_ptr);
}

////////////////////////////////////////////////////////////////////////////////
