/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "image.h"
#include "cdata.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "pxmaplst.h"
#include "hndlrlst.h"
#include "handler.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "param.h"
#include "font.h"
#include "util.h"
#include "debug.h"
#include "aclip.h"
#include "vclip.h"
#include "field.h"
#include "chunk.h"
#include "objectstream.h"
#include "parentscript.h"
#include "bitmapeffect.h"
#include "objectpropsets.h"
#include "exec.h"

#include "globals.h"
#include "mctheme.h"
#include "redraw.h"

#include "license.h"
#include "context.h"
#include "mode.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCObject::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_ID, UInt32, MCObject, Id)
	DEFINE_RW_OBJ_PROPERTY(P_SHORT_ID, UInt32, MCObject, Id)
	DEFINE_RO_OBJ_PROPERTY(P_ABBREV_ID, String, MCObject, AbbrevId)
	DEFINE_RO_OBJ_PROPERTY(P_LONG_ID, String, MCObject, LongId)
	DEFINE_RW_OBJ_PROPERTY(P_NAME, String, MCObject, Name)
	DEFINE_RO_OBJ_PROPERTY(P_SHORT_NAME, String, MCObject, ShortName)
	DEFINE_RO_OBJ_PROPERTY(P_LONG_NAME, String, MCObject, LongName)
	DEFINE_RW_OBJ_PROPERTY(P_ALT_ID, UInt32, MCObject, AltId)
	DEFINE_RW_OBJ_PART_CUSTOM_PROPERTY(P_LAYER, InterfaceLayer, MCObject, Layer)
	DEFINE_RW_OBJ_PROPERTY(P_SCRIPT, String, MCObject, Script)
	DEFINE_RW_OBJ_PROPERTY(P_PARENT_SCRIPT, OptionalString, MCObject, ParentScript)
	DEFINE_RO_OBJ_PART_PROPERTY(P_NUMBER, UInt32, MCObject, Number)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FORE_PIXEL, OptionalUInt16, MCObject, ForePixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FORE_PIXEL, UInt16, MCObject, ForePixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BACK_PIXEL, OptionalUInt16, MCObject, BackPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BACK_PIXEL, UInt16, MCObject, BackPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HILITE_PIXEL, OptionalUInt16, MCObject, HilitePixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_HILITE_PIXEL, UInt16, MCObject, HilitePixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BORDER_PIXEL, OptionalUInt16, MCObject, BorderPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BORDER_PIXEL, UInt16, MCObject, BorderPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_PIXEL, OptionalUInt16, MCObject, TopPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_PIXEL, UInt16, MCObject, TopPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_PIXEL, OptionalUInt16, MCObject, BottomPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_PIXEL, UInt16, MCObject, BottomPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHADOW_PIXEL, OptionalUInt16, MCObject, ShadowPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHADOW_PIXEL, UInt16, MCObject, ShadowPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FOCUS_PIXEL, OptionalUInt16, MCObject, FocusPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FOCUS_PIXEL, UInt16, MCObject, FocusPixel)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_BACK_COLOR, Any, MCObject, PenBackColor)
	DEFINE_RW_OBJ_PROPERTY(P_BRUSH_BACK_COLOR, Any, MCObject, PenBackColor)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_PATTERN, OptionalUInt16, MCObject, PenPattern)
	DEFINE_RW_OBJ_PROPERTY(P_BRUSH_PATTERN, OptionalUInt16, MCObject, PenPattern)

	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_PEN_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_PEN_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BRUSH_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BRUSH_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, MCObject, HiliteColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, MCObject, HiliteColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCObject, BorderColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCObject, BorderColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_TOP_COLOR, InterfaceNamedColor, MCObject, TopColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_TOP_COLOR, InterfaceNamedColor, MCObject, TopColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BOTTOM_COLOR, InterfaceNamedColor, MCObject, BottomColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BOTTOM_COLOR, InterfaceNamedColor, MCObject, BottomColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_SHADOW_COLOR, InterfaceNamedColor, MCObject, ShadowColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_SHADOW_COLOR, InterfaceNamedColor, MCObject, ShadowColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_FOCUS_COLOR, InterfaceNamedColor, MCObject, FocusColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_FOCUS_COLOR, InterfaceNamedColor, MCObject, FocusColor)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FORE_PATTERN, OptionalUInt16, MCObject, ForePattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FORE_PATTERN, UInt16, MCObject, ForePattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BACK_PATTERN, OptionalUInt16, MCObject, BackPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BACK_PATTERN, UInt16, MCObject, BackPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HILITE_PATTERN, OptionalUInt16, MCObject, HilitePattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_HILITE_PATTERN, UInt16, MCObject, HilitePattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BORDER_PATTERN, OptionalUInt16, MCObject, BorderPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BORDER_PATTERN, UInt16, MCObject, BorderPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_PATTERN, OptionalUInt16, MCObject, TopPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_PATTERN, UInt16, MCObject, TopPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_PATTERN, OptionalUInt16, MCObject, BottomPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_PATTERN, UInt16, MCObject, BottomPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHADOW_PATTERN, OptionalUInt16, MCObject, ShadowPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHADOW_PATTERN, UInt16, MCObject, ShadowPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FOCUS_PATTERN, OptionalUInt16, MCObject, FocusPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FOCUS_PATTERN, UInt16, MCObject, FocusPattern)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_COLORS, String, MCObject, Colors)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_COLORS, String, MCObject, Colors)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_PATTERNS, String, MCObject, Patterns)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_PATTERNS, String, MCObject, Patterns)

	DEFINE_RW_OBJ_PROPERTY(P_LOCK_LOCATION, Bool, MCObject, LockLocation)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_HEIGHT, OptionalUInt16, MCObject, TextHeight)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_HEIGHT, UInt16, MCObject, TextHeight)
	DEFINE_RW_OBJ_NON_EFFECTIVE_OPTIONAL_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCObject, TextAlign)
	DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCObject, TextAlign)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_FONT, OptionalString, MCObject, TextFont)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_FONT, String, MCObject, TextFont)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_SIZE, OptionalUInt16, MCObject, TextSize)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_SIZE, UInt16, MCObject, TextSize)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCObject, TextStyle)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCObject, TextStyle)

	DEFINE_RW_OBJ_PROPERTY(P_SHOW_BORDER, Bool, MCObject, ShowBorder)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_FOCUS_BORDER, Bool, MCObject, ShowFocusBorder)
	DEFINE_RW_OBJ_PROPERTY(P_BORDER_WIDTH, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_LINE_SIZE, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_WIDTH, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_HEIGHT, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_OPAQUE, Bool, MCObject, Opaque)	
	DEFINE_RW_OBJ_PROPERTY(P_FILLED, Bool, MCObject, Opaque)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_SHADOW, InterfaceShadow, MCObject, Shadow)
	DEFINE_RW_OBJ_PROPERTY(P_SHADOW_OFFSET, Int16, MCObject, ShadowOffset)
	DEFINE_RW_OBJ_PROPERTY(P_3D, Bool, MCObject, 3D)

	DEFINE_RW_OBJ_PART_PROPERTY(P_VISIBLE, Bool, MCObject, Visible)
	DEFINE_RW_OBJ_PART_PROPERTY(P_INVISIBLE, Bool, MCObject, Invisible)
	DEFINE_RW_OBJ_PART_PROPERTY(P_ENABLED, Bool, MCObject, Enabled)
	DEFINE_RW_OBJ_PART_PROPERTY(P_DISABLED, Bool, MCObject, Disabled)
	DEFINE_RW_OBJ_PROPERTY(P_SELECTED, Bool, MCObject, Selected)
	DEFINE_RW_OBJ_PROPERTY(P_TRAVERSAL_ON, Bool, MCObject, TraversalOn)

	DEFINE_RO_OBJ_PROPERTY(P_OWNER, OptionalString, MCObject, Owner)
	DEFINE_RO_OBJ_PROPERTY(P_SHORT_OWNER, OptionalString, MCObject, ShortOwner)
	DEFINE_RO_OBJ_PROPERTY(P_ABBREV_OWNER, OptionalString, MCObject, AbbrevOwner)
	DEFINE_RO_OBJ_PROPERTY(P_LONG_OWNER, OptionalString, MCObject, LongOwner)

	DEFINE_RW_OBJ_PART_PROPERTY(P_PROPERTIES, Array, MCObject, Properties)
	DEFINE_RW_OBJ_PROPERTY(P_CUSTOM_PROPERTY_SET, String, MCObject, CustomPropertySet)
	DEFINE_RW_OBJ_LIST_PROPERTY(P_CUSTOM_PROPERTY_SETS, LinesOfString, MCObject, CustomPropertySets)
    
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_INK, InterfaceInkNames, MCObject, Ink)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_CANT_SELECT, Bool, MCObject, CantSelect)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_CANT_SELECT, Bool, MCObject, CantSelect)
	DEFINE_RW_OBJ_PROPERTY(P_BLEND_LEVEL, UInt16, MCObject, BlendLevel)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_LOCATION, Point, MCObject, Location)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_LOCATION, Point, MCObject, Location)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)

	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)

	DEFINE_RO_OBJ_ENUM_PROPERTY(P_ENCODING, InterfaceEncoding, MCObject, Encoding)
    
    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_TEXT_STYLE, Bool, MCObject, TextStyleElement)
    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_CUSTOM_KEYS, String, MCObject, CustomKeysElement)
    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_CUSTOM_PROPERTIES, Any, MCObject, CustomProperties)
    
    DEFINE_RW_OBJ_PROPERTY(P_CUSTOM_KEYS, String, MCObject, CustomKeys) 
    
};

