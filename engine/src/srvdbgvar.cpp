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

//#include "execpt.h"
#include "globals.h"
#include "debug.h"
#include "mcerror.h"
#include "variable.h"
#include "handler.h"
#include "hndlrlst.h"

#include "srvscript.h"
#include "srvdebug.h"

////////////////////////////////////////////////////////////////////////////////

template<typename T> inline bool MCDebugAllocate(uint32_t p_size, T*& r_block)
{
	T *t_block;
	t_block = (T *)malloc(p_size);
	if (t_block != NULL)
	{
		r_block = t_block;
		return true;
	}	
	return false;
}

template<typename T> inline bool MCDebugReallocate(T* p_block, uint32_t p_new_size, T*& r_new_block)
{
	T *t_new_block;
	t_new_block = (T *)realloc(p_block, p_new_size);
	if (t_new_block != NULL)
	{
		r_new_block = t_new_block;
		return true;
	}
	return false;
}
		
void MCDebugDeallocate(void *p_block)
{
	free(p_block);
}

////////////////////////////////////////////////////////////////////////////////
//
//  VARIABLE VIEW METHODS
//

enum MCVariableScope
{
	kMCVariableScopeElement,
	kMCVariableScopeLocal,
	kMCVariableScopeParameter,
	kMCVariableScopeScript,
	kMCVariableScopeGlobal
};

struct MCVariableInfo
{
	MCVariableScope scope : 3;
	bool is_expanded : 1;
	unsigned variable_count : 12;
	unsigned depth : 16;
	
	MCVariableInfo *owner;
	MCVariableInfo **variables;
	union
	{
		MCVariable *variable;
		MCHashentry *element;
		void *thing;
	};
};

struct MCVariableEntry
{
	MCVariableInfo *owner;
	MCHashentry *element;
};

struct MCVariableView
{
	MCVariableInfo **variables;
	uint32_t variable_count;
	
	// This is true if the view must be rebuilt
	bool view_changed;
	
	// The total number of lines in the view.
	uint32_t view_length;
	
	// The flat list of entries for the view
	MCVariableEntry *view;
};

////////////////////////////////////////////////////////////////////////////////

static bool MCVariableInfoCreate(MCVariableInfo *p_parent, MCVariableScope p_scope, void *p_thing, MCVariableInfo*& r_info)
{
	MCVariableInfo *self;
	if (!MCDebugAllocate(sizeof(MCVariableInfo), self))
		return false;

	self -> scope = p_scope;
	self -> is_expanded = false;
	self -> variables = NULL;
	self -> variable_count = 0;
	self -> owner = p_parent;
	self -> thing = p_thing;
	self -> depth = self -> owner != NULL ? self -> owner -> depth + 1 : 0;
	
	if (self -> scope == kMCVariableScopeElement)
		self -> element -> value . set_dbg_notify(true);
	else
		self -> variable -> getvalue() . set_dbg_notify(true);
	
	r_info = self;
	
	return true;
}

static void MCVariableInfoDestroy(MCVariableInfo *self)
{
	for(uint32_t i = 0; i < self -> variable_count; i++)
		MCVariableInfoDestroy(self -> variables[i]);
	
	MCDebugDeallocate(self -> variables);
	
	if (self -> scope == kMCVariableScopeElement)
		self -> element -> value . set_dbg_notify(false);
	else
		self -> variable -> getvalue() . set_dbg_notify(false);
	
	MCDebugDeallocate(self);
}

static bool MCVariableInfoWatch(MCVariableInfo* self, MCHashentry *p_element, MCVariableInfo*& r_info)
{
	if (self -> variable_count == 4095)
		return true;
	
	if (!MCDebugReallocate(self -> variables, (self -> variable_count + 1) * sizeof(MCVariableInfo *), self -> variables))
		return false;
	
	MCVariableInfo *t_info;
	if (!MCVariableInfoCreate(self, kMCVariableScopeElement, p_element, t_info))
		return false;
	
	self -> variables[self -> variable_count] = t_info;
	self -> variable_count += 1;
	
	r_info = t_info;
	
	return true;
}

static void MCVariableInfoUnwatch(MCVariableInfo* self, MCHashentry *p_element)
{
	for(uint32_t i = 0; i < self -> variable_count; i++)
	{
		if (self -> variables[i] -> element == p_element)
		{
			MCVariableInfoDestroy(self -> variables[i]);
			memmove(self -> variables + i, self -> variables + i + 1, (self -> variable_count - i - 1) * sizeof(MCVariableInfo *));
			self -> variable_count -= 1;
			break;
		}
	}
}

