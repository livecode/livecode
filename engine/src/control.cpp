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

//#include "execpt.h"
#include "util.h"
#include "sellst.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "control.h"
#include "group.h"
#include "field.h"
#include "scrolbar.h"
#include "mcerror.h"
#include "hndlrlst.h"
#include "stacklst.h"
#include "undolst.h"
#include "mctheme.h"
#include "dispatch.h"
#include "parentscript.h"

#include "globals.h"
#include "context.h"
#include "bitmapeffect.h"
#include "graphicscontext.h"
#include "graphics_util.h"

#include "exec.h"

MCControl *MCControl::focused;
int2 MCControl::defaultmargin = 4;
int2 MCControl::xoffset;
int2 MCControl::yoffset;
double MCControl::aspect;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCControl::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_LEFT_MARGIN, Int16, MCControl, LeftMargin)
	DEFINE_RW_OBJ_PROPERTY(P_RIGHT_MARGIN, Int16, MCControl, RightMargin)
	DEFINE_RW_OBJ_PROPERTY(P_TOP_MARGIN, Int16, MCControl, TopMargin)
	DEFINE_RW_OBJ_PROPERTY(P_BOTTOM_MARGIN, Int16, MCControl, BottomMargin)
    DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_MARGINS, InterfaceMargins, MCControl, Margins)
	DEFINE_RW_OBJ_PROPERTY(P_TOOL_TIP, String, MCControl, ToolTip)
	DEFINE_RW_OBJ_PROPERTY(P_UNICODE_TOOL_TIP, BinaryString, MCControl, UnicodeToolTip)
	DEFINE_RW_OBJ_NON_EFFECTIVE_ENUM_PROPERTY(P_LAYER_MODE, InterfaceLayerMode, MCControl, LayerMode)
	DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_LAYER_MODE, InterfaceLayerMode, MCControl, LayerMode)
    
    DEFINE_RW_OBJ_RECORD_PROPERTY(P_BITMAP_EFFECT_DROP_SHADOW, MCControl, DropShadow)
    DEFINE_RW_OBJ_RECORD_PROPERTY(P_BITMAP_EFFECT_OUTER_GLOW, MCControl, OuterGlow)
    DEFINE_RW_OBJ_RECORD_PROPERTY(P_BITMAP_EFFECT_INNER_GLOW, MCControl, InnerGlow)
    DEFINE_RW_OBJ_RECORD_PROPERTY(P_BITMAP_EFFECT_COLOR_OVERLAY, MCControl, ColorOverlay)
    DEFINE_RW_OBJ_RECORD_PROPERTY(P_BITMAP_EFFECT_INNER_SHADOW, MCControl, InnerShadow)
};

MCObjectPropertyTable MCControl::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCControl::MCControl()
{
	rect.x = rect.y = -1;
	rect.width = MCminsize << 4;
	rect.height = MCminsize << 2;
	leftmargin = rightmargin = topmargin = bottommargin = defaultmargin;

	// MW-2004-11-26: Initialise mx and my (VG)
	mx = my = 0;

	// MW-2009-06-09: [[ Bitmap Effects ]]
	MCBitmapEffectsInitialize(m_bitmap_effects);

	// MW-2011-09-21: [[ Layers ]] Clear the layer attrs.
	layer_resetattrs();
	// MW-2011-09-21: [[ Layers ]] The layer starts off as static.
	m_layer_mode_hint = kMCLayerModeHintStatic;
}

MCControl::MCControl(const MCControl &cref) : MCObject(cref)
{
	leftmargin = cref.leftmargin;
	rightmargin = cref.rightmargin;
	topmargin = cref.topmargin;
	bottommargin = cref.bottommargin;
	state &= ~CS_KFOCUSED;

	// MW-2006-04-21: [[ Purify ]] Need to initialise mx and my here too
	mx = my = 0;

	// MW-2009-06-09: [[ Bitmap Effects ]] Note that this is not currently robust
	//   as the copy could silently fail.
	MCBitmapEffectsInitialize(m_bitmap_effects);
	MCBitmapEffectsAssign(m_bitmap_effects, cref . m_bitmap_effects);

	// MW-2011-09-21: [[ Layers ]] Clear the layer attrs.
	layer_resetattrs();
	// MW-2011-09-21: [[ Layers ]] The layer takes its layer hint from the source.
	m_layer_mode_hint = cref . m_layer_mode_hint;
}

MCControl::~MCControl()
{
	if (focused == this)
		focused = NULL;
	MCscreen->stopmove(this, False);

	// MW-2009-06-11: [[ Bitmap Effects ]] Destroy the bitmap effects
	MCBitmapEffectsFinalize(m_bitmap_effects);
}

void MCControl::open()
{
	if (opened == 0)
	{
		// MW-2011-10-01: [[ Bug 9777 ]] Special case for when 'open' / 'close' used to
		//   reinitialize object (i.e. MCImage::reopen) - don't update layer attrs if
		//   KEEP_LAYER is set.
		if (!getstate(CS_KEEP_LAYER))
			layer_resetattrs();
		
		state = (state & (CS_NO_MESSAGES | CS_NO_FILE | CS_SELECTED)) | (state & CS_KEEP_LAYER);
	}
	
	MCObject::open();
}

void MCControl::close()
{
	// MW-2008-01-09: [[ Bug 5739 ]] Changing group layer cancels mousedown
	if (opened == 1)
		if (focused == this)
			focused = NULL;
	MCObject::close();
}

void MCControl::kfocus()
{
	if (flags & F_TRAVERSAL_ON)
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();

		state |= CS_KFOCUSED;

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		layer_transientchangedandredrawall(t_old_trans);;

		message(MCM_focus_in);
	}
}

void MCControl::kunfocus()
{
	if (state & CS_KFOCUSED)
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();

		state &= ~CS_KFOCUSED;
		
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		layer_transientchangedandredrawall(t_old_trans);

		message(MCM_focus_out);
	}
}

Boolean MCControl::kdown(MCStringRef p_string, KeySym key)
{
	if (MCObject::kdown(p_string, key))
		return True;
	switch (key)
	{
	case XK_space:
	case XK_Return:
	case XK_KP_Enter:
		message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
		return True;
	default:
		break;
	}
	return False;
}

