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


#include "util.h"
#include "font.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "scrolbar.h"
#include "mcerror.h"
#include "param.h"
#include "globals.h"
#include "dispatch.h"
#include "mctheme.h"

#include "exec.h"

#include "stackfileformat.h"

real8 MCScrollbar::markpos;
uint2 MCScrollbar::mode = SM_CLEARED;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCScrollbar::kProperties[] =
{
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_STYLE, InterfaceScrollbarStyle, MCScrollbar, Style)
	DEFINE_RW_OBJ_PROPERTY(P_THUMB_SIZE, Double, MCScrollbar, ThumbSize)
	DEFINE_RW_OBJ_PROPERTY(P_THUMB_POS, Double, MCScrollbar, ThumbPos)
	DEFINE_RW_OBJ_PROPERTY(P_LINE_INC, Double, MCScrollbar, LineInc)
	DEFINE_RW_OBJ_PROPERTY(P_PAGE_INC, Double, MCScrollbar, PageInc)
	DEFINE_RO_OBJ_ENUM_PROPERTY(P_ORIENTATION, InterfaceScrollbarOrientation, MCScrollbar, Orientation)
	DEFINE_RW_OBJ_PROPERTY(P_NUMBER_FORMAT, String, MCScrollbar, NumberFormat)
	DEFINE_RW_OBJ_PROPERTY(P_START_VALUE, String, MCScrollbar, StartValue)
	DEFINE_RW_OBJ_PROPERTY(P_END_VALUE, String, MCScrollbar, EndValue)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_VALUE, Bool, MCScrollbar, ShowValue)
};

MCObjectPropertyTable MCScrollbar::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCScrollbar::MCScrollbar()
{
	flags |= F_HORIZONTAL | F_TRAVERSAL_ON;
	rect.width = rect.height = DEFAULT_SB_WIDTH;
	thumbpos = 0.0;
	thumbsize = 8192.0;
	pageinc = thumbsize;
	lineinc = 512.0;
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Initialize the members to kMCEmptyString.
	startstring = MCValueRetain(kMCEmptyString);
	endstring = MCValueRetain(kMCEmptyString);
	startvalue = 0.0;
	endvalue = 65535.0;
	nffw = 0;
	nftrailing = 0;
	nfforce = 0;
	
	hover_part = WTHEME_PART_UNDEFINED;

	linked_control = NULL;
	
	m_embedded = false;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the progress bar animate message is only posted from a single thread.
    m_animate_posted = false;
}

MCScrollbar::MCScrollbar(const MCScrollbar &sref) : MCControl(sref)
{
	thumbpos = sref.thumbpos;
	thumbsize = sref.thumbsize;
	pageinc = sref.pageinc;
	lineinc = sref.lineinc;
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Initialize the members to the other scrollbars values
	startstring = MCValueRetain(sref . startstring);
	endstring = MCValueRetain(sref . endstring);
	startvalue = sref.startvalue;
	endvalue = sref.endvalue;
	nffw = sref.nffw;
	nftrailing = sref.nftrailing;
	nfforce = sref.nfforce;

	hover_part = WTHEME_PART_UNDEFINED;

	linked_control = NULL;

	m_embedded = false;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the progress bar animate message is only posted from a single thread.
    m_animate_posted = false;
}

MCScrollbar::~MCScrollbar()
{
	if (linked_control != NULL)
		linked_control -> unlink(this);
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Release the string members.
	MCValueRelease(startstring);
	MCValueRelease(endstring);
}

Chunk_term MCScrollbar::gettype() const
{
	return CT_SCROLLBAR;
}

const char *MCScrollbar::gettypestring()
{
	return MCscrollbarstring;
}

bool MCScrollbar::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnScrollbar(this);
}

void MCScrollbar::open()
{
	MCControl::open();

	// MW-2011-02-03: [[ Bug 7861 ]] Should make working with large Mac progress bars better.
	if (opened == 1 && !getflag(F_SB_STYLE))
	{
		uint2 oldwidth = MAC_SB_WIDTH;
		uint2 newwidth = DEFAULT_SB_WIDTH;
		if (IsMacLFAM())
		{
			oldwidth = DEFAULT_SB_WIDTH;
			newwidth = MAC_SB_WIDTH;
		}
		borderwidth = DEFAULT_BORDER;
		if (rect.height == oldwidth)
			rect.height = newwidth;
		if (rect.width == oldwidth)
			rect.width = newwidth;
	}

	compute_barsize();
}

