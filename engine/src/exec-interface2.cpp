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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "field.h"
#include "stack.h"
#include "card.h"
#include "button.h"
#include "util.h"
#include "dispatch.h"
#include "stacklst.h"
#include "image.h"
#include "sellst.h"
#include "chunk.h"
#include "date.h"

#include "exec.h"
#include "mode.h"

#include "eps.h"
#include "graphic.h"
#include "group.h"
#include "scrolbar.h"
#include "player.h"
#include "aclip.h"
#include "vclip.h"
#include "osspec.h"

#include "debug.h"
#include "card.h"
#include "cardlst.h"

#include "undolst.h"

#include "redraw.h"
#include "visual.h"
#include "mctheme.h"

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_GET_METHOD(Interface, DialogData, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DialogData, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LookAndFeel, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LookAndFeel, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ScreenMouseLoc, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ScreenMouseLoc, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Backdrop, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Backdrop, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BufferImages, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BufferImages, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SystemFileSelector, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SystemFileSelector, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SystemColorSelector, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SystemColorSelector, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SystemPrintSelector, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SystemPrintSelector, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PaintCompression, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PaintCompression, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BrushBackColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BrushBackColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PenBackColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PenBackColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BrushColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BrushColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PenColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PenColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BrushPattern, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BrushPattern, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PenPattern, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PenPattern, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Filled, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Filled, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PolySides, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PolySides, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LineSize, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LineSize, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RoundRadius, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RoundRadius, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StartAngle, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StartAngle, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ArcAngle, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ArcAngle, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RoundEnds, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RoundEnds, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Dashes, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Dashes, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RecentCards, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RecentNames, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, EditBackground, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, EditBackground, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockScreen, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockScreen, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, AccentColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, AccentColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, HiliteColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, HiliteColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LinkColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LinkColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LinkHiliteColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LinkHiliteColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LinkVisitedColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LinkVisitedColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, UnderlineLinks, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, UnderlineLinks, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SelectGroupedControls, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SelectGroupedControls, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Icon, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Icon, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, AllowInlineInput, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, AllowInlineInput, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DragDelta, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DragDelta, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StackFileType, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StackFileType, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StackFileVersion, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StackFileVersion, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, IconMenu, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, IconMenu, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StatusIcon, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StatusIcon, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StatusIconToolTip, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StatusIconToolTip, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, StatusIconMenu, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, StatusIconMenu, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ProcessType, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ProcessType, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ShowInvisibles, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ShowInvisibles, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Cursor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Cursor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DefaultCursor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DefaultCursor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DefaultStack, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DefaultStack, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DefaultMenubar, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DefaultMenubar, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DragSpeed, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DragSpeed, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, MoveSpeed, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, MoveSpeed, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockCursor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockCursor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockErrors, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockErrors, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockMenus, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockMenus, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockMessages, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockMessages, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockMoves, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockMoves, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, LockRecent, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, LockRecent, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, IdleRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, IdleRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, IdleTicks, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, IdleTicks, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BlinkRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BlinkRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RepeatRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RepeatRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RepeatDelay, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RepeatDelay, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, TypeRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, TypeRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SyncRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SyncRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, EffectRate, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, EffectRate, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DoubleDelta, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DoubleDelta, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DoubleTime, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DoubleTime, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, TooltipDelay, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, TooltipDelay, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, NavigationArrows, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, NavigationArrows, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ExtendKey, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ExtendKey, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, PointerFocus, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, PointerFocus, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, EmacsKeyBindings, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, EmacsKeyBindings, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RaiseMenus, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RaiseMenus, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ActivatePalettes, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ActivatePalettes, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, HidePalettes, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, HidePalettes, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RaisePalettes, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RaisePalettes, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RaiseWindows, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RaiseWindows, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, HideBackdrop, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, HideBackdrop, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, DontUseNavigationServices, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, DontUseNavigationServices, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ProportionalThumbs, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ProportionalThumbs, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SharedMemory, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SharedMemory, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ScreenGamma, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, ScreenGamma, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SelectionMode, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SelectionMode, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, SelectionHandleColor, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, SelectionHandleColor, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, WindowBoundingRect, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, WindowBoundingRect, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, JpegQuality, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, JpegQuality, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, RelayerGroupedControls, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, RelayerGroupedControls, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Brush, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Brush, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Eraser, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Eraser, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Spray, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Spray, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Centered, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Centered, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Grid, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Grid, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, GridSize, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, GridSize, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Slices, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Slices, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BeepLoudness, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BeepLoudness, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BeepPitch, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BeepPitch, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BeepDuration, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BeepDuration, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, BeepSound, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, BeepSound, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, Tool, 1)
MC_EXEC_DEFINE_SET_METHOD(Interface, Tool, 1)
MC_EXEC_DEFINE_GET_METHOD(Interface, ScreenRect, 3)
MC_EXEC_DEFINE_GET_METHOD(Interface, ScreenRects, 3)

MC_EXEC_DEFINE_EVAL_METHOD(Interface, HomeStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, HelpStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedObjectAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, TopStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickFieldAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedFieldAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedImageAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundFieldAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseControlAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FocusedObjectAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, BinaryStackAsObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, DefaultStackAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, StackOfStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, StackOfStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SubstackOfStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SubstackOfStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, AudioClipOfStackByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, AudioClipOfStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, AudioClipOfStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, VideoClipOfStackByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, VideoClipOfStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, VideoClipOfStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, BackgroundOfStackByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, BackgroundOfStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, BackgroundOfStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfStackByOrdinal, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfStackById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfStackByName, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ThisCardOfStack, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfBackgroundByOrdinal, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfBackgroundById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfBackgroundByName, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfCardByOrdinal, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfCardById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfCardByName, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfCardOrStackById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfGroupByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfGroupById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GroupOfGroupByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MenubarAsObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfGroupByOrdinal, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfGroupById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfGroupByName, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfCardByOrdinal, 5)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfCardById, 5)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfCardByName, 5)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ObjectOfCardOrStackById, 5)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, StackByValue, 2)

MC_EXEC_DEFINE_EVAL_METHOD(Interface, HomeStackAsOptionalObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, HelpStackAsOptionalObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, DefaultStackAsOptionalObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, StackOfOptionalStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, StackOfOptionalStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SubstackOfOptionalStackByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SubstackOfOptionalStackById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, OptionalStackWithBackgroundByOrdinal, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, OptionalStackWithBackgroundById, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, OptionalStackWithBackgroundByName, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfOptionalStackByOrdinal, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfOptionalStackById, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CardOfOptionalStackByName, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ThisCardOfOptionalStack, 2)

MC_EXEC_DEFINE_EVAL_METHOD(Interface, TextOfContainer, 2)

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceNamedColorParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceNamedColor& r_output)
{
	if (MCStringIsEmpty(p_input))
	{
		r_output . name = MCValueRetain(kMCEmptyString);
		return;
	}

	MCColor t_color;
	MCStringRef t_color_name;
	t_color_name = nil;
	if (!MCscreen -> parsecolor(p_input, t_color, &t_color_name))
	{
		 ctxt . LegacyThrow(EE_PROPERTY_BADCOLOR);
		 return;
	}
	
	r_output . color = t_color;
	r_output . name = t_color_name;
}

void MCInterfaceNamedColorFormat(MCExecContext& ctxt, const MCInterfaceNamedColor& p_input, MCStringRef& r_output)
{
	if (p_input . name != nil)
	{
		r_output = (MCStringRef)MCValueRetain(p_input . name);
		return;
	}
	
	if (ctxt . FormatLegacyColor(p_input . color, r_output))
		return;
		
	ctxt . Throw();
}

static void MCInterfaceNamedColorInit(MCExecContext& ctxt, MCInterfaceNamedColor& r_output)
{
	MCMemoryClear(&r_output, sizeof(MCInterfaceNamedColor));
}

void MCInterfaceNamedColorFree(MCExecContext& ctxt, MCInterfaceNamedColor& p_input)
{
	MCValueRelease(p_input . name);
}

static void MCInterfaceNamedColorCopy(MCExecContext& ctxt, const MCInterfaceNamedColor& p_source, MCInterfaceNamedColor& r_target)
{
	r_target . color = p_source . color;
	if (p_source . name != nil)
		r_target . name = (MCStringRef)MCValueRetain(p_source . name);
}

static bool MCInterfaceNamedColorIsEqualTo(const MCInterfaceNamedColor& p_left, const MCInterfaceNamedColor& p_right)
{
	if (p_left . name != nil && p_right . name != nil)
		return MCStringIsEqualTo(p_left . name, p_right . name, kMCStringOptionCompareCaseless);

	if (p_left . name != nil || p_right . name != nil)
		return false;

	return p_left . color . red == p_right . color . red &&
			p_left . color . green == p_right . color . green &&
			p_left . color . blue == p_right . color . blue;
}

static MCExecCustomTypeInfo _kMCInterfaceNamedColorTypeInfo =
{
	"Interface.NamedColor",
	sizeof(MCInterfaceNamedColor),
	(void *)MCInterfaceNamedColorParse,
	(void *)MCInterfaceNamedColorFormat,
	(void *)MCInterfaceNamedColorFree
};

//////////

enum MCInterfaceBackdropType
{
	kMCInterfaceBackdropTypeNone,
	kMCInterfaceBackdropTypeColor,
	kMCInterfaceBackdropTypePattern,
};

struct MCInterfaceBackdrop
{
	MCInterfaceBackdropType type;
	union
	{
		MCInterfaceNamedColor named_color;
		uinteger_t pattern;
	};
};

static void MCInterfaceBackdropParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceBackdrop& r_backdrop)
{
	if (MCValueIsEmpty(p_input) ||
		MCStringIsEqualToCString(p_input, "none", kMCCompareCaseless))
	{
		r_backdrop . type = kMCInterfaceBackdropTypeNone;
		return;
	}

	bool t_converted;
	if (!ctxt . TryToConvertToUnsignedInteger(p_input, t_converted, r_backdrop . pattern))
	{
		ctxt . Throw();
		return;
	}

	if (t_converted)
	{
		if (r_backdrop . pattern == 0)
			r_backdrop . type = kMCInterfaceBackdropTypeNone;
		else
			r_backdrop . type = kMCInterfaceBackdropTypePattern;

		return;
	}

	MCInterfaceNamedColorParse(ctxt, p_input, r_backdrop . named_color);
	if (ctxt . HasError())
	{
		// Should EE_BACKDROP_BADVALUE (?)
		return;
	}

	r_backdrop . type = kMCInterfaceBackdropTypeColor;
}

