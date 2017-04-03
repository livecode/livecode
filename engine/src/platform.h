/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "prefix.h"

#define MC_PLATFORM_INSIDE
#include "platform-base.h"
#undef MC_PLATFORM_INSIDE

#ifndef __MC_GRAPHICS__
#include "graphics.h"
#endif

#ifndef MCTHEME_H
#include "mctheme.h"
#endif

#ifndef CONTEXT_H
#include "context.h"
#endif

#include "raw-clipboard.h"
#include "parsedef.h"
#include "graphics.h"

#if defined(TARGET_SUBPLATFORM_IPHONE)
#include <CoreGraphics/CoreGraphics.h>
#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER)
#include <ApplicationServices/ApplicationServices.h>
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

enum MCPlatformGlobalProperty
{
    kMCPlatformGlobalPropertyDoubleDelta,
    kMCPlatformGlobalPropertyDoubleTime,
    kMCPlatformGlobalPropertyDragDelta,
    kMCPlatformGlobalPropertyMajorOSVersion,
    kMCPlatformGlobalPropertyAppIsActive,
    kMCPlatformGlobalPropertyHINSTANCE,
    kMCPlatformGlobalPropertyQuit,
};

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
	kMCPlatformKeyCodeISOSection	= 0xfffe,
	kMCPlatformKeyCodeJISYen		= 0xfffe,
	kMCPlatformKeyCodeJISUnderscore	= 0xfffe,
	kMCPlatformKeyCodeJISKeypadComma= 0xfffe,
	kMCPlatformKeyCodeJISEisu		= 0xfffe,
	kMCPlatformKeyCodeJISKana		= 0xfffe,
	kMCPlatformKeyCodeFunction		= 0xfffe,
	
	kMCPlatformKeyCodeDelete		= 0xffff,
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
    
    kMCPlatformSystemPropertyVolume,
};

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCPlatformPreWaitForEventCallback)(double duration, bool blocking);
typedef bool (*MCPlatformPostWaitForEventCallback)(bool found_event);

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

enum MCPlatformPrintDialogResult
{
    kMCPlatformPrintDialogResultContinue,
    kMCPlatformPrintDialogResultError,
    kMCPlatformPrintDialogResultSuccess,
    kMCPlatformPrintDialogResultCancel
};

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
};

////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

enum MCPlatformSoundProperty
{
    kMCPlatformSoundPropertyVolume,
    kMCPlatformSoundPropertyLooping,
    kMCPlatformSoundPropertyDuration,
};

///////////////////////////////////////////////////////////////////////////////

typedef char *(*MCPlatformScriptEnvironmentCallback)(const char * const *arguments, uindex_t argument_count);

///////////////////////////////////////////////////////////////////////////////

typedef MCPlatform::PrintDialogSession MCPlatformPrintDialogSession;
typedef MCPlatform::PrintDialogSession *MCPlatformPrintDialogSessionRef;
typedef MCPlatform::Menu MCPlatformMenu;
typedef MCPlatform::Menu *MCPlatformMenuRef;
typedef MCPlatform::Cursor MCPlatformCursor;
typedef MCPlatform::Cursor *MCPlatformCursorRef;
typedef MCPlatform::ColorTransform MCPlatformColorTransform;
typedef class MCPlatform::ColorTransform *MCPlatformColorTransformRef;
typedef class MCPlatformWindow *MCPlatformWindowRef;
typedef class MCPlatformSurface *MCPlatformSurfaceRef;
typedef MCPlatform::WindowMask MCPlatformWindowMask;
typedef MCPlatform::WindowMask *MCPlatformWindowMaskRef;
typedef MCPlatform::LoadedFont MCPlatformLoadedFont;
typedef MCPlatform::LoadedFont *MCPlatformLoadedFontRef;
typedef MCPlatform::Sound MCPlatformSound;
typedef MCPlatform::Sound *MCPlatformSoundRef;
typedef class MCPlatformPlayer *MCPlatformPlayerRef;
typedef class MCPlatformSoundRecorder *MCPlatformSoundRecorderRef;
typedef MCPlatform::ScriptEnvironment MCPlatformScriptEnvironment;
typedef MCPlatform::ScriptEnvironment *MCPlatformScriptEnvironmentRef;
typedef MCPlatform::NativeLayer MCPlatformNativeLayer;
typedef MCPlatform::NativeLayer *MCPlatformNativeLayerRef;
typedef MCPlatform::Callback MCPlatformCallback;
typedef MCPlatform::Callback *MCPlatformCallbackRef;
#ifdef PLATFORM_IS_MINIMAL
typedef MCPlatform::MinimalCore MCPlatformCore;
typedef MCPlatform::MinimalCore *MCPlatformCoreRef;
#else
typedef MCPlatform::Core MCPlatformCore;
typedef MCPlatform::Core *MCPlatformCoreRef;
#endif

///////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform
{
    class Callback: public Base
    {
    public:
        Callback() = default;
        ~Callback() {}
        
        virtual bool Callback_GetGlobalProperty(MCPlatformGlobalProperty p_property, MCPlatformPropertyType p_type, void *r_value);
        
        virtual void Callback_SendApplicationStartup(int argc, MCStringRef *argv, MCStringRef *envp, int& r_error_code, MCStringRef& r_error_message);
        virtual void Callback_SendApplicationShutdown(int& r_exit_code);
        virtual void Callback_SendApplicationShutdownRequest(bool& r_terminate);
        virtual void Callback_SendApplicationRun(bool& r_continue);
        virtual void Callback_SendApplicationSuspend(void);
        virtual void Callback_SendApplicationResume(void);
        
        virtual void Callback_SendScreenParametersChanged(void);
        
        virtual void Callback_SendWindowCloseRequest(MCPlatformWindowRef window);
        virtual void Callback_SendWindowClose(MCPlatformWindowRef window);
        virtual void Callback_SendWindowCancel(MCPlatformWindowRef window);
        virtual void Callback_SendWindowReshape(MCPlatformWindowRef window, MCRectangle new_content);
        virtual void Callback_SendWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size);
        virtual void Callback_SendWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCGRegionRef dirty_rgn);
        virtual void Callback_SendWindowIconify(MCPlatformWindowRef window);
        virtual void Callback_SendWindowUniconify(MCPlatformWindowRef window);
        virtual void Callback_SendWindowFocus(MCPlatformWindowRef window);
        virtual void Callback_SendWindowUnfocus(MCPlatformWindowRef window);
        
        virtual void Callback_SendModifiersChanged(MCPlatformModifiers modifiers);
        
        virtual void Callback_SendMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
        virtual void Callback_SendMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
        virtual void Callback_SendMouseDrag(MCPlatformWindowRef window, uint32_t button);
        virtual void Callback_SendMouseRelease(MCPlatformWindowRef window, uint32_t button, bool was_menu);
        virtual void Callback_SendMouseEnter(MCPlatformWindowRef window);
        virtual void Callback_SendMouseLeave(MCPlatformWindowRef window);
        virtual void Callback_SendMouseMove(MCPlatformWindowRef window, MCPoint location);
        virtual void Callback_SendMouseScroll(MCPlatformWindowRef window, int32_t dx, int32_t dy);
        
        virtual void Callback_SendDragEnter(MCPlatformWindowRef window, MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation);
        virtual void Callback_SendDragLeave(MCPlatformWindowRef window);
        virtual void Callback_SendDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation);
        virtual void Callback_SendDragDrop(MCPlatformWindowRef window, bool& r_accepted);
        
        virtual void Callback_SendRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        
        virtual void Callback_SendKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        virtual void Callback_SendKeyUp(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        
        virtual void Callback_SendTextInputQueryTextRanges(MCPlatformWindowRef window, MCRange& r_marked_range, MCRange& r_selected_range);
        virtual void Callback_SendTextInputQueryTextIndex(MCPlatformWindowRef window, MCPoint location, uindex_t& r_index);
        virtual void Callback_SendTextInputQueryTextRect(MCPlatformWindowRef window, MCRange range, MCRectangle& first_line_rect, MCRange& r_actual_range);
        virtual void Callback_SendTextInputQueryText(MCPlatformWindowRef window, MCRange range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range);
        virtual void Callback_SendTextInputInsertText(MCPlatformWindowRef window, unichar_t *chars, uindex_t char_count, MCRange replace_range, MCRange selection_range, bool mark);
        virtual void Callback_SendTextInputAction(MCPlatformWindowRef window, MCPlatformTextInputAction action);
        
        virtual void Callback_SendMenuUpdate(MCPlatformMenuRef menu);
        virtual void Callback_SendMenuSelect(MCPlatformMenuRef menu, uindex_t item);
        
        virtual void Callback_SendViewFocusSwitched(MCPlatformWindowRef window, uint32_t view_id);
        
        virtual void Callback_SendPlayerFrameChanged(MCPlatformPlayerRef player);
        virtual void Callback_SendPlayerMarkerChanged(MCPlatformPlayerRef player, MCPlatformPlayerDuration time);
        virtual void Callback_SendPlayerCurrentTimeChanged(MCPlatformPlayerRef player);
        virtual void Callback_SendPlayerFinished(MCPlatformPlayerRef player);
        virtual void Callback_SendPlayerBufferUpdated(MCPlatformPlayerRef player);
        
        virtual void Callback_SendSoundFinished(MCPlatformSoundRef sound);
        
        virtual bool Callback_SystemWaitForEvent(double p_duration, bool p_blocking);
        virtual void Callback_SystemBreakWait(void);
#if defined(TARGET_SUBPLATFORM_IPHONE)
        virtual void Callback_RunBlockOnMainFiber(void (^block)(void));
#endif
        
        virtual void *Callback_MCListPopFront(void*& x_list);
        virtual void Callback_MCListPushBack(void*& x_list, void *p_element);
        virtual bool Callback_MCFontCreateWithHandle(MCSysFontHandle p_handle, MCNameRef p_name, MCFontRef& r_font);
        virtual MCRectangle Callback_MCU_reduce_rect(const MCRectangle &srect, int2 amount);
        virtual MCRectangle Callback_MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two);
        virtual Boolean Callback_MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y);
        virtual void Callback_MCU_urldecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result) const;
        virtual bool Callback_MCU_urlencode(MCStringRef p_url, bool p_use_utf8, MCStringRef &r_encoded) const;
        virtual void Callback_MCU_set_rect(MCRectangle &rect, int2 p_x, int2 p_y, uint2 p_w, uint2 p_h);
        virtual bool Callback_MCS_pathtonative(MCStringRef p_livecode_path, MCStringRef& r_native_path);
        virtual bool Callback_MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved_path);
        virtual Boolean Callback_MCS_exists(MCStringRef p_path, bool p_is_file);
        virtual Boolean Callback_MCS_unlink(MCStringRef p_path);
        virtual real8 Callback_MCS_time(void);
        virtual void Callback_MCGRasterApplyAlpha(MCGRaster &x_raster, const MCGRaster &p_alpha, const MCGIntegerPoint &p_offset);
        virtual bool Callback_MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count);
        virtual void Callback_MCImageFreeBitmap(MCImageBitmap *p_bitmap);
        virtual void Callback_MCImageBitmapClear(MCImageBitmap *p_bitmap);
        virtual bool Callback_MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap);
        virtual bool Callback_MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap);
        virtual MCGRaster Callback_MCImageBitmapGetMCGRaster(MCImageBitmap *p_bitmap, bool p_is_premultiplied);
        virtual bool Callback_MCSecureModeCanAccessAppleScript(void);
        virtual void Callback_SendOpenDoc(MCStringRef p_string);
        virtual void Callback_DoScript(MCStringRef p_script, MCStringRef &r_result);
        virtual void Callback_Eval(MCStringRef p_script, MCStringRef &r_result);
        virtual Exec_stat Callback_SendMessage(MCNameRef p_message, MCValueRef p_val1, MCValueRef p_val2, MCValueRef p_val3);
        
        // libgraphics
        virtual MCGImageRef Callback_MCGImageRetain(MCGImageRef image);
        virtual void Callback_MCGImageRelease(MCGImageRef image);
        virtual bool Callback_MCGRegionCreate(MCGRegionRef &r_region);
        virtual int32_t Callback_MCGImageGetWidth(MCGImageRef image);
        virtual bool Callback_MCGImageIsOpaque(MCGImageRef image);
        virtual bool Callback_MCGRegionAddRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect);
        virtual void Callback_MCGRegionDestroy(MCGRegionRef p_region);
        virtual bool Callback_MCGRegionIsEmpty(MCGRegionRef p_region);
        virtual bool Callback_MCGRegionIterate(MCGRegionRef p_region, MCGRegionIterateCallback p_callback, void *p_context);
        virtual void Callback_MCGContextRelease(MCGContextRef context);
        virtual int32_t Callback_MCGImageGetHeight(MCGImageRef image);
        virtual bool Callback_MCGImageGetRaster(MCGImageRef image, MCGRaster &r_raster);
        virtual bool Callback_MCGRegionSetEmpty(MCGRegionRef p_region);
        virtual void Callback_MCGContextScaleCTM(MCGContextRef context, MCGFloat xscale, MCGFloat yscale);
        virtual bool Callback_MCGRegionAddRegion(MCGRegionRef p_region, MCGRegionRef p_other);
        virtual MCGIntegerRectangle Callback_MCGRegionGetBounds(MCGRegionRef p_region);
        virtual void Callback_MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter);
        virtual void Callback_MCGContextClipToRect(MCGContextRef context, MCGRectangle rect);
        virtual void Callback_MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter);
        virtual MCGIntegerRectangle Callback_MCGRectangleGetBounds(const MCGRectangle &p_rect);
        virtual void Callback_MCGContextClipToRegion(MCGContextRef self, MCGRegionRef p_region);
        virtual void Callback_MCGContextTranslateCTM(MCGContextRef context, MCGFloat xoffset, MCGFloat yoffset);
        virtual bool Callback_MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context);
        virtual MCGAffineTransform Callback_MCGContextGetDeviceTransform(MCGContextRef context);
        virtual bool Callback_MCGImageCreateWithRasterNoCopy(const MCGRaster &raster, MCGImageRef &r_image);
        virtual MCGIntegerRectangle Callback_MCGIntegerRectangleIntersection(const MCGIntegerRectangle &rect_1, const MCGIntegerRectangle &rect_2);
        
        // cgimageutil.cpp
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        virtual CGBitmapInfo Callback_MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
        virtual bool Callback_MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);
        virtual bool Callback_MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image);
        virtual bool Callback_MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
