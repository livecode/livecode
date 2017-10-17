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


#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "image.h"
#include "cdata.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "hndlrlst.h"
#include "handler.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "param.h"
#include "font.h"
#include "util.h"
#include "debug.h"
#include "aclip.h"
#include "vclip.h"
#include "field.h"
#include "chunk.h"
#include "objectstream.h"
#include "parentscript.h"
#include "bitmapeffect.h"
#include "objectpropsets.h"
#include "exec.h"
#include "flst.h"

#include "globals.h"
#include "mctheme.h"
#include "redraw.h"

#include "license.h"
#include "context.h"
#include "mode.h"

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCObject::kProperties[] =
{
	DEFINE_RW_OBJ_PROPERTY(P_ID, UInt32, MCObject, Id)
	DEFINE_RW_OBJ_PROPERTY(P_SHORT_ID, UInt32, MCObject, Id)
	DEFINE_RO_OBJ_PROPERTY(P_ABBREV_ID, String, MCObject, AbbrevId)
	DEFINE_RO_OBJ_PART_PROPERTY(P_LONG_ID, String, MCObject, LongId)
	DEFINE_RW_OBJ_PROPERTY(P_NAME, String, MCObject, Name)
    // SN-2014-08-25: [[ Bug 13276 ]] Added the property definition for 'abbreviated name'
    DEFINE_RO_OBJ_PROPERTY(P_ABBREV_NAME, String, MCObject, AbbrevName)
	DEFINE_RO_OBJ_PROPERTY(P_SHORT_NAME, String, MCObject, ShortName)
	DEFINE_RO_OBJ_PART_PROPERTY(P_LONG_NAME, String, MCObject, LongName)
	DEFINE_RW_OBJ_PROPERTY(P_ALT_ID, UInt32, MCObject, AltId)
	DEFINE_RW_OBJ_PART_CUSTOM_PROPERTY(P_LAYER, InterfaceLayer, MCObject, Layer)
	DEFINE_RW_OBJ_PROPERTY(P_SCRIPT, String, MCObject, Script)
	DEFINE_RW_OBJ_PROPERTY(P_PARENT_SCRIPT, OptionalString, MCObject, ParentScript)
	DEFINE_RO_OBJ_PART_PROPERTY(P_NUMBER, UInt32, MCObject, Number)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FORE_PIXEL, OptionalUInt32, MCObject, ForePixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FORE_PIXEL, UInt32, MCObject, ForePixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BACK_PIXEL, OptionalUInt32, MCObject, BackPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BACK_PIXEL, UInt32, MCObject, BackPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HILITE_PIXEL, OptionalUInt32, MCObject, HilitePixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_HILITE_PIXEL, UInt32, MCObject, HilitePixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BORDER_PIXEL, OptionalUInt32, MCObject, BorderPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BORDER_PIXEL, UInt32, MCObject, BorderPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_PIXEL, OptionalUInt32, MCObject, TopPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_PIXEL, UInt32, MCObject, TopPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_PIXEL, OptionalUInt32, MCObject, BottomPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_PIXEL, UInt32, MCObject, BottomPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHADOW_PIXEL, OptionalUInt32, MCObject, ShadowPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHADOW_PIXEL, UInt32, MCObject, ShadowPixel)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FOCUS_PIXEL, OptionalUInt32, MCObject, FocusPixel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FOCUS_PIXEL, UInt32, MCObject, FocusPixel)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_BACK_COLOR, Any, MCObject, PenBackColor)
	DEFINE_RW_OBJ_PROPERTY(P_BRUSH_BACK_COLOR, Any, MCObject, PenBackColor)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_PATTERN, OptionalUInt32, MCObject, PenPattern)
	DEFINE_RW_OBJ_PROPERTY(P_BRUSH_PATTERN, OptionalUInt32, MCObject, PenPattern)

	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_PEN_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_PEN_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BRUSH_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BRUSH_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_FORE_COLOR, InterfaceNamedColor, MCObject, ForeColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BACK_COLOR, InterfaceNamedColor, MCObject, BackColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, MCObject, HiliteColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_HILITE_COLOR, InterfaceNamedColor, MCObject, HiliteColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCObject, BorderColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BORDER_COLOR, InterfaceNamedColor, MCObject, BorderColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_TOP_COLOR, InterfaceNamedColor, MCObject, TopColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_TOP_COLOR, InterfaceNamedColor, MCObject, TopColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_BOTTOM_COLOR, InterfaceNamedColor, MCObject, BottomColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_BOTTOM_COLOR, InterfaceNamedColor, MCObject, BottomColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_SHADOW_COLOR, InterfaceNamedColor, MCObject, ShadowColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_SHADOW_COLOR, InterfaceNamedColor, MCObject, ShadowColor)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_FOCUS_COLOR, InterfaceNamedColor, MCObject, FocusColor)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_FOCUS_COLOR, InterfaceNamedColor, MCObject, FocusColor)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FORE_PATTERN, OptionalUInt32, MCObject, ForePattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FORE_PATTERN, OptionalUInt32, MCObject, ForePattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BACK_PATTERN, OptionalUInt32, MCObject, BackPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BACK_PATTERN, OptionalUInt32, MCObject, BackPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HILITE_PATTERN, OptionalUInt32, MCObject, HilitePattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_HILITE_PATTERN, OptionalUInt32, MCObject, HilitePattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BORDER_PATTERN, OptionalUInt32, MCObject, BorderPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BORDER_PATTERN, OptionalUInt32, MCObject, BorderPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_PATTERN, OptionalUInt32, MCObject, TopPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_PATTERN, OptionalUInt32, MCObject, TopPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_PATTERN, OptionalUInt32, MCObject, BottomPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_PATTERN, OptionalUInt32, MCObject, BottomPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_SHADOW_PATTERN, OptionalUInt32, MCObject, ShadowPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_SHADOW_PATTERN, OptionalUInt32, MCObject, ShadowPattern)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_FOCUS_PATTERN, OptionalUInt32, MCObject, FocusPattern)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_FOCUS_PATTERN, OptionalUInt32, MCObject, FocusPattern)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_COLORS, String, MCObject, Colors)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_COLORS, String, MCObject, Colors)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_PATTERNS, String, MCObject, Patterns)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_PATTERNS, String, MCObject, Patterns)

	DEFINE_RW_OBJ_PROPERTY(P_LOCK_LOCATION, Bool, MCObject, LockLocation)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_HEIGHT, OptionalUInt16, MCObject, TextHeight)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_HEIGHT, UInt16, MCObject, TextHeight)
	DEFINE_RW_OBJ_NON_EFFECTIVE_OPTIONAL_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCObject, TextAlign)
	DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_TEXT_ALIGN, InterfaceTextAlign, MCObject, TextAlign)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_FONT, OptionalString, MCObject, TextFont)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_FONT, String, MCObject, TextFont)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TEXT_SIZE, OptionalUInt16, MCObject, TextSize)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TEXT_SIZE, UInt16, MCObject, TextSize)
	DEFINE_RW_OBJ_NON_EFFECTIVE_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCObject, TextStyle)
	DEFINE_RO_OBJ_EFFECTIVE_CUSTOM_PROPERTY(P_TEXT_STYLE, InterfaceTextStyle, MCObject, TextStyle)

	DEFINE_RW_OBJ_PROPERTY(P_SHOW_BORDER, Bool, MCObject, ShowBorder)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_FOCUS_BORDER, Bool, MCObject, ShowFocusBorder)
	DEFINE_RW_OBJ_PROPERTY(P_BORDER_WIDTH, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_LINE_SIZE, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_WIDTH, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_PEN_HEIGHT, UInt16, MCObject, BorderWidth)
	DEFINE_RW_OBJ_PROPERTY(P_OPAQUE, Bool, MCObject, Opaque)	
	DEFINE_RW_OBJ_PROPERTY(P_FILLED, Bool, MCObject, Opaque)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_SHADOW, InterfaceShadow, MCObject, Shadow)
	DEFINE_RW_OBJ_PROPERTY(P_SHADOW_OFFSET, Int16, MCObject, ShadowOffset)
	DEFINE_RW_OBJ_PROPERTY(P_3D, Bool, MCObject, 3D)

    // MERG-2013-05-01: [[ EffVisible ]] Add 'effective' adjective to
    //   the visible property.
	DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_VISIBLE, Bool, MCObject, Visible)
    DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_VISIBLE, Bool, MCObject, Visible)
	DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_INVISIBLE, Bool, MCObject, Invisible)
    DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_INVISIBLE, Bool, MCObject, Invisible)
	DEFINE_RW_OBJ_PART_PROPERTY(P_ENABLED, Bool, MCObject, Enabled)
	DEFINE_RW_OBJ_PART_PROPERTY(P_DISABLED, Bool, MCObject, Disabled)
	DEFINE_RW_OBJ_PROPERTY(P_SELECTED, Bool, MCObject, Selected)
	DEFINE_RW_OBJ_PROPERTY(P_TRAVERSAL_ON, Bool, MCObject, TraversalOn)

	DEFINE_RO_OBJ_PROPERTY(P_OWNER, OptionalString, MCObject, Owner)
	DEFINE_RO_OBJ_PROPERTY(P_SHORT_OWNER, OptionalString, MCObject, ShortOwner)
	DEFINE_RO_OBJ_PROPERTY(P_ABBREV_OWNER, OptionalString, MCObject, AbbrevOwner)
	DEFINE_RO_OBJ_PART_PROPERTY(P_LONG_OWNER, OptionalString, MCObject, LongOwner)

	DEFINE_RW_OBJ_PART_NON_EFFECTIVE_PROPERTY(P_PROPERTIES, Array, MCObject, Properties)
    // MERG-2013-05-07: [[ RevisedPropsProp ]] Add support for 'the effective
    //   properties of object ...'.
	DEFINE_RO_OBJ_PART_EFFECTIVE_PROPERTY(P_PROPERTIES, Array, MCObject, Properties)
	DEFINE_RW_OBJ_PROPERTY(P_CUSTOM_PROPERTY_SET, String, MCObject, CustomPropertySet)
	DEFINE_RW_OBJ_LIST_PROPERTY(P_CUSTOM_PROPERTY_SETS, LinesOfString, MCObject, CustomPropertySets)

	DEFINE_RW_OBJ_ENUM_PROPERTY(P_INK, InterfaceInkNames, MCObject, Ink)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_CANT_SELECT, Bool, MCObject, CantSelect)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_CANT_SELECT, Bool, MCObject, CantSelect)
	DEFINE_RW_OBJ_PROPERTY(P_BLEND_LEVEL, UInt16, MCObject, BlendLevel)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_LOCATION, Point, MCObject, Location)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_LOCATION, Point, MCObject, Location)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)

	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RW_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)

	DEFINE_RO_OBJ_ENUM_PROPERTY(P_ENCODING, InterfaceEncoding, MCObject, Encoding)

    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_TEXT_STYLE, Bool, MCObject, TextStyleElement)
    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_CUSTOM_KEYS, String, MCObject, CustomKeysElement)
    DEFINE_RW_OBJ_ARRAY_PROPERTY(P_CUSTOM_PROPERTIES, Any, MCObject, CustomPropertiesElement)
    DEFINE_RW_OBJ_PROPERTY(P_CUSTOM_PROPERTIES, Any, MCObject, CustomProperties)

    DEFINE_RW_OBJ_PROPERTY(P_CUSTOM_KEYS, String, MCObject, CustomKeys)
    
    DEFINE_RW_OBJ_NON_EFFECTIVE_ENUM_PROPERTY(P_THEME, InterfaceTheme, MCObject, Theme)
    DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_THEME, InterfaceTheme, MCObject, Theme)
    
    DEFINE_RW_OBJ_NON_EFFECTIVE_ENUM_PROPERTY(P_THEME_CONTROL_TYPE, InterfaceThemeControlType, MCObject, ThemeControlType)
    DEFINE_RO_OBJ_EFFECTIVE_ENUM_PROPERTY(P_THEME_CONTROL_TYPE, InterfaceThemeControlType, MCObject, ThemeControlType)

    DEFINE_RO_OBJ_ENUM_PROPERTY(P_SCRIPT_STATUS, InterfaceScriptStatus, MCObject, ScriptStatus)
    
	DEFINE_RO_OBJ_NON_EFFECTIVE_LIST_PROPERTY(P_REV_AVAILABLE_HANDLERS, LinesOfString, MCObject, RevAvailableHandlers)
	DEFINE_RO_OBJ_EFFECTIVE_LIST_PROPERTY(P_REV_AVAILABLE_HANDLERS, LinesOfString, MCObject, RevAvailableHandlers)
    DEFINE_RO_OBJ_ARRAY_PROPERTY(P_REV_AVAILABLE_VARIABLES, String, MCObject, RevAvailableVariables)
    DEFINE_RO_OBJ_PROPERTY(P_REV_AVAILABLE_VARIABLES, String, MCObject, RevAvailableVariablesNonArray)
    
    DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_REV_SCRIPT_DESCRIPTION, Any, MCObject, RevScriptDescription)
    DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_REV_SCRIPT_DESCRIPTION, Any, MCObject, RevScriptDescription)

    DEFINE_RO_OBJ_PROPERTY(P_REV_BEHAVIOR_USES, Array, MCObject, RevBehaviorUses)

};

