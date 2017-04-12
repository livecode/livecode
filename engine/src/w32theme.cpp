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
#include "mctheme.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "context.h"
#include "button.h"

#include "w32dc.h"
#include "w32theme.h"

#include "exec.h"
#include "graphics_util.h"

// The header contains nothing without this define for the Win7 SDK
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600

#include <uxtheme.h>

////////////////////////////////////////////////////////////////////////////////

// Generic state constants
#define TS_NORMAL    1
 #define TS_HOVER     2
 #define TS_ACTIVE    3
 #define TS_DISABLED  4
 #define TS_FOCUSED   5
#define TS_CONTROL_HOVER 5 // 'Hover' state for scrollbars

// Button constants
#define BP_BUTTON    1
 #define BP_RADIO     2
 #define BP_CHECKBOX  3
#define BP_GROUPBOX 4

// Textfield constants
#define TFP_TEXTFIELD 1
 #define TFS_READONLY  6

// Treeview/listbox constants
#define TREEVIEW_BODY 1

// Scrollbar constants
#define SP_BUTTON          1
 #define SP_THUMBHOR        2
 #define SP_THUMBVERT       3
 #define SP_TRACKSTARTHOR   4
 #define SP_TRACKENDHOR     5
 #define SP_TRACKSTARTVERT  6
 #define SP_TRACKENDVERT    7
 #define SP_GRIPPERHOR      8
 #define SP_GRIPPERVERT     9

#define SLIDERP_TRACKHOR 1
#define SLIDERP_TRACKVERT 2
#define SLIDERP_THUMBHOR 4
#define SLIDERP_THUMBVERT 8

#define SLIDERP_THUMBDISABLED 5



// Progress bar constants
#define PP_BAR             1
 #define PP_BARVERT         2
 #define PP_CHUNK           3
 #define PP_CHUNKVERT       4


// Tab constants
#define TABP_TAB             4
#define TABP_TAB_SELECTED    5
#define TABP_PANELS          9
#define TABP_PANEL           9


// Tooltip constants
#define TTP_STANDARD         1

// Dropdown constants
#define CBP_DROPMARKER       1
#define CBP_READONLY		 5
#define CBP_DROPDOWNBUTTONRIGHT 6


enum {
	MENU_MENUITEM_TMSCHEMA = 1,
	MENU_MENUDROPDOWN_TMSCHEMA = 2,
	MENU_MENUBARITEM_TMSCHEMA = 3,
	MENU_MENUBARDROPDOWN_TMSCHEMA = 4,
	MENU_CHEVRON_TMSCHEMA = 5,
	MENU_SEPARATOR_TMSCHEMA = 6,
	MENU_BARBACKGROUND = 7,
	MENU_BARITEM = 8,
	MENU_POPUPBACKGROUND = 9,
	MENU_POPUPBORDERS = 10,
	MENU_POPUPCHECK = 11,
	MENU_POPUPCHECKBACKGROUND = 12,
	MENU_POPUPGUTTER = 13,
	MENU_POPUPITEM = 14,
	MENU_POPUPSEPARATOR = 15,
	MENU_POPUPSUBMENU = 16,
	MENU_SYSTEMCLOSE = 17,
	MENU_SYSTEMMAXIMIZE = 18,
	MENU_SYSTEMMINIMIZE = 19,
	MENU_SYSTEMRESTORE = 20,
};

enum {
	MB_ACTIVE = 1,
	MB_INACTIVE = 2,
};

enum {
	MBI_NORMAL = 1,
	MBI_HOT = 2,
	MBI_PUSHED = 3,
	MBI_DISABLED = 4,
	MBI_DISABLEDHOT = 5,
	MBI_DISABLEDPUSHED = 6,
};

enum {
	MC_CHECKMARKNORMAL = 1,
	MC_CHECKMARKDISABLED = 2,
	MC_BULLETNORMAL = 3,
	MC_BULLETDISABLED = 4,
};

enum {
	MCB_DISABLED = 1,
	MCB_NORMAL = 2,
	MCB_BITMAP = 3,
};

enum {
	MPI_NORMAL = 1,
	MPI_HOT = 2,
	MPI_DISABLED = 3,
	MPI_DISABLEDHOT = 4,
};

enum {
	MSM_NORMAL = 1,
	MSM_DISABLED = 2,
};

typedef HANDLE HPAINTBUFFER;

typedef HANDLE (WINAPI*OpenThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI*CloseThemeDataPtr)(HANDLE hTheme);
typedef HRESULT (WINAPI*DrawThemeBackgroundPtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, const RECT *pRect,
        const RECT* pClipRect);
typedef HRESULT (WINAPI*GetThemeContentRectPtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, const RECT* pRect,
        RECT* pContentRect);
typedef HRESULT (WINAPI*GetThemePartSizePtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, RECT* prc, int ts,
        SIZE* psz);
typedef HRESULT (WINAPI*GetThemeFontPtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, int iPropId, OUT LOGFONT* pFont);
typedef HRESULT (WINAPI*GetThemeSysFontPtr)(HANDLE hTheme, int iFontId, OUT LOGFONT* pFont);
typedef HRESULT (WINAPI*GetThemeColorPtr)(HANDLE hTheme, HDC hdc, int iPartId,
        int iStateId, int iPropId, OUT COLORREF* pFont);
typedef HRESULT (WINAPI*GetThemeTextMetricsPtr)(HANDLE hTheme, OPTIONAL HDC hdc, int iPartId,
        int iStateId, OUT TEXTMETRICA* ptm);
typedef HRESULT (WINAPI *GetThemeBackgroundRegionPtr)(HANDLE hTheme, OPTIONAL HDC hdc, int iPartId,
				int iStateId, const RECT *pRect, HRGN *pRegion);
typedef HRESULT (WINAPI *DrawThemeBackgroundExPtr)(HANDLE hTheme, HDC hdc, 
    int iPartId, int iStateId, const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions);
