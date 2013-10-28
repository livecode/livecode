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

#ifndef	STACK_H
#define	STACK_H

#ifndef __MC_OBJECT__
#include "object.h"
#endif

#ifndef __MC_UIDC__
#include "uidc.h"
#endif

#include "tilecache.h"

#define EXTERNAL_WAIT 10.0
#define MENU_SPACE 8
#define MENU_ARROW_SIZE 16

typedef struct
{
	MCFontStruct *font;
	char *name;
	uint2 size;
	uint2 style;
}
Fontcache;

typedef struct
{
	MCStringRef stackname;
	MCStringRef filename;
}
MCStackfile;

typedef struct _Mnemonic Mnemonic;

struct MCStackModeData;

class MCStackIdCache;

// MCStackSurface is an interim abstraction that should be rolled into the Window
// abstraction at some point - it represents a display rendering target.

enum MCStackSurfaceTargetType
{
	kMCStackSurfaceTargetNone,
	kMCStackSurfaceTargetWindowsDC,
	kMCStackSurfaceTargetQuickDraw,
	kMCStackSurfaceTargetCoreGraphics,
	kMCStackSurfaceTargetEAGLContext,
	kMCStackSurfaceTargetPixmap,
};

struct MCInterfaceDecoration;

////////////////////////////////////////////////////////////////////////////////

// IM-2013-09-23: [[ FullscreenMode ]] Available fullscreen modes
enum MCStackFullscreenMode
{
	kMCStackFullscreenModeNone,

	kMCStackFullscreenResize,	// ""			stack is resized to fill screen without scaling
	kMCStackFullscreenExactFit,	// "exact fit"	stack is stretched to fill screen
	kMCStackFullscreenShowAll,	// "show all"	whole stack is shown, scaled to take up as much screen space as possible. Both full width and height are visible
	kMCStackFullscreenNoBorder, // "no border"	scaled to cover whole screen, top+bottom or left+right of stack may be clipped
	kMCStackFullscreenNoScale,	// "no scale"	stack is centered on screen with no scaling
};

extern const char *MCStackFullscreenModeToString(MCStackFullscreenMode p_mode);
extern bool MCStackFullscreenModeFromString(const char *p_string, MCStackFullscreenMode &r_mode);

#define kMCStackFullscreenModeDefault (kMCStackFullscreenResize)

////////////////////////////////////////////////////////////////////////////////

class MCStackSurface
{
public:
	// Lock the surface for access with an MCGContextRef
	virtual bool LockGraphics(MCRegionRef area, MCGContextRef& r_context) = 0;
	// Unlock the surface.
	virtual void UnlockGraphics(void) = 0;
	
	// Lock the pixels within the given region. The bits are returned relative
	// to the top-left of the region.
	virtual bool LockPixels(MCRegionRef area, MCGRaster& r_raster) = 0;
	// Unlock the surface.
	virtual void UnlockPixels(void) = 0;
	
	// Lock the surface for direct access via the underlying system resource.
	virtual bool LockTarget(MCStackSurfaceTargetType type, void*& r_context) = 0;
	// Unlock the target.
	virtual void UnlockTarget(void) = 0;
	
	// Composite the source image onto the surface using the given blend mode & opacity
	virtual bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_source, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend) = 0;

	/* OVERHAUL - REVISIT: ideally, these methods should not be part of the public MCStackSurface interface */
	// Prepare surface for use - do not call from within drawing code
	virtual bool Lock(void) = 0;
	// Atomically update target surface with drawn image - do not call from within drawing code
	virtual void Unlock(void) = 0;
};

typedef bool (*MCStackUpdateCallback)(MCStackSurface *p_surface, MCRegionRef p_region, void *p_context);

class MCStack : public MCObject
{
	friend class MCHcstak;
	friend class MCHccard;

protected:
	Window window;
	MCCursorRef cursor;
	MCStack *substacks;
	MCCard *cards;
	MCCard *curcard;
	MCControl *controls;
	MCGroup *editing;
	MCCard *savecard;
	MCCard *savecards;
	MCControl *savecontrols;
	MCAudioClip *aclips;
	MCVideoClip *vclips;
	uint4 backgroundid;
	Window_mode mode;
	Window_position wposition;
	Object_pos walignment;
	Mnemonic *mnemonics;
	MCButton **needs;

	// These fields are now UTF8
	MCStringRef title;
	MCStringRef titlestring;
	
	uint4 iconid;
	uint4 windowshapeid;
	uint2 minwidth;
	uint2 minheight;
	uint2 maxwidth;
	uint2 maxheight;
	uint2 decorations;
	uint2 nfuncs;
	uint2 nmnemonics;
	int2 lasty;
	uint2 nneeds;
	uint2 nstackfiles;
	int2 menuy;
	uint2 menuheight;
	uint1 scrollmode;
	uint1 old_blendlevel;
	MCStackfile *stackfiles;
	Linkatts *linkatts;
	MCStringRef externalfiles;
	MCStringRef filename;
	MCNameRef _menubar;
	void (*idlefunc)();
	
	uint4 f_extended_state;

#if defined _WINDOWS_DESKTOP
	CDropTarget *droptarget;
#endif

	Window parentwindow;
	
	MCExternalHandlerList *m_externals;

	// MW-2011-09-12: [[ MacScroll ]] The current y-scroll setting of the stack.
	int32_t m_scroll;
	
