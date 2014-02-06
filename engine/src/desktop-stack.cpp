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

#include "platform.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
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
		t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(rect));
	
		// Sort out name ?
		
		loadwindowshape();
		
		// Compute the level of the window
		MCPlatformWindowStyle t_window_style;;
		if (getflag(F_DECORATIONS) && (decorations & WD_UTILITY) != 0)
			t_window_style = kMCPlatformWindowStyleUtility;
		else if (mode == WM_PALETTE)
			t_window_style = kMCPlatformWindowStylePalette;
		else if (mode == WM_MODAL || mode == WM_SHEET)
			t_window_style = kMCPlatformWindowStyleDialog;
		else if (mode == WM_PULLDOWN || mode == WM_OPTION || mode == WM_COMBO)
			t_window_style = kMCPlatformWindowStylePopUp;
		else if (mode == WM_CASCADE)
			t_window_style = kMCPlatformWindowStylePopUp;
		else if (mode == WM_TOOLTIP)
			t_window_style = kMCPlatformWindowStyleToolTip;
		else if (mode == WM_DRAWER)
			; // COCOA-TODO
		else
			t_window_style = kMCPlatformWindowStyleDocument;
		
		bool t_has_titlebox, t_has_closebox, t_has_collapsebox, t_has_zoombox, t_has_sizebox;
		if (getflag(F_DECORATIONS))
		{
			t_has_titlebox = (decorations & WD_TITLE) != 0;
			t_has_closebox = (decorations & WD_CLOSE) != 0;
			t_has_collapsebox = (decorations & WD_MINIMIZE) != 0;
			t_has_zoombox = (decorations & WD_MAXIMIZE) == 0;
			t_has_sizebox = getflag(F_RESIZABLE);
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
		
		
		MCPlatformWindowRef t_window;
		MCPlatformCreateWindow(t_window);
		
		window = t_window;
		
		MCPlatformSetWindowContentRect(t_window, t_device_rect);
		
		if (m_window_shape != nil)
			MCPlatformSetWindowProperty(t_window, kMCPlatformWindowPropertyMask, kMCPlatformPropertyTypeWindowMask, (MCPlatformWindowMaskRef *)&m_window_shape -> handle);
		MCPlatformSetWindowProperty(t_window, kMCPlatformWindowPropertyStyle, kMCPlatformPropertyTypeWindowStyle, &t_window_style);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasTitleWidget, t_has_titlebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasCloseWidget, t_has_closebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasCollapseWidget, t_has_collapsebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasZoomWidget, t_has_zoombox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasSizeWidget, t_has_sizebox);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyHasShadow, (decorations & WD_NOSHADOW) != 0);
		MCPlatformSetWindowBoolProperty(t_window, kMCPlatformWindowPropertyUseLiveResizing, (decorations & WD_LIVERESIZING) != 0);
									