#endif
        
        // libfoundation
        virtual bool Callback_MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data);
        virtual const byte_t *Callback_MCDataGetBytePtr(MCDataRef p_data);
        virtual uindex_t Callback_MCDataGetLength(MCDataRef p_data);
        virtual bool Callback_MCListAppend(MCListRef list, MCValueRef value);
        virtual bool Callback_MCListCopy(MCListRef list, MCListRef& r_new_list);
        virtual bool Callback_MCListCopyAsString(MCListRef list, MCStringRef& r_string);
        virtual bool Callback_MCMemoryAllocate(size_t size, void*& r_block);
        virtual bool Callback_MCMemoryReallocate(void *block, size_t new_size, void*& r_new_block);
        virtual void Callback_MCMemoryDeallocate(void *block);
        virtual bool Callback_MCMemoryNew(size_t size, void*& r_record);
        virtual void Callback_MCMemoryDelete(void *p_record);
        virtual MCNameRef Callback_MCNAME(const char *);
        virtual MCStringRef Callback_MCSTR(const char *string);
        virtual bool Callback_MCStringAppendFormatV(MCStringRef string, const char *format, va_list args);
        virtual bool Callback_MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options);
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        virtual bool Callback_MCStringConvertToCFStringRef(MCStringRef string, CFStringRef& r_cfstring);
        virtual bool Callback_MCStringCreateWithCFString(CFStringRef cf_string, MCStringRef& r_string);
#endif
        virtual bool Callback_MCStringConvertToCString(MCStringRef string, char*& r_cstring);
        virtual bool Callback_MCStringConvertToUTF8(MCStringRef string, char*& r_chars, uindex_t& r_char_count);
        virtual bool Callback_MCStringCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring);
        virtual bool Callback_MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithBytesAndRelease(byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithCString(const char *cstring, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithCStringAndRelease(char *cstring /*delete[]*/, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string);
        virtual bool Callback_MCStringCreateWithPascalString(const unsigned char* pascal_string, MCStringRef& r_string);
        virtual bool Callback_MCStringEncode(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data);
        virtual bool Callback_MCStringDecode(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
        virtual bool Callback_MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options);
        virtual bool Callback_MCStringFirstIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);
        virtual bool Callback_MCStringFormatV(MCStringRef& r_string, const char *format, va_list args);
        virtual uindex_t Callback_MCStringGetLength(const MCStringRef string);
        virtual bool Callback_MCStringIsEmpty(MCStringRef string);
        virtual bool Callback_MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options);
        virtual bool Callback_MCStringIsEqualToCString(MCStringRef string, const char *cstring, MCStringOptions options);
        virtual bool Callback_MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);
        virtual bool Callback_MCStringMutableCopy(MCStringRef string, MCStringRef& r_new_string);
        virtual bool Callback_MCStringPrepend(MCStringRef string, MCStringRef prefix);
        virtual bool Callback_MCStringSubstringIsEqualTo(MCStringRef string, MCRange range, MCStringRef p_other, MCStringOptions p_options);
        virtual void Callback_MCValueRelease(MCValueRef value);
        virtual MCValueRef Callback_MCValueRetain(MCValueRef value);
        virtual bool Callback_MCMemoryNewArray(uindex_t count, size_t size, void*& r_array, uindex_t& r_count);
        virtual bool Callback_MCMemoryNewArray(uindex_t count, size_t size, void*& r_array);
        virtual bool Callback_MCListCreateMutable(char_t delimiter, MCListRef& r_list);
        virtual void Callback_MCMemoryDeleteArray(void *array);
        virtual bool Callback_MCMemoryResizeArray(uindex_t new_count, size_t size, void*& x_array, uindex_t& x_count);
        virtual void Callback___MCAssert(const char *file, uint32_t line, const char *message);
        virtual void Callback___MCUnreachable(void);
        virtual bool Callback_MCWin32GetScreenDPI(uint32_t &r_xdpi, uint32_t &r_ydpi);

 };
    
    typedef Ref<Callback> CallbackRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    class Stubs
    {
    public:
        constexpr Stubs() = default;
        ~Stubs() {}
        void SetCallback(MCPlatformCallbackRef p_callback) {m_callback = p_callback;}
        MCPlatformCallbackRef GetCallback(void) {return m_callback; }
        
        bool GetGlobalProperty(MCPlatformGlobalProperty p_property, MCPlatformPropertyType p_type, void *r_value)
        {
            return m_callback -> Callback_GetGlobalProperty(p_property, p_type, r_value);
        }
        
        void SendApplicationStartup(int argc, MCStringRef *argv, MCStringRef *envp, int& r_error_code, MCStringRef& r_error_message)
        {
            m_callback -> Callback_SendApplicationStartup(argc, argv, envp, r_error_code, r_error_message);
        }
        void SendApplicationShutdown(int& r_exit_code)
        {
            m_callback -> Callback_SendApplicationShutdown(r_exit_code);
        }
        
        void SendApplicationShutdownRequest(bool& r_terminate)
        {
            m_callback -> Callback_SendApplicationShutdownRequest(r_terminate);
        }
        
        void SendApplicationRun(bool& r_continue)
        {
            m_callback -> Callback_SendApplicationRun(r_continue);
        }
        
        void SendApplicationSuspend(void)
        {
            m_callback -> Callback_SendApplicationSuspend();
        }
        
        void SendApplicationResume(void)
        {
            m_callback -> Callback_SendApplicationResume();
        }
        
        void SendScreenParametersChanged(void)
        {
            m_callback -> Callback_SendScreenParametersChanged();
        }
        
        void SendWindowCloseRequest(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowCloseRequest(window);
        }
        
        void SendWindowClose(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowClose(window);
        }
        
        void SendWindowCancel(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowCancel(window);
        }
        
        void SendWindowReshape(MCPlatformWindowRef window, MCRectangle new_content)
        {
            m_callback -> Callback_SendWindowReshape(window, new_content);
        }
        
        void SendWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size)
        {
            m_callback -> Callback_SendWindowConstrain(window, proposed_size, r_wanted_size);
        }
        
        void SendWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCGRegionRef dirty_rgn)
        {
            m_callback -> Callback_SendWindowRedraw(window, surface, dirty_rgn);
        }
        
        void SendWindowIconify(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowIconify(window);
        }
        
        void SendWindowUniconify(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowUniconify(window);
        }
        
        void SendWindowFocus(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowFocus(window);
        }
        
        void SendWindowUnfocus(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendWindowUnfocus(window);
        }
        
        void SendModifiersChanged(MCPlatformModifiers modifiers)
        {
            m_callback -> Callback_SendModifiersChanged(modifiers);
        }
        
        
        void SendMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count)
        {
            m_callback -> Callback_SendMouseDown(window, button, count);
        }
        
        void SendMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count)
        {
            m_callback -> Callback_SendMouseUp(window, button, count);
        }
        
        void SendMouseDrag(MCPlatformWindowRef window, uint32_t button)
        {
            m_callback -> Callback_SendMouseDrag(window, button);
        }
        
        void SendMouseRelease(MCPlatformWindowRef window, uint32_t button, bool was_menu)
        {
            m_callback -> Callback_SendMouseRelease(window, button, was_menu);
        }
        
        void SendMouseEnter(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendMouseEnter(window);
        }
        
        void SendMouseLeave(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendMouseLeave(window);
        }
        
        void SendMouseMove(MCPlatformWindowRef window, MCPoint location)
        {
            m_callback -> Callback_SendMouseMove(window, location);
        }
        
        void SendMouseScroll(MCPlatformWindowRef window, int32_t dx, int32_t dy)
        {
            m_callback -> Callback_SendMouseScroll(window, dx, dy);
        }
        
        void SendDragEnter(MCPlatformWindowRef window, MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation)
        {
            m_callback -> Callback_SendDragEnter(window, p_dragboard, r_operation);
        }
        
        void SendDragLeave(MCPlatformWindowRef window)
        {
            m_callback -> Callback_SendDragLeave(window);
        }
        
        void SendDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation)
        {
            m_callback -> Callback_SendDragMove(window, location, r_operation);
        }
        
        void SendDragDrop(MCPlatformWindowRef window, bool& r_accepted)
        {
            m_callback -> Callback_SendDragDrop(window, r_accepted);
        }
        
        void SendRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint)
        {
            m_callback -> Callback_SendRawKeyDown(window, key_code, mapped_codepoint, unmapped_codepoint);
        }
        
        void SendKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint)
        {
            m_callback -> Callback_SendKeyDown(window, key_code, mapped_codepoint, unmapped_codepoint);
        }
        
        void SendKeyUp(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint)
        {
            m_callback -> Callback_SendKeyUp(window, key_code, mapped_codepoint, unmapped_codepoint);
        }
        
        void SendTextInputQueryTextRanges(MCPlatformWindowRef window, MCRange& r_marked_range, MCRange& r_selected_range)
        {
            m_callback -> Callback_SendTextInputQueryTextRanges(window, r_marked_range, r_selected_range);
        }
        
        void SendTextInputQueryTextIndex(MCPlatformWindowRef window, MCPoint location, uindex_t& r_index)
        {
            m_callback -> Callback_SendTextInputQueryTextIndex(window, location, r_index);
        }
        
        void SendTextInputQueryTextRect(MCPlatformWindowRef window, MCRange range, MCRectangle& first_line_rect, MCRange& r_actual_range)
        {
            m_callback -> Callback_SendTextInputQueryTextRect(window, range, first_line_rect, r_actual_range);
        }
        
        void SendTextInputQueryText(MCPlatformWindowRef window, MCRange range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range)
        {
            m_callback -> Callback_SendTextInputQueryText(window, range, r_chars, r_char_count, r_actual_range);
        }
        
        void SendTextInputInsertText(MCPlatformWindowRef window, unichar_t *chars, uindex_t char_count, MCRange replace_range, MCRange selection_range, bool mark)
        {
            m_callback -> Callback_SendTextInputInsertText(window,chars, char_count, replace_range, selection_range, mark);
        }
        
        void SendTextInputAction(MCPlatformWindowRef window, MCPlatformTextInputAction action)
        {
            m_callback -> Callback_SendTextInputAction(window, action);
        }
        
        void SendMenuUpdate(MCPlatformMenuRef menu)
        {
            m_callback -> Callback_SendMenuUpdate(menu);
        }
        
        void SendMenuSelect(MCPlatformMenuRef menu, uindex_t item)
        {
            m_callback -> Callback_SendMenuSelect(menu, item);
        }
        
        void SendViewFocusSwitched(MCPlatformWindowRef window, uint32_t view_id)
        {
            m_callback -> Callback_SendViewFocusSwitched(window, view_id);
        }
        
        void SendPlayerFrameChanged(MCPlatformPlayerRef player)
        {
            m_callback -> Callback_SendPlayerFrameChanged(player);
        }
        
        void SendPlayerMarkerChanged(MCPlatformPlayerRef player, MCPlatformPlayerDuration time)
        {
            m_callback -> Callback_SendPlayerMarkerChanged(player, time);
        }
        
        void SendPlayerCurrentTimeChanged(MCPlatformPlayerRef player)
        {
            m_callback -> Callback_SendPlayerCurrentTimeChanged(player);
        }
        
        void SendPlayerFinished(MCPlatformPlayerRef player)
        {
            m_callback -> Callback_SendPlayerFinished(player);
        }
        
        void SendPlayerBufferUpdated(MCPlatformPlayerRef player)
        {
            m_callback -> Callback_SendPlayerBufferUpdated(player);
        }
        
        void SendSoundFinished(MCPlatformSoundRef sound)
        {
            m_callback -> Callback_SendSoundFinished(sound);
        }
        
        bool SystemWaitForEvent(double p_duration, bool p_blocking)
        {
            return m_callback -> Callback_SystemWaitForEvent(p_duration, p_blocking);
        }
        void SystemBreakWait(void)
        {
            m_callback -> Callback_SystemBreakWait();
        }

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        // Apple platforms only
        void RunBlockOnMainFiber(void (^block)(void))
        {
#if defined(TARGET_SUBPLATFORM_IPHONE)
            m_callback -> Callback_RunBlockOnMainFiber(block);
#else
            block();
#endif
        }
