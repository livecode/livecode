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
#include "widget.h"
#include "osspec.h"
#include "variable.h"

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
#include "mcerror.h"

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

void MCInterfaceVisualEffectArgumentCopy(MCExecContext& ctxt, MCInterfaceVisualEffectArgument p_source, MCInterfaceVisualEffectArgument& r_target)
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
MCExecCustomTypeInfo *kMCInterfaceImagePaletteSettingsTypeInfo = &_kMCInterfaceImagePaletteSettingsTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceVisualEffectTypeInfo = &_kMCInterfaceVisualEffectTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceVisualEffectArgumentTypeInfo = &_kMCInterfaceVisualEffectArgumentTypeInfo;
//////////

bool MCInterfaceTryToResolveObject(MCExecContext& ctxt, MCStringRef long_id, MCObjectPtr& r_object)
{
	bool t_found;
	t_found = false;
	
	MCChunk *tchunk = new (nothrow) MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(long_id);
	if (tchunk->parse(sp, False) == PS_NORMAL)
	{
		if (tchunk->getobj(ctxt, r_object, True))
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
		r_settings . optimal . palette_size = *count;
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
    // AL-2014-08-14: [[ Bug 13176 ]] Use p_value for argument value
	if (p_has_id)
	{
		if (!MCStringFormat(r_arg . value, "id %@", p_value))
		{
			ctxt . Throw();
			return;
		}
	}
	else
		r_arg . value = (MCStringRef)MCValueRetain(p_value);

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
    
    if (t_displays)
    {
        x = t_displays->viewport.x + (t_displays->viewport.width >> 1);
        y = t_displays->viewport.y + (t_displays->viewport.height >> 1);
    }
    else
    {
        // No-UI mode
        x = y = 0;
    }
    
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
	if (!MCclickfield)
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
	if (!MCclickfield)
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
	if (!MCclickfield)
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
	if (!MCclickfield)
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
	if (!MCclickfield)
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
	if (!MCclickfield)
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
	if (!MCclickstackptr)
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
    
	// IM-2013-10-10: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_mouseloc;
	t_mouseloc = MCdefaultstackptr->globaltostackloc(MCPointMake(x, y));
    r_value = t_mouseloc . x;
}

void MCInterfaceEvalMouseV(MCExecContext& ctxt, integer_t& r_value)
{
	int16_t x, y;
	MCscreen->querymouse(x, y);
    
	// IM-2013-10-10: [[ FullscreenMode ]] Update to use stack coord conversion methods
	MCPoint t_mouseloc;
	t_mouseloc = MCdefaultstackptr->globaltostackloc(MCPointMake(x, y));
    r_value = t_mouseloc . y;
}

void MCInterfaceEvalMouseLoc(MCExecContext& ctxt, MCStringRef& r_string)
{
	int16_t x, y;
	MCscreen->querymouse(x, y); 
    MCPoint t_mouse_loc;
    t_mouse_loc = MCdefaultstackptr -> globaltostackloc(MCPointMake(x, y));
    if (MCStringFormat(r_string, "%d,%d", t_mouse_loc. x, t_mouse_loc . y))
        return;
    ctxt . Throw();
}

//////////

void MCInterfaceEvalMouseChar(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (MCmousestackptr)
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
	if (MCmousestackptr)
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
	if (MCmousestackptr)
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
	if (MCmousestackptr)
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
	if (MCmousestackptr)
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
	if (MCmousestackptr)
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
	if (!MCmousestackptr)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	MCmousestackptr -> getstringprop(ctxt, 0, P_SHORT_NAME, False, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceEvalFoundChunk(MCExecContext& ctxt, MCStringRef& r_string)
{
	if (!MCfoundfield)
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
	if (!MCfoundfield)
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
	if (!MCfoundfield)
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
	if (!MCfoundfield)
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
	if (!MCfoundfield)
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
	if (!MCactivefield)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

    // MW-2013-08-07: [[ Bug 10689 ]] If the parent of the field is a button
    //   then return the chunk of the button, not the embedded field.
    if (MCactivefield -> getparent() -> gettype() == CT_BUTTON)
    {
        if (static_cast<MCButton *>(MCactivefield -> getparent()) -> selectedchunk(r_string))
            return;
    }
    else if (MCactivefield->selectedchunk(r_string))
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
	if (!MCactivefield)
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
	if (!MCactivefield)
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
	if (!MCactivefield)
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
	if (!MCactivefield)
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
	if (!MCactiveimage)
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

void MCInterfaceEvalEventCapsLockKey(MCExecContext& ctxt, MCNameRef& r_result)
{
    r_result = MCInterfaceKeyConditionToName((MCmodifierstate & MS_CAPS_LOCK) != 0);
    MCValueRetain(r_result);
}

void MCInterfaceEvalEventCommandKey(MCExecContext& ctxt, MCNameRef& r_result)
{
    r_result = MCInterfaceKeyConditionToName((MCmodifierstate & MS_CONTROL) != 0);
    MCValueRetain(r_result);
}

void MCInterfaceEvalEventControlKey(MCExecContext& ctxt, MCNameRef& r_result)
{
    r_result = MCInterfaceKeyConditionToName((MCmodifierstate & MS_MAC_CONTROL) != 0);
    MCValueRetain(r_result);
}

void MCInterfaceEvalEventOptionKey(MCExecContext& ctxt, MCNameRef& r_result)
{
    r_result = MCInterfaceKeyConditionToName((MCmodifierstate & MS_ALT) != 0);
    MCValueRetain(r_result);
}

void MCInterfaceEvalEventShiftKey(MCExecContext& ctxt, MCNameRef& r_result)
{
    r_result = MCInterfaceKeyConditionToName((MCmodifierstate & MS_SHIFT) != 0);
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
	Boolean t_abort = false;
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
	if (!MCfocusedstackptr)
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
		if (MCNameIsEqualToCaseless(p_name, enames[i]))
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
	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	r_global_point = MCdefaultstackptr->stacktogloballoc(p_point);
}

void MCInterfaceEvalLocalLoc(MCExecContext& ctxt, MCPoint p_point, MCPoint& r_local_point)
{
	// IM-2013-10-09: [[ FullscreenMode ]] Update to use stack coord conversion methods
	r_local_point = MCdefaultstackptr->globaltostackloc(p_point);
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
    MCObjectPtr t_object;
	MCerrorlock++;
    p_object->getoptionalobj(ctxt, t_object, True);
    r_exists = (t_object . object == nil ? false : true);
    ctxt . IgnoreLastError();
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
	if (MCfocusedstackptr && MCfocusedstackptr -> getcard() != NULL)
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
			MCtemplateaudio = new (nothrow) MCAudioClip;
			break;
		case RT_TEMPLATE_BUTTON:
			delete MCtemplatebutton;
			MCtemplatebutton = new (nothrow) MCButton;
			break;
		case RT_TEMPLATE_CARD:
			delete MCtemplatecard;
			MCtemplatecard = new (nothrow) MCCard;
			break;
		case RT_TEMPLATE_EPS:
			delete MCtemplateeps;
			MCtemplateeps = new (nothrow) MCEPS;
			break;
		case RT_TEMPLATE_FIELD:
			delete MCtemplatefield;
			MCtemplatefield = new (nothrow) MCField;
			break;
		case RT_TEMPLATE_GRAPHIC:
			delete MCtemplategraphic;
			MCtemplategraphic = new (nothrow) MCGraphic;
			break;
		case RT_TEMPLATE_GROUP:
			delete MCtemplategroup;
			MCtemplategroup = new (nothrow) MCGroup;
			break;
		case RT_TEMPLATE_IMAGE:
			delete MCtemplateimage;
			MCtemplateimage = new (nothrow) MCImage;
			break;
		case RT_TEMPLATE_SCROLLBAR:
			delete MCtemplatescrollbar;
			MCtemplatescrollbar = new (nothrow) MCScrollbar;
			break;
		case RT_TEMPLATE_PLAYER:
			delete MCtemplateplayer;
			MCtemplateplayer = new (nothrow) MCPlayer;
			break;
		case RT_TEMPLATE_STACK:
			delete MCtemplatestack;
			/* UNCHECKED */ MCStackSecurityCreateStack(MCtemplatestack);
			break;
		case RT_TEMPLATE_VIDEO_CLIP:
			delete MCtemplatevideo;
			MCtemplatevideo = new (nothrow) MCVideoClip;
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
    MCPoint t_location;
	t_stack = MCscreen -> getstackatpoint(p_location . x, p_location . y);
    
    // IM-2013-10-11: [[ FullscreenMode ]] Update to use stack coord conversion methods
    if (t_stack != nil)
        t_location = t_stack->globaltostackloc(p_location);

	// If the location is not over a stack, then return empty.    
	if (t_stack == nil)
	{
		r_control = MCValueRetain(kMCEmptyString);
		return;
	}

    // We now have a stack and a location in card co-ords so let's do the hittest.
	MCObject *t_object;
    // SN-2014-08-27: [[ Bug 13288 ]] t_location should be used instead of p_location
	t_object = MCInterfaceEvalControlAtLocInStack(t_stack, t_location);

	MCAutoValueRef t_control;
	if (t_object -> names(P_LONG_ID, &t_control))
		if (ctxt.ConvertToString(*t_control, r_control))
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
		MCdefaultstackptr->mup(p_which, false);
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
		int2 t_oldx = x;
		int2 t_oldy = y;
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
		if (x != t_oldx || y != t_oldy)
			MCdefaultstackptr->mfocus(x, y);
	}
	MCdefaultstackptr->mup(p_which, false);
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
    // AL-2014-01-07 need to lock mods here to ensure MCmodifierstate determines the modifiers
    MCscreen -> setlockmods(True);
    
	MCdefaultstackptr->kfocus();
	uint2 i;
	real8 nexttime = MCS_time();

	for (i = 0 ; i < MCStringGetLength(p_typing); i++)
	{
		// Fetch the codepoint at the given codeunit index
		codepoint_t t_cp_char =
			MCStringGetCodepointAtIndex(p_typing, i);
		
		// Compute the number of codeunits used by the char
		uindex_t t_cp_length =
			MCUnicodeCodepointGetCodeunitLength(t_cp_char);
		
		KeySym keysym = t_cp_char;
        MCAutoStringRef t_char;
		if (keysym < 0x20 || keysym == 0xFF)
		{
			if (keysym == 0x0A)
				keysym = 0x0D;
			keysym |= 0xFF00;
		}
		else
        {
            /* If the character is in the BMP *and* it has a native mapping then
             * we use the mapped native char as the keycode. This makes things
             * consistent with normal keyboard entry. Any non-native unicode char
             * will pass through with a keycode with the XK_Class_codepoint bit
             * set. */
            unichar_t t_bmp_codepoint = t_cp_char & 0xFFFF;
            char_t t_native_char = 0;
            if (t_cp_char <= 0xFFFF &&
                MCUnicodeMapToNative(&t_bmp_codepoint, 1, t_native_char))
            {
                keysym = t_native_char;
            }
            else if (keysym > 0x7F)
                keysym |= XK_Class_codepoint;
		
            if (!MCStringCopySubstring(p_typing, MCRangeMake(i, t_cp_length), &t_char))
            {
                ctxt.Throw();
                break;
            }
        }
        // PM-2014-10-03: [[ Bug 13907 ]] Make sure we don't pass nil to kdown
		if (*t_char == nil)
			t_char = kMCEmptyString;
		
		MCdefaultstackptr->kdown(*t_char, keysym);
		MCdefaultstackptr->kup(*t_char, keysym);
		nexttime += (real8)MCtyperate / 1000.0;
		real8 delay = nexttime - MCS_time();
		if (MCscreen->wait(delay, False, False))
		{
			ctxt . LegacyThrow(EE_TYPE_ABORT);
			break;
		}
		
		// If the codepoint was in SMP, then make sure we bump two codeunit
		// indicies.
		i += t_cp_length - 1;
	}
    
    // AL-2014-01-07 return lock mods to false
    MCscreen -> setlockmods(False);
	MCmodifierstate = oldstate;
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
	MCAutoValueRef t_element;
	if (cptr -> names(P_LONG_ID, &t_element))
		if (ctxt.ConvertToString(*t_element, r_element))
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
	        || !MCdefaultstackptr->haswindow())
	{
		ctxt . LegacyThrow(EE_CLICK_STACKNOTOPEN);
		return;
	}
    
    // IM-2013-09-23: [[ FullscreenMode ]] get / set mouseloc & clickloc in view coords
	MCPoint t_view_clickloc;
    // IM-2014-01-06: [[ Bug 11624 ]] Use MCStack::stacktowindowloc to account for stack scroll
	t_view_clickloc = MCdefaultstackptr->stacktowindowloc(p_location);
    
	uint2 oldmstate = MCmodifierstate;
	uint2 oldbstate = MCbuttonstate;
    
    MCStack *t_old_mousestack;
	MCPoint t_old_mouseloc;
    
	MCscreen->getmouseloc(t_old_mousestack, t_old_mouseloc);
	MCscreen->setmouseloc(MCdefaultstackptr, t_view_clickloc);
    
    // AL-2014-01-07 need to lock mods here to ensure MCmodifierstate determines the modifiers
    MCscreen -> setlockmods(True);
    
	MCmodifierstate = p_modifiers;
	MCbuttonstate |= 0x1L << (p_button - 1);
    
	MCdispatcher->wmfocus_stack(MCdefaultstackptr, t_view_clickloc.x, t_view_clickloc.y);
	MCmodifierstate = p_modifiers;
	MCbuttonstate |= 0x1L << (p_button - 1);
	MCdispatcher->wmdown_stack(MCdefaultstackptr, p_button);

	if (MCmousestackptr)
		MCscreen->sync(MCmousestackptr->getw());
    
	Boolean abort = MCscreen->wait(CLICK_INTERVAL, False, False);
    
	MCscreen->setclickloc(MCdefaultstackptr, t_view_clickloc);
    
	MCmodifierstate = p_modifiers;
	MCbuttonstate &= ~(0x1L << (p_button - 1));
	MCdispatcher->wmup_stack(MCdefaultstackptr, p_button);
	MCmodifierstate = oldmstate;
	MCbuttonstate = oldbstate;
	MCControl *mfocused = MCdefaultstackptr->getcard()->getmfocused();
	if (mfocused != NULL
	    && ((mfocused->gettype() == CT_GRAPHIC
	         && mfocused->getstate(CS_CREATE_POINTS))
	        || (mfocused->gettype() == CT_IMAGE && mfocused->getstate(CS_DRAW)
	            && MCdefaultstackptr->gettool(mfocused) == T_POLYGON)))
		mfocused->doubleup(1); // cancel polygon create
	if (t_old_mousestack == NULL || t_old_mousestack->getmode() != 0)
	{
		MCscreen->setmouseloc(t_old_mousestack, t_old_mouseloc);
		if (t_old_mousestack != NULL)
			MCdispatcher->wmfocus_stack(t_old_mousestack, t_old_mouseloc.x, t_old_mouseloc.y);
	}
    // AL-2014-01-07 return lock mods to false
    MCscreen -> setlockmods(False);
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
	if (MCtopstackptr)
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

static void MCInterfaceRevertStack(MCExecContext& ctxt, MCStack *p_stack)
{
    MCAssert(p_stack != nil);
    
    Window_mode oldmode = p_stack->getmode();
    MCRectangle oldrect = p_stack->getrect();
    
    if (!MCdispatcher->ismainstack(p_stack))
        p_stack = (MCStack *)p_stack->getparent();
    if (p_stack == MCdispatcher->gethome())
    {
        ctxt . LegacyThrow(EE_REVERT_HOME);
        return;
    }
    
    MCAutoStringRef t_filename;
    p_stack -> getstringprop(ctxt, 0, P_FILE_NAME, False, &t_filename);
    
    MCNewAutoNameRef t_name;
    if (!MCNameCreate(*t_filename, &t_name))
        return;
    
    // we don't want to check flags on stack revert
    if (p_stack->del(false))
    {
        p_stack -> scheduledelete();
        p_stack = MCdispatcher->findstackname(*t_name);
        if (p_stack != NULL)
            p_stack->openrect(oldrect, oldmode, NULL, WP_DEFAULT, OP_NONE);
    }
    else
        ctxt . Throw();
}

void MCInterfaceExecRevert(MCExecContext& ctxt)
{
    MCInterfaceRevertStack(ctxt, MCtopstackptr);
}

void MCInterfaceExecRevertStack(MCExecContext& ctxt, MCObject *p_stack)
{
    if (p_stack == nil || p_stack->gettype() != CT_STACK)
    {
        ctxt . LegacyThrow(EE_REVERT_NOSTACK);
        return;
    }

    MCInterfaceRevertStack(ctxt, (MCStack *)p_stack);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecGroupControls(MCExecContext& ctxt, MCObjectPtr *p_controls, uindex_t p_control_count)
{
    if (p_control_count == 0)
        return;
    
    // MW-2013-06-20: [[ Bug 10863 ]] Make sure all objects have this parent, after
    //   the first object has been resolved.
    MCObject *t_required_parent;
    t_required_parent = nil;

    MCCard *t_card = nil;
    MCControl *controls = nil;
    MCObject *t_this_parent = nil;
    MCControl *cptr = nil;
    
    uindex_t i;
    for (i = 0; i < p_control_count; ++i)
    {
        t_this_parent = (p_controls[i] . object) -> getparent();
        if (t_this_parent == nil || t_this_parent -> gettype() != CT_CARD)
        {
            ctxt . LegacyThrow(EE_GROUP_NOTGROUPABLE);
            return;
        }
        
        cptr = (MCControl *)p_controls[i] . object;
        // MW-2011-01-21: Make sure we don't try and group shared groups
        if (cptr -> gettype() == CT_GROUP && static_cast<MCGroup *>(cptr) -> isshared())
        {
            ctxt . LegacyThrow(EE_GROUP_NOBG);
            return;
        }
        
        // MW-2013-06-20: [[ Bug 10863 ]] Take the parent of the first object for
        //   future comparisons.
        if (t_required_parent == nil)
            t_required_parent = t_this_parent;
        
        // MERG-2013-05-07: [[ Bug 10863 ]] Make sure all objects have the same
        //   parent.
        if (t_this_parent != t_required_parent)
        {
            ctxt . LegacyThrow(EE_GROUP_DIFFERENTPARENT);
            return;
        }
    }
    
    // If we made it this far, the controls are ok to group.
    for (i = 0; i < p_control_count; ++i)
    {
        cptr = (MCControl *)p_controls[i] . object;
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
	
	MCAutoValueRef t_id;
	gptr -> names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}

void MCInterfaceExecGroupSelection(MCExecContext& ctxt)
{
	MCGroup *t_group = nil;
	if (MCselected->group(ctxt.GetLine(), ctxt.GetPos(), t_group) != ES_NORMAL)
	{
		ctxt.Throw();
		return;
	}
	
	if (t_group != nil)
	{
		MCAutoValueRef t_id;
		t_group -> names(P_LONG_ID, &t_id);
		ctxt.SetItToValue(*t_id);
	}
	else
	{
		ctxt.SetItToEmpty();
	}
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
				t_aclip = new (nothrow) MCAudioClip(*static_cast<MCAudioClip *>(t_object));

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
				t_aclip = new (nothrow) MCVideoClip(*static_cast<MCVideoClip *>(t_object));

			t_new_object = t_aclip;
			p_dst . object -> getstack() -> appendvclip(t_aclip);
		}
		break;

		case CT_CARD:
		{
			if (!p_cut)
			{
                MCStackHandle t_old_defaultstack = MCdefaultstackptr;
                MCdefaultstackptr = static_cast<MCStack *>(p_dst . object);
				MCdefaultstackptr -> stopedit();

				MCCard *t_card;
				t_card = static_cast<MCCard *>(t_object);

				t_new_object = t_card -> clone(True, True);

                if (t_old_defaultstack.IsValid())
                    MCdefaultstackptr = t_old_defaultstack;

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
        case CT_WIDGET:
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

                // SN-2014-12-08: [[ Bug 12726 ]] Avoid to dereference a nil pointer (and fall back
                //  to the default stack pointer if needed).
				MCControl *t_new_control;
				t_new_control = static_cast<MCControl *>(t_new_object);
				if (t_old_parent == NULL)
                    t_new_control -> resetfontindex(MCdefaultstackptr);
                else if (p_dst . object -> getstack() != t_old_parent -> getstack())
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
		MCAutoValueRef t_id;
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
	if (MCactivefield)
		MCactivefield->deleteselection(False);
	else if (MCactiveimage)
		MCactiveimage->delimage();	
	else if (!MCselected->del() && !MCeerror->isempty())
        ctxt . Throw();
}

void MCInterfaceExecDeleteObjects(MCExecContext& ctxt, MCObjectPtr *p_objects, uindex_t p_object_count)
{
	for(uindex_t i = 0; i < p_object_count; i++)
	{
		if (!p_objects[i] . object -> del(true))
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
			/* UNCHECKED */ MCStringRemove(t_value, MCRangeMakeMinMax(p_chunks[i] . mark . start, p_chunks[i] . mark . finish));
			/* UNCHECKED */ MCStringCopyAndRelease(t_value, t_value);
			p_chunks[i] . object -> setstringprop(ctxt, p_chunks[i] . part_id, P_TEXT, False, t_value);
			MCValueRelease(t_value);
		}
		else if (p_chunks[i] . object -> gettype() == CT_FIELD)
        {
            MCField *t_field;
            t_field = static_cast<MCField *>(p_chunks[i] . object);
			t_field -> settextindex(p_chunks[i] . part_id, p_chunks[i] . mark . start, p_chunks[i] . mark . finish, kMCEmptyString, False);
        }
	}
}

////////////////////////////////////////////////////////////////////////////////

static void MCInterfaceExecChangeChunkOfButton(MCExecContext& ctxt, MCObjectChunkPtr p_target, Properties p_prop, bool p_value)
{
	MCStringRef t_value;
	p_target . object -> getstringprop(ctxt, p_target . part_id, P_TEXT, False, t_value);

	/* UNCHECKED */ MCStringMutableCopyAndRelease(t_value, t_value);

	int4 start;
	start = p_target . mark . start;

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
	if (MCactivefield)
	{
		MCactivefield->unselect(False, True);
		if (MCactivefield)
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

// AL-2014-08-04: [[ Bug 13079 ]] Implement 'select text of button'
void MCInterfaceExecSelectAllTextOfButton(MCExecContext& ctxt, MCObjectPtr p_target)
{
	if (!p_target . object -> getopened() && p_target . object -> getid())
	{
		ctxt . LegacyThrow(EE_CHUNK_NOTOPEN);
		return;
	}
    
    MCButton *bptr = static_cast<MCButton *>(p_target . object);
    
    if (bptr -> getentry() != nil)
    {
        p_target . object = bptr -> getentry();
        MCInterfaceExecSelectAllTextOfField(ctxt, p_target);
    }
    else if (!MCStringIsEmpty(bptr -> getmenustring()))
        bptr -> setmenuhistory(1);
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
	default:
		MCUnreachable();
		break;
	}
    
	static_cast<MCField *>(p_target . object) -> seltext(t_start, t_finish, True);
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
		t_lines = MCStringCountChar(*t_text, MCRangeMake(0, p_target . mark . start), '\n', kMCStringOptionCompareExact);
		
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

void
MCInterfaceExecSaveStackWithVersion(MCExecContext & ctxt,
                                    MCStack *p_target,
                                    MCStringRef p_version)
{
	MCInterfaceExecSaveStackAsWithVersion(ctxt, p_target, kMCEmptyString, p_version);
}

void
MCInterfaceExecSaveStackWithNewestVersion(MCExecContext & ctxt,
                                          MCStack *p_target)
{
	MCInterfaceExecSaveStackAsWithNewestVersion(ctxt, p_target, kMCEmptyString);
}

void MCInterfaceExecSaveStackAs(MCExecContext& ctxt, MCStack *p_target, MCStringRef p_new_filename)
{
	ctxt . SetTheResultToEmpty();
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;
	
	p_target -> saveas(p_new_filename, MCstackfileversion);
}

void
MCInterfaceExecSaveStackAsWithVersion(MCExecContext & ctxt,
                                      MCStack *p_target,
                                      MCStringRef p_new_filename,
                                      MCStringRef p_version)
{
	ctxt.SetTheResultToEmpty();
	if (!ctxt.EnsureDiskAccessIsAllowed())
		return;

	MCInterfaceStackFileVersion t_version;
	MCInterfaceStackFileVersionParse(ctxt, p_version, t_version);
	if (ctxt.HasError())
		return;

	p_target->saveas(p_new_filename, t_version.version);
}

void
MCInterfaceExecSaveStackAsWithNewestVersion(MCExecContext & ctxt,
                                            MCStack * p_target,
                                            MCStringRef p_new_filename)
{
	ctxt.SetTheResultToEmpty();
	if (!ctxt.EnsureDiskAccessIsAllowed())
		return;

	p_target->saveas(p_new_filename);
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

	MCPoint *t_motion = new (nothrow) MCPoint[p_motion_count];
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
        p_effect->exec_ctxt(ctxt);
		if (ctxt . GetExecStat() != ES_NORMAL)
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
        ctxt . IgnoreLastError();
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

    p_effect->exec_ctxt(ctxt);
	if (ctxt . GetExecStat() != ES_NORMAL)
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

void MCInterfaceExecPopupWidget(MCExecContext &ctxt, MCNameRef p_kind, MCPoint *p_at, MCArrayRef p_properties)
{
	extern bool MCWidgetPopupAtLocationWithProperties(MCNameRef p_kind, const MCPoint &p_at, MCArrayRef p_properties, MCValueRef &r_result);
	
	MCPoint t_at;
	if (p_at != nil)
    {
        if (!MCtargetptr)
        {
            ctxt . LegacyThrow(EE_SUBWINDOW_NOSTACK);
            return;
        }
        
		t_at = MCtargetptr->getstack()->stacktogloballoc(*p_at);
    }
	else
    {
        if (!MCmousestackptr)
        {
            ctxt . LegacyThrow(EE_SUBWINDOW_NOSTACK);
            return;
        }
        
		t_at = MCmousestackptr->stacktogloballoc(MCPointMake(MCmousex, MCmousey));
    }
	
	MCAutoValueRef t_result;
	if (!MCWidgetPopupAtLocationWithProperties(p_kind, t_at, p_properties, &t_result) || MCValueIsEmpty(*t_result))
	{
		if (MCErrorIsPending())
			MCExtensionCatchError(ctxt);
		
		ctxt.SetTheResultToCString(MCcancelstring);
		ctxt.SetItToEmpty();
	}
	else
	{
		ctxt.SetTheResultToEmpty();
		ctxt.SetItToValue(*t_result);
	}
}

void MCInterfaceExecPopupButton(MCExecContext& ctxt, MCButton *p_target, MCPoint *p_at)
{
	if (!MCmousestackptr)
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
		// IM-2015-03-10: [[ Bug 14851 ]] Send mouseup release for each depressed button.
		uint16_t t_state;
		t_state = MCbuttonstate;
		
		uint16_t t_which;
		t_which = 1;
		
		while (t_state)
		{
			if ((t_state & 0x1) && MCtargetptr)
				MCtargetptr -> mup(t_which, true);
			t_state >>= 1;
			t_which += 1;
		}
		
		p_target->openmenu(True);
	}
}

void MCInterfaceExecSubwindow(MCExecContext& ctxt, MCStack *p_target, MCStack *p_parent, MCRectangle p_rect, int p_at, int p_aligned, int p_mode)
{
	if (p_mode != WM_PULLDOWN && p_mode != WM_POPUP && p_mode != WM_OPTION)
    	MCU_watchcursor(ctxt . GetObject()->getstack(), False);
        
	// MW-2007-05-01: Reverting this as it causes problems :o(
	//stackptr -> setflag(True, F_VISIBLE);
    MCStackHandle t_old_defaultstack = MCdefaultstackptr;
    Boolean oldtrace = MCtrace;
	MCtrace = False;
	if (p_mode >= WM_MODELESS)
		MCRedrawForceUnlockScreen();
    
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
		added = True;
	}
    
	if (p_target->openrect(p_rect, (Window_mode)p_mode, p_parent, (Window_position)p_at, (Object_pos)p_aligned) != ES_NORMAL)
    {
        ctxt.Throw();
    }

	if (MCwatchcursor)
	{
		MCwatchcursor = False;
		p_target->resetcursor(True);
		if (MCmousestackptr && !MCmousestackptr.IsBoundTo(p_target))
			MCmousestackptr->resetcursor(True);
	}
    
	if (added)
		MCnexecutioncontexts--;
    
	MCtrace = oldtrace;
    
	if (p_mode > WM_TOP_LEVEL && t_old_defaultstack.IsValid())
		MCdefaultstackptr = t_old_defaultstack;
}

void MCInterfaceExecDrawerOrSheetStack(MCExecContext& ctxt, MCStack *p_target, MCNameRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned, int p_mode)
{
	MCStack *parentptr;
    parentptr = nil;
    
    if (p_parent_name != nil)
    {
        parentptr = ctxt . GetObject()->getstack()->findstackname(p_parent_name);
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
            // AL-2014-11-24: [[ Bug 14076 ]] Don't override window mode with WM_DRAWER
			MCInterfaceExecSubwindow(ctxt, p_target, parentptr, parentptr->getrect(), p_at, p_aligned, p_mode);
	}
	else if (MCdefaultstackptr->getopened() || !MCtopstackptr)
		MCInterfaceExecSubwindow(ctxt, p_target, MCdefaultstackptr, MCdefaultstackptr->getrect(), p_at, p_aligned, p_mode);
	else
		MCInterfaceExecSubwindow(ctxt, p_target, MCtopstackptr, MCtopstackptr->getrect(), p_at, p_aligned, p_mode);
}

void MCInterfaceExecDrawerOrSheetStackByName(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname(p_name);

	if (sptr == nil)
	{
		if (MCresult -> isclear())
			ctxt. SetTheResultToStaticCString("can't find stack");
		return;
	}
	
	MCInterfaceExecDrawerOrSheetStack(ctxt, sptr, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, p_mode);
}

void MCInterfaceExecDrawerStack(MCExecContext& ctxt, MCStack *p_target, MCNameRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned)
{	
	MCInterfaceExecDrawerOrSheetStack(ctxt, p_target, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, WM_DRAWER);
}

void MCInterfaceExecDrawerStackByName(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_parent_name, bool p_parent_is_thisstack, int p_at, int p_aligned)
{	
	MCInterfaceExecDrawerOrSheetStackByName(ctxt, p_name, p_parent_name, p_parent_is_thisstack, p_at, p_aligned, WM_DRAWER);
}

void MCInterfaceExecDrawerStackLegacy(MCExecContext& ctxt, MCStack *p_target, MCNameRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned)
{
	MCInterfaceExecDrawerStack(ctxt, p_target, parent, p_parent_is_thisstack, (int)p_at, (int)p_aligned);
}

void MCInterfaceExecDrawerStackByNameLegacy(MCExecContext& ctxt, MCNameRef p_name, MCNameRef parent, bool p_parent_is_thisstack, intenum_t p_at, intenum_t p_aligned)
{
	MCInterfaceExecDrawerStackByName(ctxt, p_name, parent, p_parent_is_thisstack, (int)p_at, (int)p_aligned);
}

void MCInterfaceExecSheetStack(MCExecContext& ctxt, MCStack *p_target, MCNameRef p_parent_name, bool p_parent_is_thisstack)
{
	MCInterfaceExecDrawerOrSheetStack(ctxt, p_target, p_parent_name, p_parent_is_thisstack, WP_DEFAULT, OP_CENTER, WM_SHEET);
}

void MCInterfaceExecSheetStackByName(MCExecContext& ctxt, MCNameRef p_name, MCNameRef p_parent_name, bool p_parent_is_thisstack)
{
	MCInterfaceExecDrawerOrSheetStackByName(ctxt, p_name, p_parent_name, p_parent_is_thisstack, WP_DEFAULT, OP_CENTER, WM_SHEET);
}

static MCStack* open_stack_relative_to(MCStack *p_target)
{
    if (MCdefaultstackptr->getopened() && MCdefaultstackptr->isvisible())
        return MCdefaultstackptr;
    else if (MCtopstackptr && MCtopstackptr->isvisible())
        return MCtopstackptr;
    else
        return p_target;
}

void MCInterfaceExecOpenStack(MCExecContext& ctxt, MCStack *p_target, int p_mode)
{
    MCStack* t_stack = open_stack_relative_to(p_target);
    MCInterfaceExecSubwindow(ctxt, p_target, nil, t_stack->getrect(), WP_DEFAULT, OP_NONE, p_mode);
}

void MCInterfaceExecOpenStackByName(MCExecContext& ctxt, MCNameRef p_name, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname(p_name);

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
    if (!MCtargetptr)
    {
        ctxt . LegacyThrow(EE_NOTARGET);
        return;
    }

	// MW-2007-04-10: [[ Bug 4260 ]] We shouldn't attempt to attach a menu to a control that is descendent of itself
	if (MCtargetptr -> getstack() == p_target)
	{
		ctxt . LegacyThrow(EE_SUBWINDOW_BADEXP);
		return;
	}

	if (MCtargetptr -> attachmenu(p_target))
	{
		if (p_mode == WM_POPUP && p_at != nil)
		{
			MCmousex = p_at -> x;
			MCmousey = p_at -> y;
		}
		MCRectangle t_rect;
		t_rect = MCU_recttoroot(MCtargetptr -> getstack(), MCtargetptr -> getrect());
		MCInterfaceExecSubwindow(ctxt, p_target, nil, t_rect, WP_DEFAULT, OP_NONE, p_mode);
		if (!MCabortscript)
			return;

		ctxt . Throw();	
	}
}

void MCInterfaceExecPopupStackByName(MCExecContext& ctxt, MCNameRef p_name, MCPoint *p_at, int p_mode)
{
	MCStack *sptr;
	sptr = ctxt . GetObject()->getstack()->findstackname(p_name);

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
	MCStackHandle t_old_defaultstack = MCdefaultstackptr;
	Boolean wasvisible = MCtemplatestack->isvisible();

	/* Check that a specified parent stack has a usable name before
	 * doing anything with side-effects. */
	MCAutoValueRef t_object_name;
	if (!p_with_group && p_object != nil)
	{
		if (!p_object->names(P_NAME, &t_object_name))
		{
			ctxt.Throw();
			return;
		}
	}

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
		MCdefaultstackptr->setvariantprop(ctxt, 0, P_MAIN_STACK, False, *t_object_name);
		if (ctxt . HasError())
		{
			delete MCdefaultstackptr;
			ctxt . LegacyThrow(EE_CREATE_BADBGORCARD);
			return;
		}
	}

	MCtemplatestack->setflag(wasvisible, F_VISIBLE);
	MCObject *t_object = MCdefaultstackptr;
    
    if (t_old_defaultstack.IsValid())
        MCdefaultstackptr = t_old_defaultstack;
	
	if (p_new_name != nil)
		t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
	
	MCAutoValueRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}

void MCInterfaceExecCreateScriptOnlyStack(MCExecContext& ctxt, MCStringRef p_new_name)
{
    MCStack *t_new_stack;
    MCStackSecurityCreateStack(t_new_stack);
    MCdispatcher -> appendstack(t_new_stack);
    t_new_stack -> setparent(MCdispatcher -> gethome());
    t_new_stack -> message(MCM_new_stack);
    t_new_stack -> setflag(False, F_VISIBLE);
    t_new_stack -> setasscriptonly(kMCEmptyString);
    
	if (p_new_name != nil)
		t_new_stack -> setstringprop(ctxt, 0, P_NAME, False, p_new_name);
	
    // PM-2015-10-26: [[ Bug 16283 ]] Automatically update project browser to show newly created script only stacks
    t_new_stack -> open();
    
    MCAutoValueRef t_id;
	t_new_stack -> names(P_LONG_ID, &t_id);
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


void MCInterfaceExecCreateCard(MCExecContext& ctxt, MCStringRef p_new_name, MCStack *p_parent, bool p_force_invisible)
{
    if (p_parent == nullptr)
    {
        p_parent = MCdefaultstackptr;
    }
    
    if (p_parent->islocked())
    {
    		ctxt . LegacyThrow(EE_CREATE_LOCKED);
    		return;
    }
    
    p_parent->stopedit();
    MCObject *t_object = MCtemplatecard->clone(True, False,p_parent);
    
    if (p_new_name != nil)
    {
     	t_object->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
    }
    
    MCAutoValueRef t_id;
    t_object->names(P_LONG_ID, &t_id);
    ctxt . SetItToValue(*t_id);
}

MCControl* MCInterfaceExecCreateControlGetObject(MCExecContext& ctxt, int p_type, MCObject *&r_parent)
{
	switch (p_type)
	{
	case CT_BACKGROUND:
	case CT_GROUP:
		return MCtemplategroup;
	case CT_BUTTON:
		return MCtemplatebutton;
	case CT_MENU:
		r_parent = MCmenubar ? MCmenubar : MCdefaultmenubar;
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

void MCInterfaceExecCreateControl(MCExecContext& ctxt, MCStringRef p_new_name, int p_type, MCObject *p_container, bool p_force_invisible)
{
    
    MCStack *t_current_stack = p_container == nullptr ? MCdefaultstackptr : p_container->getstack();
    
    if (t_current_stack->islocked())
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

    // AL-2015-06-30: [[ Bug 15556 ]] Ensure mouse focus is synced after creating object
    t_object -> sync_mfocus(false, true);
    
	MCAutoValueRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);
}

void MCInterfaceExecCreateWidget(MCExecContext& ctxt, MCStringRef p_new_name, MCNameRef p_kind, MCObject* p_container, bool p_force_invisible)
{
    
    MCStack *t_current_stack = p_container == nullptr ? MCdefaultstackptr : p_container->getstack();
    
    if (t_current_stack->islocked())
    {
        ctxt . LegacyThrow(EE_CREATE_LOCKED);
        return;
    }
    
    MCWidget* t_widget = new (nothrow) MCWidget();
    if (t_widget == NULL)
        return;
    t_widget -> bind(p_kind, nil);
    if (p_force_invisible)
        t_widget->setflag(!p_force_invisible, F_VISIBLE);
    
    // AL-2015-05-21: [[ Bug 15405 ]] Honour specified parent container when creating widget
    if (p_container == nil)
        t_widget->setparent(MCdefaultstackptr->getcard());
    else
        t_widget -> setparent(p_container);
    
    t_widget->attach(OP_CENTER, false);
    
    if (p_new_name != nil)
        t_widget->setstringprop(ctxt, 0, P_NAME, False, p_new_name);
    
    // AL-2015-06-30: [[ Bug 15556 ]] Ensure mouse focus is synced after creating object
    t_widget -> sync_mfocus(false, true);
    
    MCAutoValueRef t_id;
    t_widget->names(P_LONG_ID, &t_id);
    ctxt . SetItToValue(*t_id);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecClone(MCExecContext& ctxt, MCObject *p_target, MCStringRef p_new_name, bool p_force_invisible)
{
    MCStackHandle t_old_defaultstack = MCdefaultstackptr;
    
	MCObject *t_object = nil;
	switch (p_target->gettype())
	{
	case CT_STACK:
		{
			MCStack *t_stack = (MCStack *)p_target;
			t_object = t_stack->clone();
			if (p_new_name == nil)
			{
				MCAutoValueRef t_short_name;
				MCAutoStringRef t_short_name_str;
				t_stack->names(P_SHORT_NAME, &t_short_name);
				/* UNCHECKED */ ctxt.ConvertToString(*t_short_name, &t_short_name_str);
				MCAutoStringRef t_new_name;
				MCStringMutableCopy(*t_short_name_str, &t_new_name);
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
    case CT_WIDGET:
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
	
	MCAutoValueRef t_id;
	t_object->names(P_LONG_ID, &t_id);
	ctxt . SetItToValue(*t_id);

    if (t_old_defaultstack.IsValid())
        MCdefaultstackptr = t_old_defaultstack;
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecPutIntoField(MCExecContext& ctxt, MCStringRef p_string, int p_where, MCObjectChunkPtr p_chunk)
{
	if (p_chunk . chunk == CT_UNDEFINED && p_where == PT_INTO)
	{
		p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, p_string);
        return;
	}
 
    // we have a field chunk
    MCField *t_field;
    t_field = static_cast<MCField *>(p_chunk . object);
    
    // If we forced new delimiters, then reset the text of the field
    // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
    if (p_chunk . mark . changed != 0)
    {
        findex_t t_added_start;
        t_added_start = p_chunk.mark.start - p_chunk.mark.changed;
        
        MCAutoStringRef t_string;
        // SN-2015-05-05: [[ Bug 15315 ]] Changing the whole text of a field
        //  will delete all the settings of the field, so we only append the
        //  chars which were added (similar to MCExecResolveCharsOfField).
        if (!MCStringCopySubstring((MCStringRef)p_chunk . mark . text,
                                   MCRangeMake(t_added_start, p_chunk.mark.changed),
                                   &t_string))
            return;
        
        // The insertion position of the added chunk delimiters is at
        // the position prior to the added chunk adjustment
        t_field -> settextindex(p_chunk .part_id, t_added_start,
                                t_added_start, *t_string, False);
    }
     
    integer_t t_start, t_finish;
    if (p_where == PT_INTO)
        t_start = p_chunk . mark . start, t_finish = p_chunk . mark . finish;
    else if (p_where == PT_AFTER)
        t_start = t_finish = p_chunk . mark . finish;
    else /* PT_BEFORE */
        t_start = t_finish = p_chunk . mark . start;
    
    // otherwise just alter the contents of the marked range
    if (t_field -> settextindex(p_chunk . part_id, t_start, t_finish, p_string, False) != ES_NORMAL)
    {
        ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
        return;
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
		integer_t t_start, t_finish;
		if (p_where == PT_INTO)
			t_start = p_chunk . mark . start, t_finish = p_chunk . mark . finish;
		else if (p_where == PT_AFTER)
			t_start = t_finish = p_chunk . mark . finish;
		else /* PT_BEFORE */
			t_start = t_finish = p_chunk . mark . start;
		
        // AL-2014-09-10: [[ Bug 13388 ]] If the chunk type is undefined, we need to get the text of the object here.
        MCAutoStringRef t_string, t_text;
        if (p_chunk . chunk == CT_UNDEFINED)
            p_chunk . object -> getstringprop(ctxt, p_chunk . part_id, P_TEXT, False, &t_text);
        else
            t_text = (MCStringRef)p_chunk . mark . text;

        if (ctxt . HasError() || !MCStringMutableCopy(*t_text, &t_string))
            return;
        
        /* UNCHECKED */ MCStringReplace(*t_string, MCRangeMakeMinMax(t_start, t_finish), p_string);
        
        p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, *t_string);
	}
}

void MCInterfaceExecPutIntoObject(MCExecContext& ctxt, MCExecValue p_value, int p_where, MCObjectChunkPtr p_chunk)
{
	if (p_where == PT_INTO && p_chunk . chunk == CT_UNDEFINED)
	{
		p_chunk . object -> setprop(ctxt, p_chunk . part_id, P_TEXT, nil, False, p_value);
	}
	else
	{
		integer_t t_start, t_finish;
		if (p_where == PT_INTO)
			t_start = p_chunk . mark . start, t_finish = p_chunk . mark . finish;
		else if (p_where == PT_AFTER)
			t_start = t_finish = p_chunk . mark . finish;
		else /* PT_BEFORE */
			t_start = t_finish = p_chunk . mark . start;
		

        // AL-2014-09-10: [[ Bug 13388 ]] If the chunk type is undefined, we need to get the text of the object here.
        MCAutoStringRef t_string, t_text;
        if (p_chunk . chunk == CT_UNDEFINED)
            p_chunk . object -> getstringprop(ctxt, p_chunk . part_id, P_TEXT, False, &t_text);
        else
            t_text = (MCStringRef)p_chunk . mark . text;
        
        if (ctxt . HasError() || !MCStringMutableCopy(*t_text, &t_string))
            return;
        
        MCAutoStringRef t_string_value;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_string_value));
        
        if (ctxt . HasError())
            return;
        
        /* UNCHECKED */ MCStringReplace(*t_string, MCRangeMakeMinMax(t_start, t_finish), *t_string_value);
        
        p_chunk . object -> setstringprop(ctxt, p_chunk . part_id, P_TEXT, False, *t_string);
	}
    
    p_chunk.object->signallisteners(P_TEXT);
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
	MCscreen->setlockmoves(True);
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
            // AL-2014-03-27: [[ Bug 12038 ]] Actually set the effect rect.
			MCcur_effects_rect = *p_region;
		
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
	MCscreen->setlockmoves(False);
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
    p_effect -> exec_ctxt(ctxt);
	if (ctxt . GetExecStat() != ES_NORMAL)
	{
		ctxt . LegacyThrow(EE_UNLOCK_BADEFFECT);
		return;
	}
	
	MCRedrawUnlockScreenWithEffects();
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecImportSnapshot(MCExecContext& ctxt, MCStringRef p_display, MCRectangle *p_region, uint4 p_window, MCPoint *p_size)
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
    
	MCImageBitmap *t_bitmap = nil;
	t_bitmap = MCscreen->snapshot(t_rect, p_window, p_display, p_size);
	
	if (t_bitmap != nil)
	{
        MCImageBitmapCheckTransparency(t_bitmap);
        
		MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
        if (iptr != nil)
        {
            // IM-2013-08-01: [[ ResIndependence ]] pass image scale when setting bitmap
            iptr->setbitmap(t_bitmap, 1.0f, true);
            iptr->attach(OP_CENTER, false);
            MCAutoValueRef t_id;
            iptr -> names(P_LONG_ID, &t_id);
            ctxt . SetItToValue(*t_id);
        }
        
        MCImageFreeBitmap(t_bitmap);
    }
    else
    {
        ctxt . LegacyThrow(EE_SNAPSHOT_FAILED);
        return ;
    }
}
void MCInterfaceExecImportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_size)
{
	MCInterfaceExecImportSnapshot(ctxt, nil, p_region, 0, p_size);
}

void MCInterfaceExecImportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_size)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_IMPORT_BADNAME);
	else
		MCInterfaceExecImportSnapshot(ctxt, p_display, p_region, t_window, p_size);
}
void MCInterfaceExecImportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size)
{
	if (MCdefaultstackptr->islocked())
	{	
		ctxt . LegacyThrow(EE_IMPORT_LOCKED);
		return;
	}
    
	MCImageBitmap *t_bitmap = nil;
	t_bitmap = p_target -> snapshot(p_region, p_at_size, 1.0f, p_with_effects);
	
	// OK-2007-04-24: If the import rect doesn't intersect with the object, MCobject::snapshot
	// may return null. In this case, return an error.
	if (t_bitmap == nil)
	{
		ctxt . LegacyThrow(EE_IMPORT_EMPTYRECT);
		return;
	}
	
    // MW-2013-05-20: [[ Bug 10897 ]] Object snapshot returns a premultipled
	//   bitmap, which needs to be processed before compression.
    
    MCImageBitmapUnpremultiply(t_bitmap);
    MCImageBitmapCheckTransparency(t_bitmap);
    
    MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
    
    if (iptr != nil)
    {
        // IM-2013-08-01: [[ ResIndependence ]] pass image scale when setting bitmap
        iptr->setbitmap(t_bitmap, 1.0f, true);
        iptr->attach(OP_CENTER, false);
        MCAutoValueRef t_id;
        iptr -> names(P_LONG_ID, &t_id);
        ctxt . SetItToValue(*t_id);
    }
    
    MCImageFreeBitmap(t_bitmap);
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

	r_stream = MCS_open(p_filename, kMCOpenFileModeRead, True, False, 0);
}

void MCInterfaceExecImportAudioClip(MCExecContext& ctxt, MCStringRef p_filename)
{
	MCU_watchcursor(ctxt . GetObject()->getstack(), True);

	IO_handle t_stream = NULL;
	MCInterfaceExecImportGetStream(ctxt, p_filename, t_stream);

	if (t_stream != NULL)
	{
		MCAudioClip *aptr = new (nothrow) MCAudioClip;
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
		MCVideoClip *vptr = new (nothrow) MCVideoClip;
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

			if (t_image->import(p_filename, t_stream, t_mask_stream) == IO_NORMAL)
			{
				t_image->attach(OP_CENTER, false);
				MCAutoValueRef t_id;
				t_image -> names(P_LONG_ID, &t_id);
				ctxt . SetItToValue(*t_id);
			}
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

void MCInterfaceExecImportObjectFromArray(MCExecContext& ctxt, MCArrayRef p_array, MCObject *p_container)
{
    if ((p_container == nil && MCdefaultstackptr->islocked()) ||
        (p_container != nil && p_container -> getstack() -> islocked()))
    {
        ctxt . LegacyThrow(EE_CREATE_LOCKED);
        return;
    }
    
    MCNewAutoNameRef t_kind;
    MCAutoArrayRef t_state;
    MCValueRef t_value;
    if (!MCArrayFetchValue(p_array, false, MCNAME("$kind"), t_value) ||
        !ctxt . ConvertToName(t_value, &t_kind) ||
        !MCArrayFetchValue(p_array, false, MCNAME("$state"), t_value) ||
        !ctxt . ConvertToArray(t_value, &t_state))
    {
        ctxt . LegacyThrow(EE_IMPORT_NOTANOBJECTARRAY);
        return;
    }
    
    MCWidget *t_widget;
    t_widget = new (nothrow) MCWidget;
    if (t_widget == NULL)
        return;
    
    t_widget -> bind(*t_kind, *t_state);
    
    if (p_container == nil)
        t_widget -> setparent(MCdefaultstackptr -> getcard());
    else
        t_widget -> setparent(p_container);
    
    t_widget -> attach(OP_CENTER, false);
    
    MCAutoValueRef t_id;
    t_widget -> names(P_LONG_ID, &t_id);
    ctxt . SetItToValue(*t_id);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExportBitmap(MCExecContext &ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
    if (p_bitmap == nil)
        return;
    
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
	t_stream = MCS_fakeopenwrite();
    if (t_stream == nil)
        t_success = false;
    if (t_success)
        t_success = MCImageExport(p_bitmap, (Export_format)p_format, t_ps_ptr, p_dither, p_metadata, t_stream, nil);
	
	MCAutoByteArray t_autobuffer;
	void *t_buffer = nil;
	size_t t_size = 0;
    if (t_success &&
        MCS_closetakingbuffer(t_stream, t_buffer, t_size) != IO_NORMAL)
        t_success = false;
    
    if (t_success)
        t_autobuffer.Give((char_t*)t_buffer, t_size);

    if (t_success)
        t_success = t_autobuffer.CreateDataAndRelease(r_data);
    
	if (!t_success)
	{
		ctxt.LegacyThrow(EE_EXPORT_CANTWRITE);
		
		return;
	}
}

void MCInterfaceExportBitmapAndRelease(MCExecContext &ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
    if (p_bitmap != nil)
    {
        MCInterfaceExportBitmap(ctxt, p_bitmap, p_format, p_palette, p_dither, p_metadata, r_data);
        MCImageFreeBitmap(p_bitmap);
    }
}

void MCInterfaceExportBitmapToFile(MCExecContext& ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	if (!ctxt . EnsureDiskAccessIsAllowed())
		return;

	IO_handle t_mstream = nil;
	if (p_mask_filename != nil)
	{
		
		if ((t_mstream = MCS_open(p_mask_filename, kMCOpenFileModeWrite, False, False, 0)) == nil)
		{
			ctxt . LegacyThrow(EE_EXPORT_CANTOPEN);
			return;
		}
	}
	IO_handle t_fstream;
	if ((t_fstream = MCS_open(p_filename, kMCOpenFileModeWrite, False, False, 0)) == nil)
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
	if (!MCImageExport(p_bitmap, (Export_format)p_format, t_ps_ptr, p_dither, p_metadata, t_fstream, t_mstream))
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

void MCInterfaceExportBitmapToFileAndRelease(MCExecContext& ctxt, MCImageBitmap *p_bitmap, int p_format, MCInterfaceImagePaletteSettings *p_palette, bool p_dither, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
    if (p_bitmap != nil)
    {
        MCInterfaceExportBitmapToFile(ctxt, p_bitmap, p_format, p_palette, p_dither, p_metadata, p_filename, p_mask_filename);
        MCImageFreeBitmap(p_bitmap);
    }
}

MCImageBitmap* MCInterfaceGetSnapshotBitmap(MCExecContext &ctxt, MCStringRef p_display, MCRectangle *p_region, uint4 p_window, MCPoint *p_size)
{
	MCRectangle t_rect;
	if (p_region == nil)
	{
		t_rect.x = t_rect.y = -32768;
		t_rect.width = t_rect.height = 0;
	}
	else	
		t_rect = *p_region;
	
	MCImageBitmap *t_bitmap = nil;

	t_bitmap = MCscreen->snapshot(t_rect, p_window, p_display, p_size);
	if (t_bitmap == nil)
	{
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
        return nil;
	}
    
    MCImageBitmapCheckTransparency(t_bitmap);
	return t_bitmap;
}

bool MCInterfaceGetDitherImage(MCImage *p_image)
{
	if (p_image == nil)
		p_image = MCtemplateimage;
	
	return !p_image->getflag(F_DONT_DITHER);
}

void MCInterfaceExecExportSnapshotOfScreen(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, nil, p_region, 0, p_size);
	MCInterfaceExportBitmapAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, r_data);
}

void MCInterfaceExecExportSnapshotOfScreenToFile(MCExecContext& ctxt, MCRectangle *p_region, MCPoint *p_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, nil, p_region, 0, p_size);
	// IM-2014-10-24: [[ Bug 13784 ]] Don't export unless we get a valid bitmap
	if (t_bitmap != nil)
		MCInterfaceExportBitmapToFileAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, p_filename, p_mask_filename);
}

void MCInterfaceExecExportSnapshotOfStack(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
	else
	{
		MCImageBitmap *t_bitmap;
		t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, p_display, p_region, t_window, p_size);
		MCInterfaceExportBitmapAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, r_data);
	}
}

void MCInterfaceExecExportSnapshotOfStackToFile(MCExecContext& ctxt, MCStringRef p_stack, MCStringRef p_display, MCRectangle *p_region, MCPoint *p_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	uint4 t_window;
	if (!MCU_stoui4(p_stack, t_window))
		ctxt . LegacyThrow(EE_EXPORT_NOSELECTED);
	else
	{
		MCImageBitmap *t_bitmap;
		t_bitmap = MCInterfaceGetSnapshotBitmap(ctxt, p_display, p_region, t_window, p_size);
		// IM-2014-10-24: [[ Bug 13784 ]] Don't export unless we get a valid bitmap
		if (t_bitmap != nil)
			MCInterfaceExportBitmapToFileAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, p_filename, p_mask_filename);
	}
}