MCObjectPropertyTable MCObject::kPropertyTable =
{
	nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

static const char *ink_names[] =
{
	"clear",
	"srcAnd",
	"srcAndReverse",
	"srcCopy",
	"notSrcAnd",
	"noop",
	"srcXor",
	"srcOr",
	"notSrcAndReverse",
	"notSrcXor",
	"reverse",
	"srcOrReverse",
	"notSrcCopy",
	"notSrcOr",
	"notSrcOrReverse",
	"set",
	"srcBic",
	"notSrcBic",

	"blend",
	"addpin",
	"addOver",
	"subPin",
	"transparent",
	"addMax",
	"subOver",
	"adMin",
	
	"blendClear",
	"blendSrc",
	"blendDst",
	"blendSrcOver",
	"blendDstOver",
	"blendSrcIn",
	"blendDstIn",
	"blendSrcOut",
	"blendDstOut",
	"blendSrcAtop",
	"blendDstAtop",
	"blendXor",
	"blendPlus",
	"blendMultiply",
	"blendScreen",

	"blendOverlay",
	"blendDarken",
	"blendLighten",
	"blendDodge",
	"blendBurn",
	"blendHardLight",
	"blendSoftLight",
	"blendDifference",
	"blendExclusion"
};

////////////////////////////////////////////////////////////////////////////////
#ifdef /* MCObject::getrectprop */ LEGACY_EXEC
Exec_stat MCObject::getrectprop(Properties p_which, MCExecPoint& ep, Boolean p_effective)
{
	MCRectangle t_rect;
	t_rect = getrectangle(p_effective == True);

	switch(p_which)
	{
	case P_LOCATION:
		ep.setpoint(t_rect.x + (t_rect.width >> 1), t_rect.y + (t_rect.height >> 1));
		break;
	case P_LEFT:
		ep.setint(t_rect.x);
		break;
	case P_TOP:
		ep.setint(t_rect.y);
		break;
	case P_RIGHT:
		ep.setint(t_rect.x + t_rect.width);
		break;
	case P_BOTTOM:
		ep.setint(t_rect.y + t_rect.height);
		break;
	case P_TOP_LEFT:
		ep.setpoint(t_rect.x, t_rect.y);
		break;
	case P_TOP_RIGHT:
		ep.setpoint(t_rect.x + t_rect.width, t_rect.y);
		break;
	case P_BOTTOM_LEFT:
		ep.setpoint(t_rect.x, t_rect.y + t_rect.height);
		break;
	case P_BOTTOM_RIGHT:
		ep.setpoint(t_rect.x + t_rect.width, t_rect.y + t_rect.height);
		break;
	case P_WIDTH:
		ep.setint(t_rect.width);
		break;
	case P_HEIGHT:
		ep.setint(t_rect.height);
		break;
	case P_RECTANGLE:
		ep.setrectangle(t_rect);
		break;
	default:
		assert(false);
		break;
	}

	return ES_NORMAL;
}
#endif /* MCObject::getrectprop */

Exec_stat MCObject::sendgetprop(MCExecPoint& ep, MCNameRef p_set_name, MCNameRef p_prop_name)
{
	// If the set name is nil, then we send a 'getProp <propname>' otherwise we
	// send a 'getProp <setname> <propname>'.
	//
	MCNameRef t_getprop_name;
	MCNameRef t_param_name;
	if (MCNameIsEqualTo(p_set_name, kMCEmptyName, kMCCompareCaseless))
		t_getprop_name = p_prop_name, t_param_name = kMCEmptyName;
	else
		t_getprop_name = p_set_name, t_param_name = p_prop_name;

	Exec_stat t_stat = ES_NOT_HANDLED;
	if (!MClockmessages && (ep.getobj() != this || !ep.gethandler()->hasname(t_getprop_name)))
	{
		MCParameter p1;
		p1.setvalueref_argument(t_param_name);

		MCStack *oldstackptr = MCdefaultstackptr;
		MCdefaultstackptr = getstack();
		MCObject *oldtargetptr = MCtargetptr;
		MCtargetptr = this;
		Boolean added = False;
		MCExecContext ctxt(ep);
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
			added = True;
		}
		t_stat = MCU_dofrontscripts(HT_GETPROP, t_getprop_name, &p1);
		if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
			t_stat = handle(HT_GETPROP, t_getprop_name, &p1, this);
		MCdefaultstackptr = oldstackptr;
		MCtargetptr = oldtargetptr;
		if (added)
			MCnexecutioncontexts--;
	}

	if (t_stat == ES_NORMAL)
		MCresult -> eval(ep);

	return t_stat;
}

Exec_stat MCObject::getcustomprop(MCExecPoint& ep, MCNameRef p_set_name, MCNameRef p_prop_name)
{
	assert(p_set_name != nil);
	assert(p_prop_name != nil);

	Exec_stat t_stat;

	t_stat = sendgetprop(ep, p_set_name, p_prop_name);
	if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
	{
		MCObjectPropertySet *p;
		if (!findpropset(p_set_name, false, p) ||
			!p -> fetchelement(ep, p_prop_name))
			ep.clear();

		t_stat = ES_NORMAL;
	}

	return t_stat;
}

Exec_stat MCObject::getprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
#ifdef /* MCObject::getprop */ LEGACY_EXEC
	uint2 num = 0;

	switch (which)
	{
	case P_ID:
	case P_SHORT_ID:
	case P_LONG_ID:
	case P_ABBREV_ID:
	case P_NAME:
	case P_SHORT_NAME:
	case P_ABBREV_NAME:
	case P_LONG_NAME:
		return names(which, ep, parid);
	case P_ALT_ID:
		ep.setint(altid);
		break;
	case P_LAYER:
		// OK-2009-03-12: [[Bug 8049]] - Fix relayering of grouped objects broken by 
		// previous fix for crash when attempting to get the layer of an object outside
		// the group being edited in edit group mode.

		if (parent != NULL)
		{
			MCCard *t_card;
			t_card = getcard(parid);

			if(parid != 0 && t_card == NULL)
				t_card = getstack()->findcardbyid(parid);

			if (t_card == NULL)
			{
				// This shouldn't happen, but rather than a crash, throw a random execution error..
				MCeerror -> add(EE_CHUNK_NOCARD, 0, 0);
				return ES_ERROR;
			}

			t_card -> count(CT_LAYER, CT_UNDEFINED, this, num, True);
		}

		ep.setint(num);
		break;
	case P_SCRIPT:
		if (!MCdispatcher->cut(True))
		{
			MCeerror->add
			(EE_OBJECT_NOHOME, 0, 0);
			return ES_ERROR;
		}
		if (!getstack()->iskeyed())
		{
			MCeerror->add
			(EE_STACK_NOKEY, 0, 0);
			return ES_ERROR;
		}
		if (script != nil)
		{
			getstack() -> unsecurescript(this);
			ep.copysvalue(script);
			getstack() -> securescript(this);
		}
		else
			ep . clear();
		break;

	// MW-2008-10-25: Handle the parentScript property when getting
	case P_PARENT_SCRIPT:
	{
		ep . clear();

		// If there is a parent script we return a reference string
		if (parent_script != NULL)
		{
			MCParentScript *t_parent;
			t_parent = parent_script -> GetParent();
 
			ep . setstringf("button id %d of stack \"%s\"",
								t_parent -> GetObjectId(),
								MCNameGetCString(t_parent -> GetObjectStack()));
		}
	}
	break;

	case P_NUMBER:
		if (getstack()->hcaddress())
			if (parent->gettype() == CT_CARD)
				getcard(parid)->count(gettype(), CT_CARD, this, num, True);
			else
				getcard(parid)->count(gettype(), CT_BACKGROUND, this, num, True);
		else
			getcard(parid)->count(gettype(), CT_UNDEFINED, this, num, True);
		ep.setint(num);
		break;
	case P_FORE_PIXEL:
	case P_BACK_PIXEL:
	case P_HILITE_PIXEL:
	case P_BORDER_PIXEL:
	case P_TOP_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
	{
		// MW-2011-02-27: [[ Bug 9419 ]] If the object isn't already open, then alloc the color
		//   first.
		uint2 i;
		if (getcindex(which - P_FORE_PIXEL, i))
		{
			if (!opened)
				MCscreen->alloccolor(colors[i]);
			ep.setint(colors[i].pixel & 0xFFFFFF);
		}
		else if (effective && parent != NULL)
			return parent->getprop(parid, which, ep, effective);
		else
			ep.clear();
	}
	break;
	case P_PEN_BACK_COLOR:
	case P_BRUSH_BACK_COLOR:
	case P_PEN_PATTERN:
	case P_BRUSH_PATTERN:
		ep.clear();
		break;
	case P_PEN_COLOR:
	case P_BRUSH_COLOR:
	case P_FORE_COLOR:
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
	{
		uint2 i;
		if (which == P_PEN_COLOR)
			which = P_FORE_COLOR;
		else if (which == P_BRUSH_COLOR)
			which = P_BACK_COLOR;
		if (getcindex(which - P_FORE_COLOR, i))
			ep.setcolor(colors[i], colornames[i]);
		else if (effective && parent != NULL)
			return parent->getprop(parid, which, ep, effective);
		else
			ep.clear();
	}
	break;
	case P_COLORS:
		{
			MCExecPoint ep2(ep);
			ep.clear();
			uint2 p;
			for (p = P_FORE_COLOR ; p <= P_FOCUS_COLOR ; p++)
			{
				getprop(parid, (Properties)p, ep2, effective);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, p == P_FORE_COLOR);
			}
		}
		break;
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
	{
		uint2 i;
		if (getpindex(which - P_FORE_PATTERN, i))
			if (pixmapids[i] < PI_END && pixmapids[i] > PI_PATTERNS)
				ep.setint(pixmapids[i] - PI_PATTERNS);

			else
				ep.setint(pixmapids[i]);
		else
			if (effective && parent != NULL)
				return parent->getprop(parid, which, ep, effective);
			else
				ep.clear();
	}
	break;
	case P_PATTERNS:
		{
			MCExecPoint ep2(ep);
			ep.clear();
			uint2 p;
			for (p = P_FORE_PATTERN ; p <= P_FOCUS_PATTERN ; p++)
			{
				getprop(parid, (Properties)p, ep2, effective);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, p == P_FORE_PATTERN);
			}
		}
		break;
	case P_LOCK_LOCATION:
		ep.setboolean(getflag(F_LOCK_LOCATION));
		break;
	case P_TEXT_HEIGHT:
		// MW-2012-02-19: [[ SplitTextAttrs ]] If the textHeight has been set, or we
		//   want the effective property, then call 'gettextheight()'. Otherwise the
		//   value is empty.
		if (fontheight != 0 || effective)
			ep . setuint(gettextheight());
		else
			ep . clear();
		break;
	case P_TEXT_ALIGN:
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
		// MW-2012-02-19: [[ SplitTextAttrs ]] If we don't have the requested prop
		//   set, then handle by delegating upwards.
		if (which != P_TEXT_ALIGN &&
			 (which == P_TEXT_FONT && (m_font_flags & FF_HAS_TEXTFONT) == 0) ||
			 (which == P_TEXT_SIZE && (m_font_flags & FF_HAS_TEXTSIZE) == 0) ||
			 (which == P_TEXT_STYLE && (m_font_flags & FF_HAS_TEXTSTYLE) == 0))
		{
			if (effective && parent != NULL)
				return parent->getprop(parid, which, ep, effective);
			else
				ep.clear();
		}
		else
		{
			uint2 fheight, fontsize, fontstyle;
			const char *fontname;
			getfontattsnew(fontname, fontsize, fontstyle);
			fheight = gettextheight();
			return MCF_unparsetextatts(which, ep, flags, fontname, fheight, fontsize, fontstyle);
		}
		break;
	case P_SHOW_BORDER:
		ep.setboolean(getflag(F_SHOW_BORDER));
		break;
	case P_SHOW_FOCUS_BORDER:
		ep.setboolean(!(extraflags & EF_NO_FOCUS_BORDER));
		break;
	case P_BORDER_WIDTH:
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
		ep.setint(borderwidth);
		break;
	case P_OPAQUE:
	case P_FILLED:
		ep.setboolean(getflag(F_OPAQUE));
		break;
	case P_SHADOW:
		ep.setboolean(getflag(F_SHADOW));
		break;
	case P_SHADOW_OFFSET:
		ep.setint(shadowoffset);
		break;
	case P_3D:
		ep.setboolean(getflag(F_3D));
		break;
	case P_VISIBLE:
		ep.setboolean(getflag(F_VISIBLE));
		break;
	case P_INVISIBLE:
		ep.setboolean(!(flags & F_VISIBLE));
		break;
	case P_DISABLED:
		ep.setboolean(getflag(F_DISABLED));
		break;
	case P_ENABLED:
		ep.setboolean(!(flags & F_DISABLED));
		break;
	case P_SELECTED:
		ep.setboolean(getstate(CS_SELECTED));
		break;
	case P_TRAVERSAL_ON:
		ep.setboolean(getflag(F_TRAVERSAL_ON));
		break;
	case P_OWNER:
	case P_SHORT_OWNER:
	case P_ABBREV_OWNER:
	case P_LONG_OWNER:
		if (parent != NULL)
			return parent->getprop(0, (Properties)(P_NAME + which - P_OWNER), ep, False);
		ep.clear();
		break;
	case P_PROPERTIES:
		return getproparray(ep, parid);
	case P_CUSTOM_PROPERTY_SET:
		ep . setnameref_unsafe(getdefaultpropsetname());
		break;
	case P_CUSTOM_PROPERTY_SETS:
		listpropsets(ep);
		break;
	case P_INK:
		ep.setstaticcstring(ink_names[ink]);
		break;
	case P_CANT_SELECT:
		ep.setboolean(!isselectable(effective == False));
		break;
	case P_BLEND_LEVEL:
		ep.setint(100 - blendlevel);
		break;
	case P_LOCATION:
	case P_LEFT:
	case P_TOP:
	case P_RIGHT:
	case P_BOTTOM:
	case P_TOP_LEFT:
	case P_TOP_RIGHT:
	case P_BOTTOM_LEFT:
	case P_BOTTOM_RIGHT:
	case P_WIDTH:
	case P_HEIGHT:
	case P_RECTANGLE:
		return getrectprop(which, ep, effective);
	// MW-2012-02-22: [[ IntrinsicUnicode ]] Handle the encoding property.
	case P_ENCODING:
		if (hasunicode())
			ep . setstaticcstring(MCunicodestring);
		else
			ep . setstaticcstring(MCnativestring);
		break;
	default:
		{
			Exec_stat t_stat;
			t_stat = mode_getprop(parid, which, ep, kMCEmptyString, effective);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
				return ES_ERROR;
			}
			return t_stat;
		}
	}

	return ES_NORMAL;