#endif
        
        void *MCListPopFront(void*& x_list)
        {
            return m_callback -> Callback_MCListPopFront(x_list);
        }
        
        void MCListPushBack(void*& x_list, void *p_element)
        {
            m_callback -> Callback_MCListPushBack(x_list, p_element);
        }
        
        bool MCFontCreateWithHandle(MCSysFontHandle p_handle, MCNameRef p_name, MCFontRef& r_font)
        {
            return m_callback -> Callback_MCFontCreateWithHandle(p_handle, p_name, r_font);
        }
        
        MCRectangle MCU_reduce_rect(const MCRectangle &srect, int2 amount)
        {
            return m_callback -> Callback_MCU_reduce_rect(srect, amount);
        }
        
        MCRectangle MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two)
        {
            return m_callback -> Callback_MCU_intersect_rect(one, two);
        }
        Boolean MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y)
        {
            return m_callback -> Callback_MCU_point_in_rect(srect, x, y);
        }
        void MCU_urldecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result) const
        {
            m_callback -> Callback_MCU_urldecode(p_source, p_use_utf8, r_result);
        }
        bool MCU_urlencode(MCStringRef p_url, bool p_use_utf8, MCStringRef &r_encoded) const
        {
            return m_callback -> Callback_MCU_urlencode(p_url, p_use_utf8, r_encoded);
        }
        void MCU_set_rect(MCRectangle &rect, int2 p_x, int2 p_y, uint2 p_w, uint2 p_h)
        {
            return m_callback -> Callback_MCU_set_rect(rect, p_x, p_y, p_w, p_h);
        }
        bool MCS_pathtonative(MCStringRef p_livecode_path, MCStringRef& r_native_path)
        {
            return m_callback -> Callback_MCS_pathtonative(p_livecode_path, r_native_path);
        }
        bool MCS_resolvepath(MCStringRef p_path, MCStringRef& r_resolved_path)
        {
            return m_callback -> Callback_MCS_resolvepath(p_path, r_resolved_path);
        }
        real8 MCS_time(void)
        {
            return m_callback -> Callback_MCS_time();
        }
        Boolean MCS_exists(MCStringRef p_path, bool p_is_file)
        {
            return m_callback -> Callback_MCS_exists(p_path, p_is_file);
        }
        Boolean MCS_unlink(MCStringRef p_path)
        {
            return m_callback -> Callback_MCS_unlink(p_path);
        }
        void MCGRasterApplyAlpha(MCGRaster &x_raster, const MCGRaster &p_alpha, const MCGIntegerPoint &p_offset)
        {
            m_callback -> Callback_MCGRasterApplyAlpha(x_raster, p_alpha, p_offset);
        }
        bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count)
        {
            return m_callback -> Callback_MCStringsSplit(p_string, p_separator, r_strings, r_count);
        }
        void MCImageFreeBitmap(MCImageBitmap *p_bitmap)
        {
            return m_callback -> Callback_MCImageFreeBitmap(p_bitmap);
        }
        void MCImageBitmapClear(MCImageBitmap *p_bitmap)
        {
            return m_callback -> Callback_MCImageBitmapClear(p_bitmap);
        }
        bool MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap)
        {
            return m_callback -> Callback_MCImageBitmapCreate(p_width, p_height, r_bitmap);
        }
        bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap)
        {
            return m_callback -> Callback_MCImageBitmapHasTransparency(p_bitmap);
        }
        MCGRaster MCImageBitmapGetMCGRaster(MCImageBitmap *p_bitmap, bool p_is_premultiplied)
        {
            return m_callback -> Callback_MCImageBitmapGetMCGRaster(p_bitmap, p_is_premultiplied);
        }
        bool MCSecureModeCanAccessAppleScript(void)
        {
            return m_callback -> Callback_MCSecureModeCanAccessAppleScript();
        }
        void SendOpenDoc(MCStringRef p_string)
        {
            m_callback -> Callback_SendOpenDoc(p_string);
        }
        void DoScript(MCStringRef p_script, MCStringRef &r_result)
        {
            m_callback -> Callback_DoScript(p_script, r_result);
        }
        void Eval(MCStringRef p_script, MCStringRef &r_result)
        {
            m_callback -> Callback_Eval(p_script, r_result);
        }
        Exec_stat SendMessage(MCNameRef p_message, MCValueRef p_val1, MCValueRef p_val2, MCValueRef p_val3)
        {
            return m_callback -> Callback_SendMessage(p_message, p_val1, p_val2, p_val3);
        }
        
        // libgraphics
        MCGImageRef MCGImageRetain(MCGImageRef image)
        {
            return m_callback -> Callback_MCGImageRetain(image);
        }
        void MCGImageRelease(MCGImageRef image)
        {
            m_callback -> Callback_MCGImageRelease(image);
        }
        bool MCGRegionCreate(MCGRegionRef &r_region)
        {
            return m_callback -> Callback_MCGRegionCreate(r_region);
        }
        int32_t MCGImageGetWidth(MCGImageRef image)
        {
            return m_callback -> Callback_MCGImageGetWidth(image);
        }
        bool MCGImageIsOpaque(MCGImageRef image)
        {
            return m_callback -> Callback_MCGImageIsOpaque(image);
        }
        bool MCGRegionAddRect(MCGRegionRef p_region, const MCGIntegerRectangle &p_rect)
        {
            return m_callback -> Callback_MCGRegionAddRect(p_region, p_rect);
        }
        void MCGRegionDestroy(MCGRegionRef p_region)
        {
            m_callback -> Callback_MCGRegionDestroy(p_region);
        }
        bool MCGRegionIsEmpty(MCGRegionRef p_region)
        {
            return m_callback -> Callback_MCGRegionIsEmpty(p_region);
        }
        bool MCGRegionIterate(MCGRegionRef p_region, MCGRegionIterateCallback p_callback, void *p_context)
        {
            return m_callback -> Callback_MCGRegionIterate(p_region, p_callback, p_context);
        }
        void MCGContextRelease(MCGContextRef context)
        {
            m_callback -> Callback_MCGContextRelease(context);
        }
        int32_t MCGImageGetHeight(MCGImageRef image)
        {
            return m_callback -> Callback_MCGImageGetHeight(image);
        }
        bool MCGImageGetRaster(MCGImageRef image, MCGRaster &r_raster)
        {
            return m_callback -> Callback_MCGImageGetRaster(image, r_raster);
        }
        bool MCGRegionSetEmpty(MCGRegionRef p_region)
        {
            return m_callback -> Callback_MCGRegionSetEmpty(p_region);
        }
        void MCGContextScaleCTM(MCGContextRef context, MCGFloat xscale, MCGFloat yscale)
        {
            m_callback -> Callback_MCGContextScaleCTM(context, xscale, yscale);
        }
        bool MCGRegionAddRegion(MCGRegionRef p_region, MCGRegionRef p_other)
        {
            return m_callback -> Callback_MCGRegionAddRegion(p_region, p_other);
        }
        MCGIntegerRectangle MCGRegionGetBounds(MCGRegionRef p_region)
        {
            return m_callback -> Callback_MCGRegionGetBounds(p_region);
        }
        void MCGContextDrawImage(MCGContextRef context, MCGImageRef image, MCGRectangle dst_rect, MCGImageFilter filter)
        {
            m_callback -> Callback_MCGContextDrawImage(context, image, dst_rect, filter);
        }
        void MCGContextClipToRect(MCGContextRef context, MCGRectangle rect)
        {
            m_callback -> Callback_MCGContextClipToRect(context, rect);
        }
        void MCGContextDrawPixels(MCGContextRef context, const MCGRaster& raster, MCGRectangle dst_rect, MCGImageFilter filter)
        {
            m_callback -> Callback_MCGContextDrawPixels(context, raster, dst_rect, filter);
        }
        MCGIntegerRectangle MCGRectangleGetBounds(const MCGRectangle &p_rect)
        {
            return m_callback -> Callback_MCGRectangleGetBounds(p_rect);
        }
        void MCGContextClipToRegion(MCGContextRef self, MCGRegionRef p_region)
        {
            m_callback -> Callback_MCGContextClipToRegion(self, p_region);
        }
        void MCGContextTranslateCTM(MCGContextRef context, MCGFloat xoffset, MCGFloat yoffset)
        {
            m_callback -> Callback_MCGContextTranslateCTM(context, xoffset, yoffset);
        }
        bool MCGContextCreateWithRaster(const MCGRaster& raster, MCGContextRef& r_context)
        {
            return m_callback -> Callback_MCGContextCreateWithRaster(raster, r_context);
        }
        MCGAffineTransform MCGContextGetDeviceTransform(MCGContextRef context)
        {
            return m_callback -> Callback_MCGContextGetDeviceTransform(context);
        }
        bool MCGImageCreateWithRasterNoCopy(const MCGRaster &raster, MCGImageRef &r_image)
        {
            return m_callback -> Callback_MCGImageCreateWithRasterNoCopy(raster, r_image);
        }
        MCGIntegerRectangle MCGIntegerRectangleIntersection(const MCGIntegerRectangle &rect_1, const MCGIntegerRectangle &rect_2)
        {
            return m_callback -> Callback_MCGIntegerRectangleIntersection(rect_1, rect_2);
        }
        
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        // cgimageutil.cpp
        CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha)
        {
            return m_callback -> Callback_MCGPixelFormatToCGBitmapInfo(p_pixel_format, p_alpha);
        }
        bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace)
        {
            return m_callback -> Callback_MCImageGetCGColorSpace(r_colorspace);
        }
        bool MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image)
        {
            return m_callback -> Callback_MCGImageToCGImage(p_src, p_src_rect, p_invert, r_image);
        }
        bool MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image)
        {
            return m_callback -> Callback_MCGRasterToCGImage(p_raster, p_src_rect, p_colorspace, p_copy, p_invert, r_image);
        }
#endif
        
        // libfoundation
        bool MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data) const
        {
            return m_callback -> Callback_MCDataCreateWithBytes(p_bytes, p_byte_count, r_data);
        }
        const byte_t *MCDataGetBytePtr(MCDataRef p_data)
        {
            return m_callback -> Callback_MCDataGetBytePtr(p_data);
        }
        uindex_t MCDataGetLength(MCDataRef p_data)
        {
            return m_callback -> Callback_MCDataGetLength(p_data);
        }
        bool MCListAppend(MCListRef list, MCValueRef value)
        {
            return m_callback -> Callback_MCListAppend(list, value);
        }
        bool MCListCopy(MCListRef list, MCListRef& r_new_list)
        {
            return m_callback -> Callback_MCListCopy(list, r_new_list);
        }
        bool MCListCopyAsString(MCListRef list, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCListCopyAsString(list, r_string);
        }
        bool MCMemoryAllocate(size_t size, void*& r_block)
        {
            return m_callback -> Callback_MCMemoryAllocate(size, r_block);
        }
        bool MCMemoryReallocate(void *block, size_t new_size, void*& r_new_block)
        {
            return m_callback -> Callback_MCMemoryReallocate(block, new_size, r_new_block);
        }
        void MCMemoryDeallocate(void *block)
        {
            m_callback -> Callback_MCMemoryDeallocate(block);
        }
        bool MCMemoryNew(size_t size, void*& r_record)
        {
            return m_callback -> Callback_MCMemoryNew(size, r_record);
        }
        void MCMemoryDelete(void *p_record)
        {
            return m_callback -> Callback_MCMemoryDelete(p_record);
        }
        MCNameRef MCNAME(const char *p_string)
        {
            return m_callback -> Callback_MCNAME(p_string);
        }
        MCStringRef MCSTR(const char *string) const
        {
            return m_callback -> Callback_MCSTR(string);
        }
        bool MCStringAppendFormat(MCStringRef string, const char *p_format, ...)
        {
            bool t_success;
            va_list t_args;
            va_start(t_args, p_format);
            t_success =  m_callback -> Callback_MCStringAppendFormatV(string, p_format, t_args);
            va_end(t_args);
            return t_success;
        }
        bool MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options) const
        {
            return m_callback -> Callback_MCStringBeginsWithCString(string, prefix_cstring, options);
        }
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        bool MCStringConvertToCFStringRef(MCStringRef string, CFStringRef& r_cfstring) const
        {
            return m_callback -> Callback_MCStringConvertToCFStringRef(string, r_cfstring);
        }
         bool MCStringCreateWithCFString(CFStringRef cf_string, MCStringRef& r_string) const
        {
            return m_callback -> Callback_MCStringCreateWithCFString(cf_string, r_string);
        }
