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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "util.h"
#include "group.h"
#include "image.h"
#include "menuparse.h"
#include "stacklst.h"
#include "font.h"

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

void MCButton::UpdateIconAndMenus(void)
{
	reseticon();
	freemenu(False);
	findmenu(true);
	if (parent && parent->gettype() == CT_GROUP)
	{
		parent->setstate(True, CS_NEED_UPDATE);
		if ((parent.GetAs<MCGroup>() == MCmenubar || parent.GetAs<MCGroup>() == MCdefaultmenubar) && !MClockmenus)
			MCscreen->updatemenubar(True);
	}
	Redraw();
}

////////////////////////////////////////////////////////////////////////////////

enum MCInterfaceButtonStyle
{
	kMCButtonStyleStandard = F_STANDARD,
	kMCButtonStyleRoundrect = F_ROUNDRECT,
	kMCButtonStyleCheck = F_CHECK,
	kMCButtonStyleRadio = F_RADIO,
	kMCButtonStyleMenu = F_MENU,
	kMCButtonStyleOval = F_OVAL_BUTTON,
	kMCButtonStyleRectangle = F_RECTANGLE,
	kMCButtonStylePopup,
	kMCButtonStyleTransparent,
	kMCButtonStyleShadow,
	kMCButtonStyleShowBorder,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceButtonStyleElementInfo[] =
{	
	{ MCstandardstring, kMCButtonStyleStandard, false },
	{ "", kMCButtonStyleStandard, false },
	{ MCmenustring, kMCButtonStyleMenu, false },
	{ MCpopupstring, kMCButtonStylePopup, false },
	{ MCcheckboxstring, kMCButtonStyleCheck, false },
	{ MCradiobuttonstring, kMCButtonStyleRadio, false },
	{ MCroundrectstring, kMCButtonStyleRoundrect, false },
	{ MCrectanglestring, kMCButtonStyleRectangle, false },
	{ MCovalstring, kMCButtonStyleOval, false },
	{ MCtransparentstring, kMCButtonStyleTransparent, false },
	{ MCshadowstring, kMCButtonStyleShadow, false },
	{ MCopaquestring, kMCButtonStyleShowBorder, false },
};

static MCExecEnumTypeInfo _kMCInterfaceButtonStyleTypeInfo =
{
	"Interface.ButtonStyle",
	sizeof(_kMCInterfaceButtonStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceButtonStyleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceButtonMenuModeElementInfo[] =
{	
	{ MCtabstring, WM_TOP_LEVEL, false },
	{ MCpulldownstring, WM_PULLDOWN, false },
	{ MCpopupstring, WM_POPUP, false },
	{ MCoptionstring, WM_OPTION, false },
	{ MCcascadestring, WM_CASCADE, false },
	{ MCcombostring, WM_COMBO, false },
	{ "", WM_CLOSED, false },
};

static MCExecEnumTypeInfo _kMCInterfaceButtonMenuModeTypeInfo =
{
	"Interface.ButtonMenuMode",
	sizeof(_kMCInterfaceButtonMenuModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceButtonMenuModeElementInfo
};

//////////

enum MCButtonAcceleratorModifiers
{
	kMCButtonAcceleratorShiftBit = 0,
	kMCButtonAcceleratorShift = 1 << kMCButtonAcceleratorShiftBit,
	kMCButtonAcceleratorControlBit = 1,
	kMCButtonAcceleratorControl = 1 << kMCButtonAcceleratorControlBit,
	kMCButtonAcceleratorMod1Bit = 2,
	kMCButtonAcceleratorMod1 = 1 << kMCButtonAcceleratorMod1Bit,
#ifdef _MAC_DESKTOP
	kMCButtonAcceleratorMacControlBit = 3,
	kMCButtonAcceleratorMacControl = 1 << kMCButtonAcceleratorMacControlBit,
#endif
	kMCButtonAcceleratorCommandBit = 4,
	kMCButtonAcceleratorCommand = 1 << kMCButtonAcceleratorCommandBit,
	kMCButtonAcceleratorOptionBit = 5,
	kMCButtonAcceleratorOption = 1 << kMCButtonAcceleratorOptionBit
};

static MCExecSetTypeElementInfo _kMCInterfaceButtonAcceleratorModifiersElementInfo[] =
{
	{ MCshiftstring, kMCButtonAcceleratorShiftBit },
#ifdef _MAC_DESKTOP
	{ MCcommandstring, kMCButtonAcceleratorControlBit },
	{ MCcontrolstring, kMCButtonAcceleratorMacControlBit },
#else
	{ MCcontrolstring, kMCButtonAcceleratorControlBit },
	{ MCcommandstring, kMCButtonAcceleratorCommandBit },
#endif
	{ MCmod1string, kMCButtonAcceleratorMod1Bit },
	{ MCoptionstring, kMCButtonAcceleratorOptionBit },
};

static MCExecSetTypeInfo _kMCInterfaceButtonAcceleratorModifiersTypeInfo =
{
	"Interface.AcceleratorModifiers",
	sizeof(_kMCInterfaceButtonAcceleratorModifiersElementInfo) / sizeof(MCExecSetTypeElementInfo),
	_kMCInterfaceButtonAcceleratorModifiersElementInfo
};

//////////

enum MCInterfaceButtonIconType
{
    kMCInterfaceButtonIconId,
    kMCInterfaceButtonIconCustom
};

struct MCInterfaceButtonIcon
{
    MCInterfaceButtonIconType type;
    
    union
    {
        MCStringRef custom;
        uint4 id;
    };
};

static void MCInterfaceButtonIconParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceButtonIcon& r_output)
{
    if (MCU_stoui4(p_input, r_output . id))
        r_output . type = kMCInterfaceButtonIconId;
    else
    {
        r_output . type = kMCInterfaceButtonIconCustom;
        r_output . custom = MCValueRetain(p_input);
    }
}

static void MCInterfaceButtonIconFormat(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_input, MCStringRef& r_output)
{
    if (MCStringFormat(r_output, "%d", p_input . id))
        return;
    
    ctxt . Throw();
}

static void MCInterfaceButtonIconFree(MCExecContext& ctxt, MCInterfaceButtonIcon& p_input)
{
    if (p_input . type == kMCInterfaceButtonIconCustom && p_input . custom != nil)
        MCValueRelease(p_input . custom);
}

static MCExecCustomTypeInfo _kMCInterfaceButtonIconTypeInfo =
{
	"Interface.ButtonIcon",
	sizeof(MCInterfaceButtonIcon),
	(void *)MCInterfaceButtonIconParse,
	(void *)MCInterfaceButtonIconFormat,
	(void *)MCInterfaceButtonIconFree
};

//////////

// SN-2014-06-25: [[ MERGE-6.7 ]] Definition of the iconGravity relocated
// MW-2014-06-19: [[ IconGravity ]] Strings for the 'iconGravity' property.
static MCExecEnumTypeElementInfo _kMCInterfaceButtonIconGravityElementInfo[] =
{
	{ "", kMCGravityNone, false },
	{ "left", kMCGravityLeft, false },
	{ "right", kMCGravityRight, false },
	{ "bottom", kMCGravityBottom, false },
    // AL-2014-09-12: [[ Bug 13422 ]] Reinstate "top" value for gravity
    { "top", kMCGravityTop, false },
	{ "topLeft", kMCGravityTopLeft, false },
	{ "topRight", kMCGravityTopRight, false },
	{ "bottomLeft", kMCGravityBottomLeft, false },
	{ "bottomRight", kMCGravityBottomRight, false },
    { "center", kMCGravityCenter, false },
    { "resize", kMCGravityResize, false },
    { "resizeAspect", kMCGravityResizeAspect, false },
    { "resizeAspectFill", kMCGravityResizeAspectFill, false }
};

static MCExecEnumTypeInfo _kMCInterfaceButtonIconGravityTypeInfo =
{
	"Interface.ButtonGravity",
	sizeof(_kMCInterfaceButtonIconGravityElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceButtonIconGravityElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceButtonStyleTypeInfo = &_kMCInterfaceButtonStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceButtonMenuModeTypeInfo = &_kMCInterfaceButtonMenuModeTypeInfo;
MCExecSetTypeInfo *kMCInterfaceButtonAcceleratorModifiersTypeInfo = &_kMCInterfaceButtonAcceleratorModifiersTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceButtonIconTypeInfo = &_kMCInterfaceButtonIconTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceButtonIconGravityTypeInfo = &_kMCInterfaceButtonIconGravityTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCButton::SetName(MCExecContext& ctxt, MCStringRef p_name)
{
	MCObject::SetName(ctxt, p_name);

	if (!ctxt . HasError())
	{
		clearmnemonic();
		setupmnemonic();
	}
}

void MCButton::GetStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	uint4 t_style;
	t_style = getstyleint(flags);

	if (t_style == F_MENU && getstack() -> hcaddress())
		r_style = kMCButtonStylePopup;
	else if (F_ROUNDRECT <= t_style && t_style <= F_RECTANGLE)
		r_style = (MCInterfaceButtonStyle)t_style;
	else if (!(flags & F_OPAQUE))
		r_style = kMCButtonStyleTransparent;
	else if (flags & F_SHADOW)
		r_style = kMCButtonStyleShadow;
	else if (!(flags & F_SHOW_BORDER))
		r_style = kMCButtonStyleShowBorder;
	else
		r_style = kMCButtonStyleStandard;
}

void MCButton::SetStyle(MCExecContext& ctxt, intenum_t p_style)
{
	flags &= ~(F_STYLE | F_DISPLAY_STYLE | F_ALIGNMENT);
	if (entry != NULL)
		deleteentry();

	switch (p_style)
	{
	case kMCButtonStyleStandard:
		flags |= F_STANDARD | F_SHOW_BORDER | F_OPAQUE
				| F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;
		break;
	case kMCButtonStyleMenu:
	case kMCButtonStylePopup:
		flags |= F_MENU | F_SHOW_BORDER | F_OPAQUE | F_ALIGN_CENTER | F_ARM_BORDER;
		if (menumode == WM_COMBO)
			createentry();
		if (menumode == WM_TOP_LEVEL)
		{
			MCValueRelease(tabs);
			/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
		}
		break;
	case kMCButtonStyleCheck:
		flags |= F_CHECK | F_ALIGN_LEFT;
		break;
	case kMCButtonStyleRadio:
		flags |= F_RADIO | F_ALIGN_LEFT;
		flags &= ~F_SHARED_HILITE;
		break;
	case kMCButtonStyleRoundrect:
		flags |=  F_ROUNDRECT | F_SHOW_BORDER | F_OPAQUE | F_ALIGN_CENTER | F_HILITE_FILL;
		break;
	case kMCButtonStyleRectangle:
		flags |= F_RECTANGLE | F_SHOW_BORDER | F_OPAQUE
		        | F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;
		break;
	case kMCButtonStyleOval:
		flags |= F_OVAL_BUTTON | F_HILITE_FILL;
		break;
	case kMCButtonStyleTransparent:
		flags |= F_STANDARD | F_ALIGN_CENTER;
		break;
	case kMCButtonStyleShadow:
		flags |= F_STANDARD | F_SHOW_BORDER | F_OPAQUE
				| F_SHADOW | F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;
		break;
	case kMCButtonStyleShowBorder:
		flags |= F_STANDARD | F_OPAQUE
		        | F_ALIGN_CENTER | F_HILITE_FILL | F_ARM_BORDER;
		break;
	default:
		break;
	}
	// MW-2011-09-21: [[ Layers ]] Make sure the layerattrs are recomputed.
	m_layer_attr_changed = true;
	Redraw();
}

void MCButton::GetAutoArm(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_AUTO_ARM);
}

void MCButton::SetAutoArm(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_AUTO_ARM))
		Redraw();
}

void MCButton::GetAutoHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_AUTO_HILITE);
}

