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

#ifndef	STACK_H
#define	STACK_H

#ifdef _LINUX_DESKTOP
#include <gdk/gdk.h>
#endif

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
// IM-2013-12-16: [[ ShowAll ]] Add showAll mode to fullscreen modes
enum MCStackFullscreenMode
{
	kMCStackFullscreenModeNone,

	kMCStackFullscreenResize,		// ""			stack is resized to fill screen without scaling
	kMCStackFullscreenExactFit,		// "exactFit"	stack is stretched to fill screen
	kMCStackFullscreenLetterbox,	// "letterbox"	whole stack is shown, scaled to take up as much screen space as possible. Both full width and height are visible
	kMCStackFullscreenNoBorder,		// "noBorder"	scaled to cover whole screen, top+bottom or left+right of stack may be clipped
	kMCStackFullscreenNoScale,		// "noScale"	stack is centered on screen with no scaling
	kMCStackFullscreenShowAll,		// "showAll"	whole stack is shown, scaled to take up as much screen space as possible. Visible areas outside the stack rect will be drawn.
};

extern const char *MCStackFullscreenModeToString(MCStackFullscreenMode p_mode);
extern bool MCStackFullscreenModeFromString(const char *p_string, MCStackFullscreenMode &r_mode);

#define kMCStackFullscreenModeDefault (kMCStackFullscreenResize)

////////////////////////////////////////////////////////////////////////////////

enum MCStackObjectVisibility
{
	kMCStackObjectVisibilityDefault,
	
	kMCStackObjectVisibilityShow,
	kMCStackObjectVisibilityHide,
};

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated the API so you can now lock multiple areas of the surface.
//  The context and raster for the locked area must now be stored locally rather than directly in the surface.
class MCStackSurface
{
public:
	// Lock the surface for access with an MCGContextRef
	virtual bool LockGraphics(MCGIntegerRectangle area, MCGContextRef& r_context, MCGRaster &r_raster) = 0;
	// Unlock the surface.
	virtual void UnlockGraphics(MCGIntegerRectangle area, MCGContextRef context, MCGRaster &raster) = 0;
	
	// Lock the pixels within the given region. The bits are returned relative
	// to the top-left of the region.
	// IM-2014-08-26: [[ Bug 13261 ]] Return the actual locked area covered by the pixels
	virtual bool LockPixels(MCGIntegerRectangle area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area) = 0;
	// Unlock the surface.
	virtual void UnlockPixels(MCGIntegerRectangle area, MCGRaster& raster) = 0;
	
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

typedef bool (*MCStackForEachCallback)(MCStack *p_stack, void *p_context);

enum MCStackAttachmentEvent
{
    kMCStackAttachmentEventDeleting,
    kMCStackAttachmentEventRealizing,
    kMCStackAttachmentEventUnrealizing,
    kMCStackAttachmentEventToolChanged,
};
typedef void (*MCStackAttachmentCallback)(void *context, MCStack *stack, MCStackAttachmentEvent event);
struct MCStackAttachment
{
    MCStackAttachment *next;
    void *context;
    MCStackAttachmentCallback callback;
};

typedef MCObjectProxy<MCStack>::Handle MCStackHandle;

class MCStack : public MCObject, public MCMixinObjectHandle<MCStack>
{
public:
    
    enum { kObjectType = CT_STACK };
    using MCMixinObjectHandle<MCStack>::GetHandle;

protected:
    
    friend class MCHcstak;
    friend class MCHccard;
    
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

	// IM-2014-07-23: [[ Bug 12930 ]] The stack whose window is parent to this stack
	MCStackHandle m_parent_stack;
	
	MCExternalHandlerList *m_externals;

	// MW-2011-09-12: [[ MacScroll ]] The current y-scroll setting of the stack.
	int32_t m_scroll;
	
	// MW-2011-09-13: [[ Effects ]] The temporary snapshot of a rect of the
	//  window.
	MCGImageRef m_snapshot;
	
