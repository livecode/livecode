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
	char *stackname;
	char *filename;
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
	char *title;
	char *titlestring;
	
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
	char *externalfiles;
	char *filename;
	MCNameRef _menubar;
	void (*idlefunc)();
	
	uint4 f_extended_state;

#if defined _WINDOWS_DESKTOP
	CDropTarget *droptarget;
#endif

	Window parentwindow;
	
	MCExternalHandlerList *m_externals;

	// MW-2011-08-19: [[ Redraw ]] The region of the stack that needs to be
	//   drawn to the screen on the next update.
	MCRegionRef m_update_region;

	// MW-2011-08-26: [[ TileCache ]] The stack's tilecache renderer - if any.
	MCTileCacheRef m_tilecache;

	// MW-2011-09-12: [[ MacScroll ]] The current y-scroll setting of the stack.
	int32_t m_scroll;
	
	// MW-2011-09-13: [[ Effects ]] The temporary snapshot of a rect of the
	//  window.
	MCGImageRef m_snapshot;
	
	// MW-2011-09-13: [[ Masks ]] The window mask for the stack.
	MCWindowShape *m_window_shape;
	
	// MW-2012-10-10: [[ IdCache ]]
	MCStackIdCache *m_id_cache;
	
	// MW-2011-11-24: [[ UpdateScreen ]] If true, then updates to this stack should only
	//   be flushed at the next updateScreen point.
	bool m_defer_updates : 1;
	
	MCRectangle old_rect ; 	// The rectangle of the stack before it was "fullscreened"
	
	static uint2 ibeam;