Boolean MCScrollbar::kdown(MCStringRef p_string, KeySym key)
{
	if (!(state & CS_NO_MESSAGES))
		if (MCObject::kdown(p_string, key))
			return True;
	Boolean done = False;
	switch (key)
	{
	case XK_Home:
		update(0.0, MCM_scrollbar_line_inc);
		done = True;
		break;
	case XK_End:
		update(endvalue, MCM_scrollbar_end);
		done = True;
		break;
	case XK_Right:
	case XK_Down:
		update(thumbpos + lineinc, MCM_scrollbar_line_inc);
		done = True;
		break;
	case XK_Left:
	case XK_Up:
		update(thumbpos - lineinc, MCM_scrollbar_line_dec);
		done = True;
		break;
	case XK_Prior:
		update(thumbpos - pageinc, MCM_scrollbar_page_dec);
		done = True;
		break;
	case XK_Next:
		update(thumbpos + pageinc, MCM_scrollbar_page_inc);
		done = True;
		break;
	default:
		break;
	}
	if (done)
		message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
	return done;
}

Boolean MCScrollbar::mfocus(int2 x, int2 y)
{
	// MW-2007-09-18: [[ Bug 1650 ]] Disabled state linked to thumb size
	if (!(flags & F_VISIBLE || showinvisible())
	    || (issbdisabled() && getstack()->gettool(this) == T_BROWSE))
		return False;
	if (state & CS_SCROLL)
	{
		// I.M. [[bz 9559]] disable scrolling where start value & end value are the same
		if (startvalue == endvalue)
			return True;

		real8 newpos;

		double t_thumbsize = thumbsize;
		if (t_thumbsize > fabs(endvalue - startvalue))
			t_thumbsize = fabs(endvalue - startvalue);
		if (flags & F_SCALE)
			t_thumbsize = 0;

		bool t_forward;
		t_forward = (endvalue > startvalue);
		MCRectangle t_bar_rect, t_thumb_rect, t_thumb_start_rect, t_thumb_end_rect;
		t_bar_rect = compute_bar();
		t_thumb_rect = compute_thumb(markpos);
		t_thumb_start_rect = compute_thumb(startvalue);
		if (t_forward)
			t_thumb_end_rect = compute_thumb(endvalue - t_thumbsize);
		else
			t_thumb_end_rect = compute_thumb(endvalue + t_thumbsize);

		int32_t t_bar_start, t_bar_length, t_thumb_start, t_thumb_length;
		int32_t t_movement;
		if (getstyleint(flags) == F_VERTICAL)
		{
			t_bar_start = t_thumb_start_rect.y;
			t_bar_length = t_thumb_end_rect.y + t_thumb_end_rect.height - t_bar_start;
			t_thumb_start = t_thumb_rect.y;
			t_thumb_length = t_thumb_rect.height;
			t_movement = y - my;
		}
		else
		{
			t_bar_start = t_thumb_start_rect.x;
			t_bar_length = t_thumb_end_rect.x + t_thumb_end_rect.width - t_bar_start;
			t_thumb_start = t_thumb_rect.x;
			t_thumb_length = t_thumb_rect.width;
			t_movement = x - mx;
		}

		t_bar_start += t_thumb_length / 2;
		t_bar_length -= t_thumb_length;

        // AL-2013-07-26: [[ Bug 11044 ]] Prevent divide by zero when computing scrollbar thumbposition
        if (t_bar_length == 0)
            t_bar_length = 1;
        
		int32_t t_new_position;
		t_new_position = t_thumb_start + t_thumb_length / 2 + t_movement;
		t_new_position = MCU_min(t_bar_start + t_bar_length, MCU_max(t_bar_start, t_new_position));
		
		if (t_forward)
			newpos = startvalue + ((t_new_position - t_bar_start) / (double)t_bar_length) * (fabs(endvalue - startvalue) - t_thumbsize);
		else
			newpos = startvalue - ((t_new_position - t_bar_start) / (double)t_bar_length) * (fabs(endvalue - startvalue) - t_thumbsize);

		update(newpos, MCM_scrollbar_drag);
		return True;
	}
	else if (!MCdispatcher -> isdragtarget() && MCcurtheme && MCcurtheme->getthemepropbool(WTHEME_PROP_SUPPORTHOVERING)
	         && MCU_point_in_rect(rect, x, y) )
	{
		if (!(state & CS_MFOCUSED) && !getstate(CS_SELECTED))
		{
			MCWidgetInfo winfo;
			winfo.type = (Widget_Type)getwidgetthemetype();
			if (MCcurtheme->iswidgetsupported(winfo.type))
			{
				getwidgetthemeinfo(winfo);
				Widget_Part wpart = MCcurtheme->hittest(winfo,mx,my,rect);
				if (wpart != hover_part)
				{
					hover_part = wpart;
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					redrawall();
				}
			}
		}
	}
	return MCControl::mfocus(x, y);
}