	// MW-2011-09-13: [[ Masks ]] The window mask for the stack.
	MCWindowShape *m_window_shape;
	MCSysBitmapHandle m_window_buffer;

	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    
    static MCPropertyInfo kModeProperties[];
	static MCObjectPropertyTable kModePropertyTable;
	
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

	// IM-2014-01-16: [[ StackScale ]] Store the requested stack rect here rather than reset
	// to the old_rect held by the stack
	MCRectangle m_view_requested_stack_rect;
	MCRectangle m_view_adjusted_stack_rect;
	MCRectangle m_view_rect;
	// IM-2013-12-20: [[ ShowAll ]] The visible area of the stack
	MCRectangle m_view_stack_visible_rect;

	// IM-2013-10-14: [[ FullscreenMode ]] Indicates whether the view needs to be redrawn
	bool m_view_need_redraw;
	
	// IM-2014-09-23: [[ Bug 13349 ]] Indicates whether the view window needs to be resized
	bool m_view_need_resize;
	
	MCGAffineTransform m_view_transform;

	// MW-2011-08-26: [[ TileCache ]] The stack's tilecache renderer - if any.
	MCTileCacheRef m_view_tilecache;

	// IM-2013-10-14: [[ FullscreenMode ]] The tilecache layer ID of the view background
	uint32_t m_view_bg_layer_id;
	
	// MW-2011-08-19: [[ Redraw ]] The region of the view that needs to be
	//   drawn to the screen on the next update.
	MCRegionRef m_view_update_region;
	
	// IM-2014-01-07: [[ StackScale ]] the drawing scale of the stack
	// IM-2014-01-16: [[ StackScale ]] Move stack scale factor to view abstraction
	MCGFloat m_view_content_scale;

	// IM-2014-01-23: [[ HiDPI ]] The backing scale of the surface onto which this view is drawn
    
	MCGFloat m_view_backing_scale;
    
	// MW-2014-03-12: [[ Bug 11914 ]] If this is true then the stack is an engine menu.
	bool m_is_menu : 1;
    
    // MW-2014-09-30: [[ ScriptOnlyStack ]] If true, the stack is a script-only-stack.
    bool m_is_script_only : 1;
    
    // BWM-2017-08-16: [[ Bug 17810 ]] Line endings for imported script-only-stack.
    MCStringLineEndingStyle m_line_encoding_style : 3;
	
	bool m_is_ide_stack : 1;
	
	// IM-2014-05-27: [[ Bug 12321 ]] Indicate if we need to purge fonts when reopening the window
	bool m_purge_fonts;
    
    // MERG-2015-10-11: [[ DocumentFilename ]] The filename the stack represnts
    MCStringRef m_document_filename;
    
    virtual MCPlatformControlType getcontroltype();
    virtual MCPlatformControlPart getcontrolsubpart();

    MCStackAttachment *m_attachments;
	
	// IM-2016-02-26: [[ Bug 16244 ]] Determines whether or not to show hidden objects.
	MCStackObjectVisibility m_hidden_object_visibility;
    
public:
    
	Boolean menuwindow;

	MCStack(void);
	MCStack(const MCStack &sref);
	// virtual functions from MCObject
	virtual ~MCStack();
	virtual Chunk_term gettype() const;
	virtual const char *gettypestring();

	virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCObjectPropertyTable *getmodepropertytable(void) const { return &kModePropertyTable; }
	
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
	virtual void munfocus(void);
	virtual void mdrag(void);
	virtual Boolean mdown(uint2 which);
	virtual Boolean mup(uint2 which, bool p_release);
	virtual Boolean doubledown(uint2 which);
	virtual Boolean doubleup(uint2 which);
	virtual void timer(MCNameRef mptr, MCParameter *params);
	virtual void applyrect(const MCRectangle &nrect);

    virtual void removereferences(void);
    Boolean dodel(void);
	virtual Boolean del(bool p_check_flag);
    virtual bool isdeletable(bool p_check_flag);
    
	virtual void paste(void);