#ifdef PRE_PLATFORM
		// Compute the level of the window
		int32_t t_window_level;
		if (getflag(F_DECORATIONS) && (decorations & WD_UTILITY) != 0)
			t_window_level = kCGUtilityWindowLevelKey;
		else if (mode == WM_PALETTE)
			t_window_level = kCGFloatingWindowLevelKey;
		else if (mode == WM_MODAL || mode == WM_SHEET)
			t_window_level = kCGModalPanelWindowLevelKey;
		else if (mode == WM_PULLDOWN || mode == WM_OPTION || mode == WM_COMBO)
			t_window_level = kCGPopUpMenuWindowLevelKey;
		else if (mode == WM_CASCADE)
			t_window_level = kCGPopUpMenuWindowLevelKey;
		else if (mode == WM_TOOLTIP)
			t_window_level = kCGStatusWindowLevelKey;
		else if (mode == WM_SHEET)
			; // COCOA-TODO
		else if (mode == WM_DRAWER)
			; // COCOA-TODO
		else
			t_window_level = kCGNormalWindowLevelKey;
		
		bool t_has_titlebox, t_has_closebox, t_has_collapsebox, t_has_zoombox, t_has_sizebox;
		if (getflag(F_DECORATIONS))
		{
			t_has_titlebox = (decorations & WD_TITLE) != 0;
			t_has_closebox = (decorations & WD_CLOSE) != 0;
			t_has_collapsebox = (decorations & WD_MINIMIZE) != 0;
			t_has_zoombox = (decorations & WD_MAXIMIZE) == 0;
			t_has_sizebox = getflag(F_RESIZABLE);
		}
		else
		{
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
			if (t_window_level == kCGNormalWindowLevelKey)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_zoombox = true;
				t_has_collapsebox = true;
				t_has_sizebox = true;
			}
			else if (t_window_level == kCGFloatingWindowLevelKey ||
					 t_window_level == kCGUtilityWindowLevelKey)
			{
				t_has_titlebox = true;
				t_has_closebox = true;
				t_has_collapsebox = true;
			}
		}
		
		if (getflag(F_DECORATIONS) && ((decorations & (WD_TITLE | WD_MENU | WD_CLOSE | WD_MINIMIZE | WD_MAXIMIZE)) == 0))
			t_has_sizebox = false;
		
		// If the window has a windowshape, we don't have any decorations.
		if (m_window_shape != nil)
			t_has_titlebox = t_has_closebox = t_has_collapsebox = t_has_zoombox = t_has_sizebox = false;
		
		// If the window is not normal, utility or floating we don't have close or zoom boxes.
		if (t_window_level != kCGNormalWindowLevelKey &&
			t_window_level != kCGUtilityWindowLevelKey &&
			t_window_level != kCGFloatingWindowLevelKey)
		{
			t_has_closebox = false;
			t_has_zoombox = false;
		}
		
		// If the window is not normal level, we don't have a collapse box.
		if (t_window_level != kCGNormalWindowLevelKey)
			t_has_collapsebox = false;
		
		// If the window is not one that would be expected to be resizable, don't give it
		// a size box.
		if (t_window_level != kCGNormalWindowLevelKey &&
			t_window_level != kCGFloatingWindowLevelKey &&
			t_window_level != kCGUtilityWindowLevelKey &&
			t_window_level != kCGModalPanelWindowLevelKey)
		/*t_window_level != kCGSheet / Drawer*/
			t_has_sizebox = false;
		
		// Compute the style of the window
		NSUInteger t_window_style;
		t_window_style = NSBorderlessWindowMask;
		if (t_has_titlebox)
			t_window_style |= NSTitledWindowMask;
		if (t_has_closebox)
			t_window_style |= NSClosableWindowMask;
		if (t_has_collapsebox)
			t_window_style |= NSMiniaturizableWindowMask;
		if (t_has_sizebox)
			t_window_style |= NSResizableWindowMask;
		if (t_window_level == kCGFloatingWindowLevelKey)
			t_window_style |= NSUtilityWindowMask;

		NSRect t_rect;
		t_rect = NSRectFromMCRectangleGlobal(t_device_rect);

		NSWindow *t_window;
		if (t_window_level != kCGFloatingWindowLevelKey)
			t_window = [[NSWindow alloc] initWithContentRect: t_rect styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
		else
			t_window = [[NSPanel alloc] initWithContentRect: t_rect styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
		
		com_runrev_livecode_MCStackWindowDelegate *t_delegate;
		t_delegate = [[com_runrev_livecode_MCStackWindowDelegate alloc] init];
		[t_window setDelegate: t_delegate];
		
		// Set the stack window
		window = (MCSysWindowHandle)t_window;
		
		// Create the content view
		[t_window setContentView: [[[com_runrev_livecode_MCStackView alloc] initWithFrame: NSZeroRect] autorelease]];
		
		// Configure properties of the window now its been created.
		if (m_window_shape != nil)
			[t_window setOpaque: NO];
		
		if ((decorations & WD_NOSHADOW) != 0)
			[t_window setHasShadow: NO];
		
		if ((decorations & WD_LIVERESIZING) != 0)
			; // COCOA-TODO
		
		if (t_has_zoombox)
			[[t_window standardWindowButton: NSWindowZoomButton] setEnabled: NO];
		
		[t_window setLevel: t_window_level];
#endif
		
		setopacity(blendlevel * 255 / 100);
		
		// Sort out drawers
		
		updatemodifiedmark();
	}
	
	start_externals();
}

MCRectangle MCStack::device_getwindowrect() const
{
	MCRectangle t_rect;
	MCPlatformGetWindowFrameRect(window, t_rect);
	return t_rect;
	
#ifdef PRE_PLATFORM
	NSRect t_frame;
	t_frame = [(NSWindow *)window frame];
	return NSRectToMCRectangleGlobal(t_frame);
#endif
}

// IM-2013-09-23: [[ FullscreenMode ]] Factor out device-specific window sizing
void MCStack::device_setgeom(const MCRectangle &p_rect)
{
	MCPlatformSetWindowContentRect(window, p_rect);
	
#ifdef PRE_PLATFORM
	NSRect t_content;
	t_content = [(NSWindow *)window contentRectForFrameRect: [(NSWindow *)window frame]];
	
	MCRectangle t_win_rect;
	t_win_rect = NSRectToMCRectangleGlobal(t_content); //MCU_make_rect(t_content . origin . x, t_content . origin . y, t_content . size . width, t_content . size . height);
	
	/*if ([(NSWindow *)window isVisible])
	 {
	 if (mode != WM_SHEET && mode != WM_DRAWER &&
	 (p_rect.x != t_win_rect.x || p_rect.y != t_win_rect.y))
	 [(NSWindow *)window setFrameTopLeftPoint: NSMakePoint(p_rect.x, p_rect.y)];
	 }
	 else
	 {
	 if (mode != WM_SHEET && mode != WM_DRAWER)
	 [(NSWindow *)window setFrameTopLeftPoint: NSMakePoint(p_rect.x, p_rect.y)];
	 }
	 
	 if (p_rect.width != t_win_rect.width || p_rect.height != t_win_rect.height)
	 [(NSWindow *)window setContentSize:	NSMakeSize(p_rect . width, p_rect . height)];*/
	
	if (mode != WM_SHEET && mode != WM_DRAWER &&
		!MCU_equal_rect(p_rect, t_win_rect))
		[(NSWindow *)window setFrame: [(NSWindow *)window frameRectForContentRect: NSRectFromMCRectangleGlobal(p_rect)] display: YES];
#endif
}