#endif
        bool MCStringConvertToCString(MCStringRef string, char*& r_cstring)
        {
            return m_callback -> Callback_MCStringConvertToCString(string, r_cstring);
        }
        bool MCStringConvertToUTF8(MCStringRef string, char*& r_chars, uindex_t& r_char_count)
        {
            return m_callback -> Callback_MCStringConvertToUTF8(string, r_chars, r_char_count);
        }
        bool MCStringCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring) const
        {
            return m_callback -> Callback_MCStringCopySubstring(string, range, r_substring);
        }
        bool MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateMutable(initial_capacity, r_string);
        }
        bool MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithBytes(bytes, byte_count, encoding, is_external_rep, r_string);
        }
        bool MCStringCreateWithBytesAndRelease(byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithBytesAndRelease(bytes, byte_count, encoding, is_external_rep, r_string);
        }
        bool MCStringCreateWithCString(const char *cstring, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithCString(cstring, r_string);
        }
        bool MCStringCreateWithCStringAndRelease(char *cstring /*delete[]*/, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithCStringAndRelease(cstring , r_string);
        }
        bool MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithNativeChars(chars, char_count, r_string);
        }
        bool MCStringCreateWithPascalString(const unsigned char* pascal_string, MCStringRef& r_string)
        {
            return m_callback -> Callback_MCStringCreateWithPascalString(pascal_string, r_string);
        }
        bool MCStringEncode(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data) const
        {
            return m_callback -> Callback_MCStringEncode(string, encoding, is_external_rep, r_data);
        }
        bool MCStringDecode(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string) const
        {
            return m_callback -> Callback_MCStringDecode(data, encoding, is_external_rep, r_string);
        }
        bool MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options) const
        {
            return m_callback -> Callback_MCStringFindAndReplace(string, pattern, replacement, options);
        }
        bool MCStringFirstIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset) const
        {
            return m_callback -> Callback_MCStringFirstIndexOfChar(string, needle, after, options, r_offset);
        }
        bool MCStringFormat(MCStringRef& r_string, const char *p_format, ...)
        {
            bool t_success;
            
            va_list t_args;
            va_start(t_args, p_format);
            t_success = m_callback -> Callback_MCStringFormatV(r_string, p_format, t_args);
            va_end(t_args);
            
            return t_success;
        }
        uindex_t MCStringGetLength(const MCStringRef string) const
        {
            return m_callback -> Callback_MCStringGetLength(string);
        }
        bool MCStringIsEmpty(MCStringRef string)
        {
            return m_callback -> Callback_MCStringIsEmpty(string);
        }
        bool MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options) const
        {
            return m_callback -> Callback_MCStringIsEqualTo(string, other, options);
        }
        bool MCStringIsEqualToCString(MCStringRef string, const char *cstring, MCStringOptions options)
        {
            return m_callback -> Callback_MCStringIsEqualToCString(string, cstring, options);
        }
        bool MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t before, MCStringOptions options, uindex_t& r_offset)
        {
            return m_callback -> Callback_MCStringLastIndexOfChar(string, needle, before, options, r_offset);
        }
        bool MCStringMutableCopy(MCStringRef string, MCStringRef& r_new_string) const
        {
            return m_callback -> Callback_MCStringMutableCopy(string, r_new_string);
        }
        bool MCStringPrepend(MCStringRef string, MCStringRef prefix) const
        {
            return m_callback -> Callback_MCStringPrepend(string, prefix);
        }
        bool MCStringSubstringIsEqualTo(MCStringRef string, MCRange range, MCStringRef p_other, MCStringOptions p_options)
        {
            return m_callback -> Callback_MCStringSubstringIsEqualTo(string, range, p_other, p_options);
        }
        void MCValueRelease(MCValueRef value)
        {
            return m_callback -> Callback_MCValueRelease(value);
        }
        MCValueRef MCValueRetain(MCValueRef value) const
        {
            return m_callback -> Callback_MCValueRetain(value);
        }
        bool MCMemoryNewArray(uindex_t count, size_t size, void*& r_array, uindex_t& r_count)
        {
            return m_callback -> Callback_MCMemoryNewArray(count, size, r_array, r_count);
        }
        bool MCMemoryNewArray(uindex_t count, size_t size, void*& r_array)
        {
            return m_callback -> Callback_MCMemoryNewArray(count, size, r_array);
        }
        bool MCListCreateMutable(char_t delimiter, MCListRef& r_list)
        {
            return m_callback -> Callback_MCListCreateMutable(delimiter, r_list);
        }
        void MCMemoryDeleteArray(void *array)
        {
            return m_callback -> Callback_MCMemoryDeleteArray(array);
        }
        bool MCMemoryResizeArray(uindex_t new_count, size_t size, void*& x_array, uindex_t& x_count)
        {
            return m_callback -> Callback_MCMemoryResizeArray(new_count, size, x_array, x_count);
        }
        void __MCAssert(const char *file, uint32_t line, const char *message) const
        {
            return m_callback -> Callback___MCAssert(file, line, message);
        }
        void __MCUnreachable(void)
        {
            return m_callback -> Callback___MCUnreachable();
        }
        bool MCWin32GetScreenDPI(uint32_t &r_xdpi, uint32_t &r_ydpi)
        {
            return m_callback -> Callback_MCWin32GetScreenDPI(r_xdpi, r_ydpi);
        }

        template<typename T> bool MCMemoryNewArray(uindex_t p_count, T*& r_array, uindex_t& r_count)
        {
            void *t_array;
            if (m_callback -> Callback_MCMemoryNewArray(p_count, sizeof(T), t_array, r_count))
                return r_array = static_cast<T *>(t_array), true;
            return false;
        }
        
        template<typename T> bool MCMemoryNewArray(uindex_t p_count, T*& r_array)
        {
            void *t_array;
            if (m_callback -> Callback_MCMemoryNewArray(p_count, sizeof(T), t_array))
                return r_array = static_cast<T *>(t_array), true;
            return false;
        }
        template<typename T> bool MCMemoryResizeArray(uindex_t p_new_count, T*& x_array, uindex_t& x_count)
        {
            void *t_array;
            t_array = x_array;
            if (m_callback -> Callback_MCMemoryResizeArray(p_new_count, sizeof(T), t_array, x_count))
            {
                x_array = static_cast<T *>(t_array);
                return true;
            }
            return false;
        }
        template<typename T> bool MCMemoryAllocate(size_t p_size, T*& r_block)
        {
            void *t_block;
            if (m_callback -> Callback_MCMemoryAllocate(p_size, t_block))
            {
                r_block = static_cast<T *>(t_block);
                return true;
            }
            return false;
        }
        template<typename T> bool MCMemoryNew(T*& r_record)
        {
            void *t_record;
            if (MCMemoryNew(sizeof(T), t_record))
            {
                r_record = new (t_record) T;
                return true;
            }
            return false;
        }
        template<typename T> T MCValueRetain(T value) const
        {
            return reinterpret_cast<T>(m_callback -> Callback_MCValueRetain((MCValueRef)value));
        }
        template<typename T> void MCValueAssign(T& dst, T src)
        {
            if (src == dst)
                return;
            
            m_callback -> Callback_MCValueRetain(src);
            m_callback -> Callback_MCValueRelease(dst);
            dst = src;
        }
        template<typename T> bool MCMemoryReallocate(T *p_block, size_t p_new_size, T*& r_new_block)
        {
            void *t_new_block;
            if (m_callback -> Callback_MCMemoryReallocate(p_block, p_new_size, t_new_block))
            {
                r_new_block = static_cast<T *>(t_new_block);
                return true;
            }
            return false;
        }
        template<typename T> void MCMemoryDeleteArray(T* p_array)
        {
            m_callback -> Callback_MCMemoryDeleteArray(static_cast<void *>(p_array));
        }
        
        // Array deleter that runs the destructor for each element
        template <typename T>
        void MCMemoryDeleteArray(T* p_array, uindex_t N)
        {
            // Run the destructor for each of the elements
            for (size_t i = 0; i < N; i++)
                p_array[i].~T();
            
            // Destroy the array
            m_callback -> Callback_MCMemoryDeleteArray(static_cast<void *>(p_array));
        }
    protected:
        MCPlatformCallbackRef m_callback = nil;
    };
}

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class WindowMask: public CoreReference, public Stubs
    {
    public:
        WindowMask(void) = default;
        virtual ~WindowMask(void) {}
        
        virtual bool IsValid(void) const = 0;
        
        virtual bool CreateWithAlphaAndRelease(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits) = 0;
    };
    
    typedef Ref<WindowMask> WindowMaskRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to match the new stack surface API.
//  You can now lock/unlock multiple areas of the surface, but need to store the context and raster for those areas locally.
class MCPlatformSurface: public MCPlatform::Stubs
{
public:
    constexpr MCPlatformSurface(void) = default;
    virtual ~MCPlatformSurface(void) {}
    
    virtual bool LockGraphics(MCGIntegerRectangle area, MCGContextRef& r_context, MCGRaster &r_raster) = 0;
    virtual void UnlockGraphics(MCGIntegerRectangle area, MCGContextRef context, MCGRaster &raster) = 0;
    
    virtual bool LockPixels(MCGIntegerRectangle area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area) = 0;
    virtual void UnlockPixels(MCGIntegerRectangle area, MCGRaster& raster) = 0;
    
    virtual bool LockSystemContext(void*& r_context) = 0;
    virtual void UnlockSystemContext(void) = 0;
    
    virtual bool Composite(MCGRectangle dst_rect, MCGImageRef src_image, MCGRectangle src_rect, MCGFloat opacity, MCGBlendMode blend) = 0;
    
    virtual MCGFloat GetBackingScaleFactor(void) = 0;
};

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCPlatformWindowAttachmentCallback)(void *object, bool realized);

struct MCPlatformWindowAttachment
{
    void *object;
    MCPlatformWindowAttachmentCallback callback;
};

class MCPlatformWindow: public MCPlatform::CoreReference, public MCPlatform::Stubs
{
public:
    MCPlatformWindow(MCPlatformCoreRef p_platform);
    virtual ~MCPlatformWindow(void);
    
    // Returns true if the window is being shown.
    bool IsVisible(void);
    
    // Force an immediate update of the window's dirty region. Assuming there
    // is updates to be made, this will cause an immediate redraw callback.
    void Update(void);
    
    // Add the given region to the window's dirty region.
    void Invalidate(MCGRegionRef region);
    
    // Make the window visible as the given class.
    void Show(void);
    void ShowAsSheet(MCPlatformWindowRef parent);
    
    // Make the window invisible.
    void Hide(void);
    
    // Set input focus to the window.
    void Focus(void);
    
    // Bring the window to front.
    void Raise(void);
    
    // Minimize / miniturize the window.
    void Iconify(void);
    
    // Deminimize / deminiturize the window.
    void Uniconify(void);
    
    // Manage text input sessions
    void ConfigureTextInput(bool enabled);
    void ResetTextInput(void);
    bool IsTextInputActive(void);
    
    // Set the given window property.
    void SetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value);
    
    // Get the given window property.
    void GetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value);
    
    // Map co-ords from window to screen and vice-versa.
    void MapPointFromScreenToWindow(MCPoint screen_point, MCPoint& r_window_point);
    void MapPointFromWindowToScreen(MCPoint window_point, MCPoint& r_screen_point);
    
    // Attach an object that needs to be notified when the window is realized / unrealized.
    void AttachObject(void *object, MCPlatformWindowAttachmentCallback callback);
    void DetachObject(void *object);
    