void MCButton::SetAutoHilite(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_AUTO_HILITE))
		Redraw();
}

void MCButton::GetArmBorder(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_ARM_BORDER);
}

void MCButton::SetArmBorder(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_ARM_BORDER))
	{
		// MW-2011-09-21: [[ Layers ]] Changing the armBorder property
		//   affects the layer attrs.
		m_layer_attr_changed = true;
		Redraw();
	}
}

void MCButton::GetArmFill(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_ARM_FILL);
}

void MCButton::SetArmFill(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_ARM_FILL))
		Redraw();
}

void MCButton::GetHiliteBorder(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_HILITE_BORDER);
}

void MCButton::SetHiliteBorder(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_HILITE_BORDER))
	{
		// MW-2011-09-21: [[ Layers ]] Changing the hiliteBorder property
		//   affects the layer attrs.
		m_layer_attr_changed = true;
		Redraw();
	}
}

void MCButton::GetHiliteFill(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_HILITE_FILL);
}

void MCButton::SetHiliteFill(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_HILITE_FILL))
		Redraw();
}

void MCButton::GetShowHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_HILITE);
}

void MCButton::SetShowHilite(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_HILITE))
		Redraw();
}

void MCButton::GetArm(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getstate(CS_ARMED);
}

void MCButton::SetArm(MCExecContext& ctxt, bool setting)
{
	if (changestate(setting, CS_ARMED))
		Redraw();
}