typedef HPAINTBUFFER (WINAPI *BeginBufferedPaintPtr)(HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat,
												BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (WINAPI *EndBufferedPaintPtr)(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);
typedef HRESULT (WINAPI *BufferedPaintClearPtr)(HPAINTBUFFER hBufferedPaint, const RECT *prc);
typedef HRESULT (WINAPI *GetBufferedPaintBitsPtr)(HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer, int *pcxRow);

static OpenThemeDataPtr openTheme = NULL;
static CloseThemeDataPtr closeTheme = NULL;
static DrawThemeBackgroundPtr drawThemeBG = NULL;
static DrawThemeBackgroundExPtr drawThemeBGEx = NULL;
static GetThemeContentRectPtr getThemeContentRect = NULL;
static GetThemePartSizePtr getThemePartSize = NULL;
static GetThemeFontPtr getThemeFont = NULL;
static GetThemeSysFontPtr getThemeSysFont = NULL;
static GetThemeColorPtr getThemeColor = NULL;
static GetThemeTextMetricsPtr getThemeTextMetrics = NULL;
static GetThemeBackgroundRegionPtr getThemeBackgroundRegion = NULL;
static BeginBufferedPaintPtr beginBufferedPaint = NULL;
static EndBufferedPaintPtr endBufferedPaint = NULL;
static BufferedPaintClearPtr bufferedPaintClear = NULL;
static GetBufferedPaintBitsPtr getBufferedPaintBits = NULL;

#define NMENUCOLORS 4
static MCStringRef menucolors[NMENUCOLORS];


static char menucolorsregs[][255] = {
                                        "HKEY_CURRENT_USER\\Control Panel\\Colors\\MenuText",
                                        "HKEY_CURRENT_USER\\Control Panel\\Colors\\MenuHilight",
                                        "HKEY_CURRENT_USER\\Control Panel\\Colors\\Menu",
                                        "HKEY_CURRENT_USER\\Control Panel\\Colors\\ButtonShadow"
                                    };

#define FIXED_THUMB_SIZE 17

Boolean MCNativeTheme::load()
{
	if (mThemeDLL != NULL)
		return True;
	mThemeDLL = NULL;
	mButtonTheme = NULL;
	mTextFieldTheme = NULL;
	mTooltipTheme = NULL;
	mToolbarTheme = NULL;
	mRebarTheme = NULL;
	mProgressTheme = NULL;
	mScrollbarTheme = NULL;
	mSmallScrollbarTheme = NULL;
	mStatusbarTheme = NULL;
	mTabTheme = NULL;
	mTreeViewTheme = NULL;
	mComboBoxTheme = NULL;
	mSliderTheme = NULL;
	mHeaderTheme = NULL;
	mMenuTheme = NULL;
	mSpinTheme = NULL;
	mThemeDLL = (MCSysModuleHandle)LoadLibraryA("UxTheme.dll");
	if (mThemeDLL == NULL)
		return False;

	openTheme = (OpenThemeDataPtr)GetProcAddress((HMODULE)mThemeDLL, "OpenThemeData");
	closeTheme = (CloseThemeDataPtr)GetProcAddress((HMODULE)mThemeDLL, "CloseThemeData");
	drawThemeBG = (DrawThemeBackgroundPtr)GetProcAddress((HMODULE)mThemeDLL, "DrawThemeBackground");
	getThemeContentRect = (GetThemeContentRectPtr)GetProcAddress((HMODULE)mThemeDLL, "GetThemeBackgroundContentRect");
	getThemePartSize = (GetThemePartSizePtr)GetProcAddress((HMODULE)mThemeDLL, "GetThemePartSize");
	getThemeSysFont = (GetThemeSysFontPtr)GetProcAddress((HMODULE)mThemeDLL, "GetThemeSysFont");
	getThemeColor = (GetThemeColorPtr)GetProcAddress((HMODULE)mThemeDLL, "GetThemeColor");
	getThemeBackgroundRegion = (GetThemeBackgroundRegionPtr)GetProcAddress((HMODULE)mThemeDLL, "GetThemeBackgroundRegion");
	beginBufferedPaint = (BeginBufferedPaintPtr)GetProcAddress((HMODULE)mThemeDLL, "BeginBufferedPaint");
	endBufferedPaint = (EndBufferedPaintPtr)GetProcAddress((HMODULE)mThemeDLL, "EndBufferedPaint");
	bufferedPaintClear = (BufferedPaintClearPtr)GetProcAddress((HMODULE)mThemeDLL, "BufferedPaintClear");
	getBufferedPaintBits = (GetBufferedPaintBitsPtr)GetProcAddress((HMODULE)mThemeDLL, "GetBufferedPaintBits");

	drawThemeBGEx = (DrawThemeBackgroundExPtr)GetProcAddress((HMODULE)mThemeDLL, "DrawThemeBackgroundEx");

	uint2 i;
    MCExecContext ctxt(nil, nil, nil);
	for (i = 0; i < NMENUCOLORS; i++)
	{
		menucolors[i] = nil;

        MCAutoStringRef t_type, t_error, t_string_value;
        MCAutoValueRef t_value;
        if (MCS_query_registry(MCSTR(menucolorsregs[i]), &t_value, &t_type, &t_error)
            && *t_value != nil
            && ctxt . ConvertToMutableString(*t_value, &t_string_value)
            && MCStringFindAndReplaceChar(*t_string_value, ' ', ',', kMCCompareExact))
		{
			/* UNCHECKED */ MCStringCopy(*t_string_value, menucolors[i]);
		}
	}
	
	if (MCmajorosversion >= 0x0600)
		mMenuTheme = (MCWinSysHandle)openTheme(NULL, L"Menu");

	return GetTheme(WTHEME_TYPE_PUSHBUTTON) != NULL;
}


void MCNativeTheme::unload()
{
	if (mThemeDLL == NULL)
		return;
	uint2 i;
	for (i = 0; i < NMENUCOLORS; i++)
	{
		if (menucolors[i] != nil)
		{
			MCValueRelease(menucolors[i]);
			menucolors[i] = nil;
		}
	}
	CloseData();
	if (mThemeDLL)
		FreeLibrary((HMODULE)mThemeDLL);
	mThemeDLL = NULL;
}

MCWinSysHandle MCNativeTheme::GetTheme(Widget_Type wtype)
{
	if (!mThemeDLL)
		return NULL;
	switch (wtype)
	{

	case WTHEME_TYPE_CHECKBOX:
	case WTHEME_TYPE_PUSHBUTTON:
	case WTHEME_TYPE_RADIOBUTTON:
	case WTHEME_TYPE_GROUP_FRAME:
	case WTHEME_TYPE_GROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FRAME:
	case WTHEME_TYPE_SECONDARYGROUP_FILL:
		{
			if (!mButtonTheme)
				mButtonTheme = (MCWinSysHandle)openTheme(NULL, L"Button");
			return mButtonTheme;
		}
	case WTHEME_TYPE_TEXTFIELD:
	case WTHEME_TYPE_TEXTFIELD_FRAME:
	case WTHEME_TYPE_TEXTFIELD_FILL:
	case WTHEME_TYPE_COMBOFRAME:
	case WTHEME_TYPE_COMBOTEXT:
		{
			if (!mTextFieldTheme)
				mTextFieldTheme = (MCWinSysHandle)openTheme(NULL, L"Edit");
			return mTextFieldTheme;
		}
	case WTHEME_TYPE_TOOLTIP:
		{
			if (!mTooltipTheme)
				mTooltipTheme = (MCWinSysHandle)openTheme(NULL, L"Tooltip");
			return mTooltipTheme;
		}
	case WTHEME_TYPE_PROGRESSBAR:
	case WTHEME_TYPE_PROGRESSBAR_HORIZONTAL:
	case WTHEME_TYPE_PROGRESSBAR_VERTICAL:
	case WTHEME_TYPE_PROGRESSBAR_CHUNK:
	case WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL:
		{
			if (!mProgressTheme)
				mProgressTheme = (MCWinSysHandle)openTheme(NULL, L"Progress");
			return mProgressTheme;
		}
	case WTHEME_TYPE_TAB:
	case WTHEME_TYPE_TABPANE:
		{
			if (!mTabTheme)
				mTabTheme = (MCWinSysHandle)openTheme(NULL, L"Tab");
			return mTabTheme;
		}

	case WTHEME_TYPE_SMALLSCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_UP:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_DOWN:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_LEFT:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_RIGHT:

		{
			if (!mSmallScrollbarTheme)
				mSmallScrollbarTheme = (MCWinSysHandle)openTheme(NULL, L"Spin");
			return mSmallScrollbarTheme;
		}
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_UP:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT:
	case WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL:
		{
			if (!mScrollbarTheme)
				mScrollbarTheme = (MCWinSysHandle)openTheme(NULL, L"Scrollbar");
			return mScrollbarTheme;
		}
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL:
	case WTHEME_TYPE_SLIDER_TRACK_VERTICAL:
	case WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL:
	case WTHEME_TYPE_SLIDER_THUMB_VERTICAL:
		{
			if (!mSliderTheme)
				mSliderTheme = (MCWinSysHandle)openTheme(NULL, L"Trackbar");
			return mSliderTheme;
		}
	case WTHEME_TYPE_OPTIONBUTTON:
	case WTHEME_TYPE_OPTIONBUTTONTEXT:
	case WTHEME_TYPE_OPTIONBUTTONARROW:
		if (MCmajorosversion < 0x0600)
			return NULL;
	case WTHEME_TYPE_COMBOBUTTON:
	case WTHEME_TYPE_COMBO:
		{
			if (!mComboBoxTheme)
				mComboBoxTheme = (MCWinSysHandle)openTheme(NULL, L"Combobox");
			return mComboBoxTheme;
		}
	case WTHEME_TYPE_TREEVIEW_HEADER_CELL:
	case WTHEME_TYPE_TREEVIEW_HEADER_SORTARROW:
		{
			if (!mHeaderTheme)
				mHeaderTheme = (MCWinSysHandle)openTheme(NULL, L"Header");
			return mHeaderTheme;
		}
	case WTHEME_TYPE_LISTBOX:
	case WTHEME_TYPE_LISTBOX_LISTITEM:
	case WTHEME_TYPE_TREEVIEW:
	case WTHEME_TYPE_TREEVIEW_TWISTY_OPEN:
	case WTHEME_TYPE_TREEVIEW_TREEITEM:
		{
			if (!mTreeViewTheme)
				mTreeViewTheme = (MCWinSysHandle)openTheme(NULL, L"Listview");
			return mTreeViewTheme;
		}
	case WTHEME_TYPE_SPIN:
		{
			if (!mSpinTheme)
				mSpinTheme = (MCWinSysHandle)openTheme(NULL, L"Spin");
			return mSpinTheme;
		}
	}
	return NULL;
}

void MCNativeTheme::CloseData()
{
	if (mToolbarTheme)
	{
		closeTheme(mToolbarTheme);
		mToolbarTheme = NULL;
	}
	if (mScrollbarTheme)
	{
		closeTheme(mScrollbarTheme);
		mScrollbarTheme = NULL;
	}
	if (mSmallScrollbarTheme)
	{
		closeTheme(mSmallScrollbarTheme);
		mSmallScrollbarTheme = NULL;
	}
	if (mRebarTheme)
	{
		closeTheme(mRebarTheme);
		mRebarTheme = NULL;
	}
	if (mProgressTheme)
	{
		closeTheme(mProgressTheme);
		mProgressTheme = NULL;
	}
	if (mButtonTheme)
	{
		closeTheme(mButtonTheme);
		mButtonTheme = NULL;
	}
	if (mTextFieldTheme)
	{
		closeTheme(mTextFieldTheme);
		mTextFieldTheme = NULL;
	}
	if (mTooltipTheme)
	{
		closeTheme(mTooltipTheme);
		mTooltipTheme = NULL;
	}
	if (mSliderTheme)
	{
		closeTheme(mSliderTheme);
		mTooltipTheme = NULL;
	}
	if (mStatusbarTheme)
	{
		closeTheme(mStatusbarTheme);
		mStatusbarTheme = NULL;
	}
	if (mTabTheme)
	{
		closeTheme(mTabTheme);
		mTabTheme = NULL;
	}
	if (mTreeViewTheme)
	{
		closeTheme(mTreeViewTheme);
		mTreeViewTheme = NULL;
	}
	if (mComboBoxTheme)
	{
		closeTheme(mComboBoxTheme);
		mComboBoxTheme = NULL;
	}
	if (mHeaderTheme)
	{
		closeTheme(mHeaderTheme);
		mHeaderTheme = NULL;
	}
}

Boolean MCNativeTheme::iswidgetsupported(Widget_Type wtype)
{

	HANDLE htheme = GetTheme(wtype);
	return htheme != NULL;

}

int4 MCNativeTheme::getmetric(Widget_Metric wmetric)
{
	switch (wmetric)
	{
	case WTHEME_METRIC_TABSTARTOFFSET:
		return 2;
		break;
	case WTHEME_METRIC_TABOVERLAP:
		return -1;
		break;
	case WTHEME_METRIC_TABNONSELECTEDOFFSET:
		return 2;
		break;
	case WTHEME_METRIC_TABRIGHTMARGIN:
		return 12;
	case WTHEME_METRIC_TABLEFTMARGIN:
		return 11;
		break;
	case WTHEME_METRIC_COMBOSIZE:
		return -1;
		break;
	// MH-2007-03-16 [[ Bug 3598 ]] Adding in support for option menu button arrow size for appropriate clipping.
	case WTHEME_METRIC_OPTIONBUTTONARROWSIZE:
		return 20;
		break;
	}
	return 0;
}

int4 MCNativeTheme::getwidgetmetric(const MCWidgetInfo &winfo, Widget_Metric wmetric)
{
	return 0;
}

Boolean MCNativeTheme::getthemepropbool(Widget_ThemeProps themeprop)
{
	if (themeprop == WTHEME_PROP_SUPPORTHOVERING)
		return True;
	return False;
}

Widget_Part MCNativeTheme::hittest(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect)
{

	switch (winfo.type)
	{
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR:
		return hittestscrollcontrols(winfo,mx,my,drect);
		break;
	default:
		return MCU_point_in_rect(drect, mx, my)?WTHEME_PART_ALL:WTHEME_PART_UNDEFINED;
		break;
	}
}



Widget_Part MCNativeTheme::hittestscrollcontrols(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect)
{
	Widget_Part wpart = WTHEME_PART_UNDEFINED;
	MCRectangle sbincarrowrect,sbdecarrowrect, sbthumbrect, sbinctrackrect, sbdectrackrect;
	getscrollbarrects(winfo, drect, sbincarrowrect, sbdecarrowrect, sbthumbrect,sbinctrackrect,sbdectrackrect);
	if (MCU_point_in_rect(sbincarrowrect, mx, my))
		wpart = WTHEME_PART_ARROW_INC;
	else if (MCU_point_in_rect(sbinctrackrect, mx, my))
		wpart = WTHEME_PART_TRACK_INC;
	else if (MCU_point_in_rect(sbthumbrect, mx, my))
		wpart = WTHEME_PART_THUMB;
	else if (MCU_point_in_rect(sbdectrackrect, mx, my))
		wpart = WTHEME_PART_TRACK_DEC;
	else if (MCU_point_in_rect(sbdecarrowrect, mx, my))
		wpart = WTHEME_PART_ARROW_DEC;

	bool t_vertical;
	t_vertical = (winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0;
	if ((t_vertical && drect . width * 2 >= drect . height) || (!t_vertical && drect . height * 2 >= drect . width))
	{
		if (wpart == WTHEME_PART_ARROW_INC)
			wpart = WTHEME_PART_ARROW_DEC;
		else if (wpart == WTHEME_PART_ARROW_DEC)
			wpart = WTHEME_PART_ARROW_INC;
	}

	return wpart;
}


void MCNativeTheme::getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect,
                                  MCRectangle &drect)
{
	if (wmetric == WTHEME_METRIC_PARTSIZE || wmetric == WTHEME_METRIC_CONTENTSIZE && winfo.type == WTHEME_TYPE_COMBOTEXT && winfo . part == WTHEME_PART_COMBOTEXT)
	{
		switch (winfo.type)
		{
		case WTHEME_TYPE_SCROLLBAR:
		case WTHEME_TYPE_PROGRESSBAR:
		case WTHEME_TYPE_SLIDER:
			{
				MCRectangle sbincarrowrect,sbdecarrowrect, sbthumbrect, sbinctrackrect, sbdectrackrect;
				getscrollbarrects(winfo, srect, sbincarrowrect, sbdecarrowrect, sbthumbrect,sbinctrackrect,sbdectrackrect);
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
				}
				return;
			}
		case WTHEME_TYPE_COMBO:
		case WTHEME_TYPE_COMBOTEXT:
			{
				MCRectangle combobuttonrect = srect;
				MCWidgetInfo twinfo = winfo;
				twinfo.type = WTHEME_TYPE_COMBOBUTTON;
				uint1 comboframesize = 1;//we should query comboframe value
				int4 t_button_size;
				t_button_size = srect . height - 2;

				combobuttonrect.x = srect . x + srect . width - comboframesize - t_button_size;
				combobuttonrect.y = srect . y + 1 + ((srect . height - 2) - t_button_size) / 2;
				combobuttonrect.width = t_button_size;
				combobuttonrect.height = t_button_size;
				if (winfo.part == WTHEME_PART_COMBOTEXT)
				{
					if (wmetric == WTHEME_METRIC_CONTENTSIZE)
					{
						drect = MCU_reduce_rect(srect,comboframesize);
						drect.width -= combobuttonrect . width;
					}
					else
						drect = srect;
				}
				else if (winfo.part == WTHEME_PART_COMBOBUTTON)
					drect = combobuttonrect;
				return;
			}
		}
		MCTheme::getwidgetrect(winfo,wmetric,srect,drect);
	}
	else
	{
		RECT trect;
		HANDLE htheme = GetTheme(winfo.type);
		if (!htheme)
			return;
		int4 part, state;
		Boolean res = GetThemePartAndState(winfo, part, state);
		if (!res)
			return;
		SetRect(&trect,srect.x,srect.y,
		        srect.width+srect.x,srect.height+srect.y);
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		HDC tdc = pms->getsrchdc();
		if (!tdc)
			return;
		if (wmetric == WTHEME_METRIC_CONTENTSIZE)
		{
			RECT contentrect;
			if (getThemeContentRect && getThemeContentRect(htheme, tdc, part, state, &trect,&contentrect) == S_OK)
			{
				drect.x = (int2)contentrect.left;
				drect.y = (int2)contentrect.top;
				drect.width = uint2(contentrect.right - contentrect.left);
				drect.height = uint2(contentrect.bottom - contentrect.top);
			}
			return;
		}
		else
		{
			THEMESIZE themesize = TS_MIN;
			if (wmetric == WTHEME_METRIC_OPTIMUMSIZE)
				themesize = TS_TRUE;
			else if (wmetric == WTHEME_METRIC_DRAWSIZE)
				themesize = TS_DRAW;
			SIZE tsize;
			if (getThemePartSize && getThemePartSize(htheme, tdc, part, state, &trect, themesize, &tsize) == S_OK)
			{
				drect.x = srect.x;
				drect.y = srect.y;
				drect.width = (uint2)tsize.cx;
				drect.height = (uint2)tsize.cy;
				return;
			}
		}
		MCTheme::getwidgetrect(winfo,wmetric,srect,drect);
	}
}

