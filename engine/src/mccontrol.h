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
// Base class for all control objects
//
#ifndef	CONTROL_H
#define	CONTROL_H
#define __CONTROL_H

#include "object.h"

// This enum describes the 'hint' that is applied to the object via
// the 'layerMode' property. The engine uses this to derive the actual
// type of layer that will be used.
enum MCLayerModeHint
{
	// The object's layer should be considered to be rarely changing
	// and composited with the background wherever possible.
	kMCLayerModeHintStatic,
	// The object's layer should be considered to be rapidly changing
	// and cached independently.
	kMCLayerModeHintDynamic,
	// The object's layer should be considered to be scrolling, its
	// content cached independently and clipped to the bounds.
	kMCLayerModeHintScrolling,
	// The object's layer should be considered a union of its
	// children's layers, rather than a layer in its own right.
	kMCLayerModeHintContainer
};

struct MCInterfaceMargins;
union MCBitmapEffect;

typedef MCObjectProxy<MCControl>::Handle MCControlHandle;

class MCControl : public MCObject, public MCMixinObjectHandle<MCControl>
{
public:
    
    using MCMixinObjectHandle<MCControl>::GetHandle;
    
protected:
	int2 mx;
	int2 my;
	int2 leftmargin;
	int2 rightmargin;
	int2 topmargin;
	int2 bottommargin;
	
	MCBitmapEffectsRef m_bitmap_effects;

	// MW-2011-08-24: [[ Layers ]] The layer id of the control.
	uint32_t m_layer_id;
    MCRectangle m_layer_clip_rect;
	
	// MW-2011-09-21: [[ Layers ]] Whether something about the control has
	//   changed requiring a recompute the layer attributes.
	bool m_layer_attr_changed : 1;
	// MW-2011-09-21: [[ Layers ]] The layerMode as specified by the user
	MCLayerModeHint m_layer_mode_hint : 3;
	// MW-2011-09-21: [[ Layers ]] The effective layerMode as used in the
	//   last frame.
	MCLayerModeHint m_layer_mode : 3;
	// MW-2011-09-21: [[ Layers ]] Whether the layer is top-level or not.
	//   A layer is considered top-level if it's parent is a group, or all
	//   it's ancestors (up to card) are of 'container' type.
	bool m_layer_is_toplevel : 1;
	// MW-2011-09-21: [[ Layers ]] Whether the layer should be considered
	//   completely opaque.
	bool m_layer_is_opaque : 1;
	// MW-2011-09-21: [[ Layers ]] Whether the layer's content is simple
	//   enough that it can be passed directly (images, buttons with icons).
	bool m_layer_is_direct : 1;
	// MW-2011-09-21: [[ Layers ]] Whether the layer's object is unadorned
	//   (i.e. has no scrollbars, borders, effects etc.).
	bool m_layer_is_unadorned : 1;
	// MW-2011-09-21: [[ Layers ]] Whether the layer is a sprite or scenery
	//   layer.
	bool m_layer_is_sprite : 1;
    
    bool m_layer_has_clip_rect : 1;

	static int2 defaultmargin;
	static int2 xoffset;
	static int2 yoffset;
	static MCControlHandle focused;
	static double aspect;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
public:
	MCControl();
	MCControl(const MCControl &cref);
	~MCControl();

