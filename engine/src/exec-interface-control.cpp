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
#include "tilecache.h"
#include "scrolbar.h"

#include "tooltip.h"

#include "exec-interface.h"

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceLayerModeElementInfo[] =
{	
	{ "static", kMCLayerModeHintStatic, false },
	{ "dynamic", kMCLayerModeHintDynamic, false },
	{ "scrolling", kMCLayerModeHintScrolling, false },
	{ "container", kMCLayerModeHintContainer, false },
};

static MCExecEnumTypeInfo _kMCInterfaceLayerModeTypeInfo =
{
	"Interface.LayerMode",
	sizeof(_kMCInterfaceLayerModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceLayerModeElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceLayerModeTypeInfo = &_kMCInterfaceLayerModeTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCControl::Redraw(void)
{
	if (!opened)
		return;
	
	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCControl::DoSetHScroll(MCExecContext& ctxt, int4 tx, integer_t scroll)
{
	if (opened)
	{
		if (hscroll(scroll - tx, True) != ES_NORMAL)
			ctxt . Throw();
		resetscrollbars(True);
	}
}

void MCControl::DoSetVScroll(MCExecContext& ctxt, int4 ty, integer_t scroll)
{
	if (opened)
	{
		if (vscroll(scroll - ty, True) != ES_NORMAL)
			ctxt . Throw();
		resetscrollbars(True);
	}	
}

void MCControl::DoSetHScrollbar(MCExecContext& ctxt, MCScrollbar*& hsb, uint2& sbw)
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
			sbw = trect . height;
			trect.width = trect . height + 1; // make sure SB is horizontal
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
void MCControl::DoSetVScrollbar(MCExecContext& ctxt, MCScrollbar*& vsb, uint2& sbw)
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

void MCControl::DoSetScrollbarWidth(MCExecContext& ctxt, uint2& sbw, uinteger_t p_width)
{
	sbw = p_width;
	setsbrects();
}

////////////////////////////////////////////////////////////////////////////////

void MCControl::GetLeftMargin(MCExecContext& ctxt, integer_t& r_margin)
{
	r_margin = leftmargin;
}

void MCControl::SetLeftMargin(MCExecContext& ctxt, integer_t p_margin)
{
	leftmargin = p_margin;
	Redraw();
}

void MCControl::GetRightMargin(MCExecContext& ctxt, integer_t& r_margin)
{
	r_margin = rightmargin;
}

void MCControl::SetRightMargin(MCExecContext& ctxt, integer_t p_margin)
{
	rightmargin = p_margin;
	Redraw();
}

void MCControl::GetTopMargin(MCExecContext& ctxt, integer_t& r_margin)
{
	r_margin = topmargin;
}

void MCControl::SetTopMargin(MCExecContext& ctxt, integer_t p_margin)
{
	topmargin = p_margin;
	Redraw();
}

void MCControl::GetBottomMargin(MCExecContext& ctxt, integer_t& r_margin)
{
	r_margin = bottommargin;
}

void MCControl::SetBottomMargin(MCExecContext& ctxt, integer_t p_margin)
{
	bottommargin = p_margin;
	Redraw();
}

void MCControl::SetToolTip(MCExecContext& ctxt, MCStringRef p_tooltip, bool is_unicode)
{
	bool t_dirty;
	t_dirty = true;

	bool t_success;
	t_success = true;

	if (tooltip != MCtooltip->gettip())
		t_dirty = false;
	delete tooltip;

	if (p_tooltip == nil)
		tooltip = nil;
	else
	{
		// MW-2012-03-13: [[ UnicodeToolTip ]] Convert the value to UTF-8 - either
		//   from native or UTF-16 depending on what prop was set.

		MCAutoStringRef t_tooltip;
		
		if (is_unicode)
			t_success = MCU_unicodetomultibyte(p_tooltip, LCH_UTF8, &t_tooltip);
		else
			t_success = MCU_nativetoutf8(p_tooltip, &t_tooltip);

		if (t_success)
		{
			tooltip = strclone(MCStringGetCString(*t_tooltip));
			if (t_dirty && focused == this)
			{
				MCtooltip->settip(tooltip);
				t_dirty = false;
			}
		}
	}

	if (t_success)
	{
		if (t_dirty)
			Redraw();
		return;
	}

	ctxt . Throw();
}

void MCControl::GetToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip)
{
	if (tooltip == nil)
		return;

	MCAutoStringRef t_tooltip;
	if (MCStringCreateWithCString(tooltip, &t_tooltip) && MCU_utf8tonative(*t_tooltip, r_tooltip))
		return;
	
	ctxt . Throw();
}

void MCControl::SetToolTip(MCExecContext& ctxt, MCStringRef p_tooltip)
{
	SetToolTip(ctxt, p_tooltip, false);
}

void MCControl::GetUnicodeToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip)
{
	if (tooltip == nil)
		return;

	MCAutoStringRef t_tooltip;
	if (MCStringCreateWithCString(tooltip, &t_tooltip) && MCU_multibytetounicode(*t_tooltip, LCH_UTF8, r_tooltip))
		return;
	
	ctxt . Throw();
}

void MCControl::SetUnicodeToolTip(MCExecContext& ctxt, MCStringRef p_tooltip)
{
	SetToolTip(ctxt, p_tooltip, true);
}

void MCControl::GetLayerMode(MCExecContext& ctxt, intenum_t& r_mode)
{
	r_mode = (intenum_t)m_layer_mode_hint;
}

void MCControl::SetLayerMode(MCExecContext& ctxt, intenum_t p_mode)
{
	// MW-2011-08-25: [[ TileCache ]] Handle layerMode property store.
	// MW-2011-09-21: [[ Layers ]] Updated to use new layermode hint system.

	MCLayerModeHint t_mode = (MCLayerModeHint)p_mode;

#if !NOT_YET_IMPLEMENTED
	if (t_mode == kMCLayerModeHintContainer)
	{
		ctxt . LegacyThrow(EE_CONTROL_BADLAYERMODE);
		return;
	}
#endif

	// If the layer mode hint has changed, update and mark the attrs
	// for recompute. If the hint hasn't changed, then there's no need
	// to redraw.
	if (t_mode != m_layer_mode_hint)
	{
		m_layer_attr_changed = true;
		m_layer_mode_hint = t_mode;
		Redraw();
	}
}

void MCControl::GetEffectiveLayerMode(MCExecContext& ctxt, intenum_t& r_mode)
{
	r_mode = (intenum_t)layer_geteffectivemode();
}