Boolean MCControl::mfocus(int2 x, int2 y)
{
	// SMR 594 do menu stuff before visibility check
	if (state & CS_MENU_ATTACHED)
		return MCObject::mfocus(x, y);
	if (!(flags & F_VISIBLE || MCshowinvisibles)
	    || (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return False;
	if (state & CS_GRAB)
	{
		MCRectangle newrect = rect;
		newrect.x = x - xoffset;
		newrect.y = y - yoffset;
		mx = x;
		my = y;
		// MW-2011-08-18: [[ Layers ]] Set the rect.
		if (newrect.x != rect.x || newrect.y != rect.y)
			layer_setrect(newrect, false);
		message_with_args(MCM_mouse_move, x, y);
		return True;
	}
	if (state & CS_MOVE)
	{
		mx = x;
		my = y;
		MCselected->continuemove(x, y);
		// MM-2012-09-05: [[ Property Listener ]] Moving object using IDE. Make sure location property is flagged as changed.
		signallisteners(P_LOCATION);
		message_with_args(MCM_mouse_move, x, y);
		return True;
	}
	if (state & CS_SIZE)
	{
		continuesize(x, y);
		// MM-2012-09-05: [[ Property Listener ]] Resizing object using IDE. Make sure rect property is flagged as changed.
		signallisteners(P_RECTANGLE);
		message_with_args(MCM_mouse_move, x, y);
		return True;
	}
	MCRectangle srect;
	MCU_set_rect(srect, x, y, 1, 1);
	Boolean is = maskrect(srect) || (state & CS_SELECTED
	                                 && MCU_point_in_rect(rect, x, y)
	                                 && sizehandles() != 0);
	mx = x;
	my = y;
	if (is || state & CS_MFOCUSED)
	{
		if (focused == this || getstack() -> gettool(this) == T_POINTER)
		{
			if (MCdispatcher -> isdragtarget())
				message_with_args(MCM_drag_move, x, y);
			else
				message_with_args(MCM_mouse_move, x, y);
		}
	}
	
	switch (getstack()->gettool(this))
	{
	case T_BROWSE:
	case T_HELP:
	case T_POINTER:
		if (state & CS_MFOCUSED)
			return True;
		break;
	case T_BUTTON:
	case T_SCROLLBAR:
	case T_PLAYER:
	case T_FIELD:
	case T_IMAGE:
	case T_GRAPHIC:
		if ((is && state & CS_SELECTED) || state & CS_MFOCUSED)
			return True;
		else
			return False;
		break;
	default:
		is = False;
		break;
	}
	return is;
}

void MCControl::munfocus()
{
	if (focused == this)
	{
		if (state & CS_MFOCUSED)
		{
			mfocus(mx, my);
			// IM-2013-08-07: [[ Bug 10671 ]] Release grabbed controls when removing focus
			state &= ~(CS_MFOCUSED | CS_GRAB);
			message(MCM_mouse_release);
		}
		else
			leave();
	}
	mx = rect.x - 1;
	my = rect.y - 1;
}

Boolean MCControl::doubledown(uint2 which)
{
	switch (which)
	{
	case Button1:
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
		case T_BUTTON:
		case T_FIELD:
		case T_SCROLLBAR:
		case T_PLAYER:
		case T_IMAGE:
		case T_GRAPHIC:
		case T_POINTER:
			message_with_valueref_args(MCM_mouse_double_down, MCSTR("1"));
			break;
		default:
			return False;
		}
		break;
	case Button2:
	case Button3:
		message_with_args(MCM_mouse_double_down, which);
		break;
	}
	return True;
}

Boolean MCControl::doubleup(uint2 which)
{
	switch (which)
	{
	case Button1:
		switch (getstack()->gettool(this))
		{
		case T_BUTTON:
		case T_FIELD:
		case T_SCROLLBAR:
		case T_PLAYER:
		case T_IMAGE:
		case T_GRAPHIC:
		case T_POINTER:
			// MW-2010-10-15: [[ Bug 9055 ]] Pass false here to prevent 'mouseUp' being sent.
			end(false, false);
		case T_BROWSE:
			message_with_valueref_args(MCM_mouse_double_up, MCSTR("1"));
			break;
		default:
			return False;
		}
		break;
	case Button2:
	case Button3:
		message_with_args(MCM_mouse_double_up, which);
		break;
	}
	return True;
}

void MCControl::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_idle, kMCCompareCaseless))
	{
		if (opened && getstack()->gettool(this) == T_BROWSE)
		{
			Boolean again = False;
			MCRectangle srect;
			MCU_set_rect(srect, mx, my, 1, 1);
			if (maskrect(srect))
			{
				if (conditionalmessage(HH_MOUSE_WITHIN, MCM_mouse_within) == ES_ERROR)
					senderror();
				else
					again = True;
			}
			if (state & CS_MFOCUSED)
			{
				if (conditionalmessage(HH_MOUSE_STILL_DOWN, MCM_mouse_still_down) == ES_ERROR)
					senderror();
				else
					again = True;
			}
			if (hashandlers & HH_IDLE)
			{
				if (message(MCM_idle) == ES_ERROR)
					senderror();
				else
					again = True;
			}
			if (again)
				MCscreen->addtimer(this, MCM_idle, MCidleRate);
		}
	}
	else
		MCObject::timer(mptr, params);
}

uint2 MCControl::gettransient() const
{
	if (state & CS_KFOCUSED && MClook == LF_MOTIF
	        && !(extraflags & EF_NO_FOCUS_BORDER))
		return MCfocuswidth;
	return 0;
}

