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
#include "graphic.h"

#include "exec-interface.h"

//////////

enum MCInterfaceGraphicStyle
{
	kMCGraphicStyleRectangle = F_G_RECTANGLE,
	kMCGraphicStyleRoundrect = F_ROUNDRECT,
	kMCGraphicStylePolygon = F_POLYGON,
	kMCGraphicStyleCurve = F_CURVE,
	kMCGraphicStyleOval = F_OVAL,
	kMCGraphicStyleRegular = F_REGULAR,
	kMCGraphicStyleLine = F_LINE,
	kMCGraphicStyleText,
	kMCGraphicStyleArc,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGraphicFillRuleElementInfo[] =
{	
	{ "none", kMCFillRuleNone, false },
	{ "nonzero", kMCFillRuleNonZero, false },
	{ "evenodd", kMCFillRuleEvenOdd, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGraphicFillRuleTypeInfo =
{
	"Interface.GraphicFillRule",
	sizeof(_kMCInterfaceGraphicFillRuleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGraphicFillRuleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGraphicEditModeElementInfo[] =
{	
	{ "none", kMCEditModeNone, false },
	{ "fillgradient", kMCEditModeFillGradient, false },
	{ "strokegradient", kMCEditModeStrokeGradient, false },
	{ "polygon", kMCEditModePolygon, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGraphicEditModeTypeInfo =
{
	"Interface.GraphicEditMode",
	sizeof(_kMCInterfaceGraphicEditModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGraphicEditModeElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGraphicCapStyleElementInfo[] =
{	
	{ "round", CapRound, false },
	{ "square", CapProjecting, false },
	{ "butt", CapButt, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGraphicCapStyleTypeInfo =
{
	"Interface.GraphicCapStyle",
	sizeof(_kMCInterfaceGraphicCapStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGraphicCapStyleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGraphicJoinStyleElementInfo[] =
{	
	{ "round", JoinRound, false },
	{ "bevel", JoinBevel, false },
	{ "miter", JoinMiter, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGraphicJoinStyleTypeInfo =
{
	"Interface.GraphicJoinStyle",
	sizeof(_kMCInterfaceGraphicJoinStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGraphicJoinStyleElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGraphicStyleElementInfo[] =
{	
	{ MCrectanglestring, kMCGraphicStyleRectangle, false },
	{ MCroundrectstring, kMCGraphicStyleRoundrect, false },
	{ MCpolygonstring, kMCGraphicStylePolygon, false },
	{ MCcurvestring, kMCGraphicStyleCurve, false },
	{ MCovalstring, kMCGraphicStyleOval, false },
	{ MCregularstring, kMCGraphicStyleRegular, false },
	{ MClinestring, kMCGraphicStyleLine, false },
	{ MCtextstring, kMCGraphicStyleText, false },
	{ MCarcstring, kMCGraphicStyleArc, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGraphicStyleTypeInfo =
{
	"Interface.GraphicStyle",
	sizeof(_kMCInterfaceGraphicStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGraphicStyleElementInfo
};


////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceGraphicFillRuleTypeInfo = &_kMCInterfaceGraphicFillRuleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceGraphicEditModeTypeInfo = &_kMCInterfaceGraphicEditModeTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceGraphicCapStyleTypeInfo = &_kMCInterfaceGraphicCapStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceGraphicJoinStyleTypeInfo = &_kMCInterfaceGraphicJoinStyleTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceGraphicStyleTypeInfo = &_kMCInterfaceGraphicStyleTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCGraphic::Redraw(MCRectangle drect)
{
	if (rect.x != drect.x || rect.y != drect.y || 
		rect.width != drect.width || rect.height != drect.height)
    {
        /* If the rect has changed, then notify the change to the selection
         * layer. */
        if (opened && (getselected() || m_edit_tool != nullptr))
        {
            getcard()->dirtyselection(drect);
            getcard()->dirtyselection(rect);
        }
        
        if (resizeparent())
            return;
    }
    
	if (!opened)
		return;

	// MW-2011-08-18: [[ Layers ]] Notify of rect changed and invalidate.
	layer_rectchanged(drect, true);
}

void MCGraphic::Redraw()
{
	Redraw(rect);
}

////////////////////////////////////////////////////////////////////////////////

void MCGraphic::GetAntiAliased(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_G_ANTI_ALIASED);
}

void MCGraphic::SetAntiAliased(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_G_ANTI_ALIASED))
		Redraw();
}

void MCGraphic::GetFillRule(MCExecContext& ctxt, intenum_t& r_rule)
{
	r_rule = getfillrule();
}

void MCGraphic::SetFillRule(MCExecContext& ctxt, intenum_t rule)
{
	uint1 t_new_fill_rule;
	t_new_fill_rule = (uint1)rule;

	if (t_new_fill_rule != getfillrule())
	{
		setfillrule(t_new_fill_rule);
		Redraw();
	}
}

void MCGraphic::GetEditMode(MCExecContext& ctxt, intenum_t& r_mode)
{
	if (m_edit_tool == NULL)
		r_mode = kMCEditModeNone;
	else
		r_mode = m_edit_tool -> type();
}

void MCGraphic::SetEditMode(MCExecContext& ctxt, intenum_t mode)
{
	MCEditMode t_old_mode;
	if (m_edit_tool == NULL)
		t_old_mode = kMCEditModeNone;
	else
		t_old_mode = m_edit_tool->type();

	MCEditMode t_new_mode = (MCEditMode)mode;

	if (t_old_mode != t_new_mode)
	{
		if (m_edit_tool != NULL)
		{
            /* Dirty the previous edit tool's rect. */
            getcard()->dirtyselection(m_edit_tool->drawrect());
        
			delete m_edit_tool;
			m_edit_tool = NULL;
		}

		MCEditTool *t_new_tool = NULL;
		switch (t_new_mode)
		{
		case kMCEditModeFillGradient:
			if (m_fill_gradient != NULL)
				t_new_tool = new (nothrow) MCGradientEditTool(this, m_fill_gradient, t_new_mode);
			break;
		case kMCEditModeStrokeGradient:
			if (m_stroke_gradient != NULL)
				t_new_tool = new (nothrow) MCGradientEditTool(this, m_stroke_gradient, t_new_mode);
			break;
		case kMCEditModePolygon:
			t_new_tool = new (nothrow) MCPolygonEditTool(this);
			break;
		case kMCEditModeNone:
			break;
		}
		m_edit_tool = t_new_tool;

        if (m_edit_tool != nullptr)
        {
            /* Dirty the new edit tool's rect. */
            getcard()->dirtyselection(m_edit_tool->drawrect());
        }
	}
}

void MCGraphic::GetCapStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	uint2 style;
	style = getcapstyle();
	r_style = (intenum_t)style;
}

void MCGraphic::SetCapStyle(MCExecContext& ctxt, intenum_t style)
{
	uint2 t_new_style;
	t_new_style = (uint2)style;
	if (t_new_style != getcapstyle())
	{
		MCRectangle t_oldrect = rect;
		setcapstyle(t_new_style);
		compute_minrect();
		Redraw(t_oldrect);
	}
}

void MCGraphic::GetJoinStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	uint2 style;
	style = getjoinstyle();
	r_style = (intenum_t)style;
}

void MCGraphic::SetJoinStyle(MCExecContext& ctxt, intenum_t style)
{
	uint2 t_new_style;
	t_new_style = (uint2)style;
	if (t_new_style != getjoinstyle())
	{
		MCRectangle t_oldrect = rect;
		setjoinstyle(t_new_style);
		compute_minrect();
		Redraw(t_oldrect);
	}
}

void MCGraphic::GetMiterLimit(MCExecContext& ctxt, double& r_limit)
{
	r_limit = m_stroke_miter_limit;
}

void MCGraphic::SetMiterLimit(MCExecContext& ctxt, double limit)
{
	real8 t_new_limit;
	t_new_limit = MCU_fmax(1, limit);
	if (m_stroke_miter_limit != t_new_limit)
	{
		MCRectangle t_oldrect = rect;
		m_stroke_miter_limit = (real4)t_new_limit;
		compute_minrect();
		Redraw(t_oldrect);
	}
}

void MCGraphic::GetLineSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = linesize;
}

void MCGraphic::SetLineSize(MCExecContext& ctxt, integer_t size)
{
	MCRectangle t_oldrect = rect;
	linesize = size;
	compute_minrect();
	delpoints();
	Redraw(t_oldrect);
}

void MCGraphic::GetPolySides(MCExecContext& ctxt, integer_t& r_sides)
{
	r_sides = nsides;
}

void MCGraphic::SetPolySides(MCExecContext& ctxt, integer_t p_sides)
{
	nsides = MCU_max(p_sides, 3);
	delpoints();
	Redraw();
}

void MCGraphic::GetAngle(MCExecContext& ctxt, integer_t& r_angle)
{
	if (getstyleint(flags) == F_OVAL)
		r_angle = startangle;
	else
		r_angle = angle;
}

void MCGraphic::SetAngle(MCExecContext& ctxt, integer_t p_angle)
{
	while (p_angle < 0)
		p_angle += 360;
	p_angle %= 360;
	if (getstyleint(flags) == F_OVAL)
		startangle = p_angle;
	else
		angle = p_angle;
	delpoints();
	Redraw();
}

void MCGraphic::GetStartAngle(MCExecContext& ctxt, integer_t& r_angle)
{
	r_angle = startangle;
}

void MCGraphic::SetStartAngle(MCExecContext& ctxt, integer_t p_angle)
{
	while (p_angle < 0)
		p_angle += 360;
	startangle = p_angle % 360;
	Redraw();
}

void MCGraphic::GetArcAngle(MCExecContext& ctxt, integer_t& r_angle)
{
	r_angle = arcangle;
}

void MCGraphic::SetArcAngle(MCExecContext& ctxt, integer_t p_angle)
{
	arcangle = MCU_max(MCU_min(360, p_angle), 0);
	Redraw();
}

void MCGraphic::GetRoundRadius(MCExecContext& ctxt, integer_t& r_radius)
{
	r_radius = roundradius;
}

void MCGraphic::SetRoundRadius(MCExecContext& ctxt, integer_t p_radius)
{
	roundradius = p_radius;
	delpoints();
	Redraw();
}

void MCGraphic::GetArrowSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = arrowsize;
}

void MCGraphic::SetArrowSize(MCExecContext& ctxt, integer_t p_size)
{
	MCRectangle t_oldrect = rect;
	arrowsize = p_size;
	compute_minrect();
	Redraw(t_oldrect);
}

void MCGraphic::GetStartArrow(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_START_ARROW);
}

void MCGraphic::SetStartArrow(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_START_ARROW);

	MCRectangle t_oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(t_oldrect);
}

void MCGraphic::GetEndArrow(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_END_ARROW);	
}

void MCGraphic::SetEndArrow(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_END_ARROW);

	MCRectangle t_oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(t_oldrect);
}

void MCGraphic::GetMarkerLineSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = markerlsize;
}

void MCGraphic::SetMarkerLineSize(MCExecContext& ctxt, integer_t size)
{
	MCRectangle t_oldrect = rect;
	markerlsize = size;
	compute_minrect();
	Redraw(t_oldrect);
}

void MCGraphic::GetMarkerDrawn(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MARKER_DRAWN);
}

void MCGraphic::SetMarkerDrawn(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_MARKER_DRAWN);

	MCRectangle t_oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(t_oldrect);
}

void MCGraphic::GetMarkerOpaque(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MARKER_OPAQUE);
}

void MCGraphic::SetMarkerOpaque(MCExecContext& ctxt, bool setting)
{
    if (!getflag(F_MARKER_OPAQUE) && setting)
    {
        bool t_succ = closepolygon(markerpoints, nmarkerpoints);
        if (!t_succ)
        {
            ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
            return;
        }
    }
	if (changeflag(setting, F_MARKER_OPAQUE))
	{
		Redraw();
	}
}

void MCGraphic::GetRoundEnds(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_CAPROUND);
}

void MCGraphic::SetRoundEnds(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_CAPROUND))
		Redraw();
}

void MCGraphic::GetDontResize(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DONT_RESIZE);
}

void MCGraphic::SetDontResize(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_DONT_RESIZE))
		Redraw();
}