public:
    
    void HandleCloseRequest(void);
    
    void HandleRedraw(MCPlatformSurfaceRef surface, MCGRegionRef update_rgn);
    void HandleReshape(MCRectangle new_content);
    void HandleIconify(void);
    void HandleUniconify(void);
    void HandleFocus(void);
    void HandleUnfocus(void);
    
    void HandleKeyDown(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
    void HandleKeyUp(MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
    
    void HandleDragEnter(class MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation);
    void HandleDragMove(MCPoint location, MCPlatformDragOperation& r_operation);
    void HandleDragLeave(void);
    void HandleDragDrop(bool& r_accepted);
    // Called to tell attachments there is a handle.
    void RealizeAndNotify(void);
    
    //////////
    
    virtual void SwitchFocusToView(uint32_t p_id) = 0;
    
    virtual void DoRealize(void) = 0;
    virtual void DoSynchronize(void) = 0;
    
    virtual bool DoSetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value) = 0;
    virtual bool DoGetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value) = 0;
    
    virtual void DoShow(void) = 0;
    virtual void DoShowAsSheet(MCPlatformWindowRef parent) = 0;
    virtual void DoHide(void) = 0;
    virtual void DoFocus(void) = 0;
    virtual void DoRaise(void) = 0;
    virtual void DoUpdate(void) = 0;
    virtual void DoIconify(void) = 0;
    virtual void DoUniconify(void) = 0;
    virtual void DoConfigureTextInput(void) = 0;
    virtual void DoResetTextInput(void) = 0;
    
    virtual void DoMapContentRectToFrameRect(MCRectangle content, MCRectangle& r_frame) = 0;
    virtual void DoMapFrameRectToContentRect(MCRectangle frame, MCRectangle& r_content) = 0;
    
protected:
    
    // Any attachments the window has.
    MCPlatformWindowAttachment *m_attachments;
    uindex_t m_attachment_count;
    
    // Universal property values.
    struct
    {
        bool style_changed : 1;
        bool opacity_changed : 1;
        bool mask_changed : 1;
        bool content_changed : 1;
        bool title_changed : 1;
        bool has_title_widget_changed : 1;
        bool has_close_widget_changed : 1;
        bool has_collapse_widget_changed : 1;
        bool has_zoom_widget_changed : 1;
        bool has_size_widget_changed : 1;
        bool has_shadow_changed : 1;
        bool is_opaque_changed : 1;
        bool has_modified_mark_changed : 1;
        bool use_live_resizing_changed : 1;
        
        // MW-2014-04-08: [[ Bug 12073 ]] Changed flag for mouse cursor.
        bool cursor_changed : 1;
        
        bool hides_on_suspend_changed : 1;
        
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Changed flag for ignore mouse events.
        bool ignore_mouse_events_changed : 1;
        
        // MERG-2015-10-11: [[ DocumentFilename ]] Changed flag for docuent filename
        bool document_filename_changed : 1;
    } m_changes;
    MCPlatformWindowStyle m_style;
    MCStringRef m_title;
    MCPlatformWindowMaskRef m_mask;
    float m_opacity;
    MCRectangle m_content;
    MCCursorRef m_cursor;
    // MERG-2015-10-11: [[ DocumentFilename ]] documentFilename property
    MCStringRef m_document_filename;
    struct
    {
        bool m_has_title_widget : 1;
        bool m_has_close_widget : 1;
        bool m_has_collapse_widget : 1;
        bool m_has_zoom_widget : 1;
        bool m_has_size_widget : 1;
        bool m_has_shadow : 1;
        bool m_is_opaque : 1;
        bool m_has_modified_mark : 1;
        bool m_use_live_resizing : 1;
        bool m_hides_on_suspend : 1;
        // MERG-2014-06-02: [[ IgnoreMouseEvents ]] ignoreMouseEvents property
        bool m_ignore_mouse_events : 1;
    };
    
    // Universal state.
    MCGRegionRef m_dirty_region;
    struct
    {
        bool m_is_visible : 1;
        bool m_is_focused : 1;
        bool m_is_iconified : 1;
        bool m_use_text_input : 1;
        bool m_is_realized : 1;
    };
};

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class ColorTransform: public CoreReference, public Stubs
    {
    public:
        virtual bool CreateWithColorSpace(const MCColorSpaceInfo& p_info) = 0;
        virtual bool Apply(MCImageBitmap *p_image) = 0;
    };
    
    typedef Ref<ColorTransform> ColorTransformRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class LoadedFont: public CoreReference, public Stubs
    {
    public:
        virtual bool CreateWithPath(MCStringRef p_path, bool p_globally) = 0;
    };
    
    typedef Ref<LoadedFont> LoadedFontRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Cursor: public CoreReference, public Stubs
    {
    public:
        virtual void CreateStandard(MCPlatformStandardCursor p_standard_cursor) = 0;
        virtual void CreateCustom(MCImageBitmap *p_image, MCPoint p_hotspot) = 0;
        virtual void Set(void) = 0;
    };
    
    typedef Ref<Cursor> CursorRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Menu: public CoreReference, public Stubs
    {
    public:
        virtual void SetTitle(MCStringRef p_title) = 0;
        virtual uindex_t CountItems(void) = 0;
        virtual void AddItem(uindex_t p_where) = 0;
        virtual void AddSeparatorItem(uindex_t p_where) = 0;
        virtual void RemoveItem(uindex_t p_where) = 0;
        virtual void RemoveAllItems(void) = 0;
        
        /* Warning: the pointer returned into r_parent is _not_ owned by the
         * caller. */
        /* TODO [2017-02-24] Maybe this should return a
         * std::tuple<Ref<Menu>, uindex_t> */
        virtual void GetParent(MCPlatformMenuRef& r_parent, uindex_t& r_index) = 0;
        
        virtual void GetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value) = 0;
        virtual void SetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value) = 0;
        virtual bool PopUp(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item) = 0;
        virtual void StartUsingAsMenubar(void) = 0;
        virtual void StopUsingAsMenubar(void) = 0;
    };
    
    typedef Ref<Menu> MenuRef;
    
} /* namespace MCPlatform */


////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class PrintDialogSession: public CoreReference, public Stubs
    {
    public:
        virtual void BeginPageSetup(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format) = 0;
        virtual void BeginSettings(MCPlatformWindowRef p_window, void *p_session, void *p_settings, void * p_page_format) = 0;
        virtual void CopyInfo(void *&r_session, void *&r_settings, void *&r_page_format) = 0;
        virtual void SetResult(MCPlatformPrintDialogResult p_result) = 0;
        virtual MCPlatformPrintDialogResult GetResult(void) = 0;
    };
    
    typedef Ref<PrintDialogSession> PrintDialogSessionRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Sound: public CoreReference, public Stubs
    {
    public:
        virtual bool IsValid(void) const = 0;
        
        virtual bool CreateWithData(const void *data, size_t data_size) = 0;
        
        virtual bool IsPlaying(void) const = 0;
        
        virtual void Play(void) = 0;
        virtual void Pause(void) = 0;
        virtual void Resume(void) = 0;
        virtual void Stop(void) = 0;
        
        virtual void SetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value) = 0;
        virtual void GetProperty(MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value) = 0;
    };
    
    typedef Ref<Sound> SoundRef;
}

////////////////////////////////////////////////////////////////////////////////

class MCPlatformPlayer: public MCPlatform::CoreReference, public MCPlatform::Stubs
{
public:
    MCPlatformPlayer(void) = default;
    virtual ~MCPlatformPlayer(void) {};
    
    virtual bool GetNativeView(void *&r_view) = 0;
    virtual bool SetNativeParentView(void *p_parent_view) = 0;
    
    virtual bool IsPlaying(void) = 0;
    // PM-2014-05-28: [[ Bug 12523 ]] Take into account the playRate property
    virtual void Start(double rate) = 0;
    virtual void Stop(void) = 0;
    virtual void Step(int amount) = 0;
    
    virtual bool LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap) = 0;
    virtual void UnlockBitmap(MCImageBitmap *bitmap) = 0;
    
    virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
    virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value) = 0;
    
    virtual void CountTracks(uindex_t& r_count) = 0;
    virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index) = 0;
    virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
    virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value) = 0;
    
protected:
    virtual void Realize(void) = 0;
    virtual void Unrealize(void) = 0;
    
};

////////////////////////////////////////////////////////////////////////////////

class MCPlatformSoundRecorder: public MCPlatform::CoreReference, public MCPlatform::Stubs
{
public:
    MCPlatformSoundRecorder(void);
    virtual ~MCPlatformSoundRecorder(void);
    
    virtual bool IsRecording(void);
    
    virtual void SetProperty(MCPlatformSoundRecorderProperty property, MCPlatformPropertyType type, void *value);
    virtual void GetProperty(MCPlatformSoundRecorderProperty property, MCPlatformPropertyType type, void *value);
    
    virtual void GetConfiguration(MCPlatformSoundRecorderConfiguration &r_config);
    virtual void SetConfiguration(const MCPlatformSoundRecorderConfiguration p_config);
    
    virtual void BeginDialog(void) = 0;
    virtual MCPlatformDialogResult EndDialog(void) = 0;
    virtual bool StartRecording(MCStringRef filename) = 0;
    virtual void StopRecording(void) = 0;
    virtual void PauseRecording(void) = 0;
    virtual void ResumeRecording(void) = 0;
    virtual double GetLoudness(void) = 0;
    
    virtual bool ListInputs(MCPlatformSoundRecorderListInputsCallback callback, void *context) = 0;
    virtual bool ListCompressors(MCPlatformSoundRecorderListCompressorsCallback callback, void *context) = 0;
    
protected:
    
    bool m_recording;;
    MCStringRef m_filename;
    
    // The recorder's current configuration settings.
    MCPlatformSoundRecorderConfiguration m_configuration;
};

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform
{
    
    class ScriptEnvironment: public CoreReference, public Stubs
    {
    public:
        virtual bool Define(const char *p_function, MCPlatformScriptEnvironmentCallback p_callback) = 0;
        
        virtual void Run(MCStringRef p_script, MCStringRef &r_result) = 0;
        
        virtual char *Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count) = 0;
    };
    
    typedef Ref<ScriptEnvironment> ScriptEnvironmentRef;
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////


namespace MCPlatform
{
    
    class NativeLayer : public CoreReference, public Stubs
    {
    public:
        
        virtual bool GetNativeView(void *&r_view) = 0;
        virtual void SetNativeView(void *p_view) = 0;
        
        // Performs the attach/detach operations
        virtual void Attach(MCPlatformWindowRef p_window, void *p_container_view, void *p_view_above, bool p_visible) = 0;
        virtual void Detach() = 0;
        
        virtual bool Paint(MCGContextRef p_context) = 0;
        virtual void SetGeometry(const MCRectangle &p_rect) = 0;
        virtual void SetViewportGeometry(const MCRectangle &p_rect) = 0;
        virtual void SetVisible(bool p_visible) = 0;
        
        // Performs a relayering operation
        virtual void Relayer(void *p_container_view, void *p_view_above) = 0;
    };
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class MinimalCore: public Base, public Stubs
    {
    public:
        // Wait
        virtual bool WaitForEvent(double p_duration, bool p_blocking) = 0;
        virtual void BreakWait(void) = 0;
        
        // Player
        virtual MCPlatformPlayerRef CreatePlayer(void) = 0;
        
        // Themes
        virtual bool GetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool) = 0;
        virtual bool GetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int) = 0;
        virtual bool GetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color) = 0;
        virtual bool GetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font) = 0;
        virtual bool GetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string) = 0;
        
        // Creates a new clipboard associated with the main system clipboard. Note
        // that these are not kept synchronised - clipboards are only updated when
        // the "synchronizeUpdates" method is called.
        virtual MCRawClipboard* CreateSystemClipboard() = 0;
        virtual MCRawClipboard* CreateSystemSelectionClipboard() = 0;
        virtual MCRawClipboard* CreateSystemDragboard() = 0;
    };
    
#ifdef PLATFORM_IS_MINIMAL
    typedef Ref<MinimalCore> CoreRef;
