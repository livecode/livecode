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

MCObjptr::MCObjptr() :
  m_id(0),
  m_objptr(nullptr),
  m_parent(nullptr)
{
}

MCObjptr::~MCObjptr()
{
}

bool MCObjptr::visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
    // Skip this node if the reference has been broken
    if (!Get().IsValid())
        return true;
    
    return Get()->visit(p_options, p_part, p_visitor);
}

IO_stat MCObjptr::load(IO_handle stream)
{
	return checkloadstat(IO_read_uint4(&m_id, stream));
}

IO_stat MCObjptr::save(IO_handle stream, uint4 p_part)
{
	IO_stat stat;
	if ((stat = IO_write_uint1(OT_PTR, stream)) != IO_NORMAL)
		return stat;
	return IO_write_uint4(m_id, stream);
}

void MCObjptr::setparent(MCObject *newparent)
{
    // Assumption: the new parent is a valid object pointer
    MCAssert(newparent != nullptr);
    
    // Assumption: this pointer is still valid given the new parent
    // (i.e the re-parenting isn't to a completely unrelated object)
    //
    // This is a relatively expensive assertion to check so should only be
    // enabled in debug builds.
    MCAssert(!m_objptr.IsBound() || newparent->getstack()->getcontrolid(CT_LAYER, m_id) == m_objptr);
    
    m_parent = newparent;
}

MCObjectHandle MCObjptr::Get()
{
    // If the object pointer hasn't yet been bound, resolve it using the ID
    if (!m_objptr.IsBound() && m_parent . IsValid())
    {
        // Note that this may return null if the control doesn't exist
        m_objptr = m_parent->getstack()->getcontrolid(CT_LAYER, m_id);
    }
    
    return m_objptr;
}

MCControl* MCObjptr::getref()
{
    // A number of callers expect this method to return null when not valid
    MCObjectHandle t_handle = Get();
    if (!Get().IsValid())
        return nullptr;
    
    return t_handle.GetAs<MCControl>();
}

void MCObjptr::setref(MCControl *optr)
{
    // Assumption: the new control is a valid control pointer
    MCAssert(optr != nullptr);
    
    // Store a handle to the control so we don't have a dangling pointer should
    // the referent be deleted (shared background groups can cause this to
    // happen as the group isn't "really" a child of each card it is on)
    m_objptr = optr;
    
    // Store the ID too as it is what is stored when serialising this pointer
    m_id = m_objptr->getid();
}

void MCObjptr::clearref()
{
    // Note that this doesn't reset the ID
    m_objptr = nullptr;
}

uint4 MCObjptr::getid()
{
	return m_id;
}

void MCObjptr::setid(uint4 newid)
{
    // Update the stored ID. The object pointer will only be bound when needed.
    m_objptr = nullptr;
    m_id = newid;
}