	// virtual functions from MCObject
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual void kunfocus();
	virtual Boolean kdown(MCStringRef p_string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void munfocus();
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual uint2 gettransient() const;

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

	virtual void select();
	virtual void deselect();
	virtual Boolean del(bool p_check_flag);
	virtual void paste(void);

	virtual void undo(Ustruct *us);

	// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
	virtual void draw(MCDC *dc, const MCRectangle &dirty, bool p_isolated, bool p_sprite) = 0;

	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	virtual Boolean kfocusset(MCControl *target);
	virtual MCControl *clone(Boolean attach, Object_pos p, bool invisible);
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
	virtual void fliph();
	virtual void flipv();
	virtual void getwidgetthemeinfo(MCWidgetInfo &widgetinfo);
	virtual void unlink(MCControl *p_control);
	
	virtual MCObject *hittest(int32_t x, int32_t y);

	IO_stat save_extended(IO_handle p_stream, const MCString& p_data, uint4 p_part);

	// MCControl functions
	void attach(Object_pos p, bool invisible);

	void redraw(MCDC *dc, const MCRectangle &dirty);

	// IM-2016-09-26: [[ Bug 17247 ]] Return rect of selection handles for the given object rect
	static void sizerects(const MCRectangle &p_object_rect, MCRectangle rects[8]);
	
    /* The drawselection method should render any selection / edit related
     * decorations the control currently has. */
	virtual void drawselection(MCDC *dc, const MCRectangle& p_dirty);
    
	void drawarrow(MCDC *dc, int2 x, int2 y, uint2 size,
	               Arrow_direction dir, Boolean border, Boolean hilite);
	void continuesize(int2 x, int2 y);
	uint2 sizehandles(int2 px, int2 py);
	void start(Boolean canclone);
	void end(bool p_send_mouse_up, bool p_release);
	void create(int2 x, int2 y);
	Boolean moveable();
	void newmessage();
	void enter();
	void leave();
	void hblt(MCRectangle &drect, int4 offset);
	void vblt(MCRectangle &drect, int4 offset);
	Boolean sbfocus(int2 x, int2 y, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdown(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdoubledown(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Boolean sbdoubleup(uint2 which, MCScrollbar *hsb, MCScrollbar *vsb);
	Exec_stat setsbprop(Properties which, bool p_enable, int4 tx, int4 ty,
	                    uint2 &sbw, MCScrollbar *&hsb, MCScrollbar *&vsb,
	                    Boolean &dirty);

	void drawfocus(MCDC* p_dc, const MCRectangle& p_dirty);

	void grab();
	void ungrab(uint2 which);

	// MW-2009-06-11: [[ Bitmap Effects ]] This call computes the pixel bounds of the
	//   control, rather than just its active bounds. (Some effects extend outside of
	//   a control's boundary).
	virtual MCRectangle geteffectiverect(void) const;

	// MW-2009-08-24: Accessor for bitmap effects to allow saving in MCObject.
	MCBitmapEffectsRef getbitmapeffects(void);
	void setbitmapeffects(MCBitmapEffectsRef bitmap_effects);

	// This method computes the area of effect of a stack for the current bitmap
	// effects applied to this control (and its parents) when using the given rect.
	MCRectangle computeeffectsrect(const MCRectangle& area) const;

	// MW-2011-08-18: [[ Layers ]] Mark the whole object's layer as needing redrawn.
	void layer_redrawall(void);
	// MW-2011-08-18: [[ Layers ]] Mark a portion of the object's layer as needing redrawn.
	void layer_redrawrect(const MCRectangle& rect);
	// MW-2011-08-18: [[ Layers ]] Take note of any changes to 'transient' and mark the whole object's layer as needing redrawn.
	void layer_transientchangedandredrawall(int32_t old_transient);
	// MW-2011-08-18: [[ Layers ]] Set the rect of the control, invalidating as necessary.
	void layer_setrect(const MCRectangle& new_rect, bool redrawall);
	// MW-2011-08-18: [[ Layers ]] Take note of the fact that the rect has changed and invalidate the layer.
	void layer_rectchanged(const MCRectangle& old_rect, bool redrawall);
	// MW-2011-08-18: [[ Layers [[ Take note of the fact that the effective rect has changed and invalidate the layer.
	void layer_effectiverectchangedandredrawall(const MCRectangle& old_effective_rect);
	// MW-2011-08-18: [[ Layers ]] Take note of any changes in effects, only invalidating as necessary.
	void layer_effectschanged(const MCRectangle& old_effective_rect);
	// MW-2011-09-30: [[ Layers ]] The content origin has changed by the given amount.
	void layer_contentoriginchanged(int32_t dx, int32_t dy);
	// MW-2011-08-18: [[ Layers ]] Take note of a change in visibility.
	void layer_visibilitychanged(const MCRectangle& old_effective_rect);
	// MW-2011-09-26: [[ Layers ]] Mark the layer as having scrolled.
	void layer_scrolled(void);
	
	// MW-2011-10-04: [[ Layers ]] Used internally to apply an update. If 'update_card' is
	//   true then the dirty rect of the stack will be updated too.
	void layer_dirtyeffectiverect(const MCRectangle& effective_rect, bool update_card);
	// MW-2011-08-24: [[ Layers ]] Used internally to apply a size change. If 'update_card' is
	//   true then the dirty rect of the stack will be updated too.
	void layer_changeeffectiverect(const MCRectangle& old_effective_rect, bool force_update, bool update_card);
	// MW-2011-09-07: [[ Layers ]] Used internally to apply an update to a scrolling layer. If
	//   'update_card' is true then the dirty rect of the stack will be updated too.
	void layer_dirtycontentrect(const MCRectangle& content_rect, bool update_card);

	// MW-2011-08-24: [[ TileCache ]] Returns the current layer id.
	uint32_t layer_getid(void) { return m_layer_id; }
	// MW-2011-08-24: [[ TileCache ]] Set thes layer id.
	void layer_setid(uint32_t p_id) { m_layer_id = p_id; }

	// MW-2011-09-22: [[ Layers ]] Returns the layer mode hint.
	MCLayerModeHint layer_getmodehint(void) { return m_layer_mode_hint; }
	// MW-2011-11-24: [[ LayerMode Save ]] Sets the layer mode hint (used by object unpickling).
	void layer_setmodehint(MCLayerModeHint p_mode) { m_layer_mode_hint = p_mode; m_layer_attr_changed = true; }
	// MW-2011-09-21: [[ Layers ]] Calculates the effective layer mode.
	MCLayerModeHint layer_geteffectivemode(void) { return layer_computeattrs(false); }
	// MW-2011-09-07: [[ Layers ]] Returns the content rect of the layer (if scrolling).
	MCRectangle layer_getcontentrect(void);

	// MW-2011-09-21: [[ Layers ]] Returns whether the layer is a sprite or not.
	bool layer_issprite(void) { return m_layer_is_sprite; }
	// MW-2011-09-21: [[ Layers ]] Returns whether the layer is scrolling or not.
    bool layer_isscrolling(void) { return m_layer_mode == kMCLayerModeHintScrolling; }
    // MW-2011-09-21: [[ Layers ]] Returns whether the layer is a container or not.
    bool layer_iscontainer(void) { return m_layer_mode == kMCLayerModeHintContainer; }
	// MW-2011-09-21: [[ Layers ]] Returns whether the layer is opaque or not.
	bool layer_isopaque(void) { return m_layer_is_opaque; }

    bool layer_has_clip_rect(void) { return m_layer_has_clip_rect; }
    
    // Note: The returned value only has meaning if layer_has_clip_rect() returns
    // true.
    MCRectangle layer_get_clip_rect(void) { return m_layer_clip_rect; }
    
	// MW-2011-09-21: [[ Layers ]] Make sure the layerMode attr's are accurate.
	MCLayerModeHint layer_computeattrs(bool commit);
	// MW-2011-09-21: [[ Layers ]] Reset the attributes to defaults.
	void layer_resetattrs(void);

	static MCControl *getfocused()
	{
        if (focused.IsValid())
        {
            return focused;
        }
        return nullptr;
	}

	uint32_t getstyle()
	{
		return getstyleint(flags);
	}

	int16_t getleftmargin() const
	{
		return leftmargin;
	}

	int16_t getrightmargin() const
	{
		return rightmargin;
	}

	MCControl *next()
	{
		return (MCControl *)MCDLlist::next();
	}

	MCControl *prev()
	{
		return (MCControl *)MCDLlist::prev();
	}

	void totop(MCControl *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}

	void insertto(MCControl *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}

	void appendto(MCControl *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}

	void append(MCControl *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}

	void splitat(MCControl *node)
	{
		MCDLlist::splitat((MCDLlist *)node);
	}

	MCControl *remove(MCControl *&list)
	{
		return (MCControl *)MCDLlist::remove((MCDLlist *&)list);
	}

	////////// PROPERTY SUPPORT METHODS

	void Redraw(void);
	void SetToolTip(MCExecContext& ctxt, MCStringRef p_tooltip, bool is_unicode);

	void DoSetHScroll(MCExecContext& ctxt, int4 tx, integer_t scroll);
	void DoSetVScroll(MCExecContext& ctxt, int4 ty, integer_t scroll);
	void DoSetHScrollbar(MCExecContext& ctxt, MCScrollbar*& hsb, uint2& sbw);
	void DoSetVScrollbar(MCExecContext& ctxt, MCScrollbar*& vsb, uint2& sbw);
	void DoSetScrollbarWidth(MCExecContext& ctxt, uint2& sbw, uinteger_t p_width);

    void EffectRedraw(MCRectangle p_old_rect);
    
	////////// PROPERTY ACCESSORS

	void GetLeftMargin(MCExecContext& ctxt, integer_t& r_margin);
	virtual void SetLeftMargin(MCExecContext& ctxt, integer_t p_margin);
	void GetRightMargin(MCExecContext& ctxt, integer_t& r_margin);
	virtual void SetRightMargin(MCExecContext& ctxt, integer_t p_margin);
	void GetTopMargin(MCExecContext& ctxt, integer_t& r_margin);
	virtual void SetTopMargin(MCExecContext& ctxt, integer_t p_margin);
	void GetBottomMargin(MCExecContext& ctxt, integer_t& r_margin);
	virtual void SetBottomMargin(MCExecContext& ctxt, integer_t p_margin);
	void GetToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip);
	void SetToolTip(MCExecContext& ctxt, MCStringRef p_tooltip);
	void GetUnicodeToolTip(MCExecContext& ctxt, MCDataRef& r_tooltip);
	void SetUnicodeToolTip(MCExecContext& ctxt, MCDataRef p_tooltip);
    void GetLayerClipRect(MCExecContext& ctxt, MCRectangle*& r_layer_clip_rect);
    void SetLayerClipRect(MCExecContext& ctxt, MCRectangle* p_layer_clip_rect);
	void GetLayerMode(MCExecContext& ctxt, intenum_t& r_mode);
    void SetLayerMode(MCExecContext& ctxt, intenum_t p_mode);
	void GetEffectiveLayerMode(MCExecContext& ctxt, intenum_t& r_mode);
    virtual void SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins);
    void GetMargins(MCExecContext& ctxt, MCInterfaceMargins& r_margins);
    
    virtual void SetInk(MCExecContext& ctxt, intenum_t ink);
    virtual void SetShowBorder(MCExecContext& ctxt, bool setting);
	virtual void SetShowFocusBorder(MCExecContext& ctxt, bool setting);
    virtual void SetOpaque(MCExecContext& ctxt, bool setting);
	virtual void SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow);

    void GetDropShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value);
    void SetDropShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value);
    void GetInnerShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value);
    void SetInnerShadowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value);
    void GetInnerGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value);
    void SetInnerGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value);
    void GetOuterGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value);
    void SetOuterGlowProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value);
    void GetColorOverlayProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue& r_value);
    void SetColorOverlayProperty(MCExecContext& ctxt, MCNameRef index, MCExecValue p_value);

};


// MCControl has lots of derived classes so this (fragile!) specialisation is
// needed to account for them. It depends on the correctness of the CT_x_CONTROL
// enum values (i.e everything within that range must derive from MCControl).
template <>
inline MCControl* MCObjectCast<MCControl>(MCObject* p_object)
{
    Chunk_term t_type = p_object->gettype();
    MCAssert(t_type >= CT_FIRST_CONTROL && t_type <= CT_LAST_CONTROL);
    return static_cast<MCControl*> (p_object);
}


#endif
