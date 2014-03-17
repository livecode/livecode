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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "button.h"
#include "card.h"
#include "group.h"
#include "stacklst.h"
#include "sellst.h"
#include "util.h"

#include "globals.h"

MCStacknode::~MCStacknode()
{}

MCStack *MCStacknode::getstack()
{
	return stackptr;
}

MCStacklist::MCStacklist()
{
	stacks = NULL;
	menus = NULL;
	nmenus = 0;
	accelerators = NULL;
	naccelerators = 0;
	locktop = False;
#ifdef _MACOSX
	active = False;
#else
	active = True;
#endif
	dirty = false;
}

MCStacklist::~MCStacklist()
{
	while (stacks != NULL)
	{
		MCStacknode *tptr = stacks->remove
		                    (stacks);
		delete tptr;
	}
	if (menus != NULL)
		delete menus;
	if (accelerators != NULL)
		delete accelerators;
}

void MCStacklist::add(MCStack *sptr)
{
	MCStacknode *tptr = new MCStacknode(sptr);
	tptr->appendto(stacks);
	if (this == MCstacks) // should be done with subclass
		top(sptr);
}

void MCStacklist::remove(MCStack *sptr)
{
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	do
	{
		if (tptr->getstack() == sptr)
		{
			tptr = tptr->remove
			       (stacks);
			delete tptr;
			top(NULL);
			restart = True;
			return;
		}
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

void MCStacklist::destroy()
{
	if (stacks != NULL)
	{
		Boolean oldstate = MClockmessages;
		MClockmessages = True;
		while (stacks != NULL)
		{
			MCStacknode *tptr = stacks->remove(stacks);
			MCdispatcher->destroystack(tptr->getstack(), True);
			delete tptr;
		}
		MClockmessages = oldstate;
	}
}

Boolean MCStacklist::isempty()
{
	return stacks == NULL;
}

static int stack_real_mode(MCStack *p_stack)
{
	MCStack *t_parent;
	t_parent = MCdispatcher -> findstackd(p_stack -> getparentwindow());
	if (t_parent != NULL && t_parent -> getmode() == WM_MODAL)
		return WM_MODAL;
	return p_stack -> getmode();
}

static bool stack_is_above(MCStack *p_stack_a, MCStack *p_stack_b)
{
	int t_mode_a, t_mode_b;

	t_mode_a = stack_real_mode(p_stack_a);
	t_mode_b = stack_real_mode(p_stack_b);

	if (t_mode_a == WM_SHEET && t_mode_b == WM_MODAL)
		return false;
	else if (t_mode_a == WM_MODAL && t_mode_b == WM_SHEET)
		return true;

	return t_mode_a > t_mode_b;
}

void MCStacklist::top(MCStack *sptr)
{
	if (stacks == NULL || locktop)
		return;

	MCStacknode *tptr = stacks;
	
	if (sptr != NULL)
	{
#ifdef _WINDOWS
		MCStacknode *t_above = NULL;
		MCStacknode *t_current = NULL;

		// MW-2007-04-02: Bug 3916 - having an invisible palette causes things to become out of order
		do
		{
			if (tptr -> getstack() -> getmode() >= (MCraisepalettes ? WM_PALETTE : WM_MODAL) && stack_is_above(tptr -> getstack(), sptr))
				t_above = tptr;
			else if (tptr -> getstack() == sptr)
				t_current = tptr;

			tptr = tptr -> next();
		}
		while(tptr != stacks);

		if (t_above == NULL)
		{
			// MW-2012-03-29: [[ LobsterCrash ]] It looks like it is possible for top() to be called
			//   (from restackwindows()) with an sptr not in the stacks list. Thus make sure t_current
			//   is not nil before attempting to move it!
			if (t_current != nil)
				t_current -> totop(stacks);
		}
		else if (t_current != NULL)
		{
			t_current -> remove(stacks);
			t_above -> append(t_current);
		}

#else

		do
		{
			if (tptr->getstack() == sptr)
			{
				tptr->totop(stacks);
				break;
			}
			tptr = tptr->next();
		}
		while (tptr != stacks);
		// This is only necessary on MWM...
#ifdef X11
		restack(sptr);
#endif
#endif
	}

	uint2 pass = WM_TOP_LEVEL;
	MCtopstackptr = NULL;
	do
	{
		tptr = stacks;
		do
		{
			if (tptr->getstack()->getmode() == pass)
			{
				MCtopstackptr = tptr->getstack();
				return;
			}
			tptr = tptr->next();
		}
		while (tptr != stacks);
		pass++;
	}
	while (pass != WM_LAST);
}

void MCStacklist::setcmap()
{
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	do
	{
		MCscreen->setcmap(tptr->getstack());
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

void MCStacklist::closeall()
{
	while (stacks != NULL)
	{
		MCStack *sptr = stacks->getstack();
		sptr->close();
	}
}

MCStack *MCStacklist::getstack(uint2 n)
{
	uint2 i;
	MCStacknode *tptr = stacks;
	for (i = 1 ; i < n ; i++)
	{
		if (tptr->next() == stacks)
			return NULL;
		tptr = tptr->next();
	}
	if (tptr->getstack()->getparent()->gettype() == CT_BUTTON
	        || tptr->getstack() == (MCStack *)MCtooltip)
		return NULL;
	return tptr->getstack();
}

void MCStacklist::stackprops(MCExecPoint &ep, Properties p)
{
	ep.clear();
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	MCExecPoint ep2(ep);

	do
	{
		MCStack *stackptr = tptr->getstack();

		if (stackptr->getparent()->gettype() != CT_BUTTON && stackptr != (MCStack *)MCtooltip)
		{
			stackptr->getprop(0, p, ep2, True);
			ep.concatmcstring(ep2.getsvalue(), EC_RETURN, tptr == stacks);
		}
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

Boolean MCStacklist::doaccelerator(KeySym key)
{
	if (stacks == NULL)
		return False;
	if (key < 256 && isupper((uint1)key))
		key = MCS_tolower((uint1)key);
	uint2 t_mod_mask = MS_CONTROL | MS_MAC_CONTROL | MS_MOD1;

	if ((key >= 'a' && key <= 'z') || (key & 0xFF00) == 0xFF00)
		t_mod_mask |= MS_SHIFT;
	// There are numerous issues with menu accelerators at the moment. I belive these
	// problems are caused by two things:
	//   * No mousedown sent to the current menu bar before an accelerator search
	//   * The current menubar is not searched first
	// We fix these issues here...

	MCGroup *t_menubar;
	if (MCmenubar != NULL)
		t_menubar = MCmenubar;
	else
		t_menubar = MCdefaultmenubar;

	if (t_menubar != NULL)
	{
		// OK-2008-03-20 : Bug 6153. Don't send a mouse button number if the mouseDown is due to
		// a menu accelerator.
		// MW-2008-08-27: [[ Bug 6995 ]] Slowdown caused by repeated invocation of mouseDown even if
		//   an accelerator is not present. To mitigate this we only mouseDown if we find the accelerator
		//   first.
		for(uint2 i = 0; i < naccelerators; i++)
		{
			if (key == (KeySym)accelerators[i] . key && (MCmodifierstate & t_mod_mask) == (accelerators[i].mods & t_mod_mask) && accelerators[i] . button -> getparent() == t_menubar)
			{
				t_menubar -> message_with_args(MCM_mouse_down, "");

				// We now need to re-search for the accelerator, since it could have gone/been deleted in the mouseDown
				for(uint2 i = 0; i < naccelerators; i++)
				{
					if (key == (KeySym)accelerators[i] . key && (MCmodifierstate & t_mod_mask) == (accelerators[i].mods & t_mod_mask) && accelerators[i] . button -> getparent() == t_menubar)
					{
						MCmodifierstate &= t_mod_mask;
						accelerators[i] . button -> activate(True, (uint2)key);
						return True;
					}
				}
				
				// MW-2008-09-01: If we get here we've failed to fine the accelerator we originally found, so
				//   we drop through.
				break;
			}
		}
	}

	// IM-2008-09-05: Reorganize loop to be more efficient - only loop through stacks once we've
	// found a matching accelerator.
	for (uint2 i = 0 ; i < naccelerators ; i++)
	{
		if (key == (KeySym)accelerators[i].key
				&& (MCmodifierstate & t_mod_mask) == (accelerators[i].mods & t_mod_mask))
		{
			MCStacknode *tptr = stacks;
			do
			{
				if (accelerators[i].stack == tptr->getstack()
#ifdef _MACOSX
					&& tptr->getstack()->getstate(CS_KFOCUSED)
#endif
					)
				{
					MCmodifierstate &= t_mod_mask;
					accelerators[i].button->activate(True, (uint2)key);
					return True;
				}
				tptr = tptr->next();
			}
			while (tptr != stacks);
		}
	}
	return False;
}

void MCStacklist::addaccelerator(MCButton *button, MCStack *stack,
                                 uint2 key, uint1 mods)
{
	MCU_realloc((char **)&accelerators, naccelerators, naccelerators + 1,
	            sizeof(Accelerator));
	accelerators[naccelerators].button = button;
	accelerators[naccelerators].stack = stack;
	accelerators[naccelerators].key = key;
	accelerators[naccelerators].mods = mods;
	naccelerators++;
}

void MCStacklist::deleteaccelerator(MCButton *button, MCStack *stack)
{
	uint2 i = 0;
	while (i < naccelerators)
		if (accelerators[i].button == button
		        && (stack == NULL || accelerators[i].stack == stack))
		{
			uint2 j = i;
			while (++j < naccelerators)
				accelerators[j - 1] = accelerators[j];
			naccelerators--;
			if (stack != NULL)
				return;
		}
		else
			i++;
}

void MCStacklist::changeaccelerator(MCButton *button, uint2 key, uint1 mods)
{
	uint2 i;
	for (i = 0 ; i < naccelerators ; i++)
		if (accelerators[i].button == button)
		{
			accelerators[i].key = key;
			accelerators[i].mods = mods;
			return;
		}
	addaccelerator(button, button->getstack(), key, mods);
}

MCButton *MCStacklist::findmnemonic(char key)
{
	MCStacknode *tptr = stacks;
	do
	{
		uint2 i;
		for (i = 0 ; i < nmenus ; i++)
			if (menus[i].key == MCS_tolower(key)
			        && menus[i].button->getstack() == tptr->getstack())
			{
				return menus[i].button;
			}
		tptr = tptr->next();
	}
	while (tptr != stacks);
	return NULL;
}

void MCStacklist::addmenu(MCButton *button, char key)
{
	MCU_realloc((char **)&menus, nmenus, nmenus + 1, sizeof(Mnemonic));
	menus[nmenus].button = button;
	menus[nmenus].key = MCS_tolower(key);
	nmenus++;
}

void MCStacklist::deletemenu(MCButton *button)
{
	uint2 i;
	for (i = 0 ; i < nmenus ; i++)
		if (menus[i].button == button)
		{
			while (++i < nmenus)
				menus[i - 1] = menus[i];
			nmenus--;
			break;
		}
}

Window MCStacklist::restack(MCStack *sptr)
{
	Window_mode m = sptr == NULL ? WM_TOP_LEVEL : sptr->getrealmode();
	if (stacks != NULL && MCraisepalettes && m < WM_PALETTE)
	{
		MCStacknode *tptr = stacks->prev();
		MCStacknode *startptr = stacks;
		MCStacknode *bottompalette = NULL;

#if defined(LINUX)
		// MW-2004-11-24: If sptr == NULL, then find the top non-palette and use that
		if (sptr == NULL)
			for(MCStacknode *tptr = stacks; tptr != NULL; tptr = tptr -> prev())
			{
				if (tptr -> getstack() -> getrealmode() < WM_PALETTE)
				{
					sptr = tptr -> getstack();
					break;
				}
			}
#endif

		do
		{
			m = tptr->getstack()->getrealmode();
			if (m >= WM_PALETTE && tptr->getstack()->isvisible())
			{
#if defined(LINUX)
				// MW-2004-11-16: Make sure all palettes are TrasientFor the new topstack
				// MW-2004-11-24: Added explicit raisewindow to force WM to act
				if (m == WM_PALETTE && tptr->getstack()->isvisible() && sptr != NULL)
				{
					extern void MCLinuxWindowSetTransientFor(Window, Window);
					MCLinuxWindowSetTransientFor(tptr -> getstack() -> getwindow(), sptr -> getwindow());
					MCscreen->raisewindow(tptr->getstack()->getwindow());
				}
#endif
				if (bottompalette == NULL)
					bottompalette = tptr;
				MCStacknode *ttptr = tptr;
				tptr = tptr->prev();
				ttptr->totop(stacks);
			}
			else
				tptr = tptr->prev();
		}
		while (tptr != startptr);
		if (startptr->getstack()->getrealmode() == WM_PALETTE
		        && startptr->getstack()->isvisible())
			startptr->totop(stacks);
		if (bottompalette == NULL && stacks->getstack()->getrealmode() == WM_PALETTE
		        && stacks->getstack()->isvisible())
			bottompalette = stacks;
		if (bottompalette != NULL)
			return bottompalette->getstack()->getwindow();
	}
	return DNULL;
}

void MCStacklist::restartidle()
{
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;
	do
	{
		MCStack *sptr = tptr->getstack();
		MCscreen->cancelmessageobject(sptr, MCM_idle);
		MCscreen->addtimer(sptr, MCM_idle, MCidleRate);
		MCscreen->cancelmessageobject(sptr->getcurcard(), MCM_idle);
		MCscreen->addtimer(sptr->getcurcard(), MCM_idle, MCidleRate);
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

void MCStacklist::refresh(void)
{
	MCStacknode *t_node = stacks;
	if (t_node == NULL)
		return;
		
	do
	{
		Window t_handle;
		t_handle = t_node -> getstack() -> getwindow();
		if (t_handle != NULL)
			MCscreen -> assignbackdrop(t_node -> getstack() -> getmode(), t_handle);
		t_node = t_node -> next();
	}
	while(t_node != stacks);
}

#if !defined(_MAC_DESKTOP)
void MCStacklist::ensureinputfocus(Window window)
{
}
#endif

void MCStacklist::purgefonts()
{
	if (stacks != NULL)
	{
		MCStacknode *tptr = stacks;
		do
		{
			tptr->getstack()->purgefonts();
			tptr = tptr->next();
		}
		while (tptr != stacks);
	}
}

// This method iterates through all the stacks from top to bottom, and enables
// or disables them depending on type and what windows are above.
void MCStacklist::enableformodal(Window modalwindow, Boolean isenabled)
{
	if (stacks == NULL)
		return;

	MCStack *t_target_stack;
	t_target_stack = MCdispatcher -> findstackd(modalwindow);

	MCStacknode *t_node;
	t_node = stacks;
	do
	{
		MCStack *t_stack;
		t_stack = t_node -> getstack();

		if (t_stack -> isvisible() && t_stack -> getw() != NULL)
		{
			// We have a window whose enable state we need to compute
			bool t_is_enabled;
			t_is_enabled = true;

			// A window should be disabled if:
			//   1) it is an ancestor of a sheet
			//   2) it is lower in the stack than a modal dialog
			//
			MCStacknode *t_other_node;
			t_other_node = t_node -> prev();
			while(t_other_node != stacks -> prev())
			{
				MCStack *t_other_stack;
				t_other_stack = t_other_node -> getstack();
				
				// If we are called with isenabled True, it means that 'modalwindow' is
				// being closed and shouldn't be factored into any enable calculations.
				bool t_ignore;
				t_ignore = false;
				if (t_other_stack == t_target_stack && isenabled)
					t_ignore = true;
				else if (t_other_stack -> getw() == NULL || !t_other_stack -> isvisible())
					t_ignore = true;

				// If it is a candidate window that could affect the enable state of t_stack...
				if (!t_ignore)
				{
					// Compute the effective mode of the stack above <t_stack> we are
					// considering - this is to handle the case of SHEETs having a 
					// MODAL in their ancestry.
					int t_mode;
					t_mode = stack_real_mode(t_other_stack);

					// MODAL windows disable all windows underneath them
					if (t_mode == WM_MODAL)
					{
						t_is_enabled = false;
						break;
					}

					// SHEET windows disable only ancestor windows
					if (t_mode == WM_SHEET)
					{
						MCStack *t_other_parent_stack;
						t_other_parent_stack = t_other_stack;
						while(t_other_parent_stack != NULL)
						{
							t_other_parent_stack = MCdispatcher -> findstackd(t_other_parent_stack -> getparentwindow());
							
							if (t_other_parent_stack == t_stack)
							{
								t_is_enabled = false;
								break;
							}
						}

						if (t_other_parent_stack != NULL)
							break;
					}
				}

				t_other_node = t_other_node -> prev();
			}

			t_stack -> setextendedstate(!t_is_enabled, ECS_DISABLED_FOR_MODAL);
			t_stack -> enablewindow(t_is_enabled);
		}

		t_node = t_node -> next();
	}
	while(t_node != stacks);
}

////////////////////////////////////////////////////////////////////////////////
