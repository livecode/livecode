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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"
#include "mcutility.h"

#include "stack.h"
#include "card.h"
#include "field.h"
#include "handler.h"
#include "hndlrlst.h"

#include "scriptpt.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"
#include "globals.h"
#include "objectstream.h"
#include "parentscript.h"
#include "dispatch.h"
#include "variable.h"

////////

static uint32_t mchash(uint32_t p_current, const void *p_data, uint32_t p_length)
{
	uint32_t t_hash;
	t_hash = p_current;

	if (p_data != NULL)
	{
		for(uint32_t i = 0; i < p_length; ++i)
		{
			t_hash += ((char *)p_data)[i];
			t_hash += (t_hash << 10);
			t_hash ^= (t_hash >> 6);
		}
	}
	else
	{
		t_hash += (t_hash << 3);
		t_hash ^= (t_hash >> 11);
		t_hash += (t_hash << 15);
	}

	return t_hash;
}

////////

uint32_t MCParentScript::s_table_capacity = 0;
uint32_t MCParentScript::s_table_occupancy = 0;
MCParentScript **MCParentScript::s_table = NULL;

////////

MCParentScriptUse::MCParentScriptUse(MCParentScript *p_parent, MCObject *p_referrer)
{
	// When created the use will be unlinked.
	m_next_use = NULL;
	m_previous_use = NULL;

	// MW-2013-05-30: [[ InheritedPscripts ]] When created the use has no super.
	m_super_use = NULL;

	// Setup the parent and referrer correctly
	m_parent = p_parent;
	m_referrer = p_referrer;

	// Locals variables will be created on first use of the parent script at which
	// point the parent script's object's variable list will be copied.
	m_local_count = 0;
	m_locals = NULL;
}

MCParentScriptUse::~MCParentScriptUse(void)
{
	// Destroy the use's variable list
	ClearVars();

	// MW-2013-05-30: [[ InheritedPscripts ]] Release the super-use chain, if any.
	if (m_super_use != NULL)
		m_super_use -> Release();
}

MCVariable *MCParentScriptUse::GetVariable(uint32_t i)
{
	// If locals have already been allocated for this use, return the approp-
	// riate variable object.
	if (m_locals != NULL)
		return m_locals[i];

	// Fetch the handler list of our parentScript
	MCHandlerlist *t_handlers;
	t_handlers = GetParent() -> GetObject() -> gethandlers();

	// Fetch the initializers for our parentScript
	MCValueRef *t_vinits;
	t_vinits = t_handlers -> getvinits();

	// Fetch the linked list of variables from the handler-list
	MCVariable *t_vars;
	t_vars = t_handlers -> getvars();

	// Allocate the array of variables needed
	m_local_count = t_handlers -> getnvars();
	m_locals = new (nothrow) MCVariable *[m_local_count];

	// Loop through initializing the variables as appropriate
	for(uint32_t j = 0; j < m_local_count; ++j, t_vars = t_vars -> getnext())
	{
		// AL-2013-02-04: [[ Bug 9981 ]] Make sure the variable is created with its name so
		//   it can be watched.
		/* UNCHECKED */ MCVariable::createwithname(t_vars -> getname(), m_locals[j]);
		m_locals[j] -> setvalueref(t_vinits[j] != nil ? t_vinits[j] : kMCNull);
	}

	return m_locals[i];
}

////

void MCParentScriptUse::Release(void)
{
	// First release us from the parent.
	m_parent -> Detach(this);

	// Now delete this object.
	delete this;
}

void MCParentScriptUse::ClearVars(void)
{
	// Now delete all the associated locals
	for(uint32_t i = 0; i < m_local_count; ++i)
		delete m_locals[i];

	// Finally delete the locals array
	delete[] m_locals; /* Allocated with new[] */
	
	m_locals = NULL;
	m_local_count = 0;
}