#endif /* MCObject::getprop */
	Exec_stat t_stat;
	t_stat = mode_getprop(parid, which, ep, kMCEmptyString, effective);
	if (t_stat == ES_NOT_HANDLED)
	{
		MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
		return ES_ERROR;
	}
	return t_stat;
}

static bool string_contains_item(const char *p_string, const char *p_item)
{
	const char *t_offset;
	t_offset = strstr(p_string, p_item);
	if (t_offset == nil)
		return false;
	if (t_offset != p_string && t_offset[-1] != ',')
		return false;
	
	uint32_t t_length;
	t_length = strlen(p_item);
	if (t_offset[t_length] != '\0' && t_offset[t_length] != ',')
		return false;

	return true;
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCObject::getarrayprop_legacy(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
	switch(which)
	{
	// MW-2011-11-23: [[ Array TextStyle ]] We now treat textStyle as (potentially) an
	//   an array.
	case P_TEXT_STYLE:
	{
		// First fetch the current property value.
		if (getprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;

		// If the key is empty, then we are done.
		if (MCNameIsEmpty(key))
			return ES_NORMAL;

		// Parse the requested text style.
		Font_textstyle t_style;
		if (MCF_parsetextstyle(MCNameGetOldString(key), t_style) != ES_NORMAL)
			return ES_ERROR;

		// Check the textstyle string is within the object's textstyle set.
		ep . setboolean(string_contains_item(ep . getcstring(), MCF_unparsetextstyle(t_style)));
	}
	break;
	case P_CUSTOM_KEYS:
	case P_CUSTOM_PROPERTIES:
		{
			MCObjectPropertySet *p;
			if (findpropset(key, true, p))
			{
				if (which == P_CUSTOM_KEYS)
					p -> list(ep);
				else
					p -> fetch(ep);
			}
			else
				ep.clear();
		}
		break;
	default:
		{
			Exec_stat t_stat;
			t_stat = mode_getprop(parid, which, ep, MCNameGetString(key), False);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
				return ES_ERROR;
			}
			return t_stat;
		}
	}
	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
#ifdef /* MCObject::setrectprop */ LEGACY_EXEC
Exec_stat MCObject::setrectprop(Properties p_which, MCExecPoint& ep, Boolean p_effective)
{
	MCString t_data;
	t_data = ep . getsvalue();

	// MW-2012-10-26: Use 'getrectangle()' so we also make 'effective' work for a stack.
	MCRectangle t_outer_rect, t_rect;
	t_outer_rect = t_rect = getrectangle(p_effective == True);

	int2 i1, i2, i3, i4;
	switch(p_which)
	{
	case P_BOTTOM_LEFT:
	case P_BOTTOM_RIGHT:
	case P_LOCATION:
	case P_TOP_LEFT:
	case P_TOP_RIGHT:
		if (!MCU_stoi2x2(t_data, i1, i2))
		{
			MCeerror->add(EE_OBJECT_NAP, 0, 0, t_data);
			return ES_ERROR;
		}

		switch (p_which)
		{
		case P_BOTTOM_LEFT:
			i2 -= t_rect.height;
			break;
		case P_BOTTOM_RIGHT:
			i1 -= t_rect.width;
			i2 -= t_rect.height;
			break;
		case P_LOCATION:
			i1 -= t_rect.width >> 1;
			i2 -= t_rect.height >> 1;
			break;
		case P_TOP_LEFT:
			break;
		case P_TOP_RIGHT:
			i1 -= t_rect.width;
			break;
		default:
			break;
		}

		t_rect . x = i1;
		t_rect . y = i2;
		break;
	case P_WIDTH:
	case P_HEIGHT:
		if (!MCU_stoi2(t_data, i1) || i1 < 0)
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, t_data);
			return ES_ERROR;
		}
		if (p_which == P_WIDTH)
		{
			if (!getflag(F_LOCK_LOCATION))
				t_rect.x += (t_rect.width - i1) >> 1;
			t_rect.width = MCU_max(i1, 1);
		}
		else
		{
			if (!getflag(F_LOCK_LOCATION))
				t_rect.y += (t_rect.height - i1) >> 1;
			t_rect.height = MCU_max(i1, 1);
		}
		break;
	case P_LEFT:
	case P_RIGHT:
	case P_TOP:
	case P_BOTTOM:
		if (!MCU_stoi2(t_data, i1))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, t_data);
			return ES_ERROR;
		}
		switch (p_which)
		{
		case P_LEFT:
			t_rect.x = i1;
			break;
		case P_RIGHT:
			t_rect.x = i1 - t_rect.width;
			break;
		case P_TOP:
			t_rect.y = i1;
			break;
		case P_BOTTOM:
			t_rect.y = i1 - t_rect.height;
			break;
		default:
			break;
		}
		break;
	
	case P_RECTANGLE:
		if (!MCU_stoi2x4(t_data, i1, i2, i3, i4))
		{
			MCeerror->add(EE_OBJECT_NAR, 0, 0, t_data);
			return ES_ERROR;
		}
		t_rect.x = i1;
		t_rect.y = i2;
		t_rect.width = MCU_max(i3 - i1, 1);
		t_rect.height = MCU_max(i4 - i2, 1);
		break;

	default:
		break;
	}

	// MW-2012-10-26: Adjust the rectangle appropriately based on any effective margins.
	if (p_effective)
	{
		MCRectangle t_inner_rect;
		t_inner_rect = getrectangle(false);

		t_rect . x += t_inner_rect . x - t_outer_rect . x;
		t_rect . y += t_inner_rect . y - t_outer_rect . y;
		t_rect . width -= (t_inner_rect . x - t_outer_rect . x) + (t_outer_rect . x + t_outer_rect . width - (t_inner_rect . x + t_inner_rect . width));
		t_rect . height -= (t_inner_rect . y - t_outer_rect . y) + (t_outer_rect . y + t_outer_rect . height - (t_inner_rect . y + t_inner_rect . height));
	}

	if (!MCU_equal_rect(t_rect, rect))
	{
		Boolean needmfocus;
		needmfocus = false;

		if (opened && getstack() == MCmousestackptr)
		{
			MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
			if (MCU_point_in_rect(rect, MCmousex, MCmousey))
			{
				if (!MCU_point_in_rect(t_rect, MCmousex, MCmousey) && this == mfocused)
					needmfocus = True;
			}
			else
				if (MCU_point_in_rect(t_rect, MCmousex, MCmousey) && this != mfocused)
					needmfocus = True;
		}

		if (gettype() >= CT_GROUP)
		{
			// MW-2011-08-18: [[ Layers ]] Notify of change of rect.
			static_cast<MCControl *>(this) -> layer_setrect(t_rect, false);
			// Notify the parent of the resize.
			resizeparent();
		}
		else
			setrect(t_rect);

		if (needmfocus)
			MCmousestackptr->getcard()->mfocus(MCmousex, MCmousey);
	}
	
	return ES_NORMAL;
}
#endif /* MCObject::setrectprop */

#ifdef /* MCObject::setscriptprop */ LEGACY_EXEC
Exec_stat MCObject::setscriptprop(MCExecPoint& ep)
{
	if (!MCdispatcher->cut(True))
	{
		MCeerror->add(EE_OBJECT_NOHOME, 0, 0);
		return ES_ERROR;
	}
	if (!getstack()->iskeyed())
	{
		MCeerror->add(EE_STACK_NOKEY, 0, 0);
		return ES_ERROR;
	}
	if (scriptdepth != 0)
	{
		MCeerror->add(EE_OBJECT_SCRIPTEXECUTING, 0, 0);
		return ES_ERROR;
	}
	
	MCString data;
	data = ep . getsvalue();

	uint4 length;
	length = data.getlength();
	if (length == 0)
	{
		delete hlist;
		hlist = NULL;
		delete script;
		script = NULL;
		flags &= ~F_SCRIPT;
		hashandlers = 0;
	}
	else
	{
		char *oldscript = script;
		if (data.getstring()[length - 1] != '\n')
		{
			script = new char[length + 2];
			memcpy(script, data.getstring(), length);
			script[length++] = '\n';
			script[length] = '\0';
		}
		else
			script = data.clone();
			
		getstack() -> securescript(this);
		
		if (MCModeCanSetObjectScript(obj_id))
		{ // not template object
			hashandlers = 0;
			parsescript(False, True);
			if (hlist != NULL && MClicenseparameters . script_limit > 0 && hlist -> linecount() >= MClicenseparameters . script_limit)
			{
				delete hlist;
				hlist = NULL;
				delete script;
				script = oldscript;
				oldscript = NULL;
				if (script == NULL)
					flags &= ~F_SCRIPT;
				MCperror->add(PE_OBJECT_NOTLICENSED, 0, 0);
			}
			if (!MCperror->isempty())
			{
				MCresult->copysvalue(MCperror->getsvalue());
				MCperror->clear();
			}
			else
				MCresult->clear();
		}
		delete oldscript;
	}

	return ES_NORMAL;
}
#endif /* MCObject::setscriptprop */