MCObjectPropertyTable MCObject::kPropertyTable =
{
	nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCObject::sendgetprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCValueRef& r_value)
{
	// If the set name is nil, then we send a 'getProp <propname>' otherwise we
	// send a 'getProp <setname> <propname>'.
	//
	MCNameRef t_getprop_name;
	MCNameRef t_param_name;
	if (MCNameIsEmpty(p_set_name))
		t_getprop_name = p_prop_name, t_param_name = kMCEmptyName;
	else
		t_getprop_name = p_set_name, t_param_name = p_prop_name;
    
	Exec_stat t_stat = ES_NOT_HANDLED;
	if (!MClockmessages && (ctxt . GetObject() != this || !ctxt . GetHandler() -> hasname(t_getprop_name)))
	{
		MCParameter p1;
		p1.setvalueref_argument(t_param_name);
        
        MCStackHandle t_old_defaultstack = MCdefaultstackptr;
        MCdefaultstackptr = getstack();
        MCObjectPartHandle oldtargetptr(this);
        swap(MCtargetptr, oldtargetptr);
		Boolean added = False;
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
			added = True;
		}
		t_stat = MCU_dofrontscripts(HT_GETPROP, t_getprop_name, &p1);
		if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
        {
            /* If the target object was deleted in the frontscript, prevent
             * normal message dispatch as if the frontscript did not pass the
             * message. */
            MCAssert(!MCtargetptr || MCtargetptr.IsBoundTo(this));
            if (MCtargetptr)
            {
                // MAY-DELETE: Handle the message - this/MCtargetptr might be unbound after
                // this call if it is deleted.
                t_stat = handle(HT_GETPROP, t_getprop_name, &p1, this);
            }
            else
            {
                t_stat = ES_NORMAL;
            }
        }
        
        if (t_old_defaultstack.IsValid())
            MCdefaultstackptr = t_old_defaultstack;

        swap(MCtargetptr, oldtargetptr);
		if (added)
			MCnexecutioncontexts--;
	}
    
    if (t_stat == ES_NORMAL)
        t_stat = MCresult -> eval(ctxt, r_value) ? ES_NORMAL : ES_ERROR;
    
	return t_stat;
}

