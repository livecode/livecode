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
#include "util.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "graphic.h"
#include "mcerror.h"
#include "globals.h"
#include "core.h"

#include "path.h"
#include "context.h"
#include "gradient.h"
#include "objectstream.h"
#include "font.h"


#define GRAPHIC_EXTRA_MITERLIMIT		(1UL << 0)
#define GRAPHIC_EXTRA_FILLGRADIENT		(1UL << 1)
#define GRAPHIC_EXTRA_STROKEGRADIENT	(1UL << 2)
#define GRAPHIC_EXTRA_MARGINS			(1UL << 3)

MCGraphic::MCGraphic()
{
	initialise();
}

MCGraphic::MCGraphic(const MCGraphic &gref) : MCControl(gref)
{
	linesize = gref.linesize;
	angle = gref.angle;
	startangle = gref.startangle;
	arcangle = gref.arcangle;
	roundradius = gref.roundradius;
	arrowsize = gref.arrowsize;
	nsides = gref.nsides;
	ndashes = gref.ndashes;
	npoints = 0;
	nrealpoints = gref.nrealpoints;
	nmarkerpoints = gref.nmarkerpoints;
	markerlsize = gref.markerlsize;
	if (gref.dashes != NULL)
	{
		dashes = new uint1[ndashes];
		memcpy(dashes, gref.dashes, ndashes);
	}
	else
		dashes = NULL;
	points = NULL;
	if (gref.realpoints != NULL)
		realpoints = new MCPoint[nrealpoints];
	else
		realpoints = NULL;
	if (gref.markerpoints != NULL)
		markerpoints = new MCPoint[nmarkerpoints];
	else
		markerpoints = NULL;
	uint2 i;
	for (i = 0 ; i < nrealpoints ; i++)
		realpoints[i] = gref.realpoints[i];
	for (i = 0 ; i < nmarkerpoints ; i++)
		markerpoints[i] = gref.markerpoints[i];
	oldpoints = NULL;
	label = NULL;
	labelsize = 0;
	if (gref.label)
	{
		labelsize = gref.labelsize;
		label = new char[labelsize];
		memcpy(label, gref.label, labelsize);
	}

	m_fill_gradient = MCGradientFillCopy(gref.m_fill_gradient);
	m_stroke_gradient = MCGradientFillCopy(gref.m_stroke_gradient);

	m_edit_tool = NULL;
	minrect.x = MININT2;
	minrect.y = MININT2;
	minrect.width = 0;
	minrect.height = 0;

	m_stroke_miter_limit = gref.m_stroke_miter_limit;
}

void MCGraphic::initialise(void)
{
	flags |= F_G_RECTANGLE | F_ALIGN_CENTER | F_CAPROUND | F_G_ANTI_ALIASED;
	flags &= ~(F_SHOW_BORDER | F_OPAQUE | F_TRAVERSAL_ON);
	linesize = 1;
	angle = 0;
	startangle = 0;
	arcangle = 360;
	roundradius = DEFAULT_RADIUS;
	arrowsize = DEFAULT_ARROW_SIZE;
	nsides = 4;
	ndashes = 0;
	npoints = 0;
	nrealpoints = 0;
	nmarkerpoints = 0;
	markerlsize = 1;
	dashes = NULL;
	points = NULL;
	realpoints = NULL;
	markerpoints = NULL;
	oldpoints = NULL;
	label = NULL;
	labelsize = 0;

	setfillrule(kMCFillRuleNone);
	m_fill_gradient = NULL;
	m_stroke_gradient = NULL;
	m_edit_tool = NULL;

	minrect.x = MININT2;
	minrect.y = MININT2;
	minrect.width = 0;
	minrect.height = 0;

	m_stroke_miter_limit = 10.0;
}

void MCGraphic::finalise(void)
{
	delete dashes;
	delete points;
	delete realpoints;
	delete markerpoints;
	delete oldpoints;
	delete label;

	if (m_fill_gradient != NULL)
		MCGradientFillFree(m_fill_gradient);

	if (m_stroke_gradient != NULL)
		MCGradientFillFree(m_stroke_gradient);
}

MCGraphic::~MCGraphic()
{
	finalise();
}

Chunk_term MCGraphic::gettype() const
{
	return CT_GRAPHIC;
}

const char *MCGraphic::gettypestring()
{
	return MCgraphicstring;
}

Boolean MCGraphic::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
	        || flags & F_DISABLED && (getstack()->gettool(this) == T_BROWSE))
		return False;
	if ((state & CS_SIZE || state & CS_MOVE) && points != NULL
	        && getstyleint(flags) != F_G_RECTANGLE && getstyleint(flags) != F_OVAL)
	{
		delete points;
		points = NULL;
		npoints = 0;
	}
	if (state & CS_CREATE && nrealpoints == 0
	        && (getstyleint(flags) == F_CURVE || getstyleint(flags) == F_POLYGON
	            || getstyleint(flags) == F_LINE))
	{
		realpoints = new MCPoint[MCscreen->getmaxpoints()];
		MCU_snap(rect.x);
		MCU_snap(rect.y);
		startx = rect.x;
		starty = rect.y;
		if (getstyleint(flags) == F_CURVE)
			nrealpoints = 0;
		else
		{
			realpoints[0].x = rect.x;
			realpoints[0].y = rect.y;
			nrealpoints = 2;
			if (getstyleint(flags) == F_POLYGON)
				MCscreen->grabpointer(getw());
		}
		state |= CS_CREATE_POINTS;
	}
	if (state & CS_CREATE_POINTS)
	{
		if (getstyleint(flags) == F_CURVE)
		{
			if (nrealpoints < MCscreen->getmaxpoints())
			{
				realpoints[nrealpoints].x = x;
				realpoints[nrealpoints++].y = y;
			}
		}
		else
		{
			mx = x;
			my = y;
			MCU_snap(mx);
			MCU_snap(my);
			if (MCmodifierstate & MS_SHIFT)
			{
				real8 dx = (real8)(mx - startx);
				real8 dy = (real8)(my - starty);
				real8 length = sqrt(dx * dx + dy * dy);
				real8 angle = atan2(dy, dx);
				real8 quanta = M_PI * 2.0 / (real8)MCslices;
				angle = floor((angle + quanta / 2.0) / quanta) * quanta;
				mx = startx + (int2)(cos(angle) * length);
				my = starty + (int2)(sin(angle) * length);
			}
			realpoints[nrealpoints - 1].x = mx;
			realpoints[nrealpoints - 1].y = my;
		}
		MCRectangle drect = rect;
		compute_minrect();
		// MW-2011-08-18: [[ Layers ]] Notify of a changed rect and invalidate.
		layer_rectchanged(drect, true);
		message_with_args(MCM_mouse_move, x, y);
		return True;
	}
	else if (m_edit_tool != NULL && m_edit_tool->mfocus(x, y))
	{
		mx = x;
		my = y;
		return True;
	}
	else
		return MCControl::mfocus(x, y);
}

Boolean MCGraphic::mdown(uint2 which)
{
	if (state & CS_CREATE_POINTS && getstyleint(flags) == F_POLYGON)
	{
		if (realpoints[nrealpoints - 1].x == realpoints[0].x
		        && realpoints[nrealpoints - 1].y == realpoints[0].y)
		{
			MCscreen->ungrabpointer();
			state &= ~CS_CREATE_POINTS;
			end();
		}
		else
			nrealpoints++;
		startx = mx;
		starty = my;
		return True;
	}
	if (state & CS_MFOCUSED)
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	state |= CS_MFOCUSED;
	if (which == Button1)
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (message_with_args(MCM_mouse_down, "1") != ES_NORMAL && m_edit_tool != NULL)
			{
				if (m_edit_tool->mdown(mx, my, which))
					return True;
			}

			break;
		case T_POINTER:
		case T_GRAPHIC:
			if (m_edit_tool == NULL || !m_edit_tool->mdown(mx, my, which))
				start(True);
			break;
		case T_HELP:
			break;
		default:
			return False;
		}
	else
		message_with_args(MCM_mouse_down, which);
	return True;
}

Boolean MCGraphic::mup(uint2 which)
{
	if (state & CS_CREATE_POINTS && getstyleint(flags) == F_POLYGON)
		return True;
	if (!(state & CS_MFOCUSED))
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which);
	state &= ~(CS_MFOCUSED | CS_CREATE_POINTS);
	if (state & CS_GRAB)
	{
		ungrab(which);
		return True;
	}
	if (which == Button1)
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (m_edit_tool != NULL)
				m_edit_tool->mup(mx, my, which);
			if (MCU_point_in_rect(rect, mx, my))
				message_with_args(MCM_mouse_up, "1");
			else
				message_with_args(MCM_mouse_release, "1");
			break;
		case T_GRAPHIC:
		case T_POINTER:
			if (m_edit_tool != NULL)
				m_edit_tool->mup(mx, my, which);
			end();
			break;
		case T_HELP:
			help();
			break;
		default:
			return False;
		}
	else
		if (MCU_point_in_rect(rect, mx, my))
			message_with_args(MCM_mouse_up, which);
		else
			message_with_args(MCM_mouse_release, which);
	return True;
}

Boolean MCGraphic::doubledown(uint2 which)
{
	return MCControl::doubledown(which);
}

Boolean MCGraphic::doubleup(uint2 which)
{
	if (state & CS_CREATE_POINTS && getstyleint(flags) == F_POLYGON)
	{
		MCscreen->ungrabpointer();
		nrealpoints--;
		state &= ~(CS_CREATE_POINTS | CS_MFOCUSED);
		end(false);
		return True;
	}
	return MCControl::doubleup(which);
}