#ifdef /* MCObject::setparentscriptprop */ LEGACY_EXEC
Exec_stat MCObject::setparentscriptprop(MCExecPoint& ep)
{
	// MW-2008-10-25: Add the setting logic for parent scripts. This code is a
	//   modified version of what goes on in MCChunk::getobj when the final
	//   target for a chunk is an expression. We first parse the string as a
	//   chunk expression, then attempt to get the object of it. If the object
	//   doesn't exist, the set fails.
	Exec_stat t_stat;
	t_stat = ES_ERROR;

	// MW-2008-11-02: [[ Bug ]] Setting the parentScript of an object to
	//   empty should unset the parent script property and not throw an
	//   error.
	MCString data;
	data = ep . getsvalue();
	if (data . getlength() == 0)
	{
		if (parent_script != NULL)
			parent_script -> Release();
		parent_script = NULL;
		return ES_NORMAL;
	}

	// Create a script point with the value are setting the property to
	// as source text.
	MCScriptPoint sp(data);

	// Create a new chunk object to parse the reference into
	MCChunk *t_chunk;
	t_chunk = new MCChunk(False);

	// Attempt to parse a chunk. We also check that there is no 'junk' at
	// the end of the string - if there is, its an error. Note the errorlock
	// here - it stops parse errors being pushed onto MCperror.
	Symbol_type t_next_type;
	MCerrorlock++;
	if (t_chunk -> parse(sp, False) == PS_NORMAL && sp.next(t_next_type) == PS_EOF)
		t_stat = ES_NORMAL;
	MCerrorlock--;

	// Now attempt to evaluate the object reference - this will only succeed
	// if the object exists.
	MCExecPoint ep2(ep);
	MCObject *t_object;
	uint32_t t_part_id;
	if (t_stat == ES_NORMAL)
		t_stat = t_chunk -> getobj(ep2, t_object, t_part_id, False);

	// Check that the object is a button
	if (t_stat == ES_NORMAL && t_object -> gettype() != CT_BUTTON)
		t_stat = ES_ERROR;

	// MW-2009-01-28: [[ Bug ]] Make sure we aren't setting the parentScript of
	//   an object to itself.
	if (t_stat == ES_NORMAL && t_object == this)
		t_stat = ES_ERROR;

	if (t_stat == ES_NORMAL)
	{
		// Check to see if we are already parent-linked to t_object and if so
		// do nothing.
		//
		if (parent_script == NULL || parent_script -> GetParent() -> GetObject() != t_object)
		{
			// We have the target object, so extract its rugged id. That is the
			// (id, stack, mainstack) triple. Note that mainstack is NULL if the
			// object lies on a mainstack.
			//
			uint32_t t_id;
			t_id = t_object -> getid();

			MCNameRef t_stack;
			t_stack = t_object -> getstack() -> getname();

			// Now attempt to acquire a parent script use object. This can only
			// fail if memory is exhausted, so in this case just return an error
			// stat.
			MCParentScriptUse *t_use;
			t_use = MCParentScript::Acquire(this, t_id, t_stack);
			if (t_use == NULL)
				t_stat = ES_ERROR;

			// MW-2009-01-28: [[ Inherited parentScripts ]]
			// Next we have to ensure the inheritence hierarchy is in place (The
			// inherit call will create super-uses, and will return false if there
				// is not enough memory).
#ifdef FEATURE_INHERITED_PARENTSCRIPTS
			if (!t_use -> Inherit())
				t_stat = ES_ERROR;

			// TODO: Update all the Uses of this object, if it is currently
			// being used as a parentScript, because we have now changed the
			// inheritence hierarchy dynamically, and the various uses need
			// their super_use chains updated.
			// 
#endif

			// We have succeeded in creating a new use of an object as a parent
			// script, so now release the old parent script this object points
			// to (if any) and install the new one.
			if (parent_script != NULL)
				parent_script -> Release();

			parent_script = t_use;

			// Finally resolve the parent script as pointing to the object.
			parent_script -> GetParent() -> Resolve(t_object);
		}
	}
	else
		MCeerror -> add(EE_PARENTSCRIPT_BADOBJECT, 0, 0, data);

	// Delete our temporary chunk object.
	delete t_chunk;

	return t_stat;
}
#endif /* MCObject::setparentscriptprop */

#ifdef /* MCObject::setshowfocusborderprop */ LEGACY_EXEC
Exec_stat MCObject::setshowfocusborderprop(MCExecPoint& ep)
{
	MCString data;
	data = ep . getsvalue();

	Boolean newstate;
	if (!MCU_stob(ep . getsvalue(), newstate))
	{
		MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
		return ES_ERROR;
	}

	// Fetch the current transient rect of the control
	uint2 t_old_trans;
	t_old_trans = gettransient();

	if (newstate)
		extraflags &= ~EF_NO_FOCUS_BORDER;
	else
		extraflags |= EF_NO_FOCUS_BORDER;

	// Redraw the control if the parent doesn't and we are open
	if (opened)
	{
		// MW-2011-08-18: [[ Layers ]] Take note of transient change and invalidate.
		if (gettype() >= CT_GROUP)
			static_cast<MCControl *>(this) -> layer_transientchangedandredrawall(t_old_trans);
	}

	return ES_NORMAL;
}
#endif /* MCObject::setshowfocusborderprop */

#ifdef /* MCObject::setvisibleprop */ LEGACY_EXEC
Exec_stat MCObject::setvisibleprop(uint4 parid, Properties which, MCExecPoint& ep)
{
	Boolean dirty;
	dirty = True;

	MCString data;
	data = ep . getsvalue();
	if (!MCU_matchflags(data, flags, F_VISIBLE, dirty))
	{
		MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
		return ES_ERROR;
	}

	if (which == P_INVISIBLE)
	{
		flags ^= F_VISIBLE;
		dirty = !dirty;
	}

	// MW-2011-10-17: [[ Bug 9813 ]] Record the current effective rect of the object.
	MCRectangle t_old_effective_rect;
	if (dirty && opened && gettype() >= CT_GROUP)
		t_old_effective_rect = static_cast<MCControl *>(this) -> geteffectiverect();
	
	Boolean needmfocus;
	needmfocus = False;
	if (dirty)
	{
		if (opened && getstack() == MCmousestackptr)
			if (!(flags & F_VISIBLE))
			{
				MCObject *mfocused = MCmousestackptr->getcard()->getmfocused();
				// MW-2012-02-22: [[ Bug 10018 ]] If the target is a group then check
				//   to see if it is the ancestor of the mfocused control; otherwise
				//   just compare directly.
				if (mfocused != nil && gettype() == CT_GROUP)
				{
					while(mfocused -> gettype() != CT_CARD)
					{
						if (mfocused == this)
						{
							needmfocus = True;
							break;
						}
						mfocused = mfocused -> getparent();
					}
				}
				else if (this == mfocused)
					needmfocus = True;
			}
			else if (MCU_point_in_rect(rect, MCmousex, MCmousey))
				needmfocus = True;

		if (state & CS_KFOCUSED)
			getcard(parid)->kunfocus();

		// MW-2008-08-04: [[ Bug 7094 ]] If we change the visibility of the control
		//   while its grabbed, we should ungrab it - otherwise it sticks to the
		//   cursor.
		if (gettype() >= CT_GROUP && getstate(CS_GRAB))
			state &= ~CS_GRAB;

		if (resizeparent())
			dirty = False;
	}

	if (dirty)
		signallisteners(P_VISIBLE);
	
	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Take note of the change in visibility.
		if (gettype() >= CT_GROUP)
			static_cast<MCControl *>(this) -> layer_visibilitychanged(t_old_effective_rect);
	}

	if (needmfocus)
		MCmousestackptr->getcard()->mfocus(MCmousex, MCmousey);

	return ES_NORMAL;
}
#endif /* MCObject::setvisibleprop */

Exec_stat MCObject::sendsetprop(MCExecPoint& ep, MCNameRef p_set_name, MCNameRef p_prop_name)
{
	// If the set name is nil, then we send a 'setProp <propname> <value>'
	// otherwise we send a 'setProp <setname>[<propname>] <value>'.
	MCNameRef t_setprop_name;
	MCNameRef t_param_name;
	if (MCNameIsEqualTo(p_set_name, kMCEmptyName, kMCCompareCaseless))
		t_setprop_name = p_prop_name, t_param_name = kMCEmptyName;
	else
		t_setprop_name = p_set_name, t_param_name = p_prop_name;

	// Note that in either case (non-array or array setProp), the param list is
	// the same:
	//   setProp pPropName, pValue
	// The parameter list is auto-adjusted if it is of array type in MCHandler::exec.

	Exec_stat t_stat = ES_NOT_HANDLED;
	if (!MClockmessages && (ep.getobj() != this || !ep.gethandler()->hasname(t_setprop_name)))
	{
		MCParameter p1, p2;
		p1.setnext(&p2);

		p1.setvalueref_argument(t_param_name);
		p2.set_argument(ep);
		
		MCStack *oldstackptr = MCdefaultstackptr;
		MCdefaultstackptr = getstack();
		MCObject *oldtargetptr = MCtargetptr;
		MCtargetptr = this;
		Boolean added = False;
		MCExecContext ctxt(ep);
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
			added = True;
		}

		t_stat = MCU_dofrontscripts(HT_SETPROP, t_setprop_name, &p1);
		if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
			t_stat = handle(HT_SETPROP, t_setprop_name, &p1, this);

		if (added)
			MCnexecutioncontexts--;
		MCdefaultstackptr = oldstackptr;
		MCtargetptr = oldtargetptr;
	}

	return t_stat;
}

Exec_stat MCObject::setcustomprop(MCExecPoint& ep, MCNameRef p_set_name, MCNameRef p_prop_name)
{
	Exec_stat t_stat;
	t_stat = sendsetprop(ep, p_set_name, p_prop_name);

	if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
	{
		MCObjectPropertySet *p;
		/* UNCHECKED */ ensurepropset(p_set_name, false, p);
		if (!p -> storeelement(ep, p_prop_name))
			return ES_ERROR;
		return ES_NORMAL;
	}

	return t_stat;
}

