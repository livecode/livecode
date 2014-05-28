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
#include "mcio.h"

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
    DEFINE_RO_PSEUDO_OBJ_ENUM_PROPERTY(P_OPEN_MODE, FilesOpenMode, MCProcess, OpenMode)
};

static MCObjectPropertyTable kPropertyTable =
{
    nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

MCObjectPropertyTable *MCProcessGetPropertyTable()
{
    return &kPropertyTable;
}

bool MCProcessGetObject(MCExecContext& ctxt, MCNameRef p_expression, MCPseudoObjectChunkPtr& r_pobj)
{
    uindex_t t_index;
    if (!IO_findprocess(p_expression, t_index))
        return false;
    
    r_pobj . process = &MCprocesses[t_index];
    r_pobj . type = kMCPseudoObjectTypeProcess;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCProcessGetOpenMode(MCExecContext& ctxt, Streamnode *file, intenum_t& r_mode)
{
    r_mode = file -> mode;
}
