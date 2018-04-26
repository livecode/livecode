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
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "util.h"
#include "group.h"
#include "image.h"
#include "tilecache.h"

#include "dispatch.h"
#include "parentscript.h"
#include "stacklst.h"
#include "cardlst.h"
#include "redraw.h"
#include "external.h"

#include "exec-interface.h"

void MCCard::GetLayer(MCExecContext &ctxt, MCInterfaceLayer &r_layer)
{
	uint16_t t_num;
	getstack()->count(CT_CARD, CT_UNDEFINED, this, t_num);
	
	r_layer.layer = t_num;
}

void MCCard::SetLayer(MCExecContext &ctxt, const MCInterfaceLayer &p_layer)
{
	if (parent)
		getstack()->renumber(this, p_layer.layer);
}

void MCCard::GetCantDelete(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_C_CANT_DELETE);
}

void MCCard::SetCantDelete(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_C_CANT_DELETE);
}

void MCCard::GetDontSearch(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_C_DONT_SEARCH);
}

void MCCard::SetDontSearch(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_C_DONT_SEARCH);
}

void MCCard::GetMarked(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MARKED);
}

void MCCard::SetMarked(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_MARKED);
}

void MCCard::GetShowPict(MCExecContext& ctxt, bool& r_value)
{
	r_value = true;
}

void MCCard::SetShowPict(MCExecContext& ctxt, bool value)
{
	// NO OP
}

void MCCard::GetPropList(MCExecContext& ctxt, Properties which, uint32_t part_id, MCStringRef& r_props)
{
	clean();

	MCAutoListRef t_prop_list;

	bool t_success;
	t_success = true;

	if (t_success)
		t_success = MCListCreateMutable('\n', &t_prop_list);

	if (t_success && objptrs != nil)
	{
		bool t_want_background;
		t_want_background = which == P_BACKGROUND_NAMES || which == P_BACKGROUND_IDS;
		
		bool t_want_shared;
		t_want_shared = which == P_SHARED_GROUP_NAMES || which == P_SHARED_GROUP_IDS;

        bool t_controls;
        t_controls = which == P_CHILD_CONTROL_NAMES ||  which == P_CHILD_CONTROL_IDS || which == P_CONTROL_NAMES || which == P_CONTROL_IDS;
        
		MCObjptr *optr = objptrs;
		
        do
		{
            MCObject *t_object;
            t_object = optr -> getref();
            
            optr = optr -> next();
            
            if (t_object->gettype() == CT_GROUP)
            {
                if (t_want_background && !static_cast<MCGroup *>(t_object)  -> isbackground())
                    continue;
                
                if (t_want_shared && !static_cast<MCGroup *>(t_object) -> isshared())
                    continue;
            }
            else if (!t_controls)
                continue;
            

			MCAutoStringRef t_property;

			if (which == P_BACKGROUND_NAMES || which == P_SHARED_GROUP_NAMES || which == P_GROUP_NAMES ||
                which == P_CONTROL_NAMES || which == P_CHILD_CONTROL_NAMES)
			{
				t_object -> GetShortName(ctxt, &t_property);
				t_success = !ctxt . HasError();
			}
			else
			{
				uint32_t t_id;
				t_object -> GetId(ctxt, t_id);
				t_success = MCStringFormat(&t_property, "%d", t_id);
			}

			if (t_success)
				t_success = MCListAppend(*t_prop_list, *t_property);
            
            if (t_success && t_object->gettype() == CT_GROUP && (which == P_CONTROL_IDS || which == P_CONTROL_NAMES))
            {
                MCAutoStringRef t_group_props;
                if (which ==  P_CONTROL_IDS)
                    static_cast<MCGroup *>(t_object) -> GetControlIds(ctxt, part_id, &t_group_props);
                else
                    static_cast<MCGroup *>(t_object) -> GetControlNames(ctxt, part_id, &t_group_props);
                
                // MERG-2013-11-03: [[ ChildControlProps ]] Handle empty groups
                if (!MCStringIsEmpty(*t_group_props))
                    t_success = MCListAppend(*t_prop_list , *t_group_props);
            }

		}
		while (t_success && optr != objptrs);

		if (!opened)
			clear();
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_prop_list, r_props);

	if (t_success)
		return;

	ctxt . Throw();
}

void MCCard::GetBackgroundNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	GetPropList(ctxt, P_BACKGROUND_NAMES, 0, r_names);
}

void MCCard::GetBackgroundIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetPropList(ctxt, P_BACKGROUND_IDS, 0, r_ids);
}

