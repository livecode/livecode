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

// The lower 21-bits hold a codepoint, the upper bits hold modifiers. Some
// codepoint values are specific to accelerators and represent non-printing
// keys.
typedef uint32_t MCPlatformAccelerator;
enum
{
	kMCPlatformAcceleratorNone,
	
	kMCPlatformAcceleratorTabKey = 0x0009,
	kMCPlatformAcceleratorBackspaceKey = 0x0008,
	kMCPlatformAcceleratorReturnKey = 0x000d,
	kMCPlatformAcceleratorEnterKey = 0x0003,
	kMCPlatformAcceleratorBackTabKey = 0x0019,
	kMCPlatformAcceleratorDeleteKey = 0x007f,
	
	kMCPlatformAcceleratorUpArrowKey = 0xF700,
	kMCPlatformAcceleratorDownArrowKey = 0xF701,
	kMCPlatformAcceleratorLeftArrowKey = 0xF702,
	kMCPlatformAcceleratorRightArrowKey = 0xF703,
	kMCPlatformAcceleratorF1Key = 0xF704,
	kMCPlatformAcceleratorF2Key = 0xF705,
	kMCPlatformAcceleratorF3Key = 0xF706,
	kMCPlatformAcceleratorF4Key = 0xF707,
	kMCPlatformAcceleratorF5Key = 0xF708,
	kMCPlatformAcceleratorF6Key = 0xF709,
	kMCPlatformAcceleratorF7Key = 0xF70A,
	kMCPlatformAcceleratorF8Key = 0xF70B,
	kMCPlatformAcceleratorF9Key = 0xF70C,
	kMCPlatformAcceleratorF10Key = 0xF70D,
	kMCPlatformAcceleratorF11Key = 0xF70E,
	kMCPlatformAcceleratorF12Key = 0xF70F,
	kMCPlatformAcceleratorF13Key = 0xF710,
	kMCPlatformAcceleratorF14Key = 0xF711,
	kMCPlatformAcceleratorF15Key = 0xF712,
	kMCPlatformAcceleratorF16Key = 0xF713,
	kMCPlatformAcceleratorF17Key = 0xF714,
	kMCPlatformAcceleratorF18Key = 0xF715,
	kMCPlatformAcceleratorF19Key = 0xF716,
	kMCPlatformAcceleratorF20Key = 0xF717,
	kMCPlatformAcceleratorF21Key = 0xF718,
	kMCPlatformAcceleratorF22Key = 0xF719,
	kMCPlatformAcceleratorF23Key = 0xF71A,
	kMCPlatformAcceleratorF24Key = 0xF71B,
	kMCPlatformAcceleratorF25Key = 0xF71C,
	kMCPlatformAcceleratorF26Key = 0xF71D,
	kMCPlatformAcceleratorF27Key = 0xF71E,
	kMCPlatformAcceleratorF28Key = 0xF71F,
	kMCPlatformAcceleratorF29Key = 0xF720,
	kMCPlatformAcceleratorF30Key = 0xF721,
	kMCPlatformAcceleratorF31Key = 0xF722,
	kMCPlatformAcceleratorF32Key = 0xF723,
	kMCPlatformAcceleratorF33Key = 0xF724,
	kMCPlatformAcceleratorF34Key = 0xF725,
	kMCPlatformAcceleratorF35Key = 0xF726,
	kMCPlatformAcceleratorInsertKey = 0xF727,
	//kMCPlatformAcceleratorDeleteKey = 0xF728,
	kMCPlatformAcceleratorHomeKey = 0xF729,
	kMCPlatformAcceleratorBeginKey = 0xF72A,
	kMCPlatformAcceleratorEndKey = 0xF72B,
	kMCPlatformAcceleratorPageUpKey = 0xF72C,
	kMCPlatformAcceleratorPageDownKey = 0xF72D,
	kMCPlatformAcceleratorPrintScreenKey = 0xF72E,
	kMCPlatformAcceleratorScrollLockKey = 0xF72F,
	kMCPlatformAcceleratorPauseKey = 0xF730,
	kMCPlatformAcceleratorSysReqKey = 0xF731,
	kMCPlatformAcceleratorBreakKey = 0xF732,
	kMCPlatformAcceleratorResetKey = 0xF733,
	kMCPlatformAcceleratorStopKey = 0xF734,
	kMCPlatformAcceleratorMenuKey = 0xF735,
	kMCPlatformAcceleratorUserKey = 0xF736,
	kMCPlatformAcceleratorSystemKey = 0xF737,
	kMCPlatformAcceleratorPrintKey = 0xF738,
	kMCPlatformAcceleratorClearLineKey = 0xF739,
	kMCPlatformAcceleratorClearDisplayKey = 0xF73A,
	kMCPlatformAcceleratorInsertLineKey = 0xF73B,
	kMCPlatformAcceleratorDeleteLineKey = 0xF73C,
	kMCPlatformAcceleratorInsertCharKey = 0xF73D,
	//kMCPlatformAcceleratorDeleteCharKey = 0xF73E,
	kMCPlatformAcceleratorPrevKey = 0xF73F,
	kMCPlatformAcceleratorNextKey = 0xF740,
	kMCPlatformAcceleratorSelectKey = 0xF741,
	kMCPlatformAcceleratorExecuteKey = 0xF742,
	kMCPlatformAcceleratorUndoKey = 0xF743,
	kMCPlatformAcceleratorRedoKey = 0xF744,
	kMCPlatformAcceleratorFindKey = 0xF745,
	kMCPlatformAcceleratorHelpKey = 0xF746,
	kMCPlatformAcceleratorModeSwitchKey = 0xF747,

