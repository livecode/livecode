/* Copyright (C) 2003-2017 LiveCode Ltd.

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
#include "mcerror.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "stacklst.h"
#include "variable.h"

#include "globals.h"

#include <algorithm>
#include <type_traits>

/* ---------------------------------------------------------------- */

MCSellist::MCSelnode::MCSelnode() : MCObjectHandle() {}

/* When constructing an MCSelnode with an object, mark the
 * object as internally selected. */
MCSellist::MCSelnode::MCSelnode(MCObjectHandle object)
    : MCObjectHandle(object)
{
    if (IsValid())
        Get()->select();
}

/* Allow move construction of an MCSelnode.  The object's
 * selection state becomes "owned" by the newly-constructed
 * MCSelnode.  The old MCSelnode has its object reference
 * stolen, so it won't deselect the object when it's
 * destroyed. */
MCSellist::MCSelnode::MCSelnode(MCSelnode&& other)
    : MCObjectHandle(std::forward<MCObjectHandle>(other))
{}

/* Allow move assignment, but ensure that object selection
 * status is managed correctly. */
MCSellist::MCSelnode&
MCSellist::MCSelnode::operator=(MCSelnode&& other) {
    if (IsValid()) Get()->deselect();
    MCObjectHandle::operator=(nullptr);
    swap(other);
    return *this;
}

/* When destroying an MCSelnode with an object, mark the
   object as internally deselected. */
MCSellist::MCSelnode::~MCSelnode()
{
    if (IsValid()) Get()->deselect();
}

/* ---------------------------------------------------------------- */

MCSellist::MCSellist()
{
    static_assert(std::is_move_constructible<MCSelnode>::value,
                  "MCSelnode is not move constructible");
    static_assert(std::is_move_assignable<MCSelnode>::value,
                  "MCSelnode is not move assignable");

    static_assert(!std::is_copy_assignable<MCSelnode>::value,
                  "MCSelnode is copy assignable");
    static_assert(!std::is_copy_constructible<MCSelnode>::value,
                  "MCSelnode is copy constructible");
}

MCSellist::~MCSellist()
{
}

MCObjectHandle MCSellist::getfirst()
{
    if (m_objects.empty()) return {};
    return m_objects.front();
}

bool MCSellist::getids(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

    for (const auto& t_node : m_objects)
    {
        if (t_node)
        {
            MCAutoValueRef t_string;
            if (!t_node->names(P_LONG_ID, &t_string))
                return false;
            if (!MCListAppend(*t_list, *t_string))
                return false;
        }
    }

	return MCListCopy(*t_list, r_list);
}

void MCSellist::Clean()
{
    /* Remove any dead objects from the selected list. */
    m_objects.erase(
         std::remove_if(m_objects.begin(), m_objects.end(),
                        [](const MCSelnode& node) { return bool{!node}; }),
         m_objects.end());
}

void MCSellist::clear(Boolean message)
{
	if (locked)
		return;
    
    const MCObjectHandle optr = m_objects.empty() ? MCObjectHandle{} : m_objects.back();

    m_objects.clear();
    
	if (message && optr)
		optr->message(MCM_selected_object_changed);
}