Exec_stat MCObject::setprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
#ifdef /* MCObject::setprop */ LEGACY_EXEC
	Boolean dirty = True;
	Boolean newstate;
	int2 i1;
	uint2 i;
	char *newfontname = NULL;
	uint2 fontsize, fontstyle;
	MCString data = ep.getsvalue();

	switch (which)
	{
	case P_ID:
		uint4 newid;
	
		if (!MCU_stoui4(data, newid))
		{
			MCeerror->add(EE_OBJECT_IDNAN, 0, 0, data);
			return ES_ERROR;
		}

		return changeid(newid);
	case P_NAME:
		{
			// Cannot have return characters in object names.
			ep . replacechar('\n', '_');
	
			MCAutoNameRef t_new_name;
			/* UNCHECKED */ ep . copyasnameref(t_new_name);
				
			// MW-2012-09-12; [[ Bug ]] Make sure we compare literally, otherwise can't
			//   change case of names of objects.
			if (getname() != t_new_name)
			{
				MCAutoNameRef t_old_name;
				t_old_name . Clone(getname());
				setname(t_new_name);
				message_with_args(MCM_name_changed, t_old_name, getname());
			}
		}
		break;
	case P_ALT_ID:
		if (!MCU_stoui2(data, i))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		altid = i;
		dirty = False;
		break;
	case P_LAYER:
		if (data == MCtopstring)
			i1 = MAXINT2;
		else if (data == MCbottomstring)
			i1 = 1;
		else if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_OBJECT_LAYERNAN, 0, 0, data);
			return ES_ERROR;
		}
		if (parent == NULL || getcard(parid)->relayer((MCControl *)this, i1) != ES_NORMAL)
		{
			MCeerror->add(EE_OBJECT_BADRELAYER, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SCRIPT:
		return setscriptprop(ep);
	case P_PARENT_SCRIPT:
		return setparentscriptprop(ep);

	case P_FORE_PIXEL:
	case P_BACK_PIXEL:
	case P_HILITE_PIXEL:
	case P_BORDER_PIXEL:
	case P_TOP_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
		if (!getcindex(which - P_FORE_PIXEL, i))
			i = createcindex(which - P_FORE_PIXEL);
		uint4 pvalue;
		if (!MCU_stoui4(data, pvalue))
		{
			MCeerror->add(EE_OBJECT_PIXELNAN, 0, 0, data);
			return ES_ERROR;
		}
		colors[i].pixel = pvalue;
		MCscreen->querycolor(colors[i]);
		delete colornames[i];
		colornames[i] = NULL;
		break;
	case P_BRUSH_COLOR:
		if (!setcolor(DI_BACK, data))
			return ES_ERROR;
		break;
	case P_PEN_COLOR:
		if (!setcolor(DI_FORE, data))
			return ES_ERROR;
		break;
	case P_BRUSH_BACK_COLOR:
	case P_PEN_BACK_COLOR:
		break;
	case P_FORE_COLOR:
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
		if (!setcolor(which - P_FORE_COLOR, data))
			return ES_ERROR;
		break;
	case P_COLORS:
		if (!setcolors(data))
			return ES_ERROR;
		break;
	case P_BRUSH_PATTERN:
		if (!setpattern(DI_BACK, data))
			return ES_ERROR;
		break;
	case P_PEN_PATTERN:
		if (!setpattern(DI_FORE, data))
			return ES_ERROR;
		break;
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
		if (!setpattern(which - P_FORE_PATTERN, data))
			return ES_ERROR;
		break;
	case P_PATTERNS:
		if (!setpatterns(data))
			return ES_ERROR;
		break;
	case P_LOCK_LOCATION:
		if (!MCU_matchflags(data, flags, F_LOCK_LOCATION, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_TEXT_ALIGN:
		if (MCF_parsetextatts(which, data, flags, newfontname, fontheight, fontsize, fontstyle) != ES_NORMAL)
			return ES_ERROR;
		break;
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_TEXT_HEIGHT:
		{
			// MW-2013-03-06: [[ Bug 10698 ]] When a text property is set on a stack we
			//   must recomputefonts() due to substacks. However, substacks can be open
			//   independently of their mainstacks which causes a problem for font mapping.
			//   If we map the font after setting attributes, we won't be able to tell if
			//   the concrete font has changed. Therefore, we map here if not mapped and
			//   then unmap afterwards (only stacks need this - for all other objects
			//   child open => parent open).
			// MW-2013-03-21: [[ Bug ]] The templateStack has no parent, so probably best
			//   *not* to attempt to mapfonts on it!
			// MW-2013-03-28: [[ Bug 10791 ]] Exceptions to every rule - the home stack
			//   can be open but with no font...
			bool t_unmap_font;
			t_unmap_font = false;
			if ((opened == 0 || m_font == nil) && gettype() == CT_STACK && parent != nil)
			{
				mapfont();
				t_unmap_font = true;
			}

			if (data.getlength() == 0)
			{
				// MW-2012-02-19: [[ SplitTextAttrs ]] If the attr is textHeight, just
				//   set the instance var to 0. Otherwise, use setfontattrs() to clear
				//   the appropriate property.
				if (which == P_TEXT_HEIGHT)
					fontheight = 0;
				else
				{
					uint32_t t_font_flags;
					t_font_flags = 0;
					switch(which)
					{
					case P_TEXT_FONT: t_font_flags |= FF_HAS_TEXTFONT; break;
					case P_TEXT_STYLE: t_font_flags |= FF_HAS_TEXTSTYLE; break;
					case P_TEXT_SIZE: t_font_flags |= FF_HAS_TEXTSIZE; break;
					default:
						break;
					}
					setfontattrs(t_font_flags, nil, 0, 0);
				}
			}
			else
			{
				// Parse the data out to the appropriate attrs.
				if (MCF_parsetextatts(which, data, flags, newfontname, fontheight, fontsize, fontstyle) != ES_NORMAL)
					return ES_ERROR;

				// MW-2012-02-19: [[ SplitTextAttrs ] The fontheight value will be set
				//   by MCF_parsetextatts if appropriate. Otherwise, work out what attr
				//   is being set, and use 'setfontattrs()' to do so.
				if (which != P_TEXT_HEIGHT)
				{
					// Convert the font name to a name, but only in the case of it being
					// the textFont prop that we are setting.
					MCAutoNameRef t_font_name;
					if (which == P_TEXT_FONT)
						/* UNCHECKED */ MCNameCreateWithCString(newfontname, t_font_name);

					uint32_t t_font_flags;
					t_font_flags = 0;
					switch(which)
					{
					case P_TEXT_FONT: t_font_flags |= FF_HAS_TEXTFONT; break;
					case P_TEXT_STYLE: t_font_flags |= FF_HAS_TEXTSTYLE; break;
					case P_TEXT_SIZE: t_font_flags |= FF_HAS_TEXTSIZE; break;
					default:
						break;
					}

					setfontattrs(t_font_flags, t_font_name, fontsize, fontstyle);
				}
			}

			// MW-2012-02-19: [[ SplitTextAttrs ]] If the textSize is changed, then make sure we
			//   reset the textHeight to derive from it.
			if (which == P_TEXT_SIZE)
				fontheight = 0;

			// MW-2012-02-14: [[ FontRefs ]] If font/size/style was set, then force a recompute
			//   of fonts if opened.
			if (which != P_TEXT_HEIGHT)
			{
				// MW-2012-12-14: [[ Bug ]] If this object is a stack, always recompute fonts
				//   to ensure substacks update properly.
				// MW-2013-03-21: [[ Bug ]] Unless its the templateStack (parent == nil) in which
				//   case we don't want to do any font recomputation.
				if ((gettype() == CT_STACK && parent != nil) || opened)
				{
					dirty = recomputefonts(parent -> getfontref());
					if (dirty)
						recompute();
				}
			}
			else
				recompute();
				
			if (t_unmap_font)
				unmapfont();

			delete newfontname;
		}
		break;
	case P_SHOW_BORDER:
		if (!MCU_matchflags(data, flags, F_SHOW_BORDER, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SHOW_FOCUS_BORDER:
		return setshowfocusborderprop(ep);

	case P_BORDER_WIDTH:
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		borderwidth = (uint1)i1;
		break;
	case P_FILLED:
	case P_OPAQUE:
		if (!MCU_matchflags(data, flags, F_OPAQUE, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SHADOW:
		if (!MCU_matchflags(data, flags, F_SHADOW, dirty))
		{
			if (!MCU_stoi2(data, i1))
			{ // for SC shadow
				MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
			else
			{
				shadowoffset = (int1)i1;
				flags |= F_SHADOW;
			}
		}
		break;
	case P_SHADOW_OFFSET:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		shadowoffset = (int1)i1;
		break;
	case P_3D:
		if (!MCU_matchflags(data, flags, F_3D, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_VISIBLE:
	case P_INVISIBLE:
		return setvisibleprop(parid, which, ep);
	case P_TRAVERSAL_ON:
		if (!MCU_matchflags(data, flags, F_TRAVERSAL_ON, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (state & CS_KFOCUSED && !(flags & F_TRAVERSAL_ON))
			state &= ~CS_KFOCUSED;
		break;
	case P_ENABLED:
	case P_DISABLED:
		if (!MCU_matchflags(data, flags, F_DISABLED, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (which == P_ENABLED)
		{
			flags ^= F_DISABLED;
			dirty = !dirty;
		}
		if (flags & F_DISABLED && state & CS_KFOCUSED)
			getcard(parid)->kunfocus();
		break;
	case P_SELECTED:
		if (!MCU_stob(data, newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (newstate != (Boolean)((state & CS_SELECTED) != 0))
			if (newstate)
				MCselected->add
				(this);
			else
				MCselected->remove
				(this);
		dirty = False;
		break;
	case P_PROPERTIES:
		if (ep.getformat() == VF_ARRAY)
			return ep.getarray()->setprops(parid, this);
		dirty = False;
		break;
	case P_CUSTOM_PROPERTY_SET:
		{
		MCAutoNameRef t_propset_name;
		/* UNCHECKED */ ep . copyasnameref(t_propset_name);
		/* UNCHECKED */ setpropset(t_propset_name);
		dirty = False;
	}
		break;
	case P_CUSTOM_PROPERTY_SETS:
		/* UNCHECKED */ changepropsets(ep);
		dirty = False;
		break;
	case P_INK:
		for (i = 0 ; i < NUM_INKS ; i++)
			if (data == ink_names[i])
				break;
		if (i < NUM_INKS && (uint1)i != ink)
			ink = (uint1)i;
		else
			dirty = False;
		break;
	case P_CANT_SELECT:
		if (!MCU_stob(data, newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (newstate)
			extraflags |= EF_CANT_SELECT;
		else
			extraflags &= ~EF_CANT_SELECT;
		break;
	case P_BLEND_LEVEL:
	{
		uint2 t_new_blendlevel;
		if (!MCU_stoui2(data, t_new_blendlevel))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		t_new_blendlevel = 100 - MCU_min(MCU_max(0, t_new_blendlevel), 100);
		if ((uint1)t_new_blendlevel != blendlevel)
		{
			blendlevel = (uint1)t_new_blendlevel;

			// MW-2012-04-11: [[ Bug ]] Special case for when a dynamic layer has its
			//   blend level changed - all we need do is invalidate the card.
			if (gettype() < CT_GROUP || !static_cast<MCControl *>(this) -> layer_issprite())
				dirty = True;
			else
			{
				dirty = False;
				static_cast<MCCard *>(parent) -> layer_dirtyrect(static_cast<MCControl *>(this) -> geteffectiverect());
			}
		}
		else
			dirty = False;
	}
	break;

	case P_BOTTOM_LEFT:
	case P_BOTTOM_RIGHT:
	case P_LOCATION:
	case P_TOP_LEFT:
	case P_TOP_RIGHT:
	case P_LEFT:
	case P_RIGHT:
	case P_TOP:
	case P_BOTTOM:
	case P_WIDTH:
	case P_HEIGHT:
	case P_RECTANGLE:
		return setrectprop(which, ep, effective);

	default:
		MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	}
	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		if (gettype() >= CT_GROUP)
			static_cast<MCControl *>(this) -> layer_redrawall();
	}
	return ES_NORMAL;
#endif /* MCObject::setprop */

	MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
	return ES_ERROR;
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCObject::setarrayprop_legacy(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
	switch(which)
	{
	case P_TEXT_STYLE:
	{
		// MW-2013-05-13: [[ Bug ]] Make sure we don't co-erce to a string unless
		//   we are only processing a string.
		MCString data;
		data = ep . getsvalue();
	
		// MW-2011-11-23: [[ Array TextStyle ]] If the key is empty, then we are
		//   manipulating the whole set at once.
		if (MCNameIsEmpty(key))
			return setprop(parid, which, ep, effective);

		// Determine whether we are setting or unsetting.
		Boolean newstate;
		if (!MCU_stob(data, newstate))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}

		// Parse the requested text style.
		Font_textstyle t_style;
		if (MCF_parsetextstyle(MCNameGetOldString(key), t_style) != ES_NORMAL)
			return ES_ERROR;

		// MW-2012-02-19: [[ SplitTextAttrs ]] If we have no textStyle set then
		//   start off with a textStyle of 0, otherwise fetch its setting.
		// MW-2012-02-23: [[ Bug ]] The default style should be FA_DEFAULT_STYLE,
		//   not 0.
		uint2 t_style_set;
		if ((m_font_flags & FF_HAS_TEXTSTYLE) == 0)
			t_style_set = FA_DEFAULT_STYLE;
		else
			t_style_set = gettextstyle();

		// Change the requested style in the style set.
		MCF_changetextstyle(t_style_set, t_style, newstate == True);

		// Now unparse to a style string, and apply.
		MCF_unparsetextatts(P_TEXT_STYLE, ep, 0, nil, 0, 0, t_style_set);
		return setprop(parid, which, ep, False);
	}
	break;
	case P_CUSTOM_KEYS:
		{
			// MW-2013-05-13: [[ Bug ]] Make sure we don't co-erce to a string unless
			//   we are only processing a string.
			MCString data;
			data = ep . getsvalue();
		
			MCObjectPropertySet *p;

			if (data == MCnullmcstring)
			{
				if (findpropset(key, true, p))
					p -> clear();
				break;
			}

			/* UNCHECKED */ ensurepropset(key, true, p);
			/* UNCHECKED */ p -> restrict(ep);
		}
		break;
	case P_CUSTOM_PROPERTIES:
		{
			MCObjectPropertySet *p;

			if (!ep . isarray())
			{
				if (findpropset(key, true, p))
					p -> clear();
				break;
			}
			/* UNCHECKED */ ensurepropset(key, true, p);
			p -> store(ep);
		}
		break;
	default:
		{
			Exec_stat t_stat;
			t_stat = mode_getprop(parid, which, ep, MCNameGetString(key), False);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
				return ES_ERROR;
			}
			return t_stat;
		}
	}
	return ES_NORMAL;
}

#ifdef OLD_EXEC
Exec_stat MCObject::setprops(uint32_t p_parid, MCExecPoint& ep)
{
	MCAutoArrayRef t_array;
	if (!ep . copyasarrayref(&t_array))
		return ES_NORMAL;

	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	MCerrorlock++;

	uintptr_t t_iterator;
	t_iterator = 0;
	MCNameRef t_key;
	MCValueRef t_value;
	while(MCArrayIterate(*t_array, t_iterator, t_key, t_value))
	{
		MCScriptPoint sp(MCStringGetCString(MCNameGetString(t_key)));
		Symbol_type type;
		const LT *te;
		if (sp.next(type) && sp.lookup(SP_FACTOR, te) == PS_NORMAL
		        && te->type == TT_PROPERTY && te->which != P_ID)
		{
			ep . setvalueref(t_value);
			setprop(p_parid, (Properties)te->which, ep, False);
		}
	}	
	MCerrorlock--;
	MCRedrawUnlockScreen();

	return ES_NORMAL;
}
#endif
////////////////////////////////////////////////////////////////////////////////

static bool MCPropertyFormatUIntList(uinteger_t *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
		if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppendFormat(*t_list, "%d", p_list[i]);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyFormatStringList(MCStringRef *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
        if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppend(*t_list, p_list[i]);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyFormatPointList(MCPoint *p_list, uindex_t p_count, char_t p_delimiter, MCStringRef& r_string)
{
    if (p_count == 0)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_count && t_success; i++)
	{
        if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, p_delimiter);
        
		t_success = MCStringAppendFormat(*t_list, "%d,%d", p_list[i].x, p_list[i].y);
	}
	
	if (t_success)
		return MCStringCopy(*t_list, r_string);
	
	return false;
}

static bool MCPropertyParseUIntList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, uinteger_t*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<uinteger_t> t_list;
	
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_uint_string;
		uinteger_t t_d;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), &t_uint_string);
		
		if (t_success)
			t_success = MCU_stoui4(*t_uint_string, t_d);
		
		if (t_success)
			t_success = t_list . Push(t_d);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static bool MCPropertyParseStringList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, MCStringRef*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<MCStringRef> t_list;
    
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCStringRef t_string;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), t_string);
		
		if (t_success)
			t_success = t_list . Push(t_string);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static bool MCPropertyParsePointList(MCStringRef p_input, char_t p_delimiter, uindex_t& r_count, MCPoint*& r_list)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_count = 0;
        return true;
    }
    
	MCAutoArray<MCPoint> t_list;
    
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_point_string;
        MCPoint t_point;
		
		if (!MCStringFirstIndexOfChar(p_input, p_delimiter, t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), &t_point_string);

        if (t_success)
            MCU_stoi2x2(*t_point_string, t_point . x, t_point . y);
        
		if (t_success)
			t_success = t_list . Push(t_point);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
		t_list . Take(r_list, r_count);
	
	return t_success;
}

static MCPropertyInfo *lookup_object_property(const MCObjectPropertyTable *p_table, Properties p_which, bool p_effective, bool p_array_prop)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which && (!p_table -> table[i] . has_effective || p_table -> table[i] . effective == p_effective) &&
            (p_array_prop == p_table -> table[i] . is_array_prop))
			return &p_table -> table[i];
	
	if (p_table -> parent != nil)
		return lookup_object_property(p_table -> parent, p_which, p_effective, p_array_prop);
	
	return nil;
}

Exec_stat MCObject::getarrayprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, MCNameRef p_index, Boolean p_effective)
{
    if (MCNameIsEmpty(p_index))
        return getprop(p_part_id, p_which, ep, p_effective);
    
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, true);
	
	if (t_info != nil && t_info -> getter == nil)
	{
		MCeerror -> add(EE_OBJECT_GETNOPROP, 0, 0);
		return ES_ERROR;
	}
	
	if (t_info != nil)
	{
		MCExecContext ctxt(ep);
		
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				MCAutoValueRef t_any;
				((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCValueRef&))t_info -> getter)(ctxt, t_object, p_index, &t_any);
				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_any);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeBool:
			{
				bool t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, bool&))t_info -> getter)(ctxt, t_object, p_index, t_value);
				if (!ctxt . HasError())
				{
					ep . setboolean(t_value ? True : False);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeString:
			{
				MCAutoStringRef t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCStringRef&))t_info -> getter)(ctxt, t_object, p_index, &t_value);
				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_value);
					return ES_NORMAL;
				}
			}
				break;
                
			case kMCPropertyTypeArray:
			{
				MCAutoArrayRef t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCArrayRef&))t_info -> getter)(ctxt, t_object, p_index, &t_value);
				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_value);
					return ES_NORMAL;
				}
			}
				break;
        }
        return ctxt . Catch(0, 0);
    }
    
    Exec_stat t_stat;
    t_stat = mode_getprop(p_part_id, p_which, ep, MCNameGetString(p_index), False);
    if (t_stat == ES_NOT_HANDLED)
    {
        MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
        return ES_ERROR;
    }
    return t_stat;
}