static void MCInterfaceBackdropFormat(MCExecContext& ctxt, const MCInterfaceBackdrop& p_backdrop, MCStringRef& r_output)
{
	switch(p_backdrop . type)
	{
	case kMCInterfaceBackdropTypeNone:
		if (MCStringCreateWithCString("none", r_output))
			return;
		break;
	case kMCInterfaceBackdropTypeColor:
		MCInterfaceNamedColorFormat(ctxt, p_backdrop . named_color, r_output);
		return;
	case kMCInterfaceBackdropTypePattern:
		if (ctxt . FormatUnsignedInteger(p_backdrop . pattern, r_output))
			return;
		break;
	}

	ctxt . Throw();
}

static void MCInterfaceBackdropInit(MCExecContext& ctxt, MCInterfaceBackdrop& r_backdrop)
{
	MCMemoryClear(&r_backdrop, sizeof(MCInterfaceBackdrop));
}

static void MCInterfaceBackdropFree(MCExecContext& ctxt, MCInterfaceBackdrop& p_backdrop)
{
	if (p_backdrop . type == kMCInterfaceBackdropTypeColor)
		MCInterfaceNamedColorFree(ctxt, p_backdrop . named_color);
}

static void MCInterfaceBackdropCopy(MCExecContext& ctxt, const MCInterfaceBackdrop& p_source, MCInterfaceBackdrop& r_target)
{
	r_target . type = p_source . type;
	if (r_target . type == kMCInterfaceBackdropTypePattern)
		r_target . pattern = p_source . pattern;
	else if (r_target . type == kMCInterfaceBackdropTypeColor)
		MCInterfaceNamedColorCopy(ctxt, p_source . named_color, r_target . named_color);
}

static bool MCInterfaceBackdropIsEqualTo(const MCInterfaceBackdrop& p_left, const MCInterfaceBackdrop& p_right)
{
	if (p_left . type != p_right . type)
		return false;

	if (p_left . type == kMCInterfaceBackdropTypeNone)
		return true;

	if (p_left . type == kMCInterfaceBackdropTypePattern)
		return p_left . pattern == p_right . pattern;

	return MCInterfaceNamedColorIsEqualTo(p_left . named_color, p_right . named_color);
}

static MCExecCustomTypeInfo _kMCInterfaceBackdropTypeInfo =
{
	"Interface.Backdrop",
	sizeof(MCInterfaceBackdrop),
	(void *)MCInterfaceBackdropParse,
	(void *)MCInterfaceBackdropFormat,
	(void *)MCInterfaceBackdropFree,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceLookAndFeelElementInfo[] =
{	
	{ "Appearance Manager", LF_AM, false },
	{ "Motif", LF_MOTIF, false },
	{ "Macintosh", LF_MAC, false },
	{ "Windows 95", LF_WIN95, false },
};

static MCExecEnumTypeInfo _kMCInterfaceLookAndFeelTypeInfo =
{
	"Interface.LookAndFeel",
	sizeof(_kMCInterfaceLookAndFeelElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceLookAndFeelElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfacePaintCompressionElementInfo[] =
{
	{ "png", EX_PNG },
	{ "jpeg", EX_JPEG },
	{ "gif", EX_GIF },
	{ "rle", EX_PBM },
};

static MCExecEnumTypeInfo _kMCInterfacePaintCompressionTypeInfo =
{
	"Interface.PaintCompression",
	sizeof(_kMCInterfacePaintCompressionElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfacePaintCompressionElementInfo,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceProcessTypeElementInfo[] =
{
	{ "background", 0 },
	{ "foreground", 1 },
};

static MCExecEnumTypeInfo _kMCInterfaceProcessTypeTypeInfo =
{
	"Interface.ProcessType",
	sizeof(_kMCInterfaceProcessTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceProcessTypeElementInfo,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceSelectionModeElementInfo[] =
{
	{ "surround", 0 },
	{ "intersect", 1 },
};

static MCExecEnumTypeInfo _kMCInterfaceSelectionModeTypeInfo =
{
	"Interface.ProcessType",
	sizeof(_kMCInterfaceSelectionModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceSelectionModeElementInfo,
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfaceLookAndFeelTypeInfo = &_kMCInterfaceLookAndFeelTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceBackdropTypeInfo = &_kMCInterfaceBackdropTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceNamedColorTypeInfo = &_kMCInterfaceNamedColorTypeInfo;
MCExecEnumTypeInfo *kMCInterfacePaintCompressionTypeInfo = &_kMCInterfacePaintCompressionTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceProcessTypeTypeInfo = &_kMCInterfaceProcessTypeTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceSelectionModeTypeInfo = &_kMCInterfaceSelectionModeTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static MCInterfaceBackdrop MCbackdrop;
static uint4 MCiconid;
static MCStringRef MCiconmenu;
static uint4 MCstatusiconid;
static MCStringRef MCstatusiconmenu;
static MCStringRef MCstatusicontooltip;

void MCInterfaceInitialize(MCExecContext& ctxt)
{
	MCInterfaceBackdropInit(ctxt, MCbackdrop);
	MCiconid = 0;
	MCiconmenu = (MCStringRef)MCValueRetain(kMCEmptyString);
	MCstatusiconid = 0;
	MCstatusiconmenu = (MCStringRef)MCValueRetain(kMCEmptyString);
	MCstatusicontooltip = (MCStringRef)MCValueRetain(kMCEmptyString);
}

void MCInterfaceFinalize(MCExecContext& ctxt)
{
	MCInterfaceBackdropFree(ctxt, MCbackdrop);
	MCValueRelease(MCiconmenu);
	MCValueRelease(MCstatusiconmenu);
	MCValueRelease(MCstatusicontooltip);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetDialogData(MCExecContext& ctxt, MCValueRef& r_value)
{
	r_value = MCValueRetain(MCdialogdata -> getvalueref());
}

void MCInterfaceSetDialogData(MCExecContext& ctxt, MCValueRef p_value)
{
	MCdialogdata -> setvalueref(p_value);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetLookAndFeel(MCExecContext& ctxt, intenum_t& r_value)
{
	if (MCcurtheme != nil)
		r_value = LF_AM;
	else
		r_value = MClook;
}

void MCInterfaceSetLookAndFeel(MCExecContext& ctxt, intenum_t p_value)
{
	MCTheme *newtheme = NULL;
	MCField *oldactive = MCactivefield;
	if (oldactive != NULL)
		oldactive->kunfocus();
	uint2 oldlook = MClook;
	MCTheme *oldtheme = MCcurtheme;
	MCcurtheme = NULL;
	MClook = p_value;
	
	if (p_value == LF_AM)
	{
		if (!oldtheme || 
			(oldtheme->getthemeid() != LF_NATIVEWIN && 
			 oldtheme->getthemeid() != LF_NATIVEMAC &&
			 oldtheme->getthemeid() != LF_NATIVEGTK))
		{
			MCcurtheme = MCThemeCreateNative();
			if (MCcurtheme->load())
			{
				if (oldtheme != NULL)
					oldtheme -> unload();
				delete oldtheme;
				MClook = MCcurtheme->getthemefamilyid();
			}
			else
			{
				delete MCcurtheme;
				MCcurtheme = oldtheme;
				MClook = oldlook;
			}
		}
		else
		{
			MCcurtheme = oldtheme;
			MClook = oldlook;
		}
	}
	if (MClook != oldlook || MCcurtheme != oldtheme)
	{
		if (IsMacEmulatedLF())
		{
			MCtemplatescrollbar->alloccolors();
			MCtemplatebutton->allocicons();
		}
		if (oldtheme)
		{
			oldtheme->unload();
			delete oldtheme;
		}
		
		// MW-2011-08-17: [[ Redraw ]] Changing theme means we must dirty
		//   everything.
		MCRedrawDirtyScreen();
	}
	if (oldactive != NULL)
		oldactive->kfocus();
}

void MCInterfaceGetScreenMouseLoc(MCExecContext& ctxt, MCPoint& r_value)
{
	int2 mx, my;
	MCscreen->querymouse(mx, my);
	r_value . x = mx;
	r_value . y = my;
}

void MCInterfaceSetScreenMouseLoc(MCExecContext& ctxt, MCPoint p_value)
{
	MCscreen->setmouse(p_value . x, p_value . y);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetBackdrop(MCExecContext& ctxt, MCInterfaceBackdrop& r_backdrop)
{
	MCInterfaceBackdropCopy(ctxt, MCbackdrop, r_backdrop);
}

void MCInterfaceSetBackdrop(MCExecContext& ctxt, const MCInterfaceBackdrop& p_backdrop)
{
	if (MCInterfaceBackdropIsEqualTo(p_backdrop, MCbackdrop))
		return;
	
	MCInterfaceBackdropFree(ctxt, MCbackdrop);
	MCInterfaceBackdropCopy(ctxt, p_backdrop, MCbackdrop);
	if (ctxt . HasError())
		return;

	if (MCbackdroppattern != nil)
	{
		MCpatternlist -> freepat(MCbackdroppattern);
		MCbackdroppattern = nil;
	}

	switch (p_backdrop . type)
	{
	case kMCInterfaceBackdropTypeNone:
		MCscreen -> disablebackdrop();
		MCscreen -> configurebackdrop(MCscreen -> getwhite(), NULL, nil);
		return;
	case kMCInterfaceBackdropTypeColor:
		MCscreen -> configurebackdrop(MCbackdrop . named_color . color, nil, nil);
		MCscreen -> enablebackdrop();
		break;
	case kMCInterfaceBackdropTypePattern:
		MCbackdroppattern = MCpatternlist->allocpat(p_backdrop . pattern, ctxt . GetObject());
		MCscreen -> configurebackdrop(MCbackdrop . named_color . color, MCbackdroppattern, nil);
		MCscreen -> enablebackdrop();
		break;
	}
}

void MCInterfaceGetBufferImages(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCbufferimages == True;
}

void MCInterfaceSetBufferImages(MCExecContext& ctxt, bool p_value)
{
	MCbufferimages = p_value ? True : False;
}

void MCInterfaceGetSystemFileSelector(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCsystemFS == True;
}

void MCInterfaceSetSystemFileSelector(MCExecContext& ctxt, bool p_value)
{
	MCsystemFS = p_value ? True : False;
}

void MCInterfaceGetSystemColorSelector(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCsystemCS == True;
}

void MCInterfaceSetSystemColorSelector(MCExecContext& ctxt, bool p_value)
{
	MCsystemCS = p_value ? True : False;
}

void MCInterfaceGetSystemPrintSelector(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCsystemPS == True;
}

void MCInterfaceSetSystemPrintSelector(MCExecContext& ctxt, bool p_value)
{
	MCsystemPS = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void get_interface_color(const MCColor& p_color, MCStringRef p_color_name, MCInterfaceNamedColor& r_color)
{
	r_color . name = p_color_name != nil ? (MCStringRef)MCValueRetain(p_color_name) : nil;
	r_color . color = p_color;
}

void set_interface_color(MCColor& x_color, MCStringRef& x_color_name, const MCInterfaceNamedColor& p_color)
{
	if (x_color_name != nil)
		MCValueRelease(x_color_name);
	x_color_name = p_color . name != nil ? (MCStringRef)MCValueRetain(p_color . name) : nil;
	x_color = p_color . color;
}

void MCInterfaceGetPaintCompression(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = MCpaintcompression;
}

void MCInterfaceSetPaintCompression(MCExecContext& ctxt, intenum_t p_value)
{
	MCpaintcompression = (Export_format)p_value;
}

//////////

void MCInterfaceGetBrushBackColor(MCExecContext& ctxt, MCValueRef& r_color)
{
	r_color = MCValueRetain(kMCEmptyString);
}

void MCInterfaceSetBrushBackColor(MCExecContext& ctxt, MCValueRef color)
{
	// NO-OP
}

void MCInterfaceGetPenBackColor(MCExecContext& ctxt, MCValueRef& r_color)
{
	r_color = MCValueRetain(kMCEmptyString);
}

void MCInterfaceSetPenBackColor(MCExecContext& ctxt, MCValueRef color)
{
	// NO-OP
}

//////////

void MCInterfaceGetBrushColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MCbrushcolor, MCbrushcolorname, r_color);
}

void MCInterfaceSetBrushColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	MCeditingimage = nil;
	MCpatternlist->freepat(MCbrushpattern);
	set_interface_color(MCbrushcolor, MCbrushcolorname, p_color);
	MCscreen -> alloccolor(MCbrushcolor);
}

void MCInterfaceGetPenColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MCpencolor, MCpencolorname, r_color);
}

void MCInterfaceSetPenColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	MCeditingimage = nil;
	MCpatternlist->freepat(MCpenpattern);
	set_interface_color(MCpencolor, MCpencolorname, p_color);
	MCscreen -> alloccolor(MCpencolor);
}

void MCInterfaceGetBrushPattern(MCExecContext& ctxt, uinteger_t& r_pattern)
{
	if (MCbrushpmid < PI_END && MCbrushpmid > PI_PATTERNS)
		r_pattern = MCbrushpmid - PI_PATTERNS;
	else
		
		r_pattern = MCbrushpmid;
}

void MCInterfaceSetBrushPattern(MCExecContext& ctxt, uinteger_t pattern)
{
	MCPatternRef newpm;
	if (MCbrushpmid < PI_END)
		MCbrushpmid += PI_PATTERNS;
	newpm = MCpatternlist->allocpat(MCbrushpmid, ctxt . GetObject());
	if (newpm == None)
	{
		ctxt . LegacyThrow(EE_PROPERTY_BRUSHPATNOIMAGE);
		return;
	}
	MCeditingimage = nil;
	MCpatternlist->freepat(MCbrushpattern);
	MCbrushpattern = newpm;
}

void MCInterfaceGetPenPattern(MCExecContext& ctxt, uinteger_t& r_pattern)
{
	if (MCpenpmid < PI_END && MCpenpmid > PI_PATTERNS)
		r_pattern = MCpenpmid - PI_PATTERNS;
	else
		r_pattern = MCpenpmid;
}

void MCInterfaceSetPenPattern(MCExecContext& ctxt, uinteger_t pattern)
{
	MCPatternRef newpm;
	if (MCpenpmid < PI_END)
		MCpenpmid += PI_PATTERNS;
	newpm = MCpatternlist->allocpat(MCpenpmid, ctxt . GetObject());
	if (newpm == nil)
	{
		ctxt . LegacyThrow(EE_PROPERTY_PENPATNOIMAGE);
		return;
	}
	MCeditingimage = nil;
	MCpatternlist->freepat(MCpenpattern);
	MCpenpattern = newpm;
}

void MCInterfaceGetFilled(MCExecContext& ctxt, bool& r_filled)
{
	r_filled = MCfilled == True;
}

void MCInterfaceSetFilled(MCExecContext& ctxt, bool p_filled)
{
	MCfilled = p_filled;
}

void MCInterfaceGetPolySides(MCExecContext& ctxt, uinteger_t& r_sides)
{
	r_sides = MCpolysides;
}

void MCInterfaceSetPolySides(MCExecContext& ctxt, uinteger_t p_sides)
{
	MCpolysides = MCU_max(3U, MCU_min(p_sides, (uinteger_t)MCscreen->getmaxpoints()));
}

void MCInterfaceGetLineSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	r_size = MClinesize;
}

void MCInterfaceSetLineSize(MCExecContext& ctxt, uinteger_t p_size)
{
	MClinesize = p_size;
}

void MCInterfaceGetRoundRadius(MCExecContext& ctxt, uinteger_t& r_radius)
{
	r_radius = MCroundradius;
}

void MCInterfaceSetRoundRadius(MCExecContext& ctxt, uinteger_t p_radius)
{
	MCroundradius = p_radius;
}

void MCInterfaceGetStartAngle(MCExecContext& ctxt, uinteger_t& r_angle)
{
	r_angle = MCstartangle;
}

void MCInterfaceSetStartAngle(MCExecContext& ctxt, uinteger_t p_angle)
{
	MCstartangle = p_angle % 360;
}

void MCInterfaceGetArcAngle(MCExecContext& ctxt, uinteger_t& r_angle)
{
	r_angle = MCarcangle;
}

void MCInterfaceSetArcAngle(MCExecContext& ctxt, uinteger_t p_angle)
{
	MCarcangle = p_angle % 361;
}

void MCInterfaceGetRoundEnds(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCroundends == True;
}

void MCInterfaceSetRoundEnds(MCExecContext& ctxt, bool p_value)
{
	MCroundends = p_value;
}

void MCInterfaceGetDashes(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_dashes)
{
    MCAutoArray<uinteger_t> t_dashes;
    
    for (uindex_t i = 0; i < MCndashes; i++)
        t_dashes . Push(MCdashes[i]);
    
    t_dashes . Take(r_dashes, r_count);
}

void MCInterfaceSetDashes(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_dashes)
{
    MCAutoArray<uint1> t_dashes;
    
    uint1 *newdashes = nil;
    uint2 newndashes = 0;
    uint4 t_dash_length = 0;
    uindex_t t_new_count;
    
    for (uindex_t i = 0; i < p_count; i++)
    {
        if (p_dashes[i] >= 256)
        {
            ctxt . LegacyThrow(EE_GRAPHIC_NAN);
            return;
        }
        t_dashes . Push((uint1)p_dashes[i]);
        t_dash_length += p_dashes[i];
    }
    
    t_dashes . Take(newdashes, t_new_count);
    newndashes = t_new_count;
    
    if (newndashes > 0 && t_dash_length == 0)
    {
        delete newdashes;
        newdashes = nil;
        newndashes = 0;
    }
    
	delete MCdashes;
	MCdashes = newdashes;
	MCndashes = newndashes;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetRecentCards(MCExecContext& ctxt, MCStringRef& r_cards)
{
	if (MCrecent -> GetRecent(ctxt, nil, P_LONG_ID, r_cards))
		return;

	ctxt . Throw();
}

void MCInterfaceGetRecentNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	if (MCrecent -> GetRecent(ctxt, nil, P_SHORT_NAME, r_names))
		return;

	ctxt . Throw();
}

void MCInterfaceGetEditBackground(MCExecContext& ctxt, bool& r_value)
{
	MCdefaultstackptr -> getboolprop(ctxt, 0, P_EDIT_BACKGROUND, False, r_value);
}

void MCInterfaceSetEditBackground(MCExecContext& ctxt, bool p_new_value)
{
	MCdefaultstackptr -> setboolprop(ctxt, 0, P_EDIT_BACKGROUND, False, p_new_value);
}

void MCInterfaceGetLockScreen(MCExecContext& ctxt, bool& r_value)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	r_value = MCRedrawIsScreenLocked();
}

void MCInterfaceSetLockScreen(MCExecContext& ctxt, bool p_new_lock)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw methods.
	if (p_new_lock)
		MCRedrawLockScreen();
	else
		MCRedrawUnlockScreenWithEffects();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetAccentColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MCaccentcolor, MCaccentcolorname, r_color);
}

void MCInterfaceSetAccentColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MCaccentcolor, MCaccentcolorname, p_color);
}

void MCInterfaceGetHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MChilitecolor, MChilitecolorname, r_color);
}

void MCInterfaceSetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MChilitecolor, MChilitecolorname, p_color);
}

void MCInterfaceGetLinkColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MClinkatts . color, MClinkatts . colorname, r_color);
}

void MCInterfaceSetLinkColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MClinkatts . color, MClinkatts . colorname, p_color);
	
	// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
	MCRedrawDirtyScreen();
}

void MCInterfaceGetLinkHiliteColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MClinkatts . color, MClinkatts . colorname, r_color);
}

void MCInterfaceSetLinkHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MClinkatts . hilitecolor, MClinkatts . hilitecolorname, p_color);
	
	// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
	MCRedrawDirtyScreen();
}

void MCInterfaceGetLinkVisitedColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MClinkatts . visitedcolor, MClinkatts . visitedcolorname, r_color);
}

void MCInterfaceSetLinkVisitedColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MClinkatts . visitedcolor, MClinkatts . visitedcolorname, p_color);
	
	// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
	MCRedrawDirtyScreen();
}

void MCInterfaceGetUnderlineLinks(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClinkatts . underline == True;
}

void MCInterfaceSetUnderlineLinks(MCExecContext& ctxt, bool p_value)
{
	MClinkatts . underline = p_value;
	
	// MW-2011-08-17: [[ Redraw ]] Global property could affect anything so dirty screen.
	MCRedrawDirtyScreen();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetSelectGroupedControls(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCselectgrouped == True;
}

void MCInterfaceSetSelectGroupedControls(MCExecContext& ctxt, bool p_value)
{
	MCselectgrouped = p_value;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetIcon(MCExecContext& ctxt, uinteger_t& r_icon)
{
	r_icon = MCiconid;
}

void MCInterfaceSetIcon(MCExecContext& ctxt, uinteger_t p_icon)
{
	MCiconid = p_icon;
	MCscreen -> seticon(MCiconid);
}

void MCInterfaceGetIconMenu(MCExecContext& ctxt, MCStringRef& r_menu)
{
	r_menu = (MCStringRef)MCValueRetain(MCiconmenu);
}

void MCInterfaceSetIconMenu(MCExecContext& ctxt, MCStringRef p_menu)
{
	MCValueAssign(MCiconmenu, p_menu);
	MCscreen -> seticonmenu(MCiconmenu);
}

void MCInterfaceGetStatusIcon(MCExecContext& ctxt, uinteger_t& r_icon)
{
	r_icon = MCstatusiconid;
}

void MCInterfaceSetStatusIcon(MCExecContext& ctxt, uinteger_t p_icon)
{
	if (MCstatusiconid == p_icon)
		return;
		
	MCstatusiconid = p_icon;
	MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
}

void MCInterfaceGetStatusIconToolTip(MCExecContext& ctxt, MCStringRef& r_tooltip)
{
	r_tooltip = (MCStringRef)MCValueRetain(MCstatusicontooltip);
}

void MCInterfaceSetStatusIconToolTip(MCExecContext& ctxt, MCStringRef p_tooltip)
{
	MCValueAssign(MCstatusicontooltip, p_tooltip);
	MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
}

void MCInterfaceGetStatusIconMenu(MCExecContext& ctxt, MCStringRef& r_icon_menu)
{
	r_icon_menu = (MCStringRef)MCValueRetain(MCstatusiconmenu);
}

void MCInterfaceSetStatusIconMenu(MCExecContext& ctxt, MCStringRef p_icon_menu)
{
	MCValueAssign(MCstatusiconmenu, p_icon_menu);
	MCscreen -> configurestatusicon(MCstatusiconid, MCstatusiconmenu, MCstatusicontooltip);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetAllowInlineInput(MCExecContext& ctxt, bool &r_value)
{
	r_value = MCinlineinput == True;
}

void MCInterfaceSetAllowInlineInput(MCExecContext& ctxt, bool p_value)
{
	MCinlineinput = p_value ? True : False;
}

void MCInterfaceGetDragDelta(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdragdelta;
}

void MCInterfaceSetDragDelta(MCExecContext& ctxt, uinteger_t p_value)
{
	MCdragdelta = p_value;
}

void MCInterfaceGetStackFileType(MCExecContext& ctxt, MCStringRef& r_value)
{
	r_value = MCValueRetain(MCstackfiletype);
}

void MCInterfaceSetStackFileType(MCExecContext& ctxt, MCStringRef p_value)
{
	MCValueAssign(MCstackfiletype, p_value);
}


void MCInterfaceGetStackFileVersion(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (MCstackfileversion % 100 == 0) 
	{
		if (MCStringFormat(r_value, "%d.%d", MCstackfileversion / 1000, (MCstackfileversion % 1000) / 100))
			return;
	}
	else
	{
		if (MCStringFormat(r_value, "%d.%d.%d", MCstackfileversion / 1000, (MCstackfileversion % 1000) / 100, (MCstackfileversion % 100) / 10))
			return;
	}

	ctxt . Throw();
}

void MCInterfaceSetStackFileVersion(MCExecContext& ctxt, MCStringRef p_value)
{
	uint4 major = 0, minor = 0, revision = 0, version = 0;
	uint4 count;
	// MW-2006-03-24: This should be snscanf - except it doesn't exist on BSD!!
	char *t_version;
	
	t_version = strclone(MCStringGetCString(p_value));
	count = sscanf(t_version, "%d.%d.%d", &major, &minor, &revision);
	delete t_version;
	
	version = major * 1000 + minor * 100 + revision * 10;
	
	// MW-2012-03-04: [[ StackFile5500 ]] Allow versions up to 5500 to be set.
	if (count < 2 || version < 2400 || version > 5500)
	{
		ctxt . LegacyThrow(EE_PROPERTY_STACKFILEBADVERSION);
		return;
	}
	MCstackfileversion = version;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetProcessType(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = MCS_processtypeisforeground() ? 1 : 0;
}

void MCInterfaceSetProcessType(MCExecContext& ctxt, intenum_t p_value)
{
	bool t_wants_foreground;
	if (p_value == 0)
		t_wants_foreground = false;
	else
		t_wants_foreground = true;
		
	if (MCS_changeprocesstype(t_wants_foreground))
		return;
		
	ctxt . LegacyThrow(EE_PROCESSTYPE_NOTPOSSIBLE);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetShowInvisibles(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCshowinvisibles == True;
}

void MCInterfaceSetShowInvisibles(MCExecContext& ctxt, bool p_value)
{
	MCshowinvisibles = p_value ? True : False;
	
	// MW-2011-08-17: [[ Redraw ]] Things are now visible that weren't before so dirty the screen.
	MCRedrawDirtyScreen();
}

void MCInterfaceSetCursor(MCExecContext& ctxt, uinteger_t& r_id, bool p_is_default, MCCursorRef& r_cursor)
{
	if (r_id < PI_NCURSORS)
	{
		if (!p_is_default && r_id == PI_BUSY)
		{
			r_id = PI_BUSY1 + MCbusycount;
			MCbusycount = MCbusycount + 1 & 0x7;
		}
		r_cursor = MCcursors[r_id];
	}
	else
	{
		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method - note
		// here we use the object attached to the execution context rather than the
		// 'parent' - this ensures that the search follows the use of behavior.
		MCImage *newim;
		newim = ctxt . GetObject() -> resolveimageid(r_id);
		
		if (newim == nil)
		{
			if (r_id == 28)
				r_cursor = MCcursors[8];
			else if (r_id == 29)
				r_cursor = MCcursors[1];
			else
			{
				ctxt . LegacyThrow(EE_PROPERTY_CURSORNOIMAGE);
				return;
			}
		}
		else
			r_cursor = newim->getcursor(p_is_default);
	}
}


void MCInterfaceGetCursor(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCcursorid;
}

void MCInterfaceSetCursor(MCExecContext& ctxt, uinteger_t p_value)
{
	MCCursorRef t_cursor;
	MCInterfaceSetCursor(ctxt, p_value, false, t_cursor);
	if (t_cursor != nil)
	{
		MCcursor = t_cursor;
		MCcursorid = p_value;
		if (MCmousestackptr != NULL)
			MCmousestackptr->resetcursor(True);
		else
			MCdefaultstackptr->resetcursor(True);
	}
}

void MCInterfaceGetDefaultCursor(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdefaultcursorid;
}

void MCInterfaceSetDefaultCursor(MCExecContext& ctxt, uinteger_t p_value)
{
	MCCursorRef t_cursor;
	MCInterfaceSetCursor(ctxt, p_value, true, t_cursor);
	if (t_cursor != nil)
	{
		MCdefaultcursor = t_cursor;
		MCdefaultcursorid = p_value;
		if (MCmousestackptr != NULL)
			MCmousestackptr->resetcursor(True);
		else
			MCdefaultstackptr->resetcursor(True);
	}
}
void MCInterfaceGetDefaultStack(MCExecContext& ctxt, MCStringRef& r_value)
{
	MCdefaultstackptr -> names(P_NAME, r_value);
}

void MCInterfaceSetDefaultStack(MCExecContext& ctxt, MCStringRef p_value)
{
	MCStack *sptr;
	sptr = MCdefaultstackptr->findstackname_oldstring(MCStringGetOldString(p_value));
	
	if (sptr == nil)
	{
		ctxt . LegacyThrow(EE_PROPERTY_NODEFAULTSTACK);
		return;
	}

	MCdefaultstackptr = MCstaticdefaultstackptr = sptr;
}

void MCInterfaceGetDefaultMenubar(MCExecContext& ctxt, MCStringRef& r_value)
{	
	if (MCdefaultmenubar == nil)
	{
		r_value = (MCStringRef)MCValueRetain(kMCEmptyString);
		return;
	}
	else
	{
		if (MCdefaultmenubar->names(P_LONG_NAME, r_value))
			return;
	}

	ctxt . Throw();
}

void MCInterfaceSetDefaultMenubar(MCExecContext& ctxt, MCStringRef p_value)
{
	MCGroup *gptr = (MCGroup *)MCdefaultstackptr->getobjname(CT_GROUP, p_value);
																	 
	if (gptr == NULL)
	{
		ctxt . LegacyThrow(EE_PROPERTY_NODEFAULTMENUBAR);
		return;
	}
	
	MCdefaultmenubar = gptr;
	MCscreen->updatemenubar(False);
}
void MCInterfaceGetDragSpeed(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdragspeed;
}

void MCInterfaceSetDragSpeed(MCExecContext& ctxt, uinteger_t p_value)
{
	MCdragspeed = p_value;
}

void MCInterfaceGetMoveSpeed(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCmovespeed;
}

void MCInterfaceSetMoveSpeed(MCExecContext& ctxt, uinteger_t p_value)
{
	MCmovespeed = p_value;
}

void MCInterfaceGetLockCursor(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockcursor == True;
}

void MCInterfaceSetLockCursor(MCExecContext& ctxt, bool p_value)
{
	MClockcursor = p_value ? True : False;
	if (!MClockcursor)
		ctxt . GetObject()->getstack()->resetcursor(True);
}

void MCInterfaceGetLockErrors(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockerrors == True;
}

void MCInterfaceSetLockErrors(MCExecContext& ctxt, bool p_value)
{			
	MCerrorlockptr = ctxt . GetObject();
	MClockerrors = p_value ? True : False;
}

void MCInterfaceGetLockMenus(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockmenus == True;
}

void MCInterfaceSetLockMenus(MCExecContext& ctxt, bool p_value)
{
	MClockmenus = p_value ? True : False;
	MCscreen->updatemenubar(True);
}

void MCInterfaceGetLockMessages(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockmessages == True;
}

void MCInterfaceSetLockMessages(MCExecContext& ctxt, bool p_value)
{
	MClockmessages = p_value ? True : False;
}

void MCInterfaceGetLockMoves(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockmoves == True;
}

void MCInterfaceSetLockMoves(MCExecContext& ctxt, bool p_value)
{
	MClockmoves = p_value ? True : False;
}

void MCInterfaceGetLockRecent(MCExecContext& ctxt, bool& r_value)
{
	r_value = MClockrecent == True;
}

void MCInterfaceSetLockRecent(MCExecContext& ctxt, bool p_value)
{
	MClockrecent = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetIdleRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCidleRate;
}

void MCInterfaceSetIdleRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCidleRate = MCU_max(p_value, (uint4)1);
}

void MCInterfaceGetIdleTicks(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCidleRate * 60 / 1000;
}

void MCInterfaceSetIdleTicks(MCExecContext& ctxt, uinteger_t p_value)
{
	MCidleRate = MCU_max(p_value * 1000 / 60, (uint4)1);
}

void MCInterfaceGetBlinkRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCblinkrate;
}

void MCInterfaceSetBlinkRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCblinkrate = MCU_max(p_value, (uint4)1);
}

void MCInterfaceGetRepeatRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCrepeatrate;
}

void MCInterfaceSetRepeatRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCrepeatrate = MCU_max(p_value, (uint4)1);
}