static MCVariableValue *MCVariableInfoGetValue(MCVariableInfo *self)
{
	if (self -> scope == kMCVariableScopeElement)
		return &self -> element -> value;
	
	return &self -> variable -> getvalue();
}

static uint32_t MCVariableInfoCountDescendents(MCVariableInfo* self)
{
	uint32_t t_count;
	t_count = 0;
	
	if (self -> is_expanded)
	{
		t_count += MCVariableInfoGetValue(self) -> get_array() -> getnfilled();
		for(uint32_t i = 0; i < self -> variable_count; i++)
			t_count += MCVariableInfoCountDescendents(self -> variables[i]);
	}
	
	return t_count;
}

static bool MCVariableInfoIsArray(MCVariableInfo *self)
{
	if (self -> scope == kMCVariableScopeElement)
		return self -> element -> value . is_array();
	
	return self -> variable -> getvalue() . is_array();
}

static int compare_variable_info(const void *pa, const void *pb)
{
	const MCVariableInfo **a, **b;
	a = (const MCVariableInfo **)pa;
	b = (const MCVariableInfo **)pb;
	return strcmp((*a) -> element -> string, (*b) -> element -> string);
}

static void MCVariableInfoSort(MCVariableInfo* self, bool p_recurse)
{
	qsort(self -> variables, self -> variable_count, sizeof(MCVariableInfo *), compare_variable_info);
}

////////////////////////////////////////////////////////////////////////////////

// Create a variable view
static bool MCVariableViewCreate(MCVariableView*& r_view)
{
	MCVariableView *self;
	if (!MCDebugAllocate(sizeof(MCVariableView), self))
		return false;
	
	self -> variables = NULL;
	self -> variable_count = 0;
	
	self -> view_changed = true;
	self -> view_length = 0;
	self -> view = NULL;
	
	r_view = self;
	
	return true;
}

// Destroy a variable view
static void MCVariableViewDestroy(MCVariableView* self)
{
	// Destroy the variable info items
	for(uint32_t i = 0; i < self -> variable_count; i++)
		MCVariableInfoDestroy(self -> variables[i]);
	
	// Destroy the variables array
	MCDebugDeallocate(self -> variables);
	
	// Destroy the window array
	MCDebugDeallocate(self -> view);
	
	// Destroy the view
	MCDebugDeallocate(self);
}

// Add a variable to watch in the view
static bool MCVariableViewWatch(MCVariableView* self, MCVariableScope p_scope, MCVariable *p_variable)
{ 
	if (!MCDebugReallocate(self -> variables, sizeof(MCVariableInfo *) * (self -> variable_count + 1), self -> variables))
		return false;
	
	MCVariableInfo *t_info;
	if (!MCVariableInfoCreate(NULL, p_scope, p_variable, t_info))
		return false;
	
	self -> variables[self -> variable_count] = t_info; 
	self -> variable_count += 1;
	
	return true;
}

static void MCVariableViewLookupEntry(MCVariableView* self, uint32_t p_index, MCVariableInfo*& r_owner, MCHashentry*& r_element)
{
	r_owner = self -> view[p_index] . owner;
	r_element = self -> view[p_index] . element;
}

// Change the expand/collapse state of an item in the current window
static bool MCVariableViewTwiddle(MCVariableView* self, uint32_t p_index, bool p_expand)
{
	if (p_index >= self -> view_length)
		return true;
	
	MCVariableInfo *t_owner;
	MCHashentry *t_element;
	MCVariableViewLookupEntry(self, p_index, t_owner, t_element);
	
	// Check to see if we are manipulating an existing watched element
	if (t_element == NULL)
	{
		// Do nothing if nothing has changed
		if (t_owner -> is_expanded == p_expand)
			return true;
		
		// Do nothing if the target is not an array
		if (!MCVariableInfoIsArray(t_owner))
			return true;
		
		// If we are unexpanding and there are no other expanded keys in the owner,
		// unwatch it.
		if (!p_expand && t_owner -> owner != NULL && t_owner -> variable_count == 0)
		{
			self -> view_changed = true;

			MCVariableInfoUnwatch(t_owner -> owner, t_owner -> element);
			
			return true;
		}
		
		t_owner -> is_expanded = p_expand;
		self -> view_changed = true;
		
		return true;
	}
	
	// We are looking at an unwatched element, so watch it if expanding
	if (p_expand)
	{
		// Do nothing if the element is not an array
		if (!t_element -> value . is_array())
			return true;
		
		// Watch it
		MCVariableInfo *t_watched_var;
		if (!MCVariableInfoWatch(t_owner, t_element, t_watched_var))
			return false;
		
		t_watched_var -> is_expanded = true;
		self -> view_changed = true;
		
		// Make sure we sort the owner
		MCVariableInfoSort(t_owner, false);
		
		return true;
	}
	
	return true;
}