void MCStack::syncscroll(void)
{
	// COCOA-TODO: Do something with scroll
}

void MCStack::start_externals()
{
#ifdef PRE_PLATFORM
	[[(NSWindow *)window contentView] setStack: this];
	[[(NSWindow *)window delegate] setStack: this];
#endif
	loadexternals();
}

void MCStack::stop_externals()
{
	Boolean oldlock = MClockmessages;
	MClockmessages = True;
	
	MCPlayer *tptr = MCplayers;
	
	while (tptr != NULL)
	{
		if (tptr->getstack() == this)
		{
			if (tptr->playstop())
				tptr = MCplayers; // was removed, start search over
		}
		else
			tptr = tptr->getnextplayer();
	}
	destroywindowshape();
	
	MClockmessages = oldlock;
	
	unloadexternals();
	
#ifdef PRE_PLATFORM
	[[(NSWindow *)window contentView] setStack: nil];
	[[(NSWindow *)window delegate] setStack: nil];
#endif
}

void MCStack::setopacity(uint1 p_level)
{
	if (window == nil)
		return;
	
	MCPlatformSetWindowFloatProperty(window, kMCPlatformWindowPropertyOpacity, p_level / 255.0f);
	
#ifdef PRE_PLATFORM
	[(NSWindow *)window setAlphaValue: p_level / 255.0f];
#endif
}

void MCStack::updatemodifiedmark(void)
{
	if (window == nil)
		return;
	
	MCPlatformSetWindowBoolProperty(window, kMCPlatformWindowPropertyHasModifiedMark, getextendedstate(ECS_MODIFIED_MARK) == True);
	
#ifdef PRE_PLATFORM	
	[(NSWindow *)window setDocumentEdited: getextendedstate(ECS_MODIFIED_MARK) == True];
#endif
}

// MW-2011-09-11: [[ Redraw ]] Force an immediate update of the window within the given
//   region. The actual rendering is done by deferring to the 'redrawwindow' method.
void MCStack::device_updatewindow(MCRegionRef p_region)
{
	if (window == nil)
		return;
	
	MCPlatformInvalidateWindow(window, p_region);
	MCPlatformUpdateWindow(window);
	
#ifdef PRE_PLATFORM
	if (window == nil)
		return;
	
	NSView *t_view;
	t_view = [(NSWindow *)window contentView];
	
	if (!getextendedstate(ECS_MASK_CHANGED) || s_update_callback != nil)
		[t_view setNeedsDisplayInRect: NSRectFromMCRectangleLocal(t_view, MCRegionGetBoundingBox(p_region))];
	else
	{
		[t_view setNeedsDisplay: YES];
		NSDisableScreenUpdates();
	}
	
	[t_view display];
	
	if (getextendedstate(ECS_MASK_CHANGED))
	{
		[(NSWindow *)window invalidateShadow];
		
		NSEnableScreenUpdates();
		
		setextendedstate(False, ECS_MASK_CHANGED);
	}
#endif
}

void MCStack::device_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	// Set the file-local static to the callback to use (stacksurface picks this up!)
	s_update_callback = p_callback;
	s_update_context = p_context;
	// IM-2013-08-29: [[ RefactorGraphics ]] simplify by calling device_updatewindow, which performs the same actions
	device_updatewindow(p_region);
	// Unset the file-local static.
	s_update_callback = nil;
	s_update_context = nil;
}

////////////////////////////////////////////////////////////////////////////////

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
	
	bool LockGraphics(MCRegionRef p_area, MCGContextRef& r_context)
	{
		return MCPlatformSurfaceLockGraphics(m_surface, p_area, r_context);
	}
	
	void UnlockGraphics(void)
	{
		MCPlatformSurfaceUnlockGraphics(m_surface);
	}
	
	bool LockPixels(MCRegionRef p_area, MCGRaster& r_raster)
	{
		return MCPlatformSurfaceLockPixels(m_surface, p_area, r_raster);
	}
	
	void UnlockPixels(void)
	{
		MCPlatformSurfaceUnlockPixels(m_surface);
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
void MCDispatch::wredraw(Window p_window, MCPlatformSurfaceRef p_surface, MCRegionRef p_update_rgn)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	MCDesktopStackSurface t_stack_surface(p_surface);
	
	// If we don't have an update pixmap, then use redrawwindow.
	if (s_update_callback == nil)
		t_stack -> device_redrawwindow(&t_stack_surface, (MCRegionRef)p_update_rgn);
	else
		s_update_callback(&t_stack_surface, (MCRegionRef)p_update_rgn, s_update_context);
}

void MCDispatch::wreshape(Window p_window)
{
	MCStack *t_stack;
	t_stack = findstackd(p_window);
	if (t_stack == nil)
		return;
	
	t_stack -> view_configure(true);
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