MCImageBitmap *MCInterfaceGetSnapshotOfObjectBitmap(MCExecContext &ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size)
{
	MCImageBitmap *t_bitmap = nil;
	/* UNCHECKED */ t_bitmap = p_target -> snapshot(p_region, p_at_size, 1.0, p_with_effects);
    
    if (t_bitmap == nil)
    {
        ctxt . LegacyThrow(EE_EXPORT_EMPTYRECT);
        return nil;
    }
    
    // MW-2013-05-20: [[ Bug 10897 ]] The 'snapshot' command produces a premultiplied bitmap
    //   so unpremultiply.
    MCImageBitmapUnpremultiply(t_bitmap);
    MCImageBitmapCheckTransparency(t_bitmap);
    
	return t_bitmap;
}

void MCInterfaceExecExportSnapshotOfObject(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotOfObjectBitmap(ctxt, p_target, p_region, p_with_effects, p_at_size);
    
	MCInterfaceExportBitmapAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, r_data);
}
void MCInterfaceExecExportSnapshotOfObjectToFile(MCExecContext& ctxt, MCObject *p_target, MCRectangle *p_region, bool p_with_effects, MCPoint *p_at_size, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	MCImageBitmap *t_bitmap;
	t_bitmap = MCInterfaceGetSnapshotOfObjectBitmap(ctxt, p_target, p_region, p_with_effects, p_at_size);
    
    // AL-2014-03-20: [[ Bug 11948 ]] t_bitmap nil here causes a crash.
    if (t_bitmap != nil)
        MCInterfaceExportBitmapToFileAndRelease(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(nil), p_metadata, p_filename, p_mask_filename);
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

void MCInterfaceExecExportImage(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCDataRef &r_data)
{
	if (p_target == nil)
		p_target = MCInterfaceExecExportSelectImage(ctxt);
	if (p_target != nil)
	{
		if (p_target->getrect() . width == 0 || p_target -> getrect() . height == 0)
		{
			r_data = MCValueRetain(kMCEmptyData);
			return;
		}
        
        MCImageBitmap *t_bitmap;
		
		// IM-2014-09-02: [[ Bug 13295 ]] Call shorthand version of lockbitmap(),
		// which will copy if necessary.
		// IM-2014-10-24: [[ Bug 13784 ]] Don't export unless we get a valid bitmap
		if (p_target->lockbitmap(t_bitmap, false, true))
		{
			MCInterfaceExportBitmap(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(p_target), p_metadata, r_data);
            p_target->unlockbitmap(t_bitmap);
		}
	}
}
void MCInterfaceExecExportImageToFile(MCExecContext& ctxt, MCImage *p_target, int p_format, MCInterfaceImagePaletteSettings *p_palette, MCImageMetadata* p_metadata, MCStringRef p_filename, MCStringRef p_mask_filename)
{
	if (p_target == nil)
		p_target = MCInterfaceExecExportSelectImage(ctxt);
	if (p_target != nil)
	{
        MCImageBitmap *t_bitmap;
		
		// IM-2014-09-02: [[ Bug 13295 ]] Call shorthand version of lockbitmap(),
		// which will copy if necessary.
		// IM-2014-10-24: [[ Bug 13784 ]] Don't export unless we get a valid bitmap
		if (p_target->lockbitmap(t_bitmap, false, true))
		{
			MCInterfaceExportBitmapToFile(ctxt, t_bitmap, p_format, p_palette, MCInterfaceGetDitherImage(p_target), p_metadata, p_filename, p_mask_filename);
            p_target->unlockbitmap(t_bitmap);
		}
	}
}

void MCInterfaceExecExportObjectToArray(MCExecContext& ctxt, MCObject *p_object, MCArrayRef& r_array)
{
    if (p_object -> gettype() != CT_WIDGET)
    {
        r_array = MCValueRetain(kMCEmptyArray);
        return;
    }
    
    MCWidget *t_widget;
    t_widget = static_cast<MCWidget *>(p_object);
    
    MCNewAutoNameRef t_kind;
    t_widget -> GetKind(ctxt, &t_kind);
    if (ctxt . HasError())
        return;
    
    MCAutoArrayRef t_state;
    t_widget -> GetState(ctxt, &t_state);
    if (ctxt . HasError())
        return;
    
    MCAutoArrayRef t_array;
    if (!MCArrayCreateMutable(&t_array) ||
        !MCArrayStoreValue(*t_array, false, MCNAME("$kind"), *t_kind) ||
        !MCArrayStoreValue(*t_array, false, MCNAME("$state"), *t_state) ||
        !t_array . MakeImmutable())
        return;
    
    r_array = t_array . Take();
}

////////////////////////////////////////////////////////////////////////////////

bool MCInterfaceExecSortContainer(MCExecContext &ctxt, MCStringRef p_data, int p_type, Sort_type p_direction, int p_form, MCExpression *p_by, MCStringRef &r_output)
{
	if (MCStringIsEmpty(p_data))
	{
		MCStringCopy(kMCEmptyString, r_output);
		return true;
	}

	// If sorting items of the container, then we use the current itemDelimiter to split each item,
	// all other forms of search default to the lineDelimiter for now. Note that this is a slight
	// change of behavior as previously sorting containers by line ignored the lineDelimiter and
	// always delimited by ascii 10.
	MCStringRef t_delimiter;
	if (p_type == CT_ITEM)
		t_delimiter = ctxt . GetItemDelimiter();
	else
		t_delimiter = ctxt . GetLineDelimiter();

	if (MCStringIsEqualToCString(t_delimiter, "\0",
	                             kMCStringOptionCompareExact))
		return false;

	extern bool MCStringsSplit(MCStringRef p_string, MCStringRef p_separator, MCStringRef*&r_strings, uindex_t& r_count);
	
    MCAutoStringRefArray t_chunks;
    if (!MCStringsSplit(p_data, t_delimiter, t_chunks . PtrRef(), t_chunks . CountRef()))
        return false;
    
    uindex_t t_item_count;
    t_item_count = t_chunks . Count();
    
	bool t_trailing_delim = false;
	if (p_type != CT_ITEM && MCStringIsEmpty(t_chunks[t_item_count - 1]))
    {
        t_trailing_delim = true;
        t_item_count--;
	}
	
	// MCStringsExecSort allocates memory for the sorted string array,
	// but does not retain the string pointers passed to it. Hence we
	// use an MCAutoArray of MCStringRefs so that only the array is
	// deallocated in the destructor of t_sorted. The strings themselves
	// need to be released in the destructor of t_chunks, hence its use
 	// of an MCAutoStringRefArray.
	MCAutoArray<MCStringRef> t_sorted;
	MCStringsExecSort(ctxt, p_direction, (Sort_type)p_form,
	                  *t_chunks, t_item_count, p_by,
	                  t_sorted . PtrRef(), t_sorted . SizeRef());
	
	// Build the output string
	MCAutoListRef t_list;
	if (!MCListCreateMutable(t_delimiter, &t_list))
		return false;

    uindex_t i;
    for (i = 0; i < t_sorted . Size(); i++)
	{
        if (!MCListAppend(*t_list, t_sorted[i]))
			return false;
	}

    MCAutoStringRef t_list_string;
	if (!MCListCopyAsString(*t_list, &t_list_string))
		return false;
	
    if (t_trailing_delim)
    {
        return MCStringCreateWithStrings(r_output, *t_list_string, t_delimiter);
    }
    
    r_output = MCValueRetain(*t_list_string);
    return true;
}


void MCInterfaceExecSortCardsOfStack(MCExecContext &ctxt, MCStack *p_target, bool p_ascending, int p_format, MCExpression *p_by, bool p_only_marked)
{
	if (p_target == nil)
		p_target = MCdefaultstackptr;

	if (!p_target->sort(ctxt, p_ascending ? ST_ASCENDING : ST_DESCENDING, (Sort_type)p_format, p_by, p_only_marked))
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
        MCValueAssign(x_target, *t_sorted_string);
		return;
	}
	
	ctxt . LegacyThrow(EE_SORT_CANTSORT);
}

