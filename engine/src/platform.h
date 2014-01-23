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

enum MCPlatformCursorImageSupport
{
	kMCPlatformCursorImageSupportMonochrome,
	kMCPlatformCursorImageSupportBilevel,
	kMCPlatformCursorImageSupportColor,
	kMCPlatformCursorImageSupportAlpha,
};

////////////////////////////////////////////////////////////////////////////////

// System properties are settings that can be queried from the system.
//
// TODO-REVIEW: Perhaps these would be better classed as metrics?
//   Or perhaps MCPlatformQuery(...)

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

typedef uint32_t MCPlatformKeyCode;
enum
{
	kMCPlatformKeyCodeUndefined		= 0x0000,
	
	kMCPlatformKeyCodeSpace			= 0x0020,
	kMCPlatformKeyCodeQuote			= 0x0027,
	kMCPlatformKeyCodeComma			= 0x002C,
	kMCPlatformKeyCodeMinus			= 0x002D,
	kMCPlatformKeyCodePeriod		= 0x002E,
	kMCPlatformKeyCodeSlash			= 0x002F,
	
	kMCPlatformKeyCode0				= 0x0030,
	kMCPlatformKeyCode1				= 0x0031,
	kMCPlatformKeyCode2				= 0x0032,
	kMCPlatformKeyCode3				= 0x0033,
	kMCPlatformKeyCode4				= 0x0034,
	kMCPlatformKeyCode5				= 0x0035,
	kMCPlatformKeyCode6				= 0x0036,
	kMCPlatformKeyCode7				= 0x0037,
	kMCPlatformKeyCode8				= 0x0038,
	kMCPlatformKeyCode9				= 0x0039,
	
	kMCPlatformKeyCodeSemicolon		= 0x003B,
	kMCPlatformKeyCodeEqual			= 0x003D,
	
	kMCPlatformKeyCodeA				= 0x0041,
	kMCPlatformKeyCodeB				= 0x0042,
	kMCPlatformKeyCodeC				= 0x0043,
	kMCPlatformKeyCodeD				= 0x0044,
	kMCPlatformKeyCodeE				= 0x0045,
	kMCPlatformKeyCodeF				= 0x0046,
	kMCPlatformKeyCodeG				= 0x0047,
	kMCPlatformKeyCodeH				= 0x0048,
	kMCPlatformKeyCodeI				= 0x0049,
	kMCPlatformKeyCodeJ				= 0x004A,
	kMCPlatformKeyCodeK				= 0x004B,
	kMCPlatformKeyCodeL				= 0x004C,
	kMCPlatformKeyCodeM				= 0x004D,
	kMCPlatformKeyCodeN				= 0x004E,
	kMCPlatformKeyCodeO				= 0x004F,
	kMCPlatformKeyCodeP				= 0x0050,
	kMCPlatformKeyCodeQ				= 0x0051,
	kMCPlatformKeyCodeR				= 0x0052,
	kMCPlatformKeyCodeS				= 0x0053,
	kMCPlatformKeyCodeT				= 0x0054,
	kMCPlatformKeyCodeU				= 0x0055,
	kMCPlatformKeyCodeV				= 0x0056,
	kMCPlatformKeyCodeW				= 0x0057,
	kMCPlatformKeyCodeX				= 0x0058,
	kMCPlatformKeyCodeY				= 0x0059,
	kMCPlatformKeyCodeZ				= 0x005A,
	
	kMCPlatformKeyCodeLeftBracket	= 0x005B,
	kMCPlatformKeyCodeBackslash		= 0x005C,
	kMCPlatformKeyCodeRightBracket	= 0x005D,
	
	kMCPlatformKeyCodeGrave			= 0x0060,
	
	
	kMCPlatformKeyCodeBackspace		= 0xff08,
	kMCPlatformKeyCodeTab			= 0xff09,
	kMCPlatformKeyCodeLinefeed		= 0xff0a,
	kMCPlatformKeyCodeClear			= 0xff0b,
	
	kMCPlatformKeyCodeReturn		= 0xff0d,
	
	kMCPlatformKeyCodePause			= 0xff13,
	kMCPlatformKeyCodeScrollLock	= 0xff14,
	kMCPlatformKeyCodeSysReq		= 0xff15,
	kMCPlatformKeyCodeEscape		= 0xff1b,
	