void MCSellist::top(MCObject *objptr)
{
    /* Find the requested object, then rotate its entry to the front
     * of the selected list. */
    const auto t_found =
        std::find(m_objects.begin(), m_objects.end(), objptr);
    if (t_found != m_objects.end())
    {
        std::rotate(m_objects.begin(), t_found, t_found+1);
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

    if (!m_objects.empty() &&
        (objptr->getstack() != m_objects.front()->getstack() ||
         m_objects.front()->gettype() < CT_GROUP))
		clear(False);
    
	if (MCactivefield)
		MCactivefield->unselect(True, True);
    
    /* UNCHECKED */ m_objects.emplace_back(objptr->GetHandle());
    
	if (p_sendmessage)
		objptr->message(MCM_selected_object_changed);
}

void MCSellist::remove(MCObject *objptr, bool p_sendmessage)
{
    /* MCSellist::remove() is called recursively via
     * MCSellist::del(). MCSellist::del() deletes from the end of the
     * selection towards the start, so in that case it's optimal to
     * search from the end of the selection.  In the case where
     * remove() is being called for an arbitrary selected object, it
     * doesn't make any difference whether we're searching from the
     * start or the end.*/
    const auto&& t_found =
        std::find(m_objects.rbegin(), m_objects.rend(), objptr);
    if (t_found != m_objects.rend())
    {
        if (p_sendmessage)
            (*t_found)->message(MCM_selected_object_changed);
        m_objects.erase(t_found.base(), t_found.base()+1);
    }
}

void MCSellist::sort()
{
    // Remove all dead objects from the list before sorting
    Clean();

    if (m_objects.size() < 2)
        return;

    /* Create a list of object handles with their layer numbers. We do
     * this because computing the layer number is relatively
     * expensive. */
    struct SortNode
    {
        SortNode() = default;
        SortNode(MCObject* o, uint2 n) : m_object(o), m_num(n) {}
        MCObject *m_object {};
        uint2 m_num {};
        bool operator<(const SortNode other) const { return m_num < other.m_num; }
    };

    /* UNCHECKED */ std::vector<SortNode> t_items {m_objects.size()};
    MCCard *t_card = m_objects.front()->getcard();

    std::transform(m_objects.begin(), m_objects.end(),
                   t_items.begin(),
                   [&](const MCSelnode& t_node) {
                       uint2 t_num {};
                       t_card->count(CT_LAYER, CT_UNDEFINED,
                                     t_node, t_num, True);
                       return SortNode{t_node.Get(), t_num};
                   });
    m_objects.clear();

    /* Sort the object handles by layer number */
    std::sort(t_items.begin(), t_items.end());

    /* Rebuild the selection in the appropriate order */
    for (const auto& t_sorted : t_items)
        m_objects.emplace_back(MCObjectHandle(t_sorted.m_object));
}

uint32_t MCSellist::count()
{
    return std::count_if(m_objects.begin(), m_objects.end(),
                         [](const MCSelnode& t_node) {
                             return static_cast<bool>(t_node);
                         });
}

MCControl *MCSellist::clone(MCObject *target)
{
    /* Remove any dead objects before trying to clone them & sort
     * what's left */
    sort();

    /* Work through the selection, replacing each selected object's
     * entry with its clone. */
    MCControlHandle t_newtarget;
    std::transform(
        m_objects.begin(), m_objects.end(), m_objects.begin(),
        [&](const MCSelnode& t_node) {
            MCControl* t_clone =
                t_node.GetAs<MCControl>()->clone(True, OP_NONE, false);
            if (t_node == target)
                t_newtarget = t_clone;
            /* Deselect the original object; select the
             * clone. */
            return MCSelnode{MCObjectHandle(t_clone)};
        });

	if (t_newtarget.IsValid())
	{
        t_newtarget->message(MCM_selected_object_changed);
        
        // Note that t_newtarget->Get() can change while executing the above message
        // hence we re-evaluate here.
        return t_newtarget;
	}

	return nullptr;
}

Exec_stat MCSellist::group(uint2 line, uint2 pos, MCGroup*& r_group_ptr)
{
    // Remove dead objects before trying to group them
    Clean();
    
    MCresult->clear(False);
    if (!m_objects.empty() &&
        m_objects.front()->gettype() <= CT_LAST_CONTROL &&
        m_objects.front()->gettype() >= CT_GROUP)
	{
        MCObject *parent = m_objects.front()->getparent();
        for (const auto& t_node : m_objects)
		{
			// MERG-2013-05-07: [[ Bug 10863 ]] If grouping a shared group, throw
			//   an error.
            if (t_node->gettype() == CT_GROUP &&
                t_node.GetAs<MCGroup>()->isshared())
            {
                MCeerror->add(EE_GROUP_NOBG, line, pos);
				return ES_ERROR;
            }
			
			// MERG-2013-05-07: [[ Bug 10863 ]] If the parent of all the objects
			//   isn't the same, throw an error.
            if (t_node->getparent() != parent)
			{
                MCeerror->add(EE_GROUP_DIFFERENTPARENT, line, pos);
				return ES_ERROR;
            }
		}
        
		sort();
		MCControl *controls = NULL;
        for (auto&& t_iter : m_objects)
		{
            /* Steal each object reference from the selected list */
            MCSelnode t_node = std::move(t_iter);

            auto cptr = t_node.GetAs<MCControl>();
            if (parent->gettype() == CT_CARD)
			{
                auto card = MCObjectCast<MCCard>(parent);
				card->removecontrol(cptr, False, True);
				card->getstack()->removecontrol(cptr);
			}
			else
			{
                auto group = MCObjectCast<MCGroup>(parent);
				group->removecontrol(cptr, True);
			}
			cptr->appendto(controls);
		}
        m_objects.clear();

		MCGroup *gptr = nullptr;
		if (MCsavegroupptr == NULL)
			gptr = (MCGroup *)MCtemplategroup->clone(False, OP_NONE, false);
		else
			gptr = (MCGroup *)MCsavegroupptr->remove(MCsavegroupptr);
		gptr->makegroup(controls, parent);

        /* UNCHECKED */ m_objects.emplace_back(
                            MCObjectCast<MCObject>(gptr)->GetHandle());
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
    
    if (m_objects.empty())
        return false;

    // First we construct the pickle of the list of selected objects
    MCPickleContext *t_context;
		
    // AL-2014-02-14: [[ UnicodeFileFormat ]] When pickling for the clipboard, make sure it
    //   includes 2.7, 5.5 and 7.0 stackfile formats.
    t_context = MCObject::startpickling(true);

    // OK-2008-08-06: [[Bug 6794]] - If no objects were copied because the stack was password protected,
    // then don't write anything to the clipboard. Otherwise the MCClipboard function returns "objects",
    // yet the clipboardData["objects"] will be empty.
    bool t_objects_were_copied;
    t_objects_were_copied = false;

    for (const auto& t_node : m_objects)
    {
        if (!t_node -> getstack() -> iskeyed())
            MCresult -> sets("can't cut object (stack is password protected)");
        else
        {
            MCObject::continuepickling(t_context, t_node,
                                       t_node -> getcard() -> getid());
            t_objects_were_copied = true;
        }
    }

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
        if (m_objects.size() == 1 &&
            m_objects.front() -> gettype() == CT_IMAGE)
        {
            const MCSelnode& t_node = m_objects.front();
            // Failure to add the image to the clipboard is ignored as
            // (while sub-optimal), the paste was mostly successful.
            MCAutoDataRef t_data;
            t_node.GetAs<MCImage>()->getclipboardtext(&t_data);
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
            MCStack *sptr = m_objects.front()->getstack();
            /* Because MCObject::del() will call MCSellist::remove(),
             * iterate backwards over the list of objects, making sure
             * that when MCObject::del() is called the object is at
             * the end of the list of selected objects.  See also
             * comments in MCSellist::del(). */
            while (!m_objects.empty())
            {
                MCObjectHandle t_node = m_objects.back();

                // MW-2008-06-12: [[ Bug 6466 ]] Make sure we don't still delete an
                //   object if the stack is protected.
                if (t_node && t_node -> getstack() -> iskeyed())
                {
                    // Because we need to manipulate the object after it has
                    // been 'deleted', take a strong reference to the object
                    // here
                    MCObject* t_obj = t_node.Get();
                        
                    if (t_obj->del(true))
                    {
                        if (t_obj->gettype() == CT_STACK)
                            MCtodestroy -> remove(MCObjectCast<MCStack>(t_obj));
                        t_obj->scheduledelete();
                    }
                }
                /* If the object couldn't be deleted, deselect it
                 * anyway. */
                if (!m_objects.empty() && m_objects.back() == t_node)
                    m_objects.pop_back();
            }
            m_objects.clear();
            sptr->message(MCM_selected_object_changed);
        }
    }
    else
        MCresult -> sets("can't write to clipboard");

    return true;
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
    if (!IsDeletable() || m_objects.empty())
        return False;

    MCundos->freestate();

    MCStack *sptr = m_objects.front()->getstack();
    /* When we delete an object, it removes itself from the selection,
     * by calling MCSellist::remove().  This not only mutates the
     * selection, but sends a message that may cause code to run that
     * further invalidates the selection.  To cope with this,
     * repeatedly delete the last object in the selection (because
     * removing objects from the end of the selection is cheap), until
     * the selection is empty. */
    /* TODO[2017-04-10] Find some way to prevent user code from
     * turning this into an infinite loop.  At the moment, a stack can
     * respond to a deletion-triggered "selected object changed"
     * message by creating a new object on itself and selecting it, ad
     * infinitum. */
    while (!m_objects.empty())
    {
        /* Can't move the object out of the selected list here.  If
         * the object isn't the last item in m_objects when
         * MCObject::del() is called, MCSellist::remove() will have to
         * scan the whole of m_objects rather than getting an
         * immediate hit.  That would balloon the cost of
         * MCSellist::del() from O(N) to O(N^2). */
        MCObjectHandle t_node = m_objects.back();
        if (t_node && t_node->gettype() >= CT_GROUP)
        {
            MCControl *cptr = t_node.GetAs<MCControl>();
            uint2 num = 0;
            cptr->getcard()->count(CT_LAYER, CT_UNDEFINED, cptr, num, True);

            /* UNCHECKED */ Ustruct *us = new (nothrow) Ustruct;
            us->type = UT_DELETE;
            us->ud.layer = num;
            MCundos->savestate(cptr, us);

            cptr->del(true);
        }
        /* Deselect non-deletable selected objects rather than
         * deleting thme. */
        if (!m_objects.empty() && m_objects.back() == t_node)
            m_objects.pop_back();
    }
    sptr->message(MCM_selected_object_changed);
    return True;
}

bool MCSellist::IsDeletable()
{
    Clean();
    return
        std::all_of(m_objects.begin(), m_objects.end(),
                    [](const MCSelnode& t_node) {
                        return t_node->isdeletable(true);
                    });
}

void MCSellist::startmove(int2 x, int2 y, Boolean canclone)
{
    if (m_objects.empty())
        return;
	dropclone = canclone;
	lastx = startx = x;
	lasty = starty = y;
}

void MCSellist::continuemove(int2 x, int2 y)
{
    if (m_objects.empty())
        return;

	int2 dx = lastx - startx;
	int2 dy = lasty - starty;

    auto t_first_valid =
        std::find_if(m_objects.begin(), m_objects.end(),
                     [](const MCSelnode& p_node) {
                         return bool{p_node};
                     });
    for (auto&& t_iter = t_first_valid;
         t_iter != std::end(m_objects);
         ++t_iter)
    {
        if (*t_iter)
		{
			MCControl *cptr = (*t_iter).GetAs<MCControl>();
			MCRectangle trect = cptr->getrect();
			MCRectangle t_startrect = trect;
			t_startrect.x -= dx;
			t_startrect.y -= dy;
			trect.x = t_startrect.x + (x - startx);
			trect.y = t_startrect.y + (y - starty);
			if (t_iter == t_first_valid)
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
	}
	lastx = x;
	lasty = y;
}

Boolean MCSellist::endmove()
{
    if (m_objects.empty())
        return False;
    
	if (startx == lastx && starty == lasty)
		return False;
    
	MCundos->freestate();

    for (const auto& t_node : m_objects)
    {
        if (t_node)
        {
            auto cptr = t_node.GetAs<MCControl>();
            /* UNCHECKED */ auto us = new (nothrow) Ustruct;
            us->type = UT_MOVE;
            us->ud.deltas.x = lastx - startx;
            us->ud.deltas.y = lasty - starty;
            MCundos->savestate(cptr, us);
        }
    }
	return True;
}

void MCSellist::redraw()
{
    // MW-2011-08-19: [[ Layers ]] Invalidate each object.
    for (const auto& t_node : m_objects)
    {
        if (t_node)
        {
            auto cptr = t_node.GetAs<MCControl>();
            if (cptr -> gettype() <= CT_CARD)
                cptr -> getstack() -> dirtyall();
            else
                cptr -> layer_redrawall();
        }
    }
}
