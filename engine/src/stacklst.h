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

//
// List of windows in their stacking order
//
#ifndef	STACKLIST_H
#define	STACKTLIST_H

#include "dllst.h"

typedef struct _Accelerator
{
	uint2 key;
	uint2 mods;
	MCButton *button;
	MCStack *stack;
}
Accelerator;

typedef struct _Mnemonic
{
	char key;
	MCButton *button;
}
Mnemonic;

class MCStacknode : public MCDLlist
{
	MCStack *stackptr;
public:
	MCStacknode(MCStack *sptr)
	{
		stackptr = sptr;
	}
	~MCStacknode();
	MCStack *getstack();
	MCStacknode *next()
	{
		return (MCStacknode *)MCDLlist::next();
	}
	MCStacknode *prev()
	{
		return (MCStacknode *)MCDLlist::prev();
	}
	void totop(MCStacknode *&list)
	{
		MCDLlist::totop((MCDLlist *&)list);
	}
	void insertto(MCStacknode *&list)
	{
		MCDLlist::insertto((MCDLlist *&)list);
	}
	void appendto(MCStacknode *&list)
	{
		MCDLlist::appendto((MCDLlist *&)list);
	}
	void append(MCStacknode *node)
	{
		MCDLlist::append((MCDLlist *)node);
	}
	void splitat(MCStacknode *node)
	{
		MCDLlist::splitat((MCDLlist *)node) ;
	}
	MCStacknode *remove
	(MCStacknode *&list)
	{
		return (MCStacknode *)MCDLlist::remove
			       ((MCDLlist *&)list);
	}
};

class MCStacklist
{
	MCStacknode *stacks;
	Mnemonic *menus;
	uint2 nmenus;
	Accelerator *accelerators;
	uint2 naccelerators;
	Boolean locktop;
	Boolean restart;
	Boolean active;

	// MW-2011-08-17: [[ Redraw ]] This is true if something needs updating.
	bool dirty;

public:
	MCStacklist();
	~MCStacklist();
	void add(MCStack *sptr);
	void remove(MCStack *sptr);

	MCStacknode *topnode(void) { return stacks; }
	MCStacknode *bottomnode(void) { return stacks -> prev(); }

	void destroy();
	Boolean isempty();
	void stackprops(MCExecPoint &, Properties p);
	Boolean doaccelerator(KeySym key);
	void addaccelerator(MCButton *button, MCStack *stack, uint2 key, uint1 mods);
	void deleteaccelerator(MCButton *button, MCStack *stack);
	void changeaccelerator(MCButton *button, uint2 key, uint1 mods);
	MCButton *findmnemonic(char key);
	void addmenu(MCButton *button, char key);
	void deletemenu(MCButton *button);

	void setcmap();
	void redrawall(Boolean force);
	void closeall();
	MCStack *getstack(uint2 n);

	// Called from MCButton::openmenu when MCraisemenus is true
	// Called from MCStack::kfocus
	// Called from MCStack::setprop(CANT_MODIFY)
	// Called from MCStack::iconify
	// Called from MCStack::uniconify
	// Called from MCStack::openrect if already opened
	// Called from MCStacklist::add if MCstacks
	// Called from MCStacklist::remove
	// Called from MCScreenDC::restack
	void top(MCStack *sptr);

	// Called from WM_SIZE
	// Called from set raisepalettes on Linux
	Window restack(MCStack *sptr);

	// Called from WM_ACTIVATEAPP
	void hidepalettes(Boolean hide);

	void restartidle();

	void refresh(void);

	Boolean getactive()
	{
		return active;
	}
	
	void enableformodal(Window modalwindow, Boolean isenabled);
	
	// MW-2013-09-11: [[ DynamicFonts ]] Purge all references to fonts so they
	//   can be re-referenced (needed after loading new fonts or formatForPrinting
	//   change).
	void purgefonts(void);

	void ensureinputfocus(Window window);
};

#endif
