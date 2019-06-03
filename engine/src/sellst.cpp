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


#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "field.h"
#include "paragraf.h"
#include "image.h"
#include "widget.h"
#include "mcerror.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "stacklst.h"
#include "variable.h"

#include "globals.h"


class MCSellist::MCSelnode : public MCDLlist
{
public:
    
    MCSelnode(MCObjectHandle object);
    ~MCSelnode();
    
    MCDLlistAdaptorFunctions(MCSelnode);
    
    MCObjectHandle m_ref;
};


MCSellist::MCSelnode::MCSelnode(MCObjectHandle object) :
  m_ref(object)
{
	if (m_ref)
		m_ref->select();
}

MCSellist::MCSelnode::~MCSelnode()
{
	if (m_ref)
		m_ref->deselect();
}

MCSellist::MCSellist() :
  m_owner(nil),
  objects(nil),
  curobject(nil),
  startx(0), starty(0),
  lastx(0), lasty(0),
  locked(false),
  dropclone(false)
{
}

MCSellist::~MCSellist()
{
	clear(False);
}

MCObjectHandle MCSellist::getfirst()
{
	if (objects != nil)
	{
		curobject = objects;
		return curobject->m_ref;
	}
	else
		return nil;
}

bool MCSellist::getids(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	if (objects != nil)
	{
		MCSelnode *tptr = objects;
		do
		{
			MCAutoValueRef t_string;
            if (tptr->m_ref)
            {
                if (!tptr->m_ref->names(P_LONG_ID, &t_string))
                    return false;
                if (!MCListAppend(*t_list, *t_string))
                    return false;
            }
			tptr = tptr->next();
		}
		while (tptr != objects);
	}

	return MCListCopy(*t_list, r_list);
}

void MCSellist::Clean()
{
	if (objects == nil)
		return;

	// Remove any dead objects from the selected list
	MCSelnode* t_cursor = objects;
	bool t_continue = true;

	while (t_continue && objects != nil)
	{
		// If the next object wraps around then we've reached the end of the list
		MCSelnode* t_next = t_cursor->next();
		t_continue = t_next != objects;
		if (!t_cursor->m_ref)
		{
			t_cursor->remove(objects);
			delete t_cursor;
		}

		t_cursor = t_next;
	}
}

void MCSellist::clear(Boolean message)
{
	if (locked)
		return;
    
	MCObjectHandle optr = nil;
	while (objects != nil)
	{
		MCSelnode *nodeptr = objects->remove(objects);
		optr = nodeptr->m_ref;
		delete nodeptr;
	}
    
	if (message && optr)
		optr->message(MCM_selected_object_changed);
}

void MCSellist::top(MCObject *objptr)
{
	Clean();
    
    if (objects != nil)
	{
		MCSelnode *tptr = objects;
		do
		{
			if (tptr->m_ref == objptr)
			{
				tptr->remove(objects);
				break;
			}
			tptr = tptr->next();
		}
		while (tptr != objects);
		tptr->insertto(objects);
	}
}

void MCSellist::replace(MCObject *objptr)
{
	clear(False);
	add(objptr);
}

void MCSellist::add(MCObject *objptr, bool p_sendmessage)
{
    // Ensure that the top object in the list isn't dead as we need to check its
    // type before adding a new object
    Clean();
    
    if (objects != NULL && (objptr->getstack() != objects->m_ref->getstack()
	                        || objects->m_ref->gettype() < CT_GROUP))
		clear(False);
    
	if (MCactivefield)
		MCactivefield->unselect(True, True);
    
	MCSelnode *nodeptr = new (nothrow) MCSelnode(objptr->GetHandle());

	nodeptr->appendto(objects);
    
	if (p_sendmessage)
		objptr->message(MCM_selected_object_changed);
}

void MCSellist::remove(MCObject *objptr, bool p_sendmessage)
{
    // Remove any dead objects before removing objptr
    Clean();

	if (objects != NULL)
	{
		MCSelnode *tptr = objects;
		do
		{
			if (tptr->m_ref == objptr)
			{
				tptr->remove(objects);
				if (p_sendmessage)
					tptr->m_ref->message(MCM_selected_object_changed);
				delete tptr;
				return;
			}
			tptr = tptr->next();
		}
		while (tptr != objects);
	}
}