void MCGraphic::GetStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	uint4 style;
	style = getstyleint(flags);

	if (style == F_G_RECTANGLE && linesize == 0 && flags & F_G_SHOW_NAME)
		r_style = kMCGraphicStyleText;
	else
		r_style = (intenum_t)style;
}

void MCGraphic::SetStyle(MCExecContext& ctxt, intenum_t p_style)
{
	flags &= ~F_STYLE;
	MCRectangle t_oldrect = rect;

	if (p_style == kMCGraphicStyleText)
	{
		flags |= F_G_RECTANGLE | F_G_SHOW_NAME;
		linesize = 0;
	}
	else if (p_style == kMCGraphicStyleArc)
	{
		flags |= F_OVAL;
		arcangle = 90;
	}
	else
	{
		flags |= (uint4)p_style;
		switch (p_style)
		{
		case kMCGraphicStylePolygon:
		case kMCGraphicStyleCurve:
		case kMCGraphicStyleLine:
			compute_minrect();
			break;
		default: 
			break;
		}
	}
	delpoints();
	Redraw(t_oldrect);
}

void MCGraphic::GetShowName(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_G_SHOW_NAME);
}

void MCGraphic::SetShowName(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_G_SHOW_NAME))
		Redraw();
}

void MCGraphic::GetLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	r_label = MCValueRetain(label);
}

