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
// MCGroup class declarations
//
#ifndef	GROUP_H
#define	GROUP_H

#include "mccontrol.h"

class MCScrollbar;

typedef MCObjectProxy<MCGroup>::Handle MCGroupHandle;

class MCGroup : public MCControl, public MCMixinObjectHandle<MCGroup>
{
public:
    
    enum { kObjectType = CT_GROUP };
    using MCMixinObjectHandle<MCGroup>::GetHandle;

private:
    
	friend class MCHcbkgd;
	friend class MCHcstak;
	MCControl *controls;
	MCControl *kfocused;
	MCControl *oldkfocused;
	MCControl *newkfocused;
	MCControlHandle mfocused;
	MCScrollbar *vscrollbar;
	MCScrollbar *hscrollbar;
	int4 scrollx;
	int4 scrolly;
	uint2 scrollbarwidth;
	MCStringRef label;
	MCRectangle minrect;
	uint2 number;
	Boolean mgrabbed;
	
	// MERG-2013-06-02: [[ GrpLckUpdates ]] True if updates to bounding rect and
	//   parents locked.
    bool m_updates_locked : 1;

    // MW-2014-06-20: [[ ClipsToRect ]] If true, group acts like lockLocation set, but can be resized.
    bool m_clips_to_rect : 1;
    
	static uint2 labeloffset;
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCGroup();
	MCGroup(const MCGroup &gref, bool p_copy_ids);
	// virtual functions from MCObject
	virtual ~MCGroup();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual bool visit_self(MCObjectVisitor *p_visitor);
	virtual bool visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean kup(MCStringRef p_string, KeySym key);
	virtual void mdrag(void);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual uint2 gettransient(void) const;
	virtual void applyrect(const MCRectangle &nrect);

    virtual void removereferences(void);
	virtual Boolean del(bool p_check_flag);
	virtual void recompute();

	// MW-2012-02-14: [[ Fonts ]] Recompute the font inheritence hierarchy.
	virtual bool recomputefonts(MCFontRef parent_font, bool force);

