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

#include "globals.h"
#include "mctheme.h"

#include "license.h"
#include "context.h"
#include "mode.h"

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
    // SN-2013-07-26: [[ Bug 11020 ]] ep.gethandler() result was not checked
    // before calling hasname and caused a crash with undefined properties
	if (!MClockmessages && (ep.getobj() != this || (ep.gethandler() != nil && !ep.gethandler()->hasname(t_getprop_name))))
	{
		MCParameter p1;
		p1.setnameref_unsafe_argument(t_param_name);

		MCStack *oldstackptr = MCdefaultstackptr;
		MCdefaultstackptr = getstack();
		MCObject *oldtargetptr = MCtargetptr;
		MCtargetptr = this;
		Boolean added = False;
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
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
	{
		MCresult -> fetch(ep);

		// MW-2007-07-03: [[ Bug 3213 ]] Failing to grab the value means that
		//   things such as doSomething the uProp of me, the uProp2 of me
		// results in incorrect parameter evaluation (the second custom prop
		// invocation clobbers the result of the first).
		if (ep.getformat() == VF_STRING || ep.getformat() == VF_BOTH)
			ep.grabsvalue();
		else if (ep.getformat() == VF_ARRAY)
			ep.grabarray();
	}

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
		MCVariableValue *p;
		if (findpropset(p_set_name, false, p))
			p -> fetch_element(ep, MCNameGetOldString(p_prop_name));
		else
			ep.clear();

		t_stat = ES_NORMAL;
	}

	return t_stat;
}

Exec_stat MCObject::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
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
			if (patterns[i].id < PI_END && patterns[i].id > PI_PATTERNS)
				ep.setint(patterns[i].id - PI_PATTERNS);

			else
				ep.setint(patterns[i].id);
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
    case P_INVISIBLE:
        {
			// MERG-2013-05-01: [[ EffVisible ]] Add 'effective' adjective to
			//   the visible property.
            bool t_vis;
			t_vis = getflag(F_VISIBLE);
			
            // if visible and effective and parent is a
            // group then keep searching parent properties 
            if (t_vis && effective && parent != NULL && parent->gettype() == CT_GROUP)
                return parent->getprop(parid, which, ep, effective);
			
			ep.setboolean(which == P_VISIBLE ? t_vis : !t_vis);
        }
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
		// MERG-2013-05-07: [[ RevisedPropsProp ]] Add support for 'the effective
		//   properties of object ...'.
		return getproparray(ep, parid, effective);
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
			t_stat = mode_getprop(parid, which, ep, MCnullmcstring, effective);
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
Exec_stat MCObject::getarrayprop(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
#ifdef /* MCObject::getarrayprop */ LEGACY_EXEC
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
			MCVariableValue *p;
			if (findpropset(key, true, p))
			{
				if (which == P_CUSTOM_KEYS)
					p -> getkeys(ep);
				else
					ep.setarray(p, False);
			}
			else
				ep.clear();
		}
		break;
	default:
		{
			Exec_stat t_stat;
			t_stat = mode_getprop(parid, which, ep, MCNameGetOldString(key), False);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
				return ES_ERROR;
			}
			return t_stat;
		}
	}
	return ES_NORMAL;
