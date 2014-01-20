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

#ifndef __MC_PLATFORM__
#define __MC_PLATFORM__

#ifndef __MC_GLOBDEFS__
#include "globdefs.h"
#endif

#ifndef __MC_GRAPHICS__
#include "graphics.h"
#endif

////////////////////////////////////////////////////////////////////////////////

// COCOA-TODO: Remove external declaration.
struct MCImageBitmap;

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformCallbackType
{
	kMCPlatformCallbackApplicationStartup,
	kMCPlatformCallbackApplicationShutdown,
	kMCPlatformCallbackApplicationShutdownRequest,
	kMCPlatformCallbackApplicationSuspend,
	kMCPlatformCallbackApplicationResume,
	kMCPlatformCallbackApplicationRun,
};

struct MCPlatformCallback
{
	MCPlatformCallbackType type;
	union
	{
		union
		{
			struct
			{
				// Incoming fields
				uindex_t argc;
				char **argv;
				char **envp;
				
				// Outgoing fields
				int error_code;
				char *error_message;
			} startup;
			
			struct
			{
				// Incoming fields
				
				// Outgoing fields
				int exit_code;
			} shutdown;
			
			struct
			{
				// Incoming fields
				
				// Outgoing fields
				bool terminate;
			} shutdown_request;
			
		} application;
	};
};

void MCPlatformProcess(MCPlatformCallback& event);

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformPropertyType
{
	kMCPlatformPropertyTypeUnknown,
	
	kMCPlatformPropertyTypeBool,
	kMCPlatformPropertyTypeInt32,
	kMCPlatformPropertyTypeFloat,
	kMCPlatformPropertyTypeDouble,
	kMCPlatformPropertyTypeRectangle,
	kMCPlatformPropertyTypeColor,
	
	kMCPlatformPropertyTypeUTF8CString,
	
	kMCPlatformPropertyTypeWindowStyle,
	kMCPlatformPropertyTypeWindowMask,
	
	kMCPlatformPropertyTypeMenuRef,
	kMCPlatformPropertyTypeAccelerator,
	kMCPlatformPropertyTypeMenuItemHighlight,
	kMCPlatformPropertyTypeMenuItemAction,
	kMCPlatformPropertyTypeCursorImageSupport,
};

typedef uint32_t MCPlatformAccelerator;
enum
{
	kMCPlatformAcceleratorNone,
};

////////////////////////////////////////////////////////////////////////////////

// System properties are settings that can be queried from the system.
//
// TODO-REVIEW: Perhaps these would be better classed as metrics?

enum MCPlatformCursorImageSupport
{
	kMCPlatformCursorImageSupportMonochrome,
	kMCPlatformCursorImageSupportBilevel,
	kMCPlatformCursorImageSupportColor,
	kMCPlatformCursorImageSupportAlpha,
};

enum MCPlatformSystemProperty
{
	kMCPlatformSystemPropertyUnknown,
	
	kMCPlatformSystemPropertyDoubleClickInterval,
	kMCPlatformSystemPropertyCaretBlinkInterval,
	
	kMCPlatformSystemPropertyHiliteColor,
	kMCPlatformSystemPropertyAccentColor,
	
	kMCPlatformSystemPropertyMaximumCursorSize,
	kMCPlatformSystemPropertyCursorImageSupport,
};

void MCPlatformGetSystemProperty(MCPlatformSystemProperty property, MCPlatformPropertyType type, void *value);

////////////////////////////////////////////////////////////////////////////////

// Break the current WaitForEvent which is progress.
void MCPlatformBreakWait(void);

// Wait for any event for at most duration seconds. If blocking is true then
// no events which cause a dispatch should be processed. If an event is processed
// during duration, true is returned; otherwise false is.
bool MCPlatformWaitForEvent(double duration, bool blocking);

// Return true if the abort key has been pressed since the last check.
bool MCPlatformGetAbortKeyPressed(void);

// Get the current (right now!) state of the mouse button.
bool MCPlatformGetMouseButtonState(uindex_t button);

// Peek into the event queue and pull out a mouse click event (down then up)
// for the given button. If button is 0, then any button click will do.
bool MCPlatformGetMouseClick(uindex_t button);