Exec_stat MCObject::setarrayprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, MCNameRef p_index, Boolean p_effective)
{
    if (MCNameIsEmpty(p_index))
        return setprop(p_part_id, p_which, ep, p_effective);
    
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, true);
    
	if (t_info != nil && t_info -> setter == nil)
	{
		MCeerror -> add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	}
    
	if (t_info != nil)
	{
		MCExecContext ctxt(ep);
		
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				MCAutoValueRef t_value;
				/* UNCHECKED */ ep . copyasvalueref(&t_value);
				((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCValueRef))t_info -> setter)(ctxt, t_object, p_index, *t_value);
			}
                break;
				
			case kMCPropertyTypeBool:
			{
				bool t_value;
				if (!ep . copyasbool(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAB);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, bool))t_info -> setter)(ctxt, t_object, p_index, t_value);
			}
                break;
                
                
			case kMCPropertyTypeString:
			{
				MCAutoStringRef t_value;
                if (!ep . copyasstringref(&t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAS);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCStringRef))t_info -> setter)(ctxt, t_object, p_index, *t_value);
			}
                break;
                
			case kMCPropertyTypeArray:
			{
				MCAutoArrayRef t_value;
				if (!ep . copyasarrayref(&t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTANARRAY);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCNameRef, MCArrayRef))t_info -> setter)(ctxt, t_object, p_index, *t_value);
			}
                break;
		}
		
		if (!ctxt . HasError())
			return ES_NORMAL;
        
		return ctxt . Catch(0, 0);
	}
    
    Exec_stat t_stat;
    t_stat = mode_getprop(p_part_id, p_which, ep, MCNameGetString(p_index), False);
    if (t_stat == ES_NOT_HANDLED)
    {
        MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
        return ES_ERROR;
    }
    return t_stat;
}

