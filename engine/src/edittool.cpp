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

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"

#include "globals.h"

#include "path.h"
#include "context.h"
#include "variable.h"
#include "meta.h"


#include "graphic.h"
#include "edittool.h"


MCGradientEditTool::MCGradientEditTool(MCGraphic *p_graphic, MCGradientFill *p_gradient, MCEditMode p_mode)
{
	graphic = p_graphic;
	mode = p_mode;
	gradient = p_gradient;
	m_gradient_edit_point = -1;
}

MCEditMode MCGradientEditTool::type()
{
	return mode;
}

uint4 MCGradientEditTool::handle_under_point(int2 x, int2 y)
{
	if (gradient == NULL)
		return -1;

	MCRectangle rects[3];
	gradient_rects(rects);

	for (int i=2; i>=0; i--)
		if (MCU_point_in_rect(rects[i], x, y))
			return i;

	return -1;
}


bool MCGradientEditTool::mdown(int2 x, int2 y, uint2 which)
{

	if (gradient == NULL)
	{
		return false;
	}
	
	// MM-2012-11-06: [[ Property Listener ]]
	graphic -> signallistenerswithmessage(kMCPropertyChangedMessageTypeGradientEditStarted);
	
	m_gradient_edit_point = handle_under_point(x, y);
	MCRectangle rects[3];
	gradient_rects(rects);

	switch(m_gradient_edit_point)
	{
	case 1:
		gradient->old_origin.x = MININT2;
		gradient->old_origin.y = MININT2;
		gradient->old_primary.x = gradient->primary.x;
		gradient->old_primary.y = gradient->primary.y;
		gradient->old_secondary.x = gradient->secondary.x;
		gradient->old_secondary.y = gradient->secondary.y;
	case 0:
	case 2:
		xoffset = x - (rects[m_gradient_edit_point].x + MCsizewidth / 2);
		yoffset = y - (rects[m_gradient_edit_point].y + MCsizewidth / 2);
		return true;
	default:
		return false;
	}

	return false;
}

bool MCGradientEditTool::mfocus(int2 x, int2 y)
{
	if (m_gradient_edit_point == -1)
	{
		if (gradient != NULL)
		{
			MCRectangle t_rects[3];
			gradient_rects(t_rects);
			for (int i=0; i<3; i++)
			{
				if (MCU_point_in_rect(t_rects[i], x, y))
					return true;
			}
		}
		return false;
	}

	MCRectangle t_old_effectiverect;
	t_old_effectiverect = graphic -> geteffectiverect();

	switch (m_gradient_edit_point)
	{
	case 0:
		{
			int4 dx = x - xoffset - gradient->origin.x;
			int4 dy = y - yoffset - gradient->origin.y;
			gradient->origin.x += dx;
			gradient->origin.y += dy;
			if ((MCmodifierstate & MS_SHIFT) == 0)
			{
				gradient->primary.x += dx;
				gradient->primary.y += dy;
				gradient->secondary.x += dx;
				gradient->secondary.y += dy;
			}
		}
	break;
	case 1:
		{
			int4 x0, y0, x1, y1;
			x0 = gradient->old_primary.x - gradient->origin.x;
			y0 = gradient->old_primary.y - gradient->origin.y;
			gradient->primary.x = x - xoffset;
			gradient->primary.y = y - yoffset;
			x1 = gradient->primary.x - gradient->origin.x;
			y1 = gradient->primary.y - gradient->origin.y;
			if ((MCmodifierstate & MS_SHIFT) == 0)
			{
				int4 x2, y2, d0sqr;
				int4 a, b;
				d0sqr = x0*x0 + y0*y0;
				if (d0sqr > 0)
				{
					x2 = gradient->old_secondary.x - gradient->origin.x;
					y2 = gradient->old_secondary.y - gradient->origin.y;
					a = x1 * x0 + y1 * y0;
					b = y1 * x0 - x1 * y0;

					gradient->secondary.x = gradient->origin.x + (x2 * a - y2 * b) / d0sqr;
					gradient->secondary.y = gradient->origin.y + (y2 * a + x2 * b) / d0sqr;
				}
			}
			else
			{
				gradient->old_primary.x = gradient->primary.x;
				gradient->old_primary.y = gradient->primary.y;
				gradient->old_secondary.x = gradient->secondary.x;
				gradient->old_secondary.y = gradient->secondary.y;
			}
		}
	break;
	case 2:
		gradient->secondary.x = x - xoffset;
		gradient->secondary.y = y - yoffset;
	break;
	}

	gradient->old_origin.x = MININT2;
	gradient->old_origin.y = MININT2;

	// MW-2011-08-18: [[ Layers ]] Notify the graphic its effective rect has changed and invalidate all.
	graphic -> layer_effectiverectchangedandredrawall(t_old_effectiverect);

	graphic->message_with_args(MCM_mouse_move, x, y);

	return True;
}

