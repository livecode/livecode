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
struct MCColorSpaceInfo;

template<typename T> struct array_t
{
    T *ptr;
    uindex_t count;
};

template <typename T>
inline void MCPlatformArrayClear(array_t<T> &p_array)
{
	MCMemoryDeleteArray(p_array.ptr);
	p_array.count = 0;
	p_array.ptr = nil;
}

template <typename T>
inline bool MCPlatformArrayCopy(const array_t<T> &p_src, array_t<T> &p_dst)
{
	if (!MCMemoryAllocateCopy(p_src.ptr, p_src.count * sizeof(T), p_dst.ptr))
		return false;

	p_dst.count = p_src.count;
	return true;
}

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
	kMCPlatformPropertyTypeUInt16,
	kMCPlatformPropertyTypeInt32,
	kMCPlatformPropertyTypeUInt32,
	kMCPlatformPropertyTypeInt64,
	kMCPlatformPropertyTypeUInt64,
	kMCPlatformPropertyTypeFloat,
	kMCPlatformPropertyTypeDouble,
	kMCPlatformPropertyTypeRectangle,
	kMCPlatformPropertyTypeColor,
	
	kMCPlatformPropertyTypeNativeCString,
	kMCPlatformPropertyTypeMCString,
	
	kMCPlatformPropertyTypeWindowStyle,
	kMCPlatformPropertyTypeWindowMask,
	
	kMCPlatformPropertyTypeMenuRef,
	kMCPlatformPropertyTypeAccelerator,
	kMCPlatformPropertyTypeMenuItemHighlight,
	kMCPlatformPropertyTypeMenuItemAction,
	kMCPlatformPropertyTypeCursorImageSupport,
	
	kMCPlatformPropertyTypePlayerMediaTypes,
	kMCPlatformPropertyTypePlayerQTVRConstraints,
	
	kMCPlatformPropertyTypeCursorRef,
    
    kMCPlatformPropertyTypeUInt32Array,
	kMCPlatformPropertyTypeUInt64Array,
	
	kMCPlatformPropertyTypePointer,
    
    kMCPlatformPropertyType_Last,
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

typedef uint32_t MCPlatformModifiers;
enum
{
	kMCPlatformModifierShift = 1 << 0,
	kMCPlatformModifierControl = 1 << 1,
	kMCPlatformModifierAlt = 1 << 2,
	kMCPlatformModifierOption = kMCPlatformModifierAlt,
	kMCPlatformModifierMeta = 1 << 3,
	kMCPlatformModifierCommand = kMCPlatformModifierMeta,
	kMCPlatformModifierCapsLock = 1 << 4,
};

enum MCPlatformCursorImageSupport
{
	kMCPlatformCursorImageSupportMonochrome,
	kMCPlatformCursorImageSupportBilevel,
	kMCPlatformCursorImageSupportColor,
	kMCPlatformCursorImageSupportAlpha,
};

typedef uint32_t MCPlatformEventMask;
enum
{
	kMCPlatformEventMouseDown = 1 << 0,
	kMCPlatformEventMouseUp = 1 << 1,
	kMCPlatformEventKeyDown = 1 << 2,
	kMCPlatformEventKeyUp = 1 << 3,
};

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
	
	kMCPlatformKeyCodeISOSection	= 0x00A7,
	
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
	kMCPlatformKeyCodeKeypadEqual	= 0xFFBD,
	
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
	
	// COCOA-TODO: Do these key codes need to be mapped?
	kMCPlatformKeyCodeVolumeUp		= 0xfffe,
	kMCPlatformKeyCodeVolumeDown	= 0xfffe,
	kMCPlatformKeyCodeMute			= 0xfffe,
	kMCPlatformKeyCodeJISYen		= 0xfffe,
	kMCPlatformKeyCodeJISUnderscore	= 0xfffe,
	kMCPlatformKeyCodeJISKeypadComma= 0xfffe,
	kMCPlatformKeyCodeJISEisu		= 0xfffe,
	kMCPlatformKeyCodeJISKana		= 0xfffe,
	kMCPlatformKeyCodeFunction		= 0xfffe,
	
	kMCPlatformKeyCodeDelete		= 0xffff,
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
    
    kMCPlatformSystemPropertyVolume,

	kMCPlatformSystemPropertySystemAppearance,
};

void MCPlatformGetSystemProperty(MCPlatformSystemProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformSetSystemProperty(MCPlatformSystemProperty property, MCPlatformPropertyType type, void *value);

enum MCPlatformSystemAppearance
{
	kMCPlatformSystemAppearanceLight = 0,
	kMCPlatformSystemAppearanceDark = 1,
};

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCPlatformPreWaitForEventCallback)(double duration, bool blocking);
typedef bool (*MCPlatformPostWaitForEventCallback)(bool found_event);

// Set the callbacks to call before and after waitforevent has been executed.
void MCPlatformSetWaitForEventCallbacks(MCPlatformPreWaitForEventCallback p_pre, MCPlatformPostWaitForEventCallback p_post);