void MCButton::DoGetIcon(MCExecContext& ctxt, Current_icon which, MCInterfaceButtonIcon& r_icon)
{
    r_icon . type = kMCInterfaceButtonIconId;
	r_icon . id = icons == NULL ? 0 : icons->iconids[which];
}

void MCButton::DoSetIcon(MCExecContext& ctxt, Current_icon which, const MCInterfaceButtonIcon& p_icon)
{
    uint4 t_id;
    
	if (icons == NULL)
	{
		icons = new (nothrow) iconlist;
		memset(icons, 0, sizeof(iconlist));
	}
    
	if (p_icon . type == kMCInterfaceButtonIconCustom)
	{
		// MW-2013-03-06: [[ Bug 10695 ]] When searching for the image to resolve to an id,
		//   make sure we use the behavior aware search function.
		MCImage *ticon = resolveimagename(p_icon . custom);
		if (ticon != NULL)
			t_id = ticon->getid();
		else
			t_id = 0;
	}
    else
    {
        t_id = p_icon . id;
    }
    
	if (icons->iconids[which] != t_id)
	{
		icons->iconids[which] = t_id;

		if (icons->iconids[CI_ARMED] == 0 && icons->iconids[CI_DISABLED] == 0
		        && icons->iconids[CI_HILITED] == 0 && icons->iconids[CI_DEFAULT] == 0
						&& icons->iconids[CI_VISITED] == 0 && icons->iconids[CI_HOVER] == 0)
		{
			flags &= ~(F_SHOW_ICON | F_HAS_ICONS);
			if (icons->curicon != NULL)
			{
				icons->curicon->close();
			}
			delete icons;
			icons = NULL;
		}
		else
		{
			flags |= F_SHOW_ICON | F_HAS_ICONS;
			reseticon();
		}
	}
	
	Redraw();
}