	virtual MCStackHandle getstack();
	virtual Exec_stat handle(Handler_type, MCNameRef, MCParameter *, MCObject *pass_from);
	virtual void recompute();
	
    virtual void toolchanged(Tool p_new_tool);
	
	virtual void OnViewTransformChanged();
	
	virtual void OnAttach();
	virtual void OnDetach();
    
	// MW-2011-09-20: [[ Collision ]] Compute shape of stack.
	virtual bool lockshape(MCObjectShape& r_shape);
	virtual void unlockshape(MCObjectShape& shape);
	
	// IM-2015-02-23: [[ WidgetPopup ]] Return true if the contents of this stack are competely opaque.
	// By default, stacks are opaque.
	virtual bool isopaque(void) { return true; }
	
	// MW-2011-08-17: [[ Redraw ]] Render the stack into the given context 'dirty' is the
	//   hull of the clipping region.
    virtual void render(MCContext *dc, const MCRectangle& dirty);
	void render(MCGContextRef p_context, const MCRectangle &p_dirty);

	// MW-2012-02-14: [[ FontRefs ]] Recompute the font inheritence hierarchy.
	virtual bool recomputefonts(MCFontRef parent_font, bool force);
	
	// IM-2016-02-26: [[ Bug 16244 ]] Return visibility of hidden objects - show, hide, or inherited from MCshowinvisibles.
	MCStackObjectVisibility gethiddenobjectvisibility();
	void sethiddenobjectvisibility(MCStackObjectVisibility p_visibility);

	// IM-2016-03-01: [[ Bug 16244 ]] Return true if invisible objects on this stack should be drawn.
	bool geteffectiveshowinvisibleobjects();
	
	//////////
	// view interface

	// IM-2014-01-24: [[ HiDPI ]] Convert device-space methods to logical-space platform-specific methods
	
	//////////
	// platform-specific view methods
	
	MCRectangle view_platform_getwindowrect() const;
	MCRectangle view_platform_setgeom(const MCRectangle &p_rect);

	// Request an immediate update of the given region of the window.
	// IM-2014-01-24: [[ HiDPI ]] The request region is specified in logical coordinates.
	void view_platform_updatewindow(MCRegionRef p_region);

	// IM-2014-01-24: [[ Redraw ]] Request an immediate update of the given region of the
	//   window using the given callback to perform the drawing.
	// IM-2014-01-24: [[ HiDPI ]] The request region is specified in logical coordinates.
	void view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context);
	
	// Some platforms require the entire window to be redrawn when resized.
	// This method indicates whether or not to mark the entire view as dirty
	//   when resized.
	bool view_platform_dirtyviewonresize() const;
	
	//////////
	
	// MW-2011-09-10: [[ Redraw ]] Perform a redraw of the window's content to the given surface.
	// IM-2014-01-24: [[ HiDPI ]] Update region is given in surface coordinates.
	void view_surface_redrawwindow(MCStackSurface *surface, MCGRegionRef region);
	
	//////////

	void view_init(void);
	void view_copy(const MCStack &p_view);
	void view_destroy(void);

	// Get visible stack region
	MCRectangle view_getstackviewport(void);
	// Set the visible stack region. Returns modified rect if constrained by fullscreen mode
	MCRectangle view_setstackviewport(const MCRectangle &p_viewport);

	// IM-2013-12-19: [[ ShowAll ]] Return the visible area of the view into which the stack will be rendered
	MCRectangle view_getstackvisiblerect(void);

	// Return the visible stack region constrained by the fullscreen settings
	// IM-2014-02-13: [[ StackScale ]] Update to work with MCGRectangles
	MCGRectangle view_constrainstackviewport(const MCGRectangle &p_viewport);
	
	// IM-2014-01-16: [[ StackScale ]] Ensure the view rect & transform are in sync with the configured view properties
	// (stack viewport, fullscreen mode, fullscreen, scale factor)
	void view_update_transform(bool p_ensure_onscreen = false);
	