#endif
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Core: public MinimalCore
    {
    public:
        virtual int Run(int argc, char *argv[], char *envp[]) = 0;
        
        // Wait
        virtual bool WaitForEvent(double p_duration, bool p_blocking) = 0;
        virtual void BreakWait(void) = 0;
        virtual void SetWaitForEventCallbacks(MCPlatformPreWaitForEventCallback p_pre, MCPlatformPostWaitForEventCallback p_post) = 0;
        
        // Callbacks
        virtual void ScheduleCallback(void (*p_callback)(void *), void *p_context) = 0;
        
        // Abort key
        virtual bool InitializeAbortKey(void) = 0;
        virtual void FinalizeAbortKey(void) = 0;
        virtual bool GetAbortKeyPressed(void) = 0;
        virtual void DisableAbortKey(void) = 0;
        virtual void EnableAbortKey(void) = 0;
        virtual bool IsAbortKeyEnabled(void) = 0;
        
        // Color transform
        virtual MCPlatformColorTransformRef CreateColorTransform(void) = 0;
        virtual bool InitializeColorTransform(void) = 0;
        virtual void FinalizeColorTransform(void) = 0;
        
        // Menus
        virtual MCPlatformMenuRef CreateMenu(void) = 0;
        virtual bool InitializeMenu(void) = 0;
        virtual void FinalizeMenu(void) = 0;
        virtual void ShowMenubar(void) = 0;
        virtual void HideMenubar(void) = 0;
        virtual void SetMenubar(MCPlatformMenuRef p_menu) = 0;
        virtual MCPlatformMenuRef GetMenubar(void) = 0;
        virtual void SetIconMenu(MCPlatformMenuRef p_menu) = 0;
        virtual void SaveQuittingState() = 0;
        virtual void PopQuittingState() = 0;
        virtual bool IsInQuittingState(void) = 0;
        virtual void LockMenuSelect(void) = 0;
        virtual void UnlockMenuSelect(void) = 0;
        
        // Dialogs
        virtual void ShowMessageDialog(MCStringRef p_title, MCStringRef p_message) = 0;
        
        // Windows
        virtual MCPlatformWindowRef CreateWindow(void) = 0;
        virtual bool GetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window) = 0;
        
        // Window mask
        virtual MCPlatformWindowMaskRef CreateWindowMask(void) = 0;
        
        // Print dialog
        virtual MCPlatformPrintDialogSessionRef CreatePrintDialogSession(void) = 0;
        
        // Color dialog
        virtual void BeginColorDialog(MCStringRef p_title, const MCColor& p_color) = 0;
        virtual MCPlatformDialogResult EndColorDialog(MCColor& r_color) = 0;
        
        // File & folder dialog
        virtual void BeginFileDialog(MCPlatformFileDialogKind p_kind, MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef *p_types, uint4 p_type_count, MCStringRef p_initial_folder, MCStringRef p_initial_file) = 0;
        virtual MCPlatformDialogResult EndFileDialog(MCPlatformFileDialogKind p_kind, MCStringRef &r_paths, MCStringRef &r_type) = 0;
        virtual void BeginFolderDialog(MCPlatformWindowRef p_owner, MCStringRef p_title, MCStringRef p_prompt, MCStringRef p_initial) = 0;
        virtual MCPlatformDialogResult EndFolderDialog(MCStringRef& r_selected_folder) = 0;
        
        // System Properties
        virtual void GetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value) = 0;
        virtual void SetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value) = 0;
        
        // Player
        virtual MCPlatformPlayerRef CreatePlayer(void) = 0;
        
        // Snapshots
        virtual void ScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshotOfWindow(uint32_t p_window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshotOfWindowArea(uint32_t p_window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        virtual void ScreenSnapshot(MCRectangle p_screen_rect, MCPoint *p_size, MCImageBitmap*& r_bitmap) = 0;
        
        virtual void Beep(void) = 0;
        
        virtual void DeathGrip(MCPlatform::Base * p_window) = 0;
        
        // Events
        virtual void FlushEvents(MCPlatformEventMask p_mask) = 0;
        virtual uint32_t GetEventTime(void) = 0;
        
        // Mice
        virtual MCPlatformCursorRef CreateCursor(void) = 0;
        virtual void HideCursorUntilMouseMoves(void) = 0;
        virtual bool GetMouseButtonState(uindex_t p_button) = 0;
        virtual bool GetMouseClick(uindex_t p_button, MCPoint& r_location) = 0;
        virtual void GetMousePosition(MCPoint& r_location) = 0;
        virtual void SetMousePosition(MCPoint p_location) = 0;
        virtual void GetWindowAtPoint(MCPoint p_loc, MCPlatformWindowRef& r_window) = 0;
        virtual void GrabPointer(MCPlatformWindowRef p_window) = 0;
        virtual void UngrabPointer(void) = 0;
        virtual void ClearLastMouseEvent(void) = 0;
        
        // Modifier Keys
        virtual MCPlatformModifiers GetModifiersState(void) = 0;
        virtual bool GetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count) = 0;
        
        // Drag and drop
        virtual void DoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation) = 0;
        virtual bool ConvertTIFFToPNG(MCDataRef p_in_data, MCDataRef& r_out_data) = 0;
        
        // Screens
        virtual void GetScreenCount(uindex_t& r_count) = 0;
        virtual void GetScreenViewport(uindex_t p_index, MCRectangle& r_viewport) = 0;
        virtual void GetScreenWorkarea(uindex_t p_index, MCRectangle& r_workarea) = 0;
        virtual void GetScreenPixelScale(uindex_t p_index, MCGFloat& r_scale) = 0;
        virtual void DisableScreenUpdates(void) = 0;
        virtual void EnableScreenUpdates(void) = 0;
        
        // Backdrop
        virtual void ConfigureBackdrop(MCPlatformWindowRef p_backdrop_window) = 0;
        
        // Themes
        virtual bool GetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool) = 0;
        virtual bool GetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int) = 0;
        virtual bool GetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color) = 0;
        virtual bool GetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font) = 0;
        virtual bool GetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string) = 0;
        virtual bool DrawTheme(MCGContextRef p_context, MCThemeDrawType p_type, MCThemeDrawInfo *p_info_ptr) = 0;
        virtual bool LoadTheme(void) = 0;
        virtual uint16_t GetThemeId(void) = 0;
        virtual uint16_t GetThemeFamilyId(void) = 0;
        virtual bool IsThemeWidgetSupported(Widget_Type wtype) = 0;
        virtual int32_t GetThemeMetric(Widget_Metric wmetric) = 0;
        virtual int32_t GetThemeWidgetMetric(const MCWidgetInfo &winfo,Widget_Metric wmetric) = 0;
        virtual void GetThemeWidgetRect(const MCWidgetInfo &winfo, Widget_Metric wmetric, const MCRectangle &srect, MCRectangle &drect) = 0;
        virtual bool GetThemePropBool(Widget_ThemeProps themeprop) = 0;
        virtual bool DrawThemeWidget(MCDC *dc, const MCWidgetInfo &winfo, const MCRectangle &d) = 0;
        virtual Widget_Part HitTestTheme(const MCWidgetInfo &winfo, int2 mx, int2 my, const MCRectangle &drect) = 0;
        virtual void UnloadTheme(void) = 0;
        virtual bool DrawThemeFocusBorder(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect) = 0;
        virtual bool DrawThemeMetalBackground(MCContext *p_context, const MCRectangle& p_dirty, const MCRectangle& p_rect, MCPlatformWindowRef p_window) = 0;
        
        // Script Environment
        virtual MCPlatformScriptEnvironmentRef CreateScriptEnvironment(void) = 0;
        
        // Fonts
        virtual MCPlatformLoadedFontRef CreateLoadedFont(void) = 0;
        
        // Sound
        virtual MCPlatformSoundRef CreateSound(void) = 0;
        
        // Native layer
        virtual MCPlatformNativeLayerRef CreateNativeLayer(void) = 0;
        virtual bool CreateNativeContainer(void *&r_view) = 0;
        virtual void ReleaseNativeView(void *p_view) = 0;
        
        // AppleEvent
        virtual void Send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean p_reply, MCStringRef &r_result) = 0;
        virtual void Reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error) = 0;
        virtual void RequestAE(MCStringRef p_message, uint16_t p_ae, MCStringRef& r_value) = 0;
        virtual bool RequestProgram(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_value, MCStringRef& r_result) = 0;
        
        // Clipboard
        virtual MCRawClipboard* CreateSystemClipboard() = 0;
        virtual MCRawClipboard* CreateSystemSelectionClipboard() = 0;
        virtual MCRawClipboard* CreateSystemDragboard() = 0;
    };
    
#ifndef PLATFORM_IS_MINIMAL
    typedef Ref<Core> CoreRef;
#endif
    
} /* namespace MCPlatform */

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCPlatformAutoValueRefBase;

template<typename T>
T In(const MCPlatformAutoValueRefBase<T>& p_auto)
{
    return p_auto . In();
}

template<typename T>
T& Out(MCPlatformAutoValueRefBase<T>& p_auto)
{
    return p_auto . Out();
}

template<typename T>
T& InOut(MCPlatformAutoValueRefBase<T>& p_auto)
{
    return p_auto . InOut();
}

template<typename T> class MCPlatformAutoValueRefBase :  public MCPlatform::Stubs
{
public:
    
    inline MCPlatformAutoValueRefBase(MCPlatformCallbackRef p_callback)
    : m_value(nil)
    {
        m_callback = p_callback;
    }
    
    inline MCPlatformAutoValueRefBase(const MCPlatformAutoValueRefBase& other) :
    m_value(nil)
    {
        m_callback = other.m_callback;
        Reset(other.m_value);
    }
    
    MCPlatformAutoValueRefBase(MCPlatformAutoValueRefBase&& other)
    : m_value(other.Take())
    {
        m_callback = other.m_callback;
    }
    
    inline MCPlatformAutoValueRefBase(T p_value, MCPlatformCallbackRef p_callback)
    : m_value(nil)
    {
        m_callback = p_callback;
        
        if (p_value)
            m_value = MCValueRetain(p_value);
    }
    
    inline ~MCPlatformAutoValueRefBase(void)
    {
        MCValueRelease(m_value);
    }
    
    inline T operator = (T value)
    {
        MCAssert(m_value == nil);
        m_value = (T)MCValueRetain(value);
        return value;
    }
    
    MCPlatformAutoValueRefBase& operator=(MCPlatformAutoValueRefBase&& other)
    {
        Give(other.Take());
        return *this;
    }
    
    inline T& operator & (void)
    {
        MCAssert(m_value == nil);
        return m_value;
    }
    
    inline T operator * (void) const
    {
        return m_value;
    }
    
    bool IsSet() const
    {
        return m_value != nil;
    }
    
    void Reset(T value = nil)
    {
        if (value == m_value)
            return;
        
        if (m_value)
            MCValueRelease(m_value);
        m_value = (value) ? (T)MCValueRetain(value) : NULL;
    }
    
    // The give method places the given value into the container without
    // retaining it - the auto container is considered to now own the value.
    inline void Give(T value)
    {
        if (m_value)
            MCValueRelease(m_value);
        m_value = value;
    }
    
    // The take method removes the value from the container passing ownership
    // to the caller.
    inline T Take(void)
    {
        T t_value;
        t_value = m_value;
        m_value = nil;
        return t_value;
    }
    
    friend T In<>(const MCPlatformAutoValueRefBase<T>&);
    friend T& Out<>(MCPlatformAutoValueRefBase<T>&);
    friend T& InOut<>(MCPlatformAutoValueRefBase<T>&);
    
protected:
    T m_value = nullptr;
    
    // Return the contents of the auto pointer in a form for an in parameter.
    T In(void) const
    {
        return m_value;
    }
    
    // Return the contents of the auto pointer in a form for an out parameter.
    T& Out(void)
    {
        MCAssert(m_value == nil);
        return m_value;
    }
    
    // Return the contents of the auto pointer in a form for an inout parameter.
    T& InOut(void)
    {
        return m_value;
    }
    
private:
    MCPlatformAutoValueRefBase<T>& operator = (MCPlatformAutoValueRefBase<T>& x);
};

template<typename T, bool (*MutableCopyAndRelease)(T, T&), bool (*ImmutableCopyAndRelease)(T, T&)>
class MCPlatformAutoMutableValueRefBase :
public MCPlatformAutoValueRefBase<T>
{
public:
    constexpr MCPlatformAutoMutableValueRefBase(MCPlatformCallbackRef p_callback) :
    MCPlatformAutoValueRefBase<T>(p_callback)
    {
    }
    
    inline explicit MCPlatformAutoMutableValueRefBase(T p_value, MCPlatformCallbackRef p_callback) :
    MCPlatformAutoValueRefBase<T>(p_value, p_callback)
    {
    }
    
    inline T operator = (T value)
    {
        return MCPlatformAutoValueRefBase<T>::operator =(value);
    }
    
    inline bool MakeMutable(void)
    {
        return MutableCopyAndRelease(MCPlatformAutoValueRefBase<T>::m_value, MCPlatformAutoValueRefBase<T>::m_value);
    }
    
    inline bool MakeImmutable(void)
    {
        return ImmutableCopyAndRelease(MCPlatformAutoValueRefBase<T>::m_value, MCPlatformAutoValueRefBase<T>::m_value);
    }
    
private:
    MCPlatformAutoMutableValueRefBase<T, MutableCopyAndRelease, ImmutableCopyAndRelease>& operator = (MCPlatformAutoMutableValueRefBase<T, MutableCopyAndRelease, ImmutableCopyAndRelease>& x);
};