	kMCPlatformKeyCodeHome			= 0xff50,
	kMCPlatformKeyCodeLeft			= 0xff51,
	kMCPlatformKeyCodeUp			= 0xff52,
	kMCPlatformKeyCodeRight			= 0xff53,
	kMCPlatformKeyCodeDown			= 0xff54,
	kMCPlatformKeyCodePrevious		= 0xff55,
	kMCPlatformKeyCodeNext			= 0xff56,
	kMCPlatformKeyCodeEnd			= 0xff57,
	kMCPlatformKeyCodeBegin			= 0xff58, 
	
	kMCPlatformKeyCodeSelect		= 0xff60,
	kMCPlatformKeyCodePrint			= 0xff61,
	kMCPlatformKeyCodeExecute		= 0xff62,
	kMCPlatformKeyCodeInsert		= 0xff63,
	
	kMCPlatformKeyCodeUndo			= 0xff65,
	kMCPlatformKeyCodeRedo			= 0xff66,
	kMCPlatformKeyCodeMenu			= 0xff67,
	kMCPlatformKeyCodeFind			= 0xff68,
	kMCPlatformKeyCodeCancel		= 0xff69,
	kMCPlatformKeyCodeHelp			= 0xff6a,
	kMCPlatformKeyCodeBreak			= 0xff6b,
	
	kMCPlatformKeyCodeModeSwitch	= 0xff7e,
	kMCPlatformKeyCodeNumLock		= 0xff7f,
	
	kMCPlatformKeyCodeKeypadSpace	= 0xff80,
	kMCPlatformKeyCodeKeypadTab		= 0xff89,
	kMCPlatformKeyCodeKeypadEnter	= 0xff8D,
	kMCPlatformKeyCodeKeypadF1		= 0xff91,
	kMCPlatformKeyCodeKeypadF2		= 0xff92,
	kMCPlatformKeyCodeKeypadEqual	= 0xFFBD,
	kMCPlatformKeyCodeKeypadMultiply	= 0xFFAA,
	kMCPlatformKeyCodeKeypadAdd		= 0xFFAB,
	kMCPlatformKeyCodeKeypadSeparator	= 0xFFAC,
	kMCPlatformKeyCodeKeypadSubtract	= 0xFFAD,
	kMCPlatformKeyCodeKeypadDecimal	= 0xFFAE,
	kMCPlatformKeyCodeKeypadDivide	= 0xFFAF,
	kMCPlatformKeyCodeKeypad0		= 0xFFB0,
	kMCPlatformKeyCodeKeypad1		= 0xFFB1,
	kMCPlatformKeyCodeKeypad2		= 0xFFB2,
	kMCPlatformKeyCodeKeypad3		= 0xFFB3,
	kMCPlatformKeyCodeKeypad4		= 0xFFB4,
	kMCPlatformKeyCodeKeypad5		= 0xFFB5,
	kMCPlatformKeyCodeKeypad6		= 0xFFB6,
	kMCPlatformKeyCodeKeypad7		= 0xFFB7,
	kMCPlatformKeyCodeKeypad8		= 0xFFB8,
	kMCPlatformKeyCodeKeypad9		= 0xFFB9,
	
	kMCPlatformKeyCodeF1			= 0xFFBE,
	kMCPlatformKeyCodeF2			= 0xFFBF,
	kMCPlatformKeyCodeF3			= 0xFFC0,
	kMCPlatformKeyCodeF4			= 0xFFC1,
	kMCPlatformKeyCodeF5			= 0xFFC2,
	kMCPlatformKeyCodeF6			= 0xFFC3,
	kMCPlatformKeyCodeF7			= 0xFFC4,
	kMCPlatformKeyCodeF8			= 0xFFC5,
	kMCPlatformKeyCodeF9			= 0xFFC6,
	kMCPlatformKeyCodeF10			= 0xFFC7,
	kMCPlatformKeyCodeF11			= 0xFFC8,
	kMCPlatformKeyCodeF12			= 0xFFC9,
	kMCPlatformKeyCodeF13			= 0xFFCA,
	kMCPlatformKeyCodeF14			= 0xFFCB,
	kMCPlatformKeyCodeF15			= 0xFFCC,
	kMCPlatformKeyCodeF16			= 0xFFCD,
	kMCPlatformKeyCodeF17			= 0xFFCE,
	kMCPlatformKeyCodeF18			= 0xFFCF,
	kMCPlatformKeyCodeF19			= 0xFFD0,
	kMCPlatformKeyCodeF20			= 0xFFD1,
	kMCPlatformKeyCodeF21			= 0xFFD2,
	kMCPlatformKeyCodeF22			= 0xFFD3,
	kMCPlatformKeyCodeF23			= 0xFFD4,
	kMCPlatformKeyCodeF24			= 0xFFD5,
	kMCPlatformKeyCodeF25			= 0xFFD6,
	kMCPlatformKeyCodeF26			= 0xFFD7,
	kMCPlatformKeyCodeF27			= 0xFFD8,
	kMCPlatformKeyCodeF28			= 0xFFD9,
	kMCPlatformKeyCodeF29			= 0xFFDA,
	kMCPlatformKeyCodeF30			= 0xFFDB,
	kMCPlatformKeyCodeF31			= 0xFFDC,
	kMCPlatformKeyCodeF32			= 0xFFDD,
	kMCPlatformKeyCodeF33			= 0xFFDE,
	kMCPlatformKeyCodeF34			= 0xFFDF,
	kMCPlatformKeyCodeF35			= 0xFFE0,
	