void MCScrollbar::munfocus()
{
	if (MCcurtheme && hover_part != WTHEME_PART_UNDEFINED && MCcurtheme->getthemepropbool(WTHEME_PROP_SUPPORTHOVERING))
	{
		hover_part = WTHEME_PART_UNDEFINED;
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		redrawall();
	}
	MCControl::munfocus();
}

Boolean MCScrollbar::mdown(uint2 which)
{
	if (state & CS_MFOCUSED)
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	state |= CS_MFOCUSED;
	if (!IsMacEmulatedLF() && flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
		getstack()->kfocusset(this);
	uint2 margin;
	MCRectangle brect = compute_bar();
	if (flags & F_SCALE)
		margin = 0;
	else
		if (getstyleint(flags) == F_VERTICAL)
			margin = brect.width - 1;
		else
			margin = brect.height - 1;
	Tool tool = state & CS_NO_MESSAGES ? T_BROWSE : getstack()->gettool(this);
	MCWidgetInfo winfo;
	winfo.type = (Widget_Type)getwidgetthemetype();
	switch (which)
	{
	case Button1:
		switch (tool)
		{
		case T_BROWSE:
			message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
			if (flags & F_PROGRESS) //progress bar does not respond to mouse down event
				return False;
			if (MCcurtheme && MCcurtheme->iswidgetsupported(winfo.type))
			{
				getwidgetthemeinfo(winfo);
				Widget_Part wpart = MCcurtheme->hittest(winfo,mx,my,rect);
				// scrollbar needs to check first if mouse-down occurred in arrows
				switch (wpart)
				{
				case WTHEME_PART_ARROW_DEC:
					if (MCmodifierstate & MS_SHIFT)
						mode = SM_BEGINNING;
					else
						mode = SM_LINEDEC;
					break;
				case WTHEME_PART_ARROW_INC:
					if (MCmodifierstate & MS_SHIFT)
						mode = SM_END;
					else
						mode = SM_LINEINC;
					break;
				case WTHEME_PART_TRACK_DEC:
					mode = SM_PAGEDEC;
					break;
				case WTHEME_PART_TRACK_INC:
					mode = SM_PAGEINC;
					break;
				default:
					break;
				}
			}
			else
			{ //Non-theme appearence stuff: for vertical scrollbar or scale
				if (getstyleint(flags) == F_VERTICAL)
				{
					uint2 height;
					if (brect.height <= margin << 1)
						height = 1;
					else
						height = brect.height - (margin << 1);
					markpos = (my - (brect.y + margin))
					          * fabs(endvalue - startvalue) / height;
					if (my < brect.y + margin)
						if (MCmodifierstate & MS_SHIFT)
							mode = SM_BEGINNING;
						else
							mode = SM_LINEDEC;
					else
						if (my > brect.y + brect.height - margin)
							if (MCmodifierstate & MS_SHIFT)
								mode = SM_END;
							else
								mode = SM_LINEINC;
						else
						{
							MCRectangle thumb = compute_thumb(thumbpos);
							if (my < thumb.y)
								mode = SM_PAGEDEC;
							else
								if (my > thumb.y + thumb.height)
									mode = SM_PAGEINC;
						}
				}
				else
				{ //for Horizontal scrollbar or scale
					uint2 width;
					if (brect.width <= (margin << 1))
						width = 1;
					else
						width = brect.width - (margin << 1);
					markpos = (mx - (brect.x + margin))
					          * fabs(endvalue - startvalue) / width;
					if (mx < brect.x + margin)
						if (MCmodifierstate & MS_SHIFT)
							mode = SM_BEGINNING;
						else
							mode = SM_LINEDEC;
					else
						if (mx > brect.x + brect.width - margin)
							if (MCmodifierstate & MS_SHIFT)
								mode = SM_END;
							else
								mode = SM_LINEINC;
						else
						{
							MCRectangle thumb = compute_thumb(thumbpos);
							if (mx < thumb.x)
								mode = SM_PAGEDEC;
							else
								if (mx > thumb.x + thumb.width)
									mode = SM_PAGEINC;
						}
				}
			} //end of Non-MAC-Appearance Manager stuff
			switch (mode)
			{
			case SM_BEGINNING:
				update(0, MCM_scrollbar_beginning);
				break;
			case SM_END:
				update(endvalue, MCM_scrollbar_end);
				break;
			case SM_LINEDEC:
			case SM_LINEINC:
				timer(MCM_internal, NULL);
				redrawarrow(mode);
				break;
			case SM_PAGEDEC:
			case SM_PAGEINC:
				timer(MCM_internal, NULL);
				break;
			default:
				state |= CS_SCROLL;
				markpos = thumbpos;
				if (IsMacEmulatedLF())
					movethumb(thumbpos--);
				else if (MCcurtheme)
				{
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					redrawall();
				}
			}
			break;
		case T_POINTER:
		case T_SCROLLBAR:
			start(True);
			break;
		case T_HELP:
			break;
		default:
			return False;
		}
		break;
	case Button2:
		if (message_with_valueref_args(MCM_mouse_down, MCSTR("2")) == ES_NORMAL)
			return True;
		state |= CS_SCROLL;
		{
			real8 newpos;
			real8 range = endvalue - startvalue;
			real8 offset = startvalue;
			if (flags & F_SCALE)
				margin = MOTIF_SCALE_THUMB_SIZE >> 1;
			else
				if (MCproportionalthumbs)
					if (startvalue > endvalue)
						offset += thumbsize / 2.0;
					else
						offset -= thumbsize / 2.0;
				else
					margin = FIXED_THUMB_SIZE >> 1;
			if (getstyleint(flags) == F_VERTICAL)
				newpos = (my - brect.y - margin) * range
				         / (brect.height - (margin << 1)) + offset;
			else
				newpos = (mx - brect.x - margin) * range
				         / (brect.width - (margin << 1)) + offset;
			update(newpos, MCM_scrollbar_drag);
			markpos = thumbpos;
		}
		break;
	case Button3:
		message_with_valueref_args(MCM_mouse_down, MCSTR("3"));
		break;
	}
	return True;
}

Boolean MCScrollbar::mup(uint2 which, bool p_release)
{
	if (!(state & CS_MFOCUSED))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
	state &= ~CS_MFOCUSED;
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	Tool tool = state & CS_NO_MESSAGES ? T_BROWSE : getstack()->gettool(this);
	switch (which)
	{
	case Button1:
		switch (tool)
		{
		case T_BROWSE:
			if (state & CS_SCROLL)
			{
				state &= ~CS_SCROLL;
				if (IsMacEmulatedLF())
					movethumb(thumbpos--);
				else if (MCcurtheme)
				{
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					redrawall();
				}
			}
			else
			{
				uint2 oldmode = mode;
				mode = SM_CLEARED;
				if (MCcurtheme)
				{
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					redrawall();
				}
				else if (oldmode == SM_LINEDEC || oldmode == SM_LINEINC)
					redrawarrow(oldmode);
			}
			if (!p_release && MCU_point_in_rect(rect, mx, my))
				message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
			else
				message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
			break;
		case T_SCROLLBAR:
		case T_POINTER:
			end(true, p_release);
			break;
		case T_HELP:
			help();
			break;
		default:
			return False;
		}
		break;
	case Button2:
		if (state & CS_SCROLL)
		{
			state &= ~CS_SCROLL;
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			redrawall();
		}
	case Button3:
		if (!p_release && MCU_point_in_rect(rect, mx, my))
			message_with_args(MCM_mouse_up, which);
		else
			message_with_args(MCM_mouse_release, which);
		break;
	}
	return True;
}

Boolean MCScrollbar::doubledown(uint2 which)
{
	if (which == Button1 && getstack()->gettool(this) == T_BROWSE)
		return mdown(which);
	return MCControl::doubledown(which);
}

Boolean MCScrollbar::doubleup(uint2 which)
{
	if (which == Button1 && getstack()->gettool(this) == T_BROWSE)
		return mup(which, false);
	return MCControl::doubleup(which);
}

void MCScrollbar::applyrect(const MCRectangle &nrect)
{
	rect = nrect;
	compute_barsize();
}


void MCScrollbar::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualToCaseless(mptr, MCM_internal) ||
		MCNameIsEqualToCaseless(mptr, MCM_internal2))
	{
		// MW-2009-06-16: [[ Bug ]] Previously this was only waiting for one
		//   one event (anyevent == True) this meant that it was possible for
		//   the critical 'mouseUp' event to be missed and an effectively
		//   infinite timer loop to be entered if the amount of processing
		//   inbetween timer invocations was too high. So, instead, we process
		//   all events at this point to ensure the mouseUp is handled.
		//   (In the future we should flush mouseUp events to the dispatch queue)
        // MW-2014-04-16: [[ Bug 12183 ]] This wait does not seem to make much
        //   sense. It seems to be so that a mouseUp in a short space of time
        //   stops the scrollbar from moving. This isn't how things should be I
        //   don't think - so commenting it out for now.
        // MCscreen->wait(MCsyncrate / 1000.0, True, False); // dispatch mup
		if (state & CS_MFOCUSED && !MCbuttonstate)
		{
			mup(Button1, false);
			return;
		}

		if (mode != SM_CLEARED)
		{
			uint2 delay = MCNameIsEqualToCaseless(mptr, MCM_internal) ? MCrepeatdelay : MCrepeatrate;
			MCscreen->addtimer(this, MCM_internal2, delay);
			MCRectangle thumb;
			switch (mode)
			{
			case SM_LINEDEC:
				update(thumbpos - lineinc, MCM_scrollbar_line_dec);
				break;
			case SM_LINEINC:
				update(thumbpos + lineinc, MCM_scrollbar_line_inc);
				break;
			case SM_PAGEDEC:
				if ( flags & F_SCALE )	
					update(thumbpos - pageinc, MCM_scrollbar_page_dec);
				else 
				{
					// TH-2008-01-23. Previously was pageinc - lineinc which appeared to be wrong
					update(thumbpos - (pageinc - lineinc), MCM_scrollbar_page_dec);
				}
				thumb = compute_thumb(thumbpos);
				if (getstyleint(flags) == F_VERTICAL)
				{
					if (thumb.y + thumb.height < my)
						mode = SM_CLEARED;
				}
				else
					if (thumb.x + thumb.height < mx)
						mode = SM_CLEARED;
				break;
			case SM_PAGEINC:
				if (flags & F_SCALE)
					update(thumbpos + pageinc, MCM_scrollbar_page_inc);
				else
				{
					// TH-2008-01-23. Previously was pageinc + lineinc which appeared to be wrong.
					update(thumbpos + (pageinc - lineinc), MCM_scrollbar_page_inc);
				}
				thumb = compute_thumb(thumbpos);
				if (getstyleint(flags) == F_VERTICAL)
				{
					if (thumb.y > my)
						mode = SM_CLEARED;
				}
				else
					if (thumb.x > mx)
						mode = SM_CLEARED;
				break;
			}
			if (parent->gettype() != CT_CARD)
			{
				MCControl *cptr = parent.GetAs<MCControl>();
				cptr->readscrollbars();
			}
		}
	}
	else if (MCNameIsEqualToCaseless(mptr, MCM_internal3))
	{
#ifdef _MAC_DESKTOP
		// MW-2012-09-17: [[ Bug 9212 ]] Mac progress bars do not animate.
		if (getflag(F_PROGRESS))
		{
            // MM-2014-07-31: [[ ThreadedRendering ]] Flag that there is no longer a progress bar animation message pending.
            m_animate_posted = false;
            
			redrawall();
		}
#endif
	}
	else
		MCControl::timer(mptr, params);
}