void MCGraphic::setrect(const MCRectangle &nrect)
{
	if (realpoints != NULL)
	{
		if (nrect.width != rect.width || nrect.height != rect.height)
		{
			MCRectangle trect = reduce_minrect(nrect);
			if (oldpoints == NULL)
			{
				oldpoints = new MCPoint[nrealpoints];
				uint2 i = nrealpoints;
				while (i--)
					oldpoints[i] = realpoints[i];
				oldrect = reduce_minrect(rect);
				MCU_offset_points(oldpoints, nrealpoints, -oldrect.x, -oldrect.y);
				if (nrealpoints == 2)
				{
					if (oldpoints[0].x == oldpoints[1].x)
					{
						oldpoints[1].x++;
						oldrect.width++;
					}
					if (oldpoints[0].y == oldpoints[1].y)
					{
						oldpoints[1].y++;
						oldrect.height++;
					}
				}
			}
			uint2 i;
			for (i = 0 ; i < nrealpoints ; i++)
				if (oldpoints[i].x == MININT2)
					realpoints[i] = oldpoints[i];
				else
				{
					if (oldrect.width != 0)
						realpoints[i].x = oldpoints[i].x * trect.width
						                  / oldrect.width + trect.x;
					if (oldrect.height != 0)
						realpoints[i].y = oldpoints[i].y * trect.height
						                  / oldrect.height + trect.y;
				}
		}
		else
			MCU_offset_points(realpoints, nrealpoints, nrect.x - rect.x, nrect.y - rect.y);
	}
	if (m_fill_gradient != NULL)
		setgradientrect(m_fill_gradient, nrect);
	if (m_stroke_gradient != NULL)
		setgradientrect(m_stroke_gradient, nrect);

	rect = nrect;
	delpoints();
}

void MCGraphic::setgradientrect(MCGradientFill *p_gradient, const MCRectangle &nrect)
{
	if (nrect.width != rect.width || nrect.height != rect.height)
	{
		MCRectangle trect = reduce_minrect(nrect);
		if (p_gradient->old_origin.x == MININT2 && p_gradient->old_origin.y == MININT2)
		{
			p_gradient->old_rect = reduce_minrect(rect);
			p_gradient->old_origin.x = p_gradient->origin.x - p_gradient->old_rect.x;
			p_gradient->old_origin.y = p_gradient->origin.y - p_gradient->old_rect.y;
			p_gradient->old_primary.x = p_gradient->primary.x - p_gradient->old_rect.x;
			p_gradient->old_primary.y = p_gradient->primary.y - p_gradient->old_rect.y;
			p_gradient->old_secondary.x = p_gradient->secondary.x - p_gradient->old_rect.x;
			p_gradient->old_secondary.y = p_gradient->secondary.y - p_gradient->old_rect.y;
		}
		if (p_gradient->old_rect.width != 0)
		{
			p_gradient->origin.x = p_gradient->old_origin.x * trect.width / p_gradient->old_rect.width + trect.x;
			p_gradient->primary.x = p_gradient->old_primary.x * trect.width / p_gradient->old_rect.width + trect.x;
			p_gradient->secondary.x = p_gradient->old_secondary.x * trect.width / p_gradient->old_rect.width + trect.x;
		}
		if (p_gradient->old_rect.height != 0)
		{
			p_gradient->origin.y = p_gradient->old_origin.y * trect.height / p_gradient->old_rect.height + trect.y;
			p_gradient->primary.y = p_gradient->old_primary.y * trect.height / p_gradient->old_rect.height + trect.y;
			p_gradient->secondary.y = p_gradient->old_secondary.y * trect.height / p_gradient->old_rect.height + trect.y;
		}
	}
	else
	{
		p_gradient->origin.x += nrect.x - rect.x;
		p_gradient->origin.y += nrect.y - rect.y;
		p_gradient->primary.x += nrect.x - rect.x;
		p_gradient->primary.y += nrect.y - rect.y;
		p_gradient->secondary.x += nrect.x - rect.x;
		p_gradient->secondary.y += nrect.y - rect.y;
	}
}

