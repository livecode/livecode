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

#ifndef MCTHEME_H
#define MCTHEME_H

enum Widget_Type {
    WTHEME_TYPE_UNDEFINED,
    WTHEME_TYPE_ARROW,

    WTHEME_TYPE_CHECKBOX,
    WTHEME_TYPE_PUSHBUTTON,
    WTHEME_TYPE_BEVELBUTTON,
    WTHEME_TYPE_RADIOBUTTON,
    WTHEME_TYPE_OPTIONBUTTON,
	WTHEME_TYPE_OPTIONBUTTONARROW,
	WTHEME_TYPE_OPTIONBUTTONTEXT,
    WTHEME_TYPE_COMBO,
    WTHEME_TYPE_COMBOBUTTON,
    WTHEME_TYPE_COMBOTEXT,
    WTHEME_TYPE_COMBOFRAME,
    WTHEME_TYPE_PULLDOWN,
    WTHEME_TYPE_TABPANE,
    WTHEME_TYPE_TAB,
    WTHEME_TYPE_TEXTFIELD,
    WTHEME_TYPE_TEXTFIELD_FRAME,
    WTHEME_TYPE_TEXTFIELD_FILL,
    WTHEME_TYPE_SMALLSCROLLBAR,
    WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_UP,
    WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_DOWN,
    WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_LEFT,
    WTHEME_TYPE_SMALLSCROLLBAR_BUTTON_RIGHT,
    WTHEME_TYPE_SCROLLBAR,
    WTHEME_TYPE_SCROLLBAR_TRACK_VERTICAL,
    WTHEME_TYPE_SCROLLBAR_TRACK_HORIZONTAL,
    WTHEME_TYPE_SCROLLBAR_BUTTON_UP,
    WTHEME_TYPE_SCROLLBAR_BUTTON_DOWN,
    WTHEME_TYPE_SCROLLBAR_BUTTON_LEFT,
    WTHEME_TYPE_SCROLLBAR_BUTTON_RIGHT,
    WTHEME_TYPE_SCROLLBAR_THUMB_VERTICAL,
    WTHEME_TYPE_SCROLLBAR_THUMB_HORIZONTAL,
    WTHEME_TYPE_SCROLLBAR_GRIPPER_VERTICAL,
    WTHEME_TYPE_SCROLLBAR_GRIPPER_HORIZONTAL,
    WTHEME_TYPE_TREEVIEW_HEADER_CELL,
    WTHEME_TYPE_TREEVIEW_HEADER_SORTARROW,
    WTHEME_TYPE_LISTBOX,
    WTHEME_TYPE_LISTBOX_LISTITEM,
    WTHEME_TYPE_LISTBOX_FILL,
    WTHEME_TYPE_LISTBOX_FRAME,
    WTHEME_TYPE_TREEVIEW,
    WTHEME_TYPE_TREEVIEW_TWISTY_OPEN,
    WTHEME_TYPE_TREEVIEW_TREEITEM,
    WTHEME_TYPE_PROGRESSBAR,
    WTHEME_TYPE_PROGRESSBAR_HORIZONTAL,
    WTHEME_TYPE_PROGRESSBAR_VERTICAL,
    WTHEME_TYPE_PROGRESSBAR_CHUNK,
    WTHEME_TYPE_PROGRESSBAR_CHUNK_VERTICAL,
    WTHEME_TYPE_SLIDER,
    WTHEME_TYPE_SLIDER_TRACK_VERTICAL,
    WTHEME_TYPE_SLIDER_TRACK_HORIZONTAL,
    WTHEME_TYPE_SLIDER_THUMB_HORIZONTAL,
    WTHEME_TYPE_SLIDER_THUMB_VERTICAL,
    WTHEME_TYPE_TOOLTIP,
    WTHEME_TYPE_MENUITEMHIGHLIGHT,
    WTHEME_TYPE_GROUP_FRAME,
    WTHEME_TYPE_GROUP_FILL,
    WTHEME_TYPE_SECONDARYGROUP_FRAME,
    WTHEME_TYPE_SECONDARYGROUP_FILL,
    WTHEME_TYPE_MENU,
	WTHEME_TYPE_SPIN
};