#ifdef LEGACY_EXEC
Exec_stat MCControl::getprop_legacy(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective, bool recursive = false)
{
	switch (which)
	{
#ifdef /* MCControl::getprop */ LEGACY_EXEC
	case P_MARGINS:
		if (leftmargin == rightmargin && leftmargin == topmargin && leftmargin == bottommargin)
			ep.setint(leftmargin);
		else
			ep.setrectangle(leftmargin, topmargin, rightmargin, bottommargin);
		break;
	case P_LEFT_MARGIN:
		ep.setint(leftmargin);
		break;
	case P_RIGHT_MARGIN:
		ep.setint(rightmargin);
		break;
	case P_TOP_MARGIN:
		ep.setint(topmargin);
		break;
	case P_BOTTOM_MARGIN:
		ep.setint(bottommargin);
		break;
	// MW-2012-03-013: [[ UnicodeToolTip ]] If normal tool tip is requested convert to
	//   native.
	case P_TOOL_TIP:
		ep.setsvalue(tooltip);
		ep.utf8tonative();
		break;
	// MW-2012-03-013: [[ UnicodeToolTip ]] If unicode tool tip is requested convert to
	//   UTF-16.
	case P_UNICODE_TOOL_TIP:
		ep.setsvalue(tooltip);
		ep.utf8toutf16();
		break;

	// MW-2011-08-25: [[ Layers ]] Handle layerMode property fetch.
	case P_LAYER_MODE:
	{
		// MW-2011-09-21: [[ Layers ]] Updated to use layerMode hint system.
		MCLayerModeHint t_mode;
		if (effective)
			t_mode = layer_geteffectivemode();
		else
			t_mode = m_layer_mode_hint;

		const char *t_value;
		switch(t_mode)
		{
		case kMCLayerModeHintStatic:
			t_value = "static";
			break;
		case kMCLayerModeHintDynamic:
			t_value = "dynamic";
			break;
		case kMCLayerModeHintScrolling:
			t_value = "scrolling";
			break;
		case kMCLayerModeHintContainer:
			t_value = "container";
			break;
		}

		ep.setstaticcstring(t_value);
	}
	break;
#endif /* MCControl::getprop */
	default:
		return MCObject::getprop_legacy(parid, which, ep, effective, recursive);
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCControl::getarrayprop_legacy(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
	switch(which)
	{
#ifdef /* MCControl::getarrayprop */ LEGACY_EXEC
	// MW-2009-06-09: [[ Bitmap Effects ]]
	case P_BITMAP_EFFECT_DROP_SHADOW:
	case P_BITMAP_EFFECT_INNER_SHADOW:
	case P_BITMAP_EFFECT_OUTER_GLOW:
	case P_BITMAP_EFFECT_INNER_GLOW:
	case P_BITMAP_EFFECT_COLOR_OVERLAY:
		return MCBitmapEffectsGetProperties(m_bitmap_effects, which, ep, key);
#endif /* MCControl::getarrayprop */
	default:
		return MCObject::getarrayprop_legacy(parid, which, ep, key, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCControl::setprop_legacy(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = True;
	int2 i1, i2, i3, i4;
	MCString data = ep.getsvalue();

	switch (which)
	{
#ifdef /* MCControl::setprop */ LEGACY_EXEC
	case P_MARGINS:
		if (MCU_stoi2(data, i1))
			leftmargin = rightmargin = topmargin = bottommargin = i1;
		else
		{
			if (!MCU_stoi2x4(data, i1, i2, i3, i4))
			{
				MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
				return ES_ERROR;
			}
			leftmargin = i1;
			topmargin = i2;
			rightmargin = i3;
			bottommargin = i4;
		}
		break;
	case P_LEFT_MARGIN:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
			return ES_ERROR;
		}
		leftmargin = i1;
		break;
	case P_RIGHT_MARGIN:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
			return ES_ERROR;
		}
		rightmargin = i1;
		break;
	case P_TOP_MARGIN:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
			return ES_ERROR;
		}
		topmargin = i1;
		break;
	case P_BOTTOM_MARGIN:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
			return ES_ERROR;
		}
		bottommargin = i1;
		break;
	// MW-2012-03-13: [[ UnicodeToolTip ]] Handle the unicodeToolTip property - same
	//   as tooltip except expects UTF-16 encoded string, rather than native.
	case P_TOOL_TIP:
	case P_UNICODE_TOOL_TIP:
		if (tooltip != MCtooltip->gettip())
			dirty = False;
		delete tooltip;
		
		// MW-2012-03-13: [[ UnicodeToolTip ]] Convert the value to UTF-8 - either
		//   from native or UTF-16 depending on what prop was set.
		if (which == P_TOOL_TIP)
			ep.nativetoutf8();
		else
			ep.utf16toutf8();
		data = ep . getsvalue();
		
		if (data != MCnullmcstring)
			tooltip = data.clone();
		else
			tooltip = NULL;
		if (dirty && focused == this)
		{
			MCtooltip->settip(tooltip);
			dirty = False;
		}
		break;

	// MW-2011-08-25: [[ TileCache ]] Handle layerMode property store.
	case P_LAYER_MODE:
	{
		// MW-2011-09-21: [[ Layers ]] Updated to use new layermode hint system.
		MCLayerModeHint t_mode;
		if (data == "static")
			t_mode = kMCLayerModeHintStatic;
		else if (data == "dynamic")
			t_mode = kMCLayerModeHintDynamic;
		else if (data == "scrolling")
			t_mode = kMCLayerModeHintScrolling;
#if NOT_YET_IMPLEMENTED
		else if (data == "container")
			t_mode = kMCLayerModeHintContainer;
#endif
		else
		{
			MCeerror -> add(EE_CONTROL_BADLAYERMODE, 0, 0, data);
			return ES_ERROR;
		}
		
		// If the layer mode hint has changed, update and mark the attrs
		// for recompute. If the hint hasn't changed, then there's no need
		// to redraw.
		if (t_mode != m_layer_mode_hint)
		{
			m_layer_attr_changed = true;
			m_layer_mode_hint = t_mode;
		}
		else
			dirty = False;
	}
	break;

	// MW-2011-09-21: [[ Layers ]] There are numerous properties which
	//   can affect the layer attributes, thus in those cases we pass back
	//   the handling and then mark the attrs for a recompute.

	// The 'ink' property affects whether a layer can be static.
	case P_INK:
	// The border, focus border and shadow properties can affect whether the layer
	// is direct.
	case P_SHOW_BORDER:
	case P_SHOW_FOCUS_BORDER:
	case P_SHADOW:
	// The opaque property can affect the layer's opaqueness.
	case P_FILLED:
	case P_OPAQUE:
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;

		// Mark the layer attrs for recompute.
		m_layer_attr_changed = true;
		return ES_NORMAL;
#endif /* MCControl::setprop */
	default:
		return MCObject::setprop_legacy(parid, which, ep, effective);
	}

	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
}

Exec_stat MCControl::setarrayprop_legacy(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
	Boolean dirty;
	dirty = False;
	switch(which)
	{
#ifdef /* MCControl::setarrayprop */ LEGACY_EXEC
	case P_BITMAP_EFFECT_DROP_SHADOW:
	case P_BITMAP_EFFECT_INNER_SHADOW:
	case P_BITMAP_EFFECT_OUTER_GLOW:
	case P_BITMAP_EFFECT_INNER_GLOW:
	case P_BITMAP_EFFECT_COLOR_OVERLAY:
	{	
		MCRectangle t_old_effective_rect = geteffectiverect();
		if (MCBitmapEffectsSetProperties(m_bitmap_effects, which, ep, key, dirty) != ES_NORMAL)
			return ES_ERROR;

		if (dirty && opened)
		{
			// MW-2011-09-21: [[ Layers ]] Mark the attrs as needing redrawn.
			m_layer_attr_changed = true;

			// MW-2011-08-17: [[ Layers ]] Make sure any redraw needed due to effects
			//   changing occur.
			layer_effectschanged(t_old_effective_rect);
		}
	}
	return ES_NORMAL;
#endif /* MCControl::setarrayprop */
	default:
		break;
	}
	return MCObject::setarrayprop_legacy(parid, which, ep, key, effective);
}
#endif

void MCControl::select()
{
	state |= CS_SELECTED;
	kunfocus();

	// MW-2011-09-23: [[ Layers ]] Mark the layer attrs as having changed - the selection
	//   setting can influence the layer type.
	m_layer_attr_changed = true;

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCControl::deselect()
{
	if (state & CS_SELECTED)
	{
		state &= ~(CS_SELECTED | CS_MOVE | CS_SIZE | CS_CREATE);

		// MW-2011-09-23: [[ Layers ]] Mark the layer attrs as having changed - the selection
		//   setting can influence the layer type.
		m_layer_attr_changed = true;

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

Boolean MCControl::del()
{
	if (parent == NULL || scriptdepth != 0 || getstack()->islocked())
	{
		MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0);
		return False;
	}
	switch (gettype())
	{
	case CT_BUTTON:
		message(MCM_delete_button);
		break;
	case CT_EPS:
		message(MCM_delete_eps);
		break;
	case CT_FIELD:
		message(MCM_delete_field);
		break;
	case CT_IMAGE:
		message(MCM_delete_image);
		break;
	case CT_SCROLLBAR:
		message(MCM_delete_scrollbar);
		break;
	case CT_PLAYER:
		message(MCM_delete_player);
		break;
	case CT_GRAPHIC:
		message(MCM_delete_graphic);
		break;
	case CT_GROUP:
		message(MCM_delete_group);
		message(MCM_delete_background);
		break;
    case CT_WIDGET:
        message(MCM_delete_widget);
        break;
	default:
		break;
	}
	uint2 num = 0;
	getcard()->count(CT_LAYER, CT_UNDEFINED, this, num, True);
	switch (parent->gettype())
	{
	case CT_CARD:
		{
			MCCard *cptr = (MCCard *)parent;
			if (!cptr->removecontrol(this, False, True))
				return False;
			getstack()->removecontrol(this);
			break;
		}
	case CT_GROUP:
		{
			MCGroup *gptr = (MCGroup *)parent;
			gptr->removecontrol(this, True);
			break;
		}
	default:
		{ //stack
			MCStack *sptr = (MCStack *)parent;
			sptr->removecontrol(this);
		}
	}

	// MW-2008-10-28: [[ ParentScripts ]] If the object is marked as being used
	//   as a parentScript, flush the parentScript table so we don't get any
	//   dangling pointers.
	if (getstate(CS_IS_PARENTSCRIPT) && gettype() == CT_BUTTON)
	{
		MCParentScript::FlushObject(this);
		setstate(False, CS_IS_PARENTSCRIPT);
	}

    // IM-2012-05-16 [[ BZ 10212 ]] deleting the dragtarget control in response
    // to a 'dragdrop' message would leave these globals pointing to the deleted
    // object, leading to an infinite loop if the target was a field
    if (MCdragdest == this)
    {
        MCdragdest = nil;
        MCdropfield = nil;
    }
    
    if (MCdragsource == this)
        MCdragsource = nil;
    
	return True;
}

void MCControl::paste(void)
{
	if (MCdefaultstackptr->getmode() != WM_TOP_LEVEL)
		return;

	parent = MCdefaultstackptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
	MCCard *cptr = (MCCard *)parent;
	obj_id = 0;
	//newcontrol->resetfontindex(oldstack);
	if (!MCU_point_in_rect(cptr->getrect(), rect.x + (rect.width >> 1),
	                       rect.y + (rect.height >> 1)))
		setrect(MCU_center_rect(cptr->getrect(), rect));
	attach(OP_NONE, false);
	if (getstack()->gettool(this) == T_POINTER && opened)
	{
		state |= CS_SELECTED;
		MCselected->clear(False);
		MCselected->add(this);
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

void MCControl::undo(Ustruct *us)
{
	MCRectangle newrect = rect;
	switch (us->type)
	{
	case UT_MOVE:
		if (!(flags & F_LOCK_LOCATION))
		{
			newrect.x -= us->ud.deltas.x;
			newrect.y -= us->ud.deltas.y;
			us->ud.deltas.x = -us->ud.deltas.x;
			us->ud.deltas.y = -us->ud.deltas.y;
		}
		break;
	case UT_SIZE:
		newrect = us->ud.rect;
		us->ud.rect = rect;
		break;
	case UT_DELETE:
		{
			MCCard *card = (MCCard *)parent->getcard();
			getstack()->appendcontrol(this);
			card->newcontrol(this, False);
			Boolean oldrlg = MCrelayergrouped;
			MCrelayergrouped = True;
			card->relayer(this, us->ud.layer);
			MCrelayergrouped = oldrlg;
			Boolean oldlock = MClockmessages;
			MClockmessages = True;
			MCselected->add(this);
			MClockmessages = oldlock;
			MCscreen->delaymessage(this, MCM_selected_object_changed);
			us->type = UT_REPLACE;
		}
		return;
	case UT_REPLACE:
		del();
		us->type = UT_DELETE;
		return;
	default:
		break;
	}
	
	// MW-2011-08-18: [[ Layers ]] Set the rect.
	layer_setrect(newrect, false);
	
	// MW-2011-10-03: [[ Scrolling ]] Make sure the parent is notified.
	resizeparent();
}

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCControl::draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite)
{
	fprintf(stderr, "Control: ERROR tried to draw control id %d\n", obj_id);
}

IO_stat MCControl::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	return MCObject::save(stream, p_part, p_force_ext);
}

Boolean MCControl::kfocusset(MCControl *target)
{
	if (target == this)
		return True;
	return False;
}

MCControl *MCControl::clone(Boolean attach, Object_pos p, bool invisible)
{
	fprintf(stderr, "Control: ERROR tried to clone control id %d\n", obj_id);
	return NULL;
}

MCControl *MCControl::findnum(Chunk_term type, uint2 &num)
{
	if ((type == gettype() || type == CT_LAYER) && num-- == 0)
		return this;
	else
		return NULL;
}

MCControl *MCControl::findname(Chunk_term type, MCNameRef p_name)
{
	if ((type == gettype() || type == CT_LAYER)
	        && MCU_matchname(p_name, gettype(), getname()))
		return this;
	else
		return NULL;
}

MCControl *MCControl::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if ((type == gettype() || type == CT_LAYER)
	    && (inid == obj_id || (alt && inid == altid)))
		return this;
	else
		return NULL;
}

Boolean MCControl::count(Chunk_term type, MCObject *stop, uint2 &num)
{
	if (type == gettype() || type == CT_LAYER)
		num++;
	if (stop == this)
		return True;
	return False;
}

Boolean MCControl::maskrect(const MCRectangle &srect)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles))
		return False;
	MCRectangle drect = MCU_intersect_rect(srect, rect);
	return drect.width != 0 && drect.height != 0;
}

