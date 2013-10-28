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

#include "scriptpt.h"
#include "iquantization.h"
#include "stacksecurity.h"

#include "exec-interface.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////
MC_EXEC_DEFINE_MAKE_METHOD(Interface, CustomImagePaletteSettings, 2)
MC_EXEC_DEFINE_MAKE_METHOD(Interface, OptimalImagePaletteSettings, 2)
MC_EXEC_DEFINE_MAKE_METHOD(Interface, WebSafeImagePaletteSettings, 1)
MC_EXEC_DEFINE_MAKE_METHOD(Interface, VisualEffect, 8)
MC_EXEC_DEFINE_MAKE_METHOD(Interface, VisualEffectArgument, 4)

MC_EXEC_DEFINE_EVAL_METHOD(Interface, ScreenColors, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ScreenDepth, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ScreenName, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ScreenRect, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ScreenLoc, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickH, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickV, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickLoc, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickChar, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickText, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickCharChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickLine, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickField, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ClickStack, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, Mouse, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseClick, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseColor, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseH, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseV, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseLoc, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseChar, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseText, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseCharChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseLine, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseControl, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MouseStack, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundText, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundField, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundLine, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FoundLoc, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedText, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedTextOf, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedChunk, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedChunkOf, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedLine, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedLineOf, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedLoc, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedLocOf, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedField, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedImage, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, SelectedObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CapsLockKey, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, CommandKey, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ControlKey, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, OptionKey, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ShiftKey, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, KeysDown, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MainStacks, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, OpenStacks, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, Stacks, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, TopStack, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, TopStackOf, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FocusedObject, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ColorNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, FlushEvents, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, GlobalLoc, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, LocalLoc, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, MovingControls, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, WaitDepth, 1)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, Intersect, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, IntersectWithThreshold, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, Within, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ThereIsAnObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ThereIsNotAnObject, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ControlAtLoc, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Interface, ControlAtScreenLoc, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Beep, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ClickCmd, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CloseStack, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CloseDefaultStack, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Drag, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, FocusOnNothing, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, FocusOn, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Grab, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GroupControls, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GroupSelection, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PopToLast, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Pop, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PushRecentCard, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PushCurrentCard, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PushCard, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PlaceGroupOnCard, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, RemoveGroupFromCard, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ResetCursors, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ResetTemplate, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Revert, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SelectEmpty, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SelectAllTextOfField, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SelectTextOfField, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SelectTextOfButton, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SelectObjects, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, StartEditingGroup, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, StopEditingDefaultStack, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, StopEditingGroup, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, StopMovingObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Type, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Undo, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UngroupObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UngroupSelection, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CopyObjectsToContainer, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CutObjectsToContainer, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Delete, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DeleteObjects, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DeleteObjectChunks, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DisableChunkOfButton, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, EnableChunkOfButton, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnhiliteChunkOfButton, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HiliteChunkOfButton, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DisableObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, EnableObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnhiliteObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HiliteObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SaveStack, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SaveStackAs, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, MoveObjectBetween, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, MoveObjectAlong, 8)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HideGroups, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HideObject, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HideObjectWithEffect, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HideMenuBar, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, HideTaskBar, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowGroups, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowAllCards, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowMarkedCards, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowCards, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowObject, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowObjectWithEffect, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowMenuBar, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ShowTaskBar, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PopupButton, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DrawerStack, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DrawerStackByName, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DrawerStackLegacy, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, DrawerStackByNameLegacy, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SheetStack, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SheetStackByName, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, OpenStack, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, OpenStackByName, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PopupStack, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PopupStackByName, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CreateStack, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CreateStackWithGroup, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CreateCard, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, CreateControl, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Clone, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, Find, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PutIntoObject, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, PutIntoField, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockCursor, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockMenus, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockMoves, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockRecent, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockScreen, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, LockScreenForEffect, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockCursor, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockMenus, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockMoves, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockRecent, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockScreen, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, UnlockScreenWithEffect, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportSnapshotOfScreen, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportSnapshotOfStack, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportSnapshotOfObject, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportAudioClip, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportVideoClip, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ImportImage, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfScreen, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfScreenToFile, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfStack, 6)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfStackToFile, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfObject, 7)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportSnapshotOfObjectToFile, 8)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportImage, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ExportImageToFile, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SortCardsOfStack, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SortField, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, SortContainer, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, ChooseTool, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoCardAsMode, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoCardInWindow, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoRecentCard, 0)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoCardRelative, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoCardEnd, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, GoHome, 1)
MC_EXEC_DEFINE_EXEC_METHOD(Interface, VisualEffect, 1)

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCInterfaceWindowPositionElementInfo[] =
{
	{ "", WP_DEFAULT, false },
	{ "right", WP_PARENTRIGHT, false },
	{ "left", WP_PARENTLEFT, false },
	{ "top", WP_PARENTTOP, false },
	{ "bottom", WP_PARENTBOTTOM, false },
};

static MCExecEnumTypeInfo _kMCInterfaceWindowPositionTypeInfo =
{
	"Interface.WindowPosition",
	sizeof(_kMCInterfaceWindowPositionElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceWindowPositionElementInfo
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceWindowAlignmentElementInfo[] =
{
	{ "center", OP_CENTER, false },
	{ "right", OP_RIGHT, false },
	{ "left", OP_LEFT, false },
	{ "top", OP_TOP, false },
	{ "bottom", OP_BOTTOM, false },
};

static MCExecEnumTypeInfo _kMCInterfaceWindowAlignmentTypeInfo =
{
	"Interface.WindowAlignment",
	sizeof(_kMCInterfaceWindowAlignmentElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceWindowAlignmentElementInfo
};

//////////

void MCInterfaceImagePaletteSettingsFree(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& p_settings)
{
	if (p_settings . type == kMCImagePaletteTypeCustom)
		MCMemoryDeleteArray(p_settings . custom . colors);
}

static MCExecCustomTypeInfo _kMCInterfaceImagePaletteSettingsTypeInfo =
{
	"Interface.ImagePaletteSettings",
	sizeof(MCInterfaceImagePaletteSettings),
	(void *)nil,
	(void *)nil,
	(void *)MCInterfaceImagePaletteSettingsFree
};

//////////

static void MCInterfaceVisualEffectArgumentCopy(MCExecContext& ctxt, MCInterfaceVisualEffectArgument p_source, MCInterfaceVisualEffectArgument& r_target)
{
	r_target . key = (MCStringRef)MCValueRetain(p_source . key);
	r_target . value = (MCStringRef)MCValueRetain(p_source . value);
}

void MCInterfaceVisualEffectArgumentFree(MCExecContext& ctxt, MCInterfaceVisualEffectArgument& p_arg)
{
	MCValueRelease(p_arg . key);
	MCValueRelease(p_arg . value);
}

static MCExecCustomTypeInfo _kMCInterfaceVisualEffectArgumentTypeInfo =
{
	"Interface.VisualEffectArgument",
	sizeof(MCInterfaceVisualEffectArgument),
	(void *)nil,
	(void *)nil,
	(void *)MCInterfaceVisualEffectArgumentFree
};

//////////

void MCInterfaceVisualEffectFree(MCExecContext& ctxt, MCInterfaceVisualEffect& p_effect)
{
	MCValueRelease(p_effect . name);
	MCValueRelease(p_effect . sound);
	for (uindex_t i = 0; i < p_effect . nargs; i++)
		MCInterfaceVisualEffectArgumentFree(ctxt, p_effect . arguments[i]);
}

static MCExecCustomTypeInfo _kMCInterfaceVisualEffectTypeInfo =
{
	"Interface.VisualEffect",
	sizeof(MCInterfaceVisualEffect),
	(void *)nil,
	(void *)nil,
	(void *)MCInterfaceVisualEffectFree
};

//////////

MCExecEnumTypeInfo *kMCInterfaceWindowPositionTypeInfo = &_kMCInterfaceWindowPositionTypeInfo;
MCExecEnumTypeInfo *kMCInterfaceWindowAlignmentTypeInfo = &_kMCInterfaceWindowAlignmentTypeInfo;

//////////

bool MCInterfaceTryToResolveObject(MCExecContext& ctxt, MCStringRef long_id, MCObjectPtr& r_object)
{
	bool t_found;
	t_found = false;
	
	MCExecPoint& ep = ctxt . GetEP();
	ep . setvalueref(long_id);
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(ep);
	if (tchunk->parse(sp, False) == PS_NORMAL)
	{
		if (tchunk->getobj(ep, r_object, True) == ES_NORMAL)
			t_found = true;
	}
	MCerrorlock--;
	delete tchunk;

	return t_found;
}

//////////

void MCInterfaceMakeCustomImagePaletteSettings(MCExecContext& ctxt, MCColor *colors, uindex_t color_count, MCInterfaceImagePaletteSettings& r_settings)
{
	if (color_count == 0)
	{
		ctxt . LegacyThrow(EE_EXPORT_BADPALETTE);
		return;
	}

	if (MCMemoryAllocate(color_count * sizeof(MCColor), r_settings . custom . colors))
	{
		for (uindex_t i = 0; i < color_count; i++)
			r_settings . custom . colors[i] = colors[i];
		r_settings. custom . count = color_count;
		r_settings . custom . colors = colors;
		r_settings . type = kMCImagePaletteTypeCustom;
		return;
	}

	ctxt . Throw();
}

void MCInterfaceMakeOptimalImagePaletteSettings(MCExecContext& ctxt, integer_t *count, MCInterfaceImagePaletteSettings& r_settings)
{
	if (count != nil)
	{
		if (*count < 1 || *count > 256)
		{
			ctxt . LegacyThrow(EE_EXPORT_BADPALETTESIZE);
			return;
		}
		r_settings . optimal . palette_size = (uinteger_t)count;
	}
	else
		r_settings . optimal . palette_size = 256;	

	r_settings . type = kMCImagePaletteTypeOptimal;
}

void MCInterfaceMakeWebSafeImagePaletteSettings(MCExecContext& ctxt, MCInterfaceImagePaletteSettings& r_settings)
{
	r_settings . type = kMCImagePaletteTypeEmpty;
}

//////////

void MCInterfaceMakeVisualEffect(MCExecContext& ctxt, MCStringRef name, MCStringRef sound, MCInterfaceVisualEffectArgument *effect_args, uindex_t count, Visual_effects type, Visual_effects direction, Visual_effects speed, Visual_effects image, MCInterfaceVisualEffect& r_effect)
{
	if (MCMemoryAllocate(count * sizeof(MCInterfaceVisualEffectArgument), r_effect . arguments))
	{
		for (uindex_t i = 0; i < count; i++)
			MCInterfaceVisualEffectArgumentCopy(ctxt, effect_args[i], r_effect . arguments[i]);
		r_effect . nargs = count;
		r_effect . name = (MCStringRef)MCValueRetain(name);
		if (sound != nil)
			r_effect . sound = (MCStringRef)MCValueRetain(sound);
		else
			r_effect . sound = (MCStringRef)MCValueRetain(kMCEmptyString);

		r_effect . type = type;
		r_effect . direction = direction;
		r_effect . speed = speed;
		r_effect . image = image;
		return;
	}

	ctxt . Throw();
}

void MCInterfaceMakeVisualEffectArgument(MCExecContext& ctxt, MCStringRef p_value, MCStringRef p_key, bool p_has_id, MCInterfaceVisualEffectArgument& r_arg)
{
	if (p_has_id)
	{
		if (!MCStringFormat(r_arg . value, "id %s", MCStringGetCString(p_key)))
		{
			ctxt . Throw();
			return;
		}
	}
	else
		r_arg . key = (MCStringRef)MCValueRetain(p_value);

	r_arg . key = (MCStringRef)MCValueRetain(p_key);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalScreenColors(MCExecContext& ctxt, real64_t& r_colors)
{
    r_colors = pow(2.0, MCscreen->getdepth());
}

void MCInterfaceEvalScreenDepth(MCExecContext& ctxt, integer_t& r_depth)
{
    r_depth = MCscreen->getdepth();
}

void MCInterfaceEvalScreenName(MCExecContext& ctxt, MCNameRef& r_name)
{
	r_name = MCValueRetain(MCscreen->getdisplayname());
}

void MCInterfaceEvalScreenRect(MCExecContext& ctxt, bool p_working, bool p_plural, bool p_effective, MCStringRef& r_string)
{
	if (p_plural)
	{
		MCInterfaceGetScreenRects(ctxt, p_working, p_effective, r_string);
		return;
	}
	else
	{
		MCRectangle t_rect;
		MCInterfaceGetScreenRect(ctxt, p_working, p_effective, t_rect);
		if (MCStringFormat(r_string, "%d,%d,%d,%d", t_rect.x, t_rect.y,
			t_rect.x + t_rect.width, t_rect.y + t_rect.height))
			return;
	}

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalScreenLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
    MCDisplay const *t_displays;
    MCscreen->getdisplays(t_displays, false);
    integer_t x, y;
    x = t_displays->viewport.x + (t_displays->viewport.width >> 1);
    y = t_displays->viewport.y + (t_displays->viewport.height >> 1);
    
    if (MCStringFormat(r_string, "%d,%d", x, y))
        return;
    
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalClickH(MCExecContext& ctxt, integer_t& r_value)
{
	r_value = MCclicklocx;
}

void MCInterfaceEvalClickV(MCExecContext& ctxt, integer_t& r_value)
{
	r_value = MCclicklocy;
}

void MCInterfaceEvalClickLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCStringFormat(r_string, "%d,%d", MCclicklocx, MCclicklocy))
		return;
	ctxt . Throw();
}

//////////

void MCInterfaceEvalClickChar(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCclickfield -> locchar(True, r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalClickText(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCclickfield -> loctext(True, r_string))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalClickCharChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCclickfield -> loccharchunk(True, r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalClickChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCclickfield -> locchunk(True, r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalClickLine(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCclickfield -> locline(True, r_string))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalClickField(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	// The get*prop() methods Throw() if an error occurs, so we can exit if an
	// exception happens.
	uinteger_t t_number;
	MCclickfield -> getuintprop(ctxt, 0, P_NUMBER, False, t_number);
	if (ctxt . HasError())
		return;

	// Construct the string and return if successful.
	if (MCStringFormat(r_string, MCclickfield->getparent()->gettype() == CT_CARD && MCclickfield->getstack()->hcaddress() ? "card field %d" : "field %d", t_number))
		return;

	// As MCStringFormat doesn't throw, we need to sync error state here.
	ctxt . Throw();
}

void MCInterfaceEvalClickStack(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCclickstackptr == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	// No need to check for errors since getstringprop() will Throw() if an
	// error occurs.
	MCclickstackptr -> getstringprop(ctxt, 0, P_LONG_NAME, False, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalMouseH(MCExecContext& ctxt, integer_t& r_value)
{
	int16_t x, y;
	MCscreen->querymouse(x, y);
	MCRectangle t_rect = MCdefaultstackptr->getrect();
	r_value = x - t_rect.x;
}

void MCInterfaceEvalMouseV(MCExecContext& ctxt, integer_t& r_value)
{
	int16_t x, y;
	MCscreen->querymouse(x, y);
	MCRectangle t_rect = MCdefaultstackptr->getrect();
	r_value = y - t_rect.y;
}

void MCInterfaceEvalMouseLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
	int16_t x, y;
	MCscreen->querymouse(x, y);
	MCRectangle t_rect = MCdefaultstackptr->getrect();
	if (MCStringFormat(r_string, "%d,%d", x - t_rect.x, y - t_rect.y))
		return;
	ctxt . Throw();
}

//////////

void MCInterfaceEvalMouseChar(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr != nil)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			if (fptr->locchar(False, r_string))
				return;

			ctxt . Throw();
			return;
		}
	}

	r_string = MCValueRetain(kMCEmptyString);
}

void MCInterfaceEvalMouseText(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr != nil)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			if (fptr->loctext(False, r_string))
				return;

			ctxt . Throw();
			return;
		}
	}

	r_string = MCValueRetain(kMCEmptyString);
}

//////////

void MCInterfaceEvalMouseCharChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr != nil)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			if (fptr->loccharchunk(False, r_string))
				return;

			ctxt . Throw();
			return;
		}
	}

	r_string = MCValueRetain(kMCEmptyString);
}

