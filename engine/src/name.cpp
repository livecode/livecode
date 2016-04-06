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

#include "name.h"

////////////////////////////////////////////////////////////////////////////////

MCNameRef kMCEmptyName;

////////////////////////////////////////////////////////////////////////////////

struct MCName
{
	uint32_t references;
	MCName *next;
	MCName *key;
	char *chars;
	uindex_t char_count;
	hash_t hash;
};

////////////////////////////////////////////////////////////////////////////////

static MCNameRef *s_name_table;
static uindex_t s_name_table_occupancy;
static uindex_t s_name_table_capacity;

///////////////////////////////////////////////////////////////////////////////

static bool chars_clone(const char *chars, uindex_t p_char_count, char*& r_chars, uindex_t& r_char_count);
static hash_t chars_hash_caseless(const char *p_chars, uindex_t p_char_count);
static bool chars_equal_caseless(const char *a, uindex_t ac, const char *b, uindex_t bc);
static bool chars_equal(const char *a, uindex_t ac, const char *b, uindex_t bc);

static void MCNameGrowTable(void);
static void MCNameShrinkTable(void);

////////////////////////////////////////////////////////////////////////////////

bool MCNameCreateWithChars(const char *p_chars, uindex_t p_char_count, MCNameRef& r_name)
{
	// Special-case the empty string.
	if (p_char_count == 0 && kMCEmptyName != nil)
		return MCNameClone(kMCEmptyName, r_name);

	// Compute the hash of the characters, up to case.
	hash_t t_hash;
	t_hash = chars_hash_caseless(p_chars, p_char_count);

	// Calculate the index of the chain in the name table where this name might
	// be found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representative of the would-be name's equivalence class.
	MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the full
		// hash first.
		if (t_hash == t_key_name -> hash &&
			chars_equal_caseless(p_chars, p_char_count, t_key_name -> chars, t_key_name -> char_count))
			break;

		// Otherwise skip all other members of the same equivalence class.
		while(t_key_name -> next != nil && t_key_name -> key == t_key_name -> next -> key)
			t_key_name = t_key_name -> next;

		// Next name must be the next one
		t_key_name = t_key_name -> next;
	}

	// Now search within the equivalence class for one with the same string and
	// return immediately if we find a match.
	MCName *t_name;
	for(t_name = t_key_name; t_name != nil && t_name -> key == t_key_name; t_name = t_name -> next)
		if (chars_equal(p_chars, p_char_count, t_name -> chars, t_name -> char_count))
		{
			t_name -> references += 1;
			r_name = t_name;
			return true;
		}

	// We haven't found an exact match, so we create a new name...
	bool t_success;
	t_success = true;

	// Allocate a name record
	if (t_success)
		t_success = MCMemoryNew(t_name);

	// Copy in our string
	if (t_success)
		t_success = chars_clone(p_chars, p_char_count, t_name -> chars, t_name -> char_count);

	// Now add the name to the table.
	if (t_success)
	{
		// If there is no existing equivalence class, we chain at the start,
		// otherwise we insert the name after the representative.
		if (t_key_name == nil)
		{
			// To keep the hashing efficient, we (try to) double the size of the table
			// each time occupancy reaches capacity.
			if (s_name_table_occupancy == s_name_table_capacity)
			{
				MCNameGrowTable();
				t_index = t_hash & (s_name_table_capacity - 1);
			}

			// Increase occupancy
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
			// it to 'hang around' for the entire lifetime of all others in
			// the equivalence class to give a search handle in handler lists
			// etc.
			t_key_name -> references += 1;
		}

		// Fill in the remaining fields
		t_name -> references = 1;
		t_name -> hash = t_hash;

		// And the return value...
		r_name = t_name;
	}
	else
	{
		if (t_name != nil)
			MCMemoryDeleteArray(t_name -> chars);
		MCMemoryDelete(t_name);
	}
	
	return t_success;
}

bool MCNameCreateWithCString(const char *p_cstring, MCNameRef& r_name)
{
	return MCNameCreateWithChars(p_cstring, MCCStringLength(p_cstring), r_name);
}

bool MCNameCreateWithOldString(const MCString& p_string, MCNameRef& r_name)
{
	return MCNameCreateWithChars(p_string . getstring(), p_string . getlength(), r_name);
}

bool MCNameClone(MCNameRef self, MCNameRef& r_new_name)
{
	self -> references += 1;
	r_new_name = self;
	return true;
}

