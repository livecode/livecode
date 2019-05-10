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
#include "tilecache.h"
#include "scrolbar.h"

#include "tooltip.h"
#include "bitmapeffect.h"
#include "bitmapeffectblur.h"

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
		if (hscroll(scroll - tx, True) == ES_ERROR)
			ctxt . Throw();
		resetscrollbars(True);
	}
}

void MCControl::DoSetVScroll(MCExecContext& ctxt, int4 ty, integer_t scroll)
{
	if (opened)
	{
		if (vscroll(scroll - ty, True) == ES_ERROR)
			ctxt . Throw();
		resetscrollbars(True);
	}	
}

void MCControl::DoSetHScrollbar(MCExecContext& ctxt, MCScrollbar*& hsb, uint2& sbw)
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
	
	if (focused.IsBoundTo(this))
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

void MCControl::GetLayerClipRect(MCExecContext& ctxt, MCRectangle*& r_layer_clip_rect)
{
    if (m_layer_has_clip_rect)
    {
        *r_layer_clip_rect = m_layer_clip_rect;
    }
    else
    {
        r_layer_clip_rect = nullptr;
    }
}

void MCControl::SetLayerClipRect(MCExecContext& ctxt, MCRectangle* p_layer_clip_rect)
{
    bool t_old_has_layer_clip_rect = m_layer_has_clip_rect;
    MCRectangle t_old_layer_clip_rect = m_layer_clip_rect;
    
    bool t_redraw = false;
    if (p_layer_clip_rect != nullptr)
    {
        m_layer_clip_rect = *p_layer_clip_rect;
        m_layer_has_clip_rect = true;
        
        if (!t_old_has_layer_clip_rect ||
                MCU_equal_rect(m_layer_clip_rect, t_old_layer_clip_rect))
        {
            t_redraw = true;
        }
    }
    else
    {
        m_layer_has_clip_rect = false;
        if (t_old_has_layer_clip_rect)
        {
            t_redraw = true;
        }
    }
    
    if (t_redraw)
    {
        Redraw();
    }
}

void MCControl::SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins)
{
    if (p_margins . type == kMCInterfaceMarginsTypeSingle)
        leftmargin = rightmargin = topmargin = bottommargin = p_margins . margin;
    else
    {
        leftmargin = p_margins . margins[0];
        topmargin = p_margins . margins[1];
        rightmargin = p_margins . margins[2];
        bottommargin = p_margins . margins[3];
    }
    Redraw();
}