void MCControl::installaccels(MCStack *stack)
{ }

void MCControl::removeaccels(MCStack *stack)
{ }

MCCdata *MCControl::getdata(uint4 cardid, Boolean clone)
{
	return NULL;
}

void MCControl::replacedata(MCCdata *&data, uint4 newid)
{ }

void MCControl::compactdata()
{ }

void MCControl::resetfontindex(MCStack *oldstack)
{
}

Exec_stat MCControl::hscroll(int4 offset, Boolean doredraw)
{
	return ES_NORMAL;
}
Exec_stat MCControl::vscroll(int4 offset, Boolean doredraw)
{
	return ES_NORMAL;
}
void MCControl::readscrollbars()
{ }
void MCControl::setsbrects()
{ }
void MCControl::resetscrollbars(Boolean move)
{ }
void MCControl::fliph()
{ }
void MCControl::flipv()
{ }

void MCControl::attach(Object_pos p, bool invisible)
{
	int2 v1 = rect.x;
	int2 v2 = rect.y;
	if (p == OP_CENTER && v1 == -1 && v2 == -1)
		positionrel(getcard()->getrect(), OP_CENTER, OP_MIDDLE);
	else
		if (p == OP_OFFSET)
		{
			MCRectangle newrect = rect;
			newrect.x += MCcloneoffset;
			newrect.y += MCcloneoffset;
			setrect(newrect);
		}
	uint4 cid = parent->gettype() == CT_CARD ? parent->getid() : 0;

	if (obj_id == 0)
	{
		obj_id = getstack()->newid();
		
		// MW-2011-08-09: If we are attaching a group, make sure the object
		//   id's are non-zero.
		if (gettype() == CT_GROUP)
			static_cast<MCGroup *>(this) -> ensureids();
	}

	if (invisible)
		setflag(False, F_VISIBLE);

	if (parent == NULL || parent->gettype() == CT_CARD
	        || parent->gettype() == CT_STACK)
	{
		MCCard *card = getcard(cid);
		getstack()->appendcontrol(this);
		card->newcontrol(this, True);
	}
	else
	{
		MCGroup *gptr = (MCGroup *)parent;
		gptr->appendcontrol(this);
	}
	newmessage();
}