	// IM-2014-01-16: [[ StackScale ]] Calculate the new view rect, transform, and adjusted stack rect for the given stack rect
	void view_calculate_viewports(const MCRectangle &p_stack_rect, MCRectangle &r_adjusted_stack_rect, MCRectangle &r_view_rect, MCGAffineTransform &r_transform);
	
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
	
	// IM-2013-10-17: [[ FullscreenMode ]] Return the view -> root window transform
	MCGAffineTransform view_getroottransform(void) const;
	
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
	
	// IM-2014-01-24: [[ HiDPI ]] Update the tilecache viewport to match the view rect at the current backing scale
	void view_updatetilecacheviewport(void);
	
	// IM-2013-10-10: [[ FullscreenMode ]] Reconfigure view after window rect changes
	void view_configure(bool p_user);
	void view_configure_with_rect(bool p_user, MCRectangle rect);
	
	// IM-2013-10-10: [[ FullscreenMode ]] Update the on-screen bounds of the view
	void view_setrect(const MCRectangle &p_new_rect);
	// IM-2013-10-10: [[ FullscreenMode ]] Notify view of changes to its bounds
	void view_on_rect_changed(void);
	
	bool view_need_redraw(void) const { return m_view_need_redraw; }
	
	// IM-2013-10-14: [[ FullscreenMode ]] Clear any pending updates of the view
	void view_reset_updates(void);
	// IM-2013-10-14: [[ FullscreenMode ]] Mark the given view region as needing redrawn
	void view_dirty_rect(const MCRectangle &p_rect);
	// IM-2013-10-14: [[ FullscreenMode ]] Request a redraw of the marked areas
	void view_updatewindow(void);
	// IM-2013-10-14: [[ FullscreenMode ]] Ensure the view content is up to date
	void view_apply_updates(void);
	
	// IM-2013-12-05: [[ PixelScale ]] Update view window geometry to scaled view rect
	void view_sync_window_geometry(void);
	
	// IM-2014-01-07: [[ StackScale ]] Get / Set the content scale of the stack
	// IM-2014-01-16: [[ StackScale ]] Move stack scale factor to view abstraction
	MCGFloat view_get_content_scale() const;
	void view_set_content_scale(MCGFloat p_scale);

	// IM_2014-01-24: [[ HiDPI ]] Return the view window rect in logical coords
	MCRectangle view_getwindowrect() const;

	// IM-2014-01-24: [[ HiDPI ]] Set the view window rect in logical coords
	MCRectangle view_setgeom(const MCRectangle &p_rect);
	
	// IM-2014-09-23: [[ Bug 13349 ]] Perform any deferred view window resizing needed
	void view_update_geometry(void);
	
	// IM-2014-01-24: [[ HiDPI ]] Return the scale factor from logical to pixel coords for the surface onto which the view is drawn
	MCGFloat view_getbackingscale(void) const;

	// IM-2014-01-24: [[ HiDPI ]] Called to update the view's backing scale to match the target surface before drawing
	void view_setbackingscale(MCGFloat p_scale);
	
	//////////
	
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> stack viewport coordinate transform (Currently only applies the stack vertical scroll)
	MCGAffineTransform gettransform(void) const;
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> view coordinate transform
	MCGAffineTransform getviewtransform(void) const;
	// IM-2013-10-14: [[ FullscreenMode ]] Return the stack -> device coordinate transform
	MCGAffineTransform getdevicetransform(void) const;
	// IM-2013-10-17: [[ FullscreenMode ]] Return the stack -> root window transform
	MCGAffineTransform getroottransform(void) const;
	
	// IM-2013-10-30: [[ FullscreenMode ]] Return the maximum horizontal / vertical scaling
	// factor of the stack -> device coordinate transform
	MCGFloat getdevicescale(void) const;
	
	// IM-2013-12-20: [[ ShowAll ]] Return the visible area of the stack contents
	MCRectangle getvisiblerect(void);
	
	//////////
	