MCControl *MCScrollbar::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCScrollbar *newscrollbar = new (nothrow) MCScrollbar(*this);
	if (attach)
		newscrollbar->attach(p, invisible);
	return newscrollbar;
}

void MCScrollbar::compute_barsize()
{
	if (flags & F_SHOW_VALUE && (MClook == LF_MOTIF
	                             || getstyleint(flags) == F_VERTICAL))
	{
		if (getstyleint(flags) == F_VERTICAL)
		{
			uint2 twidth = rect.width != 0 ? rect.width : 1;
			barsize = MCU_max(nffw, 1);
			// MW-2013-08-27: [[ UnicodifyScrollbar ]] Use MCString primitives.
			if (MCStringGetLength(startstring) > barsize)
				barsize = MCStringGetLength(startstring);
			if (MCStringIsEmpty(endstring))
				barsize = MCU_max(barsize, 5);
			else
                barsize = MCU_max((uindex_t)barsize, MCStringGetLength(endstring));
            // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            barsize *= MCFontMeasureText(m_font, MCSTR("0"), getstack() -> getdevicetransform());
			barsize = twidth - (barsize + barsize * (twidth - barsize) / twidth);
		}
		else
		{
			uint2 theight = rect.height;
			barsize = theight - gettextheight();
		}
	}
	else
		if (getstyleint(flags) == F_VERTICAL)
			barsize = rect.width;
		else
			barsize = rect.height;
}