void MCGraphic::SetLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	if (MCStringIsEqualTo(p_label, label, kMCStringOptionCompareExact))
		return;
	
	MCValueAssign(label, p_label);
	Redraw();
}

void MCGraphic::GetEffectiveLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	r_label = MCValueRetain(getlabeltext());
}

void MCGraphic::GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label)
{
	MCStringRef t_label = nil;
	GetLabel(ctxt, t_label);
	if (MCStringEncodeAndRelease(t_label, kMCStringEncodingUTF16, false, r_label))
		return;
	MCValueRelease(t_label);
	
	ctxt.Throw();
}

void MCGraphic::SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label)
{
	MCAutoStringRef t_new_label;
	if (MCStringDecode(p_label, kMCStringEncodingUTF16, false, &t_new_label))
	{
		SetLabel(ctxt, *t_new_label);
		return;
	}
	
	ctxt.Throw();
}

void MCGraphic::GetEffectiveUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label)
{
	MCStringRef t_label = nil;
	GetEffectiveLabel(ctxt, t_label);
	if (MCStringEncodeAndRelease(t_label, kMCStringEncodingUTF16, false, r_label))
		return;
	MCValueRelease(t_label);
	
	ctxt.Throw();
}

void MCGraphic::GetFilled(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_OPAQUE);
}

