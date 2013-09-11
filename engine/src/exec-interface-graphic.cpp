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
			if (resizeparent())
				return;

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
		MCRectangle t_old_effective_rect;
		t_old_effective_rect = geteffectiverect();

		if (m_edit_tool != NULL)
		{
			delete m_edit_tool;
			m_edit_tool = NULL;
		}

		MCEditTool *t_new_tool = NULL;
		switch (t_new_mode)
		{
		case kMCEditModeFillGradient:
			if (m_fill_gradient != NULL)
				t_new_tool = new MCGradientEditTool(this, m_fill_gradient, t_new_mode);
			break;
		case kMCEditModeStrokeGradient:
			if (m_stroke_gradient != NULL)
				t_new_tool = new MCGradientEditTool(this, m_stroke_gradient, t_new_mode);
			break;
		case kMCEditModePolygon:
			t_new_tool = new MCPolygonEditTool(this);
			break;
		}
		m_edit_tool = t_new_tool;

		layer_effectiverectchangedandredrawall(t_old_effective_rect);
	}

	Redraw();
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
		MCRectangle oldrect = rect;
		setcapstyle(t_new_style);
		compute_minrect();
		Redraw(oldrect);
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
		MCRectangle oldrect = rect;
		setjoinstyle(t_new_style);
		compute_minrect();
		Redraw(oldrect);
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
		MCRectangle oldrect = rect;
		m_stroke_miter_limit = (real4)t_new_limit;
		compute_minrect();
		Redraw(oldrect);
	}
}

void MCGraphic::GetLineSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = linesize;
}

void MCGraphic::SetLineSize(MCExecContext& ctxt, integer_t size)
{
	MCRectangle oldrect = rect;
	linesize = size;
	compute_minrect();
	delpoints();
	Redraw(oldrect);
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
	MCRectangle oldrect = rect;
	arrowsize = p_size;
	compute_minrect();
	Redraw(oldrect);
}

void MCGraphic::GetStartArrow(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_START_ARROW);
}

void MCGraphic::SetStartArrow(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_START_ARROW);

	MCRectangle oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(oldrect);
}

void MCGraphic::GetEndArrow(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_END_ARROW);	
}

void MCGraphic::SetEndArrow(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_END_ARROW);

	MCRectangle oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(oldrect);
}

void MCGraphic::GetMarkerLineSize(MCExecContext& ctxt, integer_t& r_size)
{
	r_size = markerlsize;
}

void MCGraphic::SetMarkerLineSize(MCExecContext& ctxt, integer_t size)
{
	MCRectangle oldrect = rect;
	markerlsize = size;
	compute_minrect();
	Redraw(oldrect);
}

void MCGraphic::GetMarkerDrawn(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MARKER_DRAWN);
}

void MCGraphic::SetMarkerDrawn(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_MARKER_DRAWN);

	MCRectangle oldrect = rect;
	compute_minrect();
	if (t_dirty)
		Redraw(oldrect);
}

void MCGraphic::GetMarkerOpaque(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MARKER_OPAQUE);
}

void MCGraphic::SetMarkerOpaque(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_MARKER_OPAQUE))
	{
		if (flags & F_MARKER_OPAQUE)
			closepolygon(markerpoints, nmarkerpoints);
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
	MCRectangle oldrect = rect;

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
	Redraw(oldrect);
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

void MCGraphic::DoGetLabel(MCExecContext& ctxt, bool to_unicode, bool effective, MCStringRef r_string)
{
	// Fetch the label, taking note of its encoding.
	MCString slabel;
	bool is_unicode;
	if (effective)
		getlabeltext(slabel, is_unicode);
	else
		slabel.set(label,labelsize), is_unicode = hasunicode();

	// If the label's encoding doesn't match the request, map.
	if (MCU_mapunicode(slabel, is_unicode, to_unicode, r_string))
		return;

	ctxt . Throw();
}

void MCGraphic::DoSetLabel(MCExecContext& ctxt, bool to_unicode, MCStringRef p_label)
{
	if (label == NULL || p_label == nil ||
		MCStringIsEqualToOldString(p_label, MCString(label, labelsize), kMCCompareExact) ||
		to_unicode != hasunicode())
	{
		delete label;
		label = NULL;
		if (p_label != nil)
		{
			labelsize = MCStringGetLength(p_label);
			label = new char[labelsize];
			memcpy(label, MCStringGetCString(p_label), labelsize);
			flags |= F_G_LABEL;

			// If we are setting the unicode label we become unicode; else we revert
			// to native.
			if (to_unicode)
				m_font_flags |= FF_HAS_UNICODE;
			else
				m_font_flags &= ~FF_HAS_UNICODE;
		}
		else
		{
			labelsize = 0;
			flags &= ~F_G_LABEL;
		}
		Redraw();
	}
}

void MCGraphic::GetLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, false, false, r_label);
}

void MCGraphic::SetLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	DoSetLabel(ctxt, false, p_label);
}

void MCGraphic::GetEffectiveLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, false, true, r_label);
}

void MCGraphic::GetUnicodeLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, true, false, r_label);
}

void MCGraphic::SetUnicodeLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	DoSetLabel(ctxt, true, p_label);
}

void MCGraphic::GetEffectiveUnicodeLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, true, true, r_label);
}

void MCGraphic::GetFilled(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_OPAQUE);
}

void MCGraphic::SetFilled(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_OPAQUE))
	{
		if (flags & F_OPAQUE)
			closepolygon(realpoints, nrealpoints);
		Redraw();
	}
}