#endif /* MCObject::getarrayprop */
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
		bool t_old_script_encrypted = m_script_encrypted;
		if (data.getstring()[length - 1] != '\n')
		{
			script = new char[length + 2];
			memcpy(script, data.getstring(), length);
			script[length++] = '\n';
			script[length] = '\0';
		}
		else
			script = data.clone();
			
		// IM-2013-05-29: [[ BZ 10916 ]] flag new script as unencrypted
		m_script_encrypted = false;
		getstack() -> securescript(this);
		
		flags |= F_SCRIPT;
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
				m_script_encrypted = t_old_script_encrypted;
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
	
	// MW-2013-07-18: [[ Bug 11037 ]] Make sure the object isn't in the hierarchy
	//   of the parentScript.
	bool t_is_cyclic;
	t_is_cyclic = false;
	if (t_stat == ES_NORMAL)
	{
		MCObject *t_parent_object;
		t_parent_object = t_object;
		while(t_parent_object != nil)
		{
			if (t_parent_object == this)
			{
				t_is_cyclic = true;
				break;
			}
			
			MCParentScript *t_super_parent_script;
			t_super_parent_script = t_parent_object -> getparentscript();
			if (t_super_parent_script != nil)
				t_parent_object = t_super_parent_script -> GetObject();
			else
				t_parent_object = nil;
		}
		
		if (t_is_cyclic)
			t_stat = ES_ERROR;
	}

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
				
			// MW-2013-05-30: [[ InheritedPscripts ]] Make sure we resolve the the
			//   parent script as pointing to the object (so Inherit works correctly).
			t_use -> GetParent() -> Resolve(t_object);
			
			// MW-2013-05-30: [[ InheritedPscripts ]] Next we have to ensure the
			//   inheritence hierarchy is in place (The inherit call will create
			//   super-uses, and will return false if there is not enough memory).
			if (!t_use -> Inherit())
				t_stat = ES_ERROR;

			// We have succeeded in creating a new use of an object as a parent
			// script, so now release the old parent script this object points
			// to (if any) and install the new one.
			if (parent_script != NULL)
				parent_script -> Release();

			parent_script = t_use;

			// MW-2013-05-30: [[ InheritedPscripts ]] Make sure we update all the
			//   uses of this object if it is being used as a parentScript. This
			//   is because the inheritence hierarchy has been updated and so the
			//   super_use chains need to be remade.
			MCParentScript *t_this_parent;
			if (getstate(CS_IS_PARENTSCRIPT))
			{
				t_this_parent = MCParentScript::Lookup(this);
				if (t_this_parent != nil)
					if (!t_this_parent -> Reinherit())
						t_stat = ES_ERROR;
			}
		}
	}
	else
	{
		// MW-2013-07-18: [[ Bug 11037 ]] Report an appropriate error if the hierarchy
		//   is cyclic.
		if (!t_is_cyclic)
			MCeerror -> add(EE_PARENTSCRIPT_BADOBJECT, 0, 0, data);
		else
			MCeerror -> add(EE_PARENTSCRIPT_CYCLICOBJECT, 0, 0, data);
	}

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
    // SN-2013-07-26: [[ Bug 11020 ]] ep.gethandler() result was not checked
    // before calling hasname and caused a crash with undefined properties
	if (!MClockmessages && (ep.getobj() != this || (ep.gethandler() != nil && !ep.gethandler()->hasname(t_setprop_name))))
	{
		MCParameter p1, p2;
		p1.setnext(&p2);

		p1.setnameref_unsafe_argument(t_param_name);
		p2.set_argument(ep);
		
		MCStack *oldstackptr = MCdefaultstackptr;
		MCdefaultstackptr = getstack();
		MCObject *oldtargetptr = MCtargetptr;
		MCtargetptr = this;
		Boolean added = False;
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
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
		MCVariableValue *p;
		/* UNCHECKED */ ensurepropset(p_set_name, false, p);
		return p->store_element(ep, MCNameGetOldString(p_prop_name));
	}

	return t_stat;
}

Exec_stat MCObject::setprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
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
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCObject::setarrayprop(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
#ifdef /* MCObject::setarrayprop */ LEGACY_EXEC
	MCString data;
	data = ep . getsvalue();
	switch(which)
	{
	case P_TEXT_STYLE:
	{
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
			MCVariableValue *p;

			if (data == MCnullmcstring)
			{
				if (findpropset(key, true, p))
					p -> clear();
				break;
			}

			/* UNCHECKED */ ensurepropset(key, true, p);

			if (ep.getsvalue().getstring()[ep.getsvalue().getlength() - 1] != '\n')
				ep.appendnewline();
			char *string = ep.getsvalue().clone();
			char *eptr = string;
			MCVariableValue *newp = new MCVariableValue;
			while ((eptr = strtok(eptr, "\n")) != NULL)
			{
				if (p != NULL)
					p->fetch_element(ep, eptr);
				else
					ep.clear();
				newp->store_element(ep, eptr);
				eptr = NULL;
			}
			delete string;
			p -> exchange(*newp);
			delete newp;
		}
		break;
	case P_CUSTOM_PROPERTIES:
		{
			MCVariableValue *p;

			if (ep.getformat() != VF_ARRAY)
			{
				if (findpropset(key, true, p))
					p -> clear();
				break;
			}
			/* UNCHECKED */ ensurepropset(key, true, p);
			p -> assign(*ep.getarray());
		}
		break;
	default:
		{
			Exec_stat t_stat;
			t_stat = mode_getprop(parid, which, ep, MCNameGetOldString(key), False);
			if (t_stat == ES_NOT_HANDLED)
			{
				MCeerror->add(EE_OBJECT_GETNOPROP, 0, 0);
				return ES_ERROR;
			}
			return t_stat;
		}
	}
	return ES_NORMAL;
#endif /* MCObject::setarrayprop */
}

////////////////////////////////////////////////////////////////////////////////

#include "props.cpp"