Exec_stat MCObject::getprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, Boolean p_effective)
{
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, false);
	
	if (t_info != nil && t_info -> getter == nil)
	{
		MCeerror -> add(EE_OBJECT_GETNOPROP, 0, 0);
		return ES_ERROR;
	}
	
	if (t_info != nil)
	{
		MCExecContext ctxt(ep);
		
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				MCAutoValueRef t_any;
				((void(*)(MCExecContext&, MCObjectPtr, MCValueRef&))t_info -> getter)(ctxt, t_object, &t_any);
				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_any);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeBool:
			{
				bool t_value;
				((void(*)(MCExecContext&, MCObjectPtr, bool&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setboolean(t_value ? True : False);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeInt16:
			case kMCPropertyTypeInt32:
			{
				integer_t t_value;
				((void(*)(MCExecContext&, MCObjectPtr, integer_t&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setint(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeUInt16:
			case kMCPropertyTypeUInt32:
			{
				uinteger_t t_value;
				((void(*)(MCExecContext&, MCObjectPtr, uinteger_t&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setuint(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeDouble:
			{
				double t_value;
				((void(*)(MCExecContext&, MCObjectPtr, double&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setnvalue(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeChar:
			{
				char_t t_value;
				((void(*)(MCExecContext&, MCObjectPtr, char_t&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setchar((char)t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeString:
			case kMCPropertyTypeBinaryString:
			{	
				MCAutoStringRef t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCStringRef&))t_info -> getter)(ctxt, t_object, &t_value);
				if (!ctxt . HasError())
				{
					ep . setvalueref(*t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeColor:
			{
				MCColor t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCColor&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setcolor(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeRectangle:
			{
				MCRectangle t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCRectangle&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setrectangle(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypePoint:
			{
				MCPoint t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCPoint&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setpoint(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeInt16X2:
			{
				integer_t t_value[2];
				((void(*)(MCExecContext&, MCObjectPtr, integer_t[2]))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setstringf("%d,%d", t_value[0], t_value[1]);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeInt16X4:
			{
				integer_t t_value[4];
				((void(*)(MCExecContext&, MCObjectPtr, integer_t[4]))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					ep . setstringf("%d,%d,%d,%d", t_value[0], t_value[1], t_value[2], t_value[3]);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeArray:
			{
				MCAutoArrayRef t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCArrayRef&))t_info -> getter)(ctxt, t_object, &t_value);
				if (!ctxt . HasError())
				{
                    if (*t_value != nil)
                        ep . setvalueref(*t_value);
                    else
                        ep . clear();
                    return ES_NORMAL;
				}
			}
				break;
                
			case kMCPropertyTypeEnum:
			{
				int t_value;
				((void(*)(MCExecContext&, MCObjectPtr, int&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					MCExecEnumTypeInfo *t_enum_info;
					t_enum_info = (MCExecEnumTypeInfo *)(t_info -> type_info);
					for(uindex_t i = 0; i < t_enum_info -> count; i++)
						if (t_enum_info -> elements[i] . value == t_value)
						{
							ep . setcstring(t_enum_info -> elements[i] . tag);
							return ES_NORMAL;
						}
					
					// THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
					MCAssert(false);
					return ES_ERROR;
				}
			}
				break;

			case kMCPropertyTypeOptionalEnum:
			{
				int t_value;
				int *t_value_ptr;
				t_value_ptr = &t_value;
				((void(*)(MCExecContext&, MCObjectPtr, int*&))t_info -> getter)(ctxt, t_object, t_value_ptr);
				if (!ctxt . HasError())
				{
					if (t_value_ptr == nil)
						ep . clear();
					else
					{
						MCExecEnumTypeInfo *t_enum_info;
						t_enum_info = (MCExecEnumTypeInfo *)(t_info -> type_info);
						for(uindex_t i = 0; i < t_enum_info -> count; i++)
							if (t_enum_info -> elements[i] . value == t_value)
							{
								ep . setcstring(t_enum_info -> elements[i] . tag);
								return ES_NORMAL;
							}
						
						// THIS MEANS A METHOD HAS RETURNED AN ILLEGAL VALUE
						MCAssert(false);
						return ES_ERROR;
					}
					return ES_NORMAL;
				}
			}
				break;
								
			case kMCPropertyTypeSet:
			{
				unsigned int t_value;
				((void(*)(MCExecContext&, MCObjectPtr, unsigned int&))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					MCExecSetTypeInfo *t_set_info;
					t_set_info = (MCExecSetTypeInfo *)(t_info -> type_info);
					
					bool t_first;
					t_first = true;
					
					ep . clear();
					for(uindex_t i = 0; i < t_set_info -> count; i++)
						if (((1 << t_set_info -> elements[i] . bit) & t_value) != 0)
						{
							ep . concatcstring(t_set_info -> elements[i] . tag, EC_COMMA, t_first);
							t_first = false;
						}
					
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeCustom:
			{
				MCExecCustomTypeInfo *t_custom_info;
				t_custom_info = (MCExecCustomTypeInfo *)(t_info -> type_info);
				
				MCAssert(t_custom_info -> size <= 64);
				
				char t_value[64];
				((void(*)(MCExecContext&, MCObjectPtr, void *))t_info -> getter)(ctxt, t_object, t_value);
				if (!ctxt . HasError())
				{
					MCAutoStringRef t_value_ref;
					((MCExecCustomTypeFormatProc)t_custom_info -> format)(ctxt, t_value, &t_value_ref);
					((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
					if (!ctxt . HasError())
					{
						ep . setvalueref(*t_value_ref);
						return ES_NORMAL;
					}
				}
				
			}
				break;

			case kMCPropertyTypeOptionalInt16:	
			{
				integer_t t_value;
				integer_t *t_value_ptr;
				t_value_ptr = &t_value;
				((void(*)(MCExecContext&, MCObjectPtr, integer_t*&))t_info -> getter)(ctxt, t_object, t_value_ptr);
				if (!ctxt . HasError())
				{
					if (t_value_ptr == nil)
						ep . clear();
					else
						ep . setint(t_value);
					return ES_NORMAL;
				}
			}
				break;

			case kMCPropertyTypeOptionalUInt16:	
			case kMCPropertyTypeOptionalUInt32:
			{
				uinteger_t t_value;
				uinteger_t *t_value_ptr;
				t_value_ptr = &t_value;
				((void(*)(MCExecContext&, MCObjectPtr, uinteger_t*&))t_info -> getter)(ctxt, t_object, t_value_ptr);
				if (!ctxt . HasError())
				{
					if (t_value_ptr == nil)
						ep . clear();
					else
						ep . setint(t_value);
					return ES_NORMAL;
				}
			}
				break;
				
			case kMCPropertyTypeOptionalString:
			{
				MCAutoStringRef t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCStringRef&))t_info -> getter)(ctxt, t_object, &t_value);
				if (!ctxt . HasError())
				{
					if (*t_value == nil)
						ep . clear();
					else
						ep . setvalueref(*t_value);
					
					return ES_NORMAL;
				}
				
			}
				break;

			case kMCPropertyTypeOptionalRectangle:
			{
				MCRectangle t_value;
				MCRectangle *t_value_ptr;
				t_value_ptr = &t_value;
				((void(*)(MCExecContext&, MCObjectPtr, MCRectangle*&))t_info -> getter)(ctxt, t_object, t_value_ptr);
				if (!ctxt . HasError())
				{
					if (t_value_ptr == nil)
						ep . clear();
					else
						ep . setrectangle(t_value);
					return ES_NORMAL;
				}
			}
				break;
                
            case kMCPropertyTypeLinesOfString:
            {
				MCStringRef* t_value;
                uindex_t t_count;
				((void(*)(MCExecContext&, MCObjectPtr, uindex_t&, MCStringRef*&))t_info -> getter)(ctxt, t_object, t_count, t_value);
				if (!ctxt . HasError())
                {
                    MCAutoStringRef t_output;
                    if (MCPropertyFormatStringList(t_value, t_count, '\n', &t_output))
                    {
                        ep . setvalueref(*t_output);
                        return ES_NORMAL;
                    }
				}
			}
				break;
                
            case kMCPropertyTypeLinesOfUInt:
            case kMCPropertyTypeItemsOfUInt:
            {
				uinteger_t* t_value;
                uindex_t t_count;
				((void(*)(MCExecContext&, MCObjectPtr, uindex_t&, uinteger_t*&))t_info -> getter)(ctxt, t_object, t_count, t_value);
				if (!ctxt . HasError())
                {
                    MCAutoStringRef t_output;
                    char_t t_delimiter;
                    t_delimiter = t_info -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';
                    if (MCPropertyFormatUIntList(t_value, t_count, t_delimiter, &t_output))
                    {
                        ep . setvalueref(*t_output);
                        return ES_NORMAL;
                    }
				}
			}
				break;
                
            case kMCPropertyTypeLinesOfPoint:
            {
				MCPoint* t_value;
                uindex_t t_count;
				((void(*)(MCExecContext&, MCObjectPtr, uindex_t&, MCPoint*&))t_info -> getter)(ctxt, t_object, t_count, t_value);
				if (!ctxt . HasError())
                {
                    MCAutoStringRef t_output;
                    if (MCPropertyFormatPointList(t_value, t_count, '\n', &t_output))
                    {
                        ep . setvalueref(*t_output);
                        return ES_NORMAL;
                    }
				}
			}
				break;
		}
		
		return ctxt . Catch(0, 0);
	}
	

	return getprop_legacy(p_part_id, p_which, ep, p_effective);
}

Exec_stat MCObject::setprop(uint32_t p_part_id, Properties p_which, MCExecPoint& ep, Boolean p_effective)
{
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, false);

	if (t_info != nil && t_info -> setter == nil)
	{
		MCeerror -> add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	}

	if (t_info != nil)
	{
		MCExecContext ctxt(ep);
		
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		switch(t_info -> type)
		{
			case kMCPropertyTypeAny:
			{
				MCAutoValueRef t_value;
				if (!ep . copyasvalueref(&t_value))
					return ES_ERROR;
				((void(*)(MCExecContext&, MCObjectPtr, MCValueRef))t_info -> setter)(ctxt, t_object, *t_value);
			}
			break;
				
			case kMCPropertyTypeBool:
			{
				bool t_value;
				if (!ep . copyasbool(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAB);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, bool))t_info -> setter)(ctxt, t_object, t_value);
			}
			break;
				 
			case kMCPropertyTypeInt16:
			{
				integer_t t_value;
				if (!ep . copyasint(t_value) ||
					t_value < -32768 || t_value > 32767)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, integer_t))t_info -> setter)(ctxt, t_object, t_value);				
			}
			break;
				
			case kMCPropertyTypeInt32:
			{
				integer_t t_value;
				if (!ep . copyasint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, integer_t))t_info -> setter)(ctxt, t_object, t_value);				
			}
			break; 
			
			case kMCPropertyTypeUInt16:
			{
				uinteger_t t_value;
				if (!ep . copyasuint(t_value) ||
					t_value < 0 || t_value > 65535)
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, uinteger_t))t_info -> setter)(ctxt, t_object, t_value);				
			}
			break;
				
			case kMCPropertyTypeUInt32:
			{
				uinteger_t t_value;
				if (!ep . copyasuint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, uinteger_t))t_info -> setter)(ctxt, t_object, t_value);				
			}
			break;
				
			case kMCPropertyTypeDouble:
			{
				double t_value;
				if (!ep . copyasdouble(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAN);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, double))t_info -> setter)(ctxt, t_object, t_value);	
			}
			break;
			
			case kMCPropertyTypeChar:
			{
				char_t t_value;
				if (!ep . copyaschar(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAC);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, char_t))t_info -> setter)(ctxt, t_object, t_value);	
			}
			break;
				
			case kMCPropertyTypeString:
			case kMCPropertyTypeBinaryString:
			{
				MCAutoStringRef t_value;
				if (!ep . copyasstringref(&t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NAC);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCStringRef))t_info -> setter)(ctxt, t_object, *t_value);	
			}
			break;
				
			case kMCPropertyTypeColor:
			{
				MCColor t_value;
				if (!ep . copyaslegacycolor(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTACOLOR);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCColor))t_info -> setter)(ctxt, t_object, t_value);	
			}
			break;
				
			case kMCPropertyTypeRectangle:
			{
				MCRectangle t_value;
				if (!ep . copyaslegacyrectangle(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCRectangle))t_info -> setter)(ctxt, t_object, t_value);	
			}
			break;
				
			case kMCPropertyTypePoint:
			{
				MCPoint t_value;
				if (!ep . copyaslegacypoint(t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAPOINT);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCPoint))t_info -> setter)(ctxt, t_object, t_value);	
			}
			break;
			
			case kMCPropertyTypeInt16X2:
			{
				int2 a, b;
				if (!MCU_stoi2x2(ep . getsvalue(), a, b))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAINTPAIR);
				if (!ctxt . HasError())
				{
					integer_t t_value[2];
					t_value[0] = a;
					t_value[1] = b;
					((void(*)(MCExecContext&, MCObjectPtr, integer_t[2]))t_info -> setter)(ctxt, t_object, t_value);
				}
			}
			break;
			
			case kMCPropertyTypeInt16X4:
			{
				int2 a, b, c, d;
				if (!MCU_stoi2x4(ep . getsvalue(), a, b, c, d))
					ctxt . LegacyThrow(EE_PROPERTY_NOTAINTQUAD);
				if (!ctxt . HasError())
				{
					integer_t t_value[4];
					t_value[0] = a;
					t_value[1] = b;
					t_value[2] = c;
					t_value[3] = d;
					((void(*)(MCExecContext&, MCObjectPtr, integer_t[4]))t_info -> setter)(ctxt, t_object, t_value);
				}
			}
			break;

			case kMCPropertyTypeArray:
			{
				MCAutoArrayRef t_value;
				if (!ep . copyasarrayref(&t_value))
					ctxt . LegacyThrow(EE_PROPERTY_NOTANARRAY);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCArrayRef))t_info -> setter)(ctxt, t_object, *t_value);	
			}
			break;	

			case kMCPropertyTypeEnum:
			{
				MCExecEnumTypeInfo *t_enum_info;
				t_enum_info = (MCExecEnumTypeInfo *)t_info -> type_info;
				
				bool t_found;
				t_found = false;
				intenum_t t_value;
				for(uindex_t i = 0; i < t_enum_info -> count; i++)
					if (!t_enum_info -> elements[i] . read_only &&
						MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
					{
						t_found = true;
						t_value = t_enum_info -> elements[i] . value;
					}
			
				if (!t_found)
					ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, int))t_info -> setter)(ctxt, t_object, t_value);
			}
			break;

			case kMCPropertyTypeOptionalEnum:
			{
				MCExecEnumTypeInfo *t_enum_info;
				t_enum_info = (MCExecEnumTypeInfo *)t_info -> type_info;
				
				intenum_t t_value;
				intenum_t* t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					bool t_found;
					t_found = false;
					for(uindex_t i = 0; i < t_enum_info -> count; i++)
						if (!t_enum_info -> elements[i] . read_only &&
							MCU_strcasecmp(ep . getcstring(), t_enum_info -> elements[i] . tag) == 0)
						{
							t_found = true;
							t_value = t_enum_info -> elements[i] . value;
						}
				
					if (!t_found)
						ctxt . LegacyThrow(EE_PROPERTY_BADENUMVALUE);
				}
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, int*))t_info -> setter)(ctxt, t_object, t_value_ptr);
			}
			break;

			case kMCPropertyTypeSet:
			{
				MCExecSetTypeInfo *t_set_info;
				t_set_info = (MCExecSetTypeInfo *)(t_info -> type_info);
				
				intset_t t_value = 0;
				char **t_elements;
				uindex_t t_element_count;
				MCCStringSplit(ep . getcstring(), ',', t_elements, t_element_count);

				for (uindex_t i = 0; i < t_element_count; i++)
				{
					for (uindex_t j = 0; j < t_set_info -> count; j++)
					{
						if (MCU_strcasecmp(t_elements[i], t_set_info -> elements[j] . tag) == 0)
						{
							t_value |= 1 << t_set_info -> elements[j] . bit;
							break;
						}
					}
				}

				MCCStringArrayFree(t_elements, t_element_count);
				((void(*)(MCExecContext&, MCObjectPtr, unsigned int))t_info -> setter)(ctxt, t_object, t_value);
			}
			break;
				
			case kMCPropertyTypeCustom:
			{
				MCExecCustomTypeInfo *t_custom_info;
				t_custom_info = (MCExecCustomTypeInfo *)(t_info -> type_info);
				
				MCAssert(t_custom_info -> size <= 64);
			
				MCAutoStringRef t_input_value;
				/* UNCHECKED */ ep . copyasstringref(&t_input_value);
				
				char t_value[64];
				((MCExecCustomTypeParseProc)t_custom_info -> parse)(ctxt, *t_input_value, t_value);
				if (!ctxt . HasError())
				{
					((void(*)(MCExecContext&, MCObjectPtr, void *))t_info -> setter)(ctxt, t_object, t_value);
					((MCExecCustomTypeFreeProc)t_custom_info -> free)(ctxt, t_value);
				}
			}
			break;

			case kMCPropertyTypeOptionalInt16:
			{
				integer_t t_value;
				integer_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasint(t_value) ||
						t_value < -32768 || t_value > 32767)
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, integer_t*))t_info -> setter)(ctxt, t_object, t_value_ptr);		
			}
			break;

			case kMCPropertyTypeOptionalUInt16:
			{
				uinteger_t t_value;
				uinteger_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasuint(t_value) ||
						t_value < 0 || t_value > 65535)
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, uinteger_t*))t_info -> setter)(ctxt, t_object, t_value_ptr);		
			}
			break;

			case kMCPropertyTypeOptionalUInt32:
			{
				uinteger_t t_value;
				uinteger_t *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyasuint(t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NAN);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, uinteger_t*))t_info -> setter)(ctxt, t_object, t_value_ptr);		
			}
			break;
			
			case kMCPropertyTypeOptionalString:
			{
				MCAutoStringRef t_value;
				if (!ep . isempty())
				{
					if (!ep . copyasstringref(&t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NAS);
				}
				
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCStringRef))t_info -> setter)(ctxt, t_object, *t_value);
			}
			break;

			case kMCPropertyTypeOptionalRectangle:
			{
				MCRectangle t_value;
				MCRectangle *t_value_ptr;
				if (ep . isempty())
					t_value_ptr = nil;
				else
				{
					t_value_ptr = &t_value;
					if (!ep . copyaslegacyrectangle(t_value))
						ctxt . LegacyThrow(EE_PROPERTY_NOTARECT);
				}
				if (!ctxt . HasError())
					((void(*)(MCExecContext&, MCObjectPtr, MCRectangle*))t_info -> setter)(ctxt, t_object, t_value_ptr);	
			}
			break;
                
            case kMCPropertyTypeLinesOfString:
            {
				MCAutoStringRef t_input;
                MCStringRef *t_value;
                uindex_t t_count;
                
                if (!ep . copyasstringref(&t_input) || !MCPropertyParseStringList(*t_input, '\n', t_count, t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAS);
                
				if (!ctxt . HasError())
                    ((void(*)(MCExecContext&, MCObjectPtr, uindex_t, MCStringRef*))t_info -> setter)(ctxt, t_object, t_count, t_value);
			}
				break;
                
            case kMCPropertyTypeLinesOfUInt:
            case kMCPropertyTypeItemsOfUInt:
            {
                MCAutoStringRef t_input;
				uinteger_t* t_value;
                uindex_t t_count;
                
                char_t t_delimiter;
                t_delimiter = t_info -> type == kMCPropertyTypeLinesOfUInt ? '\n' : ',';

                if (!ep . copyasstringref(&t_input) || !MCPropertyParseUIntList(*t_input, t_delimiter, t_count, t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAN);
                
				if (!ctxt . HasError())
                    ((void(*)(MCExecContext&, MCObjectPtr, uindex_t, uinteger_t*))t_info -> setter)(ctxt, t_object, t_count, t_value);
			}
				break;
                
            case kMCPropertyTypeLinesOfPoint:
            {
				MCAutoStringRef t_input;
                MCPoint *t_value;
                uindex_t t_count;
                
                if (!ep . copyasstringref(&t_input) || !MCPropertyParsePointList(*t_input, '\n', t_count, t_value))
                    ctxt . LegacyThrow(EE_PROPERTY_NAS);
                
				if (!ctxt . HasError())
                    ((void(*)(MCExecContext&, MCObjectPtr, uindex_t, MCPoint*))t_info -> setter)(ctxt, t_object, t_count, t_value);
			}
				break;

			default:
				ctxt . Unimplemented();
				break;
		}
		
		if (!ctxt . HasError())
			return ES_NORMAL;

		return ctxt . Catch(0, 0);
	}
	
	return setprop_legacy(p_part_id, p_which, ep, p_effective);
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::getboolprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, bool& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasbool(r_value))
		return;

	ctxt . Throw();
}