uint2 MCNativeTheme::getthemeid()
{
	return LF_NATIVEWIN; //it's a native windows theme
}

void MCNativeTheme::getthemecolor(const MCWidgetInfo &winfo, Widget_Color ctype, MCStringRef &r_colorbuf)
{
	if (winfo.type == WTHEME_TYPE_MENU)
	{
		switch (ctype)
		{
            case WCOLOR_TEXT:
                if (menucolors[0] != nil)
                    r_colorbuf = MCValueRetain(menucolors[0]);
                else
                    /* UNCHECKED */ MCStringCreateWithCString("0,0,0", r_colorbuf);
                break;
            case WCOLOR_HILIGHT:
                if (menucolors[1] != nil)
                    r_colorbuf = MCValueRetain(menucolors[1]);
                else
                    /* UNCHECKED */ MCStringCreateWithCString("255,0,0", r_colorbuf);
                break;
            case WCOLOR_BACK:
                if (menucolors[2] != nil)
                    r_colorbuf = MCValueRetain(menucolors[2]);
                else
                    /* UNCHECKED */ MCStringCreateWithCString("255,255,0", r_colorbuf);
                break;
            case WCOLOR_BORDER:
                if (menucolors[3] != nil)
                    r_colorbuf = MCValueRetain(menucolors[3]);
                else
                    /* UNCHECKED */ MCStringCreateWithCString("155,155,155", r_colorbuf);
                break;
		}
	}
    else
        r_colorbuf = MCValueRetain(kMCEmptyString);
}




uint2 MCNativeTheme::getthemefamilyid()
{
	return LF_WIN95; //however it belongs to the win32 theme family
}