void MCGraphic::SetFilled(MCExecContext& ctxt, bool setting)
{
    if (!getflag(F_OPAQUE) && setting)
    {
        bool t_succ = closepolygon(realpoints, nrealpoints);
        if (!t_succ)
        {
            ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
            return;
        }
    }
	if (changeflag(setting, F_OPAQUE))
	{
		sync_mfocus(false, false);
		Redraw();
	}
}

void MCGraphic::DoGetGradientFill(MCExecContext& ctxt, MCGradientFill*& p_fill, MCNameRef p_prop, MCExecValue& r_value)
{
    if ((p_prop == nil || MCNameIsEmpty(p_prop)) && MCGradientFillGetProperties(ctxt, p_fill, r_value))
        return;
    
    if (MCGradientFillGetElement(ctxt, p_fill, p_prop, r_value))
        return;
    
    ctxt . Throw();
}

void MCGraphic::DoSetGradientFill(MCExecContext& ctxt, MCGradientFill*& p_fill, Draw_index p_di, MCNameRef p_prop, MCExecValue p_value)
{
    bool t_dirty = false;
    bool t_success = true;
    
    if (p_prop == nil || MCNameIsEmpty(p_prop))
        t_success = MCGradientFillSetProperties(ctxt, p_fill, rect, p_value, t_dirty);
    else
        t_success = MCGradientFillSetElement(ctxt, p_fill, p_prop, rect, p_value, t_dirty);
    
    if (t_success)
    {
        if (p_fill != nil)
		{
			MCInterfaceNamedColor t_empty_color;
            t_empty_color . name = MCValueRetain(kMCEmptyString);
            SetColor(ctxt, p_di, t_empty_color);
            SetPattern(ctxt, p_di, nil);
		}
        if (t_dirty && opened)
        {
            // MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
            layer_redrawall();
        }
        return;
    }
    
    ctxt . Throw();
}

