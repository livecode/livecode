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

void MCGraphic::GetLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	r_label = MCValueRetain(label);
}

void MCGraphic::SetLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	if (p_label != nil)
	{
		MCValueAssign(label, p_label);
		Redraw();
		return;
	}
	
	ctxt.Throw();
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
	if (changeflag(setting, F_OPAQUE))
	{
		if (flags & F_OPAQUE)
			closepolygon(realpoints, nrealpoints);
		Redraw();
	}
}