void MCGraphic::DoGetGradientFillArray(MCExecContext& ctxt, MCGradientFill *p_fill, MCArrayRef& r_array)
{
    if (MCGradientFillGetProperties(p_fill, r_array))
        ctxt . Throw();
}

void MCGraphic::DoSetGradientFillArray(MCExecContext& ctxt, MCGradientFill *p_fill, Draw_index p_di, MCArrayRef p_array)
{
    bool t_dirty;
    if (MCGradientFillSetProperties(p_fill, rect, p_array, t_dirty))
    {
        if (p_fill != nil)
		{
			MCInterfaceNamedColor t_empty_color;
            t_empty_color . name = MCValueRetain(kMCEmptyString);
            SetColor(ctxt, p_di, t_empty_color);
            SetPattern(ctxt, p_di, nil);
            
		}
        if (t_dirty)
            Redraw();
        return;
    }
    
    ctxt . Throw();
}

void MCGraphic::DoGetGradientFillElement(MCExecContext& ctxt, MCGradientFill *p_fill, MCNameRef p_prop, MCValueRef& r_value)
{
    if (MCGradientFillGetElement(p_fill, p_prop, r_value))
        return;
    
    ctxt . Throw();
}

void MCGraphic::DoSetGradientFillElement(MCExecContext& ctxt, MCGradientFill *p_fill, Draw_index p_di, MCNameRef p_prop, MCValueRef p_value)
{
    bool t_dirty;
    if (MCGradientFillSetElement(p_fill, p_prop, rect, p_value, t_dirty))
    {
        if (p_fill != nil)
		{
			MCInterfaceNamedColor t_empty_color;
            t_empty_color . name = MCValueRetain(kMCEmptyString);
            SetColor(ctxt, p_di, t_empty_color);
            SetPattern(ctxt, p_di, nil);
		}
        if (t_dirty)
            Redraw();
        return;
    }
    
    ctxt . Throw();
}

void MCGraphic::GetGradientFill(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetGradientFillArray(ctxt, m_fill_gradient, r_array);
}

void MCGraphic::SetGradientFill(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetGradientFillArray(ctxt, m_fill_gradient, DI_BACK, p_array);
}

void MCGraphic::GetGradientFillElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetGradientFillElement(ctxt, m_fill_gradient, p_prop, r_value);
}

void MCGraphic::SetGradientFillElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetGradientFillElement(ctxt, m_fill_gradient, DI_BACK, p_prop, p_value);
}

void MCGraphic::GetGradientStroke(MCExecContext& ctxt, MCArrayRef& r_array)
{
    DoGetGradientFillArray(ctxt, m_stroke_gradient, r_array);
}

void MCGraphic::SetGradientStroke(MCExecContext& ctxt, MCArrayRef p_array)
{
    DoSetGradientFillArray(ctxt, m_stroke_gradient, DI_FORE, p_array);
}

void MCGraphic::GetGradientStrokeElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef& r_value)
{
    DoGetGradientFillElement(ctxt, m_stroke_gradient, p_prop, r_value);
}

void MCGraphic::SetGradientStrokeElement(MCExecContext& ctxt, MCNameRef p_prop, MCValueRef p_value)
{
    DoSetGradientFillElement(ctxt, m_stroke_gradient, DI_FORE, p_prop, p_value);
}

void MCGraphic::DoCopyPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points, uindex_t& r_count, MCPoint*& r_points)
{
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
    if (p_count == 0)
        flags &= ~F_MARKER_DRAWN;
    else
    {
        uindex_t t_new_count;
        DoCopyPoints(ctxt, p_count, p_points, t_new_count, markerpoints);
        nmarkerpoints = (uint2)t_new_count;
        flags |= F_MARKER_DRAWN;
    }
    
    if (flags & F_MARKER_DRAWN && flags & F_MARKER_OPAQUE)
        closepolygon(markerpoints, nmarkerpoints);
    
    compute_minrect();
    Redraw();
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

void MCGraphic::GetPoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points)
{
    DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
}

void MCGraphic::SetPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points)
{
    if (oldpoints != nil)
    {
        delete oldpoints;
        oldpoints = nil;
    }
    uindex_t t_new_count;
    DoCopyPoints(ctxt, p_count, p_points, t_new_count, realpoints);
    nrealpoints = (uint2)t_new_count;
    
    if (flags & F_OPAQUE)
        closepolygon(realpoints, nrealpoints);
    
    compute_minrect();
    Redraw();
}

void MCGraphic::GetRelativePoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points)
{
    MCRectangle trect = reduce_minrect(rect);
    MCU_offset_points(realpoints, nrealpoints, -trect.x, -trect.y);
    
    DoCopyPoints(ctxt, nrealpoints, realpoints, r_count, r_points);
    
    MCU_offset_points(realpoints, nrealpoints, trect.x, trect.y);
}

void MCGraphic::SetRelativePoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points)
{
    if (oldpoints != nil)
    {
        delete oldpoints;
        oldpoints = nil;
    }
    MCRectangle trect = reduce_minrect(rect);
    MCU_offset_points(realpoints, nrealpoints, -trect.x, -trect.y);
    
    uindex_t t_new_count;
    DoCopyPoints(ctxt, p_count, p_points, t_new_count, realpoints);
    nrealpoints = (uint2)t_new_count;
    
    MCU_offset_points(realpoints, nrealpoints, trect.x, trect.y);
    
    if (flags & F_OPAQUE)
        closepolygon(realpoints, nrealpoints);
    
    compute_minrect();
    Redraw();
}