void MCGraphic::GetGradientFillProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value)
{
    DoGetGradientFill(ctxt, m_fill_gradient, p_prop, r_value);
}

void MCGraphic::SetGradientFillProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue p_value)
{
    /* If the editMode is the fill gradient we must ensure we update the selection
     * layer. */
    bool t_is_editing =
            opened && m_edit_tool != nullptr && m_edit_tool->type() == kMCEditModeFillGradient;
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
    
    DoSetGradientFill(ctxt, m_fill_gradient, DI_BACK, p_prop, p_value);
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
}

void MCGraphic::GetGradientStrokeProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value)
{
    DoGetGradientFill(ctxt, m_stroke_gradient, p_prop, r_value);
}

void MCGraphic::SetGradientStrokeProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue p_value)
{
    /* If the editMode is the fill gradient we must ensure we update the selection
     * layer. */
    bool t_is_editing =
            opened && m_edit_tool != nullptr && m_edit_tool->type() == kMCEditModeStrokeGradient;
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
    
    DoSetGradientFill(ctxt, m_stroke_gradient, DI_FORE, p_prop, p_value);
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
}

void MCGraphic::DoCopyPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points, uindex_t& r_count, MCPoint*& r_points)
{
    if (p_count > 65535)
    {
        ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
        return;
    }
    
    MCAutoArray<MCPoint> t_points;
    
    for (uindex_t i = 0; i < p_count; i++)
        t_points . Push(p_points[i]);
    
    t_points . Take(r_points, r_count);
}

void MCGraphic::GetMarkerPoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points)
{
    DoCopyPoints(ctxt, nmarkerpoints, markerpoints, r_count, r_points);
}

void MCGraphic::SetMarkerPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points)
{
    if (p_count > 65535)
    {
        ctxt.LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
        return;
    }
    
    if (p_count == 0)
    {
        flags &= ~F_MARKER_DRAWN;
        delete[] markerpoints;
		markerpoints = NULL;
        nmarkerpoints = 0;
    }
    else
    {
        MCPoint *t_closed_points;
        uindex_t t_r_count;
        uint2 t_newcount;
        DoCopyPoints(ctxt, p_count, p_points, t_r_count, t_closed_points);
        t_newcount = uint2(t_r_count);
        if (flags & F_MARKER_OPAQUE)
        {
            if (!closepolygon(t_closed_points, t_newcount))
            {
                delete[] t_closed_points;
                ctxt.LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
                return;
            }
        }
        delete[] markerpoints;
        markerpoints = t_closed_points;
        nmarkerpoints = t_newcount;
        flags |= F_MARKER_DRAWN;
    }
    
    // SN-2014-06-02 [[ Bug 12576 ]] drawing_bug_when_rotating_graphic
    // Ensure that the functions which might change the size of the graphic redraw the former rectangle
    MCRectangle t_oldrect = rect;
    compute_minrect();
    Redraw(t_oldrect);
}

