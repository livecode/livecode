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
// MCScrollbar class declarations
//
#ifndef	SCROLLBAR_H
#define	SCROLLBAR_H

#include "mccontrol.h"

#define MIN_THUMB_SIZE 8

//the height required for a WIN95 full Scale thumb
#define WIN_SCALE_THUMB_HEIGHT 21
#define WIN_SCALE_HEIGHT 26 //include the tics on the bottom of thumb
#define WIN_SCALE_THUMB_WIDTH 11 //full scale with
/*WIN95 Scale's thumb width is varies depend on the height of the rec.  If the button's
 rect height is 25 or larger, the thub width is 11.  the actual thumb width will be 
 computed in compute_thumb() */
#define MOTIF_SCALE_THUMB_SIZE 30 //fixed thumb width for Motif's Scale
#define MAC_SCALE_THUMB_SIZE 15 //fixed thumb width for Mac's Scale
#define FIXED_THUMB_SIZE 17

typedef MCObjectProxy<MCScrollbar>::Handle MCScrollbarHandle;

class MCScrollbar : public MCControl, public MCMixinObjectHandle<MCScrollbar>
{
public:
    
    enum { kObjectType = CT_SCROLLBAR };
    using MCMixinObjectHandle<MCScrollbar>::GetHandle;
    
private:
    
	real8 thumbpos;
	real8 thumbsize;
	real8 lineinc;
	real8 pageinc;
	uint2 barsize;
	uint2 nffw;
	uint2 nftrailing;
	uint2 nfforce;
	MCStringRef startstring;
	MCStringRef endstring;
	real8 startvalue;
	real8 endvalue;
	uint1 hover_part;

	// MW-2012-09-20: [[ Bug 10395 ]] If this flag is set then the scrollbar is
	//   embedded within another control and thus must be redrawn differently in
	//   compositing mode.
	bool m_embedded : 1;
	
	MCControl *linked_control;

	static real8 markpos;
	static uint2 mode;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the progress bar animate message is only posted from a single thread.
    bool m_animate_posted : 1;
	
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCScrollbar();
	MCScrollbar(const MCScrollbar &sref);

	// virtual functions from MCObject
	virtual ~MCScrollbar();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    
    virtual bool visit_self(MCObjectVisitor *p_visitor);
    
	virtual void open();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void applyrect(const MCRectangle &nrect);
	virtual void timer(MCNameRef mptr, MCParameter *params);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);
	
	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	// MCScrollbar functions
	void DrawMacProgressBar(MCDC *dc, MCRectangle &rect, int2 endpos);
	void DrawWinProgressBar(MCDC *dc, MCRectangle &rect, int2 endpos);
	void DrawMacScale(MCDC *dc, MCRectangle &trect, MCRectangle &thumb);
	void DrawWinScale(MCDC *dc, MCRectangle &trect, MCRectangle &thumb);
	void drawvalue(MCDC *dc, MCRectangle &thumb);
	void drawticks(MCDC *dc, MCRectangle &thumb);
	void compute_barsize();
	MCRectangle compute_bar();
	MCRectangle compute_thumb(real8 pos);
	void update(real8 newpos, MCNameRef mess);
	void getthumb(real8 &pos);
	void setthumb(real8 pos, real8 size, real8 linc, real8 ev);
	void movethumb(real8 pos);
	void setborderwidth(uint1 nw);
	void reset();
	void redrawarrow(uint2 oldmode);
	void drawmacthumb(MCDC *dc, MCRectangle &thumb);
	uint2 getwidgetthemetype();

	bool issbdisabled(void) const;

	void link(MCControl *p_control);

	void setembedded(void);
	void redrawall(void);

	////////// PROPERTY SUPPORT METHODS

	void Redraw(bool dirty = true);

	////////// PROPERTY ACCESSORS

	void GetStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetStyle(MCExecContext& ctxt, intenum_t p_style);
	void GetThumbSize(MCExecContext& ctxt, double& r_size);
	void SetThumbSize(MCExecContext& ctxt, double p_size);
	void GetThumbPos(MCExecContext& ctxt, double& r_pos);
	void SetThumbPos(MCExecContext& ctxt, double p_pos);
	void GetLineInc(MCExecContext& ctxt, double& r_inc);
	void SetLineInc(MCExecContext& ctxt, double p_inc);
	void GetPageInc(MCExecContext& ctxt, double& r_inc);
	void SetPageInc(MCExecContext& ctxt, double p_inc);
	void GetOrientation(MCExecContext& ctxt, intenum_t& r_style);
	void GetNumberFormat(MCExecContext& ctxt, MCStringRef& r_format);
	void SetNumberFormat(MCExecContext& ctxt, MCStringRef p_format);
	void GetStartValue(MCExecContext& ctxt, MCStringRef& r_value);
	void SetStartValue(MCExecContext& ctxt, MCStringRef p_value);
	void GetEndValue(MCExecContext& ctxt, MCStringRef& r_value);
	void SetEndValue(MCExecContext& ctxt, MCStringRef p_value);
	void GetShowValue(MCExecContext& ctxt, bool& r_setting);
	void SetShowValue(MCExecContext& ctxt, bool setting);
    
protected:
    
    virtual MCPlatformControlType getcontroltype();
};
#endif