public:
	Boolean menuwindow;

	MCStack(void);
	MCStack(const MCStack &sref);
	// virtual functions from MCObject
	virtual ~MCStack();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();
	
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
	virtual Exec_stat getprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);
	virtual Exec_stat setprop(uint4 parid, Properties which, MCExecPoint &, Boolean effective);

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
	
    // IM-2012-05-15: [[ Effective Rect ]] get the rect of the window (including decorations)
    MCRectangle getwindowrect() const;
	MCRectangle device_getwindowrect() const;
	
	void setgeom();
	MCRectangle device_setgeom(const MCRectangle &p_rect);
	//////////
	
    virtual MCRectangle getrectangle(bool p_effective) const;
    
	void external_idle();
	void loadwindowshape();
	void setidlefunc(void (*newfunc)());
	Boolean setscript(char *newscript);
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

	char *resolve_filename(const char *filename);

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
	IO_stat saveas(const MCString &);
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
	MCObject *getAV(Chunk_term etype, const MCString &, Chunk_term otype);
	MCCard *getchild(Chunk_term etype, const MCString &, Chunk_term otype);
	MCGroup *getbackground(Chunk_term etype, const MCString &, Chunk_term otype);
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
	void stringtostackfiles(char *d, MCStackfile **sf, uint2 &nf);
	void setstackfiles(const MCString &);
	char *getstackfile(const MCString &);
	void setfilename(char *f);

	virtual IO_stat load(IO_handle stream, const char *version, uint1 type);
	IO_stat load_stack(IO_handle stream, const char *version);
	IO_stat extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length);
	virtual IO_stat load_substacks(IO_handle stream, const char *version);
	
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat save_stack(IO_handle stream, uint4 p_part, bool p_force_ext);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part);

	Exec_stat resubstack(char *data);
	MCCard *getcardid(uint4 inid);
	MCCard *findcardbyid(uint4 p_id);

	MCControl *getcontrolid(Chunk_term type, uint4 inid, bool p_recurse = false);
	MCControl *getcontrolname(Chunk_term type, const MCString &);
	MCObject *getAVid(Chunk_term type, uint4 inid);
	MCObject *getAVname(Chunk_term type, const MCString &);
	Exec_stat setcard(MCCard *card, Boolean recent, Boolean dynamic);
	MCStack *findstackfile(const MCString &s);
	MCStack *findstackname(const MCString &);
	MCStack *findsubstackname(const MCString &);
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
	MCObject *getsubstackobjname(Chunk_term type, const MCString &);
	MCObject *getobjname(Chunk_term type, const MCString &);
	void createmenu(MCControl *nc, uint2 width, uint2 height);
	void menuset(uint2 button, uint2 defy);
	void menumup(uint2 which, MCString &s, uint2 &selline);
	void menukdown(const char *string, KeySym key,
	               MCString &s, uint2 &selline);
	void findaccel(uint2 key, MCString &tpick, bool &r_disabled);
	void raise();
	void enter();
	void flip(uint2 count);
	Exec_stat sort(MCExecPoint &ep, Sort_type dir, Sort_type form,
	               MCExpression *by, Boolean marked);
	void breakstring(const MCString &, MCString **dest, uint2 &nstrings,
	                 Find_mode fmode);
	Boolean findone(MCExecPoint &ep, Find_mode mode, const MCString *strings,
	                uint2 nstrings, MCChunk *field, Boolean firstcard);
	void find(MCExecPoint &ep, Find_mode mode, const MCString &, MCChunk *field);
	void markfind(MCExecPoint &ep, Find_mode mode, const MCString &,
	              MCChunk *, Boolean mark);
	void mark(MCExecPoint &ep, MCExpression *where, Boolean mark);
	Linkatts *getlinkatts();
	Boolean cantabort()
	{
		return (flags & F_CANT_ABORT) != 0;
	}
	const char *getfilename()
	{
		return filename;
	}
	const char *gettitletext()
	{
		return title != NULL ? title : MCNameGetCString(_name);
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
	// IM-2013-08-29: [[ ResIndependence ]] add device-specific version of updatewindow.
	//   device_updatewindow takes a region in device coordinates.
	void updatewindow(MCRegionRef region);
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
	
	// MW-2012-08-06: [[ Fibers ]] Ensure the tilecache is updated to reflect the current
	//   frame.
	void updatetilecache(void);
	// MW-2013-01-08: Snapshot the contents of the tilecache (if any).
	bool snapshottilecache(MCRectangle area, MCGImageRef& r_image);
	// MW-2013-06-26: [[ Bug 10990 ]] Deactivate the tilecache.
    void deactivatetilecache(void);
    
	// MW-2011-09-10: [[ Redraw ]] Perform a redraw of the window's content to the given
	//   surface.
	void device_redrawwindow(MCStackSurface *surface, MCRegionRef region);
	
	// MW-2011-09-08: [[ Redraw ]] Capture the contents of the given rect of the window
	//   into a temporary buffer. The buffer is ditched at the next update.
	void snapshotwindow(const MCRectangle& rect);
	// MW-2011-10-01: [[ Redraw ]] Take the given pixmap as the window's own snapshot. Used
	//   to do 'go stack' with effect on mobile.
	void takewindowsnapshot(MCStack *other);
	
	// MW-2012-12-09: [[ Bug 9901 ]] Preserve the contents of the android bitmap view before
	//   locking screen for a visual effect.
	void preservescreenforvisualeffect(const MCRectangle& p_rect);

	// MW-2011-08-26: [[ TileCache ]] Fetch the stack's tilecache.
	MCTileCacheRef gettilecache(void) { return m_tilecache; }

	// MW-2011-11-23: [[ AccelRender ]] Set / get the accelerated rendering derived property.
	bool getacceleratedrendering(void);
	void setacceleratedrendering(bool value);
	
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
	
private:
	void loadexternals(void);
	void unloadexternals(void);

	// Mode-specific hooks, implemented in the various mode* files.
	MCStackModeData *m_mode_data;

	void mode_create(void);
	void mode_copy(const MCStack& other);
	void mode_destroy(void);

	void mode_load(void);

	Exec_stat mode_getprop(uint4 parid, Properties which, MCExecPoint &, const MCString &carray, Boolean effective);
	Exec_stat mode_setprop(uint4 parid, Properties which, MCExecPoint &, const MCString &cprop, const MCString &carray, Boolean effective);

	char *mode_resolve_filename(const char *filename);
	void mode_getrealrect(MCRectangle& r_rect);
	void mode_takewindow(MCStack *other);
	void mode_takefocus(void);
	void mode_setgeom(void);
	void mode_setcursor(void);
	
	bool mode_openasdialog(void);
	void mode_closeasdialog(void);
};
#endif