void MCButton::SetArmedIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_ARMED, p_icon);
}

void MCButton::GetArmedIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_ARMED, r_icon);
}

void MCButton::SetDisabledIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_DISABLED, p_icon);
}

void MCButton::GetDisabledIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_DISABLED, r_icon);    
}

void MCButton::SetIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_DEFAULT, p_icon);
}

void MCButton::GetIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_DEFAULT, r_icon);    
}

void MCButton::SetHiliteIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_HILITED, p_icon);
}

void MCButton::GetHiliteIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_HILITED, r_icon);    
}

void MCButton::SetVisitedIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_VISITED, p_icon);
}

void MCButton::GetVisitedIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_VISITED, r_icon);    
}

void MCButton::SetHoverIcon(MCExecContext& ctxt, const MCInterfaceButtonIcon& p_icon)
{
    DoSetIcon(ctxt, CI_HOVER, p_icon);
}

void MCButton::GetHoverIcon(MCExecContext& ctxt, MCInterfaceButtonIcon& r_icon)
{
    DoGetIcon(ctxt, CI_HOVER, r_icon);    
}

void MCButton::GetIconGravity(MCExecContext &ctxt, intenum_t &r_gravity)
{
    r_gravity = (intenum_t)m_icon_gravity;
}

void MCButton::SetIconGravity(MCExecContext &ctxt, intenum_t p_gravity)
{
    m_icon_gravity = (MCGravity)p_gravity;
    
    // AL-2014-09-12: [[ Bug 13422 ]] Redraw after setting iconGravity
    Redraw();
}

void MCButton::GetSharedHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHARED_HILITE);
}

void MCButton::SetSharedHilite(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHARED_HILITE))
		Redraw();
}
void MCButton::GetShowIcon(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_ICON);
}

void MCButton::SetShowIcon(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_ICON))
	{
		// MW-2011-09-21: [[ Layers ]] Changing the showIcon property
		//   affects the layer attrs.
		m_layer_attr_changed = true;
		Redraw();
	}
}

void MCButton::GetShowName(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_NAME);
}

void MCButton::SetShowName(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_NAME))
	{
		// MW-2011-09-21: [[ Layers ]] Changing the showName property
		//   affects the layer attrs.
		m_layer_attr_changed = true;
		Redraw();
	}
}

void MCButton::GetLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	if (entry != NULL)
		r_label = MCValueRetain(getlabeltext());
	else
		r_label = MCValueRetain(label);
}

