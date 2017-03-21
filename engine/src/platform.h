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

#ifndef __MC_GLOBDEFS__
#include "globdefs.h"
#endif

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

//enum MCPlatformCallbackType
//{
//	kMCPlatformCallbackApplicationStartup,
//	kMCPlatformCallbackApplicationShutdown,
//	kMCPlatformCallbackApplicationShutdownRequest,
//	kMCPlatformCallbackApplicationSuspend,
//	kMCPlatformCallbackApplicationResume,
//	kMCPlatformCallbackApplicationRun,
//};
//
//struct MCPlatformCallback
//{
//	MCPlatformCallbackType type;
//	union
//	{
//		union
//		{
//			struct
//			{
//				// Incoming fields
//				uindex_t argc;
//				char **argv;
//				char **envp;
//				
//				// Outgoing fields
//				int error_code;
//				char *error_message;
//			} startup;
//			
//			struct
//			{
//				// Incoming fields
//				
//				// Outgoing fields
//				int exit_code;
//			} shutdown;
//			
//			struct
//			{
//				// Incoming fields
//				
//				// Outgoing fields
//				bool terminate;
//			} shutdown_request;
//			
//		} application;
//	};
//};
//
//void MCPlatformProcess(MCPlatformCallback& event);

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

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to match the new stack surface API.
//  You can now lock/unlock multiple areas of the surface, but need to store the context and raster for those areas locally.
class MCPlatformSurface
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

typedef class MCPlatformSurface *MCPlatformSurfaceRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class WindowMask: public CoreReference
    {
    public:
        constexpr WindowMask(void) = default;
        virtual ~WindowMask(void) {}
        
        virtual bool IsValid(void) const = 0;
        
        virtual bool CreateWithAlphaAndRelease(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits) = 0;
    };
    
    typedef Ref<WindowMask> WindowMaskRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::WindowMask MCPlatformWindowMask;
typedef MCPlatform::WindowMask *MCPlatformWindowMaskRef;

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCPlatformWindowAttachmentCallback)(void *object, bool realized);

struct MCPlatformWindowAttachment
{
    void *object;
    MCPlatformWindowAttachmentCallback callback;
};

class MCPlatformWindow: public MCPlatform::CoreReference
{
public:
    MCPlatformWindow(void);
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

typedef class MCPlatformWindow *MCPlatformWindowRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class ColorTransform: public virtual CoreReference
    {
    public:
        virtual bool CreateWithColorSpace(const MCColorSpaceInfo& p_info) = 0;
        virtual bool Apply(MCImageBitmap *p_image) = 0;
    };
    
    typedef Ref<ColorTransform> ColorTransformRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::ColorTransform MCPlatformColorTransform;
typedef class MCPlatform::ColorTransform *MCPlatformColorTransformRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class LoadedFont: public virtual CoreReference
    {
    public:
        virtual bool CreateWithPath(MCStringRef p_path, bool p_globally) = 0;
    };
    
    typedef Ref<LoadedFont> LoadedFontRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::LoadedFont MCPlatformLoadedFont;
typedef MCPlatform::LoadedFont *MCPlatformLoadedFontRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Cursor: public virtual CoreReference
    {
    public:
        virtual void CreateStandard(MCPlatformStandardCursor p_standard_cursor) = 0;
        virtual void CreateCustom(MCImageBitmap *p_image, MCPoint p_hotspot) = 0;
        virtual void Set(void) = 0;
    };
    
    typedef Ref<Cursor> CursorRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::Cursor MCPlatformCursor;
typedef MCPlatform::Cursor *MCPlatformCursorRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Menu: public CoreReference
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

typedef MCPlatform::Menu MCPlatformMenu;
typedef MCPlatform::Menu *MCPlatformMenuRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class PrintDialogSession: public CoreReference
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

typedef MCPlatform::PrintDialogSession MCPlatformPrintDialogSession;
typedef MCPlatform::PrintDialogSession *MCPlatformPrintDialogSessionRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class Sound: public CoreReference
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

typedef MCPlatform::Sound MCPlatformSound;
typedef MCPlatform::Sound *MCPlatformSoundRef;

////////////////////////////////////////////////////////////////////////////////

class MCPlatformPlayer: public MCPlatform::CoreReference
{
public:
    constexpr MCPlatformPlayer(void) = default;
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

typedef class MCPlatformPlayer *MCPlatformPlayerRef;

////////////////////////////////////////////////////////////////////////////////

class MCPlatformSoundRecorder: public MCPlatform::CoreReference
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

typedef class MCPlatformSoundRecorder *MCPlatformSoundRecorderRef;

////////////////////////////////////////////////////////////////////////////////

namespace MCPlatform {
    