// Break the current WaitForEvent which is progress.
void MCPlatformBreakWait(void);

// Wait for any event for at most duration seconds. If blocking is true then
// no events which cause a dispatch should be processed. If an event is processed
// during duration, true is returned; otherwise false is.
bool MCPlatformWaitForEvent(double duration, bool blocking);

// Disables abort key checks
void MCPlatformDisableAbortKey(void);

// Enables abort key checks
void MCPlatformEnableAbortKey(void);

// Return true if the abort key has been pressed since the last check.
bool MCPlatformGetAbortKeyPressed(void);

// Get the current (right now!) state of the mouse button.
bool MCPlatformGetMouseButtonState(uindex_t button);

// Returns an array of all the currently pressed keys.
bool MCPlatformGetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count);

// Get the current (right now!) state of the modifier keys.
MCPlatformModifiers MCPlatformGetModifiersState(void);

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

bool MCPlatformGetWindowWithId(uint32_t p_window_id, MCPlatformWindowRef& r_window);

// Return the 'time' of the last event.
uint32_t MCPlatformGetEventTime(void);

// Flush events of the specified types in mask.
void MCPlatformFlushEvents(MCPlatformEventMask mask);

// Produce a system beep.
void MCPlatformBeep(void);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetScreenCount(uindex_t& r_count);
void MCPlatformGetScreenPixelScale(uindex_t index, float& r_scale);
void MCPlatformGetScreenViewport(uindex_t index, MCRectangle& r_viewport);
void MCPlatformGetScreenWorkarea(uindex_t index, MCRectangle& r_workarea);

void MCPlatformScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap);
void MCPlatformScreenSnapshot(MCRectangle area, MCPoint *p_size, MCImageBitmap*& r_bitmap);
void MCPlatformScreenSnapshotOfWindow(uint32_t window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap);
void MCPlatformScreenSnapshotOfWindowArea(uint32_t window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformLoadedFont *MCPlatformLoadedFontRef;

bool MCPlatformLoadFont(MCStringRef p_path, bool globally, MCPlatformLoadedFontRef& r_loaded_font);
bool MCPlatformUnloadFont(MCStringRef p_path, bool globally, MCPlatformLoadedFontRef loaded_font);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformColorTransform *MCPlatformColorTransformRef;

void MCPlatformCreateColorTransform(const MCColorSpaceInfo& info, MCPlatformColorTransformRef& r_transform);
void MCPlatformRetainColorTransform(MCPlatformColorTransformRef transform);
void MCPlatformReleaseColorTransform(MCPlatformColorTransformRef transform);

bool MCPlatformApplyColorTransform(MCPlatformColorTransformRef transform, MCImageBitmap *image);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformWindowMask *MCPlatformWindowMaskRef;

void MCPlatformWindowMaskCreate(int32_t width, int32_t height, int32_t stride, void *bits, MCPlatformWindowMaskRef& r_mask);
void MCPlatformWindowMaskCreateWithAlphaAndRelease(int32_t width, int32_t height, int32_t stride, void *bits, MCPlatformWindowMaskRef& r_mask);
void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef mask);
void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef mask);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformMenu *MCPlatformMenuRef;

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

void MCPlatformSetMenuTitle(MCPlatformMenuRef menu, MCStringRef title);

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

void MCPlatformSetIconMenu(MCPlatformMenuRef menu);

//////////

void MCPlatformShowMenubar(void);
void MCPlatformHideMenubar(void);

void MCPlatformSetMenubar(MCPlatformMenuRef menu);
void MCPlatformGetMenubar(MCPlatformMenuRef menu);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformCursor *MCPlatformCursorRef;

// SN-2015-06-16: [[ Bug 14056 ]] Add hidden cursor as part of the standard ones
enum MCPlatformStandardCursor
{
	kMCPlatformStandardCursorUnknown,
    
    kMCPlatformStandardCursorNone,
	kMCPlatformStandardCursorArrow,
	kMCPlatformStandardCursorWatch,
	kMCPlatformStandardCursorCross,
	kMCPlatformStandardCursorIBeam,
};

void MCPlatformCreateStandardCursor(MCPlatformStandardCursor standard_cusor, MCPlatformCursorRef& r_cursor);
void MCPlatformCreateCustomCursor(MCImageBitmap *image, MCPoint hot_spot, MCPlatformCursorRef& r_cursor);
void MCPlatformRetainCursor(MCPlatformCursorRef cursor);
void MCPlatformReleaseCursor(MCPlatformCursorRef cursor);

void MCPlatformSetCursor(MCPlatformCursorRef cursor);
void MCPlatformHideCursorUntilMouseMoves(void);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformPasteboard *MCPlatformPasteboardRef;

typedef uint32_t MCPlatformAllowedDragOperations;

enum MCPlatformDragOperation
{
	kMCPlatformDragOperationNone = 0,
	kMCPlatformDragOperationCopy = 1 << 0,
	kMCPlatformDragOperationLink = 1 << 1,
	kMCPlatformDragOperationMove = 1 << 2,
	