MCRectangle MCScrollbar::compute_bar()
{
	MCRectangle brect = rect;
	if (flags & F_SHOW_VALUE && (MClook == LF_MOTIF
	                             || getstyleint(flags) == F_VERTICAL))
	{
		if (getstyleint(flags) == F_VERTICAL)
			brect.width = barsize;
		else
		{
			brect.y += brect.height - barsize;
			brect.height = barsize;
		}
	}
	return brect;
}

MCRectangle MCScrollbar::compute_thumb(real8 pos)
{
	MCRectangle thumb;
	MCWidgetInfo winfo;
	winfo.type = (Widget_Type)getwidgetthemetype();
	
	thumb . width = thumb . height = thumb . x = thumb . y = 0 ; // Initialize the values.
	
	if (MCcurtheme && MCcurtheme->iswidgetsupported(winfo.type))
	{
		getwidgetthemeinfo(winfo);
		winfo.part = WTHEME_PART_THUMB;
		((MCWidgetScrollBarInfo*)(winfo.data))->thumbpos = pos;
		MCcurtheme->getwidgetrect(winfo,WTHEME_METRIC_PARTSIZE,rect,thumb);
	}
	else
	{
		MCRectangle trect = compute_bar();
		real8 range = endvalue - startvalue;
		if (flags & F_SHOW_BORDER && (MClook == LF_MOTIF || !(flags & F_SCALE)
		                              || getstyleint(flags) == F_VERTICAL))
		{
			if (IsMacEmulatedLF())
				trect = MCU_reduce_rect(trect, 1);
			else
				trect = MCU_reduce_rect(trect, borderwidth);
		}
		if (getstyleint(flags) == F_VERTICAL)
		{
			if (flags & F_SCALE || (thumbsize != 0 && rect.height > rect.width * 3))
			{
				thumb.x = trect.x;
				thumb.width = trect.width;
				if (flags & F_SCALE)
				{
					real8 height = trect.height - MOTIF_SCALE_THUMB_SIZE;
					real8 offset = height * (pos - startvalue);
					thumb.y = trect.y;
					if (range != 0.0)
						thumb.y += (int2)(offset / range);
					thumb.height = MOTIF_SCALE_THUMB_SIZE;
				}
				else
					if (MCproportionalthumbs)
					{
						thumb.y = trect.y + trect.width;
						real8 height = trect.height - (trect.width << 1) + 1;
						if (range != 0.0 && fabs(endvalue - startvalue) != thumbsize)
						{
							int2 miny = thumb.y;
							real8 offset = height * (pos - startvalue);
							thumb.y += (int2)(offset / range);
							range = fabs(range);
							thumb.height = (uint2)(thumbsize * height / range);
							uint2 minsize = IsMacEmulatedLF() ? trect.width : MIN_THUMB_SIZE;
							if (thumb.height < minsize)
							{
								uint2 diff = minsize - thumb.height;
								thumb.height = minsize;
								thumb.y -= (int2)(diff * (pos + thumbsize - startvalue) / range);
								if (thumb.y < miny)
									thumb.y = miny;
							}
						}
						else
							thumb.height = (int2)height - 1;
					}
					else
					{
						real8 height = trect.height - (trect.width << 1) - FIXED_THUMB_SIZE;
						real8 offset = height * (pos - startvalue);
						if (range < 0)
							range += thumbsize;
						else
							range -= thumbsize;
						thumb.y = trect.y + trect.width;
						if (range != 0.0)
							thumb.y += (int2)(offset / range);
						thumb.height = FIXED_THUMB_SIZE;
					}
			}
			else
				thumb.height = 0;
		}
		else
		{ // horizontal
			if (flags & F_SCALE || (thumbsize != 0 && rect.width > rect.height * 3))
			{
				thumb.y = trect.y;
				thumb.height = trect.height;
				if (flags & F_SCALE)
				{
					switch (MClook)
					{
					case LF_MOTIF:
						thumb.width = MOTIF_SCALE_THUMB_SIZE;
						break;
					case LF_MAC:
						thumb.width = MAC_SCALE_THUMB_SIZE;
						thumb.height = 16;
						trect.x += 7;
						trect.width -= 11;
						break;
					default:
						// Win95's thumb width is computed, varied depends on the height of the rect
						// if rect.height < 21, scale down the width by a scale factor
						// WIN_SCALE_THUMB_HEIGHT + space + 4(tic mark)
						if (trect.height < WIN_SCALE_HEIGHT)
							thumb.width = trect.height
							              * WIN_SCALE_THUMB_WIDTH / WIN_SCALE_HEIGHT;
						else
							thumb.width = WIN_SCALE_THUMB_WIDTH;
						thumb.height = MCU_min(WIN_SCALE_HEIGHT, trect.height) - 6;
					}
					real8 width = trect.width - thumb.width;
					real8 offset = width * (pos - startvalue);
					thumb.x = trect.x;
					if (range != 0.0)
						thumb.x += (int2)(offset / range);
				}
				else
					if (MCproportionalthumbs)
					{
						thumb.x = trect.x + trect.height;
						real8 width = trect.width - (trect.height << 1) + 1;
						if (range != 0.0 && fabs(endvalue - startvalue) != thumbsize)
						{
							int2 minx = thumb.x;
							real8 offset = width * (pos - startvalue);
							thumb.x += (int2)(offset / range);
							range = fabs(range);
							thumb.width = (uint2)(thumbsize * width / range);
							uint2 minsize = IsMacEmulatedLF() ? trect.height : MIN_THUMB_SIZE;
							if (thumb.width < minsize)
							{
								uint2 diff = minsize - thumb.width;
								thumb.width = minsize;
								thumb.x -= (int2)(diff * (pos + thumbsize - startvalue) / range);
								if (thumb.x < minx)
									thumb.x = minx;
							}
						}
						else
							thumb.width = (int2)width - 1;
					}
					else
					{
						real8 width = trect.width - (trect.height << 1) - FIXED_THUMB_SIZE;
						real8 offset = width * (pos - startvalue);
						thumb.x = trect.x + trect.height;
						if (range < 0)
							range += thumbsize;
						else
							range -= thumbsize;
						if (range != 0.0)
							thumb.x += (int2)(offset / range);
						thumb.width = FIXED_THUMB_SIZE;
					}
			}
			else
				thumb.width = 0;
		}
	}
	return thumb;
}