inline MCRectangle MCGRectangleGetPixelRect(const MCGRectangle &p_rect)
{
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = floorf(p_rect.origin.x);
	t_top = floorf(p_rect.origin.y);
	t_right = floorf(p_rect.origin.x + p_rect.size.width);
	t_bottom = floorf(p_rect.origin.y + p_rect.size.height);
	
	return MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
}

void MCControl::redraw(MCDC *dc, const MCRectangle &dirty)
{
    // SN-2014-11-14: [[ Bug 14028 ]] Use the current control visibility state
	if (!opened || !(getflag(F_VISIBLE) || MCshowinvisibles))
		return;

	// MW-2009-06-11: [[ Bitmap Effects ]] A control needs to be (partially)
	//   redrawn if the dirty region touches it or one of its effects.
	MCRectangle trect = MCU_intersect_rect(dirty, geteffectiverect());
	if (trect.width != 0 && trect.height != 0)
	{
		dc->save();
		
		dc -> setopacity(255);
		dc -> setfunction(GXcopy);
		dc->cliprect(trect);
        
		// MW-2011-09-06: [[ Redraw ] Make sure we draw the control normally (not
		//   as a sprite).
		draw(dc, trect, false, false);
		
		dc->restore();
	}
}

void MCControl::sizerects(MCRectangle *rects)
{
	int2 x[3];
	int2 y[3];

	uint2 handlesize = MCsizewidth;
	if (handlesize > MCU_min(rect.width, rect.height) / 3)
		handlesize = MCU_max(MCU_min(rect.width, rect.height) / 3, 3);

	x[0] = rect.x;
	x[1] = rect.x + ((rect.width - handlesize) >> 1);
	x[2] = rect.x + rect.width - handlesize;
	y[0] = rect.y;
	y[1] = rect.y + ((rect.height - handlesize) >> 1);
	y[2] = rect.y + rect.height - handlesize;

	uint2 i;
	uint2 j;
	uint2 k = 0;
	for (i = 0 ; i < 3 ; i++)
		for (j = 0 ; j < 3 ; j++)
			if (i != 1 || j != 1)
			{
				rects[k].width = rects[k].height = handlesize;
				rects[k].x = x[j];
				rects[k].y = y[i];
				k++;
			}
}

void MCControl::drawselected(MCDC *dc)
{
	if (MCdragging)
		return;

	dc -> setopacity(255);
	dc -> setfunction(GXcopy);

	MCRectangle rects[8];
	sizerects(rects);
	if (flags & F_LOCK_LOCATION)
		dc->setfillstyle(FillStippled, nil, 0, 0);
	else
		dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->setforeground(MCselectioncolor);
	dc->fillrects(rects, 8);
	if (flags & F_LOCK_LOCATION)
		dc->setfillstyle(FillSolid, nil, 0, 0);
}

void MCControl::drawarrow(MCDC *dc, int2 x, int2 y, uint2 size,
                          Arrow_direction dir, Boolean border, Boolean hilite)
{
	MCRectangle trect;
	MCU_set_rect(trect, x, y, size, size);
	MCPoint ap[5];
	int2 ho = hilite && MClook == LF_WIN95 ? 0 : -1;
	if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_ARROW))
	{
		//fill winfo structure for arrow
		return;//exit
	}
	switch (MClook)
	{
	case LF_MOTIF:
		if (dir == AD_UP || dir == AD_DOWN)
		{
			ap[0].x = x + (size >> 1);
			if (dir == AD_UP)
			{
				ap[1].x = x + size - 1;
				ap[2].x = x + 1;
				ap[0].y = y + 1;
				ap[1].y = ap[2].y = y + size - 2;
			}
			else
			{
				ap[1].x = x + 1;
				ap[2].x = x + size - 1;
				ap[0].y = y + size - 1;
				ap[1].y = ap[2].y = y + 1;
			}
		}
		else
		{
			ap[0].y = y + (size >> 1);
			if (dir == AD_LEFT)
			{
				ap[0].x = x;
				ap[1].x = ap[2].x = x + size - 2;
				ap[1].y = y + size - 1;
				ap[2].y = y + 1;
			}
			else
			{
				ap[0].x = x + size - 1;
				ap[1].x = ap[2].x = x + 1;
				ap[1].y = y + 1;
				ap[2].y = y + size - 1;
			}
		}
		setforeground(dc, DI_BACK, False);
		dc->fillpolygon(ap, 3);
		uint1 i;
		for (i = 0 ; i < 2 ; i++)
		{
			setforeground(dc, flags & F_3D ? DI_BOTTOM : DI_BORDER, hilite);
			if (dir == AD_UP || dir == AD_LEFT)
				dc->drawlines(ap, 3);
			else
				dc->drawline(ap[0].x, ap[0].y, ap[2].x, ap[2].y);
			setforeground(dc, flags & F_3D ? DI_TOP : DI_BORDER, hilite);
			if (dir == AD_UP || dir == AD_LEFT)
				dc->drawline(ap[0].x, ap[0].y, ap[2].x, ap[2].y);
			else
				dc->drawlines(ap, 3);
			switch (dir)
			{
			case AD_UP:
				ap[0].y++;
				ap[1].x--;
				ap[1].y--;
				ap[2].x++;
				ap[2].y--;
				break;
			case AD_DOWN:
				ap[0].y--;
				ap[1].x++;
				ap[1].y++;
				ap[2].x--;
				ap[2].y++;
				break;
			case AD_LEFT:
				ap[0].x++;
				ap[1].y--;
				ap[1].x--;
				ap[2].y++;
				ap[2].x--;
				break;
			case AD_RIGHT:
				ap[0].x--;
				ap[1].y++;
				ap[1].x++;
				ap[2].y--;
				ap[2].x++;
				break;
			}
		}
	default:
		{
			if (border)
			{
				if (flags & F_OPAQUE && hilite && IsMacLF())
					setforeground(dc, DI_HILITE, False);
				else
					if (gettype() == CT_SCROLLBAR && parent->gettype() == CT_FIELD)
						parent->getparent()->setforeground(dc, DI_BACK, False);
					else
						setforeground(dc, DI_BACK, False);
				dc->fillrect(trect);
			}
			if (IsMacLF())
				size += 2;
			uint2 abase = (size >> 1) - 2;
			if (IsMacLF())
				abase++;
			uint2 aheight = abase >> 1;
			uint2 abp = (size - abase) >> 1;
			uint2 ahp = (size - aheight) >> 1;
			if (dir == AD_UP || dir == AD_DOWN)
			{
				ap[0].x = x + abp + ho;
				ap[1].x = ap[0].x + abase;
				ap[2].x = ap[0].x + aheight;
				if (dir == AD_UP)
				{
					ap[2].y = y + ahp + ho;
					ap[0].y = ap[1].y = ap[2].y + aheight;
				}
				else
				{
					ap[0].y = ap[1].y = y + ahp + ho;
					ap[2].y = ap[0].y + aheight;
				}
				ap[3] = ap[2];
				if (abase & 1)
					ap[2].x++;
				ap[4] = ap[0];
			}
			else
			{
				ap[0].y = y + abp + ho;
				ap[1].y = ap[0].y + abase;
				ap[2].y = ap[0].y + aheight;
				if (dir == AD_RIGHT)
				{
					ap[0].x = ap[1].x = x + ahp + ho;
					ap[2].x = ap[0].x + aheight;
				}
				else
				{
					ap[2].x = x + ahp + ho;
					ap[0].x = ap[1].x = ap[2].x + aheight;
				}
				ap[3] = ap[2];
				if (abase & 1)
					ap[2].y++;
				ap[4] = ap[0];
			}
			if (flags & F_OPAQUE)
				if (hilite && !border && IsMacLF())
					setforeground(dc, DI_BACK, False, True);
				else
					if (flags & F_DISABLED && MClook != LF_MOTIF)
						dc->setforeground(dc->getgray());
					else
						dc->setforeground(dc->getblack());
			else
				setforeground(dc, DI_FORE, False);
			dc->fillpolygon(ap, 5);
			dc->drawlines(ap, 5);
			if (border)
			{
				if (IsMacLF())
				{
					size -= 2;
					MCU_set_rect(trect, x, y, size, size);
					draw3d(dc, trect, hilite ? ETCH_SUNKEN : ETCH_RAISED, 1);
				}
				else
				{
					MCU_set_rect(trect, x, y, size, size);
					if (hilite)
						drawborder(dc, trect, 1);
					else
						draw3d(dc, trect, ETCH_RAISED_SMALL, DEFAULT_BORDER);
				}
			}
		}
		break;
	}
}

