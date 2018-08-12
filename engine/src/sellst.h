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
// MCselected object list and clipboard class declarations
//
#ifndef	SELLIST_H
#define	SELLIST_H

#include "dllst.h"
#include "stack.h"

class MCSellist
{
private:
    
    class MCSelnode;
    
	MCStackHandle m_owner;
	MCSelnode *objects;
	MCSelnode *curobject;
	int2 startx, starty;
	int2 lastx, lasty;
	Boolean locked;
	Boolean dropclone;
    
    void Clean();
    bool IsDeletable();
    
public:
    
	MCSellist();
	~MCSellist();
	MCObjectHandle getfirst();
	bool getids(MCListRef& r_list);
	void clear(Boolean message);
	void top(MCObject *objptr);
	void replace(MCObject *objptr);
	void add(MCObject *objptr, bool p_sendmessage = true);
	void remove(MCObject *objptr, bool p_sendmessage = true);
	void sort();
	uint32_t count();
	MCControl *clone(MCObject *target);
	Exec_stat group(uint2 line, uint2 pos, MCGroup*& r_group);
	Boolean copy();
	Boolean cut();
	Boolean del();
    Boolean usercopy();
    Boolean usercut();
    Boolean userdel();
	void startmove(int2 x, int2 y, Boolean canclone);
	void continuemove(int2 x, int2 y);
	Boolean endmove();
	void lockclear()
	{
		locked = True;
	}
	void unlockclear()
	{
		locked = False;
	}
	void redraw();

private:
	bool clipboard(bool p_is_cut);
};
#endif