void MCScrollbar::update(real8 newpos, MCNameRef mess)
{
	real8 oldpos = thumbpos;
	real8 ts = thumbsize;
	if (thumbsize > fabs(endvalue - startvalue))
		ts = thumbsize = fabs(endvalue - startvalue);
	if (flags & F_SB_STYLE)
		ts = 0;
	if (startvalue < endvalue)
		if (newpos < startvalue)
			thumbpos = startvalue;
		else
			if (newpos + ts > endvalue)
				thumbpos = endvalue - ts;
			else
				thumbpos = newpos;
	else
		if (newpos > startvalue)
			thumbpos = startvalue;
		else
			if (newpos - ts < endvalue)
				thumbpos = endvalue + ts;
			else
				thumbpos = newpos;
	
	if (thumbpos != oldpos)
		signallisteners(P_THUMB_POS);
	
	if ((thumbpos != oldpos || mode == SM_LINEDEC || mode == SM_LINEINC)
	        && opened && (flags & F_VISIBLE || showinvisible()))
	{
		if (thumbpos != oldpos)
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			redrawall();
		}
		MCAutoStringRef t_data;
		MCU_r8tos(thumbpos, nffw, nftrailing, nfforce, &t_data);
		switch (message_with_valueref_args(mess, *t_data))
		{
		case ES_NOT_HANDLED:
		case ES_PASS:
			if (!MCNameIsEqualToCaseless(mess, MCM_scrollbar_drag))
				message_with_valueref_args(MCM_scrollbar_drag, *t_data);
		default:
			break;
		}

		if (linked_control != NULL)
			linked_control -> readscrollbars();
	}
}