	// COCOA-TODO: Add other drag operation types.
};

////////////////////////////////////////////////////////////////////////////////

void MCPlatformDoDragDrop(MCPlatformWindowRef window, MCPlatformAllowedDragOperations allowed_operations, MCImageBitmap *image, const MCPoint *image_loc, MCPlatformDragOperation& r_operation);

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

bool MCPlatformIsWindowVisible(MCPlatformWindowRef window);

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
	kMCPlatformWindowPropertyIsOpaque,
	
	kMCPlatformWindowPropertyHasTitleWidget,
	kMCPlatformWindowPropertyHasCloseWidget,
	kMCPlatformWindowPropertyHasCollapseWidget,
	kMCPlatformWindowPropertyHasZoomWidget,
	kMCPlatformWindowPropertyHasSizeWidget,
	
	kMCPlatformWindowPropertyHasShadow,
	
	kMCPlatformWindowPropertyHasModifiedMark,

	kMCPlatformWindowPropertyUseLiveResizing,
	
	kMCPlatformWindowPropertySystemId,
	kMCPlatformWindowPropertySystemHandle,
	
	kMCPlatformWindowPropertyCursor,
    
    kMCPlatformWindowPropertyHideOnSuspend,
    
    kMCPlatformWindowPropertyIgnoreMouseEvents,
    
    kMCPlatformWindowPropertyDocumentFilename,
};

void MCPlatformSetWindowProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
void MCPlatformGetWindowProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, MCPlatformPropertyType type, void *value);

void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, bool value);
void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef window, MCPlatformWindowProperty property, float value);

void MCPlatformMapPointFromWindowToScreen(MCPlatformWindowRef window, MCPoint window_point, MCPoint& r_screen_point);
void MCPlatformMapPointFromScreenToWindow(MCPlatformWindowRef window, MCPoint screen_point, MCPoint& r_window_point);

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformTextInputAction
{
	kMCPlatformTextInputActionCapitalizeWord,
	kMCPlatformTextInputActionChangeCaseOfLetter,
	kMCPlatformTextInputActionDeleteBackward,
	kMCPlatformTextInputActionDeleteBackwardByDecomposingPreviousCharacter,
	kMCPlatformTextInputActionDeleteForward,
	kMCPlatformTextInputActionDeleteToBeginningOfLine,
	kMCPlatformTextInputActionDeleteToBeginningOfParagraph,
	kMCPlatformTextInputActionDeleteToEndOfLine,
	kMCPlatformTextInputActionDeleteToEndOfParagraph,
	kMCPlatformTextInputActionDeleteWordBackward,
	kMCPlatformTextInputActionDeleteWordForward,
	kMCPlatformTextInputActionInsertBacktab,
	kMCPlatformTextInputActionInsertContainerBreak,
	kMCPlatformTextInputActionInsertLineBreak,
	kMCPlatformTextInputActionInsertNewline,
	kMCPlatformTextInputActionInsertParagraphSeparator,
	kMCPlatformTextInputActionInsertTab,
	kMCPlatformTextInputActionLowercaseWord,
	kMCPlatformTextInputActionMoveBackward,
	kMCPlatformTextInputActionMoveBackwardAndModifySelection,
	kMCPlatformTextInputActionMoveParagraphForwardAndModifySelection,
	kMCPlatformTextInputActionMoveParagraphBackwardAndModifySelection,
	kMCPlatformTextInputActionMoveToBeginningOfDocumentAndModfySelection,
	kMCPlatformTextInputActionMoveToEndOfDocumentAndModfySelection,
	kMCPlatformTextInputActionMoveToBeginningOfLineAndModfySelection,
	kMCPlatformTextInputActionMoveToEndOfLineAndModfySelection,
	kMCPlatformTextInputActionMoveToBeginningOfParagraphAndModfySelection,
	kMCPlatformTextInputActionMoveToEndOfParagraphAndModfySelection,
	kMCPlatformTextInputActionMoveToLeftEndOfLine,
	kMCPlatformTextInputActionMoveToLeftEndOfLineAndModfySelection,
	kMCPlatformTextInputActionMoveToRightEndOfLine,
	kMCPlatformTextInputActionMoveToRightEndOfLineAndModfySelection,
	kMCPlatformTextInputActionMoveDown,
	kMCPlatformTextInputActionMoveDownAndModifySelection,
	kMCPlatformTextInputActionMoveForward,
	kMCPlatformTextInputActionMoveForwardAndModifySelection,
	kMCPlatformTextInputActionMoveLeft,
	kMCPlatformTextInputActionMoveLeftAndModifySelection,
	kMCPlatformTextInputActionMoveRight,
	kMCPlatformTextInputActionMoveRightAndModifySelection,
	kMCPlatformTextInputActionMoveToBeginningOfDocument,
	kMCPlatformTextInputActionMoveToBeginningOfLine,
	kMCPlatformTextInputActionMoveToBeginningOfParagraph,
	kMCPlatformTextInputActionMoveToEndOfDocument,
	kMCPlatformTextInputActionMoveToEndOfLine,
	kMCPlatformTextInputActionMoveToEndOfParagraph,
	kMCPlatformTextInputActionMoveUp,
	kMCPlatformTextInputActionMoveUpAndModifySelection,
	kMCPlatformTextInputActionMoveWordBackward,
	kMCPlatformTextInputActionMoveWordBackwardAndModifySelection,
	kMCPlatformTextInputActionMoveWordForward,
	kMCPlatformTextInputActionMoveWordForwardAndModifySelection,
	kMCPlatformTextInputActionMoveWordLeft,
	kMCPlatformTextInputActionMoveWordLeftAndModifySelection,
	kMCPlatformTextInputActionMoveWordRight,
	kMCPlatformTextInputActionMoveWordRightAndModifySelection,
	kMCPlatformTextInputActionPageUp,
	kMCPlatformTextInputActionPageUpAndModifySelection,
	kMCPlatformTextInputActionPageDown,
	kMCPlatformTextInputActionPageDownAndModifySelection,
	kMCPlatformTextInputActionScrollToBeginningOfDocument,
	kMCPlatformTextInputActionScrollToEndOfDocument,
	kMCPlatformTextInputActionScrollLineUp,
	kMCPlatformTextInputActionScrollLineDown,
	kMCPlatformTextInputActionScrollPageUp,
	kMCPlatformTextInputActionScrollPageDown,
	kMCPlatformTextInputActionSelectAll,
	kMCPlatformTextInputActionSelectLine,
	kMCPlatformTextInputActionSelectParagraph,
	kMCPlatformTextInputActionSelectWord,
	kMCPlatformTextInputActionTranspose,
	kMCPlatformTextInputActionTransposeWords,
	kMCPlatformTextInputActionUppercaseWord,
	kMCPlatformTextInputActionYank,
	kMCPlatformTextInputActionCut,
	kMCPlatformTextInputActionCopy,
	kMCPlatformTextInputActionPaste,
	kMCPlatformTextInputActionUndo,
	kMCPlatformTextInputActionRedo,
	kMCPlatformTextInputActionDelete,
};

