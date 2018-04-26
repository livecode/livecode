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
#include "parsedef.h"
#include "objdefs.h"


#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "cardlst.h"
#include "util.h"
#include "exec.h"

#include "globals.h"

MCCardnode::~MCCardnode()
{ }

MCCardlist::MCCardlist()
    : first(nullptr)
{
	cards = NULL;
	interval = 0;
}

MCCardlist::~MCCardlist()
{
	while (cards != NULL)
	{
		MCCardnode *cptr = cards->remove
		                   (cards);
		delete cptr;
	}
}

void MCCardlist::trim()
{
    // Check for and remove any dead cards from the list
    deletecard(nil);
    
    while (interval > MIN_FILL)
	{
		cards = cards->prev();
		MCCardnode *cptr = cards->remove
		                   (cards);
		if (first == cptr)
			first = cards;
		delete cptr;
		interval--;
	}
}

bool MCCardlist::GetRecent(MCExecContext& ctxt, MCStack *stack, Properties which, MCStringRef& r_props)
{
	// Get list of recent card short names or long ids
	trim();
	MCCardnode *tmp = cards;

	MCAutoListRef t_prop_list;

	bool t_success;
	t_success = true; 

	if (t_success)
		t_success = MCListCreateMutable('\n', &t_prop_list);
	
	if (t_success && tmp != NULL)
	{
		do
		{
			if (stack == NULL || tmp->card->getstack() == stack)
			{
				MCAutoStringRef t_property;
				if (which == P_SHORT_NAME)
					tmp -> card -> GetShortName(ctxt, &t_property);
				else
					tmp -> card -> GetLongId(ctxt, 0, &t_property);
					
				t_success = !ctxt . HasError();
				if (t_success)
					t_success = MCListAppend(*t_prop_list, *t_property);
			}
			tmp = tmp->next();
		}
		while (tmp != cards && t_success);
	}

	if (t_success)
		t_success = MCListCopyAsString(*t_prop_list, r_props);

	return t_success;
}

void MCCardlist::addcard(MCCard *card)
{
    // Prune all dead cards from the recent list
    trim();
    
    // If the recent list is not to be updated or this card is already at the
    // head of the list, do nothing.
    if ((cards != NULL && cards->card == card) || MClockrecent)
		return;

	MCCardnode *nptr = new (nothrow) MCCardnode;
	nptr->card = card;
	
	if (cards == NULL)
		first = nptr;
	nptr->insertto(cards);
	if (interval++ > MAX_FILL)
		trim();
}

void MCCardlist::deletecard(MCCard *card)
{
	MCCardnode *tmp = cards;
	Boolean restart;
	if (tmp != NULL)
		do
		{
			restart = False;
			if (!tmp->card.IsValid() || tmp->card == card)
			{
				if (tmp == first)
					first = tmp->next();
				if (tmp == cards)
				{
					MCCardnode *tptr = cards->remove
					                   (cards);
					delete tptr;
					interval--;
					tmp = cards;
					if (cards == NULL)
						break;
					else
						restart = True;
				}
				else
				{
					MCCardnode *tptr = tmp->remove
					                   (tmp);
					delete tptr;
					interval--;
				}
			}
			else
				tmp = tmp->next();
		}
		while (restart || tmp != cards);
}

void MCCardlist::deletestack(MCStack *stack)
{
	MCCardnode *tmp = cards;
	Boolean restart;
	if (tmp != NULL)
		do
		{
			restart = False;
			if (!tmp->card.IsValid() || tmp->card->getstack() == stack)
			{
				if (tmp == first)
					first = tmp->next();
				if (tmp == cards)
				{
					MCCardnode *tptr = cards->remove
					                   (cards);
					delete tptr;
					interval--;
					tmp = cards;
					if (cards == NULL)
						break;
					else
						restart = True;
				}
				else
				{
					MCCardnode *tptr = tmp->remove
					                   (tmp);
					delete tptr;
					interval--;
				}
			}
			else
				tmp = tmp->next();
		}
		while (restart || tmp != cards);
}

void MCCardlist::gorel(int2 offset)
{
	if (cards == NULL)
		return;
	if (offset > 0)
		while(offset--)
			cards = cards->prev();
	else
		while(offset++)
			cards = cards->next();
	MCStack *sptr = cards->card->getstack();
	if (sptr != MCdefaultstackptr && MCdefaultstackptr->hcstack())
		MCdefaultstackptr->close();
	sptr->setcard(cards->card, False, False);
	sptr->openrect(sptr->getrect(), WM_LAST, NULL, WP_DEFAULT, OP_NONE);
	MCdefaultstackptr = sptr;
}

MCCard *MCCardlist::getrel(int2 offset)
{
	if (cards == NULL)
		return NULL;
	MCCardnode *tptr = cards;
	if (offset > 0)
		while(offset--)
			tptr = tptr->prev();
	else
		while(offset++)
			tptr = tptr->next();
	return tptr->card;
}

void MCCardlist::godirect(Boolean start)
{
	if (cards == NULL)
		return;
	if (start)
		cards = first;
	else
		cards = first->next();
	MCStack *sptr = cards->card->getstack();
	if (sptr != MCdefaultstackptr && MCdefaultstackptr->hcstack())
		MCdefaultstackptr->close();
	sptr->setcard(cards->card, False, False);
	sptr->openrect(sptr->getrect(), WM_LAST, NULL, WP_DEFAULT, OP_NONE);
	MCdefaultstackptr = sptr;
}

void MCCardlist::pushcard(MCCard *card)
{
	if (card == NULL)
		return;
	MCCardnode *nptr = new (nothrow) MCCardnode;
	nptr->card = card;
	nptr->insertto(cards);
}

MCCard *MCCardlist::popcard()
{
	if (cards != NULL)
	{
		MCCardnode *tptr = cards->remove
		                   (cards);
		MCCard *card = tptr->card;
		delete tptr;
		return card;
	}
	return MCdispatcher->gethome()->getcurcard();
}