void MCScrollbar::getthumb(real8 &pos)
{
	pos = thumbpos;
}

void MCScrollbar::setthumb(real8 pos, real8 size, real8 linc, real8 ev)
{
	thumbpos = pos;
	thumbsize = size;
	pageinc = size;
	lineinc = linc;
	endvalue = ev;
	
	// MW-2008-11-02: [[ Bug ]] This method is only used by scrollbars directly
	//   attached to controls. In this case, they are created from the templateScrollbar
	//   and this means 'startValue' could be anything - however we need it to be zero
	//   if we want to avoid strangeness.
	startvalue = 0.0;
}

void MCScrollbar::movethumb(real8 pos)
{
	if (thumbpos != pos)
	{
		thumbpos = pos;
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		redrawall();
	}
}

void MCScrollbar::setborderwidth(uint1 nw)
{
	int2 d = nw - borderwidth;
	if (rect.width <= rect.height)
	{
		rect.width += d << 1;
		rect.x -= d;
	}
	else
	{
		rect.height += d << 1;
		rect.y -= d;
	}
	borderwidth = nw;
}

void MCScrollbar::reset()
{
	flags &= ~F_HAS_VALUES;
	startvalue = 0.0;
	endvalue = 65535.0;
	// MW-2013-08-27: [[ UnicodifyScrollbar ]] Reset the string values to the empty string.
	MCValueAssign(startstring, kMCEmptyString);
	MCValueAssign(endstring, kMCEmptyString);
}