static int compare_hashentries(const void *pa, const void *pb)
{
	const MCHashentry **a, **b;
	a = (const MCHashentry **)pa;
	b = (const MCHashentry **)pb;
	
	return strcmp((*a) -> string, (*b) -> string);
}

static uint32_t MCVariableViewInfoRender(MCVariableInfo* self, MCVariableEntry *p_view)
{
	uint32_t t_index;
	t_index = 0;
	
	// First output the variable's line
	p_view[t_index] . owner = self;
	p_view[t_index] . element = NULL;
	t_index += 1;
	
	// If the view isn't expanded, we are done
	if (!self -> is_expanded)
		return t_index;
	
	// Next enumerate all elements in the array...
	MCVariableArray *t_array;
	t_array = self -> scope == kMCVariableScopeElement ? self -> element -> value . get_array() : self -> variable -> getvalue() . get_array();
	
	MCHashentry **t_elements;
	uint32_t t_element_count;
	t_element_count = t_array -> getnfilled();
	t_elements = (MCHashentry **)(p_view + t_index);
	t_array -> listelements(t_elements);
	
	// Now sort them
	qsort(t_elements, t_element_count, sizeof(MCHashentry *), compare_hashentries);
	
	for(uint32_t i = t_element_count; i > 0; i--)
		p_view[t_index + i - 1] . element = t_elements[i - 1];
	
	for(uint32_t i = 0; i < t_element_count; i++)
		p_view[t_index + i] . owner = self;

	// Iterate through the elements, comparing with the expanded children of self
	uint32_t t_var_index;
	t_var_index = 0;
	while(t_var_index < self -> variable_count)
	{
		// If we aren't the next expanded var then continue
		if (p_view[t_index] . element != self -> variables[t_var_index] -> element)
		{
			t_element_count -= 1;
			t_index += 1;
			continue;
		}
		
		// Advance to the next element
		t_element_count -= 1;
		
		// Compute the size of its descendents
		uint32_t t_desc_count;
		t_desc_count = MCVariableInfoCountDescendents(self -> variables[t_var_index]);
		
		// Shift the remaining elements
		memmove(p_view + t_index + t_desc_count + 1, p_view + t_index + 1, t_element_count * sizeof(MCVariableEntry));
		
		// Generate the children
		t_index += MCVariableViewInfoRender(self -> variables[t_var_index], p_view + t_index);

		// Move to next expanded variable
		t_var_index += 1;
	}
	
	// Advance index by the remaining element count
	t_index += t_element_count;
	
	// index is now how many entries we used
	return t_index;
}