	// IM-2013-10-08: [[ FullscreenMode ]] Ensure rect of resizable stacks is within min/max width & height
	MCRectangle constrainstackrect(const MCRectangle &p_rect);
	
    // IM-2012-05-15: [[ Effective Rect ]] get the rect of the window (including decorations)
    MCRectangle getwindowrect() const;
	
	// IM-2013-10-08: [[ FullscreenMode ]] transform to / from stack & window coords, taking stack scroll into account
	MCPoint stacktowindowloc(const MCPoint &p_stackloc) const;
	MCPoint windowtostackloc(const MCPoint &p_windowloc) const;
	
	// IM-2013-10-09: [[ FullscreenMode ]] transform to / from stack & global coords, taking stack scroll into account
	MCPoint stacktogloballoc(const MCPoint &p_stackloc) const;
	MCPoint globaltostackloc(const MCPoint &p_globalloc) const;

	//////////
	
	// IM-2014-01-07: [[ StackScale ]] Return the card rect after scale adjustment
	MCRectangle getcardrect() const;
	// IM-2014-01-07: [[ StackScale ]] Update the rect of the current card to fit the stack
	void updatecardsize();
    
	//////////
	
	void setgeom();
	
	//////////
	
    virtual MCRectangle getrectangle(bool p_effective) const;
    
	void external_idle();
	void loadwindowshape();
	void setidlefunc(void (*newfunc)());
    // MW-2014-10-24: [[ Bug 13796 ]] Separate script setting from commandline from other cases.
	Boolean setscript_from_commandline(MCStringRef newscript);
	void checkdestroy();
	IO_stat print(Print_mode mode, uint2 num, MCCard *card,
	              const MCRectangle *srect, const MCRectangle *drect);
	void resize(uint2 oldw, uint2 oldh);
	void configure(Boolean user);
	// CW-2015-09-28: [[ Bug 15873 ]] Separate the application of stack iconic properties from the (un)iconify actions.
	void seticonic(Boolean on);
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
	
	virtual bool haspassword() { return false; }
	virtual bool iskeyed() { return true; }
	virtual void securescript(MCObject *) { }
	virtual void unsecurescript(MCObject *) { }
    virtual void startparsingscript(MCObject*, MCDataRef&);
    virtual void stopparsingscript(MCObject*, MCDataRef);
	
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
	
	// IM-2013-10-30: [[ FullscreenMode ]] Resolve the given path relative to the location of the stack file
    // - Will return a path regardless of whether or not the file exists.
    bool resolve_relative_path(MCStringRef p_path, MCStringRef& r_resolved);
    
    // PM-2015-01-26: [[ Bug 14435 ]] Make possible to set the filename using a relative path to the default folder
    bool resolve_relative_path_to_default_folder(MCStringRef p_path, MCStringRef &r_resolved);

	void setopacity(uint1 p_value);
	
	void updatemodifiedmark(void);
    
    void updateignoremouseevents(void);

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
    Window getwindowalways() { return window; }
	
	// IM-2014-07-23: [[ Bug 12930 ]] Set the stack whose window is parent to this stack
	void setparentstack(MCStack *p_parent);
	MCStack *getparentstack(void);

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

	void kfocusset(MCControl *target);
	MCStack *clone();
	void compact();
	Boolean checkid(uint4 cardid, uint4 controlid);
	IO_stat saveas(const MCStringRef, uint32_t p_version = UINT32_MAX);
	MCStack *findname(Chunk_term type, MCNameRef);
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
	int32_t getnextscroll(bool p_ignore_opened);
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
    MCCard *getchildbyordinal(Chunk_term p_ordinal);
    MCCard *getchildbyid(uinteger_t p_id);
    MCCard *getchildbyname(MCNameRef p_name);
    
	/* LEGACY */ MCGroup *getbackground(Chunk_term etype, MCStringRef, Chunk_term otype);
    
