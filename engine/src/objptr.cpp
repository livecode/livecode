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

#include "object.h"
#include "mccontrol.h"
#include "stack.h"
#include "objptr.h"

#include "util.h"

MCObjptr::MCObjptr()
{
	id = 0;
	parent = NULL;
	objptr = NULL;
}

MCObjptr::~MCObjptr()
{
}

bool MCObjptr::visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	return getref() -> visit(p_options, p_part, p_visitor);
}

IO_stat MCObjptr::load(IO_handle stream)
{
	return checkloadstat(IO_read_uint4(&id, stream));
}

IO_stat MCObjptr::save(IO_handle stream, uint4 p_part)
{
	IO_stat stat;
	if ((stat = IO_write_uint1(OT_PTR, stream)) != IO_NORMAL)
		return stat;
	return IO_write_uint4(id, stream);
}

void MCObjptr::setparent(MCObject *newparent)
{
	parent = newparent;
}

MCControl *MCObjptr::getref()
{
	if (objptr == NULL)
		return (objptr = (parent->getstack())->getcontrolid(CT_LAYER, id));
	else
		return objptr;
}

void MCObjptr::setref(MCControl *optr)
{
	objptr = optr;
	id = objptr->getid();
}

void MCObjptr::clearref()
{
	objptr = NULL;
}

uint4 MCObjptr::getid()
{
	return id;
}

void MCObjptr::setid(uint4 newid)
{
	id = newid;
	objptr = NULL;
}
