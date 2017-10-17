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

#include "osxprefix.h"

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

#ifndef _IOS_MOBILE
#define CGFloat float
#endif

extern double MCMacGetAnimationStartTime(void);
extern double MCMacGetAnimationCurrentTime(void);

static inline Rect MCRectToMacRect(const MCRectangle &p_rect)
{
	Rect t_rect;
	t_rect.left = p_rect.x;
	t_rect.top = p_rect.y;
	t_rect.right = p_rect.x + p_rect.width;
	t_rect.bottom = p_rect.y + p_rect.height;
	
	return t_rect;
}

////////////////////////////////////////////////////////////////////////////////

static inline CGAffineTransform MCGAffineTransformToCGAffineTransform(const MCGAffineTransform &p_transform)
{
	return CGAffineTransformMake(p_transform.a, p_transform.b, p_transform.c, p_transform.d, p_transform.tx, p_transform.ty);
}

static void convertcgtomcrect(CGRect r, MCRectangle& r_mc)
{
    r_mc . x = r . origin . x;
    r_mc . y = r . origin . y;
    r_mc . width = r . size . width;
    r_mc . height = r . size . height;
}

static void convertmctocgrect(MCRectangle r, CGRect& r_cg)
{
    r_cg . origin . x = r . x;
    r_cg . origin . y = r . y;
    r_cg . size . width = r . width;
    r_cg . size . height = r . height;
}

////////////////////////////////////////////////////////////////////////////////

static void getthemebuttonpartandstate(const MCWidgetInfo &winfo, HIThemeButtonDrawInfo &bNewInfo,const MCRectangle &drect, HIRect &macR);
static void drawthemebutton(MCDC *dc, const MCWidgetInfo &widgetinfo, const MCRectangle &drect);
static void drawthemetabs(MCDC *dc, const MCWidgetInfo &widgetinfo, const MCRectangle &drect);
static Widget_Part HitTestScrollControls(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect);
static void DrawMacAMScrollControls(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect);
static ThemeTrackKind getscrollbarkind(Widget_Type wtype);
static void getscrollbarpressedstate(const MCWidgetInfo &winfo, HIThemeTrackDrawInfo &drawInfo);
static void fillTrackDrawInfo(const MCWidgetInfo &winfo, HIThemeTrackDrawInfo &drawInfo, const MCRectangle &drect);

enum {
    kControlLowerUpButtonPart = 28,
    kControlUpperDownButtonPart = 29
};


static Boolean doublesbarrows;

Boolean MCNativeTheme::load()
{	
	return True;
}


static void converttohirect(const Rect *trect, HIRect *dhrect)
{
	dhrect->origin.x = trect->left;
	dhrect->origin.y = trect->top;
	dhrect->size.width = trect->right - trect->left;
	dhrect->size.height = trect->bottom - trect->top;
}

static void converttohirect(const MCRectangle& in, HIRect& out)
{
    out.origin.x = in.x;
    out.origin.y = in.y;
    out.size.width = in.width;
    out.size.height = in.height;
}

static void converttonativerect(const MCRectangle &mcR, Rect &macR)
{
	macR.top = mcR.y;
	macR.left = mcR.x;
	macR.bottom = mcR.y + mcR.height;
	macR.right = mcR.x + mcR.width;
}


static void converttomcrect(const Rect &macR, MCRectangle &mcR)
{
	mcR.x = macR.left;
	mcR.y = macR.top;
	mcR.width = macR.right - macR.left;
	mcR.height = macR.bottom - macR.top;
}


void MCNativeTheme::unload()
{
}



Boolean MCNativeTheme::iswidgetsupported(Widget_Type w)
{
	switch (w)
	{
	case WTHEME_TYPE_GROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FRAME:
	case WTHEME_TYPE_GROUP_FRAME:
		return True;
		break;
	case WTHEME_TYPE_TABPANE:
	case WTHEME_TYPE_TAB:
		return True;
		break;
	default:
		return True;
	}
}

int4 MCNativeTheme::getmetric(Widget_Metric wmetric)
{
	switch (wmetric)
	{
	case WTHEME_METRIC_TABOVERLAP:
		return -1;
		break;
	case WTHEME_METRIC_TABRIGHTMARGIN:
		return 11;
	case WTHEME_METRIC_TABLEFTMARGIN:
		return 12;
		break;
	case WTHEME_METRIC_TABNONSELECTEDOFFSET:
		return 0;
		break;
	case WTHEME_METRIC_COMBOSIZE:
		return 22;
		break;
	// MH-2007-03-16 [[ Bug 3598 ]] Adding support for the option button arrow size.
	case WTHEME_METRIC_OPTIONBUTTONARROWSIZE:
		return 21;
		break;
    case WTHEME_METRIC_TABBUTTON_HEIGHT:
        return 21;
        break;
	default:
		return 0;
	}
}

int4 MCNativeTheme::getwidgetmetric(const MCWidgetInfo &winfo, Widget_Metric wmetric)
{
	return 0;
}

Boolean MCNativeTheme::getthemepropbool(Widget_ThemeProps themeprop)
{
	if (themeprop == WTHEME_PROP_DRAWTABPANEFIRST)
		return true;
	else if (themeprop == WTHEME_PROP_TABSELECTONMOUSEUP)
		return true;
	else if (themeprop == WTHEME_PROP_TABBUTTONSOVERLAPPANE)
		return true;
	return False;
}