typedef MCPlatformAutoValueRefBase<MCValueRef> MCPlatformAutoValueRef;
typedef MCPlatformAutoValueRefBase<MCNumberRef> MCPlatformAutoNumberRef;
typedef MCPlatformAutoMutableValueRefBase<MCStringRef, MCStringMutableCopyAndRelease, MCStringCopyAndRelease> MCPlatformAutoStringRef;
typedef MCPlatformAutoMutableValueRefBase<MCArrayRef, MCArrayMutableCopyAndRelease, MCArrayCopyAndRelease> MCPlatformAutoArrayRef;
typedef MCPlatformAutoValueRefBase<MCListRef> MCPlatformAutoListRef;
typedef MCPlatformAutoValueRefBase<MCBooleanRef> MCPlatformAutoBooleanRef;
typedef MCPlatformAutoMutableValueRefBase<MCSetRef, MCSetMutableCopyAndRelease, MCSetCopyAndRelease> MCPlatformAutoSetRef;
typedef MCPlatformAutoValueRefBase<MCNameRef> MCPlatformNewAutoNameRef;
typedef MCPlatformAutoMutableValueRefBase<MCDataRef, MCDataMutableCopyAndRelease, MCDataCopyAndRelease> MCPlatformAutoDataRef;
typedef MCPlatformAutoValueRefBase<MCTypeInfoRef> MCPlatformAutoTypeInfoRef;
typedef MCPlatformAutoMutableValueRefBase<MCRecordRef, MCRecordMutableCopyAndRelease, MCRecordCopyAndRelease> MCPlatformAutoRecordRef;
typedef MCPlatformAutoValueRefBase<MCErrorRef> MCPlatformAutoErrorRef;
typedef MCPlatformAutoMutableValueRefBase<MCProperListRef, MCProperListMutableCopyAndRelease, MCProperListCopyAndRelease> MCPlatformAutoProperListRef;

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCPlatformAutoValueRefArrayBase :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoValueRefArrayBase(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoValueRefArrayBase(void)
    {
        Reset();
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_values == nil);
        return MCMemoryNewArray(p_size, m_values, m_value_count);
    }
    
    //////////
    
    void Give(T* p_array, uindex_t p_size)
    {
        MCAssert(m_values == nil);
        m_values = p_array;
        m_value_count = p_size;
    }
    
    void Take(T*& r_array, uindex_t& r_count)
    {
        r_array = m_values;
        r_count = m_value_count;
        
        m_values = nil;
        m_value_count = 0;
    }
    
    /* Take the contents of the array as an immutable MCProperList.
     * The contents of the array will no longer be accessible. */
    bool TakeAsProperList (MCProperListRef& r_list)
    {
        MCProperListRef t_list;
        if (!MCProperListCreateAndRelease ((MCValueRef *) m_values,
                                           m_value_count,
                                           t_list))
            return false;
        
        m_values = nil;
        m_value_count = 0;
        
        r_list = t_list;
        return true;
    }
    
    /* Reset the managed array by releasing all the values in the
     * array and the underlying array storage. */
    void Reset()
    {
        if (m_values != nil)
        {
            for (uindex_t i = 0; i < m_value_count; i++)
                MCValueRelease(m_values[i]);
            MCMemoryDeleteArray(m_values);
        }
    }
    
    //////////
    
    T* Ptr()
    {
        return m_values;
    }
    
    uindex_t Size()
    {
        return m_value_count;
    }
    
    T*& PtrRef()
    {
        MCAssert(m_values == nil);
        return m_values;
    }
    
    uindex_t& CountRef()
    {
        MCAssert(m_value_count == 0);
        return m_value_count;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        return MCMemoryResizeArray(p_new_size, m_values, m_value_count);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_value_count);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_value_count);
        Resize(p_new_size);
    }
    
    //////////
    
    bool Append(MCPlatformAutoValueRefArrayBase<T> &p_array)
    {
        uindex_t t_index = Count();
        if (!Extend(t_index + p_array.Count()))
            return false;
        
        for (uindex_t i = 0; i < p_array.m_value_count; i++)
            m_values[t_index + i] = MCValueRetain(p_array.m_values[i]);
        
        return true;
    }
    
    bool Push(T p_value)
    {
        if (!Extend(m_value_count + 1))
            return false;
        m_values[m_value_count - 1] = MCValueRetain(p_value);
        return true;
    }
    
    //////////
    
    uindex_t Count(void)
    {
        return m_value_count;
    }
    
    //////////
    
    T* operator * (void) const
    {
        return m_values;
    }
    
    T& operator [] (const uindex_t p_index)
    {
        MCAssert(m_values != nil);
        return m_values[p_index];
    }
    
    MCSpan<T> Span() const
    {
        return MCMakeSpan(m_values, m_value_count);
    }
    
private:
    T* m_values = nullptr;
    uindex_t m_value_count = 0;
};

typedef MCPlatformAutoValueRefArrayBase<MCValueRef> MCPlatformAutoValueRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCNumberRef> MCPlatformAutoNumberRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCStringRef> MCPlatformAutoStringRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCDataRef> MCPlatformAutoDataRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCArrayRef> MCPlatformAutoArrayRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCListRef> MCPlatformAutoListRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCBooleanRef> MCPlatformAutoBooleanRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCSetRef> MCPlatformAutoSetRefArray;
typedef MCPlatformAutoValueRefArrayBase<MCNameRef> MCPlatformAutoNameRefArray;

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsUTF16String :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsUTF16String(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringUnicodeCopy(p_string, &m_string);
    }
    void Unlock()
    {
        m_string.Reset();
    }
    const unichar_t *Ptr() const
    {
        MCAssert(!MCStringIsNative(*m_string));
        return MCStringGetCharPtr(*m_string);
    }
    uindex_t Size() const
    {
        MCAssert(!MCStringIsNative(*m_string));
        return MCStringGetLength(*m_string);
    }
    const unichar_t *operator*() const
    {
        return Ptr();
    }
private:
    MCPlatformAutoStringRef m_string = nil;
};

typedef MCPlatformAutoStringRefAsUTF16String MCPlatformAutoStringRefAsWString;

////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS__)
typedef MCPlatformAutoStringRefAsUTF16String MCPlatformAutoStringRefAsLPCWSTR;

/* Always ensures that its internal string buffer is always owned by
 * the auto class, so that it's safe to pass to Windows API functions
 * that take an LPWSTR rather than an LPCWSTR. */
class MCPlatformAutoStringRefAsLPWSTR :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsLPWSTR(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoStringRefAsLPWSTR()
    {
        Unlock();
    }
    bool Lock(MCStringRef p_string)
    {
        MCAssert(nil == m_buffer);
        m_size = MCStringGetLength(p_string);
        return MCStringConvertToWString(p_string, m_buffer);
    }
    void Unlock()
    {
        MCMemoryDeleteArray(m_buffer);
        m_buffer = nil;
    }
    unichar_t *Ptr()
    {
        MCAssert(nil != m_buffer);
        return m_buffer;
    }
    uindex_t Size() const
    {
        MCAssert(nil != m_buffer);
        return m_size;
    }
    unichar_t *operator*()
    {
        return Ptr();
    }
private:
    unichar_t *m_buffer = nullptr;
    uindex_t m_size = 0;
};
#endif /* __WINDOWS__ */

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsUTF8String :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsUTF8String(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoStringRefAsUTF8String(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToUTF8(p_string, m_utf8string, m_size);
    }
    
    void Unlock(void)
    {
        MCMemoryDeleteArray(m_utf8string);
        m_utf8string = nil;
    }
    
    const char *operator * (void) const
    {
        return m_utf8string;
    }
    
    uindex_t Size()
    {
        return m_size;
    }
    
private:
    char *m_utf8string = nullptr;
    uindex_t m_size = 0;
};

////////////////////////////////////////////////////////////////////////////////

#if defined(__MAC__) || defined(__IOS__)
#include <CoreFoundation/CoreFoundation.h>
class MCPlatformAutoStringRefAsCFString :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsCFString(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoStringRefAsCFString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToCFStringRef(p_string, m_cfstring);
    }
    
    void Unlock(void)
    {
        if (m_cfstring != nil)
            CFRelease(m_cfstring);
        m_cfstring = nil;
    }
    
    CFStringRef operator * (void)
    {
        return m_cfstring;
    }
    
private:
    CFStringRef m_cfstring = nullptr;
};

#endif

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsPascalString :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsPascalString(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoStringRefAsPascalString(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        // A Pascal-style counted string can only contain a maximum of 255 chars
        // so the string length has to be clamped to that.
        uindex_t t_length = MCStringGetLength(p_string);
        if (t_length > 255)
            return false;
        
        // Allocate a buffer, adding an extra char for the count byte
        MCMemoryNewArray(t_length + 1, m_pascal_string);
        
        // Copy the string to the buffer, leaving a byte at the beginning for
        // the count.
        MCStringGetNativeChars(p_string, MCRangeMake(0, t_length), m_pascal_string+1);
        
        // Write the count byte
        *m_pascal_string = uint8_t(t_length);
        return true;
    }
    
    void Unlock(void)
    {
        MCMemoryDeleteArray(m_pascal_string);
        m_pascal_string = nil;
    }
    
    const unsigned char* operator * (void) const
    {
        return m_pascal_string;
    }
    
private:
    unsigned char *m_pascal_string = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsSysString :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsSysString(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }

    ~MCPlatformAutoStringRefAsSysString()
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        MCAssert(m_bytes == nil);
        return MCStringConvertToSysString(p_string, m_bytes, m_byte_count);
    }
    
    void Unlock()
    {
        if (m_bytes != nil)
        {
            free(m_bytes);
            m_bytes = nil;
            m_byte_count = 0;
        }
    }
    
    const char *operator * () const
    {
        return Ptr();
    }
    
    const char *Ptr(void) const
    {
        MCAssert(m_bytes != nil);
        return m_bytes;
    }
    
    size_t Size(void) const
    {
        return m_byte_count;
    }
    
private:
    char *m_bytes = nullptr;
    size_t m_byte_count = 0;
};

////////////////////////////////////////////////////////////////////////////////

