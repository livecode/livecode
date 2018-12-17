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

#include "platform.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "exec.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "debug.h"
#include "resolution.h"
#include "player.h"
#include "dispatch.h"
#include "redraw.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-13: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   HIView update.
// IM-2013-06-19: [[ RefactorGraphics ]] Now using callback function to update
//   the HIView instead of a Pixmap
static MCStackUpdateCallback s_update_callback = nil;
static void *s_update_context = nil;

////////////////////////////////////////////////////////////////////////////////

// We have:
//   topLevel
//   topLevelLocked
//   modeless
//   closed
//			=> document
//   palette => floating
//
//   modal => modal
//
//   drawer => drawer
//
//   sheet => sheet
//
// decorations & UTILITY => system
// decorations & CLOSE
// decorations & METAL
// decorations & NOSHADOW
// decorations & MINIMIZE
// decorations & MAXIMIZE
// decorations & LIVERESIZING
// decorations & RESIZABLE

// TITLE
// MENU

void MCStack::realize(void)
{
	if (!MCnoui)
	{
		// Sort out fullscreen mode
		
		// IM-2013-08-01: [[ ResIndependence ]] scale stack rect to device coords
		MCRectangle t_device_rect;
		// IM-2014-06-06: [[ Bug 12321 ]] Get the window rect from the view rather than the stack
		t_device_rect = view_getrect();
	
		// Sort out name ?
		
		loadwindowshape();
		
		// Compute the level of the window
        // MW-2014-06-11: [[ Bug 12297 ]] Make sure 'popup' mode means PopUp window (i.e. no decorations!)
		MCPlatformWindowStyle t_window_style;;
		if (getflag(F_DECORATIONS) && (decorations & WD_UTILITY) != 0)
			t_window_style = kMCPlatformWindowStyleUtility;
		else if (mode == WM_PALETTE
                 || mode == WM_DRAWER)  // COCOA-TODO: Implement drawers
			t_window_style = kMCPlatformWindowStylePalette;
		else if (mode == WM_MODAL || mode == WM_SHEET)
			t_window_style = kMCPlatformWindowStyleDialog;
		else if (mode == WM_PULLDOWN || mode == WM_OPTION || mode == WM_COMBO || mode == WM_POPUP)
			t_window_style = kMCPlatformWindowStylePopUp;
		else if (mode == WM_CASCADE)
			t_window_style = kMCPlatformWindowStylePopUp;
		else if (mode == WM_TOOLTIP)
			t_window_style = kMCPlatformWindowStyleToolTip;
		else
			t_window_style = kMCPlatformWindowStyleDocument;
		
		bool t_has_titlebox, t_has_closebox, t_has_collapsebox, t_has_zoombox, t_has_sizebox;
		if (getflag(F_DECORATIONS))
		{
			t_has_titlebox = (decorations & WD_TITLE) != 0;
			t_has_closebox = (decorations & WD_CLOSE) != 0;
			t_has_collapsebox = (decorations & WD_MINIMIZE) != 0;
            
            // MW-2014-04-22: [[ Bug 12264 ]] Wrong test for maximize (was == 0!).
			t_has_zoombox = (decorations & WD_MAXIMIZE) != 0;
		}
		else
		{
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
			if (t_window_style == kMCPlatformWindowStyleDocument)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_zoombox = true;
				t_has_collapsebox = true;
				t_has_sizebox = true;
			}
			else if (t_window_style == kMCPlatformWindowStylePalette ||
					 t_window_style == kMCPlatformWindowStyleUtility)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_collapsebox = true;
			}
			else if (t_window_style == kMCPlatformWindowStyleDialog)
				t_has_titlebox = true;
		}
		
        // MW-2014-04-22: [[ Bug 12264 ]] Make sure we use the resizable property in all cases, to be
        //   switched off for certain window types.
        t_has_sizebox = getflag(F_RESIZABLE);
        
		if (getflag(F_DECORATIONS) && ((decorations & (WD_TITLE | WD_MENU | WD_CLOSE | WD_MINIMIZE | WD_MAXIMIZE)) == 0))
			t_has_sizebox = false;
		
		// If the window has a windowshape, we don't have any decorations.
		if (m_window_shape != nil)
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
		
		// If the window is not normal, utility or floating we don't have close or zoom boxes.
		if (t_window_style != kMCPlatformWindowStyleDocument &&
			t_window_style != kMCPlatformWindowStyleUtility &&
			t_window_style != kMCPlatformWindowStylePalette)
		{
			t_has_closebox = false;
			t_has_zoombox = false;
		}
		
		// If the window is not normal level, we don't have a collapse box.
		if (t_window_style != kMCPlatformWindowStyleDocument)
			t_has_collapsebox = false;
		
		// If the window is not one that would be expected to be resizable, don't give it
		// a size box.
		if (t_window_style != kMCPlatformWindowStyleDocument &&
			t_window_style != kMCPlatformWindowStylePalette &&
			t_window_style != kMCPlatformWindowStyleUtility &&
			t_window_style != kMCPlatformWindowStyleDialog)
			t_has_sizebox = false;
		
		// If the window is a tooltip or popup, we want no decorations.
		if (t_window_style == kMCPlatformWindowStyleToolTip ||
			t_window_style == kMCPlatformWindowStylePopUp)
		{
			t_has_titlebox = false;
			t_has_closebox = false;
			t_has_zoombox = false;
			t_has_collapsebox = false;
			t_has_sizebox = false;
		}
		
		MCPlatformWindowRef t_window;
		MCPlatformCreateWindow(t_window);
		
		window = t_window;
		
		MCPlatformSetWindowContentRect(t_window, t_device_rect);
		
		if (m_window_shape != nil)
			MCPlatformSetWindowProperty(t_window, kMCPlatformWindowPropertyMask, kMCPlatformPropertyTypeWindowMask, (MCPlatformWindowMaskRef *)&m_window_shape -> handle);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyIsOpaque, isopaque());
		MCPlatformSetWindowProperty(t_window, kMCPlatformWindowPropertyStyle, kMCPlatformPropertyTypeWindowStyle, &t_window_style);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasTitleWidget, t_has_titlebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasCloseWidget, t_has_closebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasCollapseWidget, t_has_collapsebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasZoomWidget, t_has_zoombox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasSizeWidget, t_has_sizebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasShadow, (decorations & WD_NOSHADOW) == 0);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyUseLiveResizing, (decorations & WD_LIVERESIZING) != 0);
        
        // MW-2014-04-23: [[ Bug 12080 ]] If the window is a palette and hidePalettes is true then HideOnSuspend.
        MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHideOnSuspend, MChidepalettes && t_window_style == kMCPlatformWindowStylePalette);
		
		setopacity(blendlevel * 255 / 100);
		
		// Sort out drawers
		
		updatemodifiedmark();
        
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] update the window with the ignore mouse events property
        updateignoremouseevents();
        
        // MW-2014-06-11: [[ Bug 12467 ]] Make sure we reset the cursor property of the window.
        resetcursor(True);
        
        // MERG-2015-10-11: [[ DocumentFilename ]] update the window with the document filename property
        MCPlatformSetWindowProperty(t_window, kMCPlatformWindowPropertyDocumentFilename, kMCPlatformPropertyTypeMCString, &m_document_filename);
	}
	
	start_externals();
}