enum Widget_DataType {
    WTHEME_DATA_NONE,
    //no extra data
    WTHEME_DATA_SCROLLBAR,
    //pointer to mcwidgetscrollbarinfo structure
    WTHEME_DATA_TABPANE,
    WTHEME_DATA_MCOBJECT,
    WTHEME_DATA_RECT
    //pointer to mcobject
};


enum Widget_Color {
    WCOLOR_TEXT,
    WCOLOR_HILIGHT,
    WCOLOR_BACK,
    WCOLOR_BORDER
};

enum Widget_Metric {
    WTHEME_METRIC_TABOVERLAP,
    WTHEME_METRIC_TABRIGHTMARGIN,
    WTHEME_METRIC_TABLEFTMARGIN,
    WTHEME_METRIC_TABNONSELECTEDOFFSET,
    WTHEME_METRIC_TABSTARTOFFSET,
    WTHEME_METRIC_COMBOSIZE,
    WTHEME_METRIC_MINIMUMSIZE,
    WTHEME_METRIC_OPTIMUMSIZE,
    WTHEME_METRIC_DRAWSIZE,
    WTHEME_METRIC_CONTENTSIZE, //use instead of progressbarborder
    WTHEME_METRIC_PARTSIZE,
    WTHEME_METRIC_TRACKSIZE,
	WTHEME_METRIC_OPTIONBUTTONARROWSIZE,
	WTHEME_METRIC_RADIOBUTTON_INDICATORSIZE,
	WTHEME_METRIC_RADIOBUTTON_INDICATORSPACING,
	WTHEME_METRIC_CHECKBUTTON_INDICATORSIZE,
	WTHEME_METRIC_CHECKBUTTON_INDICATORSPACING,
    WTHEME_METRIC_TABBUTTON_HEIGHT,             // Height of tab buttons, if fixed
};

enum Widget_Part {
    //not related to state but more or less defines what the style of the control or a part in it
    WTHEME_PART_UNDEFINED,
    WTHEME_PART_ALL,
    //for hit testing when it is a hit but the part cannot be specified
    //SCROLLBARS
    WTHEME_PART_ARROW_DEC,
    WTHEME_PART_ARROW_INC,
    WTHEME_PART_TRACK_DEC,
    WTHEME_PART_TRACK_INC,
    WTHEME_PART_THUMB,
    // COMBO
    WTHEME_PART_COMBOBUTTON,
    WTHEME_PART_COMBOTEXT,
	// OPTION
	WTHEME_PART_OPTIONBUTTONARROW,
	WTHEME_PART_OPTIONBUTTONTEXT,
	// SPIN
	WTHEME_PART_SPIN_ARROW_UP,
	WTHEME_PART_SPIN_ARROW_DOWN,
	WTHEME_PART_SPIN_ARROW_RIGHT,
	WTHEME_PART_SPIN_ARROW_LEFT
};

enum Widget_ThemeProps {
    WTHEME_PROP_SUPPORTHOVERING,
    WTHEME_PROP_ALWAYSBUFFER,
    WTHEME_PROP_TABSELECTONMOUSEUP,
    WTHEME_PROP_DRAWTABPANEFIRST,
    WTHEME_PROP_TABBUTTONSOVERLAPPANE,
    //theme controls should redraw when mouse is over objects
};



//basic state stuff
#define WTHEME_STATE_CLEAR 0
#define WTHEME_STATE_DISABLED  (1UL << 0)
#define WTHEME_STATE_PRESSED (1UL << 1)
#define WTHEME_STATE_HILITED (1UL << 2)
#define WTHEME_STATE_HASDEFAULT (1UL << 3)
#define WTHEME_STATE_HOVER (1UL << 4)
#define WTHEME_STATE_HASFOCUS (1UL << 5)
#define WTHEME_STATE_READONLY (1UL << 6)
#define WTHEME_STATE_CONTROL_HOVER (1UL << 6)
#define WTHEME_STATE_SUPPRESSDEFAULT (1UL << 7)


//attributes start at 13
//TAB attributes
#define WTHEME_ATT_CLEAR 0
#define WTHEME_ATT_TABRIGHTEDGE (1UL << 13)
#define WTHEME_ATT_TABLEFTEDGE  (1UL << 14)
//#define WTHEME_ATT_TABBEFORESELECTED maps to tableftedge