void MCGraphic::GetDashes(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_dashes)
{
    MCAutoArray<uinteger_t> t_dashes;
    
    for (uindex_t i = 0; i < ndashes; i++)
        t_dashes . Push(dashes[i]);
    
    t_dashes . Take(r_dashes, r_count);
}

void MCGraphic::SetDashes(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_dashes)
{
    MCAutoArray<uint1> t_dashes;
    
    uint1 *newdashes = nil;
    uint2 newndashes = 0;
    uint4 t_dash_length = 0;
    uindex_t t_new_count;
    
    for (uindex_t i = 0; i < p_count; i++)
    {
        if (p_dashes[i] >= 256)
        {
            ctxt . LegacyThrow(EE_GRAPHIC_NAN);
            return;
        }
        t_dashes . Push((uint1)p_dashes[i]);
        t_dash_length += p_dashes[i];
    }
    
    t_dashes . Take(newdashes, t_new_count);
    newndashes = t_new_count;
    
    if (newndashes > 0 && t_dash_length == 0)
    {
        delete newdashes;
        newdashes = nil;
        newndashes = 0;
    }
    delete dashes;
    dashes = newdashes;
    ndashes = newndashes;
    if (newndashes == 0)
        flags &= ~F_DASHES;
    else
        flags |= F_DASHES;
    
    Redraw();
}

// MDW-2014-06-21: [[ oval_points ]] refactoring
static bool effective_points_only(int graphic_type)
{
	bool effective_only = False;
	
	switch (graphic_type)
	{
		case F_ROUNDRECT:
		case F_G_RECTANGLE:
		case F_REGULAR:
		case F_OVAL:
			effective_only = True;
	}
	return (effective_only);
}


void MCGraphic::GetPoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points)
{
    // SN-2014-06-25: [[ MERGE-6.7 ]] P_POINTS getter updated
    // MDW-2014-06-18: [[ rect_points ]] allow effective points as read-only
    uint4 t_graphic_type;
    
    t_graphic_type = getstyleint(flags);
    
    if (effective_points_only(t_graphic_type))
    {
        r_count = 0;
        r_points = nil;
        return;
    }
    
    DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
}

void MCGraphic::SetPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points)
{
    SetPointsCommon(ctxt, p_count, p_points, false);
}

void MCGraphic::GetRelativePoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points)
{
    // SN-2014-06-25: [[ MERGE-6.7 ]]
    // MDW-2014-06-18: [[ rect_points ]] allow effective relativepoints as read-only
    uint4 t_graphic_type;
    
    t_graphic_type = getstyleint(flags);
    
    if (effective_points_only(t_graphic_type))
    {
        r_count = 0;
        r_points = nil;
        return;
    }
    
    MCRectangle trect = reduce_minrect(rect);
    MCU_offset_points(realpoints, nrealpoints, -trect.x, -trect.y);
    
    DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
    
    MCU_offset_points(realpoints, nrealpoints, trect.x, trect.y);
}