void MCButton::SetLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	// Make sure the label is up to date
	if (entry != NULL)
		getentrytext();
	
	// Don't make any changes if it isn't necessary
	if (MCStringIsEqualTo(p_label, label, kMCStringOptionCompareExact))
		return;
	
	MCValueAssign(label, p_label);
    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
    //  we need to rely on the F_LABEL flag
    // AL-2014-09-10: [[ Bug 13401 ]] If label is empty, the flag should reflect the lack of label
    if (MCStringIsEmpty(label))
        flags &= ~F_LABEL;
    else
        flags |= F_LABEL;

	if (entry != NULL)
		entry->settext(0, label, False);
	
	clearmnemonic();
	setupmnemonic();
	Redraw();
}

void MCButton::GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label)
{
	MCStringRef t_label = nil;
	GetLabel(ctxt, t_label);
	if (MCStringEncodeAndRelease(t_label, kMCStringEncodingUTF16, false, r_label))
		return;
	MCValueRelease(t_label);
	
	ctxt.Throw();
}

void MCButton::SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label)
{
	MCAutoStringRef t_new_label;
	if (MCStringDecode(p_label, kMCStringEncodingUTF16, false, &t_new_label))
	{
		SetLabel(ctxt, *t_new_label);
		return;
	}
	
	ctxt.Throw();
}

void MCButton::GetEffectiveLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	r_label = MCValueRetain(getlabeltext());
}

void MCButton::GetEffectiveUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label)
{
	MCStringRef t_label = nil;
	GetEffectiveLabel(ctxt, t_label);
	if (MCStringEncodeAndRelease(t_label, kMCStringEncodingUTF16, false, r_label))
		return;
	MCValueRelease(t_label);
	
	ctxt.Throw();
}

void MCButton::GetLabelWidth(MCExecContext& ctxt, uinteger_t& r_width)
{
	r_width = labelwidth;
}

void MCButton::SetLabelWidth(MCExecContext& ctxt, uinteger_t p_width)
{
	labelwidth = p_width;
	if (labelwidth == 0)
		flags &= ~F_LABEL_WIDTH;
	else
		flags |= F_LABEL_WIDTH;
	Redraw();
}

void MCButton::GetFamily(MCExecContext& ctxt, uinteger_t& r_family)
{
	r_family = family;
}

void MCButton::SetFamily(MCExecContext& ctxt, uinteger_t p_family)
{
	family = p_family;
}

void MCButton::GetVisited(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getstate(CS_VISITED) == True;
}

void MCButton::SetVisited(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changestate(setting, CS_VISITED);

	reseticon();

	if (t_dirty)
		Redraw();
}

void MCButton::GetMenuHistory(MCExecContext& ctxt, uinteger_t& r_history)
{
	r_history = menuhistory;
}

void MCButton::SetMenuHistory(MCExecContext& ctxt, uinteger_t p_history)
{
	setmenuhistory(p_history);
}

void MCButton::GetMenuLines(MCExecContext& ctxt, uinteger_t*& r_lines)
{
	*r_lines = (uinteger_t)menulines;
}

void MCButton::SetMenuLines(MCExecContext& ctxt, uinteger_t* p_lines)
{
	if (p_lines != nil)
	{
		menulines = *p_lines;
		flags |= F_MENU_LINES;
	}
	else
	{
		flags &= ~F_MENU_LINES;
		menulines = DEFAULT_MENU_LINES;
	}
	freemenu(False);
}

void MCButton::GetMenuButton(MCExecContext& ctxt, uinteger_t& r_button)
{
	r_button = menubutton;
}

void MCButton::SetMenuButton(MCExecContext& ctxt, uinteger_t p_button)
{
	menubutton = p_button;
}

void MCButton::GetMenuMode(MCExecContext& ctxt, intenum_t& r_mode)
{
	r_mode = (intenum_t)menumode;
}

void MCButton::SetMenuMode(MCExecContext& ctxt, intenum_t p_mode)
{
	if (entry != nil)
		deleteentry();
	else
		freemenu(False);

	setmenumode((uint1)p_mode);
	
	if (p_mode == WM_COMBO)
		createentry();
	else if (p_mode == WM_TOP_LEVEL)
	{
		if (getstyleint(flags) == F_MENU)
		{
			MCValueRelease(tabs);
			/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
		}
	}
	
	Redraw();
}

void MCButton::GetMenuName(MCExecContext& ctxt, MCNameRef& r_name)
{
	r_name = MCValueRetain(menuname);
}