#define WTHEME_ATT_TABFIRSTSELECTED (1UL << 15)

#define WTHEME_ATT_TABPOSTOP (1UL << 17)
#define WTHEME_ATT_TABPOSBOTTOM (1UL << 18)
#define WTHEME_ATT_TABPOSLEFT (1UL << 19)
#define WTHEME_ATT_TABPOSRIGHT (1UL << 20)
#define WTHEME_ATT_FIRSTTAB (1UL << 21)
#define WTHEME_ATT_LASTTAB (1UL << 22)

//SCROLLBAR attributes
#define WTHEME_ATT_SHOWVALUE (1UL << 13)
#define WTHEME_ATT_SBVERTICAL (1UL << 14)

typedef struct MCWidgetInfo
{
	Widget_Type type;
	uint4 state;
	uint4 attributes;
	Widget_Part part;
	Widget_DataType datatype;
	void *data;
	MCObject *whichobject;
	//optional object to be passed along with widget info
}
MCWidgetInfo;

typedef struct MCWidgetScrollBarInfo
{
	double startvalue;
	double thumbpos;
	double endvalue;
	double thumbsize;
	double pageinc;
	double lineinc;
}
MCWidgetScrollbarInfo;


typedef struct _MCWidgetTabPaneInfo
{
	int gap_start;
	int gap_length;
}
MCWidgetTabPaneInfo;

#define IsMacLFAM() (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEMAC)
#define IsMacLF() (MClook == LF_MAC || (MCcurtheme && MCcurtheme->getthemefamilyid() == LF_MAC))
#define IsXLF() (MClook == LF_MOTIF || (MCcurtheme && MCcurtheme->getthemefamilyid() == LF_MOTIF))
#define IsMacEmulatedLF() (MClook == LF_MAC && MCcurtheme == NULL)
#define IsXEmulatedLF() (MClook == LF_MOTIF && MCcurtheme == NULL)

// Added by TS, 2008-01-21
#define IsNativeWin() ( MCcurtheme && MCcurtheme -> getthemeid() == LF_NATIVEWIN)
#define IsNativeMac() ( MCcurtheme && MCcurtheme -> getthemeid() == LF_NATIVEMAC)
#define IsNativeGTK() ( MCcurtheme && MCcurtheme -> getthemeid() == LF_NATIVEGTK)

class MCTheme //base class for all widgets
{
public:
	virtual ~MCTheme(void) {};
	virtual Boolean load();
	virtual void unload();

	virtual uint2 getthemeid();
	virtual uint2 getthemefamilyid();
	virtual int4 getmetric(Widget_Metric wmetric);
	virtual Boolean iswidgetsupported(Widget_Type wtype);
	virtual int4 getwidgetmetric(const MCWidgetInfo &winfo,Widget_Metric wmetric);
	virtual void getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect, MCRectangle &drect);
	virtual Boolean drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &d);
	virtual Widget_Part hittest(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect);
	virtual Boolean getthemepropbool(Widget_ThemeProps themeprop);
	virtual void getthemecolor(const MCWidgetInfo &winfo,Widget_Color ctype, MCStringRef &r_colortheme);

	// MW-2011-09-14: [[ Bug 9719 ]] The MCThemeDrawInfo structure is opaque, but
	//   the metacontext needs to know its size.
	virtual uint32_t getthemedrawinfosize(void);

	virtual bool applythemetotooltipwindow(Window window, const MCRectangle& rect);
	virtual bool drawtooltipbackground(MCContext *context, const MCRectangle& rect);
	virtual bool settooltiptextcolor(MCContext *context);
	virtual int32_t fetchtooltipstartingheight(void);
	
	virtual bool drawmenubackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool with_gutter);
	virtual bool drawmenubarbackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, bool is_active);
	virtual bool drawmenuheaderbackground(MCContext *context, const MCRectangle& dirty, MCButton *button);
	virtual bool drawmenuitembackground(MCContext *context, const MCRectangle& dirty, MCButton *button);

	virtual bool drawfocusborder(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect);
	virtual bool drawmetalbackground(MCContext *context, const MCRectangle& dirty, const MCRectangle& rect, MCObject *object);
};

MCTheme *MCThemeCreateNative(void);
		
#endif