Boolean MCNativeTheme::drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	HANDLE htheme = GetTheme(winfo.type);
	if (!htheme)
		return False;
	if (!drawThemeBG)
		return False;
	int4 part, state;
	Boolean res = GetThemePartAndState(winfo, part, state);
	if (!res)
		return False;
		
	MCRectangle crect = drect;
	MCRectangle trect = drect;
	bool t_clip_interior = false;
	MCRectangle t_interior;

	switch (winfo.type)
	{
	case WTHEME_TYPE_COMBO:
		{
			MCWidgetInfo twinfo = winfo;
			MCRectangle comboentryrect,combobuttonrect;
			//draw text box
			twinfo.part = WTHEME_PART_COMBOTEXT;
			getwidgetrect(twinfo, WTHEME_METRIC_PARTSIZE,drect,comboentryrect);
			twinfo.part = WTHEME_PART_COMBOBUTTON;
			getwidgetrect(twinfo, WTHEME_METRIC_PARTSIZE,drect,combobuttonrect);

			twinfo.type = WTHEME_TYPE_COMBOBUTTON;
			drawwidget(dc, twinfo, combobuttonrect);
			
			twinfo.type = WTHEME_TYPE_COMBOTEXT;

			drawwidget(dc, twinfo, comboentryrect);
			return True;
		}

	case WTHEME_TYPE_OPTIONBUTTON:
		{
			MCWidgetInfo twinfo = winfo;
			MCRectangle arrowrect;
			
			MCU_set_rect(arrowrect, trect . x + trect . width - 4 - 7 - 4 - 2, trect . y, 4 + 7 + 4, trect . height);

			twinfo.type = WTHEME_TYPE_OPTIONBUTTONTEXT;
			drawwidget(dc, twinfo, trect);

			twinfo.type = WTHEME_TYPE_OPTIONBUTTONARROW;
			drawwidget(dc, twinfo, arrowrect);
			return True;
		}

	case WTHEME_TYPE_TAB:
		{
			if (winfo.attributes & WTHEME_ATT_TABFIRSTSELECTED &&
			        winfo.state & WTHEME_STATE_HILITED)
			{
				//align to edge of pane
				trect.x -= 2;
				trect.height++;
				;
				trect.width +=2;
				crect = trect;
				crect.height--;
			}
			if (!(winfo.state & WTHEME_STATE_HILITED))
				crect.height--; //make sure non hilited tab doesn't draw over tab pane
			if (winfo.attributes & WTHEME_ATT_TABRIGHTEDGE ||
			        winfo.attributes & WTHEME_ATT_TABLEFTEDGE)
			{
				//hardcoded to make it draw correctly
				uint1 edgeSize = 2;
				// Armed with the size of the edge, we now need to either shift to the left or to the
				// right.  The clip rect won't include this extra area, so we know that we're
				// effectively shifting the edge out of view (such that it won't be painted).
				if (winfo.attributes & WTHEME_ATT_TABLEFTEDGE)
					// The right edge should not be drawn.  Extend our rect by the edge size.
					trect.width += edgeSize;
				else
				{
					// The left edge should not be drawn.  Move the widget rect's left coord back.
					trect.x -= edgeSize;
					trect.width += edgeSize;
				}
			}
			break;
		}
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR:
		return drawscrollcontrols(dc, winfo, drect);
		break;
	case WTHEME_TYPE_PROGRESSBAR:
		return drawprogressbar(dc, winfo, drect);
		break;
	case WTHEME_TYPE_SLIDER:
		return drawslider(dc, winfo, drect);
		break;
	case WTHEME_TYPE_COMBOTEXT:
	case WTHEME_TYPE_TEXTFIELD_FRAME:
	case WTHEME_TYPE_TEXTFIELD_FILL:
		//this is used to draw border of text fields and not contents..
		//there is a drawthemebackex call which can do this, but only in win2000!.
		MCRectangle tfcontentrect;
		getwidgetrect(winfo,WTHEME_METRIC_CONTENTSIZE,crect,tfcontentrect);
		if (winfo . type == WTHEME_TYPE_TEXTFIELD_FRAME || winfo . type == WTHEME_TYPE_COMBOTEXT)
		{
			t_clip_interior = true;
			t_interior = tfcontentrect;

			// MW-2007-07-05: [[ Bug 2508 ]] - Missing single pixel border on XP fields
			t_interior = MCU_reduce_rect(t_interior, 1);
		}
		else
			crect = tfcontentrect;
		break;
	case WTHEME_TYPE_SECONDARYGROUP_FRAME:
	case WTHEME_TYPE_GROUP_FRAME:
	case WTHEME_TYPE_GROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FILL:
		if (winfo . datatype == WTHEME_DATA_RECT)
		{
			t_clip_interior = true;
			t_interior = *(MCRectangle *)winfo . data;
		}
		break;
	}

	MCThemeDrawInfo t_info;
	t_info . theme = (MCWinSysHandle)htheme;
	t_info . part = part;
	t_info . state = state;
	t_info . bounds = trect;
	t_info . clip = crect;
	t_info . clip_interior = t_clip_interior;
	if (t_clip_interior)
		t_info . interior = t_interior;
	dc -> drawtheme(THEME_DRAW_TYPE_BACKGROUND, &t_info);

	return True;
}


Boolean MCNativeTheme::drawprogressbar(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR)
		return False;
	uint4 pbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED? WTHEME_STATE_DISABLED: WTHEME_STATE_CLEAR;
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
	MCWidgetInfo twinfo = winfo;
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL ? WTHEME_TYPE_PROGRESSBAR_VERTICAL:
	              WTHEME_TYPE_PROGRESSBAR_HORIZONTAL;
	twinfo.state = pbpartdefaultstate;
	drawwidget(dc, twinfo, drect);
	MCRectangle progressbarrect;
	getwidgetrect(twinfo,WTHEME_METRIC_CONTENTSIZE,drect,progressbarrect);
	if (progressbarrect.width && progressbarrect.height)
	{
		int2 endpos = 0;
		if (winfo.attributes & WTHEME_ATT_SBVERTICAL)
		{
			endpos = (int2)(sbinfo->thumbpos / (sbinfo->endvalue - sbinfo->startvalue)
			                * (real8)progressbarrect.height) + progressbarrect.y;
			uint2 ty = (progressbarrect.y+progressbarrect.height) - (endpos - progressbarrect.y);
			progressbarrect.height =  endpos - progressbarrect.y;
			progressbarrect.y = ty;
		}
		else
		{
			endpos = (int2)(sbinfo->thumbpos / (sbinfo->endvalue - sbinfo->startvalue)
			                * (real8)progressbarrect.width) + progressbarrect.x;
			progressbarrect.width =  endpos - progressbarrect.x;
		}
		twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL ? WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL:
		              WTHEME_TYPE_PROGRESSBAR_CHUNK;
		twinfo.state = pbpartdefaultstate;
		drawwidget(dc, twinfo, progressbarrect);
	}
	return True;
}




Boolean MCNativeTheme::drawslider(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR &&
	        winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return False;
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
	//draw arrows
	MCWidgetInfo twinfo = winfo;
	MCRectangle sbincarrowrect,sbdecarrowrect, sbthumbrect, sbinctrackrect, sbdectrackrect;
	getscrollbarrects(winfo, drect, sbincarrowrect, sbdecarrowrect, sbthumbrect,sbinctrackrect,sbdectrackrect);
	uint4 sbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED? WTHEME_STATE_DISABLED: WTHEME_STATE_CLEAR;
	memset(&twinfo,0,sizeof(MCWidgetInfo)); //clear widget info
	//draw upper and lower tracks first
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL ? WTHEME_TYPE_SLIDER_TRACK_VERTICAL: WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL;
	twinfo.state = sbpartdefaultstate;
	if (winfo.part == WTHEME_PART_TRACK_DEC || winfo.part == WTHEME_PART_TRACK_INC)
		twinfo.state = winfo.state;
	MCRectangle trect;
	getwidgetrect(twinfo,WTHEME_METRIC_OPTIMUMSIZE,drect,trect);
	if (winfo.attributes & WTHEME_ATT_SBVERTICAL)
	{
		trect.height = drect.height;
		trect.x += (sbthumbrect.width - trect.width) >> 1;
	}
	else
	{
		trect.width = drect.width;
		trect.y += (sbthumbrect.height - trect.height) >> 1;
	}
	drawwidget(dc, twinfo, trect);
	twinfo.state = sbpartdefaultstate;
	twinfo.type = winfo.attributes & WTHEME_ATT_SBVERTICAL ? WTHEME_TYPE_SLIDER_THUMB_VERTICAL: WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL;
	if (winfo.part == WTHEME_PART_THUMB)
		twinfo.state = winfo.state;
	drawwidget(dc, twinfo, sbthumbrect);

	return True;
}

