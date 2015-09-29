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

class MCObjptr : public MCDLlist
{
protected:
	uint4 id;
	MCControl *objptr;
	MCObject *parent;
public:
	MCObjptr();
	virtual ~MCObjptr();

	bool visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor);

	IO_stat load(IO_handle stream);
	IO_stat save(IO_handle stream, uint4 p_part);
	void setparent(MCObject *newparent);
	MCControl *getref();
	void setref(MCControl *optr);
	void clearref();
	uint4 getid();
	void setid(uint4 id);

	// MW-2011-08-08: [[ Groups ]] Returns the referenced object as an MCGroup
	//   or nil, if it isn't a group.
	MCGroup *getrefasgroup(void)
	{
		MCObject *t_group;
		t_group = (MCObject *)getref();
		if (t_group -> gettype() != CT_GROUP)
			return NULL;
		return (MCGroup *)t_group;
	}

	MCObjptr *next()
	{
		return (MCObjptr *)MCDLlist::next();
	}
	MCObjptr *prev()
	{
		return (MCObjptr *)MCDLlist::prev();
	}
	void totop(MCObjptr *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCObjptr *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCObjptr *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCObjptr *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCObjptr *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCObjptr *remove(MCObjptr *&list)
	{
		return (MCObjptr *)MCDLlist::remove((MCDLlist *&)list);
	}
};
#endif
