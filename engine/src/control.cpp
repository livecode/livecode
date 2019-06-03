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


#include "util.h"
#include "sellst.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "mccontrol.h"
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
#include "widget.h"

#include "exec.h"

MCControlHandle MCControl::focused;
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
    DEFINE_RW_OBJ_PROPERTY(P_LAYER_CLIP_RECT, OptionalRectangle, MCControl, LayerClipRect)
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
    m_layer_has_clip_rect = false;
    m_layer_clip_rect = kMCEmptyRectangle;
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
    m_layer_has_clip_rect = cref.m_layer_has_clip_rect;
    m_layer_clip_rect = cref.m_layer_clip_rect;
}

MCControl::~MCControl()
{
	if (focused.IsBoundTo(this))
		focused = nullptr;
    
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
		
        // Make sure we keep state which should be preserved across open.
		state = (state & (CS_NO_MESSAGES | CS_NO_FILE | CS_SELECTED)) | (state & CS_KEEP_LAYER);
	}
	
	MCObject::open();
}

void MCControl::close()
{
	// MW-2008-01-09: [[ Bug 5739 ]] Changing group layer cancels mousedown
	if (opened == 1 && focused.IsBoundTo(this))
    {
        focused = nullptr;
    }
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
	if (!(flags & F_VISIBLE || showinvisible())
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
    
    mx = x;
    my = y;
    
	Boolean is = maskrect(srect) || (state & CS_SELECTED
	                                 && MCU_point_in_rect(geteffectiverect(), x, y)
	                                 && sizehandles(x, y) != 0);
    
	if (is || state & CS_MFOCUSED)
	{
		if (focused.IsBoundTo(this) || getstack() -> gettool(this) == T_POINTER)
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
	if (focused.IsBoundTo(this))
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
	if (MCNameIsEqualToCaseless(mptr, MCM_idle))
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

void MCControl::select()
{
	state |= CS_SELECTED;
	kunfocus();

	getcard()->dirtyselection(rect);
}

void MCControl::deselect()
{
	if (state & CS_SELECTED)
	{
		getcard()->dirtyselection(rect);

        state &= ~(CS_SELECTED | CS_MOVE | CS_SIZE | CS_CREATE);
	}
}

Boolean MCControl::del(bool p_check_flag)
{
	if (!isdeletable(p_check_flag))
	    return False;
	
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

	// IM-2016-10-05: [[ Bug 17008 ]] Dirty selection handles when object deleted
	if (getselected())
		getcard()->dirtyselection(rect);

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
    
	switch (parent->gettype())
	{
        case CT_STACK:
            removereferences();
            parent.GetAs<MCStack>()->removecontrol(this);
            break;
        
        case CT_CARD:
            if (!parent.GetAs<MCCard>()->removecontrol(this, False, True))
				return False;
            removereferences();
			getstack()->removecontrol(this);
			break;
        
        case CT_GROUP:
			parent.GetAs<MCGroup>()->removecontrol(this, True);
            removereferences();
			break;

        default:
            MCUnreachable();
	}
    
    // MCObject now does things on del(), so we must make sure we finish by
    // calling its implementation.
    return MCObject::del(p_check_flag);
}

void MCControl::paste(void)
{
	if (MCdefaultstackptr->getmode() != WM_TOP_LEVEL)
		return;

	parent = MCdefaultstackptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
	MCCard *cptr = parent.GetAs<MCCard>();
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

/* The MCRereferenceChildrenVisitor visits each of a control's descendents
 * recursively and makes sure they have a weak-proxy and that the parent is
 * updated. */
class MCRereferenceChildrenVisitor: public MCObjectVisitor
{
public:
    static void Visit(MCControl *p_control)
    {
        MCRereferenceChildrenVisitor t_visitor(p_control);
        
        /* Make sure the control has a weak proxy (needed to set the parent of
         * its children!). */
        p_control->ensure_weak_proxy();
        
        /* Now visit the controls children. */
        p_control->visit_children(0, 0, &t_visitor);
    }

private:
    MCRereferenceChildrenVisitor(MCObject *p_new_parent)
        : m_new_parent(p_new_parent)
    {
    }

    bool OnControl(MCControl* p_control)
    {
        /* Update the control's parent */
        p_control->setparent(m_new_parent);
        
        /* Update its children */
        MCRereferenceChildrenVisitor::Visit(p_control);
        
        return true;
    }
    
    MCObject *m_new_parent;
};

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
            
            /* Visit the control and its children, creating weak_proxys and
             * reparenting as we go. */
            MCRereferenceChildrenVisitor::Visit(this);
            
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
        if (gettype() == CT_WIDGET)
        {
            static_cast<MCWidget*>(this)->delforundo(true);
        }
        else
        {
            del(true);
        }
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

IO_stat MCControl::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	return MCObject::save(stream, p_part, p_force_ext, p_version);
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
	if (!(flags & F_VISIBLE || showinvisible()))
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

	if (!parent || parent->gettype() == CT_CARD
	        || parent->gettype() == CT_STACK)
	{
		MCCard *card = getcard(cid);
		getstack()->appendcontrol(this);
		card->newcontrol(this, True);
	}
	else
	{
		MCGroup *gptr = parent.GetAs<MCGroup>();
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
	if (!opened || !(getflag(F_VISIBLE) || showinvisible()))
		return;

	// MW-2009-06-11: [[ Bitmap Effects ]] A control needs to be (partially)
	//   redrawn if the dirty region touches it or one of its effects.
	MCRectangle trect = MCU_intersect_rect(dirty, geteffectiverect());
	if (trect.width != 0 && trect.height != 0)
	{
		dc->save();
		
		dc -> setopacity(255);
		dc -> setfunction(GXcopy);
        
        /* Apply the layerClipRect property, if set. */
        if (m_layer_has_clip_rect)
        {
            trect = MCU_intersect_rect(trect, m_layer_clip_rect);
        }
        
		dc->cliprect(trect);
        
		// MW-2011-09-06: [[ Redraw ] Make sure we draw the control normally (not
		//   as a sprite).
		draw(dc, trect, false, false);
		
		dc->restore();
	}
}

void MCControl::sizerects(const MCRectangle &p_object_rect, MCRectangle r_rects[8])
{
	int2 x[3];
	int2 y[3];

	uint2 handlesize = MCsizewidth;

	x[0] = p_object_rect.x - (handlesize >> 1);
	x[1] = p_object_rect.x + ((p_object_rect.width - handlesize) >> 1);
	x[2] = p_object_rect.x + p_object_rect.width - (handlesize >> 1);
	y[0] = p_object_rect.y - (handlesize >> 1);
	y[1] = p_object_rect.y + ((p_object_rect.height - handlesize) >> 1);
	y[2] = p_object_rect.y + p_object_rect.height - (handlesize >> 1);
	
	uint2 i;
	uint2 j;
	uint2 k = 0;
	for (i = 0 ; i < 3 ; i++)
		for (j = 0 ; j < 3 ; j++)
			if (i != 1 || j != 1)
			{
				r_rects[k].width = r_rects[k].height = handlesize;
				r_rects[k].x = x[j];
				r_rects[k].y = y[i];
				k++;
			}
}

void MCControl::drawselection(MCDC *dc, const MCRectangle& p_dirty)
{
    MCAssert(getopened() != 0 && (getflag(F_VISIBLE) || showinvisible()));
    
    if (!getselected())
    {
        return;
    }
    
	if (MCdragging)
		return;

	dc -> setopacity(255);
    dc -> setquality(QUALITY_SMOOTH);
    dc -> setfunction(GXcopy);

    drawmarquee(dc, rect);
    
    MCRectangle rects[8];
    sizerects(rect, rects);
    if (flags & F_LOCK_LOCATION)
        dc->setfillstyle(FillStippled, nil, 0, 0);
    else
        dc->setfillstyle(FillSolid, nil, 0, 0);
    dc->setforeground(MCselectioncolor);

    for (uint2 i = 0; i < 8; i++)
        dc->fillarc(rects[i], 0, 360, false);
    if (flags & F_LOCK_LOCATION)
        dc->setfillstyle(FillSolid, nil, 0, 0);

    dc->setquality(QUALITY_DEFAULT);
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
		if (newrect.x > rx)
			newrect.x = rx;
		newrect.width = rx - newrect.x;
	}
	else
		if (state & CS_SIZER)
		{
			int2 rx = x + xoffset;
			MCU_snap(rx);
			if (rx - newrect.x < 0)
				newrect.width = 0;
			else
				newrect.width = rx - newrect.x;
		}
	if (state & CS_SIZET)
	{
		int2 ly = newrect.y + newrect.height;
		newrect.y = y - yoffset;
		MCU_snap(newrect.y);
		if (newrect.y > ly)
			newrect.y = ly;
		newrect.height = ly - newrect.y;
	}
	else
		if (state & CS_SIZEB)
		{
			int2 ly = y + yoffset;
			MCU_snap(ly);
			if (ly - newrect.y < 0)
				newrect.height = 0;
			else
				newrect.height = ly - newrect.y;
		}
	if (MCmodifierstate & MS_SHIFT)
	{
		double newaspect, newwidth;
        newwidth = newrect.width != 0 ? (double)newrect.width : 1;
        newaspect = newrect.height / newwidth;

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
			int2 t_newwidth;
			t_newwidth = (int2)(newrect.height / aspect);
			if (state & CS_SIZEL)
				newrect.x += newrect.width - t_newwidth;
			newrect.width = t_newwidth;
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
		state |= CS_SIZEL;
		fliph();
	}
	else
		if (mx > newrect.x + newrect.width && state & CS_SIZEL)
		{
			state &= ~CS_SIZEL;
			state |= CS_SIZER;
			fliph();
		}
	if (my < newrect.y && state & CS_SIZEB)
	{
		state &= ~CS_SIZEB;
		state |= CS_SIZET;
		flipv();
	}
	else
		if (my > newrect.y + newrect.height && state & CS_SIZET)
		{
			state &= ~CS_SIZET;
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

#define SIZE_HANDLE_HIT_TOLERANCE 1

uint2 MCControl::sizehandles(int2 px, int2 py)
{
	uint2 newstate = 0;
	if (!(flags & F_LOCK_LOCATION))
	{
		MCRectangle rects[8];
		sizerects(rect, rects);
		int2 i;
		for (i = 7 ; i >= 0 ; i--)
        {
            // Be more forgiving about handle hit detection
            rects[i] = MCU_reduce_rect(rects[i], -SIZE_HANDLE_HIT_TOLERANCE);
			if (MCU_point_in_rect(rects[i], px, py))
			{
				if (i < 3)
				{
					newstate |= CS_SIZET;
					yoffset = py - rect.y;
				}
				else
					if (i > 4)
					{
						newstate |= CS_SIZEB;
						yoffset = rect.y + rect.height - py;
					}
				if (i == 0 || i == 3 || i == 5)
				{
					newstate |= CS_SIZEL;
					xoffset = px - rect.x;
				}
				else
					if (i == 2 || i == 4 || i == 7)
					{
						newstate |= CS_SIZER;
						xoffset = rect.x + rect.width - px;
					}
				break;
			}
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
	
	state |= sizehandles(mx, my);
	if (!(state & CS_SELECTED))
	{
		if (MCmodifierstate & MS_SHIFT)
			MCselected->add(this);
		else
			MCselected->replace(this);
	}
	else
	{
		if (MCmodifierstate & MS_SHIFT && sizehandles(mx, my) == 0)
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
				Ustruct *us = new (nothrow) Ustruct;
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
	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_message_str;
	if (t_success)
		t_success = MCStringFormat(&t_message_str, "new%s", gettypestring());
	
	MCNewAutoNameRef t_message;
	if (t_success)
		t_success = MCNameCreate(*t_message_str, &t_message);
	
	if (t_success)
		message(*t_message);

	if (gettype() == CT_GROUP)
		message(MCM_new_background);
}

void MCControl::enter()
{
    MCControlHandle t_this(this);
    
    if (focused.IsValid() && !focused.IsBoundTo(this))
    {
        leave();
    }
    
    if (!t_this.IsValid())
    {
        return;
    }
    
	if (MCdispatcher -> isdragtarget())
	{
		MCdragaction = DRAG_ACTION_NONE;
		MCdragdest = this;
        
        // Give the script a chance to respond to the drag operation. If it
        // doesn't complete successfully or passes, attempt an
        // automatic drag-drop operation (if this is a field object).
		if (message(MCM_drag_enter) != ES_NORMAL && gettype() == CT_FIELD
		        && !(flags & F_LOCK_TEXT)
                && MCdragboard->HasTextOrCompatible())
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
    if(!t_this.IsValid())
    {
        return;
    }
    if (handlesmessage(MCM_mouse_within) && !(hashandlers & HH_IDLE))
		MCscreen->addtimer(this, MCM_idle, MCidleRate);
	if (getstack()->gettool(this) == T_BROWSE)
		MCtooltip->settip(tooltip);
	focused = this;
}

void MCControl::leave()
{
	MCControlHandle oldfocused = focused;
	if (MCdispatcher -> isdragtarget())
	{
		// MW-2013-08-08: [[ Bug 10655 ]] If oldfocused is a field and has dragText set,
		//   then make sure we unset it (otherwise the caret will continue moving around
		//   on mouseMove).
		if (oldfocused.IsValid())
        {
            if (oldfocused->gettype() == CT_FIELD
		        && oldfocused -> getstate(CS_DRAG_TEXT))
            {
                MCField *fptr = oldfocused.GetAs<MCField>();
                fptr->removecursor();
                getstack()->clearibeam();
                oldfocused->state &= ~(CS_DRAG_TEXT | CS_IBEAM);
            }
            oldfocused->message(MCM_drag_leave);
        }
		MCdragaction = DRAG_ACTION_NONE;
		MCdragdest = nil;
	}
	else if (oldfocused.IsValid())
    {
		oldfocused->message(MCM_mouse_leave);
    }
    
    focused = nullptr;
}

Boolean MCControl::sbfocus(int2 x, int2 y, MCScrollbar *hsb, MCScrollbar *vsb)
{
	if (state & (CS_HSCROLL | CS_VSCROLL))
	{
		if (state & CS_HSCROLL && hsb != nil)
			hsb->mfocus(x, y);
		else if (vsb != nil)
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
				hsb = new (nothrow) MCScrollbar(*MCtemplatescrollbar);
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
				// PM-2015-07-16: [[ Bug 11569 ]] Unset CS_HSCROLL when the hscrollBar of a control is set to false
			    state &= ~CS_HSCROLL;
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
				vsb = new (nothrow) MCScrollbar(*MCtemplatescrollbar);
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
				// PM-2015-07-16: [[ Bug 11569 ]] Unset CS_VSCROLL when the vscrollBar of a control is set to false
				state &= ~CS_VSCROLL;
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
	if (!(flags & F_VISIBLE || showinvisible()) ||
	    (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return nil;
	
	MCRectangle r;
	MCU_set_rect(r, x, y, 1, 1);
	if (maskrect(r))
		return this;
	return nil;
}