void MCGraphic::SetRelativePoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points)
{
    if (p_count > 65535)
    {
        ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
        return;
    }
        
    
    if (oldpoints != nil)
    {
        delete oldpoints;
        oldpoints = nil;
    }
    
    MCRectangle trect = reduce_minrect(rect);
    
    MCPoint *t_closed_points;
    uindex_t t_r_count;
    uint2 t_newcount;
    DoCopyPoints(ctxt, p_count, p_points, t_r_count, t_closed_points);
    t_newcount = uint2(t_r_count);
    MCU_offset_points(t_closed_points, t_newcount, trect.x, trect.y);
    if (flags & F_OPAQUE)
    {
        if (!closepolygon(t_closed_points, t_newcount))
        {
            delete[] t_closed_points;
            ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
            return;
        }
    }
    delete[] realpoints;
    realpoints = t_closed_points;
    nrealpoints = t_newcount;
    
    // SN-2014-06-02 [[ Bug 12576 ]] drawing_bug_when_rotating_graphic
    // Ensure that the functions which might change the size of the graphic redraw the former rectangle
    MCRectangle t_oldrect = rect;
    compute_minrect();
    Redraw(t_oldrect);
}

void MCGraphic::SetPointsCommon(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points, bool p_is_relative)
{
    if (p_count > 65535)
    {
        ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
        return;
    }
    
    if (oldpoints != nil)
    {
        delete oldpoints;
        oldpoints = nil;
    }
    
    MCPoint *t_closed_points;
    uindex_t t_r_count;
    uint2 t_newcount;
    DoCopyPoints(ctxt, p_count, p_points, t_r_count, t_closed_points);
    t_newcount = uint2(t_r_count);
    
    if (p_is_relative)
    {
        MCRectangle trect = reduce_minrect(rect);
        MCU_offset_points(t_closed_points, t_newcount, trect.x, trect.y);
    }
    
    if (flags & F_OPAQUE)
    {
        if (!closepolygon(t_closed_points, t_newcount))
        {
            delete[] t_closed_points;
            ctxt . LegacyThrow(EE_GRAPHIC_TOOMANYPOINTS);
            return;
        }
    }
    
    /* If we are editing the points, then we must update the edit tool's
     * part of the selection layer. */
    bool t_is_editing =
            m_edit_tool != nullptr && m_edit_tool->type() == kMCEditModePolygon;
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
    
    delete[] realpoints;
    realpoints = t_closed_points;
    nrealpoints = t_newcount;
    
    if (t_is_editing)
    {
        getcard()->dirtyselection(m_edit_tool->drawrect());
    }
    
    // SN-2014-06-02 [[ Bug 12576 ]] drawing_bug_when_rotating_graphic
    // The rectangle might change, we need to redraw what was the previous size.
    MCRectangle t_oldrect = rect;
    compute_minrect();
    Redraw(t_oldrect);
}

// SN-2014-06-25: [[ MERGE-6.7 ]] Effective points getter udpated
// MDW-2014-06-18: [[ rect_points ]] allow effective points as read-only
void MCGraphic::GetEffectivePoints(MCExecContext &ctxt, uindex_t &r_count, MCPoint *&r_points)
{
    switch (getstyleint(flags))
    {
        case F_ROUNDRECT:
        {
            /* UNCHECKED */ get_points_for_roundrect(r_points, r_count);
            break;
        }
        case F_G_RECTANGLE:
        {
            /* UNCHECKED */ get_points_for_rect(r_points, r_count);
            break;
        }
        case F_REGULAR:
        {
			/* UNCHECKED */ get_points_for_regular_polygon(r_points, r_count);
            break;
        }
        // MDW-2014-06-21: [[ oval_points ]] allow effective points for ovals
        case F_OVAL:
        {
            /* UNCHECKED */ get_points_for_oval(r_points, r_count);
            break;
        }
        default:
            DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
    }
}