Boolean MCNativeTheme::drawscrollcontrols(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR &&
	        winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return False;
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;

	//draw arrows
	MCWidgetInfo twinfo = winfo;
	MCRectangle sbincarrowrect, sbdecarrowrect, sbthumbrect, sbinctrackrect, sbdectrackrect;
	getscrollbarrects(winfo, drect, sbincarrowrect, sbdecarrowrect, sbthumbrect,sbinctrackrect,sbdectrackrect);

	uint4 sbpartdefaultstate = winfo.state & WTHEME_STATE_DISABLED ? WTHEME_STATE_DISABLED: WTHEME_STATE_CLEAR;
	if ((winfo . state & WTHEME_STATE_CONTROL_HOVER) != 0 && MCmajorosversion >= 0x0600)
		sbpartdefaultstate |= WTHEME_STATE_CONTROL_HOVER;
	
	memset(&twinfo,0,sizeof(MCWidgetInfo)); //clear widget info

	bool t_vertical;
	t_vertical = (winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0;

	if ((t_vertical && drect . width * 2 >= drect . height) || (!t_vertical && drect . height * 2 >= drect . width))
	{
		twinfo . type = WTHEME_TYPE_SPIN;
		twinfo . part = t_vertical ? WTHEME_PART_SPIN_ARROW_UP : WTHEME_PART_SPIN_ARROW_LEFT;
		twinfo . state = (winfo . part == WTHEME_PART_ARROW_DEC ? winfo . state : sbpartdefaultstate);
		drawwidget(dc, twinfo, sbincarrowrect);

		twinfo . type = WTHEME_TYPE_SPIN;
		twinfo . part = t_vertical ? WTHEME_PART_SPIN_ARROW_DOWN : WTHEME_PART_SPIN_ARROW_RIGHT;
		twinfo . state = (winfo . part == WTHEME_PART_ARROW_INC ? winfo . state : sbpartdefaultstate);
		drawwidget(dc, twinfo, sbdecarrowrect);
	}
	else
	{
		twinfo . type = t_vertical ? WTHEME_TYPE_SCROLLBAR_BUTTON_UP : WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT;
		twinfo . state = winfo . part == WTHEME_PART_ARROW_DEC ? winfo . state : sbpartdefaultstate;
		drawwidget(dc, twinfo, sbdecarrowrect);

		twinfo . type = t_vertical ? WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL : WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL;
		twinfo . part = WTHEME_PART_TRACK_DEC;
		twinfo . state = winfo . part == WTHEME_PART_TRACK_DEC ? winfo . state : sbpartdefaultstate;
		drawwidget(dc, twinfo, sbdectrackrect);

		if ((sbthumbrect . height != 0 && sbthumbrect . width != 0) || (winfo . state & WTHEME_STATE_DISABLED) == 0)
		{
			twinfo . type =  t_vertical ? WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL : WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL;
			twinfo . state = winfo . part == WTHEME_PART_THUMB ? winfo . state : sbpartdefaultstate;
			drawwidget(dc, twinfo, sbthumbrect);

			MCRectangle sbgripperrect;
			twinfo.type = t_vertical ? WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL : WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL;
			twinfo.state = sbpartdefaultstate;
			getwidgetrect(twinfo, WTHEME_METRIC_OPTIMUMSIZE, sbthumbrect, sbgripperrect);

			sbgripperrect . x += (sbthumbrect . width - sbgripperrect . width) / 2;
			sbgripperrect . y += (sbthumbrect . height - sbgripperrect . height) / 2;

			if (t_vertical && sbgripperrect . y > sbthumbrect . y || !t_vertical && sbgripperrect . x > sbthumbrect . x)
				drawwidget(dc, twinfo, sbgripperrect);
		}

		twinfo . type = t_vertical ? WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL : WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL;
		twinfo . part = WTHEME_PART_TRACK_INC;
		twinfo . state = winfo . part == WTHEME_PART_TRACK_INC ? winfo . state : sbpartdefaultstate;
		drawwidget(dc, twinfo, sbinctrackrect);

		twinfo . type = t_vertical ? WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN : WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT;
		twinfo . state = winfo . part == WTHEME_PART_ARROW_INC ? winfo . state : sbpartdefaultstate;
		drawwidget(dc, twinfo, sbincarrowrect);
	}

	return True;
}


void MCNativeTheme::getsliderrects(const MCWidgetInfo &winfo, const MCRectangle &srect,
                                   MCRectangle &sbincarrowrect,MCRectangle &sbdecarrowrect, MCRectangle &sbthumbrect,
                                   MCRectangle &sbinctrackrect,MCRectangle &sbdectrackrect)
{

	if (winfo.datatype != WTHEME_DATA_SCROLLBAR)
		return;

	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
	MCWidgetInfo twinfo;
	memset(&twinfo,0,sizeof(MCWidgetInfo));
	
	bool t_vertical;
	t_vertical = (winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0;

	MCU_set_rect(sbdecarrowrect, 0, 0, 0, 0);

	twinfo . type = t_vertical ? WTHEME_TYPE_SLIDER_THUMB_VERTICAL : WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL;
	getwidgetrect(twinfo, WTHEME_METRIC_OPTIMUMSIZE, srect, sbthumbrect);

	int4 t_track_size;
	t_track_size = t_vertical ? srect . height : srect . width;

	int4 t_thumb_size;
	t_thumb_size = t_vertical ? sbthumbrect . height : sbthumbrect . width;

	int4 t_thumb_offset;
	if (sbinfo -> endvalue - sbinfo -> startvalue != 0)
	{
		t_thumb_offset = t_thumb_size / 2 + (int4)floor((t_track_size - t_thumb_size) * (sbinfo -> thumbpos - sbinfo -> startvalue) / (sbinfo -> endvalue - sbinfo -> startvalue));

		if (t_vertical)
			MCU_set_rect(sbthumbrect, srect . x, srect . y + t_thumb_offset - sbthumbrect . height / 2, sbthumbrect . width, sbthumbrect . height);
		else
			MCU_set_rect(sbthumbrect, srect . x + t_thumb_offset - sbthumbrect . width / 2, srect . y, sbthumbrect . width, sbthumbrect . height);
	}
	else
	{
		t_thumb_offset = t_track_size;
		t_thumb_size = 0;
		MCU_set_rect(sbthumbrect, 0, 0, 0, 0);
	}

	if (t_vertical)
	{
		MCU_set_rect(sbdectrackrect, srect . x, srect . y, srect . width, t_thumb_offset - t_thumb_size / 2);
		MCU_set_rect(sbinctrackrect, srect . x, srect . y + t_thumb_offset - t_thumb_size / 2 + t_thumb_size, srect . width, t_track_size - (t_thumb_offset - t_thumb_size / 2 + t_thumb_size));
	}
	else
	{
		MCU_set_rect(sbdectrackrect, srect . x, srect . y, t_thumb_offset - t_thumb_size / 2, srect . height);
		MCU_set_rect(sbinctrackrect, srect . x + t_thumb_offset - t_thumb_size / 2 + t_thumb_size, srect . y, t_track_size - (t_thumb_offset - t_thumb_size / 2 + t_thumb_size), srect . height);
	}

	MCU_set_rect(sbincarrowrect, 0, 0, 0, 0);
}


void MCNativeTheme::getscrollbarrects(const MCWidgetInfo &winfo, const MCRectangle &srect,
                                      MCRectangle &sbincarrowrect,MCRectangle &sbdecarrowrect, MCRectangle &sbthumbrect,
                                      MCRectangle &sbinctrackrect,MCRectangle &sbdectrackrect)
{
	if (winfo.type == WTHEME_TYPE_SLIDER)
	{
		getsliderrects(winfo,srect,sbincarrowrect,sbdecarrowrect,sbthumbrect,sbinctrackrect,sbdectrackrect);
		return;
	}

	if (winfo.datatype != WTHEME_DATA_SCROLLBAR && winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return;

	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;

	if ((winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0 && srect . width * 2 >= srect . height)
	{
		MCU_set_rect(sbincarrowrect, srect . x, srect . y, srect . width, srect . height / 2);
		MCU_set_rect(sbdecarrowrect, srect . x, srect . y + srect . height / 2, srect . width, srect . height / 2);
		MCU_set_rect(sbthumbrect, 0, 0, 0, 0);
		MCU_set_rect(sbinctrackrect, 0, 0, 0, 0);
		MCU_set_rect(sbdectrackrect, 0, 0, 0, 0);
	}
	else if ((winfo . attributes & WTHEME_ATT_SBVERTICAL) == 0 && srect . height * 2 >= srect . width)
	{
		MCU_set_rect(sbincarrowrect, srect . x, srect . y, srect . width / 2, srect . height);
		MCU_set_rect(sbdecarrowrect, srect . x + srect . width / 2, srect . y, srect . width / 2, srect . height);
		MCU_set_rect(sbthumbrect, 0, 0, 0, 0);
		MCU_set_rect(sbinctrackrect, 0, 0, 0, 0);
		MCU_set_rect(sbdectrackrect, 0, 0, 0, 0);
	}
	else
	{
		int4 t_track_size;
		if ((winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0)
			t_track_size = srect . height - 2 * srect . width;
		else
			t_track_size = srect . width - 2 * srect . height;

		int4 t_thumb_size;
		int4 t_thumb_offset;
		if ((winfo.state & WTHEME_STATE_DISABLED) != 0 || (sbinfo -> endvalue - sbinfo -> startvalue) == 0)
		{
			t_thumb_offset = t_track_size;
			t_thumb_size = 0;
		}
		else
		{
			MCRectangle sbminthumbrect;
			MCWidgetInfo twinfo;
			twinfo.type = (winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0 ? WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL : WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL;
			twinfo.state = TS_NORMAL;
			getwidgetrect(twinfo, WTHEME_METRIC_OPTIMUMSIZE, srect, sbminthumbrect);

			int4 t_thumb_minimum_size;
			t_thumb_minimum_size = (winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0 ? sbminthumbrect . height : sbminthumbrect . width;

			t_thumb_size = (int4)ceil(t_track_size * sbinfo -> thumbsize / (sbinfo -> endvalue - sbinfo -> startvalue));
			if (t_thumb_size < t_thumb_minimum_size || !MCproportionalthumbs)
			{
				t_thumb_size = t_thumb_minimum_size;
				t_thumb_offset = (int4)floor((t_track_size - t_thumb_size) * (sbinfo -> thumbpos - sbinfo -> startvalue) / (sbinfo -> endvalue - sbinfo -> startvalue - (sbinfo -> endvalue > sbinfo -> startvalue ? sbinfo -> thumbsize : -sbinfo -> thumbsize)));
			}
			else
				t_thumb_offset = (int4)floor(t_track_size * (sbinfo -> thumbpos - sbinfo -> startvalue) / (sbinfo -> endvalue - sbinfo -> startvalue));


			if (t_thumb_size > t_track_size)
			{
				t_thumb_size = 0;
				t_thumb_offset = t_track_size;
			}
		}

		if ((winfo . attributes & WTHEME_ATT_SBVERTICAL) != 0)
		{
			MCU_set_rect(sbdecarrowrect, srect . x, srect . y, srect . width, srect . width);
			MCU_set_rect(sbincarrowrect, srect . x, srect . y + srect . height - srect . width, srect . width, srect . width);
			MCU_set_rect(sbdectrackrect, srect . x, srect . y + srect . width, srect . width, t_thumb_offset);
			MCU_set_rect(sbthumbrect, srect . x, srect . y + srect . width + t_thumb_offset, srect . width, t_thumb_size);
			MCU_set_rect(sbinctrackrect, srect . x, srect . y + srect . width + t_thumb_offset + t_thumb_size, srect . width, t_track_size - t_thumb_size - t_thumb_offset);
		}
		else
		{
			MCU_set_rect(sbdecarrowrect, srect . x, srect . y, srect . height, srect . height);
			MCU_set_rect(sbincarrowrect, srect . x + srect . width - srect . height, srect . y, srect . height, srect . height);
			MCU_set_rect(sbdectrackrect, srect . x + srect . height, srect . y, t_thumb_offset, srect . height);
			MCU_set_rect(sbthumbrect, srect . x + srect . height + t_thumb_offset, srect . y, t_thumb_size, srect . height);
			MCU_set_rect(sbinctrackrect, srect . x + srect . height + t_thumb_offset + t_thumb_size, srect . y, t_track_size - t_thumb_size - t_thumb_offset, srect . height);
		}
	}
}

Boolean MCNativeTheme::GetThemePartAndState(const MCWidgetInfo &winfo, int4& aPart, int4& aState)
{
	// MW-2006-04-21: [[ Purify ]] aPart, aState should be 0
	aPart = 0;
	aState = 0;
	switch (winfo.type)
	{
	case WTHEME_TYPE_PUSHBUTTON:
		aPart = BP_BUTTON;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{
			aState = TS_NORMAL;

			if (winfo.state & WTHEME_STATE_PRESSED &&
			        winfo.state & WTHEME_STATE_HOVER)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HASFOCUS)
				aState = TS_FOCUSED;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else
				aState = TS_NORMAL;
			if (aState == TS_NORMAL && winfo.state & WTHEME_STATE_HASDEFAULT)
				aState = TS_FOCUSED;
		}
		break;
	case WTHEME_TYPE_CHECKBOX:
	case WTHEME_TYPE_RADIOBUTTON:
		aPart = (winfo.type == WTHEME_TYPE_CHECKBOX) ? BP_CHECKBOX : BP_RADIO;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;

		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED &&
			        winfo.state & WTHEME_STATE_HOVER)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else
				aState = TS_NORMAL;
		}
		if (winfo.state & WTHEME_STATE_HILITED)
			aState += 4; // 4 unchecked states, 4 checked states.
		break;
	case WTHEME_TYPE_GROUP_FRAME:
	case WTHEME_TYPE_GROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FRAME:
	case WTHEME_TYPE_SECONDARYGROUP_FILL:
		aPart = BP_GROUPBOX;
		aState = winfo.state & WTHEME_STATE_DISABLED? TS_DISABLED: TS_NORMAL;
		break;
	case WTHEME_TYPE_COMBOFRAME:
	case WTHEME_TYPE_COMBOTEXT:
	case WTHEME_TYPE_TEXTFIELD:
	case WTHEME_TYPE_TEXTFIELD_FRAME:
	case WTHEME_TYPE_TEXTFIELD_FILL:
		aPart = TFP_TEXTFIELD;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{

			if (winfo.state & WTHEME_STATE_READONLY)
				aState = TFS_READONLY;
			else
			{
				if (winfo.state & WTHEME_STATE_PRESSED &&
				        winfo.state & WTHEME_STATE_HOVER)
					aState = TS_ACTIVE;
				else if (winfo.state & WTHEME_STATE_HASFOCUS)
					aState = TS_FOCUSED;
				else if (winfo.state & WTHEME_STATE_HOVER)
					aState = TS_HOVER;
				else
					aState = TS_NORMAL;
			}
		}
		break;
	case WTHEME_TYPE_TOOLTIP:
		aPart = TTP_STANDARD;
		aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_PROGRESSBAR:
	case WTHEME_TYPE_PROGRESSBAR_HORIZONTAL:
		aPart = PP_BAR;
		aState = winfo.state & WTHEME_STATE_DISABLED? TS_DISABLED: TS_NORMAL;
		break;
	case WTHEME_TYPE_PROGRESSBAR_CHUNK:
		aPart = PP_CHUNK;
		aState = winfo.state & WTHEME_STATE_DISABLED? TS_DISABLED: TS_NORMAL;
		break;
	case WTHEME_TYPE_PROGRESSBAR_VERTICAL:
		aPart = PP_BARVERT;
		aState = winfo.state & WTHEME_STATE_DISABLED? TS_DISABLED: TS_NORMAL;
		break;
	case WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL:
		aPart = PP_CHUNKVERT;
		aState = winfo.state & WTHEME_STATE_DISABLED? TS_DISABLED: TS_NORMAL;
		break;
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_UP:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_DOWN:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_LEFT:
	case WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_RIGHT:
		if (winfo.type == WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_UP ||
		        winfo.type == WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_DOWN)
			aPart =   winfo.type == WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_UP? 1: 2;
		else
			aPart =  winfo.type == WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_RIGHT? 3: 4;

		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED && winfo.state & WTHEME_STATE_HOVER)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else if (winfo.state & WTHEME_STATE_CONTROL_HOVER)
				aState = TS_CONTROL_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_SCROLLBAR_BUTTON_UP:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT:
	case WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT:
		aPart = SP_BUTTON;
		aState = (winfo.type - WTHEME_TYPE_SCROLLBAR_BUTTON_UP)*4;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState += TS_DISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED && winfo.state & WTHEME_STATE_HOVER)
				aState += TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState += TS_HOVER;
			else if (winfo.state & WTHEME_STATE_CONTROL_HOVER)
				aState = 17 + (winfo.type - WTHEME_TYPE_SCROLLBAR_BUTTON_UP);
			else
				aState += TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL:
		if (winfo . type == WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL)
			aPart = winfo . part == WTHEME_PART_TRACK_DEC ? SP_TRACKSTARTHOR : SP_TRACKSTARTHOR;
		else
			aPart = winfo . part == WTHEME_PART_TRACK_INC ? SP_TRACKENDVERT : SP_TRACKENDVERT;

		if (winfo.state & WTHEME_STATE_DISABLED)
			aState += TS_DISABLED;
		else 	if (winfo.state & WTHEME_STATE_PRESSED && winfo.state & WTHEME_STATE_HOVER)
			aState = TS_ACTIVE;
		else
			aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL:
	case WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL:
		aPart = (winfo.type == WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL) ? SP_THUMBHOR : SP_THUMBVERT;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else if (winfo.state & WTHEME_STATE_CONTROL_HOVER)
				aState = TS_CONTROL_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL:
	case WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL:
		aPart = (winfo.type == WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL) ? SP_GRIPPERHOR : SP_GRIPPERVERT;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else if (winfo.state & WTHEME_STATE_CONTROL_HOVER)
				aState = TS_CONTROL_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL:
	case WTHEME_TYPE_SLIDER_TRACK_VERTICAL:
		aPart = (winfo.type == WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL)? SLIDERP_TRACKHOR : SLIDERP_TRACKVERT;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState += TS_DISABLED;
		else 	if (winfo.state & WTHEME_STATE_PRESSED && winfo.state & WTHEME_STATE_HOVER)
			aState = TS_ACTIVE;
		else
			aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL:
	case WTHEME_TYPE_SLIDER_THUMB_VERTICAL:
		aPart = (winfo.type == WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL)? SLIDERP_THUMBHOR : SLIDERP_THUMBVERT;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = SLIDERP_THUMBDISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_PRESSED)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;


	case WTHEME_TYPE_SMALLSCROLLBAR:
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_COMBO:
	case WTHEME_TYPE_OPTIONBUTTON:
		aPart = aState = 0;
		break;
	case WTHEME_TYPE_TREEVIEW:
	case WTHEME_TYPE_LISTBOX:
		aPart = TREEVIEW_BODY;
		aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_TABPANE:
		aPart = TABP_PANEL;
		aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_TAB:
		aPart = TABP_TAB;
		if (winfo.state & WTHEME_STATE_DISABLED)
		{
			aState = TS_DISABLED;
			break;
		}
		if (winfo.state & WTHEME_STATE_HILITED)
		{
			aPart = TABP_TAB_SELECTED;
			aState = TS_ACTIVE;
		}
		else
		{
			if (winfo.state & WTHEME_STATE_HOVER && winfo.state & WTHEME_STATE_PRESSED)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HASFOCUS)
				aState = TS_FOCUSED;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_TREEVIEW_HEADER_SORTARROW:
		aPart = 4;
		aState = 1;
		break;
	case WTHEME_TYPE_COMBOBUTTON:
		aPart = CBP_DROPMARKER;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else
		{
			if (winfo.state & WTHEME_STATE_HOVER && winfo.state & WTHEME_STATE_PRESSED)
				aState = TS_ACTIVE;
			else if (winfo.state & WTHEME_STATE_HOVER)
				aState = TS_HOVER;
			else
				aState = TS_NORMAL;
		}
		break;
	case WTHEME_TYPE_OPTIONBUTTONTEXT:
		aPart = CBP_READONLY;
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else if (winfo.state & WTHEME_STATE_HOVER && winfo.state & WTHEME_STATE_PRESSED)
			aState = TS_ACTIVE;
		else if (winfo . state & WTHEME_STATE_HOVER)
			aState = TS_HOVER;
		else
			aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_OPTIONBUTTONARROW:
		aPart = CBP_DROPDOWNBUTTONRIGHT;
		aState = TS_NORMAL;
		break;
	case WTHEME_TYPE_SPIN:
		aPart = 1 + (winfo.part - WTHEME_PART_SPIN_ARROW_UP);
		if (winfo.state & WTHEME_STATE_DISABLED)
			aState = TS_DISABLED;
		else if (winfo.state & WTHEME_STATE_HOVER && winfo.state & WTHEME_STATE_PRESSED)
			aState = TS_ACTIVE;
		else if (winfo . state & WTHEME_STATE_HOVER)
			aState = TS_HOVER;
		else
			aState = TS_NORMAL;
		break;
	default:
		aPart = 0;
		aState = 0;
		return False;
	}
	return True;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Bug 9719 ]] Override to return the actual sizeof(MCThemeDrawInfo).