bool MCObject::getcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCProperListRef p_path, MCExecValue& r_value)
{
    if (p_path != nil)
        return false;
    
	assert(p_set_name != nil);
	assert(p_prop_name != nil);
    
	Exec_stat t_stat;
	t_stat = sendgetprop(ctxt, p_set_name, p_prop_name, r_value . valueref_value);
	if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
	{
        MCValueRef t_value;
		MCObjectPropertySet *p;
		if (!findpropset(p_set_name, false, p) ||
			!p -> fetchelement(ctxt, p_prop_name, t_value))
        {
            r_value . stringref_value = MCValueRetain(kMCEmptyString);
            r_value . type = kMCExecValueTypeStringRef;
            return true;
        }
        r_value . valueref_value = MCValueRetain(t_value);
		t_stat = ES_NORMAL;
	}
    
	if (t_stat == ES_NORMAL)
    {
        r_value . type = kMCExecValueTypeValueRef;
        return true;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCObject::sendsetprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCValueRef p_value)
{
	// If the set name is nil, then we send a 'setProp <propname> <value>'
	// otherwise we send a 'setProp <setname>[<propname>] <value>'.
	MCNameRef t_setprop_name;
	MCNameRef t_param_name;
	if (MCNameIsEmpty(p_set_name))
		t_setprop_name = p_prop_name, t_param_name = kMCEmptyName;
	else
		t_setprop_name = p_set_name, t_param_name = p_prop_name;
    
	// Note that in either case (non-array or array setProp), the param list is
	// the same:
	//   setProp pPropName, pValue
	// The parameter list is auto-adjusted if it is of array type in MCHandler::exec.
    
	Exec_stat t_stat = ES_NOT_HANDLED;
	if (!MClockmessages && (ctxt . GetObject() != this || !ctxt . GetHandler()->hasname(t_setprop_name)))
	{
		MCParameter p1, p2;
		p1.setnext(&p2);
        
		p1.setvalueref_argument(t_param_name);
		p2.setvalueref_argument(p_value);
		
		MCStackHandle t_old_defaultstack = MCdefaultstackptr;
		MCdefaultstackptr = getstack();
        MCObjectPartHandle oldtargetptr(this);
        swap(MCtargetptr, oldtargetptr);
		Boolean added = False;
		if (MCnexecutioncontexts < MAX_CONTEXTS)
		{
			MCexecutioncontexts[MCnexecutioncontexts++] = &ctxt;
			added = True;
		}

		t_stat = MCU_dofrontscripts(HT_SETPROP, t_setprop_name, &p1);
		if (t_stat == ES_NOT_HANDLED || t_stat == ES_PASS)
        {
            /* If the target object was deleted in the frontscript, prevent
             * normal message dispatch as if the frontscript did not pass the
             * message. */
            MCAssert(!MCtargetptr || MCtargetptr.IsBoundTo(this));
            if (MCtargetptr)
            {
                // MAY-DELETE: Handle the message - this/MCtargetptr might be unbound after
                // this call if it is deleted.
                t_stat = handle(HT_SETPROP, t_setprop_name, &p1, this);
            }
            else
            {
                t_stat = ES_NORMAL;
            }
        }
        
		if (added)
			MCnexecutioncontexts--;
        
        if (t_old_defaultstack.IsValid())
            MCdefaultstackptr = t_old_defaultstack;

        swap(MCtargetptr, oldtargetptr);
	}
    
	return t_stat;
}

bool MCObject::setcustomprop(MCExecContext& ctxt, MCNameRef p_set_name, MCNameRef p_prop_name, MCProperListRef p_path, MCExecValue p_value)
{
    if (p_path != nil)
        return false;
    
    MCAutoValueRef t_value;
    MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value , kMCExecValueTypeValueRef, &(&t_value));
    
	Exec_stat t_stat;
	t_stat = sendsetprop(ctxt, p_set_name, p_prop_name, *t_value);
    
	if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
	{
		MCObjectPropertySet *p;
		/* UNCHECKED */ ensurepropset(p_set_name, false, p);
		if (!p -> storeelement(ctxt, p_prop_name, *t_value))
			return false;
		return true;
	}
	return t_stat == ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo *lookup_object_property(const MCObjectPropertyTable *p_table, Properties p_which, bool p_effective, bool p_array_prop, MCPropertyInfoChunkType p_chunk_type)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which && (!p_table -> table[i] . has_effective || p_table -> table[i] . effective == p_effective) &&
            (p_array_prop == p_table -> table[i] . is_array_prop) &&
            (p_chunk_type == p_table -> table[i] . chunk_type))
			return &p_table -> table[i];
	
	if (p_table -> parent != nil)
		return lookup_object_property(p_table -> parent, p_which, p_effective, p_array_prop, p_chunk_type);
	
	return nil;
}

