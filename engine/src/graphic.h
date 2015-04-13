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

//
// MCGraphic class declarations
//
#ifndef	GRAPHIC_H
#define	GRAPHIC_H
#define __GRAPHIC_H

#ifndef __CONTROL_H
#include "control.h"
#endif

#include "gradient.h"
#include "edittool.h"

// We have:
//   rectangle - width, height
//   regular - nsides, angle, width, height
//   oval - startangle, arcangle, width, height
//   rounded rectangle - roundradius

enum
{
	kMCFillRuleNone,
	kMCFillRuleNonZero,
	kMCFillRuleEvenOdd
};

class MCGraphic : public MCControl
{
	uint2 linesize;
	uint2 angle;
	uint2 startangle;
	uint2 arcangle;
	uint2 roundradius;
	uint2 arrowsize;
	uint2 nsides;
	uint2 ndashes;
	uint2 npoints;
	uint2 nrealpoints;
	uint2 nmarkerpoints;
	uint2 markerlsize;
	uint1 *dashes;
	MCPoint *points;
	MCPoint *realpoints;
	MCPoint *markerpoints;
	MCPoint *oldpoints;
	MCRectangle oldrect;
	MCRectangle minrect;
	char *label;
	uint2 labelsize;

	MCGradientFill *m_fill_gradient;
	MCGradientFill *m_stroke_gradient;
	MCEditTool *m_edit_tool;
	real4 m_stroke_miter_limit;

public:
	MCGraphic();
	MCGraphic(const MCGraphic &sref);
	virtual ~MCGraphic();

	void initialise(void);
	void finalise(void);

	// virtual functions from MCObject
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual Boolean mfocus(int2 x, int2 y);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void setrect(const MCRectangle &nrect);

	// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective, bool recursive = false);
	virtual Exec_stat getarrayprop(uint4 parid, Properties which, MCExecPoint &, MCNameRef key, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setarrayprop(uint4 parid, Properties which, MCExecPoint&, MCNameRef key, Boolean effective);

	// virtual functions from MCControl
	IO_stat load(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);

	virtual Boolean maskrect(const MCRectangle &srect);
	virtual void fliph();
	virtual void flipv();
	// MCGraphic functions
	uint2 get_arrow_size();
	void draw_arrow(MCDC *dc, MCPoint &p1, MCPoint &p2);
	void draw_lines(MCDC *dc, MCPoint *pts, uint2 npts);
	void fill_polygons(MCDC *dc, MCPoint *pts, uint2 npts);
	void compute_extents(MCPoint *pts, uint2 npts, int2 &minx, int2 &miny,
	                     int2 &maxx, int2 &maxy);
	MCRectangle expand_minrect(const MCRectangle &trect);
	MCRectangle reduce_minrect(const MCRectangle &trect);
	void compute_minrect();
	virtual MCRectangle geteffectiverect(void) const;
	void delpoints();
	void closepolygon(MCPoint *&pts, uint2 &npts);
	void getlabeltext(MCString &s, bool& isunicode);
	void drawlabel(MCDC *dc, int2 sx, int sy, uint2 twidth, const MCRectangle &srect, const MCString &s, bool isunicode, uint2 fstyle);

	MCGradientFill *getgradient();
	MCPoint *getpoints();
	uint2 getnumpoints();
	void setpoint(uint4 i, int2 x, int2 y, bool redraw = true);
	MCRectangle getminrect();
	void setgradientrect(MCGradientFill *p_gradient, const MCRectangle &nrect);

	uint2 getjoinstyle();
	void setjoinstyle(uint2 p_style);
	uint2 getcapstyle();
	void setcapstyle(uint2 p_style);
	uint2 getfillrule();
	void setfillrule(uint2 p_rule);

	bool get_points_for_rect(MCPoint*& r_points, uint2& r_point_count);
	bool get_points_for_roundrect(MCPoint*& r_points, uint2& r_point_count);
	bool get_points_for_regular_polygon(MCPoint*& r_points, uint2& r_point_count);
	bool get_points_for_oval(MCPoint*& r_points, uint2& r_point_count);
};
#endif