void MCButton::SetMenuName(MCExecContext& ctxt, MCNameRef p_name)
{
	freemenu(False);
	MCValueAssign(menuname, p_name);

	if (opened)
	{
		if (findmenu(true) && menu.IsValid())
			menu->installaccels(getstack());
	}
}

void MCButton::SetShowBorder(MCExecContext& ctxt, bool setting)
{
	MCControl::SetShowBorder(ctxt, setting);
	if (MCaqua && menumode == WM_PULLDOWN)
	{
		freemenu(False);
		findmenu(true);
	}
	Redraw();
}

void MCButton::GetAcceleratorText(MCExecContext& ctxt, MCStringRef& r_text)
{
	r_text = MCValueRetain(acceltext);
}

void MCButton::SetAcceleratorText(MCExecContext& ctxt, MCStringRef p_text)
{
	if (MCStringIsEqualTo(acceltext, p_text, kMCStringOptionCompareExact))
		return;
	MCValueAssign(acceltext, p_text);
	Redraw();
}

void MCButton::GetUnicodeAcceleratorText(MCExecContext& ctxt, MCDataRef& r_text)
{
	MCDataRef t_text = nil;
	if (MCStringEncode(acceltext, kMCStringEncodingUTF16, false, t_text))
	{
		r_text = t_text;
		return;
	}
	
	ctxt.Throw();
}

void MCButton::GetAcceleratorKey(MCExecContext& ctxt, MCStringRef& r_key)
{
	if (accelkey & 0xFF00)
	{
		const char *t_keyname = MCLookupAcceleratorName(accelkey);
		if (t_keyname == NULL || MCStringCreateWithCString(t_keyname, r_key))
			return;
	}
    else if (accelkey)
    {
        char t_accel_key = (char)accelkey;
        if (MCStringFormat(r_key, "%c", t_accel_key))
            return;
    }
    else
        return;

	ctxt . Throw();
}

void MCButton::SetAcceleratorKey(MCExecContext& ctxt, MCStringRef p_name)
{
	if (p_name != nil)
	{
		accelkey = MCStringGetCharAtIndex(p_name, 0);
		if (MCStringGetLength(p_name) > 1)
		{
			uint4 t_accelkey = MCLookupAcceleratorKeysym(p_name);
			if (t_accelkey != 0)
				accelkey = t_accelkey;
		}
	}
	else
		accelkey = 0;
	MCstacks->changeaccelerator(this, accelkey, accelmods);
}


void MCButton::GetAcceleratorModifiers(MCExecContext& ctxt, intset_t& r_mods)
{
	r_mods = (intset_t)accelmods;
}

void MCButton::SetAcceleratorModifiers(MCExecContext& ctxt, intset_t p_mods)
{
	if (p_mods & kMCButtonAcceleratorCommand)
	{
		p_mods ^= kMCButtonAcceleratorCommand;
		p_mods |= kMCButtonAcceleratorControl;
	}

	if (p_mods & kMCButtonAcceleratorOption)
	{
		p_mods ^= kMCButtonAcceleratorOption;
		p_mods |= kMCButtonAcceleratorMod1;
	}
	accelmods = (uint1)p_mods;
	MCstacks -> changeaccelerator(this, accelkey, accelmods);
}


void MCButton::GetMnemonic(MCExecContext& ctxt, uinteger_t& r_mnemonic)
{
	r_mnemonic = mnemonic;
}

void MCButton::SetMnemonic(MCExecContext& ctxt, uinteger_t p_mnemonic)
{		
	clearmnemonic();
	mnemonic = p_mnemonic;
	setupmnemonic();
	Redraw();
}