void MCSellist::sort()
{
    // Remove all dead objects from the list before sorting
    Clean();
    
    MCSelnode *optr = objects;
	MCAutoArrayZeroedNonPod<MCSortnode> items;
	uint4 nitems = 0;
	MCCard *cptr = optr->m_ref->getcard();
	do
	{
		items.Extend(nitems + 1);
		items[nitems].data = (void *)optr;
		uint2 num = 0;
		cptr->count(CT_LAYER, CT_UNDEFINED, optr->m_ref, num, True);
		/* UNCHECKED */ MCNumberCreateWithUnsignedInteger(num, items[nitems].nvalue);
		nitems++;
		optr = optr->next();
	}
	while (optr != objects);
	if (nitems > 1)
	{
        extern void MCStringsSort(MCSortnode *p_items, uint4 nitems, Sort_type p_dir, Sort_type p_form, MCStringOptions p_options);
		MCStringsSort(items.Ptr(), nitems, ST_ASCENDING, ST_NUMERIC, kMCStringOptionCompareExact);
		uint4 i;
		MCSelnode *newobjects = NULL;
		for (i = 0 ; i < nitems ; i++)
		{
			optr = (MCSelnode *)items[i].data;
			optr->remove(objects);
			optr->appendto(newobjects);
		}
		objects = newobjects;
	}
}

uint32_t MCSellist::count()
{
	uint32_t t_count = 0;
	MCSelnode *t_node = objects;
	if (t_node != nil)
	{
		do
		{
            // Only count non-zombie objects
            if (t_node->m_ref)
                t_count++;
            
			t_node = t_node->next();
		} while (t_node != objects);
	}

	return t_count;
}

MCControl *MCSellist::clone(MCObject *target)
{
    // Remove any dead objects before trying to clone them
    Clean();
    
    MCObjectHandle *t_selobj_handles = nil;
	uint32_t t_selobj_count;
	t_selobj_count = count();

	/* UNCHECKED */ MCMemoryNewArrayInit(t_selobj_count, t_selobj_handles);
	sort();

	MCSelnode *t_node = objects;
	for (uint32_t i = 0; i < t_selobj_count; i++)
	{
		t_selobj_handles[i] = t_node->m_ref;
		t_node = t_node->next();
	}

	clear(false);

	MCControlHandle t_newtarget = nil;
	for (uint32_t i = 0; i < t_selobj_count; i++)
	{
		if (t_selobj_handles[i].IsValid())
		{
			MCControl *t_control = t_selobj_handles[i].GetAs<MCControl>();
			MCControl *t_clone = t_control->clone(True, OP_NONE, false);
			t_clone->select();
			if (t_control == target)
				t_newtarget = t_clone->GetHandle();
			t_control->deselect();
			add(t_clone, false);
		}
	}
	
	MCControl *t_result;
	t_result = nil;
	if (t_newtarget.IsValid())
	{
        t_newtarget->message(MCM_selected_object_changed);
        
        // Note that t_newtarget->Get() can change while executing the above message
        // hence we re-evaluate here.
        t_result = t_newtarget;
	}
	
	// MW-2010-05-06: Make sure we clean up the temp array
	MCMemoryDeleteArray(t_selobj_handles, t_selobj_count);

	return t_result;
}

Exec_stat MCSellist::group(uint2 line, uint2 pos, MCGroup*& r_group_ptr)
{
    // Remove dead objects before trying to group them
    Clean();
    
    MCresult->clear(False);
	if (objects != nil && objects->m_ref->gettype() <= CT_LAST_CONTROL
	        && objects->m_ref->gettype() >= CT_GROUP)
	{
		MCObject *parent = objects->m_ref->getparent();
		MCSelnode *tptr = objects;
		do
		{
			// MERG-2013-05-07: [[ Bug 10863 ]] If grouping a shared group, throw
			//   an error.
            if (tptr->m_ref->gettype() == CT_GROUP && tptr->m_ref.GetAs<MCGroup>()->isshared())
            {
                MCeerror->add(EE_GROUP_NOBG, line, pos);
				return ES_ERROR;
            }
			
			// MERG-2013-05-07: [[ Bug 10863 ]] If the parent of all the objects
			//   isn't the same, throw an error.
			if (tptr->m_ref->getparent() != parent)
			{
                MCeerror->add(EE_GROUP_DIFFERENTPARENT, line, pos);
				return ES_ERROR;
            }
			
			tptr = tptr->next();
		}
		while (tptr != objects);
        
		sort();
		MCControl *controls = NULL;
		while (objects != NULL)
		{
			tptr = objects->remove(objects);
			MCControl *cptr = tptr->m_ref.GetAs<MCControl>();
			delete tptr;
			if (parent->gettype() == CT_CARD)
			{
				MCCard *card = (MCCard *)parent;
				card->removecontrol(cptr, False, True);
				card->getstack()->removecontrol(cptr);
			}
			else
			{
				MCGroup *group = (MCGroup *)parent;
				group->removecontrol(cptr, True);
			}
			cptr->appendto(controls);
		}
		MCGroup *gptr;
		if (MCsavegroupptr == NULL)
			gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		else
			gptr = (MCGroup *)MCsavegroupptr->remove(MCsavegroupptr);
		gptr->makegroup(controls, parent);

		objects = new (nothrow) MCSelnode(MCObjectHandle(gptr));
		gptr->message(MCM_selected_object_changed);
		
		r_group_ptr = gptr;
	}
	else
	{
		r_group_ptr = nil;
	}
	
    return ES_NORMAL;
}

