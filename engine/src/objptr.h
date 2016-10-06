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

//
// MCObject pointer class declarations
//
#ifndef	OBJPTR_H
#define	OBJPTR_H

#include "dllst.h"
#include "object.h"
#include "mccontrol.h"
#include "group.h"

class MCObjptr : public MCDLlist
{
private:
    
	uint32_t            m_id;
	MCObjectHandle      m_objptr;
	MCObjectHandle      m_parent;
    
public:
    
    MCDLlistAdaptorFunctions(MCObjptr)
    
	MCObjptr();
	~MCObjptr();

	bool visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor* p_visitor);

	IO_stat load(IO_handle stream);
	IO_stat save(IO_handle stream, uint4 p_part);
	void setparent(MCObject *newparent);
	MCControl *getref();
    MCObjectHandle Get();
	void setref(MCControl *optr);
	void clearref();
	uint32_t getid();
	void setid(uint32_t id);

	// MW-2011-08-08: [[ Groups ]] Returns the referenced object as an MCGroup
	//   or nil, if it isn't a group.
	MCGroup *getrefasgroup(void)
	{
		MCControl* t_ref = getref();
        if (t_ref == nil || t_ref->gettype() != CT_GROUP)
            return nil;
        
        return static_cast<MCGroup*>(t_ref);
	}
};
#endif