void MCNativeTheme::getwidgetrect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect,
                                  MCRectangle &drect)
{

	switch (winfo.type)
	{
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR:
	case WTHEME_TYPE_PROGRESSBAR:
	case WTHEME_TYPE_SLIDER:
		if (wmetric == WTHEME_METRIC_PARTSIZE)
		{
			HIThemeTrackDrawInfo drawInfo;
			fillTrackDrawInfo(winfo,drawInfo,srect);
			if (winfo.part == WTHEME_PART_THUMB)
			{
				HIShapeRef t_shape;
                
				HIThemeGetTrackThumbShape(&drawInfo, &t_shape);
                
				CGRect t_rect;
                HIShapeGetBounds(t_shape, &t_rect);
                CFRelease(t_shape);
				convertcgtomcrect(t_rect,drect);
				return;
			}
			else if (winfo.part == WTHEME_PART_TRACK_INC)
			{
				CGRect t_rect;
                
				HIThemeGetTrackBounds(&drawInfo, &t_rect);
                
				convertcgtomcrect(t_rect,drect);
				return;
			}
		}
	case WTHEME_TYPE_COMBO:
		{
			if (wmetric == WTHEME_METRIC_PARTSIZE)
			{
				MCRectangle combobuttonrect = srect;
				combobuttonrect.x += srect.width - 22 - 2;
				combobuttonrect.width = 18;
				if (winfo.part == WTHEME_PART_COMBOTEXT)
				{
					HIThemeButtonDrawInfo bNewInfo;
					HIRect macR,maccontentbounds;
					getthemebuttonpartandstate(winfo, bNewInfo,srect,macR);

                    HIThemeGetButtonBackgroundBounds(&macR, &bNewInfo, &maccontentbounds);
                    
					drect = srect;
					drect.height = maccontentbounds.size.height - 1;
					drect = MCU_reduce_rect(drect,2);
					drect.width -= combobuttonrect.width;
				}
				else if (winfo.part == WTHEME_PART_COMBOBUTTON)
					drect = srect;
				return;
			}
		}
	case WTHEME_TYPE_COMBOTEXT:
		{
			if (wmetric == 	WTHEME_METRIC_PARTSIZE)
			{
				drect.width = drect.height = 0;
			}
			else
				drect = srect;
		}
	default:
		break;
	}
	MCTheme::getwidgetrect(winfo,wmetric,srect,drect);
}

uint2 MCNativeTheme::getthemeid()
{
	return LF_NATIVEMAC;
}

uint2 MCNativeTheme::getthemefamilyid()
{
	return LF_MAC;
}

Boolean MCNativeTheme::drawwidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	MCRectangle trect = drect;
	switch (winfo.type)
{
	case WTHEME_TYPE_CHECKBOX:
	case WTHEME_TYPE_PUSHBUTTON:
	case WTHEME_TYPE_BEVELBUTTON:
	case WTHEME_TYPE_RADIOBUTTON:
	case WTHEME_TYPE_OPTIONBUTTON:
	case WTHEME_TYPE_PULLDOWN:
		drawthemebutton(dc, winfo, drect);
		break;
	case WTHEME_TYPE_COMBOFRAME:
		break;

	case WTHEME_TYPE_TABPANE:
	case WTHEME_TYPE_TAB:
		drawthemetabs(dc, winfo, drect);
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
			twinfo.type = WTHEME_TYPE_COMBOBUTTON;
			drawwidget(dc, twinfo, combobuttonrect);
		}
		break;
	case WTHEME_TYPE_COMBOBUTTON:
		drawthemebutton(dc, winfo, trect);
		break;
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR:
	case WTHEME_TYPE_PROGRESSBAR:
		DrawMacAMScrollControls(dc, winfo, drect);
		break;
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
		break; //support in future for drawing primatives

	case WTHEME_TYPE_TEXTFIELD_FRAME:
	case WTHEME_TYPE_COMBOTEXT:
	case WTHEME_TYPE_LISTBOX_FRAME:
		{
			MCThemeDrawInfo t_info;
			t_info.dest = drect;
			converttonativerect(MCU_reduce_rect(trect, 1), t_info . frame . bounds);
			if ((winfo . state & WTHEME_STATE_DISABLED) != 0)
				t_info . frame . state = kThemeStateInactive;
			else if ((winfo . state & WTHEME_STATE_PRESSED) != 0)
				t_info . frame . state = kThemeStatePressed;
			else
				t_info . frame . state = kThemeStateActive;
			t_info . frame . is_list = false;
			dc -> drawtheme(THEME_DRAW_TYPE_FRAME, &t_info);
		}
		break;
	case WTHEME_TYPE_GROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FILL:
	case WTHEME_TYPE_SECONDARYGROUP_FRAME:
	case WTHEME_TYPE_GROUP_FRAME:
		{
			MCThemeDrawInfo t_info;
			t_info.dest = drect;
			converttonativerect(trect, t_info . group . bounds);
			
			if ((winfo . state & WTHEME_STATE_DISABLED) != 0)
				t_info . group . state = kThemeStateInactive;
			else
				t_info . group . state = kThemeStateActive;
				
			if (winfo . datatype == WTHEME_DATA_RECT)
			{
				MCRectangle *textrect = (MCRectangle *)winfo.data;
				t_info . group . bounds . top = textrect -> y + textrect -> height + 2;
			}
			
			t_info . group . is_secondary = (winfo . type == WTHEME_TYPE_SECONDARYGROUP_FILL || winfo . type == WTHEME_TYPE_SECONDARYGROUP_FRAME);
			t_info . group . is_filled = (winfo . type == WTHEME_TYPE_GROUP_FILL || winfo . type == WTHEME_TYPE_SECONDARYGROUP_FILL);
			
			dc -> drawtheme(THEME_DRAW_TYPE_GROUP, &t_info);
		}
		break;
	default:
		break;
	}
	
	return True;
}