	// COCOA-TODO: Complete platformkeycode list.
	
	kMCPlatformKeyCodeLeftShift		= 0xffe1,
	kMCPlatformKeyCodeRightShift	= 0xffe2,
	kMCPlatformKeyCodeLeftControl	= 0xffe3,
	kMCPlatformKeyCodeRightControl	= 0xffe4,
	kMCPlatformKeyCodeCapsLock		= 0xffe5,
	kMCPlatformKeyCodeShiftLock		= 0xffe6,
	kMCPlatformKeyCodeLeftMeta		= 0xffe7, // Command key on mac
	kMCPlatformKeyCodeLeftCommand	= kMCPlatformKeyCodeLeftMeta,
	kMCPlatformKeyCodeRightMeta		= 0xffe8, // Command key on mac
	kMCPlatformKeyCodeRightCommand	= kMCPlatformKeyCodeRightMeta,
	kMCPlatformKeyCodeLeftAlt		= 0xffe9, // Option key on mac
	kMCPlatformKeyCodeLeftOption	= kMCPlatformKeyCodeLeftAlt,
	kMCPlatformKeyCodeRightAlt		= 0xffea, // Option key on mac
	kMCPlatformKeyCodeRightOption	= kMCPlatformKeyCodeRightAlt,
	
	// COCOA-TODO: Map these appropriately.
	kMCPlatformKeyCodeVolumeUp		= 0xfffe,
	kMCPlatformKeyCodeVolumeDown	= 0xfffe,
	kMCPlatformKeyCodeMute			= 0xfffe,
	kMCPlatformKeyCodeISOSection	= 0xfffe,
	kMCPlatformKeyCodeJISYen		= 0xfffe,
	kMCPlatformKeyCodeJISUnderscore	= 0xfffe,
	kMCPlatformKeyCodeJISKeypadComma= 0xfffe,
	kMCPlatformKeyCodeJISEisu		= 0xfffe,
	kMCPlatformKeyCodeJISKana		= 0xfffe,
	kMCPlatformKeyCodeFunction		= 0xfffe,
	
	kMCPlatformKeyCodeDelete		= 0xffff,
};

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
	kMCPlatformMenuItemActionAbout,
	kMCPlatformMenuItemActionPreferences,
	
	kMCPlatformMenuItemActionUndo,
	kMCPlatformMenuItemActionRedo,
	kMCPlatformMenuItemActionCut,
	kMCPlatformMenuItemActionCopy,
	kMCPlatformMenuItemActionPaste,
	kMCPlatformMenuItemActionClear,
	kMCPlatformMenuItemActionSelectAll,
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

enum MCPlatformWindowEdge
{
	kMCPlatformWindowEdgeNone,
	
	KMCPlatformWindowEdgeTop,
	kMCPlatformWindowEdgeLeft,
	kMCPlatformWindowEdgeBottom,
	kMCPlatformWindowEdgeRight,
};

void MCPlatformCreateWindow(MCPlatformWindowRef& r_window);
void MCPlatformRetainWindow(MCPlatformWindowRef window);
void MCPlatformReleaseWindow(MCPlatformWindowRef window);

void MCPlatformInvalidateWindow(MCPlatformWindowRef window, MCRegionRef region);
void MCPlatformUpdateWindow(MCPlatformWindowRef window);

void MCPlatformShowWindow(MCPlatformWindowRef window);
void MCPlatformShowWindowAsSheet(MCPlatformWindowRef window, MCPlatformWindowRef parent_window);
void MCPlatformShowWindowAsDrawer(MCPlatformWindowRef window, MCPlatformWindowRef parent_window, MCPlatformWindowEdge edge);
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