// Get the position of the mouse in global coords.
void MCPlatformGetMousePosition(MCPoint& r_location);
// Set the position of the mouse in global coords.
void MCPlatformSetMousePosition(MCPoint location);

// Make the given window grab the pointer (if possible).
void MCPlatformGrabPointer(MCPlatformWindowRef window);

// Release the pointer from a grab.
void MCPlatformUngrabPointer(void);

// Get the window (that we know about) at the given co-ords.
void MCPlatformGetWindowAtPoint(MCPoint location, MCPlatformWindowRef& r_window);

// Return the 'time' of the last event.
uint32_t MCPlatformGetEventTime(void);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformWindowMask *MCPlatformWindowMaskRef;

void MCPlatformWindowMaskCreate(int32_t width, int32_t height, int32_t stride, void *bits, MCPlatformWindowMaskRef& r_mask);
void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef mask);
void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef mask);

////////////////////////////////////////////////////////////////////////////////

typedef struct MCPlatformMenu *MCPlatformMenuRef;

enum MCPlatformMenuItemProperty
{
	kMCPlatformMenuItemPropertyUnknown,
	
	kMCPlatformMenuItemPropertyTitle,
	kMCPlatformMenuItemPropertyTag,
	kMCPlatformMenuItemPropertyAction,
	kMCPlatformMenuItemPropertyAccelerator,
	kMCPlatformMenuItemPropertyEnabled,
	kMCPlatformMenuItemPropertySubmenu,
	kMCPlatformMenuItemPropertyHighlight,
};

enum MCPlatformMenuItemHighlight
{
	kMCPlatformMenuItemHighlightNone,
	kMCPlatformMenuItemHighlightTick,
	kMCPlatformMenuItemHighlightDiamond,
	kMCPlatformMenuItemHighlightBar,
};

enum MCPlatformMenuItemAction
{
	kMCPlatformMenuItemActionNone,
	
	kMCPlatformMenuItemActionQuit,
};

void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu);
void MCPlatformRetainMenu(MCPlatformMenuRef menu);
void MCPlatformReleaseMenu(MCPlatformMenuRef menu);

void MCPlatformSetMenuTitle(MCPlatformMenuRef menu, const char *title);

void MCPlatformCountMenuItems(MCPlatformMenuRef menu, uindex_t& r_count);

void MCPlatformAddMenuItem(MCPlatformMenuRef menu, uindex_t where);
void MCPlatformAddMenuSeparatorItem(MCPlatformMenuRef menu, uindex_t where);
void MCPlatformRemoveMenuItem(MCPlatformMenuRef menu, uindex_t where);
void MCPlatformRemoveAllMenuItems(MCPlatformMenuRef menu);

void MCPlatformGetMenuParent(MCPlatformMenuRef menu, MCPlatformMenuRef& r_parent, uindex_t& r_index);

void MCPlatformGetMenuItemProperty(MCPlatformMenuRef menu, uindex_t index, MCPlatformMenuItemProperty property, MCPlatformPropertyType type, void *r_value);
void MCPlatformSetMenuItemProperty(MCPlatformMenuRef menu, uindex_t index, MCPlatformMenuItemProperty property, MCPlatformPropertyType type, const void *value);

//////////

void MCPlatformShowMenubar(void);
void MCPlatformHideMenubar(void);

void MCPlatformSetMenubar(MCPlatformMenuRef menu);
void MCPlatformGetMenubar(MCPlatformMenuRef menu);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformCursor *MCPlatformCursorRef;

enum MCPlatformStandardCursor
{
	kMCPlatformStandardCursorUnknown,
	
	kMCPlatformStandardCursorArrow,
	kMCPlatformStandardCursorWatch,
	kMCPlatformStandardCursorCross,
	kMCPlatformStandardCursorIBeam,
};

void MCPlatformCreateStandardCursor(MCPlatformStandardCursor standard_cusor, MCPlatformCursorRef& r_cursor);
void MCPlatformCreateCustomCursor(MCImageBitmap *image, MCPoint hot_spot, MCPlatformCursorRef& r_cursor);
void MCPlatformRetainCursor(MCPlatformCursorRef cursor);
void MCPlatformReleaseCursor(MCPlatformCursorRef cursor);

