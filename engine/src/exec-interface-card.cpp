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
	if (parent != NULL)
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

void MCCard::GetGroupProps(MCExecContext& ctxt, Properties which, MCStringRef& r_props)
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

		MCObjptr *optr = objptrs;
		uint2 i = 0;
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();

			optr = optr -> next();

			if (t_group == nil)
				continue;

			if (t_want_background && !t_group -> isbackground())
				continue;

			if (t_want_shared && !t_group -> isshared())
				continue;

			MCAutoStringRef t_property;

			if (which == P_BACKGROUND_NAMES || which == P_SHARED_GROUP_NAMES || which == P_GROUP_NAMES)
			{
				t_group -> GetShortName(ctxt, &t_property);
				t_success = !ctxt . HasError();
			}
			else
			{
				uint32_t t_id;
				t_group -> GetId(ctxt, t_id);
				t_success = MCStringFormat(&t_property, "%d", t_id);
			}

			if (t_success)
				t_success = MCListAppend(*t_prop_list, *t_property);

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
	GetGroupProps(ctxt, P_BACKGROUND_NAMES, r_names);
}

void MCCard::GetBackgroundIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetGroupProps(ctxt, P_BACKGROUND_IDS, r_ids);
}

void MCCard::GetSharedGroupNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	GetGroupProps(ctxt, P_SHARED_GROUP_NAMES, r_names);
}

void MCCard::GetSharedGroupIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetGroupProps(ctxt, P_SHARED_GROUP_IDS, r_ids);
}

void MCCard::GetGroupNames(MCExecContext& ctxt, MCStringRef& r_names)
{
	GetGroupProps(ctxt, P_GROUP_NAMES, r_names);
}

void MCCard::GetGroupIds(MCExecContext& ctxt, MCStringRef& r_ids)
{
	GetGroupProps(ctxt, P_GROUP_IDS, r_ids);
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

void MCCard::GetFormattedHeight(MCExecContext& ctxt, uinteger_t& r_value)
{
	MCRectangle minrect = computecrect();
	r_value = minrect . height;
}

void MCCard::GetFormattedWidth(MCExecContext& ctxt, uinteger_t& r_value)
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
			defbutton -> GetLongId(ctxt, r_button);
		else
			odefbutton -> GetLongId(ctxt, r_button);
}