static bool MCVariableViewUpdate(MCVariableView* self)
{
	// Do nothing if the structure hasn't changed.
	if (!self -> view_changed)
		return true;
	
	// Compute the new length of the view.
	uint32_t t_view_length;
	t_view_length = 0;
	for(uint32_t i = 0; i < self -> variable_count; i++)
	{
		// The variable's entry
		t_view_length += 1;
		
		// The variable's descendents entry
		t_view_length += MCVariableInfoCountDescendents(self -> variables[i]);
	}
	
	// Resize the view vector
	if (!MCDebugReallocate(self -> view, sizeof(MCVariableEntry) * t_view_length, self -> view))
		return false;
	
	self -> view_length = t_view_length;
	
	// Now build the thing
	uint32_t t_index;
	t_index = 0;
	for(uint32_t i = 0; i < self -> variable_count; i++)
		t_index += MCVariableViewInfoRender(self -> variables[i], self -> view + t_index);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCVariableView *s_variable_view = NULL;

static MCVariableValue **s_variable_deletions = NULL;
static uint32_t s_variable_deletion_frontier = 0;
static uint32_t s_variable_deletion_capacity = 0;

////////////////////////////////////////////////////////////////////////////////

static int compare_variable_deletion(const void *a, const void *b)
{
	uintptr_t *ae, *be;
	ae = (uintptr_t *)a;
	be = (uintptr_t *)b;
	return (int)(*ae - *be);
}

static bool MCVariableInfoFlush(MCVariableInfo *self)
{
	// This will be true if a child element has changed structure.
	bool t_changed;
	t_changed = false;
	
	for(uint32_t i = 0; i < self -> variable_count; i++)
	{
		// This will be true if we should delete this entry
		bool t_should_delete;
		t_should_delete = false;
		
		// Check each expanded var's value
		MCVariableValue *t_value;
		t_value = &self -> variables[i] -> element -> value;
		if (bsearch(&t_value, s_variable_deletions, s_variable_deletion_frontier, sizeof(uintptr_t), compare_variable_deletion) == NULL)
		{
			// It hasn't been deleted, so if it is still an array, then flush it
			if (t_value -> is_array())
			{
				t_changed = MCVariableInfoFlush(self -> variables[i]);
				
				// If we are not currently expanded, and we (now) have no children we
				// should delete.
				if (!self -> variables[i] -> is_expanded && self -> variables[i] -> variable_count == 0)
					t_should_delete = true;
				else if (self -> variables[i] -> is_expanded && t_value -> get_dbg_mutated())
					t_changed = true;
				
				// The mutation has been noted
				t_value -> set_dbg_mutated(false);
			}
			else
				t_should_delete = true;
			
			if (!t_should_delete)
				continue;
		}
		
		// We found that it has been deleted
		MCVariableInfoDestroy(self -> variables[i]);
		memmove(self -> variables + i, self -> variables + i + 1, (self -> variable_count - i - 1) * sizeof(MCVariableInfo *));
		self -> variable_count -= 1;
		
		// A child has changed
		t_changed = true;
	}
	
	// Only iterate change back up if we are expanded
	return t_changed && self -> is_expanded;
}

void MCVariableViewFlush(void)
{
	// First sort the change list.
	qsort(s_variable_deletions, s_variable_deletion_frontier, sizeof(uintptr_t), compare_variable_deletion);
	
	// Now flush all the children
	for(uint32_t i = 0; i < s_variable_view -> variable_count; i++)
		if (MCVariableInfoFlush(s_variable_view -> variables[i]))
			s_variable_view -> view_changed = true;
	
	s_variable_deletion_frontier = 0;
}

////////

// Notification received when a value is destroyed
void MCDebugNotifyValueDeleted(MCVariableValue *p_value)
{
	if (s_variable_deletion_frontier == s_variable_deletion_capacity)
		MCVariableViewFlush();
	
	s_variable_deletions[s_variable_deletion_frontier++] = p_value;
}

////////

void MCVariableViewEnter(MCExecContext &ctxt)
{
	// Allocate a change buffer, if there is none.
	if (s_variable_deletions == NULL)
	{
		if (!MCDebugAllocate(sizeof(uintptr_t) * 1024, s_variable_deletions))
			return;
		
		s_variable_deletion_frontier = 0;
		s_variable_deletion_capacity = 1024;
	}

	if (!MCVariableViewCreate(s_variable_view))
		return;
	
	// First list the handler locals and parameters (if any)
	MCHandler *t_handler;
	t_handler = ctxt.GetHandler();
	if (t_handler != NULL)
	{
		MCVariable **t_vars;
		uint32_t t_var_count;
		
		// Locals
		t_handler -> getvarlist(t_vars, t_var_count);
		for(uint32_t i = 0; i < t_var_count; i++)
			MCVariableViewWatch(s_variable_view, kMCVariableScopeLocal, t_vars[i]);
		
		// Parameters
		t_handler -> getparamlist(t_vars, t_var_count);
		for(uint32_t i = 0; i < t_var_count; i++)
			MCVariableViewWatch(s_variable_view, kMCVariableScopeParameter, t_vars[i]);
		
		// Globals
		t_handler -> getgloballist(t_vars, t_var_count);
		for(uint32_t i = 0; i < t_var_count; i++)
			MCVariableViewWatch(s_variable_view, kMCVariableScopeGlobal, t_vars[i]);
	}
	
	// Script locals
	MCHandlerlist *t_hlist;
	t_hlist = ctxt.GetEP().gethlist();
	for(MCVariable *t_var = t_hlist -> getvars(); t_var != NULL; t_var = t_var -> getnext())
		MCVariableViewWatch(s_variable_view, kMCVariableScopeScript, t_var);
	
	// Now from the handler list
	for(uint32_t i = 0; i < t_hlist -> getnglobals(); i++)
		if (!t_hlist -> getglobal(i) -> getvalue() . get_dbg_notify())
			MCVariableViewWatch(s_variable_view, kMCVariableScopeGlobal, t_hlist -> getglobal(i));
	
	if (!MCVariableViewUpdate(s_variable_view))
	{
		MCVariableViewDestroy(s_variable_view);
		s_variable_view = NULL;
	}
}

void MCVariableViewLeave(void)
{
	if (s_variable_view == NULL)
		return;
	
	MCVariableViewDestroy(s_variable_view);
	s_variable_view = NULL;
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCServerDebugListVariablesCallback)(void *p_context, uint32_t p_index, uint32_t p_depth, char p_scope, bool p_state, const char *p_name, const char *p_value);

uint32_t MCServerDebugCountVariables(void)
{
	if (s_variable_view == NULL)
		return 0;

	return s_variable_view -> view_length;
}

bool MCServerDebugListVariables(uint32_t p_start, uint32_t p_count, MCServerDebugListVariablesCallback p_callback, void *p_context)
{
	if (s_variable_view == NULL)
		return true;
	
	if (p_count > s_variable_view -> view_length)
		p_count = s_variable_view -> view_length;
	
	p_start = MCU_min(p_start, s_variable_view -> view_length - p_count);
	
	MCExecPoint ep;
	for(uint32_t i = p_start; i < p_start + p_count; i++)
	{
		MCVariableInfo *t_owner;
		MCHashentry *t_element;
		MCVariableViewLookupEntry(s_variable_view, i, t_owner, t_element);
		
		const char *t_name_ptr;
        MCAutoPointer<char> temp_name;
		uint32_t t_name_length;
		MCVariableValue *t_value;
		if (t_element == NULL)
		{
			if (t_owner -> scope == kMCVariableScopeElement)
			{
				t_value = &t_owner -> element -> value;
				t_name_ptr = t_owner -> element -> string;
				t_name_length = strlen(t_name_ptr);
			}
			else
			{
				t_value = &t_owner -> variable -> getvalue();
                /* UNCHECKED */ MCStringConvertToCString(MCNameGetString(t_owner -> variable -> getname()), &temp_name);
				t_name_ptr = *temp_name;
				t_name_length = MCStringGetLength(MCNameGetString(t_owner -> variable -> getname());
			}
		}
		else
		{
			t_value = &t_element -> value;
			t_name_ptr = t_element -> string;
			t_name_length = strlen(t_name_ptr);
		}
		
		char t_name[64];
		t_name_length = MCU_min(63U, t_name_length);
		memcpy(t_name, t_name_ptr, t_name_length);
		t_name[t_name_length] = '\0';
		
		uint32_t t_depth;
		t_depth = t_element == NULL ? t_owner -> depth : t_owner -> depth + 1;
		
		bool t_expanded;
		t_expanded = t_element == NULL && t_owner -> is_expanded;
		
		bool t_array;
		t_array = t_value -> is_array();
		
		char t_val[65];
		bool t_truncated;
		t_truncated = false;
		if (t_array)
			t_val[0] = '\0';
		else
		{
			uint32_t t_val_length;
			t_value -> fetch(ep);
			t_val_length = ep . getsvalue() . getlength();
			
			t_truncated = t_val_length > 64;
			if (t_truncated)
				t_val_length = 64;
			memcpy(t_val, ep . getsvalue() . getstring(), t_val_length);
			t_val[t_val_length] = '\0';
			if (strlen(t_val) < t_val_length)
				t_truncated = true;
			
			char *t_break;
			t_break = strchr(t_val, '\n');
			if (t_break != NULL)
			{
				*t_break = '\0';
				t_truncated = true;
			}
		}

		char t_scope;
		if (t_element != NULL)
			t_scope = 'E';
		else
		{
			switch(t_owner -> scope)
			{
				case kMCVariableScopeElement: t_scope = 'E'; break;
				case kMCVariableScopeLocal: t_scope = 'L'; break;
				case kMCVariableScopeParameter: t_scope = 'P'; break;
				case kMCVariableScopeScript: t_scope = 'S'; break;
				case kMCVariableScopeGlobal: t_scope = 'G'; break;
			}
		}
		
		if (!p_callback(p_context, i, t_depth, t_scope, t_array ? t_expanded : t_truncated, t_name, t_array ? NULL : t_val))
			return false;
	}
						
	return true;
}

void MCServerDebugTwiddleVariables(uint32_t *p_indices, uint32_t p_index_count, bool p_expand)
{
	if (s_variable_view == NULL)
		return;
	
	if (MCVariableViewTwiddle(s_variable_view, p_indices[0], p_expand))
		MCVariableViewUpdate(s_variable_view);
}

////////////////////////////////////////////////////////////////////////////////