void MCParentScriptUse::PreserveVars(uint32_t *p_map, MCValueRef *p_new_var_inits, uint32_t p_new_var_count)
{
	// If we don't have any vars then do nothing, since they will be initialized
	// correctly on first use.
	if (m_local_count == 0)
		return;

	// We have some vars so we need to do some remapping. First allocate a new array
	MCVariable **t_new_locals;
	t_new_locals = new (nothrow) MCVariable *[p_new_var_count];
	
	// Initialize it to NULL
	memset(t_new_locals, 0, sizeof(MCVariable *) * p_new_var_count);

	// Now use the map to move across the old variables
	for(uint32_t i = 0; i < m_local_count; ++i)
	{
		if (p_map[i] != 0xffffffff)
		{
			t_new_locals[p_map[i]] = m_locals[i];
			m_locals[i] = NULL;
		}
	}

	// Delete the old locals
	for(uint32_t i = 0; i < m_local_count; ++i)
		if (m_locals[i] != NULL)
			delete m_locals[i];
	delete m_locals;

	// Now copy across any new/initialized vars from p_new_vars.
	for(uint32_t i = 0; i < p_new_var_count; ++i)
	{
		// If the local is not NULL then it means its already there.
		if (t_new_locals[i] != NULL)
			continue;

		// Create a new var
		/* UNCHECKED */ MCVariable::create(t_new_locals[i]);

		// Initialize the variable
		t_new_locals[i] -> setvalueref(p_new_var_inits[i] != nil ? p_new_var_inits[i] : kMCNull);
	}

	m_locals = t_new_locals;
	m_local_count = p_new_var_count;
}

MCParentScriptUse *MCParentScriptUse::Clone(MCObject *p_new_referrer)
{
	// First allocate a new instance
	MCParentScriptUse *t_new_use;
	t_new_use = new (nothrow) MCParentScriptUse(m_parent, p_new_referrer);
	if (t_new_use == NULL)
		return NULL;

	// MW-2013-05-30: [[ InheritedPscripts ]] Clone the super-use chain, if any.
	if (m_super_use != NULL)
	{
		MCParentScriptUse *t_new_super_use;
		t_new_super_use = m_super_use -> Clone(p_new_referrer);
		if (t_new_super_use == NULL)
		{
			// Note we 'delete' the new use, rather than Release as we haven't
			// yet linked it into the parentScript object.
			delete t_new_use;
			return NULL;
		}

		// Set the new use's super use
		t_new_use -> m_super_use = t_new_super_use;
	}

	// Attach this use to the parent, this will initialize the next/previous
	// links correctly.
	m_parent -> Attach(t_new_use);

	return t_new_use;
}

// MW-2013-05-30: [[ InheritedPscripts ]] This method creates the super-use
//   chain for the object.
bool MCParentScriptUse::Inherit(void)
{
	// If this use already has a super_use then release it. This method
	// is called both when first creating the super-use chain, as well
	// as when it needs to be reset (if it changes dynamically).
	if (m_super_use)
	{
		m_super_use -> Release();
		m_super_use = NULL;
	}

	// Get the parentScript's object
	MCObject *t_super_object;
	t_super_object = m_parent -> GetObject();

	// If the parentScript is unresolved (i.e. the object is NULL) then there is
	// nothing to do.
	if (t_super_object == NULL)
		return true;

	// Check to see if the parentScript object has a parentScript
	MCParentScript *t_super_parentscript;
	t_super_parentscript = t_super_object -> getparentscript();

	// If the super-object has no parent-script itself, then do nothing.
	if (t_super_parentscript == NULL)
		return true;

	// Create a new use of the super-object's parentScript
	MCParentScriptUse *t_super_use;
	t_super_use = new (nothrow) MCParentScriptUse(t_super_parentscript, m_referrer);
	if (t_super_use == NULL)
		return false;

	// Recurse to build super-chain for the new use
	if (!t_super_use -> Inherit())
	{
		// Note we 'delete' the new use, rather than Release as we haven't
		// yet linked it into the parentScript object.
		delete t_super_use;
		return false;
	}

	// Finally attach the new use to it's parentScript...
	t_super_parentscript -> Attach(t_super_use);

	// ... And set this use to reference it.
	m_super_use = t_super_use;

	return true;
}

MCParentScript *MCParentScriptUse::GetParent(void) const
{
	return m_parent;
}

MCObject *MCParentScriptUse::GetReferrer(void) const
{
	return m_referrer;
}

// MW-2013-05-30: [[ InheritedPscripts ]] This method returns the super-use
//   (if any).
MCParentScriptUse *MCParentScriptUse::GetSuper(void) const
{
	return m_super_use;
}

////////

MCParentScript::MCParentScript(void)
{
	m_chain = NULL;

	m_object_stack = NULL;

	m_object = NULL;

	m_first_use = NULL;
	m_last_use = NULL;

	m_blocked = false;
}

MCParentScript::~MCParentScript(void)
{
	MCValueRelease(m_object_stack);
}

////

uint32_t MCParentScript::GetObjectId(void) const
{
	return m_object_id;
}

MCNameRef MCParentScript::GetObjectStack(void) const
{
	return m_object_stack;
}

MCObject *MCParentScript::GetObject(void) const
{
	return m_object;
}