MCRectangle MCStack::view_platform_getwindowrect() const
{
    MCRectangle t_frame_rect;
	MCPlatformGetWindowFrameRect(window, t_frame_rect);
    if (MClockscreen != 0)
    {
        MCRectangle t_content_rect, t_diff_rect;
        MCscreen->platform_getwindowgeometry(window, t_content_rect);
        // the content rect of a window should always be contained (or equal) to the frame rect
        // so compute these 4 margins and then apply them to the rect of the stack
        t_diff_rect.x = rect.x - (t_content_rect.x - t_frame_rect.x);
        t_diff_rect.y = rect.y - (t_content_rect.y - t_frame_rect.y);
        t_diff_rect.width = rect.width + (t_frame_rect.width - t_content_rect.width);
        t_diff_rect.height = rect.height + (t_frame_rect.height - t_content_rect.height);
        return t_diff_rect;
    }
    
    return t_frame_rect;
	
}

bool MCStack::view_platform_dirtyviewonresize() const
{
	return false;
}

// IM-2013-09-23: [[ FullscreenMode ]] Factor out device-specific window sizing
MCRectangle MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	MCPlatformSetWindowContentRect(window, p_rect);
	return p_rect;
}

void MCStack::syncscroll(void)
{
    // AL-2014-07-22: [[ Bug 12764 ]] Update the stack viewport after applying menu scroll
    view_setstackviewport(rect);
	// COCOA-TODO: Make sure contained views also scroll (?)

	// IM-2017-05-04: [[ Bug 19327 ]] Notify layers of transform change after scroll sync
	OnViewTransformChanged();
}

void MCStack::start_externals()
{
	loadexternals();
}

void MCStack::stop_externals()
{
	Boolean oldlock = MClockmessages;
	MClockmessages = True;
	
    MCPlayer::SyncPlayers(this, nil);
    
    MCPlayer::StopPlayers(this);
    
	destroywindowshape();
	
	MClockmessages = oldlock;
	
	unloadexternals();
}

void MCStack::setopacity(uint1 p_level)
{
	if (window == nil)
		return;
	
	MCPlatformSetWindowFloatProperty(window, kMCPlatformWindowPropertyOpacity, p_level / 255.0f);
}

void MCStack::updatemodifiedmark(void)
{
	if (window == nil)
		return;
	
	MCPlatformSetWindowBoolProperty(window, kMCPlatformWindowPropertyHasModifiedMark, getextendedstate(ECS_MODIFIED_MARK) == True);
}

// MERG-2014-06-02: [[ IgnoreMouseEvents ]] update the window with the ignore mouse events property
void MCStack::updateignoremouseevents(void)
{
	if (window == nil)
		return;
	
	MCPlatformSetWindowBoolProperty(window, kMCPlatformWindowPropertyIgnoreMouseEvents, getextendedstate(ECS_IGNORE_MOUSE_EVENTS) == True);
}