//////////

void MCInterfaceEvalMouseChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr != nil)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			if (fptr->locchunk(False, r_string))
				return;

			ctxt . Throw();
			return;
		}
	}

	r_string = MCValueRetain(kMCEmptyString);
}

//////////

void MCInterfaceEvalMouseLine(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr != nil)
	{
		MCControl *mfocused = MCmousestackptr->getcard()->getmfocused();
		if (mfocused != NULL && mfocused->gettype() == CT_FIELD)
		{
			MCField *fptr = (MCField *)mfocused;
			if (fptr->locline(False, r_string))
				return;

			ctxt . Throw();
			return;
		}
	}

	r_string = MCValueRetain(kMCEmptyString);
}

//////////

void MCInterfaceEvalMouseControl(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCControl *t_focused = nil;
	if (MCmousestackptr != nil)
		t_focused = MCmousestackptr->getcard()->getmousecontrol();

	if (t_focused == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	uinteger_t t_layer;
	t_focused -> getuintprop(ctxt, 0, P_LAYER, False, t_layer);
	if (ctxt . HasError())
		return;

	if (MCStringFormat(r_string, "control %d", t_layer))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalMouseStack(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCmousestackptr -> getstringprop(ctxt, 0, P_SHORT_NAME, False, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalFoundChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfoundfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCfoundfield -> foundchunk(r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalFoundText(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfoundfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCfoundfield -> foundtext(r_string))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalFoundLine(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfoundfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCfoundfield -> foundline(r_string))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalFoundField(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfoundfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	// The get*prop() methods Throw() if an error occurs, so we can exit if an
	// exception happens.
	uinteger_t t_number;
	MCfoundfield -> getuintprop(ctxt, 0, P_NUMBER, False, t_number);
	if (ctxt . HasError())
		return;

	// Construct the string and return if successful.
	if (MCStringFormat(r_string, "field %d", t_number))
		return;

	// As MCStringFormat doesn't throw, we need to sync error state here.
	ctxt . Throw();
}

//////////

void MCInterfaceEvalFoundLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfoundfield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCfoundfield->foundloc(r_string))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalSelectedChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactivefield == NULL)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCactivefield -> selectedchunk(r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalSelectedChunkOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string)
{
	switch (p_target . object -> gettype())
	{
	case CT_FIELD:
		{
			MCField *fptr = (MCField *)p_target . object;
			if (fptr->selectedchunk(r_string))
				return;
		}
		break;
	case CT_BUTTON:
		{
			MCButton *bptr = (MCButton *)p_target . object;
			if (bptr->selectedchunk(r_string))
				return;
		}
		break;
	default:
		ctxt.LegacyThrow(EE_SELECTED_BADSOURCE);
		return;
	}

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedLine(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactivefield == NULL)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCactivefield -> selectedline(r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalSelectedLineOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string)
{
	switch (p_target . object -> gettype())
	{
	case CT_FIELD:
		{
			MCField *fptr = (MCField *)p_target . object;
			if (fptr->selectedline(r_string))
				return;
		}
		break;
	case CT_BUTTON:
		{
			MCButton *bptr = (MCButton *)p_target . object;
			if (bptr->selectedline(r_string))
				return;
		}
		break;
	default:
		ctxt.LegacyThrow(EE_SELECTED_BADSOURCE);
		return;
	}

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedText(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactivefield == NULL)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCactivefield -> selectedtext(r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalSelectedTextOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string)
{
	switch (p_target . object -> gettype())
	{
	case CT_FIELD:
		{
			MCField *fptr = (MCField *)p_target . object;
			if (fptr->selectedtext(r_string))
				return;
		}
		break;
	case CT_BUTTON:
		{
			MCButton *bptr = (MCButton *)p_target . object;
			if (bptr->selectedtext(r_string))
				return;
		}
		break;
	default:
		ctxt.LegacyThrow(EE_SELECTED_BADSOURCE);
		return;
	}

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactivefield == NULL)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	if (MCactivefield -> selectedloc(r_string))
		return;

	ctxt . Throw();
}

void MCInterfaceEvalSelectedLocOf(MCExecContext& ctxt, MCObjectPtr p_target, MCStringRef& r_string)
{
	switch (p_target . object -> gettype())
	{
	case CT_FIELD:
		{
			MCField *fptr = (MCField *)p_target . object;
			if (fptr->selectedloc(r_string))
				return;
		}
		break;
	case CT_BUTTON:
		{
			r_string = MCValueRetain(kMCEmptyString);
			return;
		}
		break;
	default:
		ctxt.LegacyThrow(EE_SELECTED_BADSOURCE);
		return;
	}

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedField(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactivefield == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	uinteger_t t_number;
	MCactivefield -> getuintprop(ctxt, 0, P_NUMBER, False, t_number);
	if (ctxt . HasError())
		return;

	if (MCStringFormat(r_string, "field %d", t_number))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedImage(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCactiveimage == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	uinteger_t t_number;
	MCactiveimage -> getuintprop(ctxt, 0, P_NUMBER, False, t_number);
	if (ctxt . HasError())
		return;

	if (MCStringFormat(r_string, "image %d", t_number))
		return;

	ctxt . Throw();
}

//////////

void MCInterfaceEvalSelectedObject(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCselected->getids(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

MCNameRef MCInterfaceKeyConditionToName(bool condition)
{
	if (condition)
		return MCN_down;
	else
		return MCN_up;
}

void MCInterfaceEvalCapsLockKey(MCExecContext& ctxt, MCNameRef& r_result)
{
	r_result = MCInterfaceKeyConditionToName((MCscreen->querymods() & MS_CAPS_LOCK) != 0);
	MCValueRetain(r_result);
}

void MCInterfaceEvalCommandKey(MCExecContext& ctxt, MCNameRef& r_result)
{
	r_result = MCInterfaceKeyConditionToName((MCscreen->querymods() & MS_CONTROL) != 0);
	MCValueRetain(r_result);
}

void MCInterfaceEvalControlKey(MCExecContext& ctxt, MCNameRef& r_result)
{
	r_result = MCInterfaceKeyConditionToName((MCscreen->querymods() & MS_MAC_CONTROL) != 0);
	MCValueRetain(r_result);
}

void MCInterfaceEvalOptionKey(MCExecContext& ctxt, MCNameRef& r_result)
{
	r_result = MCInterfaceKeyConditionToName((MCscreen->querymods() & MS_MOD1) != 0);
	MCValueRetain(r_result);
}

void MCInterfaceEvalShiftKey(MCExecContext& ctxt, MCNameRef& r_result)
{
	r_result = MCInterfaceKeyConditionToName((MCscreen->querymods() & MS_SHIFT) != 0);
	MCValueRetain(r_result);
}

void MCInterfaceEvalKeysDown(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCscreen->getkeysdown(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalMouse(MCExecContext& ctxt, integer_t p_which, MCNameRef& r_result)
{
	Boolean t_abort;
	r_result = MCInterfaceKeyConditionToName(MCscreen->getmouse(p_which, t_abort) != 0);
	MCValueRetain(r_result);
	if (t_abort)
		ctxt.LegacyThrow(EE_WAIT_ABORT);
}

//////////

void MCInterfaceEvalMouseClick(MCExecContext& ctxt, bool& r_bool)
{
	Boolean t_abort;
	r_bool = MCscreen->getmouseclick(0, t_abort) == True;

	if (t_abort)
		ctxt.LegacyThrow(EE_WAIT_ABORT);
}

//////////

void MCInterfaceEvalMouseColor(MCExecContext& ctxt, MCColor& r_color)
{
	int16_t t_x, t_y;
	MCscreen->querymouse(t_x, t_y);
	MCscreen->dropper(nil, t_x, t_y, &r_color);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalFocusedObject(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCfocusedstackptr == nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCControl *t_cptr;
	t_cptr = MCfocusedstackptr->getcard()->getkfocused();
	if (t_cptr != nil)
		t_cptr->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
	else
		MCfocusedstackptr->getcard()->getstringprop(ctxt, 0, P_LONG_ID, False, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalColorNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCscreen->getcolornames(r_string))
		return;
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalFlushEvents(MCExecContext& ctxt, MCNameRef p_name, MCStringRef& r_string)
{
	static MCNameRef enames[FE_LAST] =
		{ MCN_all, MCM_mouse_down, MCM_mouse_up,
	      MCM_key_down, MCM_key_up, MCN_auto_key,
	      MCN_disk, MCN_activate, MCN_high_level,
	      MCN_system
	    };
	for (integer_t i = 0; i < FE_LAST; i++)
	{
		if (MCNameIsEqualTo(p_name, enames[i]))
		{
			MCscreen->flushevents(i);
			break;
		}
	}
	r_string = MCValueRetain(kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalGlobalLoc(MCExecContext& ctxt, MCPoint p_point, MCPoint& r_global_point)
{
	MCRectangle t_rect;
	t_rect = MCdefaultstackptr->getrect();
	
	r_global_point . x = p_point . x + t_rect . x;
	r_global_point . y = p_point . y + t_rect . y - MCdefaultstackptr -> getscroll();

/*	int16_t t_x, t_y;
	if (!MCU_stoi2x2(p_point, t_x, t_y))
	{
		ctxt . LegacyThrow(EE_GLOBALLOC_NAP, p_point);
		return;
	}

	if (MCStringFormat(r_string, "%d,%d", t_x + t_rect.x, t_y + t_rect.y - MCdefaultstackptr->getscroll()))
		return;

	ctxt.Throw();*/
}

void MCInterfaceEvalLocalLoc(MCExecContext& ctxt, MCPoint p_point, MCPoint& r_local_point)
{
	MCRectangle t_rect;
	t_rect = MCdefaultstackptr->getrect();
	
	r_local_point . x = p_point . x - t_rect . x;
	r_local_point . y = p_point . y - t_rect . y + MCdefaultstackptr -> getscroll();

/*	int16_t t_x, t_y;
	if (!MCU_stoi2x2(p_point, t_x, t_y))
	{
		ctxt . LegacyThrow(EE_LOCALLOC_NAP, p_point);
		return;
	}

	MCRectangle t_rect = MCdefaultstackptr->getrect();
	if (MCStringFormat(r_string, "%d,%d", t_x - t_rect.x, t_y - t_rect.y + MCdefaultstackptr->getscroll()))
		return;

	ctxt.Throw();*/
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalMainStacks(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCdispatcher->getmainstacknames(&t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

//////////

void MCInterfaceEvalOpenStacks(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCstacks->stackprops(ctxt, P_SHORT_NAME, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	if (!ctxt.HasError())
		ctxt.Throw();
}

//////////

void MCInterfaceEvalStacks(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCstacks->stackprops(ctxt, P_FILE_NAME, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	if (!ctxt.HasError())
		ctxt.Throw();
}

void MCInterfaceTopStack(MCExecContext& ctxt, MCStack *p_stack, MCStringRef& r_string)
{
	if (p_stack == nil)
		r_string = MCValueRetain(kMCEmptyString);
	else
		p_stack->getstringprop(ctxt, 0, P_LONG_NAME, False, r_string);
}

void MCInterfaceEvalTopStack(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCInterfaceTopStack(ctxt, MCtopstackptr, r_string);
}

void MCInterfaceEvalTopStackOf(MCExecContext& ctxt, integer_t p_stack_number, MCStringRef& r_string)
{
	MCInterfaceTopStack(ctxt, MCstacks->getstack(p_stack_number), r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalMovingControls(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCAutoListRef t_list;
	if (MCscreen->listmoves(ctxt, &t_list) && MCListCopyAsString(*t_list, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalWaitDepth(MCExecContext& ctxt, integer_t& r_depth)
{
	r_depth = MCwaitdepth;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCInterfaceIntersect(MCObject *p_object_a, MCObject *p_object_b, uinteger_t p_threshold)
{
	return p_object_a->intersects(p_object_b, p_threshold);
}

void MCInterfaceEvalIntersectWithThreshold(MCExecContext& ctxt, MCObjectPtr p_object_a, MCObjectPtr p_object_b, MCStringRef p_threshold, bool& r_intersect)
{
	uinteger_t t_threshold = 0;
	if (!ctxt.ConvertToUnsignedInteger(p_threshold, t_threshold))
	{
		if (MCStringIsEqualTo(p_threshold, MCNameGetString(MCN_bounds), kMCStringOptionCompareCaseless))
			t_threshold = 0;
		else if (MCStringIsEqualTo(p_threshold, MCNameGetString(MCN_pixels), kMCStringOptionCompareCaseless))
			t_threshold = 1;
		else if (MCStringIsEqualTo(p_threshold, MCNameGetString(MCN_opaque_pixels), kMCStringOptionCompareCaseless))
			t_threshold = 255;
		else
		{
			ctxt.LegacyThrow(EE_INTERSECT_ILLEGALTHRESHOLD, p_threshold);
			return;
		}
	}

	r_intersect = MCInterfaceIntersect(p_object_a . object, p_object_b . object, t_threshold);
}

void MCInterfaceEvalIntersect(MCExecContext& ctxt, MCObjectPtr p_object_a, MCObjectPtr p_object_b, bool& r_intersect)
{
	r_intersect = MCInterfaceIntersect(p_object_a . object, p_object_b . object, 0);
}

void MCInterfaceEvalWithin(MCExecContext& ctxt, MCObjectPtr p_object, MCPoint p_point, bool& r_within)
{
	if (p_object . object -> gettype() < CT_GROUP)
		r_within = MCU_point_in_rect(p_object . object -> getrect(), p_point . x, p_point . y) == True;
	else
	{
		MCControl *t_control = (MCControl*)p_object . object;
		MCRectangle t_rect;
		MCU_set_rect(t_rect, p_point . x, p_point . y, 1, 1);
		r_within = t_control->maskrect(t_rect) == True;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalThereIsAnObject(MCExecContext& ctxt, MCChunk *p_object, bool& r_exists)
{
	MCObject *optr;
	uint4 parid;
	MCerrorlock++;
	r_exists = p_object->getobj(ctxt . GetEP(), optr, parid, True) == ES_NORMAL;
	MCerrorlock--;
}

void MCInterfaceEvalThereIsNotAnObject(MCExecContext& ctxt, MCChunk *p_object, bool& r_not_exists)
{
	bool t_exists;
	MCInterfaceEvalThereIsAnObject(ctxt, p_object, t_exists);
	r_not_exists = !t_exists;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecBeep(MCExecContext& ctxt, integer_t p_count)
{
	while (p_count--)
	{
		MCscreen->beep();
		
		// MW-2010-01-08: [[ Bug 1690 ]] We need a break on all beeps but the last
		if (p_count >= 1)
		{
			// MW-2008-03-17: [[ Bug 6098 ]] Make sure we check for an abort from wait
			if (MCscreen->wait(BEEP_INTERVAL, False, False))
			{
				ctxt . LegacyThrow(EE_WAIT_ABORT);
				return;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecFocusOnNothing(MCExecContext &ctxt)
{
	if (MCfocusedstackptr != NULL && MCfocusedstackptr -> getcard() != NULL)
		MCfocusedstackptr -> getcard() -> kunfocus();
#ifdef _MOBILE
	// Make sure the IME is forced closed if explicitly asked to be.
	MCscreen -> closeIME();
#endif
}

void MCInterfaceExecFocusOn(MCExecContext &ctxt, MCObject *p_object)
{
	if (!p_object->getflag(F_TRAVERSAL_ON))
	{
		ctxt . LegacyThrow(EE_FOCUS_BADOBJECT);
		return;
	}
	p_object->getstack()->kfocusset((MCControl *)p_object);
}


////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecGrab(MCExecContext &ctxt, MCControl *p_control)
{
	p_control->grab();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecResetCursors(MCExecContext& ctxt)
{
	MCModeResetCursors();
}

void MCInterfaceExecResetTemplate(MCExecContext& ctxt, Reset_type p_type)
{
	switch(p_type)
	{
		case RT_TEMPLATE_AUDIO_CLIP:
			delete MCtemplateaudio;
			MCtemplateaudio = new MCAudioClip;
			break;
		case RT_TEMPLATE_BUTTON:
			delete MCtemplatebutton;
			MCtemplatebutton = new MCButton;
			break;
		case RT_TEMPLATE_CARD:
			delete MCtemplatecard;
			MCtemplatecard = new MCCard;
			break;
		case RT_TEMPLATE_EPS:
			delete MCtemplateeps;
			MCtemplateeps = new MCEPS;
			break;
		case RT_TEMPLATE_FIELD:
			delete MCtemplatefield;
			MCtemplatefield = new MCField;
			break;
		case RT_TEMPLATE_GRAPHIC:
			delete MCtemplategraphic;
			MCtemplategraphic = new MCGraphic;
			break;
		case RT_TEMPLATE_GROUP:
			delete MCtemplategroup;
			MCtemplategroup = new MCGroup;
			break;
		case RT_TEMPLATE_IMAGE:
			delete MCtemplateimage;
			MCtemplateimage = new MCImage;
			break;
		case RT_TEMPLATE_SCROLLBAR:
			delete MCtemplatescrollbar;
			MCtemplatescrollbar = new MCScrollbar;
			break;
		case RT_TEMPLATE_PLAYER:
			delete MCtemplateplayer;
			MCtemplateplayer = new MCPlayer;
			break;
		case RT_TEMPLATE_STACK:
			delete MCtemplatestack;
			/* UNCHECKED */ MCStackSecurityCreateStack(MCtemplatestack);
			break;
		case RT_TEMPLATE_VIDEO_CLIP:
			delete MCtemplatevideo;
			MCtemplatevideo = new MCVideoClip;
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

static MCObject *MCInterfaceEvalControlAtLocInStack(MCStack *p_stack, MCPoint p_location)
{
	return p_stack -> getcard() -> hittest(p_location . x, p_location . y);
}

void MCInterfaceEvalControlAtLoc(MCExecContext& ctxt, MCPoint p_location, MCStringRef& r_control)
{
	MCObject *t_object;
	t_object = MCInterfaceEvalControlAtLocInStack(MCdefaultstackptr, p_location);
	if (t_object -> gettype() != CT_CARD)
	{
		uinteger_t t_layer;
		t_object -> getuintprop(ctxt, 0, P_LAYER, False, t_layer);
		if (ctxt . HasError())
			return;

		if (MCStringFormat(r_control, "control %d", t_layer))
			return;
	}
	else
	{
		r_control = MCValueRetain(kMCEmptyString);
		return;
	}

	ctxt . Throw();
}

void MCInterfaceEvalControlAtScreenLoc(MCExecContext& ctxt, MCPoint p_location, MCStringRef& r_control)
{
	MCStack *t_stack;
	t_stack = MCscreen -> getstackatpoint(p_location . x, p_location . y);
	if (t_stack != nil)
	{
		MCRectangle t_rect;
		t_rect = t_stack -> getrect();
		p_location . x -= t_rect . x;
		p_location . y -= t_rect . y - t_stack -> getscroll();
	}

	if (t_stack == nil)
	{
		r_control = MCValueRetain(kMCEmptyString);
		return;
	}

	MCObject *t_object;
	t_object = MCInterfaceEvalControlAtLocInStack(MCdefaultstackptr, p_location);

	if (t_object -> names(P_LONG_ID, r_control))
		return;

	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecDrag(MCExecContext& ctxt, uint2 p_which, MCPoint p_start, MCPoint p_end, uint2 p_modifiers)
{
	uint2 oldmstate = MCmodifierstate;
	uint2 oldbstate = MCbuttonstate;
	int2 oldx = MCmousex;
	int2 oldy = MCmousey;
	MCmodifierstate = p_modifiers;
	MCbuttonstate = 0x1 << (p_which - 1);
	MCmousex = p_start . x;
	MCmousey = p_start . y;
	//MCdragging = True;
	MCscreen->setlockmods(True);
	MCdefaultstackptr->mfocus(p_start . x, p_start . y);
	MCdefaultstackptr->mdown(p_which);
	if (MCdragspeed == 0)
	{
		MCmousex = p_end . x;
		MCmousey = p_end . y;
		MCdefaultstackptr->mfocus(p_end . x, p_end . y);
		MCdefaultstackptr->mup(p_which);
		MCscreen->setlockmods(False);
		MCmodifierstate = oldmstate;
		MCbuttonstate = oldbstate;
		MCmousex = oldx;
		MCmousey = oldy;
		return;
	}
	MCscreen->sync(MCdefaultstackptr->getw());
	real8 dx = MCU_abs(p_end . x - p_start . x);
	real8 dy = MCU_abs(p_end . y - p_start . y);
	real8 ix = 0.0;
	if (dx != 0.0)
		ix = dx / (p_end . x - p_start . x);
	real8 iy = 0.0;
	if (dy != 0.0)
		iy = dy / (p_end . y - p_start . y);
	real8 starttime = MCS_time();
	real8 curtime = starttime;
	real8 duration = 0.0;
	if (MCdragspeed != 0)
		duration = sqrt((double)(dx * dx + dy * dy)) / (real8)MCdragspeed;
	real8 endtime = starttime + duration;
	Boolean abort = False;
	MCdragging = True;
	int2 x = p_start . x;
	int2 y = p_start . y;
	while (x != p_end . x || y != p_end . y)
	{
		int2 oldx = x;
		int2 oldy = y;
		x = p_start . x + (int2)(ix * (dx * (curtime - starttime) / duration));
		y = p_start . y + (int2)(iy * (dy * (curtime - starttime) / duration));
		if (MCscreen->wait((real8)MCsyncrate / 1000.0, False, True))
		{
			abort = True;
			break;
		}
		curtime = MCS_time();
		if (curtime >= endtime)
		{
			x = p_end . x;
			y = p_end . y;
			curtime = endtime;
		}
		if (x != oldx || y != oldy)
			MCdefaultstackptr->mfocus(x, y);
	}
	MCdefaultstackptr->mup(p_which);
	MCmodifierstate = oldmstate;
	MCbuttonstate = oldbstate;
	MCmousex = oldx;
	MCmousey = oldy;
	MCscreen->setlockmods(False);
	MCdragging = False;
	if (abort)
	{
		ctxt . LegacyThrow(EE_DRAG_ABORT);
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecType(MCExecContext& ctxt, MCStringRef p_typing, uint2 p_modifiers)
{
	uint2 oldstate = MCmodifierstate;
	MCmodifierstate = p_modifiers;
	MCdefaultstackptr->kfocus();
	uint2 i;
	char string[2];
	string[1] = '\0';
	real8 nexttime = MCS_time();

	for (i = 0 ; i < MCStringGetLength(p_typing); i++)
	{
		KeySym keysym = (unsigned char) MCStringGetNativeCharAtIndex(p_typing, i);
		if (keysym < 0x20 || keysym == 0xFF)
		{
			if (keysym == 0x0A)
				keysym = 0x0D;
			keysym |= 0xFF00;
			string[0] = '\0';
		}
		else
			string[0] = MCStringGetNativeCharAtIndex(p_typing, i);
		MCdefaultstackptr->kdown(string, keysym);
		MCdefaultstackptr->kup(string, keysym);
		nexttime += (real8)MCtyperate / 1000.0;
		real8 delay = nexttime - MCS_time();
		if (MCscreen->wait(delay, False, False))
			ctxt . LegacyThrow(EE_TYPE_ABORT);
	}

	MCmodifierstate = oldstate;
	return;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPopToLast(MCExecContext& ctxt)
{
	MCCard *cptr = MCcstack->popcard();
	MCStack *sptr = cptr->getstack();
	MCdefaultstackptr = sptr;
	Boolean oldtrace = MCtrace;
	MCtrace = False;
	if (sptr->setcard(cptr, True, False) == ES_NORMAL
		        && sptr->openrect(sptr->getrect(), WM_LAST, NULL, WP_DEFAULT, OP_NONE) == ES_NORMAL)
	{
		MCtrace = oldtrace;
		return;
	}
	MCtrace = oldtrace;
	ctxt . Throw();
}

void MCInterfaceExecPop(MCExecContext& ctxt, MCStringRef& r_element)
{
	MCCard *cptr = MCcstack->popcard();
	if (cptr -> names(P_LONG_ID, r_element))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPushRecentCard(MCExecContext& ctxt)
{
	MCcstack->pushcard(MCrecent->getrel(-1));
}

void MCInterfaceExecPushCurrentCard(MCExecContext& ctxt)
{
	MCcstack->pushcard(MCdefaultstackptr->getcurcard());
}

void MCInterfaceExecPushCard(MCExecContext& ctxt, MCCard *p_target)
{	
	MCcstack->pushcard(p_target);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecClickCmd(MCExecContext& ctxt, uint2 p_button, MCPoint p_location, uint2 p_modifiers)
{
	if (!MCdefaultstackptr->getopened()
	        || !MCdefaultstackptr->mode_haswindow())
	{
		ctxt . LegacyThrow(EE_CLICK_STACKNOTOPEN);
		return;
	}
	uint2 oldmstate = MCmodifierstate;
	uint2 oldbstate = MCbuttonstate;
	int2 oldmx = MCmousex;
	int2 oldmy = MCmousey;
	MCmousex = p_location . x;
	MCmousey = p_location . y;
	MCStack *oldms = MCmousestackptr;
	MCmodifierstate = p_modifiers;
	MCbuttonstate |= 0x1L << (p_button - 1);
	MCmousestackptr = MCdefaultstackptr;
	MCdispatcher->wmfocus_stack(MCdefaultstackptr, p_location . x, p_location . y);
	MCmodifierstate = p_modifiers;
	MCbuttonstate |= 0x1L << (p_button - 1);
	MCdispatcher->wmdown_stack(MCdefaultstackptr, p_button);
	// **** NULL POINTER FIX
	if (MCmousestackptr != NULL)
		MCscreen->sync(MCmousestackptr->getw());
	Boolean abort = MCscreen->wait(CLICK_INTERVAL, False, False);
	MCclicklocx = p_location . x;
	MCclicklocy = p_location . y;
	MCmodifierstate = p_modifiers;
	MCbuttonstate &= ~(0x1L << (p_button - 1));
	MCdispatcher->wmup_stack(MCdefaultstackptr, p_button);
	MCmodifierstate = oldmstate;
	MCbuttonstate = oldbstate;
	MCControl *mfocused = MCdefaultstackptr->getcard()->getmfocused();
	if (mfocused != NULL
	        && (mfocused->gettype() == CT_GRAPHIC
	            && mfocused->getstate(CS_CREATE_POINTS)
	            || (mfocused->gettype() == CT_IMAGE && mfocused->getstate(CS_DRAW)
	                && MCdefaultstackptr->gettool(mfocused) == T_POLYGON)))
		mfocused->doubleup(1); // cancel polygon create
	if (oldms == NULL || oldms->getmode() != 0)
	{
		MCmousestackptr = oldms;
		MCmousex = oldmx;
		MCmousey = oldmy;
		if (oldms != NULL)
			MCdispatcher->wmfocus_stack(oldms, oldmx, oldmy);
	}
	if (abort)
	{
		ctxt . LegacyThrow(EE_CLICK_ABORT);
		return;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecRemoveGroupFromCard(MCExecContext& ctxt, MCObjectPtr p_group, MCCard *p_target)
{
				
	// MW-2011-08-09: [[ Groups ]] If the group's parent is a group then we
	//   can't unplace it.

	if (p_group . object -> getparent() -> gettype() == CT_GROUP)
	{
		ctxt . LegacyThrow(EE_GROUP_CANNOTBEBGORSHARED);
		return;
	}

	p_target->removecontrol((MCControl *)p_group . object, True, True);

	// MW-2011-08-09: [[ Groups ]] Removing a group from a card implicitly
	//   makes it shared (rather than a background).
	p_group . object -> setflag(True, F_GROUP_SHARED);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPlaceGroupOnCard(MCExecContext& ctxt, MCObject *p_group, MCCard *p_target)
{
	if (p_group -> getparent() -> gettype() != CT_CARD && p_group -> getparent() -> gettype() != CT_STACK)
	{
		ctxt . LegacyThrow(EE_PLACE_NOTABACKGROUND);
		return;
	}
	if (p_target->getparent() != p_group->getstack() || p_target->countme(p_group->getid(), False))
	{
		ctxt . LegacyThrow(EE_PLACE_ALREADY);
		return;
	}

	// MW-2011-08-09: [[ Groups ]] If the group is not already marked as shared
	//   then turn on backgroundBehavior (legacy requirement).
	if (!static_cast<MCGroup *>(p_group) -> isshared())
		p_group->setflag(False, F_GROUP_ONLY);
	
	// MW-2011-08-09: [[ Groups ]] Make sure the group is marked as shared.
	p_group->setflag(True, F_GROUP_SHARED);

	p_target->newcontrol((MCControl *)p_group, True);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecUngroupObject(MCExecContext& ctxt, MCObject *p_group)
{
	p_group->getstack()->ungroup((MCGroup *)p_group);
}

void MCInterfaceExecUngroupSelection(MCExecContext& ctxt)
{
	if (MCtopstackptr != NULL)
	{
		MCObject *t_group;
		t_group = MCselected->getfirst();
		if (t_group == NULL || t_group->gettype() != CT_GROUP)
		{
			ctxt . LegacyThrow(EE_UNGROUP_NOTAGROUP);
			return;
		}
		MCInterfaceExecUngroupObject(ctxt, t_group);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecUndo(MCExecContext& ctxt)
{
	MCundos->undo();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecRevert(MCExecContext& ctxt)
{
	Window_mode oldmode = MCtopstackptr->getmode();
	MCRectangle oldrect = MCtopstackptr->getrect();
	MCStack *t_sptr = MCtopstackptr;
	if (!MCdispatcher->ismainstack(t_sptr))
		t_sptr = (MCStack *)t_sptr->getparent();
	if (t_sptr == MCdispatcher->gethome())
	{
		ctxt . LegacyThrow(EE_REVERT_HOME);
		return;
	}
	MCAutoStringRef t_filename;
	t_sptr->getstringprop(ctxt, 0, P_FILE_NAME, False, &t_filename);
	Boolean oldlock = MClockmessages;
	MClockmessages = True;
	MCerrorlock++;
	t_sptr->del();
	MCerrorlock--;
	MClockmessages = oldlock;
	MCtodestroy->add
	(t_sptr);
	MCNewAutoNameRef t_name;
	/* UNCHECKED */ MCNameCreate(*t_filename, &t_name);
	t_sptr = MCdispatcher->findstackname(*t_name);
	if (t_sptr != NULL)
		t_sptr->openrect(oldrect, oldmode, NULL, WP_DEFAULT, OP_NONE);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecGroupControls(MCExecContext& ctxt, MCObjectPtr *p_controls, uindex_t p_control_count)
{
	if (p_control_count != 0)
	{
		MCCard *t_card = NULL;
		MCControl *controls = NULL;
		for (uindex_t i = 0; i < p_control_count; ++i)
		{
			if ((p_controls[i] . object) -> getparent() -> gettype() != CT_CARD)
			{
				ctxt . LegacyThrow(EE_GROUP_NOOBJ);
				return;
			}
			MCControl *cptr = (MCControl *)p_controls[i] . object;
			// MW-2011-01-21: Make sure we don't try and group shared groups
			if (cptr -> gettype() == CT_GROUP && static_cast<MCGroup *>(cptr) -> isshared())
			{
				ctxt . LegacyThrow(EE_GROUP_NOBG);
				return;
			}
			t_card = cptr->getcard(p_controls[i] . part_id);
			t_card -> removecontrol(cptr, False, True);
			cptr -> getstack() -> removecontrol(cptr);
			cptr -> appendto(controls);
		}
		MCGroup *gptr;
		if (MCsavegroupptr == NULL)
			gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		else
			gptr = (MCGroup *)MCsavegroupptr->remove(MCsavegroupptr);
		gptr->makegroup(controls, t_card); 
	}
}

void MCInterfaceExecGroupSelection(MCExecContext& ctxt)
{
	MCselected->group();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceProcessToContainer(MCExecContext& ctxt, MCObjectPtr *p_objects, uindex_t p_object_count, MCObjectPtr p_dst, bool p_cut)
{
	// Check destination compatibility
	Chunk_term t_dst_type;
	t_dst_type = p_dst . object -> gettype();
	for(uindex_t i = 0; i < p_object_count; ++i)
	{
		Chunk_term t_src_type;
		t_src_type = p_objects[i] . object -> gettype();

		if ((t_src_type <= CT_CARD && t_dst_type != CT_STACK) ||
			(t_dst_type != CT_STACK && t_dst_type != CT_CARD && t_dst_type != CT_GROUP))
		{
			ctxt . LegacyThrow(EE_CLIPBOARD_BADDEST);
			return;
		}
	}
	MCObject *t_new_object = NULL;
	for(uindex_t i = 0; i < p_object_count; ++i)
	{
		MCObject *t_object;
		t_object = p_objects[i] . object;
		if (!t_object -> getstack() -> iskeyed())
		{
			ctxt . SetTheResultToStaticCString("can't cut object (stack is password protected)");
			continue;
		}
		uindex_t t_part;
		t_part = p_objects[i] . part_id;

		switch(t_object -> gettype())
		{
		case CT_AUDIO_CLIP:
		{
			MCAudioClip *t_aclip;
			if (p_cut)
			{
				t_aclip = static_cast<MCAudioClip *>(t_object);
				t_object -> getstack() -> removeaclip(t_aclip);
			}
			else
				t_aclip = new MCAudioClip(*static_cast<MCAudioClip *>(t_object));

			t_new_object = t_aclip;
			p_dst . object -> getstack() -> appendaclip(t_aclip);
		}
		break;

		case CT_VIDEO_CLIP:
		{
			MCVideoClip *t_aclip;
			if (p_cut)
			{
				t_aclip = static_cast<MCVideoClip *>(t_object);
				t_object -> getstack() -> removevclip(t_aclip);
			}
			else
				t_aclip = new MCVideoClip(*static_cast<MCVideoClip *>(t_object));

			t_new_object = t_aclip;
			p_dst . object -> getstack() -> appendvclip(t_aclip);
		}
		break;

		case CT_CARD:
		{
			if (!p_cut)
			{
				MCStack *t_old_default;
				t_old_default = MCdefaultstackptr;
				MCdefaultstackptr = static_cast<MCStack *>(p_dst . object);
				MCdefaultstackptr -> stopedit();

				MCCard *t_card;
				t_card = static_cast<MCCard *>(t_object);

				t_new_object = t_card -> clone(True, True);

				MCdefaultstackptr = t_old_default;
			}
		}
		break;

		case CT_GROUP:
		case CT_BUTTON:
		case CT_SCROLLBAR:
		case CT_PLAYER:
		case CT_IMAGE:
		case CT_GRAPHIC:
		case CT_EPS:
		case CT_COLOR_PALETTE:
		case CT_FIELD:
		{
			if (p_dst . object -> gettype() == CT_STACK)
				p_dst . object = static_cast<MCStack *>(p_dst . object) -> getcurcard();

			if (!p_cut)
			{
				MCObject *t_old_parent;
				t_old_parent = t_object -> getparent();
				t_object -> setparent(p_dst . object);

				MCControl *t_control;
				t_control = static_cast<MCControl *>(t_object);

				// MW-2011-08-18: [[ Redraw ]] Move to use redraw lock/unlock.
				MCRedrawLockScreen();
				
				t_new_object = t_control -> clone(True, OP_NONE, false);

				MCControl *t_new_control;
				t_new_control = static_cast<MCControl *>(t_new_object);
				if (p_dst . object -> getstack() != t_old_parent -> getstack())
					t_new_control -> resetfontindex(t_old_parent -> getstack());

				// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
				t_new_control -> layer_redrawall();

				MCRedrawUnlockScreen();

				t_object -> setparent(t_old_parent);
			}
		}
		break;

		default:
		{
			ctxt . LegacyThrow(EE_CLIPBOARD_BADOBJ);
			return;
		}
		break;
		}
	}

	if (t_new_object != NULL)
	{
		MCAutoStringRef t_id;
		if (t_new_object -> names(P_LONG_ID, &t_id))
			ctxt . SetItToValue(*t_id);
	}
}
void MCInterfaceExecCopyObjectsToContainer(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count, MCObjectPtr p_destination)
{
	MCInterfaceProcessToContainer(ctxt, p_targets, p_target_count, p_destination, false);
}

void MCInterfaceExecCutObjectsToContainer(MCExecContext& ctxt, MCObjectPtr *p_targets, uindex_t p_target_count, MCObjectPtr p_destination)
{
	MCInterfaceProcessToContainer(ctxt, p_targets, p_target_count, p_destination, true);
	//Cut to container not currently implemented for certain object types
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecDelete(MCExecContext& ctxt)
{
	if (MCactivefield != NULL)
		MCactivefield->deleteselection(False);
	else if (MCactiveimage != NULL)
		MCactiveimage->delimage();	
	else
		MCselected->del();
}

void MCInterfaceExecDeleteObjects(MCExecContext& ctxt, MCObjectPtr *p_objects, uindex_t p_object_count)
{
	for(uindex_t i = 0; i < p_object_count; i++)
	{
		if (!p_objects[i] . object -> del())
		{
			ctxt . LegacyThrow(EE_CHUNK_CANTDELETEOBJECT);
			return;
		}

		if (p_objects[i] . object -> gettype() == CT_STACK)
			MCtodestroy -> remove((MCStack *)p_objects[i] . object);
		p_objects[i] . object -> scheduledelete();
	}
}

void MCInterfaceExecDeleteObjectChunks(MCExecContext& ctxt, MCObjectChunkPtr *p_chunks, uindex_t p_chunk_count)
{
	for(uindex_t i = 0; i < p_chunk_count; i++)
	{
		if (p_chunks[i] . object -> gettype() == CT_BUTTON)
		{
			MCStringRef t_value; 
			t_value = nil;
			p_chunks[i] . object -> getstringprop(ctxt, p_chunks[i] . part_id, P_TEXT, False, t_value);

			/* UNCHECKED */ MCStringMutableCopyAndRelease(t_value, t_value);
			/* UNCHECKED */ MCStringRemove(t_value, MCRangeMake(p_chunks[i] . mark . start, p_chunks[i] . mark . finish - p_chunks[i] . mark . start));
			/* UNCHECKED */ MCStringCopyAndRelease(t_value, t_value);
			p_chunks[i] . object -> setstringprop(ctxt, p_chunks[i] . part_id, P_TEXT, False, t_value);
			MCValueRelease(t_value);
		}
		else if (p_chunks[i] . object -> gettype() == CT_FIELD)
        {
            MCField *t_field;
            t_field = static_cast<MCField *>(p_chunks[i] . object);
            integer_t t_si, t_ei;
            t_si = 0;
            t_ei = INT32_MAX;
            t_field -> resolvechars(p_chunks[i] . part_id, t_si, t_ei, p_chunks[i] . mark . start, p_chunks[i] . mark . finish - p_chunks[i] . mark . start);
            
			t_field -> settextindex_stringref(p_chunks[i] . part_id, t_si, t_ei, kMCEmptyString, False);
        }
	}
}

////////////////////////////////////////////////////////////////////////////////

static void MCInterfaceExecChangeChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target, Properties p_prop, bool p_value)
{
	MCStringRef t_value;
	p_target . object -> getstringprop(ctxt, p_target . part_id, P_TEXT, False, t_value);

	/* UNCHECKED */ MCStringMutableCopyAndRelease(t_value, t_value);

	int4 start, end;
	start = p_target . mark . start;
	end = p_target . mark . finish;

	bool t_changed;
	t_changed = false;
	if (p_prop == P_DISABLED)
		if (p_value)
		{
			if (MCStringGetNativeCharAtIndex(t_value, start) != '(')
		        /* UNCHECKED */ MCStringInsert(t_value, start, MCSTR("(")), t_changed = true;
		}
		else
		{
			 if (MCStringGetNativeCharAtIndex(t_value, start) == '(')
		        /* UNCHECKED */ MCStringRemove(t_value, MCRangeMake(start, 1)), t_changed = true;
		}
	else
	{
		if (MCStringGetNativeCharAtIndex(t_value, start) == '(')
			start++;
		if (p_value)
		{
			if (MCStringGetNativeCharAtIndex(t_value, start + 1) == 'n')
		        /* UNCHECKED */ MCStringReplace(t_value, MCRangeMake(start + 1, 1), MCSTR("c")), t_changed = true;
			else
				if (MCStringGetNativeCharAtIndex(t_value, start + 1) == 'u')
					/* UNCHECKED */ MCStringReplace(t_value, MCRangeMake(start + 1, 1), MCSTR("r")), t_changed = true;
		}
		else
		{
			if (MCStringGetNativeCharAtIndex(t_value, start + 1) == 'c')
		        /* UNCHECKED */ MCStringReplace(t_value, MCRangeMake(start + 1, 1), MCSTR("n")), t_changed = true;
			else
				if (MCStringGetNativeCharAtIndex(t_value, start + 1) == 'r')
					/* UNCHECKED */ MCStringReplace(t_value, MCRangeMake(start + 1, 1), MCSTR("u")), t_changed = true;
		}
	}

	if (t_changed)
	{
		p_target . object->setstringprop(ctxt, p_target . part_id, P_TEXT, False, t_value);    
	}
	MCValueRelease(t_value);
}

void MCInterfaceExecEnableObject(MCExecContext& ctxt, MCObjectPtr p_target)
{
	p_target . object -> setboolprop(ctxt, 0, P_DISABLED, False, false);
}

void MCInterfaceExecDisableObject(MCExecContext& ctxt, MCObjectPtr p_target)
{
	p_target . object -> setboolprop(ctxt, 0, P_DISABLED, False, true);
}

void MCInterfaceExecHiliteObject(MCExecContext& ctxt, MCObjectPtr p_target)
{
	p_target . object -> setboolprop(ctxt, 0, P_HILITE, False, true);
}

void MCInterfaceExecUnhiliteObject(MCExecContext& ctxt, MCObjectPtr p_target)
{
	p_target . object -> setboolprop(ctxt, 0, P_HILITE, False, false);
}

void MCInterfaceExecEnableChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{
	MCInterfaceExecChangeChunkOfButton(ctxt, p_target, P_DISABLED, false); 
}

void MCInterfaceExecDisableChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{
	MCInterfaceExecChangeChunkOfButton(ctxt, p_target, P_DISABLED, true);
}

void MCInterfaceExecHiliteChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{
	MCInterfaceExecChangeChunkOfButton(ctxt, p_target, P_HILITE, true);
}

void MCInterfaceExecUnhiliteChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target)
{ 
	MCInterfaceExecChangeChunkOfButton(ctxt, p_target, P_HILITE, false);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecSelectEmpty(MCExecContext& ctxt)
{
	MCselected->clear(True);
	if (MCactivefield != NULL)
	{
		MCactivefield->unselect(False, True);
		if (MCactivefield != NULL)
			MCactivefield->getcard()->kunfocus();
	}
}

//////////

void MCInterfaceExecSelectAllTextOfField(MCExecContext& ctxt, MCObjectPtr p_target)
{
	if (!p_target . object -> getopened() && p_target . object -> getid())
	{
		ctxt . LegacyThrow(EE_CHUNK_NOTOPEN);
		return;
	}

	static_cast<MCField *>(p_target . object) -> seltext(0, static_cast<MCField *>(p_target . object) -> getpgsize(nil), True);
}

void MCInterfaceExecSelectTextOfField(MCExecContext& ctxt, Preposition_type p_type, MCObjectChunkPtr p_target)
{
	if (!p_target . object -> getopened() && p_target . object -> getid())
	{
		ctxt . LegacyThrow(EE_CHUNK_NOTOPEN);
		return;
	}

	uindex_t t_start, t_finish;
	t_start = p_target . mark . start;
	t_finish = p_target . mark . finish;
	switch(p_type)
	{
	case PT_AT:
		break;
	case PT_BEFORE:
		t_finish = t_start;
		break;
	case PT_AFTER:
		t_start = t_finish;
		break;
	}
    
    MCField *t_field;
    t_field = static_cast<MCField *>(p_target . object);
    integer_t t_si, t_ei;
    t_si = 0;
    t_ei = INT32_MAX;
    t_field -> resolvechars(p_target . part_id, t_si, t_ei, t_start, t_finish - t_start);
    
	static_cast<MCField *>(p_target . object) -> seltext(t_si, t_ei, True);
}

//////////

void MCInterfaceExecSelectTextOfButton(MCExecContext& ctxt, Preposition_type p_type, MCObjectChunkPtr p_target)
{
	// Handle the option menu case

	if (!p_target . object -> getopened() && p_target . object -> getid())
	{
		ctxt . LegacyThrow(EE_CHUNK_NOTOPEN);
		return;
	}

	bool t_success;
	t_success = true;
	
	MCAutoStringRef t_text;
	if (t_success)
	{
		p_target . object -> getstringprop(ctxt, p_target . part_id, P_TEXT, False, &t_text);
		t_success = !ctxt . HasError();
	}
		
	if (t_success)
	{
		uindex_t t_lines;
		t_lines = MCStringCountChar(*t_text, MCRangeMake(0, p_target . mark . start), '\n', kMCStringOptionCompareCaseless);
		
		static_cast<MCButton *>(p_target . object) -> setmenuhistory(t_lines + 1);
	}
	
	if (t_success)
		return;
		
	ctxt . Throw();
}

//////////

static void MCInterfaceExecSelectObject(MCExecContext& ctxt, MCObjectPtr p_object, bool p_first)
{
	if (!p_object . object -> getopened() && p_object . object -> getid())
	{
		ctxt . LegacyThrow(EE_CHUNK_NOTOPEN);
		return;
	}

	if (p_first)
		MCselected -> clear(False);

	MCselected -> add(p_object . object);
}

void MCInterfaceExecSelectObjects(MCExecContext& ctxt, MCObjectPtr *p_objects, uindex_t p_object_count)
{
	for(uindex_t i = 0; i < p_object_count; i++)
		MCInterfaceExecSelectObject(ctxt, p_objects[i], i == 0);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecStartEditingGroup(MCExecContext& ctxt, MCGroup *p_target)
{
	if (p_target->getstack()->islocked())
	{
		ctxt . LegacyThrow(EE_START_LOCKED);
		return;
	}
	p_target->getstack()->startedit(p_target);
}

void MCInterfaceExecStopEditingDefaultStack(MCExecContext& ctxt)
{
	MCdefaultstackptr->stopedit();
}

void MCInterfaceExecStopEditingGroup(MCExecContext& ctxt, MCGroup *p_target)
{
	p_target->getstack()->stopedit();
}
void MCInterfaceExecStopMovingObject(MCExecContext& ctxt, MCObject *p_target)
{
	MCscreen->stopmove(p_target, False);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecCloseStack(MCExecContext& ctxt, MCStack *p_target)
{						
	p_target->close();		
	p_target->checkdestroy();
}

void MCInterfaceExecCloseDefaultStack(MCExecContext& ctxt)
{
	MCInterfaceExecCloseStack(ctxt, MCdefaultstackptr);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecSaveStack(MCExecContext& ctxt, MCStack *p_target)
{
	MCInterfaceExecSaveStackAs(ctxt, p_target, kMCEmptyString);
}

void MCInterfaceExecSaveStackAs(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_new_filename)
{
	ctxt . SetTheResultToEmpty();
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;
	
	p_target -> saveas(p_new_filename);
}

////////////////////////////////////////////////////////////////////////////////
void MCInterfaceExecMoveObject(MCExecContext& ctxt, MCObject *p_target, MCPoint *p_motion, uindex_t p_motion_count, double p_duration, int p_units, bool p_wait, bool p_dispatch)
{
	if (p_motion_count < 2)
		return;

	switch (p_units)
	{
	case F_MILLISECS:
		p_duration /= 1000.0;
		break;
	case F_TICKS:
		p_duration /= 60.0;
		break;
	default:
		break;
	}

	MCPoint *t_motion = new MCPoint[p_motion_count];
	for (uindex_t i = 0; i < p_motion_count; i++)
		t_motion[i] = p_motion[i];

	MCscreen -> addmove(p_target, t_motion, p_motion_count, p_duration, p_wait);
	if (p_wait)
	{
		if (MCscreen->wait(p_duration, p_dispatch, False))
		{
			ctxt . LegacyThrow(EE_MOVE_ABORT);
			return;
		}
		MCscreen->stopmove(p_target, True);
	}
}

void MCInterfaceExecMoveObjectBetween(MCExecContext& ctxt, MCObject *p_target, MCPoint p_from, MCPoint p_to, double p_duration, int p_units, bool p_wait, bool p_dispatch)
{
	MCPoint t_motion[2];
	t_motion[0] = p_from;
	t_motion[1] = p_to;
	MCInterfaceExecMoveObject(ctxt, p_target, t_motion, 2, p_duration, p_units, p_wait, p_dispatch);
}

void MCInterfaceExecMoveObjectAlong(MCExecContext& ctxt, MCObject *p_target, MCPoint *p_motion, uindex_t p_motion_count, bool p_is_relative, double p_duration, int p_units, bool p_wait, bool p_dispatch)
{
	if (p_motion_count == 1)
	{		
		MCRectangle trect = p_target->getrect();
		MCPoint t_objloc;
		t_objloc.x = trect.x + (trect.width >> 1);
		t_objloc.y = trect.y + (trect.height >> 1);
		if (p_is_relative)
		{
			p_motion->x += t_objloc.x;
			p_motion->y += t_objloc.y;
		}
		MCInterfaceExecMoveObjectBetween(ctxt, p_target, t_objloc, *p_motion, p_duration, p_units, p_wait, p_dispatch);
	}
	else
		MCInterfaceExecMoveObject(ctxt, p_target, p_motion, p_motion_count, p_duration, p_units, p_wait, p_dispatch);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecHideGroups(MCExecContext& ctxt)
{
	MClinkatts.underline = False;
	
	// MW-2011-08-17: [[ Redraw ]] We've changed a global property that could
	//   affect the display of all stacks.
	MCRedrawDirtyScreen();
}

void MCInterfaceExecHideObject(MCExecContext& ctxt, MCObjectPtr p_target)
{
	p_target . object -> setboolprop(ctxt, 0, P_VISIBLE, False, false);
}

void MCInterfaceExecHideObjectWithEffect(MCExecContext& ctxt, MCObjectPtr p_target, MCVisualEffect *p_effect)
{
	// MW-2011-09-13: [[ Effects ]] Only apply the effect if the screen is not
	//   locked.
	if (MCRedrawIsScreenLocked())
		MCInterfaceExecHideObject(ctxt, p_target);
	else
	{	
		if (p_effect->exec(ctxt . GetEP()) != ES_NORMAL)
		{
			ctxt . LegacyThrow(EE_HIDE_BADEFFECT);
			return;
		}
		// MW-2010-04-26: [[ Bug 8661 ]] Make sure we use the effective rect for
		//   effectarea computation.
		MCRectangle t_rect;
		if (p_target . object -> gettype() >= CT_GROUP)
			t_rect = static_cast<MCControl *>(p_target . object) -> geteffectiverect();
		else
			t_rect = p_target . object -> getrect();
		
		// MW-2011-09-13: [[ Effects ]] Cache the rect we want to play the effect
		//   in.
		p_target . object -> getstack() -> snapshotwindow(t_rect);
		
		// MW-2011-11-15: [[ Bug 9846 ]] Lock the screen to prevent the snapshot
		//   being dumped inadvertantly.
		MCRedrawLockScreen();
		
		// MW-2011-11-15: [[ Bug 9846 ]] Make sure we use the same mechanism to
		//   set visibility as the non-effect case.
		p_target . object -> setboolprop(ctxt, 0, P_VISIBLE, False, false);
		
		MCRedrawUnlockScreen();
		
		// Run the effect - this will use the previously cached image.
		Boolean abort = False;
		p_target . object -> getstack() -> effectrect(t_rect, abort);
		
		if (abort)
			ctxt . LegacyThrow(EE_HANDLER_ABORT);
	}
}

void MCInterfaceExecHideMenuBar(MCExecContext& ctxt)
{
	MCscreen->hidemenu();
}

void MCInterfaceExecHideTaskBar(MCExecContext& ctxt)
{
	MCscreen->hidetaskbar();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecShowGroups(MCExecContext& ctxt)
{
	MClinkatts.underline = True;

	// MW-2011-08-17: [[ Redraw ]] We've changed a global property that could
	//   affect the display of all stacks.
	MCRedrawDirtyScreen();
}

void MCInterfaceExecShowAllCards(MCExecContext& ctxt)
{
	uint2 t_count;
	MCdefaultstackptr->count(CT_CARD, CT_UNDEFINED, NULL, t_count);
	MCdefaultstackptr->flip(t_count);
}

void MCInterfaceExecShowMarkedCards(MCExecContext& ctxt)
{
	uint2 t_count;
	MCdefaultstackptr->setmark();
	MCdefaultstackptr->count(CT_CARD, CT_UNDEFINED, NULL, t_count);
	MCdefaultstackptr->flip(t_count);
	MCdefaultstackptr->clearmark();
}

void MCInterfaceExecShowCards(MCExecContext& ctxt, uint2 p_count)
{
	MCdefaultstackptr->flip(p_count);
}

void MCInterfaceExecShowObject(MCExecContext& ctxt, MCObjectPtr p_target, MCPoint *p_at)
{
	if (p_at != nil)
		p_target.object->setpointprop(ctxt, p_target.part_id, P_LOCATION, False, *p_at);

	if (!ctxt.HasError())
	{
		p_target.object->setboolprop(ctxt, p_target.part_id, P_VISIBLE, False, kMCTrue);
		return;
	}
	
	ctxt.Throw();
}

void MCInterfaceExecShowObjectWithEffect(MCExecContext& ctxt, MCObjectPtr p_target, MCPoint *p_at, MCVisualEffect *p_effect)
{
	if (MCRedrawIsScreenLocked())
	{
		MCInterfaceExecShowObject(ctxt, p_target, p_at);
		return;
	}

	if (p_at != nil)
		p_target.object->setpointprop(ctxt, p_target.part_id, P_LOCATION, False, *p_at);
		
	if (ctxt.HasError())
		return;

	if (p_effect->exec(ctxt . GetEP()) != ES_NORMAL)
	{
		ctxt . LegacyThrow(EE_SHOW_BADEFFECT);
		return;
	}

	// MW-2010-04-26: [[ Bug 8661 ]] Make sure we use the effective rect for
	//   effectarea computation.
	MCRectangle t_rect;
	if (p_target . object -> gettype() >= CT_GROUP)
		t_rect = static_cast<MCControl *>(p_target . object) -> geteffectiverect();
	else
		t_rect = p_target . object -> getrect();
	
	// MW-2011-09-13: [[ Effects ]] Cache the rect we want to play the effect
	//   in.
	p_target . object -> getstack() -> snapshotwindow(t_rect);
	
	// MW-2011-11-15: [[ Bug 9846 ]] Lock the screen to prevent the snapshot
	//   being dumped inadvertantly.
	MCRedrawLockScreen();
	
	// MW-2011-11-15: [[ Bug 9846 ]] Make sure we use the same mechanism to
	//   set visibility as the non-effect case.
	p_target . object->setsprop(P_VISIBLE, kMCTrueString);
	
	MCRedrawUnlockScreen();
	
	// Run the effect - this will use the previously cached image.
	Boolean abort = False;
	p_target . object->getstack()->effectrect(t_rect, abort);
	
	if (abort)
		ctxt . LegacyThrow(EE_HANDLER_ABORT);
}

void MCInterfaceExecShowMenuBar(MCExecContext& ctxt)
{
	MCscreen->showmenu();
}

void MCInterfaceExecShowTaskBar(MCExecContext& ctxt)
{
	MCscreen->showtaskbar();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPopupButton(MCExecContext& ctxt, MCButton *p_target, MCPoint *p_at)
{
	if (MCmousestackptr == NULL)
	{
		ctxt . LegacyThrow(EE_SUBWINDOW_NOSTACK);
		return;
	}
	if (p_at != nil)
	{
		MCmousex = p_at -> x;
		MCmousey = p_at -> y;
	}

	p_target->setmenumode(WM_POPUP);
	if (p_target->findmenu())
	{
		if (MCbuttonstate)
			MCtargetptr -> mup(0);
		p_target->openmenu(True);
	}
}

void MCInterfaceExecSubwindow(MCExecContext& ctxt, MCStack *p_target, MCStack *p_parent, MCRectangle p_rect, int p_at, int p_aligned, int p_mode)
{
	// MW-2007-05-01: Reverting this as it causes problems :o(
	//stackptr -> setflag(True, F_VISIBLE);

	MCStack *olddefault = MCdefaultstackptr;
	Boolean oldtrace = MCtrace;
	MCtrace = False;
	if (p_mode >= WM_MODELESS)
		MCRedrawForceUnlockScreen();

	p_target->openrect(p_rect, (Window_mode)p_mode, p_parent, (Window_position)p_at, (Object_pos)p_aligned);

	if (MCwatchcursor)
	{
		MCwatchcursor = False;
		p_target->resetcursor(True);
		if (MCmousestackptr != NULL && MCmousestackptr != p_target)
			MCmousestackptr->resetcursor(True);
	}
	MCtrace = oldtrace;
	if (p_mode > WM_TOP_LEVEL)
		MCdefaultstackptr = olddefault;
}

void MCInterfaceExecDrawerOrSheetStack(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned, int p_mode)
{
	MCStack *parentptr;
    parentptr = nil;
    
    if (p_parent_name != nil)
    {
        parentptr = ctxt . GetObject()->getstack()->findstackname_oldstring(MCStringGetOldString(p_parent_name));
        if (parentptr == nil)
        {
            ctxt . LegacyThrow(EE_SUBWINDOW_BADEXP);
            return;
        }
    }
	if (p_parent_is_thisstack)
		parentptr = MCdefaultstackptr;
	if (parentptr == p_target)
		parentptr = nil;

	if (parentptr != nil)
	{
		if (!parentptr->getopened())
		{
			ctxt . LegacyThrow(EE_SUBWINDOW_BADEXP);
			return;
		}
		else
			MCInterfaceExecSubwindow(ctxt, p_target, parentptr, parentptr->getrect(), p_at, p_aligned, WM_DRAWER);
	}
	else if (MCdefaultstackptr->getopened() || MCtopstackptr == NULL)
		MCInterfaceExecSubwindow(ctxt, p_target, MCdefaultstackptr, MCdefaultstackptr->getrect(), p_at, p_aligned, WM_DRAWER);
	else
		MCInterfaceExecSubwindow(ctxt, p_target, MCtopstackptr, MCtopstackptr->getrect(), p_at, p_aligned, WM_DRAWER);
}

void MCInterfaceExecDrawerOrSheetStackByName(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname_oldstring(MCStringGetOldString(p_name));

	if (sptr == nil)
	{
		if (MCresult->isclear())
			ctxt. SetTheResultToStaticCString("can't find stack");
		return;
	}
	
	MCInterfaceExecDrawerOrSheetStack(ctxt, sptr, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, p_mode);
}

void MCInterfaceExecDrawerStack(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned)
{	
	MCInterfaceExecDrawerOrSheetStack(ctxt, p_target, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, WM_DRAWER);
}

void MCInterfaceExecDrawerStackByName(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned)
{	
	MCInterfaceExecDrawerOrSheetStackByName(ctxt, p_name, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, WM_DRAWER);
}

void MCInterfaceExecDrawerStackLegacy(MCExecContext& ctxt, MCStack *p_target, MCStringRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned)
{
	MCInterfaceExecDrawerStack(ctxt, p_target, parent, p_parent_is_thisstack, (int)p_at, (int)p_aligned);
}

void MCInterfaceExecDrawerStackByNameLegacy(MCExecContext& ctxt, MCStringRef p_name, MCStringRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned)
{
	MCInterfaceExecDrawerStackByName(ctxt, p_name, parent, p_parent_is_thisstack, (int)p_at, (int)p_aligned);
}

void MCInterfaceExecSheetStack(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_parent_name, bool p_parent_is_thisstack)
{
	MCInterfaceExecDrawerOrSheetStack(ctxt, p_target, p_parent_name, p_parent_is_thisstack, WP_DEFAULT, OP_CENTER, WM_SHEET);
}

void MCInterfaceExecSheetStackByName(MCExecContext& ctxt, MCStringRef p_name, MCStringRef p_parent_name, bool p_parent_is_thisstack)
{
	MCInterfaceExecDrawerOrSheetStackByName(ctxt, p_name, p_parent_name, p_parent_is_thisstack, WP_DEFAULT, OP_CENTER, WM_SHEET);
}

void MCInterfaceExecOpenStack(MCExecContext& ctxt, MCStack *p_target, int p_mode)
{
	if (MCdefaultstackptr->getopened() || MCtopstackptr == NULL)
		MCInterfaceExecSubwindow(ctxt, p_target, nil, MCdefaultstackptr->getrect(), WP_DEFAULT, OP_NONE, p_mode);
	else
		MCInterfaceExecSubwindow(ctxt, p_target, nil, MCtopstackptr->getrect(), WP_DEFAULT, OP_NONE, p_mode);
}

void MCInterfaceExecOpenStackByName(MCExecContext& ctxt, MCStringRef p_name, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname_oldstring(MCStringGetOldString(p_name));

	if (sptr == nil)
	{
		if (MCresult->isclear())
			ctxt. SetTheResultToStaticCString("can't find stack");
		return;
	}
	
	MCInterfaceExecOpenStack(ctxt, sptr, p_mode);
}

void MCInterfaceExecPopupStack(MCExecContext& ctxt, MCStack *p_target, MCPoint *p_at, int p_mode)
{
	Boolean oldtrace = MCtrace;
	MCU_watchcursor(ctxt . GetObject()->getstack(), False);

	// MW-2007-04-10: [[ Bug 4260 ]] We shouldn't attempt to attach a menu to a control that is descendent of itself
	if (MCtargetptr -> getstack() == p_target)
	{
		ctxt . LegacyThrow(EE_SUBWINDOW_BADEXP);
		return;
	}

	if (MCtargetptr->attachmenu(p_target))
	{
		if (p_mode == WM_POPUP && p_at != nil)
		{
			MCmousex = p_at -> x;
			MCmousey = p_at -> y;
		}
		MCRectangle t_rect;
		t_rect = MCU_recttoroot(MCtargetptr->getstack(), MCtargetptr->getrect());
		MCInterfaceExecSubwindow(ctxt, p_target, nil, t_rect, WP_DEFAULT, OP_NONE, p_mode);
		if (!MCabortscript)
			return;

		MCtrace = oldtrace;
		ctxt . Throw();	
	}
}

void MCInterfaceExecPopupStackByName(MCExecContext& ctxt, MCStringRef p_name, MCPoint *p_at, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname_oldstring(MCStringGetOldString(p_name));

	if (sptr == nil)
	{
		if (MCresult->isclear())
			ctxt. SetTheResultToStaticCString("can't find stack");
		return;
	}
	
	MCInterfaceExecPopupStack(ctxt, sptr, p_at, p_mode);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecCreateStack(MCExecContext& ctxt, MCObject *p_object, MCStringRef p_new_name, bool p_force_invisible, bool p_with_group)
{
	MCStack *odefaultstackptr = MCdefaultstackptr;
	Boolean wasvisible = MCtemplatestack->isvisible();

	if (p_force_invisible)
		MCtemplatestack->setflag(!p_force_invisible, F_VISIBLE);

	MCdefaultstackptr = MCtemplatestack->clone();
	MCdefaultstackptr->open();

	if (p_with_group)
	{
		MCGroup *t_group = (MCGroup *)p_object;
		MCdefaultstackptr->setrect(t_group->getstack()->getrect());
		t_group = (MCGroup *)t_group->clone(False, OP_NONE, false);
		t_group->setparent(MCdefaultstackptr);
		t_group->resetfontindex(p_object->getstack());
		t_group->attach(OP_NONE, false);
	}
	else if (p_object != nil)
	{
		MCAutoStringRef t_name;
		p_object->names(P_NAME, &t_name);
		MCdefaultstackptr->setstringprop(ctxt, 0, P_MAIN_STACK, False, *t_name);
		if (ctxt . HasError())
		{
			delete MCdefaultstackptr;
			ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
			return;
		}
	}

	MCtemplatestack->setflag(wasvisible, F_VISIBLE);
	MCObject *t_object = MCdefaultstackptr;
	MCdefaultstackptr = odefaultstackptr;

	if (p_new_name != nil)
		t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
	
	MCAutoStringRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}


void MCInterfaceExecCreateStack(MCExecContext& ctxt, MCStack *p_owner, MCStringRef p_new_name, bool p_force_invisible)
{
	MCInterfaceExecCreateStack(ctxt, p_owner, p_new_name, p_force_invisible, false);
}

void MCInterfaceExecCreateStackWithGroup(MCExecContext& ctxt, MCGroup *p_group_to_copy, MCStringRef p_new_name, bool p_force_invisible)
{
	MCInterfaceExecCreateStack(ctxt, p_group_to_copy, p_new_name, p_force_invisible, true);
}

void MCInterfaceExecCreateCard(MCExecContext& ctxt, MCStringRef p_new_name, bool p_force_invisible)
{
	if (MCdefaultstackptr->islocked())
	{
		ctxt . LegacyThrow(EE_CREATE_LOCKED);
		return;
	}

	MCdefaultstackptr->stopedit();
	MCObject *t_object = MCtemplatecard->clone(True, False);

	if (p_new_name != nil)
		t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
	
	MCAutoStringRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}

MCControl* MCInterfaceExecCreateControlGetObject(MCExecContext& ctxt, int p_type, MCGroup *&r_parent)
{
	switch (p_type)
	{
	case CT_BACKGROUND:
	case CT_GROUP:
		return MCtemplategroup;
	case CT_BUTTON:
		return MCtemplatebutton;
	case CT_MENU:
		r_parent = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
		return MCtemplatebutton;
	case CT_SCROLLBAR:
		return MCtemplatescrollbar;
	case CT_PLAYER:
		return MCtemplateplayer;
	case CT_IMAGE:
		return MCtemplateimage;
	case CT_GRAPHIC:
		return MCtemplategraphic;
	case CT_EPS:
		return MCtemplateeps;
	case CT_FIELD:
		return MCtemplatefield;
	default:
		return NULL;
	}
}

void MCInterfaceExecCreateControl(MCExecContext& ctxt, MCStringRef p_new_name, int p_type, MCGroup *p_container, bool p_force_invisible)
{
	if (MCdefaultstackptr->islocked())
	{
		ctxt . LegacyThrow(EE_CREATE_LOCKED);
		return;
	}

	MCControl *t_control = MCInterfaceExecCreateControlGetObject(ctxt, p_type, p_container);
	if (t_control == NULL)
		return;
	Boolean wasvisible = t_control->isvisible();
	if (p_force_invisible)
		t_control->setflag(!p_force_invisible, F_VISIBLE);
	t_control->setparent(p_container);
	MCObject *t_object = t_control->clone(True, OP_CENTER, false);
	if (t_control == MCInterfaceExecCreateControlGetObject(ctxt, p_type, p_container))
	{ // handle case where template reset
		t_control->setparent(NULL);
		if (p_force_invisible)
			t_control->setflag(wasvisible, F_VISIBLE);
	}
	if (p_type == CT_MENU)
	{
		MCButton *t_button = (MCButton *)t_object;
		t_button->setupmenu();
	}

	if (p_new_name != nil)
		t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);

	MCAutoStringRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecClone(MCExecContext& ctxt, MCObject *p_target, MCStringRef p_new_name, bool p_force_invisible)
{
	MCStack *odefaultstackptr = MCdefaultstackptr;

	MCObject *t_object;
	switch (p_target->gettype())
	{
	case CT_STACK:
		{
			MCStack *t_stack = (MCStack *)p_target;
			t_object = t_stack->clone();
			if (p_new_name == nil)
			{
				MCStringRef t_short_name;
				t_stack->names(P_SHORT_NAME, t_short_name);
				MCAutoStringRef t_new_name;
				MCStringMutableCopyAndRelease(t_short_name, &t_new_name);
				MCStringPrependNativeChars(*t_new_name, (const char_t *)MCcopystring, strlen(MCcopystring));
				t_object->setstringprop(ctxt, 0, P_NAME, False, *t_new_name);
			}
			MCdefaultstackptr = (MCStack *)t_object;

			// OK-2008-06-23: [[Bug 6590]]
			if (p_force_invisible)
				t_object->setflag(!p_force_invisible, F_VISIBLE);

			t_object->open();
		}
		break;
	case CT_CARD:
		// MW-2005-03-10: Fix issue with card cloning which meant it wasn't working...
		if ((p_target -> getstack() -> islocked() && MCdefaultstackptr == p_target -> getstack()) ||
		        (MCdefaultstackptr != p_target -> getstack() && MCdefaultstackptr -> islocked()))
		{
			ctxt . LegacyThrow(EE_CLONE_LOCKED);
			return;
		}
		else if (MCdefaultstackptr != p_target -> getstack() &&
		         (!p_target -> getstack() -> iskeyed() || !MCdefaultstackptr -> iskeyed()))
		{
			ctxt . LegacyThrow(EE_CLONE_CANTCLONE);
			return;
		}
		else
		{
			MCCard *t_card = (MCCard *)p_target;
			t_card->getstack()->stopedit();
			t_object = t_card->clone(True, True);
		}
		break;
	case CT_GROUP:
		// MW-2010-10-12: [[ Bug 8494 ]] Surely its just the group being edited that you
		//   don't want to clone... Indeed, it shouldn't even be possible to reference
		//   that group since it 'doesn't exist' in group editing mode.
		if (p_target->getstack()->isediting() && p_target->getstack()->getediting() == p_target)
		{
			t_object = nil;
			break;
		}
	case CT_BUTTON:
	case CT_FIELD:
	case CT_IMAGE:
	case CT_SCROLLBAR:
	case CT_PLAYER:
	case CT_GRAPHIC:
	case CT_EPS:
	case CT_COLOR_PALETTE:
	case CT_MAGNIFY:
		if (p_target -> getstack() -> islocked())
		{
			ctxt . LegacyThrow(EE_CLONE_LOCKED);
			return;
		}
		else
		{
			MCControl *coptr = (MCControl *)p_target;
			t_object = coptr -> clone(True, OP_OFFSET, p_force_invisible);
		}
		break;
	default:
		break;
	}

	if (t_object == nil)
	{
		ctxt . LegacyThrow(EE_CLONE_CANTCLONE);
		return;
	}

	if (p_new_name != nil)
		t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
	
	MCAutoStringRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);

	MCdefaultstackptr = odefaultstackptr;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPutIntoField(MCExecContext& ctxt, MCStringRef p_string, int p_where, MCObjectChunkPtr p_chunk)
{
	if (p_chunk . chunk == CT_UNDEFINED && p_where == PT_INTO)
	{
		p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, p_string);
	}
	else
	{
        MCField *t_field;
        t_field = static_cast<MCField *>(p_chunk . object);
		integer_t t_start, t_finish;
		if (p_where == PT_INTO)
			t_start = p_chunk . mark . start, t_finish = p_chunk . mark . finish;
		else if (p_where == PT_AFTER)
			t_start = t_finish = p_chunk . mark . finish;
		else /* PT_BEFORE */
			t_start = t_finish = p_chunk . mark . start;
		integer_t t_si, t_ei;
        t_si = 0;
        t_ei = INT32_MAX;
        t_field -> resolvechars(p_chunk . part_id, t_si, t_ei, t_start, t_finish - t_start);
		if (t_field -> settextindex_stringref(p_chunk . part_id, t_si, t_ei, p_string, False) != ES_NORMAL)
		{
			ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
			return;
		}
	}
}

void MCInterfaceExecPutUnicodeIntoField(MCExecContext& ctxt, MCDataRef p_data, int p_where, MCObjectChunkPtr p_chunk)
{
	if (p_chunk . chunk == CT_UNDEFINED && p_where == PT_INTO)
	{
		p_chunk . object -> setdataprop(ctxt, p_chunk . part_id, P_UNICODE_TEXT, False, p_data);
	}
	else
	{
        MCAutoStringRef t_string;
        if (MCStringDecode(p_data, kMCStringEncodingUTF16, false, &t_string))
        {
            MCInterfaceExecPutIntoField(ctxt, *t_string, p_where, p_chunk);
            return;
        }
        
        ctxt.Throw();
   	}
}

void MCInterfaceExecPutIntoObject(MCExecContext& ctxt, MCStringRef p_string, int p_where, MCObjectChunkPtr p_chunk)
{
	if (p_where == PT_INTO && p_chunk . chunk == CT_UNDEFINED)
	{
		p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, p_string);
	}
	else
	{
		MCStringRef t_current_value;
		p_chunk . object -> getstringprop(ctxt, p_chunk . part_id, P_TEXT, False, t_current_value);
		if (ctxt . HasError())
			return;
		
		MCAutoStringRef t_mutable_current_value;
		if (!MCStringMutableCopyAndRelease(t_current_value, &t_mutable_current_value))
		{
			MCValueRelease(t_current_value);
			ctxt . Throw();
			return;
		}
		
		integer_t t_start, t_finish;
		if (p_where == PT_INTO)
			t_start = p_chunk . mark . start, t_finish = p_chunk . mark . finish;
		else if (p_where == PT_AFTER)
			t_start = t_finish = p_chunk . mark . finish;
		else /* PT_BEFORE */
			t_start = t_finish = p_chunk . mark . start;
		
		if (!MCStringReplace(*t_mutable_current_value, MCRangeMake(t_start, t_finish), p_string))
		{
			ctxt . Throw();
			return;
		}
		
		p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, *t_mutable_current_value);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecLockCursor(MCExecContext& ctxt)
{
	MClockcursor = True;
}

void MCInterfaceExecLockMenus(MCExecContext& ctxt)
{
	MClockmenus = True;
}

void MCInterfaceExecLockMoves(MCExecContext& ctxt)
{
	MClockmoves = True;
}

void MCInterfaceExecLockRecent(MCExecContext& ctxt)
{
	MClockrecent = True;
}

void MCInterfaceExecLockScreen(MCExecContext& ctxt)
{
	MCRedrawLockScreen();
}

void MCInterfaceExecLockScreenForEffect(MCExecContext& ctxt, MCRectangle *p_region)
{
	// MW-2011-09-13: [[ Effects ]] If the screen is not locked capture a snapshot
	//   of the default stack.
	if (!MCRedrawIsScreenLocked())
	{
		// MW-2011-09-24: [[ Effects ]] Process the 'rect' clause (if any).
		if (p_region == nil)
			MCcur_effects_rect = MCdefaultstackptr -> getcurcard() -> getrect();
		else
			MCcur_effects_rect = MCRectangleMake(0,0,0,0);
		
		
		MCdefaultstackptr -> snapshotwindow(MCcur_effects_rect);
	}
	
	MCRedrawLockScreen();
}

void MCInterfaceExecUnlockCursor(MCExecContext& ctxt)
{
	MClockcursor = False;
	MCdefaultstackptr->resetcursor(False);
}

void MCInterfaceExecUnlockMenus(MCExecContext& ctxt)
{
	MClockmenus = False;
	MCscreen->updatemenubar(True);
}

void MCInterfaceExecUnlockMoves(MCExecContext& ctxt)
{
	MClockmoves = False;
}

void MCInterfaceExecUnlockRecent(MCExecContext& ctxt)
{
	MClockrecent = False;
}

void MCInterfaceExecUnlockScreen(MCExecContext& ctxt)
{
	MCRedrawUnlockScreenWithEffects();
}

void MCInterfaceExecUnlockScreenWithEffect(MCExecContext& ctxt, MCVisualEffect *p_effect)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	if (p_effect -> exec(ctxt . GetEP()) != ES_NORMAL)
	{
		ctxt . LegacyThrow(EE_UNLOCK_BADEFFECT);
		return;
	}
	
	MCRedrawUnlockScreenWithEffects();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecImportSnapshot(MCExecContext& ctxt, MCStringRef p_display, MCRectangle *p_region, uint4 p_window)
{
	if (!ctxt . EnsurePrivacyIsAllowed())
		return;

	if (MCdefaultstackptr->islocked())
	{
		ctxt . LegacyThrow(EE_IMPORT_LOCKED);
		return;
	}

	MCRectangle t_rect;
	if (p_region == nil)
	{
		t_rect.x = t_rect.y = -32768;
		t_rect.width = t_rect.height = 0;
	}
	else	
		t_rect = *p_region;
	// TODO - graphics refactor
    /*
	MCBitmap *t_bitmap = nil;
	t_bitmap = MCscreen->snapshot(t_rect, p_window, p_display);
	
	if (t_bitmap != nil)
	{
		MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
		iptr->compress(t_bitmap, true, true);
		iptr->attach(OP_CENTER, false);
		MCscreen->destroyimage(t_bitmap);
	} */
}
void MCInterfaceExecImportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region)
{
	MCInterfaceExecImportSnapshot(ctxt, nil, p_region, 0);
}

void MCInterfaceExecImportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_IMPORT_BADNAME);
	else
		MCInterfaceExecImportSnapshot(ctxt, p_display, p_region, t_window);
}
void MCInterfaceExecImportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size)
{
// TODO - graphics refactor
/*
	if (MCdefaultstackptr->islocked())
	{	
		ctxt . LegacyThrow(EE_IMPORT_LOCKED);
		return;
	}

	MCBitmap *t_bitmap = nil;
	t_bitmap = p_target -> snapshot(p_region, p_at_size, p_with_effects);
	
	// OK-2007-04-24: If the import rect doesn't intersect with the object, MCobject::snapshot
	// may return null. In this case, return an error.
	if (t_bitmap == nil)
	{
		ctxt . LegacyThrow(EE_IMPORT_EMPTYRECT);
		return;
	}
	
	if (t_bitmap != nil)
	{
		MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
		iptr->compress(t_bitmap, true, true);
		iptr->attach(OP_CENTER, false);
		MCscreen->destroyimage(t_bitmap);
	}
 */
}

void MCInterfaceExecImportGetStream(MCExecContext& ctxt, MCStringRef p_filename, IO_handle &r_stream)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	if (MCdefaultstackptr->islocked())
	{
		ctxt . LegacyThrow(EE_IMPORT_LOCKED);
		return;
	}

	r_stream = MCS_open(p_filename, kMCSOpenFileModeRead, True, False, 0);
}

void MCInterfaceExecImportAudioClip(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCU_watchcursor(ctxt . GetObject()->getstack(), True);

	IO_handle t_stream = NULL;
	MCInterfaceExecImportGetStream(ctxt, p_filename, t_stream);

	if (t_stream != NULL)
	{
		MCAudioClip *aptr = new MCAudioClip;
		if (!aptr->import(p_filename, t_stream))
		{
			ctxt . LegacyThrow(EE_IMPORT_CANTREAD);
			delete aptr;
		}
		else
			MCdefaultstackptr->appendaclip(aptr);
		MCS_close(t_stream);
	}
	else
		ctxt . LegacyThrow(EE_IMPORT_CANTOPEN);

	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ctxt . GetObject()->getstack(), True);
}
void MCInterfaceExecImportVideoClip(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCU_watchcursor(ctxt . GetObject()->getstack(), True);

	IO_handle t_stream = NULL;
	MCInterfaceExecImportGetStream(ctxt, p_filename, t_stream);

	if (t_stream != NULL)
	{
		MCVideoClip *vptr = new MCVideoClip;
		if (!vptr->import(p_filename, t_stream))
		{
			ctxt . LegacyThrow(EE_IMPORT_CANTREAD);
			delete vptr;
		}
		else
			MCdefaultstackptr->appendvclip(vptr);
		MCS_close(t_stream);
	}
	else
		ctxt . LegacyThrow(EE_IMPORT_CANTOPEN);

	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ctxt . GetObject()->getstack(), True);
}
void MCInterfaceExecImportImage(MCExecContext& ctxt, MCStringRef p_filename, MCStringRef p_mask_filename, MCObject *p_container)
{
	MCU_watchcursor(ctxt . GetObject()->getstack(), True);

	IO_handle t_stream = NULL;
	MCInterfaceExecImportGetStream(ctxt, p_filename, t_stream);

	if (t_stream != NULL)
	{
		IO_handle t_mask_stream = NULL;
		if (p_mask_filename != nil)
			MCInterfaceExecImportGetStream(ctxt, p_mask_filename, t_mask_stream);
		if (p_mask_filename == nil || t_mask_stream != NULL)
		{
			MCtemplateimage->setparent(p_container);
			MCImage *t_image = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
			MCtemplateimage->setparent(NULL);
			t_image->setflag(True, F_I_ALWAYS_BUFFER);
			if (t_image->import(MCStringGetCString(p_filename), t_stream, t_mask_stream) == IO_NORMAL)
				t_image->attach(OP_CENTER, false);
			else
			{
				ctxt . LegacyThrow(EE_IMPORT_CANTREAD);
				delete t_image;
			}
			if (t_mask_stream != NULL)
				MCS_close(t_mask_stream);
		}
		else
			ctxt . LegacyThrow(EE_IMPORT_CANTOPEN);
		MCS_close(t_stream);
	}
	else
		ctxt . LegacyThrow(EE_IMPORT_CANTOPEN);

	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ctxt . GetObject()->getstack(), True);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExportBitmap(MCExecContext &ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCStringRef &r_data)
{
	bool t_success = true;
	
	MCImagePaletteSettings t_palette_settings;
	MCImagePaletteSettings *t_ps_ptr = nil;
	if (p_palette != nil)
	{
		t_palette_settings . type = p_palette -> type;
		if (p_palette -> type == kMCImagePaletteTypeCustom)
		{
			t_palette_settings . colors = p_palette -> custom . colors;
			t_palette_settings . ncolors = p_palette -> custom . count;
		}
		else if (p_palette -> type == kMCImagePaletteTypeOptimal)
			t_palette_settings . ncolors = p_palette -> optimal . palette_size;
		else
			t_palette_settings . ncolors = 0;
		t_ps_ptr = &t_palette_settings;
	}
	
	IO_handle t_stream = nil;
	/* UNCHECKED */ t_stream = MCS_fakeopenwrite();
	t_success = MCImageExport(p_bitmap, (Export_format)p_format, t_ps_ptr, p_dither, t_stream, nil);
	
	MCAutoNativeCharArray t_autobuffer;
	void *t_buffer = nil;
	size_t t_size = 0;
	MCS_closetakingbuffer(t_stream, t_buffer, t_size);
	t_autobuffer.Give((char_t*)t_buffer, t_size);

	if (!t_success)
	{
		ctxt.LegacyThrow(EE_EXPORT_CANTWRITE);
		
		return;
	}
	
	/* UNCHECKED */ t_autobuffer.CreateStringAndRelease(r_data);
}

void MCInterfaceExportBitmapToFile(MCExecContext& ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	IO_handle t_mstream = nil;
	if (p_mask_filename != nil)
	{
		
		if ((t_mstream = MCS_open(p_mask_filename, kMCSOpenFileModeWrite, False, False, 0)) == nil)
		{
			ctxt . LegacyThrow(EE_EXPORT_CANTOPEN);
			return;
		}
	}
	IO_handle t_fstream;
	if ((t_fstream = MCS_open(p_filename, kMCSOpenFileModeWrite, False, False, 0)) == nil)
	{
		ctxt . LegacyThrow(EE_EXPORT_CANTOPEN);
		if (t_mstream != nil)
			MCS_close(t_mstream);
		return;
	}
	
	MCImagePaletteSettings t_palette_settings;
	MCImagePaletteSettings *t_ps_ptr = nil;
	if (p_palette != nil)
	{
		t_palette_settings . type = p_palette -> type;
		if (p_palette -> type == kMCImagePaletteTypeCustom)
		{
			t_palette_settings . colors = p_palette -> custom . colors;
			t_palette_settings . ncolors = p_palette -> custom . count;
		}
		else if (p_palette -> type == kMCImagePaletteTypeOptimal)
			t_palette_settings . ncolors = p_palette -> optimal . palette_size;
		else
			t_palette_settings . ncolors = 0;
		t_ps_ptr = &t_palette_settings;
	}
	
	bool t_delete_file = false;
	if (!MCImageExport(p_bitmap, (Export_format)p_format, t_ps_ptr, p_dither, t_fstream, t_mstream))
	{
		t_delete_file = true;
		ctxt . LegacyThrow(EE_EXPORT_CANTWRITE);
	}
	
	MCS_close(t_fstream);
	if (t_mstream != nil)
		MCS_close(t_mstream);
	
	if (t_delete_file)
		MCS_unlink(p_filename);
}

MCImageBitmap* MCInterfaceGetSnapshotBitmap(MCExecContext &ctxt, MCStringRef p_display, MCRectangle *p_region, uint4 p_window)
{
	MCRectangle t_rect;
	if (p_region == nil)
	{
		t_rect.x = t_rect.y = -32768;
		t_rect.width = t_rect.height = 0;
	}
	else	
		t_rect = *p_region;
	
	MCBitmap *t_bitmap = nil;
	MCImageBitmap *t_image_bitmap = nil;
    // TODO - graphics refactor
    /*
	t_bitmap = MCscreen->snapshot(t_rect, p_window, p_display);
	if (t_bitmap == nil)
	{
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
	}
	else
	{
		MCImageBitmapCreateWithOldBitmap(t_bitmap, t_image_bitmap);
		MCscreen->destroyimage(t_bitmap);
	}
    */
	return t_image_bitmap;
}

bool MCInterfaceGetDitherImage(MCImage *p_image)
{
	if (p_image == nil)
		p_image = MCtemplateimage;
	
	return !p_image->getflag(F_DONT_DITHER);
}

void MCInterfaceExecExportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef &r_data)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, nil, p_region, 0);
	MCInterfaceExportBitmap(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), r_data);
}

void MCInterfaceExecExportSnapshotOfScreenToFile(MCExecContext& ctxt, MCRectangle *p_region, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, nil, p_region, 0);
	MCInterfaceExportBitmapToFile(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_filename, p_mask_filename);
}

void MCInterfaceExecExportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef &r_data)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
	else
	{
		MCImageBitmap *t_bitmap;
		t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, p_display, p_region, t_window);
		MCInterfaceExportBitmap(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), r_data);
	}
}

void MCInterfaceExecExportSnapshotOfStackToFile(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
	else
	{
		MCImageBitmap *t_bitmap;
		t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, p_display, p_region, t_window);
		MCInterfaceExportBitmapToFile(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_filename, p_mask_filename);
	}
}

MCImageBitmap *MCInterfaceGetSnapshotOfObjectBitmap(MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size)
{
	MCBitmap *t_bitmap = nil;
	MCImageBitmap *t_image_bitmap = nil;
    // TODO - graphics refactor
    /*
	t_bitmap = p_target -> snapshot(p_region, p_at_size, p_with_effects);
	if (!t_bitmap == nil)
	{
		MCImageBitmapCreateWithOldBitmap(t_bitmap, t_image_bitmap);
		MCscreen->destroyimage(t_bitmap);
	}
	return t_image_bitmap;
     */
}

void MCInterfaceExecExportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef &r_data)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotOfObjectBitmap(p_target, p_region, p_with_effects, p_at_size);
	MCInterfaceExportBitmap(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), r_data);
}
void MCInterfaceExecExportSnapshotOfObjectToFile(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotOfObjectBitmap(p_target, p_region, p_with_effects, p_at_size);
	MCInterfaceExportBitmapToFile(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_filename, p_mask_filename);
}

MCImage* MCInterfaceExecExportSelectImage(MCExecContext& ctxt)
{
	MCObject *optr = MCselected->getfirst();
	if (optr == nil)
	{
		MCCard *cardptr = MCdefaultstackptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
		optr = cardptr->getchild(CT_LAST, kMCEmptyString, CT_IMAGE, CT_UNDEFINED);
	}
	if (optr == nil || !optr->getopened())
	{
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
		return nil;
	}
	if (optr->gettype() != CT_IMAGE)
	{
		ctxt . LegacyThrow(EE_EXPORT_NOTANIMAGE);
		return nil;
	}
	return (MCImage *)optr;
}

void MCInterfaceExecExportImage(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef &r_data)
{
	if (p_target == nil)
		p_target = MCInterfaceExecExportSelectImage(ctxt);
	if (p_target != nil)
	{
		if (p_target->getrect() . width == 0 || p_target -> getrect() . height == 0)
		{
			MCStringCopy(kMCEmptyString, r_data);
			return;
		}
        // TODO - graphics refactor
        /*
		MCImageBitmap *t_bitmap;
		if (p_target->lockbitmap(t_bitmap))
			MCInterfaceExportBitmap(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(p_target), r_data);
		p_target->unlockbitmap(t_bitmap);
         */
	}
}
void MCInterfaceExecExportImageToFile(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	if (p_target == nil)
		p_target = MCInterfaceExecExportSelectImage(ctxt);
	if (p_target != nil)
	{
        // TODO - graphics refactor
        /*
		MCImageBitmap *t_bitmap;
		if (p_target->lockbitmap(t_bitmap))
			MCInterfaceExportBitmapToFile(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(p_target), p_filename, p_mask_filename);
		p_target->unlockbitmap(t_bitmap);
         */
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecSortAddItem(MCExecContext &ctxt, MCSortnode *items, uint4 &nitems, int form, MCValueRef p_input, MCExpression *by)
{
	MCAutoValueRef t_output;
	if (by != NULL)
	{
		MCerrorlock++;
		ctxt . GetEP() . setvalueref(p_input);
		MCeach->set(ctxt . GetEP());
		if (by->eval(ctxt . GetEP()) == ES_NORMAL)
			t_output = MCValueRetain(ctxt.GetEP().getvalueref());
		else
			t_output = MCValueRetain(kMCEmptyString);
		MCerrorlock--;
	}
	else
		t_output = MCValueRetain(p_input);
	
	MCAutoStringRef t_converted;
	switch (form)
	{
	case ST_DATETIME:
		if (MCD_convert(ctxt, *t_output, CF_UNDEFINED, CF_UNDEFINED, CF_SECONDS, CF_UNDEFINED, &t_converted))
			if (ctxt.ConvertToNumber(*t_converted, items[nitems].nvalue))
				break;
	
		/* UNCHECKED */ MCNumberCreateWithReal(-MAXREAL8, items[nitems].nvalue);
		break;
			
	case ST_NUMERIC:
		if (ctxt.ConvertToNumber(*t_output, items[nitems].nvalue))
			break;
			
		/* UNCHECKED */ MCNumberCreateWithReal(-MAXREAL8, items[nitems].nvalue);
		break;
			
	default:
		if (ctxt . GetCaseSensitive())
			/* UNCHECKED */ ctxt.ConvertToString(*t_output, items[nitems].svalue);
		else
		{
			MCStringRef t_fixed, t_mutable;
			/* UNCHECKED */ ctxt.ConvertToString(*t_output, t_fixed);
			/* UNCHECKED */ MCStringMutableCopyAndRelease(t_fixed, t_mutable);
			/* UNCHECKED */ MCStringLowercase(t_mutable);
			/* UNCHECKED */ MCStringCopyAndRelease(t_mutable, items[nitems].svalue);
		}
			
		break;
	}
	nitems++;
}

bool MCInterfaceExecSortContainer(MCExecContext &ctxt, MCStringRef p_data, int p_type, Sort_type p_direction, int p_form, MCExpression *p_by, MCStringRef &r_output)
{
	if (MCStringGetLength(p_data) == 0)
	{
		MCStringCopy(kMCEmptyString, r_output);
		return true;
	}

	// If sorting items of the container, then we use the current itemDelimiter to split each item,
	// all other forms of search default to the lineDelimiter for now. Note that this is a slight
	// change of behavior as previously sorting containers by line ignored the lineDelimiter and
	// always delimited by ascii 10.
	char t_delimiter;
	if (p_type == CT_ITEM)
		t_delimiter = ctxt . GetItemDelimiter();
	else
		t_delimiter = ctxt . GetLineDelimiter();

	if (t_delimiter == '\0')
		return false;

	uindex_t t_item_count;
	t_item_count = 0;

	// Calculate the number of items we need to sort, store this in t_item_count.
	uint4 t_item_size;
	t_item_size = MCStringGetLength(p_data);

	MCAutoPointer<char> t_item_text;
	t_item_text = strclone(MCStringGetCString(p_data));

	char *t_string_pointer;
	t_string_pointer = *t_item_text;

	char *t_end_pointer;
	bool t_trailing_delim = false;
	while ((t_end_pointer = strchr(t_string_pointer, t_delimiter)) != NULL)
	{
		// knock out last delim for lines with a trailing return char
		if (p_type != CT_ITEM && t_end_pointer[1] == '\0')
		{
			t_end_pointer[0] = '\0';
			t_trailing_delim = true;
		}
		else
			t_item_count++;
		t_string_pointer = t_end_pointer + 1;
	}

	// OK-2008-12-11: [[Bug 7503]] - If there are 0 items in the string, don't carry out the search,
	// this keeps the behavior consistent with previous versions of Revolution.
	if (t_item_count < 1)
	{
		MCStringCopy(p_data, r_output);
		return true;
	}

	// Now we know the item count, we can allocate an array of MCSortnodes to store them.
	MCAutoArray<MCSortnode> t_items;
	t_items.Extend(t_item_count + 1);
	t_item_count = 0;
	t_string_pointer = *t_item_text;

	// Next, populate the MCSortnodes with all the items to be sorted
	do
	{
		MCAutoStringRef t_string;
		if ((t_end_pointer = strchr(t_string_pointer, t_delimiter)) != NULL)
		{
			*t_end_pointer++ = '\0';
			 MCStringCreateWithNativeChars((const char_t *)t_string_pointer, t_end_pointer - t_string_pointer - 1, &t_string);
		}
		else
			MCStringCreateWithNativeChars((const char_t *)t_string_pointer, strlen(t_string_pointer), &t_string);

		MCInterfaceExecSortAddItem(ctxt, t_items.Ptr(), t_item_count, p_form, *t_string, p_by);

		t_items[t_item_count - 1] . data = (void *)t_string_pointer;
		t_string_pointer = t_end_pointer;
	}
	while(t_end_pointer != NULL);

	// Sort the array
	MCU_sort(t_items.Ptr(), t_item_count, p_direction, (Sort_type)p_form);

	// Build the output string
	MCAutoPointer<char> t_output;
	t_output = new char[t_item_size + 1];
	(*t_output)[0] = '\0';
	
	uint4 t_length;
	t_length = 0;

	for (unsigned int i = 0; i < t_item_count; i++)
	{
		uint4 t_item_length;
		t_item_length = strlen((const char *)t_items[i] . data);
		strncpy(&(*t_output)[t_length], (const char *)t_items[i] . data, t_item_length);
		t_length = t_length + t_item_length;

		if (t_trailing_delim || i < t_item_count - 1)
			(*t_output)[t_length++] = t_delimiter;
	}
	(*t_output)[t_length] = '\0';

	MCStringCreateWithNativeChars((const char_t *)*t_output, t_length, r_output);

	return true; 
}


void MCInterfaceExecSortCardsOfStack(MCExecContext &ctxt, MCStack *p_target, bool p_ascending, int p_format, MCExpression *p_by, bool p_only_marked)
{
	if (p_target == nil)
		p_target = MCdefaultstackptr;

	if (p_target->sort(ctxt, p_ascending ? ST_ASCENDING : ST_DESCENDING, (Sort_type)p_format, p_by, p_only_marked) != ES_NORMAL)
		ctxt . LegacyThrow(EE_SORT_CANTSORT);
}

void MCInterfaceExecSortField(MCExecContext &ctxt, MCObjectPtr p_target, int p_chunk_type, bool p_ascending, int p_format, MCExpression *p_by)
{
	MCField *t_field =(MCField *)p_target . object;
	if (t_field->sort(ctxt, p_target . part_id, (Chunk_term)p_chunk_type, p_ascending ? ST_ASCENDING : ST_DESCENDING, (Sort_type)p_format, p_by) != ES_NORMAL)
		ctxt . LegacyThrow(EE_SORT_CANTSORT);
}

void MCInterfaceExecSortContainer(MCExecContext &ctxt, MCStringRef& x_target, int p_chunk_type, bool p_ascending, int p_format, MCExpression *p_by)
{
	MCAutoStringRef t_sorted_string;

	if (MCInterfaceExecSortContainer(ctxt, x_target, p_chunk_type, p_ascending ? ST_ASCENDING : ST_DESCENDING, p_format, p_by, &t_sorted_string))
	{
		x_target = (MCStringRef)MCValueRetain(*t_sorted_string);
		return;
	}
	
	ctxt . LegacyThrow(EE_SORT_CANTSORT);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecFind(MCExecContext& ctxt, int p_mode, MCStringRef p_needle, MCChunk *p_target)
{
	if (MCStringGetLength(p_needle) == 0)
	{
		if (MCfoundfield != NULL)
			MCfoundfield->clearfound();
		ctxt .SetTheResultToCString(MCnotfoundstring);
		return;
	}
	MCdefaultstackptr->find(ctxt, (Find_mode)p_mode, p_needle, p_target);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecChooseTool(MCExecContext& ctxt, MCStringRef p_input, int p_tool)
{
	MCU_choose_tool(ctxt, p_input, (Tool)p_tool);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecGo(MCExecContext& ctxt, MCCard *p_card, MCStringRef p_window, int p_mode, bool p_this_stack, bool p_visible)
{
	if (p_card == nil)
    {
        if (MCresult -> isclear())
            ctxt . SetTheResultToStaticCString("No such card");
		return;
    }
    
    MCStack *t_stack;
    t_stack = p_card -> getstack();
    
	MCRectangle rel;
	MCStack *parentptr;

	if (p_mode == WM_PULLDOWN || p_mode == WM_POPUP || p_mode == WM_OPTION)
	{
		MCButton *bptr = (MCButton *)ctxt . GetObject();
		if (ctxt . GetObject()->gettype() == CT_BUTTON && bptr->attachmenu(t_stack))
			rel = MCU_recttoroot(bptr->getstack(), bptr->getrect());
		else
		{
			ctxt . LegacyThrow(EE_GO_CANTATTACH);
			return;
		}
	}
	else
	{
		// MW-2011-02-27: [[ Bug ]] Make sure that if we open as a sheet, we have a parent pointer!
		if (ctxt . GetObject()->getstack()->getopened() || MCtopstackptr == NULL)
			parentptr = ctxt . GetObject() -> getstack();
		else
			parentptr = MCtopstackptr;

		rel = parentptr -> getrect();
	}

	t_stack->stopedit();

	Window_mode wm = (Window_mode)p_mode;
	if (wm == WM_LAST && t_stack->userlevel() != 0 && p_window == nil && !p_this_stack)
		wm = (Window_mode)(t_stack->userlevel() + WM_TOP_LEVEL_LOCKED);

	uint2 oldw = t_stack->getrect().width;
	uint2 oldh = t_stack->getrect().height;

	// Here 'oldstack' is the pointer to the stack's window we are taking over.
	// If it turns out NULL then we aren't subverting another stacks' window to
	// our cause :o)
	MCStack *oldstack = NULL;
	if (p_window != nil || p_this_stack)
	{
		Window w = DNULL;
		if (p_this_stack)
		{
			oldstack = MCdefaultstackptr;
			w = oldstack->getwindow();
		}
		else
		{
			uint4 win;
			if (MCU_stoui4(p_window, win) && MCscreen->uint4towindow(win, w))
				oldstack = MCdispatcher->findstackd(w);
			else
				oldstack = ctxt . GetObject()->getstack()->findstackname_oldstring(MCStringGetOldString(p_window));
		}
		
		if (oldstack == NULL || !oldstack->getopened())
		{
			ctxt . LegacyThrow(EE_GO_BADWINDOWEXP);
			return;
		}
		
		if (oldstack == t_stack)
			oldstack = NULL;
		else
		{
			// MW-2011-10-01: [[ Effects ]] Snapshot the old stack window.
			if (!MCRedrawIsScreenLocked() && MCcur_effects != NULL)
				oldstack -> snapshotwindow(oldstack -> getcurcard() -> getrect());
			
			// MW-2011-10-01: [[ Redraw ]] Lock the screen until we are done.
			MCRedrawLockScreen();
			
			// MW-2012-09-19: [[ Bug 10383 ]] Use the 'real' mode - otherwise we get one
			//   modified for ICONIC or CLOSED states which screw things up a bit!
			wm = oldstack->getrealmode();
			if (wm == WM_MODAL || wm == WM_SHEET)
			{
				MCRedrawUnlockScreen();
				ctxt . LegacyThrow(EE_GO_BADWINDOWEXP);
				return;
			}
			oldstack->kunfocus();
			t_stack->close();
			
			MCPlayer *tptr = MCplayers;
			while (tptr != NULL)
			{
				MCPlayer *oldptr = tptr;
				tptr = tptr->getnextplayer();
				if (oldptr->getstack() == oldstack)
					oldptr->close();
			}

			if (!t_stack->takewindow(oldstack))
			{
				MCRedrawUnlockScreen();
				ctxt . LegacyThrow(EE_GO_BADWINDOWEXP);
				return;
			}
		}
	}
	else if (p_mode != WM_LAST && wm >= WM_MODELESS)
	{
		// MW-2011-08-18: [[ Redraw ]] Move to use redraw lock/unlock.
		MCRedrawForceUnlockScreen();
	}

	Boolean oldtrace = MCtrace;
	MCtrace = False;

	// MW-2007-02-11: [[ Bug 4029 ]] - 'go invisible' fails to close stack window if window already open
	if (!p_visible && t_stack -> getflag(F_VISIBLE))
	{
		if (t_stack -> getwindow() != NULL)
			MCscreen -> closewindow(t_stack -> getwindow());
		t_stack->setflag(False, F_VISIBLE);
	}

	// MW-2011-02-27: [[ Bug ]] Make sure that if we open as a sheet, we have a parent pointer!
	if (wm != WM_SHEET && wm != WM_DRAWER)
		parentptr = nil;

	Exec_stat stat = ES_NORMAL;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt . GetEP();
		added = True;
	}

#ifdef _MOBILE
	// MW-2011-01-30: [[ Effects ]] On Mobile, we must twiddle with snapshots to
	//   ensure go stack with visual effect works.
	if (oldstack == nil && MCcur_effects != nil && MCdefaultstackptr != t_stack)
	{
		MCdefaultstackptr -> snapshotwindow(MCdefaultstackptr -> getcurcard() -> getrect());
		t_stack -> takewindowsnapshot(MCdefaultstackptr);
		MCRedrawLockScreen();
	}
#endif	
	
	if (t_stack->setcard(p_card, True, True) == ES_ERROR
	        || t_stack->openrect(rel, wm, parentptr, WP_DEFAULT, OP_NONE) == ES_ERROR)
	{
		MCtrace = oldtrace;
		stat = ES_ERROR;
	}
	
	if (oldstack != NULL)
	{
		MCRectangle trect = t_stack->getcurcard()->getrect();
		t_stack->getcurcard()->message_with_args(MCM_resize_stack, trect.width, trect.height, oldw, oldh);
		
		MCRedrawUnlockScreen();
		
		if (MCcur_effects != nil)
		{
			Boolean t_abort;
			t_stack -> effectrect(t_stack -> getcurcard() -> getrect(), t_abort);
		}
		
		Boolean oldlock = MClockmessages;
		MClockmessages = True;
		oldstack->close();
		MClockmessages = oldlock;
		t_stack->kfocus();
	}
	
#ifdef _MOBILE
	// MW-2011-01-30: [[ Effects ]] Apply any stack level visual efect.
	if (oldstack == nil && MCcur_effects != nil && MCdefaultstackptr != t_stack)
	{
		MCRedrawUnlockScreen();
		
		// MW-2011-10-17: [[ Bug 9811 ]] Make sure we configure the new card now.
		MCRedrawDisableScreenUpdates();
		t_stack -> configure(True);
		MCRedrawEnableScreenUpdates();
			
		Boolean t_abort;
		t_stack -> effectrect(t_stack -> getcurcard() -> getrect(), t_abort);
	}
#endif	

	if (added)
		MCnexecutioncontexts--;
	
	MCtrace = oldtrace;
	if (t_stack->getmode() == WM_TOP_LEVEL || t_stack->getmode() == WM_TOP_LEVEL_LOCKED)
		MCdefaultstackptr = t_stack;
	if (MCmousestackptr != NULL)
		MCmousestackptr->resetcursor(True);
	if (MCabortscript)
		stat = ES_ERROR;
	if (stat == ES_ERROR)
		ctxt . Throw();
}

void MCInterfaceExecGoCardAsMode(MCExecContext& ctxt, MCCard *p_card, int p_mode, bool p_visible, bool p_this_stack)
{
	MCInterfaceExecGo(ctxt, p_card, nil, p_mode, p_this_stack, p_visible);
}

void MCInterfaceExecGoCardInWindow(MCExecContext& ctxt, MCCard *p_card, MCStringRef p_window, bool p_visible, bool p_this_stack)
{
	MCInterfaceExecGo(ctxt, p_card, p_window, WM_MODELESS, p_this_stack, p_visible);
}

void MCInterfaceExecGoRecentCard(MCExecContext& ctxt)
{
	MCrecent->gorel(-1);
}

void MCInterfaceExecGoCardRelative(MCExecContext& ctxt, bool p_forward, real8 p_amount)
{
	int2 i = (int2) (p_forward ? p_amount : -p_amount);
	MCrecent->gorel(i);
}

void MCInterfaceExecGoCardEnd(MCExecContext& ctxt, bool p_is_start)
{
	MCrecent->godirect(p_is_start);
}

void MCInterfaceExecGoHome(MCExecContext& ctxt, MCCard *p_card)
{
	if (p_card -> getstack() != MCdefaultstackptr)
	{
		MCdefaultstackptr->close();
		MCdefaultstackptr->checkdestroy();
	}
	MCInterfaceExecGo(ctxt, p_card, nil, 0, false, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecVisualEffect(MCExecContext& ctxt, MCInterfaceVisualEffect p_effect)
{
	MCEffectList *effectptr = MCcur_effects;
	if (effectptr == nil)
		MCcur_effects = effectptr = new MCEffectList;
	else
	{
		while (effectptr->next != NULL)
			effectptr = effectptr->next;
		effectptr->next = new MCEffectList;
		effectptr = effectptr->next;
	}
	
	effectptr -> type = p_effect . type;
	effectptr -> direction = p_effect . direction;
	effectptr -> speed = p_effect . speed;
	effectptr -> image = p_effect . image;
	effectptr -> name = strclone(MCStringGetCString(p_effect . name));
	if (MCStringGetLength(p_effect . sound) == 0)
		effectptr -> sound = NULL;
	else
		effectptr -> sound = strclone(MCStringGetCString(p_effect . sound));
	
	MCEffectArgument *t_arguments = nil;
	for (uindex_t i = 0; i < p_effect . nargs; i++)
	{
		MCInterfaceVisualEffectArgument t_arg = p_effect . arguments[i];
		MCEffectArgument *t_kv;
		t_kv = new MCEffectArgument;
		t_kv -> next = t_arguments;
		t_kv -> key = strclone(MCStringGetCString(t_arg . key));
		t_kv -> value = strclone(MCStringGetCString(t_arg . value));
		t_arguments = t_kv;
		t_arg = p_effect . arguments[i+1];
	}

	effectptr -> arguments = t_arguments;
}