bool MCParentScript::IsBlocked(void) const
{
	return m_blocked;
}

void MCParentScript::Resolve(MCObject *p_object)
{
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// If we've already resolved it to the given object, then don't do anything
	if (m_object == p_object)
		return;

	// Assign the reference to the object
	m_object = p_object;

	// Unblock this
	m_blocked = false;

	// Mark the object as being used as a parent script.
	m_object -> setisparentscript(true);

	// Mark the object's stack as having an object which is a parent script.
	MCStack *t_stack;
	t_stack = m_object -> getstack();
	if (!t_stack -> getextendedstate(ECS_HAS_PARENTSCRIPTS))
	{
		t_stack -> setextendedstate(true, ECS_HAS_PARENTSCRIPTS);

		// Finally, mark the object's parent stack as having parent scripts.
		if (!MCdispatcher -> ismainstack(t_stack))
			static_cast<MCStack *>(t_stack -> getparent()) -> setextendedstate(true, ECS_HAS_PARENTSCRIPTS);
	}
}

void MCParentScript::Block(void)
{
	m_blocked = true;
}

void MCParentScript::Flush(void)
{
	// Clear the reference
	m_object = NULL;

	// Iterate through all the uses, clearing out variables
	for(MCParentScriptUse *t_use = m_first_use; t_use != NULL; t_use = t_use -> m_next_use)
		t_use -> ClearVars();
}

bool MCParentScript::CopyUses(MCArrayRef& r_use)
{
    MCAutoArrayRef t_use_list;
    if (!MCArrayCreateMutable(&t_use_list))
        return false;
    
    index_t t_index = 1;
    for(MCParentScriptUse *t_use = m_first_use; t_use != NULL; t_use = t_use -> m_next_use)
    {
        MCAutoValueRef t_object_id;
        if (!(t_use -> GetReferrer() -> names(P_LONG_ID, &t_object_id)) ||
            !MCArrayStoreValueAtIndex(*t_use_list, t_index++, *t_object_id))
            return false;
    }
    
    if (!t_use_list.MakeImmutable())
    {
        return false;
    }
    
    r_use = t_use_list.Take();
    
    return true;
}

// MW-2013-05-30: [[ InheritedPscripts ]] Loop through all uses of this parentScript
//   and ensure the super-use chains are correct.
bool MCParentScript::Reinherit(void)
{
	// Iterate through all the uses, calling inherit on each one to make sure that
	// the super-use chain is updated appropriately.
	for(MCParentScriptUse *t_use = m_first_use; t_use != NULL; t_use = t_use -> m_next_use)
		if (!t_use -> Inherit())
			return false;
			
	return true;
}

////

MCParentScriptUse *MCParentScript::Acquire(MCObject *p_referrer, uint32_t p_id, MCNameRef p_stack)
{
	// Check that the table exists, and if it doesn't attempt to create it.
	if (s_table == NULL)
	{
		s_table = (MCParentScript **)malloc(sizeof(MCParentScript *) * 1024);
		if (s_table == NULL)
			return NULL;

		memset(s_table, 0, sizeof(MCParentScript *) * 1024);

		s_table_occupancy = 0;
		s_table_capacity = 1024;
	}

	// First hash the triple.
	uint32_t t_hash;
	t_hash = Hash(p_id, p_stack);

	// Now compute the index - our hash-table is a power of two in size and so
	// this is a simple masking operation.
	uint32_t t_index;
	t_index = t_hash & (s_table_capacity - 1);

	// Search for entries with the same hash value, and if we find one then check
	// if the triple is the same.
	MCParentScript *t_parent;
	for(t_parent = s_table[t_index]; t_parent != NULL; t_parent = t_parent -> m_chain)
		if (t_parent -> m_hash == t_hash &&
			t_parent -> m_object_id == p_id &&
			MCNameIsEqualToCaseless(t_parent -> m_object_stack, p_stack))
			break;

	// At this point we start a success variable since we are about to have to
	// track the success of memory allocations...
	bool t_success;
	t_success = true;

	// Next ensure that we have a script object - if we didn't find an existing one
	// matching our triple in the hash-table, we construct a new one.
	if (t_parent == NULL)
	{
		// Note that the default constructor for the parent script object just
		// zeros out the fields - we need to track the cloning of the stack
		// and mainstack strings in case they error out.
		t_parent = new (nothrow) MCParentScript;
		if (t_parent == NULL)
			t_success = false;

		if (t_success)
		{
            t_parent->m_object_stack = MCValueRetain(p_stack);
			t_parent -> m_hash = t_hash;
			t_parent -> m_object_id = p_id;
		}
	}

	// We have a parent script object, so next we have to create a use to
	// return and install into the referring object.
	MCParentScriptUse *t_use;
	t_use = NULL;
	if (t_success)
	{
		t_use = new (nothrow) MCParentScriptUse(t_parent, p_referrer);
		if (t_use == NULL)
			t_success = false;
	}

	// Finally link the parent object into the hash-table (if we created
	// it this time round) and attach the new use.
	if (t_success)
	{
		if (t_parent -> m_first_use == NULL)
		{
			// If we have filled the table, try to double it in size. If that's
			// not possible, it doesn't matter - it just means things will be
			// a little less efficient than they could be.
			if (s_table_occupancy == s_table_capacity)
			{
				Grow();

				// Recompute the index which may have changed.
				t_index = t_hash & (s_table_capacity - 1);
			}

			t_parent -> m_chain = s_table[t_index];
			s_table[t_index] = t_parent;

			// Increment the table occupancy.
			s_table_occupancy += 1;
		}

		t_parent -> Attach(t_use);
	}
	else
	{
		// An allocation failed, so delete the parent object if we
		// created it.
		if (t_parent -> m_first_use == NULL)
			delete t_parent;
	}

	return t_use;
}