void MCControl::continuesize(int2 x, int2 y)
{
	MCRectangle newrect = rect;
	if (state & CS_SIZEL)
	{
		int2 rx = newrect.x + newrect.width;
		newrect.x = x - xoffset;
		MCU_snap(newrect.x);
		if (newrect.x > rx - MCminsize)
			newrect.x = rx - MCminsize;
		newrect.width = rx - newrect.x;
	}
	else
		if (state & CS_SIZER)
		{
			int2 rx = x + xoffset;
			MCU_snap(rx);
			if (rx - newrect.x < MCminsize)
				newrect.width = MCminsize;
			else
				newrect.width = rx - newrect.x;
		}
	if (state & CS_SIZET)
	{
		int2 ly = newrect.y + newrect.height;
		newrect.y = y - yoffset;
		MCU_snap(newrect.y);
		if (newrect.y > ly - MCminsize)
			newrect.y = ly - MCminsize;
		newrect.height = ly - newrect.y;
	}
	else
		if (state & CS_SIZEB)
		{
			int2 ly = y + yoffset;
			MCU_snap(ly);
			if (ly - newrect.y < MCminsize)
				newrect.height = MCminsize;
			else
				newrect.height = ly - newrect.y;
		}
	if (MCmodifierstate & MS_SHIFT)
	{
		double newaspect;
		newaspect = newrect.height / (double)newrect.width;

		if (aspect < newaspect)
		{
			int2 newheight;
			newheight = (int2)(newrect.width * aspect);
			if (state & CS_SIZET)
				newrect.y += newrect.height - newheight;
			newrect.height = newheight;
		}
		else
		{
			int2 newwidth;
			newwidth = (int2)(newrect.height / aspect);
			if (state & CS_SIZEL)
				newrect.x += newrect.width - newwidth;
			newrect.width = newwidth;
		}
	}
	else if (MCmodifierstate & MS_MOD1)
	{
		if (newrect.height > newrect.width)
		{
			if (state & CS_SIZET)
				newrect.y += newrect.height - newrect.width;
			newrect.height = newrect.width;
		}
		else
		{
			if (state & CS_SIZEL)
				newrect.x += newrect.width - newrect.height;
			newrect.width = newrect.height;
		}
	}
	mx = x;
	my = y;
	if (mx < newrect.x && state & CS_SIZER)
	{
		state &= ~CS_SIZER;
		newrect.x -= MCminsize;
		state |= CS_SIZEL;
		fliph();
	}
	else
		if (mx > newrect.x + newrect.width && state & CS_SIZEL)
		{
			state &= ~CS_SIZEL;
			newrect.x += MCminsize;
			state |= CS_SIZER;
			fliph();
		}
	if (my < newrect.y && state & CS_SIZEB)
	{
		state &= ~CS_SIZEB;
		newrect.y -= MCminsize;
		state |= CS_SIZET;
		flipv();
	}
	else
		if (my > newrect.y + newrect.height && state & CS_SIZET)
		{
			state &= ~CS_SIZET;
			newrect.y += MCminsize;
			state |= CS_SIZEB;
			flipv();
		}
	if (MCcentered)
	{
		if (rect.x == newrect.x)
			newrect.x += rect.width - newrect.width;
		newrect.width += rect.x - newrect.x;
		if (rect.y == newrect.y)
			newrect.y += rect.height - newrect.height;
		newrect.height += rect.y - newrect.y;
	}

	// MW-2011-08-18: [[ Layers ]] Set the rect.
	layer_setrect(newrect, true);
	
	// MW-2011-10-03: [[ Scrolling ]] Make sure the parent is notified.
	resizeparent();
}

uint2 MCControl::sizehandles()
{
	uint2 newstate = 0;
	if (!(flags & F_LOCK_LOCATION))
	{
		MCRectangle rects[8];
		sizerects(rects);
		int2 i;
		for (i = 7 ; i >= 0 ; i--)
			if (MCU_point_in_rect(rects[i], mx, my))
			{
				if (i < 3)
				{
					newstate |= CS_SIZET;
					yoffset = my - rect.y;
				}
				else
					if (i > 4)
					{
						newstate |= CS_SIZEB;
						yoffset = rect.y + rect.height - my;
					}
				if (i == 0 || i == 3 || i == 5)
				{
					newstate |= CS_SIZEL;
					xoffset = mx - rect.x;
				}
				else
					if (i == 2 || i == 4 || i == 7)
					{
						newstate |= CS_SIZER;
						xoffset = rect.x + rect.width - mx;
					}
				break;
			}
	}
	return newstate;
}

void MCControl::start(Boolean canclone)
{
	if (message_with_valueref_args(MCM_mouse_down, MCSTR("1")) == ES_NORMAL && !MCexitall)
		return;
	MCexitall = False;
	getstack()->kfocusset(NULL);
	kunfocus();
	
	state |= sizehandles();
	if (!(state & CS_SELECTED))
	{
		if (MCmodifierstate & MS_SHIFT)
			MCselected->add(this);
		else
			MCselected->replace(this);
	}
	else
	{
		if (MCmodifierstate & MS_SHIFT && sizehandles() == 0)
		{
			MCselected->remove(this);
			return;
		}
		else
		{
			MCselected->top(this);
		}
	}
	if (MCbuttonstate == 0)
	{
		state &= ~(CS_MOVE | CS_SIZE | CS_CREATE);
	}
	else
	{
		if (canclone)
		{
			if (!(state & CS_SIZE))
			{
				state |= CS_MOVE;
				Boolean hascontrol = IsMacLF()
				                     ? (MCmodifierstate & MS_MOD1) != 0
				                     : (MCmodifierstate & MS_CONTROL) != 0;
				Boolean noshift = IsMacLF()
				                  ? !(MCmodifierstate & (MS_SHIFT | MS_CONTROL))
				                  : !(MCmodifierstate & (MS_SHIFT | MS_MOD1));
				MCselected->startmove(mx, my, canclone && hascontrol
				                      && noshift && !(state & CS_SIZE));
			}
			else
			{
				Ustruct *us = new Ustruct;
				us->type = UT_SIZE;
				us->ud.rect = rect;
				MCundos->freestate();
				MCundos->savestate(this, us);
				aspect = rect . height / (double)rect . width;
			}
		}
	}
	
	// MM-2012-11-06: [[ Property Listener ]]
	if (state & CS_SIZE)
		signallistenerswithmessage(kMCPropertyChangedMessageTypeResizeControlStarted);
}