Exec_stat MCGraphic::getprop(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective)
{
	uint2 i;

	switch (which)
	{
#ifdef /* MCGraphic::getprop */ LEGACY_EXEC
	case P_ANTI_ALIASED:
		ep.setboolean(getflag(F_G_ANTI_ALIASED));
		break;

	// PROPERTY - FILL RULE
	case P_FILL_RULE:
		switch(getfillrule())
		{
		case kMCFillRuleNone:
			ep . setstaticcstring("none");
		break;

		case kMCFillRuleNonZero:
			ep . setstaticcstring("nonzero");
		break;

		case kMCFillRuleEvenOdd:
			ep . setstaticcstring("evenodd");
		break;
		};
	break;

	case P_EDIT_MODE:
		if (m_edit_tool == NULL)
		{
			ep.setstaticcstring("none");
			break;
		}
		switch (m_edit_tool->type())
		{
		case kMCEditModeFillGradient:
			ep.setstaticcstring("fillgradient");
			break;
		case kMCEditModeStrokeGradient:
			ep.setstaticcstring("strokegradient");
			break;
		case kMCEditModePolygon:
			ep.setstaticcstring("polygon");
			break;
		}
		break;
	case P_CAP_STYLE:
		switch (getcapstyle())
		{
		case CapRound:
			ep.setstaticcstring("round");
			break;
		case CapProjecting:
			ep.setstaticcstring("square");
			break;
		case CapButt:
			ep.setstaticcstring("butt");
			break;
		}
		break;
	case P_JOIN_STYLE:
		switch (getjoinstyle())
		{
		case JoinBevel:
			ep.setstaticcstring("bevel");
			break;
		case JoinRound:
			ep.setstaticcstring("round");
			break;
		case JoinMiter:
			ep.setstaticcstring("miter");
			break;
		}
		break;
	case P_MITER_LIMIT:
		ep.setr8(m_stroke_miter_limit, ep.getnffw(), ep.getnftrailing(), ep.getnfforce());
		break;
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
		ep.setint(linesize);
		break;
	case P_POLY_SIDES:
		ep.setint(nsides);
		break;
	case P_POINTS:
		MCU_unparsepoints(realpoints, nrealpoints, ep);
		break;
	case P_RELATIVE_POINTS:
		{
			MCRectangle trect = reduce_minrect(rect);
			MCU_offset_points(realpoints, nrealpoints, -trect.x, -trect.y);
			MCU_unparsepoints(realpoints, nrealpoints, ep);
			MCU_offset_points(realpoints, nrealpoints, trect.x, trect.y);
		}
		break;
	case P_ANGLE:
		if (getstyleint(flags) == F_OVAL)
			ep.setint(startangle);
		else
			ep.setint(angle);
		break;
	case P_START_ANGLE:
		ep.setint(startangle);
		break;
	case P_ARC_ANGLE:
		ep.setint(arcangle);
		break;
	case P_ROUND_RADIUS:
		ep.setint(roundradius);
		break;
	case P_ARROW_SIZE:
		ep.setint(arrowsize);
		break;
	case P_START_ARROW:
		ep.setboolean(getflag(F_START_ARROW));
		break;
	case P_END_ARROW:
		ep.setboolean(getflag(F_END_ARROW));
		break;
	case P_MARKER_LSIZE:
		ep.setint(markerlsize);
		break;
	case P_MARKER_DRAWN:
		ep.setboolean(getflag(F_MARKER_DRAWN));
		break;
	case P_MARKER_OPAQUE:
		ep.setboolean(getflag(F_MARKER_OPAQUE));
		break;
	case P_MARKER_POINTS:
		MCU_unparsepoints(markerpoints, nmarkerpoints, ep);
		break;
	case P_DASHES:
		ep.clear();
		for (i = 0 ; i < ndashes ; i++)
			ep.concatuint(dashes[i], EC_COMMA, i == 0);
		break;
	case P_ROUND_ENDS:
		ep.setboolean(getflag(F_CAPROUND));
		break;
	case P_DONT_RESIZE:
		ep.setboolean(getflag(F_DONT_RESIZE));
		break;
	case P_STYLE:
		{
			const char *t_style_string;
			switch (getstyleint(flags))
			{
			case F_G_RECTANGLE:
				if (linesize == 0 && flags & F_G_SHOW_NAME)
					t_style_string = MCtextstring;
				else
					t_style_string = MCrectanglestring;
				break;
			case F_ROUNDRECT:
				t_style_string = MCroundrectstring;
				break;
			case F_POLYGON:
				t_style_string = MCpolygonstring;
				break;
			case F_OVAL:
				t_style_string = MCovalstring;
				break;
			case F_REGULAR:
				t_style_string = MCregularstring;
				break;
			case F_CURVE:
				t_style_string = MCcurvestring;
				break;
			case F_LINE:
				t_style_string = MClinestring;
				break;
			}
			ep . setstaticcstring(t_style_string);
		}
		break;
	case P_SHOW_NAME:
		ep.setboolean(getflag(F_G_SHOW_NAME));
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for a 'unicodeLabel' property.
	case P_TEXT:
	case P_LABEL:
	case P_UNICODE_TEXT:
	case P_UNICODE_LABEL:
		{
			// Fetch the label, taking note of its encoding.
			MCString slabel;
			bool isunicode;
			if (effective)
				getlabeltext(slabel, isunicode);
			else
				slabel.set(label,labelsize), isunicode = hasunicode();

			ep.setsvalue(slabel);

			// If the label's encoding doesn't match the request, map.
			ep.mapunicode(isunicode, (which == P_UNICODE_TEXT || which == P_UNICODE_LABEL));
		}
		break;
#endif /* MCGraphic::getprop */
	default:
		return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCGraphic::getarrayprop(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
#ifdef /* MCGraphic::getarrayprop */ LEGACY_EXEC
	switch(which)
	{
	case P_GRADIENT_FILL:
		return MCGradientFillGetProperty(m_fill_gradient, ep, key);
	break;
	case P_GRADIENT_STROKE:
		return MCGradientFillGetProperty(m_stroke_gradient, ep, key);
	break;
	
	default:
		return MCControl::getarrayprop(parid, which, ep, key, effective);
	}
	return ES_NORMAL;
#endif /* MCGraphic::getarrayprop */
}

Exec_stat MCGraphic::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = True;
	int2 i1;
	MCRectangle drect = rect;
	MCString data = ep.getsvalue();

	switch (p)
	{
#ifdef /* MCGraphic::setprop */ LEGACY_EXEC
	case P_ANTI_ALIASED:
		if (!MCU_matchflags(data, flags, F_G_ANTI_ALIASED, dirty))
		{
			MCeerror -> add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;

	// PROPERTY - FILL RULE
	case P_FILL_RULE:
	{
		uint1 t_new_fill_rule;
		if (ep . getsvalue() == "evenodd")
			t_new_fill_rule = kMCFillRuleEvenOdd;
		else if (ep . getsvalue() == "nonzero")
			t_new_fill_rule = kMCFillRuleNonZero;
		else if (ep . getsvalue() == "none" || ep . getsvalue() == "")
			t_new_fill_rule = kMCFillRuleNone;
		else
		{
			MCeerror->add(EE_GRAPHIC_BADFILLRULE, 0, 0, data);
			return ES_ERROR;
		}

		if (t_new_fill_rule != getfillrule())
		{
			setfillrule(t_new_fill_rule);
			dirty = True;
		}
	}
	break;
	case P_CAP_STYLE:
	{
		uint2 t_new_style;
		if (ep.getsvalue() == "butt")
			t_new_style = CapButt;
		else if (ep.getsvalue() == "round")
			t_new_style = CapRound;
		else if (ep.getsvalue() == "square")
			t_new_style = CapProjecting;
		else
		{
			MCeerror->add(EE_GRAPHIC_BADCAPSTYLE, 0, 0, data);
			return ES_ERROR;
		}
		if (t_new_style != getcapstyle())
		{
			setcapstyle(t_new_style);
			dirty = true;
			compute_minrect();
		}
	}
	break;
	case P_JOIN_STYLE:
	{
		uint2 t_new_style;
		if (ep.getsvalue() == "bevel")
			t_new_style = JoinBevel;
		else if (ep.getsvalue() == "round")
			t_new_style = JoinRound;
		else if (ep.getsvalue() == "miter")
			t_new_style = JoinMiter;
		else
		{
			MCeerror->add(EE_GRAPHIC_BADJOINSTYLE, 0, 0, data);
			return ES_ERROR;
		}
		if (t_new_style != getjoinstyle())
		{
			setjoinstyle(t_new_style);
			dirty = true;
			compute_minrect();
		}
	}
	break;
	case P_MITER_LIMIT:
	{
		real8 t_new_limit;
		if (!MCU_stor8(data, t_new_limit))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		t_new_limit = MCU_fmax(1, t_new_limit);
		if (m_stroke_miter_limit != t_new_limit)
		{
			dirty = true;
			m_stroke_miter_limit = (real4)t_new_limit;
			compute_minrect();
		}
	}
	break;
	case P_BACK_COLOR:
	case P_BACK_PATTERN:
		if (data.getlength() != 0 && m_fill_gradient != NULL)
		{
			MCGradientFillFree(m_fill_gradient);
			m_fill_gradient = NULL;
		}
		return MCControl::setprop(parid, p, ep, effective);
	break;
	case P_FORE_COLOR:
	case P_FORE_PATTERN:
		if (data.getlength() != 0 && m_stroke_gradient != NULL)
		{
			MCGradientFillFree(m_stroke_gradient);
			m_stroke_gradient = NULL;
		}
		return MCControl::setprop(parid, p, ep, effective);
	break;
	case P_EDIT_MODE:
		{
			MCEditMode t_old_mode;
			if (m_edit_tool == NULL)
				t_old_mode = kMCEditModeNone;
			else
				t_old_mode = m_edit_tool->type();

			MCEditMode t_new_mode;
			if (ep.getsvalue() == "none" || ep.isempty())
			{
				t_new_mode = kMCEditModeNone;
			}
			else if (ep.getsvalue() == "fillgradient")
			{
				t_new_mode = kMCEditModeFillGradient;
			}
			else if (ep.getsvalue() == "strokegradient")
			{
				t_new_mode = kMCEditModeStrokeGradient;
			}
			else if (ep.getsvalue() == "polygon")
			{
				t_new_mode = kMCEditModePolygon;
			}
			else
			{
				MCeerror->add(EE_GRAPHIC_BADEDITMODE, 0, 0, data);
				return ES_ERROR;
			}
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
		}
		break;
	break;
	case P_FILLED:
	case P_OPAQUE:
		if (!MCU_matchflags(data, flags, F_OPAQUE, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && flags & F_OPAQUE)
		{
			closepolygon(realpoints, nrealpoints);
		}
		break;
	case P_LINE_SIZE:
	case P_PEN_WIDTH:
	case P_PEN_HEIGHT:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		linesize = i1;
		compute_minrect();
		delpoints();
		break;
	case P_POLY_SIDES:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		nsides = MCU_max(i1, 3);
		delpoints();
		break;
	case P_POINTS:
		if (oldpoints != NULL)
		{
			delete oldpoints;
			oldpoints = NULL;
		}
		MCU_parsepoints(realpoints, nrealpoints, data);
		if (flags & F_OPAQUE)
			closepolygon(realpoints, nrealpoints);
		compute_minrect();
		break;
	case P_RELATIVE_POINTS:
		{
			if (oldpoints != NULL)
			{
				delete oldpoints;
				oldpoints = NULL;
			}
			MCRectangle trect = reduce_minrect(rect);
			MCU_parsepoints(realpoints, nrealpoints, data);
			MCU_offset_points(realpoints, nrealpoints, trect.x, trect.y);
			if (flags & F_OPAQUE)
				closepolygon(realpoints, nrealpoints);
			compute_minrect();
		}
		break;
	case P_ANGLE:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		while (i1 < 0)
			i1 += 360;
		i1 %= 360;
		if (getstyleint(flags) == F_OVAL)
			startangle = i1;
		else
			angle = i1;
		delpoints();
		break;
	case P_START_ANGLE:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		while (i1 < 0)
			i1 += 360;
		startangle = i1 % 360;
		break;
	case P_ARC_ANGLE:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		arcangle = MCU_max(MCU_min(360, i1), 0);
		break;
	case P_ROUND_RADIUS:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		roundradius = i1;
		delpoints();
		break;
	case P_ARROW_SIZE:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		arrowsize = i1;
		compute_minrect();
		break;
	case P_START_ARROW:
		if (!MCU_matchflags(data, flags, F_START_ARROW, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		compute_minrect();
		break;
	case P_END_ARROW:
		if (!MCU_matchflags(data, flags, F_END_ARROW, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		compute_minrect();
		break;
	case P_MARKER_LSIZE:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_GRAPHIC_NAN, 0, 0, data);
			return ES_ERROR;
		}
		markerlsize = i1;
		compute_minrect();
		break;
	case P_MARKER_DRAWN:
		if (!MCU_matchflags(data, flags, F_MARKER_DRAWN, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		compute_minrect();
		break;
	case P_MARKER_OPAQUE:
		if (!MCU_matchflags(data, flags, F_MARKER_OPAQUE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && flags & F_MARKER_OPAQUE)
			closepolygon(markerpoints, nmarkerpoints);
		break;
	case P_MARKER_POINTS:
		if (data == MCnullmcstring)
			flags &= ~F_MARKER_DRAWN;
		else
		{
			MCU_parsepoints(markerpoints, nmarkerpoints, data);
			flags |= F_MARKER_DRAWN;
		}
		if (flags & F_MARKER_DRAWN && flags & F_MARKER_OPAQUE)
			closepolygon(markerpoints, nmarkerpoints);
		compute_minrect();
		break;
	case P_DASHES:
		{
			uint1 *newdashes = NULL;
			uint2 newndashes = 0;
			uint4 l = data.getlength();
			const char *sptr = data.getstring();
			uint4 t_dash_len = 0;
			while (l)
			{
				Boolean done;
				i1 = MCU_strtol(sptr, l, ',', done);
				if (i1 < 0 || i1 >= 256 || !done)
				{
					MCeerror->add(EE_GRAPHIC_NAN, 0, 0, data);
					delete newdashes;
					return ES_ERROR;
				}
				t_dash_len += i1;
				MCU_realloc((char **)&newdashes, newndashes, newndashes + 1, 1);
				newdashes[newndashes++] = (uint1)i1;
			}
			if (newndashes > 0 && t_dash_len == 0)
			{
				delete newdashes;
				newdashes = NULL;
				newndashes = 0;
			}
			delete dashes;
			dashes = newdashes;
			ndashes = newndashes;
			if (newndashes == 0)
				flags &= ~F_DASHES;
			else
				flags |= F_DASHES;
		}
		break;
	case P_ROUND_ENDS:
		if (!MCU_matchflags(data, flags, F_CAPROUND, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DONT_RESIZE:
		if (!MCU_matchflags(data, flags, F_DONT_RESIZE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_STYLE:
		flags &= ~F_STYLE;
		if (data == MCrectanglestring)
			flags |= F_G_RECTANGLE;
		else if (data == MCroundrectstring)
			flags |= F_ROUNDRECT;
		else if (data == MCpolygonstring)
		{
			flags |= F_POLYGON;
			compute_minrect();
		}
		else if (data == MCovalstring)
			flags |= F_OVAL;
		else if (data == MCarcstring)
		{
			flags |= F_OVAL;
			arcangle = 90;
		}
		else if (data == MCregularstring)
			flags |= F_REGULAR;
		else if (data == MCcurvestring)
		{
			flags |= F_CURVE;
			compute_minrect();
		}
		else if (data == MCtextstring)
		{
			flags |= F_G_RECTANGLE | F_G_SHOW_NAME;
			linesize = 0;
		}
		else if (data == MClinestring)
		{
			flags |= F_LINE;
			compute_minrect();
		}
		delpoints();
		break;
	case P_SHOW_NAME:
		if (!MCU_matchflags(data, flags, F_G_SHOW_NAME, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for setting the unicodeLabel.
	case P_LABEL:
	case P_UNICODE_LABEL:
		if (label == NULL ||
			data.getlength() != labelsize ||
			memcmp(data.getstring(), label, data.getlength()) != 0 ||
			(p == P_UNICODE_LABEL) != hasunicode())
		{
			delete label;
			label = NULL;
			if (data != MCnullmcstring)
			{
				labelsize = data.getlength();
				label = new char[labelsize];
				memcpy(label, data.getstring(), labelsize);
				flags |= F_G_LABEL;

				// If we are setting the unicode label we become unicode; else we revert
				// to native.
				if (p == P_UNICODE_LABEL)
					m_font_flags |= FF_HAS_UNICODE;
				else
					m_font_flags &= ~FF_HAS_UNICODE;
			}
			else
			{
				label = NULL;
				labelsize = 0;
				flags &= ~F_G_LABEL;
			}
		}
		else
			dirty = False;
		break;
#endif /* MCGraphic::setprop */
	default:
		return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty)
	{
		if (rect.x != drect.x || rect.y != drect.y
		        || rect.width != drect.width || rect.height != drect.height)
			if (resizeparent())
				dirty = False;
		if (dirty && opened)
		{
			// MW-2011-08-18: [[ Layers ]] Notify of rect changed and invalidate.
			layer_rectchanged(drect, true);
		}
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
Exec_stat MCGraphic::setarrayprop(uint4 parid, Properties which, MCExecPoint& ep, MCNameRef key, Boolean effective)
{
#ifdef /* MCGraphic::setarrayprop */ LEGACY_EXEC
	Boolean dirty;
	dirty = False;
	switch(which)
	{
	case P_GRADIENT_FILL:
	{
		if (MCGradientFillSetProperty(m_fill_gradient, ep, key, dirty, rect) == ES_ERROR)
			return ES_ERROR;
		if (m_fill_gradient != NULL)
		{
			setcolor(P_BACK_COLOR - P_FORE_COLOR, MCnullmcstring);
			setpattern(P_BACK_PATTERN - P_FORE_PATTERN, MCnullmcstring);
		}
	}
	break;
	case P_GRADIENT_STROKE:
	{
		if (MCGradientFillSetProperty(m_stroke_gradient, ep, key, dirty, rect) == ES_ERROR)
			return ES_ERROR;
		if (m_stroke_gradient != NULL)
		{
			setcolor(P_FORE_COLOR - P_FORE_COLOR, MCnullmcstring);
			setpattern(P_FORE_COLOR - P_FORE_COLOR, MCnullmcstring);
		}
	}
	break;
	default:
		return MCControl::setarrayprop(parid, which, ep, key, effective);
	}

	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	
	return ES_NORMAL;
#endif /* MCGraphic::setarrayprop */
}

MCControl *MCGraphic::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCGraphic *newgraphic = new MCGraphic(*this);
	if (attach)
		newgraphic->attach(p, invisible);
	return newgraphic;
}

Boolean MCGraphic::maskrect(const MCRectangle &srect)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles))
		return False;
	MCRectangle drect = MCU_intersect_rect(srect, rect);
	if (drect.width == 0 || drect.height == 0)
		return False;
	if (state & CS_SELECTED || drect.width > 2 || drect.height > 2)
		return True;
	if (m_edit_tool != NULL && m_edit_tool->handle_under_point(drect.x, drect.y) != -1)
		return True;
	switch (getstyleint(flags))
	{
	case F_G_RECTANGLE:
	case F_ROUNDRECT:
		if (flags & F_OPAQUE)
			return True;
		else
		{
			uint2 b = linesize;
			if (flags & F_SHOW_BORDER)
				b += borderwidth;
			MCRectangle irect = MCU_reduce_rect(rect, b);
			return !MCU_point_in_rect(irect, srect.x, srect.y);
		}
	case F_REGULAR:
		if (MCU_point_on_line(points, npoints, srect.x, srect.y, linesize))
			return True;
		if (flags & F_OPAQUE)
			return MCU_point_in_polygon(points, npoints, srect.x, srect.y);
		break;
	case F_LINE:
		if (MCU_point_on_line(realpoints, nrealpoints, srect.x, srect.y, linesize))
			return True;
		break;
	case F_POLYGON:
	case F_CURVE:
		if (nrealpoints <= 1)
			return True;
		if (flags & F_OPAQUE
		        && MCU_point_in_polygon(realpoints, nrealpoints, srect.x, srect.y))
			return True;
		if (MCU_point_on_line(realpoints, nrealpoints, srect.x, srect.y, linesize))
			return True;
		break;
	case F_OVAL:
		// MW-2011-09-23: [[ Bug ]] If the oval <= 4 on either side, assume intersect.
		if (rect . width <= 4 || rect . height <= 4)
			return True;

		int32_t hw = (rect.width >> 1) + 1;
		int32_t hh = (rect.height >> 1) + 1;
		int2 cx = rect.x + hw;
		int2 cy = rect.y + hh;
		real8 dx = (real8)MCU_abs(srect.x - cx) / (real8) hw;
		real8 dy = (real8)MCU_abs(srect.y - cy) / (real8) hh;
		if (dx * dx + dy * dy > 1.0)
			return False;
		if (flags & F_OPAQUE )
			return True;
		hh -= linesize + 2;
		hw -= linesize + 2;
		if (hh <= 0 || hw <= 0)
			return True;

		dx = (real8)MCU_abs(srect.x - cx) / (real8) hw;
		dy = (real8)MCU_abs(srect.y - cy) / (real8) hh;
		if (dx * dx + dy * dy > 1.0)
			return True;
		else
			return False;
	}
	return False;
}

void MCGraphic::fliph()
{
	uint2 i;
	if (oldpoints != NULL)
		for (i = 0 ; i < nrealpoints ; i++)
			if (oldpoints[i].x != MININT2)
				oldpoints[i].x = oldrect.width - oldpoints[i].x;
	if (m_fill_gradient != NULL && m_fill_gradient->old_origin.x != MININT2)
	{
		m_fill_gradient->old_origin.x = m_fill_gradient->old_rect.width - m_fill_gradient->old_origin.x;
		m_fill_gradient->old_primary.x = m_fill_gradient->old_rect.width - m_fill_gradient->old_primary.x;
		m_fill_gradient->old_secondary.x = m_fill_gradient->old_rect.width - m_fill_gradient->old_secondary.x;
	}
	if (m_stroke_gradient != NULL && m_stroke_gradient->old_origin.x != MININT2)
	{
		m_stroke_gradient->old_origin.x = m_stroke_gradient->old_rect.width - m_stroke_gradient->old_origin.x;
		m_stroke_gradient->old_primary.x = m_stroke_gradient->old_rect.width - m_stroke_gradient->old_primary.x;
		m_stroke_gradient->old_secondary.x = m_stroke_gradient->old_rect.width - m_stroke_gradient->old_secondary.x;
	}
}

void MCGraphic::flipv()
{
	uint2 i;
	if (oldpoints != NULL)
		for (i = 0 ; i < nrealpoints ; i++)
			if (oldpoints[i].y != MININT2)
				oldpoints[i].y = oldrect.height - oldpoints[i].y;
	if (m_fill_gradient != NULL && m_fill_gradient->old_origin.y != MININT2)
	{
		m_fill_gradient->old_origin.y = m_fill_gradient->old_rect.height - m_fill_gradient->old_origin.y;
		m_fill_gradient->old_primary.y = m_fill_gradient->old_rect.height - m_fill_gradient->old_primary.y;
		m_fill_gradient->old_secondary.y = m_fill_gradient->old_rect.height - m_fill_gradient->old_secondary.y;
	}
	if (m_stroke_gradient != NULL && m_stroke_gradient->old_origin.y != MININT2)
	{
		m_stroke_gradient->old_origin.y = m_stroke_gradient->old_rect.height - m_stroke_gradient->old_origin.y;
		m_stroke_gradient->old_primary.y = m_stroke_gradient->old_rect.height - m_stroke_gradient->old_primary.y;
		m_stroke_gradient->old_secondary.y = m_stroke_gradient->old_rect.height - m_stroke_gradient->old_secondary.y;
	}
}

uint2 MCGraphic::get_arrow_size()
{
	return arrowsize + 2 + ((arrowsize + 8) * linesize >> 3);
}

void MCGraphic::draw_arrow(MCDC *dc, MCPoint &p1, MCPoint &p2)
{
	real8 dx = p2.x - p1.x;
	real8 dy = p2.y - p1.y;
	if (arrowsize == 0 || dx == 0.0 && dy == 0.0)
		return;
	MCPoint pts[3];
	real8 angle = atan2(dy, dx);
	real8 a1 = angle + 3.0 * M_PI / 4.0;
	real8 a2 = angle - 3.0 * M_PI / 4.0;
	real8 size = get_arrow_size();
	pts[0].x = p2.x + (int2)(cos(angle) * size);
	pts[0].y = p2.y + (int2)(sin(angle) * size);
	pts[1].x = p2.x + (int2)(cos(a1) * size);
	pts[1].y = p2.y + (int2)(sin(a1) * size);
	pts[2].x = p2.x + (int2)(cos(a2) * size);
	pts[2].y = p2.y + (int2)(sin(a2) * size);
	dc->fillpolygon(pts, 3);
}

void MCGraphic::draw_lines(MCDC *dc, MCPoint *pts, uint2 npts)
{
	uint2 i = 0;
	while (i < npts)
	{
		bool t_degenerate;
		t_degenerate = true;
		
		uint2 count = 0;
		while (i + count < npts && pts[count + i].x != MININT2)
		{
			if (count > 0 &&
				(pts[i + count - 1] . x != pts[i + count] . x ||
				 pts[i + count - 1] . y != pts[i + count] . y))
				t_degenerate = false;
			count++;
		}
		
		// MW-2010-10-22: [[ Bug 8107 ]] Make sure we only 'auto-close' if the first and last points coincide.
		if (!getflag(F_G_ANTI_ALIASED) || !t_degenerate)
		{
			if (count > 1)
				dc->drawlines(&pts[i], count, pts[i].x == pts[i + count - 1].x && pts[i].y == pts[i + count - 1].y);
			else
			{
				MCPoint t_dot[2];
				t_dot[0] = pts[i];
				t_dot[1] = pts[i];
				dc -> drawlines(&t_dot[0], 2, false);
			}
		}
		else if (getcapstyle() != CapButt)
		{
			MCPath *t_path;
			t_path = MCPath::create_dot(pts[i] . x, pts[i] . y, true);
			dc -> drawpath(t_path);
			t_path -> release();
		}
			
		i += count + 1;
	}
}

void MCGraphic::fill_polygons(MCDC *dc, MCPoint *pts, uint2 npts)
{
	if (getfillrule() == kMCFillRuleNone || !getflag(F_G_ANTI_ALIASED))
	{
		uint2 i = 0;
		while (i < npts)
		{
			uint2 count = 0;
			Boolean vx = False;
			Boolean vy = False;
			while (i + count < npts && pts[count + i].x != MININT2)
			{
				if (pts[count + i].x != pts[i].x)
					vx = True;
				if (pts[count + i].y != pts[i].y)
					vy = True;
				count++;
			}
			if (vx && vy && count > 2) // mac crashes on print if !vx || !vy
				dc->fillpolygon(&pts[i], count);
			i += count + 1;
		}
	}
	else
	{
		MCPath *t_path;
		t_path = MCPath::create_polypolygon(pts, npts, true);
		dc -> fillpath(t_path, getfillrule() == kMCFillRuleEvenOdd);
		t_path -> release();
	}
}

void MCGraphic::compute_extents(MCPoint *pts, uint2 npts, int2 &minx,
                                int2 &miny, int2 &maxx, int2 &maxy)
{
	minx = miny = MAXINT2;
	maxx = maxy = MININT2;
	uint2 i;
	for (i = 0 ; i < npts ; i++)
		if (pts[i].x != MININT2)
		{
			if (pts[i].x > maxx)
				maxx = pts[i].x;
			if (pts[i].x < minx)
				minx = pts[i].x;
			if (pts[i].y > maxy)
				maxy = pts[i].y;
			if (pts[i].y < miny)
				miny = pts[i].y;
		}
}

MCRectangle MCGraphic::expand_minrect(const MCRectangle &srect)
{
	MCRectangle trect;
	if (flags & F_DONT_RESIZE)
		return srect;
	if (linesize)
	{
		uint2 t_linesize = linesize;
		switch (getstyleint(flags))
		{
		case F_POLYGON:
		case F_CURVE:
			if (getjoinstyle() == JoinMiter && m_stroke_miter_limit > 1)
				t_linesize = (uint2)ceil(t_linesize * m_stroke_miter_limit);
		case F_LINE:
			if (getcapstyle() == CapProjecting)
				t_linesize = MCU_max(t_linesize, (uint2)ceil(t_linesize * 1.414213562));
		case F_REGULAR:
			if (flags & (F_START_ARROW | F_END_ARROW))
				trect = MCU_reduce_rect(srect, -get_arrow_size());
			else
				trect = MCU_reduce_rect(srect, (int2)-((t_linesize >> 1) + 1));
			break;
		default:
			trect = MCU_reduce_rect(srect, -((t_linesize >> 1) + 1));
			break;
		}
	}
	else
		trect = rect;
	if (getstyleint(flags) == F_POLYGON)
	{
		if (flags & F_MARKER_DRAWN)
		{
			int2 minx, miny, maxx, maxy;
			compute_extents(markerpoints, nmarkerpoints, minx, miny, maxx, maxy);
			if (maxx >= minx && maxy >= miny)
			{
				if (minx < 0)
				{
					trect.x += minx;
					trect.width -= minx;
				}
				if (miny < 0)
				{
					trect.y += miny;
					trect.height -= miny;
				}
				if (maxx > 0)
					trect.width += maxx;
				if (maxy > 0)
					trect.height += maxy;
				trect = MCU_reduce_rect(trect, -((markerlsize >> 1) + 1));
			}
		}
		else
		{
			if (trect.width < 8)
			{
				uint2 dx = 8 - trect.width;
				trect.x -= dx >> 1;
				trect.width += dx;
			}
			if (trect.height < 8)
			{
				uint2 dy = 8 - trect.height;
				trect.y -= dy >> 1;
				trect.height += dy;
			}
		}
	}

	return trect;
}

MCRectangle MCGraphic::reduce_minrect(const MCRectangle &srect)
{
	MCRectangle trect;

	if (flags & F_DONT_RESIZE)
		return srect;
	
	// MW-2007-03-08: [[ Bug 4095 ]] - if linesize is 0 then the minimum rectangle should be srect not rect!
	if (linesize)
	{
		uint2 t_linesize = linesize;
		switch (getstyleint(flags))
		{
		case F_CURVE:
		case F_POLYGON:
			if (getjoinstyle() == JoinMiter && m_stroke_miter_limit > 1)
				t_linesize = (uint2)ceil(t_linesize * m_stroke_miter_limit);
		case F_LINE:
			if (getcapstyle() == CapProjecting)
				t_linesize = MCU_max(t_linesize, (uint2)ceil(t_linesize * 1.414213562));
		case F_REGULAR:
			if (flags & (F_START_ARROW | F_END_ARROW))
				trect = MCU_reduce_rect(srect, get_arrow_size());
			else
				trect = MCU_reduce_rect(srect, (int2)((t_linesize >> 1) + 1));
			break;
		default:
			trect = MCU_reduce_rect(srect, (t_linesize >> 1) + 1);
			break;
		}
	}
	else
		trect = srect;
	
	if (getstyleint(flags) == F_POLYGON && flags & F_MARKER_DRAWN)
	{
		int2 minx, miny, maxx, maxy;
		compute_extents(markerpoints, nmarkerpoints, minx, miny, maxx, maxy);
		if (maxx >= minx && maxy >= miny)
		{
			if (minx < 0)
			{
				trect.x -= minx;
				trect.width += minx;
			}
			if (miny < 0)
			{
				trect.y -= miny;
				trect.height += miny;
			}
			if (maxx > 0)
				trect.width -= maxx;
			if (maxy > 0)
				trect.height -= maxy;
			trect = MCU_reduce_rect(trect, (markerlsize >> 1) + 1);
		}
		if (trect.width == 0)
			trect.width = 1;
		if (trect.height == 0)
			trect.height = 1;
	}

	return trect;
}

void MCGraphic::compute_minrect()
{
	if (!(flags & F_DONT_RESIZE)
	        && (getstyleint(flags) == F_POLYGON || getstyleint(flags) == F_CURVE
	            || getstyleint(flags) == F_LINE))
	{
		int2 minx, miny, maxx, maxy;
		compute_extents(realpoints, nrealpoints, minx, miny, maxx, maxy);
		if (maxx >= minx && maxy >= miny)
		{
			rect.x = minx;
			rect.y = miny;
			rect.width = maxx - minx;
			rect.height = maxy - miny;
			rect = expand_minrect(rect);
		}
	}
}

MCRectangle MCGraphic::geteffectiverect(void) const
{
	MCRectangle t_effectiverect;
	t_effectiverect = MCControl::geteffectiverect();

	if (m_edit_tool != NULL)
	{
		MCRectangle t_tool_rect = m_edit_tool->drawrect();
		if (t_tool_rect.width != 0 && t_tool_rect.height != 0)
			t_effectiverect = MCU_union_rect(t_effectiverect, t_tool_rect);
	}

	return t_effectiverect;
}

void MCGraphic::delpoints()
{
	if (points != NULL)
	{
		delete points;
		points = NULL;
		npoints = 0;
	}
}

void MCGraphic::closepolygon(MCPoint *&pts, uint2 &npts)
{
	if (getstyleint(flags) == F_POLYGON && npts > 1)
	{
		uint2 i ;
		uint2 t_count = 0 ;
		MCPoint *startpt = pts ;
		for (i=1; i<=npts; i++)
		{
			if ((i==npts) || (pts[i].x == MININT2))
			{
				if (pts[i - 1].x != startpt->x || pts[i - 1].y != startpt->y)
				{
					t_count++ ;
				}
				startpt = pts+i+1 ;
			}
		}
		if (t_count)
		{
			MCPoint *newpts = new MCPoint[npts+t_count] ;
			startpt = pts ;
			MCPoint *currentpt = newpts ;
			*(currentpt++) = pts[0] ;
			for (i=1; i<npts; i++)
			{
				if (pts[i].x == MININT2)
				{
					if (pts[i - 1].x != startpt->x || pts[i - 1].y != startpt->y)
					{
						*(currentpt++) = *startpt ;
					}
					startpt = pts+i+1 ;
				}
				*(currentpt++) = pts[i] ;
			}
			if (pts[npts - 1].x != startpt->x || pts[npts - 1].y != startpt->y)
			{
				*(currentpt++) = *startpt ;
			}
			
			delete pts;
			pts = newpts;
			npts += t_count ;
			delete oldpoints;
			oldpoints = NULL;
		}
	}
}

void MCGraphic::getlabeltext(MCString &s, bool& isunicode)
{
	// MW-2012-02-29: [[ Bug 10038 ]] Make sure we set the 'isunicode' param to the
	//   appropriate value.
	if (label != NULL)
	{
		s.set(label,labelsize);

		// Whether the label is unicode depends on the hasunicode() setting.
		isunicode = hasunicode();
	}
	else
	{
		s = getname_oldstring();

		// The name is always native (at the moment).
		isunicode = false;
	}
}

void MCGraphic::drawlabel(MCDC *dc, int2 sx, int sy, uint2 twidth, const MCRectangle &srect, const MCString &s, bool isunicode, uint2 fstyle)
{
	MCFontDrawText(m_font, s.getstring(), s.getlength(), isunicode, dc, sx, sy, False);
	if (fstyle & FA_UNDERLINE)
		dc->drawline(sx, sy + 1, sx + twidth, sy + 1);
	if (fstyle & FA_STRIKEOUT)
		dc->drawline(sx, sy - (MCFontGetAscent(m_font) >> 1), sx + twidth, sy - (MCFontGetAscent(m_font) >> 1));
}

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCGraphic::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	dc -> setquality(getflag(F_G_ANTI_ALIASED) ? QUALITY_SMOOTH : QUALITY_DEFAULT);

	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}
		
		// MW-2009-06-10: [[ Bitmap Effects ]]
		if (m_bitmap_effects == NULL)
			dc -> begin(dc -> gettype() == CONTEXT_TYPE_PRINTER && (m_fill_gradient != NULL) || (m_stroke_gradient != NULL));
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, rect))
				return;
			dirty = dc -> getclip();
		}
	}

	MCRectangle trect = rect;
	if (MClook == LF_MOTIF && state & CS_KFOCUSED
	        && !(extraflags & EF_NO_FOCUS_BORDER))
		drawfocus(dc, p_dirty);
	if (flags & F_SHOW_BORDER)
		trect = MCU_reduce_rect(trect, borderwidth);
	if (points == NULL && getstyleint(flags) == F_REGULAR)
	{
		if (npoints <= nsides)
		{
			npoints = nsides + 1;
			if (points != NULL)
				delete points;
			points = new MCPoint[npoints];
		}
		real8 dx = (real8)((trect.width >> 1) - 1);
		real8 dy = (real8)((trect.height >> 1) - 1);
		real8 rangle = (real8)angle * 2.0 * M_PI / 360.0;
		int2 cx = trect.x + (trect.width >> 1);
		int2 cy = trect.y + (trect.height >> 1);
		real8 factor = 2.0 * M_PI / (real8)nsides;
		uint2 i;
		for (i = 0 ; i < nsides ; i++)
		{
			real8 iangle = rangle + (real8)i * factor;
			points[i].x = cx + (int2)(cos(iangle) * dx);
			points[i].y = cy + (int2)(sin(iangle) * dy);
		}
		points[nsides] = points[0];
	}
	if (flags & F_OPAQUE)
	{
		setforeground(dc, DI_BACK, False);
		if (m_fill_gradient != NULL)
			dc->setgradient(m_fill_gradient);
		switch (getstyleint(flags))
		{
		case F_G_RECTANGLE:
			dc->fillrect(trect);
			break;
		case F_ROUNDRECT:
			dc->fillroundrect(trect, roundradius);
			break;
		case F_REGULAR:
			dc->fillpolygon(points, npoints);
			break;
		case F_POLYGON:
		case F_CURVE:
			fill_polygons(dc, realpoints, nrealpoints);
			break;
		case F_OVAL:
			dc->fillarc(trect, startangle, arcangle);
			break;
		}
		if (m_fill_gradient != NULL)
			dc->setgradient(NULL);
	}
	if (linesize != 0)
	{
		int4 lstyle, lends, jstyle;
		setforeground(dc, DI_FORE, False);
		if (m_stroke_gradient != NULL)
			dc->setgradient(m_stroke_gradient);
		if (dashes)
		{
			lstyle = LineOnOffDash;
			dc->setdashes(0, dashes, ndashes);
		}
		else
			lstyle = LineSolid;

		jstyle = getjoinstyle();
		switch (getstyleint(flags))
		{
		case F_G_RECTANGLE:
			dc->setlineatts(linesize, lstyle, CapButt, JoinMiter);
				dc->setmiterlimit(10.0);
				// MW-2013-09-06: [[ RefactorGraphics ]] Make sure we draw on the inside of the rect.
			dc->drawrect(trect, true);
			break;
		case F_ROUNDRECT:
			dc->setlineatts(linesize, lstyle, CapButt, JoinBevel);
			// MW-2013-09-06: [[ RefactorGraphics ]] Make sure we draw on the inside of the rect.
			dc->drawroundrect(trect, roundradius, true);
			break;
		case F_REGULAR:
			dc->setlineatts(linesize, lstyle, CapButt, JoinRound);
			dc->drawlines(points, npoints, true);
			break;
		case F_LINE:
		case F_POLYGON:
		case F_CURVE:
			if (!(flags & F_G_ANTI_ALIASED) && (flags & (F_START_ARROW | F_END_ARROW)))
				lends = CapButt;
			else
			{
				lends = getcapstyle();
				if (flags & F_START_ARROW)
					lends |= NoStartCap;
				if (flags & F_END_ARROW)
					lends |= NoEndCap;
			}

			dc->setlineatts(linesize, lstyle, lends, jstyle);
			if (jstyle == JoinMiter)
				dc->setmiterlimit(m_stroke_miter_limit);
			draw_lines(dc, realpoints, nrealpoints);
			if (nrealpoints > 1)
			{
				if (flags & F_START_ARROW)
					draw_arrow(dc, realpoints[1], realpoints[0]);
				if (flags & F_END_ARROW)
					draw_arrow(dc, realpoints[nrealpoints - 2],
					           realpoints[nrealpoints - 1]);
			}
			break;
		case F_OVAL:
			dc->setlineatts(linesize, lstyle, CapButt, JoinBevel);
			// MW-2013-09-06: [[ RefactorGraphics ]] Make sure we draw on the inside of the rect.
			if (getflag(F_OPAQUE) && arcangle != 360)
				dc -> drawsegment(trect, startangle, arcangle, true);
			else
				dc -> drawarc(trect, startangle, arcangle, true);
			break;
		}
		if (m_stroke_gradient != NULL)
			dc->setgradient(NULL);
	}
	// MW-2012-09-21: [[ Bug 10163 ]] We should draw markers either if the markerLineSize
	//   is non-zero, or if the markers are filled.
	if (flags & F_MARKER_DRAWN && nrealpoints != 0 && nmarkerpoints != 0
	        && (markerlsize != 0 || getflag(F_MARKER_OPAQUE)) && getstyleint(flags) == F_POLYGON)
	{
		setforeground(dc, DI_HILITE, False);
		dc->setlineatts(markerlsize, LineSolid, CapRound, JoinRound);
		uint2 i;
		uint2 last = MAXUINT2;
		for (i = 0 ; i < nrealpoints ; i++)
		{
			if (realpoints[i].x != MININT2)
			{
				if (last != MAXUINT2)
					MCU_offset_points(markerpoints, nmarkerpoints,
					                  realpoints[i].x - realpoints[last].x,
					                  realpoints[i].y - realpoints[last].y);
				else
					MCU_offset_points(markerpoints, nmarkerpoints,
					                  realpoints[i].x, realpoints[i].y);
				last = i;
				if (flags & F_MARKER_OPAQUE)
				{
					setforeground(dc, DI_BORDER, False);
					fill_polygons(dc, markerpoints, nmarkerpoints);
					setforeground(dc, DI_HILITE, False);
				}
				// MW-2012-09-21: [[ Bug 10163 ]] Only draw the border round the markers
				//   if lineSize is not 0.
				if (markerlsize != 0)
					draw_lines(dc, markerpoints, nmarkerpoints);
			}
		}
		if (last != MAXUINT2)
			MCU_offset_points(markerpoints, nmarkerpoints,
			                  -realpoints[last].x, -realpoints[last].y);
	}
	MCString slabel;
	bool isunicode;
	getlabeltext(slabel, isunicode);
	if (flags & F_G_SHOW_NAME &&  slabel.getstring() != NULL)
	{
		setforeground(dc, DI_FORE, False);

		MCString *lines = NULL;
		uint2 nlines = 0;
		MCU_break_string(slabel, lines, nlines, isunicode);

		uint2 fheight;
		fheight = gettextheight();

		uint2 fontstyle;
		fontstyle = gettextstyle();

		int32_t fascent, fdescent;
		fascent = MCFontGetAscent(m_font);
		fdescent = MCFontGetDescent(m_font);

		int2 centerx = trect.x + leftmargin + ((trect.width - leftmargin - rightmargin) >> 1);
		int2 centery = trect.y + topmargin + ((trect.height - topmargin - bottommargin) >> 1);
		uint2 theight = nlines == 1 ? fascent : nlines * fheight;
		int2 sx = trect.x + leftmargin + borderwidth - DEFAULT_BORDER;
		int2 sy = centery - (theight >> 1) + fascent;
		
		uint2 i;
		uint2 twidth = 0;
		for (i = 0 ; i < nlines ; i++)
		{
			twidth = MCFontMeasureText(m_font, lines[i].getstring(), lines[i].getlength(), isunicode);
			switch (flags & F_ALIGNMENT)
			{
			case F_ALIGN_LEFT:
			case F_ALIGN_JUSTIFY:
				break;
			case F_ALIGN_CENTER:
				sx = centerx - (twidth >> 1);
				break;
			case F_ALIGN_RIGHT:
				sx = trect.x + trect.width - rightmargin - twidth - (borderwidth - DEFAULT_BORDER);
				break;
			}
			if (flags & F_DISABLED && MClook != LF_MOTIF)
			{
				drawlabel(dc, sx + 1, sy + 1, twidth, trect, lines[i], isunicode, fontstyle);
				setforeground(dc, DI_BOTTOM, False);
			}
			drawlabel(dc, sx, sy, twidth, trect, lines[i], isunicode, fontstyle);
			sy += fheight;
		}
		if (state & CS_KFOCUSED)
		{
			if (nlines == 1)
				MCU_set_rect(trect, sx - 1, sy - fascent - fheight, twidth + 2, fascent + fdescent);
			else
				trect = MCU_reduce_rect(trect, 3);
			dc->setlineatts(0, LineOnOffDash, CapButt, JoinBevel);
			dc->setdashes(0, dotlist, 2);
			dc->drawrect(trect);
			dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
		}
		delete lines;
	}
	dc->setlineatts(0, LineSolid, CapButt, JoinBevel);

	dc -> setquality(QUALITY_DEFAULT);

	if (flags & F_SHOW_BORDER)
	{
		if (flags & F_3D)
			draw3d(dc, rect, ETCH_RAISED, borderwidth);
		else
			drawborder(dc, rect, borderwidth);
	}

	if (!p_isolated)
	{
		dc -> end();

		if (getstate(CS_SELECTED))
			drawselected(dc);
		if (m_edit_tool != NULL)
			m_edit_tool->drawhandles(dc);
	}

	dc -> setquality(QUALITY_DEFAULT);
}

MCGradientFill *MCGraphic::getgradient()
{
	return m_fill_gradient;
}

uint2 MCGraphic::getnumpoints()
{
	return nrealpoints;
}

MCPoint *MCGraphic::getpoints()
{
	return realpoints;
}


void MCGraphic::setpoint(uint4 i, int2 x, int2 y, bool redraw)
{
	if (realpoints[i].x != x || realpoints[i].y != y)
	{
		MCRectangle drect = rect;

		MCRectangle t_old_effective_rect;
		t_old_effective_rect = geteffectiverect();

		realpoints[i].x = x;
		realpoints[i].y = y;

		if (oldpoints != NULL)
		{
			delete oldpoints;
			oldpoints = NULL;
		}

		if (redraw)
		{
			if (flags & F_OPAQUE)
				closepolygon(realpoints, nrealpoints);
			compute_minrect();
			if (rect.x != drect.x || rect.y != drect.y
					|| rect.width != drect.width || rect.height != drect.height)
			{
				if (m_fill_gradient != NULL)
				{
					m_fill_gradient->old_origin.x = MININT2;
					m_fill_gradient->old_origin.y = MININT2;
				}
				if (resizeparent())
					return;
			}
			if (opened)
			{
				// MW-2011-08-18: [[ Layers ]] Notify of the change in effective rect and invalidate.
				layer_effectiverectchangedandredrawall(t_old_effective_rect);
			}
		}
	}
}

uint2 MCGraphic::getcapstyle()
{
	if (flags & F_CAPROUND)
		return CapRound;
	else if (flags & F_CAPSQUARE)
		return CapProjecting;
	else
		return CapButt;
}

void MCGraphic::setcapstyle(uint2 p_style)
{
	switch (p_style)
	{
	case CapRound:
		flags = (flags & ~F_CAPSQUARE) | F_CAPROUND;
		break;
	case CapProjecting:
		flags = (flags & ~F_CAPROUND) | F_CAPSQUARE;
		break;
	case CapButt:
		flags = flags & ~(F_CAPROUND | F_CAPSQUARE);
		break;
	}
}

uint2 MCGraphic::getjoinstyle()
{
	switch (flags & F_JOINSTYLE)
	{
	case F_JOINROUND:
		return JoinRound;
	case F_JOINBEVEL:
		return JoinBevel;
	case F_JOINMITER:
		return JoinMiter;
	default:
		assert(true);
		return 0;
	}
}

void MCGraphic::setjoinstyle(uint2 p_style)
{
	switch (p_style)
	{
	case JoinRound:
		flags = (flags & ~F_JOINSTYLE) | F_JOINROUND;
		break;
	case JoinBevel:
		flags = (flags & ~F_JOINSTYLE) | F_JOINBEVEL;
		break;
	case JoinMiter:
		flags = (flags & ~F_JOINSTYLE) | F_JOINMITER;
		break;
	}
}

uint2 MCGraphic::getfillrule()
{
	switch (flags & F_FILLRULE)
	{
	case F_FILLRULENONE:
		return kMCFillRuleNone;
	case F_FILLRULENONZERO:
		return kMCFillRuleNonZero;
	case F_FILLRULEEVENODD:
		return kMCFillRuleEvenOdd;
	default:
		assert(true);
		return 0;
	}
}

void MCGraphic::setfillrule(uint2 p_rule)
{
	switch (p_rule)
	{
	case kMCFillRuleNone:
		flags = (flags & ~F_FILLRULE) | F_FILLRULENONE;
		break;
	case kMCFillRuleNonZero:
		flags = (flags & ~F_FILLRULE) | F_FILLRULENONZERO;
		break;
	case kMCFillRuleEvenOdd:
		flags = (flags & ~F_FILLRULE) | F_FILLRULEEVENODD;
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCGraphic::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	// Extended data area for a graphic consists of:
	//   tag graphic_extensions
	//   if (tag & GRAPHIC_EXTRA_MITERLIMIT)
	//	   float miter_limit	
	//   if (tag & GRAPHIC_EXTRA_STROKEGRADIENT)
	//     gradient stroke_gradient
	//   if (tag & GRAPHIC_EXTRA_FILLGRADIENT)
	//     gradient fill_gradient
	//	 if (tag & GRAPHIC_EXTRA_MARGINS)
	//	   int16 leftmargin
	//     int16 topmargin
	//     int16 rightmargin
	//     int16 bottommargin
	//
	//   MCObject::extensions

	uint4 t_flags;
	t_flags = 0;

	uint4 t_length;
	t_length = 0;

	if (m_stroke_miter_limit != 10.0)
	{
		t_flags |= GRAPHIC_EXTRA_MITERLIMIT;
		t_length += sizeof(float);
	}

	if (m_fill_gradient != NULL)
	{
		t_flags |= GRAPHIC_EXTRA_FILLGRADIENT;
		t_length += MCGradientFillMeasure(m_fill_gradient);
	}

	if (m_stroke_gradient != NULL)
	{
		t_flags |= GRAPHIC_EXTRA_STROKEGRADIENT;
		t_length += MCGradientFillMeasure(m_stroke_gradient);
	}
	
	// MW-2008-08-12: [[ Bug 2849 ]] Margins should be saved in the stackfile
	if (leftmargin != defaultmargin || topmargin != defaultmargin ||
		rightmargin != defaultmargin || bottommargin != defaultmargin)
	{
		t_flags |= GRAPHIC_EXTRA_MARGINS;
		t_length += 8;
	}

	IO_stat t_stat;
	t_stat = p_stream . WriteTag(t_flags, t_length);
	
	if (t_stat == IO_NORMAL && m_stroke_miter_limit != 10.0)
		t_stat = p_stream . WriteFloat32(m_stroke_miter_limit);
		
	if (t_stat == IO_NORMAL && m_fill_gradient != NULL)
		t_stat = MCGradientFillSerialize(m_fill_gradient, p_stream);
		
	if (t_stat == IO_NORMAL && m_stroke_gradient != NULL)
		t_stat = MCGradientFillSerialize(m_stroke_gradient, p_stream);
		
	// MW-2008-08-12: [[ Bug 2849 ]] Margins should be saved in the stackfile
	if (t_stat == IO_NORMAL && (t_flags & GRAPHIC_EXTRA_MARGINS) != 0)
	{
		t_stat = p_stream . WriteS16(leftmargin);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteS16(topmargin);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteS16(rightmargin);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . WriteS16(bottommargin);
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part);

	return t_stat;
}

IO_stat MCGraphic::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (p_remaining > 0)
	{
		uint4 t_flags, t_length, t_header_size;
		t_stat = p_stream . ReadTag(t_flags, t_length, t_header_size);
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . Mark();

		uint4 t_corrected_length;
		t_corrected_length = 0;

		if (t_stat == IO_NORMAL && (t_flags & GRAPHIC_EXTRA_MITERLIMIT) != 0)
		{
			t_stat = p_stream . ReadFloat32(m_stroke_miter_limit);
			if (t_length == 0)
				t_corrected_length += 4;
		}

		if (t_stat == IO_NORMAL && (t_flags & GRAPHIC_EXTRA_FILLGRADIENT) != 0)
		{
			MCGradientFillInit(m_fill_gradient, rect);
			t_stat = MCGradientFillUnserialize(m_fill_gradient, p_stream);
			if (t_length == 0)
				t_corrected_length += MCGradientFillMeasure(m_fill_gradient);
		}

		if (t_stat == IO_NORMAL && (t_flags & GRAPHIC_EXTRA_STROKEGRADIENT) != 0)
		{
			MCGradientFillInit(m_stroke_gradient, rect);
			t_stat = MCGradientFillUnserialize(m_stroke_gradient, p_stream);
			if (t_length == 0)
				t_corrected_length += MCGradientFillMeasure(m_stroke_gradient);
		}

		// MW-2008-08-12: [[ Bug 2849 ]] Make sure margins are saved with the graphic object
		if (t_stat == IO_NORMAL && (t_flags & GRAPHIC_EXTRA_MARGINS) != 0)
		{
			t_stat = p_stream . ReadS16(leftmargin);
			if (t_stat == IO_NORMAL)
				t_stat = p_stream . ReadS16(topmargin);
			if (t_stat == IO_NORMAL)
				t_stat = p_stream . ReadS16(rightmargin);
			if (t_stat == IO_NORMAL)
				t_stat = p_stream . ReadS16(bottommargin);
		}

		// During the 3.0.0 development cycle, the graphic extended data area was different since it
		// was done before extendedload/save was added. This means that it is possible for flags to
		// be non-zero, and length to be zero. In this case, we rely on the file pointer not needing
		// updating so we only do the following for non-zero length.
		if (t_stat == IO_NORMAL && t_length != 0)
			t_stat = p_stream . Skip(t_length);

		if (t_stat == IO_NORMAL)
		{
			if (t_length != 0)
				p_remaining -= t_length;
			else
				p_remaining -= t_corrected_length;

			p_remaining -= t_header_size;
		}
	}

	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCGraphic::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	uint2 i;
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_GRAPHIC, stream)) != IO_NORMAL)
		return stat;

//---- 2.7+:
//  . F_G_ANTI_ALIASED now defined, default false
	uint4 t_old_flags;
	if (MCstackfileversion < 2700)
	{
		t_old_flags = flags;
		flags &= ~F_G_ANTI_ALIASED;
	}
//----

	bool t_has_extensions;
	t_has_extensions = m_stroke_gradient != NULL || m_fill_gradient != NULL || m_stroke_miter_limit != 10.0 ||
		leftmargin != defaultmargin || topmargin != defaultmargin || rightmargin != defaultmargin || bottommargin != defaultmargin;
	if ((stat = MCControl::save(stream, p_part, t_has_extensions || p_force_ext)) != IO_NORMAL)
		return stat;

//---- 2.7+:
	if (MCstackfileversion < 2700)
		flags = t_old_flags;
//----

	if ((stat = IO_write_uint2(angle, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(linesize, stream)) != IO_NORMAL)
		return stat;
	switch (flags & F_STYLE)
	{
	case F_G_RECTANGLE:
		break;
	case F_ROUNDRECT:
		if ((stat = IO_write_uint2(roundradius, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_REGULAR:
		if ((stat = IO_write_uint2(nsides, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_OVAL:
		if ((stat = IO_write_uint2(startangle, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(arcangle, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_POLYGON:
		if ((stat = IO_write_uint2(markerlsize, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(nmarkerpoints, stream)) != IO_NORMAL)
			return stat;
		for (i = 0 ; i < nmarkerpoints ; i++)
		{
			if ((stat = IO_write_int2(markerpoints[i].x, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_int2(markerpoints[i].y, stream)) != IO_NORMAL)
				return stat;
		}
	case F_LINE:
	case F_CURVE:
		if ((stat = IO_write_uint2(arrowsize, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(nrealpoints, stream)) != IO_NORMAL)
			return stat;
		// MW-2012-02-22: [[ NoScrollSave ]] Adjust the real points by the current group offset.
		for (i = 0 ; i < nrealpoints ; i++)
		{
			if ((stat = IO_write_int2(realpoints[i].x + MCgroupedobjectoffset . x, stream)) != IO_NORMAL)
				return stat;
			if ((stat = IO_write_int2(realpoints[i].y + MCgroupedobjectoffset . y, stream)) != IO_NORMAL)
				return stat;
		}
	}
	if (flags & F_DASHES)
	{
		if ((stat = IO_write_uint2(ndashes, stream)) != IO_NORMAL)
			return stat;
		for (i = 0 ; i < ndashes ; i++)
			if ((stat = IO_write_int1(dashes[i], stream)) != IO_NORMAL)
				return stat;
	}
	if (flags & F_G_LABEL)
		if ((stat = IO_write_string(label, labelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
	return savepropsets(stream);
}

IO_stat MCGraphic::load(IO_handle stream, const char *version)
{
	uint2 i;
	IO_stat stat;
	Boolean loaddashes = False;

	if ((stat = MCControl::load(stream, version)) != IO_NORMAL)
		return stat;

//---- 2.7+:
//  . F_G_ANTI_ALIASED now defined
	if (strncmp(version, "2.7", 3) < 0)
		flags &= ~F_G_ANTI_ALIASED;
//----

	// MW-2012-02-17: [[ IntrinsicUnicode ]] If the unicode tag is set, then we are unicode.
	if ((m_font_flags & FF_HAS_UNICODE_TAG) != 0)
		m_font_flags |= FF_HAS_UNICODE;

	if ((stat = IO_read_uint2(&angle, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint2(&linesize, stream)) != IO_NORMAL)
		return stat;
	switch (flags & F_STYLE)
	{
	case F_G_RECTANGLE:
		break;
	case F_ROUNDRECT:
		if ((stat = IO_read_uint2(&roundradius, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_REGULAR:
		if ((stat = IO_read_uint2(&nsides, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_OVAL:
		if ((stat = IO_read_uint2(&startangle, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&arcangle, stream)) != IO_NORMAL)
			return stat;
		break;
	case F_POLYGON:
		if ((stat = IO_read_uint2(&markerlsize, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&nmarkerpoints, stream)) != IO_NORMAL)
			return stat;
		if (nmarkerpoints != 0)
		{
			markerpoints = new MCPoint[nmarkerpoints];
			for (i = 0 ; i < nmarkerpoints ; i++)
			{
				if ((stat = IO_read_int2(&markerpoints[i].x, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_read_int2(&markerpoints[i].y, stream)) != IO_NORMAL)
					return stat;
			}
		}
	case F_LINE:
	case F_CURVE:
		if ((stat = IO_read_uint2(&arrowsize, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&nrealpoints, stream)) != IO_NORMAL)
			return stat;
		if (nrealpoints != 0)
		{
			realpoints = new MCPoint[nrealpoints];
			for (i = 0 ; i < nrealpoints ; i++)
			{
				if ((stat = IO_read_int2(&realpoints[i].x, stream)) != IO_NORMAL)
					return stat;
				if ((stat = IO_read_int2(&realpoints[i].y, stream)) != IO_NORMAL)
					return stat;
			}
		}
		if (strncmp(version, "1.4", 3) < 0)
			loaddashes = True;
		if (strncmp(version, "1.4", 3) <= 0)
			arrowsize = DEFAULT_ARROW_SIZE;
		break;
	}
	if (loaddashes || flags & F_DASHES)
	{
		if ((stat = IO_read_uint2(&ndashes, stream)) != IO_NORMAL)
			return stat;
		if (ndashes != 0)
		{
			flags |= F_DASHES;
			dashes = new uint1[ndashes];
			for (i = 0 ; i < ndashes ; i++)
				if ((stat = IO_read_uint1(&dashes[i], stream)) != IO_NORMAL)
					return stat;
			// sanity check: ensure dashes are not all 0
			bool t_allzero = true;
			for (i=0; i<ndashes && t_allzero; i++)
				if (dashes[i] > 0)
					t_allzero = false;
			if (t_allzero)
			{
				delete [] dashes;
				dashes = NULL;
				ndashes = 0;
			}
		}
	}
	if (flags & F_G_LABEL)
	{
		uint4 tlabelsize;
		if ((stat = IO_read_string(label, tlabelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
		labelsize = tlabelsize;
	}
	return loadpropsets(stream);
}