uint32_t MCNativeTheme::getthemedrawinfosize(void)
{
	return sizeof(MCThemeDrawInfo);
}

int32_t MCNativeTheme::fetchtooltipstartingheight(void)
{
	if (MCmajorosversion < 0x0500)
		return 0;

	// MW-2012-09-19: [[ Bug ]] Adjustment to tooltip metrics for XP.
	if (MCmajorosversion < 0x600)
		return 2;

	return 3;
}

bool MCNativeTheme::applythemetotooltipwindow(Window p_window, const MCRectangle& p_rect)
{
	if (MCmajorosversion < 0x0600)
		return false;

	// IM-2014-04-21: [[ Bug 12235 ]] Scale themed tooltip rect to screen coords
	MCRectangle t_screen_rect;
	t_screen_rect = MCscreen->logicaltoscreenrect(p_rect);

	RECT t_win_rect;
	SetRect(&t_win_rect, 0, 0, t_screen_rect . width, t_screen_rect . height);

	HDC t_dc;
	t_dc = GetDC(NULL);

	HRGN t_region;
	getThemeBackgroundRegion(mTooltipTheme, t_dc, TTP_STANDARD, TS_NORMAL, &t_win_rect, &t_region);

	ReleaseDC(NULL, t_dc);

	SetWindowRgn((HWND)p_window -> handle . window, t_region, TRUE);

	return true;
}

