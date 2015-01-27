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

#include "uidc.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
    DEFINE_RO_PSEUDO_OBJ_PROPERTY(P_RECTANGLE, Rectangle, MCScreen, Rect)
};

static MCObjectPropertyTable kPropertyTable =
{
    nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

MCObjectPropertyTable *MCScreenGetPropertyTable()
{
    return &kPropertyTable;
}

bool MCScreenGetObject(MCExecContext& ctxt, MCNameRef p_expression, MCPseudoObjectChunkPtr& r_pobj)
{
    uindex_t t_display;
    if (!ctxt . ConvertToUnsignedInteger(p_expression, t_display))
        return false;
    
    const MCDisplay *t_displays;
    uindex_t t_count = MCscreen -> getdisplays(t_displays, /*p_effective*/ false);
    
    if (t_display > t_count)
        return false;
    
    r_pobj . display = &t_displays[t_display - 1];
    r_pobj . type = kMCPseudoObjectTypeScreen;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenGetRect(MCExecContext& ctxt, const MCDisplay *display, MCRectangle& r_rect)
{
    r_rect = display -> viewport;
}