void MCButton::GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width)
{
    // MW-2012-02-16: [[ FontRefs ]] As 'formatted' properties require
	//   access to the font, we must be open before we can compute them.
	if (opened)
	{
        // MW-2007-07-05: [[ Bug 2328 ]] - Formatted width of tab buttons incorrect.
		if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
			r_width = (uinteger_t)formattedtabwidth();
		else
		{
			uint2 fwidth;
			
			MCStringRef const t_label = getlabeltext();
			if (MCStringIsEmpty(t_label))
				fwidth = 0;
			else
            {
                MCAutoArrayRef lines;
                MCStringSplit(t_label, MCSTR("\n"), nil, kMCCompareExact, &lines);
                uindex_t line_count = MCArrayGetCount(*lines);
                int32_t max_length = 0;
                
                for (uindex_t i = 0; i < line_count; ++i)
                {
                    MCValueRef line = nil;
                    MCArrayFetchValueAtIndex(*lines, i + 1, line);
                    MCStringRef const string_line = static_cast<MCStringRef const>(line);
                    
                    int32_t string_line_length = MCFontMeasureText(m_font, string_line, getstack() -> getdevicetransform());
                    
                    max_length = MCMax<int32_t>(max_length, string_line_length);
                }
                fwidth = leftmargin + rightmargin + max_length;
            }
			if (flags & F_SHOW_ICON && icons != NULL)
			{
				reseticon();
				if (icons->curicon != NULL)
				{
					MCRectangle trect = icons->curicon->getrect();
					if (trect.width > fwidth)
						fwidth = trect.width;
				}
			}
			else
            {
				if (getstyleint(flags) == F_CHECK || getstyleint(flags) == F_RADIO)
                {
                    fwidth += leftmargin + GetCheckSize();
                }
            }
			if (menumode == WM_OPTION)
				fwidth += optionrect.width + (optionrect.width >> 1);
			if (menumode == WM_CASCADE)
				fwidth += rect.height;
			r_width = fwidth;
		}
	}
	else
		r_width = 0;
}
void MCButton::GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height)
{
	// MW-2012-02-16: [[ FontRefs ]] As 'formatted' properties require
	//   access to the font, we must be open before we can compute them.
	if (opened)
	{
		uint2 fheight;
		fheight = topmargin + bottommargin + MCFontGetAscent(m_font) + MCFontGetDescent(m_font);
		if (flags & F_SHOW_ICON && icons != NULL)
		{
			reseticon();
			if (icons->curicon != NULL)
			{
				MCRectangle trect = icons->curicon->getrect();
				if (trect.height > fheight)
					fheight = trect.height;
			}
		}
		else if ((getstyleint(flags) == F_CHECK || getstyleint(flags) == F_RADIO) && GetCheckSize() > fheight)
			fheight = GetCheckSize();
		else if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
			fheight += 8;
		r_height = fheight;
	}
	else
		r_height = 0;
}

void MCButton::GetDefault(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DEFAULT);
}

void MCButton::SetDefault(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_DEFAULT) && opened && 
		((flags & F_DEFAULT) != 0) != ((state & CS_SHOW_DEFAULT) != 0))
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();
		if (flags & F_DEFAULT)
		{
			getcard()->setdefbutton(this);
			state |= CS_SHOW_DEFAULT;
		}
		else
		{
			getcard()->freedefbutton(this);
			state &= ~CS_SHOW_DEFAULT;
		}
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		layer_transientchangedandredrawall(t_old_trans);
	}
}	

void MCButton::SetTextFont(MCExecContext& ctxt, MCStringRef p_font)
{
	MCObject::SetTextFont(ctxt, p_font);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetTextFont(ctxt, p_font);

		UpdateIconAndMenus();
	}
}

void MCButton::SetTextHeight(MCExecContext& ctxt, uinteger_t* p_height)
{
	MCObject::SetTextHeight(ctxt, p_height);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetTextHeight(ctxt, p_height);

		UpdateIconAndMenus();
	}
}

void MCButton::SetTextSize(MCExecContext& ctxt, uinteger_t* p_size)
{
	MCObject::SetTextSize(ctxt, p_size);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetTextSize(ctxt, p_size);

		UpdateIconAndMenus();
	}
}

void MCButton::SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style)
{
	MCObject::SetTextStyle(ctxt, p_style);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetTextStyle(ctxt, p_style);

		UpdateIconAndMenus();
	}
}

void MCButton::SetEnabled(MCExecContext& ctxt, uint32_t part, bool flag)
{
	MCObject::SetEnabled(ctxt, part, flag);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetEnabled(ctxt, part, flag);

		UpdateIconAndMenus();
	}
}

void MCButton::SetDisabled(MCExecContext& ctxt, uint32_t part, bool flag)
{
	MCObject::SetDisabled(ctxt, part, flag);

	if (!ctxt . HasError())
	{
		if (entry != nil)
			entry -> SetDisabled(ctxt, part, flag);

		UpdateIconAndMenus();
	}
}

void MCButton::GetText(MCExecContext& ctxt, MCStringRef& r_text)
{
	r_text = MCValueRetain(menustring);
}