void MCCard::GetSharedGroupNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	GetPropList(ctxt, P_SHARED_GROUP_NAMES, 0, r_names);
}

void MCCard::GetSharedGroupIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetPropList(ctxt, P_SHARED_GROUP_IDS, 0, r_ids);
}

void MCCard::GetGroupNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	GetPropList(ctxt, P_GROUP_NAMES, 0, r_names);
}

void MCCard::GetGroupIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetPropList(ctxt, P_GROUP_IDS, 0, r_ids);
}

void MCCard::GetControlNames(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_names)
{
    GetPropList(ctxt, P_CONTROL_NAMES, part_id, r_names);
}

void MCCard::GetControlIds(MCExecContext& ctxt, uint32_t part_id, MCStringRef& r_ids)
{
    GetPropList(ctxt, P_CONTROL_IDS, part_id, r_ids);
}

void MCCard::GetChildControlNames(MCExecContext& ctxt, MCStringRef& r_names)
{
    GetPropList(ctxt, P_CHILD_CONTROL_NAMES, 0, r_names);
}

void MCCard::GetChildControlIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
    GetPropList(ctxt, P_CHILD_CONTROL_IDS, 0, r_ids);
}

void MCCard::GetFormattedLeft(MCExecContext& ctxt, integer_t& r_value)
{
	MCRectangle minrect = computecrect();
	r_value = minrect . x;
}

void MCCard::GetFormattedTop(MCExecContext& ctxt, integer_t& r_value)
{
	MCRectangle minrect = computecrect();
	r_value = minrect . y;
}

void MCCard::GetFormattedHeight(MCExecContext& ctxt, integer_t& r_value)
{
	MCRectangle minrect = computecrect();
	r_value = minrect . height;
}

void MCCard::GetFormattedWidth(MCExecContext& ctxt, integer_t& r_value)
{
	MCRectangle minrect = computecrect();
	r_value = minrect . width;
}

void MCCard::GetFormattedRect(MCExecContext& ctxt, MCRectangle& r_rect)
{
	r_rect = computecrect();
}

void MCCard::GetDefaultButton(MCExecContext& ctxt, MCStringRef& r_button)
{
	if (defbutton == nil && odefbutton == nil)
		return;
	else
		if (defbutton != nil)
			defbutton -> GetLongId(ctxt, 0, r_button);
		else
			odefbutton -> GetLongId(ctxt, 0, r_button);
}

void MCCard::SetForePixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetForePixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBackPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBackPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetHilitePixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetHilitePixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBorderPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBorderPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetTopPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetTopPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBottomPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBottomPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetShadowPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetShadowPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetFocusPixel(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetFocusPixel(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetForeColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetForeColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetBackColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetBackColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetHiliteColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetHiliteColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetBorderColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetBorderColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetTopColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetTopColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetBottomColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetBottomColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetShadowColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetShadowColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetFocusColor(MCExecContext& ctxt, const MCInterfaceNamedColor& r_color)
{
    MCObject::SetFocusColor(ctxt, r_color);
    getstack() -> dirtyall();
}

void MCCard::SetForePattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetForePattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBackPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBackPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetHilitePattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetHilitePattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBorderPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBorderPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetTopPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetTopPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetBottomPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetBottomPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetShadowPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetShadowPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetFocusPattern(MCExecContext& ctxt, uinteger_t* pixel)
{
    MCObject::SetFocusPattern(ctxt, pixel);
    getstack() -> dirtyall();
}

void MCCard::SetTextHeight(MCExecContext& ctxt, uinteger_t* height)
{
    MCObject::SetTextHeight(ctxt, height);
    getstack() -> dirtyall();
}

void MCCard::SetTextFont(MCExecContext& ctxt, MCStringRef font)
{
    MCObject::SetTextFont(ctxt, font);
    if (!ctxt . HasError())
       getstack() -> dirtyall();     
}

void MCCard::SetTextSize(MCExecContext& ctxt, uinteger_t* size)
{
    MCObject::SetTextSize(ctxt, size);
    getstack() -> dirtyall();
}

void MCCard::SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style)
{
    MCObject::SetTextStyle(ctxt, p_style);
    getstack() -> dirtyall();
}

void MCCard::SetTheme(MCExecContext& ctxt, intenum_t p_theme)
{
    MCObject::SetTheme(ctxt, p_theme);
    getstack() -> dirtyall();
}