void MCControl::GetMargins(MCExecContext& ctxt, MCInterfaceMargins& r_margins)
{
    if (leftmargin == rightmargin && leftmargin == topmargin && leftmargin == bottommargin)
    {
        r_margins . type = kMCInterfaceMarginsTypeSingle;
        r_margins . margin = leftmargin;
    }
    else
    {
        r_margins . type = kMCInterfaceMarginsTypeQuadruple;
        r_margins . margins[0] = leftmargin;
        r_margins . margins[1] = topmargin;
        r_margins . margins[2] = rightmargin;
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

static MCExecEnumTypeElementInfo _kMCInterfaceBitmapEffectBlendModeElementInfo[] =
{
	{ "normal", kMCBitmapEffectBlendModeNormal, false },
	{ "multiply", kMCBitmapEffectBlendModeMultiply, false },
	{ "colordodge", kMCBitmapEffectBlendModeColorDodge, false },
};

static MCExecEnumTypeInfo _kMCInterfaceBitmapEffectBlendModeTypeInfo =
{
	"Interface.BitmapEffectBlendMode",
	sizeof(_kMCInterfaceBitmapEffectBlendModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceBitmapEffectBlendModeElementInfo
};

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCInterfaceBitmapEffectFilterElementInfo[] =
{
	{ "gaussian", kMCBitmapEffectFilterFastGaussian, false },
	{ "box1pass", kMCBitmapEffectFilterOnePassBox, false },
	{ "box2pass", kMCBitmapEffectFilterTwoPassBox, false },
	{ "box3pass", kMCBitmapEffectFilterThreePassBox, false },
};

static MCExecEnumTypeInfo _kMCInterfaceBitmapEffectFilterTypeInfo =
{
	"Interface.BitmapEffectFilter",
	sizeof(_kMCInterfaceBitmapEffectFilterElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceBitmapEffectFilterElementInfo
};

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCInterfaceBitmapEffectSourceElementInfo[] =
{
	{ "edge", kMCBitmapEffectSourceEdge, false },
	{ "center", kMCBitmapEffectSourceCenter, false },
};

static MCExecEnumTypeInfo _kMCInterfaceBitmapEffectSourceTypeInfo =
{
	"Interface.BitmapEffectSource",
	sizeof(_kMCInterfaceBitmapEffectSourceElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceBitmapEffectSourceElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceBitmapEffectBlendModeTypeInfo = &_kMCInterfaceBitmapEffectBlendModeTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceBitmapEffectFilterTypeInfo = &_kMCInterfaceBitmapEffectFilterTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceBitmapEffectSourceTypeInfo = &_kMCInterfaceBitmapEffectSourceTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCControl::EffectRedraw(MCRectangle p_old_rect)
{
    // MW-2011-09-21: [[ Layers ]] Mark the attrs as needing redrawn.
    m_layer_attr_changed = true;
            
    // MW-2011-08-17: [[ Layers ]] Make sure any redraw needed due to effects
    //   changing occur.
    layer_effectschanged(p_old_rect);
}

void MCControl::GetDropShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value)
{
    if (MCBitmapEffectsGetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_DROP_SHADOW, r_value))
       return;
    
    ctxt . Throw();
}

void MCControl::SetDropShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value)
{
    bool t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_DROP_SHADOW, p_value, t_dirty))
    {
        if (t_dirty)
            EffectRedraw(t_old_effective_rect);
        return;
    }
    
    ctxt . Throw();
}

void MCControl::GetInnerShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value)
{
    if (MCBitmapEffectsGetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_INNER_SHADOW, r_value))
        return;
    
    ctxt . Throw();
}

void MCControl::SetInnerShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value)
{
    bool t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_INNER_SHADOW, p_value, t_dirty))
    {
        if (t_dirty)
            EffectRedraw(t_old_effective_rect);
        return;
    }
    
    ctxt . Throw();
}

void MCControl::GetInnerGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value)
{
    if (MCBitmapEffectsGetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_INNER_GLOW, r_value))
        return;
    
    ctxt . Throw();
}

void MCControl::SetInnerGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value)
{
    bool t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_INNER_GLOW, p_value, t_dirty))
    {
        if (t_dirty)
            EffectRedraw(t_old_effective_rect);
        return;
    }
    
    ctxt . Throw();
}

void MCControl::GetOuterGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value)
{
    if (MCBitmapEffectsGetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_OUTER_GLOW, r_value))
        return;
    
    ctxt . Throw();
}

void MCControl::SetOuterGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value)
{
    bool t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_OUTER_GLOW, p_value, t_dirty))
    {
        if (t_dirty)
            EffectRedraw(t_old_effective_rect);
        return;
    }
    
    ctxt . Throw();
}

void MCControl::GetColorOverlayProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value)
{
    if (MCBitmapEffectsGetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_COLOR_OVERLAY, r_value))
        return;
    
    ctxt . Throw();
}

void MCControl::SetColorOverlayProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value)
{
    bool t_dirty = false;
    MCRectangle t_old_effective_rect = geteffectiverect();
    
    if (MCBitmapEffectsSetProperty(ctxt, m_bitmap_effects, index, P_BITMAP_EFFECT_COLOR_OVERLAY, p_value, t_dirty))
    {
        if (t_dirty)
            EffectRedraw(t_old_effective_rect);
        return;
    }
    
    ctxt . Throw();
}