void MCNameDelete(MCNameRef self)
{
	if (self == nil)
		return;

	self -> references -= 1;
	if (self -> references > 0)
		return;

	// Compute the index in the table
	uindex_t t_index;
	t_index = self -> hash & (s_name_table_capacity - 1);

	// Find the previous link in the chain
	MCName *t_previous;
	t_previous = nil;
	for(MCName *t_name = s_name_table[t_index]; t_name != self; t_name = t_name -> next)
		t_previous = t_name;

	// Update the previous name's next field
	if (t_previous == nil)
		s_name_table[t_index] = self -> next;
	else
		t_previous -> next = self -> next;

	// If this name is not the key then remove our reference to it. Otherwise
	// adjust occupancy appropriately.
	if (self -> key != self)
		MCNameDelete(self -> key);
	else
	{
		// Reduce occupancy of the table
		s_name_table_occupancy -= 1;

		// If the table is too sparse, reduce its size (current heuristic has the
		// threshold at 33%).
		if (s_name_table_capacity > 1024 && s_name_table_occupancy * 16 / s_name_table_capacity < 5)
			MCNameShrinkTable();
	}

	// Delete the resources
	MCMemoryDeleteArray(self -> chars);

	// Delete the name's record
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

uintptr_t MCNameGetCaselessSearchKey(MCNameRef self)
{
	return (uintptr_t)self -> key;
}

const char *MCNameGetCString(MCNameRef self)
{
	return self -> chars;
}

MCString MCNameGetOldString(MCNameRef self)
{
	return MCString(self -> chars, self -> char_count);
}

char MCNameGetCharAtIndex(MCNameRef self, uindex_t p_at)
{
	return self -> chars[p_at];
}

////////////////////////////////////////////////////////////////////////////////

bool MCNameIsEmpty(MCNameRef self)
{
	return self -> char_count == 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCNameIsEqualTo(MCNameRef self, MCNameRef p_other_name, MCCompareOptions p_options)
{
	if (self == p_other_name)
		return true;

	if (p_options != kMCCompareExact &&
		self -> key == p_other_name -> key)
		return true;

	return false;
}

bool MCNameIsEqualToChars(MCNameRef self, const char *p_chars, uindex_t p_char_count, MCCompareOptions p_options)
{
	if (self -> char_count != p_char_count)
		return false;

	if (self -> chars == p_chars)
		return true;

	return MCU_strncasecmp(self -> chars, p_chars, p_char_count) == 0;
}

bool MCNameIsEqualToCString(MCNameRef self, const char *p_cstring, MCCompareOptions p_options)
{
	return MCNameIsEqualToChars(self, p_cstring, MCCStringLength(p_cstring), p_options);
}

bool MCNameIsEqualToOldString(MCNameRef self, const MCString& p_string, MCCompareOptions p_options)
{
	return MCNameIsEqualToChars(self, p_string . getstring(), p_string . getlength(), p_options);
}

////////////////////////////////////////////////////////////////////////////////

MCNameRef MCNameLookupWithChars(const char *p_chars, uindex_t p_char_count, MCCompareOptions p_options)
{
	// Compute the hash of the characters, up to case.
	hash_t t_hash;
	t_hash = chars_hash_caseless(p_chars, p_char_count);

	// Calculate the index of the chain in the name table where this name might
	// be found. The capacity is always a power-of-two, so its just a mask op.
	uindex_t t_index;
	t_index = t_hash & (s_name_table_capacity - 1);

	// Search for the first representative of the would-be name's equivalence class.
	MCName *t_key_name;
	t_key_name = s_name_table[t_index];
	while(t_key_name != nil)
	{
		// If the string matches, then we are done - notice we compare the full
		// hash first.
		if (t_hash == t_key_name -> hash &&
			chars_equal_caseless(p_chars, p_char_count, t_key_name -> chars, t_key_name -> char_count))
			break;

		// Otherwise skip all other members of the same equivalence class.
		while(t_key_name -> next != nil && t_key_name -> key == t_key_name -> next -> key)
			t_key_name = t_key_name -> next;

		// Next name must be the next one
		t_key_name = t_key_name -> next;
	}

	// If we just wanted the same name up to caseless equivalence, we are done.
	if (p_options == kMCCompareCaseless || t_key_name == nil)
		return t_key_name;

	// Now search within the equivalence class for one with the same string and
	// return immediately if we find a match.
	for(MCName *t_name = t_key_name; t_name != nil && t_name -> key == t_key_name; t_name = t_name -> next)
		if (chars_equal(p_chars, p_char_count, t_name -> chars, t_name -> char_count))
			return t_name;

	return nil;
}

MCNameRef MCNameLookupWithCString(const char *p_cstring, MCCompareOptions p_options)
{
	return MCNameLookupWithChars(p_cstring, MCCStringLength(p_cstring), p_options);
}

MCNameRef MCNameLookupWithOldString(const MCString& p_string, MCCompareOptions p_options)
{
	return MCNameLookupWithChars(p_string . getstring(), p_string . getlength(), p_options);
}

////////////////////////////////////////////////////////////////////////////////

static void MCNameRelocateTableEntries(uindex_t p_start, uindex_t p_finish, uindex_t p_new_capacity)
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

static void MCNameGrowTable(void)
{
	// First attempt to double the table size.
	if (!MCMemoryResizeArray(s_name_table_capacity * 2, s_name_table, s_name_table_capacity))
		return;

	// Now relocate any entries from the used half that need to be moved to the
	// upper half (another bit has become available to determine spread).
	MCNameRelocateTableEntries(0, s_name_table_capacity / 2, s_name_table_capacity);
}

static void MCNameShrinkTable(void)
{
	// First relocate any entries from the upper half of the table to the lower
	// half (a bit has been removed from that which determines spread).
	MCNameRelocateTableEntries(s_name_table_capacity / 2, s_name_table_capacity, s_name_table_capacity / 2);

	// Now resize the array.
	MCMemoryResizeArray(s_name_table_capacity / 2, s_name_table, s_name_table_capacity);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNameInitialize(void)
{
	MCMemoryNewArray(1024, s_name_table);
	s_name_table_capacity = 1024;
	s_name_table_occupancy = 0;

	MCNameCreateWithChars(nil, 0, kMCEmptyName);

	return true;
}

void MCNameFinalize(void)
{
	MCNameDelete(kMCEmptyName);
	kMCEmptyName = nil;
	
	MCMemoryDeleteArray(s_name_table);

	s_name_table = nil;
	s_name_table_capacity = 0;
	s_name_table_occupancy = 0;
}

////////////////////////////////////////////////////////////////////////////////

static bool chars_clone(const char *p_chars, uindex_t p_char_count, char*& r_chars, uindex_t& r_char_count)
{
	// Allocate a new char array - making sure there's enough room for a NUL
	// byte.
	char *t_new_chars;
	if (!MCMemoryNewArray(p_char_count + 1, t_new_chars))
		return false;

	// Copy the chars, making sure it is NUL-terminated.
	MCMemoryCopy(t_new_chars, p_chars, p_char_count);
	t_new_chars[p_char_count] = '\0';

	r_chars = t_new_chars;
	r_char_count = p_char_count;
	
	return true;
}

static hash_t chars_hash_caseless(const char *p_chars, uindex_t p_char_count)
{
	hash_t t_value;
	t_value = 0;
	while(p_char_count--)
		t_value += (t_value << 3) + MCS_tolower(*p_chars++);
	return t_value;	
}

static bool chars_equal_caseless(const char *a, uindex_t ac, const char *b, uindex_t bc)
{
	if (ac != bc)
		return false;

	return MCU_strncasecmp(a, b, ac) == 0;
}

static bool chars_equal(const char *a, uindex_t ac, const char *b, uindex_t bc)
{
	if (ac != bc)
		return false;
	return MCMemoryEqual(a, b, ac);
}

///////////////////////////////////////////////////////////////////////////////

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
			fprintf(t_output, "'%s' (%u)", t_name -> chars, t_name -> references);
			for(t_other_name = t_name -> next; t_other_name != nil && t_other_name -> key == t_name; t_other_name = t_other_name -> next)
			{
				fprintf(t_output, " : '%s' (%u)", t_other_name -> chars, t_other_name -> references);
				t_variants_count++;
			}
			fprintf(t_output, "\n");

			t_name = t_other_name;

		}
	}

	fprintf(t_output, "\nSingleton Count = %u\nVariants Count = %u\n", t_singleton_count, t_variants_count);

	fclose(t_output);
}

///////////////////////////////////////////////////////////////////////////////
