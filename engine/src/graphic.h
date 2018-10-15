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

//
// MCGraphic class declarations
//
#ifndef	GRAPHIC_H
#define	GRAPHIC_H
#define __GRAPHIC_H

#ifndef __CONTROL_H
#include "mccontrol.h"
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

typedef MCObjectProxy<MCGraphic>::Handle MCGraphicHandle;

class MCGraphic : public MCControl, public MCMixinObjectHandle<MCGraphic>
{
public:
    
    enum { kObjectType = CT_GRAPHIC };
    using MCMixinObjectHandle<MCGraphic>::GetHandle;
    
private:
    
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
	MCStringRef label;

	MCGradientFill *m_fill_gradient;
	MCGradientFill *m_stroke_gradient;
	MCEditTool *m_edit_tool;
	real4 m_stroke_miter_limit;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCGraphic();
	MCGraphic(const MCGraphic &sref);
	virtual ~MCGraphic();

	void initialise(void);
	void finalise(void);

	// virtual functions from MCObject
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual Boolean mfocus(int2 x, int2 y);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void applyrect(const MCRectangle &nrect);

	// MW-2011-11-23: [[ Array Chunk Props ]] Add 'effective' param to arrayprop access.
	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
    
    /* The drawselection method of the graphic renders any editMode decorations
     * which have been requested. */
    virtual void drawselection(MCDC *dc, const MCRectangle& dirty);

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
	void delpoints();
	bool closepolygon(MCPoint *&pts, uint2 &npts);
	MCStringRef getlabeltext();
	void drawlabel(MCDC *dc, int2 sx, int sy, uint2 twidth, const MCRectangle &srect, const MCStringRef& s, uint2 fstyle);

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
    
    ///////////////
    
	bool get_points_for_rect(MCPoint* &r_points, uindex_t &r_point_count);
	bool get_points_for_roundrect(MCPoint* &r_points, uindex_t &r_point_count);
	bool get_points_for_regular_polygon(MCPoint *&r_points, uindex_t &r_point_count);
	bool get_points_for_oval(MCPoint* &r_points, uindex_t &r_point_count);
	
	////////// PROPERTY SUPPORT METHODS

	void Redraw(MCRectangle drect);
	void Redraw(void);

	void DoGetLabel(MCExecContext& ctxt, bool to_unicode, bool effective, MCStringRef r_string);
	void DoSetLabel(MCExecContext& ctxt, bool to_unicode, MCStringRef p_label);
    
    void DoGetGradientFill(MCExecContext& ctxt, MCGradientFill*& p_fill, MCNameRef p_prop, MCExecValue& r_value);
    void DoSetGradientFill(MCExecContext& ctxt, MCGradientFill*& p_fill, Draw_index p_di, MCNameRef p_prop, MCExecValue p_value);
    
    void DoCopyPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points, uindex_t& r_count, MCPoint*& r_points);
    
    void SetPointsCommon(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points, bool p_is_relative);

	////////// PROPERTY ACCESSORS

	void GetAntiAliased(MCExecContext& ctxt, bool& r_setting);
	void SetAntiAliased(MCExecContext& ctxt, bool setting);
	void GetFillRule(MCExecContext& ctxt, intenum_t& r_rule);
	void SetFillRule(MCExecContext& ctxt, intenum_t rule);
	void GetEditMode(MCExecContext& ctxt, intenum_t& r_mode);
	void SetEditMode(MCExecContext& ctxt, intenum_t mode);
	void GetCapStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetCapStyle(MCExecContext& ctxt, intenum_t style);
	void GetJoinStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetJoinStyle(MCExecContext& ctxt, intenum_t style);
	void GetMiterLimit(MCExecContext& ctxt, double& r_limit);
	void SetMiterLimit(MCExecContext& ctxt, double limit);
	void GetLineSize(MCExecContext& ctxt, integer_t& r_size);
	void SetLineSize(MCExecContext& ctxt, integer_t size);
	void GetPolySides(MCExecContext& ctxt, integer_t& r_sides);
	void SetPolySides(MCExecContext& ctxt, integer_t p_sides);
	void GetAngle(MCExecContext& ctxt, integer_t& r_angle);
	void SetAngle(MCExecContext& ctxt, integer_t p_angle);
	void GetStartAngle(MCExecContext& ctxt, integer_t& r_angle);
	void SetStartAngle(MCExecContext& ctxt, integer_t p_angle);
	void GetArcAngle(MCExecContext& ctxt, integer_t& r_angle);
	void SetArcAngle(MCExecContext& ctxt, integer_t p_angle);
	void GetRoundRadius(MCExecContext& ctxt, integer_t& r_radius);
	void SetRoundRadius(MCExecContext& ctxt, integer_t radius);
	void GetArrowSize(MCExecContext& ctxt, integer_t& r_size);
	void SetArrowSize(MCExecContext& ctxt, integer_t size);
	void GetStartArrow(MCExecContext& ctxt, bool& r_setting);
	void SetStartArrow(MCExecContext& ctxt, bool setting);
	void GetEndArrow(MCExecContext& ctxt, bool& r_setting);
	void SetEndArrow(MCExecContext& ctxt, bool setting);
	void GetMarkerLineSize(MCExecContext& ctxt, integer_t& r_size);
	void SetMarkerLineSize(MCExecContext& ctxt, integer_t size);
	void GetMarkerDrawn(MCExecContext& ctxt, bool& r_setting);
	void SetMarkerDrawn(MCExecContext& ctxt, bool setting);
	void GetMarkerOpaque(MCExecContext& ctxt, bool& r_setting);
	void SetMarkerOpaque(MCExecContext& ctxt, bool setting);
	void GetRoundEnds(MCExecContext& ctxt, bool& r_setting);
	void SetRoundEnds(MCExecContext& ctxt, bool setting);
	void GetDontResize(MCExecContext& ctxt, bool& r_setting);
	void SetDontResize(MCExecContext& ctxt, bool setting);
	void GetStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetStyle(MCExecContext& ctxt, intenum_t p_style);
	void GetShowName(MCExecContext& ctxt, bool& r_setting);
	void SetShowName(MCExecContext& ctxt, bool setting);
	void GetLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void SetLabel(MCExecContext& ctxt, MCStringRef p_label);
	void GetEffectiveLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label);
	void GetEffectiveUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void GetFilled(MCExecContext& ctxt, bool& r_setting);
	void SetFilled(MCExecContext& ctxt, bool setting);
    
    void GetGradientFillProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value);
    void SetGradientFillProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue p_value);
    void GetGradientStrokeProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue& r_value);
    void SetGradientStrokeProperty(MCExecContext& ctxt, MCNameRef p_prop, MCExecValue p_value);
    
    void GetMarkerPoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points);
    void SetMarkerPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points);
    void GetDashes(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_points);
    void SetDashes(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_points);
    void GetPoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points);
    void SetPoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points);
    void GetRelativePoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points);
    void SetRelativePoints(MCExecContext& ctxt, uindex_t p_count, MCPoint* p_points);
    // SN-2014-06-24: [[ rect_points ]] allow effective [relative] points as read-only
    void GetEffectivePoints(MCExecContext& ctxt, uindex_t &p_count, MCPoint*& r_points);
    void GetEffectiveRelativePoints(MCExecContext& ctxt, uindex_t& r_count, MCPoint*& r_points);
    
    virtual void SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
    virtual void SetForePattern(MCExecContext& ctxt, uinteger_t* pattern);
    virtual void SetBackPattern(MCExecContext& ctxt, uinteger_t* pattern);
};
#endif