	kMCPlatformAcceleratorKeyMask = 0x1ffff,
	
	kMCPlatformAcceleratorWithShift = 1 << 22,
	kMCPlatformAcceleratorWithControl = 1 << 23,
	kMCPlatformAcceleratorWithAlt = 1 << 24,
	kMCPlatformAcceleratorWithOption = kMCPlatformAcceleratorWithAlt,
	kMCPlatformAcceleratorWithMeta = 1 << 25,
	kMCPlatformAcceleratorWithCommand = kMCPlatformAcceleratorWithMeta,
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
bool MCPlatformGetMouseClick(uindex_t button, MCPoint& r_location);

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

void MCPlatformGetScreenCount(uindex_t& r_count);
void MCPlatformGetScreenPixelScale(uindex_t index, float& r_scale);
void MCPlatformGetScreenViewport(uindex_t index, MCRectangle& r_viewport);
void MCPlatformGetScreenWorkarea(uindex_t index, MCRectangle& r_workarea);

void MCPlatformScreenSnapshotOfUserArea(MCImageBitmap*& r_bitmap);
void MCPlatformScreenSnapshotOfWindow(uint32_t window_id, MCImageBitmap*& r_bitmap);
void MCPlatformScreenSnapshot(MCRectangle area, MCImageBitmap*& r_bitmap);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformLoadedFont *MCPlatformLoadedFontRef;

bool MCPlatformLoadFont(const char *utf8path, bool globally, MCPlatformLoadedFontRef& r_loaded_font);
bool MCPlatformUnloadFont(const char *utf8path, bool globally, MCPlatformLoadedFontRef loaded_font);

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

bool MCPlatformPopUpMenu(MCPlatformMenuRef menu, MCPlatformWindowRef window, MCPoint location, uindex_t item);

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

typedef class MCPlatformPasteboard *MCPlatformPasteboardRef;

typedef uint32_t MCPlatformAllowedDragOperations;

enum MCPlatformDragOperation
{
	kMCPlatformDragOperationNone,
	kMCPlatformDragOperationCopy,
	kMCPlatformDragOperationLink,
	kMCPlatformDragOperationMove,
	
	// COCOA-TODO: Add other drag operation types.
};

// The flavors expoted by the platform layer are currently only the ones which
// the LiveCode engine can handle on a platform-independent basis - the platform
// layer will do any internal conversions (for example, on Mac TIFF is a typical
// image format - the platform layer will recode as PNG for the engine).
enum MCPlatformPasteboardFlavor
{
	kMCPlatformPasteboardFlavorNone,
	
	kMCPlatformPasteboardFlavorUTF8,
	kMCPlatformPasteboardFlavorRTF,
	kMCPlatformPasteboardFlavorHTML,
	kMCPlatformPasteboardFlavorPNG,
	kMCPlatformPasteboardFlavorJPEG,
	kMCPlatformPasteboardFlavorGIF,
	kMCPlatformPasteboardFlavorFiles,
	
	// PLATFORM-TODO: This needs a better mechanism for extending recognised formats
	kMCPlatformPasteboardFlavorObjects,
	kMCPlatformPasteboardFlavorStyledText,
};

void MCPlatformPasteboardRetain(MCPlatformPasteboardRef pasteboard);
void MCPlatformPasteboardRelease(MCPlatformPasteboardRef pasteboard);

uindex_t MCPlatformPasteboardGetGeneration(MCPlatformPasteboardRef pasteboard);

bool MCPlatformPasteboardQuery(MCPlatformPasteboardRef pasteboard, MCPlatformPasteboardFlavor*& r_flavors, uindex_t& r_count);
bool MCPlatformPasteboardFetch(MCPlatformPasteboardRef pasteboard, MCPlatformPasteboardFlavor flavor, void*& r_bytes, uindex_t& r_byte_count);

void MCPlatformPasteboardClear(MCPlatformPasteboardRef pasteboard);
bool MCPlatformPasteboardStore(MCPlatformPasteboardRef pasteboard, MCPlatformPasteboardFlavor *flavor, uindex_t flavor_count, void *handle);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetDragboard(MCPlatformPasteboardRef& r_pasteboard);
void MCPlatformDoDragDrop(MCPlatformWindowRef window, MCPlatformAllowedDragOperations allowed_operations, MCImageBitmap *image, const MCPoint *image_loc, MCPlatformDragOperation& r_operation);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetClipboard(MCPlatformPasteboardRef& r_pasteboard);

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