// MW-2010-10-15: [[ Bug 9055 ]] 'end' is invoked in cases where we don't want to send
//   mouseUp, the new parameter controls this (default 'true').
void MCControl::end(bool p_send_mouse_up, bool p_release)
{
	uint4 oldstate = state;
	state &= ~(CS_MOVE | CS_SIZE | CS_CREATE);
	if (oldstate & CS_MOVE)
	{
		if (MCselected->endmove())
			message(MCM_move_control);
	}
	else if (oldstate & CS_CREATE)
		newmessage();
	else if (oldstate & CS_SIZE)
		message(MCM_resize_control);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
	
	if (p_send_mouse_up)
    {
		if (p_release)
            message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
        else
            message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
    }
	
	// MM-2012-11-06: [[ Property Listener ]]
	if (oldstate & CS_SIZE)
		signallistenerswithmessage(kMCPropertyChangedMessageTypeResizeControlEnded);
}

void MCControl::create(int2 x, int2 y)
{
	MCU_snap(x);
	MCU_snap(y);
	MCRectangle trect;
	trect.x = x;
	trect.y = y;
	trect.width = trect.height = MCminsize;
	setrect(trect);
	mx = x + MCminsize - 1;
	my = y + MCminsize - 1;
	state |= CS_MFOCUSED | CS_CREATE;
	start(False);
	mfocus(x, y);
}

Boolean MCControl::moveable()
{
	return !(flags & F_LOCK_LOCATION);
}

void MCControl::newmessage()
{
	char *messptr = new char[strlen(gettypestring()) + 4];
	strcpy(messptr, "new");
	strcpy(&messptr[3], gettypestring());

	MCAutoNameRef t_message;
	/* UNCHECKED */ t_message . CreateWithCString(messptr);
	message(t_message);

	delete messptr;
	if (gettype() == CT_GROUP)
		message(MCM_new_background);
}

void MCControl::enter()
{
	if (focused != NULL && focused != this)
		leave();
	if (MCdispatcher -> isdragtarget())
	{
		MCdragaction = DRAG_ACTION_NONE;
		MCdragdest = this;
		if (message(MCM_drag_enter) != ES_NORMAL && gettype() == CT_FIELD
		        && !(flags & F_LOCK_TEXT) && MCdragdata -> HasText())
		{
			state |= CS_DRAG_TEXT;
			state &= ~CS_MFOCUSED;
			MCdragaction = DRAG_ACTION_COPY;
			MCField *fptr = (MCField *)this;
			fptr->dragtext();
			MCscreen->addtimer(this, MCM_internal2, MCsyncrate);
		}
	}
	else
		message(MCM_mouse_enter);
    // AL-2013-01-14: [[ Bug 11343 ]] Add timer if the object handles mouseWithin in the behavior chain.
	if (handlesmessage(MCM_mouse_within) && !(hashandlers & HH_IDLE))
		MCscreen->addtimer(this, MCM_idle, MCidleRate);
	if (getstack()->gettool(this) == T_BROWSE)
		MCtooltip->settip(tooltip);
	focused = this;
}

void MCControl::leave()
{
	MCControl *oldfocused = focused;
	if (MCdispatcher -> isdragtarget())
	{
		// MW-2013-08-08: [[ Bug 10655 ]] If oldfocused is a field and has dragText set,
		//   then make sure we unset it (otherwise the caret will continue moving around
		//   on mouseMove).
		if (oldfocused->gettype() == CT_FIELD
		        && oldfocused -> getstate(CS_DRAG_TEXT))
		{
			MCField *fptr = (MCField *)oldfocused;
			fptr->removecursor();
			getstack()->clearibeam();
			oldfocused->state &= ~(CS_DRAG_TEXT | CS_IBEAM);
		}
		oldfocused->message(MCM_drag_leave);
		MCdragaction = DRAG_ACTION_NONE;
		MCdragdest = NULL;
	}
	else
		oldfocused->message(MCM_mouse_leave);
	focused = NULL;
}

Boolean MCControl::sbfocus(int2 x, int2 y, MCScrollbar *hsb, MCScrollbar *vsb)
{
	if (state & (CS_HSCROLL | CS_VSCROLL))
	{
		if (state & CS_HSCROLL)
			hsb->mfocus(x, y);
		else
			vsb->mfocus(x, y);
		readscrollbars();
		MCscreen->sync(getw());
		mx = x;
		my = y;
		message_with_args(MCM_mouse_move, x, y);
		return True;
	}
	else
		if (!(state & CS_SELECTED) &&
		    ((flags & F_HSCROLLBAR && MCU_point_in_rect(hsb->getrect(), x, y)) ||
		     (flags & F_VSCROLLBAR && MCU_point_in_rect(vsb->getrect(), x, y))) &&
		    (gettype() == CT_GROUP || getstack()->gettool(this) == T_BROWSE))
		{
			if(MCcurtheme && MCcurtheme->getthemepropbool(WTHEME_PROP_SUPPORTHOVERING))
			{
				if (flags & F_HSCROLLBAR && MCU_point_in_rect(hsb->getrect(), x, y))
					hsb->mfocus(x, y);
				else if (flags & F_VSCROLLBAR && MCU_point_in_rect(vsb->getrect(), x, y))
					vsb->mfocus(x, y);
			}
			mx = x;
			my = y;
			
			bool t_drag_move;
			t_drag_move = MCdispatcher -> isdragtarget();
			
			MCObject *t_target;
			if (gettype() == CT_GROUP)
				t_target = getcard();
			else
				t_target = this;

			t_target -> message_with_args(t_drag_move ? MCM_drag_move : MCM_mouse_move, x, y);

			return True;
		}
		else
		{
			if (hsb != NULL)
				hsb -> munfocus();
			if (vsb != NULL)
				vsb -> munfocus();
		}
	return False;
}

Boolean MCControl::sbdown(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb)
{
	if (flags & F_HSCROLLBAR && MCU_point_in_rect(hsb->getrect(), mx, my))
	{
		state |= CS_HSCROLL;
		hsb->mfocus(mx, my);
		hsb->mdown(which);
		readscrollbars();
		return True;
	}
	if (flags & F_VSCROLLBAR && MCU_point_in_rect(vsb->getrect(), mx, my))
	{
		state |= CS_VSCROLL;
		vsb->mfocus(mx, my);
		vsb->mdown(which);
		readscrollbars();
		return True;
	}
	return False;
}