	// MW-2011-09-13: [[ Effects ]] The temporary snapshot of a rect of the
	//  window.
	MCGImageRef m_snapshot;
	
	// MW-2011-09-13: [[ Masks ]] The window mask for the stack.
	MCWindowShape *m_window_shape;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
	
	// MW-2012-10-10: [[ IdCache ]]
	MCStackIdCache *m_id_cache;
	
	// MW-2011-11-24: [[ UpdateScreen ]] If true, then updates to this stack should only
	//   be flushed at the next updateScreen point.
	bool m_defer_updates : 1;

	MCRectangle old_rect ; 	// The rectangle of the stack before it was "fullscreened"
	
	static uint2 ibeam;

	//////////

	// Stack view properties
	bool m_view_fullscreen;
	MCStackFullscreenMode m_view_fullscreenmode;

	MCRectangle m_view_stack_rect;
	MCRectangle m_view_rect;

	// IM-2013-10-14: [[ FullscreenMode ]] Indicates whether the view needs to be redrawn
	bool m_view_need_redraw;
	
	MCGAffineTransform m_view_transform;

	// MW-2011-08-26: [[ TileCache ]] The stack's tilecache renderer - if any.
	MCTileCacheRef m_view_tilecache;

	// IM-2013-10-14: [[ FullscreenMode ]] The tilecache layer ID of the view background
	uint32_t m_view_bg_layer_id;
	
	// MW-2011-08-19: [[ Redraw ]] The region of the view that needs to be
	//   drawn to the screen on the next update.
	MCRegionRef m_view_update_region;
	
public:
	Boolean menuwindow;

