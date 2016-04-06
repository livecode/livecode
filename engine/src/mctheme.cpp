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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcstring.h"
#include "globals.h"
#include "mctheme.h"
#include "util.h"
#include "object.h"

#include "context.h"

Boolean MCTheme::load(void)
{
	return False;
}
uint2 MCTheme::getthemeid()
	{
	return 0;
	}
uint2 MCTheme::getthemefamilyid()
{
	return 0;
}
int4 MCTheme::getmetric(Widget_Metric wmetric)
{
	return 0;
}
Boolean MCTheme::iswidgetsupported(Widget_Type wtype)
{
	return False;
}

int4 MCTheme::getwidgetmetric(const MCWidgetInfo &winfo,Widget_Metric wmetric)
{
	return 0;
};

void MCTheme::getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect, MCRectangle &drect)
{
}

Boolean MCTheme::drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &d)
{
	return False;
}

Widget_Part MCTheme::hittest(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect)
{
	return WTHEME_PART_UNDEFINED;
}

Boolean MCTheme::getthemepropbool(Widget_ThemeProps themeprop)
{
	return False;
}
void MCTheme::getthemecolor(const MCWidgetInfo &winfo,Widget_Color ctype, MCStringRef &r_themecolor )
{
	r_themecolor = MCValueRetain(kMCEmptyString);
}

void MCTheme::unload()
{
}

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Bug 9719 ]] Default implementation has an empty MCThemeDrawInfo.
uint32_t MCTheme::getthemedrawinfosize(void)
{
	return 0;
}

int32_t MCTheme::fetchtooltipstartingheight(void)
{
	return 0;
}

bool MCTheme::applythemetotooltipwindow(Window window, const MCRectangle& rect)
{
	return false;
}

bool MCTheme::drawtooltipbackground(MCContext *context, const MCRectangle& rect)
{
	return false;
}

bool MCTheme::settooltiptextcolor(MCContext *context)
{
	return false;
}

bool MCTheme::drawmenubackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool with_gutter)
{
	return false;
}

bool MCTheme::drawmenubarbackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool is_active)
{
	return false;
}

bool MCTheme::drawmenuheaderbackground(MCContext *context, const MCRectangle& dirty, MCButton *button)
{
	return false;
}

bool MCTheme::drawmenuitembackground(MCContext *context, const MCRectangle& dirty, MCButton *button)
{
	return false;
}

bool MCTheme::drawfocusborder(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect)
{
	return false;
}

bool MCTheme::drawmetalbackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, MCObject *object)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