MCParentScript *MCParentScript::Lookup(MCObject *p_object)
{
	for(uint32_t t_index = 0; t_index < s_table_capacity; ++t_index)
	{
		for(MCParentScript *t_script = s_table[t_index]; t_script != NULL; t_script = t_script -> m_chain)
			if (t_script -> m_object == p_object)
				return t_script;
	}
	
	return nil;
}

void MCParentScript::PreserveVars(MCObject *p_object, uint32_t *p_map, MCValueRef *p_new_var_inits, uint32_t p_new_var_count)
{
	MCParentScript *t_script;
	t_script = Lookup(p_object);
	if (t_script == nil)
		return;

	for(MCParentScriptUse *t_use = t_script -> m_first_use; t_use != NULL; t_use = t_use -> m_next_use)
		t_use -> PreserveVars(p_map, p_new_var_inits, p_new_var_count);
}

void MCParentScript::ClearVars(MCObject *p_object)
{
	MCParentScript *t_script;
	t_script = Lookup(p_object);
	if (t_script == nil)
		return;

	for(MCParentScriptUse *t_use = t_script -> m_first_use; t_use != NULL; t_use = t_use -> m_next_use)
		t_use -> ClearVars();
}

// Remove all references to the given object
void MCParentScript::FlushObject(MCObject *p_object)
{
	MCParentScript *t_script;
	t_script = Lookup(p_object);
	if (t_script == nil)
		return;

	t_script -> Flush();
}

// Remove all references to the given object
void MCParentScript::FlushStack(MCStack *p_stack)
{
	for(uint32_t t_index = 0; t_index < s_table_capacity; ++t_index)
	{
		for(MCParentScript *t_script = s_table[t_index]; t_script != NULL; t_script = t_script -> m_chain)
			if (t_script -> m_object != NULL && t_script -> m_object -> getstack() == p_stack)
				t_script -> Flush();
	}
}

// Insert the given at the front of the doubly-linked use chain. This method
// assumes the given use is not attached anywhere else and should only be
// called from MCParentScriptUse::Clone.
//
void MCParentScript::Attach(MCParentScriptUse *p_use)
{
	p_use -> m_next_use = m_first_use;
	p_use -> m_previous_use = NULL;

	if (m_first_use != NULL)
		m_first_use -> m_previous_use = p_use;
	else
		m_last_use = p_use;
	
	m_first_use = p_use;
}

// Remove the given use from the doubly-linked use chain. If there are no
// uses left, we delete the parent script object.
//
void MCParentScript::Detach(MCParentScriptUse *p_use)
{
	// Unlink after
	if (p_use -> m_next_use != NULL)
		p_use -> m_next_use -> m_previous_use = p_use -> m_previous_use;
	else
		m_last_use = p_use -> m_previous_use;

	// Unlink before
	if (p_use -> m_previous_use != NULL)
		p_use -> m_previous_use -> m_next_use = p_use -> m_next_use;
	else
		m_first_use = p_use -> m_next_use;

	// If none left delete this
	if (m_first_use == NULL)
	{
		// First remove any references including the hash chain.
		Remove();

		// MW-2009-01-28: [[ Inherited parentScripts ]]
		// Unset the object's IS_PARENTSCRIPT state as it is no longer being used as
		// one.
		if (m_object != NULL)
			m_object -> setisparentscript(false);

		// Now delete our state
		delete this;
	}
}