void MCPlatformConfigureTextInputInWindow(MCPlatformWindowRef window, bool activate);
void MCPlatformResetTextInputInWindow(MCPlatformWindowRef window);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformConfigureBackdrop(MCPlatformWindowRef backdrop_window);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformSurface *MCPlatformSurfaceRef;

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to match new platform surface API.
bool MCPlatformSurfaceLockGraphics(MCPlatformSurfaceRef surface, MCGIntegerRectangle region, MCGContextRef& r_context, MCGRaster& r_raster);
void MCPlatformSurfaceUnlockGraphics(MCPlatformSurfaceRef surface, MCGIntegerRectangle region, MCGContextRef p_context, MCGRaster& p_raster);

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to match new platform surface API.
// IM-2014-08-26: [[ Bug 13261 ]] Return the actual locked area covered by the pixels
bool MCPlatformSurfaceLockPixels(MCPlatformSurfaceRef surface, MCGIntegerRectangle region, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area);
void MCPlatformSurfaceUnlockPixels(MCPlatformSurfaceRef surface, MCGIntegerRectangle region, MCGRaster& p_raster);

bool MCPlatformSurfaceLockSystemContext(MCPlatformSurfaceRef surface, void*& r_context);
void MCPlatformSurfaceUnlockSystemContext(MCPlatformSurfaceRef surface);

MCGFloat MCPlatformSurfaceGetBackingScaleFactor(MCPlatformSurfaceRef surface);

bool MCPlatformSurfaceComposite(MCPlatformSurfaceRef surface, MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat alpha, MCGBlendMode blend);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformPrintSession *MCPlatformPrintSessionRef;

enum MCPlatformPrintDialogResult
{
	kMCPlatformPrintDialogResultContinue,
	kMCPlatformPrintDialogResultError,
	kMCPlatformPrintDialogResultSuccess,
	kMCPlatformPrintDialogResultCancel
};

void MCPlatformBeginPrintSettingsDialog(MCPlatformWindowRef owner, void *session, void *settings, void *page_format);
void MCPlatformBeginPageSetupDialog(MCPlatformWindowRef owner, void *session, void *settings, void *page_format);
MCPlatformPrintDialogResult MCPlatformEndPrintDialog(void);

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformDialogResult
{
	kMCPlatformDialogResultContinue,
	kMCPlatformDialogResultError,
	kMCPlatformDialogResultSuccess,
	kMCPlatformDialogResultCancel,
};

enum MCPlatformFileDialogKind
{
	kMCPlatformFileDialogKindSave,
	kMCPlatformFileDialogKindOpen,
	kMCPlatformFileDialogKindOpenMultiple,
    kMCPlatformFileDialogKindFolder,
};

void MCPlatformBeginFolderOrFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial, MCStringRef *p_types = nullptr, uint4 p_type_count = 0);
MCPlatformDialogResult MCPlatformEndFolderDialog(MCStringRef & r_selected_folder);


MCPlatformDialogResult MCPlatformEndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef& r_paths, MCStringRef& r_type);

void MCPlatformBeginColorDialog(MCStringRef p_title, const MCColor& p_color);
MCPlatformDialogResult MCPlatformEndColorDialog(MCColor& r_new_color);

////////////////////////////////////////////////////////////////////////////////

typedef class MCPlatformPlayer *MCPlatformPlayerRef;

enum MCPlatformPlayerProperty
{
	kMCPlatformPlayerPropertyURL,
	kMCPlatformPlayerPropertyFilename,
    kMCPlatformPlayerPropertyInvalidFilename,
	
	kMCPlatformPlayerPropertyOffscreen,
	kMCPlatformPlayerPropertyRect,
	kMCPlatformPlayerPropertyMovieRect,
	kMCPlatformPlayerPropertyVisible,
	kMCPlatformPlayerPropertyMediaTypes,
	
	kMCPlatformPlayerPropertyDuration,
	kMCPlatformPlayerPropertyTimescale,
	kMCPlatformPlayerPropertyCurrentTime,
	kMCPlatformPlayerPropertyStartTime,
	kMCPlatformPlayerPropertyFinishTime,
	kMCPlatformPlayerPropertyPlayRate,
	kMCPlatformPlayerPropertyVolume,
    kMCPlatformPlayerPropertyMarkers,
    kMCPlatformPlayerPropertyLoadedTime,
	
    kMCPlatformPlayerPropertyLeftBalance,
    kMCPlatformPlayerPropertyRightBalance,
    kMCPlatformPlayerPropertyPan,
    
    kMCPlatformPlayerPropertyShowSelection,
	kMCPlatformPlayerPropertyOnlyPlaySelection,
	
	kMCPlatformPlayerPropertyLoop,
    kMCPlatformPlayerPropertyMirrored,
	kMCPlatformPlayerPropertyScalefactor,
    	
	kMCPlatformPlayerPropertyQTVRNode,
	kMCPlatformPlayerPropertyQTVRPan,
	kMCPlatformPlayerPropertyQTVRTilt,
	kMCPlatformPlayerPropertyQTVRZoom,
	kMCPlatformPlayerPropertyQTVRConstraints,
};

typedef uint32_t MCPlatformPlayerMediaTypes;
enum MCPlatformPlayerMediaType
{
	kMCPlatformPlayerMediaTypeVideoBit,
	kMCPlatformPlayerMediaTypeAudioBit,
	kMCPlatformPlayerMediaTypeTextBit,
	kMCPlatformPlayerMediaTypeQTVRBit,
	kMCPlatformPlayerMediaTypeSpriteBit,
	kMCPlatformPlayerMediaTypeFlashBit,

	kMCPlatformPlayerMediaTypeVideo = 1 << kMCPlatformPlayerMediaTypeVideoBit,
	kMCPlatformPlayerMediaTypeAudio = 1 << kMCPlatformPlayerMediaTypeAudioBit,
	kMCPlatformPlayerMediaTypeText = 1 << kMCPlatformPlayerMediaTypeTextBit,
	kMCPlatformPlayerMediaTypeQTVR = 1 << kMCPlatformPlayerMediaTypeQTVRBit,
	kMCPlatformPlayerMediaTypeSprite = 1 << kMCPlatformPlayerMediaTypeSpriteBit,
	kMCPlatformPlayerMediaTypeFlash = 1 << kMCPlatformPlayerMediaTypeFlashBit,
};

enum MCPlatformPlayerTrackProperty
{
	kMCPlatformPlayerTrackPropertyId,
	kMCPlatformPlayerTrackPropertyMediaTypeName,
	kMCPlatformPlayerTrackPropertyOffset,
	kMCPlatformPlayerTrackPropertyDuration,
	kMCPlatformPlayerTrackPropertyEnabled,
};

enum MCPlatformPlayerNodeProperty
{
};

enum MCPlatformPlayerHotSpotProperty
{
};

// SN-2014-06-25 [[ PlatformPlayer ]]
// MCPlatformPlayerQTVRConstraints must follow the definition of MCMultimediaQTVRConstraints
struct MCPlatformPlayerQTVRConstraints
{
	double x_min, x_max;
	double y_min, y_max;
	double z_min, z_max;
};

typedef uint64_t MCPlatformPlayerDuration;
typedef array_t<MCPlatformPlayerDuration> MCPlatformPlayerDurationArray;
#define kMCPlatformPropertyTypePlayerDuration kMCPlatformPropertyTypeUInt64
#define kMCPlatformPropertyTypePlayerDurationArray kMCPlatformPropertyTypeUInt64Array
#define MCPlatformPlayerDurationMax UINT64_MAX

