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

#ifndef __MC_PARENT_SCRIPT__
#define __MC_PARENT_SCRIPT__

// The MCParentScriptUse object represents the use of a given parent script by
// a control. The control using the parent script is called the referrer, and
// the control containing the parent script, the parent.
//
// The use objects form a doubly-linked list, anchored in the associated parent
// script object. This allows, for example, preservation of variables to act
// correctly when the parent script is recompiled.
//
// The use chain will also be useful for IDE level operations such as
// reassociating control's linked to one parentScript to another.
//
class MCParentScriptUse
{
public:
	// Release the reference to the parent script, this will have the side-
	// effect of deleting the underlying parent script object if there are no
	// other references.
	void Release(void);

	// Clone the use of the parent script - this is used when a control is
	// duplicated.
	MCParentScriptUse *Clone(MCObject *p_new_referrer);

	// Construct the super-use chain for this use - this makes a chain of
	// parentScript uses, based on the values of the chain of parentScript
	// properties. If there is not enough memory to complete the operation
	// it returns false and the object is unchanged.
	bool Inherit(void);

	// MW-2013-05-30: [[ InheritedPscripts ]] Return the super-use (if any).
	MCParentScriptUse *GetSuper(void) const;

	// Return the parent script
	MCParentScript *GetParent(void) const;

	// Get the referring control this use refers to
	MCObject *GetReferrer(void) const;

	// Ensure variables are allocated and return the variable at the given index
	MCVariable *GetVariable(uint32_t i);

	// Clear all the existing variables.
	void ClearVars(void);

	// Remap all the existing variables.
	void PreserveVars(uint32_t *p_map, MCValueRef *p_new_var_inits, uint32_t p_new_var_count);

private:
	MCParentScriptUse(MCParentScript *p_parent, MCObject *p_referrer);
	~MCParentScriptUse(void);

	// Chain links for list of all uses for the associated parent script object.
	MCParentScriptUse *m_next_use;
	MCParentScriptUse *m_previous_use;

	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// The parentScript's super-parentScript.
	MCParentScriptUse *m_super_use;

	// The associated parent script object.
	MCParentScript *m_parent;

	// The object using the parent script.
	MCObject *m_referrer;

	// The list and count of script locals for this reference
	uint32_t m_local_count;
	MCVariable **m_locals;

	friend class MCParentScript;
};

// The MCParentScript object represents a parent script that is currently being
// used by some control in the environment. Parent script objects are singletons
// keyed to a triple (id, stack, mainstack) corresponding to a unique control in
// the environment. A hash-table is used to maintain this mapping.
//
// A parent script object contains a weak-reference to the control whose script
// is to be used. This reference is broken when one of the following occurs:
//   1) The resolved control is deleted
//   2) The resolved control's id changes
//   3) The name of the resolved control's stack changes
//   4) The name of the resolved control's main stack changes.
//
// A parent script object resolves to a control by searching for the control
// matching its unique triple. This occurs at one of two points:
//   1) When a control has its parentScript property set.
//   2) Immediately after a stackfile has been loaded, when all controls are
//      iterated through to attempt to resolve references.
//
// Removing of the resolved control pointer is handled by setting of boolean
// flags on the resolved control, its stack and mainstack. This way the table
// only has to be searched when necessary as opposed to after every deletion,
// id change or name change operation.
//
class MCParentScript
{
public:
	// Acquire a new reference to a parent script object. This will look to see
	// if one exists and if not, will create one.
	static MCParentScriptUse *Acquire(MCObject *p_referrer, uint32_t p_id, MCNameRef p_stack);
	
	// Attempt to locate an existing parent script based on the given object.
	static MCParentScript *Lookup(MCObject *p_object);

	// Flush the reference to the given object since it is no longer valid.
	static void FlushObject(MCObject *p_object);

	// Flush all references to objects on the given stack since its name has
	// changed or ownership has changed.
	static void FlushStack(MCStack *p_stack);

	// Remap all the existing variables for each use using the given map.
	static void PreserveVars(MCObject *p_object, uint32_t *p_map, MCValueRef *p_new_var_inits, uint32_t p_new_var_count);

	// Clear all the existing variables for each use.
	static void ClearVars(MCObject *p_object);

	// Cleanup the parentscript stuff (called on exit)
	static void Cleanup(void);

	// Assign the given control as the parent-scripts object.
	void Resolve(MCObject *p_object);

	// Mark the parent script is unresolvable (for now). The block is lifted
	// if 'Resolve' is later called as a result of setting the parentScript
	// of an object.
	void Block(void);

	// Remove the reference and delete any per use vars
	void Flush(void);

	// Release the given use. If the parent script has no other use's it will
	// be deleted.
	void Detach(MCParentScriptUse *p_use);

	// Attach the given use to this parent script - this is used when a use is
	// cloned or when a parent script object is acquired.
	void Attach(MCParentScriptUse *p_use);
	
	// MW-2013-05-30: [[ InheritedPscripts ]] Rework the super-use chains of each use.
	bool Reinherit(void);
	
	// 

	// Return the id of the parent script's control
	uint32_t GetObjectId(void) const;

	// Return stack of the parent script's control
	MCNameRef GetObjectStack(void) const;

	// Return the resolved object reference (will be NULL if the reference
	// is unresolved).
	MCObject *GetObject(void) const;

	// Return whether the parent script is blocked from resolution.
	bool IsBlocked(void) const;
    
    bool CopyUses(MCArrayRef& r_use);

private:
	MCParentScript(void);
	~MCParentScript(void);

	// Remove this parent script from the table.
	void Remove(void);

	////////

	// Chain link for hash-table
	MCParentScript *m_chain;

	// Hash value of the (id, stack, mainstack) triple.
	uint32_t m_hash;

	// Id of control whose script is to be used
	uint32_t m_object_id;
	MCNameRef m_object_stack;

	// Currently resolved control, if this is NULL it means that the script
	// hasn't been resolved yet.
	MCObject *m_object;

	// If this flag is true, it means that resolution has failed in the
	// past and should not be attempted again indirectly.
	bool m_blocked;

	// Doubly-linked list of all uses parent script
	MCParentScriptUse *m_first_use;
	MCParentScriptUse *m_last_use;

	////////

	// Compute the hash value of the given triple
	static uint32_t Hash(uint32_t p_id, MCNameRef p_stack);

	// Grow the hash-table by doubling its size.
	static void Grow(void);

	// Shrink the hash-table by halving its size.
	static void Shrink(void);

	// Relocate entries in the hash table after a resize.
	static void Relocate(uint32_t p_start, uint32_t p_finish, uint32_t p_new_capacity);

	// Static variables for parent script hash-table
	static MCParentScript **s_table;
	static uint32_t s_table_occupancy;
	static uint32_t s_table_capacity;
};

#endif