#if 0
#ifdef __WINDOWS__
#include <WTypes.h>
class MCPlatformAutoStringRefAsBSTR :  public MCPlatform::Stubs
{
public:
    constexpr MCPlatformAutoStringRefAsBSTR(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoStringRefAsBSTR(void)
    {
        Unlock();
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringConvertToBSTR(p_string, m_bstr);
    }
    
    void Unlock(void)
    {
        SysFreeString(m_bstr);
        m_bstr = nil;
    }
    
    BSTR operator * (void)
    {
        return m_bstr;
    }
    
private:
    BSTR m_bstr = nullptr;
};
#endif // __WINDOWS__
#endif

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsNativeChars :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoStringRefAsNativeChars(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    bool Lock(MCStringRef p_string)
    {
        return MCStringNativeCopy(p_string, &m_string);
    }
    bool Lock(MCStringRef p_string,
              const char_t * & r_buffer,
              uindex_t & r_length)
    {
        if (!Lock(p_string))
            return false;
        r_buffer = MCStringGetNativeCharPtrAndLength(*m_string,
                                                     r_length);
        return true;
    }
    void Unlock()
    {
        m_string.Reset();
    }
    operator bool () const
    {
        return m_string.IsSet();
    }
    const char_t * operator*() const
    {
        MCAssert(MCStringIsNative(*m_string));
        return MCStringGetNativeCharPtr(*m_string);
    }
    uindex_t Size() const
    {
        MCAssert(MCStringIsNative(*m_string));
        uindex_t t_length;
        (void) MCStringGetNativeCharPtrAndLength(*m_string, t_length);
        return t_length;
    }
    MCSpan<const char_t> Span() const
    {
        return MCMakeSpan(operator*(), Size());
    }
private:
    MCPlatformAutoStringRef m_string = nil;
};

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoStringRefAsCString : public MCPlatformAutoStringRefAsNativeChars
{
public:
    const char * operator* () const
    {
        return reinterpret_cast<const char *>(MCPlatformAutoStringRefAsNativeChars::operator*());
    }
    MCSpan<const char> Span() const
    {
        return MCMakeSpan(operator*(), Size());
    }
};

////////////////////////////////////////////////////////////////////////////////

/* This version of MCPlatformAutoPointer should be used when you need to
 * manage a value created with the "new" operator.  For example:
 *
 *     MCPlatformAutoPointer<int> number;
 *     number = new (nothrow) int;
 */
template<typename T> class MCPlatformAutoPointer :  public MCPlatform::Stubs
{
public:
    typedef T* pointer;
    
    MCPlatformAutoPointer(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    constexpr MCPlatformAutoPointer(decltype(nullptr)) {}
    
    /* Construct the managed pointer using a specific value. */
    MCPlatformAutoPointer(pointer p, MCPlatformCallbackRef p_callback) : m_ptr(p)
    {
        m_callback = p_callback;
    }
    
    /* Construct managed pointer by moving a pointer from another
     * managed pointer of the same type. */
    MCPlatformAutoPointer(MCPlatformAutoPointer&& other)
    {
        m_callback = other.m_callback;
        m_ptr = other.Release();
    }
    
    ~MCPlatformAutoPointer() { Reset(); }
    
    /* Destroy the managed pointer, and take ownership of the value
     * provided.  Providing no value results in the managed pointer
     * being cleared after calling Reset(). */
    void Reset(pointer value = nullptr)
    {
        delete m_ptr;
        m_ptr = value;
    }
    
    /* Relinquish ownership of the managed pointer.  The MCPlatformAutoPointer is
     * null after Release() is called. */
    pointer Release() ATTRIBUTE_UNUSED
    {
        pointer t_result = m_ptr;
        m_ptr = nullptr;
        return t_result;
    }
    
    /* Test whether the MCPlatformAutoPointer has been set. */
    operator bool() { return m_ptr != nullptr; }
    
    /* Return the pointer managed by the MCPlatformAutoPointer, or nullptr if
     * there is no managed pointer. */
    pointer Get() { return m_ptr; }
    
    MCPlatformAutoPointer& operator=(MCPlatformAutoPointer&& other)
    {
        Reset(other.Release());
        return *this;
    }
    
    T*& operator & (void)
    {
        MCAssert(m_ptr == nil);
        return m_ptr;
    }
    
    T* operator -> (void)
    {
        MCAssert(m_ptr != nil);
        return m_ptr;
    }
    
    T *operator * (void) const
    {
        return m_ptr;
    }
    
    void Take(T*&r_ptr)
    {
        r_ptr = Release();
    }
    
private:
    T *m_ptr = nullptr;
};

/* This version of MCPlatformAutoPointer should be used when you need to
 * manage a value created with the "new[]" operator.  For example:
 *
 *     MCPlatformAutoPointer<int[]> numbers;
 *     numbers = new (nothrow) int[25];
 */
template<typename T> class MCPlatformAutoPointer<T[]> :  public MCPlatform::Stubs
{
public:
    typedef T* pointer;
    
    MCPlatformAutoPointer(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    constexpr MCPlatformAutoPointer(decltype(nullptr)) {}
    
    /* Construct the managed pointer using a specific value. */
    MCPlatformAutoPointer(pointer p, MCPlatformCallbackRef p_callback) : m_ptr(p)
    {
        m_callback = p_callback;
    }
    
    /* Construct managed pointer by moving a pointer from another
     * managed pointer of the same type. */
    MCPlatformAutoPointer(MCPlatformAutoPointer&& other, MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
        m_ptr = other.Release();
    }
    
    ~MCPlatformAutoPointer() { Reset(); }
    
    /* Destroy the managed pointer, and take ownership of the value
     * provided.  Providing no value results in the managed pointer
     * being cleared after calling Reset(). */
    void Reset(pointer value = nullptr)
    {
        delete[] m_ptr;
        m_ptr = value;
    }
    
    /* Relinquish ownership of the managed pointer.  The MCPlatformAutoPointer is
     * null after Release() is called. */
    pointer Release() ATTRIBUTE_UNUSED
    {
    pointer t_result = m_ptr;
    m_ptr = nullptr;
    return t_result;
}

/* Test whether the MCPlatformAutoPointer has been set. */
operator bool() { return m_ptr != nullptr; }

/* Return the pointer managed by the MCPlatformAutoPointer, or nullptr if
 * there is no managed pointer. */
pointer Get() { return m_ptr; }

MCPlatformAutoPointer& operator=(MCPlatformAutoPointer&& other)
{
    Reset(other.Release());
    return *this;
}

T*& operator & (void)
{
    MCAssert(m_ptr != nil);
    return m_ptr;
}

T* operator -> (void)
{
    MCAssert(m_ptr != nil);
    return m_ptr;
}

T* operator * (void)
{
    return m_ptr;
}

void Take(T* & r_ptr)
{
    r_ptr = Release();
}

T& operator [] (size_t x)
{
    return m_ptr[x];
}

private:
T *m_ptr = nullptr;
};

template <typename T, void (Deleter)(T*)>
class MCPlatformAutoCustomPointer :  public MCPlatform::Stubs
{
public:
    
    typedef T* pointer;
    
    MCPlatformAutoCustomPointer(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }

    MCPlatformAutoCustomPointer(T*&& take, MCPlatformCallbackRef p_callback) : m_ptr(take)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoCustomPointer()
    {
        Deleter(m_ptr);
    }
    
    MCPlatformAutoCustomPointer& operator= (T*&& value)
    {
        Deleter(m_ptr);
        m_ptr = value;
        return *this;
    }
    
    T*& operator& ()
    {
        MCAssert(m_ptr == nil);
        return m_ptr;
    }
    
    T* operator* () const
    {
        return m_ptr;
    }
    
    T* operator -> (void)
    {
        MCAssert(m_ptr != nullptr);
        return m_ptr;
    }
    
    operator bool () const
    {
        return m_ptr != nil;
    }
    
    void ReleaseTo(T*& r_ptr)
    {
        r_ptr = m_ptr;
        m_ptr = nil;
    }
    
    T* Release()
    {
        T* t_ptr = m_ptr;
        m_ptr = nil;
        return t_ptr;
    }
    
private:
    
    T* m_ptr = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCPlatformAutoBlock :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoBlock(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
        m_ptr = nil;
    }
    
    ~MCPlatformAutoBlock(void)
    {
        MCMemoryDeallocate(m_ptr);
    }
    
    bool Allocate(size_t p_size)
    {
        MCAssert(m_ptr == nil);
        return MCMemoryAllocate(p_size, m_ptr);
    }
    
    bool Reallocate(size_t p_new_size)
    {
        return MCMemoryReallocate(p_new_size, m_ptr, m_ptr);
    }
    
    void Deallocate(void)
    {
        MCMemoryDeallocate(m_ptr);
        m_ptr = nil;
    }
    
    T *operator * (void)
    {
        return m_ptr;
    }
    
private:
    T *m_ptr;
};

////////////////////////////////////////////////////////////////////////////////

template<typename T> class MCPlatformAutoArray :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoArray(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoArray(void)
    {
        MCMemoryDeleteArray(m_ptr);
    }
    
    //////////
    
    T* Ptr()
    {
        return m_ptr;
    }
    
    uindex_t Size() const
    {
        return m_size;
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_ptr == nil);
        return MCMemoryNewArray(p_size, m_ptr, m_size);
    }
    
    void Delete(void)
    {
        MCMemoryDeleteArray(m_ptr);
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        return MCMemoryResizeArray(p_new_size, m_ptr, m_size);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_size);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_size);
        Resize(p_new_size);
    }
    
    //////////
    
    bool Push(T p_value)
    {
        if (!Extend(m_size + 1))
            return false;
        m_ptr[m_size - 1] = p_value;
        return true;
    }
    
    //////////
    
    T*& PtrRef()
    {
        MCAssert(m_ptr == nil);
        return m_ptr;
    }
    
    uindex_t& SizeRef()
    {
        MCAssert(m_size == 0);
        return m_size;
    }
    
    //////////
    
    void Take(T*& r_array, uindex_t& r_count)
    {
        r_array = m_ptr;
        r_count = m_size;
        
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    T& operator [] (uindex_t p_index)
    {
        return m_ptr[p_index];
    }
    
    const T& operator [] (uindex_t p_index) const
    {
        return m_ptr[p_index];
    }
    
private:
    T *m_ptr = nullptr;
    uindex_t m_size = 0;
};

// Version of MCPlatformAutoArray that applies the provided deallocator to each element of the array when freed
template <typename T, void (*FREE)(T)> class MCPlatformAutoCustomPointerArray :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoCustomPointerArray(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoCustomPointerArray(void)
    {
        FreeElements(MCRangeMake(0, m_size));
        
        MCMemoryDeleteArray(m_ptr);
    }
    
    //////////
    
    T* Ptr()
    {
        return m_ptr;
    }
    
    uindex_t Size() const
    {
        return m_size;
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_ptr == nil);
        return MCMemoryNewArray(p_size, m_ptr, m_size);
    }
    
    void Delete(void)
    {
        FreeElements(MCRangeMake(0, m_size));
        
        MCMemoryDeleteArray(m_ptr);
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        if (p_new_size < m_size)
            FreeElements(MCRangeMake(p_new_size, m_size - p_new_size));
        
        return MCMemoryResizeArray(p_new_size, m_ptr, m_size);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_size);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_size);
        Resize(p_new_size);
    }
    
    //////////
    
    bool Push(T p_value)
    {
        if (!Extend(m_size + 1))
            return false;
        m_ptr[m_size - 1] = p_value;
        return true;
    }
    
    //////////
    
    T*& PtrRef()
    {
        MCAssert(m_ptr == nil);
        return m_ptr;
    }
    
    uindex_t& SizeRef()
    {
        MCAssert(m_size == 0);
        return m_size;
    }
    
    //////////
    
    void Take(T*& r_array, uindex_t& r_count)
    {
        r_array = m_ptr;
        r_count = m_size;
        
        m_ptr = nil;
        m_size = 0;
    }
    
    //////////
    
    T& operator [] (uindex_t p_index)
    {
        return m_ptr[p_index];
    }
    
    const T& operator [] (uindex_t p_index) const
    {
        return m_ptr[p_index];
    }
    
private:
    T *m_ptr = nullptr;
    uindex_t m_size = 0;
    
    void FreeElements(const MCRange &p_elements)
    {
        uindex_t t_end;
        t_end = (uindex_t)MCMin(m_size, p_elements.offset + p_elements.length);
        
        for (uindex_t i = p_elements.offset; i < t_end; i++)
        {
            if (m_ptr[i] != nil)
            {
                FREE(m_ptr[i]);
                m_ptr[i] = nil;
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class MCPlatformAutoNativeCharArray :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoNativeCharArray(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoNativeCharArray(void)
    {
        if (m_chars != nil)
            MCMemoryDeleteArray(m_chars);
    }
    
    //////////
    
    char_t *Chars(void) const
    {
        return m_chars;
    }
    
    size_t CharCount(void) const
    {
        return m_char_count;
    }
    
    //////////
    
    void Give(char_t *p_chars, uindex_t p_char_count)
    {
        MCAssert(m_chars == nil);
        m_chars = p_chars;
        m_char_count = p_char_count;
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_chars == nil);
        return MCMemoryNewArray(p_size, m_chars, m_char_count);
    }
    
    void Delete(void)
    {
        MCMemoryDeleteArray(m_chars);
        m_chars = nil;
        m_char_count = 0;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        return MCMemoryResizeArray(p_new_size, m_chars, m_char_count);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_char_count);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_char_count);
        Resize(p_new_size);
    }
    
    //////////
    
    bool CreateString(MCStringRef& r_string)
    {
        return MCStringCreateWithNativeChars(m_chars, m_char_count, r_string);
    }
    
    bool CreateStringAndRelease(MCStringRef& r_string)
    {
        if (MCStringCreateWithNativeChars(m_chars, m_char_count, r_string))
        {
            MCMemoryDeleteArray(m_chars);
            m_chars = nil;
            m_char_count = 0;
            return true;
        }
        return false;
    }
    
private:
    char_t *m_chars = nullptr;
    uindex_t m_char_count = 0;
};

class MCPlatformAutoByteArray :  public MCPlatform::Stubs
{
public:
    MCPlatformAutoByteArray(MCPlatformCallbackRef p_callback)
    {
        m_callback = p_callback;
    }
    
    ~MCPlatformAutoByteArray(void)
    {
        MCMemoryDeleteArray(m_bytes);
    }
    
    //////////
    
    byte_t *Bytes(void) const
    {
        return m_bytes;
    }
    
    size_t ByteCount(void) const
    {
        return m_byte_count;
    }
    
    //////////
    
    void Give(byte_t *p_bytes, uindex_t p_byte_count)
    {
        MCAssert(m_bytes == nil);
        m_bytes = p_bytes;
        m_byte_count = p_byte_count;
    }
    
    //////////
    
    bool New(uindex_t p_size)
    {
        MCAssert(m_bytes == nil);
        return MCMemoryNewArray(p_size, m_bytes, m_byte_count);
    }
    
    void Delete(void)
    {
        MCMemoryDeleteArray(m_bytes);
        m_bytes = nil;
        m_byte_count = 0;
    }
    
    //////////
    
    bool Resize(uindex_t p_new_size)
    {
        return MCMemoryResizeArray(p_new_size, m_bytes, m_byte_count);
    }
    
    bool Extend(uindex_t p_new_size)
    {
        MCAssert(p_new_size >= m_byte_count);
        return Resize(p_new_size);
    }
    
    void Shrink(uindex_t p_new_size)
    {
        MCAssert(p_new_size <= m_byte_count);
        Resize(p_new_size);
    }
    
    //////////
    
    bool CreateData(MCDataRef& r_data)
    {
        return MCDataCreateWithBytes(m_bytes, m_byte_count, r_data);
    }
    
    bool CreateDataAndRelease(MCDataRef& r_data)
    {
        if (MCDataCreateWithBytesAndRelease(m_bytes, m_byte_count, r_data))
        {
            m_bytes = nil;
            m_byte_count = 0;
            return true;
        }
        return false;
    }
    
private:
    char_t *m_bytes = nullptr;
    uindex_t m_byte_count = 0;
};

////////////////////////////////////////////////////////////////////////////////

#endif