bool MCNativeTheme::drawtooltipbackground(MCContext *p_context, const MCRectangle& p_rect)
{
	if (MCmajorosversion < 0x0600)
		return false;

	MCWidgetInfo t_info;
	t_info . attributes = 0;
	t_info . data = NULL;
	t_info . datatype = WTHEME_DATA_NONE;
	t_info . part = WTHEME_PART_ALL;
	t_info . state = 0;
	t_info . type = WTHEME_TYPE_TOOLTIP;
	t_info . whichobject = 0;
	drawwidget(p_context, t_info, p_rect);

	
	return true;
}

bool MCNativeTheme::settooltiptextcolor(MCContext *p_context)
{
	if (MCmajorosversion < 0x0600)
		return false;

	MCColor t_color;
	t_color . red = 64;
	t_color . green = 64;
	t_color . blue = 64;
	p_context -> setforeground(t_color);

	return true;
}

bool MCNativeTheme::drawmenubackground(MCDC *dc, const MCRectangle& dirty, const MCRectangle& rect, bool p_gutter)
{
	if (getmenutheme() == NULL)
		return false;

	MCThemeDrawInfo t_info;
	t_info . theme = getmenutheme();
	t_info . state = 1;
	t_info . clip = dirty;
	t_info . clip_interior = false;

	t_info . part = MENU_POPUPBORDERS;
	t_info . bounds = rect;
	dc -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

	t_info . part = MENU_POPUPBACKGROUND;
	MCU_set_rect(t_info . bounds, rect . x + 3, rect . y + 3, rect . width - 6, rect . height - 6);
	dc -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

	if (p_gutter)
	{
		t_info . part = MENU_POPUPGUTTER;
		MCU_set_rect(t_info . bounds, rect . x + 1 + 2, rect . y + 1 + 2, 22 + 4 + 2, rect . height - 2 - 4);
		dc -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);
	}
	
	return true;
}

bool MCNativeTheme::drawmenubarbackground(MCDC *dc, const MCRectangle& dirty, const MCRectangle& rect, bool is_active)
{
	if (getmenutheme() == NULL)
		return false;

	MCThemeDrawInfo t_info;
	t_info . theme = getmenutheme();
	t_info . part = MENU_BARBACKGROUND;
	t_info . state = (is_active ? MB_ACTIVE : MB_INACTIVE);
	t_info . bounds = rect;
	t_info . clip = dirty;
	t_info . clip_interior = false;
	dc -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

	return true;
}

bool MCNativeTheme::drawmenuheaderbackground(MCContext *p_context, const MCRectangle& p_dirty, MCButton *p_button)
{
	if (getmenutheme() == NULL)
		return false;

	MCThemeDrawInfo t_info;
	t_info . theme = getmenutheme();
	t_info . part = MENU_BARITEM;

	if (p_button -> getstate(CS_ARMED))
		t_info . state = MBI_PUSHED;
	else if (p_button -> gethovering())
		t_info . state = MBI_HOT;
	else
		t_info . state = MBI_NORMAL;

	if (p_button -> getflag(F_DISABLED))
		t_info . state += 3;

	t_info . bounds = p_button -> getrect();
	t_info . clip = p_dirty;
	t_info . clip_interior = false;
	p_context -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

	return true;
}

