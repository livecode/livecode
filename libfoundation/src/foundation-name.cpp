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

#include <foundation.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCNameRef kMCEmptyName;
MC_DLLEXPORT_DEF MCNameRef kMCTrueName;
MC_DLLEXPORT_DEF MCNameRef kMCFalseName;

////////////////////////////////////////////////////////////////////////////////

static MCNameRef *s_name_table;
static uindex_t s_name_table_occupancy;
static uindex_t s_name_table_capacity;

////////////////////////////////////////////////////////////////////////////////

static void __MCNameGrowTable(void);
static void __MCNameShrinkTable(void);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
MCNameRef MCNAME(const char *p_string)
{
	MCStringRef t_string;
	t_string = MCSTR(p_string);
	
	MCNameRef t_name;
	/* UNCHECKED */ MCNameCreate(t_string, t_name);
	
	MCValueRef t_name_unique;
	/* UNCHECKED */ MCValueInter(t_name, t_name_unique);
	
	MCValueRelease(t_name);
	
	return (MCNameRef)t_name_unique;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNameCreate(MCStringRef p_string, MCNameRef& r_name)
{
	__MCAssertIsString(p_string);

	if (p_string -> char_count == 0 && kMCEmptyName != nil)
	{
		MCValueRetain(kMCEmptyName);
		r_name = kMCEmptyName;
		return true;
	}

	// Compute the has of the characters, up to case.
	hash_t t_hash;
	t_hash = MCStringHash(p_string, kMCStringOptionCompareCaseless);

	// Calculate the index of the chain in the name table where this might be
	// found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representation of the would-be name's equivalence
	// class.
	__MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the
		// full hash first.
		if (t_hash == t_key_name -> hash &&
			MCStringIsEqualTo(p_string, t_key_name -> string, kMCStringOptionCompareCaseless))
			break;

		// Otherwise skip all other members of the same equivalence class.
		while(t_key_name -> next != nil &&
				t_key_name -> key == t_key_name -> next -> key)
			t_key_name = t_key_name -> next;

		// Next name must be the next one.
		t_key_name = t_key_name -> next;
	}

	// Now search within the equivalence class for one with the same string and
	// return immediately if we find a match.
	__MCName *t_name;
	for(t_name = t_key_name; t_name != nil && t_name -> key == t_key_name; t_name = t_name -> next)
		if (MCStringIsEqualTo(p_string, t_name -> string, kMCStringOptionCompareExact))
		{
			t_name -> references += 1;
			r_name = t_name;
			return true;
		}

	// We haven't found an exact match, so we create a new name...
	bool t_success;
	t_success = true;

	// Allocate a name record.
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeName, t_name);

	// Copy the string (as immutable).
	if (t_success)
		t_success = MCStringCopy(p_string, t_name -> string);

	// Now add the name to the table and fill in the rest of the fields.
	if (t_success)
	{
		// If there is no existing equivalence class, we chain at the start,
		// otherwise we insert the name after the representative.
		if (t_key_name == nil)
		{
			// To keep hashin efficient, we (try to) double the size of the
			// table each time occupancy reaches capacity.
			if (s_name_table_occupancy == s_name_table_capacity)
			{
				__MCNameGrowTable();
				t_index = t_hash & (s_name_table_capacity - 1);
			}

			// Increase occupancy.
			s_name_table_occupancy += 1;

			t_name -> next = s_name_table[t_index];
			t_name -> key = t_name;
			s_name_table[t_index] = t_name;
		}
		else
		{
			t_name -> next = t_key_name -> next;
			t_name -> key = t_key_name;
			t_key_name -> next = t_name;

			// Increment the reference count of the representative as we need
			// it to 'hang around' for the entire lifetime of all others in the
			// equivalence class to give a search handle.
			t_key_name -> references += 1;
		}

		// Record the hash (speeds up searching and such).
		t_name -> hash = t_hash;

		// Return the new name.
		r_name = t_name;
	}
	else
	{
		MCValueRelease(t_name -> string);
		MCMemoryDelete(t_name);
	}

	return t_success;
}

MC_DLLEXPORT_DEF
bool MCNameCreateWithNativeChars(const char_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStringCreateWithNativeChars(p_chars, p_count, t_string))
		return false;
	if (!MCNameCreateAndRelease(t_string, r_name))
	{
		MCValueRelease(t_string);
		return false;
	}
	return true;
}