// MW-2011-09-11: [[ Redraw ]] Force an immediate update of the window within the given
//   region. The actual rendering is done by deferring to the 'redrawwindow' method.
void MCStack::view_platform_updatewindow(MCRegionRef p_region)
{
	if (window == nil)
		return;
    
    if (!opened)
        return;
    
    if (!isvisible())
        return;
    
	MCPlatformInvalidateWindow(window, p_region);
	MCPlatformUpdateWindow(window);
}

void MCStack::view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	// Set the file-local static to the callback to use (stacksurface picks this up!)
	s_update_callback = p_callback;
	s_update_context = p_context;
	// IM-2013-08-29: [[ RefactorGraphics ]] simplify by calling device_updatewindow, which performs the same actions
	view_platform_updatewindow(p_region);
	// Unset the file-local static.
	s_update_callback = nil;
	s_update_context = nil;
}

// MERG-2015-10-12: [[ DocumentFilename ]] Stub for documentFilename.
void MCStack::updatedocumentfilename(void)
{
    if (window != nil)
        MCPlatformSetWindowProperty(window, kMCPlatformWindowPropertyDocumentFilename, kMCPlatformPropertyTypeMCString, &m_document_filename);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2014-06-11: [[ Bug 12495 ]] Update windowshape by setting window property.
void MCStack::updatewindowshape(MCWindowShape *p_shape)
{
    destroywindowshape();
    m_window_shape = p_shape;
    // IM-2014-10-22: [[ Bug 13746 ]] Check for platform window before setting mask
	if (window != nil)
		MCPlatformSetWindowProperty(window, kMCPlatformWindowPropertyMask, kMCPlatformPropertyTypeWindowMask, (MCPlatformWindowMaskRef *)&m_window_shape -> handle);
    dirtyall();
}

void MCStack::destroywindowshape(void)
{
	if (m_window_shape == nil)
		return;
	
	delete[] m_window_shape -> data;
	if (m_window_shape -> handle != nil)
		MCPlatformWindowMaskRelease((MCPlatformWindowMaskRef)m_window_shape -> handle);
	delete m_window_shape;
	m_window_shape = nil;
}

////////////////////////////////////////////////////////////////////////////////

class MCDesktopStackSurface: public MCStackSurface
{
	MCPlatformSurfaceRef m_surface;
public:
	MCDesktopStackSurface(MCPlatformSurfaceRef p_surface)
	{
		m_surface = p_surface;
	}
	
	~MCDesktopStackSurface(void)
	{
	}
	
	bool Lock(void)
	{
		return true;
	}
	
	void Unlock(void)
	{
	}
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to wrap new platform surface API.
	bool LockGraphics(MCGIntegerRectangle p_region, MCGContextRef& r_context, MCGRaster &r_raster)
	{
		return MCPlatformSurfaceLockGraphics(m_surface, p_region, r_context, r_raster);
	}
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to wrap new platform surface API.
	void UnlockGraphics(MCGIntegerRectangle p_region, MCGContextRef p_context, MCGRaster &p_raster)
	{
		MCPlatformSurfaceUnlockGraphics(m_surface, p_region, p_context, p_raster);
	}
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to wrap new platform surface API.
	bool LockPixels(MCGIntegerRectangle p_area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
	{
		return MCPlatformSurfaceLockPixels(m_surface, p_area, r_raster, r_locked_area);
	}
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to wrap new platform surface API.
	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
		MCPlatformSurfaceUnlockPixels(m_surface, p_area, p_raster);
	}
	
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		assert(p_type == kMCStackSurfaceTargetCoreGraphics);
		return MCPlatformSurfaceLockSystemContext(m_surface, r_context);
	}
	
	void UnlockTarget(void)
	{
		MCPlatformSurfaceUnlockSystemContext(m_surface);
	}
	
	bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_source, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		return MCPlatformSurfaceComposite(m_surface, p_dst_rect, p_source, p_src_rect, p_alpha, p_blend);
	}
};

// This method is not an MCStack method, however it is related to the file locals
// in here to do with update. At some point it should be refactored appropriately.
void MCDispatch::wredraw(Window p_window, MCPlatformSurfaceRef p_surface, MCGRegionRef p_update_rgn)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	// IM-2014-01-24: [[ HiRes ]] Update the view backing scale to match the surface
	t_stack -> view_setbackingscale(MCPlatformSurfaceGetBackingScaleFactor(p_surface));
	
	MCDesktopStackSurface t_stack_surface(p_surface);
	
	// If we don't have an update pixmap, then use redrawwindow.
	if (s_update_callback == nil)
		t_stack -> view_surface_redrawwindow(&t_stack_surface, p_update_rgn);
	else
		s_update_callback(&t_stack_surface, (MCRegionRef)p_update_rgn, s_update_context);
}

void MCDispatch::wiconify(Window p_window)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	t_stack -> iconify();
}

void MCDispatch::wuniconify(Window p_window)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	t_stack -> uniconify();
}

////////////////////////////////////////////////////////////////////////////////
