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

#ifndef __MC_WINDOWS_THEME__
#define __MC_WINDOWS_THEME__

#ifndef __MC_THEME__
#include "mctheme.h"
#endif

struct MCThemeDrawInfo
{
	MCWinSysHandle theme;
	int4 part;
	int4 state;
	MCRectangle bounds;
	MCRectangle clip;
	bool clip_interior;
	MCRectangle interior;
};

class MCNativeTheme: public MCTheme
{
public:
	MCNativeTheme()
	{
		mThemeDLL = NULL;
	}

	MCWinSysHandle getmenutheme(void)
	{
		return mMenuTheme;
	}

	/////////

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

	virtual bool applythemetotooltipwindow(Window window, const MCRectangle& rect);
	virtual bool drawtooltipbackground(MCContext *context, const MCRectangle& rect);
	virtual bool settooltiptextcolor(MCContext *context);
	virtual int32_t fetchtooltipstartingheight(void);

	virtual bool drawmenubackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool with_gutter);
	virtual bool drawmenubarbackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool is_active);
	virtual bool drawmenuheaderbackground(MCContext *context, const MCRectangle& dirty, MCButton *button);
	virtual bool drawmenuitembackground(MCContext *context, const MCRectangle& dirty, MCButton *button);

protected:
	virtual void getthemecolor(const MCWidgetInfo &winfo, Widget_Color ctype, MCStringRef &r_colorbuf);
	MCWinSysHandle GetTheme(Widget_Type wtype);
	void CloseData();
	Boolean GetThemePartAndState(const MCWidgetInfo &winfo, int4& aPart, int4& aState);
	Boolean drawscrollcontrols(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
	Boolean drawprogressbar(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);

	Boolean drawslider(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
	void getsliderrects(const MCWidgetInfo &winfo, const MCRectangle &srect,
	                    MCRectangle &sbincarrowrect,MCRectangle &sbdecarrowrect, MCRectangle &sbthumbrect,
	                    MCRectangle &sbinctrackrect,MCRectangle &sbdectrackrect);
	void getscrollbarrects(const MCWidgetInfo &winfo, const MCRectangle &srect,
	                       MCRectangle &sbincarrowrect,MCRectangle &sbdecarrowrect, MCRectangle &sbthumbrect,
	                       MCRectangle &sbinctrackrect,MCRectangle &sbdectrackrect);
	Widget_Part hittestscrollcontrols(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect);
	void dodrawtheme(MCSysContextHandle p_dc, MCSysContextHandle p_mask_dc, MCThemeDrawType p_type, MCThemeDrawInfo& p_info);
	
	MCSysModuleHandle mThemeDLL;
	MCWinSysHandle mButtonTheme;
	MCWinSysHandle mTextFieldTheme;
	MCWinSysHandle mTooltipTheme;
	MCWinSysHandle mToolbarTheme;
	MCWinSysHandle mRebarTheme;
	MCWinSysHandle mProgressTheme;
	MCWinSysHandle mScrollbarTheme;
	MCWinSysHandle mSmallScrollbarTheme;
	MCWinSysHandle mStatusbarTheme;
	MCWinSysHandle mTabTheme;
	MCWinSysHandle mTreeViewTheme;
	MCWinSysHandle mComboBoxTheme;
	MCWinSysHandle mHeaderTheme;
	MCWinSysHandle mSliderTheme;
	MCWinSysHandle mMenuTheme;
	MCWinSysHandle mSpinTheme;
};

#endif