// SN-2014-06-25: [[ MERGE-6.7 ]] Effective relative point getter updated
// MDW-2014-06-18: [[ rect_points ]] allow effective points as read-only
void MCGraphic::GetEffectiveRelativePoints(MCExecContext &ctxt, uindex_t &r_count, MCPoint *&r_points)
{
    MCRectangle trect;
    trect = reduce_minrect(rect);
    
    switch (getstyleint(flags))
    {
        case F_ROUNDRECT:
        {
            /* UNCHECKED */ get_points_for_roundrect(r_points, r_count);
            MCU_offset_points(r_points, r_count, -trect.x, -trect.y);
            break;
        }
        case F_G_RECTANGLE:
        {
            /* UNCHECKED */ get_points_for_rect(r_points, r_count);
            MCU_offset_points(r_points, r_count, -trect.x, -trect.y);
            break;
        }
        case F_REGULAR:
        {
			/* UNCHECKED */ get_points_for_regular_polygon(r_points, r_count);
            MCU_offset_points(r_points, r_count, -trect.x, -trect.y);
            break;
        }
        // MDW-2014-06-21: [[ oval_points ]] allow effective points for ovals
        case F_OVAL:
        {
            /* UNCHECKED */ get_points_for_oval(r_points, r_count);
            MCU_offset_points(r_points, r_count, -trect.x, -trect.y);
            break;
        }
        default:
        {
            DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
			MCU_offset_points(r_points, r_count, -trect.x, -trect.y);
        }
    }
}

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGradientFillKindElementInfo[] =
{
	{ "linear", kMCGradientKindLinear, false },
    { "radial", kMCGradientKindRadial, false },
	{ "conical", kMCGradientKindConical, false },
    { "diamond", kMCGradientKindDiamond, false },
	{ "spiral", kMCGradientKindSpiral, false },
    { "xy", kMCGradientKindXY, false },
	{ "sqrtxy", kMCGradientKindSqrtXY, false },
    { "none", kMCGradientKindNone, false },
    { "0", kMCGradientKindNone, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGradientFillKindTypeInfo =
{
	"Interface.GradientFillKind",
	sizeof(_kMCInterfaceGradientFillKindElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGradientFillKindElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceGradientFillQualityElementInfo[] =
{
	{ "normal", kMCGradientQualityNormal, false },
	{ "good", kMCGradientQualityGood, false },
};

static MCExecEnumTypeInfo _kMCInterfaceGradientFillQualityTypeInfo =
{
	"Interface.GradientFillQuality",
	sizeof(_kMCInterfaceGradientFillQualityElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceGradientFillQualityElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceGradientFillKindTypeInfo = &_kMCInterfaceGradientFillKindTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceGradientFillQualityTypeInfo = &_kMCInterfaceGradientFillQualityTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCGraphic::SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
    // PM-2015-21-01: When the graphic has a strokegradient, make sure we can set a fore color of the form "rrr,ggg,bbb" (where color.name == nil)
    if (m_stroke_gradient != nil)
    {
        MCGradientFillFree(m_stroke_gradient);
        m_stroke_gradient = nil;
    }
    MCObject::SetForeColor(ctxt, color);
}

void MCGraphic::SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color)
{
    // PM-2015-21-01: [[ Bug 14399 ]] Remove fillgradient when setting the bg color of a graphic
    // Also make sure we can set a bg color of the form "rrr,ggg,bbb" (where color.name == nil)
    if (m_fill_gradient != nil)
    {
        MCGradientFillFree(m_fill_gradient);
        m_fill_gradient = nil;
    }
    MCObject::SetBackColor(ctxt, color);
}

void MCGraphic::SetForePattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    if (m_stroke_gradient != nil)
    {
        MCGradientFillFree(m_stroke_gradient);
        m_stroke_gradient = nil;
    }
    MCObject::SetForePattern(ctxt, pattern);
}

void MCGraphic::SetBackPattern(MCExecContext& ctxt, uinteger_t* pattern)
{
    // PM-2015-21-01: [[ Bug 14399 ]] Remove fillgradient when setting the bg pattern of a graphic
    if (m_fill_gradient != nil)
    {
        MCGradientFillFree(m_fill_gradient);
        m_fill_gradient = nil;
    }
    MCObject::SetBackPattern(ctxt, pattern);
}