	MCStack(void);
	MCStack(const MCStack &sref);
	// virtual functions from MCObject
	virtual ~MCStack();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	
	virtual bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor);
	
	virtual void open();
	virtual void close();
	virtual void kfocus();
	virtual Boolean kfocusnext(Boolean top);
	virtual Boolean kfocusprev(Boolean bottom);
	virtual void kunfocus();
	virtual Boolean kdown(const char *string, KeySym key);
	virtual Boolean kup(const char *string, KeySym key);
	virtual Boolean mfocus(int2 x, int2 y);
	virtual void mfocustake(MCControl *target);
	virtual void munfocus(void);
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void setrect(const MCRectangle &nrect);
	virtual Exec_stat getprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop_legacy(uint4 parid, Properties which, MCExecPoint &, Boolean effective);

	virtual Boolean del();
	virtual void paste(void);

	virtual MCStack *getstack();
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void recompute();
	
	// MW-2011-09-20: [[ Collision ]] Compute shape of stack.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);
	
	// MW-2011-08-17: [[ Redraw ]] Render the stack into the given context 'dirty' is the
	//   hull of the clipping region.
    virtual void render(MCContext *dc, const MCRectangle& dirty);
	void render(MCGContextRef p_context, const MCRectangle &p_dirty);

	// MW-2012-02-14: [[ FontRefs ]] Recompute the font inheritence hierarchy.
	virtual bool recomputefonts(MCFontRef parent_font);
	
	//////////
	// device interface

	MCRectangle device_getwindowrect() const;
	MCRectangle device_setgeom(const MCRectangle &p_rect);
	
	// IM-2013-08-29: [[ ResIndependence ]] add device-specific version of updatewindow.
	//   device_updatewindow takes a region in device coordinates.
	void device_updatewindow(MCRegionRef p_region);
	// MW-2011-09-13: [[ Redraw ]] Request an immediate update of the given region of the
	//   window using the presented pixmap. This is a platform-specific method - note that
	//   any window-mask is ignored with per-pixel alpha assumed to come from the the image.
	//   (although a window-mask needs to be present in the stack for it not to be ignored).
	// IM-2013-06-19: [[ RefactorGraphics ]] Replace pixmap update method with this
	//   version which uses a callback function to perform the actual drawing using a
	//    provided MCStackSurface instance. The MCStackSurface class is now responsible
	//    for handling any window mask present.
	// IM-2013-08-29: [[ ResIndependence ]] change updatewindowwithcallback to device-specific version.
	//   device_updatewindowwithcallback takes a region in device coordinates.
	void device_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context);
	
	// MW-2011-09-10: [[ Redraw ]] Perform a redraw of the window's content to the given
	//   surface.
	void device_redrawwindow(MCStackSurface *surface, MCRegionRef region);
	
	//////////
	// view interface

	void view_init(void);
	void view_copy(const MCStack &p_view);
	void view_destroy(void);

	// Get visible stack region
	MCRectangle view_getstackviewport(void);
	// Set the visible stack region. Returns modified rect if constrained by fullscreen mode
	MCRectangle view_setstackviewport(const MCRectangle &p_viewport);

	// Return the visible stack region constrained by the fullscreen settings
	MCRectangle view_constrainstackviewport(const MCRectangle &p_viewport);
	
	// Return the rect of the view in logical screen coords.
	MCRectangle view_getrect(void) const;

	// Set view to fullscreen 
	void view_setfullscreen(bool p_fullscreen);
	// Return whether this view is fullscreen or not
	bool view_getfullscreen(void) const;

	// Set scaling mode when fullscreen
	void view_setfullscreenmode(MCStackFullscreenMode p_mode);
	// Get the fullscreen scaling mode
	MCStackFullscreenMode view_getfullscreenmode(void) const;

	// Request update of the given stack region
	void view_updatestack(MCRegionRef p_region);
	// Redraw the given view rect
	void view_render(MCGContextRef p_target, MCRectangle p_rect);
	
	// Translate local stack coords to view space
	MCPoint view_viewtostackloc(const MCPoint &p_loc) const;
	// Translate view coords to local stack space
	MCPoint view_stacktoviewloc(const MCPoint &p_loc) const;

	// Return the stack -> view coordinates transform
	MCGAffineTransform view_getviewtransform(void) const;
	
	//////////
	
	// IM-2013-10-03: [[ FullscreenMode ]] Move implementation of tilecache operations into stackview
	
	// MW-2011-11-23: [[ AccelRender ]] Set / get the accelerated rendering derived property.
	bool view_getacceleratedrendering(void);
	void view_setacceleratedrendering(bool value);
	
	// IM-2013-01-03: [[ FullscreenMode ]] Set / get the compositor type
	MCTileCacheCompositorType view_getcompositortype(void);
	void view_setcompositortype(MCTileCacheCompositorType p_type);
	
	// IM-2013-01-03: [[ FullscreenMode ]] Set / get the compositor cache limit
	uint32_t view_getcompositorcachelimit(void);
	void view_setcompositorcachelimit(uint32_t p_limit);
	
	// IM-2013-01-03: [[ FullscreenMode ]] Set / get the compositor tile size
	uint32_t view_getcompositortilesize(void);
	void view_setcompositortilesize(uint32_t p_size);
	// IM-2013-01-03: [[ FullscreenMode ]] Check the validity of the tilesize (accepted tile sizes are powers of two between 16 & 256 pixels)
	bool view_isvalidcompositortilesize(uint32_t p_size);
	
	// MW-2011-08-26: [[ TileCache ]] Fetch the stack's tilecache.
	MCTileCacheRef view_gettilecache(void) { return m_view_tilecache; }
	
	void view_updatetilecache(void);
	void view_deactivatetilecache(void);
	bool view_snapshottilecache(const MCRectangle &p_stack_rect, MCGImageRef &r_image);
	
	void view_flushtilecache(void);
	void view_activatetilecache(void);
	void view_compacttilecache(void);
	
	// IM-2013-10-10: [[ FullscreenMode ]] Reconfigure view after window rect changes
	void view_configure(bool p_user);
	
	// IM-2013-10-10: [[ FullscreenMode ]] Update the on-screen bounds of the view
	void view_setrect(const MCRectangle &p_new_rect);
	// IM-2013-10-10: [[ FullscreenMode ]] Notify view of changes to its bounds
	void view_on_rect_changed(void);
	
	bool view_need_redraw(void) const { return m_view_need_redraw; }
	
	// IM-2013-10-14: [[ FullscreenMode ]] Clear any pending updates of the view
	void view_reset_updates(void);
	// IM-2013-10-14: [[ FullscreenMode ]] Mark the given view region as needing redrawn
	void view_dirty_rect(const MCRectangle &p_rect);
	// IM-2013-10-14: [[ FullscreenMode ]] Mark the entire view surface as needing redrawn
	void view_dirty_all(void);
	// IM-2013-10-14: [[ FullscreenMode ]] Request a redraw of the marked areas
	void view_updatewindow(void);
	// IM-2013-10-14: [[ FullscreenMode ]] Ensure the view content is up to date
	void view_apply_updates(void);
	
	//////////
	
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> stack viewport coordinate transform (Currently only applies the stack vertical scroll)
	MCGAffineTransform gettransform(void) const;
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> view coordinate transform
	MCGAffineTransform getviewtransform(void) const;
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> device coordinate transform
	MCGAffineTransform getdevicetransform(void) const;
	
	//////////
	
	// IM-2013-10-08: [[ FullscreenMode ]] Ensure rect of resizable stacks is within min/max width & height
	MCRectangle constrainstackrect(const MCRectangle &p_rect);
	// IM-2013-10-08: [[ FullscreenMode ]] Ensure rect of resizable stacks is within screen bounds
	MCRectangle constrainstackrecttoscreen(const MCRectangle &p_rect);
	
    // IM-2012-05-15: [[ Effective Rect ]] get the rect of the window (including decorations)
    MCRectangle getwindowrect() const;
	
	// IM-2013-10-08: [[ FullscreenMode ]] transform to / from stack & window coords, taking stack scroll into account
	MCPoint stacktowindowloc(const MCPoint &p_stackloc) const;
	MCPoint windowtostackloc(const MCPoint &p_windowloc) const;
	
	// IM-2013-10-09: [[ FullscreenMode ]] transform to / from stack & global coords, taking stack scroll into account
	MCPoint stacktogloballoc(const MCPoint &p_stackloc) const;
	MCPoint globaltostackloc(const MCPoint &p_globalloc) const;

	void setgeom();
	//////////
	
    virtual MCRectangle getrectangle(bool p_effective) const;
    
	void external_idle();
	void loadwindowshape();
	void setidlefunc(void (*newfunc)());
	Boolean setscript(MCStringRef newscript);
	void checkdestroy();
	IO_stat print(Print_mode mode, uint2 num, MCCard *card,
	              const MCRectangle *srect, const MCRectangle *drect);
	void resize(uint2 oldw, uint2 oldh);
	void configure(Boolean user);
	void iconify();
	void uniconify();
	Window_mode getmode();
	Window_mode getrealmode()
	{
		return mode;
	}
	uint2 userlevel();
	Boolean hcaddress();
	Boolean hcstack();
	
	virtual bool iskeyed() { return true; }
	virtual void securescript(MCObject *) { }
	virtual void unsecurescript(MCObject *) { }
	
	Boolean islocked();
	Boolean isiconic();
	Boolean isediting();
	Tool gettool(MCObject *optr) const;
	void hidecursor();
	void setcursor(MCCursorRef newcursor, Boolean force);
	MCCursorRef getcursor();
	void resetcursor(Boolean force);
	void clearcursor(void);
	void setibeam();
	void clearibeam();
	void extraopen(bool p_force);
	void extraclose(bool p_force);

	bool resolve_filename(MCStringRef filename, MCStringRef& r_resolved);

	void setopacity(uint1 p_value);
	
	void updatemodifiedmark(void);

    // MW-2008-10-28: [[ ParentScripts ]]
	// This method is used to resolve any
	// references in this stacks controls, or its substacks controls to
	// parent scripts. It is only invoked immediately after loading a stack.
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// This method returns false if there was not enough memory to complete the
	// resolution. In this case the state of the stack should be considered
	// 'undefined' and it should be deleted.
	bool resolveparentscripts(void);

	// MW-2011-08-09: [[ Groups ]] This method ensures that the 'shared' flag
	//   is set appropriately.
	void checksharedgroups(void);
	void checksharedgroups_slow(void);

	Window getwindow();
	Window getparentwindow();

	void redrawicon();

	uint2 getdecorations()
	{
		return decorations;
	}

	Boolean is_fullscreen (void)
	{
		return ( getextendedstate(ECS_FULLSCREEN) ) ;
	}

	Boolean takewindow(MCStack *sptr);
	Boolean setwindow(Window w);
	void setparentwindow(Window w);

	void kfocusset(MCControl *target);
	MCStack *clone();
	void compact();
	Boolean checkid(uint4 cardid, uint4 controlid);
	IO_stat saveas(const MCStringRef);
	MCStack *findname(Chunk_term type, const MCString &);
	MCStack *findid(Chunk_term type, uint4 inid, Boolean alt);
	void setmark();
	void clearmark();
	void setbackground(MCControl *bptr);
	void clearbackground();
	void ungroup(MCGroup *source);
	void startedit(MCGroup *group);
	void stopedit();
	void updatemenubar();
	
	// MW-2011-09-12: [[ MacScroll ]] The 'next' scroll setting (i.e. what it should be with
	//   current menubar and such).
	int32_t getnextscroll(void);
	// MW-2011-09-12: [[ MacScroll ]] Return the current scroll setting of the stack.
	int32_t getscroll(void) const;
	// MW-2011-09-12: [[ MacScroll ]] Apply any necessary scroll setting, based on current
	//   menubar and editmenu settings.
	void applyscroll(void);
	// MW-2011-09-12: [[ MacScroll ]] Revert any necessary scroll settings that have
	//   been applied.
	void clearscroll(void);
	// MW-2011-11-30: [[ Bug 9887 ]] Refactored scroll syncing code into separate function
	//   to handle player's on the stack also.
	void syncscroll(void);
	
	void scrollintoview();
	void scrollmenu(int2 offset, Boolean draw);
	void clipmenu(MCContext *context, MCRectangle &crect);
	Boolean count(Chunk_term otype, Chunk_term ptype, MCObject *, uint2 &num);
	void renumber(MCCard *card, uint4 newnumber);
	MCObject *getAV(Chunk_term etype, MCStringRef, Chunk_term otype);
	MCCard *getchild(Chunk_term etype, MCStringRef p_expression, Chunk_term otype);