// MW-2007-08-30: [[ Bug 3118 ]] This can be called when hsb or vsb are NULL causing
//   a crash.
Boolean MCControl::sbup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb)
{
	if (state & CS_HSCROLL)
	{
		state &= ~CS_HSCROLL;
		if (hsb != NULL)
			hsb->mup(which, false);
		readscrollbars();
		return True;
	}
	if (state & CS_VSCROLL)
	{
		state &= ~CS_VSCROLL;
		if (vsb != NULL)
			vsb->mup(which, false);
		readscrollbars();
		return True;
	}
	return False;
}

Boolean MCControl::sbdoubledown(uint2 which, MCScrollbar *hsb,
                                MCScrollbar *vsb)
{
	if (flags & F_HSCROLLBAR
	        && MCU_point_in_rect(hsb->getrect(), mx, my))
	{
		state |= CS_HSCROLL;
		hsb->doubledown(which);
		readscrollbars();
		return True;
	}
	if (flags & F_VSCROLLBAR
	        && MCU_point_in_rect(vsb->getrect(), mx, my))
	{
		state |= CS_VSCROLL;
		vsb->doubledown(which);
		readscrollbars();
		return True;
	}
	return False;
}

Boolean MCControl::sbdoubleup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb)
{
	if (state & CS_HSCROLL)
	{
		state &= ~CS_HSCROLL;
		if (hsb != NULL)
			hsb->doubleup(which);
		readscrollbars();
		return True;
	}
	if (state & CS_VSCROLL)
	{
		state &= ~CS_VSCROLL;
		if (vsb != NULL)
			vsb->doubleup(which);
		readscrollbars();
		return True;
	}
	return False;
}

Exec_stat MCControl::setsbprop(Properties which, bool p_enable,
                               int4 tx, int4 ty, uint2 &sbw,
                               MCScrollbar *&hsb, MCScrollbar *&vsb,
                               Boolean &dirty)
{
	dirty = False;
	Exec_stat stat = ES_NORMAL;
	switch (which)
	{
	case P_HSCROLLBAR:
		dirty = getflag(F_HSCROLLBAR) != p_enable;
		if (p_enable)
			flags |= F_HSCROLLBAR;
		else
			flags &= ~F_HSCROLLBAR;
	
		if (dirty)
		{
			if (flags & F_HSCROLLBAR)
			{
				hsb = new MCScrollbar(*MCtemplatescrollbar);
				hsb->setparent(this);
				hsb->setflag(False, F_TRAVERSAL_ON);
				hsb->setflag(flags & F_3D, F_3D);
				hsb->setflag(flags & F_DISABLED, F_DISABLED);
				if (opened)
				{
					// MW-2010-12-10: [[ Out-of-bound Scroll ]] Make sure we reset the scroll to be in
					//   bounds when we display the scrollbar.
					hscroll(0, True);
					
					hsb->open();
					MCRectangle trect = hsb->getrect();
					sbw = trect.height;
					trect.width = trect.height + 1; // make sure SB is horizontal
					hsb->setrect(trect);
					setsbrects();
				}
				hsb->allowmessages(False);
			}
			else
			{
				delete hsb;
				hsb = NULL;
				if (opened)
					setsbrects();
			}

			// MW-2011-09-21: [[ Layers ]] Changing the property affects the
			//   object's adorned status.
			m_layer_attr_changed = true;
		}
		break;
	case P_VSCROLLBAR:
		dirty = getflag(F_VSCROLLBAR) != p_enable;
		if (p_enable)
			flags |= F_VSCROLLBAR;
		else
			flags &= ~F_VSCROLLBAR;
		
		if (dirty)
		{
			if (flags & F_VSCROLLBAR)
			{
				vsb = new MCScrollbar(*MCtemplatescrollbar);
				vsb->setparent(this);
				vsb->setflag(False, F_TRAVERSAL_ON);
				vsb->setflag(flags & F_3D, F_3D);
				vsb->setflag(flags & F_DISABLED, F_DISABLED);
				if (opened)
				{
					// MW-2010-12-10: [[ Out-of-bound Scroll ]] Make sure we reset the scroll to be in
					//   bounds when we display the scrollbar.
					vscroll(0, True);
					
					vsb->open();
					sbw = vsb->getrect().width;
					setsbrects();
				}
				vsb->allowmessages(False);
			}
			else
			{
				delete vsb;
				vsb = NULL;
				if (opened)
					setsbrects();
			}

			// MW-2011-09-21: [[ Layers ]] Changing the property affects the
			//   object's adorned status.
			m_layer_attr_changed = true;
		}
		break;
			
	default:
		break;
	}
	return stat;
}

void MCControl::grab()
{
	if (state & CS_MFOCUSED)
	{
		state |= CS_GRAB;
		xoffset = mx - rect.x;
		yoffset = my - rect.y;
	}
}

void MCControl::ungrab(uint2 which)
{
	state &= ~CS_GRAB;
	
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();

	message_with_args(MCM_mouse_up, which);
}

void MCControl::drawfocus(MCDC *dc, const MCRectangle &dirty)
{
	if (MClook == LF_WIN95 || extraflags & EF_NO_FOCUS_BORDER)
		return;
	MCRectangle trect;
	trect = MCU_reduce_rect(rect, -MCfocuswidth);
	if (MCcurtheme == NULL ||
		!MCcurtheme -> drawfocusborder(dc, dirty, trect))
	{
		setforeground(dc, DI_FOCUS, False, True);
		if (IsMacEmulatedLF() || (IsMacLFAM() && !MCaqua))
			trect = MCU_reduce_rect(trect, 1);
		drawborder(dc, trect, MCfocuswidth);
	}
}


void MCControl::getwidgetthemeinfo(MCWidgetInfo &widgetinfo)
{
	uint4 wstate = 0;
	if (flags & F_DISABLED)
		wstate |= WTHEME_STATE_DISABLED;
	else
		if (state & CS_MFOCUSED && !(state & CS_SELECTED)
		        && MCU_point_in_rect(rect, mx, my))
			wstate |= WTHEME_STATE_PRESSED;
	widgetinfo.state = wstate;
	widgetinfo.part = WTHEME_PART_UNDEFINED;
	widgetinfo.attributes = WTHEME_ATT_CLEAR;
	widgetinfo.datatype = WTHEME_DATA_NONE;
	widgetinfo.data = NULL;
	widgetinfo.whichobject = this;
}

void MCControl::unlink(MCControl *p_control)
{
}

// MW-2011-08-18: [[ Layers ]] The effective rect is the rect encompassing the
//   control on *it's* layer, any recursive affected area due to nested effects
//   is handled by the layer mechanism.
MCRectangle MCControl::geteffectiverect(void) const
{
	MCRectangle t_rect;
	t_rect = MCU_reduce_rect(rect, -gettransient());

	if (m_bitmap_effects != nil)
		MCBitmapEffectsComputeBounds(m_bitmap_effects, t_rect, t_rect);

	return t_rect;
}

MCBitmapEffectsRef MCControl::getbitmapeffects(void)
{
	return m_bitmap_effects;
}

void MCControl::setbitmapeffects(MCBitmapEffectsRef p_effects)
{
	m_bitmap_effects = p_effects;
}

MCObject *MCControl::hittest(int32_t x, int32_t y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles) ||
	    (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return nil;
	
	MCRectangle r;
	MCU_set_rect(r, x, y, 1, 1);
	if (maskrect(r))
		return this;
	return nil;
}
