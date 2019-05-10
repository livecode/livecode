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

#ifndef	CARD_H
#define	CARD_H

#include "object.h"

typedef MCObjectProxy<MCCard>::Handle MCCardHandle;

class MCCard : public MCObject, public MCMixinObjectHandle<MCCard>
{
public:
    
    enum { kObjectType = CT_CARD };
    using MCMixinObjectHandle<MCCard>::GetHandle;
	
protected:
    
    friend class MCHccard;
	MCObjptr *objptrs;
	MCObjptr *kfocused;
	MCObjptr *oldkfocused;
	MCObjptr *mfocused;
	MCButton *defbutton;
	MCButton *odefbutton;
	Boolean mgrabbed;
	MCCdata *savedata;

	// MW-2011-08-26: [[ TileCache ]] The layer id of the background.
	uint32_t m_bg_layer_id;
	// MW-2011-09-23: [[ TileCache ]] The layer id of the foreground (selection rect).
	uint32_t m_fg_layer_id;
	
	// MM-2012-11-05: [[ Object selection started/ended message ]]
	bool m_selecting_objects : 1;

	static MCRectangle selrect;
	static int2 startx;
	static int2 starty;
	static MCObjptr *removedcontrol;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;

    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();

public:
	MCCard();
	MCCard(const MCCard &cref);
	// virtual functions from MCObject
	virtual ~MCCard();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual bool visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);
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
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus();
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);

	virtual Boolean del(bool p_check_flag);
	virtual void paste(void);

	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void recompute();
	
    virtual void toolchanged(Tool p_new_tool);
	
	virtual void OnAttach();
	virtual void OnDetach();
    
	virtual void OnViewTransformChanged();
	
	// MW-2011-09-20: [[ Collision ]] Compute shape of card.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);

	// MW-2012-02-14: [[ FontRefs ]] Recompute the font inheritence hierarchy.
	virtual bool recomputefonts(MCFontRef parent_font, bool force);

	// MW-2012-06-08: [[ Relayer ]] Move a control to before target.
	virtual void relayercontrol(MCControl *p_source, MCControl *p_target);
	virtual void relayercontrol_remove(MCControl *control);
	virtual void relayercontrol_insert(MCControl *control, MCControl *target);

	virtual void geometrychanged(const MCRectangle &p_rect);

    virtual void scheduledelete(bool p_is_child);
    
    virtual bool isdeletable(bool p_check_flag);
    
    void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated);

	MCObject *hittest(int32_t x, int32_t y);
	
	// MCCard functions
	IO_stat load(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	IO_stat saveobjects(IO_handle stream, uint4 p_part, uint32_t p_version);
	IO_stat loadobjects(IO_handle stream, uint32_t version);

	void kfocusset(MCControl *target);
	MCCard *clone(Boolean attach, Boolean controls, MCStack *p_parent = nullptr);
	void clonedata(MCCard *source);
	void replacedata(MCStack *source);
	Exec_stat relayer(MCControl *optr, uint2 newlayer);
	MCCard *findname(Chunk_term type, MCNameRef);
	MCCard *findid(Chunk_term type, uint4 inid, Boolean alt);
	Boolean countme(uint4 groupid, Boolean marked);
	Boolean count(Chunk_term otype, Chunk_term ptype, MCObject *stop,
	              uint2 &n, Boolean dohc);
	MCControl *getnumberedchild(integer_t p_number, Chunk_term p_obj_type, Chunk_term p_parent_type);
	MCControl *getchild(Chunk_term e, MCStringRef p_expression, Chunk_term o, Chunk_term p);
    
    MCControl *getchildbyordinal(Chunk_term p_ordinal, Chunk_term o, Chunk_term p);
    MCControl *getchildbyid(uinteger_t p_id, Chunk_term o, Chunk_term p);
    MCControl *getchildbyname(MCNameRef p_name, Chunk_term o, Chunk_term p);
    
	Boolean getchildid(uint4 inid);
	Exec_stat groupmessage(MCNameRef message, MCCard *other);
	void installaccels(MCStack *stack);
	void removeaccels(MCStack *stack);
	void resize(uint2 width, uint2 height);
	MCImage *createimage();
	Boolean removecontrol(MCControl *cptr, Boolean needredraw, Boolean cf);
	void clearfocus(MCObjptr *oldptr, MCObjptr *newptr);
	void erasefocus(MCObject *p_object);
	MCObjptr *newcontrol(MCControl *cptr, Boolean needredraw);
	void resetid(uint4 oldid, uint4 newid);
	Boolean checkid(uint4 controlid);
	Boolean find(MCExecContext &ctxt, Find_mode mode, MCStringRef,
	             Boolean firstcard, Boolean firstword);
	MCObjptr *getrefs();
	void clean();
	void clear();
	void setmark(Boolean newstate);
	Boolean getmark()
	{
		return (flags & F_MARKED) != 0;
	}
	MCControl *getkfocused();
	MCControl *getmfocused();

	MCControl *getmousecontrol(void);
	
	MCObjptr *getobjptrs(void) { return objptrs; }
	MCObjptr *getobjptrforcontrol(MCControl *control);

	bool selectedbutton(integer_t p_family, bool p_background, MCStringRef& r_string);
	void grab()
	{
		mgrabbed = True;
	}
	void ungrab()
	{
		mgrabbed = False;
	}
	Boolean getgrab()
	{
		return mgrabbed;
	}
	void setdefbutton(MCButton *btn)
	{
		defbutton = btn;
	}
	MCButton *getodefbutton()
	{
		return odefbutton;
	}
	MCButton *getdefbutton()
	{
		return defbutton == NULL ? odefbutton : defbutton;
	}
	void freedefbutton(MCButton *btn);
	MCRectangle computecrect();
	void updateselection(MCControl *cptr, const MCRectangle &oldrect, const MCRectangle &selrect, MCRectangle &drect);

	int2 getborderwidth(void);
	void drawcardborder(MCDC *dc, const MCRectangle &dirty);
	
	// IM-2013-09-13: [[ RefactorGraphics ]] render the card background
	void drawbackground(MCContext *p_context, const MCRectangle &p_dirty);
    
    /* The drawselection method renders the 'selection layer' - i.e. all the
     * selection decorations for all controls on the card. */
    void drawselection(MCContext *p_context, const MCRectangle& p_dirty);
    
	// IM-2016-09-26: [[ Bug 17247 ]] request redraw of the area occupied by
	//      selection marquee + handles
	void dirtyselection(const MCRectangle &p_rect);
    
	Exec_stat openbackgrounds(bool p_is_preopen, MCCard *p_other);
	Exec_stat closebackgrounds(MCCard *p_other);
	
	Exec_stat opencontrols(bool p_is_preopen);
	Exec_stat closecontrols(void);

	// MW-2011-08-19: [[ Layers ]] Dirty the given rect of the viewport.
	void layer_dirtyrect(const MCRectangle& dirty_rect);
	// MW-2011-08-19: [[ Layers ]] A layer has been added to the card.
	void layer_added(MCControl *control, MCControl *previous, MCControl *next);
	// MW-2011-08-19: [[ Layers ]] A layer has been removed from the card.
	void layer_removed(MCControl *control, MCControl *previous, MCControl *next);
	// MW-2011-08-19: [[ Layers ]] The viewport displayed in the stack has changed.
	void layer_setviewport(int32_t x, int32_t y, int32_t width, int32_t height);

	// MW-2011-08-26: [[ TileCache ]] Render all layers into the stack's tilecache.
    void render(void);
    void render_control(MCTileCacheRef p_tiler, MCControl *p_control, const MCRectangle& p_visible_rect, const MCGAffineTransform& p_transform, bool p_parent_is_container);
    void render_control_reset_ids(MCControl *p_control);

	// IM-2013-09-13: [[ RefactorGraphics ]] add tilecache_ prefix to render methods to make their purpose clearer
	// MW-2011-09-23: [[ TileCache ]] Render the card's bg layer.
	static bool tilecache_render_background(void *context, MCContext *target, const MCRectangle& dirty);
	// MW-2011-09-23: [[ TileCache ]] Render the card's fg layer.
	static bool tilecache_render_foreground(void *context, MCContext *target, const MCRectangle& dirty);

	// MW-2012-06-08: [[ Relayer ]] This method returns the control on the card with
	//   the given layer. If nil is returned the control doesn't exist.
	MCObject *getobjbylayer(uint32_t layer);

    bool mfocus_control(int2 x, int2 y, bool p_check_selected);
    
	MCCard *next()
	{
		return (MCCard *)MCDLlist::next();
	}
	MCCard *prev()
	{
		return (MCCard *)MCDLlist::prev();
	}
	void totop(MCCard *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCCard *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCCard *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCCard *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCCard *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}
	MCCard *remove(MCCard *&list)
	{
		return (MCCard *)MCDLlist::remove((MCDLlist *&)list);
	}

	////////// PROPERTY SUPPORT METHODS

	void GetPropList(MCExecContext& ctxt, Properties which, uint32_t part_id, MCStringRef& r_props);

	////////// PROPERTY ACCESSORS

	void GetLayer(MCExecContext& ctxt, MCInterfaceLayer& r_layer);
	void SetLayer(MCExecContext& ctxt, const MCInterfaceLayer& p_layer);
	void GetCantDelete(MCExecContext& ctxt, bool& r_setting);
	void SetCantDelete(MCExecContext& ctxt, bool setting);
	void GetDontSearch(MCExecContext& ctxt, bool& r_setting);
	void SetDontSearch(MCExecContext& ctxt, bool setting);
	void GetMarked(MCExecContext& ctxt, bool& r_setting);
	void SetMarked(MCExecContext& ctxt, bool setting);
	void GetShowPict(MCExecContext& ctxt, bool& r_value);
	void SetShowPict(MCExecContext& ctxt, bool value);
	void GetBackgroundNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetBackgroundIds(MCExecContext& ctxt, MCStringRef& r_ids);
	void GetSharedGroupNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetSharedGroupIds(MCExecContext& ctxt, MCStringRef& r_ids);
	void GetGroupNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetGroupIds(MCExecContext& ctxt, MCStringRef& r_ids);
    void GetControlNames(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_names);
    void GetControlIds(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_ids);
    void GetChildControlNames(MCExecContext& ctxt, MCStringRef& r_names);
    void GetChildControlIds(MCExecContext& ctxt, MCStringRef& r_ids);
	void GetFormattedLeft(MCExecContext& ctxt, integer_t& r_value);
	void GetFormattedTop(MCExecContext& ctxt, integer_t& r_value);
	void GetFormattedHeight(MCExecContext& ctxt, integer_t& r_value);
	void GetFormattedWidth(MCExecContext& ctxt, integer_t& r_value);
	void GetFormattedRect(MCExecContext& ctxt, MCRectangle& r_rect);
	void GetDefaultButton(MCExecContext& ctxt, MCStringRef& r_button);
    
	virtual void SetForePixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetBackPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetHilitePixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetBorderPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetTopPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetBottomPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetShadowPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetFocusPixel(MCExecContext& ctxt, uinteger_t* pixel);
	virtual void SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetTopColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetBottomColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetShadowColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetFocusColor(MCExecContext& ctxt, const MCInterfaceNamedColor& color);
	virtual void SetForePattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetBackPattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetHilitePattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetBorderPattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetTopPattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetBottomPattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetShadowPattern(MCExecContext& ctxt, uinteger_t* pattern);
	virtual void SetFocusPattern(MCExecContext& ctxt, uinteger_t* pattern);
    virtual void SetTextHeight(MCExecContext& ctxt, uinteger_t* height);
    virtual void SetTextFont(MCExecContext& ctxt, MCStringRef font);
    virtual void SetTextSize(MCExecContext& ctxt, uinteger_t* size);
    virtual void SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style);
    virtual void SetTheme(MCExecContext& ctxt, intenum_t p_theme);
};

////////////////////////////////////////////////////////////////////////////////

// navigate controls by layer
MCControl *MCControlPreviousByLayer(MCControl *p_control);
MCControl *MCControlNextByLayer(MCControl *p_control);

////////////////////////////////////////////////////////////////////////////////

#endif