void MCButton::SetText(MCExecContext& ctxt, MCStringRef p_text)
{
	if (MCStringIsEqualTo(menustring, p_text, kMCStringOptionCompareExact))
	{
		if (resetlabel())
            Redraw();
		return;
	}
	
	freemenu(False);
	MCValueAssign(menustring, p_text);
	if (!MCStringIsEmpty(p_text))
    {
        flags |= F_MENU_STRING;
		findmenu(true);
    }
	else
    {
        flags &= ~F_MENU_STRING;
    }
	menuhistory = 1;
    
    bool t_dirty = (resetlabel() || menumode == WM_TOP_LEVEL);
    
	if (parent && parent->gettype() == CT_GROUP)
	{
		parent->setstate(True, CS_NEED_UPDATE);
		if ((parent.GetAs<MCGroup>() == MCmenubar || parent.GetAs<MCGroup>() == MCdefaultmenubar) && !MClockmenus)
			MCscreen->updatemenubar(True);
	}
	if (t_dirty)
        Redraw();
}

void MCButton::GetUnicodeText(MCExecContext& ctxt, MCDataRef& r_text)
{
	MCStringRef t_text = nil;
	GetText(ctxt, t_text);
	if (MCStringEncodeAndRelease(t_text, kMCStringEncodingUTF16, false, r_text))
		return;
	MCValueRelease(t_text);
	
	ctxt.Throw();
}

void MCButton::SetUnicodeText(MCExecContext& ctxt, MCDataRef p_text)
{
	MCAutoStringRef t_new_text;
	if (MCStringDecode(p_text, kMCStringEncodingUTF16, false, &t_new_text))
	{
		SetText(ctxt, *t_new_text);
		return;
	}
	
	ctxt.Throw();
}

void MCButton::SetCantSelect(MCExecContext& ctxt, bool setting)
{
	// MW-2005-08-16: [[Bug 2820]] If we can't be selected, let us make sure our field can't either!
	// MW-2005-09-05: [[Bug 3167]] Only set the entry's property if it exists!
	if (entry != NULL)
		entry -> SetCantSelect(ctxt, setting);
	MCObject::SetCantSelect(ctxt, setting);
}

void MCButton::SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins)
{
    MCControl::SetMargins(ctxt, p_margins);
    
    if (entry != nil)
        entry -> SetMargins(ctxt, p_margins);
}

void MCButton::GetHilite(MCExecContext& ctxt, uint32_t p_part, MCInterfaceTriState& r_hilite)
{
    r_hilite.value = gethilite(p_part);
}

void MCButton::SetHilite(MCExecContext& ctxt, uint32_t p_part, const MCInterfaceTriState& p_hilite)
{
    if (sethilite(p_part, p_hilite.value))
    {
        if (state & CS_HILITED)
        {
            // MH-2007-03-20: [[ Bug 4035 ]] If the hilite of a radio button is set programmatically, other radio buttons were not unhilited if the radiobehavior of the group is set.
            if (getstyleint(flags) == F_RADIO && parent -> gettype() == CT_GROUP)
            {
                MCGroup *gptr = parent.GetAs<MCGroup>();
                gptr->radio(p_part, this);
            }
            radio();
        }
        reseticon();
        Redraw();
    }
}

void MCButton::SetChunkProp(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, Properties which, bool setting)
{
    MCObjectChunkPtr t_obj_chunk;
    t_obj_chunk . object = this;
    t_obj_chunk . part_id = p_part;
    t_obj_chunk . mark . start = p_start;
    t_obj_chunk . mark . finish = p_finish;
    
    if (which == P_DISABLED)
    {
        if (setting)
            MCInterfaceExecDisableChunkOfButton(ctxt, t_obj_chunk);
        else
            MCInterfaceExecEnableChunkOfButton(ctxt, t_obj_chunk);
    }
    else
    {
        if (setting)
            MCInterfaceExecHiliteChunkOfButton(ctxt, t_obj_chunk);
        else
            MCInterfaceExecUnhiliteChunkOfButton(ctxt, t_obj_chunk);
    }
}

void MCButton::SetDisabledOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting)
{
    SetChunkProp(ctxt, p_part, p_start, p_finish, P_DISABLED, setting);
}

void MCButton::SetEnabledOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting)
{
    SetChunkProp(ctxt, p_part, p_start, p_finish, P_DISABLED, !setting);
}

void MCButton::SetHiliteOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting)
{
    SetChunkProp(ctxt, p_part, p_start, p_finish, P_HILITE, setting);
}

void MCButton::SetUnhiliteOfCharChunk(MCExecContext& ctxt, uint32_t p_part, int32_t p_start, int32_t p_finish, bool setting)
{
    SetChunkProp(ctxt, p_part, p_start, p_finish, P_HILITE, !setting);
}
