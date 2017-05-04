/*                                                             -*-c++-*-
Copyright (C) 2003-2017 LiveCode Ltd.

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

#include "stack.h"

#include <vector>

class MCSellist
{
private:
    /* This class represents an entry in the list of selected objects.
     * Whether or not an object is considered selected is directly
     * linked to the lifetime of its MCSelnode. While an object has a
     * corresponding MCSelnode, it's selected; once the MCSelnode is
     * destroyed, the object is no longer selected. */
    class MCSelnode : public MCObjectHandle
    {
    public:
	    MCSelnode();
        MCSelnode(MCObjectHandle object);
        MCSelnode(MCSelnode&& other);

        MCSelnode& operator=(MCSelnode&& other);

        ~MCSelnode();

        /* Prevent copying an MCSelnode, because to do so would mean
         * that multiple MCSelnode instances "own" the object's
         * selection state. */
        MCSelnode(const MCSelnode& other) = delete;
        MCSelnode& operator=(const MCSelnode& other) = delete;
    };

    MCStackHandle m_owner = nullptr;
    std::vector<MCSelnode> m_objects {};
    int2 startx = 0, starty = 0;
    int2 lastx = 0, lasty = 0;
    bool locked = false;
    bool dropclone = false;
    
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