void MCPlatformCreatePlayer(bool dontuseqt, MCPlatformPlayerRef& r_player);

void MCPlatformPlayerRetain(MCPlatformPlayerRef player);
void MCPlatformPlayerRelease(MCPlatformPlayerRef player);

void *MCPlatformPlayerGetNativeView(MCPlatformPlayerRef player);
bool MCPlatformPlayerSetNativeParentView(MCPlatformPlayerRef p_player, void *p_parent_view);

bool MCPlatformPlayerIsPlaying(MCPlatformPlayerRef player);

void MCPlatformStepPlayer(MCPlatformPlayerRef player, int amount);
void MCPlatformStartPlayer(MCPlatformPlayerRef player, double rate);
//void MCPlatformFastPlayer(MCPlatformPlayerRef player, Boolean forward);
//void MCPlatformFastForwardPlayer(MCPlatformPlayerRef player);
//void MCPlatformFastBackPlayer(MCPlatformPlayerRef player);
void MCPlatformStopPlayer(MCPlatformPlayerRef player);

bool MCPlatformLockPlayerBitmap(MCPlatformPlayerRef player, const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap);
void MCPlatformUnlockPlayerBitmap(MCPlatformPlayerRef player, MCImageBitmap *bitmap);

void MCPlatformSetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformGetPlayerProperty(MCPlatformPlayerRef player, MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);

void MCPlatformCountPlayerTracks(MCPlatformPlayerRef player, uindex_t& r_track_count);
void MCPlatformGetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformSetPlayerTrackProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
bool MCPlatformFindPlayerTrackWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index);

void MCPlatformCountPlayerNodes(MCPlatformPlayerRef player, uindex_t& r_node_count);
void MCPlatformGetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformSetPlayerNodeProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerNodeProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformFindPlayerNodeWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index);