MC_DLLEXPORT_DEF
bool MCNameCreateWithChars(const unichar_t *p_chars, uindex_t p_count, MCNameRef& r_name)
{
	MCStringRef t_string;
	if (!MCStringCreateWithChars(p_chars, p_count, t_string))
		return false;
	return MCNameCreateAndRelease(t_string, r_name);
}

MC_DLLEXPORT_DEF
bool MCNameCreateAndRelease(MCStringRef p_string, MCNameRef& r_name)
{
	if (MCNameCreate(p_string, r_name))
	{
		MCValueRelease(p_string);
		return true;
	}

	return false;
}

MC_DLLEXPORT_DEF
MCNameRef MCNameLookupCaseless(MCStringRef p_string)
{
	// Compute the hash of the characters, up to case.
	hash_t t_hash;
	t_hash = MCStringHash(p_string, kMCStringOptionCompareCaseless);

	// Calculate the index of the chain in the name table where this name might
	// be found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representative of the would-be name's equivalence class.
	__MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the full
		// hash first.
		if (t_hash == t_key_name -> hash &&
			MCStringIsEqualTo(p_string, t_key_name -> string, kMCStringOptionCompareCaseless))
			break;

		// Otherwise skip all other members of the same equivalence class.
		while(t_key_name -> next != nil && t_key_name -> key == t_key_name -> next -> key)
			t_key_name = t_key_name -> next;

		// Next name must be the next one
		t_key_name = t_key_name -> next;
	}

	return t_key_name;
}

MC_DLLEXPORT_DEF
uintptr_t MCNameGetCaselessSearchKey(MCNameRef self)
{
	__MCAssertIsName(self);
	return (uintptr_t)self -> key;
}

MC_DLLEXPORT_DEF
MCStringRef MCNameGetString(MCNameRef self)
{
	__MCAssertIsName(self);
	return self -> string;
}

MC_DLLEXPORT_DEF
bool MCNameIsEmpty(MCNameRef self)
{
	return self == kMCEmptyName;
}

MC_DLLEXPORT_DEF
bool MCNameIsEqualToCaseless(MCNameRef self, MCNameRef p_other_name)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

	return self == p_other_name ||
			self -> key == p_other_name -> key;
}

MC_DLLEXPORT_DEF
bool MCNameIsEqualTo(MCNameRef self, MCNameRef p_other_name, MCStringOptions p_options)
{
	__MCAssertIsName(self);
	__MCAssertIsName(p_other_name);

    if (self == p_other_name)
        return true;

    if (p_options == kMCStringOptionCompareExact)
        return false;
    
    if (p_options == kMCStringOptionCompareCaseless)
        return self -> key == p_other_name -> key;
    
    return MCStringIsEqualTo(self -> string, p_other_name -> string, p_options);
}

////////////////////////////////////////////////////////////////////////////////

void __MCNameDestroy(__MCName *self)
{
	// Compute the index in the table
	uindex_t t_index;
	t_index = self -> hash & (s_name_table_capacity - 1);

	// Find the previous link in the chain
	__MCName *t_previous;
	t_previous = nil;
	for(__MCName *t_name = s_name_table[t_index]; t_name != self; t_name = t_name -> next)
		t_previous = t_name;

	// Update the previous name's next field
	if (t_previous == nil)
		s_name_table[t_index] = self -> next;
	else
		t_previous -> next = self -> next;

	// If this name is not the key then remove our reference to it. Otherwise
	// adjust occupancy appropriately.
	if (self -> key != self)
		MCValueRelease(self -> key);
	else
	{
		// Reduce occupancy of the table
		s_name_table_occupancy -= 1;

		// If the table is too sparse, reduce its size (current heuristic has the
		// threshold at 33%).
		if (s_name_table_capacity > 1024 && s_name_table_occupancy * 16 / s_name_table_capacity < 5)
			__MCNameShrinkTable();
	}

	// Delete the resources
	MCValueRelease(self -> string);
}

bool __MCNameCopyDescription(__MCName *self, MCStringRef& r_string)
{
	return MCStringCopy(self -> string, r_string);
}

hash_t __MCNameHash(__MCName *self)
{
	return self -> hash;
}

