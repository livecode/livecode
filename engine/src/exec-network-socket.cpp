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

#include "socket.h"
#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
	DEFINE_RO_PSEUDO_OBJ_PROPERTY(P_NAME, Name, MCSocket, Name)
};

static MCObjectPropertyTable kPropertyTable =
{
	nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

MCObjectPropertyTable *MCSocketGetPropertyTable()
{
    return &kPropertyTable;
}

bool MCSocketGetObject(MCExecContext& ctxt, MCNameRef p_expression, MCPseudoObjectChunkPtr& r_pobj)
{
    uindex_t t_index;

    if (!IO_findsocket(p_expression, t_index))
        return false;
    
    r_pobj . socket = MCsockets[t_index];
    r_pobj . type = kMCPseudoObjectTypeSocket;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCSocket::GetName(MCExecContext& ctxt, MCNameRef& r_name)
{
    r_name = MCValueRetain(name);
}

