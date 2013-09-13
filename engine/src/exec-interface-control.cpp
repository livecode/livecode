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
#include "bitmapeffect.h"

#include "exec-interface.h"

//////////

static void MCInterfaceMarginsParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceMargins& r_output)
{
    if (MCU_stoi2(p_input, r_output . margin))
    {
        r_output . type = kMCInterfaceMarginsTypeSingle;
        return;
    }

    if (MCU_stoi2x4(p_input, r_output . margins[0], r_output . margins[1], r_output . margins[2], r_output . margins[3]))
    {
        r_output . type = kMCInterfaceMarginsTypeQuadruple;
        return;
    }
    
    ctxt . LegacyThrow(EE_OBJECT_MARGINNAN);
}

static void MCInterfaceMarginsFormat(MCExecContext& ctxt, const MCInterfaceMargins& p_input, MCStringRef& r_output)
{
    bool t_success;
    
    if (p_input . type == kMCInterfaceMarginsTypeSingle)
        t_success = MCStringFormat(r_output, "%d", p_input . margin);
    else
        t_success = MCStringFormat(r_output, "%d,%d,%d,%d", p_input . margins[0], p_input . margins[1], p_input . margins[2], p_input . margins[3]);

    if (t_success)
        return;
    
    ctxt . Throw();
}

static void MCInterfaceMarginsFree(MCExecContext& ctxt, MCInterfaceMargins& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceMarginsTypeInfo =
{
	"Interface.Margins",
	sizeof(MCInterfaceMargins),
	(void *)MCInterfaceMarginsParse,
	(void *)MCInterfaceMarginsFormat,
	(void *)MCInterfaceMarginsFree
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceLayerModeElementInfo[] =
{
	{ "static", kMCLayerModeHintStatic },
    { "dynamic", kMCLayerModeHintDynamic },
	{ "scrolling", kMCLayerModeHintScrolling },
	{ "container", kMCLayerModeHintContainer }
};

static MCExecEnumTypeInfo _kMCInterfaceLayerModeTypeInfo =
{
	"Interface.LayerMode",
	sizeof(_kMCInterfaceLayerModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceLayerModeElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecCustomTypeInfo *kMCInterfaceMarginsTypeInfo = &_kMCInterfaceMarginsTypeInfo;
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

void MCControl::SetToolTip(MCExecContext& ctxt, MCStringRef p_tooltip)
{
	if (MCStringIsEqualTo(tooltip, p_tooltip, kMCStringOptionCompareExact))
		return;
	
	MCValueAssign(tooltip, p_tooltip);
	
	if (focused == this)
		MCtooltip->settip(tooltip);
}

void MCControl::GetToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip)
{
	r_tooltip = MCValueRetain(tooltip);
}

void MCControl::GetUnicodeToolTip(MCExecContext& ctxt, MCDataRef& r_tooltip)
{
	// Convert the tooltip string into UTF-16 data
	MCDataRef t_tooltip = nil;
	if (MCStringEncode(tooltip, kMCStringEncodingUTF16, false, t_tooltip))
	{
		r_tooltip = t_tooltip;
		return;
	}
	
	ctxt.Throw();
}

void MCControl::SetUnicodeToolTip(MCExecContext& ctxt, MCDataRef p_tooltip)
{
	// Convert the supplied UTF-16 data into a string
	MCStringRef t_tooltip = nil;
	if (MCStringDecode(p_tooltip, kMCStringEncodingUTF16, false, t_tooltip))
	{
		MCValueAssign(tooltip, t_tooltip);
		return;
	}
	
	ctxt.Throw();
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

void MCControl::SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins)
{
    if (p_margins . type == kMCInterfaceMarginsTypeSingle)
        leftmargin = rightmargin = topmargin = bottommargin = p_margins . margin;
    else
    {
        leftmargin = p_margins . margins[0];
        rightmargin = p_margins . margins[1];
        topmargin = p_margins . margins[2];
        bottommargin = p_margins . margins[3];
    }
    Redraw();
}

void MCControl::GetMargins(MCExecContext& ctxt, MCInterfaceMargins& r_margins)
{
    if (leftmargin == rightmargin == topmargin == bottommargin)
    {
        r_margins . type = kMCInterfaceMarginsTypeSingle;
        r_margins . margin = leftmargin;
    }
    else
    {
        r_margins . type = kMCInterfaceMarginsTypeQuadruple;
        r_margins . margins[0] = leftmargin;
        r_margins . margins[1] = rightmargin;
        r_margins . margins[2] = topmargin;
        r_margins . margins[3] = bottommargin;
    }
}

void MCControl::SetInk(MCExecContext& ctxt, intenum_t ink)
{
    MCObject::SetInk(ctxt, ink);
    m_layer_attr_changed = true;
}

void MCControl::SetShowBorder(MCExecContext& ctxt, bool setting)
{
    MCObject::SetShowBorder(ctxt, setting);
    m_layer_attr_changed = true;
}

void MCControl::SetShowFocusBorder(MCExecContext& ctxt, bool setting)
{
    MCObject::SetShowFocusBorder(ctxt, setting);
    m_layer_attr_changed = true;
}

void MCControl::SetOpaque(MCExecContext& ctxt, bool setting)
{
    MCObject::SetOpaque(ctxt, setting);
    m_layer_attr_changed = true;
}

void MCControl::SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow)
{
    MCObject::SetShadow(ctxt, p_shadow);
    m_layer_attr_changed = true;
}

////////////////////////////////////////////////////////////////////////////////

void MCControl::DoGetBitmapEffectArray(MCExecContext& ctxt, Properties which, MCArrayRef& r_array)
{
    if (MCBitmapEffectsGetProperties(m_bitmap_effects, which, r_array))
        return;
    
    ctxt . Throw();
}

void MCControl::DoSetBitmapEffectArray(MCExecContext& ctxt, Properties which, MCArrayRef p_array)
{
    bool t_dirty;
    t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperties(m_bitmap_effects, which, p_array, t_dirty))
    {
        if (t_dirty)
        {
			// MW-2011-09-21: [[ Layers ]] Mark the attrs as needing redrawn.
			m_layer_attr_changed = true;
            
			// MW-2011-08-17: [[ Layers ]] Make sure any redraw needed due to effects
			//   changing occur.
			layer_effectschanged(t_old_effective_rect);
        }
        return;
    }
    
    ctxt . Throw();
}

void MCControl::DoGetBitmapEffectElement(MCExecContext& ctxt, Properties which, MCNameRef p_prop, MCValueRef& r_value)
{
    if (MCBitmapEffectsGetPropertyElement(m_bitmap_effects, which, p_prop, r_value))
    {
        if (r_value == nil)
            r_value = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}

void MCControl::DoSetBitmapEffectElement(MCExecContext& ctxt, Properties which, MCNameRef p_prop, MCValueRef p_value)
{
    bool t_dirty;
    t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetPropertyElement(m_bitmap_effects, which, p_value, p_prop, t_dirty))
    {
        if (t_dirty)
        {
			// MW-2011-09-21: [[ Layers ]] Mark the attrs as needing redrawn.
			m_layer_attr_changed = true;
            
			// MW-2011-08-17: [[ Layers ]] Make sure any redraw needed due to effects
			//   changing occur.
			layer_effectschanged(t_old_effective_rect);
        }
        return;
    }
    
    ctxt . Throw();
}

void MCControl::GetDropShadow(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_DROP_SHADOW, r_array);
}