    MCGroup *getbackgroundbyordinal(Chunk_term otype);
    MCGroup *getbackgroundbyid(uinteger_t p_id);
    MCGroup *getbackgroundbyname(MCNameRef p_name);
	void addneed(MCButton *bptr);
	void removeneed(MCButton *bptr);
	void addmnemonic(MCButton *button, KeySym p_key);
	void deletemnemonic(MCButton *button);
	MCButton *findmnemonic(KeySym p_key);
	void installaccels(MCStack *stack);
	void removeaccels(MCStack *stack);
	void setwindowname();
	
	void openwindow(Boolean override);
	// IM-2014-09-23: [[ Bug 13349 ]] Platform-specific openwindow method
	void platform_openwindow(Boolean override);
	
	void reopenwindow();
	Exec_stat openrect(const MCRectangle &rel, Window_mode wm, MCStack *parentwindow,
	                   Window_position wpos,  Object_pos walign);

	bool getstackfiles(MCStringRef& r_sf);
	bool stringtostackfiles(MCStringRef d, MCStackfile **sf, uint2 &nf);
	void setstackfiles(MCStringRef);
	void getstackfile(MCStringRef p_name, MCStringRef &r_name);
	void setfilename(MCStringRef f);

	virtual IO_stat load(IO_handle stream, uint32_t version); /* Don't use this */
	virtual IO_stat load(IO_handle stream, uint32_t version, uint1 type);
	IO_stat load_stack(IO_handle stream, uint32_t version);
	IO_stat extendedload(MCObjectInputStream& p_stream, uint32_t version, uint4 p_length);
	virtual IO_stat load_substacks(IO_handle stream, uint32_t version);
	
	virtual IO_stat save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat save_stack(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version);
	IO_stat extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version);

	Exec_stat resubstack(MCStringRef p_data);
	MCCard *getcardid(uint4 inid);
	MCCard *findcardbyid(uint4 p_id);

	MCControl *getcontrolid(Chunk_term type, uint4 inid, bool p_recurse = false);
	MCControl *getcontrolname(Chunk_term type, MCNameRef);
	MCObject *getAVid(Chunk_term type, uint4 inid);
    bool getAVname(Chunk_term type, MCNameRef p_name, MCObject*& r_object);
	Exec_stat setcard(MCCard *card, Boolean recent, Boolean dynamic);
	MCStack *findstackname_string(MCStringRef name);
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
	MCObject *getsubstackobjname(Chunk_term type, MCNameRef);
	MCObject *getobjname(Chunk_term type, MCNameRef);
	void createmenu(MCControl *nc, uint2 width, uint2 height);
	void menuset(uint2 button, uint2 defy);
	void menumup(uint2 which, MCStringRef &r_string, uint2 &selline);
	void menukdown(MCStringRef p_string, KeySym key, MCStringRef &r_string, uint2 &selline);
	void findaccel(uint2 key, MCStringRef &r_pick, bool &r_disabled);
	void raise();
	void enter();
	void flip(uint2 count);
	bool sort(MCExecContext &ctxt, Sort_type dir, Sort_type form,
	               MCExpression *by, Boolean marked);
	void breakstring(MCStringRef, MCStringRef*& dest, uindex_t &nstrings,
	                 Find_mode fmode);
	Boolean findone(MCExecContext &ctxt, Find_mode mode, MCStringRef *strings,
	                uint2 nstrings, MCChunk *field, Boolean firstcard);
	void find(MCExecContext &ctxt, Find_mode mode, MCStringRef, MCChunk *field);
	void markfind(MCExecContext &ctxt, Find_mode mode, MCStringRef,
	              MCChunk *, Boolean mark);
    void mark(MCExecContext& ctxt, MCExpression *p_where, bool p_mark);
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
		return MCNameGetString(getname());
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

	void effectrect(const MCRectangle &drect, Boolean &abort);
	
	// IM-2014-07-09: [[ Bug 12225 ]] Find the stack by window ID
	MCStack *findstackwindowid(uintptr_t p_win_id);
	MCStack *findstackd(Window w);
	
	// IM-2014-07-23: [[ Bug 12930 ]] Replace findchildstack method with iterating method
	bool foreachchildstack(MCStackForEachCallback p_callback, void *p_context);
    
	bool foreachstack(MCStackForEachCallback p_callback, void *p_context);
	
	void realize();
	void sethints();
	// IM-2013-10-08: [[ FullscreenMode ]] Separate out window sizing hints
	void setsizehints();
    
	bool createwindow();
	void destroywindow();
	
	void destroywindowshape();
    void updatewindowshape(MCWindowShape *shape);

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
	
	bool configure_window_buffer();
	void release_window_buffer();

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

	// MW-2013-11-07: [[ Bug 11393 ]] This returns true if the stack should use device-independent
	//   metrics.
	bool getuseideallayout(void);

    // MW-2014-09-30: [[ ScriptOnlyStack ]] Set the stack as a 'script stack'. The script for
    //   the stack is taken from ep.
    bool isscriptonly(void) const { return m_is_script_only; }
    void setasscriptonly(MCStringRef p_script);
    
    // BWM-2017-08-16: [[ Bug 17810 ]] Get/set line endings for imported script-only-stack.
    MCStringLineEndingStyle getlineencodingstyle(void) const
    {
        return m_line_encoding_style;
    }
    
    void setlineencodingstyle(MCStringLineEndingStyle p_line_encoding_style)
    {
        m_line_encoding_style = p_line_encoding_style;
    }

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

	void constrain(MCPoint p_size, MCPoint& r_out_size);
	