////

void MCParentScript::Remove(void)
{
	// Now look for this link in the hash-chain and remove.
	uint32_t t_index;
	t_index = m_hash & (s_table_capacity - 1);

	if (s_table[t_index] == this)
		s_table[t_index] = m_chain;
	else
	{
		for(MCParentScript *t_link = s_table[t_index]; t_link -> m_chain != NULL; t_link = t_link -> m_chain)
			if (t_link -> m_chain == this)
			{
				t_link -> m_chain = m_chain;
				break;
			}
	}

	// Reduce the table occupancy by one.
	s_table_occupancy -= 1;

	// Finally check to see if we should shrink the hash-table a bit.
	if (s_table_capacity > 1024 && s_table_occupancy * 16 / s_table_capacity < 5)
		Shrink();
}

////

uint32_t MCParentScript::Hash(uint32_t p_value, MCNameRef p_stack)
{
	uint32_t t_hash;
	t_hash = mchash(0, &p_value, sizeof(uint32_t));
	
	uintptr_t t_stack_key;
	t_stack_key = MCNameGetCaselessSearchKey(p_stack);
	t_hash = mchash(t_hash, &t_stack_key, sizeof(uintptr_t));

	t_hash = mchash(t_hash, NULL, 0);
	return t_hash;
}

// This method attempts to double the size of the hash table. If this fails it
// leaves no side-effects. Indeed, in this case there is no issue, it just means
// we are guaranteed to start getting chains of length > 1.
void MCParentScript::Grow(void)
{
	// First attempt to resize the table array to twice the size.
	uint32_t t_new_capacity;
	t_new_capacity = s_table_capacity * 2;

	MCParentScript **t_new_table;
	t_new_table = (MCParentScript **)realloc(s_table, sizeof(MCParentScript *) * t_new_capacity);
	if (t_new_table == NULL)
		return;

	// Now zero out the new half of the table.
	memset(t_new_table + s_table_capacity, 0, s_table_capacity * sizeof(MCParentScript *));

	// Update the table variables.
	s_table = t_new_table;
	s_table_capacity = t_new_capacity;

	// Relocate the chains as necessary
	Relocate(0, s_table_capacity / 2, s_table_capacity);
}

// This method shrinks the size of the table to half its former size. This is
// done when the occupancy falls to a fraction of the capacity.
void MCParentScript::Shrink(void)
{
	// First relocate any chains in the top half to the bottom half.
	Relocate(s_table_capacity / 2, s_table_capacity, s_table_capacity / 2);

	// Now resize the array
	s_table_capacity /= 2;
	s_table = (MCParentScript **)realloc(s_table, sizeof(MCParentScript *) * s_table_capacity);
}

// This method relocates chains in the hash-table to their new positions
// after a change in capacity.
void MCParentScript::Relocate(uint32_t p_start, uint32_t p_finish, uint32_t p_new_capacity)
{
	// Iterate through all the entries in the table, moving them as appropriate.
	// Note that due to the way the table is grown and shrunk, we will never
	// process an entry twice during this traversal, since if an entry does
	// move, it will do so to outside the range we are scanning.
	for(uint32_t t_old_index = p_start; t_old_index < p_finish; ++t_old_index)
	{
		MCParentScript *t_previous;
		t_previous = NULL;

		MCParentScript *t_parent;
		t_parent = s_table[t_old_index];

		while(t_parent != NULL)
		{
			// Record the next link
			MCParentScript *t_next;
			t_next = t_parent -> m_chain;

			uint32_t t_new_index;
			t_new_index = t_parent -> m_hash & (p_new_capacity - 1);

			// If the new index is different from the old move the bucket.
			if (t_new_index != t_old_index)
			{
				// Remove the link from the previous bucket, or the table
				// at the old index.
				if (t_previous != NULL)
					t_previous -> m_chain = t_next;
				else
					s_table[t_old_index] = t_next;

				// Push the bucket onto the front of the table at the new
				// index.
				t_parent -> m_chain = s_table[t_new_index];
				s_table[t_new_index] = t_parent;

				// Note previous doesn't change in this case.
			}
			else
				t_previous = t_parent;

			// Move to the next use.
			t_parent = t_next;
		}
	}
}

// Cleanup the resources used by the parentscript system
void MCParentScript::Cleanup(void)
{
	free(s_table);
    // IM-2011-12-09: fix crash on android by resetting statically initialized values
    s_table_capacity = 0;
    s_table_occupancy = 0;
    s_table = NULL;
}