void MCScrollbar::redrawarrow(uint2 oldmode)
{
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	redrawall();
}

bool MCScrollbar::issbdisabled(void) const
{
	bool ret; 
	ret = getflag(F_DISABLED) || (MClook != LF_MOTIF && !(flags & F_SB_STYLE) && fabs(endvalue - startvalue) == thumbsize);
	return ret;
}

void MCScrollbar::link(MCControl *p_control)
{
	linked_control = p_control;
}

// MW-2012-09-20: [[ Bug 10395 ]] This method is invoked rather than layer_redrawall()
//   to ensure that in the embedded case, the parent's layer is updated.
void MCScrollbar::redrawall(void)
{
	if (!m_embedded)
	{
		layer_redrawall();
		return;
	}
	
	parent.GetAs<MCControl>()->layer_redrawrect(getrect());
}

// MW-2012-09-20: [[ Bug 10395 ]] This method marks the control as embedded
//   thus causing it to redraw through its parent.
void MCScrollbar::setembedded(void)
{
	m_embedded = true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCScrollbar::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCScrollbar::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCScrollbar::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_SCROLLBAR, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCControl::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return stat;
	if (flags & F_SAVE_ATTS)
	{
		real8 range = endvalue - startvalue;
		if (range != 0.0)
			range = 65535.0 / range;
		uint2 i2 = (uint2)((thumbpos - startvalue) * range);
		if ((stat = IO_write_uint2(i2, stream)) != IO_NORMAL)
			return stat;
		i2 = (uint2)(thumbsize * range);
		if ((stat = IO_write_uint2(i2, stream)) != IO_NORMAL)
			return stat;
		i2 = (uint2)(lineinc * range);
		if ((stat = IO_write_uint2(i2, stream)) != IO_NORMAL)
			return stat;
		i2 = (uint2)(pageinc * range);
		if ((stat = IO_write_uint2(i2, stream)) != IO_NORMAL)
			return stat;
		if (flags & F_HAS_VALUES)
		{
            // MW-2013-08-27: [[ UnicodifyScrollbar ]] Update to use stringref primitives.
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
			if ((stat = IO_write_stringref_new(startstring, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
				return stat;
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
            if ((stat = IO_write_stringref_new(endstring, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_uint2(nffw, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_uint2(nftrailing, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_uint2(nfforce, stream)) != IO_NORMAL)
				return stat;
		}
	}
	return savepropsets(stream, p_version);
}

IO_stat MCScrollbar::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	if (flags & F_SAVE_ATTS)
	{
		uint2 i2;
		if ((stat = IO_read_uint2(&i2, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		thumbpos = (real8)i2;
		if ((stat = IO_read_uint2(&i2, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		thumbsize = (real8)i2;
		if ((stat = IO_read_uint2(&i2, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		lineinc = (real8)i2;
		if ((stat = IO_read_uint2(&i2, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		pageinc = (real8)i2;
		if (flags & F_HAS_VALUES)
		{
			// MW-2013-08-27: [[ UnicodifyScrollbar ]] Update to use stringref primitives.
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
			if ((stat = IO_read_stringref_new(startstring, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
				return checkloadstat(stat);
			if (!MCStringToDouble(startstring, startvalue))
				startvalue = 0.0;
			
			// MW-2013-08-27: [[ UnicodifyScrollbar ]] Update to use stringref primitives.
			// MW-2013-11-20: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
			if ((stat = IO_read_stringref_new(endstring, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
				return checkloadstat(stat);
			if (!MCStringToDouble(endstring, endvalue))
				endvalue = 0.0;
			
			real8 range = (endvalue - startvalue) / 65535.0;
			thumbpos = thumbpos * range + startvalue;
			thumbsize *= range;
			lineinc *= range;
			pageinc *= range;
			if ((stat = IO_read_uint2(&nffw, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			if ((stat = IO_read_uint2(&nftrailing, stream)) != IO_NORMAL)
				return checkloadstat(stat);
			if ((stat = IO_read_uint2(&nfforce, stream)) != IO_NORMAL)
				return checkloadstat(stat);
		}
	}
	if (version <= kMCStackFileFormatVersion_2_0)
	{
		if (flags & F_TRAVERSAL_ON)
			rect = MCU_reduce_rect(rect, MCfocuswidth);
		if (flags & F_SHOW_VALUE && getstyleint(flags) == F_HORIZONTAL)
			rect = MCU_reduce_rect(rect, 4);
    }
	return loadpropsets(stream, version);
}