bool MCObject::getprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue& r_value)
{
	bool t_is_array_prop;
	// MW-2011-11-23: [[ Array Chunk Props ]] If index is nil or empty, then its just a normal
	//   prop, else its an array prop.
	t_is_array_prop = (p_index != nil && !MCNameIsEmpty(p_index));
	
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, t_is_array_prop, kMCPropertyInfoChunkTypeNone);
	if (t_info == nil)
		t_info = lookup_object_property(getmodepropertytable(), p_which, p_effective == True, t_is_array_prop, kMCPropertyInfoChunkTypeNone);
	
	if (t_info == nil || t_info -> getter == nil)
	{
		MCeerror -> add(EE_OBJECT_GETNOPROP, 0, 0);
		return false;
	}
	
	if (t_is_array_prop)
	{
		MCObjectIndexPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		t_object . index = p_index;
		
		MCExecFetchProperty(ctxt, t_info, &t_object, r_value);
	}
	else
	{
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		MCExecFetchProperty(ctxt, t_info, &t_object, r_value);
	}
	
    return (!ctxt . HasError());
}

bool MCObject::setprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, MCNameRef p_index, Boolean p_effective, MCExecValue p_value)
{
	bool t_is_array_prop;
	// MW-2011-11-23: [[ Array Chunk Props ]] If index is nil or empty, then its just a normal
	//   prop, else its an array prop.
	t_is_array_prop = (p_index != nil && !MCNameIsEmpty(p_index));
	
	MCPropertyInfo *t_info;
	t_info = lookup_object_property(getpropertytable(), p_which, p_effective == True, t_is_array_prop, kMCPropertyInfoChunkTypeNone);
	if (t_info == nil)
		t_info = lookup_object_property(getmodepropertytable(), p_which, p_effective == True, t_is_array_prop, kMCPropertyInfoChunkTypeNone);
	
	if (t_info == nil || t_info -> setter == nil)
	{
		MCeerror -> add(EE_OBJECT_SETNOPROP, 0, 0);
		return false;
	}
	
	if (t_is_array_prop)
	{
		
		MCObjectIndexPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		t_object . index = p_index;
		
		MCExecStoreProperty(ctxt, t_info, &t_object, p_value);
	}
	else
	{
		MCObjectPtr t_object;
		t_object . object = this;
		t_object . part_id = p_part_id;
		
		MCExecStoreProperty(ctxt, t_info, &t_object, p_value);
	}
	
    return (!ctxt . HasError());
}