#ifdef OLD_EXEC
	MCCard *getchild(Chunk_term etype, const MCString &, Chunk_term otype);
#endif  
    MCCard *getchildbyordinal(Chunk_term p_ordinal);
    MCCard *getchildbyid(uinteger_t p_id);
    MCCard *getchildbyname(MCNameRef p_name);
    
	/* LEGACY */ MCGroup *getbackground(Chunk_term etype, const MCString &, Chunk_term otype);
    
    MCGroup *getbackgroundbyordinal(Chunk_term otype);
    MCGroup *getbackgroundbyid(uinteger_t p_id);
    MCGroup *getbackgroundbyname(MCNameRef p_name);
	void addneed(MCButton *bptr);
	void removeneed(MCButton *bptr);
	void addmnemonic(MCButton *button, uint1 key);
	void deletemnemonic(MCButton *button);
	MCButton *findmnemonic(char which);
	void installaccels(MCStack *stack);
	void removeaccels(MCStack *stack);
	void setwindowname();
	void openwindow(Boolean override);
	void reopenwindow();
	Exec_stat openrect(const MCRectangle &rel, Window_mode wm, MCStack *parentwindow,
	                   Window_position wpos,  Object_pos walign);
	void getstackfiles(MCExecPoint &);
	void stringtostackfiles(MCStringRef d, MCStackfile **sf, uint2 &nf);
	void setstackfiles(MCStringRef);
	void getstackfile(MCStringRef p_name, MCStringRef &r_name);
	void setfilename(MCStringRef f);

	virtual IO_stat load(IO_handle stream, const char *version, uint1 type);
	IO_stat load_stack(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	virtual IO_stat load_substacks(IO_handle stream, const char *version);
	
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat save_stack(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	Exec_stat resubstack(MCStringRef p_data);
	MCCard *getcardid(uint4 inid);
	MCCard *findcardbyid(uint4 p_id);

	MCControl *getcontrolid(Chunk_term type, uint4 inid, bool p_recurse = false);
	MCControl *getcontrolname(Chunk_term type, MCStringRef);
	MCObject *getAVid(Chunk_term type, uint4 inid);
	/* LEGACY */ MCObject *getAVname(Chunk_term type, MCStringRef);
    bool getAVname(Chunk_term type, MCNameRef p_name, MCObject*& r_object);
	Exec_stat setcard(MCCard *card, Boolean recent, Boolean dynamic);
	//MCStack *findstackfile_oldstring(const MCString &s);
	MCStack *findstackname_oldstring(const MCString &);
	MCStack *findsubstackname_oldstring(const MCString &);
	MCStack *findstackfile(MCNameRef name);
	MCStack *findstackname(MCNameRef name);
	MCStack *findsubstackname(MCNameRef name);
	MCStack *findstackid(uint4 fid);
	MCStack *findsubstackid(uint4 fid);
	void translatecoords(MCStack *dest, int2 &x, int2 &y);
	uint4 newid();
	void appendaclip(MCAudioClip *aptr);
	void removeaclip(MCAudioClip *aptr);
	void appendvclip(MCVideoClip *vptr);
	void removevclip(MCVideoClip *vptr);
	void appendcontrol(MCControl *cptr);
	void removecontrol(MCControl *cptr);
	void appendcard(MCCard *cptr);
	void removecard(MCCard *cptr);
	MCObject *getsubstackobjid(Chunk_term type, uint4 inid);
	MCObject *getobjid(Chunk_term type, uint4 inid);
	MCObject *getsubstackobjname(Chunk_term type, MCStringRef);
	MCObject *getobjname(Chunk_term type, MCStringRef p_name);
	void createmenu(MCControl *nc, uint2 width, uint2 height);
	void menuset(uint2 button, uint2 defy);
	void menumup(uint2 which, MCStringRef &r_string, uint2 &selline);
	void menukdown(const char *string, KeySym key, MCStringRef &r_string, uint2 &selline);
	void findaccel(uint2 key, MCStringRef &r_pick, bool &r_disabled);
	void raise();
	void enter();
	void flip(uint2 count);
	bool sort(MCExecContext &ctxt, Sort_type dir, Sort_type form,
	               MCExpression *by, Boolean marked);
	void breakstring(MCStringRef, MCStringRef*& dest, uint2 &nstrings,
	                 Find_mode fmode);
	Boolean findone(MCExecContext &ctxt, Find_mode mode, MCStringRef *strings,
	                uint2 nstrings, MCChunk *field, Boolean firstcard);
	void find(MCExecPoint &ep, int p_mode, MCStringRef p_needle, MCChunk *p_target);
	void find(MCExecContext &ctxt, Find_mode mode, MCStringRef, MCChunk *field);
	void markfind(MCExecContext &ctxt, Find_mode mode, MCStringRef,
	              MCChunk *, Boolean mark);
	void mark(MCExecPoint &ep, MCExpression *where, Boolean mark);
	Linkatts *getlinkatts();
	Boolean cantabort()
	{
		return (flags & F_CANT_ABORT) != 0;
	}
	MCStringRef getfilename(void)
	{
		return filename;
	}
	MCStringRef gettitletext(void)
	{
		if (!MCStringIsEmpty(title))
			return title;
		return MCNameGetString(_name);
	}
	MCControl *getcontrols()
	{
		return controls;
	}
	MCCard *getcurcard()
	{
		return curcard;
	}
	MCCard *getcards()
	{
		return cards;
	}

	bool hasmenubar(void)
	{
		return !MCNameIsEmpty(_menubar);
	}
	MCNameRef getmenubar(void)
	{
		return _menubar;
	}

	MCGroup *getediting(void)
	{
		return editing;
	}

	MCStack *getsubstacks(void)
	{
		return substacks;
	}

	MCStackModeData *getmodedata(void)
	{
		return m_mode_data;
	}

	void effectrect(const MCRectangle &drect, Boolean &abort);
	
	MCStack *findstackd(Window w);
	MCStack *findchildstackd(Window w,uint2 &ccount, uint2 cindex);
	void realize();
	void sethints();
	// IM-2013-10-08: [[ FullscreenMode ]] Separate out window sizing hints
	void setsizehints();
	void destroywindowshape();

	// MW-2011-08-17: [[ Redraw ]] Mark the whole content area as needing redrawn.
	void dirtyall(void);
	// MW-2011-08-17: [[ Redraw ]] Mark a region of the content area as needing redrawn.
	void dirtyrect(const MCRectangle& rect);
	// MW-2011-08-17: [[ Redraw ]] Mark the title as needing reset
	void dirtywindowname(void);

	// MW-2011-08-17: [[ Redraw ]] Cause any updates to be applied to the stack.
	void applyupdates(void);
	
	// MW-2011-09-10: [[ Redraw ]] Request an immediate update of the given region of the
	//   window. This is a platform-specific method which causes 'redrawwindow' to be
	//   invoked.
	void updatewindow(MCRegionRef region);
	
	// MW-2012-08-06: [[ Fibers ]] Ensure the tilecache is updated to reflect the current
	//   frame.
	void updatetilecache(void);
	// MW-2013-01-08: Snapshot the contents of the tilecache (if any).
	bool snapshottilecache(MCRectangle area, MCGImageRef& r_image);
	// MW-2013-06-26: [[ Bug 10990 ]] Deactivate the tilecache.
    void deactivatetilecache(void);
    
	// MW-2011-09-08: [[ Redraw ]] Capture the contents of the given rect of the window
	//   into a temporary buffer. The buffer is ditched at the next update.
	void snapshotwindow(const MCRectangle& rect);
	// MW-2011-10-01: [[ Redraw ]] Take the given pixmap as the window's own snapshot. Used
	//   to do 'go stack' with effect on mobile.
	void takewindowsnapshot(MCStack *other);
	
	// MW-2012-12-09: [[ Bug 9901 ]] Preserve the contents of the android bitmap view before
	//   locking screen for a visual effect.
	void preservescreenforvisualeffect(const MCRectangle& p_rect);

	// MW-2012-10-10: [[ IdCache ]] Add and remove an object from the id cache.
	void cacheobjectbyid(MCObject *object);
	void uncacheobjectbyid(MCObject *object);
	MCObject *findobjectbyid(uint32_t object);
	void freeobjectidcache(void);

	inline bool getextendedstate(uint4 flag) const
	{
		return (f_extended_state & flag) != 0;
	}
	
	inline void setextendedstate(bool value, uint4 flag)
	{
		if (value)
			f_extended_state |= flag;
		else
			f_extended_state &= ~flag;
	}

	bool changeextendedstate(bool setting, uint32_t mask);

	MCExternalHandlerList *getexternalhandlers(void)
	{
		return m_externals;
	}

	void purgefonts();
	
	bool ismetal(void)
	{
		return getflag(F_DECORATIONS) && (getdecorations() & WD_METAL) != 0;
	}
	
	MCWindowShape *getwindowshape(void) { return m_window_shape; }

#if defined(_WINDOWS_DESKTOP)
	MCSysWindowHandle getrealwindow();
	MCSysWindowHandle getqtwindow(void);

	// MW-2011-09-14: [[ Redraw ]] The 'onpaint()' method is called when a WM_PAINT
	//   event is called.
	void onpaint(void);

	// MW-2011-09-14: [[ Redraw ]] This applies the updated window mask buffer to the
	//   window.
	void composite(void);
	
	void getstyle(uint32_t &wstyle, uint32_t &exstyle);
	void constrain(intptr_t lp);
#elif defined(_MAC_DESKTOP)
	MCSysWindowHandle getrealwindow()
	{
		return window->handle.window;
	}
	MCSysWindowHandle getqtwindow(void);
	void showmenubar();
	void getWinstyle(uint32_t &wstyle, uint32_t &wclass);

	void getminmax(MCMacSysRect *winrect);
#elif defined(_LINUX_DESKTOP)
	void setmodalhints(void);

	// MW-201-09-15: [[ Redraw ]] The 'onexpose()' method is called when a sequence
	//   of Expose events are recevied.
	void onexpose(MCRegionRef dirty);
#endif
	
	bool cursoroverride ;

	void start_externals();
	void stop_externals();

	MCStack *next()
	{
		return (MCStack *)MCDLlist::next();
	}
	MCStack *prev()
	{
		return (MCStack *)MCDLlist::prev();
	}
	void totop(MCStack *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCStack *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCStack *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCStack *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCStack *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCStack *remove(MCStack *&list)
	{
		return (MCStack *)MCDLlist::remove((MCDLlist *&)list);
	}
	
	MCRectangle recttoroot(const MCRectangle& orect);
	MCRectangle rectfromroot(const MCRectangle& rrect);
	
	void enablewindow(bool p_enable);
	bool mode_haswindow(void);

	void mode_openasmenu(MCStack *grab);
	void mode_closeasmenu(void);

	void mode_constrain(MCRectangle& rect);
	bool mode_needstoopen(void);

	////////// PROPERTY SUPPORT METHODS

	void SetDecoration(Properties which, bool setting);
	void GetGroupProps(MCExecContext& ctxt, Properties which, MCStringRef& r_props);
	void GetCardProps(MCExecContext& ctxt, Properties which, MCStringRef& r_props);
	void SetLinkAtt(MCExecContext& ctxt, Properties which, MCInterfaceNamedColor p_color);

	////////// PROPERTY ACCESSORS

	void GetFullscreen(MCExecContext& ctxt, bool &r_flag);
	void SetFullscreen(MCExecContext& ctxt, bool setting);
	virtual void SetName(MCExecContext& ctxt, MCStringRef p_name);
	virtual void SetId(MCExecContext& ctxt, uinteger_t p_new_id);
	virtual void SetVisible(MCExecContext& ctxt, uint32_t part, bool setting);
	void GetNumber(MCExecContext& ctxt, uinteger_t& r_number);
	void GetLayer(MCExecContext& ctxt, integer_t& r_layer);
	void GetFileName(MCExecContext& ctxt, MCStringRef& r_file_name);
	void SetFileName(MCExecContext& ctxt, MCStringRef p_file_name);
	void GetEffectiveFileName(MCExecContext& ctxt, MCStringRef& r_file_name);
	void GetSaveCompressed(MCExecContext& ctxt, bool& r_setting);
	void SetSaveCompressed(MCExecContext& ctxt, bool setting);
	void GetCantAbort(MCExecContext& ctxt, bool& r_setting);
	void SetCantAbort(MCExecContext& ctxt, bool setting);
	void GetCantDelete(MCExecContext& ctxt, bool& r_setting);
	void SetCantDelete(MCExecContext& ctxt, bool setting);
	void GetStyle(MCExecContext& ctxt, intenum_t& r_style);
	void SetStyle(MCExecContext& ctxt, intenum_t p_style);
	void GetCantModify(MCExecContext& ctxt, bool& r_setting);
	void SetCantModify(MCExecContext& ctxt, bool setting);
	void GetCantPeek(MCExecContext& ctxt, bool& r_setting);
	void SetCantPeek(MCExecContext& ctxt, bool setting);
	void GetDynamicPaths(MCExecContext& ctxt, bool& r_setting);
	void SetDynamicPaths(MCExecContext& ctxt, bool setting);
	void GetDestroyStack(MCExecContext& ctxt, bool& r_setting);
	void SetDestroyStack(MCExecContext& ctxt, bool setting);
	void GetDestroyWindow(MCExecContext& ctxt, bool& r_setting);
	void SetDestroyWindow(MCExecContext& ctxt, bool setting);
	void GetAlwaysBuffer(MCExecContext& ctxt, bool& r_setting);
	void SetAlwaysBuffer(MCExecContext& ctxt, bool setting);
	void GetLabel(MCExecContext& ctxt, MCStringRef& r_label);
	void SetLabel(MCExecContext& ctxt, MCStringRef p_label);
	void GetUnicodeLabel(MCExecContext& ctxt, MCDataRef& r_label);
	void SetUnicodeLabel(MCExecContext& ctxt, MCDataRef p_label);
	void GetCloseBox(MCExecContext& ctxt, bool& r_setting);
	void SetCloseBox(MCExecContext& ctxt, bool setting);
	void GetZoomBox(MCExecContext& ctxt, bool& r_setting);
	void SetZoomBox(MCExecContext& ctxt, bool setting);
	void GetDraggable(MCExecContext& ctxt, bool& r_setting);
	void SetDraggable(MCExecContext& ctxt, bool setting);
	void GetCollapseBox(MCExecContext& ctxt, bool& r_setting);
	void SetCollapseBox(MCExecContext& ctxt, bool setting);
	void GetLiveResizing(MCExecContext& ctxt, bool& r_setting);
	void SetLiveResizing(MCExecContext& ctxt, bool setting);
	void GetSystemWindow(MCExecContext& ctxt, bool& r_setting);
	void SetSystemWindow(MCExecContext& ctxt, bool setting);
	void GetMetal(MCExecContext& ctxt, bool& r_setting);
	void SetMetal(MCExecContext& ctxt, bool setting);
	void GetShadow(MCExecContext& ctxt, bool& r_setting);
	void SetShadow(MCExecContext& ctxt, bool setting);
	void GetResizable(MCExecContext& ctxt, bool& r_setting);
	void SetResizable(MCExecContext& ctxt, bool setting);
	void GetMinWidth(MCExecContext& ctxt, uinteger_t& r_width);
	void SetMinWidth(MCExecContext& ctxt, uinteger_t p_width);
	void GetMaxWidth(MCExecContext& ctxt, uinteger_t& r_width);
	void SetMaxWidth(MCExecContext& ctxt, uinteger_t p_width);
	void GetMinHeight(MCExecContext& ctxt, uinteger_t& r_height);
	void SetMinHeight(MCExecContext& ctxt, uinteger_t p_height);
	void GetMaxHeight(MCExecContext& ctxt, uinteger_t& r_height);
	void SetMaxHeight(MCExecContext& ctxt, uinteger_t p_height);
	void GetRecentNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetRecentCards(MCExecContext& ctxt, MCStringRef& r_cards);
	void GetIconic(MCExecContext& ctxt, bool& r_setting);
	void SetIconic(MCExecContext& ctxt, bool setting);
	void GetStartUpIconic(MCExecContext& ctxt, bool& r_setting);
	void SetStartUpIconic(MCExecContext& ctxt, bool setting);
	void GetIcon(MCExecContext& ctxt, uinteger_t& r_id);
	void SetIcon(MCExecContext& ctxt, uinteger_t p_id);
	void GetOwner(MCExecContext& ctxt, MCStringRef& r_owner);
	void GetMainStack(MCExecContext& ctxt, MCStringRef& r_main_stack);
	void SetMainStack(MCExecContext& ctxt, MCStringRef p_main_stack);
	void GetSubstacks(MCExecContext& ctxt, MCStringRef& r_substacks);
	void SetSubstacks(MCExecContext& ctxt, MCStringRef p_substacks);
	void GetBackgroundNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetBackgroundIds(MCExecContext& ctxt, MCStringRef& r_ids);
	void GetSharedGroupNames(MCExecContext& ctxt, MCStringRef& r_names);
	void GetSharedGroupIds(MCExecContext& ctxt, MCStringRef& r_ids);
    void GetCardIds(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_ids);
    void GetCardNames(MCExecContext& ctxt, uindex_t& r_count, MCStringRef*& r_names);
	void GetEditBackground(MCExecContext& ctxt, bool& r_value);
	void SetEditBackground(MCExecContext& ctxt, bool p_value);
	void GetExternals(MCExecContext& ctxt, MCStringRef& r_externals);
	void SetExternals(MCExecContext& ctxt, MCStringRef p_externals);
	void GetExternalCommands(MCExecContext& ctxt, MCStringRef& r_commands);
	void GetExternalFunctions(MCExecContext& ctxt, MCStringRef& r_functions);
	void GetExternalPackages(MCExecContext& ctxt, MCStringRef& r_externals);
	void GetMode(MCExecContext& ctxt, integer_t& r_mode);
	void GetWmPlace(MCExecContext& ctxt, bool& r_setting);
	void SetWmPlace(MCExecContext& ctxt, bool setting);
	void GetWindowId(MCExecContext& ctxt, uinteger_t& r_id);
	void GetPixmapId(MCExecContext& ctxt, uinteger_t& r_id);
	void GetHcAddressing(MCExecContext& ctxt, bool& r_setting);
	void SetHcAddressing(MCExecContext& ctxt, bool setting);
	void GetHcStack(MCExecContext& ctxt, bool& r_setting);
	void GetSize(MCExecContext& ctxt, uinteger_t& r_size);
	void GetFreeSize(MCExecContext& ctxt, uinteger_t& r_size);
	void GetLockScreen(MCExecContext& ctxt, bool& r_locked);
	void SetLockScreen(MCExecContext& ctxt, bool lock);
	void GetStackFiles(MCExecContext& ctxt, MCStringRef& r_files);
	void SetStackFiles(MCExecContext& ctxt, MCStringRef p_files);
	void GetMenuBar(MCExecContext& ctxt, MCStringRef& r_menubar);
	void SetMenuBar(MCExecContext& ctxt, MCStringRef p_menubar);
	void GetEditMenus(MCExecContext& ctxt, bool& r_setting);
	void SetEditMenus(MCExecContext& ctxt, bool setting);
	void GetVScroll(MCExecContext& ctxt, integer_t& r_scroll);
	void GetCharset(MCExecContext& ctxt, intenum_t& r_charset);
	void GetFormatForPrinting(MCExecContext& ctxt, bool& r_setting);
	void SetFormatForPrinting(MCExecContext& ctxt, bool setting);
	void GetLinkColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void SetLinkColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
	void GetEffectiveLinkColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetLinkHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void SetLinkHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
	void GetEffectiveLinkHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetLinkVisitedColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void SetLinkVisitedColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color);
	void GetEffectiveLinkVisitedColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color);
	void GetUnderlineLinks(MCExecContext& ctxt, bool& r_value);
	void SetUnderlineLinks(MCExecContext& ctxt, bool p_value);
	void GetEffectiveUnderlineLinks(MCExecContext& ctxt, bool& r_value);
	void GetWindowShape(MCExecContext& ctxt, uinteger_t& r_shape);
	void SetWindowShape(MCExecContext& ctxt, uinteger_t p_shape);
	virtual void SetBlendLevel(MCExecContext& ctxt, uinteger_t p_level);
	void GetScreen(MCExecContext& ctxt, integer_t& r_screen);
	void GetCurrentCard(MCExecContext& ctxt, MCStringRef& r_card);
	void SetCurrentCard(MCExecContext& ctxt, MCStringRef p_card);
	void GetModifiedMark(MCExecContext& ctxt, bool& r_setting);
	void SetModifiedMark(MCExecContext& ctxt, bool setting);
	void GetAcceleratedRendering(MCExecContext& ctxt, bool& r_value);
	void SetAcceleratedRendering(MCExecContext& ctxt, bool p_value);
	void GetCompositorType(MCExecContext& ctxt, intenum_t*& r_type);
	void SetCompositorType(MCExecContext& ctxt, intenum_t* p_type);
	void GetDeferScreenUpdates(MCExecContext& ctxt, bool& r_value);
	void SetDeferScreenUpdates(MCExecContext& ctxt, bool p_value);
	void GetEffectiveDeferScreenUpdates(MCExecContext& ctxt, bool& r_value);
    void SetDecorations(MCExecContext& ctxt, const MCInterfaceDecoration& p_value);
    void GetDecorations(MCExecContext& ctxt, MCInterfaceDecoration& r_value);
    
	void GetCompositorTileSize(MCExecContext& ctxt, uinteger_t*& p_size);
	void SetCompositorTileSize(MCExecContext& ctxt, uinteger_t* p_size);
	void GetCompositorCacheLimit(MCExecContext& ctxt, uinteger_t*& p_size);
	void SetCompositorCacheLimit(MCExecContext& ctxt, uinteger_t* p_size);

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
    
private:
	void loadexternals(void);
	void unloadexternals(void);

	// Mode-specific hooks, implemented in the various mode* files.
	MCStackModeData *m_mode_data;

	void mode_create(void);
	void mode_copy(const MCStack& other);
	void mode_destroy(void);

	void mode_load(void);

	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, MCStringRef carray, Boolean effective);
	Exec_stat mode_setprop(uint4 parid, Properties which, MCExecPoint &, MCStringRef cprop, MCStringRef carray, Boolean effective);

	void mode_getrealrect(MCRectangle& r_rect);
	void mode_takewindow(MCStack *other);
	void mode_takefocus(void);
	void mode_setgeom(void);
	void mode_setcursor(void);
	
	bool mode_openasdialog(void);
	void mode_closeasdialog(void);
};
#endif