bool MCSellist::clipboard(bool p_is_cut)
{
    // Remove any dead objects before trying to put them on the clipboard
    Clean();
    
    if (objects != NULL)
	{
		// First we construct the pickle of the list of selected objects
		MCPickleContext *t_context;
		
        // AL-2014-02-14: [[ UnicodeFileFormat ]] When pickling for the clipboard, make sure it
        //   includes 2.7, 5.5 and 7.0 stackfile formats.
		t_context = MCObject::startpickling(true);

		MCSelnode *t_node;
		t_node = objects;

		// OK-2008-08-06: [[Bug 6794]] - If no objects were copied because the stack was password protected,
		// then don't write anything to the clipboard. Otherwise the MCClipboard function returns "objects",
		// yet the clipboardData["objects"] will be empty.
		bool t_objects_were_copied;
		t_objects_were_copied = false;

		do
		{
			if (!t_node -> m_ref -> getstack() -> iskeyed())
				MCresult -> sets("can't cut object (stack is password protected)");
			else
			{
				MCObject::continuepickling(t_context, t_node -> m_ref, t_node -> m_ref -> getcard() -> getid());
				t_objects_were_copied = true;
			}

			t_node = t_node -> next();
		}
		while(t_node != objects);

		bool t_success;
		t_success = true;

		MCAutoDataRef t_pickle;
		MCObject::stoppickling(t_context, &t_pickle);
		if (*t_pickle == nil)
			t_success = false;

		// OK-2008-08-06: [[Bug 6794]] - Return here before writing to the clipboard to preserve the message in the result.
		if (!t_objects_were_copied)
			return false;

		// Now attempt to write it to the clipboard
        if (!MCclipboard->Lock())
            return false;

        // Clear the current contents of the clipboard
        MCclipboard->Clear();
        
        // Add the serialised objects to the clipboard
        if (t_success)
            t_success = MCclipboard->AddLiveCodeObjects(*t_pickle);
    
		// If we are pasting just one object and it is an image, add it to the
        // clipboard as an image, too, so that other applications can paste it.
		if (t_success)
			if (objects == objects -> next() && objects -> m_ref -> gettype() == CT_IMAGE)
			{
                // Failure to add the image to the clipboard is ignored as
                // (while sub-optimal), the paste was mostly successful.
                MCAutoDataRef t_data;
                objects->m_ref.GetAs<MCImage>()->getclipboardtext(&t_data);
				if (*t_data != nil)
                    MCclipboard->AddImage(*t_data);
			}
		
        // Ensure out changes to the clipboard get pushed out to the OS
        MCclipboard->Unlock();

		// If we succeeded remove the objects if its a cut operation
		if (t_success)
		{
			if (p_is_cut)
			{
				MCStack *sptr = objects->m_ref->getstack();
				while (objects != NULL)
				{
					MCSelnode *tptr = objects->remove(objects);
					
					// MW-2008-06-12: [[ Bug 6466 ]] Make sure we don't still delete an
					//   object if the stack is protected.
					if (tptr -> m_ref -> getstack() -> iskeyed())
					{
                        // Because we need to manipulate the object after it has
                        // been 'deleted', take a strong reference to the object
                        // here
                        MCObject* t_obj = tptr->m_ref;
                        
                        if (t_obj->del(true))
                        {
                            if (t_obj->gettype() == CT_STACK)
                                MCtodestroy -> remove(MCObjectCast<MCStack>(t_obj));
							t_obj->scheduledelete();
                        }
					}

					delete tptr;
				}
				sptr->message(MCM_selected_object_changed);
			}
		}
		else
			MCresult -> sets("can't write to clipboard");

		return true;
	}

	return false;
}

Boolean MCSellist::usercopy()
{
    MCerrorlock++;
    Boolean t_stat = clipboard(false);
    MCerrorlock--;
    return t_stat;
}

Boolean MCSellist::usercut()
{
    MCerrorlock++;
    Boolean t_stat = clipboard(true);
    MCerrorlock--;
    return t_stat;
}

Boolean MCSellist::userdel()
{
    MCerrorlock++;
    Boolean t_stat = del();
    MCerrorlock--;
    return t_stat;
}

Boolean MCSellist::copy()
{
	return clipboard(false);
}

Boolean MCSellist::cut()
{
	return clipboard(true);
}