	// virtual functions from MCControl
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	virtual Boolean kfocusset(MCControl *target);
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite);

	virtual MCControl *findnum(Chunk_term type, uint2 &num);
	virtual MCControl *findname(Chunk_term type, MCNameRef);
	virtual MCControl *findid(Chunk_term type, uint4 inid, Boolean alt);
	virtual Boolean count(Chunk_term otype, MCObject *stop, uint2 &num);
	virtual Boolean maskrect(const MCRectangle &srect);
	virtual void installaccels(MCStack *stack);
	virtual void removeaccels(MCStack *stack);
	virtual MCCdata *getdata(uint4 cardid, Boolean clone);
	virtual void replacedata(MCCdata *&data, uint4 newid);
	virtual void compactdata();
	virtual void resetfontindex(MCStack *oldstack);
	virtual Exec_stat hscroll(int4 offset, Boolean doredraw);
	virtual Exec_stat vscroll(int4 offset, Boolean doredraw);
	virtual void readscrollbars();
	virtual void setsbrects();
	virtual void resetscrollbars(Boolean move);
	
	virtual MCObject *hittest(int32_t x, int32_t y);

	virtual void relayercontrol(MCControl *source, MCControl *target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	virtual void toolchanged(Tool p_new_tool);
	
	virtual void OnAttach();
	virtual void OnDetach();

	virtual void OnViewTransformChanged();
	
	virtual void geometrychanged(const MCRectangle &p_rect);

	virtual void viewportgeometrychanged(const MCRectangle &p_rect);

	virtual MCRectangle getviewportgeometry();
	
	bool getNativeContainerLayer(MCNativeLayer *&r_layer);
	
    virtual void scheduledelete(bool p_is_child);
    
    virtual bool isdeletable(bool p_check_flag);
    
    /* The drawselection method of the group recurses to draw all child control
     * selection decorations. */
    virtual void drawselection(MCDC *dc, const MCRectangle& p_dirty);
    
	MCControl *findchildwithid(Chunk_term type, uint4 p_id);

	// MCGroup functions
	MCControl *doclone(Boolean attach, Object_pos p, bool p_copy_ids, bool invisible);
	void drawthemegroup(MCDC *dc, const MCRectangle &dirty, Boolean drawframe);
	void drawbord(MCDC *dc, const MCRectangle &dirty);
	MCControl *getchild(Chunk_term etype, MCStringRef p_expression,Chunk_term otype, Chunk_term ptype);
    MCControl *getchildbyordinal(Chunk_term p_ordinal, Chunk_term o);
    MCControl *getchildbyid(uinteger_t p_id, Chunk_term o);
    MCControl *getchildbyname(MCNameRef p_name, Chunk_term o);
    
	void makegroup(MCControl *newcontrols, MCObject *newparent);
	MCControl *getcontrols();
	void setcontrols(MCControl *newcontrols);
	void appendcontrol(MCControl *cptr);
	void removecontrol(MCControl *cptr, Boolean cf);
	MCControl *getkfocused();
	MCControl *getmfocused();
	void clearfocus(MCControl *cptr);
	void radio(uint4 parid, MCControl *focused);
	MCButton *gethilitedbutton(uint4 parid);
	MCRectangle getgrect();
	void computecrect();
	bool computeminrect(Boolean scrolling);
	void boundcontrols();
	
	Exec_stat opencontrols(bool p_is_preopen);
	Exec_stat closecontrols(void);
	
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// This method returns false if there was not enough memory to complete the
	// resolution.
	bool resolveparentscript(void);

	// MW-2011-08-08: [[ Groups ]] Returns 'true' if the group is a background.
	bool isbackground(void) const { return getflag(F_GROUP_ONLY) == False; }
	// MW-2011-08-09: [[ Groups ]] Returns 'true' if the group is on/can be on multiple cards.
	bool isshared(void) const { return getflag(F_GROUP_SHARED); }

	// MW-2011-08-09: Ensure that all children of the group have non-zero id.
	void ensureids(void);
	
	// MW-2011-09-07: Return the group's minrect (contained control bounds).
	const MCRectangle& getminrect(void) { return minrect; }

	// MW-2012-03-01: [[ Bug 10045 ]] Clear the mfocus setting of the group without
	//   dispatching any messages.
	void clearmfocus(void);
	
	// MERG-2013-06-02: [[ GrpLckUpdates ]] Returns the update locking state of the
	//   group.
    bool islocked(void) { return m_updates_locked; }

    bool mfocus_control(int2 x, int2 y, bool p_check_selected);
    
	MCGroup *next()
	{
		return (MCGroup *)MCDLlist::next();
	}
	MCGroup *prev()
	{
		return (MCGroup *)MCDLlist::prev();
	}
	void totop(MCGroup *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCGroup *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCGroup *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCGroup *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCGroup *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCGroup *remove(MCGroup *&list)
	{
		return (MCGroup *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
	
	////////// PROPERTY SUPPORT METHODS

	void SetChildDisabled(MCExecContext& ctxt, uint32_t part, bool setting);
    void GetCardProps(MCExecContext& ctxt, Properties which, uindex_t& r_count, MCStringRef*& r_list);
    void GetPropList(MCExecContext& ctxt, Properties which, uint32_t part_id, MCStringRef& r_props);
    
    void UpdateMargins(void);

	////////// PROPERTY ACCESSORS

	void GetCantDelete(MCExecContext& ctxt, bool& r_setting);
	void SetCantDelete(MCExecContext& ctxt, bool setting);
	void GetDontSearch(MCExecContext& ctxt, bool& r_setting);
	void SetDontSearch(MCExecContext& ctxt, bool setting);
	void GetShowPict(MCExecContext& ctxt, bool& r_setting);
	void SetShowPict(MCExecContext& ctxt, bool setting);
	void GetRadioBehavior(MCExecContext& ctxt, uint32_t part, bool& r_setting);
	void SetRadioBehavior(MCExecContext& ctxt, uint32_t part, bool setting);
	void GetTabGroupBehavior(MCExecContext& ctxt, bool& r_setting);
	void SetTabGroupBehavior(MCExecContext& ctxt, bool setting);
	void GetHilitedButton(MCExecContext& ctxt, uint32_t part, integer_t& r_button);
	void SetHilitedButton(MCExecContext& ctxt, uint32_t part, integer_t p_button);
	void GetHilitedButtonId(MCExecContext& ctxt, uint32_t part, integer_t& r_id);
	void SetHilitedButtonId(MCExecContext& ctxt, uint32_t part, integer_t p_id);
	void GetHilitedButtonName(MCExecContext& ctxt, uint32_t part, MCStringRef& r_name);
	void SetHilitedButtonName(MCExecContext& ctxt, uint32_t part, MCStringRef p_name);
	void GetShowName(MCExecContext& ctxt, bool& r_setting);
	void SetShowName(MCExecContext& ctxt, bool setting);
	void GetLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void SetLabel(MCExecContext& ctxt, MCStringRef p_label);
	void GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label);
	void GetHScroll(MCExecContext& ctxt, integer_t& r_scroll);
	void SetHScroll(MCExecContext& ctxt, integer_t p_scroll);
	void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
	void SetVScroll(MCExecContext& ctxt, integer_t p_scroll);
	void GetUnboundedHScroll(MCExecContext& ctxt, bool& r_setting);
	void SetUnboundedHScroll(MCExecContext& ctxt, bool setting);
	void GetUnboundedVScroll(MCExecContext& ctxt, bool& r_setting);
	void SetUnboundedVScroll(MCExecContext& ctxt, bool setting);
	void GetHScrollbar(MCExecContext& ctxt, bool& r_setting);
	void SetHScrollbar(MCExecContext& ctxt, bool setting);
	void GetVScrollbar(MCExecContext& ctxt, bool& r_setting);
	void SetVScrollbar(MCExecContext& ctxt, bool setting);
	void GetScrollbarWidth(MCExecContext& ctxt, integer_t& r_width);
	void SetScrollbarWidth(MCExecContext& ctxt, integer_t p_width);
	void GetFormattedLeft(MCExecContext& ctxt, integer_t& r_left);
	void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height);
	void GetFormattedTop(MCExecContext& ctxt, integer_t& r_top);
	void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width);
	void GetFormattedRect(MCExecContext& ctxt, MCRectangle& r_rect);
	void GetBackgroundBehavior(MCExecContext& ctxt, bool& r_setting);
	void SetBackgroundBehavior(MCExecContext& ctxt, bool setting);
	void GetSharedBehavior(MCExecContext& ctxt, bool& r_setting);
	void SetSharedBehavior(MCExecContext& ctxt, bool setting);
	void GetBoundingRect(MCExecContext& ctxt, MCRectangle*& r_rect);
	void SetBoundingRect(MCExecContext& ctxt, MCRectangle* p_rect);
	void GetBackSize(MCExecContext& ctxt, MCPoint& r_size);
	void SetBackSize(MCExecContext& ctxt, MCPoint p_size);
	void GetSelectGroupedControls(MCExecContext& ctxt, bool& r_setting);
	void SetSelectGroupedControls(MCExecContext& ctxt, bool setting);
    void GetCardNames(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_list);
    void GetCardIds(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_list);
    void GetControlNames(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_names);
    void GetControlIds(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_ids);
    void GetChildControlNames(MCExecContext& ctxt, MCStringRef& r_names);
    void GetChildControlIds(MCExecContext& ctxt, MCStringRef& r_ids);
    void GetLockUpdates(MCExecContext& ctxt, bool& r_locked);
    void SetLockUpdates(MCExecContext& ctxt, bool p_locked);
    void SetClipsToRect(MCExecContext& ctxt, bool p_clips_to_rect);
    void GetClipsToRect(MCExecContext& ctxt, bool &r_clips_to_rect);

	virtual void SetVisible(MCExecContext& ctxt, uinteger_t part, bool setting);
	virtual void SetOpaque(MCExecContext& ctxt, bool setting);
	
	virtual void SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting);
	virtual void SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting);
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
    virtual void SetTextHeight(MCExecContext& ctxt, uinteger_t* height);
    virtual void SetTextSize(MCExecContext& ctxt, uinteger_t* size);
    virtual void SetBorderWidth(MCExecContext& ctxt, uinteger_t width);
    
    virtual void SetLeftMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetRightMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetTopMargin(MCExecContext& ctxt, integer_t p_margin);
	virtual void SetBottomMargin(MCExecContext& ctxt, integer_t p_margin);
    virtual void SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins);
};
#endif