void MCPlatformShowCursor(MCPlatformCursorRef cursor);
void MCPlatformHideCursor(void);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformWindow *MCPlatformWindowRef;

enum MCPlatformWindowStyle
{
	kMCPlatformWindowStyleNone,
	kMCPlatformWindowStyleDocument,
	kMCPlatformWindowStylePalette,
	kMCPlatformWindowStyleDialog,
	kMCPlatformWindowStyleUtility,
	kMCPlatformWindowStylePopUp,
	kMCPlatformWindowStyleToolTip,
};

void MCPlatformCreateWindow(MCPlatformWindowRef& r_window);
void MCPlatformRetainWindow(MCPlatformWindowRef window);
void MCPlatformReleaseWindow(MCPlatformWindowRef window);

void MCPlatformInvalidateWindow(MCPlatformWindowRef window, MCRegionRef region);
void MCPlatformUpdateWindow(MCPlatformWindowRef window);

void MCPlatformShowWindow(MCPlatformWindowRef window);
void MCPlatformHideWindow(MCPlatformWindowRef window);
void MCPlatformFocusWindow(MCPlatformWindowRef window);
void MCPlatformRaiseWindow(MCPlatformWindowRef window);
void MCPlatformIconifyWindow(MCPlatformWindowRef window);
void MCPlatformUniconifyWindow(MCPlatformWindowRef window);

void MCPlatformSetWindowContentRect(MCPlatformWindowRef window, MCRectangle content_rect);
void MCPlatformGetWindowContentRect(MCPlatformWindowRef window, MCRectangle& r_content_rect);

void MCPlatformSetWindowFrameRect(MCPlatformWindowRef window, MCRectangle frame_rect);
void MCPlatformGetWindowFrameRect(MCPlatformWindowRef window, MCRectangle& r_frame_rect);

enum MCPlatformWindowProperty
{
	kMCPlatformWindowPropertyUnknown,
	
	kMCPlatformWindowPropertyTitle,
	kMCPlatformWindowPropertyStyle,
	kMCPlatformWindowPropertyOpacity,
	kMCPlatformWindowPropertyMask,
	kMCPlatformWindowPropertyFrameRect,
	kMCPlatformWindowPropertyContentRect,
	
	kMCPlatformWindowPropertyHasTitleWidget,
	kMCPlatformWindowPropertyHasCloseWidget,
	kMCPlatformWindowPropertyHasCollapseWidget,
	kMCPlatformWindowPropertyHasZoomWidget,
	kMCPlatformWindowPropertyHasSizeWidget,
	
	kMCPlatformWindowPropertyHasShadow,
	
	kMCPlatformWindowPropertyHasModifiedMark,

	kMCPlatformWindowPropertyUseLiveResizing,
};

void MCPlatformSetWindowProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
void MCPlatformGetWindowProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, MCPlatformPropertyType type, void *value);

void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, bool value);
void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, float value);

void MCPlatformMapPointFromWindowToScreen(MCPlatformWindowRef window, MCPoint window_point, MCPoint& r_screen_point);
void MCPlatformMapPointFromScreenToWindow(MCPlatformWindowRef window, MCPoint screen_point, MCPoint& r_window_point);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformSurface *MCPlatformSurfaceRef;

bool MCPlatformSurfaceLockGraphics(MCPlatformSurfaceRef surface, MCRegionRef region, MCGContextRef& r_context);
void MCPlatformSurfaceUnlockGraphics(MCPlatformSurfaceRef surface);

bool MCPlatformSurfaceLockPixels(MCPlatformSurfaceRef surface, MCRegionRef region, MCGRaster& r_raster);
void MCPlatformSurfaceUnlockPixels(MCPlatformSurfaceRef surface);

bool MCPlatformSurfaceLockSystemContext(MCPlatformSurfaceRef surface, void*& r_context);
void MCPlatformSurfaceUnlockSystemContext(MCPlatformSurfaceRef surface);

bool MCPlatformSurfaceComposite(MCPlatformSurfaceRef surface, MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat alpha, MCGBlendMode blend);

////////////////////////////////////////////////////////////////////////////////

#endif