    class MinimalCore: public Base
    {
    public:
        // Wait
        virtual bool WaitForEvent(double p_duration, bool p_blocking) = 0;
        virtual void BreakWait(void) = 0;
        
        // Callbacks
        virtual void ScheduleCallback(void (*p_callback)(void *), void *p_context) = 0;
        
        // Player
        virtual MCPlatformPlayerRef CreatePlayer(void) = 0;
        
        // Themes
        virtual bool GetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool) = 0;
        virtual bool GetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int) = 0;
        virtual bool GetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color) = 0;
        virtual bool GetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font) = 0;
        virtual bool GetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string) = 0;
        
        // Callbacks
        virtual MCPlatformCallbackRef GetCallback(void) = 0;
        virtual void SetCallback(MCPlatformCallbackRef p_callback) = 0;
        
        // Platform extensions
        virtual bool QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface) = 0;
        
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        // Apple platforms only
        virtual void RunBlockOnMainFiber(void (^block)(void)) = 0;
#endif
    };
    
    typedef Ref<MinimalCore> MinimalCoreRef;
    
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
        
        // Callbacks
        virtual void ScheduleCallback(void (*p_callback)(void *), void *p_context) = 0;
        
        // Abort key
        virtual bool InitializeAbortKey(void) = 0;
        virtual void FinalizeAbortKey(void) = 0;
        virtual bool GetAbortKeyPressed(void) = 0;
        
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
        
        // Modifier Keys
        virtual MCPlatformModifiers GetModifiersState(void) = 0;
        virtual bool GetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count) = 0;
        
        // Drag and drop
        virtual void DoDragDrop(MCPlatformWindowRef p_window, MCPlatformAllowedDragOperations p_allowed_operations, MCImageBitmap *p_image, const MCPoint *p_image_loc, MCPlatformDragOperation& r_operation) = 0;
        
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
        
        // Callbacks
        virtual MCPlatformCallbackRef GetCallback(void) = 0;
        virtual void SetCallback(MCPlatformCallbackRef p_callback) = 0;
        
        // Platform extensions
        virtual bool QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface) = 0;
        
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)
        // Apple platforms only
        virtual void RunBlockOnMainFiber(void (^block)(void)) = 0;
#endif
    };
    
    typedef Ref<Core> CoreRef;
    
} /* namespace MCPlatform */

#ifdef PLATFORM_IS_MINIMAL
typedef MCPlatform::MinimalCore MCPlatformCore;
typedef MCPlatform::MinimalCore *MCPlatformCoreRef;
#else
typedef MCPlatform::Core MCPlatformCore;
typedef MCPlatform::Core *MCPlatformCoreRef;
#endif

////////////////////////////////////////////////////////////////////////////////


namespace MCPlatform
{
    
    class ScriptEnvironment: public CoreReference
    {
    public:
        virtual bool Define(const char *p_function, MCPlatformScriptEnvironmentCallback p_callback) = 0;
        
        virtual void Run(MCStringRef p_script, MCStringRef &r_result) = 0;
        
        virtual char *Call(const char *p_method, const char **p_arguments, unsigned int p_argument_count) = 0;
    };
    
    typedef Ref<ScriptEnvironment> ScriptEnvironmentRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::ScriptEnvironment MCPlatformScriptEnvironment;
typedef MCPlatform::ScriptEnvironment *MCPlatformScriptEnvironmentRef;


////////////////////////////////////////////////////////////////////////////////


namespace MCPlatform
{
    
    class NativeLayer : public CoreReference
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

typedef MCPlatform::NativeLayer MCPlatformNativeLayer;
typedef MCPlatform::NativeLayer *MCPlatformNativeLayerRef;

////////////////////////////////////////////////////////////////////////////////


namespace MCPlatform
{
    class Callback: public Base
    {
    public:
        constexpr Callback() = default;
        ~Callback() {}
        
        virtual void SendApplicationStartup(int argc, MCStringRef *argv, MCStringRef *envp, int& r_error_code, MCStringRef& r_error_message);
        virtual void SendApplicationShutdown(int& r_exit_code);
        virtual void SendApplicationShutdownRequest(bool& r_terminate);
        virtual void SendApplicationRun(bool& r_continue);
        virtual void SendApplicationSuspend(void);
        virtual void SendApplicationResume(void);
        