bool MCGradientEditTool::mup(int2 x, int2 y, uint2 which)
{
	// MM-2012-11-06: [[ Property Listener ]]
	graphic -> signallistenerswithmessage(kMCPropertyChangedMessageTypeGradientEditEnded);
	
	if (m_gradient_edit_point != -1)
	{
		m_gradient_edit_point = -1;
		return true;
	}
	else
		return false;
}

void MCGradientEditTool::drawhandles(MCDC *dc)
{
	if (MCdragging)
		return;

	if (gradient == NULL)
		return;

	dc -> setopacity(255);
	dc -> setfunction(GXcopy);

	MCRectangle rects[3];

	gradient_rects(rects);

	dc->setfillstyle(FillSolid, nil, 0, 0);
	dc->setlineatts(1, LineSolid, CapButt, JoinBevel);
	dc -> setquality(graphic->getflag(F_G_ANTI_ALIASED) ? QUALITY_SMOOTH : QUALITY_DEFAULT);
	dc->setforeground(dc->getblack());
	dc->drawline(rects[0].x + MCsizewidth /2, rects[0].y + MCsizewidth /2, rects[1].x + MCsizewidth /2, rects[1].y + MCsizewidth /2);
	dc->drawline(rects[0].x + MCsizewidth /2, rects[0].y + MCsizewidth /2, rects[2].x + MCsizewidth /2, rects[2].y + MCsizewidth /2);
	dc->setopacity(255);

	dc->setforeground(MCselectioncolor);
	for (int i=0; i<3; i++)
	{
		dc->fillarc(rects[i], 0, 360);
	}
	dc->setquality(QUALITY_DEFAULT);
}

void MCGradientEditTool::gradient_rects(MCRectangle *rects)
{
	rects[0].width = rects[0].height = MCsizewidth;
	rects[0].x = gradient->origin.x - MCsizewidth / 2;
	rects[0].y = gradient->origin.y - MCsizewidth / 2;
	rects[1].width = rects[1].height = MCsizewidth;
	rects[1].x = gradient->primary.x - MCsizewidth / 2;
	rects[1].y = gradient->primary.y - MCsizewidth / 2;
	rects[2].width = rects[2].height = MCsizewidth;
	rects[2].x = gradient->secondary.x - MCsizewidth / 2;
	rects[2].y = gradient->secondary.y - MCsizewidth / 2;
}

MCRectangle MCGradientEditTool::drawrect()
{
	MCRectangle drect; 
	if (gradient == NULL)
	{
		drect.width = 0;
		drect.height = 0;
		return drect;
	}

	MCRectangle rects[3];
	gradient_rects(rects);
	drect = MCU_union_rect(rects[0], rects[1]);
	return MCU_union_rect(drect, rects[2]);
}

MCRectangle MCGradientEditTool::minrect()
{
	int4 minx, miny, maxx, maxy;
	minx = MAXINT4;  miny = MAXINT4;
	maxx = MININT4;  maxy = MININT4;

	MCRectangle rect = {MININT2,MININT2,0,0};

	if (gradient != NULL)
	{
		minx = MCU_min(gradient->origin.x, gradient->primary.x);
		maxx = MCU_max(gradient->origin.x, gradient->primary.x);

		minx = MCU_min(minx, gradient->secondary.x);
		maxx = MCU_max(maxx, gradient->secondary.x);

		miny = MCU_min(gradient->origin.y, gradient->primary.y);
		maxy = MCU_max(gradient->origin.y, gradient->primary.y);

		miny = MCU_min(miny, gradient->secondary.y);
		maxy = MCU_max(maxy, gradient->secondary.y);

		if (minx <= maxx && miny <= maxy)
		{
			rect.x = minx;
			rect.y = miny;
			rect.width = maxx - minx ;
			rect.height = maxy - miny;
		}
	}
	return rect;
}

MCPolygonEditTool::MCPolygonEditTool(MCGraphic *p_graphic)
{
	graphic = p_graphic;
	m_polygon_edit_point = -1;
}

MCEditMode MCPolygonEditTool::type()
{
	return kMCEditModePolygon;
}