////////////////////////////////////////////////////////////////////////////////

void MCObject::getboolprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, bool& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeBool, &r_value);
}

void MCObject::setboolprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, bool p_value)
{
    MCExecValue t_value;
    t_value . bool_value = p_value;
    t_value . type = kMCExecValueTypeBool;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
	ctxt . Throw();
}

//////////

void MCObject::getintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, integer_t& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeInt, &r_value);
}

//////////

void MCObject::getuintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, uinteger_t& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeUInt, &r_value);
}

void MCObject::setuintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, uinteger_t p_value)
{
    MCExecValue t_value;
    t_value . uint_value = p_value;
    t_value . type = kMCExecValueTypeUInt;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
	ctxt . Throw();
}

void MCObject::setintprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, integer_t p_value)
{
    MCExecValue t_value;
    t_value . int_value = p_value;
    t_value . type = kMCExecValueTypeInt;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
	ctxt . Throw();
}
//////////

void MCObject::getdoubleprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, double& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeDouble, &r_value);
}

void MCObject::getnumberprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCNumberRef& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeNumberRef, &r_value);
}

/////////

void MCObject::getstringprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCStringRef& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeStringRef, &r_value);
}

void MCObject::setstringprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCStringRef p_value)
{
    MCExecValue t_value;
    t_value . stringref_value = MCValueRetain(p_value);
    t_value . type = kMCExecValueTypeStringRef;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
	ctxt . Throw();
}