Boolean MCSellist::del()
{
    if (!IsDeletable())
        return False;
 
	if (nullptr == objects)
        return False;

    MCundos->freestate();
    
    MCStack *sptr = objects->m_ref->getstack();
    while (objects != NULL)
    {
        MCSelnode *tptr = objects->remove(objects);
        if (tptr->m_ref->gettype() >= CT_GROUP)
        {
            MCControl *cptr = tptr->m_ref.GetAs<MCControl>();
            uint2 num = 0;
            cptr->getcard()->count(CT_LAYER, CT_UNDEFINED, cptr, num, True);
            
            bool t_del;
            if (cptr->gettype() == CT_WIDGET)
            {
                t_del = static_cast<MCWidget*>(cptr)->delforundo(true);
            }
            else
            {
                t_del = cptr->del(true);
            }

            if (t_del)
            {
                Ustruct *us = new (nothrow) Ustruct;
                us->type = UT_DELETE;
                us->ud.layer = num;
                MCundos->savestate(cptr, us);
                
                tptr->m_ref = nil;
            }
            
            /* No scheduledelete() as the undo record now owns it */
        }
        delete tptr;
    }
    sptr->message(MCM_selected_object_changed);
    return True;
}

bool MCSellist::IsDeletable()
{
    if (nullptr == objects)
        return false;
    
    // Clear all deleted objects first
    Clean();

    MCSelnode *t_object = objects;
    do
    {
        if (!t_object->m_ref->isdeletable(true))
            return false;
        
        t_object = t_object->next();
    }
    while (t_object != objects);

    return true;
}

void MCSellist::startmove(int2 x, int2 y, Boolean canclone)
{
	if (objects == NULL)
		return;
	dropclone = canclone;
	lastx = startx = x;
	lasty = starty = y;
}

void MCSellist::continuemove(int2 x, int2 y)
{
	if (objects == NULL)
		return;

	MCSelnode *tptr = objects;
	int2 dx = lastx - startx;
	int2 dy = lasty - starty;
	do
	{
		if (tptr->m_ref)
		{
			MCControl *cptr = tptr->m_ref.GetAs<MCControl>();
			MCRectangle trect = cptr->getrect();
			MCRectangle t_startrect = trect;
			t_startrect.x -= dx;
			t_startrect.y -= dy;
			trect.x = t_startrect.x + (x - startx);
			trect.y = t_startrect.y + (y - starty);
			if (tptr == objects)
			{
				MCU_snap(trect.x);
				MCU_snap(trect.y);
				if (dropclone && (trect.x != cptr->getrect().x
								|| trect.y != cptr->getrect().y))
				{
					dropclone = False;
					MCControl *control = clone(cptr);
					control->setflag(False, F_LOCK_LOCATION);
					control->mfocustake(control);
					cptr->setstate(False, CS_MOVE);
					cptr->setstate(False, CS_MFOCUSED);
					control->setstate(True, CS_MFOCUSED);
					control->setstate(True, CS_MOVE);
					continuemove(x, y);
					return;
				}
				x = startx + (trect.x - t_startrect.x);
				y = starty + (trect.y - t_startrect.y);
			}
			if (cptr->moveable())
			{
				// IM-2016-09-27: [[ Bug 17779 ]] Change to always calling layer_setrect, which invalidates selection handles if required.
				cptr->layer_setrect(trect, false);
				cptr->resizeparent();
			}
		}
		tptr = tptr->next();
	}
	while (tptr != objects);
	lastx = x;
	lasty = y;
}

Boolean MCSellist::endmove()
{
	if (objects == NULL)
		return False;
    
	if (startx == lastx && starty == lasty)
		return False;
    
	MCundos->freestate();
	MCSelnode *tptr = objects;
	do
	{
		if (tptr->m_ref)
		{
			MCControl *cptr = tptr->m_ref.GetAs<MCControl>();
			Ustruct *us = new (nothrow) Ustruct;
			us->type = UT_MOVE;
			us->ud.deltas.x = lastx - startx;
			us->ud.deltas.y = lasty - starty;
			MCundos->savestate(cptr, us);
		}
		tptr = tptr->next();
	}
	while (tptr != objects);
	return True;
}

void MCSellist::redraw()
{
	if (objects == NULL)
		return;

	// MW-2011-08-19: [[ Layers ]] Invalidate each object.
	MCSelnode *tptr = objects;
	do
	{
		if (tptr->m_ref)
		{
			MCControl *cptr = tptr->m_ref.GetAs<MCControl>();

			// MW-2012-09-19: [[ Bug 10182 ]] Cards and stacks can be in the list
			//   so make sure we treat those differently from controls!
			if (cptr -> gettype() <= CT_CARD)
				cptr -> getstack() -> dirtyall();
			else
			cptr -> layer_redrawall();
		}

		tptr = tptr->next();
	}
	while (tptr != objects);
}