void MCInterfaceGetRepeatDelay(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCrepeatdelay;
}

void MCInterfaceSetRepeatDelay(MCExecContext& ctxt, uinteger_t p_value)
{
	MCrepeatdelay = p_value;
}

void MCInterfaceGetTypeRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCtyperate;
}

void MCInterfaceSetTypeRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCtyperate = p_value;
}

void MCInterfaceGetSyncRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCsyncrate;
}

void MCInterfaceSetSyncRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCsyncrate = MCU_max(p_value, (uint4)1);
}

void MCInterfaceGetEffectRate(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCeffectrate;
}

void MCInterfaceSetEffectRate(MCExecContext& ctxt, uinteger_t p_value)
{
	MCeffectrate = p_value;
}

void MCInterfaceGetDoubleDelta(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdoubledelta;
}

void MCInterfaceSetDoubleDelta(MCExecContext& ctxt, uinteger_t p_value)
{
	MCdoubledelta = p_value;
}

void MCInterfaceGetDoubleTime(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCdoubletime;
}

void MCInterfaceSetDoubleTime(MCExecContext& ctxt, uinteger_t p_value)
{
	MCdoubletime = p_value;
}

void MCInterfaceGetTooltipDelay(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCtooltipdelay;
}

void MCInterfaceSetTooltipDelay(MCExecContext& ctxt, uinteger_t p_value)
{
	MCtooltipdelay = p_value;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetNavigationArrows(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCnavigationarrows == True;
}

void MCInterfaceSetNavigationArrows(MCExecContext& ctxt, bool p_value)
{
	MCnavigationarrows = p_value ? True : False;
}

void MCInterfaceGetExtendKey(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCextendkey;
}

void MCInterfaceSetExtendKey(MCExecContext& ctxt, uinteger_t p_value)
{
	MCextendkey = p_value;
}

void MCInterfaceGetPointerFocus(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCpointerfocus == True;
}

void MCInterfaceSetPointerFocus(MCExecContext& ctxt, bool p_value)
{
	MCpointerfocus = p_value ? True : False;
}

void MCInterfaceGetEmacsKeyBindings(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCemacskeys == True;
}

void MCInterfaceSetEmacsKeyBindings(MCExecContext& ctxt, bool p_value)
{
	MCemacskeys = p_value ? True : False;
}

void MCInterfaceGetRaiseMenus(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCraisemenus == True;
}

void MCInterfaceSetRaiseMenus(MCExecContext& ctxt, bool p_value)
{
	MCraisemenus = p_value ? True : False;
}

void MCInterfaceGetActivatePalettes(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCactivatepalettes == True;
}

void MCInterfaceSetActivatePalettes(MCExecContext& ctxt, bool p_value)
{
	MCactivatepalettes = p_value ? True : False;
}

void MCInterfaceGetHidePalettes(MCExecContext& ctxt, bool& r_value)
{
	r_value = MChidepalettes == True;
}

void MCInterfaceSetHidePalettes(MCExecContext& ctxt, bool p_value)
{
	MChidepalettes = p_value ? True : False;
}


void MCInterfaceGetRaisePalettes(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCraisepalettes == True;
}

void MCInterfaceSetRaisePalettes(MCExecContext& ctxt, bool p_value)
{
	// MW-2004-11-17: On Linux, effect a restack if 'raisepalettes' is changed
	// MW-2004-11-24: Altered MCStacklst::restack to find right stack if passed NULL
	MCraisepalettes = p_value ? True : False;
#ifdef LINUX
	MCstacks -> restack(NULL);
#endif
}

void MCInterfaceGetRaiseWindows(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCraisewindows == True;
}

void MCInterfaceSetRaiseWindows(MCExecContext& ctxt, bool p_value)
{
	MCraisewindows = p_value ? True : False;
	MCscreen -> enactraisewindows();
}

void MCInterfaceGetHideBackdrop(MCExecContext& ctxt, bool& r_value)
{
	r_value = MChidebackdrop == True;
}

void MCInterfaceSetHideBackdrop(MCExecContext& ctxt, bool p_value)
{
	MChidebackdrop = p_value ? True : False;
}

void MCInterfaceGetDontUseNavigationServices(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCdontuseNS == True;
}

void MCInterfaceSetDontUseNavigationServices(MCExecContext& ctxt, bool p_value)
{
	MCdontuseNS = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetProportionalThumbs(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCproportionalthumbs == True;
}

void MCInterfaceSetProportionalThumbs(MCExecContext& ctxt, bool p_value)
{
	if (IsMacLFAM())
		return;

	MCproportionalthumbs = p_value ? True : False;
	
	// MW-2011-08-17: [[ Redraw ]] This affects the redraw of any scrollbar so dirty everything.
	MCRedrawDirtyScreen();
}

void MCInterfaceGetSharedMemory(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCshm == True;
}

void MCInterfaceSetSharedMemory(MCExecContext& ctxt, bool p_value)
{
	MCshm = p_value ? True : False;
}

void MCInterfaceGetScreenGamma(MCExecContext& ctxt, double& r_value)
{
	r_value = MCgamma;
}

void MCInterfaceSetScreenGamma(MCExecContext& ctxt, double p_value)
{
	MCgamma = p_value;
}

void MCInterfaceGetSelectionMode(MCExecContext& ctxt, intenum_t& r_value)
{
	r_value = (intenum_t)MCselectintersect;
}

void MCInterfaceSetSelectionMode(MCExecContext& ctxt, intenum_t p_value)
{
	MCselectintersect = (Boolean)p_value;
}

void MCInterfaceGetSelectionHandleColor(MCExecContext& ctxt, MCInterfaceNamedColor& r_color)
{
	get_interface_color(MCselectioncolor, MCselectioncolorname, r_color);
}

void MCInterfaceSetSelectionHandleColor(MCExecContext& ctxt, const MCInterfaceNamedColor& p_color)
{
	set_interface_color(MCselectioncolor, MCselectioncolorname, p_color);
	MCselected->redraw();
}

void MCInterfaceGetWindowBoundingRect(MCExecContext& ctxt, MCRectangle& r_value)
{
	r_value = MCwbr;
}

void MCInterfaceSetWindowBoundingRect(MCExecContext& ctxt, MCRectangle p_value)
{
	MCwbr = p_value;
}

void MCInterfaceGetJpegQuality(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCjpegquality;
}

void MCInterfaceSetJpegQuality(MCExecContext& ctxt, uinteger_t p_value)
{
	MCjpegquality = MCU_min(p_value, (uint4)100);
}

void MCInterfaceGetRelayerGroupedControls(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCrelayergrouped == True;
}

void MCInterfaceSetRelayerGroupedControls(MCExecContext& ctxt, bool p_value)
{
	MCrelayergrouped= p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceSetBrush(MCExecContext& ctxt, Properties p_which, uinteger_t p_value)
{
	uint4 t_newbrush = p_value;
	if (t_newbrush < PI_PATTERNS)
		t_newbrush += PI_BRUSHES;

	// MW-2009-02-02: [[ Improved image search ]]
	// Search for the appropriate image object using the standard method - note
	// here we use the object attached to the execution context rather than the
	// 'parent' - this ensures that the search follows the use of behavior.
	MCImage *newim;
	newim = ctxt . GetObject() -> resolveimageid(t_newbrush);

	if (newim == NULL)
	{
		ctxt . LegacyThrow(EE_PROPERTY_BRUSHNOIMAGE);
		return;
	}

	switch (p_which)
	{
	case P_BRUSH:
		MCbrush = t_newbrush;
		break;
	case P_ERASER:
		MCeraser = t_newbrush;
		break;
	case P_SPRAY:
		MCspray = t_newbrush;
		break;
	default:
		break;
	}

	newim->createbrush(p_which);
	ctxt . GetObject() -> getstack() -> resetcursor(True);
}

void MCInterfaceGetBrush(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCbrush < PI_PATTERNS ? MCbrush - PI_BRUSHES : MCbrush;
}

void MCInterfaceSetBrush(MCExecContext& ctxt, uinteger_t p_value)
{
	MCInterfaceSetBrush(ctxt, P_BRUSH, p_value);
}

void MCInterfaceGetEraser(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCeraser < PI_PATTERNS ? MCeraser - PI_BRUSHES : MCeraser;
}

void MCInterfaceSetEraser(MCExecContext& ctxt, uinteger_t p_value)
{
	MCInterfaceSetBrush(ctxt, P_ERASER, p_value);
}

void MCInterfaceGetSpray(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCspray < PI_PATTERNS ? MCspray - PI_BRUSHES : MCspray;
}

void MCInterfaceSetSpray(MCExecContext& ctxt, uinteger_t p_value)
{
	MCInterfaceSetBrush(ctxt, P_SPRAY, p_value);
}

void MCInterfaceGetCentered(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCcentered == True;
}

void MCInterfaceSetCentered(MCExecContext& ctxt, bool p_value)
{
	MCcentered = p_value ? True : False;
}

void MCInterfaceGetGrid(MCExecContext& ctxt, bool& r_value)
{
	r_value = MCgrid == True;
}

void MCInterfaceSetGrid(MCExecContext& ctxt, bool p_value)
{
	MCgrid = p_value ? True : False;
}

void MCInterfaceGetGridSize(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCgridsize;
}

void MCInterfaceSetGridSize(MCExecContext& ctxt, uinteger_t p_value)
{
	MCgridsize = MCU_max(p_value, (uint4)1);
}

void MCInterfaceGetSlices(MCExecContext& ctxt, uinteger_t& r_value)
{
	r_value = MCslices;
}

void MCInterfaceSetSlices(MCExecContext& ctxt, uinteger_t p_value)
{
	MCslices = p_value < 2 ? 2 : p_value;
}

void MCInterfaceGetBeepLoudness(MCExecContext& ctxt, integer_t& r_value)
{
	MCscreen -> getbeep(P_BEEP_LOUDNESS, r_value);
}

void MCInterfaceSetBeepLoudness(MCExecContext& ctxt, integer_t p_value)
{
	MCscreen -> setbeep(P_BEEP_LOUDNESS, p_value);
}

void MCInterfaceGetBeepPitch(MCExecContext& ctxt, integer_t& r_value)
{
	MCscreen -> getbeep(P_BEEP_PITCH, r_value);
}

void MCInterfaceSetBeepPitch(MCExecContext& ctxt, integer_t p_value)
{
	MCscreen -> setbeep(P_BEEP_PITCH, p_value);
}

void MCInterfaceGetBeepDuration(MCExecContext& ctxt, integer_t& r_value)
{
	MCscreen -> getbeep(P_BEEP_DURATION, r_value);
}

void MCInterfaceSetBeepDuration(MCExecContext& ctxt, integer_t p_value)
{
	MCscreen -> setbeep(P_BEEP_DURATION, p_value);
}

void MCInterfaceGetBeepSound(MCExecContext& ctxt, MCStringRef& r_value)
{
	if (!MCscreen -> getbeepsound(r_value))
		r_value = MCValueRetain(kMCEmptyString);
}

void MCInterfaceSetBeepSound(MCExecContext& ctxt, MCStringRef p_value)
{
	if (MCscreen -> setbeepsound(p_value))
		return;

	ctxt . Throw();
}
void MCInterfaceGetTool(MCExecContext& ctxt, MCStringRef& r_value)
{
	MCStringFormat(r_value, "%s tool", MCtoolnames[MCcurtool]);
}

void MCInterfaceSetTool(MCExecContext& ctxt, MCStringRef p_value)
{
	MCU_choose_tool(ctxt, p_value, T_UNDEFINED);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceGetScreenRect(MCExecContext& ctxt, bool p_working, bool p_effective, MCRectangle& r_value)
{
	const MCDisplay *t_displays;
	MCscreen -> getdisplays(t_displays, p_effective);

	r_value = p_working ? t_displays[0] . workarea : t_displays[0] . viewport;
}

void MCInterfaceGetScreenRects(MCExecContext& ctxt, bool p_working, bool p_effective, MCStringRef& r_value)
{
	MCAutoListRef t_list;
	bool t_success = true;

	t_success = MCListCreateMutable('\n', &t_list);

	const MCDisplay *t_displays;
	uinteger_t t_count = 0;

	t_count = MCscreen->getdisplays(t_displays, p_effective);

	for (uinteger_t i = 0; t_success && i < t_count; i++)
	{
		MCAutoStringRef t_string;
		MCRectangle t_rect;

		t_rect = p_working ? t_displays[i].workarea : t_displays[i].viewport;
		t_success = MCStringFormat(&t_string, "%d,%d,%d,%d", t_rect.x, t_rect.y,
			t_rect.x + t_rect.width, t_rect.y + t_rect.height) &&
			MCListAppend(*t_list, *t_string);
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_list, r_value);

	if (t_success)
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalHelpStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    MCStack *t_stack;
    t_stack = MCdefaultstackptr->findstackname(MCM_help);
    
    if (t_stack != nil)
    {
        r_object . object = t_stack;
        r_object . part_id = 0;
		return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalHomeStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    MCStack *t_stack;
    t_stack = MCdispatcher -> gethome();
    
    if (t_stack != nil)
    {
        r_object . object = t_stack;
        r_object . part_id = 0;
		return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalSelectedObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = MCselected -> getfirst();
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalTopStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCtopstackptr != nil)
    {
        r_object . object = MCtopstackptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalClickStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCclickstackptr != nil)
    {
        r_object . object = MCclickstackptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalMouseStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCmousestackptr != nil)
    {
        r_object . object = MCmousestackptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalClickFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCclickfield != nil)
    {
        r_object . object = MCclickfield;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalSelectedFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCactivefield != nil)
    {
        r_object . object = MCactivefield;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}
void MCInterfaceEvalSelectedImageAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCactiveimage != nil)
    {
        r_object . object = MCactiveimage;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}
void MCInterfaceEvalFoundFieldAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCfoundfield != nil)
    {
        r_object . object = MCfoundfield;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalMouseControlAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    // OK-2009-01-19: Refactored to ensure behaviour is the same as the mouseControl.
    if (MCmousestackptr != nil)
    {
        r_object . object = MCmousestackptr->getcard()->getmousecontrol();
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

void MCInterfaceEvalFocusedObjectAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = MCdefaultstackptr->getcard()->getkfocused();
    
    if (t_object == nil)
        t_object = MCdefaultstackptr->getcard();
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
}

static MCStack *MCInterfaceTryToEvalBinaryStack(MCStringRef p_data, bool& r_binary_fail)
{
    uint4 offset;
    MCStack *t_stack;
    bool t_binary_fail;
    
    t_stack = nil;
    t_binary_fail = false;
    
    if (MCU_offset(SIGNATURE, MCStringGetCString(p_data), offset) && (MCStringGetLength(p_data) > 8 && strncmp(MCStringGetCString(p_data), "REVO", 4) == 0))
    {
        IO_handle stream = MCS_fakeopen(MCStringGetOldString(p_data));
        /* UNCHECKED */ MCdispatcher->readfile(NULL, NULL, stream, t_stack);
        MCS_close(stream);
        t_binary_fail = t_stack == nil;
    }

    r_binary_fail = t_binary_fail;
    return t_stack;
}
                                           
void MCInterfaceEvalBinaryStackAsObject(MCExecContext& ctxt, MCStringRef p_data, MCObjectPtr& r_object)
{
    MCStack *t_stack;
    bool t_binary_fail;
    
    t_stack = MCInterfaceTryToEvalBinaryStack(p_data, t_binary_fail);
    
    if (t_stack != nil)
    {
        r_object . object = t_stack;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalDefaultStackAsObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    if (MCdefaultstackptr != nil)
    {
        r_object . object = MCdefaultstackptr;
        r_object . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalStackOfStackByName(MCExecContext& ctxt, MCObjectPtr p_parent, MCNameRef p_name, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_parent . object) -> findstackname(p_name);
    if (t_stack != nil)
    {
        r_stack . object = t_stack;
        r_stack . part_id = p_parent . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalStackOfStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_parent . object) -> findstackid(p_id);
    
    if (t_stack != nil)
    {
        r_stack . object = t_stack;
        r_stack . part_id = p_parent . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalStackByValue(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_stack)
{
    uint4 offset;
    if (MCU_offset(SIGNATURE, MCStringGetCString((MCStringRef)p_value), offset) && (MCStringGetLength((MCStringRef)p_value) > 8 && strncmp(MCStringGetCString((MCStringRef)p_value), "REVO", 4) == 0))
    {
        MCInterfaceEvalBinaryStackAsObject(ctxt, (MCStringRef)p_value, r_stack);
        return;
    }
    
    MCStack *t_stack;
    
    integer_t t_id;                            
    if (MCU_stoi4((MCStringRef)p_value, t_id))
    {
        t_stack = MCdefaultstackptr -> findstackid(t_id);
        if (t_stack != nil)
        {
            r_stack . object = t_stack;
            r_stack . part_id = 0;
            return;
        }
    }
    
	t_stack = MCdefaultstackptr -> findstackname((MCNameRef)p_value);
	if (t_stack != nil)
    {
        r_stack . object = t_stack;
        r_stack . part_id = 0;
        return;
    }

    MCEngineEvalValueAsObject(ctxt, p_value, r_stack);
}

void MCInterfaceEvalSubstackOfStackByName(MCExecContext& ctxt, MCObjectPtr p_parent, MCNameRef p_name, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
	t_stack = static_cast<MCStack *>(p_parent . object) -> findsubstackname(p_name);
    if (t_stack != nil)
    {
        r_stack . object = t_stack;
        r_stack . part_id = p_parent . part_id;
        return;
    }

    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalSubstackOfStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_parent . object) -> findsubstackid(p_id);
    
    if (t_stack != nil)
    {
        r_stack . object = t_stack;
        r_stack . part_id = p_parent . part_id;
        return;
    }

    ctxt . LegacyThrow(EE_CHUNK_NOSTACK);
}

void MCInterfaceEvalAudioClipOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_ordinal_type, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    t_clip = static_cast<MCStack *>(p_stack . object) -> getAV((Chunk_term)p_ordinal_type, kMCEmptyString, CT_AUDIO_CLIP);
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalAudioClipOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    t_clip = static_cast<MCStack *>(p_stack . object) -> getAVid(CT_AUDIO_CLIP, p_id);
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalAudioClipOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    
    if (!static_cast<MCStack *>(p_stack . object) -> getAVname(CT_AUDIO_CLIP, p_name, t_clip) &&
        (MCacptr != NULL && MCacptr -> hasname(p_name)))
            t_clip = MCacptr;
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalVideoClipOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_ordinal_type, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    t_clip = static_cast<MCStack *>(p_stack . object) -> getAV((Chunk_term)p_ordinal_type, kMCEmptyString, CT_VIDEO_CLIP);
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalVideoClipOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    t_clip = static_cast<MCStack *>(p_stack . object) -> getAVid(CT_VIDEO_CLIP, p_id);
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalVideoClipOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_clip)
{
    MCObject *t_clip;
    
    if (!static_cast<MCStack *>(p_stack . object) -> getAVname(CT_VIDEO_CLIP, p_name, t_clip))
    {
        IO_cleanprocesses();
        MCPlayer *tptr = MCplayers;
        while (tptr != NULL)
        {
            if (tptr -> hasname(p_name))
            {
                t_clip = tptr;
                break;
            }
            tptr = tptr->getnextplayer();
        }
    }
    
    if (t_clip != nil)
    {
        r_clip . object = t_clip;
        r_clip . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalBackgroundOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, Chunk_term p_ordinal_type, MCObjectPtr& r_bg)
{
    MCGroup *t_background;
    t_background = static_cast<MCStack *>(p_stack . object) -> getbackgroundbyordinal(p_ordinal_type);
    
    if (t_background != nil)
    {
        r_bg . object = t_background;
        r_bg . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalBackgroundOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_bg)
{
    MCGroup *t_background;
    t_background =  static_cast<MCStack *>(p_stack . object) -> getbackgroundbyid(p_id);
    
    if (t_background != nil)
    {
        r_bg . object = t_background;
        r_bg . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalBackgroundOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_bg)
{
    MCGroup *t_background;
    t_background = static_cast<MCStack *>(p_stack . object) -> getbackgroundbyname(p_name);
    
    if (t_background != nil)
    {
        r_bg . object = t_background;
        r_bg . part_id = p_stack . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalStackWithBackgroundByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, Chunk_term p_ordinal_type, MCObjectPtr& r_stack)
{
    MCGroup *t_background;
    MCStack *t_stack;
    t_stack =  static_cast<MCStack *>(p_stack . object);
    t_background = t_stack -> getbackgroundbyordinal(p_ordinal_type);
    
    if (t_background != nil)
    {
        t_stack -> setbackground(t_background);
        r_stack . object = t_stack;
        r_stack . part_id = p_stack . part_id;
    }
}

void MCInterfaceEvalStackWithBackgroundById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCGroup *t_background;
    MCStack *t_stack;
    t_stack =  static_cast<MCStack *>(p_stack . object);
    t_background = t_stack -> getbackgroundbyid(p_id);
    
    if (t_background != nil)
    {
        t_stack -> setbackground(t_background);
        r_stack . object = t_stack;
        r_stack . part_id = p_stack . part_id;
    }
}

void MCInterfaceEvalStackWithBackgroundByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_stack)
{
    MCGroup *t_background;
    MCStack *t_stack;
    t_stack =  static_cast<MCStack *>(p_stack . object);
    t_background = t_stack -> getbackgroundbyname(p_name);
    
    if (t_background != nil)
    {
        t_stack -> setbackground(t_background);
        r_stack . object = t_stack;
        r_stack . part_id = p_stack . part_id;
    }
}

void MCInterfaceEvalCardOfStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, Chunk_term p_ordinal_type, MCObjectPtr& r_card)
{
    MCCard *t_card;
    
    if (p_ordinal_type == CT_RECENT)
        t_card = MCrecent->getrel(-1);
    else
    {
        MCStack *t_stack;
        t_stack = static_cast<MCStack *>(p_stack . object);
        if (p_marked)
            t_stack -> setmark();
        t_card = t_stack -> getchild(p_ordinal_type, kMCEmptyString, CT_CARD);
        t_stack -> clearmark();
    }
    
    if (t_card != nil)
    {
        r_card . object = t_card;
        r_card . part_id = t_card -> getid();
        return;
    }

    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalThisCardOfStack(MCExecContext& ctxt, MCObjectPtr p_stack, MCObjectPtr& r_card)
{
    // if a target object has been evaluated, then this method should not be called.
    // Since this is run-time information, instead we call the method but return the
    // previously evaluated object.
    if (p_stack . object -> gettype() == CT_CARD || p_stack . object -> gettype() == CT_GROUP)
    {
        r_card . object = p_stack . object;
        r_card . part_id = p_stack . part_id;
        return;
    }
    
    MCInterfaceEvalCardOfStackByOrdinal(ctxt, p_stack, false, CT_THIS, r_card);
}

void MCInterfaceEvalCardOfStackById(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, uinteger_t p_id, MCObjectPtr& r_card)
{
    MCCard *t_card;
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_stack . object);
    
    if (p_marked)
        t_stack -> setmark();
    t_card = t_stack -> getchildbyid(p_id);
    t_stack -> clearmark();
    
    if (t_card != nil)
    {
        r_card . object = t_card;
        r_card . part_id = t_card -> getid();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalCardOfStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, MCNameRef p_name, MCObjectPtr& r_card)
{
    if (MCStringIsEqualToCString(MCNameGetString(p_name), "window", kMCCompareExact))
    {
        MCInterfaceEvalThisCardOfStack(ctxt, p_stack, r_card);
        return;
    }
    
    MCCard *t_card;
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_stack . object);
    
    if (p_marked)
        t_stack -> setmark();
    t_card = t_stack -> getchildbyname(p_name);
    t_stack -> clearmark();
    
    if (t_card != nil)
    {
        r_card . object = t_card;
        r_card . part_id = t_card -> getid();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalCardOfBackgroundByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack_with_background, bool p_marked, Chunk_term p_ordinal_type, MCObjectPtr& r_card)
{
    MCCard *t_card;
    t_card = nil;
    
    if (p_ordinal_type == CT_RECENT)
        t_card = MCrecent->getrel(-1);
    else
    {
        MCStack *t_stack;
        t_stack = static_cast<MCStack *>(p_stack_with_background . object);
        
        if (t_stack != nil)
        {
            MCObjectPtr t_objptr;
            t_objptr . object = t_stack;
            t_objptr . part_id = 0;
            MCInterfaceEvalCardOfStackByOrdinal(ctxt, t_objptr, p_marked, p_ordinal_type, r_card);
            t_stack -> clearbackground();
            return;
        }
    }
    
    if (t_card != nil)
    {
        r_card . object = t_card;
        r_card . part_id = t_card -> getid();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalCardOfBackgroundById(MCExecContext& ctxt, MCObjectPtr p_stack_with_background, bool p_marked, uinteger_t p_id, MCObjectPtr& r_card)
{
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_stack_with_background . object);
    
    if (t_stack != nil)
    {
        MCObjectPtr t_objptr;
        t_objptr . object = t_stack;
        t_objptr . part_id = 0;
        MCInterfaceEvalCardOfStackById(ctxt, t_objptr, p_marked, p_id, r_card);
        t_stack -> clearbackground();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalCardOfBackgroundByName(MCExecContext& ctxt, MCObjectPtr p_stack_with_background, bool p_marked, MCNameRef p_name, MCObjectPtr& r_card)
{
    MCStack *t_stack;
    t_stack = static_cast<MCStack *>(p_stack_with_background . object);
    
    if (t_stack != nil)
    {
        MCObjectPtr t_objptr;
        t_objptr . object = t_stack;
        t_objptr . part_id = 0;
        MCInterfaceEvalCardOfStackByName(ctxt, t_objptr, p_marked, p_name, r_card);
        t_stack -> clearbackground();
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOCARD);
}

void MCInterfaceEvalGroupOfCardByOrdinal(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, Chunk_term p_ordinal_type, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCCard *>(p_card . object) -> getchildbyordinal(p_ordinal_type, CT_GROUP, p_parent_type);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalGroupOfCardById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCCard *>(p_card . object) -> getchildbyid(p_id, CT_GROUP, p_parent_type);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}


void MCInterfaceEvalGroupOfCardByName(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, MCNameRef p_name, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCCard *>(p_card . object) -> getchildbyname(p_name, CT_GROUP, p_parent_type);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalGroupOfCardOrStackById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_group)
{
    MCControl *t_group;
    uinteger_t t_part_id;
    MCCard *t_card;
    t_card = static_cast<MCCard *>(p_card . object);
    
    t_group = t_card -> getchildbyid(p_id, CT_GROUP, p_parent_type);
    
    if (t_group == nil)
    {
        MCStack *t_stack;
        t_stack = t_card -> getstack();
        if (t_stack != nil)
            t_group = t_stack -> getcontrolid(CT_GROUP, p_id, true);
        
        t_part_id = 0;
    }
    else
        t_part_id = t_card -> getid();
    
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = t_part_id;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalGroupOfGroupByOrdinal(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_ordinal_type, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCGroup *>(p_group . object) -> getchildbyordinal(p_ordinal_type, CT_GROUP);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalGroupOfGroupById(MCExecContext& ctxt, MCObjectPtr p_group, uinteger_t p_id, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCGroup *>(p_group . object) -> getchildbyid(p_id, CT_GROUP);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalGroupOfGroupByName(MCExecContext& ctxt, MCObjectPtr p_group, MCNameRef p_name, MCObjectPtr& r_group)
{
    MCControl *t_group;
    t_group = static_cast<MCGroup *>(p_group.  object) -> getchildbyname(p_name, CT_GROUP);
    
    if (t_group != nil)
    {
        r_group . object = t_group;
        r_group . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalMenubarAsObject(MCExecContext& ctxt, MCObjectPtr& r_menubar)
{
    if (MCmenubar != nil)
    {
        r_menubar . object = MCmenubar;
        r_menubar . part_id = 0;
        return;
    }
    
    if (MCdefaultmenubar != nil)
    {
        r_menubar . object = MCdefaultmenubar;
        r_menubar . part_id = 0;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOBACKGROUND);
}

void MCInterfaceEvalObjectOfGroupByOrdinal(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, Chunk_term p_ordinal_type, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCGroup *>(p_group.  object) -> getchildbyordinal(p_ordinal_type, p_object_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfGroupById(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, uinteger_t p_id, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCGroup *>(p_group.  object) -> getchildbyid(p_id, p_object_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfGroupByName(MCExecContext& ctxt, MCObjectPtr p_group, Chunk_term p_object_type, MCNameRef p_name, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCGroup *>(p_group.  object) -> getchildbyname(p_name, p_object_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_group . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfCardByOrdinal(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, Chunk_term p_ordinal_type, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCCard *>(p_card . object) -> getchildbyordinal(p_ordinal_type, p_object_type, p_parent_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfCardById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCCard *>(p_card . object) -> getchildbyid(p_id, p_object_type, p_parent_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfCardOrStackById(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, uinteger_t p_id, MCObjectPtr& r_object)
{
    MCObject *t_object;
    uinteger_t t_part_id;
    MCCard *t_card;
    t_card = static_cast<MCCard *>(p_card . object);
    
    t_object = t_card -> getchildbyid(p_id, p_object_type, p_parent_type);
    
    if (t_object == nil)
    {
        MCStack *t_stack;
        t_stack = t_card -> getstack();
        if (t_stack != nil)
            t_object = t_stack -> getcontrolid(p_object_type, p_id, true);
        
        t_part_id = 0;
    }
    else
        t_part_id = t_card -> getid();
    
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = t_part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalObjectOfCardByName(MCExecContext& ctxt, MCObjectPtr p_card, Chunk_term p_object_type, Chunk_term p_parent_type, MCNameRef p_name, MCObjectPtr& r_object)
{
    MCObject *t_object;
    t_object = static_cast<MCCard *>(p_card . object) -> getchildbyname(p_name, p_object_type, p_parent_type);
    
    if (t_object != nil)
    {
        r_object . object = t_object;
        r_object . part_id = p_card . part_id;
        return;
    }
    
    ctxt . LegacyThrow(EE_CHUNK_NOOBJECT);
}

void MCInterfaceEvalStackOfObject(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_object)
{
    if (p_object . object -> gettype() == CT_STACK)
    {
        r_object . object = p_object . object;
        r_object . part_id = p_object . part_id;
        return;
    }
    
    MCCard *t_card;
    t_card = p_object . object -> getcard();
    
    r_object . object = t_card -> getstack();
    r_object . part_id = p_object . part_id;
}

void MCInterfaceEvalStackWithOptionalBackground(MCExecContext& ctxt, MCObjectPtr p_object, MCObjectPtr& r_object)
{
    if (p_object . object -> gettype() == CT_GROUP)
    {
        MCStack *sptr;
        MCGroup *bptr;
        bptr = static_cast<MCGroup *>(p_object . object);
        sptr = bptr -> getcard() -> getstack();
        sptr -> setbackground(bptr);
        r_object . object = sptr;
        r_object . part_id = p_object . part_id;
        return;
    }
    else if (p_object . object -> gettype() == CT_CARD)
    {
        r_object . object = p_object . object -> getstack();
        r_object . part_id = 0;
        return;
    }

    r_object . object = p_object . object;
    r_object . part_id = p_object . part_id;
}

////////////////////////////////////////////////////////////////////////////////

// The following are used by MCGo, and do not throw any errors. If the incoming object pointer is nil, they return nil.

void MCInterfaceEvalDefaultStackAsOptionalObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    ctxt . SetTheResultToEmpty();
    r_object . object = MCdefaultstackptr;
    r_object . part_id = 0;
}

void MCInterfaceEvalHomeStackAsOptionalObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    ctxt . SetTheResultToEmpty();
    r_object . object = MCdispatcher -> gethome();
    r_object . part_id = 0;
}

void MCInterfaceEvalHelpStackAsOptionalObject(MCExecContext& ctxt, MCObjectPtr& r_object)
{
    ctxt . SetTheResultToEmpty();
    r_object . object = MCdefaultstackptr->findstackname(MCM_help);
    r_object . part_id = 0;
}

void MCInterfaceEvalStackOfOptionalStackByName(MCExecContext& ctxt, MCNameRef p_name, MCObjectPtr p_parent, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_parent . object != nil)
    {
        t_stack = static_cast<MCStack *>(p_parent . object) -> findstackname(p_name);
    }
    
    r_stack . object = t_stack;
    r_stack . part_id = p_parent . part_id;
}

void MCInterfaceEvalStackOfOptionalStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_parent . object != nil)
        t_stack = static_cast<MCStack *>(p_parent . object) -> findstackid(p_id);
    
    r_stack . object = t_stack;
    r_stack . part_id = p_parent . part_id;
}

void MCInterfaceEvalOptionalStackOrCardByValue(MCExecContext& ctxt, MCValueRef p_value, MCObjectPtr& r_object)
{
    ctxt . SetTheResultToEmpty();
    
    MCStack *t_stack;
    bool t_binary_fail;
    
    t_stack = MCInterfaceTryToEvalBinaryStack((MCStringRef)p_value, t_binary_fail);
    
    if (t_binary_fail)
    {
        ctxt . SetTheResultToStaticCString("can't build stack from string");
        r_object . object = nil;
        r_object . part_id = 0;
        return;
    }
    
    if (t_stack == nil)
    {
        integer_t t_id;
        if (MCU_stoi4((MCStringRef)p_value, t_id))
            t_stack = MCdefaultstackptr -> findstackid(t_id);
    }
    
    if (t_stack == nil)
        t_stack = MCdefaultstackptr -> findstackname((MCNameRef)p_value);
    
    if (t_stack != nil)
    {
        r_object . object = t_stack;
        r_object . part_id = 0;
        return;
    }
    
    bool t_parse_error;
    
    if (!MCEngineEvalValueAsObject(p_value, false, r_object, t_parse_error))
    {
        if (t_parse_error)
            ctxt . SetTheResultToStaticCString("no such card");
        r_object . object = nil;
        r_object . part_id = 0;
    }
}

void MCInterfaceEvalSubstackOfOptionalStackByName(MCExecContext& ctxt, MCObjectPtr p_parent, MCNameRef p_name, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_parent . object != nil)
        t_stack = static_cast<MCStack *>(p_parent . object) -> findsubstackname(p_name);

    r_stack . object = t_stack;
    r_stack . part_id = p_parent . part_id;

}

void MCInterfaceEvalSubstackOfOptionalStackById(MCExecContext& ctxt, MCObjectPtr p_parent, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_parent . object != nil)
        t_stack = static_cast<MCStack *>(p_parent . object) -> findsubstackid(p_id);
    
    r_stack . object = t_stack;
    r_stack . part_id = p_parent . part_id;
    return;
}

void MCInterfaceEvalOptionalStackWithBackgroundByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, Chunk_term p_ordinal_type, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_stack . object != nil)
    {
        MCGroup *t_background;
        
        t_stack = static_cast<MCStack *>(p_stack . object);
        t_background = t_stack -> getbackgroundbyordinal(p_ordinal_type);
        t_stack -> setbackground(t_background);
    }
    
    r_stack . object = t_stack;
    r_stack . part_id = p_stack . part_id;
}

void MCInterfaceEvalOptionalStackWithBackgroundById(MCExecContext& ctxt, MCObjectPtr p_stack, uinteger_t p_id, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_stack . object != nil)
    {
        MCGroup *t_background;
        
        t_stack = static_cast<MCStack *>(p_stack . object);
        t_background =  t_stack -> getbackgroundbyid(p_id);
        t_stack -> setbackground(t_background);
    }
    
    r_stack . object = t_stack;
    r_stack . part_id = p_stack . part_id;
}

void MCInterfaceEvalOptionalStackWithBackgroundByName(MCExecContext& ctxt, MCObjectPtr p_stack, MCNameRef p_name, MCObjectPtr& r_stack)
{
    MCStack *t_stack;
    t_stack = nil;
    
    if (p_stack . object != nil)
    {
        MCGroup *t_background;
        
        t_stack = static_cast<MCStack *>(p_stack . object);
        t_background = t_stack -> getbackgroundbyname(p_name);
    }
        
    r_stack . object = t_stack;
    r_stack . part_id = p_stack . part_id;
}

void MCInterfaceEvalCardOfOptionalStackByOrdinal(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, Chunk_term p_ordinal_type, MCObjectPtr& r_card)
{
    MCCard *t_card;
    t_card = nil;
    
    if (p_stack . object != nil)
    {
        if (p_ordinal_type == CT_RECENT)
            t_card = MCrecent->getrel(-1);
        else
        {
            MCStack *t_stack;
            t_stack = static_cast<MCStack *>(p_stack . object);
            if (p_marked)
                t_stack -> setmark();
            t_card = t_stack -> getchild(CT_ID, kMCEmptyString, p_ordinal_type);
            t_stack -> clearmark();
            t_stack -> clearbackground();
        }
    }
    
    r_card . object = t_card;
    r_card . part_id = t_card != nil ? t_card -> getid() : p_stack . part_id;
}

void MCInterfaceEvalThisCardOfOptionalStack(MCExecContext& ctxt, MCObjectPtr p_stack, MCObjectPtr& r_card)
{
    MCInterfaceEvalCardOfOptionalStackByOrdinal(ctxt, p_stack, false, CT_THIS, r_card);
}

void MCInterfaceEvalCardOfOptionalStackById(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, uinteger_t p_id, MCObjectPtr& r_card)
{
    MCCard *t_card;
    t_card = nil;
    
    if (p_stack . object != nil)
    {
        MCStack *t_stack;
        t_stack = static_cast<MCStack *>(p_stack . object);
        
        if (p_marked)
            t_stack -> setmark();
        t_card = t_stack -> getchildbyid(p_id);
        t_stack -> clearmark();
        t_stack -> clearbackground();
    }
    
    r_card . object = t_card;
    r_card . part_id = t_card != nil ? t_card -> getid() : p_stack . part_id;
}

void MCInterfaceEvalCardOfOptionalStackByName(MCExecContext& ctxt, MCObjectPtr p_stack, bool p_marked, MCNameRef p_name, MCObjectPtr& r_card)
{
    if (MCStringIsEqualToCString(MCNameGetString(p_name), "window", kMCCompareExact))
    {
        MCInterfaceEvalThisCardOfOptionalStack(ctxt, p_stack, r_card);
        return;
    }
    
    MCCard *t_card;
    t_card = nil;
    
    if (p_stack . object != nil)
    {
        MCStack *t_stack;
        t_stack = static_cast<MCStack *>(p_stack . object);
        
        if (p_marked)
            t_stack -> setmark();
        r_card . object = t_stack -> getchildbyname(p_name);
        t_stack -> clearmark();
        t_stack -> clearbackground();
    }

    r_card . object = t_card;
    r_card . part_id = t_card != nil ? t_card -> getid() : p_stack . part_id;
}

void MCInterfaceMarkObject(MCExecContext& ctxt, MCObjectPtr p_object, Boolean wholechunk, MCMarkedText& r_mark)
{
    if (p_object . object -> gettype() == CT_FIELD || p_object . object -> gettype() == CT_BUTTON)
    {
        p_object . object -> getstringprop(ctxt, p_object . part_id, P_TEXT, False, r_mark . text);
        r_mark . start = 0;
        r_mark . finish = MCStringGetLength(r_mark . text);
    	return;
    }
    r_mark . text = nil;
}

void MCInterfaceMarkContainer(MCExecContext& ctxt, MCObjectPtr p_container, MCMarkedText& r_mark)
{
    switch (p_container . object -> gettype())
    {
        case CT_FIELD:
        case CT_BUTTON:
            MCInterfaceMarkObject(ctxt, p_container, false, r_mark);
            return;
        case CT_IMAGE:
        case CT_AUDIO_CLIP:
        case CT_VIDEO_CLIP:
            p_container . object -> getstringprop(ctxt, p_container . part_id, P_TEXT, False, r_mark . text);
            r_mark . start = 0;
            r_mark . finish = MCStringGetLength(r_mark . text);
            return;
        default:
            break;
    }
    
    r_mark . text = nil;
    ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
}

void MCInterfaceMarkFunction(MCExecContext& ctxt, MCObjectPtr p_object, Functions p_function, bool p_whole_chunk, MCMarkedText& r_mark)
{
    if (p_object . object -> gettype() != CT_FIELD)
        return;
    
    MCInterfaceMarkObject(ctxt, p_object, p_whole_chunk, r_mark);
    
    MCField *t_field;
    t_field = (MCField *)p_object . object;
    // MW-2012-12-13: [[ Bug 10592 ]] If wholechunk is False then we don't expand
    //   line chunks to include the CR at the end.
    
    int4 start, end;
    
    start = r_mark . start;
    end = r_mark . finish;
    
	Boolean wholeline = True;
	Boolean wholeword = True;
	switch (p_function)
	{
        case F_CLICK_CHAR_CHUNK:
            wholeword = False;
        case F_CLICK_CHUNK:
        case F_CLICK_TEXT:
            wholeline = False;
        case F_CLICK_LINE:
            if (!t_field->locmark(wholeline, wholeword, True, True, p_whole_chunk, start, end))
                start = end = 0;
            break;
        case F_FOUND_CHUNK:
        case F_FOUND_TEXT:
            wholeline = False;
        case F_FOUND_LINE:
            if (!t_field->foundmark(wholeline, p_whole_chunk, start, end))
                start = end = 0;
            break;
        case F_SELECTED_CHUNK:
        case F_SELECTED_TEXT:
            wholeline = False;
        case F_SELECTED_LINE:
            if (!t_field->selectedmark(wholeline, start, end, False, p_whole_chunk))
                start = end = 0;
            break;
        case F_MOUSE_CHAR_CHUNK:
            wholeword = False;
        case F_MOUSE_CHUNK:
        case F_MOUSE_TEXT:
            wholeline = False;
        case F_MOUSE_LINE:
            if (!t_field->locmark(wholeline, wholeword, False, True, p_whole_chunk, start, end))
                start = end = 0;
            break;
        default:
            start = 0;
            end = t_field->getpgsize(NULL);
            break;
	}
    r_mark . start = start;
    r_mark . finish  = end;
}

void MCInterfaceEvalTextOfContainer(MCExecContext& ctxt, MCObjectPtr p_container, MCStringRef &r_text)
{
    switch (p_container . object -> gettype())
    {
        case CT_BUTTON:
        case CT_IMAGE:
        case CT_AUDIO_CLIP:
        case CT_VIDEO_CLIP:
            p_container . object -> getstringprop(ctxt, p_container . part_id, P_TEXT, False, r_text);
            return;
        default:
            ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
    }
}