bool MCNativeTheme::drawmenuitembackground(MCContext *p_context, const MCRectangle& p_dirty, MCButton *p_button)
{
	if (getmenutheme() == NULL)
		return false;

	MCThemeDrawInfo t_info;
	t_info . theme = getmenutheme();

	if (p_button -> getmenucontrol() == MENUCONTROL_ITEM)
	{
		t_info . part = MENU_POPUPITEM;

		if (p_button -> getstate(CS_ARMED))
			t_info . state = MBI_HOT;
		else
			t_info . state = MBI_NORMAL;

		if (p_button -> getflag(F_DISABLED))
			t_info . state += 2;
	}
	else
	{
		t_info . part = MENU_POPUPSEPARATOR;
		t_info . state = 1;
	}

	MCRectangle t_rect;
	t_rect = p_button -> getrect();

	t_info . bounds = t_rect;
	t_info . clip = p_dirty;
	t_info . clip_interior = false;

	p_context -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

	if ((p_button -> getstyle() == F_CHECK || p_button -> getstyle() == F_RADIO) && p_button -> getstate(CS_HILITED))
	{
		MCU_set_rect(t_info . bounds, t_rect . x, t_rect . y, 22, t_rect . height);

		t_info . part = MENU_POPUPCHECKBACKGROUND;
		t_info . state = p_button -> getflag(F_DISABLED) ? MCB_DISABLED : MCB_NORMAL;
		p_context -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);

		t_info . part = MENU_POPUPCHECK;
		if (p_button -> getstyle() == F_CHECK)
			t_info . state = MC_CHECKMARKNORMAL;
		else
			t_info . state = MC_BULLETNORMAL;

		if (p_button -> getflag(F_DISABLED))
			t_info . state += 1;

		p_context -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);
	}

	if (p_button -> getstyle() == F_MENU && p_button -> getmenumode() == WM_CASCADE)
	{
		t_info . part = MENU_POPUPSUBMENU;
		t_info . state = p_button -> getflag(F_DISABLED) ? MSM_DISABLED : MSM_NORMAL;
		MCU_set_rect(t_info . bounds, t_rect . x + t_rect . width - 17, t_rect . y, 17, t_rect . height);
		p_context -> drawtheme(THEME_DRAW_TYPE_MENU, &t_info);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCTheme *MCThemeCreateNative(void)
{
	return new MCNativeTheme;
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCGDIDrawFunc)(HDC p_hdc, void *p_context);
bool MCGDIDrawAlpha(uint32_t p_width, uint32_t p_height, MCGDIDrawFunc p_draw, void *p_context, MCImageBitmap *&r_bitmap);
void MCGDIDrawTheme(HDC p_dc, void *p_context);

typedef struct
{
	MCThemeDrawType type;
	MCThemeDrawInfo *info;
	MCPoint origin;
} MCGDIThemeDrawContext;

bool MCWin32ThemeDrawBuffered(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr)
{
	bool t_success = true;

	int32_t t_x, t_y;
	uint32_t t_width, t_height;

	MCGDIThemeDrawContext t_context;
	t_context.type = p_type;
	t_context.info = p_info_ptr;

	t_x = p_info_ptr->bounds.x;
	t_y = p_info_ptr->bounds.y;
	t_width = p_info_ptr->bounds.width;
	t_height = p_info_ptr->bounds.height;

	HDC t_dc = ((MCScreenDC*)MCscreen)->getdsthdc();
	t_success = t_dc != nil;

	HDC t_paintdc = nil;
	HPAINTBUFFER t_buffer = nil;
	RECT t_target_rect;

	if (t_success)
	{
		SetRect(&t_target_rect, 0, 0, p_info_ptr->bounds.width, p_info_ptr->bounds.height);
		t_buffer = beginBufferedPaint(t_dc, &t_target_rect, BPBF_TOPDOWNDIB, NULL, &t_paintdc);
		t_success = t_buffer != nil;
	}

	RGBQUAD *t_bits = nil;
	int t_row_width = 0;
	if (t_success)
		t_success = S_OK == bufferedPaintClear(t_buffer, NULL);

	if (t_success)
	{
		MCGDIDrawTheme(t_paintdc, &t_context);
		t_success = S_OK == getBufferedPaintBits(t_buffer, &t_bits, &t_row_width);
	}

	if (t_success)
	{
		MCGRaster t_raster;
		t_raster.width = t_width;
		t_raster.height = t_height;
		t_raster.stride = t_row_width * sizeof(uint32_t);
		t_raster.pixels = t_bits;
		t_raster.format = kMCGRasterFormat_ARGB;

		MCGRectangle t_dst = MCGRectangleMake(t_x, t_y, t_width, t_height);
		
		// MM-2013-12-16: [[ Bug 11567 ]] Use bilinear filter when drawing theme elements.
        // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was bilinear).
		MCGContextDrawPixels(p_context, t_raster, t_dst, kMCGImageFilterMedium);
	}

	if (t_buffer != nil)
		endBufferedPaint(t_buffer, FALSE);

	return t_success;
}

bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr)
{
	bool t_success = true;

	/* OVERHAUL - REVISIT: This does not seem to fix the issue of alpha-transparency with windows GDI calls, disabling for now */
	//if (beginBufferedPaint != nil)
	//	return MCWin32ThemeDrawBuffered(p_context, p_type, p_info_ptr);

	MCImageBitmap *t_bitmap = nil;
	int32_t t_x, t_y;
	uint32_t t_width, t_height;

	MCGRectangle t_dst;

	MCRectangle t_old_bounds, t_old_interior, t_old_clip;
	t_old_bounds = p_info_ptr->bounds;
	t_old_clip = p_info_ptr->clip;
	t_old_interior = p_info_ptr->interior;

	MCGAffineTransform t_transform;
	t_transform = MCGContextGetDeviceTransform(p_context);

	// IM-2013-12-13: [[ HiDPI ]] Improve scaled UI appearance by rendering at transformed size.
	if (MCGAffineTransformIsRectangular(t_transform))
	{
		// render theme elements at scaled size

		MCGRectangle t_scaled_bounds;
		t_scaled_bounds = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_info_ptr->bounds), t_transform);

		MCGRectangle t_scaled_interior;
		t_scaled_interior = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_info_ptr->interior), t_transform);

		MCGRectangle t_scaled_clip;
		t_scaled_clip = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_info_ptr->clip), t_transform);

		MCRectangle t_int_bounds;
		t_int_bounds = MCGRectangleGetIntegerInterior(t_scaled_bounds);

		MCRectangle t_int_interior;
		t_int_interior = MCGRectangleGetIntegerBounds(t_scaled_interior);

		MCRectangle t_int_clip;
		t_int_clip = MCGRectangleGetIntegerBounds(t_scaled_clip);
		t_int_clip = MCU_intersect_rect(t_int_clip, t_int_bounds);


		t_width = t_int_clip.width;
		t_height = t_int_clip.height;

		p_info_ptr->bounds = t_int_bounds;
		p_info_ptr->interior = t_int_interior;
		p_info_ptr->clip = t_int_clip;

		t_x = t_int_clip.x;
		t_y = t_int_clip.y;

		t_dst = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(t_int_clip), MCGAffineTransformInvert(t_transform));
	}
	else
	{
		// render at normalsize & draw into target rect
	t_x = p_info_ptr->bounds.x;
	t_y = p_info_ptr->bounds.y;

	t_width = p_info_ptr->bounds.width;
	t_height = p_info_ptr->bounds.height;

		t_dst = MCGRectangleMake(t_x, t_y, t_width, t_height);
	}

	MCGDIThemeDrawContext t_context;
	t_context.type = p_type;
	t_context.info = p_info_ptr;
	t_context.origin = MCPointMake(t_x, t_y);


	// render theme to bitmap
	t_success = MCGDIDrawAlpha(t_width, t_height, MCGDIDrawTheme, &t_context, t_bitmap);
	if (t_success)
	{
		MCGRaster t_raster;
		t_raster = MCImageBitmapGetMCGRaster(t_bitmap, true);
		t_raster.format = kMCGRasterFormat_ARGB;

		
		// MM-2013-12-16: [[ Bug 11567 ]] Use bilinear filter when drawing theme elements.
        // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was bilinear).
		MCGContextDrawPixels(p_context, t_raster, t_dst, kMCGImageFilterMedium);
	}

	p_info_ptr->bounds = t_old_bounds;
	p_info_ptr->interior = t_old_interior;
	p_info_ptr->clip = t_old_clip;

	MCImageFreeBitmap(t_bitmap);

	return t_success;
}

void MCGDIDrawTheme(HDC p_dc, void *p_context)
{
	MCGDIThemeDrawContext *t_context = (MCGDIThemeDrawContext*)p_context;

	MCThemeDrawInfo& p_info = *t_context->info;

	HRGN t_clip_region = NULL;

	// IM-2013-12-13: [[ HiDPI ]] Use context origin to offset drawing rects
	int32_t t_xoff, t_yoff;
	t_xoff = t_context->origin.x;
	t_yoff = t_context->origin.y;

	MCRectangle t_bounds, t_interior, t_clip;
	t_bounds = MCU_offset_rect(p_info.bounds, -t_xoff, -t_yoff);
	t_interior = MCU_offset_rect(p_info.interior, -t_xoff, -t_yoff);
	t_clip = MCU_offset_rect(p_info.clip, -t_xoff, -t_yoff);

	if (p_info . clip_interior)
	{
		HRGN t_outside_region;
		t_outside_region = CreateRectRgn(t_bounds.x, t_bounds.y, t_bounds.x + t_bounds.width, t_bounds.y + t_bounds.height);

		HRGN t_inside_region;
		t_inside_region = CreateRectRgn(t_interior.x, t_interior.y, t_interior.x + t_interior.width, t_interior.y + t_interior.height);

		CombineRgn(t_inside_region, t_outside_region, t_inside_region, RGN_DIFF);
		DeleteObject(t_outside_region);

		t_clip_region = t_inside_region;
	}

	SaveDC(p_dc);

	if (t_clip_region != NULL)
		SelectClipRgn(p_dc, t_clip_region);

	RECT t_widget_rect;
	RECT t_clip_rect;
	SetRect(&t_widget_rect, t_bounds.x, t_bounds.y, t_bounds.x + t_bounds.width, t_bounds.y + t_bounds.height);
	SetRect(&t_clip_rect, t_clip.x, t_clip.y, t_clip.x + t_clip.width, t_clip.y + t_clip.height);
	drawThemeBG(p_info . theme, p_dc, p_info . part, p_info . state, &t_widget_rect, &t_clip_rect);

	RestoreDC(p_dc, -1);

	if (t_clip_region != NULL)
		DeleteObject(t_clip_region);
}

////////////////////////////////////////////////////////////////////////////////