Widget_Part MCNativeTheme::hittest(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect)
{
	switch (winfo.type)
	{
	case WTHEME_TYPE_SLIDER:
	case WTHEME_TYPE_SCROLLBAR:
	case WTHEME_TYPE_SMALLSCROLLBAR:
		return HitTestScrollControls(winfo,mx,my,drect);
		break;
	default:
		return MCU_point_in_rect(drect, mx, my)?WTHEME_PART_ALL:WTHEME_PART_UNDEFINED;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void getthemebuttonpartandstate(const MCWidgetInfo &widgetinfo, HIThemeButtonDrawInfo &bNewInfo,const MCRectangle &drect, HIRect &macR)
{
	MCRectangle trect = drect;
	ThemeButtonKind themebuttonkind;
	switch (widgetinfo.type)
	{
	case WTHEME_TYPE_CHECKBOX:
		themebuttonkind = kThemeCheckBox;
		break;
	case WTHEME_TYPE_PUSHBUTTON:
		themebuttonkind = kThemePushButton;
		break;
	case WTHEME_TYPE_BEVELBUTTON:
		themebuttonkind = kThemeBevelButton;
		break;
	case WTHEME_TYPE_RADIOBUTTON:
		themebuttonkind = kThemeRadioButton;
		break;
	case WTHEME_TYPE_OPTIONBUTTON:
		themebuttonkind =  kThemePopupButton;
		break;
	case WTHEME_TYPE_COMBOBUTTON:
	case WTHEME_TYPE_COMBO:
		themebuttonkind = kThemeComboBox;
		break;
	case WTHEME_TYPE_PULLDOWN:
		themebuttonkind = kThemeBevelButton;
		break;
	default:
		MCUnreachable();
		break;
	}
	//set state stuff

    // Version number is always zero
    bNewInfo.version = 0;
    
	if (widgetinfo.state & WTHEME_STATE_DISABLED)
		bNewInfo.state = kThemeStateInactive;
	else
	{
		if (widgetinfo.state & WTHEME_STATE_PRESSED)
			bNewInfo.state = kThemeStatePressed;
		else
			bNewInfo.state = kThemeStateActive;
	}
	if (widgetinfo.state & WTHEME_STATE_HILITED)
		bNewInfo.value = kThemeButtonOn;
	else
		bNewInfo.value = kThemeButtonOff;
	//set adornment
	bNewInfo.adornment = kThemeAdornmentNone;
	if (themebuttonkind == kThemeCheckBox)
		bNewInfo.adornment = kThemeAdornmentDrawIndicatorOnly;
	else if (themebuttonkind == kThemeArrowButton)
		bNewInfo.adornment = kThemeAdornmentArrowDownArrow;
	else if (widgetinfo.state & WTHEME_STATE_HASDEFAULT &&
	         themebuttonkind == kThemePushButton )
	{
		if (!(widgetinfo.state & (WTHEME_STATE_PRESSED | WTHEME_STATE_SUPPRESSDEFAULT)))
			bNewInfo.adornment = kThemeAdornmentDefault;
	}

    Rect t_rect;
	converttonativerect(trect, t_rect);
	
	if (themebuttonkind == kThemeCheckBox || themebuttonkind == kThemeRadioButton)
	{
		if (trect.height < 16)
			if (themebuttonkind == kThemeCheckBox)
				themebuttonkind = kThemeSmallCheckBox;
			else
				themebuttonkind = kThemeSmallRadioButton;
		else
			t_rect.left--;
	}
	else if (themebuttonkind == kThemePushButton || themebuttonkind == kThemePopupButton)
	{
		if (themebuttonkind == kThemePushButton && trect.height < 21)
		{
			t_rect.top--;
			t_rect.left += 2;
			t_rect.right -= 2;
		}
		t_rect.bottom -= 2;
		
		// IM-2014-02-12: [[ Bug 11785 ]] Constrain option button height to 20 px.
		//   Drawing above this height is not supported on OSX with a retina display. 
		if (themebuttonkind == kThemePopupButton && t_rect.bottom - t_rect.top > 20)
		{
			// center within button rect
			uint32_t t_adjust = ((t_rect.bottom - t_rect.top) - 20) / 2;
			t_rect.top += t_adjust;
			t_rect.bottom = t_rect.top + 20;
		}
	}

    converttohirect(&t_rect, &macR);
    
    // Set the kind, too
    bNewInfo.kind = themebuttonkind;
}

//draw theme button to MCScreen
static void drawthemebutton(MCDC *dc, const MCWidgetInfo &widgetinfo, const MCRectangle &drect)
{
	MCThemeDrawInfo t_info;
	t_info . dest = drect;
	getthemebuttonpartandstate(widgetinfo, t_info . button . info, drect, t_info . button . bounds);
	if (t_info . button . info . kind == kThemePushButton && t_info . button . info . adornment == kThemeAdornmentDefault)
	{
		t_info . button . info . animation . time . start = MCMacGetAnimationStartTime();
		t_info . button . info . animation . time . current = MCMacGetAnimationCurrentTime();
	}
	else
		t_info . button . info . animation . time . start = t_info . button . info . animation . time . current = 0;
		
	dc -> drawtheme(THEME_DRAW_TYPE_BUTTON, &t_info);
}


static void drawthemetabs(MCDC *dc, const MCWidgetInfo &widgetinfo, const MCRectangle &drect)
{
	if (widgetinfo.type == WTHEME_TYPE_TABPANE)
	{
		MCThemeDrawInfo t_info;
		t_info . dest = drect;
		
		converttonativerect(drect, t_info . tab_pane . bounds);
		
		t_info . tab_pane . state = (widgetinfo.state & WTHEME_STATE_DISABLED) != 0 ? kThemeStateInactive: kThemeStateActive;
		
		dc -> drawtheme(THEME_DRAW_TYPE_TAB_PANE, &t_info);
	}
	else
	{
		MCThemeDrawInfo t_info;
		t_info.dest = drect;
		
		// MM-2012-11-18: [[ Bug 11456 ]] The height in the dest rect is based on font size.
		//   However, on OS X, tab buttons are always 22 pixels high, irrespective of font size. Was causing clipping with small font sizes.
		t_info . dest . height = 22;
				
		converttonativerect(drect, t_info . tab . bounds);
		t_info . tab . is_hilited = (widgetinfo . state & WTHEME_STATE_HILITED) != 0;
		t_info . tab . is_disabled = (widgetinfo . state & WTHEME_STATE_DISABLED) != 0;
		t_info . tab . is_pressed = (widgetinfo . state & WTHEME_STATE_PRESSED) != 0;
		t_info . tab . is_first = (widgetinfo . attributes & WTHEME_ATT_FIRSTTAB) != 0;
		t_info . tab . is_last = (widgetinfo . attributes & WTHEME_ATT_LASTTAB) != 0;
		dc -> drawtheme(THEME_DRAW_TYPE_TAB, &t_info);
	}
}

static Widget_Part HitTestScrollControls(const MCWidgetInfo &winfo, int2 mx,int2 my, const MCRectangle &drect)
{
	Widget_Part wpart =  WTHEME_PART_THUMB;
	HIPoint mouseLoc;
	mouseLoc.x = mx;
	mouseLoc.y = my;
	HIThemeTrackDrawInfo ttdi;
	fillTrackDrawInfo(winfo,ttdi,drect);
	Boolean inScrollbarArrow = False;
	ControlPartCode partCode;
	// scrollbar needs to check first if mouse-down occurred in arrows
	if (ttdi.kind == kThemeScrollBar || ttdi.kind == kThemeSmallScrollBar)
	{
		// MW-2008-11-02: [[ Bug ]] If this scrollbar is rendered as a little arrows control, then
		//   ensure we hit-test it as such!
		if (drect.height > drect.width && (drect.width == 0 || drect.height / drect.width < 3))
		{
			if (my <= drect . y + drect . height / 2)
				wpart = WTHEME_PART_ARROW_DEC;
			else
				wpart = WTHEME_PART_ARROW_INC;
			
			// MW-2011-10-10: [[ Bug 9797 ]] Make sure we flag ourselves as having
			//   finished the hit test otherwise we invariable end up with the wrong
			//   part code for little arrows.
			inScrollbarArrow = True;
		}
		else
        {
            HIScrollBarTrackInfo t_sbinfo;
            t_sbinfo . version = 0;
            t_sbinfo . pressState = ttdi . trackInfo . scrollbar . pressState;
            t_sbinfo . enableState = ttdi . enableState;
            t_sbinfo . viewsize = ttdi . trackInfo . scrollbar . viewsize;
            if (HIThemeHitTestScrollBarArrows(&ttdi.bounds, &t_sbinfo,
		                                drect.width > drect.height, &mouseLoc,
		                                &ttdi.bounds, &partCode))
            {
                inScrollbarArrow = True;
                if (partCode == kControlUpButtonPart
                        || partCode == kControlLowerUpButtonPart)
                    wpart = WTHEME_PART_ARROW_DEC;
                else if (partCode == kControlDownButtonPart
                         || partCode == kControlUpperDownButtonPart)
                    wpart = WTHEME_PART_ARROW_INC;
                doublesbarrows = partCode == kControlLowerUpButtonPart
                                 || partCode == kControlUpperDownButtonPart;
            }
        }
	}
	if (!inScrollbarArrow)
	{
		if (HIThemeHitTestTrack(&ttdi, &mouseLoc, &partCode))
		{
			if (partCode == kControlPageUpPart)
				wpart = WTHEME_PART_TRACK_DEC;
			else
				if (partCode == kControlPageDownPart)
					wpart = WTHEME_PART_TRACK_INC;
		}//if partCode == kControlIndicatorPart, fall through
	}
	return wpart;
}

static void DrawMacAMScrollControls(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &drect)
{
	MCThemeDrawInfo t_info;
	t_info . dest = drect;
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR && winfo.type != WTHEME_TYPE_SMALLSCROLLBAR)
		return;
	fillTrackDrawInfo(winfo, t_info . slider . info, drect);
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
	
	// MW-2007-08-30: [[ Bug 4155 ]] Ensure the case of endvalue < startvalue is handled correctly.
	if (t_info . slider . info . kind == kThemeSlider)
	{
		if ((winfo.attributes & WTHEME_ATT_SHOWVALUE) != 0)
			t_info . slider . count = (uint2)(fabs(sbinfo->endvalue - sbinfo->startvalue) / (sbinfo->pageinc - sbinfo->lineinc));
		else
			t_info . slider . count = 0;
		dc -> drawtheme(THEME_DRAW_TYPE_SLIDER, &t_info);
	}
	else if (t_info.slider.info.kind == kThemeScrollBar || t_info.slider.info.kind == kThemeSmallScrollBar)
	{
		if (drect.height > drect.width && (drect.width == 0 || drect.height / drect.width < 3))
		{
			converttohirect(drect, t_info . button . bounds);
			t_info . button . bounds . origin . x++;
            t_info . button . bounds . size . width--;
				
			if (winfo.state & WTHEME_STATE_DISABLED)
				t_info . button . info . state = kThemeStateInactive;
			else
				if (winfo.state & WTHEME_STATE_PRESSED)
					if (winfo.part == WTHEME_PART_ARROW_DEC)
						t_info . button . info . state = kThemeStatePressedUp;
					else
						t_info . button . info . state = kThemeStatePressedDown;
				else
					t_info . button . info . state = kThemeStateActive;
			t_info . button . info . adornment = kThemeAdornmentNone;
			t_info . button . info . value = kThemeButtonOff;
			t_info . button . info . kind = kThemeIncDecButton;
			dc -> drawtheme(THEME_DRAW_TYPE_BUTTON, &t_info);
		}
		else
		{
			t_info . scrollbar . horizontal = drect . width > drect . height;
			dc -> drawtheme(THEME_DRAW_TYPE_SCROLLBAR, &t_info);
		}
	}
	else
	{
		// MW-2012-09-17: [[ Bug 9212 ]] Set the phase appropriately for animation.
		int32_t t_steps_per_second, t_millisecs_per_step;
		t_steps_per_second = 30;
		t_millisecs_per_step = 1000 / t_steps_per_second;
		
		// MW-2012-10-01: [[ Bug 10419 ]] Wrap phase at 256 as more frames on Lion+
		//   animation than in Snow Leopard.
		t_info . progress . info . trackInfo . progress . phase = ((int32_t)((MCMacGetAnimationCurrentTime() - MCMacGetAnimationStartTime()) * 1000)) / t_millisecs_per_step % 256;
		dc -> drawtheme(THEME_DRAW_TYPE_PROGRESS, &t_info);
	}
}

//get theme track kind
static ThemeTrackKind getscrollbarkind(Widget_Type wtype)
{
	switch (wtype)
	{
	case WTHEME_TYPE_PROGRESSBAR:
		return kThemeProgressBar;
		break;
	case WTHEME_TYPE_SLIDER:
		return kThemeSlider;
		break;
	case WTHEME_TYPE_SMALLSCROLLBAR:
		return kThemeSmallScrollBar;
		break;
	default:
		return kThemeScrollBar;
	}
}

//fill themetrackinfo with state
static void getscrollbarpressedstate(const MCWidgetInfo &winfo, HIThemeTrackDrawInfo &drawInfo)
{
	ThemeTrackPressState ps = 0;
	if (winfo.state & WTHEME_STATE_PRESSED)
	{
		if (winfo.part == WTHEME_PART_THUMB)  //in scrolling mode, means the thumb is pressed
			ps = kThemeThumbPressed;
		else
		{
			switch (winfo.part)
			{
			case WTHEME_PART_ARROW_DEC:
				{
                    NSString* t_arrow_style = [[NSUserDefaults standardUserDefaults] objectForKey:@"AppleScrollBarVariant"];
                    if (![t_arrow_style isEqualToString:@"DoubleBoth"])
					{
						if (doublesbarrows)
							ps = kThemeLeftInsideArrowPressed;
						else
							ps = kThemeLeftOutsideArrowPressed;
					}
					else
					{
						// MW-2012-09-20: [[ Bug ]] Arrow not highlighting when pressed as
						//   wrong constant was used.
						ps = kThemeLeftInsideArrowPressed;
				}
				}
				break;
			case WTHEME_PART_ARROW_INC:
				if (doublesbarrows)
					ps = kThemeRightInsideArrowPressed;
				else
					ps = kThemeRightOutsideArrowPressed;
				break;
			case WTHEME_PART_TRACK_DEC:
				ps = kThemeLeftTrackPressed;
				break;
			case WTHEME_PART_TRACK_INC:
				ps = kThemeRightTrackPressed;
				break;
			default:
				break;
			}
		}
	}
	if (winfo.type == WTHEME_TYPE_SLIDER)
		drawInfo.trackInfo.slider.pressState = ps;
	else
		drawInfo.trackInfo.scrollbar.pressState = ps;
}

#define SB_MAXVALUE 16384

static void fillTrackDrawInfo(const MCWidgetInfo &winfo, HIThemeTrackDrawInfo &drawInfo, const MCRectangle &drect)
{
	/******************************************************************************
	* fill in the fields in the ThemeTrackDrawInfo structure to be used by other *
	* Appearance Manager routines                                                *
	******************************************************************************/
    drawInfo.version = 0;
	drawInfo.kind = getscrollbarkind(winfo.type);
	if (winfo.datatype != WTHEME_DATA_SCROLLBAR)
		return;
	
	// MW-2010-12-05: [[ Bug 9211 ]] If the height of the control >= 20 then render as a large progress bar.
	if (drawInfo.kind == kThemeProgressBar && drect.height >= 20)
		drawInfo.kind = kThemeLargeProgressBar;
    
    MCRectangle trect = winfo.type == WTHEME_TYPE_SLIDER ? MCU_reduce_rect(drect, 2) : drect;
	convertmctocgrect(trect, drawInfo.bounds);
    
	//FIX
	MCWidgetScrollBarInfo *sbinfo = (MCWidgetScrollBarInfo *)winfo.data;
    
    // MW-2014-04-10: [[ Bug 12027 ]] Calculate all the values in pixels (rather than relative to INT2_MAX). This
    //   means we know the size of the thumb (viewsize) and thus can work-around the HiDPI rendering glitch.
    //   (in HiDPI mode, the min size of thumb that will be rendered is < in non-HiDPI mode; however hittesting
    //    is not adjusted by the HITheme API).
    int t_size;
	if (drect.width > drect.height)
        t_size = drect . width;
    else
        t_size = drect . height;
    
    int t_viewsize, t_maxval, t_val;
    
    if (winfo . type == WTHEME_TYPE_PROGRESSBAR || winfo . type == WTHEME_TYPE_SLIDER)
    {
        t_maxval = t_size;
        if ((sbinfo -> endvalue - sbinfo -> startvalue) != 0.0)
            t_val = (sbinfo -> thumbpos - sbinfo -> startvalue) * t_maxval / (sbinfo -> endvalue - sbinfo -> startvalue);
        else
            t_maxval = 0, t_val = 0;
    }
    else
    {
        if ((sbinfo -> endvalue - sbinfo -> startvalue - sbinfo -> thumbsize) != 0.0)
        {
            t_viewsize = sbinfo -> thumbsize / (sbinfo -> endvalue - sbinfo -> startvalue) * t_size;
            t_maxval = t_size - t_viewsize;
            t_val = (sbinfo -> thumbpos - sbinfo -> startvalue) * t_maxval / (sbinfo -> endvalue - sbinfo -> startvalue - sbinfo -> thumbsize);
        }
        else
            t_maxval = 0, t_val = 0, t_viewsize = 0;
    }

	drawInfo.min = 0;
	drawInfo.max = t_maxval;
	drawInfo.value = t_val;
	
	if (drect.width > drect.height)
		drawInfo.attributes = kThemeTrackHorizontal | kThemeTrackShowThumb;
	else
		drawInfo.attributes = kThemeTrackShowThumb;
	// MW-2012-09-17: [[ Bug ]] If the app is inactive, then render the progressbar
	//   as such.
	if (winfo.state & WTHEME_STATE_DISABLED)
		drawInfo.enableState = kThemeTrackDisabled;
	else if (MCappisactive)
		drawInfo.enableState = kThemeTrackActive;
	else
		drawInfo.enableState = kThemeTrackInactive;
	switch (drawInfo.kind)
	{
	case kThemeProgressBar: //progress bar is always horizontal
		drawInfo.attributes = kThemeTrackHorizontal;
		drawInfo.trackInfo.progress.phase = 0;
		break;
	case kThemeSlider:
		if (drect.width > drect.height)
			drawInfo.bounds.size.height = (drect.y - 1 + 17) - drawInfo . bounds . origin . y;
		else
			drawInfo.bounds.size.width = drawInfo.bounds.origin.x + 17 - drawInfo . bounds . origin . x;
		if (winfo.attributes & WTHEME_ATT_SHOWVALUE)
			drawInfo.trackInfo.slider.thumbDir = kThemeThumbDownward;
		else
			drawInfo.trackInfo.slider.thumbDir = kThemeThumbPlain;
		if (!(winfo.state & WTHEME_STATE_DISABLED))
			getscrollbarpressedstate(winfo,drawInfo);
		break;

		// MW-2005-01-13: Fix bug 2523 - for some reason this was commented out in the recent
		//   past.
	case kThemeSmallScrollBar:
	case kThemeScrollBar:
		if (!(winfo.state & WTHEME_STATE_DISABLED))
			getscrollbarpressedstate(winfo,drawInfo);
		else
			drawInfo . trackInfo . scrollbar . pressState = 0;
			
        drawInfo.trackInfo.scrollbar.viewsize = t_viewsize;
		break;
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////

static inline void assign(HIRect& d, Rect s)
{
	d . origin . x = s . left;
	d . origin . y = s . top;
	d . size . width = s . right - s . left;
	d . size . height = s . bottom - s . top;
}

void MCMacDrawTheme(MCThemeDrawType p_type, MCThemeDrawInfo& p_info, CGContextRef p_context, bool p_hidpi)
{
	CGContextRef t_context = p_context;
	
	switch(p_type)
	{
		case THEME_DRAW_TYPE_SLIDER:
		{
			HIThemeTrackDrawInfo t_info;
			
			t_info . version = 0;
			t_info . kind = p_info . slider . info . kind;
			t_info . bounds = p_info . slider . info . bounds;
			t_info . min = p_info . slider . info . min;
			t_info . max = p_info . slider . info . max;
			t_info . value = p_info . slider . info . value;
			t_info . reserved = 0;
			t_info . attributes = p_info . slider . info . attributes;
			t_info . enableState = p_info . slider . info . enableState;
			t_info . filler1 = 0;
			t_info . trackInfo . slider . thumbDir = p_info . slider . info . trackInfo . slider . thumbDir;
			t_info . trackInfo . slider . pressState = p_info . slider . info . trackInfo . slider . pressState;
			if (p_info . slider . count > 0)
				HIThemeDrawTrackTickMarks(&t_info, p_info . slider . count, t_context, kHIThemeOrientationNormal);
            t_info . bounds . origin . y += 1;
            
			HIThemeDrawTrack(&t_info, NULL, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_SCROLLBAR:
		{
			HIThemeTrackDrawInfo t_info;
			
			t_info . version = 0;
			t_info . kind = p_info . scrollbar . info . kind;
			t_info . bounds = p_info . slider . info . bounds;
			t_info . min = p_info . scrollbar . info . min;
			t_info . max = p_info . scrollbar . info . max;
			t_info . value = p_info . scrollbar . info . value;
			t_info . reserved = 0;
			t_info . attributes = p_info . scrollbar . info . attributes;
			t_info . enableState = p_info . scrollbar . info . enableState;
			t_info . filler1 = 0;
			t_info . trackInfo . scrollbar . viewsize = p_info . scrollbar . info . trackInfo . scrollbar . viewsize;
			t_info . trackInfo . scrollbar . pressState = p_info . scrollbar . info . trackInfo . scrollbar . pressState;
            
            // MW-2014-04-11: [[ Bug 12027 ]] It seems that the minimum size of a scrollbar thumb is 18px. However
            //   when rendering at Retina resolution the HITheme API will render them smaller, even though the
            //   hit-test rect is still 18px. We account for this here.
            if (p_hidpi && t_info . trackInfo . scrollbar . viewsize < 18)
            {
                t_info . trackInfo . scrollbar . viewsize = 18;
            }
            
			HIThemeDrawTrack(&t_info, NULL, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_PROGRESS:
		{
			HIThemeTrackDrawInfo t_info;
			
			t_info . version = 0;
			t_info . kind = p_info . progress . info . kind;
			t_info . bounds = p_info . slider . info . bounds;
			t_info . min = p_info . progress . info . min;
			t_info . max = p_info . progress . info . max;
			t_info . value = p_info . progress . info . value;
			t_info . reserved = 0;
			t_info . attributes = p_info . progress . info . attributes;
			t_info . enableState = p_info . progress . info . enableState;
			t_info . filler1 = 0;
			t_info . trackInfo . progress . phase = p_info . progress . info . trackInfo . progress . phase;
            
			HIThemeDrawTrack(&t_info, NULL, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_BUTTON:
		{
			HIThemeDrawButton(&p_info.button.bounds, &p_info.button.info, t_context, kHIThemeOrientationNormal, NULL);
		}
			break;
			
		case THEME_DRAW_TYPE_GROUP:
		{
			HIRect t_rect;
			HIThemeGroupBoxDrawInfo t_info;
			
			assign(t_rect, p_info . group . bounds);
			
			t_info . version = 0;
			t_info . state = p_info . group . state;
			t_info . kind = p_info . group . is_secondary ? kHIThemeGroupBoxKindSecondary : kHIThemeGroupBoxKindPrimary;
            
			HIThemeDrawGroupBox(&t_rect, &t_info, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_FRAME:
		{
			HIRect t_bounds;
			HIThemeFrameDrawInfo t_info;
			
			assign(t_bounds, p_info . frame . bounds);
			
			t_info . version = 0;
			t_info . kind = p_info . frame . is_list ? kHIThemeFrameListBox : kHIThemeFrameTextFieldSquare;
			t_info . state = p_info . frame . state;
			t_info . isFocused = false;
            
			HIThemeDrawFrame(&t_bounds, &t_info, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_TAB:
		{
			HIRect t_bounds;
			HIThemeTabDrawInfo t_info;
			
			assign(t_bounds, p_info . tab . bounds);
			
			t_info . version = 1;
			t_info . direction = kThemeTabNorth;
			t_info . size = kHIThemeTabSizeNormal;
			if (p_info . tab . is_hilited)
				t_info . style = (p_info . tab . is_disabled ? kThemeTabFrontInactive : kThemeTabFront);
			else if (p_info . tab . is_disabled)
				t_info . style = kThemeTabNonFrontInactive;
			else
				t_info . style = (p_info . tab . is_pressed ? kThemeTabNonFrontPressed : kThemeTabNonFront);
			t_info . adornment = kHIThemeTabAdornmentNone;
			t_info . kind = kHIThemeTabKindNormal;
			if (p_info . tab . is_first && p_info . tab . is_last)
				t_info . position = kHIThemeTabPositionOnly;
			else if (p_info . tab . is_first)
			{
				t_info . adornment = kHIThemeTabAdornmentTrailingSeparator;
				t_info . position = kHIThemeTabPositionFirst;
			}
			else if (p_info . tab . is_last)
				t_info . position = kHIThemeTabPositionLast;
			else
			{
				t_info . adornment = kHIThemeTabAdornmentTrailingSeparator;
				t_info . position = kHIThemeTabPositionMiddle;
			}
            
			HIThemeDrawTab(&t_bounds, &t_info, t_context, kHIThemeOrientationNormal, NULL);
		}
			break;
			
		case THEME_DRAW_TYPE_TAB_PANE:
		{
			HIRect t_bounds;
			HIThemeTabPaneDrawInfo t_info;
			
			// MW-2009-04-26: [[ Bug ]] Incrementing right hand bound is no longer necessary, 
			//   as we've fixed the call sites.
			
			p_info . tab_pane . bounds . top += 1;
			assign(t_bounds, p_info . tab_pane . bounds);
			
			t_info . version = 1;
			t_info . state = p_info . tab_pane . state;
			t_info . direction = kThemeTabNorth;
			t_info . size = kHIThemeTabSizeNormal;
			t_info . kind = kHIThemeTabKindNormal;
			t_info . adornment = kHIThemeTabPaneAdornmentNormal;
            
			HIThemeDrawTabPane(&t_bounds, &t_info, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_BACKGROUND:
		{
			HIRect t_bounds;
			HIThemeBackgroundDrawInfo t_info;
			
			assign(t_bounds, p_info . background . bounds);
			
			t_info . version = 0;
			t_info . state = p_info . background . state;
			t_info . kind = kThemeBackgroundMetal;
            
			HIThemeDrawBackground(&t_bounds, &t_info, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_FOCUS_RECT:
		{
			HIRect t_bounds;
			assign(t_bounds, p_info . focus_rect . bounds);
			
			HIThemeDrawFocusRect(&t_bounds, p_info . focus_rect . focused, t_context, kHIThemeOrientationNormal);
		}
			break;
			
		case THEME_DRAW_TYPE_MENU:
		case THEME_DRAW_TYPE_GTK:
			MCUnreachable();
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Bug 9719 ]] Override to return the actual sizeof(MCThemeDrawInfo).
uint32_t MCNativeTheme::getthemedrawinfosize(void)
{
	return sizeof(MCThemeDrawInfo);
}

bool MCNativeTheme::drawfocusborder(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect)
{
	MCRectangle trect;
	trect = MCU_reduce_rect(p_rect, 3);
	MCThemeDrawInfo t_info;
	t_info.dest = p_rect;
	//MCScreenDC *pms = (MCScreenDC *)MCscreen;
	t_info . focus_rect . focused = True;
	t_info . focus_rect . bounds = MCRectToMacRect(trect);
	p_context -> drawtheme(THEME_DRAW_TYPE_FOCUS_RECT, &t_info);
	
	return true;
}

bool MCNativeTheme::drawmetalbackground(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect, MCObject *p_object)
{
	if (p_context -> gettype() == CONTEXT_TYPE_PRINTER)
		return false;

	uint2 i;
	if (p_object -> getcindex(DI_BACK, i) || p_object -> getpindex(DI_BACK, i))
		return false;

	MCThemeDrawInfo p_info;
	p_info.dest = p_rect;
	p_info . background . bounds . left = p_dirty . x;
	p_info . background . bounds . top = p_dirty . y;
	p_info . background . bounds . right = p_dirty . x + p_dirty . width;
	p_info . background . bounds . bottom = p_dirty . y + p_dirty . height;
	
	Window t_window;
	t_window = p_object -> getstack() -> getwindow();
    p_info . background . state = kThemeStateActive;

	MCRectangle t_clip;
	t_clip = p_context -> getclip();
	
	p_context->save();
	p_context->cliprect(MCU_intersect_rect(p_rect, p_dirty));
	p_context -> drawtheme(THEME_DRAW_TYPE_BACKGROUND, &p_info);

	p_context->restore();
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCTheme *MCThemeCreateNative(void)
{
	return new MCNativeTheme;
}

////////////////////////////////////////////////////////////////////////////////

extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
extern bool MCMacPlatformGetImageColorSpace(CGColorSpaceRef &r_colorspace);

// IM-2014-01-24: [[ HiDPI ]] Factor out creation of CGBitmapContext for MCImageBitmap
bool MCOSXCreateCGContextForBitmap(MCImageBitmap *p_bitmap, CGContextRef &r_context)
{
	bool t_success;
	t_success = true;
	
	CGContextRef t_cgcontext = nil;
	CGColorSpaceRef t_colorspace = nil;
	
	t_success = MCMacPlatformGetImageColorSpace(t_colorspace);
	
	if (t_success)
	{
		// IM-2013-08-21: [[ RefactorGraphics ]] Refactor CGImage creation code to be pixel-format independent
		CGBitmapInfo t_bitmap_info;
		t_bitmap_info = MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true);
		
		t_success = nil != (t_cgcontext = CGBitmapContextCreate(p_bitmap->data, p_bitmap->width, p_bitmap->height, 8, p_bitmap->stride, t_colorspace, t_bitmap_info));
	}
	
	if (t_colorspace != nil)
		CGColorSpaceRelease(t_colorspace);
	
	
	if (t_success)
		r_context = t_cgcontext;
	
	return t_success;
}
		

bool MCThemeDraw(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr)
{
	bool t_success = true;
	
	MCImageBitmap *t_bitmap = nil;
	CGContextRef t_cgcontext = nil;
	MCRectangle t_rect;

	t_rect = p_info_ptr->dest;
	
	MCGAffineTransform t_transform;
	t_transform = MCGContextGetDeviceTransform(p_context);
	
	CGAffineTransform t_cg_transform;
	t_cg_transform = CGAffineTransformIdentity;
	
	MCGRectangle t_dst;
	uint32_t t_width, t_height;
	int32_t t_x, t_y;
	
	// IM-2014-01-23: [[ HiDPI ]] OSX theme rendering only supports 2x scale transform,
	// so limit transform scale to 1.0 or 2.0
	MCGFloat t_ui_scale;
	t_ui_scale = 1.0;
	
	if (MCGAffineTransformIsRectangular(t_transform))
	{
		MCGFloat t_transform_scale;
		t_transform_scale = MCGAffineTransformGetEffectiveScale(t_transform);
		
		if (t_transform_scale > 1.0)
			t_ui_scale = 2.0;
	}
	
	// IM-2014-01-20: [[ HiDPI ]] Improve scaled UI appearance by rendering to the temporary bitmap with the scale transform.
	if (t_ui_scale != 1.0)
	{
		t_cg_transform = CGAffineTransformMakeScale(t_ui_scale, t_ui_scale);
		
		// calculate scaled destination rect and device buffer size & origin
		MCGRectangle t_scaled_bounds;
		t_scaled_bounds = MCGRectangleScale(MCRectangleToMCGRectangle(t_rect), t_ui_scale);
		
		MCRectangle t_int_bounds;
		t_int_bounds = MCGRectangleGetIntegerBounds(t_scaled_bounds);
		
		t_width = t_int_bounds.width;
		t_height = t_int_bounds.height;
		
		t_x = t_int_bounds.x;
		t_y = t_int_bounds.y;
		
		// Caculate new destination rect for temporary image
		t_dst = MCGRectangleScale(MCRectangleToMCGRectangle(t_int_bounds), 1 / t_ui_scale);
	}
	else
	{
		// render at normal size & draw into target rect
		t_x = t_rect.x;
		t_y = t_rect.y;
		
		t_width = t_rect.width;
		t_height = t_rect.height;
		
		t_dst = MCGRectangleMake(t_x, t_y, t_width, t_height);
	}
	
	t_success = MCImageBitmapCreate(t_width, t_height, t_bitmap);
	
	if (t_success)
	{
		MCImageBitmapClear(t_bitmap);
		
		t_success = MCOSXCreateCGContextForBitmap(t_bitmap, t_cgcontext);
	}
	
	if (t_success)
	{
		// Invert y-axis origin to top-left of bitmap
		CGContextTranslateCTM(t_cgcontext, 0, (CGFloat)t_bitmap->height);
		CGContextScaleCTM(t_cgcontext, 1.0, -1.0);
		
		// Relocate origin to top-left of destination rect
		CGContextTranslateCTM(t_cgcontext, -(CGFloat)t_x, -(CGFloat)t_y);
		
		// Apply UI scaling transform
		CGContextConcatCTM(t_cgcontext, t_cg_transform);
		
		MCMacDrawTheme(p_type, *p_info_ptr, t_cgcontext, t_ui_scale > 1.0);
		
		CGContextRelease(t_cgcontext);
		
		MCGRaster t_raster;
		t_raster.width = t_bitmap->width;
		t_raster.height = t_bitmap->height;
		t_raster.pixels = t_bitmap->data;
		t_raster.stride = t_bitmap->stride;
		t_raster.format = kMCGRasterFormat_ARGB;
		
		// MM-2013-12-16: [[ Bug 11567 ]] Use bilinear filter when drawing theme elements.
        // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was bilinear).
		MCGContextDrawPixels(p_context, t_raster, t_dst, kMCGImageFilterMedium);
	}
	
	if (t_bitmap != nil)
		MCImageFreeBitmap(t_bitmap);
		
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