#if defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
	MCSysWindowHandle getrealwindow();
#ifdef _WINDOWS_DESKTOP
	MCSysWindowHandle getqtwindow(void);

	// MW-2011-09-14: [[ Redraw ]] The 'onpaint()' method is called when a WM_PAINT
	//   event is called.
	void onpaint(void);

	// MW-2011-09-14: [[ Redraw ]] This applies the updated window mask buffer to the
	//   window.
	void composite(void);
	
	void getstyle(uint32_t &wstyle, uint32_t &exstyle);
	void constrain(intptr_t lp);

#endif // _WINDOWS_DESKTOP specific
#elif defined(_MAC_DESKTOP)
#elif defined(_LINUX_DESKTOP)
	void setmodalhints(void);

	// MW-201-09-15: [[ Redraw ]] The 'onexpose()' method is called when a sequence
	//   of Expose events are recevied.
	void onexpose(MCRegionRef dirty);

	// IM-2014-01-29: [[ HiDPI ]] platform-specific view device methods

	MCRectangle view_device_getwindowrect() const;
	MCRectangle view_device_setgeom(const MCRectangle &p_rect,
		uint32_t p_minwidth, uint32_t p_minheight,
		uint32_t p_maxwidth, uint32_t p_maxheight);
	void view_device_updatewindow(MCRegionRef p_region);
#elif defined(_MOBILE)

	// IM-2014-01-30: [[ HiDPI ]] platform-specific view device methods
	
	void view_device_updatewindow(MCRegionRef p_region);