void MCObject::setboolprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, bool p_value)
{
	ctxt . GetEP() . setbool(p_value);
	if (setprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL)
		return;
	
	ctxt . Throw();
}

//////////

void MCObject::getintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, integer_t& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasint(r_value))
		return;

	ctxt . Throw();
}

//////////

void MCObject::getuintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, uinteger_t& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasuint(r_value))
		return;

	ctxt . Throw();
}

void MCObject::setuintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, uinteger_t p_value)
{
	ctxt . GetEP() . setuint(p_value);
	if (setprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL)
		return;
	
	ctxt . Throw();
}

//////////

void MCObject::getdoubleprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, double& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasdouble(r_value))
		return;

	ctxt . Throw();
}

void MCObject::getnumberprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCNumberRef& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasnumber(r_value))
		return;

	ctxt . Throw();
}

/////////

void MCObject::getstringprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCStringRef& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasstring(r_value))
		return;

	ctxt . Throw();
}

void MCObject::setstringprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCStringRef p_value)
{
	ctxt . GetEP() . setvalueref(p_value);
	if (setprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL)
		return;
	
	ctxt . Throw();
}

/////////

void MCObject::getarrayprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCArrayRef& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasarray(r_value))
		return;

	ctxt . Throw();
}

void MCObject::getvariantprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCValueRef& r_value)
{
	if (getprop(p_part_id, p_which, ctxt . GetEP(), p_effective) == ES_NORMAL &&
		ctxt . GetEP() . copyasvariant(r_value))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

#include "props.cpp"