bool __MCNameIsEqualTo(__MCName *self, __MCName *p_other_self)
{
	return self == p_other_self;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCNameRelocateTableEntries(uindex_t p_start, uindex_t p_finish, uindex_t p_new_capacity)
{
	// Loop through the entries, moving them as necessary. As the hash-table
	// grows by powers of two, an entry either stays put or moves to outside
	// the range of consideration.
	for(uindex_t t_old_index = p_start; t_old_index < p_finish; t_old_index++)
	{
		// The last name in the chain we previously processed which might
		// get its 'next' field updated.
		MCNameRef t_previous;
		t_previous = nil;

		// The first name in the current chain being processed.
		MCNameRef t_first;
		t_first = s_name_table[t_old_index];
		while(t_first != nil)
		{
			// Compute the new index.
			uindex_t t_new_index;
			t_new_index = t_first -> hash & (p_new_capacity - 1);

			// Search for the last name to relocate.
			MCNameRef t_last;
			for(t_last = t_first; t_last -> next != nil; t_last = t_last -> next)
				if ((t_last -> next -> hash & (p_new_capacity - 1)) != t_new_index)
					break;

			// Record the next name to process after this chain.
			MCNameRef t_next;
			t_next = t_last -> next;

			// If the new index is different from the old, then we need to move
			// the names from first->last.
			if (t_new_index != t_old_index)
			{
				// Remove the link from the previous name, or the table at the
				// old index.
				if (t_previous != nil)
					t_previous -> next = t_last -> next;
				else
					s_name_table[t_old_index] = t_last -> next;

				// Push the chain of names onto the front of the table at the
				// new index.
				t_last -> next = s_name_table[t_new_index];
				s_name_table[t_new_index] = t_first;
			}
			else
				t_previous = t_last;

			// Move to the next chain.
			t_first = t_next;
		}
	}
}

static void __MCNameGrowTable(void)
{
	// First attempt to double the table size.
	if (!MCMemoryResizeArray(s_name_table_capacity * 2, s_name_table, s_name_table_capacity))
		return;

	// Now relocate any entries from the used half that need to be moved to the
	// upper half (another bit has become available to determine spread).
	__MCNameRelocateTableEntries(0, s_name_table_capacity / 2, s_name_table_capacity);
}

static void __MCNameShrinkTable(void)
{
	// First relocate any entries from the upper half of the table to the lower
	// half (a bit has been removed from that which determines spread).
	__MCNameRelocateTableEntries(s_name_table_capacity / 2, s_name_table_capacity, s_name_table_capacity / 2);

	// Now resize the array.
	MCMemoryResizeArray(s_name_table_capacity / 2, s_name_table, s_name_table_capacity);
}

////////////////////////////////////////////////////////////////////////////////

bool __MCNameInitialize(void)
{
	if (!MCMemoryNewArray(1024, s_name_table, s_name_table_capacity))
		return false;

	if (!MCNameCreate(kMCEmptyString, kMCEmptyName))
		return false;

	if (!MCNameCreate(kMCTrueString, kMCTrueName))
		return false;
		
	if (!MCNameCreate(kMCFalseString, kMCFalseName))
		return false;

	s_name_table_occupancy = 0;

	return true;
}

void __MCNameFinalize(void)
{
	MCValueRelease(kMCEmptyName);
	kMCEmptyName = nil;

	MCMemoryDeleteArray(s_name_table);
	s_name_table = nil;
	s_name_table_capacity = 0;
	s_name_table_occupancy = 0;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __DUMP
void MCNameDumpTable(void)
{
	FILE *t_output;
	t_output = fopen("C:\\Users\\Mark\\Desktop\\mcnamedump.txt", "w");
	if (t_output == nil)
		return;

	uint32_t t_singleton_count, t_variants_count;
	t_singleton_count = 0;
	t_variants_count = 0;

	for(uint32_t i = 0; i < s_name_table_capacity; i++)
	{
		if (s_name_table[i] == nil)
			continue;

		MCNameRef t_name;
		t_name = s_name_table[i];
		while(t_name != nil)
		{
			if (t_name -> references == 1)
				t_singleton_count++;

			MCNameRef t_other_name;
			fprintf(t_output, "'%s' (%u)", t_name -> string -> chars, t_name -> references);
			for(t_other_name = t_name -> next; t_other_name != nil && t_other_name -> key == t_name; t_other_name = t_other_name -> next)
			{
				fprintf(t_output, " : '%s' (%u)", t_other_name -> string -> chars, t_other_name -> references);
				t_variants_count++;
			}
			fprintf(t_output, "\n");

			t_name = t_other_name;

		}
	}

	fprintf(t_output, "\nSingleton Count = %u\nVariants Count = %u\n", t_singleton_count, t_variants_count);

	fclose(t_output);
}
#endif