void MCInterfaceExecReplaceInField(MCExecContext& ctxt,
								   MCStringRef p_pattern,
								   MCStringRef p_replacement,
								   MCObjectChunkPtr& p_container,
								   bool p_preserve_styles)
{
	// Both these conditions are guaranteed by the caller.
	MCAssert(p_container . object -> gettype() == CT_FIELD);
	MCAssert(p_container . mark . text == nil ||
			 MCValueGetTypeCode(p_container . mark . text) == kMCValueTypeCodeString);
	
	MCField *t_field;
	t_field = static_cast<MCField *>(p_container . object);
	
	// If this was a whole field ref (e.g. field 1) then the text field will
	// be nil. Thus we must fetch it here.
	// Note: If present, the text will the entire text of the container, and
	// the range to act on should be taken as [start,finish).
	MCAutoStringRef t_text;
	if (p_container . mark . text != nil)
		t_text = (MCStringRef)p_container . mark . text;
	else
	{
		t_field -> getstringprop(ctxt,
								 p_container . part_id,
								 P_TEXT,
								 false,
								 &t_text);
		if (ctxt . HasError())
			return;
	}
	
	MCStringOptions t_options;
	t_options = ctxt.GetStringComparisonType();
	
	// The indicies in the field will drift away from the original mark as
	// we replace text - this is the delta we need to apply.
	findex_t t_delta;
	t_delta = 0;
	
	// Start with the specified range in the marked text.
	MCRange t_range;
	t_range = MCRangeMake(p_container . mark . start,
						  p_container . mark . finish);
	for(;;)
	{
		// Find the next occurrance of pattern in text - we are done if not
		// found.
		MCRange t_found_range;
		if (!MCStringFind(*t_text,
						  t_range,
						  p_pattern,
						  t_options,
						  &t_found_range))
			break;
		
		// The range in the field we must replace is t_found_range + start.
		t_field -> settextindex(p_container . part_id,
								(findex_t)t_found_range . offset + t_delta,
								(findex_t)(t_found_range . offset + t_found_range . length) + t_delta,
								p_replacement,
								False,
								p_preserve_styles ? kMCFieldStylingFromAfter : kMCFieldStylingNone);
		
		// Update the field index delta.
		t_delta += MCStringGetLength(p_replacement) - t_found_range . length;
		
		// Update the range we want to consider in the source text.
		t_range = MCRangeMakeMinMax(t_found_range . offset + t_found_range . length,
									p_container . mark . finish);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecFind(MCExecContext& ctxt, int p_mode, MCStringRef p_needle, MCChunk *p_target)
{
	if (MCStringGetLength(p_needle) == 0)
	{
		if (MCfoundfield)
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

void MCInterfaceExecGo(MCExecContext& ctxt, MCCard *p_card, MCStringRef p_window, int p_mode, bool p_this_stack, MCInterfaceExecGoVisibility p_visibility_type)
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
	MCStack *parentptr = nullptr;

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
		if (ctxt . GetObject()->getstack()->getopened())
			parentptr = ctxt . GetObject() -> getstack();
		else
			parentptr = open_stack_relative_to(t_stack);

		rel = parentptr -> getrect();
        
    }

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
		Window w = NULL;
		if (p_this_stack)
		{
			oldstack = MCdefaultstackptr;
			w = oldstack->getwindow();
		}
		else
		{
			uint4 win;
            // SN-2015-01-07: [[ iOS-64bit ]] Update to uinttowindow
            if (MCU_stoui4(p_window, win) && MCscreen->uinttowindow(win, w))
				oldstack = MCdispatcher->findstackd(w);
			else
				oldstack = ctxt . GetObject()->getstack()->findstackname_string(p_window);
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
			
            MCPlayer::ClosePlayers(oldstack);

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
    
    // Need a parentptr that is an open stack
    if (parentptr == t_stack && (wm == WM_SHEET || wm == WM_DRAWER))
    {
        ctxt . LegacyThrow(EE_GO_BADWINDOWEXP);
        return;
    }

	Boolean oldtrace = MCtrace;
	MCtrace = False;

	// MW-2007-02-11: [[ Bug 4029 ]] - 'go invisible' fails to close stack window if window already open
	if (p_visibility_type != kMCInterfaceExecGoVisibilityImplicit)
	{
		if (t_stack -> getwindow() != NULL)
			MCscreen -> closewindow(t_stack -> getwindow());
		if (p_visibility_type == kMCInterfaceExecGoVisibilityExplicitVisible)
			t_stack->setflag(true, F_VISIBLE);
		if (p_visibility_type == kMCInterfaceExecGoVisibilityExplicitInvisible)
			t_stack->setflag(false, F_VISIBLE);
	}

	// MW-2011-02-27: [[ Bug ]] Make sure that if we open as a sheet, we have a parent pointer!
	if (wm != WM_SHEET && wm != WM_DRAWER)
		parentptr = nil;

	Exec_stat stat = ES_NORMAL;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
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
		t_stack -> view_configure(True);
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
	if (MCmousestackptr)
		MCmousestackptr->resetcursor(True);
	if (MCabortscript)
		stat = ES_ERROR;
	if (stat == ES_ERROR)
		ctxt . Throw();
}

void MCInterfaceExecGoCardAsMode(MCExecContext& ctxt, MCCard *p_card, int p_mode, MCInterfaceExecGoVisibility p_visibility_type, bool p_this_stack)
{
	MCInterfaceExecGo(ctxt, p_card, nil, p_mode, p_this_stack, p_visibility_type);
}

void MCInterfaceExecGoCardInWindow(MCExecContext& ctxt, MCCard *p_card, MCStringRef p_window, MCInterfaceExecGoVisibility p_visibility_type, bool p_this_stack)
{
	MCInterfaceExecGo(ctxt, p_card, p_window, WM_MODELESS, p_this_stack, p_visibility_type);
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
	MCInterfaceExecGo(ctxt, p_card, nil, 0, false, kMCInterfaceExecGoVisibilityImplicit);
}

////////////////////////////////////////////////////////////////////////////////

void MCInterfaceExecVisualEffect(MCExecContext& ctxt, MCInterfaceVisualEffect p_effect)
{
	MCEffectList *effectptr = MCcur_effects;
	if (effectptr == nil)
		MCcur_effects = effectptr = new (nothrow) MCEffectList;
	else
	{
		while (effectptr->next != NULL)
			effectptr = effectptr->next;
		effectptr->next = new (nothrow) MCEffectList;
		effectptr = effectptr->next;
	}
	
	effectptr -> type = p_effect . type;
	effectptr -> direction = p_effect . direction;
	effectptr -> speed = p_effect . speed;
	effectptr -> image = p_effect . image;
	effectptr -> name = MCValueRetain(p_effect . name);
	if (MCStringGetLength(p_effect . sound) == 0)
		effectptr -> sound = NULL;
	else
		effectptr -> sound = MCValueRetain(p_effect . sound);
	
	MCEffectArgument *t_arguments = nil;
    MCInterfaceVisualEffectArgument t_arg;
	for (uindex_t i = 0; i < p_effect . nargs; i++)
	{
        t_arg = p_effect . arguments[i];
		MCEffectArgument *t_kv;
		t_kv = new (nothrow) MCEffectArgument;
		t_kv -> next = t_arguments;
		t_kv -> key = MCValueRetain(t_arg . key);
		t_kv -> value = MCValueRetain(t_arg . value);
		t_arguments = t_kv;
	}

	effectptr -> arguments = t_arguments;
}