        virtual void SendScreenParametersChanged(void);
        
        virtual void SendWindowCloseRequest(MCPlatformWindowRef window);
        virtual void SendWindowClose(MCPlatformWindowRef window);
        virtual void SendWindowCancel(MCPlatformWindowRef window);
        virtual void SendWindowReshape(MCPlatformWindowRef window, MCRectangle new_content);
        virtual void SendWindowConstrain(MCPlatformWindowRef window, MCPoint proposed_size, MCPoint& r_wanted_size);
        virtual void SendWindowRedraw(MCPlatformWindowRef window, MCPlatformSurfaceRef surface, MCGRegionRef dirty_rgn);
        virtual void SendWindowIconify(MCPlatformWindowRef window);
        virtual void SendWindowUniconify(MCPlatformWindowRef window);
        virtual void SendWindowFocus(MCPlatformWindowRef window);
        virtual void SendWindowUnfocus(MCPlatformWindowRef window);
        
        virtual void SendModifiersChanged(MCPlatformModifiers modifiers);
        
        virtual void SendMouseDown(MCPlatformWindowRef window, uint32_t button, uint32_t count);
        virtual void SendMouseUp(MCPlatformWindowRef window, uint32_t button, uint32_t count);
        virtual void SendMouseDrag(MCPlatformWindowRef window, uint32_t button);
        virtual void SendMouseRelease(MCPlatformWindowRef window, uint32_t button, bool was_menu);
        virtual void SendMouseEnter(MCPlatformWindowRef window);
        virtual void SendMouseLeave(MCPlatformWindowRef window);
        virtual void SendMouseMove(MCPlatformWindowRef window, MCPoint location);
        virtual void SendMouseScroll(MCPlatformWindowRef window, int32_t dx, int32_t dy);
        
        virtual void SendDragEnter(MCPlatformWindowRef window, class MCRawClipboard* p_dragboard, MCPlatformDragOperation& r_operation);
        virtual void SendDragLeave(MCPlatformWindowRef window);
        virtual void SendDragMove(MCPlatformWindowRef window, MCPoint location, MCPlatformDragOperation& r_operation);
        virtual void SendDragDrop(MCPlatformWindowRef window, bool& r_accepted);
        
        virtual void SendRawKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        
        virtual void SendKeyDown(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        virtual void SendKeyUp(MCPlatformWindowRef window, MCPlatformKeyCode key_code, codepoint_t mapped_codepoint, codepoint_t unmapped_codepoint);
        
        virtual void SendTextInputQueryTextRanges(MCPlatformWindowRef window, MCRange& r_marked_range, MCRange& r_selected_range);
        virtual void SendTextInputQueryTextIndex(MCPlatformWindowRef window, MCPoint location, uindex_t& r_index);
        virtual void SendTextInputQueryTextRect(MCPlatformWindowRef window, MCRange range, MCRectangle& first_line_rect, MCRange& r_actual_range);
        virtual void SendTextInputQueryText(MCPlatformWindowRef window, MCRange range, unichar_t*& r_chars, uindex_t& r_char_count, MCRange& r_actual_range);
        virtual void SendTextInputInsertText(MCPlatformWindowRef window, unichar_t *chars, uindex_t char_count, MCRange replace_range, MCRange selection_range, bool mark);
        virtual void SendTextInputAction(MCPlatformWindowRef window, MCPlatformTextInputAction action);
        
        virtual void SendMenuUpdate(MCPlatformMenuRef menu);
        virtual void SendMenuSelect(MCPlatformMenuRef menu, uindex_t item);
        
        virtual void SendViewFocusSwitched(MCPlatformWindowRef window, uint32_t view_id);
        
        virtual void SendPlayerFrameChanged(MCPlatformPlayerRef player);
        virtual void SendPlayerMarkerChanged(MCPlatformPlayerRef player, MCPlatformPlayerDuration time);
        virtual void SendPlayerCurrentTimeChanged(MCPlatformPlayerRef player);
        virtual void SendPlayerFinished(MCPlatformPlayerRef player);
        virtual void SendPlayerBufferUpdated(MCPlatformPlayerRef player);
        
        virtual void SendSoundFinished(MCPlatformSoundRef sound);
    };
    
    typedef Ref<Callback> CallbackRef;
    
} /* namespace MCPlatform */

typedef MCPlatform::Callback MCPlatformCallback;
typedef MCPlatform::Callback *MCPlatformCallbackRef;

////////////////////////////////////////////////////////////////////////////////

#endif