uint4 MCPolygonEditTool::handle_under_point(int2 x, int2 y)
{
	uint4 npts = graphic->getnumpoints();

	MCRectangle *rects = new MCRectangle[npts];
	point_rects(rects);

	for (uint4 i=0; i<npts; i++)
		if (MCU_point_in_rect(rects[i], x, y))
		{
			delete rects;
			return i;
		}

	delete rects;
	return -1;
}

bool MCPolygonEditTool::mdown(int2 x, int2 y, uint2 which)
{
	uint4 npts = graphic->getnumpoints();
	MCPoint *pts = graphic->getpoints();

	MCRectangle *rects = new MCRectangle[npts];

	point_rects(rects);

	m_path_start_point = -1;

	uint4 i = 0;

	// find point under x, y
	// if point is start of path, continue loop until end of path
	// check if last point in current path is same as first point

	for (; i < npts; i++)
	{
		if (MCU_point_in_rect(rects[i], x, y))
		{
			m_polygon_edit_point = i;
			xoffset = x - pts[i].x;
			yoffset = y - pts[i].y;
			if (i == 0 ||  rects[i - 1].x == MININT2)
			{
				for (; i < npts && pts[i].x != MININT2; i++);
				if (pts[i - 1].x == pts[m_polygon_edit_point].x && pts[i - 1].y == pts[m_polygon_edit_point].y)
				{
					m_path_start_point = m_polygon_edit_point;
					m_polygon_edit_point = i - 1;
				}
			}
			break;
		}
	}
	return (m_polygon_edit_point != -1);
}

bool MCPolygonEditTool::mfocus(int2 x, int2 y)
{
	if (m_polygon_edit_point == -1)
	{
		bool t_focus = false;
		int t_npts;
		t_npts = graphic->getnumpoints();

		if (t_npts > 0)
		{
			MCRectangle *t_rects;
			t_rects = new MCRectangle[t_npts];
			point_rects(t_rects);
			for (int i=0; i<t_npts; i++)
			{
				if (MCU_point_in_rect(t_rects[i], x, y))
				{
					t_focus = true;
					break;
				}
			}
			delete [] t_rects;
		}
		return t_focus;
	}

	bool closed = (m_path_start_point != -1);
	graphic->setpoint(m_polygon_edit_point, x - xoffset, y - yoffset, !closed);
	if (closed)
		graphic->setpoint(m_path_start_point, x - xoffset, y - yoffset, true);

	return true;
}

bool MCPolygonEditTool::mup(int2 x, int2 y, uint2 which)
{
	if (m_polygon_edit_point != -1)
	{
		m_polygon_edit_point = -1;
		return true;
	}
	else
		return false;
}

void MCPolygonEditTool::drawhandles(MCDC *dc)
{
	uint4 npts = graphic->getnumpoints();

	if (npts > 0)
	{
		dc -> setopacity(255);
		dc -> setfunction(GXcopy);

		MCRectangle *rects = new MCRectangle[npts];

		point_rects(rects);

		dc->setfillstyle(FillSolid, nil, 0, 0);
		dc->setlineatts(1, LineSolid, CapButt, JoinBevel);
		dc -> setquality(graphic->getflag(F_G_ANTI_ALIASED) ? QUALITY_SMOOTH : QUALITY_DEFAULT);
		
		dc->setforeground(MCselectioncolor);

		for (uint4 i=0; i<npts; i++)
		{
			if (rects[i].x != MININT2)
				dc->fillarc(rects[i], 0, 360);
		}
		dc->setquality(QUALITY_DEFAULT);
		delete rects;
	}
}

void MCPolygonEditTool::point_rects(MCRectangle *rects)
{
	uint4 npts = graphic->getnumpoints();
	MCPoint *pts = graphic->getpoints();

	for (uint4 i=0; i<npts; i++)
	{
		if (pts[i].x == MININT2)
		{
			rects[i].x = MININT2;
			rects[i].width = rects[i].height = 0;
		}
		else
		{
			rects[i].width = rects[i].height = MCsizewidth;
			rects[i].x = pts[i].x - MCsizewidth / 2;
			rects[i].y = pts[i].y - MCsizewidth / 2;
		}
	}
}

MCRectangle MCPolygonEditTool::drawrect()
{
	uint4 npts = graphic->getnumpoints();
	MCRectangle *rects = new MCRectangle[npts];
	point_rects(rects);

	MCRectangle drect = {0,0,0,0};

	for (uint4 i = 0; i < npts; i++)
	{
		drect = MCU_union_rect(drect, rects[i]);
	}
	return drect;
}

MCRectangle MCPolygonEditTool::minrect()
{
	MCRectangle rect = {MININT2,MININT2,0,0};
	return rect;
}