#endif
	
	bool cursoroverride ;

	void start_externals();
	void stop_externals();
    
    uint2 getminwidth() const { return minwidth; }
    uint2 getmaxwidth() const { return maxwidth; }
    uint2 getminheight() const { return minheight; }
    uint2 getmaxheight() const { return maxheight; }

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
	bool haswindow(void);

    bool attach(void *ctxt, MCStackAttachmentCallback callback);
    void detach(void *ctxt, MCStackAttachmentCallback callback);
    void notifyattachments(MCStackAttachmentEvent event);
    
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
    void GetFullscreenMode(MCExecContext& ctxt, intenum_t& r_mode);
    void SetFullscreenMode(MCExecContext& ctxt, intenum_t p_mode);
    void GetScaleFactor(MCExecContext& ctxt, double& r_scale);
    void SetScaleFactor(MCExecContext& ctxt, double p_scale);
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
	void GetWindowShadow(MCExecContext& ctxt, bool& r_setting);
	void SetWindowShadow(MCExecContext& ctxt, bool setting);
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
	void GetUnderlineLinks(MCExecContext& ctxt, bool*& r_value);
	void SetUnderlineLinks(MCExecContext& ctxt, bool* p_value);
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
	void GetCompositorType(MCExecContext& ctxt, intenum_t& r_type);
	void SetCompositorType(MCExecContext& ctxt, intenum_t p_type);
	void GetDeferScreenUpdates(MCExecContext& ctxt, bool& r_value);
	void SetDeferScreenUpdates(MCExecContext& ctxt, bool p_value);
	void GetEffectiveDeferScreenUpdates(MCExecContext& ctxt, bool& r_value);
    void SetDecorations(MCExecContext& ctxt, const MCInterfaceDecoration& p_value);
    void GetDecorations(MCExecContext& ctxt, MCInterfaceDecoration& r_value);
    
	void GetCompositorTileSize(MCExecContext& ctxt, uinteger_t*& p_size);
	void SetCompositorTileSize(MCExecContext& ctxt, uinteger_t* p_size);
	void GetCompositorCacheLimit(MCExecContext& ctxt, uinteger_t*& p_size);
	void SetCompositorCacheLimit(MCExecContext& ctxt, uinteger_t* p_size);

	void GetPassword(MCExecContext& ctxt, MCValueRef& r_value);
    void SetPassword(MCExecContext &ctxt, MCValueRef p_password);
    void GetKey(MCExecContext& ctxt, MCValueRef& r_value);
    void SetKey(MCExecContext &ctxt, MCValueRef p_password);
    
    // SN-2014-06-25: [[ IgnoreMouseEvents ]] Setter and getter added
    void SetIgnoreMouseEvents(MCExecContext &ctxt, bool p_ignore);
    void GetIgnoreMouseEvents(MCExecContext &ctxt, bool &r_ignored);
    
    // MERG-2015-08-31: [[ ScriptOnly ]] Setter and getter added
    void GetScriptOnly(MCExecContext &ctxt, bool &r_script_only);
    void SetScriptOnly(MCExecContext &ctxt, bool p_script_only);
    
    // MERG-2015-10-11: [[ DocumentFilename ]] Add stack documentFilename property
    void GetDocumentFilename(MCExecContext &ctxt, MCStringRef& r_document_filename);
    void SetDocumentFilename(MCExecContext &ctxt, MCStringRef p_document_filename);
	
	// IM-2016-02-26: [[ Bug 16244 ]] Add stack showInvisibles property
	void GetShowInvisibleObjects(MCExecContext &ctxt, bool *&r_show_invisibles);
	void SetShowInvisibleObjects(MCExecContext &ctxt, bool *p_show_invisibles);
	void GetEffectiveShowInvisibleObjects(MCExecContext &ctxt, bool &r_show_invisibles);
    
    void GetMinStackFileVersion(MCExecContext &ctxt, MCStringRef& r_stack_file_version);
    
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
    
#ifdef MODE_DEVELOPMENT
    void GetReferringStack(MCExecContext& ctxt, MCStringRef& r_id);
    void GetUnplacedGroupIds(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_ids);
    void GetIdeOverride(MCExecContext& ctxt, bool& r_value);
    void SetIdeOverride(MCExecContext& ctxt, bool p_value);
#endif
    
private:
    /* Explicitly forbid use of the base class's load() method */
	using MCObject::load;

	void loadexternals(void);
	void unloadexternals(void);
    
    // MERG-2015-10-12: [[ DocumentFilename ]] documentFilename property
    void updatedocumentfilename(void);

	void mode_load(void);

	void mode_getrealrect(MCRectangle& r_rect);
	void mode_takewindow(MCStack *other);
	void mode_takefocus(void);
	void mode_setgeom(void);
	void mode_setcursor(void);
	
	bool mode_openasdialog(void);
	void mode_closeasdialog(void);
};
#endif