/////////

void MCObject::getarrayprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCArrayRef& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeArrayRef, &r_value);
}

/////////

void MCObject::getvariantprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCValueRef& r_value)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypeValueRef, &r_value);
}

void MCObject::setvariantprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCValueRef p_value)
{
    MCExecValue t_value;
    t_value . valueref_value = MCValueRetain(p_value);
    t_value . type = kMCExecValueTypeValueRef;
    if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        return;
    
    ctxt . Throw();
}

/////////

void MCObject::setdataprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCDataRef p_value)
{
    MCExecValue t_value;
    t_value . dataref_value = p_value;
    t_value . type = kMCExecValueTypeDataRef;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
    ctxt . Throw();
}

//////////

void MCObject::getpointprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, MCPoint &r_point)
{
    MCExecValue t_value;
	if (getprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value , kMCExecValueTypePoint, &r_point);
}

void MCObject::setpointprop(MCExecContext& ctxt, uint32_t p_part_id, Properties p_which, Boolean p_effective, const MCPoint &p_point)
{
    MCExecValue t_value;
    MCPoint t_point;
    t_point . x = p_point . x;
    t_point . y = p_point . y;
    t_value . point_value = t_point;
    t_value . type = kMCExecValueTypePoint;
	if (setprop(ctxt, p_part_id, p_which, nil, p_effective, t_value))
		return;
	
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

MCPlatformControlType MCObject::getcontroltype()
{
    return m_theme_type;
}

MCPlatformControlPart MCObject::getcontrolsubpart()
{
    return kMCPlatformControlPartNone;
}

MCPlatformControlState MCObject::getcontrolstate()
{
    int t_state = kMCPlatformControlStateNormal;
    
    if (flags & F_DISABLED)
        t_state |= kMCPlatformControlStateDisabled;
    
    if (getstack() && MCmousestackptr == getstack() && getstack()->getcurcard() == getcard())
    {
        if (MCmousex <= rect.x && rect.x+rect.width < MCmousex
            && MCmousey <= rect.y && rect.y+rect.height < MCmousey)
            t_state |= kMCPlatformControlStateMouseOver;
        
        if (getstack()->getcurcard()->getmousecontrol() == this)
            t_state |= kMCPlatformControlStateMouseFocus;
    }
    
    if (getstack() && getstack()->getcurcard() == getcard() && getstack()->state & CS_KFOCUSED)
        t_state |= kMCPlatformControlStateWindowActive;
    
    // Remain in backwards-compatible mode if requested
    intenum_t t_theme = gettheme();
    if (t_theme == kMCInterfaceThemeLegacy)
        t_state |= kMCPlatformControlStateCompatibility;
    
    return MCPlatformControlState(t_state);
}

bool MCObject::getthemeselectorsforprop(Properties which, MCPlatformControlType& r_type, MCPlatformControlPart& r_part, MCPlatformControlState& r_state, MCPlatformThemeProperty& r_prop, MCPlatformThemePropertyType& r_proptype)
{
    // Get the theming selectors for this object
    MCPlatformControlType t_type;
    MCPlatformControlPart t_part;
    MCPlatformControlState t_state;
    MCPlatformThemeProperty t_prop;
    MCPlatformThemePropertyType t_proptype;
    t_type = getcontroltype();
    t_part = getcontrolsubpart();
    t_state = getcontrolstate();
    
    // Transform the LiveCode property into the corresponding theme property
    switch (which)
    {
        case P_FORE_PIXEL:
        case P_FORE_COLOR:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyTextColor;
            break;
            
        case P_BACK_PIXEL:
        case P_BACK_COLOR:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyBackgroundColor;
            break;
            
        case P_HILITE_PIXEL:
        case P_HILITE_COLOR:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyBackgroundColor;
            t_state = MCPlatformControlState(t_state | kMCPlatformControlStateSelected);
            break;
            
        case P_FOCUS_COLOR:
        case P_FOCUS_PIXEL:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyFocusColor;
            break;
            
        case P_BORDER_COLOR:
        case P_BORDER_PIXEL:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyBorderColor;
            break;
            
        case P_SHADOW_COLOR:
        case P_SHADOW_PIXEL:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyShadowColor;
            break;
            
        case P_TOP_COLOR:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyTopEdgeColor;
            break;
            
        case P_BOTTOM_COLOR:
            t_proptype = kMCPlatformThemePropertyTypeColor;
            t_prop = kMCPlatformThemePropertyBottomEdgeColor;
            break;
            
        case P_TEXT_FONT:
            t_proptype = kMCPlatformThemePropertyTypeFont;
            t_prop = kMCPlatformThemePropertyTextFont;
            break;
            
        case P_TEXT_SIZE:
            t_proptype = kMCPlatformThemePropertyTypeInteger;
            t_prop = kMCPlatformThemePropertyTextSize;
            break;
            
        default:
            // Unsupported property type
            return false;
    }
    
    r_type = t_type;
    r_part = t_part;
    r_state = t_state;
    r_prop = t_prop;
    r_proptype = t_proptype;
    return true;
}

MCInterfaceTheme MCObject::gettheme() const
{
    if (m_theme != kMCInterfaceThemeEmpty)
        return m_theme;
    else if (parent)
        return parent->gettheme();
    else
        return kMCInterfaceThemeNative;
}

////////////////////////////////////////////////////////////////////////////////


#include "props.cpp"