void MCControl::SetDropShadow(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_DROP_SHADOW, p_array);
}

void MCControl::GetDropShadowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_DROP_SHADOW, p_prop, r_value);
}

void MCControl::SetDropShadowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_DROP_SHADOW, p_prop, p_value);
}

void MCControl::GetInnerShadow(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_INNER_SHADOW, r_array);
}

void MCControl::SetInnerShadow(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_INNER_SHADOW, p_array);
}

void MCControl::GetInnerShadowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_INNER_SHADOW, p_prop, r_value);
}

void MCControl::SetInnerShadowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_INNER_SHADOW, p_prop, p_value);
}

void MCControl::GetOuterGlow(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_OUTER_GLOW, r_array);
}

void MCControl::SetOuterGlow(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_OUTER_GLOW, p_array);
}

void MCControl::GetOuterGlowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_OUTER_GLOW, p_prop, r_value);
}

void MCControl::SetOuterGlowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_OUTER_GLOW, p_prop, p_value);
}

void MCControl::GetInnerGlow(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_INNER_GLOW, r_array);
}

void MCControl::SetInnerGlow(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_INNER_GLOW, p_array);
}

void MCControl::GetInnerGlowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_INNER_GLOW, p_prop, r_value);
}

void MCControl::SetInnerGlowElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_INNER_GLOW, p_prop, p_value);
}

void MCControl::GetColorOverlay(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_COLOR_OVERLAY, r_array);
}

void MCControl::SetColorOverlay(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetBitmapEffectArray(ctxt, P_BITMAP_EFFECT_COLOR_OVERLAY, p_array);
}

void MCControl::GetColorOverlayElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_COLOR_OVERLAY, p_prop, r_value);
}

void MCControl::SetColorOverlayElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetBitmapEffectElement(ctxt, P_BITMAP_EFFECT_COLOR_OVERLAY, p_prop, p_value);
}
