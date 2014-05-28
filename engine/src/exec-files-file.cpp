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

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCFilesOpenModeEnumElementInfo[] =
{
    { "update", OM_UPDATE, false },
	{ "write", OM_WRITE, false },
	{ "read", OM_READ, false },
    { "append", OM_APPEND, false },
	{ "neither", OM_NEITHER, false },
};

static MCExecEnumTypeInfo _kMCFilesOpenModeEnumTypeInfo =
{
	"Files.OpenModeEnum",
	sizeof(_kMCFilesOpenModeEnumElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCFilesOpenModeEnumElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCFilesOpenModeEnumTypeInfo = &_kMCFilesOpenModeEnumTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static MCPropertyInfo kProperties[] =
{
    DEFINE_RO_PSEUDO_OBJ_ENUM_PROPERTY(P_OPEN_MODE, FilesOpenMode, MCFile, OpenMode)
};

static MCObjectPropertyTable kPropertyTable =
{
    nil,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

MCObjectPropertyTable *MCFileGetPropertyTable()
{
    return &kPropertyTable;
}

bool MCFileGetObject(MCExecContext& ctxt, MCNameRef p_expression, MCPseudoObjectChunkPtr& r_pobj)
{
    uindex_t t_index;
    if (!IO_findfile(p_expression, t_index))
        return false;
    
    r_pobj . file = &MCfiles[t_index];
    r_pobj . type = kMCPseudoObjectTypeFile;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCFileGetOpenMode(MCExecContext& ctxt, Streamnode *file, intenum_t& r_mode)
{
    r_mode = file -> mode;
}