void MCPlatformCountPlayerHotSpots(MCPlatformPlayerRef player, uindex_t& r_node_count);
void MCPlatformGetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformSetPlayerHotSpotProperty(MCPlatformPlayerRef player, uindex_t index, MCPlatformPlayerHotSpotProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformFindPlayerHotSpotWithId(MCPlatformPlayerRef player, uint32_t id, uindex_t& r_index);

////////////////////////////////////////////////////////////////////////////////

typedef struct MCPlatformScriptEnvironment *MCPlatformScriptEnvironmentRef;

typedef char *(*MCPlatformScriptEnvironmentCallback)(const char * const *arguments, uindex_t argument_count);

// SN-2014-07-23: [[ Bug 12907 ]]
//  Update as well MCSreenDC::createscriptenvironment (and callees)
void MCPlatformScriptEnvironmentCreate(MCStringRef language, MCPlatformScriptEnvironmentRef& r_env);
void MCPlatformScriptEnvironmentRetain(MCPlatformScriptEnvironmentRef env);
void MCPlatformScriptEnvironmentRelease(MCPlatformScriptEnvironmentRef env);
bool MCPlatformScriptEnvironmentDefine(MCPlatformScriptEnvironmentRef env, const char *function, MCPlatformScriptEnvironmentCallback callback);
void MCPlatformScriptEnvironmentRun(MCPlatformScriptEnvironmentRef env, MCStringRef script, MCStringRef& r_result);
void MCPlatformScriptEnvironmentCall(MCPlatformScriptEnvironmentRef env, const char *method, const char **arguments, uindex_t argument_count, char*& r_result);

////////////////////////////////////////////////////////////////////////////////

typedef struct MCPlatformSound *MCPlatformSoundRef;

enum MCPlatformSoundProperty
{
    kMCPlatformSoundPropertyVolume,
    kMCPlatformSoundPropertyLooping,
    kMCPlatformSoundPropertyDuration,
};

void MCPlatformSoundCreateWithData(const void *data, size_t data_size, MCPlatformSoundRef& r_sound);

void MCPlatformSoundRetain(MCPlatformSoundRef sound);
void MCPlatformSoundRelease(MCPlatformSoundRef sound);

bool MCPlatformSoundIsPlaying(MCPlatformSoundRef sound);

void MCPlatformSoundPlay(MCPlatformSoundRef sound);
void MCPlatformSoundPause(MCPlatformSoundRef sound);
void MCPlatformSoundResume(MCPlatformSoundRef sound);
void MCPlatformSoundStop(MCPlatformSoundRef sound);

void MCPlatformSoundSetProperty(MCPlatformSoundRef sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value);
void MCPlatformSoundGetProperty(MCPlatformSoundRef sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value);

////////////////////////////////////////////////////////////////////////////////

// Implementation guidence:
//
// This is basic abstraction of the sound recording functionality which the
// engine currently has.
//
// The platform sound recorder opaque type should be a procedural wrapper around
// a class which implements an abstract interface (just like the PlatformPlayer).
//
// Initially we need an implementation using the QT Sequence Grabber which needs
// to use the SGAudioMediaType (essentially this means that the code in the engine
// already for this is not usable - its heavily tied to SoundMediaType which
// crashes on modern Mac's).
//
// There is a sample code project 'WhackedTV' which should be useful to base the
// implementation on. This project goes a bit further than we need though - our
// dialog only needs to be the QT one provided by SCRequestImageSettings so that
// should hopefully simplify things.
//
// The SoundRecorder object should hold all the necessary system state to operate
// and whilst recording should ensure that things are idled appropriately using
// a Cocoa timer rather than requiring the engine to call an idle-proc (which is
// what is currently required).
//

typedef class MCPlatformSoundRecorder *MCPlatformSoundRecorderRef;

enum MCPlatformSoundRecorderProperty
{
	kMCPlatformSoundRecorderPropertyInput,
	kMCPlatformSoundRecorderPropertySampleRate,
    kMCPlatformSoundRecorderPropertySampleBitCount,
	kMCPlatformSoundRecorderPropertyChannelCount,
    kMCPlatformSoundRecorderPropertyCompressionType,
	kMCPlatformSoundRecorderPropertyExtraInfo,
};


struct MCPlatformSoundRecorderConfiguration
{
    // The input to use for sound recording - this should be an id as returned by ListInputs.
    unsigned int input;
    // The sample rate for recording.
    double sample_rate;
    // The number of bits per sample.
    unsigned int sample_bit_count;
    // The number of channels.
    unsigned int channel_count;
    // The compressor to use - this should be an id as returned by ListCompressors.
    unsigned int compression_type;
    // Any extra info specific to a compressor - this is an opaque sequence of bytes. If nil
    // when setting configuration, per-compressor defaults should be used.
    uint8_t *extra_info;
    size_t extra_info_size;
};

typedef bool (*MCPlatformSoundRecorderListInputsCallback)(void *context, unsigned int input_id, const char *label);
typedef bool (*MCPlatformSoundRecorderListCompressorsCallback)(void *context, unsigned int compressor_id, const char *label);
typedef bool (*MCPlatformSoundRecorderListFormatsCallback)(void *context, intenum_t format_id, MCStringRef label);

void MCPlatformSoundRecorderCreate(MCPlatformSoundRecorderRef& r_recorder);

void MCPlatformSoundRecorderRetain(MCPlatformSoundRecorderRef recorder);
void MCPlatformSoundRecorderRelease(MCPlatformSoundRecorderRef recorder);

// Return true if the recorder is recording.
bool MCPlatformSoundRecorderIsRecording(MCPlatformSoundRecorderRef recorder);

// Return the current volume of the recorded input - if not recording, return 0.
double MCPlatformSoundRecorderGetLoudness(MCPlatformSoundRecorderRef recorder);

// Start sound recording to the given file. If the sound recorder is already recording then
// the existing recording should be cancelled (stop and delete output file).
bool MCPlatformSoundRecorderStart(MCPlatformSoundRecorderRef recorder, MCStringRef filename);
// Stop the sound recording.
void MCPlatformSoundRecorderStop(MCPlatformSoundRecorderRef recorder);

void MCPlatformSoundRecorderPause(MCPlatformSoundRecorderRef recorder);
void MCPlatformSoundRecorderResume(MCPlatformSoundRecorderRef recorder);

// Call callback for each possible input device available - if the callback returns 'false' at any point
// enumeration is cancelled, and the false will be returned.
bool MCPlatformSoundRecorderListInputs(MCPlatformSoundRecorderRef recorder, MCPlatformSoundRecorderListInputsCallback callback, void *context);
// Call callback for each possible compressor available - if the callback returns 'false' at any point
// enumeration is cancelled, and the false will be returned.
bool MCPlatformSoundRecorderListCompressors(MCPlatformSoundRecorderRef recorder, MCPlatformSoundRecorderListCompressorsCallback callback, void *context);
// Call callback for each possible output format available - if the callback returns 'false' at any point
// enumeration is cancelled, and the false will be returned.
bool MCPlatformSoundRecorderListFormats(MCPlatformSoundRecorderRef recorder, MCPlatformSoundRecorderListFormatsCallback callback, void *context);

// Get the current sound recording configuration. The caller is responsible for freeing 'extra_info'.
void MCPlatformSoundRecorderGetConfiguration(MCPlatformSoundRecorderRef recorder, MCPlatformSoundRecorderConfiguration& r_config);
// Set the current sound recording configuration.
void MCPlatformSoundRecorderSetConfiguration(MCPlatformSoundRecorderRef recorder, const MCPlatformSoundRecorderConfiguration& config);

// Popup a configuration dialog for the compressors. If the dialog is not cancelled the
// sound recorders config will have been updated.
void MCPlatformSoundRecorderBeginConfigurationDialog(MCPlatformSoundRecorderRef recorder);
// End the configuration dialog.
MCPlatformDialogResult MCPlatformSoundRecorderEndConfigurationDialog(MCPlatformSoundRecorderRef recorder);

////////////////////////////////////////////////////////////////////////////////

void MCPlatformSwitchFocusToView(MCPlatformWindowRef window, uint32_t id);

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformControlType
{
    kMCPlatformControlTypeGeneric = 0,  // Global theming (i.e the theme inherited by all controls)
    kMCPlatformControlTypeButton,       // Buttons not covered more specifically
    kMCPlatformControlTypeCheckbox,     // On-off tick box
    kMCPlatformControlTypeRadioButton,  // One-of-many selection button
    kMCPlatformControlTypeTabButton,    // Selector buttons on a tab control
    kMCPlatformControlTypeTabPane,      // Pane area of a tab control
    kMCPlatformControlTypeLabel,        // Non-modifiable text
    kMCPlatformControlTypeInputField,   // Standard text entry box
    kMCPlatformControlTypeList,         // Itemised text box
    kMCPlatformControlTypeMenu,         // Menus not covered more specifically
    kMCPlatformControlTypeMenuItem,     // Item within a menu
    kMCPlatformControlTypeOptionMenu,   // Select a single item
    kMCPlatformControlTypePulldownMenu, // Menu as found in menubars
    kMCPlatformControlTypeComboBox,     // Input field/option menu combination
    kMCPlatformControlTypePopupMenu,    // Menu as appears when right-clicking
    kMCPlatformControlTypeProgressBar,  // Visual indicator of progress
    kMCPlatformControlTypeRichText,     // Text editing with user formatting control
    kMCPlatformControlTypeScrollBar,    // For scrolling, apparently
    kMCPlatformControlTypeSlider,       // Selects a value between two extremes
    kMCPlatformControlTypeSpinArrows,   // Up-down arrows for value adjustment
    kMCPlatformControlTypeTooltip,      // Tooltip popups
    kMCPlatformControlTypeWindow,       // Windows can have theming props too
    kMCPlatformControlTypeMessageBox    // Pop-up alert dialogue
};

typedef unsigned int MCPlatformControlState;
enum
{
    kMCPlatformControlStateDisabled         = (1<<0),   // Control is disabled
    kMCPlatformControlStateOn               = (1<<1),   // Control is "on" (e.g. ticked checkbox)
    kMCPlatformControlStateMouseOver        = (1<<2),   // Mouse is within the control's bounds
    kMCPlatformControlStateMouseFocus       = (1<<3),   // Control has mouse focus
    kMCPlatformControlStatePressed          = (1<<5),   // Mouse is down (and this control has mouse focus)
    kMCPlatformControlStateDefault          = (1<<6),   // Control is the default action
    kMCPlatformControlStateReadOnly         = (1<<7),   // Control is not modifiable
    kMCPlatformControlStateSelected         = (1<<8),   // Control is selected
    kMCPlatformControlStateWindowActive     = (1<<9),   // Control is in focused window
    
    kMCPlatformControlStateCompatibility    = (1<<31),   // Use backwards-compatible theming
    
    kMCPlatformControlStateNormal           = 0
};

enum MCPlatformControlPart
{
    kMCPlatformControlPartNone              // No sub-part of the control
};

enum MCPlatformThemeProperty
{
    // These properties may vary by control type
    kMCPlatformThemePropertyTextFont,               // [Font]       Font for text drawing
    kMCPlatformThemePropertyTextColor,              // [Color]      Text color
    kMCPlatformThemePropertyTextSize,               // [Integer]    Text point size
    kMCPlatformThemePropertyBackgroundColor,        // [Color]      Background color
    kMCPlatformThemePropertyAlpha,                  // [Integer]    Whole-control transparency
    kMCPlatformThemePropertyShadowColor,            // [Color]      Color for control shadow
    kMCPlatformThemePropertyBorderColor,            // [Color]      Color for control borders
    kMCPlatformThemePropertyFocusColor,             // [Color]      Color for keyboard focus indicator
    kMCPlatformThemePropertyTopEdgeColor,           // [Color]      Color for the top edge of 3D controls
    kMCPlatformThemePropertyBottomEdgeColor,        // [Color]      Color for the bottom edge of 3D controls
    kMCPlatformThemePropertyLeftEdgeColor,          // [Color]      Color for the left edge of 3D controls
    kMCPlatformThemePropertyRightEdgeColor          // [Color]      Color for the right edge of 3D controls
};

enum MCPlatformThemePropertyType
{
    kMCPlatformThemePropertyTypeFont,
    kMCPlatformThemePropertyTypeColor,
    kMCPlatformThemePropertyTypeInteger
};

bool MCPlatformGetControlThemePropBool(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, bool&);
bool MCPlatformGetControlThemePropInteger(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, int&);
bool MCPlatformGetControlThemePropColor(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCColor&);
bool MCPlatformGetControlThemePropFont(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCFontRef&);
bool MCPlatformGetControlThemePropString(MCPlatformControlType, MCPlatformControlPart, MCPlatformControlState, MCPlatformThemeProperty, MCStringRef&);

////////////////////////////////////////////////////////////////////////////////

#endif
